#!/usr/bin/env python3
"""Validate the ARM candidate image and compare static sizes against accepted R22 TEST03.

Not a flash-ready release, real-time measurement or hardware test.
"""
from pathlib import Path
import hashlib
import json
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[2]
BUILD=ROOT/"build/release"
OUT=ROOT/"build/lxr-bug-033-evidence"
OUT.mkdir(parents=True,exist_ok=True)
elf=(BUILD/"LXR_V2_STM32F765.elf").read_bytes()
img=(BUILD/"LXR_V2_V1.7.img").read_bytes()
assert elf[:7]==b"\x7fELF\x01\x01\x01"
assert struct.unpack_from("<H",elf,18)[0]==40  # ARM
assert img[:8]==b"LXRV2IMG"
assert len(img)-16==struct.unpack_from("<I",img,8)[0]
assert img[12]==255-(sum(img[16:])&255) and img[13:16]==bytes(3)
assert b"LXR LAB R22" in img
phoff=struct.unpack_from("<I",elf,28)[0]
entry,count=struct.unpack_from("<HH",elf,42)
segments=[]
for i in range(count):
    kind,offset,virt,phys,size,mem,flags,align=struct.unpack_from("<8I",elf,phoff+i*entry)
    if kind!=1 or size==0: continue
    assert 0x08008000<=phys and phys+size<=0x08100000
    assert img[16+phys-0x08008000:16+phys-0x08008000+size]==elf[offset:offset+size]
    segments.append({"address":phys,"bytes":size,"runtime_address":virt,"memory_bytes":mem})
assert segments
assert max(s["address"]+s["bytes"] for s in segments)-0x08008000==len(img)-16
sp,reset=struct.unpack_from("<II",img,16)
assert sp%8==0 and 0x20000000<=sp<=0x20080000
assert reset&1 and 0x08008000<=reset<0x08100000
shoff=struct.unpack_from("<I",elf,32)[0]
shsize,shcount,shstr=struct.unpack_from("<HHH",elf,46)
headers=[struct.unpack_from("<10I",elf,shoff+i*shsize) for i in range(shcount)]
strings=elf[headers[shstr][4]:headers[shstr][4]+headers[shstr][5]]
sections={strings[h[0]:strings.index(0,h[0])].decode():
          {"address":h[3],"bytes":h[5]} for h in headers if h[0]}
def end_span(names,origin):
    return max(sections[n]["address"]+sections[n]["bytes"] for n in names)-origin
current={".text":sections[".text"]["bytes"],
         ".rodata":sections[".rodata"]["bytes"],
         "ITCM":end_span([".itcm_text"],0),
         "DTCM":end_span([".dtcm",".dtcmz"],0x20000000),
         "SRAM":end_span([".data",".bss","._user_heap"],0x20020000),
         "FLASH_payload":len(img)-16}
# Accepted R22 TEST03: retained CI source 0e37942250f79306627702061d851731bdfdbbc8;
# evidence docs/codex-berichte/2026-11-15_r22-kit-index-display-test03.md
base={".text":160712,".rodata":83272,"ITCM":15304,"DTCM":107424,"SRAM":275164,"FLASH_payload":297664}
capacity={"ITCM":16384,"DTCM":131072,"SRAM":393216,"FLASH_payload":1015808}
regions={k:{"R22_TEST03":base[k],"candidate":v,"delta":v-base[k],
            **({"remaining":capacity[k]-v} if k in capacity else {})}
         for k,v in current.items()}
assert all(v["remaining"]>0 for v in regions.values() if "remaining" in v)
candidate=OUT/"LXR_BUG_033_R22_CANDIDATE_NOT_FOR_FLASHING.img"
candidate.write_bytes(img)
files=[candidate,BUILD/"LXR_V2_STM32F765.elf",BUILD/"LXR_V2_STM32F765.map"]
checks={p.name:{"bytes":p.stat().st_size,"sha256":hashlib.sha256(p.read_bytes()).hexdigest()} for p in files}
result={"source_sha":subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT,text=True).strip(),
        "baseline_sha":"0e37942250f79306627702061d851731bdfdbbc8",
        "files":checks,"regions":regions,"sections":sections,"segments":segments,
        "boot_string":"LXR LAB R22","checksum_byte":img[12],
        "CPU_WCET":"UNMEASURED","IRQ_DMA":"UNMEASURED","stack_high_water":"UNMEASURED",
        "physical_midi":"NOT TESTED","release_status":"NOT FOR FLASHING"}
(OUT/"arm-validation.json").write_text(json.dumps(result,indent=2)+"\n")
print(json.dumps({"files":checks,"regions":regions,"status":"PASS"},indent=2))
