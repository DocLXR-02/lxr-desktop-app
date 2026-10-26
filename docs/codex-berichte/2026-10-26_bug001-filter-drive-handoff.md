# LXR Firmware Lab — Bug 001: Filter Drive clicks during kit morphing

Date: 2026-10-26 (UTC)

Repository: https://github.com/DocLXR-02/lxr-desktop-app

Report branch: `codex-reports/2026-10-26-bug001-handoff`

Report path: `docs/codex-berichte/2026-10-26_bug001-filter-drive-handoff.md`

## 1. Handoff status and authorization

The source investigation and the requested minimal corrective implementation are complete. The corrective firmware patch is **local and uncommitted** on `bug001/minimal-correction`. Both firmware configurations build successfully. Targeted host regressions pass. Hardware/audio confirmation has not been performed.

This report is the only file authorized for publication in this handoff step. Its branch is based on the existing remote `lab-setup` commit, not on the Bug 001 development branch. The report embeds the complete production diff and all five regression-test files so another assistant can reconstruct and review the work without this conversation or the local workspace. Embedded code is documentation; this report does not install that patch into the source tree on its report branch.

The user first authorized source investigation only, then explicitly authorized implementation on an isolated development branch and both firmware builds. They excluded changes to filter algorithms, Drive curves, smoothing and Morph scheduling, and prohibited flashing, pushing firmware changes, PR creation and merging without approval. The latest instruction authorizes committing/publishing **this report** on `lab-setup` or a branch based on it. It does not authorize publishing the firmware patch.

No firmware was flashed. No patch commit, PR, merge, force-push, credential inspection or unrelated project change was made. No source/test file is included as an active repository change in the report publication. No secrets are included.

### Repository and environment identities

| Item | Value |
| --- | --- |
| Investigated source and local patch base | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Base commit subject | GCC 15 adds thread-jumps by default. Added -fno-thread-jumps because otherwise ITCM overflows. |
| Original checkout branch | `work` |
| Local implementation branch | `bug001/minimal-correction` |
| Remote `main`, read again for handoff | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Remote `lab-setup`, report branch base | `56593cacc789e4367f7bb7a63ac2511bfdcd1195` |
| `lab-setup` parent | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Additional `lab-setup` commit | Add GitHub Actions workflow for LXR firmware build |
| Local repository | `/workspace/lxr-desktop-app` |
| Local build toolchain | `/workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc` |
| Compiler version used | Arm GNU 14.2.1 20241119, package banner `(15:14.2.rel1-1)` |
| CMake/Ninja location | `/workspace/.onboarding/python/bin` |

The remote `lab-setup` tree was inspected recursively before publication: no `AGENTS.md` instructions or existing `docs/codex-berichte/` reports were found. Its added workflow is `.github/workflows/lxr-build.yml`. The report branch name was absent from the branch list. The contents API's create-only operation is used to avoid overwriting reports. The original local checkout is retained on the Bug 001 branch; publishing the report through the GitHub connector does not switch or commit that checkout.

**Version limitation:** `CMakeLists.txt:23` declares firmware version `1.7`. The hardware observation concerns installed v1.72. Exact source/binary correspondence to v1.72 is not established. Version metadata was not changed by the patch.

## 2. Hardware observations supplied by the user

These are user observations, not measurements independently repeated by this agent:

- LXR-02 firmware v1.72, BassKick on Voice 1.
- Morphing between identical kits reproduces the clicks.
- An empty project without LFOs or automation reproduces them.
- Virtually every small Morph-knob movement clicks throughout 0–255.
- Continued physical movement at either endpoint also clicks.
- A stationary Morph knob does not click.
- All filter types are similarly affected; filter TYPE OFF suppresses clicks.
- Intensity increases with Filter Drive.
- During movement, the kick temporarily appears to lose Drive and approach Drive=0.
- The Drive=0 endpoint is quiet; Drive=127 produces severe disturbances.
- Earlier comparisons suggested Voices 2/3 were unaffected.

No BassKick kit dump, reserved-slot byte values, runtime NRPN-selector trace, timing trace, audio capture or installed firmware binary was provided.

## 3. Source navigation and complete control/audio path

Unless marked “patched,” line numbers below refer to the original `a43cd4f` source. Immutable source URL prefix:

`https://github.com/DocLXR-02/lxr-desktop-app/blob/a43cd4fcda23ec3d577dc425261c3eb28bd5abff/`

| Stage | Source and lines | Behavior |
| --- | --- | --- |
| Pot polling | `Core/Src/main.c:950–976` | The main loop interleaves ADC/menu input and sound-engine processing. |
| Physical movement | `Core/Src/IO/adcPots.c:174–245`, particularly 232 | ADC hysteresis/position changes produce relative `+1` or `-1` menu events. |
| Menu resolution | `Core/Src/menu/menu.c:3917–3951` | Resolves the knob's parameter from the active page and calls `limitPotValue()`. |
| Endpoint clamping | `Core/Src/menu/menu.c:3704–3773` | Clamps arithmetic to 0–255; Morph uses `DTYPE_0B255` (`menu.c:513`). |
| Store/dispatch | `Core/Src/menu/menu.c:4121–4128`, `4236–4242` | Stores the value in `activeParameters`; globals go to `menu_parseGlobalParam()`. |
| Morph command | `Core/Src/menu/menu.c:3512–3516` | Calls `preset_morph(value)`. |
| Morph start | `Core/Src/Preset/presetManager.c:2201–2206` | Sets `morph_state=1`, `morph_index=0` unconditionally. The enabled implementation does not use its `morph` argument. |
| Morph service | `Core/Src/Soundengine/Soundengine.c:77–114` | Calls `preset_morph_tick()` once per `soundengine_process()`. |
| Interpolation and sweep | `Core/Src/Preset/presetManager.c:2128–2194` | Processes one slot per tick, using the current `activeParameters[PAR_MORPH]`; routing slots use source values. |
| Internal MIDI dispatch | `Core/Src/Preset/presetManager.c:2154–2183` | Calls `midiParser_ccHandler()` directly; no external MIDI, LFO or automation is required. |
| Parameter interpretation | `Core/Src/Soundengine/MIDI/MidiParser.c:144–203`, `906–1439` | CC and CC2 commands update parameters, caches and modulation notifications. |
| Drive update | `Core/Src/Soundengine/MIDI/MidiParser.c:1096–1125` | Drive commands call `SVF_setDrive()` in the enabled configuration. |
| Drive transfer | `Core/Src/Soundengine/DSPAudio/ResonantFilter.c:89–98` | Maps normalized Drive to `0.4 + 6*d*d`. |
| Audio dispatch | `Core/Src/Soundengine/DSPAudio/mixer.c:548–602` | Restores/dispatches modulation, recalculates filter frequencies, renders voices. |
| Voice processing | `Core/Src/Soundengine/DSPAudio/DrumVoice.c:265–295` | Oscillator/transient mixture → filter → amplitude envelope → separate voice distortion. |
| Filter DSP | `Core/Src/Soundengine/DSPAudio/ResonantFilter.c:157–225` | Filter OFF returns before Drive processing; enabled filter paths multiply input by `filter->drive` before saturation. |

`activeParameters` is the source/edit buffer; `morphParameters` points into a kit slot (`menu.c:694–698`, `presetManager.c:834–844`). The global Morph value lies outside sound slots: the active enum has `END_OF_SOUND_PARAMETERS=247`, `PAR_MORPH=248`. The sweep covers indices 0–246. Some source comments and disabled duplicate enums have obsolete counts (e.g. 228); use the enabled `Core/Inc/Parameters.h` enumeration, included by `Core/Inc/Soundengine/MIDI/ParameterArray.h:27–29`.

Ordinary sound-parameter knob edits also use `frontPanel_sendData()` (`Core/Src/frontPanelParser.c:617–633`) and the Cortex parser (`Core/Src/Soundengine/MIDI/CortexParser.c:430–460`). That path applies the same low-CC offset. External MIDI enters through `midiParser_parseMidiMessage()` and ultimately uses the same CC handler, while forwarding parameter/bank messages to the UI (`MidiParser.c:1877–1910`). Morph bypasses that outer MIDI routing/UI-forwarding entry point.

