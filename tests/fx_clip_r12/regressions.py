"""Retain unchanged numeric CLP assertions; historical R11 routing differs intentionally."""
from pathlib import Path
import sys,subprocess,os,json
ROOT=Path(__file__).resolve().parents[2];out=ROOT/'build/r12-regressions';out.mkdir(parents=True,exist_ok=True)
flags=['cc','-std=c11','-D_DEFAULT_SOURCE','-O2','-fno-strict-aliasing','-fsanitize=address,undefined,bounds,float-cast-overflow','-fno-sanitize-recover=all','-I'+str(ROOT/'tests/fx_clip/include'),'-I'+str(ROOT/'Core/Inc')];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');records=[]
for name,fixture in [('CLP-unit','tests/fx_clip/unit.c'),('R7-state','tests/fx_clip_r7/state.c'),('R8-state','tests/fx_clip_r8/state.c')]:
 cmd=flags+['-DDISTORTION_SOURCE="'+str(ROOT/'Core/Src/Soundengine/DSPAudio/FX/Distortion.c')+'"',str(ROOT/fixture),'-lm','-o',str(out/name)];r=subprocess.run(cmd,text=True,capture_output=True);(out/(name+'-compile.log')).write_text(' '.join(cmd)+'\n'+r.stdout+r.stderr);r.check_returncode();r=subprocess.run([str(out/name)],text=True,capture_output=True,env=env);(out/(name+'.log')).write_text(r.stdout+r.stderr);records.append(dict(name=name,command=cmd,returncode=r.returncode,stdout=r.stdout,stderr=r.stderr));r.check_returncode()
(out/'results.json').write_text(json.dumps(records,indent=2)+'\n');print('PASS unchanged production CLP unit/R7 state/R8 state assertions')
