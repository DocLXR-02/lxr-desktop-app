"""Package and validate the application-only experimental image against its ELF."""
import hashlib, json, struct, subprocess, zipfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
BUILD=ROOT/'build/release'; OUT=ROOT/'build/r6-delivery'
OUT.mkdir(parents=True,exist_ok=True)
b=(BUILD/'LXR_V2_V1.7.img').read_bytes();elf=(BUILD/'LXR_V2_STM32F765.elf').read_bytes()
assert b[:8]==b'LXRV2IMG' and len(b)>16
assert struct.unpack_from('<I',b,8)[0]==len(b)-16
assert b[12]==255-(sum(b[16:])&255) and b[13:16]==bytes(3)
assert b'LXR LAB R6-CLP' in b
sp,reset=struct.unpack_from('<II',b,16)
assert 0x20000000<=sp<=0x20080000 and sp%8==0
assert reset&1 and 0x08008000<=reset<0x08100000
assert elf[:7]==b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H',elf,18)[0]==40
phoff=struct.unpack_from('<I',elf,28)[0];entry,count=struct.unpack_from('<HH',elf,42)
segments=[]
for i in range(count):
    kind,offset,virt,phys,size,mem,flags,align=struct.unpack_from('<8I',elf,phoff+i*entry)
    if kind!=1 or not size:continue
    assert 0x08008000<=phys and phys+size<=0x08100000
    image_offset=16+phys-0x08008000
    assert b[image_offset:image_offset+size]==elf[offset:offset+size]
    segments.append(dict(start=hex(phys),end_exclusive=hex(phys+size),bytes=size))
assert any(int(s['start'],16)<=reset-1<int(s['end_exclusive'],16) for s in segments)
assert max(int(s['end_exclusive'],16) for s in segments)-0x08008000==len(b)-16
image=OUT/'LXR_V2_R6.img';image.write_bytes(b)
archive=OUT/'LXR_V2_R6.zip'
with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    info=zipfile.ZipInfo(image.name,(2026,10,29,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
    info.external_attr=0o100644<<16;z.writestr(info,b)
with zipfile.ZipFile(archive) as z:
    assert z.namelist()==[image.name] and z.read(image.name)==b and z.testzip() is None
result=dict(source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
            vector_stack=hex(sp),vector_reset=hex(reset),load_segments=segments,
            files={p.name:dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in [image,archive]})
(ROOT/'build/r6-validation.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
