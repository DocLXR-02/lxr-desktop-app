# LXR Firmware Lab — Phase S4b: Project Storage Integrity, Failure Recovery and Transaction Design

Date: 2026-10-27 (UTC). Repository: `DocLXR-02/lxr-desktop-app`.

**Status: read-only investigation and architecture proposal completed; no production correction implemented. NOT FLASHED — NOT HARDWARE VALIDATED.** This report is a review artifact, not authorization to deploy firmware, repair a card, or implement the proposed transaction format.

## 1. Scope, evidence classes and executive findings

The task is to design a small, safer project save/replace/load/recovery architecture after inspecting the complete S1–S4-L2 lineage. Existing file formats, live musical operation, honest errors and conservative treatment of uncertain writes must be preserved. Only this Markdown report is published. Temporary host fixtures use synthetic RAM media or mocks; they never open a physical card. No firmware source, permanent tests, build configuration, previous branch or user data was changed. No worktree, merge, pull request, firmware upload, hardware operation or repair command was used.

Labels used here:

* **CODE**: confirmed in the exact inspected source; not proof of occurrence on a device.
* **HOST/BUILD**: executed host test, sanitizer finding, build or artifact analysis; scope of mocks stated.
* **OBSERVATION**: user-reported physical behavior, not independently reproduced.
* **HYPOTHESIS**: plausible mechanism without evidence identifying it on PROJ00.
* **PROPOSAL/OPEN**: design or unperformed validation, never a claim of implemented behavior.

### Risk ranking

| Priority | Finding and consequence | Evidence |
|---|---|---|
| P0 | Existing-project save deletes the old contents before writing the replacement. A failure can leave no complete version. Unknown files and child directories are deleted too. | CODE, PM:2265,2326; mocked deletion-order/failure tests |
| P0 | `empty_directory(currentFolder)` appends ordinary filenames into a **13-byte global buffer** with no capacity check. `/proj00/prj.nfo` already exceeds it. Longer normal 8.3 names overlap the next linked global. | CODE, PM:45,719,738; two expected ASan failures; ARM ELF symbol locations |
| P1 | Loads are not validated transactions: missing assets retain prior RAM, short metadata and short song data can return success, and earlier mutations are not rolled back on a later error. Startup also discards the project-load result. | CODE, PM:2537,2637; main:826–835; production-body host tests |
| P1 | Unvalidated persisted project/kit indices can reach arrays outside 64 slots. A pattern's associated kit byte can be 255; the actual RAM consumer then has an out-of-bounds access. | CODE, PM:1081,364,2835,860; UBSan consumer and project-name index tests |
| P1 | Global `/tmp` recovery data is deleted before a replacement project is known loadable, and after save without checking deletion success. It is not bound to a project generation. | CODE, PM:624,2656,2456; main:899–907 |
| P1 | Checked individual writers still open existing files with `FA_CREATE_ALWAYS`. Honest failure reporting protects neither the previous file contents nor a whole-project snapshot. | CODE, active opens listed below; S4a–L2 tests |
| P1 | Rename is a multi-step metadata update, not an atomic project commit; duplicate directory references can temporarily point to the same chain. | CODE, FatFs `f_rename`:4724–4812 |
| P2 | Global CWD, shared `FIL`, ignored directory/close results, automatic remount and synchronous work complicate recovery and foreground responsiveness. | CODE; hardware timing OPEN |
| P2 | `preset_init()` initializes only 239 of 247 parameter bytes. Last eight can retain prior RAM. Simply extending the copy would exceed the available initialization payload. | CODE, PM:773–817; production-body test |

**Recommendation:** first authorize a small containment/validation phase that removes unsafe recursive deletion and rejects unsafe indices/paths. For actual replacement, prefer **immutable project generations outside the legacy project directory, verified before a commit record written last, with read-only recovery selection**. Do not rename, truncate or delete the previous committed generation in the save transaction. Begin with save-to-unused-slot as the compatible containment mode if the new namespace has not been approved.

This deliberately does **not** promise an unconditional last-good-copy guarantee under arbitrary FAT-sector corruption or SD-controller failure. Copies on one FAT volume share metadata and media. The strict preservation invariant is achievable only under a stated logical-failure model; independent backups remain necessary. Transparent loading of the newest managed generation by unmodified manufacturer firmware is also not provided by this design. These are review decisions, not hidden implementation details.

## 2. Exact baseline, lineage, instructions and actions

