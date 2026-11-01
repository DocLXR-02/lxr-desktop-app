"""Compile-only .su sidecars after the sole firmware build; never link again."""
from pathlib import Path
import subprocess,shlex,json,struct
ROOT=Path(__file__).resolve().parents[2];out=ROOT/'build/r12-evidence/stack';out.mkdir(parents=True,exist_ok=True)
entries=json.loads((ROOT/'build/release/compile_commands.json').read_text());rows=[]
def executable_sections(p):
 b=p.read_bytes();off=struct.unpack_from('<I',b,32)[0];size,num,strings=struct.unpack_from('<HHH',b,46);h=[struct.unpack_from('<10I',b,off+i*size) for i in range(num)];st=h[strings];names=b[st[4]:st[4]+st[5]]
 return {names[x[0]:].split(b'\0')[0].decode():b[x[4]:x[4]+x[5]] for x in h if x[2]&4}
for x in entries:
 if not any(x['file'].endswith('/'+n+'.c') for n in ['SoloMix','FxBus','mixer','Distortion','DrumVoice','Snare','CymbalVoice','HiHat']):continue
 cmd=shlex.split(x['command']);idx=cmd.index('-o');original=Path(x['directory'])/cmd[idx+1];target=out/(Path(x['file']).stem+'.o');cmd[idx+1]=str(target);cmd.insert(1,'-fstack-usage');r=subprocess.run(cmd,cwd=x['directory'],text=True,capture_output=True);(out/(target.stem+'.log')).write_text(shlex.join(cmd)+'\n'+r.stdout+r.stderr);r.check_returncode();assert executable_sections(original)==executable_sections(target),x['file'];rows.append(dict(source=x['file'],command=cmd,code_sections_identical=True,stack_report=target.with_suffix('.su').read_text()))
(out/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print('PASS',len(rows),'compile-only stack reports; executable sections match original build objects; no second link')
