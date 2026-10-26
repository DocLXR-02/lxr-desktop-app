# LXR Firmware Lab — Phase B: Memory Safety and Optional Runtime Diagnostics

Date: 2026-10-26 UTC. Repository: DocLXR-02/lxr-desktop-app.

## 1. Outcome and explicit deployment status

**Not ready for deployment.** A bounded program break and optional timing/DMA observations are implemented and software-tested. A newly traced baseline allocator-reentrancy issue remains unresolved: USB configuration can allocate/free from USB interrupt context while main-context formatting can allocate, and the linked newlib allocator locks are no-ops. Bounding `_sbrk` does not make those concurrent allocator transactions safe. No crash from this overlap was reproduced; the call paths and missing serialization are confirmed, while actual overlap/corruption is a runtime risk.

No hardware was operated or flashed. No bootloader, option bytes, CPU clock, PLL, linker layout, startup, audio buffer size or interrupt priority was changed. No watchdog or automatic reset was activated. No generated firmware binary, ELF, image or build directory was committed/uploaded. This report and source are published as development work for review, not as a hardware-validated release.

CODE = inspected source/configuration; BUILD = executed test/build/binary analysis; HYPOTHESIS = plausible unconfirmed runtime consequence; OPEN = unresolved or requiring hardware/further verification.

| Reference | Exact value |
|---|---|
| Repository | `DocLXR-02/lxr-desktop-app` |
| Phase A base and preserved integration head | `e19bb0544dcc8f07138a9857c78022e8efaecd81` |
| Phase B branch | `safety/runtime-diagnostics-b1` |
| Phase B source commit | `e4c0fd3ca66ced53413d5e1733e7ac9aa1c0da6e` |
| Phase B tree | `249fbc62744a136ffe9b878fa1931b46c7bd86b6` |
| Preserved lab-setup / report base | `5829398cea5d337200393757e39a37d4d8148256` |
| Preserved Bug 001 | `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0` |
| Preserved Euclidean | `67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72` |
| Preserved main | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Report-only branch | `codex-reports/2026-10-26-phase-b-memory-safety-runtime-diagnostics` |
| Report path | `docs/codex-berichte/2026-10-26_phase-b-memory-safety-runtime-diagnostics.md` |

Source URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/safety/runtime-diagnostics-b1

Immutable commit: https://github.com/DocLXR-02/lxr-desktop-app/commit/e4c0fd3ca66ced53413d5e1733e7ac9aa1c0da6e

Only the existing `/workspace/lxr-desktop-app` checkout was used; no new worktree was created. The initial checkout was clean at the exact Phase A commit. Remote heads were checked and matched the expected commits; the requested new branch did not exist. AGENTS.md was read from remote lab-setup, blob `2f4d121f53a7055c1a30355235384a95ef920f1a`. The user's explicit Phase A base requirement governs this source branch; the separate report branch uses lab-setup as instructed. The cloud-environment-runtime skill was read and environment status checked. No sub-agents, paid services, PRs or merges were used.

The full [Phase A report](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-a-integration-safety-preflight/docs/codex-berichte/2026-10-26_phase-a-integration-safety-preflight.md) was reviewed. This report is self-contained and records the important refinement: the allocator has an interrupt entry path that was not established by the earlier Core-only search.

## 2. Baseline reproduction before source changes

Executed at Phase A before editing:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
cmake --version
ninja --version
python3 --version
arm-none-eabi-gcc --version
python3 tests/bug001/run.py
python3 tests/euclidean/run.py
cmake --preset release
cmake --build --preset release --parallel 4
cmake --preset debug
cmake --build --preset debug --parallel 4
python3 tests/euclidean/check_images.py build/release build/debug
git switch -c safety/runtime-diagnostics-b1 e19bb0544dcc8f07138a9857c78022e8efaecd81
```

All returned zero. The baseline image hashes reproduced Phase A exactly, as did ITCM 15,016, DTCM 107,392 and RAM 271,040 bytes. Existing build directories were reused; this is not claimed as an independent clean-room rebuild.

Tools: CMake 3.31.10; Ninja 1.13.0.git.kitware.jobserver-pipe-1; Python 3.12.14; Arm GNU 14.2.1 20241119 (15:14.2.rel1-1), newlib-nano; native GCC 14.2.0. No relevant compiler difference from Phase A was found. Source version remains CMake 1.7. No equivalence with official public v1.71 or private support v1.72 is claimed.

Baseline image results:

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

## 3. Heap investigation and bounded allocation

### Evidence and legal region

CODE/BUILD: The baseline map/disassembly identified libnosys `_sbrk`, which loaded a static break, initialized it from `end`, added the requested signed increment and stored the result without checking a limit. This was an established unbounded growth weakness, not a demonstrated physical crash. The custom source `_sbrk` in `Core/Src/sysmem.c` had been disabled by `#if 0`.

`STM32F765VGTX_FLASH.ld:38–47,175–183` defines general SRAM 0x20020000…0x20080000 and places `_end` at the start of the 512-byte minimum heap reservation after all general-SRAM static objects. `_estack` is the end of that region. Startup (`Core/Startup/startup_stm32f765vgtx.s:68`) explicitly changes SP to `_dtcm_stack=0x20020000` before C initialization. No active PSP/SPSEL switch was found in Core. DTCM contains the active main/interrupt stack; the general-SRAM vector `_estack` is not the running stack after reset entry. All allocated ELF sections were checked; there is no later general-SRAM static object above this heap start.

Thus, for this unchanged startup/linker configuration, the legal heap interval is aligned `_end` through exclusive 0x20080000. Baseline start was 0x200620c0; Phase B normal start is 0x200620b8 (122,696 bytes available including the minimum heap reservation), instrumented start 0x20062100 (122,624 bytes). These move naturally with linked BSS. The 512-byte reservation is a minimum allocation reservation, not a maximum heap bound. No fixed heap address was added to source.

### Implemented correction and exact guarantee

`Core/Src/sysmem.c:50ff` now supplies a strong `caddr_t _sbrk(int increment)`. New `Core/Inc/safety/heapBounds.h` contains the tested integer-only boundary operation.

- Align the initial break up to 8 bytes with an overflow check. Existing target `_end` values already satisfy that alignment.
- Preserve byte-granular sbrk increments, including valid negative shrink. The allocator retains responsibility for chunk alignment; rounding every increment would violate sbrk semantics.
- Reject growth beyond the end symbol, shrink below heap start, invalid existing bounds/current position and arithmetic wrap. Negative magnitude is calculated without negating INT_MIN.
- Return previous break on success. On failure return `(caddr_t)-1`, set `errno=ENOMEM`, and do not move the break. Success does not clear errno, matching ordinary runtime conventions.
- Use existing symbols only: no startup/linker/ABI modification, buffer relocation or active-stack probing.

BUILD: The final ELF resolves `_sbrk` to this 100-byte strong function, not the libnosys routine. `_sbrk_r` disassembly confirms its return/errno convention: it calls `_sbrk`, tests for -1, and copies errno to the reentrant structure. Newlib `_malloc_r` uses that path and returns allocation failure when growth fails. Actual physical OOM handling and every caller's response were not executed on target. Some existing callers ignore formatting failures; a bounded heap cannot make unchecked application error handling safe.

**Scope of proof:** arithmetic and range safety for serialized break operations. No claim of allocator reentrancy, arbitrary heap-corruption protection, fragmentation prevention or whole-program OOM safety is made. This limitation is explicit in code comments and test documentation.

### Allocating library operations and interrupt context: serious residual issue

CODE/BUILD:

- The map pulls `_malloc_r`, `_realloc_r` and `_free_r` through newlib-nano formatted I/O (`sprintf` → `_svfprintf_r`). It also links `malloc` through USB class code. Absence of direct Core `malloc` calls is not evidence of a heap-free application.
- `USB_DEVICE_MIDI/Target/usbd_conf.h:96,99` maps `USBD_malloc/free` to `malloc/free`.
- `Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c:729` allocates class data; `:731–734` checks NULL and returns failure; `:786–790` frees it during deinitialization.
- `OTG_FS_IRQHandler` (`Core/Src/stm32f7xx_it.c:376ff`, line positions include diagnostic additions) calls HAL_PCD_IRQHandler. Setup callback (`USB_DEVICE_MIDI/Target/usbd_conf.c:136–139`) calls USBD_LL_SetupStage, which dispatches standard requests. `usbd_ctlreq.c:585,603,609,613,628` calls class config/clear; `usbd_core.c:224,247` dispatches class initialization/deinitialization. These provide an interrupt-context allocation/free path during configuration changes.
- The linked `__malloc_lock` and `__malloc_unlock` each disassemble to `bx lr`: they provide no allocator serialization. Main-context formatting can therefore overlap USB allocator operations. Allocator metadata and the program break are not reentrant under that overlap.

HYPOTHESIS: such an overlap can corrupt allocator state or duplicate an allocation; no such crash or exact interrupt interleaving was reproduced. CODE confirms the unsafe paths/absent lock, not an observed failure. This is an **unresolved deployment blocker**, unchanged rather than fixed by the range guard. No new allocation or formatting call was added to an interrupt handler.

A complete remedy would require a separately reviewed allocator/USB-lifetime design, such as validated fixed class storage, or an allocator serialization strategy with bounded and measured interrupt latency. Adding IRQ masking/allocator locks or changing class initialization here would exceed the authorized timing/behavior constraints. That part is deliberately stopped at investigation/design, not silently implemented. Do not deploy or declare concurrent allocator use safe because boundary tests pass.

