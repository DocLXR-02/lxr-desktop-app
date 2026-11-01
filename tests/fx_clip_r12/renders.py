"""Paired synthetic production voice/mixer renders; never instrument recordings."""
from pathlib import Path
import subprocess,json,os,wave,sys
import numpy as np
ROOT=Path(__file__).resolve().parents[2];out=ROOT/'build/r12-renders';out.mkdir(parents=True,exist_ok=True);checks=ROOT/'build/r12-checks';results=[]
for voice,profile,mix in [(v,pr,m) for v in [0,3,5] for pr in [0,2] for m in [5,20,127]]+[(3,10,10),(3,10,50)]:
 waves=[]
 for label,folder in [('R11','baseline'),('R12','candidate')]:
  name=f'voice{voice+1}-profile{profile}-mix{mix}-{label}';raw=out/(name+'.raw');cmd=[str(checks/folder/'check'),*map(str,[voice,0,mix,1,profile,768,63 if profile==10 else 1<<voice,0,2,1,1,1]),str(raw)];r=subprocess.run(cmd,text=True,capture_output=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));r.check_returncode();a=np.fromfile(raw,dtype='<i2').reshape(-1,4);raw.unlink();waves.append(a);(out/(name+'.json')).write_text(json.dumps(dict(command=cmd,metrics=json.loads(r.stdout)),indent=2)+'\n')
  with wave.open(str(out/(name+'.wav')),'wb') as f:f.setnchannels(4);f.setsampwidth(2);f.setframerate(44140);f.writeframes(a.tobytes())
 a,b=waves;x=a.astype(np.float64);y=b.astype(np.float64);results.append(dict(voice=voice,profile=profile,mix=mix,r11_rms=float(np.sqrt(np.mean(x*x))),r12_rms=float(np.sqrt(np.mean(y*y))),difference_rms=float(np.sqrt(np.mean((x-y)**2))),r11_peak=int(np.max(np.abs(x))),r12_peak=int(np.max(np.abs(y))),r12_max_step=int(np.max(np.abs(np.diff(y,axis=0)))),r11_channel_rms=np.sqrt(np.mean(x*x,axis=0)).tolist(),r12_channel_rms=np.sqrt(np.mean(y*y,axis=0)).tolist(),channel_exact=[bool(np.array_equal(a[:,i],b[:,i])) for i in range(4)],equivalence='NOT asserted: mono before PAN and separate histories are intentional'))
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print('PASS',len(results),'paired synthetic comparisons; 4 channels DAC1 L/R, DAC2 L/R; no musical-equivalence claim')
