#!/usr/bin/env python3
"""Verify CI-produced R21 image/ELF identity, updater and static headroom."""
from pathlib import Path
import hashlib, json, struct, subprocess, zipfile
R=Path(__file__).resolve().parents[2]
B=R/'build/release';O=R/'build/r21-voice-copy-evidence'
O.mkdir(parents=True,exist_ok=True)
elf=(B/'LXR_V2_STM32F765.elf').read_bytes()
img=(B/'LXR_V2_V1.7.img').read_bytes()
assert elf[:7]==b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H',elf,18)[0]==40
assert img[:8]==b'LXRV2IMG' and len(img)-16==struct.unpack_from('<I',img,8)[0]
assert img[12]==255-(sum(img[16:])&255) and img[13:16]==bytes(3)
assert b'LXR LAB R21' in img and b'LXR LAB R20' not in img
phoff=struct.unpack_from('<I',elf,28)[0]
entry,count=struct.unpack_from('<HH',elf,42)
segments=[]
for i in range(count):
 kind,offset,virt,phys,size,mem,flags,align=struct.unpack_from('<8I',elf,phoff+i*entry)
 if kind!=1 or size==0:continue
 assert 0x08008000<=phys and phys+size<=0x08100000
 assert img[16+phys-0x08008000:16+phys-0x08008000+size]==elf[offset:offset+size]
 segments.append({'address':phys,'bytes':size,'runtime_address':virt,'memory_bytes':mem})
assert max(s['address']+s['bytes'] for s in segments)-0x08008000==len(img)-16
sp,reset=struct.unpack_from('<II',img,16)
assert sp%8==0 and 0x20000000<=sp<=0x20080000
assert reset&1 and 0x08008000<=reset<0x08100000
shoff=struct.unpack_from('<I',elf,32)[0]
shsize,shcount,shstr=struct.unpack_from('<HHH',elf,46)
headers=[struct.unpack_from('<10I',elf,shoff+i*shsize) for i in range(shcount)]
strings=elf[headers[shstr][4]:headers[shstr][4]+headers[shstr][5]]
sections={strings[h[0]:strings.index(0,h[0])].decode():{'address':h[3],'bytes':h[5]} for h in headers if h[0]}
def end_span(names,origin):return max(sections[n]['address']+sections[n]['bytes'] for n in names)-origin
current={'.text':sections['.text']['bytes'], 'ITCM':end_span(['.itcm_text'],0),
 'DTCM':end_span(['.dtcm','.dtcmz'],0x20000000),
 'SRAM':end_span(['.data','.bss','._user_heap'],0x20020000), 'FLASH_payload':len(img)-16}
# Matched ARM GNU14.3.Rel1 Release, exact R20 SOURCE b015372...
base={'.text':157064,'ITCM':15304,'DTCM':107424,'SRAM':274884,'FLASH_payload':293488}
capacity={'ITCM':16384,'DTCM':131072,'SRAM':393216,'FLASH_payload':1015808}
regions={k:{'R20':base[k],'R21':v,'delta':v-base[k],**({'headroom':capacity[k]-v} if k in capacity else {})} for k,v in current.items()}
assert all(v['headroom']>0 for v in regions.values() if 'headroom' in v)
image=O/'LXR_V2_R21_TEST01.img';image.write_bytes(img)
archive=O/'LXR_V2_R21_TEST01.zip'
with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED) as z:z.write(image,image.name)
with zipfile.ZipFile(archive) as z:assert z.namelist()==[image.name] and z.testzip() is None and z.read(image.name)==img
files=[image,archive,O/'LXR_V2_STM32F765.elf',O/'LXR_V2_STM32F765.map']
records={p.name:{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in files}
result={'source_sha':subprocess.check_output(['git','rev-parse','HEAD'],cwd=R,text=True).strip(),
 'base_sha':'b01537282047bda5299cdb1bedeab5e5d5b9bcca','files':records,'regions':regions,
 'sections':sections,'segments':segments,'boot_string':'LXR LAB R21','checksum_byte':img[12],
 'CPU_WCET':'UNMEASURED','IRQ_DMA':'UNMEASURED','stack_high_water':'UNMEASURED','reliability':'UNMEASURED'}
(O/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'files':records,'regions':regions,'status':'PASS'},indent=2))
