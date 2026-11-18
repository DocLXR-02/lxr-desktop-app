#!/usr/bin/env python3
"""Compile exact FX parameter bank and effect-output dispatch sources as a host test.

DSP setter implementations are observable stubs. This tests state ownership,
not actual DSP audio output, disk storage, MIDI framing or STM32 scheduling.
"""
import hashlib, re, subprocess, sys
from pathlib import Path

root=Path(__file__).resolve().parents[2]
out=root/"build/r23-host-evidence"
out.mkdir(parents=True,exist_ok=True)
hdr=(root/"Core/Inc/Soundengine/FxBus.h").read_text()
src=(root/"Core/Src/Soundengine/FxBus.c").read_text()
for token in ("bankParameters[FX_PARAMETER_BANK_COUNT][FX_PARAMETER_COUNT]","parameterBankType",
              "FxBus_setBankParameter","FxBus_getBankParameter"):
    assert token in hdr, "Missing production header token: "+token
def function(name):
    rx=r"(?m)^(?:(?:static|IN_ITCM_RAM)\s+)*(?:void|float)\s+"+re.escape(name)+r"\s*\([^;]*?\)\s*\{"
    match=re.search(rx,src,re.S|re.M)
    if not match: raise AssertionError("Production function not found: "+name)
    p=src.index("{",match.start());count=0
    for i in range(p,len(src)):
        if src[i]=="{":count+=1
        elif src[i]=="}":
            count-=1
            if count==0:return src[match.start():i+1]
    raise AssertionError("Unclosed "+name)
names=["FxBus_init","FxBus_syncSelectedBank","FxBus_applyBank","FxBus_updateParameter",
       "FxBus_setParameter","FxBus_setBankParameter","FxBus_getBankParameter",
       "FxBus_setBankWet","FxBus_setBankOutput","FxBus_setSelectedWet",
       "FxBus_setSelectedOutput","FxBus_processSelected"]
