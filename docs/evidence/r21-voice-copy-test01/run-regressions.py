from pathlib import Path
import concurrent.futures,json,subprocess,time
root=Path('/workspace/lxr-desktop-app');out=Path(__file__).parent
commands={
'project-transaction':['python3','tests/project_transaction/run.py'],
'r20-ui':['python3','tests/r20_ui/run.py'],
'r20-integration':['python3','tests/r20_ui/integration/probe.py','.',str(out/'r20-integration')],
'delay':['python3','tests/r18_delay_mono/run.py',str(out/'delay')],
'usb':['python3','tests/r18_usb_midi/run.py',str(out/'usb')],
'compressor':['python3','tests/r18_compressor_mono/run.py',str(out/'compressor')],
'clp':['python3','tests/fx_clip_r18/compare.py'],
'parameter-sweeps':['python3','tests/bug001/run.py'],
}
def run(item):
 name,cmd=item;start=time.monotonic()
 with (out/(name+'.log')).open('w') as log:r=subprocess.run(cmd,cwd=root,stdout=log,stderr=subprocess.STDOUT)
 return {'name':name,'argv':cmd,'exit_code':r.returncode,'seconds':time.monotonic()-start,'log':name+'.log'}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
 results=[]
 for r in pool.map(run,commands.items()):
  results.append(r);print(json.dumps(r),flush=True)
(out/'regression-results.json').write_text(json.dumps(results,indent=2)+'\n')
