/*
 * mixer.c
 *
 *  Created on: 11.04.2012
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */

#include "Soundengine/DSPAudio/mixer.h"
#include "Soundengine/config.h"
#include "Soundengine/AudioCodecManager.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/HiHat.h"
#include "Soundengine/DSPAudio/BufferTools.h"
#include "Soundengine/DSPAudio/squareRootLut.h"
#include "main.h"
#include "Soundengine/TriggerOut.h"
#include "menu/menu.h"
#include "Soundengine/FxBus.h"
#include "Soundengine/SharedFxPool.h"

	uint8_t r2_Available;
	uint8_t l1_Available;
	uint8_t l2_Available;
	uint8_t r1_Available;


//-----------------------------------------------------------------------
INDTCMZ uint8_t mixer_audioRouting[6];
/* Fixed workspace: no extra Delay history and no runtime allocation. */
static INDTCMZ int16_t fxPoolInputs[FX_POOL_PROCESSOR_COUNT][OUTPUT_DMA_SIZE * 2] __attribute__((aligned(4)));
/* One zeroed return workspace prevents FX modules from reading other FX outputs. */
static INDTCMZ int16_t fxPoolReturn[2][OUTPUT_DMA_SIZE * 2] __attribute__((aligned(4)));
/* Fail compilation if the original FX enum and pool routing ever diverge. */
typedef char fx_pool_type_values_match[
    (FX_OFF == FX_POOL_TYPE_OFF && FX_DISTORTION == FX_POOL_TYPE_DISTORTION &&
     FX_RING == FX_POOL_TYPE_RING && FX_COMPRESSOR == FX_POOL_TYPE_COMPRESSOR &&
     FX_DELAY == FX_POOL_TYPE_DELAY) ? 1 : -1];
