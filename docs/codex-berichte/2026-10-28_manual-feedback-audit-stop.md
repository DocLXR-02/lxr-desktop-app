# Final Load/Save Feedback — Audit, Candidate Implementation and Validation Stop

Date: 2026-10-28 UTC. Repository: DocLXR-02/lxr-desktop-app.

## Status and hard stop

**PARTIALLY IMPLEMENTED — NOT VALIDATED; implementation publication withheld.** The audit is complete and a narrowly scoped candidate exists locally. The new required host regression failed. The user explicitly required: “If any required safety test fails, stop and report.” Implementation and further validation stopped at that point. No test assertion was removed or changed to claim success after the failure.

The failure is in a simulated expiry-interleaving test. Its injection hook intercepts memcpy, whereas the new renderer uses memset/strcpy, so the simulated interrupt was never injected. This identifies an invalid/unexercised concurrency scenario in the new fixture, not reproduced memory corruption or a demonstrated target crash. It nevertheless leaves the required concurrency/safety coverage incomplete. No final acceptance or hardware readiness is claimed.

No firmware was flashed. No physical SD card was accessed. No firmware binary was published. No merge or force push occurred. Main, lab-setup and all earlier implementation/delivery branches remain preserved.

## Exact baseline and working state

Approved baseline remote branch controls/morph-range-127 was checked at **83c5ec164d6c3dab439deb2f32a8edf1138b5b77**, with sole parent 5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527. The user states this exact baseline has now passed physical LXR-02 tests; those are user observations, not measurements by this assistant.

AGENTS.md was read completely from lab-setup. Existing checkout /workspace/lxr-desktop-app was initially clean. No worktree was created. New local candidate branch: **ui/manual-load-save-feedback**. Its HEAD remains the approved baseline commit; candidate source/test edits are **uncommitted and not pushed**. There is no final implementation commit. The complete candidate patch is preserved in this report so another assistant can review it independently of the workspace.

Report-only branch: codex-reports/2026-10-28-manual-feedback-audit-stop, based on lab-setup commit 5829398cea5d337200393757e39a37d4d8148256. Report path: docs/codex-berichte/2026-10-28_manual-feedback-audit-stop.md. Only this Markdown report is published.

## Operation audit matrix

Line references below refer to approved baseline 83c5ec1, unless candidate is specified. All command dispatches are in Core/Src/menu/menu.c, menu_handleLoadSaveMenu().

| Command | Actual enabled baseline operation | Completion/failure evidence | Existing feedback | Candidate policy |
|---|---|---|---|---|
| Load Kit | :2192 calls preset_loadDrumsetRam(slot,0,0); updates selected kit, target-gap index, associated kit and autosave timer | void RAM copy; presetManager.c:808 guards slot<64; valid selection is synchronous RAM action, not SD load | No explicit success | Guard slot before side effects; Load Kit / success! after action, error! on invalid slot |
| Save Kit | :2056 RAM save followed by preset_saveDrumsetSD(slot,0) | Checked SD serializer/open/write/count/close FRESULT; failed SD means RAM may already have changed | KIT_SAVE_PAGE_OK success; generic SD error on failure | Same operation order and success-only state; Save Kit / success! or error! |
| Load Morph Kit | :2216 calls preset_loadDrumsetRam(slot,1,0), sets destination identity | void RAM destination selection; valid slot guard establishes executed operation | No explicit success | Load Morph Kit / success! after valid action, error! on invalid slot |
| Save Morph Kit | :2084 preset_saveDrumsetRam(slot,1) | RAM-only baked sound save, no SD call; invalid slot silently ignored by void helper | No explicit success | Validate slot, then Save Morph Kit / success!; no new SD persistence |
| Load Settings | :2288 preset_loadGlobals() | presetManager.c:930 returns boolean; underlying preset_loadGlobalFile() checks read size/close before publication, then globals dispatched | Return value ignored | Load Settings / success! only on true result; otherwise error! |
| Save Settings | :2088 preset_saveGlobals() | FRESULT checks writes and close, S1–S4 protections remain | Generic error, no unified success | Save Settings / success! only on FR_OK, otherwise error! |
| Load Song | :2227 assigns seq_activeSong=seq_songData[slot] | RAM pointer selection; array has NUM_KIT_SLOTS rows (sequencer.h:158) | No explicit success | Validate slot, then Load Song / success!; no claim of SD read |
| Save Song | :2030 “nothing to do here” | **No persistence operation is executed** | No meaningful completion result | Save Song / error!, return; remains unsupported. No Song persistence implemented |
| Load Pattern | :2233 confirmation branch loops preset_loadPatternRAM(i) | presetManager.c:1990 explicitly unused helper returning FR_OK without loading anything; normal default branch also does nothing | Can reset menu state despite no load | Load Pattern / error!, no fake success. Existing separate pattern selection remains unchanged |
| Save Pattern | :2033 preset_savePatternRAM(slot), then preset_savePatternSD(slot) | RAM FRESULT then checked SD FRESULT; stop on first error | Error reported; normal-menu success inconsistent. Separate physical pattern-save branch shows success page | Save Pattern / success! only after required operations; error! on failure |