Primary inspected source: [S4-L2 commit 910be804c7df2757f8d2347880751f2ca86c671c](https://github.com/DocLXR-02/lxr-desktop-app/commit/910be804c7df2757f8d2347880751f2ca86c671c), branch `safety/sd-silent-autosave-recovery-s4l2`. The initial remote head matched the requested SHA exactly. All six listed lineage remote heads matched, and each adjacent source ancestry check from lab-setup through S1/S2/S3/S4a/S4-L/S4-L2 returned exit0. The existing checkout was clean and remained on this commit during investigation.

Report base: `lab-setup`, **5829398cea5d337200393757e39a37d4d8148256**. Main was **a43cd4fcda23ec3d577dc425261c3eb28bd5abff** and was not checked out or changed. `git merge-base --is-ancestor 5829398cea5d337200393757e39a37d4d8148256 910be804c7df2757f8d2347880751f2ca86c671c` succeeded. Remote heads were inspected before conclusions; final verification is recorded in the publication section.

| Phase | Source branch | Verified expected implementation commit |
|---|---|---|
| S1 | `safety/sd-storage-foundation-s1` | `a4be3b94c552dbc867b197fad76655bed40cea34` |
| S2 | `safety/sd-hal-bsp-timeouts-s2` | `c88933a4b1968c8f9d173b71d86154e8969436be` |
| S3 | `safety/sd-fatfs-integrity-s3` | `cbffa2d756658b6f32d8e6417ea40ec2cda828d5` |
| S4a | `safety/sd-save-errors-s4a` | `c87666ff945e2e01cbbf1ceda72fb503638de579` |
| S4-L | `safety/sd-live-error-policy-s4l` | `feaff649599472036390ecef24f7193b7cc2e2ab` |
| S4-L2 | `safety/sd-silent-autosave-recovery-s4l2` | `910be804c7df2757f8d2347880751f2ca86c671c` |

The complete `AGENTS.md` from lab-setup was read (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`). It requires diagnostic discipline, exact references, complete handoff and verification of a documentation-only publication. The cloud environment runtime skill was read for environment/network rules. No delegation was used. GitHub connector reads/publication were used; no credentials are included here.

Complete prior reports read:

* S1: `codex-reports/2026-10-26-phase-s1-sd-storage-foundation`, `docs/codex-berichte/2026-10-26_phase-s1-sd-storage-foundation.md`, blob `e1c646c3e588df061ed4df183e87e5822aaa9f79`.
* S2: `codex-reports/2026-10-26-phase-s2-sd-hal-bsp-timeouts`, corresponding `2026-10-26_phase-s2-sd-hal-bsp-timeouts.md`, blob `9b44e0ef9bc1c8585e04fbe05249ab5fc691b81e`.
* S3: `codex-reports/2026-10-26-phase-s3-fatfs-data-integrity`, corresponding `2026-10-26_phase-s3-fatfs-data-integrity.md`, blob `88ab6565c42881762d47f9be819f9c50ae5e3e76`.
* S4a: `codex-reports/2026-10-26-phase-s4a-safe-file-saving`, **`2026-10-26_phase-s4a-safe-file-saving-verified.md`**, blob `15916ed38af3af63d65538ea8933d91de67a05dd`.
* S4-L: `codex-reports/2026-10-27-phase-s4l-live-safe-storage-recovery`, `2026-10-27_phase-s4l-live-safe-storage-recovery.md`, blob `64db7ab625457a4876483b522e9483d2fa352eef`.
* S4-L2: `codex-reports/2026-10-27-phase-s4l2-silent-autosave-recovery`, `2026-10-27_phase-s4l2-silent-autosave-recovery.md`, blob `8bbcb4d454d31fd53c03c1dfc33be2a7bc719c0e`.

Prior safeguards remain relevant, not superseded:

* S1 rejects unsuitable execution contexts, uses a 5,000 ms transfer argument and a separate 5,000 ms readiness allowance, and latches serious/uncertain disk faults until reboot. Status/init/remount cannot clear that latch. These are development timing policies, not qualified guarantees for every card.
* S2 bounds active SD command waits and improves timeout/error/state fidelity. It does not establish physical card quiescence or authorize reset/retry after ambiguity.
* S3 checks FAT mirrors/FSInfo and initialization status; synchronization/close failures remain visible. It does not make metadata writes transactional.
* S4a checks individual payload writes, counts and closes. Uncertain shared-file ownership remains quarantined rather than force-closed or overwritten.
* S4-L allows harmless failed opens to be retried and uses non-blocking manual errors. S4-L2 makes autosave silent, keeps independent dirty/retry state, backs off safe failures at 2/4/8/16/32 seconds, and never replays uncertain writes. Preserve these behaviors and the unrelated Phase A–C corrections.

For original-source comparison, the publicly released repository source as preserved on lab-setup was examined. Normalized function-body comparisons show `empty_directory`, `delete_node`, `preset_init` and `preset_makeFileNameSave` unchanged there versus S4-L2. Destructive project replacement also predates the safety lineage; later functions gained guards/error handling without a new project transaction. This is **repository-source comparison only**. The source calls itself **1.7**. Official public v1.71 remains an external reference; neither it nor the private v1.72 binary nor the user's currently installed image was obtained/examined here. No equivalence is asserted.

## 3. Source reference convention and complete execution map

`PM:n` means `Core/Src/Preset/presetManager.c:n` at commit 910be804. `main:n` and `menu:n` mean `Core/Src/main.c:n` and `Core/Src/menu/menu.c:n`. `ff:n` means `Middlewares/Third_Party/FatFs/src/ff.c:n`. These are source line numbers, not report lines.

Stable source entry points:

* [presetManager.c at the baseline](https://github.com/DocLXR-02/lxr-desktop-app/blob/910be804c7df2757f8d2347880751f2ca86c671c/Core/Src/Preset/presetManager.c)
* [main.c](https://github.com/DocLXR-02/lxr-desktop-app/blob/910be804c7df2757f8d2347880751f2ca86c671c/Core/Src/main.c)
* [menu.c](https://github.com/DocLXR-02/lxr-desktop-app/blob/910be804c7df2757f8d2347880751f2ca86c671c/Core/Src/menu/menu.c)
* [FatFs ff.c](https://github.com/DocLXR-02/lxr-desktop-app/blob/910be804c7df2757f8d2347880751f2ca86c671c/Middlewares/Third_Party/FatFs/src/ff.c)

### Startup, mounting and selection

| Stage / function | Path and lines | Actual behavior / caveat |
|---|---|---|
| Registration | `FATFS/App/fatfs.c:30,53`; `ff_gen_drv.c:40–109` | Link SD driver; ReInit unlinks/relinks generic registration, not a filesystem transaction or release of uncertain application ownership. |
| Initial mount | main:367 | Eager FatFs mount. S1/S3 initialization remains authoritative. On FR_NO_FILESYSTEM, main:384 enters existing format-confirmation UI; see note below. |
| Root/temp/global setup | main:456–475 | `/tmp` stat/mkdir/chdir and global initialization; some setup results ignored. Startup is not purely read-only. |
| Project-name scan | main:817–822 → PM:2594 | Scans 64 `/projNN/prj.nfo` files. Missing folder gets “Empty”; errors/open/close/CWD are not reliably propagated by void helper. |
| Last project identity | main:826 → PM:1081 | Root `/proj.idx`, one byte; short read/close and index bound not fully validated. |
| Boot load | main:828–835 → PM:2637(clearTmp=0) | **Result not assigned to `res`**; unrelated earlier `res` drives error/UI identity. Thus a failed load need not prevent a displayed loaded project identity. |
| Temp overlay | main:899–907 → PM:204,303,392 | Global `/tmp`: manually named assets, then `tmp.snd`, then `tmp.pat`; results ignored and dirty timers reset. Overlay not identified with loaded project/generation. |
| Foreground hot insertion | main:923–940 | Unmount, zero `SDFatFS`, remount, rescan names. No coordinated application-file disposal. Cannot clear S1's fault; not proof of safe recovery. |
| Normal foreground service | main:951–992 | Audio service interleaved with other work; `preset_processTimer()` at 992. Autosave is foreground, no new interrupt-side filesystem activity. |

### Create, edit, manual save, autosave and full save

| Function / caller | References | Path / ownership / result |
|---|---|---|
| Project selection/load | menu:2258–2280 | Calls load with clearTmp=1; **any `FR_NO_FILE` creates a fresh RAM project**, even when folder exists but required metadata is missing. On reported success updates loaded identity. |
| `preset_createProject` | PM:2215–2259 | Resets kit/pattern/song RAM and names, ignores temp deletion result, returns FR_OK; does not itself persist a complete project. Uses unvalidated project index. `preset_init` leaves last eight kit bytes stale. |
| RAM kit store/load | PM:860,891 | Copies active kit to/from slot; RAM-load consumer has no slot bound. Must distinguish RAM editing from SD persistence. |
| RAM pattern store/load | PM:2059,2081 | Existing RAM/backup buffer transfers; not project-wide rollback snapshots. |
| Edit scheduling | PM:264–282 | Kit/pattern dirty and inactivity timestamps. Existing pattern-start cancellation deliberately clears pending pattern autosave; not a comprehensive content-version hook. |
| `preset_processTimer`, autosave helpers | PM:649–713 | Silent, independent kit/pattern pending state, safe-open cooldown, fault/ownership holds. No notifications, no continuous SD polling. |
| `preset_saveTmpKit` | PM:285–301 | Absolute `/tmp/tmp.snd`, slot byte + active kit bytes; checked writer and close, direct truncating replacement. |
| `preset_saveTmpPat` | PM:555–575 | Absolute `/tmp/tmp.pat`, slot byte + pattern data; direct truncating replacement, no new file-format fields. |
| Named kit writer | PM:821,924 | `preset_writeDrumsetData`, `preset_saveDrumsetSD`: 8-byte name + 247 parameters; checked writes/close. |
| Named pattern writer | PM:1573,1663 | `preset_writePatternData` now returns FRESULT; 8-byte name, six-byte PatV03 tag, ordered pattern body; checked every payload write. |
| Song writer | PM:2579 | 8-byte name + 64 song bytes; checked. |
| Globals writer | PM:942,955 | 30 active global bytes, relative `glo.cfg`; checked write/close, no transaction replacement. |
| Root identity writer | PM:1104 | Absolute `/proj.idx`, one byte, checked truncating replacement. |
| Shared file helpers | PM:107–170 | One `SDFile`; uncertain write/short write/close sets app failure; no force-close/err clearing/reuse. Logical safe failed opens do not universally latch. S1 remains a separate lower-level latch. |
| Project-save menu | menu:2096–2118 | Calls save and displays returned error. Still executes `menu_resetSaveParameters()` after project case even when save failed; do not infer all project-only state is success-gated. No U1 change proposed here. |
| Full project writer | PM:2265–2458 | Save active RAM buffers; start TIM2 save-time rendering; disable display transfer processing; spin until display buffer ready; stat `/projNN`; mkdir or recursively empty existing directory; chdir; nfo, globals, then 64 times kit/pattern/song; re-enter `/tmp`; delete `/tmp`; return FR_OK. Detailed matrix follows. |

### Load, parsing, file naming and deletion

| Function | References | Behavior / validation limits |
|---|---|---|
| `preset_loadProjectNames` | PM:2594–2634 | Relative metadata access after CWD change; read/open/close errors not returned; project index must be 0–63 but is not checked. |
| `preset_loadProject` | PM:2637–2866 | Clears `/tmp` before new project validation if requested; creates missing folder; metadata required but short metadata tolerated; globals result ignored; missing individual files tolerated without deterministic default fill; RAM updated piecemeal; index write side effect; no rollback. |
| Named kit reader | PM:1117,1157 | SFN wildcard then old filename fallback; name length checked; short parameter tail deliberately zero-filled for legacy compatibility; later close result ignored. |
| Named pattern reader | PM:1685,1979 | Version handling includes legacy untagged/older tagged files; data mutates RAM before completion. Some rewind/close outcomes ignored. Avoid imposing V3-only checks on all legacy files. |
| Track length / kit tail | PM:364–389 | Succeeded Euclidean fix: legacy zero→16, valid 1–64, safe malformed length handling. **Associated kit byte has no <64 validation.** |
| Named song reader | PM:2537–2577 | Only nonzero name read demanded; payload byte count ignored, close ignored; can report FR_OK with stale song tail. |
| Globals reader | PM:967–1004 | Byte-count check but read result handling incomplete; caller ignores boolean failure. A short/failed load can leave earlier settings. |
| Temp kit / pattern readers | PM:303–352,392–549 | Kit slot byte unchecked; temp pattern index checked. Temp pattern publishes several active identities before entire payload validated. Both need deliberate incomplete-read semantics. |
| `preset_makeFileNameLoad` | PM:3012–3058 | First `NN-*.ext` match in directory order; scan errors collapsed to FR_NO_FILE; caller may try legacy filename. Does not validate uniqueness. S1 can still stop a fallback after physical fault. |
| `preset_makeFileNameOld` | PM:2981 | Legacy `pNNN.snd/.pat/.sng` etc., retained read compatibility. |
| `clearIllegalCharacters`, `preset_makeFileNameSave` | PM:3060,3090 | SFN `NN-` + first five sanitized name characters + extension; fixed 8-byte display name stored inside file. Name change can leave two files for same slot. Wildcard load may pick older one. |
| `empty_directory` | PM:719–750 | Unbounded append, recursive depth-first traversal, deletes every non-dot file/subdir, ignores directory-close result. Enumeration order is FAT on-disk order, not guaranteed filename/type order. Leaves root directory itself. |
| `delete_node`, `preset_clearTmpFolder` | PM:582–642 | Recursively deletes entire `/tmp` including directory; buffer guard is ineffective on overflow; unchecked root chdir and reinitialization; directory-close errors ignored. |
| `preset_initTmpFolder` | PM:174–201 | Void mkdir/chdir setup helper, shared guard, does not reliably surface all CWD/mkdir failure. Global path and relative save callers create ownership coupling. |
| Rename/unlink semantics | ff:4532,4724 | No active project rename transaction exists. Bootloader-only rename/unlink code in main is under `UPDATE_BOOTLOADER=0`, not normal project recovery. |

Additional lifecycle notes: `sd_formatRoutine` at main:108–185 is an existing **user-confirmed format path** reached after startup FR_NO_FILESYSTEM (main:384), with blocking button-confirmation loops and `f_mkfs` at main:145. It is not an unconditional automatic format, but must never be invoked by the proposed transaction recovery. Its later `preset_saveProject(0)` result is ignored before a Success/reboot message. This path was inspected only, never executed; no card was formatted. Startup also retains HAL_Delay(1000) error screens, distinct from the non-blocking S4-L manual notification helper. `menu_resetSaveParameters` (menu:3130) resets dialog edit state/repaints, not dirty flags; the concern at menu:2118 is misleading completion workflow, not a demonstrated clearing of every RAM dirty flag. `buttonHandler.c:1153–1175` contains a checked direct named-pattern save; `sequencer.c:650` is another associated-kit RAM-load consumer, reinforcing the need to validate the persisted kit index before playback.

Storage execution chain remains: application `f_open/write/read/close` → FatFs window/FAT/data/sync → generic `disk_*` registration → `FATFS/Target/sd_diskio.c` → BSP → SD-specific HAL/LL. `_FS_REENTRANT=0`, `_FS_LOCK=1`, one volume, `_USE_LFN=0`, `_FS_RPATH=1`, exFAT disabled. A global CWD and one file lock cannot safely support ad hoc nested/multi-owner filesystem work. Future chunking needs explicit single foreground ownership; no SD calls from audio/DMA interrupts.

## 4. Confirmed destructive-save and memory defects

### 4.1 Cleanup before replacement and buffer overflow

The existing-directory branch calls `empty_directory(currentFolder)` at PM:2326 before nfo/globals/assets are written. The routine recursively unlinks everything except dot-prefixed directory entries. It has no whitelist for kits/patterns/songs, no backup, no transaction marker and no limit on recursion depth. Unknown user files are treated as disposable. Ordinary FatFs errors stop traversal after earlier deletions have already committed; there is no rollback.

`currentFolder[13]` can hold `/proj00`, but not a full child path:

* `/proj00/prj.nfo` needs **16 bytes including NUL** (15 visible characters), three beyond the object.
* `/proj00/00-KICK1.SND` needs **21 bytes including NUL**, eight beyond it.

ASan proves the out-of-object write at PM:738 using the actual extracted function, real headers and mock directory entries. For the deletion-order control, the caller supplies a larger safe buffer so that traversal can be observed independently of the known overflow; it is not evidence that the actual 13-byte call is safe. It does not prove corruption of a physical FAT. Release ELF links `currentFolder` at `0x20025ee8` (size 13) and `savefileNames` at `0x20025ef8` (size 2048): three padding bytes separate them. The first case crosses C object bounds into padding; the second can overwrite five bytes of the name array. Layout could differ under another build. Both are C undefined behavior and must be fixed before exercising existing project overwrite on hardware.

`delete_node` is a separate unsafe cleanup routine: after detecting `i+j >= sz_buff`, it sets error 100 but still calls recursion/unlink, replacing that result. A mocked overlong path produced an attempted unlink despite the guard. It also appends the slash before checking capacity. Merely increasing `currentFolder` is insufficient: require capacity-aware absolute path builders, valid 8.3 components, bounded depth and **no operation after path construction failure**. The safer project architecture should remove generic recursive deletion from save/load entirely, not just enlarge this buffer.

### 4.2 Remaining metadata false-success and ownership issues

Project nfo writes at PM:2378 bypass `preset_writeChecked`: `bytesWritten` is never compared to 8, and close at 2391 is ignored. A write returning FR_OK with fewer than eight bytes can therefore be followed by checked child saves and ultimately a project FR_OK. In contrast, a close failure generally sets the shared failure latch; the following globals writer returns that failure, so **do not claim every ignored project close still reaches FR_OK**. It is nevertheless indirect, incomplete propagation. A raw write error returns without a safe close; `SDFile.obj.fs` can remain live, preventing further use. Do not “repair” that by force-closing or clearing FIL.err.

The stat branch treats every result other than FR_NO_FILE as if the directory exists. A new-directory chdir is unchecked. If chdir fails, relative metadata/assets can target the previous CWD. Serious disk failures may stop later access through S1; logical path failures do not automatically make CWD safe. Root `/tmp` chdir/mkdir errors are also ignored in several helpers. All paths in a new transaction should be explicit, bounded absolute paths with verified parent attributes; never infer a usable directory from an unrelated stat error.

### 4.3 Invalid indices, truncated loads and stale defaults

Actual production-body tests show:

* Root project-index reader accepts byte 255; it can later reach a project-name array. `preset_loadProjectNames(255)` with a missing directory produces an expected UBSan array-index failure at PM:2606.
* Temp kit reader accepts/publishes slot 255.
* Pattern length tail accepts associated kit 255; actual `preset_loadDrumsetRam(255,0,0)` produces UBSan at PM:870. This is a demonstrated source/host memory defect, **not a demonstrated device exploit or crash**.
* `preset_loadProject(0,0)` returns FR_OK when nfo reads eight bytes but all 192 kit/pattern/song files return FR_NO_FILE; prior RAM remains. It also returns FR_OK after a **zero-byte** nfo read in the fixture. Existing comments explicitly permit incomplete projects, so the remedy must distinguish deliberate legacy omissions from truncation rather than simply banning every incomplete layout.
* Song reader returns FR_OK with an eight-byte name and zero payload bytes, retaining previous 64-byte song data.
* `preset_init` copies indices 8..246 of initKit into parameter indices 0..238, leaving indices 239..246 unchanged (except any separately normalized fields). `initKit` has only 246 payload bytes after its name. A blind upper-limit extension to copy all 247 parameters would read past that payload; use reviewed explicit defaults, with a regression for every parameter.

Missing assets, failed reads, unavailable globals, partially applied patterns and boot/temp overlays can combine to make a “load” sound plausible without proving a complete project was read. A successful audible kick is not an integrity check for the directory or the other 193 files.

## 5. Stage-by-stage current failure and data-loss matrix

The directory-deletion order is arbitrary on-disk enumeration, depth first. Replacement order is fixed: `prj.nfo`, `glo.cfg`, then `kit[i]`, `pattern[i]`, `song[i]` for i=0..63. Failures at each stage are distinct:

| Stage | What may already have changed | Failure outcome / error fidelity | Last-good / retry implications |
|---|---|---|---|
| Capture selected RAM buffers | Current slot/backup RAM updated | Void/internal state not project transaction result | Disk unchanged; RAM snapshot not frozen against all edits |
| Display drain / TIM2 switch | Display processing suspended, save-time rendering enabled | Display-buffer spin has no project-specific deadline | Not an SD timeout; foreground can wait independently of storage safeguards |
| stat / mkdir / chdir | New directory may exist | Non-NO_FILE stat errors enter existing branch; new chdir unchecked | Wrong-CWD risk; logical preflight failure can be retryable only with valid ownership/path |
| First unlink | Old file removed, FAT/dir metadata changed | Checked error for current unlink only | Previous complete project already lost; no future new writes needed to cause loss |
| Middle cleanup / nested dirs | Arbitrary subset removed, unknown files included | First unlink/readdir failure stops; path overflow may corrupt RAM first | No automatic way to reconstruct deleted assets; retry deletes more, not rollback |
| Cleanup ends | Existing folder empty; root remains | closedir ignored | Power loss now leaves an apparently empty project directory |
| nfo open | Existing name truncated/new entry created | Open checked | Old metadata gone; new zero-length file possible |
| nfo write | 0..8 bytes, metadata possibly pending | FRESULT checked, **byte count ignored**; failure can retain shared object | Short FR_OK can masquerade as complete; uncertain failures must not replay |
| nfo close | Some data/dir/FAT writes may have occurred | Return ignored locally, app latch generally catches later | Physical completion not proven; no old backup |
| globals | New 30-byte file complete/partial | Checked writes/close; failure returned | Prefix only; no asset set. Corrected S4-L returns before later writes/temp deletion |
| kit[i] | Prior slot triplets complete, current kit partial/complete | Checked S4a writes/close | Mixed/incomplete project; other missing slots may later keep stale RAM |
| pattern[i] | Kit[i] may exist, pattern partial | Checked writes/count/close | No per-project rollback; legacy loader cannot know it is interrupted save |
| song[i] | Prior files plus current pair | Checked writes/count/close | Same; no metadata-level transaction ID |
| final child close | Nominally all 194 payloads written | Checked child results, but no complete-set readback | API success ≠ independent power-loss durability proof |
| return to `/tmp` | CWD/setup may fail silently | Void helper | Subsequent relative operations can be misplaced |
| delete `/tmp` | Old recovery files removed, possibly partial deletion | Return ignored at PM:2456 | Can lose independent recovery state; returns project FR_OK despite cleanup failure |
| menu completion | Save dialog reset | Error displayed for returned failure, not hidden errors | Must not interpret dismissed/reset dialog as persisted project |
| subsequent retry | Existing incomplete folder selected | Cleanup repeats; S1/shared latch may forbid all physical work | Harmless pre-open failures can be retried, but uncertain write/owner state cannot safely be unlocked |
| reboot | RAM lost; partial disk and surviving `/tmp` persist | Boot ignores load result and temp-load errors | No commit detection; “loaded” identity may not match a validated whole project |

Load-specific failure stages:

| Stage | Confirmed effect | Required future boundary |
|---|---|---|
| clearTmp before selecting target | Recovery data deleted before target validation | No deletion during validation/load |
| absent target / missing nfo | Directory may be created; manual caller can create new RAM project | Separate explicit creation from invalid/missing existing project |
| short nfo / globals failure | Label can become Empty; old globals retained; continue | Validate count/result; deliberate defaults only for documented legacy profile |
| missing asset | Prior slot survives; no missing-set inventory | Deterministic initialization/reported omission; strict managed generation |
| mid-pattern/read failure | Some RAM already changed | Per-file scratch, validation before publication; honest partial-load outcome if full rollback impossible |
| associated kit out of range | Invalid RAM access possible | Validate every persisted index before array access |
| project-index save at end | Root file truncated/written even though this was a load | Selection persistence separate from successful content load; no success masking |
| temp overlay | Potential unrelated project data overlaid | Project/generation-bound recovery metadata; never silent cross-project overlay |

### Blast radius

Direct recursive deletes are scoped by the constructed path, not by a recognized project file inventory. With valid paths/healthy filesystem they target `/projNN` descendants and global `/tmp`; they do not intentionally delete other `/projMM` folders. However, path UB and unchecked CWD remove confidence in intended ownership, FAT/FSInfo/root sectors are shared, and deleting a cross-linked chain on an already damaged filesystem can harm unrelated files. No claim is made that such cross-link damage occurred on the user's card. Do not follow directory anomalies with automatic repair/deletion.

## 6. Individual-file replacement inventory

All following **active** `FA_CREATE_ALWAYS` paths truncate the target before later checked writes/close prove completion. Error reporting is substantially improved by S4a–L2, but previous contents can be lost:

| File / writer | Reference | Existing-content risk |
|---|---|---|
| `/tmp/tmp.snd` | PM:285–301 | Latest temporary kit recovery can be replaced with incomplete data |
| `/tmp/tmp.pat` | PM:555–575 | Latest temporary pattern recovery can be lost; uncertain result must remain silent/held |
| Named `.snd` | PM:924 | Existing same filename lost; different display name creates another file for same slot |
| `glo.cfg` | PM:955 | Current directory's saved settings truncated |
| `/proj.idx` | PM:1104 | Last-project selector can be empty/invalid; must not become unchecked array index |
| Named `.pat` | PM:1663 | Existing pattern truncated, or older differently named file remains wildcard-visible |
| `prj.nfo` | PM:2364 | Project label/required metadata lost before project completion; unchecked short write |
| Named `.sng` | PM:2579 | Existing song truncated |

Disabled legacy writers around PM:1632 and 2460 are not active persistence paths. This report does not propose modifying them as if enabled. No general project rename/backup exists today.

Unification opportunity: reuse a bounded checked-file writer, immutable destination policy, explicit ownership, content digest/length validation and commit-selection rules for both whole projects and later individual replacements. Do not immediately convert every autosave into a heavyweight 194-file transaction. Preserve silent bounded retries and treat autosave versioning as a separate reviewed unit. A per-file generation scheme can later use the same validation primitives, but must solve exact-slot selection and backup lifetime; `temp→oldname` rename alone is not a complete solution.

## 7. PROJ00: observation, hypotheses and nondestructive evidence plan

### Reported facts, not reproduced here

User reports `/PROJ00` appears empty in iPhone Files, copying fails with code **-36**, other projects remain visible/copyable, and LXR can load distinctive Voice 1 kick/four hits over sixteen steps/Drive–Morph settings. Many restarts and use of other projects occurred. Saving PROJ00 again did not restore host visibility. Exact installed firmware/binary correspondence is **unknown**. No card, card image, reader log, filesystem geometry, error-domain definition or sector dump was supplied. Error -36 alone does not identify FAT corruption, a specific driver or a particular errno/OSStatus domain.

### Ranked explanations

| Relative plausibility | Mechanism | Supporting / limiting evidence |
|---|---|---|
| Higher, still HYPOTHESIS | A readable early portion of a directory contains files found by targeted SFN searches, but a later directory cluster/sector or chain is unreadable/inconsistent. Host enumeration/copy reaches the bad part. | Whole-production FatFs RAM test proves **named read + early wildcard search can succeed while complete enumeration errors**. It does not emulate iOS or prove all 194 project files are readable. Required entries could in principle precede the bad tail. |
| Higher, unresolved | Local card/reader/adapter/provider I/O failure, unstable sector reads, SD controller/FTL issue, host cache/error policy | Other directories working does not exclude localized media or metadata faults. No hardware evidence to discriminate. |
| Moderate | FAT copies disagree; implementations use or validate different chains, active-FAT/metadata policies differ | Source `get_fat` reads the primary FAT; synthetic conflicting-copy test confirms. Actual card FAT type/BPB and iOS policy unknown. S3 reports mirror failures but cannot undo primary-success/secondary-failure divergence. |
| Moderate | Directory attributes, malformed LFN entries, invalid SFN metadata or host filtering/validation differ | Production `_USE_LFN=0` ignores LFN entries and accepts hidden/system SFN files. Synthetic test demonstrates this tolerance. Ordinary hidden files alone do not satisfactorily explain a copy error; successful recreate often resets attributes, so “hidden only” is weak. |
| Moderate to lower as attribution | Interrupted destructive save, partial metadata update, or path-buffer UB left persistent anomalies | The source can destroy/partially replace a directory and corrupt RAM. **Installed source unknown**; no causal link to PROJ00 established. |
| Partial alternative | Apparent load uses stale RAM, ignored startup result, incomplete-project tolerance or global `/tmp` overlay | Source/test mechanisms confirmed. Reboots weaken RAM-only explanation; persistent `/tmp` can cross reboot, but successful manual project switches normally clear it. Does not by itself establish why host copy fails. |
| Weak alone | Directory is simply empty with normal zero end-marker before entries | RAM test shows both ordinary named lookup and wildcard search then miss the same files. A genuinely empty normal directory cannot by itself explain repeated successful same-path disk reads. Fallback/overlay would be an additional explanation. |

No filesystem corruption, hardware recovery, malicious input or specific firmware fault is diagnosed. Resaving not helping could reflect a failed save, unchanged bad chain, wrong path/overlay, reader error or caching. It is not a reason to keep resaving the original card.

### Future evidence acquisition — separate explicit authorization required

Do not change the original card to test a theory. Do not run repair-enabled fsck/chkdsk, format, delete, rename or another firmware save as an experiment. Preserve the original; if feasible use a genuinely read-only acquisition path and obtain a complete block image plus cryptographic digest and acquisition errors. A mechanical lock tab alone is not universal proof of write prevention. Work only on clones/images thereafter; prevent automatic OS repair/indexing writes.

Record reader/adapter/device and iOS error context; identify FAT type, cluster size, BPB, FAT copy flags and project directory start cluster. On copies, enumerate raw 32-byte directory entries, attributes, SFN/LFN checksums, `.`/`..`, end/deleted markers; compare FAT chains/copies for loops/cross-links/free-vs-allocated/out-of-range clusters; compare targeted SFN reads with complete traversal and a second read-only filesystem implementation. Hash repeated reads and inspect I/O error offsets. Inventory `/tmp` and `/proj.idx` without altering them. Compare actual metadata/files with a known project copy. Any recovery extraction goes to a different destination; no in-place repair recommendation follows automatically.

## 8. Candidate transaction designs and power-loss analysis

No candidate may rely on rename atomicity, successful API acceptance as absolute durability, or automatic replay after ambiguity.

| Candidate | Last-good behavior through interruption | Compatibility / costs / residual risks | Decision |
|---|---|---|---|
| Keep old directory; save only to unused project slot | Old slot never intentionally changed; incomplete new slot can be ignored | Existing 64-slot/legacy format compatible; needs accurate validation, checked paths and zero cleanup. Slots/space finite, no transparent same-slot replace. FAT/media shared risks remain. | **Smallest initial containment** while full design is reviewed |
| File temp + backup/rename | At best protects one file; interrupted multi-file update mixes versions. Renames can alias same chain or lose names | Small RAM, extra file/directory slots, repeated metadata I/O, rollback requires per-file state; old firmware sees mixed/current canonical files | Reject as sole project transaction |
| Stage directory; rename old to backup; rename new to canonical | Removes canonical name during commit; moving only old reference is a danger. Repeated interruption requires careful alias detection | Convenient legacy path after success, roughly one full extra project, multi-step FAT directory/parent updates, not atomic directory swap | Not preferred without stronger filesystem guarantees |
| **Immutable generations + verified manifest + commit record last** | Old canonical/generations untouched by transaction; new ignored until full validation and commit-record confirmation; read-only selection can be idempotent | Extra full-generation space and directory scans, new resolver/ancillary format, bounded fixed workspace. Old firmware sees legacy canonical, not newest managed generation. Shared FAT/card damage still possible | **Preferred minimal full replacement architecture**, subject to compatibility decision |
| Preallocated dual-slot container / custom store | Can reduce normal FAT allocation metadata updates; selection still needs durable records and validation | Large up-front reservation/migration, random update protocol, incompatible container or complex legacy export; hardware sector/FTL limits remain | Future alternative if latency/space measurements reject file generations; not initial S4b |

FatFs `f_rename` (ff:4783–4812) registers a destination, copies starting cluster/attributes, may change `..`, removes old entry, then syncs. Source explicitly labels a critical region where interruption can cause a **cross-link**. Two names temporarily sharing a cluster chain are **not two independent backups**; deleting either can free the other's data. A same-sector torn update can affect both names. No proposed automatic cleanup may assume duplicate names are independent objects.

## 9. Recommended bounded architecture — proposal, not implementation

### 9.1 Storage contract and compatibility decision

Use a separate reserved namespace, for example:

```text
/PROJ00/                         existing legacy project, never modified by new-generation save
/LXRSAFE/PROJ00/G0000001/         immutable managed generation (all path components 8.3)
    prj.nfo                     existing 8-byte payload
    glo.cfg                     existing 30-byte payload
    NN-NAME.snd/.pat/.sng        all 64 slots, unchanged successful file bytes
    MANIFEST.BIN                new ancillary validation record
    COMMIT.DAT                  new ancillary commit record, created last
```

The numbers and ancillary schema below are a concrete starting contract for review. They are **not a new implemented on-disk format**. Choose one reserved root, validate existing collisions/attributes, and refuse unexpected contents rather than taking ownership. Never put backups inside `/PROJ00`, where old recursive save would erase them. Existing original firmware normally ignores `/LXRSAFE` but cannot select its latest generation. It continues to see `/PROJ00`. Firmware downgrade or external edits to that canonical copy require conflict detection and explicit selection/import; do not silently shadow a newer legacy copy. Record a canonical import fingerprint and report ambiguity rather than automatically reconcile it.

If unmodified official firmware must transparently load the latest replacement, this namespace choice is incompatible with that requirement. Do not hide it by automatically overwriting canonical files after commit: that reintroduces the unsafe operation. An explicit separately reviewed export to a **new unused legacy slot** is safer. Current source compatibility and binary/manufacturer compatibility are separate questions.

### 9.2 Invariants and failure model

1. Establish which baseline is actually known-good: validate a prior managed generation or a documented legacy profile before promising fallback. A directory name, audible kick or `prj.nfo` alone is not that evidence. If no candidate validates, report no verified fallback and preserve evidence/RAM; never relabel a damaged project as known-good. Transaction never deletes, renames, truncates, opens-for-write or frees the previous committed project's files/chains intentionally.
2. New paths are exclusive-create, uniquely owned by this transaction, bounds-checked and never reused after ambiguous completion. No automatic overwrite/retry of an uncertain file.
3. A generation is selectable only if commit record, manifest, expected file inventory, exact sizes, content digests and semantic bounds all validate. Merely having a directory or `prj.nfo` is insufficient.
4. No reported save success until every required write/count/close/sync/readback and the final commit confirmation succeeds. A persistence API success is still not a power-failure guarantee.
5. Recovery selection is read-only and repeatable. Uncommitted/invalid candidates are not automatically deleted or replayed. No repairs, formatting, peripheral reset, latch clearing or reboot loop.
6. Dirty RAM is cleared only for the exact captured content version successfully committed. Edits during save remain dirty; a failed transaction never claims a recovery file exists.
7. Safe logical preflight failures may be explicitly retried with new clean ownership; ambiguous physical/metadata failures retain S1/S3/application protection. Live audio/sequencer/RAM operation is not deliberately stopped by an error.

**Conditional preservation guarantee:** under failed/short/reordered/interrupted *new logical operations* with an otherwise consistent readable filesystem and no corruption of unrelated persisted sectors, old committed content remains reachable and recovery selects it unless a new fully valid generation exists. **Counterexample:** a torn/shared FAT sector or arbitrary SD-controller corruption can invalidate chains of old and new files together. There is no unconditional same-card last-good guarantee. CRC/readback detects many errors; it neither authenticates hostile data nor proves physical flush/sector atomicity. The host model explicitly includes this counterexample.

### 9.3 Proposed transaction state machine

| State | Permitted work | Error / power-cycle result |
|---|---|---|
| PREFLIGHT | Validate slot0–63, root attributes, ownership, space estimate, source epoch; reserve single foreground owner | No payload changes; logical failure can be retried. No SD polling loop or deletion |
| STAGING | Create never-used generation directory and exclusive files; serialize existing bytes once; checked writes/count/close | Partial new generation uncommitted; old selected. Any uncertain result holds writes; no replay |
| VALIDATING | Reopen each new file read-only; exact length, digest, semantic checks; construct/verify manifest | Failure leaves uncommitted candidate, RAM dirty; no cleanup to “make room” |
| COMMITTING | Confirm source epoch unchanged; exclusive-create fixed commit record last; checked close/sync/readback | Partial/invalid record ignored. If API failure after record may have reached card, outcome **uncertain**, not success or automatic retry |
| COMMITTED | Full revalidation/confirmation establishes new generation per software contract | New selected; old retained. RAM dirty reset only for matching epoch |
| HOLD | Preserve evidence/ownership/latches after ambiguous result | No reset/remount/force-close; manual RAM work continues; controlled reboot/inspection may be necessary |
| RECOVERY SELECTION | Read candidates, validate, choose newest unambiguous complete generation, else prior/canonical | No writes/deletes. Repeated interruptions choose same validated result on same bytes |

Use a monotonic generation identifier with checked exhaustion; never wrap/recycle a name. Example `G0000001` has seven decimal digits, not unbounded time/order metadata. No invented clock timestamps. Candidate-count and traversal limits must be explicit (e.g. review a cap of 32 generations per project and bounded directory entries); exceeding a limit returns maintenance-required instead of a scan/delete loop. Bound discovery across 64 slots, avoid a full verification scan for every UI repaint, and do full verification only for candidate selection/explicit access. Never trust a cached candidate without revalidating relevant identity and volume ownership.

Initial implementation should **not garbage collect automatically**. At capacity/full media, refuse the new save truthfully while retaining old versions/RAM. A later explicit maintenance operation can retire only manifest-owned generations after retaining at least current + previous fully validated independent generations. Unknown files, cross-links, directory errors or suspect media mean no deletion. The maintenance fault model and retention policy need a separate review; recursive delete is not acceptable.

### 9.4 Manifest, commit and per-file verification

Proposed manifest: fixed 64-byte header, 194 fixed 32-byte entries, 4-byte record checksum = **6,276 bytes**. Define endianness, schema/version, project identity, generation ID, count, slot/type, exact SFN, expected size and CRC32. Stream entries, use a 192-bit (24-byte) slot/type duplicate bitmap; validate the two metadata entries separately. Reject duplicate slot/type, duplicate filename, missing entries, invalid paths, separators/`..`, unsupported version, overflow, inconsistent identity or unexpected size. Manifest must describe actual written filenames, not wildcard resolution. CRC is error detection, not security authentication.

Proposed commit: fixed 64-byte `COMMIT.DAT` includes magic/schema/project/generation, manifest digest/length and its own checksum. Create via `FA_CREATE_NEW`, only after all payload files and manifest are closed, synchronized as required and read back. Reopen/read exact record after close; require healthy disk sync. Record names/schema still need formal review/tests; do not embed secrets or pretend a tick is a wall-clock time. No mutable “current generation” pointer is necessary for correctness. A cache, if later added, must be only a hint.

File digests must be computed over exactly the successful baseline byte stream, not in-memory structs with uninitialized padding. Current pattern format deliberately serializes a known `Step` layout; preserve it and assert target/host sizes. A successful CRC of mixed RAM epochs is **not** proof of a coherent project snapshot.

### 9.5 RAM consistency while the musician edits

Project payload is about 255 KB; the instrument has no demonstrated free region large enough for a second whole project. Existing save-time TIM2/sequencer/parameter activity can mutate RAM while serialization spans many files. Do not silently call that one consistent snapshot.

Prefer an explicitly reviewed persistent-content epoch: increment for every mutation relevant to saved kits, patterns, songs, names and settings, including recording/MIDI paths; exclude purely playback position. Capture epoch before serialization and check before commit. If it changed, keep the new generation uncommitted, preserve dirty state, and return a safe retry-needed result; never loop automatically until the musician stops editing. Existing `preset_touchTimer` is **not sufficient coverage** (RAM-copy/name/song paths and interrupt edits must be inventoried). Epoch overflow, read-modify-write ownership, interrupt interleaving and final dirty-clear must be tested; no global interrupt mask. This small additional mutation hook requires explicit implementation review because this phase does not authorize audio/MIDI changes.

If complete epoch coverage cannot be proven, specify a limited quiescent-save contract rather than pretending there is a coherent snapshot. Do not auto-stop the transport or disable edits. A later design can offer an explicit user-approved safe point or per-file version capture; that still needs cross-file semantics. No speculative full shadow allocation or reuse of live audio buffers.

### 9.6 Loading, legacy compatibility and temp recovery

Separate **discovery → validation → application → selection persistence**. Validation must be read-only: no mkdir, no clearing `/tmp`, no `/proj.idx` write, no early RAM identity publication. Validate project/kit/pattern indices 0–63, all lengths, exact supported version tags, names, counts and bounds before consumers. Fix the boot call to use the actual returned load result in a later approved patch.

Managed generations require the complete 194-file inventory and digests. Legacy projects need explicit profiles: current source intentionally tolerates missing assets and older pattern tails. Show/report an incomplete legacy load honestly and initialize missing slots deterministically rather than retain previous project content. Do not label a missing-nfo existing folder as an empty new project automatically. Unavailable media/FR_DISK_ERR is not FR_NO_FILE. Resolve duplicate wildcard matches as ambiguity, not arbitrary first-file selection.

Legacy patterns include untagged/V1 and earlier tagged layouts without all V3 timing data; optional length tail historically decodes zero as16. Kits historically zero-fill a short parameter tail. Preserve **documented supported** old lengths/defaults, not arbitrary truncation under the label “legacy.” Exact accepted profiles require fixtures. Songs need a verified legacy END/default rule; absent payload cannot inherit another song. Review all global/index/temp reads, ignored seeks/closes and associated kit bounds. `preset_init` must get reviewed complete defaults before it can safely serve as missing-slot initialization.

A two-pass validator with bounded per-file scratch (one pattern ~3.7 KB if needed) can avoid publishing partially decoded files. It cannot guarantee the card will not fail on the second pass. Whole-project RAM rollback is not available merely because prevalidation passed. For application during active playback, defer to an explicit safe handover policy; do not silently stop music. If apply fails after some slots changed, report **partial RAM load**, do not update loaded identity or automatically autosave mixed state, preserve remaining RAM, and require explicit verified reload/initialization. Guaranteeing exact old-RAM preservation would require a separately reviewed snapshot/journal architecture and memory budget.

Global `/tmp` files must not be deleted before new-project validation/selection. Bind future recovery to project and content generation using ancillary metadata, without changing existing sound/pattern payloads. On ambiguous legacy temp ownership, offer later explicit selection rather than automatically overlaying another project. Preserve S4-L2's **zero autosave notifications**, normal delay, bounded safe retry and separate kit/pattern dirty state. Do not implement U1 feedback, Phase D export or a new autosave transaction in this architecture-only task.

## 10. Error severity, recovery and live-performance policy

| Class | Examples / evidence needed | Allowed behavior | Forbidden behavior |
|---|---|---|---|
| Recoverable logical preflight | Invalid name/index, missing parent before modification, exclusive-name collision, insufficient staging space before uncertain writes | Truthful concise manual error; preserve RAM; explicit retry after correction, safe autosave backoff only for already proven paths | Blanket reboot-only latch merely for harmless open failure; continuous SD polling |
| Incomplete but known-owned staging | New files exist, operation stopped for content-epoch change, no unresolved I/O/owner state | Old remains active; mark candidate uncommitted; retry as a distinct new transaction when explicitly requested | Reuse/truncate uncertain file, delete old generation, claim new save succeeded |
| Uncertain write/sync/close/owner | Short payload, failed close, ambiguous commit completion, sticky FIL error | Preserve dirty RAM/evidence, fail closed for unsafe storage work; manual music/edits continue | Force-close, clear FIL.err, replay write, reset/remount to unlock |
| Directory/filesystem integrity concern | Duplicate slots, invalid chain/manifest, inconsistent traversal, invalid layout | Reject candidate; read-only fallback only if reads trustworthy and fault state permits; recommend backup/inspection | Speculative FAT repair, delete anomalies, use invalid directory as empty |
| Physical SD communication fault | S1 latch, S2 command/transfer timeout/error | Preserve existing latches; no further physical disk I/O, controlled reboot/card investigation may be necessary | Automatic card/CPU reset, automatic retry, assertion of quiescence |

S1's conservative latch also prevents reads, not only writes; do not promise same-session recovery fallback can bypass it. A write-protected card or missing card before any I/O may be logical/check-required rather than proof of corruption; classify using actual path and ownership, not an FRESULT code alone. A serious failure can require reboot or card replacement **for storage**, not to dismiss a notification or keep RAM sound editing operational.

No storage error should deliberately stop transport/audio/MIDI/RAM editing. Existing manual error notification is non-blocking; do not regress it or add autosave notifications. Existing synchronous file calls still occupy foreground: waits can delay UI/UART parsing/USB polling and parameter service even while audio IRQs run. Full-project save enables TIM2 rendering during I/O; this is not proof of deadline compliance, zero MIDI loss or glitch-free audio. There is also an unbounded display-buffer wait independent of SD timeouts. No new ISR work, priority/clock/DMA change, recursive audio service or interrupt masking is proposed.

A bounded owner/state machine can yield between file/chunk operations, but the current synchronous FatFs/HAL call remains a non-preemptible **foreground task segment**, not a measured real-time bound. Serialization, directory scans, f_getfree, 194 closes/syncs and readback can be expensive. Keep one foreground filesystem owner; do not let autosave/manual/remount interleave shared CWD/FIL. If fairness requires a new scheduler or asynchronous driver, stop that part for separate review rather than promise low latency from generation naming alone.

## 11. Resource and latency budget

### Existing successful project bytes (HOST + CODE)

Target/host layout: `sizeof(Step)=7`, `sizeof(StepTiming)=1`, `sizeof(PatternSet)=233536`; 64 slots; 7 tracks; 64 steps; 4 bars. Current successful output:

| Payload | Files | Bytes each | Total |
|---|---:|---:|---:|
| Project nfo | 1 | 8 | 8 |
| Globals | 1 | 30 | 30 |
| Named kit | 64 | 8+247=255 | 16320 |
| PatV03 | 64 | 14 + 7×64×8 + 7×4×2 + 1 + 7 + 1 =3663 | 234432 |
| Song | 64 | 8+64=72 | 4608 |
| **Complete project** | **194** | | **255398** |

The named-pattern formula includes the six-byte tag, step timing, main-step words, shuffle, seven lengths and associated kit byte. Autosave formats use their existing slot prefix instead of named header. Do not infer that every historical incomplete project must have current exact sizes.

Proposed generation space, **estimates**, before parent growth/FAT overhead/reserve: sum each file rounded to cluster size, plus a directory with 198 entries (`.`/`..`,194 payload,manifest,commit), manifest6276 and commit64. Single-SFN/no LFN is assumed.

| Cluster bytes | Rounded payload | Generation directory | Manifest | Commit | Approx. new generation allocation |
|---:|---:|---:|---:|---:|---:|
| 512 | 328704 | 6656 | 6656 | 512 | **342528** |
| 4096 | 794624 | 8192 | 8192 | 4096 | **815104** |
| 32768 | 6356992 | 32768 | 32768 | 32768 | **6455296** |

Save needs this extra space **without freeing the old copy first**; retention multiplies it. Reserve parent entries/cluster growth and allocation safety margin. A preflight free-space estimate is not a reservation or protection from later I/O failure, and f_getfree can scan FAT synchronously. Filename/candidate exhaustion is a truthful refusal, not grounds to delete the only good version.

Design workspace target: ~1–2 KB fixed sector/path/manifest buffers plus 24-byte inventory bitmap; optionally one ~3.7 KB pattern decode buffer. This is **not compiled or measured S4b usage**. No heap or large stack project copy; use bounded static foreground workspace, explicit ownership and stack measurements. Flash cost is unmeasured until implementation; budget a few KB only as an early estimate, not acceptance evidence. Add **zero ITCM** by design unless separately justified; limited headroom makes moving validation/CRC/UI into ITCM unacceptable. Full project double buffering would exceed currently demonstrated free general SRAM and DTCM combined.

New-stage writes plus full readback at least double payload traffic, and metadata close/sync counts may dominate. No sustained SD speed or CPU overhead was measured. Proposed future tests must record worst/average foreground call durations and audio block deadlines using optional diagnostics, with no printf/file I/O in audio IRQs and no reset of shared DWT CYCCNT.

## 12. Executed baseline verification and host investigations

### Environment and exact baseline commands

Existing checkout, bare-metal target cross builds; Linux host GCC for mocks. Per shell:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
cmake --version
ninja --version
python3 --version
arm-none-eabi-gcc --version
```

Observed: CMake3.31.10; Ninja1.13.0.git.kitware.jobserver-pipe-1; Python3.12.14; ARM GCC14.2.1 20241119 (15:14.2.rel1-1); native GCC14.2.0. Diagnostics remain **OFF by default**. No source changes preceded or followed the baseline checks.

All ten established commands returned exit0:

```sh
python3 tests/bug001/run.py
python3 tests/euclidean/run.py
python3 tests/safety/run.py
python3 tests/usb_allocator/run.py
python3 tests/sd_storage/run.py
python3 tests/sd_hal/run.py
python3 tests/fatfs_integrity/run.py
python3 tests/save_errors/run.py
python3 tests/live_storage/run.py
python3 tests/silent_autosave/run.py
```

These include their existing sanitizer and static-boundary checks; complete relevant output is appended below. They are passing regression tests, **not proof of project transaction safety**. New negative tests deliberately find uncovered defects. No existing suite was weakened, altered or skipped.

All four configuration/build pairs and image validator returned exit0:

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

Build output prints installation instructions as an existing post-build message; **none were followed**. Generated artifacts remained local/ignored and are not included in this report commit. App image header: 16 bytes, `LXRV2IMG`, LE32 payload length, one-byte checksum `0xFF-(sum(payload)&0xFF)`, three zero bytes. All payloads matched exact `.bin`; MSP `0x20080000`, reset `0x08014E4D` (Thumb); all load sections within application flash beginning `0x08008000`. First32KiB excluded; `UPDATE_BOOTLOADER=0`. No startup/linker/clock/bootloader source changed.

| Build | .bin bytes | .img bytes | Checksum | Linker-reported Flash | All Flash LMA span / remaining |
|---|---:|---:|---|---:|---:|
| Release OFF | 278604 | 278620 | 78 | 226592 | 278604 /737204 |
| Debug OFF | 278652 | 278668 | A4 | 226640 | 278652 /737156 |
| Release ON | 279044 | 279060 | 16 | 226960 | 279044 /736764 |
| Debug ON | 279092 | 279108 | B9 | 227008 | 279092 /736716 |

Important accounting distinction: the existing validator's Flash VMA high-water (225740/225788/226108/226156) excludes RAM/TCM initialization load images; the linker memory report also does not give the full physical span of explicit `AT(...)` TCM load data. An additional read-only objdump check validated **every LOAD section's LMA** and confirmed maximum end minus app origin equals actual `.bin` length above. Use that larger span for physical Flash headroom (capacity1,015,808), not only VMA or linker report. This is an accounting clarification, not a linker change.

| Static region | Capacity | OFF used / remaining | ON used / remaining | S4b production delta |
|---|---:|---:|---:|---:|
| ITCM | 16384 | 15016 /1368 | 15088 /1296 | **0** |
| DTCM | 131072 | 107392 /23680 | 107392 /23680 | **0** |
| General RAM | 393216 | 271600 /121616 | 271672 /121544 | **0** |

These are static allocations/high-water, not runtime free heap or stack measurements. `_Min_Heap_Size=512`, `_Min_Stack_Size=1024`; startup/Phase B stack usage is not proven by these reservations. Build resources match unchanged S4-L2; proposed S4b workspace is not present. Artifact hashes and full image validation records appear in the appendices.

### New temporary tests actually executed

All fixtures are in `/tmp/phase-s4b`, not repository files. ASan/UBSan builds use `-fsanitize=address,undefined -fno-sanitize-recover=all`, non-PIE for stable host sanitizer startup; LeakSanitizer disabled in this constrained executor. No reported leak result is claimed. Each destructive-function fixture uses fake directory APIs and never touches disk media. Test extraction preserves production function bodies and reports original line numbers with `#line`; renamed functions/stubs are disclosed in appended scripts.

| Group | Execution | Actual result and limit |
|---|---|---|
| Destructive cleanup / indices | `PYTHONDONTWRITEBYTECODE=1 python3 /tmp/phase-s4b/run_erasure.py` | PASS harness expectations: ASan catches metadata and ordinary-SFN global overflow; UBSan catches project index255; fitting-path control passes; simulated deletion order and stop-on-error pass; temp guard still attempts unlink after overflow. **Expected defect subprocesses exit1**, not clean production behavior. |
| Project/asset loading | `PYTHONDONTWRITEBYTECODE=1 python3 /tmp/phase-s4b/run_load.py` | PASS expected observations: FR_OK stale RAM with all192 children absent for normal/zero nfo; short song FR_OK; scan error→NO_FILE; unchecked project/temp kit255; associatedkit255; last8 init bytes stale. Separate actual kit consumer exits1 with expected UBSan. Timers/display/child loads/volume APIs are mocked. |
| Directory/FAT mechanism | Compile `filesystem_host.c` as appended; run `ASAN_OPTIONS=detect_leaks=0 /tmp/phase-s4b/filesystem_host` | PASS whole production FatFs/disk units on synthetic RAM FAT16: early SFN read works, complete enumeration fails at injected later sector; primary-FAT behavior; malformed LFN ignored; zero-end-marker counterexample. No real iOS/device filesystem test. |
| Proposal model | `PYTHONDONTWRITEBYTECODE=1 python3 /tmp/phase-s4b/model.py` | PASS abstract 200 crash cuts ×10 read-only restarts; invalid commit/manifest/payload falls back. Explicit shared-corruption counterexample loses both. **Not production code, not sector-level FAT or power-cut validation.** |
| Original comparison | Extracted normalized bodies from lab-setup/current | Destructive helper/init/naming behavior predates safety lineage; full comparison hashes appended. No official binary comparison. |

### Development errors and limitations

The first loader harness compilation lacked local FEXT constants in the fixture; corrected only the `/tmp` stub setup. An extension initially referenced `Core/Src/Preset/InitKit.c`; actual filename is lowercase `initKit.c`; corrected only the test command. Final host runs passed their stated expectations. Read-only searches also initially guessed incorrect soundengine/linker filename casing; actual linker is `STM32F765VGTX_FLASH.ld`. No firmware change resulted.

A historical Git log/diff traversal encountered missing local old tree `76f4ac9cfd7fc7016d9617e33070fd619fe60776` in this partially provisioned history. Current source/lab-setup files, exact remote heads and required ancestry were accessible; comparison used those available objects/connector content rather than claiming complete original author history. No history reset or unrelated fetch/repair was performed.

ASan expected failures are new confirmed defects, so this baseline must **not** be described as ready for destructive project-save hardware tests despite passing established suites. None of the tests measures SD electrical timing, real card programming latency, audio continuity, CPU utilization, thermal behavior or actual PROJ00 contents.

## 13. Proposed fault-injection and regression plan — NOT EXECUTED as S4b implementation tests

### Exact acceptance invariants

* Under the isolated logical-failure model, byte-for-byte old committed content and reachability remain unchanged until and after a new generation commits; transaction never intentionally targets its chains/names. After reboot, choose only a fully validated generation.
* No FR_OK/save confirmation after any required failed/short write, failed close/sync/readback/commit verification; no loaded identity update after partial/invalid load.
* No physical write after an S1 fault; no forced shared-file reuse, hidden retry, reset/remount, repair, format, cleanup or latch bypass.
* An uncommitted/invalid candidate is never mistaken for current; recovery is read-only and idempotent after every interruption, including interrupted recovery itself.
* No unrelated project/unknown file deletion; no path/slot/length overflow; no dynamic allocation or SD work in an audio ISR.
* Dirty RAM remains pending if content version changes or save fails; autosave stays silent, independently bounded and nonintrusive.
* Explicitly test the counterexample: corruption of shared FAT/root/media sectors may defeat same-card preservation. Tests must document this residual risk, not weaken or falsely satisfy the invariant by excluding all difficult failures without saying so.

### Ordered test matrix

| Area | Fault/case injection | Required checks |
|---|---|---|
| Path/memory containment | Every index0..255; all8.3 lengths; nesting/depth; bad components; overlong path; unknown entries | ASan/UBSan no OOB; failed builder performs zero operations; no recursion in normal save; slot63 valid,64+ rejected |
| Preflight | No card, wrong directory attributes, bad namespace ownership, missing path, name collision, free space insufficient | Old unchanged, no cleanup, correct retry classification; initialization cannot bypass S1 |
| Staging setup | Failure before first write, mkdir/open/entry allocation failures, empty stage | Old resolves; exclusive names; no truncation of existing targets |
| Every payload | Inject error/short write at **every** write call/byte-boundary class for nfo/globals/64 kits/64 patterns/64 songs, including success count with error | First error retained, no later payload writes, appropriate ownership hold; existing successful bytes exactly equal S4-L2 oracle |
| Close/sync | Data/FAT mirror/FSInfo/CTRL_SYNC failure at each required close, failure after data accepted | No false success, no implicit replay/force-close; sticky S3 behavior and S1 latch verified |
| Validation | Missing/duplicate files/slots, extra unknown files, wrong lengths/digest/tag/manifest identity, invalid associated kit, corrupt length | Reject before selecting/publishing; legacy policy distinct from managed strictness |
| Commit | Partial/short/failed write, close error, readback mismatch, record persists despite error return, directory-entry tear | New accepted only after complete validation; ambiguous original operation never reported success/retried; next boot may select independently validated completed record |
| Rename candidate comparison | Power cut after destination registration, parent update, old entry removal, sync | Demonstrate aliases/cross-links and non-atomicity on RAM images; no rename reliance in preferred design |
| Recovery | Reboot after each API and each simulated sector write, repeated interrupted read-only recovery, multiple valid/invalid generations, ID exhaustion | Deterministic bounded selection, no deletes/writes/replay; newest valid or explicit ambiguity/error |
| Cleanup/retention (later explicit feature) | Failure during each owned old-generation deletion, unknown files, cross-links, no free directory entries | Never delete sole verified good; current+previous retained; no automatic boot/save cleanup |
| Legacy load | Complete current; actual supported older kits/V1/V2/V3 patterns; missing nfo, missing assets, truncated song/settings, duplicate wildcard files | Deterministic defaults/warnings for intentional omissions; malformed content rejected; no stale RAM inherited or fabricated identity |
| Apply interleaving | Read error on second pass; edits/recording/MIDI while saving; content epoch rollover/interruption; partial RAM apply | No false coherent snapshot, no clearing newer edits; partial apply explicitly contained; no unapproved stop/audio mutation |
| Temp recovery | Old/new project identities, unmatched `/tmp`, failures before/after selection, interrupted cleanup | Old recovery never deleted during validation; no cross-project silent overlay; autosave remains silent |
| Read-only/physical faults | Permanent/temporary busy, command errors, removal, S1 latched fault, historical S2 errors | No unsafe subsequent transfer/reset; no recovery claim based only on READY |
| Media model | FAT12/16/32 supported fixtures, cluster sizes512..32768, full root/subdir, torn/reordered sectors, divergent FATs/FSInfo | Document where logical preservation holds and where shared-sector corruption defeats it |

Implementation tests should commit reproducible harnesses, not depend on `/tmp` surviving. Use whole production FatFs over a RAM block device with write-event recording and cut/replay at every sector boundary, plus complete production transaction functions with mocked timers/UI. Exercise recovered images in at least two independent **read-only** parsers. Mocks should support disk returning an error after applying a write, not just failure-before-write. Existing S1–S4-L2 suites and all four firmware/image configurations must remain passing; compare successful bytes and unchanged audio/interrupt objects. No production test should erase a real card.

### Separate PROJ00-inspired synthetic plan

Construct clone-only/RAM images with: bad later directory cluster; primary/secondary FAT disagreement; loop/cross-link/out-of-range chain; hidden/system SFN; malformed LFN/checksum/duplicate short names; incorrect size/cluster entry; early0x00/deleted markers; targeted-read success with full-enumeration error; repeatable localized read failure; truncated nfo/song and persistent tmp overlay. Check named reads, wildcard and full enumeration separately. Model host UI filtering as a hypothesis, not an iPhone emulator. Record both interpretations without “repairing” input. Compare actual acquired PROJ00 only if read-only image becomes available with separate authorization; synthetic matches alone do not establish cause.

## 14. Ordered implementation plan and review checkpoints

1. **Containment review:** independently authorize bounds/path fixes, complete defaults, error-preserving name lookup and checked nfo. Remove destructive cleanup from replacement path; temporarily allow only new-unused-slot save if necessary. Tests must prove no target deletion and no invalid-index consumer. Do not combine with firmware deployment.
2. **Compatibility review:** decide immutable namespace vs transparent legacy replacement. Approve ancillary schema, downgrade/external-edit conflict policy, retention/candidate limits and exact conditional preservation guarantee. If same-path manufacturer transparency is mandatory, revisit the design before code.
3. **Ownership/read-only validation:** introduce one explicit foreground storage owner without locks in IRQs; bounded absolute paths; no CWD-dependent transaction state; no remount with unresolved FIL. Separate create/discover/load/selection persistence. Commit strict managed and explicit legacy validators/tests first.
4. **Content coherence:** inventory every persistent mutation source and prove epoch/version ownership; quantify hook cost and same-tick edit behavior. If this cannot be done safely, approve a quiescent workflow instead of claiming snapshot consistency.
5. **Immutable write/commit engine:** reuse successful serializers; exclusive files, checked writes/close/sync/readback; manifest/commit-last; no delete/rename/retry; distinct outcomes for logical retry, incomplete stage and uncertain commit. Preserve dirty RAM.
6. **Read-only recovery/load:** bounded candidate selection, full semantic validation, safe legacy fallback, explicit partial-RAM-load handling; project-bound temp recovery; correct boot/menu result use. No silent creation/cleanup/auto-repair.
7. **Fault matrix review:** sector-cut simulations, all legacy profiles, mount/owner failures, unchanged S1–L2 tests, four builds/resources/object comparison. Review the unavoidable FAT/shared-media counterexamples rather than declaring all power-loss cases solved.
8. **Maintenance/export separate review:** only if needed, design explicit safe generation retirement and export-to-new-slot; no automatic garbage collection in the initial correction.
9. **Hardware validation proposal only:** after code review and separate Daniel authorization with backups/recovery plan, use disposable cloned cards and known firmware identity to measure maximum SD/foreground latency, audio DMA/render diagnostics and behavior under controlled failures. Never start with the original PROJ00 card. No such testing is authorized/performed by this report.

## 15. Unresolved questions and readiness

* What binary is actually installed on the affected instrument, and does it contain these source paths? What is PROJ00's FAT type/cluster geometry and raw directory chain? No attribution possible yet.
* Which historical incomplete/legacy project layouts are intentionally supported in real user libraries? Need authentic fixtures before tightening validators.
* Does latest-generation transparency to unmodified official firmware outweigh immutable-generation safety? This must be an explicit product/workflow decision.
* What bounded retention/discovery limit and maintenance workflow is acceptable given card cluster sizes and finite free space?
* Can persistent-content epoch coverage be proven without timing-sensitive changes? Can project application while playing be safely deferred without losing the current RAM session?
* What happens to a live shared file during card removal/remount? S1 protects serious faults, but volume/application ownership needs deliberate integration rather than zeroing objects.
* Hardware-specific peripheral/card quiescence, physical power-loss durability, maximum audio/foreground latency, stack high-water and media-write ordering remain OPEN.

**Readiness:** ready for architecture review and separately authorized containment/implementation planning. **Not ready for destructive project overwrite validation or firmware installation.** Existing new source/host defects are significant despite the passing baseline suites. The proposed architecture improves logical preservation and honest recovery but cannot certify a damaged FAT or physical SD card. No source correction was made, and no actual PROJ00 diagnosis or recovery was demonstrated.

## 16. Publication and verification contract

The report-only branch is intended as `codex-reports/2026-10-27-phase-s4b-project-storage-design`, directly based on lab-setup `5829398cea5d337200393757e39a37d4d8148256`, with only this new Markdown file. It does not contain the S4-L2 implementation tree; source references above must be followed to the exact baseline commit. The report commit identifier is supplied by its GitHub commit metadata and the final handoff (a file cannot truthfully embed its own future commit hash before creation).

Publication is claimed only after reading back the exact GitHub file, checking commit parent/file list, and confirming the source checkout and all pre-existing remote heads are unchanged. No firmware artifacts or temporary executable files are published. The appendices preserve investigation source/logs as **text within this report**, not production changes. If publication verification fails, the final handoff must state that failure and provide the complete report instead.

## Appendices

The following appendices provide exact baseline outputs, decisive excerpts and reproducible temporary fixtures. Everything described as a proposal/model remains unimplemented. Original card/hardware access was never performed.

## Appendix A — Decisive source excerpts

All excerpts are from source commit 910be804; CRLF is normalized for presentation. They are evidence, not proposed replacement code.

### Core/Src/Preset/presetManager.c:39–46

```c
//FATFS SDFatFS;		/* File system object for the logical drive */
//FIL SDFile;		/* place to hold 1 file*/
static uint8_t morph_state = 0; //flag for non blocking morph
static uint8_t morph_index = 0;
char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];

char currentFolder[13];
```

### Core/Src/Preset/presetManager.c:719–750

```c
FRESULT empty_directory (
    char* path      /* Working buffer filled with start directory */
)
{
	FILINFO fno;
    UINT i, j;
    FRESULT fr;
    DIR dir;

    fr = f_opendir(&dir, path);
    if (fr == FR_OK) {
        for (i = 0; path[i]; i++) ;
        path[i++] = '/';
        for (;;) {
            fr = f_readdir(&dir, &fno);
            if (fr != FR_OK || !fno.fname[0]) break;
            if (_FS_RPATH && fno.fname[0] == '.') continue;
            j = 0;
            do
                path[i+j] = fno.fname[j];
            while (fno.fname[j++]);
            if (fno.fattrib & AM_DIR) {
                fr = empty_directory(path);
                if (fr != FR_OK) break;
            }
            fr = f_unlink(path);
            if (fr != FR_OK) break;
        }
        path[--i] = '\0';
        f_closedir(&dir);
    }
```

### Core/Src/Preset/presetManager.c:595–620

```c

    for (i = 0; path[i]; i++) ; /* Get current path length */
    path[i++] = _T('/');

    for (;;) {
        fr = f_readdir(&dir, fno);  /* Get a directory item */
        if (fr != FR_OK || !fno->fname[0]) break;   /* End of directory? */
        j = 0;
        do {    /* Make a path name */
            if (i + j >= sz_buff) { /* Buffer over flow? */
                fr = 100; break;    /* Fails with 100 when buffer overflow */
            }
            path[i + j] = fno->fname[j];
        } while (fno->fname[j++]);
        if (fno->fattrib & AM_DIR) {    /* Item is a sub-directory */
            fr = delete_node(path, sz_buff, fno);
        } else {                        /* Item is a file */
            fr = f_unlink(path);
        }
        if (fr != FR_OK) break;
    }

    path[--i] = 0;  /* Restore the path name */
    f_closedir(&dir);

    if (fr == FR_OK) fr = f_unlink(path);  /* Delete the empty sub-directory */
```

### Core/Src/Preset/presetManager.c:2376–2404

```c
	unsigned int bytesWritten;
	//write preset name to file
	res = f_write((FIL*)&SDFile,(void*)savefileNames[presetNr][PROJECT_NAMES],8,&bytesWritten);
	if(res != FR_OK) {
		lcd_process_display_interrupt = 1;
		preset_initTmpFolder();
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
#else

#endif

		return res;
	}
	//close the file
	preset_closeShared((FIL*)&SDFile);

//	soundengine_process();
	//save globals
	res = preset_saveGlobals();
    if(res != FR_OK) {
        lcd_process_display_interrupt = 1;
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
        HAL_TIM_Base_Stop_IT(&htim2);
#endif
        return res; /* No later project writes or temporary-file deletion. */
    }
	//save all drumkits, patterns and songs
	for(uint8_t i=0;i<NUM_KIT_SLOTS;i++)
```

### Core/Src/Preset/presetManager.c:2770–2791

```c
	//close the file
	preset_closeShared((FIL*)&SDFile);

	//load globals
	preset_loadGlobals();
	//load all drumkits, patterns and songs
	for(uint8_t i=0;i<NUM_KIT_SLOTS;i++)
	{

		sprintf(text, "%d%%", (uint8_t)(i/(float)NUM_KIT_SLOTS * 100.f));
		lcd_setcursor(0,2);
		lcd_string(text);

		//FR_NO_FILE just means an incomplete project and is no reason to abort!
		res = preset_loadDrumsetSD(i, 0);
		if ((res != FR_OK) && (res != FR_NO_FILE))
		{
			lcd_process_display_interrupt = 1;
			preset_initTmpFolder();
			preset_closeShared((FIL*)&SDFile);
#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
```

### Core/Src/Preset/presetManager.c:2560–2577

```c
	res = f_read((FIL*)&SDFile,(void*)savefileNames[songNr][SONG_NAMES],8,&bytesRead);

	if((!bytesRead) || (res != FR_OK))
		goto closeFile;

	if(preset_isNameEmpty(savefileNames[songNr][SONG_NAMES]))
	{
		sprintf((void*)savefileNames[songNr][SONG_NAMES], "Empty");
	}

	//read song data
	res = f_read((FIL*)&SDFile, seq_songData[songNr], MAX_SONG_LENGTH, &bytesRead);

	closeFile:
		//close the file handle
		preset_closeShared((FIL*)&SDFile);
		return res;
}
```

### Core/Src/Preset/presetManager.c:383–389

```c
	{
		if(f_read((FIL*)&SDFile, &kit, sizeof(kit), &bytesRead) != FR_OK || bytesRead != sizeof(kit)) return 0;
	}
	for(uint8_t trk=0; trk<NUM_TRACKS; trk++)
		seq_patternSet.length[patternNr][trk] = lengths[trk];
	if(dataVersion > 1) seq_patternSet.kitNr[patternNr] = kit;
	return 1;
```

### Core/Src/Preset/presetManager.c:791–803

```c
	int16_t i;
	uint8_t *para;


	para=mainParameters[presetNr];

	for(int i=8;i<END_OF_SOUND_PARAMETERS; i++)
	{
		para[i-8] = initKit[i];
	}

	//special case mod targets. normalize.
	const uint8_t nmt=getNumModTargets();
```

### Core/Src/main.c:824–837

```c
#if LOAD_PROJECT_ON_BOOT
	uint8_t lastUsedProject;
	preset_loadLastUsedProjectNrFromSD(&lastUsedProject);
	//do not clean the tmp folder here, as we are just booting
	preset_loadProject(lastUsedProject, 0);
	if(res != FR_OK)
	{
		menu_showSdError(res);
	}
	else
	{
		menu_loadedPresetNr[CURRENT_PROJECT] = menu_selectedPresetNr[SAVE_TYPE_PROJECT] = lastUsedProject;
		//menu_resetSaveParameters();
	}
```

### Middlewares/Third_Party/FatFs/src/ff.c:4780–4815

```c
					res = (djn.obj.sclust == djo.obj.sclust && djn.dptr == djo.dptr) ? FR_NO_FILE : FR_EXIST;
				}
				if (res == FR_NO_FILE) { 				/* It is a valid path and no name collision */
					res = dir_register(&djn);			/* Register the new entry */
					if (res == FR_OK) {
						dir = djn.dir;					/* Copy information about object except name */
						mem_cpy(dir + 13, buf + 2, 19);
						dir[DIR_Attr] = buf[0] | AM_ARC;
						fs->wflag = 1;
						if ((dir[DIR_Attr] & AM_DIR) && djo.obj.sclust != djn.obj.sclust) {	/* Update .. entry in the sub-directory if needed */
							dw = clust2sect(fs, ld_clust(fs, dir));
							if (!dw) {
								res = FR_INT_ERR;
							} else {
/* Start of critical section where an interruption can cause a cross-link */
								res = move_window(fs, dw);
								dir = fs->win + SZDIRE * 1;	/* Ptr to .. entry */
								if (res == FR_OK && dir[1] == '.') {
									st_clust(fs, dir, djn.obj.sclust);
									fs->wflag = 1;
								}
							}
						}
					}
				}
			}
			if (res == FR_OK) {
				res = dir_remove(&djo);		/* Remove old entry */
				if (res == FR_OK) {
					res = sync_fs(fs);
				}
			}
/* End of the critical section */
		}
		FREE_NAMBUF();
	}
```

## Appendix B — Actual baseline regression output

Each command completed with exit0; these are existing committed suites on the unchanged source. Sanitizer error-exit negative tests later in Appendix D are separate.

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

## Appendix C — Artifact verification and original comparison

### Complete image validator output

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0x78", "configuration": "release", "elf_sha256": "369d90d42420b351fe9f363137f80870b56eb7ae2adf360a327817fea6066b2a", "image_bytes": 278620, "image_sha256": "a9b8f516b0df0679b93ee87199838a55864aa6f22eed68272a5601654f010d5a", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 225740, "ITCM_RAM": 15016, "RAM": 271600}, "payload_bytes": 278604, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xA4", "configuration": "debug", "elf_sha256": "9f4483da680195d450d6311ce7927fa6af67ea0b0077e6da2090c05174612d4e", "image_bytes": 278668, "image_sha256": "7afe3cf0777638f7ba8d0e1d8fb76ba8e4768920d67e1c262092d711a02c778c", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 225788, "ITCM_RAM": 15016, "RAM": 271600}, "payload_bytes": 278652, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x16", "configuration": "diagnostics-release", "elf_sha256": "a86e8c1cb97a0d0dbaa4bce5bf026e803142e122aa738b756ed814d7004fc874", "image_bytes": 279060, "image_sha256": "47e6695d50ff8fc37fb96ef3bd067fad7a10054452652bf9620da25e8325091b", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 226108, "ITCM_RAM": 15088, "RAM": 271672}, "payload_bytes": 279044, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xB9", "configuration": "diagnostics-debug", "elf_sha256": "6d57c773d04c01ca0db45e42808a3c35b5776c609faec2c94632ce13991b0f79", "image_bytes": 279108, "image_sha256": "0ded5c50957ca111c5609c4e6d3c72e1c2b732aa4d6bba9bf456a39d1a9a7221", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 226156, "ITCM_RAM": 15088, "RAM": 271672}, "payload_bytes": 279092, "reset_vector": "0x08014E4D", "result": "PASS"}

