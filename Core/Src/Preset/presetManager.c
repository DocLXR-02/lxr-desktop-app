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
//#include "../config.h"
#include <frontPanelParser_ui.h>
#include "Preset/PresetManager.h"
//#include "../Hardware/SD/ff.h"
#include "ff.h"
#include <stdio.h>
#include "menu/CcNr2Text.h"
#include "menu/menu.h"
//#include <util/delay.h>
#include "lcd.h"
//#include <avr/pgmspace.h>
#include <stdlib.h>
//#include <util/atomic.h>
#include "IO/uart.h"
#include "IO/DIO.h"

#include "Soundengine/MIDI/MidiParser.h"
#include "Soundengine/Sequencer/sequencer.h"
#include "Soundengine/Soundengine.h"




//#include "../Hardware/timebase.h"

//FATFS SDFatFS;		/* File system object for the logical drive */
//FIL SDFile;		/* place to hold 1 file*/
static uint8_t morph_state = 0; //flag for non blocking morph
static uint8_t morph_index = 0;
char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];

char currentFolder[13];

//char presetNameBuffer[8];
//char currentName[8];
//uint8_t activePresetNr = 0;

#define FILE_VERSION 2

/*
#define NUM_TRACKS 7
#define NUM_PATTERN 8*/
#define STEPS_PER_PATTERN NUM_STEPS //64


#define FEXT_SOUND 	0
#define FEXT_PAT 	1
#define FEXT_ALL 	2
#define FEXT_PROJ 	3
#define FEXT_SONG 	4



// fill buffer (length 9 total) with a filename. type is one of above
// eg p001.snd
static void preset_makeFileNameOld(char *buf, uint8_t num, uint8_t type);

//load uses wildcard to find a name of the pattern 00-*.ext
static FRESULT preset_makeFileNameLoad(char *buf, uint8_t num, uint8_t type);
//save uses the kit name to create a filename eg 00-NAME.ext
static void preset_makeFileNameSave(char *buf, uint8_t num, uint8_t type, char *name);

//static void preset_makeFileNameTemp(char *buf, uint8_t num, uint8_t type);


#define PRESET_SAVE_TIME 2000

uint32_t preset_kitTimer = 0;
uint32_t preset_patTimer = 0;

volatile uint8_t preset_kitDirty = 0;
volatile uint8_t preset_patDirty = 0;

/* One shared FIL: uncertain writes/closes are quarantined until reboot.
 * Never clear FIL.err or close after an incomplete write: close could flush more data.
 */
static FRESULT preset_sharedFailure = FR_OK;
/* Only unsafe autosave failures pause the session; safe open failures back off. */
static uint8_t preset_autosavePaused;
/* Autosave retry state: foreground only, independent of edit inactivity timers. */
typedef struct {
    uint32_t at;
    uint32_t wait;
} PresetAutosaveRetry;
static PresetAutosaveRetry preset_patRetry, preset_kitRetry;
#define PRESET_AUTOSAVE_MAX_WAIT (PRESET_SAVE_TIME * 16u)
/* End autosave retry state. */
uint8_t preset_autosaveIsPaused(void)
{
    return preset_autosavePaused;
}
FRESULT preset_storageError(void)
{
    return preset_sharedFailure;
}
static FRESULT preset_sharedStatus(void)
{
    if(preset_sharedFailure != FR_OK) return preset_sharedFailure;
    return SDFile.obj.fs ? FR_LOCKED : FR_OK;
}
static FRESULT preset_openShared(FIL *file, const TCHAR *name, BYTE mode)
{
    FRESULT res = preset_sharedStatus();
    if(res != FR_OK) return res;
    res = f_open(file, name, mode);
    if(res != FR_OK) {
        /* These f_open failures precede file creation/truncation in this FatFs.
         * The S1 disk latch still blocks hardware access after a physical fault.
         * DENIED is logical only for the configured single-SFN, non-exFAT path.
         */
        switch(res) {
        case FR_NO_FILE: case FR_NO_PATH: case FR_INVALID_NAME: case FR_EXIST:
        case FR_INVALID_DRIVE: case FR_NOT_ENABLED: case FR_NO_FILESYSTEM:
        case FR_WRITE_PROTECTED: case FR_LOCKED: case FR_TOO_MANY_OPEN_FILES:
        case FR_INVALID_PARAMETER: case FR_NOT_READY:
#if _USE_LFN == 0 && !_FS_EXFAT
        case FR_DENIED: /* Read-only/directory target, full SFN table or no free cluster. */
#endif
            if(file->obj.fs) preset_sharedFailure = res;
            break;
        default:
            preset_sharedFailure = res;
            break;
        }
    }
    return res;
}
static FRESULT preset_closeShared(FIL *file)
{
    FRESULT res;
    if(preset_sharedFailure != FR_OK) return preset_sharedFailure;
    if(!file->obj.fs) return FR_INVALID_OBJECT;
    res = f_close(file);
    if(res != FR_OK) preset_sharedFailure = res;
    return res;
}
static FRESULT preset_writeChecked(const void *data, UINT count)
{
    UINT written = 0;
    FRESULT res;
    if(preset_sharedFailure != FR_OK) return preset_sharedFailure;
    res = f_write((FIL*)&SDFile, data, count, &written);
    if(res == FR_OK && written != count) res = FR_DISK_ERR;
    if(res != FR_OK) preset_sharedFailure = res;
    return res;
}
static FRESULT preset_finishSave(FRESULT res)
{
    if(res != FR_OK) return res;
    res = preset_closeShared((FIL*)&SDFile);
    /* A completed save can re-arm check-required autosave holds, never a fault:
     * unresolved app/physical faults prevent this contract from succeeding.
     * Keep dirty state and cooldowns: this may not be a temporary-file save.
     */
    if(res == FR_OK && preset_sharedFailure == FR_OK && !SDFile.obj.fs)
        preset_autosavePaused = 0;
    return res;
}

/* Root project paths only: never append a child to currentFolder[13]. */
static FRESULT preset_projectPath(char *path, size_t capacity, uint8_t slot)
{
    if(!path || capacity < sizeof("/proj00") || slot >= NUM_KIT_SLOTS)
        return FR_INVALID_PARAMETER;
    memcpy(path, "/proj00", sizeof("/proj00"));
    path[5] = '0' + slot / 10;
    path[6] = '0' + slot % 10;
    return FR_OK;
}

/* Only an absent path can be reserved; never inspect/delete old contents. */
static FRESULT preset_createProjectFolder(const char *folder)
{
    FILINFO info;
    FRESULT res = f_stat(folder, &info);
    if(res == FR_OK) return FR_EXIST;
    if(res != FR_NO_FILE) return res;
    return f_mkdir(folder);
}

/* A single SFN child, built completely before any filesystem operation. */
static FRESULT preset_childPath(char *path, size_t capacity,
                               const char *folder, const char *name)
{
    size_t base = 0, length = 0, dot = 0;
    if(!path || !folder || !name) return FR_INVALID_PARAMETER;
    while(base < capacity && folder[base]) base++;
    if(base == 0 || base == capacity || folder[0] != '/') return FR_INVALID_NAME;
    while(length < 13 && name[length]) {
        unsigned char c = name[length];
        if(c <= ' ' || c >= 127 || strchr("\"*+,/:;<=>?[\\]|", c)) return FR_INVALID_NAME;
        if(c == '.') {
            if(dot || length == 0 || length > 8) return FR_INVALID_NAME;
            dot = length + 1;
        }
        length++;
    }
    if(length == 0 || length == 13 || (!dot && length > 8) ||
       (dot && (length == dot || length - dot > 3))) return FR_INVALID_NAME;
    if(capacity - base < length + 2) return FR_INVALID_NAME;
    memmove(path, folder, base);
    path[base] = '/';
    memcpy(path + base + 1, name, length + 1);
    return FR_OK;
}

/* Read-only handles may be closed; retain the first error if close also fails. */
static FRESULT preset_finishRead(FRESULT res)
{
    FRESULT closed = preset_closeShared((FIL*)&SDFile);
    return res == FR_OK ? closed : res;
}

/* Only names produced by the firmware in its reserved flat /tmp directory. */
static uint8_t preset_isTmpFile(const char *name)
{
    char lower[13];
    size_t n;
    for(n = 0; n < sizeof(lower); n++) {
        unsigned char c = name[n];
        lower[n] = c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
        if(!c) break;
    }
    if(n == sizeof(lower)) return 0;
    if(!strcmp(lower, "tmp.snd") || !strcmp(lower, "tmp.pat")) return 1;
    if(n < 7) return 0;
    const char *ext = lower + n - 4;
    if(strcmp(ext, ".snd") && strcmp(ext, ".pat") && strcmp(ext, ".sng")) return 0;
    if(lower[0] >= '0' && lower[0] <= '9' && lower[1] >= '0' && lower[1] <= '9' &&
       lower[2] == '-' && n - 4 >= 3 && n - 4 <= 8)
        return (lower[0] - '0') * 10 + lower[1] - '0' < NUM_KIT_SLOTS;
    if(n != 8 || lower[0] != 'p') return 0;
    for(size_t i = 1; i < 4; i++) if(lower[i] < '0' || lower[i] > '9') return 0;
    return (lower[1] - '0') * 100 + (lower[2] - '0') * 10 + lower[3] - '0' < NUM_KIT_SLOTS;
}

/* Match the legacy short-kit zero-fill rule; no stale tail or initKit overread. */
static void preset_defaultKit(uint8_t slot)
{
    if(slot >= NUM_KIT_SLOTS) return;
    size_t count = sizeof(initKit) - 8;
    if(count > END_OF_SOUND_PARAMETERS) count = END_OF_SOUND_PARAMETERS;
    memcpy(savefileNames[slot][KIT_NAMES], initKit, 8);
    memset(mainParameters[slot], 0, END_OF_SOUND_PARAMETERS);
    memcpy(mainParameters[slot], initKit + 8, count);
}

static FRESULT preset_writePatternData(uint8_t patternNr);

FRESULT preset_initTmpFolder(void)
{
    FRESULT res = preset_sharedStatus();
    FILINFO info;
    if(res != FR_OK) return res;
    res = f_stat("/tmp", &info);
    if(res == FR_NO_FILE) {
        res = f_mkdir("/tmp");
        if(res != FR_OK) return res;
    } else if(res != FR_OK) return res;
    else if(!(info.fattrib & AM_DIR)) return FR_NO_PATH;
    return f_chdir("/tmp");
}

