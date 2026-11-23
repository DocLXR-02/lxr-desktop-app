#!/usr/bin/env python3
"""Compare matched, freshly built R22 and CC0 candidate ARM ELF/IMG resources.

Only static section sizes/headroom are measured: not CPU cycles or runtime WCET.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct

CAPACITY = {"ITCM": 16384, "DTCM": 131072, "SRAM": 393216,
            "FLASH_payload": 1015808}

def examine(folder):
    elf = (folder / "LXR_V2_STM32F765.elf").read_bytes()
    img = (folder / "LXR_V2_V1.7.img").read_bytes()
    if elf[:7] != b"\x7fELF\x01\x01\x01" or struct.unpack_from("<H",elf,18)[0] != 40:
        raise AssertionError("Not an ARM 32-bit little-endian ELF")
    if img[:8] != b"LXRV2IMG":
        raise AssertionError("Invalid LXR update image magic")
    if len(img)-16 != struct.unpack_from("<I",img,8)[0]:
        raise AssertionError("Invalid image payload length")
    if img[12] != (255-sum(img[16:])%256):
        raise AssertionError("Invalid LXR updater checksum")
    if b"LXR LAB R22" not in img:
        raise AssertionError("Expected accepted R22 boot string absent")
    headers_at = struct.unpack_from("<I",elf,32)[0]
    sh_size,sh_n,sh_strings = struct.unpack_from("<HHH",elf,46)
    headers = [struct.unpack_from("<10I",elf,headers_at+i*sh_size)
               for i in range(sh_n)]
    strings = elf[headers[sh_strings][4]:headers[sh_strings][4]+headers[sh_strings][5]]
    sections = {}
    for item in headers:
        name_at = item[0]
        if not name_at:
            continue
        name = strings[name_at:strings.index(0,name_at)].decode()
        sections[name] = {"address":item[3], "bytes":item[5]}
    for k in (".text",".rodata",".itcm_text",".dtcm",".dtcmz",".data",".bss","._user_heap"):
        if k not in sections:
            raise AssertionError("Expected ELF section absent: "+k)
    def span(names, origin):
        return max(sections[n]["address"]+sections[n]["bytes"] for n in names)-origin
    metrics = {
        "FLASH_payload":len(img)-16,
        "FLASH_IMG_total":len(img),
        "ITCM":span([".itcm_text"],0),
        "DTCM":span([".dtcm",".dtcmz"],0x20000000),
        "SRAM":span([".data",".bss","._user_heap"],0x20020000),
        ".text":sections[".text"]["bytes"],
        ".rodata":sections[".rodata"]["bytes"],
    }
    for key,cap in CAPACITY.items():
        if not 0 < metrics[key] < cap:
            raise AssertionError(f"{key} exceeds declared capacity: {metrics[key]}/{cap}")
    files = {}
    for name in ("LXR_V2_STM32F765.elf","LXR_V2_STM32F765.map","LXR_V2_V1.7.img"):
        raw = (folder/name).read_bytes()
        files[name] = {"bytes":len(raw),
                       "sha256":hashlib.sha256(raw).hexdigest()}
    return {"metrics":metrics,"sections":sections,"files":files,
            "headroom":{key:cap-metrics[key] for key,cap in CAPACITY.items()}}

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--baseline",type=Path,required=True)
    p.add_argument("--candidate",type=Path,required=True)
    p.add_argument("--out",type=Path,required=True)
    a=p.parse_args()
    a.out.mkdir(parents=True,exist_ok=True)
    base=examine(a.baseline)
    candidate=examine(a.candidate)
    report={
        "comparison":"SAME_ARM_GNU_14_3_REL1_RELEASE_PINNED_R22_VS_CC0",
        "baseline_sha":"0e37942250f79306627702061d851731bdfdbbc8",
        "candidate_source_patch_sha":"280d57972a03bf65e3e0e36145abd4fa082a3d0b",
        "firmware_tests":"HOST_AND_STATIC_BUILD_ONLY",
        "CPU_WCET":"UNMEASURED",
        "stack_high_water":"UNMEASURED",
        "real_hardware":"NOT_RUN",
        "baseline":base,
        "candidate":candidate,
        "delta_candidate_minus_baseline":{
            k:candidate["metrics"][k]-base["metrics"][k]
            for k in sorted(base["metrics"])
        },
    }
    (a.out/"resource-comparison.json").write_text(json.dumps(report,indent=2,sort_keys=True)+"\n")
    print(json.dumps({"status":"PASS_ELF_IMG_STATIC_LAYOUT_AND_HEADROOM",
       "baseline_metrics":base["metrics"],"candidate_metrics":candidate["metrics"],
       "delta":report["delta_candidate_minus_baseline"]},indent=2,sort_keys=True))
if __name__=="__main__":
    main()
