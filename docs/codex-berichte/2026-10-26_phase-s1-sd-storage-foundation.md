# Phase S1 — SD storage reliability foundation

Date: 2026-10-26. Repository: `DocLXR-02/lxr-desktop-app`.

## Outcome and scope

**IMPLEMENTED within the authorized S1 disk-driver scope; software validation passed.** This is a development correction for review, not complete SD storage reliability or approval for hardware deployment. Serious observed failures now close the disk interface until reboot; S1 cannot stop an already active physical transfer, undo uncertain writes, repair filesystem metadata, or make ignored application errors visible.

Read complete AGENTS.md from lab-setup (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`) and the complete Phase D/D0 reports. Phase D report branch `codex-reports/2026-10-26-phase-d-capture-sd-export`, commit `b40ff1105d4154e9cce654300f65206ef00464cc`; D0 report branch `codex-reports/2026-10-26-phase-d0-sd-storage-safety`, commit `a80a37e0704cc5f178a36878f1f22a155cc6f21e`. This task's explicit authorization separates the limited disk-driver foundation from the broader problems that blocked D/D0.

Verified baseline branch: `safety/usb-allocator-reentrancy-c1`, exact remote/local commit `091f502de6f57f3262ffc68659eafac6f8bdfa19`, tree `11f5e713e124416ecc13cc788d3123ae4c683d17`. Clean existing checkout `/workspace/lxr-desktop-app` used; no worktree created.

Source branch: `safety/sd-storage-foundation-s1`.
Final source commit: `a4be3b94c552dbc867b197fad76655bed40cea34`.
Its parent is the initial S1 correction commit `28a634030a65ab9330a2ac142d2fd4f8bc240769`, whose sole parent is the exact Phase C baseline. Initial S1 validation passed; final review then preserved healthy explicit remount initialization in a follow-up commit and reran all regression suites/builds/images. The final tree is `9afb21a5c1ae614431f4b87b35a0de18c234274e`. No source files changed after final validation; Git metadata publication/import does not alter validated file bytes or embed a different build identity.

Report-only branch: `codex-reports/2026-10-26-phase-s1-sd-storage-foundation`, created separately from lab-setup `5829398cea5d337200393757e39a37d4d8148256`. Only added file: `docs/codex-berichte/2026-10-26_phase-s1-sd-storage-foundation.md`. Its own commit is available from the report branch head/final response, not recursively embedded here.

All previous branches preserved, including main, lab-setup, both original fixes, Phase A/B/C, Phase D and D0. Source changes uploaded through GitHub Git-data APIs and a non-forced fast-forward of the dedicated new S1 ref, then imported locally to match; no merge, PR, force push, binary upload, hardware operation, bootloader or option-byte activity occurred. Local checkout is clean at the final source commit.

## Evidence labels

CODE: inspected source. BUILD: host execution, compiler/linker or binary analysis. HYPOTHESIS: plausible unmeasured physical effect. OPEN: subsequent design/validation required.

## Fault-state contract

| State/event | Result and permitted work |
|---|---|
| Fresh boot | Stat=STA_NOINIT, Fault=0; no data I/O before successful initialize |
| Startup absent card | Initialize returns NOINIT|NODISK without BSP init; later insertion can initialize because no uncertain transfer occurred |
| Healthy initialize | Calls BSP init and finite readiness wait; only successful completion publishes Stat=0 |
| Healthy explicit repeated init/remount | Calls BSP init again, preserving existing card-state/geometry refresh behavior |
| BSP initialization failure | Latches Fault and NOINIT until reboot |
| BSP read/write failure | First request returns RES_ERROR and latches Fault/NOINIT; no replay |
| Readiness timeout | RES_ERROR plus latched Fault/NOINIT |
| Detect switch shows removal during access/wait | RES_NOTRDY, Fault plus NOINIT|NODISK; reinsertion cannot clear latch |
| Status/sync sees persistent non-ready state | Uses same wait policy and latches on expiry |
| Any unresolved latched fault | Read/write/recognized ioctl reject with RES_NOTRDY; status/init retain NOINIT; no further BSP transfer/init/status/geometry calls |
| Invalid argument or forbidden caller context | Reject safely; do not turn malformed input into a media fault |
| Recovery | Reboot-required; no public clear function, retry, remount, reset or recovery write |

The latch is one file-static byte independent of FatFs driver-link or filesystem objects. Ordinary SD_status, SD_initialize and driver unlink/relink cannot clear it. A latched fault blocks reads too, avoiding unsafe continued interaction with a possibly active transfer. It reduces hotplug convenience after a serious observed fault by design. It is not persistent across power-off/reboot, does not autonomously initiate a reboot, and does not prove media safe to reuse after reboot.

The interface assumes the existing serialized bare-metal foreground owner. Calls from exceptions, with PRIMASK/BASEPRI/FAULTMASK nonzero, or with disabled SysTick/counter interrupts are rejected before storage work. No locks or interrupt masking were added. These checks do not establish that an enabled SysTick can never malfunction or be starved; finite readiness requires the existing HAL tick to advance. Debug halt, catastrophic interrupt starvation or hardware clock failure is not solved by S1.

## Changed files and rationale

1. `FATFS/Target/sd_diskio.c` — only production change. Replaced the old readiness loops/status clearing with fault-guarded operations, distinct software millisecond budgets, context/range checks and meaningful CTRL_SYNC. Retained license/header and original CRLF. Generated scaffolding/comments and unused optional initialization template were condensed; the project's active BSP-init behavior remains enabled. No header, CMake, HAL, BSP or FatFs source changed.
2. `tests/sd_storage/README.md` — contract, commands, mocks and limitations.
3. `tests/sd_storage/run.py` — reproducible native compiler/sanitizer runner with subprocess deadline.
4. `tests/sd_storage/host.c` — includes the **whole production driver**; hardware mocks and assertions.
5. `tests/sd_storage/ff_gen_drv.h` — test disk types and driver table shape.
6. `tests/sd_storage/stm32f7xx_hal.h` — test CMSIS/tick/card-info declarations.

Combined baseline-to-final diff: six files,302 insertions/193 deletions. No existing regression was weakened or modified. Full diff and whitespace review completed with `git -c core.whitespace=cr-at-eol diff --check 091f502de6f57f3262ffc68659eafac6f8bdfa19` (exit0). Existing defaults and Bug001/Euclidean/heap/diagnostic/USB fixes remain untouched.

## Source design and decisive references

Final `FATFS/Target/sd_diskio.c`:

- Lines31–39: distinct 5000ms transfer and5000ms readiness constants, max128 sectors,512-byte sector convention.
- Lines41–59: existing Stat plus new Fault byte; SD_Fail only sets latch and NOINIT.
- Lines45–51: foreground/unmasked/enabled-SysTick contract guard; no privileged register write.
- Lines61–85: access guard/card presence and wrap-safe readiness loop.
- Lines88–109: status/init consistency, failure persistence and healthy explicit initialization.
- Lines112–141: geometry-based subtraction range validation and guarded transfers; exactly one BSP transfer per accepted request.
- Lines146–164: validated ioctl, finite CTRL_SYNC, conventional metadata widths.
- Lines168–176: same active disk operation table.

Fault check precedes BSP initialization; no normal polling path assigns Fault=0. The only clear in tests is a clearly labeled simulated reboot. Stat becomes0 only after BSP init and readiness success, with no faulted path reaching that assignment. Status does not overwrite a latched NOINIT merely because a card later reports TRANSFER.

Ranges: reject NULL, count0 or>128, nonzero LUN and sector/count beyond LogBlockNbr. `count <= capacity && sector <= capacity-count` avoids overflowing sector+count. The documented disk-driver count range1–128 matches this FatFs sector/cluster API convention; test all values. Buffer capacity and validity beyond NULL cannot be proven from a raw pointer; callers must provide count*512 accessible bytes. The BSP HAL polling path consumes bytes and the existing cast is retained; no DMA alignment or buffer layout change was introduced.

Ioctl rejects NULL metadata destinations and unsupported commands. GET_SECTOR_SIZE now writes a WORD, as required by FatFs's ioctl contract, rather than the old DWORD write into a potentially two-byte destination; this is a directly related disk-interface bounds correction. GET_BLOCK_SIZE retains the old LogBlockSize/512 convention without speculatively fixing erase geometry. CTRL_SYNC accepts NULL data and checks readiness/latch; it does not certify card-cache/power-loss durability.

## Timeout policy and limits

HAL_GetTick is a millisecond timebase (default1kHz, `stm32f7xx_hal.c`), advanced in `Core/Src/stm32f7xx_it.c:210–215`; TICK_INT_PRIORITY0 is configured in `Core/Inc/stm32f7xx_hal_conf.h:148`. Existing supported calls run in foreground with interrupts available. The code uses unsigned `(uint32_t)(HAL_GetTick()-start) >= 5000u`, supporting counter wrap during the short wait. DWT/CYCCNT is not used, written, reset or reinitialized; LCD timing is untouched.

Policy: up to64KiB/128 sectors per request,5000ms passed to BSP polling transfer, plus5000ms for subsequent readiness. This is a generous development allowance relative to ordinary transfer/programming behavior, chosen to avoid a very short interactive timeout and provide one consistent budget across supported counts. It is **not** a manufacturer-tested supported-card latency guarantee, an SD standard maximum, or a measured worst case. There is no empirical card qualification in this cloud task. Cards exceeding policy may be conservatively rejected until reboot. Future authorized card testing must evaluate policy before deployment; tune only from evidence.

SDMMC_DATATIMEOUT remains unchanged as a hardware-register constant. S1 no longer passes its0xFFFFFFFF value as the millisecond transfer argument. Initialization also uses finite driver readiness after BSP init, but the driver cannot preempt a BSP init that has not returned.

**No end-to-end physical operation deadline is claimed.** BSP_GetCardState and HAL transfer/init invoke inner command/status/stop loops; a single lower-layer call can outlast the driver's budget. The loop checks elapsed time after a non-ready status return, so a slow individual command can also overrun the readiness allowance. A late ready return is accepted. These lower-layer deadlines and peripheral cleanup are explicitly deferred to S2. SysTick malfunction despite enabled control bits and unbounded interrupt starvation also remain outside the guarantee.

## Failure fidelity and recovery behavior

BSP returns MSD_ERROR for any non-HAL_OK transfer, collapsing timeout/busy/CRC and other errors. S1 preserves that as conservative RES_ERROR plus a fault rather than guessing exact cause. Only the existing detect-switch observation supplies NODISK; generic failure is not called removal or write protection. No reliable protection sensor is introduced and RES_WRPRT is not invented. Invalid API parameters use RES_PARERR; unavailable/uninitialized/context-disallowed requests use RES_NOTRDY.

The latch prevents **new driver-issued** physical writes after an observed ambiguous failure. It neither aborts the current card transaction nor guarantees that a card ceases programming after HAL returns. No HAL abort, peripheral reset, DMA change, read/write replay, automatic reinit/remount, filesystem format, erase or deletion is invoked. Existing main insertion logic may attempt a remount, but latched SD_initialize rejects it before BSP init; its effects on higher-level cached objects remain a later concern.

Healthy repeated initialization was deliberately preserved in the final follow-up: initial S1 returned status without BSP init when already initialized, which could prevent an explicit healthy remount from refreshing geometry after an unobserved card swap. The final code calls BSP init on explicit healthy initialize requests; calls after failure remain rejected. This is not automatic recovery: S1 itself schedules no initialize or reset, and never initializes after its latch is set.

## Tests and execution environment

Prepared PATH for firmware commands: `/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH`. CMake3.31.10, Ninja1.13.0.git.kitware.jobserver-pipe-1, Python3.12.14, ARM GCC14.2.1 (20241119;15:14.2.rel1-1), newlib-nano, native cc/GCC14.2.0. Cloud environment checked through the runtime skill/tool; no credential values or secrets inspected/published.

`python3 tests/sd_storage/run.py`: PASS, actual production translation unit included with real local BSP declarations. Native `-std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`. LeakSanitizer explicitly disabled due to the executor's tracing limitations; no leak-validation claim.15-second host process timeout ensures a bad mock test cannot hang the job.

Mocks: BSP transfer/init/status/presence/geometry, HAL tick, CPU exception/mask registers and SysTick bits. File-static reset is test-only simulated reboot. Hardware transfer functions assert request data and count, increment call counters and return controlled results. No real SD card, electrical timing, FAT metadata, USB or audio runtime execution is simulated.

| Required scenario | Actual result |
|---|---|
| Single read/write and multi-sector1–128 | PASS, exact end-of-card edge and preserved RES_OK |
| Delayed readiness | PASS, five status polls in simulated delay |
| Permanent busy | PASS,5000 mocked ms then RES_ERROR; latch |
| BSP read failure | PASS, one read call, zero ready polls, fault retained |
| BSP write/ambiguous failure | PASS, one write call; subsequent physical writes/init/status/geometry denied |
| Repeated requests after fault | PASS,20 rounds of read/write/status/init/sync/geometry checks with unchanged hardware call counters |
| Removal before and during post-write wait | PASS, known detect absence returns NOTRDY/NODISK; reinsertion cannot clear latch |
| Startup absent, then insertion | PASS, no BSP init while absent; later healthy init allowed |
| BSP init failure/readiness timeout | PASS, NOINIT persists through repeat init |
| CTRL_SYNC ready/delayed/busy/faulted | PASS, no unconditional success on unresolved fault |
| Tick wrap near UINT32_MAX | PASS, successful delayed read and permanent-busy write timeout |
| Invalid pointers/count/LUN/overflowing ranges/ioctl | PASS, no transfer and no media fault; metadata WORD destination sanitizer-safe |
| Forbidden IRQ/masks/SysTick disabled | PASS, no physical access; transient context rejection does not poison healthy state |
| No reset/remount/retry | PASS at driver-link/call-graph scope: those functions are not provided by the host fixture; unexpected references fail link; counters verify no repeats |

HAL_TIMEOUT is not independently simulated at a HAL ABI layer: unchanged BSP collapses it to the same MSD_ERROR covered by failure tests. Peripheral quiescence, real programming delays, initialization internals, DWT/LCD behavior on device, filesystem consistency and audio behavior require later work. No host test is described as hardware validation.

All four existing suites passed on final bytes: Bug001, Euclidean (six ASan/UBSan groups), Phase B safety and USB allocator. No pre-existing failure suppressed. All normal and diagnostic-enabled Release/Debug firmware builds passed. The first implementation and final healthy-init adjustment both passed; commands/output below distinguish intermediate from final artifacts. Final test-only coverage addition for removal was rerun before publication. Final complete builds compiled the changed storage object and relinked; unchanged translation units were reused incrementally. No clean-rebuild or exhaustive whole-program verification claim is made. No tests failed in S1.

## Resource and binary results — final revision

| Region | Phase C normal | S1 normal | Phase C ON | S1 ON | Capacity |
|---|---:|---:|---:|---:|---:|
| ITCM |15016|15016|15088|15088|16384|
| DTCM |107392|107392|107392|107392|131072|
| General RAM |271576|271576|271648|271648|393216|
| Full Release payload span |275948|276868|276388|277308|1015808 application Flash|

Final Flash payload delta **+920 bytes** in all four configurations. Initial intermediate result was+1088; do not confuse it with the final result. General RAM aggregate delta0 despite one new latch byte, accommodated by existing linker packing. `Fault` is1 byte at0x2002094d; `Stat` remains1 byte at0x20020058. No heap allocation, persistent storage record or large local array was added. Additional local geometry uses a fixed HAL card-info object; no whole-program stack high-water measurement is claimed.

Normal headroom: ITCM1368, DTCM23680 before runtime stack, RAM121640. ON headroom: ITCM1296, DTCM23680, RAM121568. Runtime stack and heap usage remain unmeasured. Minimum stack/heap reservations do not prove safety.

Final Release/Debug payloads276868/276916; ON Release/Debug277308/277356. Full application Flash headroom738940/738892/738500/738452 bytes. Image format16-byte LXRV2IMG header/LE length/checksum/bin identity, ELF memory limits and plausible vectors passed. Initial MSP0x20080000, reset0x08015311 (Thumb), application origin0x08008000; existing startup moves SP to DTCM. Reset vector movement reflects code layout, not a startup change. Source version remains1.7; generated LXR_V2_V1.7.img/.bin and LXR_V2_STM32F765.elf remain local. No binary uploaded. No claim of equivalence to official public v1.71 or private support1.72.

Final SHA-256/length/checksum/ELF details appear in `final-08.log` below. Validator FLASH high-water bytes224004/224052/224372/224420 are ELF VMA metrics, not the complete Flash payload span including copied load sections/gaps.

All four configurations' Soundengine.c, cs4344_cs5343.c, stm32f7xx_it.c and bsp_driver_sd.c objects remain byte-identical to Phase C. Only sd_diskio.c changes among those compared. No additional code/work in audio ISR paths, no ITCM movement, IRQ source or priority change. Source diff contains no bootloader/linker/startup/clock/protection change. Physical write protection was not inspected.

## Deferred risks and next phases

**S2:** correct/review BSP fidelity and HAL/LL command-response/stop/init deadlines and timeout cleanup. `stm32f7xx_ll_sdmmc.h` command5000 and STOP100000000 feed an iteration-count conversion that can overflow32 bits. Public HAL millisecond Timeout does not preempt these loops. `HAL_SD_WriteBlocks` timeout may return READY/ContextNONE without stopping transfer or correctly setting timeout ErrorCode; do not infer quiescence from READY. No broad HAL reset/abort workaround was added.

**S3–S5 (proposed sequencing, not completed):** FatFs ignored mirror/FSInfo write errors and disk_initialize cached-success bookkeeping; preset helper short-write/close error propagation and shared FIL lifetime; filesystem session/remount ownership and any controlled recovery; then integrate full regression and design safe Phase D export. Exact later phase boundaries should be agreed in review rather than presumed implemented. S1 cannot make a caller that ignores a returned error honest, and does not change existing FA_CREATE_ALWAYS truncation/cleanup behavior or user-data format.

CODE/BUILD: S1 driver latches serious observed failure, refuses new I/O until reboot and bounds its own readiness loop under the advancing-tick/returning-lower-call assumptions. CODE: higher layers and inner HAL waits remain unresolved. HYPOTHESIS: conservative timeouts may reject unusually slow valid cards; polls/status may occupy foreground for seconds and affect UI/MIDI responsiveness. OPEN: actual interrupt/bus impact, SD latency, quiescence, media durability, stack headroom and audio reliability.

The existing TIM2 audio interrupt can continue while foreground SD polling runs, but this is not proof of glitch-free audio. SD status calls themselves may now wait under policy rather than performing one immediate status query. No recursive soundengine_process call, global masking or new task was added. The5-second policy does not add a new fixed sleep: ready cards return immediately, busy/error cards terminate instead of waiting indefinitely, subject to S2 inner waits.

Review S1 as a foundation only; proceed to S2 software work before resuming diagnostic SD exporter implementation or considering hardware deployment. Further phases, backup/recovery planning and Daniel's separate explicit authorization remain required.

**NOT FLASHED — NOT HARDWARE VALIDATED.**

## Executed command transcripts

Initial commands are included for reproducibility. Final logs supersede intermediate image/memory values after the healthy-initialize follow-up.

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
$ python3 tests/sd_storage/run.py
PASS all 1..128 sector read/write counts; exact range edge and 5000ms HAL argument
PASS finite readiness, permanent busy deadline, tick wrap success/timeout; persistent reboot-only fault
PASS BSP read/write/init failure, ambiguous write blocks subsequent I/O; removal/reinsertion cannot clear latch
PASS CTRL_SYNC ready/delayed/timeout/fault; status cannot clear or bypass fault
PASS invalid pointers/counts/LUN/overflowing ranges/ioctls; invalid input does not poison valid operation
PASS IRQ/masked/suspended tick rejection; no reset/remount/retry dependencies in linked driver

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
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      225024 B       992 KB     22.15%
   text	   data	    bss	    dec	    hex	filename
 276176	    852	 341112	 618140	  96e9c	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 277036 bytes, checksum 0xEA
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
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      225072 B       992 KB     22.16%
   text	   data	    bss	    dec	    hex	filename
 276224	    852	 341112	 618188	  96ecc	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 277084 bytes, checksum 0xBB
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
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      225392 B       992 KB     22.19%
   text	   data	    bss	    dec	    hex	filename
 276616	    852	 341184	 618652	  9709c	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 277476 bytes, checksum 0x27
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
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      225440 B       992 KB     22.19%
   text	   data	    bss	    dec	    hex	filename
 276664	    852	 341184	 618700	  970cc	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 277524 bytes, checksum 0x56
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

### 17.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0xEA", "configuration": "release", "elf_sha256": "a791ab076304808335226b3cd33663418774a70a3e436b3fdda4b18430c0e915", "image_bytes": 277052, "image_sha256": "06df1f66a58ba407ae8acc4f3fd43408da01a02275443dd28e3e5f5db9911aee", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224172, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 277036, "reset_vector": "0x080153BD", "result": "PASS"}
{"checksum": "0xBB", "configuration": "debug", "elf_sha256": "c2fb14323f4ca7275f9845c21e0c1cb1dac83b3aaae55bf887384cb4d16722f7", "image_bytes": 277100, "image_sha256": "9c3ed021fcceb588e4c8a4b90890500b86de86e6e0e665f01f5d79e72638a414", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224220, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 277084, "reset_vector": "0x080153BD", "result": "PASS"}
{"checksum": "0x27", "configuration": "diagnostics-release", "elf_sha256": "bf57b031583a5f7bb72b02d5933aae93e0ade53a625a36c1a204116b2a68823a", "image_bytes": 277492, "image_sha256": "c6a816d8743f31731358eb7054dcb0ffba961319002229a1c4ffb3a7019c8c58", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224540, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 277476, "reset_vector": "0x080153BD", "result": "PASS"}
{"checksum": "0x56", "configuration": "diagnostics-debug", "elf_sha256": "e61f93d1bd4170644a0ddbf8112f5c039679051d30436fe807e980681207a120", "image_bytes": 277540, "image_sha256": "5b3352be0dfdc5395b88f048292e9a116515ac2acc21ecd6ec0c49e6c4c3a9f3", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224588, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 277524, "reset_vector": "0x080153BD", "result": "PASS"}

EXIT 0
```

### final-sd.log

```text
python3 tests/sd_storage/run.py (exit0)
PASS all 1..128 sector read/write counts; exact range edge and 5000ms HAL argument
PASS finite readiness, permanent busy deadline, tick wrap success/timeout; persistent reboot-only fault
PASS BSP read/write/init failure, ambiguous write blocks subsequent I/O; removal before/during transfer and reinsertion cannot clear latch
PASS CTRL_SYNC ready/delayed/timeout/fault; status cannot clear or bypass fault
PASS invalid pointers/counts/LUN/overflowing ranges/ioctls; invalid input does not poison valid operation
PASS IRQ/masked/suspended tick rejection; no reset/remount/retry dependencies in linked driver
```

### final-00.log

```text
$ python3 tests/bug001/run.py
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence

EXIT 0
```

### final-01.log

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

### final-02.log

```text
$ python3 tests/safety/run.py
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state

EXIT 0
```

### final-03.log

```text
$ python3 tests/usb_allocator/run.py
PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing
PASS USB failures: 14 rollback/retention/retry scenarios plus invalid requests; no allocator calls
Host object: size=552 alignment=8 (target size/alignment checked separately in ELF)
PASS complete production USB class object: no heap/allocator-lock references

EXIT 0
```

### final-04.log

```text
$ cmake --build --preset release --parallel 4
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      224856 B       992 KB     22.14%
   text	   data	    bss	    dec	    hex	filename
 276008	    852	 341112	 617972	  96df4	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 276868 bytes, checksum 0xBA
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

### final-05.log

```text
$ cmake --build --preset debug --parallel 4
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      224904 B       992 KB     22.14%
   text	   data	    bss	    dec	    hex	filename
 276056	    852	 341112	 618020	  96e24	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 276916 bytes, checksum 0xD8
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

### final-06.log

```text
$ cmake --build build/diagnostics-release --parallel 4
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      225224 B       992 KB     22.17%
   text	   data	    bss	    dec	    hex	filename
 276448	    852	 341184	 618484	  96ff4	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 277308 bytes, checksum 0x10
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

### final-07.log

```text
$ cmake --build build/diagnostics-debug --parallel 4
[1/3] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      225272 B       992 KB     22.18%
   text	   data	    bss	    dec	    hex	filename
 276496	    852	 341184	 618532	  97024	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 277356 bytes, checksum 0x33
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

### final-08.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0xBA", "configuration": "release", "elf_sha256": "ab4a5c4ac2fac23775b5e30ffbd12b3d137bdaaee73d9d904b54d2dc2d9a4069", "image_bytes": 276884, "image_sha256": "1728ab9586b90df7df1a825b837d47375ee947764b9ffceecfcc3c13371ee1b6", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224004, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 276868, "reset_vector": "0x08015311", "result": "PASS"}
{"checksum": "0xD8", "configuration": "debug", "elf_sha256": "d4517309c8be09ad7c584734dd8ad4eef39db1efa9d2433f5d7b12d0983c54b3", "image_bytes": 276932, "image_sha256": "edeff41ae5afec6a292f77eae1026c836048faa4e74fd7fd98749230c1e479d2", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224052, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 276916, "reset_vector": "0x08015311", "result": "PASS"}
{"checksum": "0x10", "configuration": "diagnostics-release", "elf_sha256": "29bd47e6ec9a1cbaab4704a36b4a4127dfa65b7cad4278c4e7ae046794ecd5eb", "image_bytes": 277324, "image_sha256": "e28b74fbdca1ee7f9e998ea0443ccaf596a5be2e72f4b53734428dc4901e15cc", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224372, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 277308, "reset_vector": "0x08015311", "result": "PASS"}
{"checksum": "0x33", "configuration": "diagnostics-debug", "elf_sha256": "3eda37c7db800c601869f6d7eea911bdc65d47568a36263106464c444cfa3ef6", "image_bytes": 277372, "image_sha256": "4897782c9e5440de7590320543da08ee37b474e0fee7f3a114cb51e4248c3e6a", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224420, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 277356, "reset_vector": "0x08015311", "result": "PASS"}

EXIT 0
```

## Final object comparison

```text
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED 5f6fa6ba76fd1a4792e4b67bf9098622c22285d2899ad2ba14187bc6973f6f4f
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED d1d426fccabe65ff11979c545afa1d7e67b6dd09f021f195c2188f00e13eccee
build/debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 72519986928007069087a7eed0942c711cb2e272226ef1c18ad7085186d8043a
build/debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj: UNCHANGED 6f6613e154b661089a31376d71255decc41bd247ea43826dcc24ca34bd675c80
build/debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj: CHANGED c252baf206173d4bcedbd936f14a83293d56cda725b11c9e2b500bf8679de6f7
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED d6ee7b60ad6a2a1924e096b3631b8f38284349e3ad7c037c348e5ca3e7f17aca
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 10472838499bb9806639fc82153ba880797c3c496ac48e10e6724aeaa2e12ac5
build/release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 01463848afda54efbbeea116af34c9ff35db325406b42b003432ee46fba5c468
build/release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj: UNCHANGED c2ecbd314ff85224322a8ab344122e53fbfb06846075c47e2842bc2ae0ba6342
build/release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj: CHANGED 214f679c4f91c15e4d488b22a029572f0b0fb4e45dcadbc4d81ee7d447eaaa7c
build/diagnostics-release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 66cd4b9de166101afc4fae3970b75e726495517a6de6957ca4b7ef72863fa5d6
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED d1ed2c18ce02443fa1ed7ddae0633a61de9f31e39ba3882bb5f73378ad38dea2
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 46b4aa19065504b170ae148e4e6d0794856bd512529aa837a4d5e77479c5ab7d
build/diagnostics-release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj: UNCHANGED c2ecbd314ff85224322a8ab344122e53fbfb06846075c47e2842bc2ae0ba6342
build/diagnostics-release/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj: CHANGED 214f679c4f91c15e4d488b22a029572f0b0fb4e45dcadbc4d81ee7d447eaaa7c
build/diagnostics-debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 3fa6b6fbec925306271e90d029e0910f0eeacb0232dc244bf3a2fd40aab905b6
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED 6282e9bc0e06c0a2945f6baf9be36f5670aabd823d9e0e48c23da6e052a2a34c
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 2ac6331a297b1d626080c6dc4e5ebaff3be4f31872d0a16df6c9d0488f71788b
build/diagnostics-debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj: UNCHANGED a6ae57f8027ae9b3f5443e9b87b2870d4510d1a561a58fe35f31a14ab9e23181
build/diagnostics-debug/Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj: CHANGED 134d990430257542c8f1a04b44f539b46373ea340c8d302dfd43490c6102b4b9
```

## Complete final production implementation

Path: FATFS/Target/sd_diskio.c. This is the sole production change.

```c
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sd_diskio.c
  * @brief   SD Disk I/O driver
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Note: code generation based on sd_diskio_template_bspv1.c v2.1.4
   as "Use dma template" is disabled. */

/* USER CODE BEGIN firstSection */
/* can be used to modify / undefine following code or add new definitions */
/* USER CODE END firstSection*/

#include "ff_gen_drv.h"
#include "sd_diskio.h"

/* Software milliseconds, not the SDMMC hardware data-timeout register.
 * Up to 128 sectors (64 KiB): allow 5 s for the polling transfer and another
 * 5 s for card programming/readiness. Conservative development policy, not a
 * guarantee for every card. Inner HAL command waits remain outside this bound.
 */
#define SD_TRANSFER_TIMEOUT_MS 5000u
#define SD_READY_TIMEOUT_MS 5000u
#define SD_MAX_SECTORS 128u
#define SD_DEFAULT_BLOCK_SIZE 512u

static volatile DSTATUS Stat = STA_NOINIT;
/* Cleared only by C runtime initialization at reboot; never by mount/init. */
static uint8_t Fault;

static int SD_ContextOK(void)
{
  /* HAL's millisecond tick must be able to preempt this foreground caller. */
  return __get_IPSR() == 0u && __get_PRIMASK() == 0u &&
         __get_BASEPRI() == 0u && __get_FAULTMASK() == 0u &&
         (SysTick->CTRL & (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk)) ==
         (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);
}

static DRESULT SD_Fail(void)
{
  Fault = 1u;
  Stat |= STA_NOINIT;
  return RES_ERROR;
}

static DRESULT SD_Access(BYTE lun)
{
  if(lun != 0u) return RES_PARERR;
  if(!SD_ContextOK() || Fault || (Stat & STA_NOINIT)) return RES_NOTRDY;
  if(BSP_SD_IsDetected() != SD_PRESENT) {
    (void)SD_Fail();
    Stat |= STA_NODISK;
    return RES_NOTRDY;
  }
  return RES_OK;
}

static DRESULT SD_WaitReady(void)
{
  uint32_t start = HAL_GetTick();
  for(;;) {
    if(BSP_SD_IsDetected() != SD_PRESENT) {
      (void)SD_Fail();
      Stat |= STA_NODISK;
      return RES_NOTRDY;
    }
    if(BSP_SD_GetCardState() == MSD_OK) return RES_OK;
    if((uint32_t)(HAL_GetTick() - start) >= SD_READY_TIMEOUT_MS)
      return SD_Fail();
  }
}

DSTATUS SD_status(BYTE lun)
{
  if(lun != 0u || !SD_ContextOK()) return STA_NOINIT;
  if(Fault || (Stat & STA_NOINIT)) return Stat;
  if(SD_Access(lun) == RES_OK) (void)SD_WaitReady();
  return Stat;
}

DSTATUS SD_initialize(BYTE lun)
{
  if(lun != 0u || !SD_ContextOK()) return STA_NOINIT;
  if(Fault) return Stat;
  /* Preserve explicit healthy remount initialization; faults never reach BSP. */
  /* An absent card at startup has not created an uncertain transfer. */
  if(BSP_SD_IsDetected() != SD_PRESENT) return (Stat = STA_NOINIT | STA_NODISK);
  Stat = STA_NOINIT;
  if(BSP_SD_Init() != MSD_OK) {
    (void)SD_Fail();
    return Stat;
  }
  if(SD_WaitReady() == RES_OK) Stat = 0u;
  return Stat;
}

static int SD_ValidRange(DWORD sector, UINT count)
{
  BSP_SD_CardInfo info;
  BSP_SD_GetCardInfo(&info);
  return count <= info.LogBlockNbr && sector <= info.LogBlockNbr - count;
}

DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
  DRESULT result;
  if(!buff || count == 0u || count > SD_MAX_SECTORS) return RES_PARERR;
  result = SD_Access(lun);
  if(result != RES_OK) return result;
  if(!SD_ValidRange(sector, count)) return RES_PARERR;
  if(BSP_SD_ReadBlocks((uint32_t*)buff, sector, count, SD_TRANSFER_TIMEOUT_MS) != MSD_OK)
    return SD_Fail();
  return SD_WaitReady();
}