## 4. Stack safety and design-only monitoring

CODE: Runtime MSP starts at 0x20020000 and grows downward in DTCM. The vector initially names 0x20080000 for boot entry; startup then moves it. No PSP use or RTOS stack switching was established. The linker `_Min_Stack_Size=0x400` and DTCM assertion reserve only 1,024 bytes, not worst-case consumption. Static DTCM ends at 107,392 bytes, leaving 23,680 bytes for runtime stack and margin. No Phase B DTCM object is added.

Interrupt priorities/nesting remain unchanged. TIM2 renders and services MIDI; main audio service masks TIM2 around its sequence/render work. Other higher-priority timer/USB/storage events can preempt appropriate contexts. Cortex-M7 exception stacking includes the basic 8-word frame, alignment padding and potentially floating-point extended context (18 additional words) plus software frames; lazy FP stacking must be accounted for. A minimum stack assertion is not a nested-call proof.

BUILD: Both normal and instrumented application compile-command sets were analyzed in isolated `/tmp/b-stack-off` and `/tmp/b-stack-on` with `-fstack-usage -Wstack-usage=2048`. All 78 application translation units compiled. Largest fixed frames remain: formatting routine 1,096 bytes, mixer 448, saveProject 416, loadProject 384. Variable-size arrays in DrumVoice, CymbalVoice, HiHat, Snare, Compressor and Ringmod retain six baseline compiler warnings. Known audio calls pass 64 samples, but whole-program library/nested-interrupt bounds were not proven.

Measured static frames from compiler output (not runtime measurements):

| Function | Diagnostics off | On |
|---|---:|---:|
| `_sbrk` |8|8|
| `soundengine_calcNextSampleBlock` |8|16|
| TIM2 handler |8|8|
| I2S completion callback |16|16|
| `lxr_diag_init` |absent|4|
| `lxr_diag_record` / `now` / `rearm` |absent|0|
| `lxr_diag_callback` |absent|16|

These frames are not additive worst-case totals by themselves. Euclidean recursion is limited by supported small inputs; directory deletion remains recursive with nesting and storage waits remain latency risks.

**Not implemented:** no DTCM fill, stack canary, active-frame scan or linker/startup edit. Although the static lower boundary is known, safe initialization relative to live C frames and nested interrupts is not unambiguously established for a watermark routine. The task explicitly requires stopping that implementation rather than guessing.

Proposed future strategy: after a separately reviewed safe initialization point, paint only proven-unused space between aligned static DTCM end and a conservatively bounded live-stack floor, never current frames; retain a guard margin for initialization calls/interrupts. Validate MSP/PSP selection and FP stacking. Scan incrementally outside audio and faults, comparing against measured minimum SP, and preserve evidence across scheduling transitions. An early startup stub or linker-defined monitored region would need separate authorization/review. Stress testing alone cannot prove worst-case depth. No high-water number is reported as measured.

## 5. Optional runtime diagnostic implementation

### Build separation and disabled behavior

New `LXR_RUNTIME_DIAGNOSTICS` CMake option defaults OFF. The header defaults to 0 when no definition is supplied. Existing Release/Debug presets are unchanged. CMake adds one platform source (`Core/Src/safety/runtimeDiagnostics.c`); when disabled it contributes no diagnostic state/functions. Only bounded heap behavior changes in normal firmware.

BUILD: All three edited Release audio/IRQ object disassemblies and relocations were compared with saved Phase A objects and matched exactly when diagnostics were off:

```text
PASS Soundengine.c instructions/relocations identical to Phase A
PASS cs4344_cs5343.c instructions/relocations identical to Phase A
PASS stm32f7xx_it.c instructions/relocations identical to Phase A
```

No `lxr_diag*` symbol exists in the normal ELF. Host disabled tests confirm diagnostic arguments are not evaluated and the original DMA rearm call executes exactly once. This is stronger evidence than merely assuming an empty macro is harmless. No interrupt priority, buffer size, routing, sequencer/MIDI algorithm or clock was changed.

### Enabled metrics, state and measurement points

`Core/Inc/safety/runtimeDiagnostics.h` defines a 72-byte fixed volatile structure, and the source supplies flash-resident helpers. No heap, printf, file I/O, locks, waits, ring buffers or new interrupt is used. Small call sites and link veneers account for the instrumented ITCM increase; helpers themselves are outside ITCM.

- Initialization in `soundengine_init` enables CoreDebug trace and DWT CYCCNT, unlocks the DWT register interface and checks NOCYCCNT/enable bits. It does not reset CYCCNT or change CPU clocks. This optional debug-peripheral configuration is absent from normal firmware. Register support/actual counting behavior still need physical validation.
- Render timing surrounds actual mixer work through buffer-readiness publication in `Soundengine.c:23ff`; no sample count, audio math or routing changes.
- Timer timing surrounds sequence/render/UART work in TIM2 (`stm32f7xx_it.c:269ff`), not complete IRQ entry/exit, GPIO calls, HAL completion or final statistics-update time.
- Per-stream callback observations are taken in SPI2/SPI3 branches (`cs4344_cs5343.c:94ff`); rearm instrumentation records the original HAL return status without changing the rearm call or its order.

`lxr_diagnostics.version=1` is a schema identifier, not a firmware SHA. Matching ELF/source commit is the build identifier for external observation. `render` and `timer_isr` hold timed count, maximum duration and over-budget count. `dma[0]` is SPI2, `[1]` SPI3, with callback count, not-ready observations, non-OK HAL rearm returns, long intervals and last timestamp.

All event counts saturate at UINT32_MAX. Cycle differences use unsigned modulo subtraction, valid for an interval shorter than one full CYCCNT wrap (~19.9 seconds at nominal 216 MHz). The threshold is a clearly named nominal 313,181-cycle block budget (64 frames at 44,140.625 Hz); it is not a measured deadline. Long callbacks mean an interval exceeding twice that nominal period. No 64-bit sum or average is kept. When CYCCNT is unavailable, timed metrics are skipped; callback/readiness/rearm observations remain available. A zero timed count with enable flag false must not be read as zero workload.

State ownership follows the existing scheduling: render from TIM2 or main with TIM2 masked, timer metric from TIM2 only, each DMA stream's counters from same-priority callbacks. Fields are 32-bit volatile observations; no atomic multi-field snapshot/reset API exists. Later debugger reads may mix epochs; do not clear counters while audio runs. If scheduling/owners change later, this ownership assumption must be reviewed.

### What is and is not detected

CODE: readiness counts reflect the shared flag at each stream callback, not a proven analog underrun. Existing global error behavior remains untouched. HAL non-OK return counts detect rearm failure reports, not every hardware DMA transfer error. Callback intervals reveal long observed separations; they do not prove a missed physical callback. Render/ISR over-budget counts show elapsed spans above a nominal threshold, not request-to-ready deadline misses.

Not implemented: generation sequence identities, exact repeated/missed buffer detection, request-to-completion latency, complete hardware DMA-error aggregation, or analog audio capture. Counts of renders versus callbacks can suggest a problem but cannot disambiguate startup, stream ordering and intentional service phases. A hang never produces an end timestamp, and intervals longer than a full counter wrap alias. These gaps are explicit, not labeled confirmed underruns.

AN4667, as cited in the task, documents STM32F7 DTCM accessibility through Cortex-M7 AHBS. DTCM placement alone is **not** evidence of invalid DMA configuration. The unchanged audio arrays remain DTCM halfword buffers at the mapped addresses; architectural access does not prove actual stream setup, data freshness or runtime behavior. F7/H7 access assumptions must not be conflated.

### Overhead, calibration and safe observation

Instrumentation is O(1) with fixed counters and flash calls. Expected overhead is a small fixed sequence of register reads, branches, saturating updates and function calls; cache misses, veneers and preemption make an exact cycle estimate uncertain. Tens to hundreds of cycles per hook under favorable cache conditions is only an engineering expectation, not a benchmark or an established upper bound. Callback hooks delay rearming slightly when enabled, and statistics-update overhead is not wholly included in the reported duration. Full request-to-ready timing and an empty-hook calibration remain necessary.

Future authorized testing should measure empty hooks and actual clocks, cross-check the existing GPIO pulse with an analyzer, stress all voices/FX/Morph/MIDI/Euclidean/storage combinations, correlate both output streams, and compare instrumented and normal builds. Require a reviewed margin below the actual block deadline and investigate every unexplained event; no average CPU load or reliability claim follows from these counters. DWT may pause under debug or power states. No physical performance, stack high-water, CPU utilization or thermal measurement was made.

Counters can eventually be inspected with an explicitly authorized debugger and matching ELF. No UI, serial protocol, SD log or binary download endpoint was added. Debugger attachment/halting itself can disturb timing and is not authorized by this task.

## 6. Fault diagnostics and recovery: reviewed design only

The existing HardFault, MemManage, BusFault and UsageFault handlers remain infinite loops; DEBUG HardFault retains CFSR-derived local variables only. No active watchdog initialization/refresh was established. The handler bodies, reset behavior and hardware initialization files remain unchanged.

No fault-capture shim or persistent crash storage was added. Safe stacked-PC/LR recovery requires EXC_RETURN decoding (MSP/PSP, basic/extended FP frame), validation of stack range and fault state, and a capture path safe even if the ordinary stack is corrupt or initialization has not completed. A naked entry shim, dedicated stack or noinit record would affect exception entry/startup/linker assumptions; those are deliberately outside this implementation.

