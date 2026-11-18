# R23 — R23-A selected as the shared FX development baseline

**Date:** 2026-11-18 (Europe/Vienna)  
**Repository:** `DocLXR-02/lxr-desktop-app`  
**Task:** Owner-authorized selection of the previously tested R23-A mixer layout as the active experimental R23 development baseline.  
**Disposition:** **SOURCE ADOPTED / HOST PASS / ARM BUILD PASS / NOT FOR FLASHING**. No stable release or sonic acceptance.  
**Execution:** CHAT and GitHub CI; no Codex, WORK or hardware interaction.

## Authorization and lineage

The owner approved adopting the isolated R23-A noinline mixer optimization into `release/r23`. No authorization was given to merge experiment branches wholesale, change FX DSP algorithms or audio parameters, flash a device, modify a bootloader, release firmware, or modify protected stable references.

| Record | Git identity |
|---|---|
| Protected owner-accepted R22 TEST03 | `0e37942250f79306627702061d851731bdfdbbc8` |
| R23 before adoption (source parent) | `d0b49dff26b6fcf11e0367249d329a6682bfefa4` |
| Previously tested R23-A experiment | `a6e0cf675837ad01f94e52d7813b2d49e5795814` |
| **New R23 head / exact retested source** | **`238389475359f11bf317b446e668e0c82333c231`** |
| Protected upstream main (checked after selection) | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |

On `release/r23`, exactly one source line changed at `Core/Src/Soundengine/DSPAudio/mixer.c:453`:

```diff
-IN_ITCM_RAM static inline void mixer_addDataToOutput(...)
+IN_ITCM_RAM static __attribute__((noinline)) void mixer_addDataToOutput(...)
```

The literal original signatures have unchanged arguments, return type and function body; the displayed `...` abbreviation is descriptive. The modified line uses LF to satisfy the existing `git diff --check` gate. The complete `Core/Src/Soundengine/DSPAudio/mixer.c` blob of the selected R23 head is **`9e7cd2b1959d1258bb7ddaa7b4f93b020e5c0002`**, byte-identical to the originally tested R23-A experiment. No experimental CI workflow or native benchmark source was merged. GitHub comparison from the parent confirms one changed file, one added and one removed line.

## Independent CI revalidation of exact selected R23 SHA

[GitHub Actions run **44302818179**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44302818179), checked-out source `238389475359f11bf317b446e668e0c82333c231`, **overall SUCCESS**.

- Host regression job [134796638950](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44302818179/job/134796638950): **PASS**. The production shared FX selector test passed **280,032 routing/mask cases**. Existing R18 CLP-core snapshots (135), Delay and Compressor mono regression, USB MIDI, R21/R22 voice/drum copy, project storage and transaction regression suites passed.
- ARM Release job [134796638415](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44302818179/job/134796638415): **PASS**, pinned Arm GNU Toolchain 14.3.rel1; complete compile, ELF link, MAP generation, section usage checks, output identity capture.
- The unchanged R23 workflow `.github/workflows/r23-p17b-validation.yml` preserved raw logs and the ELF/MAP. The generated IMG was **not** included in the downloadable evidence ZIP or approved for flashing.
- Six existing nonblocking `mixer.c` warnings persisted (five enum comparisons and one misleading indentation). These were not modified in the narrow adoption task.

### Matched static memory evidence

| Item | R23 before selection | Selected R23-A baseline | Difference |
|---|---:|---:|---:|
| ITCM occupied | 15,960 B | **13,968 B** | **-1,992 B** |
| ITCM nominal free (16,384 B capacity) | 424 B | **2,416 B** | +1,992 B |
| DTCM linker region used | 109,216 B | 109,216 B | 0 |
| General SRAM linker region used | 275,164 B | 275,164 B | 0 |
| Flash `.text` | 161,472 B | 161,472 B | 0 |

Against accepted R22, the selected R23 uses **1,336 B less ITCM**. These are static linker values, not measured CPU/WCET or a certified timing margin.

### Build identities and byte-level cross-check

- Selected build ELF: **403,056 B**, SHA-256 **`97215731fa196eaa2e0009aa6b66cf645de8c4279b588e38f662b54f9049fa6d`**.
- Selected build MAP: **553,252 B**, SHA-256 **`9ff7e3163527563f6a2edc032c3701a7842df1795c6dec711c53c3d3ae1f9f3e`**.
- Original R23-A experimental ELF and MAP have **exactly the same byte lengths, SHA-256 and actual bytes** as the newly selected build; verified by directly comparing ZIP members of the original and new CI artifacts.
- CI-generated `LXR_V2_V1.7.img`: **297,104 B**, CI SHA-256 **`28dae4f47d1de4aabe138391b4adec5289300adbf50e90670901cd4aae5f65c1`**, identical to the original R23-A build's reported hash. The IMG bytes were not included in either evidence ZIP and thus were **not independently rehashed**. No IMG was delivered.
- [Selected ARM artifact 14426003868](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44302818179/artifacts/14426003868): ZIP SHA-256 `6bbd0be9f6e4b0748998a86c9d3ccd197621824f332faabd0e8e763e57ddc28d`, ZIP CRC PASS, 306,123 bytes.
- [Selected host artifact 14426003864](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44302818179/artifacts/14426003864): ZIP SHA-256 `1ff1c591ca4cbc95932d00c918c09f0dcf740b9eae7d0a3e219e79430601dbac`, ZIP CRC PASS, 6,127 bytes.
- GitHub CI artifacts expire 2026-12-18 UTC. Both original archives were separately downloaded, verified against GitHub digests, ZIP-CRC-tested and retained for the local owner evidence package.

## What previous tests establish — and do not establish

The prior [native isolated A/B report](https://github.com/DocLXR-02/lxr-desktop-app/blob/cb8ec5f389ab0c9f225a6fd45e38b5722840fe20/docs/codex-berichte/2026-11-18_r23-ab-native-runtime-and-parity.md) confirmed 3,520 six-voice cases with byte-identical routed DAC/FX-input buffers (6,307,840 comparison bytes) and an x86 host-only ~0.6–1.7% isolated-helper timing penalty. The exact A function body is now adopted. That prior experiment is reused, **not rerun** in the selected R23 CI.

Unmeasured: real-device CPU/WCET, IRQ/DMA deadlines, runtime stack high-water, complete multi-FX integration audio behavior, effect tails, live audio fidelity and long-term reliability. The LXR-02 is currently with the manufacturer for DATA encoder examination. **No device flash, physical test or listening test was performed.** A successful ARM compile and current host regression coverage do not establish sound acceptance or device safety.

## Protection, next decision and status

`release/r23` is the active **experimental development baseline** at `238389475359f11bf317b446e668e0c82333c231`; the source-isolated A and B branches, the independent native A/B test branch, stable R22, protected main, accepted R18 CLP and all historical records remain preserved. No merge, force push, release, installation or bootloader change took place.

Do not automatically initiate follow-up feature development. Next work (only with new scoped owner permission) may address integrated multi-FX routing behavior, tails, warnings, or parameter/menu/persistence design using software-only CI and explicit sound-change controls. Hardware runtime and acoustic acceptance remain deferred until a suitable device is available.

**Final disposition: R23-A selection completed, exact-source host PASS / ARM PASS, experimental NOT FOR FLASHING.**
