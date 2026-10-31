/* Production snare path with mocked control/hardware services. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/modulationNode.h"
ModulationNode velocityModulators[6];
void modNode_updateValue(ModulationNode *node,float value){(void)node;(void)value;}
void lfo_init(Lfo *lfo){(void)lfo;}
void lfo_retrigger(uint8_t voice){(void)voice;}
float adc_getFaderVolume(uint8_t voice){assert(voice==3);return 1;}
uint32_t GetRngValue(void){return 0;}
void calcWavetableOscBlock(OscInfo* osc,const int16_t table[][1024],int16_t* buf,const uint8_t size,const float gain){assert(0);}
void calcPwmBlock(OscInfo *osc,int16_t *buf,uint8_t size,const float gain,const float pwm){assert(0);}
#include "Soundengine/MIDI/MidiNoteNumbers.h"
#define __CLZ(x) __builtin_clz((unsigned)(x))
#include "osc.inc"
int main(int argc,char **argv){
 assert(argc==4);setbuf(stdout,NULL);unsigned mix=atoi(argv[1]),velocityMode=atoi(argv[2]),transient=atoi(argv[3]);
 memset(&snareVoice,0,sizeof snareVoice);Snare_init();
 snareVoice.vol=mix/127.f;snareVoice.volumeMod=velocityMode;snareVoice.mix=0;snareVoice.osc.waveform=SINE;snareVoice.filterType=FILTER_OFF;snareVoice.egPitchModAmount=0;
 transient_setWaveform(&snareVoice.transGen,2);snareVoice.transGen.volume=transient/127.f;snareVoice.transGen.pitch=1.f+((25/33.9f)-.75f);
 slopeEg2_setAttack(&snareVoice.oscVolEg,0,0);slopeEg2_setDecay(&snareVoice.oscVolEg,100,0);
 snareVoice.FaderVolumeFilter.b=1;Snare_trigger(127,60);Snare_calcAsync();
 assert(snareVoice.egValueOscVol==1);assert(snareVoice.osc.phase==((uint32_t)0x3ff<<20));
 SnareVoice before=snareVoice;OscInfo osc=snareVoice.osc;TransientGenerator trans=snareVoice.transGen;
 int16_t oscillator[64],transientSamples[64],out[64];calcSineBlock(&osc,oscillator,64,1);transient_calcBlock(&trans,transientSamples,64);
 printf("mix=%u velocity_mode=%u transient=%u oscillator0=%d transient0=%d envelope=%.9g physical_squared=1 sum0=%.9g converted0=%.9g\n",mix,velocityMode,transient,oscillator[0],transientSamples[0],snareVoice.egValueOscVol,(float)oscillator[0]+transientSamples[0],snareVoice.vol*((float)oscillator[0]+transientSamples[0]));
 for(unsigned i=0;i<64;i++){float sum=(float)oscillator[i]+transientSamples[i],value=snareVoice.vol*sum; if(value>=32768 || value<=-32769) {printf("first_overflow_frame=%u oscillator=%d transient=%d sum=%.9g conversion=%.9g frequency=%.9g phase_increment=%u\n",i,oscillator[i],transientSamples[i],sum,value,snareVoice.osc.freq,snareVoice.osc.phaseInc);break;}}
 float maximum=0;for(unsigned i=0;i<64;i++){float a=fabsf(snareVoice.vol*((float)oscillator[i]+transientSamples[i]));if(a>maximum)maximum=a;}printf("max_absolute_preconversion=%.9g\n",maximum);
 assert(!memcmp(&before,&snareVoice,sizeof before));Snare_calcSyncBlock(out,64);if(mix==0){for(unsigned i=0;i<64;i++)assert(out[i]==0);}puts("PASS native conversion sanitizer");return 0;
}
