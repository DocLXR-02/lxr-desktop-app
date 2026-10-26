# Phase D0 — SD storage timeout, error propagation and recovery safety

Date: 2026-10-26. Repository: DocLXR-02/lxr-desktop-app.

## Status: BLOCKED before source implementation

The initial read-only audit and software reproductions are complete. No correction was implemented. No production file, repository regression file, file format, or build configuration changed. The pre-existing SD readiness hangs remain present. This report must not be interpreted as a validated storage correction.

The requested narrow correction encounters end-to-end error propagation and ownership issues beyond the two readiness loops. Existing active saves can report success after write/close failure; the filesystem can ignore a mirrored-FAT write failure; the generic disk initializer can report success on a later call after failed initialization; HAL timeout returns do not establish that the card/peripheral transfer is quiescent. A timeout-only patch would therefore fail the requested false-success/recovery requirements. Implementing safe fault latching, coordinated mounted-volume invalidation and controlled reinitialization, auditing all save helpers/callers, and resolving filesystem error propagation requires a reviewed multi-layer storage change. Under the user's explicit stop conditions concerning filesystem consistency and ownership, no partial timeout workaround was published.

This does **not** establish that a safe software correction is impossible. It identifies why a two-loop or two-file timeout patch is insufficient for this task's full contract, and the exact additional design decisions/tests needed. A deliberately fail-closed design may avoid peripheral resets and automatic retries, but its recovery lifetime and foreground error handling must be established before implementation. No actual media corruption, physical hang, or audible failure was observed.

## Exact repository state and publication

Read complete AGENTS.md from remote lab-setup, blob `2f4d121f53a7055c1a30355235384a95ef920f1a`, and the complete preceding Phase D report on `codex-reports/2026-10-26-phase-d-capture-sd-export`, report commit `b40ff1105d4154e9cce654300f65206ef00464cc`.

Verified local and remote baseline branch: `safety/usb-allocator-reentrancy-c1`. Exact base and final source head: `091f502de6f57f3262ffc68659eafac6f8bdfa19`. A clean existing checkout at /workspace/lxr-desktop-app was used. Created `safety/sd-storage-timeout-d0` at that exact commit. There is no D0 implementation commit; the baseline-only remote ref identifies the review starting point and is not a completed correction. Existing Phase D branch `diagnostics/runtime-capture-sd-export-d1` remains unchanged at the same baseline. No worktree was created.

Report-only branch: `codex-reports/2026-10-26-phase-d0-sd-storage-safety`. Parent is lab-setup `5829398cea5d337200393757e39a37d4d8148256`; the only added path is `docs/codex-berichte/2026-10-26_phase-d0-sd-storage-safety.md`. The report's own commit can be read from its GitHub branch head; it is not recursively embedded in its contents. Remote content and branch heads are verified after creation and supplied in the final response.

Remote reference heads before work: main `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`; lab-setup as above; Bug001 `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`; Euclidean `67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72`; Phase A `e19bb0544dcc8f07138a9857c78022e8efaecd81`; Phase B `e4c0fd3ca66ced53413d5e1733e7ac9aa1c0da6e`; Phase C as above. All previous branches/reports are preserved. Source diff against baseline is empty.

No flashing, hardware operation, SD formatting/deletion, peripheral reset, bootloader change, option-byte change, clock/PLL/priority/startup/linker change, merge, PR, force push, firmware image upload or automatic reset occurred. Runtime diagnostics remain OFF by default. No Phase D UI, capture state machine or exporter was added.

## Evidence labels

CODE: directly inspected source. BUILD: executed host/build/artifact analysis. HYPOTHESIS: plausible unobserved runtime consequence. OPEN: requires further design or authorized device observation.

## Complete relevant call chain

### File operations, ownership and dispatch

`FATFS/App/fatfs.c:22–33` defines shared SDFatFS, SDFile and SDPath and links SD_Driver. Foreground preset/menu/load/save code uses that shared FIL. No new concurrent user or ISR file operation was introduced; reviewed active calls are foreground. There is no demonstrated concurrent filesystem corruption in this work. Shared handle reuse and error exits are nevertheless relevant to recovery ownership.

`Middlewares/Third_Party/FatFs/src/ff.c` file operations use disk_read/disk_write/disk_ioctl; `src/diskio.c` simply dispatches to the registered SD_Driver for read/write/ioctl. There are no retries in those dispatch wrappers. `FATFS/Target/sd_diskio.c` installs SD_initialize, SD_status, SD_read, SD_write, SD_ioctl. The active read/write path is synchronous polling, not the separately defined BSP DMA APIs.

`BSP_SD_ReadBlocks` and `BSP_SD_WriteBlocks` in `FATFS/Target/bsp_driver_sd.c:91–125` invoke HAL_SD_ReadBlocks/HAL_SD_WriteBlocks. They collapse every non-HAL_OK return to MSD_ERROR, losing HAL_TIMEOUT versus generic HAL_ERROR. `BSP_SD_GetCardState:205–207` maps every HAL state other than TRANSFER to BUSY. `HAL_SD_GetCardState` at HAL source2266–2281 calls SD_SendStatus, ORs command errors into hsd->ErrorCode, and derives a state from resp1 initialized to zero even on command failure. Thus BUSY alone cannot distinguish temporary programming from a command failure or removed card.

