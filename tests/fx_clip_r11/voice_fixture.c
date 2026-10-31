/* Full voice generation; hardware/control services mocked, no firmware candidate. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
#include "Soundengine/DSPAudio/HiHat.h"
#include "Soundengine/MIDI/MidiNoteNumbers.h"
#include "audit.h"
#include "Soundengine/MIDI/valueShaper.h"
int audit_full;unsigned audit_saturations;
ModulationNode velocityModulators[6];
static uint32_t rng=1;static float qtarget;
uint32_t GetRngValue(void){rng=rng*1664525u+1013904223u;return rng;}
void modNode_updateValue(ModulationNode *node,float value){(void)node;(void)value;}
void lfo_init(Lfo *lfo){memset(lfo,0,sizeof *lfo);}
void lfo_retrigger(uint8_t voice){(void)voice;}
float adc_getFaderVolume(uint8_t voice){assert(voice<6);return qtarget;}
void calcUserSampleOscFmBlock(OscInfo *osc,int16_t *mod,int16_t *out,uint8_t n,float g){assert(0);}
#define __CLZ(x) __builtin_clz((unsigned)(x))
#define SEQ_DEFAULT_NOTE 60
#define SEMITONE_UP 1.0594630943592952645618252949463f
#include "osc.inc"
static void setup(unsigned v,unsigned cfg,float m,unsigned vm){
 initDrumVoice();Snare_init();Cymbal_init();HiHat_init();
 OscInfo *osc=v<3?&voiceArray[v].osc:v==3?&snareVoice.osc:v==4?&cymbalVoice.osc:&hatVoice.osc;
 TransientGenerator *tg=v<3?&voiceArray[v].transGen:v==3?&snareVoice.transGen:v==4?&cymbalVoice.transGen:&hatVoice.transGen;
 SlopeEg2 *eg=v<3?&voiceArray[v].oscVolEg:v==3?&snareVoice.oscVolEg:v==4?&cymbalVoice.oscVolEg:&hatVoice.oscVolEg;
 Distortion *dist=v<3?&voiceArray[v].distortion:v==3?&snareVoice.distortion:v==4?&cymbalVoice.distortion:&hatVoice.distortion;
 OnePoleFilter *f=v<3?&voiceArray[v].FaderVolumeFilter:v==3?&snareVoice.FaderVolumeFilter:v==4?&cymbalVoice.FaderVolumeFilter:&hatVoice.FaderVolumeFilter;
 ResonantFilter *filter=v<3?&voiceArray[v].filter:v==3?&snareVoice.filter:v==4?&cymbalVoice.filter:&hatVoice.filter;
 float *vol=v<3?&voiceArray[v].vol:v==3?&snareVoice.vol:v==4?&cymbalVoice.vol:&hatVoice.vol;
 uint8_t *veloMode=v<3?&voiceArray[v].volumeMod:v==3?&snareVoice.volumeMod:v==4?&cymbalVoice.volumeMod:&hatVoice.volumeMod;
 uint8_t *filterType=v<3?&voiceArray[v].filterType:v==3?&snareVoice.filterType:v==4?&cymbalVoice.filterType:&hatVoice.filterType;
 *vol=m;*veloMode=vm;osc->waveform=cfg==3?TRI:cfg==4?NOISE:cfg==10?PWM:SINE;osc->pitchMod=1;osc->midiFreq=(70<<8)|64;
 *filterType=((cfg>=4&&cfg<=8)||cfg==11)?FILTER_LP:FILTER_OFF;SVF_directSetFilterValue(filter,cfg==11?valueShaperF2F(126/127.f,-.9f):cfg==7?.9f:.35f);SVF_setReso(filter,(cfg==7||cfg==11)?1:.3f);SVF_setDrive(filter,(cfg==7||cfg==11)?1:.4f);
 transient_setWaveform(tg,cfg==5?3:(cfg==1||cfg==7||cfg==12)?2:0);tg->volume=(cfg==1||cfg==5||cfg==7)?1:0;tg->pitch=1.f+(25/33.9f-.75f);
 slopeEg2_setAttack(eg,0,0);slopeEg2_setDecay(eg,100,0);
 setDistortionShape(dist,(cfg==3||cfg==4||cfg==7||cfg==9)?1:cfg==2?100/127.f:0);dist->DriveParamFilter.r=.01f;dist->DriveParamFilter.b=dist->shape;
 snareVoice.pwm=cymbalVoice.pwm=hatVoice.pwm=cfg==10?1:0;if(v<3)voiceArray[v].pwm=cfg==10?1:0;
 qtarget=cfg==6?0:cfg==8?.01f:1;f->b=qtarget;
 snareVoice.mix=0;snareVoice.egPitchModAmount=0;
 cymbalVoice.fmModAmount1=cymbalVoice.fmModAmount2=hatVoice.fmModAmount1=hatVoice.fmModAmount2=cfg==2?.5f:0;
 cymbalVoice.modOsc.waveform=cymbalVoice.modOsc2.waveform=hatVoice.modOsc.waveform=hatVoice.modOsc2.waveform=SINE;
 cymbalVoice.osc.pitchMod=hatVoice.osc.pitchMod=1;
 hatVoice.decayClosed=hatVoice.decayOpen=slopeEg2_calcDecay(100);
 unsigned note=cfg==0||cfg==7?36:cfg==3||cfg==6?96:60;unsigned vel=cfg==8?32:127;
 if(v<3){voiceArray[v].mixOscs=cfg!=2;voiceArray[v].fmModAmount=cfg==2?.5f:0;voiceArray[v].modOsc.waveform=SINE;voiceArray[v].egPitchModAmount=0;voiceArray[v].MixerPageFilter[0].b=m;Drum_trigger(v,vel,note);}
 else if(v==3)Snare_trigger(vel,note);else if(v==4)Cymbal_trigger(vel,note);else HiHat_trigger(vel,0,note);
}
int main(int argc,char **argv){
 assert(argc==7);setbuf(stdout,NULL);unsigned v=atoi(argv[1]),cfg=atoi(argv[2]),m=atoi(argv[3]),vm=atoi(argv[4]),blocks=atoi(argv[6]);audit_full=atoi(argv[5]);assert(v<6&&cfg<=12&&m<128);
 setup(v,cfg,m/127.f,vm);uint64_t hash=1469598103934665603ull;double sum=0;int peak=0,last=0;_Alignas(4) int16_t data[66];
 for(unsigned b=0;b<blocks;b++){
  if(v<3)calcDrumVoiceAsync(v);else if(v==3)Snare_calcAsync();else if(v==4)Cymbal_calcAsync();else HiHat_calcAsync();
  data[64]=12345;data[65]=-12345;
  if(v<3)calcDrumVoiceSyncBlock(v,data,64);else if(v==3)Snare_calcSyncBlock(data,64);else if(v==4)Cymbal_calcSyncBlock(data,64);else HiHat_calcSyncBlock(data,64);
  assert(data[64]==12345&&data[65]==-12345);
  for(unsigned i=0;i<64;i++){int x=data[i];sum+=(double)x*x;if(abs(x)>peak)peak=abs(x);if(x)last=b*64+i+1;hash=(hash^(uint16_t)x)*1099511628211ull;}
 }
 printf("{\"peak\":%d,\"rms\":%.12g,\"last_nonzero\":%d,\"hash\":\"%016llx\",\"snare_saturations\":%u}\n",peak,sqrt(sum/(64*blocks)),last,(unsigned long long)hash,audit_saturations);return 0;
}
