#!/usr/bin/env python3
"""Validate the version-marker-only R19 candidate from exact R18 three-fix integrated source."""
from pathlib import Path
import hashlib, json, re, struct, subprocess, sys

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "build" / "release"
OUT = ROOT / "build" / "r19-candidate-arm-evidence"
ELF = BUILD / "LXR_V2_STM32F765.elf"
IMG = BUILD / "LXR_V2_V1.7.img"
MAP = BUILD / "LXR_V2_STM32F765.map"
R18 = "41b364067bce0d8cde624c184933d9cb88e2725b"
INTEGRATED_PRODUCTION_BLOBS = {
    "Core/Src/Soundengine/DSPAudio/FX/Compressor.c": "7bb84666575c0a35e93e10563aa5b5c559432030",
    "Core/Src/Soundengine/DSPAudio/FX/Delay.c": "1a4bbac4021efe9e0c0d9cfa9bee317e5a51e6e6",
    "Core/Src/Soundengine/USB/usb_manager.c": "7f30ed0fcdb433f280b98bb238879cb28834b585",
    "Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h": "56bb9165ae5d2bf0134a55a4a95652bc706aef77",
    "Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c": "ef33e3e2941db2a8a72705d66d419c29966f4d47",
}
INTEGRATION_BASE_SHA = "cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6"
# Pinned, previously archived R18 release-build section values (CI 41725066117).
R18_SECTIONS = {
    ".isr_vector":504, ".text":151784, ".rodata":82560,
    ".ARM":8, ".init_array":4, ".fini_array":4, ".data":852,
    ".bss":273280, "._user_heap":512,
    ".dtcm":36996, ".dtcmz":70424, ".itcm_text":15208,
}
# Pinned source-matched R18 three-fix ARM CI 42069839191 baseline.
INTEGRATED_SECTIONS = {
    ".isr_vector":504, ".text":152024, ".rodata":82560,
    ".ARM":8, ".init_array":4, ".fini_array":4, ".data":852,
    ".bss":273296, "._user_heap":512,
    ".dtcm":36996, ".dtcmz":70424, ".itcm_text":15208,
}
INTEGRATED_IMAGE_BYTES = 288184
INTEGRATED_FLASH_PAYLOAD = 288168
INTEGRATED_BUILD_COMPILATION_SHA256 = "c66381f02a90e7ef1b2fb37369849e7e8e4ee4c3b689f4942211ef2a6b1acdf8"

def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()
def check(condition, message):
    if not condition: raise AssertionError(message)
