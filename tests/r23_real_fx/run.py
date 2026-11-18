#!/usr/bin/env python3
"""R23 actual four-DSP production-source integration on host.

Exact immutable production function bodies, native ARM SIMD semantic shims,
and synthetic input voices. Does NOT emulate Cortex-M7 timing or DMA.
"""
import hashlib, re, subprocess, sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
PIN="238389475359f11bf317b446e668e0c82333c231"
OUT=ROOT/"build/r23-real-dsp-integration"
OUT.mkdir(parents=True,exist_ok=True)
def source(path):
    raw=subprocess.check_output(["git","show",PIN+":"+path],cwd=ROOT)
    return raw.decode("utf-8").replace("\r\n","\n")
def find(name,s):
    rx=r"(?m)^[ \t]*(?:(?:static|__inline|inline|IN_ITCM_RAM)\s+)*(?:void|int|float|double|int16_t|uint32_t|uint8_t|int32_t)\s+"+re.escape(name)+r"\s*\([^;]*?\)\s*(?://[^\n]*\n\s*)?\{"
    m=re.search(rx,s,flags=re.M|re.S)
    if not m: raise AssertionError("Source function absent: "+name)
    k=s.index("{",m.start());level=0
    for pos in range(k,len(s)):
        if s[pos]=="{":level+=1
        elif s[pos]=="}":
            level-=1
            if level==0:return s[m.start():pos+1]
    raise AssertionError("Unbalanced source function: "+name)
def brace_segment(s,token,tail=";"):
    p=s.index(token);i=s.index("{",p);level=0
    for j in range(i,len(s)):
        if s[j]=="{":level+=1
        elif s[j]=="}":
            level-=1
            if level==0:
                k=s.index(tail,j)
                return s[p:k+len(tail)]
    raise AssertionError("Unbalanced source table/type: "+token)
def strip_includes(s):
    return "\n".join(x for x in s.split("\n") if not x.lstrip().startswith("#include "))
C="Core/Src/Soundengine/DSPAudio/"
H="Core/Inc/Soundengine/DSPAudio/"
D={name:source(C+"FX/"+name+".c") for name in ("Distortion","Ringmod","Compressor","Delay")}
headers={name:source(H+"FX/"+name+".h") for name in D}
headers["Oscillator"]=source(H+"Oscillator.h")
headers["OnePole"]=source(H+"1PoleLp.h")
headers["FxBus"]=source("Core/Inc/Soundengine/FxBus.h")
headers["Mixer"]=source("Core/Inc/Soundengine/DSPAudio/mixer.h")
OS=source(C+"Oscillator.c")
WAV=source(C+"wavetable.c")
LP=source(C+"1PoleLp.c")
MIX=source(C+"mixer.c")
BUS=source("Core/Src/Soundengine/FxBus.c")
for name,body in [(path,source(path)) for path in (C+"FX/Distortion.c", C+"FX/Ringmod.c", C+"FX/Compressor.c", C+"FX/Delay.c", C+"Oscillator.c", C+"wavetable.c", C+"1PoleLp.c", C+"mixer.c", "Core/Src/Soundengine/FxBus.c")]:
    (OUT/("source-"+name.split("/")[-1]+".sha256")).write_text(hashlib.sha256(body.encode()).hexdigest()+" "+name+"\n")

