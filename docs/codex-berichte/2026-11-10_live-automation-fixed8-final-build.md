# Live Automation Recording — Fixed 8× Finalization ARM Build

**Date:** 2026-11-10 (Europe/Vienna)  
**Classification:** OWNER-SELECTED MUSICAL DESIGN + SOURCE/CI BUILD VALIDATION  
**Branch:** `feature/automation-fixed8-final-2026-11-10`  
**Final source SHA:** `e82fd4f3c239bfcb21400c3f94756f6fced02586`  
**Base SHA:** `3ca1256ba62ee5cf4411a68cc0df6c6769ec98b9` (owner-heard fixed/adaptive A/B test)  
**Previous owner-tested capture base:** `2e3a60e251dffbceefc0ee4af258463f5d87126c`  
**Protected approved R19 SHA:** `638b6eaae5b1624637d20cbc11a4a3e0175edca2`  
**Protected CLP R18 reference:** `41b364067bce0d8cde624c184933d9cb88e2725b`  

## Decision and permission

The owner explicitly chose **fixed 8 updates per sequencer step** as the permanent live automation playback design after hearing the fixed/adaptive A/B firmware on the physical LXR-02. The owner requested removal of the `res` and `alg` controls and complete removal of the adaptive decision path while preserving an English documentation note for potential future use. The owner considers the live-recording development work musically **closed**. This authorizes the narrow source changes, ARM validation and documentation, **not** a merge into `main`, public release, bootloader action, autonomous device flashing, or a statement that the newly built binary was owner-flashed.

## Source changes

- Preserve two fixed automation destinations per voice, recording across silent sequencer steps, multiple passes and idle auditioning. Record mode stays `mod=mid/par`; `trk` remains on `SHIFT+REC`.
- Remove `res` and `alg` entries, their text/enum/menu tables, front-panel protocol commands, handlers and sequencer runtime choice variables.
- Remove `chooseDivisions()`, the `adaptive` and `resolution` arguments and the adaptive threshold/step-division decision from `automationPlayback.c`.
- Hardcode **8** playback divisions and 3/24-phase spacing, retaining the same signed 7-bit integer interpolation and intermediate-value deduplication used by A/B `FIX 8x`. Step boundaries still explicitly apply each recorded value, even if equal to the previous step.
- Keep the previously corrected `glo.cfg` serialization boundary: R19-length global writes; read accepts legacy R19 and prior one-byte-expanded capture experiment formats. No project/pattern serialization change.
- Do not edit oscillator, filter, effects, CLP kernel, note trigger timing, bootloader, `main`, accepted R18/R19 or previous A/B source. The legacy A/B branch remains immutable evidence.
- Remove the obsolete A/B CI workflow **from this new branch only** and add `.github/workflows/automation-fixed8-validation.yml`; leave historical A/B commit and its CI untouched.

## Validation and evidence

**Final successful source-matched CI:** [GitHub Actions run 42894478378](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42894478378), single workflow job SUCCESS, exact HEAD `e82fd4f3c239bfcb21400c3f94756f6fced02586`; Arm GNU Toolchain **14.3.rel1 / GCC 14.3.1**, CMake Release.

- **FIX8 numerical playback:** PASS — exhaustive **16,384** pairs of 7-bit input Step start/end values, all 24 sequencer phase positions and duplicate-phase checks against the *old fixed-8 integer calculation*. The host test reported **106,792** distinct intermediate changes and exact per-event equivalence, including values, event suppression and next-step boundary. This verifies numerical equivalence; it does **not** measure sound or CPU timing.
- **Two-lane continuous recording capture fixture:** PASS, reusing the prior owner-tested multiple-take state-machine host test, no newly changed recording logic.
- **Source exclusion gate:** PASS — removed menu IDs, parser commands, runtime resolution/adaptive selectors and `chooseDivisions` are absent from `Core/Inc` and `Core/Src`.
- **Project storage and transactional FatFs regression suites:** PASS, including their existing fault/retry and file-validation fixtures. This is **not** a real SD-card `glo.cfg` migration test or a full-system end-to-end emulator.
- **Previous delay, USB-MIDI, compressor and FX-CLP R18 isolated comparison suites:** PASS. These are host-fixture regressions, not real hardware timing.
- **Full firmware ARM Release build:** PASS; CI archived image/ELF/MAP, source identity, per-check logs, and checksums.

