// SPDX-License-Identifier: GPL-3.0-only
// Existing deterministic file/HAL mocks, unchanged complete production bodies.
#define main containment_fixture_main
#include "../project_storage/host.c"
#undef main
static void bank(void)
{
 reset();morphParameters=mainParameters[1];
 memset(activeParameters,0,END_OF_SOUND_PARAMETERS);memset(morphParameters,127,END_OF_SOUND_PARAMETERS);
 for(unsigned i=0;i<6;i++)activeParameters[PAR_VEL_DEST_1+i]=activeParameters[PAR_TARGET_LFO1+i]=activeParameters[PAR_VOICE_LFO1+i]=0;
}
static unsigned excluded(unsigned i)
{return (i>=PAR_VEL_DEST_1 && i<=PAR_VEL_DEST_6)||(i>=PAR_TARGET_LFO1 && i<=PAR_TARGET_LFO6)||(i>=PAR_VOICE_LFO1 && i<=PAR_VOICE_LFO6);}
int main(void)
{
 for(unsigned m=0;m<128;m++){
  bank();activeParameters[PAR_MORPH]=m;preset_saveDrumsetRam(2,1);
  assert(preset_saveDrumsetSD(2,1)==FR_OK);int e=lookup("/tmp/02-KICKN.snd");assert(e>=0 && entries[e].size==END_OF_SOUND_PARAMETERS+8);
  for(unsigned i=0;i<END_OF_SOUND_PARAMETERS;i++){
   unsigned expected=excluded(i)?0:m;
   assert(mainParameters[2][i]==expected && entries[e].data[i+8]==expected);
  }
  assert(preset_loadDrumsetSD(2,0)==FR_OK);
  for(unsigned i=0;i<END_OF_SOUND_PARAMETERS;i++)assert(mainParameters[2][i]==(excluded(i)?0:m));
  assert(activeParameters[PAR_MORPH]==m);
 }
 /* Every byte-valued runtime amount is excluded from ordinary file payloads. */
 bank();assert(preset_saveTmpKit()==FR_OK);BYTE tmp[4096];unsigned n=entries[lookup("/tmp/tmp.snd")].size;memcpy(tmp,entries[lookup("/tmp/tmp.snd")].data,n);
 for(unsigned m=0;m<256;m++){
  activeParameters[PAR_MORPH]=m;assert(preset_saveTmpKit()==FR_OK);assert(entries[lookup("/tmp/tmp.snd")].size==n && !memcmp(tmp,entries[lookup("/tmp/tmp.snd")].data,n));
 }
 bank();activeParameters[PAR_MORPH]=255;memset(mainParameters[3],255,END_OF_SOUND_PARAMETERS);
 assert(preset_saveDrumsetSD(3,0)==FR_OK);memset(mainParameters[3],0,END_OF_SOUND_PARAMETERS);assert(preset_loadDrumsetSD(3,0)==FR_OK);
 /* Only the pre-existing LFO-target normalization may alter out-of-range bytes. */
 assert(mainParameters[3][PAR_FILTER_DRIVE_1]==255 && mainParameters[3][PAR_FILTER_FREQ_1]==255);
 assert(activeParameters[PAR_MORPH]==255); // Load never reinterprets a stored Morph field: none exists.
 bank();activeParameters[PAR_MORPH]=64;
 preset_loadDrumsetRam(1,1,0);assert(morphParameters==mainParameters[1] && activeParameters[PAR_MORPH]==64);
 memset(mainParameters[3],42,END_OF_SOUND_PARAMETERS);preset_loadDrumsetRam(3,0,0);
 assert(morphParameters==mainParameters[1] && activeParameters[PAR_MORPH]==64 && activeParameters[PAR_FILTER_FREQ_1]==42);
 assert(preset_getMorphValue(PAR_FILTER_FREQ_1,0)==42 && preset_getMorphValue(PAR_FILTER_FREQ_1,127)==127);
 puts("PASS 128 actual RAM/SD Morph Kit save/load amounts (including exact target), mod-target exceptions and unchanged 255-byte layout; 256 runtime/legacy amounts absent from temp persistence; legacy sound bytes >127 retained");
}