PRE= r'''
/* AUTO-GENERATED FROM IMMUTABLE R23 SOURCES; not firmware code. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#define REAL_FS 44140.f
#define TABLESIZE 4096
#define INTERPOLATE_OSC 1
#define INTERPOLATE_FM_OSC 1
#define IN_ITCM_RAM
#define INDTCM
#define INDTCMZ
#define __fp16 _Float16
#define FX_BUFFER_SIZE 0x7FFF
#define USE_REVERB 0
#define MAX_ALLPASS_SIZE 0x800
static int32_t host_sat16(int64_t x) {return (int32_t)(x>32767?32767:x< -32768?-32768:x);}
#define __SSAT(v,bits) host_sat16((int64_t)(v))
static int32_t host_qadd16(int32_t a,int32_t b) {
 int32_t lo=host_sat16((int16_t)(uint16_t)a + (int16_t)(uint16_t)b);
 int32_t hi=host_sat16((int16_t)(uint16_t)((uint32_t)a>>16) + (int16_t)(uint16_t)((uint32_t)b>>16));
 return (int32_t)((uint32_t)(uint16_t)lo | ((uint32_t)(uint16_t)hi<<16));
}
static int32_t host_qsub16(int32_t a,int32_t b) {
 int32_t lo=host_sat16((int16_t)(uint16_t)a - (int16_t)(uint16_t)b);
 int32_t hi=host_sat16((int16_t)(uint16_t)((uint32_t)a>>16) - (int16_t)(uint16_t)((uint32_t)b>>16));
 return (int32_t)((uint32_t)(uint16_t)lo | ((uint32_t)(uint16_t)hi<<16));
}
#define __QADD16(a,b) host_qadd16((a),(b))
#define __QSUB16(a,b) host_qsub16((a),(b))
#define __PKHBT(a,b,shift) ((int32_t)((uint32_t)(uint16_t)(a)|((uint32_t)(uint16_t)(b)<<(shift))))
#define __CLZ(a) ((a)==0?32:__builtin_clz((unsigned)(a)))
static uint32_t host_rng=0xdeadbeef;
static int16_t GetRngValue(void) {host_rng^=host_rng<<13;host_rng^=host_rng>>17;host_rng^=host_rng<<5;return (int16_t)host_rng;}
static uint8_t r1_Available,l1_Available,r2_Available,l2_Available;
static void bufferTool_clearBuffer(int16_t*b,unsigned samples){memset(b,0,samples*sizeof(*b));}
'''
# Existing routing definitions are imported unchanged from the pinned header.
PRE+='#include "Soundengine/SharedFxPool.h"\n'
types=[
  brace_segment(headers["OnePole"],"typedef struct OnePoleStruct"),
  brace_segment(headers["Oscillator"],"enum OscWaveforms"),
  brace_segment(headers["Oscillator"],"typedef struct OscStruct"),
  brace_segment(headers["Distortion"],"enum DistortionTypesEnum"),
  brace_segment(headers["Distortion"],"typedef struct DistortionStruct"),
  brace_segment(headers["Ringmod"],"typedef struct RingmodStruct"),
  brace_segment(headers["Compressor"],"typedef struct CompressorStruct"),
  brace_segment(headers["Delay"],"enum DelayTypesEnum"),
  brace_segment(headers["Delay"],"typedef struct DelayStruct"),
  brace_segment(headers["FxBus"],"enum FxTypeEnum"),
  brace_segment(headers["FxBus"],"typedef struct FxStruct"),
]
m=re.search(r"enum\s*\{\s*MIXER_ROUTING_DAC1_STEREO.*?\};",headers["Mixer"],re.S)
assert m
types.append(m.group(0))
TYPES="\n".join(types)
# Entire four actual effect translation units are appended, stripped only of #include lines.
EFFECTS="\n".join(strip_includes(D[x]) for x in ["Distortion","Compressor","Delay","Ringmod"])
# Actual one-pole source is included for Delay's production filter recurrence.
LP_BODY=strip_includes(LP)
# Ring waveform paths use the exact oscillator sample and frequency helpers.
osc_funcs=["freq2PhaseIncr","fast_log2","freqToMidiNote","freqToTableIndex",
 "freq2PhaseIncr1024","freq2PhaseIncr32767","calcSineBlock","calcWavetableOscBlock",
 "calcNoiseBlock","calcPwmBlock","calcNextOscSampleBlock",
 "osc_calcSineFreq","osc_calcNoiseFreq","osc_calcWavetableFreq","osc_calcSampleFreq",
 "osc_calcUserSampleFreq","osc_setFreq"]
