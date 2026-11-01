# R11 resource audit: evidence and reproduction record

Date: 2026-11-01 UTC. Audited source: 64263ed25f2e9c7f5d880b493d943bfba29271a8. This is a companion to the [audit](2026-11-01_r11-resource-reliability-audit.md), not a new firmware package. Production files, tests and workflows stayed unchanged. Commands below describe work actually performed unless explicitly labeled proposed. Temporary analysis source/objects were kept outside the checkout; publication is restricted to Markdown evidence summaries and the owner report ZIP.

## Tools and source export

Native compiler: Debian GCC 14.2.0-19, cc 14.2.0. Python 3.12.14. Analysis dependencies: pyelftools 0.32, Capstone 5.0.6, Unicorn 2.1.4. Dependencies were available under /workspace/clp-arm-triage/deps; importing Unicorn with plain system Python fails, while the documented PYTHONPATH succeeds. That setup error is not a firmware failure.

Working paths:

- /workspace/r11-source: pinned clean Git worktree.
- /workspace/r11-reserve-audit/source: immutable git-archive export of the source pin.
- /workspace/r11-downloads/evidence and image: verified existing CI downloads.
- /workspace/r11-reserve-audit/{stack,soak,boundaries,results,reports,inputs}: analysis-only files.
- /workspace/r11-audit-report: new documentation worktree based exactly on 5829398cea5d337200393757e39a37d4d8148256.

Source/ref checks used git rev-parse, git status --short, git ls-remote --heads origin, git show at exact report commits and git merge-base. Source archive verification hashes each archived Git blob and compares with the pinned tree, rather than trusting the branch name. All 432 match. Existing source heads were recorded before/after; no existing head moved. A readonly fetch recovered the missing local R4 hardware-validation report object; an initial local git-show error was missing local history, not proof of a bad report URL.

Delivered compiler metadata: Arm GNU 14.3.Rel1, GCC 14.3.1 20250623, arm-14.174. Official archive SHA recorded in CI: 8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd. R10 and R11 have matching common production compile commands (121), with SoloMix added in R11. Important flags: -O3 -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -ffast-math -freciprocal-math -mfp16-format=ieee -fomit-frame-pointer, function/data sections; retain the per-file recorded filter option -fno-thread-jumps.

No native Arm compiler was installed. Compile-only work used public container jafee201153/arm-none-eabi-gcc:14.3.Rel1-ubuntu-24.04, immutable digest sha256:beb41ddad38f61795d2f9c9cd8385e11b94f962cb83b655d38aaf6b54b2cc82a. It reports the same compiler version. **This third-party distribution was not independently matched byte-for-byte to the official archive**. Version, generated retained function sizes and selected identical executable sections corroborate compatibility; do not describe it as a fully authenticated replacement archive.

Container execution used the managed Docker daemon, explicit local unix socket, --network none --read-only --cap-drop ALL --security-opt no-new-privileges --user 1000:1000, read-only source mount, writable audit mount and temporary /tmp. No credentials/host-home mount or registry configuration change. Script paths were rewritten from the CI prefix to /src and output objects to /audit/stack. Exact recorded commands were replayed with only -fstack-usage added; **no firmware link, binary/image creation or Release rebuild** occurred.

Compile-only command sequence (paths as executed):

    python /workspace/r11-reserve-audit/prepare_stack.py
    env -u DOCKER_HOST -u DOCKER_CONTEXT -u DOCKER_TLS -u DOCKER_TLS_VERIFY -u DOCKER_CERT_PATH docker --host=unix:///var/run/docker.sock run --rm --network none --read-only --cap-drop ALL --security-opt no-new-privileges --user 1000:1000 --tmpfs /tmp --mount type=bind,src=/workspace/r11-reserve-audit/source,dst=/src,readonly --mount type=bind,src=/workspace/r11-reserve-audit,dst=/audit jafee201153/arm-none-eabi-gcc@sha256:beb41ddad38f61795d2f9c9cd8385e11b94f962cb83b655d38aaf6b54b2cc82a sh /audit/stack-compile.sh

The analogous second container invocation runs /audit/extra-compile.sh, compiling only existing diagnostic-ON objects and a source-pinned R9 FxBus comparison. This is not a second firmware build.

The generated stack-commands.json records 122 production compilation commands, including startup assembly; 121 C .su files result. 819 retained strong function size comparisons match actual ELF; overridden weak HAL/startup definitions were excluded from false mismatch reporting. All SoloMix executable sections with/without -fstack-usage match. A target object containing sizeof/offsetof constants confirms OnePoleFilter16, OscInfo40, Distortion176, Ringmod44, Compressor32, Delay96, FxBus376, member offsets24/200/248/280. Native sizeof was not substituted for target layout.

## Existing artifact checks

Existing R11 CI run: 41214348060, conclusion success, attempt 1. Outer downloadable artifact wrappers:

| Wrapper | Bytes | SHA-256 |
|---|---:|---|
| lxr-r11-image.zip, artifact 13047418960 | 422404 | 566e335f41546c43313f96b4648e86e7f2dacc82eb67d99e2fd2d47d49d7cbcc |
| lxr-r11-evidence.zip, artifact 13047144663 | 8719802 | 341324a0b404bfd1775c7eccc77bac4dc9686a94ac335b63b31eb67b64899d83 |

Wrapper SHA values match live GitHub artifact metadata; internal manifest checks cover 29 evidence files. Direct signed-download URL access via urllib was rejected by the network proxy; the supported GitHub artifact/download path succeeded. Do not infer artifact absence from the first HTTP failure. Firmware/package identities are in the main report and final R11 build handoff.

Image verification reads header fields, checks checksum as 255-(sum(payload)&255), zero reserved bytes, actual length, all vectors, label and every file-backed PT_LOAD segment at its physical address. Runtime SP is decoded from the first Reset_Handler literal-load instruction, not inferred from the image's initial SP. Verified result file: /workspace/r11-downloads/verified.json. Existing build/source evidence was reused; no claim that an independently rebuilt R11 image was compared byte-for-byte.