#if _USE_WRITE == 1
DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  DRESULT result;
  if(!buff || count == 0u || count > SD_MAX_SECTORS) return RES_PARERR;
  result = SD_Access(lun);
  if(result != RES_OK) return result;
  if(!SD_ValidRange(sector, count)) return RES_PARERR;
  if(BSP_SD_WriteBlocks((uint32_t*)buff, sector, count, SD_TRANSFER_TIMEOUT_MS) != MSD_OK)
    return SD_Fail();
  return SD_WaitReady();
}
#endif

#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff)
{
  BSP_SD_CardInfo info;
  DRESULT result;
  if(cmd != CTRL_SYNC && cmd != GET_SECTOR_COUNT && cmd != GET_SECTOR_SIZE &&
     cmd != GET_BLOCK_SIZE) return RES_PARERR;
  if(cmd != CTRL_SYNC && !buff) return RES_PARERR;
  result = SD_Access(lun);
  if(result != RES_OK) return result;
  if(cmd == CTRL_SYNC) return SD_WaitReady();
  BSP_SD_GetCardInfo(&info);
  switch(cmd) {
  case GET_SECTOR_COUNT: *(DWORD*)buff = info.LogBlockNbr; break;
  case GET_SECTOR_SIZE: *(WORD*)buff = info.LogBlockSize; break;
  /* Preserve the existing block-size reporting convention. */
  case GET_BLOCK_SIZE: *(DWORD*)buff = info.LogBlockSize / SD_DEFAULT_BLOCK_SIZE; break;
  default: return RES_PARERR;
  }
  return RES_OK;
}
#endif

