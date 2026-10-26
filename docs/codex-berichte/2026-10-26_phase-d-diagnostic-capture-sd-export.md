# Phase D — Runtime diagnostic capture and manual SD export: audit and blocker

Date: 2026-10-26. Repository: `DocLXR-02/lxr-desktop-app`.

## Status and exact revisions

**BLOCKED before implementation. No production source, repository tests, build configuration, or existing branch was changed.** The requested capture/UI/export facility is not implemented. This is an audit-and-blocker handoff, not a validated correction or deployment candidate.

Verified remote and local source baseline: `safety/usb-allocator-reentrancy-c1`, `091f502de6f57f3262ffc68659eafac6f8bdfa19`. A clean checkout was confirmed before creating local `diagnostics/runtime-capture-sd-export-d1` from that exact commit. Its final head remains that same commit: there is no Phase D implementation commit. The new remote development ref is published at the unchanged baseline solely to identify the isolated review starting point.

Report branch: `codex-reports/2026-10-26-phase-d-capture-sd-export`, based exclusively on `lab-setup` commit `5829398cea5d337200393757e39a37d4d8148256`. This report is its only added file. Its own commit is deliberately not embedded recursively; read the report branch head on GitHub. Filename: `docs/codex-berichte/2026-10-26_phase-d-diagnostic-capture-sd-export.md`.

