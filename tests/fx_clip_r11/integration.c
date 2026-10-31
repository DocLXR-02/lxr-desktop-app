/* Actual six voice trigger/async/sync paths and verbatim mixer block. Hardware
 * and control scheduling services are deterministic shims, not a firmware boot. */
#define main voice_fixture_main
#include "voice_fixture.c"
#undef main
#include "Soundengine/SoloMix.h"
#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/mixer.h"
#include "Soundengine/DSPAudio/BufferTools.h"
#define main old_fx_fixture_main
#include "../fx_clip/fxbus.c"
#undef main
#include SOLO_SOURCE
float mixer_decimation_rate[7],mixer_decimation_cnt[6];int16_t mixer_voice_samples[6];
uint8_t mixer_audioRouting[6],r2_Available,l1_Available,l2_Available,r1_Available;
static struct {unsigned IDR;} gpio={1};
#define switch_r2_GPIO_Port (&gpio)
#define switch_l1_GPIO_Port (&gpio)
#define switch_l2_GPIO_Port (&gpio)
#define switch_r1_GPIO_Port (&gpio)
#define switch_r2_Pin 1
#define switch_l1_Pin 1
#define switch_l2_Pin 1
#define switch_r1_Pin 1
static unsigned selectedVoice,profile,blockNumber;static float chosenMix;
static float *volume(unsigned v){return v<3?&voiceArray[v].vol:v==3?&snareVoice.vol:v==4?&cymbalVoice.vol:&hatVoice.vol;}
/* Resolved runtime target shim; profile 6 represents modulation raising a UI0.
 * Real modulation/morph/storage algorithms are not executed in this fixture. */
void modNode_resetTargets(void){}
void modNode_reassignVeloMod(void){}
void lfo_dispatchNextValue(Lfo *lfo){if(profile==6 && lfo==&voiceArray[0].lfo)*volume(selectedVoice)=chosenMix;}
static void observe_process(int16_t*,int16_t*,int16_t*,int16_t*,int16_t*,uint32_t,const float*);
#include "mixer.inc"
static unsigned saturations;
int16_t __real_SoloMix_snareSample(float);
int16_t __wrap_SoloMix_snareSample(float x){assert(isfinite(x));if(x>32767||x< -32768)saturations++;return __real_SoloMix_snareSample(x);}
static void trigger(unsigned v){if(v<3)Drum_trigger(v,127,60);else if(v==3)Snare_trigger(127,60);else if(v==4)Cymbal_trigger(127,60);else HiHat_trigger(127,0,60);}
static uint64_t fingerprint(uint64_t h,const void *p,size_t n){const unsigned char *a=p;for(size_t i=0;i<n;i++)h=(h^a[i])*1099511628211ull;return h;}
/* Observe the real bus after pan and decimation, before the actual effect. */
static uint64_t sourceHash=1469598103934665603ull,stateHash=1469598103934665603ull;static unsigned zeroInputs;
void __real_SoloMix_process(int16_t*,int16_t*,int16_t*,int16_t*,int16_t*,uint32_t,const float*);
/* Because SoloMix.c is included here, wrapping the call is done in the extracted
 * block by a name-only adapter. No production algorithm is substituted. */
