"""Existing delivered ELF filter on native-generated production input captures."""
from arm_runtime import Arm
from unicorn import UC_HOOK_CODE
from unicorn.arm_const import *
from capstone import Cs,CS_ARCH_ARM,CS_MODE_THUMB,CS_MODE_MCLASS
from pathlib import Path
import struct,math,json,os
H=Path(os.environ.get('LXR_TRIAGE_INPUTS',str(Path(__file__).parent/'inputs')));OUT=Path(os.environ['LXR_TRIAGE_OUTPUT'])

def records(path):
 b=path.read_bytes();out=[];i=0
 while i<len(b):
  kind,n=struct.unpack_from('<II',b,i);state=b[i+8:i+40];data=b[i+40:i+40+2*n];i+=40+2*n;out.append((kind,n,state,data))
 assert i==len(b);return out

def replay(name,target_coefficients=False,fpscr=0):
 a=Arm();u=a.u;state=0x20060000;buf=0x20061000;rows=records(H/(name+'.bin'));u.mem_write(state,rows[0][2]);u.reg_write(UC_ARM_REG_FPSCR,fpscr)
 if target_coefficients:
  initial=struct.unpack('<8f',rows[0][2]);setter=Arm('a15');setter.call('SVF_init',[state]);setter.call('SVF_directSetFilterValue',[state],floats=[.9 if name.endswith('C7') else initial[0]/.45]);setter.call('SVF_setReso',[state],floats=[1-initial[2]]);setter.call('SVF_setDrive',[state],floats=[math.sqrt(max(0,(initial[7]-.4)/6))]);setter.call('SVF_recalcFreq',[state]);u.mem_write(state,bytes(setter.u.mem_read(state,32)))
 coeff=list(struct.unpack('<8f',u.mem_read(state,32)));log=[];previous=[None];first=[];sample=[0]
 cs=Cs(CS_ARCH_ARM,CS_MODE_THUMB|CS_MODE_MCLASS)
 start=a.addr('SVF_calcBlockZDF');code=bytes(u.mem_read(start,1180));instructions={start+i:next(cs.disasm(code[i:i+4],start+i),None) for i in range(0,1180,2)}
 def hook(u,pc,size,user):
  prev=previous[0]
  if prev:
   reg=prev[1];bits=u.reg_read(UC_ARM_REG_S0+reg);value=struct.unpack('<f',struct.pack('<I',bits))[0]
   if not math.isfinite(value) and not any(x['result']==str(value) for x in first):first.append(dict(pc=hex(prev[0]),instruction=prev[2],sample=sample[0],result=str(value),bits=hex(bits),s1=str(a.f(UC_ARM_REG_S12))))
  ins=instructions.get(pc);previous[0]=None
  if not ins:return
  if pc in [0x2aea,0x2966]:sample[0]+=1
  if ins.mnemonic.startswith(('vmul.f32','vadd.f32','vsub.f32','vdiv.f32','vfma.f32','vfnms.f32','vfms.f32')):
   reg=ins.op_str.split(',')[0]
   if reg.startswith('s'):previous[0]=(pc,int(reg[1:]),ins.mnemonic+' '+ins.op_str)
 u.hook_add(UC_HOOK_CODE,hook,begin=a.addr('SVF_calcBlockZDF'),end=a.addr('SVF_calcBlockZDF')+1180)
 for block,(kind,n,_,data) in enumerate(rows):
  u.mem_write(buf,data)
  # Fixed controls: production recalc writes the same coefficient at each block.
  a.call('SVF_calcBlockZDF',[state,kind,buf,n]);values=list(struct.unpack('<8f',u.mem_read(state,32)));samples=list(struct.unpack('<'+'h'*n,u.mem_read(buf,n*2)))
  log.append(dict(block=block,state=[str(x) for x in values],peak=max(map(abs,samples)),zero_count=samples.count(0),fpscr=hex(u.reg_read(UC_ARM_REG_FPSCR))))
  if any(not math.isfinite(v) for v in values):break
 poisoned=None
 if any(not math.isfinite(v) for v in values):
  u.mem_write(buf,bytes(n*2));a.call('SVF_calcBlockZDF',[state,kind,buf,n]);poisoned=dict(state=[str(x) for x in struct.unpack('<8f',u.mem_read(state,32))],zero_output=not any(u.mem_read(buf,n*2)))
 return dict(poisoned_zero_followup=poisoned,name=name,target_coefficients=target_coefficients,initial_fpscr=hex(fpscr),model='Cortex-A15 VFP instruction model',coefficients=coeff,first_nonfinite=first,blocks=log)
if __name__=='__main__':
 result=[replay(name,t,f) for name in ['voice1-C7','voice4-C11','voice4-C11-UI3','voice1-C0','voice1-C11'] for t in [False,True] for f in [0,0x3000000]]
 (OUT/'filter-elf-results.json').write_text(json.dumps(result,indent=2)+'\n')
 for r in result:print(r['name'],r['target_coefficients'],r['initial_fpscr'],r['first_nonfinite'],r['blocks'][-1])
