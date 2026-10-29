#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include DISTORTION_SOURCE
static int16_t step(MasterDistortion *d,int16_t l,int16_t r,unsigned ch,float dw){
 _Alignas(4) int16_t in[2]={l,r},out[2]={0};
 Distortion_process(d,in,out,ch==2?out+1:NULL,1,ch,dw);return out[0];
}
static void init(MasterDistortion *d){memset(d,0,sizeof *d);Distortion_init(d);d->distortionType=DISTORTION_RAT;d->drive=1;d->p1=1;d->p2=1;d->p3=1;}
int main(void){
 assert(diode(1.f)==table_1n914[256]);assert(diode(4.2f)==table_1n914[256]);
 for(unsigned i=0;i<10000;i++){float x=-5.f+i*.001f;assert(isfinite(diode(x)));}
 for(unsigned ch=1;ch<=2;ch++){
  MasterDistortion a,b;init(&a);init(&b);
  for(unsigned n=0;n<10000;n++)assert(step(&a,0,0,ch,.4f)==0);
  assert(a.clipFeedbackGain==0 && a.clipSilentSamples==441);
  step(&a,1000,0,ch,.4f);assert(a.clipFeedbackGain==1 && a.clipSilentSamples==0);
  assert(!b.clipStateActive && b.clipFeedbackGain==0);
  for(unsigned n=0;n<441;n++)step(&a,0,0,ch,.4f);
  assert(a.clipFeedbackGain==1);step(&a,0,0,ch,.4f);assert(a.clipFeedbackGain<1);
  for(unsigned n=0;n<221;n++)step(&a,1,-1,ch,.4f);
  assert(a.clipFeedbackGain==0);
  /* Feedback is closed; allow the existing filters to finish their tail. */
  for(unsigned n=0;n<1000;n++)step(&a,0,0,ch,.4f);
  for(unsigned n=0;n<4000;n++)assert(step(&a,0,0,ch,.4f)==0);
  step(&a,-32768,32767,ch,.4f);assert(a.clipFeedbackGain==1);
  Distortion_resetClip(&a);assert(!a.clipStateActive);
  assert(step(&a,0,0,ch,.4f)==0 && a.clipFeedbackGain==0 && a.lastOut[0]!=0);
  a.distortionType=DISTORTION_TUBE;step(&a,1000,1000,ch,.4f);assert(!a.clipStateActive);
  a.distortionType=DISTORTION_FOLD;step(&a,1000,1000,ch,.4f);assert(!a.clipStateActive);
  a.distortionType=DISTORTION_RAT;assert(step(&a,0,0,ch,.4f)==0);
  step(&a,0,1000,ch,.4f);assert(a.clipFeedbackGain==(ch==2?1:0));
 }
 puts("PASS diode endpoints/range; quiet entry; per-instance state; 441-frame hold; fade; residual tolerance; first-sample reopen; reset; Tube/Fold re-entry; mono consumed-channel detection");
 printf("MasterDistortion bytes: %zu\n",sizeof(MasterDistortion));
 return 0;
}

