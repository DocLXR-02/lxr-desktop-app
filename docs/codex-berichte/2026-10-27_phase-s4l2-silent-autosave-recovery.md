# LXR Firmware Lab — Phase S4-L2: Silent Autosave Recovery

Date: 2026-10-27 UTC. Repository: `DocLXR-02/lxr-desktop-app`.

**Status: IMPLEMENTED within the authorized autosave scope; software validation passed. Development review only. NOT FLASHED — NOT HARDWARE VALIDATED.** The result does not certify live audio continuity, transaction safety, power-loss persistence, S4b or U1.

## 1. Exact branches, commits and instructions

- Verified source baseline: `safety/sd-live-error-policy-s4l` at `feaff649599472036390ecef24f7193b7cc2e2ab`.
- Baseline tree: `0a5549eadfa98377abbc58d04629c2045641da0f`.
- New source branch: `safety/sd-silent-autosave-recovery-s4l2`.
- Final source commit: `910be804c7df2757f8d2347880751f2ca86c671c`.
- Final source tree: `80eb4b52ed9bb2532dd5eed1ccff70d6078c9549`; sole parent is the exact required S4-L commit. No merge.
- Source URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/safety/sd-silent-autosave-recovery-s4l2
- Report-only branch: `codex-reports/2026-10-27-phase-s4l2-silent-autosave-recovery`, based directly on `lab-setup` at `5829398cea5d337200393757e39a37d4d8148256`.
- Report path: `docs/codex-berichte/2026-10-27_phase-s4l2-silent-autosave-recovery.md`.
- Report URL: https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-27-phase-s4l2-silent-autosave-recovery/docs/codex-berichte/2026-10-27_phase-s4l2-silent-autosave-recovery.md

