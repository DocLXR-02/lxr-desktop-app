"""Focused additional pan-corner full-path check; no production edits/build."""
from pathlib import Path
import argparse,json,os,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--repo',required=True,type=Path);p.add_argument('--baseline',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(a.repo/'tests/fx_clip_r11'));import build_integration as b
text=(a.repo/'tests/fx_clip_r11/integration.c').read_text();needle='setup(v,cfg,chosenMix,vm);';assert text.count(needle)==1
text=text.replace(needle,needle+'''unsigned pan=atoi(getenv("R11_PAN"));assert(pan<=127);
 if(v<3)voiceArray[v].pan=pan;else if(v==3)snareVoice.pan=pan;else if(v==4)cymbalVoice.pan=pan;else hatVoice.pan=pan;
 trigger(v);''')
(a.output/'integration.c').write_text(text);b.HERE=a.output
exes=[b.build(a.output/'baseline',a.baseline,False),b.build(a.output/'candidate')];checks=[]
for v in range(6):
 for pan in [0,64,127]:
  for vm in [0,1]:
   for route in range(6):
    group=[]
    for path,m in [(0,127),(1,127),(1,5),(1,20)]:
     cmd=[str(exes[path]),*map(str,[v,0,m,vm,0,32,1<<v,route,2,.4,1]),'-'];r=subprocess.run(cmd,text=True,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',R11_PAN=str(pan)));assert not r.returncode,(cmd,r.stderr);group.append(json.loads(r.stdout))
    for key in ['source_hash','state_hash','output_hash']:assert group[0][key]==group[1][key],(v,pan,vm,route,key)
    for key in ['source_hash','state_hash']:assert len({x[key] for x in group[1:]})==1,(v,pan,vm,route,key)
    checks.append(dict(voice=v,pan=pan,velocity_mode=vm,destination=route,unity=group[1],mix5=group[2],mix20=group[3]))
(a.output/'results.json').write_text(json.dumps(dict(groups=len(checks),runs=len(checks)*4,results=checks),indent=2)+'\n');print('PASS 864 actual full-voice/mixer executions: pan0/64/127, all six voices/destinations/both velocity modes, unity equality and positive-MIX source/state invariance')
