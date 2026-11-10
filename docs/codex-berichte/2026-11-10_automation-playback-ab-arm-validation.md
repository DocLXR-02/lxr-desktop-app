# Automation Playback A/B — ARM Build and Validation Record

**Date:** 2026-11-10 (Europe/Vienna)  
**Status:** EXPERIMENTAL SOUND-CHANGING FIRMWARE; CI AND ARM BUILD PASS; DEVICE TEST AND MUSICAL ACCEPTANCE PENDING  
**Workflow:** Direct ChatGPT CHAT + GitHub Actions. No Codex/Work used.

## 1. Source identity and authorization

- Exact candidate source: `3ca1256ba62ee5cf4411a68cc0df6c6769ec98b9` on `feature/automation-playback-ab-2026-11-10`.
- Owner-tested previous capture baseline: `2e3a60e251dffbceefc0ee4af258463f5d87126c`.
- Owner-accepted R19 baseline (protected): `638b6eaae5b1624637d20cbc11a4a3e0175edca2`.
- Accepted R18 CLP sonic source (protected): `41b364067bce0d8cde624c184933d9cb88e2725b`.
- Owner authorized an experimental implementation and evaluation of 1x/2x/4x/8x step-independent automation with fixed/adaptive interpolation. **No merge, flash by the assistant, bootloader operation, public release or musical acceptance.**

## 2. Implemented scope

- Separate parameter automation playback from note-on: stored automation is evaluated at every applicable sequencer step, including steps with no note triggers, for a maximum of two automation lanes per voice.
- Lane target remains constrained to a single parameter within a pattern; manual conflicting target assignments are rejected. The target is only reselected/reset when an actual destination change is encountered at a step boundary.
- Each step evaluates the stored value and the next step's value. If both targets match, calculate linear intermediate values using 24 pre-existing sequencer phases; if a target/OFF boundary intervenes, do not interpolate across that boundary.
- The `SHIFT+REC` page now provides `trk`, `mod=mid/par`, `res=1x/2x/4x/8x`, and `alg=fix/aut`. Settings are runtime-only and quality changes affect the next step. `mid` retains the original note-recording path but disables legacy live recording of parameter locks. `par` retains the owner-tested continuous multi-take capture.
- Fixed interpolation uses 1, 2, 4 or 8 evenly spaced evaluation points per step. Adaptive mode chooses 1/2/4/8 bounded by `res` and an initial target of at most 4 units of 7-bit parameter change per update. Redundant *intermediate* values are suppressed; each step-boundary value is deliberately reasserted.
- The `glo.cfg` writer restores original R19-length global files by excluding the new UI-only parameters. The reader and transactional project-size validator accept both the R19 length and the prior one-byte-expanded experimental capture format; this does not restore globals overwritten by the older test candidate.
- Pattern storage, MIDI note timing/recording function `seq_addNote()`, effect DSP files and bootloader remain unchanged.
- A post-build hardening commit bounds corrupt track lengths to 64 and removes the modulo operation in step-next-index selection. This **exact final commit** was rebuilt and retested.

## 3. CI, tests and limitations

**Final successful ARM/host CI:** [run 42877760459](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42877760459), attempt 1, all jobs/steps SUCCESS, source pinned to `3ca1256ba62ee5cf4411a68cc0df6c6769ec98b9`. First development CI at [42877037302](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42877037302) also passed, but is not the final source.

- Toolchain: Arm GNU Toolchain 14.3.rel1 (GCC 14.3.1), CMake Release preset, Ubuntu 24.04.
- Native host capture tests with UBSan/bounds: PASS, including multi-take, idle loops, late touch, partial-take discard.
- Native interpolation tests with UBSan/bounds: PASS for step boundaries, 1x/2x/4x/8x fixed/adaptive, deduplication, seven-bit extrema, invalid/off destinations and wrapping.
- Whole-production project storage and FatFs transaction regression fixtures: PASS; these do not independently execute a full old/new `glo.cfg` file migration on a physical SD card.
- Prior delay/USB-MIDI/compressor host regressions: PASS; previously accepted FX-CLP core comparison: PASS.
- Full target ARM Release build and target section size output: PASS.
- The existing note-recording function body was independently confirmed unchanged relative to the owner-tested capture candidate. No dedicated physical MIDI note-input test was performed.

