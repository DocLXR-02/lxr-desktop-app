#!/usr/bin/env python3
"""Native ASan/UBSan tests of the production allocator and diagnostic implementation."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='lxr-safety-') as directory:
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
    flags=[os.environ.get('CC','cc'),'-std=gnu11','-O1','-g','-fsanitize=address,undefined',
           '-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie',
           '-Wall','-Wextra','-Werror','-I'+str(HERE),'-I'+str(ROOT/'Core/Inc'),'-I'+str(ROOT)]
    for name,source,more in [('heap','test_heap.c',[]),
                            ('disabled','test_diagnostics.c',[]),
                            ('enabled','test_diagnostics.c',['-DLXR_RUNTIME_DIAGNOSTICS=1',str(ROOT/'Core/Src/safety/runtimeDiagnostics.c')])]:
        binary=Path(directory)/name
        subprocess.run(flags+[str(HERE/source)]+more+['-o',str(binary)],check=True,cwd=ROOT,env=env)
        subprocess.run([str(binary)],check=True,cwd=ROOT,env=env)
