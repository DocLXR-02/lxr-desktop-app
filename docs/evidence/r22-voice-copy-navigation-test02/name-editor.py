"""Supplemental TEST02 outside-dialog check; extracts full unchanged functions.
Run: python3 name-editor.py /path/to/lxr-desktop-app
Storage/engine actions are forbidden adapters; this checks name editing only.
"""
from pathlib import Path
import importlib.util,re,subprocess,sys,tempfile,os
sys.dont_write_bytecode=True
R=Path(sys.argv[1]).resolve();H=Path(__file__).resolve().parent
sp=importlib.util.spec_from_file_location('fixture',R/'tests/project_storage/run.py')
f=importlib.util.module_from_spec(sp);sp.loader.exec_module(f)
menu=(R/'Core/Src/menu/menu.c').read_text()
names=['copyNameToEditBuffer','copyEditBufferToName','numtostrpu','menu_handleLoadSaveMenu','menu_repaintLoadSavePage']
source='\n'.join(f.h.extract(menu,n) for n in names)
enum=menu[menu.index('enum saveStateEnum'):menu.index('} menu_saveOptions;')+len('} menu_saveOptions;')]
chars=menu[menu.index('#define NUM_CHARS'):menu.index('};',menu.index('const uint8_t characterOrder'))+2]
stubs=['preset_savePatternRAM','preset_savePatternSD','preset_saveDrumsetRam','preset_saveDrumsetSD','preset_saveGlobals','preset_saveProject','preset_loadDrumsetRam','preset_touchTimer','preset_loadPatternRAM','preset_loadProject','preset_createProject','preset_loadGlobals','seq_isRunning','getModTargetGapIndex','menu_resetSaveParameters']
headers='\n'.join(p.read_text(errors='replace') for p in (R/'Core/Inc').rglob('*.h'))
definitions=[]
for name in stubs:
    m=re.search(r'^\s*(void|uint8_t|FRESULT)\s+'+name+r'\s*\([^;{}]*?\)\s*;',headers,re.M)
    assert m,name
    decl=m.group().strip()[:-1]
    definitions.append(decl+'{assert(!"unexpected storage/engine action");'+('' if m[1]=='void' else 'return 0;')+'}')
prefix=r'''
#include <assert.h>
#include <stdio.h>
#include "menu/menu.h"
#include "front.h"
#include "menu/MenuText.h"
#include "Preset/PresetManager.h"
#include "Soundengine/Sequencer/sequencer.h"
uint8_t menu_activePage,menu_activeVoice,visibleCursor;
uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS],menu_loadedPresetNr[NUM_CURRENT];
uint8_t activeParameters[NUM_PARAMS];
char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];
uint8_t seq_songData[NUM_KIT_SLOTS][MAX_SONG_LENGTH],*seq_activeSong,seq_activePattern;
PatternSet seq_patternSet;
char nameEditBuffer[8],editDisplayBuffer[2][17];
static uint8_t editModeActive,menu_kitLoadNotice,menu_TargetVoiceGapIndex;
TIM_HandleTypeDef htim2;
#define VIS_CURS(r,c,v) (((c)<<2)|((r)<<1)|(v))
#define ARROW_SIGN '>'
void menu_showSdError(uint8_t v){(void)v;assert(0);}
void menu_showSaveSuccess(uint8_t v){(void)v;assert(0);}
static void menu_showKitLoad(uint8_t v){(void)v;assert(0);}
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef *p){(void)p;assert(0);return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *p){(void)p;assert(0);return HAL_OK;}
uint8_t preset_isNameEmpty(char *p){return p[0]==0;}
'''
with tempfile.TemporaryDirectory(prefix='lxr-r22-names-') as d:
    d=Path(d);(d/'production.inc').write_text(enum+'\n'+chars+'\n'+'\n'.join(definitions)+'\n'+source)
    (d/'host.c').write_text(prefix+'\n#include "production.inc"\n'+(H/'name-editor-cases.c').read_text())
    cmd=f.flags+['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration']
    cmd+=['-I'+str(R/p) for p in f.includes]+['-I'+str(d),str(d/'host.c'),'-o',str(d/'host')]
    subprocess.run(cmd,cwd=R,check=True)
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
    subprocess.run([str(d/'host')],cwd=R,env=env,check=True)