Proposed record: magic/schema/validity checksum, immutable diagnostic build ID, reset cause read from RCC without clearing it prematurely, CFSR/HFSR/DFSR/AFSR/MMFAR/BFAR/SHCSR, MSP/PSP/EXC_RETURN, stacked PC/LR/xPSR only after safe validation, and a small snapshot of audio counters. Capture first fault once, with no allocation, formatting, SD I/O or automatic reset. A plain BSS record lasts only until reset; persistent retention requires separately established section lifetime and startup behavior. No such persistence claim is made. A watchdog must not be enabled merely to hide hangs; recovery design must first address storage and retained fault evidence.

## 7. Tests, builds and generated image validation

All actual normal/sanitizer builds/tests described here passed. No software group was skipped. Hardware, stack painting, persistent fault capture and watchdog execution were deliberately not performed, not silently omitted.

Baseline and final suites:

```sh
python3 tests/bug001/run.py
python3 tests/euclidean/run.py
python3 tests/safety/run.py
```

The two unchanged fix suites retain ASan/UBSan. New tests use native GCC with `-Wall -Wextra -Werror`, ASan/UBSan, `-fno-sanitize-recover=all`, frame pointers, non-PIE, leak detection disabled and temporary executables. The heap test includes actual `sysmem.c` with host linker aliases to a bounded arena; the boundary helper also uses synthetic uintptr limits. The enabled diagnostic test compiles the actual production diagnostic source against fake DWT/CoreDebug registers. It does not execute target registers. Default-off evaluation behavior is tested separately.

New cases include exact limit/growth/shrink/failure, ENOMEM and unchanged break, 8-byte initial alignment/byte-granular moves, INT_MIN/MAX, uintptr overflow, 601 legal/illegal moves, cycle wraparound, threshold boundary, count saturation, no-CYCCNT behavior, simulated render/ISR/DMA events, invalid stream indices and rearm statuses.

Final exact stdout:

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
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state
```

Normal builds (OFF) were configured/built first. Instrumented builds use isolated ignored directories and the unchanged default toolchain mechanism:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cmake --preset release
cmake --build --preset release --parallel 4
cmake --preset debug
cmake --build --preset debug --parallel 4
cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_BUILD_TYPE=Release   -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin -DLXR_RUNTIME_DIAGNOSTICS=ON
cmake --build build/diagnostics-release --parallel 4
cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug   -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin -DLXR_RUNTIME_DIAGNOSTICS=ON
cmake --build build/diagnostics-debug --parallel 4
python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
```

All commands returned zero. Normal builds reused prior directories; instrumented directories were newly configured for this task. Both use the established -O3 audio settings. Four cache files explicitly show OFF/OFF/ON/ON. No normal ARM build warnings/errors occurred; separate static-stack warning output is retained. After final comment/review cleanup and exact source commit import, all suites and four pinned builds were repeated successfully. No source changed after the validated tree was published.

The checker confirmed each generated image's 16-byte LXRV2IMG header, padding, little-endian length, checksum, raw .bin identity, flash limits, plausible aligned stack/reset vectors and all allocated ELF VMA/LMA bounds. Filenames remain LXR_V2_V1.7.img and LXR_V2_STM32F765.bin/.elf/.map. No claim of official v1.71 behavioral compatibility is made.

Exact final checker records and SHA-256 metadata:

```json
[
  {
    "checksum": "0x74",
    "configuration": "release",
    "elf_sha256": "8b2e958c316d8e908d8eafe9f2eb480198629d45376edea311b847e03a69c11b",
    "image_bytes": 275652,
    "image_sha256": "d8968b8717b1ded7782027c1af848d52b4372dd575c6c38afa2cba3ff8c5b2e3",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 222772,
      "ITCM_RAM": 15016,
      "RAM": 271032
    },
    "payload_bytes": 275636,
    "reset_vector": "0x08014E45",
    "result": "PASS"
  },
  {
    "checksum": "0x5D",
    "configuration": "debug",
    "elf_sha256": "c71d0e3895c559eeb37545fbfdd7d279dce859c88ce93f1b1e67b652d5913186",
    "image_bytes": 275700,
    "image_sha256": "9bfc9c5e5391bc18538b1ef38e1679ea3b41236f00bd3b30efd78f87bf44b9cc",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 222820,
      "ITCM_RAM": 15016,
      "RAM": 271032
    },
    "payload_bytes": 275684,
    "reset_vector": "0x08014E45",
    "result": "PASS"
  },
  {
    "checksum": "0xB5",
    "configuration": "diagnostics-release",
    "elf_sha256": "d07a2cef6c75b2f4ba46e42d18ff7dd7ba5c2e627d27abd8f7a7c5e59e31fe1f",
    "image_bytes": 276092,
    "image_sha256": "96ae8f2b2eff0c46556da596845f1ae1eadf4053e831ee872229ed9e95d0d900",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223140,
      "ITCM_RAM": 15088,
      "RAM": 271104
    },
    "payload_bytes": 276076,
    "reset_vector": "0x08014E45",
    "result": "PASS"
  },
  {
    "checksum": "0x2B",
    "configuration": "diagnostics-debug",
    "elf_sha256": "18a6f61ffd44af66383e702a2aa35899f6fca680a5fd50913ad2cbb9f8d2541c",
    "image_bytes": 276140,
    "image_sha256": "e8a0ee0ca1c2a33ed5a6e1f6df7ae8e7a2b3c203a544b891c523679759c4cb9d",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223188,
      "ITCM_RAM": 15088,
      "RAM": 271104
    },
    "payload_bytes": 276124,
    "reset_vector": "0x08014E45",
    "result": "PASS"
  }
]
```

Raw-bin hashes:

```text
6d4cd30c76fd1cb2dbbd81800e709356155b3eb3421b0bceb80646e3080589d2  build/release/LXR_V2_STM32F765.bin
8e08865247af357ef8c2b464de5d511b7a582f75080e3a21032fe6edeba40af6  build/debug/LXR_V2_STM32F765.bin
30d36a29c01695b9fb6988ecee2244e53111a36ed556241caef3e32f22096209  build/diagnostics-release/LXR_V2_STM32F765.bin
721306422f3bc37ceb5a747cd9963eb82f498de7bf99b4585ada88dcc779b87e  build/diagnostics-debug/LXR_V2_STM32F765.bin
```

## 8. Memory impact and static budget

BUILD, bytes:

| Region/footprint | Phase A Release | Phase B normal | Phase B instrumented | Instrumented headroom |
|---|---:|---:|---:|---:|
| ITCM, capacity 16,384 |15,016|15,016|15,088|1,296|
| DTCM, capacity 131,072 |107,392|107,392|107,392|23,680 before runtime stack|
| RAM, capacity 393,216 |271,040|271,032|271,104|122,112 after static/minimum heap|
| Linker FLASH allocation, capacity 1,015,808 |223,552|223,624|223,992|791,816|
| Complete raw FLASH payload |275,564|275,636|276,076|739,732|

Normal headroom: ITCM 1,368; DTCM 23,680; RAM 122,184; full-payload FLASH 740,172 bytes. Instrumentation adds 72 ITCM bytes and 72 BSS bytes relative to the normal bounded-heap build; relative to Phase A, RAM adds 64 bytes because the bounded-heap linkage reduces baseline BSS by 8. Helpers are in flash; veneers/call-site growth consume the 72 ITCM bytes. This is within static regions, not evidence of acceptable real-time overhead.

Debug has identical ITCM/DTCM/RAM use, with 48 additional FLASH/payload bytes in each corresponding configuration. All linker assertions remain active. Linker FLASH reporting excludes some explicit TCM load-image extent; complete payload/LMAs were separately checked. Heap availability includes the minimum 512-byte reservation rather than confusing it with a cap. No worst-case runtime stack/heap usage is claimed.

## 9. Safety review answers and remaining risks

1. **Normal behavior preserved? CODE/BUILD:** default-off audio/IRQ instructions and relocations match Phase A; valid serialized allocator semantics are preserved. OOM/out-of-range growth now deliberately fails instead of escaping SRAM. Whole-system behavior with concurrent allocation is not proven.
2. **Conclusive correction? CODE/BUILD:** unbounded serialized program-break movement, invalid shrink and arithmetic wrap are prevented with tested ENOMEM behavior. This is not a complete allocator safety correction.
3. **Hypotheses?** Actual USB/main overlap causing corruption, actual overruns/audio artifacts and diagnostic timing acceptability remain unmeasured.
4. **Startup/linker/bootloader/clocks/flash? CODE:** unchanged byte-for-byte against Phase A, including main.c, startup assembly, linker script, system initialization, flash_if.c and HAL MSP/priority file. Only optional DWT setup is added to the enabled diagnostic mode. UPDATE_BOOTLOADER remains 0, application starts 0x08008000 and first 32 KiB remains excluded. Hardware protection state is not inspected.
5. **Memory impact? BUILD:** table above; no DTCM growth and no extra default-off audio work. Enabled helpers are outside ITCM.
6. **Default disabled? CODE/BUILD:** yes, option/header/cache/disassembly/symbol evidence agree.
7. **Instrumented static regions valid? BUILD:** yes; all four generated images validate.
8. **Additional risks? OPEN:** bounded instrumented callback latency, cache/DWT effects, saturation/wrap limitations and non-atomic debugger snapshots; allocator reentrancy remains the major deployment blocker. No diagnostic lock/IRQ change is introduced.
9. **Observation interface?** Matching-ELF debugger access to fixed volatile lxr_diagnostics only, under later explicit authorization. Fault/stack records are not implemented.
10. **Before first deployment?** Resolve allocator ownership/reentrancy with evidence and tests, review OOM handling, establish stack margins and safe fault/recovery design, calibrate instrumentation and actual clocks, then obtain explicit hardware authorization with validated backups/recovery plan. A successful build alone grants none of that authorization.

