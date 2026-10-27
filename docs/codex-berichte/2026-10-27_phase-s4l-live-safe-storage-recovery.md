# LXR Firmware Lab — Phase S4-L: Live-Safe Storage Error Policy

Date: 2026-10-27 UTC. Repository: `DocLXR-02/lxr-desktop-app`.

**Status: IMPLEMENTED within the bounded S4-L scope; published for review. NOT FLASHED — NOT HARDWARE VALIDATED.** This is a correction to retry classification, error feedback and RAM dirty-state handling, not certification of complete storage integrity, live audio continuity, safe deployment or the pending S4b lifecycle.

## 1. Exact revision, instructions and publication identity

- Source baseline: `safety/sd-save-errors-s4a` at `c87666ff945e2e01cbbf1ceda72fb503638de579`.
- New implementation branch: `safety/sd-live-error-policy-s4l`.
- Final implementation commit: `feaff649599472036390ecef24f7193b7cc2e2ab`.
- Final source tree: `0a5549eadfa98377abbc58d04629c2045641da0f`; its sole parent is the initial S4-L commit `ea503434d001f1b793f9cb6fe95a69ece3972796`, whose sole parent is the exact S4a baseline. The final follow-up corrects notification expiry/copy interleaving. No merge.
- Source URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/safety/sd-live-error-policy-s4l
- Report-only branch: `codex-reports/2026-10-27-phase-s4l-live-safe-storage-recovery`, based on `lab-setup` at `5829398cea5d337200393757e39a37d4d8148256`.
- Report path: `docs/codex-berichte/2026-10-27_phase-s4l-live-safe-storage-recovery.md`.
- Report URL: https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-27-phase-s4l-live-safe-storage-recovery/docs/codex-berichte/2026-10-27_phase-s4l-live-safe-storage-recovery.md

The existing `/workspace/lxr-desktop-app` checkout was clean on the expected baseline. Remote head and local commit matched; `git merge-base --is-ancestor 5829398cea5d337200393757e39a37d4d8148256 c87666ff945e2e01cbbf1ceda72fb503638de579` succeeded. A new branch was created from that exact commit. No extra checkout/worktree was created.

