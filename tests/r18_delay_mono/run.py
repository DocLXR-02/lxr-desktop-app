#!/usr/bin/env python3
"""Compare exact R18 Delay production bodies against the mono-access-only candidate."""
from pathlib import Path
import hashlib, json, os, subprocess, sys, textwrap
ROOT = Path(__file__).resolve().parents[2]
BASE = "41b364067bce0d8cde624c184933d9cb88e2725b"
SOURCE = "Core/Src/Soundengine/DSPAudio/FX/Delay.c"
EXPECTED_SOURCE_BLOB = "64d2f4721347c53208d80257ca607b079d987ff5"
EXPECTED_FIXED_BLOB = "1a4bbac4021efe9e0c0d9cfa9bee317e5a51e6e6"
def run(cmd, **kwargs):
    return subprocess.run(cmd, capture_output=True, **kwargs)
def blob(raw):
    return hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()
def extract(s, name):
    import re
    match = re.search(r"(?m)^[ \t]*(?:void|float)\s+" + name + r"\([^;]*?\)\s*\{", s)
    if not match: raise ValueError("production function missing: " + name)
    p, depth = match.end(), 1
    while depth:
        depth += (s[p] == "{") - (s[p] == "}")
        p += 1
    return s[match.start():p]
def source_module(src):
    names = ("interpolate_s16", "Delay_calcBufferPingPong", "Delay_calcBuffer",
             "Delay_processPingPong", "Delay_process")
    prefix = r"""
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#define FX_BUFFER_SIZE 0x7fff
#define __fp16 _Float16
static __fp16 FX_BUFFER[FX_BUFFER_SIZE+1];
static int32_t sat(int64_t a) {return a>32767?32767:a < -32768?-32768:(int32_t)a;}
static uint32_t qadd(uint32_t a, uint32_t b) {
 int32_t lo=sat((int16_t)a+(int16_t)b), hi=sat((int16_t)(a>>16)+(int16_t)(b>>16));
 return ((uint32_t)(uint16_t)hi<<16)|(uint16_t)lo;
}
#define __SSAT(a,bits) sat((int64_t)(a))
#define __QADD16(a,b) qadd((uint32_t)(a),(uint32_t)(b))
#define __PKHBT(a,b,shift) ((int32_t)((uint16_t)(a)|((uint32_t)(uint16_t)(b)<<16)))
typedef struct {float r,b;} OnePoleFilter;
static float calcOnePole(OnePoleFilter* f, float x) {f->b+=f->r*(x-f->b);return f->b;}
typedef struct {
 uint32_t oldRead,newRead,write;
 float fracReadNew,fracReadOld,interpValue,interpStep;
 int16_t tmpBuffer[2],tmpBuffer2[2];
 float feedback,range,delayTime,pendingDelayTime;uint8_t timePending;
 OnePoleFilter tone,tone2;uint8_t delayType;
} Delay;
static void Delay_setDelayTime(Delay* d,float t) {(void)d;(void)t;}
"""
    main = r"""
int main(int argc, char **argv) {
 if(argc!=7) return 90;
 unsigned mode=(unsigned)atoi(argv[1]), channels=(unsigned)atoi(argv[2]);
 unsigned destination=(unsigned)atoi(argv[3]), n=(unsigned)atoi(argv[4]);
 unsigned pending=(unsigned)atoi(argv[5]);float dw=(float)atof(argv[6]);
 assert(mode<2 && (channels==1 || channels==2) && destination<2 && n>=1 && n<=64);
 Delay d={0};
 d.feedback=0.37f;d.write=100;d.oldRead=50;d.newRead=80;
 d.fracReadOld=0.18f;d.fracReadNew=0.79f;
 d.interpValue=pending?0.8f:0.f;d.interpStep=0.004f;
 d.tone.r=0.51f;d.tone2.r=0.61f;
 for(unsigned i=0;i<FX_BUFFER_SIZE+1;i++)
   FX_BUFFER[i]=(_Float16)(((int)((i*11u)%500)-250)/32767.f);
 _Alignas(4) int16_t input[128]={0},output[128]={0};
 for(unsigned i=0;i<n;i++) {
   input[2*i]=(int16_t)((int)(i*1371u%52000)-26000);
   input[2*i+1]=(int16_t)((int)(i*1931u%52000)-26000);
   int16_t init=(int16_t)((int)(i*3137u%52000)-26000);
   output[2*i]=init;output[2*i+1]=init;
 }
 const int16_t *before=output;
 int16_t *dest=output+(channels==2?0:destination);
 if(mode) Delay_processPingPong(&d,input,dest,channels==2?output+1:NULL,(uint16_t)n,(uint8_t)channels,dw);
 else Delay_process(&d,input,dest,channels==2?output+1:NULL,(uint16_t)n,(uint8_t)channels,dw);
 if(channels==1) {
   for(unsigned i=0;i<n;i++) {
     int16_t init=(int16_t)((int)(i*3137u%52000)-26000);
     assert(before[2*i+1-destination]==init);
     fwrite(&output[2*i+destination],sizeof(int16_t),1,stdout);
   }
 } else {
   fwrite(output,sizeof(int16_t),2*n,stdout);
 }
 return 0;
}
"""
    return prefix + "\n".join(extract(src, name) for name in names) + main

