"""Decode matched ELF Thumb entry saves/SP subtraction; not runtime stack.
Only the observed/asserted prologue forms below are accepted.
"""
from pathlib import Path
import hashlib,json,struct,subprocess
rows=[]
for label,folder,image in [('TEST02','r22-test02-ci-evidence','LXR_V2_R22_TEST02.img'),('TEST03','r22-test03-ci-evidence','LXR_V2_R22_TEST03.img')]:
    root=Path('/workspace')/folder;elf=root/'LXR_V2_STM32F765.elf';raw=(root/image).read_bytes()
    symbols=[x.split() for x in subprocess.check_output(['readelf','-sW',str(elf)],text=True).splitlines() if x.split()]
    for name in ['preset_transferVoiceTo','menu_parseEncoder','menu_voiceTransferButton','menu_repaint']:
        symbol=[s for s in symbols if s[-1]==name];assert len(symbol)==1
        address=int(symbol[0][1],16)&~1;offset=16+address-0x08008000
        code=raw[offset:offset+20];words=struct.unpack('<10H',code)
        start=1 if name=='menu_repaint' else 0
        if words[start]==0xe92d:
            mask=words[start+1];saved=mask.bit_count()*4
        else:
            assert words[start]&0xfe00==0xb400
            mask=(words[start]&255)|((words[start]&256)<<6)
            saved=mask.bit_count()*4
        if name=='preset_transferVoiceTo':
            assert words[2]&0xff80==0xb080
            allocated=(words[2]&127)*4
        elif name=='menu_repaint':
            assert words[4]&0xff80==0xb080
            allocated=(words[4]&127)*4
        else:
            # No SUB SP in the fixed prologue. Later path/callee frames excluded.
            assert all(w&0xff80!=0xb080 for w in words[:5]);allocated=0
        rows.append({'candidate':label,'symbol':name,'address':hex(address),'symbol_bytes':int(symbol[0][2]),'entry_hex':code.hex(),'saved_register_mask':hex(mask),'saved_register_bytes':saved,'fixed_sp_subtract_bytes':allocated,'fixed_entry_frame_bytes':saved+allocated,'elf_sha256':hashlib.sha256(elf.read_bytes()).hexdigest()})
delta={n:next(r['fixed_entry_frame_bytes'] for r in rows if r['candidate']=='TEST03' and r['symbol']==n)-next(r['fixed_entry_frame_bytes'] for r in rows if r['candidate']=='TEST02' and r['symbol']==n) for n in {r['symbol'] for r in rows}}
result={'entries':rows,'fixed_entry_deltas':delta,'scope':'Observed Thumb entry PUSH/PUSH.W plus immediate SUB SP only; excludes later path allocation, callers/callees, interrupts, runtime high-water. Selector encoder/renderer are inlined into menu_parseEncoder/menu_repaint.'}
Path(__file__).with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