## Endurance fixture and exact workload

The existing tests/fx_clip_r11/common.py extraction uses actual named Oscillator functions and midiParser_calcDetune, production voice .c files, actual trigger/async/sync paths and verbatim selected mixer functions. Hardware/control services are explicit shims. New endurance fixture links real FxBus, Distortion, Ringmod, Compressor, Delay and FxRamBuffer instead of the earlier fixture's stub other-FX entry points. That change is why FxBus_init's CLZ(0) was newly exercised.

Selected fixture setup is the production R11 voice_fixture setup(v,2,0.2,1), production 64-frame cadence, deterministic seed 1, all six filters OFF. Envelope, voice distortion and synthesis execute; note generation is synthetic. Every block sets routes/targets before the verbatim mixer, which performs live runtime selection, real async/sync synthesis, decimation, pan, bus conversion and SoloMix post processing. LFO/morph control service stubs are not equivalent to executing actual modulation/storage schedulers. Two velocity modes and broader generation configurations remain in existing CI/all-six audit evidence; this soak does not claim to re-cover every such combination.

Core workload rules, recorded rather than an idealized source-times-volume model:

- Four scenarios: (0) mask32, solo hat/Voice6 plus other direct voices; (1) mask63, all shared; (2) mask=(block/128)%64 and fx=(block/2048)%5, real FX dispatch/transitions; (3) masks32,1,2,4,8,16,0,3,63 in 512-block windows, sparse/return/zero/frozen-decimator behavior.
- Per-voice MIX cycles through 0,1,5,10,20,127 divided by127, staggered by voice every512 blocks. Physical u cycles0,.25,.5,.75,1 every256 blocks; ADC target is u². FX destination cycles0..5 every1024 blocks, except qualified Delay uses stereo destination0.
- D/W cycles0,.5,1 every512 blocks. CLP drive/ton/col/vol are set to maximum in its selected windows. Other FX parameters begin .5; qualified Compressor makeup control is0 (gain1).
- Retrigger voice v every31+5v blocks when not quiet, velocity=(block%4==0)?1:127, notes36+(block%5)*12. Hi-hat open/closed alternates with block/31, using the same production object.
- Quiet starts at block%4096 >=3500: MIX and physical target0, no triggers. From quiet sub-block3700, CLP output must be exactly quantized zero. Scenario3 freezes decimation only for the eligible zero-MIX source, checking the real post-decimator gate.
- FxBus_prepareFxChange runs on changes and every4096 blocks; this is the real audio-owned reset endpoint, not a full SD kit load. Existing source/CI covers lifecycle selection; real long-run SD/morph/IRQ behavior remains unmeasured.
- Each output array has130 int16 elements; first128 are valid interleaved output, last two are guard values23456/-23456. CLP double state pairs and feedback gain are checked for finite exponent bits by a separate non-fast-math TU after actual processing. FNV64 fingerprints cover outputs and full Distortion struct; repeated hash equality is a reproducibility check, not a collision-free proof of all internal state equivalence.

Native flags used by both variants:

    cc -std=c11 -D_DEFAULT_SOURCE -DNUM_DRUM_VOICES=3 -DOUTPUT_DMA_SIZE=64 -DUSE_BIG_SAMPLE_BUFFER=1 -DINTERPOLATE_OSC=1 -DINTERPOLATE_FM_OSC=1 -DSMOOTH_MIXER_VOL_PAN_DRIVE=1 -DENABLE_DRUM_SVF=1 -DENABLE_MIX_OSC=1 -DAMP_EG_SYNC=0 -DUSE_FILTER_DRIVE=0 -Dtrue=1 -Dfalse=0 -fno-strict-aliasing -ffunction-sections -fdata-sections -fsanitize=address,undefined,bounds,float-cast-overflow -fno-sanitize-recover=all -Werror=implicit-function-declaration

O2 variant adds -O2; optimized variant adds -O3 -ffast-math -freciprocal-math. Include directories are the generated soak include/root, pinned tests/fx_clip_r11, tests/fx_clip/include and Core/Inc. Linked production sources: DrumVoice, Snare, CymbalVoice, HiHat, transientGenerator, transientTables, wavetable, SlopeEg2, Decay, snapEg, 1PoleLp, voice distortion, ResonantFilter, FxBus and all four actual FX plus FxRamBuffer. SoloMix.c is included directly for access to test-only state observations; mixer functions are copied verbatim through the existing extractor. Link flags --wrap=SoloMix_snareSample --gc-sections -lm. The wrapper asserts finiteness/counts over-range samples then calls the real production helper; it does not alter samples. The finite-check TU is always built without fast-math.

Preparation/run commands:

    python /workspace/r11-reserve-audit/prepare_soak.py
    python /workspace/r11-reserve-audit/run_soak.py
    ASAN_OPTIONS=detect_leaks=0 /workspace/r11-reserve-audit/soak/sanitized 2 82763
    ASAN_OPTIONS=detect_leaks=0 /workspace/r11-reserve-audit/soak/optimized 2 82763

run_soak.py launches four scenarios in O2/O3/O2-repeat groups and retains return codes/stderr/JSON. Specific scenario2 runs were repeated only after documenting the previous failure and narrowing its cross-FX control boundary. LeakSanitizer is disabled in this environment; AddressSanitizer, undefined, bounds and float-cast checks remain active/nonrecovering. No broad sanitizer suppression or exclusion of failing voices. This fixture has fixed storage; it is not an allocator-leak or real-device memory-growth test.

### Failures and continuation boundaries

