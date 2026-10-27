// Host seams around original source bodies; never execute firmware or touch hardware.
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Soundengine/Sequencer/sequencer.h"
#include "Soundengine/MIDI/CortexParser.h"
#include "Soundengine/MIDI/MidiMessages.h"
#include "Soundengine/MIDI/MidiVoiceControl.h"
#include "Preset/PresetManager.h"
#include "menu/menu.h"
#include "frontPanelParser_ui.h"
#include "ledHandler.h"
#ifdef BASELINE_GENERATOR
// Oracle only: model ARM CLZ(0)=32 for the pre-fix builtin. Candidate has no shim.
#undef __CLZ
#define __CLZ(x) ((uint32_t)(x) ? __builtin_clz((uint32_t)(x)) : 32)
#include "baseline-generator.inc"
#else
#include "Core/Src/Soundengine/Sequencer/EuklidGenerator.c"
#endif

PatternSet seq_patternSet;
PatternBackupBuffer patternBackupBuffer;
uint8_t seq_activePattern, seq_pendingPattern, seq_running, seq_recordActive;
static uint8_t seq_mutedTracks;
static int8_t seq_stepIndex[NUM_TRACKS];
int8_t seq_stepToSkip[NUM_TRACKS];
uint8_t lastWasNoteOn[NUM_TRACKS], chokeCurrent, chokeNext;
struct { uint8_t prev, current, next; } seq_rndValue[NUM_TRACKS];
uint8_t frontParser_shownPattern, frontParser_activeTrack;
uint8_t frontParser_sysexActive, frontParser_rxCnt;
uint16_t frontParser_sysexSeqStepNr, frontParser_twoByteData;
static uint8_t frontParser_sysexBuffer[7];
uint8_t menu_activeBar, menu_activePage, menu_playedPattern, menu_shownPattern;
uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS];
char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];
TIM_HandleTypeDef htim2;
FIL SDFile;
static FATFS host_filesystem;
static FRESULT preset_sharedFailure;
static uint8_t preset_autosavePaused;
#define STEPS_PER_PATTERN NUM_STEPS
#define FEXT_PAT 1
static uint8_t stream[65536], saved_stream[65536];
static size_t stream_pos, stream_size, error_at = SIZE_MAX;
static float elapsed = 75.f;
static unsigned triggers;
FRESULT f_open(FIL *file, const TCHAR *name, BYTE flags) {
    (void)name; stream_pos=0; file->fptr=0; file->obj.fs=&host_filesystem;
    if(flags & FA_CREATE_ALWAYS) stream_size=0;
    return FR_OK;
}
FRESULT f_close(FIL *file) { file->obj.fs=0; return FR_OK; }
FRESULT f_lseek(FIL *file, DWORD offset) {
    assert(offset<=stream_size); stream_pos=offset; file->fptr=offset; return FR_OK;
}
FRESULT f_read(FIL *file, void *dst, UINT count, UINT *actual) {
    assert(stream_pos<=stream_size);
    size_t old=stream_pos;
    size_t n=count < stream_size-stream_pos ? count : stream_size-stream_pos;
    memcpy(dst,stream+stream_pos,n); stream_pos+=n; file->fptr=stream_pos; *actual=n;
    // Deliberately return an error with a full byte count too: callers must check both.
    return old>=error_at ? FR_DISK_ERR : FR_OK;
}
FRESULT f_write(FIL *file, const void *src, UINT count, UINT *actual) {
    assert(stream_pos+count<=sizeof(stream)); memcpy(stream+stream_pos,src,count);
    stream_pos+=count; file->fptr=stream_pos;
    if(stream_pos>stream_size) stream_size=stream_pos;
    *actual=count; return FR_OK;
}
static FRESULT preset_makeFileNameLoad(char *buf,uint8_t num,uint8_t type) {
    (void)buf; (void)num; (void)type; return f_open(&SDFile,"pattern",FA_READ);
}
static void preset_makeFileNameSave(char *buf,uint8_t num,uint8_t type,char *name) {
    (void)num; (void)type; (void)name; strcpy(buf,"pattern");
}
static void preset_makeFileNameOld(char *buf,uint8_t num,uint8_t type) {
    (void)num; (void)type; strcpy(buf,"pattern");
}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *timer) { (void)timer; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *timer) { (void)timer; return HAL_OK; }
void menu_repaintAll(void) {}
void seq_sendStepInfoToFront(uint16_t step) { (void)step; }
void frontPanel_sendData(uint8_t a,uint8_t b,uint8_t c) {
    if(a==SEQ_CC && b==SEQ_SET_SHOWN_PATTERN) frontParser_shownPattern=c;
}
void seq_triggerVoice(uint8_t voice,uint8_t vel,uint8_t note) {
    assert(voice<NUM_TRACKS && vel<=127 && note==60); triggers++;
}
void voiceControl_noteOff(uint8_t voice) { assert(voice<NUM_TRACKS); }
float seq_getElapsedStepTimePercent(void) { return elapsed; }
uint8_t seq_determineNextPattern(uint8_t change) { (void)change; abort(); }
void led_setValue(uint8_t value,uint8_t led) { (void)value; (void)led; }
#include "functions.inc"

