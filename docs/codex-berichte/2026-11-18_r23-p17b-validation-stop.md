# R23 P17-B — Shared FX Pool validation STOP report

**Date:** 2026-11-18 UTC  
**Repository:** `DocLXR-02/lxr-desktop-app`  
**Task:** Authorized host regressions and ARM Release compilation, resource comparison, and evidence report for the experimental P17-B Shared FX Pool.  
**Disposition:** **STOP — HOST PASS / ARM FAIL; no complete ARM image or resource comparison.**  
**Report branch:** `reports/r23-p17b-validation-stop-2026-11-18` (documentation only).

## Authorization and immutable identities

- Owner explicitly approved the R23 validation workflow, host/audio regressions, ARM compilation, static resource comparison and a separate English report. This approval does not authorize device flashing, release, hardware testing, unrelated production refactoring or CLP sound changes.
- Protected stable source: `0e37942250f79306627702061d851731bdfdbbc8` (`release/r22`).
- R23 validation source: **`8a31e690b28338bf00c66d382bb62afc10076e18`** (`release/r23`); immediate predecessor P17-B prototype `778014358301a2d723e49286e3aeecba20e05b57`.
- The only new source-tree change for this validation was `.github/workflows/r23-p17b-validation.yml` at commit `8a31e690b28338bf00c66d382bb62afc10076e18`. Production DSP and R23 prototype source files were not modified.
- Canonical governance documentation branch at report creation: `docs/lab-project-state` pinned `672891c23809e8a2014457f564c16732f970e4f9`. The approved corrected `AGENTS.md` is already present on `release/r23`.
- Prior evidence: R22 TEST03 CI [43707621969](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43707621969), accepted build report and current project resource ledger; the P17-B prototype ZIP supplied in the previous task. Historical failing tests remain preserved.

## Executed GitHub Actions run

**Run:** [44233493705](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44233493705), source `8a31e690b28338bf00c66d382bb62afc10076e18`, 2026-11-18 03:53 UTC, **overall FAILURE**.

| CI job | Job ID | Actual result |
|---|---:|---|
| Host FX routing, DSP and storage regressions | [134563942140](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44233493705/job/134563942140) | **PASS** |
| ARM Release compile, section and image identity | [134563941848](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44233493705/job/134563941848) | **FAIL** during compilation |

### Host tests actually run

- `python3 tests/fx_pool_r23/run.py`: **PASS**, 280,032 routing and effect-mask cases, including six voices sharing one Delay selection and three simultaneously selected effect types. This validates the standalone production header's routing selection, **not** complete audio processing.
- `python3 tests/r18_delay_mono/run.py build/r23-delay-host`: **PASS**, including 144 byte-level normal/ping-pong mono routing comparisons and an intentionally reproduced original failure.
- `python3 tests/r18_compressor_mono/run.py build/r23-compressor-host`: **PASS**, 144 normal-domain comparisons plus documented expected-failure/saturation cases.
- `python3 tests/fx_clip_r18/compare.py`: **PASS**, 135 unchanged-core state snapshots against the pinned R14 baseline; this does not establish live sound or R23 multi-effect behavior.
- `python3 tests/r18_usb_midi/run.py build/r23-usb-host`: **PASS**, source-based USB-MIDI regression.
- `python3 tests/r21_voice_copy/run.py`, `python3 tests/r22_drum_copy/run.py`, `python3 tests/project_storage/run.py`, `python3 tests/project_transaction/run.py`: **PASS**, inherited voice-copy, Kit/Project storage and transaction regressions.

**Coverage boundary:** There is not yet a production-integrated host audio comparison of simultaneous R23 Delay+Ring+CLP/Compressor routing, output summation and effect tails. The pre-existing FX DSP files are unchanged from R22, but this is not proof that the new combined routing works correctly.

### ARM compile: exact STOP finding

The pinned Arm GNU **14.3.rel1** toolchain was downloaded and its archive checksum verified by CI. `cmake --preset release` completed; `cmake --build --preset release --parallel 2` stopped with exit code 1 in compilation object `Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj`.

Decisive compiler diagnostics:

```text
Core/Src/buttonHandler.c:33:16: error: storage class specified for parameter 'lastActiveSubPage'
   33 | static uint8_t lastActiveSubPage = 0;
      |                ^~~~~~~~~~~~~~~~~
Core/Src/buttonHandler.c:33:34: error: expected ';', ',' or ')' before '=' token
```

**Root cause confirmed by source inspection:** the R23 `Core/Inc/Soundengine/FxBus.h` contains an incomplete new function prototype:

```c
void FxBus_processSelected(uint8_t fxType, int16_t *fxBuffer,
void FxBus_process(int16_t *fxBuffer, int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2, uint32_t size);
```

The missing remaining arguments and closing `);` let parsing continue into the next declaration and later translation-unit text. Thus the reported error location in `buttonHandler.c` is secondary. The `buttonHandler.c` blob is byte-identical on R22 and R23, SHA-1 `b4669178de810c50273b98cf2b6a3c4439bb9d73`; the incomplete prototype was introduced in R23. The first observed compiler failure does **not** prove no other errors remain.

**Not executed or produced:** successful ELF/IMG/MAP, final image hash, section-size report, matched R22/R23 static resource delta, full firmware audio execution, or hardware validation. Resource estimates from architecture design remain estimates, not linker measurements.

## Preserved evidence

- [Host evidence ZIP artifact 14395897108](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44233493705/artifacts/14395897108) — GitHub SHA-256 `a245461078f148f8a787301e8b36eea28e12fa3103174425aba7fccf5b7b0021`, 6,127 B, expires 2026-12-18.
- [ARM failure evidence ZIP artifact 14395219307](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44233493705/artifacts/14395219307) — GitHub SHA-256 `7f04de6758003f197c9c4fe7ac35fda8e83c5989c4c6e99a6330115c08543d33`, 3,030 B, expires 2026-12-18.
- Original unabridged GitHub job logs at the linked job URLs. The ARM artifact contains the partial build log and configuration metadata; no flashable image was distributed.
- The CI workflow itself is a reproducible executable record of the exact steps. The upstream tests and firmware reference are retained at their immutable commit SHAs.

## Protected boundaries, risk and next decision

No changes were made to `main`, protected R18/R22 references, DSP algorithms, audio settings, Kit/Project formats, bootloader, or hardware. No device flash, release, merge or physical test. CPU cycles/WCET, IRQ/DMA deadlines, runtime stack high-water and long-term audio reliability remain **UNMEASURED**.

**STOP criterion met:** production-source compile failure. A correction to `Core/Inc/Soundengine/FxBus.h` needs narrowly scoped owner authorization before implementation. Proposed declaration to review:

```c
void FxBus_processSelected(uint8_t fxType, int16_t *fxBuffer,
                           int16_t* outL, int16_t* outR,
                           int16_t* outL2, int16_t* outR2,
                           uint32_t size);
```

After approval, fix only the declaration on `release/r23`, independently inspect the diff and rerun the same host/ARM validation. If another source error, static region overflow, regression failure or new audio defect appears, stop and report rather than silently widening scope. A successful future build still does not establish instrument-side timing or sonic acceptance. Do not deliver an IMG as stable firmware until separately approved.

**Final status:** Host PASS; full ARM build FAIL; R23 experimental, **NOT FOR FLASHING**. No Codex or WORK used.
