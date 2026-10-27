#!/usr/bin/env python3
"""Actual project/FatFs operations on RAM media, faults and sector power cuts."""
from pathlib import Path
import importlib.util, os, subprocess, sys, tempfile
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('project_fixture',ROOT/'tests/project_storage/run.py')
fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
with tempfile.TemporaryDirectory(prefix='lxr-project-transaction-') as d:
    d=Path(d);(d/'functions.inc').write_text(fixture.production_source())
    flags=fixture.flags+['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast',
                         '-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration']
    flags+=['-I'+str(ROOT/p) for p in fixture.includes]+['-I'+str(d)]
    subprocess.run(flags+[str(HERE/'whole.c'),str(ROOT/'Core/Src/Preset/initKit.c'),'-o',str(d/'whole')],cwd=ROOT,env=env,check=True)
    for group in (sys.argv[1:] or ['normal','faults','boundaries','sector','edge']):
        subprocess.run([str(d/'whole'),group],cwd=ROOT,env=env,check=True,timeout=120)
