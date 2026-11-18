#!/usr/bin/env python3
"""R23/R23-A exact-source, isolated, native host mixer parity and runtime probe.
This does not measure STM32 cycles or end-to-end FX audio.
"""
import hashlib, os, re, subprocess, statistics
from pathlib import Path

R23="d0b49dff26b6fcf11e0367249d329a6682bfefa4"
R23A="a6e0cf675837ad01f94e52d7813b2d49e5795814"
MIX="Core/Src/Soundengine/DSPAudio/mixer.c"
out=Path("build/r23-ab-runtime")
out.mkdir(parents=True,exist_ok=True)
def get(sha,path):
    return subprocess.check_output(["git","show",sha+":"+path],text=True).replace("\r\n","\n")
def slice_between(s,start,end):
    p=s.index(start)
    return s[p:s.index(end,p)].rstrip()
inline="IN_ITCM_RAM static inline void mixer_addDataToOutput("
noinline="IN_ITCM_RAM static __attribute__((noinline)) void mixer_addDataToOutput("
original=get(R23,MIX);candidate=get(R23A,MIX)
f0=slice_between(original,inline,"//#pragma GCC pop_options")
f1=slice_between(candidate,noinline,"//#pragma GCC pop_options")
assert f0.replace(inline,"static void mixer_addDataToOutput(")==f1.replace(noinline,"static void mixer_addDataToOutput("),"Function bodies differ"
start="uint8_t mixer_checkOutJackAvailable(uint8_t dest)"
fallback=slice_between(original,start,"\n//-----------------------------------------------------------------------")
assert fallback==slice_between(candidate,start,"\n//-----------------------------------------------------------------------")
h=get(R23,"Core/Inc/Soundengine/DSPAudio/mixer.h")
enum=re.search(r"enum\s*\{\s*MIXER_ROUTING_DAC1_STEREO,.*?\};",h,re.S)
assert enum, "Cannot extract production routing enum"
prefix=r"""
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Soundengine/SharedFxPool.h"
#define OUTPUT_DMA_SIZE 64
#define IN_ITCM_RAM
/* Reference implementations of ARM's signed packed 16-bit saturated add and halfword pack. */
static inline int16_t sat16(int32_t x) {return (int16_t)(x>32767?32767:x< -32768?-32768:x);}
static inline int32_t simd_add16(int32_t a,int32_t b) {
 int32_t lo=(int16_t)(uint16_t)a+(int16_t)(uint16_t)b;
 int32_t hi=(int16_t)(uint16_t)((uint32_t)a>>16)+(int16_t)(uint16_t)((uint32_t)b>>16);
 return (int32_t)((uint32_t)(uint16_t)sat16(lo)|((uint32_t)(uint16_t)sat16(hi)<<16));
}
#define __QADD16(a,b) simd_add16((a),(b))
#define __PKHBT(a,b,n) ((int32_t)((uint32_t)(uint16_t)(a)|((uint32_t)(uint16_t)(b)<<(n))))
static uint8_t r1_Available,l1_Available,r2_Available,l2_Available;
static struct {uint8_t fxType;} fxBus;
"""
tail=r"""
#define N 64
static _Alignas(4) int16_t out1[128],out2[128],fx[5][128],data[6][64];
static volatile uint64_t sink;
typedef struct {uint8_t route[6],jack,legacy;float l[6],r[6];} Case;
static void init(Case *c,unsigned id){
 unsigned jack=id%16, legacy=(id/16)%5, shift=(id/80)%11, mode=(id/880)%4;
 c->jack=jack;c->legacy=legacy;
 for(unsigned v=0;v<6;v++){
  unsigned route=(shift+v*(mode+1))%11;
  if(mode==1)route=v%6;
  if(mode==2)route=6+v%5;
  if(mode==3)route=(v%2)?6:v;
  c->route[v]=route;
  c->l[v]=((v*17+mode*11)%129)/128.f;
  c->r[v]=((v*13+jack*7)%129)/128.f;
 }
}
static void samples(unsigned seed){
 uint32_t x=seed^0xBEE5AB1Eu;
 for(unsigned v=0;v<6;v++)for(unsigned j=0;j<N;j++){
  x^=x<<13;x^=x>>17;x^=x<<5;data[v][j]=(int16_t)(uint16_t)x;
 }
}
static void render(const Case*c){
 fxBus.fxType=c->legacy;
 l1_Available=!!(c->jack&1);r1_Available=!!(c->jack&2);
 l2_Available=!!(c->jack&4);r2_Available=!!(c->jack&8);
 memset(out1,0,sizeof(out1));memset(out2,0,sizeof(out2));memset(fx,0,sizeof(fx));
 for(unsigned v=0;v<6;v++)
  mixer_addDataToOutput(c->route[v],c->l[v],c->r[v],data[v],&out1[0],&out1[1],&out2[0],&out2[1],&fx[0][0]);
}
static uint64_t ns(void){
 struct timespec t;
 if(clock_gettime(CLOCK_MONOTONIC,&t))exit(6);
 return (uint64_t)t.tv_sec*1000000000ULL+t.tv_nsec;
}
int main(int argc,char**argv){
 if(argc!=4)return 2;
 if(!strcmp(argv[1],"parity")){
  FILE*f=fopen(argv[2],"wb");if(!f)return 3;Case c;
  for(unsigned i=0;i<3520;i++){
   init(&c,i);samples(i*0x9e3779b9u);render(&c);
   if(fwrite(out1,sizeof(out1),1,f)!=1||fwrite(out2,sizeof(out2),1,f)!=1||fwrite(fx,sizeof(fx),1,f)!=1)return 4;
  }
  fclose(f);printf("PARITY %u six-voice frames\n",3520);return 0;
 }
 if(strcmp(argv[1],"bench"))return 5;
 Case c;
 for(unsigned sc=0;sc<8;sc++){
  init(&c,sc*123+13);samples((sc+17)*743);
  for(unsigned rep=0;rep<9;rep++){
   uint64_t start=ns();
   for(unsigned i=0;i<5500;i++){
    render(&c);
    sink+=(uint16_t)out1[i&127]+(uint16_t)out2[(i*11)&127]+(uint16_t)fx[(i+rep)%5][i&127];
   }
   printf("%s,%u,%u,%.3f,%llu\n",argv[3],sc,rep,(double)(ns()-start)/5500.,
      (unsigned long long)sink);
  }
 }
 return 0;
}
"""
files={}
for name,src in [("R23",f0),("R23A",f1)]:
    path=out/(name+".c")
    path.write_text(prefix+enum.group(0)+"\n"+fallback+"\n"+src+"\n"+tail,encoding="utf-8")
    subprocess.run(["cc","-std=c11","-O3","-ffast-math","-fno-strict-aliasing",
      "-Wno-unused-parameter","-I","Core/Inc",str(path),"-o",str(out/name)],check=True)
    print("SOURCE",name,hashlib.sha256(path.read_bytes()).hexdigest())