Additional paths: menu.c:2123 confirmation branch commits every pattern to RAM, not SD; candidate checks each existing RAM result before success. Core/Src/buttonHandler.c around :1140 performs manual pattern RAM+SD saving and was included for consistent result feedback. Selecting a pattern for playback on the alternative button branch is a separate RAM/possibly deferred sequencer selection, not the no-op Load Pattern command. No new success is claimed for deferred pattern activation.

Existing checked serializers and shared FIL ownership/fault latches reside in Core/Src/Preset/presetManager.c. They were not modified. Save Song's no-op still has pre-existing name-editor side effects before dispatch; candidate does not invent persistence to justify feedback.

## Candidate design, not final validated behavior

- Reuses existing two-line display buffer, queued repaint and bounded display refresh.
- Adds public foreground helper menu_showOperationResult(category,saving,failed), labels for five operation types and a single 32-bit encoded result plus 32-bit millisecond timestamp.
- Shows exactly the operation label and success! or error!; no RAM/SD explanatory text, permanent status indicator or new button mapping.
- HAL_GetTick unsigned subtraction expires the overlay at 1000 ms, including wraparound. No delay, busy wait, allocation, storage work or retry is added by notification functions.
- Existing KIT/PATTERN success pages return normally using the same millisecond deadline instead of an inferred tick frequency. TIM14 uses Prescaler=13500 and Period=1; guessing that systick_ticks was milliseconds would be wrong.
- menu_tick() is called by the existing TIM14 display path at priority 0. The candidate adds bounded display-state checks there, not audio/DMA instrumentation. Actual ISR execution-time impact is **unmeasured** and must not be described as zero. No interrupt source or priority is changed.
- Repaint overlays do not consume input events or change the current page. Normal input/navigation can continue while feedback is visible; expiry repaints the then-current page. LCD transmission cadence means logical expiry is not proof that physical pixels change at precisely 1000 ms.
- Project dispatch bodies and the existing generic SD error renderer retain their old semantics and two-second duration. A project error cancels manual feedback. Interaction between an already active manual overlay and direct project progress drawing still needs explicit acceptance coverage before claiming project behavior fully preserved.
- Autosave functions and retry/dirty-state logic remain unchanged. No autosave calls to the manual helper are added.
- RAM-only actions stay RAM-only; mixed actions do not claim success merely because the RAM half completed. No file formats, Morph math, DSP, transaction protocol or MIDI mapping changes are proposed.

Concurrency remains a review item: result is a volatile atomic-width value, timestamp is published first, renderer snapshots the result and schedules repaint if it changes. Expiry may run in TIM14 while foreground queues/renders a message. The failed fixture did not exercise the intended new-renderer copy interleaving, so coherence is not established by that test.

## Candidate file inventory

Production:
- Core/Inc/menu/menu.h: new manual-feedback declaration. Candidate placement is at file end; placement inside the existing header guard should be reviewed before completion.
- Core/Src/menu/menu.c: result overlay, bounded expiry, labels, checked dispatch results, unsupported command handling, RAM slot guards and success-page timing.
- Core/Src/buttonHandler.c: existing manual pattern-save results routed to the same helper.

Tests:
- tests/live_storage/run.py: include actual new overlay functions/state in existing notification fixture.
- tests/morph_range/run.py: replace whole-menu equality with exact equality of Morph input/format/global-handler functions and the required datatype. Morph implementation/serialization tests retained.
- tests/save_errors/host.c: notification mock accepts new manual helper and preserves original error values for existing assertions.
- tests/silent_autosave/run.py and new manual_feedback_boundary.json: separate candidate UI source digests from original audio/storage source digests. These new UI pins are candidate bookkeeping, not independent proof of correctness; final review must check them against the complete intended diff.
- tests/manual_feedback/run.py: new sanitized production-body fixture and project-command preservation checks.

No other production source changed. Full patch below includes new/untracked test files.

## Commands, actual results and errors

