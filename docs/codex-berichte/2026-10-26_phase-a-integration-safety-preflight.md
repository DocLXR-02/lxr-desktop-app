# LXR Firmware Lab — Phase A integration and software safety preflight

Date: 2026-10-26 UTC. Repository: DocLXR-02/lxr-desktop-app.

## Status and scope

Both independent fixes were integrated and validated in the existing `/workspace/lxr-desktop-app` checkout. No additional worktree was created. This is a **software-validated development integration**, not a hardware-validated release candidate or authorization to flash.

AGENTS.md was read from remote lab-setup (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`). The user authorized source integration, software tests, builds, static analysis and restricted GitHub publication. No hardware operation, flashing utility, option-byte operation, bootloader modification, new instrumentation, PR or merge into an existing branch occurred. No binary is committed or published. All new documentation and commit messages are English.

| Reference | Verified commit |
|---|---|
| lab-setup, integration/report base | `5829398cea5d337200393757e39a37d4d8148256` |
| Bug 001, preserved | `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0` |
| Euclidean fix, preserved | `67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72` |
| main, preserved | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Integration branch | `integration/bug001-euclidean-preflight` |
| Integration commit | `e19bb0544dcc8f07138a9857c78022e8efaecd81` |
| Integration tree | `6dfeb48fac6a1eb49b41c60590aa7f9eae5be4d5` |
| Separate report branch | `codex-reports/2026-10-26-phase-a-integration-safety-preflight` |
| Report path | `docs/codex-berichte/2026-10-26_phase-a-integration-safety-preflight.md` |

The current remote heads matched all three user-supplied fix/base IDs exactly; there was no drift. The desired integration branch did not exist. The initial checkout was clean on Bug 001. The existing separate Euclidean worktree from an earlier task was not used or modified.

Integration URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/integration/bug001-euclidean-preflight

CODE = confirmed source/configuration. BUILD = executed compilation, tests or binary analysis. HYPOTHESIS = plausible consequence not demonstrated. OPEN = not established or requiring hardware/external evidence. Numbers called budgets below are derived configuration values, not measured CPU load.

## Integration, complete change scope and review

Executed:

```sh
git status --short --branch
git switch -c integration/bug001-euclidean-preflight 5829398cea5d337200393757e39a37d4d8148256
git cherry-pick --no-commit 7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0 67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72
git diff --cached --stat
git -c core.whitespace=cr-at-eol diff --cached --check
```

CODE/BUILD: Git auto-merged `presetManager.c` without conflicts. No manual or architectural resolution was needed. There are exactly five changed production files and nine original regression files. Every nonshared file (13 files) was byte-compared with its independently validated fix commit and matched. The shared preset manager contains the disjoint transport-sweep and pattern-reader changes. The full combined diff is preserved in Appendix A. No additional correction was invented.

- `Core/Src/Preset/presetManager.c`: Bug 001 reserved transport-slot suppression in Morph/immediate sweep, plus Euclidean staged/validated length tail in both loaders. Integration references: `preset_isTransportParameter():1267`, `preset_morph_tick():2120`, `preset_morph():2201`; shared tail reader near303; active normal reader1716 onward.
- `Core/Src/Soundengine/MIDI/MidiParser.c`: CC0 cache/notification protection, CC2 and NRPN bounds, preserving supported external mute/bank functionality.
- `Core/Src/Soundengine/MIDI/CortexParser.c:255ff`: bounded private length receiver, legacy zero decoding, invalid-byte consumption without destination misalignment, counter saturation.
- `Core/Src/Soundengine/Sequencer/EuklidGenerator.c:53ff,144ff,232ff`: defined CLZ zero case, bounded ingress, literal16 transfer; rhythm recursion unchanged.
- `Core/Src/Soundengine/Sequencer/sequencer.c:274ff`: length setter/getter boundaries. Consumers retain their existing scheduling, flam and quantization algorithms.
- Five `tests/bug001/` files and four `tests/euclidean/` files are unchanged copies of the validated suites.

CODE: All files outside this exact set match lab-setup. Therefore no linker, clock, startup, vector table, flash, bootloader, build configuration or unrelated DSP code changed. Version remains CMake1.7. No integration merge commit was created: the published combined commit has one parent, lab-setup.

## Environment and build results

The cloud-environment-runtime skill was used to check the prepared environment. No tool installation or purchase occurred. Each build/test shell used:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

Tools: CMake3.31.10, Ninja1.13.0.git.kitware.jobserver-pipe-1, Python3.12.14, Arm GNU `14.2.1 20241119 (15:14.2.rel1-1)` with newlib-nano, native GCC14.2.0. These match the provisioned environment and independent Euclidean compiler. Ninja's detailed version suffix is recorded rather than shortened away.

BUILD: All requested commands returned0:

```sh
cmake --preset release
cmake --build --preset release --parallel 4
cmake --preset debug
cmake --build --preset debug --parallel 4
```

Existing ignored build directories were reused; Ninja rebuilt affected units and relinked each configuration. These were not falsely described as fresh clean builds. The final same-tree builds were repeated after importing the exact published integration commit and succeeded. No warnings/errors occurred in the normal firmware builds. Separate static-analysis warnings are explicitly retained below. Presets preserve the source's `-O3` audio optimization in both configurations; Debug adds debug information/DEBUG. Generated application names, derived from the current CMake project, are `LXR_V2_STM32F765.elf`, `.bin`, `.map`, and `LXR_V2_V1.7.img`.

The build initially ran against the staged combined tree while HEAD was still the base. Publication verified the exact tree hash above; final builds on pinned commit `e19bb054...` used that same tree. No source content changed between those operations. This exact commit identifies the artifacts; the version label was not changed to imitate official1.71.

## Regression results: all available fix suites

BUILD: Both suites ran in the same Linux host checkout using native GCC, original/extracted source functions, and ASan+UBSan. No test group was skipped or failed. The image checker also ran. The optional Euclidean baseline-recording utility is an oracle-generation aid, not an omitted regression group; the default suite verifies all pinned masks automatically without network access.

| Command/group | Status | Meaning |
|---|---|---|
| `python3 tests/bug001/run.py` parser group | PASS | CC0, Drive CC/NRPN, external bank forwarding, supported NRPN mutes, invalid selectors, CC2 bounds |
| Same command, sweep group | PASS | All256 Morph positions, reserved slots suppressed, restart/tick behavior and immediate sweep service cadence preserved |
| `python3 tests/euclidean/run.py` six groups | PASS | Generation/bounds/playback/persistence/malformed/private protocol; exact output below |
| `python3 tests/euclidean/check_images.py build/release build/debug` | PASS | Both current generated application images/ELFs |

Native tests use `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all`, non-PIE executables and leak detection disabled for the environment. They do not execute ARM DSP instructions or firmware on hardware. Stubbed HAL/FatFs/audio functions and extracted-body boundaries are described in the unchanged test READMEs. Bug 001 host traps verify command suppression and memory bookkeeping; they do not measure audible Filter Drive behavior. Euclidean tests cover original consumer bodies, not a full interrupt-driven clock/song simulation.

Exact final regression stdout:

```text
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence
PASS generation: 2176 exact masks match lab-setup; literal lengths; CLZ unshimmed
PASS bounds: all byte-valued track/pattern/length inputs
PASS playback: 294912 flam/shift ticks; 4160 recording positions; mixed lengths
PASS persistence: 130 full named/autosave roundtrips, unchanged layout/payload
PASS malformed: 10210 truncation/range/I-O/legacy-tail cases; no partial lengths
PASS protocol: 180802 byte-value/counter boundary calls; no wrap
PASS all six ASan/UBSan groups
```

The Euclidean mask oracle SHA-256 remains `212cd0c7e3da155464507d1bf22e90c0e50d38f32700cfbfefcde8f3307ac883`. Candidate tests use the actual production CLZ guard without a shim. Legacy0 decodes16; valid1…64 is preserved; incomplete/invalid required metadata fails without installing partial lengths/kit. V1 absence defaults16; partial V1 tails fail. PatV03 layout and save functions are unchanged. Real historical/official file compatibility remains OPEN.

## Image validation and boot boundary

BUILD: The checker validated16-byte header, `LXRV2IMG`, little-endian payload size, zero padding, checksum `0xFF-(sum(payload)&0xFF)`, exact raw-bin identity, image length, flash limit, stack/reset-vector plausibility, and allocated ELF VMA/LMA bounds. No binaries were uploaded.

```json
[
  {
    "checksum": "0xFB",
    "configuration": "release",
    "elf_sha256": "19bf4a6211b19649eefd6843e5426c65b89a8384ca408fb1f6e8b48410f7d9c3",
    "image_bytes": 275580,
    "image_sha256": "41e225f8398b8cd80c916be0aaebd1045a9fa505cf444c51856c2649f243f905",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 222700,
      "ITCM_RAM": 15016,
      "RAM": 271040
    },
    "payload_bytes": 275564,
    "reset_vector": "0x08014E5D",
    "result": "PASS"
  },
  {
    "checksum": "0x36",
    "configuration": "debug",
    "elf_sha256": "0e7738f3f3d058d7405ec4b06518e1fb911db729ee4bdb092ddf5b63596b63db",
    "image_bytes": 275628,
    "image_sha256": "66f697135ba7e93df549da6923237ae337abc3e039133a17f5598c4e69cefbb9",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 222748,
      "ITCM_RAM": 15016,
      "RAM": 271040
    },
    "payload_bytes": 275612,
    "reset_vector": "0x08014E5D",
    "result": "PASS"
  }
]
```

Raw-bin SHA-256:

```text
5fa94b1a800d180a5fe4edaa5c2369eb67c45ed74f25bb2a9008998fe3b5ae3c  build/release/LXR_V2_STM32F765.bin
b45fcf422f9c2b5c835658413518cbf8dcad885bab4df9e43b076618eee176b8  build/debug/LXR_V2_STM32F765.bin
```

## Static memory safety

BUILD: Release linker map, ELF sections and symbol sizes were inspected with `arm-none-eabi-objdump -h`, `arm-none-eabi-nm -S --size-sort`, `nm`, and the unchanged linker script `STM32F765VGTX_FLASH.ld:30ff`.

| Region | Integrated Release used | Capacity | Static remainder | Independent Euclidean used | Delta |
|---|---:|---:|---:|---:|---:|
| ITCM |15016|16384|1368|15008|+8|
| DTCM |107392|131072|23680|107392|0|
| General RAM |271040|393216|122176|271040|0|
| FLASH linker report |223552|1015808|792256|224680|−1128|
| FLASH complete raw load footprint |275564|1015808|740244|276684|−1120|

Debug FLASH linker report is223600; full payload275612, leaving740196 bytes by full footprint. Other Debug region uses are equal to Release. Linker FLASH allocation omits some manually placed TCM load-image extent; the complete payload and all LMAs were checked separately. The table does not mistake linker allocation for raw image footprint. Comparison uses the same GCC/configuration, but different source paths/debug metadata prevent claiming full binary reproducibility; instruction-layout differences can change linker veneers. No performance conclusion follows from8 added ITCM bytes or smaller FLASH.

CODE/BUILD: Heap minimum is0x200 (512 bytes), stack minimum0x400 (1024). Heap reservation is included in general RAM accounting. `_estack=0x20080000` is the initial vector value; critically, `Core/Startup/startup_stm32f765vgtx.s:68` loads SP from `_dtcm_stack=0x20020000`. Runtime main and interrupt stack therefore grows into the DTCM remainder, not automatically into general RAM's122176 free bytes. Linker DTCM stack assertion only reserves the minimum1KiB. Actual worst-case stack consumption is OPEN. Hardware exception/FPU stacking and nested IRQs must be included.

Large linked objects (BUILD): `seq_patternSet`233536 bytes at0x20029014; `FX_BUFFER`65536 at0x20009e18; `transientData`26460 at0x20000000; `mainParameters`15808 at0x20021e54; `sine_table`8194; pattern backup3650; song data4096. These are static, within measured regions; free space is not equivalent to runtime safety.

Additional static analysis actually compiled77 application translation units to `/tmp/phase-a-stack/` using original Release compile commands plus `-fstack-usage -Wstack-usage=2048`, without changing source/configuration or replacing linked objects. All analysis compiles returned0, with six stack warnings retained in Appendix B. Largest fixed frames: formatting routine1096, mixer448, saveProject416, loadProject384. Mixer locals at `mixer.c:590–591` are128-byte sampleData and256-byte fxBusData before frame overhead.

CODE: DrumVoice239ff, CymbalVoice157ff, HiHat165ff, Snare148ff and FX Compressor104ff/Ringmod31ff contain size-dependent local arrays. Known audio paths pass64 samples, bounding individual buffers there, but GCC reports potentially unbounded usage when it cannot prove that argument bound. These diagnostics do not prove an infinite allocation or a concrete stack overflow. A complete call-chain bound, including library and preemption frames, was not derived. Euclidean recursion (`EuklidGenerator.c:62–140`) is bounded by validated small length inputs in supported generation; filesystem `delete_node()` (`presetManager.c:608–636`) recurses with directory nesting. Storage wait loops and deep paths remain latency/stack concerns.

CODE/BUILD: No direct application `malloc/calloc/realloc` call was found, but **runtime allocation is linked**: map pulls `_malloc_r/_realloc_r` from newlib-nano formatted I/O (`sprintf`→`_svfprintf_r`). `Core/Src/sysmem.c:63ff` custom `_sbrk` is disabled. Linked `_sbrk` at0x08009fdc comes from libnosys, starts heap at0x200620c0 and merely advances a pointer with no bounds test (disassembly inspected). Thus the512-byte heap reservation is not an allocation cap. Heap exhaustion/fragmentation/library reentrancy require assessment and monitoring; no actual heap high-water was measured.

### DMA placement and memory-access questions

CODE/BUILD: `AudioCodecManager.c:17–18` places both256-element int16 arrays (512 bytes each) in `.dtcmz`. ELF addresses: dma_buffer0x2001a01c, dma_buffer2 0x20019e1c. Both are4-byte aligned, not32-byte cache-line aligned. Audio HAL MSP uses halfword DMA alignment (`stm32f7xx_hal_msp.c:275–276,336–337`). DTCM is normally outside the Cortex data cache; main enables both caches (`main.c:254–257`). Do not infer that arbitrary DMA buffers elsewhere are coherent from audio placement.

OPEN: Confirm STM32F765 DMA1/DMA2 bus visibility of these exact DTCM addresses against the device bus matrix and actual setup; no target memory-access test or authoritative device-manual verification was performed here. F7 and H7 assumptions must not be interchanged. This is an unchanged placement, not a new patch regression or a claimed proven DMA failure. ADC buffer also lies in DTCM. SD disk I/O inspected (`FATFS/Target/sd_diskio.c:147ff`) uses blocking BSP block reads/writes and polling; configured SD DMA channels alone do not prove this path uses DMA. SRAM cache maintenance/alignment needs review for whichever transfer paths are actually active. No MPU/cache-placement changes were made.

## Audio scheduling and CPU budget

CODE: `config.h:29–40,72–75` selects interrupt processing, big buffer64, SYNTH_FS44.1kHz and synthesis compensation REAL_FS44140. `main.c:1027–1060` uses HSE16MHz (`stm32f7xx_hal_conf.h:96`), PLL M8/N216/P2, giving nominal216MHz CPU, PLLI2S N113/R5 giving45.2MHz I2S clock. Both I2S initializers (`main.c:1244ff,1277ff`) select16-bit stereo and MCLK output. HAL divider rounding (`stm32f7xx_hal_i2s.c:350–406`) yields aggregate divider4: **derived nominal sample rate44140.625Hz**. REAL_FS44140 is a rounded algorithm constant, not an observed clock measurement. Actual oscillator/sample clocks remain OPEN.

CODE/derived: Each DMA request transmits128 halfwords =64 stereo frames per DAC,256 bytes per half, with two halves per512-byte output array. Two DAC/I2S streams provide four channels. Nominal block interval64/44140.625 ≈1.450ms, approximately313181 CPU cycles at216MHz. This is the upper periodic opportunity, **not free processing time**: interrupt latency, DMA rearm, sequencer/MIDI and other work consume it.

`Soundengine.c:23–36` only renders when shared bCurrentSampleValid is not SAMPLE_VALID, selects the requested free half, renders both output buffers and marks ready. `mixer.c:548ff` runs voice async updates, per-voice sync blocks and FX. `stm32f7xx_it.c:261–273` TIM2 handler calls seq_tick, renders, then UART MIDI. `Soundengine.c:77–119` main-loop service temporarily disables TIM2, services sequence/render, resets timer counter, re-enables interrupt, processes MIDI/USB and one Morph tick. Parameter sweeps repeatedly service this path. Euclidean generation/load validation occurs on control paths and can overlap ongoing audio work; neither fix changes scheduling or audio algorithms.

`cs4344_cs5343.c:86–139`: normal-mode DMA completion callbacks rearm the corresponding SPI2/SPI3 stream; SPI3 (`SWITCH_FLAG=1`) requests next free-half rendering and preloads TIM2 counter1450. TIM2 ARR1500/prescaler1 (`main.c:1405–1407`). Main service resets it to1950 (`Soundengine.c:85`), above the configured ARR; effective runtime behavior and latency must be measured, not inferred as a clean fixed-period scheduler.

NVIC CODE: TIM2 priority4, audio DMA1 streams priority5, timer14 priority0, USB/UART priority3, SDMMC2 and some SD DMA IRQs0 (`main.c:1549–1561`, `stm32f7xx_hal_msp.c:542,682,696,774,844`). With configured grouping, audio rendering can delay lower-priority DMA callbacks, while higher-priority work can delay rendering. Blocking SD loops, UI/name formatting, dense MIDI, FX/voice-dependent computation, parameter-update peaks and flash-resident tables can add jitter. HYPOTHESIS: those peaks can threaten deadlines; their duration has not been measured. No CPU-load percentage, thermal reading or audio reliability result is claimed.

### Existing error counter and its limits

CODE: volatile global `error` (`cs4344_cs5343.c:82`) increments if a completion callback observes bCurrentSampleValid not ready (`:88–92`), before discriminating SPI2 vs SPI3. It detects that specific shared-flag observation. It is not a complete DMA underrun/timing monitor: two stream callbacks can see different readiness phases, the second may count a request just posted by the first, callback lateness/rearm gaps can occur without that observation, and it does not measure maximum render time, data freshness, analog glitches, or faults. No user-visible/persistent report or comprehensive HAL-I2S error aggregation was established. A zero counter alone would not prove reliable audio; nonzero needs event-order correlation.

## Proposed future hardware measurement plan — NOT implemented/executed

Requires separate explicit approval, validated project/firmware backup, recovery procedure and appropriate device/probe access. No flash/debug/option-byte utility was invoked in this task.

1. On an explicitly authorized instrumentation build, enable Cortex-M7 DWT CYCCNT and record unsigned entry/exit deltas around `mixer_calcNextSampleBlock`, entire TIM2 sequence/render/MIDI service, and main `soundengine_process`. Also record SPI3 completion/request→ready elapsed cycles and callback separation on both streams. Track maximum, count and accumulated cycles in small fixed RAM fields; no per-block printf, SD logging or allocations. Handle counter wrap (~19.9s at216MHz); read and clear snapshots atomically outside the audio path. DWT support/access must be confirmed on the device.
2. Measure an empty instrumentation path to calibrate overhead; expect tens to a few hundred cycles per sample point depending on interrupt/cache/bus effects, target <0.1% of a313k-cycle block. This is a design estimate, not a benchmark. Consider existing GPIOE2 TIM2 pulse with a logic analyzer as an independent timing cross-check, respecting its existing use.
3. Stress all voices/FX/filter Drive, sustained Morph motion including endpoints/identical kits, dense MIDI/NRPN traffic, Euclidean lengths1/16 with shifts/flams/live recording, pattern transitions and SD/UI activity. Compare idle and combined demand using fixed scenarios and bounded observation windows. Read actual clock/register state to establish the true deadline first.
4. Correlate shared readiness counter with per-stream DMA/HAL errors, missed request sequence numbers and callback lateness. Suggested initial criterion: zero verified missed/reused blocks/errors, maximum request→ready safely below measured block deadline, provisional20% margin (≈1.16ms at nominal configuration). Any overrun, unexplained counter growth or rearm gap requires investigation, not acceptance by average CPU load.
5. Establish a safe DTCM stack watermark region from static data end to runtime stack top, paint only unused space before workload begins, preserve live frames, include nested IRQ and FPU frames, then inspect high-water/canaries without stopping live audio unpredictably. Separately monitor heap high-water against SRAM end and implement a reviewed bound before treating allocator safety as established. Require measured margin above demonstrated peak, not just1KiB linker minimum; a full bound remains preferable to stress-only evidence.
6. Proposed fault record: stacked r0–r3/r12/LR/PC/xPSR, EXC_RETURN, MSP/PSP, CFSR/HFSR/DFSR/AFSR/MMFAR/BFAR/SHCSR, reset cause, readiness/error counts, build ID and checksum in a small reviewed retention/noinit area. No formatting, FATFS or heap call in a fault handler. Validate stack validity/FPU extended frames before dereferencing. Capture first fault once, retain evidence across a controlled reset if retention semantics are established.
7. Recovery plan must address loss of audio service, hangs and partial file writes. Do not enable a watchdog blindly: choose timeout/refresh ownership only after worst-case storage/startup/stress latency is understood; test capture/reset behavior in a separately approved phase. No automatic reset or new fault handler was added here.

## Fault handlers and watchdog

CODE: HardFault102ff, MemManage125ff, BusFault140ff and UsageFault155ff in `stm32f7xx_it.c` loop forever. HardFault in DEBUG reads CFSR and splits it into volatile debugger locals but does not persist it, capture stacked PC or provide a user-visible report. No active IWDG/WWDG initialization/refresh call was found in Core. A weak WWDG vector exists in ELF but is not evidence of activation. Runtime inherited watchdog/option-byte state is OPEN without device inspection; source provides no established watchdog recovery. Existing GPIO/debug markers do not replace a persistent fault record.

## Bootloader, flash and configuration safety

CODE/BUILD: Application FLASH starts0x08008000, length992KiB; vector table offset0x8000 in `system_stm32f7xx.c:79,145`. First32KiB is excluded from the normal application image; ELF ISR vector and reset-vector range checks support that boundary. `config.h` contains a stale0x4000 bootloader comment, but the effective linker/vector/image evidence is0x8000.

`main.c:17` defines UPDATE_BOOTLOADER0. Bootloader overwrite/option-byte/write-protection code around539–802 is behind that disabled branch. Generic flash routines remain in source (`flash_if.c`) but neither correction calls them; a targeted Release symbol search found no linked FLASH_If/option-byte routines. The integration diff changes no startup, CPU PLL/I2S clocks, linker region, flash file, full-device target or bootloader source. Only normal configure/build and read-only binary tools were run, never flash/flash_full/OpenOCD. No bootloader hex/directory was configured for these builds.

Protection from source exclusion is not hardware write protection. BUILD proves placement of this application payload; it does not establish physical sector0 protection, bootloader content/authenticity, option bytes, bootloader acceptance of a particular source version or recovery success. These remain OPEN and must not be checked by disabling protection.

## Compatibility and explicit pre-deployment decision

Official public reference identified2026-10-26 is v1.71; private support1.72 is not an established public release. Open-source version1.7 is unchanged. No official image was obtained/compared in this task. BUILD confirms application-format consistency; CODE confirms local source invariants; runtime/official source equivalence remains OPEN.

1. **Both fixes integrated? BUILD: yes**, exact combined tree and both suites verified, no conflict resolution required.
2. **All relevant tests pass? BUILD: yes**, no skipped regression groups; no hardware test implied.
3. **Static allocations within regions? BUILD: yes**, with the headroom table above.
4. **Headroom? BUILD:** ITCM1368, DTCM23680 before runtime stack, RAM122176 beyond static+heap minimum, full Release load footprint leaves740244 FLASH bytes.
5. **Bootloader/protection/startup/clocks changed? CODE: no.** Physical protection state is OPEN.
6. **New safety concerns?** Newly documented baseline concerns include unbounded heap growth through linked libc, size-dependent stack frames, DTCM runtime stack, DMA visibility/coherency questions and weak fault recovery. No new integration-specific critical failure was demonstrated. The separate static analysis warnings must not be hidden behind successful normal builds.
7. **Before first controlled hardware test:** obtain explicit approval and backups/recovery, confirm DMA/bus and clock assumptions, agree on lightweight diagnostic instrumentation and acceptance criteria, establish heap/stack bounds/margins, and review fault/recovery strategy. Measurements have not been performed.
8. **Technically ready?** Ready as an integrated software development revision for review and preparation of a future hardware-validation phase. **Not ready for deployment or an uninstrumented acceptance claim.** Physical execution remains gated by the outstanding checks and explicit approval. Publication is not approval to flash.

## Publication, errors and limitations

The source branch was published through authenticated GitHub Git-data APIs after both suites/builds/image validation passed, using existing validated blobs plus the new combined preset-manager blob. The remote tree matched local tree6dfeb48f exactly; readback confirmed14 changed files and sole parent lab-setup. API publication is not described as native git push. Local exact commit import was hash-verified. All original branches remain preserved. The report uses a new one-file branch based directly on lab-setup, with final exact content and branch verification; no report overwrite or generated artifact upload.

Errors: broad exploratory search output was truncated and focused reads followed; guessed FatFs/startup paths were corrected. An initial local commit import message-newline assumption failed its hash assertion before moving the branch; exact reconstruction succeeded. Original build/test execution used the verified staged tree; final pinned builds confirm the same source tree. Static-analysis warnings are genuine analysis results, not build failures. No test failure, merge conflict or unresolved architecture choice was bypassed. There is no claim of exhaustive static analysis or a complete whole-program stack proof.

Recommended next action: review this report and approve a separately scoped diagnostic/validation phase only after the memory, DMA and recovery gates are addressed. Keep the integration branch separate; no merge or hardware operation is authorized by this preflight.

## Appendix A — complete integrated diff

The following exact reviewed patch is source/tests only. Existing comments are quoted verbatim as evidence; no new non-English comments were authored.

```diff
diff --git a/Core/Src/Preset/presetManager.c b/Core/Src/Preset/presetManager.c
index 41ef096..bffe86e 100644
--- a/Core/Src/Preset/presetManager.c
+++ b/Core/Src/Preset/presetManager.c
@@ -299,6 +299,35 @@ FRESULT preset_LoadTmpKit()
 		return res;
 }
 
