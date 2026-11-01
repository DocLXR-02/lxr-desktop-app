# Proposed R11 target measurement task and owner protocol

Date: 2026-11-01. Design only; **not implemented, built, installed or approved for flashing**. Reference source: 64263ed25f2e9c7f5d880b493d943bfba29271a8. Read with the [audit](2026-11-01_r11-resource-reliability-audit.md). No claim of target timing, safe stack depth or temperature follows from this proposal.

## Preferred bounded implementation

Create a separate opt-in diagnostic branch only after authorization, retaining the exact audible R11 reference artifact. Keep normal delivery diagnostics OFF. Use ordinary application flash for new helpers where practical; no relocation of existing hot code, compiler-flag change, watchdog, MPU, automatic reset, DSP redesign or new processor instance. Any startup-only stack probe is a separately reviewed part of this diagnostic task, not a casual C loop.

The existing LXR_RUNTIME_DIAGNOSTICS flag has no UI/export and is OFF in delivered R11. Its render interval excludes work before the mixer call; TIM2 interval excludes prologue/epilogue and surrounding instructions; callbacks sample a shared flag without generation correlation. It cannot certify full ISR latency, missed generations, all hardware DMA errors, MIDI drops, actual stack margin or thermal state. Object-only ON analysis gives 72 B metric BSS and 308 B helper code, with callback ITCM increasing from 160 to 212 B before link. These are raw object costs, **not a linked diagnostic firmware size**.

### Fixed record, ownership and coherent display

Proposed initial ceiling: at most **768 B live record +768 B static display snapshot in general SRAM**, no heap or variable-length logging. This is a design budget to enforce at compile/link, not a measured allocation. If generation coverage needs more, report the new budget before expanding it. Include version/source tag, run ID, clock-calibration status, uptime, latched error bits, saturating uint32 event counts and maxima. No printf, SD writes, LCD operations, allocation, long loops or synchronization waits in audio/IRQ paths.

Give each metric one defined writer or protect its short update from a documented higher-priority writer; volatile alone is not synchronization and saturating read/modify/write is not atomic across preemption. Unsigned wrapping generation IDs are distinct from saturating event counts. Snapshot in foreground into a fixed buffer using a brief measured PRIMASK-protected copy, restoring the previous interrupt mask rather than blindly enabling IRQs. No formatting/scan/I/O while masked. Bound and report its worst masked time; make snapshot frequency low (e.g. at most once/second or on page entry), and count any deadline interference. A lock-free/per-owner alternative is possible only if its consistency and bounded retries are proved; do not call a set of independent volatile reads a globally coherent snapshot.

Latched faults stay until explicit stopped-playback “new run” confirmation; changing pages must not clear maxima. Show overflow/saturated and UNKNOWN states explicitly. Read-only status pages use ordinary menu/encoder navigation; do not remap performance keys. The present firmware has none of these pages. Proposed pages, each split as needed for the physical two-line display:

| Page group | Owner-visible data |
|---|---|
| Identity/run | R11 diagnostic ID, run number, elapsed time, clock valid/unknown |
| Audio deadlines | max render cycles/us, max request-to-ready, late/stale count |
| DMA A / DMA B | TC count, missed generation, rearm failure, hardware DMA error, last slot |
| Interrupts | maxima for full TIM2 and both DMA handlers; nesting/overlap flags |
| Queues | UART/front/USB drop/full/BUSY and max occupancy |
| Stack | painted margin, minimum sampled MSP, probe status/limitations |
| Environment | measured temperature method/status, optional value; calibration flags |

Provide a one-page legend with units and abbreviations so iPhone photos are interpretable. A photo/video sequence of fixed pages is the preferred first export: it does not contend with SD/audio storage. An optional later SD snapshot must be a bounded foreground action while playback is stopped, using existing storage ownership/checked transaction APIs and a fixed small record. Never log every block or invoke SD from IRQs. Count total time and any audio interference even when stopped playback still renders tails. Repeated snapshot requests cannot build an unbounded queue. No assumption is made that existing SD code has a safe live logging path.

### Freshness for both physical streams

Instrument the **actual buffer addresses and half slots**, not merely bCurrentSampleValid. For each SPI2/SPI3 stream/half record requested generation, render-start generation, ready generation and generation armed for DMA, with timestamps. A shared render produces both buffers but each stream's completion/arm is checked independently.

1. At actual consumption/rearm, verify the half being armed has the required completed generation and is not currently being written. Count stale, partial, skipped and duplicate generations separately; latch stream/slot/request IDs for the first failure.
2. At request, record the expected render generation and request timestamp. At render start, claim that generation; at ready, publish the same generation, then compare whether a newer request arrived. Diagnostic observations must not repair/drop/reorder scheduler requests in this first measurement task.
3. Count actual DMA transfer/direct/FIFO error callbacks and I2S error state, HAL rearm return failures, and absence of expected callbacks. Neither a successful HAL rearm nor SAMPLE_VALID proves fresh samples. Review normal-mode HAL callback registration, request clearing and stream phase.
4. Measure request→start, start→ready and request→ready, along with TC→rearm and per-stream TC cadence/phase. Separate missed request, late finish and stale consumption. Use modular generation comparison with a bounded outstanding-window contract; inject ID wrap before target use.
5. Detect render re-entry/depth and foreground/ISR overlap without changing scheduling. Track whether work ran from foreground or TIM2 during storage. Any unexplained depth is a failure to investigate, not a counter to normalize away.

