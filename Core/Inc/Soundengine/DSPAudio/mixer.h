/*
 * mixer.h
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


#ifndef MIXER_H_
#define MIXER_H_

#include "stm32f7xx_hal.h"
#include "Soundengine/SharedFxPool.h"

#define USE_SWITCH_ROUTING 1
#define USE_DECIMATOR 1
#define USE_TRIGGERED_PAN 1

#if USE_DECIMATOR
extern float mixer_decimation_rate[7];		/**<sets the sample rate decimation. 0..1 = full rate*/
#endif

extern uint8_t mixer_audioRouting[6];

enum
{
	MIXER_ROUTING_DAC1_STEREO,
	MIXER_ROUTING_DAC2_STEREO,
	MIXER_ROUTING_DAC1_L,
	MIXER_ROUTING_DAC1_R,
	MIXER_ROUTING_DAC2_L,
	MIXER_ROUTING_DAC2_R,
	MIXER_ROUTING_FX_BUS = FX_POOL_ROUTE_LEGACY,
	MIXER_ROUTING_FX_DELAY = FX_POOL_ROUTE_DELAY,
	MIXER_ROUTING_FX_RING = FX_POOL_ROUTE_RING,
	MIXER_ROUTING_FX_DISTORTION = FX_POOL_ROUTE_DISTORTION,
	MIXER_ROUTING_FX_COMPRESSOR = FX_POOL_ROUTE_COMPRESSOR,
};

void mixer_init();
void  mixer_calcNextSampleBlock(int16_t* output,int16_t* output2);

#endif /* MIXER_H_ */
