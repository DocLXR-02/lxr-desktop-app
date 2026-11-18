# R23 — Independent effect Dry/Wet and output controls, menu and CC2: verified experimental build

**Date:** 2026-11-18 (Europe/Vienna)  
**Status:** **HOST PASS / ARM RELEASE PASS / EXPERIMENTAL / NOT FOR FLASHING.**  
**Repository:** `DocLXR-02/lxr-desktop-app`  
**Authorized scope:** independently stored Dry/Wet and output routing for all four effect processors, reuse the existing FX menu and selected-effect CC2 messages, validate with host tests and ARM CI. Owner accepts that future Kit/Project format need not read older versions; **no storage format modification was approved or performed in this stage**.

## Source lineage and boundaries

- Development baseline: independent P1–P4 FX bank core `dc9538e0a858c9761c2590d411deba90e8fc64cf`.
- New experimental branch: `experiment/r23-fx-independent-controls-2026-11-18`.
- **Final source and tested build SHA:** `119f35759ac1c1b218ae94f40fc77ac61a410ceb`.
- Prior selected `release/r23` unchanged: `238389475359f11bf317b446e668e0c82333c231`.
- Stable R22 TEST03 unchanged: `0e37942250f79306627702061d851731bdfdbbc8`.
- Protected `main` unchanged: `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`.
- Accepted R18 CLP sound algorithm, bootloader, individual `Distortion.c`, `Ringmod.c`, `Compressor.c`, `Delay.c`, kit/project file readers/writers, and hardware were **NOT MODIFIED**.

### Changes

Seven files changed relative to the prior core branch: production `Core/Inc/Soundengine/FxBus.h`, `Core/Src/Soundengine/FxBus.c`, `Core/Src/Soundengine/MIDI/MidiParser.c`, `Core/Src/menu/menu.c`, `Core/Inc/menu/menu.h`; test `tests/r23_fx_banks/run.py`; new isolated CI workflow `.github/workflows/r23-fx-independent-controls.yml`.

The FX runtime state includes `bankDryWet[FX_PARAMETER_BANK_COUNT]` and `bankAudioRouting[FX_PARAMETER_BANK_COUNT]` alongside independently stored P1–P4. The FX type in `FxBus_processSelected()` selects its own output routing and Dry/Wet **regardless of which effect the front-panel editor currently selects**. The four original DSP processing bodies and their parameter mappings were not changed. The two old global `fxBus.dryWet` and `fxBus.audioRouting` remain **editor views**, saved and restored when switching the selected effect, alongside the existing `fxBus.parameter[4]` view.

The pre-existing `CC2_FX_ROUTING` and `CC2_FX_AMOUNT` handlers now call bounded setters for the currently selected bank, and `CC2_FX_TYPE` updates the visible menu values on FX page selection. The FX menu's P1–P4, route and wet fields are refreshed on page entry and selection changes. The independent setters allow editing an inactive processor's Wet/output bank without changing the editor-selected type.

**Six routing modes** means two stereo destination choices and four left/right mono channel choices across the existing outputs, **not six newly created physical output jacks**. No new dedicated per-effect MIDI CC/NRPN addresses have been assigned: the existing FX CC2 controls address the selected effect. Distinct target IDs for simultaneous independent external automation are future design work.

## Validation and first STOP retained

[GitHub Actions final CI **44333348672**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44333348672), checked out exact SHA `119f35759ac1c1b218ae94f40fc77ac61a410ceb`: **both host and ARM jobs SUCCESS**.

- Host job `134903297970`: extracted unchanged production function bodies (original `FxBus_init`, bank operations and `FxBus_processSelected`), GCC 64-bit host with observable DSP stubs, **246,615 assertion checks per sanitizer**, 4,096 deterministic selection/parameter/bank edits, and **264 processor/route/wet dispatch test cases** (4 × 6 × 11). **UBSan PASS / ASan PASS**. Extracted function source SHA-256 `f2b0a9e976312facf1291a5e1bd8314ea1f251c2352c1d41445c3001eb3102bf`.
- Existing shared FX pool selector: **280,032 routing/mask cases PASS**; inherited R18 mono Delay/Compressor, CLP reference, USB MIDI, R21/R22 copy and all applicable FatFs storage/transaction host regressions **PASS**.
- ARM job `134903297456`: GNU Arm Toolchain 14.3.rel1 and CMake Release compile/link/ELF/MAP/firmware-image generation **PASS**. Six nonblocking pre-existing `mixer.c` warnings (five enum comparisons, one misleading indentation) were not changed.
- The first [run **44333175448**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44333175448) **STOPPED BEFORE COMPILATION OR TESTS** at `git diff --check` on five new lines with CRLF endings. A narrowly scoped follow-up commit `119f357...` normalized these line endings. The initial STOP is retained as evidence; it is not a DSP failure.
- The exact source-extracted test C from the downloaded CI archive was **independently recompiled and run locally** with UBSan and ASan; both returned PASS with the same 246,615 checks.

