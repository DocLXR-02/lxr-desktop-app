/*
 * FxBus.c
 *
 *  Created on: 26.08.2020
 *      Author: julian
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */


#include "Soundengine/FxBus.h"
#include "Soundengine/DSPAudio/BufferTools.h"
#include "Soundengine/DSPAudio/mixer.h"
#include <string.h>
INDTCMZ FxBus fxBus;

void FxBus_init()
{
	fxBus.dryWet = 0;
	fxBus.audioRouting = 0;
	fxBus.fxType = FX_OFF;

	/* No prior kit-format migration is performed in the core-only phase. */
	memset(fxBus.bankParameters, 0, sizeof(fxBus.bankParameters));
	memset(fxBus.bankDryWet, 0, sizeof(fxBus.bankDryWet));
	memset(fxBus.bankAudioRouting, 0, sizeof(fxBus.bankAudioRouting));
	fxBus.parameterBankType = FX_OFF;

	fxBus.delay.delayType = DELAY_NORMAL;

	Distortion_init(&fxBus.distortion);
	Ringmod_init(&fxBus.ringmod);
	Compressor_init(&fxBus.compressor);
	Delay_init(&fxBus.delay);



#if USE_REVERB
	Reverb_init(&fxBus.reverb);
#endif
}

IN_ITCM_RAM void FxBus_processSelected(uint8_t selectedType, int16_t *fxBuffer, int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2, uint32_t size)
{
	/* The processing effect, not the selected editor page, owns output and wet mix. */
	const uint8_t routeType = selectedType < FX_PARAMETER_BANK_COUNT ? selectedType : FX_OFF;
	const uint8_t selectedRoute = fxBus.bankAudioRouting[routeType];
	const float selectedWet = fxBus.bankDryWet[routeType];
	int16_t* out;
	int16_t* out2 = NULL;
	// < MIXER_ROUTING_DAC1_L -> stereo 1 or 2
	const uint8_t channels = selectedRoute < MIXER_ROUTING_DAC1_L ? 2 : 1;
	switch(selectedRoute)
	{
	default:
	case MIXER_ROUTING_DAC1_STEREO:
		out = outL2;
		out2 = outR2;
		break;
	case MIXER_ROUTING_DAC2_STEREO:
		out = outL;
		out2 = outR;
		break;
	case MIXER_ROUTING_DAC1_L:
		out = outL2;
		break;
	case MIXER_ROUTING_DAC1_R:
		out = outR2;
		break;
	case MIXER_ROUTING_DAC2_L:
		out = outL;
		break;
	case MIXER_ROUTING_DAC2_R:
		out = outR;
		break;

	}

	switch(selectedType)
	{
	default:
	case FX_OFF:
	{
		if(channels == 2)
		{
			int32_t* stereoIn = (int32_t*)fxBuffer;
			int32_t* stereoOut = (int32_t*)out;

			for(uint8_t i=0;i<size;i++)
			{
				//add stereo samples using SIMD
				*stereoOut = __QADD16(*stereoOut, *stereoIn);
				stereoOut++;
				stereoIn++;
			}
		}
		else
		{
			int32_t* in = (int32_t*)fxBuffer;
			for(uint8_t i=0;i<size;i++)
			{
				*out =  __QADD16(*out, *in);
				out +=2;
				in++;

			}
		}
	}
	break;

	case FX_DISTORTION:
		Distortion_process(&fxBus.distortion, fxBuffer, out, out2, size, channels, selectedWet);
		break;

	case FX_RING:
		Ringmod_process(&fxBus.ringmod, fxBuffer, out, out2, size, channels, selectedWet);
		break;

	case FX_COMPRESSOR:
		Compressor_process(&fxBus.compressor, fxBuffer, out, out2, size, channels, selectedWet);
		break;

	case FX_DELAY:
		if(fxBus.delay.delayType == DELAY_NORMAL)
		{
			Delay_process(&fxBus.delay, fxBuffer, out, out2, size, channels, selectedWet);
		}
		else
		{
			Delay_processPingPong(&fxBus.delay, fxBuffer, out, out2, size, channels, selectedWet);
		}
		break;
#if USE_REVERB
	case FX_REVERB:
		Reverb_process(&fxBus.reverb, fxBuffer, out, out2, size, channels, selectedWet);
		break;
#endif

	}

}


