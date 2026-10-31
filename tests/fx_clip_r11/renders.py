"""Paired synthetic production-voice renders; no claim of musical equivalence."""
from pathlib import Path
import argparse,csv,json,os,subprocess,wave
import numpy as np
from common import ROOT
from build_integration import build
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'build/r11-renders');p.add_argument('--baseline',type=Path,required=True);a=p.parse_args();O=a.output;O.mkdir(parents=True,exist_ok=True)
exes=[build(O/'baseline',a.baseline,False),build(O/'candidate')];rows=[];env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
for v in [0,3,5]:
 for m in [5,10,20,127]:
  for profile in [0,1,2]:
   for path in [0,1]:
    raw=O/'frames.raw';cmd=[str(exes[path]),*map(str,[v,0,m,1,profile,2048,1<<v,0,2,1,1]),str(raw)]
    r=subprocess.run(cmd,text=True,capture_output=True,env=env);assert not r.returncode,(cmd,r.stderr);metrics=json.loads(r.stdout)
    x=np.fromfile(raw,'<i2').reshape(-1,2,64,2)[:,1].reshape(-1,2);assert len(x)==131072
    # Selected fader-equivalent input and post response include natural decay
    # and returning triggers; spectra are signal metrics, not bass preference.
    for lo,hi in [(0,8192),(32768,40960),(65536,73728)]:
     z=x[lo:hi].astype(float)/32768;fft=np.fft.rfft(z[:,0]*np.hanning(len(z)));f=np.fft.rfftfreq(len(z),1/44140);low=float(np.sum(abs(fft[(f>=20)&(f<150)])**2));total=float(np.sum(abs(fft)**2))
     rows.append(dict(voice=v+1,mix=m,profile=profile,path=['prerequisite','R11'][path],start_frame=lo,rms=float(np.sqrt(np.mean(z*z))),low_band_fraction=low/total if total else 0,peak=int(np.abs(x.astype(np.int32)).max()),max_step=int(np.abs(np.diff(x.astype(np.int32),axis=0)).max()),source_hash=metrics['source_hash'],state_hash=metrics['state_hash'],last_nonzero=metrics['last_nonzero']))
    # Keep compact representative WAVs; the complete metrics cover all runs.
    if m in [5,127] and profile in [1,2]:
     with wave.open(str(O/f'voice{v+1}-mix{m}-profile{profile}-{["baseline","R11"][path]}.wav'),'wb') as w:w.setparams((2,2,44140,len(x),'NONE','uncompressed'));w.writeframes(x.tobytes())
raw.unlink()
with (O/'metrics.csv').open('w') as f:w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
(O/'results.json').write_text(json.dumps(dict(renders=72,windows=len(rows),voices=[1,4,6],mix=[5,10,20,127],profiles=['static','zero/return','physical sweep'],source='Synthetic notes from real production voice paths with deterministic shims; FILTER-OFF. Not instrument recordings.'),indent=2)+'\n')
print('PASS 72 paired native production-path renders; 216 output/spectral windows, 24 representative WAVs')
