# LXR Firmware Lab — Phase S4b-1: Critical Project Storage Containment and Validation

Date: 2026-10-27 (UTC). Repository: `DocLXR-02/lxr-desktop-app`.

**Status: IMPLEMENTED — containment and software validation only. NOT READY FOR HARDWARE INSTALLATION until S4b-2 is completed and reviewed. NOT FLASHED — NOT HARDWARE VALIDATED.**

## 1. Objective, scope and result

Implement the confirmed project-storage memory-safety and false-success corrections without a transaction-system redesign. Protect existing project directories, preserve ordinary successful output and legacy formats, retain recoverable retries and all S1–S4-L2 safeguards, and leave audio/transport/RAM operation independent of storage errors as far as the existing architecture permits.

The resulting firmware:

* Rejects full-project saving to **any existing project path** with `FR_EXIST`. No existing project directory is emptied, renamed, truncated or replaced to begin a save. Even an empty or incomplete existing directory is protected.
* Saves a complete project to an **absent** `PROJ00`–`PROJ63` directory using the existing payload formats and checked writers. A failed new save can leave a partial directory; it is not automatically deleted or replayed.
* Removes `empty_directory()` and `delete_node()` entirely. Root project paths remain in `currentFolder[13]`; separately constructed child paths use capacity-checked buffers sized for an entire SFN path. Temporary cleanup is flat and limited to recognized firmware filenames under `/tmp`; unknown files and every subdirectory remain untouched.
* Validates persisted project/kit indices before unsafe consumers, rejects short project metadata and song data, propagates required read/close errors, checks the project-name write count/close, preserves search errors and checks CWD changes.
* Keeps intentionally incomplete legacy projects loadable by giving missing assets deterministic defaults rather than inheriting another project's RAM. It does not invent a new legacy file format or treat arbitrary truncation as a valid song.
* Adds a short storage-design note emphasizing the live instrument, honest results, bounded mechanisms, conventional project slots and independent backups. It expressly rejects unlimited generation archives.

The implementation is suitable for code review and the next **software-only** S4b-2 step. It is not a completed project replacement/recovery system, an SD-card reliability certification, or authorization to install firmware.

Evidence labels:

* **CODE**: established from the exact source, not observed physical behavior.
* **BUILD/HOST**: executed software tests, compiler output or binary analysis; mocks described.
* **OPEN**: deferred work or hardware-dependent behavior.

## 2. Exact repository state, instructions and publication

| Item | Exact reference |
|---|---|
| Authorized baseline | `safety/sd-silent-autosave-recovery-s4l2` |
| Baseline commit | **`910be804c7df2757f8d2347880751f2ca86c671c`** |
| New implementation branch | **`safety/sd-project-containment-s4b1`** |
| Published implementation commit | **`e49bb028462c339c4998cd08a845db17e1fc778f`** |
| Implementation parent | `910be804c7df2757f8d2347880751f2ca86c671c` (one parent) |
| Reviewed/published tree | `842396dafc554d9fb88ec7cd7b48a4d16484aad7` |
| Report-only base | `lab-setup`, **`5829398cea5d337200393757e39a37d4d8148256`** |
| Preserved main head | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Report branch | `codex-reports/2026-10-27-phase-s4b1-project-containment` |
| Report path | `docs/codex-berichte/2026-10-27_phase-s4b1-project-containment.md` |

Implementation URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/safety/sd-project-containment-s4b1

Commit URL: https://github.com/DocLXR-02/lxr-desktop-app/commit/e49bb028462c339c4998cd08a845db17e1fc778f

The existing checkout `/workspace/lxr-desktop-app` was initially clean at the exact requested baseline. The remote source head matched. `git merge-base --is-ancestor 5829398cea5d337200393757e39a37d4d8148256 910be804c7df2757f8d2347880751f2ca86c671c` succeeded. A new local branch was created directly from that commit; no worktree was created.

The complete lab-setup `AGENTS.md` was read, blob `2f4d121f53a7055c1a30355235384a95ef920f1a`. The complete authoritative S4b report was obtained from `codex-reports/2026-10-27-phase-s4b-project-storage-design`, file `docs/codex-berichte/2026-10-27_phase-s4b-project-storage-design.md`, blob **`ce222c2679e1bb5bd249743c9223ea26770caaa3`**, and matched the previously read complete report, including its appendices. Its report commit is `5886663f95ce4305cecac33bc856243f05c09aac`. The prior S1/S2/S3/S4a/S4-L/S4-L2 source lineage is already included in baseline 910be804. No previous correction was cherry-picked, replaced or reset.

The cloud-environment skill, current environment status and network policy were inspected. No subagents, paid services, hardware operations, physical card access, flashing utility, repair command or firmware upload was used.

Publication used GitHub Git-data APIs: upload exact file blobs, create a tree from the baseline tree, verify that it exactly matches local `git write-tree`, create a one-parent commit, and create only the new implementation branch. The exact published commit was fetched using `git fetch origin refs/heads/safety/sd-project-containment-s4b1`; the new local branch was fast-forwarded to it after checking tree equality. The checkout is clean at the published SHA. No destructive reset, branch merge or force-push occurred. The 32 pre-existing remote branch heads were checked unchanged after implementation publication; final report verification checks them again.

Only the completed Markdown report is published on the separate report-only branch from lab-setup. Its own commit SHA is provided in GitHub metadata/final handoff rather than attempting to embed its future self-hash in this file. The report branch does not contain the implementation tree; use the explicit source commit links here.

## 3. Modified files — complete list

| File | Reason |
|---|---|
| `Core/Src/Preset/presetManager.c` | Bounded paths, removal of recursive deletion, conservative new-slot-only project save, checked reads/writes/closes, indices, deterministic missing-asset defaults and verified CWD transitions |
| `Core/Inc/Preset/PresetManager.h` | `preset_initTmpFolder()` now returns `FRESULT`; existing callers may ignore it, corrected project/cleanup paths consume it |
| `Core/Src/main.c` | Use actual boot index/load results; existing format-initialization caller checks project-save result before showing its existing success/error text |
| `docs/storage-design-s4b.md` | Concise agreed live-performance/storage principles and temporary overwrite restriction |
| `tests/project_storage/run.py` | Reproducible production-function extraction, static scope checks, ASan/UBSan compile/run orchestration |
| `tests/project_storage/host.c` | Fault-injected production-body project/read/path/cleanup tests with mocked filesystem/HAL/UI |
| `tests/project_storage/filesystem.c` | Whole-production FatFs RAM-volume test of project directory reservation and old-data preservation |
| `tests/project_storage/README.md` | Exact test scope, mocks, limitations and run command |
| `tests/euclidean/run.py` | Include new shared read-finalization dependency when extracting existing production readers; no assertion/oracle removed |
| `tests/save_errors/run.py` | Include path/whitelist dependencies; adapt old duplicated-return static assertion to common cleanup exit; retain byte oracle and all injected-error assertions |
| `tests/save_errors/host.c` | Replace removed recursive-cleanup mock with the actual flat directory/unlink API mocks |

Source commit diff: **11 files, 977 insertions, 1,012 deletions**. Most production deletion is replacement of duplicated project save/load exits and removal of their disabled legacy bulk-project branches and unsafe recursive helpers. This is not removal of supported old kit/pattern/song readers: legacy filenames and V1/V2/V3 pattern decoding remain tested. No complete vendor package was upgraded or replaced.

No changes to `AGENTS.md`, CMake/presets, FatFs core, SD HAL/BSP/S1 driver, startup, linker, clocks/PLL, IRQ priorities, bootloader/updater, DSP, MIDI mappings, sequencer source, USB, runtime diagnostics or menu/button source. `Core/Src/main.c` is an application source filename; the Git **main branch** was not modified.

## 4. Confirmed defects and corrections

References below are to `Core/Src/Preset/presetManager.c` at **e49bb028** unless a different file is named. Permanent links use this exact commit, not a moving baseline.

