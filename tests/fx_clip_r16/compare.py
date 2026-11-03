#!/usr/bin/env python3
"""R16 check: render the production CLP DSP against exact R15 with only HP1 retuned.

The R15 input trim, no-parallel-bass path, idle handling and feedback DSP are
otherwise unchanged. Compare full output waveforms and core-state traces.
"""
from pathlib import Path
import hashlib, os, subprocess, tempfile, zipfile
ROOT=Path(__file__).resolve().parents[2]
BASE="03bcb4f4e21d368d7a5eda2691ad619bb595d994"
COEFF="0.13267953435832913"
R15_ALPHA="0.015536190543605555"
DIST="Core/Src/Soundengine/DSPAudio/FX/Distortion.c"
HDR="Core/Inc/Soundengine/DSPAudio/FX/Distortion.h"

def oldfile(path):
    archive=os.environ.get("R16_BASE_SOURCE_ARCHIVE")
    if archive:
        with zipfile.ZipFile(archive) as z:
            return z.read(path)
    return subprocess.check_output(["git","show","{}:{}".format(BASE,path)],cwd=ROOT)

old=oldfile(DIST).decode()
assert old.count("#define CLIP_HP1_ALPHA "+R15_ALPHA)==1
ref=old.replace("#define CLIP_HP1_ALPHA "+R15_ALPHA,
                "#define CLIP_HP1_ALPHA "+COEFF)
actual=(ROOT/DIST).read_text()
actual_hdr=(ROOT/HDR).read_text()
assert actual.count("CLIP_HP1_ALPHA * (signal")==3
assert "#define CLIP_HP1_ALPHA "+COEFF in actual
for forbidden in ("Distortion_clipBass","clipBassLp_state","CLIP_BASS_WEIGHT","CLIP_BASS_LP_ALPHA"):
    assert forbidden not in actual and forbidden not in actual_hdr,forbidden
assert "CLIP_INPUT_TRIM 0.22" in actual
with tempfile.TemporaryDirectory() as directory:
    p=Path(directory)
    old_src=p/'r15_hp1000.c'
    old_src.write_text(ref)
    inc=p/'R15_header/Soundengine/DSPAudio/FX/Distortion.h'
    inc.parent.mkdir(parents=True)
    inc.write_bytes(oldfile(HDR))
    exe_ref=p/'r15_ref'
    exe_r16=p/'r16_actual'
    test=str(ROOT/'tests/fx_clip_r7/render.c')
    for exe,src,inc_dir in ((exe_ref,old_src,str(p/'R15_header')),
                            (exe_r16,ROOT/DIST,str(ROOT/'Core/Inc'))):
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
                    for exe,name in ((exe_ref,'reference'),(exe_r16,'actual')):
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
    print("PASS R16 HP1000/no bass: {} sample-exact R15-retuned DSP renders + core-state traces".format(cases))
