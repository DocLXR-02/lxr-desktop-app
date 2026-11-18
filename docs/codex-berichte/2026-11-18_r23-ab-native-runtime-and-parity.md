# R23 versus R23-A — Software-only mixer runtime and output parity

**Date:** 2026-11-18 (Europe/Vienna)  
**Status:** **PASS** isolated output parity and host benchmark; hardware runtime **UNMEASURED**.  
**Task:** Explicit owner approval for software-only A/B timing and identical-sample tests while the LXR-02 is at the manufacturer for DATA encoder inspection. No device tests, flashing, firmware merge or release.

## Immutable scope and sources

- R23 reference `release/r23`: `d0b49dff26b6fcf11e0367249d329a6682bfefa4` (unchanged).
- R23-A experiment: `a6e0cf675837ad01f94e52d7813b2d49e5795814` (unchanged).
- Accepted R22 TEST03: `0e37942250f79306627702061d851731bdfdbbc8` (unchanged).
- Test-only branch `experiment/r23-ab-native-runtime-2026-11-18`, commit `a7d02fbce71a3f7906f25a19e0d1bc55a7f60dab`: added only `tests/r23_mixer_runtime/run.py` and `.github/workflows/r23-ab-native-runtime.yml`, leaving production firmware unchanged.
- [GitHub Actions run 44300200988](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44300200988): **SUCCESS**, original raw artifacts preserved.

## Executed methodology and results

The native runner extracts the **actual** `mixer_addDataToOutput` and physical-jack fallback functions with `git show` from both exact source SHAs; the test asserts identical function bodies after normalizing only the inline/noinline attribute. It compiles them with identical GCC `-O3 -ffast-math -fno-strict-aliasing` flags and native reference implementations of `__QADD16` and `__PKHBT`.

**Byte parity PASS:** 3,520 deterministic six-voice × 64-frame scenarios, varying physical output fallback, FX selection, source samples and pan, generated **6,307,840 byte-identical DAC and FX-input output bytes**, SHA-256 `cdfa48873ce70bc2c30cce5d5dea91d01eb091d637a55cfb6ba4b267caf503bd`. Independently recompiled the saved C harness source locally using GCC 14.2.0 plus `-fsanitize=undefined,bounds -fno-sanitize-recover=all`; **PASS** on all 3,520 scenarios, no sanitizer errors, same byte SHA.

**Native timing observation:** Eight deterministic six-voice workloads; nine repetitions of 5,500 blocks in each of two interleaved passes per variant (18 timing samples per variant/scenario). Per-scenario median R23-A relative to R23: **+1.42%, +1.11%, +0.98%, +1.70%, +1.32%, +0.57%, +1.35%, +0.94%**. Median of eight deltas approximately **+1.21%**, range **+0.57–1.70%**. Raw per-run timing CSV retained. These are native x86 nanoseconds for an isolated routing helper, **not** predicted STM32 execution time, percentage total LXR CPU load, deadline margin or WCET.

**Existing ARM source-matched ELF static disassembly (no new firmware build):**

| Exact symbol | R23 | R23-A |
|---|---:|---:|
| `mixer_calcNextSampleBlock` ITCM size | 4,554 B | 2,140 B |
| Separate `mixer_addDataToOutput.constprop.0` ITCM | none | 428 B |
| Branch-with-link (`BL`) instructions in `mixer_calcNextSampleBlock` | 49 | 49 |
| Calls from block renderer to separate mixer helper | 0 | **6 per audio block** |
| Total occupied ITCM | 15,960 B | 13,968 B |
| Nominal free ITCM | 424 B | **2,416 B** |

The 1,992 B ITCM savings are independently grounded in prior Release ELF/MAP artifacts. Calls are at block rate, not sample rate; machine instruction counts alone cannot prove timing.

## Evidence, caveats, decision

- [Exact CI run](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44300200988).
- [Raw artifact 14425860736](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44300200988/artifacts/14425860736), ZIP SHA-256 `7aa855482af74d47a864e737064808ad74b202cff29aeb9224bd96a8c988cadb`, independent ZIP CRC PASS; expires 2026-12-18. Preserves extracted production helper C for both variants, per-repetition timing CSV, original log and parity checksum. No firmware IMG.
- Detailed English handoff and independent local UBSan/ARM-symbol extracts are retained in the owner evidence ZIP `LXR_R23_AB_Runtime_Evidence_2026-11-18.zip`.

**Limits:** Neither native host timings nor identical isolated mixer buffers prove full four-FX DSP return behavior, real LXR-02 audio, Cortex-M7 CPU cycles, IRQ/DMA deadlines, runtime stack, WCET or long-term reliability. All real-device timing is **UNMEASURED**. The device is with the manufacturer; no hardware measurements were run. Do not merge or flash on this evidence alone.

**Recommendation:** Keep R23-A as the preferred memory-saving experimental variant pending separately authorized hardware DWT-cycle benchmark after the device returns, and integrated multi-FX audio regression before any sonic acceptance. Do not alter stable R22, accepted R18 CLP or `main`.
