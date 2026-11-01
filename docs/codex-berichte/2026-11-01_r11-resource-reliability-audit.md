# R11 resource and reliability audit

Date: 2026-11-01 UTC. Repository: DocLXR-02/lxr-desktop-app.

## Decision and gates

R11 fits its linked memory regions. This audit does **not** establish device CPU capacity, a complete stack bound, thermal reliability, or permission to add simultaneous CLP instances. The owner reports successful intended solo CLP/MIX behavior on the instrument with all six voice filters OFF. That listening evidence remains valuable within its scope.

Offline work also found inherited source defects and transport/scheduling risks that an unconditional “reliability passed” statement would conceal. In particular, USB TX wrap can pass an out-of-buffer range to the driver, Delay mono-right performs an unaligned word read that can extend beyond the last DMA half, and strict full-FX initialization reaches CLZ(0) source undefined behavior. These are distinguished below from observed hardware malfunction. No production correction was made here.

| Gate | Result | Evidence and limit |
|---|---|---|
| Static region fit | PASS for delivered R11 | Verified ELF/image/map; 824 B ITCM free; stack use not included |
| Numerical scope | PARTIAL, known failures retained | FILTER-OFF CLP cases pass qualified tests; unstable filters, legacy snare overflow, CLZ(0), Delay and Compressor cases remain unresolved |
| Whole-engine stack bound | INCOMPLETE | Production-optimization compiler frame reports plus call graph; indirect/library/interrupt/FPU gaps remain |
| Native endurance | QUALIFIED PASS | Twelve 120-second processes, 24 minutes aggregate simulated audio; disclosed host substitution and effect restrictions |
| Actual audio deadlines/fresh generations | UNMEASURED | Source trace and injected late-render counterexample are not device measurements |
| Actual stack high-water margin | UNMEASURED | 22,888 B static gap is not a measured margin |
| Thermal behavior | UNMEASURED | No calibrated temperature or target workload evidence |
| Long-run instrument reliability | UNMEASURED | Boundary probes are not days of physical operation |
| Multi-instance/chain expansion | NOT APPROVED | Requires target measurement gate, robustness disposition and routing/product decisions |

Preferred next work is a separately authorized, bounded robustness pass for the confirmed inherited faults, followed by the diagnostic-only implementation specified in the [measurement plan](2026-11-01_r11-resource-measurement-plan.md). Do not expand DSP or silently change the approved sound during that work. A separate unused-dispatch placement experiment is justified, but its possible savings are not booked as available capacity.

## Identity, scope and provenance

- Audited source/base/head: **64263ed25f2e9c7f5d880b493d943bfba29271a8**, fx/clp-solo-post-mix-r11; tree ad5d81b94ede2af0ff776254e539c50be24c76e1.
- Corrected R10: 8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7. Manufacturer source: a43cd4fcda23ec3d577dc425261c3eb28bd5abff.
- R11 prerequisite reference: d949b3e132fd8afe5f076e6465aae1ffe507c938. R11 feature commit is the audited head above.
- Publication branch: codex-reports/2026-11-01-r11-resource-reliability-audit, based exactly on lab-setup **5829398cea5d337200393757e39a37d4d8148256**. Only this report, relevant Markdown evidence/design summaries and a Markdown-only ZIP are added. No source, tests, workflows, linker/startup, clocks, display, normal build configuration or musical behavior changed.
- Existing branch heads were captured and compared. No pre-existing head moved during the audit. The preceding authorized R11 handoff publication added its own report branch; this audit adds only its report branch. No firmware was linked or packaged for installation, no hardware was accessed and no flash/release/merge/force-push/settings action occurred.

Read AGENTS.md at the source pin and publication base, the four supplied audit/planning/ledger/task files, R5–R10 build reports, prior R11 blockers and ARM triage, and earlier integration/storage/UX reports. The supplied KnowledgeBase v1.2.0 sections 6.1/7 and the separately mentioned preceding R11 independent review were not available after requesting their location. This report does not claim to have read them or to be that missing review. Their absence does not prevent the byte-level and source checks reported here; any conflicting architectural knowledge still needs reconciliation.

Existing evidence was reused first. [CI 41214348060](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41214348060) succeeded, attempt 1. Image artifact [13047418960](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41214348060/artifacts/13047418960) and source/build evidence [13047144663](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41214348060/artifacts/13047144663) were downloaded and checked against their GitHub wrapper digests, then their internal manifest. All 432 archived source blobs match Git at the pin; 29 manifest files match. Artifacts have retention limits; the report links and source commits are the durable references.