+// Read the existing length/kit tail without exposing partial or invalid lengths.
+static uint8_t preset_readPatternLengths(uint8_t patternNr, uint8_t dataVersion)
+{
+	if(patternNr >= NUM_PATTERN) return 0;
+	uint8_t lengths[NUM_TRACKS];
+	uint8_t kit = 0;
+	UINT bytesRead;
+	if(f_read((FIL*)&SDFile, lengths, sizeof(lengths), &bytesRead) != FR_OK) return 0;
+	if(dataVersion == 1 && bytesRead == 0)
+	{
+		// Untagged V1 files may have no length tail at all.
+		for(uint8_t trk=0; trk<NUM_TRACKS; trk++) lengths[trk] = 16;
+	}
+	else if(bytesRead != sizeof(lengths)) return 0;
+	for(uint8_t trk=0; trk<NUM_TRACKS; trk++)
+	{
+		if(lengths[trk] == 0) lengths[trk] = 16;
+		if(lengths[trk] > NUM_STEPS) return 0;
+	}
+	if(dataVersion > 1)
+	{
+		if(f_read((FIL*)&SDFile, &kit, sizeof(kit), &bytesRead) != FR_OK || bytesRead != sizeof(kit)) return 0;
+	}
+	for(uint8_t trk=0; trk<NUM_TRACKS; trk++)
+		seq_patternSet.length[patternNr][trk] = lengths[trk];
+	if(dataVersion > 1) seq_patternSet.kitNr[patternNr] = kit;
+	return 1;
+}
+
 FRESULT preset_LoadTmpPat()
 {
 
@@ -326,21 +355,22 @@ FRESULT preset_LoadTmpPat()
 				return res;
 		}
 
