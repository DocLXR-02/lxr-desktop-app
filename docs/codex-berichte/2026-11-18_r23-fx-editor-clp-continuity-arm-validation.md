# LXR Firmware Lab — R23 FX Editor / CLP Continuity: ARM Validation Build Report

**Date:** 2026-11-18 (Europe/Vienna)  
**Result:** PASS — EXPERIMENTAL SOURCE / HOST / ARM VALIDATION ONLY  
**Status:** NOT APPROVED FOR FLASHING, AUDIO ACCEPTANCE, MERGE OR RELEASE  
**Task scope:** Remove CLP runtime reset on FX editor-only selection; preserve real Kit-load reset paths and the accepted R18 DSP algorithms.

## Pinned identity and authorization

- **Repository:** `DocLXR-02/lxr-desktop-app`
- **Exact parent/source SHA:** `5e99e4f57499e0946c90546d5b1fc6f1867bebd6` (`experiment/r23-midi-hardening-2026-11-18`).
- **Branch:** `experiment/r23-fx-editor-clp-continuity-2026-11-18`.
- **Exact validated HEAD:** `f8fc6fba3b7599095bd073460760b47ad49b173c`.
- **Owner direction:** Continue with the previously identified editor-only CLP reset correction, preserving the Delay freeze/resume policy, avoiding unneeded DSP work, and maintaining R18 musical behavior. Authorization covered this isolated code/test/ARM validation task, not flashing, final sound acceptance, or release.
- **Accepted R18 CLP source:** `41b364067bce0d8cde624c184933d9cb88e2725b` (protected).
- **Owner-accepted stable R22:** `0e37942250f79306627702061d851731bdfdbbc8` (unchanged).
- **Protected upstream `main`:** `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` (unchanged).
- **Validation CI:** [GitHub Actions run 44390775828](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44390775828), attempt 1, SUCCESS. Both host and ARM validation jobs PASS; the exact run SHA equals the source SHA above.

## Production code change and intent

Only `Core/Src/Soundengine/MIDI/MidiParser.c` changes in production: **one added comment and five removed lines** in `case CC2_FX_TYPE`. The prior conditional `FxBus_prepareFxChange()` invocation is removed from the editor-selection branch; the FX type assignment, `FxBus_updateParameter()` bank behavior, visible menu synchronization and MIDI input-domain guard are retained verbatim. The original source still contains `FxBus_prepareFxChange()` in `Core/Src/Soundengine/FxBus.c`, and the two genuine Kit-parameter broadcast/load pathways in `Core/Src/Preset/presetManager.c` still explicitly invoke it.

Causal distinction: switching the selected editor should not clear `Distortion.clipStateActive` or the regenerative CLP histories while a voice is routed directly to CLP. Real Kit loading remains an intentional CLP re-entry reset. The correction deliberately changes editor-navigation behavior but **does not alter** any of the four DSP implementation bodies, feedback coefficients, wet mixing, fader control, output saturation, sample rendering, Delay state policy, Kit encoding, bootloader, or MIDI hardening.

**Changed paths, compared to exact parent:**

1. `Core/Src/Soundengine/MIDI/MidiParser.c` — 1 line added, 5 removed.
2. `tests/r23_clp_editor_continuity/run.py` — source-extracted host continuity verification (new).
3. `.github/workflows/r23-clp-editor-continuity.yml` — isolated pinned host and ARM validation (new).

No previous experimental branch was overwritten; the current branch is three commits ahead of the exact parent SHA. No merge, release, device flash, or hardware action occurred.

## Host validation — actual DSP and bytewise continuity

- Host job **`135107489919` — SUCCESS** (Ubuntu 24.04).
- Inherited real four-FX production-source integration generates the actual host translation unit from immutable production code (`tests/r23_final_fx_integration/run.py`). The new continuation test `tests/r23_clp_editor_continuity/run.py` then extracts the **actual production MIDI `CC2_FX_TYPE` case and `FxBus_prepareFxChange`** into that translation unit, keeping original Distortion/CLP, Ringmod, Compressor and Delay DSP bodies. It compares the CLP memory state and both DAC output buffers for **44 paired 64-frame blocks per sanitizer**: with six voice routes to CLP, editor selection off and on the FX page, and sequential switches through non-CLP editors and back. Outputs and complete `MasterDistortion` snapshots are bit-identical to a no-editor-switch control. Both UBSan and ASan **PASS**.
- The test separately checks that an explicit Kit-load-style `FxBus_prepareFxChange()` **still clears** `clipStateActive` and that both real preset-broadcast source paths retain the call.
- The inherited real-DSP integration also passes **5,832 six-voice scenes / 1,296 all-four-FX scenes per sanitizer**, plus real Delay mono/ping-pong pause/resume and 64 Kit/FX bank roundtrips.
- MIDI hardening source-extraction suite still passes **879 cases under UBSan and ASan**; old Kit upgrade still passes **786,432 voice checks and 131,072 Kit layouts per sanitizer**; independent FX bank test passes **246,615 checks per sanitizer**; R18 CLP/R14 135-state core parity, R21/R22, FAT16/FAT32, project transaction, USB-MIDI and Compressor regression tests all PASS.