EXIT 0
```

### Additional all-LOAD-LMA validation and binary SHA-256

```json
[
  {
    "configuration": "release",
    "all_flash_load_span": 278604,
    "flash_capacity": 1015808,
    "flash_remaining": 737204,
    "bin_sha256": "96c1421e7364044bb2f8497e3ae8138d1f856c10d40e3e169d1629d19d19d8d3"
  },
  {
    "configuration": "debug",
    "all_flash_load_span": 278652,
    "flash_capacity": 1015808,
    "flash_remaining": 737156,
    "bin_sha256": "48862832cad72c3975942164798e8ba91298f8867a7d6453c966c0a2dccbeb7a"
  },
  {
    "configuration": "diagnostics-release",
    "all_flash_load_span": 279044,
    "flash_capacity": 1015808,
    "flash_remaining": 736764,
    "bin_sha256": "9d4515a108d84ea2bc007cc44f0ed24e5352c26a1a88418c4f1d43a4dfaa5d60"
  },
  {
    "configuration": "diagnostics-debug",
    "all_flash_load_span": 279092,
    "flash_capacity": 1015808,
    "flash_remaining": 736716,
    "bin_sha256": "794acf876077cd9372c92556bf6a1f8720297f4f0900834a0c21692746319a26"
  }
]
```

### Original lab-setup / baseline extracted-body comparison

```json
[
  {
    "function": "empty_directory",
    "same_function_text_excluding_line_directive": true,
    "original_sha256": "54e38dfa5e5374aefb5f86a2aa6dc292af4874f17a1fbe19f4491145237d8f15",
    "baseline_sha256": "54e38dfa5e5374aefb5f86a2aa6dc292af4874f17a1fbe19f4491145237d8f15"
  },
  {
    "function": "delete_node",
    "same_function_text_excluding_line_directive": true,
    "original_sha256": "f86e60de63466825b7838e29c8e8202faf2ece0655d6473f6d9fbf5a0e8bf3a7",
    "baseline_sha256": "f86e60de63466825b7838e29c8e8202faf2ece0655d6473f6d9fbf5a0e8bf3a7"
  },
  {
    "function": "preset_createProject",
    "same_function_text_excluding_line_directive": false,
    "original_sha256": "2286e9c629a66071fc9888b11119f3f012f4474fba54bc4817fc03a075d7aabb",
    "baseline_sha256": "4afadf64be11955d12e3d63edcb6907a4c61ffc9201d9dd28b402a5a05c4ca2b"
  },
  {
    "function": "preset_init",
    "same_function_text_excluding_line_directive": true,
    "original_sha256": "e33f91341fabf796a4eaecb7b9e54eb0ea9e376e4513213d88e36b7ce20c4028",
    "baseline_sha256": "e33f91341fabf796a4eaecb7b9e54eb0ea9e376e4513213d88e36b7ce20c4028"
  },
  {
    "function": "preset_makeFileNameSave",
    "same_function_text_excluding_line_directive": true,
    "original_sha256": "839c5bd6b7032e07d14e22021b265754f64b7a89d834b47b285ef3456b2e6489",
    "baseline_sha256": "839c5bd6b7032e07d14e22021b265754f64b7a89d834b47b285ef3456b2e6489"
  },
  {
    "function": "preset_saveProject",
    "same_function_text_excluding_line_directive": false,
    "original_sha256": "229533974558585faf89a7e220ca1e0db7460ff2a22ce8066d29aa2bb04124de",
    "baseline_sha256": "7b6e2c25f9b2bfab0f4bc094ae15b0ce234da834d1a96e24768aaa5d8f5c4b4d"
  },
  {
    "function": "preset_loadProject",
    "same_function_text_excluding_line_directive": false,
    "original_sha256": "686012ce5b1874ec8c30583d4784edc4bc74eace3818d2164010deff00be3867",
    "baseline_sha256": "3e6065c11a0638227bfab3b0fdac24f0114c9e32ad7d21f12238789dd941bea5"
  },
  {
    "function": "preset_loadProjectNames",
    "same_function_text_excluding_line_directive": false,
    "original_sha256": "90a33cbde9e2a1703c5039b9c5306bc1db3c3cb02da418c44e8c4bfcad821c56",
    "baseline_sha256": "c6ccf0b0e08cc0e041908eb71fd369cc5c0bdd36e5132667b1558f0ff9339e63"
  },
  {
    "function": "preset_loadSongSD",
    "same_function_text_excluding_line_directive": false,
    "original_sha256": "1d5fcdaa824f3226bc8388d8d9a301b1f9796e53ba8bb265e451cd7afea7432d",
    "baseline_sha256": "7db1ce8837b97aeb51c92c0bc47ad21a95ac8d912175f24dee5542f5275a8f0b"
  },
  {
    "function": "preset_makeFileNameLoad",
    "same_function_text_excluding_line_directive": false,
    "original_sha256": "6619d08965574424a59b5d6c5b36a263c910bd2288944d4d9ccbdd119a7688f2",
    "baseline_sha256": "40e8db3b39e6f09c2f308d88fc263cac12e116153df23e573b34ef56aa7f341d"
  }
]
```

## Appendix D — Temporary investigation outputs

Expected ASan/UBSan exits below confirm unsafe source behavior; no device failure is claimed. ASan host addresses are unrelated to ARM ELF addresses. Reproduction fixture source follows.

### metadata-overflow.log

```text
$ erase_host overflow prj.nfo
=================================================================
==4530==ERROR: AddressSanitizer: global-buffer-overflow on address 0x0000004099cd at pc 0x000000401c36 bp 0x7ffee2ed0c80 sp 0x7ffee2ed0c78
WRITE of size 1 at 0x0000004099cd thread T0
    #0 0x401c35 in empty_directory Core/Src/Preset/presetManager.c:738
    #1 0x402692 in main /tmp/phase-s4b/erase_host.c:34
    #2 0x7f6aa1c25ca7  (/lib/x86_64-linux-gnu/libc.so.6+0x29ca7) (BuildId: 5e5b51f4c39af9ac589408c1dc432ba390a273cc)
    #3 0x7f6aa1c25d64 in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x29d64) (BuildId: 5e5b51f4c39af9ac589408c1dc432ba390a273cc)
    #4 0x401210 in _start (/tmp/phase-s4b/erase_host+0x401210) (BuildId: 5df533e14d1eed86b006983483910d51be2a4176)

