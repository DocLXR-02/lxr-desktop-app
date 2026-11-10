# Archived Adaptive Automation Strategy — Reference Only

**Recorded:** 2026-11-10 (Europe/Vienna)  
**Disposition:** ARCHIVED, NOT ACTIVE IN FIX8 FIRMWARE. Do not silently reactivate.  
**Last source containing the full A/B implementation:** `3ca1256ba62ee5cf4411a68cc0df6c6769ec98b9` on `feature/automation-playback-ab-2026-11-10` (preserved).  
**Final fixed-8 source:** `e82fd4f3c239bfcb21400c3f94756f6fced02586` on `feature/automation-fixed8-final-2026-11-10`.

## Context and owner decision

Both modes operate on the existing two fixed automation lanes per voice, without extra pattern data. The owner evaluated 120 BPM, one long-decay note on Step 1 lasting until Step 16, and a rapid up/down COA automation curve. Fixed 8 updates per Step sounded smooth; fixed 4x retained audible quantized modulation; adaptive 8x sounded similar to fixed 4x. Owner selected **FIX 8x** and considers the live recording/automation architecture musically closed.

The maximum theoretical events for 14 active lanes at 120 BPM and 8x = **896 parameter applications per second** before suppression of repeated intermediate values (step boundaries are still explicitly applied). This is a bounded event count, **not** a measured CPU cost. Measured WCET/IRQ/DMA/stack/long-run values are unavailable.

## Previously evaluated algorithm

In the original A/B source file `Core/Src/Soundengine/Sequencer/automationPlayback.c`, `chooseDivisions(from,to,resolution,adaptive)` accepted an allowed maximum of `1 << resolution` with `resolution = 0..3` (1x/2x/4x/8x).

In adaptive mode it started with one division and doubled 1→2→4→8 while `absolute_difference > 4 * divisions`, bounded by the user-selected maximum. Equivalently, at an 8x cap:

| Absolute start/target step difference | Selected divisions |
|---|---:|
| 0–4 | 1 |
| 5–8 | 2 |
| 9–16 | 4 |
| 17–127 | 8 |

An unchanged 0-delta lane produced no distinct *intermediate* updates after the step boundary. Fixed mode already suppressed identical successive interpolated values. The adaptive mode therefore saved real CC updates only when suppressing numerically different intermediate values, but it did **not** eliminate the sequencer's regular phase checks.

The original A/B implementation exposed `res=1x/2x/4x/8x`, `alg=fix/aut` in `SHIFT+REC`; this UI, the adaptive decision function, associated protocol and runtime configuration are **fully absent** from the final fixed-8 branch.

## What would justify reconsideration?

Only observed CPU/IRQ/deadline pressure with fully loaded 7-voice/2-lane playback should prompt reconsideration. First measure actual per-parameter CC-handler CPU time, worst-case 14-lane load, audio/DMA deadlines and stack high-water, rather than assuming interpolation dominates. Compare at identical parameter curves, controlled audio level and 120/160 BPM. An adaptive rule with threshold 2 or 1 was discussed but **NOT implemented or physically tested**; its sonic equivalence to FIX8 is unknown. Parameter-specific sensitivity might be valuable but is likewise an untested future hypothesis.

If authorized, branch from the exact accepted future FIX8 source; copy/inspect the archived algorithm from the pinned A/B commit; retain the current project/recording format; use HOST exact 7-bit boundary tests, target ARM Release resource report and the owner's A/B hearing test before any musical acceptance. Do not alter protected CLP R18 or approved R19.

## Evidence

- Original owner-heard A/B source and working code: [commit `3ca1256…`](https://github.com/DocLXR-02/lxr-desktop-app/tree/3ca1256ba62ee5cf4411a68cc0df6c6769ec98b9).
- Original A/B [successful ARM/host CI 42877760459](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42877760459).
- Final FIX8 [successful ARM/host CI 42894478378](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42894478378).
- Full final [build report](../codex-berichte/2026-11-10_live-automation-fixed8-final-build.md).