Prepared toolchain PATH was used:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
```

| Command | Actual outcome |
|---|---|
| git status --short --branch | Initially clean approved baseline; now local candidate edits preserved |
| git show origin/lab-setup:AGENTS.md | Complete instructions read |
| GitHub remote refs query | Approved branch head exactly 83c5ec1 |
| git switch -c ui/manual-load-save-feedback | Succeeded; no prior branch overwritten |
| cmake --build --preset release --parallel 4 | Preliminary candidate compiled/linked successfully before subsequent candidate edits. **Not a final-source build** |
| python3 tests/live_storage/run.py | PASS: existing generic SD notification and whole-FatFs logical retry tests |
| python3 tests/save_errors/run.py (first attempt) | Fixture compile failure: extracted menu cases referenced menu_saveOptions.category, absent from host fixture. Production calls changed to explicit category constants; no storage assertion removed |
| python3 tests/save_errors/run.py (second attempt) | PASS: all 1536 old/new file comparisons; 2544 failure positions; manual save result tests; 936 recoverable opens then 104 successes |
| python3 tests/manual_feedback/run.py | **FAIL, child SIGABRT**, generated host.c:100 assertion !menu_manualResult; runner exit 1. Stop gate triggered |

The shell command displaying the failing test log itself returned 0 after cat; this does **not** mean the test passed. The test runner traceback and child failure are the authoritative outcome.

Exact failing line:

```c
menu_showOperationResult(SAVE_TYPE_KIT,0,0);
expire_on_copy=1;
menu_repaint();
menu_tick();
assert(!menu_manualResult);
```

The reused fixture advances time inside its intercepted memcpy. The new renderer performs memset/strcpy instead. No time advance happened, so the assertion fails while a valid current result remains active. This diagnosis is based on source inspection after the stop, not a subsequent repaired test run.

Earlier assertions in that same execution completed: all 20 label/result combinations, 999/1000 ms wraparound, retaining current navigation page, repeated failure/success replacement, generic project-error priority, and old success-page deadline. The final summary line was not reached; the suite as a whole remains failed. Unsupported-command routing/project-case equality are static source checks, not execution of every complete menu handler.

**Not run after the stop:** all-suite final validation (BUG001, Euclidean, safety, USB allocator, SD storage, HAL, FatFs, Morph, silent autosave, project storage/transactions), final Release build, Debug build, final image checker, final memory and stack measurements. Prior task passes are not substituted for these missing results.

## Resources and image limitations

No final candidate image identity is approved. The preliminary Release build cannot be delivered as the final source image because later edits occurred. Do not package or flash it.

Approved baseline Release measurements (from preceding verified build, not new measurements here): ITCM 15016/16384 bytes, DTCM 107392/131072, general SRAM 274640/393216; full flash load span 288820 bytes, linker FLASH summary 236808. Baseline image LXR_V2_V1.7.img is 288836 bytes, SHA-256 adb30f63c6c35080611f9d749bd7ea329e9f8c67f0bf61f024d77294fdec1642.

Candidate explicitly declares eight bytes of new fixed result/time state; actual alignment/linked RAM, Flash and static stack impact were **not finally measured**. No dynamic allocation or whole-file buffer was added. Added timing checks/formatting are bounded but run in the existing display architecture, which can preempt audio. No hardware CPU/stack/audio measurement was performed. Bootloader, linker, startup, clocks, DMA configuration and image-generation source remain unchanged; this is source evidence, not completed final-image verification.

## Resume plan and acceptance boundaries

1. Obtain review of this stop report. Retain baseline 83c5ec1 and existing deliveries; do not use the unvalidated candidate binary.
2. Correct the fixture to inject expiry at an actual new-renderer memory operation or explicit deterministic seam, without weakening expiry/coherence assertions. Test foreground publication during TIM14 expiry as well as expiry during rendering, repeated events and wraparound.
3. Review/complete project-progress precedence, all live input paths and the no-op command policy. Do not implement Song persistence or rewrite project transactions. Decide whether unsupported commands should remain error! or be disabled in a separately reviewed UI change.
4. Exercise actual load command branches and all-pattern RAM saves with mocks, rather than treating label-helper tests as full operation completion coverage. Keep separate button pattern selection semantics intact.
5. Re-pin UI hashes only after a final complete diff review; keep original storage/IRQ/DSP safety boundaries. Move public declaration inside normal header structure if appropriate.
6. Run all existing applicable suites plus focused manual tests, final Release/Debug builds, image format/LMA/vector checks, static RAM/Flash and per-function stack comparisons. Report failures rather than suppressing them.
7. Only after successful software gates commit/push an implementation branch for review and issue a revised complete report. Hardware acceptance would then require separate authorization: all supported operation labels/results, true save/read failures, no autosave messages, navigation/editing during overlays, one-second lifetime, project progress and normal audio/MIDI continuity. No destructive SD experiments are needed for the feedback test.

This task is not complete: the report and candidate patch are independently reviewable, but required validation and an implementation commit are missing. The original approved implementation remains unchanged.

## Complete unvalidated candidate patch

```diff
diff --git a/Core/Inc/menu/menu.h b/Core/Inc/menu/menu.h
index e686bd7..55513ad 100644
--- a/Core/Inc/menu/menu.h
+++ b/Core/Inc/menu/menu.h
@@ -713,3 +713,6 @@ void numtostru(char *buf, uint8_t num);
 //void numtostrs(char *buf, int8_t num);
 
 #endif
