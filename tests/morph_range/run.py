#!/usr/bin/env python3
"""Sanitized production UI/interpolation and save/load tests; no physical hardware."""
from pathlib import Path
import importlib.util,os,re,subprocess,sys,tempfile
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('fixture',ROOT/'tests/project_storage/run.py')
f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f);h=f.h
menu=(ROOT/'Core/Src/menu/menu.c').read_text();preset=h.preset_source()
start=menu.index('const enum Datatypes PROGMEM parameter_dtypes');end=menu.index('};',start)+2
dtypes=menu[start:end]
name_start=menu.index('const Name valueNames');name_end=menu.index('};',name_start)+2
dtypes=menu[name_start:name_end]+'\n'+dtypes
old=subprocess.check_output(['git','show','5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527:Core/Src/menu/menu.c'],cwd=ROOT).decode().replace('\r\n','\n')
for name in ['menu_encoderChangeParameter','limitPotValue','menu_parseKnobValue','menu_repaintGeneric','menu_parseGlobalParam']:
 assert h.extract(menu,name)==h.extract(old,name)
assert '/*PAR_MORPH*/ \t\t\tDTYPE_0B127' in menu
# No serialization or automation format is redefined. Existing bytes and guards
# are exercised below and in the established full regression suites.
assert 'PAR_MORPH' not in (ROOT/'Core/Src/menu/Cc2Text.c').read_text()
for name in ['preset_writeGlobalData','preset_writePatternData','preset_readPatternData','preset_writeDrumsetData','preset_loadDrumsetRam']:
 before=subprocess.check_output(['git','show','5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527:Core/Src/Preset/presetManager.c'],cwd=ROOT).decode().replace('\r\n','\n')
 assert h.extract(preset,name)==h.extract(before,name)
ui_names=['interpolate','preset_getMorphValue','preset_isTransportParameter','preset_morph_tick','preset_morph']
menu_names=['limitPotValue','menu_encoderChangeParameter','menu_parseKnobValue','menu_repaintGeneric','numtostrpu','numtostrpu_2Bytes','numtostru','numtostrps','upr_three']
# Actual Morph dispatch case; unrelated global command cases are not executed.
case=menu[menu.index('case PAR_MORPH:',menu.index('void menu_parseGlobalParam')):]
case=case[:case.index('case PAR_FETCH:')]
button=(ROOT/'Core/Src/buttonHandler.c').read_text()
source=dtypes+'\n'+''.join(h.extract(preset,n) for n in ui_names)
source+='void menu_parseGlobalParam(uint16_t p,uint8_t value){assert(p==PAR_MORPH);switch(p){'+case+'}}\n'
prototypes=''.join(h.extract(menu,n).split('{',1)[0].rstrip()+';\n' for n in menu_names)
source+=prototypes+''.join(h.extract(menu,n) for n in menu_names)
source+=h.extract((ROOT/'Core/Src/buttonHandler.c').read_text(),'buttonHandler_disarmTimerActionStep')
flags=f.flags+['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration']
flags+=['-I'+str(ROOT/p) for p in f.includes]
env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
with tempfile.TemporaryDirectory(prefix='lxr-morph-range-') as d:
 d=Path(d);(d/'button_constants.inc').write_text('\n'.join(re.findall(r'^#define (?:ARM_AUTOMATION|DISARM_AUTOMATION)\s+[^\r\n]+',button,re.M)));(d/'ui.inc').write_text(source);(d/'functions.inc').write_text(f.production_source())
 for name in ['ui','storage']:
  cmd=flags+['-I'+str(d),str(HERE/(name+'.c'))]
  cmd += [str(ROOT/'Core/Src/menu/Cc2Text.c')] if name=='ui' else [str(ROOT/'Core/Src/Preset/initKit.c')]
  subprocess.run(cmd+['-lm','-o',str(d/name)],cwd=ROOT,env=env,check=True)
  subprocess.run([str(d/name)],cwd=ROOT,env=env,check=True,timeout=90)
print('PASS Morph amount is not serialized or an automation target; serializers/loaders, MIDI/NRPN guards and UI feedback remain unchanged')