Direct review links: [path and reservation helpers](https://github.com/DocLXR-02/lxr-desktop-app/blob/e49bb028462c339c4998cd08a845db17e1fc778f/Core/Src/Preset/presetManager.c#L174), [project writer](https://github.com/DocLXR-02/lxr-desktop-app/blob/e49bb028462c339c4998cd08a845db17e1fc778f/Core/Src/Preset/presetManager.c#L2168), [new test runner](https://github.com/DocLXR-02/lxr-desktop-app/blob/e49bb028462c339c4998cd08a845db17e1fc778f/tests/project_storage/run.py), [short agreed design note](https://github.com/DocLXR-02/lxr-desktop-app/blob/e49bb028462c339c4998cd08a845db17e1fc778f/docs/storage-design-s4b.md).

### 4.1 Path construction and deletion containment

The prior normal project save called `empty_directory(currentFolder)` with `currentFolder[13]`. `/proj00/prj.nfo` requires **16 bytes including NUL**; `/proj00/00-KICK1.SND` requires **21**. The S4b host investigation demonstrated ASan global-buffer overflows using ordinary filenames. Its separate `delete_node()` guard detected overflow but still attempted filesystem work on the failed/truncated path.

Corrections:

* `preset_projectPath()` **line174** validates slot0–63 and output capacity before writing the fixed `/projNN` root path. Invalid input does not initiate filesystem work.
* `preset_childPath()` **line195** checks a single 8.3 component, rejects separators/dot traversal/invalid characters and overlong base/extension, checks the full output capacity, and writes only after validation. `currentFolder` is no longer an expandable child-path workspace; it remains a small project-root string.
* `empty_directory()` and `delete_node()` no longer exist. No recursive function is used in normal project save or temporary cleanup.
* `preset_clearTmpFolder()` **line617** enumerates only `/tmp`, uses a bounded child-path buffer and checks unlink/closedir results. It preserves every subdirectory and unknown filename. On path construction failure, no unlink, recursive call, further enumeration or other operation uses the invalid path; the already-open read-only directory handle is closed as required cleanup.

Recognized temporary filenames (`preset_isTmpFile`, **line228**): `tmp.snd`, `tmp.pat`; firmware-style `NN-xxxxx.snd/.pat/.sng` with slot00–63 and at most five suffix characters; legacy `pNNN.snd/.pat/.sng` with slot0–63. Matching is ASCII case-insensitive. These are treated as firmware-owned **only inside its reserved `/tmp` directory**. Files matching those conventions placed there by an external user are indistinguishable from firmware temporaries; do not use `/tmp` as a personal backup folder. Unknown files such as README, slot64+ names and all nested directories remain untouched. The root `/tmp` itself is not deleted.

The flat scan is foreground work, not a background task; it does not introduce a hard wall-clock bound on arbitrarily large or damaged directory traversal. Lower-level S1/S2 timing protections remain unchanged. There is no recursive stack growth.

### 4.2 Never erase an existing project to start a replacement

`preset_createProjectFolder()` **line185** reserves only an absent path. Any existing object returns `FR_EXIST`, including empty directories, incomplete directories and ordinary files. Non-FR_NO_FILE stat errors are returned directly. A failed mkdir is not followed by payload writes.

`preset_saveProject()` **line2168**:

1. Check shared-file fault/ownership, project index, selected kit/active pattern and associated-kit indices.
2. Keep existing save-time TIM2 rendering hook; reserve an absent project directory.
3. Check `f_chdir` before any relative payload write. Only then capture current RAM kit/pattern buffers.
4. Create `prj.nfo` exclusively with `FA_CREATE_NEW`; write exactly eight checked bytes and check close through `preset_finishSave`.
5. Save globals, then all 64 kit/pattern/song triplets using the existing checked helpers. Stop on the first error. Successful output remains 194 files / **255,398 payload bytes**.
6. Clear only recognized temporary files after all project payload saves succeed; propagate cleanup/setup failures rather than reporting unconditional success.
7. Restore normal display processing and return the first error. Restore `/tmp` only after this function actually entered the project CWD, and only while shared ownership is healthy. No restoration failure replaces an earlier error. Existing timer hooks are balanced; no transport-stop call was added.

There is no rename transaction, staging manifest, archive, migration or automatic rollback. **A failed new save can consume that formerly unused directory name**, and a later overwrite request is refused. Choose another absent slot for a safe subsequent save if no serious fault is latched. Do not bypass fault/ownership protection to reclaim the partial directory.

S4b-2 must resolve bounded replacement and explicit handling of partial directories. This phase intentionally favors preserving existing data over silently implementing an incomplete replacement protocol.

### 4.3 Bounds for persisted indices and RAM consumers

* `preset_readPatternLengths()` **line408** stages seven lengths and associated kit locally, validates kit0–63 before publishing **either** lengths or kit. Existing length1–64 and legacy zero→16 behavior is retained. V1 files without associated-kit storage use deterministic kit0 rather than a stale RAM association.
* `preset_LoadTmpKit()` **line383** validates the stored kit byte before publication, stages parameters locally, checks read result/close, and uses an absolute `/tmp/tmp.snd` path. Legacy short kit tails still zero-fill; no invalid kit index is published.
* Root project-index reader **line990** accepts only a complete one-byte valid index and successful close, initializes its output to safe0 on failure, and handles a null output pointer. Index writer **line1005** rejects invalid slots.
* Project create/save/load/name scan and named kit/song readers reject slot64–255 before array use/filesystem payload work. RAM kit load/store **lines805,837**, init **line739**, and pattern RAM save **line1962** also have defensive bounds.
* Full-project save refuses invalid in-RAM associated-kit selectors rather than knowingly persisting an unloadable set.

The RAM kit-load guard is also reached through existing sequencer calls. It adds a small bounds comparison/branch at that call, **not a new per-sample task or ISR filesystem path**. No cycle cost was measured on hardware. The sequencer/audio/IRQ source and their rebuilt objects remain identical.

### 4.4 Metadata, song and required close validation

`preset_finishRead()` **line221** closes an already-open read-only handle and preserves the first read/validation error if close also fails. A close failure cannot become a success. Failed open is never followed by a fabricated close. Uncertain write cleanup continues to use the existing different rule: do not force-close after incomplete write.

* Project metadata: require a successful eight-byte read and successful close **before** changing project CWD/RAM or cleaning temporary recovery data. A missing `prj.nfo` in an existing directory becomes `FR_INT_ERR`, not the `FR_NO_FILE` that the existing menu interprets as a request to create a fresh RAM project. Truly absent project directories still return `FR_NO_FILE`; load no longer creates them.
* `preset_loadSongSD()` **line2231** stages name8 + song64 in small local buffers, checks both counts/results and close, then publishes. Every shorter 0..71-byte file is rejected without stale song-tail publication. Legacy `pNNN.sng` remains supported with the same complete72-byte payload; no documented variable-length song format was found, so arbitrary short data is not relabeled “legacy.”
* `preset_loadGlobalFile()` **line914** stages all30 global bytes, checks read/count/close, then publishes. Public `preset_loadGlobals()` retains its boolean interface but no longer returns success on failed close. Missing globals in an intentional incomplete project use zeroed global storage followed by existing defaults.
* Named kit, named pattern, temporary pattern and root selector readers consume required close results. Named-kit header reads now test both FRESULT and byte count; payload short-tail zero-fill compatibility remains.
* Pattern tags are validated as supported PatV01/02/03 when the tag prefix is present. Untagged V1 still rewinds using a now-checked seek. Existing Euclidean tests exercise old layouts and current exact byte sequences.
* `preset_makeFileNameLoad()` **line2423** preserves search/closedir/open errors instead of collapsing them into FR_NO_FILE. Old-name fallback occurs only for true FR_NO_FILE. A directory handle is zero-initialized and closed only if opened.

Minimum required bytes are checked; this is not a new blanket ban on appended bytes in every historical format. Metadata/song fields are no longer accepted **short**, but no new serialization header, padding or file-layout field is added.

### 4.5 Loading and intentionally incomplete legacy projects

`preset_loadProject()` **line2294** validates directory/metadata, verifies CWD, then reads the existing assets. Missing kit/pattern/song files are still permitted, as the original source explicitly intended. They now get:

| Missing component | Deterministic behavior |
|---|---|
| Kit | Name/init payload from `initKit`, bounded copy, zero-filled remainder |
| Pattern | Existing `seq_clearPattern` defaults, shuffle0, associated kit0, Empty name |
| Song | Entire64-byte SONG_END content, Empty name |
| Globals | Zero global storage followed by existing default settings |

`preset_defaultKit()` **line251** fixes the S4b-confirmed initialization tail defect. The old init loop assigned only239 of247 parameter bytes. The new copy uses the actual246-byte initialization payload and zeroes the remaining byte; it never blindly copies247 bytes past `initKit`. `preset_init()` retains its existing modulation normalization and parameter send behavior.

Project name publication occurs after asset processing. The root last-project selector result is checked. `/tmp` cleanup, when requested by the existing manual load flow, is deferred until a completed load and successful selector update; it is no longer the first action before validation. Cleanup failure is returned honestly. The implementation does not pretend this is whole-project RAM rollback; see limitations.

`preset_loadProjectNames()` **line2272** uses a full checked **absolute metadata path**, stages its8-byte name and publishes only after successful read/close. It no longer depends on unchecked chdir or attempts a read after failed open. Its existing void interface remains; failures do not falsely claim a project loaded, but this helper does not introduce a new list-error UI.

### 4.6 Direct callers and feedback

`Core/Src/main.c:826–830` now consumes the actual index and project-load results. A missing selector defaults safely to project0; invalid/read/close errors do not silently become a loaded identity. The existing non-blocking `menu_showSdError()` path reports failures. Its implementation, display duration and autosave silence were not changed.

`main.c:172–180` is the **existing user-confirmed format-initialization flow**, not the bootloader. It now checks `preset_saveProject(0)` and selects its existing error text rather than unconditional Success. No format command, confirmation mapping, reset or wait was added; the existing format/reboot workflow was not executed. This caller was checked statically, not simulated as a physical format. Normal project menu save/load callers already consume FRESULT and retain their S4-L behavior.

## 5. Storage error contract and temporary restrictions

| Condition | Result / permitted recovery |
|---|---|
| Existing project directory/file | `FR_EXIST`, no payload open/write/unlink/mkdir; no app fault latch merely for refusal. Repeated refusal does not poison later safe save-to-absent-slot. |
| Invalid index/path/capacity | `FR_INVALID_PARAMETER` or `FR_INVALID_NAME`, no operation using the failed path/selector. |
| Missing entire project directory | `FR_NO_FILE`; normal explicit new-project RAM initialization remains possible without creating the directory during load. |
| Existing folder with missing/short metadata | `FR_INT_ERR`, no auto-create/reset of RAM as an empty new project and no temporary deletion. |
| Missing optional legacy assets | Deliberate defaults, FROK if all required present/read/close/index/setup operations succeed; no retained old slot data. |
| Failed/short required write | First failure, no later payload writes, no forced close/replay; existing shared-file and S1 safeguards remain authoritative. |
| Failed required read or close | First failure returned, no fabricated success; unresolved close/ownership remains protected. |
| CWD/setup failure | No payload in the wrong directory; propagate error. Failed newly created directory is not automatically removed. |
| Serious disk fault | Existing reboot-only S1 fail-closed policy remains; no reset/remount/automatic write retry. Music/RAM remain available subject to existing execution architecture. |

**Temporary user-visible restriction:** project overwrite is unavailable. “Empty” in the existing name list is not proof a directory is absent: an empty directory, failed-save directory or boot-created `PROJ00` path still returns FR_EXIST. No special UI status or new button mapping was introduced. An absent alternative slot may be used when storage ownership is healthy; there is no approved instruction here to delete/reformat the original card.

No unlimited generation accumulation or recovery database was added. New saves consume only their conventional project slot; failed new saves may leave partial contents. No automatic cleanup pretends those contents are valid or retries an uncertain write.

## 6. Live performance and safety review

**CODE:** no new interrupt source, priority, clock, DMA size/layout, watchdog, reset, card remount, background polling/retry loop, heap allocation, audio ISR logging or filesystem operation was introduced. Existing save/load TIM2 render hooks remain. Directory cleanup no longer recurses or uses a256-byte recursive path buffer. No transport STOP call or dirty-flag clear was added to save error handling. Silent autosave functions and their edit hook remain unchanged and pass all prior tests.

The critical RAM kit guard adds a conditional check when that function is already called, including from existing sequencer paths. It is directly required for memory safety. IRQ and audio objects being unchanged does **not** imply every callee has identical cycle counts.

**OPEN:** synchronous FatFs/SD operations still occupy foreground; they can delay UI/MIDI/USB servicing despite interrupt-driven audio. The pre-existing display-buffer spin remains. Directory enumeration and individual file calls do not have a measured whole-operation latency budget. No glitch-free playback, CPU load, worst-case execution time, actual stack high-water, physical write completion or card quiescence was measured. Existing DWT timing users and diagnostics were preserved; no CYCCNT reset or new instrumentation was added.

No hardware/card, firmware update, option-byte, bootloader protection, application flash origin or hardware initialization change was made. Application origin remains `0x08008000`; first32KiB excluded; `UPDATE_BOOTLOADER=0`. Source version remains **1.7**. No equivalence with public Erica Synths v1.71 or the user's installed binary is claimed.

## 7. Regression tests — actual execution

Environment:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

CMake **3.31.10**; Ninja **1.13.0.git.kitware.jobserver-pipe-1**; Python **3.12.14**; ARM GCC **14.2.1 20241119 (15:14.2.rel1-1)** with prepared newlib; native GCC **14.2.0**. No toolchain package/clock/build-preset change.

All ten established suites were executed at the final source content, plus the new suite. **11/11 commands exit0; no suite skipped or suppressed.** Existing sanitizer modes remained enabled; new native tests use ASan/UBSan, `-fno-sanitize-recover=all`, non-PIE and fixed target FatFs integer widths. LeakSanitizer is disabled (`ASAN_OPTIONS=detect_leaks=0`) in the managed executor; no leak-validation claim is made.

| Command | Result and key coverage |
|---|---|
| `python3 tests/bug001/run.py` | PASS; CC0/cache bounds, transport slots, external NRPN, all Morph values and unchanged cadence |
| `python3 tests/euclidean/run.py` | PASS; 2176 original masks, byte-value bounds, 294912 flam/shift ticks, 4160 recording positions,130 roundtrips,10210 malformed/legacy cases,180802 protocol calls; six ASan/UBSan groups |
| `python3 tests/safety/run.py` | PASS; bounded sbrk and diagnostics OFF/ON logic/wrap/saturation |
| `python3 tests/usb_allocator/run.py` | PASS; 2000 production lifecycle cycles,14 failure cases, no class heap refs |
| `python3 tests/sd_storage/run.py` | PASS; counts1..128, timeout/wrap, S1 fault/initialization/CTRL_SYNC/context policy |
| `python3 tests/sd_hal/run.py` | PASS; actual command/HAL/BSP bounds/error/state/timeout cases, no replay/reset |
| `python3 tests/fatfs_integrity/run.py` | PASS; whole/extracted production FatFs sync/close/mirrors/FSInfo/init and read-only configuration |
| `python3 tests/save_errors/run.py` | PASS;1536 byte-identical S3 oracle saves,2544 injected write/short-write positions,936 logical failed opens then104 successful explicit saves, menu/button/ownership behavior |
| `python3 tests/live_storage/run.py` | PASS; non-blocking feedback/wrap and actual FatFs harmless retry/data-preservation cases |
| `python3 tests/silent_autosave/run.py` | PASS;216 safe opens before24 tenth-attempt successes,4000 edit/tick calls, independent dirty/cooldown, silent unsafe holds and exact `/tmp` paths |
| `python3 tests/project_storage/run.py` | PASS; new production-body and whole-FatFs groups described below |

### New production-body tests

`tests/project_storage/run.py` extracts complete actual functions from final production source, using original prototypes/bodies and unchanged production pattern reset/copy helpers and `initKit`. The orchestration is committed and reproducible. `host.c` mocks FatFs file APIs, HAL timers, LCD and sound-send operations. This is **not** an LXR or SD-device test.

Coverage includes:

* All256 project path indices; valid0/63 and invalid64..255 public project/kit calls; maximum12-character SFN; formerly overflowing metadata path; one-byte-short output capacity; traversal/separator/base/extension rejection and unchanged output on failure.
* All256 associated-kit bytes:0..63 valid;64..255 rejected before staged length/kit publication. Invalid root selector and temp-kit slot cannot reach the old out-of-bounds consumers. Invalid public RAM calls have no send/audio-service/array effects.
* Flat temp cleanup: exact maximum filename, case-insensitive recognized/legacy names, unknown and slot64+ files retained, nested directories untouched, invalid child path causes no unlink, unlink/closedir errors retained.
* Ten consecutive overwrite rejections preserve every existing/unknown entry byte; successful save to absent slot63 writes194 files/255398 payload bytes; ordinary files and empty directories are also protected. Wrong CWD prevents payload writes.
* **38 reached project-failure scenarios**: payload positions1,2,3,249,250,300,500,5000,19000,19394 for errors and short writes; file open/close positions1,2,3,4,5,33,97,193,194. Every selected point is asserted reached. No later payload, old-project deletion or temp deletion after failure; retained uncertainty prevents replay.
* Project metadata lengths0..7 fail before CWD/RAM/temp mutation. Missing directory is not created by load; missing metadata in an existing folder is an error. Full-count read errors and failed opens/closes are exercised.
* An intentionally incomplete legacy project with valid nfo and missing all192 assets loads deterministic defaults (including zeroing unused global storage), not stale previous data.
* Every short song file length0..71 fails without publishing old/stale tails; complete legacy p000.sng and short-tail p000.snd remain supported. Root selector failed close does not publish an index; directory search errors are not hidden by old-name fallback.
* Required close failures across named kit/pattern/song, temporary kit/pattern, globals and name-scan paths never report success or bypass retained FIL ownership. Initialization copy matches the actual246-byte payload and safely fills the final parameter.

### Whole-production FatFs group

`filesystem.c` includes the existing `tests/fatfs_integrity/host.c` fixture with its main renamed: complete production FatFs/generic disk/registration/S1 code on synthetic RAM sectors. It executes the actual new path/reservation functions, not a rewritten gate. The limited fixture advertises FAT16 geometry and accesses only its seeded RAM sectors; it is not a physical volume or full-project block-device simulation.

Ten existing-project reservation attempts return FR_EXIST with **zero sector writes** and byte-identical RAM media. The old nfo is then read through actual f_open/f_read/f_close and remains OLDVALID. Creating absent slot63 succeeds and existing metadata remains readable. No formatting or deletion is used. This establishes the gate's real FatFs behavior separately from mocked full-project serialization.

### Existing test adaptations and errors encountered

The Euclidean extraction fixture needed `preset_finishRead`; the save-error extraction fixture needed the new pure path/whitelist helpers and flat directory mocks. The old static check expected a duplicated `return res` after globals; it now checks the common `goto done` error exit. None of the byte oracles, fault positions or safety assertions was removed to make code pass.

Early new-fixture development found missing front-panel/automation constant includes and a mock bug that resolved empty CWD-relative directory names as `/projNN/` instead of the current directory. Those **test fixture** issues were corrected. The initial static scan also matched a stale removed-function comment, which was removed. Final runs pass; no firmware defect was hidden by those fixes.

Plain `git diff --check` treats the repository's preserved CRLF as trailing whitespace; the appropriate `git -c core.whitespace=cr-at-eol diff --check` and staged equivalent pass. No mass line-ending conversion was performed. The established FatFs host compiler still warns about pre-existing unused `lun` in `ff_gen_drv.c`; firmware builds report no new warning/error. No required test remains failing.

GitHub publication initially attempted to reconstruct a commit from normalized API metadata to synchronize the local object; the hash did not match, so no local ref was changed by that attempt. A normal targeted Git fetch obtained the canonical object, verified its tree/parent, and the new branch was fast-forwarded safely. Source content/tests were unaffected; publication was verified afterward.

## 8. Builds, image validation and exact resource impact

All following configuration/build commands returned exit0:

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

After the exact reviewed tree was committed as e49bb028 and the checkout synchronized cleanly, all four build commands and image checks were run again; there was no source recompilation required and artifact hashes matched. Diagnostics remain **OFF by default**; ON variants use their existing isolated directories. Existing post-build output prints firmware installation instructions; none were followed. No generated file is in Git or uploaded here.

Artifacts in each local build directory: `LXR_V2_STM32F765.elf`, `.bin` and `LXR_V2_V1.7.img`. Header16 bytes: eight-byte `LXRV2IMG`, little-endian payload length, one-byte checksum `0xFF-(sum(payload)&0xFF)`, three zero bytes. Each exact payload matched `.bin`. MSP `0x20080000`; reset vector `0x0801519D` (Thumb, inside image). Every allocated section and **every LOAD LMA** was checked against the configured regions and application Flash.

| Build | .bin payload | .img total | Checksum | Physical Flash span / free | Delta from S4-L2 |
|---|---:|---:|---|---:|---:|
| Release OFF |280268|280284|7A|280268 /735540|+1664|
| Debug OFF |280316|280332|9A|280316 /735492|+1664|
| Release ON |280708|280724|1D|280708 /735100|+1664|
| Debug ON |280756|280772|EF|280756 /735052|+1664|

Physical application Flash capacity is **1,015,808 bytes** (`0x08008000..0x080FFFFF`). The all-LMA span equals `.bin` length. Linker reports separately give228256/228304/228624/228672 bytes, also+1664 each; those do not include all explicitly placed TCM initialization payloads. The existing validator's Flash **VMA** high-water is227404/227452/227772/227820. Do not use either smaller number as the physical installed image span. The additional all-LMA analysis checks the larger true span; this is accounting, not a linker change.

| Region | Capacity | S4-L2 normal → S4b-1 normal | S4-L2 ON → S4b-1 ON | Current remaining |
|---|---:|---:|---:|---:|
| ITCM |16384|15016 →15016|15088 →15088|1368 OFF /1296 ON|
| DTCM |131072|107392 →107392|107392 →107392|23680 both|
| General SRAM |393216|271600 →271600|271672 →271672|121616 OFF /121544 ON|

**Static memory delta: zero in all three RAM regions.** No new static capture buffer, archive index, heap object or runtime instrumentation exists. Small read-staging buffers are on the foreground stack. These are static usage figures, not measured runtime free stack/heap.

### Stack-frame and object comparison

The Release compile command was reproduced with `-fstack-usage` in `/tmp`, separately for baseline source/header and final source. This measures individual compiler frames, not a whole call-chain/interrupt worst case:

| Function/frame | Baseline bytes | Current bytes |
|---|---:|---:|
| `preset_saveProject` |152|104|
| `preset_loadProject` |144|160|
| `preset_LoadTmpKit` |32|272|
| `preset_loadSongSD` |136|144|
| `preset_loadProjectNames` |96|72|
| `preset_clearTmpFolder.part.0` |336|160|
| `delete_node` |88 per recursive call|removed|
| `empty_directory` |104 per recursive call|removed|

Temp-kit staging adds240 bytes to that foreground frame; its benefit is no invalid/failed-close publication. Removing recursive deletion removes unbounded depth. Optimizer wrappers for clearTmp have zero-sized frames while real work is in `.part.0`; globals helper is inlined. Existing DTCM static headroom23,680 is **not** proof of adequate worst-case stack use. Actual nesting/high-water remains unmeasured.

Seven Release objects were rebuilt using unchanged source and the old baseline header, with the same compiler/options, and compared byte-for-byte to final objects: IRQ handlers, sound engine, audio DMA callbacks, sequencer, S1 disk driver, bounded sysmem and runtime diagnostics. **All seven object SHA-256 values match** (appendix). This is stronger than source-only scope checking, but does not measure latency of the new validated storage/kit callees.

## 9. Remaining limitations and readiness

1. **Project overwrite is intentionally disabled**, including empty/partial existing directories. There is no automatic deletion, replacement or repair. Normal boot code can create PROJ00 before a save, so its name may be unavailable even if the UI says Empty. Conventional slots and files remain unchanged.
2. A failed new save may leave partial new files. The older valid project is never intentionally deleted to make room. No atomic/power-loss guarantee is claimed.
3. Required metadata is written before the rest of a new project as in the original ordering. Because intentionally incomplete legacy projects remain supported, a later boot cannot always distinguish an interrupted new directory with missing assets from a legitimate incomplete legacy project. It can load defaults for missing files. S4b-2 should address completion/replacement ordering; this phase does not introduce a half-finished marker protocol.
4. Full-project loading still applies assets incrementally. A late read/index/cleanup error can leave a partially changed RAM project, although it is not falsely reported successful. Whole-RAM rollback, consistent snapshots while recording/editing and atomic identity handover remain S4b-2/integration design questions. A failed **save** does not clear dirty state or intentionally discard edits.
5. Global `/tmp` recovery is still not tied to a project identity. Boot still runs its recovery overlay and ignores some legacy overlay results. New bounds/close checks help, but no project-bound recovery format was added. Unknown nested tmp contents are retained rather than recursively erased.
6. Legacy pattern readers retain existing optional timing/layout semantics; current strict index/tag checks are not proof of compatibility with every external/private firmware file. Named-slot wildcard duplicates can still select an older file after a display-name change. Individual `FA_CREATE_ALWAYS` replacements remain nontransactional even though their errors are checked.
7. The name-scan helper retains a void interface and can leave an earlier displayed name after a read/close error; it does not establish a loaded-project success. No new notification/list-status feature was added.
8. Existing mount/remount/shared-volume ownership is not redesigned. S1 serious-fault and application uncertain-file protection still require controlled reboot/inspection in some cases. No card/peripheral quiescence recovery was established or latches cleared.
9. Synchronous storage, foreground MIDI/UI responsiveness, stack high-water, audio timing and physical media behavior need later authorized measurement. Static/build/host results cannot approve installation.
10. PROJ00's physical anomaly remains **undiagnosed**. No card/image was available, no corruption or recovery was demonstrated, and exact installed firmware identity is unknown. This correction is not evidence that it caused or repaired that incident.

The critical containment fixes and their regression tests are implemented. The scope is ready for software review. **NOT READY FOR HARDWARE INSTALLATION until S4b-2 is completed and reviewed.** Neither publication nor a successful build authorizes flashing.

## 10. Recommended minimal S4b-2, for separate review only

Follow the new short design note, which supersedes the earlier S4b report's suggestion of immutable generation archives. Preserve the conventional PROJ00–PROJ63 interface and existing payload formats; do not build a recovery database or unbounded history.

1. Retain the current rejection until a bounded replacement is validated. Review **one staging directory and at most one previous-copy directory per slot**, with explicit ownership and finite names/space requirements. No unlimited generations.
2. Write a complete new set using existing checked serializers, close/sync all required files, and validate required metadata/counts before touching the old project. Consider publishing required `prj.nfo` last in a fresh staged set so common interrupted writes do not look complete. This is not an atomicity claim.
3. Review the short rename/commit sequence and every error/power-cut transition with actual FatFs RAM images. FAT directory rename is **not inherently atomic** and duplicate names may alias one chain. A small bounded marker is justified only if needed to interpret these few locations safely; do not invent a broad recovery database.
4. Keep the previous complete copy until new completion is verified. Never clear the only known-good copy to make staging space. Unknown files, ambiguous aliasing, integrity faults or uncertain writes must produce a truthful hold, not automatic repair/deletion/replay. Garbage collection is bounded and separately justified.
5. Separate validation from RAM application as far as practical without a whole-project copy; define honest partial-load handling and temp ownership. Review consistent save capture while editing rather than silently claiming a snapshot. Do not stop music automatically.
6. Commit sector-level fault-injection tests at each write/short-write/close/rename/cleanup/reboot point; re-run these11 suites and all four builds/resources. Add authentic legacy fixtures and read-only clone inspection if available. Only then prepare a separate controlled hardware-validation proposal with Daniel's approval and independent backups.

Same-card FAT/SD damage and unexpected power loss cannot be given absolute guarantees by a small application protocol. Practical preservation, honest failure and a conventional independent backup are the intended standard.

## 11. Handoff verification and exclusions

The source branch/commit and changed-file set were read back on GitHub: parent910be804, tree842396da, exactly the11 listed files. All32 earlier remote heads remained unchanged. Local source branch is clean at e49bb028. Builds/images were rechecked there without changing source. Only this new Markdown file is committed on the separate lab-setup-based report branch; final file content, parent and branch head are verified before claiming handoff success.

No firmware artifact, temporary executable, secret, token, private identity metadata or physical-card content is included. No main/lab-setup/previous-branch modification, merge, force-push, firmware flashing, hardware operation, SD format/repair/delete experiment, bootloader/option-byte change or installation authorization occurred.

The appendices retain complete relevant test output, artifact hashes and decisive source references so another assistant can continue from the exact source revision without this conversation.

## Appendix A — Decisive final source excerpts

Line numbers refer to commit e49bb028462c339c4998cd08a845db17e1fc778f; CRLF normalized for presentation.

### Core/Src/Preset/presetManager.c:173–224

```c
/* Root project paths only: never append a child to currentFolder[13]. */
static FRESULT preset_projectPath(char *path, size_t capacity, uint8_t slot)
{
    if(!path || capacity < sizeof("/proj00") || slot >= NUM_KIT_SLOTS)
        return FR_INVALID_PARAMETER;
    memcpy(path, "/proj00", sizeof("/proj00"));
    path[5] = '0' + slot / 10;
    path[6] = '0' + slot % 10;
    return FR_OK;
}

/* Only an absent path can be reserved; never inspect/delete old contents. */
static FRESULT preset_createProjectFolder(const char *folder)
{
    FILINFO info;
    FRESULT res = f_stat(folder, &info);
    if(res == FR_OK) return FR_EXIST;
    if(res != FR_NO_FILE) return res;
    return f_mkdir(folder);
}

/* A single SFN child, built completely before any filesystem operation. */
static FRESULT preset_childPath(char *path, size_t capacity,
                               const char *folder, const char *name)
{
    size_t base = 0, length = 0, dot = 0;
    if(!path || !folder || !name) return FR_INVALID_PARAMETER;
    while(base < capacity && folder[base]) base++;
    if(base == 0 || base == capacity || folder[0] != '/') return FR_INVALID_NAME;
    while(length < 13 && name[length]) {
        unsigned char c = name[length];
        if(c <= ' ' || c >= 127 || strchr("\"*+,/:;<=>?[\\]|", c)) return FR_INVALID_NAME;
        if(c == '.') {
            if(dot || length == 0 || length > 8) return FR_INVALID_NAME;
            dot = length + 1;
        }
        length++;
    }
    if(length == 0 || length == 13 || (!dot && length > 8) ||
       (dot && (length == dot || length - dot > 3))) return FR_INVALID_NAME;
    if(capacity - base < length + 2) return FR_INVALID_NAME;
    memmove(path, folder, base);
    path[base] = '/';
    memcpy(path + base + 1, name, length + 1);
    return FR_OK;
}

/* Read-only handles may be closed; retain the first error if close also fails. */
static FRESULT preset_finishRead(FRESULT res)
{
    FRESULT closed = preset_closeShared((FIL*)&SDFile);
    return res == FR_OK ? closed : res;
```

### Core/Src/Preset/presetManager.c:251–276

```c
static void preset_defaultKit(uint8_t slot)
{
    if(slot >= NUM_KIT_SLOTS) return;
    size_t count = sizeof(initKit) - 8;
    if(count > END_OF_SOUND_PARAMETERS) count = END_OF_SOUND_PARAMETERS;
    memcpy(savefileNames[slot][KIT_NAMES], initKit, 8);
    memset(mainParameters[slot], 0, END_OF_SOUND_PARAMETERS);
    memcpy(mainParameters[slot], initKit + 8, count);
}

static FRESULT preset_writePatternData(uint8_t patternNr);

FRESULT preset_initTmpFolder(void)
{
    FRESULT res = preset_sharedStatus();
    FILINFO info;
    if(res != FR_OK) return res;
    res = f_stat("/tmp", &info);
    if(res == FR_NO_FILE) {
        res = f_mkdir("/tmp");
        if(res != FR_OK) return res;
    } else if(res != FR_OK) return res;
    else if(!(info.fattrib & AM_DIR)) return FR_NO_PATH;
    return f_chdir("/tmp");
}
```

### Core/Src/Preset/presetManager.c:408–434

```c
static uint8_t preset_readPatternLengths(uint8_t patternNr, uint8_t dataVersion)
{
	if(patternNr >= NUM_PATTERN) return 0;
	uint8_t lengths[NUM_TRACKS];
	uint8_t kit = 0;
	UINT bytesRead;
	if(f_read((FIL*)&SDFile, lengths, sizeof(lengths), &bytesRead) != FR_OK) return 0;
	if(dataVersion == 1 && bytesRead == 0)
	{
		// Untagged V1 files may have no length tail at all.
		for(uint8_t trk=0; trk<NUM_TRACKS; trk++) lengths[trk] = 16;
	}
	else if(bytesRead != sizeof(lengths)) return 0;
	for(uint8_t trk=0; trk<NUM_TRACKS; trk++)
	{
		if(lengths[trk] == 0) lengths[trk] = 16;
		if(lengths[trk] > NUM_STEPS) return 0;
	}
	if(dataVersion > 1)
	{
		if(f_read((FIL*)&SDFile, &kit, sizeof(kit), &bytesRead) != FR_OK || bytesRead != sizeof(kit) || kit >= NUM_KIT_SLOTS) return 0;
	}
	for(uint8_t trk=0; trk<NUM_TRACKS; trk++)
		seq_patternSet.length[patternNr][trk] = lengths[trk];
	seq_patternSet.kitNr[patternNr] = kit;
	return 1;
}
```

### Core/Src/Preset/presetManager.c:617–641

```c
FRESULT preset_clearTmpFolder(void)
{
    FRESULT res = preset_sharedStatus();
    FILINFO info;
    DIR dir;
    char path[21]; /* /tmp + slash + maximum 12-byte SFN + NUL. */
    if(res != FR_OK) return res;
    res = f_opendir(&dir, "/tmp");
    if(res == FR_NO_PATH || res == FR_NO_FILE) return preset_initTmpFolder();
    if(res != FR_OK) return res;
    for(;;) {
        res = f_readdir(&dir, &info);
        if(res != FR_OK || !info.fname[0]) break;
        /* Preserve unknown files and all directories; never recurse. */
        if((info.fattrib & AM_DIR) || !preset_isTmpFile(info.fname)) continue;
        res = preset_childPath(path, sizeof(path), "/tmp", info.fname);
        if(res != FR_OK) break;
        res = f_unlink(path);
        if(res != FR_OK) break;
    }
    FRESULT closed = f_closedir(&dir);
    if(res == FR_OK) res = closed;
    if(res == FR_OK) res = preset_initTmpFolder();
    return res;
}
```

### Core/Src/Preset/presetManager.c:990–1014

```c
FRESULT preset_loadLastUsedProjectNrFromSD(uint8_t *projNr)
{
    if(!projNr) return FR_INVALID_PARAMETER;
    *projNr = 0;
    uint8_t slot;
    UINT count;
    FRESULT res = preset_openShared((FIL*)&SDFile, "/proj.idx", FA_READ);
    if(res != FR_OK) return res;
    res = f_read((FIL*)&SDFile, &slot, 1, &count);
    if(res == FR_OK && (count != 1 || slot >= NUM_KIT_SLOTS)) res = FR_INT_ERR;
    res = preset_finishRead(res);
    if(res == FR_OK) *projNr = slot;
    return res;
}
//----------------------------------------------------
FRESULT preset_saveLastUsedProjectNrToSD(uint8_t projNr)
{
    if(projNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
#if USE_SD_CARD
    FRESULT res = preset_openShared((FIL*)&SDFile, "/proj.idx", FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) return res;
    return preset_finishSave(preset_writeChecked(&projNr, 1));
#else
    return FR_NOT_READY;
#endif
```

### Core/Src/Preset/presetManager.c:2168–2228

```c
FRESULT preset_saveProject(uint8_t presetNr)
{
    FRESULT res = preset_sharedStatus();
    uint8_t entered = 0;
    char folder[8];
    if(res != FR_OK) return res;
    res = preset_projectPath(folder, sizeof(folder), presetNr);
    if(res != FR_OK) return res;
    if(menu_selectedPresetNr[SAVE_TYPE_KIT] >= NUM_KIT_SLOTS || seq_activePattern >= NUM_PATTERN)
        return FR_INVALID_PARAMETER;
    for(uint8_t i = 0; i < NUM_PATTERN; i++)
        if(seq_patternSet.kitNr[i] >= NUM_KIT_SLOTS) return FR_INT_ERR;
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Start_IT(&htim2);
#endif
    /* S4b-1 containment: even empty existing directories are never replaced. */
    res = preset_createProjectFolder(folder);
    if(res != FR_OK) goto done;
    res = f_chdir(folder);
    if(res != FR_OK) goto done;
    entered = 1;
    memcpy(currentFolder, folder, sizeof(folder));
    preset_saveDrumsetRam(menu_selectedPresetNr[SAVE_TYPE_KIT], 0);
    res = preset_savePatternRAM(seq_activePattern);
    if(res != FR_OK) goto done;
    lcd_process_display_interrupt = 0;
    while(sendDisplayBuffer() != 0);
    lcd_clear();
    lcd_home();
    lcd_string(PSTR("Saving Project"));
    res = preset_openShared((FIL*)&SDFile, "prj.nfo", FA_CREATE_NEW | FA_WRITE);
    if(res != FR_OK) goto done;
    res = preset_finishSave(preset_writeChecked(savefileNames[presetNr][PROJECT_NAMES], 8));
    if(res != FR_OK) goto done;
    res = preset_saveGlobals();
    if(res != FR_OK) goto done; /* No later writes or temporary-file deletion. */
    //save all drumkits, patterns and songs
    for(uint8_t i = 0; i < NUM_KIT_SLOTS; i++) {
        char text[17];
        snprintf(text, sizeof(text), "%u%%", (unsigned)i * 100u / NUM_KIT_SLOTS);
        lcd_setcursor(0,2);
        lcd_string(text);
        res = preset_saveDrumsetSD(i, 0);
        if(res != FR_OK) goto done;
        res = preset_savePatternSD(i);
        if(res != FR_OK) goto done;
        res = preset_saveSongSD(i);
        if(res != FR_OK) goto done;
    }
    res = preset_clearTmpFolder();
done:
    lcd_process_display_interrupt = 1;
    if(entered && preset_sharedStatus() == FR_OK) {
        FRESULT restored = preset_initTmpFolder();
        if(res == FR_OK) res = restored;
    }
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Stop_IT(&htim2);
#endif
    return res;
}
```

### Core/Src/Preset/presetManager.c:2231–2255

```c
FRESULT preset_loadSongSD(uint8_t songNr)
{
    if(songNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
    char filename[13], name[8];
    uint8_t data[MAX_SONG_LENGTH];
    UINT count;
    FRESULT res = preset_makeFileNameLoad(filename, songNr, FEXT_SONG);
    if(res == FR_NO_FILE) {
        preset_makeFileNameOld(filename, songNr, FEXT_SONG);
        res = preset_openShared((FIL*)&SDFile, filename, FA_READ);
    }
    if(res != FR_OK) return res;
    res = f_read((FIL*)&SDFile, name, sizeof(name), &count);
    if(res == FR_OK && count != sizeof(name)) res = FR_INT_ERR;
    if(res == FR_OK) {
        res = f_read((FIL*)&SDFile, data, sizeof(data), &count);
        if(res == FR_OK && count != sizeof(data)) res = FR_INT_ERR;
    }
    res = preset_finishRead(res);
    if(res != FR_OK) return res;
    if(preset_isNameEmpty(name)) memcpy(name, "Empty\0\0", 8);
    memcpy(savefileNames[songNr][SONG_NAMES], name, sizeof(name));
    memcpy(seq_songData[songNr], data, sizeof(data));
    return FR_OK;
}
```

### Core/Src/Preset/presetManager.c:2294–2335

```c
FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
{
    FRESULT res = preset_sharedStatus();
    FILINFO info;
    uint8_t entered = 0;
    char folder[8], path[21], name[8];
    UINT count;
    if(res != FR_OK) return res;
    res = preset_projectPath(folder, sizeof(folder), presetNr);
    if(res != FR_OK) return res;
    if(seq_activePattern >= NUM_PATTERN) return FR_INVALID_PARAMETER;
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
    HAL_TIM_Base_Start_IT(&htim2);
#endif
    res = f_stat(folder, &info);
    if(res != FR_OK) goto done; /* No mkdir or tmp cleanup while validating a load. */
    if(!(info.fattrib & AM_DIR)) { res = FR_NO_PATH; goto done; }
    res = preset_childPath(path, sizeof(path), folder, "prj.nfo");
    if(res != FR_OK) goto done;
    res = preset_openShared((FIL*)&SDFile, path, FA_READ);
    /* An existing project without metadata is invalid, not a request to create. */
    if(res == FR_NO_FILE) res = FR_INT_ERR;
    if(res != FR_OK) goto done;
    res = f_read((FIL*)&SDFile, name, sizeof(name), &count);
    if(res == FR_OK && count != sizeof(name)) res = FR_INT_ERR;
    res = preset_finishRead(res);
    if(res != FR_OK) goto done;
    res = f_chdir(folder);
    if(res != FR_OK) goto done;
    entered = 1;
    memcpy(currentFolder, folder, sizeof(folder));
    lcd_process_display_interrupt = 0;
    while(sendDisplayBuffer() != 0);
    lcd_clear();
    lcd_home();
    lcd_string(PSTR("Loading Project"));
    res = preset_loadGlobalFile();
    if(res == FR_NO_FILE) {
        memset(&activeParameters[PAR_BEGINNING_OF_GLOBALS], 0, NUM_PARAMS - PAR_BEGINNING_OF_GLOBALS);
        preset_loadDefaultGlobals();
        res = FR_OK;
    }
```

### Core/Src/main.c:169–184

```c

		sprintf(savefileNames[0][PROJECT_NAMES], "Init");
		//preset_saveDrumsetSD(0);
		res = preset_saveProject(0);

		//success
		lcd_clear();
		lcd_home();
		lcd_string(res == FR_OK ? "Success!" : "Error-Please try");
		//goto 2nd line
		lcd_setcursor(0,2);
		lcd_string("Please reboot.");

		while(1);
	}
}
```

### Core/Src/main.c:824–840

```c
#if LOAD_PROJECT_ON_BOOT
	uint8_t lastUsedProject;
	res = preset_loadLastUsedProjectNrFromSD(&lastUsedProject);
	if(res == FR_NO_FILE) res = FR_OK; /* No saved selection: use validated default 0. */
	//do not clean the tmp folder here, as we are just booting
	if(res == FR_OK) res = preset_loadProject(lastUsedProject, 0);
	if(res != FR_OK)
	{
		menu_showSdError(res);
	}
	else
	{
		menu_loadedPresetNr[CURRENT_PROJECT] = menu_selectedPresetNr[SAVE_TYPE_PROJECT] = lastUsedProject;
		//menu_resetSaveParameters();
	}
#else
	for(uint8_t i=0;i<NUM_KIT_SLOTS;i++)
```

## Appendix B — Actual test commands and complete relevant output

The output below is from the final source content. Every listed command returned exit0; host results are not hardware observations.

### python3 tests/bug001/run.py

```text
$ python3 tests/bug001/run.py
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence

EXIT 0
```

### python3 tests/euclidean/run.py

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

### python3 tests/safety/run.py

```text
$ python3 tests/safety/run.py
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state

EXIT 0
```

### python3 tests/usb_allocator/run.py

```text
$ python3 tests/usb_allocator/run.py
PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing
PASS USB failures: 14 rollback/retention/retry scenarios plus invalid requests; no allocator calls
Host object: size=552 alignment=8 (target size/alignment checked separately in ELF)
PASS complete production USB class object: no heap/allocator-lock references

EXIT 0
```

### python3 tests/sd_storage/run.py

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

### python3 tests/sd_hal/run.py

```text
$ python3 tests/sd_hal/run.py
PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP
PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap
PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset
PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation

EXIT 0
```

### python3 tests/fatfs_integrity/run.py

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

### python3 tests/save_errors/run.py

```text
$ python3 tests/save_errors/run.py
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical resolved /tmp targets/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, silent controlled background retries; no UI/hardware execution
PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held

EXIT 0
```

### python3 tests/live_storage/run.py

```text
$ python3 tests/live_storage/run.py
PASS actual notification/tick/repaint bodies: queued/busy display, 1999/2000ms and wrap, latest error, all result codes, expiry/copy interleaving, page/RAM independence, no storage unlock or blocking calls
PASS complete production FatFs: nine missing-path/invalid-name/existing-file/readonly-target/full-root failures make zero disk writes, preserve all RAM media/valid file bytes, then explicit create/write/close succeeds

EXIT 0
```

### python3 tests/silent_autosave/run.py

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

### python3 tests/project_storage/run.py

```text
$ python3 tests/project_storage/run.py
PASS capacity-checked project/SFN paths: all256 indices, exact maximum8.3, old overflowing metadata path and unchanged output on rejection
PASS invalid project/kit consumers and all256 associated-kit values: no OOB, no publication or I/O on rejected public indices
PASS nonrecursive temp cleanup: maximum SFN, owned whitelist, unknown files/subdirectories retained, invalid path causes no unlink, close/unlink errors retained
PASS overwrite containment: ten truthful FR_EXIST results preserve all old/unknown data; safe slot63 saves194 files/255398 bytes; wrong CWD never receives payload
PASS injected project nfo/global/early/late child open/write/short-write/close failures: first error, no later payload, no old-project or temp deletion, unsafe replay blocked
Project failure scenarios exercised: 38
PASS short metadata/songs, read/open/close/search errors and absolute name lookup; intentionally incomplete projects get deterministic defaults; old kit/song filenames preserved
PASS required reader closes: kit/pattern/song/temp/global/name failure never reports success or bypasses retained ownership; init defaults bounded to actual payload
PASS all S4b-1 production-body groups under ASan/UBSan (filesystem, HAL, LCD and sound sending are mocked)
PASS whole production FatFs: ten overwrite rejections perform zero RAM-sector writes; previous metadata still readable; absent slot63 creation succeeds without deleting old project
PASS S4b-1 static scope: no recursive deletion, replacement protocol or remount/reset; no new filesystem path in ISR sources

EXIT 0
```

## Appendix C — Build, binary and memory evidence

### Configure: cmake --preset release

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

### Configure: cmake --preset debug

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

### Configure: cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### Configure: cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### Executed build

```text
$ cmake --build --preset release --parallel 4
[1/4] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[2/4] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271600 B       384 KB     69.07%
           FLASH:      228256 B       992 KB     22.47%
   text	   data	    bss	    dec	    hex	filename
 279408	    852	 341136	 621396	  97b54	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 280268 bytes, checksum 0x7A
[4/4]
[Existing installation-instructions message omitted; not followed.]
EXIT 0
```

### Executed build

```text
$ cmake --build --preset debug --parallel 4
[1/4] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[2/4] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271600 B       384 KB     69.07%
           FLASH:      228304 B       992 KB     22.48%
   text	   data	    bss	    dec	    hex	filename
 279456	    852	 341136	 621444	  97b84	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 280316 bytes, checksum 0x9A
[4/4]
[Existing installation-instructions message omitted; not followed.]
EXIT 0
```

### Executed build

```text
$ cmake --build build/diagnostics-release --parallel 4
[1/4] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[2/4] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271672 B       384 KB     69.09%
           FLASH:      228624 B       992 KB     22.51%
   text	   data	    bss	    dec	    hex	filename
 279848	    852	 341208	 621908	  97d54	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 280708 bytes, checksum 0x1D
[4/4]
[Existing installation-instructions message omitted; not followed.]
EXIT 0
```

### Executed build

```text
$ cmake --build build/diagnostics-debug --parallel 4
[1/4] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[2/4] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271672 B       384 KB     69.09%
           FLASH:      228672 B       992 KB     22.51%
   text	   data	    bss	    dec	    hex	filename
 279896	    852	 341208	 621956	  97d84	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 280756 bytes, checksum 0xEF
[4/4]
[Existing installation-instructions message omitted; not followed.]
EXIT 0
```

### Complete final image validator output

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0x7A", "configuration": "release", "elf_sha256": "5d0d0553c95bc711548ab35a5b0987dbd00439edd815e179aafd51ff4091d69f", "image_bytes": 280284, "image_sha256": "aee6d39f30cc3acb177bff676a48ec38398480c26df45a4f02c34b86c39a7836", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 227404, "ITCM_RAM": 15016, "RAM": 271600}, "payload_bytes": 280268, "reset_vector": "0x0801519D", "result": "PASS"}
{"checksum": "0x9A", "configuration": "debug", "elf_sha256": "5af49ad49231ef997bdb41c80216a48e6855ff7ab1fe20ee88a3b6fac743b4ec", "image_bytes": 280332, "image_sha256": "0120527692b72a20347bafb0867478962cc29799235ce07dc78aa06152a4a3c6", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 227452, "ITCM_RAM": 15016, "RAM": 271600}, "payload_bytes": 280316, "reset_vector": "0x0801519D", "result": "PASS"}
{"checksum": "0x1D", "configuration": "diagnostics-release", "elf_sha256": "b54a81255dca468f8c72ff989e2d477a930a236b6e3f7cb177421d4d94d2e519", "image_bytes": 280724, "image_sha256": "a3895c2e382fcbce6184794750de576c6408f91b259afb7422cbacc14beac43b", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 227772, "ITCM_RAM": 15088, "RAM": 271672}, "payload_bytes": 280708, "reset_vector": "0x0801519D", "result": "PASS"}
{"checksum": "0xEF", "configuration": "diagnostics-debug", "elf_sha256": "dee7b53d5c881eb573e61b433bf9d206535e685e919d5caab8c58945aaa240ef", "image_bytes": 280772, "image_sha256": "92951a1ea9a3a378bb0d863a92553740425926ede304067c5b948bd5ae2622dd", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 227820, "ITCM_RAM": 15088, "RAM": 271672}, "payload_bytes": 280756, "reset_vector": "0x0801519D", "result": "PASS"}

