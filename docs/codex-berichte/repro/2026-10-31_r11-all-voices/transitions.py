from pathlib import Path
import concurrent.futures,json,os,subprocess
H=Path(__file__).resolve().parent;env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');values=[0,1,5,10,20,30,60,78,79,80,90,115,116,117,127]
jobs=[(v,m,vm,profile,path) for v in range(6) for m in values for vm in [0,1] for profile in [0,1] for path in ['R10','reference']]
def run(j):
 v,m,vm,profile,path=j
 r=subprocess.run([str(H/('transition-'+path)),str(v),str(m),str(vm),str(int(path=='reference')),str(profile),'-'],text=True,capture_output=True,env=env)
 if r.returncode:raise RuntimeError((j,r.stderr))
 return dict(voice=v+1,mix=m,velocity_mode=vm,profile=profile,path=path,**json.loads(r.stdout))
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:r=list(pool.map(run,jobs))
for row in r:
 if row['path']=='reference':assert row['zero_target_after_proposed_gate_nonzero']==0
for v in range(6):
 for vm in [0,1]:
  for profile in [0,1]:
   subset=[row for row in r if row['voice']==v+1 and row['velocity_mode']==vm and row['profile']==profile and row['path']=='reference' and row['mix']>0]
   assert len(set(row['source_hash'] for row in subset))==1,'positive MIX changed full-source samples'
(H/'results/transitions.json').write_text(json.dumps(r,indent=2)+'\n');print('Completed',len(r),'full-generation/decimator transition runs; positive-MIX source invariance; explicit post-decimator zero endpoint')
