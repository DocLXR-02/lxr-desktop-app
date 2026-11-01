"""Isolated delivered-ELF probes, not hardware execution or firmware boot.
Requires unicorn2.1.4 + pyelftools0.32; Cortex-A15 VFP instruction model, as in
ARM triage. Does not model M7 pipeline, interrupts, caches, DMA or performance.
"""
from pathlib import Path
import argparse,hashlib,json,struct
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_THUMB
from unicorn.arm_const import *
p=argparse.ArgumentParser();p.add_argument('--r10',required=True,type=Path);p.add_argument('--r11',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(exist_ok=True,parents=True)
SHAS=['3cc3b78888aaacf4341f9775efcb7c675cb9e11a11954e3501b289f869418f0b','350696b2365c973d07b2670a57c72950a65f9a818090e3599c8487f59d313043']
class Arm:
 def __init__(self,file,digest):
  assert hashlib.sha256(file.read_bytes()).hexdigest()==digest
  self.elf=ELFFile(open(file,'rb'));self.syms={s.name:s for s in self.elf.get_section_by_name('.symtab').iter_symbols()};self.u=Uc(UC_ARCH_ARM,UC_MODE_THUMB);self.u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A15)
  for addr,n in [(0,0x20000),(0x08000000,0x100000),(0x20000000,0x80000),(0xE000E000,0x1000)]:self.u.mem_map(addr,n)
  for s in self.elf.iter_segments():
   if s['p_type']=='PT_LOAD' and s['p_filesz']:self.u.mem_write(s['p_vaddr'],s.data())
  self.u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);self.u.reg_write(UC_ARM_REG_FPEXC,0x40000000);self.u.reg_write(UC_ARM_REG_SP,0x2007f000);self.u.reg_write(UC_ARM_REG_FPSCR,0)
 def addr(self,n):return self.syms[n]['st_value']&~1
 def call(self,n,args=(),floats=()):
  for i,x in enumerate(args[4:]):self.u.mem_write(0x2007f000+4*i,struct.pack('<I',x))
  for reg,x in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):self.u.reg_write(reg,x)
  for i,x in enumerate(floats):self.u.reg_write(UC_ARM_REG_S0+i,struct.unpack('<I',struct.pack('<f',x))[0])
  self.u.reg_write(UC_ARM_REG_LR,0x11001);self.u.emu_start(self.addr(n)|1,0x11000,count=500000);assert self.u.reg_read(UC_ARM_REG_PC)==0x11000
 def osc(self,phase,row=4):
  b=bytearray(40);struct.pack_into('<I',b,8,phase);b[17]=row;struct.pack_into('<f',b,24,1);self.u.mem_write(0x20060000,bytes(b))
 def sample(self):return struct.unpack('<h',self.u.mem_read(0x20061000,2))[0]
def periodic(arm,table,row,phase):
 k=phase>>22;f=phase&0x3fffff;values=struct.unpack('<1024h',arm.u.mem_read(arm.addr(table)+row*2048,2048));return values[k]+(((values[(k+1)&1023]-values[k])*f)>>22)
old,new=[Arm(path,sha) for path,sha in zip([a.r10,a.r11],SHAS)];fm=[];pwm=[];seams=[]
for w,name in enumerate(['sine','triTable','recTable','sawTable','pwm']):
 for mod in [-32768,-100,-1,0,1,100,32767]:
  outputs=[]
  for arm in [old,new]:
   arm.osc(0x12345678);arm.u.mem_write(0x20060800,struct.pack('<h',mod))
   if w==0:arm.call('calcFmSineBlock',[0x20060000,0x20060800,0x20061000,1],[1])
   elif w==4:arm.call('calcPwmFmBlock',[0x20060000,0x20060800,0x20061000,1],[1,.5])
   else:arm.call('calcFmBlock',[0x20060000,arm.addr(name),0x20060800,0x20061000,1],[1])
   outputs.append(arm.sample())
  assert outputs[0]==outputs[1],(name,mod,outputs);fm.append(dict(wave=name,mod=mod,old=outputs[0],new=outputs[1]))
F=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
for row in [4,10]:
 for phase in [0,0x7fffff00,0x7fffffff,0x80000000,0xc0000000,0xffff0000,0xffffffff]:
  for control in [0,.5,1]:
   outputs=[]
   for arm in [old,new]:arm.osc(phase,row);arm.call('calcPwmBlock',[0x20060000,0x20061000,1],[1,control]);outputs.append(arm.sample())
   total=F(F(phase)+F(control*2147483648.));p2=0xffffffff if total>=2**32 else int(total)
   expected=max(-32768,min(32767,periodic(new,'sawTable',row,phase)-periodic(new,'sawTable',row,p2)));assert outputs[1]==expected
   if outputs[0]!=outputs[1]:assert row==10 and (phase>>22==1023 or p2>>22==1023)
   pwm.append(dict(row=row,phase=hex(phase),control=control,old=outputs[0],new=outputs[1]))
for table in ['triTable','recTable','sawTable']:
 for row in range(11):
  for frac in [0,0x200000,0x3fffff]:
   phase=(1023<<22)|frac;new.osc(phase,row);new.call('calcWavetableOscBlock',[0x20060000,new.addr(table),0x20061000,1],[1]);assert new.sample()==periodic(new,table,row,phase);seams.append(dict(table=table,row=row,fraction=frac,value=new.sample()))
transient=[];transient_count=0
for row in range(12):
 data=bytes(new.u.mem_read(new.addr('transientData')+row*2205,2205));idx=next((i for i,x in enumerate(data) if x>=128),0)
 for vol in range(128):
  outputs=[];phases=[]
  for arm in [old,new]:
   t=bytearray(24);struct.pack_into('<If',t,4,idx<<20,1.);t[12]=row+2;struct.pack_into('<f',t,16,F(vol/127.));arm.u.mem_write(0x20060000,bytes(t));arm.call('transient_calcBlock',[0x20060000,0x20061000,1]);outputs.append(arm.sample());phases.append(struct.unpack('<I',arm.u.mem_read(0x20060004,4))[0])
  assert outputs[0]==outputs[1] and phases==[(idx+1)<<20]*2;transient_count+=1
  if vol in [0,1,127]:transient.append(dict(row=row,index=idx,volume=vol,result=outputs[1],phase=phases[1]))
clamp=[]
for x in [-100000,-32768.75,-32768,-1.5,0,1.5,32767,32767.75,100000]:
 new.call('SoloMix_snareSample',[],[x]);v=new.u.reg_read(UC_ARM_REG_R0);v=v-2**32 if v>=2**31 else v;assert v==max(-32768,min(32767,int(x)));clamp.append(dict(input=x,result=v))
r=dict(model='Unicorn2.1.4 Cortex-A15 VFP instruction model, not device execution',elf_sha256=SHAS,fm=fm,pwm=pwm,periodic_seams=seams,snare_clamp=clamp,transient_comparisons=transient_count,transient_examples=transient)
(a.output/'arm-results.json').write_text(json.dumps(r,indent=2)+'\n');print('PASS actual R10/R11 ELF:35 FM,42 PWM outputs(7 declared seam changes),99 periodic seams,1536 transient comparisons,9 snare conversion boundaries')