//-----------------------------------------------------------------------
#if USE_DECIMATOR
INDTCMZ float mixer_decimation_rate[7];		/**<sets the sample rate decimation. 0..1 = full rate*/
INDTCMZ float mixer_decimation_cnt[6];		/**<s'n'h counter for decimator*/
INDTCMZ int16_t mixer_voice_samples[6];		/**< stores the last outputted sample of the 6 voices*/
#endif
//-----------------------------------------------------------------------
void mixer_init()
{
#if USE_DECIMATOR
	int i;
	for(i=0;i<6;i++)
	{
		mixer_decimation_rate[i] 	= 1;
		mixer_decimation_cnt[i] 	= 0;
		mixer_voice_samples[i] 		= 0;
		mixer_audioRouting[i]		= 0;
	}
	mixer_decimation_rate[6] 		= 1;
#endif

	FxBus_init();
}
//-----------------------------------------------------------------------
IN_ITCM_RAM void mixer_decimateBlock(const uint8_t voiceNr, int16_t* buffer)
{
	uint8_t i;
	float phaseInc = mixer_decimation_rate[voiceNr]*mixer_decimation_rate[6];
	for(i=0;i<OUTPUT_DMA_SIZE;i++)
	{
		mixer_decimation_cnt[voiceNr] += phaseInc;
		if(mixer_decimation_cnt[voiceNr] >= 1.f)
		{
			mixer_decimation_cnt[voiceNr] -= 1.f;
			mixer_voice_samples[voiceNr] = buffer[i];

		}
		buffer[i] = mixer_voice_samples[voiceNr];
	}
}
//#pragma GCC push_options
//#pragma GCC optimize ("O0")
//-----------------------------------------------------------------------
/*IN_ITCM_RAM*/ uint8_t mixer_checkOutJackAvailable(uint8_t dest)
{
	//read input pins
	//TODO move pin reading in low priority idle loop
	/*
	 #define switch_l1_Pin GPIO_PIN_6
#define switch_l1_GPIO_Port GPIOD
#define switch_r1_Pin GPIO_PIN_7
#define switch_r1_GPIO_Port GPIOD
#define switch_l2_Pin GPIO_PIN_3
#define switch_l2_GPIO_Port GPIOB
#define switch_r2_Pin GPIO_PIN_4
#define switch_r2_GPIO_Port GPIOB
	 */


	//return MIXER_ROUTING_DAC2_STEREO;

	switch(dest)
	{
        case MIXER_ROUTING_FX_DELAY:
        case MIXER_ROUTING_FX_RING:
        case MIXER_ROUTING_FX_DISTORTION:
        case MIXER_ROUTING_FX_COMPRESSOR:
            return dest;

		case MIXER_ROUTING_DAC1_STEREO:
			if(r1_Available && l1_Available) {
				return MIXER_ROUTING_DAC1_STEREO;
			} else
			if (r2_Available && l2_Available) {
				return MIXER_ROUTING_DAC2_STEREO;
			}
			else if(r1_Available) {
				return MIXER_ROUTING_DAC1_R;
			}else if(l1_Available) {
				return MIXER_ROUTING_DAC1_L;
			} else
			if (r2_Available) {
				return MIXER_ROUTING_DAC2_R;
			}else
			if (l2_Available) {
				return MIXER_ROUTING_DAC2_L;
			}
			break;

		case MIXER_ROUTING_DAC2_STEREO:

			if (r2_Available && l2_Available) {
				return MIXER_ROUTING_DAC2_STEREO;
			} else
			if(r1_Available && l1_Available) {
				return MIXER_ROUTING_DAC1_STEREO;
			}
			else
			if (r2_Available) {
				return MIXER_ROUTING_DAC2_R;
			}else
			if (l2_Available) {
				return MIXER_ROUTING_DAC2_L;
			}else if(r1_Available) {
				return MIXER_ROUTING_DAC1_R;
			}else if(l1_Available) {
				return MIXER_ROUTING_DAC1_L;
			}
			break;

		case MIXER_ROUTING_DAC1_L:
			if(l1_Available) {
				return dest;
			} else if (r1_Available) {
				return MIXER_ROUTING_DAC1_R;
			} else if (l2_Available) {
				return MIXER_ROUTING_DAC2_L;
			} else if (r2_Available) {
				return MIXER_ROUTING_DAC2_R;
			}
			break;

		case MIXER_ROUTING_DAC1_R:
			if(r1_Available) {
				return dest;
			} else if (l1_Available) {
				return MIXER_ROUTING_DAC1_L;
			} else if (l2_Available) {
				return MIXER_ROUTING_DAC2_L;
			} else if (r2_Available) {
				return MIXER_ROUTING_DAC2_R;
			}
			break;

		case MIXER_ROUTING_DAC2_L:
			if(l2_Available) {
				return dest;
			} else if (r2_Available) {
				return MIXER_ROUTING_DAC2_R;
			} else if (l1_Available) {
				return MIXER_ROUTING_DAC1_L;
			} else if (r1_Available) {
				return MIXER_ROUTING_DAC1_R;
			}
			break;

		case MIXER_ROUTING_DAC2_R:
			if(r2_Available) {
				return dest;
			} else if (l2_Available) {
				return MIXER_ROUTING_DAC2_L;
			} else if (r1_Available) {
				return MIXER_ROUTING_DAC1_R;
			} else if (l1_Available) {
				return MIXER_ROUTING_DAC1_L;
			}
			break;
	}
	return dest;
}

//-----------------------------------------------------------------------
#if 0
static inline void mixer_moveDataToOutput(uint8_t dest, const float panL, const float panR, int16_t* data,int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2)
{
	//check if a cable is in the selected out
	dest = mixer_checkOutJackAvailable(dest);

	uint8_t i;
	switch(dest)
	{

	case MIXER_ROUTING_DAC1_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL2 = data[i] * panL;
			outL2 += 2;

			*outR2 = data[i] * panR;
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL = data[i] * panL;
			outL += 2;

			*outR = data[i] * panR;
			outR += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL2 = data[i];
			outL2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR2 = data[i];
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL = data[i];
			outL += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR = data[i];
			outR += 2;
		}
		break;
	}
}
#endif
//-----------------------------------------------------------------------
#if 0
static float fastTanh(float var)
{
   if(var < -1.95f)     return -1.0f;
   else if(var > 1.95f) return  1.0f;
   else					return  4.15f*var/(4.29f+var*var);
}