MIDI producer full conditions need observable counters at the actual producers, including the current ignored FIFO returns. USB TX invalid ranges/lifetime must first receive a separately authorized disposition; counting an out-of-bounds request is not an acceptable transport implementation. A bounded diagnostic drop policy must not silently change musical/control behavior. Keep queue corrections distinct from instrumenting an unchanged reference.

### Timing and overhead

DWT CYCCNT at nominal 216 MHz wraps in about **19.884 seconds**. Unsigned subtraction handles a short interval crossing the wrap only when actual duration is less than 2^32 cycles. A >one-epoch outage can alias a short interval; it must not appear healthy. Use an independent coarse uptime/epoch observation, maximum-gap validity flag and explicit assumptions. Saturating counts avoid rollover into “no errors”; generation/tick wrap gets boundary tests. A long run need not stop every CYCCNT wrap, but the epoch logic must remain valid through it. Do not use unrelated absolute float deadlines to extend the counter.

Read and report the actual core clock/dividers and compare CYCCNT over a long known timer interval. Independently verify sample-clock/block cadence and relative phases of both streams; nominal 64/44140.625 ≈ **1.44991 ms /313181 cycles** is a starting estimate, not the deadline at every polling IRQ. PLL/register knowledge alone does not calibrate oscillator accuracy. A scope/logic analyzer or debugger trace can establish absolute timing if available; otherwise report remaining clock accuracy uncertainty.

Measure full render and whole selected interrupt paths with matched entry/exit instrumentation, covering all returns and rearm work. Distinguish time spent preempted from exclusive body work. Timing inside a C handler does not include hardware exception entry, vector dispatch, compiler prologue/epilogue or stacking. To account for full ISR service, use separately reviewed minimal entry/exit wrappers and measured/architecturally bounded hardware and FP context overhead, or hardware trace. Keep these categories visible; do not relabel the old inner TIM2 hooks “full ISR.” Request-to-consumption generation correctness is the end-to-end criterion even if internal duration counters look small.

Calibrate empty hooks, DWT reads, snapshot masking, normal/cache-cold calls, FPU lazy/eager context behavior and nested interrupt overhead on the actual unit. Compare matched workloads with diagnostics enabled/disabled, using external timing where possible. Report raw instrumented maxima and overhead estimates separately. Instrumentation-caused misses remain misses; subtracting overhead must not erase a failed run. No host/A15 emulator timing is an M7 utilization measurement.

### Safe stack method

The runtime MSP starts at 0x20020000; current static DTCM ends 0x2001a698. There is **no present high-water instrumentation**. Sampling MSP at chosen hooks only gives sampled minima, potentially missing deep callees/interrupt frames.

Preferred future method, only after startup/bootloader/IRQ review: a stack-free assembly paint of exactly the linker-defined unused DTCM gap before any C calls or possible interrupt stacking, after correct region initialization. Prove interrupt state, current MSP and all bounds; preserve bootloader vector/SP expectations. Never paint initialized/static DTCM, active frames, DMA data or arbitrary memory. The current Reset_Handler relocates SP before SystemInit; a C function called later cannot safely paint the entire nominal gap. If the pre-C/interrupt preconditions cannot be proven, omit painting and mark the result UNKNOWN pending debugger work.

Combine the approved watermark with minimum-MSP sampling and compiler/nesting analysis. Lazy FP reservation may lower MSP without writing every reserved word, so untouched paint alone can overstate margin. Include hardware exception frames, alignment, FPU context, library/indirect calls and intended worst interrupt nesting. Scan a bounded chunk in foreground outside the active stack, never in the audio path; avoid interrupt-masked full-gap scans. Any scan must tolerate concurrent lower-MSP activity without touching it. Do not install an automatic reset/MPU guard as a side effect. Retain diagnostics overhead and stack use in the margin.

Debugger alternative: SWD/ETM trace/cycle facilities and MSP/stack-range inspection, with actual bootloader/load identity, external sample-clock measurement and controlled interrupts. A paused instrument cannot prove uninterrupted audio deadlines; live trace plus generation markers is preferable. A breakpoint SP value is not high-water. This alternative needs equipment beyond the iPhone-only workflow and separate hardware authorization.

### Temperature

ST DS11532 Rev9 §3.43/§6.3.25 describes the die sensor on ADC1_IN18, shared with VBAT, and factory calibration around 30°C/110°C at 3.3 V. Its sensor startup/sample timing matters; the specification emphasizes temperature changes more than precision absolute readings. Current ADC1 scans the fader/pot channels with DMA and does **not** configure this sensor. Adding it could change scan cadence and fader behavior, so it is not a free diagnostic channel or silently authorized implementation detail.

