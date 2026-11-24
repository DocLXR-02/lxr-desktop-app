"""Validate two actual ARM outputs; report static resource deltas, not timing."""
import hashlib,json,struct,subprocess
from pathlib import Path
out=Path("build/r25-diag01-evidence")
def inspect(mode):
    p=out/mode
    elf=(p/"LXR_V2_STM32F765.elf").read_bytes()
    img=(p/"LXR_V2_V1.7.img").read_bytes()
    assert elf[:7]==b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H',elf,18)[0]==40
    assert img[:8]==b'LXRV2IMG' and len(img)-16==struct.unpack_from('<I',img,8)[0]
    assert img[12]==255-(sum(img[16:])&255) and img[13:16]==bytes(3)
    phoff=struct.unpack_from('<I',elf,28)[0]
    entry,count=struct.unpack_from('<HH',elf,42)
    segments=[]
    for i in range(count):
        kind,offset,virt,phys,size,mem,flags,align=struct.unpack_from('<8I',elf,phoff+i*entry)
        if kind!=1 or size==0:continue
        assert 0x08008000<=phys and phys+size<=0x08100000
        assert img[16+phys-0x08008000:16+phys-0x08008000+size]==elf[offset:offset+size]
        segments.append(phys+size)
    assert max(segments)-0x08008000==len(img)-16
    shoff=struct.unpack_from('<I',elf,32)[0]
    shsize,shcount,shstr=struct.unpack_from('<HHH',elf,46)
    headers=[struct.unpack_from('<10I',elf,shoff+i*shsize) for i in range(shcount)]
    strings=elf[headers[shstr][4]:headers[shstr][4]+headers[shstr][5]]
    sec={strings[h[0]:strings.index(0,h[0])].decode():(h[3],h[5]) for h in headers if h[0]}
    def span(names,origin):return max(sum(sec[n]) for n in names)-origin
    regions={"ITCM":span([".itcm_text"],0),"DTCM":span([".dtcm",".dtcmz"],0x20000000),
             "SRAM":span([".data",".bss","._user_heap"],0x20020000),"FLASH_payload":len(img)-16}
    capacities={"ITCM":16384,"DTCM":131072,"SRAM":393216,"FLASH_payload":1015808}
    assert all(regions[k]<capacities[k] for k in regions)
    nm=(p/"symbols.txt").read_text()
    assert (" lxr_diag_session_start" in nm)==(mode=="ON")
    assert (" lxr_diagnostics" in nm)==(mode=="ON")
    return {"regions":regions,"headroom":{k:capacities[k]-v for k,v in regions.items()},
            "files":{f.name:{"bytes":f.stat().st_size,"sha256":hashlib.sha256(f.read_bytes()).hexdigest()} for f in p.iterdir() if f.is_file()}}
a,b=inspect("OFF"),inspect("ON")
delta={k:b["regions"][k]-a["regions"][k] for k in a["regions"]}
report={"source_sha":subprocess.check_output(["git","rev-parse","HEAD"],text=True).strip(),
        "OFF":a,"ON":b,"ON_minus_OFF":delta,"runtime":"UNMEASURED","status":"STATIC_ONLY"}
(out/"validation.json").write_text(json.dumps(report,indent=2)+"\n")
print(json.dumps({"OFF":a["regions"],"ON":b["regions"],"delta":delta},indent=2))
