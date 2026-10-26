#!/usr/bin/env python3
"""Read-only desktop update-image and ELF memory validation; never flash/upload."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

REGIONS = {'ITCM_RAM': (0, 16*1024), 'DTCM': (0x20000000, 128*1024),
           'RAM': (0x20020000, 384*1024), 'FLASH': (0x08008000, 992*1024)}

def check(directory, objdump):
    image = (directory / 'LXR_V2_V1.7.img').read_bytes()
    binary = (directory / 'LXR_V2_STM32F765.bin').read_bytes()
    elf = directory / 'LXR_V2_STM32F765.elf'
    magic, length, checksum, padding = struct.unpack('<8sIB3s', image[:16])
    payload = image[16:]
    assert magic == b'LXRV2IMG' and padding == b'\0\0\0'
    assert length == len(payload) == len(binary) and payload == binary
    assert len(image) == length+16 and 0 < length < 0x080fffff-0x08008000
    assert checksum == 0xff-(sum(payload) & 0xff)
    msp, reset = struct.unpack('<II', payload[:8])
    assert 0x20020000 < msp <= 0x20080000 and msp % 8 == 0
    assert reset & 1 and 0x08008000 <= (reset & ~1) < 0x08008000+length
    listing = subprocess.run([objdump, '-h', str(elf)], check=True,
                             capture_output=True, text=True).stdout
    high = {name: 0 for name in REGIONS}
    sections = []
    for match in re.finditer(r'^\s*\d+\s+(\S+)\s+([\da-f]+)\s+([\da-f]+)\s+([\da-f]+)[^\n]*\n([^\n]*)', listing, re.M):
        name, size, vma, lma, flags = match.groups()
        size, vma, lma = (int(x,16) for x in (size,vma,lma))
        if not size or 'ALLOC' not in flags:
            continue
        regions = [(label,base,capacity) for label,(base,capacity) in REGIONS.items()
                   if base <= vma and vma+size <= base+capacity]
        assert len(regions)==1, f'{name} outside memory regions'
        label,base,capacity = regions[0]
        high[label] = max(high[label],vma+size-base)
        if 'LOAD' in flags:
            assert 0x08008000 <= lma and lma+size <= 0x08008000+length
        sections.append(name)
    assert {'.isr_vector','.text','.data','.bss','.itcm_text','.dtcm','.dtcmz'} <= set(sections)
    assert high['DTCM']+0x400 <= REGIONS['DTCM'][1], 'minimum DTCM stack reserve'
    assert high['RAM']+0x400 <= REGIONS['RAM'][1], 'minimum main stack reserve'
    return dict(configuration=directory.name, magic=magic.decode(), image_bytes=len(image),
                payload_bytes=length, checksum=f'0x{checksum:02X}', initial_msp=f'0x{msp:08X}',
                reset_vector=f'0x{reset:08X}', memory_high_water_bytes=high,
                image_sha256=hashlib.sha256(image).hexdigest(),
                elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest(), result='PASS')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--objdump', default='arm-none-eabi-objdump')
    parser.add_argument('directories', nargs='+', type=Path)
    args = parser.parse_args()
    for directory in args.directories:
        print(json.dumps(check(directory,args.objdump),sort_keys=True))

if __name__ == '__main__':
    main()