void preset_resetAllSaveTimers()
{
	preset_kitDirty = 0;
	preset_patDirty = 0;
}

FRESULT loadTmpFolderFiles()
{
    FRESULT shared = preset_sharedStatus();
    if(shared != FR_OK) return shared;
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
	HAL_TIM_Base_Start_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif

	FRESULT res;
	//check tmp folder for *.snd and *.pat files and load them to RAM
	for(uint8_t i=0;i<NUM_KIT_SLOTS;i++)
	{

		//FR_NO_FILE just means an incomplete project and is no reason to abort!
		res = preset_loadDrumsetSD(i, 0);
		if ((res != FR_OK) && (res != FR_NO_FILE))
		{
			lcd_process_display_interrupt = 1;
			preset_initTmpFolder();
			preset_closeShared((FIL*)&SDFile);
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
			return res;
		}
		res = preset_loadPatternSD(i);
		if ((res != FR_OK) && (res != FR_NO_FILE))
		{
			preset_closeShared((FIL*)&SDFile);
			lcd_process_display_interrupt = 1;
			preset_initTmpFolder();
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
			return res;
		}
		res = preset_loadSongSD(i);
		if ((res != FR_OK) && (res != FR_NO_FILE))
		{
			preset_closeShared((FIL*)&SDFile);
			lcd_process_display_interrupt = 1;
			preset_initTmpFolder();
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
			return res;
		}
	}

	return FR_OK;
}

void preset_touchTimer(PresetTimer type)
{
	if(type==PRESET_KIT_TIMER)
	{
		preset_kitTimer = HAL_GetTick();
		preset_kitDirty = 1;
	}
	else if(type==PRESET_PAT_TIMER)
	{
		preset_patTimer = HAL_GetTick();
		preset_patDirty = 1;
	}
	else if(type==PRESET_STOP_PAT_TIMER)
	{
		//abort pattern save timer, since seq is running again...
		preset_patDirty = 0;
	}

};


FRESULT preset_saveTmpKit()
{
    if(menu_selectedPresetNr[SAVE_TYPE_KIT] >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
    FRESULT res;
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Start_IT(&htim2);
#endif
    res = preset_openShared((FIL*)&SDFile, "/tmp/tmp.snd", FA_CREATE_ALWAYS | FA_WRITE);
    if(res == FR_OK) {
        res = preset_writeChecked(&menu_selectedPresetNr[SAVE_TYPE_KIT], 1);
        if(res == FR_OK) res = preset_writeChecked(activeParameters, END_OF_SOUND_PARAMETERS);
        res = preset_finishSave(res);
    }
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Stop_IT(&htim2);
#endif
    return res;
}

FRESULT preset_LoadTmpKit(void)
{
    FRESULT res;
    UINT count;
    uint8_t slot, data[END_OF_SOUND_PARAMETERS] = {0};
    res = preset_openShared((FIL*)&SDFile, "/tmp/tmp.snd", FA_READ);
    if(res != FR_OK) return res;
    res = f_read((FIL*)&SDFile, &slot, 1, &count);
    if(res == FR_OK && (count != 1 || slot >= NUM_KIT_SLOTS)) res = FR_INT_ERR;
    if(res == FR_OK) res = f_read((FIL*)&SDFile, data, sizeof(data), &count);
    res = preset_finishRead(res);
    if(res != FR_OK) return res;
    /* Short legacy kit tails remain zero-filled, exactly as named kit loading. */
    const uint8_t nmt = getNumModTargets();
    for(uint8_t i = 0; i < 6; i++) {
        if(data[PAR_VEL_DEST_1+i] >= nmt) data[PAR_VEL_DEST_1+i] = 0;
        if(data[PAR_TARGET_LFO1+i] >= nmt) data[PAR_TARGET_LFO1+i] = 0;
    }
    memcpy(activeParameters, data, sizeof(data));
    menu_selectedPresetNr[SAVE_TYPE_KIT] = slot;
    preset_sendDrumsetParameters();
    return FR_OK;
}

// Read the existing length/kit tail without exposing partial or invalid lengths.
static uint8_t preset_readPatternLengths(uint8_t patternNr, uint8_t dataVersion)
{
	if(patternNr >= NUM_PATTERN) return 0;
	uint8_t lengths[NUM_TRACKS];
	uint8_t kit = 0;
	UINT bytesRead;
	if(f_read((FIL*)&SDFile, lengths, sizeof(lengths), &bytesRead) != FR_OK) return 0;
	if(dataVersion == 1 && bytesRead == 0)
	{
		// Untagged V1 files may have no length tail at all.
		for(uint8_t trk=0; trk<NUM_TRACKS; trk++) lengths[trk] = 16;
	}
	else if(bytesRead != sizeof(lengths)) return 0;
	for(uint8_t trk=0; trk<NUM_TRACKS; trk++)
	{
		if(lengths[trk] == 0) lengths[trk] = 16;
		if(lengths[trk] > NUM_STEPS) return 0;
	}
	if(dataVersion > 1)
	{
		if(f_read((FIL*)&SDFile, &kit, sizeof(kit), &bytesRead) != FR_OK || bytesRead != sizeof(kit) || kit >= NUM_KIT_SLOTS) return 0;
	}
	for(uint8_t trk=0; trk<NUM_TRACKS; trk++)
		seq_patternSet.length[patternNr][trk] = lengths[trk];
	seq_patternSet.kitNr[patternNr] = kit;
	return 1;
}

FRESULT preset_LoadTmpPat()
{

	uint8_t success=1; // start off succeeding
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
	HAL_TIM_Base_Start_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
	//filename in 8.3  format
		//uint8_t fileVersion = 1;
		UINT bytesRead;
		FRESULT res;



		res = preset_openShared((FIL*)&SDFile,"/tmp/tmp.pat",FA_OPEN_EXISTING | FA_READ);
		if(res != FR_OK)
		{
		#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
					HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
		#else

		#endif
				return res;
		}

		// Validate the stored pattern index before publishing it to UI/playback state.
		uint8_t loadedPattern;
		res = f_read((FIL*)&SDFile,(void*)&loadedPattern,1,&bytesRead);
		if(res != FR_OK || bytesRead != 1)
		{
			success = 0;
			goto closeFile;
		}

		if(loadedPattern >= NUM_PATTERN)
		{
			success = 0;
			goto closeFile;
		}

		menu_playedPattern = loadedPattern;
		menu_shownPattern = menu_playedPattern;
		seq_activePattern = menu_playedPattern;
		seq_pendingPattern = menu_playedPattern;
		menu_selectedPresetNr[SAVE_TYPE_PATTERN] = menu_playedPattern;

		frontPanel_sendData(SEQ_CC,SEQ_SET_SHOWN_PATTERN,menu_playedPattern);

		seq_copyPatternToBackupBuffer(seq_activePattern);
		//frontParser_shownPattern = menu_playedPattern;
		uint8_t patternNr = menu_playedPattern;

	//---------------- read pattern data -----------------------------

		// note that the pattern length data is no longer stored in the same way.
		// This means that loading old patterns with non-standard length, the length will be
		// ignored and a standard length used. I think it's acceptable as long as users are aware of this.


		// ---------- Load step data first

		// load step data



		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{

			if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
			{
				success=0;
				break; // file error, or we ran out of file (eof)
			}

			//Pattern V3 and up => flam added
			//if(dataVersion > 2)
			{
				if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
				{
					success=0;
					break; // file error, or we ran out of file (eof)
				}
			}

		}



		// ---------- load the main step data next
		uint16_t mainStepData;


			for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
			{
				for(uint8_t bar=0;bar<NUM_BARS; bar++)
				{
					if(f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead) == FR_OK && bytesRead==sizeof(uint16_t)) {
						seq_patternSet.seq_mainSteps[bar][patternNr][trk] = mainStepData;
					}
					 else {
							success=0;
							break;
						}
				}
			}



		// ------------ shuffle settings
		if(success) {
			//load the shuffle settings
			if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead) == FR_OK && bytesRead == sizeof(uint8_t))
			{

				//activeParameters[PAR_SHUFFLE] = seq_patternSet.shuffle[patternNr];
				//frontPanel_sendData(SEQ_CC,SEQ_SHUFFLE,parameter_values[PAR_SHUFFLE]);

				//seq_setShuffle(activeParameters[PAR_SHUFFLE]/127.f);
			}
			else {
				success=0;
			}
		}

		if(success) success = preset_readPatternLengths(patternNr, 3);

		if( success == 0)
		{


			res = preset_finishRead(res == FR_OK && !success ? FR_INT_ERR : res);
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
			return FR_INT_ERR;
		}

		success=1;
		//close the file handle



	closeFile:
		res = preset_finishRead(res == FR_OK && !success ? FR_INT_ERR : res);

		#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
					HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
		#else

		#endif


		// return success or failure
		return res;


}

FRESULT preset_saveTmpPat()
{
    FRESULT res;
    if(menu_playedPattern >= NUM_PATTERN) return FR_INVALID_PARAMETER;
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Start_IT(&htim2);
#endif
    res = preset_openShared((FIL*)&SDFile, "/tmp/tmp.pat", FA_CREATE_ALWAYS | FA_WRITE);
    if(res == FR_OK) {
        res = preset_writeChecked(&menu_playedPattern, 1);
        if(res == FR_OK) res = preset_writePatternData(menu_playedPattern);
        res = preset_finishSave(res);
    }
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Stop_IT(&htim2);
#endif
    return res;
}

FRESULT preset_clearTmpFolder(void)
{
    FRESULT res = preset_sharedStatus();
    FILINFO info;
    DIR dir;
    char path[21]; /* /tmp + slash + maximum 12-byte SFN + NUL. */
    if(res != FR_OK) return res;
    res = f_opendir(&dir, "/tmp");
    if(res == FR_NO_PATH || res == FR_NO_FILE) return preset_initTmpFolder();
    if(res != FR_OK) return res;
    for(;;) {
        res = f_readdir(&dir, &info);
        if(res != FR_OK || !info.fname[0]) break;
        /* Preserve unknown files and all directories; never recurse. */
        if((info.fattrib & AM_DIR) || !preset_isTmpFile(info.fname)) continue;
        res = preset_childPath(path, sizeof(path), "/tmp", info.fname);
        if(res != FR_OK) break;
        res = f_unlink(path);
        if(res != FR_OK) break;
    }
    FRESULT closed = f_closedir(&dir);
    if(res == FR_OK) res = closed;
    if(res == FR_OK) res = preset_initTmpFolder();
    return res;
}