The existing `/workspace/lxr-desktop-app` checkout was clean at the expected S4-L commit; remote and local baseline matched. Ancestry to lab-setup was verified with `git merge-base --is-ancestor`. No second checkout/worktree was created. The complete remote lab-setup AGENTS.md was read (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`). The S1/S2/S3/S4a/S4-L reports and relevant implementation contracts were inspected.

Reference reports remain on their original branches. Blob identities fetched for this task:

| Report branch suffix under `codex-reports/` | Report blob |
|---|---|
| `2026-10-26-phase-s1-sd-storage-foundation` | `e1c646c3e588df061ed4df183e87e5822aaa9f79` |
| `2026-10-26-phase-s2-sd-hal-bsp-timeouts` | `9b44e0ef9bc1c8585e04fbe05249ab5fc691b81e` |
| `2026-10-26-phase-s3-fatfs-data-integrity` | `88ab6565c42881762d47f9be819f9c50ae5e3e76` |
| `2026-10-26-phase-s4a-safe-file-saving` (verified filename) | `15916ed38af3af63d65538ea8933d91de67a05dd` |
| `2026-10-27-phase-s4l-live-safe-storage-recovery` | `64db7ab625457a4876483b522e9483d2fa352eef` |

Source ancestry retains S1 `a4be3b94c552dbc867b197fad76655bed40cea34`, S2 `c88933a4b1968c8f9d173b71d86154e8969436be`, S3 `cbffa2d756658b6f32d8e6417ea40ec2cda828d5`, S4a `c87666ff945e2e01cbbf1ceda72fb503638de579` and the S4-L baseline, plus earlier Bug001/Euclidean/A/B/C corrections. All29 pre-existing remote heads were captured for preservation checks. Main and lab-setup were not modified.

Evidence labels: **CODE** = inspected production source; **BUILD** = executed host/build/image analysis; **HYPOTHESIS** = plausible unmeasured runtime effect; **OPEN** = future design/hardware validation required.

## 2. Objective and deliberately limited scope

Make recoverable background-save errors silent and recoverable without weakening the protection for incomplete/uncertain writes. Keep the original two-second post-edit delay and preserve pending RAM changes. Add only bounded scheduling in the existing foreground service; no new background task, timer interrupt, UI control, data format or recovery manager.

Only one production file changes: `Core/Src/Preset/presetManager.c`. Changes are the per-type retry state, silent scheduler, safe completion handling, healthy explicit-save re-arming and two absolute paths for the existing temporary files. Existing manual UI code, file data serializers, S1/S2/S3 drivers, audio, sequencer/MIDI/USB processing, bootloader/update/startup/linker/clocks/DMA/priorities remain unchanged. The new paths address the same normal on-disk files; no folder creation, migration or filename-layout change is introduced.

U1 is explicitly not implemented. The existing S4-L manual feedback (including its two-second overlay and explanatory labels) is unchanged. The agreed future U1 two-line manual success!/error!, at most one second and no RAM-vs-SD labels, remains separate. No autosave notification or progress display is added. Full project saving and individual-file transactions remain S4b work.

## 3. Baseline failure state and active context

**CODE, baseline presetManager.c:629:** `preset_processTimer()` returned immediately if the application failure latch or `preset_autosavePaused` was set. A due pattern save ran first; a successful result cleared its dirty flag. A due kit save followed only while the prior result was FR_OK. Any failure set `preset_autosavePaused=1` and called `menu_showSdError(res)`. Dirty flags survived, but a missing-path open failure disabled background saves for the session and demanded attention through the error overlay.

S4-L already distinguishes failed logical opens from uncertain writes: `preset_openShared` does not latch selected pre-create/truncate failures with no owner, while any write/short count or failed close latches `preset_sharedFailure`. No partial write is force-closed. S1's private physical Fault cannot be cleared by status, initialization or remount; S2 retains failed state/context rather than pretending READY; S3 makes relevant sync/close errors sticky and truthful.

**CODE, main.c:992:** the service is called from the main foreground loop, after regular rendering/MIDI/UI work. It is not called by audio or DMA interrupts. However edits, kit changes and sequencer start/stop can affect pending state while synchronous file I/O is executing: `preset_touchTimer` is reached from menu/front-panel/button and sequencer paths. It sets kit/pattern dirty and edit timestamps; PRESET_STOP_PAT_TIMER deliberately cancels the pending pattern timer when playback starts. Its exact S4-L body is preserved and hash-checked. No new per-edit counter or interrupt hook is added.

The old “save succeeded, then dirty=0” pattern could lose an edit arriving during the save or between a post-save check and a store. A timestamp-only comparison also misses same-millisecond edits. S4-L2 handles this in foreground completion without an interrupt mask or additional audio-ISR work.

## 4. Retry policy and state-transition table

Ordinary inactivity stays `PRESET_SAVE_TIME=2000ms`. Each type has its own `{at,wait}` record: pattern and kit are independent. After a safe rejected open, waits are2000,4000,8000,16000,32000ms, then remain32000ms. The cap is `PRESET_SAVE_TIME*16u`; no multiplication can grow past the defined power-of-two sequence. Cooldown starts after the synchronous attempt returns, not at its start.

Both conditions must hold before an attempt: elapsed since the latest edit >=2000ms, and elapsed since the last failed attempt >=its cooldown. `preset_touchTimer` changes neither cooldown timestamp nor backoff. Thus new edits can postpone an attempt through the ordinary inactivity delay, but cannot shorten/reset a cooldown into a retry storm. No SD call is made merely to check whether a cooldown condition has cleared.

These values are a conservative foreground scheduling policy derived from the existing two-second convenience delay, not SD programming deadlines or manufacturer-qualified timing. A persistent harmless error reaches at most one attempt per32s per type, plus actual I/O duration. There are at most two due type attempts in one service call; no retry loop. Unsigned HAL_GetTick subtraction supports the tested wraparound. The policy assumes the existing millisecond tick advances and does not claim to distinguish multiple unobserved49.7-day wraps, debugger halts or catastrophic interrupt starvation.

| State/event | Pending state | Next action |
|---|---|---|
| Normal/clean | dirty0; wait0 | Return without I/O or notification |
| New pending edit | dirty1, latest edit timestamp | Wait for ordinary2000ms inactivity and any existing cooldown |
| Due attempt | dirty becomes tagged nonzero2 before payload is read | Call current temporary writer once; no snapshot/history buffer |
| Safe rejected open | Owner null, no app failure; dirty remains pending | Schedule/increase that type's2/4/8/16/32s wait; remain silent; other due type may proceed |
| Cooldown plus more edits | Dirty remains pending; edit timestamp moves | Cooldown is unchanged; both gates must pass |
| Recovered, full write and successful close | Clear only an unchanged tagged dirty state | Reset failure backoff; return to ordinary scheduling |
| New edit during successful I/O/completion | New dirty1 retained | New current RAM state remains pending for ordinary inactivity; do not falsely claim it was backed up |
| Exclusive completion reservation lost | Nonzero pending state retained | On known file success, use ordinary2s spacing for a conservative follow-up, not immediate repeated writes or the old failure penalty |
| Pattern playback-start cancellation | Existing touch hook writes dirty0 | Preserve the deliberate cancellation; do not resurrect it as an edit |
| Cached volume unmounted or shared FIL owned | No file open attempted | Hold autosave silently; no lazy mount/reinitialization or forced owner release |
| Not-ready/mount/unknown/unsafe result | Global autosave hold; both pending types preserved | No automatic repeat; no second type after a serious result |
| Existing app/physical fault | Pending state preserved | Do not clear/bypass fault or initiate new unsafe writes |
| Fully successful explicit save, owner closed and app error FR_OK | Pending temporary flags/cooldowns untouched | May clear a check-required autosave hold; cannot produce success through a retained S1/app fault |

If pattern has a safe logical rejection, a due kit save can still succeed in that service call. Each type keeps its own state. A serious/check-required failure stops later autosave work globally because the shared file/volume safety contract cannot establish independent access. Safe failure handling never clears a dirty state. Explicit cancellation is a separate existing sequencer action, not a claimed successful save. It does not reset a retained cooldown; later pending work still respects that deadline.

## 5. Error classification: narrower than manual retry

`preset_autosaveCanRetry()` at657 first requires `preset_sharedFailure==FR_OK` and no `SDFile.obj.fs`. Its only eligible results are:

- `FR_NO_FILE`, `FR_NO_PATH`, `FR_INVALID_NAME`, `FR_EXIST`, `FR_WRITE_PROTECTED`.
- `FR_DENIED` only under `_USE_LFN==0 && !_FS_EXFAT` (the actual configuration).

For the actual temporary writers, these can only be rejected opens in that postcondition: all write/short-count/close failures set the application latch, even if an injected result numerically matches an open-safe error. An invalid pattern index returns FR_INVALID_PARAMETER and is not in the automatic whitelist. Manual S4-L classification stays broader; it is not replaced by this autosave list.

**CODE:** configured FatFs f_open follows/mounts the path and checks attributes/locks/existence before file creation/truncation. Its non-LFN dir_register requests one SFN entry; table/no-free-cluster logical denial differs from a propagated physical error. S4-L's complete FatFs tests cover rejected path/name/existing/read-only/full-root opens without media changes, and that suite is rerun. No generic FR_DENIED-after-write is treated as safe. No claim is generalized to LFN/exFAT or corrupt filesystem state.

**Held automatically:** FR_NOT_READY, NO_FILESYSTEM, NOT_ENABLED, INVALID_DRIVE, lock/too-many-owner errors, invalid-parameter/internal errors, DISK_ERR/TIMEOUT and unknown states. Some remain recoverable through explicit checking; the automatic scheduler does not probe, initialize or remount to find out. A latched S1 fault normally surfaces as NOT_READY before another physical operation, and that result does not enter a repeating autosave schedule. Existing S1/S2/S3 tests independently verify the low-level fail-closed contracts; host autosave mocks do not claim a physical end-to-end card test.

The new cached `SDFatFS.fs_type` check at the service entry rejects an unmounted volume before f_open can initiate FatFs lazy mounting. Main already eagerly mounts the actual SDFatFS object at startup and detected insertion (`main.c:367`, `:928`). No new f_mount, BSP init, status polling, reset, abort or recovery call is added. S1 still blocks physical initialization after a latched fault; S4-L2 does not expose a latch-clear API.

## 6. Dirty completion and edits during a save

Dirty fields at84–85 are now volatile bytes with internal meanings0 clean,1 pending edit,2 tagged pending attempt. Both nonzero values mean unsaved/pending. No other production consumer depends on equality to1. Each attempt tags its own flag before reading payload. The writers use current RAM as before; they do not retain an old payload from a failed attempt.

Completion at649 uses existing CMSIS Cortex-M7 byte-exclusive instructions:

```c
static uint8_t preset_completeAutosave(volatile uint8_t *dirty, FRESULT res)
{
    if(__LDREXB(dirty) != 2u) {
        __CLREX();
        return 1;
    }
    return __STREXB(res == FR_OK ? 0u : 1u, dirty) == 0u;
}
```

It runs only in foreground and makes a single store-exclusive attempt, with no mask, lock acquisition or spin loop. An edit writes1; existing pattern-start cancellation writes0. If either happened during I/O, the tag is not cleared over it. If an interrupt arrives after exclusive load, a lost reservation makes the store fail rather than erase the interrupt's update. An unrelated preemption can conservatively retain pending work even after a known successful file save; that case is deferred by the ordinary2s spacing. Successful completion resets the old failure penalty; a pending normal edit or reservation loss is not reported as a file error or a saved new snapshot.

The fields are ordinary CPU-owned SRAM, not DMA buffers; target ELF symbols and generated LDREXB/STREXB/CLREX are recorded below. Native tests replace only those intrinsics with deterministic reservation/interleaving seams. They do not physically test the M7 exclusive monitor or interrupt timing. No new work is placed in the edit hook or audio IRQ; twenty full objects compare unchanged.

This prevents lost pending edits; it does not make a multi-write file a transactionally coherent RAM snapshot while other contexts keep editing. A file may reflect interleaved values if edits occur during serialization. The new dirty state preserves a later resave of the current intended state after inactivity rather than falsely clearing it. No complete RAM copy/lock/history was introduced. More extensive snapshot/transaction guarantees remain separate design work.

## 7. Temporary target isolation and manual independence

**Confirmed audit finding:** the two old autosave opens used `tmp.snd` and `tmp.pat` relative to CWD. A retry can outlive a foreground project/menu operation that changes CWD. That would direct an otherwise safe attempt into another directory.

`preset_saveTmpKit:285` and `preset_saveTmpPat:555` now open `/tmp/tmp.snd` and `/tmp/tmp.pat`. These are the existing intended files under the normal `/tmp` working directory. The active configuration has one logical volume (`_VOLUMES=1`, `_FS_RPATH=1`), so the rooted paths select the same existing SD volume. The scheduler neither changes CWD nor creates a missing folder. Missing `/tmp` becomes a safely rejected path and the bounded schedule waits for the condition to be corrected through existing authorized operations. No existing project/file is deleted as recovery.

The frozen S3 writer oracle remains unchanged. Its relative filenames are resolved against the established `/tmp` context when checking equivalence. All successful bytes, lengths, write ordering/counts and final names match. A complete production FatFs RAM-media test changes CWD to `/proj`, seeds same-named project files, opens the actual paths extracted from production writers and verifies CWD, project entries and existing bytes remain intact. This is stronger than assuming the new literals resolve correctly in a name-only mock.

Manual save/load functions do not consult cooldowns. `preset_finishSave:160` still returns the first payload error without closing it; otherwise it checks close. Only after FR_OK, app error FR_OK and a closed/null owner does it clear the autosave hold. It leaves both dirty flags and retry records unchanged: saving globals/a named file does not prove that the pending temporary backup was saved.

This is re-arming, not clearing a fault. Retained app/file failure prevents the shared open/close contract from succeeding. S1's latched hardware fault prevents legitimate required writes/sync from succeeding. No failed manual operation becomes success and no autosave error changes a successful manual result. An explicit load alone is not used as write/sync availability evidence. Manual feedback, physical buttons, pages, focus/cursor/parameter selection and transport code are unchanged. The only autosave-specific `menu_showSdError()` call is removed.

## 8. Tests and actual results

The exact baseline was reproduced before editing: nine suites, four ARM configurations and four images PASS. Final validation runs ten suites and all four configurations. Every final test/configure/build/image command exits0.

| Command | Final result |
|---|---|
| `python3 tests/bug001/run.py` | PASS CC0/CC2/NRPN/mutes/Drive/bank, all256 Morph positions, restart/sweep/cadence |
| `python3 tests/euclidean/run.py` | PASS six ASan/UBSan groups;2176 masks,294912 playback ticks,4160 recording positions,130 roundtrips,10210 malformed cases,180802 protocol boundary calls |
| `python3 tests/safety/run.py` | PASS bounded heap/601 moves and diagnostics OFF/ON, wraps/saturation/budget/single HAL evaluation |
| `python3 tests/usb_allocator/run.py` | PASS2000 lifecycle cycles,14 failure cases, ownership/zeroing/no heap references |
| `python3 tests/sd_storage/run.py` | PASS1..128-sector cases, readiness/wrap/latch/init/status/sync/invalid contexts and no replay/reset/remount |
| `python3 tests/sd_hal/run.py` | PASS active production command/HAL/BSP bodies, timeout/state/error fidelity and no unsafe replay |
| `python3 tests/fatfs_integrity/run.py` | PASS configured/read-only complete units, FAT mirror/FSInfo, public sticky sync/close, S1 integration/init/mount/read |
| `python3 tests/save_errors/run.py` | PASS1536 S3 byte-oracle comparisons,2544 injected write positions, manual UI/first error/ownership and936 manual logical errors then104 successes |
| `python3 tests/live_storage/run.py` | PASS actual S4-L notification/tick/repaint/interleaving and full FatFs logical-open RAM-media tests |
| `python3 tests/silent_autosave/run.py` | PASS new autosave recovery/interleaving/target tests plus the existing save fixture |
| `python3 tests/save_errors/run.py --verify-baseline` | PASS additional verification that the frozen oracle exactly matches actual S3 Git bodies; logged separately |

New deterministic cases include:

1. Ordinary1999/2000ms inactivity, simultaneous due types and no success display.
2. Six eligible results, both types and normal/wrapping time:216 safe rejected opens and24 tenth-attempt successes. Exact2/4/8/16/32s boundary checks, including wrap during cooldown and repeated cap intervals.
3.4000 edit/tick calls during cooldown cause no retry storm; edit inactivity and failure cooldown remain independent. Current RAM changes are compared with a fresh S3 serializer after recovery.
4. Independent pattern failure and successful kit save; later pattern recovery does not clear a newly edited kit. A manual save during cooldown remains usable without claiming the temporary backup saved.
5. Same-tick edits injected into actual f_write and between the completion exclusive load/store remain pending. Unrelated reservation loss waits2s before a conservative follow-up. Explicit playback-start cancellation is not resurrected.
6. NOT_READY, ownership/lock/mount/invalid/unknown results and a cached unmounted volume do not repeat automatically. The unmounted case performs zero opens. No new card query or recovery call is used.
7. Short writes, payload errors and close failures never replay. An injected open-safe numeric result from a write or close is still unsafe because the actual application latch is set. A serious first result prevents the second pending type from starting.
8. A completed healthy explicit save re-arms a check-required hold; a retained app error cannot produce that success or be cleared. Pending dirty flags/cooldowns survive manual operations.
9. Mock notification/repaint/page counters remain zero/unchanged for autosave, including recovery/failure paths. Manual error checks still execute their existing UI path.
10. Complete production FatFs path resolution proves both actual autosave targets stay under `/tmp` while CWD is `/proj`, without altering that directory or same-named saved project data.

The save fixture extracts actual production writer/state/helper bodies and retry declarations; the new suite reuses it, not a separately rewritten state machine. File operations, tick, timers, UI counters and exclusive operations are controlled mocks. The path fixture includes complete ff.c/diskio.c/ff_gen_drv.c/sd_diskio.c and real structures with RAM sectors; seeded boot/dir/lock state is test setup only, not production formatting/recovery or forced lock release.

Pinned hashes cover unchanged manual menu/button, audio/IRQ, S1 source and the edit hook. Static tests reject waits/loops/notifications/allocation/remount/BSP polling/audio calls or interrupt-mask changes in the new helpers. The full ARM translation unit separately compiles the real intrinsics. Final executable disassembly contains byte exclusives; it is not a native-only workaround.

ASan/UBSan are fatal with no recovery. Shared native suite deadline40s; full path fixture20s. LeakSanitizer is disabled due to executor tracing and is not claimed as passed. No physical card/LCD/audio/interrupt behavior was simulated as a hardware result. S1/S2/S3 suites independently execute their actual lower-layer contracts; the scheduler mock does not claim a complete physical end-to-end card test.

Existing assertion changes are limited to intentionally replaced S4-L autosave pause/notification expectations, new dependencies and target resolution against `/tmp`. No payload/error/short-count/close/first-error/success-UI assertion was removed. The Euclidean fixture adds only the new referenced pause-state dependency/reset. The frozen baseline oracle/common digests are unchanged.

## 9. Toolchain, builds and image validation

Each build shell used:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

Tools: CMake3.31.10, Ninja1.13.0.git.kitware.jobserver-pipe-1, Python3.12.14, ARM GCC14.2.1 20241119 (`15:14.2.rel1-1`), native GCC14.2.0. Prepared tools were used; none purchased/installed. Debug retains the project's `-O3 -g3` configuration.

Executed on both baseline and final source:

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

All configurations/images PASS. Diagnostics remain OFF by default and OFF in normal build caches, ON only in isolated diagnostic directories. No CMake/preset/version change. Actual names remain `LXR_V2_V1.7.img`, `LXR_V2_STM32F765.bin`, `LXR_V2_STM32F765.elf`. Source version1.7 is not claimed equivalent to the manufacturer's lab reference public v1.71; private support1.72 is not the public reference. No official binary was examined here.

Validation checks16-byte `<8sIB3s` header, `LXRV2IMG`, LE payload count, zero padding, checksum `0xff-(sum(payload)&0xff)`, exact .bin payload identity, allocated/load-section memory bounds and plausible vector entries. Initial MSP0x20080000 and reset0x08014E4D Thumb are unchanged. Application Flash begins0x08008000 and excludes the first32KiB. Bootloader/update/options/clocks/startup/linker/DMA/priority settings are unmodified. Hardware protection state is not examined.

Images were built from the exact working-tree source/test bytes later committed as910be804c7df2757f8d2347880751f2ca86c671c; Git tree/blob equality was checked after publication. HEAD did not yet contain that API-created commit when the builds executed. All generated binaries stay local and are not in Git or uploaded. Build output prints generic installation instructions; they were not followed and confer no authorization.

## 10. Resource and performance impact

| Bytes | S4-L Release OFF | L2 Release OFF | S4-L Debug OFF | L2 Debug OFF | S4-L Release ON | L2 Release ON | S4-L Debug ON | L2 Debug ON |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| ITCM |15016|15016|15016|15016|15088|15088|15088|15088|
| DTCM |107392|107392|107392|107392|107392|107392|107392|107392|
| General SRAM |271584|271600|271584|271600|271656|271672|271656|271672|
| Linker FLASH used |225280|226592|225328|226640|225648|226960|225696|227008|
| Full .bin payload |277292|278604|277340|278652|277732|279044|277780|279092|
| Header+payload .img |277308|278620|277356|278668|277748|279060|277796|279108|

**BUILD: +1312bytes Flash/payload and +16bytes SRAM in every configuration; ITCM/DTCM unchanged.** The added fixed storage is exactly two8-byte retry records. Dirty flags remain one byte each; volatile qualifiers and internal tag add no storage. Existing pause flag is reused. No heap allocation, full payload copy, framebuffer/history or new stack array is added. The Flash increase is measured foreground scheduling/completion/re-arming code plus target strings; it was not concealed or traded for ITCM growth. No DSP/size optimization is added to offset it.

Remaining headroom: ITCM1368 normal/1296 diagnostics; DTCM23680 before runtime stack; SRAM121616 normal/121544 diagnostics. Full application Flash capacity1015808 leaves payload headroom737204/737156/736764/736716 bytes respectively. Validator Flash VMA high-water225740/225788/226108/226156 differs from linker FLASH used and complete Flash load span because initialized ITCM/DTCM/data occupy Flash too.

Existing minimum heap0x200/stack0x400 linker reservations remain; they do not prove worst-case runtime stack consumption. Added helpers use scalar/pointer state; no stack/heap high-water or real-time CPU measurement was performed. Core code stays outside ITCM. Twenty complete objects (Soundengine, codec, IRQ, menu, button handler across four modes) match captured baseline hashes. Edit-hook source is unchanged. An additional full baseline translation-unit compile using the actual Release compile command confirms its72-byte preset_touchTimer target section and relocation records are identical; the volatile qualifiers add no edit-hook instructions. This object-level check does not claim unchanged final RAM addresses/cache behavior or measured timing. No regular audio-ISR or per-edit work is added; only foreground service gates/attempt/completion run.

Synchronous FatFs operations still can delay foreground USB/MIDI/UI and other main-loop servicing. Existing TIM2 rendering/sequence/UART work and save start/stop hooks are retained, but that does not prove uninterrupted audio. S1's5000ms transfer/readiness policies and nested S2 command/STOP/initialization waits are not a universal end-to-end deadline. HAL tick availability, SD programming latency, IRQ scheduling, stack peaks and actual audio reliability remain hardware questions. DWT/CYCCNT is untouched.

## 11. Errors, limitations and remaining intervention cases

- Initial shell access occurred while the managed environment was starting; connector verification continued and the same existing checkout became available. No replacement checkout was created.
- First full final-validation pass stopped at Euclidean host compilation: its extracted preset_finishSave now referenced preset_autosavePaused, absent from that fixture. Added only its test-state declaration/reset; no production code or assertions were weakened. The failed command/output is preserved below, followed by successful complete reruns.
- Cached mount and absolute-target checks were added during pre-publication review to avoid lazy mount and redirected CWD retries. All ten suites/builds/images were rerun after the final production changes. Later stricter static-test checks also passed.
- Existing FatFs native registration emits an unchanged unused-lun warning; suites exit0. No sanitizer/assertion was disabled to hide it. Full final diff whitespace check passes; inherited frozen-oracle whitespace is unchanged.
- The one-shot completion primitive can conservatively retain work after a successful file if its reservation is lost to an interrupt. It uses ordinary2s spacing, not a fast retry loop; runtime frequency/overhead has not been measured. Native seams validate the state policy, not actual M7 timing.
- Same-tick edits/cancellation are covered, but no atomic full-file snapshot is promised during ongoing edits. A later stable save uses current RAM.

Reboot/media intervention remains necessary where the S1 physical latch or app/file uncertainty prevents a legitimate successful close/save. No auto recovery method clears them. Reboot is a recovery boundary, not proof that a card is repaired or previous file intact. Check-required mount/owner/invalid-state holds can be re-armed by a genuinely completed explicit save after the condition is resolved; an explicit load alone does not prove write/sync readiness. Some such cases still need controlled manual recovery or reboot outside this task.

File replacement is still non-transactional: existing `FA_CREATE_ALWAYS` can truncate an old temporary/named file before a later failure. No automatic replay follows an uncertain failure, but the prior valid file is not restored. Project saving can still empty an existing destination before replacement, ignore header short counts/close/cleanup results, and delete temporary recovery state too early. Existing project load/create/remount/format wrappers retain their own risks. Absolute autosave targets do not certify that lifecycle or remove pre-existing false-success paths outside corrected individual saves.

No physical card, LXR, LCD, audio glitch, actual timeout, CPU load, thermal behavior, crash recovery or power-loss behavior was measured. All media writes in host tests are RAM/mock operations. No firmware was flashed or installation image published.

## 12. Readiness and recommended next steps

- **CODE/BUILD:** recoverable autosave open failures are silent, bounded and eventually recover without interaction once their conditions clear. Dirty states/types/cooldowns remain distinct; no aggressive reset from edits.
- **CODE/BUILD:** uncertain writes/close/owner/not-ready states do not automatically repeat; app/S1/S2/S3 protections remain. Completed explicit-save evidence re-arms only a healthy check-required hold and never marks the pending temporary file saved.
- **CODE/BUILD:** no autosave page/focus/cursor/transport/parameter change or popup; manual error/result handling remains. Ten suites, four builds and four images pass; static memory fits and twenty protected objects are unchanged.
- **OPEN:** synchronous responsiveness and actual M7 exclusive/IRQ timing need future authorized physical measurement. Builds do not prove live musical continuity or authorize deployment.
- **OPEN:** proceed to reviewed S4b non-destructive project/individual replacement and shared volume/recovery-state ownership. No speculative rename transaction/reset/format should be introduced merely to bypass retained faults.
- **OPEN:** implement U1 separately using the already agreed manual-only two-line success!/error! feedback, max1s, no RAM-vs-SD labels, existing project progress retained, no autosave messages. This phase deliberately leaves it untouched.

No hardware access/flashing, physical SD modification/format/deletion, automatic reset/watchdog, bootloader/updater/option-byte/startup/clock/linker/DMA/priority change, merge/force-push or main/lab-setup modification occurred. The source branch is development review only; hardware testing requires separate explicit approval and a recovery plan.

## 13. Publication and verification

GitHub Git-data APIs created exact base64 blobs, a tree based on S4-L, a single-parent source commit and a new branch. The exact remote commit/tree was imported into the existing local branch, leaving the checkout clean. No previous source ref was moved.

The report is a new Markdown-only file on a separate report-only branch based directly on lab-setup. Its own commit SHA is read from the final branch head rather than embedded recursively. Before final success response, fetch its exact GitHub URL and compare full text; verify source/report heads, source parent/tree/blobs, the single added report-file diff and preservation of all29 pre-existing remote heads. This verification occurs after creating this immutable report commit; no report overwrite is needed.


## Appendix A — Complete file manifest and source/test diff

10 files;379 insertions,41 deletions. One production file, existing test seams/docs and a new focused suite. No binaries/build directories.

- `Core/Src/Preset/presetManager.c` — blob `5cd9154403f2b9bea5d44b1d408d479a51219358`.
- `tests/euclidean/host.c` — blob `89321b1028b203bde5966c4b5f7ec84ed0eb7670`.
- `tests/save_errors/README.md` — blob `bc31283e6e04b9dc51788f5bc340eca95e2c04a5`.
- `tests/save_errors/host.c` — blob `af32ca3d54643149381f61dc1a6ca652f42f92ae`.
- `tests/save_errors/run.py` — blob `7afa39d4ade6f8a92735012d31baeef365e77360`.
- `tests/silent_autosave/README.md` — blob `95b876d226efcf7131858181a324897a9a796c31`.
- `tests/silent_autosave/baseline_sources.json` — blob `f77869d97a035b58b1143c38ced199b79e24aae0`.
- `tests/silent_autosave/host.c` — blob `f57300a7b2ca76235b5e527e6747c2f712a0f684`.
- `tests/silent_autosave/paths.c` — blob `7bf70aeaeac8c971d577bce94d7ddf351a185463`.
- `tests/silent_autosave/run.py` — blob `ea0ddf97ecc28b8be3bb1eea8dade68604d989bc`.

```diff
diff --git a/Core/Src/Preset/presetManager.c b/Core/Src/Preset/presetManager.c
index ad18c40..5cd9154 100644
--- a/Core/Src/Preset/presetManager.c
+++ b/Core/Src/Preset/presetManager.c
@@ -81,15 +81,23 @@ static void preset_makeFileNameSave(char *buf, uint8_t num, uint8_t type, char *
 uint32_t preset_kitTimer = 0;
 uint32_t preset_patTimer = 0;
 
-uint8_t preset_kitDirty = 0;
-uint8_t preset_patDirty = 0;
+volatile uint8_t preset_kitDirty = 0;
+volatile uint8_t preset_patDirty = 0;
 
 /* One shared FIL: uncertain writes/closes are quarantined until reboot.
  * Never clear FIL.err or close after an incomplete write: close could flush more data.
  */
 static FRESULT preset_sharedFailure = FR_OK;
-/* Pending RAM changes survive failure; no background retry after an error. */
+/* Only unsafe autosave failures pause the session; safe open failures back off. */
 static uint8_t preset_autosavePaused;
+/* Autosave retry state: foreground only, independent of edit inactivity timers. */
+typedef struct {
+    uint32_t at;
+    uint32_t wait;
+} PresetAutosaveRetry;
+static PresetAutosaveRetry preset_patRetry, preset_kitRetry;
+#define PRESET_AUTOSAVE_MAX_WAIT (PRESET_SAVE_TIME * 16u)
+/* End autosave retry state. */
 uint8_t preset_autosaveIsPaused(void)
 {
     return preset_autosavePaused;
@@ -151,7 +159,15 @@ static FRESULT preset_writeChecked(const void *data, UINT count)
 }
 static FRESULT preset_finishSave(FRESULT res)
 {
-    return res == FR_OK ? preset_closeShared((FIL*)&SDFile) : res;
+    if(res != FR_OK) return res;
+    res = preset_closeShared((FIL*)&SDFile);
+    /* A completed save can re-arm check-required autosave holds, never a fault:
+     * unresolved app/physical faults prevent this contract from succeeding.
+     * Keep dirty state and cooldowns: this may not be a temporary-file save.
+     */
+    if(res == FR_OK && preset_sharedFailure == FR_OK && !SDFile.obj.fs)
+        preset_autosavePaused = 0;
+    return res;
 }
 static FRESULT preset_writePatternData(uint8_t patternNr);
 
@@ -272,7 +288,7 @@ FRESULT preset_saveTmpKit()
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
     HAL_TIM_Base_Start_IT(&htim2);
 #endif
-    res = preset_openShared((FIL*)&SDFile, "tmp.snd", FA_CREATE_ALWAYS | FA_WRITE);
+    res = preset_openShared((FIL*)&SDFile, "/tmp/tmp.snd", FA_CREATE_ALWAYS | FA_WRITE);
     if(res == FR_OK) {
         res = preset_writeChecked(&menu_selectedPresetNr[SAVE_TYPE_KIT], 1);
         if(res == FR_OK) res = preset_writeChecked(activeParameters, END_OF_SOUND_PARAMETERS);
@@ -543,7 +559,7 @@ FRESULT preset_saveTmpPat()
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
     HAL_TIM_Base_Start_IT(&htim2);
 #endif
-    res = preset_openShared((FIL*)&SDFile, "tmp.pat", FA_CREATE_ALWAYS | FA_WRITE);
+    res = preset_openShared((FIL*)&SDFile, "/tmp/tmp.pat", FA_CREATE_ALWAYS | FA_WRITE);
     if(res == FR_OK) {
         res = preset_writeChecked(&menu_playedPattern, 1);
         if(res == FR_OK) res = preset_writePatternData(menu_playedPattern);
@@ -626,26 +642,70 @@ FRESULT preset_clearTmpFolder()
 }
 
 
-void preset_processTimer()
+/* Clear only our pending tag, not an edit/cancellation delivered during I/O.
+ * A single exclusive attempt runs in foreground: no IRQ mask, spin or ISR work.
+ * If preempted, retain pending work and defer a known-success follow-up by 2 s.
+ */
+static uint8_t preset_completeAutosave(volatile uint8_t *dirty, FRESULT res)
 {
-    FRESULT res = FR_OK;
-    if(preset_sharedFailure != FR_OK || preset_autosavePaused) return;
-    if(preset_patDirty && HAL_GetTick() - preset_patTimer >= PRESET_SAVE_TIME) {
-        res = preset_saveTmpPat();
-        if(res == FR_OK) preset_patDirty = 0;
+    if(__LDREXB(dirty) != 2u) {
+        __CLREX();
+        return 1;
     }
-    if(res == FR_OK && preset_kitDirty && HAL_GetTick() - preset_kitTimer >= PRESET_SAVE_TIME) {
-        res = preset_saveTmpKit();
-        if(res == FR_OK) preset_kitDirty = 0;
+    return __STREXB(res == FR_OK ? 0u : 1u, dirty) == 0u;
+}
+static uint8_t preset_autosaveCanRetry(FRESULT res)
+{
+    if(preset_sharedFailure != FR_OK || SDFile.obj.fs) return 0;
+    /* These can only be rejected opens in the temporary writers: write/close
+     * errors latch above. NOT_READY, lock/owner and mount errors need checks.
+     */
+    switch(res) {
+    case FR_NO_FILE: case FR_NO_PATH: case FR_INVALID_NAME: case FR_EXIST:
+    case FR_WRITE_PROTECTED:
+#if _USE_LFN == 0 && !_FS_EXFAT
+    case FR_DENIED:
+#endif
+        return 1;
+    default:
+        return 0;
     }
-    if(res != FR_OK) {
-        /* Keep RAM dirty state; manual saves remain available after logical errors.
-         * Do not restart autosave on edits or notifications: no implicit replay.
-         */
+}
+static void preset_autosaveOne(volatile uint8_t *dirty, uint32_t edited,
+                               PresetAutosaveRetry *retry, uint8_t pattern)
+{
+    uint32_t now;
+    FRESULT res;
+    uint8_t completed;
+    if(!*dirty) return;
+    now = HAL_GetTick();
+    if((uint32_t)(now - edited) < PRESET_SAVE_TIME ||
+       (retry->wait && (uint32_t)(now - retry->at) < retry->wait)) return;
+    *dirty = 2u; /* Still pending, tagged before any payload is read. */
+    res = pattern ? preset_saveTmpPat() : preset_saveTmpKit();
+    completed = preset_completeAutosave(dirty, res);
+    now = HAL_GetTick(); /* Cooldown begins after the synchronous attempt returns. */
+    if(res == FR_OK) {
+        retry->at = now;
+        retry->wait = completed ? 0u : PRESET_SAVE_TIME;
+    } else if(preset_autosaveCanRetry(res)) {
+        retry->at = now;
+        if(!retry->wait) retry->wait = PRESET_SAVE_TIME;
+        else if(retry->wait < PRESET_AUTOSAVE_MAX_WAIT) retry->wait *= 2u;
+    } else {
         preset_autosavePaused = 1;
-        menu_showSdError(res);
     }
 }
+void preset_processTimer()
+{
+    if(preset_sharedFailure != FR_OK || preset_autosavePaused) return;
+    if(!preset_patDirty && !preset_kitDirty) return;
+    /* Do not use a retry to initiate FatFs lazy mounting/reinitialization. */
+    if(!SDFatFS.fs_type || SDFile.obj.fs) { preset_autosavePaused = 1; return; }
+    preset_autosaveOne(&preset_patDirty, preset_patTimer, &preset_patRetry, 1);
+    if(preset_autosavePaused) return;
+    preset_autosaveOne(&preset_kitDirty, preset_kitTimer, &preset_kitRetry, 0);
+}
 
 
 /*------------------------------------------------------------/
diff --git a/tests/euclidean/host.c b/tests/euclidean/host.c
index 16ac85d..89321b1 100644
--- a/tests/euclidean/host.c
+++ b/tests/euclidean/host.c
@@ -40,6 +40,7 @@ TIM_HandleTypeDef htim2;
 FIL SDFile;
 static FATFS host_filesystem;
 static FRESULT preset_sharedFailure;
+static uint8_t preset_autosavePaused;
 #define STEPS_PER_PATTERN NUM_STEPS
 #define FEXT_PAT 1
 static uint8_t stream[65536], saved_stream[65536];
@@ -95,7 +96,7 @@ void led_setValue(uint8_t value,uint8_t led) { (void)value; (void)led; }
 #include "functions.inc"
 
 static void reset(void) {
-    memset(&SDFile,0,sizeof SDFile); preset_sharedFailure=FR_OK;
+    memset(&SDFile,0,sizeof SDFile); preset_sharedFailure=FR_OK; preset_autosavePaused=0;
     memset(&seq_patternSet,0,sizeof(seq_patternSet));
     for(unsigned p=0;p<NUM_PATTERN;p++) for(unsigned t=0;t<NUM_TRACKS;t++)
         seq_patternSet.length[p][t]=16;
diff --git a/tests/save_errors/README.md b/tests/save_errors/README.md
index 3b37aaa..bc31283 100644
--- a/tests/save_errors/README.md
+++ b/tests/save_errors/README.md
@@ -16,7 +16,7 @@ FatFs file operations, RAM-editor callbacks and UI notifications are mocks;
 this is not a physical card, LCD, audio or complete menu-state simulation.
 
 Coverage includes 1536 direct byte-for-byte comparisons with the S3 oracle:
-eight save variants, all 64 slots, Morph 0/127/255, identical filenames and write
+eight save variants, all 64 slots, Morph 0/127/255, identical resolved /tmp targets and write
 call counts. Every payload position is tested with an error, error plus full
 byte count, short successful write, and disk error. Open/close failures, invalid
 indices, balanced existing timer hooks, first-error retention, object ownership,
@@ -38,10 +38,13 @@ rejection are distinct from observed disk errors. Payload DENIED remains held.
 Any write failure/short write or failed close still latches shared-file uncertainty
 until reboot. Short writes map to FR_DISK_ERR. There is no close after incomplete
 write, FIL.err reset, automatic retry/reset/remount, or forced lock release.
-Normal saves close exactly once. Failed temporary saves retain dirty flags and
-pause background saves for the session; manual saves after logical errors remain
-available. A manual save does not silently restart background retries or discard
-pending temporary-save flags. The production latch has no clear API.
+Normal saves close exactly once. S4-L2 temporary saves retain dirty flags and retry only selected safe open
+failures, silently, with per-type 2/4/8/16/32-second cooldowns. Unknown/not-ready/
+ownership/write/close outcomes remain held. A fully successful explicit save can
+re-arm a check-required hold, without clearing a fault, dirty flag or cooldown.
+The production fault latches still have no clear API. The original S4-L session-
+pause assertions are updated only for the intentionally changed autosave policy;
+manual UI and every incomplete-write/close protection assertion remain.
 
 The existing Euclidean tests have only extraction/ownership mocks updated for
 the new helper dependencies and FatFs open/close ownership contract; no assertions
@@ -53,3 +56,7 @@ reboots and are not firmware recovery APIs.
 Complete project-directory replacement remains S4b work. The old destination
 emptying, unchecked project-header short write/close, ignored global-save result,
 and temp cleanup results are not certified safe by these individual-save tests.
+
+S4-L2 uses absolute /tmp paths for the two temporary writers so a retry cannot
+follow a changed foreground CWD. The frozen relative filenames are resolved
+against the established /tmp baseline context; file bytes and final names match.
diff --git a/tests/save_errors/host.c b/tests/save_errors/host.c
index c03a8fb..af32ca3 100644
--- a/tests/save_errors/host.c
+++ b/tests/save_errors/host.c
@@ -25,11 +25,12 @@ uint8_t menu_playedPattern,menu_activePage,menu_activePatternBank,lcd_process_di
 uint32_t sucessTimer;
 volatile uint32_t systick_ticks;
 FIL SDFile;
+FATFS SDFatFS;
 TIM_HandleTypeDef htim2;
 static FATFS host_fs;
 static FRESULT preset_sharedFailure;
 static uint8_t preset_autosavePaused;
-uint8_t preset_kitDirty,preset_patDirty;
+volatile uint8_t preset_kitDirty,preset_patDirty;
 uint32_t preset_kitTimer,preset_patTimer;
 static BYTE output[65536],oracle[65536];
 static size_t used;
@@ -37,16 +38,33 @@ static unsigned opens,writes,closes,fail_at,short_at,full_error_count,starts,sto
 static FRESULT open_error,write_error,close_error,last_notification,ram_error;
 static uint32_t now=3000;
 static char last_name[64],oracle_name[64];
+static const char *open_error_name;
+static void (*write_hook)(void);
+static void (*exclusive_hook)(volatile uint8_t *);
+static uint8_t lose_exclusive;
+static uint32_t host_ldrexb(volatile uint8_t *p){return *p;}
+static uint32_t host_strexb(uint8_t value,volatile uint8_t *p)
+{
+ if(exclusive_hook){void (*hook)(volatile uint8_t *)=exclusive_hook;exclusive_hook=0;hook(p);return 1;}
+ if(lose_exclusive){lose_exclusive=0;return 1;}
+ *p=value;return 0;
+}
+static void host_clrex(void){}
+#define __LDREXB host_ldrexb
+#define __STREXB host_strexb
+#define __CLREX host_clrex
 FRESULT f_open(FIL *f,const TCHAR *name,BYTE flags)
 {
  assert(f==&SDFile && !f->obj.fs);opens++;strcpy(last_name,name);
- if(open_error!=FR_OK)return open_error;
+ if(open_error!=FR_OK && (!open_error_name || !strcmp(open_error_name,name)))return open_error;
+ if(flags&FA_WRITE)used=0;
  assert(flags==(FA_CREATE_ALWAYS|FA_WRITE) || flags==FA_READ);
  f->obj.fs=&host_fs;f->err=0;f->fptr=0;return FR_OK;
 }
 FRESULT f_write(FIL *f,const void *data,UINT count,UINT *actual)
 {
  assert(f==&SDFile && f->obj.fs && used+count<=sizeof output);writes++;
+ if(write_hook){void (*hook)(void)=write_hook;write_hook=0;hook();}
  *actual=writes==short_at?count-1:count;
  if(writes==fail_at && !full_error_count)*actual=0;
  memcpy(output+used,data,*actual);used+=*actual;f->fptr+=*actual;
@@ -79,10 +97,12 @@ FRESULT delete_node(TCHAR *path,UINT size,FILINFO *info){(void)path;(void)size;(
 static void session(void)
 {
  /* Test-only simulated reboot/controller reset. There is no production clear. */
- memset(&SDFile,0,sizeof SDFile);preset_sharedFailure=FR_OK;preset_autosavePaused=0;
+ memset(&SDFile,0,sizeof SDFile);preset_sharedFailure=FR_OK;preset_autosavePaused=0;SDFatFS.fs_type=1;
  used=opens=writes=closes=fail_at=short_at=full_error_count=starts=stops=notifications=repaints=resets=directory_calls=0;
  open_error=close_error=last_notification=ram_error=FR_OK;write_error=FR_DENIED;
  preset_patDirty=preset_kitDirty=0;preset_patTimer=preset_kitTimer=0;
+ memset(&preset_patRetry,0,sizeof preset_patRetry);memset(&preset_kitRetry,0,sizeof preset_kitRetry);
+ open_error_name=0;write_hook=0;exclusive_hook=0;lose_exclusive=0;now=3000;
  menu_playedPattern=0;menu_loadedPresetNr[CURRENT_KIT]=63;menu_activePage=SAVE_PAGE;menu_activePatternBank=0;
  sucessTimer=123;systick_ticks=456;last_name[0]=0;
 }
@@ -118,11 +138,13 @@ static void equivalence(void)
    session();assert(save(kind,slot,1)==FR_OK);size_t expected=used;unsigned calls=writes;
    memcpy(oracle,output,used);strcpy(oracle_name,last_name);
    session();assert(save(kind,slot,0)==FR_OK && used==expected && writes==calls);
-   assert(!memcmp(output,oracle,used) && !strcmp(last_name,oracle_name));
+   assert(!memcmp(output,oracle,used));
+   if(kind==0 || kind==3){assert(!strcmp(last_name,kind==0?"/tmp/tmp.snd":"/tmp/tmp.pat"));assert(!strcmp(last_name+5,oracle_name));}
+   else assert(!strcmp(last_name,oracle_name));
    assert(opens==1 && closes==1 && !SDFile.obj.fs && starts==stops);cases++;
   }
  }
- printf("PASS %u byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical names/write counts and balanced timer hooks\n",cases);
+ printf("PASS %u byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical resolved /tmp targets/write counts and balanced timer hooks\n",cases);
 }
 static void quarantined(FRESULT expected)
 {
@@ -181,14 +203,14 @@ static void ui_and_background(void)
   assert(starts==stops);
  }
  session();preset_patDirty=preset_kitDirty=1;short_at=1;preset_processTimer();
- assert(notifications==1 && opens==1 && preset_patDirty && preset_kitDirty && preset_autosavePaused);
- preset_patDirty=preset_kitDirty=1;preset_processTimer();assert(notifications==1 && opens==1 && preset_patDirty && preset_kitDirty);
+ assert(notifications==0 && opens==1 && preset_patDirty && preset_kitDirty && preset_autosavePaused);
+ preset_patDirty=preset_kitDirty=1;preset_processTimer();assert(notifications==0 && opens==1 && preset_patDirty && preset_kitDirty);
  session();preset_patDirty=preset_kitDirty=1;preset_processTimer();assert(!notifications && opens==2 && closes==2 && !preset_patDirty && !preset_kitDirty);
- session();preset_kitDirty=1;short_at=2;preset_processTimer();assert(notifications==1 && opens==1 && preset_kitDirty && preset_autosavePaused);
- session();preset_patDirty=1;open_error=FR_NO_PATH;preset_processTimer();assert(notifications==1 && opens==1 && preset_patDirty && preset_autosavePaused);preset_processTimer();assert(opens==1);
- open_error=FR_OK;assert(preset_saveGlobals()==FR_OK && preset_patDirty && preset_autosavePaused);
+ session();preset_kitDirty=1;short_at=2;preset_processTimer();assert(notifications==0 && opens==1 && preset_kitDirty && preset_autosavePaused);
+ session();preset_patDirty=1;open_error=FR_NO_PATH;preset_processTimer();assert(notifications==0 && opens==1 && preset_patDirty && !preset_autosavePaused);preset_processTimer();assert(opens==1);
+ open_error=FR_OK;assert(preset_saveGlobals()==FR_OK && preset_patDirty && !preset_autosavePaused);
  session();short_at=2;preset_createGlobals();assert(notifications==1 && last_notification==FR_DISK_ERR && writes==2 && !closes);
- puts("PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, stopped background retries; no UI/hardware execution");
+ puts("PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, silent controlled background retries; no UI/hardware execution");
 }
 static void logical_retries(void)
 {
@@ -205,4 +227,4 @@ static void logical_retries(void)
  session();open_error=FR_TIMEOUT;assert(preset_saveGlobals()==FR_TIMEOUT);quarantined(FR_TIMEOUT);
  puts("PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held");
 }
-int main(void){setvbuf(stdout,0,_IONBF,0);contents();equivalence();failures();ui_and_background();logical_retries();}
+int main(void){setvbuf(stdout,0,_IONBF,0);contents();equivalence();failures();ui_and_background();logical_retries();return 0;}
diff --git a/tests/save_errors/run.py b/tests/save_errors/run.py
index 71616b3..7afa39d 100644
--- a/tests/save_errors/run.py
+++ b/tests/save_errors/run.py
@@ -23,7 +23,7 @@ def case(text,name,next_name):
     start=text.index('case '+name+':');end=text.index('case '+next_name+':',start)
     return text[start:end]
 def main():
-    parser=argparse.ArgumentParser();parser.add_argument('--verify-baseline',action='store_true');args=parser.parse_args()
+    parser=argparse.ArgumentParser();parser.add_argument('--verify-baseline',action='store_true');parser.add_argument('--autosave-regressions',action='store_true');args=parser.parse_args()
     snapshot=(HERE/'baseline_s3.inc').read_text()
     assert hashlib.sha256(snapshot.encode()).hexdigest()==(HERE/'baseline_s3.sha256').read_text().strip()
     if args.verify_baseline:
@@ -32,13 +32,14 @@ def main():
         print('PASS frozen oracle exactly matches pinned S3 production function bodies',flush=True)
     text=(ROOT/PRESET).read_bytes().decode('latin1').replace('\r\n','\n')
     funcs=['preset_sharedStatus','preset_openShared','preset_closeShared','preset_writeChecked',
-           'preset_finishSave','interpolate','preset_getMorphValue']+WRITERS+['preset_resetAllSaveTimers','preset_processTimer','preset_initTmpFolder','preset_clearTmpFolder','preset_makeFileNameLoad','preset_loadDefaultGlobals','preset_createGlobals']
+           'preset_finishSave','interpolate','preset_getMorphValue']+WRITERS+['preset_resetAllSaveTimers','preset_touchTimer','preset_completeAutosave','preset_autosaveCanRetry','preset_autosaveOne','preset_processTimer','preset_initTmpFolder','preset_clearTmpFolder','preset_makeFileNameLoad','preset_loadDefaultGlobals','preset_createGlobals']
     names='\n'.join('#define '+n+' baseline_'+n for n in WRITERS)
     unnames='\n'.join('#undef '+n for n in WRITERS)
     menu=(ROOT/'Core/Src/menu/menu.c').read_text()
     for name,digest in json.loads((HERE/'common_s3.json').read_text()).items():
         assert hashlib.sha256(extract(menu if name=='numtostrpu_2Bytes' else text,name).encode()).hexdigest()==digest
     source=extract(menu,'numtostrpu_2Bytes')+extract(text,'clearIllegalCharacters')+extract(text,'preset_makeFileNameSave')