for name in ["R23","R23A"]:
    subprocess.run([str(out/name),"parity",str(out/(name+".samples")),name],check=True)
a=(out/"R23.samples").read_bytes()
b=(out/"R23A.samples").read_bytes()
assert a==b, "FAIL: audio sample output differs"
digest=hashlib.sha256(a).hexdigest()
print("PASS PARITY: 3520 full-six-voice frames, "+str(len(a))+" bytes, sha256 "+digest)
(out/"parity.txt").write_text("3520 six-voice cases; byte-identical 64-frame DAC and FX inputs; sha256 "+digest+"\n")
for f in out.glob("*.samples"): f.unlink()
rows=[]
for name in ["R23","R23A","R23A","R23"]:
    lines=subprocess.check_output([str(out/name),"bench","unused",name],text=True)
    rows+=lines.strip().splitlines()
(out/"raw-bench.csv").write_text("variant,scenario,rep,ns_per_block,sink\n"+"\n".join(rows)+"\n")
values={}
for row in rows:
    name,sc,rep,ns,sink=row.split(",")
    values.setdefault((name,int(sc)),[]).append(float(ns))
summary=[]
for sc in range(8):
    a=statistics.median(values["R23",sc])
    b=statistics.median(values["R23A",sc])
    summary.append(dict(scenario=sc,baseline_ns=a,candidate_ns=b,delta_percent=100*(b/a-1)))
for item in summary: print("HOST MEDIAN",item)
print("NOTICE: host x86 nanoseconds do not measure ARM cycles, CPU margin or audio deadlines")
print("ALL PASS")
