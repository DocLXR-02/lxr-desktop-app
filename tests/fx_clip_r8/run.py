"""Actual R8 DSP, R7 reference, routing capture, and fader replay checks."""
from pathlib import Path
import argparse, hashlib, json, os, re, subprocess, tempfile, wave
import numpy as np
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).parent
BASE='6d7cb7f7763158ed9abc465a72df7f2560f26b57';REL='Core/Src/Soundengine/DSPAudio/FX/Distortion.c';FS=44140
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'build/r8-host');args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env.setdefault('ASAN_OPTIONS','detect_leaks=0')
def call(cmd):subprocess.run([str(x) for x in cmd],cwd=ROOT,env=env,check=True)
def prior(path):return subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT).decode().replace('\r\n','\n')
def body(s,name):
    m=re.search(r'^(?:IN_ITCM_RAM )?(?:static )?(?:inline )?(?:const float \*|float |void )'+name+r'\([^;]*?\)\s*\{',s,re.M)
    assert m,name
    end=m.end();depth=1
    while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
    return s[m.start():end]
base=prior(REL);src=(ROOT/REL).read_text()
restored=body(src,'Distortion_processWithClipGain').replace('Distortion_processWithClipGain(', 'Distortion_process(').replace(', const float *clipGain)',')').replace(' * (clipGain ? clipGain[i] : 1.f)','')
assert restored==body(base,'Distortion_process'), 'unexpected R7 DSP change'
for name in ['Distortion_clipFeedback','Distortion_init','Distortion_resetClip']:
    assert body(src,name)==body(base,name)
for path in ['Core/Src/Preset/presetManager.c','Core/Src/IO/adcPots.c']+[f'Core/Src/Soundengine/DSPAudio/{f}.c' for f in ['DrumVoice','Snare','CymbalVoice','HiHat','1PoleLp']]:
    assert (ROOT/path).read_text()==prior(path),path
