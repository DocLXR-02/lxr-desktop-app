# LXR build evidence and effect cost ledger

Date: 2026-11-01 (Europe/Vienna). Current synchronization: LXR-SYNC-2026-11-01-01. This ledger incorporates the completed offline audit; the original comparison used existing builds. No new production firmware, build or flash is introduced by publishing this document.

## 1. Correction: reports are not unique to R4

The preceding response singled out the R4 report as additional provenance for the R4 number. It should not have suggested that later builds lack reports.

The live GitHub branch inventory contains dedicated R5, R6, R7, R8, R9 and R10 build reports, plus a corrected-R10 delivery report. Earlier memory/USB/storage/Morph work also has detailed reports. This review retrieved 31 relevant reports and inspected their scope, outcome and resource evidence, alongside current compiled artifacts. This is not a rerun of all the historical test commands.

The earlier R11 report branches record blocked prerequisites, before a candidate existed. They remain historical reports. The final report for source `64263ed25f2e9c7f5d880b493d943bfba29271a8` is now [published](https://github.com/DocLXR-02/lxr-desktop-app/blob/84bc8a8c81fbc3d348c509ee5a2c7d5329d4cf8c/docs/codex-berichte/2026-10-31_r11-arithmetic-and-filter-off-post-mix.md); the documentation gap is closed. The new task reconstructed evidence from identifiable source/build artifacts rather than claiming unrecorded commands from the interrupted session. The [offline resource/reliability audit](https://github.com/DocLXR-02/lxr-desktop-app/blob/fa3c5b711733d022301669f0bdacd650cde5d6d9/docs/codex-berichte/2026-11-01_r11-resource-reliability-audit.md) and its [input addendum](https://github.com/DocLXR-02/lxr-desktop-app/blob/3ca14c236a2301b3883b6ebd377f9dee68385d4e/docs/codex-berichte/2026-11-01_r11-resource-audit-knowledgebase-addendum.md) are complete within their stated scope. They do not close device timing, stack high-water or long-run gates.

Every new build intended for delivery should have a report. Existing project rules already require it. A report marked NOT HARDWARE VALIDATED can coexist with a later owner-confirmed listening test: the report describes its execution date; a new dated hardware observation should supplement it.

## 2. What each evidence type answers

| Evidence | Content / use | What it does not establish alone |
|---|---|---|
| Changelog | What changed and why the user should care | Exact generated code or runtime cost |
| Build/handoff report | Base/head, source files, reasoning, commands, tests, failures, toolchain, CI, sizes/hashes, resources and limits | Physical device timing unless measured |
| Compiler/linker log | Compilation outcome, warnings, linking and region fit | Musical correctness or all runtime behavior |
| ELF and map | Actual sections, symbols, object contributions, load/run addresses, generated instructions | Actual worst-case cache/IRQ timing |
| Native test results | Reproduction, arithmetic, state, bounds, transitions and persistence within the fixture | M7 timing or physical SD/codec behavior |
| Owner listening result | Sound and usability on the stated device/setup | Measured CPU/stack/temperature reserve |
| Target profiling | Cycles, request deadlines, ISR interference, errors and stack use under stated workload | Every untested setting or lifetime guarantee |

Reports are therefore suitable for technical research. They must be read with their identity, compiler, scope, chronology and raw artifacts. They already expose material costs that a static-size table misses: the R10 fader-replay helper's own stack frame grows from 48 to 168 bytes, **+120 runtime stack bytes**, despite zero static DTCM increase.

## 3. Accounting conventions

- All quantities below are **bytes**, unless stated otherwise. KiB means 1024 bytes.
- ITCM = fast instruction RAM; DTCM = fast data RAM and live runtime stack; general SRAM = other static data/heap; flash payload = the complete application load image.
- Flash payload includes initial images copied to ITCM and initialized DTCM. It is not just the linker's FLASH-summary number. Do not add overlapping categories as if they were disjoint parts of one memory chip.
- A zero static-region delta does not mean zero CPU, stack, I/O or behavior cost.
- Compare matched builds for a change. A different compiler or independent branch is not a clean causal baseline.
- Distinguish a module's linked bytes, an instance's state, and the marginal whole-firmware cost of adding/removing an effect. Shared helpers/libraries/veneers and alignment prevent equating these automatically.

## 4. Earlier changes before R4

The following normal diagnostics-OFF resource deltas are taken from each corresponding report's own before/after comparison. Earlier static ITCM is 15016 and DTCM 107392 throughout this table. Complete flash deltas equal linker FLASH deltas here because the initialized DTCM and ITCM load sizes do not change.

| Change / predecessor | Technical nature | ITCM delta | DTCM static delta | General SRAM delta | Complete flash delta |
|---|---|---:|---:|---:|---:|
| B / A: bounded `_sbrk`; optional diagnostics OFF | Bounds hardening | 0 | 0 | -8 | +72 |
| C / B: fixed USB class storage instead of interrupt-context allocation | Structural mitigation of the identified allocator overlap | 0 | 0 | +544 | +312 |
| S1 / C: SD fault latch and finite driver waits | Failure containment | 0 | 0 | 0 | +920 |
| S2 / S1: SD HAL/BSP timeouts and failure propagation | Error/timeout correction | 0 | 0 | 0 | -1320 |
| S3 / S2: FAT/FSInfo/sync error checks | Integrity correction | 0 | 0 | 0 | +96 |
| S4a / S3: checked individual saves and ownership | Save-path hardening | 0 | 0 | 0 | +1792 |
| S4-L / S4a: retry classification and live-safe error feedback | Recovery-policy/UX correction | 0 | 0 | +8 | -144 |
| S4-L2 / S4-L: bounded silent autosave retries | Recovery-policy change | 0 | 0 | +16 | +1312 |
| S4b-1 / S4-L2: project containment and validation | Bounds/ownership hardening; interim restrictions | 0 | 0 | 0 | +1664 |
| S4b-2 / S4b-1: staged project replacement/recovery | Storage architecture change | 0 | 0 | +3040 | +8328 |
| R3 / prior delivery: Morph 0–127 | Explicit UX/design change | 0 | 0 | 0 | +224 |
| R4 / R3: Kit/Morph Kit success feedback | UX feature | 0 | 0 | +8 | -160 |
| **A to R4 total** | Sum along this reported sequence | **0** | **0** | **+3608** | **+13096** |

The independent BUG001 and Euclidean fixes precede integrated phase A. Their reports exist and contain absolute build figures, but these do not by themselves provide a clean, sequential per-fix cost relative to the official 1.71 binary. The integration report compares two different corrected configurations; its +8 ITCM and -1120 raw flash difference must not be mistaken for a universal cost of BUG001 or Euclidean correction.

The official-binary-to-R4 ITCM difference is +464 bytes, with possible compiler/configuration differences as already documented. No allocation of those 464 bytes among individual early fixes is invented.

The historical D0 audit stopped before source implementation. It must not be counted as a correction or a resource-increasing firmware revision. Likewise, phase R2 packaging is a delivery/rebuild operation, not a new audio algorithm.

Before R4 was not uniformly system-neutral: Morph travel and feedback were deliberate UX changes; USB moved allocation into fixed storage; storage safety changed failure policy and write sequencing. Their intended normal audio behavior may remain unchanged while resource use and exceptional behavior change materially.

## 5. CLP development: exact matched deltas

| Change / actual baseline | ITCM delta | DTCM static delta | General SRAM delta | Complete flash delta |
|---|---:|---:|---:|---:|
| R5 / same-toolchain R4 rebuild | +88 | +8 | 0 | +752 |
| R6 / R5, rejected full-body experiment | 0 | +16 | 0 | +224 |
| R7 / R5, selected parallel-bass alternative | 0 | +16 | 0 | +248 |
| R8 / R7, audible fader following | +504 | +256 | 0 | +2808 |
| R9 / R8, revised attenuation curve | 0 | 0 | 0 | +72 |
| Original R10 / R9, routed-fader maximum | 0 | 0 | 0 | +136 |
| Corrected R10 / original R10, startup display | 0 | 0 | 0 | -56 |
| R11 / corrected R10, arithmetic prerequisite plus solo post-MIX | -48 | +512 | +8 | +1252 |
| **Selected R4-to-R11 path, excluding discarded R6** | **+544** | **+792** | **+8** | **+5212** |

R6 and R7 are alternatives branching from R5. Summing both would double-count an abandoned experiment. R10 total relative to R9 is +80 flash after the display correction. The R4 comparator was rebuilt with the CLP toolchain; its full payload is 286112 bytes, versus R11 291324. Do not subtract the differently compiled original delivered R4 image to estimate CLP source cost.

The selected-path sums are reproducible:

```text
ITCM: 88 + 0 + 504 + 0 + 0 + 0 - 48 = 544
DTCM: 8 + 16 + 256 + 0 + 0 + 0 + 512 = 792
SRAM: 0 + 0 + 0 + 0 + 0 + 0 + 8 = 8
Flash: 752 + 248 + 2808 + 72 + 136 - 56 + 1252 = 5212
```

This is the CLP development branch's total cost, including the technical source-arithmetic prerequisite. It is not a pure byte price of creativity. Storage transaction S4b-2 alone costs more flash (+8328) and general SRAM (+3040) than this selected development path, but uses no additional ITCM. Costs depend on implementation and memory placement, not whether the task was called FIX or FEAT.

## 6. Existing R11 effect modules: linked code and state

The module-code figures below were summed from retained **input sections** in the actual R11 map after the live link map begins. Discarded sections, debug/comment sections, shared library/helper code, linker veneers and inter-section padding were excluded. This is an exact defined subtotal, **not a complete marginal removal/addition cost**.

| Existing module | Own code in ordinary flash | Own code in ITCM | Per-instance struct size | Other currently allocated data |
|---|---:|---:|---:|---|
| DRV: tube/fold/CLP combined | 6252 | 32 | 176 | 2060 initialized DTCM bytes for shared distortion tables |
| Ring modulation | 380 | 0 | 44 | Uses shared oscillator code/tables, excluded from this module subtotal |
| Compressor | 1136 | 0 | 32 | Shared math-library dependencies excluded |
| Delay, normal and ping-pong combined | 3396 | 0 | 96 | 65536 DTCM bytes in the separate global FX_BUFFER |

The 6252 bytes cannot be called CLP-only: all three distortion modes share the processing entry and helpers. Similarly, normal/ping-pong delay are grouped. Zero own ITCM bytes does not mean no calls into shared ITCM helpers. Ordinary-flash functions execute using the processor's memory/cache architecture; code location does not imply an effect is inactive or computationally free.

Shared FX infrastructure in R11:

| Allocation | Bytes | Scope |
|---|---:|---|
| FxBus own ordinary-flash code | 480 | Initialization, fader replay and FX change handling |
| FxBus own ITCM code | 1156 | Both process entries, parameter update/set |
| Complete FxBus state object | 376 | Already contains all four per-effect state structs; do not add them twice |
| SoloMix ordinary-flash code | 860 | Solo full-source / post-return handling |
| SoloMix general-SRAM metadata | 8 | Selection/target metadata across data/BSS |
| SoloMix DTCM workspace | 512 | 64-float post levels and 128-int16 isolated return |
| Mixer CLP gain buffer | 256 | Per-frame fader gain storage |
| Whole-engine linker stubs in ITCM | 544 | Shared across many functions, not all attributable to FX |

FxBus state sum: 176 + 44 + 32 + 96 = 348 bytes for the effect structs; control fields and alignment add 28, for 376 total. All four state structs are present in the current fixed object even when only one algorithm is selected. Selecting another current effect does not itself allocate another copy of its code.

### State-layout verification

The exact structs were extracted from the pinned headers (comments/includes removed, disabled reverb excluded), then compiled in a local native **layout-only probe**, not a firmware build or audio test:

```text
cc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
cc -std=c11 -Wall -Wextra reserve-audit/effect-layout-probe.c \
   -o reserve-audit/effect-layout-probe
reserve-audit/effect-layout-probe
```

Result, exit 0:

```json
{"OnePoleFilter":16,"OscInfo":40,"MasterDistortion":176,"Ringmod":44,
 "Compressor":32,"Delay":96,"FxBus":376,
 "dist_offset":24,"ring_offset":200,"comp_offset":248,"delay_offset":280}
```

These pointer-free structs use the relevant native sizes/alignments also expected for this Arm ABI. The four parent offsets match actual R11 `FxBus_init` instructions, and 376 matches the actual target ELF symbol size. This is a cross-check of the layout calculation, not a new GCC14.3 target compilation. In particular, the Ringmod struct is **44**, not the 48-byte parent-offset gap: the next member requires four alignment bytes. Delay is **96**, including its eight-byte alignment because OnePoleFilter holds doubles.

## 7. Instances, simultaneous rendering and unknown timing

| Number of independent instances | CLP state bytes | Additional CLP state versus one | Delay-history bytes at current per-instance capacity |
|---:|---:|---:|---:|
| 1 | 176 | 0 | 65536 |
| 2 | 352 | 176 | 131072 |
| 4 | 704 | 528 | 262144 |
| 7 | 1232 | 1056 | 458752 |

These are arithmetic state/history totals only. They exclude routing, per-instance controls outside those structs, working buffers, stack, shared dependencies and timing. Independent delay histories also require refactoring the current global-buffer access. Existing code does not become instance-safe merely by creating more structs.

The current R11 implementation still processes one shared FX selection. Multiple CLP/FX instances could reuse a function body while being processed sequentially within every audio block and heard simultaneously after mixing. They are not independent CPU cores.

The number of simultaneous active instances cannot be computed from 824 free ITCM bytes or from 176 state bytes. Nominal 64-frame duration is about 1.44991 ms at 44140.625 Hz; the audio calculation must share the relevant scheduling window with other work. No per-effect M7 cycle budget or combined worst-case timing was measured in these reports. Therefore **the maximum independent simultaneously rendered instance count remains unknown**.

The eventual measured model must separate base synthesis/sequencing, active instance costs, routing, tails, cache/interrupt interference and diagnostic overhead. Per-instance timing need not be constant across mono/stereo modes, parameters, transitions or input conditions. Measuring a small isolated routine is insufficient to certify the complete engine.

## 8. Improvement, hardening, workaround and design

| Term | Engineering meaning | Project example | Interpretation |
|---|---|---|---|
| Root-cause correction | Fix the identified invalid operation | Reserved NRPN/CC0 and index guards; row-local wavetable boundaries | Validity improves; edge-case output and generated size can change |
| Hardening | Bound failure consequences or reject invalid operations | `_sbrk` bounds, checked I/O, transaction ownership | Often additional state/code; does not repair every physical fault |
| Structural change | Change ownership/architecture to remove an identified hazard | Fixed USB class storage instead of asynchronous heap allocation | +544 SRAM and +312 flash in the recorded comparison |
| Workaround / scope restriction | Avoid an unresolved failing path | All six voice filters OFF for R11 experiments | Does not repair filter instability; no added code cost, but removes filter behavior and can change load |
| Intended behavior/UX change | Choose different musical/control semantics | Morph 0–127; R9 gain curve; R11 post-MIX behavior | Product decision, not automatically a manufacturer bug |
| Parallel signal compensation | Combine a retained processor with another path | R7 bass addition outside CLP feedback | Deliberate sonic tradeoff; does not make the CLP feedback intrinsically preserve bass |
| Idle-feedback policy | Change when an otherwise oscillating effect is allowed to sustain | R5 inactivity detector | Deliberate change to documented original self-oscillation, not proof the original was a broken safety mechanism |
| Optimization | Reduce a defined cost; an audible tradeoff is allowed only with explicit owner approval | Candidate retained FxBus wrapper cleanup, or an explicitly approved algorithmic approximation | Quantify resource gain and behavioral differences before accepting the tradeoff |

"Improvement" describes the chosen benefit, not a memory category. A workaround may be the preferred practical design; a root-cause fix can cost resources. Conversely, adding a function can reduce linked size through compiler/layout changes. Each claim needs its acceptance criteria.

## 9. Two-pass development standard requested by the owner

The owner's requested standard is recorded here and was included in the completed audit task:

| Pass | Primary objective | Required evidence |
|---|---|---|
| 1. Functional / musical | Make the intended behavior work and preserve the declared boundaries | Pinned baseline; focused tests; build fit; listening where needed; initial cost ledger |
| 2. Efficiency / robustness | Review all material resource costs after behavior is accepted | Code/ITCM/DTCM/SRAM/flash deltas; stack/heap; per-block work; numeric stability; timing/IRQ/DMA; tails/transitions and long-run behavior |

Owner clarification: exact preservation of the prior sound is not an absolute condition for an efficiency improvement. Audible deviations are permitted when they are explicitly described, compared with the reference, weighed against the measured efficiency gain and approved by Daniel before acceptance. The second pass must not silently alter the accepted sound. Preserve the pre-optimization artifact, compare the same workloads/toolchain, and document expected numerical differences or audible tradeoffs separately. Do not optimize arbitrary unrelated subsystems or turn every low-impact UI edit into an expensive full hardware campaign.

Every deliverable build report should contain a compact ledger with actual baseline/head identities, code and data deltas, shared versus per-instance allocations, transient stack/scratch use, measured timing or explicit UNMEASURED, known limitations and artifact identities. Before expanding simultaneous processors, target deadline and stack measurements are a gate rather than an optional estimate.

The completed audit found no production source caller, decoded call/tail reference or reference relocation in its analyzed objects for the retained 384-byte legacy FxBus entry. It remains an optimization candidate, not a booked saving. No removal experiment or source optimization has been performed.

## 10. Local analysis outputs and limits

Current analysis files include the retrieved report snapshots, `effect-module-allocation.json`, `effect-state-layout.json`, `change-deltas.json` and the preceding release-resource/symbol-history data. The allocation parser excludes discarded map sections and uses retained object input sections; the explicit module totals were asserted against the recorded section sums. The layout-only native probe succeeded and target offset/size consistency checks passed.

No historical test result was promoted to a current rerun. No device profiling, soak test or final numerical bound on active effect count was obtained. No new firmware image or release was produced. The separate offline Codex resource/architecture audit has completed and published its reports. No diagnostic implementation or additional development task is started by this synchronization.

## Appendix: retrieved report inventory

The following report files were retrieved from their respective report-only branches for this comparison. Scope/outcome/resource sections were inspected; historical commands were not rerun. Blob SHA identifies the report content retrieved.

| Report | Git blob SHA |
|---|---|
| [2026-10-28_kit-load-confirmation.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-kit-load-confirmation/docs/codex-berichte/2026-10-28_kit-load-confirmation.md) | `fe73637493b03c200b241e77559e3fa768606783` |
| [2026-10-28_morph-range-127.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-morph-range-127/docs/codex-berichte/2026-10-28_morph-range-127.md) | `5cee9bc2e479da3e48836b1b294ed009ca9e25d3` |
| [2026-10-28_phase-r1-hardware-readiness.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-phase-r1-hardware-readiness/docs/codex-berichte/2026-10-28_phase-r1-hardware-readiness.md) | `036253ab39ebc7de29bc4136eea56f0dda3ed9ae` |
| [2026-10-28_phase-r2-verified-firmware-packaging.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-phase-r2-firmware-delivery/docs/codex-berichte/2026-10-28_phase-r2-verified-firmware-packaging.md) | `50c288dc58046297238c5392972df8cee673be48` |
| [2026-10-28_r3-morph-firmware-package.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-r3-morph-firmware-package/docs/codex-berichte/2026-10-28_r3-morph-firmware-package.md) | `3ca96aa03f5a59dbc54052b65a6c5f1ed4e6c2f6` |
| [2026-10-28_r4-firmware-package.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-r4-firmware-package/docs/codex-berichte/2026-10-28_r4-firmware-package.md) | `05d2a8b1e15d4ccda6fe3c438408cdf5e8b4a5a6` |
| [2026-10-29_r5-clp-independent-review.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r5-clp-independent-review/docs/codex-berichte/2026-10-29_r5-clp-independent-review.md) | `eb4d036e6064797b6cdf267ac25092158c955fdb` |
| [2026-10-29_r5-clp-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r5-clp-test-build/docs/codex-berichte/2026-10-29_r5-clp-test-build.md) | `21ca5c9a32237502edbc95a2732b66cc56089180` |
| [2026-10-29_r6-clp-shared-core-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r6-clp-shared-core-test-build/docs/codex-berichte/2026-10-29_r6-clp-shared-core-test-build.md) | `45d29766d26b99f13777d41becf795465cdb2cde` |
| [2026-10-30_r7-clp-parallel-bass-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-30-r7-clp-parallel-bass-test-build/docs/codex-berichte/2026-10-30_r7-clp-parallel-bass-test-build.md) | `195b1db07b7f3811c6ba08f04848c6785109b0a5` |
| [2026-10-30_r8-clp-single-voice-fader-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-30-r8-clp-single-voice-fader-test-build/docs/codex-berichte/2026-10-30_r8-clp-single-voice-fader-test-build.md) | `136686fb3ea58645f0a6cddcff8440ff46852b38` |
| [2026-10-30_r9-clp-fader-curve-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-30-r9-clp-fader-curve-test-build/docs/codex-berichte/2026-10-30_r9-clp-fader-curve-test-build.md) | `1d422a039342a0fc414d2d7144e3188652e72dd4` |
| [2026-10-31_r10-clp-multivoice-max-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-31-r10-clp-multivoice-max-test-build/docs/codex-berichte/2026-10-31_r10-clp-multivoice-max-test-build.md) | `852fbf1b7135a1b735aa46c1fad38eb0a953aeca` |
| [2026-10-31_r10-startup-display-correction.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-31-r10-startup-display-correction/docs/codex-berichte/2026-10-31_r10-startup-display-correction.md) | `8ae625b8794b2ac25967123163cb4955f367aae6` |
| [2026-10-31_r11-all-voices-headroom-and-post-mix.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-31-r11-all-voices-headroom-and-post-mix/docs/codex-berichte/2026-10-31_r11-all-voices-headroom-and-post-mix.md) | `1c236ec986160df5faca875d23227a34b33db802` |
| [2026-10-31_r11-clp-solo-post-mix-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-31-r11-clp-solo-post-mix-test-build/docs/codex-berichte/2026-10-31_r11-clp-solo-post-mix-test-build.md) | `5c1d6fdbde981608412c01a27ab643d424cebfc3` |
| [2026-10-26_bug001-validated-correction-publication.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-bug001-validated-publication/docs/codex-berichte/2026-10-26_bug001-validated-correction-publication.md) | `61463954da8b60580a25b0f2d311052d4ceaf870` |
| [2026-10-26_euclidean-length16-correction.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-euclidean-length16-correction/docs/codex-berichte/2026-10-26_euclidean-length16-correction.md) | `af77c8a2a088de272c86c6a4416be5450668a054` |
| [2026-10-26_phase-a-integration-safety-preflight.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-a-integration-safety-preflight/docs/codex-berichte/2026-10-26_phase-a-integration-safety-preflight.md) | `474ee90eeb24946bde178db259a5329de0100b50` |
| [2026-10-26_phase-b-memory-safety-runtime-diagnostics.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-b-memory-safety-runtime-diagnostics/docs/codex-berichte/2026-10-26_phase-b-memory-safety-runtime-diagnostics.md) | `62b6ee170196249dd8c59f3ae1a8726a09ef578a` |
| [2026-10-26_phase-c-usb-allocator-safety.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-c-usb-allocator-safety/docs/codex-berichte/2026-10-26_phase-c-usb-allocator-safety.md) | `02980d6b0de11fd9e43b2621c35a04e39653f9bf` |
| [2026-10-26_phase-d0-sd-storage-safety.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-d0-sd-storage-safety/docs/codex-berichte/2026-10-26_phase-d0-sd-storage-safety.md) | `75d483c6906f01ade4e43d8f9a2bbd578dc0a6b7` |
| [2026-10-26_phase-s1-sd-storage-foundation.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-s1-sd-storage-foundation/docs/codex-berichte/2026-10-26_phase-s1-sd-storage-foundation.md) | `e1c646c3e588df061ed4df183e87e5822aaa9f79` |
| [2026-10-26_phase-s2-sd-hal-bsp-timeouts.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-s2-sd-hal-bsp-timeouts/docs/codex-berichte/2026-10-26_phase-s2-sd-hal-bsp-timeouts.md) | `9b44e0ef9bc1c8585e04fbe05249ab5fc691b81e` |
| [2026-10-26_phase-s3-fatfs-data-integrity.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-s3-fatfs-data-integrity/docs/codex-berichte/2026-10-26_phase-s3-fatfs-data-integrity.md) | `88ab6565c42881762d47f9be819f9c50ae5e3e76` |
| [2026-10-26_phase-s4a-safe-file-saving-verified.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-s4a-safe-file-saving/docs/codex-berichte/2026-10-26_phase-s4a-safe-file-saving-verified.md) | `15916ed38af3af63d65538ea8933d91de67a05dd` |
| [2026-10-26_phase-s4a-safe-file-saving.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-s4a-safe-file-saving/docs/codex-berichte/2026-10-26_phase-s4a-safe-file-saving.md) | `54050291791db427e3678836b691633c38aa4605` |
| [2026-10-27_phase-s4b1-project-containment.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-27-phase-s4b1-project-containment/docs/codex-berichte/2026-10-27_phase-s4b1-project-containment.md) | `2d5411d023486ee8aa11b2957a754938e6700d8f` |
| [2026-10-27_phase-s4b2-project-replacement.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-27-phase-s4b2-project-replacement/docs/codex-berichte/2026-10-27_phase-s4b2-project-replacement.md) | `7686e79f361e9487a0ef52cbea6e316b67df568a` |
| [2026-10-27_phase-s4l-live-safe-storage-recovery.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-27-phase-s4l-live-safe-storage-recovery/docs/codex-berichte/2026-10-27_phase-s4l-live-safe-storage-recovery.md) | `64db7ab625457a4876483b522e9483d2fa352eef` |
| [2026-10-27_phase-s4l2-silent-autosave-recovery.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-27-phase-s4l2-silent-autosave-recovery/docs/codex-berichte/2026-10-27_phase-s4l2-silent-autosave-recovery.md) | `8bbcb4d454d31fd53c03c1dfc33be2a7bc719c0e` |



## Current audit completion and outstanding measurements

The qualified continuation comprised twelve separate ~120-second native processes, not a single24-minute device test. All filters were OFF; CLZ-zero behavior was substituted in the native harness; Delay was restricted to stereo and Compressor to unity makeup. Earlier strict failures remain failures. See [evidence](https://github.com/DocLXR-02/lxr-desktop-app/blob/fa3c5b711733d022301669f0bdacd650cde5d6d9/docs/codex-berichte/2026-11-01_r11-resource-audit-evidence.md).

Representative own compiler frames: mixer488 bytes; fader replay168 (R9 48, delta120); Distortion184. These do not form a whole-engine upper bound. Actual render deadlines, stack high-water and temperature remain UNMEASURED. The proposed diagnostics budget is768 bytes live plus768 bytes snapshot in general SRAM, with normal diagnostics OFF; it is a design budget, not an implemented linked-image cost.

The current [state](PROJECT_STATUS.md) and [hardware record](HARDWARE_VALIDATION.md) control chronology. No current-source numerical failure is silently reclassified as a passed device test.