| Attempt | Actual result | Disposition |
|---|---|---|
| Initial strict smoke, 4 scenarios ×2 flags | 8 failures: passing zero to clz | Original source failure retained; not certified as safe C |
| ARM-compatible CLZ-zero host shim smoke | 8 passes, 4096 blocks each | Analysis-only substitution returns32 for0; builtin for nonzero |
| First 120-second jobs | 9 passes; scenario2 O2/O3/repeat fail Delay.c264 unaligned int32 load | Retain all failures; restrict Delay only to stereo for continuation |
| Stereo-Delay scenario2 jobs | Fail Compressor.c180, 33683.1 outside short range | Retain failures; restrict makeup control0 for continuation |
| Final qualified 12-job matrix | 12 pass | All restrictions/substitution explicit; not an untouched-production universal pass |
| Delay full ELF emulation | UC_ERR_INSN_INVALID at0x0802485c, vrintz.f32 | Tool limitation; no complete target Delay execution claim |
| Signed error counter injection | UBSan INT_MAX+1 failure | Expected failure in unchanged source, not a passing counter policy |
| Float shuffle deadline near2^32 | UBSan unsigned conversion failure | Expected failure; separate delivered ARM saturated result |

Every final process: 82763 blocks,5296832 frames,120.000724966 seconds at44140.0. Total final frames63561984; total simulated1440.008699592 seconds. Do not count earlier failed attempts as passing duration or combine simultaneous processes into continuous target uptime. O2 deterministic repeats match exactly per scenario; cross-O2/O3 equality is not asserted. Final summaries follow.

## Small component and delivered-ELF probes

Commands actually used, with local dependency and ELF selection:

    python /workspace/r11-reserve-audit/boundaries.py
    python /workspace/r11-reserve-audit/scheduling.py
    PYTHONPATH=/workspace/clp-arm-triage/deps LXR_TRIAGE_ELF=/workspace/r11-downloads/evidence/LXR_V2_STM32F765.elf python /workspace/r11-reserve-audit/arm_boundaries.py
    PYTHONPATH=/workspace/clp-arm-triage/deps LXR_TRIAGE_ELF=/workspace/r11-downloads/evidence/LXR_V2_STM32F765.elf python /workspace/r11-reserve-audit/usb_arm.py
    PYTHONPATH=/workspace/clp-arm-triage/deps python /workspace/r11-reserve-audit/static_stack.py
    PYTHONPATH=/workspace/clp-arm-triage/deps python /workspace/r11-reserve-audit/callgraph.py
    PYTHONPATH=/workspace/clp-arm-triage/deps python /workspace/r11-reserve-audit/history.py

The actual .py scripts are session-local analysis artifacts, not published code or part of firmware. This appendix preserves their decisive inputs/results, flags, source locations and hashes so the experiments can be independently recreated; it is not a claim that one can execute these workspace paths after downloading only Markdown. Existing publicly committed R11 fixtures/common extraction remain the reusable starting point. Publication authorization here excludes adding analysis sources to the repository.

Queue fixture: extract fifo_init/In/Out, fifoBig_init/In/Out, USBD_CDC_DataOut, usb_getMidi, usb_sendByte and usb_flushMidi verbatim; preserve actual MidiMsg/USB event constants. Mock only USB class receive/control handles and CDC transmit status. Native compile is cc -std=c11 -O2 -fsanitize=address,undefined,bounds,float-cast-overflow -fno-sanitize-recover=all, with assert/guards. Results: byte FIFO255 usable; front RX63;100000 enqueue/dequeue wrap checks; four64-byte RX packets produce64 overwritten/unobservable events; BUSY on a four-byte TX consumes four unsent bytes. These component tests are not a real PC USB/MIDI stack test.

Scheduling fixture: extract actual HAL_I2S_TxCpltCallback and soundengine_calcNextSampleBlock; USE_DAC2=1,OUTPUT_DMA_SIZE=64,SWITCH_FLAG=1, CALC_SAMPLES_IN_INTERRUPT=1. HAL rearm records slots, mixer shim injects one SPI3 completion while rendering the requested half. Result partial arm1, overwritten newer request1, stale half1 consumed1; existing error at that point1, demonstrating it is not a complete generation-error count. An opposite callback-order injection increments not-ready despite modeled no extra render. No probability or actual target order/latency is inferred. Existing diagnostic metric header gets direct wrap/saturating-count unit tests. A separate argument sets error=INT_MAX; the original callback increments and UBSan fails as expected.

For ARM probes, Arm() loads delivered file-backed PT_LOAD bytes at run addresses and uses a private SRAM/SP, enables VFP and initializes FPSCR. It never runs Reset_Handler or boots the firmware; peripherals/cache/bus arbitration/IRQ timing are not modeled. A SHA assertion rejects any ELF other than 350696b2365c973d07b2670a57c72950a65f9a818090e3599c8487f59d313043. Model is Unicorn2.1.4 Cortex-A15 VFP because available M7 emulation does not reliably cover the production FP instruction set; the A15 also rejects Delay's vrintz instruction. Do not label either model result device execution.

USB control-flow probe initializes actual usb_MidiInWrPtr/RdPtr symbols, calls delivered usb_tick and hooks only CDC_Transmit_FS to record (offset,length), return chosen status and resume. Other empty USB paths retain their zero state. Tests:

| Read | Write | Status | Actual submitted range | Read afterward |
|---:|---:|---:|---|---:|
| 0 | 4 | BUSY1 | offset0,length4 | 4 |
| 124 | 0 | OK0 | offset124,length132, exceeds128 | 0 |
| 120 | 4 | OK0 | offset120,length140, exceeds128 | 0 |
| 0 | 4 | OK0 | offset0,length4 | 4 |

CLZ probes call actual osc_setFreq/FxBus_init with initialized private state; Ringmod_init's100Hz call is real. Observed table rows: .1/10/25/55/110/220/439/440Hz→4,880Hz→5,15000Hz→10. Native CLZ(0) remains invalid despite ARM's CLZ instruction yielding32. The 440Hz difference illustrates why an “ideal” host formula cannot silently replace delivered fast-math behavior.