0x0000004099cd is located 51 bytes before global variable 'microseconds_nonblock' defined in '/workspace/lxr-desktop-app/Core/Inc/delay.h:32:30' (0x409a00) of size 4
0x0000004099cd is located 0 bytes after global variable 'currentFolder' defined in '/tmp/phase-s4b/erase_host.c:5:6' (0x4099c0) of size 13
SUMMARY: AddressSanitizer: global-buffer-overflow Core/Src/Preset/presetManager.c:738 in empty_directory

[Shadow-byte legend/dump omitted; subprocess exited 1 as expected.]
```

### normal-sfn-overflow.log

```text
$ erase_host overflow 00-KICK1.SND
=================================================================
==4531==ERROR: AddressSanitizer: global-buffer-overflow on address 0x0000004099cd at pc 0x000000401c36 bp 0x7ffe1db841c0 sp 0x7ffe1db841b8
WRITE of size 1 at 0x0000004099cd thread T0
    #0 0x401c35 in empty_directory Core/Src/Preset/presetManager.c:738
    #1 0x402692 in main /tmp/phase-s4b/erase_host.c:34
    #2 0x7f2b41a79ca7  (/lib/x86_64-linux-gnu/libc.so.6+0x29ca7) (BuildId: 5e5b51f4c39af9ac589408c1dc432ba390a273cc)
    #3 0x7f2b41a79d64 in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x29d64) (BuildId: 5e5b51f4c39af9ac589408c1dc432ba390a273cc)
    #4 0x401210 in _start (/tmp/phase-s4b/erase_host+0x401210) (BuildId: 5df533e14d1eed86b006983483910d51be2a4176)