**Crucial limits:** CI does **not** prove safe live MCU worst-case timing, USB hardware behavior, all discrete synth-parameter semantics, external sync/shuffle phase quality, MIDI/knob priority interactions, real SD-card migration, or audible interpolation quality. All of these await owner device observations; CPU/WCET, IRQ/DMA deadlines, stack high-water and long-run reliability remain **UNMEASURED**.

## 4. Static resource comparison

| Static resource (bytes) | Approved R19 | Owner-tested capture candidate | Final A/B candidate | A/B minus capture |
|---|---:|---:|---:|---:|
| FLASH `.text` | 152024 | 153984 | **154744** | **+760** |
| ITCM `.itcm_text` | 15208 | 15272 | **15304** | **+32** |
| DTCM (`.dtcm + .dtcmz`) | 107420 | 107424 | **107424** | **0** |
| General SRAM region use | 274660 | 274748 | **274868** | **+120** |
| Complete FLASH load payload | 288168 | 290216 | **291080** | **+864** |
| IMG incl. 16-byte updater header | 288184 | 290232 | **291096** | **+864** |

Nominal remaining gaps: **ITCM 1080 B**, **DTCM 23648 B before runtime stack**, **SRAM 118348 B**. These are linker-derived values, not real-time, stack or long-use measurements.

## 5. Verified image and retained evidence

- CI test image: `AUTOMATION_AB_TEST_NOT_FOR_FLASH.img`. The owner test package renames the identical bytes to `LXR_V2_AUTO_AB_TEST.img`.
- Exact image size: **291096 B**. SHA-256: `45604028ba6b655422e33578a24b3d84baedd180f82914d3a9f38a8e5b5abcb9`.
- Header verified: `LXRV2IMG`, payload **291080 B**, checksum byte `0x1a`; `(sum(payload)+checksum) % 256 == 255`. ZIP CRC and CI-to-package IMG hash equivalence independently checked.
- Image-only owner test ZIP: `LXR_V2_AUTO_AB_TEST.zip`, **208300 B**, SHA-256 `20bf5a8cff317122d532113422241a6e268826a87fe8f73cfb0971d0479eedcb`.
- ELF SHA-256: `ebb2f70042f52acc9285fc2b7038ce2348199c0dbf1de8702df749f75c6a178a`.
- MAP SHA-256: `c9cec2d3e514a5f7ddcbeb755d1ee6b131ab85037eee718a39ada200ae4ab7b7`.
- CI evidence artifact: `automation-ab-candidate-evidence` ID **13791443731**, expiry 2026-12-10; independently downloaded ZIP **526194 B**, SHA-256 `ad6492ea84583cf68e8838e7927662dec2d4776924c3b1113c9f1985399cf0a9`. CRC PASS; contains image, ELF, MAP, source SHA, toolchain, raw host/ARM logs and checksums. Long-term Library preservation: NOT VERIFIED.
- Full source and validation workflow are reproducible at the exact source commit; workflow `.github/workflows/automation-playback-validation.yml`.
- Original startup display remains `LXR LAB R19`; the test candidate is identified by `res` and `alg` under `SHIFT+REC`, not by the boot string.

## 6. Owner testing and stop criteria

Use a backed-up SD card and disposable project, initially one long-decay voice with just Step 1 active, plus one filter automation curve covering a 16-step loop. At conservative audio levels compare identical saved material at `res=1x/2x/4x/8x`, first `alg=fix` then `alg=aut`, while monitoring audible stepping, unexpected transients or dropouts. Then check both automation lanes, different loop lengths, transport/start-stop, MIDI note recording and the previous multi-take behavior. Do not treat a subjective hearing comparison as measured WCET or CPU reserve.

**STOP** on abnormal loudness, audio interruption, restart/lockup, repeat `glo.cfg` errors, SD/project anomalies or MIDI regression. Keep the exact accepted R19 recovery image and owner backup. Musical changes to accepted CLP sound require renewed owner evaluation/approval; no retuning has been performed.

## 7. Decision

**Complete as experimental source + CI/ARM-validated owner A/B candidate, not as an approved replacement for R19.** No hardware installation of this particular candidate is claimed, and no release/merge was performed. Subsequent work must use the exact pinned SHA and real owner observations.
