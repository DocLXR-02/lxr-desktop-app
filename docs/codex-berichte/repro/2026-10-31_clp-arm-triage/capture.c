/* Capture native production filter inputs/state before expected failures. */
#include <stdio.h>
#include <stdlib.h>
#include "Soundengine/DSPAudio/ResonantFilter.h"
#include "Soundengine/DSPAudio/Snare.h"
void __real_SVF_calcBlockZDF(ResonantFilter*,const uint8_t,int16_t*,const uint8_t);
void __wrap_SVF_calcBlockZDF(ResonantFilter *f,const uint8_t type,int16_t *data,const uint8_t size){
 SVF_recalcFreq(f); /* Fixed controls: same audio-block coefficient update as production. */
 static FILE *file; if(!file){file=fopen(getenv("FILTER_CAPTURE"),"wb");if(!file)abort();}
 uint32_t header[2]={type,size};fwrite(header,sizeof header,1,file);fwrite(f,sizeof *f,1,file);fwrite(data,sizeof *data,size,file);fflush(file);
 __real_SVF_calcBlockZDF(f,type,data,size);
}

void __real_Snare_calcAsync(void);
void __wrap_Snare_calcAsync(void){if(getenv("NOISE_UI3"))snareVoice.noiseOsc.freq=(3/127.f)*22000;__real_Snare_calcAsync();}
