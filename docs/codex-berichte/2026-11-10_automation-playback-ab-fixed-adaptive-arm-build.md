# Automation Playback A/B — Verified ARM Build and Experimental Listening Candidate

**Date:** 2026-11-10 (Europe/Vienna). **Status:** ARM/host PASS; owner physical listening and real-time measurements PENDING. Experimental sound-changing firmware, NOT A STABLE RELEASE.

## Identity and authorization

- **Protected approved R19 baseline:** source `638b6eaae5b1624637d20cbc11a4a3e0175edca2`; accepted R18 CLP sonic reference `41b364067bce0d8cde624c184933d9cb88e2725b`.
- **Previous hardware-tested multi-take capture source:** `2e3a60e251dffbceefc0ee4af258463f5d87126c` — source base for this work.
- **Final experimental source:** `3ca1256ba62ee5cf4411a68cc0df6c6769ec98b9`; branch `feature/automation-playback-ab-2026-11-10`.
- Owner explicitly requested to implement/test the selected step-synchronous and interpolated playback architecture; no merger, release, or physical flashing by the assistant occurred.
- Executed directly in CHAT using GitHub and CI; **no Codex or Work agent**. Agent quota/reset information was **UNKNOWN**, with no agent usage claimed.
- **Successful final [GitHub CI 42877760459](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42877760459)**; Arm GNU Toolchain 14.3.rel1 / GCC 14.3.1; Release CMake preset on Ubuntu 24.04.

## Implemented scope

- Automation playback now applies values on **every sequencer step**, even without note-on, while retaining the existing two lanes per voice and one destination per populated lane.
- User may audition `res=1x/2x/4x/8x` and `alg=fix/aut` on `SHIFT+REC`. The existing 24-phase step clock is used. Changes are enacted from the next step. Adaptive uses absolute difference between adjacent 7-bit values, an initial threshold of four units per update, and the selected maximum resolution.
- Intermediate values are generated with integer arithmetic and are not stored in a new pattern format. Repeated rounded intermediate values are suppressed. Only a changed parameter destination is reassigned; the last application at every step boundary is retained to avoid loss of normal live-knob override semantics.
- Original note trigger and MIDI note capture paths remain otherwise unchanged. `mod=mid` now performs MIDI note recording only, and `mod=par` keeps the owner-tested continuous multi-take parameter recording.
- New REC and A/B controls are runtime-only for persistence. `glo.cfg` writes use original R19 length; loading accepts original R19 and one-byte-expanded experimental global files. Project transaction validation accepts both lengths without changing the normal R19 file format.
- No FX DSP engine code, bootloader, accepted R19 branch, or accepted R18 CLP reference was modified. Corrupt track length is bounded to 64 before accessing the next step.

## Regression results

Final [CI run 42877760459](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42877760459) **SUCCESS**: original multi-take host capture, fixed/adaptive interpolation/bounds host test, project containment and whole-FatFs project transaction tests, delay mono, USB-MIDI host, compressor mono, accepted CLP core isolation, complete STM32 ARM Release build and section report. No real LXR-02 installation or listening test has been reported for this new binary. The storage tests are host/mock and do not reconstruct previously overwritten `glo.cfg` values.

## Matched ARM static resources (bytes)

| Metric | Accepted R19 | Previous multi-take candidate | Final A/B | Delta vs previous |
|---|---:|---:|---:|---:|
| FLASH `.text` | 152024 | 153984 | 154744 | **+760** |
| ITCM `.itcm_text` | 15208 | 15272 | 15304 | **+32** |
| DTCM | 107420 | 107424 | 107424 | **0** |
| General SRAM / linker RAM region | 274660 | 274748 | 274868 | **+120** |
| Complete flash payload | 288168 | 290216 | 291080 | **+864** |
| IMG including update header | 288184 | 290232 | 291096 | **+864** |

Nominal free ITCM **1080 B**, DTCM **23648 B** before actual runtime stack, and general SRAM **118348 B**. **CPU/WCET, IRQ/DMA deadlines, stack high-water, actual worst-case 14-lane/8x load, sustained reliability and hardware musical quality: UNMEASURED.** Section sizes are not CPU measurements.

## Hashes, artifact retention and delivered package

- CI artifact: `automation-ab-candidate-evidence` ID **13791443731**, retention to **2026-12-10**. Downloaded artifact ZIP 526194 B SHA-256 `ad6492ea84583cf68e8838e7927662dec2d4776924c3b1113c9f1985399cf0a9`; CRC PASS; includes exact source identity, test logs, compiler, ARM configuration and build logs, ELF/MAP, section report and IMG.
- **Experimental IMG** `LXR_V2_AUTO_AB_TEST.img`: 291096 bytes; SHA-256 **`45604028ba6b655422e33578a24b3d84baedd180f82914d3a9f38a8e5b5abcb9`**. Same bytes as the successful CI's `AUTOMATION_AB_TEST_NOT_FOR_FLASH.img`; only archive-member filename differs.
- Independent update header check: `LXRV2IMG`, payload length **291080 B**, checksum field **0x1a**; `(sum(payload) + checksum) mod 256 == 255`: PASS.
- **Owner test package** `LXR_V2_AUTO_AB_TEST.zip`: 208300 bytes, SHA-256 **`20bf5a8cff317122d532113422241a6e268826a87fe8f73cfb0971d0479eedcb`**, precisely one IMG; integrity verified.
- Separate English report/checklist package `LXR_Automation_AB_Test_Documents.zip`: 6619 bytes, SHA-256 `991e5f31d9b49b6cad1492c2e77e2d7a59be9912f58e6345c14a8e9d97984c57`, ZIP CRC verified. This package was delivered to the current chat; **permanent user Library archive NOT VERIFIED**.
- ELF SHA-256 `ebb2f70042f52acc9285fc2b7038ce2348199c0dbf1de8702df749f75c6a178a`; MAP SHA-256 `c9cec2d3e514a5f7ddcbeb755d1ee6b131ab85037eee718a39ada200ae4ab7b7`.
- Boot line remains **`LXR LAB R19`** as this is an unnumbered experimental branch; distinguish using `res` and `alg` menu entries.

## Device test and stop criteria

Back up the full SD card and retain the known-good R19 image. A previous experimental build may have already overwritten `PROJ00/glo.cfg` with defaults; the new firmware cannot restore lost previous values. Begin at low volume with one note on step 1, a long decay, and a continuous cutoff automation across 16 steps. A/B compare `1x/2x/4x/8x` at identical pattern settings with `fix`, then repeat with `aut`, before increasing voice/lane count. Check 16→1 wrap, 32/64 steps, MIDI recording, REC take retention and no boot `glo.cfg` error. Discrete/mode-valued parameters have not been tested for interpolation suitability. Stop on abnormal loudness, clicks/dropouts, crashes, timing issues, file/save errors or note recording regression, and restore the known-good R19/SD backup. The exact acoustic and physical test results require owner reporting.

**Disposition:** Experiment compiled and tests passed; no merge, force-push, native release, bootloader action or automatic device flashing. R19 remains the accepted final distribution.
