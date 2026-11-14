#!/usr/bin/env python3
"""R21 production selector/transfer and real-FatFs project round trips, ASan/UBSan."""
from pathlib import Path
import importlib.util, json, os, subprocess, sys, tempfile
sys.dont_write_bytecode = True
R = Path(__file__).resolve().parents[2]
H = Path(__file__).resolve().parent
sp = importlib.util.spec_from_file_location('fixture', R/'tests/project_storage/run.py')
f = importlib.util.module_from_spec(sp); sp.loader.exec_module(f)
menu = (R/'Core/Src/menu/menu.c').read_text()
seq = (R/'Core/Src/Soundengine/Sequencer/sequencer.c').read_text()
buttons = (R/'Core/Src/buttonHandler.c').read_text()
with tempfile.TemporaryDirectory(prefix='lxr-r21-') as tmp:
    tmp = Path(tmp)
    (tmp/'functions.inc').write_text(f.production_source())
    whole = (R/'tests/project_transaction/whole.c').read_text()
    whole = whole[:whole.index('int main(int argc,char **argv)')]
    whole = whole.replace('uint8_t getNumModTargets(void){return 16;}', '')
    whole = whole.replace('void menu_repaintAll(void){}', '')
    whole = whole.replace('void preset_sendDrumsetParameters(void){}', '')
    whole = whole.replace('void preset_sendDrumsetParametersImmediate(void){}', '')
    whole = whole.replace('void soundengine_process(void){}', '')
    preset = (R/'Core/Src/Preset/presetManager.c').read_text().replace('IN_ITCM_RAM ', '')
    names = ['interpolate', 'preset_isTransportParameter', 'preset_morph', 'preset_morph_tick',
             'preset_sendDrumsetParameters', 'preset_sendDrumsetParametersImmediate']
    # interpolate is already present in the project fixture.
    (tmp/'load.inc').write_text('\n'.join(f.h.extract(preset,n) for n in names if n!='interpolate'))
    cortex = (R/'Core/Src/Soundengine/MIDI/CortexParser.c').read_text()
    a=cortex.index('\tcase FRONT_CC_LFO_TARGET:'); b=cortex.index('    case FRONT_SET_P1_DEST:',a)
    (tmp/'destination.inc').write_text('switch(cortex_midiMsg.status){'+cortex[a:b]+'default: assert(0);}')
    lfo = (R/'Core/Src/Soundengine/DSPAudio/lfo.c').read_text()
    (tmp/'retrigger.inc').write_text(f.h.extract(lfo,'lfo_retrigger'))
    whole = whole.replace('void frontPanel_sendData(uint8_t a,uint8_t b,uint8_t c){(void)a;(void)b;(void)c;}',
                         'static unsigned sends;\nvoid frontPanel_sendData(uint8_t a,uint8_t b,uint8_t c){sends++;r21_frontSend(a,b,c);}')
    (tmp/'whole.inc').write_text(whole)
    a=seq.index('if(activeParameters[PAR_PATTERN_MODE] == SKIP_NEXT_KIT_AUTOLOAD)'); b=seq.index('//use shuffle value from pattern',a)
    (tmp/'automatic.inc').write_text(seq[a:b])
    (tmp/'pending.inc').write_text(f.h.extract(seq, 'seq_hasPendingKitChange'))
    (tmp/'encoder.inc').write_text(f.h.extract(menu, 'menu_parseEncoder'))
    # The entire production public button dispatchers run, with unrelated actions
    # adapted in panel.c. This catches hook placement and COPY release fallthrough.
    (tmp/'base.inc').write_text(f.h.extract(buttons, 'buttonHandler_copyEditableBase'))
    (tmp/'buttons.inc').write_text(f.h.extract(buttons, 'buttonHandler_buttonPressed') +
                                   f.h.extract(buttons, 'buttonHandler_buttonReleased'))
    cmd = f.flags + ['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast',
                    '-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration']
    cmd += ['-I'+str(R/p) for p in f.includes] + ['-I'+str(tmp), '-I'+str(H)]
    cmd += [str(H/'host.c'), str(R/'Core/Src/menu/Cc2Text.c'),
            str(R/'Core/Src/Preset/initKit.c'), str(R/'Core/Src/Parameters.c'), '-o', str(tmp/'host')]
    env = os.environ.copy(); env['ASAN_OPTIONS'] = 'detect_leaks=0'
    subprocess.run(cmd,cwd=R,env=env,check=True)
    subprocess.run([str(tmp/'host'),*sys.argv[1:]],cwd=R,env=env,check=True,timeout=90)