Shuffle probe sets actual reachable shuffle64/127,tempo120,odd master step and clock phase0, runs sync_tick until its actual deadline store, and records the result. Inputs 0x00ffffff,0x01000000,0x01000001 yield 0x010000a6,0x010000a8,0x010000a8; inputs0xfffffeff,0xffffff7f,0xfffffff0 all yield0xffffffff. Stopping after the store avoids pretending to execute unrelated services. Native standalone expression at the wrap edge fails float-cast-overflow. Screensaver executes the actual function with only LCD FIFO drain stubbed: NEW_POS0,now0xffffff9c,deadline400→state1; OFF_WAIT2,now5,deadline0xfffffff0→state2; ordinary now16,deadline32→state2. These are deadline-policy probes, not weeks of instrument operation.

## Compiled-path and source reference discipline

Active UART implementation is Core/Src/Soundengine/MIDI/Uart.c, not similarly named legacy IO/uart.c. Its line63 detects a full RX FIFO but has an empty failure body; line91 drains until empty; line111 ignores TX enqueue failure. usb_flushMidi has no standalone retained function because it is inlined into delivered usb_tick (0x0802c730); absence of a symbol did not justify declaring it dead.

Conversely preset_queryStepDataFromSeq, preset_queryPatternInfoFromSeq and preset_queryMainStepDataFromSeq have no retained symbols or active production callers. Their legacy timeout-width code was not reported as an active R11 defect. DMA1_Stream7 legacy code is under #if0. Commented mixer pan alternatives are likewise not the active production configuration. Call graph work decodes executable mapping regions, skips literal data and resolves eight-byte LDR-PC veneers; it is not a blind BL-byte text search. All C object relocations were also checked, including data/function-pointer references, for the unused FxBus_process candidate.

Inherited-body comparisons with manufacturer a43cd4fcda23ec3d577dc425261c3eb28bd5abff return true for fast_log2, Ringmod_init, Delay_process, Compressor_process, sync_tick, screensaver_process, USBD_CDC_DataOut and usb_flushMidi. This establishes origin of those functions, not that every called dependency/configuration is identical across releases.

Useful pinned production references beyond the main report:

- Core/Src/Soundengine/FxBus.c:22 initialization;46 map;54 replay;88 legacy dispatch;93 active dispatch.
- Core/Src/Soundengine/SoloMix.c:13 eligibility;37 actual MIX/fader update;50 finite snare clamp;58 post-decimator gate;65 isolated return.
- Core/Src/Soundengine/DSPAudio/mixer.c:551 fader buffer/capture;566 runtime target selection;574 render;615 stack blocks;627 capture;631–747 six sync/decimator/pan routes and post return.
- Core/Src/Soundengine/Soundengine.c:22 validity publication;81 foreground TIM2 masking.
- Core/Src/Soundengine/cs4344_cs5343.c:83 signed error;87 completion/rearm.
- Core/Src/stm32f7xx_it.c:262 TIM2 body; Core/Src/main.c:1533 DMA priorities; stm32f7xx_hal_msp.c:82 group and682 timer priority.
- Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c: normal DMA completion/error callbacks; inspect actual registered function pointers, not weak defaults alone.
- Core/Src/Soundengine/Sequencer/sequencer.c:1657 delayed-step absolute deadline;1697 elapsed-difference path (distinct policy).
- STM32F765VGTX_FLASH.ld:33 minimum stack;47 actual stack top;242–252 static assertion. Core/Startup/startup_stm32f765vgtx.s:68 runtime SP relocation.

All references are to the audited pin, not a moving default branch. Some tools initially used wrong filename capitalization/paths; corrected searches located actual sources. Those failed reads contributed no code conclusions.

## Historical ledger details

Matched reported intermediate deltas below are cross-checked against retained technical report tables, not new rebuilds or independently recovered per-fix ELFs. Zero ITCM/DTCM in this sequence is not proof of zero CPU/stack cost.

| Reported pair | ITCM | DTCM | SRAM | Flash |
|---|---:|---:|---:|---:|
| B/A memory bounds/default-off diagnostics | 0 | 0 | -8 | +72 |
| C/B fixed USB storage | 0 | 0 | +544 | +312 |
| S1/C storage foundation | 0 | 0 | 0 | +920 |
| S2/S1 bounded HAL/BSP | 0 | 0 | 0 | -1320 |
| S3/S2 FatFs integrity | 0 | 0 | 0 | +96 |
| S4a/S3 safe saves | 0 | 0 | 0 | +1792 |
| S4-L/S4a live recovery | 0 | 0 | +8 | -144 |
| S4-L2/S4-L autosave | 0 | 0 | +16 | +1312 |
| S4b1/S4-L2 containment | 0 | 0 | 0 | +1664 |
| S4b2/S4b1 staged replacement | 0 | 0 | +3040 | +8328 |
| R3/prior Morph range | 0 | 0 | 0 | +224 |
| R4/R3 load success feedback | 0 | 0 | +8 | -160 |
| Sum A→R4 | 0 | 0 | +3608 | +13096 |

The official→R4 +464 ITCM remains unallocated across compiler/configuration/source changes. Standalone BUG001/Euclidean reports precede integrated A; do not add their absolute sizes or an integrated +8/-1120 comparison as clean sequential fix costs. R4 hardware-validation branch at48e8b4e6a0b3f6d2cd0c5e614ace39725ddc42f5 history was fetched only for its existing CHANGELOG classification. No root-cause/UX/hardening category is assumed resource-neutral.

New static history script reads every available R5–R11 existing image/ELF pair, checks header/checksum/full physical PT_LOAD/image equality, extracts run extents and verifies R6/R7 common base. R4 calculation uses only retained R4_SECTIONS.txt/report values and is explicitly not a byte-reproduced official or original R4 delivery. See exact identities appended below.

## Primary documentation and limits