0x0000004099cd is located 51 bytes before global variable 'microseconds_nonblock' defined in '/workspace/lxr-desktop-app/Core/Inc/delay.h:32:30' (0x409a00) of size 4
0x0000004099cd is located 0 bytes after global variable 'currentFolder' defined in '/tmp/phase-s4b/erase_host.c:5:6' (0x4099c0) of size 13
SUMMARY: AddressSanitizer: global-buffer-overflow Core/Src/Preset/presetManager.c:738 in empty_directory

[Shadow-byte legend/dump omitted; subprocess exited 1 as expected.]
```

### small-control.log

```text
$ erase_host overflow a.x
mock unlink /proj00/a.x

EXIT 0
```

### delete-order.log

```text
$ erase_host order
mock unlink /proj00/prj.nfo
mock unlink /proj00/glo.cfg
mock unlink /proj00/00-KICK1.SND
mock unlink /proj00/README.TXT
mock unlink /proj00/SUBDIR/nested.bin
mock unlink /proj00/SUBDIR
PASS actual deletion body: enumeration/depth-first ordering and first failure stop, without physical files

EXIT 0
```

### delete-failure.log

```text
$ erase_host failure
mock unlink /proj00/prj.nfo
mock unlink /proj00/glo.cfg
PASS actual deletion body: enumeration/depth-first ordering and first failure stop, without physical files

