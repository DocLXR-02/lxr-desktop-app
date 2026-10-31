"""Verify the existing GitHub evidence wrapper, source archive and exact ELF."""
from pathlib import Path
import argparse,hashlib,io,json,subprocess,zipfile
p=argparse.ArgumentParser();p.add_argument('--artifact',type=Path,required=True);p.add_argument('--repo',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();pin='8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7';raw=a.artifact.read_bytes();sha=lambda b:hashlib.sha256(b).hexdigest()
assert len(raw)==8941901 and sha(raw)=='e5c6ca009efb0be182fcbd73711ff1e17372be5e3194dae256157d0bfee4839a'
with zipfile.ZipFile(io.BytesIO(raw)) as z:
 assert z.testzip() is None and z.read('SOURCE_COMMIT.txt').decode().strip()==pin
 source=z.read('SOURCE.zip');elf=z.read('LXR_V2_STM32F765.elf');dis=z.read('DISASSEMBLY.txt')
 assert sha(source)=='abd31c64a5cb90c4c325c5ef8c23ad787631db393f3acc2f3de1353cfb65ad19'
 assert sha(elf)=='3cc3b78888aaacf4341f9775efcb7c675cb9e11a11954e3501b289f869418f0b'
 assert sha(dis)=='d297538e69d5a694a7bfabc962130ac58bfce53d48273907a277f3452e72217b'
 entries=subprocess.check_output(['git','-C',str(a.repo),'ls-tree','-r','-z',pin]).split(b'\0');paths=[]
 with zipfile.ZipFile(io.BytesIO(source)) as s:
  for entry in entries:
   if not entry:continue
   meta,name=entry.split(b'\t');mode,kind,digest=meta.split();assert kind==b'blob';b=s.read(name.decode());paths.append(name.decode());assert hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()==digest.decode()
  assert set(paths)=={x.filename for x in s.infolist() if not x.is_dir()} and len(paths)==410
 result=dict(source_commit=pin,verified_source_blobs=len(paths),artifact_id=13032864252,artifact_bytes=len(raw),artifact_sha256=sha(raw),elf_bytes=len(elf),elf_sha256=sha(elf),disassembly_sha256=sha(dis),toolchain=z.read('TOOLCHAIN.txt').decode().splitlines()[0])
a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
