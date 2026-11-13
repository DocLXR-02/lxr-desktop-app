# R20 REC Status, Conflict Protection and Display Timing — ARM Build Report

**Date:** 2026-11-13 (Europe/Vienna)  
**Classification:** Authorized source change; successful host regression and ARM Release CI; physical R20 validation pending  
**Repository:** `DocLXR-02/lxr-desktop-app`  
**Development branch:** `feature/r20-rec-ux-global-conflict-2026-11-13`  
**Exact approved FIX8 base:** `e82fd4f3c239bfcb21400c3f94756f6fced02586`  
**Final built R20 commit:** `b56e70ced8337476f1e302b61edcc3d718eb4e2f`  
**Successful source-matched CI:** https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43338109758  
**CI evidence artifact ID:** `13997413754` (`r20-rec-ui-build-evidence`; expires 2026-12-13)

## Authorization and protection

The owner explicitly authorized R20 implementation on a separate branch and a subsequent CI build. The approved scope is the previously agreed REC LCD messages, local/global conflict protection, correction of REC LED state when arming is rejected, and a common 1,500-ms notice duration. The accepted R18 FX-CLP sound, R19 stable distribution, FIX8 interpolation/reference and `main` have not been edited. No merge, hardware flash, bootloader change or stable R20 release was performed.

## Implemented behavior

- REC overview: the selected automation lane is on row 1 (`REC trk1 OFF` or `REC trk2 FRQ`); the sibling lane is on row 2 (`    trk2 COA` or `    trk1 COA`). The existing target is named when known; `MUL` indicates mixed legacy targets on the sibling lane.
- Local sibling-lane duplicate: `PARAM BLOCKED!` / `COA on trk2` (parameter and lane vary).
- Attempt to record a different target on an already bound lane: `PARAM BLOCKED!` / `trk1: COA ONLY` (dynamic).
- Legacy multi-target lane: `PARAM BLOCKED!` / `trk1 MULTI PARAM`. The lane is not overwritten; REC arming is rejected and its LED is cleared.
- Global duplicate: `PARAM BLOCKED!` / `P4 on V5 trk1` (dynamic voice/lane). The search covers the active pattern, all seven voice tracks, both lanes, and up to 64 steps per lane.
- Global conflict coverage is limited to the 12 owner-confirmed live-recordable targets: global sample-rate reduction; FX Type, Routing, Dry/Wet; Distortion Type; FX P1–P4; Compressor Ratio; Ring Wave; Delay Type. Reverb Type is excluded because its effect is disabled.
- Existing global duplicates are preserved and may be edited in place. The guard prevents creating additional duplicates; it never automatically rewrites existing pattern data. Existing project and pattern serialization remain unchanged.
- All new REC notices and existing Save Kit, Save Pattern, Load Kit and Load Morph Kit `success!` notices use a shared `DISPLAY_NOTICE_DURATION_MS = 1500u`, measured only with the unsigned difference of `HAL_GetTick()` values. No blocking display delay is used. Repeated rejected knob moves re-trigger the notice.
- The test image startup label is `LXR LAB R20`.

## Source and test scope

Production files changed: `Core/Src/menu/menu.c`, `Core/Inc/menu/menu.h`, `Core/Src/Soundengine/Sequencer/sequencer.c`, `Core/Src/buttonHandler.c`, `Core/Inc/buttonHandler.h`, `Core/Src/main.c`. New validation files: `.github/workflows/automation-r20-validation.yml` and `tests/r20_ui/run.py`.

The dedicated R20 host test extracts and compiles the actual conflict-search functions from the final sequencer source with bounded synthetic pattern storage. It checks all 12 global target IDs, collision location, local sibling-lane rejection, single-target lane locking, rejection of false cross-voice collisions for voice-local parameters, preservation of an existing global duplicate, and multi-target identification. Static checks verify notice length, timer selection, startup marker and necessary call paths. These tests are not an LCD/device emulator.

## Source-matched CI results

**PASS** — Workflow `43338109758`, one successful job at exact head `b56e70ced8337476f1e302b61edcc3d718eb4e2f`, Arm GNU Toolchain 14.3.Rel1 / GCC 14.3.1, CMake Release.