double ecc83(float x, float v)
{
	/*
	double input = x -0.05*v*x +0.01*v;//(h100f(x -0.05*v*x +0.01*v))*(1.0+0.55*v) +0.55*v;
	input += 1.0; // [0:2]
	input *= 64.0;
	uint8_t itg = input;
	float frac = input-itg;
	double val;
	if(itg<127)
		val = (1.0-frac) * ecc83Table[itg] + frac * ecc83Table[itg+1];
	else val = ecc83Table[127];
	val = val *(1.0+0.55*v) +0.55*v;
	//if(val>0.999999)val =0.999999;
	return val;
	*/
}

void mixer_saturateOutputs(float gain,  int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2)
{
    gain = 1+gain*2;
    //float v = gain
	for(uint32_t i=0;i<OUTPUT_DMA_SIZE;i++)
	{
		/*
		*outR = __SSAT((int32_t)(ecc83(*outR/32768.0f, gain) * 32768.0f), 16); ;
		outR += 2;

		*outL =  __SSAT((int32_t)(ecc83(*outL/32768.0f, gain) * 32768.0f), 16); ;
		outL += 2;
		*/

		*outR2 =  __SSAT((int32_t)(ecc83(*outR2/32768.0f*gain, 1.) * 32768.0f), 16); ;
		outR2 += 2;

		*outL2 =  __SSAT((int32_t)(ecc83(*outL2/32768.0f*gain, 1.) * 32768.0f), 16); ;
		outL2 += 2;
	}
}
#endif
//-----------------------------------------------------------------------
IN_ITCM_RAM static inline void mixer_addDataToOutputFilteredPan(uint8_t dest, const float pan,  int16_t* data,int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2, int16_t *fxBuffer, OnePoleFilter* filter)
{


	//check if a cable is in the selected out
	dest = mixer_checkOutJackAvailable(dest);
	const uint8_t selectedType = SharedFxPool_typeForRoute(dest);
	if (selectedType != FX_POOL_INVALID)
		fxBuffer = fxPoolInputs[selectedType];
	//assert_param(dest <= MIXER_ROUTING_DAC2_R);

	//TODO may be possible to optimize here using both halfwords of add for stereo mixing
	uint8_t i;
	switch(dest)
	{

	case MIXER_ROUTING_FX_DELAY:
	case MIXER_ROUTING_FX_RING:
	case MIXER_ROUTING_FX_DISTORTION:
	case MIXER_ROUTING_FX_COMPRESSOR:
#if 0
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			fxBuffer[i] = __QADD16(fxBuffer[i],data[i]) & 0xFFFF;
		}
#else
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{

			//----
			float mixPan 	= calcOnePole(filter, pan);
			float panL = sqrtf((127-mixPan)/127.f);
			float panR = sqrtf(mixPan/127.f);
			//----
			int32_t y;
			int32_t x;
			y = *((int32_t*)fxBuffer);
			x = __PKHBT( (int16_t)(data[i] * panL), (int16_t)(data[i] * panR), 16);

			*((int32_t*)fxBuffer) = __QADD16(x, y);
			fxBuffer += 2;
		}
#endif
		break;

	case MIXER_ROUTING_DAC1_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			//----
			float mixPan 	= calcOnePole(filter, pan);
			float panL = sqrtf((127-mixPan)/127.f);
			float panR = sqrtf(mixPan/127.f);
			//----
#if 1
			int32_t y;
			int32_t x;
			y = *((int32_t*)outL2);
			//x = ((int16_t)(data[i] * panL)& 0xFFFF) | (((int16_t)(data[i] * panR)& 0xFFFF)<<16);
			x = __PKHBT( (int16_t)(data[i] * panL), (int16_t)(data[i] * panR), 16);
			*((int32_t*)outL2) = __QADD16(x, y);
			outL2 += 2;
#else


			*outL2 = __QADD16(*outL2,(int16_t)(data[i] * panL)) & 0xFFFF;
			outL2 += 2;

			*outR2 = __QADD16(*outR2,(int16_t)(data[i] * panR)) & 0xFFFF;
			outR2 += 2;
#endif
		}
		break;
	case MIXER_ROUTING_DAC2_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			//----
			float mixPan 	= calcOnePole(filter, pan);
			float panL = sqrtf((127-mixPan)/127.f);
			float panR = sqrtf(mixPan/127.f);
			//----