EXIT 0
```

### guard.log

```text
$ erase_host guard
PASS guard overflow sentinel 100 is overwritten by attempted unlink result; no real deletion performed

EXIT 0
```

### project-index.log

```text
$ erase_host index
Core/Src/Preset/presetManager.c:2606:31: runtime error: index 255 out of bounds for type 'char [64][4][8]'

EXIT 1
```

### load-results.log

```text
PASS actual project loader returns FR_OK with 8-byte metadata and all 192 assets absent; old RAM retained; index-save side effect reached (mocked)
PASS actual project loader returns FR_OK with zero-byte metadata and all 192 assets absent; old RAM retained; index-save side effect reached (mocked)
PASS actual song loader returns FR_OK for zero-byte payload after 8-byte name; previous song data remains
PASS actual filename lookup collapses FR_DISK_ERR from directory search into FR_NO_FILE; real S1 may block a later fallback
PASS actual root project-index and temporary-kit loaders publish unchecked slot255
PASS actual pattern-tail loader accepts associated kit255 despite NUM_KIT_SLOTS=64
PASS actual init leaves last8 of247 kit bytes stale; available InitKit payload246 bytes needs explicit defaults, not a blind loop extension
LAYOUT Step=7 StepTiming=1 PatternSet=233536 kitData=247 globals=30 patternV3=3663 song=72 projectBytes=255398

