#!/usr/bin/env python3
"""Run the shared production-body fixture plus deterministic silent-autosave cases."""
from pathlib import Path
import subprocess,sys,importlib.util,hashlib,json,re,tempfile,os
sys.dont_write_bytecode=True
root=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('save_helpers',root/'tests/save_errors/run.py')
h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
text=(root/'Core/Src/Preset/presetManager.c').read_bytes().decode('latin1').replace('\r\n','\n')
for path,want in json.loads((Path(__file__).parent/'baseline_sources.json').read_text()).items():
    actual=h.extract(text,path).encode() if path=='preset_touchTimer' else (root/path).read_bytes()
    if path=='Core/Src/menu/menu.c':
        old=b'/*PAR_MORPH*/ \t\t\tDTYPE_0B255';new=b'/*PAR_MORPH*/ \t\t\tDTYPE_0B127'
        assert actual.count(new)==1
        actual=actual.replace(new,old)
    assert hashlib.sha256(actual).hexdigest()==want,path
for name in ['preset_processTimer','preset_autosaveOne','preset_autosaveCanRetry','preset_completeAutosave']:
    body=h.extract(text,name)
    assert not re.search(r'\b(?:menu_\w+|lcd_\w+|HAL_Delay|__disable_irq|__enable_irq|__set_PRIMASK|__set_BASEPRI|malloc|free|f_mount|BSP_SD_\w+|soundengine_\w+)\s*\(',body)
    assert not re.search(r'\b(?:while|for)\s*\(',body)
assert h.extract(text,'preset_completeAutosave').count('__STREXB(')==1
print('PASS static boundary: manual feedback/audio/IRQ/S1 sources (only Morph dtype differs) and edit hook unchanged; autosave helpers have no waits, allocation, notifications, polling or loops',flush=True)
subprocess.run([sys.executable,str(root/'tests/save_errors/run.py'),'--autosave-regressions'],cwd=root,check=True)

paths=[]
for name in ['preset_saveTmpKit','preset_saveTmpPat']:
    paths.append(re.search(r'preset_openShared\([^,]+, "([^"]+)"',h.extract(text,name))[1])
assert paths==['/tmp/tmp.snd','/tmp/tmp.pat']
with tempfile.TemporaryDirectory(prefix='lxr-autosave-paths-') as d:
    d=Path(d);(d/'paths.inc').write_text('static const char *actual_autosave_paths[]={'+','.join(json.dumps(p) for p in paths)+'};\n')
    cmd=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
    cmd+=['-I'+str(p) for p in [d,root,root/'tests/fatfs_integrity',root/'Middlewares/Third_Party/FatFs/src',root/'tests/sd_storage',root/'FATFS/Target']]
    cmd+=[str(root/'tests/silent_autosave/paths.c'),'-o',str(d/'paths')]
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
    subprocess.run(cmd,check=True,env=env);subprocess.run([str(d/'paths')],check=True,env=env,timeout=20)