- Recording capture host regression: PASS.
- FIX8 numeric parity: PASS, all 16,384 possible 7-bit endpoint pairs; 106,792 distinct intermediate changes matched the accepted algorithm.
- R20 actual-source conflict host regression and LCD timing assertions: PASS.
- Adaptive-feature source exclusion checks: PASS.
- Existing global/project storage and real-FatFs transaction/failure-recovery regression suites: PASS.
- R19/R18 isolated Delay, USB-MIDI, Compressor and accepted FX-CLP comparison suites: PASS.
- Full ARM Release compilation and firmware image generation: PASS; final `build.log` contains no compiler warnings or errors.

An initial workflow failed because its new test extracted a function declaration rather than a definition; the test extractor was corrected. The first passing ARM build had two compiler truncation warnings in newly added LCD format strings; the final source bounds voice/track digits and the final build contains zero such warnings. These earlier runs are historical diagnostic evidence, not the delivered build.

## Final binary, distribution and evidence

| Artifact | Exact result |
|---|---|
| CI image | `LXR_V2_R20_CANDIDATE_NOT_FOR_FLASH.img` |
| Delivered test image | `LXR_V2_R20_TEST.img` (identical bytes) |
| Image size | **292,400 bytes** |
| Image SHA-256 | `af6f784f674a5cd77d8096b2b78d72ba638496a1c7ec65ade632423b84039b94` |
| Download ZIP | `LXR_V2_R20_TEST.zip`, exactly one image, **209,295 bytes** |
| Download ZIP SHA-256 | `b0d717bde3e6288023676ce003b55ebce282c2451deed5332da8104dd594c4b0` |
| ELF SHA-256 | `fe454c7597659dce89192d2fa20ce4d55767010c2ff910eaeb34f184b35867ee` |
| MAP SHA-256 | `d5dc06a4af56c282b343e624f4bc6ae929fefcbd718eab21b50802947c9bd1f0` |
| Original CI evidence ZIP SHA-256 | `3b5a12ac6b44b08b5936d9ad2f9ee70a678993ec5ccb3cb92db5767410529dec` |

The image header starts with `LXRV2IMG`, the encoded payload is **292,384 bytes** matching actual payload length, the checksum byte at offset 12 is `0xE1`, and `(sum(payload) + 0xE1) % 256 == 255`. The `LXR LAB R20` startup string is present. The distributed image equals the source-matched CI image byte-for-byte. The ZIP has one entry and passes CRC/integrity testing. User-side filename changes are unnecessary.

## Static section report and unmeasured limits

| Section | R20 bytes |
|---|---:|
| `.text` | 155,960 |
| `.itcm_text` | 15,304 |
| `.dtcm` | 36,996 |
| `.dtcmz` | 70,424 |
| `.data` | 852 |
| `.bss` | 273,508 |

These values are linker/build observations, not measured CPU time or remaining runtime stack. **CPU/WCET, IRQ/DMA deadlines, stack high-water, audio timing, long-term stability and actual LCD timing on the device remain UNMEASURED.** Compared with the fixed-8 reference, the firmware updater image has grown by **1,672 bytes**; this is a file-size difference, not an estimate of CPU cost.

## Owner device-test boundary and next action

The owner previously reported actual on-device recording and persistence after reboot for all 12 global parameter targets on the earlier FIX8 candidate. That is relevant input evidence, but **no owner installation or physical validation of the new R20 image has yet been reported**. The next authorized step must be an owner-directed, backed-up device test: verify both REC status layouts, all blocked-message classes, repeated bad-knob alerts, blocked REC LED behavior, local/global conflict prevention, preservation of legacy multi-target lanes and existing global duplicates, 1.5-second success/loading notices, save/reboot persistence, unchanged MIDI-note mode and FIX8 audio behavior. Stop if projects, volume, sound, or storage behave unexpectedly. Preserve R19 recovery media and SD backup. Do not promote R20 to a stable release without explicit owner acceptance.

**Record status:** Verified CI-built R20 **test candidate only**; development branch intentionally unmerged; report is evidence, not hardware acceptance.