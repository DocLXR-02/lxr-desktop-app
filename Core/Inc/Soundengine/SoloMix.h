/* Experimental independent mono CLP post-MIX paths. Synchronous, audio-owned metadata. */
#ifndef LXR_SOLO_MIX_H
#define LXR_SOLO_MIX_H
#include "Soundengine/DSPAudio/1PoleLp.h"
void SoloMix_begin(const uint8_t routes[6], uint8_t fxType, uint8_t distortionType,
                   const float targets[6]);
/* Audio block contract: voice < 6; count <= OUTPUT_DMA_SIZE; interleaved
 * output pointers address disjoint physical channels of 2*count samples. */
void SoloMix_captureFaders(const OnePoleFilter *const filters[6], const float targets[6]);
void SoloMix_addVoice(uint8_t voice, const int16_t *samples, float panL, float panR,
                       int16_t *outL, int16_t *outR, int16_t *outL2, int16_t *outR2, uint16_t size);
int SoloMix_isVoice(uint8_t voice);
float SoloMix_sourceLevel(uint8_t voice, float original);
void SoloMix_drum(uint8_t voice, int16_t *samples, uint8_t size,
                  OnePoleFilter *mix, OnePoleFilter *fader, float *combined);
int16_t SoloMix_snareSample(float sample);
void SoloMix_gate(uint8_t voice, int16_t *samples, uint8_t size);
void SoloMix_process(int16_t *input, int16_t *outL, int16_t *outR,
                     int16_t *outL2, int16_t *outR2, uint32_t size,
                     const float *clipGain);
#endif
