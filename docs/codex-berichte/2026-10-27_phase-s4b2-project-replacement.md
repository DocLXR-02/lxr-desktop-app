# LXR Firmware Lab — Phase S4b-2: Bounded Project Replacement and Recovery

Date: 2026-10-27 UTC. Repository: **DocLXR-02/lxr-desktop-app**.

## 1. Outcome and evidence boundaries

**IMPLEMENTED — software validation passed; published for review only.** Existing full-project overwrite works again. The implementation writes a complete checked staging project before moving the old project aside, publishes the replacement under conventional `PROJ00`–`PROJ63`, and bounds temporary storage to one staging directory, one backup directory and one small transaction record globally. It never accumulates generations. Successful projects retain the existing payload files and byte formats.

All eleven established regression suites and the new production/FatFs transaction suite passed. All four ARM configurations built and their images passed header, checksum, payload, vector and memory checks. Source and tested tree are identified below. No physical device/card was accessed; no firmware was flashed or published. **NOT FLASHED — NOT HARDWARE VALIDATED — NOT APPROVED FOR INSTALLATION.**

The safety claim is deliberately conditional: tested isolated API failures and tested logical/sector publication interruptions preserve a complete readable old or new version. FAT rename is not atomic. Some interrupted renames leave duplicate directory entries pointing at the same cluster chain; these are detected and held read-only rather than automatically repaired or deleted. Torn sectors, reordered/cached card writes, arbitrary shared-FAT corruption and actual peripheral quiescence are outside the proven model. Independent backups remain necessary.

Evidence labels used throughout:

- **CODE**: established in inspected source.
- **BUILD/HOST**: actually executed compiler, binary or host simulation evidence.
- **OPEN**: requires review or physical validation; no measured runtime claim.

## 2. Exact repository state and instructions

| Item | Reference |
|---|---|
| Authorized baseline branch | `safety/sd-project-containment-s4b1` |
| Verified baseline commit / source parent | `e49bb028462c339c4998cd08a845db17e1fc778f` |
| Baseline tree | `842396dafc554d9fb88ec7cd7b48a4d16484aad7` |
| New source branch | `safety/sd-project-transaction-s4b2` |
| Published source commit | **`5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527`** |
| Exact reviewed, built and published tree | `aca77103ba2509431cc966febc0ec6468defac6a` |
| Report-only base | `lab-setup`, `5829398cea5d337200393757e39a37d4d8148256` |
| Preserved main | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Report branch | `codex-reports/2026-10-27-phase-s4b2-project-replacement` |
| Report path | `docs/codex-berichte/2026-10-27_phase-s4b2-project-replacement.md` |

Source branch: https://github.com/DocLXR-02/lxr-desktop-app/tree/safety/sd-project-transaction-s4b2

Source commit: https://github.com/DocLXR-02/lxr-desktop-app/commit/5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527

The existing `/workspace/lxr-desktop-app` checkout was inspected before editing, preserving all work; it was clean at the requested baseline. The remote S4b-1 head matched exactly. `git merge-base --is-ancestor 5829398cea5d337200393757e39a37d4d8148256 e49bb028462c339c4998cd08a845db17e1fc778f` passed. The new source branch was created directly from e49; no worktree, reset, merge or replacement of a prior branch was used.

