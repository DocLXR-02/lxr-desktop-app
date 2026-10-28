"""Real RAM loaders/menu cases and display bodies; no device execution."""
from pathlib import Path
import importlib.util,subprocess,tempfile,os,sys,hashlib,re
sys.dont_write_bytecode=True
R=Path(__file__).resolve().parents[2];H=Path(__file__).parent
sp=importlib.util.spec_from_file_location('f',R/'tests/project_storage/run.py');f=importlib.util.module_from_spec(sp);sp.loader.exec_module(f);h=f.h
s=(R/'Core/Src/menu/menu.c').read_text();old=subprocess.check_output(['git','show','83c5ec164d6c3dab439deb2f32a8edf1138b5b77:Core/Src/menu/menu.c'],cwd=R,text=True)
# Only the two load cases may differ in the command handler, apart from cancelling
# the notice when a new explicit command starts. All save/project/no-op cases stay exact.
a=s.index('//load page');b=s.index('case SAVE_TYPE_SONG:',a)
oa=old.index('//load page');ob=old.index('case SAVE_TYPE_SONG:',oa)
current=h.extract(s,'menu_handleLoadSaveMenu');prior=h.extract(old,'menu_handleLoadSaveMenu')
current=current.replace('    /* A new explicit command supersedes only the transient load notice. */\n    if(btnClicked) menu_kitLoadNotice = 0;\n','')
assert current.replace(s[a:b],old[oa:ob])==prior
for n in ['menu_showSdError','menu_renderSdError','menu_repaintSimplePage','menu_tick']:
 assert h.extract(s,n)==h.extract(old,n)
assert hashlib.sha256((R/'Core/Src/menu/menu.c').read_bytes()).hexdigest()==(H/'menu.sha256').read_text().strip()
funcs=['menu_showKitLoad','menu_renderKitLoad','menu_expireSdError','menu_renderSdError','menu_showSdError','menu_tick','menu_repaintAll','menu_repaint']
ui='static volatile uint8_t menu_kitLoadNotice; static volatile uint32_t menu_kitLoadSince;\n'+''.join(h.extract(s,n) for n in funcs)
base=(R/'tests/live_storage/notifications.c').read_text().replace('int main(void)','int legacy_main(void)')
uihost=base+'''
int main(void){
 legacy_main();
 for(unsigned morph=0;morph<2;morph++){
  menu_kitLoadNotice=0;reset();now=UINT32_MAX-400;
  menu_showKitLoad(morph);menu_tick();
  assert(!memcmp(editDisplayBuffer[0],morph?"Load Morph Kit  ":"Load Kit        ",16));
  assert(!memcmp(editDisplayBuffer[1],"success!        ",16));
  menu_activePage=LOAD_PAGE;now+=749;menu_tick();assert(menu_kitLoadNotice);
  now++;menu_tick();assert(!menu_kitLoadNotice && menu_activePage==LOAD_PAGE);
  for(unsigned i=0;i<10;i++){menu_showKitLoad(morph);now+=750;menu_tick();assert(!menu_kitLoadNotice);}
  menu_showKitLoad(morph);expire_on_copy=1;menu_repaint();
  assert(!menu_kitLoadNotice && display_needs_clear_all);menu_tick();
  for(unsigned row=0;row<2;row++)for(unsigned col=0;col<16;col++)assert(editDisplayBuffer[row][col]=='R');
 }
 puts("PASS both actual labels, 749/750ms and wrap, repeated notices, current-page restoration, actual memcpy/IRQ-expiry interleaving and unchanged legacy errors");
}
'''
loads='void exercise_load(unsigned category){menu_saveOptions.category=category;switch(category){'+h.case(s[a:b],'SAVE_TYPE_KIT','SAVE_TYPE_MORPH')+s[a:b][s[a:b].index('case SAVE_TYPE_MORPH:'):]+ '}}\n'
loadhost='''#define main previous_main
#include "../project_storage/host.c"
#undef main
struct {uint8_t category;} menu_saveOptions;
uint8_t menu_activeVoice,menu_TargetVoiceGapIndex;
static unsigned notices;
static void menu_showKitLoad(uint8_t morph){
 assert(morph<2);unsigned slot=menu_selectedPresetNr[morph?SAVE_TYPE_MORPH:SAVE_TYPE_KIT];
 assert(slot<64);if(morph)assert(morphParameters==mainParameters[slot]);
 else assert(activeParameters[PAR_FILTER_FREQ_1]==42);
 notices++;
}
uint8_t getModTargetGapIndex(uint8_t v){return v;}
void preset_touchTimer(PresetTimer t){(void)t;}
#include "loads.inc"
int main(void){
 for(unsigned kind=0;kind<2;kind++)for(unsigned slot=0;slot<256;slot++){
  reset();notices=0;memset(activeParameters,55,sizeof(activeParameters));
  memset(mainParameters,42,sizeof(mainParameters));morphParameters=mainParameters[0];
  unsigned category=kind?SAVE_TYPE_MORPH:SAVE_TYPE_KIT;menu_selectedPresetNr[category]=slot;
  exercise_load(category);
  assert(notices==(slot<64));
  if(slot<64){if(kind)assert(morphParameters==mainParameters[slot]);else assert(activeParameters[PAR_FILTER_FREQ_1]==42);}
  else {assert(activeParameters[PAR_FILTER_FREQ_1]==55 && morphParameters==mainParameters[0]);}
  assert(!opens && !writes && !reads);
 }
 puts("PASS 512 actual RAM load/menu cases: source copy/destination pointer, invalid indices no action/confirmation, no SD access");
}
'''
with tempfile.TemporaryDirectory() as d:
 d=Path(d);(d/'functions.inc').write_text(ui);(d/'ui.c').write_text(uihost)
 (d/'loads.inc').write_text(loads);(d/'load.c').write_text(loadhost)
 env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
 flags=f.flags+['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast']+['-I'+str(R/p) for p in f.includes]+['-I'+str(d),'-I'+str(H)]
 subprocess.run(flags+[str(d/'ui.c'),'-o',str(d/'ui')],check=True);subprocess.run([str(d/'ui')],env=env,check=True,timeout=20)
 (d/'functions.inc').write_text(f.production_source())
 subprocess.run(flags+[str(d/'load.c'),str(R/'Core/Src/Preset/initKit.c'),'-lm','-o',str(d/'load')],check=True);subprocess.run([str(d/'load')],env=env,check=True,timeout=20)
