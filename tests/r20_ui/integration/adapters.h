#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Soundengine/Sequencer/automationCapture.h"
#include "Soundengine/MIDI/MidiMessages.h"
#define AVR 0
#define USE_PATTERN_EDIT_BUFFER 0
#define DISABLE_CONV_WARNING
#define END_DISABLE_CONV_WARNING
#define pgm_read_byte(address) (*(address))
#define pgm_read_word(address) (*(address))
#define SEQ_CC 0xd2
#define SEQ_REC_ON_OFF FRONT_SEQ_REC_ON_OFF
#define SEQ_SELECT_ACTIVE_STEP FRONT_SEQ_SELECT_ACTIVE_STEP
#define SET_P1_DEST FRONT_SET_P1_DEST
#define SET_P2_DEST FRONT_SET_P2_DEST
#define SET_P1_VAL FRONT_SET_P1_VAL
#define SET_P2_VAL FRONT_SET_P2_VAL
#define SELECT_MODE_VOICE 0
#define LED_REC 0
#define PRESET_PAT_TIMER 0
#define PRESET_STOP_PAT_TIMER 1
enum { FR_OK, FR_NO_FILE, FR_NO_PATH, FR_INVALID_NAME, FR_INVALID_DRIVE,
       FR_INVALID_PARAMETER, FR_EXIST, FR_LOCKED, FR_TOO_MANY_OPEN_FILES,
       FR_DENIED, FR_WRITE_PROTECTED, FR_NO_FILESYSTEM, FR_NOT_READY, FR_NOT_ENABLED };

enum { LED_STEP1 = 1, LED_STEP2, LED_STEP3, LED_STEP4, LED_STEP5, LED_STEP6,
       LED_STEP7, LED_STEP8, LED_STEP9, LED_STEP10, LED_STEP11, LED_STEP12,
       LED_STEP13, LED_STEP14, LED_STEP15, LED_STEP16 };
#define STEP_ACTIVE_MASK 0x80
