from pathlib import Path
import argparse,subprocess,os
from common import ROOT,prepare
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'build/r11-component');a=p.parse_args();flags,_=prepare(a.output)
cmd=flags+['-DSOLO_SOURCE="'+str(ROOT/'Core/Src/Soundengine/SoloMix.c')+'"',str(Path(__file__).with_suffix('.c')),str(ROOT/'Core/Src/Soundengine/FxBus.c'),str(ROOT/'Core/Src/Soundengine/DSPAudio/FX/Distortion.c'),str(ROOT/'Core/Src/Soundengine/DSPAudio/1PoleLp.c'),'-Wl,--gc-sections','-lm','-o',str(a.output/'check')]
r=subprocess.run(cmd,text=True,capture_output=True);(a.output/'compile.log').write_text(' '.join(cmd)+'\n'+r.stdout+r.stderr);r.check_returncode();subprocess.run([a.output/'check'],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
