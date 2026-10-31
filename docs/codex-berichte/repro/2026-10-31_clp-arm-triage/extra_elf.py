from arm_runtime import Arm
from osc_elf import osc,seams,fm
from unicorn import UC_HOOK_CODE
from unicorn.arm_const import *
from pathlib import Path
import struct,json,os,math,os
H=Path(os.environ.get('LXR_TRIAGE_INPUTS',str(Path(__file__).parent/'inputs')));OUT=Path(os.environ['LXR_TRIAGE_OUTPUT'])
F=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]

def interpolate(a,base,row,phase,wrap=False):
 idx=phase>>22;frac=phase&0x3fffff
 x=struct.unpack('<h',a.u.mem_read(base+row*2048+2*idx,2))[0]
 y=struct.unpack('<h',a.u.mem_read(base+row*2048+2*((idx+1)&1023 if wrap else idx+1),2))[0]
 return x+(((y-x)*frac)>>22)

def pwm():
 a=Arm();u=a.u;base=a.addr('sawTable');rows=[];conv=[]
 u.hook_add(UC_HOOK_CODE,lambda u,pc,size,data:conv.append(u.reg_read(UC_ARM_REG_S15)),begin=0x1e24,end=0x1e24)
 for row in [4,10]:
  for phase in [0,0x7fffff00,0x7fffffff,0x80000000,0xc0000000,0xffff0000,0xffffffff]:
   for p in [0,.5,1]:
    conv.clear();osc(a,phase,row=row);a.call('calcPwmBlock',[0x20060000,0x20061000,1],[1,p]);out=struct.unpack('<h',u.mem_read(0x20061000,2))[0]
    total=F(F(phase)+F(p*2147483648.));compat=0xffffffff if total>=2**32 else int(total);mod=(phase+int(F(p*2147483648.)))&0xffffffff
    assert conv==[compat]
    def diff(q):return max(-32768,min(32767,interpolate(a,base,row,phase)-interpolate(a,base,row,q)))
    assert out==diff(compat)
    rows.append(dict(row=row,phase=hex(phase),pwm=p,float_sum=total,elf_phase2=hex(conv[0]),modular_phase2=hex(mod),elf_sample=out,modular_sample=diff(mod)))
 return rows

def transient():
 a=Arm();u=a.u
 snippets=[(0x802394c,2),(0x8023954,4),(0x8023958,4),(0x802395c,4),(0x8023964,4),(0x802396c,4)]
 u.mem_write(0x10000,b''.join(bytes(u.mem_read(p,n)) for p,n in snippets)+b'\x70\x47');count=0
 for v in range(-128,128):
  for vol in range(128):
   g=F(vol/127.);u.reg_write(UC_ARM_REG_R3,v&0xffffffff);u.reg_write(UC_ARM_REG_S11,struct.unpack('<I',struct.pack('<f',g))[0]);a.call(0x10000)
   actual=u.reg_read(UC_ARM_REG_R7);actual=actual-2**32 if actual>=2**31 else actual;expected=int(F((v*256)*g));assert actual==expected;(count:=count+1)
 cases=[]
 table=a.addr('transientData');data=bytes(u.mem_read(table,26460))
 for row in range(12):
  idx=next((i for i in range(2205) if data[row*2205+i]>=128),0)
  for g in [0,1]:
   t=bytearray(24);struct.pack_into('<If',t,4,idx<<20,1.);t[12]=row+2;struct.pack_into('<f',t,16,g);u.mem_write(0x20060000,bytes(t));a.call('transient_calcBlock',[0x20060000,0x20061000,1]);out=struct.unpack('<h',u.mem_read(0x20061000,2))[0];phase=struct.unpack('<I',u.mem_read(0x20060004,4))[0];signed=data[row*2205+idx];signed=signed-256 if signed>=128 else signed;assert out==signed*256*g and phase==(idx+1)<<20
   cases.append(dict(row=row,index=idx,signed=signed,volume=g,elf_sample=out,phase=phase))
 return dict(instruction_cases=count,full_function_cases=cases)

def conversions():
 a=Arm();u=a.u;u.mem_write(0x10000,bytes(u.mem_read(0x18a8,4))+b'\x70\x47');r=[]
 for x in [-32768,-8.91,-.5,0,12.9,4294967040,4294967296,6442450944,float('nan')]:
  u.reg_write(UC_ARM_REG_S15,struct.unpack('<I',struct.pack('<f',x))[0]);u.reg_write(UC_ARM_REG_FPSCR,0);a.call(0x10000);r.append(dict(input=str(x),result=hex(u.reg_read(UC_ARM_REG_S15)),fpscr=hex(u.reg_read(UC_ARM_REG_FPSCR))))
 return r
def row_reachability():
 a=Arm();u=a.u;rows=[]
 for coarse,note,amount in [(70,60,0),(70,96,0),(127,60,10)]:
  b=bytearray(40);b[16]=1;struct.pack_into('<f',b,20,F(1+F(F(F(amount/127.)*F(amount/127.))*32)*.99));struct.pack_into('<f',b,28,1.);struct.pack_into('<H',b,32,(coarse<<8)|64);u.mem_write(0x20060000,bytes(b));a.call('osc_setBaseNote',[0x20060000,note]);a.call('osc_setFreq',[0x20060000]);raw=u.mem_read(0x20060000,40);rows.append(dict(coarse=coarse,note=note,pitch_amount=amount,frequency=struct.unpack_from('<f',raw,12)[0],pitch_mod=struct.unpack_from('<f',raw,20)[0],phase_increment=struct.unpack_from('<I',raw,4)[0],row=raw[17]))
 assert rows[-1]['row']==10 and rows[-1]['phase_increment']<2**32
 return rows

if __name__=='__main__':
 result=dict(model='Unicorn 2.1.4 Cortex-A15 VFP instruction model',row_reachability=row_reachability(),conversions=conversions(),seams=seams(),fm=fm(),pwm=pwm(),transient=transient())
 (OUT/'instruction-results.json').write_text(json.dumps(result,indent=2)+'\n')
 print('PASS',len(result['pwm']),'PWM probes;',result['transient']['instruction_cases'],'transient instruction comparisons;',len(result['transient']['full_function_cases']),'actual transient calls')
 for r in result['pwm']:
  if r['row']==4 and r['pwm']==1:print(r)
