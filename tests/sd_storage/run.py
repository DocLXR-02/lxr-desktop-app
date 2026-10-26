#!/usr/bin/env python3
"""Compile the whole production disk driver with mocked BSP/CMSIS, ASan/UBSan."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='lxr-sd-storage-') as directory:
    binary=Path(directory)/'storage'
    env=os.environ.copy(); env['ASAN_OPTIONS']='detect_leaks=0'
    command=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
             '-fsanitize=address,undefined','-fno-sanitize-recover=all',
             '-fno-omit-frame-pointer','-fno-pie','-no-pie','-I'+str(HERE),
             '-I'+str(ROOT),str(HERE/'host.c'),'-o',str(binary)]
    subprocess.run(command,check=True,cwd=ROOT,env=env)
    subprocess.run([str(binary)],check=True,cwd=ROOT,env=env,timeout=15)