# Compiler sees forward function prototypes because original function definitions have
# a different source ordering than their host translation-unit layout.
OSC_PROTO=r'''
void calcSineBlock(OscInfo*,int16_t*,uint8_t,float);
void calcWavetableOscBlock(OscInfo*,const int16_t[][1024],int16_t*,uint8_t,float);
void calcNoiseBlock(OscInfo*,int16_t*,uint8_t,float);
void calcPwmBlock(OscInfo*,int16_t*,uint8_t,float,float);
void calcNextOscSampleBlock(OscInfo*,int16_t*,uint8_t,float,float);
void osc_setFreq(OscInfo*);
'''
osc_body="\n".join(find(x,OS) for x in osc_funcs)
# All four original wavetables, not generated approximations.
tables="\n".join(brace_segment(WAV,needle) for needle in ["INDTCM const int16_t sine_table"," const int16_t recTable","const int16_t triTable","const int16_t sawTable"])
ext_fx=find("FxBus_processSelected",BUS)
mixhelpers=find("mixer_checkOutJackAvailable",MIX)+"\n"+find("mixer_addDataToOutput",MIX)
# Source-exact routing and return program slice from production mixer.
lo=MIX.index("    if (!(fxPoolMask & (1u << FX_DISTORTION)) && fxBus.distortion.clipStateActive)")
hi=MIX.index("\n/*\n\tif(activeParameters",lo)
returns=MIX[lo:hi]
assert "FxBus_processSelected" in returns and "Distortion_resetClip" in returns
assert "IN_ITCM_RAM static __attribute__((noinline)) void mixer_addDataToOutput" in mixhelpers
setup=r'''
#define OUTPUT_DMA_SIZE 64
static FxBus fxBus;
static _Alignas(4) int16_t fxPoolInputs[FX_POOL_PROCESSOR_COUNT][OUTPUT_DMA_SIZE*2];
static _Alignas(4) int16_t fxPoolReturn[2][OUTPUT_DMA_SIZE*2];
static _Alignas(4) int16_t stereoOut2[OUTPUT_DMA_SIZE*2],stereoOut1[OUTPUT_DMA_SIZE*2];
static _Alignas(4) __fp16 FX_BUFFER[FX_BUFFER_SIZE+1];
static uint8_t mixer_audioRouting[6];
static void initOnePole(OnePoleFilter*);
static float calcOnePole(OnePoleFilter*,double);
void Distortion_resetClip(MasterDistortion*);
void Distortion_process(MasterDistortion*,int16_t*,int16_t*,int16_t*,uint16_t,uint8_t,float);
void Ringmod_process(Ringmod*,int16_t*,int16_t*,int16_t*,uint16_t,uint8_t,float);
void Compressor_process(Compressor*,int16_t*,int16_t*,int16_t*,uint16_t,uint8_t,float);
void Delay_process(Delay*,int16_t*,int16_t*,int16_t*,uint16_t,uint8_t,float);
void Delay_processPingPong(Delay*,int16_t*,int16_t*,int16_t*,uint16_t,uint8_t,float);
void Delay_setDelayTime(Delay*,float);
'''
# Forward prototype for Ringmod dependencies and oscillator lookup tables.
prototypes=r'''
extern const int16_t sine_table[TABLESIZE+1];
extern const int16_t recTable[11][1024],triTable[11][1024],sawTable[11][1024];
'''
RUNTIME=r'''
static void process_active(uint8_t mask,int16_t*output,int16_t*output2) {
 const uint8_t pos=0;
 const uint8_t fxPoolMask=mask;
 /* BEGIN_EXACT_R23_RETURN */
'''
RUNTIME2=r'''
 /* END_EXACT_R23_RETURN */
}
static uint32_t seed=0x14c0ffee;
static uint32_t rand32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static void state_init(int mode,int ping,int wave) {
 memset(&fxBus,0,sizeof(fxBus));memset(FX_BUFFER,0,sizeof(FX_BUFFER));
 fxBus.fxType=FX_DISTORTION;fxBus.audioRouting=(uint8_t)mode;fxBus.dryWet=.75f;
 Distortion_init(&fxBus.distortion);
 fxBus.distortion.distortionType=DISTORTION_RAT;
 fxBus.distortion.drive=.36f;fxBus.distortion.p1=.28f;
 fxBus.distortion.p2=.48f;fxBus.distortion.p3=.78f;
 Ringmod_init(&fxBus.ringmod);
 fxBus.ringmod.osc.waveform=(uint8_t)wave;
 Ringmod_setFreq(&fxBus.ringmod, .5f);
 Compressor_init(&fxBus.compressor);
 Compressor_setAttack(&fxBus.compressor,.28f);
 Compressor_setRelease(&fxBus.compressor,.52f);
 Compressor_setTreshold(&fxBus.compressor,.78f);
 Compressor_setRatio(&fxBus.compressor,3);
 Compressor_setGain(&fxBus.compressor,.25f);
 Delay_init(&fxBus.delay);fxBus.delay.delayType=(uint8_t)ping;
 Delay_setRange(&fxBus.delay,.48f);Delay_setDelayTime(&fxBus.delay,.23f);
 Delay_setFeedback(&fxBus.delay,.43f);
}
static uint8_t voice_routes(int scenario) {
 static const uint8_t routes[9][6]={
  {9,8,10,7,9,7}, /* four genuine processors in same block */
  {9,9,9,9,9,9}, /* distortion alone */
  {8,8,8,8,8,8}, /* ring alone */
  {10,10,10,10,10,10}, /* compressor alone */
  {7,7,7,7,7,7}, /* delay alone */
  {9,8,9,8,9,8}, /* distortion + ring */
  {9,7,9,7,9,7}, /* distortion + delay */
  {0,1,2,3,4,5}, /* physical bypass */
  {6,8,9,10,7,0} /* global legacy and all four, plus physical */
 };
 for(int v=0;v<6;v++)mixer_audioRouting[v]=routes[scenario][v];
 return SharedFxPool_activeMask(mixer_audioRouting,fxBus.fxType);
}
static void make_inputs(int scenario,int block,int amp) {
 memset(fxPoolInputs,0,sizeof(fxPoolInputs));
 memset(stereoOut1,0,sizeof(stereoOut1));memset(stereoOut2,0,sizeof(stereoOut2));
 r1_Available=l1_Available=r2_Available=l2_Available=1;
 for(int v=0;v<6;v++) {
  _Alignas(4) int16_t data[OUTPUT_DMA_SIZE];
  for(int i=0;i<OUTPUT_DMA_SIZE;i++){
   int32_t val=(int32_t)(rand32()%((uint32_t)amp*2+1))-(int32_t)amp;
   if(scenario==7)val=val/2;
   if((block%5)==4)val=0;
   data[i]=(int16_t)val;
  }
  const float panL=(float)(40+v*9)/128.f;
  const float panR=(float)(110-v*8)/128.f;
  mixer_addDataToOutput(mixer_audioRouting[v],panL,panR,data,
    stereoOut2,stereoOut2+1,stereoOut1,stereoOut1+1,&fxPoolInputs[0][0]);
 }
}
static void sum_return(int16_t*dest,const int16_t*source){
 for(int i=0;i<OUTPUT_DMA_SIZE;i++)
  ((int32_t*)dest)[i]=__QADD16(((int32_t*)dest)[i],((const int32_t*)source)[i]);
}
static int failures;
static unsigned completeCases,allFourCases;
static void case_run(int scenario,int mode,int wet,int ping,int wave,int dist,int block) {
 state_init(mode,ping,wave);
 fxBus.dryWet=wet==0?0.f:wet==1?.45f:1.f;
 fxBus.distortion.distortionType=(uint8_t)dist;
 uint8_t mask=voice_routes(scenario);
 make_inputs(scenario,block,6000);
 _Alignas(4) int16_t beforeDac2[128],beforeDac1[128],actual2[128],actual1[128],ref2[128],ref1[128];
 _Alignas(4) int16_t inputCopy[5][128];
 memcpy(beforeDac2,stereoOut2,sizeof(beforeDac2));memcpy(beforeDac1,stereoOut1,sizeof(beforeDac1));
 memcpy(inputCopy,fxPoolInputs,sizeof(inputCopy));
 FxBus before=fxBus;
 __fp16 *bufferBefore=malloc(sizeof(FX_BUFFER));
 if(!bufferBefore)abort();
 memcpy(bufferBefore,FX_BUFFER,sizeof(FX_BUFFER));
 process_active(mask,stereoOut2,stereoOut1);
 memcpy(actual2,stereoOut2,sizeof(actual2));memcpy(actual1,stereoOut1,sizeof(actual1));
 memcpy(ref2,beforeDac2,sizeof(ref2));memcpy(ref1,beforeDac1,sizeof(ref1));
 const bool multi=(mask&(mask-1u))!=0;
 for(uint8_t type=FX_OFF;type<=FX_DELAY;type++)if(mask&(1u<<type)){
  fxBus=before;memcpy(FX_BUFFER,bufferBefore,sizeof(FX_BUFFER));
  if(multi){
   _Alignas(4) int16_t tmp2[128]={0},tmp1[128]={0};
   FxBus_processSelected(type,inputCopy[type],tmp2,tmp2+1,tmp1,tmp1+1,OUTPUT_DMA_SIZE);
   sum_return(ref2,tmp2);sum_return(ref1,tmp1);
  }else{
   FxBus_processSelected(type,inputCopy[type],ref2,ref2+1,ref1,ref1+1,OUTPUT_DMA_SIZE);
  }
 }
 free(bufferBefore);
 if(memcmp(actual2,ref2,sizeof(ref2))||memcmp(actual1,ref1,sizeof(ref1))){
  fprintf(stderr,"DIFF scenario=%d mode=%d wet=%d ping=%d wave=%d distortion=%d block=%d mask=0x%x\n",
          scenario,mode,wet,ping,wave,dist,block,mask);
  failures++;
 }
 if(mask==0x1eu)allFourCases++;
 completeCases++;
}
int main(void) {
 for(int sc=0;sc<9;sc++)for(int mode=0;mode<6;mode+=2)
 for(int wet=0;wet<3;wet++)for(int ping=0;ping<2;ping++)
 for(int wave=0;wave<2;wave++)for(int dist=0;dist<3;dist++)
 for(int block=0;block<3;block++)
  case_run(sc,mode,wet,ping,wave?SINE:REC,dist,block);
 if(failures){
  fprintf(stderr,"STOP %d output mismatches across %u 64-frame cases\n",failures,completeCases);
  return 1;
 }
 printf("PASS REAL FX DSP integrated output parity: %u 64-frame six-voice scenes; all-four cases=%u\n",completeCases,allFourCases);
 puts("NOTICE: exact production DSP source bodies, real REC/SINE oscillator and Delay buffer; native SIMD/CPU abstractions.");
 puts("LIMIT: no original voice synthesis, no STM32 timing/DMA, and independent parity uses same DSP bodies.");
 return 0;
}
'''
# Real original all-four DSP bodies and native host shims are compiled in one program.
# These function prototypes must appear before the original source bodies.
forward=r'''
void Ringmod_init(Ringmod*);
void Ringmod_setFreq(Ringmod*,float);
void Compressor_init(Compressor*);
void Compressor_setAttack(Compressor*,float);
void Compressor_setRelease(Compressor*,float);
void Compressor_setTreshold(Compressor*,float);
void Compressor_setRatio(Compressor*,uint8_t);
void Compressor_setGain(Compressor*,float);
void Distortion_init(MasterDistortion*);
void Delay_init(Delay*);
void Delay_setRange(Delay*,float);
void Delay_setFeedback(Delay*,float);
'''
code=(PRE+"\n"+TYPES+"\n"+setup+prototypes+OSC_PROTO+forward+"\n"+tables+"\n"+osc_body+"\n"+
      LP_BODY+"\n"+EFFECTS+"\n"+ext_fx+"\n"+mixhelpers+"\n"+RUNTIME+returns+"\n"+RUNTIME2)