/* Compatibility entry for callers expecting the original single-FX bus. */
void FxBus_process(int16_t *fxBuffer, int16_t* outL,int16_t* outR,
                   int16_t* outL2,int16_t* outR2,uint32_t size)
{
    if (fxBus.fxType != FX_DISTORTION && fxBus.distortion.clipStateActive)
        Distortion_resetClip(&fxBus.distortion);
    FxBus_processSelected(fxBus.fxType, fxBuffer, outL, outR, outL2, outR2, size);
}

void FxBus_prepareFxChange()
{
	/* Re-entry to CLP starts from quiet runtime history, including same-type recalls. */
	Distortion_resetClip(&fxBus.distortion);
	//memset(FX_BUFFER, 0, FX_BUFFER_SIZE*sizeof(__fp16) );
//	Ringmod_resetPhase(&fxBus.ringmod);
}

/* The legacy four-value edit buffer follows the selected FX type.
 * Keep banks separate even when the menu/MIDI system writes the legacy view.
 */
static void FxBus_syncSelectedBank(void)
{
	const uint8_t nextType = fxBus.fxType;
	if (nextType >= FX_PARAMETER_BANK_COUNT)
		return;
	const uint8_t previousType = fxBus.parameterBankType;
	if (previousType != nextType)
	{
		if (previousType < FX_PARAMETER_BANK_COUNT)
		{
			memcpy(fxBus.bankParameters[previousType], fxBus.parameter,
			       sizeof(fxBus.parameter));
			fxBus.bankDryWet[previousType] = fxBus.dryWet;
			fxBus.bankAudioRouting[previousType] = fxBus.audioRouting;
		}
		memcpy(fxBus.parameter, fxBus.bankParameters[nextType],
		       sizeof(fxBus.parameter));
		fxBus.dryWet = fxBus.bankDryWet[nextType];
		fxBus.audioRouting = fxBus.bankAudioRouting[nextType];
		fxBus.parameterBankType = nextType;
	}
	else
	{
		memcpy(fxBus.bankParameters[nextType], fxBus.parameter,
		       sizeof(fxBus.parameter));
		fxBus.bankDryWet[nextType] = fxBus.dryWet;
		fxBus.bankAudioRouting[nextType] = fxBus.audioRouting;
	}
}

static void FxBus_applyBank(uint8_t type)
{
	if (type >= FX_PARAMETER_BANK_COUNT)
		return;
	const float *const p = fxBus.bankParameters[type];
	switch (type)
	{
	case FX_OFF:
		break;
	case FX_DISTORTION:
		Distortion_setDrive(&fxBus.distortion, p[0]);
		Distortion_setParam1(&fxBus.distortion, p[1]);
		Distortion_setParam2(&fxBus.distortion, p[2]);
		Distortion_setParam3(&fxBus.distortion, p[3]);
		break;
	case FX_RING:
		Ringmod_setFreq(&fxBus.ringmod, p[0]);
		break;
	case FX_COMPRESSOR:
		Compressor_setAttack(&fxBus.compressor, p[0]);
		Compressor_setRelease(&fxBus.compressor, p[1]);
		Compressor_setTreshold(&fxBus.compressor, p[2]);
		Compressor_setGain(&fxBus.compressor, p[3]);
		break;
	case FX_DELAY:
		Delay_setRange(&fxBus.delay, p[1]);
		Delay_setDelayTime(&fxBus.delay, p[0]);
		Delay_setFilter(&fxBus.delay, p[2]);
		Delay_setFeedback(&fxBus.delay, p[3]);
		break;
#if USE_REVERB
	case FX_REVERB:
		Reverb_setDelayTime(&fxBus.reverb, p[0]);
		Reverb_setFeedback(&fxBus.reverb, p[1]);
		Reverb_setComposition(&fxBus.reverb, p[2]);
		Reverb_setVariation(&fxBus.reverb, p[3]);
		break;
#endif
	default:
		break;
	}
}

void FxBus_updateParameter()
{
	FxBus_syncSelectedBank();
	FxBus_applyBank(fxBus.fxType);
}

void FxBus_setParameter(uint8_t idx, float value)
{
	if (idx >= FX_PARAMETER_COUNT)
		return;
	FxBus_syncSelectedBank();
	fxBus.parameter[idx] = value;
	fxBus.bankParameters[fxBus.parameterBankType][idx] = value;
	FxBus_applyBank(fxBus.parameterBankType);
}