EXIT 0
```

### All LOAD LMAs, Flash limits and .bin/.img/.elf SHA-256

```json
[
  {
    "configuration": "release",
    "payload": 280268,
    "flash_load_span": 280268,
    "flash_capacity": 1015808,
    "flash_remaining": 735540,
    "binary_sha256": "7052bb3899c6155dcd7816611654f25dd805dc7af2c87c38c609bd77e9967e1c",
    "image_sha256": "aee6d39f30cc3acb177bff676a48ec38398480c26df45a4f02c34b86c39a7836",
    "elf_sha256": "5d0d0553c95bc711548ab35a5b0987dbd00439edd815e179aafd51ff4091d69f"
  },
  {
    "configuration": "debug",
    "payload": 280316,
    "flash_load_span": 280316,
    "flash_capacity": 1015808,
    "flash_remaining": 735492,
    "binary_sha256": "db1d9c90d3a81d217bb4d9e69785d2ed8947a94ccc86d3d63d46d40957e462ac",
    "image_sha256": "0120527692b72a20347bafb0867478962cc29799235ce07dc78aa06152a4a3c6",
    "elf_sha256": "5af49ad49231ef997bdb41c80216a48e6855ff7ab1fe20ee88a3b6fac743b4ec"
  },
  {
    "configuration": "diagnostics-release",
    "payload": 280708,
    "flash_load_span": 280708,
    "flash_capacity": 1015808,
    "flash_remaining": 735100,
    "binary_sha256": "ac6eea4b253c59f90c51ccbe747dc347fedb1637ca643bd7704ad395b46e0f71",
    "image_sha256": "a3895c2e382fcbce6184794750de576c6408f91b259afb7422cbacc14beac43b",
    "elf_sha256": "b54a81255dca468f8c72ff989e2d477a930a236b6e3f7cb177421d4d94d2e519"
  },
  {
    "configuration": "diagnostics-debug",
    "payload": 280756,
    "flash_load_span": 280756,
    "flash_capacity": 1015808,
    "flash_remaining": 735052,
    "binary_sha256": "ae62c8260a23df5ab7b3391e8d4d25bef7c9c43945478490dc445fc2613befcb",
    "image_sha256": "92951a1ea9a3a378bb0d863a92553740425926ede304067c5b948bd5ae2622dd",
    "elf_sha256": "dee7b53d5c881eb573e61b433bf9d206535e685e919d5caab8c58945aaa240ef"
  }
]
```

### Release baseline/current object identity and static stack analysis

```json
{
  "objects": [
    {
      "source": "Core/Src/stm32f7xx_it.c",
      "baseline_header_and_source": true,
      "object_sha256": "01463848afda54efbbeea116af34c9ff35db325406b42b003432ee46fba5c468",
      "identical": true
    },
    {
      "source": "Core/Src/Soundengine/Soundengine.c",
      "baseline_header_and_source": true,
      "object_sha256": "d6ee7b60ad6a2a1924e096b3631b8f38284349e3ad7c037c348e5ca3e7f17aca",
      "identical": true
    },
    {
      "source": "Core/Src/Soundengine/cs4344_cs5343.c",
      "baseline_header_and_source": true,
      "object_sha256": "10472838499bb9806639fc82153ba880797c3c496ac48e10e6724aeaa2e12ac5",
      "identical": true
    },
    {
      "source": "Core/Src/Soundengine/Sequencer/sequencer.c",
      "baseline_header_and_source": true,
      "object_sha256": "126fd4049cfb37baba465f8841bffaca509f1a38eaae84135430bc4413b9a257",
      "identical": true
    },
    {
      "source": "FATFS/Target/sd_diskio.c",
      "baseline_header_and_source": true,
      "object_sha256": "92362fc88603fa5f853e490ff2e909558770f9619693fc09c9581ebc76869b2c",
      "identical": true
    },
    {
      "source": "Core/Src/sysmem.c",
      "baseline_header_and_source": true,
      "object_sha256": "df9e63061b13f6fcd12e4756ae5d2db73033bcfc1f5d1fe9ea0d561ac07508e0",
      "identical": true
    },
    {
      "source": "Core/Src/safety/runtimeDiagnostics.c",
      "baseline_header_and_source": true,
      "object_sha256": "84ffc4dc90244d3186a3748fe71eba9795648c7490447e20fe056319ad9f0da6",
      "identical": true
    }
  ],
  "stack_frames": {
    "preset_saveProject": {
      "baseline": {
        "bytes": 152,
        "kind": "static"
      },
      "current": {
        "bytes": 104,
        "kind": "static"
      }
    },
    "preset_loadProject": {
      "baseline": {
        "bytes": 144,
        "kind": "static"
      },
      "current": {
        "bytes": 160,
        "kind": "static"
      }
    },
    "preset_LoadTmpKit": {
      "baseline": {
        "bytes": 32,
        "kind": "static"
      },
      "current": {
        "bytes": 272,
        "kind": "static"
      }
    },
    "preset_loadSongSD": {
      "baseline": {
        "bytes": 136,
        "kind": "static"
      },
      "current": {
        "bytes": 144,
        "kind": "static"
      }
    },
    "preset_clearTmpFolder": {
      "baseline": {
        "bytes": 0,
        "kind": "static"
      },
      "current": {
        "bytes": 0,
        "kind": "static"
      }
    },
    "delete_node": {
      "baseline": {
        "bytes": 88,
        "kind": "static"
      },
      "current": null
    },
    "empty_directory": {
      "baseline": {
        "bytes": 104,
        "kind": "static"
      },
      "current": null
    },
    "preset_loadGlobalFile": {
      "baseline": null,
      "current": null
    },
    "preset_loadProjectNames": {
      "baseline": {
        "bytes": 96,
        "kind": "static"
      },
      "current": {
        "bytes": 72,
        "kind": "static"
      }
    }
  },
  "maximum_local_frames": {
    "baseline": [
      [
        "preset_clearTmpFolder.part.0",
        {
          "bytes": 336,
          "kind": "static"
        }
      ],
      [
        "preset_loadPatternSD",
        {
          "bytes": 152,
          "kind": "static"
        }
      ],
      [
        "preset_saveProject",
        {
          "bytes": 152,
          "kind": "static"
        }
      ],
      [
        "preset_loadProject",
        {
          "bytes": 144,
          "kind": "static"
        }
      ],
      [
        "preset_loadDrumsetSD",
        {
          "bytes": 136,
          "kind": "static"
        }
      ],
      [
        "preset_loadSongSD",
        {
          "bytes": 136,
          "kind": "static"
        }
      ],
      [
        "empty_directory",
        {
          "bytes": 104,
          "kind": "static"
        }
      ],
      [
        "preset_loadProjectNames",
        {
          "bytes": 96,
          "kind": "static"
        }
      ]
    ],
    "current": [
      [
        "preset_LoadTmpKit",
        {
          "bytes": 272,
          "kind": "static"
        }
      ],
      [
        "preset_clearTmpFolder.part.0",
        {
          "bytes": 160,
          "kind": "static"
        }
      ],
      [
        "preset_loadProject",
        {
          "bytes": 160,
          "kind": "static"
        }
      ],
      [
        "preset_loadPatternSD",
        {
          "bytes": 152,
          "kind": "static"
        }
      ],
      [
        "preset_loadSongSD",
        {
          "bytes": 144,
          "kind": "static"
        }
      ],
      [
        "preset_loadDrumsetSD",
        {
          "bytes": 136,
          "kind": "static"
        }
      ],
      [
        "preset_saveProject",
        {
          "bytes": 104,
          "kind": "static"
        }
      ],
      [
        "preset_loadProjectNames",
        {
          "bytes": 72,
          "kind": "static"
        }
      ]
    ]
  }
}
```

### Committed-source build verification

```text
release: build command exit0; no source recompilation, existing update-instructions target only
debug: build command exit0; no source recompilation, existing update-instructions target only
diagnostics-release: build command exit0; no source recompilation, existing update-instructions target only
diagnostics-debug: build command exit0; no source recompilation, existing update-instructions target only
Image checks at committed HEAD: PASS, exact hashes above unchanged.
```

## Appendix D — Reproducing the additional analysis

The permanent regression suite is `python3 tests/project_storage/run.py`; all source and mocks are committed at the source SHA. The next two analysis scripts ran only under `/tmp/phase-s4b1` and are included as text to make the image LMA and baseline-object/stack comparisons reproducible. They do not modify firmware, invoke flashing, access a card or publish a binary. Run after the four configured builds. The object comparison uses baseline header bytes obtained via `git show`, unchanged original source files, and the actual Release compile command. Baseline preset source is a temporary file, not a second checkout/worktree. Seven unchanged compiled objects must match; new preset frame measurements are not runtime high-water measurements.

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
# Recreate the following scripts under /tmp/phase-s4b1 if needed.
PYTHONDONTWRITEBYTECODE=1 python3 /tmp/phase-s4b1/image_audit.py
PYTHONDONTWRITEBYTECODE=1 python3 /tmp/phase-s4b1/object_audit.py
git -c core.whitespace=cr-at-eol diff --check
```

