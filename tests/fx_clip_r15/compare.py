#!/usr/bin/env python3
"""Compare real R15 DSP with exact R14 source, changed only to HP1 110Hz and bass=0.

The reference retains the R7 bass filter computation but mutes its output.
R15 removes that whole DSP path; rendered samples and CLP core-state traces
must be identical, across mono/stereo, control rows, and dry/wet settings.
"""
from pathlib import Path
import hashlib, os, subprocess, tempfile, zipfile
ROOT=Path(__file__).resolve().parents[2]
BASE="00ea59d38de6f006dfdbcd472c2cfe5040a440c3"
COEFF="0.015536190543605555"
R14_ALPHA="0.64116641920131193922"
DIST="Core/Src/Soundengine/DSPAudio/FX/Distortion.c"
HDR="Core/Inc/Soundengine/DSPAudio/FX/Distortion.h"

def oldfile(path):
    archive=os.environ.get("R15_BASE_SOURCE_ARCHIVE")
    if archive:
        with zipfile.ZipFile(archive) as z:
            return z.read(path)
    return subprocess.check_output(["git","show","{}:{}".format(BASE,path)],cwd=ROOT)

old=oldfile(DIST).decode()
assert old.count(R14_ALPHA+" * (signal")==3
assert old.count("#define CLIP_BASS_WEIGHT -0.25")==1
ref=old.replace(R14_ALPHA+" * (signal",COEFF+" * (signal").replace(
    "#define CLIP_BASS_WEIGHT -0.25", "#define CLIP_BASS_WEIGHT 0")
actual=(ROOT/DIST).read_text()
actual_hdr=(ROOT/HDR).read_text()
assert actual.count("CLIP_HP1_ALPHA * (signal")==3
assert "#define CLIP_HP1_ALPHA "+COEFF in actual
for forbidden in ("Distortion_clipBass","clipBassLp_state","CLIP_BASS_WEIGHT","CLIP_BASS_LP_ALPHA"):
    assert forbidden not in actual and forbidden not in actual_hdr,forbidden
assert "CLIP_INPUT_TRIM 0.22" in actual
with tempfile.TemporaryDirectory() as directory:
    p=Path(directory)
    old_src=p/'r14_hp110_bass0.c'
    old_src.write_text(ref)
    inc=p/'R14_header/Soundengine/DSPAudio/FX/Distortion.h'
    inc.parent.mkdir(parents=True)
    inc.write_bytes(oldfile(HDR))
    exe_ref=p/'r14_ref'
    exe_r15=p/'r15_actual'
    test=str(ROOT/'tests/fx_clip_r7/render.c')
    for exe,src,inc_dir in ((exe_ref,old_src,str(p/'R14_header')),
                            (exe_r15,ROOT/DIST,str(ROOT/'Core/Inc'))):
        cmd=[os.environ.get('CC','cc'),'-std=c11','-O2','-fno-strict-aliasing',
             '-I'+str(ROOT/'tests/fx_clip/include'),'-I'+inc_dir,
             '-I'+str(ROOT/'Core/Inc'),test,str(src),'-lm','-o',str(exe)]
        compiler=subprocess.run(cmd,check=False,capture_output=True,text=True)
        if compiler.returncode:
            raise RuntimeError("Compilation failed: "+compiler.stderr)
    cases=0
    for scenario in (0,2,3,5,9):
        for row in (0,3,6):
            for mode in (0,1,2):
                for dw in (0,.5,1.):
                    travel=.85 if scenario!=9 else 1.
                    args=[str(scenario),str(row),str(travel),str(mode),'64','2',str(dw)]
                    outputs=[]
                    for exe,name in ((exe_ref,'reference'),(exe_r15,'actual')):
                        wave=p/(name+'.raw')
                        result=subprocess.run([str(exe),*args,str(wave)],capture_output=True,text=True)
                        if result.returncode:
                            raise RuntimeError("{} failed case {}: {}".format(name,args,result.stderr))
                        outputs.append((wave.read_bytes(),result.stdout.strip()))
                    if outputs[0]!=outputs[1]:
                        raise AssertionError("DSP mismatch case {}; hashes {} vs {}; traces {} vs {}".format(
                            args,hashlib.sha256(outputs[0][0]).hexdigest(),
                            hashlib.sha256(outputs[1][0]).hexdigest(),
                            outputs[0][1],outputs[1][1]))
                    cases+=1
    print("PASS R15 HP110/no bass: {} sample-exact R14-matched DSP renders + core-state traces".format(cases))
