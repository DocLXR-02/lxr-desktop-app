"""Numerical recurrence probes, not proposed production implementations."""
from pathlib import Path
import json,math,struct,os
import numpy as np
from filter_elf import records
H=Path(os.environ.get('LXR_TRIAGE_INPUTS',str(Path(__file__).parent/'inputs')));OUT=Path(os.environ['LXR_TRIAGE_OUTPUT'])

def run(name,policy):
 rows=records(H/(name+'.bin'));kind,n,state,_=rows[0];data=b''.join(r[3] for r in rows);f,g,R,s1,s2,aa,bb,drive=struct.unpack('<8f',state);f=np.float32(g);R=np.float32(R);s1=np.float32(s1);s2=np.float32(s2);g0=np.float32(1/(1+np.float32(f*2*R)));f1=np.float32(f*f*g0)
 source=np.frombuffer(data,'<i2');signal=np.r_[source,np.zeros(max(0,4096-len(source)),dtype=np.int16)]
 if kind==8:return dict(name=name,policy=policy,finite=True,samples=len(signal),peak_state=0,bypass=True)
 first=None;peak=0;out=[]
 def clip(v,stage,i):
  nonlocal first
  if policy=='bounded-tanh':return np.float32(2*math.tanh(float(v)*.5))
  x=np.float32(.5*v)
  if policy=='wide-rational':a=float(x)*float(x);num=(a+105)*a+945;den=(15*a+420)*a+945;r=np.float32(num/den if math.isfinite(den) else float('nan'))
  else:
   a=np.float32(x*x);num=np.float32(np.float32(a+105)*a+945);den=np.float32(np.float32(15*a+420)*a+945)
   if not math.isfinite(float(den)) and first is None:first=dict(sample=i,stage=stage,operation='rational denominator',x=float(x),a=float(a),numerator=str(num),denominator=str(den))
   r=np.float32(num/den)
  return np.float32(v*r)
 with np.errstate(over='ignore',invalid='ignore',divide='ignore'):
  for i,iv in enumerate(signal):
   x=clip(np.float32((np.float32(iv)/32767)*drive),'input',i)
   c=s1 if policy=='linear-state' else clip(s1,'state',i)
   y1=np.float32((f1*x+s2+f*g0*s1)/(f1+1));xx=np.float32(x-y1);y0=np.float32((c+f*xx)*g0)
   s1=np.float32(c+2*f*(xx-2*R*y0));s2=np.float32(s2+2*f*y0)
   peak=max(peak,float(abs(s1)),float(abs(s2)))
   if not all(math.isfinite(float(v)) for v in [s1,s2,y1]):
    if first is None:first=dict(sample=i,stage='state',operation='recurrence overflow',s1=str(s1),s2=str(s2),y1=str(y1))
    return dict(name=name,policy=policy,first=first,finite=False,stopped_sample=i,peak_state=peak)
   out.append(float(y1))
 return dict(name=name,policy=policy,first=first,finite=True,samples=len(out),peak_state=peak)

def large_state_eigenvalues(name):
 _,_,state,_=records(H/(name+'.bin'))[0];_,f,R,*_=struct.unpack('<8f',state);g=1/(1+2*f*R);d1=f*g/(1+f*f*g);d2=1/(1+f*f*g)
 out=[]
 for c in [1,0,1/15]:
  y01=(c-f*d1)*g;y02=-f*d2*g
  A=np.array([[c+2*f*(-d1-2*R*y01),2*f*(-d2-2*R*y02)],[2*f*y01,1+2*f*y02]])
  eig=np.linalg.eigvals(A);out.append(dict(c=c,eigenvalues=[str(x) for x in eig],radius=float(max(abs(eig)))))
 return out
if __name__=='__main__':
 r=dict(probes=[run(n,p) for n in ['voice1-C7','voice4-C11','voice1-C4'] for p in ['original-rational','wide-rational','bounded-tanh','linear-state']],eigenvalues={n:large_state_eigenvalues(n) for n in ['voice1-C7','voice4-C11']})
 (OUT/'filter-alternative-results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