| Existing artifact | Bytes | SHA-256 |
|---|---:|---|
| LXR_V2_R11.img | 291340 | f1779db7861d4c9a54aded8121eb5659a445256ee64453635fcfa4fb1593d2b0 |
| LXR_V2_R11.zip | 207732 | b31c93b5742732e1c280a4789b5ebde75e4354e7acf44cead54ec23a3fce8a30 |
| SOURCE.zip | 1787323 | 4c1b17d51fe290d0a42accadef9d7d30947b2f485910cea5c148cc838f17b9cf |
| LXR_V2_STM32F765.elf | 397620 | 350696b2365c973d07b2670a57c72950a65f9a818090e3599c8487f59d313043 |
| LXR_V2_STM32F765.map | 544145 | 2b7edab25607c147b06fa2b98d23375732b995a74c09adaaad7261ef114d7d28 |
| HOST_RESULTS.tar.gz | 6154837 | 3db412a4f3353ee40452031b286c23f72f95bd2742a7df516f94a768c30bd121 |

These are **existing R11 artifacts**, not a new audit delivery. Image checks reproduce LXRV2IMG magic, 291324-byte payload, internal checksum 0x0d, zero reserved bytes, 126 vector entries/115 nonreserved handlers, reset 0x08013555, application-only contiguous ELF load bytes and startup “LXR LAB R11” with blank second line. ZIP has only the image. The initial vector SP is 0x20080000; the first Reset_Handler SP load instead uses **0x20020000**. See startup source line 68 and delivered literal at 0x080135b0.

## Evidence classes and prioritized findings

“S” means confirmed source/control-flow evidence; “N” means native execution under stated fixture/flags; “A” means isolated execution/disassembly of delivered ARM ELF, not device execution; “I” means scheduling/boundary injection; “H” means owner observation; “R” means existing report evidence. Severity is practical prioritization, not a claim of observed field failure. All new defects below are inherited, not introduced by the R11 solo feature; selected function bodies were compared with manufacturer source. No fix is claimed.

### 1. USB output wrap passes an invalid buffer range — high priority, S/A

