/* R22 preflight: unchanged R21 engine assignment path; no firmware patch. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define midiParser_ccHandler r22_real_ccHandler
#include "Core/Src/Soundengine/MIDI/MidiParser.c"
#undef midiParser_ccHandler
DrumVoice voiceArray[NUM_DRUM_VOICES];
SnareVoice snareVoice;
HiHatVoice hatVoice;
CymbalVoice cymbalVoice;
FxBus fxBus;
uint8_t activeParameters[NUM_PARAMS],mixer_audioRouting[6],frontParser_activeTrack,menu_activePage;
float mixer_decimation_rate[7];
PatternSet seq_patternSet;
volatile uint32_t systick_ticks;
void SVF_setDrive(ResonantFilter *f,float v){(void)f;(void)v;assert(!"unexpected filter drive");}
void seq_setMute(uint8_t v,uint8_t state){(void)v;(void)state;assert(!"unexpected mute");}
static unsigned cc_dispatches,reset_attempts,reset_valid,fx_setters;
static unsigned reset_seen[END_OF_SOUND_PARAMETERS];
void midiParser_ccHandler(MidiMsg msg,uint8_t original,uint8_t floating,float value)
{
 cc_dispatches++;
 r22_real_ccHandler(msg,original,floating,value);
}
#define paramArray_setParameter r22_real_paramSet
#include "Core/Src/Soundengine/MIDI/ParameterArray.c"
#undef paramArray_setParameter
void paramArray_setParameter(uint16_t id,ptrValue value)
{
 reset_attempts++;reset_seen[id]++;
 if(parameterArray[id].ptr)reset_valid++;
 r22_real_paramSet(id,value);
}
#include "Core/Src/Soundengine/DSPAudio/modulationNode.c"
#include "Core/Src/menu/Cc2Text.c"
/* The real shared FX dispatch bodies call these observed leaf boundaries.
 * No audio effect renderer or physical sound is simulated. */
