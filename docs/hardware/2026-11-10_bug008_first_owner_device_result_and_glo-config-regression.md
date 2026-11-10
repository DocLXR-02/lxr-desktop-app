# BUG008 — First Owner Device Observations and Boot Global-Configuration Regression

**Date:** 2026-11-10 (Europe/Vienna)  
**Classification:** OWNER HARDWARE OBSERVATIONS + INDEPENDENT SOURCE ANALYSIS  
**Candidate source:** `2e3a60e251dffbceefc0ee4af258463f5d87126c`  
**Protected baseline:** accepted R19 at `638b6eaae5b1624637d20cbc11a4a3e0175edca2`  
**Previous build evidence:** [BUG008 successful ARM and host validation](../codex-berichte/2026-11-10_bug008-continuous-loop-automation-candidate-arm-validation.md)  
**Firmware changes/builds in this review:** NONE

## 1. Owner-reported real-device results

The owner installed the BUG008 validation candidate on an LXR-02 and reported that parameter recording behaved exactly as planned. Multiple consecutive takes work; the owner can audition the most recent committed loop while recording remains armed; subsequent loops with no new parameter input preserve the last completed automation curve; later input allows another complete take without menu navigation or forcing an immediate manual REC-off. The owner described the workflow as intuitive and free of the earlier partial-overdub confusion.

**Scope of PASS:** limited owner-reported practical parameter-recording behavior. This does **not** certify Note/MIDI-REC hardware regressions, every target, all pattern lengths, stored-project recovery, sparse-note automation playback, or long-term/real-time reliability.

## 2. Owner-observed one-time startup LCD message

On the first post-update reboot the owner briefly observed a nearly full-width, two-line message containing something resembling `error` and `lo.`, lasting approximately one second. It did not recur on subsequent restarts. The exact literal text was **not captured**, and the owner reported normal apparent operation thereafter.

**Likely source-matched message:**
```text
glo.cfg error
Using defaults.
```

Source: `Core/Src/main.c` near lines 448–468. The two-line message has an exact `HAL_Delay(1000)`.

### Confirmed code-level compatibility regression introduced by this candidate

- The candidate appended `PAR_REC_MODE` after `PAR_PATTERN_MODE` in `Core/Inc/Parameters.h`, increasing `NUM_PARAMS` by exactly one compared with R19.
- `preset_loadGlobalFile()` reads `NUM_PARAMS - PAR_BEGINNING_OF_GLOBALS` bytes from each existing `glo.cfg`. A normal R19 global file is therefore one byte shorter than the candidate expects. A short read is treated as `FR_INT_ERR`.
- In the startup path, a failed `preset_loadGlobals()` invokes `preset_createGlobals()` when the normal project folder is present and no pending transaction is detected. This routine loads default globals and calls `preset_saveGlobals()` with `FA_CREATE_ALWAYS | FA_WRITE`, which replaces the existing global file.
- This precisely explains the nonrecurring message after the first successful candidate boot. **It is a strong source-supported explanation, not a photographed confirmation of the exact observed text.**
- `Core/Src/Preset/projectStorage.inc` also validates global-file size using `NUM_PARAMS - PAR_BEGINNING_OF_GLOBALS`, creating an additional legacy-project compatibility concern.

**Potential impact:** The global settings file of `/proj00` could already have been overwritten with defaults. Other projects or backups must not be assumed unchanged or corrupt without inspection. No actual file bytes have been reviewed.

**Immediate precaution:** Make a complete SD-card backup including `/proj00/glo.cfg` and any previous backup; avoid speculative re-flashing to reproduce the transient error and avoid saving/migrating important projects until the format is corrected and independently tested. A future fix should allocate the REC-mode menu state outside the persistent globals serialization (or otherwise maintain exact R19 global file compatibility), test both existing and candidate-created global files and handle migration without discarding settings. **No code change was authorized or executed during this analysis.**

## 3. REC mode label

The owner requests `mid` instead of `org` under `SHIFT+REC / mod`. Source inspection shows that `org` presently retains the *original combined* note + parameter event recording; its existing `seq_recordAutomation()` legacy branch still executes when `seq_recordActive` is set. A simple label rename would therefore be misleading. The desired `mid` mode should explicitly preserve note recording while suppressing legacy *live parameter recording*, and `par` should handle the new parameter take workflow. No implementation is claimed.

## 4. Automation playback limitation (BUG008B proposal)

The owner independently reproduced the original LXR-02 limitation: a sustained decaying voice triggered only at step 1 is not modulated by step 2–16 automation values, although direct live knob movement affects the decaying sound.

**Source-level explanation:** `seq_parseAutomationNodes()` is invoked from `seq_triggerVoice()`; automation is applied with `autoNode_updateValue()` and a direct MIDI CC handler call. The new BUG008 recording buffer saves one value per step, including silent steps, but old playback does not evaluate those values when no voice is triggered. No general linear interpolation of automation step points is present.

**Future separate design decision:** decouple step-synchronous automation playback from the note-trigger operation, retaining the selected lane's per-step parameter updates during decays; evaluate optional value smoothing/interpolation separately. Such playback changes can alter sound in existing projects and require explicit owner comparison/approval before implementation. No change has been made.

## 5. Remaining verification and preserved boundaries

- The existing hardware test candidate is NOT a stable accepted release.
- The approved R19 original and accepted R18 CLP sound references remain protected; no new IMG, flash operation, merge, release or bootloader change was carried out in this review.
- Confirm whether legacy `glo.cfg` content was changed by inspecting an owner-supplied before/after SD backup if available; do not infer historical global values from successful boot.
- Hardware MIDI note recording regression matrix **NOT REPORTED**.
- CPU/WCET, IRQ/DMA audio deadlines, stack high-water, sustained reliability **UNMEASURED**.
