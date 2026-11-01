/* R12: independent mono CLP paths. All histories and block metadata are owned
 * by the synchronous audio renderer. Shared controls are copied before each
 * call; no runtime parameter target or live smoother is modified for metadata. */
#include "Soundengine/SoloMix.h"
#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/mixer.h"
#include "IO/adcPots.h"

static uint8_t selectedMask, previousMask;
static uint32_t resetSerial;
static struct { float drive, p1, p2, p3, dryWet; uint8_t destination; } controls;
static INDTCMZ float targets[6], faderTargets[6];
static INDTCMZ OnePoleFilter faderReplay[6];
static INDTCMZ MasterDistortion paths[6];
/* Each voice is consumed before the next one is synthesized. */
static INDTCMZ float postMix[OUTPUT_DMA_SIZE], clipGain[OUTPUT_DMA_SIZE];
static INDTCMZ int16_t input[OUTPUT_DMA_SIZE * 2], isolated[OUTPUT_DMA_SIZE * 2];

void SoloMix_begin(const uint8_t routes[6], uint8_t fxType, uint8_t distortionType,
                   const float runtimeTargets[6])
{
    controls.drive = fxBus.distortion.drive;
    controls.p1 = fxBus.distortion.p1;
    controls.p2 = fxBus.distortion.p2;
    controls.p3 = fxBus.distortion.p3;
    controls.dryWet = fxBus.dryWet;
    controls.destination = fxBus.audioRouting;
    selectedMask = 0;
    if(fxType == FX_DISTORTION && distortionType == DISTORTION_RAT)
        for(unsigned v = 0; v < 6; v++)
            if(routes[v] == MIXER_ROUTING_FX_BUS) selectedMask |= 1u << v;
    /* FX/kit reset requests do not write any per-voice DSP history. New routes
     * also re-arm from quiet. Ordinary MIX/fader motion never resets history. */
    const uint32_t serial = fxBus_clipResetSerial;
    if(serial != resetSerial) { previousMask = 0; resetSerial = serial; }
    for(unsigned v = 0; v < 6; v++)
    {
        targets[v] = runtimeTargets[v];
        if((selectedMask & (1u << v)) && !(previousMask & (1u << v)))
            Distortion_resetClip(&paths[v]);
    }
    previousMask = selectedMask;
}
void SoloMix_captureFaders(const OnePoleFilter *const filters[6], const float values[6])
{
    for(unsigned v = 0; v < 6; v++)
    {
        faderReplay[v] = *filters[v];
        faderTargets[v] = values[v];
    }
}
int SoloMix_isVoice(uint8_t voice) { return !!(selectedMask & (1u << voice)); }
float SoloMix_sourceLevel(uint8_t voice, float original)
{
    return SoloMix_isVoice(voice) ? (targets[voice] == 0.f ? 0.f : 1.f) : original;
}
void SoloMix_drum(uint8_t voice, int16_t *samples, uint8_t size,
                  OnePoleFilter *mix, OnePoleFilter *fader, float *combined)
{
    for(unsigned n = 0; n < size; n++)
    {
        float m = calcOnePole(mix, targets[voice]);
        postMix[n] = targets[voice] == 0.f ? 0.f : m;
        *combined = (targets[voice] == 0.f ? 0.f : 1.f) * calcOnePole(fader, adc_getFaderVolume(voice));
        samples[n] *= *combined;
    }
}
int16_t SoloMix_snareSample(float sample)
{
    /* Finite upstream arithmetic remains a precondition, as in R11. */
    if(sample > 32767.f) return 32767;
    if(sample < -32768.f) return -32768;
    return (int16_t)sample;
}
void SoloMix_gate(uint8_t voice, int16_t *samples, uint8_t size)
{
    /* After decimation: hold history, but prevent injection at exact MIX zero. */
    if(SoloMix_isVoice(voice) && targets[voice] == 0.f)
        for(unsigned n = 0; n < size; n++) samples[n] = 0;
}
void SoloMix_addVoice(uint8_t voice, const int16_t *samples, float panL, float panR,
                       int16_t *outL, int16_t *outR, int16_t *outL2, int16_t *outR2,
                       uint16_t size)
{
    MasterDistortion *d = &paths[voice];
    d->distortionType = DISTORTION_RAT;
    d->drive = controls.drive;
    d->p1 = controls.p1;
    d->p2 = controls.p2;
    d->p3 = controls.p3;
    for(unsigned n = 0; n < size; n++)
    {
        input[2*n] = samples[n]; input[2*n+1] = 0;
        isolated[2*n] = isolated[2*n+1] = 0;
        clipGain[n] = FxBus_clipCoreGain(calcOnePole(&faderReplay[voice], faderTargets[voice]));
    }
    /* Existing mono branch: original-input detector, bass and dry/wet; audible
     * core gain remains outside lastOut. No second stereo channel is rendered. */
    Distortion_processWithClipGain(d, input, isolated, NULL, size, 1, controls.dryWet, clipGain);
    int16_t *left = outL2, *right = outR2;
    switch(controls.destination)
    {
    case MIXER_ROUTING_DAC2_STEREO: left = outL; right = outR; break;
    case MIXER_ROUTING_DAC1_R: left = outR2; break;
    case MIXER_ROUTING_DAC2_L: left = outL; break;
    case MIXER_ROUTING_DAC2_R: left = outR; break;
    default: break;
    }
    /* Match the existing mono-destination policy: PAN is used on stereo pairs,
     * while a selected single output receives the unpanned mono return. FX bus
     * destinations are explicit and do not follow direct-route jack fallback. */
    for(unsigned n = 0; n < size; n++)
    {
        const float m = targets[voice] == 0.f ? 0.f : (voice < 3 ? postMix[n] : targets[voice]);
        const int16_t sample = (int16_t)(isolated[2*n] * m);
        const int16_t l = controls.destination < MIXER_ROUTING_DAC1_L ? (int16_t)(sample * panL) : sample;
        left[n*2] = __SSAT((int32_t)left[n*2] + l, 16);
        if(controls.destination < MIXER_ROUTING_DAC1_L)
            right[n*2] = __SSAT((int32_t)right[n*2] + (int16_t)(sample * panR), 16);
    }
}
void SoloMix_process(int16_t *bus, int16_t *outL, int16_t *outR,
                     int16_t *outL2, int16_t *outR2, uint32_t size,
                     const float *unusedGain)
{
    (void)unusedGain;
    if(fxBus.fxType != FX_DISTORTION || fxBus.distortion.distortionType != DISTORTION_RAT)
        FxBus_processWithClipGain(bus, outL, outR, outL2, outR2, size, NULL);
    /* CLP returns have already been summed per voice. No shared residual core. */
    selectedMask = 0;
}
