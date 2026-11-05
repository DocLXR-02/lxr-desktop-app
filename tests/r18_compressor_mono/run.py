#!/usr/bin/env python3
"""R18 compressor mono saturation: source-extracted original/candidate host comparisons.

Host-only model; never claim device audio, ARM timing or full DSP equivalence.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
PIN = "41b364067bce0d8cde624c184933d9cb88e2725b"
PATH = "Core/Src/Soundengine/DSPAudio/FX/Compressor.c"
ORIGINAL_BLOB = "9e5f34bcecae43a9696cca30658c34a668c8f679"
OLD = b"left[i]  = fxBuffer[idx1] * gr * c->markupGain;//gain;"
NEW = b"left[i]  = __SSAT(fxBuffer[idx1] * gr * c->markupGain, 16);//gain;"


def git_blob(raw):
    return hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest()


def function(source, name):
    p = re.compile(r"(?m)^[ \t]*(?:static\s+)?(?:double|void)\s+" +
                   re.escape(name) + r"\s*\([^;]*?\)\s*\{")
    m = p.search(source)
    assert m, "Cannot locate original production function: " + name
    pos = m.end()
    depth = 1
    while depth:
        depth += (source[pos] == "{") - (source[pos] == "}")
        pos += 1
    return source[m.start():pos]


PRELUDE = r"""
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#define RATIO_1 0
#define RATIO_2 1
#define RATIO_3 2
#define RATIO_4 3
#define RATIO_6 4
#define RATIO_8 5
typedef struct CompressorStruct {
    float tresh, attack, release, ratio, markupGain;
    double env;
} Compressor;
static const double DC_OFFSET = 1.0E-25;
static int32_t saturation16(int32_t a) {return a>32767?32767:a<(-32768)?-32768:a;}
#define __SSAT(v,bits) saturation16((int32_t)(v))
static uint32_t qadd16(uint32_t a,uint32_t b) {
    int32_t lo=saturation16((int16_t)a+(int16_t)b);
    int32_t hi=saturation16((int16_t)(a>>16)+(int16_t)(b>>16));
    return ((uint32_t)(uint16_t)hi<<16)|(uint16_t)lo;
}
#define __QADD16(a,b) qadd16((uint32_t)(a),(uint32_t)(b))
#define __PKHBT(a,b,shift) ((int32_t)(((uint32_t)(uint16_t)(a))|((uint32_t)(uint16_t)(b)<<16)))
"""

MAIN = r"""
int main(int argc, char **argv) {
    if(argc!=2)return 10;
    if(argv[1][0]=='n') {
        for(int ch=1;ch<=2;ch++)
        for(int ratio=0;ratio<6;ratio++)
        for(int gain=0;gain<3;gain++)
        for(int wet=0;wet<4;wet++) {
            Compressor c={0}; Compressor_init(&c); Compressor_setRatio(&c,(uint8_t)ratio);
            c.tresh=-12.f;
            c.attack=0.8f; c.release=0.99f;
            c.markupGain=(float)(gain+1);
            int16_t in[128];
            _Alignas(4) int16_t out[128];
            for(int i=0;i<64;i++){
                in[2*i]=(int16_t)(((i*131+ratio*23)%5001)-2500);
                in[2*i+1]=(int16_t)(((i*73+gain*29)%5001)-2500);
                out[2*i]=(int16_t)(((i*7)%121)-60);
                out[2*i+1]=(int16_t)(((i*11)%121)-60);
            }
            Compressor_process(&c,in,out,out+1,64,(uint8_t)ch,wet*0.25f);
            printf("%d/%d/%d/%d:",ch,ratio,gain,wet);
            for(int i=0;i<128;i++)printf("%d,",out[i]);
            puts("");
        }
        return 0;
    }
    int positive = (argv[1][0]=='p' || argv[1][0]=='s');
    int stereo = (argv[1][0]=='s' || argv[1][0]=='t');
    int16_t in[2] = {positive?32767:-32768,0};
    _Alignas(4) int16_t out[2] = {0,0};
    Compressor c={0};Compressor_init(&c);Compressor_setRatio(&c,0);
    c.tresh=0; c.markupGain=3.0f; c.attack=0.8f; c.release=0.99f;
    Compressor_process(&c,in,out,out+1,1,(uint8_t)(stereo?2:1),1.f);
    assert(out[0]==(positive?32767:-32768));
    printf("PASS %s saturation output %d\n",stereo?"stereo":"mono",(int)out[0]);
    return 0;
}
"""


def main():
    out = Path(sys.argv[1] if len(sys.argv)>1 else "build/r18-compressor-mono-host")
    out.mkdir(parents=True,exist_ok=True)
    original = subprocess.check_output(["git","show",PIN+":"+PATH],cwd=ROOT)
    candidate = (ROOT/PATH).read_bytes()
    assert git_blob(original)==ORIGINAL_BLOB, "Original R18 compressor blob mismatch"
    assert original.count(OLD)==1, "Expected exactly one original conversion"
    assert candidate==original.replace(OLD,NEW,1), "Source diff includes unapproved changes"
    assert b"\r\n" in candidate, "Original production CRLF must be retained"
    src_original = original.decode("utf-8")
    src_candidate = candidate.decode("utf-8")
    funcs = ("lin2dB","dB2lin","Compressor_init","Compressor_setRatio","Compressor_process")
    cflags = ["cc","-std=gnu11","-O1","-g","-fno-strict-aliasing",
              "-fsanitize=undefined,alignment,bounds,float-cast-overflow",
              "-fno-sanitize-recover=all"]
    for name,src in (("original",src_original),("candidate",src_candidate)):
        cpath=out/(name+".c")
        cpath.write_text(PRELUDE+"\n".join(function(src,f) for f in funcs)+MAIN,encoding="utf-8")
        proc=subprocess.run(cflags+[str(cpath),"-lm","-o",str(out/name)],capture_output=True,text=True)
        (out/(name+".build.log")).write_text(proc.stdout+proc.stderr,encoding="utf-8")
        assert proc.returncode==0, name+" host compilation failed: "+proc.stderr[:1500]

    cases=[
        ("original","normal",True),("candidate","normal",True),
        ("original","positive",False),("original","negative",False),
        ("candidate","positive",True),("candidate","negative",True),
        ("original","stereo-positive",True),("original","stereo-negative",True),
        ("candidate","stereo-positive",True),("candidate","stereo-negative",True),
    ]
    results=[]
    for implementation,mode,should_pass in cases:
        actual_mode={"normal":"normal","positive":"positive","negative":"negative",
                     "stereo-positive":"stereo","stereo-negative":"two"}[mode]
        proc=subprocess.run([str(out/implementation),actual_mode],capture_output=True,text=True,
                            env={**os.environ,"UBSAN_OPTIONS":"halt_on_error=1:print_stacktrace=1"})
        name=implementation+"-"+mode
        (out/(name+".run.log")).write_text(proc.stdout+proc.stderr,encoding="utf-8")
        valid=(proc.returncode==0 if should_pass else
               proc.returncode!=0 and "outside the range of representable values" in proc.stderr)
        if should_pass and mode=="normal":
            (out/(implementation+"-normal-output.txt")).write_text(proc.stdout,encoding="utf-8")
        results.append({"name":name,"expected":"PASS" if should_pass else "EXPECTED_FLOAT_CAST_FAILURE",
                        "exit_code":proc.returncode,"verified":valid,
                        "stderr_excerpt":proc.stderr[-300:]})
        print(("PASS" if valid else "FAIL")+" "+name+" exit="+str(proc.returncode),flush=True)
        if not valid:
            print((proc.stdout+proc.stderr)[-2000:],flush=True)
    a=(out/"original-normal-output.txt").read_bytes()
    b=(out/"candidate-normal-output.txt").read_bytes()
    same=a==b
    print("PASS" if same else "FAIL","normal-domain raw-sample parity across 144 channel/ratio/gain/wet cases")
    results.append({"name":"normal-domain-byte-identical","verified":same,
                    "combination_count":144,"original_sha256":hashlib.sha256(a).hexdigest(),
                    "candidate_sha256":hashlib.sha256(b).hexdigest()})
    result={"original_source_sha":PIN,"original_compressor_blob":ORIGINAL_BLOB,
            "candidate_compressor_blob":git_blob(candidate),
            "case_count":len(cases),"normal_combinations":144,
            "all_expected_outcomes":all(x["verified"] for x in results),
            "limitations":["source-extracted host shims, not Cortex-M7 or hardware",
                           "normal-domain sample parity is bounded to fixtures",
                           "out-of-range saturation changes previously undefined mono output",
                           "stereo and CLP production source unchanged; actual device sound not heard"],
            "tests":results}
    (out/"RESULTS.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    files=sorted(p for p in out.iterdir() if p.is_file() and p.name!="SHA256SUMS.txt")
    (out/"SHA256SUMS.txt").write_text("".join(hashlib.sha256(p.read_bytes()).hexdigest()+
                                            "  "+p.name+"\n" for p in files),encoding="utf-8")
    assert result["all_expected_outcomes"],"Host test failure"
    print("PASS all cases, precise original sanitizer reproductions and 144 exact sample comparisons")


if __name__=="__main__":
    main()
