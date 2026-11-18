# R23-B — Block-prepared mixer routing comparison: verified ARM build report

**Date:** 2026-11-18 (Europe/Vienna)  
**Status:** Experimental ARM BUILD PASS / HOST PASS; **NOT FOR FLASHING**.  
**Repository:** `DocLXR-02/lxr-desktop-app`  
**R23 baseline:** `d0b49dff26b6fcf11e0367249d329a6682bfefa4`.  
**Immediate experiment parent:** R23-A `a6e0cf675837ad01f94e52d7813b2d49e5795814`.  
**Experimental branch:** `experiment/r23-b-precomputed-routing-2026-11-18`.  
**Tested source:** **`fee14e391e804150948d6084d34d1fe3e6be92d9`**.  
**Protected stable reference:** R22 TEST03 `0e37942250f79306627702061d851731bdfdbbc8`.

## Authorization and changes

Owner approved isolated A/B comparisons of the R23 mixer and CI validation without physical LXR-02 tests, because the instrument is with the manufacturer for DATA-encoder examination.

This variant retains R23-A's noinline ITCM mixer function, then prepares six physical output fallbacks, FX buffer pointers, and unique FX mask **once per block**, after input jack availability is read. It removes redundant per-voice FX-route/buffer address selection inside the audio-distribution helper, and passes prepared destinations and FX pointers from six call sites. The DSP effect bodies and return mixing logic were unchanged.

Initial [commit `55439e126892d3af34e1a7c945cebcc2a565802b`](https://github.com/DocLXR-02/lxr-desktop-app/commit/55439e126892d3af34e1a7c945cebcc2a565802b) triggered [CI 44292091682](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44292091682), which **STOPPED BEFORE TESTS** because `git diff --check` flagged whitespace at CRLF-terminated edited call-site lines. The second [commit `fee14e391e804150948d6084d34d1fe3e6be92d9`](https://github.com/DocLXR-02/lxr-desktop-app/commit/fee14e391e804150948d6084d34d1fe3e6be92d9) normalized only the twenty edited call-site line endings without semantic changes. No history was rewritten.

## Independently verified final CI

[GitHub Actions run **44292230152**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44292230152), checkout SHA `fee14e391e804150948d6084d34d1fe3e6be92d9`: **host SUCCESS and ARM SUCCESS**. Same pinned Arm GNU 14.3.rel1 Release pipeline and inherited host regressions as R23-A. Routing-mask test (280,032 cases) PASS. No end-to-end integrated multi-effect audio host test or physical instrument listening was performed.

| Static measurement | R23-A | R23-B | Difference B minus A |
|---|---:|---:|---:|
| ITCM occupied | **13,968 B** | 14,376 B | **+408 B** |
| ITCM nominal free | **2,416 B** | 2,008 B | **-408 B** |
| DTCM linker region | 109,216 B | 109,216 B | 0 |
| General SRAM linker region | 275,164 B | 275,164 B | 0 |
| Flash `.text` section | 161,472 B | 161,472 B | 0 |

Against original R23 baseline, R23-B still saves 1,584 B ITCM and exceeds the intended 1 KiB headroom. Nevertheless, **block precomputation adds 408 B ITCM relative to A**, with no measurable static memory benefit. It can additionally change timing semantics if a physical jack or routing parameter changes between voices within one render block; this dynamic behavior has not been verified at hardware or production-integrated host level.

## Binary identities and retained artifacts

- CI-generated `LXR_V2_V1.7.img`: **297,512 B**, CI-reported SHA-256 `e02627862c4471280cde7ee73fb1e5bc2eb63c48d273d786fbe14512fd99be82`; IMG bytes were not retained in the evidence artifact and were not independently rehashed. **Do not flash.**
- ELF: **403,404 B**, SHA-256 `4d5b87bb0385bf8e3a54c32f40a630f26871770cd1bb83942fd95a3974916227` (independently verified).
- MAP: **553,291 B**, SHA-256 `bcc0f829a41a681c2497946ae3098a1a0950aa76d24584ab65347b6fcd280177` (independently verified).
- [ARM artifact 14421737163](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44292230152/artifacts/14421737163), ZIP SHA-256 `40cd3012b2853818acadbbf92d61534d3435f33c900ab69fc24bb1f23073388d`, CRC PASS.
- [Host artifact 14422565663](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44292230152/artifacts/14422565663), ZIP SHA-256 `d613e7482edec26069b334441c8e4d27aaf4e9d193a3c0610990db9c6d7d2990`, CRC PASS.
- Retention through 2026-12-18 UTC. Original ZIPs were separately downloaded and verified for the owner A/B archive.

Five nonblocking enum-comparison warnings remain. No additional cleanup was authorized.

## STOP / disposition

**R23-B is not recommended for selection on static ITCM grounds.** R23-A is smaller and avoids altering block-level routing snapshot semantics. Preserve R23-B as a documented experimental alternative; do not merge to `release/r23`. No real device testing, CPU/WCET, IRQ/DMA, stack high-water or sonic acceptance; all remain **UNMEASURED**. No device flashing, release, bootloader or accepted R22/R18 modifications took place.