#if 1
			int32_t x,y;
			y = *((int32_t*)outL);
			//simdBuffer x;
			//x = ((int16_t)(data[i] * panL)& 0xFFFF) | (((int16_t)(data[i] * panR)& 0xFFFF)<<16);
			//*((int32_t*)outL) = __SADD16(x, y);
			//x.word[0] = data[i] * panL;
			//x.word[1] = data[i] * panR;
			x = __PKHBT( (int16_t)(data[i] * panL), (int16_t)(data[i] * panR), 16);
			//x = __PKHBT(data[i] * panL, data[i] * panR, 16);
			*((int32_t*)outL) = __QADD16(x, y);
			outL += 2;
#else
			*outL = __QADD16(*outL,(int16_t)(data[i] * panL)) & 0xFFFF;
			outL += 2;

			*outR = __QADD16(*outR,(int16_t)(data[i] * panR)) & 0xFFFF;
			outR += 2;
#endif
		}
		break;
	case MIXER_ROUTING_DAC1_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{

			*outL2 = __QADD16(*outL2,data[i]);
			outL2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR2 = __QADD16(*outR2,data[i]) ;
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL = __QADD16(*outL,data[i]);
			outL += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR = __QADD16(*outR,data[i]);
			outR += 2;
		}
		break;

	}
}
//-----------------------------------------------------------------------
IN_ITCM_RAM static __attribute__((noinline)) void mixer_addDataToOutput(uint8_t dest, const float panL, const float panR,  int16_t* data,int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2, int16_t *fxBuffer)
{
	//check if a cable is in the selected out
	dest = mixer_checkOutJackAvailable(dest);

    const uint8_t selectedType = SharedFxPool_typeForRoute(dest);
    if (selectedType != FX_POOL_INVALID)
        fxBuffer = fxPoolInputs[selectedType];
	//assert_param(dest <= MIXER_ROUTING_DAC2_R);

	//TODO may be possible to optimize here using both halfwords of add for stereo mixing
	uint8_t i;
	switch(dest)
	{

    case MIXER_ROUTING_FX_DELAY:
    case MIXER_ROUTING_FX_RING:
    case MIXER_ROUTING_FX_DISTORTION:
    case MIXER_ROUTING_FX_COMPRESSOR:
#if 0
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			fxBuffer[i] = __QADD16(fxBuffer[i],data[i]) & 0xFFFF;
		}
#else
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			int32_t y;
			int32_t x;
			y = *((int32_t*)fxBuffer);
			x = __PKHBT( (int16_t)(data[i] * panL), (int16_t)(data[i] * panR), 16);

			*((int32_t*)fxBuffer) = __QADD16(x, y);
			fxBuffer += 2;
		}
#endif
		break;

	case MIXER_ROUTING_DAC1_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
#if 1
			int32_t y;
			int32_t x;
			y = *((int32_t*)outL2);
			//x = ((int16_t)(data[i] * panL)& 0xFFFF) | (((int16_t)(data[i] * panR)& 0xFFFF)<<16);
			x = __PKHBT( (int16_t)(data[i] * panL), (int16_t)(data[i] * panR), 16);
			*((int32_t*)outL2) = __QADD16(x, y);
			outL2 += 2;
