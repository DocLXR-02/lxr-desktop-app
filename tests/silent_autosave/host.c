/* Reuse the complete checked-writer fixture/extraction, not a retry approximation. */
#define main existing_save_suite_main
#include "../save_errors/host.c"
#undef main
static void silence(void)
{
 assert(!notifications && !repaints && !resets && menu_activePage==SAVE_PAGE);
 assert(menu_loadedPresetNr[CURRENT_KIT]==63 && sucessTimer==123);
 assert(starts==stops);
}
static void touch(unsigned pattern){preset_touchTimer(pattern?PRESET_PAT_TIMER:PRESET_KIT_TIMER);}
static volatile uint8_t *pending(unsigned pattern){return pattern?&preset_patDirty:&preset_kitDirty;}
static PresetAutosaveRetry *retry(unsigned pattern){return pattern?&preset_patRetry:&preset_kitRetry;}
static void current_bytes(unsigned pattern)
{
 size_t size=used;memcpy(oracle,output,size);
 assert((pattern?baseline_preset_saveTmpPat():baseline_preset_saveTmpKit())==FR_OK);
 assert(used==size && !memcmp(oracle,output,size));
}
static void normal_and_recoverable(void)
{
 session();contents();now=100;touch(0);touch(1);now+=1999;preset_processTimer();assert(!opens);
 now++;preset_processTimer();assert(opens==2 && closes==2 && !preset_kitDirty && !preset_patDirty);silence();
 const FRESULT safe[]={FR_NO_PATH,FR_NO_FILE,FR_INVALID_NAME,FR_EXIST,FR_WRITE_PROTECTED,FR_DENIED};
 unsigned failures_count=0;
 for(unsigned pattern=0;pattern<2;pattern++)for(unsigned e=0;e<sizeof safe/sizeof *safe;e++)for(unsigned wrap=0;wrap<2;wrap++){
  session();contents();now=wrap?UINT32_MAX-3000u:100u;touch(pattern);now+=2000u;open_error=safe[e];
  uint32_t expected=2000;
  for(unsigned n=0;n<9;n++){
   unsigned before=opens;preset_processTimer();assert(opens==before+1 && !writes && !closes);
   assert(*pending(pattern) && !preset_autosavePaused && preset_sharedFailure==FR_OK);
   assert(retry(pattern)->wait==expected);failures_count++;
   now+=expected-1;preset_processTimer();assert(opens==before+1);now++;
   if(expected<32000u)expected*=2u;
  }
  /* All attempts serialize live RAM; no cached payload from the first failure. */
  memset(activeParameters,0x5a,END_OF_SOUND_PARAMETERS);
  seq_patternSet.seq_subStepPattern[0][0][0].note=73;
  open_error=FR_OK;preset_processTimer();
  assert(opens==10 && closes==1 && !*pending(pattern) && !retry(pattern)->wait);
  silence();current_bytes(pattern);
 }
 printf("PASS silent recovery: %u safe failed opens then 24 tenth-attempt successes; independent types; 2/4/8/16/32s cap and tick wrap\n",failures_count);
}
static void edits_and_independence(void)
{
 session();contents();touch(0);now+=2000;open_error=FR_NO_PATH;preset_processTimer();
 uint32_t failed=retry(0)->at;
 for(unsigned i=0;i<4000;i++){now++;activeParameters[5]++;touch(0);preset_processTimer();assert(opens==1);}
 assert(retry(0)->at==failed && retry(0)->wait==2000 && preset_kitDirty);
 open_error=FR_OK;now+=1999;preset_processTimer();assert(opens==1);
 now++;preset_processTimer();assert(opens==2 && !preset_kitDirty);silence();current_bytes(0);
 session();contents();touch(0);touch(1);now+=2000;
 open_error=FR_NO_PATH;open_error_name="/tmp/tmp.pat";preset_processTimer();
 assert(opens==2 && preset_patDirty && !preset_kitDirty && preset_patRetry.wait==2000 && !preset_kitRetry.wait);
 assert(preset_saveGlobals()==FR_OK && preset_patDirty && preset_patRetry.wait==2000); /* manual during cooldown */
 now+=1000;touch(0);open_error=FR_OK;now+=1000;preset_processTimer();
 assert(!preset_patDirty && preset_kitDirty && !preset_patRetry.wait);
 now+=1000;preset_processTimer();assert(!preset_kitDirty);silence();
 puts("PASS edits/cooldown independence: 4000 edit/tick calls cause no storm; current RAM retained; kit/pattern deadlines separate; manual save does not clear pending backups");
}
static void kit_edit(void){activeParameters[5]^=0x7f;touch(0);}
static void pattern_edit(void){seq_patternSet.seq_subStepPattern[0][0][0].note^=0x3f;touch(1);}
static void completion_edit(volatile uint8_t *dirty){if(dirty==&preset_kitDirty)kit_edit();else pattern_edit();}
static void cancel_pattern(void){preset_touchTimer(PRESET_STOP_PAT_TIMER);}
static void interleavings(void)
{
 for(unsigned pattern=0;pattern<2;pattern++)for(unsigned stage=0;stage<2;stage++){
  session();contents();touch(pattern);now+=2000;
  if(stage)exclusive_hook=completion_edit;else write_hook=pattern?pattern_edit:kit_edit;
  preset_processTimer();assert(*pending(pattern) && closes==1 && !preset_autosavePaused);
  unsigned before=opens;preset_processTimer();assert(opens==before);
  now+=1999;preset_processTimer();assert(opens==before);
  now++;preset_processTimer();assert(!*pending(pattern) && closes==2);silence();current_bytes(pattern);
 }
 session();contents();touch(0);now+=2000;lose_exclusive=1;preset_processTimer();
 assert(preset_kitDirty && preset_kitRetry.wait==2000);
 for(unsigned i=0;i<100;i++)preset_processTimer();assert(opens==1);
 now+=2000;preset_processTimer();assert(!preset_kitDirty && closes==2);silence();
 session();contents();touch(1);now+=2000;write_hook=cancel_pattern;preset_processTimer();
 assert(!preset_patDirty && !preset_autosavePaused);now+=32000;preset_processTimer();assert(opens==1);silence();
 puts("PASS production completion logic: same-tick edits during payload and exclusive store survive; reservation-loss cooldown; explicit pattern-start cancellation preserved; no ISR edit hook added");
}
static void unsafe_holds(void)
{
 const FRESULT check[]={FR_NOT_READY,FR_LOCKED,FR_TOO_MANY_OPEN_FILES,FR_NO_FILESYSTEM,FR_NOT_ENABLED,FR_INVALID_DRIVE,FR_INVALID_PARAMETER,FR_DISK_ERR,FR_INT_ERR,FR_TIMEOUT};
 for(unsigned pattern=0;pattern<2;pattern++)for(unsigned e=0;e<sizeof check/sizeof *check;e++){
  session();touch(pattern);now+=2000;open_error=check[e];preset_processTimer();
  assert(*pending(pattern) && preset_autosavePaused && opens==1 && !writes);
  for(unsigned i=0;i<10;i++){now+=32000;preset_processTimer();assert(opens==1);}silence();
 }
 for(unsigned pattern=0;pattern<2;pattern++)for(unsigned failure=0;failure<4;failure++){
  session();touch(0);touch(1);if(!pattern)preset_patDirty=0;now+=2000;
  if(failure==0)short_at=1;
  if(failure==1){fail_at=1;write_error=FR_NO_PATH;} /* Open-safe result is NOT write-safe. */
  if(failure==2)close_error=FR_DISK_ERR;
  if(failure==3)close_error=FR_NO_PATH;
  preset_processTimer();assert(*pending(pattern) && preset_autosavePaused && preset_sharedFailure!=FR_OK);
  if(pattern)assert(preset_kitDirty && opens==1);
  unsigned before=opens,write_before=writes,close_before=closes;
  for(unsigned i=0;i<100;i++){now+=32000;preset_processTimer();}
  assert(opens==before && writes==write_before && closes==close_before);
  assert(preset_saveGlobals()!=FR_OK && preset_autosavePaused);silence();
 }
 session();touch(0);now+=2000;SDFile.obj.fs=&host_fs;preset_processTimer();
 assert(preset_autosavePaused && preset_kitDirty && !opens);silence();
 session();touch(0);now+=2000;SDFatFS.fs_type=0;preset_processTimer();
 assert(preset_autosavePaused && preset_kitDirty && !opens);
 for(unsigned n=0;n<10;n++){now+=32000;preset_processTimer();assert(!opens);}silence();
 /* Completed explicit save can re-arm a check-required hold; no latch is cleared. */
 session();contents();touch(0);now+=2000;open_error=FR_NOT_READY;preset_processTimer();assert(preset_autosavePaused);
 open_error=FR_OK;assert(preset_saveGlobals()==FR_OK && !preset_autosavePaused && preset_kitDirty);
 preset_processTimer();assert(!preset_kitDirty);silence();
 puts("PASS unsafe/check-required holds: NOT_READY and ownership/mount failures do not loop; partial/write/close errors never replay; serious latches remain; completed explicit save re-arms only a healthy check-required hold");
}
int main(void)
{
 existing_save_suite_main();normal_and_recoverable();edits_and_independence();interleavings();unsafe_holds();return 0;
}