+    source+=text[text.index('/* Autosave retry state:'):text.index('/* End autosave retry state. */')+len('/* End autosave retry state. */')]+'\n'
     source+=''.join(extract(text,n) for n in funcs)
     source+='\n'+names+'\n#include "baseline_s3.inc"\n'+unnames+'\n'
     menu=(ROOT/'Core/Src/menu/menu.c').read_text()
@@ -82,6 +83,7 @@ def main():
                '-fno-pie','-no-pie','-ffunction-sections','-fdata-sections']
         flags+=['-I'+str(ROOT/p) for p in includes]+['-I'+str(directory),'-I'+str(HERE)]
         binary=directory/'host';env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
-        subprocess.run(flags+[str(HERE/'host.c'),'-Wl,--gc-sections','-o',str(binary)],cwd=ROOT,check=True,env=env)
+        host=ROOT/'tests/silent_autosave/host.c' if args.autosave_regressions else HERE/'host.c'
+        subprocess.run(flags+[str(host),'-Wl,--gc-sections','-o',str(binary)],cwd=ROOT,check=True,env=env)
         subprocess.run([str(binary)],cwd=ROOT,check=True,env=env,timeout=40)
 if __name__=='__main__':main()
diff --git a/tests/silent_autosave/README.md b/tests/silent_autosave/README.md
new file mode 100644
index 0000000..95b876d
--- /dev/null
+++ b/tests/silent_autosave/README.md
@@ -0,0 +1,50 @@
+# S4-L2 silent autosave tests
+
+Run `python3 tests/silent_autosave/run.py` with the prepared ARM toolchain on PATH.
+The runner uses the same production-body extraction and fatal ASan/UBSan fixture as
+`tests/save_errors/run.py --autosave-regressions`. It runs all existing checked-save
+cases before the new silent-autosave cases. There is no rewritten retry algorithm.
+Production retry declarations and all four helpers are extracted too. Macros replace
+only CMSIS byte-exclusive operations for native execution; deterministic callbacks
+model edits/cancellation during payload or between exclusive load/store, and loss
+of the exclusive reservation. The actual ARM implementation is compiled separately.
+No physical interrupt, card, LCD or audio execution occurs in these host tests.
+
+Coverage: ordinary 1999/2000ms inactivity; six selected open failures for kit and
+pattern at normal/wrapping ticks; 216 failed opens and 24 tenth-attempt successes;
+2/4/8/16/32s capped waits; 4000 new edits during cooldown without storms; current-RAM
+serialization; independent pending types; manual operations during cooldown;
+same-tick edits during I/O/completion; one-shot reservation loss and spacing;
+explicit pattern-start cancellation; check-required/unsafe holds; short/payload/
+close failures without replay; successful explicit-save re-arming only after
+healthy closure; zero autosave UI calls. Existing S3 byte oracle/2544 injected
+writes and manual S4-L retry/UI cases remain.
+
+Pinned hashes protect unchanged manual-feedback, audio/IRQ, S1 sources and the
+edit notification hook from scope drift. Static checks reject loops, UI, allocation,
+remounts, BSP polling and audio calls in the new helpers. Full ARM objects and
+firmware images are checked during the task; source hashes are not runtime timing
+measurements. Native tests use mocked file APIs; established S1/S2/S3 suites still
+exercise their production layers independently, including public sync/close faults.
+LeakSanitizer is disabled for executor tracing; ASan/UBSan remain fatal. Native
+execution retains the existing 40-second deadline.
+
+A dirty tag remains nonzero during synchronous I/O. A single foreground exclusive
+store clears only that tag after full success, never a concurrent edit/cancellation.
+No interrupt mask, lock/spin loop, per-edit generation hook, or extra audio ISR
+work is added. Lost reservations retain pending work with ordinary two-second
+spacing. Readiness/mount/ownership/invalid-state errors are intentionally not
+silently retried. A held S1 or shared-file fault cannot be cleared by this feature.
+
+Only the two autosave writes are rooted at the existing /tmp/tmp.snd and
+/tmp/tmp.pat destinations. Manual/project/load paths are untouched. A full
+production FatFs RAM-media test changes CWD to /proj, creates autosave output
+using paths extracted from the actual writers, and verifies existing project
+kit/pattern bytes and directory entries remain unchanged. The baseline oracle's
+relative names are compared at their established /tmp CWD; payload, final target
+and write counts remain equal. No folder creation, deletion or CWD mutation is
+added to the scheduler.
+
+A cached unmounted SDFatFS state is held without even f_open: autosave does not
+use retries to initiate FatFs lazy mounting or reinitialization. Existing manual
+mounting and S1 fault protection are unchanged.
diff --git a/tests/silent_autosave/baseline_sources.json b/tests/silent_autosave/baseline_sources.json
new file mode 100644
index 0000000..f77869d
--- /dev/null
+++ b/tests/silent_autosave/baseline_sources.json
@@ -0,0 +1,9 @@
+{
+  "Core/Src/menu/menu.c": "47ce3c8d52c87b0866f9cca6f0acbd79bd2161ef919c264a6f3cfa9ad017cf11",
+  "Core/Src/buttonHandler.c": "1aae6cc6a5a6ab1d762d1d42df50f77d894bc0397d8d1d5c578bd319f2e9eff1",
+  "Core/Src/stm32f7xx_it.c": "059988319962606810845a8be75f11d843c6f8d2f93060dd7424d0eba5383b4a",
+  "Core/Src/Soundengine/Soundengine.c": "900b27069d52d4e57e55f5e47a76aecaf6e271ac4f9f0a145b0fd25ffe0a2928",
+  "Core/Src/Soundengine/cs4344_cs5343.c": "b4f0331fc196dacd41a0ccd76507a7775d45340db224670b1dfa9f66db45c34c",
+  "FATFS/Target/sd_diskio.c": "9faaf6dcb11d21d3442f9c9fa250f5cc676542d4509bb58deae17894e0c389de",
+  "preset_touchTimer": "da166fb5e3be2c23cd436716c20eced447a1e5de8a31670a44dd84eedba08683"
+}
diff --git a/tests/silent_autosave/host.c b/tests/silent_autosave/host.c
new file mode 100644
index 0000000..f57300a
--- /dev/null
+++ b/tests/silent_autosave/host.c
@@ -0,0 +1,119 @@
+/* Reuse the complete checked-writer fixture/extraction, not a retry approximation. */
+#define main existing_save_suite_main
+#include "../save_errors/host.c"
+#undef main
+static void silence(void)
+{
+ assert(!notifications && !repaints && !resets && menu_activePage==SAVE_PAGE);
+ assert(menu_loadedPresetNr[CURRENT_KIT]==63 && sucessTimer==123);
+ assert(starts==stops);
+}
+static void touch(unsigned pattern){preset_touchTimer(pattern?PRESET_PAT_TIMER:PRESET_KIT_TIMER);}
+static volatile uint8_t *pending(unsigned pattern){return pattern?&preset_patDirty:&preset_kitDirty;}
+static PresetAutosaveRetry *retry(unsigned pattern){return pattern?&preset_patRetry:&preset_kitRetry;}
+static void current_bytes(unsigned pattern)
+{
+ size_t size=used;memcpy(oracle,output,size);
+ assert((pattern?baseline_preset_saveTmpPat():baseline_preset_saveTmpKit())==FR_OK);
+ assert(used==size && !memcmp(oracle,output,size));
+}
+static void normal_and_recoverable(void)
+{
+ session();contents();now=100;touch(0);touch(1);now+=1999;preset_processTimer();assert(!opens);
+ now++;preset_processTimer();assert(opens==2 && closes==2 && !preset_kitDirty && !preset_patDirty);silence();
+ const FRESULT safe[]={FR_NO_PATH,FR_NO_FILE,FR_INVALID_NAME,FR_EXIST,FR_WRITE_PROTECTED,FR_DENIED};
+ unsigned failures_count=0;
+ for(unsigned pattern=0;pattern<2;pattern++)for(unsigned e=0;e<sizeof safe/sizeof *safe;e++)for(unsigned wrap=0;wrap<2;wrap++){
+  session();contents();now=wrap?UINT32_MAX-3000u:100u;touch(pattern);now+=2000u;open_error=safe[e];
+  uint32_t expected=2000;
+  for(unsigned n=0;n<9;n++){
+   unsigned before=opens;preset_processTimer();assert(opens==before+1 && !writes && !closes);
+   assert(*pending(pattern) && !preset_autosavePaused && preset_sharedFailure==FR_OK);
+   assert(retry(pattern)->wait==expected);failures_count++;
+   now+=expected-1;preset_processTimer();assert(opens==before+1);now++;
+   if(expected<32000u)expected*=2u;
+  }
+  /* All attempts serialize live RAM; no cached payload from the first failure. */
+  memset(activeParameters,0x5a,END_OF_SOUND_PARAMETERS);
+  seq_patternSet.seq_subStepPattern[0][0][0].note=73;
+  open_error=FR_OK;preset_processTimer();
+  assert(opens==10 && closes==1 && !*pending(pattern) && !retry(pattern)->wait);
+  silence();current_bytes(pattern);
+ }
+ printf("PASS silent recovery: %u safe failed opens then 24 tenth-attempt successes; independent types; 2/4/8/16/32s cap and tick wrap\n",failures_count);
+}
+static void edits_and_independence(void)
+{
+ session();contents();touch(0);now+=2000;open_error=FR_NO_PATH;preset_processTimer();
+ uint32_t failed=retry(0)->at;
+ for(unsigned i=0;i<4000;i++){now++;activeParameters[5]++;touch(0);preset_processTimer();assert(opens==1);}
+ assert(retry(0)->at==failed && retry(0)->wait==2000 && preset_kitDirty);
+ open_error=FR_OK;now+=1999;preset_processTimer();assert(opens==1);
+ now++;preset_processTimer();assert(opens==2 && !preset_kitDirty);silence();current_bytes(0);
+ session();contents();touch(0);touch(1);now+=2000;
+ open_error=FR_NO_PATH;open_error_name="/tmp/tmp.pat";preset_processTimer();
+ assert(opens==2 && preset_patDirty && !preset_kitDirty && preset_patRetry.wait==2000 && !preset_kitRetry.wait);
+ assert(preset_saveGlobals()==FR_OK && preset_patDirty && preset_patRetry.wait==2000); /* manual during cooldown */
+ now+=1000;touch(0);open_error=FR_OK;now+=1000;preset_processTimer();
+ assert(!preset_patDirty && preset_kitDirty && !preset_patRetry.wait);
+ now+=1000;preset_processTimer();assert(!preset_kitDirty);silence();
+ puts("PASS edits/cooldown independence: 4000 edit/tick calls cause no storm; current RAM retained; kit/pattern deadlines separate; manual save does not clear pending backups");
+}
+static void kit_edit(void){activeParameters[5]^=0x7f;touch(0);}
+static void pattern_edit(void){seq_patternSet.seq_subStepPattern[0][0][0].note^=0x3f;touch(1);}
+static void completion_edit(volatile uint8_t *dirty){if(dirty==&preset_kitDirty)kit_edit();else pattern_edit();}
+static void cancel_pattern(void){preset_touchTimer(PRESET_STOP_PAT_TIMER);}
+static void interleavings(void)
+{
+ for(unsigned pattern=0;pattern<2;pattern++)for(unsigned stage=0;stage<2;stage++){
+  session();contents();touch(pattern);now+=2000;
+  if(stage)exclusive_hook=completion_edit;else write_hook=pattern?pattern_edit:kit_edit;
+  preset_processTimer();assert(*pending(pattern) && closes==1 && !preset_autosavePaused);
+  unsigned before=opens;preset_processTimer();assert(opens==before);
+  now+=1999;preset_processTimer();assert(opens==before);
+  now++;preset_processTimer();assert(!*pending(pattern) && closes==2);silence();current_bytes(pattern);
+ }
+ session();contents();touch(0);now+=2000;lose_exclusive=1;preset_processTimer();
+ assert(preset_kitDirty && preset_kitRetry.wait==2000);
+ for(unsigned i=0;i<100;i++)preset_processTimer();assert(opens==1);
+ now+=2000;preset_processTimer();assert(!preset_kitDirty && closes==2);silence();
+ session();contents();touch(1);now+=2000;write_hook=cancel_pattern;preset_processTimer();
+ assert(!preset_patDirty && !preset_autosavePaused);now+=32000;preset_processTimer();assert(opens==1);silence();
+ puts("PASS production completion logic: same-tick edits during payload and exclusive store survive; reservation-loss cooldown; explicit pattern-start cancellation preserved; no ISR edit hook added");
+}
+static void unsafe_holds(void)
+{
+ const FRESULT check[]={FR_NOT_READY,FR_LOCKED,FR_TOO_MANY_OPEN_FILES,FR_NO_FILESYSTEM,FR_NOT_ENABLED,FR_INVALID_DRIVE,FR_INVALID_PARAMETER,FR_DISK_ERR,FR_INT_ERR,FR_TIMEOUT};
+ for(unsigned pattern=0;pattern<2;pattern++)for(unsigned e=0;e<sizeof check/sizeof *check;e++){
+  session();touch(pattern);now+=2000;open_error=check[e];preset_processTimer();
+  assert(*pending(pattern) && preset_autosavePaused && opens==1 && !writes);
+  for(unsigned i=0;i<10;i++){now+=32000;preset_processTimer();assert(opens==1);}silence();
+ }
+ for(unsigned pattern=0;pattern<2;pattern++)for(unsigned failure=0;failure<4;failure++){
+  session();touch(0);touch(1);if(!pattern)preset_patDirty=0;now+=2000;
+  if(failure==0)short_at=1;
+  if(failure==1){fail_at=1;write_error=FR_NO_PATH;} /* Open-safe result is NOT write-safe. */
+  if(failure==2)close_error=FR_DISK_ERR;
+  if(failure==3)close_error=FR_NO_PATH;
+  preset_processTimer();assert(*pending(pattern) && preset_autosavePaused && preset_sharedFailure!=FR_OK);
+  if(pattern)assert(preset_kitDirty && opens==1);
+  unsigned before=opens,write_before=writes,close_before=closes;
+  for(unsigned i=0;i<100;i++){now+=32000;preset_processTimer();}
+  assert(opens==before && writes==write_before && closes==close_before);
+  assert(preset_saveGlobals()!=FR_OK && preset_autosavePaused);silence();
+ }
+ session();touch(0);now+=2000;SDFile.obj.fs=&host_fs;preset_processTimer();
+ assert(preset_autosavePaused && preset_kitDirty && !opens);silence();
+ session();touch(0);now+=2000;SDFatFS.fs_type=0;preset_processTimer();
+ assert(preset_autosavePaused && preset_kitDirty && !opens);
+ for(unsigned n=0;n<10;n++){now+=32000;preset_processTimer();assert(!opens);}silence();
+ /* Completed explicit save can re-arm a check-required hold; no latch is cleared. */
+ session();contents();touch(0);now+=2000;open_error=FR_NOT_READY;preset_processTimer();assert(preset_autosavePaused);
+ open_error=FR_OK;assert(preset_saveGlobals()==FR_OK && !preset_autosavePaused && preset_kitDirty);
+ preset_processTimer();assert(!preset_kitDirty);silence();
+ puts("PASS unsafe/check-required holds: NOT_READY and ownership/mount failures do not loop; partial/write/close errors never replay; serious latches remain; completed explicit save re-arms only a healthy check-required hold");
+}
+int main(void)
+{
+ existing_save_suite_main();normal_and_recoverable();edits_and_independence();interleavings();unsafe_holds();return 0;
+}
diff --git a/tests/silent_autosave/paths.c b/tests/silent_autosave/paths.c
new file mode 100644
index 0000000..7bf70ae
--- /dev/null
+++ b/tests/silent_autosave/paths.c
@@ -0,0 +1,37 @@
+/* Full production FatFs; RAM-only geometry. Paths are extracted from actual writers. */
+#define main earlier_integrity_main
+#include "../fatfs_integrity/host.c"
+#undef main
+#include "paths.inc"
+static void dir_entry(BYTE *p,const char *name,BYTE attr,WORD cluster,DWORD size)
+{
+ memcpy(p,name,11);p[DIR_Attr]=attr;fixture_put16(p+DIR_FstClusLO,cluster);
+ p[DIR_FileSize]=(BYTE)size;
+}
+int main(void)
+{
+ setup();memset(Files,0,sizeof Files);FATFS mounted;memset(&mounted,0,sizeof mounted);
+ BYTE *boot=media[0];boot[0]=0xeb;boot[1]=0x3c;boot[2]=0x90;memcpy(boot+BS_FilSysType,"FAT16   ",8);
+ fixture_put16(boot+BPB_BytsPerSec,512);boot[BPB_SecPerClus]=1;fixture_put16(boot+BPB_RsvdSecCnt,1);
+ boot[BPB_NumFATs]=2;fixture_put16(boot+BPB_RootEntCnt,512);fixture_put16(boot+BPB_TotSec16,8192);
+ fixture_put16(boot+BPB_FATSz16,32);boot[BPB_Media]=0xf8;fixture_put16(boot+BS_55AA,0xaa55);
+ media[1][0]=0xf8;media[1][1]=media[1][2]=media[1][3]=0xff;
+ for(unsigned c=2;c<=6;c++)if(c!=4)fixture_put16(media[1]+2*c,0xffff);
+ memcpy(media[33],media[1],512);
+ dir_entry(media[65],"TMP        ",AM_DIR,2,0);dir_entry(media[65]+32,"PROJ       ",AM_DIR,3,0);
+ dir_entry(media[98],"TMP     SND",AM_ARC,5,8);dir_entry(media[98]+32,"TMP     PAT",AM_ARC,6,8);
+ memcpy(media[100],"kit kept",8);memcpy(media[101],"pat kept",8);
+ assert(FATFS_UnLinkDriver(drive)==0 && FATFS_LinkDriver(&fake_driver,drive)==0);
+ assert(f_mount(&mounted,drive,1)==FR_OK && f_chdir("/proj")==FR_OK);
+ BYTE directory[512];memcpy(directory,media[98],512);
+ for(unsigned kind=0;kind<2;kind++){
+  FIL f;memset(&f,0,sizeof f);UINT n;
+  assert(f_open(&f,actual_autosave_paths[kind],FA_CREATE_ALWAYS|FA_WRITE)==FR_OK);
+  assert(f_write(&f,"new temp",8,&n)==FR_OK && n==8 && f_close(&f)==FR_OK);
+  assert(mounted.cdir==3 && !memcmp(directory,media[98],512));
+  assert(f_open(&f,kind?"tmp.pat":"tmp.snd",FA_READ)==FR_OK);char data[8];
+  assert(f_read(&f,data,8,&n)==FR_OK && n==8 && !memcmp(data,kind?"pat kept":"kit kept",8));
+  assert(f_close(&f)==FR_OK);
+ }
+ puts("PASS complete production FatFs path resolution: actual autosave paths stay under /tmp while CWD=/proj; current directory, project entries and existing kit/pattern bytes preserved");
+}
diff --git a/tests/silent_autosave/run.py b/tests/silent_autosave/run.py
new file mode 100644
index 0000000..ea0ddf9
--- /dev/null
+++ b/tests/silent_autosave/run.py
@@ -0,0 +1,31 @@
+#!/usr/bin/env python3
+"""Run the shared production-body fixture plus deterministic silent-autosave cases."""
+from pathlib import Path
+import subprocess,sys,importlib.util,hashlib,json,re,tempfile,os
+sys.dont_write_bytecode=True
+root=Path(__file__).resolve().parents[2]
+spec=importlib.util.spec_from_file_location('save_helpers',root/'tests/save_errors/run.py')
+h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
+text=(root/'Core/Src/Preset/presetManager.c').read_bytes().decode('latin1').replace('\r\n','\n')
+for path,want in json.loads((Path(__file__).parent/'baseline_sources.json').read_text()).items():
+    actual=h.extract(text,path).encode() if path=='preset_touchTimer' else (root/path).read_bytes()
+    assert hashlib.sha256(actual).hexdigest()==want,path
+for name in ['preset_processTimer','preset_autosaveOne','preset_autosaveCanRetry','preset_completeAutosave']:
+    body=h.extract(text,name)
+    assert not re.search(r'\b(?:menu_\w+|lcd_\w+|HAL_Delay|__disable_irq|__enable_irq|__set_PRIMASK|__set_BASEPRI|malloc|free|f_mount|BSP_SD_\w+|soundengine_\w+)\s*\(',body)
+    assert not re.search(r'\b(?:while|for)\s*\(',body)
+assert h.extract(text,'preset_completeAutosave').count('__STREXB(')==1
+print('PASS static boundary: manual UI/audio/IRQ/S1 sources and edit hook unchanged; autosave helpers have no waits, allocation, notifications, polling or loops',flush=True)
+subprocess.run([sys.executable,str(root/'tests/save_errors/run.py'),'--autosave-regressions'],cwd=root,check=True)
+
+paths=[]
+for name in ['preset_saveTmpKit','preset_saveTmpPat']:
+    paths.append(re.search(r'preset_openShared\([^,]+, "([^"]+)"',h.extract(text,name))[1])
+assert paths==['/tmp/tmp.snd','/tmp/tmp.pat']
+with tempfile.TemporaryDirectory(prefix='lxr-autosave-paths-') as d:
+    d=Path(d);(d/'paths.inc').write_text('static const char *actual_autosave_paths[]={'+','.join(json.dumps(p) for p in paths)+'};\n')
+    cmd=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
+    cmd+=['-I'+str(p) for p in [d,root,root/'tests/fatfs_integrity',root/'Middlewares/Third_Party/FatFs/src',root/'tests/sd_storage',root/'FATFS/Target']]
+    cmd+=[str(root/'tests/silent_autosave/paths.c'),'-o',str(d/'paths')]
+    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
+    subprocess.run(cmd,check=True,env=env);subprocess.run([str(d/'paths')],check=True,env=env,timeout=20)

