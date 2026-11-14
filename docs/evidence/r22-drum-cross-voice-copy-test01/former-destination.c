#define main r22_standard_main
#include "tests/r22_drum_copy/cases.c"
#undef main
int main(void)
{
 paramToModTargetInit();
 const uint16_t drive[3]={PAR_FILTER_DRIVE_1,PAR_FILTER_DRIVE_2,PAR_FILTER_DRIVE_3};
 const uint16_t fine[3]={PAR_FINE1,PAR_FINE2,PAR_FINE3};
 for(unsigned source=0;source<3;source++)for(unsigned target=0;target<3;target++)for(unsigned remaining=0;remaining<2;remaining++){
  r22_reset(source);activeParameters[PAR_AMOUNT_LFO1+source]=64;
  activeParameters[PAR_TARGET_LFO1+source]=target_index(source+1,drive[source]);
  mainParameters[1][PAR_TARGET_LFO1+target]=target_index(target+1,fine[target]);
  if(remaining){mainParameters[1][PAR_TARGET_LFO4]=target_index(target+1,fine[target]);mainParameters[1][PAR_VOICE_LFO4]=target+1;}
  PresetVoiceCopy copy;assert(preset_captureVoice(&copy,source));
  unsigned dispatch=sends,fx=fx_preparations,io=api_writes;
  assert(preset_transferVoiceTo(&copy,1,target)==PRESET_VOICE_OK);
  assert(dispatch==sends&&fx==fx_preparations&&io==api_writes);
  assert(preset_saveProject(0)==FR_OK&&preset_loadProject(0,0)==FR_OK);
  preset_loadDrumsetRam(1,0,1);
  assert(voiceArray[target].lfo.modTarget.destination==drive[target]);
  assert(engine_values[fine[target]]==mainParameters[1][fine[target]]);
  assert(snareVoice.lfo.modTarget.destination==(remaining?fine[target]:PAR_NONE));
 }
 puts("PASS18 R22 prior non-OFF own-LFO overrides: former destination receives saved base or retains another LFO assignment after real project roundtrip/target load; no transfer-time dispatch/SD");
 return 0;
}