[usb_manager.c lines 94–129](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/USB/usb_manager.c#L94): the ring has 128 bytes, but len is uint8_t(write-read), then the whole range is sent from read. With read offset 124, write offset 0, len becomes 132. Actual delivered usb_tick, containing the inlined flush, calls the hooked CDC_Transmit_FS with buffer+124 and length 132. A second probe at read 120/write 4 requests 140 bytes. This exceeds the array; it is not just a lost-MIDI-message issue. The hook records the submitted range; it does not execute the USB peripheral or prove physical leakage/crash.

Also, read advances on BUSY: read 0/write 4/status BUSY consumes four unsent bytes. RX writes 64 unread events into a 64-event ring, making write equal read and the queue appear empty. RX component test uses actual USBD_CDC_DataOut and usb_getMidi, four 64-byte packets, mocked HAL/class plumbing; all 64 events then become unobservable. There is no full/drop telemetry. Source: [usbd_cdc.c lines 998–1013](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c#L998).

Bounded future correction: explicitly bounded contiguous TX chunks, packet-atomic producer/full policy, asynchronous in-flight buffer ownership, consumption only after the chosen accepted/completed contract, and counted RX/TX drops. Test wrap positions, exact-full/empty, BUSY/retry/disconnect/completion/producer interleavings and guards. Do not merely increase the buffer or add counters while leaving invalid ranges. No transport code changed here.

### 2. Known resonant-filter instability and legacy snare overflow — high priority, R/N scope boundary

The earlier all-six audit/ARM triage established nonfinite resonant-filter arithmetic for valid mapped controls; R11 deliberately leaves recurrence, coefficients, reset policy and fast-math unchanged. FILTER-OFF is an explicit temporary listening boundary, not a root-cause fix or automatic eligibility condition. Old failing filter reproductions remain failures and were not rerun merely to increase counts.

Likewise, the new eligible solo full-source snare conversion clamps finite peaks, but ordinary direct/multi-route/other-effect snare conversions retain known original over-range cases. The original R11 test evidence separates 98 baseline failures in the selected sweep and 11 supplemental legacy failures from R11 eligible-path passes. Do not promote these to passing universal firmware cases. Their exact configurations and arithmetic disposition remain in the immutable prior reports linked in the evidence appendix.

### 3. Delay mono-right word access — high priority, S/N/A-disassembly

[Delay.c line 264](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/DSPAudio/FX/Delay.c#L264), analogous ping-pong line 178: an int32_t pointer derived from the mono output is read as a word, although only a uint16_t is stored. Mono-right starts two bytes off word alignment. At the final sample of the second DMA half, that read can extend beyond the full 256-int16 DMA array. Native UBSan catches the actual production Delay mono-right path in scenario 2. The ELF contains LDR.W/QADD16/STRH for this path. Full Delay emulation stopped at unsupported vrintz.f32 at 0x0802485c; it is explicitly **not** an emulated complete Delay pass/failure.

ARM unaligned access tolerance and CCR.UNALIGN_TRP/boot state are unmeasured; neither can legitimize invalid C or the final out-of-array read. Future minimal correction should use the correct halfword-width mono read/add/store, preserving intended saturation and channel isolation. Test normal/ping-pong, both DACs, all six destinations, last DMA half guards and stereo compatibility. Continued soak restricts Delay to stereo and retains the failing mono case separately.

### 4. Compressor wet conversion exceeds int16 — medium/high priority, S/N

[Compressor.c line 180](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/DSPAudio/FX/Compressor.c#L180): the mono branch stores the floating wet sample directly into an int16_t scratch element before dry/wet and accumulation. The stereo branch already applies __SSAT at its corresponding assignments (lines 138–139); do not extend the mono finding to that branch without evidence. Selected valid controls, including makeup parameter 0.5 (gain 2), produce **33683.1**, outside the defined int16 conversion domain. Both O2 and O3 sanitizer runs fail. A predictable ARM low-halfword outcome would not make this C defined, and was not observed on the instrument.

Future local conversion/headroom policy needs an explicit sound decision; saturation differs from wrap-like peaks. No blanket limiter/AGC is justified by this audit. Continued cross-FX soak uses Compressor makeup control 0 (gain 1); this does not fix or certify other settings.

### 5. Full FX initialization reaches CLZ(0) C undefined behavior — medium priority, S/N/A

[Oscillator.c lines 38–45](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/DSPAudio/Oscillator.c#L38): fast_log2 computes 31-__CLZ(val). CMSIS cmsis_gcc.h line 1011 implements CLZ using __builtin_clz, whose GCC contract leaves zero undefined. Ringmod_init sets frequency 100 and calls osc_setFreq; FxBus_init always initializes it even when CLP is selected. All eight original strict smoke jobs failed here. Earlier R11 fixtures stubbed other FX initialization/processing and did not cover this path.

Isolated delivered FxBus_init completes with Ringmod table row 4; actual delivered osc_setFreq probes 0.1–440 Hz also select row 4, 880 Hz row 5 and 15000 Hz row 10. In particular native/ARM boundary rounding at 440 differs under optimization. This is a source defect, **not evidence the instrument failed to initialize**. A future zero-safe policy must preserve the delivered finite row/phase behavior deliberately, rather than replacing this with a musically different ideal log2/table choice.

To continue independent endurance work, the native-only CLZ shim returns 32 for zero (the ARM instruction result) and uses builtin CLZ otherwise. Strict original failures remain failed; subsequent runs are clearly marked target-compatible-host-substitution tests, not untouched-production sanitizer passes. No NaN policy or production patch was added.

### 6. Ready flag is not buffer-generation freshness — reachable timing risk, S/I

[Soundengine.c lines 22–35](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/Soundengine.c#L22) and [cs4344_cs5343.c lines 83–142](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/cs4344_cs5343.c#L83): two normal-mode DMA streams are rearmed in completion callbacks. SWITCH_FLAG=1 makes SPI3 publish the shared next-half request. Both output halves rely on stream phase alignment and a single validity flag, without generation tags.

A source-extracted scheduling probe injects the next completion during a late foreground render: a partial buffer can be armed; a newer request is then overwritten by the unconditional final SAMPLE_VALID store; a following old generation can be consumed despite that flag. The existing error counter does not count every stale generation. Opposite stream-callback ordering also changes not-ready counts in the model. This demonstrates a possible control-flow failure **if execution is late**; no target underrun or actual stream phase was measured.

HAL normal completion disables the TX DMA request and sets READY before rearm. Release ignores rearm status; the weak HAL_I2S_ErrorCallback is effectively empty. Existing optional diagnostics count nonzero rearm return values, not all DMA transfer errors or recovery/freshness. Measure both streams and generations before proposing a scheduling correction. Do not change the scheduler based on this injected trace alone.

### 7. Long-uptime deadlines, error counters and service load — S/N/A/I

- [clockSync.c line 56](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/Sequencer/clockSync.c#L56) adds a float shuffle delay to uint32 systick_ticks, promoting the absolute tick count to float. Delivered ARM probes at 0x00ffffff/0x01000000/0x01000001 give deadlines 0x010000a6/0x010000a8/0x010000a8. Near UINT32_MAX they saturate to 0xffffffff in the delivered conversion. Native source conversion is invalid when rounding reaches 2^32. Approximately 4 kHz ticks lose one-tick float precision after about 70 minutes and wrap after about 12.43 days. These are reachable long-uptime paths, not a device-duration test.
- [screensaver.c lines 258–383](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/menu/screensaver.c#L258) uses absolute unsigned deadline comparisons. Delivered function with only LCD drain stubbed advances NEW_POS too early at now 0xffffff9c/deadline 400; OFF_WAIT at now 5/deadline 0xfffffff0 stays waiting. HAL's 1 ms tick wraps about 49.7 days. Other unsigned elapsed-difference timers were not automatically classified as defective.
- The live volatile signed int error++ in the audio callback overflows at INT_MAX; a boundary-injected UBSan failure is retained, not described as a normal-duration crash. Existing diagnostic metric increments instead saturate and unsigned short-interval wrap tests pass (0xfffffff0 to 0x10 gives 32 cycles). Full counter epochs remain necessary for long outages.
- UART byte FIFO capacity 255 and front RX FIFO capacity 63 pass exact-full/empty plus 100000 wrap operations. Some producers ignore full returns, so bounded memory does not imply lossless control. uart_processMidi drains until empty without a per-service work bound; ongoing higher-priority producers can extend it. At 31250 baud/8N1, 255 bytes represent about 81.6 ms of unserviced wire input, not a measured safe latency.

Future timer changes should keep relative delays in bounded integer ticks with explicit wrap-safe deadline contracts; tests must include both sides of wrap, zero sentinel and external clock/shuffle. Queue/transport ownership requires its own bounded task. Neither is silently patched in a resource audit.

## Memory accounting and ownership

All values below are bytes. “Free” is a static address span, never a CPU or measured-stack claim.

| Region/metric | Official 1.71, supplied only | Delivered R11, reproduced |
|---|---:|---:|
| ITCM used / capacity | 14552 / 16384 | 15560 / 16384 |
| ITCM free | 1832 | 824 |
| DTCM static extent / capacity | 107392 / 131072 | 108184 / 131072 |
| Gap to runtime SP at DTCM top | 23680 | 22888 |
| General SRAM data+BSS | 270500 | 274144 |
| General SRAM heap reservation | not reproduced | 512 additional |
| Printed RAM including reservation | not reproduced | 274656 / 393216 |
| Full application flash load span | 273528 | 291324 |

Official 1.71 bytes were unavailable: those official figures retain supplied-document provenance and are **not reproduced**. Public source labeled 1.7 and its differently compiled rebuild are not the official 1.71 binary.

R11 ordinary flash/run code plus rodata/vector/alignment ends at 0x08042158 (237912 B from app base). SRAM .data load 856 B ends at 0x080424b0; initialized DTCM load 36996 B ends at 0x0804b534; ITCM load 15560 B ends at **0x0804f1fc**. Thus printed linker FLASH usage 238768 +36996 +15560 = **291324**. Available app span is 0x08008000–0x08100000; remaining physical span is **724484**, not 777040. ITCM/DTCM initialized data occupy both load flash and run memory; do not add run bytes again as a second flash payload.

DTCM .dtcm is 36996 B, then 4 B alignment, .dtcmz 71184 B, ending 0x2001a698. SRAM .data 856 + .bss 273288 ends at 0x20062ee0. _sbrk starts there, aligned to eight, and bounds the break at 0x20080000: at most **119072 B** including the nominal 512-byte reservation; printed unreserved SRAM is 118560 B. Heap is not sharing the DTCM runtime stack.

[Linker lines 242–252](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/STM32F765VGTX_FLASH.ld#L242) assert only a minimum 1024-byte static DTCM gap. The empty .dtcmstack section is not a runtime fence, canary, guard page or high-water measurement. [_sbrk](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/sysmem.c#L58) bounds heap growth; it does not prove newlib allocator serialization. malloc and weak/no-op retarget locks remain linked. Prior fixed USB class storage removes that known interrupt allocator caller, not all possible library allocation.

Largest DTCM occupants: FX_BUFFER 65536, transientData 26460, sine_table 8194, three DrumVoice structs 2112, DMA buffers 512 each. Largest SRAM: seq_patternSet 233536, mainParameters 15808, seq_songData 4096, patternBackupBuffer 3650. Largest ITCM symbols: mixer 3400, SVF_calcBlockZDF 1180, SVF_calcBlockCheapZDF 1140, seq_nextStep 1096, drum sync 704. These are candidates for measured lifetime/placement work, not permission to shrink histories, move DMA buffers, alter filters or alias storage.

R11 adds two fixed 256-byte DTCM buffers, postMix and isolated, and 8 SRAM bytes for selection/target. No new allocation or persistent smoother. Production contract is 64 frames; SoloMix postMix[64], isolated[128], drum samples[64], stereo FX input[128] and uint8 block loops rely on that bound. Actual production caller supplies two distinct aligned interleaved DMA windows. SoloMix's isolated output aliases are deliberate for channel selection and tested in its earlier all-destination integration; they are not general permission for arbitrary output aliases or larger blocks. One-shot metadata is refreshed after runtime modulation, before synthesis, and invalidated after the synchronous effect call. Exact zero gates after decimation; held sample history is preserved rather than injected behind mute.

ST AN4667 distinguishes this F7 from assumptions about other STM32 families: DTCM can be reached by DMA through AHBS; ITCM cannot. Existing DTCM DMA placement is not automatically a D-cache-coherency defect. A future move to cacheable SRAM requires separate ownership, alignment, cache maintenance and contention analysis.

## Stack and scheduling audit

Compile-only analysis used the recorded Cortex-M7/hard-float O3/fast-math flags plus -fstack-usage; no Release link or firmware image. Delivered toolchain is Arm GNU 14.3.Rel1, GCC 14.3.1 20250623, build arm-14.174. The available container distribution reports the same version; its image digest and provenance caveat are in the appendix. Retained strong-function sizes match the delivered ELF in **819 comparisons**, with no mismatch after excluding overridden weak definitions; SoloMix executable sections with/without the stack-report flag are identical. This corroborates frame relevance but is not verification that the container archive equals the official toolchain archive.

| Function | Own compiler frame / bounded dynamic addition |
|---|---:|
| mixer_calcNextSampleBlock | 488 |
| FxBus_clipFaderGains | 168; R9 own frame 48, increase 120 |
| Distortion_processWithClipGain | 184 |
| SoloMix_process / SoloMix_drum | 64 / 48 |
| Drum sync | 80 +128 VLA at 64 frames; fixed part R10 64 |
| Snare sync | 48 +256 VLA |
| Cymbal / hi-hat sync | 40 +256 VLA each |
| Compressor / Ringmod process | 104 +256 / 40 +128 VLA |
| sd_formatRoutine | 1104 |
| preset_LoadTmpKit / preset_saveProject | 272 / 256 |

The R10 fader-replay +120 is an own-frame delta, not 120 B persistent RAM or a complete engine bound. Foreground/ISR stacks also include callers, callees, compiler spills, libc and hardware exception frames. Analysis has 121 C stack-report files, 40 unresolved indirect call sites and 51 retained library/assembly functions without compiler frame records. Ten graph cycles include bounded Euclidean recursion, wildcard recursion and UI/parser re-entry; cycle detection alone is not proof of infinite recursion.

Direct/tail-call and veneer graph sums were generated, but are **not upper bounds**: some predicate-incompatible paths combine solo-CLP with other effects, some tail frames are overcounted, and unknown callbacks/library frames are omitted. Reporting its largest number as “the stack bound” would be misleading. Valid important chains include foreground soundengine_process → mixer → drum sync/SoloMix_drum or SoloMix_process → FxBus → CLP; storage/parameter-send paths can invoke soundengine_process; DMA IRQ → HAL_DMA_IRQHandler → registered I2S completion callback → HAL rearm. Indirect USB/SD/HAL paths still require completion or measured bounds.

Configured NVIC grouping is GROUP_3. Priorities include TIM14/SD DMA 0, SDMMC 2, UART/USB/front/ADC 3, TIM2 4, audio DMA streams 5. Lower numeric preemption levels can interrupt rendering; equal audio priorities do not establish aligned output consumption. FPU extended exception context can add 72 B beyond the 32-byte basic frame, plus alignment, at relevant nesting levels; actual lazy-save/control state, bootloader setup and worst nesting remain unmeasured. Compiler .su alone omits those hardware frames.

CALC_SAMPLES_IN_INTERRUPT=1 and USE_INTERRUPT_ONLY_DURING_SAVE_PROJ=1 are active. Normally foreground masks TIM2's update interrupt only, performs sequencing/render, then reenables it; other IRQs remain enabled. TIM2 rendering is used during storage paths. The timer's nominal polling interval is not the audio block period, and starting/stopping it in storage helpers must be included in target workloads. No source-only proof of all pending-interrupt/re-entry windows is claimed. The audio-owned SoloMix/replay arrangement relies on this existing serialization; metadata checks do not prove scheduler timing.

## Native endurance and boundary results

New work was justified by missing sustained-state coverage and actual full-FX initialization/dispatch, not by a desire to repeat the entire prior sweep/build. The pinned production six voice trigger/async/sync implementations, verbatim mixer/decimator/pan and all four real effects are used. Hardware ADC/RNG/time/control services are deterministic shims. This is not a booted firmware, actual SD recall/morph engine, IRQ scheduler or USB controller.

Strict initial eight smoke jobs fail CLZ(0). Qualified continuation uses the disclosed ARM-compatible native CLZ-zero result. Native half storage is two-byte IEEE _Float16, replacing the earlier host-only four-byte float shim. A snare wrapper counts/validates finite values and calls the unchanged production eligible clamp; it does not replace synthesis. All six filters are explicitly OFF in fixtures. Later actual FX sweep failures exposed Delay mono-right, then Compressor conversion. They are retained in separate result files; final scenario 2 restricts Delay to stereo and Compressor to unity makeup.

Four scenarios cover solo Voice 6 plus unrelated direct voices; all six shared routes; all 64 route masks/output destinations/FX transitions; and sparse/returning/zero sequences including frozen decimation. Selected MIX targets 0/1/5/10/20/127, squared fader targets 0/0.0625/0.25/0.5625/1, dry/wet 0/0.5/1, dense retriggers/tails and quiet windows run repeatedly. Real FxBus_prepareFxChange reset endpoints are called; synthetic parameter/kit-lifecycle steps are **not full storage recall coverage**. Open/closed hi-hat alternation uses the real shared object.

Each process runs 82763 blocks ×64 = **5296832 frames**, 120.000724966 simulated seconds at production DSP REAL_FS=44140.0. Hardware nominal I2S sample rate 44140.625 gives a slightly different duration; that clock was not measured. Four scenarios run under O2 sanitizers, O3 fast-math sanitizers and an O2 deterministic repeat: **63561984 frames / 1440.008699592 simulated seconds in twelve processes**. This is about 24 minutes aggregate, not a single 24-minute soak or hours of instrument uptime. The two-minute budget targets millions of updates and many control/tail cycles without repeating the earlier 39936-case audit; long-uptime counters are injected directly instead.

ASan/UBSan/bounds/float-cast-overflow are nonrecovering. Finite CLP histories are also checked by a separate no-fast-math bit-check TU, avoiding optimized-away isfinite checks. Output sentinels remain intact; tested quiet CLP windows reach quantized zero; largest observed absolute lastOut is 0.647478997707. O2 repeat hashes match per scenario; O2/O3 hashes differ, as expected for different FP optimization, and no cross-toolchain waveform identity is claimed. Peak 32768 and adjacent sample step as large as 65535 occur in mixed/transition workloads: these numbers do **not** establish click-free output or unclipped mixing. No heap-growth conclusion follows from a fixed-buffer fixture without the real storage/USB lifecycle.

Extracted queue and scheduling tests, and isolated ARM timer/control probes, add exact boundary evidence described above. Emulator is Unicorn **2.1.4**, Cortex-A15/VFP compatibility mode; it is not a Cortex-M7 timing, interrupt, bus/cache or peripheral model. The unsupported Delay instruction is a tool limitation, not a target fault. Full command/failed-attempt/result summaries are in the [evidence appendix](2026-11-01_r11-resource-audit-evidence.md).

## Release history, attribution and two-pass ledger

Existing R5–R11 ELF/image bytes reproduce the following extents; no historical rebuild was performed. R4's same-source GCC14.3 rebuild is corroborated by R5's retained R4_SECTIONS.txt and immutable reports, but its image/ELF was unavailable locally. R3/R4 15016 ITCM is historical report evidence; the original delivered R4 used GCC14.2 and a different payload. Do not compare it as a matched compiler source-cost pair.

| Revision | ITCM | DTCM static | SRAM data+BSS | Full flash payload |
|---|---:|---:|---:|---:|
| Official 1.71, supplied only | 14552 | 107392 | 270500 | 273528 |
| R4 matched-toolchain rebuild, retained sections/reports | 15016 | 107392 | 274136 | 286112 |
| R5 | 15104 | 107400 | 274136 | 286864 |
| R6, discarded alternative | 15104 | 107416 | 274136 | 287088 |
| R7 | 15104 | 107416 | 274136 | 287112 |
| R8 | 15608 | 107672 | 274136 | 289920 |
| R9 | 15608 | 107672 | 274136 | 289992 |
| Corrected R10 | 15608 | 107672 | 274136 | 290072 |
| R11 | 15560 | 108184 | 274144 | 291324 |

R5 versus rebuilt R4 full flash is **+752**, not linker-only +664: +88 ITCM load bytes matter. R6 and R7 both branch from R5; merge-base verification confirms af3562fece090288a83727524c1035b79f4cf20b. The selected R4→R5→R7→R8→R9→corrected R10→R11 sum is **+544 ITCM, +792 DTCM, +8 SRAM, +5212 full flash**. Do not add abandoned R6.

R7→R8 section growth +504 combines named-symbol +506 and alignment -2. R10→R11 net -48 combines oscillator symbols -196 with mixer/drum/helper veneers +148. The latter are mixer +52, drum sync +56, five veneers +40. Oscillator individual changes include FM +30, sine-FM +40, next oscillator +4, next FM -348, PWM +84 and table -6. These endpoint measurements do not isolate each arithmetic correction's causal cost, and smaller code does not establish less CPU work.

LXR-001/BUG001 (NRPN CC0/filter-drive dispatch) is a root-cause control fix; LXR-002 Morph 0–127 and LXR-003 successful-load feedback are explicit UX/design changes. LXR-004 storage work combines bounds/ownership hardening, timeout/recovery policy and staged write architecture, not a resource-neutral patch. A matched R3→R4 load-confirmation comparison demonstrates **zero ITCM delta**. Supplied technical report tables for integrated A→R4 sum zero ITCM/DTCM, +3608 SRAM and +13096 flash, including S4b-2 alone +3040 SRAM/+8328 flash. The +464 official→R4 ITCM difference cannot be allocated to individual early fixes without matched compiler/configuration evidence. Absolute standalone BUG001/Euclidean reports and an integration comparison are not clean per-fix removal costs. Earlier reported phase deltas and exact research-report identities are in the appendix.

R5 idle suppression deliberately changes feedback policy; it is not proof original intentional self-oscillation was accidental. R7 parallel bass is a signal-design change. R11 arithmetic prerequisites are defined-behavior hardening with declared seam sound exceptions; FILTER-OFF is a workaround boundary, not a filter fix. Capacity recovery is optimization. These categories do not predict byte costs.

### Module boundaries and unused dispatch

Target sizeof probes and map retained-section sums reproduce:

| Module | Own ordinary-flash code | Own ITCM code | Per-instance state |
|---|---:|---:|---:|
| DRV/Distortion (all its algorithms) | 6252 | 32 | 176 |
| Ringmod | 380 | 0 | 44 |
| Compressor | 1136 | 0 | 32 |
| Delay | 3396 | 0 | 96, excluding global history |
| Shared FxBus | 480 | 1156 | parent 376 includes all four states |
| SoloMix helpers | 860 | 0 | new fixed buffers 512 DTCM +8 SRAM |

These own-code sums exclude shared oscillator/math/filter dependencies, tables, veneers, debug/discarded sections and alignment; they are not marginal removal costs. DRV shared tables occupy 2060 DTCM; global FX_BUFFER 65536 is separate. Four effect structs sum 348; FxBus controls/alignment add 28. Ringmod is **44**, not its 48-byte parent gap (four padding bytes). Delay's OnePoleFilter includes double alignment; OnePoleFilter sizeof16, OscInfo40. Shared ITCM stubs total 544 and are not charged entirely to any one effect.

Delivered FxBus_process is 384 ITCM bytes, FxBus_processWithClipGain 388; compiler inlining duplicates dispatch. The former has no production source caller, no decoded direct/tail call reference, and no relocation in any of the 121 C analysis objects, including data/function-pointer relocations. Firmware has no dynamic symbol-loader API. It remains linked with the used functions in their grouped .itcm_text input section. This is strong evidence for an **excluded/dead-code capacity candidate**, not a claim that source search alone proved absence.

Smallest separate capacity experiment: remove only the unused wrapper's ITCM annotation so ordinary function sections can be garbage-collected, then compare matched link maps, references, bytes, regressions and timing. Do not move existing hot routines or count 384 B as guaranteed recovered: alignment, veneers and compiler decisions can alter the net result. It was not implemented or linked here.

### Report completeness and development policy

R5–R10 each have build reports. The two blocked R11 prerequisite reports predate final successful CI and are historical evidence, not the final handoff. At the time the supplied ledger was written that final handoff was missing. It has now been recovered from published source/artifacts and separately published under the preceding authorized task: [R11 final handoff](https://github.com/DocLXR-02/lxr-desktop-app/blob/84bc8a8c81fbc3d348c509ee5a2c7d5329d4cf8c/docs/codex-berichte/2026-10-31_r11-arithmetic-and-filter-off-post-mix.md). No private interrupted-session command or independent review is invented. Its date follows the requested build-report path; publication occurred during this continuation.

Every delivery needs a matched base/head/toolchain report, actual commands/results/failures, artifacts/hashes, full load span, run-memory/state/scratch/stack and CPU/deadline evidence or explicit UNMEASURED. Retain reference artifacts. Apply the owner's two passes: first validate bounded function/musical behavior with initial resource checks; then focused efficiency/robustness across flash, ITCM, data, histories, scratch, stack, deadlines, I/O and long-run behavior before freezing or scaling. A known invalid memory/deadline path cannot be deferred merely as “optimization.” Conversely speculative optimization must not derail a bounded listening experiment.

Exact reference sound is not an absolute optimization requirement. Any audible deviation must be described, compared, paired with a measured efficiency gain under matched conditions and explicitly accepted by Daniel before promotion. This policy authorizes no DSP edit in this audit.

## Expanded FX feasibility and product choices

The full goal remains different effects per musical voice, multiple effects on a voice and a larger algorithm library. A bounded slot pool is a capability option, not an equivalent replacement for independent chains.

[MidiVoiceControl.c line 41](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/MIDI/MidiVoiceControl.c#L41) maps musical voices 5/6 (zero based) to HiHat_trigger(vel,voice-5,note). Mixer has three drums, snare, cymbal and **one shared hi-hat object/route/fader**: six synthesis paths but seven track controls. Independent closed/open-hat FX requires a deliberate source/tail/choke separation; simply adding a seventh bus does not create two independent sources.

| Architecture | Capability | Active processor count at full use | Main compromise |
|---|---|---:|---|
| One routable slot | Current shared effect | 1 | Shared nonlinear interactions; no independent wet stems |
| Two-slot pool | Two groups or short serial/parallel use | up to 2 | Cannot give every track independent settings/chains |
| Four-slot pool | e.g. kick CLP→Compressor, snare Delay, hats Ringmod | up to 4 | Pool contention/allocation and tails; still not seven independent effects |
| One independent stage per track | Different state/settings per seven tracks | up to 7 | Requires hat/source separation; changes shared intermodulation |
| Two-stage per-track chains | Two ordered effects on each track | up to 14 | History/CPU/route/UI and output headroom grow |
| Four-stage per-track chains | Four ordered effects on each track | up to 28 | Much larger runtime requirement; no present capacity proof |

Algorithm library code is generally shared once, not multiplied by count; active call count and per-instance state/history do scale. An unselected menu algorithm may consume flash but no render calls; keeping tails live can consume calls even with no new notes. Compile specialization can clone code and must be measured rather than assumed.

For CLP alone, 176-byte contexts for 1/2/4/7/14/28 instances require 176/352/704/1232/2464/4928 B. Current FxBus376 stores all four algorithms' structs concurrently, not just the selected one. A future tagged union could reduce inactive state but loses simultaneous tail/settings history unless handled separately. It does not eliminate delay audio storage. These are target layout calculations, not an implemented instance API or CPU budget.

[FxRamBuffer.c line 18](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c#L18) defines one global 32768-sample half-float history, **65536 B**. Delay accesses that global; duplicating Delay structs would still alias audio history. Full-capacity histories for 1/2/4/7/14/28 slots require 65536/131072/262144/458752/917504/1835008 B. Seven need six additional lines, **393216 extra bytes**, beyond the current combined static free DTCM+unreserved SRAM 141448 B even before stack/heap reserves. Four total lines need 196608 additional bytes, also too much unchanged. Two total lines need 65536 additional bytes: an arithmetic possibility in general SRAM, not an approved placement, cache, bandwidth, CPU or stack fit. No unpopulated external RAM is assumed.

Serial processing can reuse proven-lifetime scratch (a 64-frame stereo int16 block is 256 B; two ping-pong blocks 512 B). Parallel branches need separate live intermediates/accumulation or serialized accumulation with explicit saturation/headroom; histories of active tails cannot be aliased. Shared code and shared scratch do not mean shared nonlinear/filter/delay histories. A larger library and many simultaneous long delays are different memory problems.

Preserving R11 as a reference requires explicit control ownership: physical fader drives excitation, CLP H(qmax) affects audible core outside feedback, selected MIX post-scales the isolated solo return once. A per-voice chain could post-scale once after its whole chain, but multiple serial CLPs or per-voice cores change the sound and intermodulation. One aggregate bus multiplier cannot recover independent processed stems or MIX balances from a nonlinear sum. Multiple routed voices in R11 still use R10 Maximum policy, including the silent-high-fader compromise.

Per-stage dry/wet, per-voice dry sends, global return and output destinations must avoid duplicate dry paths. Pan before/after a nonlinear stereo stage changes interaction and resource needs. Serial graphs add no inherent extra 64-frame queue if processed within one block, but algorithmic latency and actual completion deadlines still matter; parallel latency/phase matching can change bass cancellation and peaks. Tail ownership on bypass/reassignment/recall must be specified; no silent resets, crossfades or normalization are presumed. Initial graph design should be acyclic, with explicit tail/bypass/output rules; creative feedback routing is a different feature.

Priority: close measurement/robustness gates on the successful reference. The smallest subsequent capacity prototype is two independent CLP contexts on two existing synthesis paths, with separate histories/settings and preserved one-slot R11 reference behavior; test two simultaneous sources and tails on target before adding chains. This is a bounded experiment, not a substitute for the full seven-track goal. Choosing a two-slot product pool versus true per-track source separation, especially hats, still needs an explicit decision. If the owner chooses a pool, document its limitations before implementing it. Four-slot or seven-track/four-stage claims remain unapproved. **824 free ITCM bytes, large flash headroom, native timing and memory fit cannot determine how many cores meet M7 deadlines.**

## Flash endurance and remaining device work

ST [DS11532 Rev 9 Table 59](https://www.st.com/resource/en/datasheet/stm32f765zi.pdf), page 152, specifies minimum **10000 erase/program cycles** within its stated temperature conditions (suffix 6 through +85°C; suffix 7 through +105°C, lower bound -40°C). This is a per-sector endurance rating, not a universal remaining-life estimate or permission for repeated flashing.

Assuming each actual installation erases each affected app sector exactly once, 1000 actual flashes use 1000 cycles, 10% of that minimum rating for those sectors. 1000 releases installed twice use 2000 cycles, 20%. Extra erases/retries, different sector sets, temperature/retention and bootloader behavior change this accounting. A GitHub build/download or SD copy alone is not an MCU flash cycle. Exact installed bootloader version/erase sequence/option bytes were not established; repository bootloader code is not proof of the unit's installed implementation. No physical option bytes were read or modified and no wear counter is proposed.

Offline conclusions are complete within the listed coverage; physical timing/phase, interrupt latency, actual high-water stack, cache/bus interference, calibrated thermal behavior and prolonged instrument reliability require separately approved target work. The attached measurement specification gives owner-readable pages, staged workloads, stop criteria and safe stack/clock methods; it is a design, not an existing or delivered diagnostics feature. Independent per-voice processing, hat separation, chain/pool selection and audible optimization tradeoffs remain product decisions. Nothing is frozen.

## Publication contents and continuation

Read the [evidence appendix](2026-11-01_r11-resource-audit-evidence.md) for exact checks/commands/results/limitations and the [measurement plan](2026-11-01_r11-resource-measurement-plan.md) for the bounded next instrumentation task and iPhone protocol. The [package manifest](2026-11-01_r11-resource-audit-package.md) identifies the Markdown-only owner ZIP. Analysis scripts/objects remain outside the source checkout; they are not firmware or repository changes. Remote report/ZIP bytes and preservation of existing heads are verified as part of handoff.
