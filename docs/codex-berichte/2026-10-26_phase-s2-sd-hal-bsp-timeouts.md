# Phase S2 — SD HAL/BSP timeout and error handling

Date: 2026-10-26. Repository: `DocLXR-02/lxr-desktop-app`.

## Outcome

**IMPLEMENTED: the authorized software corrections passed validation and are published for review.** Active SD command waits no longer multiply CPU-clock-derived iteration counts or use the enormous STOP response allowance. Polling transfer failures retain accurate failure state/context, write timeout records its timeout bit, card-status errors are explicit, BSP preserves timeout results, and S1 immediately latches explicit status errors.

Physical transfer cleanup/recovery is deliberately **deferred**, not claimed implemented. No automatic STOP-on-error, abort, peripheral reset, recovery write or retry was added. There is no assertion that the card/data path is quiescent after a failure. This is not complete SD stack safety or deployment approval.

## Exact baseline and publication

Read complete AGENTS.md from lab-setup (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`) and previous D0/S1 reports:

- D0 report branch `codex-reports/2026-10-26-phase-d0-sd-storage-safety`, report commit `a80a37e0704cc5f178a36878f1f22a155cc6f21e`.
- S1 report branch `codex-reports/2026-10-26-phase-s1-sd-storage-foundation`, report commit `e66f31b979455d977993ad21583a4ff191ea6cf6`.

Verified source baseline: `safety/sd-storage-foundation-s1`, exact local/remote head `a4be3b94c552dbc867b197fad76655bed40cea34`, tree `9afb21a5c1ae614431f4b87b35a0de18c234274e`. Checkout was clean before creating the new branch; requested branch absent remotely. Existing `/workspace/lxr-desktop-app` checkout used. No additional worktree.

Source branch: `safety/sd-hal-bsp-timeouts-s2`.
Final source commit: `c88933a4b1968c8f9d173b71d86154e8969436be`.
Sole parent: exact S1 baseline above.
Final tree: `34a9f2fcc4ad41efd9ac292e53f6846b73b27f46`.

Report branch: `codex-reports/2026-10-26-phase-s2-sd-hal-bsp-timeouts`, independently based on lab-setup `5829398cea5d337200393757e39a37d4d8148256`. Only added file: `docs/codex-berichte/2026-10-26_phase-s2-sd-hal-bsp-timeouts.md`. Report commit is read from its branch head/final response, rather than recursively embedded here.

Source published with GitHub Git-data APIs, exact blob hashes checked against validated file bytes, then a new branch created at the source commit. Commit imported locally so checkout/index/tree agree and are clean. No force push, merge, PR, change to any previous branch, or generated artifact upload. All firmware images remain local. No hardware, flashing, physical SD access, option-byte/bootloader/reset/clock/interrupt-priority/startup/linker operation occurred.

## Evidence labels

CODE: inspected source. BUILD: executed host tests, compiler/linker or artifact analysis. HYPOTHESIS: plausible unobserved runtime effect. OPEN: further design or physical validation required.

## Relevant active paths and scope

The existing application mounts/uses the disk through foreground FatFs and S1's `FATFS/Target/sd_diskio.c`. S1 rejects ISR/masked/disabled-SysTick callers before BSP access. Initialization reaches BSP_SD_Init → HAL_SD_Init → HAL_SD_InitCard → CMD0/CMD8/voltage negotiation/CID/RCA/CSD/select/block length. Read/write use polling HAL data transfers and command responses; multi-block successful data transfer uses CMD12 STOP. Readiness uses BSP_SD_GetCardState → HAL_SD_GetCardState → SD_SendStatus → CMD13.

The active response formats are CMD0/no-response, R1, R2, R3, R6 and R7. Their old software limits all used CPU-clock multiplication. R3 deliberately ignores response CRC because OCR has no valid CRC; that exception is preserved. R1/R6 command-index checks and all existing response error-bit parsing remain intact. R7 retains its original checks; no unrelated response validation feature was invented.

The runtime uses1-bit SD polling. Optional wide-bus SCR/status-data APIs, SD DMA/IT transfers, erase/format flows and MMC are not normal read/write users exercised here. No whole HAL upgrade or optional API rewrite occurred. The shared LL response helper also changes the software timeout policy for direct/optional LL callers: any requested command-response timeout above5000ms is capped. In particular an optional erase command response caller previously passed63000; its response wait is now capped too, without implementing/claiming erase completion or qualification. Optional clients require separate review if enabled later. This is a disclosed consequence of correcting the shared SDMMC response API, not a claim those features were validated.

## Changed files

Six production files:

1. `Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c` — one millisecond response wait helper reused by five response formats and CMD0; removes all six old clock-scaled loops.
2. `Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_ll_sdmmc.h` — STOP response timeout100000000 →5000; shared SDMMC_DATATIMEOUT hardware constant unchanged.
3. `Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c` — polling read/write failure state/context and timeout recording; successful transfer Context NONE; explicit failed-status sentinel; finite outer voltage-negotiation allowance.
4. `FATFS/Target/bsp_driver_sd.c` — distinguish HAL_TIMEOUT/HAL_BUSY; classify valid transitional card states as busy and actual status errors as error without discarding existing ErrorCode.
5. `FATFS/Target/bsp_driver_sd.h` — additive nonzero result codes MSD_TIMEOUT=3, MSD_BUSY=4 and SD_TRANSFER_ERROR=2; existing numeric success/error/busy values and function signatures unchanged.
6. `FATFS/Target/sd_diskio.c` — narrowly required S1 compatibility: explicit BSP status error immediately calls existing SD_Fail rather than being treated as another temporary-busy observation. No latch reset/recovery change.

Tests/documentation:

7. `tests/sd_storage/host.c` — retains all existing S1 assertions and adds immediate error/status/sync fault-latch checks.
8. `tests/sd_hal/run.py` — extract production definitions and real target constant values, native sanitizer build/run.
9. `tests/sd_hal/host.c` — explicit register/time/handle mocks, production-body assertions.
10. `tests/sd_hal/README.md` — reproducibility, modeled versus physical behavior and assumptions.

Baseline-to-final diff:10 files,359 insertions/135 deletions. Full combined diff is included below. Existing unrelated suites unchanged. No source changes after final validation. Firmware build configuration/presets/version untouched. Original production CRLF retained. Whitespace check with `git -c core.whitespace=cr-at-eol diff --check a4be3b94c552dbc867b197fad76655bed40cea34` passed.

## Command timeout policy

`stm32f7xx_ll_sdmmc.c:1196` SDMMC_WaitCommand uses HAL milliseconds, unsigned subtraction and a maximum SDMMC_CMDTIMEOUT=5000. A zero requested timeout returns software timeout immediately. Oversized values, including UINT32_MAX, are clamped before waiting; there is no timeout*SystemCoreClock multiplication and no overflow-prone count conversion. Existing STOP definition at LL header323 now also reads5000.

The helper permits successful response completion only with an expected terminal flag and, for response-bearing commands, CMDACT clear. CMD0 preserves its original CMDSENT completion behavior. After wait success each response routine performs its original hardware timeout, CRC, command-index and card-response error checks. A software wait timeout returns SDMMC_ERROR_TIMEOUT; hardware command timeout remains SDMMC_ERROR_CMD_RSP_TIMEOUT. No unbounded fallback or interrupt disabling is added.

The helper checks foreground context, zero PRIMASK/BASEPRI/FAULTMASK, and enabled SysTick counter/interrupt before sampling the tick. Unsupported/pre-tick calls return failure without waiting. The helper itself does not undo a command a direct LL caller may already have dispatched before entering the response routine. The active firmware's S1 guard rejects inappropriate callers before BSP dispatch; normal startup mount is after HAL_Init/timebase setup. HAL_SD_InitCard still uses its existing HAL_Delay(2), which assumes a running tick; S1 prevents an unsupported pre-tick call in the active path. No claim is made that arbitrary external direct HAL callers can initialize safely before HAL_Init.

Conditions: actual HAL_GetTick must advance, supported foreground execution must receive CPU time, and registers must behave as documented. Enabled SysTick control bits do not prove the oscillator, interrupt service or debugger are functioning. A stalled tick or indefinite interrupt starvation remains outside these time bounds. No watchdog or DWT reset is added. DWT/LCD shared free-running counter is untouched.

The 5000ms cap preserves the existing ordinary command allowance and replaces only the extreme STOP/default conversion behavior. It is a conservative development response policy, not manufacturer-qualified card timing or measured electrical response latency. Normal commands return immediately on completion; there is no mandatory5-second sleep. Slow legitimate cards/unsupported optional callers may now fail rather than wait longer; hardware qualification remains required.

### Voltage negotiation

`stm32f7xx_hal_sd.c:2712–2781` SD_PowerON previously permitted65535 iterations of successful CMD55/ACMD41 without voltage readiness. It retains the trial limit but now measures a separate5000ms outer allowance (`voltage_start`), returning HAL_SD_ERROR_TIMEOUT when validvoltage remains zero after that allowance. The check occurs after a command pair, so the pair's nested waits can overrun the allowance before control returns. It does not reject a just-completed successful voltage response or introduce replays outside the existing negotiation loop.

This is not a single whole-startup or whole-transfer deadline. Initialization contains several sequential commands and a fixed delay; public HAL read/write Timeout remains a separate data-polling allowance sampled before initial command dispatch; successful multi-block transfers also invoke a bounded STOP response. S1 readiness has its own5000ms allowance and a status command can run past that outer allowance before the next check. No claim of a universal5-second or10-second bound for the complete physical operation is made. All inspected active command response loops are now tick-bounded under the stated assumptions; optional status-data/SCR helpers retain their pre-existing large hardware/data constants and were not promoted into the active path.

## HAL failure state and cleanup reasoning

ReadBlocks begins at HAL source570; WriteBlocks at782. On command failure, data CRC/hardware data timeout, RX overrun/TX underrun or failed STOP, the synchronous functions now set HAL_SD_STATE_ERROR and retain attempted-transfer Context. Software data polling timeout sets HAL_SD_STATE_TIMEOUT and records HAL_SD_ERROR_TIMEOUT. The read FIFO-tail timeout now consistently returns HAL_TIMEOUT rather than HAL_ERROR. The write timeout previously ORed the prior successful command result (often zero); it now ORs HAL_SD_ERROR_TIMEOUT explicitly.

The existing static-flag clearing remains; S2 adds no data-path or peripheral control write for failure recovery. READY is published and Context cleared only on the successful synchronous completion path. A direct second polling transfer on a TIMEOUT/ERROR handle fails before another command/FIFO setup, preserving previous errors while adding the existing busy error as applicable. Actual S1 additionally latches failure and blocks the entire disk interface until reboot.

Retained Context is evidence of what was attempted, not evidence of an active or stopped physical transaction. A card might still be programming and DPSM might still be enabled after failure. The tests explicitly check that S2 has not silently reset/disabled it. HAL_SD_Abort is not called, no unchecked STOP is issued after uncertain timeout, no DMA or clock reset is attempted, and no data transfer is replayed. The pre-existing normal successful multiblock STOP is preserved but its response wait is bounded. Physical cleanup cannot be established with this register model and remains deliberately deferred. Direct use of other HAL APIs that reset/change handle state remains outside S1's disk-owner contract.

## Card-status and BSP fidelity

HAL_SD_GetCardState at2268 now returns existing HAL_SD_CARD_ERROR (0xFF) on SD_SendStatus failure, while OR-preserving the actual command error. It no longer derives a card state from the zero-initialized response after failure. A successful status query still returns the actual card-state bits and does not erase prior relevant ErrorCode.

BSP_GetCardState at199 treats HAL_CARD_ERROR or nonzero handle ErrorCode as SD_TRANSFER_ERROR. TRANSFER returns0/OK. Other valid defined states READY through DISCONNECTED return BUSY, preserving the possibility of temporary initialization/programming transitions; they are not invented evidence of physical removal. Invalid/unknown values return error. This avoids incorrectly treating a valid STANDBY/initialization transition as a terminal status error while still separating actual command failure from busy. Host coverage iterates all defined states1–8 plus invalid15 and explicit failed command/history.

BSP read/write at91/111 return MSD_TIMEOUT when the HAL result is HAL_TIMEOUT, MSD_BUSY for HAL_BUSY, generic MSD_ERROR otherwise. Signatures remain uint8_t and all newly exposed failure codes are nonzero. The active S1 caller already rejects every transfer result !=MSD_OK, so this cannot turn a new error code into success. HAL_BUSY is an additive mapping; current synchronous HAL implementations usually report busy as HAL_ERROR plus ErrorCode BUSY, which remains generic error. No new guessed removal/protection classification.

For hardware DTIMEOUT branches the vendor HAL still uses HAL_ERROR with HAL_SD_ERROR_DATA_TIMEOUT; BSP's scalar result is generic error, and the handle retains the specific bit. S2 does not guess a scalar timeout from possibly historical bits. The native tests cover this distinction. S1 SD_WaitReady at73–87 now latches SD_TRANSFER_ERROR immediately; busy still follows the S1 deadline, status/init cannot clear Fault, and no new recovery route exists.

## Tests: exact scope and results

Prepared tools: CMake3.31.10; Ninja1.13.0.git.kitware.jobserver-pipe-1; Python3.12.14; ARM GCC14.2.1 20241119 (15:14.2.rel1-1), newlib-nano; native GCC14.2.0. Commands use PATH `/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH`. Environment/runtime skill checked; no secret values inspected or included.

New command: `python3 tests/sd_hal/run.py` — PASS with native ASan/UBSan. It extracts actual function definitions, including private helper bodies, with brace-balanced selection. Native test code does not rewrite their algorithms. Target preprocessing supplies real macro constants from target headers; minimal register/handle shapes, HAL enum values, BLOCKSIZE512, clock/context/time behavior, hardware flag/FIFO and command-dispatch behavior are explicit mocks. It is **not** a native build of the entire HAL translation unit. Complete production units are ARM-compiled in four firmware configurations.

Extracted bodies: SDMMC_WaitCommand; R1/R2/R3/R6/R7; CMD0 error response; CMD12 stop construction/response; SD_PowerON; SD_SendStatus; HAL_SD_GetCardState; HAL_SD_ReadBlocks/WriteBlocks; BSP_SD_ReadBlocks/WriteBlocks/GetCardState.

| Scenario | Result |
|---|---|
| Successful response in each format/CMD0 | PASS |
| Missing response and CMDACT stuck | PASS, finite software timeout |
| Hardware command timeout | PASS, command-response-timeout result |
| CRC failure | PASS; R3's intended CRC exception preserved |
| R1/R6 index mismatch | PASS, failure with unchanged RCA output |
| STOP no-response | PASS, one command dispatch then timeout; no retry |
| UINT32_MAX timeout and zero timeout | PASS,5000ms cap and immediate zero rejection; no multiplication |
| Tick wrap near UINT32_MAX | PASS, command waits, voltage negotiation and HAL data timeout |
| Pre-tick and ISR/masked wait | PASS, rejection without tick polling |
| Voltage ready, initial command error, permanently non-ready | PASS, correct type/error and outer retry limit |
| HAL read/write data polling timeout | PASS, timeout bit, TIMEOUT state, retained Context |
| Read FIFO-tail timeout | PASS, HAL_TIMEOUT consistency |
| Data CRC and hardware data timeout | PASS, ERROR state and relevant recorded bits |
| First transfer command failure | PASS, failure state/context, no success |
| Single/multiblock FIFO read/write success | PASS, bytes consumed, READY and Context NONE |
| Failed multiblock STOP | PASS, retained failed-attempt context, no data replay |
| Direct second transfer after failure | PASS, no additional command dispatch |
| BSP normal/error/HAL_TIMEOUT | PASS; HAL_BUSY branch is simple static mapping, not independently forced by the actual synchronous HAL body |
| Status error sentinel/old error retention/defined states | PASS |
| S1 fault rejection | PASS in whole production sd_diskio.c suite; explicit status error now latches on first poll,20 repeated request rounds cannot clear/bypass it |

No reset/abort/remount/recovery implementations are supplied by the native fixture; new accidental references would fail linking. Call counters and retained mock DCTRL demonstrate no retry/reset in tested execution paths; they do not prove hardware quiescence. Register mocks model flags/FIFO/control values, not electrical timing, actual W1C semantics, card internal caches, DMA or bus contention. LeakSanitizer is explicitly disabled because this executor's tracing cannot support it; ASan/UBSan are active, fatal on errors.20-second native-process deadline prevents test hangs. S1 suite has its own15-second guard.

All existing suites rerun and PASS: Bug001, Euclidean, safety, USB allocator and S1 sd_storage. Existing S1 assertions were retained and augmented; no existing test was weakened. Final tests after the last production change all pass; the final additional test-only FIFO-tail/hardware-timeout checks also pass. Full transcripts below.

### Development errors and limitations

Initial temporary BSP editing produced a malformed cast capture; it was corrected before the first target build. Early native harness compilation failed because generated constant imports omitted HAL_SD_ERROR_CMD_CRC_FAIL, then SD_SWITCH_1_8V_CAPACITY when voltage tests were added. Import selection was corrected to include actual target macros; final fixture compilation and execution pass. The failed development test command was `python3 tests/sd_hal/run.py` (exit1 for each missing-macro compilation); it was rerun successfully after correction. These were development/harness errors, not observed firmware/hardware failures. No sanitizer runtime failure or unresolved firmware build/test failure remains.

During review the BSP classification was kept conservative for all valid transitional states instead of classifying STANDBY as terminal; corresponding new tests now cover all valid states. This preserves ordinary initialization semantics while exposing only reliable errors. No old test was suppressed to achieve this result.

## Firmware builds, images and resources

Release/Debug with LXR_RUNTIME_DIAGNOSTICS OFF and ON all configured/built and passed image validation. Standard release/debug caches OFF, separate diagnostics-release/debug caches ON; default OFF unchanged. Header changes recompiled dependent objects, followed by final narrower rebuilds after the last source edit. These were incremental project builds, not a claim of four clean directory rebuilds. Build output installation instructions were printed by an existing target and never followed.

Actual source version remains1.7. Generated application files LXR_V2_V1.7.img/.bin and LXR_V2_STM32F765.elf remain local. No equivalence to the external official public v1.71 reference or private support1.72 is claimed. No official binary was examined in S2.

| Region/metric | S1 normal Release | S2 normal Release | S1 diagnostics Release | S2 diagnostics Release |
|---|---:|---:|---:|---:|
| ITCM |15016|15016|15088|15088|
| DTCM |107392|107392|107392|107392|
| General RAM |271576|271576|271648|271648|
| Linker FLASH used |224856|223536|225224|223904|
| Full application payload |276868|275548|277308|275988|

Delta **-1320 bytes Flash** in all four configurations; ITCM/DTCM/general RAM unchanged. No new static diagnostic/global allocation, heap, queue, history or buffer. New command timing uses small fixed locals; voltage loop adds one local tick. Normal headroom ITCM1368, DTCM23680 before stack, RAM121640; instrumented ITCM1296, DTCM23680, RAM121568. A stack reservation/headroom is not measured worst-case stack safety.

Application Flash is0x08008000 with992KiB=1015808 bytes, first32KiB excluded. Final payload spans: Release275548, Debug275596, diagnostics Release275988, diagnostics Debug276036. Full-span Flash headroom740260/740212/739820/739772. Linker FLASH used and validator FLASH VMA high-water are different from full payload span; do not substitute one for another. Final image checker FLASH VMA figures222684/222732/223052/223100.

All four images passed16-byte header LXRV2IMG, little-endian payload length, checksum, exact .bin payload identity, ELF/load/memory boundaries and plausible vectors. Initial MSP0x20080000; reset0x08014DED (Thumb), changed by application layout. Existing startup relocates stack into DTCM; no startup/vector-table configuration changed. Exact final image/ELF SHA-256 and byte counts appear in final-10.log below. Intermediate image logs are explicitly superseded by final logs.

### Audio-object and stack checks

Saved12 S1 audio/interrupt disassembly+relocation dumps after verifying full baseline hashes, then compared final outputs. Soundengine.c, cs4344_cs5343.c and stm32f7xx_it.c instructions/relocations match in all configurations. Release objects are entirely byte-identical. Debug objects differ; changed shared header macro information is included under -g3, while instruction/relocation comparison is unchanged. The comparison does not assert that every debug metadata byte was exhaustively attributed. SD-specific objects changed as expected. No audio-path instrumentation, clock, scheduling, buffers or priorities changed.

Additional temporary Release compilations of the four affected C translation units used their compile_commands.json arguments, redirecting output to /tmp/phase-s2/stack-N.o with -fstack-usage. Relevant records are appended below: polling read/write64-byte fixed frames, R1/R6 response24, R2/R3/R7 response16, BSP transfer16 and status8. Inlining can fold helper/voltage locals into caller frames. These are per-function compiler frames, not a complete call-chain/exception/floating-point stack bound or a measured high-water mark. No VLA, recursion or dynamic storage was added. Temporary stack-analysis objects are not uploaded.

## Deferred physical cleanup and S3 prerequisites

CODE/BUILD: active command response waits are now bounded by tick policy without overflowing arithmetic, failure results retain timeout/status information, and S1 closes further disk I/O after failure. CODE: no recovery/reset/replay route added. HYPOTHESIS: shorter software command caps or voltage allowance may reject unusually slow valid hardware; extra tick reads and polling may affect foreground latency. OPEN: actual response/programming latency, card compatibility, data-path state after failure, storage bus/audio interference and runtime stack margin.

Physical cleanup remains hardware-dependent. Preserving Context/TIMEOUT/ERROR prevents a misleading new transfer admission but does not stop DPSM or card programming. Before any future recovery, establish a reviewed SDMMC/card-state sequence and validate with separately authorized hardware observation; do not assume HAL_SD_Abort is universally safe or clear the S1 latch. Reboot-only disk policy remains unchanged.

S3 should now address the already-confirmed FatFs mirror/FSInfo write-error propagation and generic disk initialization cached-success behavior, with tests proving failures remain visible. Subsequent preset short-write/close error checks, shared FIL ownership/cleanup, remount/cache lifetime and diagnostic exporter remain separate. These issues cannot be inferred solved from S1/S2 errors now reaching the driver. Existing FA_CREATE_ALWAYS truncation/cleanup behavior is unchanged. No promise of filesystem transaction atomicity, power-loss durability or glitch-free audio.

The software correction is suitable for review and continuation to S3. It is **not** approval to install firmware. Complete remaining software phases, supported-card policy review, backup/recovery planning and Daniel's separate explicit hardware authorization before any controlled physical validation.

**NOT FLASHED — NOT HARDWARE VALIDATED.** No firmware binary published. No physical SD card read/written/formatted/deleted. No bootloader, option-byte, clock, priority, startup/linker or automatic-reset change.

## Actual commands and output

Initial logs provide versions/configuration and first full validation pass. Final logs supersede intermediate image hashes after voltage-negotiation and transitional-state refinements.

### 00.log

```text
$ cmake --version
cmake version 3.31.10

CMake suite maintained and supported by Kitware (kitware.com/cmake).

EXIT 0
```

### 01.log

```text
$ ninja --version
1.13.0.git.kitware.jobserver-pipe-1

EXIT 0
```

### 02.log

```text
$ python3 --version
Python 3.12.14

EXIT 0
```

### 03.log

```text
$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (15:14.2.rel1-1) 14.2.1 20241119
Copyright (C) 2024 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


EXIT 0
```

### 04.log

```text
$ python3 tests/sd_hal/run.py
PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP
PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset
PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation

EXIT 0
```

### 05.log

```text
$ python3 tests/bug001/run.py
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence

EXIT 0
```

### 06.log

```text
$ python3 tests/euclidean/run.py
PASS generation: 2176 exact masks match lab-setup; literal lengths; CLZ unshimmed
PASS bounds: all byte-valued track/pattern/length inputs
PASS playback: 294912 flam/shift ticks; 4160 recording positions; mixed lengths
PASS persistence: 130 full named/autosave roundtrips, unchanged layout/payload
PASS malformed: 10210 truncation/range/I-O/legacy-tail cases; no partial lengths
PASS protocol: 180802 byte-value/counter boundary calls; no wrap
PASS all six ASan/UBSan groups

EXIT 0
```

### 07.log

```text
$ python3 tests/safety/run.py
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state

EXIT 0
```

### 08.log

```text
$ python3 tests/usb_allocator/run.py
PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing
PASS USB failures: 14 rollback/retention/retry scenarios plus invalid requests; no allocator calls
Host object: size=552 alignment=8 (target size/alignment checked separately in ELF)
PASS complete production USB class object: no heap/allocator-lock references

EXIT 0
```

### 09.log

```text
$ python3 tests/sd_storage/run.py
PASS all 1..128 sector read/write counts; exact range edge and 5000ms HAL argument
PASS finite readiness, permanent busy deadline, tick wrap success/timeout; persistent reboot-only fault
PASS BSP read/write/init failure, ambiguous write blocks subsequent I/O; removal before/during transfer and reinsertion cannot clear latch
PASS CTRL_SYNC ready/delayed/timeout/fault and explicit BSP status error; status cannot clear or bypass fault
PASS invalid pointers/counts/LUN/overflowing ranges/ioctls; invalid input does not poison valid operation
PASS IRQ/masked/suspended tick rejection; no reset/remount/retry dependencies in linked driver

EXIT 0
```

### 10.log

```text
$ cmake --preset release
Preset CMake variables:

  CMAKE_BUILD_TYPE="Release"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/release

EXIT 0
```

### 11.log

```text
$ cmake --build --preset release --parallel 4
[1/1] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================

EXIT 0
```

### 12.log

```text
$ cmake --preset debug
Preset CMake variables:

  CMAKE_BUILD_TYPE="Debug"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/debug

EXIT 0
```

### 13.log

```text
$ cmake --build --preset debug --parallel 4
[1/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c.obj
[2/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c.obj
[3/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c.obj
[4/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c.obj
[5/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c.obj
[6/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c.obj
[7/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c.obj
[8/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c.obj
[9/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c.obj
[10/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c.obj
[11/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c.obj
[12/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c.obj
[13/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c.obj
[14/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c.obj
[15/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c.obj
[16/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c.obj
[17/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c.obj
[18/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c.obj
[19/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c.obj
[20/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c.obj
[21/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c.obj
[22/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj
[23/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c.obj
[24/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c.obj
[25/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c.obj
[26/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c.obj
[27/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c.obj
[28/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c.obj
[29/118] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[30/118] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj
[31/118] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/App/fatfs.c.obj
[32/118] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff_gen_drv.c.obj
[33/118] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/fatfs_platform.c.obj
[34/118] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/option/syscall.c.obj
[35/118] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[36/118] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[37/118] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[38/118] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[39/118] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[40/118] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usb_device.c.obj
[41/118] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[42/118] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_cdc_if.c.obj
[43/118] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/Target/usbd_conf.c.obj
[44/118] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_desc.c.obj
[45/118] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[46/118] Building C object Core/CMakeFiles/lxr_platform.dir/Src/system_stm32f7xx.c.obj
[47/118] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_hal_msp.c.obj
[48/118] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[49/118] Building C object Core/CMakeFiles/lxr_platform.dir/Src/delay.c.obj
[50/118] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/DIO.c.obj
[51/118] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/adcPots.c.obj
[52/118] Building C object Core/CMakeFiles/lxr_platform.dir/Src/flash_if.c.obj
[53/118] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[54/118] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/uart.c.obj
[55/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/encoder.c.obj
[56/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[57/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/graphic.c.obj
[58/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/ledHandler.c.obj
[59/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/lcd.c.obj
[60/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/frontPanelParser.c.obj
[61/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/Cc2Text.c.obj
[62/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/Parameters.c.obj
[63/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/copyClearTools.c.obj
[64/118] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/initKit.c.obj
[65/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/screensaver.c.obj
[66/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/automationNode.c.obj
[67/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/1PoleLp.c.obj
[68/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/BufferTools.c.obj
[69/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/CymbalVoice.c.obj
[70/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Decay.c.obj
[71/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/distortion.c.obj
[72/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/dither.c.obj
[73/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/DrumVoice.c.obj
[74/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/HiHat.c.obj
[75/118] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[76/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/lfo.c.obj
[77/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/mixer.c.obj
[78/118] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[79/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/modulationNode.c.obj
[80/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/SlopeEg2.c.obj
[81/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Oscillator.c.obj
[82/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/random.c.obj
[83/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/snapEg.c.obj
[84/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Snare.c.obj
[85/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/squareRootLut.c.obj
[86/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/ResonantFilter.c.obj
[87/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/TiltEq.c.obj
[88/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientGenerator.c.obj
[89/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientTables.c.obj
[90/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Compressor.c.obj
[91/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/wavetable.c.obj
[92/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Delay.c.obj
[93/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c.obj
[94/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Ringmod.c.obj
[95/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c.obj
[96/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c.obj
[97/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Distortion.c.obj
[98/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Reverb.c.obj
[99/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c.obj
[100/118] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[101/118] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FIFO.c.obj
[102/118] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/AudioCodecManager.c.obj
[103/118] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FxBus.c.obj
[104/118] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c.obj
[105/118] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[106/118] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[107/118] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiMessages.c.obj
[108/118] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/TriggerOut.c.obj
[109/118] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[110/118] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiVoiceControl.c.obj
[111/118] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/clockSync.c.obj
[112/118] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/ParameterArray.c.obj
[113/118] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/Uart.c.obj
[114/118] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[115/118] Building C object Core/CMakeFiles/lxr_usb_app.dir/Src/Soundengine/USB/usb_manager.c.obj
[116/118] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[117/118] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      223544 B       992 KB     22.01%
   text	   data	    bss	    dec	    hex	filename
 274696	    852	 341112	 616660	  968d4	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 275556 bytes, checksum 0x23
[118/118] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================

EXIT 0
```

### 14.log

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### 15.log

```text
$ cmake --build build/diagnostics-release --parallel 4
[1/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c.obj
[2/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c.obj
[3/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c.obj
[4/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c.obj
[5/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c.obj
[6/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c.obj
[7/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c.obj
[8/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c.obj
[9/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c.obj
[10/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c.obj
[11/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c.obj
[12/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c.obj
[13/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c.obj
[14/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c.obj
[15/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c.obj
[16/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c.obj
[17/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c.obj
[18/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c.obj
[19/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c.obj
[20/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c.obj
[21/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c.obj
[22/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj
[23/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c.obj
[24/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c.obj
[25/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c.obj
[26/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj
[27/119] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[28/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c.obj
[29/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c.obj
[30/119] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/option/syscall.c.obj
[31/119] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/App/fatfs.c.obj
[32/119] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff_gen_drv.c.obj
[33/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c.obj
[34/119] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/fatfs_platform.c.obj
[35/119] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[36/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[37/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[38/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[39/119] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[40/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[41/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usb_device.c.obj
[42/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_cdc_if.c.obj
[43/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_desc.c.obj
[44/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[45/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/Target/usbd_conf.c.obj
[46/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[47/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/system_stm32f7xx.c.obj
[48/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_hal_msp.c.obj
[49/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/flash_if.c.obj
[50/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/delay.c.obj
[51/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/safety/runtimeDiagnostics.c.obj
[52/119] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/adcPots.c.obj
[53/119] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/uart.c.obj
[54/119] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/DIO.c.obj
[55/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/encoder.c.obj
[56/119] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[57/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[58/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/graphic.c.obj
[59/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/lcd.c.obj
[60/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/frontPanelParser.c.obj
[61/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/Cc2Text.c.obj
[62/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/Parameters.c.obj
[63/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/copyClearTools.c.obj
[64/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/screensaver.c.obj
[65/119] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/initKit.c.obj
[66/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/ledHandler.c.obj
[67/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/1PoleLp.c.obj
[68/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/BufferTools.c.obj
[69/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/automationNode.c.obj
[70/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Decay.c.obj
[71/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/CymbalVoice.c.obj
[72/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/distortion.c.obj
[73/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/dither.c.obj
[74/119] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[75/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/DrumVoice.c.obj
[76/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/lfo.c.obj
[77/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/HiHat.c.obj
[78/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[79/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/mixer.c.obj
[80/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/modulationNode.c.obj
[81/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/random.c.obj
[82/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/SlopeEg2.c.obj
[83/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/ResonantFilter.c.obj
[84/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/snapEg.c.obj
[85/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/squareRootLut.c.obj
[86/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Snare.c.obj
[87/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Oscillator.c.obj
[88/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientGenerator.c.obj
[89/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/TiltEq.c.obj
[90/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientTables.c.obj
[91/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Compressor.c.obj
[92/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/wavetable.c.obj
[93/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c.obj
[94/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Delay.c.obj
[95/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Reverb.c.obj
[96/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Distortion.c.obj
[97/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Ringmod.c.obj
[98/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c.obj
[99/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c.obj
[100/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c.obj
[101/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/AudioCodecManager.c.obj
[102/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[103/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c.obj
[104/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FIFO.c.obj
[105/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FxBus.c.obj
[106/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[107/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiMessages.c.obj
[108/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiVoiceControl.c.obj
[109/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/TriggerOut.c.obj
[110/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[111/119] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/clockSync.c.obj
[112/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/Uart.c.obj
[113/119] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[114/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[115/119] Building C object Core/CMakeFiles/lxr_usb_app.dir/Src/Soundengine/USB/usb_manager.c.obj
[116/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/ParameterArray.c.obj
[117/119] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[118/119] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      223864 B       992 KB     22.04%
   text	   data	    bss	    dec	    hex	filename
 275088	    852	 341184	 617124	  96aa4	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 275948 bytes, checksum 0xF7
[119/119] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================

EXIT 0
```

### 16.log

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### 17.log

```text
$ cmake --build build/diagnostics-debug --parallel 4
[1/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c.obj
[2/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c.obj
[3/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c.obj
[4/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c.obj
[5/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c.obj
[6/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c.obj
[7/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c.obj
[8/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c.obj
[9/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c.obj
[10/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c.obj
[11/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c.obj
[12/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c.obj
[13/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c.obj
[14/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c.obj
[15/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c.obj
[16/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c.obj
[17/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c.obj
[18/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c.obj
[19/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c.obj
[20/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c.obj
[21/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c.obj
[22/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj
[23/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c.obj
[24/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c.obj
[25/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c.obj
[26/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c.obj
[27/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj
[28/119] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[29/119] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff_gen_drv.c.obj
[30/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c.obj
[31/119] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c.obj
[32/119] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/App/fatfs.c.obj
[33/119] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/option/syscall.c.obj
[34/119] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/fatfs_platform.c.obj
[35/119] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[36/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[37/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[38/119] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[39/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[40/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[41/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_cdc_if.c.obj
[42/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usb_device.c.obj
[43/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_desc.c.obj
[44/119] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/Target/usbd_conf.c.obj
[45/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[46/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[47/119] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[48/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/safety/runtimeDiagnostics.c.obj
[49/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/system_stm32f7xx.c.obj
[50/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/delay.c.obj
[51/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/flash_if.c.obj
[52/119] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_hal_msp.c.obj
[53/119] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/DIO.c.obj
[54/119] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/adcPots.c.obj
[55/119] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/uart.c.obj
[56/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/encoder.c.obj
[57/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/graphic.c.obj
[58/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[59/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/lcd.c.obj
[60/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/frontPanelParser.c.obj
[61/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/ledHandler.c.obj
[62/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/Cc2Text.c.obj
[63/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/Parameters.c.obj
[64/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/copyClearTools.c.obj
[65/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/screensaver.c.obj
[66/119] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/initKit.c.obj
[67/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/1PoleLp.c.obj
[68/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/automationNode.c.obj
[69/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/BufferTools.c.obj
[70/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/CymbalVoice.c.obj
[71/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/distortion.c.obj
[72/119] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[73/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/dither.c.obj
[74/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Decay.c.obj
[75/119] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[76/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/DrumVoice.c.obj
[77/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/modulationNode.c.obj
[78/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/HiHat.c.obj
[79/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/lfo.c.obj
[80/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/mixer.c.obj
[81/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/SlopeEg2.c.obj
[82/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/random.c.obj
[83/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/snapEg.c.obj
[84/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Oscillator.c.obj
[85/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/ResonantFilter.c.obj
[86/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/squareRootLut.c.obj
[87/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Snare.c.obj
[88/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientGenerator.c.obj
[89/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientTables.c.obj
[90/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/TiltEq.c.obj
[91/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/wavetable.c.obj
[92/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Delay.c.obj
[93/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Compressor.c.obj
[94/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Distortion.c.obj
[95/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c.obj
[96/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Ringmod.c.obj
[97/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c.obj
[98/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Reverb.c.obj
[99/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c.obj
[100/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c.obj
[101/119] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c.obj
[102/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/AudioCodecManager.c.obj
[103/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[104/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FxBus.c.obj
[105/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FIFO.c.obj
[106/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[107/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[108/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiMessages.c.obj
[109/119] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/TriggerOut.c.obj
[110/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiVoiceControl.c.obj
[111/119] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/clockSync.c.obj
[112/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/Uart.c.obj
[113/119] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[114/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/ParameterArray.c.obj
[115/119] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[116/119] Building C object Core/CMakeFiles/lxr_usb_app.dir/Src/Soundengine/USB/usb_manager.c.obj
[117/119] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[118/119] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      223912 B       992 KB     22.04%
   text	   data	    bss	    dec	    hex	filename
 275136	    852	 341184	 617172	  96ad4	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 275996 bytes, checksum 0xFE
[119/119] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================

EXIT 0
```

### 18.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0xCE", "configuration": "release", "elf_sha256": "f5f7775315e3283daa0085277b1a162ecdc32a88f27706032bd75c4828a630fc", "image_bytes": 275524, "image_sha256": "984a30630549756a6a57f5bd5b65611fd22ca3b36598fb33ae767076efa5ad8a", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 222644, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275508, "reset_vector": "0x08014DC5", "result": "PASS"}
{"checksum": "0x23", "configuration": "debug", "elf_sha256": "2d0c58ffc25a3d071c244557c819b93c969b1e4a3f19d7ea9437b152c0e1b4b9", "image_bytes": 275572, "image_sha256": "fd590d821ffbf8f5e4e4a95473bd31608b4a5b0a9e921dbc7b20c7bdcd43688f", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 222692, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275556, "reset_vector": "0x08014DC5", "result": "PASS"}
{"checksum": "0xF7", "configuration": "diagnostics-release", "elf_sha256": "ac6af9af3731e2b9b0f3e9866c105ca4bc595087120983b3957498ffc0b6a506", "image_bytes": 275964, "image_sha256": "b1f7536da12b5cdeb5614e42c3481fa7b231459a54862a52cf1207ac8eb73dac", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223012, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 275948, "reset_vector": "0x08014DC5", "result": "PASS"}
{"checksum": "0xFE", "configuration": "diagnostics-debug", "elf_sha256": "5da93e40a650d67ed991d782714969be4dea0a73962d0a8b0e0da41c0d115ea6", "image_bytes": 276012, "image_sha256": "16e667945e0143495661bf08b785982990c1d76e7503e83d6954b212b062a1ea", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223060, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 275996, "reset_vector": "0x08014DC5", "result": "PASS"}

EXIT 0
```

### final-00.log

```text
$ python3 tests/sd_hal/run.py
PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP
PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap
PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset
PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation

EXIT 0
```

### final-01.log

```text
$ python3 tests/sd_storage/run.py
PASS all 1..128 sector read/write counts; exact range edge and 5000ms HAL argument
PASS finite readiness, permanent busy deadline, tick wrap success/timeout; persistent reboot-only fault
PASS BSP read/write/init failure, ambiguous write blocks subsequent I/O; removal before/during transfer and reinsertion cannot clear latch
PASS CTRL_SYNC ready/delayed/timeout/fault and explicit BSP status error; status cannot clear or bypass fault
PASS invalid pointers/counts/LUN/overflowing ranges/ioctls; invalid input does not poison valid operation
PASS IRQ/masked/suspended tick rejection; no reset/remount/retry dependencies in linked driver

EXIT 0
```

### final-02.log

```text
$ python3 tests/bug001/run.py
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence

EXIT 0
```

### final-03.log

```text
$ python3 tests/euclidean/run.py
PASS generation: 2176 exact masks match lab-setup; literal lengths; CLZ unshimmed
PASS bounds: all byte-valued track/pattern/length inputs
PASS playback: 294912 flam/shift ticks; 4160 recording positions; mixed lengths
PASS persistence: 130 full named/autosave roundtrips, unchanged layout/payload
PASS malformed: 10210 truncation/range/I-O/legacy-tail cases; no partial lengths
PASS protocol: 180802 byte-value/counter boundary calls; no wrap
PASS all six ASan/UBSan groups

EXIT 0
```

### final-04.log

```text
$ python3 tests/safety/run.py
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state

EXIT 0
```

### final-05.log

```text
$ python3 tests/usb_allocator/run.py
PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing
PASS USB failures: 14 rollback/retention/retry scenarios plus invalid requests; no allocator calls
Host object: size=552 alignment=8 (target size/alignment checked separately in ELF)
PASS complete production USB class object: no heap/allocator-lock references

EXIT 0
```

### final-06.log

```text
$ cmake --build --preset release --parallel 4
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      223536 B       992 KB     22.01%
   text	   data	    bss	    dec	    hex	filename
 274688	    852	 341112	 616652	  968cc	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 275548 bytes, checksum 0xDB
[3/3] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================

EXIT 0
```

### final-07.log

```text
$ cmake --build --preset debug --parallel 4
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      223584 B       992 KB     22.01%
   text	   data	    bss	    dec	    hex	filename
 274736	    852	 341112	 616700	  968fc	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 275596 bytes, checksum 0xB0
[3/3] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================

EXIT 0
```

### final-08.log

```text
$ cmake --build build/diagnostics-release --parallel 4
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      223904 B       992 KB     22.04%
   text	   data	    bss	    dec	    hex	filename
 275128	    852	 341184	 617164	  96acc	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 275988 bytes, checksum 0x50
[3/3] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================

EXIT 0
```

### final-09.log

```text
$ cmake --build build/diagnostics-debug --parallel 4
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      223952 B       992 KB     22.05%
   text	   data	    bss	    dec	    hex	filename
 275176	    852	 341184	 617212	  96afc	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 276036 bytes, checksum 0x5F
[3/3] Firmware update instructions

======================================================================
  Firmware update file:
    /workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img

  To install it on the LXR:
    1. Copy LXR_V2_V1.7.img to the top folder of the SD card and
       delete any older LXR*.img files from the card.
    2. Put the card in the LXR, hold down the encoder button and
       switch the LXR on. Keep holding the button until
       'LXR Updater' appears on the display.
    3. Wait for 'Update OK!'. The LXR then restarts by itself.
======================================================================

EXIT 0
```

### final-10.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0xDB", "configuration": "release", "elf_sha256": "8671eb46bc88a625cf1f1b7dfc38ed6db1f785b04654121989f30bd59605f4c4", "image_bytes": 275564, "image_sha256": "711c9c6fbd23a32d62c33063543c9c28e5743364f7e64df10f69762a596451f6", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 222684, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275548, "reset_vector": "0x08014DED", "result": "PASS"}
{"checksum": "0xB0", "configuration": "debug", "elf_sha256": "ecf1ba25e86bbb53c2fcb2f5c4d7747b62cb929075b64089c29a8dc2a7bfa170", "image_bytes": 275612, "image_sha256": "fc0116ce9d6e7820a1404e86324d34f56c1403de207319bfd1589f9ee1ce5786", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 222732, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275596, "reset_vector": "0x08014DED", "result": "PASS"}
{"checksum": "0x50", "configuration": "diagnostics-release", "elf_sha256": "28ac01a9f077ef6cbbda7eac5d7a32892409dd1c28dc12aac0c1b2fcbd21f8e8", "image_bytes": 276004, "image_sha256": "feac2b1491eada9ce7f4906db90f675a0041d45bea2e23f31c2ea0e828730a58", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223052, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 275988, "reset_vector": "0x08014DED", "result": "PASS"}
{"checksum": "0x5F", "configuration": "diagnostics-debug", "elf_sha256": "2c0b52dfab1ec739119575e10a6ce559c0ff8c60f147debd3286c7c889aca754", "image_bytes": 276052, "image_sha256": "c4678f9842dc853458bb3b1c582b33018c154121b82a7cc79c9b29128ac49856", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223100, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 276036, "reset_vector": "0x08014DED", "result": "PASS"}

EXIT 0
```

### Final additional S2 test run

`python3 tests/sd_hal/run.py` exit0, after final test-only coverage additions.

```text
PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP
PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap
PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset
PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation
```

## Final object comparison

```text
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: object CHANGED; instructions/relocations UNCHANGED
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: object CHANGED; instructions/relocations UNCHANGED
build/debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: object CHANGED; instructions/relocations UNCHANGED
build/debug/Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj: object CHANGED
build/debug/Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj: object CHANGED
build/debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj: object CHANGED
build/debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj: object CHANGED
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: object UNCHANGED; instructions/relocations UNCHANGED
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: object UNCHANGED; instructions/relocations UNCHANGED
build/release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: object UNCHANGED; instructions/relocations UNCHANGED
build/release/Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj: object CHANGED
build/release/Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj: object CHANGED
build/release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj: object CHANGED
build/release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj: object CHANGED
build/diagnostics-release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: object UNCHANGED; instructions/relocations UNCHANGED
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: object UNCHANGED; instructions/relocations UNCHANGED
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: object UNCHANGED; instructions/relocations UNCHANGED
build/diagnostics-release/Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj: object CHANGED
build/diagnostics-release/Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj: object CHANGED
build/diagnostics-release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj: object CHANGED
build/diagnostics-release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj: object CHANGED
build/diagnostics-debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: object CHANGED; instructions/relocations UNCHANGED
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: object CHANGED; instructions/relocations UNCHANGED
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: object CHANGED; instructions/relocations UNCHANGED
build/diagnostics-debug/Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj: object CHANGED
build/diagnostics-debug/Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj: object CHANGED
build/diagnostics-debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj: object CHANGED
build/diagnostics-debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj: object CHANGED
```

## Release compiler stack-frame records

```text
/workspace/lxr-desktop-app/FATFS/Target/bsp_driver_sd.c:91:16:BSP_SD_ReadBlocks	16	static
/workspace/lxr-desktop-app/FATFS/Target/bsp_driver_sd.c:111:16:BSP_SD_WriteBlocks	16	static
/workspace/lxr-desktop-app/FATFS/Target/bsp_driver_sd.c:130:16:BSP_SD_ReadBlocks_DMA	8	static
/workspace/lxr-desktop-app/FATFS/Target/bsp_driver_sd.c:153:16:BSP_SD_WriteBlocks_DMA	8	static
/workspace/lxr-desktop-app/FATFS/Target/bsp_driver_sd.c:199:16:BSP_SD_GetCardState	8	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c:1216:10:SDMMC_GetCmdResp1.part.0	0	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c:1216:10:SDMMC_GetCmdResp1	24	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c:1339:10:SDMMC_GetCmdResp2	16	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c:1372:10:SDMMC_GetCmdResp3	16	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c:1401:10:SDMMC_GetCmdResp6	24	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c:1462:10:SDMMC_GetCmdResp7	16	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c:570:19:HAL_SD_ReadBlocks	64	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c:782:19:HAL_SD_WriteBlocks	64	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c:403:19:HAL_SD_InitCard	120	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c:335:19:HAL_SD_Init	8	static
/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c:2268:25:HAL_SD_GetCardState	8	static
```

## Complete baseline-to-final diff

This includes the decisive implementation excerpts, paths/line context and permanent test runner/fixture so another assistant can continue without the conversation.

```diff
diff --git a/Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_ll_sdmmc.h b/Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_ll_sdmmc.h
index 4122a81..9f2f7b4 100644
--- a/Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_ll_sdmmc.h
+++ b/Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_ll_sdmmc.h
@@ -320,7 +320,7 @@ typedef struct
 
 #define SDMMC_CMDTIMEOUT                       5000U         /* Command send and response timeout */
 #define SDMMC_MAXERASETIMEOUT                  63000U        /* Max erase Timeout 63 s            */
-#define SDMMC_STOPTRANSFERTIMEOUT              100000000U    /* Timeout for STOP TRANSMISSION command */
+#define SDMMC_STOPTRANSFERTIMEOUT              5000U    /* Timeout for STOP TRANSMISSION command */
 
 /** @defgroup SDMMC_LL_Clock_Edge Clock Edge
   * @{
diff --git a/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c b/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c
index 4b9e95d..f2a77ee 100644
--- a/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c
+++ b/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c
@@ -631,8 +631,8 @@ HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint3
       /* Clear all the static flags */
       __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
       hsd->ErrorCode |= errorstate;
-      hsd->State = HAL_SD_STATE_READY;
-      hsd->Context = SD_CONTEXT_NONE;
+      hsd->State = HAL_SD_STATE_ERROR;
+      /* Retain attempted-transfer context; physical quiescence is unproven. */
       return HAL_ERROR;
     }
 
@@ -666,8 +666,8 @@ HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint3
         /* Clear all the static flags */
         __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
         hsd->ErrorCode |= HAL_SD_ERROR_TIMEOUT;
-        hsd->State= HAL_SD_STATE_READY;
-        hsd->Context = SD_CONTEXT_NONE;
+        hsd->State = HAL_SD_STATE_TIMEOUT;
+        /* Retain attempted-transfer context; physical quiescence is unproven. */
         return HAL_TIMEOUT;
       }
     }
@@ -684,8 +684,8 @@ HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint3
           /* Clear all the static flags */
           __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
           hsd->ErrorCode |= errorstate;
-          hsd->State = HAL_SD_STATE_READY;
-          hsd->Context = SD_CONTEXT_NONE;
+          hsd->State = HAL_SD_STATE_ERROR;
+          /* Retain attempted-transfer context; physical quiescence is unproven. */
           return HAL_ERROR;
         }
       }
@@ -697,8 +697,8 @@ HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint3
       /* Clear all the static flags */
       __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
       hsd->ErrorCode |= HAL_SD_ERROR_DATA_TIMEOUT;
-      hsd->State = HAL_SD_STATE_READY;
-      hsd->Context = SD_CONTEXT_NONE;
+      hsd->State = HAL_SD_STATE_ERROR;
+      /* Retain attempted-transfer context; physical quiescence is unproven. */
       return HAL_ERROR;
     }
     else if(__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DCRCFAIL))
@@ -706,8 +706,8 @@ HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint3
       /* Clear all the static flags */
       __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
       hsd->ErrorCode |= HAL_SD_ERROR_DATA_CRC_FAIL;
-      hsd->State = HAL_SD_STATE_READY;
-      hsd->Context = SD_CONTEXT_NONE;
+      hsd->State = HAL_SD_STATE_ERROR;
+      /* Retain attempted-transfer context; physical quiescence is unproven. */
       return HAL_ERROR;
     }
     else if(__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_RXOVERR))
@@ -715,8 +715,8 @@ HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint3
       /* Clear all the static flags */
       __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
       hsd->ErrorCode |= HAL_SD_ERROR_RX_OVERRUN;
-      hsd->State = HAL_SD_STATE_READY;
-      hsd->Context = SD_CONTEXT_NONE;
+      hsd->State = HAL_SD_STATE_ERROR;
+      /* Retain attempted-transfer context; physical quiescence is unproven. */
       return HAL_ERROR;
     }
     else
@@ -746,9 +746,9 @@ HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint3
         /* Clear all the static flags */
         __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
         hsd->ErrorCode |= HAL_SD_ERROR_TIMEOUT;
-        hsd->State= HAL_SD_STATE_READY;
-        hsd->Context = SD_CONTEXT_NONE;
-        return HAL_ERROR;
+        hsd->State = HAL_SD_STATE_TIMEOUT;
+        /* Retain attempted-transfer context; physical quiescence is unproven. */
+        return HAL_TIMEOUT;
       }
     }
 
@@ -756,6 +756,7 @@ HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint3
     __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS);
 
     hsd->State = HAL_SD_STATE_READY;
+    hsd->Context = SD_CONTEXT_NONE;
 
     return HAL_OK;
   }
@@ -842,8 +843,8 @@ HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint
       /* Clear all the static flags */
       __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
       hsd->ErrorCode |= errorstate;
-      hsd->State = HAL_SD_STATE_READY;
-      hsd->Context = SD_CONTEXT_NONE;
+      hsd->State = HAL_SD_STATE_ERROR;
+      /* Retain attempted-transfer context; physical quiescence is unproven. */
       return HAL_ERROR;
     }
 
@@ -876,9 +877,9 @@ HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint
       {
         /* Clear all the static flags */
         __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
-        hsd->ErrorCode |= errorstate;
-        hsd->State = HAL_SD_STATE_READY;
-        hsd->Context = SD_CONTEXT_NONE;
+        hsd->ErrorCode |= HAL_SD_ERROR_TIMEOUT;
+        hsd->State = HAL_SD_STATE_TIMEOUT;
+        /* Retain attempted-transfer context; physical quiescence is unproven. */
         return HAL_TIMEOUT;
       }
     }
@@ -895,8 +896,8 @@ HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint
           /* Clear all the static flags */
           __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
           hsd->ErrorCode |= errorstate;
-          hsd->State = HAL_SD_STATE_READY;
-          hsd->Context = SD_CONTEXT_NONE;
+          hsd->State = HAL_SD_STATE_ERROR;
+          /* Retain attempted-transfer context; physical quiescence is unproven. */
           return HAL_ERROR;
         }
       }
@@ -908,8 +909,8 @@ HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint
       /* Clear all the static flags */
       __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
       hsd->ErrorCode |= HAL_SD_ERROR_DATA_TIMEOUT;
-      hsd->State = HAL_SD_STATE_READY;
-      hsd->Context = SD_CONTEXT_NONE;
+      hsd->State = HAL_SD_STATE_ERROR;
+      /* Retain attempted-transfer context; physical quiescence is unproven. */
       return HAL_ERROR;
     }
     else if(__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_DCRCFAIL))
@@ -917,8 +918,8 @@ HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint
       /* Clear all the static flags */
       __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
       hsd->ErrorCode |= HAL_SD_ERROR_DATA_CRC_FAIL;
-      hsd->State = HAL_SD_STATE_READY;
-      hsd->Context = SD_CONTEXT_NONE;
+      hsd->State = HAL_SD_STATE_ERROR;
+      /* Retain attempted-transfer context; physical quiescence is unproven. */
       return HAL_ERROR;
     }
     else if(__HAL_SD_GET_FLAG(hsd, SDMMC_FLAG_TXUNDERR))
@@ -926,8 +927,8 @@ HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint
       /* Clear all the static flags */
       __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
       hsd->ErrorCode |= HAL_SD_ERROR_TX_UNDERRUN;
-      hsd->State = HAL_SD_STATE_READY;
-      hsd->Context = SD_CONTEXT_NONE;
+      hsd->State = HAL_SD_STATE_ERROR;
+      /* Retain attempted-transfer context; physical quiescence is unproven. */
       return HAL_ERROR;
     }
     else
@@ -939,6 +940,7 @@ HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint
     __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_DATA_FLAGS);
 
     hsd->State = HAL_SD_STATE_READY;
+    hsd->Context = SD_CONTEXT_NONE;
 
     return HAL_OK;
   }
@@ -2273,6 +2275,7 @@ HAL_SD_CardStateTypeDef HAL_SD_GetCardState(SD_HandleTypeDef *hsd)
   if(errorstate != HAL_SD_ERROR_NONE)
   {
     hsd->ErrorCode |= errorstate;
+    return HAL_SD_CARD_ERROR;
   }
 
   cardstate = ((resp1 >> 9U) & 0x0FU);
@@ -2711,6 +2714,7 @@ static uint32_t SD_PowerON(SD_HandleTypeDef *hsd)
   __IO uint32_t count = 0U;
   uint32_t response = 0U, validvoltage = 0U;
   uint32_t errorstate;
+  uint32_t voltage_start;
 
   /* CMD0: GO_IDLE_STATE */
   errorstate = SDMMC_CmdGoIdleState(hsd->Instance);
@@ -2748,6 +2752,7 @@ static uint32_t SD_PowerON(SD_HandleTypeDef *hsd)
   }
   /* SD CARD */
   /* Send ACMD41 SD_APP_OP_COND with Argument 0x80100000 */
+  voltage_start = HAL_GetTick();
   while((count < SDMMC_MAX_VOLT_TRIAL) && (validvoltage == 0U))
   {
     /* SEND CMD55 APP_CMD with RCA as 0 */
@@ -2771,6 +2776,9 @@ static uint32_t SD_PowerON(SD_HandleTypeDef *hsd)
     validvoltage = (((response >> 31U) == 1U) ? 1U : 0U);
 
     count++;
+    /* Bound repeated successful commands that never establish voltage readiness. */
+    if(validvoltage == 0U && (uint32_t)(HAL_GetTick() - voltage_start) >= SDMMC_CMDTIMEOUT)
+      return HAL_SD_ERROR_TIMEOUT;
   }
 
   if(count >= SDMMC_MAX_VOLT_TRIAL)
diff --git a/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c b/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c
index 82893db..63fdda9 100644
--- a/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c
+++ b/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c
@@ -1188,24 +1188,37 @@ uint32_t SDMMC_CmdSendEXTCSD(SDMMC_TypeDef *SDMMCx, uint32_t Argument)
   * @param  SD_CMD: The sent command index  
   * @retval SD Card error state
   */
+/* Command-response software milliseconds, independent of SDMMC DTIMER.
+ * Active firmware callers run after HAL_Init with an advancing SysTick.
+ * Reject pre-tick/masked/ISR calls; no unbounded or cycle-count fallback.
+ * The cap also protects direct callers passing oversized timeout values.
+ */
+static uint32_t SDMMC_WaitCommand(SDMMC_TypeDef *SDMMCx, uint32_t flags, uint32_t timeout)
+{
+  uint32_t start;
+  if(timeout == 0U || __get_IPSR() != 0U || __get_PRIMASK() != 0U ||
+     __get_BASEPRI() != 0U || __get_FAULTMASK() != 0U ||
+     (SysTick->CTRL & (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk)) !=
+     (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk))
+    return SDMMC_ERROR_TIMEOUT;
+  if(timeout > SDMMC_CMDTIMEOUT) timeout = SDMMC_CMDTIMEOUT;
+  start = HAL_GetTick();
+  for(;;) {
+    uint32_t status = SDMMCx->STA;
+    if((status & flags) != 0U &&
+       (flags == SDMMC_FLAG_CMDSENT || (status & SDMMC_FLAG_CMDACT) == 0U))
+      return SDMMC_ERROR_NONE;
+    if((uint32_t)(HAL_GetTick() - start) >= timeout)
+      return SDMMC_ERROR_TIMEOUT;
+  }
+}
+
 uint32_t SDMMC_GetCmdResp1(SDMMC_TypeDef *SDMMCx, uint8_t SD_CMD, uint32_t Timeout)
 {
   uint32_t response_r1;
-  uint32_t sta_reg;
-  
-  /* 8 is the number of required instructions cycles for the below loop statement.
-  The Timeout is expressed in ms */
-  uint32_t count = Timeout * (SystemCoreClock / 8U /1000U);
-  
-  do
-  {
-    if (count-- == 0U)
-    {
-      return SDMMC_ERROR_TIMEOUT;
-    }
-    sta_reg = SDMMCx->STA;
-  }while(((sta_reg & (SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND | SDMMC_FLAG_CTIMEOUT)) == 0U) ||
-         ((sta_reg & SDMMC_FLAG_CMDACT) != 0U ));
+  if(SDMMC_WaitCommand(SDMMCx, SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND |
+                       SDMMC_FLAG_CTIMEOUT, Timeout) != SDMMC_ERROR_NONE)
+    return SDMMC_ERROR_TIMEOUT;
     
   if(__SDMMC_GET_FLAG(SDMMCx, SDMMC_FLAG_CTIMEOUT))
   {
@@ -1325,20 +1338,9 @@ uint32_t SDMMC_GetCmdResp1(SDMMC_TypeDef *SDMMCx, uint8_t SD_CMD, uint32_t Timeo
   */
 uint32_t SDMMC_GetCmdResp2(SDMMC_TypeDef *SDMMCx)
 {
-  uint32_t sta_reg;
-  /* 8 is the number of required instructions cycles for the below loop statement.
-  The SDMMC_CMDTIMEOUT is expressed in ms */
-  uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
-  
-  do
-  {
-    if (count-- == 0U)
-    {
-      return SDMMC_ERROR_TIMEOUT;
-    }
-    sta_reg = SDMMCx->STA;
-  }while(((sta_reg & (SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND | SDMMC_FLAG_CTIMEOUT)) == 0U) ||
-         ((sta_reg & SDMMC_FLAG_CMDACT) != 0U ));
+  if(SDMMC_WaitCommand(SDMMCx, SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND |
+                       SDMMC_FLAG_CTIMEOUT, SDMMC_CMDTIMEOUT) != SDMMC_ERROR_NONE)
+    return SDMMC_ERROR_TIMEOUT;
     
   if (__SDMMC_GET_FLAG(SDMMCx, SDMMC_FLAG_CTIMEOUT))
   {
@@ -1369,20 +1371,9 @@ uint32_t SDMMC_GetCmdResp2(SDMMC_TypeDef *SDMMCx)
   */
 uint32_t SDMMC_GetCmdResp3(SDMMC_TypeDef *SDMMCx)
 {
-  uint32_t sta_reg;
-  /* 8 is the number of required instructions cycles for the below loop statement.
-  The SDMMC_CMDTIMEOUT is expressed in ms */
-  uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
-  
-  do
-  {
-    if (count-- == 0U)
-    {
-      return SDMMC_ERROR_TIMEOUT;
-    }
-    sta_reg = SDMMCx->STA;
-  }while(((sta_reg & (SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND | SDMMC_FLAG_CTIMEOUT)) == 0U) ||
-         ((sta_reg & SDMMC_FLAG_CMDACT) != 0U ));
+  if(SDMMC_WaitCommand(SDMMCx, SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND |
+                       SDMMC_FLAG_CTIMEOUT, SDMMC_CMDTIMEOUT) != SDMMC_ERROR_NONE)
+    return SDMMC_ERROR_TIMEOUT;
     
   if(__SDMMC_GET_FLAG(SDMMCx, SDMMC_FLAG_CTIMEOUT))
   {
@@ -1410,21 +1401,9 @@ uint32_t SDMMC_GetCmdResp3(SDMMC_TypeDef *SDMMCx)
 uint32_t SDMMC_GetCmdResp6(SDMMC_TypeDef *SDMMCx, uint8_t SD_CMD, uint16_t *pRCA)
 {
   uint32_t response_r1;
-  uint32_t sta_reg;
-
-  /* 8 is the number of required instructions cycles for the below loop statement.
-  The SDMMC_CMDTIMEOUT is expressed in ms */
-  uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
-  
-  do
-  {
-    if (count-- == 0U)
-    {
-      return SDMMC_ERROR_TIMEOUT;
-    }
-    sta_reg = SDMMCx->STA;
-  }while(((sta_reg & (SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND | SDMMC_FLAG_CTIMEOUT)) == 0U) ||
-         ((sta_reg & SDMMC_FLAG_CMDACT) != 0U ));
+  if(SDMMC_WaitCommand(SDMMCx, SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND |
+                       SDMMC_FLAG_CTIMEOUT, SDMMC_CMDTIMEOUT) != SDMMC_ERROR_NONE)
+    return SDMMC_ERROR_TIMEOUT;
     
   if(__SDMMC_GET_FLAG(SDMMCx, SDMMC_FLAG_CTIMEOUT))
   {
@@ -1482,20 +1461,9 @@ uint32_t SDMMC_GetCmdResp6(SDMMC_TypeDef *SDMMCx, uint8_t SD_CMD, uint16_t *pRCA
   */
 uint32_t SDMMC_GetCmdResp7(SDMMC_TypeDef *SDMMCx)
 {
-  uint32_t sta_reg;
-  /* 8 is the number of required instructions cycles for the below loop statement.
-  The SDMMC_CMDTIMEOUT is expressed in ms */
-  uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
-  
-  do
-  {
-    if (count-- == 0U)
-    {
-      return SDMMC_ERROR_TIMEOUT;
-    }
-    sta_reg = SDMMCx->STA;
-  }while(((sta_reg & (SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND | SDMMC_FLAG_CTIMEOUT)) == 0U) ||
-         ((sta_reg & SDMMC_FLAG_CMDACT) != 0U ));
+  if(SDMMC_WaitCommand(SDMMCx, SDMMC_FLAG_CCRCFAIL | SDMMC_FLAG_CMDREND |
+                       SDMMC_FLAG_CTIMEOUT, SDMMC_CMDTIMEOUT) != SDMMC_ERROR_NONE)
+    return SDMMC_ERROR_TIMEOUT;
     
   if(__SDMMC_GET_FLAG(SDMMCx, SDMMC_FLAG_CTIMEOUT))
   {
@@ -1542,18 +1510,8 @@ uint32_t SDMMC_GetCmdResp7(SDMMC_TypeDef *SDMMCx)
   */
 static uint32_t SDMMC_GetCmdError(SDMMC_TypeDef *SDMMCx)
 {
-  /* 8 is the number of required instructions cycles for the below loop statement.
-  The SDMMC_CMDTIMEOUT is expressed in ms */
-  uint32_t count = SDMMC_CMDTIMEOUT * (SystemCoreClock / 8U /1000U);
-  
-  do
-  {
-    if (count-- == 0U)
-    {
-      return SDMMC_ERROR_TIMEOUT;
-    }
-    
-  }while(!__SDMMC_GET_FLAG(SDMMCx, SDMMC_FLAG_CMDSENT));
+  if(SDMMC_WaitCommand(SDMMCx, SDMMC_FLAG_CMDSENT, SDMMC_CMDTIMEOUT) != SDMMC_ERROR_NONE)
+    return SDMMC_ERROR_TIMEOUT;
   
   /* Clear all the static flags */
   __SDMMC_CLEAR_FLAG(SDMMCx, SDMMC_STATIC_CMD_FLAGS);
diff --git a/FATFS/Target/bsp_driver_sd.c b/FATFS/Target/bsp_driver_sd.c
index 8ab3801..4efc31a 100644
--- a/FATFS/Target/bsp_driver_sd.c
+++ b/FATFS/Target/bsp_driver_sd.c
@@ -90,14 +90,11 @@ __weak uint8_t BSP_SD_ITConfig(void)
   */
 __weak uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout)
 {
-  uint8_t sd_state = MSD_OK;
-
-  if (HAL_SD_ReadBlocks(&hsd1, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout) != HAL_OK)
-  {
-    sd_state = MSD_ERROR;
-  }
-
-  return sd_state;
+  HAL_StatusTypeDef status = HAL_SD_ReadBlocks(&hsd1, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout);
+  if(status == HAL_OK) return MSD_OK;
+  if(status == HAL_TIMEOUT) return MSD_TIMEOUT;
+  if(status == HAL_BUSY) return MSD_BUSY;
+  return MSD_ERROR;
 }
 
 /* USER CODE BEGIN BeforeWriteBlocksSection */
@@ -113,14 +110,11 @@ __weak uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t Nu
   */
 __weak uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout)
 {
-  uint8_t sd_state = MSD_OK;
-
-  if (HAL_SD_WriteBlocks(&hsd1, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout) != HAL_OK)
-  {
-    sd_state = MSD_ERROR;
-  }
-
-  return sd_state;
+  HAL_StatusTypeDef status = HAL_SD_WriteBlocks(&hsd1, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout);
+  if(status == HAL_OK) return MSD_OK;
+  if(status == HAL_TIMEOUT) return MSD_TIMEOUT;
+  if(status == HAL_BUSY) return MSD_BUSY;
+  return MSD_ERROR;
 }
 
 /* USER CODE BEGIN BeforeReadDMABlocksSection */
@@ -204,7 +198,15 @@ __weak uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr)
   */
 __weak uint8_t BSP_SD_GetCardState(void)
 {
-  return ((HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
+  HAL_SD_CardStateTypeDef state = HAL_SD_GetCardState(&hsd1);
+  /* Do not erase prior relevant errors merely because a later command succeeds. */
+  if(hsd1.ErrorCode != HAL_SD_ERROR_NONE || state == HAL_SD_CARD_ERROR)
+    return SD_TRANSFER_ERROR;
+  if(state == HAL_SD_CARD_TRANSFER) return SD_TRANSFER_OK;
+  /* Valid transitional states are not evidence of a failed status command. */
+  if(state >= HAL_SD_CARD_READY && state <= HAL_SD_CARD_DISCONNECTED)
+    return SD_TRANSFER_BUSY;
+  return SD_TRANSFER_ERROR;
 }
 
 /**
diff --git a/FATFS/Target/bsp_driver_sd.h b/FATFS/Target/bsp_driver_sd.h
index f154e0e..95ad7db 100644
--- a/FATFS/Target/bsp_driver_sd.h
+++ b/FATFS/Target/bsp_driver_sd.h
@@ -42,12 +42,15 @@
 #define   MSD_OK                        ((uint8_t)0x00)
 #define   MSD_ERROR                     ((uint8_t)0x01)
 #define   MSD_ERROR_SD_NOT_PRESENT      ((uint8_t)0x02)
+#define   MSD_TIMEOUT                   ((uint8_t)0x03)
+#define   MSD_BUSY                      ((uint8_t)0x04)
 
 /**
   * @brief  SD transfer state definition
   */
 #define   SD_TRANSFER_OK                ((uint8_t)0x00)
 #define   SD_TRANSFER_BUSY              ((uint8_t)0x01)
+#define   SD_TRANSFER_ERROR             ((uint8_t)0x02)
 
 #define SD_PRESENT               ((uint8_t)0x01)
 #define SD_NOT_PRESENT           ((uint8_t)0x00)
diff --git a/FATFS/Target/sd_diskio.c b/FATFS/Target/sd_diskio.c
index 874b508..4cb6af3 100644
--- a/FATFS/Target/sd_diskio.c
+++ b/FATFS/Target/sd_diskio.c
@@ -73,13 +73,16 @@ static DRESULT SD_Access(BYTE lun)
 static DRESULT SD_WaitReady(void)
 {
   uint32_t start = HAL_GetTick();
+  uint8_t state;
   for(;;) {
     if(BSP_SD_IsDetected() != SD_PRESENT) {
       (void)SD_Fail();
       Stat |= STA_NODISK;
       return RES_NOTRDY;
     }
-    if(BSP_SD_GetCardState() == MSD_OK) return RES_OK;
+    state = BSP_SD_GetCardState();
+    if(state == SD_TRANSFER_OK) return RES_OK;
+    if(state == SD_TRANSFER_ERROR) return SD_Fail();
     if((uint32_t)(HAL_GetTick() - start) >= SD_READY_TIMEOUT_MS)
       return SD_Fail();
   }
diff --git a/tests/sd_hal/README.md b/tests/sd_hal/README.md
new file mode 100644
index 0000000..6018124
--- /dev/null
+++ b/tests/sd_hal/README.md
@@ -0,0 +1,41 @@
+# S2 SD HAL/BSP regression tests
+
+Run `python3 tests/sd_hal/run.py` with the prepared ARM compiler on PATH. Native
+ASan/UBSan executes actual extracted production function bodies: LL wait helper,
+R1/R2/R3/R6/R7/CMD0 response handlers and STOP command; polling HAL read/write,
+voltage negotiation, SD_SendStatus and card-state query; BSP read/write/state wrappers. Extraction is
+brace-balanced and selects definitions, not declarations. Target preprocessing
+provides actual constant values; small CMSIS/register/handle shapes and function
+behavior are explicit mocks in host.c. BLOCKSIZE=512 and HAL state enum values
+model this target. Complete production translation units are additionally ARM-built.
+
+The register model covers command completion, CMDACT, no response, hardware
+command timeout, CRC, the R3 CRC exception, R1/R6 index mismatch, zero/oversized
+software timeout, tick wrap, pre-tick rejection, STOP failure, data FIFO success,
+polling timeout/CRC failure, preserved context and non-ready state, no direct
+retry, BSP timeout fidelity and busy/error distinction. S1's whole-driver tests
+also verify an explicit BSP state error immediately latches and cannot be cleared.
+No HAL abort/reset/recovery functions are supplied by the test model; an accidental
+new dependency fails native linking. Call counts detect replay in tested paths.
+
+LeakSanitizer is disabled because the cloud executor's tracing cannot support it;
+ASan/UBSan remain enabled and errors are fatal. A 20-second process deadline bounds
+test execution. The model does not emulate electrical timing, card programming,
+DMA, hardware clock failure, exact W1C behavior, bus contention or physical
+quiescence. It does not simulate a physical SD card or prove firmware audio safety.
+
+Command deadlines are milliseconds capped at 5000; no CPU-clock multiplication
+or spin-count fallback. They require foreground, unmasked, enabled/advancing
+SysTick. Calls before timebase setup or from disallowed contexts fail closed.
+A halted/malfunctioning tick despite enabled control bits, or indefinite interrupt
+starvation, remains outside the software timing assumptions. Aggregate startup
+and a whole file operation can contain multiple bounded command calls; the
+public transfer timeout is not a single shared deadline for all nested calls.
+Voltage acquisition also has a 5000 ms outer retry allowance, in addition to
+the existing finite trial count; one nested command pair can overrun it before
+the next check. SDMMC_DATATIMEOUT is unchanged.
+
+S2 intentionally adds no physical abort, STOP-on-error, peripheral reset or
+recovery/retry. Failure state/context reports an incomplete attempt, not proof the
+card/data path has stopped. S1's reboot-only disk latch remains in force. Higher
+FatFs/save-call propagation and filesystem ownership remain later-phase work.
diff --git a/tests/sd_hal/host.c b/tests/sd_hal/host.c
new file mode 100644
index 0000000..a3fe4f8
--- /dev/null
+++ b/tests/sd_hal/host.c
@@ -0,0 +1,157 @@
+/* Native register model: tests actual extracted bodies, never physical SD. */
+#include <stdint.h>
+#include <assert.h>
+#include <stdio.h>
+#include <string.h>
+#include "constants.h"
+#ifndef BLOCKSIZE
+#define BLOCKSIZE 512u
+#endif
+#ifndef HAL_SD_CARD_TRANSFER
+#define HAL_SD_CARD_TRANSFER 4u
+#define HAL_SD_CARD_SENDING 5u
+#define HAL_SD_CARD_RECEIVING 6u
+#define HAL_SD_CARD_PROGRAMMING 7u
+#define HAL_SD_CARD_ERROR 255u
+#endif
+typedef uint32_t HAL_SD_CardStateTypeDef;
+typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
+enum {HAL_SD_STATE_RESET,HAL_SD_STATE_READY,HAL_SD_STATE_TIMEOUT,HAL_SD_STATE_BUSY,HAL_SD_STATE_ERROR=15};
+typedef struct { volatile uint32_t STA,ICR,DCTRL,RESPCMD,RESP1; } SDMMC_TypeDef;
+typedef struct { uint32_t DataTimeOut,DataLength,DataBlockSize,TransferDir,TransferMode,DPSM; } SDMMC_DataInitTypeDef;
+typedef struct { uint32_t Argument,CmdIndex,Response,WaitForInterrupt,CPSM; } SDMMC_CmdInitTypeDef;
+typedef struct { SDMMC_TypeDef *Instance;uint32_t State,ErrorCode,Context;struct {uint32_t LogBlockNbr,CardType,RelCardAdd,CardVersion;} SdCard; } SD_HandleTypeDef;
+static struct { uint32_t CTRL; } systick={3};
+#define SysTick (&systick)
+#define SysTick_CTRL_ENABLE_Msk 1u
+#define SysTick_CTRL_TICKINT_Msk 2u
+static uint32_t ipsr,primask,basepri,faultmask;
+#define __IO volatile
+#define __get_IPSR() ipsr
+#define __get_PRIMASK() primask
+#define __get_BASEPRI() basepri
+#define __get_FAULTMASK() faultmask
+static uint32_t tick,remaining,commands,status_error,status_response,command_error,stop_fail;
+static uint32_t voltage_response;
+static unsigned transfer_mode; /* 0 timeout, 1 FIFO, 2 CRC, 3 hardware timeout, 4 read FIFO tail. */
+static SDMMC_TypeDef regs;
+SD_HandleTypeDef hsd1;
+static uint32_t HAL_GetTick(void){return tick++;}
+#define __SDMMC_GET_FLAG(r,f) ((r)->STA & (f))
+#define __SDMMC_CLEAR_FLAG(r,f) do {(r)->ICR=(f);(r)->STA &= ~(f);}while(0)
+static uint32_t data_flag(SD_HandleTypeDef *h,uint32_t mask)
+{
+ (void)h;
+ if(transfer_mode==2) return mask & SDMMC_FLAG_DCRCFAIL;
+ if(transfer_mode==3) return mask & SDMMC_FLAG_DTIMEOUT;
+ if(transfer_mode==4 && (mask & SDMMC_FLAG_DATAEND)) return SDMMC_FLAG_DATAEND;
+ if(!transfer_mode) return 0;
+ if(mask & (SDMMC_FLAG_DATAEND | SDMMC_FLAG_RXOVERR)) return remaining==0 ? (mask & SDMMC_FLAG_DATAEND):0;
+ if(mask & (SDMMC_FLAG_RXFIFOHF|SDMMC_FLAG_TXFIFOHE|SDMMC_FLAG_RXDAVL)) return remaining?mask:0;
+ return 0;
+}
+#define __HAL_SD_GET_FLAG(h,f) data_flag(h,f)
+#define __HAL_SD_CLEAR_FLAG(h,f) do {(h)->Instance->ICR=(f);}while(0)
+static uint32_t SDMMC_GetCommandResponse(SDMMC_TypeDef *r){return r->RESPCMD;}
+static uint32_t SDMMC_GetResponse(SDMMC_TypeDef *r,uint32_t n){(void)n;return r->RESP1;}
+static uint32_t SDMMC_ConfigData(SDMMC_TypeDef *r,SDMMC_DataInitTypeDef *c){remaining=c->DataLength;r->DCTRL=c->DPSM;return 0;}
+static uint32_t SDMMC_ReadFIFO(SDMMC_TypeDef *r){(void)r;assert(remaining>=4);remaining-=4;return 0x12345678;}
+static uint32_t SDMMC_WriteFIFO(SDMMC_TypeDef *r,uint32_t *v){(void)r;(void)v;assert(remaining>=4);remaining-=4;return 0;}
+static uint32_t transfer_command(SDMMC_TypeDef *r,uint32_t a){(void)r;(void)a;commands++;return command_error;}
+#define SDMMC_CmdReadMultiBlock transfer_command
+#define SDMMC_CmdReadSingleBlock transfer_command
+#define SDMMC_CmdWriteMultiBlock transfer_command
+#define SDMMC_CmdWriteSingleBlock transfer_command
+static uint32_t SDMMC_CmdSendStatus(SDMMC_TypeDef *r,uint32_t a){(void)a;r->RESP1=status_response;return status_error;}
+static uint32_t SDMMC_SendCommand(SDMMC_TypeDef *r,SDMMC_CmdInitTypeDef *c){commands++;r->RESPCMD=c->CmdIndex;r->RESP1=0;r->STA=stop_fail?0:SDMMC_FLAG_CMDREND;return 0;}
+static uint32_t SDMMC_CmdGoIdleState(SDMMC_TypeDef *r){return transfer_command(r,0);}
+static uint32_t SDMMC_CmdOperCond(SDMMC_TypeDef *r){return transfer_command(r,0);}
+static uint32_t SDMMC_CmdAppCommand(SDMMC_TypeDef *r,uint32_t a){return transfer_command(r,a);}
+static uint32_t SDMMC_CmdAppOperCommand(SDMMC_TypeDef *r,uint32_t a){r->RESP1=voltage_response;return transfer_command(r,a);}
+#include "production.c"
+static void reset(void)
+{
+ memset(&regs,0,sizeof regs);memset(&hsd1,0,sizeof hsd1);hsd1.Instance=&regs;
+ hsd1.State=HAL_SD_STATE_READY;hsd1.SdCard.LogBlockNbr=1024;hsd1.SdCard.CardType=CARD_SDHC_SDXC;
+ tick=remaining=commands=status_error=status_response=command_error=stop_fail=0;transfer_mode=0;voltage_response=0;
+ ipsr=primask=basepri=faultmask=0;systick.CTRL=3;
+}
+static uint32_t response(unsigned type)
+{
+ uint16_t rca=0;
+ switch(type){case 1:return SDMMC_GetCmdResp1(&regs,13,5000);case 2:return SDMMC_GetCmdResp2(&regs);
+ case 3:return SDMMC_GetCmdResp3(&regs);case 6:return SDMMC_GetCmdResp6(&regs,13,&rca);
+ case 7:return SDMMC_GetCmdResp7(&regs);default:return SDMMC_GetCmdError(&regs);}
+}
+int main(void)
+{
+ const unsigned types[]={0,1,2,3,6,7};
+ for(unsigned n=0;n<6;n++) {
+  unsigned type=types[n];reset();regs.RESPCMD=13;regs.STA=type?SDMMC_FLAG_CMDREND:SDMMC_FLAG_CMDSENT;
+  assert(response(type)==SDMMC_ERROR_NONE);
+  reset();tick=UINT32_MAX-2500;assert(response(type)==SDMMC_ERROR_TIMEOUT && tick<3000);
+  reset();regs.RESPCMD=13;regs.STA=SDMMC_FLAG_CMDREND|SDMMC_FLAG_CMDACT;assert(response(type)==SDMMC_ERROR_TIMEOUT);
+  reset();systick.CTRL=0;assert(response(type)==SDMMC_ERROR_TIMEOUT && tick==0);
+ }
+ for(unsigned n=1;n<6;n++) {
+  unsigned type=types[n];reset();regs.STA=SDMMC_FLAG_CTIMEOUT;assert(response(type)==SDMMC_ERROR_CMD_RSP_TIMEOUT);
+  reset();regs.STA=SDMMC_FLAG_CCRCFAIL;regs.RESPCMD=13;
+  assert(response(type)==(type==3?SDMMC_ERROR_NONE:SDMMC_ERROR_CMD_CRC_FAIL));
+ }
+ reset();regs.STA=SDMMC_FLAG_CMDREND;regs.RESPCMD=99;
+ assert(SDMMC_GetCmdResp1(&regs,13,5000)==SDMMC_ERROR_CMD_CRC_FAIL);
+ uint16_t rca=123;regs.STA=SDMMC_FLAG_CMDREND;assert(SDMMC_GetCmdResp6(&regs,13,&rca)==SDMMC_ERROR_CMD_CRC_FAIL && rca==123);
+ reset();assert(SDMMC_GetCmdResp1(&regs,13,UINT32_MAX)==SDMMC_ERROR_TIMEOUT && tick==5001);
+ reset();assert(SDMMC_GetCmdResp1(&regs,13,0)==SDMMC_ERROR_TIMEOUT && tick==0);
+ reset();stop_fail=1;assert(SDMMC_CmdStopTransfer(&regs)==SDMMC_ERROR_TIMEOUT && commands==1 && tick==5001);
+ uint32_t *masks[]={&ipsr,&primask,&basepri,&faultmask};
+ for(unsigned n=0;n<4;n++){reset();*masks[n]=1;assert(SDMMC_GetCmdResp1(&regs,13,5000)==SDMMC_ERROR_TIMEOUT && tick==0);}
+ puts("PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP");
+ reset();tick=UINT32_MAX-2500;assert(SD_PowerON(&hsd1)==HAL_SD_ERROR_TIMEOUT && tick<3000);
+ reset();voltage_response=0x80000000u|SDMMC_HIGH_CAPACITY;assert(SD_PowerON(&hsd1)==HAL_SD_ERROR_NONE && hsd1.SdCard.CardType==CARD_SDHC_SDXC);
+ reset();command_error=SDMMC_ERROR_CMD_RSP_TIMEOUT;assert(SD_PowerON(&hsd1)==command_error && commands==1);
+ puts("PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap");
+ uint8_t data[1024];memset(data,0x5a,sizeof data);
+ for(unsigned write=0;write<2;write++) {
+  reset();tick=UINT32_MAX-2;
+  HAL_StatusTypeDef r=write?HAL_SD_WriteBlocks(&hsd1,data,0,2,4):HAL_SD_ReadBlocks(&hsd1,data,0,2,4);
+  assert(regs.DCTRL==SDMMC_DPSM_ENABLE);
+  assert(r==HAL_TIMEOUT && (hsd1.ErrorCode & HAL_SD_ERROR_TIMEOUT) && hsd1.State==HAL_SD_STATE_TIMEOUT && hsd1.Context!=SD_CONTEXT_NONE && commands==1);
+  uint32_t context=hsd1.Context;unsigned before=commands;
+  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,2,4):HAL_SD_ReadBlocks(&hsd1,data,0,2,4);
+  assert(r!=HAL_OK && context==hsd1.Context && commands==before);
+  reset();command_error=SDMMC_ERROR_CMD_CRC_FAIL;
+  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,1,5000):HAL_SD_ReadBlocks(&hsd1,data,0,1,5000);
+  assert(r==HAL_ERROR && hsd1.State==HAL_SD_STATE_ERROR && (hsd1.ErrorCode & command_error) && hsd1.Context!=0 && commands==1);
+  reset();transfer_mode=2;
+  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,1,5000):HAL_SD_ReadBlocks(&hsd1,data,0,1,5000);
+  assert(r==HAL_ERROR && hsd1.State==HAL_SD_STATE_ERROR && (hsd1.ErrorCode & HAL_SD_ERROR_DATA_CRC_FAIL));
+  reset();transfer_mode=3;
+  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,1,5000):HAL_SD_ReadBlocks(&hsd1,data,0,1,5000);
+  assert(r==HAL_ERROR && hsd1.State==HAL_SD_STATE_ERROR && (hsd1.ErrorCode & HAL_SD_ERROR_DATA_TIMEOUT));
+  for(unsigned count=1;count<=2;count++) {
+   reset();transfer_mode=1;
+   r=write?HAL_SD_WriteBlocks(&hsd1,data,0,count,5000):HAL_SD_ReadBlocks(&hsd1,data,0,count,5000);
+   assert(r==HAL_OK && hsd1.State==HAL_SD_STATE_READY && hsd1.Context==SD_CONTEXT_NONE && remaining==0 && commands==count);
+  }
+  reset();transfer_mode=1;stop_fail=1;
+  r=write?HAL_SD_WriteBlocks(&hsd1,data,0,2,5000):HAL_SD_ReadBlocks(&hsd1,data,0,2,5000);
+  assert(r==HAL_ERROR && hsd1.State==HAL_SD_STATE_ERROR && hsd1.Context!=0 && commands==2);
+ }
+ reset();transfer_mode=4;assert(HAL_SD_ReadBlocks(&hsd1,data,0,1,1)==HAL_TIMEOUT && hsd1.State==HAL_SD_STATE_TIMEOUT && (hsd1.ErrorCode & HAL_SD_ERROR_TIMEOUT));
+ puts("PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset");
+ reset();status_error=HAL_SD_ERROR_CMD_CRC_FAIL;hsd1.ErrorCode=HAL_SD_ERROR_TIMEOUT;
+ assert(HAL_SD_GetCardState(&hsd1)==HAL_SD_CARD_ERROR && (hsd1.ErrorCode & HAL_SD_ERROR_TIMEOUT));
+ assert(BSP_SD_GetCardState()==SD_TRANSFER_ERROR);
+ reset();status_response=HAL_SD_CARD_PROGRAMMING<<9;assert(BSP_SD_GetCardState()==SD_TRANSFER_BUSY);
+ reset();status_response=HAL_SD_CARD_TRANSFER<<9;assert(BSP_SD_GetCardState()==SD_TRANSFER_OK);
+ hsd1.ErrorCode=HAL_SD_ERROR_TIMEOUT;assert(BSP_SD_GetCardState()==SD_TRANSFER_ERROR);
+ for(unsigned state=1;state<=8;state++){reset();status_response=state<<9;assert(BSP_SD_GetCardState()==(state==4?SD_TRANSFER_OK:SD_TRANSFER_BUSY));}
+ reset();status_response=15<<9;assert(BSP_SD_GetCardState()==SD_TRANSFER_ERROR);
+ reset();transfer_mode=1;assert(BSP_SD_ReadBlocks((uint32_t*)data,0,1,5000)==MSD_OK);
+ reset();transfer_mode=1;assert(BSP_SD_WriteBlocks((uint32_t*)data,0,1,5000)==MSD_OK);
+ reset();assert(BSP_SD_ReadBlocks((uint32_t*)data,0,1,3)==MSD_TIMEOUT);
+ reset();assert(BSP_SD_WriteBlocks((uint32_t*)data,0,1,3)==MSD_TIMEOUT && (hsd1.ErrorCode & HAL_SD_ERROR_TIMEOUT));
+ reset();command_error=SDMMC_ERROR_CMD_CRC_FAIL;assert(BSP_SD_WriteBlocks((uint32_t*)data,0,1,5000)==MSD_ERROR);
+ puts("PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation");
+}
diff --git a/tests/sd_hal/run.py b/tests/sd_hal/run.py
new file mode 100644
index 0000000..b2f44a4
--- /dev/null
+++ b/tests/sd_hal/run.py
@@ -0,0 +1,50 @@
+#!/usr/bin/env python3
+"""Extract actual SD production functions; compile native register/HAL simulations."""
+from pathlib import Path
+import os,re,shutil,subprocess,tempfile
+ROOT=Path(__file__).resolve().parents[2]
+HERE=Path(__file__).resolve().parent
+LL=ROOT/'Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c'
+HAL=ROOT/'Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c'
+BSP=ROOT/'FATFS/Target/bsp_driver_sd.c'
+def extract(path,name):
+    text=path.read_text();match=re.search(r'^(?:static |__weak )?(?:uint32_t|uint8_t|HAL_StatusTypeDef|HAL_SD_CardStateTypeDef) '+name+r'\([^;]*?\)\s*\{',text,re.M)
+    assert match,name
+    start=match.start();pos=text.index('{',match.start());depth=1;end=pos+1
+    while depth:
+        depth+=(text[end]=='{')-(text[end]=='}');end+=1
+    return text[start:end].replace('__weak ','')
+names=['SDMMC_WaitCommand','SDMMC_GetCmdResp1','SDMMC_GetCmdResp2','SDMMC_GetCmdResp3','SDMMC_GetCmdResp6','SDMMC_GetCmdResp7','SDMMC_GetCmdError','SDMMC_CmdStopTransfer']
+functions='\n'.join(extract(LL,n) for n in names)
+functions+='\n'+extract(HAL,'SD_PowerON')+'\n'+extract(HAL,'SD_SendStatus')+'\n'+extract(HAL,'HAL_SD_GetCardState')
+functions+='\n'+extract(HAL,'HAL_SD_ReadBlocks')+'\n'+extract(HAL,'HAL_SD_WriteBlocks')
+functions+='\n'+'\n'.join(extract(BSP,n) for n in ['BSP_SD_ReadBlocks','BSP_SD_WriteBlocks','BSP_SD_GetCardState'])
+# Use target preprocessing to obtain the real constant values without ARM inline assembly.
+# The mock header supplies only register/handle shapes and deliberately simulated functions.
+compiler=shutil.which('arm-none-eabi-gcc') or '/workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc'
+includes=['Core/Inc','Drivers/STM32F7xx_HAL_Driver/Inc','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/CMSIS/Include']
+cmd=[compiler,'-E','-dM','-x','c','-DSTM32F765xx','-DUSE_HAL_DRIVER']+['-I'+str(ROOT/i) for i in includes]+['-include','stm32f7xx_hal.h','/dev/null']
+dump=subprocess.check_output(cmd,text=True)
+macros={m[1]:m[2] for m in re.finditer(r'^#define (\w+) (.+)$',dump,re.M)}
+# Local BSP result definitions are also production values.
+for m in re.finditer(r'^#define\s+(\w+)\s+(.+)$',(ROOT/'FATFS/Target/bsp_driver_sd.h').read_text(),re.M):
+    macros[m[1]]=m[2]
+wanted=set()
+def add(name):
+    if name in wanted or name not in macros:return
+    wanted.add(name)
+    for token in re.findall(r'\b[A-Za-z_]\w*\b',macros[name]):add(token)
+for name in re.findall(r'\b[A-Za-z_]\w*\b',functions+'\n'+(HERE/'host.c').read_text()):
+    if name.startswith(('SDMMC_','SD_SWITCH_','HAL_SD_ERROR_','HAL_SD_CARD_','SD_CONTEXT_','CARD_','MSD_','SD_TRANSFER_')) or name=='BLOCKSIZE':add(name)
+constants='\n'.join('#define '+n+' '+macros[n] for n in sorted(wanted))
+with tempfile.TemporaryDirectory(prefix='lxr-sd-hal-') as directory:
+    directory=Path(directory)
+    (directory/'constants.h').write_text(constants+'\n')
+    (directory/'production.c').write_text(functions+'\n')
+    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
+    binary=directory/'host'
+    flags=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
+           '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie',
+           '-I'+str(directory),'-I'+str(HERE)]
+    subprocess.run(flags+[str(HERE/'host.c'),'-o',str(binary)],check=True,env=env)
+    subprocess.run([str(binary)],check=True,env=env,timeout=20)
diff --git a/tests/sd_storage/host.c b/tests/sd_storage/host.c
index bbddcef..9324afd 100644
--- a/tests/sd_storage/host.c
+++ b/tests/sd_storage/host.c
@@ -5,12 +5,12 @@
 SysTick_Type host_systick;
 uint32_t host_ipsr,host_primask,host_basepri,host_faultmask;
 static uint32_t tick, step, busy, polls, reads, writes, inits, infos;
-static uint8_t present, transfer_result, init_result;
+static uint8_t present, transfer_result, init_result, state_error;
 static uint32_t last_count, last_sector, last_timeout, remove_at;
 uint32_t HAL_GetTick(void) { uint32_t n=tick; tick+=step; return n; }
 uint8_t BSP_SD_IsDetected(void) { return polls >= remove_at ? SD_NOT_PRESENT : present; }
 uint8_t BSP_SD_Init(void) { ++inits; return init_result; }
-uint8_t BSP_SD_GetCardState(void) { ++polls; assert(polls<100000); return polls<=busy?SD_TRANSFER_BUSY:MSD_OK; }
+uint8_t BSP_SD_GetCardState(void) { ++polls; assert(polls<100000); if(state_error) return SD_TRANSFER_ERROR; return polls<=busy?SD_TRANSFER_BUSY:MSD_OK; }
 void BSP_SD_GetCardInfo(BSP_SD_CardInfo *i) { ++infos; i->LogBlockNbr=1024; i->LogBlockSize=512; }
 uint8_t BSP_SD_ReadBlocks(uint32_t *b,uint32_t s,uint32_t n,uint32_t t)
 { assert(b);++reads;last_count=n;last_sector=s;last_timeout=t;return transfer_result; }
@@ -23,7 +23,7 @@ static void reboot(void)
  Stat=STA_NOINIT;Fault=0;host_systick.CTRL=3;
  host_ipsr=host_primask=host_basepri=host_faultmask=0;
  tick=0;step=1;busy=polls=reads=writes=inits=infos=0;
- present=SD_PRESENT;transfer_result=init_result=MSD_OK;remove_at=UINT32_MAX;
+ present=SD_PRESENT;transfer_result=init_result=MSD_OK;remove_at=UINT32_MAX;state_error=0;
 }
 static void start(void) { reboot();assert(SD_initialize(0)==0 && inits==1);polls=0; }
 static void locked(void)
@@ -65,7 +65,9 @@ int main(void)
  assert(SD_ioctl(0,CTRL_SYNC,0)==RES_OK && polls==4);
  polls=0;busy=UINT32_MAX;assert(SD_ioctl(0,CTRL_SYNC,0)==RES_ERROR);locked();
  start();busy=UINT32_MAX;assert(SD_status(0)&STA_NOINIT);locked();
- puts("PASS CTRL_SYNC ready/delayed/timeout/fault; status cannot clear or bypass fault");
+ start();state_error=1;assert(SD_ioctl(0,CTRL_SYNC,0)==RES_ERROR && polls==1);locked();
+ start();state_error=1;assert(SD_status(0)&STA_NOINIT);assert(polls==1);locked();
+ puts("PASS CTRL_SYNC ready/delayed/timeout/fault and explicit BSP status error; status cannot clear or bypass fault");
  start();DWORD v=0;WORD size=0;
  assert(SD_ioctl(0,GET_SECTOR_COUNT,&v)==RES_OK && v==1024);
  assert(SD_ioctl(0,GET_SECTOR_SIZE,&size)==RES_OK && size==512);
```

## Publication verification

Only the dedicated S2 source branch and separate lab-setup-based single-file report branch are published. Exact source blob/tree/parent identities and report contents/parent/files are read back from GitHub. All pre-existing branch heads are compared against the pre-task remote snapshot. Final response supplies the verified URL and report commit. No generated firmware/build/stack-analysis artifact is committed.