+
+/* Foreground manual feedback only; never called by autosave. */
+void menu_showOperationResult(uint8_t category, uint8_t saving, uint8_t failed);
diff --git a/Core/Src/buttonHandler.c b/Core/Src/buttonHandler.c
index a3affbd..f64f1fc 100644
--- a/Core/Src/buttonHandler.c
+++ b/Core/Src/buttonHandler.c
@@ -1151,7 +1151,7 @@ static void buttonHandler_seqButtonPressed(uint8_t seqButtonPressed)
 
 
 				FRESULT saveResult = preset_savePatternRAM(seqButtonPressed + 16 * menu_activePatternBank);
-				if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
+				if(saveResult != FR_OK) { menu_showOperationResult(SAVE_TYPE_PATTERN,1,saveResult); return; }
 
 				//save to tmp folder
 				#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
@@ -1167,9 +1167,10 @@ static void buttonHandler_seqButtonPressed(uint8_t seqButtonPressed)
 				#endif
 
 
-				if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
+				if(saveResult != FR_OK) { menu_showOperationResult(SAVE_TYPE_PATTERN,1,saveResult); return; }
 				menu_selectedPresetNr[SAVE_TYPE_PATTERN] = seqButtonPressed + 16 * menu_activePatternBank;
-				menu_activePage = PATTERN_SAVE_PAGE_OK;
+				menu_showOperationResult(SAVE_TYPE_PATTERN,1,0);
+                menu_activePage = PATTERN_SAVE_PAGE_OK;
 				sucessTimer = systick_ticks;
 				menu_repaint();
 
diff --git a/Core/Src/menu/menu.c b/Core/Src/menu/menu.c
index 0715d15..66ded55 100644
--- a/Core/Src/menu/menu.c
+++ b/Core/Src/menu/menu.c
@@ -671,6 +671,8 @@ uint8_t menu_loadedPresetNr[NUM_CURRENT];
 
 uint32_t sucessTimer = 0;
 /* Notification only: no page/input/transport state is changed. Latest error wins. */
+static volatile uint32_t menu_manualResult;
+static volatile uint32_t menu_manualSince;
 static volatile uint8_t menu_sdError;
 static volatile uint8_t menu_sdErrorHeld;
 static volatile uint8_t menu_sdErrorAuto;