```

## Appendix B — Exact executed command outputs

Baseline and final outputs are retained. Generic installation text is build-system output only; no instruction to flash was executed.

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
PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held

EXIT 0
```

### baseline-12.log

```text
$ python3 tests/live_storage/run.py
PASS actual notification/tick/repaint bodies: queued/busy display, 1999/2000ms and wrap, latest error, all result codes, expiry/copy interleaving, page/RAM independence, no storage unlock or blocking calls
PASS complete production FatFs: nine missing-path/invalid-name/existing-file/readonly-target/full-root failures make zero disk writes, preserve all RAM media/valid file bytes, then explicit create/write/close succeeds

EXIT 0
```

### baseline-13.log

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

### baseline-14.log

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

### baseline-15.log

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

### baseline-16.log

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

### baseline-17.log

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### baseline-18.log

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

### baseline-19.log

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### baseline-20.log

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

### baseline-21.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0x38", "configuration": "release", "elf_sha256": "faa871619f547c00a0c0ae9c98395dbb580819c14efb394f110e5a0d24d17e89", "image_bytes": 277308, "image_sha256": "b2fc09e17afe4e5fc3e7a0929ce60c368cd7a81d90a3f0300e5d5d8e3b10bf71", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224428, "ITCM_RAM": 15016, "RAM": 271584}, "payload_bytes": 277292, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x42", "configuration": "debug", "elf_sha256": "08169386236e26f6024ee533529c92b748391ee13c81031e4a5708b4088015d2", "image_bytes": 277356, "image_sha256": "bbfa527402ab823b8b1a9c53995a4263b2a64c0e1fa1f41704fdbc0cfd3f8558", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224476, "ITCM_RAM": 15016, "RAM": 271584}, "payload_bytes": 277340, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x0D", "configuration": "diagnostics-release", "elf_sha256": "6fa83eeddff95619163f213664b5518798289edec13dbed66490974d013c30e1", "image_bytes": 277748, "image_sha256": "6871ece11a6420eb93a0401e02693ca797b18607b3c41bdc2d0968d6f7328103", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224796, "ITCM_RAM": 15088, "RAM": 271656}, "payload_bytes": 277732, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x09", "configuration": "diagnostics-debug", "elf_sha256": "f61dc0e4901658f0807ea8e9095c70d15d530cd476d892ed507e3c33f6431ec3", "image_bytes": 277796, "image_sha256": "bea169680109f3c028c09de366f659dc1aa9fc8179354f924d5a38b0a2caf964", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224844, "ITCM_RAM": 15088, "RAM": 271656}, "payload_bytes": 277780, "reset_vector": "0x08014E4D", "result": "PASS"}

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
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical resolved /tmp targets/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, silent controlled background retries; no UI/hardware execution
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
$ python3 tests/silent_autosave/run.py
PASS static boundary: manual UI/audio/IRQ/S1 sources and edit hook unchanged; autosave helpers have no waits, allocation, notifications, polling or loops
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical resolved /tmp targets/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, silent controlled background retries; no UI/hardware execution
PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held
PASS silent recovery: 216 safe failed opens then 24 tenth-attempt successes; independent types; 2/4/8/16/32s cap and tick wrap
PASS edits/cooldown independence: 4000 edit/tick calls cause no storm; current RAM retained; kit/pattern deadlines separate; manual save does not clear pending backups
PASS production completion logic: same-tick edits during payload and exclusive store survive; reservation-loss cooldown; explicit pattern-start cancellation preserved; no ISR edit hook added
PASS unsafe/check-required holds: NOT_READY and ownership/mount failures do not loop; partial/write/close errors never replay; serious latches remain; completed explicit save re-arms only a healthy check-required hold
PASS complete production FatFs path resolution: actual autosave paths stay under /tmp while CWD=/proj; current directory, project entries and existing kit/pattern bytes preserved