No deployed reliability, maximum CPU load, physical stack high-water, timing margin, thermal performance or analog audio result is claimed. Source-format compatibility, official-source identity and runtime behavior remain separate questions.

## 10. Changes, errors and publication audit

Exact change list and full patch follow. Fourteen source/test/config files changed; original fix tests/source algorithms, startup/linker, hardware initialization and existing branches were preserved. The source branch has one parent, exact Phase A. The optional source-list and compile-definition changes are necessary to build the facility; presets are unchanged.

GitHub publication uses authenticated Git-data APIs rather than claiming a native git push. Every base64 blob SHA matched local bytes, the remote tree exactly matched 249fbc62, and remote commit/file/parent contents were read back. A hash-verified exact commit object was imported locally, leaving a clean checkout. No existing ref was force-updated. The separate report is based on lab-setup and adds one unique Markdown file; final verification checks exact content, branch heads and preservation of every pre-existing branch.

Errors/limitations: some broad search outputs were truncated and focused reads followed. The newly traced USB allocation path corrected the earlier Core-only inference; documentation now states the unresolved issue explicitly. No test/compiler/image failure was suppressed. Static stack-analysis warnings remain as baseline evidence, not hidden passes or proven overflows. No runtime allocator stress/fault injection occurred on target. No additional timing/USB allocator fix was improvised.

Recommended next step: review the bounded-heap contract and serious USB allocator concurrency finding first. Authorize a separately scoped solution and test strategy before physical validation. Retain B1 as reviewed software work, not a deployment candidate.

## Appendix A. Complete source/test/configuration diff

Existing source contexts are quoted as evidence. The actual Git blobs retain CRLF where present; display line endings below are normalized only in this Markdown report.