static void observe_process(int16_t *in,int16_t*l,int16_t*r,int16_t*l2,int16_t*r2,uint32_t n,const float*g){
 sourceHash=fingerprint(sourceHash,in,n*4);
 if(*volume(selectedVoice)==0 && selected==(int)selectedVoice)for(unsigned i=0;i<n*2;i++){assert(in[i]==0);zeroInputs++;}

#ifdef FEATURE
 SoloMix_process(in,l,r,l2,r2,n,g);
#else
 FxBus_processWithClipGain(in,l,r,l2,r2,n,g);
#endif
 stateHash=fingerprint(stateHash,&fxBus.distortion,sizeof fxBus.distortion);
 double *histories[]={fxBus.distortion.hp1_state,fxBus.distortion.hp2_state,fxBus.distortion.hp3_state,fxBus.distortion.lp1_state,fxBus.distortion.lp2_state,fxBus.distortion.lp3_state,fxBus.distortion.slewState,fxBus.distortion.lastOut,fxBus.distortion.clipBassLp_state};
 for(unsigned j=0;j<9;j++)for(unsigned c=0;c<2;c++)assert(isfinite(histories[j][c]));assert(isfinite(fxBus.distortion.clipFeedbackGain));
}
int main(int argc,char **argv){
 assert(argc==13);unsigned v=atoi(argv[1]),cfg=atoi(argv[2]),m=atoi(argv[3]),vm=atoi(argv[4]),blocks=atoi(argv[6]),mask=atoi(argv[7]),route=atoi(argv[8]),kind=atoi(argv[9]);profile=atoi(argv[5]);selectedVoice=v;chosenMix=m/127.f;assert(v<6&&m<128&&route<6);
 setup(v,cfg,chosenMix,vm);
 for(unsigned j=0;j<6;j++){
  if(j!=v)*volume(j)=0;
  mixer_audioRouting[j]=(mask>>j)&1?MIXER_ROUTING_FX_BUS:0;
  mixer_decimation_rate[j]=1;
 }
 mixer_decimation_rate[6]=1;
 if(profile==9){unsigned direct=(v+1)%6;*volume(direct)=.35f;trigger(direct);}
 if(profile==8){if(v<3)Drum_trigger(v,0,60);else if(v==3)Snare_trigger(0,60);else if(v==4)Cymbal_trigger(0,60);else HiHat_trigger(0,0,60);}
 for(unsigned j=0;j<3;j++)voiceArray[j].filterType=FILTER_OFF;
 snareVoice.filterType=cymbalVoice.filterType=hatVoice.filterType=FILTER_OFF;
 /* Known unstable reproduction is a separate explicit mode, never a pass. */
 if(profile==7){if(v<3)voiceArray[v].filterType=FILTER_LP;else if(v==3)snareVoice.filterType=FILTER_LP;}
 FxBus_init();fxBus.fxType=FX_DISTORTION;fxBus.audioRouting=route;fxBus.dryWet=strtof(argv[10],NULL);fxBus.distortion.distortionType=kind;fxBus.distortion.drive=fxBus.distortion.p1=fxBus.distortion.p2=1;fxBus.distortion.p3=strtof(argv[11],NULL);
 SlopeEg2 *eg=v<3?&voiceArray[v].oscVolEg:v==3?&snareVoice.oscVolEg:v==4?&cymbalVoice.oscVolEg:&hatVoice.oscVolEg;
 if(profile){slopeEg2_setDecay(eg,30,0);if(v==5)hatVoice.decayClosed=hatVoice.decayOpen=slopeEg2_calcDecay(30);}
 FILE *f=strcmp(argv[12],"-")?fopen(argv[12],"wb"):NULL;if(strcmp(argv[12],"-"))assert(f);
 uint64_t outputHash=sourceHash;double sum=0;int peak=0,step=0,prev[4]={0},last=0;unsigned long count=0;
 for(unsigned b=0;b<blocks;b++){
  blockNumber=b;
  *volume(v)=profile==1&&((b>=8&&b<32)||b>=768)?0:profile==4?(b/8)%5==0?0:((b/8)%5)/4.f:profile==6?0:chosenMix;
  float u=profile==2?b<128?1:b<512?(512-b)/384.f:b<640?0:b<768?(b-640)/128.f:1:1;qtarget=cfg==6?0:cfg==8?.01f:u*u;
  if(profile && (b==512||b==1024))trigger(v);
  mixer_decimation_rate[v]=profile==1&&b>=8&&b<32?0:profile?.125f:1;
  if(profile==5){mixer_audioRouting[(v+1)%6]=b%8<4?MIXER_ROUTING_FX_BUS:0;if(b%32==0)FxBus_prepareFxChange();}
  OnePoleFilter expected=v<3?voiceArray[v].MixerPageFilter[0]:(OnePoleFilter){0};float expectedMix[64];for(unsigned n=0;n<64;n++)expectedMix[n]=v<3?calcOnePole(&expected,*volume(v)):*volume(v);
  if(profile==6)for(unsigned n=0;n<64;n++)expectedMix[n]=chosenMix;
  _Alignas(4) int16_t a[130]={0},a2[130]={0};a[128]=a2[128]=12345;a[129]=a2[129]=-12345;
  mixer_calcNextSampleBlock(a,a2);assert(a[128]==12345&&a2[128]==12345&&a[129]==-12345&&a2[129]==-12345);
#ifdef FEATURE
  unsigned routes=0;for(unsigned j=0;j<6;j++)routes+=mixer_audioRouting[j]==MIXER_ROUTING_FX_BUS;
  if(routes==1&&kind==DISTORTION_RAT){for(unsigned n=0;n<64;n++)assert(postMix[n]==(*volume(v)==0?0:expectedMix[n]));assert(selected==-1);}
#endif
  if(v<3 && profile!=6)assert(!memcmp(&expected,&voiceArray[v].MixerPageFilter[0],sizeof expected));
  for(unsigned n=0;n<128;n++){int x[2]={a[n],a2[n]};for(unsigned j=0;j<2;j++){sum+=(double)x[j]*x[j];count++;if(abs(x[j])>peak)peak=abs(x[j]);unsigned c=j*2+n%2;if(abs(x[j]-prev[c])>step)step=abs(x[j]-prev[c]);prev[c]=x[j];if(x[j])last=b*64+n/2+1;}}
  outputHash=fingerprint(outputHash,a,256);outputHash=fingerprint(outputHash,a2,256);if(f){fwrite(a,256,1,f);fwrite(a2,256,1,f);}
 }
 if(f)fclose(f);
 printf("{\"source_hash\":\"%016llx\",\"state_hash\":\"%016llx\",\"output_hash\":\"%016llx\",\"peak\":%d,\"rms\":%.12g,\"max_step\":%d,\"last_nonzero\":%d,\"zero_input_checks\":%u,\"snare_saturations\":%u}\n",(unsigned long long)sourceHash,(unsigned long long)stateHash,(unsigned long long)outputHash,peak,sqrt(sum/count),step,last,zeroInputs,saturations);
}
