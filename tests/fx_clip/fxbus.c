#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/mixer.h"
/* Other effects are linked stubs: this fixture exercises actual bus dispatch/reset. */
void Ringmod_init(Ringmod *d){(void)d;}
void Compressor_init(Compressor *d){(void)d;}
void Delay_init(Delay *d){(void)d;}
void Ringmod_process(Ringmod*d,int16_t*i,int16_t*l,int16_t*r,uint16_t n,uint8_t c,float w){(void)d;(void)i;(void)l;(void)r;(void)n;(void)c;(void)w;}
void Compressor_process(Compressor*d,int16_t*i,int16_t*l,int16_t*r,uint16_t n,uint8_t c,float w){(void)d;(void)i;(void)l;(void)r;(void)n;(void)c;(void)w;}
void Delay_process(Delay*d,int16_t*i,int16_t*l,int16_t*r,uint16_t n,uint8_t c,float w){(void)d;(void)i;(void)l;(void)r;(void)n;(void)c;(void)w;}
void Delay_processPingPong(Delay*d,int16_t*i,int16_t*l,int16_t*r,uint16_t n,uint8_t c,float w){(void)d;(void)i;(void)l;(void)r;(void)n;(void)c;(void)w;}
int main(void){
 for(unsigned routing=0;routing<6;routing++){
  _Alignas(4) int16_t in[128]={0},a[128]={0},b[128]={0};
  FxBus_init();fxBus.fxType=FX_DISTORTION;fxBus.audioRouting=routing;fxBus.dryWet=.4f;
  fxBus.distortion.distortionType=DISTORTION_RAT;
  fxBus.distortion.drive=fxBus.distortion.p1=fxBus.distortion.p2=fxBus.distortion.p3=1;
  for(unsigned i=0;i<128;i++)in[i]=2000;
  FxBus_process(in,a,a+1,b,b+1,64);assert(fxBus.distortion.clipStateActive);
  FxBus_prepareFxChange();assert(!fxBus.distortion.clipStateActive);
  memset(in,0,sizeof in);memset(a,0,sizeof a);memset(b,0,sizeof b);
  FxBus_process(in,a,a+1,b,b+1,64);assert(fxBus.distortion.clipFeedbackGain==0);
  for(unsigned i=0;i<128;i++)assert(a[i]==0 && b[i]==0);
  fxBus.fxType=FX_OFF;FxBus_process(in,a,a+1,b,b+1,64);assert(!fxBus.distortion.clipStateActive);
  fxBus.fxType=FX_DISTORTION;FxBus_process(in,a,a+1,b,b+1,64);assert(fxBus.distortion.clipFeedbackGain==0);
 }
 puts("PASS actual FX-bus dispatch/reset: all six output routes, same-type reset and off/re-entry");
}

