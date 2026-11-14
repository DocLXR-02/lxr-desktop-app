/* Unchanged mode/voice/step helpers are adapters; public dispatchers are exact. */
static struct {unsigned selectButtonMode:4,seqRunning:1,seqRecording:1,seqErasing:1;} buttonHandler_stateMemory;
uint8_t menu_activePatternBank,menu_activeBar;
static unsigned voice_calls,step_calls,part_calls,mode_calls,track_copies;
static void buttonHandler_voiceButtonPressed(uint8_t v){voice_calls++;if(copyClear_Mode==MODE_COPY_TRACK)track_copies++;else menu_activeVoice=v;}
static void buttonHandler_selectButtonPressed(uint8_t v){(void)v;part_calls++;}
static void buttonHandler_seqButtonPressed(uint8_t v){(void)v;step_calls++;}
static void buttonHandler_handleModeButtons(uint8_t v){(void)v;mode_calls++;}
static void buttonHandler_partButtonReleased(uint8_t v){(void)v;}
static void buttonHandler_seqButtonReleased(uint8_t v){(void)v;}
void buttonHandler_setRunStopState(uint8_t v){buttonHandler_stateMemory.seqRunning=v;running=v;}
void buttonHandler_showMuteLEDs(void){}
void led_clearAllBlinkLeds(void){}
void led_clearSelectLeds(void){}
void led_clearVoiceLeds(void){}
void led_setValue(uint8_t a,uint8_t b){(void)a;(void)b;}
void led_setBlinkLed(uint8_t a,uint8_t b){(void)a;(void)b;}
void led_setActiveVoice(uint8_t v){(void)v;}
void led_setMode2(uint8_t v){(void)v;}
void led_updateBarLeds(void){}
void led_updateSeqLeds(void){}
void led_updateStepLength(void){}
uint8_t led_idx2StepLed(uint8_t v){return v;}
void menu_switchSubPage(uint8_t a,uint8_t b){(void)a;(void)b;}
void menu_switchPage(uint8_t p){menu_activePage=p;}
void seq_setSongModeEnabled(uint8_t v){(void)v;}
void seq_insertRow(void){}
void seq_deleteRow(void){}
void copyClear_armClearMenu(uint8_t v){(void)v;}
void led_initPatternSelectLeds(void){}
#include "buttons.inc"
static void panel_press(uint8_t button){if(button==BUT_COPY)host_copy=1;if(button==BUT_SHIFT)host_shift=1;buttonHandler_buttonPressed(button);}
static void panel_release(uint8_t button){if(button==BUT_COPY)host_copy=0;if(button==BUT_SHIFT)host_shift=0;buttonHandler_buttonReleased(button);}