EXIT 0
```

### final-10.log

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

### final-11.log

```text
$ cmake --build --preset release --parallel 4
[1/3] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271600 B       384 KB     69.07%
           FLASH:      226592 B       992 KB     22.31%
   text	   data	    bss	    dec	    hex	filename
 277744	    852	 341136	 619732	  974d4	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 278604 bytes, checksum 0x78
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

### final-12.log

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

### final-13.log

```text
$ cmake --build --preset debug --parallel 4
[1/3] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271600 B       384 KB     69.07%
           FLASH:      226640 B       992 KB     22.31%
   text	   data	    bss	    dec	    hex	filename
 277792	    852	 341136	 619780	  97504	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 278652 bytes, checksum 0xA4
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

### final-14.log

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### final-15.log

```text
$ cmake --build build/diagnostics-release --parallel 4
[1/3] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271672 B       384 KB     69.09%
           FLASH:      226960 B       992 KB     22.34%
   text	   data	    bss	    dec	    hex	filename
 278184	    852	 341208	 620244	  976d4	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 279044 bytes, checksum 0x16
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

### final-16.log

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### final-17.log

```text
$ cmake --build build/diagnostics-debug --parallel 4
[1/3] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271672 B       384 KB     69.09%
           FLASH:      227008 B       992 KB     22.35%
   text	   data	    bss	    dec	    hex	filename
 278232	    852	 341208	 620292	  97704	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 279092 bytes, checksum 0xB9
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

### final-18.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0x78", "configuration": "release", "elf_sha256": "369d90d42420b351fe9f363137f80870b56eb7ae2adf360a327817fea6066b2a", "image_bytes": 278620, "image_sha256": "a9b8f516b0df0679b93ee87199838a55864aa6f22eed68272a5601654f010d5a", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 225740, "ITCM_RAM": 15016, "RAM": 271600}, "payload_bytes": 278604, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xA4", "configuration": "debug", "elf_sha256": "9f4483da680195d450d6311ce7927fa6af67ea0b0077e6da2090c05174612d4e", "image_bytes": 278668, "image_sha256": "7afe3cf0777638f7ba8d0e1d8fb76ba8e4768920d67e1c262092d711a02c778c", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 225788, "ITCM_RAM": 15016, "RAM": 271600}, "payload_bytes": 278652, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x16", "configuration": "diagnostics-release", "elf_sha256": "a86e8c1cb97a0d0dbaa4bce5bf026e803142e122aa738b756ed814d7004fc874", "image_bytes": 279060, "image_sha256": "47e6695d50ff8fc37fb96ef3bd067fad7a10054452652bf9620da25e8325091b", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 226108, "ITCM_RAM": 15088, "RAM": 271672}, "payload_bytes": 279044, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xB9", "configuration": "diagnostics-debug", "elf_sha256": "6d57c773d04c01ca0db45e42808a3c35b5776c609faec2c94632ce13991b0f79", "image_bytes": 279108, "image_sha256": "0ded5c50957ca111c5609c4e6d3c72e1c2b732aa4d6bba9bf456a39d1a9a7221", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 226156, "ITCM_RAM": 15088, "RAM": 271672}, "payload_bytes": 279092, "reset_vector": "0x08014E4D", "result": "PASS"}

EXIT 0
```

