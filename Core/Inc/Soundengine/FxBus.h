/*
 * FxBus.h
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

#ifndef INC_SOUNDENGINE_FXBUS_H_
#define INC_SOUNDENGINE_FXBUS_H_

#include "Soundengine/globals.h"
#include "Soundengine/config.h"
#include "Soundengine/DSPAudio/FX/Distortion.h"
#include "Soundengine/DSPAudio/FX/Ringmod.h"
#include "Soundengine/DSPAudio/FX/Compressor.h"
#include "Soundengine/DSPAudio/FX/Delay.h"
#include "Soundengine/DSPAudio/FX/Reverb.h"

#define USE_REVERB 0

/* Five slots include FX_OFF; FX types 1..4 have independent parameter state. */
#define FX_PARAMETER_COUNT 4u
#define FX_BANK_SERIALIZED_BYTES 24u
#define FX_PARAMETER_BANK_COUNT (5u + USE_REVERB)

enum FxTypeEnum
{
	FX_OFF,
	FX_DISTORTION,
	FX_RING,
	FX_COMPRESSOR,
	FX_DELAY,
#if USE_REVERB
	FX_REVERB
#endif
};

typedef struct FxStruct
{

	uint8_t fxType;
	uint8_t audioRouting;
	float dryWet;

	/* Legacy menu/MIDI edit view, backed by independently stored effect values. */
	float parameter[FX_PARAMETER_COUNT];

	/* Stage 1: four P1-P4 banks. Wet/output and kit persistence are separate. */
	float bankParameters[FX_PARAMETER_BANK_COUNT][FX_PARAMETER_COUNT];
	/* Independent per-processor wet mix and physical output destination. */
	float bankDryWet[FX_PARAMETER_BANK_COUNT];
	uint8_t bankAudioRouting[FX_PARAMETER_BANK_COUNT];
	uint8_t parameterBankType;


	MasterDistortion distortion;
	Ringmod ringmod;
	Compressor compressor;
	Delay delay;



#if USE_REVERB
	Reverb reverb;
#endif

} FxBus;

extern INDTCMZ FxBus fxBus;

void FxBus_init();
void FxBus_processSelected(uint8_t fxType, int16_t *fxBuffer, int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2, uint32_t size);
void FxBus_process(int16_t *fxBuffer, int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2, uint32_t size);
void FxBus_setParameter(uint8_t idx, float value);
void FxBus_updateParameter();
void FxBus_setBankParameter(uint8_t fxType, uint8_t idx, float value);
float FxBus_getBankParameter(uint8_t fxType, uint8_t idx);
void FxBus_setBankWet(uint8_t type, float value);
void FxBus_setBankOutput(uint8_t type, uint8_t routing);
void FxBus_setSelectedWet(float value);
void FxBus_setSelectedOutput(uint8_t routing);
/* Compact, lossless at the original 7-bit menu resolution. */
void FxBus_exportKitBanks(uint8_t *bytes);
void FxBus_importKitBanks(const uint8_t *bytes, uint8_t selectedType);
//to prevent NaN pollution from delay
void FxBus_prepareFxChange();


#endif /* INC_SOUNDENGINE_FXBUS_H_ */