extracted="\n\n".join(function(n) for n in names)
stub=r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define USE_REVERB 0
enum FxTypeEnum { FX_OFF,FX_DISTORTION,FX_RING,FX_COMPRESSOR,FX_DELAY };
#define FX_PARAMETER_COUNT 4u
#define FX_PARAMETER_BANK_COUNT 5u
#define DELAY_NORMAL 0
#define IN_ITCM_RAM
#define OUTPUT_DMA_SIZE 64
#define MIXER_ROUTING_DAC1_STEREO 0
#define MIXER_ROUTING_DAC2_STEREO 1
#define MIXER_ROUTING_DAC1_L 2
#define MIXER_ROUTING_DAC1_R 3
#define MIXER_ROUTING_DAC2_L 4
#define MIXER_ROUTING_DAC2_R 5
#define __QADD16(a,b) ((a)+(b))
typedef struct {float p[4];uint8_t distortionType;int clipStateActive;} MasterDistortion;
typedef struct {float p[4];} Ringmod;
typedef struct {float p[4];} Compressor;
typedef struct {float p[4];uint8_t delayType;} Delay;
typedef struct {
 uint8_t fxType,audioRouting;float dryWet;
 float parameter[FX_PARAMETER_COUNT];
 float bankParameters[FX_PARAMETER_BANK_COUNT][FX_PARAMETER_COUNT];
 float bankDryWet[FX_PARAMETER_BANK_COUNT];
 uint8_t bankAudioRouting[FX_PARAMETER_BANK_COUNT];
 uint8_t parameterBankType;
 MasterDistortion distortion;Ringmod ringmod;Compressor compressor;Delay delay;
} FxBus;
static FxBus fxBus;
static void Distortion_init(MasterDistortion*p){memset(p,0,sizeof(*p));}
static void Ringmod_init(Ringmod*p){memset(p,0,sizeof(*p));}
static void Compressor_init(Compressor*p){memset(p,0,sizeof(*p));}
static void Delay_init(Delay*p){memset(p,0,sizeof(*p));}
static void Distortion_setDrive(MasterDistortion*p,float x){p->p[0]=x;}
static void Distortion_setParam1(MasterDistortion*p,float x){p->p[1]=x;}
static void Distortion_setParam2(MasterDistortion*p,float x){p->p[2]=x;}
static void Distortion_setParam3(MasterDistortion*p,float x){p->p[3]=x;}
static void Ringmod_setFreq(Ringmod*p,float x){p->p[0]=x;}
static void Compressor_setAttack(Compressor*p,float x){p->p[0]=x;}
static void Compressor_setRelease(Compressor*p,float x){p->p[1]=x;}
static void Compressor_setTreshold(Compressor*p,float x){p->p[2]=x;}
static void Compressor_setGain(Compressor*p,float x){p->p[3]=x;}
static void Delay_setRange(Delay*p,float x){p->p[1]=x;}
static void Delay_setDelayTime(Delay*p,float x){p->p[0]=x;}
static void Delay_setFilter(Delay*p,float x){p->p[2]=x;}
static void Delay_setFeedback(Delay*p,float x){p->p[3]=x;}
static unsigned lastType,callCount;
static unsigned lastChannels,lastFrames;
static float lastWet;
static int16_t*lastLeft,*lastRight;
static void observe(unsigned type,int16_t*l,int16_t*r,unsigned frames,unsigned channels,float wet) {
 lastType=type;lastLeft=l;lastRight=r;lastFrames=frames;lastChannels=channels;lastWet=wet;callCount++;
}
static void Distortion_process(MasterDistortion*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float wet){(void)d;(void)s;observe(1,l,r,n,ch,wet);}
static void Ringmod_process(Ringmod*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float wet){(void)d;(void)s;observe(2,l,r,n,ch,wet);}
static void Compressor_process(Compressor*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float wet){(void)d;(void)s;observe(3,l,r,n,ch,wet);}
static void Delay_process(Delay*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float wet){(void)d;(void)s;observe(4,l,r,n,ch,wet);}
static void Delay_processPingPong(Delay*d,int16_t*s,int16_t*l,int16_t*r,unsigned n,unsigned ch,float wet){(void)d;(void)s;observe(4,l,r,n,ch,wet);}
"""
main=r"""
static int failures=0;
static unsigned checked=0;
static float expected[5][4];
static float expectedWet[5];
static uint8_t expectedRoute[5];
static unsigned rnd=0x91ae0293u;
static uint32_t next(void){rnd^=rnd<<13;rnd^=rnd>>17;rnd^=rnd<<5;return rnd;}
static void ensure(int check,int step,const char*why){
 checked++;
 if(!check){fprintf(stderr,"FAIL at %d: %s\n",step,why);failures++;}
}
static void verify(int k){
 for(unsigned t=1;t<5;t++)for(unsigned i=0;i<4;i++){
  ensure(FxBus_getBankParameter(t,i)==expected[t][i],k,"read bank");
  ensure(fxBus.bankParameters[t][i]==expected[t][i],k,"stored bank");
 }
 ensure(fxBus.parameterBankType==fxBus.fxType,k,"selected bank");
 for(unsigned i=0;i<4;i++)
  ensure(fxBus.parameter[i]==expected[fxBus.fxType][i],k,"legacy edit view");
 for(unsigned i=0;i<4;i++){
  ensure(fxBus.distortion.p[i]==expected[FX_DISTORTION][i],k,"distortion setter state");
  ensure(fxBus.compressor.p[i]==expected[FX_COMPRESSOR][i],k,"compressor setter state");
  ensure(fxBus.delay.p[i]==expected[FX_DELAY][i],k,"delay setter state");
 }
 ensure(fxBus.ringmod.p[0]==expected[FX_RING][0],k,"ring frequency");
 for(unsigned t=1;t<5;t++) {
  ensure(fxBus.bankDryWet[t]==expectedWet[t],k,"independent wet");
  ensure(fxBus.bankAudioRouting[t]==expectedRoute[t],k,"independent output");
 }
 ensure(fxBus.dryWet==expectedWet[fxBus.fxType],k,"selected wet view");
 ensure(fxBus.audioRouting==expectedRoute[fxBus.fxType],k,"selected output view");
}
int main(void){
 FxBus_init();fxBus.dryWet=.375f;fxBus.audioRouting=2;
 ensure(fxBus.parameterBankType==FX_OFF,0,"init OFF");
 for(int step=0;step<4096;step++){
  unsigned type=1+(next()%4);
  fxBus.fxType=(uint8_t)type;
  FxBus_updateParameter();
  unsigned i=next()%4;
  float value=(float)(next()%1000u)/1000.f;
  if(step&1){fxBus.parameter[i]=value;FxBus_updateParameter();}
  else FxBus_setParameter((uint8_t)i,value);
  expected[type][i]=value;
  unsigned other=1+(next()%4);
  i=next()%4;
  float otherValue=(float)(next()%1000u)/1000.f;
  FxBus_setBankParameter((uint8_t)other,(uint8_t)i,otherValue);
  expected[other][i]=otherValue;
  unsigned wetType=1+(next()%4);
  float wet=(float)(next()%101u)/100.f;
  uint8_t output=(uint8_t)(next()%6u);
  FxBus_setBankWet((uint8_t)wetType,wet);
  FxBus_setBankOutput((uint8_t)wetType,output);
  expectedWet[wetType]=wet;expectedRoute[wetType]=output;
  if((step&7)==0) {
   float selectedWet=(float)(next()%101u)/100.f;
   uint8_t selectedOutput=(uint8_t)(next()%6u);
   FxBus_setSelectedWet(selectedWet);
   FxBus_setSelectedOutput(selectedOutput);
   expectedWet[type]=selectedWet;expectedRoute[type]=selectedOutput;
  }
  verify(step);
 }
 /* Run the production dispatcher against independently chosen per-FX output banks. */
 {
  int16_t audio[128]={0},first[128]={0},second[128]={0};
  for(unsigned type=1;type<5;type++)for(unsigned route=0;route<6;route++)for(unsigned mix=0;mix<=10;mix++){
   float wet=(float)mix/10.f;
   FxBus_setBankWet((uint8_t)type,wet);
   FxBus_setBankOutput((uint8_t)type,(uint8_t)route);
   expectedWet[type]=wet;expectedRoute[type]=(uint8_t)route;
   callCount=0;lastLeft=lastRight=0;lastFrames=lastChannels=0;
   FxBus_processSelected((uint8_t)type,audio,second,second+1,first,first+1,64);
   int16_t *pointer=(route==0||route==2||route==3)?first:second;
   int16_t *other=(route==0||route==2||route==3)?second:first;
   (void)other;
   if(route==1||route==0) {
    ensure(lastLeft==pointer&&lastRight==pointer+1,4097,"stereo output pointers");
   } else {
    ensure(lastLeft==pointer+((route==3||route==5)?1:0)&&lastRight==0,4097,"mono pointer");
   }
   ensure(callCount==1&&lastType==type&&lastFrames==64,4097,"exact single processor");
   ensure(lastChannels==(route<2?2:1)&&lastWet==wet,4097,"effect-local wet and channels");
  }
 }
 FxBus_setBankWet(255,1.f);FxBus_setBankOutput(255,3);
 FxBus_setBankOutput(FX_DELAY,9);
  FxBus_setBankParameter(255,0,1.f);FxBus_setBankParameter(FX_OFF,0,1.f);
 FxBus_setBankParameter(FX_DELAY,4,1.f);FxBus_setParameter(4,1.f);
 ensure(FxBus_getBankParameter(255,0)==0.f,4096,"invalid type");
 ensure(FxBus_getBankParameter(2,4)==0.f,4096,"invalid index");
 verify(4096);
 printf("%s %u checks, 4096 selections and 264 per-FX DSP-routing cases\n",
 failures?"FAIL":"PASS",checked);
 return failures!=0;
}
"""
# Static integration guard: menu/CC2 use the same bank-aware APIs.
menu_source=(root/"Core/Src/menu/menu.c").read_text()
midi_source=(root/"Core/Src/Soundengine/MIDI/MidiParser.c").read_text()
assert "menu_syncFxParameterView(void)" in menu_source
assert "menu_syncFxParameterView();" in midi_source
assert "FxBus_setSelectedOutput(value.itg)" in midi_source
assert "FxBus_setSelectedWet(value.itg/100.f)" in midi_source

code=stub+"\n"+extracted+"\n"+main
outfile=out/"fx_bank_exact_source.c"
outfile.write_text(code,encoding="utf-8")
print("PRODUCTION-FUNCTION-SHA256",hashlib.sha256(extracted.encode()).hexdigest())
for label,extra in [("ubsan",["-fsanitize=undefined,bounds","-fno-sanitize-recover=all"]),("asan",["-fsanitize=address","-fno-sanitize-recover=all"])]:
 exe=out/("fx_bank_"+label)
 run=subprocess.run(["cc","-std=c11","-O2","-Wall","-Wextra","-Werror","-fno-omit-frame-pointer",
        *extra,str(outfile),"-lm","-o",str(exe)],text=True,capture_output=True)
 (out/(label+"_compile.log")).write_text(run.stdout+run.stderr)
 if run.returncode:
  print("COMPILE STOP",label,run.stderr[-5000:]);sys.exit(run.returncode)
 res=subprocess.run([str(exe)],text=True,capture_output=True)
 (out/(label+"_run.log")).write_text(res.stdout+res.stderr)
 print(label,res.stdout,res.stderr[-2000:],flush=True)
 if res.returncode:sys.exit(res.returncode)
print("PASS both sanitizer variants; production code extracted without body modification")
