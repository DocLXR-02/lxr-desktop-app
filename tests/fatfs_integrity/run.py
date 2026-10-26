#!/usr/bin/env python3
"""Whole production FatFs/disk glue/S1 driver tests with RAM media and BSP mocks."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='lxr-fatfs-integrity-') as directory:
    directory=Path(directory)
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
    for name,readonly in [('configured',False),('readonly',True)]:
        config=directory/name;config.mkdir()
        text=(ROOT/'FATFS/Target/ffconf.h').read_text()
        if readonly:
            for option in ['_FS_READONLY','_FS_LOCK','_USE_MKFS']:
                text=re.sub(r'(#define\s+'+option+r'\s+)\d+',lambda m:m[1]+('1' if option=='_FS_READONLY' else '0'),text)
        (config/'ffconf.h').write_text(text)
        binary=config/'host'
        command=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra',
                 '-Wno-unused-function','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                 '-fno-omit-frame-pointer','-fno-pie','-no-pie','-ffunction-sections','-fdata-sections',
                 '-Wl,--gc-sections','-I'+str(HERE),'-I'+str(config),'-I'+str(ROOT),
                 '-I'+str(ROOT/'Middlewares/Third_Party/FatFs/src'),
                 '-I'+str(ROOT/'tests/sd_storage'),'-I'+str(ROOT/'FATFS/Target'),str(HERE/'host.c'),'-o',str(binary)]
        subprocess.run(command,check=True,cwd=ROOT,env=env)
        print('Configuration:',name,flush=True)
        subprocess.run([str(binary)],check=True,cwd=ROOT,env=env,timeout=20)