/* Clear only our pending tag, not an edit/cancellation delivered during I/O.
 * A single exclusive attempt runs in foreground: no IRQ mask, spin or ISR work.
 * If preempted, retain pending work and defer a known-success follow-up by 2 s.
 */
static uint8_t preset_completeAutosave(volatile uint8_t *dirty, FRESULT res)
{
    if(__LDREXB(dirty) != 2u) {
        __CLREX();
        return 1;
    }
    return __STREXB(res == FR_OK ? 0u : 1u, dirty) == 0u;
}
static uint8_t preset_autosaveCanRetry(FRESULT res)
{
    if(preset_sharedFailure != FR_OK || SDFile.obj.fs) return 0;
    /* These can only be rejected opens in the temporary writers: write/close
     * errors latch above. NOT_READY, lock/owner and mount errors need checks.
     */
    switch(res) {
    case FR_NO_FILE: case FR_NO_PATH: case FR_INVALID_NAME: case FR_EXIST:
    case FR_WRITE_PROTECTED:
#if _USE_LFN == 0 && !_FS_EXFAT
    case FR_DENIED:
#endif
        return 1;
    default:
        return 0;
    }
}
static void preset_autosaveOne(volatile uint8_t *dirty, uint32_t edited,
                               PresetAutosaveRetry *retry, uint8_t pattern)
{
    uint32_t now;
    FRESULT res;
    uint8_t completed;
    if(!*dirty) return;
    now = HAL_GetTick();
    if((uint32_t)(now - edited) < PRESET_SAVE_TIME ||
       (retry->wait && (uint32_t)(now - retry->at) < retry->wait)) return;
    *dirty = 2u; /* Still pending, tagged before any payload is read. */
    res = pattern ? preset_saveTmpPat() : preset_saveTmpKit();
    completed = preset_completeAutosave(dirty, res);
    now = HAL_GetTick(); /* Cooldown begins after the synchronous attempt returns. */
    if(res == FR_OK) {
        retry->at = now;
        retry->wait = completed ? 0u : PRESET_SAVE_TIME;
    } else if(preset_autosaveCanRetry(res)) {
        retry->at = now;
        if(!retry->wait) retry->wait = PRESET_SAVE_TIME;
        else if(retry->wait < PRESET_AUTOSAVE_MAX_WAIT) retry->wait *= 2u;
    } else {
        preset_autosavePaused = 1;
    }
}
void preset_processTimer()
{
    if(preset_sharedFailure != FR_OK || preset_autosavePaused) return;
    if(!preset_patDirty && !preset_kitDirty) return;
    /* Do not use a retry to initiate FatFs lazy mounting/reinitialization. */
    if(!SDFatFS.fs_type || SDFile.obj.fs) { preset_autosavePaused = 1; return; }
    preset_autosaveOne(&preset_patDirty, preset_patTimer, &preset_patRetry, 1);
    if(preset_autosavePaused) return;
    preset_autosaveOne(&preset_kitDirty, preset_kitTimer, &preset_kitRetry, 0);
}


/*------------------------------------------------------------/
/ Remove all contents of a directory
/ This function works regardless of FF_FS_RPATH.
/------------------------------------------------------------*/





//----------------------------------

uint8_t preset_isNameEmpty(char* name)
{
	uint8_t isEmpty = 1;

	if( (name[0] != 'E') && (name[0] != 0) ) isEmpty = 0;
	if( (name[1] != 'm') && (name[1] != 0) ) isEmpty = 0;
	if( (name[2] != 'p') && (name[2] != 0) ) isEmpty = 0;
	if( (name[3] != 't') && (name[3] != 0) ) isEmpty = 0;
	if( (name[4] != 'y') && (name[4] != 0) ) isEmpty = 0;
	if( (name[5] != 0) ) isEmpty = 0;
	if( (name[6] != 0) ) isEmpty = 0;
	if( (name[7] != 0) ) isEmpty = 0;


	return isEmpty;
}
//----------------------------------------------------
//load init kit
void preset_init(uint8_t presetNr)
{

    if(presetNr >= NUM_KIT_SLOTS) return;
    preset_defaultKit(presetNr);
    uint8_t *para = mainParameters[presetNr];
    int16_t i;

	//special case mod targets. normalize.
	const uint8_t nmt=getNumModTargets();
	for(i=0;i<6;i++) {
		// since I've changed the meaning of these, I'll ensure that it's valid for kits saved prior to the
		// change
		if(para[PAR_VEL_DEST_1+i] >= nmt )
			para[PAR_VEL_DEST_1+i] = 0;
		if(para[PAR_TARGET_LFO1+i] >= nmt )
			para[PAR_TARGET_LFO1+i] = 0;
	}

	//copy parameters to edit buffer
	memcpy(activeParameters, mainParameters[presetNr], sizeof(uint8_t) * END_OF_SOUND_PARAMETERS);
	preset_sendDrumsetParameters();
	//frontPanel_sendData(PRESET,PRESET_LOAD,presetNr);

}

//----------------------------------------------------
static FRESULT preset_writeDrumsetData(uint8_t presetNr, uint8_t isMorph)
{
	uint16_t i;
	FRESULT res;

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
			res = preset_writeChecked(&value,1);
			if(res != FR_OK) return res;
		}
	} else {
		for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
		{
			res = preset_writeChecked(&mainParameters[presetNr][i],1);
			if(res != FR_OK) return res;
		}
	}
    return FR_OK;
}