The GPIO presence check (`FATFS/Target/fatfs_platform.c:20–30`, BSP_SD_IsDetected) can identify a detect-switch change, not prove electrical availability or successful command response. No reliable write-protect sensor was established. Do not invent RES_WRPRT based on generic command failure. MSD_ERROR_SD_NOT_PRESENT exists for initialization but normal BSP polling wrappers do not independently classify removal.

### Bounds, timeout units and timebase

SD_read/write at sd_diskio.c150–195 have no input validation and wait indefinitely after a successful BSP transfer until GetCardState returns MSD_OK. They return RES_ERROR for immediate BSP failure. Their documented 1–128-sector count is not checked there; the HAL itself also performs address arithmetic whose overflow and data-length constraints need careful validation for arbitrary direct inputs. Normal FatFs cluster-limited operations should be traced before imposing any new maximum that could reject supported requests.

SD_TIMEOUT at sd_diskio.c35–40 selects SDMMC_DATATIMEOUT, defined in `Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_ll_sdmmc.h:303–304` as0xFFFFFFFF. The same constant is a hardware data timeout register value in HAL data-path configuration, whereas the public HAL polling Timeout argument is compared to HAL_GetTick millisecond elapsed time. These are different uses; globally redefining this shared macro is not a narrow safe timeout fix.

`HAL_SD_ReadBlocks:570–766`, `HAL_SD_WriteBlocks:781–946`: tickstart is sampled before command setup, and data polling uses unsigned HAL_GetTick()-tickstart>=Timeout. For a finite modest timeout this idiom handles one32-bit tick wrap, provided the tick advances and calls are serviced. `HAL_GetTick` is milliseconds in this firmware (`stm32f7xx_hal.c:76,231–244,290–305`, default1kHz); `Core/Src/stm32f7xx_it.c:210–215` calls HAL_IncTick. TICK_INT_PRIORITY is0 (`Core/Inc/stm32f7xx_hal_conf.h:148`). The inspected foreground call path normally permits SysTick preemption. No active HAL_SuspendTick was found in Core/FATFS. Tiny encoder critical sections exist, not surrounding SD polling. Tick-based termination is not guaranteed if called from a nonpreemptible ISR, with global masks held, or with SysTick suspended; a future implementation must define/check the foreground caller contract and must not add IRQ masking. No such new guard was implemented.

The passed timeout does not bound all inner calls. `SDMMC_CmdRead*`/Write* use `SDMMC_CMDTIMEOUT=5000` at LL header321; normal successful multiblock transfers call CmdStopTransfer, which uses `SDMMC_STOPTRANSFERTIMEOUT=100000000` at323. LL source1191–1210 GetCmdResp1 derives an iteration count using `Timeout * (SystemCoreClock/8/1000)` in32-bit arithmetic; that multiplication can wrap. At nominal216MHz the stop count evaluates to2755506176 iterations after modulo2^32, not a100000000ms deadline. These loops terminate by an iteration counter, not SysTick. Interrupt preemption and optimizer execution make their wall-clock duration different from the comment's approximate instruction-based estimate. The outer HAL Timeout is not checked while inside them. No measured latency is claimed.

Initial HAL init uses HAL_Delay(2), bounded command polling, and a finite voltage trial loop; it is not equivalent to the post-transfer readiness loop. `MX_SDMMC1_SD_Init:1323` only assigns handle configuration (1-bit bus), and BSP_SD_Init performs the actual HAL_SD_Init at mount. SD_FindSCR/SD_SendSDStatus contain additional huge-timeout/data loops but are not automatically part of every disk read/write. In this configured1-bit path no active application call to ConfigWideBusOperation or GetCardStatus was found; do not report those helpers as a demonstrated active defect path. They remain relevant if recovery design expands to other HAL operations.

### Timeout cleanup and transfer uncertainty

On data polling timeout the HAL clears static flags, sets software State READY and Context NONE, and returns without executing the normal multiblock stop path. That branch does not clear DPSM DTEN or otherwise establish card quiescence. This is code evidence that software READY alone is insufficient; it is not physical proof that a transfer always remains active. In the write timeout branch at875–884, ErrorCode is ORed with errorstate from the previously successful command (normally zero), so HAL_TIMEOUT may not set HAL_SD_ERROR_TIMEOUT. Consulting ErrorCode alone can therefore lose the timeout classification already collapsed by BSP.

A write timeout/CRC/error does not establish which data reached media. Blindly replaying writes can corrupt ordering or modify data after an uncertain result. The active disk wrappers contain no automatic replay, but a future implementation needs a sticky failure state to reject subsequent writes/readiness success until reviewed recovery. Updating Stat alone is insufficient because SD_CheckStatus resets it from card readiness, and SD_initialize may be invoked again by remount machinery.

