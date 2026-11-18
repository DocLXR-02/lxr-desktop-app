# R23 — Shared FX Pool: ITCM parameter-handler relocation and ARM validation

**Date:** 2026-11-18 (Europe/Vienna)  
**Repository:** `DocLXR-02/lxr-desktop-app`  
**Task:** Owner-approved minimal ITCM optimization for experimental P17-B, limited to moving `FxBus_updateParameter()` and `FxBus_setParameter()` into normal Flash and validating with the existing CI workflow.  
**Disposition:** **HOST PASS / ARM BUILD PASS / STATIC ITCM FIT; experimental, NOT FOR FLASHING.**  
**Firmware branch:** `release/r23`  
**Before:** `d796b7da6d4f885d01895f6dec95eeb3b73e64d2`  
**After, exact tested source:** **`d0b49dff26b6fcf11e0367249d329a6682bfefa4`**  
**Protected R22 accepted source:** `0e37942250f79306627702061d851731bdfdbbc8`  
**Documentation report base:** `docs/lab-project-state` at `672891c23809e8a2014457f564c16732f970e4f9`.  
**Execution mode:** CHAT/GitHub/CI; no Codex, WORK or real-device execution.

## Authorization, constraints and input evidence

The owner explicitly approved the read-only R23 ITCM investigation's first implementation option: remove the ITCM placement from exactly two FX parameter functions, inspect the diff, and run the established host and ARM validation. No authorization for mixer redesign, FX dispatcher movement, DSP/audio algorithm changes, project/kit/menu edits, bootloader modifications, release, device flash or hardware audio tests.

Reused evidence: R22 accepted build and resource ledger; prior R23 host PASS and ARM ITCM link STOP report [44234450455](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44234450455); pre-change failed link reported ITCM **16,416 B (32 B overflow)**, DTCM **109,216 B**, SRAM **275,164 B**. R22 accepted ITCM **15,304 B** with **1,080 B** nominal headroom. CPU/WCET, IRQ/DMA deadlines, runtime stack high-water and long-term reliability remain **UNMEASURED**.

## Exactly one edited source file