The immediate kit-load path is `preset_sendDrumsetParametersImmediate()` (`presetManager.c:1263–1346`): it initializes modulation destinations, then sends all entries in `parameterOrder`, calling `soundengine_process()` between entries. The deferred kit-load path `preset_sendDrumsetParameters()` (`1349–1379`) starts the same Morph sweep. Both must exclude transport slots.

## 4. Confirmed primary defect: reserved NRPN commands inside sound sweeps

The enabled parameter enumeration embeds MIDI transport slots among sound values:

| Sound-array index | Symbol | Low-CC translation / purpose |
| ---: | --- | --- |
| 5 | `PAR_NRPN_DATA_ENTRY_COARSE` | CC6: execute NRPN Data Entry |
| 97 | `PAR_NRPN_FINE` | CC98: change NRPN low bits |
| 98 | `PAR_NRPN_COARSE` | CC99: change NRPN high bits |
| 127 | `PAR_BANK_CHANGE` | Wraps to CC0 |
| 128 | `PAR_FILTER_DRIVE_1` | CC2 number 0: Voice 1 Filter Drive |
| 129–133 | `PAR_FILTER_DRIVE_2` through `_6` | Remaining Voice Filter Drives |

Definitions: `Core/Inc/Parameters.h:35–40`, `143–147`, `181–192`; `Core/Inc/Soundengine/MIDI/MidiMessages.h:84–90`, `238–247`.

Original sweep dispatch:

```c
const uint8_t val = interpolate(activeParameters[morph_index],
                                morphParameters[morph_index], morph);
if(morph_index < 128) {
    msg.status = MIDI_CC;
    msg.data1 = morph_index;
    msg.data2 = val;
    msg.data1 += 1;
    msg.data1 &= 0x7f;
    midiParser_ccHandler(msg, 1, 0, 0.f);
} else {
    msg.status = CC_2;
    msg.data1 = morph_index - 128;
    msg.data2 = val;
    msg.data1 &= 0x7f;
    midiParser_ccHandler(msg, 1, 0, 0.f);
}
```

At index 5 this sends CC6. The CC handler executes:

```c
case NRPN_DATA_ENTRY_COARSE:
    midiParser_nrpnHandler(value.itg);
    return;
```

The original NRPN helper (`MidiParser.c:134–140`) creates a CC2 command using the selected NRPN:

```c
MidiMsg msg2;
msg2.status = MIDI_CC2;
msg2.data1 = midiParser_activeNrpnNumber;
msg2.data2 = value;
midiParser_ccHandler(msg2, true, 0, 0.f);
```

The selector is initialized to 0 (`MidiParser.c:35`). CC2 number 0 is Voice 1 Filter Drive. Thus, **when selector=0 and reserved slot 5 contains 0**, each original sweep writes Drive=0 near its start, and restores the intended Drive only at index 128:

```text
Before sweep       Intended Voice 1 Drive
Index 5            Voice 1 Drive <- interpolated reserved-slot byte (0 in this scenario)
Indices 6–127      Wrong Drive persists unless another writer intervenes
Index 128          Voice 1 Drive <- intended interpolated Filter Drive
```

**Confirmed:** the unintended command/write path exists. The new sweep regression rejects the original sweep precisely when it emits NRPN Data Entry.

**Conditional:** the user's actual reserved-slot bytes and runtime selector were not inspected. Therefore the precise zero-injection scenario is the strongest source explanation, not a hardware-traced fact. The selector can be changed by external MIDI and by sweep slots 97/98; subsequent sweeps can differ depending on those values.

This explains identical-kit reproduction without interpolation differences, the Voice 1 bias when selector=0, the temporary Drive=0 sound, larger disturbances at high Drive, and suppression when filtering is OFF. It does not require any memory corruption.

## 5. Confirmed sweep restart behavior and endpoint explanation

Every Morph event unconditionally resets `morph_index=0`. There is no unchanged-value suppression, in-progress check or pending-target mechanism. A complete sweep needs 247 service calls. Original restoration of Voice 1 Drive is the 129th tick (index 128).

- New events arriving before index 128 can indefinitely postpone restoration.
- Events arriving after index 128 can create repeated down/up Drive excursions.
- Restart is an amplifier of the reserved-command defect, not the initial cause of the unintended write.
- Actual event/service timing was not measured, so starvation on every movement is not proven.
- Even an uninterrupted original sweep contains the unintended write.

At an endpoint, physical movement still creates an input event even when clamping leaves the value at 0 or 255. The menu dispatches it and restarts Morph. A stationary knob stops producing fresh events; the remaining sweep can finish. This is consistent with the observations. Scheduling/redundancy suppression was explicitly excluded from this patch and remains unchanged.

## 6. Confirmed independent defect: CC0 cache underflow and memory corruption

At index 127, `(127+1)&0x7f` becomes CC0. The original handler executes cache bookkeeping before the switch:

```c
const uint16_t paramNr = msg.data1 - 1; // CC0 -> 65535
if(updateOriginalValue && !isFloatModulator) {
    midiParser_originalCcValues[paramNr + 1] = value.itg;
}
```

The cache is declared `uint8_t midiParser_originalCcValues[0xff]` (`MidiParser.c:37`, header `MidiParser.h:53`), with valid indices 0–254. On the 32-bit target, `paramNr+1` promotes to an integer value of **65536**; it does not wrap to zero. This out-of-bounds C access has undefined behavior. The original path also calls `modNode_originalValueChanged(65535)` after the switch for non-float events.

During the investigation, the pre-patch release ELF and map already present in the workspace were inspected without rebuilding. Observed layout:

| Item | Observed pre-patch artifact value |
| --- | --- |
| Cache address | `0x2002653c` |
| Erroneous address, base + 65536 | `0x2003653c` |
| `seq_patternSet` address | `0x20029014` |
| Erroneous offset inside `seq_patternSet` | `0xd528` |
| `voiceArray` address | `0x20009238` |

Historical map references were `build/release/LXR_V2_STM32F765.map:5994`, `6079`, `6204`. The original disassembly contained a narrowed 16-bit subtraction followed by the indexed byte store. **These were historical artifact observations; implementation builds later regenerated those map/ELF files.** They are not claims about the current patched layout or the user's v1.72 layout.

This defect can corrupt sequencer pattern memory in that observed build. It could cause secondary disturbances when the affected data is used, but there is no established path from that particular location to the reported Drive-specific symptoms. It did not directly target the filter/voice structure in that artifact. Rank it below the direct NRPN-to-Drive path as an explanation of the clicks, while still treating it as a confirmed defect requiring correction.

The original `CC_BANK_CHANGE` switch body itself was inert/commented out (`MidiParser.c:179–187`). External bank changes are forwarded to the UI by the outer MIDI parser. The internal Morph sweep calls only the CC handler; it does not automatically reload a kit merely by reaching the CC0 case.

## 7. CC2/NRPN bounds and external MIDI compatibility

Original CC2 bookkeeping uses `paramNr=msg.data1+128` without validating either the sound-parameter boundary or cache capacity (`MidiParser.c:908–911`). A raw 8-bit CC2 number can yield indices 128–383. Ordinary Morph CC2 indices 128–246 fit; malformed or special command inputs do not necessarily fit.

The NRPN selector is 16-bit but `MidiMsg.data1` is 8-bit (`MidiMessages.h:34–40`). Without validation, NRPN 256 can alias CC2 number 0 and modify Voice 1 Drive; other high values can alias other parameters or mute commands.

The CC2 enumeration has explicit external mute commands `CC2_MUTE_1=200` through `CC2_MUTE_7=206` (`MidiMessages.h:399–411` in the original checkout). They are legitimate external NRPN commands, but their derived cache indices 328–334 and modulation indices are not sound parameters. Simply rejecting every CC2 command above the sound range would break supported external MIDI muting.

The patch therefore:

1. Rejects NRPN selections greater than `UINT8_MAX` before narrowing; remaining unsupported numbers are rejected by the CC2 sound-range validation.
2. Rejects CC0 and malformed low-CC numbers above 127 before sound-parameter decoding.
3. Allows normal CC2 sound indices only when below both `END_OF_SOUND_PARAMETERS` and cache size.
4. Preserves mute commands 200–206, but excludes them from cache writes and modulation notifications.
5. Keeps external CC/NRPN routing, data scaling, valid parameter handling and external bank forwarding intact.