The complete lab-setup `AGENTS.md` was read (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`). The complete S4b-1 handoff was read from report branch `codex-reports/2026-10-27-phase-s4b1-project-containment`, commit `09ed6500ed592a418ba1689504c801268087e062`, file `docs/codex-berichte/2026-10-27_phase-s4b1-project-containment.md`, blob `2d5411d023486ee8aa11b2957a754938e6700d8f`. `docs/storage-design-s4b.md` was read and updated narrowly. The preceding architecture report is commit `5886663f95ce4305cecac33bc856243f05c09aac`, report branch `codex-reports/2026-10-27-phase-s4b-project-storage-design`, file `docs/codex-berichte/2026-10-27_phase-s4b-project-storage-design.md` (blob `ce222c2679e1bb5bd249743c9223ea26770caaa3`).

Ancestry retained: lab-setup → Phase A e19bb054 → B e4c0fd3c → C 091f502d → S1 a4be3b94 → S2 c88933a4 → S3 cbffa2d7 → S4a c87666ff → S4-L feaff649 → S4-L2 910be804 → S4b-1 e49bb028 → this commit. No prior fix was replaced. The source still identifies as **1.7**; no equivalence to public manufacturer v1.71 is asserted. The privately supplied v1.72 is not the public reference.

Publication used the authorized GitHub Git-data interfaces: exact base64 blobs (preserving original CRLF files), tree creation from e49, comparison against `git write-tree`, one-parent commit and a new branch. The published commit was fetched and its tree rechecked; the new local branch was fast-forwarded to that exact commit. All twelve changed paths match the published commit; no binaries/build directories are committed. A 34-branch remote snapshot preceded publication; final checks compare every pre-existing head unchanged. The separate report branch contains only this new Markdown file over lab-setup. Its own future self-hash is not embedded recursively: the containing report commit is available in GitHub metadata and the final publication handoff.

## 3. Initial architecture findings and design choice

**CODE:** S4b-1 eliminated recursive destructive replacement and the path overflow, validated project/kit ranges and short metadata/song reads, but intentionally rejected every existing project directory. It was safe containment, not replacement.

Actual FatFs configuration uses short filenames (`_USE_LFN=0`), one volume, non-exFAT, 512-byte sectors and `_FS_LOCK=1`. A directory handle and a file handle cannot remain open simultaneously under that lock limit. The implementation therefore inventories short filenames into a fixed foreground workspace, closes the DIR, then opens each FIL sequentially. There are no whole-project RAM copies, runtime allocations, recursive deletes, background scanning, new interrupts or locks.

`Middlewares/Third_Party/FatFs/src/ff.c:4724` implements `f_rename()`: register destination, copy object information, remove source, then `sync_fs()` (`:4809`). The operations may touch different directory sectors. A reset between writes can leave two names pointing to one chain. `f_unlink()` (`:4532`) frees the chain, so deleting either alias would endanger the surviving name. This was reproduced using the complete production filesystem, not assumed away. Renaming within the root avoids modifying the moved directory's `..` parent field; root membership and directory cluster ID can be verified afterward, but the rename still is not atomic.

Chosen smallest practical arrangement:

| Name | Role | Lifetime |
|---|---|---|
| `/proj00` … `/proj63` | Conventional canonical project | Permanent normal layout |
| `/LXRSTG` | One full replacement under construction | Temporary, global singleton |
| `/LXRBAK` | Previous validated project during publication/cleanup | Temporary, global singleton |
| `/LXRTRANS.DAT` | 32-byte ownership header plus optional 16-byte completion footer | Temporary, global singleton |

All reserved names are FAT 8.3 compatible. A transaction for another slot blocks new full-project saves with `FR_LOCKED`; it does not scan/migrate other slots. No transaction database, unlimited archive or permanent generation selector was introduced. Reserved paths without a valid owning transaction are collisions, not disposable leftovers.

Why a record is needed: names alone cannot establish ownership, a stage's completion or whether a partially cleaned backup is still complete. A fixed immutable header records old root/count/digest before staging; a completion footer records the fully validated new root/digest. This is bounded protocol metadata, not a new project payload format. Two-directory rename without completion/ownership evidence would permit unsafe selection or cleanup. Overwriting each original file would lose project-wide consistency and previous data. Unlimited immutable generations were explicitly rejected.

## 4. Source changes and decisive references

All line references refer to source commit `5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527`.

| Changed file | Purpose |
|---|---|
| `Core/Src/Preset/projectStorage.inc` (new, 515 lines) | Private bounded transaction, structural validator, CRC/record, selection and cleanup logic; included only by presetManager |
| `Core/Src/Preset/presetManager.c` | Include private implementation (`:227`), factor existing serializers into checked project writer (`:2172`), transaction save wrapper (`:2197`), read-only recovery-aware load (`:2297`) |
| `Core/Inc/Preset/PresetManager.h:96` | Two foreground-only recovery queries used by startup |
| `Core/Src/main.c:447` | Avoid creating canonical directories in a rename gap; default globals in RAM when necessary; skip stale tmp overlay during recovery (`:882`) |
| `docs/storage-design-s4b.md` | Concise bounded workflow, live-performance principles and limits |
| `tests/project_transaction/run.py` (new) | Sanitized complete-production-body/whole-FatFs runner |
| `tests/project_transaction/whole.c` (new) | FAT16/FAT32 RAM-media, API/sector fault and reboot tests |
| `tests/project_transaction/README.md` (new) | Reproduction and precise mock/physical limitations |
| `tests/project_storage/run.py` | Shared extraction of unchanged production bodies and new private include; keep S4b-1 checks |
| `tests/project_storage/host.c` | Adapt filesystem mocks to bounded replacement and retain path/index/legacy tests |
| `tests/project_storage/README.md` | Clarify preserved gate tests versus new complete replacement |
| `tests/save_errors/run.py` | Expand private include and extraction return types; keep serializer oracle/fault checks intact |

Diff: **12 files, 1,131 insertions, 114 deletions**. No FatFs, HAL/BSP, S1 disk, audio, sequencer, MIDI, USB allocator, DSP, linker, startup, clock, interrupt-priority, bootloader or updater implementation changed. Main changes are confined to project boot persistence behavior; the disabled bootloader-update block is unchanged. `UPDATE_BOOTLOADER` remains **0** (`Core/Src/main.c:17`). `LXR_RUNTIME_DIAGNOSTICS` remains OFF by default (`CMakeLists.txt:72`).

Private implementation reference map:

| Function / area | `Core/Src/Preset/projectStorage.inc` line |
|---|---:|
| Names, fixed workspace and profile constants | 6 |
| Serious mutating-error hold | 26 |
| CRC32 / explicit little-endian helpers | 34 |
| Case-insensitive fixed-slot filename classifier | 72 |
| Directory identity inspection | 98 |
| Flat bounded inventory / duplicate rejection | 116 |
| Streaming strict/legacy validation | 146 |
| Immutable ownership header | 213 |
| Record parsing and completion-footer validation | 224 |
| Checked seal append and readback | 251 |
| Complete-candidate comparison | 271 |
| Deterministic recovery selection | 285 |
| Redundant owned directory removal | 317 |
| Explicit-Save normalization | 342 |
| Free-cluster preflight | 407 |
| New replacement execution | 425 |
| Read-only load selection / failed-recovery guard | 480 |
| Foreground recovery queries | 506, 511 |

Decisive publication sequence (excerpt):

```c
res = preset_writeProjectData(slot);
if(res != FR_OK) return res;
res = preset_txnValidate(PROJECT_STAGE, 1,
                         &t.newRoot, &t.newHash, &check.oldCount);
if(res != FR_OK) return res;
res = preset_txnSeal(&t);
if(res != FR_OK) return res;
res = f_chdir("/");
if(res != FR_OK) return res;
if(t.oldRoot) {
    res = preset_txnMutation(f_rename(canonical, PROJECT_BACKUP));
    if(res != FR_OK) return res;
}
res = preset_txnMutation(f_rename(PROJECT_STAGE, canonical));
if(res != FR_OK) return res;
```

Deletion is permitted only after enumeration, expected-root checking and independent complete-version selection. `preset_txnPrepare()` explicitly refuses alias roots:

```c
if((c && (c == s || c == b)) || (s && s == b)) return FR_INT_ERR;
```

A mutating API error can leave a dirty shared FatFs window even without a live FIL. Relevant errors therefore retain the existing application hold:

```c
if(res == FR_DISK_ERR || res == FR_INT_ERR || res == FR_TIMEOUT ||
   res == FR_NOT_READY || res == FR_INVALID_OBJECT)
    if(preset_sharedFailure == FR_OK) preset_sharedFailure = res;
```

This does not clear the S1 disk latch or invent peripheral recovery. Logical precondition errors (`FR_DENIED`, `FR_EXIST`, `FR_LOCKED`) remain retryable; nine harmless failed opens followed by a successful tenth Save are tested. Directory-close ownership failures are also retained. A failed stage write/short-write/close uses the existing S4a shared-file policy: no force-close, err clearing, reuse or automatic replay.

## 5. Validation contract and unchanged bytes

The new strict stage comprises exactly **194 files / 255,398 payload bytes**:

- `prj.nfo`: 8 bytes, written last.
- `glo.cfg`: 30 bytes under current parameter enum.
- 64 kit files: 255 bytes each.
- 64 PatV03 pattern files: 3,663 bytes each.
- 64 song files: 72 bytes each.

Actual existing checked serializers are reused. The moved order is globals → 64 kit/pattern/song triplets → project metadata last. **Individual file byte sequences are unchanged**; no new fields/padding, kit numbers, lengths or song markers are introduced. The existing frozen S3 oracle passed 1,536 successful save comparisons and all 2,544 injected write/short-write positions. Tests also check all194 canonical file entries and their full structural/fingerprint profile after repeated replacement.

Inventory rejects unknown filenames, directories, invalid/duplicate slot keys and excessive entries before deleting anything. All project/child paths retain S4b-1 explicit capacities (canonical7+NUL; child buffer21 for slash+maximum8.3+NUL). `currentFolder[13]` is never appended to. All byte-valued invalid indices and associated-kit values remain covered. Case-insensitive names follow FAT semantics; arbitrary path separators and non-SFN components cannot enter the cleanup list.

Strict stage validation reads every byte through a fixed aligned512-byte buffer; checks exact required inventory/sizes, PatV03/version, all seven literal lengths1–64, associated kit0–63 and serializer-derived SFN/name consistency. It closes each read handle and checks the close. CRC32 (standard vector `123456789` → `CBF43926`) frames each file with LE16 key and LE32 size, then hashes content in stable key order. The digest is an inexpensive corruption/identity check, **not cryptographic authentication** and **not an independently computed checksum of intended RAM output**. Checked serializer write counts/closes establish API acceptance; the read pass establishes readable structurally valid contents and fingerprints those bytes.

Existing-project validation intentionally supports documented legacy/incomplete layouts: requires an eight-byte `prj.nfo`, validates every present recognized file, allows missing kit/pattern/song/global files that the S4b-1 loader defaults, old `p000.*` slot names, older short kit tails, V1 untagged/tagged patterns with supported optional length tails and V2/V3 pattern bodies. Legacy zero lengths are decoded by the existing Euclidean reader; strict new stages require literal values. Short present songs, malformed metadata, unsupported pattern versions, invalid kit references and unknown contents are rejected rather than replaced. This is not a requirement that every historical project already contain all194 files. The baseline project_storage/Euclidean legacy tests remain passing.

Old-project count/root/CRC in the immutable header prevents a partially deleted backup from being mistaken for that previous complete profile. New-project validation is completed **before** any canonical rename. After publication, the code confirms directory-root identity, disappearance of the stage, expected backup identity, and the canonical nfo's type/size. It does not perform a second full payload read: same-root-parent rename does not rewrite validated child payloads. Physical media failure after validation remains outside an absolute guarantee.

## 6. Record layout, free space and operation ordering

`/LXRTRANS.DAT` is created exclusively; existing contents are never overwritten as an ownership shortcut. All integers are explicit little-endian; C struct padding is not serialized.

| Offset | Bytes | Meaning |
|---:|---:|---|
| 0 | 8 | `LXRS4B2T` |
| 8 | 1 | Schema1 |
| 9 | 1 | Slot0–63 |
| 10 | 2 | Old recognized-file count |
| 12 | 4 | Old directory cluster ID (0 for first save) |
| 16 | 4 | Old framed-content CRC32 |
| 20 | 8 | Reserved zero |
| 28 | 4 | CRC32 of first28 bytes |
| 32 | 4 | New validated directory cluster ID |
| 36 | 4 | New framed-content CRC32 |
| 40 | 2 | Complete file count194 |
| 42 | 2 | Reserved zero |
| 44 | 4 | CRC32 of first44 bytes |

Header is32 bytes. A sealed record is48 bytes. A partial/invalid footer never authorizes publication. A corrupt header is not trusted to identify a directory for cleanup. A strict-complete canonical can be loaded independently when record contents are invalid, but project writes remain held pending controlled attention. This intentionally preserves evidence rather than silently deleting unknown data.

Before creating the header, `f_getfree("0:")` checks space for every staged file rounded to the actual cluster size, directory entries, record and conservative root growth. With one512-byte cluster this estimate is657 clusters /336,384 bytes. Larger-cluster cards need proportionally more allocated space because each of194 files occupies at least one cluster. This is a preflight using FatFs accounting, not a reservation or guarantee against externally modified/stale free-space metadata. If ordinary space is insufficient, Save returns `FR_DENIED` without creating transaction files; freeing space permits a later explicit retry. The old project is never deleted to make room.

Normal sequence:

1. Verify shared-file state, index0–63, active pattern/kit bounds and every pattern's associated kit.
2. Inspect only the singleton record/reserved paths; reject unexplained collisions or another slot's transaction.
3. On an explicitly requested Save, normalize only a previously owned, validated non-alias transaction (details below).
4. Check free space. Validate/fingerprint existing canonical with the legacy profile if present.
5. Create, write, close and read back ownership header.
6. Reserve absent stage; enter it; write all checked current serializers; write metadata last.
7. Strictly stream-validate stage; append and verify completion footer.
8. Leave stage CWD for `/`. If replacing, rename canonical→backup; rename stage→canonical.
9. Confirm expected root identities/nfo and absent stage. Only now is the old backup redundant.
10. Enumerate/close backup DIR, reject unknown contents, unlink only recognized children and then that owned backup directory. Delete record last.
11. After successful transaction, run pre-existing whitelist temporary-file cleanup and restore `/tmp`.
12. Return `FR_OK` only if all required operations succeeded. Cleanup/tmp errors return failure even if publication has already committed the new project. No automatic retry is performed.

`preset_saveProject()` does not falsely clear autosave dirty flags after a failure. Existing RAM capture of the active kit/pattern is retained; it is not a whole-project rollback/snapshot. New Save after a safe logical error writes current RAM, not an old staged snapshot presented as current data. If restoring `/tmp` after Save fails, the application retains an error rather than allowing later relative writers to inherit an unresolved stage CWD.

## 7. Failure and recovery matrix

C=canonical, S=stage, B=backup. “Valid” means structural profile and expected root/count/digest where the record provides them. The previous project is required to have passed baseline validation; an intentionally incomplete legacy layout is validated as its actual prior profile.

| Interruption/failure point | Complete version retained | Recovery load | Explicit later Save |
|---|---|---|---|
| Before record/preflight | Old C unchanged | Normal C | Retry after logical issue corrected |
| Header creation/write/close | Old C unchanged; header may be absent/invalid | Valid old C independently; strict fallback if header malformed | Malformed ownership record is held, not auto-deleted |
| Header valid; no stage or incomplete stage | Old C unchanged | Matching old C | Discard only owned redundant recognized S; retry current RAM |
| Stage fully written but footer absent, old exists | Old C; complete S may also exist | Prefer old C | Remove redundant owned S only after old validation |
| First-ever save, incomplete S, no old | No previously complete project existed | Honest load failure | May discard recognized incomplete owned S on explicit Save; unknown contents retained |
| First-ever save, fully closed strict S before seal | Complete S | Read-only strict S | Revalidate, establish fresh seal, publish sole complete version before starting a new Save |
| Sealed S; C still old | Complete old C and new S | Prefer old C | Normalize and perform requested fresh Save |
| C→B complete, before S→C | Complete old B and new S; C absent | Prefer matching old B | Restore B→C first, check identities, then proceed |
| S→C complete, B old | Complete new C and old B | Prefer matching new C | Clean only redundant owned B/record; proceed |
| Backup cleanup partial/failed | Complete new C; B may be incomplete | New C; old hash/count will reject partial B | Resume known redundant cleanup only if unambiguous |
| Record deletion complete | Complete new C, no reserved data | Normal C | Normal repeated replacement |
| Cross-sector rename alias | One chain under two names; complete old/new retained in tested cuts | Read-only validated preference | `FR_INT_ERR`, no unlink/project write; controlled external attention required |
| Unknown files/dirs or reserved collisions | Existing data untouched | Normal unrelated projects; record rules for target | `FR_DENIED`/`FR_EXIST`, no unknown deletion |
| Serious physical/mutating/file ownership failure | Last verified version retained under isolated-fault model | In-session access may be held; reboot does not prove media healthy | Existing app/S1 holds prevent replay; no reset/remount/repair |
| Pending transaction for another slot | Its C/S/B retained | Other normal slot can be selected | New full-project transaction returns `FR_LOCKED` |

Read-only selection prefers sealed new canonical, then matching old canonical/backup, then sealed stage, with the strict unsealed first-save exception above. Failed candidate validation never silently chooses an incomplete directory. Communication errors abort rather than being reclassified as “missing file.” When no safe selection exists, an error is returned and evidence remains.

Startup changes are directly required: it no longer creates `/proj00` in a publication gap or creates default globals while a transaction is pending. Recovery loads skip `/proj.idx` updates, tmp cleanup/creation and old tmp overlays. Even a failed recovery sets the guard, so startup cannot overlay stale temporary data and pretend it is the chosen project. Ordinary non-transaction legacy loading retains the S4b-1 defaulting behavior and tmp workflow. No boot-time rename, repair, cleanup, whole-card scan or automatic payload replay is added.

Explicit normalization is bounded to the current slot. If canonical is absent, it first restores/publishes an independently validated complete reference, verifies root identities/source absence, then discards only redundant owned data. An interrupted restore can be attempted again by another explicit Save, as tested. It is not automatic background recovery. No safe alias repair with public `f_unlink()` was found; this specific state stays read-only.

## 8. Host test architecture and exact results

Execution environment: existing prepared Linux cloud checkout; native GCC14.2.0, Python3.12.14; no physical card/HAL/USB/audio execution. Native production-body tests use ASan/UBSan where supported, `-fno-sanitize-recover=all`, `-fno-pie -no-pie`; new fixture disables leak checking for fixed host fixture allocations. Existing tests retain their own sanitizer/static checks. No required suite was skipped or weakened to hide a failure.

New runner: `python3 tests/project_transaction/run.py`. Complete extracted production project/path/save/load/transaction bodies are compiled with whole production `ff.c`, `diskio.c`, `ff_gen_drv.c` and unchanged init-kit/sequence helpers. Extraction uses the existing source-body helper, not a separately rewritten algorithm. Public API seams inject only chosen failures; non-injected calls run real FatFs. TIM2/LCD/sound sends are mocks. RAM sectors model isolated faults and fail-closed I/O, **not the actual S1 hardware driver**; other existing suites test the actual S1/S2/S3 units and their integration.

Volumes: synthetic FAT16 and FAT32,512-byte sectors, one-sector clusters, two FATs, unrelated root entries forcing cross-sector renames. Simulated reboot discards FatFs/handle/application volatile state while retaining acknowledged sector contents. Sectors are atomic and written in program order. A power cut is simulated before/after chosen operations via longjmp, not by executing real firmware.

| New group | Actual result |
|---|---|
| `normal` | New save plus20 replacements on each FAT16/FAT32 volume; exact194-file layout, constant free clusters, no reserved-path accumulation; load and CRC vector pass |
| `faults` | **118** cases: writes before/after acceptance, read/open/close/sync/short-write failures, invalid/truncated stage, kit255, unknown stage contents, rename/cleanup denial, nine harmless failures then success; no false success or unsafe replay |
| `boundaries` | **50** before/after logical cuts plus repeated interrupted explicit recovery; full expected old/new CRC retained; recovery load performs zero physical writes |
| `sector` | **16** before/after cuts spanning every sector write of both cross-sector publication renames on both filesystems; **8 alias states** detected, no deletion/further project writes, complete old/new retained |
| `edge` | **36** full-card/retry, unknown/collision/invalid record/index, failed-recovery overlay guard and first-ever-save interruption/rescue cases |

Total new injected edge/failure/interruption cases: **220**, in addition to42 normal new/replacement saves and subsequent validation/load checks. A baseline sampled successful replacement in this fixture produced FAT16 **2,087 sector-write calls,15,578 read calls,395 syncs,587 opens,586 closes**; FAT32 **2,497 writes,16,393 reads,395 syncs,587 opens,586 closes**. These are instrumented host I/O call counts with particular geometry/root entries, not a device latency measurement. The mismatched open/close count includes the deliberate absent-record open. Validation and FatFs path traversal add real foreground I/O; this overhead must be measured on cards. No misleading “negligible runtime cost” claim is made.

All required suite commands returned **exit0**:

```text
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
python3 tests/project_storage/run.py
python3 tests/project_transaction/run.py
```

The first eleven are the established suites. The S4b-1 low-level absent-directory gate remains tested directly; the complete Save now legitimately allows validated replacement instead of blanket FR_EXIST. Unit fixtures were extended with rename/free-space operations; unknown-data and wrong-CWD protections remain. The S3 serializer oracle, S1 latch, S2 timeouts, S3 public sync/close errors and silent S4-L2 backoff tests were not suppressed. Exact final suite output is appended later in this report.

## 9. Build, image and resource verification

Every shell used:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

Tools actually reported: CMake **3.31.10**; Ninja **1.13.0.git.kitware.jobserver-pipe-1**; Python **3.12.14**; ARM GCC **14.2.1 20241119 (15:14.2.rel1-1)** with prepared newlib. No toolchain/config upgrade was made. Comparisons use the S4b-1 measurements from the same prepared environment and recompiled baseline source for object/stack comparison.

Executed, all exit0:

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

The four build commands and image checks were repeated at the published source commit, with no source modifications and unchanged hashes. The build system prints existing update instructions as a normal informational target; none were executed. No full-device/bootloader build or flashing target was invoked.

Actual outputs per directory are `LXR_V2_STM32F765.elf`, `LXR_V2_STM32F765.bin`, `LXR_V2_V1.7.img` (all local, ignored, not published). Image validation checks magic `LXRV2IMG`, header16 bytes, LE32 payload length, checksum byte `0xFF - (sum(payload) & 0xFF)`, three zero padding bytes, exact entire `.bin` identity and plausible vectors. Additional ELF `objdump -h` validation checks **every LOAD LMA** lies within application flash and the highest load end equals `.bin` size; this includes explicit ITCM/DTCM initializer load addresses. The small checksum is format compatibility, not cryptographic integrity; SHA-256 values follow.

Initial MSP **0x20080000**, aligned8; reset **0x080154E1**, Thumb bit set and within application payload, in all four images. Startup remains unchanged and then moves SP to `_dtcm_stack=0x20020000` (`Core/Startup/startup_stm32f765vgtx.s:68`). The header is not a flash address; normal application payload starts **0x08008000**, excluding first32KiB. Hardware write-protection/option-byte state was not inspected or changed.

### Resource comparison

| Region / metric | S4b-1 OFF | S4b-2 OFF | Delta | Capacity | S4b-2 remaining |
|---|---:|---:|---:|---:|---:|
| ITCM | 15,016 | 15,016 | 0 | 16,384 | **1,368** |
| DTCM | 107,392 | 107,392 | 0 | 131,072 | **23,680** |
| General RAM linker usage | 271,600 | 274,640 | **+3,040** | 393,216 | **118,576** |
| Release linker-reported FLASH | 228,256 | 236,584 | **+8,328** | 1,015,808 | 779,224 (region accounting) |
| Release complete flash load span / `.bin` | 280,268 | 288,596 | **+8,328** | 1,015,808 | **727,212** (true image span) |

Diagnostics ON: ITCM15,088 (unchanged;1,296 remaining), DTCM107,392 (unchanged), RAM274,712 versus271,672 (**+3,040**,118,504 remaining). Release-ON linker FLASH236,952 versus228,624 (+8,328), true payload289,036 versus280,708 (+8,328), flash span headroom726,772. Debug adds48 Flash bytes to the matching Release configuration; no RAM/TCM change.

Do not confuse the image validator's FLASH VMA high-water (235,732 ReleaseOFF) with linker-reported FLASH usage (236,584, includes852-byte data allocation) or full LMA span288,596. The full payload-span check is the conservative flash-fit check; all pass.

Fixed additional objects in general SRAM (ReleaseOFF map):

| Object | Address | Bytes |
|---|---|---:|
| Recovery flag | `0x20025ECC` | 1 |
| Aligned validation buffer | `0x20025ED0` | 512 |
| 194×13 filename inventory | `0x200260D0` | 2,522 |

Objects total3,035 bytes; layout/alignment makes measured RAM delta3,040. CRC nibble table16×4=64 bytes is in Flash. There is no heap allocation, whole-project buffer or new ITCM/DTCM section. Global singleton workspace is foreground-only and nonreentrant, consistent with existing FatFs/shared FIL ownership.

### Stack and unchanged object verification

ARM Release source was recompiled with the same production flags plus `-fstack-usage`, comparing baseline e49 header/source with current source. Individual static frames: `preset_saveProject` **104→256 bytes (+152)** (several private functions are inlined); `preset_loadProject`160 unchanged; tmp-kit272 unchanged; song144 unchanged; names72 unchanged. New out-of-line examples: txnInventory144, txnValidate.part.0=136, txnIds104, txnDirectory96, txnSeal88, txnRemove56, txnChoose48, txnRead16. Maximum individual preset translation-unit frame remains272. These are **not** maximum call-chain/interrupt-nesting stack consumption. Existing linker minima heap512 and stack1024 are unchanged; DTCM headroom is not proof of adequate runtime stack. Runtime high-water and FP/exception context remain OPEN.

Seven exact Release object comparisons with recompiled baseline headers/sources passed, byte-for-byte: `Core/Src/stm32f7xx_it.c`, `Core/Src/Soundengine/Soundengine.c`, `Core/Src/Soundengine/cs4344_cs5343.c`, `Core/Src/Soundengine/Sequencer/sequencer.c`, `FATFS/Target/sd_diskio.c`, `Core/Src/sysmem.c`, `Core/Src/safety/runtimeDiagnostics.c`. SHA-256 values are appended. Thus no per-audio-block, DMA, ISR, S1-driver, heap or disabled-diagnostics object work was added. Linked addresses may shift; identical relocatable objects do not prove identical physical runtime timing.

## 10. Live-performance behavior and compatibility

**CODE:** Save/load remains synchronous foreground FatFs work under the existing TIM2 rendering hooks. No I/O enters audio/DMA ISRs; no new locks, interrupt masking, timer sources, buffer sizes, sample rates or priorities. Audio/transport is not deliberately stopped by the transaction engine. Existing nonblocking manual error feedback remains; autosave has no notifications, new polling or new processing hooks. Existing S1/S2 quarantine and S4-L2 safe cooldown behavior are retained.

**OPEN:** Synchronous card commands, directory traversal, f_getfree and full validation can delay foreground MIDI/USB/UI processing; rendering preemption/cache/bus behavior and stack overlap must be measured. Existing display-buffer wait and boot-only delays are unchanged. Software tests do not establish glitch-free audio, worst-case CPU utilization, actual SD latency, stack high-water or thermal performance. No runtime diagnostics export/UI work was added.

Completed projects use only original canonical names and formats, so original firmware can realistically use their payloads subject to separately unproven source/binary behavior compatibility. **During an interrupted transaction** the only complete copy can be under LXRBAK/LXRSTG, which manufacturer firmware does not understand; it may show an empty/missing canonical slot or create files. Do not switch to stock firmware or manually manipulate reserved paths mid-transaction and expect automatic recovery. A cleanly completed Save removes all protocol metadata. Legacy/incomplete projects are not silently migrated on load; replacing them intentionally writes a full current-format project through existing serializers.

Individual named kit/pattern/temp saves still use their existing direct replacement policies; this phase does not make every file save transactional. The transaction validator's full project read is not an atomic RAM snapshot while the musician edits. Project loading still applies files incrementally; a later load failure can leave partial new RAM state, with an honest error. Atomic RAM loading, new user feedback, SD repair and generic recovery architecture were not added.

## 11. Development failures, corrections and limitations

Completed-task failures were diagnosed rather than suppressed:

- Early build warned about comparing a source-folder pointer against a string literal; replaced with an explicit `restoring` boolean before mutation decisions.
- Review found a sole selected stage could otherwise be discarded when a present canonical failed validation. Normalization now refuses when a present canonical differs from the validated selected path.
- Explicit restore now re-reads IDs/source absence after rename; it does not assume a successful API return proves a unique name.
- Mutating-operation/closedir error holds were added to prevent unrelated cleanup flushing a dirty window or bypassing failed ownership.
- Early whole-test sync injection position500 exceeded the actual395 sync calls; fixed the fixture to require reached points including actual final sync. This was a test-position error, not a production success.
- A boundary event used lowercase against FatFs uppercase names; fixed case-insensitive matching, then all required cuts were verified reached.
- Full-card preflight was added to reject ordinary space exhaustion before creating a potentially zero-length record; actual full-media/retry tests pass.
- First-save rescue and failed-recovery tmp-overlay guards were completed and tested. Read-only selection does not silently declare an incomplete first-save stage valid.
- Initial connector blob-manifest read was truncated; no commit/branch was created from it. Upload was retried using exact chunks, verifying every Git blob SHA and complete tree equality.
- Existing host FatFs warning remains: unused `lun` parameter in `ff_gen_drv.c:80`, emitted in configured/read-only fixture builds. Final ARM builds reported no new warnings. No third-party cleanup was made to hide it.

Temporary probes/logs stayed under `/tmp/phase-s4b2` and ignored build directories; they are not repository artifacts. A small initial rename probe preceded the committed complete FatFs16/32 suite; final acceptance relies on the permanent suite. All final required commands succeeded; no unresolved test failure is concealed.

Remaining limitations are intentional and concrete:

1. Reserved aliases/corrupt record/unknown contents can require controlled external inspection rather than in-firmware repair. Never delete an alias with f_unlink as a shortcut.
2. Full card needs room for one extra complete project, including cluster rounding; cache/free-space estimates are not a reservation.
3. Serious write/close faults may require reboot before further storage access under existing policies; reboot alone is not proof of card integrity. RAM/audio editing remains independent as far as existing architecture allows.
4. No guarantee against torn/reordered sectors, card internal caching, corrupt FAT chains, reader faults, power loss at every possible metadata/data-sector write or all geometries. Publication-sector cuts are exhaustive only for the actual two tested cross-sector renames and stated atomic512-byte model. FAT12, other cluster sizes, arbitrary file attributes and manufacturer firmware were not physically tested.
5. CRC and root/count comparison are bounded integrity checks, not authenticity or complete filesystem consistency proofs. External media mutation during a transaction violates exclusive ownership assumptions.
6. The reported PROJ00/iPhone error-36 incident remains an observation with unknown firmware/card correspondence; no actual card image was inspected, repaired or reproduced here. It is not evidence that this code fixes that incident.
7. Old project metadata can load while `/proj.idx` refers elsewhere; recovery only targets the requested conventional slot. It does not auto-select a different user project or scan all slots.

## 12. Acceptance assessment and genuinely necessary next work

| Criterion | Assessment |
|---|---|
| Existing project overwrite restored | **BUILD/HOST: yes**, new +20 repeats on each real FatFs fixture |
| S4b-1 critical bounds/legacy protections retained | **BUILD/HOST: yes**, existing sanitized groups pass |
| Bounded storage / no archives | **CODE/HOST: yes**, singleton directories/record and constant-cluster repeated saves |
| Previous version preserved through staged-write failure | **CODE/HOST: yes**, tested isolated failures and logical cuts |
| Tested publication interruptions recover a complete version | **HOST: yes**, including read-only aliases; not absolute power-loss safety |
| Truthful result including cleanup failure | **CODE/HOST: yes**, no observed failure converted to FR_OK |
| Existing suites and four builds/images | **BUILD: all pass**, no required skip |
| Resource impact | **BUILD:** +8,328 bytes Flash, +3,040 SRAM; ITCM/DTCM unchanged |
| Hardware-ready/approved | **OPEN / NO authorization**; no physical validation or flashing performed |

Recommended next step is a focused source review of ownership/cleanup invariants and the selected error/recovery policy, then preparation of the first controlled hardware validation with explicit authorization, independent project/card backups, a known recovery path and **disposable cloned media**. Measure full Save/load latency, foreground responsiveness, audio/TIM2/DMA peaks and stack high-water under demanding playback, then validate normal repeated replacements and bounded recovery on test media. Do not perform destructive interruption experiments on the original PROJ00 card. No additional general transaction database, generation archive or speculative repair phase is justified by these software results.

A clean review and later hardware approval can proceed independently of optional U1 feedback or Phase D diagnostics export. Those features remain separate; no autosave notifications were added. Remaining individual-file replacement and atomic RAM load limitations must be understood, not silently certified away. This task completes the bounded project replacement implementation under its stated software failure model; it does not certify the entire SD stack or authorize installation.

## Appendix A. Exact final regression output

```text
$ python3 tests/bug001/run.py
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence

EXIT 0
```

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

```text
$ python3 tests/safety/run.py
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state

EXIT 0
```

```text
$ python3 tests/usb_allocator/run.py
PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing
PASS USB failures: 14 rollback/retention/retry scenarios plus invalid requests; no allocator calls
Host object: size=552 alignment=8 (target size/alignment checked separately in ELF)
PASS complete production USB class object: no heap/allocator-lock references

EXIT 0
```

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

```text
$ python3 tests/sd_hal/run.py
PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP
PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap
PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset
PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation

EXIT 0
```

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

```text
$ python3 tests/save_errors/run.py
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical resolved /tmp targets/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, silent controlled background retries; no UI/hardware execution
PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held

EXIT 0
```

```text
$ python3 tests/live_storage/run.py
PASS actual notification/tick/repaint bodies: queued/busy display, 1999/2000ms and wrap, latest error, all result codes, expiry/copy interleaving, page/RAM independence, no storage unlock or blocking calls
PASS complete production FatFs: nine missing-path/invalid-name/existing-file/readonly-target/full-root failures make zero disk writes, preserve all RAM media/valid file bytes, then explicit create/write/close succeeds

EXIT 0
```

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

```text
$ python3 tests/project_storage/run.py
PASS capacity-checked project/SFN paths: all256 indices, exact maximum8.3, old overflowing metadata path and unchanged output on rejection
PASS invalid project/kit consumers and all256 associated-kit values: no OOB, no publication or I/O on rejected public indices
PASS nonrecursive temp cleanup: maximum SFN, owned whitelist, unknown files/subdirectories retained, invalid path causes no unlink, close/unlink errors retained
PASS S4b-1 containment retained: unknown data and wrong CWD protected; new/repeated slot63 saves194 files/255398 bytes without stage/backup accumulation
PASS injected transaction/header/global/early/late child open/write/short-write/close failures: first error, no later payload, no old-project or temp deletion, unsafe replay blocked
Project failure scenarios exercised: 38
PASS short metadata/songs, read/open/close/search errors and absolute name lookup; intentionally incomplete projects get deterministic defaults; old kit/song filenames preserved
PASS required reader closes: kit/pattern/song/temp/global/name failure never reports success or bypasses retained ownership; init defaults bounded to actual payload
PASS all S4b-1 production-body groups under ASan/UBSan (filesystem, HAL, LCD and sound sending are mocked)
PASS whole production FatFs: ten overwrite rejections perform zero RAM-sector writes; previous metadata still readable; absent slot63 creation succeeds without deleting old project
PASS S4b-1 static scope: no recursive deletion or remount/reset; bounded replacement is tested separately; no new filesystem path in ISR sources

EXIT 0
```

```text
$ python3 tests/project_transaction/run.py
PASS whole production project/FatFs FAT16+FAT32: new save +20 replacements each, exact194-file layout, constant free clusters, no stage/backup/record accumulation, valid load and CRC vector
Profile FAT16: 2087 sector writes, 15578 reads, 395 syncs, 587 opens, 586 closes
Profile FAT32: 2497 sector writes, 16393 reads, 395 syncs, 587 opens, 586 closes
PASS 118 whole-FatFs fault/retry cases: writes before/after acceptance, reads/opens/closes/sync, partial writes, bad staged size/kit/unknown contents, rename/cleanup errors, nine safe failures then success; no false success or unsafe replay
PASS 50 logical before/after transaction cuts and repeated interrupted user recovery: complete old/new fingerprint retained; recovery load is read-only
PASS 16 real FAT16/FAT32 cross-sector rename cuts (atomic512-byte writes), 8 alias states detected and held without deletion; complete old/new retained
PASS 36 real-FatFs full-card/retry, unknown/collision/invalid record/index, failed-recovery overlay guard and first-save interruption/rescue cases

EXIT 0
```

## Appendix B. Complete artifact hashes and image measurements

These are hashes of local validation artifacts, not downloadable firmware.

```json
[
  {
    "configuration": "release",
    "payload": 288596,
    "flash_load_span": 288596,
    "flash_capacity": 1015808,
    "flash_remaining": 727212,
    "binary_sha256": "86169ccaa31d74ff997aa5b69f2fcfe6cf61e0aeed29e8b926607ca9b552c7bd",
    "image_sha256": "00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76",
    "elf_sha256": "57758e60214c3865d16ccb86303c4346f7a51c268f2c5b77b286daebe4bf6fed"
  },
  {
    "configuration": "debug",
    "payload": 288644,
    "flash_load_span": 288644,
    "flash_capacity": 1015808,
    "flash_remaining": 727164,
    "binary_sha256": "fee8de2e30e9cc64a7321579a3665019d095e300be076f60c1fae670acd10331",
    "image_sha256": "ae9a61736732ee65d347c796ee28c8fc9ce9ac442d7ad5bc1f1e36612a306fa6",
    "elf_sha256": "a32668d2e00d4ebe5aa10c6a8b84a16ee945ffe8017d001261d1d03777e35d27"
  },
  {
    "configuration": "diagnostics-release",
    "payload": 289036,
    "flash_load_span": 289036,
    "flash_capacity": 1015808,
    "flash_remaining": 726772,
    "binary_sha256": "7ac72b7fdd0dfb34884bc5bc0fd4fbd50e30a0ba170fd3c4242e19b4a7a5a2b1",
    "image_sha256": "c1853483a567f9d41e0e1338564440489f9f1ba7a616c7ff4d8cf8b8f74556d8",
    "elf_sha256": "0305a5f6870e32d6ebdd4054aec019a71c116303cd29e5cc2ddfcc4b74ab80f5"
  },
  {
    "configuration": "diagnostics-debug",
    "payload": 289084,
    "flash_load_span": 289084,
    "flash_capacity": 1015808,
    "flash_remaining": 726724,
    "binary_sha256": "24e9414071a82ffc8f2b7333939543fb1444d48b2a1ea82c659a26bafdf6ec40",
    "image_sha256": "5404c31c128d8d42c9b45b0de80637e6abe17918b998cc4a4602840159accb33",
    "elf_sha256": "84a4477982a7eadec254b7cf8a14430b8a6efeb1259a21c7ca2bc9cb1255841e"
  }
]
```

Header/checksum/vector and section high-water output:

```text
{"checksum": "0xBB", "configuration": "release", "elf_sha256": "57758e60214c3865d16ccb86303c4346f7a51c268f2c5b77b286daebe4bf6fed", "image_bytes": 288612, "image_sha256": "00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 235732, "ITCM_RAM": 15016, "RAM": 274640}, "payload_bytes": 288596, "reset_vector": "0x080154E1", "result": "PASS"}
{"checksum": "0xBD", "configuration": "debug", "elf_sha256": "a32668d2e00d4ebe5aa10c6a8b84a16ee945ffe8017d001261d1d03777e35d27", "image_bytes": 288660, "image_sha256": "ae9a61736732ee65d347c796ee28c8fc9ce9ac442d7ad5bc1f1e36612a306fa6", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 235780, "ITCM_RAM": 15016, "RAM": 274640}, "payload_bytes": 288644, "reset_vector": "0x080154E1", "result": "PASS"}
{"checksum": "0x4D", "configuration": "diagnostics-release", "elf_sha256": "0305a5f6870e32d6ebdd4054aec019a71c116303cd29e5cc2ddfcc4b74ab80f5", "image_bytes": 289052, "image_sha256": "c1853483a567f9d41e0e1338564440489f9f1ba7a616c7ff4d8cf8b8f74556d8", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 236100, "ITCM_RAM": 15088, "RAM": 274712}, "payload_bytes": 289036, "reset_vector": "0x080154E1", "result": "PASS"}
{"checksum": "0xDA", "configuration": "diagnostics-debug", "elf_sha256": "84a4477982a7eadec254b7cf8a14430b8a6efeb1259a21c7ca2bc9cb1255841e", "image_bytes": 289100, "image_sha256": "5404c31c128d8d42c9b45b0de80637e6abe17918b998cc4a4602840159accb33", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 236148, "ITCM_RAM": 15088, "RAM": 274712}, "payload_bytes": 289084, "reset_vector": "0x080154E1", "result": "PASS"}