```diff
diff --git a/CMakeLists.txt b/CMakeLists.txt
index 7991457..af49d28 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -69,6 +69,11 @@ target_compile_options(lxr_board_config INTERFACE
     $<$<COMPILE_LANGUAGE:C>:-ffast-math -freciprocal-math -mfp16-format=ieee -fomit-frame-pointer>
 )
 
+option(LXR_RUNTIME_DIAGNOSTICS "Enable optional DWT/audio diagnostics (no hardware validation implied)" OFF)
+if(LXR_RUNTIME_DIAGNOSTICS)
+    target_compile_definitions(lxr_board_config INTERFACE LXR_RUNTIME_DIAGNOSTICS=1)
+endif()
+
 add_subdirectory(Drivers)
 add_subdirectory(Middlewares)
 add_subdirectory(Core)
diff --git a/Core/CMakeLists.txt b/Core/CMakeLists.txt
index 0214530..c78247d 100644
--- a/Core/CMakeLists.txt
+++ b/Core/CMakeLists.txt
@@ -26,6 +26,7 @@ add_library(lxr_platform OBJECT
     Src/system_stm32f7xx.c
     Src/syscalls.c
     Src/sysmem.c
+    Src/safety/runtimeDiagnostics.c
     Src/delay.c
     Src/flash_if.c
 )
diff --git a/Core/Inc/safety/heapBounds.h b/Core/Inc/safety/heapBounds.h
new file mode 100644
index 0000000..6f7d75b
--- /dev/null
+++ b/Core/Inc/safety/heapBounds.h
@@ -0,0 +1,20 @@
+#ifndef LXR_HEAP_BOUNDS_H
+#define LXR_HEAP_BOUNDS_H
+#include <stdint.h>
+/* Byte-granular sbrk semantics; malloc owns chunk alignment. No pointer arithmetic. */
+static inline int lxr_heap_move(uintptr_t start, uintptr_t limit, uintptr_t *current, int increment)
+{
+    uintptr_t amount;
+    if(start > limit || *current < start || *current > limit) return 0;
+    if(increment >= 0) {
+        amount = (uintptr_t)increment;
+        if(amount > limit - *current) return 0;
+        *current += amount;
+    } else {
+        amount = (uintptr_t)(-(increment + 1)) + 1u; /* also defined for INT_MIN */
+        if(amount > *current - start) return 0;
+        *current -= amount;
+    }
+    return 1;
+}
+#endif
diff --git a/Core/Inc/safety/runtimeDiagnostics.h b/Core/Inc/safety/runtimeDiagnostics.h
new file mode 100644
index 0000000..456855d
--- /dev/null
+++ b/Core/Inc/safety/runtimeDiagnostics.h
@@ -0,0 +1,43 @@
+#ifndef LXR_RUNTIME_DIAGNOSTICS_H
+#define LXR_RUNTIME_DIAGNOSTICS_H
+#include <stdint.h>
+#ifndef LXR_RUNTIME_DIAGNOSTICS
+#define LXR_RUNTIME_DIAGNOSTICS 0
+#endif
+#if LXR_RUNTIME_DIAGNOSTICS
+/* Nominal 64-frame period at 216 MHz / 44140.625 Hz; not a measured deadline. */
+#define LXR_DIAG_BUDGET 313181u
+typedef struct { uint32_t count, max_cycles, over_budget; } LxrDiagMetric;
+typedef struct { uint32_t callbacks, not_ready, rearm_errors, long_intervals, last_cycle; } LxrDiagDma;
+typedef struct {
+    uint32_t version, cycle_counter_enabled;
+    LxrDiagMetric render, timer_isr;
+    LxrDiagDma dma[2];
+} LxrDiagnostics;
+extern volatile LxrDiagnostics lxr_diagnostics;
+static inline void lxr_diag_increment(volatile uint32_t *value) { if(*value != UINT32_MAX) ++*value; }
+static inline void lxr_diag_metric(volatile LxrDiagMetric *metric, uint32_t begin, uint32_t end)
+{
+    uint32_t duration = end - begin; /* one CYCCNT wrap is supported */
+    lxr_diag_increment(&metric->count);
+    if(duration > metric->max_cycles) metric->max_cycles = duration;
+    if(duration > LXR_DIAG_BUDGET) lxr_diag_increment(&metric->over_budget);
+}
+void lxr_diag_init(void);
+uint32_t lxr_diag_now(void);
+void lxr_diag_record(unsigned metric, uint32_t begin);
+void lxr_diag_callback(unsigned stream, unsigned ready);
+void lxr_diag_rearm(unsigned stream, unsigned status);
+#define LXR_DIAG_INIT() lxr_diag_init()
+#define LXR_DIAG_START(name) uint32_t name = lxr_diag_now()
+#define LXR_DIAG_END(metric, name) lxr_diag_record(metric, name)
+#define LXR_DIAG_CALLBACK(stream, ready) lxr_diag_callback(stream, ready)
+#define LXR_DIAG_REARM(stream, call) lxr_diag_rearm(stream, (unsigned)(call))
+#else
+#define LXR_DIAG_INIT() ((void)0)
+#define LXR_DIAG_START(name)
+#define LXR_DIAG_END(metric, name) ((void)0)
+#define LXR_DIAG_CALLBACK(stream, ready) ((void)0)
+#define LXR_DIAG_REARM(stream, call) (call)
+#endif
+#endif
diff --git a/Core/Src/Soundengine/Soundengine.c b/Core/Src/Soundengine/Soundengine.c
index 041f0d2..16de43d 100644
--- a/Core/Src/Soundengine/Soundengine.c
+++ b/Core/Src/Soundengine/Soundengine.c
@@ -11,6 +11,7 @@
  * Public License version 3, with the additional permission and terms in
  * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
  */
+#include "safety/runtimeDiagnostics.h"
 #include "Soundengine/Soundengine.h"
 #include "Soundengine/AudioCodecManager.h"
 #include "Preset/PresetManager.h"
@@ -22,12 +23,14 @@ uint8_t soundengine_calcNextSampleBlock()
 {
 	if(bCurrentSampleValid!= SAMPLE_VALID)
 	{
+		LXR_DIAG_START(render_begin);
 	#if USE_DAC2
 		mixer_calcNextSampleBlock((int16_t*)&dma_buffer[bCurrentSampleValid*(OUTPUT_DMA_SIZE*2)],(int16_t*)&dma_buffer2[bCurrentSampleValid*(OUTPUT_DMA_SIZE*2)]);
 	#else
 		mixer_calcNextSampleBlock(&dma_buffer[bCurrentSampleValid*(OUTPUT_DMA_SIZE*2)],&dma_buffer2[(1-bCurrentSampleValid)*(OUTPUT_DMA_SIZE*2)]);
 	#endif
 		bCurrentSampleValid = SAMPLE_VALID;
+		LXR_DIAG_END(0, render_begin);
 
 		return 1;
 	}
@@ -37,6 +40,7 @@ uint8_t soundengine_calcNextSampleBlock()
 
 void soundengine_init()
 {
+	LXR_DIAG_INIT();
 	mixer_init();
 
 
diff --git a/Core/Src/Soundengine/cs4344_cs5343.c b/Core/Src/Soundengine/cs4344_cs5343.c
index 7dbc1eb..4095b82 100644
--- a/Core/Src/Soundengine/cs4344_cs5343.c
+++ b/Core/Src/Soundengine/cs4344_cs5343.c
@@ -10,6 +10,7 @@
  */
 
 
+#include "safety/runtimeDiagnostics.h"
 #include "Soundengine/cs4344_cs5343.h"
 #include "Soundengine/globals.h"
 #include "main.h"
@@ -93,6 +94,7 @@ IN_ITCM_RAM void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
 
 	if(hi2s->Instance == SPI2)
 	{
+		LXR_DIAG_CALLBACK(0, bCurrentSampleValid == SAMPLE_VALID);
 
 		//Master Out
 		dmaPtr2++;
@@ -113,7 +115,7 @@ IN_ITCM_RAM void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
 
 		   /* Enable the I2S DMA Stream*/
 		   //DMA_Cmd(DMA1_Stream7, ENABLE);
-		   HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t*)&dma_buffer2[(OUTPUT_DMA_SIZE*2)*(dmaPtr2&0x1)], OUTPUT_DMA_SIZE*2);
+		   LXR_DIAG_REARM(0, HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t*)&dma_buffer2[(OUTPUT_DMA_SIZE*2)*(dmaPtr2&0x1)], OUTPUT_DMA_SIZE*2));
 
 
 #if !SWITCH_FLAG
@@ -127,9 +129,10 @@ IN_ITCM_RAM void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
 	}
 	else if(hi2s->Instance == SPI3)
 	{
+		LXR_DIAG_CALLBACK(1, bCurrentSampleValid == SAMPLE_VALID);
 		//Out 2
 		dmaPtr++;
-		HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)&dma_buffer[(OUTPUT_DMA_SIZE*2)*(dmaPtr&0x1)], OUTPUT_DMA_SIZE*2);
+		LXR_DIAG_REARM(1, HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)&dma_buffer[(OUTPUT_DMA_SIZE*2)*(dmaPtr&0x1)], OUTPUT_DMA_SIZE*2));
 
 #if SWITCH_FLAG
 		   //now start next sample block calculation
diff --git a/Core/Src/safety/runtimeDiagnostics.c b/Core/Src/safety/runtimeDiagnostics.c
new file mode 100644
index 0000000..119ac6f
--- /dev/null
+++ b/Core/Src/safety/runtimeDiagnostics.c
@@ -0,0 +1,39 @@
+#include "safety/runtimeDiagnostics.h"
+#if LXR_RUNTIME_DIAGNOSTICS
+#include "stm32f7xx_hal.h"
+/* Flash-resident helpers; no ITCM section, allocation, I/O, resets or locks. */
+volatile LxrDiagnostics lxr_diagnostics;
+void lxr_diag_init(void)
+{
+    lxr_diagnostics.version = 1;
+    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
+    DWT->LAR = 0xC5ACCE55u;
+    if(!(DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk)) {
+        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
+        lxr_diagnostics.cycle_counter_enabled = !!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk);
+    }
+    /* Preserve CYCCNT rather than resetting any other user's counter. */
+}
+uint32_t lxr_diag_now(void) { return DWT->CYCCNT; }
+void lxr_diag_record(unsigned metric, uint32_t begin)
+{
+    if(!lxr_diagnostics.cycle_counter_enabled) return;
+    volatile LxrDiagMetric *m = metric == 0 ? &lxr_diagnostics.render : &lxr_diagnostics.timer_isr;
+    lxr_diag_metric(m, begin, lxr_diag_now());
+}
+void lxr_diag_callback(unsigned stream, unsigned ready)
+{
+    if(stream >= 2) return;
+    volatile LxrDiagDma *d = &lxr_diagnostics.dma[stream];
+    uint32_t now = lxr_diag_now();
+    if(lxr_diagnostics.cycle_counter_enabled && d->callbacks && now - d->last_cycle > 2u * LXR_DIAG_BUDGET)
+        lxr_diag_increment(&d->long_intervals);
+    d->last_cycle = now;
+    lxr_diag_increment(&d->callbacks);
+    if(!ready) lxr_diag_increment(&d->not_ready);
+}
+void lxr_diag_rearm(unsigned stream, unsigned status)
+{
+    if(stream < 2 && status != 0) lxr_diag_increment(&lxr_diagnostics.dma[stream].rearm_errors);
+}
+#endif
diff --git a/Core/Src/stm32f7xx_it.c b/Core/Src/stm32f7xx_it.c
index 176f3c1..e2f2825 100644
--- a/Core/Src/stm32f7xx_it.c
+++ b/Core/Src/stm32f7xx_it.c
@@ -19,6 +19,7 @@
 /* USER CODE END Header */
 
 /* Includes ------------------------------------------------------------------*/
+#include "safety/runtimeDiagnostics.h"
 #include "main.h"
 #include "stm32f7xx_it.h"
 /* Private includes ----------------------------------------------------------*/
@@ -268,9 +269,11 @@ void TIM2_IRQHandler(void)
 //#ifndef DEBUG
 
   HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);
+	LXR_DIAG_START(timer_begin);
 	seq_tick();
 	soundengine_calcNextSampleBlock();
   uart_processMidi();
+  LXR_DIAG_END(1, timer_begin);
   HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
 
 
diff --git a/Core/Src/sysmem.c b/Core/Src/sysmem.c
index 94cd942..feeaebe 100644
--- a/Core/Src/sysmem.c
+++ b/Core/Src/sysmem.c
@@ -46,42 +46,28 @@
 *****************************************************************************
 */
 
-/* Includes */
+/* Runtime stack is in DTCM (Reset_Handler). General SRAM after _end is heap.
+ * _estack is the existing end-of-SRAM symbol, not the active runtime stack.
+ * This bounds the break only; concurrent newlib allocator access remains unresolved.
+ */
 #include <errno.h>
-#include <stdio.h>
+#include <stdint.h>
+#include <sys/types.h>
+#include "safety/heapBounds.h"
 
-/* Variables */
-extern int errno;
-register char * stack_ptr asm("sp");
-
-/* Functions */
-
-/**
- _sbrk
- Increase program data space. Malloc and related functions depend on this
-**/
-#if 0
-caddr_t _sbrk(int incr)
+caddr_t _sbrk(int increment)
 {
-	extern char end asm("end");
-	static char *heap_end;
-	char *prev_heap_end;
-
-	if (heap_end == 0)
-		heap_end = &end;
-
-	prev_heap_end = heap_end;
-	/*//does not work with CCM stack - removed as I have no idea ATM how to fix it
-	if (heap_end + incr > stack_ptr)
-	{
-		errno = ENOMEM;
-		return (caddr_t) -1;
-	}
-	*/
-
-	heap_end += incr;
-
-	return (caddr_t) prev_heap_end;
+    extern char _end, _estack;
+    static uintptr_t current;
+    uintptr_t start = (uintptr_t)&_end;
+    const uintptr_t limit = (uintptr_t)&_estack;
+    if(start > UINTPTR_MAX - 7u) { errno = ENOMEM; return (caddr_t)-1; }
+    start = (start + 7u) & ~(uintptr_t)7u;
+    if(current == 0) current = start;
+    const uintptr_t previous = current;
+    if(!lxr_heap_move(start, limit, &current, increment)) {
+        errno = ENOMEM;
+        return (caddr_t)-1;
+    }
+    return (caddr_t)previous;
 }
-#endif
-
diff --git a/tests/safety/README.md b/tests/safety/README.md
new file mode 100644
index 0000000..ca1e980
--- /dev/null
+++ b/tests/safety/README.md
@@ -0,0 +1,77 @@
+# Phase B bounded heap and optional diagnostics
+
+Run `python3 tests/safety/run.py` with native GCC/ASan/UBSan. Tests compile the
+actual `_sbrk` source with host linker-symbol aliases, and the actual diagnostic
+implementation with fake DWT registers. Tests never operate hardware. Existing
+Bug 001 and Euclidean suites remain required.
+
+The strong `_sbrk(int)` replaces the unbounded libnosys implementation. It uses
+existing `_end` and `_estack` symbols; startup moves the active MSP to DTCM before
+C initialization. SRAM from the 8-byte-aligned `_end` to exclusive `_estack` is
+available to the heap. No linker/startup change is made. The break remains byte
+granular, as required by sbrk; malloc owns chunk alignment. Invalid growth,
+shrink below start, corrupted bounds and integer wrap are rejected with
+`(caddr_t)-1` and `errno=ENOMEM`, leaving the break unchanged. This does not make
+newlib reentrant or make formatting on allocation failure safe. No allocator or
+formatting call is added to interrupt handlers. Existing USB configuration can allocate/free in USB interrupt context while main
+formatting may allocate. Newlib locks remain no-ops: concurrent allocator access
+is an unresolved deployment blocker. This patch bounds the program break only;
+it does not make allocator metadata or `_sbrk` calls reentrant. Do not enable
+concurrent allocation workloads without a separately reviewed solution.
+
+Diagnostics default OFF in CMake (`LXR_RUNTIME_DIAGNOSTICS`) and in the header.
+Normal presets remain unchanged. Example isolated build:
+
+```sh
+cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
+  -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin \
+  -DLXR_RUNTIME_DIAGNOSTICS=ON
+cmake --build build/diagnostics-release --parallel 4
+python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release
+```
+
+For Debug use a separate directory and `-DCMAKE_BUILD_TYPE=Debug`. Do not flash or
+upload the resulting binaries. Default-off hooks evaluate no diagnostic arguments;
+the original DMA rearm call still executes exactly once. New code uses fixed BSS
+and flash-resident helpers, with no heap, logging, locks or additional interrupts.
+
+When enabled, `lxr_diagnostics` can be inspected later via the matching ELF and an
+explicitly authorized debugger. There is no UI, serial command or file output.
+Fields are volatile 32-bit observations; a running multi-field debugger read is
+not an atomic snapshot. Do not zero counters while audio is running. Metrics have
+separate owners under existing scheduling: render is serviced with TIM2 masked in
+main context or by TIM2; timer metric is TIM2-only; DMA metrics belong to each
+same-priority callback. This assumes those original scheduling rules remain true.
+
+- `version`: diagnostic schema 1, not a firmware build ID.
+- `cycle_counter_enabled`: DWT supports/enables CYCCNT; not proof the clock runs
+  under every debug/security/power state. Initialization unlocks DWT and enables
+  trace/counting without resetting CYCCNT. Disabled builds touch no DWT register.
+- `render`: successful render count, maximum elapsed cycles, over-budget count.
+  Measured around the actual mixer work through readiness publication.
+- `timer_isr`: analogous metric around sequence/render/UART work, excluding GPIO
+  edges, IRQ entry/exit and the final statistics update.
+- `dma[0]` = SPI2, `dma[1]` = SPI3: callback count, not-ready observation count,
+  HAL rearm non-OK return count, intervals longer than two nominal block periods,
+  and last timestamp. The existing global error counter remains untouched.
+
+Counts saturate at UINT32_MAX. Durations use modulo-32-bit subtraction; intervals
+must be shorter than one full CYCCNT period (~19.9 seconds at 216 MHz). The nominal
+budget 313181 cycles assumes 216 MHz and 64 frames at 44140.625 Hz. It is a reporting
+threshold, not a measured hardware deadline. Metrics include preemption within the
+measurement span. No cycle sum/average is maintained, avoiding a 64-bit hot counter.
+
+The counters do not identify sample freshness, repeated generation identities,
+request-to-ready latency, every DMA hardware error, or confirmed analog underruns.
+A missing render caused by a hang does not produce an end timestamp. Readiness
+observations can reflect callback order rather than an audible fault. Distinguish
+rearm status from transfer errors. Generation sequence tracking, persistent crash
+capture and a safe stack watermark remain design-only.
+
+Before authorized hardware evaluation, calibrate empty hooks and DWT behavior,
+compare instrumented versus normal workloads, verify actual clock/deadline values,
+and correlate both streams with existing GPIO timing. Each hook is bounded O(1),
+but flash calls/cache misses and callback latency add unknown runtime cost; no
+performance measurements have been made. Require measured margin and investigate
+all unexplained events before deployment. Stack/fault diagnostics must be reviewed
+separately; never paint arbitrary DTCM or enable a watchdog as part of this test.
diff --git a/tests/safety/run.py b/tests/safety/run.py
new file mode 100644
index 0000000..b01ecc1
--- /dev/null
+++ b/tests/safety/run.py
@@ -0,0 +1,19 @@
+#!/usr/bin/env python3
+"""Native ASan/UBSan tests of the production allocator and diagnostic implementation."""
+from pathlib import Path
+import os
+import subprocess
+import tempfile
+ROOT=Path(__file__).resolve().parents[2]
+HERE=Path(__file__).resolve().parent
+with tempfile.TemporaryDirectory(prefix='lxr-safety-') as directory:
+    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
+    flags=[os.environ.get('CC','cc'),'-std=gnu11','-O1','-g','-fsanitize=address,undefined',
+           '-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie',
+           '-Wall','-Wextra','-Werror','-I'+str(HERE),'-I'+str(ROOT/'Core/Inc'),'-I'+str(ROOT)]
+    for name,source,more in [('heap','test_heap.c',[]),
+                            ('disabled','test_diagnostics.c',[]),
+                            ('enabled','test_diagnostics.c',['-DLXR_RUNTIME_DIAGNOSTICS=1',str(ROOT/'Core/Src/safety/runtimeDiagnostics.c')])]:
+        binary=Path(directory)/name
+        subprocess.run(flags+[str(HERE/source)]+more+['-o',str(binary)],check=True,cwd=ROOT,env=env)
+        subprocess.run([str(binary)],check=True,cwd=ROOT,env=env)
diff --git a/tests/safety/stm32f7xx_hal.h b/tests/safety/stm32f7xx_hal.h
new file mode 100644
index 0000000..d02c620
--- /dev/null
+++ b/tests/safety/stm32f7xx_hal.h
@@ -0,0 +1,13 @@
+#ifndef LXR_TEST_HAL_H
+#define LXR_TEST_HAL_H
+#include <stdint.h>
+typedef struct { uint32_t CTRL, CYCCNT, LAR; } TestDwt;
+typedef struct { uint32_t DEMCR; } TestDebug;
+extern TestDwt test_dwt;
+extern TestDebug test_debug;
+#define DWT (&test_dwt)
+#define CoreDebug (&test_debug)
+#define CoreDebug_DEMCR_TRCENA_Msk (1u << 24)
+#define DWT_CTRL_NOCYCCNT_Msk (1u << 25)
+#define DWT_CTRL_CYCCNTENA_Msk 1u
+#endif
diff --git a/tests/safety/test_diagnostics.c b/tests/safety/test_diagnostics.c
new file mode 100644
index 0000000..adde9b9
--- /dev/null
+++ b/tests/safety/test_diagnostics.c
@@ -0,0 +1,53 @@
+#include <assert.h>
+#include <stdint.h>
+#include <string.h>
+#include <stdio.h>
+#include "stm32f7xx_hal.h"
+#include "safety/runtimeDiagnostics.h"
+TestDwt test_dwt;
+TestDebug test_debug;
+#if LXR_RUNTIME_DIAGNOSTICS
+int main(void)
+{
+    test_dwt.CTRL=DWT_CTRL_NOCYCCNT_Msk;
+    lxr_diag_init(); assert(!lxr_diagnostics.cycle_counter_enabled);
+    lxr_diag_record(0,0); assert(!lxr_diagnostics.render.count);
+    test_dwt.CTRL=0; test_dwt.CYCCNT=123;
+    lxr_diag_init(); assert(lxr_diagnostics.cycle_counter_enabled && test_dwt.CYCCNT==123);
+    lxr_diag_metric(&lxr_diagnostics.render,UINT32_MAX-4,5);
+    assert(lxr_diagnostics.render.count==1 && lxr_diagnostics.render.max_cycles==10);
+    lxr_diag_metric(&lxr_diagnostics.render,0,LXR_DIAG_BUDGET);
+    assert(!lxr_diagnostics.render.over_budget);
+    test_dwt.CYCCNT=LXR_DIAG_BUDGET+1; lxr_diag_record(0,0);
+    assert(lxr_diagnostics.render.over_budget==1);
+    test_dwt.CYCCNT=40; lxr_diag_record(1,10);
+    assert(lxr_diagnostics.timer_isr.max_cycles==30);
+    lxr_diagnostics.render.count=UINT32_MAX; lxr_diagnostics.render.over_budget=UINT32_MAX;
+    lxr_diag_metric(&lxr_diagnostics.render,0,UINT32_MAX);
+    assert(lxr_diagnostics.render.count==UINT32_MAX && lxr_diagnostics.render.over_budget==UINT32_MAX);
+    test_dwt.CYCCNT=UINT32_MAX-4; lxr_diag_callback(0,1);
+    test_dwt.CYCCNT=5; lxr_diag_callback(0,0);
+    assert(lxr_diagnostics.dma[0].callbacks==2 && lxr_diagnostics.dma[0].not_ready==1);
+    assert(!lxr_diagnostics.dma[0].long_intervals);
+    test_dwt.CYCCNT=2*LXR_DIAG_BUDGET+6; lxr_diag_callback(0,1);
+    assert(lxr_diagnostics.dma[0].long_intervals==1);
+    lxr_diag_rearm(0,0); lxr_diag_rearm(0,1); lxr_diag_rearm(0,2);
+    assert(lxr_diagnostics.dma[0].rearm_errors==2);
+    lxr_diagnostics.dma[0].callbacks=UINT32_MAX; lxr_diagnostics.dma[0].not_ready=UINT32_MAX;
+    lxr_diagnostics.dma[0].rearm_errors=UINT32_MAX;
+    lxr_diag_callback(0,0); lxr_diag_rearm(0,1);
+    assert(lxr_diagnostics.dma[0].callbacks==UINT32_MAX && lxr_diagnostics.dma[0].not_ready==UINT32_MAX && lxr_diagnostics.dma[0].rearm_errors==UINT32_MAX);
+    lxr_diag_callback(2,0); lxr_diag_rearm(2,1);
+    puts("PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state");
+}
+#else
+int main(void)
+{
+    int value=0;
+    LXR_DIAG_INIT(); LXR_DIAG_START(unused);
+    LXR_DIAG_END(++value,unused); LXR_DIAG_CALLBACK(++value,++value);
+    LXR_DIAG_REARM(++value,++value);
+    assert(value==1 && test_debug.DEMCR==0);
+    puts("PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once");
+}
+#endif
diff --git a/tests/safety/test_heap.c b/tests/safety/test_heap.c
new file mode 100644
index 0000000..aef7b93
--- /dev/null
+++ b/tests/safety/test_heap.c
@@ -0,0 +1,41 @@
+#include <assert.h>
+#include <stdint.h>
+#include <limits.h>
+#include <errno.h>
+#include <stdio.h>
+unsigned char test_heap[256] __attribute__((aligned(8)));
+__asm__(".global test_end\n.set test_end,test_heap\n.global test_limit\n.set test_limit,test_heap+256\n");
+#define _end test_end
+#define _estack test_limit
+#include "Core/Src/sysmem.c"
+int main(void)
+{
+    uintptr_t base=(uintptr_t)test_heap;
+    assert((uintptr_t)_sbrk(0)==base && !(base&7));
+    assert((uintptr_t)_sbrk(1)==base);
+    assert((uintptr_t)_sbrk(7)==base+1);
+    assert((uintptr_t)_sbrk(248)==base+8);
+    errno=0; assert(_sbrk(1)==(caddr_t)-1 && errno==ENOMEM);
+    assert((uintptr_t)_sbrk(0)==base+256);
+    assert((uintptr_t)_sbrk(-256)==base+256);
+    assert((uintptr_t)_sbrk(0)==base);
+    assert(_sbrk(-1)==(caddr_t)-1 && errno==ENOMEM);
+    assert(_sbrk(INT_MIN)==(caddr_t)-1);
+    assert(_sbrk(INT_MAX)==(caddr_t)-1);
+    assert((uintptr_t)_sbrk(0)==base);
+    uintptr_t current=UINTPTR_MAX-7;
+    assert(!lxr_heap_move(UINTPTR_MAX-7,UINTPTR_MAX,&current,8));
+    assert(current==UINTPTR_MAX-7);
+    assert(lxr_heap_move(UINTPTR_MAX-7,UINTPTR_MAX,&current,7));
+    assert(current==UINTPTR_MAX);
+    assert(!lxr_heap_move(100,99,&current,0));
+    current=99; assert(!lxr_heap_move(100,200,&current,0));
+    current=201; assert(!lxr_heap_move(100,200,&current,-1));
+    for(int i=-300;i<=300;i++) {
+        current=128;
+        int valid=lxr_heap_move(0,256,&current,i);
+        assert(valid==(i>=-128 && i<=128));
+        assert(current==(uintptr_t)(valid?128+i:128));
+    }
+    puts("PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves");
+}

```