EXIT 0
```

### kit-index-consumer.log

```text
Core/Src/Preset/presetManager.c:870:42: runtime error: index 255 out of bounds for type 'uint8_t [64][247]'

EXIT 1
```

### filesystem-results.log

```text
PASS actual FatFs: hidden/system SFN file read and early wildcard search succeed while full enumeration hits a mocked unreadable later directory cluster; no media writes
PASS actual get_fat reads first FAT despite conflicting second FAT; this does not identify iOS FAT policy or real card state
PASS actual SFN-only directory path ignores malformed LFN entry and still finds SFN
PASS zero end-marker before data makes both enumeration and named lookup miss it: simple emptiness alone cannot explain successful same-path reads
```

### model-results.log

```text
PASS abstract model: 200 crash cuts x 10 read-only recoveries; old generation untouched
PASS abstract invalid/partial record or payload rejects new generation; previous selected
LIMIT demonstrated: shared filesystem/media corruption can destroy both generations; model gives no unconditional FAT/SD durability guarantee
```

## Appendix E — Reproducing the temporary tests without firmware changes

Use a disposable **software workspace** at the exact baseline commit, or the existing clean baseline checkout. Recreate the following text files under `/tmp/phase-s4b`; these are host tests, not firmware implementation. Existing repository headers and `tests/fatfs_integrity/host.c` are dependencies at commit 910be804. Extraction scripts use the actual source bodies, normalize CRLF and add original line directives; loader aliases only isolate stubbed callers from separately tested actual functions. The extractor is tailored to this source, not a general C parser. Mocks never access hardware.

```sh
cd /workspace/lxr-desktop-app
mkdir -p /tmp/phase-s4b
# Save the following blocks using the named paths.
PYTHONDONTWRITEBYTECODE=1 python3 /tmp/phase-s4b/run_erasure.py
PYTHONDONTWRITEBYTECODE=1 python3 /tmp/phase-s4b/run_load.py
cc -std=c11 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie -ffunction-sections -fdata-sections -Wl,--gc-sections -I. -Itests/fatfs_integrity -IFATFS/Target -IMiddlewares/Third_Party/FatFs/src -Itests/sd_storage /tmp/phase-s4b/filesystem_host.c -o /tmp/phase-s4b/filesystem_host
ASAN_OPTIONS=detect_leaks=0 /tmp/phase-s4b/filesystem_host
PYTHONDONTWRITEBYTECODE=1 python3 /tmp/phase-s4b/model.py
```

The whole-production FatFs fixture includes production `ff.c`, generic disk/registration and S1 through the existing test fixture; HAL/card behavior is RAM/mocked. The loader/cleanup tests extract complete relevant production bodies but mock filesystem and unrelated UI/audio/children. The transaction model is explicitly **abstract proposal logic**, not extracted production code. Neither synthetic directory behavior nor a model proves iOS or actual card behavior.

### /tmp/phase-s4b/extract.py

```python
import re
def extract(text,name):
 masked=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),text,flags=re.S)
 m=re.search(r'^(?:static\s+)?(?:FRESULT|void|uint8_t)\s+'+name+r'\s*\([^;]*?\)\s*\{',masked,re.M)
 assert m,name
 i=m.end();depth=1
 while depth:depth+=(masked[i]=='{')-(masked[i]=='}');i+=1
 return '#line '+str(text.count('\n',0,m.start())+1)+' "Core/Src/Preset/presetManager.c"\n'+text[m.start():i]+'\n'
```

### /tmp/phase-s4b/erase_host.c

```c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "Preset/PresetManager.h"
char currentFolder[13];
char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];
FIL SDFile;
static unsigned unlinks,fail_at;
static int guard_mode;
static const char *one;
static FRESULT preset_sharedStatus(void){return FR_OK;}
static FRESULT preset_openShared(FIL *f,const TCHAR *p,BYTE m){(void)f;(void)p;(void)m;return FR_NO_FILE;}
static FRESULT preset_closeShared(FIL *f){(void)f;return FR_OK;}
void preset_initTmpFolder(void){}
uint8_t preset_isNameEmpty(char *p){(void)p;return 0;}
FRESULT f_stat(const TCHAR *p,FILINFO *f){(void)p;(void)f;return FR_NO_FILE;}
FRESULT f_read(FIL *f,void *p,UINT n,UINT *got){(void)f;(void)p;(void)n;*got=0;return FR_OK;}
FRESULT f_opendir(DIR *d,const TCHAR *p){memset(d,0,sizeof *d);d->obj.sclust=strstr(p,"SUBDIR")?1:0;return FR_OK;}
FRESULT f_readdir(DIR *d,FILINFO *f)
{
 memset(f,0,sizeof *f);
 const char *names[]={"prj.nfo","glo.cfg","00-KICK1.SND","README.TXT","SUBDIR"};
 if(one){if(!d->dptr)strcpy(f->fname,one);d->dptr++;return FR_OK;}
 if(d->obj.sclust){if(!d->dptr)strcpy(f->fname,"nested.bin");d->dptr++;return FR_OK;}
 if(d->dptr<5){strcpy(f->fname,names[d->dptr]);if(d->dptr==4)f->fattrib=AM_DIR;d->dptr++;}
 return FR_OK;
}
FRESULT f_closedir(DIR *d){(void)d;return FR_OK;}
FRESULT f_unlink(const TCHAR *p){unlinks++;if(!guard_mode)printf("mock unlink %s\n",p);return guard_mode||unlinks==fail_at?FR_DENIED:FR_OK;}
#include "erase_functions.inc"
int main(int argc,char **argv)
{
 setvbuf(stdout,0,_IONBF,0);assert(argc>=2);
 if(!strcmp(argv[1],"overflow")){strcpy(currentFolder,"/proj00");one=argv[2];return empty_directory(currentFolder);}
 if(!strcmp(argv[1],"index")){preset_loadProjectNames(255);return 0;}
 if(!strcmp(argv[1],"guard")){char path[16]="/tmp/ABCDEFGHIJ";FILINFO f;guard_mode=1;FRESULT r=delete_node(path,sizeof path,&f);assert(r==FR_DENIED && unlinks==1);puts("PASS guard overflow sentinel 100 is overwritten by attempted unlink result; no real deletion performed");return 0;}
 char path[64]="/proj00";if(!strcmp(argv[1],"failure"))fail_at=2;
 FRESULT r=empty_directory(path);assert(r==(fail_at?FR_DENIED:FR_OK));assert(unlinks==(fail_at?2:6));
 puts("PASS actual deletion body: enumeration/depth-first ordering and first failure stop, without physical files");return 0;
}
```

### /tmp/phase-s4b/run_erasure.py

```python
from pathlib import Path
import subprocess,os,sys
sys.path.insert(0,'/tmp/phase-s4b');from extract import extract
root=Path('/workspace/lxr-desktop-app');p=Path('/tmp/phase-s4b')
s=(root/'Core/Src/Preset/presetManager.c').read_bytes().decode('latin1').replace('\r\n','\n')
(p/'erase_functions.inc').write_text(''.join(extract(s,n) for n in ['empty_directory','delete_node','preset_loadProjectNames']))
inc=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc','Middlewares/Third_Party/FatFs/src','FATFS/App','FATFS/Target']
cmd=['cc','-std=gnu11','-O1','-g','-DSTM32F765xx','-DUSE_HAL_DRIVER','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast']+['-I'+str(root/x) for x in inc]+['-I'+str(p),str(p/'erase_host.c'),'-o',str(p/'erase_host')]
subprocess.run(cmd,check=True);env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
for label,args,negative in [('metadata-overflow',['overflow','prj.nfo'],True),('normal-sfn-overflow',['overflow','00-KICK1.SND'],True),('small-control',['overflow','a.x'],False),('delete-order',['order'],False),('delete-failure',['failure'],False),('guard',['guard'],False),('project-index',['index'],True)]:
 r=subprocess.run([str(p/'erase_host')]+args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,env=env,timeout=10)
 (p/(label+'.log')).write_text('$ erase_host '+' '.join(args)+'\n'+r.stdout+f'\nEXIT {r.returncode}\n')
 assert (r.returncode!=0)==negative,(label,r.stdout)
 if negative:assert 'buffer-overflow' in r.stdout or 'out of bounds' in r.stdout,(label,r.stdout)
 print(label,'EXPECTED_SANITIZER_FINDING' if negative else 'PASS',r.returncode)