`HAL_SD_Abort:2289–2346` disables SD peripheral interrupts, clears flags and DTEN, handles possible DMA abort based on Context, queries card status and may issue STOP. It does not retry data, but includes the same command-wait machinery. Since timeout already cleared Context, this is not automatically proof of correct cleanup for every prior transfer mode. It was inspected, not called or proposed as an unreviewed blanket fix. No global/shared peripheral reset is justified here.

### CTRL_SYNC and false-success paths

`SD_ioctl:219–222` returns RES_OK unconditionally for CTRL_SYNC after the cached Stat check. It does not establish card readiness or absence of a failed earlier write. A future synchronous design should wait/check readiness with the same finite policy and fail closed when prior I/O outcome is uncertain. Even that cannot claim SD internal-cache/power-loss durability beyond card reporting.

More importantly, returning an error from disk_write is not enough for the whole existing storage contract:

1. `ff.c:sync_window` around886–908 checks the first window write but discards return values for additional FAT copies and clears wflag. An actual extracted-function host test with first write success/second-FAT failure returns FR_OK and wflag=0. This does not prove all f_sync calls return success: a sticky disk fault propagated through CTRL_SYNC could catch a later sync. It does prove this local consumer loses a real error and cannot be assumed correct.
2. `ff.c:sync_fs:950–968` also discards the FSInfo disk_write result before calling CTRL_SYNC. Fail-closed CTRL_SYNC would help, but its failure lifetime and mounted-volume state must be explicit.
3. `diskio.c:disk_initialize` sets disk.is_initialized before initialization succeeds. A later call with the flag set skips the driver and returns initial RES_OK. SD_status can still reveal failure, so this is not proof that every mount succeeds incorrectly; it is another lost error at an API boundary that recovery design must account for.
4. `Core/Src/Preset/presetManager.c:195–239` active preset_saveTmpKit checks the first metadata write but ignores the payload write, byte counts and final close, then returns FR_OK. Actual extracted body tests reproduce false success after payload error, short write or close error.
5. `preset_savePatternSD:1677–1711` checks open and two header writes, invokes void payload helper preset_writePatternData, discards close status, then returns FR_OK. Actual caller body returns FR_OK after mocked close failure; the helper failure reproduction uses a mocked helper and is identified as such. Early header error returns without closing the shared FIL (confirmed host call count). This is an ownership issue before a proposed remount.
6. Existing named-kit/global save helpers similarly contain unchecked operations (source inspection only, not fully tested here). `menu.c:2042–2055` ignores the pattern-save result. `preset_saveProject:2294` orchestrates helpers and performs tmp-folder lifecycle changes; returning proper disk errors does not guarantee its helpers/callers honor them. `preset_clearTmpFolder:650–665` invokes delete_node, so cleanup decisions after failed saves must be audited before claiming preservation of fallback data. No actual deletion was executed or proven to follow every failed operation.

The current FA_CREATE_ALWAYS behavior can truncate existing files before a later failure. It is not transactional. This work does not claim that any filesystem can guarantee rollback of every power loss or media failure. The concrete concern is avoidable false success, stale handles, uncertain subsequent writes, and existing cleanup sequencing. These need coordinated review rather than silently expanding a timeout change into a new persistence scheme.

### Initialization/remount behavior

`SD_initialize:108–126` calls BSP_SD_Init, then SD_CheckStatus. `SD_CheckStatus:91–101` overwrites Stat based only on collapsed readiness. `FATFS/App/fatfs.c:53–56` MX_FATFS_ReInit unlinks/relinks the driver. `Core/Src/main.c:923–943` detects a card insertion edge, relinks, unmounts, clears SDFatFS and mounts, then loads names. It does not coordinate explicit closure of all outstanding shared FIL objects in that block. FatFs object IDs can invalidate handles after remount, but that is not an application-level proof that pending dirty data/error ownership is resolved. A transient I/O timeout does not necessarily produce a detect edge, so it also does not automatically invoke this recovery.

A safe fail-closed design must decide whether a latch survives unlink/relink, card insertion and SD_initialize. Clearing automatically on TRANSFER is unsafe after an ambiguous write. Reboot-only recovery could be conservative but would change existing hotplug behavior and must be explicitly reviewed. Controlled reinitialization needs volume/file invalidation rules and prevention of replay from old caches. No speculative reinit/reset/latch semantics were added.

## Why implementation stopped

The requested result includes meaningful error propagation, no false success after incomplete operations, safe handling of uncertain writes, and preservation of existing save/load ownership. A finite post-transfer loop alone meets none of the latter guarantees. A fail-closed disk latch addresses repeated physical writes, but cannot make existing application helpers return errors or close shared handles, and cannot recover cached/partly modified filesystem metadata. The current command-stop path also lies outside the public polling deadline.

The stop decision is the engineering assessment of these combined findings, not a claim that AGENTS.md explicitly forbids editing FatFs or save callers. The user permits a narrow correction and expressly says to stop if filesystem consistency/ownership requires broader architecture. Coordinated changes across disk status, BSP status fidelity, command/cleanup deadlines, FatFs error handling, save helper signatures/callers and remount ownership exceed the confidently established narrow design in this audit. No partial correction was made to hide those remaining obligations.

