"""Bounded R12 production integration matrix; retain every command and failure."""
from pathlib import Path
import argparse,concurrent.futures,json,os,subprocess,tarfile,io
from build import ROOT,BASE,build
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'build/r12-checks');a=p.parse_args();O=a.output.resolve();O.mkdir(parents=True,exist_ok=True)
S=O/'r11';S.mkdir(exist_ok=True)
with tarfile.open(fileobj=io.BytesIO(subprocess.check_output(['git','-C',str(ROOT),'archive',BASE]))) as t:t.extractall(S,filter='data')
exes=[build(O/'candidate'),build(O/'baseline',S,True),build(O/'optimized',optimized=True)]
rows=[]
def run(job):
 variant,v,cfg,m,vm,profile,blocks,mask,route,kind,dw,vol,fx=job
 cmd=[str(exes[variant]),*map(str,job[1:]),'-'];r=subprocess.run(cmd,text=True,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
 x=dict(variant=['R12-O2','R11-O2','R12-O3-fast'][variant],args=list(job[1:]),command=cmd,returncode=r.returncode)
 if r.returncode:x['stderr']=r.stderr
 else:x.update(json.loads(r.stdout))
 return x
# Selected, finite source/filter-off configurations, both velocity branches.
jobs=[(0,v,cfg,m,vm,0,64,1<<v,0,2,dw,1,1) for v in range(6) for cfg in [0,1] for vm in [0,1] for m in [0,1,5,10,20,50,127] for dw in [0,.4,1]]
# Selected FM/PWM source configurations reuse unchanged production synthesis.
jobs += [(0,v,cfg,20,vm,0,64,mask,0,2,.4,1,1) for v in range(6) for cfg in [2,10] for vm in [0,1] for mask in [1<<v,63]]
# All configured masks, every destination; silent routes still count.
jobs += [(0,5,0,20,1,0,16,mask,route,2,.4,1,1) for mask in range(64) for route in range(6)]
# Zero/return, fader, route/FX/lifecycle, runtime target modulation, direct voice.
jobs += [(0,v,0,20,vm,pr,768,1<<v,v,2,.4,vol,1) for v in range(6) for vm in [0,1] for pr in [1,2,3,4,5,6,8,9,11,13] for vol in [0,1]]
# All six paths sounding with unequal MIX/faders; all output routes, DW endpoints.
jobs += [(mode,3,cfg,m,vm,10,768,63,route,2,dw,1,1) for mode in [0,2] for cfg in [0,1] for m in [10,50] for vm in [0,1] for route in range(6) for dw in [0,.4,1]]
# Single/multi source/state equality for the selected voice, with silent peers.
jobs += [(0,v,0,20,vm,0,64,63,0,2,1,1,1) for v in range(6) for vm in [0,1]]
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
 for row in pool.map(run,jobs):rows.append(row)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
failed=[r for r in rows if r['returncode']]
assert not failed,failed[:3]
# Positive MIX never changes full-source excitation or pre-post DSP histories.
for v in range(6):
 for cfg in [0,1]:
  for vm in [0,1]:
   for dw in [0,.4,1]:
    group=[r for r in rows if r['variant']=='R12-O2' and r['args'][0:2]==[v,cfg] and r['args'][3]==vm and r['args'][4]==0 and r['args'][5]==64 and r['args'][6]==1<<v and r['args'][9]==dw and r['args'][2]>0]
    assert len({r['sources'][v] for r in group})==1,(v,cfg,vm,dw,'source')
    assert len({r['states'][v] for r in group})==1,(v,cfg,vm,dw,'state')
    assert len({r['returns'][v] for r in group})==1,(v,cfg,vm,dw,'return')
for r in rows:
 if r['args'][2]==0 and r['args'][6]==1<<r['args'][0]:assert r['peak']==0,r
# R11 fallback/other algorithms: exact output hashes, actual effects linked.
pairs=[]
for fx,kind in [(0,2),(1,0),(1,1),(2,2),(3,2),(4,2)]:
 for v in range(6):
  for route in ([0,1] if fx==4 else range(6)):
   job=(0,v,0,20,1,0,32,1<<v,route,kind,.4,1,fx);b=run((1,*job[1:]));c=run(job);pairs+=[b,c]
   assert b['returncode']==c['returncode']==0,(b,c)
   assert b['output_hash']==c['output_hash'],(b,c)
(O/'fallbacks.json').write_text(json.dumps(pairs,indent=2)+'\n')
# Moderate filters: selected integration; inherited unstable settings are not fixed.
filters=[]
for v in [0,3]:
 for mode in [0,1]:
  r=run((mode,v,0,20,1,7,16,1<<v,0,2,.4,1,1));filters.append(r)
assert all(not r['returncode'] for r in filters),filters
(O/'filters.json').write_text(json.dumps(filters,indent=2)+'\n')
# Expected unchanged legacy snare conversion: no/multi direct/other-FX only.
known=[]
for mode in [0,1]:
 for fx,mask in [(0,0),(1,0),(1,8)]:
  # last tuple uses tube, deliberately ineligible; new multi CLP must PASS above.
  r=run((mode,3,1,127,1,0,1,mask,0,0,1,1,fx));known.append(r)
  assert r['returncode'] and 'outside the range' in r.get('stderr',''),r
(O/'known-failures.json').write_text(json.dumps(known,indent=2)+'\n')
summary=dict(main_cases=len(rows),passed=len(rows),fallback_pairs=len(pairs)//2,moderate_filter_cases=len(filters),expected_legacy_failures=len(known),coverage='Actual six voice trigger/async/sync paths and verbatim mixer; deterministic hardware/control shims; inherited CLZ-zero host substitution; unsupported user sample synthesis aborts. No device/SD/morph boot claims.')
(O/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
