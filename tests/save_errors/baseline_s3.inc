/* Frozen successful-write oracle from S3 cbffa2d756658b6f32d8e6417ea40ec2cda828d5. */
/*
 * presetManager.c
 *
 * Created: 09.05.2012 16:06:19
 *  Author: Julian
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */
static void preset_writeDrumsetData(uint8_t presetNr, uint8_t isMorph)
{
	uint16_t i;
	unsigned int bytesWritten;

	if(isMorph)
	{
		for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
		{
			uint8_t value;
			//Mod targets are not morphed!!!
			if( (i >= PAR_VEL_DEST_1) && (i <= PAR_VEL_DEST_6) )
			{
				value = activeParameters[i];
			} else if( (i >= PAR_TARGET_LFO1) && (i <= PAR_TARGET_LFO6) )
			{
				value = activeParameters[i];
			} else if( (i >= PAR_VOICE_LFO1) && (i <= PAR_VOICE_LFO6) )
			{
				value = activeParameters[i];
			} 
			else 
			{
				value = preset_getMorphValue(i,activeParameters[PAR_MORPH]);
			}					
			f_write((FIL*)&SDFile,&value,1,&bytesWritten);
		}
	} else {
		for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
		{
			f_write((FIL*)&SDFile,&mainParameters[presetNr][i],1,&bytesWritten);
		}
	}
}
static void preset_writeGlobalData()
{
	unsigned int bytesWritten;
	int i;
	for(i=PAR_BEGINNING_OF_GLOBALS;(i<NUM_PARAMS);i++)
	{
		f_write((FIL*)&SDFile,&activeParameters[i],1,&bytesWritten);
	}
}
static void preset_writePatternData(uint8_t patternNr)
{
#if 0
#else
		UINT bytesWritten;
		uint8_t length;

		//---------------  write the step data (note, vol, prob, autom.) ---------------
		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
			f_write((FIL*)&SDFile,(const void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesWritten);

			f_write((FIL*)&SDFile,(const void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesWritten);
		}

		//--------------- now the main step data ---------------
		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
			for(uint8_t bar=0;bar<NUM_BARS; bar++)
			{
				const uint16_t mainStepData = seq_patternSet.seq_mainSteps[bar][patternNr][trk];
				f_write((FIL*)&SDFile,(const void*)&mainStepData,sizeof(uint16_t),&bytesWritten);
			}
		}

		//----- shuffle setting ------


		//f_write((FIL*)&SDFile,(const void*)&activeParameters[PAR_SHUFFLE],sizeof(uint8_t),&bytesWritten);
		f_write((FIL*)&SDFile,(const void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesWritten);

		//----- pattern/track lengths ------
		// we reuse the same call from above (when saving main step data)
		// but we only use the length info retrieved. We want to store it at the end
		// to avoid breaking compatibility with save file


		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
				length = seq_patternSet.length[patternNr][trk] ;//seq_patternSet.seq_mainSteps[bar][pat][trk];
				f_write((FIL*)&SDFile,(const void*)&length,sizeof(uint8_t),&bytesWritten);
		}

		//write associated kit nr
		f_write((FIL*)&SDFile,&seq_patternSet.kitNr[patternNr],sizeof(uint8_t),&bytesWritten);

#endif
}
FRESULT preset_saveTmpKit()
{
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
	HAL_TIM_Base_Start_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
	FRESULT res;
	unsigned int bytesWritten;

	//open the file
	res = f_open((FIL*)&SDFile,"tmp.snd",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK)
	{
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
		return res;
	}

	//write current preset nr to file
	res = f_write((FIL*)&SDFile,(void*)&menu_selectedPresetNr[SAVE_TYPE_KIT],1,&bytesWritten);
	if(res != FR_OK)
	{
		f_close((FIL*)&SDFile);
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
		return res;
	}
	//write the preset data
	f_write((FIL*)&SDFile,activeParameters,END_OF_SOUND_PARAMETERS,&bytesWritten);

	//close the file
	f_close((FIL*)&SDFile);
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
	return FR_OK;
}
FRESULT preset_saveTmpPat()
{

	//don't save invalid pattern
	if(menu_playedPattern >= NUM_PATTERN) return FR_INVALID_PARAMETER;
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
	HAL_TIM_Base_Start_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif

	FRESULT res;
	UINT bytesWritten;
	uint8_t presetNr = menu_playedPattern;
	//lcd_clear();
	//lcd_home();
	//lcd_string(PSTR("Saving pattern"));

	//open the file
	res = f_open((FIL*)&SDFile,"tmp.pat",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK)
	{
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
		return res;
	}

	//write current preset nr to file
	res = f_write((FIL*)&SDFile,(void*)&menu_playedPattern,1,&bytesWritten);
	if(res != FR_OK)
	{
		f_close((FIL*)&SDFile);
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
		return res;
	}

	//write pattern data
	//preset_writePatternData(presetNr);
#if 0
#else

		uint8_t length;

		//---------------  write the step data (note, vol, prob, autom.) ---------------
		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
			f_write((FIL*)&SDFile,(const void*)&seq_patternSet.seq_subStepPattern[presetNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesWritten);

			f_write((FIL*)&SDFile,(const void*)&seq_patternSet.seq_stepTiming[presetNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesWritten);
		}

		//--------------- now the main step data ---------------
		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
			for(uint8_t bar=0;bar<NUM_BARS; bar++)
			{
				const uint16_t mainStepData = seq_patternSet.seq_mainSteps[bar][presetNr][trk];
				f_write((FIL*)&SDFile,(const void*)&mainStepData,sizeof(uint16_t),&bytesWritten);
			}
		}

		//----- shuffle setting ------


		//f_write((FIL*)&SDFile,(const void*)&activeParameters[PAR_SHUFFLE],sizeof(uint8_t),&bytesWritten);
		f_write((FIL*)&SDFile,(const void*)&seq_patternSet.shuffle[presetNr],sizeof(uint8_t),&bytesWritten);

		//----- pattern/track lengths ------
		// we reuse the same call from above (when saving main step data)
		// but we only use the length info retrieved. We want to store it at the end
		// to avoid breaking compatibility with save file


		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
				length = seq_patternSet.length[presetNr][trk] ;//seq_patternSet.seq_mainSteps[bar][pat][trk];
				f_write((FIL*)&SDFile,(const void*)&length,sizeof(uint8_t),&bytesWritten);
		}

		//write associated kit nr
		f_write((FIL*)&SDFile,&seq_patternSet.kitNr[presetNr],sizeof(uint8_t),&bytesWritten);