## Proposed continuation — design only

1. Review a serialized foreground storage session/fault-latch contract. After ambiguous transfer failure, reject further physical I/O (including dirty close/sync retries) without reset or replay; retain reason and HAL status. Specify exactly what user-controlled recovery may invalidate mounted caches/FILs, and whether card reinsertion is sufficient or a power cycle is required. Do not activate a watchdog or reset the whole device.
2. Separate hardware SDMMC data timeout register values from millisecond software budgets. Establish operation-size limits from actual FatFs usage; test single and maximal supported multi-sector operations. Select conservative read/write/programming time allowances from applicable SD guarantees and supported-card policy; validate on authorized hardware later. Do not silently choose a short per-file timeout. Account for preemption and nested command waits. No production timeout value was selected here because the overall contract is unresolved.
3. Preserve BSP status fidelity (HAL_TIMEOUT vs command/CRC/busy) and independent card-detect evidence; expose a diagnostic reason alongside FatFs DRESULT where needed. FatFs has no distinct timeout DRESULT: generic I/O/timeout maps RES_ERROR, unavailable/uninitialized maps RES_NOTRDY, reliable write-protect only maps RES_WRPRT, invalid parameters RES_PARERR. Temporary busy can wait only within policy. Do not infer removal or write protection solely from HAL_ERROR.
4. Make CTRL_SYNC observe finite readiness and latched I/O errors. It cannot prove power-loss durability. Fix or explicitly contain ignored mirror/FSInfo errors and disk initialization status bookkeeping.
5. Review and test save helper return/byte-count/close checks and single-owner shared FIL lifecycle. Stop project cleanup on failed save. This need not alter on-disk formats, but it is a separate multi-caller propagation change requiring review. No automatic retry of writes whose result is unknown.
6. Revisit bounded command stop/abort without global IRQ masking, CPU/peripheral reset, clock/DMA changes or recursive soundengine service. Confirm no active transfer can be reset underneath another caller. If this requires broad HAL changes, obtain that scoped review before implementing.

## Real-time analysis

CALC_SAMPLES_IN_INTERRUPT is1 (`Core/Inc/Soundengine/config.h:29`). TIM2 invokes sequencing/render/UART (`stm32f7xx_it.c:262–291`); audio DMA callbacks rearm streams and update readiness. Main also services soundengine, MIDI/UI/USB and presets. Main SD polling leaves interrupts enabled in the reviewed path, so a blocked main is not proof that audio ISR rendering stops. Bus contention, preemption, MIDI/UI stalling and peripheral command waits can still matter; actual effects remain OPEN. No new loops, priorities, masks, locks, allocations, logging, file I/O in ISR, buffer changes or timing instrumentation were introduced. DWT/CYCCNT was neither reset nor used by the temporary tests or any source change.

## Tests actually executed

All four existing suites passed. These validate preserved Phase A–C behaviors, not a storage correction. Four normal/instrumented configure/build pairs passed; builds are incremental and reused unchanged objects, with the always-run instruction target. Image checker passed four images with hashes identical to Phase C. No installation instructions printed by build output were executed.

Temporary host tests at /tmp/phase-d0 compile extracted actual production functions with gcc -O1 -g -fsanitize=address,undefined -fno-pie -no-pie. ASAN_OPTIONS=detect_leaks=0 is explicit because LeakSanitizer is unsupported under this executor's tracing; ASan/UBSan remain active. The first sync_window harness compilation failed due to a missing mock UINT typedef; added that typedef in /tmp only, then reran successfully. This was a harness error, not a firmware compile failure.

Mocks: f_open/write/close, timer/UI/filename helpers, globals and payload helper for the named-pattern caller; disk_write and minimal FATFS fields for actual sync_window. No real storage/SD/HAL media or LXR device is used. Sources and outputs are embedded below for reproducibility. Reproduced known read/write permanently-busy flow using prior actual-body mock tests, terminated after0.3s by the host; finite readiness and immediate failure return as expected. The prior navigation repro also reruns but is not a D0 correction or relevant storage regression.

Required new correction matrix status: successful single/multi-sector corrected I/O, bounded permanent busy, HAL timeout cleanup, removal handling, wraparound deadline, repeated-fault latch, no post-failure writes, corrected CTRL_SYNC, controlled recovery and new error classification are **NOT RUN against a correction because none exists**. Baseline read/write tests exercise a generic sector call, not full single/multi-sector media correctness. Reproduced failures must not be reported as passing storage-safety requirements. No formatting/deletion mock or real operation was invoked by the audit tests; no disk timeout policy is claimed validated.

## Memory, binary and code comparison

No source changes: additional Flash/ITCM/DTCM/general SRAM are all zero; no new static state, buffers or allocations. Storage, audio and interrupt source/object code unchanged; image/ELF hashes match exact prior Phase C artifacts. Current object hashes are included below for later comparison, without uploading objects.

| Region | Normal | Diagnostics ON | Capacity | Normal / ON headroom |
|---|---:|---:|---:|---:|
| ITCM |15016|15088|16384|1368 /1296|
| DTCM |107392|107392|131072|23680 /23680|
| General SRAM |271576|271648|393216|121640 /121568|