Additional pre-existing hardening concerns remain outside this patch: active `paramArray_setParameter()` indexes before validation (`Core/Src/Soundengine/MIDI/ParameterArray.c:32–39`), and modulation destination setters/update paths similarly assume valid destinations (`Core/Src/Soundengine/DSPAudio/modulationNode.c:120–136`, `164–172`). The disabled `#else` parameter setter contains a guard, but the enabled path does not. There is no demonstrated role for these concerns with valid disabled modulators in the user's reproduction. This patch is not a general memory-safety audit or remediation of all modulation paths.

## 8. Drive/DSP interpretation and ranked causes

Enabled configuration: `UNIT_GAIN_DRIVE=0`, `USE_FILTER_DRIVE=0` in `Core/Inc/Soundengine/config.h:43–45`; `USE_SHAPER_NONLINEARITY=0` in `Core/Inc/Soundengine/DSPAudio/ResonantFilter.h:35`.

```c
// ResonantFilter.c:89–98, enabled mapping
filter->drive = 0.4f + drive * drive * 6;

// ResonantFilter.c:165
if(type == FILTER_OFF) return;

// ResonantFilter.c:174 and 225, enabled input processing
float x = softClipTwo((buf[i] / (float)0x7fff) * filter->drive);
```

Drive UI values 0, 64, 127 correspond to input multipliers approximately 0.4, 1.923715, 6.4. Resetting UI Drive to zero is therefore not literal zero gain; it restores the low-Drive processing curve and level. A high-Drive excursion to that state and back can be large.

This is an abrupt parameter change, not evidence of an explicit temporary filter bypass. The drum filter-type handler only assigns the type and does not reset the filter (`MidiParser.c:1039–1052`). No filter-state reset is needed to explain the reproduction. The filter OFF branch bypasses the affected processing, explaining its diagnostic value.

Filter Drive is not smoothed in this path. `SMOOTH_MIXER_VOL_PAN_DRIVE=1` concerns other processing, notably the separate voice distortion after the filter (`DrumVoice.c:290–295`), not `filter.drive` itself. Audio-generation calls and parameter writes are interleaved rather than treating the whole sweep as an atomic update. Successive blocks can therefore use different Drive values. No allocation or file I/O is present in the Morph tick. No timing trace established DMA underruns or an audio deadline failure.

Original interpolation (`presetManager.c:1253–1258`):

```c
uint16_t fixedPointValue = (uint16_t)((a * 256) + (b - a) * x);
uint8_t result = (uint8_t)(fixedPointValue / 256);
return (uint8_t)((fixedPointValue & 0xff) < 0x7f ? result : result + 1);
```

For identical endpoints `a==b`, this preserves the byte exactly for every Morph value. That was checked in-memory during investigation and through sweep regression assertions during implementation. General rounding/endpoint behavior for different kits was not changed or comprehensively audited. The source comment calls the rounding “bankers rounding”; this report does not endorse that description.

| Rank | Cause/mechanism | Evidence status |
| ---: | --- | --- |
| 1 | Reserved NRPN Data Entry changes Voice 1 Drive early in a sweep | Confirmed code path; exact hardware selector/slot values unverified |
| 2 | Unsmoothed Drive writes make the unwanted excursion discontinuous | Confirmed mechanism; audible output not measured here |
| 3 | Repeated sweep restart extends/repeats the wrong state | Confirmed behavior; actual starvation timing unmeasured |
| 4 | CC0 out-of-bounds cache write | Confirmed independent defect; weaker explanation for filter-specific symptoms |
| 5 | Other parameter steps/audio deadline misses | Hypotheses, no establishing measurements |

## 9. Exact implemented correction and deliberately unchanged behavior

Only two production files changed, totaling **35 insertions, 12 deletions**:

| File | Patch location | Correction |
| --- | --- | --- |
| `Core/Src/Preset/presetManager.c` | patched 1264 | Shared predicate identifying the four transport slots. |
| Same file | patched 1316 | Immediate kit sweep skips commands for those slots, retaining one `soundengine_process()` call per entry. |
| Same file | patched 2150 | Morph consumes a reserved slot's existing tick by incrementing the index and returning without dispatch. All four slots precede the final sound parameter. |
| `Core/Src/Soundengine/MIDI/MidiParser.c` | patched 134–143 | NRPN pre-narrowing guard. |
| Same file | patched 170–176 | CC0/malformed low-CC early return; removes obsolete unreachable CC0 switch body. |
| Same file | patched 905–913, 1438–1440 | CC2 bounds validation and mute-command cache/notification exemptions. |

Representative new guards:

```c
static uint8_t preset_isTransportParameter(uint8_t paramNr)
{
    return paramNr == PAR_NRPN_DATA_ENTRY_COARSE ||
        paramNr == PAR_NRPN_FINE || paramNr == PAR_NRPN_COARSE ||
        paramNr == PAR_BANK_CHANGE;
}

if(midiParser_activeNrpnNumber > UINT8_MAX) return;

if(msg.data1 == CC_BANK_CHANGE || msg.data1 > 127) return;

const uint8_t isMuteCommand =
    msg.data1 >= CC2_MUTE_1 && msg.data1 <= CC2_MUTE_7;
if(!isMuteCommand && (paramNr >= END_OF_SOUND_PARAMETERS ||
    paramNr >= sizeof(midiParser_originalCcValues))) return;
```

No filter algorithm, Drive transfer curve, smoothing constant, interpolation expression, Morph restart policy, sweep rate, parameter layout, preset format, version string or build configuration was changed. Source files retain their original CRLF endings. The local patch branch was created before edits. No commit was made for the implementation.

## 10. Regression methodology and observed results

Five new files, 397 lines total, exist under `tests/bug001/` and are reproduced in Appendix B. Run:

```sh
python3 tests/bug001/run.py
```

The runner uses a native GNU-compatible C compiler, `-O1 -g`, AddressSanitizer, UndefinedBehaviorSanitizer, and `-fno-sanitize-recover=all`. It compiles the complete production `MidiParser.c` directly into the parser fixture, so real static NRPN state and dispatch logic execute. It observes normalized Drive writes, modulation notifications, mute calls and external MIDI forwarding. Unrelated firmware function dependencies link to aborting trap stubs. Those traps intentionally have no production signatures/headers because they must never execute; encountering one fails the test rather than silently simulating hardware. No LTO is used for that host harness.

The sweep fixture extracts the **unmodified current production function bodies** of `interpolate`, `preset_isTransportParameter`, `preset_sendDrumsetParametersImmediate`, `preset_morph_tick` and `preset_morph`; compiles them with the real `Core/Src/Parameters.c` ordering array; and records their MIDI output. This avoids pulling SD-card/hardware operations into host execution or adding production test hooks. Its MIDI sink asserts absence of reserved transport commands and validates every dispatched parameter/value. The kit fixture includes nonzero reserved bytes.

ARM header compatibility flags supply `__fp16=_Float16` and suppress host-only unused-inline intrinsic declaration warnings. These tests do not run the ARM DSP intrinsics. The Drive stub records the normalized control input; it does not execute the nonlinear filter or emulate its curve.

### Passing coverage

- CC0 under all four combinations of cache-update/float-modulator flags: no cache change, notification or selector change.
- External bank change retains the three-byte UI forwarding path and creates no sound-parameter notification.
- Low CC and CC127 cache/notification mapping preserved.
- Valid NRPN writes to Voice 1/3 Drive; external MIDI pipeline write to Voice 2 Drive.
- Last valid sound index updates its cache and notification safely.
- Invalid selectors 119, 126, 127, 128, 199, 207, 255, 256, 258, 456 and 16383 neither alias Drive nor invoke unintended mutes/notifications.
- All seven NRPN mutes/unmutes work without invalid notifications; external MIDI mute pipeline also exercised.
- Every CC2 byte outside the sound range, under all four flag combinations, leaves the cache unchanged and does not notify invalid modulation targets. Supported mute commands remain executable.
- All 256 Morph positions for identical kits dispatch exactly 243 nontransport entries once and preserve identical values.
- The existing one-index-per-tick and restart behavior is asserted, including repeatedly restarted sweeps followed by convergence.
- Immediate kit sweep dispatch and exactly 247 audio-service calls preserved.
- Idle Morph tick sends nothing.

