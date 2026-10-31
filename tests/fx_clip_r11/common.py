"""Native production voice fixtures. HAL/control shims are explicit, not an engine boot."""
from pathlib import Path
import os,re,subprocess
ROOT=Path(__file__).resolve().parents[2]
def body(s,name):
 m=re.search(r'^ *(?:IN_ITCM_RAM )?(?:static )?(?:__inline |inline )?(?:const float \*|void |int16_t |uint32_t |uint8_t |int |float )'+name+r'\s*\([^;]*?\)[^\n{]*\s*\{',s,re.M);assert m,name
 n=m.end();depth=1
 while depth:depth+=(s[n]=='{')-(s[n]=='}');n+=1
 return s[m.start():n]
def prepare(out,source=ROOT):
 out=Path(out);out.mkdir(parents=True,exist_ok=True);inc=out/'include';(inc/'Soundengine/DSPAudio').mkdir(parents=True,exist_ok=True)
 (inc/'Soundengine/DSPAudio/BufferTools.h').write_text((source/'Core/Inc/Soundengine/DSPAudio/BufferTools.h').read_text().replace('#include "dither.h"','#include "Soundengine/DSPAudio/dither.h"'))
 (inc/'main.h').write_text('#include "Soundengine/globals.h"\n')
 (inc/'stm32f7xx_hal.h').write_text('''#ifndef AUDIT_HAL_H
#define AUDIT_HAL_H
#include <stdint.h>
#include <assert.h>
#include "Soundengine/globals.h"
typedef struct {int unused;} RNG_HandleTypeDef;
static inline int32_t audit_sub16(int32_t a,int32_t b){uint32_t lo=(uint16_t)host_sat((int16_t)a-(int16_t)b,16);uint32_t hi=(uint16_t)host_sat((int16_t)((uint32_t)a>>16)-(int16_t)((uint32_t)b>>16),16);return (int32_t)(lo|(hi<<16));}
#define __QSUB16(a,b) audit_sub16(a,b)
#endif
''')
 (inc/'audit.h').write_text('/* No temporary source substitution: production code executes. */\n')
 dsp=source/'Core/Src/Soundengine/DSPAudio';s=(dsp/'Oscillator.c').read_text()
 names=['osc_finitePhaseU32','freq2PhaseIncr','freq2PhaseIncr1024','freq2PhaseIncr32767','fast_log2','freqToMidiNote','freqToTableIndex','calcSineBlock','calcSine','calcNoiseBlock','calcNextOscSampleBlock','calcFmSineBlock','calcFmSine','calcFmBlock','calcFm','calcPwmFmBlock','calcNextOscSampleFmBlock','calcWavetableOscBlock','calcWavetableOsc','calcPwmBlock','osc_calcSineFreq','osc_calcNoiseFreq','osc_calcWavetableFreq','osc_calcUserSampleFreq','osc_setFreq','osc_setBaseNote']
 parser=(source/'Core/Src/Soundengine/MIDI/MidiParser.c').read_text();(out/'osc.inc').write_text(body(parser,'midiParser_calcDetune')+'\n'+'\n'.join(body(s,n) for n in names))
 flags=[os.environ.get('CC','cc'),'-std=c11','-D_DEFAULT_SOURCE','-DNUM_DRUM_VOICES=3','-DOUTPUT_DMA_SIZE=64','-DUSE_BIG_SAMPLE_BUFFER=1','-DINTERPOLATE_OSC=1','-DINTERPOLATE_FM_OSC=1','-DSMOOTH_MIXER_VOL_PAN_DRIVE=1','-DENABLE_DRUM_SVF=1','-DENABLE_MIX_OSC=1','-DAMP_EG_SYNC=0','-DUSE_FILTER_DRIVE=0','-Dtrue=1','-Dfalse=0','-O2','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-fsanitize=address,undefined,bounds,float-cast-overflow','-fno-sanitize-recover=all','-Werror=implicit-function-declaration','-I'+str(inc),'-I'+str(out),'-I'+str(ROOT/'tests/fx_clip_r11'),'-I'+str(source/'tests/fx_clip/include'),'-I'+str(source/'Core/Inc')]
 config=(source/'Core/Inc/Soundengine/config.h').read_text()
 for flag in flags:
  if flag.startswith('-D') and '=' in flag:
   k,v=flag[2:].split('=');assert re.search(r'^#define '+k+r'\s+'+v+r'\b',config,re.M),flag
 files=['DrumVoice','Snare','CymbalVoice','HiHat','transientGenerator','transientTables','wavetable','SlopeEg2','Decay','snapEg','1PoleLp','distortion','ResonantFilter']
 extra=[source/'Core/Src/Soundengine/SoloMix.c'] if (source/'Core/Src/Soundengine/SoloMix.c').exists() else []
 return flags,[dsp/(n+'.c') for n in files]+extra
def compile_run(out,fixture,source=ROOT,extra=()):
 flags,files=prepare(out,source);cmd=flags+[str(fixture)]+list(map(str,files))+list(map(str,extra))+['-Wl,--gc-sections','-lm','-o',str(Path(out)/'check')]
 r=subprocess.run(cmd,text=True,capture_output=True);(Path(out)/'compile.log').write_text(' '.join(cmd)+'\n'+r.stdout+r.stderr);r.check_returncode();return Path(out)/'check'