**Host limitations:** Native sanitizer/emulation plus source-extracted DSP logic are not Cortex-M7 cycle timing, device DAC capture, real MIDI/SD interrupts, or owner listening. The A/B compares the exact CLP DSP bodies to themselves under the two control conditions, not against a physical audio recording. The accepted R18 DSP source was not edited. The existing R18-vs-R14 core regression also remains passing.

## Pinned ARM validation and exact resource ledger

- ARM job **`135107490286` — SUCCESS** (Arm GNU Toolchain `14.3.rel1`, pinned download digest checked; CMake Release, ELF/MAP/IMG generation, static region tests all PASS).
- CI-reported **validation-only image**: `LXR_V2_V1.7.img`, **307,184 B**, SHA-256 **`3a97bc7f409426d4ef9ddddd04ec67e50075753493eacfacfa778a45e9a331dc`**. The `.img` bytes are **NOT** in the evidence artifact and were not independently retrieved or re-hashed. This file is **NOT FOR FLASHING**.
- CI ELF: `LXR_V2_STM32F765.elf`, **415,660 B**, SHA-256 `e3d19c2afc571b96d862eabb7d4e41476d3d25f283b1e425a1b7156f0b711a63` (original ELF included in ARM evidence).
- CI MAP: `LXR_V2_STM32F765.map`, **555,842 B**, SHA-256 `57a07ebe33ff9aa384e8f31441e6f9dcf41d2616ff6bdbbd3903a5cdbcef1bb6` (original MAP included in ARM evidence).

| Linker region | Pre-change R23 MIDI | New R23 editor CLP | Delta |
|---|---:|---:|---:|
| Ordinary flash `.text` | 171,904 B | **171,888 B** | −16 B |
| `.rodata` | 83,304 B | **83,304 B** | 0 B |
| ITCM | 13,600 B | **13,600 B** | 0 B |
| DTCM static | 109,324 B | **109,324 B** | 0 B |
| General SRAM static | 276,952 B | **276,952 B** | 0 B |
| Validation-only image size (incl. header) | 307,200 B | **307,184 B** | −16 B |

Nominal linker-region gaps: **ITCM 2,784 B**, **DTCM 21,748 B before actual stack**, **general SRAM 116,264 B**. These are **static** calculations. CPU/WCET, interrupt/DMA deadline margin, runtime stack high-water, hardware audio continuity, and long-term stability are **UNMEASURED**.

## Original CI evidence, retention and failure handling

The archive supplies *original byte-identical nested GitHub Actions artifact ZIPs* and their SHA-256 manifest (no reconstructed substitute):

| Evidence | Original GitHub artifact ID | ZIP bytes | SHA-256 |
|---|---:|---:|---|
| Host CI evidence | `14469125612` | 1,082,010 | `df69c905b8ecc1a78e5a46a61347f1dc0ed2d857655cbcf6b9e21871300c5d0d` |
| ARM CI evidence | `14469165754` | 312,760 | `e15e55d0819ab7a44a973c48d1a4bbac709c6250131caa787e6fe2ad31e58dc8` |

Both original ZIPs have independently checked SHA-256 hashes equal to GitHub's artifact digests, valid ZIP CRCs, and `SOURCE_SHA.txt` files pointing to `f8fc6fba3b7599095bd073460760b47ad49b173c`. Their contents include generated source, raw compiler/host regression outputs and logs, ELF/MAP, pinned toolchain identity, `OUTPUT_SHA256SUMS.txt`, section sizes, and static comparison. The ARM evidence deliberately excludes the installable `.img`. GitHub artifact expiration is **2026-12-18**; retain this owner archive as the durable copy.

The final CI run passed on its first attempt. No red CI attempt or rewritten failed test is associated with this isolated editor-CLP branch. The inherited experimental history, including previous R23 MIDI STOP attempts, remains separately preserved and is not relabeled as PASS.

## Disposition and next authorization

**Result:** This targeted control-path defect is source/host/ARM-corrected on an **experimental** branch. It prevents editor-only CLP feedback clearing while keeping true Kit resets and all previous FX/MIDI functionality as tested. It is **not** owner-heard sound acceptance. The owner's decision is still needed before an eventual combined R23 hardware audition, a flashable firmware package, publication/promotion, or other DSP-affecting change.

**Explicitly unchanged:** frozen/resumed Delay tails with no active Delay voice; accepted R18 CLP algorithms; R22 stable; protected upstream `main`; current unmerged experimental `release/r23` ref. No separate new release or hardware measurement was performed.