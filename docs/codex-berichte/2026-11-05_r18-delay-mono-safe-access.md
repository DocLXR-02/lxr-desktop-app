# R18 Delay Mono Safe-Access Candidate — Verified Host Report

**Date:** 2026-11-05 (Europe/Vienna)  
**Authorization:** Owner approved the narrowly scoped Delay correction (item 1). ARM firmware build, flashing, release and sound modification are NOT approved.
**Status:** Candidate source fix PASS in host CI; no ARM/device verification.

## Exact source identities

| Role | Identity |
| --- | --- |
| Accepted R18 source | 41b364067bce0d8cde624c184933d9cb88e2725b |
| Candidate branch | fix/r18-delay-mono-safe-access |
| Production patch commit | c0ade6400479ced32260a799a4974eef4ae48b93 |
| Tested candidate commit | 35802b4abaf0b080e1d0e5d3b35ea90a212a0178 |
| Only changed production file | Core/Src/Soundengine/DSPAudio/FX/Delay.c |
| Original Delay blob | 64d2f4721347c53208d80257ca607b079d987ff5 |
| Fixed Delay blob | 1a4bbac4021efe9e0c0d9cfa9bee317e5a51e6e6 |
| Added test | tests/r18_delay_mono/run.py |
| Added workflow | .github/workflows/r18-delay-mono-host.yml |
| Final CI | https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41965755184 |
| CI job | 127042256379 |
| CI artifact | r18-delay-mono-host-evidence, ID 13388015047, retention 30 days |

Downloaded CI artifact ZIP was 83,457 bytes, SHA-256 54ccb27b6dd56397325e2a5bc5e7f57bec6efd07c7f2bf88282129eadd0983e5. ZIP integrity passed, 22 members were present, and 17/17 recorded hashes were independently verified. Artifact retention is limited.

## Fault and correction

The original normal Delay and ping-pong Delay routines both perform a 32-bit read from the selected output pointer when routing to a single 16-bit mono channel. Selecting the right halfword gives an unaligned 32-bit read; the original R18 normal mono-right fixture produces a UBSan misaligned int32_t diagnostic. The original right-channel input is not a C-defined behavior suitable for bit-identical output claims.

Both functions now create the stereo int32 pointer only when channels == 2. Mono output uses a separate aligned 16-bit pointer, performs the original QADD16 low-lane saturating addition, stores only the selected mono halfword, and advances by two int16 elements per interleaved frame. The Stereo branch and Delay timing/feedback/tone/dry-wet paths are unchanged. CLP source, mixer and FX routing are unchanged. Actual GitHub diff: 10 added and 6 removed lines in Delay.c, plus one test file and one CI workflow.

## Executed host verification

Reproduction/verification command:

    python3 tests/r18_delay_mono/run.py build/r18-delay-mono-evidence

The test extracts genuine pinned R18 and candidate production functions, checks both Git source blob identities and exact allowed patch, and compiles native host fixtures with AddressSanitizer, UndefinedBehaviorSanitizer, alignment, bounds and float-cast-overflow checks. MCU intrinsics and hardware services are represented with host shims, not executed on ARM hardware.

- Expected **baseline fault** confirmed: original R18 mono-right raises: "runtime error: load of misaligned address ... for type 'int32_t', which requires 4 byte alignment". The original full diagnostic is retained in r18-mono-right.expected-failure.log.
- **Fixed host pass:** normal and ping-pong Delay mono-left, mono-right and stereo paths pass in selected fixtures.
- **144 exact output comparisons passed:** modes normal/ping-pong; sizes 1, 7 and 64; dry/wet 0, 0.25, 0.5 and 1; steady and interpolating delay state. Candidate mono-left/stereo are byte-identical to defined R18 reference outputs, and candidate mono-right matches the equivalent safe mono-left lane given matched channel starting data.
- **CLP regression passed:** existing R18 CLP unit, state, FX-bus, output-mix, core-isolation and R14 MAX fader checks. Existing fixture highlights: 75,000 crossover calculations, 135 core snapshots, 16,384 routing tests and 4,096 ramp checks.
- **Source protection passed:** Only Delay.c was changed among production files. Protected main, R4, R11, R14 and accepted R18 refs verified unchanged.

**Recorded initial failure:** CI run 41965648399 failed before executing tests because a generic Git whitespace checker rejected legacy CRLF line endings in Delay.c. Only the workflow was changed to use the CRLF-aware diff check; the actual production fix was not changed. CI run 41965755184 then succeeded. Do not mistake the preflight failure for a DSP regression.

## Limits and decision gate

No ARM compilation, image, flash, code generation/resource ledger, hardware audio, Cortex-M7 CPU/WCET/IRQ/DMA deadlines, stack high-water, thermal/soak test or general real-device reliability measurement was performed. The changed Delay output is not the CLP effect and no musical CLP retuning occurred. Other Delay issues and USB/Compressor are outside this task.

A **separately approved ARM validation build** should be the next gate, from this exact tested candidate. Require a new independent English build report with source SHA, CI/toolchain, full load/memory deltas, image hashes and explicitly unmeasured device limits. Do not flash, release, merge, or modify accepted R18 automatically.

[Candidate diff](https://github.com/DocLXR-02/lxr-desktop-app/compare/41b364067bce0d8cde624c184933d9cb88e2725b...35802b4abaf0b080e1d0e5d3b35ea90a212a0178) · [Test source](https://github.com/DocLXR-02/lxr-desktop-app/blob/35802b4abaf0b080e1d0e5d3b35ea90a212a0178/tests/r18_delay_mono/run.py) · [Final CI](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41965755184)

**Publication boundary:** This report-only branch descends from the documentation branch. The canonical project status, private KnowledgeBase and accepted firmware refs remain unchanged pending new approval.