### oracle-verify.log

```text
PASS frozen oracle exactly matches pinned S3 production function bodies
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical resolved /tmp targets/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, silent controlled background retries; no UI/hardware execution
PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held
```

### silent-final.log

```text
PASS static boundary: manual UI/audio/IRQ/S1 sources and edit hook unchanged; autosave helpers have no waits, allocation, notifications, polling or loops
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical resolved /tmp targets/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, silent controlled background retries; no UI/hardware execution
PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held
PASS silent recovery: 216 safe failed opens then 24 tenth-attempt successes; independent types; 2/4/8/16/32s cap and tick wrap
PASS edits/cooldown independence: 4000 edit/tick calls cause no storm; current RAM retained; kit/pattern deadlines separate; manual save does not clear pending backups
PASS production completion logic: same-tick edits during payload and exclusive store survive; reservation-loss cooldown; explicit pattern-start cancellation preserved; no ISR edit hook added
PASS unsafe/check-required holds: NOT_READY and ownership/mount failures do not loop; partial/write/close errors never replay; serious latches remain; completed explicit save re-arms only a healthy check-required hold
PASS complete production FatFs path resolution: actual autosave paths stay under /tmp while CWD=/proj; current directory, project entries and existing kit/pattern bytes preserved
```

### euclidean-seam-failure.log