The complete `AGENTS.md` from `lab-setup` was read (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`). Prior S1/S2/S3/S4a report files were fetched and inspected, including source contracts, limitations and tests. Their report branches remain unchanged:

| Reference | Branch | Report blob |
|---|---|---|
| S1 | `codex-reports/2026-10-26-phase-s1-sd-storage-foundation` | `e1c646c3e588df061ed4df183e87e5822aaa9f79` |
| S2 | `codex-reports/2026-10-26-phase-s2-sd-hal-bsp-timeouts` | `9b44e0ef9bc1c8585e04fbe05249ab5fc691b81e` |
| S3 | `codex-reports/2026-10-26-phase-s3-fatfs-data-integrity` | `88ab6565c42881762d47f9be819f9c50ae5e3e76` |
| S4a final verified report | `codex-reports/2026-10-26-phase-s4a-safe-file-saving` | `15916ed38af3af63d65538ea8933d91de67a05dd` |

S4a's final report filename is `2026-10-26_phase-s4a-safe-file-saving-verified.md`; the earlier report is also preserved. Source lineage includes Bug001, Euclidean, A/B/C, S1/S2/S3/S4a. No older implementation branch was reset, modified, merged or force-pushed. The initial remote-ref inventory contains27 pre-existing branches. Publication verification checks those heads again rather than assuming preservation from a clean local status.

Evidence labels: **CODE** = inspected production source; **BUILD** = executed host/compiler/linker/image checks; **HYPOTHESIS** = plausible but unmeasured physical consequence; **OPEN** = follow-up design or hardware measurement required.

## 2. Objective and bounded decisions

The purpose is to keep the instrument usable in RAM and permit repeated safe, user-initiated save attempts after harmless logical errors. It must not convert a failed/partial save into success or relax protection after uncertain physical writes.

Implemented priorities:

1. Classify **failed opens separately from writes/closes**, using the actual configured FatFs paths and ownership state.
2. Retain S1/S2 hardware/error latch protection and S3/S4a truthful write/count/sync/close behavior.
3. Replace blocking SD-error feedback with a bounded, timed overlay in the existing display architecture; no new input mapping or parameter enumeration.
4. Preserve pending temporary-save dirty flags after failure, pause automatic attempts, and retain manual saves after logical errors.
5. Check one directly affected project caller that previously depended on blanket quarantine for containment. Do not change its destructive directory sequence.

Not implemented: asynchronous storage, automatic retries, automatic reinitialization/remount/recovery, file rollback or transactions, independent read sessions after a failed shared file, project-folder replacement, diagnostic capture/export, or the separate one-second LOADED display enhancement. No hardware-dependent cleanup is guessed.

## 3. Inventory of baseline error and lockout behavior

### Disk/HAL/BSP layer: intentionally retained

**CODE, `FATFS/Target/sd_diskio.c:41–109`:** `Fault` is a file-static byte cleared only by C-runtime initialization at reboot. Serious BSP initialization/read/write failure, card-state command error, readiness timeout or observed removal during an initialized access sets Fault and STA_NOINIT. Further read/write/status/init/ioctl cannot clear it or initiate fresh BSP transfers. Unlink/relink/remount cannot bypass it. Invalid arguments and forbidden ISR/masked/tick-disabled contexts reject without poisoning the media latch. Startup absence before successful initialization is non-destructive and can be followed by later initialization when a card becomes available.

S1 uses5000ms transfer and5000ms readiness allowances,128-sector maximum, wrap-safe HAL tick arithmetic and enabled-interrupt foreground ownership. S2 bounded active command-response waits and voltage negotiation, distinguishes BSP timeout/busy/status errors, retains failed HAL transfer state/context and forbids direct transfer replay after uncertainty. S2 does not establish physical card/data-path quiescence. No abort, reset or recovery write was introduced in these phases or S4-L.

**Decision:** retain this latch. A card can still be programming or a transaction unresolved when software returns. Opening another file or merely seeing a later ready status is insufficient evidence to clear it. Restricting SD reads as well as writes remains defensible because they also issue commands to a possibly active peripheral. This restricts storage, not RAM synthesis, MIDI input or the sequencer by policy. Whether they continue reliably under physical failure load remains unmeasured.

### Filesystem layer: intentionally retained

**CODE, `Middlewares/Third_Party/FatFs/src/ff.c`:** S3 checks every required FAT mirror write in `sync_window`, checks FSInfo write/pending flags in `sync_fs`, keeps file errors sticky in `f_sync` and only completes `f_close`/invalidates ownership after successful synchronization/validation. Generic disk initialization is not cached as success prematurely. `_FS_LOCK=1`, `_USE_LFN=0`, `_FS_EXFAT=0` in `FATFS/Target/ffconf.h` are material to this task.

These changes prevent false success but do not make metadata updates transactional. A partial primary/mirror write, buffered file data, dirty directory entry or allocated chain can remain after failure. S4-L does not clear FIL.err or force-release filesystem locks.

### Application layer: refined

**CODE, S4a baseline `presetManager.c:87–126`:** every failed write-mode open latched `preset_sharedFailure`, even `FR_NO_PATH`. Any write error/short count or failed close also latched; guarded later opens/closes/filename scans/project entry points rejected. Only reboot cleared the application latch. This contained uncertainty, but a missing path before modification unnecessarily closed otherwise healthy storage for the session.

The shared global `SDFile` is defined in `FATFS/App/fatfs.c:24`. All enabled `presetManager.c` opens/closes remain wrapped. An unresolved owned `FIL` still cannot be overwritten; active ownership returns `FR_LOCKED` before `f_open`. Serious app errors still block guarded directory/lifecycle entry points too. Independent read-only access would require a reviewed shared-volume/file-session design, not merely a new FIL.

**CODE, S4a baseline `preset_processTimer`:** background temporary saves cleared dirty flags before knowing save success and cleared both flags after failure, even though unsaved RAM remained. It suppressed retries after blanket quarantine, but did not retain truthful pending state.

### Feedback and callers: refined without changing sound/UI mapping

**CODE, baseline `menu_showSdError`:** started/stopped TIM2, disabled display processing, waited in `while(sendDisplayBuffer()!=0)`, wrote LCD synchronously and called `HAL_Delay(2000)`. Every foreground caller inherited that delay. This was additional blocking after storage itself had returned.

Call sites are startup/mount and foreground card insertion in `main.c`, individual/global/project menu actions, pattern-button save in `buttonHandler.c`, default-global creation and background save service in `presetManager.c`. No new notification caller is in the audio/DMA ISR. Existing UI callers already gate individual save success after S4a; their result checking remains intact.

Full-project save/load and formatting flows contain separate waits, unchecked results and deletion risks described below. Replacing this one error-notification function does not make every existing UI/storage operation non-blocking.

## 4. Severity, retry and recovery policy

The table describes **stage + result + ownership**, not a universal classification of an integer error code.

| Event | Meaning and application action | Explicit retry/recovery |
|---|---|---|
| Required writes return FR_OK with full counts; close/sync returns FR_OK | Save completed according to the applicable FatFs contract; existing success UI allowed | Normal operation; not absolute physical/power-loss persistence proof |
| Failed open: NO_FILE, NO_PATH | Missing target/path before create/truncate; no application latch if owner null | Correct path/context; user can try again, including nine failures then success |
| Failed open: INVALID_NAME, INVALID_DRIVE, INVALID_PARAMETER | Invalid logical input before file creation/truncation | Correct input; no permanent application lockout |
| Failed open: EXIST | Exclusive-create conflict before replacement | Choose correct target/name; do not delete automatically |
| Failed open: LOCKED, TOO_MANY_OPEN_FILES | Open rejected; existing file-lock ownership must be respected | User may try when legitimately available; do not force-release an unresolved owner |
| Failed open: WRITE_PROTECTED | Logical write-access rejection; no reason to invent media corruption | Check media/protection outside this task; no automatic modification |
| Failed open: NOT_ENABLED, NO_FILESYSTEM | Mount/volume checks required | No application latch from this result alone; do not format automatically |
| Failed open: NOT_READY | Unavailable/uninitialized volume; can reflect initial absence or S1's serious fault | Safe repeated API requests do not bypass S1. If S1 is latched, no physical retries are issued; reboot/check may still be necessary |
| Failed open: DENIED, configured SFN/non-exFAT path only | Read-only/directory target, full SFN table or no free directory cluster; no uncertain transfer inferred from logical denial | Check target/space; retry explicitly after correction. Guarded by `_USE_LFN==0 && !_FS_EXFAT` |
| Failed open leaves non-null owner, or any unclassified/serious error (DISK_ERR, INT_ERR, TIMEOUT, etc.) | Cannot justify safe reuse from public result; retain first app failure | Additional checks/recovery design required; app state remains reboot-required in this task |
| Any payload write error, even full reported byte count | Required operation failed; outcome not inferred complete | Stop; do not automatically replay or close/flush; retain shared-file quarantine |
| FR_OK but short write, including disk full after open | Incomplete file, possibly dirty FAT/directory/buffer and old file already truncated | FR_DISK_ERR, no success; retain owner/first error. Full-file retry/disposal requires S4b-style safe replacement/ownership design |
| Sync/close failure | Metadata/data completion not established; S3 may retain sticky error and lock | Failure returned, no force-close/reuse/retry; reboot/check remains necessary |
| SD communication/status timeout/failure | Physical state/transaction uncertain | S1/S2 fault policy unchanged; no automatic abort/reset/remount/write replay |
| Error notification expiry | Only removes overlay | Never clears a disk/application latch; never requires reboot to dismiss |

The whitelist is intentionally confined to failed `f_open`. Payload `FR_DENIED` still quarantines; the same code at a different stage has different implications. In this synchronous configuration `FR_TIMEOUT` is not evidence of a harmless reentrant lock timeout; `_FS_REENTRANT=0`, and unknown cases remain conservative.

### Why configured open DENIED can be treated separately

**CODE:** `ff.c:3330–3500` resolves/mounts/follows the path, checks attribute/lock/existence errors, then enters creation/truncation only after successful checks. Non-LFN `dir_register` at2370 requests a single SFN entry through `dir_alloc(dp,1)`. `dir_alloc` maps table exhaustion to DENIED; `dir_next` at1523–1584 reports no-free-cluster as DENIED before directory growth completes, while actual synchronization/command errors propagate DISK_ERR/INT_ERR. Successful `dir_register` is required before truncate/create processing. Attribute rejection also precedes truncate. With one requested entry there is no multi-entry LFN reservation to partially assemble before a later logical denial.

The code uses a compile-time guard rather than silently generalizing this claim to LFN/exFAT. Tests use the actual configured FatFs for read-only target and full static SFN-root rejection. Dynamic-directory full-chain paths were source-reviewed; not every possible on-disk geometry is exhaustively executed. Filesystem corruption producing unexpected internal errors still enters the conservative default, not this logical whitelist.

No API success is claimed to undo prior writes. A failed open can flush previously dirty filesystem windows while traversing; a newly observed disk error is not in the whitelist. “No modification” reasoning refers to the rejected requested file create/truncate, not a universal promise that every cached filesystem read operation never writes.

## 5. Exact implementation and source references

Only three production files changed:

1. `Core/Src/Preset/presetManager.c` — failed-open result classification at106; readonly getters at93/97; autosave pause and preserved dirty flags at629; direct project global-save result check at2335.
2. `Core/Inc/Preset/PresetManager.h:121` — declarations for `preset_storageError()` and `preset_autosaveIsPaused()`. No new recovery/clear API. Added at the end to preserve existing declaration line positions/debug metadata.
3. `Core/Src/menu/menu.c` — fixed notification state at674–677; expiry in existing `menu_tick:992`; overlay in `menu_repaint:1819`; bounded render/notification functions at1937–1978.

Seven test/documentation files change or are added; full ten-file list, blob hashes and complete diff are appended. No SD disk/HAL/BSP/FatFs-core, sound/sequence/MIDI/USB, interrupt source, build configuration, startup/linker/clock/bootloader file changes.

Decisive failed-open body:

```c
res = f_open(file, name, mode);
if(res != FR_OK) {
    switch(res) {
    case FR_NO_FILE: case FR_NO_PATH: case FR_INVALID_NAME: case FR_EXIST:
    case FR_INVALID_DRIVE: case FR_NOT_ENABLED: case FR_NO_FILESYSTEM:
    case FR_WRITE_PROTECTED: case FR_LOCKED: case FR_TOO_MANY_OPEN_FILES:
    case FR_INVALID_PARAMETER: case FR_NOT_READY:
#if _USE_LFN == 0 && !_FS_EXFAT
    case FR_DENIED:
#endif
        if(file->obj.fs) preset_sharedFailure = res;
        break;
    default:
        preset_sharedFailure = res;
        break;
    }
}
```

Existing write helper remains: check actual `f_write` result; only map FR_OK plus short count to FR_DISK_ERR; latch first failure; stop payload generation; do not close a partial write. A fully written save closes exactly once and returns close failure. S3's f_close calls f_sync and performs required synchronization; no redundant sync or bypass was added.

### Temporary-save service

On successful temporary pattern/kit save, clear only its corresponding dirty flag. On failure keep its flag and any other pending flag, set `preset_autosavePaused=1`, report the error once and perform no further automatic attempt. Later edits or notification dismissal do not resume autosave. If the application latch was already set by another operation, pending flags are left intact and storage is not attempted.

Manual saves continue after retryable logical errors. A successful manual save does not silently restart previously paused background writes or claim a temporary backup was saved. Session-long paused autosave is an explicit conservative limitation: use manual saves to retain new changes. The message says **Autosave paused** when that is the relevant state. No new button mapping or background recovery manager was added. Pending flags are RAM state only, not a persistent backup; power-off can still lose unsaved edits. Existing deliberate project/pattern changes can replace RAM state as before; S4-L does not implement an editor-wide unsaved-changes manager.

### Minimal project compatibility change

S4a's project caller ignored `preset_saveGlobals()`. The old blanket application latch incidentally made its next child save fail even for a harmless open error. Once such errors become retryable, leaving this ignored result would permit later writes and temporary cleanup after a failed required save. The caller now assigns `res`, checks failure, restores the existing LCD processing flag/timer hook and returns before later project writes or temp deletion.

This is directly required to remove blanket quarantine safely. It changes no folder naming, deletion sequence, file format, save order on success or transaction architecture. The branch is checked statically before child-write/temp-cleanup code; complete destructive project saving is not host-executed by this task.

## 6. Non-blocking notification design

`menu_showSdError` now only snapshots application-held/autosave-paused status, publishes result/time and queues `menu_repaintAll`. FR_OK is ignored. It does not wait, call HAL_Delay/LCD functions, start/stop TIM2, disable display processing, touch storage or alter menu page/selection/edit/transport state.

Existing `menu_tick` is invoked four times in the TIM14 display IRQ (`stm32f7xx_it.c:305–320`). A small expiry check uses the existing millisecond HAL_GetTick; unsigned subtraction handles wrap. At2000ms it clears only the error-overlay code and requests an ordinary repaint. Idle checks short-circuit without a tick read when no notification exists. This is display work in the existing display path, not new work in TIM2/audio/DMA processing.

`menu_repaint` renders two fixed16-character lines into the existing editDisplayBuffer during normal painting. `menu_tick` already queues clear/repaint until display_refresh_state==0; existing nonblocking sendDisplayBuffer advances its state machine normally. No new framebuffer, polling source or delay loop is added. Normal input event handlers continue; their screen is temporarily overlaid, and expiration repaints the **current** page rather than reverting to a saved page or swallowing navigation/edit events.

Headers are **SD error**, **Autosave paused** or **SD writes held**. Details include **Path? Retry save**, **Name? Retry save**, **Denied/full? Fix**, **SD unavailable**, **Check filesystem** or **RAM OK; check SD**. These are concise guidance, not invented removal/protection causes or recovery guarantees. The held caption reflects the application latch; a lower-level S1 fault that surfaces only as NOT_READY may instead say SD unavailable. The UI does not probe the card to improve the label or promise that repeated initialization can recover it.

The fixed overlay state consists of three volatile bytes plus one volatile uint32 timestamp; getters are readonly. Main publishes status/time before the code and queues a repaint. No history/queue allocation is added; latest error wins. A display IRQ can briefly observe an older/newer notification context during publication, as with existing buffer repaint updates; another queued repaint converges to the latest message. If expiry preempts a foreground text copy, the renderer rechecks the notification code after its fixed32-byte copy and queues one normal repaint when it changed. This prevents stale error text from remaining after an interrupt already consumed the expiry repaint. A deterministic memcpy-interleaving fixture covers this case. This is not a transactional diagnostic snapshot or an audio-synchronization mechanism. No long interrupt masking or lock was introduced. Very early startup or a caller that keeps display processing disabled can defer visibility; real LCD delivery/timing needs hardware observation.

## 7. Live operation: preserved paths and remaining synchronous stalls

**CODE/BUILD:** Notification expiry or a failed save never calls transport stop, sound reset, MIDI disable or RAM-kit/pattern disable. The new code does not discard active sound parameters, pattern data, pending temporary-save flags or success-only UI state. Existing RAM editor/copy/selection paths have no new storage guard. Audio/codec/IRQ full objects compare identical to baseline in all four builds (12/12).

This does not make storage asynchronous. FatFs calls perform synchronous traversal, allocation, disk reads/writes and metadata synchronization; S1's status checks can wait. S1 still allows5000ms transfer plus5000ms readiness; a nested S2 command/status/STOP wait can consume its own bounded allowance, and card initialization contains command/negotiation loops. No global deadline, supported-card latency or useful live maximum is established. The relevant HAL tick must run; halted/failed clocks or interrupt starvation are outside these software guarantees.

TIM2/foreground architecture remains: `config.h:29–30` enables CALC_SAMPLES_IN_INTERRUPT and USE_INTERRUPT_ONLY_DURING_SAVE_PROJ; main normally calls soundengine_process explicitly, which services rendering/sequence/MIDI and manipulates its existing TIM2 update state; save callers retain their existing TIM2 start/stop pairing. The removed error-specific TIM2 toggling is not replaced with new scheduling. While a synchronous foreground save is running, normal main-loop UI/USB and some foreground MIDI servicing can be delayed; existing interrupt-driven rendering/sequence/UART paths may mitigate this, but that is not proof of glitch-free operation.

Existing `preset_saveProject`/`preset_loadProject` still contain display wait loops and synchronous progress LCD work. Other load/communication loops exist in presetManager; S4-L does not redesign them. An error banner is non-blocking even though the operation that caused it can have blocked before returning. SDMMC peripheral cleanup after timeout remains unresolved. Hardware CPU/stack/audio/UART/USB timing measurements are still required after separate authorization.

## 8. Stored data and remaining S4b risks

**CODE:** Individual writers still use `FA_CREATE_ALWAYS|FA_WRITE`. A previously valid destination can be truncated during a successful open, then left incomplete by a later write/short-write/close error. Retaining a truthful error and fault latch does not restore its old bytes. No new file is deleted as recovery, but this pre-existing destructive overwrite architecture remains. Safe replacement requires a reviewed transaction/staging/ownership design; S4-L does not invent one or claim atomicity.

The complete-project path is still high priority:

- `preset_saveProject:2205` can empty an existing destination through `empty_directory` at2266 before all replacement files exist.
- Its f_stat branching, unchecked new-folder chdir, project header short-count handling, ignored header close return and temp-folder init/clear results remain S4b work. Only the directly affected global-save result is corrected here.
- `preset_clearTmpFolder` is still called after project completion at2396; a still-unchecked project-header short write can evade detection and permit backup deletion. S4-L does not certify project success.
- Project create and project load can clear temporary recovery data with ignored results (`:2196`, `:2596`); temporary-folder helpers and error cleanup own shared directory state incompletely.
- `main.c:925–939` still unlinks/unmounts/clears SDFatFS/remounts on detected insertion. Neither S1 nor app uncertainty is cleared by S4-L, but cached file/volume ownership after remount remains a broader concern.
- Some load-only functions ignore their close result when computing their own return value; wrapper quarantine protects ownership but does not fix all read-result truthfulness.
- Existing explicit format/startup/project wrapper flows in main can still show their own success despite ignored project/default-global results. No format was run; no recovery format was added. This task does not authorize or certify that destructive flow.

**Recommendation for S4b:** define shared volume/file/session lifecycle first; propagate every required project/header/dir/cleanup result; prohibit recovery-backup deletion after any unsuccessful save; review a non-destructive replacement design for projects and existing individual destinations before implementing staging/rename transactions. Test each child failure and power-loss/state boundary in RAM simulations. Review remount and retained FIL/lock ownership together. Do not clear serious latches simply to allow reuse, and do not convert these remaining issues into an unreviewed asynchronous/recovery architecture here.

## 9. Tests, evidence and exact outcomes

The exact S4a baseline was reproduced before source edits: eight suites, four ARM configure/build modes and image validation all passed. Final S4-L validation runs nine suites and the same four modes; every final command exited0.

| Command | Final result |
|---|---|
| `python3 tests/bug001/run.py` | PASS; CC0/CC2/NRPN/mutes/bank/Drive,256 Morph positions and cadence |
| `python3 tests/euclidean/run.py` | PASS six sanitized groups;2176 masks,294912 playback ticks,4160 recording positions,130 persistence roundtrips,10210 malformed cases,180802 protocol boundary calls |
| `python3 tests/safety/run.py` | PASS bounded heap/601 moves; diagnostics OFF/ON/wrap/saturation/single HAL call |
| `python3 tests/usb_allocator/run.py` | PASS2000 lifecycle cycles,14 failure scenarios, ownership/full zeroing and no heap references |
| `python3 tests/sd_storage/run.py` | PASS1..128-sector operations, readiness/wrap/fault persistence/status/init/ioctl/context and no reset/remount/retry |
| `python3 tests/sd_hal/run.py` | PASS active production command/HAL/BSP extracted bodies, bounded error/timeout/STOP/state/error fidelity, no replay/reset |
| `python3 tests/fatfs_integrity/run.py` | PASS configured/read-only full production units, mirror/FSInfo/sticky sync+close, S1 integration/init/mount/read |
| `python3 tests/save_errors/run.py` | PASS1536 exact successful S3 byte-oracle saves,2544 injected write/short/full-count-error positions; UI/owner/no-replay tests;936 logical open failures then104 successful explicit saves |
| `python3 tests/save_errors/run.py --verify-baseline` | PASS additionally confirms frozen actual S3 function bodies against exact Git object; run separately during development |
| `python3 tests/live_storage/run.py` | PASS actual notification/tick/repaint bodies plus complete production FatFs RAM-media cases described below |

Existing S4a tests change only the policy expectations that this task intentionally replaces: harmless open failures no longer quarantine; failed temporary saves retain dirty flags and pause background attempts. All uncertain-write/short-count/close/first-error/no-reuse assertions and successful byte comparisons remain. S3 frozen oracle and common helper digests are unchanged. No existing suite was weakened to hide an unrelated failure.

New/revised evidence:

- Actual application writer bodies are extracted by the existing runner, with production headers and mocked FatFs operations/UI notifications/timers. For13 open-result classes and all eight save variants, nine errors followed by one corrected success are executed:936 failures +104 successes. No error accumulation into a new app latch, writes/closes after rejected open, output-byte deletion or active-parameter loss occurs. Serious/unknown open timeout and all payload/close failures still quarantine.
- All2544 injected payload positions still stop immediately, preserve first errors and retain shared ownership; short FR_OK is detected. No success page/loaded-state update follows failed saves. Background dirty flags survive pattern/kit/path failures; later timer calls/edits make no automatic retry. Manual save after logical background failure remains possible without clearing paused/pending backup state.
- `tests/live_storage/notifications.c` extracts actual menu_showSdError/expire/render/menu_tick/menu_repaintAll/menu_repaint bodies. HAL tick, normal painters and LCD sender are mocked. Checks include1999/2000ms, unsigned wrap, queued notification while sender busy, latest-error replacement, all FatFs result labels, autosave caption, expiry preempting a foreground copy, current page preserved and storage-held state unchanged after dismissal.
- Static assertions reject delay/wait loops, HAL timer/LCD/filesystem/soundengine calls in new notification helpers. They verify project global-save failure returns before the later child/cleanup block. This is a static compatibility check, not a full project transaction simulation.
- `tests/live_storage/filesystem.c` includes complete production ff.c/diskio.c/ff_gen_drv.c/sd_diskio.c through the existing fixture. A RAM-only FAT16 volume is seeded with boot-sector bytes; it does not call f_mkfs or access a card. Actual f_open produces repeated missing-path, invalid-name, existing-file, read-only-target and full-SFN-root errors with zero mock physical writes and unchanged RAM media at the relevant rejection snapshots. An existing file reads back exact bytes and an explicit subsequent create/write/close succeeds. Filesystem cache/lock changes used to construct fixtures are test-only, not firmware recovery.

Sanitizers: fatal ASan/UBSan, no recovery; new native executables have20s deadlines, save suite40s. LeakSanitizer is disabled because executor tracing is unsupported; it is not claimed as passed. Native mocks do not prove physical timing, SD programming completion, real LCD/IRQ interleavings or power-loss durability. The notification test uses real menu function bodies, not the whole application or physical LCD. Existing S3 tests independently execute full public f_sync/f_close with production S1 faults; the save test mocks close to inject each result rather than pretending those are physical card tests.

## 10. Builds, image structure and resources

Prepared command environment:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

Tools: CMake3.31.10; Ninja1.13.0.git.kitware.jobserver-pipe-1; Python3.12.14; ARM GCC14.2.1 20241119 (`15:14.2.rel1-1`), native GCC14.2.0. Baseline tool versions match the previous prepared setup. Release and Debug retain established settings; Debug is optimized `-O3 -g3`, not an assumed unoptimized configuration.

Executed both baseline and final:

```sh
cmake --preset release
cmake --build --preset release --parallel 4
cmake --preset debug
cmake --build --preset debug --parallel 4
cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
cmake --build build/diagnostics-release --parallel 4
cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
cmake --build build/diagnostics-debug --parallel 4
python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
```

All four modes and all images PASS. Defaults/presets remain unchanged; caches confirm diagnostics OFF for normal release/debug, ON only for isolated directories. Version remains1.7, with `LXR_V2_V1.7.img`, `LXR_V2_STM32F765.bin`, `LXR_V2_STM32F765.elf` under each build directory. No full-flash/bootloader image was configured. The build prints generic installation instructions; none were followed.

All images pass16-byte `<8sIB3s` header, `LXRV2IMG`, LE payload length, zero padding, checksum `0xff-(sum(payload)&0xff)`, exact .bin correspondence, configured memory/load-address limits and plausible vector entries. Initial MSP is0x20080000 and reset0x08014E4D (Thumb), unchanged. Application starts0x08008000; first32KiB excluded. Linker/bootloader/update/option-byte configuration is unchanged; physical protection state is not inspected. No binary-equivalence claim with manufacturer v1.71 (private support1.72 is not the reference) or physical updater qualification follows from format validity.

| Bytes | S4a Release OFF | S4-L Release OFF | S4a Debug OFF | S4-L Debug OFF | S4a Release ON | S4-L Release ON | S4a Debug ON | S4-L Debug ON |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| ITCM |15016|15016|15016|15016|15088|15088|15088|15088|
| DTCM |107392|107392|107392|107392|107392|107392|107392|107392|
| General SRAM |271576|271584|271576|271584|271648|271656|271648|271656|
| Linker FLASH used |225424|225280|225472|225328|225792|225648|225840|225696|
| Full .bin payload |277436|277292|277484|277340|277876|277732|277924|277780|
| Header+payload .img |277452|277308|277500|277356|277892|277748|277940|277796|

**BUILD impact: -144bytes Flash/payload and +8bytes SRAM per mode; ITCM/DTCM unchanged.** New fixed fields are seven notification bytes (uint32 timestamp plus three uint8 flags) and one autosave-pause byte. Existing app failure storage remains one byte in this ELF; no portable enum-width claim is inferred. There is no new frame/export/history buffer or allocation; existing LCD buffers are reused.

Remaining capacity: ITCM1368 normal/1296 ON; DTCM23680 before runtime stack; SRAM121632 normal/121560 ON. Application Flash capacity1015808 leaves full-payload headroom738516/738468/738076/738028 bytes respectively. Validator Flash VMA high-water224428/224476/224796/224844 differs from linker FLASH used and the whole load/payload span; initialized ITCM/DTCM/data occupy load storage too. Static reservations are not runtime stack/heap high-water measurements.

Source images were built from the exact working-tree bytes later published as `feaff649599472036390ecef24f7193b7cc2e2ab`; Git tree/blob equality was verified. HEAD had not yet acquired the API-created commit when the builds ran. No source bytes changed after final validation. All artifacts remain local. Complete image/ELF/bin SHA-256 and object comparisons are appended; no installation binaries are attached.

## 11. Development errors and review limitations

- The managed environment initially reported starting/unavailable to a shell call; repository connector verification proceeded and the same existing checkout became available. No replacement worktree was created.
- First new notification harness compile lacked ff.h, so FRESULT/FR_* were undefined. Added the actual header; production was not changed to silence a test error.
- First RAM FatFs test could not open its file because the reused fixture deliberately holds one artificial lock and target `_FS_LOCK=1`. The new RAM test now removes only that test fixture's artificial pre-opened lock before mounting. No firmware lock clear/recovery was added; the intended real failure tests then passed.
- Initial full-object comparison failed for Debug after inserting declarations mid-header, because existing declaration line positions/debug metadata shifted. Moving the new declarations to the header end preserved existing positions. Final rebuilt full hashes are identical12/12; no code assertion was suppressed and executable code was not altered to manufacture a comparison.
- Final concurrency review identified possible stale text if TIM14 expiry preempted the foreground renderer after it read the error code. A bounded requeue and deterministic interrupt/copy interleaving test were added in the final follow-up commit. All nine suites/four builds/images and full-object comparisons were rerun successfully; the earlier published commit remains in history rather than being overwritten.
- Existing native FatFs registration test warns about unused `lun`; unchanged. All suites exit0 and no sanitizer/assertion was disabled to hide it. Final diff whitespace check passes. The old frozen S3 oracle's inherited whitespace is unchanged and not a new S4-L diff finding.
- Early prototype iterations used a more conservative open-DENIED policy; source review of the active SFN path and actual full-root/read-only tests justified the final guarded classification. All final suites/builds were rerun after the final code change, including autosave caption status.

No physical card errors, corrupt filesystem, CPU load, latency, stack use, thermal/audio reliability or power-loss recovery were observed/measured. Classification protects software invariants under tested returns; it cannot identify electrical card removal/write protection from a generic command error. S1 faults may still require reboot/card replacement, but reboot does not guarantee the old file is intact or that a defective card is repaired.

## 12. Final safety assessment and continuation

1. **CODE/BUILD:** harmless classified failed opens no longer create application-wide reboot-only storage lockout. Repeated explicit attempts can succeed after correction; unknown/owned/payload/close failures retain protection.
2. **CODE/BUILD:** the two-second error delay and LCD completion wait are removed from notification. No notification-specific TIM2 toggling or new audio work. Fixed overlay automatically dismisses and retains menu/RAM input state.
3. **CODE/BUILD:** dirty temporary-save state survives failure. Autosave pauses rather than silently clearing flags or replaying writes. Manual saves remain available after logical errors; a paused background session does not automatically resume after a manual save.
4. **CODE:** serious S1/S2 faults and unresolved shared-FIL state are not cleared; some scenarios still require reboot and media inspection/replacement. This is storage containment, not punishment of RAM performance. Independent safe read recovery is not implemented.
5. **BUILD:** nine final suites, four ARM modes and four images pass; baseline reproduced; memory fits; audio/codec/IRQ objects unchanged. This is review evidence, not physical live-performance validation.
6. **OPEN:** synchronous I/O can still delay main-loop MIDI/USB/UI and affect performance under load. No glitch-free guarantee. Physical cleanup, real timeout policy and persistent data recovery remain unresolved.
7. **OPEN:** individual overwrite safety, destructive project replacement, recovery-backup ownership, remount/retained FIL lifecycle and high-level false-success gaps remain S4b prerequisites. Proceed to a bounded S4b design/review, not hardware deployment or the diagnostic exporter.

No hardware was operated/flashed; no physical SD card accessed, formatted, modified or deleted; no bootloader/update/option-byte/startup/linker/clock/priority change; no watchdog/automatic reset; no branch merge/force push; no binary publication. Physical testing requires separate explicit authorization after review.

## 13. Publication method and verification contract

GitHub Git-data APIs published base64 blobs preserving original CRLF/non-UTF8 source bytes, an initial tree/commit based on exact S4a, followed by one single-parent concurrency correction and a non-forced, expected-head-checked fast-forward of this task's new source ref. The exact commit/tree was imported into the existing local branch and the checkout is clean. No older branch was moved.

The report is a separate new Markdown-only file on a new branch directly based on lab-setup. Its own commit cannot contain its own hash; use the verified remote branch head. Before final success response, fetch the exact report URL and compare full text to this local report, verify both new branch heads, source parent/tree/blob identities, single added report-file diff and every pre-existing remote head. This post-publication verification necessarily occurs after this immutable document is committed; no report overwrite is required.


## Appendix A — Complete changed-file manifest and diff

10 files,341 insertions,82 deletions relative to exact S4a. No generated binaries/build directories.

- `Core/Inc/Preset/PresetManager.h` — blob `1c797e2e555442df391bbf65ebf13aa1279c37fb`.
- `Core/Src/Preset/presetManager.c` — blob `ad18c404a6f38f4a9fedb299bbea4977294ac070`.
- `Core/Src/menu/menu.c` — blob `737cf6af915f3845066313e46e13e0500d321993`.
- `tests/save_errors/README.md` — blob `3b37aaa2658017d043c0ab1348703f293b078a24`.
- `tests/save_errors/host.c` — blob `c03a8fb4389dd7e35f881697489bf4edc9785ed1`.
- `tests/save_errors/run.py` — blob `71616b30b86bf5adb331625be59005d824d061be`.
- `tests/live_storage/README.md` — blob `73f0b9ad6269a0df55dc5a7e66619414fc4d41ce`.
- `tests/live_storage/filesystem.c` — blob `02ae91eb9e46fde2a6db64340216f3d9a902f8df`.
- `tests/live_storage/notifications.c` — blob `16fdd451942b6a8651c8c27c0289df97492db5dc`.
- `tests/live_storage/run.py` — blob `c2254d9341193103a73a664337e6995ccc1b6e42`.

```diff
diff --git a/Core/Inc/Preset/PresetManager.h b/Core/Inc/Preset/PresetManager.h
index bf7cc40..1c797e2 100644
--- a/Core/Inc/Preset/PresetManager.h
+++ b/Core/Inc/Preset/PresetManager.h
@@ -118,4 +118,8 @@ void preset_sendDrumsetParametersImmediate();
 
 uint8_t preset_isNameEmpty(char* name);
 
+/* Application uncertainty only; FR_OK does not clear or bypass the S1 disk latch. */
+FRESULT preset_storageError(void);
+uint8_t preset_autosaveIsPaused(void);
+
 #endif /* PRESETMANAGER_H_ */
diff --git a/Core/Src/Preset/presetManager.c b/Core/Src/Preset/presetManager.c
index 90a4046..ad18c40 100644
--- a/Core/Src/Preset/presetManager.c
+++ b/Core/Src/Preset/presetManager.c
@@ -84,10 +84,20 @@ uint32_t preset_patTimer = 0;
 uint8_t preset_kitDirty = 0;
 uint8_t preset_patDirty = 0;
 
-/* One shared FIL: failed/short writes and failed closes are quarantined until reboot.
+/* One shared FIL: uncertain writes/closes are quarantined until reboot.
  * Never clear FIL.err or close after an incomplete write: close could flush more data.
  */
 static FRESULT preset_sharedFailure = FR_OK;
+/* Pending RAM changes survive failure; no background retry after an error. */
+static uint8_t preset_autosavePaused;
+uint8_t preset_autosaveIsPaused(void)
+{
+    return preset_autosavePaused;
+}
+FRESULT preset_storageError(void)
+{
+    return preset_sharedFailure;
+}
 static FRESULT preset_sharedStatus(void)
 {
     if(preset_sharedFailure != FR_OK) return preset_sharedFailure;
@@ -98,8 +108,26 @@ static FRESULT preset_openShared(FIL *file, const TCHAR *name, BYTE mode)
     FRESULT res = preset_sharedStatus();
     if(res != FR_OK) return res;
     res = f_open(file, name, mode);
-    if(res != FR_OK && ((mode & FA_WRITE) || res == FR_DISK_ERR || res == FR_INT_ERR))
-        preset_sharedFailure = res;
+    if(res != FR_OK) {
+        /* These f_open failures precede file creation/truncation in this FatFs.
+         * The S1 disk latch still blocks hardware access after a physical fault.
+         * DENIED is logical only for the configured single-SFN, non-exFAT path.
+         */
+        switch(res) {
+        case FR_NO_FILE: case FR_NO_PATH: case FR_INVALID_NAME: case FR_EXIST:
+        case FR_INVALID_DRIVE: case FR_NOT_ENABLED: case FR_NO_FILESYSTEM:
+        case FR_WRITE_PROTECTED: case FR_LOCKED: case FR_TOO_MANY_OPEN_FILES:
+        case FR_INVALID_PARAMETER: case FR_NOT_READY:
+#if _USE_LFN == 0 && !_FS_EXFAT
+        case FR_DENIED: /* Read-only/directory target, full SFN table or no free cluster. */
+#endif
+            if(file->obj.fs) preset_sharedFailure = res;
+            break;
+        default:
+            preset_sharedFailure = res;
+            break;
+        }
+    }
     return res;
 }
 static FRESULT preset_closeShared(FIL *file)
@@ -601,21 +629,21 @@ FRESULT preset_clearTmpFolder()
 void preset_processTimer()
 {
     FRESULT res = FR_OK;
-    if(preset_sharedFailure != FR_OK) {
-        preset_resetAllSaveTimers();
-        return;
-    }
+    if(preset_sharedFailure != FR_OK || preset_autosavePaused) return;
     if(preset_patDirty && HAL_GetTick() - preset_patTimer >= PRESET_SAVE_TIME) {
-        preset_patDirty = 0;
         res = preset_saveTmpPat();
+        if(res == FR_OK) preset_patDirty = 0;
     }
     if(res == FR_OK && preset_kitDirty && HAL_GetTick() - preset_kitTimer >= PRESET_SAVE_TIME) {
-        preset_kitDirty = 0;
         res = preset_saveTmpKit();
+        if(res == FR_OK) preset_kitDirty = 0;
     }
     if(res != FR_OK) {
-        preset_resetAllSaveTimers();
-        menu_showSdError(res); /* This service is called only by the foreground loop. */
+        /* Keep RAM dirty state; manual saves remain available after logical errors.
+         * Do not restart autosave on edits or notifications: no implicit replay.
+         */
+        preset_autosavePaused = 1;
+        menu_showSdError(res);
     }
 }
 
@@ -2304,7 +2332,14 @@ FRESULT preset_saveProject(uint8_t presetNr)
 
 //	soundengine_process();
 	//save globals
-	preset_saveGlobals();
+	res = preset_saveGlobals();
+    if(res != FR_OK) {
+        lcd_process_display_interrupt = 1;
+#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
+        HAL_TIM_Base_Stop_IT(&htim2);
+#endif
+        return res; /* No later project writes or temporary-file deletion. */
+    }
 	//save all drumkits, patterns and songs
 	for(uint8_t i=0;i<NUM_KIT_SLOTS;i++)
 	{
diff --git a/Core/Src/menu/menu.c b/Core/Src/menu/menu.c
index c6b45d2..737cf6a 100644
--- a/Core/Src/menu/menu.c
+++ b/Core/Src/menu/menu.c
@@ -670,6 +670,13 @@ uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS];
 uint8_t menu_loadedPresetNr[NUM_CURRENT];
 
 uint32_t sucessTimer = 0;
+/* Notification only: no page/input/transport state is changed. Latest error wins. */
+static volatile uint8_t menu_sdError;
+static volatile uint8_t menu_sdErrorHeld;
+static volatile uint8_t menu_sdErrorAuto;
+static volatile uint32_t menu_sdErrorSince;
+static void menu_expireSdError(void);
+static uint8_t menu_renderSdError(void);
 
 uint8_t menu_shownPattern = 0;
 uint8_t menu_muteModeActive = 0;
@@ -984,6 +991,7 @@ uint8_t sendDisplayBuffer()
 //-----------------------------------------------------------------
 void menu_tick()
 {
+    menu_expireSdError();
 
 	if((menu_activePage == PATTERN_SAVE_PAGE_OK) || (menu_activePage == KIT_SAVE_PAGE_OK))
 	{
@@ -1810,6 +1818,7 @@ void menu_repaintGeneric()
 // repaint the screen
 void menu_repaint()
 {
+    if(menu_renderSdError()) return;
 	if(screensaver_Active) return;
 
 	visibleCursor = 0;
@@ -1924,62 +1933,48 @@ void menu_handleSaveScreenKnobValue(uint8_t potNr, uint8_t value)
 
 }
 //-----------------------------------------------------------------
+/* Rendered by the existing bounded display refresh, never by a wait loop. */
+static void menu_expireSdError(void)
+{
+    if(menu_sdError != FR_OK && (uint32_t)(HAL_GetTick() - menu_sdErrorSince) >= 2000u) {
+        menu_sdError = FR_OK;
+        menu_repaintAll();
+    }
+}
+static uint8_t menu_renderSdError(void)
+{
+    const char *detail;
+    uint8_t error = menu_sdError;
+    if(error == FR_OK) return 0;
+    if(menu_sdErrorHeld) detail = "RAM OK; check SD";
+    else switch(error) {
+    case FR_NO_FILE: case FR_NO_PATH: detail = "Path? Retry save"; break;
+    case FR_INVALID_NAME: case FR_INVALID_DRIVE: case FR_INVALID_PARAMETER: detail = "Name? Retry save"; break;
+    case FR_EXIST: detail = "Exists: rename  "; break;
+    case FR_LOCKED: case FR_TOO_MANY_OPEN_FILES: detail = "File busy: retry"; break;
+    case FR_DENIED: detail = "Denied/full? Fix"; break;
+    case FR_WRITE_PROTECTED: detail = "Write protected "; break;
+    case FR_NO_FILESYSTEM: detail = "Check filesystem"; break;
+    case FR_NOT_READY: case FR_NOT_ENABLED: detail = "SD unavailable  "; break;
+    default: detail = "Check SD; RAM OK"; break;
+    }
+    visibleCursor = 0;
+    memcpy(editDisplayBuffer[0], menu_sdErrorHeld ? "SD writes held  " :
+        (menu_sdErrorAuto ? "Autosave paused " : "SD error        "), 16);
+    memcpy(editDisplayBuffer[1], detail, 16);
+    display_needs_refresh = 1;
+    /* Expiry may preempt a foreground repaint; never leave its stale overlay. */
+    if(menu_sdError != error) menu_repaintAll();
+    return 1;
+}
 void menu_showSdError(uint8_t res)
 {
-
-#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-	HAL_TIM_Base_Start_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
-#endif
-
-	lcd_process_display_interrupt = 0;
-	while(sendDisplayBuffer() != 0);
-
-	lcd_clear();
-	lcd_home();
-	lcd_string(PSTR("SD Card error:"));
-	lcd_setcursor(0,2);
-	switch(res)
-	{
-		// There is no valid FAT volume on the drive or wrong lower layer implementation.
-		case FR_NO_FILESYSTEM:
-			lcd_string("No FS found!");
-			break;
-
-		// The lower layer, disk_read, disk_write or disk_ioctl function, reported that an unrecoverable hard error occured.
-		case FR_DISK_ERR:
-
-			lcd_string("Disk error!");
-			break;
-
-		/*The lower layer, disk_initialize function, reported that the storage device could not be got ready to work. One of the following possibilities is suspected.
-
-		No medium in the drive.
-		Wrong lower layer implementation.
-		Wrong hardware configuration.
-		The storage device has broken.
-		*/
-		case FR_NOT_READY:
-			lcd_string("No Card!");
-			break;
-
-		case FR_NO_FILE:
-			lcd_string("File not found!");
-			break;
-
-		default:
-			lcd_string("unknown!");
-			break;
-
-	}
-	HAL_Delay(2000);
-#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-	HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
-#endif
-	lcd_process_display_interrupt = 1;
+    if(res == FR_OK) return;
+    menu_sdErrorHeld = preset_storageError() != FR_OK;
+    menu_sdErrorAuto = preset_autosaveIsPaused();
+    menu_sdErrorSince = HAL_GetTick();
+    menu_sdError = res;
+    menu_repaintAll(); /* Queue; no delay, LCD wait, timer stop or storage work. */
 }
 //-----------------------------------------------------------------
 void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
diff --git a/tests/live_storage/README.md b/tests/live_storage/README.md
new file mode 100644
index 0000000..73f0b9a
--- /dev/null
+++ b/tests/live_storage/README.md
@@ -0,0 +1,41 @@
+# S4-L live storage policy tests
+
+Run with the prepared toolchain on PATH:
+
+```
+python3 tests/save_errors/run.py --verify-baseline
+python3 tests/live_storage/run.py
+```
+
+The first suite retains the S3 byte oracle, 1536 exact-success comparisons and
+2544 injected write positions. It extends the actual application writer bodies
+with nine recoverable open failures then one success for 13 result classes across
+eight variants, no silent object/valid-byte reuse, retained RAM/dirty flags and
+paused background retries. Only assertions for intentionally changed logical-open
+quarantine/dirty-flag policy are updated; uncertain-write/close tests remain.
+
+The second suite extracts actual menu_showSdError, notification expiry/renderer,
+menu_tick, menu_repaintAll and menu_repaint. Native mocks replace HAL time,
+normal menu painters and the LCD sender. Tests cover two-second expiry (1999/2000),
+wraparound, busy display queuing, latest-message replacement, every FatFs result,
+page state retention, expiry preempting a foreground copy and no storage-latch
+clear on dismissal. A test-only memcpy wrapper injects one menu_tick interleaving;
+production uses ordinary fixed-size memcpy and a bounded repaint request. Static checks reject
+waits, delay, timer/LCD/storage/audio calls in the new notification helpers.
+Existing menu_tick/sendDisplayBuffer architecture is not a physical LCD test.
+
+filesystem.c includes the entire production FatFs, generic disk and S1 units via
+the existing integrity fixture. A RAM-only FAT16 volume verifies nine repetitions
+of actual missing-path, invalid-name and exclusive-create conflicts make zero
+physical mock writes, preserve all RAM-sector bytes and an existing file, and
+permit a subsequent successful create/write/close. Read-only-target and full
+SFN-root denial are also verified without disk writes. Boot-sector bytes are fixture
+initialization, not filesystem formatting or physical media operations. The fixture
+has an artificial pre-opened lock; only this test fixture clears it before mounting.
+No production file-lock release or hardware recovery is implemented.
+
+Both native executables use fatal ASan/UBSan and 20-second process deadlines.
+LeakSanitizer is disabled for executor tracing. Full translation units are compiled
+by ARM builds. No card programming latency, audio continuity, physical persistence,
+interrupt timing or live hardware behavior is established. S4b overwrite/project
+lifecycle and shared-volume recovery remain necessary follow-up work.
diff --git a/tests/live_storage/filesystem.c b/tests/live_storage/filesystem.c
new file mode 100644
index 0000000..02ae91e
--- /dev/null
+++ b/tests/live_storage/filesystem.c
@@ -0,0 +1,50 @@
+/* Full production FatFs/disk/S1 units via the established fixture, RAM sectors only. */
+#define main previous_integrity_main
+#include "../fatfs_integrity/host.c"
+#undef main
+static BYTE saved_media[sizeof media];
+int main(void)
+{
+ setup();
+ /* Drop only the fixture's artificial pre-opened lock before mounting RAM media. */
+ memset(Files,0,sizeof Files);
+ FATFS mounted;memset(&mounted,0,sizeof mounted);
+ BYTE *boot=media[0];boot[0]=0xeb;boot[1]=0x3c;boot[2]=0x90;
+ memcpy(boot+BS_FilSysType,"FAT16   ",8);
+ fixture_put16(boot+BPB_BytsPerSec,512);boot[BPB_SecPerClus]=1;
+ fixture_put16(boot+BPB_RsvdSecCnt,1);boot[BPB_NumFATs]=2;
+ fixture_put16(boot+BPB_RootEntCnt,512);fixture_put16(boot+BPB_TotSec16,8192);
+ fixture_put16(boot+BPB_FATSz16,32);boot[BPB_Media]=0xf8;fixture_put16(boot+BS_55AA,0xaa55);
+ media[1][0]=media[33][0]=0xf8;media[1][1]=media[1][2]=media[1][3]=0xff;
+ memcpy(media[33],media[1],512);
+ assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&fake_driver,drive)==0);
+ assert(f_mount(&mounted,drive,1)==FR_OK);
+ FIL save;memset(&save,0,sizeof save);UINT count;
+ const char payload[]="Previously valid kit bytes";
+ assert(f_open(&save,"GOOD.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_OK);
+ assert(f_write(&save,payload,sizeof payload,&count)==FR_OK && count==sizeof payload);
+ assert(f_close(&save)==FR_OK);
+ memcpy(saved_media,media,sizeof media);unsigned original_writes=writes;
+ for(unsigned attempt=0;attempt<9;attempt++){
+  assert(f_open(&save,"MISSING/GOOD.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_NO_PATH && !save.obj.fs);
+  assert(f_open(&save,"BAD|NAME.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_INVALID_NAME && !save.obj.fs);
+  assert(f_open(&save,"GOOD.SND",FA_CREATE_NEW|FA_WRITE)==FR_EXIST && !save.obj.fs);
+ }
+ /* Read-only overwrite denial is before truncation in configured f_open. */
+ media[65][DIR_Attr]|=AM_RDO;mounted.winsect=0xffffffffu;
+ for(unsigned n=0;n<9;n++)assert(f_open(&save,"GOOD.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_DENIED && !save.obj.fs);
+ media[65][DIR_Attr]&=(BYTE)~AM_RDO;mounted.winsect=0xffffffffu;
+ assert(writes==original_writes && !memcmp(saved_media,media,sizeof media));
+ assert(f_open(&save,"GOOD.SND",FA_READ)==FR_OK);char actual[sizeof payload];
+ assert(f_read(&save,actual,sizeof actual,&count)==FR_OK && count==sizeof payload && !memcmp(payload,actual,sizeof payload));
+ assert(f_close(&save)==FR_OK);
+ assert(f_open(&save,"NEW.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_OK);
+ assert(f_write(&save,payload,sizeof payload,&count)==FR_OK && count==sizeof payload);
+ assert(f_close(&save)==FR_OK);
+ /* Full static SFN root, fixture bytes only; no format/recovery operation. */
+ memset(media[65],0x41,32*512);mounted.winsect=0xffffffffu;
+ memcpy(saved_media,media,sizeof media);original_writes=writes;
+ for(unsigned n=0;n<9;n++)assert(f_open(&save,"FULL.SND",FA_CREATE_ALWAYS|FA_WRITE)==FR_DENIED && !save.obj.fs);
+ assert(writes==original_writes && !memcmp(saved_media,media,sizeof media));
+ puts("PASS complete production FatFs: nine missing-path/invalid-name/existing-file/readonly-target/full-root failures make zero disk writes, preserve all RAM media/valid file bytes, then explicit create/write/close succeeds");
+}
diff --git a/tests/live_storage/notifications.c b/tests/live_storage/notifications.c
new file mode 100644
index 0000000..16fdd45
--- /dev/null
+++ b/tests/live_storage/notifications.c
@@ -0,0 +1,80 @@
+/* Actual menu notification/refresh bodies; time, LCD sender and normal painters mocked. */
+#include <assert.h>
+#include <stdint.h>
+#include <stdio.h>
+#include <string.h>
+#include "menu/menu.h"
+#include "ff.h"
+uint8_t menu_activePage,visibleCursor,screensaver_Active,copyClear_showClearMenu;
+uint8_t display_refresh_state,display_needs_refresh,display_needs_clear_all;
+char editDisplayBuffer[2][17],currentDisplayBuffer[2][16];
+volatile uint32_t systick_ticks;
+uint32_t sucessTimer;
+static volatile uint8_t menu_sdError,menu_sdErrorHeld,menu_sdErrorAuto;
+static uint8_t autosave_paused;
+static volatile uint32_t menu_sdErrorSince;
+static uint32_t now;
+static FRESULT held;
+static unsigned paints,sends;
+static uint8_t expire_on_copy;
+#define SELECT_MODE_VOICE 0
+uint32_t HAL_GetTick(void){return now;}
+FRESULT preset_storageError(void){return held;}
+uint8_t preset_autosaveIsPaused(void){return autosave_paused;}
+void buttonHandler_setMode(uint8_t mode){(void)mode;}
+void led_clearAllBlinkLeds(void){}
+void led_setMode2(uint8_t mode){(void)mode;}
+void menu_switchPage(uint8_t page){menu_activePage=page;}
+uint8_t menu_getActiveVoice(void){return 0;}
+uint8_t menu_getSubPage(void){return 0;}
+void led_setActiveSelectButton(uint8_t page){(void)page;}
+void menu_resetActiveParameter(void){}
+static void paint(void){paints++;memset(editDisplayBuffer,'R',sizeof editDisplayBuffer);}
+static void menu_repaintLoadSavePage(void){paint();}
+static void menu_repaintSimplePage(void){paint();}
+void menu_repaintGeneric(void){paint();}
+uint8_t sendDisplayBuffer(void){sends++;return display_refresh_state;}
+static void *interleaved_copy(void *to,const void *from,size_t count)
+{
+ void *result=memcpy(to,from,count);
+ if(expire_on_copy){expire_on_copy=0;now+=2000;menu_tick();}
+ return result;
+}
+#define memcpy interleaved_copy
+#include "functions.inc"
+#undef memcpy
+static void reset(void)
+{
+ menu_activePage=SAVE_PAGE;visibleCursor=2;screensaver_Active=copyClear_showClearMenu=0;
+ display_refresh_state=display_needs_clear_all=display_needs_refresh=0;
+ memset(editDisplayBuffer,0,sizeof editDisplayBuffer);menu_sdError=FR_OK;held=FR_OK;
+ now=100;autosave_paused=0;expire_on_copy=0;paints=sends=0;systick_ticks=0;
+}
+int main(void)
+{
+ reset();menu_showSdError(FR_OK);assert(!display_needs_clear_all);
+ menu_showSdError(FR_NO_PATH);assert(display_needs_clear_all && menu_activePage==SAVE_PAGE && !sends);
+ menu_tick();assert(sends==1 && !memcmp(editDisplayBuffer[1],"Path? Retry save",16));
+ /* Navigation/edit events are not consumed; overlay expires onto the current page. */
+ menu_activePage=LOAD_PAGE;menu_repaint();assert(menu_activePage==LOAD_PAGE && !paints);
+ now+=1999;menu_tick();assert(menu_sdError==FR_NO_PATH && !paints);
+ now++;menu_tick();assert(menu_sdError==FR_OK && paints==1 && menu_activePage==LOAD_PAGE);
+ reset();now=UINT32_MAX-500u;menu_showSdError(FR_INVALID_NAME);now+=1999u;menu_tick();assert(menu_sdError);
+ now++;menu_tick();assert(!menu_sdError && paints==1);
+ /* Notification queues while refresh is busy; no wait for mocked LCD readiness. */
+ reset();display_refresh_state=3;menu_showSdError(FR_EXIST);menu_tick();assert(display_needs_clear_all && !paints);
+ held=FR_DISK_ERR;menu_showSdError(FR_DISK_ERR);display_refresh_state=0;menu_tick();
+ assert(!memcmp(editDisplayBuffer[0],"SD writes held  ",16) && !memcmp(editDisplayBuffer[1],"RAM OK; check SD",16));
+ now+=2000;menu_tick();assert(!menu_sdError && held==FR_DISK_ERR); /* dismiss never unlocks storage */
+ reset();autosave_paused=1;menu_showSdError(FR_NO_PATH);menu_tick();assert(!memcmp(editDisplayBuffer[0],"Autosave paused ",16));
+ /* Simulated display IRQ expires midway through actual foreground overlay copy. */
+ reset();menu_showSdError(FR_NO_PATH);menu_tick();expire_on_copy=1;menu_repaint();
+ assert(!menu_sdError && display_needs_clear_all);menu_tick();
+ for(unsigned row=0;row<2;row++)for(unsigned col=0;col<16;col++)assert(editDisplayBuffer[row][col]=='R');
+ for(unsigned result=1;result<=FR_INVALID_PARAMETER;result++){
+  reset();menu_showSdError((uint8_t)result);menu_tick();
+  assert(editDisplayBuffer[0][16]==' ' && editDisplayBuffer[1][16]==' ' && !paints);
+  now+=2000;menu_tick();assert(menu_sdError==FR_OK && paints==1);
+ }
+ puts("PASS actual notification/tick/repaint bodies: queued/busy display, 1999/2000ms and wrap, latest error, all result codes, expiry/copy interleaving, page/RAM independence, no storage unlock or blocking calls");
+}
diff --git a/tests/live_storage/run.py b/tests/live_storage/run.py
new file mode 100644
index 0000000..c2254d9
--- /dev/null
+++ b/tests/live_storage/run.py
@@ -0,0 +1,25 @@
+#!/usr/bin/env python3
+"""S4-L production notification and full FatFs RAM-media tests. No hardware."""
+from pathlib import Path
+import importlib.util,os,re,subprocess,sys,tempfile
+sys.dont_write_bytecode=True
+ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
+spec=importlib.util.spec_from_file_location('save_helpers',ROOT/'tests/save_errors/run.py')
+helpers=importlib.util.module_from_spec(spec);spec.loader.exec_module(helpers)
+text=(ROOT/'Core/Src/menu/menu.c').read_text()
+functions=['menu_expireSdError','menu_renderSdError','menu_showSdError','menu_tick','menu_repaintAll','menu_repaint']
+source=''.join(helpers.extract(text,n) for n in functions)
+for name in functions[:3]:
+    body=helpers.extract(text,name)
+    assert not re.search(r'\b(?:HAL_Delay|HAL_TIM_\w+|lcd_\w+|sendDisplayBuffer|f_\w+|soundengine_process)\s*\(',body)
+    assert not re.search(r'\b(?:while|for)\s*\(',body)
+assert text.count('menu_expireSdError();')==1 and 'if(menu_renderSdError()) return;' in helpers.extract(text,'menu_repaint')
+includes=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc','Middlewares/Third_Party/FatFs/src','FATFS/App','FATFS/Target']
+with tempfile.TemporaryDirectory(prefix='lxr-live-storage-') as work:
+    work=Path(work);(work/'functions.inc').write_text(source)
+    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
+    flags=[os.environ.get('CC','cc'),'-std=gnu11','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
+    cmd=flags+['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast']+['-I'+str(ROOT/p) for p in includes]+['-I'+str(work),str(HERE/'notifications.c'),'-o',str(work/'notifications')]
+    subprocess.run(cmd,cwd=ROOT,env=env,check=True);subprocess.run([str(work/'notifications')],check=True,env=env,timeout=20)
+    cmd=flags+['-I'+str(ROOT/'tests/fatfs_integrity'),'-I'+str(ROOT/'FATFS/Target'),'-I'+str(ROOT),'-I'+str(ROOT/'Middlewares/Third_Party/FatFs/src'),'-I'+str(ROOT/'tests/sd_storage'),str(HERE/'filesystem.c'),'-o',str(work/'filesystem')]
+    subprocess.run(cmd,cwd=ROOT,env=env,check=True);subprocess.run([str(work/'filesystem')],check=True,env=env,timeout=20)
diff --git a/tests/save_errors/README.md b/tests/save_errors/README.md
index 68a2fd0..3b37aaa 100644
--- a/tests/save_errors/README.md
+++ b/tests/save_errors/README.md
@@ -26,15 +26,22 @@ preprocessor scan ensures only ownership wrappers call f_open/f_close and raw
 writes remain only in the checked helper and the explicitly deferred project
 header. Project lifecycle guards are checked before destructive entry work.
 
-A failed write-mode open, any write failure/short write, or failed close latches
-the shared-file error until reboot. Read-mode no-file/path fallback stays usable;
-serious read-open or close failures also quarantine. There is no production latch
-clear, FIL.err reset, automatic retry/reset/remount, or forced file-lock release.
-Short writes are mapped to FR_DISK_ERR. The policy deliberately does not close
-a partially written file: close can flush further payload/FAT/directory data even
-when the short write returned FR_OK and FIL.err is still zero. Normal saves close
-exactly once. The retained object/lock and even known write-open errors such as
-missing paths require reboot; this conservative restriction is intentional.
+S4-L refines open failures: missing paths/files, invalid names/drives/parameters,
+existence conflicts, disabled/unavailable/unknown filesystem, protection and lock
+errors do not set an application latch when the failed open leaves no owner.
+Nine failures followed by a corrected successful attempt are tested for all 13
+classified results and all eight save variants. S1's physical fault latch is
+never cleared or bypassed. Open DENIED is logical only under the configured _USE_LFN=0/non-exFAT guard:
+SFN allocation requests one entry; full-table/no-free-cluster and attribute
+rejection are distinct from observed disk errors. Payload DENIED remains held.
+
+Any write failure/short write or failed close still latches shared-file uncertainty
+until reboot. Short writes map to FR_DISK_ERR. There is no close after incomplete
+write, FIL.err reset, automatic retry/reset/remount, or forced lock release.
+Normal saves close exactly once. Failed temporary saves retain dirty flags and
+pause background saves for the session; manual saves after logical errors remain
+available. A manual save does not silently restart background retries or discard
+pending temporary-save flags. The production latch has no clear API.
 
 The existing Euclidean tests have only extraction/ownership mocks updated for
 the new helper dependencies and FatFs open/close ownership contract; no assertions
diff --git a/tests/save_errors/host.c b/tests/save_errors/host.c
index 2696be7..c03a8fb 100644
--- a/tests/save_errors/host.c
+++ b/tests/save_errors/host.c
@@ -28,6 +28,7 @@ FIL SDFile;
 TIM_HandleTypeDef htim2;
 static FATFS host_fs;
 static FRESULT preset_sharedFailure;
+static uint8_t preset_autosavePaused;
 uint8_t preset_kitDirty,preset_patDirty;
 uint32_t preset_kitTimer,preset_patTimer;
 static BYTE output[65536],oracle[65536];
@@ -78,7 +79,7 @@ FRESULT delete_node(TCHAR *path,UINT size,FILINFO *info){(void)path;(void)size;(
 static void session(void)
 {
  /* Test-only simulated reboot/controller reset. There is no production clear. */
- memset(&SDFile,0,sizeof SDFile);preset_sharedFailure=FR_OK;
+ memset(&SDFile,0,sizeof SDFile);preset_sharedFailure=FR_OK;preset_autosavePaused=0;
  used=opens=writes=closes=fail_at=short_at=full_error_count=starts=stops=notifications=repaints=resets=directory_calls=0;
  open_error=close_error=last_notification=ram_error=FR_OK;write_error=FR_DENIED;
  preset_patDirty=preset_kitDirty=0;preset_patTimer=preset_kitTimer=0;
@@ -140,7 +141,8 @@ static void failures(void)
  unsigned positions=0;
  for(unsigned kind=0;kind<8;kind++){
   session();assert(save(kind,3,0)==FR_OK);unsigned count=writes;
-  session();open_error=FR_NO_PATH;assert(save(kind,3,0)==FR_NO_PATH && opens==1 && !writes && !closes && !SDFile.obj.fs && starts==stops);quarantined(FR_NO_PATH);
+  session();open_error=FR_NO_PATH;assert(save(kind,3,0)==FR_NO_PATH && opens==1 && !writes && !closes && !SDFile.obj.fs && starts==stops);
+  assert(preset_sharedFailure==FR_OK);open_error=FR_OK;assert(save(kind,3,0)==FR_OK && opens==2 && closes==1);
   session();open_error=FR_DISK_ERR;assert(save(kind,3,0)==FR_DISK_ERR && !writes && !closes);quarantined(FR_DISK_ERR);
   for(unsigned pos=1;pos<=count;pos++)for(unsigned mode=0;mode<4;mode++){
    session();close_error=FR_NOT_READY;
@@ -179,12 +181,28 @@ static void ui_and_background(void)
   assert(starts==stops);
  }
  session();preset_patDirty=preset_kitDirty=1;short_at=1;preset_processTimer();
- assert(notifications==1 && opens==1 && !preset_patDirty && !preset_kitDirty);
- preset_patDirty=preset_kitDirty=1;preset_processTimer();assert(notifications==1 && opens==1 && !preset_patDirty && !preset_kitDirty);
+ assert(notifications==1 && opens==1 && preset_patDirty && preset_kitDirty && preset_autosavePaused);
+ preset_patDirty=preset_kitDirty=1;preset_processTimer();assert(notifications==1 && opens==1 && preset_patDirty && preset_kitDirty);
  session();preset_patDirty=preset_kitDirty=1;preset_processTimer();assert(!notifications && opens==2 && closes==2 && !preset_patDirty && !preset_kitDirty);
- session();preset_kitDirty=1;short_at=2;preset_processTimer();assert(notifications==1 && opens==1 && !preset_kitDirty);
- session();preset_patDirty=1;open_error=FR_NO_PATH;preset_processTimer();assert(notifications==1 && opens==1 && !preset_patDirty);preset_processTimer();assert(opens==1);
+ session();preset_kitDirty=1;short_at=2;preset_processTimer();assert(notifications==1 && opens==1 && preset_kitDirty && preset_autosavePaused);
+ session();preset_patDirty=1;open_error=FR_NO_PATH;preset_processTimer();assert(notifications==1 && opens==1 && preset_patDirty && preset_autosavePaused);preset_processTimer();assert(opens==1);
+ open_error=FR_OK;assert(preset_saveGlobals()==FR_OK && preset_patDirty && preset_autosavePaused);
  session();short_at=2;preset_createGlobals();assert(notifications==1 && last_notification==FR_DISK_ERR && writes==2 && !closes);
  puts("PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, stopped background retries; no UI/hardware execution");
 }
-int main(void){setvbuf(stdout,0,_IONBF,0);contents();equivalence();failures();ui_and_background();}
+static void logical_retries(void)
+{
+ const FRESULT retryable[]={FR_NO_FILE,FR_NO_PATH,FR_INVALID_NAME,FR_EXIST,FR_INVALID_DRIVE,
+  FR_NOT_ENABLED,FR_NO_FILESYSTEM,FR_WRITE_PROTECTED,FR_LOCKED,FR_TOO_MANY_OPEN_FILES,FR_INVALID_PARAMETER,FR_NOT_READY,FR_DENIED};
+ for(unsigned k=0;k<8;k++)for(unsigned e=0;e<sizeof retryable/sizeof *retryable;e++){
+  session();contents();BYTE before[32];memcpy(before,activeParameters,sizeof before);
+  memcpy(output,"Existing valid bytes",20);used=20;
+  open_error=retryable[e];
+  for(unsigned n=0;n<9;n++)assert(save(k,2,0)==open_error && !writes && !closes && !SDFile.obj.fs && preset_sharedFailure==FR_OK);
+  assert(used==20 && !memcmp(output,"Existing valid bytes",20) && !memcmp(before,activeParameters,sizeof before));
+  open_error=FR_OK;used=0;assert(save(k,2,0)==FR_OK && opens==10 && closes==1);
+ }
+ session();open_error=FR_TIMEOUT;assert(preset_saveGlobals()==FR_TIMEOUT);quarantined(FR_TIMEOUT);
+ puts("PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held");
+}
+int main(void){setvbuf(stdout,0,_IONBF,0);contents();equivalence();failures();ui_and_background();logical_retries();}
diff --git a/tests/save_errors/run.py b/tests/save_errors/run.py
index 9a900a3..71616b3 100644
--- a/tests/save_errors/run.py
+++ b/tests/save_errors/run.py
@@ -51,6 +51,10 @@ def main():
     for name in ['preset_createProject','preset_saveProject','preset_loadProject','preset_loadProjectNames','loadTmpFolderFiles','preset_makeFileNameLoad']:
         body=extract(text,name);assert 'preset_sharedStatus()' in body[:body.index('{')+180]
     assert 'f_close(' not in extract(text,'preset_finishSave')
+    project=extract(text,'preset_saveProject')
+    start=project.index('res = preset_saveGlobals();');end=project.index('//save all drumkits',start)
+    assert 'if(res != FR_OK)' in project[start:end] and 'return res;' in project[start:end]
+    assert 'preset_clearTmpFolder' not in project[start:end]
     includes=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include',
               'Drivers/STM32F7xx_HAL_Driver/Inc','Middlewares/Third_Party/FatFs/src','FATFS/App','FATFS/Target']
     with tempfile.TemporaryDirectory(prefix='lxr-save-errors-') as directory:

```

## Appendix B — Exact baseline and final command logs

Both baseline and final logs are retained. The build output's generic installation instructions are evidence only; they were not executed or authorized. Final validation supersedes prototype runs described in section11.

### baseline-00.log

```text
$ cmake --version
cmake version 3.31.10

CMake suite maintained and supported by Kitware (kitware.com/cmake).

EXIT 0
```

### baseline-01.log

```text
$ ninja --version
1.13.0.git.kitware.jobserver-pipe-1

EXIT 0
```

### baseline-02.log

```text
$ python3 --version
Python 3.12.14

EXIT 0
```

### baseline-03.log

```text
$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (15:14.2.rel1-1) 14.2.1 20241119
Copyright (C) 2024 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


EXIT 0
```

### baseline-04.log

```text
$ python3 tests/bug001/run.py
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence

EXIT 0
```

### baseline-05.log

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

### baseline-06.log

```text
$ python3 tests/safety/run.py
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state

EXIT 0
```

### baseline-07.log

```text
$ python3 tests/usb_allocator/run.py
PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing
PASS USB failures: 14 rollback/retention/retry scenarios plus invalid requests; no allocator calls
Host object: size=552 alignment=8 (target size/alignment checked separately in ELF)
PASS complete production USB class object: no heap/allocator-lock references

EXIT 0
```

### baseline-08.log

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

### baseline-09.log

```text
$ python3 tests/sd_hal/run.py
PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP
PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap
PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset
PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation

EXIT 0
```

### baseline-10.log

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

### baseline-11.log

```text
$ python3 tests/save_errors/run.py
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical names/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, stopped background retries; no UI/hardware execution

EXIT 0
```

### baseline-12.log

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

### baseline-13.log

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

### baseline-14.log

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

### baseline-15.log

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

### baseline-16.log

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### baseline-17.log

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

### baseline-18.log

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### baseline-19.log

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

### baseline-20.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0x8B", "configuration": "release", "elf_sha256": "08234d93e804e81eecd0ddbf6f77194ec672b930515996bf1166f1f6d9700a03", "image_bytes": 277452, "image_sha256": "3ba0f3f1d455712858f5d7c551b7528d6166a13f556fd04071fc27aba791545e", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224572, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 277436, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x20", "configuration": "debug", "elf_sha256": "da108c8feb0ec6bee35e04d502dd95b3b3aed6b7e9242bd3c1ed418e8ccf1cf9", "image_bytes": 277500, "image_sha256": "6284756fda982152eef935a78d991c3a63741fb0cb4cb7f38696baf51a91aa98", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224620, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 277484, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x07", "configuration": "diagnostics-release", "elf_sha256": "cd684374537d526ef201097d14796de59315486eef10cc8fac9187f227d7f255", "image_bytes": 277892, "image_sha256": "f416b68866f55b64be6b6e34e76c3f5664f931db55c74d8baa982ad531536b6d", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224940, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 277876, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xDF", "configuration": "diagnostics-debug", "elf_sha256": "3edd5fe5734854a71b483abd3c35a63b5ca403ed3c1b77de07006c7cb6dcd8ab", "image_bytes": 277940, "image_sha256": "6b2038462795ba0e7f9da0132935012cb927de61a420dff1fd83d9abbaafa0ed", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224988, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 277924, "reset_vector": "0x08014E4D", "result": "PASS"}

EXIT 0
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
$ python3 tests/sd_storage/run.py
PASS all 1..128 sector read/write counts; exact range edge and 5000ms HAL argument
PASS finite readiness, permanent busy deadline, tick wrap success/timeout; persistent reboot-only fault
PASS BSP read/write/init failure, ambiguous write blocks subsequent I/O; removal before/during transfer and reinsertion cannot clear latch
PASS CTRL_SYNC ready/delayed/timeout/fault and explicit BSP status error; status cannot clear or bypass fault
PASS invalid pointers/counts/LUN/overflowing ranges/ioctls; invalid input does not poison valid operation
PASS IRQ/masked/suspended tick rejection; no reset/remount/retry dependencies in linked driver

EXIT 0
```

### final-05.log

```text
$ python3 tests/sd_hal/run.py
PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP
PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap
PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset
PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation

EXIT 0
```

### final-06.log

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

### final-07.log

```text
$ python3 tests/save_errors/run.py
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical names/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, stopped background retries; no UI/hardware execution
PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held

EXIT 0
```

### final-08.log

```text
$ python3 tests/live_storage/run.py
PASS actual notification/tick/repaint bodies: queued/busy display, 1999/2000ms and wrap, latest error, all result codes, expiry/copy interleaving, page/RAM independence, no storage unlock or blocking calls
PASS complete production FatFs: nine missing-path/invalid-name/existing-file/readonly-target/full-root failures make zero disk writes, preserve all RAM media/valid file bytes, then explicit create/write/close succeeds

EXIT 0
```

### final-09.log

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

### final-10.log

```text
$ cmake --build --preset release --parallel 4
[1/3] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271584 B       384 KB     69.07%
           FLASH:        220 KB       992 KB     22.18%
   text	   data	    bss	    dec	    hex	filename
 276432	    852	 341120	 618404	  96fa4	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 277292 bytes, checksum 0x38
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

### final-11.log

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

### final-12.log

```text
$ cmake --build --preset debug --parallel 4
[1/3] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271584 B       384 KB     69.07%
           FLASH:      225328 B       992 KB     22.18%
   text	   data	    bss	    dec	    hex	filename
 276480	    852	 341120	 618452	  96fd4	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 277340 bytes, checksum 0x42
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

### final-13.log

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### final-14.log

```text
$ cmake --build build/diagnostics-release --parallel 4
[1/3] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271656 B       384 KB     69.09%
           FLASH:      225648 B       992 KB     22.21%
   text	   data	    bss	    dec	    hex	filename
 276872	    852	 341192	 618916	  971a4	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 277732 bytes, checksum 0x0D
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

### final-15.log

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### final-16.log

```text
$ cmake --build build/diagnostics-debug --parallel 4
[1/3] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271656 B       384 KB     69.09%
           FLASH:      225696 B       992 KB     22.22%
   text	   data	    bss	    dec	    hex	filename
 276920	    852	 341192	 618964	  971d4	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 277780 bytes, checksum 0x09
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

### final-17.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0x38", "configuration": "release", "elf_sha256": "faa871619f547c00a0c0ae9c98395dbb580819c14efb394f110e5a0d24d17e89", "image_bytes": 277308, "image_sha256": "b2fc09e17afe4e5fc3e7a0929ce60c368cd7a81d90a3f0300e5d5d8e3b10bf71", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224428, "ITCM_RAM": 15016, "RAM": 271584}, "payload_bytes": 277292, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x42", "configuration": "debug", "elf_sha256": "08169386236e26f6024ee533529c92b748391ee13c81031e4a5708b4088015d2", "image_bytes": 277356, "image_sha256": "bbfa527402ab823b8b1a9c53995a4263b2a64c0e1fa1f41704fdbc0cfd3f8558", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224476, "ITCM_RAM": 15016, "RAM": 271584}, "payload_bytes": 277340, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x0D", "configuration": "diagnostics-release", "elf_sha256": "6fa83eeddff95619163f213664b5518798289edec13dbed66490974d013c30e1", "image_bytes": 277748, "image_sha256": "6871ece11a6420eb93a0401e02693ca797b18607b3c41bdc2d0968d6f7328103", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224796, "ITCM_RAM": 15088, "RAM": 271656}, "payload_bytes": 277732, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x09", "configuration": "diagnostics-debug", "elf_sha256": "f61dc0e4901658f0807ea8e9095c70d15d530cd476d892ed507e3c33f6431ec3", "image_bytes": 277796, "image_sha256": "bea169680109f3c028c09de366f659dc1aa9fc8179354f924d5a38b0a2caf964", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224844, "ITCM_RAM": 15088, "RAM": 271656}, "payload_bytes": 277780, "reset_vector": "0x08014E4D", "result": "PASS"}

EXIT 0
```

## Appendix C — Final binary hashes and fixed state

Binaries remain local; hashes are identifiers, not installation artifacts.

| Configuration | .bin SHA-256 | Image first16 bytes |
|---|---|---|
|release|`637756f88a4e23e62333156dbf543994fd43bae333bdbd3a76d337f71be37d18`|`4c58525632494d472c3b040038000000`|
|debug|`78407131ff54fa0ff99e457a8ba105f313d4cd6cc3adbfca842d6de07575950b`|`4c58525632494d475c3b040042000000`|
|diagnostics-release|`b2dc70b20203627df7b91657d0225bda47595827311dd2c77849027120c20dfd`|`4c58525632494d47e43c04000d000000`|
|diagnostics-debug|`76f690b2d9007f0de0a0b1ef3891e1f6c288b0d27edeb244e163e115fcfc4a37`|`4c58525632494d47143d040009000000`|

Audio/codec/IRQ full-object comparison:

```text
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj UNCHANGED 7885756e6a7c84b5a8dcf8cb870bc6571480027eb25eaa6959b3be287f0693f1
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj UNCHANGED 06db7a571ad5abd61c30d6b74509e2a0c0e15326f0dc9cce6aff7c3cd13a4682
build/debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj UNCHANGED b3c0a151f657e4ed3438bdea9686705d729aa7e66dae1293d32f20e1af5e5a3c
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj UNCHANGED d6ee7b60ad6a2a1924e096b3631b8f38284349e3ad7c037c348e5ca3e7f17aca
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj UNCHANGED 10472838499bb9806639fc82153ba880797c3c496ac48e10e6724aeaa2e12ac5
build/release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj UNCHANGED 01463848afda54efbbeea116af34c9ff35db325406b42b003432ee46fba5c468
build/diagnostics-release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj UNCHANGED 66cd4b9de166101afc4fae3970b75e726495517a6de6957ca4b7ef72863fa5d6
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj UNCHANGED d1ed2c18ce02443fa1ed7ddae0633a61de9f31e39ba3882bb5f73378ad38dea2
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj UNCHANGED 46b4aa19065504b170ae148e4e6d0794856bd512529aa837a4d5e77479c5ab7d
build/diagnostics-debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj UNCHANGED 9c55bb9c71c07e4bd35ee221dfae028522f877dbae168836196cdd3498ea07c5
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj UNCHANGED 498f228e10b3e059fcf4a3033150897ebd12ee3005c4d4f9e8eedb78d9bc38a8
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj UNCHANGED 15922ba42e0b45f1193e5a83e721079d11b3ee0ada756c9113991b707611268c
```

Release fixed-state symbols (address, size bytes):

```text
20025e86 00000001 b menu_sdError
20025e84 00000001 b menu_sdErrorAuto
20025e85 00000001 b menu_sdErrorHeld
20025e80 00000004 b menu_sdErrorSince
20025ecc 00000001 b preset_autosavePaused
20025ecd 00000001 b preset_sharedFailure
```

The final source ref and all10 changed blobs must match the exact local tree. All27 pre-existing remote heads matched before report publication. The report is only this new Markdown file from lab-setup. Final post-creation exact-file/ref/tree verification is required before the success response. No report overwrite or source-branch merge is involved.