## Matched static resource figures

| Static resource | Approved R19 | Owner-heard A/B | Final FIX8 | FIX8 minus A/B |
|---|---:|---:|---:|---:|
| FLASH `.text` | 152024 B | 154744 B | **154448 B** | **−296 B** |
| ITCM `.itcm_text` | 15208 B | 15304 B | **15304 B** | **0 B** |
| DTCM `.dtcm + .dtcmz` | 107420 B | 107424 B | **107424 B** | **0 B** |
| General SRAM region | 274660 B | 274868 B | **274860 B** | **−8 B** |
| Complete FLASH load payload | 288168 B | 291080 B | **290712 B** | **−368 B** |
| Updater IMG including 16-byte header | 288184 B | 291096 B | **290728 B** | **−368 B** |

Linker-derived nominal headroom after FIX8: ITCM **1080 B**, DTCM **23648 B** before runtime stack, general SRAM **118356 B**. The FLASH region reports 238412 B used of 992 KiB. These static counts **cannot establish** CPU utilization, audio/IRQ/DMA deadline margin, stack high-water or sustained reliability; each remains **UNMEASURED**.

## Exact binary and evidence identities

- Build output stored in CI as `AUTOMATION_FIXED8_TEST_NOT_FOR_FLASH.img`; identical bytes in the downloadable owner ZIP as `LXR_V2_FIXED8_TEST.img`.
- IMG size **290728 B**; SHA-256 `f8ce015318bcd806f21c0d6f41e468d15eaf78f09ac5b552992fe67e3606764e`.
- Firmware header: ASCII `LXRV2IMG`; encoded payload length **290712 B**; checksum byte at **offset 12** is `0x17`. Verified `(sum(payload) + 0x17) % 256 == 255`.
- Downloadable single-IMG ZIP `LXR_V2_FIXED8_TEST.zip`: **208030 B**, SHA-256 `97f58b5557bbd15630c28fe6d66c6e2e957f8cdf5462b1c793d5d29e2a1d9d26`, CRC PASS and IMG payload SHA parity.
- ELF SHA-256 `7970225e7d2391f5107e252866a3d369923936a9810ea333c01b4025d36adf0d`.
- MAP SHA-256 `787db2e08e0225f9b1a92cde884a999102a0cd40f73dbfcea1d0aed93ed97e05`.
- CI downloadable evidence artifact **13799587699** (`automation-fixed8-candidate-evidence`, ZIP **524986 B**, expires **2026-12-10**); independent download integrity and extracted manifest verified, ZIP SHA-256 `cf5b826f567329e49e35c74d4defa0df2aa8aee431c1581ba139be3bab6df3c8`. Durable GitHub source and CI workflow are preserved; CI binary retention has an expiry. Owner private Library preservation is **NOT VERIFIED**.
- The existing boot string remains `LXR LAB R19`; this finalization **is not** a new R19 distribution or R20 release.

## Owner observations and remaining gates

The owner played the A/B candidate on the real device at **120 BPM**, with a single Step-1 note, decay continuing to Step 16 and a rapid, large, bidirectional **COA** automation curve. They report **FIX 8x sounds continuous**; FIX 4x still has lightly audible steps; AUTO 8x sounded approximately like FIX 4x. Based on that comparison, the owner specifically selected FIX 8x and declared the live-recording change series complete. These are actual owner-hearing observations about the **preceding A/B binary**, not measurements of the new FIX8 binary.

The exact new FIX8 candidate has **NOT** been reflashed or retested on real hardware as part of this development task. Before promotion to an approved new firmware distribution, use a backed-up SD card and known-good R19 recovery image; verify first boot, `SHIFT+REC` showing only `trk/mod`, original repeated-take recording, long-decay silent-step playback, MIDI note recording, project reload and absence of unexpected loudness or SD/global-setting anomalies. No automatic release or merge is authorized.

CPU/WCET, IRQ/DMA timing deadlines, stack high-water and long-term audio reliability remain **UNMEASURED** despite the stable owner listening outcome.

## Reversible historical design

The adaptive alternative is documented separately in `docs/design/2026-11-10_adaptive-automation-retained-design.md` and remains recoverable from exact former A/B commit `3ca1256ba62ee5cf4411a68cc0df6c6769ec98b9`. Do not reinstate absent measured benefit and renewed owner consent.
