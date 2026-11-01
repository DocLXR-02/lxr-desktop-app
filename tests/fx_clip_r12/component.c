/* Component contracts use the actual new helpers and unchanged CLP DSP. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "Soundengine/SoloMix.h"
#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/mixer.h"
#define main old_fx_fixture_main
#include "../fx_clip/fxbus.c"
#undef main
#include SOLO_SOURCE
static float qtarget=1;
float adc_getFaderVolume(uint8_t voice){assert(voice<6);return qtarget;}
static void init(void){FxBus_init();fxBus.fxType=FX_DISTORTION;fxBus.distortion.distortionType=DISTORTION_RAT;fxBus.distortion.drive=fxBus.distortion.p1=fxBus.distortion.p2=fxBus.distortion.p3=1;fxBus.dryWet=1;fxBus.audioRouting=0;}
static void selection(void){
 for(unsigned mask=0;mask<64;mask++)for(unsigned fx=0;fx<5;fx++)for(unsigned kind=0;kind<3;kind++){
  uint8_t routes[6];float t[6]={0,.1,.2,.3,.4,.5};for(unsigned v=0;v<6;v++)routes[v]=(mask>>v)&1?6:0;
  SoloMix_begin(routes,fx,kind,t);
  for(unsigned v=0;v<6;v++){int want=fx==1&&kind==2&&((mask>>v)&1);assert(SoloMix_isVoice(v)==want);assert(SoloMix_sourceLevel(v,t[v])==(want?(t[v]==0?0:1):t[v]));}
 }
}
static void mapping(void){float u[]={0,.1,.25,.4,.5,.75,1};float g[]={0,.2,.5,.8,1,1,1};for(unsigned i=0;i<7;i++)assert(fabsf(FxBus_clipCoreGain(u[i]*u[i])-g[i])<2e-7f);float prev=0;for(unsigned i=0;i<=100000;i++){float x=FxBus_clipCoreGain(i/100000.f);assert(x>=prev&&x>=0&&x<=1);prev=x;}assert(FxBus_clipCoreGain(-1)==0&&FxBus_clipCoreGain(2)==1);}
static void conversion(void){for(int i=-100000;i<100000;i++){float x=i*.32768f;assert(SoloMix_snareSample(x)==(int16_t)x);}float x[]={-100000,-32768,-32767.9f,-.9f,0,32767,32767.9f,34647,52787};for(unsigned i=0;i<9;i++)assert(SoloMix_snareSample(x[i])==(x[i]>32767?32767:x[i]< -32768?-32768:(int16_t)x[i]));}
static void partitions(void){
 static int16_t rendered[3][6][2048];static MasterDistortion final[3][6];unsigned sizes[]={1,7,64};
 for(unsigned p=0;p<3;p++){
  memset(paths,0,sizeof paths);previousMask=selectedMask=0;resetSerial=0;init();OnePoleFilter f[6],mix[3];const OnePoleFilter *fp[6];float qt[6],mt[6]={.1,.2,.3,.4,.5,.6};uint8_t routes[6]={6,6,6,6,6,6};
  for(unsigned v=0;v<6;v++){f[v]=(OnePoleFilter){v<3?.01:.1,1};fp[v]=f+v;qt[v]=1;if(v<3)mix[v]=(OnePoleFilter){.01,mt[v]};}
  for(unsigned at=0;at<2048;){unsigned n=sizes[p];if(at+n>2048)n=2048-at;SoloMix_captureFaders(fp,qt);SoloMix_begin(routes,1,2,mt);
   for(unsigned v=0;v<6;v++){
    int16_t s[64];for(unsigned i=0;i<n;i++)s[i]=at+i<700?(int16_t)(10000*sin((at+i)*.071*(v+1))):0;
    if(v<3){float combined;SoloMix_drum(v,s,n,mix+v,f+v,&combined);}else for(unsigned i=0;i<n;i++)s[i]*=calcOnePole(f+v,1);
    SoloMix_gate(v,s,n);_Alignas(4) int16_t a[132]={0},b[132]={0};a[128]=b[128]=12345;
    SoloMix_addVoice(v,s,1,0,a,a+1,b,b+1,n);assert(a[128]==12345&&b[128]==12345);for(unsigned i=0;i<n;i++)rendered[p][v][at+i]=b[2*i];
   }
   _Alignas(4) int16_t z[128]={0};SoloMix_process(z,z,z+1,z,z+1,n,NULL);at+=n;
  }
  memcpy(final[p],paths,sizeof paths);
 }
 assert(!memcmp(rendered[0],rendered[1],sizeof rendered[0])&&!memcmp(rendered[0],rendered[2],sizeof rendered[0]));assert(!memcmp(final[0],final[1],sizeof final[0])&&!memcmp(final[0],final[2],sizeof final[0]));
}
static void lifecycle(void){
 init();uint8_t routes[6]={6,6,6,6,6,6};float t[6]={1,1,1,1,1,1};OnePoleFilter f[6];const OnePoleFilter*fp[6];for(unsigned v=0;v<6;v++){f[v]=(OnePoleFilter){.1,1};fp[v]=f+v;}SoloMix_captureFaders(fp,t);SoloMix_begin(routes,1,2,t);
 int16_t s[64];for(unsigned i=0;i<64;i++)s[i]=2000;_Alignas(4)int16_t z[128]={0};for(unsigned v=0;v<6;v++)SoloMix_addVoice(v,s,.7f,.7f,z,z+1,z,z+1,64);
 assert(paths[5].clipFeedbackGain==1);MasterDistortion before=paths[5];routes[5]=0;SoloMix_begin(routes,1,2,t);assert(!memcmp(&before,paths+5,sizeof before));routes[5]=6;SoloMix_begin(routes,1,2,t);assert(!paths[5].clipStateActive);
 memset(s,0,sizeof s);memset(z,0,sizeof z);SoloMix_addVoice(5,s,1,0,z,z+1,z,z+1,64);for(unsigned i=0;i<128;i++)assert(z[i]==0);assert(paths[5].clipFeedbackGain==0);
 fxBus_clipResetSerial=UINT32_MAX;SoloMix_begin(routes,1,2,t);FxBus_prepareFxChange();assert(fxBus_clipResetSerial==0);SoloMix_begin(routes,1,2,t);for(unsigned v=0;v<6;v++)assert(!paths[v].clipStateActive);
}
static void idle(void){
 init();uint8_t routes[6]={6,6,6,6,6,6};float t[6]={1,1,1,1,1,1};OnePoleFilter f[6];const OnePoleFilter *fp[6];for(unsigned v=0;v<6;v++){f[v]=(OnePoleFilter){v<3?.01:.1,1};fp[v]=f+v;}
 for(unsigned b=0;b<128;b++){
  SoloMix_captureFaders(fp,t);SoloMix_begin(routes,1,2,t);
  for(unsigned v=0;v<6;v++){
   int16_t s[64];for(unsigned i=0;i<64;i++)s[i]=b<4?2000:0;
   if(v<3)for(unsigned i=0;i<64;i++)postMix[i]=1;
   _Alignas(4) int16_t a[128]={0},c[128]={0};SoloMix_addVoice(v,s,1,0,a,a+1,c,c+1,64);
   if(b>80){for(unsigned i=0;i<128;i++)assert(a[i]==0&&c[i]==0);assert(paths[v].clipFeedbackGain==0);}
  }
 }
}
static void control_snapshot(void){
 init();uint8_t routes[6]={6,6,6,6,6,6};float t[6]={1,1,1,1,1,1};OnePoleFilter f[6];const OnePoleFilter *fp[6];for(unsigned v=0;v<6;v++){f[v]=(OnePoleFilter){.1,1};fp[v]=f+v;}
 fxBus.distortion.drive=.37f;fxBus.distortion.p1=.83f;fxBus.distortion.p2=.12f;
 FxBus_prepareFxChange();SoloMix_captureFaders(fp,t);SoloMix_begin(routes,1,2,t);
 int16_t src[64];for(unsigned i=0;i<64;i++){src[i]=2000;postMix[i]=1;} _Alignas(4) int16_t a[128]={0},b[128]={0};
 SoloMix_addVoice(0,src,1,0,a,a+1,b,b+1,64);
 fxBus.distortion.drive=.9f;fxBus.distortion.p1=.1f;fxBus.distortion.p2=.8f;
 SoloMix_addVoice(1,src,1,0,a,a+1,b,b+1,64);
 assert(!memcmp(paths,paths+1,sizeof paths[0]));assert(paths[1].drive==.37f&&paths[1].p1==.83f&&paths[1].p2==.12f);
}
int main(void){init();mapping();selection();conversion();partitions();lifecycle();idle();control_snapshot();puts("PASS: 960 eligibility combinations; actual H mapping; finite snare conversion; six-path 1/7/64 partitions; route/FX reset and epoch wrap");}