For the first protocol, record ambient, enclosure/ventilation, supply and duration and use a calibrated external measurement if available. Otherwise thermal gate stays UNMEASURED. A later die-sensor proposal must separately review ADC scheduling, Vref correction, TS_CAL1/2, startup/sample timing, calibration and placement/ambient correlation; no thermal certificate follows from a displayed uncalibrated value or touching the case. Do not add an ADC scan entry in this audit.

## Proposed owner protocol after separate approval

Nothing below directs an immediate flash or hardware test. The owner first approves a reviewed diagnostic candidate, verified package/hash, backup and recovery plan. Preserve the existing successful R11 image and a backed-up/spare SD card with known FILTER-OFF kits. Do not silently edit kit filters, run destructive storage tests or infer the installed bootloader erase sequence from repository code.

Before testing: document hardware revision, power supply, firmware/hash/diagnostic ID, bootloader version if safely obtainable, sample clock/calibration status, ambient/enclosure, kit/pattern IDs, routing and existing all-six FILTER-OFF settings. Keep monitor levels conservative and a physical stop/power/recovery method available. Confirm counters start a fresh run and take baseline page photos on the iPhone.

| Stage | Proposed duration | Workload and observation |
|---|---|---|
| Instrumentation sanity | 2–5 min | Cold silence; solo Voice 6; no storage; both stream counts/cadence, valid DWT, no false stale flags; record baseline overhead |
| Solo musical reference | 5–10 min | Full fader, MIX5/10/20/127, fader sweeps/zero/return, sparse tails, dry/wet/VOL endpoints; then drum/snare including approved finite saturation case |
| Bounded stress | 30 min | All six paths dense/velocity/bright PWM/transient settings with filters already OFF; six routes plus silent-high fader, direct routes, route/FX switches; snapshot maxima periodically |
| Interference | 30 min after prior pass | UI/encoders, pattern/kit/morph/automation, UART/USB bursts with known queue disposition; bounded non-destructive SD load/save on spare backed-up card; foreground and TIM2 render ownership |
| Endurance | 2 h after earlier passes; 24 h only if justified | Repeat representative dense/sparse/quiet/return cycles, photograph snapshots and environment periodically, verify no unexplained drift, accumulating errors or margin loss |

Known unstable-filter configurations are a **separate numerical-failure investigation**, not a stress test expected to pass. Do not enable them during this safe-filter capacity run. Until corrected/dispositioned, Delay mono-right and Compressor mono makeup over-range cases likewise remain negative tests, not approved passing performance workloads. FILTER-OFF does not protect those unrelated paths; CLP-only performance results must be labeled accordingly. Bright seam sounds test the declared arithmetic sound exception, not CPU headroom by ear.

Stop a run for any unexplained stale/partial/missing generation, DMA/rearm error, render re-entry, invalid clock epoch, nonfinite output/state, unexpected restart, MIDI drop beyond a declared bounded stress policy, stack watermark breach or margin trending toward the reviewed bound, abnormal heat/power behavior or audible discontinuity requiring diagnosis. Record run/page/phase before restart; do not keep increasing load to “find a pass.” Do not choose an arbitrary safe temperature or stack threshold without the actual part/board and measured uncertainty.

Acceptance requires zero unexplained generation and DMA faults under declared loads, bounded and understood queues, all known numerical exclusions explicit, worst measured render/request/ISR peaks plus justified preemption/cache/measurement reserve within the **actual** phase deadline, and measured stack margin exceeding the completed worst-nesting analysis plus justified uncertainty. A fixed 20–30% reserve is not universally sufficient. Derive reserve from measured peak variability, interrupt/SD interference, cold paths, instrument overhead and untested worst cases, then challenge it with targeted tests. Zero recorded misses alone is insufficient when the detector cannot see freshness failures.

Final owner actions are limited and concrete: approve the diagnostic task/package separately; prepare the backup/spare card and selected unedited FILTER-OFF kits; record the identity/setup; run the staged plan; photograph each fixed page at baseline/phase end/failure; return photos and exact workload notes. The developer analyzes those observations before authorizing a longer run or any multi-instance prototype. No independent per-voice wet mixing, thermal capacity or processor-count promise is made here.

## Bounded next tasks, in order

1. Review and authorize separate minimal robustness corrections: USB bounded TX/RX ownership/drop handling, Delay mono-width access, CLZ-zero defined compatibility and Compressor mono conversion policy; preserve R11 artifacts and approve any audible exceptions. Keep filter redesign and old direct snare policy separate.
2. Optionally authorize a one-change unused-FxBus-wrapper placement experiment with matched compiler/maps/tests; report actual recovered capacity, not a forecast.
3. Authorize the diagnostic-only branch above with fixed record/UI budget, generation correlation, timing/snapshot/stack safety proofs and host boundary tests. Build/package only under that separate task; ensure normal build remains OFF.
4. After separately approved target measurements close the gate, choose a two-slot pool/group prototype or true independent-track source architecture, explicitly deciding hat separation, tails/dry/pan/MIX/fader/chain semantics. Do not silently replace the full seven-track chain objective with a smaller pool or infer its realtime capacity from this audit.