-		//read current preset nr
-		res = f_read((FIL*)&SDFile,(void*)&menu_playedPattern,1,&bytesRead);
-		if(bytesRead != 1)
+		// Validate the stored pattern index before publishing it to UI/playback state.
+		uint8_t loadedPattern;
+		res = f_read((FIL*)&SDFile,(void*)&loadedPattern,1,&bytesRead);
+		if(res != FR_OK || bytesRead != 1)
 		{
 			success = 0;
 			goto closeFile;
 		}
 
-		if(menu_playedPattern >= NUM_PATTERN)
+		if(loadedPattern >= NUM_PATTERN)
 		{
-			menu_playedPattern = 0;
 			success = 0;
 			goto closeFile;
 		}
 
+		menu_playedPattern = loadedPattern;
 		menu_shownPattern = menu_playedPattern;
 		seq_activePattern = menu_playedPattern;
 		seq_pendingPattern = menu_playedPattern;
@@ -358,7 +388,6 @@ FRESULT preset_LoadTmpPat()
 		// This means that loading old patterns with non-standard length, the length will be
 		// ignored and a standard length used. I think it's acceptable as long as users are aware of this.
 
-		uint8_t next=0;
 
 		// ---------- Load step data first
 
@@ -369,9 +398,7 @@ FRESULT preset_LoadTmpPat()
 		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
 		{
 
-			f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead);
-
-			if(bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
+			if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
 			{
 				success=0;
 				break; // file error, or we ran out of file (eof)
@@ -380,9 +407,7 @@ FRESULT preset_LoadTmpPat()
 			//Pattern V3 and up => flam added
 			//if(dataVersion > 2)
 			{
-				f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead);
-
-				if(bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
+				if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
 				{
 					success=0;
 					break; // file error, or we ran out of file (eof)
@@ -401,9 +426,7 @@ FRESULT preset_LoadTmpPat()
 			{
 				for(uint8_t bar=0;bar<NUM_BARS; bar++)
 				{
-					f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead);
-
-					if( bytesRead==sizeof(uint16_t)) {
+					if(f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead) == FR_OK && bytesRead==sizeof(uint16_t)) {
 						seq_patternSet.seq_mainSteps[bar][patternNr][trk] = mainStepData;
 					}
 					 else {
@@ -418,9 +441,7 @@ FRESULT preset_LoadTmpPat()
 		// ------------ shuffle settings
 		if(success) {
 			//load the shuffle settings
-			f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead);
-			//and update on cortex
-			if(bytesRead)
+			if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead) == FR_OK && bytesRead == sizeof(uint8_t))
 			{
 
 				//activeParameters[PAR_SHUFFLE] = seq_patternSet.shuffle[patternNr];
@@ -433,25 +454,7 @@ FRESULT preset_LoadTmpPat()
 			}
 		}
 
-		// ----------- Pattern/track lengths
-		// -- AS this might not exist in the saved data file, we still want success
-		if(success) {
-
-				for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
-				{
-					f_read((FIL*)&SDFile,(void*)&next,sizeof(uint8_t),&bytesRead);
-					seq_patternSet.length[patternNr][trk] = next;
-				}
-
-			success=1; // just to document that we don't consider this a failure, we just didn't read the bytes for this
-		}
-
-		//if(dataVersion > 1)
-		{
-			f_read((FIL*)&SDFile,(void*)&seq_patternSet.kitNr[patternNr],sizeof(uint8_t),&bytesRead);
-			if(bytesRead == 0) success = 0;
-
-		}
+		if(success) success = preset_readPatternLengths(patternNr, 3);
 
 		if( success == 0)
 		{
@@ -1259,6 +1262,14 @@ static uint8_t interpolate(uint8_t a, uint8_t b, uint8_t x)
 	//return ((a*255) + (b-a)*x)/255;
 }
 
+//----------------------------------------------------
+// These slots store MIDI transport state, not sound parameters.
+static uint8_t preset_isTransportParameter(uint8_t paramNr)
+{
+	return paramNr == PAR_NRPN_DATA_ENTRY_COARSE ||
+		paramNr == PAR_NRPN_FINE || paramNr == PAR_NRPN_COARSE ||
+		paramNr == PAR_BANK_CHANGE;
+}
 //----------------------------------------------------
 IN_ITCM_RAM void preset_sendDrumsetParametersImmediate()
 {
@@ -1305,6 +1316,11 @@ IN_ITCM_RAM void preset_sendDrumsetParametersImmediate()
 		{
 
 			uint8_t paramNumber = parameterOrder[i];
+			if(preset_isTransportParameter(paramNumber))
+			{
+				soundengine_process();
+				continue;
+			}
 
 			//const uint8_t morph = activeParameters[PAR_MORPH];
 
@@ -1699,6 +1715,7 @@ FRESULT preset_savePatternSD(uint8_t presetNr)
 // returns 1 on success
 static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 {
+	if(patternNr >= NUM_PATTERN) return 0;
 
 #if 0
 
@@ -1871,7 +1888,6 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 
 	UINT bytesRead;
 	uint8_t success=1; // start off succeeding
-	uint8_t next=0;
 
 	// ---------- Load step data first
 
@@ -1882,9 +1898,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 	for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
 	{
 
-		f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead);
-
-		if(bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
+		if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
 		{
 			success=0;
 			break; // file error, or we ran out of file (eof)
@@ -1893,9 +1907,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 		//Pattern V3 and up => flam added
 		if(dataVersion > 2)
 		{
-			f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead);
-
-			if(bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
+			if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
 			{
 				success=0;
 				break; // file error, or we ran out of file (eof)
@@ -1914,9 +1926,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 		{
 			for(uint8_t bar=0;bar<NUM_BARS; bar++)
 			{
-				f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead);
-
-				if( bytesRead==sizeof(uint16_t)) {
+				if(f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead) == FR_OK && bytesRead==sizeof(uint16_t)) {
 					seq_patternSet.seq_mainSteps[bar][patternNr][trk] = mainStepData;
 				}
 				 else {
@@ -1931,9 +1941,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 	// ------------ shuffle settings
 	if(success) {
 		//load the shuffle settings
-		f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead);
-		//and update on cortex
-		if(bytesRead)
+		if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead) == FR_OK && bytesRead == sizeof(uint8_t))
 		{
 
 			//activeParameters[PAR_SHUFFLE] = seq_patternSet.shuffle[patternNr];
@@ -1946,25 +1954,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 		}
 	}
 
-	// ----------- Pattern/track lengths
-	// -- AS this might not exist in the saved data file, we still want success
-	if(success) {
-
-			for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
-			{
-				f_read((FIL*)&SDFile,(void*)&next,sizeof(uint8_t),&bytesRead);
-				seq_patternSet.length[patternNr][trk] = next;
-			}
-
-		success=1; // just to document that we don't consider this a failure, we just didn't read the bytes for this
-	}
-
-	if(dataVersion > 1)
-	{
-		f_read((FIL*)&SDFile,(void*)&seq_patternSet.kitNr[patternNr],sizeof(uint8_t),&bytesRead);
-		if(bytesRead == 0) success = 0;
-
-	}
+	if(success) success = preset_readPatternLengths(patternNr, dataVersion);
 
 	return success;
 #endif
@@ -2019,6 +2009,7 @@ frontPanel_sendData(PRESET,PATTERN_LOAD,presetNr);
 //----------------------------------------------------
 FRESULT preset_loadPatternSD(uint8_t presetNr)
 {
+	if(presetNr >= NUM_PATTERN) return FR_INVALID_PARAMETER;
 #if USE_SD_CARD		
 	//filename in 8.3  format
 	uint8_t fileVersion = 1;
@@ -2045,11 +2036,12 @@ FRESULT preset_loadPatternSD(uint8_t presetNr)
 	
 	//read the preset name
 	res = f_read((FIL*)&SDFile,(void*)savefileNames[presetNr][PATTERN_NAMES],8,&bytesRead);
-	if(bytesRead != 8)
+	if(res != FR_OK || bytesRead != 8)
 		goto closeFile;
 	
 	char buf[7];
 	res = f_read((FIL*)&SDFile,buf,6,&bytesRead);
+	if(res != FR_OK || bytesRead != 6) goto closeFile;
 	//zero termination
 	buf[6]=0;
 	fileVersion = atoi(&buf[4]);
@@ -2132,6 +2124,14 @@ void preset_morph_tick()
 	if(morph_state)
 	{
 
+		// Consume the reserved slot's tick without executing a MIDI command.
+		// All transport slots precede the final sound parameter.
+		if(preset_isTransportParameter(morph_index))
+		{
+			morph_index++;
+			return;
+		}
+
 		uint8_t morph;
 
 		//Do not morph audio routing
diff --git a/Core/Src/Soundengine/MIDI/CortexParser.c b/Core/Src/Soundengine/MIDI/CortexParser.c
index 5a06962..269b617 100644
--- a/Core/Src/Soundengine/MIDI/CortexParser.c
+++ b/Core/Src/Soundengine/MIDI/CortexParser.c
@@ -255,10 +255,20 @@ static void frontParser_handleSysexData(unsigned char data)
 	case SYSEX_RECEIVE_PAT_LEN_DATA:
 		// same as above but we are receiving length data for each pattern
 		{
+			// Saturate at the end of the length stream; never wrap the receive counter.
+			if(frontParser_sysexSeqStepNr >= NUM_PATTERN * NUM_TRACKS) break;
 			//calculate the step pattern and track indices
-			const uint8_t currentPattern	= frontParser_sysexSeqStepNr / 7;
-			const uint8_t currentTrack  	= frontParser_sysexSeqStepNr - currentPattern*7;
+			const uint8_t currentPattern	= frontParser_sysexSeqStepNr / NUM_TRACKS;
+			const uint8_t currentTrack  	= frontParser_sysexSeqStepNr - currentPattern*NUM_TRACKS;
+
 
+			if(data == 0) data = 16; // legacy length encoding
+			if(data > NUM_STEPS)
+			{
+				frontParser_sysexSeqStepNr++;
+				frontParser_rxCnt = 0;
+				break;
+			}
 
 /*
 			if( (currentPattern == seq_activePattern) && seq_isRunning() )
diff --git a/Core/Src/Soundengine/MIDI/MidiParser.c b/Core/Src/Soundengine/MIDI/MidiParser.c
index bb68cb4..d119e9f 100644
--- a/Core/Src/Soundengine/MIDI/MidiParser.c
+++ b/Core/Src/Soundengine/MIDI/MidiParser.c
@@ -133,6 +133,9 @@ float midiParser_calcDetune(uint8_t value)
 //-----------------------------------------------------------
 static void midiParser_nrpnHandler(uint16_t value)
 {
+	// Reject unsupported 14-bit selections before narrowing to a MIDI byte.
+	if(midiParser_activeNrpnNumber > UINT8_MAX) return;
+
 	MidiMsg msg2;
 	msg2.status = MIDI_CC2;
 	msg2.data1 = midiParser_activeNrpnNumber;
@@ -164,6 +167,10 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 	if((msg.status &0xF0) == MIDI_CC)
 	{
 
+		// Bank changes are forwarded to the UI by parseMidiMessage(). They have
+		// no sound parameter and must not enter the cache or modulation path.
+		if(msg.data1 == CC_BANK_CHANGE || msg.data1 > 127) return;
+
 		const uint16_t paramNr = msg.data1-1;
 		if(updateOriginalValue && !isFloatModulator) {
 			midiParser_originalCcValues[paramNr+1] = value.itg;
@@ -176,16 +183,6 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 
 			break;
 
-		case CC_BANK_CHANGE:
-			// bank change (coarse) selects kit (sound)
-			/*
-			 * already send in parseMidiMessage()
-			uart_sendFrontpanelByte(MIDI_CC);
-			uart_sendFrontpanelByte(CC_BANK_CHANGE);
-			uart_sendFrontpanelByte(value.itg);
-			*/
-			break;
-
 		case NRPN_DATA_ENTRY_COARSE:
 			midiParser_nrpnHandler(value.itg);
 			return;
@@ -906,8 +903,13 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 	else //MIDI_CC2
 	{
 		const uint16_t paramNr = msg.data1 + 1 + 127;
+		// External NRPN mute commands are not sound parameters and have no
+		// cache entry or modulation target. Preserve their command handling.
+		const uint8_t isMuteCommand = msg.data1 >= CC2_MUTE_1 && msg.data1 <= CC2_MUTE_7;
+		if(!isMuteCommand && (paramNr >= END_OF_SOUND_PARAMETERS ||
+			paramNr >= sizeof(midiParser_originalCcValues))) return;
 
-		if(updateOriginalValue && !isFloatModulator) {
+		if(updateOriginalValue && !isFloatModulator && !isMuteCommand) {
 			midiParser_originalCcValues[paramNr] = value.itg;
 		}
 		switch(msg.data1)
@@ -1433,7 +1435,7 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 			default:
 				break;
 		}
-		if(!isFloatModulator)
+		if(!isFloatModulator && !isMuteCommand)
 		{
 			modNode_originalValueChanged(paramNr);
 		}
diff --git a/Core/Src/Soundengine/Sequencer/EuklidGenerator.c b/Core/Src/Soundengine/Sequencer/EuklidGenerator.c
index a9c2f2a..e5fcd00 100644
--- a/Core/Src/Soundengine/Sequencer/EuklidGenerator.c
+++ b/Core/Src/Soundengine/Sequencer/EuklidGenerator.c
@@ -52,6 +52,8 @@ void euklid_init()
  */
 static inline uint8_t getObjSize(uint16_t group)
 {
+	// The C builtin underlying __CLZ is undefined for zero.
+	if(group == 0) return 1;
 	uint8_t ret = 32 - __CLZ((uint32_t)group);
 	if(ret==0) return 1;
 	return ret;
@@ -141,6 +143,8 @@ void euklid_calcRecursive(uint8_t length, uint8_t steps, uint8_t iteration, uint
 //-----------------------------------------------------
 void euklid_generate(uint8_t trackNr, uint8_t patternNr)
 {
+	if(trackNr >= NUM_TRACKS || patternNr >= NUM_PATTERN) return;
+	if(euklid_length[trackNr] < 1 || euklid_length[trackNr] > 16) return;
 	uint8_t length,steps,rotation;
 	length = euklid_length[trackNr];
 	steps = euklid_steps[trackNr];
@@ -168,24 +172,28 @@ void euklid_generate(uint8_t trackNr, uint8_t patternNr)
 //-----------------------------------------------------
 uint8_t euklid_getLength(uint8_t trackNr)
 {
+	if(trackNr >= NUM_TRACKS) return 16;
 	return euklid_length[trackNr];
 }
 //-----------------------------------------------------
 uint8_t euklid_getSteps(uint8_t trackNr)
 {
+	if(trackNr >= NUM_TRACKS) return 1;
 	return euklid_steps[trackNr];
 };
 
 //-----------------------------------------------------
 void euklid_setLength(uint8_t trackNr, uint8_t value)
 {
+	if(trackNr >= NUM_TRACKS) return;
 	if(value<=0)value=1;
-	//else if(value>16)value=16;
+	else if(value>16)value=16;
 	euklid_length[trackNr] = value;
 }
 //-----------------------------------------------------
 void euklid_setSteps(uint8_t trackNr, uint8_t value, uint8_t patternNr)
 {
+	if(trackNr >= NUM_TRACKS || patternNr >= NUM_PATTERN) return;
 	if(value<=0)value=1;
 
 	if(value>euklid_length[trackNr]) value= euklid_length[trackNr];
@@ -196,11 +204,13 @@ void euklid_setSteps(uint8_t trackNr, uint8_t value, uint8_t patternNr)
 //-----------------------------------------------------
 uint8_t euklid_getRotation(uint8_t trackNr)
 {
+	if(trackNr >= NUM_TRACKS) return 0;
 	return euklid_rotation[trackNr];
 }
 //-----------------------------------------------------
 void euklid_setRotation(uint8_t trackNr, uint8_t value, uint8_t patternNr)
 {
+	if(trackNr >= NUM_TRACKS || patternNr >= NUM_PATTERN) return;
 	if(value>euklid_length[trackNr]) value = euklid_rotation[trackNr];
 
 	euklid_rotation[trackNr] = value;
@@ -209,6 +219,7 @@ void euklid_setRotation(uint8_t trackNr, uint8_t value, uint8_t patternNr)
 //-----------------------------------------------------
 void euklid_rotatePattern(uint8_t length, uint8_t amount)
 {
+	if(length < 1 || length > 16) return;
 	if( amount==0 || length==1 || amount%length==0  ) return; //Nothing to be done
 
 	amount = amount%length;
@@ -220,6 +231,8 @@ void euklid_rotatePattern(uint8_t length, uint8_t amount)
 //-----------------------------------------------------
 void euklid_transferPattern(uint8_t trackNr, uint8_t patternNr)
 {
+	if(trackNr >= NUM_TRACKS || patternNr >= NUM_PATTERN) return;
+	if(euklid_length[trackNr] < 1 || euklid_length[trackNr] > 16) return;
 	uint8_t len=euklid_length[trackNr];
 	for(int i=0; i<NUM_BARS; i++)
 	{
@@ -230,10 +243,7 @@ void euklid_transferPattern(uint8_t trackNr, uint8_t patternNr)
 #endif
 
 	}
-// **PATROT - pattern end is now stored differently
-
-	if(len == 16)
-		len=0;
+	// Track lengths are literal 1..64 values, including length 16.
 #if USE_PATTERN_EDIT_BUFFER
 	patternBackupBuffer.length[trackNr]=len;
 #else
diff --git a/Core/Src/Soundengine/Sequencer/sequencer.c b/Core/Src/Soundengine/Sequencer/sequencer.c
index 9b51d96..be480d5 100644
--- a/Core/Src/Soundengine/Sequencer/sequencer.c
+++ b/Core/Src/Soundengine/Sequencer/sequencer.c
@@ -273,11 +273,9 @@ void seq_setShuffle(float shuffle)
 //------------------------------------------------------------------------------
 void seq_setTrackLength(uint8_t trackNr, uint8_t length)
 {
-	//set new end marker
-	/*
-	if(length == 16)
-		length=0;
-	*/
+	if(trackNr >= NUM_TRACKS || frontParser_shownPattern >= NUM_PATTERN) return;
+	if(length == 0) length = 16; // legacy input encoding
+	if(length > NUM_STEPS) return;
 	// **PATROT this was changed from setting length on seq_activePattern to shown (or edited) pattern
 
 #if USE_PATTERN_EDIT_BUFFER
@@ -292,6 +290,7 @@ void seq_setTrackLength(uint8_t trackNr, uint8_t length)
 //------------------------------------------------------------------------------
 uint8_t seq_getTrackLength(uint8_t trackNr)
 {
+	if(trackNr >= NUM_TRACKS || frontParser_shownPattern >= NUM_PATTERN) return 16;
 	// **PATROT this was changed from getting seq_activePattern to shown (or edited) pattern
 #if USE_PATTERN_EDIT_BUFFER
 	uint8_t r=patternBackupBuffer.length[trackNr];
diff --git a/tests/bug001/README.md b/tests/bug001/README.md
new file mode 100644
index 0000000..8f22d91
--- /dev/null
+++ b/tests/bug001/README.md
@@ -0,0 +1,26 @@
+# Bug 001 regressions
+
+Run `python3 tests/bug001/run.py` from the repository root. Requires a native
+GNU-compatible C compiler with AddressSanitizer/UndefinedBehaviorSanitizer.
+Temporary binaries and generated files are removed after the run.
+
+The parser test compiles the complete production `MidiParser.c`. It observes
+Drive writes, modulation notifications, mute commands and external bank-change
+forwarding. Unrelated DSP/hardware calls abort through link stubs. The Drive stub
+records the normalized input; it does not model the filter or its Drive curve.
+
+The sweep test compiles the unmodified production interpolation, reserved-slot
+predicate, immediate kit sweep, Morph tick and Morph start functions extracted
+by the runner, plus the real parameter-order array. The MIDI sink records every
+dispatch and rejects transport commands. This avoids bringing SD-card/hardware
+code into the host process without adding test hooks to firmware.
+
+Coverage includes all 256 Morph positions with identical kits, nonzero reserved
+slots, existing restart/tick behavior, kit-load audio service cadence, CC0 with
+all cache/modulator flags, valid CC/CC2/NRPN mapping, all seven external NRPN
+mutes, invalid/high NRPN selectors, and every out-of-range CC2 byte with all
+cache/modulator flags. Sanitizers check exercised memory accesses.
+Leak detection is disabled by default because LeakSanitizer cannot run under
+ptrace; AddressSanitizer and UndefinedBehaviorSanitizer remain enabled.
+
+These are control-path tests, not an audio simulation or on-device validation.
diff --git a/tests/bug001/run.py b/tests/bug001/run.py
new file mode 100644
index 0000000..d91117d
--- /dev/null
+++ b/tests/bug001/run.py
@@ -0,0 +1,77 @@
+#!/usr/bin/env python3
+# SPDX-License-Identifier: GPL-3.0-only
+"""Run focused production-code regressions using a native C compiler."""
+import os
+from pathlib import Path
+import re
+import subprocess
+import tempfile
+
+ROOT = Path(__file__).resolve().parents[2]
+HERE = Path(__file__).resolve().parent
+
+
+def function(source, name):
+    # Mask comments/strings before counting braces; return the original text.
+    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
+                    lambda m: " " * len(m[0]), source, flags=re.S)
+    match = re.search(r"^(?:IN_ITCM_RAM )?(?:static )?(?:uint8_t|void) "
+                      + name + r"\([^;]*?\)\s*\{", masked, re.M)
+    if not match:
+        raise RuntimeError(f"Production function not found: {name}")
+    depth = 1
+    end = match.end()
+    while depth:
+        depth += (masked[end] == "{") - (masked[end] == "}")
+        end += 1
+    return source[match.start():end]
+
+
+def run(*args):
+    env = os.environ.copy()
+    # LeakSanitizer cannot run in ptraced execution environments. These tests
+    # target bounds/UB, not allocation; retain ASan and UBSan instrumentation.
+    env.setdefault("ASAN_OPTIONS", "detect_leaks=0")
+    subprocess.run(args, cwd=ROOT, env=env, check=True)
+
+
+def main():
+    includes = [".", "Core/Inc", "Drivers/CMSIS/Include",
+                "Drivers/CMSIS/Device/ST/STM32F7xx/Include",
+                "Drivers/STM32F7xx_HAL_Driver/Inc",
+                "Middlewares/Third_Party/FatFs/src",
+                "Middlewares/JS/STM32_USB_Device_Library/Core/Inc",
+                "Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc",
+                "USB_DEVICE_MIDI/Target"]
+    # ARM-specific inline DSP intrinsics are not executed by these tests.
+    # _Float16 supplies the host spelling of the firmware's half-float type.
+    flags = [os.environ.get("CC", "cc"), "-std=gnu11", "-O1", "-g",
+             "-DSTM32F765xx", "-DUSE_HAL_DRIVER", "-D__fp16=_Float16",
+             "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast",
+             "-Wno-implicit-function-declaration", "-Wno-unused-function",
+             "-ffunction-sections", "-fdata-sections", "-fno-pie", "-no-pie",
+             "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
+             "-fno-omit-frame-pointer"]
+    flags += ["-I" + str(ROOT / p) for p in includes]
+    with tempfile.TemporaryDirectory(prefix="lxr-bug001-") as tmp:
+        tmp = Path(tmp)
+        parser = tmp / "test_parser"
+        run(*flags, str(HERE / "test_parser.c"), str(HERE / "trap_stubs.c"),
+            "-Wl,--gc-sections", "-lm", "-o", str(parser))
+        run(str(parser))
+
+        source = (ROOT / "Core/Src/Preset/presetManager.c").read_text()
+        names = ["interpolate", "preset_isTransportParameter",
+                 "preset_sendDrumsetParametersImmediate", "preset_morph_tick",
+                 "preset_morph"]
+        (tmp / "sweep_functions.inc").write_text(
+            "\n\n".join(function(source, name) for name in names))
+        sweeps = tmp / "test_sweeps"
+        run(*flags, "-I" + str(tmp), str(HERE / "test_sweeps.c"),
+            str(ROOT / "Core/Src/Parameters.c"), "-Wl,--gc-sections",
+            "-o", str(sweeps))
+        run(str(sweeps))
+
+
+if __name__ == "__main__":
+    main()
diff --git a/tests/bug001/test_parser.c b/tests/bug001/test_parser.c
new file mode 100644
index 0000000..3ca4dbd
--- /dev/null
+++ b/tests/bug001/test_parser.c
@@ -0,0 +1,157 @@
+// SPDX-License-Identifier: GPL-3.0-only
+// Compile the complete production parser, including its static NRPN state.
+#include <assert.h>
+#include <stdio.h>
+#include <string.h>
+#include "Core/Src/Soundengine/MIDI/MidiParser.c"
+
+DrumVoice voiceArray[NUM_DRUM_VOICES];
+SnareVoice snareVoice;
+HiHatVoice hatVoice;
+CymbalVoice cymbalVoice;
+FxBus fxBus;
+ModulationNode velocityModulators[6];
+uint8_t activeParameters[NUM_PARAMS];
+uint8_t mixer_audioRouting[6];
+float mixer_decimation_rate[7];
+uint8_t frontParser_activeTrack, menu_activePage;
+PatternSet seq_patternSet;
+volatile uint32_t systick_ticks;
+
+static unsigned notifications, drive_writes, mute_writes, forwarded;
+static uint8_t mute_voice, mute_value, forwarded_bytes[3];
+static uint16_t last_notification;
+
+void modNode_originalValueChanged(uint16_t idx)
+{
+    assert(idx < END_OF_SOUND_PARAMETERS);
+    last_notification = idx;
+    notifications++;
+}
+void SVF_setDrive(ResonantFilter *filter, float drive)
+{
+    filter->drive = drive;
+    drive_writes++;
+}
+void seq_setMute(uint8_t voice, uint8_t muted)
+{
+    assert(voice < 7);
+    mute_voice = voice;
+    mute_value = muted;
+    mute_writes++;
+}
+void seq_recordAutomation(uint8_t track, uint8_t param, uint8_t value)
+{
+    (void)track; (void)param; (void)value;
+}
+void uart_sendFrontpanelByte(uint8_t value)
+{
+    assert(forwarded < sizeof(forwarded_bytes));
+    forwarded_bytes[forwarded++] = value;
+}
+
+static void cc(uint8_t number, uint8_t value)
+{
+    MidiMsg msg = {.status = MIDI_CC, .data1 = number, .data2 = value};
+    midiParser_ccHandler(msg, 1, 0, 0.f);
+}
+static void nrpn(uint16_t number, uint8_t value)
+{
+    cc(NRPN_FINE, number & 127);
+    cc(NRPN_COARSE, (number >> 7) & 127);
+    cc(NRPN_DATA_ENTRY_COARSE, value);
+}
+static void external_cc(uint8_t number, uint8_t value)
+{
+    MidiMsg msg = {.status = MIDI_CC, .data1 = number, .data2 = value};
+    forwarded = 0;
+    midiParser_parseMidiMessage(msg);
+    assert(forwarded == 3 && forwarded_bytes[0] == MIDI_CC);
+    assert(forwarded_bytes[1] == number && forwarded_bytes[2] == value);
+}
+
+int main(void)
+{
+    uint8_t cache[sizeof(midiParser_originalCcValues)];
+    memset(midiParser_originalCcValues, 0xa5, sizeof(cache));
+    memcpy(cache, midiParser_originalCcValues, sizeof(cache));
+
+    // CC0 must not touch the cache, notify modulators, or change the selector.
+    midiParser_activeNrpnNumber = 3;
+    for(unsigned flags = 0; flags < 4; flags++) {
+        MidiMsg msg = {.status = MIDI_CC, .data1 = 0, .data2 = 127};
+        midiParser_ccHandler(msg, flags & 1, (flags >> 1) & 1, 0.5f);
+    }
+    assert(!notifications && !memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
+    assert(midiParser_activeNrpnNumber == 3);
+
+    // Bank change still reaches the UI through the external MIDI entry point.
+    MidiMsg bank = {.status = MIDI_CC, .data1 = CC_BANK_CHANGE, .data2 = 17};
+    midiParser_parseMidiMessage(bank);
+    assert(forwarded == 3 && forwarded_bytes[0] == MIDI_CC);
+    assert(forwarded_bytes[1] == CC_BANK_CHANGE && forwarded_bytes[2] == 17);
+    assert(!notifications && !memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
+
+    // Valid low CC, CC127 and CC2 Drive retain their cache/notification mapping.
+    cc(CC_MOD_WHEEL, 23);
+    assert(midiParser_originalCcValues[1] == 23 && last_notification == 0);
+    cc(AMOUNT_LFO6, 31);
+    assert(midiParser_originalCcValues[127] == 31 && last_notification == 126);
+    nrpn(CC2_FILTER_DRIVE_1, 127);
+    assert(drive_writes == 1 && voiceArray[0].filter.drive == 1.f);
+    assert(midiParser_originalCcValues[128] == 127 && last_notification == 128);
+    nrpn(CC2_FILTER_DRIVE_3, 64);
+    assert(drive_writes == 2 && voiceArray[2].filter.drive == 64 / 127.f);
+    nrpn(END_OF_SOUND_PARAMETERS - 129, 42);
+    assert(last_notification == END_OF_SOUND_PARAMETERS - 1);
+    assert(midiParser_originalCcValues[END_OF_SOUND_PARAMETERS - 1] == 42);
+
+    // Valid NRPN also works through the external MIDI routing/channel path.
+    external_cc(NRPN_FINE, CC2_FILTER_DRIVE_2);
+    external_cc(NRPN_COARSE, 0);
+    external_cc(NRPN_DATA_ENTRY_COARSE, 31);
+    assert(drive_writes == 3 && voiceArray[1].filter.drive == 31 / 127.f);
+
+    // A high 14-bit selector must never alias a low parameter or mute command.
+    const uint16_t invalid[] = {119, 126, 127, 128, 199, 207, 255, 256, 258, 456, 16383};
+    for(unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
+        cc(NRPN_FINE, invalid[i] & 127);
+        cc(NRPN_COARSE, invalid[i] >> 7);
+        unsigned before = notifications;
+        cc(NRPN_DATA_ENTRY_COARSE, 0);
+        assert(notifications == before && drive_writes == 3 && !mute_writes);
+    }
+
+    // Special external NRPN mutes work, without cache access or notification.
+    for(unsigned voice = 0; voice < 7; voice++) {
+        cc(NRPN_FINE, (CC2_MUTE_1 + voice) & 127);
+        cc(NRPN_COARSE, (CC2_MUTE_1 + voice) >> 7);
+        unsigned before = notifications;
+        cc(NRPN_DATA_ENTRY_COARSE, 127);
+        assert(notifications == before && mute_voice == voice && mute_value == 1);
+        cc(NRPN_DATA_ENTRY_COARSE, 0);
+        assert(notifications == before && mute_value == 0);
+    }
+    assert(mute_writes == 14);
+    external_cc(NRPN_FINE, CC2_MUTE_7 & 127);
+    external_cc(NRPN_COARSE, CC2_MUTE_7 >> 7);
+    unsigned before = notifications;
+    external_cc(NRPN_DATA_ENTRY_COARSE, 127);
+    assert(mute_writes == 15 && mute_voice == 6 && mute_value == 1);
+    assert(notifications == before);
+
+    // Exhaust every CC2 byte for all cache/modulator flag combinations.
+    for(unsigned flags = 0; flags < 4; flags++) {
+        for(unsigned number = 0; number < 256; number++) {
+            unsigned param = number + 128;
+            if(param < END_OF_SOUND_PARAMETERS) continue; // normal DSP covered by firmware builds
+            MidiMsg msg = {.status = MIDI_CC2, .data1 = number, .data2 = 0};
+            unsigned before = notifications;
+            memcpy(cache, midiParser_originalCcValues, sizeof(cache));
+            midiParser_ccHandler(msg, flags & 1, (flags >> 1) & 1, 0.f);
+            assert(notifications == before);
+            assert(!memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
+        }
+    }
+    puts("PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds");
+}
diff --git a/tests/bug001/test_sweeps.c b/tests/bug001/test_sweeps.c
new file mode 100644
index 0000000..b06a004
--- /dev/null
+++ b/tests/bug001/test_sweeps.c
@@ -0,0 +1,89 @@
+// SPDX-License-Identifier: GPL-3.0-only
+#include <assert.h>
+#include <stdio.h>
+#include <string.h>
+#include "menu/menu.h"
+#include "frontPanelParser_ui.h"
+#include "Soundengine/MIDI/MidiParser.h"
+
+uint8_t activeParameters[NUM_PARAMS];
+uint8_t mainParameters[NUM_KIT_SLOTS][END_OF_SOUND_PARAMETERS];
+uint8_t *morphParameters = mainParameters[0];
+const ModTarg modTargets[] = {{0, 0}};
+static uint8_t morph_state, morph_index;
+static unsigned seen[END_OF_SOUND_PARAMETERS], dispatched, audio_calls;
+
+void midiParser_ccHandler(MidiMsg msg, uint8_t update, uint8_t floating, float v)
+{
+    (void)v;
+    unsigned param;
+    assert(update == 1 && floating == 0);
+    if(msg.status == MIDI_CC) {
+        assert(msg.data1 != CC_BANK_CHANGE);
+        assert(msg.data1 != NRPN_DATA_ENTRY_COARSE);
+        assert(msg.data1 != NRPN_FINE && msg.data1 != NRPN_COARSE);
+        param = msg.data1 - 1;
+    } else {
+        assert(msg.status == CC_2);
+        param = msg.data1 + 128;
+    }
+    assert(param < END_OF_SOUND_PARAMETERS);
+    assert(msg.data2 == activeParameters[param]); // identical kits must stay identical
+    seen[param]++;
+    dispatched++;
+}
+void frontPanel_sendData(uint8_t status, uint8_t data1, uint8_t data2)
+{
+    (void)data1; (void)data2;
+    assert(status == CC_VELO_TARGET || status == CC_LFO_TARGET);
+}
+void soundengine_process(void) { audio_calls++; }
+
+// The runner inserts unmodified production function bodies here.
+#include "sweep_functions.inc"
+
+static void check_sweep(void)
+{
+    assert(dispatched == END_OF_SOUND_PARAMETERS - 4);
+    for(unsigned i = 0; i < END_OF_SOUND_PARAMETERS; i++)
+        assert(seen[i] == (preset_isTransportParameter(i) ? 0u : 1u));
+    memset(seen, 0, sizeof(seen));
+    dispatched = 0;
+}
+
+int main(void)
+{
+    // Nonzero transport slots ensure their values cannot silently act as commands.
+    memset(activeParameters, 73, END_OF_SOUND_PARAMETERS);
+    for(unsigned i = 0; i < 6; i++) {
+        activeParameters[PAR_VEL_DEST_1 + i] = 0;
+        activeParameters[PAR_TARGET_LFO1 + i] = 0;
+        activeParameters[PAR_VOICE_LFO1 + i] = 1;
+    }
+    memcpy(morphParameters, activeParameters, END_OF_SOUND_PARAMETERS);
+    for(unsigned morph = 0; morph < 256; morph++) {
+        activeParameters[PAR_MORPH] = morph;
+        preset_morph(morph);
+        for(unsigned tick = 0; tick < END_OF_SOUND_PARAMETERS; tick++) {
+            assert(morph_state && morph_index == tick);
+            preset_morph_tick();
+        }
+        assert(!morph_state);
+        preset_morph_tick(); // idle must not dispatch
+        check_sweep();
+    }
+    // Repeated events retain the existing restart behavior and final convergence.
+    for(unsigned event = 0; event < 16; event++) {
+        preset_morph(255);
+        for(unsigned tick = 0; tick < 110; tick++) preset_morph_tick();
+    }
+    memset(seen, 0, sizeof(seen)); dispatched = 0;
+    preset_morph(255);
+    while(morph_state) preset_morph_tick();
+    check_sweep();
+
+    preset_sendDrumsetParametersImmediate();
+    check_sweep();
+    assert(audio_calls == END_OF_SOUND_PARAMETERS);
+    puts("PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence");
+}
diff --git a/tests/bug001/trap_stubs.c b/tests/bug001/trap_stubs.c
new file mode 100644
index 0000000..300a072
--- /dev/null
+++ b/tests/bug001/trap_stubs.c
@@ -0,0 +1,48 @@
+// SPDX-License-Identifier: GPL-3.0-only
+// Unrelated firmware dependencies needed to link the complete production parser.
+// These deliberately have no production headers or ABI assumptions: they must
+// never be called. An unexpected path aborts instead of silently passing.
+#include <stdio.h>
+#include <stdlib.h>
+#define TRAP(name) void name(void) { fputs("Unexpected call: " #name "\n", stderr); abort(); }
+TRAP(Compressor_setRatio)
+TRAP(Cymbal_setPan)
+TRAP(DecayEg_setDecay)
+TRAP(DecayEg_setSlope)
+TRAP(FxBus_prepareFxChange)
+TRAP(FxBus_setParameter)
+TRAP(FxBus_updateParameter)
+TRAP(HiHat_setPan)
+TRAP(Ringmod_setWave)
+TRAP(SVF_directSetFilterValue)
+TRAP(SVF_setReso)
+TRAP(Snare_setPan)
+TRAP(lfo_setFreq)
+TRAP(lfo_setSync)
+TRAP(menu_getActiveVoice)
+TRAP(menu_getSubPage)
+TRAP(menu_getViewedPattern)
+TRAP(menu_repaint)
+TRAP(menu_switchSubPage)
+TRAP(osc_recalcFreq)
+TRAP(seq_addNote)
+TRAP(seq_getExtSync)
+TRAP(seq_isRunning)
+TRAP(seq_isTrackMuted)
+TRAP(seq_midiNoteOff)
+TRAP(seq_sendMidiNoteOn)
+TRAP(seq_setNextPattern)
+TRAP(seq_setRunning)
+TRAP(seq_sync)
+TRAP(setDistortionShape)
+TRAP(setPan)
+TRAP(slopeEg2_calcDecay)
+TRAP(slopeEg2_setAttack)
+TRAP(slopeEg2_setDecay)
+TRAP(slopeEg2_setSlope)
+TRAP(sync_midiResetPosition)
+TRAP(sync_midiStartStop)
+TRAP(transient_setWaveform)
+TRAP(uart_sendMidi)
+TRAP(usb_sendMidi)
+TRAP(voiceControl_noteOn)
diff --git a/tests/euclidean/README.md b/tests/euclidean/README.md
new file mode 100644
index 0000000..4656954
--- /dev/null
+++ b/tests/euclidean/README.md
@@ -0,0 +1,60 @@
+# Euclidean length regression
+
+Run from the repository root:
+
+```sh
+python3 tests/euclidean/run.py
+```
+
+Requires Python 3 and a native GCC-compatible C compiler with ASan/UBSan (`CC`
+selects the compiler). No firmware or hardware executes. Temporary build outputs
+are automatically removed outside the repository. The candidate generator uses
+the original, unshimmed CMSIS intrinsic and its production zero guard.
+
+The fixture extracts original function bodies at runtime with source line
+annotations. HAL, audio output, display/routing and FatFs have bounded host seams.
+The normal named-file and autosave wrappers execute against an in-memory byte
+stream. No SD card, target interrupts or full clock/song engine is simulated.
+
+The six groups cover:
+
+- 2,176 exact rhythm masks for lengths 1–16, pulses 1–length, rotation inputs
+  0–15 (including the original retained-rotation behavior for rejected inputs).
+  The pinned SHA-256 oracle comes from `lab-setup` commit
+  `5829398cea5d337200393757e39a37d4d8148256`, with ARM CLZ(0)=32 emulation only
+  for that old baseline. To reproduce it where the base Git object is available,
+  run `python3 tests/euclidean/run.py --record-baseline`. Normal tests do not need
+  Git history or networking. Review the oracle before changing it.
+- All byte-valued track/pattern/length boundary inputs, literal lengths 1–64,
+  legacy zero decoding, and invalid setter rejection.
+- Playback next/previous wrap arithmetic across all lengths, flam 0–5, all
+  16 encoded shifts, both boundary steps and all 24 sub-ticks; recording at
+  every step before/after half-step quantization; mixed longest-track selection.
+- Full named PatV03 and temporary pattern save/load/save, including legacy zero,
+  all lengths 1–64, payload/mask preservation, exact bytes and layout. Named
+  file size stays 3663 with length offset 3655; tmp size stays 3650, offset 3642.
+- Every current-format byte truncation, invalid lengths 65–255 on every track,
+  I/O errors with full-looking counts, invalid temporary pattern prefixes, and
+  synthetic V1/V2 length tails. The loader installs no partial length/kit tail.
+  A wholly absent V1 length tail defaults to 16; partial tails fail. V2/V3/tmp
+  require all seven lengths and the kit byte. Real historical files still need
+  separate compatibility validation.
+- All byte-valued internal SysEx lengths across 448 legal slots, extra bytes,
+  every counter value 448–65535, and mode restart. Invalid length values consume
+  their slot without writing; counters stop advancing at the end of the stream.
+
+After building Release and Debug, validate the local images without uploading:
+
+```sh
+python3 tests/euclidean/check_images.py build/release build/debug
+```
+
+Use `--objdump /path/to/arm-none-eabi-objdump` when absent from PATH. This checks
+header/magic, payload length, checksum, raw-bin identity, vectors, allocated ELF
+section bounds, load addresses and minimum stack space. Linker checks remain
+active. Static memory checks do not establish worst-case runtime stack use or
+real-time audio performance. Do not flash or publish binaries through these tests.
+
+Both scripts stop on any unexpected compiler, sanitizer, assertion or image error.
+The host fixture is source regression coverage, not an equivalence claim for
+public official v1.71 or private support firmware.
diff --git a/tests/euclidean/check_images.py b/tests/euclidean/check_images.py
new file mode 100644
index 0000000..c971b0a
--- /dev/null
+++ b/tests/euclidean/check_images.py
@@ -0,0 +1,62 @@
+#!/usr/bin/env python3
+"""Read-only desktop update-image and ELF memory validation; never flash/upload."""
+import argparse
+import hashlib
+import json
+from pathlib import Path
+import re
+import struct
+import subprocess
+
+REGIONS = {'ITCM_RAM': (0, 16*1024), 'DTCM': (0x20000000, 128*1024),
+           'RAM': (0x20020000, 384*1024), 'FLASH': (0x08008000, 992*1024)}
+
+def check(directory, objdump):
+    image = (directory / 'LXR_V2_V1.7.img').read_bytes()
+    binary = (directory / 'LXR_V2_STM32F765.bin').read_bytes()
+    elf = directory / 'LXR_V2_STM32F765.elf'
+    magic, length, checksum, padding = struct.unpack('<8sIB3s', image[:16])
+    payload = image[16:]
+    assert magic == b'LXRV2IMG' and padding == b'\0\0\0'
+    assert length == len(payload) == len(binary) and payload == binary
+    assert len(image) == length+16 and 0 < length < 0x080fffff-0x08008000
+    assert checksum == 0xff-(sum(payload) & 0xff)
+    msp, reset = struct.unpack('<II', payload[:8])
+    assert 0x20020000 < msp <= 0x20080000 and msp % 8 == 0
+    assert reset & 1 and 0x08008000 <= (reset & ~1) < 0x08008000+length
+    listing = subprocess.run([objdump, '-h', str(elf)], check=True,
+                             capture_output=True, text=True).stdout
+    high = {name: 0 for name in REGIONS}
+    sections = []
+    for match in re.finditer(r'^\s*\d+\s+(\S+)\s+([\da-f]+)\s+([\da-f]+)\s+([\da-f]+)[^\n]*\n([^\n]*)', listing, re.M):
+        name, size, vma, lma, flags = match.groups()
+        size, vma, lma = (int(x,16) for x in (size,vma,lma))
+        if not size or 'ALLOC' not in flags:
+            continue
+        regions = [(label,base,capacity) for label,(base,capacity) in REGIONS.items()
+                   if base <= vma and vma+size <= base+capacity]
+        assert len(regions)==1, f'{name} outside memory regions'
+        label,base,capacity = regions[0]
+        high[label] = max(high[label],vma+size-base)
+        if 'LOAD' in flags:
+            assert 0x08008000 <= lma and lma+size <= 0x08008000+length
+        sections.append(name)
+    assert {'.isr_vector','.text','.data','.bss','.itcm_text','.dtcm','.dtcmz'} <= set(sections)
+    assert high['DTCM']+0x400 <= REGIONS['DTCM'][1], 'minimum DTCM stack reserve'
+    assert high['RAM']+0x400 <= REGIONS['RAM'][1], 'minimum main stack reserve'
+    return dict(configuration=directory.name, magic=magic.decode(), image_bytes=len(image),
+                payload_bytes=length, checksum=f'0x{checksum:02X}', initial_msp=f'0x{msp:08X}',
+                reset_vector=f'0x{reset:08X}', memory_high_water_bytes=high,
+                image_sha256=hashlib.sha256(image).hexdigest(),
+                elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest(), result='PASS')
+
+def main():
+    parser = argparse.ArgumentParser(description=__doc__)
+    parser.add_argument('--objdump', default='arm-none-eabi-objdump')
+    parser.add_argument('directories', nargs='+', type=Path)
+    args = parser.parse_args()
+    for directory in args.directories:
+        print(json.dumps(check(directory,args.objdump),sort_keys=True))
+
+if __name__ == '__main__':
+    main()
diff --git a/tests/euclidean/host.c b/tests/euclidean/host.c
new file mode 100644
index 0000000..d173290
--- /dev/null
+++ b/tests/euclidean/host.c
@@ -0,0 +1,326 @@
+// Host seams around original source bodies; never execute firmware or touch hardware.
+#include <assert.h>
+#include <inttypes.h>
+#include <stdio.h>
+#include <stdlib.h>
+#include <string.h>
+#include "Soundengine/Sequencer/sequencer.h"
+#include "Soundengine/MIDI/CortexParser.h"
+#include "Soundengine/MIDI/MidiMessages.h"
+#include "Soundengine/MIDI/MidiVoiceControl.h"
+#include "Preset/PresetManager.h"
+#include "menu/menu.h"
+#include "frontPanelParser_ui.h"
+#include "ledHandler.h"
+#ifdef BASELINE_GENERATOR
+// Oracle only: model ARM CLZ(0)=32 for the pre-fix builtin. Candidate has no shim.
+#undef __CLZ
+#define __CLZ(x) ((uint32_t)(x) ? __builtin_clz((uint32_t)(x)) : 32)
+#include "baseline-generator.inc"
+#else
+#include "Core/Src/Soundengine/Sequencer/EuklidGenerator.c"
+#endif
+
+PatternSet seq_patternSet;
+PatternBackupBuffer patternBackupBuffer;
+uint8_t seq_activePattern, seq_pendingPattern, seq_running, seq_recordActive;
+static uint8_t seq_mutedTracks;
+static int8_t seq_stepIndex[NUM_TRACKS];
+int8_t seq_stepToSkip[NUM_TRACKS];
+uint8_t lastWasNoteOn[NUM_TRACKS], chokeCurrent, chokeNext;
+struct { uint8_t prev, current, next; } seq_rndValue[NUM_TRACKS];
+uint8_t frontParser_shownPattern, frontParser_activeTrack;
+uint8_t frontParser_sysexActive, frontParser_rxCnt;
+uint16_t frontParser_sysexSeqStepNr, frontParser_twoByteData;
+static uint8_t frontParser_sysexBuffer[7];
+uint8_t menu_activeBar, menu_activePage, menu_playedPattern, menu_shownPattern;
+uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS];
+char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];
+TIM_HandleTypeDef htim2;
+FIL SDFile;
+#define STEPS_PER_PATTERN NUM_STEPS
+#define FEXT_PAT 1
+static uint8_t stream[65536], saved_stream[65536];
+static size_t stream_pos, stream_size, error_at = SIZE_MAX;
+static float elapsed = 75.f;
+static unsigned triggers;
+FRESULT f_open(FIL *file, const TCHAR *name, BYTE flags) {
+    (void)name; stream_pos=0; file->fptr=0;
+    if(flags & FA_CREATE_ALWAYS) stream_size=0;
+    return FR_OK;
+}
+FRESULT f_close(FIL *file) { (void)file; return FR_OK; }
+FRESULT f_lseek(FIL *file, DWORD offset) {
+    assert(offset<=stream_size); stream_pos=offset; file->fptr=offset; return FR_OK;
+}
+FRESULT f_read(FIL *file, void *dst, UINT count, UINT *actual) {
+    assert(stream_pos<=stream_size);
+    size_t old=stream_pos;
+    size_t n=count < stream_size-stream_pos ? count : stream_size-stream_pos;
+    memcpy(dst,stream+stream_pos,n); stream_pos+=n; file->fptr=stream_pos; *actual=n;
+    // Deliberately return an error with a full byte count too: callers must check both.
+    return old>=error_at ? FR_DISK_ERR : FR_OK;
+}
+FRESULT f_write(FIL *file, const void *src, UINT count, UINT *actual) {
+    assert(stream_pos+count<=sizeof(stream)); memcpy(stream+stream_pos,src,count);
+    stream_pos+=count; file->fptr=stream_pos;
+    if(stream_pos>stream_size) stream_size=stream_pos;
+    *actual=count; return FR_OK;
+}
+static FRESULT preset_makeFileNameLoad(char *buf,uint8_t num,uint8_t type) {
+    (void)buf; (void)num; (void)type; return f_open(&SDFile,"pattern",FA_READ);
+}
+static void preset_makeFileNameSave(char *buf,uint8_t num,uint8_t type,char *name) {
+    (void)num; (void)type; (void)name; strcpy(buf,"pattern");
+}
+static void preset_makeFileNameOld(char *buf,uint8_t num,uint8_t type) {
+    (void)num; (void)type; strcpy(buf,"pattern");
+}
+HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *timer) { (void)timer; return HAL_OK; }
+HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *timer) { (void)timer; return HAL_OK; }
+void menu_repaintAll(void) {}
+void seq_sendStepInfoToFront(uint16_t step) { (void)step; }
+void frontPanel_sendData(uint8_t a,uint8_t b,uint8_t c) {
+    if(a==SEQ_CC && b==SEQ_SET_SHOWN_PATTERN) frontParser_shownPattern=c;
+}
+void seq_triggerVoice(uint8_t voice,uint8_t vel,uint8_t note) {
+    assert(voice<NUM_TRACKS && vel<=127 && note==60); triggers++;
+}
+void voiceControl_noteOff(uint8_t voice) { assert(voice<NUM_TRACKS); }
+float seq_getElapsedStepTimePercent(void) { return elapsed; }
+uint8_t seq_determineNextPattern(uint8_t change) { (void)change; abort(); }
+void led_setValue(uint8_t value,uint8_t led) { (void)value; (void)led; }
+#include "functions.inc"
+
+static void reset(void) {
+    memset(&seq_patternSet,0,sizeof(seq_patternSet));
+    for(unsigned p=0;p<NUM_PATTERN;p++) for(unsigned t=0;t<NUM_TRACKS;t++)
+        seq_patternSet.length[p][t]=16;
+    seq_activePattern=seq_pendingPattern=frontParser_shownPattern=menu_playedPattern=0;
+    frontParser_activeTrack=menu_activeBar=menu_shownPattern=0;
+    menu_activePage=PERF_PAGE; seq_running=seq_recordActive=1; seq_mutedTracks=0x7e;
+    memset(seq_stepIndex,0,sizeof(seq_stepIndex)); error_at=SIZE_MAX;
+    stream_pos=stream_size=0; elapsed=75.f; triggers=0; euklid_init();
+}
+static void lengths_unchanged(unsigned pattern) {
+    for(unsigned t=0;t<NUM_TRACKS;t++) assert(seq_patternSet.length[pattern][t]==t+1);
+}
+static void old_lengths(unsigned pattern) {
+    for(unsigned t=0;t<NUM_TRACKS;t++) seq_patternSet.length[pattern][t]=t+1;
+}
+static void generation(void) {
+    for(unsigned length=1;length<=16;length++) for(unsigned pulses=1;pulses<=length;pulses++) {
+        euklid_init(); euklid_setLength(0,length); euklid_setSteps(0,pulses,0);
+        for(unsigned rotation=0;rotation<16;rotation++) {
+            euklid_setRotation(0,rotation,0);
+#ifndef BASELINE_GENERATOR
+            assert(seq_patternSet.length[0][0]==length);
+#endif
+            assert(__builtin_popcountll(euklid_patternBuffer.quadword)==pulses);
+            assert((euklid_patternBuffer.quadword>>length)==0);
+            printf("%u,%u,%u,%016" PRIx64 "\n",length,pulses,rotation,euklid_patternBuffer.quadword);
+        }
+    }
+}
+static void bounds(void) {
+    for(unsigned t=0;t<NUM_TRACKS;t++) for(unsigned p=0;p<NUM_PATTERN;p++) {
+        euklid_setLength(t,16); euklid_setSteps(t,4,p);
+        assert(seq_patternSet.length[p][t]==16);
+    }
+    for(unsigned bad=NUM_TRACKS;bad<=255;bad++) {
+        euklid_setLength(bad,255); euklid_setSteps(bad,3,0); euklid_setRotation(bad,1,0);
+        euklid_generate(bad,0); euklid_transferPattern(bad,0);
+        assert(euklid_getLength(bad)==16 && euklid_getSteps(bad)==1 && euklid_getRotation(bad)==0);
+        seq_setTrackLength(bad,5); assert(seq_getTrackLength(bad)==16);
+    }
+    for(unsigned bad=NUM_PATTERN;bad<=255;bad++) {
+        euklid_setSteps(0,3,bad); euklid_setRotation(0,1,bad);
+        euklid_generate(0,bad); euklid_transferPattern(0,bad);
+        frontParser_shownPattern=bad; seq_setTrackLength(0,3); assert(seq_getTrackLength(0)==16);
+        assert(!preset_readPatternData(bad,3)); assert(preset_loadPatternSD(bad)==FR_INVALID_PARAMETER);
+    }
+    frontParser_shownPattern=0;
+    for(unsigned value=0;value<=255;value++) {
+        euklid_setLength(0,value);
+        assert(euklid_getLength(0)==(value==0?1:value>16?16:value));
+        seq_patternSet.length[0][0]=9; seq_setTrackLength(0,value);
+        assert(seq_patternSet.length[0][0]==(value==0?16:value<=64?value:9));
+    }
+    euklid_rotatePattern(0,1); euklid_rotatePattern(64,1);
+    puts("PASS bounds: all byte-valued track/pattern/length inputs");
+}
+static void playback(void) {
+    unsigned ticks=0,notes=0;
+    for(unsigned length=1;length<=64;length++) {
+        seq_setTrackLength(0,length);
+        for(unsigned i=0;i<64;i++) {
+            seq_setMainStep(0,0,i,1);
+            seq_patternSet.seq_subStepPattern[0][0][i].note=60;
+            seq_patternSet.seq_subStepPattern[0][0][i].volume=100;
+            seq_patternSet.seq_subStepPattern[0][0][i].prob=100;
+        }
+        for(unsigned flam=0;flam<=5;flam++) for(unsigned shift=0;shift<16;shift++) {
+            for(unsigned i=0;i<64;i++) {
+                seq_patternSet.seq_stepTiming[0][0][i].flam=flam;
+                seq_patternSet.seq_stepTiming[0][0][i].shift=shift;
+            }
+            for(unsigned side=0;side<2;side++) {
+                seq_stepIndex[0]=side?length-1:0;
+                for(unsigned tick=0;tick<24;tick++) { seq_processFlam(tick); ticks++; }
+            }
+        }
+        for(unsigned step=0;step<length;step++) for(unsigned phase=0;phase<2;phase++) {
+            elapsed=phase?75.f:25.f; seq_stepIndex[0]=step;
+            unsigned expected=(step+phase)%length;
+            memset(seq_patternSet.seq_subStepPattern[0][0],0,sizeof(seq_patternSet.seq_subStepPattern[0][0]));
+            seq_addNote(0,100,60);
+            assert(seq_patternSet.seq_subStepPattern[0][0][expected].note==60);
+            assert(seq_isMainStepActive(0,expected,0)); notes++;
+        }
+    }
+    seq_setTrackLength(0,0); assert(seq_getTrackLength(0)==16);
+    uint8_t last=seq_getTrackLength(0); last-=1; seq_setTrackLength(0,last); assert(seq_getTrackLength(0)==15);
+    seq_patternSet.length[0][0]=16; seq_patternSet.length[0][1]=8;
+    for(unsigned t=2;t<7;t++) seq_patternSet.length[0][t]=1;
+    assert(seq_getLongestTrack()==0);
+    seq_patternSet.length[0][6]=64; assert(seq_getLongestTrack()==6);
+    assert(triggers>0);
+    printf("PASS playback: %u flam/shift ticks; %u recording positions; mixed lengths\n",ticks,notes);
+}
+static void fill_pattern(unsigned p,unsigned length) {
+    for(unsigned t=0;t<7;t++) {
+        seq_patternSet.length[p][t]=length;
+        for(unsigned i=0;i<64;i++) {
+            Step *step=&seq_patternSet.seq_subStepPattern[p][t][i];
+            memset(step,(i+t)%128,sizeof(*step));
+            seq_patternSet.seq_stepTiming[p][t][i].flam=(i+t)%6;
+            seq_patternSet.seq_stepTiming[p][t][i].shift=(i+t)%15;
+        }
+        for(unsigned b=0;b<4;b++) seq_patternSet.seq_mainSteps[b][p][t]=0xa55a^(b*7+t);
+    }
+    seq_patternSet.shuffle[p]=23; seq_patternSet.kitNr[p]=5;
+    memcpy(savefileNames[p][PATTERN_NAMES],"HOSTTEST",8);
+}
+static void roundtrips(void) {
+    unsigned cases=0;
+    for(unsigned tmp=0;tmp<2;tmp++) for(unsigned length=0;length<=64;length++) {
+        unsigned p=tmp?63:0; menu_playedPattern=p; frontParser_shownPattern=p;
+        fill_pattern(p,length?length:16);
+        assert((tmp?preset_saveTmpPat():preset_savePatternSD(p))==FR_OK);
+        size_t size=stream_size,offset=tmp?3642:3655;
+        assert(size==(tmp?3650:3663));
+        if(!tmp) assert(!memcmp(stream,"HOSTTESTPatV03",14));
+        assert(stream[offset]==(length?length:16));
+        // Construct a legacy zero-bearing file without using an invalid RAM state.
+        if(length==0) memset(stream+offset,0,NUM_TRACKS);
+        memcpy(saved_stream,stream,size);
+        Step saved_steps[NUM_TRACKS][NUM_STEPS]; StepTiming saved_timing[NUM_TRACKS][NUM_STEPS];
+        memcpy(saved_steps,seq_patternSet.seq_subStepPattern[p],sizeof(saved_steps));
+        memcpy(saved_timing,seq_patternSet.seq_stepTiming[p],sizeof(saved_timing));
+        old_lengths(p); stream_pos=0; error_at=SIZE_MAX;
+        assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(p))==FR_OK);
+        for(unsigned t=0;t<7;t++) assert(seq_patternSet.length[p][t]==(length?length:16));
+        assert(!memcmp(saved_steps,seq_patternSet.seq_subStepPattern[p],sizeof(saved_steps)));
+        assert(!memcmp(saved_timing,seq_patternSet.seq_stepTiming[p],sizeof(saved_timing)));
+        for(unsigned t=0;t<7;t++) for(unsigned b=0;b<4;b++) assert(seq_patternSet.seq_mainSteps[b][p][t]==(0xa55a^(b*7+t)));
+        assert(seq_patternSet.shuffle[p]==23 && seq_patternSet.kitNr[p]==5);
+        assert((tmp?preset_saveTmpPat():preset_savePatternSD(p))==FR_OK);
+        if(length==0) memset(saved_stream+offset,16,NUM_TRACKS);
+        assert(stream_size==size && !memcmp(stream,saved_stream,size)); cases++;
+        if(length==0) {
+            // Exercise the formerly failing consumers directly after each legacy loader.
+            seq_activePattern=p;
+            for(unsigned i=0;i<64;i++) seq_patternSet.seq_subStepPattern[p][0][i].note=60;
+            seq_stepIndex[0]=0; seq_processFlam(0);
+            seq_stepIndex[0]=15; elapsed=75.f; seq_addNote(0,100,60);
+            assert(seq_patternSet.seq_subStepPattern[p][0][0].note==60);
+            seq_patternSet.length[p][1]=8;
+            for(unsigned t=2;t<7;t++) seq_patternSet.length[p][t]=1;
+            assert(seq_getLongestTrack()==0);
+        }
+    }
+    printf("PASS persistence: %u full named/autosave roundtrips, unchanged layout/payload\n",cases);
+}
+static void malformed(void) {
+    unsigned cases=0;
+    for(unsigned tmp=0;tmp<2;tmp++) {
+        reset(); fill_pattern(0,16);
+        assert((tmp?preset_saveTmpPat():preset_savePatternSD(0))==FR_OK);
+        size_t size=stream_size,offset=tmp?3642:3655; memcpy(saved_stream,stream,size);
+        for(size_t cut=0;cut<size;cut++) {
+            stream_size=cut; old_lengths(0);
+            assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(0))!=FR_OK);
+            lengths_unchanged(0); cases++;
+        }
+        stream_size=size;
+        for(unsigned t=0;t<7;t++) for(unsigned bad=65;bad<=255;bad++) {
+            memcpy(stream,saved_stream,size); stream[offset+t]=bad; old_lengths(0);
+            assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(0))!=FR_OK);
+            lengths_unchanged(0); cases++;
+        }
+        memcpy(stream,saved_stream,size);
+        // Error injection covers every read start, including successful-looking counts.
+        size_t points[]={0,tmp?1:14,offset-57,offset-1,offset,offset+7};
+        for(unsigned i=0;i<sizeof(points)/sizeof(points[0]);i++) {
+            error_at=points[i]; old_lengths(0);
+            assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(0))!=FR_OK);
+            lengths_unchanged(0); cases++;
+        }
+        error_at=SIZE_MAX;
+    }
+    // V1/V2 synthetic bodies use the source-defined no-timing layout.
+    const size_t old_offset=7*64*sizeof(Step)+7*4*2+1;
+    for(unsigned version=1;version<=2;version++) for(unsigned tail=0;tail<=8;tail++) {
+        memset(stream,0,sizeof(stream)); stream_size=old_offset+tail; stream_pos=0;
+        if(tail) memset(stream+old_offset,8,tail);
+        old_lengths(0); unsigned ok=preset_readPatternData(0,version);
+        unsigned expected=(version==1 && (tail==0 || tail>=7)) || (version==2 && tail==8);
+        assert(!!ok==!!expected);
+        if(expected) for(unsigned t=0;t<7;t++) assert(seq_patternSet.length[0][t]==(tail?8:16));
+        else lengths_unchanged(0);
+        cases++;
+    }
+    menu_playedPattern=menu_shownPattern=seq_activePattern=seq_pendingPattern=3;
+    for(unsigned invalid=64;invalid<=255;invalid++) {
+        stream[0]=invalid; stream_size=1; assert(preset_LoadTmpPat()!=FR_OK);
+        assert(menu_playedPattern==3 && menu_shownPattern==3 && seq_activePattern==3 && seq_pendingPattern==3);
+    }
+    error_at=0; stream[0]=255; assert(preset_LoadTmpPat()!=FR_OK);
+    assert(menu_playedPattern==3 && seq_activePattern==3); error_at=SIZE_MAX;
+    printf("PASS malformed: %u truncation/range/I-O/legacy-tail cases; no partial lengths\n",cases+193);
+}
+static void protocol(void) {
+    unsigned calls=0;
+    for(unsigned input=0;input<=255;input++) {
+        frontParser_sysexActive=SYSEX_RECEIVE_PAT_LEN_DATA; frontParser_sysexSeqStepNr=0;
+        for(unsigned slot=0;slot<NUM_PATTERN*NUM_TRACKS;slot++) {
+            unsigned p=slot/7,t=slot%7; seq_patternSet.length[p][t]=9;
+            frontParser_handleSysexData(input);
+            assert(seq_patternSet.length[p][t]==(input==0?16:input<=64?input:9));
+            assert(frontParser_sysexSeqStepNr==slot+1); calls++;
+        }
+        for(unsigned extra=0;extra<4;extra++) {
+            frontParser_handleSysexData(1); assert(frontParser_sysexSeqStepNr==448); calls++;
+        }
+    }
+    for(unsigned counter=448;counter<=65535;counter++) {
+        frontParser_sysexSeqStepNr=counter; frontParser_handleSysexData(16);
+        assert(frontParser_sysexSeqStepNr==counter); calls++;
+    }
+    // Existing mode-selection mechanism restarts a new legal stream.
+    frontParser_sysexActive=SYSEX_ACTIVE_MODE_NONE; frontParser_handleSysexData(SYSEX_RECEIVE_PAT_LEN_DATA);
+    assert(frontParser_sysexSeqStepNr==0); frontParser_handleSysexData(0);
+    assert(frontParser_sysexSeqStepNr==1 && seq_patternSet.length[0][0]==16);
+    printf("PASS protocol: %u byte-value/counter boundary calls; no wrap\n",calls+2);
+}
+int main(int argc,char **argv) {
+    assert(argc==2); reset();
+    if(!strcmp(argv[1],"generation")) generation();
+    else if(!strcmp(argv[1],"bounds")) bounds();
+    else if(!strcmp(argv[1],"playback")) playback();
+    else if(!strcmp(argv[1],"roundtrips")) roundtrips();
+    else if(!strcmp(argv[1],"malformed")) malformed();
+    else if(!strcmp(argv[1],"protocol")) protocol();
+    else abort();
+    return 0;
+}
diff --git a/tests/euclidean/run.py b/tests/euclidean/run.py
new file mode 100644
index 0000000..a66c836
--- /dev/null
+++ b/tests/euclidean/run.py
@@ -0,0 +1,100 @@
+#!/usr/bin/env python3
+"""Sanitized host regression of original source bodies. No firmware execution."""
+import argparse
+import hashlib
+import os
+from pathlib import Path
+import re
+import subprocess
+import tempfile
+
+ROOT = Path(__file__).resolve().parents[2]
+HERE = Path(__file__).resolve().parent
+BASE = '5829398cea5d337200393757e39a37d4d8148256'
+GENERATOR = 'Core/Src/Soundengine/Sequencer/EuklidGenerator.c'
+# SHA-256 of 2176 exact length,pulse,rotation,mask lines from lab-setup,
+# with only host CLZ(0)=32 emulation. --record-baseline reproduces the oracle.
+EXPECTED_MASKS = '212cd0c7e3da155464507d1bf22e90c0e50d38f32700cfbfefcde8f3307ac883'
+
+def extract(path, name):
+    source = (ROOT / path).read_text(errors='replace')
+    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
+                    lambda m: ' ' * len(m[0]), source, flags=re.S)
+    match = re.search(r'^(?:IN_ITCM_RAM\s+)?(?:static\s+)?(?:inline\s+)?'
+                      r'(?:void|float|FRESULT|uint\d+_t|int\d+_t)\s+'
+                      + name + r'\([^;]*?\)\s*\{', masked, re.M)
+    if not match:
+        raise RuntimeError('Cannot extract ' + name)
+    depth, end = 1, match.end()
+    while depth:
+        depth += (masked[end] == '{') - (masked[end] == '}')
+        end += 1
+    line = source.count('\n', 0, match.start()) + 1
+    return f'#line {line} "{path}"\n' + source[match.start():end] + '\n'
+
+def run(command, **kwargs):
+    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
+                            timeout=120, **kwargs)
+    if result.returncode:
+        raise RuntimeError(f'{command[0]} exited {result.returncode}\n'
+                           + result.stdout + result.stderr)
+    return result
+
+def main():
+    parser = argparse.ArgumentParser(description=__doc__)
+    parser.add_argument('--record-baseline', action='store_true',
+                        help='requires the pinned base git object; print oracle hash only')
+    args = parser.parse_args()
+    seq = 'Core/Src/Soundengine/Sequencer/sequencer.c'
+    preset = 'Core/Src/Preset/presetManager.c'
+    functions = [(seq, name) for name in (
+        'seq_setTrackLength', 'seq_getTrackLength', 'seq_getLongestTrack',
+        'seq_copyPatternToBackupBuffer', 'seq_copyBackupBufferToPattern',
+        'getShiftValue', 'flamIdxToPattern', 'seq_intIsMainStepActive',
+        'seq_isMainStepActive', 'seq_setMainStep', 'seq_getShiftFlamPattern',
+        'seq_processFlam', 'seq_addNote')]
+    functions += [(preset, name) for name in (
+        'preset_readPatternLengths', 'preset_writePatternData',
+        'preset_readPatternData', 'preset_savePatternSD', 'preset_loadPatternSD',
+        'preset_saveTmpPat', 'preset_LoadTmpPat')]
+    functions += [('Core/Src/Soundengine/MIDI/CortexParser.c', 'frontParser_handleSysexData')]
+    includes = ['.', 'Core/Inc', 'Drivers/CMSIS/Include',
+                'Drivers/CMSIS/Device/ST/STM32F7xx/Include',
+                'Drivers/STM32F7xx_HAL_Driver/Inc',
+                'Middlewares/Third_Party/FatFs/src', 'FATFS/App', 'FATFS/Target']
+    with tempfile.TemporaryDirectory(prefix='lxr-euclidean-') as directory:
+        temporary = Path(directory)
+        (temporary / 'functions.inc').write_text(''.join(extract(*f) for f in functions))
+        compiler = os.environ.get('CC', 'cc')
+        flags = [compiler, '-std=gnu11', '-O1', '-g', '-DSTM32F765xx', '-DUSE_HAL_DRIVER',
+                 '-Wno-int-to-pointer-cast', '-Wno-pointer-to-int-cast',
+                 '-Werror=implicit-function-declaration', '-ffunction-sections',
+                 '-fdata-sections', '-fno-pie', '-no-pie', '-fsanitize=address,undefined',
+                 '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
+        flags += ['-I' + str(ROOT / p) for p in includes] + ['-I' + directory]
+        if args.record_baseline:
+            original = run(['git', 'show', BASE + ':' + GENERATOR]).stdout
+            (temporary / 'baseline-generator.inc').write_text(original)
+            flags += ['-DBASELINE_GENERATOR']
+        executable = temporary / 'host'
+        build = run(flags + [str(HERE / 'host.c'), '-Wl,--gc-sections', '-o', str(executable)])
+        if build.stderr:
+            print(build.stderr, end='')
+        env = os.environ.copy()
+        env['ASAN_OPTIONS'] = 'detect_leaks=0'
+        result = run([str(executable), 'generation'], env=env)
+        digest = hashlib.sha256(result.stdout.encode()).hexdigest()
+        assert len(result.stdout.splitlines()) == 2176
+        if args.record_baseline:
+            print('BASELINE_MASKS=' + digest)
+            return
+        assert digest == EXPECTED_MASKS, f'Rhythm masks changed: {digest}'
+        print('PASS generation: 2176 exact masks match lab-setup; literal lengths; CLZ unshimmed')
+        for scenario in ('bounds', 'playback', 'roundtrips', 'malformed', 'protocol'):
+            result = run([str(executable), scenario], env=env)
+            print(result.stdout, end='')
+            assert not result.stderr, result.stderr
+        print('PASS all six ASan/UBSan groups')
+
+if __name__ == '__main__':
+    main()

```

## Appendix B — static stack analysis output

```text
Compiled 77 application translation units with unchanged Release flags plus -fstack-usage.
(1096, '/workspace/lxr-desktop-app/Core/Src/main.c:108:6:sd_formatRoutine', 'static')
(448, '/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/mixer.c:548:18:mixer_calcNextSampleBlock', 'static')
(416, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:2294:9:preset_saveProject', 'static')
(384, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:2674:9:preset_loadProject', 'static')
(336, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:650:9:preset_clearTmpFolder', 'static')
(248, '/workspace/lxr-desktop-app/Core/Src/main.c:1010:6:SystemClock_Config', 'static')
(152, '/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/FX/Reverb.c:207:6:Reverb_process', 'static')
(152, '/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/FX/Distortion.c:208:6:Distortion_process', 'static')
(136, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:2010:9:preset_loadPatternSD', 'static')
(120, '/workspace/lxr-desktop-app/Core/Src/main.c:239:5:main', 'static')
(120, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:2557:9:preset_loadSongSD', 'static')
(120, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:1179:9:preset_loadDrumsetSD', 'static')
(104, '/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/FX/Compressor.c:104:6:Compressor_process', 'dynamic')
(104, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:699:9:empty_directory', 'static')
(88, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:608:9:delete_node', 'static')
(88, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:2632:6:preset_loadProjectNames', 'static')
(80, '/workspace/lxr-desktop-app/Core/Src/stm32f7xx_hal_msp.c:235:6:HAL_I2S_MspInit', 'static')
(80, '/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/FX/Delay.c:93:6:Delay_processPingPong', 'static')
(80, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:901:9:preset_saveDrumsetSD', 'static')
(72, '/workspace/lxr-desktop-app/Core/Src/Soundengine/Sequencer/sequencer.c:2617:6:seq_copyPattern', 'static')
(72, '/workspace/lxr-desktop-app/Core/Src/Soundengine/Sequencer/EuklidGenerator.c:62:6:euklid_calcRecursive', 'static')
(72, '/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/FX/Delay.c:187:6:Delay_process', 'static')
(72, '/workspace/lxr-desktop-app/Core/Src/Preset/presetManager.c:1677:9:preset_savePatternSD', 'static')
(64, '/workspace/lxr-desktop-app/Core/Src/stm32f7xx_hal_msp.c:466:6:HAL_SD_MspInit', 'static')
(64, '/workspace/lxr-desktop-app/Core/Src/stm32f7xx_hal_msp.c:107:6:HAL_ADC_MspInit', 'static')
/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/CymbalVoice.c: In function 'Cymbal_calcSyncBlock':
/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/CymbalVoice.c:157:6: warning: stack usage might be unbounded [-Wstack-usage=]
  157 | void Cymbal_calcSyncBlock(int16_t* buf, const uint8_t size)
      |      ^~~~~~~~~~~~~~~~~~~~

/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/DrumVoice.c: In function 'calcDrumVoiceSyncBlock':
/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/DrumVoice.c:239:18: warning: stack usage might be unbounded [-Wstack-usage=]
  239 | IN_ITCM_RAM void calcDrumVoiceSyncBlock(const uint8_t voiceNr, int16_t* buf, const uint8_t size)
      |                  ^~~~~~~~~~~~~~~~~~~~~~

/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/HiHat.c: In function 'HiHat_calcSyncBlock':
/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/HiHat.c:165:6: warning: stack usage might be unbounded [-Wstack-usage=]
  165 | void HiHat_calcSyncBlock(int16_t* buf, const uint8_t size)
      |      ^~~~~~~~~~~~~~~~~~~

/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/Snare.c: In function 'Snare_calcSyncBlock':
/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/Snare.c:148:6: warning: stack usage might be unbounded [-Wstack-usage=]
  148 | void Snare_calcSyncBlock(int16_t* buf, const uint8_t size)
      |      ^~~~~~~~~~~~~~~~~~~

/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/FX/Compressor.c: In function 'Compressor_process':
/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/FX/Compressor.c:104:6: warning: stack usage might be unbounded [-Wstack-usage=]
  104 | void Compressor_process(Compressor *c, int16_t *fxBuffer,int16_t* outL, int16_t* outR, uint16_t size,  uint8_t channels, float dryWet)
      |      ^~~~~~~~~~~~~~~~~~

/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/FX/Ringmod.c: In function 'Ringmod_process':
/workspace/lxr-desktop-app/Core/Src/Soundengine/DSPAudio/FX/Ringmod.c:31:6: warning: stack usage might be unbounded [-Wstack-usage=]
   31 | void Ringmod_process(Ringmod *rm, int16_t *fxBuffer, int16_t* outL, int16_t* outR, uint16_t size,  uint8_t channels, float dryWet)
      |      ^~~~~~~~~~~~~~~
```

## Appendix C — complete executed normal build logs

Stock installation instructions printed by the build are omitted; they were not executed.

### release-config (exit0)

```text
Preset CMake variables:

  CMAKE_BUILD_TYPE="Release"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/release

```

### release-build (exit0)

```text
[1/7] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[2/7] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[3/7] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[4/7] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[5/7] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[6/7] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      223552 B       992 KB     22.01%
   text	   data	    bss	    dec	    hex	filename
 274704	    852	 340576	 616132	  966c4	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 275564 bytes, checksum 0xFB
[7/7] 
```

### debug-config (exit0)

```text
Preset CMake variables:

  CMAKE_BUILD_TYPE="Debug"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/debug

```

### debug-build (exit0)

```text
[1/7] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[2/7] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[3/7] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[4/7] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[5/7] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[6/7] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      223600 B       992 KB     22.01%
   text	   data	    bss	    dec	    hex	filename
 274752	    852	 340576	 616180	  966f4	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 275612 bytes, checksum 0x36
[7/7] 
```

### release-pinned (exit0)

```text
[1/1] 
```

### debug-pinned (exit0)

```text
[1/1] 
```

## Appendix D — allocated ELF sections and largest linked symbols

```text

build/release/LXR_V2_STM32F765.elf:     file format elf32-littlearm

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .isr_vector   000001f8  08008000  08008000  00008000  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  1 .text         00022228  08008200  08008200  00008200  2**6
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  2 .rodata       000141b4  0802a428  0802a428  0002a428  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .ARM.extab    00000000  0803e5dc  0803e5dc  00063aa8  2**0
                  CONTENTS, READONLY
  4 .ARM          00000008  0803e5dc  0803e5dc  0003e5dc  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  5 .preinit_array 00000000  0803e5e4  0803e5e4  00063aa8  2**0
                  CONTENTS, ALLOC, LOAD, DATA
  6 .init_array   00000004  0803e5e4  0803e5e4  0003e5e4  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  7 .fini_array   00000004  0803e5e8  0803e5e8  0003e5e8  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  8 .data         00000354  20020000  0803e5ec  00040000  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  9 .bss          00041d68  20020358  0803e940  00060358  2**3
                  ALLOC
 10 ._user_heap   00000200  200620c0  0803e940  000620c0  2**0
                  ALLOC
 11 .dtcm         00009084  20000000  0803e940  00050000  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
 12 .dtcmz        000112f8  20009088  080479c8  00069088  2**3
                  ALLOC
 13 .itcm_text    00003aa8  00000000  080479c4  00060000  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
 14 .ARM.attributes 00000030  00000000  00000000  00063aa8  2**0
                  CONTENTS, READONLY
 15 .comment      0000004c  00000000  00000000  00063ad8  2**0
                  CONTENTS, READONLY
 16 .debug_info   0000c79d  00000000  00000000  00063b24  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 17 .debug_abbrev 000035e8  00000000  00000000  000702c1  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 18 .debug_loclists 00003af5  00000000  00000000  000738a9  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 19 .debug_aranges 00000498  00000000  00000000  000773a0  2**3
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 20 .debug_line   000058f1  00000000  00000000  00077838  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 21 .debug_str    00001c6b  00000000  00000000  0007d129  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 22 .debug_frame  00000994  00000000  00000000  0007ed94  2**2
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 23 .debug_rnglists 000002da  00000000  00000000  0007f728  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
 24 .debug_line_str 00000151  00000000  00000000  0007fa02  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS

080181cc 000006a4 T buttonHandler_buttonPressed
080133ac 00000724 T f_mkfs
0801d46c 00000788 T menu_parseEncoder
20026640 000007b8 B parameterArray
20025cc8 00000800 B savefileNames
20009238 00000840 B voiceArray
0802a7a8 00000870 R __exp_data
080150d8 0000094c T main
0801af94 000009a4 T menu_repaintGeneric
08024b7c 00000a64 T frontParser_parseUartData
080274e0 00000a6c T parameterArray_init
08023950 00000c7c T Distortion_process
00000998 00000cb2 T mixer_calcNextSampleBlock
200281d0 00000e42 B patternBackupBuffer
200271d0 00001000 B seq_songData
0802c970 00001080 R menuPages
080256a8 00001790 T midiParser_ccHandler
20006870 00002002 R sine_table
20021e54 00003dc0 B mainParameters
08033530 00005800 R recTable
08038d30 00005800 R sawTable
0802dd30 00005800 R triTable
20000000 0000675c R transientData
20009e18 00010000 B FX_BUFFER
20029014 00039040 B seq_patternSet
```