```

## Appendix C. Unchanged object hashes and static frame comparison

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
        "bytes": 104,
        "kind": "static"
      },
      "current": {
        "bytes": 256,
        "kind": "static"
      }
    },
    "preset_loadProject": {
      "baseline": {
        "bytes": 160,
        "kind": "static"
      },
      "current": {
        "bytes": 160,
        "kind": "static"
      }
    },
    "preset_LoadTmpKit": {
      "baseline": {
        "bytes": 272,
        "kind": "static"
      },
      "current": {
        "bytes": 272,
        "kind": "static"
      }
    },
    "preset_loadSongSD": {
      "baseline": {
        "bytes": 144,
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
      "baseline": null,
      "current": null
    },
    "empty_directory": {
      "baseline": null,
      "current": null
    },
    "preset_loadGlobalFile": {
      "baseline": null,
      "current": null
    },
    "preset_loadProjectNames": {
      "baseline": {
        "bytes": 72,
        "kind": "static"
      },
      "current": {
        "bytes": 72,
        "kind": "static"
      }
    },
    "preset_txnRun": {
      "baseline": null,
      "current": null
    },
    "preset_txnPrepare": {
      "baseline": null,
      "current": null
    },
    "preset_txnChoose": {
      "baseline": null,
      "current": {
        "bytes": 48,
        "kind": "static"
      }
    },
    "preset_txnValidate": {
      "baseline": null,
      "current": null
    },
    "preset_txnInventory": {
      "baseline": null,
      "current": {
        "bytes": 144,
        "kind": "static"
      }
    },
    "preset_txnRead": {
      "baseline": null,
      "current": {
        "bytes": 16,
        "kind": "static"
      }
    },
    "preset_txnSeal": {
      "baseline": null,
      "current": {
        "bytes": 88,
        "kind": "static"
      }
    },
    "preset_txnRemove": {
      "baseline": null,
      "current": {
        "bytes": 56,
        "kind": "static"
      }
    },
    "preset_txnLoadFolder": {
      "baseline": null,
      "current": null
    },
    "preset_txnSpace": {
      "baseline": null,
      "current": null
    },
    "preset_txnMatches": {
      "baseline": null,
      "current": null
    },
    "preset_txnDirectory": {
      "baseline": null,
      "current": {
        "bytes": 96,
        "kind": "static"
      }
    },
    "preset_txnIds": {
      "baseline": null,
      "current": {
        "bytes": 104,
        "kind": "static"
      }
    }
  },
  "maximum_local_frames": {
    "baseline": [
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
        "preset_saveProject",
        {
          "bytes": 256,
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
        "preset_txnInventory",
        {
          "bytes": 144,
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
        "preset_txnValidate.part.0",
        {
          "bytes": 136,
          "kind": "static"
        }
      ]
    ]
  }
}
```