DTCM remaining space is before runtime stack usage; no actual high-water measured. Source version remains1.7; no official v1.71 equivalence is claimed. Normal Release payload275948; Debug275996; ON Release276388; ON Debug276436. Flash region0x08008000 of992KiB=1015808 bytes leaves739860/739812/739420/739372 bytes respectively based on full payload span. The image-checker's FLASH VMA high-water figures exclude some copied load sections/gaps and are a different metric; do not confuse them with payload footprint. LXRV2IMG16-byte header/length/checksum/bin correspondence/memory bounds and plausible vectors passed. Initial MSP20080000, reset08014F79 unchanged; existing startup moves stack into DTCM. The exact image/ELF hashes and validation output are in the transcript. Firmware binaries remain local and are not published.

## Final safety assessment

CODE/BUILD: no source change; all earlier fixes/defaultOFFdiagnostics preserved. CODE/BUILD: readiness nontermination under permanent-busy mock and active application false-success paths reproduced. CODE: HAL timeout does not establish quiescence, BSP loses status distinctions, disk init and FatFs lose some errors, remount ownership needs review. HYPOTHESIS: those failures could contribute to lost saves/inconsistent metadata on failing media; no physical damage demonstrated. OPEN: actual card timing, interrupt/audio interference, reliable recovery sequencing, file-session ownership and supported timeout policy.

Do not resume Phase D SD exporter implementation yet. Recommended next action is a reviewed storage error/ownership contract covering disk fault latching, command deadlines and save-caller propagation, then staged implementation/tests preserving file formats. The audit is ready for design review, not deployment. Successful unchanged baseline tests do not authorize hardware use.

**NOT FLASHED — NOT HARDWARE VALIDATED.**

## Exact commands and results

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
$ gcc --version
gcc (Debian 14.2.0-19) 14.2.0
Copyright (C) 2024 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


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

### 10.log

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

### 11.log

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

### 12.log

```text
$ cmake --build --preset debug --parallel 4
[1/1] Firmware update instructions

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

### 13.log

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### 14.log

```text
$ cmake --build build/diagnostics-release --parallel 4
[1/1] Firmware update instructions

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

### 15.log

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### 16.log

```text
$ cmake --build build/diagnostics-debug --parallel 4
[1/1] Firmware update instructions

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

### 17.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0xE6", "configuration": "release", "elf_sha256": "c31121510d8b3100d1dcdeeb89adffdb15c299837e24ded1e4af8f6926f62b7f", "image_bytes": 275964, "image_sha256": "06e9859d1ff99e3b80baf121225eb83865b8573e319f96243ccb037b291342a3", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223084, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275948, "reset_vector": "0x08014F79", "result": "PASS"}
{"checksum": "0xFF", "configuration": "debug", "elf_sha256": "5385d5fcd676f29e00d9e436979ebee55e6b8a1efdc9106b3e7c82930fe572b1", "image_bytes": 276012, "image_sha256": "da880ed68c4d0e65281e1b49aa9a007270c9ca625bc5d797d8bb0b066e9a2fd1", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223132, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275996, "reset_vector": "0x08014F79", "result": "PASS"}
{"checksum": "0xF3", "configuration": "diagnostics-release", "elf_sha256": "3e965f9cf60f4ace2511484f7cca774a1b3519b4bd4c3c08c75f2e90e8fe17da", "image_bytes": 276404, "image_sha256": "de6fe28745cb01377b2bcb7ae1c3f6f660c8050b084f5d005e9770dc117bcd30", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223452, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 276388, "reset_vector": "0x08014F79", "result": "PASS"}
{"checksum": "0xBD", "configuration": "diagnostics-debug", "elf_sha256": "32d65fcd2e5371f68d96aec33fad596440d3a48cdcd22c1b4b56a52b92ff2bbc", "image_bytes": 276452, "image_sha256": "d4173d4531cd87fe0b11885778f81df22e6c1dcd2677d4393534b167d248b037", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223500, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 276436, "reset_vector": "0x08014F79", "result": "PASS"}

EXIT 0
```

### python3 /tmp/phase-d0/audit.py (final exit0)

```text
CONFIRMED actual tmp-kit save returns FR_OK after payload write error
CONFIRMED actual tmp-kit save returns FR_OK after close error
CONFIRMED actual tmp-kit save ignores short writes
CONFIRMED actual named-pattern save returns FR_OK after close error
CONFIRMED named-pattern caller cannot propagate helper payload failure (helper mocked)
CONFIRMED actual named-pattern early write error leaves opened FIL unclosed
CONFIRMED actual FatFs sync_window ignores second-FAT write failure and clears dirty flag
```

### ASAN_OPTIONS=detect_leaks=0 python3 /tmp/phase-d0/previous_audit_host.py (final exit0)

```text
PASS read finite busy returned
CONFIRMED read permanently busy mock did not return; host process terminated after 0.3s (not hardware evidence)
PASS read transfer failure returned
PASS write finite busy returned
CONFIRMED write permanently busy mock did not return; host process terminated after 0.3s (not hardware evidence)
PASS write transfer failure returned
CONFIRMED actual navigation body wraps page 1 without second screen to page 0, skipping populated page 2
```

