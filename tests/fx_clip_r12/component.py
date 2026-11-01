from pathlib import Path
import sys,subprocess,os,json
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/fx_clip_r11'));from common import prepare
out=ROOT/'build/r12-component';flags,_=prepare(out)
cmd=flags+['-DSOLO_SOURCE="'+str(ROOT/'Core/Src/Soundengine/SoloMix.c')+'"',str(Path(__file__).with_suffix('.c')),str(ROOT/'Core/Src/Soundengine/FxBus.c'),str(ROOT/'Core/Src/Soundengine/DSPAudio/FX/Distortion.c'),str(ROOT/'Core/Src/Soundengine/DSPAudio/1PoleLp.c'),'-Wl,--gc-sections','-lm','-o',str(out/'check')]
r=subprocess.run(cmd,text=True,capture_output=True);(out/'compile.log').write_text(' '.join(cmd)+'\n'+r.stdout+r.stderr);r.check_returncode();r=subprocess.run([str(out/'check')],text=True,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));(out/'run.log').write_text(r.stdout+r.stderr);r.check_returncode();print(r.stdout)
