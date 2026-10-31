"""Selected FILTER-OFF full-generation and actual mixer integration comparisons."""
from pathlib import Path
import argparse,concurrent.futures,gzip,io,json,os,subprocess,tarfile
from common import ROOT
from build_integration import build,PREREQUISITE
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'build/r11-voices');p.add_argument('--smoke',action='store_true');a=p.parse_args();O=a.output.resolve();O.mkdir(parents=True,exist_ok=True)
S=O/'prerequisite';S.mkdir(exist_ok=True)
with tarfile.open(fileobj=io.BytesIO(subprocess.check_output(['git','-C',str(ROOT),'archive',PREREQUISITE]))) as z:z.extractall(S,filter='data')
exes=[build(O/'baseline',S,False),build(O/'candidate')]
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
def run(job):
 path,v,cfg,m,vm,profile,blocks,mask,route,kind,dw,vol=job
 cmd=[str(exes[path]),*map(str,(v,cfg,m,vm,profile,blocks,mask,route,kind,dw,vol)),'-'];r=subprocess.run(cmd,text=True,capture_output=True,env=env)
 row=dict(path=['prerequisite','R11'][path],voice=v,config=cfg,mix=m,velocity_mode=vm,profile=profile,blocks=blocks,mask=mask,route=route,kind=kind,dw=dw,vol=vol,returncode=r.returncode)
 if r.returncode:row['diagnostic']=r.stderr.strip()
 else:row.update(json.loads(r.stdout))
 return row
jobs=[(path,v,cfg,m,vm,0,1 if cfg==1 else 128,1<<v,0,2,1,1) for path in [0,1] for cfg in [0,1,2,3,6,9,10,12] for v in range(6) for vm in [0,1] for m in ([0,1,5,10,20,30,60,78,79,80,90,115,116,117,127] if a.smoke else range(128))]
rows=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
 for i,row in enumerate(pool.map(run,jobs),1):
  rows.append(row)
  if i%2048==0:print(f'{i}/{len(jobs)} generation/integration cases',flush=True)
expected=[]
for r in rows:
 if r['returncode']:
  assert r['path']=='prerequisite' and r['config']==1 and r['voice']==3 and r['mix']>=79,r
  assert 'outside the range' in r['diagnostic'],r
  expected.append(r)
# Positive selected MIX changes post level, never the excitation or DSP history.
for cfg in [0,1,2,3,6,9,10,12]:
 for v in range(6):
  for vm in [0,1]:
   q=[r for r in rows if r['path']=='R11' and r['voice']==v and r['velocity_mode']==vm and r['config']==cfg and r['mix']>0]
   assert len({r['source_hash'] for r in q})==1,(cfg,v,vm,'source')
   assert len({r['state_hash'] for r in q})==1,(cfg,v,vm,'CLP state')
   zero=next(r for r in rows if r['path']=='R11' and r['voice']==v and r['velocity_mode']==vm and r['config']==cfg and r['mix']==0)
   assert zero['peak']==0 and zero['zero_input_checks']==zero['blocks']*128,zero
   b=next(r for r in rows if r['path']=='prerequisite' and r['voice']==v and r['velocity_mode']==vm and r['config']==cfg and r['mix']==127)
   c=next(r for r in q if r['mix']==127)
   if not b['returncode']:assert all(b[k]==c[k] for k in ['source_hash','state_hash','output_hash']),(b,c)
# Long transitions with actual poles/decimator, notes, target zero and H capture.
trans=[]
for v in range(6):
 for vm in [0,1]:
  for profile in [1,2,3,4,5,6]:
   for m in ([5,127] if a.smoke else [1,5,10,20,127]):
    r=run((1,v,0,m,vm,profile,2048,1<<v,v,2,.4,1));assert not r['returncode'],r
    if profile==1:assert r['zero_input_checks']>0 and r['last_nonzero']<2048*64-4414,r
    trans.append(r)
# Configured multi/empty routes, other distortion algorithms: prerequisite equality.
fallback=[]
for v in range(6):
 for kind in [0,1,2]:
  for mask in [0,3,63] if kind==2 else [1<<v,63]:
   for route in range(6):
    b=run((0,v,0,20,1,0,32,mask,route,kind,.4,1));c=run((1,v,0,20,1,0,32,mask,route,kind,.4,1));assert b['returncode']==c['returncode']==0,(b,c)
    assert all(b[k]==c[k] for k in ['source_hash','state_hash','output_hash']),(b,c);fallback.append(c)
# Explicit unchanged, unresolved failures: isolated, not passing cases.
known=[]
known_jobs=[(path,v,cfg,m,1,profile,4,1<<v,0,2,1,1) for path,v,cfg,m,profile in [(0,0,7,20,7),(1,0,7,20,7),(0,3,11,20,7),(1,3,11,20,7),(0,3,1,79,0)]]
known_jobs += [(1,3,1,79,vm,0,1,mask,0,kind,1,1) for vm in [0,1] for mask,kind in [(0,2),(24,2),(8,0)]]
for job in known_jobs:
 r=run(job);assert r['returncode'],r
 assert 'outside the range' in r['diagnostic'],r;known.append(r)
with gzip.open(O/'all-mix.json.gz','wt') as f:json.dump(rows,f)
result=dict(prerequisite=PREREQUISITE,main_cases=len(rows),passed=len(rows)-len(expected),expected_legacy_snare_failures=len(expected),positive_mix_invariant_groups=96,transition_cases=len(trans),fallback_pairs=len(fallback),known_unresolved=known,transitions=trans,fallbacks=fallback,coverage='Actual six production voices and verbatim mixer block; deterministic HAL/ADC/RNG and control-scheduling shims. FILTER-OFF selected configurations, not all parameters or firmware boot.')
(O/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['known_unresolved','transitions','fallbacks']},indent=2))
