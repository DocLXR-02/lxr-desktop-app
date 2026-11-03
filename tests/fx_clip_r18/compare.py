#!/usr/bin/env python3
"""R18 unchanged-core regression against pinned R14 with R7 output bass muted.

The R14 source is fetched through git show, never from a moving branch.
The R18 output is intentionally DIFFERENT; this compares all R14 internal
return snapshots, input consumption, and core-state headroom.
"""
import os, subprocess, tempfile, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
BASE='00ea59d38de6f006dfdbcd472c2cfe5040a440c3'
DIST='Core/Src/Soundengine/DSPAudio/FX/Distortion.c'
HDR='Core/Inc/Soundengine/DSPAudio/FX/Distortion.h'

def original(path):
    archive=os.environ.get('R18_BASE_SOURCE_ARCHIVE')
    if archive:
        with zipfile.ZipFile(archive) as z:
            return z.read(path)
    return subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT)

old=original(DIST).decode()
assert old.count('#define CLIP_BASS_WEIGHT -0.25')==1
ref=old.replace('#define CLIP_BASS_WEIGHT -0.25','#define CLIP_BASS_WEIGHT 0')
prod=(ROOT/DIST).read_text()
newhead=(ROOT/HDR).read_text()
assert '0.64116641920131193922 * (signal' in prod
assert prod.count('0.64116641920131193922 * (signal')==3
assert 'CLP_V2_DIFF_ALPHA 0.028067915544701783' in prod
assert prod.count('Distortion_clipV2Mix(dist,')==3
for name in ('Distortion_clipBass','CLIP_BASS_WEIGHT','clipBassLp_state'):
    assert name not in prod and name not in newhead
with tempfile.TemporaryDirectory() as d:
    temp=Path(d)
    oldsrc=temp/'R14_no_bass.c';oldsrc.write_text(ref)
    inc=temp/'original_header/Soundengine/DSPAudio/FX/Distortion.h'
    inc.parent.mkdir(parents=True);inc.write_bytes(original(HDR))
    outputs=[]
    for exe,src,incdir in ((temp/'r14',oldsrc,temp/'original_header'),
                           (temp/'r18',ROOT/DIST,ROOT/'Core/Inc')):
        flags=['-std=c11','-O2','-fno-strict-aliasing',
               '-I'+str(ROOT/'tests/fx_clip/include'),'-I'+str(incdir),
               '-I'+str(ROOT),'-I'+str(ROOT/'Core/Inc')]
        command=[os.getenv('CC','cc'),*flags,str(ROOT/'tests/fx_clip_r7/render.c'),str(src),'-lm','-o',str(exe)]
        result=subprocess.run(command,capture_output=True,text=True)
        assert result.returncode==0,(command,result.stderr)
        outputs.append(exe)
    cases=0
    for scenario in (0,2,3,5,9):
      for row in (0,3,6):
        for mode in (0,1,2):
          for dw in (0,.5,1.):
            args=[str(scenario),str(row),str(.85 if scenario!=9 else 1.),str(mode),'64','2',str(dw)]
            stdout=[]
            for label,exe in zip(('R14 core','R18 core'),outputs):
                raw=temp/(label.replace(' ','_')+'.raw')
                run=subprocess.run([str(exe),*args,str(raw)],capture_output=True,text=True)
                assert run.returncode==0,(label,args,run.stderr)
                stdout.append(run.stdout.strip())
            assert stdout[0]==stdout[1],(args,stdout)
            cases+=1
    print('PASS R18 R14-HP7200 core-isolation: 135 identical return-state snapshots across 5 input histories, 3 control rows, mono/stereo and D/W grid')