- ST DS11532 Rev9, Table59 p152: https://www.st.com/resource/en/datasheet/stm32f765zi.pdf. Endurance conditions and temperature tables are manufacturer specifications, not target measurements.
- ST AN4667 Rev4: https://www.st.com/resource/en/application_note/an4667-stm32f7-series-system-architecture-and-performance-stmicroelectronics.pdf. F7 DTCM access via AHBS distinguishes DMA placement from ITCM; general SRAM/cache changes require review.
- ST PM0253 Rev5 Figure11 p45, primary URL https://www.st.com/resource/en/programming_manual/DM00237416.pdf, exception frame/FPU stacking. The long descriptive URL timed out; the primary search extract supplied the figure. No live FPCCR/CONTROL or bootloader state was read.
- GCC CLZ builtin contract: https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html and https://gcc.gnu.org/onlinedocs/gcc-7.4.0/gcc/Other-Builtins.html. Zero argument is outside the documented builtin contract; ARM instruction behavior is a separate evidence class.

Absent evidence: official1.71 bytes, supplied KnowledgeBase sections, separate preceding R11 independent review, installed bootloader/options, whole-library/indirect stack bound, target clock phase/deadlines/cache/FP nesting, calibrated thermal data, actual long-run device trace. No arbitrary CPU/headroom estimate fills these gaps.

## Final qualified endurance results

| Mode | Scenario | Repeat | Output/state FNV64 | Silent CLP blocks | Peak magnitude | Maximum adjacent step |
|---|---:|---:|---|---:|---:|---:|
| sanitized | 0 | 0 | fe075142b65ca749 | 7920 | 32768 | 63368 |
| sanitized | 1 | 0 | 26fa134119f953f3 | 7920 | 32768 | 52578 |
| sanitized | 3 | 0 | 97eeb43329fb2743 | 7920 | 32768 | 63368 |
| optimized | 0 | 0 | 1f544cb5516c8e8f | 7920 | 32768 | 63368 |
| optimized | 1 | 0 | 5c75420bee82e3ad | 7920 | 32768 | 52578 |
| optimized | 3 | 0 | 0bc29e86d92dfcb5 | 7920 | 32768 | 63368 |
| sanitized | 0 | 1 | fe075142b65ca749 | 7920 | 32768 | 63368 |
| sanitized | 1 | 1 | 26fa134119f953f3 | 7920 | 32768 | 52578 |
| sanitized | 3 | 1 | 97eeb43329fb2743 | 7920 | 32768 | 63368 |
| sanitized | 2 | 0 | 27a6489e6b2cf8e2 | 1584 | 32768 | 65535 |
| optimized | 2 | 0 | 3fc965437c13bd37 | 1584 | 32768 | 65535 |
| sanitized | 2 | 1 | 27a6489e6b2cf8e2 | 1584 | 32768 | 65535 |

All rows return0 and record max_abs_lastOut0.647478997707. Scenario2 has the additional Delay-stereo/Compressor-unity-makeup control restrictions. Every row uses the disclosed CLZ-zero native substitution.

## Existing release artifact/source identities

| Revision | Source commit | Image SHA-256 | ELF SHA-256 |
|---|---|---|---|
| R5 | af3562fece090288a83727524c1035b79f4cf20b | 90caf0cc55203a828f02a108e0bd62f4ac8396c94e823e9680ee433991284a3f | 3dca9babb14a96073d7caae2b41d7a16a073b5827bb60a83813b2336efcbb52b |
| R6 | a743444c01c622dd6a7417a36fb7faa317147310 | 898a235f31d1fd314322ddca1e4b565898f3a5af3302c8cd2f8dabd6ce7f1ac6 | 9790a7ad27463c9274bda872c2aa92ace4387e7b36e68aed2a967a03169e0102 |
| R7 | 6d7cb7f7763158ed9abc465a72df7f2560f26b57 | 991f484b57a3367fe4a93728ff076bdfdd2ebd86e600e392f609bc149958f844 | c018d756ad5405670861edd50ba546edd0b9c05ba77b4db1b4b989af4e141cf1 |
| R8 | 7681da435cb5a70be3b08654227b5183acec1053 | af2e08aa08177d27ae970cf00e5075bf60721530136ace508fcbf918a7830ce5 | 516dd0ba8c33fc44e0819882eb06009da256f485dc0046a526126fa522a87a1d |
| R9 | e2d7963d97cd2d5918e3b80b88156fbbfaa60f47 | b128d5387f421f9e20a64850f6aeac179770dbe12a2382a3fdd7e8845591516c | 4134a7dd865d59d1155613d17984499ba69d4064c1207afc9eb8c31dfd3cb9a7 |
| R10 | 8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7 | 3d8670d2d9a3448452108cc3bdd7e3cae7715aa85fc811c3a5532e11e205c119 | 3cc3b78888aaacf4341f9775efcb7c675cb9e11a11954e3501b289f869418f0b |
| R11 | 64263ed25f2e9c7f5d880b493d943bfba29271a8 | f1779db7861d4c9a54aded8121eb5659a445256ee64453635fcfa4fb1593d2b0 | 350696b2365c973d07b2670a57c72950a65f9a818090e3599c8487f59d313043 |

## Technical research report inventory

These are technical sources inspected for chronology, scope, costs and known failures, not newly rerun validations. Exact report commits make references independent of later branch movement. Two R11 blocker files initially missing from local refs were recovered by exact known commits; neither is the final successful build report.