### /tmp/phase-s4b1/image_audit.py

```python
from pathlib import Path
import subprocess,re,json,hashlib,struct
root=Path('/workspace/lxr-desktop-app');p=Path('/tmp/phase-s4b1')
out=[]
for cfg in ['release','debug','diagnostics-release','diagnostics-debug']:
 d=root/'build'/cfg;elf=d/'LXR_V2_STM32F765.elf';b=(d/'LXR_V2_STM32F765.bin').read_bytes();img=(d/'LXR_V2_V1.7.img').read_bytes()
 listing=subprocess.check_output(['arm-none-eabi-objdump','-h',str(elf)],text=True);end=0
 for m in re.finditer(r'^\s*\d+\s+(\S+)\s+([\da-f]+)\s+([\da-f]+)\s+([\da-f]+)[^\n]*\n([^\n]*)',listing,re.M):
  name,size,vma,lma,flags=m.groups();size=int(size,16);lma=int(lma,16)
  if size and 'LOAD' in flags:
   assert 0x08008000<=lma and lma+size<=0x08100000
   end=max(end,lma+size-0x08008000)
 magic,n,c,pad=struct.unpack('<8sIB3s',img[:16]);msp,rv=struct.unpack('<II',b[:8])
 assert magic==b'LXRV2IMG' and pad==b'\0'*3 and img[16:]==b and n==len(b)==end and c==255-(sum(b)&255)
 assert 0x20020000<msp<=0x20080000 and msp%8==0 and rv&1 and 0x08008000<=rv&~1<0x08008000+len(b)
 out.append(dict(configuration=cfg,payload=len(b),flash_load_span=end,flash_capacity=1015808,flash_remaining=1015808-end,
  binary_sha256=hashlib.sha256(b).hexdigest(),image_sha256=hashlib.sha256(img).hexdigest(),elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest()))
(p/'image-load-validation.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
```

