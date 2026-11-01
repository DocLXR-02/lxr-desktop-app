"""Compile actual voice/mono DSP/mixer paths; deterministic hardware shims only."""
from pathlib import Path
import sys,subprocess,json
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/fx_clip_r11'))
from common import prepare,body
BASE='64263ed25f2e9c7f5d880b493d943bfba29271a8'
def build(out,source=ROOT,baseline=False,optimized=False):
 out=Path(out);flags,files=prepare(out,source)
 f=(source/'tests/fx_clip_r11/voice_fixture.c').read_text()
 f=f.replace('static float qtarget;','static float qtarget; static float faderTargets[6];').replace('return qtarget;','return faderTargets[voice];')
 f=f.replace('initDrumVoice();Snare_init();Cymbal_init();HiHat_init();','static int initialized; if(!initialized){initDrumVoice();Snare_init();Cymbal_init();HiHat_init();initialized=1;}')
 # Existing inherited CLZ(0) is not fixed here. Production FX init calls Ringmod;
 # preserve the already disclosed ARM-compatible host-zero convention explicitly.
 f=f.replace('#define __CLZ(x) __builtin_clz((unsigned)(x))','static unsigned target_clz(unsigned x){return x?__builtin_clz(x):32u;}\n#define __CLZ(x) target_clz((unsigned)(x))')
 f=f.replace(' unsigned note=cfg', ' uint8_t *pan=v<3?&voiceArray[v].pan:v==3?&snareVoice.pan:v==4?&cymbalVoice.pan:&hatVoice.pan; *pan=(uint8_t[]){0,20,50,77,100,127}[v];\n unsigned note=cfg')
 (out/'voice_fixture.c').write_text(f)
 g=(source/'tests/fx_clip/include/Soundengine/globals.h').read_text().replace('#define __fp16 float','#define __fp16 _Float16');(out/'include/Soundengine/globals.h').write_text(g)
 m=(source/'Core/Src/Soundengine/DSPAudio/mixer.c').read_text().replace('/*IN_ITCM_RAM*/ ','')
 names=['mixer_decimateBlock','mixer_checkOutJackAvailable','mixer_addDataToOutput','mixer_clipFaderForBlock','mixer_soloMixForBlock']+([] if baseline else ['mixer_routeVoice'])+['mixer_calcNextSampleBlock']
 (out/'mixer.inc').write_text(('static float mixer_clipGains[64];\n' if baseline else '')+'\n'.join(body(m,n) for n in names))
 files=[p for p in files if p.name!='SoloMix.c']+[source/'Core/Src/Soundengine/FxBus.c']+[source/'Core/Src/Soundengine/DSPAudio/FX'/n for n in ['Distortion.c','Ringmod.c','Compressor.c','Delay.c','FxRamBuffer.c']]
 flags+=['-DSOLO_SOURCE="'+str(source/'Core/Src/Soundengine/SoloMix.c')+'"']
 if baseline:flags+=['-DBASELINE']
 if optimized:flags=[x for x in flags if x!='-O2']+['-O3','-ffast-math','-freciprocal-math']
 cmd=flags+[str(ROOT/'tests/fx_clip_r12/integration.c')]+list(map(str,files))+['-Wl,--wrap=SoloMix_snareSample','-Wl,--gc-sections','-lm','-o',str(out/'check')]
 r=subprocess.run(cmd,text=True,capture_output=True);(out/'compile.log').write_text(' '.join(cmd)+'\n'+r.stdout+r.stderr);(out/'command.json').write_text(json.dumps(cmd,indent=2));r.check_returncode();return out/'check'
if __name__=='__main__':build(ROOT/'build/r12-integration')
