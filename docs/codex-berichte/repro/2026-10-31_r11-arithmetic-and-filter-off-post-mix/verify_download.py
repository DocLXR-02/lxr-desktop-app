"""Independent downloaded-artifact/source/ELF/image verification; no firmware build."""
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,zipfile
p=argparse.ArgumentParser();p.add_argument('--repo',required=True,type=Path);p.add_argument('--image-artifact',required=True,type=Path);p.add_argument('--evidence-artifact',required=True,type=Path);p.add_argument('--image-digest',required=True);p.add_argument('--evidence-digest',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();O=a.output;O.mkdir(parents=True,exist_ok=True)
COMMIT='64263ed25f2e9c7f5d880b493d943bfba29271a8'
def sha(b):return hashlib.sha256(b).hexdigest()
wrappers=[]
for name,file,digest in [('image',a.image_artifact,a.image_digest),('evidence',a.evidence_artifact,a.evidence_digest)]:
 b=file.read_bytes();assert sha(b)==digest.removeprefix('sha256:');dest=O/name;dest.mkdir(exist_ok=True)
 with zipfile.ZipFile(file) as z:
  assert z.testzip() is None
  for member in z.infolist():assert not member.filename.startswith('/') and '..' not in Path(member.filename).parts
  z.extractall(dest)
 wrappers.append(dict(kind=name,bytes=len(b),sha256=sha(b)))
I=O/'image';E=O/'evidence';assert (E/'SOURCE_COMMIT.txt').read_text().strip()==COMMIT
entries=subprocess.check_output(['git','-C',str(a.repo),'ls-tree','-r','-z',COMMIT]).split(b'\0');paths=[]
with zipfile.ZipFile(E/'SOURCE.zip') as z:
 assert z.testzip() is None
 for entry in entries:
  if not entry:continue
  meta,path=entry.split(b'\t');mode,kind,oid=meta.split();assert kind==b'blob';name=path.decode();paths.append(name);b=z.read(name);assert hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()==oid.decode()
 assert {x.filename for x in z.infolist() if not x.is_dir()}==set(paths)
b=(I/'LXR_V2_R11.img').read_bytes();elf=(E/'LXR_V2_STM32F765.elf').read_bytes();assert b[:8]==b'LXRV2IMG';assert struct.unpack_from('<I',b,8)[0]==len(b)-16;assert b[12]==255-(sum(b[16:])&255);assert b[13:16]==bytes(3)
assert b'LXR LAB R11\0' in b and b'LXR LAB R10' not in b and b'LXR LAB R11-CLP' not in b and b'LXR2 Drums V' not in b
sp,reset=struct.unpack_from('<II',b,16);assert 0x20000000<=sp<=0x20080000 and sp%8==0;assert reset&1 and 0x08008000<=reset<0x08100000
assert elf[:7]==b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H',elf,18)[0]==40;assert struct.unpack_from('<I',elf,24)[0]==reset
phoff=struct.unpack_from('<I',elf,28)[0];size,count=struct.unpack_from('<HH',elf,42);end=0x08008000;segments=[]
for i in range(count):
 kind,offset,virtual,physical,n,mem,flags,align=struct.unpack_from('<8I',elf,phoff+i*size)
 if kind!=1 or not n:continue
 assert physical==end and 0x08008000<=physical and physical+n<=0x08100000
 assert b[16+physical-0x08008000:16+physical-0x08008000+n]==elf[offset:offset+n];end=physical+n;segments.append(dict(start=hex(physical),end_exclusive=hex(end),bytes=n))
assert end-0x08008000==len(b)-16 and any(int(x['start'],16)<=reset-1<int(x['end_exclusive'],16) for x in segments)
# Validate every nonreserved vector against actual executable ELF segments.
shoff=struct.unpack_from('<I',elf,32)[0];shsize,shcount,shnames=struct.unpack_from('<HHH',elf,46)
sections=[struct.unpack_from('<10I',elf,shoff+i*shsize) for i in range(shcount)]
name_section=sections[shnames];names=elf[name_section[4]:name_section[4]+name_section[5]]
vector=next(x for x in sections if names[x[0]:].split(b'\0',1)[0]==b'.isr_vector')
assert vector[3]==0x08008000 and vector[5]%4==0
values=struct.unpack_from('<'+'I'*(vector[5]//4),elf,vector[4]);assert values[:2]==(sp,reset)
executable=[]
for i in range(count):
 kind,offset,virtual,physical,n,mem,flags,align=struct.unpack_from('<8I',elf,phoff+i*size)
 if kind==1 and flags&1:executable.append((virtual,virtual+mem))
for v in values[1:]:assert v==0 or (v&1 and any(lo<=v-1<hi for lo,hi in executable))
with zipfile.ZipFile(I/'LXR_V2_R11.zip') as z:assert z.namelist()==['LXR_V2_R11.img'] and z.read('LXR_V2_R11.img')==b and z.testzip() is None
validation=json.loads((E/'r11-validation.json').read_text());assert validation['source_commit']==COMMIT
for name,v in validation['files'].items():data=(I/name).read_bytes();assert len(data)==v['bytes'] and sha(data)==v['sha256']
checked=0
for line in (E/'SHA256SUMS').read_text().splitlines():
 digest,path=line.split();path=Path(path)
 root=I if 'r11-delivery' in path.parts else E
 prefix='build/r11-delivery/' if root==I else 'build/r11-evidence/'
 assert str(path).startswith(prefix);f=root/str(path).removeprefix(prefix);assert sha(f.read_bytes())==digest;checked+=1
result=dict(source_commit=COMMIT,verified_source_blobs=len(paths),checked_manifest_files=checked,wrappers=wrappers,vector_entries=len(values),nonreserved_vectors=sum(v!=0 for v in values[1:]),payload_bytes=len(b)-16,internal_checksum=hex(b[12]),stack=hex(sp),reset=hex(reset),load_segments=segments,files={str(x.relative_to(O)):dict(bytes=x.stat().st_size,sha256=sha(x.read_bytes())) for x in [I/'LXR_V2_R11.img',I/'LXR_V2_R11.zip',E/'SOURCE.zip',E/'LXR_V2_STM32F765.elf',E/'LXR_V2_STM32F765.map',E/'HOST_RESULTS.tar.gz']})
(O/'verified.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
