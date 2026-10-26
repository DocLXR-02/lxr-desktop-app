# LXR Firmware Lab — Euclidean sequencer length-16 investigation

Date: 2026-10-26 (UTC). Repository: [DocLXR-02/lxr-desktop-app](https://github.com/DocLXR-02/lxr-desktop-app).

## 1. Handoff status, authority and exact scope

This is a **source investigation and safe host reproduction**, not an implemented firmware correction. No production firmware, repository tests, build configuration, existing branch, or Bug 001 correction was changed. No hardware was flashed or tested, no PR or merge was made, and no firmware image or build artifact is published with this report.

The user authorized a new documentation-only publication based on `lab-setup`. Instructions were read through GitHub from [lab-setup/AGENTS.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/lab-setup/AGENTS.md), blob `2f4d121f53a7055c1a30355235384a95ef920f1a`. These require protecting main, diagnosing before code changes, distinguishing observations from tests and hypotheses, and a self-contained, verified handoff. No sub-agents were used.

| Reference | Exact value |
|---|---|
| Existing local checkout, inspected before work | `/workspace/lxr-desktop-app`, clean `bug001/filter-drive-fix` |
| Investigated local HEAD | `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0` |
| Parent of that HEAD and required report base | `5829398cea5d337200393757e39a37d4d8148256` (`lab-setup`) |
| Public/fork main reference | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| New report-only branch | `codex-reports/2026-10-26-euclidean-length16-investigation` |
| New report path | `docs/codex-berichte/2026-10-26_euclidean-length16-investigation.md` |
| Source version metadata | Top-level `CMakeLists.txt`: `project(... VERSION 1.7 ...)` |

**Line references below use investigated commit `7d9fe99…`, unless stated otherwise.** Immutable source URLs can be formed as `https://github.com/DocLXR-02/lxr-desktop-app/blob/7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0/<path>#L<number>`. The report branch deliberately excludes the Bug 001 code/test commit, because it is based directly on `lab-setup`; this is not a rollback of that separate branch. Sequencer, generator, UI and CortexParser source is unchanged by Bug 001. Pattern-I/O functions in `presetManager.c` are unchanged too; line positions after Bug 001 additions move by 13 lines in the pattern-I/O region.

Previous context: [completed Bug 001 compatibility audit](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-bug001-compatibility-audit/docs/codex-berichte/2026-10-26_bug001-firmware-compatibility-audit.md). That audit identified the length representation inconsistency. This report independently traces and reproduces it. The public reference is official v1.71; the user's former private v1.72 support build is not the reference. No official binary was acquired or examined in this investigation. No binary/source equivalence, identification of the official v1.71 patch, or behavior of either official binary is claimed.

## 2. Confirmed conclusions

1. **One active producer explicitly converts Euclidean length 16 into stored zero:** `euklid_transferPattern()` in `Core/Src/Soundengine/Sequencer/EuklidGenerator.c:235–240`. This is an obsolete sentinel in a field that is now a full byte. The ordinary length setter no longer performs this conversion, and the ordinary getter no longer reverses it: both old conversions are commented out.
2. **Normal UI input reaches the producer.** Length is bounded to 1–16 by the UI. Changing length, pulses, or rotation regenerates and transfers the pattern. The defect does not require an invalid MIDI packet, corrupt file, or out-of-range Euclidean setting.
3. **The zero is handled inconsistently.** Main step advance decodes it as 16, but active flam/shift playback performs `% 0` at `sequencer.c:1111`; live note recording independently performs `% 0` at `sequencer.c:2402`. Both failures were reproduced using original function bodies under host UBSan.
4. **There are related indexing and UI defects.** Zero can derive previous step 255. Separately, last-step UI decrement changes zero to 255; the unchecked setter accepts it. Isolated host tests detected illegal main-step array indices 15 and 4, respectively. The previous-step test deliberately bypasses the earlier modulo failure: it proves downstream unsafe indexing, not a deterministic chain of hardware execution after undefined behavior.
5. **The zero persists.** Ordinary pattern saves and autosaves write raw length bytes, and both active loaders restore raw zero. Actual source serializer/loader function bodies reproduced this with an in-memory FatFs seam. Reloading an existing affected file therefore reintroduces the problem even if new Euclidean generation is corrected.
6. **Longest-track selection is wrong for mixed legacy/literal lengths.** For raw lengths `[0,8,1,1,1,1,1]`, it selects the 8-step track rather than the effective 16-step track. This was directly tested. The downstream consequence of an early pattern-change boundary follows from source; a full clock/song simulation was not executed.
7. **The compiled internal SysEx length receiver is another unchecked ingestion path.** It accepts zero and does not bound its receive counter. Both raw-zero acceptance and an out-of-range pattern index 64 were reproduced. This is an internal Cortex/front-panel protocol defect, distinct from standard external MIDI NRPN parsing and distinct from the normal Euclidean UI trigger.
8. **A separate generator portability issue exists:** original CMSIS `__CLZ` expands to `__builtin_clz`, which is called with zero during normal generation. Native UBSan reports this. Host length tests explicitly model ARM's CLZ instruction zero result (32) to isolate the length defect. This does not establish that the ARM build fails at this instruction.

The evidence proves defects in the released C source. It does **not** prove a specific hardware symptom, hard fault, memory corruption outcome, or the behavior of public v1.71/private v1.72 binaries.

## 3. Complete normal input and generation path

### 3.1 Selection and value routing

- Euclidean page entry is exposed through `Core/Src/buttonHandler.c:488–490` and other page/voice-selection cases. Its layout is `Core/Inc/menu/menuPages.h:465–466`: length, steps, rotation.
- `Core/Src/IO/adcPots.c:174` (`adc_checkPots`) calculates knob position changes; the STM32 path around `:210–232` calls `menu_parseKnobValue(i, diff > 0 ? 1 : -1)` for accepted movement. Encoder editing enters `menu_parseEncoder()` (`menu.c:2729`, called by `main.c:959`), whose parameter-edit route calls `menu_parseGlobalParam()` at `menu.c:3027`.
- `menu_parseKnobValue()` (`menu.c:3917–3951`) resolves the selected page's parameter and calls `limitPotValue()`. It stores `dtypeValue` into `activeParameters` at `:4123–4127` and routes global parameters to `menu_parseGlobalParam()` at `:4242`.
- Types are defined at `menu.c:518–520`: length and pulses `DTYPE_1B16`, rotation `DTYPE_0B15`. Limits at `:3780–3789` enforce length/pulses 1–16 and rotation 0–15. The ordinary track-length parameter instead has range 1–64 (`:529`, `:3784–3787`).
- In `menu_parseGlobalParam()` (`menu.c:3418`), length selection (`:3577–3596`) obtains `menu_shownPattern`, selects the active voice, calls `euklid_setLength(voice, length+1)` where the local `length` was `value-1`, and immediately calls `euklid_setSteps(voice, activeParameters[PAR_EUKLID_STEPS], pattern)`. Thus the effective length sent to the generator is the selected 1–16 value.
- Pulse edits (`:3601–3615`) call `euklid_setSteps`; rotation edits (`:3617–3628`) call `euklid_setRotation`. Both generate/transfer again. The UI refreshes LEDs from the stored main-step masks.

`frontPanel_sendData()` (`Core/Src/frontPanelParser.c:618–633`) synchronously feeds status/data bytes to `frontParser_parseUartData()` on the non-AVR path. The current Euclidean UI directly calls the generator; old packed Cortex Euclidean parameter cases also remain compiled at `CortexParser.c:758–784`, but their send calls in the current menu are commented. Do not mistake those commented sends for the normal path.

### 3.2 Generator state, mask creation and destructive transfer

`Core/Src/Soundengine/Sequencer/EuklidGenerator.c` has private per-track length, pulse and rotation arrays (`:21–23`). `euklid_init()` (`:36–46`, called by `Soundengine.c:51`) initializes them to 16, 4 and 0. This private configuration is separate from `PatternSet.length`; `euklid_getLength()` can return 16 while `seq_getTrackLength()` returns 0 for the same generated track.

`euklid_setLength()` (`:180–185`) only lower-bounds to 1; its upper-bound check is commented. The regular UI supplies a maximum of 16. `euklid_setSteps()` (`:187–195`) clamps pulses to at least 1 and at most the private length, then generates. `euklid_setRotation()` (`:202–207`) retains the previous rotation if the supplied value exceeds length, then generates. The matrix test includes all UI rotation inputs but does not assert that rejected inputs became the stored rotation.

`euklid_generate()` (`:142–166`) reads private values, clears the 64-bit `ActiveSteps` union, invokes `euklid_calcRecursive(length,steps,1,1,0)`, rotates the mask, and transfers it. The recursive `% steps` at `:65–69` is guarded against zero; it is not either of the confirmed playback/recording zero divisors. The mask uses four 16-bit words (`sequencer.h:81`), matching four bars. Transfer overwrites all four words for the chosen pattern/track (`EuklidGenerator.c:225–230`), then converts 16 to zero and writes the length (`:235–240`). It does not itself rewrite every note/velocity/automation payload.

In current configuration `USE_PATTERN_EDIT_BUFFER` is 0 (`Core/Inc/Soundengine/Sequencer/sequencer.h:137`). The active destination is `seq_patternSet.length[patternNr][trackNr]`. The alternate `patternBackupBuffer.length[trackNr]` write at generator `:238` would receive the same converted value if that disabled configuration were enabled.

## 4. Storage representation and complete writer inventory

`PatternSet` (`sequencer.h:97–106`) contains:

```c
Step seq_subStepPattern[NUM_PATTERN][NUM_TRACKS][NUM_STEPS];
StepTiming seq_stepTiming[NUM_PATTERN][NUM_TRACKS][NUM_STEPS];
uint16_t seq_mainSteps[NUM_BARS][NUM_PATTERN][NUM_TRACKS];
uint8_t length[NUM_PATTERN][NUM_TRACKS];
```

`NUM_PATTERN=64`, `NUM_TRACKS=7`, `NUM_STEPS=64`, `NUM_BARS=4`. Length is **not a four-bit bitfield**. The ordinary track-length UI accepts 1–64. Initialization (`seq_init():180–207` → `seq_clearPattern()` → `seq_clearTrack():2556`) stores literal 16. A zero-initialized BSS before initialization is not a separate intentional 16-to-zero encoding. The normal pattern serializers save the generated masks, step/timing payloads, lengths, shuffle and associated kit; they do not serialize the private Euclidean length/pulse/rotation arrays. Loading therefore restores the resulting pattern, not a separate saved generator configuration. The private values are initialized separately and queried by the Euclidean page (`CortexParser.c:884–897`). No inference about their values should be made solely from a loaded track length.

| Active writer / transport | Source references | What it does to zero |
|---|---|---|
| Euclidean transfer | `EuklidGenerator.c:221–243`, decisive `:235–240` | **Only active explicit 16→0 conversion found**; writes active pattern, alternate edit-buffer destination disabled |
| Ordinary setter | `sequencer.c:274–290`, write `:288` | Stores supplied byte unchanged; accepts 0, 65…255 and unchecked track/pattern indices |
| Internal front-panel track length | `CortexParser.c:859–861` | Delegates raw value to ordinary setter |
| Internal SysEx length receive | `CortexParser.c:255–300`, write `:282` | Stores raw received data; zero accepted; pattern/track derived from unchecked receive counter |
| Normal pattern load | `presetManager.c:1966–1970` | Assigns raw file byte to length; no 0→16 decode or range check |
| Autosave pattern load | `presetManager.c:438–444` | Same raw assignment |
| Pattern backup / restore | `sequencer.c:225–253`, memcpy `:233`, `:250` | Copies all seven bytes unchanged |
| Track copy | `sequencer.c:2613` | Copies raw source length |
| Pattern copy | `sequencer.c:2645` | Copies raw source lengths |
| Clear / initialization | `sequencer.c:2556` | Writes **literal 16** |
| Normal save / autosave | `presetManager.c:1631–1632`, `:573–574` | Serializes raw zero; does not itself invent the sentinel |

Commented/disabled lookalikes are not additional live writers: the 16→0 code inside `seq_setTrackLength():278–280`; temporary-pattern activation/copy around `sequencer.c:211–222`, `:255–267`; alternate edit-buffer paths; disabled pattern-reader branch around `presetManager.c:1713–1878`, including assignments `:1861–1863`; and disabled bulk-project format. The old length/rotation representation in commented code is not the active data structure.

Repository-wide `rg` searches for direct `length` field references, track-length getters/setters and length-related protocol constants were reviewed. Native preprocessing of the relevant translation units confirmed active versus removed code; see commands/limitations below.

## 5. Consumer inventory, exact failures and causal limits

| Consumer | Source | Zero behavior / significance |
|---|---|---|
| Main clock master-length wrap | `sequencer.c:1270–1278` | Converts selected track's zero to 16, but selection already used wrong raw ordering |
| Per-track main-step advance | `sequencer.c:1353–1363` | Explicitly treats zero as 16; does not fix other consumers |
| Longest-track selection | `sequencer.c:831–843`, compare `:837` | Compares raw bytes; mixed 0/8 selects 8 over effective 16 |
| Flam/shift playback | `sequencer.c:1091–1155`, `:1109–1112` | Unconditional raw modulo for every unmuted track while running: `%0`; previous-step expression can yield 255 |
| Shift/flam lookup | `sequencer.c:989–1087` → `seq_intIsMainStepActive():2034–2038` | No step bound before deriving `bar=step/16`; 255→15 and 64→4 exceed bar dimension 4 |
| Live note recording | `sequencer.c:2364–2417`, modulo `:2402` | `%0` when running and recording, independent of flam call |
| Public length getter | `sequencer.c:293–305` | Returns raw zero; historical decode at `:301–304` is commented |
| Last-step encoder edit | `menu.c:2772–2792` | With zero and negative increment, unsigned length underflows; −1→255; setter stores it |
| Last-step text | `menu.c:1112`, `:1120` | Passes raw zero to number formatter; cannot correctly display intended 16; exact visible formatting not hardware-tested |
| Last-step blink selection | `buttonHandler.c:913`, `:1252`, `:1269`, `:1663`, `:1717` | `getLength()-1` becomes 255. Active-bar check normally hides blink; `%16` bounds LED argument. No LED-array OOB claim from these lines |
| UI parameter refresh | `ledHandler.c:417`; active pending-pattern UI block `sequencer.c:787` | Propagates raw zero into `activeParameters[PAR_TRACK_LENGTH]` |
| Legacy external-step helper | `seq_triggerNextMasterStep():1555–1583` | Explicit zero→16, then legacy ×8 arithmetic; function compiles but found callers in `TriggerOut.c:89` and `:205` are disabled/commented in current config |
| File persistence / copy | Writer inventory above | Preserves zero across save, load, copy and undo-style backup/restore |

The apparent getter caller `sequencer.c:705` is in a commented block; `CortexParser.c:683` is also in a commented LED query block. Old modulo lookahead in `seq_getShiftFlamPattern():945–946` is `#if 0`; active lookahead is in `seq_processFlam()`. The alternate flam implementation around `:1174–1175` is disabled by `#if 1` at `:1108`. Old rotation modulo around `sequencer.c:310–350`, length sending around `:2093`, and old reset-start calculations around `:2770–2793` are commented. None is counted as another current runtime defect.

### 5.1 Playback is reachable without choosing an audible flam effect

`seq_tick()` (`sequencer.c:1650ff`) calls `seq_processFlam()` on internal sub-ticks (`:1831`) and the active external-sync upscaling route (`:1730`). `seq_processFlam()` checks running and mute state, then evaluates `nextStep=(index+1)%length` **before** testing whether generated note/flam patterns produce a trigger. Setting flam to its ordinary value is not protection from this arithmetic.

`TIM2_IRQHandler()` (`Core/Src/stm32f7xx_it.c:261–273`) calls `seq_tick()` before sample-block calculation and MIDI processing when `CALC_SAMPLES_IN_INTERRUPT=1` (`config.h:29`). `soundengine_process()` also calls it when temporarily advancing processing from the main loop (`Soundengine.c:77–91`). Main MIDI sync advances steps via `clockSync.c:45–64`. This is a real-time path: undefined arithmetic/indexing can interrupt expected scheduling and audio behavior. Exact ARM output, exception behavior and timing remain unmeasured; `%0` in C is undefined regardless of whether a particular ARM division instruction traps or returns a defined instruction-level value. No explicit application `DIV_0_TRP` configuration was found by the targeted search; that does not establish live hardware control-register state.

### 5.2 Two distinct OOB demonstrations

- **Derived previous step:** at current index 0 and raw length 0, `length-1` narrows to `uint8_t` 255. Calling the actual downstream lookup with 255 indexes `seq_mainSteps[15]`. The host test intentionally isolates this expression and lookup, since real `seq_processFlam()` encounters the zero divisor first. Once undefined behavior has occurred, a deterministic subsequent hardware memory access cannot be asserted.
- **UI-created invalid length without a zero divisor at lookup:** getter returns 0; the exact last-step decrement arithmetic produces 255; actual `seq_setTrackLength()` stores 255. With current index 63, next step becomes `(63+1)%255=64`; actual flam lookup indexes `seq_mainSteps[4]`, outside `[0..3]`. This host test uses the UI arithmetic seam, not the entire menu routine, and a deliberately selected valid old step index 63. Source main advance's 64-step mask may wrap later; it does not validate this already computed next-step lookahead. A previously playing 64-step track that is shortened during live editing is one plausible source of an old current index63; the harness selects it explicitly and does not reproduce UI/timer interleaving on hardware.

Both UBSan diagnostics establish invalid C indexing. They do not prove a lasting write into adjacent production memory: the first detected operation is a read in `seq_intIsMainStepActive()`.

### 5.3 Internal SysEx boundary defect

The active `SYSEX_RECEIVE_PAT_LEN_DATA` case (`CortexParser.c:255–300`) computes `currentPattern=frontParser_sysexSeqStepNr/7` and `currentTrack=counter-currentPattern*7`, writes length, and increments the counter. There is no check against `NUM_PATTERN*NUM_TRACKS` (448) and no length validation. At counter 448 it indexes pattern 64 outside `[0..63]`. Native preprocessing and an extracted-body sanitizer test confirm the case is compiled. A test calling the handler at that state produces the bounds diagnostic at `:282`. The input parser resets the counter when selecting a SysEx mode (`:384–390`); a sufficiently long length stream can reach 448. Host tests set the state directly rather than simulating all 448 incoming bytes.

`frontParser_parseUartData():99–143` dispatches SysEx to this handler; `Uart.c:131` calls the parser for front FIFO bytes, and non-AVR `frontPanel_sendData()` feeds the same parser synchronously for normal internal messages. This is not evidence that standard external DIN/USB SysEx is routed into this private transport: that broader routing was not established. The ordinary Euclidean UI does not send this length stream.

## 6. Save/load and compatibility with existing pattern files

### 6.1 Active formats and raw-zero persistence

`preset_savePatternSD()` (`presetManager.c:1674–1710`) writes the 8-byte preset name, six-byte ASCII `PatV03` tag, then `preset_writePatternData()` (`:1592–1639`). Active project-save logic calls the normal writer (`:2445–2464`). The body is serialized per field, not by dumping `PatternSet` including host padding.

| V3 body region | Bytes | Body-relative start |
|---|---:|---:|
| Seven tracks of 64 `Step` structs plus 64 timing bytes per track | 3584 = 7×64×(7+1) | 0 |
| Four 16-bit main-step masks per track | 56 = 7×4×2 | 3584 |
| Shuffle | 1 | 3640 |
| Seven raw track lengths | 7 | **3641** |
| Associated kit number | 1 | 3648 |
| Total body | **3649** | |

`sizeof(Step)==7` was used by the actual host serializer and confirmed by the measured body size. A current named `PatV03` file is therefore 3663 bytes, with lengths starting at absolute byte 3655 (14-byte wrapper + body offset 3641). Those full-file numbers are derived from source wrapper writes; the named wrapper's filesystem/open/name handling was not executed by the host harness.

`preset_loadPatternSD()` (`:2033–2090`) reads the name and tag, parses `PatVxx`, or rewinds six bytes and assumes version 1 for an untagged pattern. The active branch of `preset_readPatternData()` starts at `:1879`; the large earlier branch is disabled. It reads timing only for version >2, raw lengths at `:1966–1970`, and associated kit for version >1 at `:1975–1979`. No sentinel decode occurs.

`preset_saveTmpPat()` (`:491–595`) writes `tmp.pat` with one pattern-index byte followed by the V3-style body, without the 14-byte named wrapper. Thus length offset is 3642 and size 3650. `preset_LoadTmpPat()` (`:302–489`) validates the first pattern index against 64 but restores lengths raw at `:443`. Host tests executed these complete two temporary-pattern wrappers with in-memory FatFs stubs; they preserved zero.

Autosave is timer-driven (`preset_processTimer():666–675`, main loop `main.c:992`). `seq_setRunning(0)` arms the pattern save timer (`sequencer.c:1962`); other edits also arm it. Euclidean transfer itself is not claimed to directly mark pattern dirty: its normal menu path touches the kit timer. The confirmed point is that **whenever pattern save/autosave occurs with the affected pattern in memory, it writes the zero unchanged**. Startup loads ordinary temporary-folder files and then `tmp.pat` (`main.c:896–904`), so both loaders need coverage in any ingestion-based correction.

### 6.2 Missing metadata is different from a valid legacy zero byte

Both active readers comment that lengths may be absent. Each initializes local `next=0` (`presetManager.c:366`, `:1887`), calls `f_read()` for each length without checking return result or actual count, and assigns `next` regardless. EOF can preserve the prior byte in the local variable.

The synthetic version-1 host fixtures demonstrate:

- A valid core body ending just after shuffle, with no length tail, is accepted and all seven lengths become zero.
- A body with just one length byte 5 and then EOF is accepted and all seven lengths become 5, including six nonexistent bytes.

These are synthetic layouts exercising the reader's explicit version-1 compatibility route, not captured official files. Version >1 additionally requires an associated-kit byte; truncation can mark failure while earlier field assignments have already modified memory. General transactional loading, complete write-count handling and SD error recovery were not redesigned or tested.

### 6.3 Recommended interpretation and file policy

For a **present** length byte, source evidence supports:

| File value | Intended interpretation / recommended action |
|---|---|
| 0 | Legacy effective length **16**, decoded to literal 16 in RAM |
| 1…64 | Keep unchanged |
| 65…255 | Invalid for current storage/playback; reject safely or apply an explicit documented fallback, not unchecked indexing |
| Missing optional V1 length field | Initialize that track to explicit 16 per compatibility policy; never reuse the preceding byte |
| Truncated required current-format metadata / I/O error | Report load failure and avoid making unsafe partial state playable; define recovery behavior before implementation |

A complete old zero-length byte and a read error must not be conflated. For genuinely older optional metadata, default 16 is consistent with initialization and historic zero fallback. Actual historical V1/V2 fixtures are needed before promising exhaustive backward compatibility; the source's comments acknowledge earlier length-layout changes.

Writing literal 16 instead of 0 changes no byte count, tag, or field order, and the current old reader accepts literal 16. The 64-value roundtrip matrix confirms that V3 body reader/writer preserve every literal length 1…64. No on-disk version bump is needed **for the 0-versus-16 representation correction alone**. This is compatibility with this source-defined format; compatibility with official v1.71 or private v1.72 file producers/readers remains unverified. Do not rewrite users' existing files in place as a migration; decode on read and write the canonical value on a later ordinary save.

## 7. Host reproduction: executed commands and exact outcomes

Temporary files were placed outside the repository:

- `/tmp/euclid_fixture.c`: complete host fixture in Appendix A.
- `/tmp/euclid_run.py`: extractor/build/runner in Appendix B.
- `/tmp/euclid-investigation/functions.inc`: original production function bodies extracted at runtime with original `#line` locations.
- `/tmp/euclid-investigation/probe` and `probe-original-clz`: host executables, not firmware.
- `/tmp/euclid-investigation/results.json`, per-case `.log`, `build.log`, `build-original-clz.log`.
- `/tmp/euclid-length-references.txt`: search inventory.

Reproduction command actually run: `python3 /tmp/euclid_run.py`. Final exit code **0** means every scenario matched its expected outcome, including deliberate sanitizer failures. Compiler: `cc (Debian 14.2.0-19) 14.2.0`; Python 3.12.14. Build flags and include paths are fully in Appendix B. Both host build invocations exited 0 with empty build logs. Main flags: GNU C11, `-O1 -g`, `-fsanitize=address,undefined -fno-sanitize-recover=all`, `-fno-pie -no-pie`, function/data section GC, real STM32/FatFs/project headers. `ASAN_OPTIONS=detect_leaks=0`; each case is a fresh process with a 10-second timeout. No timeout occurred.

The fixture includes the entire original Euclidean generator and extracts **unmodified bodies** for ordinary length access, longest-track selection, backup/restore, flam/shift lookup, main-step predicate, note recording, normal body save/load, autosave wrappers and the SysEx receiver. HAL, voice output, routing/display and FatFs operations use narrow host stubs. Fatal arithmetic/bounds tests use actual compiler instrumentation, not just predicted arithmetic in Python. UI decrement is the exact small expression reproduced in the fixture; the full UI event loop is not linked. Only track 0 is unmuted. Loading/serialization use an in-memory byte stream, not an SD card.

The generator's `getObjSize(0)` uses a CMSIS macro expanding to a C builtin that is undefined for zero. To reach the separate length consumer defects reliably, the main fixture changes only the host seam for `__CLZ(0)` to return 32, matching the architectural ARM CLZ instruction. A second executable without that seam diagnoses the original builtin. This explicit distinction is essential: do not present the primary fixture as an entirely unmodified full firmware build.

| Scenario | Process exit | Exact relevant result |
|---|---:|---|
| `generation` | 0 | 2176 combinations: lengths 1…16, pulses 1…length, rotation inputs 0…15; requested pulse popcount retained; length16 always stored0 |
| `flam0` | 1, expected | `sequencer.c:1111:53: runtime error: division by zero` |
| `control15` | 0 | Literal15 playback arithmetic passed |
| `control16` | 0 | Literal16 playback arithmetic passed |
| `record0` | 1, expected | `sequencer.c:2402:32: runtime error: division by zero` |
| `record16` | 0 | Quantization from step15 wraps to step0 and records note60 |
| `prev255` | 1, expected | `sequencer.c:2038:38: runtime error: index 15 out of bounds for type 'uint16_t [4][64][7]'` |
| `ui-underflow` | 1, expected | `sequencer.c:2038:38: runtime error: index 4 out of bounds for type 'uint16_t [4][64][7]'` |
| `longest` | 0 | Raw0 vs8 selects8; host-normalized16 vs8 selects16 |
| `roundtrip` | 0 | Source normal writer→reader preserves zero; body3649, length offset3641 |
| `legacy-repair` | 0 | Source reader restores legacy0; **host-only** normalization to16 makes tested flam/record operations safe; re-save emits16 |
| `tmp-roundtrip` | 0 | Source autosave wrappers preserve zero; size3650, length offset3642 |
| `literal-roundtrips` | 0 | All 64 literal-length V3 body roundtrips (1…64) passed |
| `old-missing-tail` | 0 | Synthetic V1 accepted, all seven lengths become0 |
| `old-short-tail` | 0 | Synthetic V1 accepted, all seven lengths become5 after only one5 byte |
| `protocol-zero` | 0 | Actual compiled SysEx handler accepts raw0 |
| `protocol-overrun` | 1, expected | `CortexParser.c:282:23: runtime error: index 64 out of bounds for type 'uint8_t [64][7]'` |
| Separate `original-clz` build/run | 1, expected | `EuklidGenerator.c:55:21: runtime error: passing zero to clz(), which is not a valid argument` |

**Final totals:** 17 primary cases: 12 exit0 and five deliberate UBSan failures, all expectations matched. Plus one separate original-CLZ failure matched. The original-CLZ probe initially fails in `setup()` before its generation matrix can run. No unexpected sanitizer report was produced in the successful primary cases. The pulse-count test does not prove a particular canonical Euclidean rotation/spacing or every generator mathematical property.

Hashes of reproducibility inputs/results:

```text
/tmp/euclid_fixture.c
  SHA-256 e5c88820ec6c3c77360e6e8f7d92c49d8b7b18f2ae25942a78619a8b37bae50d
/tmp/euclid_run.py
  SHA-256 bff24f312d0087da20ec223ed6188641b32d7931c7f12d95be2dcb63acdcae4a
/tmp/euclid-investigation/results.json
  SHA-256 3b6beab7d4ef5f3dae14bc1938f451ad402d8ccd9d9f84e73402f644073f59cd
```

### 7.1 Read-only review commands and diagnostic errors

Commands included `git status --short --branch`, `git rev-parse HEAD HEAD^ refs/remotes/origin/main`, repository `rg` searches and numbered/sliced source reads, `git diff` between `5829398…` and HEAD, and `git diff --exit-code` / `git diff --cached --exit-code`. Initial and prepublication worktrees were clean on the same Bug 001 HEAD.

Native preprocessing used `cc -E -P -DSTM32F765xx -DUSE_HAL_DRIVER` with repository include paths. It succeeded for `CortexParser.c`, `TriggerOut.c`, `sequencer.c`, `menu.c`, `ledHandler.c`, `buttonHandler.c` and `presetManager.c`. It established compiled field accesses, one live getter refresh in sequencer, and no live `seq_triggerNextMasterStep()` caller in TriggerOut. This is a preprocessor activity, not an ARM firmware build. Required USB headers are in `Middlewares/JS/STM32_USB_Device_Library/...` and `USB_DEVICE_MIDI/{App,Target}`. Two exploratory sequencer preprocess attempts failed because the initial path set omitted `usbd_cdc.h`, then `usbd_conf.h`; adding the actual repository directories resolved both. No source modification was needed.

Other harmless investigation errors: one initial guessed `Soundengine/Serial/CortexParser.c` path did not exist (actual directory is `MIDI`); `git show lab-setup:AGENTS.md` failed because the local checkout has no local ref of that name, so the already-read remote branch content was used; a broad `/tmp` file inventory encountered a permission-denied private daemon directory and was narrowed. These did not affect test results. `command -v arm-none-eabi-gcc` did not resolve a compiler in the current shell PATH. No firmware build was requested or attempted for this investigation; earlier Bug 001 build results are not being re-certified here.

## 8. Smallest technically sound correction — recommendation only

No patch was implemented, including in production tests. The only repaired value in the experiment is a clearly marked local host fixture operation after loading.

### 8.1 Choose one canonical invariant

Recommended invariant: **in-memory track length is a literal 1…64; Euclidean generation produces literal 1…16. Legacy persisted/input zero decodes to 16 at supported ingestion boundaries.** With this invariant, existing ordinary consumers and raw copies no longer disagree, and serialized literal16 already fits the existing source format.

For ordinary valid UI plus complete existing zero-bearing files, the minimal functional change is:

1. Remove just the active `if(len==16) len=0;` conversion in `euklid_transferPattern()`.
2. In **both** active pattern loaders (`preset_readPatternData()` and `preset_LoadTmpPat()`), decode a successfully read length byte 0 as16 **before assigning it to the shared pattern**. Preserve1…64 unchanged. This is two production files total; no filter/DSP changes, no file-format change, no main playback scheduling change.

That two-file core solves the verified normal Euclidean path and complete legacy-file recurrence. **It is not a complete safety fix for all identified ingress paths:** raw setters and the compiled private SysEx receiver can reintroduce zero or >64, and short reads can reuse stale values. I do not recommend describing that reduced patch as enforcing a universal invariant.

### 8.2 Minimum boundary-complete correction scope

For a small correction that covers every active length ingestion path found, use the same canonical representation plus narrow guards in these four production files:

| File | Proposed limited work |
|---|---|
| `EuklidGenerator.c` | Remove16→0 transfer conversion; keep Euclidean inputs in1…16 and validate affected track/pattern destinations before array access. Do not change generated mask algorithm, pulse/rotation semantics or timing |
| `presetManager.c` | Both active loaders: decode present0→16, keep1…64, handle missing/invalid length reads deliberately before assigning; keep layout and tag unchanged. Do not broaden into unrelated kit/preset changes |
| `sequencer.c` | At `seq_setTrackLength()`, validate track and shown-pattern indices, normalize supported legacy0→16, reject values>64. Preserve ordinary1…64. Existing copies operate on canonical data; keep existing clock fallback as harmless defense |
| `CortexParser.c` | Length SysEx case: bound counter<448 before deriving/indexing, decode/validate length and indices before assignment, define safe end-of-stream behavior. Guarding only the setter cannot cover this direct write |

Use a small shared checked length-decoding helper if it reduces duplication without introducing a broad refactor. The exact error/default policy must be explicit. Missing optional V1 fields should initialize each track independently; current-format short reads should fail safely rather than silently turn an absent byte into a present legacy zero. If malformed-file handling cannot guarantee an entirely valid playable pattern after partial load, leave/recover a valid default length and report failure; broader transactional loading can remain a separate issue. No users' files need migration or deletion.

This is the smallest **identified boundary set**, not a claim that an unimplemented diff has the fewest possible lines. An alternative is a checked effective-length accessor used by **every** active raw arithmetic/ordering/UI consumer, preserving stored0; that avoids file writer changes but requires replacing every direct consumer, still needs index/invalid-input guards, and retains dual encodings. Merely restoring the getter fallback would leave playback, recording and longest comparison vulnerable; merely adding one flam guard would leave recording and UI wrong. A generic `% (length ? length :16)` at only one site is not sufficient.

### 8.3 Separate adjacent defects

The `__builtin_clz(0)` issue can be corrected independently with a defined zero guard in `getObjSize()` before calling the intrinsic. It does not explain the specific16→0 representation conversion and should not be silently bundled as a mathematical generator rewrite. If a future sanitizer regression builds the complete generator without a host CLZ seam, this zero handling must be accounted for explicitly. The ARM instruction behavior and compiler mapping need separate verification.

Likewise, legacy ×8 math in an unused external-step helper, general SysEx step-data format/bounds, UI decrement with unusually large increments, I/O transactionality and live regeneration synchronization merit separate work if reproduced. This investigation does not propose sweeping unrelated changes.

## 9. Independence from Bug 001

**Confirmed independent cause and separable implementation.** Bug 001 commit `7d9fe99…` changes only `presetManager.c`, `MidiParser.c` and five `tests/bug001` files. It skips reserved NRPN/bank slots in Morph/immediate kit sweeps and bounds MIDI bookkeeping. It does not change `EuklidGenerator.c`, `sequencer.c`, `CortexParser.c`, any length field, or pattern save/load implementation.

`git diff 5829398… HEAD -- Core` lists only `presetManager.c` and `MidiParser.c`. Inspection of the complete preset diff shows additions at the transport predicate/immediate sweep and Morph tick, not the pattern-I/O functions. The proposed length decoder/load changes in `presetManager.c` are separate functions and non-overlapping with those Bug 001 hunks. A future approved Euclidean correction can be based directly on `lab-setup` and then tested with and without the Bug 001 commit; there is no source requirement to alter, revert or depend on that fix. Sharing a file does require ordinary merge/diff review.

The private Cortex length protocol is not the CC0/CC2/NRPN path corrected by Bug 001. Its additional validation should not disable supported external MIDI/NRPN mute commands or modify Morph scheduling. No evidence links the length defect to filter Drive or the Bug 001 audible observations.

## 10. Proposed regression and validation plan — not executed as a correction suite

Before authorizing/implementing a patch, agree on the length decode, missing-field and malformed-file policies. Then add focused host tests that use actual source entry points, retaining instrumentation and independent processes for intentionally invalid old behavior.

1. Generation across all tracks/pattern destinations, lengths1…16, pulses1…length, rotations0…15: literal16 must remain16, pulse count/masks must preserve pre-fix behavior. Include invalid index/length entry checks; do not broaden valid Euclidean length beyond16 just because general tracks support64.
2. Playback and live recording for1,15,16,17,63,64 and legacy0 ingestion: no zero divisor, no index outside0…63, correct end-to-start note quantization. Exercise step0 and last step with ordinary/positive/negative shift and flam, muted/unmuted and stopped/running states.
3. Mixed-length master ordering: effective16 versus8; ties;1 versus64; legacy0 loaded on any voice. Full clock/pending-pattern simulation should confirm correct transition timing, beyond this report's direct selection assertion.
4. Last-step UI: loaded legacy16 displayed16; −1 becomes15, +1 becomes17; lower/upper bounds1 and64 stay safe; blink visible on correct bar. Test the actual menu entry point, not just an arithmetic seam.
5. Save/load both namedPatV03 and `tmp.pat`: legacy0→16 decode, literal1…64 unchanged, regenerated16 written16; masks, payloads, shuffle and kit untouched. Exact byte count/tag/order must remain unchanged. Copy/backup/restore should preserve invariant.
6. File robustness: actual V1/V2/V3 user-format fixtures plus synthetic complete/absent/short length tails, invalid>64, I/O errors at each relevant length byte, required kit absence; clear pass/fail/default behavior with no unsafe partially loaded lengths. Tests should assert actual read counts, not only output values.
7. Private protocol: zero,1,16,64,65,127; first/last legal destinations and counter448/oversized counters; supported normal front-panel selection remains functional. Document that these are internal SysEx tests, separate from external MIDI interoperability.
8. Run existing Bug 001 regression suite unchanged on the combined future branch (`python3 tests/bug001/run.py`). Test Euclidean fix independently on a lab-setup-based development branch too.
9. Build future Release and Debug using the provisioned ARM toolchain; validate image header/length/checksum/vector metadata. No such firmware builds were run in this investigation. Successful host checks/builds would still not prove audio or official-image equivalence.

Hardware follow-up remains **unrun and requires separate explicit approval, validated backup and recovery plan under AGENTS.md**. If later approved, use a copied project/card and matched source baseline versus candidate builds. First cover manual literal16 control versus Euclidean16,15,1; record notes across wrap, edit last step, check mixed lengths, save/reload/reboot and autosave restore. Keep clocks, tempo and voice settings fixed and log version/commit, masks/lengths if observable, playback continuity and note positions. Do not intentionally run known-invalid counter streams or malformed memory-access reproducers on hardware. Official v1.71 can be a separately identified behavioral reference only; a source-baseline/candidate A/B is needed to isolate this correction.

## 11. Remaining risks and unresolved questions

- No hardware behavior is reproduced. Host UBSan proves C defects, not the exact ARM exception, audible symptom, sequencing output or persistence behavior of official v1.71/private1.72.
- No official firmware/source correspondence is established. The public v1.71 “Euclidian sequencer fix” release note does not identify this source change.
- Real historical/user pattern fixtures were not provided. Source-format roundtrips and synthetic older-body cases do not establish every historical layout or official file compatibility.
- Existing unchecked protocol selection/index paths can introduce invalid destinations; proposed guards are limited to affected length ingress. Other protocol handlers are not fully audited.
- FatFs stubs model successful reads/writes and EOF only. No real SD, filesystem errors, interruption/power loss, concurrency or atomic save behavior was tested.
- Live UI generation writes main-step masks and then the length while real-time playback may be running; there is no transaction tested here. Removing zero addresses the representation defect but does not prove synchronization of all edits or safe behavior when shortening a track while its current index is beyond its new end.
- An isolated CLZ seam was required for host testing; its original undefined builtin input remains separate source debt. A future implementation must make its sanitizer strategy explicit.
- No production patch, future regression suite, release/debug firmware build, image validation or hardware A/B was performed for this task. Proposed code changes need review/authorization before implementation.

Recommended next action: review the canonical representation and file-policy recommendation, then authorize a small, separate Euclidean correction branch. Keep the already-published Bug 001 branch unchanged.

## 12. Documentation publication and audit trail

Only this Markdown report is authorized for GitHub publication. The planned report commit has exactly one parent, `5829398cea5d337200393757e39a37d4d8148256`, and exactly one added file at the path above. The new branch is created without modifying local checkout branches. GitHub's authenticated branch/file API creates and commits the report directly; no force-push, binary upload, PR or merge is involved. The publication process verifies exact file readback, commit parent and changed-file list, branch head, and that all pre-existing remote branches retain their initial SHAs. The report does not assert its own as-yet-unknown commit hash; the verified final user response identifies publication.

Remote heads recorded before publication: main `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`; lab-setup `5829398cea5d337200393757e39a37d4d8148256`; Bug001 `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`; earlier report branches end `9028075917b2bc1a8df43dfb2c6e0d2dab66595d`, `8ee08ec28fc74d95db02d3fe0ddaa649db018937`, and `710546c6cc4fea480b490816962ae9626d60b368`. No prior report is overwritten.

## Appendix A. Complete temporary host fixture

Recreate this text as `/tmp/euclid_fixture.c`. It reads source from the checkout and does not write inside it. Copying this fixture into a production firmware build is not proposed.

```c
// Temporary investigation fixture. Production files are read, never changed.
#include <assert.h>
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

// Model ARM CLZ's defined zero result to isolate the length defect. A separate
// build without this shim probes the original builtin under host UBSan.
#ifndef ORIGINAL_CLZ
#undef __CLZ
#define __CLZ(x) ((uint32_t)(x) ? __builtin_clz((uint32_t)(x)) : 32)
#endif
#include "Core/Src/Soundengine/Sequencer/EuklidGenerator.c"

PatternSet seq_patternSet;
PatternBackupBuffer patternBackupBuffer;
uint8_t seq_activePattern, seq_pendingPattern, seq_running, seq_recordActive;
static uint8_t seq_mutedTracks = 0x7e;
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
TIM_HandleTypeDef htim2;
FIL SDFile;
#define STEPS_PER_PATTERN NUM_STEPS

static uint8_t stream[65536];
static size_t stream_pos, stream_size;
FRESULT f_open(FIL *file, const TCHAR *name, BYTE flags) {
    (void)file; (void)name; stream_pos=0;
    if(flags & FA_CREATE_ALWAYS) stream_size=0;
    return FR_OK;
}
FRESULT f_close(FIL *file) { (void)file; return FR_OK; }
FRESULT f_read(FIL *file, void *dst, UINT count, UINT *actual) {
    (void)file;
    size_t n=count < stream_size-stream_pos ? count : stream_size-stream_pos;
    memcpy(dst, stream+stream_pos, n); stream_pos+=n; *actual=n; return FR_OK;
}
FRESULT f_write(FIL *file, const void *src, UINT count, UINT *actual) {
    (void)file; assert(stream_pos+count <= sizeof(stream));
    memcpy(stream+stream_pos, src, count); stream_pos+=count;
    if(stream_pos>stream_size) stream_size=stream_pos;
    *actual=count; return FR_OK;
}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *timer) { (void)timer; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *timer) { (void)timer; return HAL_OK; }
void menu_repaintAll(void) {}
void seq_sendStepInfoToFront(uint16_t step) { (void)step; }
void frontPanel_sendData(uint8_t a,uint8_t b,uint8_t c) { (void)a;(void)b;(void)c; }
void seq_triggerVoice(uint8_t voice,uint8_t vel,uint8_t note) { (void)voice;(void)vel;(void)note; }
void voiceControl_noteOff(uint8_t voice) { (void)voice; }
float seq_getElapsedStepTimePercent(void) { return 75.f; }
uint8_t seq_determineNextPattern(uint8_t change) { (void)change; abort(); }
void led_setValue(uint8_t value,uint8_t led) { (void)value;(void)led; }
void seq_setMainStep(uint8_t pattern,uint8_t voice,uint8_t step,uint8_t on) {
    assert(step<NUM_STEPS);
    if(on) seq_patternSet.seq_mainSteps[step/16][pattern][voice] |= (1u<<(step%16));
}

#include "functions.inc"

static void setup(unsigned length) {
    memset(&seq_patternSet,0,sizeof(seq_patternSet));
    for(unsigned t=0;t<NUM_TRACKS;t++) seq_patternSet.length[0][t]=1;
    euklid_init(); euklid_setLength(0,length); euklid_setSteps(0,4,0);
    seq_running=1; seq_recordActive=1; seq_stepIndex[0]=0;
    menu_activePage=PERF_PAGE;
}
static void canonicalize_existing_lengths(void) {
    // Hypothetical load-boundary repair, confined to the host fixture.
    for(unsigned t=0;t<NUM_TRACKS;t++)
        if(seq_patternSet.length[0][t]==0) seq_patternSet.length[0][t]=16;
}
int main(int argc,char **argv) {
    assert(argc==2); const char *mode=argv[1];
    setup(strcmp(mode,"control15")==0 ? 15 : 16);
    if(strcmp(mode,"generation")==0) {
        unsigned count=0;
        for(unsigned length=1;length<=16;length++)
            for(unsigned steps=1;steps<=length;steps++)
                for(unsigned rot=0;rot<16;rot++) {
                    euklid_setLength(0,length); euklid_setSteps(0,steps,0); euklid_setRotation(0,rot,0);
                    assert(seq_patternSet.length[0][0]==(length==16?0:length));
                    assert(__builtin_popcountll(euklid_patternBuffer.quadword)==steps);
                    count++;
                }
        printf("PASS generation: %u cases; length 16 consistently stored as 0\n",count); return 0;
    }
    if(strcmp(mode,"flam0")==0 || strcmp(mode,"control15")==0 || strcmp(mode,"control16")==0) {
        if(strcmp(mode,"control16")==0) seq_patternSet.length[0][0]=16;
        seq_processFlam(0); puts("PASS playback control"); return 0;
    }
    if(strcmp(mode,"record0")==0 || strcmp(mode,"record16")==0) {
        if(strcmp(mode,"record16")==0) seq_patternSet.length[0][0]=16;
        frontParser_shownPattern=1; seq_stepIndex[0]=15;
        seq_addNote(0,100,60);
        assert(seq_patternSet.seq_subStepPattern[0][0][0].note==60);
        puts("PASS record control"); return 0;
    }
    if(strcmp(mode,"prev255")==0) {
        // Isolate downstream risk; do not claim this bypasses the earlier UB on hardware.
        uint8_t prev=seq_patternSet.length[0][0]-1;
        assert(prev==255); seq_getShiftFlamPattern(0,prev,STEP_PREV); abort();
    }
    if(strcmp(mode,"ui-underflow")==0) {
        uint8_t length=seq_getTrackLength(0); int8_t inc=-1;
        if(length!=1) length+=inc; // exact menu.c last-step decrement arithmetic
        assert(length==255); seq_setTrackLength(0,length); seq_stepIndex[0]=63;
        seq_processFlam(0); abort();
    }
    if(strcmp(mode,"longest")==0) {
        seq_patternSet.length[0][1]=8;
        assert(seq_getTrackLength(0)==0 && seq_getLongestTrack()==1);
        canonicalize_existing_lengths();
        assert(seq_getTrackLength(0)==16 && seq_getLongestTrack()==0);
        puts("PASS: legacy zero hides effective 16 from getter and longest-track comparison"); return 0;
    }
    if(strcmp(mode,"roundtrip")==0 || strcmp(mode,"legacy-repair")==0) {
        const size_t length_offset=NUM_TRACKS*NUM_STEPS*(sizeof(Step)+1)+NUM_TRACKS*NUM_BARS*2+1;
        preset_writePatternData(0);
        assert(stream[length_offset]==0 && stream_size==length_offset+NUM_TRACKS+1);
        memset(&seq_patternSet,0,sizeof(seq_patternSet)); stream_pos=0;
        assert(preset_readPatternData(0,3)); assert(seq_patternSet.length[0][0]==0);
        if(strcmp(mode,"legacy-repair")==0) {
            canonicalize_existing_lengths();
            seq_processFlam(0); frontParser_shownPattern=1; seq_stepIndex[0]=15;
            seq_addNote(0,100,60); assert(seq_patternSet.seq_subStepPattern[0][0][0].note==60);
            stream_pos=0; preset_writePatternData(0); assert(stream[length_offset]==16);
        }
        printf("PASS normal body roundtrip: length offset %zu, body bytes %zu\n",length_offset,stream_size);
        return 0;
    }
    if(strcmp(mode,"tmp-roundtrip")==0) {
        assert(preset_saveTmpPat()==FR_OK);
        size_t offset=1+NUM_TRACKS*NUM_STEPS*(sizeof(Step)+1)+NUM_TRACKS*NUM_BARS*2+1;
        assert(stream[offset]==0); memset(&seq_patternSet,0,sizeof(seq_patternSet));
        assert(preset_LoadTmpPat()==FR_OK); assert(seq_patternSet.length[0][0]==0);
        printf("PASS autosave roundtrip: length offset %zu, file bytes %zu\n",offset,stream_size); return 0;
    }
    if(strcmp(mode,"literal-roundtrips")==0) {
        for(unsigned length=1;length<=64;length++) {
            seq_patternSet.length[0][0]=length; stream_pos=stream_size=0;
            preset_writePatternData(0); seq_patternSet.length[0][0]=0; stream_pos=0;
            assert(preset_readPatternData(0,3)); assert(seq_patternSet.length[0][0]==length);
        }
        puts("PASS 64 literal-length V3 body roundtrips (1..64)"); return 0;
    }
    if(strcmp(mode,"old-missing-tail")==0 || strcmp(mode,"old-short-tail")==0) {
        // Synthetic V1 body: no V3 timing block and no V2 associated-kit byte.
        stream_size=NUM_TRACKS*NUM_STEPS*sizeof(Step)+NUM_TRACKS*NUM_BARS*2+1;
        memset(stream,0,sizeof(stream));
        if(strcmp(mode,"old-short-tail")==0) stream[stream_size++]=5;
        stream_pos=0; assert(preset_readPatternData(0,1));
        uint8_t expected=strcmp(mode,"old-short-tail")==0 ? 5 : 0;
        for(unsigned t=0;t<NUM_TRACKS;t++) assert(seq_patternSet.length[0][t]==expected);
        printf("PASS synthetic V1 tail: all seven lengths become %u\n",expected); return 0;
    }
    if(strcmp(mode,"protocol-zero")==0 || strcmp(mode,"protocol-overrun")==0) {
        frontParser_sysexActive=SYSEX_RECEIVE_PAT_LEN_DATA;
        frontParser_sysexSeqStepNr=strcmp(mode,"protocol-overrun")==0 ? NUM_PATTERN*NUM_TRACKS : 0;
        frontParser_handleSysexData(0);
        assert(seq_patternSet.length[0][0]==0);
        puts("PASS internal SysEx receiver accepts raw zero length"); return 0;
    }
    abort();
}
```

## Appendix B. Complete temporary extractor and test runner

Recreate as `/tmp/euclid_run.py`, adjust `ROOT` if necessary, and run `python3 /tmp/euclid_run.py`. Use the exact investigated source commit. The extracted function include is generated from that source, so it need not be manually reconstructed. The initial `setup()` generates16 before each selected scenario (15 for its control). The separate original-CLZ probe is expected to stop in that setup.

```python
import json,os,re,subprocess
from pathlib import Path
ROOT=Path('/workspace/lxr-desktop-app')
TEMP=Path('/tmp/euclid-investigation'); TEMP.mkdir(exist_ok=True)

def extract(path,name):
    source=(ROOT/path).read_text(errors='replace')
    masked=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),source,flags=re.S)
    match=re.search(r'^(?:IN_ITCM_RAM\s+)?(?:static\s+)?(?:inline\s+)?(?:void|float|FRESULT|uint\d+_t|int\d+_t)\s+'+name+r'\([^;]*?\)\s*\{',masked,re.M)
    assert match, name
    depth=1; end=match.end()
    while depth:
        depth+=(masked[end]=='{')-(masked[end]=='}'); end+=1
    line=source.count('\n',0,match.start())+1
    return f'#line {line} "{path}"\n'+source[match.start():end]+'\n'

seq='Core/Src/Soundengine/Sequencer/sequencer.c'
preset='Core/Src/Preset/presetManager.c'
names=['seq_setTrackLength','seq_getTrackLength','seq_getLongestTrack','seq_copyPatternToBackupBuffer','seq_copyBackupBufferToPattern','getShiftValue','flamIdxToPattern','seq_intIsMainStepActive','seq_getShiftFlamPattern','seq_processFlam','seq_addNote']
body='\n'.join(extract(seq,name) for name in names)
body+='\n'.join(extract(preset,name) for name in ['preset_writePatternData','preset_readPatternData','preset_saveTmpPat','preset_LoadTmpPat'])
body+=extract('Core/Src/Soundengine/MIDI/CortexParser.c','frontParser_handleSysexData')
(TEMP/'functions.inc').write_text(body)
includes=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc','Middlewares/Third_Party/FatFs/src','FATFS/App','FATFS/Target']
flags=['cc','-std=gnu11','-O1','-g','-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-implicit-function-declaration','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-ffunction-sections','-fdata-sections','-fno-pie','-no-pie','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
flags+=['-I'+str(ROOT/p) for p in includes]+['-I'+str(TEMP)]
env=os.environ.copy(); env['ASAN_OPTIONS']='detect_leaks=0'
build=subprocess.run(flags+['/tmp/euclid_fixture.c','-Wl,--gc-sections','-o',str(TEMP/'probe')],cwd=ROOT,env=env,capture_output=True,text=True)
(TEMP/'build.log').write_text(build.stdout+build.stderr)
if build.returncode:
    print(build.stderr); raise SystemExit(build.returncode)
cases=['generation','flam0','control15','control16','record0','record16','prev255','ui-underflow','longest','roundtrip','legacy-repair','tmp-roundtrip','literal-roundtrips','old-missing-tail','old-short-tail','protocol-zero','protocol-overrun']
expected_fail={'flam0':'division by zero','record0':'division by zero','prev255':'out of bounds','ui-underflow':'out of bounds','protocol-overrun':'out of bounds'}
results=[]
for case in cases:
    run=subprocess.run([str(TEMP/'probe'),case],cwd=ROOT,env=env,capture_output=True,text=True,timeout=10)
    output=run.stdout+run.stderr
    ok=(run.returncode!=0 and expected_fail[case] in output) if case in expected_fail else run.returncode==0
    (TEMP/(case+'.log')).write_text(output)
    results.append(dict(case=case,returncode=run.returncode,expected_failure=case in expected_fail,matched_expectation=ok,output=output))
    print(json.dumps(results[-1]))
(TEMP/'results.json').write_text(json.dumps(results,indent=2)+'\n')
if not all(r['matched_expectation'] for r in results):raise SystemExit(1)
original=subprocess.run(flags+['-DORIGINAL_CLZ','/tmp/euclid_fixture.c','-Wl,--gc-sections','-o',str(TEMP/'probe-original-clz')],cwd=ROOT,env=env,capture_output=True,text=True)
(TEMP/'build-original-clz.log').write_text(original.stdout+original.stderr)
assert original.returncode==0, original.stderr
run=subprocess.run([str(TEMP/'probe-original-clz'),'generation'],cwd=ROOT,env=env,capture_output=True,text=True,timeout=10)
(TEMP/'original-clz.log').write_text(run.stdout+run.stderr)
print(json.dumps(dict(case='original-clz',returncode=run.returncode,output=run.stdout+run.stderr)))
assert run.returncode!=0 and 'clz()' in run.stderr
```

## Appendix C. Exact primary test-result JSON

Sanitizer paths are production paths through `#line` directives; they identify the extracted original bodies. The separate original-CLZ diagnostic, outside this JSON, was:

```text
/workspace/lxr-desktop-app/Core/Src/Soundengine/Sequencer/EuklidGenerator.c:55:21: runtime error: passing zero to clz(), which is not a valid argument
```

```json
[
  {
    "case": "generation",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS generation: 2176 cases; length 16 consistently stored as 0\n"
  },
  {
    "case": "flam0",
    "returncode": 1,
    "expected_failure": true,
    "matched_expectation": true,
    "output": "Core/Src/Soundengine/Sequencer/sequencer.c:1111:53: runtime error: division by zero\n"
  },
  {
    "case": "control15",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS playback control\n"
  },
  {
    "case": "control16",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS playback control\n"
  },
  {
    "case": "record0",
    "returncode": 1,
    "expected_failure": true,
    "matched_expectation": true,
    "output": "Core/Src/Soundengine/Sequencer/sequencer.c:2402:32: runtime error: division by zero\n"
  },
  {
    "case": "record16",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS record control\n"
  },
  {
    "case": "prev255",
    "returncode": 1,
    "expected_failure": true,
    "matched_expectation": true,
    "output": "Core/Src/Soundengine/Sequencer/sequencer.c:2038:38: runtime error: index 15 out of bounds for type 'uint16_t [4][64][7]'\n"
  },
  {
    "case": "ui-underflow",
    "returncode": 1,
    "expected_failure": true,
    "matched_expectation": true,
    "output": "Core/Src/Soundengine/Sequencer/sequencer.c:2038:38: runtime error: index 4 out of bounds for type 'uint16_t [4][64][7]'\n"
  },
  {
    "case": "longest",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS: legacy zero hides effective 16 from getter and longest-track comparison\n"
  },
  {
    "case": "roundtrip",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS normal body roundtrip: length offset 3641, body bytes 3649\n"
  },
  {
    "case": "legacy-repair",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS normal body roundtrip: length offset 3641, body bytes 3649\n"
  },
  {
    "case": "tmp-roundtrip",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS autosave roundtrip: length offset 3642, file bytes 3650\n"
  },
  {
    "case": "literal-roundtrips",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS 64 literal-length V3 body roundtrips (1..64)\n"
  },
  {
    "case": "old-missing-tail",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS synthetic V1 tail: all seven lengths become 0\n"
  },
  {
    "case": "old-short-tail",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS synthetic V1 tail: all seven lengths become 5\n"
  },
  {
    "case": "protocol-zero",
    "returncode": 0,
    "expected_failure": false,
    "matched_expectation": true,
    "output": "PASS internal SysEx receiver accepts raw zero length\n"
  },
  {
    "case": "protocol-overrun",
    "returncode": 1,
    "expected_failure": true,
    "matched_expectation": true,
    "output": "Core/Src/Soundengine/MIDI/CortexParser.c:282:23: runtime error: index 64 out of bounds for type 'uint8_t [64][7]'\n"
  }
]
```

## Appendix D. Decisive original source excerpts

These are unchanged production excerpts from the investigated commit. They are evidence, not an applied patch.

### `Core/Src/Soundengine/Sequencer/EuklidGenerator.c:221–243`

```c
void euklid_transferPattern(uint8_t trackNr, uint8_t patternNr)
{
	uint8_t len=euklid_length[trackNr];
	for(int i=0; i<NUM_BARS; i++)
	{
#if USE_PATTERN_EDIT_BUFFER
		patternBackupBuffer.seq_mainSteps[i][trackNr] = euklid_patternBuffer.word[i];
#else
		seq_patternSet.seq_mainSteps[i][patternNr][trackNr] = euklid_patternBuffer.word[i];
#endif

	}
// **PATROT - pattern end is now stored differently

	if(len == 16)
		len=0;
#if USE_PATTERN_EDIT_BUFFER
	patternBackupBuffer.length[trackNr]=len;
#else
	seq_patternSet.length[patternNr][trackNr]=len;
#endif

}
```

### `Core/Src/Soundengine/Sequencer/sequencer.c:274–305`

```c
void seq_setTrackLength(uint8_t trackNr, uint8_t length)
{
	//set new end marker
	/*
	if(length == 16)
		length=0;
	*/
	// **PATROT this was changed from setting length on seq_activePattern to shown (or edited) pattern

#if USE_PATTERN_EDIT_BUFFER

	patternBackupBuffer.length[trackNr]=length;

#else
	seq_patternSet.length[frontParser_shownPattern][trackNr]=length;
#endif
}

//------------------------------------------------------------------------------
uint8_t seq_getTrackLength(uint8_t trackNr)
{
	// **PATROT this was changed from getting seq_activePattern to shown (or edited) pattern
#if USE_PATTERN_EDIT_BUFFER
	uint8_t r=patternBackupBuffer.length[trackNr];
#else
	uint8_t r=seq_patternSet.length[frontParser_shownPattern][trackNr];
#endif
	/*
	if(r==0)
		return 16;
	*/
	return r;
```

### `Core/Src/Soundengine/Sequencer/sequencer.c:831–843`

```c
uint8_t seq_getLongestTrack()
{
	uint8_t trackId = 0;

	for(int i=1;i<NUM_TRACKS;i++)
	{
		if(seq_patternSet.length[seq_activePattern][i] > seq_patternSet.length[seq_activePattern][trackId])
		{
			trackId = i;
		}
	}
	return trackId;
}
```

### `Core/Src/Soundengine/Sequencer/sequencer.c:1103–1125`

```c
	{
		//if track is not muted
		if(!(seq_mutedTracks & (1<<track) ) )
		{
			//pattern takes main step active and probability into account
#if 1
			const uint8_t length = seq_patternSet.length[seq_activePattern][track];
			const uint8_t currentStep = seq_stepIndex[track];
			const uint8_t nextStep = (seq_stepIndex[track]+1)%length;
			const uint8_t prevStep = seq_stepIndex[track] != 0 ? seq_stepIndex[track] - 1 : length-1;

			/* we have 3 possible step sources:
			 * - flammed AND positive shifted notes from the previous step
			 * - current step data
			 * - negative shifted data from the next step
			 */
			uint32_t currentStepPattern = seq_getShiftFlamPattern(track, currentStep, STEP_CURRENT);
			uint32_t nextStepPattern 	= seq_getShiftFlamPattern(track, nextStep, STEP_NEXT);
			uint32_t prevStepPattern 	= seq_getShiftFlamPattern(track, prevStep, STEP_PREV);

			//calc priotity (next > current > prev)
			uint8_t prevActive 		= (prevStepPattern 		& (1UL<<(23-idx)) ) != 0;
			uint8_t currentActive 	= (currentStepPattern 	& (1UL<<(23-idx)) ) != 0;
```

### `Core/Src/Soundengine/Sequencer/sequencer.c:2034–2039`

```c
static uint8_t seq_intIsMainStepActive(uint8_t voice, uint8_t mainStepNr, uint8_t pattern)
{
	const uint8_t bar = mainStepNr / 16;
	const uint8_t shift = mainStepNr - bar*16;
	return (seq_patternSet.seq_mainSteps[bar][pattern][voice] & (1<<shift)) > 0;
}
```

### `Core/Src/Soundengine/Sequencer/sequencer.c:2401–2417`

```c
		//limit to track length
		quantizedStep = quantizedStep%seq_patternSet.length[targetPattern][trackNr];
		if(step != quantizedStep)
		{
			step = quantizedStep;
			seq_stepToSkip[trackNr] = step;
		}

		//set the current step in the requested track active
		stepPtr=&seq_patternSet.seq_subStepPattern[targetPattern][trackNr][step];
		stepPtr->note 		= note;				// note (was SEQ_DEFAULT_NOTE)
		stepPtr->volume		= vel;				// new velocity
		stepPtr->prob		= 100;				// 100% probability
		stepPtr->volume 	|= STEP_ACTIVE_MASK;

		//activate corresponding main step
		seq_setMainStep(targetPattern, trackNr, step,1);
```

### `Core/Src/menu/menu.c:2770–2793`

```c
	{
		uint8_t mode = buttonHandler_getMode();
		uint8_t l = seq_getTrackLength(menu_activeVoice);
		if(inc < 0)
		{
			if(l!=1)
				l += inc;
		}
		else
		{
			l += inc;
			if(l > 64) l = 64;
		}

		if(mode == SELECT_MODE_LAST_STEP_TRACK)
		{
			seq_setTrackLength(menu_activeVoice, l);
		}
		else if (mode == SELECT_MODE_LAST_STEP_ALL)
		{
			for(int i=0;i<7;i++)
			{
				seq_setTrackLength(i, l);
			}
```

### `Core/Src/Preset/presetManager.c:1960–1979`

```c
	}

	// ----------- Pattern/track lengths
	// -- AS this might not exist in the saved data file, we still want success
	if(success) {

			for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
			{
				f_read((FIL*)&SDFile,(void*)&next,sizeof(uint8_t),&bytesRead);
				seq_patternSet.length[patternNr][trk] = next;
			}

		success=1; // just to document that we don't consider this a failure, we just didn't read the bytes for this
	}

	if(dataVersion > 1)
	{
		f_read((FIL*)&SDFile,(void*)&seq_patternSet.kitNr[patternNr],sizeof(uint8_t),&bytesRead);
		if(bytesRead == 0) success = 0;

```

### `Core/Src/Preset/presetManager.c:435–453`

```c

		// ----------- Pattern/track lengths
		// -- AS this might not exist in the saved data file, we still want success
		if(success) {

				for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
				{
					f_read((FIL*)&SDFile,(void*)&next,sizeof(uint8_t),&bytesRead);
					seq_patternSet.length[patternNr][trk] = next;
				}

			success=1; // just to document that we don't consider this a failure, we just didn't read the bytes for this
		}

		//if(dataVersion > 1)
		{
			f_read((FIL*)&SDFile,(void*)&seq_patternSet.kitNr[patternNr],sizeof(uint8_t),&bytesRead);
			if(bytesRead == 0) success = 0;

```

### `Core/Src/Soundengine/MIDI/CortexParser.c:255–288`

```c
	case SYSEX_RECEIVE_PAT_LEN_DATA:
		// same as above but we are receiving length data for each pattern
		{
			//calculate the step pattern and track indices
			const uint8_t currentPattern	= frontParser_sysexSeqStepNr / 7;
			const uint8_t currentTrack  	= frontParser_sysexSeqStepNr - currentPattern*7;


/*
			if( (currentPattern == seq_activePattern) && seq_isRunning() )
			{
				seq_tmpPattern.length[currentTrack] = data;
			} else {
				*/

#if USE_PATTERN_EDIT_BUFFER
			if( (currentPattern == seq_activePattern) )
			{
				patternBackupBuffer.length[currentTrack]=data;
			}
			else
			{
				seq_patterns.length[currentPattern][currentTrack] = data;
			}
#else
				//first load into inactive track
				PatternSet* patternSet = &seq_patternSet;
				patternSet->length[currentPattern][currentTrack] = data;
#endif
	//		}


			//inc the step counter
			frontParser_sysexSeqStepNr++;
```

### `Core/Src/Soundengine/Sequencer/EuklidGenerator.c:53–57`

```c
static inline uint8_t getObjSize(uint16_t group)
{
	uint8_t ret = 32 - __CLZ((uint32_t)group);
	if(ret==0) return 1;
	return ret;
```