## Reproduction runner for newly identified propagation failures

```python
from pathlib import Path
import re,subprocess,os
root=Path('/workspace/lxr-desktop-app'); out=Path('/tmp/phase-d0')
def extract(path,name):
 s=(root/path).read_text(errors='replace');m=re.search(r'(?:FRESULT|DRESULT|DSTATUS)\s+'+name+r'\s*\(',s);assert m,name
 b=s.index('{',m.end());i=b+1;d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[m.start():i]
prefix='''#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef int FRESULT; typedef unsigned UINT; typedef int FIL;
#define FR_OK 0
#define FR_DISK_ERR 1
#define FA_CREATE_ALWAYS 8
#define FA_WRITE 2
#define USE_INTERRUPT_ONLY_DURING_SAVE_PROJ 0
#define SAVE_TYPE_KIT 0
#define END_OF_SOUND_PARAMETERS 256
#define FEXT_PAT 0
#define PATTERN_NAMES 0
FIL SDFile; unsigned char menu_selectedPresetNr[4],activeParameters[256];char savefileNames[64][4][9];
static unsigned writes,closes,fail_write,fail_close,short_write,body_calls;
FRESULT f_open(FIL*a,const char*b,int c){return FR_OK;}
FRESULT f_write(FIL*a,const void*b,UINT n,UINT*out){++writes;*out=short_write?0:n;return writes==fail_write?FR_DISK_ERR:FR_OK;}
FRESULT f_close(FIL*a){closes++;return fail_close?FR_DISK_ERR:FR_OK;}
void preset_makeFileNameSave(char*a,unsigned b,int c,char*d){a[0]=0;}
void preset_writePatternData(unsigned n){body_calls++;unsigned bw;f_write(&SDFile,activeParameters,256,&bw);}
void menu_repaintAll(void){}
'''
source=prefix+extract('Core/Src/Preset/presetManager.c','preset_saveTmpKit')+'\n'+extract('Core/Src/Preset/presetManager.c','preset_savePatternSD')+'''
void reset(void){writes=closes=fail_write=fail_close=short_write=body_calls=0;}
int main(void){
reset();assert(preset_saveTmpKit()==FR_OK && writes==2 && closes==1);
reset();fail_write=2;assert(preset_saveTmpKit()==FR_OK && writes==2);puts("CONFIRMED actual tmp-kit save returns FR_OK after payload write error");
reset();fail_close=1;assert(preset_saveTmpKit()==FR_OK);puts("CONFIRMED actual tmp-kit save returns FR_OK after close error");
reset();short_write=1;assert(preset_saveTmpKit()==FR_OK);puts("CONFIRMED actual tmp-kit save ignores short writes");
reset();fail_close=1;assert(preset_savePatternSD(0)==FR_OK && body_calls==1);puts("CONFIRMED actual named-pattern save returns FR_OK after close error");
reset();fail_write=3;assert(preset_savePatternSD(0)==FR_OK);puts("CONFIRMED named-pattern caller cannot propagate helper payload failure (helper mocked)");
reset();fail_write=1;assert(preset_savePatternSD(0)==FR_DISK_ERR && closes==0);puts("CONFIRMED actual named-pattern early write error leaves opened FIL unclosed");
return 0;}
'''
(out/'save_host.c').write_text(source)
subprocess.run(['gcc','-O1','-g','-fsanitize=address,undefined','-fno-pie','-no-pie',str(out/'save_host.c'),'-o',str(out/'save_host')],check=True)
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');subprocess.run([str(out/'save_host')],check=True,env=env)
sync='''#include <assert.h>
#include <stdio.h>
typedef int FRESULT;typedef unsigned DWORD;typedef unsigned BYTE;typedef unsigned UINT;
#define FR_OK 0
#define FR_DISK_ERR 1
#define RES_OK 0
typedef struct {unsigned wflag,winsect,fatbase,fsize,n_fats,drv;unsigned char win[512];} FATFS;
unsigned calls;int disk_write(unsigned d,const void*w,DWORD s,unsigned n){return ++calls==2?1:0;}
'''+extract('Middlewares/Third_Party/FatFs/src/ff.c','sync_window')+'''
int main(void){FATFS fs={.wflag=1,.winsect=1,.fatbase=1,.fsize=10,.n_fats=2};assert(sync_window(&fs)==FR_OK && calls==2 && fs.wflag==0);puts("CONFIRMED actual FatFs sync_window ignores second-FAT write failure and clears dirty flag");}
'''
(out/'sync_host.c').write_text(sync)
subprocess.run(['gcc','-O1','-g','-fsanitize=address,undefined','-fno-pie','-no-pie',str(out/'sync_host.c'),'-o',str(out/'sync_host')],check=True)
subprocess.run([str(out/'sync_host')],check=True,env=env)
```

## Prior polling reproduction runner (rerun unchanged)