## Appendix B. Source file summary

```text
 CMakeLists.txt                       |  5 +++
 Core/CMakeLists.txt                  |  1 +
 Core/Inc/safety/heapBounds.h         | 20 ++++++++++
 Core/Inc/safety/runtimeDiagnostics.h | 43 ++++++++++++++++++++
 Core/Src/Soundengine/Soundengine.c   |  4 ++
 Core/Src/Soundengine/cs4344_cs5343.c |  7 +++-
 Core/Src/safety/runtimeDiagnostics.c | 39 ++++++++++++++++++
 Core/Src/stm32f7xx_it.c              |  3 ++
 Core/Src/sysmem.c                    | 56 ++++++++++----------------
 tests/safety/README.md               | 77 ++++++++++++++++++++++++++++++++++++
 tests/safety/run.py                  | 19 +++++++++
 tests/safety/stm32f7xx_hal.h         | 13 ++++++
 tests/safety/test_diagnostics.c      | 53 +++++++++++++++++++++++++
 tests/safety/test_heap.c             | 41 +++++++++++++++++++
 14 files changed, 344 insertions(+), 37 deletions(-)
```

## Appendix C. Allocator disassembly evidence

### b-sbrk-disassembly.log

```text

build/release/LXR_V2_STM32F765.elf:     file format elf32-littlearm


Disassembly of section .text:

08016214 <_sbrk>:
 8016214:	4b15      	ldr	r3, [pc, #84]	@ (801626c <_sbrk+0x58>)
 8016216:	f113 0f08 	cmn.w	r3, #8
 801621a:	b510      	push	{r4, lr}
 801621c:	d80e      	bhi.n	801623c <_sbrk+0x28>
 801621e:	4c14      	ldr	r4, [pc, #80]	@ (8016270 <_sbrk+0x5c>)
 8016220:	3307      	adds	r3, #7
 8016222:	6822      	ldr	r2, [r4, #0]
 8016224:	f023 0307 	bic.w	r3, r3, #7
 8016228:	b17a      	cbz	r2, 801624a <_sbrk+0x36>
 801622a:	4912      	ldr	r1, [pc, #72]	@ (8016274 <_sbrk+0x60>)
 801622c:	4291      	cmp	r1, r2
 801622e:	468c      	mov	ip, r1
 8016230:	bf28      	it	cs
 8016232:	4694      	movcs	ip, r2
 8016234:	4563      	cmp	r3, ip
 8016236:	d801      	bhi.n	801623c <_sbrk+0x28>
 8016238:	4291      	cmp	r1, r2
 801623a:	d209      	bcs.n	8016250 <_sbrk+0x3c>
 801623c:	f7f2 f864 	bl	8008308 <__errno>
 8016240:	230c      	movs	r3, #12
 8016242:	6003      	str	r3, [r0, #0]
 8016244:	f04f 30ff 	mov.w	r0, #4294967295	@ 0xffffffff
 8016248:	bd10      	pop	{r4, pc}
 801624a:	461a      	mov	r2, r3
 801624c:	6023      	str	r3, [r4, #0]
 801624e:	e7ec      	b.n	801622a <_sbrk+0x16>
 8016250:	2800      	cmp	r0, #0
 8016252:	db06      	blt.n	8016262 <_sbrk+0x4e>
 8016254:	1a89      	subs	r1, r1, r2
 8016256:	4281      	cmp	r1, r0
 8016258:	d3f0      	bcc.n	801623c <_sbrk+0x28>
 801625a:	1813      	adds	r3, r2, r0
 801625c:	4610      	mov	r0, r2
 801625e:	6023      	str	r3, [r4, #0]
 8016260:	bd10      	pop	{r4, pc}
 8016262:	4241      	negs	r1, r0
 8016264:	1ad3      	subs	r3, r2, r3
 8016266:	4299      	cmp	r1, r3
 8016268:	d9f7      	bls.n	801625a <_sbrk+0x46>
 801626a:	e7e7      	b.n	801623c <_sbrk+0x28>
 801626c:	200620b8 	.word	0x200620b8
 8016270:	20021b40 	.word	0x20021b40
 8016274:	20080000 	.word	0x20080000

Disassembly of section .itcm_text:

```