@@ -995,7 +997,7 @@ void menu_tick()
 
 	if((menu_activePage == PATTERN_SAVE_PAGE_OK) || (menu_activePage == KIT_SAVE_PAGE_OK))
 	{
-		if(systick_ticks - sucessTimer > 4000)
+		if((uint32_t)(HAL_GetTick() - menu_manualSince) >= 1000u)
 		{
 
 			buttonHandler_setMode(SELECT_MODE_VOICE);
@@ -1933,9 +1935,55 @@ void menu_handleSaveScreenKnobValue(uint8_t potNr, uint8_t value)
 
 }
 //-----------------------------------------------------------------
+/* Manual results are foreground-published, bounded display overlays only. */
+void menu_showOperationResult(uint8_t category, uint8_t saving, uint8_t failed)
+{
+    uint8_t kind;
+    switch(category) {
+    case SAVE_TYPE_KIT: kind=1; break;
+    case SAVE_TYPE_MORPH: kind=2; break;
+    case SAVE_TYPE_GLO: kind=3; break;
+    case SAVE_TYPE_SONG: kind=4; break;
+    case SAVE_TYPE_PATTERN: kind=5; break;
+    default: return; /* Project feedback has its own established lifecycle. */
+    }
+    menu_sdError = FR_OK;
+    menu_manualSince = HAL_GetTick();
+    menu_manualResult = kind | (saving ? 8u : 0u) | (failed ? 16u : 0u);
+    menu_repaintAll();
+}
+static uint8_t menu_renderOperationResult(void)
+{
+    uint32_t result = menu_manualResult;
+    const char *label;
+    if(!result) return 0;
+    if((uint32_t)(HAL_GetTick()-menu_manualSince) >= 1000u) {
+        menu_manualResult=0;
+        menu_repaintAll();
+        return 0;
+    }
+    switch(result & 7u) {
+    case 1: label=(result & 8u) ? "Save Kit" : "Load Kit"; break;
+    case 2: label=(result & 8u) ? "Save Morph Kit" : "Load Morph Kit"; break;
+    case 3: label=(result & 8u) ? "Save Settings" : "Load Settings"; break;
+    case 4: label=(result & 8u) ? "Save Song" : "Load Song"; break;
+    default: label=(result & 8u) ? "Save Pattern" : "Load Pattern"; break;
+    }
+    visibleCursor=0;
+    memset(editDisplayBuffer,' ',sizeof(editDisplayBuffer));
+    strcpy(editDisplayBuffer[0],label);
+    strcpy(editDisplayBuffer[1],(result & 16u) ? "error!" : "success!");
+    display_needs_refresh=1;
+    if(menu_manualResult != result) menu_repaintAll();
+    return 1;
+}
 /* Rendered by the existing bounded display refresh, never by a wait loop. */
 static void menu_expireSdError(void)
 {
+    if(menu_manualResult && (uint32_t)(HAL_GetTick()-menu_manualSince) >= 1000u) {
+        menu_manualResult=0;
+        menu_repaintAll();
+    }
     if(menu_sdError != FR_OK && (uint32_t)(HAL_GetTick() - menu_sdErrorSince) >= 2000u) {
         menu_sdError = FR_OK;
         menu_repaintAll();
@@ -1943,6 +1991,7 @@ static void menu_expireSdError(void)
 }
 static uint8_t menu_renderSdError(void)
 {
+    if(menu_renderOperationResult()) return 1;
     const char *detail;
     uint8_t error = menu_sdError;
     if(error == FR_OK) return 0;
@@ -1970,6 +2019,7 @@ static uint8_t menu_renderSdError(void)
 void menu_showSdError(uint8_t res)
 {
     if(res == FR_OK) return;
+    menu_manualResult=0;
     menu_sdErrorHeld = preset_storageError() != FR_OK;
     menu_sdErrorAuto = preset_autosaveIsPaused();
     menu_sdErrorSince = HAL_GetTick();
@@ -2028,14 +2078,16 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 					switch(menu_saveOptions.category)
 					{
 						case SAVE_TYPE_SONG:
-							//nothing to do here
+							// No Song persistence command is implemented here.
+                            menu_showOperationResult(SAVE_TYPE_SONG,1,1);
+                            return;
 							break;
 						case SAVE_TYPE_PATTERN: {
 
 
 
 							FRESULT saveResult = preset_savePatternRAM(menu_selectedPresetNr[SAVE_TYPE_PATTERN]);
-							if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
+							if(saveResult != FR_OK) { menu_showOperationResult(SAVE_TYPE_PATTERN,1,saveResult); return; }
 
 							//save to tmp folder
 										#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
@@ -2049,7 +2101,8 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 										#else
 
 										#endif
-							if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
+							if(saveResult != FR_OK) { menu_showOperationResult(SAVE_TYPE_PATTERN,1,saveResult); return; }
+                            menu_showOperationResult(SAVE_TYPE_PATTERN,1,0);
 							break;
 						}
 
@@ -2072,9 +2125,10 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 
 							#endif
 
-							if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
+							if(saveResult != FR_OK) { menu_showOperationResult(SAVE_TYPE_KIT,1,saveResult); return; }
 							menu_loadedPresetNr[CURRENT_KIT] = menu_selectedPresetNr[SAVE_TYPE_KIT];
-							menu_activePage = KIT_SAVE_PAGE_OK;
+							menu_showOperationResult(SAVE_TYPE_KIT,1,0);
+                            menu_activePage = KIT_SAVE_PAGE_OK;
 							sucessTimer = systick_ticks;
 							menu_repaint();
 
@@ -2082,14 +2136,17 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 						}
 
 						case SAVE_TYPE_MORPH:
+                            if(menu_selectedPresetNr[SAVE_TYPE_MORPH] >= NUM_KIT_SLOTS) { menu_showOperationResult(SAVE_TYPE_MORPH,1,1); return; }
 							preset_saveDrumsetRam(menu_selectedPresetNr[SAVE_TYPE_MORPH],1);
+                            menu_showOperationResult(SAVE_TYPE_MORPH,1,0);
 							break;
 
 						case SAVE_TYPE_GLO:
 						{
 							FRESULT res;
 							res = preset_saveGlobals();
-							if(res != FR_OK) { menu_showSdError(res); return; }
+							if(res != FR_OK) { menu_showOperationResult(SAVE_TYPE_GLO,1,res); return; }
+                            menu_showOperationResult(SAVE_TYPE_GLO,1,0);
 						}
 							break;
 
@@ -2123,10 +2180,11 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 					{
 						for(uint8_t i=0;i<NUM_PATTERN;i++)
 						{
-							preset_savePatternRAM(i);
+							if(preset_savePatternRAM(i) != FR_OK) { menu_showOperationResult(SAVE_TYPE_PATTERN,1,FR_INT_ERR); return; }
 
 						}
-						menu_activePage = PATTERN_SAVE_PAGE_OK;
+						menu_showOperationResult(SAVE_TYPE_PATTERN,1,0);
+                        menu_activePage = PATTERN_SAVE_PAGE_OK;
 						sucessTimer = systick_ticks;
 						menu_repaint();
 					}
@@ -2190,6 +2248,7 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 				switch(menu_saveOptions.category) {
 
 				case SAVE_TYPE_KIT:
+                    if(menu_selectedPresetNr[SAVE_TYPE_KIT] >= NUM_KIT_SLOTS) { menu_showOperationResult(SAVE_TYPE_KIT,0,1); return; }
 #if 1
 
 					preset_loadDrumsetRam(menu_selectedPresetNr[menu_saveOptions.category],0, 0);
@@ -2211,9 +2270,11 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 					// **LFOTARGFIX - save the gap index
 					menu_TargetVoiceGapIndex = getModTargetGapIndex(activeParameters[PAR_TARGET_LFO1 + menu_activeVoice]);
 #endif
-					break;
+					menu_showOperationResult(SAVE_TYPE_KIT,0,0);
+                    break;
 
 				case SAVE_TYPE_MORPH:
+                    if(menu_selectedPresetNr[SAVE_TYPE_MORPH] >= NUM_KIT_SLOTS) { menu_showOperationResult(SAVE_TYPE_MORPH,0,1); return; }
 #if 1
 					preset_loadDrumsetRam(menu_selectedPresetNr[menu_saveOptions.category],1, 0);
 					activeParameters[PAR_MORPH_KIT_DEST] = menu_selectedPresetNr[menu_saveOptions.category];
@@ -2222,40 +2283,23 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 					//load to morph buffer
 					preset_loadDrumset(menu_selectedPresetNr[menu_saveOptions.category],1);
 #endif
-					break;
+					menu_showOperationResult(SAVE_TYPE_MORPH,0,0);
+                    break;
 
 				case SAVE_TYPE_SONG:
+                    if(menu_selectedPresetNr[SAVE_TYPE_SONG] >= NUM_KIT_SLOTS) { menu_showOperationResult(SAVE_TYPE_SONG,0,1); return; }
 					//just set pointer to other song
 					seq_activeSong = seq_songData[menu_selectedPresetNr[menu_saveOptions.category]];
 					menu_loadedPresetNr[CURRENT_SONG] = menu_selectedPresetNr[menu_saveOptions.category];
-					break;
+					menu_showOperationResult(SAVE_TYPE_SONG,0,0);
+                    break;
 
 				case SAVE_TYPE_PATTERN:
-					//if(preset_loadPatternSD(menu_currentPresetNr[SAVE_TYPE_PATTERN]))
-						//menu_resetSaveParameters();
-
-					switch(menu_saveOptions.state)
-					{
-						case SAVE_STATE_CHANGE_NAME_DIALOG_Y:
-							for(uint8_t i=0;i<NUM_PATTERN;i++)
-							{
-								preset_loadPatternRAM(i);
-								menu_resetSaveParameters();
-							}
-							//editModeActive = 1; //to prevent confirmation encoder click from setting edit mode active (toggle)
-							//buttonHandler_buttonPressed(BUT_MODE1); //return to main menu
-							menu_saveOptions.state = SAVE_STATE_CHANGE_NAME_DIALOG_N;
-							break;
-
-						default:
-						case SAVE_STATE_CHANGE_NAME_DIALOG_N:
-							break;
-					}
-					break;
-
-
+                    /* Existing preset_loadPatternRAM is a no-op. */
+                    menu_showOperationResult(SAVE_TYPE_PATTERN,0,1);
+                    break;
 
-				case SAVE_TYPE_PROJECT:
+                case SAVE_TYPE_PROJECT:
 				{
 
 					FRESULT res;
@@ -2286,7 +2330,7 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 					break;
 */
 				case SAVE_TYPE_GLO:
-					preset_loadGlobals();
+					menu_showOperationResult(SAVE_TYPE_GLO,0,!preset_loadGlobals());
 					menu_resetSaveParameters();						
 					break;
 #if USE_SAMPLES
diff --git a/tests/live_storage/run.py b/tests/live_storage/run.py
index c2254d9..8c29ed0 100644
--- a/tests/live_storage/run.py
+++ b/tests/live_storage/run.py
@@ -8,7 +8,8 @@ spec=importlib.util.spec_from_file_location('save_helpers',ROOT/'tests/save_erro
 helpers=importlib.util.module_from_spec(spec);spec.loader.exec_module(helpers)
 text=(ROOT/'Core/Src/menu/menu.c').read_text()
 functions=['menu_expireSdError','menu_renderSdError','menu_showSdError','menu_tick','menu_repaintAll','menu_repaint']
-source=''.join(helpers.extract(text,n) for n in functions)
+functions=['menu_showOperationResult','menu_renderOperationResult']+functions
+source='static volatile uint32_t menu_manualResult,menu_manualSince;\n'+''.join(helpers.extract(text,n) for n in functions)
 for name in functions[:3]:
     body=helpers.extract(text,name)
     assert not re.search(r'\b(?:HAL_Delay|HAL_TIM_\w+|lcd_\w+|sendDisplayBuffer|f_\w+|soundengine_process)\s*\(',body)
diff --git a/tests/morph_range/run.py b/tests/morph_range/run.py
index f703468..bce982d 100644
--- a/tests/morph_range/run.py
+++ b/tests/morph_range/run.py
@@ -12,7 +12,9 @@ dtypes=menu[start:end]
 name_start=menu.index('const Name valueNames');name_end=menu.index('};',name_start)+2
 dtypes=menu[name_start:name_end]+'\n'+dtypes
 old=subprocess.check_output(['git','show','5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527:Core/Src/menu/menu.c'],cwd=ROOT).decode().replace('\r\n','\n')
-assert menu.replace('/*PAR_MORPH*/ \t\t\tDTYPE_0B127','/*PAR_MORPH*/ \t\t\tDTYPE_0B255')==old
+for name in ['menu_encoderChangeParameter','limitPotValue','menu_parseKnobValue','menu_repaintGeneric','menu_parseGlobalParam']:
+ assert h.extract(menu,name)==h.extract(old,name)
+assert '/*PAR_MORPH*/ \t\t\tDTYPE_0B127' in menu
 # No serialization or automation format is redefined. Existing bytes and guards
 # are exercised below and in the established full regression suites.
 assert 'PAR_MORPH' not in (ROOT/'Core/Src/menu/Cc2Text.c').read_text()
diff --git a/tests/save_errors/host.c b/tests/save_errors/host.c
index 83f9e62..4907ff0 100644
--- a/tests/save_errors/host.c
+++ b/tests/save_errors/host.c
@@ -85,6 +85,7 @@ void menu_repaintAll(void){repaints++;}
 void menu_repaint(void){repaints++;}
 void menu_resetSaveParameters(void){resets++;}
 void menu_showSdError(uint8_t error){notifications++;last_notification=(FRESULT)error;}
+void menu_showOperationResult(uint8_t category,uint8_t saving,uint8_t failed){(void)category;(void)saving;if(failed)menu_showSdError(failed);}
 FRESULT preset_savePatternRAM(uint8_t p){(void)p;return ram_error;}
 void preset_saveDrumsetRam(uint8_t p,uint8_t morph){(void)p;(void)morph;}
 FRESULT f_stat(const TCHAR *name,FILINFO *info){(void)name;(void)info;directory_calls++;return FR_OK;}
diff --git a/tests/silent_autosave/run.py b/tests/silent_autosave/run.py
index 82f417d..5be829e 100644
--- a/tests/silent_autosave/run.py
+++ b/tests/silent_autosave/run.py
@@ -9,6 +9,9 @@ h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
 text=(root/'Core/Src/Preset/presetManager.c').read_bytes().decode('latin1').replace('\r\n','\n')
 for path,want in json.loads((Path(__file__).parent/'baseline_sources.json').read_text()).items():
     actual=h.extract(text,path).encode() if path=='preset_touchTimer' else (root/path).read_bytes()
+    if path in ['Core/Src/menu/menu.c','Core/Src/buttonHandler.c']:
+        assert hashlib.sha256(actual).hexdigest()==json.loads((Path(__file__).parent/'manual_feedback_boundary.json').read_text())[path],path
+        continue
     if path=='Core/Src/menu/menu.c':
         old=b'/*PAR_MORPH*/ \t\t\tDTYPE_0B255';new=b'/*PAR_MORPH*/ \t\t\tDTYPE_0B127'
         assert actual.count(new)==1
--- /dev/null
+++ b/tests/manual_feedback/run.py
@@ -0,0 +1,48 @@
+"""Actual manual overlay/tick/render bodies with deterministic LCD/time mocks."""
+from pathlib import Path
+import importlib.util,subprocess,tempfile,os,sys
+sys.dont_write_bytecode=True
+R=Path(__file__).resolve().parents[2]
+spec=importlib.util.spec_from_file_location('h',R/'tests/save_errors/run.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
+s=(R/'Core/Src/menu/menu.c').read_text();old=subprocess.check_output(['git','show','83c5ec164d6c3dab439deb2f32a8edf1138b5b77:Core/Src/menu/menu.c'],cwd=R,text=True)
+# Projects retain the actual save/load command bodies, including their old errors.
+for source in [s,old]:
+ save=source[source.index('void menu_handleLoadSaveMenu'):]
+ parts=save.split('case SAVE_TYPE_PROJECT:')
+ if source==s:current=[x.split('case SAVE_TYPE_ALL:')[0].strip() for x in parts[1:3]]
+ else:assert current==[x.split('case SAVE_TYPE_ALL:')[0].strip() for x in parts[1:3]]
+assert 'menu_showOperationResult(SAVE_TYPE_SONG,1,1)' in s
+assert 'menu_showOperationResult(SAVE_TYPE_PATTERN,0,1)' in s
+assert 'menu_showOperationResult(SAVE_TYPE_GLO,0,!preset_loadGlobals())' in s
+assert 'menu_showOperationResult' not in (R/'Core/Src/Preset/presetManager.c').read_text()
+functions=['menu_showOperationResult','menu_renderOperationResult','menu_expireSdError','menu_renderSdError','menu_showSdError','menu_tick','menu_repaintAll','menu_repaint']
+source='static volatile uint32_t menu_manualResult,menu_manualSince;\n'+''.join(h.extract(s,n) for n in functions)
+host=(R/'tests/live_storage/notifications.c').read_text().replace('int main(void)','int legacy_main(void)')+'''
+int main(void){
+ legacy_main();
+ const unsigned kinds[]={SAVE_TYPE_KIT,SAVE_TYPE_MORPH,SAVE_TYPE_GLO,SAVE_TYPE_SONG,SAVE_TYPE_PATTERN};
+ const char *load[]={"Load Kit","Load Morph Kit","Load Settings","Load Song","Load Pattern"};
+ const char *save[]={"Save Kit","Save Morph Kit","Save Settings","Save Song","Save Pattern"};
+ for(unsigned k=0;k<5;k++)for(unsigned saving=0;saving<2;saving++)for(unsigned fail=0;fail<2;fail++){
+  menu_manualResult=0;reset();now=UINT32_MAX-500;
+  menu_showOperationResult(kinds[k],saving,fail);menu_tick();
+  assert(!strcmp(editDisplayBuffer[0],saving?save[k]:load[k]));
+  assert(!strcmp(editDisplayBuffer[1],fail?"error!":"success!"));
+  menu_activePage=LOAD_PAGE;now+=999;menu_tick();assert(menu_manualResult && menu_activePage==LOAD_PAGE);
+  now++;menu_tick();assert(!menu_manualResult && menu_activePage==LOAD_PAGE);
+ }
+ for(unsigned i=0;i<10;i++){menu_showOperationResult(SAVE_TYPE_KIT,1,i<9);menu_repaint();assert(!strcmp(editDisplayBuffer[1],i<9?"error!":"success!"));}
+ menu_showSdError(FR_DISK_ERR);assert(!menu_manualResult);menu_tick();assert(menu_sdError==FR_DISK_ERR);
+ now+=2000;menu_tick();assert(!menu_sdError);
+ menu_showOperationResult(SAVE_TYPE_PROJECT,1,0);assert(!menu_manualResult);
+ menu_activePage=KIT_SAVE_PAGE_OK;menu_showOperationResult(SAVE_TYPE_KIT,1,0);now+=1000;menu_tick();assert(menu_activePage!=KIT_SAVE_PAGE_OK && !menu_manualResult);
+ menu_showOperationResult(SAVE_TYPE_KIT,0,0);expire_on_copy=1;menu_repaint();menu_tick();assert(!menu_manualResult);
+ puts("PASS 20 label/result combinations, 999/1000ms and wrap, current-page restoration, repeated failure/retry, project priority/unchanged bodies, old success-page expiry and copy/expiry interleaving");
+}
+'''
+with tempfile.TemporaryDirectory() as d:
+ d=Path(d);(d/'functions.inc').write_text(source);(d/'host.c').write_text(host)
+ flags=['cc','-std=gnu11','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast']
+ inc=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc','Middlewares/Third_Party/FatFs/src','FATFS/App','FATFS/Target']
+ subprocess.run(flags+['-I'+str(R/x) for x in inc]+['-I'+str(d),str(d/'host.c'),'-o',str(d/'host')],check=True)
+ env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0';subprocess.run([str(d/'host')],env=env,check=True,timeout=20)
--- /dev/null
+++ b/tests/silent_autosave/manual_feedback_boundary.json
@@ -0,0 +1,4 @@
+{
+  "Core/Src/menu/menu.c": "de61ba5f9059d87e93168b3eb8480856f85e8982b960dd3aac0e36d1c1d85549",
+  "Core/Src/buttonHandler.c": "0f8c2929f31806b410777952e8ab664249e1d6e7539469af8121dfc8dae9b126"
+}

```