### /tmp/phase-s4b1/object_audit.py

```python
from pathlib import Path
import subprocess,shlex,json,hashlib
root=Path('/workspace/lxr-desktop-app');work=Path('/tmp/phase-s4b1/objects');work.mkdir(exist_ok=True)
base='910be804c7df2757f8d2347880751f2ca86c671c'
header=work/'include/Preset/PresetManager.h';header.parent.mkdir(parents=True,exist_ok=True)
header.write_bytes(subprocess.check_output(['git','show',base+':Core/Inc/Preset/PresetManager.h'],cwd=root))
commands=json.loads((root/'build/release/compile_commands.json').read_text())
chosen=['Core/Src/stm32f7xx_it.c','Core/Src/Soundengine/Soundengine.c',
        'Core/Src/Soundengine/cs4344_cs5343.c','Core/Src/Soundengine/Sequencer/sequencer.c',
        'FATFS/Target/sd_diskio.c','Core/Src/sysmem.c','Core/Src/safety/runtimeDiagnostics.c']
out=[]
for i,rel in enumerate(chosen):
    assert subprocess.check_output(['git','show',base+':'+rel],cwd=root)==(root/rel).read_bytes()
    item=next(x for x in commands if x['file']==str(root/rel));cmd=shlex.split(item['command'])
    cmd[1:1]=['-I'+str(work/'include'),'-I'+str(root/'Core/Inc/Preset')]
    baseline_obj=work/f'baseline-{i}.o';cmd[cmd.index('-o')+1]=str(baseline_obj)
    subprocess.run(cmd,cwd=item['directory'],check=True,capture_output=True)
    actual=root/'build/release'/item['output']
    a=hashlib.sha256(actual.read_bytes()).hexdigest();b=hashlib.sha256(baseline_obj.read_bytes()).hexdigest()
    assert a==b,(rel,a,b)
    out.append(dict(source=rel,baseline_header_and_source=True,object_sha256=a,identical=True))
item=next(x for x in commands if x['file'].endswith('/Preset/presetManager.c'))
for label in ['baseline','current']:
    src=root/'Core/Src/Preset/presetManager.c'
    if label=='baseline':
        src=work/'baseline-preset.c';src.write_bytes(subprocess.check_output(['git','show',base+':Core/Src/Preset/presetManager.c'],cwd=root))
    cmd=shlex.split(item['command']);cmd[-1]=str(src)
    if label=='baseline':cmd[1:1]=['-I'+str(work/'include'),'-I'+str(root/'Core/Inc/Preset')]
    cmd.append('-fstack-usage');cmd[cmd.index('-o')+1]=str(work/f'{label}-preset.o')
    subprocess.run(cmd,cwd=item['directory'],capture_output=True,check=True)
frames={}
for label in ['baseline','current']:
    fs={}
    for line in (work/f'{label}-preset.su').read_text().splitlines():
        where,size,kind=line.split('\t');fs[where.split(':')[-1]]=dict(bytes=int(size),kind=kind)
    frames[label]=fs
report=dict(objects=out,stack_frames={n:{label:frames[label].get(n) for label in frames} for n in [
    'preset_saveProject','preset_loadProject','preset_LoadTmpKit','preset_loadSongSD',
    'preset_clearTmpFolder','delete_node','empty_directory','preset_loadGlobalFile','preset_loadProjectNames']},
    maximum_local_frames={label:sorted(fs.items(),key=lambda x:x[1]['bytes'],reverse=True)[:8] for label,fs in frames.items()})
(work.parent/'object-stack-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
```