| Report | Report commit | Git blob |
|---|---|---|
| [2026-10-28_kit-load-confirmation.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/5fdccfeec621b427da6f70b06355f6f86367e5ca/docs/codex-berichte/2026-10-28_kit-load-confirmation.md) | 5fdccfeec621b427da6f70b06355f6f86367e5ca | fe73637493b03c200b241e77559e3fa768606783 |
| [2026-10-28_morph-range-127.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/2e434e302fa4af731331776a6b621fbea0af06fe/docs/codex-berichte/2026-10-28_morph-range-127.md) | 2e434e302fa4af731331776a6b621fbea0af06fe | 5cee9bc2e479da3e48836b1b294ed009ca9e25d3 |
| [2026-10-28_phase-r1-hardware-readiness.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/46fd1d138f80f7ab8e382b7f33230dd6abab3285/docs/codex-berichte/2026-10-28_phase-r1-hardware-readiness.md) | 46fd1d138f80f7ab8e382b7f33230dd6abab3285 | 036253ab39ebc7de29bc4136eea56f0dda3ed9ae |
| [2026-10-28_phase-r2-verified-firmware-packaging.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/c5e70a86c74fd374dcf88f6c193a739626cf4d85/docs/codex-berichte/2026-10-28_phase-r2-verified-firmware-packaging.md) | c5e70a86c74fd374dcf88f6c193a739626cf4d85 | 50c288dc58046297238c5392972df8cee673be48 |
| [2026-10-28_r3-morph-firmware-package.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/67253d444844f4512a031c630db4cd215a067fd5/docs/codex-berichte/2026-10-28_r3-morph-firmware-package.md) | 67253d444844f4512a031c630db4cd215a067fd5 | 3ca96aa03f5a59dbc54052b65a6c5f1ed4e6c2f6 |
| [2026-10-28_r4-firmware-package.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/feffc752ca0a0f1a8e249c2ea0f210f218630991/docs/codex-berichte/2026-10-28_r4-firmware-package.md) | feffc752ca0a0f1a8e249c2ea0f210f218630991 | 05d2a8b1e15d4ccda6fe3c438408cdf5e8b4a5a6 |
| [2026-10-29_r5-clp-independent-review.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/9415539063722dbbfb7a48c6874deb98c5a546f4/docs/codex-berichte/2026-10-29_r5-clp-independent-review.md) | 9415539063722dbbfb7a48c6874deb98c5a546f4 | eb4d036e6064797b6cdf267ac25092158c955fdb |
| [2026-10-29_r5-clp-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/2e995f31d7c4f9dad2918d9f73e9874ff6796844/docs/codex-berichte/2026-10-29_r5-clp-test-build.md) | 2e995f31d7c4f9dad2918d9f73e9874ff6796844 | 21ca5c9a32237502edbc95a2732b66cc56089180 |
| [2026-10-29_r6-clp-shared-core-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/9587340f5bc2246e2aef85f77b2a5aff022604bd/docs/codex-berichte/2026-10-29_r6-clp-shared-core-test-build.md) | 9587340f5bc2246e2aef85f77b2a5aff022604bd | 45d29766d26b99f13777d41becf795465cdb2cde |
| [2026-10-30_r7-clp-parallel-bass-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/5cbcb623933686ac0f5b1003dc9497e8f6d48078/docs/codex-berichte/2026-10-30_r7-clp-parallel-bass-test-build.md) | 5cbcb623933686ac0f5b1003dc9497e8f6d48078 | 195b1db07b7f3811c6ba08f04848c6785109b0a5 |
| [2026-10-30_r8-clp-single-voice-fader-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/7a537b16166987be267acd951ab5116c34807f53/docs/codex-berichte/2026-10-30_r8-clp-single-voice-fader-test-build.md) | 7a537b16166987be267acd951ab5116c34807f53 | 136686fb3ea58645f0a6cddcff8440ff46852b38 |
| [2026-10-30_r9-clp-fader-curve-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/9bec40f335a6d347c59d7fcba428542ba53eb1bf/docs/codex-berichte/2026-10-30_r9-clp-fader-curve-test-build.md) | 9bec40f335a6d347c59d7fcba428542ba53eb1bf | 1d422a039342a0fc414d2d7144e3188652e72dd4 |
| [2026-10-31_r10-clp-multivoice-max-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/43996ab07671446f815cfc04d05a53e31092db28/docs/codex-berichte/2026-10-31_r10-clp-multivoice-max-test-build.md) | 43996ab07671446f815cfc04d05a53e31092db28 | 852fbf1b7135a1b735aa46c1fad38eb0a953aeca |
| [2026-10-31_r10-startup-display-correction.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/57d4df90658244935e297dcc81a5a7b927714330/docs/codex-berichte/2026-10-31_r10-startup-display-correction.md) | 57d4df90658244935e297dcc81a5a7b927714330 | 8ae625b8794b2ac25967123163cb4955f367aae6 |
| [2026-10-31_r11-all-voices-headroom-and-post-mix.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/b98e5d7184be245deca51891c2720035f5d8af54/docs/codex-berichte/2026-10-31_r11-all-voices-headroom-and-post-mix.md) | b98e5d7184be245deca51891c2720035f5d8af54 | 1c236ec986160df5faca875d23227a34b33db802 |
| [2026-10-31_r11-clp-solo-post-mix-test-build.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/b14e6c5777ac318cdcaa43d856ba48b713db1586/docs/codex-berichte/2026-10-31_r11-clp-solo-post-mix-test-build.md) | b14e6c5777ac318cdcaa43d856ba48b713db1586 | 5c1d6fdbde981608412c01a27ab643d424cebfc3 |
| [2026-10-26_bug001-validated-correction-publication.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/8ee08ec28fc74d95db02d3fe0ddaa649db018937/docs/codex-berichte/2026-10-26_bug001-validated-correction-publication.md) | 8ee08ec28fc74d95db02d3fe0ddaa649db018937 | 61463954da8b60580a25b0f2d311052d4ceaf870 |
| [2026-10-26_euclidean-length16-correction.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/de28994d53e0d8efc9091b24b868bd3a69295a85/docs/codex-berichte/2026-10-26_euclidean-length16-correction.md) | de28994d53e0d8efc9091b24b868bd3a69295a85 | af77c8a2a088de272c86c6a4416be5450668a054 |
| [2026-10-26_phase-a-integration-safety-preflight.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/9f02f3a2a6c60c9858d1e55d7baf9e762ae20395/docs/codex-berichte/2026-10-26_phase-a-integration-safety-preflight.md) | 9f02f3a2a6c60c9858d1e55d7baf9e762ae20395 | 474ee90eeb24946bde178db259a5329de0100b50 |
| [2026-10-26_phase-b-memory-safety-runtime-diagnostics.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/16e65a0d276f71545495d06fadb5057138f7f295/docs/codex-berichte/2026-10-26_phase-b-memory-safety-runtime-diagnostics.md) | 16e65a0d276f71545495d06fadb5057138f7f295 | 62b6ee170196249dd8c59f3ae1a8726a09ef578a |
| [2026-10-26_phase-c-usb-allocator-safety.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/9a2ba8bb4ae1928116b2d3a50ccb6dfa4c7a23ff/docs/codex-berichte/2026-10-26_phase-c-usb-allocator-safety.md) | 9a2ba8bb4ae1928116b2d3a50ccb6dfa4c7a23ff | 02980d6b0de11fd9e43b2621c35a04e39653f9bf |
| [2026-10-26_phase-d0-sd-storage-safety.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/a80a37e0704cc5f178a36878f1f22a155cc6f21e/docs/codex-berichte/2026-10-26_phase-d0-sd-storage-safety.md) | a80a37e0704cc5f178a36878f1f22a155cc6f21e | 75d483c6906f01ade4e43d8f9a2bbd578dc0a6b7 |
| [2026-10-26_phase-s1-sd-storage-foundation.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/e66f31b979455d977993ad21583a4ff191ea6cf6/docs/codex-berichte/2026-10-26_phase-s1-sd-storage-foundation.md) | e66f31b979455d977993ad21583a4ff191ea6cf6 | e1c646c3e588df061ed4df183e87e5822aaa9f79 |
| [2026-10-26_phase-s2-sd-hal-bsp-timeouts.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/bebc12701234e6b2f622449c72e291c6f0957f7f/docs/codex-berichte/2026-10-26_phase-s2-sd-hal-bsp-timeouts.md) | bebc12701234e6b2f622449c72e291c6f0957f7f | 9b44e0ef9bc1c8585e04fbe05249ab5fc691b81e |
| [2026-10-26_phase-s3-fatfs-data-integrity.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/b68238fe315c3f526aa4677d94b93d9c0d262aac/docs/codex-berichte/2026-10-26_phase-s3-fatfs-data-integrity.md) | b68238fe315c3f526aa4677d94b93d9c0d262aac | 88ab6565c42881762d47f9be819f9c50ae5e3e76 |
| [2026-10-26_phase-s4a-safe-file-saving-verified.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/2f4951f62586a0e42b44e117c04a2449c0427c94/docs/codex-berichte/2026-10-26_phase-s4a-safe-file-saving-verified.md) | 2f4951f62586a0e42b44e117c04a2449c0427c94 | 15916ed38af3af63d65538ea8933d91de67a05dd |
| [2026-10-26_phase-s4a-safe-file-saving.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/2f4951f62586a0e42b44e117c04a2449c0427c94/docs/codex-berichte/2026-10-26_phase-s4a-safe-file-saving.md) | 2f4951f62586a0e42b44e117c04a2449c0427c94 | 54050291791db427e3678836b691633c38aa4605 |
| [2026-10-27_phase-s4b1-project-containment.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/09ed6500ed592a418ba1689504c801268087e062/docs/codex-berichte/2026-10-27_phase-s4b1-project-containment.md) | 09ed6500ed592a418ba1689504c801268087e062 | 2d5411d023486ee8aa11b2957a754938e6700d8f |
| [2026-10-27_phase-s4b2-project-replacement.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/4f019a9f3cc5b433f9d026d7975c8beb9864658e/docs/codex-berichte/2026-10-27_phase-s4b2-project-replacement.md) | 4f019a9f3cc5b433f9d026d7975c8beb9864658e | 7686e79f361e9487a0ef52cbea6e316b67df568a |
| [2026-10-27_phase-s4l-live-safe-storage-recovery.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/25fbc6a9d5daa99d622a269e690c4f8197124e95/docs/codex-berichte/2026-10-27_phase-s4l-live-safe-storage-recovery.md) | 25fbc6a9d5daa99d622a269e690c4f8197124e95 | 64db7ab625457a4876483b522e9483d2fa352eef |
| [2026-10-27_phase-s4l2-silent-autosave-recovery.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/1192016476e4eff35fd4c31cebaacc54133df5f0/docs/codex-berichte/2026-10-27_phase-s4l2-silent-autosave-recovery.md) | 1192016476e4eff35fd4c31cebaacc54133df5f0 | 8bbcb4d454d31fd53c03c1dfc33be2a7bc719c0e |
| [2026-10-31_r11-arithmetic-and-filter-off-post-mix.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/84bc8a8c81fbc3d348c509ee5a2c7d5329d4cf8c/docs/codex-berichte/2026-10-31_r11-arithmetic-and-filter-off-post-mix.md) | 84bc8a8c81fbc3d348c509ee5a2c7d5329d4cf8c | 94beadf64bc73995f022b372c5a0ceec773e325e |

