"""Independently validate downloaded R12 bytes against Git and their ELF.
Requires pyelftools0.32; no build/flash/network action. Paths are explicit.
"""
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,zipfile
from elftools.elf.elffile import ELFFile
P=argparse.ArgumentParser();P.add_argument('--repo',type=Path,required=True);P.add_argument('--evidence',type=Path,required=True);P.add_argument('--image',type=Path,required=True);P.add_argument('--output',type=Path,required=True);a=P.parse_args();E=a.evidence;I=a.image
HEAD='ff47d945d283fd3def4af3aff974bdcafb69ffd3'
assert (E/'SOURCE_COMMIT.txt').read_text().strip()==HEAD
files={}
for line in (E/'SHA256SUMS').read_text().splitlines():
 h,n=line.split(maxsplit=1);p=(I/n.rsplit('/',1)[1]) if n.startswith('build/r12-delivery/') else E/n.removeprefix('build/r12-evidence/');b=p.read_bytes();assert hashlib.sha256(b).hexdigest()==h,n;files[n]=dict(bytes=len(b),sha256=h)
count=0
with zipfile.ZipFile(E/'SOURCE.zip') as z:
 assert z.comment.decode()==HEAD
 expected_names=set()
 for line in subprocess.check_output(['git','-C',str(a.repo),'ls-tree','-r',HEAD],text=True).splitlines():
  meta,n=line.split('\t');expected_names.add(n);mode,kind,h=meta.split();assert kind=='blob';b=z.read(n);assert hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()==h,n;count+=1
 assert {n for n in z.namelist() if not n.endswith('/')}==expected_names
b=(I/'LXR_V2_R12.img').read_bytes();assert b[:8]==b'LXRV2IMG' and struct.unpack_from('<I',b,8)[0]==len(b)-16 and b[12]==255-(sum(b[16:])&255) and b[13:16]==bytes(3)
with zipfile.ZipFile(I/'LXR_V2_R12.zip') as z:assert z.namelist()==['LXR_V2_R12.img'] and z.read(z.namelist()[0])==b and z.testzip() is None
assert b'LXR LAB R12\0' in b and b'LXR LAB R11' not in b and b'LXR2 Drums V' not in b
elf=ELFFile(open(E/'LXR_V2_STM32F765.elf','rb'));segments=[];last=0x08008000
for s in elf.iter_segments():
 if s['p_type']=='PT_LOAD' and s['p_filesz']:
  start=s['p_paddr'];end=start+s['p_filesz'];assert start==last and end<=0x08100000;assert b[16+start-0x08008000:16+end-0x08008000]==s.data();last=end;segments.append(dict(start=hex(start),end_exclusive=hex(end),bytes=s['p_filesz']))
assert last-0x08008000==len(b)-16
symbols={s.name:s for s in elf.get_section_by_name('.symtab').iter_symbols()};reset=symbols['Reset_Handler']['st_value']&~1
sec=next(s for s in elf.iter_sections() if s['sh_addr']<=reset<s['sh_addr']+s['sh_size'] and s['sh_type']=='SHT_PROGBITS');o=reset-sec['sh_addr'];code=sec.data();ins=code[o:o+4];assert ins[:2]==b'\xdf\xf8';word=struct.unpack('<H',ins[2:])[0];assert word>>12==13;lit=((reset+4)&~3)+(word&0xfff);sp=struct.unpack_from('<I',code,lit-sec['sh_addr'])[0];assert sp==0x20020000
vectors=struct.unpack_from('<126I',b,16);assert vectors[0]==0x20080000 and vectors[1]==reset|1
for v in vectors[1:]:assert v==0 or v&1 and (v-1<0x4000 or 0x08008000<=v-1<0x08100000)
sections={s.name:dict(address=s['sh_addr'],bytes=s['sh_size']) for s in elf.iter_sections() if s.name in ['.data','.bss','._user_heap','.dtcm','.dtcmz','.itcm_text']};dtcm=sections['.dtcmz']['address']+sections['.dtcmz']['bytes']-0x20000000;sram=sections['.bss']['address']+sections['.bss']['bytes']-0x20020000
names=['paths','targets','faderTargets','faderReplay','postMix','clipGain','input','isolated','controls','fxBus','fxBus_clipResetSerial','mixer_calcNextSampleBlock','SoloMix_begin','SoloMix_captureFaders','SoloMix_addVoice','SoloMix_process','FxBus_clipCoreGain','FxBus_process','FxBus_processWithClipGain']
occupants={n:dict(address=hex(symbols[n]['st_value']),bytes=symbols[n]['st_size']) for n in names};assert occupants['paths']['bytes']==1056
result=dict(source_commit=HEAD,source_tree=subprocess.check_output(['git','-C',str(a.repo),'rev-parse',HEAD+'^{tree}'],text=True).strip(),source_blobs_verified=count,manifest_files_verified=len(files),image_magic='LXRV2IMG',payload_bytes=len(b)-16,internal_checksum=hex(b[12]),reserved_bytes='000000',vectors=126,nonzero_handlers=sum(v!=0 for v in vectors[1:]),vector_sp=hex(vectors[0]),runtime_sp=hex(sp),reset=hex(vectors[1]),load_segments=segments,sections=sections,resource=dict(itcm=sections['.itcm_text']['bytes'],itcm_free=16384-sections['.itcm_text']['bytes'],dtcm_static_extent=dtcm,dtcm_gap_to_sp=131072-dtcm,sram_data_bss_sum=sections['.data']['bytes']+sections['.bss']['bytes'],sram_aligned_extent=sram,sram_with_heap=sram+sections['._user_heap']['bytes'],printed_flash=239316,full_flash=len(b)-16,physical_app_flash_free=0x08100000-last),occupants=occupants,files=files)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['files','sections','occupants']},indent=2))