[Commit `d0b49dff26b6fcf11e0367249d329a6682bfefa4`](https://github.com/DocLXR-02/lxr-desktop-app/commit/d0b49dff26b6fcf11e0367249d329a6682bfefa4) changes only `Core/Src/Soundengine/FxBus.c`, precisely two declaration lines:

```diff
-IN_ITCM_RAM void FxBus_updateParameter()
+void FxBus_updateParameter()
...
-IN_ITCM_RAM void FxBus_setParameter(uint8_t idx, float value)
+void FxBus_setParameter(uint8_t idx, float value)
```

Changed lines were terminated with LF rather than CRLF to pass the existing `git diff --check` gate; all function bodies, call sites, DSP algorithms and other firmware files remain byte-for-byte unchanged by this task. Independent GitHub comparison from pre-change to post-change confirms exactly one file, 2 additions and 2 deletions. `FxBus_processSelected()` remains explicitly ITCM-resident.

## Exact GitHub CI results

[Workflow run **44288230102**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44288230102), checked out **`d0b49dff26b6fcf11e0367249d329a6682bfefa4`**. Both jobs completed **SUCCESS**:

| CI job | Job ID | Result |
|---|---:|---|
| Host FX routing, DSP and storage regressions | [134746356872](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44288230102/job/134746356872) | **PASS** |
| ARM release compile, section and image identity | [134746356532](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44288230102/job/134746356532) | **PASS** |

Host: `tests/fx_pool_r23/run.py` passed **280,032 routing/mask cases**. Inherited production-sourced R18 Delay mono, Compressor mono, CLP core comparison, USB-MIDI, R21/R22 Voice/Drum copy, project storage and transaction regressions passed. These do **not** exercise R23's complete integrated multiple-FX audio mix, effect tail transitions or actual hardware audio. The CI workflow and test files themselves were unchanged.

ARM: pinned Arm GNU 14.3.rel1 toolchain checksum passed. CMake Release configuration and all 125 build steps completed, ELF linked, IMG generated in CI only. `arm-none-eabi-size -A`, output hashes, MAP and ELF were recorded.

### Static resource measurements

| Region/section | R22 accepted reference | R23 before (failed link) | R23 after (successful link) |
|---|---:|---:|---:|
| ITCM, occupied | 15,304 B | 16,416 B | **15,960 B** |
| ITCM capacity | 16,384 B | 16,384 B | 16,384 B |
| ITCM headroom | 1,080 B | -32 B | **424 B** |
| DTCM, linker region | 107,424 B | 109,216 B | **109,216 B** |
| General SRAM, linker region | 275,164 B | 275,164 B | **275,164 B** |

**Measured improvement within R23: 456 B ITCM.** R23 retains **656 B more ITCM usage than R22**; its headroom is **656 B smaller**. Successful R23 section report records `.itcm_text = 15,960 B`, `.text = 161,472 B` (R22 160,712 B, +760), `.rodata = 83,272 B` (unchanged), `.dtcm + .dtcmz = 109,212 B` (R22 107,424, +1,788), and general SRAM section subtotal 275,160 B (R22 275,164, -4). The linker region totals include alignment, explaining the four-byte difference between section subtotals and DTCM/SRAM region usage. R23 nominal DTCM headroom before stack is approximately **21,856 B region-wise** (131,072 - 109,216); this is **not** a measured safe stack reserve.

The successfully linked R23 MAP places `FxBus_updateParameter` in ordinary Flash `.text.FxBus_updateParameter` at `0x0802777c` (188 B) and `FxBus_setParameter` at `0x08027838` (196 B), rather than in ITCM. Their sum is 384 B; additional code-layout/veneer differences account for the net 456 B ITCM reduction. `FxBus_processSelected` remains in `.itcm_text` at `0x00003438` (276 B). The **456 B** is a matched-link delta, not a standalone parameter-body-size estimate.

### Build output identity and preservation

- CI-created `LXR_V2_V1.7.img`: **299,096 bytes**, CI-reported SHA-256 **`b3e4c32787dcf5a5906ae87c240962ef15ce322cebfa87355939c4ba601fb3f2`**. The IMG was **not included in the downloadable evidence artifact**, hence its bytes/hash have **not** been independently rehashed outside CI. It is not approved for owner installation.
- Preserved ELF: **405,428 bytes**, SHA-256 **`3371805e0576af85c2a445dadeafa5d57ec9e6b80ff6a01a2122af562b005bae`**; independently matched against the SHA-256 of the actual ELF bytes in downloaded CI artifact.
- Preserved MAP: **553,291 bytes**, SHA-256 **`8b2503c5bd798cba7ea3027ac4fb6a895a949093fd25bd9fd0b8291e579bfe23`**; independently matched against downloaded MAP bytes.
- [ARM validation artifact 14419554114](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44288230102/artifacts/14419554114), ZIP SHA-256 **`04d45c3a80b689f9667bccef5cd18ecab500b80ce34b0d3388ac80bb3616ae29`**, ZIP CRC PASS; expiry 2026-12-18. Contents include ELF, MAP, build/configure logs, sections, checksums and source SHA, but no IMG.
- [Host validation artifact 14420471768](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44288230102/artifacts/14420471768), ZIP SHA-256 **`73aba34a80726c63827a86c922de66d7c24822a56df3330d8f1791de21e10486`**, ZIP CRC PASS; expiry 2026-12-18.

The ARM compile emitted **six warnings** in `Core/Src/Soundengine/DSPAudio/mixer.c`: five existing R23 enum-comparison warnings around lines 43–45 and one misleading-indentation warning around line 459. These did not stop the build. They were **not corrected**, because the owner-approved task was expressly limited to the two FX parameter declarations. A future code-review task should separately assess the misleading-indentation warning and potential behavioral consequences.

## Boundaries and disposition

**Technical result: PASS for bounded memory relocation, host regression and ARM link.** The originally observed 32-byte ITCM overflow is resolved; reserve is nevertheless only **424 B** (2.59% of ITCM), not the desired ~1 KiB. No additional tuning was attempted because the initial agreed change succeeded in making the build fit, and additional mixer modifications require a separate approval.

No physical flash, listening, runtime performance or soak testing. CPU cycles, worst-case timing, IRQ/DMA deadlines, stack high-water and long-term reliability **UNMEASURED**. No complete production-integrated R23 multi-FX regression or delay-tail handling validation exists. A successful build and standalone host tests do **not** establish sound correctness or hardware safety. No stable candidate has been approved.

No changes were made to `main`, `release/r22`, R18 accepted sound, the bootloader, or a stable firmware image. R23 remains experimental and **NOT FOR FLASHING**.

**Next decision point:** Owner may authorize a new, narrowly scoped mixer-code optimization and/or integration-level FX test harness to increase ITCM reserve and characterize combined DSP behavior; prefer CHAT + CI for small edits, consider Codex only for extensive multi-module debugging. Do not automatically begin either follow-up.
