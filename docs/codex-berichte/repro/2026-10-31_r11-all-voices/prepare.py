"""Export the exact pin and compile native audit-only fixtures; no firmware build."""
from pathlib import Path
import argparse, hashlib, io, json, os, re, subprocess, tarfile
PIN='8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7'
HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--repo',required=True,type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--run',action='store_true');p.add_argument('--smoke',action='store_true');args=p.parse_args()
assert not(args.run and args.smoke),'Choose --run or --smoke'
R=args.repo.resolve();H=args.output.resolve();assert not H.is_relative_to(R) and not H.is_relative_to(HERE);H.mkdir(parents=True,exist_ok=True);S=H/'source';S.mkdir(exist_ok=True)
status=subprocess.check_output(['git','-C',str(R),'status','--porcelain=v1'])
for name in ['audit.c','transition.c','clamp.c','metrics.py','transitions.py']:(H/name).write_bytes((HERE/name).read_bytes())
with tarfile.open(fileobj=io.BytesIO(subprocess.check_output(['git','-C',str(R),'archive','--format=tar',PIN]))) as z:z.extractall(S,filter='data')
D=S/'Core/Src/Soundengine/DSPAudio';I=H/'include';I.mkdir(exist_ok=True)
(I/'Soundengine/DSPAudio').mkdir(parents=True,exist_ok=True)
(I/'Soundengine/DSPAudio/BufferTools.h').write_text((S/'Core/Inc/Soundengine/DSPAudio/BufferTools.h').read_text().replace('#include "dither.h"','#include "Soundengine/DSPAudio/dither.h"'))
(I/'main.h').write_text('#include "Soundengine/globals.h"\n')
(I/'stm32f7xx_hal.h').write_text('''#ifndef AUDIT_HAL_H
#define AUDIT_HAL_H
#include <stdint.h>
#include <assert.h>
#include "Soundengine/globals.h"
typedef struct {int unused;} RNG_HandleTypeDef;
static inline int32_t audit_sub16(int32_t a,int32_t b){uint32_t lo=(uint16_t)host_sat((int16_t)a-(int16_t)b,16);uint32_t hi=(uint16_t)host_sat((int16_t)((uint32_t)a>>16)-(int16_t)((uint32_t)b>>16),16);return (int32_t)(lo|(hi<<16));}
#define __QSUB16(a,b) audit_sub16(a,b)
#endif
''')
def body(s,name):
 m=re.search(r'^ *(?:IN_ITCM_RAM )?(?:static __inline )?(?:void|int16_t|uint32_t|uint8_t|int|float) '+name+r'\s*\([^;]*?\)[^\n{]*\s*\{',s,re.M);assert m,name
 end=m.end();depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[m.start():end]
s=(D/'Oscillator.c').read_text();names=['freq2PhaseIncr','freq2PhaseIncr1024','freq2PhaseIncr32767','fast_log2','freqToMidiNote','freqToTableIndex','calcSineBlock','calcNoiseBlock','calcNextOscSampleBlock','calcFmSineBlock','calcFmBlock','calcPwmFmBlock','calcNextOscSampleFmBlock','calcWavetableOscBlock','calcPwmBlock','osc_calcSineFreq','osc_calcNoiseFreq','osc_calcWavetableFreq','osc_calcUserSampleFreq','osc_setFreq','osc_setBaseNote']
parser=(S/'Core/Src/Soundengine/MIDI/MidiParser.c').read_text();(H/'osc.inc').write_text(body(parser,'midiParser_calcDetune')+'\n'+'\n'.join(body(s,n) for n in names))
# Temporary audit-only full-source expression and explicitly authorized snare conversion.
(I/'audit.h').write_text('''#include <math.h>
#include <stdint.h>
#include <assert.h>
extern int audit_full;
extern unsigned audit_saturations;
static inline float audit_mix(float m){return audit_full?(m==0?0:1):m;}
static inline int16_t audit_snare(float x){assert(isfinite(x));if(audit_full){if(x>32767){x=32767;audit_saturations++;}else if(x< -32768){x= -32768;audit_saturations++;}}return (int16_t)x;}
''')
for file in ['DrumVoice','Snare','CymbalVoice','HiHat']:
 text=(D/(file+'.c')).read_text()
 if file=='DrumVoice':
  needle='voiceArray[voiceNr].combinedVolume = mixVol *';assert text.count(needle)==1;text=text.replace(needle,'voiceArray[voiceNr].combinedVolume = (audit_full ? audit_mix(voiceArray[voiceNr].vol) : mixVol) *')
 else:
  who={'Snare':'snareVoice','CymbalVoice':'cymbalVoice','HiHat':'hatVoice'}[file]
  pattern=r'('+who+r'\.combinedVolume = )'+who+r'\.vol';text,count=re.subn(pattern,r'\1audit_mix('+who+'.vol)',text);assert count==2,(file,count)
 if file=='Snare':
  pattern=r'buf\[j\] =\s*(snareVoice\.combinedVolume\s*\* tmp \* calcOnePole\( &snareVoice\.FaderVolumeFilter, adc_getFaderVolume\(3\)\));'
  text,count=re.subn(pattern,r'buf[j] = audit_snare(\1);',text);assert count==2
 (H/(file+'.c')).write_text('#include "audit.h"\n'+text)
