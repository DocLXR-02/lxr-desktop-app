# Phase S3 — FatFs data integrity and error propagation

Date: 2026-10-26. Repository: `DocLXR-02/lxr-desktop-app`.

## Outcome and authorized scope

**IMPLEMENTED within the authorized S3 filesystem/glue scope; software validation passed.** Every required FAT-copy write and FAT32 FSInfo write now has its result checked. Failure stops that synchronization path and is returned rather than masked. Disk initialization caches only confirmed availability and rechecks actual status instead of fabricating success. A directly related f_sync correction retains the file error and modified state, preventing a later close from silently succeeding or replaying uncertain writes.

This is not transactional metadata rollback or complete SD stack safety. Some copies/sectors may already have reached the medium before a later failure. S1/S2 fail-closed behavior remains intact. High-level save-result handling, project cleanup, failed shared-handle ownership and recovery are reserved for S4. No physical validation or installation approval is implied.

## Repository, instructions and exact commits

Read complete AGENTS.md from remote lab-setup (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`) and the complete D0/S1/S2 report files and results. Reference report commits:

- D0 `a80a37e0704cc5f178a36878f1f22a155cc6f21e`, branch `codex-reports/2026-10-26-phase-d0-sd-storage-safety`.
- S1 `e66f31b979455d977993ad21583a4ff191ea6cf6`, branch `codex-reports/2026-10-26-phase-s1-sd-storage-foundation`.
- S2 `bebc12701234e6b2f622449c72e291c6f0957f7f`, branch `codex-reports/2026-10-26-phase-s2-sd-hal-bsp-timeouts`.

Verified local/remote source baseline: `safety/sd-hal-bsp-timeouts-s2`, exact commit `c88933a4b1968c8f9d173b71d86154e8969436be`, tree `34a9f2fcc4ad41efd9ac292e53f6846b73b27f46`. Existing checkout `/workspace/lxr-desktop-app` was clean; no worktree was created.

New source branch: `safety/sd-fatfs-integrity-s3`.
Final source commit: **`cbffa2d756658b6f32d8e6417ea40ec2cda828d5`**.
Final tree: `e3a395983bd68a36c382679ba2a5e903e0cc43a7`.
Parent: `36ceff4d9cfb7381a44b13abff93064794e509d8`, the main S3 source/test correction, whose sole parent is the verified S2 baseline. The final commit adds test/documentation coverage wiring the public failure through the actual S1 disk driver; it does not change firmware production code. Builds use the identical final production bytes, with final host tests and image checks rerun at the final committed revision.

Report-only branch: `codex-reports/2026-10-26-phase-s3-fatfs-data-integrity`, independently based on lab-setup `5829398cea5d337200393757e39a37d4d8148256`. Only added file: `docs/codex-berichte/2026-10-26_phase-s3-fatfs-data-integrity.md`. Its own commit is read from the report branch head/final response instead of recursively embedded here.

GitHub publication uses exact blob/tree/commit objects and a dedicated new source ref. The test-only follow-up is a non-forced fast-forward with expected-head checking. Local Git objects/branch/index were synchronized to the published commits; checkout is clean. All earlier branches, including main, lab-setup, original fixes and S1/S2, remain unchanged. No merge, PR, force push or generated artifact upload. No physical SD access, flashing, hardware operation, bootloader/option-byte/clock/startup/linker/priority change, format or deletion occurred.

## Evidence labels

CODE: inspected source. BUILD: executed native tests, compiler/linker or artifact analysis. HYPOTHESIS: plausible unmeasured physical consequence. OPEN: future design/validation required.

## Initial findings and directly related call chain

FatFs R0.12c in `Middlewares/Third_Party/FatFs/src/ff.c` has a shared sector window. sync_window flushed the primary sector, cleared wflag immediately, then ignored subsequent FAT-copy errors. sync_fs constructed FAT32 FSInfo, ignored its disk_write return, and cleared fsi_flag. Both could lose a confirmed error. The generic disk_initialize wrapper in `src/diskio.c` marked its cache before knowing initialization result and returned initial RES_OK on later cached calls without asking the driver.

The immediate public path also mattered: f_sync calls move_window/sync_fs but cleared FA_MODIFIED even on synchronization failure, and it did not honor fp->err. Its dirty-file-buffer write error returned without storing the file error. Thus the first failing f_sync could return an error while a later f_close called a now-unmodified f_sync and received success. Merely returning FR_DISK_ERR from the two private helpers was insufficient. The small f_sync correction is directly within S3's requested public propagation scope, using existing fields/macros and no new interface.

Call paths reviewed: sync_window → move_window/sync_fs → f_sync → f_close; validate → disk_status; disk_initialize → linked driver initialize or status; FATFS_LinkDriverEx/UnLinkDriverEx and main's insertion-driven unlink/relink/mount flow. Existing ff_gen_drv.c is unchanged: link initializes its cache slot to zero; unlink removes the pointer; relink resets the cache before a healthy explicit initialization. S1's persistent latch lives in sd_diskio.c, outside those cached registration objects.

## Production changes and precise references

Only two production files changed; vendor license text and original CRLF retained. No package replacement or version/configuration change.

### 1. FAT-copy writes: ff.c:883–910, sync_window

Each required secondary copy is checked, in the existing primary→ascending-mirror order. On the first failure, return FR_DISK_ERR immediately. Do not write any later copy in that invocation. fs->wflag is cleared only after every required write succeeds. A primary error also preserves the dirty flag, as before. Non-FAT windows still require only their one ordinary window write; clean windows do not write.

Decisive behavior:

```c
if (disk_write(fs->drv, fs->win, wsect, 1) != RES_OK) {
    return FR_DISK_ERR; /* Stop; retain the unverified dirty window. */
}
/* ... after all copies ... */
fs->wflag = 0;
```

The function retains the original window sector/content, and no new copy-progress array or retries are introduced. If primary succeeds and mirror fails, the on-media copies may disagree and the failed sector result may be ambiguous. Retaining wflag means synchronization has not been verified; it does not promise the primary was rolled back or identify exactly what the failed write did. A later explicit call in an independent non-latching stub can attempt the dirty window again. In the real firmware S1 latches a serious disk error and blocks subsequent physical writes. This distinction is intentional and tested, not hidden behind a test stub that always latches.

The internal generic loop is tested with1–4 synthetic copies. Actual unchanged mount validation supports1/2 FATs; tests do not claim the parser now mounts3/4-copy volumes. FAT layout, copy offsets and successful write bytes remain unchanged.

### 2. FSInfo: ff.c:946–975, sync_fs

The FSInfo disk_write is checked. On failure, return FR_DISK_ERR before clearing fsi_flag or invoking CTRL_SYNC. The pending-update value1 remains. The window then contains the newly constructed FSInfo sector and winsect is volbase+1; wflag is not falsely used to mark an old FAT window. Free-cluster/last-cluster counters remain in memory and an explicit subsequent sync_fs can reconstruct FSInfo if a different backend permits retry. Replacing that cached window later does not itself clear the retained fsi_flag.

Only acknowledged FSInfo write clears its pending flag. If the write succeeds but final CTRL_SYNC fails, sync_fs returns failure; fsi_flag is already zero because the write itself succeeded. That is not a claim of media durability. Public f_sync preserves the file's modified/error state on the overall failure.

No FSInfo update needed (flag0 or disabled0x80), non-FAT32 volume behavior, structure signatures/free-cluster fields and sector ordering are preserved. CTRL_SYNC still executes after successful/no-needed metadata work and its error is propagated. An earlier observed metadata-write failure cannot be overwritten by an unrelated successful CTRL_SYNC in the same call because the function returns first.

### 3. Public file error persistence: ff.c:3736–3810, f_sync

After successful object validation, honor existing fp->err. Use the existing ABORT macro for a dirty file-buffer disk failure, retaining FA_DIRTY and recording FR_DISK_ERR. Clear FA_MODIFIED only if synchronization returned FR_OK (including the compile-time exFAT branch for consistency). Record a returned FR_DISK_ERR in the existing file error field before leaving.

```c
if (fp->err) LEAVE_FF(fs, (FRESULT)fp->err);
/* dirty data buffer failure: */
if (disk_write(...) != RES_OK) ABORT(fs, FR_DISK_ERR);
/* after sync_fs: */
if (res == FR_OK) fp->flag &= (BYTE)~FA_MODIFIED;
/* before leaving a validated operation: */
if (res == FR_DISK_ERR) fp->err = (BYTE)res;
```

f_close at3820 onward is not modified. It already returns a failing f_sync result and retains the object/lock when that result fails. The correction prevents a later close from skipping failed work and invalidating the handle as if synchronization had succeeded. Normal successful close still clears the file object and releases the existing lock.

Important result distinction: with the independent RAM backend kept healthy after an injected write failure, later f_sync/f_close return the retained FR_DISK_ERR without new metadata writes. Through the actual S1 driver, the first public mirror failure returns FR_DISK_ERR and latches storage. The subsequent f_close's validate sees NOINIT and returns FR_INVALID_OBJECT before the per-file error check. Both are failures; no physical replay occurs and the handle is retained. S3 does not promise every later failure has the identical numeric result after the volume becomes unavailable.

No new volume-wide error manager or ABI field was added. An unmodified file's f_sync still follows existing no-modified-work behavior; raw private helper callers and other file objects rely on their own return handling and the production S1 volume fault. A later unrelated successful sync cannot erase this file's fp->err, demonstrated without relying on S1. New f_open initializes its own file state as before; S3 does not implement application ownership/recovery policy for discarding failed objects.

### 4. Initialization bookkeeping: diskio.c:51–74

Reject out-of-range/unregistered/missing-callback drivers with STA_NOINIT. If not cached initialized, call the underlying initializer and inspect its result before setting the cache. If already cached, call the underlying status callback and return that result instead of fabricated zero. Normalize NODISK to include NOINIT so the FatFs mount check cannot treat absent media as initialized. Cache true only when neither bit is present; preserve protection bits, so a protected but available medium is not confused with an uninitialized one.

```c
if(disk.is_initialized[pdrv] == 0)
    stat = disk.drv[pdrv]->disk_initialize(disk.lun[pdrv]);
else
    stat = disk.drv[pdrv]->disk_status(disk.lun[pdrv]);
if(stat & STA_NODISK) stat |= STA_NOINIT;
disk.is_initialized[pdrv] = (stat & (STA_NOINIT | STA_NODISK)) == 0;
return stat;
```

A cached status showing failure clears the cache and returns failure immediately; it does not automatically initialize/recover in the same invocation. A separate explicit initialize call may reach the lower initializer again. After non-destructive startup absence S1 allows insertion/init; after a serious latched fault S1 rejects that call before BSP init. Unlink/relink resets generic cache but cannot clear the persistent SD latch. Healthy repeated mount without relink checks current status; healthy unlink/relink permits existing explicit lower initialization. No peripheral reset, replay or automatic remount added.

Other generic wrappers and multi-drive registration hole behavior are not broadly hardened. Current configured _VOLUMES=1 and its lifecycle are tested. Bounds checks cover all BYTE drive values for disk_initialize, not a claim every disk API now validates arbitrary external drive indices.

## Changed files and diff summary

- `Middlewares/Third_Party/FatFs/src/ff.c`.
- `Middlewares/Third_Party/FatFs/src/diskio.c`.
- `tests/fatfs_integrity/README.md`.
- `tests/fatfs_integrity/host.c`.
- `tests/fatfs_integrity/main.h`.
- `tests/fatfs_integrity/run.py`.

Combined diff:6 files,321 insertions/10 deletions. Production-only change28 insertions/10 deletions. Full combined diff appended below, including permanent test sources. Whitespace review `git -c core.whitespace=cr-at-eol diff --check c88933a4b1968c8f9d173b71d86154e8969436be` passed. No source diff outside expected production files and tests. No presets, file formats, cleanup, diagnostics, HAL/BSP, clocks, priorities, startup/linker or protection settings changed.

## Test methodology

New suite: `python3 tests/fatfs_integrity/run.py`.

The native fixture includes the **complete production** ff.c, diskio.c, ff_gen_drv.c and sd_diskio.c in one translation unit, with real structures, macros, registration, synchronization helpers and public operations. These are not rewritten algorithm approximations or extracted helper-only tests. Directly constructed valid in-memory file objects isolate synchronization failures, and actual f_mount parses a RAM FAT16 boot sector for mount lifecycle tests.

Production integer.h assumes 32-bit long, unlike LP64 hosts. The fixture predefines only its guard and provides exact target-width integer typedefs, with size assertions; production header unchanged. The existing S1 CMSIS/HAL shim supplies modeled hardware/time declarations, and a tiny main.h stub satisfies ffconf.h's otherwise unused application include. BSP calls are mocked, not real HAL/electrical execution.

The RAM media callback records sector order/content and injects the requested Nth-write failure; it deliberately does not implement a storage latch. This proves private helper propagation and persistent file errors independently of the SD driver. A separate wired case uses the actual S1 driver behind actual generic disk/registration plus BSP mocks, proving first public mirror-error propagation, unavailable-volume close, and no new physical driver write calls. Test-only Stat/Fault assignment is clearly simulated reboot/setup; production has no new latch clear path.

Two native configurations:

1. Production ffconf.h settings: read/write, code page850, LFN0, one volume,512-byte sectors, TINY0, exFAT0, LOCK1, REENTRANT0.
2. Temporary test-only read-only variant: READONLY1, LOCK0, MKFS0, all other settings retained. These are required compatible read-only settings and do not modify firmware build configuration.

Real f_read verifies512 data bytes and read-only close issues no metadata writes. RAM FAT16 boot geometry is parsed by actual f_mount:512-byte sectors, two FATs, FAT base1, data base97. No physical filesystem mounted or formatted. The boot sector advertises8192 sectors to form valid FAT16 geometry; the sparse fixture stores only the low256 sectors exercised by these mount/read/sync tests and asserts accesses stay in that RAM range. It is not a complete filesystem/media emulator.

Native flags: `-std=c11 -O1 -g -Wall -Wextra -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie -ffunction-sections -fdata-sections -Wl,--gc-sections`. The full modules are compiled, with unused sections discarded at link. LeakSanitizer disabled because executor tracing does not support it; ASan/UBSan active/fatal.20-second process deadline per configuration. Unused static helper warnings are disabled because the whole vendor module is included; an existing unused lun parameter warning in unchanged ff_gen_drv.c remains visible. No existing tests were weakened or modified.

## Required scenario coverage and actual results

| Group | Executed scenarios and outcome |
|---|---|
| FAT copies | PASS primary success/failure; secondary success/failure;1–4 synthetic copies with every failure position; stop at first failure; sector order9,25,41,57; dirty flag retained on failure/cleared only on complete success; non-FAT single write and clean no-write |
| FSInfo | PASS successful exact signatures/free-count/next-free image at volbase+1; failure returns FR_DISK_ERR and retains pending flag; no CTRL_SYNC after metadata error; no-update/disabled flag and non-FAT32 behavior; CTRL_SYNC success/failure |
| Retry distinction | PASS no retry inside failing helper; explicit separate FSInfo retry in a non-latching mock is demonstrated and identified; later public sync/close on a failed file performs no replay even after mock recovery/unrelated successful sync |
| Public propagation | PASS actual f_sync/f_close for primary/mirror/FSInfo/data-cache/CTRL_SYNC failures; FA_MODIFIED retained; fp->err persisted; handle/lock not falsely released; normal sync/close still succeeds |
| S1 integration | PASS public mirror write failure through actual SD driver: first FR_DISK_ERR, later close FR_INVALID_OBJECT, no additional physical write callback; persistent latch unaffected |
| Initialization | PASS first success/failure, repeated request after failure, cached driver status, absent startup then available, NODISK-only normalization, protected available medium, healthy unlink/relink; invalid drives1–255/unregistered0 fail safely |
| Mount lifecycle | PASS actual f_mount with valid RAM FAT16 boot sector, repeated healthy mount using status, absent mount FR_NOT_READY then available mount FR_OK; expected geometry unchanged |
| Latched fault/remount | PASS actual generic registration+S1: ambiguous write fault, cached status rejection,10 repeated initialize requests and unlink/relink cannot invoke another BSP init/write |
| Read-only behavior | PASS actual f_read payload and normal read-only close in both native configurations; no metadata writes |

No automatic format/delete/reset/recovery API is invoked. The host compiles filesystem formatting/deletion code as part of the vendor source but tests never call it; the linker discards unused code. Direct helper retries in deliberately non-latching stubs are not proof of safe physical retries. Actual card outcome/latency, SDMMC quiescence and audio behavior remain untested.

All established suites also PASS:

- `python3 tests/bug001/run.py`.
- `python3 tests/euclidean/run.py` (six ASan/UBSan groups).
- `python3 tests/safety/run.py`.
- `python3 tests/usb_allocator/run.py`.
- `python3 tests/sd_storage/run.py`.
- `python3 tests/sd_hal/run.py`.

All seven suites passed; final expanded S3 suite rerun at the committed final revision. Firmware production code was unchanged by the final test/documentation follow-up. Four builds and image checks passed. Exact command/output transcripts follow.

### Development errors and warnings

Early `python3 tests/fatfs_integrity/run.py` invocations failed while constructing the native fixture: missing main.h shim, then missing BSP header include path; read-only compilation then exposed fields excluded from read-only FATFS/FIL layouts, corrected with configuration guards. Adding real mount tests also exposed that vendor st_word is compiled out in read-only mode; a test-only little-endian RAM-boot-sector helper replaced that fixture dependency. These were fixture compile failures, not production failures, and all were resolved. No sanitizer runtime failure or unresolved firmware/test failure remains.

The unchanged vendor ff_gen_drv.c unused lun warning is emitted in both native configurations. ARM firmware builds produced no new warning/error. No warning was hidden by weakening existing suites. One shell inspection printed an error log after a failed test, so its last-command exit status was not the test status; the Python traceback showed the failed compile and the runner itself returned failure. Final verification captures the subprocess return explicitly and checks it before claiming success.

## Firmware environment, builds and image validation

Each firmware shell activates PATH `/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH`. CMake3.31.10, Ninja1.13.0.git.kitware.jobserver-pipe-1, Python3.12.14, ARM GCC14.2.1 20241119 (15:14.2.rel1-1) with newlib-nano; native GCC14.2.0. Environment capability/readiness checked; no credentials or secrets exposed.

Configured/built Release and Debug default diagnostics OFF; separate diagnostics-release/debug directories ON. Default LXR_RUNTIME_DIAGNOSTICS remains OFF. Incremental builds compiled the two modified production objects and relinked, reusing unchanged objects. No claim of four clean rebuilds. Existing build target prints installation instructions; none were executed. Final image checker rerun at final committed source head.

All images passed16-byte LXRV2IMG header, LE payload length, checksum convention, exact .bin correspondence, application Flash/ELF memory limits and plausible vectors. Source version remains1.7; generated LXR_V2_V1.7.img/.bin and LXR_V2_STM32F765.elf stay local. No binary publication and no equivalence claim with manufacturer public v1.71/private support1.72. No official binary examined in S3.

| Metric | S2 normal Release | S3 normal Release | S2 diagnostics Release | S3 diagnostics Release |
|---|---:|---:|---:|---:|
| ITCM |15016|15016|15088|15088|
| DTCM |107392|107392|107392|107392|
| General SRAM |271576|271576|271648|271648|
| Linker FLASH used |223536|223632|223904|224000|
| Full application payload |275548|275644|275988|276084|

**Delta: +96 bytes Flash**, all four configurations; ITCM/DTCM/SRAM unchanged. Existing byte fields store the new bookkeeping; no new ABI field, allocation, static buffer or large local array. Tests have larger RAM fixtures, but these are native only and not linked into firmware.

Capacity/headroom: ITCM16KiB leaves1368 normal/1296 ON; DTCM128KiB leaves23680 before runtime stack; general RAM384KiB leaves121640 normal/121568 ON. No runtime stack/heap high-water measurement; linker reservations are not worst-case stack proof.

Application Flash begins0x08008000 and spans992KiB=1015808 bytes, first32KiB excluded. Payloads: Release275644, Debug275692, diagnostics Release276084, diagnostics Debug276132. Full payload headroom740164/740116/739724/739676 bytes respectively. Validator FLASH VMA high-water222780/222828/223148/223196 differs from complete load/payload span; linker FLASH used is another displayed metric, not interchangeable with payload bytes. Initial MSP0x20080000; reset0x08014E4D (Thumb), shifted only by code layout. Existing startup relocates runtime SP to DTCM; no startup/linker/vector configuration change.

Final exact image/ELF SHA-256, lengths and checksums are in pinned-images.log below. All12 compared audio/render/DMA/interrupt object files match S2 byte for byte, including Debug. Only ff.c/diskio.c objects change among those compared. No audio scheduling, interrupt, clock, bootloader or protection source changed. Full object inventory is appended.

## Remaining limits and S4 boundaries

CODE/BUILD: checked FAT-copy/FSInfo errors reach the corrected helpers and public sync/close path; flags remain truthful and file errors prevent implicit close replay. CODE/BUILD: initialization no longer pre-caches or fabricates success, and S1 survives actual registration/init attempts. CODE: no recovery writes, reset, formatting, file-format migration or on-disk ordering change added.

HYPOTHESIS/OPEN: partial media writes can still leave inconsistent FATs or metadata; neither software acknowledgement nor CTRL_SYNC proves power-loss durability. S2's timebase assumptions and physical cleanup uncertainty remain. No hardware SD recovery, card latency, audio reliability or CPU/stack runtime measurement performed.

Failed file objects/locks remain retained rather than falsely closed. With configured _FS_LOCK=1, a failed handle can keep the file lock occupied. S1 already makes storage unavailable after serious I/O failure, but higher-level code must handle the failed object deliberately; S3 does not automatically discard it, retry it, clear its error, or reset/remount. Application callbacks that ignore returned errors can still misreport saves. Existing named/tmp kit/pattern/project save helpers still contain ignored write/short-write/close results, and project cleanup may depend on misleading helper success. These known S4 risks are not concealed or solved here.

Recommended S4 scope:

1. Trace and preserve the first error/short-write through high-level save helpers and their callers; stop later save/cleanup work after failure.
2. Define one-owner shared SDFile lifecycle, including every early return and failed-close path. Do not clear fp->err or reuse a failed handle merely to release a lock or make a save appear successful.
3. Preserve tmp/recovery data on unsuccessful saves; do not delete/format/retry as automatic recovery.
4. Maintain existing file formats and S1 reboot-only policy. Any explicit discard/remount/volume-cache recovery design needs separate safety review rather than an incidental reset.
5. Add injected write/short-write/close failure tests for named and autosave kit/pattern/project flows, asserting no later writes or destructive cleanup and truthful UI/result propagation within the authorized next phase.

S3 is suitable for software review and continuation to S4, not installation. Complete subsequent storage phases, backup/recovery planning and separate explicit physical-test authorization before controlled hardware validation.

**NOT FLASHED — NOT HARDWARE VALIDATED.** No physical SD card read, written, formatted or deleted. No firmware binary uploaded. No bootloader, option-byte, startup/linker, clock, priority, automatic reset or watchdog change.

## Actual command and output transcripts

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
$ python3 tests/fatfs_integrity/run.py
In file included from /workspace/lxr-desktop-app/tests/fatfs_integrity/host.c:16:
/workspace/lxr-desktop-app/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c: In function ‘FATFS_UnLinkDriverEx’:
/workspace/lxr-desktop-app/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c:80:50: warning: unused parameter ‘lun’ [-Wunused-parameter]
   80 | uint8_t FATFS_UnLinkDriverEx(char *path, uint8_t lun)
      |                                          ~~~~~~~~^~~
Configuration: configured
PASS actual sync_window: 1..4 FATs, all primary/later failures, stop ordering, dirty retention, no internal replay; non-FAT/clean windows
PASS actual sync_fs: exact FSInfo layout, write/error/pending flag, explicit retry distinction, no-update/other FAT, CTRL_SYNC and no success masking
PASS full production f_sync/f_close: primary/mirror/FSInfo/data/sync errors stay sticky, modified state and handle retained, no implicit replay with healthy stub; success closes normally
PASS whole generic disk/registration/S1: successful/failed/cache status, absent then available, protected, healthy unlink/relink and all invalid drive bytes; serious SD latch survives init/remount with no physical replay
PASS actual f_read and read-only file close: payload unchanged, no metadata writes
In file included from /workspace/lxr-desktop-app/tests/fatfs_integrity/host.c:16:
/workspace/lxr-desktop-app/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c: In function ‘FATFS_UnLinkDriverEx’:
/workspace/lxr-desktop-app/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c:80:50: warning: unused parameter ‘lun’ [-Wunused-parameter]
   80 | uint8_t FATFS_UnLinkDriverEx(char *path, uint8_t lun)
      |                                          ~~~~~~~~^~~
Configuration: readonly
PASS whole generic disk/registration/S1: successful/failed/cache status, absent then available, protected, healthy unlink/relink and all invalid drive bytes; serious SD latch survives init/remount with no physical replay
PASS actual f_read and read-only file close: payload unchanged, no metadata writes

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
$ python3 tests/sd_hal/run.py
PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP
PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap
PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset
PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation

EXIT 0
```

### 11.log

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

### 12.log

```text
$ cmake --build --preset release --parallel 4
[1/4] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[2/4] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      223632 B       992 KB     22.02%
   text	   data	    bss	    dec	    hex	filename
 274784	    852	 341112	 616748	  9692c	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 275644 bytes, checksum 0x65
[4/4] Firmware update instructions

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

### 13.log

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

### 14.log

```text
$ cmake --build --preset debug --parallel 4
[1/4] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[2/4] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      223680 B       992 KB     22.02%
   text	   data	    bss	    dec	    hex	filename
 274832	    852	 341112	 616796	  9695c	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 275692 bytes, checksum 0x13
[4/4] Firmware update instructions

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

### 15.log

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### 16.log

```text
$ cmake --build build/diagnostics-release --parallel 4
[1/4] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[2/4] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      224000 B       992 KB     22.05%
   text	   data	    bss	    dec	    hex	filename
 275224	    852	 341184	 617260	  96b2c	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 276084 bytes, checksum 0xFE
[4/4] Firmware update instructions

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

### 17.log

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### 18.log

```text
$ cmake --build build/diagnostics-debug --parallel 4
[1/4] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[2/4] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      224048 B       992 KB     22.06%
   text	   data	    bss	    dec	    hex	filename
 275272	    852	 341184	 617308	  96b5c	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 276132 bytes, checksum 0xC2
[4/4] Firmware update instructions

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

### 19.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0x65", "configuration": "release", "elf_sha256": "9474032cceb04b0584745760a3a2d2447c99ed23132c5001c268644433948565", "image_bytes": 275660, "image_sha256": "d755890d43e586f93a06dbd8290dc39f0cf4ff5fbfe9c114eea3895daea4b666", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 222780, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275644, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x13", "configuration": "debug", "elf_sha256": "edd9b8e44d80dbade140e8100c521cc22f8fad134467a061516f29b64c53888a", "image_bytes": 275708, "image_sha256": "594ee27d3b659fe5216e80fc1c1bee5d1ba4cffbc0d34c5a70446d28031f07c9", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 222828, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275692, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xFE", "configuration": "diagnostics-release", "elf_sha256": "986fbf78efe05a42f8704b1ef31c11344f84ecea7904afc4a283b898c58db8e1", "image_bytes": 276100, "image_sha256": "e4abc22a07bb1b97e82bb88a39ed7a5b73cb7b92064a23f5bb2ade2807dd38b3", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223148, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 276084, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xC2", "configuration": "diagnostics-debug", "elf_sha256": "2d401bc11c8c756df3b198646119419f1d6c688ff53473363af0d0b5ec8a3b07", "image_bytes": 276148, "image_sha256": "94a205a6c9643e7aabc8b3421d3a844d2389881c22236f3382b4ee8b21a6189b", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223196, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 276132, "reset_vector": "0x08014E4D", "result": "PASS"}

EXIT 0
```

### final-fatfs.log — final committed revision

```text
$ python3 tests/fatfs_integrity/run.py
In file included from /workspace/lxr-desktop-app/tests/fatfs_integrity/host.c:16:
/workspace/lxr-desktop-app/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c: In function ‘FATFS_UnLinkDriverEx’:
/workspace/lxr-desktop-app/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c:80:50: warning: unused parameter ‘lun’ [-Wunused-parameter]
   80 | uint8_t FATFS_UnLinkDriverEx(char *path, uint8_t lun)
      |                                          ~~~~~~~~^~~
Configuration: configured
PASS actual sync_window: 1..4 FATs, all primary/later failures, stop ordering, dirty retention, no internal replay; non-FAT/clean windows
PASS actual sync_fs: exact FSInfo layout, write/error/pending flag, explicit retry distinction, no-update/other FAT, CTRL_SYNC and no success masking
PASS whole public sync/close through S1: mirror failure latches driver, first FR_DISK_ERR then invalid-object close, no extra physical writes
PASS full production f_sync/f_close: primary/mirror/FSInfo/data/sync errors stay sticky, modified state and handle retained, no implicit replay with healthy stub; success closes normally
PASS whole generic disk/registration/S1: successful/failed/cache status, absent then available, protected, healthy unlink/relink and all invalid drive bytes; serious SD latch survives init/remount with no physical replay
PASS actual f_mount: valid RAM FAT16 geometry, healthy repeated mount/cache status, absent then available without fabricated initialization
PASS actual f_read and read-only file close: payload unchanged, no metadata writes
In file included from /workspace/lxr-desktop-app/tests/fatfs_integrity/host.c:16:
/workspace/lxr-desktop-app/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c: In function ‘FATFS_UnLinkDriverEx’:
/workspace/lxr-desktop-app/Middlewares/Third_Party/FatFs/src/ff_gen_drv.c:80:50: warning: unused parameter ‘lun’ [-Wunused-parameter]
   80 | uint8_t FATFS_UnLinkDriverEx(char *path, uint8_t lun)
      |                                          ~~~~~~~~^~~
Configuration: readonly
PASS whole generic disk/registration/S1: successful/failed/cache status, absent then available, protected, healthy unlink/relink and all invalid drive bytes; serious SD latch survives init/remount with no physical replay
PASS actual f_mount: valid RAM FAT16 geometry, healthy repeated mount/cache status, absent then available without fabricated initialization
PASS actual f_read and read-only file close: payload unchanged, no metadata writes

EXIT 0
```

### pinned-images.log — final committed revision

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0x65", "configuration": "release", "elf_sha256": "9474032cceb04b0584745760a3a2d2447c99ed23132c5001c268644433948565", "image_bytes": 275660, "image_sha256": "d755890d43e586f93a06dbd8290dc39f0cf4ff5fbfe9c114eea3895daea4b666", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 222780, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275644, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x13", "configuration": "debug", "elf_sha256": "edd9b8e44d80dbade140e8100c521cc22f8fad134467a061516f29b64c53888a", "image_bytes": 275708, "image_sha256": "594ee27d3b659fe5216e80fc1c1bee5d1ba4cffbc0d34c5a70446d28031f07c9", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 222828, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 275692, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xFE", "configuration": "diagnostics-release", "elf_sha256": "986fbf78efe05a42f8704b1ef31c11344f84ecea7904afc4a283b898c58db8e1", "image_bytes": 276100, "image_sha256": "e4abc22a07bb1b97e82bb88a39ed7a5b73cb7b92064a23f5bb2ade2807dd38b3", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223148, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 276084, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xC2", "configuration": "diagnostics-debug", "elf_sha256": "2d401bc11c8c756df3b198646119419f1d6c688ff53473363af0d0b5ec8a3b07", "image_bytes": 276148, "image_sha256": "94a205a6c9643e7aabc8b3421d3a844d2389881c22236f3382b4ee8b21a6189b", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 223196, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 276132, "reset_vector": "0x08014E4D", "result": "PASS"}

EXIT 0
```

## Object SHA-256 comparison against S2

```text
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED 7885756e6a7c84b5a8dcf8cb870bc6571480027eb25eaa6959b3be287f0693f1
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 06db7a571ad5abd61c30d6b74509e2a0c0e15326f0dc9cce6aff7c3cd13a4682
build/debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED b3c0a151f657e4ed3438bdea9686705d729aa7e66dae1293d32f20e1af5e5a3c
build/debug/Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj: CHANGED 5986ce1ac976c851aefa68b1ca2704ccae035389831f5f4ca7875b852420fba0
build/debug/Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj: CHANGED 98bc51d3da5f741ec622601fb054d4241db42a66b7105ddb7456059087ccfa20
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED d6ee7b60ad6a2a1924e096b3631b8f38284349e3ad7c037c348e5ca3e7f17aca
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 10472838499bb9806639fc82153ba880797c3c496ac48e10e6724aeaa2e12ac5
build/release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 01463848afda54efbbeea116af34c9ff35db325406b42b003432ee46fba5c468
build/release/Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj: CHANGED 85ffa185ab89e94eeb205609d8e68ac913cc582df85ef685555f5db953d315b6
build/release/Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj: CHANGED cb9e318ee097cfbbf0e303f25a416037247f75bece2bb8f4aaac8b887cef1dee
build/diagnostics-release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 66cd4b9de166101afc4fae3970b75e726495517a6de6957ca4b7ef72863fa5d6
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED d1ed2c18ce02443fa1ed7ddae0633a61de9f31e39ba3882bb5f73378ad38dea2
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 46b4aa19065504b170ae148e4e6d0794856bd512529aa837a4d5e77479c5ab7d
build/diagnostics-release/Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj: CHANGED cb9e318ee097cfbbf0e303f25a416037247f75bece2bb8f4aaac8b887cef1dee
build/diagnostics-release/Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj: CHANGED 85ffa185ab89e94eeb205609d8e68ac913cc582df85ef685555f5db953d315b6
build/diagnostics-debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 9c55bb9c71c07e4bd35ee221dfae028522f877dbae168836196cdd3498ea07c5
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED 498f228e10b3e059fcf4a3033150897ebd12ee3005c4d4f9e8eedb78d9bc38a8
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 15922ba42e0b45f1193e5a83e721079d11b3ee0ada756c9113991b707611268c
build/diagnostics-debug/Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj: CHANGED 076e54e87787cb5f2cf8cf62fe65e9b5657bf59e880c3bb47b18bb42bb408baa
build/diagnostics-debug/Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj: CHANGED 92a2ce0c82ba5e744957202532d2b5ef37311bd6cf8e952f364a9ea57a39c7f4
```

## Complete baseline-to-final diff

Includes decisive source excerpts with paths/line context, permanent runner/fixture and documentation. No external conversation is needed to reconstruct the change.

```diff
diff --git a/Middlewares/Third_Party/FatFs/src/diskio.c b/Middlewares/Third_Party/FatFs/src/diskio.c
index 6c0881f..e698f0f 100644
--- a/Middlewares/Third_Party/FatFs/src/diskio.c
+++ b/Middlewares/Third_Party/FatFs/src/diskio.c
@@ -52,13 +52,24 @@ DSTATUS disk_initialize (
 	BYTE pdrv				/* Physical drive nmuber to identify the drive */
 )
 {
-  DSTATUS stat = RES_OK;
+  DSTATUS stat;
+
+  if(pdrv >= _VOLUMES || disk.drv[pdrv] == 0 ||
+     disk.drv[pdrv]->disk_initialize == 0 || disk.drv[pdrv]->disk_status == 0)
+    return STA_NOINIT;
 
   if(disk.is_initialized[pdrv] == 0)
-  {
-    disk.is_initialized[pdrv] = 1;
     stat = disk.drv[pdrv]->disk_initialize(disk.lun[pdrv]);
-  }
+  else
+    stat = disk.drv[pdrv]->disk_status(disk.lun[pdrv]);
+
+  if(stat & STA_NODISK) stat |= STA_NOINIT; /* No medium cannot be initialized. */
+
+  /* Cache confirmed availability only; never manufacture a successful status.
+   * A subsequent explicit call may initialize again. The SD driver's persistent
+   * fault latch remains authoritative and cannot be cleared by this wrapper.
+   */
+  disk.is_initialized[pdrv] = (stat & (STA_NOINIT | STA_NODISK)) == 0;
   return stat;
 }
 
diff --git a/Middlewares/Third_Party/FatFs/src/ff.c b/Middlewares/Third_Party/FatFs/src/ff.c
index 253de3c..5e635bc 100644
--- a/Middlewares/Third_Party/FatFs/src/ff.c
+++ b/Middlewares/Third_Party/FatFs/src/ff.c
@@ -894,13 +894,15 @@ FRESULT sync_window (	/* Returns FR_OK or FR_DISK_ERROR */
 		if (disk_write(fs->drv, fs->win, wsect, 1) != RES_OK) {
 			res = FR_DISK_ERR;
 		} else {
-			fs->wflag = 0;
 			if (wsect - fs->fatbase < fs->fsize) {		/* Is it in the FAT area? */
 				for (nf = fs->n_fats; nf >= 2; nf--) {	/* Reflect the change to all FAT copies */
 					wsect += fs->fsize;
-					disk_write(fs->drv, fs->win, wsect, 1);
+					if (disk_write(fs->drv, fs->win, wsect, 1) != RES_OK) {
+						return FR_DISK_ERR; /* Stop; retain the unverified dirty window. */
+					}
 				}
 			}
+			fs->wflag = 0; /* All required copies were acknowledged. */
 		}
 	}
 	return res;
@@ -961,7 +963,9 @@ FRESULT sync_fs (	/* FR_OK:succeeded, !=0:error */
 			st_dword(fs->win + FSI_Nxt_Free, fs->last_clst);
 			/* Write it into the FSInfo sector */
 			fs->winsect = fs->volbase + 1;
-			disk_write(fs->drv, fs->win, fs->winsect, 1);
+			if (disk_write(fs->drv, fs->win, fs->winsect, 1) != RES_OK) {
+				return FR_DISK_ERR; /* Keep the pending FSInfo update; do not sync past failure. */
+			}
 			fs->fsi_flag = 0;
 		}
 		/* Make sure that no pending write process in the physical drive */
@@ -3744,10 +3748,11 @@ FRESULT f_sync (
 
 	res = validate(&fp->obj, &fs);	/* Check validity of the file object */
 	if (res == FR_OK) {
+		if (fp->err) LEAVE_FF(fs, (FRESULT)fp->err);
 		if (fp->flag & FA_MODIFIED) {	/* Is there any change to the file? */
 #if !_FS_TINY
 			if (fp->flag & FA_DIRTY) {	/* Write-back cached data if needed */
-				if (disk_write(fs->drv, fp->buf, fp->sect, 1) != RES_OK) LEAVE_FF(fs, FR_DISK_ERR);
+				if (disk_write(fs->drv, fp->buf, fp->sect, 1) != RES_OK) ABORT(fs, FR_DISK_ERR);
 				fp->flag &= (BYTE)~FA_DIRTY;
 			}
 #endif
@@ -3774,7 +3779,7 @@ FRESULT f_sync (
 						res = store_xdir(&dj);	/* Restore it to the directory */
 						if (res == FR_OK) {
 							res = sync_fs(fs);
-							fp->flag &= (BYTE)~FA_MODIFIED;
+							if (res == FR_OK) fp->flag &= (BYTE)~FA_MODIFIED;
 						}
 					}
 					FREE_NAMBUF();
@@ -3792,10 +3797,12 @@ FRESULT f_sync (
 					st_word(dir + DIR_LstAccDate, 0);
 					fs->wflag = 1;
 					res = sync_fs(fs);					/* Restore it to the directory */
-					fp->flag &= (BYTE)~FA_MODIFIED;
+					if (res == FR_OK) fp->flag &= (BYTE)~FA_MODIFIED;
 				}
 			}
 		}
+		/* A later close/sync must not hide this failure or replay uncertain writes. */
+		if (res == FR_DISK_ERR) fp->err = (BYTE)res;
 	}
 
 	LEAVE_FF(fs, res);
diff --git a/tests/fatfs_integrity/README.md b/tests/fatfs_integrity/README.md
new file mode 100644
index 0000000..50ab5f3
--- /dev/null
+++ b/tests/fatfs_integrity/README.md
@@ -0,0 +1,48 @@
+# S3 FatFs integrity and disk-initialization tests
+
+Run `python3 tests/fatfs_integrity/run.py` from the repository root. Native
+ASan/UBSan tests include complete production `ff.c`, `diskio.c`, `ff_gen_drv.c`
+and `sd_diskio.c` in one translation unit. Helpers and public `f_sync`, `f_close`,
+`f_read` execute their actual implementation. This is not a rewritten algorithm.
+Fixed-width target integer typedefs replace only integer.h's LP64-incompatible
+host widths. A main.h stub and the existing S1 CMSIS/HAL shim supply unused or
+simulated hardware declarations. Real FatFs configuration, structures and macros
+are used, with a temporary read-only configuration variant (FS_READONLY=1,
+FS_LOCK=0, USE_MKFS=0). No production configuration is modified.
+
+RAM-sector callbacks record exact write order/content and inject failures. The
+metadata stub intentionally does **not** latch failures: it proves first-call
+error propagation and the file's persistent error on subsequent sync/close,
+independent of the production SD latch. An explicit separate sync_fs call can
+retry pending FSInfo with this stub; that is not automatic retry inside the
+function. In production, serious errors additionally latch the S1 driver, which
+rejects subsequent physical I/O. Tests wire that actual driver through actual
+registration/disk initialization to prove remount cannot clear the latch. The
+public mirror-failure path is also wired through that driver: f_sync returns
+FR_DISK_ERR, and later f_close returns FR_INVALID_OBJECT because S1 marks the
+volume unavailable. It retains the handle and issues no extra physical write.
+
+Coverage: primary/secondary/later FAT failures and success for 1–4 synthetic FAT
+copies; dirty retention; FSInfo bytes, pending flag, no-update and sync errors;
+public sync/close persistent errors without replay; normal file close/read;
+failed/absent/successful/protected initialization, cached status and healthy
+unlink/relink; all invalid drive-byte values; SD latch surviving repeated init
+and unlink/relink. Three/four FAT fixtures test the internal generic loop, not
+acceptance of such volumes by the unchanged mount parser (which accepts 1/2).
+No physical filesystem is mounted or formatted. Actual f_mount decodes a valid
+RAM FAT16 boot sector for healthy/absent/remount tests; constructed valid
+in-memory file objects exercise synchronization without a physical card.
+
+The runner uses a 20-second execution deadline per configuration, ASan/UBSan fatal
+errors, and disables LeakSanitizer because of executor tracing limitations.
+Unused static helper warnings are disabled because the whole vendor module is
+included. The existing unused `lun` parameter warning in ff_gen_drv.c remains
+visible. No existing tests are weakened. This does not prove electrical timing,
+card recovery, metadata atomicity, power-loss durability or audio behavior.
+
+After a metadata failure, fs->wflag or fs->fsi_flag preserves pending work as
+appropriate. Some earlier sectors may already have succeeded. No rollback or
+new retry is attempted. After a public disk synchronization failure, fp->err
+retains FR_DISK_ERR and FA_MODIFIED is not cleared; later sync/close returns an
+error and retains the handle/lock. S4 must handle save errors and failed-handle
+ownership deliberately. This test does not implement that recovery policy.
diff --git a/tests/fatfs_integrity/host.c b/tests/fatfs_integrity/host.c
new file mode 100644
index 0000000..2107e6a
--- /dev/null
+++ b/tests/fatfs_integrity/host.c
@@ -0,0 +1,215 @@
+/* Whole production FatFs/generic disk/registration/S1 driver; RAM media only. */
+#include <stdint.h>
+#include <assert.h>
+#include <stdio.h>
+#include <string.h>
+/* Match target widths on LP64 hosts without changing production integer.h. */
+#define _FF_INTEGER
+typedef int32_t INT,LONG;
+typedef uint32_t UINT,DWORD;
+typedef uint8_t BYTE;
+typedef int16_t SHORT;
+typedef uint16_t WORD,WCHAR;
+typedef uint64_t QWORD;
+#include "Middlewares/Third_Party/FatFs/src/ff.c"
+#include "Middlewares/Third_Party/FatFs/src/diskio.c"
+#include "Middlewares/Third_Party/FatFs/src/ff_gen_drv.c"
+#include "FATFS/Target/sd_diskio.c"
+_Static_assert(sizeof(DWORD)==4 && sizeof(WORD)==2,"target integer widths");
+SysTick_Type host_systick={3};
+uint32_t host_ipsr,host_primask,host_basepri,host_faultmask;
+static uint32_t tick,physical_writes,physical_inits,physical_fail_write;
+static uint8_t card_present=SD_PRESENT,bsp_result=MSD_OK;
+uint32_t HAL_GetTick(void){return tick++;}
+uint8_t BSP_SD_IsDetected(void){return card_present;}
+uint8_t BSP_SD_Init(void){physical_inits++;return bsp_result;}
+uint8_t BSP_SD_GetCardState(void){return MSD_OK;}
+void BSP_SD_GetCardInfo(BSP_SD_CardInfo *i){i->LogBlockNbr=256;i->LogBlockSize=512;}
+uint8_t BSP_SD_ReadBlocks(uint32_t *p,uint32_t s,uint32_t n,uint32_t timeout)
+{(void)p;(void)s;(void)n;(void)timeout;return bsp_result;}
+uint8_t BSP_SD_WriteBlocks(uint32_t *p,uint32_t s,uint32_t n,uint32_t timeout)
+{(void)p;(void)s;(void)n;(void)timeout;physical_writes++;return physical_writes==physical_fail_write?MSD_ERROR:bsp_result;}
+static BYTE media[256][512];
+static DWORD sectors[16];
+static unsigned writes,reads,syncs,fail_write,init_calls,status_calls;
+static DRESULT sync_result;
+static DSTATUS available;
+static DSTATUS fake_init(BYTE lun)
+{assert(lun==0);assert(!disk.is_initialized[0]);init_calls++;return available;}
+static DSTATUS fake_status(BYTE lun){assert(lun==0);status_calls++;return available;}
+static DRESULT fake_read(BYTE lun,BYTE *p,DWORD s,UINT n)
+{assert(lun==0 && s+n<=256);reads++;memcpy(p,media[s],n*512);return RES_OK;}
+static DRESULT fake_write(BYTE lun,const BYTE *p,DWORD s,UINT n)
+{
+ assert(lun==0 && n==1 && s<256 && writes<16);sectors[writes++]=s;
+ if(writes==fail_write)return RES_ERROR; /* This mock deliberately has no S1 latch. */
+ memcpy(media[s],p,512);return RES_OK;
+}
+static DRESULT fake_ioctl(BYTE lun,BYTE cmd,void *p)
+{assert(lun==0);(void)p;assert(cmd==CTRL_SYNC);syncs++;return sync_result;}
+static const Diskio_drvTypeDef fake_driver={fake_init,fake_status,fake_read,fake_write,fake_ioctl};
+static FATFS fs;
+static FIL file;
+static char drive[4];
+static void setup(void)
+{
+ memset(&disk,0,sizeof disk);memset(&fs,0,sizeof fs);memset(&file,0,sizeof file);
+ memset(media,0,sizeof media);memset(sectors,0,sizeof sectors);
+ writes=reads=syncs=fail_write=init_calls=status_calls=0;available=0;sync_result=RES_OK;
+ assert(FATFS_LinkDriver(&fake_driver,drive)==0 && !disk.is_initialized[0]);
+ assert(disk_initialize(0)==0 && disk.is_initialized[0] && init_calls==1);
+ fs.fs_type=FS_FAT16;fs.id=7;fs.drv=0;fs.csize=1;fs.n_fats=2;
+ fs.fatbase=8;fs.fsize=16;fs.n_fatent=1024;fs.database=128;fs.volbase=64;
+ fs.winsect=100;
+#if !_FS_READONLY
+ fs.free_clst=123;fs.last_clst=42;
+#endif
+ file.obj.fs=&fs;file.obj.id=7;file.obj.sclust=2;file.obj.objsize=512;
+ file.flag=FA_READ;
+#if !_FS_READONLY
+ file.dir_sect=100;file.dir_ptr=fs.win;
+#endif
+#if _FS_LOCK
+ memset(Files,0,sizeof Files);Files[0].fs=&fs;Files[0].ctr=1;file.obj.lockid=1;
+#endif
+}
+#if !_FS_READONLY
+static void mirrors(void)
+{
+ for(unsigned copies=1;copies<=4;copies++) {
+  for(unsigned failure=0;failure<=copies;failure++) {
+   setup();fs.n_fats=copies;fs.winsect=9;fs.wflag=1;memset(fs.win,0xa5,512);fail_write=failure;
+   assert(sync_window(&fs)==(failure?FR_DISK_ERR:FR_OK));
+   assert(writes==(failure?failure:copies));assert(fs.wflag==(failure?1:0));
+   assert(fs.winsect==9 && fs.win[0]==0xa5);
+   for(unsigned n=0;n<writes;n++)assert(sectors[n]==9+n*16);
+   if(!failure)for(unsigned n=0;n<copies;n++)assert(!memcmp(media[9+n*16],fs.win,512));
+   assert(!syncs); /* No automatic retry or later copy after failure. */
+  }
+ }
+ setup();fs.winsect=100;fs.wflag=1;assert(sync_window(&fs)==FR_OK && writes==1 && !fs.wflag);
+ setup();assert(sync_window(&fs)==FR_OK && writes==0);
+ puts("PASS actual sync_window: 1..4 FATs, all primary/later failures, stop ordering, dirty retention, no internal replay; non-FAT/clean windows");
+}
+static void fsinfo(void)
+{
+ setup();fs.fs_type=FS_FAT32;fs.fsi_flag=1;assert(sync_fs(&fs)==FR_OK);
+ assert(writes==1 && sectors[0]==65 && !fs.fsi_flag && syncs==1);
+ assert(ld_word(media[65]+BS_55AA)==0xaa55 && ld_dword(media[65]+FSI_LeadSig)==0x41615252);
+ assert(ld_dword(media[65]+FSI_StrucSig)==0x61417272 && ld_dword(media[65]+FSI_Free_Count)==123 && ld_dword(media[65]+FSI_Nxt_Free)==42);
+ setup();fs.fs_type=FS_FAT32;fs.fsi_flag=1;fail_write=1;
+ assert(sync_fs(&fs)==FR_DISK_ERR && fs.fsi_flag==1 && writes==1 && syncs==0 && fs.winsect==65);
+ /* A separate explicit retry in this non-latching stub is distinct from auto replay. */
+ fail_write=0;assert(sync_fs(&fs)==FR_OK && writes==2 && !fs.fsi_flag && syncs==1);
+ for(unsigned flag=0;flag<=128;flag+=128){setup();fs.fs_type=FS_FAT32;fs.fsi_flag=flag;assert(sync_fs(&fs)==FR_OK && !writes && syncs==1 && fs.fsi_flag==flag);}
+ setup();fs.fsi_flag=1;assert(sync_fs(&fs)==FR_OK && !writes && syncs==1 && fs.fsi_flag==1);
+ setup();sync_result=RES_ERROR;assert(sync_fs(&fs)==FR_DISK_ERR && syncs==1);
+ setup();fs.wflag=1;fs.winsect=9;fail_write=2;fs.fs_type=FS_FAT32;fs.fsi_flag=1;
+ assert(sync_fs(&fs)==FR_DISK_ERR && writes==2 && fs.wflag && fs.fsi_flag && !syncs);
+ puts("PASS actual sync_fs: exact FSInfo layout, write/error/pending flag, explicit retry distinction, no-update/other FAT, CTRL_SYNC and no success masking");
+}
+static void public_sync(void)
+{
+ for(unsigned kind=0;kind<5;kind++) {
+  setup();file.flag=FA_WRITE|FA_MODIFIED;
+  switch(kind){
+  case 0:fs.winsect=9;fs.wflag=1;fail_write=1;break;
+  case 1:fs.winsect=9;fs.wflag=1;fail_write=2;break;
+  case 2:fs.fs_type=FS_FAT32;fs.fsi_flag=1;fail_write=2;break;
+  case 3:sync_result=RES_ERROR;break;
+  default:file.flag|=FA_DIRTY;file.sect=150;fail_write=1;break;
+  }
+  assert(f_sync(&file)==FR_DISK_ERR && file.err==FR_DISK_ERR && (file.flag&FA_MODIFIED));
+  fail_write=0;sync_result=RES_OK;
+  FATFS other=fs;other.wflag=0;other.fsi_flag=0;
+  assert(sync_fs(&other)==FR_OK); /* Unrelated success cannot erase the file error. */
+  unsigned before=writes,oldsync=syncs;
+  assert(f_sync(&file)==FR_DISK_ERR && f_close(&file)==FR_DISK_ERR);
+  assert(writes==before && syncs==oldsync && file.obj.fs==&fs);
+#if _FS_LOCK
+  assert(Files[0].fs==&fs && Files[0].ctr==1);
+#endif
+ }
+ setup();file.flag=FA_WRITE|FA_MODIFIED;fs.fs_type=FS_FAT32;fs.fsi_flag=1;
+ assert(f_sync(&file)==FR_OK && !(file.flag&FA_MODIFIED) && !file.err);
+ assert(f_close(&file)==FR_OK && file.obj.fs==0);
+ setup();file.flag=FA_WRITE|FA_MODIFIED;fs.fs_type=FS_FAT32;fs.fsi_flag=1;fail_write=2;
+ assert(f_close(&file)==FR_DISK_ERR && file.err==FR_DISK_ERR && file.obj.fs==&fs);
+ /* Same public failure path through the real S1 disk driver, not the RAM stub. */
+ setup();disk.drv[0]=&SD_Driver;disk.is_initialized[0]=0;
+ Stat=STA_NOINIT;Fault=0;physical_writes=physical_inits=0;physical_fail_write=2;
+ card_present=SD_PRESENT;bsp_result=MSD_OK;assert(disk_initialize(0)==0);
+ file.flag=FA_WRITE|FA_MODIFIED;fs.winsect=9;fs.wflag=1;
+ assert(f_sync(&file)==FR_DISK_ERR && Fault && file.err==FR_DISK_ERR && physical_writes==2);
+ assert(f_close(&file)==FR_INVALID_OBJECT && file.obj.fs==&fs && physical_writes==2);
+ physical_fail_write=0;
+ puts("PASS whole public sync/close through S1: mirror failure latches driver, first FR_DISK_ERR then invalid-object close, no extra physical writes");
+ puts("PASS full production f_sync/f_close: primary/mirror/FSInfo/data/sync errors stay sticky, modified state and handle retained, no implicit replay with healthy stub; success closes normally");
+}
+#endif
+static void initialization(void)
+{
+ setup();assert(disk_initialize(0)==0 && init_calls==1 && status_calls==1);
+ available=STA_NOINIT;assert(disk_initialize(0)&STA_NOINIT);assert(!disk.is_initialized[0] && init_calls==1);
+ assert(disk_initialize(0)&STA_NOINIT);assert(init_calls==2 && !disk.is_initialized[0]);
+ available=STA_NOINIT|STA_NODISK;assert(disk_initialize(0)==available && !disk.is_initialized[0]);
+ available=STA_NODISK;assert(disk_initialize(0)==(STA_NOINIT|STA_NODISK) && !disk.is_initialized[0]);
+ available=0;assert(disk_initialize(0)==0 && disk.is_initialized[0]);
+ available=STA_PROTECT;assert(disk_initialize(0)==STA_PROTECT && disk.is_initialized[0]);
+ available=0;unsigned i=init_calls;assert(FATFS_UnLinkDriver(drive)==0);
+ assert(disk_initialize(0)&STA_NOINIT);assert(FATFS_LinkDriver(&fake_driver,drive)==0);
+ assert(!disk.is_initialized[0] && disk_initialize(0)==0 && init_calls==i+1);
+ for(unsigned n=1;n<256;n++)assert(disk_initialize((BYTE)n)&STA_NOINIT);
+ /* Actual S1 driver behind the actual generic wrapper and registration code. */
+ memset(&disk,0,sizeof disk);Stat=STA_NOINIT;Fault=0;physical_writes=physical_inits=tick=0;
+ card_present=SD_NOT_PRESENT;bsp_result=MSD_OK;physical_fail_write=0;assert(FATFS_LinkDriver(&SD_Driver,drive)==0);
+ assert(disk_initialize(0)==(STA_NOINIT|STA_NODISK) && !physical_inits && !disk.is_initialized[0]);
+ card_present=SD_PRESENT;assert(disk_initialize(0)==0 && physical_inits==1 && disk.is_initialized[0]);
+ BYTE buffer[512]={0};bsp_result=MSD_ERROR;
+ assert(disk_write(0,buffer,0,1)==RES_ERROR && physical_writes==1 && Fault);
+ assert(disk_initialize(0)&STA_NOINIT);assert(!disk.is_initialized[0]);
+ for(unsigned n=0;n<10;n++)assert(disk_initialize(0)&STA_NOINIT);
+ assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&SD_Driver,drive)==0);
+ assert(disk_initialize(0)&STA_NOINIT);
+ assert(disk_write(0,buffer,0,1)==RES_NOTRDY && physical_writes==1 && physical_inits==1);
+ puts("PASS whole generic disk/registration/S1: successful/failed/cache status, absent then available, protected, healthy unlink/relink and all invalid drive bytes; serious SD latch survives init/remount with no physical replay");
+}
+static void fixture_put16(BYTE *p,WORD value){p[0]=(BYTE)value;p[1]=(BYTE)(value>>8);}
+static void mount_lifecycle(void)
+{
+ setup();FATFS mounted;memset(&mounted,0,sizeof mounted);
+ /* Minimal RAM FAT16 boot sector; no format/create operation is invoked. */
+ BYTE *boot=media[0];boot[0]=0xeb;boot[1]=0x3c;boot[2]=0x90;
+ memcpy(boot+BS_FilSysType,"FAT16   ",8);
+ fixture_put16(boot+BPB_BytsPerSec,512);boot[BPB_SecPerClus]=1;
+ fixture_put16(boot+BPB_RsvdSecCnt,1);boot[BPB_NumFATs]=2;
+ fixture_put16(boot+BPB_RootEntCnt,512);fixture_put16(boot+BPB_TotSec16,8192);
+ fixture_put16(boot+BPB_FATSz16,32);boot[BPB_Media]=0xf8;fixture_put16(boot+BS_55AA,0xaa55);
+ assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&fake_driver,drive)==0);
+ unsigned before=init_calls;
+ assert(f_mount(&mounted,drive,1)==FR_OK && init_calls==before+1);
+ assert(mounted.fs_type==FS_FAT16 && mounted.n_fats==2 && mounted.fatbase==1 && mounted.database==97);
+ assert(f_mount(0,drive,0)==FR_OK && f_mount(&mounted,drive,1)==FR_OK && init_calls==before+1);
+ assert(f_mount(0,drive,0)==FR_OK);
+ assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&fake_driver,drive)==0);
+ available=STA_NOINIT|STA_NODISK;
+ assert(f_mount(&mounted,drive,1)==FR_NOT_READY && !disk.is_initialized[0]);
+ available=0;assert(f_mount(&mounted,drive,1)==FR_OK && disk.is_initialized[0]);
+ assert(f_mount(0,drive,0)==FR_OK);
+ puts("PASS actual f_mount: valid RAM FAT16 geometry, healthy repeated mount/cache status, absent then available without fabricated initialization");
+}
+static void read_only_operation(void)
+{
+ setup();BYTE data[512];UINT count;
+ for(unsigned n=0;n<512;n++)media[128][n]=(BYTE)n;
+ assert(f_read(&file,data,512,&count)==FR_OK && count==512 && !memcmp(data,media[128],512));
+ assert(reads==1 && !writes && f_close(&file)==FR_OK && !writes);
+ puts("PASS actual f_read and read-only file close: payload unchanged, no metadata writes");
+}
+int main(void)
+{
+#if !_FS_READONLY
+ mirrors();fsinfo();public_sync();
+#endif
+ initialization();mount_lifecycle();read_only_operation();
+}
diff --git a/tests/fatfs_integrity/main.h b/tests/fatfs_integrity/main.h
new file mode 100644
index 0000000..e7cb884
--- /dev/null
+++ b/tests/fatfs_integrity/main.h
@@ -0,0 +1,4 @@
+#ifndef FATFS_HOST_MAIN_H
+#define FATFS_HOST_MAIN_H
+/* ffconf.h includes firmware main.h; its hardware declarations are unused here. */
+#endif
diff --git a/tests/fatfs_integrity/run.py b/tests/fatfs_integrity/run.py
new file mode 100644
index 0000000..bcf94c8
--- /dev/null
+++ b/tests/fatfs_integrity/run.py
@@ -0,0 +1,26 @@
+#!/usr/bin/env python3
+"""Whole production FatFs/disk glue/S1 driver tests with RAM media and BSP mocks."""
+from pathlib import Path
+import os,re,subprocess,tempfile
+ROOT=Path(__file__).resolve().parents[2]
+HERE=Path(__file__).resolve().parent
+with tempfile.TemporaryDirectory(prefix='lxr-fatfs-integrity-') as directory:
+    directory=Path(directory)
+    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
+    for name,readonly in [('configured',False),('readonly',True)]:
+        config=directory/name;config.mkdir()
+        text=(ROOT/'FATFS/Target/ffconf.h').read_text()
+        if readonly:
+            for option in ['_FS_READONLY','_FS_LOCK','_USE_MKFS']:
+                text=re.sub(r'(#define\s+'+option+r'\s+)\d+',lambda m:m[1]+('1' if option=='_FS_READONLY' else '0'),text)
+        (config/'ffconf.h').write_text(text)
+        binary=config/'host'
+        command=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra',
+                 '-Wno-unused-function','-fsanitize=address,undefined','-fno-sanitize-recover=all',
+                 '-fno-omit-frame-pointer','-fno-pie','-no-pie','-ffunction-sections','-fdata-sections',
+                 '-Wl,--gc-sections','-I'+str(HERE),'-I'+str(config),'-I'+str(ROOT),
+                 '-I'+str(ROOT/'Middlewares/Third_Party/FatFs/src'),
+                 '-I'+str(ROOT/'tests/sd_storage'),'-I'+str(ROOT/'FATFS/Target'),str(HERE/'host.c'),'-o',str(binary)]
+        subprocess.run(command,check=True,cwd=ROOT,env=env)
+        print('Configuration:',name,flush=True)
+        subprocess.run([str(binary)],check=True,cwd=ROOT,env=env,timeout=20)
```

## Publication verification

Source parent/tree/blob identities are checked against validated local bytes. Report contents, exact path, single-file change and lab-setup parent are verified by GitHub read-back. All pre-existing remote branch heads are compared with the pre-task snapshot. The final response provides the report URL and commit. Generated binaries, build directories and temporary fixtures/logs are not committed.