(OUT/"all_four_real_dsp.c").write_text(code,encoding="utf-8")
(OUT/"PIN.txt").write_text(PIN+"\nSource bodies from git show, no changes to the four DSP implementations.\n")
print("Generated real-DSP source bytes",len(code.encode()),"SHA256",hashlib.sha256(code.encode()).hexdigest(),flush=True)
for san,flags in [
 ("ubsan",["-fsanitize=undefined,bounds","-fno-sanitize-recover=all"]),
 ("asan",["-fsanitize=address","-fno-sanitize-recover=all"])
]:
 exe=OUT/("real_fx_"+san)
 command=["cc","-std=gnu11","-O1","-g","-fno-strict-aliasing","-fno-omit-frame-pointer",
          "-Wno-unused-parameter","-Wno-unused-function","-I"+str(ROOT/"Core/Inc"),
          *flags,str(OUT/"all_four_real_dsp.c"),"-lm","-o",str(exe)]
 build=subprocess.run(command,text=True,capture_output=True)
 (OUT/(san+"_build.log")).write_text(build.stdout+build.stderr)
 if build.returncode:
  print("COMPILE STOP",san,build.stderr[-10000:],flush=True)
  sys.exit(build.returncode)
 run=subprocess.run([str(exe)],text=True,capture_output=True)
 (OUT/(san+"_run.log")).write_text(run.stdout+run.stderr)
 print("RUN",san,"rc",run.returncode,run.stdout,run.stderr[-6500:],flush=True)
 if run.returncode:sys.exit(run.returncode)
print("PASS both sanitizer runs using four unchanged real FX processors",flush=True)
