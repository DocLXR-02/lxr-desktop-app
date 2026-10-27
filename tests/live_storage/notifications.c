/* Actual menu notification/refresh bodies; time, LCD sender and normal painters mocked. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "menu/menu.h"
#include "ff.h"
uint8_t menu_activePage,visibleCursor,screensaver_Active,copyClear_showClearMenu;
uint8_t display_refresh_state,display_needs_refresh,display_needs_clear_all;
char editDisplayBuffer[2][17],currentDisplayBuffer[2][16];
volatile uint32_t systick_ticks;
uint32_t sucessTimer;
static volatile uint8_t menu_sdError,menu_sdErrorHeld,menu_sdErrorAuto;
static uint8_t autosave_paused;
static volatile uint32_t menu_sdErrorSince;
static uint32_t now;
static FRESULT held;
static unsigned paints,sends;
static uint8_t expire_on_copy;
#define SELECT_MODE_VOICE 0
uint32_t HAL_GetTick(void){return now;}
FRESULT preset_storageError(void){return held;}
uint8_t preset_autosaveIsPaused(void){return autosave_paused;}
void buttonHandler_setMode(uint8_t mode){(void)mode;}
void led_clearAllBlinkLeds(void){}
void led_setMode2(uint8_t mode){(void)mode;}
void menu_switchPage(uint8_t page){menu_activePage=page;}
uint8_t menu_getActiveVoice(void){return 0;}
uint8_t menu_getSubPage(void){return 0;}
void led_setActiveSelectButton(uint8_t page){(void)page;}
void menu_resetActiveParameter(void){}
static void paint(void){paints++;memset(editDisplayBuffer,'R',sizeof editDisplayBuffer);}
static void menu_repaintLoadSavePage(void){paint();}
static void menu_repaintSimplePage(void){paint();}
void menu_repaintGeneric(void){paint();}
uint8_t sendDisplayBuffer(void){sends++;return display_refresh_state;}
static void *interleaved_copy(void *to,const void *from,size_t count)
{
 void *result=memcpy(to,from,count);
 if(expire_on_copy){expire_on_copy=0;now+=2000;menu_tick();}
 return result;
}
#define memcpy interleaved_copy
#include "functions.inc"
#undef memcpy
static void reset(void)
{
 menu_activePage=SAVE_PAGE;visibleCursor=2;screensaver_Active=copyClear_showClearMenu=0;
 display_refresh_state=display_needs_clear_all=display_needs_refresh=0;
 memset(editDisplayBuffer,0,sizeof editDisplayBuffer);menu_sdError=FR_OK;held=FR_OK;
 now=100;autosave_paused=0;expire_on_copy=0;paints=sends=0;systick_ticks=0;
}
int main(void)
{
 reset();menu_showSdError(FR_OK);assert(!display_needs_clear_all);
 menu_showSdError(FR_NO_PATH);assert(display_needs_clear_all && menu_activePage==SAVE_PAGE && !sends);
 menu_tick();assert(sends==1 && !memcmp(editDisplayBuffer[1],"Path? Retry save",16));
 /* Navigation/edit events are not consumed; overlay expires onto the current page. */
 menu_activePage=LOAD_PAGE;menu_repaint();assert(menu_activePage==LOAD_PAGE && !paints);
 now+=1999;menu_tick();assert(menu_sdError==FR_NO_PATH && !paints);
 now++;menu_tick();assert(menu_sdError==FR_OK && paints==1 && menu_activePage==LOAD_PAGE);
 reset();now=UINT32_MAX-500u;menu_showSdError(FR_INVALID_NAME);now+=1999u;menu_tick();assert(menu_sdError);
 now++;menu_tick();assert(!menu_sdError && paints==1);
 /* Notification queues while refresh is busy; no wait for mocked LCD readiness. */
 reset();display_refresh_state=3;menu_showSdError(FR_EXIST);menu_tick();assert(display_needs_clear_all && !paints);
 held=FR_DISK_ERR;menu_showSdError(FR_DISK_ERR);display_refresh_state=0;menu_tick();
 assert(!memcmp(editDisplayBuffer[0],"SD writes held  ",16) && !memcmp(editDisplayBuffer[1],"RAM OK; check SD",16));
 now+=2000;menu_tick();assert(!menu_sdError && held==FR_DISK_ERR); /* dismiss never unlocks storage */
 reset();autosave_paused=1;menu_showSdError(FR_NO_PATH);menu_tick();assert(!memcmp(editDisplayBuffer[0],"Autosave paused ",16));
 /* Simulated display IRQ expires midway through actual foreground overlay copy. */
 reset();menu_showSdError(FR_NO_PATH);menu_tick();expire_on_copy=1;menu_repaint();
 assert(!menu_sdError && display_needs_clear_all);menu_tick();
 for(unsigned row=0;row<2;row++)for(unsigned col=0;col<16;col++)assert(editDisplayBuffer[row][col]=='R');
 for(unsigned result=1;result<=FR_INVALID_PARAMETER;result++){
  reset();menu_showSdError((uint8_t)result);menu_tick();
  assert(editDisplayBuffer[0][16]==' ' && editDisplayBuffer[1][16]==' ' && !paints);
  now+=2000;menu_tick();assert(menu_sdError==FR_OK && paints==1);
 }
 puts("PASS actual notification/tick/repaint bodies: queued/busy display, 1999/2000ms and wrap, latest error, all result codes, expiry/copy interleaving, page/RAM independence, no storage unlock or blocking calls");
}
