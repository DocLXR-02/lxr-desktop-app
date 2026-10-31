"""Run only isolated existing-ELF/component diagnostics, never boot firmware."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--elf',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();here=Path(__file__).resolve().parent;a.output=a.output.resolve();assert not a.output.is_relative_to(here);a.output.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env['PYTHONDONTWRITEBYTECODE']='1';env['LXR_TRIAGE_ELF']=str(a.elf.resolve());env['LXR_TRIAGE_OUTPUT']=str(a.output);env['LXR_TRIAGE_INPUTS']=str(here/'inputs')
for name in ['extra_elf.py','fm_waveforms.py','filter_elf.py','filter_alternatives.py']:
 r=subprocess.run([sys.executable,str(here/name)],text=True,capture_output=True,env=env);(a.output/(name+'.log')).write_text(r.stdout+r.stderr);r.check_returncode()
r=json.loads((a.output/'filter-elf-results.json').read_text());summary=[]
for x in r:summary.append({**{k:v for k,v in x.items() if k!='blocks'},'processed_blocks':len(x['blocks']),'last_block':x['blocks'][-1]})
alt=json.loads((a.output/'filter-alternative-results.json').read_text())
def clean(x):
 import math
 if isinstance(x,float) and not math.isfinite(x):return str(x)
 if isinstance(x,dict):return {k:clean(v) for k,v in x.items()}
 if isinstance(x,list):return list(map(clean,x))
 return x
import unicorn,capstone,numpy,elftools
bundle=dict(elf_sha256=hashlib.sha256(a.elf.read_bytes()).hexdigest(),versions=dict(unicorn=unicorn.__version__,capstone=capstone.__version__,numpy=numpy.__version__,pyelftools=elftools.__version__),filter=summary,alternatives=alt,instructions=json.loads((a.output/'instruction-results.json').read_text()),fm_waveforms=json.loads((a.output/'fm-waveform-results.json').read_text()))
(a.output/'results.json').write_text(json.dumps(clean(bundle),indent=2,allow_nan=False)+'\n');print('Existing-ELF triage probes complete; emulator/component evidence only. No firmware built.')
