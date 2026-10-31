"""Actual production R10 multivoice policy and DSP against pinned R9."""
from pathlib import Path
import argparse,csv,hashlib,io,json,os,re,subprocess,tarfile,tempfile,wave
import numpy as np
PIN='e2d7963d97cd2d5918e3b80b88156fbbfaa60f47';FS=44140
HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
a.repo=HERE.parents[1];a.output=a.output.resolve()
a.output.mkdir(parents=True,exist_ok=True)
def git(*args):return subprocess.check_output(['git','-C',str(a.repo),*args])
def function(s,name):
    m=re.search(r'^(?:IN_ITCM_RAM )?(?:static )?(?:inline )?(?:const float \*|float |void )'+name+r'\([^;]*?\)\s*\{',s,re.M);assert m,name
    n=m.end();depth=1
    while depth:depth+=(s[n]=='{')-(s[n]=='}');n+=1
    return s[m.start():n]
status=git('status','--porcelain=v1');compiler=subprocess.check_output([os.environ.get('CC','cc'),'--version'],text=True).splitlines()[0]
scenarios=['solo','fade_lower','fade_higher','silent_high','silent_zero','silent_low','collective','alternating','six_mix','routes','six_hot']
metrics=[];checks=0;worst_idle=0
with tempfile.TemporaryDirectory(prefix='clp-multivoice-') as tmp:
    root=Path(tmp);src=a.repo
    bus=src/'Core/Src/Soundengine/FxBus.c';dsp=src/'Core/Src/Soundengine/DSPAudio/FX/Distortion.c';pole=src/'Core/Src/Soundengine/DSPAudio/1PoleLp.c'
    mix=(src/'Core/Src/Soundengine/DSPAudio/mixer.c').read_text()
    reference=git('show',PIN+':Core/Src/Soundengine/FxBus.c').decode().replace('\r\n','\n')
    (root/'extracted.inc').write_text(function(reference,'FxBus_clipCoreGain').replace('FxBus_clipCoreGain','R9_coreGain')+'\n'+function(reference,'FxBus_clipFaderGains').replace('FxBus_clipFaderGains','R9_clipFaderGains').replace('FxBus_clipCoreGain','R9_coreGain')+'\n'+function(mix,'mixer_addDataToOutput')+'\n')
    cmd=[os.environ.get('CC','cc'),'-std=c11','-D_DEFAULT_SOURCE','-O2','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-fsanitize=address,undefined,bounds,float-cast-overflow','-fno-sanitize-recover=all','-I'+str(root),'-I'+str(src/'tests/fx_clip/include'),'-I'+str(src/'Core/Inc'),str(HERE/'multivoice.c'),str(dsp),str(bus),str(pole),'-Wl,--gc-sections','-lm','-o',str(root/'compare')]
    subprocess.run(cmd,check=True)
    env=os.environ.copy();env.setdefault('ASAN_OPTIONS','detect_leaks=0')
    held={}
    for sc,name in enumerate(scenarios):
        for row in [0,1]:
            for ch in [1,2]:
                data=root/'frames.raw'
                last,clips,state_size=map(int,subprocess.check_output([str(root/'compare'),str(sc),str(row),str(ch),'1',str(data)],text=True,env=env).split())
                x=np.fromfile(data,np.float32).reshape(-1,18);checks+=len(x)*2
                assert np.all(np.isfinite(x)) and np.all(x[-int(.1*FS):,7:13]==0),'invalid or persistent idle'
                if sc in [3,4,5]:held[(sc,row,ch)]=x.copy()
                for policy in range(3):
                    y=x[:,7+2*policy:9+2*policy].astype(np.int32);idx=np.flatnonzero(np.any(y[last:]!=0,axis=1));idle=(idx[-1]+1)*1000/FS if len(idx) else 0
                    worst_idle=max(worst_idle,idle);assert idle<100
                    for lo,hi in [(.10,.18),(.40,.48),(.75,.82),(1.14,1.22)]:
                        z=x[int(lo*FS):int(hi*FS)];out=z[:,7+2*policy:9+2*policy]/32768
                        metrics.append(dict(scenario=name,row=row,channels=ch,policy=['R9','R10','unity'][policy],start=lo,end=hi,gain=float(z[:,policy].mean()),out_rms=float(np.sqrt(np.mean(out**2))),core_rms=float(np.sqrt(np.mean(z[:,3:5]**2))),bass_rms=float(np.sqrt(np.mean(z[:,5:7]**2))),peak=int(np.abs(y).max()),max_step=int(np.abs(np.diff(y,axis=0)).max()),idle_ms=idle,mixer_saturations=clips))
                if row==0 and ch==2 and sc in [2,3,6,7,8,9]:
                    for policy in range(3):
                        with wave.open(str(a.output/f'{name}-{["R9","R10","unity"][policy]}.wav'),'wb') as w:
                            w.setparams((2,2,FS,len(x),'NONE','not compressed'));w.writeframes(x[:,7+2*policy:9+2*policy].astype('<i2').tobytes())
                    with (a.output/f'{name}-gains.csv').open('w') as f:
                        f.write('frame,R9,R10,unity,route_mask\n')
                        for n in range(0,len(x),64):f.write(f'{n},{x[n,0]:.9g},{x[n,1]:.9g},{x[n,2]:.9g},{int(x[n,16])}\n')
    # Counterexample variants have byte-identical consumed input and core/bass.
    for row in [0,1]:
        for ch in [1,2]:
            ref=held[(3,row,ch)]
            assert np.all(ref[int(.02*FS):,1]==1) # after cold smoother ramp, silent high fader holds unity
            for sc in [4,5]:
                candidate=held[(sc,row,ch)]
                assert np.array_equal(ref[:,14:16],candidate[:,14:16]) and np.array_equal(ref[:,3:7],candidate[:,3:7])
                assert np.array_equal(ref[:,7:9],candidate[:,7:9]) # R9 count fallback unchanged
    # Endpoints, including fractional D/W with zero VOL, use baseline arithmetic.
    for dw in ['0','.4','1']:
        subprocess.run([str(root/'compare'),'8','1','2',dw,str(root/'endpoint.raw'),'zero_vol'],check=True,env=env)
    subprocess.run([str(root/'compare'),'8','1','2','0',str(root/'dry.raw')],check=True,env=env)
    hashes={str(p.relative_to(src)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [dsp,bus,pole,src/'Core/Src/Soundengine/DSPAudio/mixer.c']}
assert git('status','--porcelain=v1')==status,'Source checkout status changed'
with (a.output/'metrics.csv').open('w') as f:
    w=csv.DictWriter(f,fieldnames=list(metrics[0]));w.writeheader();w.writerows(metrics)
result=dict(reference_commit=PIN,compiler=compiler,numpy=np.__version__,scenarios=scenarios,main_runs=44,additional_endpoint_runs=4,state_memcmp_checks=checks,partition_sizes=[1,7,64],state_bytes=state_size,worst_idle_ms=worst_idle,source_sha256=hashes,compile_command=cmd,notes='Synthetic source/fader fixtures; actual R10 helper/DSP/pole/mixer adder, pinned R9 helper. Not a full engine or ARM execution.')
(a.output/'results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