#else


			*outL2 = __QADD16(*outL2,(int16_t)(data[i] * panL)) & 0xFFFF;
			outL2 += 2;

			*outR2 = __QADD16(*outR2,(int16_t)(data[i] * panR)) & 0xFFFF;
			outR2 += 2;
#endif
		}
		break;
	case MIXER_ROUTING_DAC2_STEREO:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
#if 1
			int32_t x,y;
			y = *((int32_t*)outL);
			//simdBuffer x;
			//x = ((int16_t)(data[i] * panL)& 0xFFFF) | (((int16_t)(data[i] * panR)& 0xFFFF)<<16);
			//*((int32_t*)outL) = __SADD16(x, y);
			//x.word[0] = data[i] * panL;
			//x.word[1] = data[i] * panR;
			x = __PKHBT( (int16_t)(data[i] * panL), (int16_t)(data[i] * panR), 16);
			//x = __PKHBT(data[i] * panL, data[i] * panR, 16);
			*((int32_t*)outL) = __QADD16(x, y);
			outL += 2;
#else
			*outL = __QADD16(*outL,(int16_t)(data[i] * panL)) & 0xFFFF;
			outL += 2;

			*outR = __QADD16(*outR,(int16_t)(data[i] * panR)) & 0xFFFF;
			outR += 2;
#endif
		}
		break;
	case MIXER_ROUTING_DAC1_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL2 = __QADD16(*outL2,data[i]);
			outL2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC1_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR2 = __QADD16(*outR2,data[i]) ;
			outR2 += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_L:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outL = __QADD16(*outL,data[i]);
			outL += 2;
		}
		break;
	case MIXER_ROUTING_DAC2_R:
		for(i=0;i<OUTPUT_DMA_SIZE;i++)
		{
			*outR = __QADD16(*outR,data[i]);
			outR += 2;
		}
		break;

	}
}
//#pragma GCC pop_options
//-----------------------------------------------------------------------
/* R14: select the highest *existing* smoothed fader among FX routes.
 * This block-rate control calculation stays in flash rather than ITCM.
 * An unrouted CLP retains R13's unity-fader fallback; the R5 idle gate remains.
 * For one route, the resulting R13 feedback curve is unchanged.
 */
static __attribute__((noinline)) float mixer_clipMaxRoutedFaderGain(const uint8_t routes[FX_POOL_VOICE_COUNT])
{
    float maxQ = -1.f;
    if(SharedFxPool_typeForRoute(routes[0]) == FX_DISTORTION &&
       voiceArray[0].FaderVolumeFilter.b > maxQ)
        maxQ = voiceArray[0].FaderVolumeFilter.b;
    if(SharedFxPool_typeForRoute(routes[1]) == FX_DISTORTION &&
       voiceArray[1].FaderVolumeFilter.b > maxQ)
        maxQ = voiceArray[1].FaderVolumeFilter.b;
    if(SharedFxPool_typeForRoute(routes[2]) == FX_DISTORTION &&
       voiceArray[2].FaderVolumeFilter.b > maxQ)
        maxQ = voiceArray[2].FaderVolumeFilter.b;
    if(SharedFxPool_typeForRoute(routes[3]) == FX_DISTORTION &&
       snareVoice.FaderVolumeFilter.b > maxQ)
        maxQ = snareVoice.FaderVolumeFilter.b;
    if(SharedFxPool_typeForRoute(routes[4]) == FX_DISTORTION &&
       cymbalVoice.FaderVolumeFilter.b > maxQ)
        maxQ = cymbalVoice.FaderVolumeFilter.b;
    if(SharedFxPool_typeForRoute(routes[5]) == FX_DISTORTION &&
       hatVoice.FaderVolumeFilter.b > maxQ)
        maxQ = hatVoice.FaderVolumeFilter.b;

    if(maxQ < 0.f) return 1.f; /* No FX route: retain R13 fallback. */
    if(maxQ <= 0.f) return 0.f;
    if(maxQ >= .25f) return 1.f;
    return 2.f * sqrtf(maxQ);
}

