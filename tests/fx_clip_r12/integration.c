/* Production voices + verbatim production mixer. HAL/RNG/ADC and runtime
 * modulation dispatch are deterministic shims, not a firmware boot/SD recall. */
#define main old_voice_fixture_main
#include "voice_fixture.c"
#undef main
#include "Soundengine/SoloMix.h"
#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/mixer.h"
#include "Soundengine/DSPAudio/BufferTools.h"
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
static OnePoleFilter *fader(unsigned v){return v<3?&voiceArray[v].FaderVolumeFilter:v==3?&snareVoice.FaderVolumeFilter:v==4?&cymbalVoice.FaderVolumeFilter:&hatVoice.FaderVolumeFilter;}
void modNode_resetTargets(void){}
void modNode_reassignVeloMod(void){}
void lfo_dispatchNextValue(Lfo *lfo){if(profile==6&&lfo==&voiceArray[0].lfo)*volume(selectedVoice)=chosenMix;}
static uint64_t hash(uint64_t h,const void*p,size_t n){const unsigned char*b=p;for(size_t i=0;i<n;i++)h=(h^b[i])*1099511628211ull;return h;}
static uint64_t sourceHash[6],stateHash[6],returnHash[6];
static unsigned checks,zeroChecks,clamps;
static void finite_state(const MasterDistortion *d){const double*x[]={d->hp1_state,d->hp2_state,d->hp3_state,d->lp1_state,d->lp2_state,d->lp3_state,d->slewState,d->lastOut,d->clipBassLp_state};for(unsigned j=0;j<9;j++)for(unsigned c=0;c<2;c++){uint64_t u;memcpy(&u,&x[j][c],8);assert((u&UINT64_C(0x7ff0000000000000))!=UINT64_C(0x7ff0000000000000));}uint32_t u;memcpy(&u,&d->clipFeedbackGain,4);assert((u&0x7f800000)!=0x7f800000);}
int16_t __real_SoloMix_snareSample(float);
int16_t __wrap_SoloMix_snareSample(float x){uint32_t u;memcpy(&u,&x,4);assert((u&0x7f800000)!=0x7f800000);if(x>32767||x< -32768)clamps++;return __real_SoloMix_snareSample(x);}
#ifndef BASELINE
static MasterDistortion reference[6];static OnePoleFilter expectedFader[6];static float expectedMix[6][64];static OnePoleFilter expectedControl[3];
static void observe_addVoice(uint8_t v,const int16_t *s,float pl,float pr,int16_t*l,int16_t*r,int16_t*l2,int16_t*r2,uint16_t n){
 _Alignas(4) int16_t in[128]={0},out[128]={0};float g[64];assert(n<=64);
 for(unsigned i=0;i<n;i++){in[2*i]=s[i];g[i]=FxBus_clipCoreGain(calcOnePole(&expectedFader[v],faderTargets[v]));}
 if(*volume(v)==0)for(unsigned i=0;i<n;i++){assert(s[i]==0);zeroChecks++;}
 reference[v].distortionType=DISTORTION_RAT;reference[v].drive=fxBus.distortion.drive;reference[v].p1=fxBus.distortion.p1;reference[v].p2=fxBus.distortion.p2;reference[v].p3=fxBus.distortion.p3;
 Distortion_processWithClipGain(&reference[v],in,out,NULL,n,1,fxBus.dryWet,g);
 int16_t before[4][128];int16_t*dest[]={l,r,l2,r2};for(unsigned j=0;j<4;j++)for(unsigned i=0;i<n;i++)before[j][2*i]=dest[j][2*i];
 SoloMix_addVoice(v,s,pl,pr,l,r,l2,r2,n);
 assert(!memcmp(&reference[v],&paths[v],sizeof reference[v]));assert(!memcmp(out,isolated,n*4));assert(!memcmp(expectedFader+v,faderReplay+v,sizeof(OnePoleFilter)));assert(!memcmp(fader(v),expectedFader+v,sizeof(OnePoleFilter)));finite_state(paths+v);
 unsigned li=fxBus.audioRouting==1||fxBus.audioRouting==4?0:fxBus.audioRouting==5?1:fxBus.audioRouting==3?3:2;unsigned ri=fxBus.audioRouting==1?1:3;
 for(unsigned i=0;i<n;i++){
  float m=*volume(v)==0?0:v<3?expectedMix[v][i]:*volume(v);int16_t x=(int16_t)(out[2*i]*m);
  for(unsigned j=0;j<4;j++){int add=j==li?(fxBus.audioRouting<2?(int16_t)(x*pl):x):fxBus.audioRouting<2&&j==ri?(int16_t)(x*pr):0;assert(dest[j][2*i]==host_sat((int32_t)before[j][2*i]+add,16));}
 }
 sourceHash[v]=hash(sourceHash[v],s,n*2);stateHash[v]=hash(stateHash[v],paths+v,sizeof paths[v]);returnHash[v]=hash(returnHash[v],out,n*4);checks++;
}
#define SoloMix_addVoice observe_addVoice
#endif
#include "mixer.inc"
#ifndef BASELINE
#undef SoloMix_addVoice
#endif
static void trigger(unsigned v,unsigned velocity){if(v<3)Drum_trigger(v,velocity,60);else if(v==3)Snare_trigger(velocity,60);else if(v==4)Cymbal_trigger(velocity,60);else HiHat_trigger(velocity,0,60);}
int main(int argc,char **argv){
 assert(argc==14);unsigned v=atoi(argv[1]),cfg=atoi(argv[2]),m=atoi(argv[3]),vm=atoi(argv[4]),blocks=atoi(argv[6]),mask=atoi(argv[7]),route=atoi(argv[8]),kind=atoi(argv[9]);profile=atoi(argv[5]);float dw=strtof(argv[10],0),vol=strtof(argv[11],0);unsigned fx=atoi(argv[12]);assert(v<6&&route<6&&m<128);selectedVoice=v;chosenMix=m/127.f;
 for(unsigned j=0;j<6;j++){setup(j,j==v?cfg:0,j==v?chosenMix:profile==10?.2f:0,vm);faderTargets[j]=1;mixer_audioRouting[j]=(mask>>j)&1?6:0;mixer_decimation_rate[j]=1;sourceHash[j]=stateHash[j]=returnHash[j]=1469598103934665603ull;}
 mixer_decimation_rate[6]=1;
 FxBus_init();fxBus.fxType=fx;fxBus.audioRouting=route;fxBus.dryWet=dw;fxBus.distortion.distortionType=kind;fxBus.distortion.drive=fxBus.distortion.p1=fxBus.distortion.p2=1;fxBus.distortion.p3=vol;
 if(profile==7){if(v<3){voiceArray[v].filterType=FILTER_LP;SVF_setReso(&voiceArray[v].filter,.3f);SVF_setDrive(&voiceArray[v].filter,.4f);SVF_directSetFilterValue(&voiceArray[v].filter,.2f);}else if(v==3){snareVoice.filterType=FILTER_LP;SVF_setReso(&snareVoice.filter,.3f);SVF_setDrive(&snareVoice.filter,.4f);SVF_directSetFilterValue(&snareVoice.filter,.2f);}}
 if(profile==8)trigger(v,0);
 FILE *wav=strcmp(argv[13],"-")?fopen(argv[13],"wb"):NULL;if(strcmp(argv[13],"-"))assert(wav);
 uint64_t outputHash=1469598103934665603ull;int peak=0,step=0,prev[4]={0},last=0;double squares=0;unsigned previous=0;
 for(unsigned b=0;b<blocks;b++){
  blockNumber=b;
  if(profile==13){fxBus.distortion.drive=(b%3)/2.f;fxBus.distortion.p1=(b%5)/4.f;fxBus.distortion.p2=(b%7)/6.f;}
  *volume(v)=profile==1&&((b>=8&&b<32)||b>=512)?0:profile==6?0:chosenMix;
  if(profile==10)for(unsigned j=0;j<6;j++)*volume(j)=b>=512?0:(j==v?chosenMix:(j+1)/6.f);
  if(profile==11)*volume(v)=(float[]){0,1/127.f,.2f,1.f}[(b/16)%4];
  float u=profile==2?b<128?1:b<256?(256-b)/128.f:b<384?0:b<512?(b-384)/128.f:1:cfg==6?0:cfg==8?.1f:1;
  for(unsigned j=0;j<6;j++)faderTargets[j]=(profile==10?((b/32+j)%5)/4.f:u)* (profile==10?((b/32+j)%5)/4.f:u);
  if(profile==3){mixer_audioRouting[(v+1)%6]=b%16<8?6:0;if(b%64==0)FxBus_prepareFxChange();}
  if(profile==4){fxBus.fxType=b%32<16?fx:FX_OFF;if(b%32==0)FxBus_prepareFxChange();}
  if(profile==5){for(unsigned j=0;j<6;j++)mixer_audioRouting[j]=(b/8>>j)&1?6:0;}
  if(profile==9){*volume((v+1)%6)=.35f;if(b%64==0)trigger((v+1)%6,127);}
  if(b&&b%128==0&&b<512)for(unsigned j=0;j<6;j++)if(j==v||profile==10)trigger(j,127);
  mixer_decimation_rate[v]=profile==1&&b>=8&&b<32?0:1;
#ifndef BASELINE
  unsigned current=0;for(unsigned j=0;j<6;j++){expectedFader[j]=*fader(j);OnePoleFilter p=j<3?voiceArray[j].MixerPageFilter[0]:(OnePoleFilter){0};float mt=profile==6&&j==v?chosenMix:*volume(j);for(unsigned i=0;i<64;i++)expectedMix[j][i]=j<3?calcOnePole(&p,mt):mt;if(j<3)expectedControl[j]=p;if(fxBus.fxType==FX_DISTORTION&&kind==DISTORTION_RAT&&mixer_audioRouting[j]==6)current|=1u<<j;}
  if(b%64==0&&(profile==3)||b%32==0&&(profile==4))previous=0;
  for(unsigned j=0;j<6;j++)if((current&(1u<<j))&&!(previous&(1u<<j)))Distortion_resetClip(reference+j);previous=current;
#endif
  _Alignas(4) int16_t a[130]={0},c[130]={0};a[128]=c[128]=12345;a[129]=c[129]=-12345;mixer_calcNextSampleBlock(a,c);assert(a[128]==12345&&c[128]==12345&&a[129]==-12345&&c[129]==-12345);
#ifndef BASELINE
  assert(selectedMask==0);for(unsigned j=0;j<3;j++)assert(!memcmp(expectedControl+j,&voiceArray[j].MixerPageFilter[0],sizeof(OnePoleFilter)));
  if(b>=512&&(profile==1||profile==10))for(unsigned i=0;i<128;i++)assert(a[i]==0&&c[i]==0);
#endif
  for(unsigned i=0;i<128;i++){int x[]={a[i],c[i]};for(unsigned j=0;j<2;j++){unsigned ch=j*2+(i&1);if(abs(x[j])>peak)peak=abs(x[j]);if(abs(x[j]-prev[ch])>step)step=abs(x[j]-prev[ch]);prev[ch]=x[j];squares+=(double)x[j]*x[j];if(x[j])last=b*64+i/2+1;}}
  outputHash=hash(hash(outputHash,a,256),c,256);if(wav){for(unsigned i=0;i<64;i++){int16_t frame[]={c[2*i],c[2*i+1],a[2*i],a[2*i+1]};fwrite(frame,8,1,wav);}}
 }
 if(wav)fclose(wav);
 printf("{\"output_hash\":\"%016llx\",\"peak\":%d,\"rms\":%.12g,\"max_step\":%d,\"last_nonzero\":%d,\"voice_checks\":%u,\"zero_checks\":%u,\"snare_clamps\":%u,\"sources\":[",(unsigned long long)outputHash,peak,sqrt(squares/(blocks*256.)),step,last,checks,zeroChecks,clamps);
 for(unsigned j=0;j<6;j++)printf("%s\"%016llx\"",j?",":"",(unsigned long long)sourceHash[j]);printf("],\"states\":[");for(unsigned j=0;j<6;j++)printf("%s\"%016llx\"",j?",":"",(unsigned long long)stateHash[j]);printf("],\"returns\":[");for(unsigned j=0;j<6;j++)printf("%s\"%016llx\"",j?",":"",(unsigned long long)returnHash[j]);puts("]}");
}
