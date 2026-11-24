/* Actual source bodies via input_probe.py, with boundary adapters. */
#define main previous_host_main
#include "host.c"
#undef main
#include <stdlib.h>
enum {LOAD_PAGE=8,SAVE_PAGE=9,SONG_MODE_PAGE=10,MENU_LAST_STEP_PAGE=11,SEQ_PAGE=12,
      SELECT_MODE_LAST_STEP_TRACK=1,SELECT_MODE_LAST_STEP_ALL=2,MODE_CLEAR=1,
      CLEAR_AUTOMATION2=5,NUM_SUB_PAGES=8,TEXT_EMPTY=0,PAR_BPM=1,DTYPE_MENU=1};
static uint8_t menu_activeVoice, track_len=32, track_writes, normal_moves, knob_writes;
static volatile int8_t encDetents;
static uint8_t buttonHandler_getMode(void){return SELECT_MODE_LAST_STEP_TRACK;}
static uint8_t seq_getTrackLength(uint8_t v){(void)v;return track_len;}
static void seq_setTrackLength(uint8_t v,uint8_t l){(void)v;track_len=l;++track_writes;}
static uint8_t menu_voiceTransferEncoder(int8_t i,uint8_t b){(void)i;(void)b;return menu_voiceCopyActive||menu_voiceCopyDrain;}
static void copyClear_executeClear(void){assert(0);}
static void menu_handleLoadSaveMenu(int8_t i,uint8_t b){(void)i;(void)b;assert(0);}
static void menu_handleSongMenu(int8_t i,uint8_t b){(void)i;(void)b;assert(0);}
static uint8_t copyClear_isClearModeActive(void){return copyClear_Mode==MODE_CLEAR;}
static uint8_t copyClear_getClearTarget(void){return 0;}
static void copyClear_setClearTarget(uint8_t t){(void)t;}
static void menu_encoderChangeParameter(int8_t i){(void)i;++normal_moves;}
static void menu_moveToMenuItem(int8_t i){(void)i;++normal_moves;}
static void menu_repaint(void){}
typedef struct {uint8_t top1;uint16_t bot1;uint16_t botRest[7];} HostPage;
static HostPage menuPages[16][NUM_SUB_PAGES];
static uint8_t has2ndPage(uint8_t p){return p==0;}
#define pgm_read_byte(p) (*(p))
#define pgm_read_word(p) (*(p))
#define AVR 0
#define USE_ATAN2 0
#define HYSTERESIS 40
static uint16_t adc_buf[8],adc_buf_old[8],adc_potValues[4];
static uint8_t adc_idx,activeParameters[256];
static uint8_t getMenuIndx(void){return menuIndex;}
static uint8_t menu_getDisplayParameterType(uint8_t p){(void)p;return 0;}
static uint8_t menu_parseKnobValue(uint8_t p,int16_t d){(void)p;(void)d;++knob_writes;return 0;}
#include "production.inc"
static void poll(uint8_t b){menu_parseEncoder(encode_read4(),b);}
static void config(void){
    if(!menu_diagConfig())menu_switchSubPage(menuIndex>>PAGE_SHIFT,1);
}
int main(void){
    menuPages[MENU_MIDI_PAGE][1].top1=1;
    menuIndex=0;
    config();assert(menuIndex==4&&!menu_diagVisible);
    config();assert(menuIndex==8&&!menu_diagVisible);
    config();assert(menu_diagVisible&&menuIndex==8);
    config();assert(!menu_diagVisible&&menuIndex==0);
    poll(0);
    puts("PASS three real CONFIG subpage transitions and overlay wrap");
    unsigned n=0;
    for(int held=0;held<=1;held++)for(int depth=0;depth<=4;depth++)for(int sign=-1;sign<=1;sign+=2){
        menu_activePage=MENU_MIDI_PAGE;menuIndex=8;
        assert(menu_diagConfig());poll(0);
        menu_diagClose();menu_activePage=MENU_LAST_STEP_PAGE;
        encDetents=(int8_t)(depth*sign);track_len=32;track_writes=0;
        if(held){poll(1);assert(menu_diagDrain);poll(1);}
        for(int p=0;p<depth+2;p++)poll(0);
        if(track_len!=32||track_writes){
            printf("OLD QUEUE LEAK depth=%d sign=%d held=%d length=%u writes=%u\n",depth,sign,held,track_len,track_writes);
            return 42;
        }
        assert(!menu_diagDrain);
        encDetents=(int8_t)sign;poll(0);assert(track_len==32+sign&&track_writes==1);++n;
    }
    printf("PASS %u queue depth/direction/held-release cases; fresh input delivered\n",n);
    menu_activePage=MENU_MIDI_PAGE;menuIndex=8;
    assert(menu_diagConfig());poll(0);
    for(int i=0;i<4;i++){adc_buf[2*i]=48000;adc_buf[2*i+1]=16000;}
    adc_checkPots();assert(!knob_writes);
    menu_diagClose();adc_checkPots();assert(!knob_writes);
    for(int i=0;i<4;i++){
        assert(adc_buf_old[2*i]==48000);
        assert(adc_potValues[i]==calcPosition(3000,1000)/2);
    }
    adc_checkPots();assert(!knob_writes);
    adc_buf[0]=49000;adc_buf[1]=18000;
    adc_checkPots();assert(knob_writes==1);
    puts("PASS actual ADC scan suppression, all-four rebase, fresh knob input");
    menu_activePage=MENU_MIDI_PAGE;menuIndex=8;
    assert(menu_diagConfig());poll(0);
    menu_sdError=1;poll(0);assert(!menu_diagVisible);menu_sdError=0;
    puts("PASS SD notice closes overlay before normal encoder");
    return 0;
}