```python
from pathlib import Path
import subprocess
root=Path('/workspace/lxr-desktop-app')
def extract(path,name):
 s=(root/path).read_text(errors='replace'); pos=s.index(name+'(') if name+'(' in s else s.index(name+' ('); start=s.rfind('\n',0,pos)+1; b=s.index('{',pos); depth=1;i=b+1
 while depth:
  depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[start:i]
prefix='''#include <stdint.h>
#include <stdlib.h>
typedef unsigned char BYTE; typedef unsigned DWORD; typedef unsigned UINT; typedef int DRESULT;
#define RES_ERROR 1
#define RES_OK 0
#define MSD_OK 0
#define SD_TIMEOUT 0xffffffffu
static unsigned checks, mode;
int BSP_SD_ReadBlocks(uint32_t*a,uint32_t b,UINT c,unsigned d){return mode==2;}
int BSP_SD_WriteBlocks(uint32_t*a,uint32_t b,UINT c,unsigned d){return mode==2;}
int BSP_SD_GetCardState(void){checks++; return mode==1 || checks<3;}
'''
source=prefix+extract('FATFS/Target/sd_diskio.c','SD_read')+'\n'+extract('FATFS/Target/sd_diskio.c','SD_write')+'''
int main(int argc,char **argv){mode=atoi(argv[1]);BYTE buf[512];int r=atoi(argv[2])?SD_write(0,buf,0,1):SD_read(0,buf,0,1);return r != (mode==2?RES_ERROR:RES_OK);}
'''
Path('/tmp/phase-d/disk_host.c').write_text(source)
subprocess.run(['gcc','-O1','-g','-fsanitize=address,undefined','-fno-pie','-no-pie','/tmp/phase-d/disk_host.c','-o','/tmp/phase-d/disk_host'],check=True)
for write in (0,1):
 for mode in (0,1,2):
  try:
   p=subprocess.run(['/tmp/phase-d/disk_host',str(mode),str(write)],timeout=.3,capture_output=True,text=True)
   assert mode!=1 and p.returncode==0,(mode,p.returncode,p.stderr)
   print('PASS', 'write' if write else 'read','transfer failure returned' if mode==2 else 'finite busy returned')
  except subprocess.TimeoutExpired:
   assert mode==1
   print('CONFIRMED', 'write' if write else 'read','permanently busy mock did not return; host process terminated after 0.3s (not hardware evidence)')
nav='''#include <stdint.h>
#include <assert.h>
#define MASK_PARAMETER 7
#define MASK_PAGE 56
#define PAGE_SHIFT 3
#define NUM_SUB_PAGES 8
#define MENU_MIDI_PAGE 7
#define SEQ_PAGE 12
#define TEXT_EMPTY 0
#define pgm_read_byte(p) (*(p))
unsigned char editModeActive,menuIndex,menu_activePage=MENU_MIDI_PAGE;
struct Page {unsigned char top1,rest[7];} menuPages[16][8];
static uint8_t has2ndPage(uint8_t p){return menuPages[menu_activePage][p].rest[3]!=TEXT_EMPTY;}
'''+extract('Core/Src/menu/menu.c','menu_switchSubPage')+'''
int main(void){menuPages[7][0].top1=1;menuPages[7][0].rest[3]=1;menuPages[7][1].top1=1;menuPages[7][2].top1=1;
menu_switchSubPage(0,1);assert(menuIndex==4);
menu_switchSubPage(0,1);assert(menuIndex==8);
menu_switchSubPage(1,1);assert(menuIndex==0);
return 0;}
'''
Path('/tmp/phase-d/nav_host.c').write_text(nav)
subprocess.run(['gcc','-O1','-fsanitize=address,undefined','-fno-pie','-no-pie','/tmp/phase-d/nav_host.c','-o','/tmp/phase-d/nav_host'],check=True)
subprocess.run(['/tmp/phase-d/nav_host'],check=True)
print('CONFIRMED actual navigation body wraps page 1 without second screen to page 0, skipping populated page 2')
```

## Object SHA-256 inventory (unchanged baseline)

