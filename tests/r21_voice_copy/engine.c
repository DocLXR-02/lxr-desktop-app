/* Production load sweep and Cortex destination case. Parameter application and
 * modulation nodes are observed adapters; no audio renderer runs in this fixture. */
#include "Soundengine/MIDI/MidiParser.h"
#include "Soundengine/DSPAudio/lfo.h"
static struct {uint8_t fxType;} fxBus;
#define FX_OFF 0
void preset_touchTimer(PresetTimer type){(void)type;}
static uint8_t engine_values[END_OF_SOUND_PARAMETERS];
static unsigned engine_seen[END_OF_SOUND_PARAMETERS],fx_preparations;
static struct {Lfo lfo;} voiceArray[3],snareVoice,cymbalVoice,hatVoice;
static MidiMsg cortex_midiMsg;
void modNode_setDestination(ModulationNode *node,uint16_t destination){node->destination=destination;}
static void r21_frontSend(uint8_t status,uint8_t a,uint8_t b)
{
 if(status==CC_LFO_TARGET){
  cortex_midiMsg.status=status;cortex_midiMsg.data1=a;cortex_midiMsg.data2=b;
  #include "destination.inc"
 }
}
void midiParser_ccHandler(MidiMsg msg,uint8_t original,uint8_t floating,float value)
{
 (void)value;assert(original==1&&!floating);
 unsigned id=msg.status==MIDI_CC?msg.data1-1u:msg.data1+128u;
 assert(id<END_OF_SOUND_PARAMETERS);
 assert(id!=PAR_NRPN_DATA_ENTRY_COARSE && id!=PAR_NRPN_FINE && id!=PAR_NRPN_COARSE && id!=PAR_BANK_CHANGE);
 engine_values[id]=msg.data2;engine_seen[id]++;
}
void FxBus_prepareFxChange(void){fx_preparations++;}
void soundengine_process(void){}
#include "load.inc"
#include "retrigger.inc"
