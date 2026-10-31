"""Run isolated functions/instructions from the existing ELF; never boot firmware."""
from pathlib import Path
import struct,hashlib,os
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_THUMB,UC_MODE_MCLASS
from unicorn.arm_const import *
ELF=Path(os.environ['LXR_TRIAGE_ELF'])
assert hashlib.sha256(ELF.read_bytes()).hexdigest()=='3cc3b78888aaacf4341f9775efcb7c675cb9e11a11954e3501b289f869418f0b','Wrong ELF'
class Arm:
 def __init__(self,cpu="a15"):
  self.e=ELFFile(open(ELF,'rb'));self.symbols={x.name:x for x in self.e.get_section_by_name('.symtab').iter_symbols()}
  self.u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|(UC_MODE_MCLASS if cpu=="m7" else 0));self.u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_M7 if cpu=="m7" else UC_CPU_ARM_CORTEX_A15)
  for addr,size in [(0,0x20000),(0x08000000,0x100000),(0x20000000,0x80000),(0xE000E000,0x1000)]:self.u.mem_map(addr,size)
  for seg in self.e.iter_segments():
   if seg['p_type']=='PT_LOAD' and seg['p_filesz']:self.u.mem_write(seg['p_vaddr'],seg.data())
  self.u.reg_write(UC_ARM_REG_C1_C0_2,0x00F00000);self.u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
  self.u.mem_write(0xE000ED88,struct.pack('<I',0xF00000));self.u.reg_write(UC_ARM_REG_SP,0x2007f000)
  self.u.reg_write(UC_ARM_REG_FPSCR,0)
 def addr(self,name):return self.symbols[name]['st_value']&~1
 def call(self,name,args=(),floats=(),count=500000):
  for offset,value in enumerate(args[4:]):self.u.mem_write(self.u.reg_read(UC_ARM_REG_SP)+offset*4,struct.pack('<I',value))
  for reg,value in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):self.u.reg_write(reg,value)
  for i,value in enumerate(floats):self.u.reg_write(UC_ARM_REG_S0+i,struct.unpack('<I',struct.pack('<f',value))[0])
  self.u.reg_write(UC_ARM_REG_LR,0x11001);self.u.emu_start((self.addr(name) if isinstance(name,str) else name)|1,0x11000,count=count)
  assert self.u.reg_read(UC_ARM_REG_PC)==0x11000,hex(self.u.reg_read(UC_ARM_REG_PC))
 def f(self,reg):return struct.unpack('<f',struct.pack('<I',self.u.reg_read(reg)))[0]
if __name__=='__main__':
 a=Arm();a.u.mem_write(0x10000,bytes(a.u.mem_read(0x18a8,4))+b'\x70\x47')
 for x in [-32766.,-8.91,-.5,0,12.9,4294967296.,float('nan')]:
  a.u.reg_write(UC_ARM_REG_S15,struct.unpack('<I',struct.pack('<f',x))[0]);a.u.reg_write(UC_ARM_REG_FPSCR,0);a.call(0x10000);print(x,hex(a.u.reg_read(UC_ARM_REG_S15)),hex(a.u.reg_read(UC_ARM_REG_FPSCR)))
