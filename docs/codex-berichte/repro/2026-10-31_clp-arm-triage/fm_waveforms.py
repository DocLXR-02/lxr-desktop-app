"""Compare delivered target behavior with compatibility and signed-phase policies."""
from arm_runtime import Arm
from osc_elf import osc
from extra_elf import interpolate,F
from unicorn.arm_const import *
from pathlib import Path
import struct,json,os
H=Path(os.environ.get('LXR_TRIAGE_INPUTS',str(Path(__file__).parent/'inputs')));OUT=Path(os.environ['LXR_TRIAGE_OUTPUT'])

def wave(a,name,phase,row=4):
 if name=='sine':
  base=a.addr('sine_table');i=phase>>20;f=phase&0xfffff;x,y=struct.unpack('<hh',a.u.mem_read(base+i*2,4));return x+(((y-x)*f)>>20)
 return interpolate(a,a.addr(name),row,phase)

def probes():
 r=[]
 for name in ['sine','triTable','recTable','sawTable','pwm']:
  for mod in [-32768,-100,-1,0,1,100,32767]:
   a=Arm();u=a.u;phase=0x12345678;osc(a,phase,row=4,amount=1);u.mem_write(0x20060800,struct.pack('<h',mod))
   if name=='sine':a.call('calcFmSineBlock',[0x20060000,0x20060800,0x20061000,1],[1])
   elif name=='pwm':a.call('calcPwmFmBlock',[0x20060000,0x20060800,0x20061000,1],[1,.5])
   else:a.call('calcFmBlock',[0x20060000,a.addr(name),0x20060800,0x20061000,1],[1])
   actual=struct.unpack('<h',u.mem_read(0x20061000,2))[0];shift=17 if name=='sine' else 19
   def policy(signed):
    p=(phase+((mod if signed else max(0,mod))<<shift))&0xffffffff
    if name!='pwm':return wave(a,name,p)
    # Preserve existing PWM float addition/conversion, varying only FM policy.
    q=F(F(p)+F(.5*2147483648.));q=0xffffffff if q>=2**32 else int(q)
    return max(-32768,min(32767,wave(a,'sawTable',p)-wave(a,'sawTable',q)))
   compat=policy(False);signed=policy(True);assert actual==compat,(name,mod,actual,compat)
   r.append(dict(waveform=name,mod=mod,elf=actual,compatible=compat,signed_modular=signed))
 return r
if __name__=='__main__':
 r=probes();(OUT/'fm-waveform-results.json').write_text(json.dumps(r,indent=2)+'\n');print('PASS 35 delivered-ELF FM probes match compatible one-sided policy')
 for x in r:
  if x['mod']==-32768:print(x)