```

### /tmp/phase-s4b/load_host.c

```c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "Preset/PresetManager.h"
#include "Soundengine/Sequencer/sequencer.h"
#include "menu/menu.h"
#include "lcd.h"
char currentFolder[13],savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];
uint8_t mainParameters[NUM_KIT_SLOTS][END_OF_SOUND_PARAMETERS],activeParameters[NUM_PARAMS],*morphParameters;
uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS],menu_loadedPresetNr[NUM_CURRENT],seq_activePattern;
uint8_t seq_songData[NUM_KIT_SLOTS][MAX_SONG_LENGTH],lcd_process_display_interrupt;
PatternSet seq_patternSet;
FIL SDFile;TIM_HandleTypeDef htim2;
static FATFS hostfs;
static unsigned reads,children,index_writes,source_mode;
static int metadata_short;
static FRESULT preset_sharedStatus(void){return SDFile.obj.fs?FR_LOCKED:FR_OK;}
static FRESULT preset_openShared(FIL *f,const TCHAR *p,BYTE m){(void)p;(void)m;f->obj.fs=&hostfs;return FR_OK;}
static FRESULT preset_closeShared(FIL *f){f->obj.fs=0;return FR_OK;}
static void preset_makeFileNameOld(char *p,uint8_t n,uint8_t t){(void)n;(void)t;strcpy(p,"p000.sng");}
void preset_initTmpFolder(void){}
FRESULT preset_clearTmpFolder(void){return FR_OK;}
uint8_t preset_isNameEmpty(char *p){(void)p;return 0;}
FRESULT f_stat(const TCHAR *p,FILINFO *i){(void)p;(void)i;return FR_OK;}
FRESULT f_chdir(const TCHAR *p){(void)p;return FR_OK;}
FRESULT f_mkdir(const TCHAR *p){(void)p;return FR_OK;}
FRESULT f_read(FIL *f,void *p,UINT n,UINT *got)
{
 (void)f;reads++;
 if(source_mode==3){*got=n;if(n==1)*(uint8_t*)p=255;else memset(p,0x42,n);return FR_OK;}
 if(source_mode==4){*got=n;memset(p,n==1?255:16,n);return FR_OK;}
 if(source_mode==1){*got=n==8?8:0;if(*got)memcpy(p,"SONGNAME",8);return FR_OK;}
 *got=metadata_short?0:n;if(*got)memcpy(p,"PROJECTX",8);return FR_OK;
}
FRESULT f_findfirst(DIR *d,FILINFO *f,const TCHAR *p,const TCHAR *pattern)
{(void)d;(void)f;(void)p;(void)pattern;return source_mode==2?FR_DISK_ERR:FR_OK;}
FRESULT f_closedir(DIR *d){(void)d;return FR_OK;}
uint8_t preset_loadGlobals(void){return 0;}
FRESULT preset_loadDrumsetSD(uint8_t n,uint8_t m){(void)n;(void)m;children++;return FR_NO_FILE;}
FRESULT preset_loadPatternSD(uint8_t n){(void)n;children++;return FR_NO_FILE;}
FRESULT preset_loadSongSD(uint8_t n){(void)n;children++;return FR_NO_FILE;}
FRESULT preset_saveLastUsedProjectNrToSD(uint8_t n){(void)n;index_writes++;return FR_OK;}
void preset_loadDrumsetRam(uint8_t n,uint8_t m,uint8_t s){(void)m;(void)s;assert(n<NUM_KIT_SLOTS);memcpy(activeParameters,mainParameters[n],END_OF_SOUND_PARAMETERS);}
void seq_copyPatternToBackupBuffer(uint8_t n){(void)n;}
void seq_setNextPattern(uint8_t n){(void)n;}
uint8_t sendDisplayBuffer(void){return 0;}
void lcd_clear(void){}
void lcd_home(void){}
void lcd_setcursor(uint8_t x,uint8_t y){(void)x;(void)y;}
void lcd_string(const char *s){(void)s;}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *t){(void)t;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *t){(void)t;return HAL_OK;}
uint8_t getNumModTargets(void){return 255;}
void preset_sendDrumsetParameters(void){}
void preset_sendDrumsetParametersImmediate(void){}
void soundengine_process(void){}
#define FEXT_SOUND 0
#define FEXT_PAT 1
#define FEXT_ALL 2
#define FEXT_PROJ 3
#define FEXT_SONG 4
void numtostrpu_2Bytes(char *p,uint8_t n,char pad){(void)pad;p[0]=(char)('0'+n/10);p[1]=(char)('0'+n%10);}
#include "load_functions.inc"
int main(int argc,char **argv)
{
 (void)argv;if(argc>1){actual_ram_load(255,0,0);return 0;}
 for(metadata_short=0;metadata_short<2;metadata_short++){
  memset(mainParameters,0x42,sizeof mainParameters);memset(activeParameters,0x42,sizeof activeParameters);
  activeParameters[PAR_PATTERN_MODE]=0;memset(&seq_patternSet,0x33,sizeof seq_patternSet);memset(seq_songData,0x35,sizeof seq_songData);
  children=index_writes=0;reads=0;source_mode=0;
  assert(preset_loadProject(0,0)==FR_OK && children==192 && index_writes==1);
  assert(mainParameters[0][0]==0x42 && seq_patternSet.seq_subStepPattern[0][0][0].note==0x33 && seq_songData[0][0]==0x35);
  printf("PASS actual project loader returns FR_OK with %s metadata and all 192 assets absent; old RAM retained; index-save side effect reached (mocked)\n",metadata_short?"zero-byte":"8-byte");
 }
 source_mode=1;memset(seq_songData[0],0x69,MAX_SONG_LENGTH);assert(actual_song_loader(0)==FR_OK && seq_songData[0][0]==0x69);
 puts("PASS actual song loader returns FR_OK for zero-byte payload after 8-byte name; previous song data remains");
 source_mode=2;char name[13];assert(actual_name_lookup(name,0,4)==FR_NO_FILE);
 puts("PASS actual filename lookup collapses FR_DISK_ERR from directory search into FR_NO_FILE; real S1 may block a later fallback");
 source_mode=3;uint8_t project=0;assert(preset_loadLastUsedProjectNrFromSD(&project)==FR_OK && project==255);
 assert(preset_LoadTmpKit()==FR_OK && menu_selectedPresetNr[SAVE_TYPE_KIT]==255);
 puts("PASS actual root project-index and temporary-kit loaders publish unchecked slot255");
 source_mode=4;assert(preset_readPatternLengths(0,3) && seq_patternSet.kitNr[0]==255);
 puts("PASS actual pattern-tail loader accepts associated kit255 despite NUM_KIT_SLOTS=64");
 memset(mainParameters[0],0x42,END_OF_SOUND_PARAMETERS);actual_init(0);
 for(unsigned i=END_OF_SOUND_PARAMETERS-8;i<END_OF_SOUND_PARAMETERS;i++)assert(mainParameters[0][i]==0x42);
 puts("PASS actual init leaves last8 of247 kit bytes stale; available InitKit payload246 bytes needs explicit defaults, not a blind loop extension");
 printf("LAYOUT Step=%zu StepTiming=%zu PatternSet=%zu kitData=%u globals=%u patternV3=%zu song=%u projectBytes=%zu\n",sizeof(Step),sizeof(StepTiming),sizeof(PatternSet),END_OF_SOUND_PARAMETERS,NUM_PARAMS-PAR_BEGINNING_OF_GLOBALS,(size_t)(14+NUM_TRACKS*NUM_STEPS*(sizeof(Step)+1)+NUM_TRACKS*NUM_BARS*2+1+NUM_TRACKS+1),MAX_SONG_LENGTH+8,(size_t)(8+NUM_PARAMS-PAR_BEGINNING_OF_GLOBALS+64*(END_OF_SOUND_PARAMETERS+8+14+NUM_TRACKS*NUM_STEPS*(sizeof(Step)+1)+NUM_TRACKS*NUM_BARS*2+1+NUM_TRACKS+1+MAX_SONG_LENGTH+8)));
}
```

### /tmp/phase-s4b/run_load.py

```python
from pathlib import Path
import subprocess,sys,os
sys.path.insert(0,'/tmp/phase-s4b');from extract import extract
p=Path('/tmp/phase-s4b');root=Path('/workspace/lxr-desktop-app');s=(root/'Core/Src/Preset/presetManager.c').read_bytes().decode('latin1').replace('\r\n','\n')
body=extract(s,'preset_makeFileNameLoad').replace('preset_makeFileNameLoad(', 'actual_name_lookup(')
body+='\n#define preset_makeFileNameLoad song_name_lookup\n#define preset_loadSongSD actual_song_loader\n'+extract(s,'preset_loadSongSD')+'\n#undef preset_makeFileNameLoad\n#undef preset_loadSongSD\n'+extract(s,'preset_loadProject')+extract(s,'preset_loadLastUsedProjectNrFromSD')+extract(s,'preset_LoadTmpKit')+extract(s,'preset_readPatternLengths')+extract(s,'preset_init').replace('preset_init(', 'actual_init(')+extract(s,'preset_loadDrumsetRam').replace('preset_loadDrumsetRam(', 'actual_ram_load(')
# Separate the tested actual filename function from the song fixture's successful open.
body='static FRESULT song_name_lookup(char *p,uint8_t n,uint8_t t){(void)p;(void)n;(void)t;SDFile.obj.fs=&hostfs;return FR_OK;}\n'+body
(p/'load_functions.inc').write_text(body)
inc=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc','Middlewares/Third_Party/FatFs/src','FATFS/App','FATFS/Target']
cmd=['cc','-std=gnu11','-O1','-g','-DSTM32F765xx','-DUSE_HAL_DRIVER','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast']+['-I'+str(root/x) for x in inc]+['-I'+str(p),str(p/'load_host.c'),str(root/'Core/Src/Preset/initKit.c'),'-o',str(p/'load_host')]
r=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(p/'load-compile.log').write_text(r.stdout);print(r.stdout,end='');assert r.returncode==0
env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0';r=subprocess.run([str(p/'load_host')],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,env=env,timeout=10);(p/'load-results.log').write_text(r.stdout+f'\nEXIT {r.returncode}\n');print(r.stdout);assert r.returncode==0

r=subprocess.run([str(p/'load_host'),'badkit'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,env=env,timeout=10)
(p/'kit-index-consumer.log').write_text(r.stdout+f'\nEXIT {r.returncode}\n');assert r.returncode!=0 and 'out of bounds' in r.stdout;print('EXPECTED_SANITIZER_FINDING actual kit RAM consumer rejects no slot255 boundary')
```

### /tmp/phase-s4b/filesystem_host.c

```c
/* Whole production FatFs/disk/S1 units from the established RAM fixture. */
#define main prior_integrity_main
#include "tests/fatfs_integrity/host.c"
#undef main
static int bad_tail;
static DRESULT read_with_tail(BYTE lun,BYTE *p,DWORD s,UINT n)
{if(bad_tail && s==98)return RES_ERROR;return fake_read(lun,p,s,n);}
static const Diskio_drvTypeDef tail_driver={fake_init,fake_status,read_with_tail,fake_write,fake_ioctl};
static void seed(void)
{
 setup();memset(Files,0,sizeof Files);FATFS_UnLinkDriver(drive);assert(FATFS_LinkDriver(&tail_driver,drive)==0);
 BYTE *b=media[0];b[0]=0xeb;b[1]=0x3c;b[2]=0x90;memcpy(b+BS_FilSysType,"FAT16   ",8);
 fixture_put16(b+BPB_BytsPerSec,512);b[BPB_SecPerClus]=1;fixture_put16(b+BPB_RsvdSecCnt,1);b[BPB_NumFATs]=2;
 fixture_put16(b+BPB_RootEntCnt,512);fixture_put16(b+BPB_TotSec16,8192);fixture_put16(b+BPB_FATSz16,32);
 b[BPB_Media]=0xf8;fixture_put16(b+BS_55AA,0xaa55);
 media[1][0]=0xf8;media[1][1]=media[1][2]=media[1][3]=0xff;
 fixture_put16(media[1]+4,3);fixture_put16(media[1]+6,0xffff);fixture_put16(media[1]+8,0xffff);memcpy(media[33],media[1],512);
 memcpy(media[65],"PROJ00     ",11);media[65][DIR_Attr]=AM_DIR;fixture_put16(media[65]+DIR_FstClusLO,2);
 for(unsigned i=0;i<16;i++){BYTE *d=media[97]+i*32;char name[12];snprintf(name,sizeof name,"X%07uTXT",i);memcpy(d,name,11);d[DIR_Attr]=AM_ARC;}
 memcpy(media[97],"00-KICK SND",11);media[97][DIR_Attr]=AM_ARC|AM_HID|AM_SYS;
 fixture_put16(media[97]+DIR_FstClusLO,4);media[97][DIR_FileSize]=8;memcpy(media[99],"kickfour",8);
}
int main(void)
{
 seed();FATFS mounted;memset(&mounted,0,sizeof mounted);assert(f_mount(&mounted,drive,1)==FR_OK);bad_tail=1;
 FIL f;memset(&f,0,sizeof f);UINT got;char data[8];
 assert(f_open(&f,"/proj00/00-KICK.SND",FA_READ)==FR_OK && f_read(&f,data,8,&got)==FR_OK && got==8 && !memcmp(data,"kickfour",8) && f_close(&f)==FR_OK);
 DIR d;FILINFO info;assert(f_findfirst(&d,&info,"/proj00","00-*.snd")==FR_OK && info.fname[0] && (info.fattrib&AM_HID) && (info.fattrib&AM_SYS));assert(f_closedir(&d)==FR_OK);
 assert(f_opendir(&d,"/proj00")==FR_OK);unsigned count=0;FRESULT r;
 while((r=f_readdir(&d,&info))==FR_OK && info.fname[0])count++;
 assert(r==FR_DISK_ERR && count==16);f_closedir(&d);assert(writes==0);
 puts("PASS actual FatFs: hidden/system SFN file read and early wildcard search succeed while full enumeration hits a mocked unreadable later directory cluster; no media writes");
 _FDID object={0};object.fs=&mounted;
 fixture_put16(media[1]+4,0xffff);fixture_put16(media[33]+4,3);mounted.winsect=0xffffffffu;
 assert(get_fat(&object,2)==0xffff);
 fixture_put16(media[1]+4,3);fixture_put16(media[33]+4,0xffff);mounted.winsect=0xffffffffu;
 assert(get_fat(&object,2)==3);assert(writes==0);
 puts("PASS actual get_fat reads first FAT despite conflicting second FAT; this does not identify iOS FAT policy or real card state");
 /* Invalid LFN metadata is ignored under the production _USE_LFN=0 configuration. */
 memcpy(media[97]+32,media[97],32);memset(media[97],0x41,32);media[97][DIR_Attr]=AM_LFN;mounted.winsect=0xffffffffu;
 assert(f_findfirst(&d,&info,"/proj00","00-*.snd")==FR_OK && info.fname[0]);f_closedir(&d);
 puts("PASS actual SFN-only directory path ignores malformed LFN entry and still finds SFN");
 media[97][0]=0;mounted.winsect=0xffffffffu;
 assert(f_findfirst(&d,&info,"/proj00","00-*.snd")==FR_OK && !info.fname[0]);f_closedir(&d);
 assert(f_open(&f,"/proj00/00-KICK.SND",FA_READ)==FR_NO_FILE);
 puts("PASS zero end-marker before data makes both enumeration and named lookup miss it: simple emptiness alone cannot explain successful same-path reads");
}
```

### /tmp/phase-s4b/model.py

```python
"""Abstract transaction proposal, not production/FAT/card execution."""
import copy

old = {"files": [True] * 194, "manifest": True, "commit": True}
def usable(g):
    return g["commit"] and g["manifest"] and all(g["files"])
def recover(d):
    return next((n for n in sorted(d, reverse=True) if usable(d[n])), None)
steps = ["mkdir"] + [("file", i) for i in range(194)] + ["manifest", "verify", "commit", "confirm"]
checks = 0
for cut in range(len(steps) + 1):
    d = {1: copy.deepcopy(old)}
    for event in steps[:cut]:
        if event == "mkdir":
            d[2] = {"files": [False] * 194, "manifest": False, "commit": False}
        elif isinstance(event, tuple):
            d[2]["files"][event[1]] = True
        elif event == "manifest": d[2]["manifest"] = True
        elif event == "commit": d[2]["commit"] = True
    before = copy.deepcopy(d)
    choice = recover(d)
    assert choice in (1, 2) and usable(d[1])
    assert choice == (2 if 2 in d and usable(d[2]) else 1)
    for restart in range(10): assert recover(d) == choice and d == before
    checks += 1
print(f"PASS abstract model: {checks} crash cuts x 10 read-only recoveries; old generation untouched")
for bad in ("commit", "manifest", "payload"):
    d = {1: copy.deepcopy(old), 2: copy.deepcopy(old)}
    if bad == "payload": d[2]["files"][37] = False
    else: d[2][bad] = False
    assert recover(d) == 1
print("PASS abstract invalid/partial record or payload rejects new generation; previous selected")
# Counterexample deliberately outside the isolated-logical-write failure model.
d = {1: copy.deepcopy(old), 2: copy.deepcopy(old)}
for g in d.values(): g["files"][0] = False
assert recover(d) is None
print("LIMIT demonstrated: shared filesystem/media corruption can destroy both generations; model gives no unconditional FAT/SD durability guarantee")
```

## Appendix F — Final review checklist

* Production edits: **none**. Source branch stays `safety/sd-silent-autosave-recovery-s4l2` at `910be804c7df2757f8d2347880751f2ca86c671c`.
* Authorized report-only change: this Markdown file, based directly on lab-setup `5829398cea5d337200393757e39a37d4d8148256`.
* Existing regressions/builds: ten suites and all four configurations PASS; image/header/payload/checksum/vector/LMA validations PASS. Expected new negative sanitizer findings remain **unfixed**.
* Actual PROJ00 diagnosis/recovery, physical timing, power-cut/card tests: **NOT PERFORMED**.
* Proposed generation format, loader/epoch/ownership changes and containment fixes: **NOT IMPLEMENTED**; require review/authorization.
* No branch merge, force-push, previous-report overwrite, firmware publication, physical card access, bootloader modification or hardware operation.
* **NOT FLASHED — NOT HARDWARE VALIDATED.** Next step is review of the P0 containment fixes and the namespace/compatibility/conditional-guarantee decision, followed by separately authorized software implementation.
