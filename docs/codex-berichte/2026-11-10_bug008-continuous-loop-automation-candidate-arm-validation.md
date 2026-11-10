# BUG 008 — Continuous Loop Parameter Automation: Candidate ARM Validation Report

**Date:** 2026-11-10 (Europe/Vienna)  
**Status:** EXPERIMENTAL CANDIDATE — HOST TEST PASS / ARM RELEASE BUILD PASS / DEVICE NOT TESTED / NOT RELEASED  
**Authorized scope:** Implement the approved live parameter-recording workflow and run host and ARM validation, with no merge, release, flashing, bootloader changes, or alteration of accepted FX-CLP sound.  
**Execution:** Direct ChatGPT CHAT via read/write GitHub and GitHub Actions; no Codex or Work task was used.

## 1. Exact source and build identities

- Repository: `DocLXR-02/lxr-desktop-app`.
- Protected accepted baseline: **R19** source `638b6eaae5b1624637d20cbc11a4a3e0175edca2`, IMG SHA-256 `a1d53435ce922c4ef57ce595bf8c180b5225340780b6ad3241b6340af652e57b`. R18 CLP reference remains protected.
- Candidate branch: `fix/bug008-continuous-automation-r19-2026-11-10`.
- **Final validated candidate source SHA:** `2e3a60e251dffbceefc0ee4af258463f5d87126c`.
- Successful ARM/host workflow: [GitHub Actions run 42842043263](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42842043263), result **SUCCESS**.
- ARM toolchain: Arm GNU Toolchain **14.3.rel1 / GCC 14.3.1**, Release CMake preset, Ubuntu 24.04 runner. Toolchain archive checksum verified by CI.
- Uploaded CI evidence artifact ID: **13776637789**, `bug008-automation-candidate-evidence`. GitHub Actions 30-day retention, expiry **2026-12-10**. It includes raw host logs, ARM configure/build logs, ELF, MAP, sections, test-only IMG and SHA-256 manifest.
- Independently downloaded and verified evidence ZIP: **517,427 bytes**, SHA-256 `fab2985fa204895d27f9fc883e8f01cb29f4f84732eade113ee6ebf9c3807280`; ZIP CRC test passed.
- **TEST-ONLY candidate IMG:** `BUG008_TEST_NOT_FOR_FLASH.img`, 290,232 bytes, SHA-256 `6f5e5d2e6dd192b9385ddd0b8a95e14f4c06234b3de4f4e05f1a3878816c6f30`. ELF SHA-256 `bf6665e7de324660bc571675501e0489d10cc06ea717e6ffeff802ee2c23e832`; MAP SHA-256 `d5785b34e101823cf05421ccb75a3be780a1a459a2db1c5f6fe74b882d6d7512`.
- Candidate updater header independently checked: magic `LXRV2IMG`, payload length 290,216 bytes, checksum byte `0x21`, and `(sum(payload bytes) + checksum) mod 256 = 255`. This validates packaging, **not** safe hardware behavior.
- **No released or owner-installed BUG008 image exists.** The embedded original R19 startup identity is retained in this validation-only build; that does not imply this candidate is R19-approved.

## 2. Implemented user-facing behavior

- In `SHIFT + REC`, the existing `trk` selector chooses automation lane 1 or 2. A new `mod` option has `org` (default legacy behavior) and `par` (continuous parameter-loop recording).
- With `par` selected, REC arms a session that starts at the selected track's next step 1. The first actual parameter change selects the recording target (or must match the uniquely assigned existing target).
- Each step is filled from the held value. When the first input occurs after step 1, preceding steps are filled with that parameter's pre-change base value.
- A complete, changed loop atomically replaces **only** the selected lane's step targets/values. An idle loop produces no commit and preserves the previous completed take. The session stays active for additional takes until REC is manually turned off.
- Other parameter changes are not recorded after target selection. A mixed-parameter existing lane must be manually cleared; a target already present in the other lane is rejected. Manually programmed OFF/no-target steps are not counted as extra parameters.
- Real physical UI input is distinguished using private front-panel event statuses `0xE2`/`0xE3`; ordinary kit/parameter-refresh messages cannot start a take. External accepted MIDI CC parameter events continue to use their original input path. Unchanged values are ignored.
- In the new parameter mode, MIDI note capture is disabled; in `org` mode the original `seq_recordActive` path is used. The existing `seq_addNote` body, note timing helper, `MidiParser.c`, `buttonHandler.c`, and DSP/FX source files were independently compared or confirmed unchanged against the pinned baseline. **This is source-level evidence, not an end-to-end device MIDI regression test.**
- Pattern/track/slot/length change and transport stop discard an incomplete take. Old automation on the actively re-recorded lane is suppressed after the new take starts; the untouched lane continues to play.
- Existing step and project storage formats were not changed.

## 3. Host and ARM tests