/* Change one bank without changing the effect selected on the front panel. */
void FxBus_setBankParameter(uint8_t type, uint8_t idx, float value)
{
	if (type == FX_OFF || type >= FX_PARAMETER_BANK_COUNT ||
	    idx >= FX_PARAMETER_COUNT)
		return;
	FxBus_syncSelectedBank();
	fxBus.bankParameters[type][idx] = value;
	if (fxBus.parameterBankType == type)
		fxBus.parameter[idx] = value;
	FxBus_applyBank(type);
}

float FxBus_getBankParameter(uint8_t type, uint8_t idx)
{
	if (type == FX_OFF || type >= FX_PARAMETER_BANK_COUNT ||
	    idx >= FX_PARAMETER_COUNT)
		return 0.f;
	return fxBus.bankParameters[type][idx];
}

/* Adjust one FX without changing the selected front-panel effect. */
void FxBus_setBankWet(uint8_t type, float value)
{
	if (type >= FX_PARAMETER_BANK_COUNT)
		return;
	FxBus_syncSelectedBank();
	if (!(value >= 0.f)) value = 0.f;
	if (value > 1.f) value = 1.f;
	fxBus.bankDryWet[type] = value;
	if (type == fxBus.parameterBankType)
		fxBus.dryWet = value;
}

void FxBus_setBankOutput(uint8_t type, uint8_t routing)
{
	if (type >= FX_PARAMETER_BANK_COUNT || routing > MIXER_ROUTING_DAC2_R)
		return;
	FxBus_syncSelectedBank();
	fxBus.bankAudioRouting[type] = routing;
	if (type == fxBus.parameterBankType)
		fxBus.audioRouting = routing;
}

void FxBus_setSelectedWet(float value)
{
	FxBus_syncSelectedBank();
	FxBus_setBankWet(fxBus.parameterBankType, value);
}

void FxBus_setSelectedOutput(uint8_t routing)
{
	FxBus_syncSelectedBank();
	FxBus_setBankOutput(fxBus.parameterBankType, routing);
}

/* Serialize the four independent banks; the OFF editor slot is not stored. */
static uint8_t FxBus_storageByte(float value, unsigned maximum)
{
	if (!(value >= 0.f)) return 0u;
	if (value >= 1.f) return (uint8_t)maximum;
	return (uint8_t)(value * maximum + 0.5f);
}

void FxBus_exportKitBanks(uint8_t *bytes)
{
	if (!bytes) return;
	FxBus_syncSelectedBank();
	for (uint8_t t = FX_DISTORTION; t <= FX_DELAY; ++t)
	{
		const unsigned offset = (unsigned)(t - FX_DISTORTION) * 6u;
		for (unsigned i = 0; i < FX_PARAMETER_COUNT; ++i)
			bytes[offset+i] = FxBus_storageByte(fxBus.bankParameters[t][i], 127u);
		bytes[offset+4] = FxBus_storageByte(fxBus.bankDryWet[t], 100u);
		bytes[offset+5] = fxBus.bankAudioRouting[t] <= MIXER_ROUTING_DAC2_R ?
		                  fxBus.bankAudioRouting[t] : MIXER_ROUTING_DAC1_STEREO;
	}
}

void FxBus_importKitBanks(const uint8_t *bytes, uint8_t selectedType)
{
	if (!bytes) return;
	for (uint8_t t = FX_DISTORTION; t <= FX_DELAY; ++t)
	{
		const unsigned offset = (unsigned)(t - FX_DISTORTION) * 6u;
		for (unsigned i = 0; i < FX_PARAMETER_COUNT; ++i)
			fxBus.bankParameters[t][i] = bytes[offset+i] / 127.f;
		fxBus.bankDryWet[t] = bytes[offset+4] / 100.f;
		fxBus.bankAudioRouting[t] = bytes[offset+5];
	}
	if (selectedType > FX_DELAY) selectedType = FX_OFF;
	fxBus.fxType = selectedType;
	fxBus.parameterBankType = selectedType;
	memcpy(fxBus.parameter, fxBus.bankParameters[selectedType], sizeof(fxBus.parameter));
	fxBus.dryWet = fxBus.bankDryWet[selectedType];
	fxBus.audioRouting = fxBus.bankAudioRouting[selectedType];
	for (uint8_t t = FX_DISTORTION; t <= FX_DELAY; ++t)
		FxBus_applyBank(t);
}