Observed successful output:

```text
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence
```

### Negative controls against original source

Temporary copies of original files were read with `git show HEAD:<path>`; the working implementation was not reverted or altered.

1. Compiling the same parser fixture against original `MidiParser.c` failed as expected:

   ```text
   Assertion `idx < END_OF_SOUND_PARAMETERS' failed.
   EXPECTED BASELINE FAILURE: unpatched parser rejected by sanitized regression test
   ```

   This failure was the invalid CC0 modulation notification. It is not a claim that ASan reported the original distant write in that run.

2. Compiling the original sweep functions, retaining the new predicate only as a fixture assertion utility, failed as expected:

   ```text
   Assertion `msg.data1 != NRPN_DATA_ENTRY_COARSE' failed.
   EXPECTED BASELINE FAILURE: original sweeps dispatch reserved NRPN transport
   ```

These controls demonstrate that the regressions distinguish the defects from the corrected behavior; they are not full hardware baselines.

## 11. Release/debug builds and artifacts

The first bare `cmake` command was not on PATH. Both configurations subsequently built successfully using the provisioned tools. Reproduction commands from `/workspace/lxr-desktop-app`:

```sh
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH \
  cmake --preset release -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH \
  cmake --build --preset release

PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH \
  cmake --preset debug -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH \
  cmake --build --preset debug
```

Each build completed 122 Ninja steps, exited 0, linked the ELF and produced update images. Build logs printed normal installation instructions, but no installation/flashing command was executed. No bootloader was configured; these were firmware-only builds. Both presets retain the project's `-O3` behavior (Debug additionally has debug symbols).

| Metric | Release | Debug |
| --- | ---: | ---: |
| ITCM_RAM | 15,008 / 16,384 bytes (91.60%) | 15,008 / 16,384 bytes (91.60%) |
| DTCM | 107,392 / 131,072 bytes (81.93%) | 107,392 / 131,072 bytes (81.93%) |
| RAM | 271,040 / 393,216 bytes (68.93%) | 271,040 / 393,216 bytes (68.93%) |
| FLASH region use | 223,128 / 1,015,808 bytes (21.97%) | 223,176 / 1,015,808 bytes (21.97%) |
| GNU size text | 274,272 | 274,320 |
| GNU size data | 852 | 852 |
| GNU size bss | 340,576 | 340,576 |
| GNU size total (dec / hex) | 615,700 / `96514` | 615,748 / `96544` |
| `.img` bytes | 275,132 | 275,180 |
| Image-format checksum | `0x1B` | `0xC5` |

All memory regions remain within linker limits. Region-use values and GNU size section totals measure different things and should not be substituted for one another. No before/after size delta was established with a freshly built original baseline.

Artifacts, local only and not included in the report commit:

| Artifact | Local path |
| --- | --- |
| Release image | `build/release/LXR_V2_V1.7.img` |
| Debug image | `build/debug/LXR_V2_V1.7.img` |
| Release ELF/map | `build/release/LXR_V2_STM32F765.elf`, `.map` |
| Debug ELF/map | `build/debug/LXR_V2_STM32F765.elf`, `.map` |
| Release complete build log | `/tmp/bug001-release-build.log` |
| Debug complete build log | `/tmp/bug001-debug-build.log` |

SHA-256 of generated images:

```text
65441a47a0352d2ba69d814b044c0d014cd76fe9cba130552223489c0e028d7d  build/release/LXR_V2_V1.7.img
418c0510dcf93bfcc7a5fe1832a52f3e930d9611e878b40c01bbbd9c944596ce  build/debug/LXR_V2_V1.7.img
```

Image hashes identify the local builds, not installed-device images. Full local build logs and binaries may disappear with the execution environment; the relevant numerical output is preserved here.

## 12. Errors encountered, resolutions and verification boundaries

| Event | Resolution / impact |
| --- | --- |
| Initial environment was still starting; execution reported no usable environment | Waited for it to become ready before source access. |
| Initial Git query in `/workspace` reported “not a git repository” | Correct repository is `/workspace/lxr-desktop-app`; no changes resulted. |
| Discovery searches for nonexistent source paths/case variants returned errors | Resolved actual file names under `Core/Src/IO`, `Core/Src/Soundengine`, etc.; no source changes resulted. |
| Native parser compilation initially lacked USB includes and ARM intrinsic declarations | Added existing USB include directories and host-only warning compatibility flags in test runner. |
| Native compiler did not recognize ARM `__fp16` | Host test runner defines it as `_Float16`; firmware types/configuration unchanged. |
| Initial `cmake` invocations exited 127, command not found | Used provisioned CMake/Ninja/toolchain PATH; both actual builds then exited 0. |
| Initial host run exited because LeakSanitizer cannot operate under ptrace | Runner defaults `ASAN_OPTIONS=detect_leaks=0`; ASan/UBSan remain enabled. No allocation/leak behavior is under test. Existing user-supplied ASAN_OPTIONS are preserved. |
| Plain `git diff --check` treated newly added CRLF lines as trailing whitespace | Preserved the repository's existing CRLF files and validated with `git -c core.whitespace=cr-at-eol diff --check`; passed. No global Git configuration changed. |
| Negative baseline tests aborted | Expected evidence of the original defects, distinct from passing corrected-code runs. |
| Report directory read on `lab-setup` returned 404 | Recursive tree inspection confirmed no report directory existed; creation is required, not replacement. |
| A URL-encoded slash in a report-branch lookup was rejected by the connector URL validator (400) | Listed all branches instead; only `main` and `lab-setup` existed at that read. No write had occurred. |

No automatic approval rejection occurred during implementation. No hardware timing, audio comparison, flashing, debugger-on-device run or on-device sanitizer test was performed. Build logs contain no reported `warning:`, `error:` or failed Ninja steps, but the project's compiler options suppress some warnings; successful logs are not a warning-clean audit of the entire firmware.

## 13. Remaining risks, unresolved questions and continuation plan

1. **Hardware confirmation:** Does this patch eliminate the supplied BassKick reproduction on the actual LXR-02? Only source/control-path evidence exists so far. The patch is ready for review, not a claim of measured audio resolution.
2. **Installed version provenance:** Which source revision/build corresponds exactly to v1.72? Do not identify these v1.7-metadata artifacts as official v1.72 builds.
3. **Reproduction state:** What are kit bytes 5, 97, 98 and 127, and the NRPN selector during movement? Their values would confirm the exact zero-Drive sequence in hardware.
4. **Timing:** What are knob-event frequency, sweep-service cadence, Drive restoration delay and audio deadline margins? The restart mechanism remains and can still postpone legitimate parameter convergence.
5. **Legitimate parameter steps:** Unsmoothing of real Drive changes and other discontinuous parameter changes remains by explicit scope. If clicks remain after this correction, trace those separately before changing DSP behavior.
6. **Other bounds assumptions:** The modulation destination accessors noted above remain unchecked in this minimal patch; general hardening is separate work.
7. **Publication scope:** Only the report is published. The local firmware branch still has uncommitted source/test changes and no remote implementation branch. Its full reconstruction material is in the appendices.
8. **Future enum changes:** The minimal Morph skip assumes the four fixed transport slots precede the last sound slot. Current exhaustive tick assertions validate that layout. Revisit the termination logic if parameter enums are restructured.
9. **Host test limits:** Parser dispatch is real, but hardware/DSP effects are stubbed; the sweep tests use extracted production bodies with a recording sink. They establish reserved-command exclusion and bounds behavior, not sonic equivalence or timing safety on the MCU.

Recommended next actions for another assistant:

