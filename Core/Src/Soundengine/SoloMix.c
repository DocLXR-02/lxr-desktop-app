/* Experimental single-route reference; no persistent DSP history or smoother.
 * Helpers stay in ordinary application flash. Only the audio block owns this
 * metadata; selection is refreshed after runtime modulation, before synthesis. */
#include "Soundengine/SoloMix.h"
#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/mixer.h"
#include "IO/adcPots.h"
static int selected = -1;
static float target;
static INDTCMZ float postMix[OUTPUT_DMA_SIZE];
static INDTCMZ int16_t isolated[OUTPUT_DMA_SIZE * 2];

void SoloMix_begin(const uint8_t routes[6], uint8_t fxType, uint8_t distortionType,
                   const float targets[6])
{
    selected = -1;
    if(fxType != FX_DISTORTION || distortionType != DISTORTION_RAT) return;
    unsigned count = 0;
    int candidate = -1;
    for(unsigned v = 0; v < 6; v++)
        if(routes[v] == MIXER_ROUTING_FX_BUS) { candidate = v; count++; }
    if(count != 1) return;
    selected = candidate;
    target = targets[candidate];
    /* Percussion has block-target MIX semantics. Drum values are replaced by
     * their actual live pole outputs during synthesis, once per frame. */
    for(unsigned n = 0; n < OUTPUT_DMA_SIZE; n++) postMix[n] = target;
}
int SoloMix_isVoice(uint8_t voice) { return selected == voice; }
float SoloMix_sourceLevel(uint8_t voice, float original)
{
    return SoloMix_isVoice(voice) ? (target == 0.f ? 0.f : 1.f) : original;
}
void SoloMix_drum(uint8_t voice, int16_t *samples, uint8_t size,
                  OnePoleFilter *mix, OnePoleFilter *fader, float *combined)
{
    for(unsigned n = 0; n < size; n++)
    {
        float m = calcOnePole(mix, target);
        postMix[n] = target == 0.f ? 0.f : m;
        *combined = (target == 0.f ? 0.f : 1.f) * calcOnePole(fader, adc_getFaderVolume(voice));
        samples[n] *= *combined;
    }
}
int16_t SoloMix_snareSample(float sample)
{
    /* Only the new full-source boundary. Callers must provide finite synthesis;
     * this is saturation, not a nonfinite recovery policy. */
    if(sample > 32767.f) return 32767;
    if(sample < -32768.f) return -32768;
    return (int16_t)sample;
}
void SoloMix_gate(uint8_t voice, int16_t *samples, uint8_t size)
{
    /* Deliberately after decimation: preserve held history, block its injection. */
    if(SoloMix_isVoice(voice) && target == 0.f)
        for(unsigned n = 0; n < size; n++) samples[n] = 0;
}
void SoloMix_process(int16_t *input, int16_t *outL, int16_t *outR,
                     int16_t *outL2, int16_t *outR2, uint32_t size,
                     const float *clipGain)
{
    if(selected < 0)
    {
        FxBus_processWithClipGain(input, outL, outR, outL2, outR2, size, clipGain);
        return;
    }
    /* Production block size <= OUTPUT_DMA_SIZE. Isolate before adding direct
     * routes: post MIX applies equally to dry, bass and audible core only. */
    for(unsigned n = 0; n < size * 2; n++) isolated[n] = 0;
    int16_t *other = fxBus.audioRouting < MIXER_ROUTING_DAC1_L ? isolated + 1 : isolated;
    FxBus_processWithClipGain(input, isolated, other, isolated, other, size, clipGain);
    int16_t *left = outL2, *right = outR2;
    switch(fxBus.audioRouting)
    {
    case MIXER_ROUTING_DAC2_STEREO: left = outL; right = outR; break;
    case MIXER_ROUTING_DAC1_R: left = outR2; break;
    case MIXER_ROUTING_DAC2_L: left = outL; break;
    case MIXER_ROUTING_DAC2_R: left = outR; break;
    default: break;
    }
    for(unsigned n = 0; n < size; n++)
    {
        const float m = postMix[n];
        left[n*2] = __SSAT((int32_t)left[n*2] + (int16_t)(isolated[n*2] * m), 16);
        if(fxBus.audioRouting < MIXER_ROUTING_DAC1_L)
            right[n*2] = __SSAT((int32_t)right[n*2] + (int16_t)(isolated[n*2+1] * m), 16);
    }
    selected = -1; /* No metadata survives the synchronous effect call. */
}
