// SPDX-License-Identifier: GPL-3.0-only
// Real production menu bodies; HAL/UI side effects and parser DSP are mocked.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Preset/PresetManager.h"
#include "front.h"
#include "Soundengine/DSPAudio/modulationNode.h"
#include "menu/menu.h"
#include "menu/menuPages.h"
#include "menu/MenuText.h"
#include "menu/CcNr2Text.h"
#include "buttonHandler.h"
#include "ledHandler.h"
#include "Soundengine/globals.h"
#include "button_constants.inc"
#include "frontPanelParser_ui.h"
#include "Soundengine/MIDI/MidiParser.h"
#include "Soundengine/Sequencer/sequencer.h"
uint8_t activeParameters[NUM_PARAMS],mainParameters[64][END_OF_SOUND_PARAMETERS];
ModulationNode velocityModulators[6];
void modNode_setDestination(ModulationNode *p,uint16_t v){(void)p;(void)v;assert(0);}
uint8_t *morphParameters=mainParameters[1];
uint8_t menu_activePage,menu_activeVoice;
static uint8_t morph_state,morph_index,menuIndex,editModeActive,menu_TargetVoiceGapIndex;
char editDisplayBuffer[2][17];
static int8_t buttonHandler_armedAutomationStep=NO_STEP_SELECTED;
static unsigned sent,changed,touches,selected_auto,disarms;static uint8_t rendered[END_OF_SOUND_PARAMETERS];
uint8_t saveScreenPots[4];
volatile uint16_t buttonHandler_originalParameter[2];
uint8_t buttonHandler_originalValue[2],buttonHandler_resetLock1,buttonHandler_resetLock2;
static uint8_t checkScrollSign(uint8_t p,uint8_t n){(void)p;(void)n;return 0;}
static void menu_displayModTargetFull(uint8_t n){(void)n;assert(0);}
static void menu_displayModTargetShort(uint8_t n,char *s,char v){(void)n;(void)s;(void)v;assert(0);}
static void getMenuItemNameForValue(uint8_t m,uint8_t n,char *s){(void)m;(void)n;memcpy(s,"---",3);}
static uint8_t getMaxEntriesForMenu(uint8_t m){(void)m;return 16;}
void setNoteName(uint8_t n,char *s){(void)n;memcpy(s,"C 4",3);}
void preset_touchTimer(PresetTimer p){assert(p==PRESET_KIT_TIMER);touches++;}
void screensaver_touch(void){}
void menu_repaintAll(void){}
void menu_handleSaveScreenKnobValue(uint8_t p,uint8_t v){(void)p;(void)v;assert(0);}
static void menu_processSpecialCaseValues(uint16_t p){assert(p==PAR_MORPH);}
int8_t buttonHandler_getArmedAutomationStep(void){return buttonHandler_armedAutomationStep;}
void seq_setActiveAutomationTrack(uint8_t n){assert(n<2);selected_auto++;}
void seq_clearAutomation(uint8_t a,uint8_t b,uint8_t c){(void)a;(void)b;(void)c;assert(0);}
uint8_t seq_activePattern;
void soundengine_process(void){assert(0);}
void frontPanel_sendData(uint8_t s,uint8_t p,uint8_t v){assert(s==ARM_AUTOMATION_STEP && p==0 && v==DISARM_AUTOMATION);disarms++;}
uint8_t led_idx2StepLed(uint8_t i){assert(i<16);return i;}
void led_setBlinkLed(const uint8_t n,const uint8_t v){assert(n<16 && v==0);}
void midiParser_ccHandler(MidiMsg m,uint8_t u,uint8_t f,float v)
{
 (void)v;assert(u==1 && f==0);
 unsigned p=m.status==MIDI_CC?m.data1-1:m.data1+128;
 assert(p<END_OF_SOUND_PARAMETERS && (m.status!=MIDI_CC || m.data1!=0));
 assert(p!=PAR_NRPN_DATA_ENTRY_COARSE && p!=PAR_NRPN_FINE && p!=PAR_NRPN_COARSE && p!=PAR_BANK_CHANGE);rendered[p]=m.data2;sent++;
}
#include "ui.inc"
static void drain(void){unsigned n=0;while(morph_state){preset_morph_tick();assert(++n<=END_OF_SOUND_PARAMETERS);}}
static void maths(void)
{
 unsigned count=0;
 for(unsigned a=0;a<256;a++)for(unsigned b=0;b<256;b++){
  assert(interpolate(a,b,0)==a && interpolate(a,b,127)==b);
  int prev=a;
  for(unsigned x=0;x<128;x++){
   int actual=interpolate(a,b,x);double ideal=a+((int)b-(int)a)*(x/127.0);
   assert(abs(actual-(int)(ideal+0.5))<=1);
   assert(actual>=(a<b?a:b) && actual<=(a>b?a:b));
   assert(a<=b?actual>=prev:actual<=prev);prev=actual;count++;
  }
  for(unsigned x=128;x<256;x++)assert(interpolate(a,b,x)==b);
 }
 assert(interpolate(0,127,63)==63 && interpolate(0,127,64)==64);
 printf("PASS %u exhaustive 8-bit endpoint-pair/interior combinations, monotonicity and <=1 LSB ideal error; exact endpoints, midpoint pair and all invalid amounts\n",count);
}
static void ui(void)
{
 assert(parameter_dtypes[PAR_MORPH]==DTYPE_0B127);
 assert(END_OF_SOUND_PARAMETERS<=PAR_MORPH && PAR_MORPH<PAR_BEGINNING_OF_GLOBALS);
 for(unsigned page=0;page<NUM_PAGES;page++)for(unsigned sub=0;sub<NUM_SUB_PAGES;sub++)for(unsigned pos=0;pos<8;pos++){
  if((&menuPages[page][sub].bot1)[pos]!=PAR_MORPH)continue;
  changed++;menu_activePage=page;menuIndex=(sub<<PAGE_SHIFT)|pos;
  for(unsigned x=0;x<128;x++){
   activeParameters[PAR_MORPH]=x;editModeActive=1;menu_repaintGeneric();
   char expected[4];snprintf(expected,sizeof expected,"%3u",x);
   assert(!memcmp(editDisplayBuffer[1]+13,expected,3));
   editModeActive=0;menu_repaintGeneric();assert(!memcmp(editDisplayBuffer[1]+4*(pos%4),expected,3));
  }
  activeParameters[PAR_MORPH]=0;
  for(unsigned n=0;n<1000;n++){menu_encoderChangeParameter(-1);assert(!activeParameters[PAR_MORPH]);}
  for(unsigned n=0;n<127;n++){menu_encoderChangeParameter(1);assert(activeParameters[PAR_MORPH]==n+1);}
  for(unsigned n=0;n<1000;n++){menu_encoderChangeParameter(1);assert(activeParameters[PAR_MORPH]==127);}
  activeParameters[PAR_MORPH]=0;
  for(unsigned n=0;n<1000;n++){assert(menu_parseKnobValue(pos%4,-1));assert(!activeParameters[PAR_MORPH]);}
  for(unsigned n=0;n<127;n++){assert(menu_parseKnobValue(pos%4,1));assert(activeParameters[PAR_MORPH]==n+1);}
  for(unsigned n=0;n<1000;n++){assert(menu_parseKnobValue(pos%4,1));assert(activeParameters[PAR_MORPH]==127);}
  drain();
  // A held-step edit saves/restores the runtime-only control, not a recorded
  // Morph automation destination. Exercise the real button-disarm body.
  for(unsigned original=0;original<128;original++){
   buttonHandler_originalParameter[0]=buttonHandler_originalParameter[1]=0;
   buttonHandler_resetLock1=buttonHandler_resetLock2=0;
   preset_morph(original);buttonHandler_armedAutomationStep=0;
   assert(menu_parseKnobValue(pos%4,1));
   assert(buttonHandler_originalParameter[0]==PAR_MORPH && buttonHandler_originalValue[0]==original);
   assert(activeParameters[PAR_MORPH]==(original<127?original+1:127));
   buttonHandler_disarmTimerActionStep();
   assert(activeParameters[PAR_MORPH]==original && !buttonHandler_resetLock1 && buttonHandler_armedAutomationStep==NO_STEP_SELECTED);
   drain();
  }
 }
 assert(changed==2);
 for(unsigned value=128;value<256;value++){preset_morph(value);assert(activeParameters[PAR_MORPH]==127);drain();}
 puts("PASS both actual Morph pages: encoder/knob 0..127, 8000 endpoint movements, normal/edit display, saturated direct legacy/invalid requests and bounded sweeps");
}
int main(void)
{
 for(unsigned i=0;i<END_OF_SOUND_PARAMETERS;i++){activeParameters[i]=0;morphParameters[i]=127;}
 maths();ui();
 for(unsigned m=0;m<128;m++){
  sent=0;preset_morph(m);drain();assert(sent==END_OF_SOUND_PARAMETERS-4);
  for(unsigned p=0;p<END_OF_SOUND_PARAMETERS;p++){
   if(preset_isTransportParameter(p))continue;
   unsigned expected=(p>=PAR_AUDIO_OUT1 && p<=PAR_AUDIO_OUT6)?activeParameters[p]:m;
   assert(rendered[p]==expected);
  }
 }
 puts("PASS 128 actual non-identical-kit sweeps: full transition, source routing retained and every reserved slot excluded");
 for(unsigned i=0;i<getNumModTargets();i++)assert(modTargets[i].param!=PAR_MORPH);
 puts("PASS actual automation table excludes Morph; 256 held-step edit/disarm restorations preserve the control without recording a Morph target; no MIDI/global mapping added");
 printf("Parameter boundaries: END_OF_SOUND_PARAMETERS=%u PAR_MORPH=%u PAR_BEGINNING_OF_GLOBALS=%u NUM_PARAMS=%u\n",END_OF_SOUND_PARAMETERS,PAR_MORPH,PAR_BEGINNING_OF_GLOBALS,NUM_PARAMS);
}