const Diskio_drvTypeDef SD_Driver = {
  SD_initialize, SD_status, SD_read,
#if _USE_WRITE == 1
  SD_write,
#endif
#if _USE_IOCTL == 1
  SD_ioctl,
#endif
};
```

## Complete regression fixture and runner

These files are permanent on the source branch. The header shims and test README are there as well.

### tests/sd_storage/host.c

```c
/* Whole production driver, real local BSP declarations; simulated hardware only. */
#include <assert.h>
#include <stdio.h>
#include "FATFS/Target/sd_diskio.c"
SysTick_Type host_systick;
uint32_t host_ipsr,host_primask,host_basepri,host_faultmask;
static uint32_t tick, step, busy, polls, reads, writes, inits, infos;
static uint8_t present, transfer_result, init_result;
static uint32_t last_count, last_sector, last_timeout, remove_at;
uint32_t HAL_GetTick(void) { uint32_t n=tick; tick+=step; return n; }
uint8_t BSP_SD_IsDetected(void) { return polls >= remove_at ? SD_NOT_PRESENT : present; }
uint8_t BSP_SD_Init(void) { ++inits; return init_result; }
uint8_t BSP_SD_GetCardState(void) { ++polls; assert(polls<100000); return polls<=busy?SD_TRANSFER_BUSY:MSD_OK; }
void BSP_SD_GetCardInfo(BSP_SD_CardInfo *i) { ++infos; i->LogBlockNbr=1024; i->LogBlockSize=512; }
uint8_t BSP_SD_ReadBlocks(uint32_t *b,uint32_t s,uint32_t n,uint32_t t)
{ assert(b);++reads;last_count=n;last_sector=s;last_timeout=t;return transfer_result; }
uint8_t BSP_SD_WriteBlocks(uint32_t *b,uint32_t s,uint32_t n,uint32_t t)
{ assert(b);++writes;last_count=n;last_sector=s;last_timeout=t;return transfer_result; }
static BYTE data[128*512];
/* Test-only simulated reboot. Production provides no latch-clear operation. */
static void reboot(void)
{
 Stat=STA_NOINIT;Fault=0;host_systick.CTRL=3;
 host_ipsr=host_primask=host_basepri=host_faultmask=0;
 tick=0;step=1;busy=polls=reads=writes=inits=infos=0;
 present=SD_PRESENT;transfer_result=init_result=MSD_OK;remove_at=UINT32_MAX;
}
static void start(void) { reboot();assert(SD_initialize(0)==0 && inits==1);polls=0; }
static void locked(void)
{
 uint32_t r=reads,w=writes,i=inits,p=polls,inf=infos;DWORD v;
 for(unsigned n=0;n<20;n++) {
  assert(SD_status(0)&STA_NOINIT);assert(SD_initialize(0)&STA_NOINIT);
  assert(SD_read(0,data,0,1)==RES_NOTRDY);assert(SD_write(0,data,0,1)==RES_NOTRDY);
  assert(SD_ioctl(0,CTRL_SYNC,0)==RES_NOTRDY);
  assert(SD_ioctl(0,GET_SECTOR_COUNT,&v)==RES_NOTRDY);
 }
 assert(reads==r && writes==w && inits==i && polls==p && infos==inf);
}
int main(void)
{
 reboot();assert(SD_read(0,data,0,1)==RES_NOTRDY && reads==0);
 present=SD_NOT_PRESENT;assert(SD_initialize(0)==(STA_NOINIT|STA_NODISK) && inits==0);
 present=SD_PRESENT;assert(SD_initialize(0)==0 && inits==1);
 for(unsigned n=1;n<=128;n++) {
  start();assert(SD_read(0,data,1024-n,n)==RES_OK && reads==1);
  assert(last_count==n && last_sector==1024-n && last_timeout==5000);
  assert(SD_write(0,data,1024-n,n)==RES_OK && writes==1 && last_count==n);
  assert(SD_initialize(0)==0 && inits==2);
 }
 puts("PASS all 1..128 sector read/write counts; exact range edge and 5000ms HAL argument");
 start();busy=4;assert(SD_write(0,data,0,1)==RES_OK && polls==5);
 start();busy=UINT32_MAX;assert(SD_read(0,data,0,1)==RES_ERROR && polls==5000);locked();
 start();tick=UINT32_MAX-2500;busy=UINT32_MAX;assert(SD_write(0,data,0,1)==RES_ERROR && polls==5000);locked();
 start();tick=UINT32_MAX-2;busy=4;assert(SD_read(0,data,0,1)==RES_OK && tick<10);
 puts("PASS finite readiness, permanent busy deadline, tick wrap success/timeout; persistent reboot-only fault");
 start();transfer_result=MSD_ERROR;assert(SD_read(0,data,0,1)==RES_ERROR && reads==1 && polls==0);locked();
 start();transfer_result=MSD_ERROR;assert(SD_write(0,data,0,1)==RES_ERROR && writes==1 && polls==0);locked();
 start();present=SD_NOT_PRESENT;assert(SD_write(0,data,0,1)==RES_NOTRDY && writes==0);present=SD_PRESENT;locked();
 reboot();init_result=MSD_ERROR;assert(SD_initialize(0)&STA_NOINIT);init_result=MSD_OK;locked();
 reboot();busy=UINT32_MAX;assert(SD_initialize(0)&STA_NOINIT);locked();
 start();busy=10;remove_at=2;assert(SD_write(0,data,0,1)==RES_NOTRDY && writes==1);remove_at=UINT32_MAX;locked();
 puts("PASS BSP read/write/init failure, ambiguous write blocks subsequent I/O; removal before/during transfer and reinsertion cannot clear latch");
 start();assert(SD_ioctl(0,CTRL_SYNC,0)==RES_OK);polls=0;busy=3;
 assert(SD_ioctl(0,CTRL_SYNC,0)==RES_OK && polls==4);
 polls=0;busy=UINT32_MAX;assert(SD_ioctl(0,CTRL_SYNC,0)==RES_ERROR);locked();
 start();busy=UINT32_MAX;assert(SD_status(0)&STA_NOINIT);locked();
 puts("PASS CTRL_SYNC ready/delayed/timeout/fault; status cannot clear or bypass fault");
 start();DWORD v=0;WORD size=0;
 assert(SD_ioctl(0,GET_SECTOR_COUNT,&v)==RES_OK && v==1024);
 assert(SD_ioctl(0,GET_SECTOR_SIZE,&size)==RES_OK && size==512);
 assert(SD_ioctl(0,GET_BLOCK_SIZE,&v)==RES_OK && v==1);
 assert(SD_ioctl(0,99,&v)==RES_PARERR);
 assert(SD_ioctl(0,GET_SECTOR_COUNT,0)==RES_PARERR);
 for(unsigned n=0;n<2;n++) {
  assert((n?SD_read(0,0,0,1):SD_write(0,0,0,1))==RES_PARERR);
  assert((n?SD_read(0,data,0,0):SD_write(0,data,0,0))==RES_PARERR);
  assert((n?SD_read(0,data,0,129):SD_write(0,data,0,129))==RES_PARERR);
  assert((n?SD_read(0,data,1024,1):SD_write(0,data,1024,1))==RES_PARERR);
  assert((n?SD_read(0,data,UINT32_MAX,2):SD_write(0,data,UINT32_MAX,2))==RES_PARERR);
 }
 assert(SD_read(1,data,0,1)==RES_PARERR && SD_initialize(1)&STA_NOINIT);
 assert(!Fault && reads==0 && writes==0 && SD_status(0)==0);
 puts("PASS invalid pointers/counts/LUN/overflowing ranges/ioctls; invalid input does not poison valid operation");
 uint32_t *context[]={&host_ipsr,&host_primask,&host_basepri,&host_faultmask};
 for(unsigned n=0;n<4;n++) {
  start();*context[n]=1;
  assert(SD_read(0,data,0,1)==RES_NOTRDY && SD_write(0,data,0,1)==RES_NOTRDY);
  assert(SD_status(0)&STA_NOINIT);assert(SD_initialize(0)&STA_NOINIT);
  assert(SD_ioctl(0,CTRL_SYNC,0)==RES_NOTRDY && reads==0 && writes==0 && polls==0);
  *context[n]=0;assert(SD_status(0)==0 && !Fault);
 }
 for(unsigned n=0;n<3;n++){start();host_systick.CTRL=n;assert(SD_read(0,data,0,1)==RES_NOTRDY && !reads);}
 puts("PASS IRQ/masked/suspended tick rejection; no reset/remount/retry dependencies in linked driver");
}
```

### tests/sd_storage/run.py

```python
#!/usr/bin/env python3
"""Compile the whole production disk driver with mocked BSP/CMSIS, ASan/UBSan."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='lxr-sd-storage-') as directory:
    binary=Path(directory)/'storage'
    env=os.environ.copy(); env['ASAN_OPTIONS']='detect_leaks=0'
    command=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
             '-fsanitize=address,undefined','-fno-sanitize-recover=all',
             '-fno-omit-frame-pointer','-fno-pie','-no-pie','-I'+str(HERE),
             '-I'+str(ROOT),str(HERE/'host.c'),'-o',str(binary)]
    subprocess.run(command,check=True,cwd=ROOT,env=env)
    subprocess.run([str(binary)],check=True,cwd=ROOT,env=env,timeout=15)
```

## Publication verification

The new source branch and report-only branch are read back from GitHub. Source parent/tree/blob identities are checked against validated local content; report contents and single-file lab-setup parent are verified. All prior branch heads are compared with the pre-task snapshot. The final response supplies the exact report URL/commit. Generated binaries and build directories are excluded.
