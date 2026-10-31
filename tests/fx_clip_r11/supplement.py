"""Focused full-mixer boundary checks beyond the fixed-target sweep."""
from pathlib import Path
import argparse,json,os,subprocess
from common import ROOT
from build_integration import build
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'build/r11-supplement');a=p.parse_args();exe=build(a.output);env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');cases=[]
def run(v,cfg,m,vm,profile,mask,route=0):
 cmd=[str(exe),*map(str,[v,cfg,m,vm,profile,256,mask,route,2,.4,1]),'-'];r=subprocess.run(cmd,text=True,capture_output=True,env=env)
 assert not r.returncode,(cmd,r.stderr)
 x=dict(voice=v,config=cfg,mix=m,velocity=vm,profile=profile,mask=mask,route=route,**json.loads(r.stdout));cases.append(x);return x
for v in range(6):
 for vm in [0,1]:
  # Actual velocity32 and low physical fader, filter deliberately OFF in fixture.
  for m in range(128):run(v,8,m,vm,0,1<<v)
  # Actual zero-velocity trigger remains a configured route.
  z=run(v,0,20,vm,8,1<<v)
  if vm:assert z['peak']==0,z
  else:assert z['peak']>0,z
  for route in range(6):
   # A real second generated voice on a direct route survives selected MIX0.
   z=run(v,0,0,vm,9,1<<v,route);assert z['peak']>0,z
   x=run(v,0,20,vm,9,1<<v,route);assert x['peak']>0,x
   # Runtime modulation raises target despite a stored/initial zero.
   x=run(v,0,20,vm,6,1<<v,route);assert x['peak']>0,x
(a.output/'results.json').write_text(json.dumps(dict(cases=len(cases),results=cases),indent=2)+'\n');print('PASS',len(cases),'focused production integration cases: all128 MIX/velocity32/low fader, zero velocity, real direct voice and runtime-target modulation shim')