### Matched static memory comparison

| Region | Prior parameter-bank core | New independent controls | Delta |
|---|---:|---:|---:|
| Flash `.text` | 161,728 B | **162,808 B** | **+1,080 B** |
| Flash `.rodata` | 83,272 B | 83,272 B | 0 |
| ITCM occupied | 13,968 B | **13,968 B** | **0 B** |
| Nominal ITCM free | 2,416 B | **2,416 B** | 0 |
| DTCM occupied | 109,300 B | **109,324 B** | **+24 B** |
| General SRAM occupied | 275,160 B | 275,160 B | 0 |

DTCM nominal free before runtime stack: **21,748 B**. CPU/WCET, IRQ/DMA deadline timing, stack high-water and long-term device reliability remain **UNMEASURED**. Flash size growth is measured static code, not a cycle estimate.

## Binary identities, original evidence and retention

- CI generated `LXR_V2_V1.7.img`: **298,440 B** according to final `stat`; CI-reported SHA-256 `26c07cf429de139ef9bb52c04612cc0703136257dd17372b4cb9a2211cb0f54e`. The IMG bytes were **not** included in the original ARM evidence artifact and were **not independently rehashed or distributed**. DO NOT FLASH.
- ELF `LXR_V2_STM32F765.elf`: **407,508 B**, SHA-256 `7c014909c798376d98613ef0f65907d86960b0e2dbaa050c4984736426a754ed`; checked against actual archived bytes.
- MAP `LXR_V2_STM32F765.map`: **554,571 B**, SHA-256 `19d2947bd6b3cea11335e98e7bfc3f25147f36a5956777f320e8b20eef4a77da`; checked against actual archived bytes.
- [HOST CI artifact 14440966674](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44333348672/artifacts/14440966674): SHA-256 `6188631f476dedb01f0e22d0ab65712d3409209df9e1b8b498379e3822bac8a9`, ZIP integrity PASS, 41,586 B.
- [ARM CI artifact 14439688548](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44333348672/artifacts/14439688548): SHA-256 `b96615d689a9d079be47cf963e86b36e711b535e7fa7334b8cd4a4d60a9c331c`, ZIP integrity PASS, 307,368 B.
- Original STOP artifact 14439742571 (HOST), SHA-256 `ae47f822a6cf292848b9022cb0cd0aec07cc87c365962ea1f2c5c2de835c80b3`, and STOP artifact 14439573303 (ARM), SHA-256 `e8fdf550e3f54aaf8e8d7c3f0f63e698e3724d893f225e76bcd2c1c73e7effd0`, preserved.
- CI evidence expiry expected **2026-12-18 UTC**. The four ZIPs were downloaded and their original byte SHA-256 and CRC checked; an English manifest and original archive package are retained separately.

## Incomplete integration and next gate

The new routing/wet state is runtime-only and initialized to zero per effect. The menu and existing selected-FX CC2 path are wired in source; menu/MIDI **live physical behavior was not simulated end-to-end** in the targeted bank harness (the static source guard and inherited MIDI regressions do not prove front-panel interaction). No new bank-aware Kit save/load, autosave, Morph, automation lane destination, or Project transaction format has been added. Old project compatibility is **NOT REQUIRED BY OWNER** for a separately authorized future format but existing files should not be silently converted or overwritten.

The real four-DSP integration experiment on the older `release/r23` baseline passed previously, but it is **not a complete real-DSP audio test of this new control branch**. The new dispatch test uses stubs for its four processors. Audio correctness and actual STM32 CPU time are still unverified at hardware level; the physical LXR-02 remains with the manufacturer for DATA-knob inspection.

**Recommendation:** Keep this experimental branch isolated and unflashed. Next owner decision is a versioned Kit storage/persistence design with safe rejection of unsupported versions, complete per-effect banks including subtype settings and explicit automation/MIDI addressing requirements, followed by host failure/recovery tests and a new ARM build. Do not merge into `release/r23` or any stable source without separate approval.
