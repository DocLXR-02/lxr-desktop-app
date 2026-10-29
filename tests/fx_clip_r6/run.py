"""Actual production R6/R5 DSP, synthetic musical probes and lifecycle checks."""
from pathlib import Path
import argparse, json, os, subprocess, tempfile, hashlib, wave
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).parent
BASE='af3562fece090288a83727524c1035b79f4cf20b'
REL='Core/Src/Soundengine/DSPAudio/FX/Distortion.c'
FS=44140
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'build/r6-host');args=p.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env.setdefault('ASAN_OPTIONS','detect_leaks=0')
def call(cmd):subprocess.run([str(x) for x in cmd],check=True,env=env,cwd=ROOT)
def body(s,name):
    start=s.index(name+'(');start=s.rfind('\n',0,start)+1
    a=s.index('{',start);depth=1;b=a+1
    while depth:
        depth+=(s[b]=='{')-(s[b]=='}');b+=1
    return s[start:b]
base=subprocess.check_output(['git','show',BASE+':'+REL],cwd=ROOT).decode().replace('\r\n','\n')
src=(ROOT/REL).read_text()
assert body(base,'Distortion_clipFeedback')==body(src,'Distortion_clipFeedback')
assert '#define CLIP_IDLE_HOLD_SAMPLES 441u' in src
assert '#define CLIP_FEEDBACK_FADE_STEP (1.f / 221.f)' in src
# The fixture's kit callbacks are mocks: preserve actual integration call sites too.
for path in ['Core/Src/Soundengine/FxBus.c','Core/Src/Preset/presetManager.c']:
    assert (ROOT/path).read_bytes()==subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT)
flags=[os.environ.get('CC','cc'),'-std=c11','-O2','-fno-strict-aliasing',
       '-fsanitize=address,undefined,bounds,float-cast-overflow','-fno-sanitize-recover=all',
       '-Werror=implicit-function-declaration','-I'+str(ROOT/'tests/fx_clip/include'),'-I'+str(ROOT/'Core/Inc')]
cases=[];identity_checks=0;max_idle=0
with tempfile.TemporaryDirectory() as temp:
    d=Path(temp);(d/'r5.c').write_text(base)
    # Existing state and all-six-bus-route checks remain meaningful with new voicing.
    for name,unit in [('inherited',ROOT/'tests/fx_clip/unit.c'),('state',HERE/'state.c')]:
        call(flags+['-DDISTORTION_SOURCE="'+str(ROOT/REL)+'"',unit,'-lm','-o',d/name]);call([d/name])
    call(flags+['-ffunction-sections','-fdata-sections',ROOT/'tests/fx_clip/fxbus.c',ROOT/REL,
                ROOT/'Core/Src/Soundengine/FxBus.c','-Wl,--gc-sections','-lm','-o',d/'bus']);call([d/'bus'])
    for version,source in [('R5',d/'r5.c'),('R6',ROOT/REL)]:
        call(flags+(['-DR6'] if version=='R6' else [])+[HERE/'render.c',source,'-lm','-o',d/version])
    last_inputs={}
    def render(version,scenario,row,travel,mode,block=64,kind=2,dw=1):
        raw=d/(version+'.raw')
        command=[d/version,scenario,row,travel,mode,block,kind,dw,raw]
        count=int(subprocess.check_output([str(x) for x in command],cwd=ROOT,env=env))
        last_inputs[(scenario,row,travel,mode)]=count
        a=np.frombuffer(raw.read_bytes(),dtype='<i2').copy().reshape(-1,2)
        assert len(a)==FS*3//2
        return a
    # Other distortion algorithms and return-only recurrence must remain identical.
    for kind in [0,1,2]:
        for mode in [0,1,2]:
            scenario=9 if kind==2 else 2
            a=render('R5',scenario,3,1,mode,kind=kind)
            b=render('R6',scenario,3,1,mode,kind=kind)
            assert np.array_equal(a,b),('unchanged path',kind,mode)
            identity_checks+=1
    def bands(a):
        a=a[:int(.48*FS),0].astype(float)/32768
        w=np.hanning(len(a));s=np.abs(np.fft.rfft(a*w))**2
        f=np.fft.rfftfreq(len(a),1/FS);scale=len(a)*np.sum(w*w)
        return {name:float(np.sqrt(2*np.sum(s[(f>=lo)&(f<hi)])/scale))
                for name,lo,hi in [('low_rms',30,150),('high_rms',500,5000)]}
    matrix=[(sc,row,v,mode) for sc in [0,1,2] for row in range(4)
            for v in [1,.9,.85,.8,.7,.5,.4] for mode in [0,2]]
    matrix += [(sc,row,v,mode) for sc in range(3,9) for row in [0,3] for v in [.85,.5] for mode in [0,1,2]]
    # col=0 probes distinguish feedforward voicing from regenerative metrics.
    matrix += [(sc,4,v,mode) for sc in [0,1,2] for v in [1,.85,.5,.4] for mode in [0,2]]
    for scenario,row,travel,mode in matrix:
        record=dict(scenario=scenario,row=row,travel=travel,mode=mode)
        for version in ['R5','R6']:
            a=render(version,scenario,row,travel,mode)
            assert np.max(np.abs(a.astype(np.int32)))<=32768
            # All scenarios eventually provide exact input loss, including the natural decay.
            assert not np.any(a[FS:]),('persistent idle',version,record)
            start=last_inputs[(scenario,row,travel,mode)]
            record['last_nonzero_input_frame_exclusive']=start
            idx=np.flatnonzero(np.any(a[start:]!=0,axis=1))
            idle_ms=float((idx[-1]+1)/FS*1000) if len(idx) else 0
            assert idle_ms<100,('idle settling',record,version,idle_ms)
            if version=='R6':max_idle=max(max_idle,idle_ms)
            m=bands(a);m.update(peak=int(np.max(np.abs(a.astype(np.int32)))),
                              max_step=int(np.max(np.abs(np.diff(a.astype(np.int32),axis=0)))),
                              idle_to_last_nonzero_ms=idle_ms)
            record[version]=m
            if scenario in [0,3,4,5,6] and row==0 and travel==.85 and mode==2:
                with wave.open(str(args.output/f'{version}-scenario{scenario}.wav'),'wb') as wav:
                    wav.setparams((2,2,FS,len(a),'NONE','not compressed'));wav.writeframes(a.astype('<i2').tobytes())
            if version=='R6' and (scenario,row,travel,mode) in [(0,0,.85,0),(3,3,.5,2),(4,0,.85,1),(5,3,.85,2)]:
                for block in [1,7]:
                    assert np.array_equal(a,render(version,scenario,row,travel,mode,block))
        cases.append(record)
    # Dry path preserved at zero wet amount, including asymmetric and mono routes.
    for mode in [0,1,2]:
        assert np.array_equal(render('R5',2,3,1,mode,dw=0),render('R6',2,3,1,mode,dw=0))
    result=dict(base=BASE,source_sha256=hashlib.sha256((ROOT/REL).read_bytes()).hexdigest(),
                render_pairs=len(cases),unchanged_path_pairs=identity_checks,max_R6_idle_ms=max_idle,
                metrics='Hann-windowed band RMS on first 480ms, normalized int16; raw peaks/steps in LSB',cases=cases)
    (args.output/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'PASS {len(cases)} R5/R6 render pairs; {identity_checks} unchanged path pairs; zero idle; block 1/7/64; max R6 idle {max_idle:.3f} ms')
