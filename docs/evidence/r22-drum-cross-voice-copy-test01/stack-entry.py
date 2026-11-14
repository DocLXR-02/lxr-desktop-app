"""Decode fixed Thumb entry saves/allocation; not a whole-call-tree stack bound."""
from pathlib import Path
import hashlib,json,os,re,struct,subprocess
records=[]
for release,name in [('r21','preset_transferVoice'),('r22','preset_transferVoiceTo')]:
 root=Path('/workspace')/(release+'-ci-evidence')
 elf=root/'LXR_V2_STM32F765.elf';img=root/('LXR_V2_'+release.upper()+'_TEST01.img')
 lines=subprocess.check_output(['readelf','-sW',str(elf)],text=True).splitlines()
 symbol=[line.split() for line in lines if line.split() and line.split()[-1]==name]
 assert len(symbol)==1
 address=int(symbol[0][1],16)&~1;size=int(symbol[0][2])
 raw=img.read_bytes()[16+address-0x08008000:16+address-0x08008000+6]
 push,mask,sub=struct.unpack('<3H',raw)
 assert push==0xe92d and sub&0xff80==0xb080
 saved=mask.bit_count()*4;allocated=(sub&127)*4
 records.append({'release':release,'symbol':name,'address':hex(address),'symbol_bytes':size,'entry_hex':raw.hex(),'saved_register_mask':hex(mask),'saved_register_bytes':saved,'fixed_sp_subtract_bytes':allocated,'fixed_entry_frame_bytes':saved+allocated,'elf_sha256':hashlib.sha256(elf.read_bytes()).hexdigest()})
result={'entries':records,'fixed_entry_delta':records[1]['fixed_entry_frame_bytes']-records[0]['fixed_entry_frame_bytes'],'scope':'Thumb PUSH.W register save plus immediate SUB SP at function entry; excludes caller/callee/interrupt stack and runtime high-water'}
Path(__file__).with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