Successful run `42842043263`:
- BUG008 capture helper, built with native GCC `-Wall -Wextra -Werror` and undefined/bounds sanitizers: **PASS**. Cases cover step-1 and late first change, full-lane replacement values, inactive repeat loops, third parameter ignored, last-step update, STOP/discard and unchanged-value handling.
- Previously established R18→R19 Delay mono regression: **PASS**, including 144 byte-level output comparisons and expected historical original-fault reproduction.
- USB-MIDI ownership/ring transport host regressions: **PASS** for included cases (not an attached-device USB test).
- Compressor mono saturation/parity host regressions: **PASS** for included cases.
- Selected CLP core isolation and historical reference comparison: **PASS**.
- Complete target ARM Release build and `arm-none-eabi-size -A`: **PASS**.
- No exact device audio recording, MIDI hardware, transport-clock jitter, rapid knob sweeps, or continuous-use stress test was performed. A complete end-to-end note-recording test matrix was **not** run.

The test matrix is intentionally bounded to the requested implementation and inherited relevant regressions; passing it is not a claim of whole-device correctness.

## 4. Matched static resource measurements

| Metric (bytes) | Approved R19 | BUG008 candidate | Delta |
|---|---:|---:|---:|
| FLASH `.text` | 152,024 | 153,984 | **+1,960** |
| ITCM `.itcm_text` | 15,208 | 15,272 | **+64** |
| DTCM `.dtcm + .dtcmz` | 107,420 | 107,424 | **+4** |
| General SRAM / linker RAM-region use | 274,660 | 274,748 | **+88** |
| Complete FLASH load payload | 288,168 | 290,216 | **+2,048** |
| IMG including 16-byte updater header | 288,184 | 290,232 | **+2,048** |

**Nominal post-build headroom:** ITCM `16,384 - 15,272 = 1,112 B`; DTCM `131,072 - 107,424 = 23,648 B` **before runtime stack**; general SRAM `393,216 - 274,748 = 118,468 B`. The small ITCM gap remains a key fit constraint. Source-level attribution of the entire FLASH change to any one function would be misleading because of linking/alignment/shared code.

**UNMEASURED:** MCU CPU/WCET, audio sample and DMA/IRQ deadline margins, worst-case nested interrupt stack, stack high-water, transient scratch allocation, potentiometer jitter frequency, multi-hour soak reliability and actual real-time musical response. Static FIT is not timing certification.

## 5. Failed attempts and corrections retained

- [42840055506](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42840055506): workflow checkout lacked a parent commit; preflight failed.
- [42840152634](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42840152634): BUG008 host PASS; inherited tests could not access historical Git commits with shallow checkout.
- [42840255969](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42840255969): regression tests PASS; ARM compiler found a duplicate sequencer command constant `0x35` (existing Euclidean rotation). New command moved to `0x3A`.
- [42841015178](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42841015178): whitespace preflight flagged source files' historical CRLF endings; workflow now uses `core.whitespace=cr-at-eol`.
- [42841089220](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42841089220): ARM/host PASS for the earlier intermediate candidate `90971365...`. Final review then fixed manual OFF/unchanged-input edge cases.
- [42842043263](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42842043263): **final candidate ARM/host SUCCESS**, independently checked against ZIP/ELF/MAP/IMG hashes. Previous failures are historical, not missing final validation.

## 6. Known restrictions and safety gates

1. Existing R19 automation playback applies step data through the voice-trigger path. Recording every step does **not** prove that automation on note-inactive steps is applied audibly. No playback semantics were changed in this BUG008 recording-only scope.
2. A new take is triggered by a real physical value change or accepted external parameter CC; spontaneous ±1 potentiometer changes remain an explicitly deferred input-hardware issue.
3. Existing multi-target/manual lanes are intentionally blocked from live overwrite; manually cleared lane is needed to select a new target. A rejected manual duplicate target is prevented on the backend but explicit frontend rejection feedback is not yet implemented.
4. Session behavior on pattern or track changes is conservative: incomplete work is discarded. Device UX notification for a blocked/aborted take remains unimplemented.
5. No custom release image, owner hearing acceptance, firmware installation, flash operation, native GitHub Release, merge or bootloader edit was performed.
6. The R19-approved and R18 musical-reference SHA identities remain protected. Future device testing and any revised release require separate owner authorization. If an audio difference is found, stop and compare against the accepted reference before altering the sound.

## 7. Reproduction and archive availability

The complete source is pinned by the candidate Git commit; the validation workflow is `.github/workflows/bug008-validation.yml`. Run the BUG008 host test with:

```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=undefined,bounds -fno-sanitize-recover=all \
  -ICore/Inc Core/Src/Soundengine/Sequencer/automationCapture.c tests/bug008/host.c \
  -o bug008-host
./bug008-host
```

The matched ARM build used `cmake --preset release` and `cmake --build --preset release --parallel 2` on the exact candidate SHA with Arm GNU Toolchain 14.3.rel1. The CI artifact retains logs and hashed build outputs until its stated expiry; it has **not** been asserted to be permanently archived in the owner's Library. No separate source ZIP was preserved in CI; the immutable source commit is directly accessible on GitHub.

**Final gate:** candidate build and static resource audit PASS, musical behavior and real-device input/audio timing **NOT TESTED**. Do not promote, install or distribute as accepted firmware without renewed explicit approval and device validation.
