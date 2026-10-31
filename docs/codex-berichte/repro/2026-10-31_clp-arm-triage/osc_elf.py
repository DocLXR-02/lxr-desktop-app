from arm_runtime import Arm
from unicorn import UC_HOOK_MEM_READ,UC_HOOK_CODE
from unicorn.arm_const import *
from pathlib import Path
import struct,json,os,math,os
H=Path(os.environ.get('LXR_TRIAGE_INPUTS',str(Path(__file__).parent/'inputs')));OUT=Path(os.environ['LXR_TRIAGE_OUTPUT'])

def osc(a,phase=0,inc=0,row=0,amount=1):
 b=bytearray(40);struct.pack_into('<II',b,4,inc,phase);b[17]=row;struct.pack_into('<f',b,24,amount);a.u.mem_write(0x20060000,bytes(b))

def seams():
 a=Arm();u=a.u;rows=[]
 for name in ['triTable','recTable','sawTable']:
  base=a.addr(name)
  for row in range(11):
   vals=struct.unpack('<1024h',u.mem_read(base+2048*row,2048));nextval=struct.unpack('<h',u.mem_read(base+2048*(row+1),2))[0]
   for frac in [0,.5,(2**22-1)/2**22]:
    osc(a,(1023<<22)|int(frac*2**22),row=row)
    a.call('calcWavetableOscBlock',[0x20060000,base,0x20061000,1],[1])
    original=struct.unpack('<h',u.mem_read(0x20061000,2))[0]
    f=int(frac*2**22);wrap=vals[-1]+(((vals[0]-vals[-1])*f)>>22);hold=vals[-1]
    # int16 cast occurs before integer addition in production interpolation.
    diff=struct.unpack('<h',struct.pack('<H',(((vals[0]-vals[-1])*f)>>22)&65535))[0];wrap=struct.unpack('<h',struct.pack('<H',(vals[-1]+diff)&65535))[0]
    rows.append(dict(table=name,row=row,frac=frac,last=vals[-1],first=vals[0],physical_next=nextval,elf=original,periodic_wrap=wrap,hold_last=hold))
 return rows

def fm():
 rows=[]
 for mod in [-32768,-100,-1,0,1,100,32767]:
  for amount in [.5,1]:
   a=Arm();u=a.u;converted=[];u.hook_add(UC_HOOK_CODE,lambda u,pc,size,data:converted.append(u.reg_read(UC_ARM_REG_S15)),begin=0x18ac,end=0x18ac);osc(a,0x12345678,amount=amount);u.mem_write(0x20060800,struct.pack('<h',mod));a.call('calcFmSineBlock',[0x20060000,0x20060800,0x20061000,1],[1]);value=struct.unpack('<h',u.mem_read(0x20061000,2))[0]
   rows.append(dict(mod=mod,amount=amount,elf=value,converted_u32=hex(converted[0]),fpscr=hex(u.reg_read(UC_ARM_REG_FPSCR))))
 return rows
if __name__=='__main__':
 r=dict(seams=seams(),fm=fm());(OUT/'osc-elf-results.json').write_text(json.dumps(r,indent=2)+'\n')
 for name in ['triTable','recTable','sawTable']:
  print(name,[(x['row'],x['first'],x['physical_next'],x['elf']-x['periodic_wrap']) for x in r['seams'] if x['table']==name and x['frac']>.99])
 print(r['fm'])