- Read this report and inspect the local branch if it still exists. Check `git status` before doing anything; do not discard its uncommitted changes.
- If the workspace is gone, create a separate development branch from the investigated base (or an explicitly agreed newer base), apply Appendix A, and restore Appendix B files. Preserve CRLF when applying the production diff. Check the resulting hashes below when reconstruction is byte-identical.
- Rerun the targeted tests and both builds if reconstructing, changing code, changing toolchain or rebasing. Avoid treating the report branch itself as a firmware implementation branch.
- Review the minimal patch and test harness, especially external NRPN mutes and the two command domains. Keep malformed/unsupported selector rejection distinct from rejecting supported mute commands.
- Request authorization before committing/pushing the firmware implementation, creating a PR, merging or flashing. The authorization to publish this report does not extend to those actions.
- After approved hardware validation, exercise identical and different kits, all Morph values/endpoints, Drive 0/64/127, every filter type/OFF, Voices 1–6, external bank changes/NRPN Drive/mutes, and kit-load behavior. Capture Drive writes alongside deterministic kick audio and audio-deadline counters where practical.

## 14. Local source/test integrity snapshot

At handoff preparation the implementation checkout reported only two modified production files and untracked `tests/bug001/`. `HEAD` remained the investigated base; `origin/main` remained the same SHA. Test files are intentionally not staged/committed by this report operation.

SHA-256 (production hashes include original CRLF bytes):

```text
ac0901a2a422473de1e175807d1f808ad1878f99ede345e1c40926386e89419c  Core/Src/Preset/presetManager.c
076c8129e7d159f5e80f70584e78c2c745e15c457907539df691fb0d5ed04fab  Core/Src/Soundengine/MIDI/MidiParser.c
9ec9584e570db0a4f43d82e9959c021cba4fb2ccff225273092f065efb7ea403  tests/bug001/README.md
3fa50d94e04cbf44c82146831c76f50e4d14a9f598cf899f61c0b586385c7e8e  tests/bug001/run.py
df7c35a41140afa0d8dee2ba0b73a1b3d3c25f6dfb693f9bbdeba5418582f062  tests/bug001/test_parser.c
f086d64b13af4c58b188d9840bbf71ffc6aaf3ef3f502c1b00cd7fba3dc08800  tests/bug001/test_sweeps.c
ef8d444b079087949190ee3baa1966d032672ddb385eb6019673b8e41ae7f78d  tests/bug001/trap_stubs.c
```

## Appendix A — Complete production diff

The following is the complete diff against `a43cd4f` for the two production files. Displayed Markdown text normalizes CRLF for readability; reapply with awareness of the original line endings. No DSP file is modified.

```diff
diff --git a/Core/Src/Preset/presetManager.c b/Core/Src/Preset/presetManager.c
index 41ef096..b13a536 100644
--- a/Core/Src/Preset/presetManager.c
+++ b/Core/Src/Preset/presetManager.c
@@ -1259,6 +1259,14 @@ static uint8_t interpolate(uint8_t a, uint8_t b, uint8_t x)
 	//return ((a*255) + (b-a)*x)/255;
 }
 
+//----------------------------------------------------
+// These slots store MIDI transport state, not sound parameters.
+static uint8_t preset_isTransportParameter(uint8_t paramNr)
+{
+	return paramNr == PAR_NRPN_DATA_ENTRY_COARSE ||
+		paramNr == PAR_NRPN_FINE || paramNr == PAR_NRPN_COARSE ||
+		paramNr == PAR_BANK_CHANGE;
+}
 //----------------------------------------------------
 IN_ITCM_RAM void preset_sendDrumsetParametersImmediate()
 {
@@ -1305,6 +1313,11 @@ IN_ITCM_RAM void preset_sendDrumsetParametersImmediate()
 		{
 
 			uint8_t paramNumber = parameterOrder[i];
+			if(preset_isTransportParameter(paramNumber))
+			{
+				soundengine_process();
+				continue;
+			}
 
 			//const uint8_t morph = activeParameters[PAR_MORPH];
 
@@ -2132,6 +2145,14 @@ void preset_morph_tick()
 	if(morph_state)
 	{
 
+		// Consume the reserved slot's tick without executing a MIDI command.
+		// All transport slots precede the final sound parameter.
+		if(preset_isTransportParameter(morph_index))
+		{
+			morph_index++;
+			return;
+		}
+
 		uint8_t morph;
 
 		//Do not morph audio routing
diff --git a/Core/Src/Soundengine/MIDI/MidiParser.c b/Core/Src/Soundengine/MIDI/MidiParser.c
index bb68cb4..d119e9f 100644
--- a/Core/Src/Soundengine/MIDI/MidiParser.c
+++ b/Core/Src/Soundengine/MIDI/MidiParser.c
@@ -133,6 +133,9 @@ float midiParser_calcDetune(uint8_t value)
 //-----------------------------------------------------------
 static void midiParser_nrpnHandler(uint16_t value)
 {
+	// Reject unsupported 14-bit selections before narrowing to a MIDI byte.
+	if(midiParser_activeNrpnNumber > UINT8_MAX) return;
+
 	MidiMsg msg2;
 	msg2.status = MIDI_CC2;
 	msg2.data1 = midiParser_activeNrpnNumber;
@@ -164,6 +167,10 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 	if((msg.status &0xF0) == MIDI_CC)
 	{
 
+		// Bank changes are forwarded to the UI by parseMidiMessage(). They have
+		// no sound parameter and must not enter the cache or modulation path.
+		if(msg.data1 == CC_BANK_CHANGE || msg.data1 > 127) return;
+
 		const uint16_t paramNr = msg.data1-1;
 		if(updateOriginalValue && !isFloatModulator) {
 			midiParser_originalCcValues[paramNr+1] = value.itg;
@@ -176,16 +183,6 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 
 			break;
 
-		case CC_BANK_CHANGE:
-			// bank change (coarse) selects kit (sound)
-			/*
-			 * already send in parseMidiMessage()
-			uart_sendFrontpanelByte(MIDI_CC);
-			uart_sendFrontpanelByte(CC_BANK_CHANGE);
-			uart_sendFrontpanelByte(value.itg);
-			*/
-			break;
-
 		case NRPN_DATA_ENTRY_COARSE:
 			midiParser_nrpnHandler(value.itg);
 			return;
@@ -906,8 +903,13 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 	else //MIDI_CC2
 	{
 		const uint16_t paramNr = msg.data1 + 1 + 127;
+		// External NRPN mute commands are not sound parameters and have no
+		// cache entry or modulation target. Preserve their command handling.
+		const uint8_t isMuteCommand = msg.data1 >= CC2_MUTE_1 && msg.data1 <= CC2_MUTE_7;
+		if(!isMuteCommand && (paramNr >= END_OF_SOUND_PARAMETERS ||
+			paramNr >= sizeof(midiParser_originalCcValues))) return;
 
-		if(updateOriginalValue && !isFloatModulator) {
+		if(updateOriginalValue && !isFloatModulator && !isMuteCommand) {
 			midiParser_originalCcValues[paramNr] = value.itg;
 		}
 		switch(msg.data1)
@@ -1433,7 +1435,7 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 			default:
 				break;
 		}
-		if(!isFloatModulator)
+		if(!isFloatModulator && !isMuteCommand)
 		{
 			modNode_originalValueChanged(paramNr);
 		}
```

## Appendix B — Complete targeted regression files

These are the exact local test-file contents at handoff. Restore them under the displayed paths; run the Python runner rather than compiling fixtures individually.

### `tests/bug001/README.md`

````markdown
# Bug 001 regressions

Run `python3 tests/bug001/run.py` from the repository root. Requires a native
GNU-compatible C compiler with AddressSanitizer/UndefinedBehaviorSanitizer.
Temporary binaries and generated files are removed after the run.

The parser test compiles the complete production `MidiParser.c`. It observes
Drive writes, modulation notifications, mute commands and external bank-change
forwarding. Unrelated DSP/hardware calls abort through link stubs. The Drive stub
records the normalized input; it does not model the filter or its Drive curve.

The sweep test compiles the unmodified production interpolation, reserved-slot
predicate, immediate kit sweep, Morph tick and Morph start functions extracted
by the runner, plus the real parameter-order array. The MIDI sink records every
dispatch and rejects transport commands. This avoids bringing SD-card/hardware
code into the host process without adding test hooks to firmware.

Coverage includes all 256 Morph positions with identical kits, nonzero reserved
slots, existing restart/tick behavior, kit-load audio service cadence, CC0 with
all cache/modulator flags, valid CC/CC2/NRPN mapping, all seven external NRPN
mutes, invalid/high NRPN selectors, and every out-of-range CC2 byte with all
cache/modulator flags. Sanitizers check exercised memory accesses.
Leak detection is disabled by default because LeakSanitizer cannot run under
ptrace; AddressSanitizer and UndefinedBehaviorSanitizer remain enabled.