def parse_sections(raw):
    result={}
    for line in raw.splitlines():
        m=re.match(r"^(\.[A-Za-z0-9_.]+)\s+(\d+)\s+\d+\s*$",line)
        if m: result[m.group(1)]=int(m.group(2))
    return result

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    head=subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT,text=True).strip()
    check(subprocess.check_output(["git","merge-base",head,INTEGRATION_BASE_SHA],cwd=ROOT,text=True).strip()==INTEGRATION_BASE_SHA,
          "R19 candidate must descend from the exact integrated three-fix commit")
    original_main=subprocess.check_output(["git","show",INTEGRATION_BASE_SHA+":Core/Src/main.c"],cwd=ROOT)
    old=b'lcd_string("LXR LAB R18");'
    new=b'lcd_string("LXR LAB R19");'
    check(original_main.count(old)==1,"Unexpected R18 base marker count")
    check((ROOT/"Core/Src/main.c").read_bytes()==original_main.replace(old,new),
          "Production main.c contains changes beyond the approved R19 startup marker")
    check(b"#define UPDATE_BOOTLOADER 0" in original_main,"Bootloader update guard changed")
    src_blobs={}
    for src,expected in INTEGRATED_PRODUCTION_BLOBS.items():
        found=subprocess.check_output(["git","hash-object",src],cwd=ROOT,text=True).strip()
        check(found==expected,"Unexpected integrated production blob: "+src)
        src_blobs[src]=found
    check(all(p.exists() for p in [ELF,IMG,MAP]),"Missing expected Release build outputs")
    image=IMG.read_bytes(); elf=ELF.read_bytes()
    check(image[:8]==b"LXRV2IMG","Invalid image signature")
    check(struct.unpack_from("<I",image,8)[0]==len(image)-16,"Image payload length mismatch")
    check(image[12]==255-(sum(image[16:])&255),"Internal image checksum mismatch")
    check(image[13:16]==bytes(3),"Unexpected image header")
    check(b"LXR LAB R19" in image and b"LXR LAB R18" not in image,"R19 startup identifier missing or ambiguous")
    sp,reset=struct.unpack_from("<II",image,16)
    check(0x20000000<=sp<=0x20080000 and sp%8==0,"Invalid vector stack")
    check((reset&1)==1 and 0x08008000<=reset<0x08100000,"Invalid reset entry")
    check(elf[:7]==b"\x7fELF\x01\x01\x01","Unexpected ELF identification")
    check(struct.unpack_from("<H",elf,18)[0]==40,"Unexpected ELF target architecture")
    phoff=struct.unpack_from("<I",elf,28)[0]
    entry,count=struct.unpack_from("<HH",elf,42)
    segments=[]
    for i in range(count):
        kind,offset,virt,phys,size,mem,flags,align=struct.unpack_from("<8I",elf,phoff+i*entry)
        if kind!=1 or not size:continue
        check(0x08008000<=phys and phys+size<=0x08100000,"ELF segment outside application FLASH")
        at=16+phys-0x08008000
        check(image[at:at+size]==elf[offset:offset+size],"ELF/IMG segment byte mismatch")
        segments.append({"flash_start":hex(phys),"end_exclusive":hex(phys+size),
            "load_bytes":size,"runtime_address":hex(virt),"mem_bytes":mem})
    check(segments,"No load segments")
    check(any(int(s["flash_start"],16)<=reset-1<int(s["end_exclusive"],16) for s in segments),
          "Reset vector not inside application load segments")
    flash_payload=max(int(s["end_exclusive"],16) for s in segments)-0x08008000
    check(flash_payload==len(image)-16,"Image length does not match complete flash load span")
    cmd=["arm-none-eabi-size","-A",str(ELF)]
    size_output=subprocess.check_output(cmd,cwd=ROOT,text=True)
    (OUT/"CANDIDATE_SECTIONS.txt").write_text(size_output,encoding="utf-8")
    sec=parse_sections(size_output)
    check(all(k in sec for k in R18_SECTIONS),"Missing critical linker section")
    check(all(sec[k]==v for k,v in INTEGRATED_SECTIONS.items()),
          "R19 unexpectedly changed source-matched integrated linker section sizes")
    check(len(image)==INTEGRATED_IMAGE_BYTES and flash_payload==INTEGRATED_FLASH_PAYLOAD,
          "R19 version-only change unexpectedly altered flash payload size")
    check(digest(BUILD/"compile_commands.json")==INTEGRATED_BUILD_COMPILATION_SHA256,
          "ARM Release compile commands unexpectedly differ from integrated baseline")
    regions={
        "ITCM executable (.itcm_text)":{"capacity":16384,"base":R18_SECTIONS[".itcm_text"],"candidate":sec[".itcm_text"]},
        "DTCM allocated (.dtcm + .dtcmz)":{"capacity":131072,"base":R18_SECTIONS[".dtcm"]+R18_SECTIONS[".dtcmz"],
            "candidate":sec[".dtcm"]+sec[".dtcmz"]},
        "General SRAM sections (.data + .bss + ._user_heap)":{"capacity":393216,
            "base":sum(R18_SECTIONS[s] for s in (".data",".bss","._user_heap")),
            "candidate":sum(sec[s] for s in (".data",".bss","._user_heap"))},
        "Complete image flash payload":{"capacity":1015808,"base":287928,"candidate":flash_payload}
    }
    for x in regions.values():
        x["delta"]=x["candidate"]-x["base"]
        x["nominal_remaining"]=x["capacity"]-x["candidate"]
        check(x["nominal_remaining"]>=0,"Linker region exhausted")
    result={
        "scope":"One R19 version-string-only ARM Release candidate build, integration baseline unchanged; NOT a released firmware.",
        "accepted_R18_base":R18,
        "exact_three_fix_integration_base":INTEGRATION_BASE_SHA,
        "matched_integration_sections":INTEGRATED_SECTIONS,
        "matched_integration_image_bytes":INTEGRATED_IMAGE_BYTES,
        "source_commit":head,
        "integrated_production_blobs":src_blobs,
        "original_R18_build_run":41725066117,
        "original_R18_sections_sha256_reference":"archived R18_SECTIONS.txt in CI 41725066117",
        "vector_stack":hex(sp),"vector_reset":hex(reset),
        "load_segments":segments,"regions":regions,
        "sections":{k:{"R18":v,"integrated_base":INTEGRATED_SECTIONS[k],"candidate":sec[k],
            "delta_from_R18":sec[k]-v,"delta_from_integration":sec[k]-INTEGRATED_SECTIONS[k]}
            for k,v in R18_SECTIONS.items()},
        "files":{p.name:{"size":p.stat().st_size,"sha256":digest(p)} for p in [ELF,IMG,MAP]},
        "unmeasured":["on-target CPU/WCET","IRQ/DMA deadlines","stack high-water",
            "long-run device reliability","on-device audio and physical timing"],
        "note":"Startup identifier is LXR LAB R19. Candidate only; no installation, flash, merge, or release authorization. USB-MIDI physical hardware is NOT TESTED, owner accepted this residual risk for candidate preparation."
    }
    (OUT/"ARM_VALIDATION.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"source":head,"image_bytes":len(image),"flash_payload":flash_payload,
        "regions":regions,"image_sha256":digest(IMG),
        "passed":"header/checksum/vectors/load segments/source identity/resource-region-fit"},indent=2))
if __name__=="__main__":main()