def main():
    out = Path(sys.argv[1] if len(sys.argv)>1 else "build/r18-delay-mono-evidence")
    out.mkdir(parents=True, exist_ok=True)
    original = subprocess.check_output(["git","show",BASE+":"+SOURCE], cwd=ROOT)
    modified = (ROOT/SOURCE).read_bytes()
    assert blob(original)==EXPECTED_SOURCE_BLOB
    assert blob(modified)==EXPECTED_FIXED_BLOB
    old=original.decode("utf-8")
    new=modified.decode("utf-8")
    # Do not accept any unexpected DSP, feedback, routing, or whitespace edits.
    expected=old.replace("int32_t* stereoOut = (int32_t*)outL;",
        "int32_t* stereoOut = channels == 2 ? (int32_t*)outL : NULL;\r\n\t/* Mono destinations may be the unaligned right halfword of a stereo frame. */\r\n\tint16_t* monoOut = outL;")
    assert expected!=old
    expected=expected.replace("*(uint16_t*)stereoOut = __QADD16(*stereoOut, x);",
        "*monoOut = (int16_t)__QADD16(*monoOut, x);")
    import re
    expected,count=re.subn(r"(\*monoOut = \(int16_t\)__QADD16\(\*monoOut, x\);\r\n)([ \t]*)stereoOut\+\+;",
        r"\1\2monoOut += 2;", expected)
    assert count==2 and expected==new, "Unexpected changes to the R18 Delay source"
    for label,txt in [("r18",old),("candidate",new)]:
        file=out/(label+".c")
        file.write_text(source_module(txt),encoding="utf-8")
        args=["cc","-std=gnu11","-O1","-g","-fno-strict-aliasing","-fno-omit-frame-pointer",
            "-fsanitize=address,undefined,alignment,bounds,float-cast-overflow",
            "-fno-sanitize-recover=all",str(file),"-lm","-o",str(out/label)]
        p=run(args,text=True)
        (out/(label+".compile.log")).write_text(p.stdout+p.stderr,encoding="utf-8")
        assert p.returncode==0, (label,p.stderr)
    env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0:halt_on_error=1",
         "UBSAN_OPTIONS":"halt_on_error=1:print_stacktrace=1"}
    baseline_expected_failure=run([str(out/"r18"),"0","1","1","64","0","0.5"],env=env)
    (out/"r18-mono-right.expected-failure.log").write_bytes(
        baseline_expected_failure.stdout+baseline_expected_failure.stderr)
    assert baseline_expected_failure.returncode!=0 and (
        b"runtime error: load of misaligned address" in baseline_expected_failure.stderr), \
        "Baseline R18 mono-right failure was not reproduced"
    cases=0
    for mode in range(2):
        for size in (1,7,64):
            for pending in (0,1):
                for dw in (0,0.25,0.5,1):
                    prefix=[str(mode),str(size),str(pending),str(dw)]
                    def sample(label,channels,dest):
                        proc=run([str(out/label),str(mode),str(channels),str(dest),
                                  str(size),str(pending),str(dw)],env=env)
                        if proc.returncode:
                            raise AssertionError((label,prefix,channels,dest,
                                proc.stderr.decode(errors="replace")[-2000:]))
                        return proc.stdout
                    # Full-frame stereo and valid mono-left remain bit-identical.
                    assert sample("r18",2,0)==sample("candidate",2,0)
                    left=sample("candidate",1,0)
                    assert sample("r18",1,0)==left
                    # The formerly invalid mono-right path has the same
                    # processed channel output as valid mono-left, using 16-bit reads.
                    assert sample("candidate",1,1)==left
                    cases+=3
    result={"base":BASE,"candidate_blob":EXPECTED_FIXED_BLOB,
        "executed_comparisons":cases,"tested_modes":["normal","ping_pong"],
        "sizes":[1,7,64],"dry_wet":[0,0.25,0.5,1],
        "transition_states":["steady","interpolating"],
        "baseline_mono_right_expected_ubsan_failure":True,
        "candidate_mono_left_right_sanitizer_pass":True,
        "unchanged_stereo_and_mono_left_bytes":True,
        "limit":"Host extracted DSP functions and modeled ARM saturation; no ARM execution, DMA, WCET or device listening."}
    (out/"result.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(f"PASS: original R18 mono-right alignment failure reproduced; candidate normal/ping-pong mono-left/right pass; {cases} byte-level comparisons")
    print("PASS: original R18 stereo and mono-left output remain bit-identical in tested cases")
if __name__=="__main__":
    main()