```text
$ python3 tests/euclidean/run.py
Traceback (most recent call last):
  File "/workspace/lxr-desktop-app/tests/euclidean/run.py", line 102, in <module>
    main()
  File "/workspace/lxr-desktop-app/tests/euclidean/run.py", line 82, in main
    build = run(flags + [str(HERE / 'host.c'), '-Wl,--gc-sections', '-o', str(executable)])
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/workspace/lxr-desktop-app/tests/euclidean/run.py", line 39, in run
    raise RuntimeError(f'{command[0]} exited {result.returncode}\n'
RuntimeError: cc exited 1
In file included from /workspace/lxr-desktop-app/tests/euclidean/host.c:95:
Core/Src/Preset/presetManager.c: In function ‘preset_finishSave’:
Core/Src/Preset/presetManager.c:169:9: error: ‘preset_autosavePaused’ undeclared (first use in this function); did you mean ‘preset_autosaveIsPaused’?
  169 |         preset_autosavePaused = 0;
      |         ^~~~~~~~~~~~~~~~~~~~~
      |         preset_autosaveIsPaused
Core/Src/Preset/presetManager.c:169:9: note: each undeclared identifier is reported only once for each function it appears in


EXIT 1
```

### edit-hook-commands.log

```text
Full baseline translation unit read with git show feaff649599472036390ecef24f7193b7cc2e2ab:Core/Src/Preset/presetManager.c
Executed in /workspace/lxr-desktop-app/build/release:
/workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc -DSTM32F765xx -DUSE_HAL_DRIVER -I/workspace/lxr-desktop-app/Core/Inc -I/workspace/lxr-desktop-app/Drivers/CMSIS/Include -I/workspace/lxr-desktop-app/Drivers/CMSIS/Device/ST/STM32F7xx/Include -I/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Inc -I/workspace/lxr-desktop-app/Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I/workspace/lxr-desktop-app/Middlewares/Third_Party/FatFs/src -I/workspace/lxr-desktop-app/FATFS/App -I/workspace/lxr-desktop-app/FATFS/Target -I/workspace/lxr-desktop-app/Middlewares/JS/STM32_USB_Device_Library/Core/Inc -I/workspace/lxr-desktop-app/Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc -I/workspace/lxr-desktop-app/USB_DEVICE_MIDI/App -I/workspace/lxr-desktop-app/USB_DEVICE_MIDI/Target -O3 -g0 -std=gnu11 -Wall -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard --specs=nano.specs -ffunction-sections -fdata-sections -ffast-math -freciprocal-math -mfp16-format=ieee -fomit-frame-pointer -c /tmp/phase-s4l2/baseline-preset.c -o /tmp/phase-s4l2/baseline-preset.o
arm-none-eabi-objcopy --dump-section .text.preset_touchTimer=/tmp/phase-s4l2/baseline-touch.bin /tmp/phase-s4l2/baseline-preset.o
arm-none-eabi-objcopy --dump-section .text.preset_touchTimer=/tmp/phase-s4l2/final-touch.bin build/release/Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
arm-none-eabi-objdump -r -j .text.preset_touchTimer <each object>
```

### edit-hook-object.log

```text
PASS full production baseline/final target .text.preset_touchTimer sections identical: 72 bytes, SHA-256 1d3d8f48a7e528cbd850e2948f07152f082d0d365379e56de3ca45a51616986e
PASS identical edit-hook relocation records:
00000010 R_ARM_THM_CALL    HAL_GetTick
00000028 R_ARM_THM_CALL    HAL_GetTick
00000038 R_ARM_ABS32       preset_kitTimer
0000003c R_ARM_ABS32       preset_kitDirty
00000040 R_ARM_ABS32       preset_patDirty
00000044 R_ARM_ABS32       preset_patTimer
```

## Appendix C — Final artifact hashes and protected object comparisons

These local artifacts were not published for installation.

| Configuration | .bin SHA-256 | First16 image bytes |
|---|---|---|
|release|`96c1421e7364044bb2f8497e3ae8138d1f856c10d40e3e169d1629d19d19d8d3`|`4c58525632494d474c40040078000000`|
|debug|`48862832cad72c3975942164798e8ba91298f8867a7d6453c966c0a2dccbeb7a`|`4c58525632494d477c400400a4000000`|
|diagnostics-release|`9d4515a108d84ea2bc007cc44f0ed24e5352c26a1a88418c4f1d43a4dfaa5d60`|`4c58525632494d470442040016000000`|
|diagnostics-debug|`794acf876077cd9372c92556bf6a1f8720297f4f0900834a0c21692746319a26`|`4c58525632494d4734420400b9000000`|

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
build/debug/Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj UNCHANGED bdb63bcc117bc7f27ff8d352656a035188b3738db574241fc74c42f5980c2da7
build/debug/Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj UNCHANGED 3d765b55d878fd62927a97ba779d77c2363a849da6e42127156911075f91a0af
build/release/Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj UNCHANGED e902763d8f211095b2ee90203bc536f0649aa65f4e83c220804d22ec89f2d407
build/release/Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj UNCHANGED 4403e5a87b1e37d8dec0ab20fe282dc0b353f429ef53126afa8e5f1414973a2c
build/diagnostics-release/Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj UNCHANGED e902763d8f211095b2ee90203bc536f0649aa65f4e83c220804d22ec89f2d407
build/diagnostics-release/Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj UNCHANGED 4403e5a87b1e37d8dec0ab20fe282dc0b353f429ef53126afa8e5f1414973a2c
build/diagnostics-debug/Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj UNCHANGED 615556f2fc702ccdbd6938136f38631629828b530c66f82b1cf573288c1b5486
build/diagnostics-debug/Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj UNCHANGED edb3b02c5204bd309882bd2c6f1721cbebdb660df31813303d63fdd83788eb52
```

## Appendix D — Final target evidence for fixed state and completion

Release symbols, address/size:

```text
20025edc 00000001 b preset_autosavePaused
20025edf 00000001 B preset_kitDirty
20025ede 00000001 B preset_patDirty
20025ecc 00000008 b preset_kitRetry
20025ed4 00000008 b preset_patRetry
0801ecf4 0000007c T preset_processTimer
0801e7e8 000001c4 t preset_autosaveOne
```

The helper is outside ITCM. Compiler inlining creates multiple mutually exclusive instruction sites; source performs only one exclusive completion attempt per save, not a monitor retry loop. This is static target evidence, not measured interrupt latency.

```text

build/release/LXR_V2_STM32F765.elf:     file format elf32-littlearm


Disassembly of section .text:

0801e7e8 <preset_autosaveOne>:
 801e7e8:	e92d 47f0 	stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, lr}
 801e7ec:	461f      	mov	r7, r3
 801e7ee:	7803      	ldrb	r3, [r0, #0]
 801e7f0:	b082      	sub	sp, #8
 801e7f2:	b913      	cbnz	r3, 801e7fa <preset_autosaveOne+0x12>
 801e7f4:	b002      	add	sp, #8
 801e7f6:	e8bd 87f0 	ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, pc}
 801e7fa:	460d      	mov	r5, r1
 801e7fc:	4604      	mov	r4, r0
 801e7fe:	4616      	mov	r6, r2
 801e800:	f7eb fc28 	bl	800a054 <HAL_GetTick>
 801e804:	1b41      	subs	r1, r0, r5
 801e806:	f5b1 6ffa 	cmp.w	r1, #2000	@ 0x7d0
 801e80a:	d3f3      	bcc.n	801e7f4 <preset_autosaveOne+0xc>
 801e80c:	6873      	ldr	r3, [r6, #4]
 801e80e:	b11b      	cbz	r3, 801e818 <preset_autosaveOne+0x30>
 801e810:	6832      	ldr	r2, [r6, #0]
 801e812:	1a80      	subs	r0, r0, r2
 801e814:	4283      	cmp	r3, r0
 801e816:	d8ed      	bhi.n	801e7f4 <preset_autosaveOne+0xc>
 801e818:	2302      	movs	r3, #2
 801e81a:	7023      	strb	r3, [r4, #0]
 801e81c:	b357      	cbz	r7, 801e874 <preset_autosaveOne+0x8c>
 801e81e:	f8df 8188 	ldr.w	r8, [pc, #392]	@ 801e9a8 <preset_autosaveOne+0x1c0>
 801e822:	f898 3000 	ldrb.w	r3, [r8]
 801e826:	2b3f      	cmp	r3, #63	@ 0x3f
 801e828:	d848      	bhi.n	801e8bc <preset_autosaveOne+0xd4>
 801e82a:	f8df 9168 	ldr.w	r9, [pc, #360]	@ 801e994 <preset_autosaveOne+0x1ac>
 801e82e:	4858      	ldr	r0, [pc, #352]	@ (801e990 <preset_autosaveOne+0x1a8>)
 801e830:	f7ef fbce 	bl	800dfd0 <HAL_TIM_Base_Start_IT>
 801e834:	f899 5000 	ldrb.w	r5, [r9]
 801e838:	b935      	cbnz	r5, 801e848 <preset_autosaveOne+0x60>
 801e83a:	f8df a15c 	ldr.w	sl, [pc, #348]	@ 801e998 <preset_autosaveOne+0x1b0>
 801e83e:	f8da 3000 	ldr.w	r3, [sl]
 801e842:	2b00      	cmp	r3, #0
 801e844:	d049      	beq.n	801e8da <preset_autosaveOne+0xf2>
 801e846:	2510      	movs	r5, #16
 801e848:	4851      	ldr	r0, [pc, #324]	@ (801e990 <preset_autosaveOne+0x1a8>)
 801e84a:	f7ef fc13 	bl	800e074 <HAL_TIM_Base_Stop_IT>
 801e84e:	e8d4 3f4f 	ldrexb	r3, [r4]
 801e852:	b2db      	uxtb	r3, r3
 801e854:	2b02      	cmp	r3, #2
 801e856:	d115      	bne.n	801e884 <preset_autosaveOne+0x9c>
 801e858:	1e2b      	subs	r3, r5, #0
 801e85a:	bf18      	it	ne
 801e85c:	2301      	movne	r3, #1
 801e85e:	e8c4 3f47 	strexb	r7, r3, [r4]
 801e862:	f7eb fbf7 	bl	800a054 <HAL_GetTick>
 801e866:	b9bd      	cbnz	r5, 801e898 <preset_autosaveOne+0xb0>
 801e868:	2f00      	cmp	r7, #0
 801e86a:	bf0c      	ite	eq
 801e86c:	463d      	moveq	r5, r7
 801e86e:	f44f 65fa 	movne.w	r5, #2000	@ 0x7d0
 801e872:	e00c      	b.n	801e88e <preset_autosaveOne+0xa6>
 801e874:	f7ff ff46 	bl	801e704 <preset_saveTmpKit>
 801e878:	4605      	mov	r5, r0
 801e87a:	e8d4 3f4f 	ldrexb	r3, [r4]
 801e87e:	b2db      	uxtb	r3, r3
 801e880:	2b02      	cmp	r3, #2
 801e882:	d0e9      	beq.n	801e858 <preset_autosaveOne+0x70>
 801e884:	f3bf 8f2f 	clrex
 801e888:	f7eb fbe4 	bl	800a054 <HAL_GetTick>
 801e88c:	b925      	cbnz	r5, 801e898 <preset_autosaveOne+0xb0>
 801e88e:	e9c6 0500 	strd	r0, r5, [r6]
 801e892:	b002      	add	sp, #8
 801e894:	e8bd 87f0 	ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, pc}
 801e898:	4b3e      	ldr	r3, [pc, #248]	@ (801e994 <preset_autosaveOne+0x1ac>)
 801e89a:	781b      	ldrb	r3, [r3, #0]
 801e89c:	b9bb      	cbnz	r3, 801e8ce <preset_autosaveOne+0xe6>
 801e89e:	4b3e      	ldr	r3, [pc, #248]	@ (801e998 <preset_autosaveOne+0x1b0>)
 801e8a0:	681b      	ldr	r3, [r3, #0]
 801e8a2:	b9a3      	cbnz	r3, 801e8ce <preset_autosaveOne+0xe6>
 801e8a4:	2d08      	cmp	r5, #8
 801e8a6:	d86e      	bhi.n	801e986 <preset_autosaveOne+0x19e>
 801e8a8:	2d03      	cmp	r5, #3
 801e8aa:	d910      	bls.n	801e8ce <preset_autosaveOne+0xe6>
 801e8ac:	6873      	ldr	r3, [r6, #4]
 801e8ae:	6030      	str	r0, [r6, #0]
 801e8b0:	2b00      	cmp	r3, #0
 801e8b2:	d161      	bne.n	801e978 <preset_autosaveOne+0x190>
 801e8b4:	f44f 63fa 	mov.w	r3, #2000	@ 0x7d0
 801e8b8:	6073      	str	r3, [r6, #4]
 801e8ba:	e79b      	b.n	801e7f4 <preset_autosaveOne+0xc>
 801e8bc:	e8d4 3f4f 	ldrexb	r3, [r4]
 801e8c0:	b2db      	uxtb	r3, r3
 801e8c2:	2b02      	cmp	r3, #2
 801e8c4:	d01d      	beq.n	801e902 <preset_autosaveOne+0x11a>
 801e8c6:	f3bf 8f2f 	clrex
 801e8ca:	f7eb fbc3 	bl	800a054 <HAL_GetTick>
 801e8ce:	4b33      	ldr	r3, [pc, #204]	@ (801e99c <preset_autosaveOne+0x1b4>)
 801e8d0:	2201      	movs	r2, #1
 801e8d2:	701a      	strb	r2, [r3, #0]
 801e8d4:	b002      	add	sp, #8
 801e8d6:	e8bd 87f0 	ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, pc}
 801e8da:	220a      	movs	r2, #10
 801e8dc:	4930      	ldr	r1, [pc, #192]	@ (801e9a0 <preset_autosaveOne+0x1b8>)
 801e8de:	4650      	mov	r0, sl
 801e8e0:	f7f3 f862 	bl	80119a8 <f_open>
 801e8e4:	4605      	mov	r5, r0
 801e8e6:	b190      	cbz	r0, 801e90e <preset_autosaveOne+0x126>
 801e8e8:	2813      	cmp	r0, #19
 801e8ea:	d807      	bhi.n	801e8fc <preset_autosaveOne+0x114>
 801e8ec:	4b2d      	ldr	r3, [pc, #180]	@ (801e9a4 <preset_autosaveOne+0x1bc>)
 801e8ee:	40c3      	lsrs	r3, r0
 801e8f0:	07db      	lsls	r3, r3, #31
 801e8f2:	d503      	bpl.n	801e8fc <preset_autosaveOne+0x114>
 801e8f4:	f8da 3000 	ldr.w	r3, [sl]
 801e8f8:	2b00      	cmp	r3, #0
 801e8fa:	d0a5      	beq.n	801e848 <preset_autosaveOne+0x60>
 801e8fc:	f889 5000 	strb.w	r5, [r9]
 801e900:	e7a2      	b.n	801e848 <preset_autosaveOne+0x60>
 801e902:	2301      	movs	r3, #1
 801e904:	e8c4 3f42 	strexb	r2, r3, [r4]
 801e908:	f7eb fba4 	bl	800a054 <HAL_GetTick>
 801e90c:	e7df      	b.n	801e8ce <preset_autosaveOne+0xe6>
 801e90e:	f899 5000 	ldrb.w	r5, [r9]
 801e912:	9001      	str	r0, [sp, #4]
 801e914:	2d00      	cmp	r5, #0
 801e916:	d197      	bne.n	801e848 <preset_autosaveOne+0x60>
 801e918:	ab01      	add	r3, sp, #4
 801e91a:	2201      	movs	r2, #1
 801e91c:	4641      	mov	r1, r8
 801e91e:	4650      	mov	r0, sl
 801e920:	f7f3 faea 	bl	8011ef8 <f_write>
 801e924:	4605      	mov	r5, r0
 801e926:	2800      	cmp	r0, #0
 801e928:	d1e8      	bne.n	801e8fc <preset_autosaveOne+0x114>
 801e92a:	9b01      	ldr	r3, [sp, #4]
 801e92c:	2b01      	cmp	r3, #1
 801e92e:	d001      	beq.n	801e934 <preset_autosaveOne+0x14c>
 801e930:	463d      	mov	r5, r7
 801e932:	e7e3      	b.n	801e8fc <preset_autosaveOne+0x114>
 801e934:	f898 0000 	ldrb.w	r0, [r8]
 801e938:	f7ff fd74 	bl	801e424 <preset_writePatternData>
 801e93c:	4605      	mov	r5, r0
 801e93e:	2800      	cmp	r0, #0
 801e940:	d182      	bne.n	801e848 <preset_autosaveOne+0x60>
 801e942:	f899 5000 	ldrb.w	r5, [r9]
 801e946:	2d00      	cmp	r5, #0
 801e948:	f47f af7e 	bne.w	801e848 <preset_autosaveOne+0x60>
 801e94c:	f8da 3000 	ldr.w	r3, [sl]
 801e950:	b1e3      	cbz	r3, 801e98c <preset_autosaveOne+0x1a4>
 801e952:	4811      	ldr	r0, [pc, #68]	@ (801e998 <preset_autosaveOne+0x1b0>)
 801e954:	f7f3 fc2a 	bl	80121ac <f_close>
 801e958:	4605      	mov	r5, r0
 801e95a:	2800      	cmp	r0, #0
 801e95c:	d1ce      	bne.n	801e8fc <preset_autosaveOne+0x114>
 801e95e:	f899 3000 	ldrb.w	r3, [r9]
 801e962:	2b00      	cmp	r3, #0
 801e964:	f47f af70 	bne.w	801e848 <preset_autosaveOne+0x60>
 801e968:	f8da 3000 	ldr.w	r3, [sl]
 801e96c:	2b00      	cmp	r3, #0
 801e96e:	f47f af6b 	bne.w	801e848 <preset_autosaveOne+0x60>
 801e972:	4b0a      	ldr	r3, [pc, #40]	@ (801e99c <preset_autosaveOne+0x1b4>)
 801e974:	7018      	strb	r0, [r3, #0]
 801e976:	e767      	b.n	801e848 <preset_autosaveOne+0x60>
 801e978:	f5b3 4ffa 	cmp.w	r3, #32000	@ 0x7d00
 801e97c:	f4bf af3a 	bcs.w	801e7f4 <preset_autosaveOne+0xc>
 801e980:	005b      	lsls	r3, r3, #1
 801e982:	6073      	str	r3, [r6, #4]
 801e984:	e736      	b.n	801e7f4 <preset_autosaveOne+0xc>
 801e986:	2d0a      	cmp	r5, #10
 801e988:	d090      	beq.n	801e8ac <preset_autosaveOne+0xc4>
 801e98a:	e7a0      	b.n	801e8ce <preset_autosaveOne+0xe6>
 801e98c:	2509      	movs	r5, #9
 801e98e:	e75b      	b.n	801e848 <preset_autosaveOne+0x60>
 801e990:	2002196c 	.word	0x2002196c
 801e994:	20025edd 	.word	0x20025edd
 801e998:	200204e4 	.word	0x200204e4
 801e99c:	20025edc 	.word	0x20025edc
 801e9a0:	0802becc 	.word	0x0802becc
 801e9a4:	000d3df8 	.word	0x000d3df8
 801e9a8:	20025e33 	.word	0x20025e33

Disassembly of section .itcm_text:
```

Final source and report publication use new isolated branches. After report creation, verify exact URL/text, both heads, direct parents/tree/blob identities and all29 initial refs. The final user-facing success response is conditional on that verification; report/source checksums alone are not proof of remote publication.