flags=[os.environ.get('CC','cc'),'-std=c11','-D_DEFAULT_SOURCE','-DNUM_DRUM_VOICES=3','-DOUTPUT_DMA_SIZE=64','-DUSE_BIG_SAMPLE_BUFFER=1','-DINTERPOLATE_OSC=1','-DINTERPOLATE_FM_OSC=1','-DSMOOTH_MIXER_VOL_PAN_DRIVE=1','-DENABLE_DRUM_SVF=1','-DENABLE_MIX_OSC=1','-DAMP_EG_SYNC=0','-DUSE_FILTER_DRIVE=0','-Dtrue=1','-Dfalse=0','-O2','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-fsanitize=address,undefined,bounds,float-cast-overflow','-fno-sanitize-recover=all','-Werror=implicit-function-declaration','-I'+str(I),'-I'+str(H),'-I'+str(S/'tests/fx_clip/include'),'-I'+str(S/'Core/Inc')]
files=['transientGenerator','transientTables','wavetable','SlopeEg2','Decay','snapEg','1PoleLp','distortion','ResonantFilter']
config=(S/'Core/Inc/Soundengine/config.h').read_text()
for flag in flags:
 if flag.startswith('-D') and '=' in flag:
  name,value=flag[2:].split('=');assert re.search(r'^#define '+name+r'\s+'+value+r'\b',config,re.M),flag
mixer=(D/'mixer.c').read_text();(H/'decimator.inc').write_text(body(mixer,'mixer_decimateBlock'))
commands=[]
for version in ['R10','reference']:
 voices=[(D if version=='R10' else H)/(n+'.c') for n in ['DrumVoice','Snare','CymbalVoice','HiHat']]
 for fixture,exe in [('audit',version),('transition','transition-'+version)]:
  cmd=flags+[str(H/(fixture+'.c'))]+list(map(str,voices))+[str(D/(n+'.c')) for n in files]+['-Wl,--gc-sections','-lm','-o',str(H/exe)];commands.append(cmd)
  r=subprocess.run(cmd,text=True,capture_output=True);(H/(exe+'-compile.log')).write_text(r.stdout+r.stderr);r.check_returncode()
cmd=flags+[str(H/'clamp.c'),'-lm','-o',str(H/'clamp')];commands.append(cmd);subprocess.run(cmd,check=True)
env=os.environ.copy();env.setdefault('ASAN_OPTIONS','detect_leaks=0')
r=subprocess.run([str(H/'clamp')],text=True,capture_output=True,env=env);(H/'clamp.log').write_text(r.stdout+r.stderr);r.check_returncode()
metadata=dict(pin=PIN,compiler=subprocess.check_output([flags[0],'--version'],text=True).splitlines()[0],commands=commands,source_hashes={str(f.relative_to(S)):hashlib.sha256(f.read_bytes()).hexdigest() for f in [D/(n+'.c') for n in ['DrumVoice','Snare','CymbalVoice','HiHat']+files+['Oscillator','mixer']]})
(H/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
if args.run or args.smoke:
 runenv=env.copy();runenv.pop('AUDIT_SMOKE',None)
 if args.smoke:runenv['AUDIT_SMOKE']='1'
 subprocess.run(['python3',str(H/'metrics.py')],env=runenv,check=True)
 if args.run:subprocess.run(['python3',str(H/'transitions.py')],env=env,check=True)
assert subprocess.check_output(['git','-C',str(R),'status','--porcelain=v1'])==status,'Source checkout changed'
print('Native audit reproduction finished. A reproduced failure is not a passing R11 candidate.')
