/* Test real diagnostic module and real menu overlay with a simulated HAL/UI.
 * Not an interrupt scheduling, ADC electrical, or complete UI integration test. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "stm32f7xx_hal.h"
#include "safety/runtimeDiagnostics.h"
HostDebug host_debug;
HostDwt host_dwt;
uint32_t host_tick, host_mask, host_ipsr, host_locks, host_unlocks;
enum { MENU_MIDI_PAGE=7, MASK_PAGE=248, MASK_PARAMETER=7, PAGE_SHIFT=3, MODE_NONE=0, FR_OK=0 };
static uint8_t menu_voiceCopyActive, menu_voiceCopyDrain, menu_voiceCopyNotice;
static uint8_t menu_voiceCopyModeHeld, menu_voiceCopyCopyHeld, menu_voiceCopyVoiceHeld;
static uint8_t copyClear_Mode, copyClear_showClearMenu, menu_autoNotice, menu_kitLoadNotice;
static uint8_t menu_sdError, screensaver_Active, menu_activePage=MENU_MIDI_PAGE, menuIndex=8;
static uint8_t editModeActive, lastEncoderButton, visibleCursor, display_needs_refresh, shift, copy;
static char editDisplayBuffer[2][17];
static unsigned repaints;
static void menu_repaintAll(void) { ++repaints; }
static void screensaver_touch(void) { screensaver_Active=0; }
static uint8_t buttonHandler_getShift(void) { return shift; }
static uint8_t buttonHandler_getCopy(void) { return copy; }
#include "diagnosticsMenu.inc"
static void click(void) {
    assert(menu_diagEncoder(0,1)); assert(menu_diagEncoder(0,0));
}
static void record(unsigned metric, uint32_t start, uint32_t end) {
    host_dwt.CYCCNT=end; lxr_diag_record(metric,start);
}
static void row(const char *a,const char *b) {
    assert(menu_diagRender());
    assert(strlen(editDisplayBuffer[0])==16 && strlen(editDisplayBuffer[1])==16);
    assert(!strncmp(editDisplayBuffer[0],a,strlen(a)));
    assert(!strncmp(editDisplayBuffer[1],b,strlen(b)));
}
int main(void) {
    assert(sizeof(LxrDiagnostics)==72);
    host_dwt.CYCCNT=123;
    lxr_diag_init();
    assert(host_dwt.CYCCNT==123 && lxr_diagnostics.cycle_counter_enabled);
    assert(lxr_diag_session_state()==LXR_DIAG_READY && !lxr_diag_session_result());
    record(0,0,300000);
    host_ipsr=16; assert(!lxr_diag_session_start()); host_ipsr=0;
    host_mask=1; host_tick=UINT32_MAX-10u;
    assert(lxr_diag_session_start() && host_mask==1);
    assert(lxr_diagnostics.render.max_cycles==0 && lxr_diagnostics.version==1);
    assert(!lxr_diag_session_start()); host_mask=0;
    record(0,0,100000); record(0,0,150000);
    record(1,UINT32_MAX-49u,50u);
    assert(lxr_diagnostics.timer_isr.max_cycles==100);
    host_dwt.CYCCNT=2000000; lxr_diag_callback(0,0);
    assert(lxr_diagnostics.dma[0].long_intervals==0);
    host_dwt.CYCCNT+=626363; lxr_diag_callback(0,1);
    lxr_diag_callback(1,1); lxr_diag_rearm(1,1);
    lxr_diag_callback(2,0); lxr_diag_rearm(2,1);
    host_tick=19; host_ipsr=16; assert(!lxr_diag_session_stop()); host_ipsr=0;
    assert(lxr_diag_session_stop() && host_mask==0);
    const LxrDiagResult *f=lxr_diag_session_result();
    assert(f && f->number==1 && f->elapsed_ms==30);
    assert(f->data.render.max_cycles==150000 && f->data.render.count==2);
    assert(f->data.timer_isr.max_cycles==100);
    assert(f->data.dma[0].callbacks==2 && f->data.dma[0].not_ready==1 &&
           f->data.dma[0].long_intervals==1 && f->data.dma[1].callbacks==1 &&
           f->data.dma[1].rearm_errors==1);
    record(0,0,250000); assert(f->data.render.max_cycles==150000);
    assert(!lxr_diag_session_stop());
    puts("PASS session reset/freeze, wrap, ISR rejection, PRIMASK, DMA identity");
    menuIndex=0; assert(!menu_diagConfig()); menuIndex=8;
    menu_sdError=1; assert(!menu_diagConfig()); menu_sdError=0;
    menu_voiceCopyActive=1; assert(!menu_diagConfig()); menu_voiceCopyActive=0;
    assert(menu_diagConfig() && menuIndex==8 && menu_diagPotMode()==1);
    assert(menu_diagEncoder(0,0)); row("DIAG 01 FROZEN","DATA: NEW?");
    click(); row("DIAG 01 FROZEN","DATA: CONFIRM");
    assert(menu_diagEncoder(1,0)); row("SESSION NUMBER","1");
    assert(menu_diagEncoder(-1,0)); row("DIAG 01 FROZEN","DATA: NEW?");
    click(); click(); row("DIAG 01 RUNNING","DATA: STOP");
    assert(lxr_diag_session_state()==LXR_DIAG_RUNNING);
    assert(!menu_diagConfig()); assert(menu_diagPotMode()==2 && menu_diagPotMode()==0);
    assert(menu_diagEncoder(0,0)); assert(!menu_diagEncoder(0,0));
    assert(lxr_diag_session_state()==LXR_DIAG_RUNNING);
    assert(menu_diagConfig()); assert(menu_diagEncoder(0,0));
    lxr_diagnostics.render.count=UINT32_MAX;
    record(0,0,313182); assert(lxr_diagnostics.render.count==UINT32_MAX);
    host_tick+=100; click(); row("DIAG 01 FROZEN","DATA: NEW?");
    for(unsigned i=1;i<=18;i++){assert(menu_diagEncoder(1,0)); assert(menu_diagPage==i); assert(menu_diagRender());}
    assert(menu_diagEncoder(1,0) && menu_diagPage==0);
    for(unsigned i=0;i<6;i++)assert(menu_diagEncoder(1,0));
    row("DSP BLOCKS","4294967295 SAT");
    char line[17]; menu_diagNumber(line,UINT32_MAX,1,0,1);
    assert(!strcmp(line,"4294967295 LIMIT"));
    menu_sdError=1; menu_diagService(); assert(!menu_diagVisible && menu_diagPotMode()==2);
    menu_sdError=0;
    puts("PASS overlay entry/conflict, confirmation, 19 pages, full uint32, SAT, rebase request");
    lxr_diagnostics.cycle_counter_enabled=0;
    assert(lxr_diag_session_start()); record(0,0,999999);
    lxr_diag_callback(1,0); host_tick+=86400000u;
    assert(lxr_diag_session_poll()); f=lxr_diag_session_result();
    assert(f && f->duration_limit && !f->data.render.count && f->data.dma[1].not_ready==1);
    assert(menu_diagConfig()); assert(menu_diagEncoder(0,0));
    for(unsigned i=0;i<5;i++)assert(menu_diagEncoder(1,0));
    row("DSP PEAK cyc","N/A");
    assert(host_locks==host_unlocks && host_mask==0);
    puts("PASS timeout, unavailable DWT, N/A, balanced IRQ mask");
    return 0;
}