## Appendix D. Reproducible auxiliary binary and stack checks

The established image check command is committed. These additional read-only scripts were run from temporary storage and are reproduced here for audit continuity; they do not write firmware source.

### image_audit.py

```python
from pathlib import Path
import subprocess,re,json,hashlib,struct
root=Path('/workspace/lxr-desktop-app');p=Path('/tmp/phase-s4b2')
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

### object_audit.py

```python
from pathlib import Path
import subprocess,shlex,json,hashlib
root=Path('/workspace/lxr-desktop-app');work=Path('/tmp/phase-s4b2/objects');work.mkdir(exist_ok=True)
base='e49bb028462c339c4998cd08a845db17e1fc778f'
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
    'preset_clearTmpFolder','delete_node','empty_directory','preset_loadGlobalFile','preset_loadProjectNames','preset_txnRun','preset_txnPrepare','preset_txnChoose','preset_txnValidate','preset_txnInventory','preset_txnRead','preset_txnSeal','preset_txnRemove','preset_txnLoadFolder','preset_txnSpace','preset_txnMatches','preset_txnDirectory','preset_txnIds']},
    maximum_local_frames={label:sorted(fs.items(),key=lambda x:x[1]['bytes'],reverse=True)[:8] for label,fs in frames.items()})
(work.parent/'object-stack-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))

```

## Appendix E. Publication verification

The source GitHub ref and fetched Git tree were verified at the exact commit/tree in section2. The report is published independently from lab-setup; the publisher verifies its exact URL, blob contents, one-file commit and both branch heads after creation. Resolve this report's containing commit through GitHub History or the final handoff; embedding a future self-commit hash in its own content would be circular. No pre-existing report is overwritten. No physical firmware/card operation, flashing, option-byte change, bootloader write, merge or binary publication occurred.