IN_ITCM_RAM void mixer_calcNextSampleBlock(int16_t* output,int16_t* output2)
{

	r2_Available = (switch_r2_GPIO_Port->IDR & switch_r2_Pin)>0;
	l1_Available = (switch_l1_GPIO_Port->IDR & switch_l1_Pin)>0;
	l2_Available = (switch_l2_GPIO_Port->IDR & switch_l2_Pin)>0;
	r1_Available = (switch_r1_GPIO_Port->IDR & switch_r1_Pin)>0;

	modNode_resetTargets();
	//re assign velocity modulation
	modNode_reassignVeloMod();

	//calc and dispatch LFO
	lfo_dispatchNextValue(&voiceArray[0].lfo);
	lfo_dispatchNextValue(&voiceArray[1].lfo);
	lfo_dispatchNextValue(&voiceArray[2].lfo);
	lfo_dispatchNextValue(&snareVoice.lfo);
	lfo_dispatchNextValue(&cymbalVoice.lfo);
	lfo_dispatchNextValue(&hatVoice.lfo);

	//update filter frequencies
	SVF_recalcFreq(&voiceArray[0].filter);
	SVF_recalcFreq(&voiceArray[1].filter);
	SVF_recalcFreq(&voiceArray[2].filter);
	SVF_recalcFreq(&snareVoice.filter);
	SVF_recalcFreq(&cymbalVoice.filter);
	SVF_recalcFreq(&hatVoice.filter);

	//--- Calc async -----
	calcDrumVoiceAsync(0);
	calcDrumVoiceAsync(1);
	calcDrumVoiceAsync(2);

	Snare_calcAsync();
	Cymbal_calcAsync();
	HiHat_calcAsync();

	//calculate trigger io phase
	//trigger_tickPhaseCounter();

	//an array to store intermediate voice samples
	//befor output distribution
	int16_t sampleData[OUTPUT_DMA_SIZE];
    /* Fix voice-to-FX routing for this block after velocity/LFO updates. */
    uint8_t routesForBlock[FX_POOL_VOICE_COUNT];
    for (uint8_t voice = 0; voice < FX_POOL_VOICE_COUNT; ++voice)
        routesForBlock[voice] = mixer_audioRouting[voice];
    const uint8_t fxPoolMask = SharedFxPool_activeMask(routesForBlock);
	/* R14: evaluate shared MAX-feedback control before voice rendering. */
	const uint8_t clpFeedbackActive = (fxPoolMask & (1u << FX_DISTORTION)) != 0 &&
		fxBus.distortion.distortionType == DISTORTION_RAT;
	const float clpFeedbackFirst = clpFeedbackActive ? mixer_clipMaxRoutedFaderGain(routesForBlock) : 1.f;


	const uint8_t pos = 0;

	//Clear is necessary, otherwise we keep trash data on non routed outputs
	bufferTool_clearBuffer(output,OUTPUT_DMA_SIZE*2);
	bufferTool_clearBuffer(output2,OUTPUT_DMA_SIZE*2);

	for (uint8_t fxType = FX_OFF; fxType <= FX_DELAY; ++fxType) {
        if (fxPoolMask & (1u << fxType))
            bufferTool_clearBuffer(fxPoolInputs[fxType], OUTPUT_DMA_SIZE * 2);
    }

	//calc voice 1
	calcDrumVoiceSyncBlock(0, sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(0,sampleData);
	//copy to selected dma buffer
#if USE_SQRT_LUT
	mixer_addDataToOutput(routesForBlock[0],squareRootLut[127-voiceArray[0].pan] ,squareRootLut[voiceArray[0].pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else

#if 0//SMOOTH_MIXER_VOL_PAN_DRIVE
	//float mixPan 	= calcOnePole( &voiceArray[0].MixerPageFilter[1], voiceArray[0].pan);
	//mixer_addDataToOutput(routesForBlock[0],sqrtf((127-mixPan)/127.f) ,sqrtf(mixPan/127.f), sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
	mixer_addDataToOutputFilteredPan(routesForBlock[0], voiceArray[0].pan, sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0], &voiceArray[1].MixerPageFilter[0]);
#else

#if USE_TRIGGERED_PAN
	mixer_addDataToOutput(routesForBlock[0],voiceArray[0].triggeredPanL  ,voiceArray[0].triggeredPanR , sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
	mixer_addDataToOutput(routesForBlock[0],sqrtf((127-voiceArray[0].pan)/127.f) ,sqrtf(voiceArray[0].pan/127.f), sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#endif
#endif
#endif

#if 1

	//calc voice 2
	calcDrumVoiceSyncBlock(1, sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(1,sampleData);
	//copy to selected dma buffer
#if USE_SQRT_LUT
	mixer_addDataToOutput(routesForBlock[1],squareRootLut[127-voiceArray[1].pan] ,squareRootLut[voiceArray[1].pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
#if 0//SMOOTH_MIXER_VOL_PAN_DRIVE
	//mixPan 	= calcOnePole( &voiceArray[1].MixerPageFilter[1], voiceArray[1].pan);
	//mixer_addDataToOutput(routesForBlock[1],sqrtf((127-mixPan)/127.f) ,sqrtf(mixPan/127.f), sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
	mixer_addDataToOutputFilteredPan(routesForBlock[1], voiceArray[1].pan, sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0], &voiceArray[1].MixerPageFilter[1]);
#else
#if USE_TRIGGERED_PAN
	mixer_addDataToOutput(routesForBlock[1],voiceArray[1].triggeredPanL  ,voiceArray[1].triggeredPanR , 			sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
	mixer_addDataToOutput(routesForBlock[1],sqrtf((127-voiceArray[1].pan)/127.f) ,sqrtf(voiceArray[1].pan/127.f), 	sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#endif
#endif
#endif
	//calc voice 3
	calcDrumVoiceSyncBlock(2, sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(2,sampleData);
	//copy to selected dma buffer
#if USE_SQRT_LUT
	mixer_addDataToOutput(routesForBlock[2],squareRootLut[127-voiceArray[2].pan] ,squareRootLut[voiceArray[2].pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
#if 0//SMOOTH_MIXER_VOL_PAN_DRIVE

	mixer_addDataToOutputFilteredPan(routesForBlock[2], voiceArray[2].pan, sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0], &voiceArray[2].MixerPageFilter[1]);
#else
#if USE_TRIGGERED_PAN
	mixer_addDataToOutput(routesForBlock[2],voiceArray[2].triggeredPanL  ,voiceArray[2].triggeredPanR , 			sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
	mixer_addDataToOutput(routesForBlock[2],sqrtf((127-voiceArray[2].pan)/127.f) ,sqrtf(voiceArray[2].pan/127.f), 	sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#endif
#endif
#endif
	//calc snare
	Snare_calcSyncBlock(sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(3,sampleData);
	//copy to selected dma buffer
#if USE_SQRT_LUT
	mixer_addDataToOutput(routesForBlock[3],squareRootLut[127-snareVoice.pan], squareRootLut[snareVoice.pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else

#if USE_TRIGGERED_PAN
	mixer_addDataToOutput(routesForBlock[3],snareVoice.triggeredPanL  ,snareVoice.triggeredPanR , 			sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
	mixer_addDataToOutput(routesForBlock[3],sqrtf((127-snareVoice.pan)/127.f), sqrtf(snareVoice.pan/127.f), sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#endif
#endif
	//calc cymbal
	Cymbal_calcSyncBlock(sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(4,sampleData);
	//copy to selected dma buffer
#if USE_SQRT_LUT
	mixer_addDataToOutput(routesForBlock[4],squareRootLut[127-cymbalVoice.pan],	squareRootLut[cymbalVoice.pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
#if USE_TRIGGERED_PAN
	mixer_addDataToOutput(routesForBlock[4],cymbalVoice.triggeredPanL  ,cymbalVoice.triggeredPanR , 			sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
	mixer_addDataToOutput(routesForBlock[4],sqrtf((127-cymbalVoice.pan)/127.f),	sqrtf(cymbalVoice.pan/127.f), sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#endif
#endif


	//calc HiHat
	HiHat_calcSyncBlock(sampleData,OUTPUT_DMA_SIZE);
	//decimate voice
	mixer_decimateBlock(5,sampleData);
	//copy to selected dma buffer
#if USE_SQRT_LUT
	mixer_addDataToOutput(routesForBlock[5],squareRootLut[127-hatVoice.pan],squareRootLut[hatVoice.pan], sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
#if USE_TRIGGERED_PAN
	mixer_addDataToOutput(routesForBlock[5],hatVoice.triggeredPanL  ,hatVoice.triggeredPanR , 			sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#else
	mixer_addDataToOutput(routesForBlock[5],sqrtf((127-hatVoice.pan)/127.f),sqrtf(hatVoice.pan/127.f), sampleData,&output[pos],&output[pos+1],&output2[pos],&output2[pos+1], &fxPoolInputs[0][0]);
#endif
#endif

#endif
	/* R14: preserve R13's 64-frame ramp; only the fader selector changes. */
	fxBus.distortion.clipFaderFeedbackFirst = clpFeedbackFirst;
	if(clpFeedbackActive)
	{
		const float last = mixer_clipMaxRoutedFaderGain(routesForBlock);
		fxBus.distortion.clipFaderFeedbackStep =
			(last - clpFeedbackFirst) / (OUTPUT_DMA_SIZE - 1);
	}
	else
	{
		fxBus.distortion.clipFaderFeedbackStep = 0.f;
	}
	/* A shared processor is invoked at most once, regardless of voice count. */
    if (!(fxPoolMask & (1u << FX_DISTORTION)) && fxBus.distortion.clipStateActive)
        Distortion_resetClip(&fxBus.distortion);
    /* Preserve the R22 single-effect mix path and avoid extra return copies. */
    const uint8_t multipleFx = (fxPoolMask & (fxPoolMask - 1u)) != 0u;
    for (uint8_t fxType = FX_OFF; fxType <= FX_DELAY; ++fxType) {
        if (!(fxPoolMask & (1u << fxType)))
            continue;
        if (!multipleFx) {
            FxBus_processSelected(fxType, fxPoolInputs[fxType],
                                  &output[pos], &output[pos+1],
                                  &output2[pos], &output2[pos+1], OUTPUT_DMA_SIZE);
            continue;
        }

        bufferTool_clearBuffer(fxPoolReturn[0], OUTPUT_DMA_SIZE * 2);
        bufferTool_clearBuffer(fxPoolReturn[1], OUTPUT_DMA_SIZE * 2);
        FxBus_processSelected(fxType, fxPoolInputs[fxType],
                              &fxPoolReturn[0][0], &fxPoolReturn[0][1],
                              &fxPoolReturn[1][0], &fxPoolReturn[1][1],
                              OUTPUT_DMA_SIZE);

        /* Mix returned stereo frames after processing, not inside other FX DSP. */
        int32_t *dac1 = (int32_t *)&output2[pos];
        int32_t *dac2 = (int32_t *)&output[pos];
        const int32_t *ret1 = (const int32_t *)fxPoolReturn[1];
        const int32_t *ret2 = (const int32_t *)fxPoolReturn[0];
        for (uint8_t frame = 0; frame < OUTPUT_DMA_SIZE; ++frame) {
            dac1[frame] = __QADD16(dac1[frame], ret1[frame]);
            dac2[frame] = __QADD16(dac2[frame], ret2[frame]);
        }
    }
/*
	if(activeParameters[UNUSED_PAR_GLO_SAT])
	{
		mixer_saturateOutputs(activeParameters[UNUSED_PAR_GLO_SAT]/127.f, &output[pos],&output[pos+1],&output2[pos],&output2[pos+1]);
	}
	*/


}
