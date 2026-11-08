# R19 Performance and Reliability Baseline — Next-Chat Read-Only Investigation Plan

**Prepared:** 2026-11-08 (Europe/Vienna).  
**Lifecycle state:** **PREPARED, NOT EXECUTED**. This document defines the next investigation; it is **not** a performance audit, a measurement report, a Codex task, a build authorization, or permission to alter any source, hardware, audio or release.  
**Recommended executor:** normal CHAT, preferably GPT-6 High if available; Codex and Work are not justified for the initial read-only work. No known live Codex quota or reset data is available (**UNKNOWN**).  
**Canonical status:** [PROJECT_STATUS.md](../PROJECT_STATUS.md). **Authoritative private context:** latest English LXR Firmware Lab KnowledgeBase, currently prepared as v1.5.4.

## 1. Exact baseline and safety boundary

| Item | Pinned evidence |
|---|---|
| Repository | `DocLXR-02/lxr-desktop-app` |
| **Owner-approved stable functional baseline** | **R19**, source SHA **`638b6eaae5b1624637d20cbc11a4a3e0175edca2`** |
| Source branch (convenience pointer only) | `candidate/r19-integrated-three-fix-2026-11-07`; read by immutable SHA, not an assumed stable branch head |
| R19 image | `LXR_V2_R19.img`, 288,184 B, SHA-256 `a1d53435ce922c4ef57ce595bf8c180b5225340780b6ad3241b6340af652e57b` |
| R19 zip | `LXR_V2_R19.zip`, SHA-256 `b9c25355a0a880b4162ecfc3d439f784593f134bb21132e6d749d06103f01068` |
| Original accepted CLP musical reference | R18 `41b364067bce0d8cde624c184933d9cb88e2725b`, untouched; R19 inherits its signal-path implementation |
| Three-fix predecessor | `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` |
| ARM/host test baseline | [R19 CI 42407160858](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42407160858), SUCCESS |
| [Separate verified build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/ce2d87d499a5708881f38b0fe0ffa6b0e50c28f0/docs/codex-berichte/2026-11-07_r19-version-only-candidate-arm-build.md) | Exact build, toolchain, source/IMG hashes, matched resources and inherited host tests |
| [Owner installation observation](https://github.com/DocLXR-02/lxr-desktop-app/blob/dcd3b3c0c137259d38bd1cb9eac8bbe0622ba784/docs/hardware/2026-11-08_r19-candidate-owner-device-acceptance.md) | Successful device install and unspecified normal functional test; not independent timing measurement |
| [Final distribution authorization](../releases/2026-11-08_r19_final_owner_approved_distribution.md) | Owner approved release of the **existing tested** image; this does not authorize subsequent development |
| Canonical evidence index and rules | [EVIDENCE_INDEX.md](../EVIDENCE_INDEX.md), [WORKFLOW.md](../WORKFLOW.md), [RECORDKEEPING_STANDARD.md](../RECORDKEEPING_STANDARD.md), `AGENTS.md` **at R19 exact source SHA** |

**Do not modify:** `main`, R18/R19 reference source or image bytes, R4/R11, existing integration SHA, bootloader, project storage, accepted CLP sound or production code. **Do not run builds, edit tests, enable instrumentation, flash, or run device tests.** Those require new explicit owner approval after the initial read-only diagnosis. Do not create a native GitHub Release, tag, branch or merge simply to begin this analysis. The immutable source SHA already pins the baseline.

## 2. What is known versus unmeasured

**Verified from the existing build evidence:** R19 candidate CI passed inherited Delay, USB-MIDI, Compressor, R18 CLP and general host regressions; successful ARM Release build; independently validated updater IMG, headers, checksum, ELF/load segments, compiler commands and matched source sections. The distributed final R19 IMG is byte-identical to the owner-tested R19 candidate. Only the startup marker and updater checksum differ from the integrated TEST IMG.

**Measured by static linker accounting, not by live MCU instrumentation:**

| Section accounting | R19 static bytes | Nominal gap |
|---|---:|---:|
| ITCM `.itcm_text` | 15,208 | 1,176 |
| DTCM `.dtcm + .dtcmz` | 107,420 | 23,652 |
| General SRAM `.data + .bss + ._user_heap` | 274,660 | 118,556 |
| Application FLASH load payload | 288,168 | 727,640 (nominal capacity convention) |

The DTCM gap is **not** a measured runtime stack margin. Different report conventions may include an extra 4 bytes in the RAM region aggregate; use matched section accounting and note the convention. All values originate in the verified R19 build report and R19 resource-ledger addendum.

**Owner-hardware evidence:** R19 installed and functionally tested with no problem reported; exact matrix, duration and instrumented readback were not supplied. Previously observed multi-effect clicking ceased after reloading another project and then the original project; the transient root cause is **UNKNOWN**. **Physical USB-MIDI** was **NOT TESTED**, consciously accepted as a release risk. MIDI host fixtures passed but do not establish actual physical transport.

**Still UNMEASURED:** CPU utilization and worst-case render time (WCET), full audio-block deadline margin, TIM2 IRQ latency/jitter, SPI2/SPI3 I²S DMA deadlines and overruns, active DTCM stack high-water, concurrent allocation/reentrancy and complete long-duration/thermal reliability. No numerical runtime performance or safe additional FX-instance count may be invented.

## 3. Reusable source-grounded starting points (not observed bottlenecks)

All paths below are **from the exact R19 source**, not the documentation branch. Paths and line windows are initial navigation aids; verify the full surrounding call graph before drawing conclusions.

| Priority | R19 path / line windows | Known source behavior | Investigation question |
|---|---|---|---|
| P0, architecture | `Core/Inc/Soundengine/config.h:29-40,69-82` | `CALC_SAMPLES_IN_INTERRUPT=1`; 64-frame buffer when `USE_BIG_SAMPLE_BUFFER=1`, with conditional alternate DMA mode; `REAL_FS=44140.f` as the code's nominal configured rate | Confirm **effective** compile definitions, clock/prescaler and block period; do not confuse configured/nominal values with measurement |
| P0, deadline path | `Core/Src/stm32f7xx_it.c:262-277`; `Core/Src/Soundengine/Soundengine.c:22-39,81-99` | TIM2 IRQ schedules `seq_tick`, sample block calculation and MIDI processing; render work is conditionally run for invalid sample state | Map IRQ priority, timer placement, producer/consumer transitions, reentrancy and work included/excluded by diagnostics |
| P0, DMA/codec | `Core/Src/Soundengine/cs4344_cs5343.c:85-145`; `Core/Src/Soundengine/AudioCodecManager.c:17-27` | Two SPI/I²S DMA callbacks, distinct buffers, callback-side rearm and current-sample-valid changes | Identify observable underrun/rearm signals and asymmetric output/callback timing; avoid speculative causal attribution |
| P0, existing diagnostics | `Core/Inc/safety/runtimeDiagnostics.h:7-42`; `Core/Src/safety/runtimeDiagnostics.c:1-39`; `CMakeLists.txt:72-75` | `LXR_RUNTIME_DIAGNOSTICS` **OFF by default**; optional DWT cycle counter, render/TIM2 max counts, over-budget counts, DMA callbacks/not-ready/rearm-errors/long intervals | Audit exactly what is counted, overhead, instrumentation perturbation, visibility/readout, testability and missing measurements **before proposing** any enabled build |
| P1, mixer & FX | `Core/Src/Soundengine/DSPAudio/mixer.c:621-762`; `Core/Src/Soundengine/FxBus.c:41-135` | Six rendered voice paths, intermediate FX buffer, per-block routing to a shared processor and two physical output buffers | Identify repeated per-block/per-sample work and potential data-dependent processing; do not assume a hot spot |
| P1, Compressor | `Core/Src/Soundengine/DSPAudio/FX/Compressor.c:44-54,104-185` | `logf`/`expf` in processing helpers, sample-block VLA buffers `left[size]`/`right[size]` | Classify cost, numeric domains, stack exposure and gain saturation without changing sound or assuming CPU causality |
| P1, CLP reference | `Core/Src/Soundengine/DSPAudio/FX/Distortion.c:200-236` | Double-precision differential output processing and a soft-ceiling divide in the owner-accepted CLP branch | Compare compiled instructions, possible cost and semantic requirements; musical sound stays frozen without new owner decision |
| P1, memory/heap | `Core/Src/sysmem.c:49-72`; ARM map/ELF from retained R19 evidence | DTCM is the runtime stack region, general SRAM is used by heap; `_sbrk` bounds the break but notes possible allocator concurrency | Separate linker section margins, actual stack high-water, heap high-water, nested IRQ stacks and allocation concurrency |
| P2, transport/peripherals | `Core/Src/Soundengine/USB/usb_manager.c`; relevant UART/menu/storage source | FIFO changes are host-tested; timing under actual USB/SD/UI contention remains unmeasured | Check non-audio interference paths as a distinct, lower-priority hypothesis |

**Configured, NOT measured budget:** `runtimeDiagnostics.h` defines `LXR_DIAG_BUDGET=313181` cycles for a nominal 64-frame interval at 216 MHz and about 44,140.625 Hz, approximately 1.45 ms. This is a source constant, **not** observed available CPU time, an interrupt latency budget, proven per-block deadline, or a performance PASS. The `render` counter and `timer_isr` counter are different scopes and must not be conflated. The optional diagnostics do **not** currently deliver measured stack high-water or a complete deadline trace.

## 4. Next-chat read-only execution sequence

1. **Provenance gate:** Read the current private KnowledgeBase and live canonical status, workflow, evidence index, resource ledger, recordkeeping standard, and the R19 `AGENTS.md` at pinned SHA; recheck R19 source SHA and existing successful CI. Do not replace the pinned SHA with `main` or with a newer branch head. Inventory the retained R19 evidence ZIP and report hashes; do not rerun full builds to rediscover past passing results.
2. **Evidence-gap matrix:** Separate existing **SOURCE**, **HOST TEST**, **CI ARM**, **OWNER DEVICE** and **UNMEASURED** evidence for each reliability or performance claim. Flag what cannot be proven from reports alone, including independent sustained device behavior.
3. **Audio path review:** Draw the actual R19 data/clock/ISR/DMA dependency graph from exact sources and effective compile definitions. Identify critical path, buffer ownership and watchdog/underrun observability; separate probable hazards from demonstrated faults.
4. **Diagnostic capability review:** Audit existing OFF-by-default counters, their observation method and intrusive measurement risks. Produce an **instrumentation design only** (not patches), with candidate minimal memory/CPU overhead, startup/disable behavior, data capture, failure thresholds and stop conditions, all labeled **PROPOSED** until implemented/tested. Existing `LXR_DIAG_BUDGET` must be interpreted correctly and may require independent validation.
5. **Static efficiency review:** Identify cost candidates in mixer/FX/DSP/USB and memory lifetime. Distinguish compile-time/static evidence from actual WCET; do not invent cycle savings or measured gain.
6. **Risk-ranked recommendation:** Rank 2–3 **investigation targets** by potential benefit, acoustic regression risk, real-time safety risk, measurability, effort and evidence quality. Include an explicit `NO CHANGE` option when evidence does not justify implementation. Do not promise an achievable number of additional FX instances.
7. **Read-only deliverable:** An English summary/report with a provenance table, source paths/line evidence, potential bottleneck hypotheses, existing evidence reuse, missing measurements, recommended first measurement experiment and precise owner-approval boundary. Include no audio recording claim and no unperformed-test PASS.

### Suggested acceptance criteria for this investigation

- Every material claim traces to pinned R19 source, the referenced historical CI/report, owner observation or explicitly labeled inference.
- No firmware, audio, tests, CI workflow, Git refs, saved project/kit data or hardware were changed by the analysis.
- No CPU load, WCET, stack high-water, IRQ/DMA runtime margin or long-term reliability figure is asserted without actual target observations.
- No Codex task or expensive autonomous workflow launched. The user can decide separately whether to authorize instrumentation, CI/ARM builds and hardware testing after reviewing the read-only assessment.

## 5. Next approval gate (not granted by this plan)

If the analysis supports an instrumented diagnostic build, present to the owner **before** changing code: exact R19 base SHA; one-bounded objective and an isolated development branch; existing evidence reused; exact files and non-goals; precautions for CLP sound and real-time deadlines; independent host/CI/ARM build/tests and a **separate English per-build report**; safe installation/backup/recovery and low-volume on-device checks; evidence retention; uncertainties and stop criteria. State known effort and quota only if actually visible; otherwise mark **UNKNOWN**. Obtain explicit approval for any changes, builds, tests, flash, sound changes, release or merges. Never modify `main`, original accepted R18 or accepted R19.

**Continuity:** The same current handoff is bundled with the next private KnowledgeBase ZIP for starting a new project chat. The GitHub document is canonical for the planned investigation's *scope*; the private English KnowledgeBase remains the single authoritative private cross-chat memory.
