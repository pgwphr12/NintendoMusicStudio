"""Differential test against unmodified upstream corrscope 0.11.1 Python code.
Dependencies are test-only; nothing is imported by the native player at runtime.
Usage: script upstream_root dependency_dir native_test_exe output_directory
"""
from pathlib import Path
import sys,struct,subprocess,json
upstream,deps,exe,out=map(lambda x:Path(x).resolve(),sys.argv[1:5]);out.mkdir(parents=True,exist_ok=True)
sys.path[:0]=[str(upstream),str(deps)]
import numpy as np
from corrscope.triggers import CorrelationTriggerConfig,PerFrameCache
from corrscope.spectrum import SpectrumConfig
class LiveWave:
 smp_s=48000
 amplification=1
 def __init__(self,data):self.data=data
 def get_padded(self,begin,end,stride):
  idx=np.arange(begin,end,stride);valid=(idx>=0)&(idx<len(self.data));output=np.zeros(len(idx),np.float32);output[valid]=self.data[idx[valid]];return output
 def get_around(self,pos,n,stride):return self.get_padded(pos-(n//2)*stride,pos+(n-n//2)*stride,stride)
t=np.arange(96000,dtype=np.float64)/48000;noise=np.random.default_rng(1234).uniform(-.5,.5,len(t));note=np.where(t<1,220.,330.)
inputs=[np.zeros(len(t)),.6*np.sin(2*np.pi*220.3*t),.5*np.sin(2*np.pi*32.1*t)+.16*np.sin(2*np.pi*96.3*t+.7),np.where(np.mod(t*110.1,1)<.3,.6,-.2),noise,.6*np.sin(2*np.pi*note*t),.3+.4*np.sin(2*np.pi*440.5*t),np.where(t<.7,.5*np.sin(2*np.pi*170.2*t),0)]
positions=list(range(4000,92000,800));expected=[];binary=out/'cases.bin'
with binary.open('wb') as f:
 f.write(struct.pack('<I',len(inputs)))
 for c,values in enumerate(inputs):
  values=np.asarray(values,dtype=np.float32);cfg=CorrelationTriggerConfig(mean_responsiveness=0.,edge_strength=1.,responsiveness=.5,reset_below=.3,pitch_tracking=SpectrumConfig());trigger=cfg(LiveWave(values),1920,1,60)
  f.write(struct.pack('<II',len(values),len(positions)));f.write(values.tobytes())
  for i,pos in enumerate(positions):
   r=trigger.get_trigger(pos,PerFrameCache());period=int(round(48000/r.freq_estimate)) if r.freq_estimate else 0;expected.append((c,i,int(r.result),period));f.write(struct.pack('<q',pos))
r=subprocess.run([str(exe),str(binary)],capture_output=True,text=True,check=True,timeout=120)
actual=[tuple(map(int,line.split(','))) for line in r.stdout.splitlines()];assert len(actual)==len(expected)
errors=[];equivalent=[]
for a,b in zip(actual,expected):
 if a!=b:
  data=np.asarray(inputs[a[0]],dtype=np.float32);xs=np.arange(-960,960);av=data[np.clip(xs+a[2],0,len(data)-1)];bv=data[np.clip(xs+b[2],0,len(data)-1)];error=float(np.mean(abs(av-bv)));different=float(np.mean(av!=bv));record={'actual':a,'reference':b,'sample_delta':a[2]-b[2],'wave_mean_error':error,'different_sample_fraction':different}
  if a[3]==b[3] and b[3]>0 and abs(abs(a[2]-b[2])-b[3])<=1 and error<.005 and different<.005:equivalent.append(record)
  else:errors.append(record)
report={'cases':len(inputs),'frames':len(expected),'exact_matches':len(expected)-len(errors)-len(equivalent),'equivalent_period_ties':equivalent,'mismatches':errors,'numpy':np.__version__,'source':'unmodified corrscope 6838a0d30057543b928565df29c57ea50eba9359','pass':not errors}
(out/'result.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k not in ('mismatches','equivalent_period_ties')},indent=2));print("Equivalent periodic ties:",len(equivalent));print(errors[:12]);sys.exit(bool(errors))
