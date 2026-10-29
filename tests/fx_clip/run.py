"""Compile actual production DSP, bounded R4 reference and runtime-state checks."""
from pathlib import Path
import os,subprocess,tempfile,json,math,array,argparse
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).parent
SRC=ROOT/'Core/Src/Soundengine/DSPAudio/FX/Distortion.c'
BASE='c870f7249d7ee0d65423bc8b7bfdb422dcc74482'
def call(args,**kw):return subprocess.run(args,check=True,**kw)
parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path);args=parser.parse_args()
with tempfile.TemporaryDirectory() as td:
 d=Path(td)
 flags=[os.environ.get('CC','cc'),'-std=c11','-O2','-fno-strict-aliasing','-fsanitize=undefined,bounds','-fno-sanitize-recover=all','-Werror=implicit-function-declaration','-I'+str(HERE/'include'),'-I'+str(ROOT/'Core/Inc')]
 call(flags+['-DDISTORTION_SOURCE="'+str(SRC)+'"',str(HERE/'unit.c'),'-lm','-o',str(d/'unit')]);call([str(d/'unit')])
 call(flags+['-ffunction-sections','-fdata-sections',str(HERE/'fxbus.c'),str(SRC),str(ROOT/'Core/Src/Soundengine/FxBus.c'),'-Wl,--gc-sections','-lm','-o',str(d/'fxbus')]);call([str(d/'fxbus')])
 original=subprocess.check_output(['git','show',BASE+':Core/Src/Soundengine/DSPAudio/FX/Distortion.c'],cwd=ROOT).decode().replace('\r\n','\n')
 needle='\tuint16_t itg = (int16_t) index;';assert original.count(needle)==1
 original=original.replace(needle,'\tif(index >= 256.f) return table_1n914[256];\n'+needle)
 (d/'reference.c').write_text(original)
 for name,source in [('reference',d/'reference.c'),('candidate',SRC)]:
  call(flags+[str(HERE/'render.c'),str(source),'-lm','-o',str(d/name)])
 call([str(d/'reference'),str(d/'ref.raw'),'64'])
 call([str(d/'candidate'),str(d/'out.raw'),'64'])
 call([str(d/'candidate'),str(d/'single.raw'),'1'])
 assert (d/'single.raw').read_bytes()==(d/'out.raw').read_bytes(),'block-boundary dependence'
 ref=array.array('h');ref.frombytes((d/'ref.raw').read_bytes());out=array.array('h');out.frombytes((d/'out.raw').read_bytes())
 length=2*44140*2;assert len(out)==12*3*2*length
 results=[]
 for k in range(12):
  for sc in range(3):
   for ch in [1,2]:
    off=((k*3+sc)*2+ch-1)*length
    a=out[off:off+length];b=ref[off:off+length]
    if sc==0:assert a==b,('continuous-input regression',k,ch)
    cut=.5 if sc==1 else .25
    if sc:
     assert max(abs(v) for v in a[int((cut+.05)*44140)*2:])==0,('silence failure',k,sc,ch)
     indices=[i//2 for i,v in enumerate(a[int(cut*44140)*2:]) if abs(v)>32]
     ms=(max(indices)+1)/44140*1000 if indices else 0
     assert ms<35,(k,sc,ch,ms)
     results.append(dict(kit=k,scenario=sc,channels=ch,settle_ms=ms))
 print('PASS 72 actual-DSP renders; continuous input equals bounded R4; silence suppression; mono/stereo; sample-vs-64-frame invariance')
 result=dict(base=BASE,reference='R4 plus diode endpoint bound',renders=72,block_sizes=[1,64],max_settle_ms=max(r['settle_ms'] for r in results),cases=results)
 if args.output:
  args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')