```text
d6ee7b60ad6a2a1924e096b3631b8f38284349e3ad7c037c348e5ca3e7f17aca  build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
10472838499bb9806639fc82153ba880797c3c496ac48e10e6724aeaa2e12ac5  build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
01463848afda54efbbeea116af34c9ff35db325406b42b003432ee46fba5c468  build/release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
c2ecbd314ff85224322a8ab344122e53fbfb06846075c47e2842bc2ae0ba6342  build/release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
ae35c7db088406e8709f44a06a8e8cd0edcf9bd53acaae776ff40ebf7ad33ea2  build/release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
5f6fa6ba76fd1a4792e4b67bf9098622c22285d2899ad2ba14187bc6973f6f4f  build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
d1d426fccabe65ff11979c545afa1d7e67b6dd09f021f195c2188f00e13eccee  build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
72519986928007069087a7eed0942c711cb2e272226ef1c18ad7085186d8043a  build/debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
6f6613e154b661089a31376d71255decc41bd247ea43826dcc24ca34bd675c80  build/debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
b84e4c432b2f2ff9e888aad8a1b9d478d195b889a0eb16de1d3457a4ba992de8  build/debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
d1ed2c18ce02443fa1ed7ddae0633a61de9f31e39ba3882bb5f73378ad38dea2  build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
46b4aa19065504b170ae148e4e6d0794856bd512529aa837a4d5e77479c5ab7d  build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
66cd4b9de166101afc4fae3970b75e726495517a6de6957ca4b7ef72863fa5d6  build/diagnostics-release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
c2ecbd314ff85224322a8ab344122e53fbfb06846075c47e2842bc2ae0ba6342  build/diagnostics-release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
ae35c7db088406e8709f44a06a8e8cd0edcf9bd53acaae776ff40ebf7ad33ea2  build/diagnostics-release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
6282e9bc0e06c0a2945f6baf9be36f5670aabd823d9e0e48c23da6e052a2a34c  build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
2ac6331a297b1d626080c6dc4e5ebaff3be4f31872d0a16df6c9d0488f71788b  build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
3fa6b6fbec925306271e90d029e0910f0eeacb0232dc244bf3a2fd40aab905b6  build/diagnostics-debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
a6ae57f8027ae9b3f5443e9b87b2870d4510d1a561a58fe35f31a14ab9e23181  build/diagnostics-debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
b88c5c081634d4e1ee4b9c66871e91d195fd218046be360b0248471daaf8cb75  build/diagnostics-debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
```

## Source excerpts at the unchanged baseline

### FATFS/Target/sd_diskio.c:149

```c
{
  DRESULT res = RES_ERROR;

  if(BSP_SD_ReadBlocks((uint32_t*)buff,
                       (uint32_t) (sector),
                       count, SD_TIMEOUT) == MSD_OK)
  {
    /* wait until the read operation is finished */
    while(BSP_SD_GetCardState()!= MSD_OK)
    {
    }
    res = RES_OK;
  }

  return res;
```

### FATFS/Target/sd_diskio.c:178

```c

DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  DRESULT res = RES_ERROR;

  if(BSP_SD_WriteBlocks((uint32_t*)buff,
                        (uint32_t)(sector),
                        count, SD_TIMEOUT) == MSD_OK)
  {
	/* wait until the Write operation is finished */
    while(BSP_SD_GetCardState() != MSD_OK)
    {
    }
    res = RES_OK;
  }

  return res;
}
```

### FATFS/Target/sd_diskio.c:216

```c
  switch (cmd)
  {
  /* Make sure that no pending write process */
  case CTRL_SYNC :
    res = RES_OK;
    break;

  /* Get number of sectors on the disk (DWORD) */
```

### Core/Src/Preset/presetManager.c:218

```c
	res = f_write((FIL*)&SDFile,(void*)&menu_selectedPresetNr[SAVE_TYPE_KIT],1,&bytesWritten);
	if(res != FR_OK)
	{
		f_close((FIL*)&SDFile);
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
		return res;
	}
	//write the preset data
	f_write((FIL*)&SDFile,activeParameters,END_OF_SOUND_PARAMETERS,&bytesWritten);

	//close the file
	f_close((FIL*)&SDFile);
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif
	return FR_OK;
```

### Core/Src/Preset/presetManager.c:1696

```c
	res = f_write((FIL*)&SDFile,(void*)savefileNames[presetNr][PATTERN_NAMES],8,&bytesWritten);
	if(res != FR_OK) return res;
	//write version tag
	res = f_write((FIL*)&SDFile,"PatV03",6,&bytesWritten);
	if(res != FR_OK) return res;

	//write pattern data
	preset_writePatternData(presetNr);

	//close the file
	f_close((FIL*)&SDFile);
	
	//reset the lcd
	menu_repaintAll();
	
	return FR_OK;
```

### Middlewares/Third_Party/FatFs/src/ff.c:893

```c
		wsect = fs->winsect;	/* Current sector number */
		if (disk_write(fs->drv, fs->win, wsect, 1) != RES_OK) {
			res = FR_DISK_ERR;
		} else {
			fs->wflag = 0;
			if (wsect - fs->fatbase < fs->fsize) {		/* Is it in the FAT area? */
				for (nf = fs->n_fats; nf >= 2; nf--) {	/* Reflect the change to all FAT copies */
					wsect += fs->fsize;
					disk_write(fs->drv, fs->win, wsect, 1);
				}
			}
		}
	}
	return res;
```

### Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c:875

```c
      if(((HAL_GetTick()-tickstart) >=  Timeout) || (Timeout == 0U))
      {
        /* Clear all the static flags */
        __HAL_SD_CLEAR_FLAG(hsd, SDMMC_STATIC_FLAGS);
        hsd->ErrorCode |= errorstate;
        hsd->State = HAL_SD_STATE_READY;
        hsd->Context = SD_CONTEXT_NONE;
        return HAL_TIMEOUT;
      }
    }

```

## Publication scope

Only the new baseline-only D0 development ref and this report-only lab-setup-based branch are published. All existing refs are compared after publication. No generated artifact or temporary host test is uploaded. The final response supplies the verified file URL and report commit.