mixer=(ROOT/'Core/Src/Soundengine/DSPAudio/mixer.c').read_text()
calc=body(mixer,'mixer_calcNextSampleBlock')
assert calc.index('mixer_clipFaderForBlock()')<calc.index('calcDrumVoiceSyncBlock(0,')
calc=calc.replace('\t/* Capture before any voice advances its live fader smoother. */\n\tconst float *clipGains = mixer_clipFaderForBlock();\n\n','')
calc=calc.replace('FxBus_processWithClipGain(fxBusData, &output[pos], &output[pos+1], &output2[pos], &output2[pos+1], OUTPUT_DMA_SIZE, clipGains);','FxBus_process(fxBusData,  &output[pos],&output[pos+1],&output2[pos],&output2[pos+1], OUTPUT_DMA_SIZE);')
assert calc==body(prior('Core/Src/Soundengine/DSPAudio/mixer.c'),'mixer_calcNextSampleBlock')
flags=[os.environ.get('CC','cc'),'-std=c11','-D_DEFAULT_SOURCE','-O2','-fno-strict-aliasing','-fsanitize=address,undefined,bounds,float-cast-overflow','-fno-sanitize-recover=all','-Werror=implicit-function-declaration','-I'+str(ROOT/'tests/fx_clip/include'),'-I'+str(ROOT/'Core/Inc')]
cases=[];moving=[]
with tempfile.TemporaryDirectory() as tmp:
    d=Path(tmp);(d/'r7.c').write_text(base)
    for label,path in [('inherited',ROOT/'tests/fx_clip/unit.c'),('r7state',ROOT/'tests/fx_clip_r7/state.c'),('state',HERE/'state.c')]:
        call(flags+['-DDISTORTION_SOURCE="'+str(ROOT/REL)+'"',path,'-lm','-o',d/label]);call([d/label])
    (d/'capture.inc').write_text('static float mixer_clipGains[OUTPUT_DMA_SIZE];\n'+body(mixer,'mixer_clipFaderForBlock'))
    call(flags+['-DOUTPUT_DMA_SIZE=64','-ffunction-sections','-fdata-sections','-I'+str(d),HERE/'integration.c',ROOT/'Core/Src/Soundengine/FxBus.c',ROOT/REL,ROOT/'Core/Src/Soundengine/DSPAudio/1PoleLp.c','-Wl,--gc-sections','-lm','-o',d/'integration'])
    call([d/'integration',args.output/'gain-trajectories.csv'])
    (d/'r7-bus.c').write_text(prior('Core/Src/Soundengine/FxBus.c'))
    for version,dsp,bus in [('R7',d/'r7.c',d/'r7-bus.c'),('R8',ROOT/REL,ROOT/'Core/Src/Soundengine/FxBus.c')]:
        call(flags+(['-DR8'] if version=='R8' else [])+['-ffunction-sections','-fdata-sections',HERE/'bus_probe.c',dsp,bus,ROOT/'Core/Src/Soundengine/DSPAudio/1PoleLp.c','-Wl,--gc-sections','-lm','-o',d/(version+'bus')])
    fallback_pairs=0
    for mask in [0,3,63,1,2,4,8,16,32]:
        for route in range(6):
            for level in [.25,1]:
                for version in ['R7','R8']:call([d/(version+'bus'),mask,route,level,d/(version+'bus.raw')])
                if mask in [0,3,63] or level==1:
                    assert (d/'R7bus.raw').read_bytes()==(d/'R8bus.raw').read_bytes(),('bus fallback',mask,route,level)
                    fallback_pairs+=1
    for version,source in [('R7',d/'r7.c'),('R8',ROOT/REL)]:
        call(flags+['-DR7','-DTEST_BASS_WEIGHT=-0.25']+(['-DR8'] if version=='R8' else [])+[HERE/'render.c',source,'-lm','-o',d/version])
    def render(version,sc,row,travel,mode,profile=0,voice=0,block=64,kind=2,dw=1):
        file=d/(version+'.raw')
        cmd=[d/version,sc,row,travel,mode,block,kind,dw,file,profile,voice]
        last,trace,peak,mean=subprocess.check_output([str(x) for x in cmd],cwd=ROOT,env=env,text=True).split()
        a=np.frombuffer(file.read_bytes(),'<i2').copy().reshape(-1,2)
        return a,dict(last_input=int(last),core_trace=trace,wet_peak_block=float(peak),mean_gain=float(mean))
    matrix=[(sc,row,v,mode) for sc in [0,1,2] for row in range(4) for v in [1,.9,.85,.8,.7,.5,.4] for mode in [0,2]]
    matrix += [(sc,row,v,mode) for sc in range(3,9) for row in [0,3] for v in [.85,.5] for mode in [0,1,2]]
    matrix += [(sc,4,v,mode) for sc in [0,1,2] for v in [1,.85,.5,.4] for mode in [0,2]]
    matrix += [(sc,row,v,mode) for sc in [10,11] for row in [0,3,6] for v in [1,.5] for mode in [0,1,2]]
    matrix += [(sc,6,v,mode) for sc in [0,1,2] for v in [1,.85,.5] for mode in [0,2]]
    max_idle=0
    for sc,row,v,mode in matrix:
        a,ma=render('R7',sc,row,v,mode);b,mb=render('R8',sc,row,v,mode,profile=4)
        assert np.array_equal(a,b) and ma['core_trace']==mb['core_trace'],('unity R7 mismatch',sc,row,v,mode)
        assert not np.any(b[FS:]),'persistent idle'
        idx=np.flatnonzero(np.any(b[mb['last_input']:]!=0,axis=1));idle=(idx[-1]+1)/FS*1000 if len(idx) else 0
        max_idle=max(max_idle,idle);assert idle<100
        cases.append(dict(scenario=sc,row=row,travel=v,mode=mode,idle_ms=float(idle),peak=int(np.max(np.abs(b.astype(np.int32))))))
    # Moving output gain, with both physically scaled input and identical-input controls.
    for profile in [2,3]:
        for sc in [1,5,12]:
            for row in [0,3]:
                for mode in [0,1,2]:
                    for voice in [0,5]:
                        a,ma=render('R7',sc,row,1,mode,profile,voice)
                        b,mb=render('R8',sc,row,1,mode,profile,voice)
                        assert ma['core_trace']==mb['core_trace'], 'new gain changed feedback'
                        assert mb['wet_peak_block']<.89749
                        idx=np.flatnonzero(np.any(b[mb['last_input']:]!=0,axis=1))
                        idle=(idx[-1]+1)/FS*1000 if len(idx) else 0
                        assert idle<100 and not np.any(b[int(1.49*FS):]),'moving-fader idle'
                        record=dict(profile=profile,scenario=sc,row=row,mode=mode,voice=voice,mean_gain=mb['mean_gain'],idle_ms=float(idle),R7_peak=int(np.max(np.abs(a.astype(np.int32)))),R8_peak=int(np.max(np.abs(b.astype(np.int32)))),R7_max_step=int(np.max(np.abs(np.diff(a.astype(np.int32),axis=0)))),R8_max_step=int(np.max(np.abs(np.diff(b.astype(np.int32),axis=0)))))
                        moving.append(record)
                        if sc==12 and row==0 and mode==2:
                            for version,waveform in [('R7',a),('R8',b)]:
                                with wave.open(str(args.output/f'{version}-profile{profile}-voice{voice}.wav'),'wb') as w:
                                    w.setparams((2,2,FS,len(waveform),'NONE','not compressed'));w.writeframes(waveform.astype('<i2').tobytes())
                        if sc==12 and row==3 and mode==2 and voice==0:
                            for block in [1,7]:assert np.array_equal(b,render('R8',sc,row,1,mode,profile,voice,block)[0])
    for mode in [0,1,2]:
        for v in [0,.01,.1,.5,.85,1]:
            a,ma=render('R7',12,3,v,mode,profile=1);b,mb=render('R8',12,3,v,mode,profile=1)
            assert ma['core_trace']==mb['core_trace']
            for dw in [0,.4,1]:
                assert np.array_equal(render('R7',12,5,v,mode,profile=1,dw=dw)[0],render('R8',12,5,v,mode,profile=1,dw=dw)[0])
            assert np.array_equal(render('R7',12,3,v,mode,profile=1,dw=0)[0],render('R8',12,3,v,mode,profile=1,dw=0)[0])
    for mode in [0,1,2]:
        for kind in [0,1,2]:
            sc=9 if kind==2 else 2
            assert np.array_equal(render('R7',sc,3,1,mode,kind=kind)[0],render('R8',sc,3,1,mode,profile=4,kind=kind)[0])
            if kind!=2:assert np.array_equal(render('R7',sc,3,.5,mode,profile=1,kind=kind)[0],render('R8',sc,3,.5,mode,profile=1,kind=kind)[0])
    result=dict(base=BASE,source_sha256=hashlib.sha256((ROOT/REL).read_bytes()).hexdigest(),fallback_bus_pairs=fallback_pairs,unity_pairs=len(cases),moving_pairs=len(moving),max_idle_ms=float(max_idle),unity_cases=cases,moving_cases=moving)
    (args.output/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'PASS {len(cases)} unity R7 pairs; {len(moving)} moving fader pairs; core traces unchanged; zero VOL/DW; 1/7/64; idle max {max_idle:.3f} ms')
