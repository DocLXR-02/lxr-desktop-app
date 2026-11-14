from pathlib import Path
from zipfile import ZipFile
import hashlib,json,struct
root=Path('/workspace/r21-ci-evidence');zpath=Path('/workspace/r21-preflight/LXR_R21_TEST01_CI_Evidence_43621319613.zip')
assert hashlib.sha256(zpath.read_bytes()).hexdigest()=='ff0d61158dc2e89e25f85db92342dc09398b919135456a4b8197d8671ab65963'
with ZipFile(zpath) as ci:assert ci.testzip() is None
v=json.loads((root/'validation.json').read_text());assert v['source_sha']=='861c643632e0a88bdc7d60d9410befb3971489fc'
for name,entry in v['files'].items():
 raw=(root/name).read_bytes();assert len(raw)==entry['bytes'] and hashlib.sha256(raw).hexdigest()==entry['sha256']
img=(root/'LXR_V2_R21_TEST01.img').read_bytes();elf=(root/'LXR_V2_STM32F765.elf').read_bytes()
assert img[:8]==b'LXRV2IMG' and struct.unpack_from('<I',img,8)[0]==len(img)-16
assert img[12]==255-(sum(img[16:])&255) and img[13:16]==b'\0\0\0'
assert b'LXR LAB R21' in img and b'LXR LAB R20' not in img
phoff=struct.unpack_from('<I',elf,28)[0];size,count=struct.unpack_from('<HH',elf,42)
for i in range(count):
 kind,offset,virt,phys,filesize,memsize,flags,align=struct.unpack_from('<8I',elf,phoff+i*size)
 if kind!=1 or not filesize:continue
 assert img[16+phys-0x08008000:16+phys-0x08008000+filesize]==elf[offset:offset+filesize]
with ZipFile(root/'LXR_V2_R21_TEST01.zip') as z:
 assert z.namelist()==['LXR_V2_R21_TEST01.img'] and z.testzip() is None
 assert z.read('LXR_V2_R21_TEST01.img')==img
result={'status':'PASS','source_sha':v['source_sha'],'ci_archive_sha256':hashlib.sha256(zpath.read_bytes()).hexdigest(),'boot_string_offset':img.index(b'LXR LAB R21'),'boot_bytes_hex':img[img.index(b'LXR LAB R21'):img.index(b'LXR LAB R21')+12].hex(),'image_bytes':len(img),'payload_bytes':len(img)-16,'checksum_byte':img[12],'zip_contents':['LXR_V2_R21_TEST01.img'],'checks':['CI archive digest and CRC','exact source','IMG/ELF/MAP/ZIP hashes','updater header/length/checksum','R21 boot bytes','ELF load-segment identity','one-image ZIP CRC and byte identity']}
Path('/workspace/r21-preflight/delivery-verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