#endif

	//close the file
	f_close((FIL*)&SDFile);

	//reset the lcd
//	menu_repaintAll();

#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif

	return res;
}
FRESULT preset_saveDrumsetSD(uint8_t presetNr, uint8_t isMorph)
{
#if USE_SD_CARD
	FRESULT res;
	unsigned int bytesWritten;
	//filename in 8.3  format
	char filename[13];
	preset_makeFileNameSave(filename,presetNr,FEXT_SOUND, savefileNames[presetNr][KIT_NAMES]);
	//sprintf(filename,"p%03d.snd",presetNr);



	//open the file
	res = f_open((FIL*)&SDFile,filename,FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK) return res;
	
	//write preset name to file

	res = f_write((FIL*)&SDFile,(void*)savefileNames[presetNr][KIT_NAMES],8,&bytesWritten);
	if(res != FR_OK) return res;
	//write the preset data
	preset_writeDrumsetData(presetNr, isMorph);
	
	//close the file
	f_close((FIL*)&SDFile);

	return FR_OK;
#else
#endif
}
FRESULT preset_saveGlobals()
{
#if USE_SD_CARD
	FRESULT res;
/*
	res = f_mount((FATFS*)&SDFatFS, (TCHAR const*)SDPath,1);
	if( res != FR_OK)
	{
		return res;
	}
	*/
	//open the file
	res = f_open((FIL*)&SDFile,"glo.cfg",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK) return res;
	preset_writeGlobalData();

	//close the file
	f_close((FIL*)&SDFile);

	//f_unmount((TCHAR const*)SDPath);
	return FR_OK;
#else
#endif	
}
FRESULT preset_saveLastUsedProjectNrToSD(uint8_t projNr)
{
#if USE_SD_CARD
    FRESULT res;

	//open the file
	res = f_open((FIL*)&SDFile,"/proj.idx",FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK)
	{
		//close the file handle
		f_close((FIL*)&SDFile);
		return res;
	}

	unsigned int bytesWritten;
	res = f_write((FIL*)&SDFile,&projNr,1,&bytesWritten);
	if(res != FR_OK)
	{
		//close the file handle
		f_close((FIL*)&SDFile);
		return res;
	}

	//close the file
	f_close((FIL*)&SDFile);

	//f_unmount((TCHAR const*)SDPath);
	return res;
#else
#endif
}
FRESULT preset_savePatternSD(uint8_t presetNr)
{

	FRESULT res;
	UINT bytesWritten;
	//lcd_clear();
	//lcd_home();
	//lcd_string(PSTR("Saving pattern"));
	
	
	//filename in 8.3  format
	char filename[13];
	//sprintf(filename,"p%03d.pat",presetNr);
	preset_makeFileNameSave(filename,presetNr,FEXT_PAT, savefileNames[presetNr][PATTERN_NAMES]);
	//open the file
	res = f_open((FIL*)&SDFile,filename,FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK) return res;

	//write preset name to file
	res = f_write((FIL*)&SDFile,(void*)savefileNames[presetNr][PATTERN_NAMES],8,&bytesWritten);
	if(res != FR_OK) return res;
	//write version tag
	res = f_write((FIL*)&SDFile,"PatV03",6,&bytesWritten);
	if(res != FR_OK) return res;

	//write pattern data
	preset_writePatternData(presetNr);

	//close the file
	f_close((FIL*)&SDFile);
	
	//reset the lcd
	menu_repaintAll();
	
	return FR_OK;
}
FRESULT preset_saveSongSD(uint8_t songNr)
{
	FRESULT res;
	char filename[13];
	unsigned int bytesWritten;
	preset_makeFileNameSave(filename,songNr,FEXT_SONG, savefileNames[songNr][SONG_NAMES]);
	//open the file
	res = f_open((FIL*)&SDFile,filename,FA_CREATE_ALWAYS | FA_WRITE);
	if(res!=FR_OK)
		return res; //file open error... maybe the file does not exist?
	//write name
	res = f_write((FIL*)&SDFile,(void*)savefileNames[songNr][SONG_NAMES],8,&bytesWritten);
	if (res != FR_OK)
	{
		f_close((FIL*)&SDFile);
		return res;
	}

	//write song data
	res = f_write((FIL*)&SDFile,seq_songData[songNr],MAX_SONG_LENGTH,&bytesWritten);
	if (res != FR_OK)
	{
		f_close((FIL*)&SDFile);
		return res;
	}

	//close the file handle
	res = f_close((FIL*)&SDFile);
	return res;
}
