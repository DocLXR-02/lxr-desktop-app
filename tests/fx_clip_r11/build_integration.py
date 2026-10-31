from pathlib import Path
import subprocess
from common import ROOT,prepare,body
HERE=Path(__file__).parent
PREREQUISITE='d949b3e132fd8afe5f076e6465aae1ffe507c938'
def build(out,source=ROOT,feature=True):
 flags,files=prepare(out,source);m=(source/'Core/Src/Soundengine/DSPAudio/mixer.c').read_text();m=m.replace('/*IN_ITCM_RAM*/ ','')
 names=['mixer_decimateBlock','mixer_checkOutJackAvailable','mixer_addDataToOutput','mixer_clipFaderForBlock']+(['mixer_soloMixForBlock'] if feature else [])+['mixer_calcNextSampleBlock']
 text='static float mixer_clipGains[64];\n'+'\n'.join(body(m,n) for n in names)
 # Observation adapter only around effect call; no changed processing.
 text=text.replace(('SoloMix_process' if feature else 'FxBus_processWithClipGain')+'(fxBusData,','observe_process(fxBusData,')
 (out/'mixer.inc').write_text(text)
 files=[p for p in files if p.name!='SoloMix.c']
 extras=[source/'Core/Src/Soundengine/FxBus.c',source/'Core/Src/Soundengine/DSPAudio/FX/Distortion.c']
 cmd=flags+(['-DFEATURE'] if feature else [])+['-DSOLO_SOURCE="'+str(ROOT/'Core/Src/Soundengine/SoloMix.c')+'"','-I'+str(ROOT/'Core/Inc'),str(HERE/'integration.c')]+list(map(str,files+extras))+['-Wl,--wrap=SoloMix_snareSample','-Wl,--gc-sections','-lm','-o',str(out/'check')]
 r=subprocess.run(cmd,text=True,capture_output=True);(out/'compile.log').write_text(' '.join(cmd)+'\n'+r.stdout+r.stderr);r.check_returncode();return out/'check'
if __name__=='__main__':build(ROOT/'build/r11-integration')
