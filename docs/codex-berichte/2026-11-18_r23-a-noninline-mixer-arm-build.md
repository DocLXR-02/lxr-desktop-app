# R23-A — No-inline mixer ITCM comparison: verified ARM build report

**Date:** 2026-11-18 (Europe/Vienna)  
**Status:** Experimental ARM BUILD PASS / HOST PASS; **NOT FOR FLASHING**.  
**Repository:** `DocLXR-02/lxr-desktop-app`  
**Base source:** `d0b49dff26b6fcf11e0367249d329a6682bfefa4` (`release/r23`).  
**Experimental branch:** `experiment/r23-a-noninline-mixer-2026-11-18`.  
**Tested source:** **`a6e0cf675837ad01f94e52d7813b2d49e5795814`**.  
**Reference firmware:** R22 TEST03 `0e37942250f79306627702061d851731bdfdbbc8` (unchanged).

## Authorization and change

Owner approved isolated R23-A/B mixer code comparisons, relevant host regression and ARM validation only. Physical LXR-02 testing is unavailable because the device was returned to the manufacturer for a DATA-encoder examination. This permission did not include device flashing, release, merging, CLP sonic change or edits to accepted R22.

One production source line changed in `Core/Src/Soundengine/DSPAudio/mixer.c`: `IN_ITCM_RAM static inline void mixer_addDataToOutput(...)` became `IN_ITCM_RAM static __attribute__((noinline)) void mixer_addDataToOutput(...)`. The function body and its routing, panning and 64-frame saturating loops were not changed. The dedicated `.github/workflows/r23-ab-comparison.yml` was added on the experimental branch to run the inherited test/build pipeline without touching `release/r23`. The workflow records evidence but does not upload an IMG.

## Independently verified validation

[GitHub Actions run 44291774697](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44291774697), exact SHA above: **host job SUCCESS and ARM job SUCCESS**. Pinned Arm GNU toolchain 14.3.rel1, CMake Release, compiler and linker success, generated ELF/MAP/IMG in CI.

Host validation includes 280,032 shared-routing/mask cases, R18 Delay mono, Compressor mono, CLP source comparison, USB-MIDI, R21/R22 voice/drum copy and Project storage/transaction regression. These are existing host tests, **not** a new production-integrated simultaneous-FX audio validation. No physical device tests, listening or runtime timing measurements.

| Static measurement | R23 base | R23-A | Delta |
|---|---:|---:|---:|
| ITCM occupied | 15,960 B | **13,968 B** | **-1,992 B** |
| ITCM nominal free | 424 B | **2,416 B** | +1,992 B |
| DTCM linker region | 109,216 B | 109,216 B | 0 |
| General SRAM linker region | 275,164 B | 275,164 B | 0 |
| Flash `.text` section | 161,472 B | 161,472 B | 0 |

R22 accepted occupied ITCM 15,304 B; R23-A is **1,336 B below** R22 ITCM occupancy. These are linker/static measurements, **not** evidence of higher CPU throughput or real-time scheduling margins. The `mixer.c` ITCM input section decreased from 4,676 B in R23 base to 2,688 B in R23-A. The remaining small discrepancy versus overall ITCM is linker stubs/alignment.

## Binary and raw evidence

- CI-generated `LXR_V2_V1.7.img`: **297,104 B**, CI-reported SHA-256 `28dae4f47d1de4aabe138391b4adec5289300adbf50e90670901cd4aae5f65c1`; its bytes were **not** included in the downloadable evidence artifact and were **not** independently rehashed. **Do not flash.**
- ELF: **403,056 B**, SHA-256 `97215731fa196eaa2e0009aa6b66cf645de8c4279b588e38f662b54f9049fa6d` (**independently checked** against artifact member).
- MAP: **553,252 B**, SHA-256 `9ff7e3163527563f6a2edc032c3701a7842df1795c6dec711c53c3d3ae1f9f3e` (**independently checked**).
- [ARM evidence artifact 14422395538](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44291774697/artifacts/14422395538), ZIP SHA-256 `42e91e391de02d595fb193d9acf2a4bb55277fa6dfdb9117babf5106e5595f44`, CRC PASS.
- [Host evidence artifact 14421468446](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44291774697/artifacts/14421468446), ZIP SHA-256 `ed33e9b73aec6340665787a5351b132c74755a79ab107d52400b64d078153d08`, CRC PASS.
- Both GitHub Actions artifacts expire 2026-12-18 UTC. Original archives were separately downloaded and verified for the A/B owner evidence package.

Six nonblocking `mixer.c` warnings remain (five enum comparison and one misleading indentation). No attempt was made to change them outside this isolated code-size experiment.

## Stop boundaries / next decision

No device is available; **CPU/WCET, IRQ/DMA deadlines, runtime stack high-water, actual audio correctness and long-run reliability remain UNMEASURED**. The noinline function introduces a different runtime call structure, so host success and static ITCM savings do not guarantee deadline safety. P17-B still lacks end-to-end multi-effect DSP routing/tails tests, per-effect UI/persistence and device acceptance. Do not install or select R23-A as stable without separate owner authorization. No merge or modification of protected refs occurred.

**Conclusion:** R23-A meets the ITCM-headroom target with 2,416 B nominal free and is the preferred candidate for further isolated review, subject to future performance and audio verification.