These are control-path tests, not an audio simulation or on-device validation.
````

### `tests/bug001/run.py`

````python
#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run focused production-code regressions using a native C compiler."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def function(source, name):
    # Mask comments/strings before counting braces; return the original text.
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                    lambda m: " " * len(m[0]), source, flags=re.S)
    match = re.search(r"^(?:IN_ITCM_RAM )?(?:static )?(?:uint8_t|void) "
                      + name + r"\([^;]*?\)\s*\{", masked, re.M)
    if not match:
        raise RuntimeError(f"Production function not found: {name}")
    depth = 1
    end = match.end()
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    return source[match.start():end]


def run(*args):
    env = os.environ.copy()
    # LeakSanitizer cannot run in ptraced execution environments. These tests
    # target bounds/UB, not allocation; retain ASan and UBSan instrumentation.
    env.setdefault("ASAN_OPTIONS", "detect_leaks=0")
    subprocess.run(args, cwd=ROOT, env=env, check=True)


def main():
    includes = [".", "Core/Inc", "Drivers/CMSIS/Include",
                "Drivers/CMSIS/Device/ST/STM32F7xx/Include",
                "Drivers/STM32F7xx_HAL_Driver/Inc",
                "Middlewares/Third_Party/FatFs/src",
                "Middlewares/JS/STM32_USB_Device_Library/Core/Inc",
                "Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc",
                "USB_DEVICE_MIDI/Target"]
    # ARM-specific inline DSP intrinsics are not executed by these tests.
    # _Float16 supplies the host spelling of the firmware's half-float type.
    flags = [os.environ.get("CC", "cc"), "-std=gnu11", "-O1", "-g",
             "-DSTM32F765xx", "-DUSE_HAL_DRIVER", "-D__fp16=_Float16",
             "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast",
             "-Wno-implicit-function-declaration", "-Wno-unused-function",
             "-ffunction-sections", "-fdata-sections", "-fno-pie", "-no-pie",
             "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
             "-fno-omit-frame-pointer"]
    flags += ["-I" + str(ROOT / p) for p in includes]
    with tempfile.TemporaryDirectory(prefix="lxr-bug001-") as tmp:
        tmp = Path(tmp)
        parser = tmp / "test_parser"
        run(*flags, str(HERE / "test_parser.c"), str(HERE / "trap_stubs.c"),
            "-Wl,--gc-sections", "-lm", "-o", str(parser))
        run(str(parser))

        source = (ROOT / "Core/Src/Preset/presetManager.c").read_text()
        names = ["interpolate", "preset_isTransportParameter",
                 "preset_sendDrumsetParametersImmediate", "preset_morph_tick",
                 "preset_morph"]
        (tmp / "sweep_functions.inc").write_text(
            "\n\n".join(function(source, name) for name in names))
        sweeps = tmp / "test_sweeps"
        run(*flags, "-I" + str(tmp), str(HERE / "test_sweeps.c"),
            str(ROOT / "Core/Src/Parameters.c"), "-Wl,--gc-sections",
            "-o", str(sweeps))
        run(str(sweeps))


if __name__ == "__main__":
    main()
````

### `tests/bug001/test_parser.c`

````c
// SPDX-License-Identifier: GPL-3.0-only
// Compile the complete production parser, including its static NRPN state.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "Core/Src/Soundengine/MIDI/MidiParser.c"

DrumVoice voiceArray[NUM_DRUM_VOICES];
SnareVoice snareVoice;
HiHatVoice hatVoice;
CymbalVoice cymbalVoice;
FxBus fxBus;
ModulationNode velocityModulators[6];
uint8_t activeParameters[NUM_PARAMS];
uint8_t mixer_audioRouting[6];
float mixer_decimation_rate[7];
uint8_t frontParser_activeTrack, menu_activePage;
PatternSet seq_patternSet;
volatile uint32_t systick_ticks;

static unsigned notifications, drive_writes, mute_writes, forwarded;
static uint8_t mute_voice, mute_value, forwarded_bytes[3];
static uint16_t last_notification;

void modNode_originalValueChanged(uint16_t idx)
{
    assert(idx < END_OF_SOUND_PARAMETERS);
    last_notification = idx;
    notifications++;
}
void SVF_setDrive(ResonantFilter *filter, float drive)
{
    filter->drive = drive;
    drive_writes++;
}
void seq_setMute(uint8_t voice, uint8_t muted)
{
    assert(voice < 7);
    mute_voice = voice;
    mute_value = muted;
    mute_writes++;
}
void seq_recordAutomation(uint8_t track, uint8_t param, uint8_t value)
{
    (void)track; (void)param; (void)value;
}
void uart_sendFrontpanelByte(uint8_t value)
{
    assert(forwarded < sizeof(forwarded_bytes));
    forwarded_bytes[forwarded++] = value;
}

static void cc(uint8_t number, uint8_t value)
{
    MidiMsg msg = {.status = MIDI_CC, .data1 = number, .data2 = value};
    midiParser_ccHandler(msg, 1, 0, 0.f);
}
static void nrpn(uint16_t number, uint8_t value)
{
    cc(NRPN_FINE, number & 127);
    cc(NRPN_COARSE, (number >> 7) & 127);
    cc(NRPN_DATA_ENTRY_COARSE, value);
}
static void external_cc(uint8_t number, uint8_t value)
{
    MidiMsg msg = {.status = MIDI_CC, .data1 = number, .data2 = value};
    forwarded = 0;
    midiParser_parseMidiMessage(msg);
    assert(forwarded == 3 && forwarded_bytes[0] == MIDI_CC);
    assert(forwarded_bytes[1] == number && forwarded_bytes[2] == value);
}