//----------------------------------------------------
void preset_loadDrumsetRam(uint8_t presetNr, uint8_t isMorph, uint8_t sendImmediate)
{
    if(presetNr >= NUM_KIT_SLOTS) return;
	soundengine_process();
	if(isMorph)
	{
		morphParameters = mainParameters[presetNr];
	}
	else
	{
		//copy parameters to edit buffer
		memcpy(activeParameters, mainParameters[presetNr], sizeof(uint8_t) * END_OF_SOUND_PARAMETERS);
		//if(activeParameters[PAR_PATTERN_MODE] = 1)
		//load asociated morph kit as well

		// morphParameters = mainParameters[activeParameters[PAR_MORPH_KIT_DEST]];


	}
	soundengine_process();

	if(sendImmediate)
	{
		preset_sendDrumsetParametersImmediate();
	}
	else
	{
		//activePresetNr = menu_currentPresetNr[menu_saveOptions.category];
		preset_sendDrumsetParameters();
	}
}
//----------------------------------------------------
void preset_saveDrumsetRam(uint8_t presetNr, uint8_t isMorph)
{
    if(presetNr >= NUM_KIT_SLOTS) return;
	if(isMorph)
	{
		for(uint16_t i=0;i<END_OF_SOUND_PARAMETERS;i++)
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
			//f_write((FIL*)&SDFile,&value,1,&bytesWritten);
			mainParameters[presetNr][i] = value;
		}
	}
	else
	{
		//copy edit buffer to parameter array
		memcpy(mainParameters[presetNr], activeParameters, sizeof(uint8_t) * END_OF_SOUND_PARAMETERS);
	}
}
//----------------------------------------------------
FRESULT preset_saveDrumsetSD(uint8_t presetNr, uint8_t isMorph)
{
#if USE_SD_CARD
    char filename[13];
    FRESULT res;
    if(presetNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
    preset_makeFileNameSave(filename, presetNr, FEXT_SOUND, savefileNames[presetNr][KIT_NAMES]);
    res = preset_openShared((FIL*)&SDFile, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) return res;
    res = preset_writeChecked(savefileNames[presetNr][KIT_NAMES], 8);
    if(res == FR_OK) res = preset_writeDrumsetData(presetNr, isMorph);
    return preset_finishSave(res);
#else
    return FR_NOT_READY;
#endif
};

//----------------------------------------------------
static FRESULT preset_writeGlobalData()
{
	FRESULT res;
	int i;
	for(i=PAR_BEGINNING_OF_GLOBALS;(i<NUM_PARAMS);i++)
	{
		res = preset_writeChecked(&activeParameters[i],1);
		if(res != FR_OK) return res;
	}
    return FR_OK;
}

//----------------------------------------------------
FRESULT preset_saveGlobals()
{
#if USE_SD_CARD
    FRESULT res = preset_openShared((FIL*)&SDFile, "glo.cfg", FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) return res;
    return preset_finishSave(preset_writeGlobalData());
#else
    return FR_NOT_READY;
#endif
}
//----------------------------------------------------
// Load globals only after a complete read and a successful close.
static FRESULT preset_loadGlobalFile(void)
{
    uint8_t data[NUM_PARAMS - PAR_BEGINNING_OF_GLOBALS];
    UINT count;
    FRESULT res = preset_openShared((FIL*)&SDFile, "glo.cfg", FA_READ);
    if(res != FR_OK) return res;
    res = f_read((FIL*)&SDFile, data, sizeof(data), &count);
    if(res == FR_OK && count != sizeof(data)) res = FR_INT_ERR;
    res = preset_finishRead(res);
    if(res == FR_OK) memcpy(&activeParameters[PAR_BEGINNING_OF_GLOBALS], data, sizeof(data));
    return res;
}
//----------------------------------------------------
uint8_t preset_loadGlobals(void)
{
    if(preset_loadGlobalFile() != FR_OK) return 0;
    menu_sendAllGlobals();
    return 1;
}




//----------------------------------------------------
void preset_loadDefaultGlobals()
{
	activeParameters[PAR_BPM] = 120;

	activeParameters[PAR_MIDI_CHAN_1] = 1;
	activeParameters[PAR_MIDI_CHAN_2] = 1;
	activeParameters[PAR_MIDI_CHAN_3] = 1;
	activeParameters[PAR_MIDI_CHAN_4] = 1;
	activeParameters[PAR_MIDI_CHAN_5] = 1;
	activeParameters[PAR_MIDI_CHAN_6] = 1;
	activeParameters[PAR_MIDI_CHAN_7] = 1;

	activeParameters[PAR_MIDI_NOTE1] = 48;
	activeParameters[PAR_MIDI_NOTE2] = 48;
	activeParameters[PAR_MIDI_NOTE3] = 48;
	activeParameters[PAR_MIDI_NOTE4] = 48;
	activeParameters[PAR_MIDI_NOTE5] = 48;
	activeParameters[PAR_MIDI_NOTE6] = 48;
	activeParameters[PAR_MIDI_NOTE7] = 48;


	activeParameters[PAR_FETCH] = 0;
	activeParameters[PAR_FOLLOW] = 0;

	activeParameters[PAR_QUANTISATION] = 0;

	activeParameters[PAR_SCREENSAVER_ON_OFF] = 1;

	activeParameters[PAR_MIDI_ROUTING] = 0;
	activeParameters[PAR_MIDI_FILT_TX] = 15;	//all
	activeParameters[PAR_MIDI_FILT_RX] = 15; //all

	activeParameters[PAR_PRESCALER_CLOCK_IN] = 0; //all
	activeParameters[PAR_PRESCALER_CLOCK_OUT1] = 0; //all
	activeParameters[PAR_PRESCALER_CLOCK_OUT2] = 0; //all
	activeParameters[PAR_TRIG_GATE_MODE] = 0; //all

	activeParameters[PAR_BAR_RESET_MODE] = 1; //all
	activeParameters[PAR_MIDI_CHAN_GLOBAL] = 0; //all

	activeParameters[PAR_PATTERN_MODE] = 1; //auto

}
//----------------------------------------------------
void preset_createGlobals()
{
    FRESULT res;
    preset_loadDefaultGlobals();
    res = preset_saveGlobals();
    if(res != FR_OK) menu_showSdError(res);
}
//----------------------------------------------------
FRESULT preset_loadLastUsedProjectNrFromSD(uint8_t *projNr)
{
    if(!projNr) return FR_INVALID_PARAMETER;
    *projNr = 0;
    uint8_t slot;
    UINT count;
    FRESULT res = preset_openShared((FIL*)&SDFile, "/proj.idx", FA_READ);
    if(res != FR_OK) return res;
    res = f_read((FIL*)&SDFile, &slot, 1, &count);
    if(res == FR_OK && (count != 1 || slot >= NUM_KIT_SLOTS)) res = FR_INT_ERR;
    res = preset_finishRead(res);
    if(res == FR_OK) *projNr = slot;
    return res;
}
//----------------------------------------------------
FRESULT preset_saveLastUsedProjectNrToSD(uint8_t projNr)
{
    if(projNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
#if USE_SD_CARD
    FRESULT res = preset_openShared((FIL*)&SDFile, "/proj.idx", FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) return res;
    return preset_finishSave(preset_writeChecked(&projNr, 1));
#else
    return FR_NOT_READY;
#endif
}
//----------------------------------------------------
// read the data into either the main parameters or the morph parameters.
// fix some values, and set values to 0 that were not read in
static FRESULT preset_readDrumsetData(uint8_t presetNr, uint8_t isMorph)
{
    if(presetNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
	
#if USE_SD_CARD		
	//read the file content
	unsigned int bytesRead=1;
	int16_t i;
	uint8_t *para;
	bytesRead = 0;
	
	// TODO check: using this function for morph may be wrong
	if(isMorph)
		para=morphParameters;
	else
		para=mainParameters[presetNr];

	FRESULT res=f_read((FIL*)&SDFile, para,END_OF_SOUND_PARAMETERS, &bytesRead);
	if(res==FR_OK) {
		// set to 0 for any that were not read from the file
		if(END_OF_SOUND_PARAMETERS-bytesRead)
			memset(para+bytesRead,0,END_OF_SOUND_PARAMETERS-bytesRead);
		
		//special case mod targets. normalize.
		const uint8_t nmt=getNumModTargets();
		for(i=0;i<6;i++) {
			// since I've changed the meaning of these, I'll ensure that it's valid for kits saved prior to the
			// change
			if(para[PAR_VEL_DEST_1+i] >= nmt )
				para[PAR_VEL_DEST_1+i] = 0;
			if(para[PAR_TARGET_LFO1+i] >= nmt )
				para[PAR_TARGET_LFO1+i] = 0;
		}
	}

	return res;

#endif
}

//----------------------------------------------------
FRESULT preset_loadDrumsetSD(uint8_t presetNr, uint8_t isMorph)
{
    if(presetNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
	//activePresetNr = presetNr;
#if USE_SD_CARD
	//filename in 8.3  format
	char filename[13];
	unsigned int bytesRead;

#if !CALC_SAMPLES_IN_INTERRUPT
	while(!soundengine_calcNextSampleBlock())
	{
		//wait till new block is calculated
	}
#endif

	//sprintf(filename,"p%03d.snd",presetNr);
	/*
	preset_makeFileNameLoad(filename,presetNr,FEXT_SOUND);

	//open the file
	FRESULT res = f_open((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
	*/
	FRESULT res = preset_makeFileNameLoad(filename,presetNr,FEXT_SOUND);
	if(res==FR_NO_FILE)
	{
		//try old filename
		preset_makeFileNameOld(filename,presetNr,FEXT_SOUND);
		res = preset_openShared((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
		if(res!=FR_OK)
			goto error; //file open error... maybe the file does not exist?
	}

    if(res != FR_OK) return res;

#if !CALC_SAMPLES_IN_INTERRUPT
	//to prevent audio dropouts
	soundengine_calcNextSampleBlock();
#endif
	//first the preset name
	res=f_read((FIL*)&SDFile,(void*)savefileNames[presetNr][KIT_NAMES],8,&bytesRead);
	if(res != FR_OK || bytesRead != 8) {
        if(res == FR_OK) res=FR_INT_ERR;
		goto closeFile;
	}
	if(preset_isNameEmpty(savefileNames[presetNr][KIT_NAMES]))
	{
		sprintf((void*)savefileNames[presetNr][KIT_NAMES], "Empty");
	}



#if !CALC_SAMPLES_IN_INTERRUPT
	//to prevent audio dropouts
	soundengine_calcNextSampleBlock();
#endif
	//then the data
	res=preset_readDrumsetData(presetNr, isMorph);
#if !CALC_SAMPLES_IN_INTERRUPT
	//to prevent audio dropouts
	soundengine_calcNextSampleBlock();
#endif
closeFile:
	//close the file handle
	res = preset_finishRead(res);

	// update the soundengine with the new parameters
	if(res==FR_OK) {
		preset_sendDrumsetParameters();
		return FR_OK;
	}
error:
	return res;
#else
	frontPanel_sendData(PRESET,PRESET_LOAD,presetNr);
#endif
}
//----------------------------------------------------
//val is interpolation value between 0 and 255
//uses bankers rounding
static uint8_t interpolate(uint8_t a, uint8_t b, uint8_t x)
{
	uint16_t fixedPointValue = (uint16_t)(((a*256) + (b-a)*x));
	uint8_t result = (uint8_t)(fixedPointValue/256);

	return (uint8_t)((fixedPointValue&0xff) < 0x7f ? result : result+1);
	//return ((a*255) + (b-a)*x)/255;
}

//----------------------------------------------------
// These slots store MIDI transport state, not sound parameters.
static uint8_t preset_isTransportParameter(uint8_t paramNr)
{
	return paramNr == PAR_NRPN_DATA_ENTRY_COARSE ||
		paramNr == PAR_NRPN_FINE || paramNr == PAR_NRPN_COARSE ||
		paramNr == PAR_BANK_CHANGE;
}
//----------------------------------------------------
IN_ITCM_RAM void preset_sendDrumsetParametersImmediate()
{
		//special case mod targets
		for(uint8_t i=0;i<6;i++)
		{
			//**VELO load drumkit. translate to param value before sending
			// parameter_values[PAR_VEL_DEST_1+i] is an index into modTargets, we need to send
			// a parameter number
			uint8_t value = (uint8_t)pgm_read_word(&modTargets[activeParameters[PAR_VEL_DEST_1+i]].param);
			uint8_t upper,lower;
			upper = (uint8_t)(((value&0x80)>>7) | (((i)&0x3f)<<1));
			lower = value&0x7f;
			frontPanel_sendData(CC_VELO_TARGET,upper,lower);

			// ensure target voice # is valid
			if(activeParameters[PAR_VOICE_LFO1+i] < 1 || activeParameters[PAR_VOICE_LFO1+i] > 6 )
				activeParameters[PAR_VOICE_LFO1+i]=1;

			// **LFO par_target_lfo will be an index into modTargets, but we need a parameter number to send
			value = (uint8_t)pgm_read_word(&modTargets[activeParameters[PAR_TARGET_LFO1+i]].param);

			upper = (uint8_t)(((value&0x80)>>7) | (((i)&0x3f)<<1));
			lower = value&0x7f;
			frontPanel_sendData(CC_LFO_TARGET,upper,lower);
		}

		static MidiMsg msg = {0};
		//pre-send fx type to prevent clicks
		/*
		msg.status = CC_2;
		msg.data1 = PAR_FX_TYPE-128; //stupid offset only in CC not CC2
		msg.data2 = activeParameters[PAR_FX_TYPE];

		//	msg.data1 += 1;
		msg.data1 &= 0x7f;
		midiParser_ccHandler(msg,1, 0, 0.f);
		*/

		//try to prevent click by sending BD params first

		//stuff from the morph function
		for(uint32_t i=0;i<END_OF_SOUND_PARAMETERS; i++)
		{

			uint8_t paramNumber = parameterOrder[i];
			if(preset_isTransportParameter(paramNumber))
			{
				soundengine_process();
				continue;
			}

			//const uint8_t morph = activeParameters[PAR_MORPH];

			const uint8_t val = activeParameters[paramNumber];//interpolate(activeParameters[i],morphParameters[i],morph);

			if(paramNumber < 128)
			{
				/* comes from cortex midi handler
				 * stupid offset!
				 * TODO fix this stupid offset			 *
				 * 		frontParser_midiMsg.data1 += 1;

						//because hh slope on front is 127 and on cortex is 0 wrap data1 at 127
						frontParser_midiMsg.data1 &= 0x7f;
				 *
				 */
				msg.status = MIDI_CC;
				msg.data1 = paramNumber;
				msg.data2 = val;

				msg.data1 += 1;
				msg.data1 &= 0x7f;
				midiParser_ccHandler(msg,1, 0, 0.f);


			}
			else
			{
				msg.status = CC_2;
				msg.data1 = paramNumber-128; //stupid offset only in CC not CC2
				msg.data2 = val;

			//	msg.data1 += 1;
				msg.data1 &= 0x7f;
				midiParser_ccHandler(msg,1, 0, 0.f);

			}
			soundengine_process();
		}
}
//----------------------------------------------------
// send loaded parameters to back
void preset_sendDrumsetParameters()
{
	uint8_t i;
	//special case mod targets
	for(i=0;i<6;i++)
	{
		//**VELO load drumkit. translate to param value before sending
		// parameter_values[PAR_VEL_DEST_1+i] is an index into modTargets, we need to send
		// a parameter number
		uint8_t value = (uint8_t)pgm_read_word(&modTargets[activeParameters[PAR_VEL_DEST_1+i]].param);
		uint8_t upper,lower;
		upper = (uint8_t)(((value&0x80)>>7) | (((i)&0x3f)<<1));
		lower = value&0x7f;
		frontPanel_sendData(CC_VELO_TARGET,upper,lower);

		// ensure target voice # is valid
		if(activeParameters[PAR_VOICE_LFO1+i] < 1 || activeParameters[PAR_VOICE_LFO1+i] > 6 )
			activeParameters[PAR_VOICE_LFO1+i]=1;

		// **LFO par_target_lfo will be an index into modTargets, but we need a parameter number to send
		value = (uint8_t)pgm_read_word(&modTargets[activeParameters[PAR_TARGET_LFO1+i]].param);

		upper = (uint8_t)(((value&0x80)>>7) | (((i)&0x3f)<<1));
		lower = value&0x7f;
		frontPanel_sendData(CC_LFO_TARGET,upper,lower);
	}

	// todo will this morph (and fuck up) our modulation targets?
	// send parameters (possibly combined with morph parameters) to back
	preset_morph(activeParameters[PAR_MORPH]);



}
//----------------------------------------------------
/*
char* preset_loadName(uint8_t presetNr, uint8_t nameBufferIdx, uint8_t what)
{
	uint8_t type;

	switch(what) {
	case SAVE_TYPE_PATTERN:
		type=FEXT_PAT;
		break;
	case SAVE_TYPE_KIT:
	case SAVE_TYPE_MORPH:
		type=FEXT_SOUND;
		break;
	case SAVE_TYPE_ALL:
		type=FEXT_ALL;
		break;
	case SAVE_TYPE_PERFORMANCE:
		type=FEXT_PERF;
		break;
	default:
		return NULL; // for glo and sample we don't load a name
	}
	

#if USE_SD_CARD		
	//filename in 8.3  format
	char filename[9];
	preset_makeFileName(filename,presetNr,type);

	//try to open the file
	FRESULT res = f_open((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
	


	if(res!=FR_OK)
	{
		//error opening the file
		memcpy((void*)savefileNames[nameBufferIdx][presetNr],PSTR("Empty   "),8);
		return NULL;	
	}
	
	//file opened correctly -> extract name (first 8 bytes)
	unsigned int bytesRead;
	f_read((FIL*)&SDFile,(void*)savefileNames[nameBufferIdx][presetNr],8,&bytesRead);
	


	//close the file handle
	f_close((FIL*)&SDFile);
	
	return (char*)savefileNames[nameBufferIdx][presetNr];
#else

return "ToDo";
#endif
}
*/

//----------------------------------------------------
/** request step data from the cortex via uart and save it in the provided step struct*/
/*
void preset_queryStepDataFromSeq(uint16_t stepNr)
{
	frontParser_newSeqDataAvailable = 0;

	//request step data
	//the max number for 14 bit data is 16383!!!
	//current max step nr is 128*7*8 = 7168
	frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
	frontPanel_sendByte(stepNr&0x7f);			//lower nibble 7 bit

	//wait until data arrives
	uint8_t newSeqDataLocal = 0;
	uint16_t now = systick_ticks;
	while((newSeqDataLocal==0))
	{
		//we have to call the uart parser to handle incoming messages from the sequencer
		uart_checkAndParse();
		
		newSeqDataLocal = frontParser_newSeqDataAvailable;
		
		if(systick_ticks-now >= 31)
		{
			//timeout
			now = systick_ticks;
			//request step again
			frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
			frontPanel_sendByte(stepNr&0x7f);
		}
	}
}
*/
//----------------------------------------------------
/*
static void preset_sendMainStepDataToSeq( uint16_t mainStepData)
{
	frontPanel_sendByte(mainStepData	& 0x7f);
	frontPanel_sendByte((mainStepData>>7)	& 0x7f);
	frontPanel_sendByte((uint8_t)((mainStepData>>14)	& 0x7f));
}
*/
//----------------------------------------------------
/** send step data from SD card to the sequencer*/
/*
static void preset_sendStepDataToSeq()
{
	frontPanel_sendByte(frontParser_stepData.volume	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.prob	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.note	& 0x7f);

	frontPanel_sendByte(frontParser_stepData.param1Nr	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.param1Val	& 0x7f);

	frontPanel_sendByte(frontParser_stepData.param2Nr	& 0x7f);
	frontPanel_sendByte(frontParser_stepData.param2Val	& 0x7f);

	//now the MSBs from all 7 values
	frontPanel_sendByte((uint8_t) 	(((frontParser_stepData.volume 	& 0x80)>>7) |
							((frontParser_stepData.prob	 	& 0x80)>>6) |
							((frontParser_stepData.note	 	& 0x80)>>5) |
							((frontParser_stepData.param1Nr	& 0x80)>>4) |
							((frontParser_stepData.param1Val	& 0x80)>>3) |
							((frontParser_stepData.param2Nr	& 0x80)>>2) |
							((frontParser_stepData.param2Val	& 0x80)>>1))
							);
}	
*/
//----------------------------------------------------
void preset_queryPatternInfoFromSeq(uint8_t patternNr, uint8_t* next, uint8_t* repeat)
{
	frontParser_newSeqDataAvailable = 0;
	//request pattern info
	frontPanel_sendByte(patternNr);	

	//wait until data arrives
	uint8_t newSeqDataLocal = 0;
	uint16_t now = systick_ticks;
	while((newSeqDataLocal==0))
	{
		//we have to call the uart parser to handle incoming messages from the sequencer
		uart_checkAndParse();
		
		newSeqDataLocal = frontParser_newSeqDataAvailable;
		
		if(systick_ticks-now >= 31)
		{
			//timeout
			now = systick_ticks;
			//request step again
			frontPanel_sendByte(patternNr);	
		}
	}
	
	//the stepdata struct is used as buffer for the data
	*next = frontParser_stepData.volume; 
	*repeat =  frontParser_stepData.prob;
}
//----------------------------------------------------
void preset_queryMainStepDataFromSeq(uint16_t stepNr, uint16_t *mainStepData, uint8_t *length)
{
	frontParser_newSeqDataAvailable = 0;

	//request step data
	//the max number for 14 bit data is 16383!!!
	//current max step nr is 7*8 = 56
	frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
	frontPanel_sendByte(stepNr&0x7f);	//lower nibble 7 bit

	//wait until data arrives
	uint8_t newSeqDataLocal = 0;
	uint16_t now = systick_ticks;
	while((newSeqDataLocal==0))
	{
		//we have to call the uart parser to handle incoming messages from the sequencer
		uart_checkAndParse();
		
		newSeqDataLocal = frontParser_newSeqDataAvailable;
		
		if(systick_ticks-now >= 31)
		{
			//timeout
			now = systick_ticks;
			//request step again
			frontPanel_sendByte((stepNr>>7)&0x7f);		//upper nibble 7 bit
			frontPanel_sendByte(stepNr&0x7f);
		}
	}
	
	// we are reusing these members for purposes other than those that were originally intended
	*mainStepData =(uint16_t) ((frontParser_stepData.volume<<8) | frontParser_stepData.prob);
	*length=frontParser_stepData.note;
};

//----------------------------------------------------
static FRESULT preset_writePatternData(uint8_t patternNr)
{
#if 0
#else
		FRESULT res;
		uint8_t length;

		//---------------  write the step data (note, vol, prob, autom.) ---------------
		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
			res = preset_writeChecked((const void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN);
			if(res != FR_OK) return res;

			res = preset_writeChecked((const void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN);

			if(res != FR_OK) return res;
		}

		//--------------- now the main step data ---------------
		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
			for(uint8_t bar=0;bar<NUM_BARS; bar++)
			{
				const uint16_t mainStepData = seq_patternSet.seq_mainSteps[bar][patternNr][trk];
				res = preset_writeChecked((const void*)&mainStepData,sizeof(uint16_t));
				if(res != FR_OK) return res;
			}
		}

		//----- shuffle setting ------


		//f_write((FIL*)&SDFile,(const void*)&activeParameters[PAR_SHUFFLE],sizeof(uint8_t),&bytesWritten);
		res = preset_writeChecked((const void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t));
		if(res != FR_OK) return res;

		//----- pattern/track lengths ------
		// we reuse the same call from above (when saving main step data)
		// but we only use the length info retrieved. We want to store it at the end
		// to avoid breaking compatibility with save file


		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
				length = seq_patternSet.length[patternNr][trk] ;//seq_patternSet.seq_mainSteps[bar][pat][trk];
				res = preset_writeChecked((const void*)&length,sizeof(uint8_t));
				if(res != FR_OK) return res;
		}

		//write associated kit nr
		res = preset_writeChecked(&seq_patternSet.kitNr[patternNr],sizeof(uint8_t));
		if(res != FR_OK) return res;

#endif
    return FR_OK;
}

//----------------------------------------------------
/*
void preset_savePatternTemp(uint8_t presetNr)
{

	UINT bytesWritten;
	//lcd_clear();
	//lcd_home();
	//lcd_string(PSTR("Saving pattern"));


	//filename in 8.3  format
	char filename[9];
	//sprintf(filename,"p%03d.pat",presetNr);
	preset_makeFileNameTemp(filename,presetNr,FEXT_PAT);
	//open the file
	f_open((FIL*)&SDFile,filename,FA_CREATE_ALWAYS | FA_WRITE);

	//write preset name to file
	f_write((FIL*)&SDFile,(void*)preset_currentName,8,&bytesWritten);

	preset_writePatternData();

	//close the file
	f_close((FIL*)&SDFile);

	//reset the lcd
	menu_repaintAll();


}
*/
//----------------------------------------------------
FRESULT preset_savePatternSD(uint8_t presetNr)
{
#if USE_SD_CARD
    char filename[13];
    FRESULT res;
    if(presetNr >= NUM_PATTERN) return FR_INVALID_PARAMETER;
    preset_makeFileNameSave(filename, presetNr, FEXT_PAT, savefileNames[presetNr][PATTERN_NAMES]);
    res = preset_openShared((FIL*)&SDFile, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) return res;
    res = preset_writeChecked(savefileNames[presetNr][PATTERN_NAMES], 8);
    if(res == FR_OK) res = preset_writeChecked("PatV03", 6);
    if(res == FR_OK) res = preset_writePatternData(presetNr);
    res = preset_finishSave(res);
    if(res == FR_OK) menu_repaintAll();
    return res;
#else
    return FR_NOT_READY;
#endif
}

//----------------------------------------------------
// returns 1 on success
static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
{
	if(patternNr >= NUM_PATTERN) return 0;

#if 0


	// note that the pattern length data is no longer stored in the same way.
	// This means that loading old patterns with non-standard length, the length will be
	// ignored and a standard length used. I think it's acceptable as long as users are aware of this.

	UINT bytesRead;
	uint8_t success=1; // start off succeeding
	//uint16_t i;
	//uint8_t repeat=0;
	uint8_t next=0;

	//lcd_clear();
	//lcd_home();
	//lcd_string(PSTR("Loading pattern"));

	// ---------- Load step data first



	// load step data


	for(uint8_t pat=0; pat<NUM_PATTERN;pat++)
	{
		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{

#if 1

			if( (pat == seq_activePattern) && seq_isRunning() )
			{
				f_read((FIL*)&SDFile,(void*)&seq_tmpPattern.seq_subStepPattern[trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead);
			} else {
				f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[pat][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead);
			}



			if(bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
			{
				success=0;
				break; // file error, or we ran out of file (eof)
			}
#else
			for(uint8_t stp=0;stp<STEPS_PER_PATTERN; stp++)
			{
				f_read((FIL*)&SDFile,(void*)&frontParser_stepData,sizeof(Step),&bytesRead);
				if(bytesRead==sizeof(Step))
				{
					seq_patternSet.seq_subStepPattern[pat][trk][stp] = frontParser_stepData;
				} else {
					success=0;
					break; // file error, or we ran out of file (eof)
				}
			}
#endif
		}
	}

	// ---------- load the main step data next
	uint16_t mainStepData;

	for(uint8_t pat=0; pat<NUM_PATTERN;pat++)
	{
		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
			for(uint8_t bar=0;bar<NUM_BARS; bar++)
			{
				f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead);

				if( bytesRead==sizeof(uint16_t)) {

					if( (pat == seq_activePattern) && seq_isRunning() )
					{
						seq_tmpPattern.seq_mainSteps[bar][trk] = mainStepData;
					} else {
						seq_patternSet.seq_mainSteps[bar][pat][trk] = mainStepData;
					}
				}
				 else {
						success=0;
						break;
					}
			}
		}
	}

	//---------- pattern info (next/repeat)
	/*
	if(success) {
		for(i=0;i<(NUM_PATTERN);i++)
		{
			f_read((FIL*)&preset_File,(void*)&next,sizeof(uint8_t),&bytesRead);
			if(!bytesRead) {
				success=0;
				break;
			}
			f_read((FIL*)&preset_File,(void*)&repeat,sizeof(uint8_t),&bytesRead);
			if(!bytesRead) {
				success=0;
				break;
			}

			if( (i == seq_activePattern) && seq_isRunning() )
			{
				seq_tmpPattern.seq_patternSettings.nextPattern = next;
				seq_tmpPattern.seq_patternSettings.changeBar = repeat;
			} else {
				seq_patternSet.seq_patternSettings[i].nextPattern = next;
				seq_patternSet.seq_patternSettings[i].changeBar = repeat;
			}

		}
	}
	*/

	// ------------ shuffle settings
	if(success) {
		//load the shuffle settings
		f_read((FIL*)&SDFile,(void*)&activeParameters[PAR_SHUFFLE],sizeof(uint8_t),&bytesRead);
		//and update on cortex
		if(bytesRead)
			//frontPanel_sendData(SEQ_CC,SEQ_SHUFFLE,parameter_values[PAR_SHUFFLE]);
			seq_setShuffle(activeParameters[PAR_SHUFFLE]/127.f);
		else {
			success=0;
		}
	}

	// ----------- Pattern/track lengths
	// -- AS this might not exist in the saved data file, we still want success
	if(success) {


		for(uint8_t pat=0; pat<NUM_PATTERN;pat++)
		{
			for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
			{
				f_read((FIL*)&SDFile,(void*)&next,sizeof(uint8_t),&bytesRead);


				if( (pat == seq_activePattern) && seq_isRunning() )
				{
					seq_tmpPattern.length[trk] = next;
				} else {
					seq_patternSet.length[pat][trk] = next;
				}
			}
		}

		// signal new pattern after receiving all the data
		/*
		if( seq_isRunning() ) {
			seq_newPatternAvailable = 1;
		}
		*/

		success=1; // just to document that we don't consider this a failure, we just didn't read the bytes for this
	}

	return success;
#else

	// note that the pattern length data is no longer stored in the same way.
	// This means that loading old patterns with non-standard length, the length will be
	// ignored and a standard length used. I think it's acceptable as long as users are aware of this.

	UINT bytesRead;
	uint8_t success=1; // start off succeeding

	// ---------- Load step data first

	// load step data



	for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
	{

		if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
		{
			success=0;
			break; // file error, or we ran out of file (eof)
		}

		//Pattern V3 and up => flam added
		if(dataVersion > 2)
		{
			if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
			{
				success=0;
				break; // file error, or we ran out of file (eof)
			}
		}

	}



	// ---------- load the main step data next
	uint16_t mainStepData;


		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
		{
			for(uint8_t bar=0;bar<NUM_BARS; bar++)
			{
				if(f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead) == FR_OK && bytesRead==sizeof(uint16_t)) {
					seq_patternSet.seq_mainSteps[bar][patternNr][trk] = mainStepData;
				}
				 else {
						success=0;
						break;
					}
			}
		}



	// ------------ shuffle settings
	if(success) {
		//load the shuffle settings
		if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead) == FR_OK && bytesRead == sizeof(uint8_t))
		{

			//activeParameters[PAR_SHUFFLE] = seq_patternSet.shuffle[patternNr];
			//frontPanel_sendData(SEQ_CC,SEQ_SHUFFLE,parameter_values[PAR_SHUFFLE]);

			//seq_setShuffle(activeParameters[PAR_SHUFFLE]/127.f);
		}
		else {
			success=0;
		}
	}

	if(success) success = preset_readPatternLengths(patternNr, dataVersion);

	return success;
#endif

}
//----------------------------------------------------
/*
uint8_t preset_loadPatternTemp(uint8_t presetNr)
{
#if USE_SD_CARD
	//filename in 8.3  format
	char filename[9];
	UINT bytesRead;
	uint8_t success=0;
	//sprintf(filename,"p%03d.pat",presetNr);
	preset_makeFileNameTemp(filename,presetNr,FEXT_PAT);

	//open the file
	FRESULT res = f_open((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);

	if(res!=FR_OK)
	{

		return 0; //file open error... maybe the file does not exist?
	}


	//read the preset name
	f_read((FIL*)&SDFile,(void*)preset_currentName,8,&bytesRead);
	if(bytesRead != 8)
		goto closeFile;

	if(!preset_readPatternData())
		goto closeFile;

	success=1;
	//close the file handle
closeFile:
	f_close((FIL*)&SDFile);

	//force complete repaint
	menu_repaintAll();
	// return success or failure
	return success;

#else
frontPanel_sendData(PRESET,PATTERN_LOAD,presetNr);

#endif
}
*/
//----------------------------------------------------
FRESULT preset_loadPatternSD(uint8_t presetNr)
{
	if(presetNr >= NUM_PATTERN) return FR_INVALID_PARAMETER;
#if USE_SD_CARD		
	//filename in 8.3  format
	uint8_t fileVersion = 1;
	char filename[13];
	UINT bytesRead;
	uint8_t success=0;
	//sprintf(filename,"p%03d.pat",presetNr);

	/*
	preset_makeFileNameLoad(filename,presetNr,FEXT_PAT);

	//open the file
	FRESULT res = f_open((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
	*/
	FRESULT res = preset_makeFileNameLoad(filename,presetNr,FEXT_PAT);
	if(res==FR_NO_FILE)
	{
		//try old filename
		preset_makeFileNameOld(filename,presetNr,FEXT_PAT);
		res = preset_openShared((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
		if(res!=FR_OK)
			return res; //file open error... maybe the file does not exist?
	}
	
    if(res != FR_OK) return res;

	//read the preset name
	res = f_read((FIL*)&SDFile,(void*)savefileNames[presetNr][PATTERN_NAMES],8,&bytesRead);
	if(res != FR_OK || bytesRead != 8)
		goto closeFile;
	
    char buf[6];
    res = f_read((FIL*)&SDFile, buf, sizeof(buf), &bytesRead);
    if(res != FR_OK || bytesRead != sizeof(buf)) goto closeFile;
    if(!memcmp(buf, "PatV", 4)) {
        if(buf[4] != '0' || buf[5] < '1' || buf[5] > '3') {
            res = FR_INT_ERR;
            goto closeFile;
        }
        fileVersion = buf[5] - '0';
    } else {
        /* Untagged legacy V1: keep the original payload layout. */
        fileVersion = 1;
        res = f_lseek((FIL*)&SDFile, f_tell((FIL*)&SDFile) - sizeof(buf));
        if(res != FR_OK) goto closeFile;
    }

	//f_write((FIL*)&SDFile,"PatV02",6,&bytesWritten);

	if(!preset_readPatternData(presetNr, fileVersion))
	{
		goto closeFile;
	}
	
	success=1;
	//close the file handle



closeFile:
	res = preset_finishRead(res == FR_OK && !success ? FR_INT_ERR : res);
	
	//force complete repaint
	menu_repaintAll();
	// return success or failure
	return res;
	
#else
frontPanel_sendData(PRESET,PATTERN_LOAD,presetNr);

#endif
}

//----------------------------------------------------
FRESULT preset_savePatternRAM(uint8_t presetNr)
{
    if(presetNr >= NUM_PATTERN || seq_activePattern >= NUM_PATTERN) return FR_INVALID_PARAMETER;

	//check if current slot or new slot
	if(presetNr == seq_activePattern)
	{
		//### Current Slot ###
		//write current (changed) pattern to backup buffer, to make changes permanent
		seq_copyPatternToBackupBuffer(presetNr);
	}
	else
	{
		//### NEW SLOT ###
		//- write current (changed) pattern to new slot
		seq_copyPattern(seq_activePattern, presetNr);
		//- change active pattern to new slot (writes backup buffer back to current slot, reverting changes being made in current slot)
		seq_setNextPattern(presetNr);
	}

	return FR_OK;
}
//----------------------------------------------------
FRESULT preset_loadPatternRAM(uint8_t presetNr)
{

	//I'm unused since there is no Pattern Load page, it just uses the pattern change/select page instead
	return FR_OK;
}
//----------------------------------------------------
//called periodically to process non blocking morph
void preset_morph_tick()
{
	//morph_state = 0;
	//return;
	if(morph_state)
	{

		// Consume the reserved slot's tick without executing a MIDI command.
		// All transport slots precede the final sound parameter.
		if(preset_isTransportParameter(morph_index))
		{
			morph_index++;
			return;
		}

		uint8_t morph;

		//Do not morph audio routing
		if((morph_index >= PAR_AUDIO_OUT1) && (morph_index <= PAR_AUDIO_OUT6)) //morph_index++;
		{
			//always use source kit (morph = 0) for audio routing
			morph = 0;
		}
		else
		{
			morph = activeParameters[PAR_MORPH];
		}

		static MidiMsg msg = {0};


		const uint8_t val = interpolate(activeParameters[morph_index],morphParameters[morph_index],morph);


		if(morph_index < 128)
		{
			/* comes from cortex midi handler
			 * stupid offset!
			 * TODO fix this stupid offset			 *
			 * 		frontParser_midiMsg.data1 += 1;

					//because hh slope on front is 127 and on cortex is 0 wrap data1 at 127
					frontParser_midiMsg.data1 &= 0x7f;
			 *
			 */
			msg.status = MIDI_CC;
			msg.data1 = morph_index;
			msg.data2 = val;

			msg.data1 += 1;
			msg.data1 &= 0x7f;
			midiParser_ccHandler(msg,1, 0, 0.f);


		}
		else
		{
			msg.status = CC_2;
			msg.data1 = morph_index-128; //stupid offset only in CC not CC2
			msg.data2 = val;

		//	msg.data1 += 1;
			msg.data1 &= 0x7f;
			midiParser_ccHandler(msg,1, 0, 0.f);

		}


		morph_index++;
		if(morph_index >= END_OF_SOUND_PARAMETERS)
		{
			//finished morphing
			morph_state = 0;
		}
	}
}
//----------------------------------------------------
// This will determine an interpolated value between parameters
// and morph parameters and send the data to the back
// if the morph value is 0 it will just send the parameter values
// to the back
void preset_morph(uint8_t morph)
{
#if 1
	//initialize new morph
	morph_state = 1; //start morph
	morph_index = 0; //reset index
#else
	int i;
	//TODO so far only sound parameters are interpolated, no LFOs etc

	for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
	{


		uint8_t val;
		
		val = interpolate(mainParameters[i],morphParameters[i],morph);

		if(i<128) {
			frontPanel_sendData(MIDI_CC,(uint8_t)i,val);

		} else {
			frontPanel_sendData(CC_2,(uint8_t)(i-128),val);

		}		
		
			
		//to omit front panel button/LED lag we have to process din dout and uart here
		//read next button

		//update LEDs
		dio_process();
		//read uart messages from sequencer
		uart_checkAndParse();
	}		
#endif
}

//----------------------------------------------------
uint8_t preset_getMorphValue(uint16_t index, uint8_t morph)
{
	return interpolate(activeParameters[index],morphParameters[index],morph);
};
//----------------------------------------------------

FRESULT preset_createProject(uint8_t presetNr)
{
    if(presetNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
    FRESULT shared = preset_sharedStatus();
    if(shared != FR_OK) return shared;


	//create default globals

	//clear all patterns

	//clear all kits

	//clear all songs

	//load init kit


	int i;

	preset_loadDefaultGlobals();
	//give active song pointer a proper value
	seq_activeSong = seq_songData[0];

	memset(seq_songData, SONG_END, sizeof(uint8_t) * MAX_SONG_LENGTH * NUM_KIT_SLOTS);
	seq_songData[0][0] = 0;


	//memset(seq_stepIndex,0,NUM_TRACKS);
//	memset(seq_lastMasterStep,0,NUM_TRACKS);


	for(i=0;i<NUM_PATTERN;i++)
	{
		//seq_patternSet.seq_patternSettings[i].changeBar 	= 0;	//default setting: zero repeats (play once then change)
		//seq_patternSet.seq_patternSettings[i].nextPattern 	= i;	//default setting: repeat same pattern
		seq_clearPattern(i); // will clear all tracks in the pattern
		preset_init(i); //set all kits to init sound
	}

	sprintf(savefileNames[presetNr][PROJECT_NAMES], "Init");

	return preset_clearTmpFolder();
}
//----------------------------------------------------
// **SAVEALL
// save all settings (global, pattern, kit)
// or save performance settings (pattern, kit)

FRESULT preset_saveProject(uint8_t presetNr)
{
    FRESULT res = preset_sharedStatus();
    uint8_t entered = 0;
    char folder[8];
    if(res != FR_OK) return res;
    res = preset_projectPath(folder, sizeof(folder), presetNr);
    if(res != FR_OK) return res;
    if(menu_selectedPresetNr[SAVE_TYPE_KIT] >= NUM_KIT_SLOTS || seq_activePattern >= NUM_PATTERN)
        return FR_INVALID_PARAMETER;
    for(uint8_t i = 0; i < NUM_PATTERN; i++)
        if(seq_patternSet.kitNr[i] >= NUM_KIT_SLOTS) return FR_INT_ERR;
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Start_IT(&htim2);
#endif
    /* S4b-1 containment: even empty existing directories are never replaced. */
    res = preset_createProjectFolder(folder);
    if(res != FR_OK) goto done;
    res = f_chdir(folder);
    if(res != FR_OK) goto done;
    entered = 1;
    memcpy(currentFolder, folder, sizeof(folder));
    preset_saveDrumsetRam(menu_selectedPresetNr[SAVE_TYPE_KIT], 0);
    res = preset_savePatternRAM(seq_activePattern);
    if(res != FR_OK) goto done;
    lcd_process_display_interrupt = 0;
    while(sendDisplayBuffer() != 0);
    lcd_clear();
    lcd_home();
    lcd_string(PSTR("Saving Project"));
    res = preset_openShared((FIL*)&SDFile, "prj.nfo", FA_CREATE_NEW | FA_WRITE);
    if(res != FR_OK) goto done;
    res = preset_finishSave(preset_writeChecked(savefileNames[presetNr][PROJECT_NAMES], 8));
    if(res != FR_OK) goto done;
    res = preset_saveGlobals();
    if(res != FR_OK) goto done; /* No later writes or temporary-file deletion. */
    //save all drumkits, patterns and songs
    for(uint8_t i = 0; i < NUM_KIT_SLOTS; i++) {
        char text[17];
        snprintf(text, sizeof(text), "%u%%", (unsigned)i * 100u / NUM_KIT_SLOTS);
        lcd_setcursor(0,2);
        lcd_string(text);
        res = preset_saveDrumsetSD(i, 0);
        if(res != FR_OK) goto done;
        res = preset_savePatternSD(i);
        if(res != FR_OK) goto done;
        res = preset_saveSongSD(i);
        if(res != FR_OK) goto done;
    }
    res = preset_clearTmpFolder();
done:
    lcd_process_display_interrupt = 1;
    if(entered && preset_sharedStatus() == FR_OK) {
        FRESULT restored = preset_initTmpFolder();
        if(res == FR_OK) res = restored;
    }
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Stop_IT(&htim2);
#endif
    return res;
}

//----------------------------------------------------
FRESULT preset_loadSongSD(uint8_t songNr)
{
    if(songNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
    char filename[13], name[8];
    uint8_t data[MAX_SONG_LENGTH];
    UINT count;
    FRESULT res = preset_makeFileNameLoad(filename, songNr, FEXT_SONG);
    if(res == FR_NO_FILE) {
        preset_makeFileNameOld(filename, songNr, FEXT_SONG);
        res = preset_openShared((FIL*)&SDFile, filename, FA_READ);
    }
    if(res != FR_OK) return res;
    res = f_read((FIL*)&SDFile, name, sizeof(name), &count);
    if(res == FR_OK && count != sizeof(name)) res = FR_INT_ERR;
    if(res == FR_OK) {
        res = f_read((FIL*)&SDFile, data, sizeof(data), &count);
        if(res == FR_OK && count != sizeof(data)) res = FR_INT_ERR;
    }
    res = preset_finishRead(res);
    if(res != FR_OK) return res;
    if(preset_isNameEmpty(name)) memcpy(name, "Empty\0\0", 8);
    memcpy(savefileNames[songNr][SONG_NAMES], name, sizeof(name));
    memcpy(seq_songData[songNr], data, sizeof(data));
    return FR_OK;
}
//----------------------------------------------------
FRESULT preset_saveSongSD(uint8_t songNr)
{
    char filename[13];
    FRESULT res;
    if(songNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
    preset_makeFileNameSave(filename, songNr, FEXT_SONG, savefileNames[songNr][SONG_NAMES]);
    res = preset_openShared((FIL*)&SDFile, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) return res;
    res = preset_writeChecked(savefileNames[songNr][SONG_NAMES], 8);
    if(res == FR_OK) res = preset_writeChecked(seq_songData[songNr], MAX_SONG_LENGTH);
    return preset_finishSave(res);
}


//----------------------------------------------------
void preset_loadProjectNames(uint8_t projectNr)
{
    if(preset_sharedStatus() != FR_OK) return;
    char folder[8], path[21], name[8];
    UINT count;
    if(preset_projectPath(folder, sizeof(folder), projectNr) != FR_OK) return;
    if(preset_childPath(path, sizeof(path), folder, "prj.nfo") != FR_OK) return;
    FRESULT res = preset_openShared((FIL*)&SDFile, path, FA_READ);
    if(res == FR_NO_FILE || res == FR_NO_PATH) {
        memcpy(savefileNames[projectNr][PROJECT_NAMES], "Empty\0\0", 8);
        return;
    }
    if(res != FR_OK) return;
    res = f_read((FIL*)&SDFile, name, sizeof(name), &count);
    if(res == FR_OK && count != sizeof(name)) res = FR_INT_ERR;
    res = preset_finishRead(res);
    if(res == FR_OK) {
        if(preset_isNameEmpty(name)) memcpy(name, "Empty\0\0", 8);
        memcpy(savefileNames[projectNr][PROJECT_NAMES], name, sizeof(name));
    }
}
//----------------------------------------------------
FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
{
    FRESULT res = preset_sharedStatus();
    FILINFO info;
    uint8_t entered = 0;
    char folder[8], path[21], name[8];
    UINT count;
    if(res != FR_OK) return res;
    res = preset_projectPath(folder, sizeof(folder), presetNr);
    if(res != FR_OK) return res;
    if(seq_activePattern >= NUM_PATTERN) return FR_INVALID_PARAMETER;
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Start_IT(&htim2);
#endif
    res = f_stat(folder, &info);
    if(res != FR_OK) goto done; /* No mkdir or tmp cleanup while validating a load. */
    if(!(info.fattrib & AM_DIR)) { res = FR_NO_PATH; goto done; }
    res = preset_childPath(path, sizeof(path), folder, "prj.nfo");
    if(res != FR_OK) goto done;
    res = preset_openShared((FIL*)&SDFile, path, FA_READ);
    /* An existing project without metadata is invalid, not a request to create. */
    if(res == FR_NO_FILE) res = FR_INT_ERR;
    if(res != FR_OK) goto done;
    res = f_read((FIL*)&SDFile, name, sizeof(name), &count);
    if(res == FR_OK && count != sizeof(name)) res = FR_INT_ERR;
    res = preset_finishRead(res);
    if(res != FR_OK) goto done;
    res = f_chdir(folder);
    if(res != FR_OK) goto done;
    entered = 1;
    memcpy(currentFolder, folder, sizeof(folder));
    lcd_process_display_interrupt = 0;
    while(sendDisplayBuffer() != 0);
    lcd_clear();
    lcd_home();
    lcd_string(PSTR("Loading Project"));
    res = preset_loadGlobalFile();
    if(res == FR_NO_FILE) {
        memset(&activeParameters[PAR_BEGINNING_OF_GLOBALS], 0, NUM_PARAMS - PAR_BEGINNING_OF_GLOBALS);
        preset_loadDefaultGlobals();
        res = FR_OK;
    }
    if(res != FR_OK) goto done;
    menu_sendAllGlobals();
    for(uint8_t i = 0; i < NUM_KIT_SLOTS; i++) {
        char text[17];
        snprintf(text, sizeof(text), "%u%%", (unsigned)i * 100u / NUM_KIT_SLOTS);
        lcd_setcursor(0,2);
        lcd_string(text);
        res = preset_loadDrumsetSD(i, 0);
        if(res == FR_NO_FILE) { preset_defaultKit(i); res = FR_OK; }
        if(res != FR_OK) goto done;
        res = preset_loadPatternSD(i);
        if(res == FR_NO_FILE) {
            seq_clearPattern(i);
            seq_patternSet.shuffle[i] = 0;
            seq_patternSet.kitNr[i] = 0;
            memcpy(savefileNames[i][PATTERN_NAMES], "Empty\0\0", 8);
            res = FR_OK;
        }
        if(res != FR_OK) goto done;
        res = preset_loadSongSD(i);
        if(res == FR_NO_FILE) {
            memset(seq_songData[i], SONG_END, MAX_SONG_LENGTH);
            memcpy(savefileNames[i][SONG_NAMES], "Empty\0\0", 8);
            res = FR_OK;
        }
        if(res != FR_OK) goto done;
    }
    if(preset_isNameEmpty(name)) memcpy(name, "Empty\0\0", 8);
    memcpy(savefileNames[presetNr][PROJECT_NAMES], name, sizeof(name));
    morphParameters = mainParameters[0];
    uint8_t kit = 0;
    if(activeParameters[PAR_PATTERN_MODE] >= 1) {
        activeParameters[PAR_PATTERN_MODE] = 1;
        kit = seq_patternSet.kitNr[seq_activePattern];
    }
    if(kit >= NUM_KIT_SLOTS) { res = FR_INT_ERR; goto done; }
    preset_loadDrumsetRam(kit, 0, 1);
    menu_selectedPresetNr[SAVE_TYPE_KIT] = menu_loadedPresetNr[CURRENT_KIT] = kit;
    seq_copyPatternToBackupBuffer(seq_activePattern);
    res = preset_saveLastUsedProjectNrToSD(presetNr);
    if(res != FR_OK) goto done;
    seq_setNextPattern(0);
    if(clearTmp) res = preset_clearTmpFolder(); /* Only after a completed load. */
done:
    lcd_process_display_interrupt = 1;
    if(entered && preset_sharedStatus() == FR_OK) {
        FRESULT restored = preset_initTmpFolder();
        if(res == FR_OK) res = restored;
    }
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Stop_IT(&htim2);
#endif
    return res;
}
//----------------------------------------------------

static void preset_makeFileNameOld(char *buf, uint8_t num, uint8_t type)
{

	const char *ext;
	buf[0]='p';
	numtostrpu(&buf[1],num,'0');
	switch(type) {
	case FEXT_SOUND:
		ext=PSTR(".snd");
		break;
	case FEXT_PAT:
		ext=PSTR(".pat");
		break;
	case FEXT_ALL:
		ext=PSTR(".all");
		break;
	default:
	case FEXT_PROJ:
		ext=PSTR(".prj");
		break;

	case FEXT_SONG:
		ext=PSTR(".sng");
		break;
	}
	strcpy(&buf[4],ext);

};


//load uses wildcard to find a name of the pattern 00-*.ext
static FRESULT preset_makeFileNameLoad(char *buf, uint8_t num, uint8_t type)
{
    FRESULT shared = preset_sharedStatus();
    if(num >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
    if(shared != FR_OK) return shared;

	FRESULT fr;     /* Return value */
    DIR dj = {0};   /* Safe to close only after a successful directory open. */
    FILINFO fno;    /* File information */

	const char *ext;
	numtostrpu_2Bytes(&buf[0],num,'0');
	buf[2] = '-';
	buf[3] = '*';

	switch(type) {
	case FEXT_SOUND:
		ext=PSTR(".snd");
		break;
	case FEXT_PAT:
		ext=PSTR(".pat");
		break;
	case FEXT_ALL:
		ext=PSTR(".all");
		break;
	default:
	case FEXT_PROJ:
		ext=PSTR(".prj");
		break;

	case FEXT_SONG:
		ext=PSTR(".sng");
		break;
	}
	strcpy(&buf[4],ext);
	buf[8] = 0;


    fr = f_findfirst(&dj, &fno, "", buf);  /* Start to search for preset files */
    if(dj.obj.fs) {
        FRESULT closed = f_closedir(&dj);
        if(fr == FR_OK) fr = closed;
    }
    if(fr != FR_OK) return fr;
    if(fr == FR_OK && fno.fname[0])
    {
    fr = preset_openShared((FIL*)&SDFile,fno.fname,FA_OPEN_EXISTING | FA_READ);
    	return fr;
    }

    return FR_NO_FILE;
}

void clearIllegalCharacters(char *input, char* output, uint8_t sz)
{
	for(uint8_t i=0; i<sz; i++)
	{
		if((input[i] >= 'a') && (input[i] <= 'z')) output[i] = input[i];
		else if((input[i] >= 'A') && (input[i] <= 'Z')) output[i] = input[i];
		else if((input[i] >= '0') && (input[i] <= '9')) output[i] = input[i];
		//! # $ % & ' ( ) - @ ^ _ ` { } ~
		else if(input[i] == ' ')  output[i] = '_';
		else if(input[i] == '!')  output[i] = input[i];
		else if(input[i] == '#')  output[i] = input[i];
		else if(input[i] == '$')  output[i] = input[i];
		else if(input[i] == '%')  output[i] = input[i];
		else if(input[i] == '&')  output[i] = input[i];
		else if(input[i] == '(')  output[i] = input[i];
		else if(input[i] == ')')  output[i] = input[i];
		else if(input[i] == '-')  output[i] = input[i];
		else if(input[i] == '@')  output[i] = input[i];
		else if(input[i] == '^')  output[i] = input[i];
		else if(input[i] == '_')  output[i] = input[i];
		else if(input[i] == '`')  output[i] = input[i];
		else if(input[i] == '{')  output[i] = input[i];
		else if(input[i] == '}')  output[i] = input[i];
		else if(input[i] == '~')  output[i] = input[i];
		else if(input[i] == 0)  output[i] = input[i];
		else output[i] = '_';
	}

}
//save uses the kit name to create a filename eg 00-NAME.ext
static void preset_makeFileNameSave(char *buf, uint8_t num, uint8_t type, char *name)
{

	char clearedName[9];
	clearedName[8] = 0;
	clearIllegalCharacters(name , clearedName, 8);
	//name scheme: 01-NAME.ext
	const char *ext;
	numtostrpu_2Bytes(&buf[0],num,'0');
	buf[2] = '-';

	//copy the first 5 characters of the name to the filename if not empty
	uint8_t i=0;
	for( ; i<5; i++)
	{
		if(clearedName[i] != 0)
		{
			if(clearedName[i] == ' ')
				buf[3+i] = '_';
			else
				buf[3+i] = clearedName[i];
		}
		else
		{
			break;
		}
	}

	switch(type) {
	case FEXT_SOUND:
		ext=PSTR(".snd");
		break;
	case FEXT_PAT:
		ext=PSTR(".pat");
		break;
	case FEXT_ALL:
		ext=PSTR(".all");
		break;
	default:
	case FEXT_PROJ:
		ext=PSTR(".prj");
		break;

	case FEXT_SONG:
		ext=PSTR(".sng");
		break;
	}
	strcpy(&buf[3+i],ext);
}

//----------------------------------------------------
/*
static void preset_makeFileNameTemp(char *buf, uint8_t num, uint8_t type)
{
	const char *ext;
	buf[0]='t';
	numtostrpu(&buf[1],num,'0');
	switch(type) {
	case 0:
		ext=PSTR(".snd");
		break;
	case 1:
		ext=PSTR(".pat");
		break;
	case 2:
		ext=PSTR(".all");
		break;
	default:
	case 3:
		ext=PSTR(".prf");
		break;
	}
	strcpy(&buf[4],ext);
}
*/