static void reset(void) {
    memset(&SDFile,0,sizeof SDFile); preset_sharedFailure=FR_OK; preset_autosavePaused=0;
    memset(&seq_patternSet,0,sizeof(seq_patternSet));
    for(unsigned p=0;p<NUM_PATTERN;p++) for(unsigned t=0;t<NUM_TRACKS;t++)
        seq_patternSet.length[p][t]=16;
    seq_activePattern=seq_pendingPattern=frontParser_shownPattern=menu_playedPattern=0;
    frontParser_activeTrack=menu_activeBar=menu_shownPattern=0;
    menu_activePage=PERF_PAGE; seq_running=seq_recordActive=1; seq_mutedTracks=0x7e;
    memset(seq_stepIndex,0,sizeof(seq_stepIndex)); error_at=SIZE_MAX;
    stream_pos=stream_size=0; elapsed=75.f; triggers=0; euklid_init();
}
static void lengths_unchanged(unsigned pattern) {
    for(unsigned t=0;t<NUM_TRACKS;t++) assert(seq_patternSet.length[pattern][t]==t+1);
}
static void old_lengths(unsigned pattern) {
    for(unsigned t=0;t<NUM_TRACKS;t++) seq_patternSet.length[pattern][t]=t+1;
}
static void generation(void) {
    for(unsigned length=1;length<=16;length++) for(unsigned pulses=1;pulses<=length;pulses++) {
        euklid_init(); euklid_setLength(0,length); euklid_setSteps(0,pulses,0);
        for(unsigned rotation=0;rotation<16;rotation++) {
            euklid_setRotation(0,rotation,0);
#ifndef BASELINE_GENERATOR
            assert(seq_patternSet.length[0][0]==length);
#endif
            assert(__builtin_popcountll(euklid_patternBuffer.quadword)==pulses);
            assert((euklid_patternBuffer.quadword>>length)==0);
            printf("%u,%u,%u,%016" PRIx64 "\n",length,pulses,rotation,euklid_patternBuffer.quadword);
        }
    }
}
static void bounds(void) {
    for(unsigned t=0;t<NUM_TRACKS;t++) for(unsigned p=0;p<NUM_PATTERN;p++) {
        euklid_setLength(t,16); euklid_setSteps(t,4,p);
        assert(seq_patternSet.length[p][t]==16);
    }
    for(unsigned bad=NUM_TRACKS;bad<=255;bad++) {
        euklid_setLength(bad,255); euklid_setSteps(bad,3,0); euklid_setRotation(bad,1,0);
        euklid_generate(bad,0); euklid_transferPattern(bad,0);
        assert(euklid_getLength(bad)==16 && euklid_getSteps(bad)==1 && euklid_getRotation(bad)==0);
        seq_setTrackLength(bad,5); assert(seq_getTrackLength(bad)==16);
    }
    for(unsigned bad=NUM_PATTERN;bad<=255;bad++) {
        euklid_setSteps(0,3,bad); euklid_setRotation(0,1,bad);
        euklid_generate(0,bad); euklid_transferPattern(0,bad);
        frontParser_shownPattern=bad; seq_setTrackLength(0,3); assert(seq_getTrackLength(0)==16);
        assert(!preset_readPatternData(bad,3)); assert(preset_loadPatternSD(bad)==FR_INVALID_PARAMETER);
    }
    frontParser_shownPattern=0;
    for(unsigned value=0;value<=255;value++) {
        euklid_setLength(0,value);
        assert(euklid_getLength(0)==(value==0?1:value>16?16:value));
        seq_patternSet.length[0][0]=9; seq_setTrackLength(0,value);
        assert(seq_patternSet.length[0][0]==(value==0?16:value<=64?value:9));
    }
    euklid_rotatePattern(0,1); euklid_rotatePattern(64,1);
    puts("PASS bounds: all byte-valued track/pattern/length inputs");
}
static void playback(void) {
    unsigned ticks=0,notes=0;
    for(unsigned length=1;length<=64;length++) {
        seq_setTrackLength(0,length);
        for(unsigned i=0;i<64;i++) {
            seq_setMainStep(0,0,i,1);
            seq_patternSet.seq_subStepPattern[0][0][i].note=60;
            seq_patternSet.seq_subStepPattern[0][0][i].volume=100;
            seq_patternSet.seq_subStepPattern[0][0][i].prob=100;
        }
        for(unsigned flam=0;flam<=5;flam++) for(unsigned shift=0;shift<16;shift++) {
            for(unsigned i=0;i<64;i++) {
                seq_patternSet.seq_stepTiming[0][0][i].flam=flam;
                seq_patternSet.seq_stepTiming[0][0][i].shift=shift;
            }
            for(unsigned side=0;side<2;side++) {
                seq_stepIndex[0]=side?length-1:0;
                for(unsigned tick=0;tick<24;tick++) { seq_processFlam(tick); ticks++; }
            }
        }
        for(unsigned step=0;step<length;step++) for(unsigned phase=0;phase<2;phase++) {
            elapsed=phase?75.f:25.f; seq_stepIndex[0]=step;
            unsigned expected=(step+phase)%length;
            memset(seq_patternSet.seq_subStepPattern[0][0],0,sizeof(seq_patternSet.seq_subStepPattern[0][0]));
            seq_addNote(0,100,60);
            assert(seq_patternSet.seq_subStepPattern[0][0][expected].note==60);
            assert(seq_isMainStepActive(0,expected,0)); notes++;
        }
    }
    seq_setTrackLength(0,0); assert(seq_getTrackLength(0)==16);
    uint8_t last=seq_getTrackLength(0); last-=1; seq_setTrackLength(0,last); assert(seq_getTrackLength(0)==15);
    seq_patternSet.length[0][0]=16; seq_patternSet.length[0][1]=8;
    for(unsigned t=2;t<7;t++) seq_patternSet.length[0][t]=1;
    assert(seq_getLongestTrack()==0);
    seq_patternSet.length[0][6]=64; assert(seq_getLongestTrack()==6);
    assert(triggers>0);
    printf("PASS playback: %u flam/shift ticks; %u recording positions; mixed lengths\n",ticks,notes);
}
static void fill_pattern(unsigned p,unsigned length) {
    for(unsigned t=0;t<7;t++) {
        seq_patternSet.length[p][t]=length;
        for(unsigned i=0;i<64;i++) {
            Step *step=&seq_patternSet.seq_subStepPattern[p][t][i];
            memset(step,(i+t)%128,sizeof(*step));
            seq_patternSet.seq_stepTiming[p][t][i].flam=(i+t)%6;
            seq_patternSet.seq_stepTiming[p][t][i].shift=(i+t)%15;
        }
        for(unsigned b=0;b<4;b++) seq_patternSet.seq_mainSteps[b][p][t]=0xa55a^(b*7+t);
    }
    seq_patternSet.shuffle[p]=23; seq_patternSet.kitNr[p]=5;
    memcpy(savefileNames[p][PATTERN_NAMES],"HOSTTEST",8);
}
static void roundtrips(void) {
    unsigned cases=0;
    for(unsigned tmp=0;tmp<2;tmp++) for(unsigned length=0;length<=64;length++) {
        unsigned p=tmp?63:0; menu_playedPattern=p; frontParser_shownPattern=p;
        fill_pattern(p,length?length:16);
        assert((tmp?preset_saveTmpPat():preset_savePatternSD(p))==FR_OK);
        size_t size=stream_size,offset=tmp?3642:3655;
        assert(size==(tmp?3650:3663));
        if(!tmp) assert(!memcmp(stream,"HOSTTESTPatV03",14));
        assert(stream[offset]==(length?length:16));
        // Construct a legacy zero-bearing file without using an invalid RAM state.
        if(length==0) memset(stream+offset,0,NUM_TRACKS);
        memcpy(saved_stream,stream,size);
        Step saved_steps[NUM_TRACKS][NUM_STEPS]; StepTiming saved_timing[NUM_TRACKS][NUM_STEPS];
        memcpy(saved_steps,seq_patternSet.seq_subStepPattern[p],sizeof(saved_steps));
        memcpy(saved_timing,seq_patternSet.seq_stepTiming[p],sizeof(saved_timing));
        old_lengths(p); stream_pos=0; error_at=SIZE_MAX;
        assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(p))==FR_OK);
        for(unsigned t=0;t<7;t++) assert(seq_patternSet.length[p][t]==(length?length:16));
        assert(!memcmp(saved_steps,seq_patternSet.seq_subStepPattern[p],sizeof(saved_steps)));
        assert(!memcmp(saved_timing,seq_patternSet.seq_stepTiming[p],sizeof(saved_timing)));
        for(unsigned t=0;t<7;t++) for(unsigned b=0;b<4;b++) assert(seq_patternSet.seq_mainSteps[b][p][t]==(0xa55a^(b*7+t)));
        assert(seq_patternSet.shuffle[p]==23 && seq_patternSet.kitNr[p]==5);
        assert((tmp?preset_saveTmpPat():preset_savePatternSD(p))==FR_OK);
        if(length==0) memset(saved_stream+offset,16,NUM_TRACKS);
        assert(stream_size==size && !memcmp(stream,saved_stream,size)); cases++;
        if(length==0) {
            // Exercise the formerly failing consumers directly after each legacy loader.
            seq_activePattern=p;
            for(unsigned i=0;i<64;i++) seq_patternSet.seq_subStepPattern[p][0][i].note=60;
            seq_stepIndex[0]=0; seq_processFlam(0);
            seq_stepIndex[0]=15; elapsed=75.f; seq_addNote(0,100,60);
            assert(seq_patternSet.seq_subStepPattern[p][0][0].note==60);
            seq_patternSet.length[p][1]=8;
            for(unsigned t=2;t<7;t++) seq_patternSet.length[p][t]=1;
            assert(seq_getLongestTrack()==0);
        }
    }
    printf("PASS persistence: %u full named/autosave roundtrips, unchanged layout/payload\n",cases);
}
static void malformed(void) {
    unsigned cases=0;
    for(unsigned tmp=0;tmp<2;tmp++) {
        reset(); fill_pattern(0,16);
        assert((tmp?preset_saveTmpPat():preset_savePatternSD(0))==FR_OK);
        size_t size=stream_size,offset=tmp?3642:3655; memcpy(saved_stream,stream,size);
        for(size_t cut=0;cut<size;cut++) {
            stream_size=cut; old_lengths(0);
            assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(0))!=FR_OK);
            lengths_unchanged(0); cases++;
        }
        stream_size=size;
        for(unsigned t=0;t<7;t++) for(unsigned bad=65;bad<=255;bad++) {
            memcpy(stream,saved_stream,size); stream[offset+t]=bad; old_lengths(0);
            assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(0))!=FR_OK);
            lengths_unchanged(0); cases++;
        }
        memcpy(stream,saved_stream,size);
        // Error injection covers every read start, including successful-looking counts.
        size_t points[]={0,tmp?1:14,offset-57,offset-1,offset,offset+7};
        for(unsigned i=0;i<sizeof(points)/sizeof(points[0]);i++) {
            error_at=points[i]; old_lengths(0);
            assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(0))!=FR_OK);
            lengths_unchanged(0); cases++;
        }
        error_at=SIZE_MAX;
    }
    // V1/V2 synthetic bodies use the source-defined no-timing layout.
    const size_t old_offset=7*64*sizeof(Step)+7*4*2+1;
    for(unsigned version=1;version<=2;version++) for(unsigned tail=0;tail<=8;tail++) {
        memset(stream,0,sizeof(stream)); stream_size=old_offset+tail; stream_pos=0;
        if(tail) memset(stream+old_offset,8,tail);
        old_lengths(0); unsigned ok=preset_readPatternData(0,version);
        unsigned expected=(version==1 && (tail==0 || tail>=7)) || (version==2 && tail==8);
        assert(!!ok==!!expected);
        if(expected) for(unsigned t=0;t<7;t++) assert(seq_patternSet.length[0][t]==(tail?8:16));
        else lengths_unchanged(0);
        cases++;
    }
    menu_playedPattern=menu_shownPattern=seq_activePattern=seq_pendingPattern=3;
    for(unsigned invalid=64;invalid<=255;invalid++) {
        stream[0]=invalid; stream_size=1; assert(preset_LoadTmpPat()!=FR_OK);
        assert(menu_playedPattern==3 && menu_shownPattern==3 && seq_activePattern==3 && seq_pendingPattern==3);
    }
    error_at=0; stream[0]=255; assert(preset_LoadTmpPat()!=FR_OK);
    assert(menu_playedPattern==3 && seq_activePattern==3); error_at=SIZE_MAX;
    printf("PASS malformed: %u truncation/range/I-O/legacy-tail cases; no partial lengths\n",cases+193);
}
static void protocol(void) {
    unsigned calls=0;
    for(unsigned input=0;input<=255;input++) {
        frontParser_sysexActive=SYSEX_RECEIVE_PAT_LEN_DATA; frontParser_sysexSeqStepNr=0;
        for(unsigned slot=0;slot<NUM_PATTERN*NUM_TRACKS;slot++) {
            unsigned p=slot/7,t=slot%7; seq_patternSet.length[p][t]=9;
            frontParser_handleSysexData(input);
            assert(seq_patternSet.length[p][t]==(input==0?16:input<=64?input:9));
            assert(frontParser_sysexSeqStepNr==slot+1); calls++;
        }
        for(unsigned extra=0;extra<4;extra++) {
            frontParser_handleSysexData(1); assert(frontParser_sysexSeqStepNr==448); calls++;
        }
    }
    for(unsigned counter=448;counter<=65535;counter++) {
        frontParser_sysexSeqStepNr=counter; frontParser_handleSysexData(16);
        assert(frontParser_sysexSeqStepNr==counter); calls++;
    }
    // Existing mode-selection mechanism restarts a new legal stream.
    frontParser_sysexActive=SYSEX_ACTIVE_MODE_NONE; frontParser_handleSysexData(SYSEX_RECEIVE_PAT_LEN_DATA);
    assert(frontParser_sysexSeqStepNr==0); frontParser_handleSysexData(0);
    assert(frontParser_sysexSeqStepNr==1 && seq_patternSet.length[0][0]==16);
    printf("PASS protocol: %u byte-value/counter boundary calls; no wrap\n",calls+2);
}
int main(int argc,char **argv) {
    assert(argc==2); reset();
    if(!strcmp(argv[1],"generation")) generation();
    else if(!strcmp(argv[1],"bounds")) bounds();
    else if(!strcmp(argv[1],"playback")) playback();
    else if(!strcmp(argv[1],"roundtrips")) roundtrips();
    else if(!strcmp(argv[1],"malformed")) malformed();
    else if(!strcmp(argv[1],"protocol")) protocol();
    else abort();
    return 0;
}
