from pathlib import Path
import concurrent.futures,csv,hashlib,json,os,re,subprocess,time
H=Path(__file__).resolve().parent;O=H/'results';O.mkdir(exist_ok=True);env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
def run(job):
 v,cfg,m,mode,path=job;blocks=1 if cfg in [1,7] else 128
 r=subprocess.run([str(H/('R10' if path=='R10' else 'reference')),str(v),str(cfg),str(m),str(mode),str(int(path!='R10')),str(blocks)],text=True,capture_output=True,env=env)
 row=dict(voice=v+1,config=cfg,mix=m,velocity_mode=mode,path=path,frames=64*blocks,returncode=r.returncode)
 if not r.returncode:row.update(json.loads(r.stdout));row['failure']=''
 else:
  error=r.stderr.strip().splitlines();row['failure']=error[0] if error else 'missing diagnostic';row['diagnostic']=r.stderr
 return row
jobs=[(v,cfg,m,mode,path) for v in range(6) for cfg in range(13) for m in ([0,20,127] if os.environ.get('AUDIT_SMOKE') else range(128)) for mode in [0,1] for path in ['R10','full+snare-clamp']]
rows=[];start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
 for idx,row in enumerate(pool.map(run,jobs),1):
  rows.append(row)
  if idx%4096==0:print(f'{idx}/{len(jobs)} audit runs complete',flush=True)
(O/'cases.json').write_text(json.dumps(rows,indent=2)+'\n')
fields=['voice','config','mix','velocity_mode','path','frames','returncode','peak','rms','last_nonzero','hash','snare_saturations','failure']
with (O/'cases.csv').open('w') as f:
 w=csv.DictWriter(f,fieldnames=fields,extrasaction='ignore');w.writeheader();w.writerows(rows)
summary=[]
for v in range(6):
 for cfg in range(13):
  for mode in [0,1]:
   for path in ['R10','full+snare-clamp']:
    a=[r for r in rows if r['voice']==v+1 and r['config']==cfg and r['velocity_mode']==mode and r['path']==path]
    bad=[r['mix'] for r in a if r['returncode']];good=[r for r in a if not r['returncode']]
    classes=sorted(set(re.sub(r'^.*(?:source/|osc.inc:)', '',r['failure']) for r in a if r['returncode']))
    summary.append(dict(voice=v+1,config=cfg,velocity_mode=mode,path=path,pass_count=len(good),fail_mix_values=bad,failures=classes,positive_full_hashes=sorted(set(r['hash'] for r in good if r['mix']>0)),peak=max([r['peak'] for r in good],default=None),snare_saturations=max([r['snare_saturations'] for r in good],default=None)))
(O/'summary.json').write_text(json.dumps(dict(pin='8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7',runs=len(rows),passed=sum(not r['returncode'] for r in rows),failed=sum(bool(r['returncode']) for r in rows),groups=summary),indent=2)+'\n')
print('Audit finished:',len(rows),'runs;',sum(not r['returncode'] for r in rows),'pass;',sum(bool(r['returncode']) for r in rows),'fail',flush=True)