Additional pinned prior design/triage references:

- MIX/VOL design: ae39a8b5398da1ea36bcb0148d40c8ab8461d184, docs/codex-berichte/2026-10-31_clp-mix-vol-decoupling-design.md.
- ARM source arithmetic triage: d0215ef5b090313ff31ce2ae0da88d72dd2100b2, docs/codex-berichte/2026-10-31_clp-source-arithmetic-arm-triage.md.
- [R4 hardware-validation CHANGELOG](https://github.com/DocLXR-02/lxr-desktop-app/blob/48e8b4e6a0b3f6d2cd0c5e614ace39725ddc42f5/CHANGELOG.md), fetched read-only for classification; not an official1.71 binary validation.

## Input and local result identities

Supplied file hashes (identical duplicate attachments were not treated as new authorization):

| File | Bytes | SHA-256 |
|---|---:|---|
| LXR_Build_Cost_Ledger_2026-11-01.md | 27588 | 46113956c90a17206fc9d30d17b95090b246cbc0adc6837c689a20700f8b4c5f |
| LXR_Codex_Reserve_Audit_Task_2026-10-31.md | 17705 | af02b86d4c33f05d3819f2eeef60b5c0cd58d57ca96fca06f31e4a269de4db89 |
| LXR_FX_Expansion_Planning_2026-10-31.md | 13283 | eb5a9a300334c96fa528a7e7371a3b16efcc3358bfb6106569bce7abeb1b6398 |
| LXR_Resource_Reserve_Audit_2026-10-31.md | 29373 | a1569fca61432253004d991ff30e64724a142508df5c402126926439394b4e73 |

The following local analysis files remain outside the source checkout. Hashes identify actual retained evidence; only the Markdown summaries are published under this task. They are not firmware artifact identities or substitute CI deliverables.

| Local relative file | Bytes | SHA-256 |
|---|---:|---|
| prepare_stack.py | 1751 | c56dd199f9fedf186cc81cbf4e49263dd95e0a7738a2f93a7b342a2f466ee6f6 |
| stack-compile.sh | 94962 | 89fc4d2c6fee9140b75da31a2be98c10f975fea648e537f0bc3820fc3b0ef2b1 |
| extra-compile.sh | 4025 | 97ef4503850d230888d6a9c72491f1aaa02a03e344f7152449a5113944ebc40e |
| static_stack.py | 5541 | 4a2e5a61f6b70043b5cc399d40a36fa0ea1c0a7b145b3bd74ea45e9cc7fce232 |
| callgraph.py | 5036 | dba0983cd688f89636ab5386daa2848b6188717e05b72fae0dbb71128f130b8d |
| prepare_soak.py | 2853 | e22b3e1de48b41808e61413c080730bfd3688102e301635aa16ae0074419036d |
| soak.c | 3111 | f43df0af1ac0479a04ee83370668c41675f54c0e17058e533ca57636b02f5713 |
| finite.c | 623 | ff8e21da6ac752cae28568d1daed6ee2cfb269115684d496e37e69e436e7cc2f |
| run_soak.py | 1107 | 3bdeedd478b4cbb406a1edb8f57e0f31cafb62f065129dff7f1a98d927c7245e |
| boundaries.py | 4388 | 21851b5e832f51052bdb72cf2dfb69b3f6c2e834b96d7a453544fec6bac2c6bb |
| scheduling.py | 4114 | 6a2084462ef310da354d428ed9e1e7439302db06e258da5c389a1e73d3577d9e |
| arm_runtime.py | 2399 | 3f9e4b4c3f5336173668571ea680b8819cccdb26823179a65c9b79dff8c2f710 |
| arm_boundaries.py | 3366 | 6b528e727889d55fc7580c6368c5b0c7e39a8c52eb720e98ff32600cf09d330c |
| usb_arm.py | 1184 | bad58746663b81a7fc43c9576489e428db80fdbf44492504e4b81ed381fa33ba |
| history.py | 3361 | 0918340463d20c05dc6fb9d14c63518df53a9758cf3701ea3e2103efee0864f1 |
| results/stack-commands.json | 121687 | 5f206f508406e418e9901cd73ca0c3c9410a35c063b612f6d992ff7942ff2339 |
| results/stack-compile.log | 285 | e5d5fdd1e3a8634d1981bc07130f5176636ecd4a28fef63c6271e44f57db621b |
| results/static-stack.json | 493150 | 0ec9bad80a6d6c8f65d879a2051ad66485f4e16270bbd492490f9d7724424b13 |
| results/callgraph.json | 397702 | 11e5ec981083d9b6966eff39f7e06d1a15520833a968c1120329c111d39b50ae |
| results/diagnostic-object-sizes.json | 378 | bdb949acef7f026e116164ea00d17821a65012abc36c2c1cc0be3e4e436d05d2 |
| results/soak-commands.json | 5707 | 9d9c080b654a6152d733df14d5c912585815487a404596bd13f4d0650f52459d |
| results/soak-smoke.json | 1819 | 28d4d84607c3675734eb1ca29aa57640cca58c2bb0e5b157144872aad76a210a |
| results/soak-compatible-smoke.json | 2547 | d298afd9425bb791d2e8646f1b2e3829203f16eeede4e2125f7bd32340f6698e |
| results/soak-initial-results.json | 4293 | 943c0a24e4ebb703563ee58f9304c2c7f5b2251ddf8164fb06a3b634a2bc3244 |
| results/soak-stereo-delay-results.json | 3921 | b6b64cdd4786328183d2f1dee39394826becd93850798bfc67e1a75a5156ecd2 |
| results/soak-qualified-final.json | 4059 | bcbc9ed087faa01cff5501ac7ee37e053fcb27a4286950b65c0944f16cedcf9f |
| results/queue.log | 167 | a6ee1cafc6bb09220d6f4a65c9abab8a8adb8fb59cce6afae3ae5e85d373f55d |
| results/schedule.log | 243 | 5129feef8ef21780fc7be51175510e878b3d7676603449478a474f32271f0818 |
| results/error-counter-expected-failure.log | 148 | d0e6757a03606516bbf15a580bf5610543322307253940b08e83e75b8ef1ef9a |
| results/shuffle-native-expected-failure.log | 148 | 5a7de9dad218d9dc4062a9f3f5cae3194f25fce934afd6be53c6499833ffa4e5 |
| results/clz-arm.json | 767 | fc711f5282d657d09a04f471a8b393264649e219bad44fd9740ba8a5b875cb71 |
| results/full-fx-init-arm.json | 74 | f6675635f668fd9fc3c615f4bb8edadf396d8833e29b4fdc17358e829c337b32 |
| results/arm-boundaries.json | 1236 | df024feef0e2d388863c5732bdee6574aebbbcfc2d9c20d3056122e28685d657 |
| results/usb-arm.json | 825 | 0de9e348a69fde722d121a4a54a6f0d6c19e26dd3341f88ea9279824599704a8 |
| results/inherited-functions.json | 1180 | 879f36b992f5074b6d4cfdc5ab5f85e7d895d03dfdbe891adb1b89d88bdd4ed9 |
| results/release-history.json | 3678 | efdde98b88033a32d1264c645f82e692153af9739029f3d96a8af6203c262f68 |
| results/report-inventory.json | 8268 | d62fd24392f17849e9ab0de977de696aa35480168b2dec09157559d7d62c1d82 |