### b-sbrk-r.log

```text

build/release/LXR_V2_STM32F765.elf:     file format elf32-littlearm


Disassembly of section .text:

08009430 <_sbrk_r>:
 8009430:	b538      	push	{r3, r4, r5, lr}
 8009432:	4d07      	ldr	r5, [pc, #28]	@ (8009450 <_sbrk_r+0x20>)
 8009434:	2200      	movs	r2, #0
 8009436:	4604      	mov	r4, r0
 8009438:	4608      	mov	r0, r1
 800943a:	602a      	str	r2, [r5, #0]
 800943c:	f00c feea 	bl	8016214 <_sbrk>
 8009440:	1c43      	adds	r3, r0, #1
 8009442:	d000      	beq.n	8009446 <_sbrk_r+0x16>
 8009444:	bd38      	pop	{r3, r4, r5, pc}
 8009446:	682b      	ldr	r3, [r5, #0]
 8009448:	2b00      	cmp	r3, #0
 800944a:	d0fb      	beq.n	8009444 <_sbrk_r+0x14>
 800944c:	6023      	str	r3, [r4, #0]
 800944e:	bd38      	pop	{r3, r4, r5, pc}
 8009450:	200204b8 	.word	0x200204b8

Disassembly of section .itcm_text:

```

### b-malloc-lock.log

```text

build/release/LXR_V2_STM32F765.elf:     file format elf32-littlearm


Disassembly of section .text:

08009280 <__malloc_lock>:
 8009280:	4770      	bx	lr

Disassembly of section .itcm_text:

build/release/LXR_V2_STM32F765.elf:     file format elf32-littlearm


Disassembly of section .text:

08009284 <__malloc_unlock>:
 8009284:	4770      	bx	lr

Disassembly of section .itcm_text:

```

## Appendix D. Static stack analysis output (normal and instrumented)

### b-stack-off.txt

```text
Compiled 78 application translation units with unchanged Release flags plus -fstack-usage.
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

### b-stack-on.txt

```text
Compiled 78 application translation units with unchanged Release flags plus -fstack-usage.
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

## Appendix E. Build logs

Stock installation instructions printed by the build are omitted; none was executed. Baseline tools and each actual configure/build result are retained.

### b-baseline-tools.log (exit 0)

```text
cmake version 3.31.10

CMake suite maintained and supported by Kitware (kitware.com/cmake).
1.13.0.git.kitware.jobserver-pipe-1
Python 3.12.14
arm-none-eabi-gcc (15:14.2.rel1-1) 14.2.1 20241119
Copyright (C) 2024 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


```

### b-release-config.log (exit 0)

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

### b-release-build.log (exit 0)

```text
[1/7] Building C object Core/CMakeFiles/lxr_platform.dir/Src/safety/runtimeDiagnostics.c.obj
[2/7] Building C object Core/CMakeFiles/lxr_platform.dir/Src/sysmem.c.obj
[3/7] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[4/7] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[5/7] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[6/7] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271032 B       384 KB     68.93%
           FLASH:      223624 B       992 KB     22.01%
   text	   data	    bss	    dec	    hex	filename
 274776	    852	 340568	 616196	  96704	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 275636 bytes, checksum 0x74
[7/7] 
```

### b-debug-config.log (exit 0)