void Distortion_setDrive(MasterDistortion *d,float v){(void)d;(void)v;fx_setters++;}
void Distortion_setParam1(MasterDistortion *d,float v){(void)d;(void)v;fx_setters++;}
void Distortion_setParam2(MasterDistortion *d,float v){(void)d;(void)v;fx_setters++;}
void Distortion_setParam3(MasterDistortion *d,float v){(void)d;(void)v;fx_setters++;}
void Ringmod_setFreq(Ringmod *d,float v){(void)d;(void)v;assert(!"unselected FX");}
void Compressor_setAttack(Compressor*d,float v){(void)d;(void)v;assert(0);}
void Compressor_setRelease(Compressor*d,float v){(void)d;(void)v;assert(0);}
void Compressor_setTreshold(Compressor*d,float v){(void)d;(void)v;assert(0);}
void Compressor_setGain(Compressor*d,float v){(void)d;(void)v;assert(0);}
void Delay_setRange(Delay*d,float v){(void)d;(void)v;assert(0);}
void Delay_setDelayTime(Delay*d,float v){(void)d;(void)v;assert(0);}
void Delay_setFilter(Delay*d,float v){(void)d;(void)v;assert(0);}
void Delay_setFeedback(Delay*d,float v){(void)d;(void)v;assert(0);}
#include "fx.inc"
static MidiMsg cortex_midiMsg;
static void assign_lfo(uint8_t slot,uint8_t id)
{
 cortex_midiMsg.status=FRONT_CC_LFO_TARGET;
 cortex_midiMsg.data1=(slot<<1)|(id>>7);cortex_midiMsg.data2=id&127;
 #include "destination.inc"
}
int main(void)
{
 parameterArray_init();paramToModTargetInit();fxBus.fxType=FX_DISTORTION;
 activeParameters[PAR_FX_TYPE]=FX_DISTORTION;
 for(unsigned i=0;i<6;i++)activeParameters[PAR_VOICE_LFO1+i]=i+1;
 activeParameters[PAR_AMOUNT_LFO1]=64;activeParameters[PAR_VOICE_LFO1]=1;
 activeParameters[PAR_TARGET_LFO1]=paramToModTarget[PAR_VOL1];
 assert(voiceFromModTargValue(activeParameters[PAR_TARGET_LFO1])==1);
 activeParameters[PAR_VOL1]=32;activeParameters[PAR_VOL3]=16;activeParameters[PAR_FX_P1]=25;
 voiceArray[0].lfo.modTarget.destination=PAR_VOL1;
 voiceArray[0].lfo.modTarget.amount=64.f/127.f;
 voiceArray[2].lfo.modTarget.destination=PAR_VOL3;
 voiceArray[2].lfo.modTarget.amount=64.f/127.f;
 activeParameters[PAR_AMOUNT_LFO3]=64;activeParameters[PAR_TARGET_LFO3]=paramToModTarget[PAR_VOL3];
 snareVoice.lfo.modTarget.destination=PAR_FX_P1;
 snareVoice.lfo.modTarget.amount=64.f/127.f;
 activeParameters[PAR_AMOUNT_LFO4]=64;activeParameters[PAR_TARGET_LFO4]=paramToModTarget[PAR_FX_P1];
 activeParameters[PAR_VOICE_LFO4]=voiceFromModTargValue(activeParameters[PAR_TARGET_LFO4]);
 modNode_updateValue(&voiceArray[0].lfo.modTarget,1.f);
 modNode_updateValue(&voiceArray[2].lfo.modTarget,1.f);
 modNode_updateValue(&snareVoice.lfo.modTarget,1.f);
 float source_before=voiceArray[0].vol,unrelated_before=voiceArray[2].vol,fx_before=fxBus.parameter[0];
 unsigned fx_initial=fx_setters;
 activeParameters[PAR_VOL2]=80;
 MidiMsg destination_value={.status=MIDI_CC,.data1=VOL2,.data2=80};
 midiParser_ccHandler(destination_value,1,0,0.f);
 assert(voiceArray[0].vol==source_before&&voiceArray[2].vol==unrelated_before&&fxBus.parameter[0]==fx_before&&fx_setters==fx_initial);
 puts("PASS negative control: targeted D2 base parameter application leaves source/D3/FX unchanged.");
 uint8_t saved[NUM_PARAMS];memcpy(saved,activeParameters,sizeof saved);
 cc_dispatches=reset_attempts=reset_valid=fx_setters=0;memset(reset_seen,0,sizeof reset_seen);
 assign_lfo(1,PAR_VOL2); /* Same-kit D1 -> D2: install the mapped own LFO. */
 printf("SOURCE D1 volume: before %.9g after %.9g\n",source_before,voiceArray[0].vol);
 printf("UNRELATED D3 volume: before %.9g after %.9g\n",unrelated_before,voiceArray[2].vol);
 printf("UNRELATED shared FX P1: before %.9g after %.9g\n",fx_before,fxBus.parameter[0]);
 printf("Assignment: reset_attempts=%u reset_valid=%u cc_dispatches=%u distortion_leaf_calls=%u\n",reset_attempts,reset_valid,cc_dispatches,fx_setters);
 assert(voiceArray[1].lfo.modTarget.destination==PAR_VOL2);
 assert(!memcmp(saved,activeParameters,sizeof saved));
 assert(reset_attempts==12&&reset_valid==3&&cc_dispatches==3&&fx_setters==4);
 assert(voiceArray[0].vol!=source_before&&voiceArray[2].vol!=unrelated_before&&fxBus.parameter[0]!=fx_before);
 /* Normal renderer modulation replay restores these values at its next block.
  * This is not proof of an audible artifact or deadline miss. */
 modNode_updateValue(&voiceArray[0].lfo.modTarget,voiceArray[0].lfo.modTarget.lastVal);
 modNode_updateValue(&voiceArray[2].lfo.modTarget,voiceArray[2].lfo.modTarget.lastVal);
 modNode_updateValue(&snareVoice.lfo.modTarget,snareVoice.lfo.modTarget.lastVal);
 assert(voiceArray[0].vol==source_before&&voiceArray[2].vol==unrelated_before&&fxBus.parameter[0]==fx_before);
 puts("EXPECTED containment failure reproduced in unchanged production assignment path; next modulation replay restores values; physical audibility UNMEASURED.");
 return 0;
}
