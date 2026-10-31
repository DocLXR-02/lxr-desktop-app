"""Compile native production-input capture from the previous audit's prepared tree."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess
p=argparse.ArgumentParser();p.add_argument('--audit',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
meta=json.loads((a.audit/'metadata.json').read_text());assert meta['pin']=='8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7'
cmd=meta['commands'][0].copy();cmd[cmd.index('-o')+1]=str(a.output/'capture-native');cmd+= [str(Path(__file__).parent/'capture.c'),'-Wl,--wrap=SVF_calcBlockZDF','-Wl,--wrap=Snare_calcAsync']
r=subprocess.run(cmd,capture_output=True,text=True);(a.output/'capture-compile.log').write_text(r.stdout+r.stderr);r.check_returncode();results=[]
for v,cfg,m,blocks,ui in [(0,7,20,128,False),(3,11,20,128,False),(0,0,20,128,False),(0,11,20,128,False),(0,4,20,8,False),(0,7,0,128,False),(3,11,0,128,False),(3,11,20,128,True)]:
 name=f'voice{v+1}-C{cfg}'+('-M0' if m==0 else '')+('-UI3' if ui else '')
 env=os.environ.copy();env.setdefault('ASAN_OPTIONS','detect_leaks=0');env['FILTER_CAPTURE']=str(a.output/(name+'.bin'))
 if ui:env['NOISE_UI3']='1'
 r=subprocess.run([str(a.output/'capture-native'),str(v),str(cfg),str(m),'1','0',str(blocks)],text=True,capture_output=True,env=env);(a.output/(name+'.log')).write_text(r.stdout+r.stderr)
 expected_failure=cfg in [7,11] and (v==3 or cfg==7)
 assert bool(r.returncode)==expected_failure
 results.append(dict(name=name,returncode=r.returncode,bytes=(a.output/(name+'.bin')).stat().st_size,sha256=hashlib.sha256((a.output/(name+'.bin')).read_bytes()).hexdigest(),diagnostic=r.stderr.strip()))
(a.output/'capture-metadata.json').write_text(json.dumps(dict(pin=meta['pin'],compiler=meta['compiler'],command=cmd,cases=results),indent=2)+'\n')
print('Captured eight focused cases; original-source failures retained, no firmware build.')