```text
Preset CMake variables:

  CMAKE_BUILD_TYPE="Debug"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.3s)
-- Generating done (0.1s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/debug

```

### b-debug-build.log (exit 0)

```text
[1/7] Building C object Core/CMakeFiles/lxr_platform.dir/Src/safety/runtimeDiagnostics.c.obj
[2/7] Building C object Core/CMakeFiles/lxr_platform.dir/Src/sysmem.c.obj
[3/7] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[4/7] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[5/7] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[6/7] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271032 B       384 KB     68.93%
           FLASH:      223672 B       992 KB     22.02%
   text	   data	    bss	    dec	    hex	filename
 274824	    852	 340568	 616244	  96734	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 275684 bytes, checksum 0x5D
[7/7] 
```

### b-inst-release-config.log (exit 0)

```text
-- The C compiler identification is GNU 14.2.1
-- The ASM compiler identification is GNU
-- Found assembler: /workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

```

### b-inst-release-build.log (exit 0)

```text
[1/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c.obj
[2/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c.obj
[3/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c.obj
[4/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c.obj
[5/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c.obj
[6/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c.obj
[7/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c.obj
[8/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c.obj
[9/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c.obj
[10/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c.obj
[11/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c.obj
[12/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c.obj
[13/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c.obj
[14/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c.obj
[15/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c.obj
[16/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c.obj
[17/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c.obj
[18/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c.obj
[19/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c.obj
[20/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c.obj
[21/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj
[22/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c.obj
[23/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c.obj
[24/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c.obj
[25/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c.obj
[26/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c.obj
[27/123] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[28/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj
[29/123] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff_gen_drv.c.obj
[30/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c.obj
[31/123] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/option/syscall.c.obj
[32/123] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/App/fatfs.c.obj
[33/123] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[34/123] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/fatfs_platform.c.obj
[35/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c.obj
[36/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[37/123] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[38/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[39/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[40/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usb_device.c.obj
[41/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_desc.c.obj
[42/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_cdc_if.c.obj
[43/123] Building ASM object Core/CMakeFiles/lxr_platform.dir/Startup/startup_stm32f765vgtx.s.obj
[44/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[45/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/Target/usbd_conf.c.obj
[46/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[47/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[48/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/syscalls.c.obj
[49/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_hal_msp.c.obj
[50/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/sysmem.c.obj
[51/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/system_stm32f7xx.c.obj
[52/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/delay.c.obj
[53/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/safety/runtimeDiagnostics.c.obj
[54/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/flash_if.c.obj
[55/123] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/adcPots.c.obj
[56/123] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/DIO.c.obj
[57/123] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/uart.c.obj
[58/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/encoder.c.obj
[59/123] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[60/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/lcd.c.obj
[61/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/graphic.c.obj
[62/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/Parameters.c.obj
[63/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[64/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/frontPanelParser.c.obj
[65/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/ledHandler.c.obj
[66/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/Cc2Text.c.obj
[67/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/copyClearTools.c.obj
[68/123] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/initKit.c.obj
[69/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/screensaver.c.obj
[70/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/1PoleLp.c.obj
[71/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/automationNode.c.obj
[72/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/BufferTools.c.obj
[73/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Decay.c.obj
[74/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/distortion.c.obj
[75/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/CymbalVoice.c.obj
[76/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/dither.c.obj
[77/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/DrumVoice.c.obj
[78/123] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[79/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/HiHat.c.obj
[80/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/lfo.c.obj
[81/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[82/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/mixer.c.obj
[83/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/modulationNode.c.obj
[84/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Samples.c.obj
[85/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/random.c.obj
[86/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/SlopeEg2.c.obj
[87/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/ResonantFilter.c.obj
[88/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/snapEg.c.obj
[89/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/squareRootLut.c.obj
[90/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Snare.c.obj
[91/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Oscillator.c.obj
[92/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/TiltEq.c.obj
[93/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientTables.c.obj
[94/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientGenerator.c.obj
[95/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Compressor.c.obj
[96/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/wavetable.c.obj
[97/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c.obj
[98/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Reverb.c.obj
[99/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Ringmod.c.obj
[100/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c.obj
[101/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c.obj
[102/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Delay.c.obj
[103/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c.obj
[104/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Distortion.c.obj
[105/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c.obj
[106/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/AudioCodecManager.c.obj
[107/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FIFO.c.obj
[108/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[109/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/TriggerOut.c.obj
[110/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiMessages.c.obj
[111/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FxBus.c.obj
[112/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[113/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[114/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiVoiceControl.c.obj
[115/123] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/clockSync.c.obj
[116/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/Uart.c.obj
[117/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[118/123] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[119/123] Building C object Core/CMakeFiles/lxr_usb_app.dir/Src/Soundengine/USB/usb_manager.c.obj
[120/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/ParameterArray.c.obj
[121/123] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[122/123] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271104 B       384 KB     68.95%
           FLASH:      223992 B       992 KB     22.05%
   text	   data	    bss	    dec	    hex	filename
 275216	    852	 340640	 616708	  96904	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 276076 bytes, checksum 0xB5
[123/123] 
```

### b-inst-debug-config.log (exit 0)

```text
-- The C compiler identification is GNU 14.2.1
-- The ASM compiler identification is GNU
-- Found assembler: /workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

```

### b-inst-debug-build.log (exit 0)

```text
[1/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c.obj
[2/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c.obj
[3/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c.obj
[4/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c.obj
[5/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c.obj
[6/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c.obj
[7/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c.obj
[8/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c.obj
[9/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c.obj
[10/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c.obj
[11/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c.obj
[12/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c.obj
[13/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c.obj
[14/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c.obj
[15/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c.obj
[16/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c.obj
[17/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c.obj
[18/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c.obj
[19/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c.obj
[20/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c.obj
[21/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c.obj
[22/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c.obj
[23/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj
[24/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c.obj
[25/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c.obj
[26/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c.obj
[27/123] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[28/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c.obj
[29/123] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff_gen_drv.c.obj
[30/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c.obj
[31/123] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/option/syscall.c.obj
[32/123] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[33/123] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/App/fatfs.c.obj
[34/123] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj
[35/123] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/fatfs_platform.c.obj
[36/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[37/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[38/123] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[39/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[40/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[41/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usb_device.c.obj
[42/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_desc.c.obj
[43/123] Building ASM object Core/CMakeFiles/lxr_platform.dir/Startup/startup_stm32f765vgtx.s.obj
[44/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_cdc_if.c.obj
[45/123] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/Target/usbd_conf.c.obj
[46/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[47/123] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[48/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/system_stm32f7xx.c.obj
[49/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[50/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/syscalls.c.obj
[51/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/sysmem.c.obj
[52/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_hal_msp.c.obj
[53/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/safety/runtimeDiagnostics.c.obj
[54/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/delay.c.obj
[55/123] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/adcPots.c.obj
[56/123] Building C object Core/CMakeFiles/lxr_platform.dir/Src/flash_if.c.obj
[57/123] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/uart.c.obj
[58/123] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/DIO.c.obj
[59/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/encoder.c.obj
[60/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/graphic.c.obj
[61/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/lcd.c.obj
[62/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/ledHandler.c.obj
[63/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[64/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/frontPanelParser.c.obj
[65/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/copyClearTools.c.obj
[66/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/Cc2Text.c.obj
[67/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/Parameters.c.obj
[68/123] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/initKit.c.obj
[69/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/screensaver.c.obj
[70/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/automationNode.c.obj
[71/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/BufferTools.c.obj
[72/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/1PoleLp.c.obj
[73/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Decay.c.obj
[74/123] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[75/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/CymbalVoice.c.obj
[76/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/dither.c.obj
[77/123] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[78/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/DrumVoice.c.obj
[79/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/HiHat.c.obj
[80/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/distortion.c.obj
[81/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/modulationNode.c.obj
[82/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/mixer.c.obj
[83/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/lfo.c.obj
[84/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Samples.c.obj
[85/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/random.c.obj
[86/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/SlopeEg2.c.obj
[87/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/snapEg.c.obj
[88/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Snare.c.obj
[89/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/ResonantFilter.c.obj
[90/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Oscillator.c.obj
[91/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/squareRootLut.c.obj
[92/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/TiltEq.c.obj
[93/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientGenerator.c.obj
[94/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientTables.c.obj
[95/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/wavetable.c.obj
[96/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Compressor.c.obj
[97/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Delay.c.obj
[98/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c.obj
[99/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Reverb.c.obj
[100/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Distortion.c.obj
[101/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Ringmod.c.obj
[102/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c.obj
[103/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c.obj
[104/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[105/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/AudioCodecManager.c.obj
[106/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c.obj
[107/123] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c.obj
[108/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FIFO.c.obj
[109/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FxBus.c.obj
[110/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[111/123] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/TriggerOut.c.obj
[112/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiMessages.c.obj
[113/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[114/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/Uart.c.obj
[115/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiVoiceControl.c.obj
[116/123] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/clockSync.c.obj
[117/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[118/123] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/ParameterArray.c.obj
[119/123] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[120/123] Building C object Core/CMakeFiles/lxr_usb_app.dir/Src/Soundengine/USB/usb_manager.c.obj
[121/123] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[122/123] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271104 B       384 KB     68.95%
           FLASH:      224040 B       992 KB     22.06%
   text	   data	    bss	    dec	    hex	filename
 275264	    852	 340640	 616756	  96934	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 276124 bytes, checksum 0x2B
[123/123] 
```