## Appendix E — Publication verification checklist

* Implementation branch: `safety/sd-project-containment-s4b1`.
* Implementation commit: `e49bb028462c339c4998cd08a845db17e1fc778f`; one parent `910be804c7df2757f8d2347880751f2ca86c671c`; tree `842396dafc554d9fb88ec7cd7b48a4d16484aad7`.
* Exact11 changed files match the reviewed local index; no generated firmware/build artifact included.
* Source commit, branch and tree fetched/read back from GitHub; local checkout clean at identical commit. All32 prior remote heads unchanged at source publication.
* Report branch: `codex-reports/2026-10-27-phase-s4b1-project-containment`, based directly on lab-setup `5829398cea5d337200393757e39a37d4d8148256`.
* Report file: `docs/codex-berichte/2026-10-27_phase-s4b1-project-containment.md`. Only this new Markdown file belongs in the report commit. Its exact content/blob, one-parent lab-setup base, branch head and URL must be read back before final completion is claimed.
* Prior branches/reports are not overwritten. No merge, force-push, hardware/card operation, flashing, firmware publication, automatic reset or bootloader change.
* **NOT READY FOR HARDWARE INSTALLATION until S4b-2 is completed and reviewed. NOT FLASHED — NOT HARDWARE VALIDATED.**