int main(void)
{
    uint8_t cache[sizeof(midiParser_originalCcValues)];
    memset(midiParser_originalCcValues, 0xa5, sizeof(cache));
    memcpy(cache, midiParser_originalCcValues, sizeof(cache));

    // CC0 must not touch the cache, notify modulators, or change the selector.
    midiParser_activeNrpnNumber = 3;
    for(unsigned flags = 0; flags < 4; flags++) {
        MidiMsg msg = {.status = MIDI_CC, .data1 = 0, .data2 = 127};
        midiParser_ccHandler(msg, flags & 1, (flags >> 1) & 1, 0.5f);
    }
    assert(!notifications && !memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
    assert(midiParser_activeNrpnNumber == 3);

    // Bank change still reaches the UI through the external MIDI entry point.
    MidiMsg bank = {.status = MIDI_CC, .data1 = CC_BANK_CHANGE, .data2 = 17};
    midiParser_parseMidiMessage(bank);
    assert(forwarded == 3 && forwarded_bytes[0] == MIDI_CC);
    assert(forwarded_bytes[1] == CC_BANK_CHANGE && forwarded_bytes[2] == 17);
    assert(!notifications && !memcmp(cache, midiParser_originalCcValues, sizeof(cache)));

    // Valid low CC, CC127 and CC2 Drive retain their cache/notification mapping.
    cc(CC_MOD_WHEEL, 23);
    assert(midiParser_originalCcValues[1] == 23 && last_notification == 0);
    cc(AMOUNT_LFO6, 31);
    assert(midiParser_originalCcValues[127] == 31 && last_notification == 126);
    nrpn(CC2_FILTER_DRIVE_1, 127);
    assert(drive_writes == 1 && voiceArray[0].filter.drive == 1.f);
    assert(midiParser_originalCcValues[128] == 127 && last_notification == 128);
    nrpn(CC2_FILTER_DRIVE_3, 64);
    assert(drive_writes == 2 && voiceArray[2].filter.drive == 64 / 127.f);
    nrpn(END_OF_SOUND_PARAMETERS - 129, 42);
    assert(last_notification == END_OF_SOUND_PARAMETERS - 1);
    assert(midiParser_originalCcValues[END_OF_SOUND_PARAMETERS - 1] == 42);

    // Valid NRPN also works through the external MIDI routing/channel path.
    external_cc(NRPN_FINE, CC2_FILTER_DRIVE_2);
    external_cc(NRPN_COARSE, 0);
    external_cc(NRPN_DATA_ENTRY_COARSE, 31);
    assert(drive_writes == 3 && voiceArray[1].filter.drive == 31 / 127.f);

    // A high 14-bit selector must never alias a low parameter or mute command.
    const uint16_t invalid[] = {119, 126, 127, 128, 199, 207, 255, 256, 258, 456, 16383};
    for(unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        cc(NRPN_FINE, invalid[i] & 127);
        cc(NRPN_COARSE, invalid[i] >> 7);
        unsigned before = notifications;
        cc(NRPN_DATA_ENTRY_COARSE, 0);
        assert(notifications == before && drive_writes == 3 && !mute_writes);
    }

    // Special external NRPN mutes work, without cache access or notification.
    for(unsigned voice = 0; voice < 7; voice++) {
        cc(NRPN_FINE, (CC2_MUTE_1 + voice) & 127);
        cc(NRPN_COARSE, (CC2_MUTE_1 + voice) >> 7);
        unsigned before = notifications;
        cc(NRPN_DATA_ENTRY_COARSE, 127);
        assert(notifications == before && mute_voice == voice && mute_value == 1);
        cc(NRPN_DATA_ENTRY_COARSE, 0);
        assert(notifications == before && mute_value == 0);
    }
    assert(mute_writes == 14);
    external_cc(NRPN_FINE, CC2_MUTE_7 & 127);
    external_cc(NRPN_COARSE, CC2_MUTE_7 >> 7);
    unsigned before = notifications;
    external_cc(NRPN_DATA_ENTRY_COARSE, 127);
    assert(mute_writes == 15 && mute_voice == 6 && mute_value == 1);
    assert(notifications == before);

    // Exhaust every CC2 byte for all cache/modulator flag combinations.
    for(unsigned flags = 0; flags < 4; flags++) {
        for(unsigned number = 0; number < 256; number++) {
            unsigned param = number + 128;
            if(param < END_OF_SOUND_PARAMETERS) continue; // normal DSP covered by firmware builds
            MidiMsg msg = {.status = MIDI_CC2, .data1 = number, .data2 = 0};
            unsigned before = notifications;
            memcpy(cache, midiParser_originalCcValues, sizeof(cache));
            midiParser_ccHandler(msg, flags & 1, (flags >> 1) & 1, 0.f);
            assert(notifications == before);
            assert(!memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
        }
    }
    puts("PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds");
}
````

### `tests/bug001/test_sweeps.c`

````c
// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "menu/menu.h"
#include "frontPanelParser_ui.h"
#include "Soundengine/MIDI/MidiParser.h"

uint8_t activeParameters[NUM_PARAMS];
uint8_t mainParameters[NUM_KIT_SLOTS][END_OF_SOUND_PARAMETERS];
uint8_t *morphParameters = mainParameters[0];
const ModTarg modTargets[] = {{0, 0}};
static uint8_t morph_state, morph_index;
static unsigned seen[END_OF_SOUND_PARAMETERS], dispatched, audio_calls;

void midiParser_ccHandler(MidiMsg msg, uint8_t update, uint8_t floating, float v)
{
    (void)v;
    unsigned param;
    assert(update == 1 && floating == 0);
    if(msg.status == MIDI_CC) {
        assert(msg.data1 != CC_BANK_CHANGE);
        assert(msg.data1 != NRPN_DATA_ENTRY_COARSE);
        assert(msg.data1 != NRPN_FINE && msg.data1 != NRPN_COARSE);
        param = msg.data1 - 1;
    } else {
        assert(msg.status == CC_2);
        param = msg.data1 + 128;
    }
    assert(param < END_OF_SOUND_PARAMETERS);
    assert(msg.data2 == activeParameters[param]); // identical kits must stay identical
    seen[param]++;
    dispatched++;
}
void frontPanel_sendData(uint8_t status, uint8_t data1, uint8_t data2)
{
    (void)data1; (void)data2;
    assert(status == CC_VELO_TARGET || status == CC_LFO_TARGET);
}
void soundengine_process(void) { audio_calls++; }

// The runner inserts unmodified production function bodies here.
#include "sweep_functions.inc"

static void check_sweep(void)
{
    assert(dispatched == END_OF_SOUND_PARAMETERS - 4);
    for(unsigned i = 0; i < END_OF_SOUND_PARAMETERS; i++)
        assert(seen[i] == (preset_isTransportParameter(i) ? 0u : 1u));
    memset(seen, 0, sizeof(seen));
    dispatched = 0;
}

int main(void)
{
    // Nonzero transport slots ensure their values cannot silently act as commands.
    memset(activeParameters, 73, END_OF_SOUND_PARAMETERS);
    for(unsigned i = 0; i < 6; i++) {
        activeParameters[PAR_VEL_DEST_1 + i] = 0;
        activeParameters[PAR_TARGET_LFO1 + i] = 0;
        activeParameters[PAR_VOICE_LFO1 + i] = 1;
    }
    memcpy(morphParameters, activeParameters, END_OF_SOUND_PARAMETERS);
    for(unsigned morph = 0; morph < 256; morph++) {
        activeParameters[PAR_MORPH] = morph;
        preset_morph(morph);
        for(unsigned tick = 0; tick < END_OF_SOUND_PARAMETERS; tick++) {
            assert(morph_state && morph_index == tick);
            preset_morph_tick();
        }
        assert(!morph_state);
        preset_morph_tick(); // idle must not dispatch
        check_sweep();
    }
    // Repeated events retain the existing restart behavior and final convergence.
    for(unsigned event = 0; event < 16; event++) {
        preset_morph(255);
        for(unsigned tick = 0; tick < 110; tick++) preset_morph_tick();
    }
    memset(seen, 0, sizeof(seen)); dispatched = 0;
    preset_morph(255);
    while(morph_state) preset_morph_tick();
    check_sweep();

    preset_sendDrumsetParametersImmediate();
    check_sweep();
    assert(audio_calls == END_OF_SOUND_PARAMETERS);
    puts("PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence");
}
````

### `tests/bug001/trap_stubs.c`

````c
// SPDX-License-Identifier: GPL-3.0-only
// Unrelated firmware dependencies needed to link the complete production parser.
// These deliberately have no production headers or ABI assumptions: they must
// never be called. An unexpected path aborts instead of silently passing.
#include <stdio.h>
#include <stdlib.h>
#define TRAP(name) void name(void) { fputs("Unexpected call: " #name "\n", stderr); abort(); }
TRAP(Compressor_setRatio)
TRAP(Cymbal_setPan)
TRAP(DecayEg_setDecay)
TRAP(DecayEg_setSlope)
TRAP(FxBus_prepareFxChange)
TRAP(FxBus_setParameter)
TRAP(FxBus_updateParameter)
TRAP(HiHat_setPan)
TRAP(Ringmod_setWave)
TRAP(SVF_directSetFilterValue)
TRAP(SVF_setReso)
TRAP(Snare_setPan)
TRAP(lfo_setFreq)
TRAP(lfo_setSync)
TRAP(menu_getActiveVoice)
TRAP(menu_getSubPage)
TRAP(menu_getViewedPattern)
TRAP(menu_repaint)
TRAP(menu_switchSubPage)
TRAP(osc_recalcFreq)
TRAP(seq_addNote)
TRAP(seq_getExtSync)
TRAP(seq_isRunning)
TRAP(seq_isTrackMuted)
TRAP(seq_midiNoteOff)
TRAP(seq_sendMidiNoteOn)
TRAP(seq_setNextPattern)
TRAP(seq_setRunning)
TRAP(seq_sync)
TRAP(setDistortionShape)
TRAP(setPan)
TRAP(slopeEg2_calcDecay)
TRAP(slopeEg2_setAttack)
TRAP(slopeEg2_setDecay)
TRAP(slopeEg2_setSlope)
TRAP(sync_midiResetPosition)
TRAP(sync_midiStartStop)
TRAP(transient_setWaveform)
TRAP(uart_sendMidi)
TRAP(usb_sendMidi)
TRAP(voiceControl_noteOn)
````

## Appendix C — Complete captured firmware build logs

The following logs are preserved because the `/tmp` originals and local build artifacts are ephemeral. The installation instructions are generated build output, not operations performed by this agent.

### Release

```text
[1/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c.obj
[2/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c.obj
[3/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c.obj
[4/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c.obj
[5/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c.obj
[6/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c.obj
[7/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c.obj
[8/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c.obj
[9/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c.obj
[10/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c.obj
[11/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c.obj
[12/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c.obj
[13/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c.obj
[14/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c.obj
[15/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c.obj
[16/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c.obj
[17/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c.obj
[18/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c.obj
[19/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c.obj
[20/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c.obj
[21/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c.obj
[22/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj
[23/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c.obj
[24/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c.obj
[25/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c.obj
[26/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c.obj
[27/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c.obj
[28/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[29/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj
[30/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff_gen_drv.c.obj
[31/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/option/syscall.c.obj
[32/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c.obj
[33/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/App/fatfs.c.obj
[34/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[35/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[36/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/fatfs_platform.c.obj
[37/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[38/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[39/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[40/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[41/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usb_device.c.obj
[42/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_cdc_if.c.obj
[43/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/Target/usbd_conf.c.obj
[44/122] Building ASM object Core/CMakeFiles/lxr_platform.dir/Startup/startup_stm32f765vgtx.s.obj
[45/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_desc.c.obj
[46/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[47/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[48/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/system_stm32f7xx.c.obj
[49/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_hal_msp.c.obj
[50/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/sysmem.c.obj
[51/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/syscalls.c.obj
[52/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/delay.c.obj
[53/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/flash_if.c.obj
[54/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/DIO.c.obj
[55/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/uart.c.obj
[56/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/adcPots.c.obj
[57/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/encoder.c.obj
[58/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[59/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/lcd.c.obj
[60/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[61/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/ledHandler.c.obj
[62/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/graphic.c.obj
[63/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/frontPanelParser.c.obj
[64/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/Parameters.c.obj
[65/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/Cc2Text.c.obj
[66/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/copyClearTools.c.obj
[67/122] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/initKit.c.obj
[68/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/screensaver.c.obj
[69/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/1PoleLp.c.obj
[70/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/automationNode.c.obj
[71/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/BufferTools.c.obj
[72/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/CymbalVoice.c.obj
[73/122] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[74/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Decay.c.obj
[75/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/distortion.c.obj
[76/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/dither.c.obj
[77/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/DrumVoice.c.obj
[78/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/HiHat.c.obj
[79/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[80/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/lfo.c.obj
[81/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/modulationNode.c.obj
[82/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/mixer.c.obj
[83/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/random.c.obj
[84/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Samples.c.obj
[85/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/SlopeEg2.c.obj
[86/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Oscillator.c.obj
[87/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/ResonantFilter.c.obj
[88/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/snapEg.c.obj
[89/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/squareRootLut.c.obj
[90/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Snare.c.obj
[91/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/TiltEq.c.obj
[92/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientGenerator.c.obj
[93/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientTables.c.obj
[94/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/wavetable.c.obj
[95/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Compressor.c.obj
[96/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c.obj
[97/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Reverb.c.obj
[98/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Ringmod.c.obj
[99/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Delay.c.obj
[100/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Distortion.c.obj
[101/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c.obj
[102/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c.obj
[103/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c.obj
[104/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/AudioCodecManager.c.obj
[105/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c.obj
[106/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[107/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FIFO.c.obj
[108/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[109/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FxBus.c.obj
[110/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/TriggerOut.c.obj
[111/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiMessages.c.obj
[112/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[113/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiVoiceControl.c.obj
[114/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/Uart.c.obj
[115/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/clockSync.c.obj
[116/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[117/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[118/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/ParameterArray.c.obj
[119/122] Building C object Core/CMakeFiles/lxr_usb_app.dir/Src/Soundengine/USB/usb_manager.c.obj
[120/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[121/122] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15008 B        16 KB     91.60%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      223128 B       992 KB     21.97%
   text	   data	    bss	    dec	    hex	filename
 274272	    852	 340576	 615700	  96514	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 275132 bytes, checksum 0x1B
[122/122] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================
```

### Debug

```text
[1/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c.obj
[2/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c.obj
[3/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c.obj
[4/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c.obj
[5/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c.obj
[6/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c.obj
[7/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c.obj
[8/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c.obj
[9/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c.obj
[10/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c.obj
[11/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c.obj
[12/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c.obj
[13/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c.obj
[14/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c.obj
[15/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c.obj
[16/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c.obj
[17/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c.obj
[18/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c.obj
[19/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c.obj
[20/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c.obj
[21/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c.obj
[22/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj
[23/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c.obj
[24/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c.obj
[25/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c.obj
[26/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c.obj
[27/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c.obj
[28/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[29/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj
[30/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c.obj
[31/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff_gen_drv.c.obj
[32/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/option/syscall.c.obj
[33/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/App/fatfs.c.obj
[34/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[35/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/fatfs_platform.c.obj
[36/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[37/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[38/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[39/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[40/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[41/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[42/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usb_device.c.obj
[43/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/Target/usbd_conf.c.obj
[44/122] Building ASM object Core/CMakeFiles/lxr_platform.dir/Startup/startup_stm32f765vgtx.s.obj
[45/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_cdc_if.c.obj
[46/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_desc.c.obj
[47/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[48/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_hal_msp.c.obj
[49/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/syscalls.c.obj
[50/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[51/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/system_stm32f7xx.c.obj
[52/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/sysmem.c.obj
[53/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/delay.c.obj
[54/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/flash_if.c.obj
[55/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/adcPots.c.obj
[56/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/uart.c.obj
[57/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/DIO.c.obj
[58/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/encoder.c.obj
[59/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/lcd.c.obj
[60/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[61/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/graphic.c.obj
[62/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/ledHandler.c.obj
[63/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/Parameters.c.obj
[64/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/Cc2Text.c.obj
[65/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/frontPanelParser.c.obj
[66/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/copyClearTools.c.obj
[67/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/screensaver.c.obj
[68/122] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/initKit.c.obj
[69/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/1PoleLp.c.obj
[70/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/automationNode.c.obj
[71/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/BufferTools.c.obj
[72/122] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[73/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[74/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/CymbalVoice.c.obj
[75/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Decay.c.obj
[76/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/dither.c.obj
[77/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/distortion.c.obj
[78/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/DrumVoice.c.obj
[79/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/HiHat.c.obj
[80/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/lfo.c.obj
[81/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/modulationNode.c.obj
[82/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/mixer.c.obj
[83/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Oscillator.c.obj
[84/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Samples.c.obj
[85/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/random.c.obj
[86/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/ResonantFilter.c.obj
[87/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/SlopeEg2.c.obj
[88/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/snapEg.c.obj
[89/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/squareRootLut.c.obj
[90/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Snare.c.obj
[91/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/TiltEq.c.obj
[92/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientGenerator.c.obj
[93/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientTables.c.obj
[94/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/wavetable.c.obj
[95/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Compressor.c.obj
[96/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Delay.c.obj
[97/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c.obj
[98/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Distortion.c.obj
[99/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Reverb.c.obj
[100/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Ringmod.c.obj
[101/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c.obj
[102/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c.obj
[103/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c.obj
[104/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c.obj
[105/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/AudioCodecManager.c.obj
[106/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[107/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FIFO.c.obj
[108/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[109/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FxBus.c.obj
[110/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/TriggerOut.c.obj
[111/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiMessages.c.obj
[112/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[113/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiVoiceControl.c.obj
[114/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[115/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/Uart.c.obj
[116/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/clockSync.c.obj
[117/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/ParameterArray.c.obj
[118/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[119/122] Building C object Core/CMakeFiles/lxr_usb_app.dir/Src/Soundengine/USB/usb_manager.c.obj
[120/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[121/122] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15008 B        16 KB     91.60%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      223176 B       992 KB     21.97%
   text	   data	    bss	    dec	    hex	filename
 274320	    852	 340576	 615748	  96544	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 275180 bytes, checksum 0xC5
[122/122] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================
```

---

End of self-contained Bug 001 handoff. Only this Markdown report is intended for the report-branch commit. Firmware source/test publication, PR creation, merging and flashing remain unperformed and require separate authorization.
