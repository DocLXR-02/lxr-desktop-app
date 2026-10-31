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
static float faderTarget=1;
float adc_getFaderVolume(uint8_t voice){assert(voice<6);return faderTarget;}
static void init(void){FxBus_init();fxBus.fxType=FX_DISTORTION;fxBus.distortion.distortionType=DISTORTION_RAT;fxBus.dryWet=1;fxBus.distortion.drive=fxBus.distortion.p1=fxBus.distortion.p2=fxBus.distortion.p3=1;}
static void selection(void){
 for(unsigned mask=0;mask<64;mask++)for(unsigned fx=0;fx<5;fx++)for(unsigned kind=0;kind<3;kind++){
  uint8_t routes[6];float targets[6]={.1,.2,.3,.4,.5,.6};unsigned count=0;int v=-1;
  for(unsigned j=0;j<6;j++){routes[j]=(mask>>j)&1?MIXER_ROUTING_FX_BUS:0;if(routes[j]==MIXER_ROUTING_FX_BUS){count++;v=j;}}
  SoloMix_begin(routes,fx,kind,targets);int eligible=count==1&&fx==FX_DISTORTION&&kind==DISTORTION_RAT;
  for(unsigned j=0;j<6;j++){assert(SoloMix_isVoice(j)==(eligible&&j==(unsigned)v));assert(SoloMix_sourceLevel(j,targets[j])==((eligible&&j==(unsigned)v)?1:targets[j]));}
 }
}
static void saturation(void){
 for(int i=-100000;i<100000;i++){float x=i*.32768f;assert(SoloMix_snareSample(x)==(int16_t)x);}
 const float x[]={-100000,-32769,-32768.75f,-32768,0,32767,32767.75f,32768,100000};
 for(unsigned i=0;i<9;i++){int want=x[i]>32767?32767:x[i]< -32768?-32768:(int16_t)x[i];assert(SoloMix_snareSample(x[i])==want);}
}
static void post_arithmetic(void){
 unsigned checks=0;
 for(unsigned voice=0;voice<6;voice++)for(unsigned route=0;route<6;route++)for(unsigned dw=0;dw<3;dw++)for(unsigned vol=0;vol<2;vol++)for(unsigned mi=0;mi<3;mi++){
  init();fxBus.audioRouting=route;fxBus.dryWet=dw*.5f;fxBus.distortion.p3=vol;
  uint8_t routes[6]={0};routes[voice]=MIXER_ROUTING_FX_BUS;float targets[6]={0};targets[voice]=mi==0?0:mi==1?1/127.f:1;
  SoloMix_begin(routes,FX_DISTORTION,DISTORTION_RAT,targets);
  OnePoleFilter mix={0},fader={0};mix.r=.01;mix.b=targets[voice];fader.r=.01;fader.b=1;
  float combined;int16_t voiceSamples[64]={0};if(voice<3)SoloMix_drum(voice,voiceSamples,64,&mix,&fader,&combined);
  _Alignas(4) int16_t in[128],ref[128]={0},a[130],b[130],expectedA[128],expectedB[128];float clip[64];
  for(unsigned n=0;n<128;n++){in[n]=mi?(n%2?-3000:12000):0;a[n]=expectedA[n]=((int)(n%3)-1)*1000;b[n]=expectedB[n]=((int)(n%3)-1)*1300;}
  a[128]=b[128]=1234;a[129]=b[129]=-1234;for(unsigned n=0;n<64;n++)clip[n]=.8f;
  MasterDistortion initial=fxBus.distortion;
  FxBus_processWithClipGain(in,ref,route<2?ref+1:ref,ref,route<2?ref+1:ref,64,clip);MasterDistortion reference=fxBus.distortion;fxBus.distortion=initial;
  for(unsigned n=0;n<64;n++){
   unsigned buffer=route==1||route==4||route==5?0:1;unsigned channel=route==3||route==5?1:0;int16_t *expected=buffer?expectedB:expectedA;
   expected[2*n+channel]=host_sat((int32_t)expected[2*n+channel]+(int16_t)(ref[2*n]*postMix[n]),16);
   if(route<2)expected[2*n+1]=host_sat((int32_t)expected[2*n+1]+(int16_t)(ref[2*n+1]*postMix[n]),16);
  }
  SoloMix_process(in,a,a+1,b,b+1,64,clip);
  assert(!memcmp(a,expectedA,256)&&!memcmp(b,expectedB,256));assert(a[128]==1234&&a[129]==-1234&&b[128]==1234&&b[129]==-1234);
  assert(!memcmp(&reference,&fxBus.distortion,sizeof reference));assert(selected==-1);checks++;
 }
 printf("PASS %u whole-return post-MIX/direct-isolation cases; D/W 0/.5/1, VOL 0/1, all six solo/destination cases\n",checks);
}
static int16_t rendered[3][4096];static MasterDistortion histories[3];
static void partitions(void){
 unsigned sizes[]={1,7,64};
 for(unsigned voice=0;voice<6;voice++)for(unsigned p=0;p<3;p++){
  init();float combined;OnePoleFilter mix={0},fader={0};mix.r=.01;fader.r=voice<3?.01:.1;
  for(unsigned n=0;n<2048;){unsigned count=sizes[p];if(count>2048-n)count=2048-n;
   /* Break chunks at target transitions as the production control block does. */
   if(n<512&&n+count>512)count=512-n;if(n<1024&&n+count>1024)count=1024-n;
   float targets[6]={0};targets[voice]=n<512?.2f:n<1024?0:1;
   uint8_t routes[6]={0};routes[voice]=MIXER_ROUTING_FX_BUS;SoloMix_begin(routes,FX_DISTORTION,DISTORTION_RAT,targets);
   _Alignas(4) int16_t in[128]={0},src[64]={0},out[128]={0};float g[64];
   for(unsigned i=0;i<count;i++){src[i]=n+i<1400?(int16_t)(12000*sin(.047*(n+i))):0;g[i]=1;}
   if(voice<3)SoloMix_drum(voice,src,count,&mix,&fader,&combined);else for(unsigned i=0;i<count;i++)src[i]*=calcOnePole(&fader,1);
   SoloMix_gate(voice,src,count);for(unsigned i=0;i<count;i++)in[2*i]=in[2*i+1]=src[i];
   SoloMix_process(in,out,out+1,out,out+1,count,g);memcpy(rendered[p]+2*n,out,4*count);n+=count;
  }
  histories[p]=fxBus.distortion;
  if(p){assert(!memcmp(rendered[0],rendered[p],sizeof rendered[0]));assert(!memcmp(&histories[0],&histories[p],sizeof histories[0]));}
 }
 puts("PASS actual new gain/gate/return and CLP state consistency at 1/7/64-frame partitions; both pole-rate groups");
}
int main(void){selection();saturation();post_arithmetic();partitions();puts("PASS eligibility/mapping ownership and snare finite-domain conversion checks");}
