"""Native component experiments only; export the pin outside the source checkout."""
from pathlib import Path
import argparse,csv,hashlib,io,json,os,re,subprocess,tarfile,tempfile,wave
import numpy as np
PIN='8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7';FS=44140;HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--repo',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.repo=a.repo.resolve();a.output=a.output.resolve()
assert not a.output.is_relative_to(a.repo);a.output.mkdir(parents=True,exist_ok=True)
def git(*args):return subprocess.check_output(['git','-C',str(a.repo),*args])
def function(s,name):
    m=re.search(r'^(?:IN_ITCM_RAM )?(?:(?:static|__inline|inline) )*(?:const float \*|float |void )'+name+r'\([^;]*?\)\s*\{',s,re.M);assert m,name
    n=m.end();depth=1
    while depth:depth+=(s[n]=='{')-(s[n]=='}');n+=1
    return s[m.start():n]
def wav(name,y):
    assert np.max(np.abs(y))<=32768
    with wave.open(str(a.output/(name+'.wav')),'wb') as w:w.setparams((2,2,FS,len(y),'NONE','not compressed'));w.writeframes(y.astype('<i2').tobytes())
status=git('status','--porcelain=v1');env=os.environ.copy();env.setdefault('ASAN_OPTIONS','detect_leaks=0');rows=[];pairs=[]
with tempfile.TemporaryDirectory(prefix='clp-mix-vol-') as tmp:
    d=Path(tmp);R=d/'r10';R.mkdir()
    with tarfile.open(fileobj=io.BytesIO(git('archive','--format=tar',PIN))) as archive:archive.extractall(R,filter='data')
    rel='Core/Src/Soundengine/';D=R/rel;tails=[]
    voices=[('DrumVoice','calcDrumVoiceSyncBlock','//MIDI velocity - float processing','static void drum_tail(const uint8_t voiceNr,int16_t *buf,const uint8_t size)'),('Snare','Snare_calcSyncBlock','uint8_t j;','static void snare_tail(int16_t *buf,int16_t *oscBuf,int16_t *transBuf,const uint8_t size)'),('CymbalVoice','Cymbal_calcSyncBlock','uint8_t j;','static void cymbal_tail(int16_t *buf,int16_t *mod,const uint8_t size)'),('HiHat','HiHat_calcSyncBlock','uint8_t j;','static void hat_tail(int16_t *buf,int16_t *mod1,const uint8_t size)')]
    for file,name,marker,wrapper in voices:
        body=function((D/('DSPAudio/'+file+'.c')).read_text(),name);tail=body[body.index(marker):]
        tails.append(wrapper+'{\n'+tail+'\n')
    tails.insert(0,function((R/'Core/Inc/Soundengine/DSPAudio/BufferTools.h').read_text(),'bufferTool_addGain'))
    tails.append(function((D/'DSPAudio/mixer.c').read_text(),'mixer_addDataToOutput'))
    (d/'tails.inc').write_text('\n'.join(tails))
    config=(R/'Core/Inc/Soundengine/config.h').read_text();assert '#define SMOOTH_MIXER_VOL_PAN_DRIVE 1' in config and '#define USE_FILTER_DRIVE 0' in config
    flags=[os.environ.get('CC','cc'),'-std=c11','-D_DEFAULT_SOURCE','-DSMOOTH_MIXER_VOL_PAN_DRIVE=1','-DUSE_FILTER_DRIVE=0','-O2','-fno-strict-aliasing','-ffunction-sections','-fdata-sections','-fsanitize=address,undefined,bounds,float-cast-overflow','-fno-sanitize-recover=all','-I'+str(d),'-I'+str(R/'tests/fx_clip/include'),'-I'+str(R/'Core/Inc')]
    sources=[D/'DSPAudio/distortion.c',D/'DSPAudio/FX/Distortion.c',D/'DSPAudio/1PoleLp.c',D/'FxBus.c']
    for name,file in [('solo','experiment.c'),('mixed','mixed.c')]:subprocess.run([*flags,str(HERE/file),*map(str,sources),'-Wl,--gc-sections','-lm','-o',str(d/name)],check=True)
    reference={}
    matrix=[(v,m,drive,0) for v in range(6) for drive in [0,100] for m in [0,1,5,10,20,127]]
    matrix += [(v,m,100,1) for v in [0,5] for m in [10,127]]+[(v,10,100,2) for v in [0,5]]
    for v,m,drive,profile in matrix:
        data=d/'frames.raw';state=d/'state.raw'
        text=subprocess.check_output([str(d/'solo'),str(v),str(m),str(drive),str(profile),str(data),str(state)],text=True,env=env)
        clips,state_bytes=map(int,text.split());x=np.fromfile(data,np.float32).reshape(-1,15);states=np.fromfile(state,np.uint8).reshape(-1,2,state_bytes)
        assert np.isfinite(x).all() and not np.any(x[-int(.1*FS):,4:10]),'invalid/persistent output'
        if profile!=2 and m>0:
            key=(v,drive,profile)
            if key in reference:
                z,st=reference[key];assert np.array_equal(x[:,2:4],z[:,2:4]) and np.array_equal(states[:,1],st),'full-MIX reference depends on selected MIX'
            else:reference[key]=(x.copy(),states[:,1].copy())
        if m==0 and profile==0:assert not np.any(x[:,:10]),'hidden zero-volume excitation/output'
        w=x[int(.02*FS):int(.30*FS)];mval=m/127
        error=None if m==0 or profile!=0 else float(np.linalg.norm(w[:,0:2]/mval-w[:,2:4])/np.linalg.norm(w[:,2:4]))
        rms=lambda y:float(np.sqrt(np.mean((y/32768.)**2)))
        rows.append(dict(voice=v+1,mix=m,drive=drive,profile=profile,input_rms=rms(w[:,0:2]),full_input_rms=rms(w[:,2:4]),R10_rms=rms(w[:,4:6]),input_only_rms=rms(w[:,6:8]),post_mix_rms=rms(w[:,8:10]),baseline_core_block_rms=float(np.sqrt(np.mean(w[:,12]**2))),full_core_block_rms=float(np.sqrt(np.mean(w[:,13]**2))),inverse_relative_error=error,max_R10_step=int(np.abs(np.diff(x[:,4:6],axis=0)).max()),max_post_step=int(np.abs(np.diff(x[:,8:10],axis=0)).max()),mixer_saturations=clips))
        if v==5 and drive==100 and ((profile==0 and m in [10,127]) or profile in [1,2]):
            for name,y in [('R10',x[:,4:6]),('post',x[:,8:10])]:wav(f'voice6-M{m}-profile{profile}-{name}',y)
    dw_probes=[]
    for dw in [0,.4]:
        data=d/'dw.raw';state=d/'dw.state'
        subprocess.check_output([str(d/'solo'),'5','10','100','0',str(data),str(state),str(dw)],env=env)
        x=np.fromfile(data,np.float32).reshape(-1,15);st=np.fromfile(state,np.uint8).reshape(-1,2,state_bytes)
        assert np.array_equal(st[:,1],reference[(5,100,0)][1])
        w=x[int(.02*FS):int(.30*FS)]
        dw_probes.append(dict(dry_wet=dw,R10_rms=float(np.sqrt(np.mean((w[:,4:6]/32768.)**2))),post_mix_rms=float(np.sqrt(np.mean((w[:,8:10]/32768.)**2)))))
    for low,high in [(10,127),(5,20),(0,127),(0,0)]:
        data=d/'pair.raw';clips=list(map(int,subprocess.check_output([str(d/'mixed'),str(low),str(high),'100',str(data)],text=True,env=env).split()));x=np.fromfile(data,np.float32).reshape(-1,16)
        assert np.isfinite(x).all() and not np.any(x[-int(.1*FS):,6:]),'mixed invalid/persistent output'
        w=x[int(.02*FS):int(.30*FS)];rms=lambda y:float(np.sqrt(np.mean((y/32768.)**2)))
        if low==high==0:assert not np.any(x)
        pairs.append(dict(low_mix=low,high_mix=high,R10_rms=rms(w[:,6:8]),input_only_rms=rms(w[:,8:10]),common_factored_rms=rms(w[:,10:12]),two_core_rms=rms(w[:,12:14]),common_post_full_rms=rms(w[:,14:16]),common_vs_R10_error=rms(w[:,10:12]-w[:,6:8]),two_core_vs_shared_full_error=rms(w[:,12:14]-w[:,14:16]),mixer_saturations=clips))
        if low==10 and high==127:
            for name,y in [('R10',x[:,6:8]),('common',x[:,10:12]),('two-core',x[:,12:14])]:wav('pair-'+name,y)
    hashes={str(path.relative_to(R)):hashlib.sha256(path.read_bytes()).hexdigest() for path in sources+[D/('DSPAudio/'+v[0]+'.c') for v in voices]+[D/'DSPAudio/mixer.c']}
assert git('status','--porcelain=v1')==status,'source checkout changed'
with (a.output/'solo.csv').open('w') as f:w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
result=dict(source_commit=PIN,compiler=subprocess.check_output([os.environ.get('CC','cc'),'--version'],text=True).splitlines()[0],numpy=np.__version__,solo_runs=len(rows),pair_runs=len(pairs),dry_wet_probes=dw_probes,state_bytes=state_bytes,pairs=pairs,source_sha256=hashes,scope='Actual extracted voice tails and production voice/CLP distortion; synthetic post-filter/envelope buffers. Not full voice synthesis, firmware or hardware.')
(a.output/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