Read AGENTS.md from remote lab-setup (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`). The shallow/prepared checkout had no local lab-setup ref: `git show lab-setup:AGENTS.md` failed with `fatal: invalid object name 'lab-setup'`; the GitHub file was read successfully instead. This did not justify changing any protected branch.

Remote branch collection verified the expected baseline and absence of both requested new branches. Existing source reference heads: main `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`; lab-setup as above; Bug 001 `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`; Euclidean `67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72`; Phase A `e19bb0544dcc8f07138a9857c78022e8efaecd81`; Phase B `e4c0fd3ca66ced53413d5e1733e7ac9aa1c0da6e`; Phase C as above. No merge, force push, PR, firmware upload, physical device operation, flashing, option-byte write, bootloader change, or watchdog activation occurred.

## Objective and decisive stop condition

The requested facility would expose START/STOP/EXPORT/CLEAR through existing CONFIG controls, allow capture across menu navigation, arm nonblockingly for two seconds, record for at most ten minutes, freeze coherent statistics, and manually create exclusive 8.3 SD reports without damaging an unsaved snapshot or user files. Diagnostics must remain OFF by default and preserve the shared DWT timebase.

The user explicitly required stopping unsafe implementation if the architecture could not support reasonably bounded export or exposed unresolved SD/audio safety risks. The inspected disk layer has successful-transfer follow-up readiness loops with **no deadline, error exit, yield, or cancellation**. The transfer timeout preceding these loops is also `0xFFFFFFFF` in the configured source. A small formatted report or small application buffer cannot bound these lower-level operations. Applying a deadline around f_write cannot interrupt a synchronous lower-level call that never returns.

This is a confirmed source-level failure of the bounded-export prerequisite, safely reproduced with mocked card state. It is not evidence of an actual failed SD card, a physical hang, audible dropout, allocator corruption, or damaged user filesystem. No partial capture implementation was added because the initial audit stop condition applies before code changes. A separate reviewed storage reliability task is recommended first.

## Evidence classifications

- **CODE:** confirmed by inspected source.
- **BUILD:** executed host tests, build commands, or artifact analysis.
- **HYPOTHESIS:** plausible runtime consequence not physically observed.
- **OPEN:** requires further design, analysis, or authorized hardware validation.

## Initial architecture audit

### Existing diagnostics and default separation

`Core/Inc/safety/runtimeDiagnostics.h:6–7` defaults LXR_RUNTIME_DIAGNOSTICS to zero. `CMakeLists.txt:72–74` exposes the OFF-by-default CMake option and defines it only when enabled. `Core/Src/safety/runtimeDiagnostics.c` is conditionally compiled; the enabled global LxrDiagnostics is 72 bytes. The existing hooks are cumulative since initialization; there is currently no IDLE/ARMED/RECORDING/FROZEN/EXPORTING controller, frozen snapshot, capture timeout, SD export buffer, or diagnostic CONFIG page.

`Core/Src/Soundengine/Soundengine.c:26–33` surrounds rendering with start/end hooks. `Core/Src/stm32f7xx_it.c:272–276` surrounds sequencer/render/UART work in TIM2. `Core/Src/Soundengine/cs4344_cs5343.c` instruments per-stream DMA callbacks and wraps existing HAL rearm operations with single evaluation. OFF macros omit timing reads/counter updates; rearm executes its original call once. Existing safety tests exercise this behavior. No Phase D changes affect these objects; rebuilt image/ELF hashes equal the Phase C baseline, providing stronger artifact correspondence here than a partial object comparison.

Metrics: render/TIM2 count, max_cycles, over_budget; two DMA streams each callbacks, not_ready, rearm_errors, long_intervals, last_cycle; version and DWT availability. Counters saturate. Duration is unsigned cycle subtraction and supports less than a full counter period per interval. At nominal 216 MHz, one 32-bit cycle wrap is about 19.9 seconds; it cannot measure a ten-minute capture duration without a separate suitable timebase.

The existing nominal budget 313181 cycles is calculated from 64 frames and 44140.625 Hz, not a measured deadline or CPU load. DMA observations and nominal budget exceedances do not prove an audible underrun. There is no stack high-water or dynamic RAM measurement.

### Shared DWT/LCD timebase

`Core/Src/main.c:295` calls DWT_Delay_Init during boot. `Core/Src/delay.c:23` onward disables/enables trace/counter and resets CYCCNT during that existing boot initialization. `Core/Inc/delay.h` uses unsigned CYCCNT differences for blocking microsecond and nonblocking display timing. `Core/Src/lcd.c:27–31` uses DWT_Delay_us; later LCD write/command paths invoke that delay.

Existing diagnostics init unlocks/enables DWT but explicitly preserves CYCCNT. Capture START/STOP/CLEAR must never call DWT_Delay_Init or zero CYCCNT. It is a shared free-running timebase, not exclusive diagnostic state. Reinitializing it during capture would invalidate pending LCD/nonblocking intervals and diagnostic intervals. No such new reset was implemented. DWT availability status remains an initialization observation, not proof of an accurate CPU clock.

### CONFIG menu and input

`Core/Inc/menu/menuPages.h:369–399`: MENU_MIDI_PAGE is the CONFIG/global menu, first subpage has eight parameters/two screens; second has three parameters/one screen; later entries are empty. `Core/Src/menu/menu.c:1393` has2ndPage inspects the fifth label. At `3156–3200`, menu_switchSubPage advances to the next populated subpage only in the branch for an existing second screen; when the current page has no second screen, it resets directly to page zero. Therefore merely populating slot two would leave it unreachable through repeated CONFIG presses from the current slot one. This was reproduced using the actual extracted production navigation body and synthetic populated slot two.

`menu_switchPage` around3235 invokes this cycling for repeated CONFIG selection. `menu_parseEncoder:2729` processes encoder edges/edit mode; repaint around1810 chooses special pages or generic parameter display. Main at959 calls encoder handling; existing front-panel/dio handling continues at971. A future diagnostics handler must be a special UI action path before generic parameter handling and rendering, conditional on the diagnostic build. Do not add PAR_* values, parameter-array slots, presets, MIDI/NRPN targets, Morph indices, or automation entries. No new physical button mapping is needed or authorized.

A future minimal isolated navigation correction would apply the same bounded next-populated-page decision in the no-second-screen branch, preserving two-screen behavior and wrap. Both CONFIG and SEQ use this helper; test both and avoid accidental new SEQ behavior. This correction was **not implemented** while the mandatory audit blocker remains.

### FatFs and active SD path

`FATFS/App/fatfs.c:33,56` links SD_Driver; main mounts SDFatFS at367 and remounts in its main loop around926–928. The active driver table in FATFS/Target/sd_diskio.c points to SD_read, SD_write and SD_ioctl. These are synchronous polling calls, not the alternative DMA BSP entry points.

Path for export operations: main/UI → FatFs f_mkdir/f_open/f_write/f_sync/f_close → disk_read/disk_write/disk_ioctl → SD_read/SD_write → BSP_SD_ReadBlocks/BSP_SD_WriteBlocks → HAL polling SD transfer → BSP_SD_GetCardState readiness loop. Metadata directory search/create and final close can read/write sectors even if the report is short.

`FATFS/Target/sd_diskio.c:150–164` waits indefinitely after successful BSP read. `:179–195` has the same write issue. `FATFS/Target/bsp_driver_sd.c:205–207` maps every HAL state other than TRANSFER to BUSY. A removed/error/nontransfer card can therefore remain in the caller's loop; the readiness API loses the distinction between transient busy and terminal failure. This is a conditional code path, not a statement that actual cards necessarily enter it.

`sd_diskio.c:35–40` chooses SDMMC_DATATIMEOUT. `Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_ll_sdmmc.h:303–304` defines it as0xFFFFFFFF. HAL_SD_ReadBlocks at570 and WriteBlocks at781 use HAL_GetTick and compare elapsed milliseconds to Timeout in their polling loops. The huge supplied timeout is not an acceptable interactive export latency bound. A wall-clock bound cannot be obtained solely by limiting filename attempts or report bytes.

`sd_diskio.c:219–222` CTRL_SYNC simply returns RES_OK; it does not independently inspect card readiness. Existing synchronous writes wait for readiness, but the sync stub alone is not evidence of media durability. `Middlewares/Third_Party/FatFs/src/ff.c:3813–3821` f_close calls f_sync. `ff.c:3360` supports FA_CREATE_NEW collision detection. Future export must check every FRESULT and short write, sync/close, and only then report success, subject to lower-layer/media limitations.

Existing preset saves are synchronous and often call soundengine_process between application-level operations. Those calls cannot rescue a permanently blocked disk operation. Reusing global SDFile also creates ownership concerns if export is interleaved with other main-loop saves. A future bounded exporter needs its own FIL or explicit serialized ownership and must preserve existing file operations. No such mechanism was added.

### Audio scheduling and SD safety implications

`Core/Inc/Soundengine/config.h:29` enables CALC_SAMPLES_IN_INTERRUPT. TIM2 invokes seq_tick/render/UART; main soundengine_process also temporarily disables TIM2 updates while servicing sequencing/render. Main regularly calls soundengine_process at951,964,976. Audio DMA callbacks request/rearm buffers; they do not stop merely because transport is stopped.

Synchronous main-context SD waits do **not** automatically disable TIM2/audio DMA interrupts. Thus the audit does not claim that every SD export starves rendering or causes a glitch. However, while blocked, main UI/USB service, Morph ticks, and other foreground tasks cannot complete; card polling consumes CPU/bus bandwidth, and the foreground cannot execute a timeout outside the synchronous call. SDMMC priority2 and some SD DMA priority0 are above TIM2 priority4/audio DMA5; no priorities were changed. Actual preemption and peripheral/bus interference need measurement. No physical timing was measured.

Stopped transport is a useful future export precondition, not proof of silence or harmless I/O. A bounded record size and idle sequencer cannot fix the active lower-layer waits. Adding soundengine_process inside arbitrary disk polling is an unreviewed reentrancy/scheduling change, not an acceptable workaround.

## Completed actions and tests

Created only a new local development branch at the unchanged baseline. All audit host sources/logs were temporary files under /tmp/phase-d, outside the repository. No production or repository-test diff exists. The new development ref is baseline-only; do not infer an implemented feature from its name.

Native tests compiled extracted SD_read/SD_write bodies with mocked successful/failed BSP transfers and controlled readiness state. Finite busy and immediate transfer failure returned as expected. A permanently busy mock did not return before a 0.3-second subprocess deadline and was killed by the host runner. The source contains no termination condition, so this demonstrates the missing bound without running firmware or waiting indefinitely. Tests cover control flow, not real HAL timing/card media behavior.

A native test compiled the actual menu_switchSubPage body with mocked page labels and has2ndPage state. It confirmed the sequence first-page screen0 → first-page screen1 → second-page screen0 → first-page screen0, skipping synthetic populated third page. It did not test a correction.

Compiler: gcc -O1 -g -fsanitize=address,undefined -fno-pie -no-pie (navigation omitted -g). The initial audit test failed at process exit because LeakSanitizer cannot operate under this executor's ptrace environment. Reran with ASAN_OPTIONS=detect_leaks=0; ASan/UBSan remained enabled and all expected assertions passed. Leak checking for these temporary mocks was disabled and must not be reported as passed.

All four existing regression suites passed; all four build/configure command pairs and image validation passed. Builds reused prepared incremental outputs; the build commands reported only the always-run firmware-update instruction target, not a fresh clean compile of every translation unit. No printed installation instructions were executed. Same artifacts match prior Phase C hashes. No code changed, so additional object/code overhead is exactly zero and no OFF/ON behavior was added.

## Memory and artifacts

Firmware remains source version1.7, generated LXR_V2_V1.7.img and LXR_V2_V1.7.bin with ELF LXR_V2_STM32F765.elf. No equivalence to public official v1.71 or private support1.72 is claimed.

| Region | Normal | Diagnostics ON | Capacity | Normal / ON headroom |
|---|---:|---:|---:|---:|
| ITCM |15016|15088|16384|1368 /1296|
| DTCM |107392|107392|131072|23680 /23680|
| General RAM |271576|271648|393216|121640 /121568|

DTCM headroom is before actual runtime stack use, not a measured stack margin. Heap/stack minimum reservations are512/1024 bytes; they do not establish worst-case consumption. The existing72-byte enabled diagnostic structure is unchanged. Added capture-state/export-buffer sizes: zero, because not implemented. All Phase D Flash/ITCM/DTCM/RAM/instruction deltas are zero.

Full FLASH load footprint is captured by payload span from app0x08008000, including copied sections and gaps: normal Release275948 (remaining739860 of1015808), normal Debug275996 (739812), ON Release276388 (739420), ON Debug276436 (739372). The validator's FLASH memory_high_water_bytes below refers to ELF section VMA span (223084/223132/223452/223500), not full image load footprint. Do not compare these different measures as if identical. The first32KiB remain outside the application image. Initial vector stack20080000 and Thumb reset08014F79 remain plausible; actual startup relocates SP to DTCM as documented in A/B/C. No linker/startup/clock change occurred. Artifact SHA-256s and header/checksum measurements appear in the command transcript below, for all four configurations.

## Proposed design only — not implemented or validated

### Prerequisite storage work

Review a dedicated bounded SD operation design before resuming Phase D. Establish finite per-transfer and post-transfer-ready deadlines; distinguish terminal errors/removal; propagate failures consistently through disk layer; establish safe timeout recovery/abort and filesystem object ownership; consider how long metadata scans and close may take. A deadline fix alone does not prove audio real-time safety or reliable recovery. Changing shared diskio/HAL behavior affects existing project/pattern/kit operations and requires scoped tests and review; no speculative global storage changes were made here. Do not format, erase, globally mask interrupts, or call unrelated service code recursively from disk polling.

### Capture ownership and state

Use compile-time-only optional code in Flash. Keep OFF build free of all UI/state/export initialization and hook evaluation. Proposed state sequence: IDLE → ARMED at start with counters cleared → RECORDING after elapsed HAL millisecond tick>=2000 → FROZEN on user stop/elapsed>=600000 → EXPORTING only on explicit request and stopped playback → FROZEN whether export succeeds or fails. CLEAR explicitly resets frozen state to IDLE without touching files. Reject START outside IDLE. A user stop during ARMED needs an explicit specification decision (cancel to IDLE versus frozen zero-duration USER_STOP); do not silently invent it. Define stop duration from RECORDING start, and report stop request/finalization timing semantics.

HAL_GetTick is a candidate existing millisecond timebase; verify SysTick continues during all relevant foreground operations and use unsigned subtraction for wrap. Service arming/timeout centrally in the main loop, never in page repaint; avoid claiming a hard ten-minute limit if foreground can block. To enforce the limit despite delayed UI service, a bounded interrupt-side elapsed-time check or independently scheduled safe capture gate is required and must be reviewed for overhead. The present blocked storage layer is one reason not to promise reliable lifecycle timing from a main-only controller.

A single volatile flag plus memcpy is **not** a coherent snapshot solution. Rendering can occur in main and TIM2, and DMA IRQs update separate fields. Need explicit writer ownership and an in-flight policy: e.g. gate collection with capture generation tokens captured at begin, stop publish only at a foreground quiescent point after previous ISR writers return, and ensure resumed pre-stop main-render end hooks cannot mutate frozen counters. Verify Cortex-M ordering and aligned access, use barriers only as justified, and test pending lower-priority DMA/TIM2 interleavings. Do not use a seqlock reader that spins indefinitely when a preempted writer owns the sequence; do not introduce long IRQ masking or locks. Double buffers alone do not solve stale-token reuse or reset races. This is a design requirement, not an already proven algorithm.

No start/stop/clear may reset CYCCNT. When not RECORDING, minimize hooks to state gates and preserve HAL expression single evaluation. Saturate counters and expose saturation without claiming lost-event counts are exact. Measurements are durations/observations; no invented CPU-utilization percentage, stack usage, timestamps, clock measurement, or audible-underrun label.

### UI and export after prerequisite approval

Special-case diagnostic third CONFIG page and encoder actions outside parameter tables. Conditional labels/actions START STOP EXPORT CLEAR; state IDLE/ARM/REC/STOP/SAVE/ERR. Navigation fix must preserve existing two pages and test repeated entry/exit, short final page, SEQ shared helper, and no encoder double dispatch. Leaving a page must not clear results or stop capture.

Proposed export uses a fixed bounded buffer (size to be measured in implementation), its own controlled FIL, mkdir /DIAG only when needed, exclusive FA_CREATE_NEW /DIAG/LOG0001.TXT through LOG9999.TXT, bounded search without overwrite, all FRESULT/byte counts checked. Consider a bounded number of filename attempts per foreground service step, while recognizing each FatFs call also needs bounded underlying operations. Preserve RAM snapshot for all errors. Failed partial files should remain with their exclusive filename and be documented as incomplete; do not silently delete or reuse them. Success requires complete write plus successful final sync/close. Card removal, protection, full disk, exhausted filenames, short writes, close errors, and ownership serialization require mocked failure tests plus future authorized hardware verification.

Contents: schema, firmware string1.7, build identifier with exact commit only if reproducibly embedded and dirty-tree handling defined, recorded duration/reason/DWT status, all frozen timing/DMA counters, saturation, nominal216MHz and313181-cycle assumptions, limitations. No RTC timestamp or manufacturer equivalence invented. SD export must never occur automatically, during capture, or in fault/IRQ context.

## Required Phase D feature-test matrix status

Not implemented, therefore **not run**: all new state transitions; two-second arm; ten-minute timeout; duplicate START; STOP from another page; export rejection during recording; CLEAR; capture-specific saturation; unavailable DWT capture; capture wraparound; coherent snapshot interleavings; fixed CONFIG navigation and preservation; missing/full/write-protected card; write/close/sync failure handling; exclusive collisions/exhaustion; frozen retention on failed export; new diagnostic exclusion from Morph/NRPN/automation; new OFF build elimination; no collection after STOP. Existing safety tests cover baseline saturation/wrap/no-DWT/OFF macros, but do not substitute for these unimplemented feature tests. Existing Bug001 tests do not establish absence of an unimplemented new diagnostic parameter. The temporary navigation test demonstrates the old skip bug, not full UI regression coverage.

## Safety assessment and continuation

CODE/BUILD: Phase C source and all previous corrections preserved; default OFF; no new IRQ paths, heap calls, locks, waits, logging, startup/linker/clock/bootloader changes. BUILD: baseline tests/configuration commands/images pass. CODE/BUILD: post-transfer SD waits lack a termination bound and skip-third-page control flow reproduced. HYPOTHESIS: failing/removing a card during new export could freeze foreground and indirectly disrupt interaction/audio service. OPEN: real SD latency/audio interference, reliable disk abort/recovery, coherent capture algorithm, deadline service under foreground stalls, actual stack margin, physical USB recovery and broader data-state concurrency.

This audit is suitable for further **design review**. It is not a completed Phase D implementation and is not deployment-ready. Recommended next step: review/authorize a separate narrowly scoped SD timeout/recovery/ownership investigation and correction, then resume capture/UI/export with an explicit snapshot/timeout design and the full required deterministic test matrix. Hardware validation remains separately authorized only after software review, backup and recovery plan.

**NOT FLASHED / NOT HARDWARE VALIDATED.** No measured audio reliability, CPU utilization, stack high-water, media durability, or runtime latency is claimed.

## Actual command/output transcript

The following logs are the executed environment version, regression, configure/build, and image checks. Exit0 indicates command success; these are host/cloud results only.

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
$ python3 tests/bug001/run.py
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence

EXIT 0
```

### 05.log

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

### 06.log

```text
$ python3 tests/safety/run.py
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state

EXIT 0
```

### 07.log

```text
$ python3 tests/usb_allocator/run.py
PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing
PASS USB failures: 14 rollback/retention/retry scenarios plus invalid requests; no allocator calls
Host object: size=552 alignment=8 (target size/alignment checked separately in ELF)
PASS complete production USB class object: no heap/allocator-lock references

EXIT 0
```

### 08.log

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

### 09.log

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

### 10.log

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

### 11.log

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

### 12.log

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### 13.log

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

### 14.log

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### 15.log

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

### 16.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0xE6", "configuration": "release", "elf_sha256": "c31121510d8b3100d1dcdeeb89adffdb15c299837e24ded1e4af8f6926f62b7f", "image_bytes": 275964, "image_sha256": "06e9859d1ff99e3b80baf121225eb83865b8573e319f96243ccb037b291342a3", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223084, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275948, "reset_vector": "0x08014F79", "result": "PASS"}
{"checksum": "0xFF", "configuration": "debug", "elf_sha256": "5385d5fcd676f29e00d9e436979ebee55e6b8a1efdc9106b3e7c82930fe572b1", "image_bytes": 276012, "image_sha256": "da880ed68c4d0e65281e1b49aa9a007270c9ca625bc5d797d8bb0b066e9a2fd1", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223132, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275996, "reset_vector": "0x08014F79", "result": "PASS"}
{"checksum": "0xF3", "configuration": "diagnostics-release", "elf_sha256": "3e965f9cf60f4ace2511484f7cca774a1b3519b4bd4c3c08c75f2e90e8fe17da", "image_bytes": 276404, "image_sha256": "de6fe28745cb01377b2bcb7ae1c3f6f660c8050b084f5d005e9770dc117bcd30", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223452, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 276388, "reset_vector": "0x08014F79", "result": "PASS"}
{"checksum": "0xBD", "configuration": "diagnostics-debug", "elf_sha256": "32d65fcd2e5371f68d96aec33fad596440d3a48cdcd22c1b4b56a52b92ff2bbc", "image_bytes": 276452, "image_sha256": "d4173d4531cd87fe0b11885778f81df22e6c1dcd2677d4393534b167d248b037", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223500, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 276436, "reset_vector": "0x08014F79", "result": "PASS"}

EXIT 0
```

### Temporary audit tests

Command: `ASAN_OPTIONS=detect_leaks=0 python3 /tmp/phase-d/audit_host.py` (exit0).

```text
PASS read finite busy returned
CONFIRMED read permanently busy mock did not return; host process terminated after 0.3s (not hardware evidence)
PASS read transfer failure returned
PASS write finite busy returned
CONFIRMED write permanently busy mock did not return; host process terminated after 0.3s (not hardware evidence)
PASS write transfer failure returned
CONFIRMED actual navigation body wraps page 1 without second screen to page 0, skipping populated page 2
```

### Reproducible temporary test runner

This runner is included so another assistant can reproduce the audit without access to /tmp. Run from a checkout of the exact Phase C commit; it creates only /tmp test files. It compiles extracted production bodies and substitutes the explicitly shown BSP/menu state mocks.

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

## Publication verification

Publication is limited to the new baseline-only development ref and this single Markdown file on a new lab-setup-based report branch. Remote contents and new branch heads are read back after publication; the final response supplies the exact report commit and verified URL. No generated artifacts or temporary host files are uploaded.
