# LXR Firmware Lab — Phase S4a: Safe File Saving and Error Propagation

Date: 2026-10-26 (UTC). Repository: `DocLXR-02/lxr-desktop-app`.

**Status: IMPLEMENTED within S4a's individual-save scope. Development review only. NOT FLASHED — NOT HARDWARE VALIDATED. Complete project saving remains unsafe and is explicitly deferred to S4b.**

## 1. Identity, authorization and reproducible source

- Authorized and remotely verified baseline: `safety/sd-fatfs-integrity-s3` at `cbffa2d756658b6f32d8e6417ea40ec2cda828d5`.
- New source branch: `safety/sd-save-errors-s4a`.
- Final source commit: `c87666ff945e2e01cbbf1ceda72fb503638de579`.
- Source tree: `eb1f709ff7acc21ed64739bcb0da916fd3cecf2f`; sole parent is the exact S3 baseline. No merge commit.
- Source URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/safety/sd-save-errors-s4a
- Report-only branch: `codex-reports/2026-10-26-phase-s4a-safe-file-saving`, based on `lab-setup` at `5829398cea5d337200393757e39a37d4d8148256`.
- Report path: `docs/codex-berichte/2026-10-26_phase-s4a-safe-file-saving.md`.
- Report URL: https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-s4a-safe-file-saving/docs/codex-berichte/2026-10-26_phase-s4a-safe-file-saving.md

The existing `/workspace/lxr-desktop-app` checkout was used; it was clean before branching. No additional worktree was created. The complete `AGENTS.md` from `lab-setup` was read (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`). The full D0, S1, S2 and S3 reports were read before implementation. They are separate report-branch documents, not files automatically present on the source branch.

Reference report branches and heads:

| Report | Branch | Commit |
|---|---|---|
| D0 | `codex-reports/2026-10-26-phase-d0-sd-storage-safety` | `a80a37e0704cc5f178a36878f1f22a155cc6f21e` |
| S1 | `codex-reports/2026-10-26-phase-s1-sd-storage-foundation` | `e66f31b979455d977993ad21583a4ff191ea6cf6` |
| S2 | `codex-reports/2026-10-26-phase-s2-sd-hal-bsp-timeouts` | `bebc12701234e6b2f622449c72e291c6f0957f7f` |
| S3 | `codex-reports/2026-10-26-phase-s3-fatfs-data-integrity` | `b68238fe315c3f526aa4677d94b93d9c0d262aac` |

Pre-existing source heads were recorded and preserved: main `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`; lab-setup `5829398cea5d337200393757e39a37d4d8148256`; Bug001 `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`; Euclidean `67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72`; Phase A `e19bb0544dcc8f07138a9857c78022e8efaecd81`; Phase B `e4c0fd3ca66ced53413d5e1733e7ac9aa1c0da6e`; Phase C `091f502de6f57f3262ffc68659eafac6f8bdfa19`; S1 `a4be3b94c552dbc867b197fad76655bed40cea34`; S2 `c88933a4b1968c8f9d173b71d86154e8969436be`; S3 as above. D and D0 placeholder development branches remain at Phase C. All 25 pre-existing remote heads were compared before publication; final verification is recorded below.

Evidence labels: **CODE** means inspected production source; **BUILD** means executed host/build/binary checks; **HYPOTHESIS** means plausible runtime consequence; **OPEN** means unmeasured or reserved for follow-up.

## 2. Objective and exact boundary

S4a corrects observed file-open/write/short-write/close failures in individual temporary kits, named kits (including the existing Morph serialization variant), temporary patterns, named patterns, songs, globals and the small last-used-project index. It propagates errors into direct individual-save UI actions and foreground autosave handling. Successful filenames, file formats, byte order and serialized payloads are preserved.

It does not implement transactional replacement, new filenames, new disk layouts, recovery/remount/reset, rollback, backup deletion, full project-directory lifecycle repair, diagnostic capture/UI/export, or new MIDI/sound parameters. Existing S1/S2 fault handling, S3 synchronization, USB fixed allocation, bounded heap and optional runtime diagnostics remain unchanged. No production HAL/BSP/FatFs-core/build-configuration file was edited.

## 3. Confirmed defects and call graph

**CODE:** S3's `preset_saveTmpKit()` checked only the first write, ignored the main payload result/count and close, and returned `FR_OK`. Pattern writers ignored many writes, helper functions returned `void`, several header writes checked only `FRESULT`, and early failures could leave the shared `FIL` open. Globals ignored helper/close errors; songs did not check short counts. Some menu and sequencer-button individual-save actions discarded the result before setting success state. Foreground temporary-save service ignored errors and could continue to a second save.

Relevant current call graph:

```text
main foreground loop
  menu_parseEncoder -> menu_handleLoadSaveMenu
    kit -> RAM copy -> preset_saveDrumsetSD -> checked name + drumset data -> finish
    pattern -> RAM copy -> preset_savePatternSD -> name + PatV03 + pattern data -> finish
    globals -> preset_saveGlobals -> global data -> finish
  dio_process -> buttonHandler_seqButtonPressed
    pattern-save branch -> RAM copy -> preset_savePatternSD -> checked result
  preset_processTimer
    due pattern -> preset_saveTmpPat -> selected pattern + common pattern data -> finish
    if still successful, due kit -> preset_saveTmpKit -> selected kit + active data -> finish

preset_saveProject [directory lifecycle reserved S4b]
  project header [still separately deferred] -> globals -> 64 * (kit + pattern + song)
preset_loadProject -> preset_saveLastUsedProjectNrToSD -> one checked byte -> finish
preset_createGlobals -> defaults -> preset_saveGlobals -> existing error display on failure
```

The song menu case has no individual SD save action; the active song writer is called by project saving. No new song-save UI was invented. Commented-out autosave-on-page-switch and legacy disabled save implementations were left disabled.

## 4. Shared-file safety contract

`FATFS/App/fatfs.c:24` owns the global `FIL SDFile`. The application is foreground-driven; no new ISR filesystem operation was added. A reusable wrapper was necessary because merely fixing individual writers would still allow a later load or filename scan to overwrite the same unresolved object or flush cached metadata.

**CODE — contract:**

1. Only open the shared object when there is no application failure latch and `SDFile.obj.fs` is null. An already-owned object returns `FR_LOCKED` without calling `f_open`.
2. A failed write-mode open, any write error, any short write, or any failed close latches the first application storage failure until reboot. Serious read-open `FR_DISK_ERR`/`FR_INT_ERR` also latch. Ordinary missing read files/paths remain usable for the existing legacy-name fallback.
3. `FR_OK` plus fewer bytes than requested becomes `FR_DISK_ERR`. An actual non-OK FatFs result wins even if a mock/lower layer reports a full count.
4. Stop payload writes immediately on first error. Never close an incomplete write in this policy. Never zero/reuse the object, manually clear `FIL.err`, force-release its lock, automatically retry, remount, reset, or delete backups to recover it.
5. A fully written file is closed exactly once. A failed close is returned and latched; the object is retained as FatFs leaves it.
6. Subsequent guarded saves, loads and relevant directory/lifecycle entry points reject or return before using the shared object. Repeated background autosaves stop and do not repeatedly attempt I/O.

The application latch complements S1's disk latch. A short write from a full volume can return `FR_OK` with a zero `FIL.err` and no disk-layer fault. Relying only on S1 would permit later close to flush buffered payload, FAT or directory data. S4a chooses conservative retention instead. There is no automatic rollback of already written bytes, and a truncated or partially created file may remain on disk. This is deliberate failure containment, not transactional persistence.

Even a known write-open error such as `FR_NO_PATH` quarantines subsequent saves and cleanup. This is stricter than necessary for some errors, but prevents ignored write-open failures in existing project callers from continuing with later writes or temporary cleanup. It may reduce hotplug/path-error convenience and requires reboot before further shared-file activity. Reboot is a user recovery boundary, not an automatic reset or guarantee of filesystem repair. Unsaved RAM changes can be lost at power-off.

Decisive code at `Core/Src/Preset/presetManager.c:91` (also reproduced in the production diff appendix):

```c
static FRESULT preset_sharedFailure = FR_OK;
static FRESULT preset_sharedStatus(void)
{
    if(preset_sharedFailure != FR_OK) return preset_sharedFailure;
    return SDFile.obj.fs ? FR_LOCKED : FR_OK;
}
static FRESULT preset_openShared(FIL *file, const TCHAR *name, BYTE mode)
{
    FRESULT res = preset_sharedStatus();
    if(res != FR_OK) return res;
    res = f_open(file, name, mode);
    if(res != FR_OK && ((mode & FA_WRITE) || res == FR_DISK_ERR || res == FR_INT_ERR))
        preset_sharedFailure = res;
    return res;
}
static FRESULT preset_writeChecked(const void *data, UINT count)
{
    UINT written = 0;
    FRESULT res;
    if(preset_sharedFailure != FR_OK) return preset_sharedFailure;
    res = f_write((FIL*)&SDFile, data, count, &written);
    if(res == FR_OK && written != count) res = FR_DISK_ERR;
    if(res != FR_OK) preset_sharedFailure = res;
    return res;
}
static FRESULT preset_finishSave(FRESULT res)
{
    return res == FR_OK ? preset_closeShared((FIL*)&SDFile) : res;
}
```

`preset_closeShared()` at line105 refuses a latched failure, rejects an unopened object, calls `f_close` once for a healthy owned object and latches any close failure. S3's `ff.c:3736` `f_sync` propagates sticky file errors and synchronization errors; `ff.c:3820` `f_close` only invalidates/decrements ownership after successful sync/validation. S4a does not modify those rules. No redundant extra `f_sync` is added before `f_close`.

All enabled opens/closes in `presetManager.c`, including read paths, use the wrappers. Guards were added before filename directory scanning, temp-folder initialization/clear, temp-folder loads, project-name enumeration, and project create/save/load entry points. These are necessary ownership preconditions; the existing successful project order and destructive directory algorithm were not redesigned. Some read-only loaders still ignore their close result when computing their own return value: ownership is contained, but comprehensive read-result propagation is not claimed.

## 5. Corrected functions and callers

Line references below refer to the final source commit, not the baseline.

| Source/function | Correction |
|---|---|
| `presetManager.c:241` `preset_saveTmpKit` | Checked selected-kit byte, full active parameter payload and close; existing TIM2 start/stop pairing retained on every exit. |
| `:511` `preset_saveTmpPat` | Reject invalid pattern; checked selected-pattern byte; common checked pattern serializer replaces duplicate unchecked serializer; checked close and existing timer pairing. |
| `:733` `preset_writeDrumsetData` | `void` to `FRESULT`; each byte checked; existing Morph interpolation/exempt-target decisions untouched. |
| `:836` `preset_saveDrumsetSD` | Validate slot; checked eight-byte name and serializer; checked close. |
| `:854`, `:867` global writer/save | `FRESULT` serializer, same per-byte order; checked open/payload/close. |
| `:985` `preset_createGlobals` | Public void API retained; displays actual save error rather than silently discarding it. |
| `:1016` project-index save | Checked one-byte payload and close, no close after unsuccessful open. |
| `:1485` pattern writer | `FRESULT`, stop at first failed track/timing/main-step/shuffle/length/kit write. |
| `:1575` named pattern save | Validate slot; check eight-byte name, six-byte `PatV03`, data, close; repaint only after complete success. |
| `:2484` song save | Validate slot; check eight-byte name, 64-byte song and close. |
| `:2757` load-project index caller | Return the index-save error if no earlier error exists; preserve an earlier failure. |
| `:601` timer service | Check both temporary-save results; do not start kit after failed pattern; clear pending timers on error, show one foreground error; suppress retries after quarantine. |
| `menu/menu.c:2038` onward | Actual pattern/kit/global save cases check errors before success-only reset, selected/loaded persistence state, success page/timer/repaint. |
| `buttonHandler.c:1153` onward | Actual pattern button-save branch checks RAM and disk results before persistence success page/selection/timer. |

Existing RAM editing/name changes and pattern-mode flags that precede persistence remain as before; no rollback of user edits or unrelated sequencer behavior was added. Only persistence-success state is gated. `menu_showSdError` is reused. It has an existing blocking display wait/delay and existing timer hooks, so a failure notification can delay foreground MIDI/UI servicing. It is not newly called from an interrupt. Main foreground locations are `main.c:968` encoder handling, `:979` DIO processing and `:992` temporary-save service. No real-time performance claim is made.

## 6. Successful serialization and data compatibility

**BUILD:** 1,536 direct byte comparisons against frozen actual S3 save bodies passed: eight variants times all 64 slots times Morph0/127/255. The variants are temp kit, named normal kit, named Morph kit, temp pattern, named pattern, song, globals and index. The same deterministic varied input data and actual unchanged naming/interpolation helpers are used for both sides. Tests compare all output bytes, total lengths, filenames and write-call counts, not merely a hash or a new implementation's own expectation. Existing timer calls balance and a successful file closes once.

The committed oracle `tests/save_errors/baseline_s3.inc` contains the ten actual S3 helper/entry bodies and retained license; its SHA-256 is pinned in `baseline_s3.sha256`. `--verify-baseline` reconstructs it from the exact S3 Git object and confirms equality. The default runner needs no fetched Git history. `common_s3.json` pins the unchanged filename sanitizer, numeric formatter, name builder, interpolation and Morph helper bodies. This prevents the oracle and current code from silently sharing a changed common dependency.

Pattern ordering remains: for each of seven tracks, 64 serialized `Step` elements then 64 timing bytes; then per-track/per-bar main-step uint16 values; shuffle; seven literal length bytes; associated kit byte. Named patterns prepend eight name bytes and `PatV03`; temp patterns prepend only the selected-pattern byte, exactly as S3. Kit name/payload and globals/index layouts are unchanged. There is no new padding/header/version or migration. The separate Euclidean tests also pass 130 complete named/autosave roundtrips and unchanged-layout checks.

The host oracle uses production headers on the native compiler. This verifies the tested source serialization and byte-oriented structures, not every possible ABI/platform or arbitrary input. ARM builds compile complete production translation units. No official v1.71 binary or physical user file was examined here, and compatibility with the manufacturer's binary is not established by these tests.

## 7. Regression harness and exact coverage

New reproducible suite: `python3 tests/save_errors/run.py` (also run with `--verify-baseline`). It extracts actual production functions and the actual relevant menu cases/button branch, compiles with production headers and fatal ASan/UBSan, and executes a native C fixture. Full firmware builds separately compile the complete translation units. UI RAM-editor calls, display notifications, timers, directory calls and FatFs open/write/close are mocks. It is not a whole physical UI, card, filesystem or audio test.

**BUILD — 2,544 injected payload positions** cover every write call of all eight variants, with four modes at each position: `FR_DENIED` with zero count, `FR_DENIED` with full count, `FR_OK` short count, and `FR_DISK_ERR`. Every case asserts no subsequent payload write, no close, preserved error/object, balanced timer hooks and no later shared-save/read-open/close/directory cleanup. A different configured close error verifies that cleanup cannot replace the first error. Additional cases cover write-open `FR_NO_PATH` and `FR_DISK_ERR`, failed close after complete payload, invalid indices, already-owned object, and no automatic retry or owner/err clearing.

Actual menu cases and pattern-button bodies are exercised with success, short write, close failure and relevant RAM-copy failure. Assertions cover existing success state and its absence on failure. Background tests cover both due saves, first-pattern failure preventing kit, kit failure, path-open failure, repeated service calls after quarantine and one notification. Default-global creation error reporting is tested. The whole state machine and LCD wait are not executed; notification functions are mocked.

A configured ARM preprocessor scan maps calls to original source lines: enabled `f_open`/`f_close` calls exist only in ownership wrappers; raw `f_write` remains only in the checked helper and the explicitly deferred project header. Static checks establish preflight ownership guards before project lifecycle entry work. They do not execute destructive project saving.

Existing Euclidean test seams were updated for the new helper dependencies and realistic `FIL.obj.fs` open/close ownership. No previous assertion was removed or weakened. Simulated reboot fixture reset clears the new application failure flag only in host code; there is no firmware reset/clear API.

Native sanitizers: `-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`; process deadline40s for the new suite; `ASAN_OPTIONS=detect_leaks=0` because executor tracing prevents LeakSanitizer. ASan and UBSan remain active/fatal. LeakSanitizer is not claimed to have run. No relevant test was silently skipped.

| Command | Result and evidence |
|---|---|
| `python3 tests/save_errors/run.py --verify-baseline` | PASS; oracle verified; 1536 equivalence,2544 injected positions, ownership/preprocessor/UI/background assertions. |
| `python3 tests/bug001/run.py` | PASS; CC0/bank/CC2/NRPN/Drive/mutes, all256 Morph positions, sweep/restart/audio-service cadence. Host scenarios only. |
| `python3 tests/euclidean/run.py` | PASS six ASan/UBSan groups;2176 masks,294912 playback ticks,4160 recording positions,130 roundtrips,10210 malformed cases,180802 protocol boundary calls. |
| `python3 tests/safety/run.py` | PASS bounded `_sbrk` including601 moves; diagnostics OFF argument non-evaluation/single HAL call; ON wrap/saturation/budget/DWT/DMA. |
| `python3 tests/usb_allocator/run.py` | PASS2000 FS/HS lifecycle cycles,14 failure scenarios, full zeroing/ownership; no heap references in complete production class object. |
| `python3 tests/sd_storage/run.py` | PASS1..128-sector transfers,5s arguments, readiness/wrap/fault latch, status/init/sync, invalid contexts/ranges, no reset/remount/retry. |
| `python3 tests/sd_hal/run.py` | PASS actual extracted command/HAL/BSP paths: bounded waits/STOP/wrap/pre-tick/error propagation/read/write and no replay/reset. |
| `python3 tests/fatfs_integrity/run.py` | PASS configured and read-only groups, complete production FatFs/disk/S1 units, mirror/FSInfo/sticky sync+close/init/mount/read paths. |

Every listed command exited0. Full captured suite output is in the appendices. No physical SD/MMC/HAL electrical behavior, card quiescence, power-loss recovery, CPU load, audio glitches, stack high-water or actual allocation peaks were measured.

## 8. Toolchain, firmware builds and image validation

Each shell used:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

CMake3.31.10; Ninja1.13.0.git.kitware.jobserver-pipe-1; Python3.12.14; ARM GCC14.2.1 20241119 (`15:14.2.rel1-1`); native GCC14.2.0. These match the prepared S3 environment. Release uses the established optimization settings; the Debug preset explicitly uses `-O3 -g3` with DEBUG, not an unoptimized Debug assumption.

Commands executed successfully:

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
git -c core.whitespace=cr-at-eol diff --check
```

All configure/build/image commands exited0. Final builds were repeated after the last indentation-only cleanup. Cache values confirm diagnostics OFF for release/debug, ON for the two isolated directories. `LXR_RUNTIME_DIAGNOSTICS` remains OFF by default in unmodified CMakeLists.txt:72. No full-flash target or bootloader was configured. Existing build output prints generic update instructions; those were not followed and are not deployment authorization.

Source version remains1.7. Actual generated names are `LXR_V2_V1.7.img`, `LXR_V2_STM32F765.bin`, `LXR_V2_STM32F765.elf` under each build directory. They remain local and were not committed/uploaded. Images were built from the S3 working-tree overlay whose exact bytes were subsequently committed as `c87666ff945e2e01cbbf1ceda72fb503638de579`; tree/blob equality was verified, rather than claiming HEAD already had that commit during the build.

**BUILD:** All four images passed16-byte header (`<8sIB3s`), `LXRV2IMG`, LE payload length, zero padding, checksum `0xff - (sum(payload) & 0xff)`, exact `.bin` byte identity, load-address/memory limits and plausible vectors. Initial MSP is `0x20080000`; reset vector is `0x08014E4D` (Thumb), unchanged from S3. Application origin is `0x08008000`; first32KiB excluded from normal application image. Bootloader/physical write-protection state is not established by a cloud image.

| Configuration | Payload bytes | Image bytes | Checksum | Payload Flash headroom |
|---|---:|---:|---|---:|
| Release OFF |277436|277452|0x8B|738372|
| Debug OFF |277484|277500|0x20|738324|
| Release ON |277876|277892|0x07|737932|
| Debug ON |277924|277940|0xDF|737884|

SHA-256 values of all final images/ELFs are in the validation appendix; binary SHA-256 values are appended separately. The image validator checks loaded ELF sections lie in the matching payload and each allocated VMA section fits one configured region. It is a structural check, not an execution or updater/hardware qualification.

## 9. Resource impact and unchanged real-time boundaries

| Quantity, bytes | S3 Release OFF | S4a Release OFF | S3 Debug OFF | S4a Debug OFF | S3 Release ON | S4a Release ON | S3 Debug ON | S4a Debug ON |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| ITCM |15016|15016|15016|15016|15088|15088|15088|15088|
| DTCM |107392|107392|107392|107392|107392|107392|107392|107392|
| General SRAM |271576|271576|271576|271576|271648|271648|271648|271648|
| Linker FLASH used |223632|225424|223680|225472|224000|225792|224048|225840|
| Complete application payload |275644|277436|275692|277484|276084|277876|276132|277924|

**BUILD:** +1792bytes Flash/payload in every configuration; no aggregate ITCM/DTCM/SRAM change. The new failure-state symbol occupies one byte in the actual Release ELF (`0x20025ec4`, `nm -S`), accommodated without increasing the aggregate RAM region use. Source uses `FRESULT`; do not infer a universal one-byte C enum ABI from this particular compiler result. No complete pattern/project RAM copy, heap allocation, new static payload/export buffer or VLA was introduced.

Capacities/headroom: ITCM16384 leaves1368 normal /1296 ON; DTCM131072 leaves23680 before runtime stack; general RAM393216 leaves121640 normal /121568 ON. Application Flash capacity1015808. Linker FLASH used differs from full load/payload span because initialized ITCM/DTCM/data also occupy Flash load storage. Validator FLASH VMA high-water224572/224620/224940/224988 is another metric; do not confuse any of these with complete application size.

Linker minimum heap0x200 and stack0x400 reservations are unchanged; existing startup relocates runtime stack to DTCM. These reservations do not prove worst-case runtime stack capacity. S4a adds only small scalar locals/filename arrays in foreground call paths and helper calls; no dedicated worst-case stack analysis or runtime stack measurement was performed. Existing display/formatting/filesystem execution and interrupt nesting still require later hardware observation.

All12 complete audio/codec/IRQ object hashes (three objects times four configurations) match captured S3 hashes, including after final builds: `Soundengine.c.obj`, `cs4344_cs5343.c.obj`, `stm32f7xx_it.c.obj`. Object comparison output is appended. DSP, Morph, Euclidean, MIDI/NRPN, USB allocation, clocks, startup, linker, interrupt priorities, DMA buffers/routing, bootloader and option-byte source were not changed. There is no new interrupt source, mask, allocator, lock or file-I/O interrupt path. Existing storage can still stall foreground UI/MIDI while audio relies on existing service architecture; no glitch-free claim is justified.

## 10. Development errors and limitations

- Initial new background test failed because the fixture retained `menu_playedPattern=255` from an invalid-input test. The test-only simulated-reboot reset now restores pattern0; production was not changed to hide the failure. Full sanitized suite then passed repeatedly.
- Initial whitespace check found mixed tab/space indentation on modified interposed lines; corrected without logic changes and rechecked. Existing CRLF and legacy non-UTF8 production comments were preserved; source serialization/oracle extraction normalizes line endings only in the host harness.
- An attempted log read used a nonexistent21.log; the actual complete pipeline ended at20.log. This was an inspection path mistake, not a test or firmware failure. Final logs are separately captured.
- GitHub verification through a percent-encoded branches URL did not yield a usable JSON result; verification used the supported exact Git refs/tree endpoints instead. Source publication and content verification did succeed.
- The existing FatFs registration test emits an unchanged native `unused parameter 'lun'` warning. It exits0; no assertion/sanitizer was disabled to silence it. Firmware builds have no newly reported compiler warning.
- Host mocks cannot establish physical write duration/quiescence, partial electrical-transfer outcomes, SD-card durability, real LCD error behavior or audio continuity. Native assertions and complete ARM compilation complement each other; neither is physical validation.
- Tests do not execute every full project save/load or formatter state machine. Static preflight guards and extracted UI cases are explicitly distinguished from full-system execution.

## 11. Remaining false-success and destructive project risks: exact S4b handoff

The following are **CODE findings in the final source**, not claims that they caused a physical failure. They remain material and prevent a complete storage-safety/deployment claim.

1. `preset_saveProject:2177` preflight now blocks an already-failed/owned file, but normal existing-directory saving still calls `empty_directory(currentFolder)` at2238 **before replacement files are complete**. A later failure can leave the old project destroyed and the new project incomplete. No transaction/rollback exists.
2. Its `f_stat` branching treats every result other than `FR_NO_FILE` as the existing-directory path, including other errors. Successful `f_mkdir` is followed by an unchecked `f_chdir`. Project index/range/path ownership needs explicit S4b review.
3. `empty_directory:631` recursively deletes entries; directory-close result is ignored. This pre-existing deletion path was not redesigned, extended or executed against a physical card here.
4. The project-header write at2289 remains raw `f_write` of eight `prj.nfo` bytes with **unchecked byte count**. A short `FR_OK` header followed by a successful close can still evade detection, even if every individual helper succeeds. A returned error exits with an unresolved object; new guards prevent its reuse. S4b must use checked writing without silently assuming the full project is transactional.
5. Project-header `preset_closeShared` at2303 has an ignored return. The wrapper quarantines it, so subsequent checked writers reject use, but explicit first-result handling remains missing and earlier destination deletion is irreversible.
6. `preset_saveGlobals` at2307 is ignored by the project caller. Its new quarantine usually causes the next checked kit save to return the same failure before further physical writes, but explicit caller propagation is still required. Named kit/pattern/song results in the loop are already checked.
7. Successful project completion calls void `preset_initTmpFolder` at2354 and ignores `preset_clearTmpFolder` at2361 before returning `FR_OK`. A short project header can therefore coexist with deletion of temporary recovery files. S4a adds no new deletion and does not certify this path.
8. `preset_createProject:2127` resets RAM/defaults and ignores temp-clear result at2168, then returns `FR_OK`. This complete-project lifecycle is reserved for S4b.
9. `preset_loadProject:2542` can ignore `preset_clearTmpFolder` at2561 **before** replacement loading. It also has existing mkdir/chdir/child-load/close-result handling gaps. Only its directly related index-save result was corrected here. Do not infer complete project-load integrity.
10. `preset_initTmpFolder:130` remains void and does not fully propagate mkdir/chdir/stat errors; S4a only adds ownership preflight. Project/temp recovery state ownership needs coordinated S4b treatment.
11. The existing foreground card-insertion path `main.c:925` calls `MX_FATFS_ReInit`, unmounts, clears `SDFatFS`, remounts and enumerates projects. It was not changed. The application failure flag persists and blocks shared file reuse, but broader volume/lock lifetime after such remounts remains an explicit integration concern. S1's disk fault latch is not cleared by that path.
12. Some read-only loaders ignore failed close in their own result; wrapper quarantine now prevents silent object reuse, but read-result truthfulness remains follow-up work.
13. Existing explicit card-format/startup flows in `main.c` are untouched. The format flow calls void `preset_createGlobals` and `preset_saveProject` and can later display its own success despite earlier save errors. S4a's corrected individual global save reports/display errors but does not certify that destructive format/project caller. No format was run; no new automatic formatting was introduced.

**Recommended S4b boundaries:** first define ownership and failure contract for create/save/load/cleanup/remount and all first-error returns; correct project header/count/close and global result handling; test every stat/mkdir/chdir/enumeration/unlink/close failure and every project child failure. Prohibit deleting old destination or temp recovery contents after any unsuccessful/incomplete save. Separately review a non-destructive project replacement design before implementing directory transactions/renames, considering FatFs limits, power loss, available space and S1 reboot-only faults. Do not add speculative recovery writes or force-release unresolved files. A staged design review is needed before changing folder lifecycle. Preserve successful formats and all existing hardware boundaries.

## 12. Final assessment and next step

- **CODE/BUILD:** Corrected individual save paths return failure after observed incomplete/short writes or failed close; no later payload writes/close/reuse follows their quarantined failure in tested paths. Success UI is gated and background retries are suppressed.
- **BUILD:** Eight suites, four ARM builds and four image validations passed. Serialization byte comparisons passed. Static memory fits every configured region; ITCM reserve remains small.
- **CODE:** S1/S2/S3 source is unchanged and their regression suites pass together with S4a. No bootloader/startup/clock/linker/audio/USB changes or new recovery/reset behavior.
- **HYPOTHESIS:** Quarantine reduces the chance of compounding uncertain writes; no actual SD corruption or audio timing effect was observed in this cloud task.
- **OPEN:** Physical card/peripheral state after timeout, real write/close latency and durability, stack/CPU peaks, power-loss consistency, full project replacement and remount/file-handle ownership. S4b is required before any claim of complete storage reliability.

Recommended next action: review this source diff and conservative reboot-only policy, then authorize a narrowly scoped S4b design and tests for non-destructive project saving and shared recovery-state ownership. Do not resume the diagnostic SD exporter or physical deployment based solely on S4a. Hardware testing/flashing requires Daniel's separate explicit approval, backups and recovery plan. No firmware was flashed, physical SD card accessed, hardware operated, binary published, branch merged, protection changed or automatic reset added.

## 13. Publication and verification record

Source was published with GitHub Git-object APIs (base64 blobs preserve original production bytes), a tree based on S3, a single-parent commit and a new branch ref. This is the authorized commit/push operation via the connected API, not a merge. Every11 changed blob SHA was compared with the local manifest; the exact remote tree and parent were checked. The published commit was imported into the existing checkout with matching Git tree and a clean working tree. No pre-existing source branch was moved.

The report is published independently from `lab-setup`; its own commit SHA can be read from the verified branch head (a report cannot embed its own Git commit hash without changing it). Final remote ref/content verification details follow in the generated appendices. No generated artifact or build directory is in either published tree diff.


## Appendix A — Complete changed-file list and Git diff

Source diff:11 files,963 insertions,341 deletions. Three production files; two existing Euclidean fixture seams; six new focused regression files. The frozen oracle is test-only.

- `Core/Src/Preset/presetManager.c` — blob `90a404644f5595c9d7e9b5f1a4e6ff2760ee36db`.
- `Core/Src/buttonHandler.c` — blob `a3affbde9b2a2b4998272826bd92b1595f496b31`.
- `Core/Src/menu/menu.c` — blob `c6b45d20bd5f58df8bc8581953961dce8ad6613a`.
- `tests/euclidean/host.c` — blob `16ac85dc699f3a6bebfc9ca9c889101151a528ed`.
- `tests/euclidean/run.py` — blob `5670831b45d4ff5757e17c73dd852587b2975f98`.
- `tests/save_errors/README.md` — blob `68a2fd0c9896bcdf17f31cfd75c64e5a816ead08`.
- `tests/save_errors/baseline_s3.inc` — blob `6bcd60912440d5973bda2c824d63fae9336909d5`.
- `tests/save_errors/baseline_s3.sha256` — blob `fe4660c8b411da28edc858e262e6c78ae27cc945`.
- `tests/save_errors/common_s3.json` — blob `bb6ca21481b96f1d2cb8783aba0c0ddb2db6a6c8`.
- `tests/save_errors/host.c` — blob `2696be7950c9d3c516b925b68982dc2dd90bdab2`.
- `tests/save_errors/run.py` — blob `9a900a39e881b85fbf13908419ee8d0176f7f372`.

Production and existing-fixture diff (new test files are committed at the exact blobs above):

```diff
diff --git a/Core/Src/Preset/presetManager.c b/Core/Src/Preset/presetManager.c
index bffe86e..90a4046 100644
--- a/Core/Src/Preset/presetManager.c
+++ b/Core/Src/Preset/presetManager.c
@@ -84,8 +84,52 @@ uint32_t preset_patTimer = 0;
 uint8_t preset_kitDirty = 0;
 uint8_t preset_patDirty = 0;
 
+/* One shared FIL: failed/short writes and failed closes are quarantined until reboot.
+ * Never clear FIL.err or close after an incomplete write: close could flush more data.
+ */
+static FRESULT preset_sharedFailure = FR_OK;
+static FRESULT preset_sharedStatus(void)
+{
+    if(preset_sharedFailure != FR_OK) return preset_sharedFailure;
+    return SDFile.obj.fs ? FR_LOCKED : FR_OK;
+}
+static FRESULT preset_openShared(FIL *file, const TCHAR *name, BYTE mode)
+{
+    FRESULT res = preset_sharedStatus();
+    if(res != FR_OK) return res;
+    res = f_open(file, name, mode);
+    if(res != FR_OK && ((mode & FA_WRITE) || res == FR_DISK_ERR || res == FR_INT_ERR))
+        preset_sharedFailure = res;
+    return res;
+}
+static FRESULT preset_closeShared(FIL *file)
+{
+    FRESULT res;
+    if(preset_sharedFailure != FR_OK) return preset_sharedFailure;
+    if(!file->obj.fs) return FR_INVALID_OBJECT;
+    res = f_close(file);
+    if(res != FR_OK) preset_sharedFailure = res;
+    return res;
+}
+static FRESULT preset_writeChecked(const void *data, UINT count)
+{
+    UINT written = 0;
+    FRESULT res;
+    if(preset_sharedFailure != FR_OK) return preset_sharedFailure;
+    res = f_write((FIL*)&SDFile, data, count, &written);
+    if(res == FR_OK && written != count) res = FR_DISK_ERR;
+    if(res != FR_OK) preset_sharedFailure = res;
+    return res;
+}
+static FRESULT preset_finishSave(FRESULT res)
+{
+    return res == FR_OK ? preset_closeShared((FIL*)&SDFile) : res;
+}
+static FRESULT preset_writePatternData(uint8_t patternNr);
+
 void preset_initTmpFolder()
 {
+    if(preset_sharedStatus() != FR_OK) return;
 	char folderName[13];
 	sprintf(folderName, "/tmp");
 
@@ -115,6 +159,8 @@ void preset_resetAllSaveTimers()
 
 FRESULT loadTmpFolderFiles()
 {
+    FRESULT shared = preset_sharedStatus();
+    if(shared != FR_OK) return shared;
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
 	HAL_TIM_Base_Start_IT(&htim2);	//calc sample interrupt - prio 1
 #else
@@ -132,7 +178,7 @@ FRESULT loadTmpFolderFiles()
 		{
 			lcd_process_display_interrupt = 1;
 			preset_initTmpFolder();
-			f_close((FIL*)&SDFile);
+			preset_closeShared((FIL*)&SDFile);
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
 			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
 #else
@@ -143,7 +189,7 @@ FRESULT loadTmpFolderFiles()
 		res = preset_loadPatternSD(i);
 		if ((res != FR_OK) && (res != FR_NO_FILE))
 		{
-			f_close((FIL*)&SDFile);
+			preset_closeShared((FIL*)&SDFile);
 			lcd_process_display_interrupt = 1;
 			preset_initTmpFolder();
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
@@ -156,7 +202,7 @@ FRESULT loadTmpFolderFiles()
 		res = preset_loadSongSD(i);
 		if ((res != FR_OK) && (res != FR_NO_FILE))
 		{
-			f_close((FIL*)&SDFile);
+			preset_closeShared((FIL*)&SDFile);
 			lcd_process_display_interrupt = 1;
 			preset_initTmpFolder();
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
@@ -194,49 +240,20 @@ void preset_touchTimer(PresetTimer type)
 
 FRESULT preset_saveTmpKit()
 {
+    FRESULT res;
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-	HAL_TIM_Base_Start_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
-#endif
-	FRESULT res;
-	unsigned int bytesWritten;
-
-	//open the file
-	res = f_open((FIL*)&SDFile,"tmp.snd",FA_CREATE_ALWAYS | FA_WRITE);
-	if(res != FR_OK)
-	{
-#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
-#endif
-		return res;
-	}
-
-	//write current preset nr to file
-	res = f_write((FIL*)&SDFile,(void*)&menu_selectedPresetNr[SAVE_TYPE_KIT],1,&bytesWritten);
-	if(res != FR_OK)
-	{
-		f_close((FIL*)&SDFile);
-#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
+    HAL_TIM_Base_Start_IT(&htim2);
 #endif
-		return res;
-	}
-	//write the preset data
-	f_write((FIL*)&SDFile,activeParameters,END_OF_SOUND_PARAMETERS,&bytesWritten);
-
-	//close the file
-	f_close((FIL*)&SDFile);
+    res = preset_openShared((FIL*)&SDFile, "tmp.snd", FA_CREATE_ALWAYS | FA_WRITE);
+    if(res == FR_OK) {
+        res = preset_writeChecked(&menu_selectedPresetNr[SAVE_TYPE_KIT], 1);
+        if(res == FR_OK) res = preset_writeChecked(activeParameters, END_OF_SOUND_PARAMETERS);
+        res = preset_finishSave(res);
+    }
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
+    HAL_TIM_Base_Stop_IT(&htim2);
 #endif
-	return FR_OK;
+    return res;
 }
 
 FRESULT preset_LoadTmpKit()
@@ -246,7 +263,7 @@ FRESULT preset_LoadTmpKit()
 	uint8_t kitNr = 0;
 
 	//open the file
-	res = f_open((FIL*)&SDFile,"tmp.snd",FA_OPEN_EXISTING | FA_READ);
+	res = preset_openShared((FIL*)&SDFile,"tmp.snd",FA_OPEN_EXISTING | FA_READ);
 	if(res != FR_OK) return res;
 
 	//first the preset nr
@@ -288,7 +305,7 @@ FRESULT preset_LoadTmpKit()
 
 	closeFile:
 		//close the file handle
-		f_close((FIL*)&SDFile);
+		preset_closeShared((FIL*)&SDFile);
 
 		// update the soundengine with the new parameters
 		if(res==FR_OK) {
@@ -344,7 +361,7 @@ FRESULT preset_LoadTmpPat()
 
 
 
-		res = f_open((FIL*)&SDFile,"tmp.pat",FA_OPEN_EXISTING | FA_READ);
+		res = preset_openShared((FIL*)&SDFile,"tmp.pat",FA_OPEN_EXISTING | FA_READ);
 		if(res != FR_OK)
 		{
 		#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
@@ -460,7 +477,7 @@ FRESULT preset_LoadTmpPat()
 		{
 
 
-			f_close((FIL*)&SDFile);
+			preset_closeShared((FIL*)&SDFile);
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
 			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
 #else
@@ -475,7 +492,7 @@ FRESULT preset_LoadTmpPat()
 
 
 	closeFile:
-		f_close((FIL*)&SDFile);
+		preset_closeShared((FIL*)&SDFile);
 
 		#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
 					HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
@@ -493,108 +510,21 @@ FRESULT preset_LoadTmpPat()
 
 FRESULT preset_saveTmpPat()
 {
-
-	//don't save invalid pattern
-	if(menu_playedPattern >= NUM_PATTERN) return FR_INVALID_PARAMETER;
-#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-	HAL_TIM_Base_Start_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
-#endif
-
-	FRESULT res;
-	UINT bytesWritten;
-	uint8_t presetNr = menu_playedPattern;
-	//lcd_clear();
-	//lcd_home();
-	//lcd_string(PSTR("Saving pattern"));
-
-	//open the file
-	res = f_open((FIL*)&SDFile,"tmp.pat",FA_CREATE_ALWAYS | FA_WRITE);
-	if(res != FR_OK)
-	{
-#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
-#endif
-		return res;
-	}
-
-	//write current preset nr to file
-	res = f_write((FIL*)&SDFile,(void*)&menu_playedPattern,1,&bytesWritten);
-	if(res != FR_OK)
-	{
-		f_close((FIL*)&SDFile);
+    FRESULT res;
+    if(menu_playedPattern >= NUM_PATTERN) return FR_INVALID_PARAMETER;
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
+    HAL_TIM_Base_Start_IT(&htim2);
 #endif
-		return res;
-	}
-
-	//write pattern data
-	//preset_writePatternData(presetNr);
-#if 0
-#else
-
-		uint8_t length;
-
-		//---------------  write the step data (note, vol, prob, autom.) ---------------
-		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
-		{
-			f_write((FIL*)&SDFile,(const void*)&seq_patternSet.seq_subStepPattern[presetNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesWritten);
-
-			f_write((FIL*)&SDFile,(const void*)&seq_patternSet.seq_stepTiming[presetNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesWritten);
-		}
-
-		//--------------- now the main step data ---------------
-		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
-		{
-			for(uint8_t bar=0;bar<NUM_BARS; bar++)
-			{
-				const uint16_t mainStepData = seq_patternSet.seq_mainSteps[bar][presetNr][trk];
-				f_write((FIL*)&SDFile,(const void*)&mainStepData,sizeof(uint16_t),&bytesWritten);
-			}
-		}
-
-		//----- shuffle setting ------
-
-
-		//f_write((FIL*)&SDFile,(const void*)&activeParameters[PAR_SHUFFLE],sizeof(uint8_t),&bytesWritten);
-		f_write((FIL*)&SDFile,(const void*)&seq_patternSet.shuffle[presetNr],sizeof(uint8_t),&bytesWritten);
-
-		//----- pattern/track lengths ------
-		// we reuse the same call from above (when saving main step data)
-		// but we only use the length info retrieved. We want to store it at the end
-		// to avoid breaking compatibility with save file
-
-
-		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
-		{
-				length = seq_patternSet.length[presetNr][trk] ;//seq_patternSet.seq_mainSteps[bar][pat][trk];
-				f_write((FIL*)&SDFile,(const void*)&length,sizeof(uint8_t),&bytesWritten);
-		}
-
-		//write associated kit nr
-		f_write((FIL*)&SDFile,&seq_patternSet.kitNr[presetNr],sizeof(uint8_t),&bytesWritten);
-
-#endif
-
-	//close the file
-	f_close((FIL*)&SDFile);
-
-	//reset the lcd
-//	menu_repaintAll();
-
+    res = preset_openShared((FIL*)&SDFile, "tmp.pat", FA_CREATE_ALWAYS | FA_WRITE);
+    if(res == FR_OK) {
+        res = preset_writeChecked(&menu_playedPattern, 1);
+        if(res == FR_OK) res = preset_writePatternData(menu_playedPattern);
+        res = preset_finishSave(res);
+    }
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
-			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
-#else
-
+    HAL_TIM_Base_Stop_IT(&htim2);
 #endif
-
-	return res;
+    return res;
 }
 
 /*------------------------------------------------------------/
@@ -649,6 +579,8 @@ FRESULT delete_node (
 
 FRESULT preset_clearTmpFolder()
 {
+    FRESULT shared = preset_sharedStatus();
+    if(shared != FR_OK) return shared;
 	FILINFO fno;
 	FRESULT fr;
 	TCHAR buff[256];
@@ -668,23 +600,23 @@ FRESULT preset_clearTmpFolder()
 
 void preset_processTimer()
 {
-	if(preset_patDirty == 1)
-	{
-		if(HAL_GetTick()-preset_patTimer >= PRESET_SAVE_TIME )
-		{
-			preset_patDirty = 0;
-			preset_saveTmpPat();
-		}
-	}
-
-	if(preset_kitDirty == 1)
-	{
-		if(HAL_GetTick()-preset_kitTimer >= PRESET_SAVE_TIME )
-		{
-			preset_kitDirty = 0;
-			preset_saveTmpKit();
-		}
-	}
+    FRESULT res = FR_OK;
+    if(preset_sharedFailure != FR_OK) {
+        preset_resetAllSaveTimers();
+        return;
+    }
+    if(preset_patDirty && HAL_GetTick() - preset_patTimer >= PRESET_SAVE_TIME) {
+        preset_patDirty = 0;
+        res = preset_saveTmpPat();
+    }
+    if(res == FR_OK && preset_kitDirty && HAL_GetTick() - preset_kitTimer >= PRESET_SAVE_TIME) {
+        preset_kitDirty = 0;
+        res = preset_saveTmpKit();
+    }
+    if(res != FR_OK) {
+        preset_resetAllSaveTimers();
+        menu_showSdError(res); /* This service is called only by the foreground loop. */
+    }
 }
 
 
@@ -798,10 +730,10 @@ void preset_init(uint8_t presetNr)
 }
 
 //----------------------------------------------------
-static void preset_writeDrumsetData(uint8_t presetNr, uint8_t isMorph)
+static FRESULT preset_writeDrumsetData(uint8_t presetNr, uint8_t isMorph)
 {
 	uint16_t i;
-	unsigned int bytesWritten;
+	FRESULT res;
 
 	if(isMorph)
 	{
@@ -823,14 +755,17 @@ static void preset_writeDrumsetData(uint8_t presetNr, uint8_t isMorph)
 			{
 				value = preset_getMorphValue(i,activeParameters[PAR_MORPH]);
 			}					
-			f_write((FIL*)&SDFile,&value,1,&bytesWritten);
+			res = preset_writeChecked(&value,1);
+			if(res != FR_OK) return res;
 		}
 	} else {
 		for(i=0;i<END_OF_SOUND_PARAMETERS;i++)
 		{
-			f_write((FIL*)&SDFile,&mainParameters[presetNr][i],1,&bytesWritten);
+			res = preset_writeChecked(&mainParameters[presetNr][i],1);
+			if(res != FR_OK) return res;
 		}
 	}
+    return FR_OK;
 }
 
 //----------------------------------------------------
@@ -901,69 +836,43 @@ void preset_saveDrumsetRam(uint8_t presetNr, uint8_t isMorph)
 FRESULT preset_saveDrumsetSD(uint8_t presetNr, uint8_t isMorph)
 {
 #if USE_SD_CARD
-	FRESULT res;
-	unsigned int bytesWritten;
-	//filename in 8.3  format
-	char filename[13];
-	preset_makeFileNameSave(filename,presetNr,FEXT_SOUND, savefileNames[presetNr][KIT_NAMES]);
-	//sprintf(filename,"p%03d.snd",presetNr);
-
-
-
-	//open the file
-	res = f_open((FIL*)&SDFile,filename,FA_CREATE_ALWAYS | FA_WRITE);
-	if(res != FR_OK) return res;
-	
-	//write preset name to file
-
-	res = f_write((FIL*)&SDFile,(void*)savefileNames[presetNr][KIT_NAMES],8,&bytesWritten);
-	if(res != FR_OK) return res;
-	//write the preset data
-	preset_writeDrumsetData(presetNr, isMorph);
-	
-	//close the file
-	f_close((FIL*)&SDFile);
-
-	return FR_OK;
+    char filename[13];
+    FRESULT res;
+    if(presetNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
+    preset_makeFileNameSave(filename, presetNr, FEXT_SOUND, savefileNames[presetNr][KIT_NAMES]);
+    res = preset_openShared((FIL*)&SDFile, filename, FA_CREATE_ALWAYS | FA_WRITE);
+    if(res != FR_OK) return res;
+    res = preset_writeChecked(savefileNames[presetNr][KIT_NAMES], 8);
+    if(res == FR_OK) res = preset_writeDrumsetData(presetNr, isMorph);
+    return preset_finishSave(res);
 #else
+    return FR_NOT_READY;
 #endif
 };
 
 //----------------------------------------------------
-static void preset_writeGlobalData()
+static FRESULT preset_writeGlobalData()
 {
-	unsigned int bytesWritten;
+	FRESULT res;
 	int i;
 	for(i=PAR_BEGINNING_OF_GLOBALS;(i<NUM_PARAMS);i++)
 	{
-		f_write((FIL*)&SDFile,&activeParameters[i],1,&bytesWritten);
+		res = preset_writeChecked(&activeParameters[i],1);
+		if(res != FR_OK) return res;
 	}
+    return FR_OK;
 }
 
 //----------------------------------------------------
 FRESULT preset_saveGlobals()
 {
 #if USE_SD_CARD
-	FRESULT res;
-/*
-	res = f_mount((FATFS*)&SDFatFS, (TCHAR const*)SDPath,1);
-	if( res != FR_OK)
-	{
-		return res;
-	}
-	*/
-	//open the file
-	res = f_open((FIL*)&SDFile,"glo.cfg",FA_CREATE_ALWAYS | FA_WRITE);
-	if(res != FR_OK) return res;
-	preset_writeGlobalData();
-
-	//close the file
-	f_close((FIL*)&SDFile);
-
-	//f_unmount((TCHAR const*)SDPath);
-	return FR_OK;
+    FRESULT res = preset_openShared((FIL*)&SDFile, "glo.cfg", FA_CREATE_ALWAYS | FA_WRITE);
+    if(res != FR_OK) return res;
+    return preset_finishSave(preset_writeGlobalData());
 #else
-#endif	
+    return FR_NOT_READY;
+#endif
 }
 //----------------------------------------------------
 // returns 1 on success
@@ -1006,17 +915,17 @@ uint8_t preset_loadGlobals()
 	*/
 
 	//open the file
-	res = f_open((FIL*)&SDFile,"glo.cfg",FA_OPEN_EXISTING | FA_READ);
+	res = preset_openShared((FIL*)&SDFile,"glo.cfg",FA_OPEN_EXISTING | FA_READ);
 	
 	if(res!=FR_OK)
 		return 0; //file open error... maybe the file does not exist?
 
 	if(!preset_readGlobalData())
 	{
-		f_close((FIL*)&SDFile);
+		preset_closeShared((FIL*)&SDFile);
 		return 0;
 	}
-	f_close((FIL*)&SDFile);
+	preset_closeShared((FIL*)&SDFile);
 	menu_sendAllGlobals();
 	
 	//f_unmount((TCHAR const*)SDPath);
@@ -1075,8 +984,10 @@ void preset_loadDefaultGlobals()
 //----------------------------------------------------
 void preset_createGlobals()
 {
-	preset_loadDefaultGlobals();
-	preset_saveGlobals();
+    FRESULT res;
+    preset_loadDefaultGlobals();
+    res = preset_saveGlobals();
+    if(res != FR_OK) menu_showSdError(res);
 }
 //----------------------------------------------------
 FRESULT preset_loadLastUsedProjectNrFromSD(uint8_t* projNr)
@@ -1086,17 +997,17 @@ FRESULT preset_loadLastUsedProjectNrFromSD(uint8_t* projNr)
     *projNr = 0;
     unsigned int bytesRead;
 
-    res = f_open((FIL*)&SDFile,"/proj.idx",FA_OPEN_EXISTING | FA_READ);
+    res = preset_openShared((FIL*)&SDFile,"/proj.idx",FA_OPEN_EXISTING | FA_READ);
     if(res != FR_OK)
     {
     	//close the file handle
-    	f_close((FIL*)&SDFile);
+    preset_closeShared((FIL*)&SDFile);
     	return res;
     }
 
     res=f_read((FIL*)&SDFile, projNr,1, &bytesRead);
 	//close the file handle
-	f_close((FIL*)&SDFile);
+	preset_closeShared((FIL*)&SDFile);
 
 
 	return res;
@@ -1105,32 +1016,11 @@ FRESULT preset_loadLastUsedProjectNrFromSD(uint8_t* projNr)
 FRESULT preset_saveLastUsedProjectNrToSD(uint8_t projNr)
 {
 #if USE_SD_CARD
-    FRESULT res;
-
-	//open the file
-	res = f_open((FIL*)&SDFile,"/proj.idx",FA_CREATE_ALWAYS | FA_WRITE);
-	if(res != FR_OK)
-	{
-		//close the file handle
-		f_close((FIL*)&SDFile);
-		return res;
-	}
-
-	unsigned int bytesWritten;
-	res = f_write((FIL*)&SDFile,&projNr,1,&bytesWritten);
-	if(res != FR_OK)
-	{
-		//close the file handle
-		f_close((FIL*)&SDFile);
-		return res;
-	}
-
-	//close the file
-	f_close((FIL*)&SDFile);
-
-	//f_unmount((TCHAR const*)SDPath);
-	return res;
+    FRESULT res = preset_openShared((FIL*)&SDFile, "/proj.idx", FA_CREATE_ALWAYS | FA_WRITE);
+    if(res != FR_OK) return res;
+    return preset_finishSave(preset_writeChecked(&projNr, 1));
 #else
+    return FR_NOT_READY;
 #endif
 }
 //----------------------------------------------------
@@ -1203,7 +1093,7 @@ FRESULT preset_loadDrumsetSD(uint8_t presetNr, uint8_t isMorph)
 	{
 		//try old filename
 		preset_makeFileNameOld(filename,presetNr,FEXT_SOUND);
-		res = f_open((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
+		res = preset_openShared((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
 		if(res!=FR_OK)
 			goto error; //file open error... maybe the file does not exist?
 	}
@@ -1237,7 +1127,7 @@ FRESULT preset_loadDrumsetSD(uint8_t presetNr, uint8_t isMorph)
 #endif
 closeFile:
 	//close the file handle
-	f_close((FIL*)&SDFile);
+	preset_closeShared((FIL*)&SDFile);
 
 	// update the soundengine with the new parameters
 	if(res==FR_OK) {
@@ -1592,19 +1482,22 @@ void preset_queryMainStepDataFromSeq(uint16_t stepNr, uint16_t *mainStepData, ui
 };
 
 //----------------------------------------------------
-static void preset_writePatternData(uint8_t patternNr)
+static FRESULT preset_writePatternData(uint8_t patternNr)
 {
 #if 0
 #else
-		UINT bytesWritten;
+		FRESULT res;
 		uint8_t length;
 
 		//---------------  write the step data (note, vol, prob, autom.) ---------------
 		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
 		{
-			f_write((FIL*)&SDFile,(const void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesWritten);
+			res = preset_writeChecked((const void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN);
+			if(res != FR_OK) return res;
+
+			res = preset_writeChecked((const void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN);
 
-			f_write((FIL*)&SDFile,(const void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesWritten);
+			if(res != FR_OK) return res;
 		}
 
 		//--------------- now the main step data ---------------
@@ -1613,7 +1506,8 @@ static void preset_writePatternData(uint8_t patternNr)
 			for(uint8_t bar=0;bar<NUM_BARS; bar++)
 			{
 				const uint16_t mainStepData = seq_patternSet.seq_mainSteps[bar][patternNr][trk];
-				f_write((FIL*)&SDFile,(const void*)&mainStepData,sizeof(uint16_t),&bytesWritten);
+				res = preset_writeChecked((const void*)&mainStepData,sizeof(uint16_t));
+				if(res != FR_OK) return res;
 			}
 		}
 
@@ -1621,7 +1515,8 @@ static void preset_writePatternData(uint8_t patternNr)
 
 
 		//f_write((FIL*)&SDFile,(const void*)&activeParameters[PAR_SHUFFLE],sizeof(uint8_t),&bytesWritten);
-		f_write((FIL*)&SDFile,(const void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesWritten);
+		res = preset_writeChecked((const void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t));
+		if(res != FR_OK) return res;
 
 		//----- pattern/track lengths ------
 		// we reuse the same call from above (when saving main step data)
@@ -1632,13 +1527,16 @@ static void preset_writePatternData(uint8_t patternNr)
 		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
 		{
 				length = seq_patternSet.length[patternNr][trk] ;//seq_patternSet.seq_mainSteps[bar][pat][trk];
-				f_write((FIL*)&SDFile,(const void*)&length,sizeof(uint8_t),&bytesWritten);
+				res = preset_writeChecked((const void*)&length,sizeof(uint8_t));
+				if(res != FR_OK) return res;
 		}
 
 		//write associated kit nr
-		f_write((FIL*)&SDFile,&seq_patternSet.kitNr[patternNr],sizeof(uint8_t),&bytesWritten);
+		res = preset_writeChecked(&seq_patternSet.kitNr[patternNr],sizeof(uint8_t));
+		if(res != FR_OK) return res;
 
 #endif
+    return FR_OK;
 }
 
 //----------------------------------------------------
@@ -1676,39 +1574,22 @@ void preset_savePatternTemp(uint8_t presetNr)
 //----------------------------------------------------
 FRESULT preset_savePatternSD(uint8_t presetNr)
 {
-
-	FRESULT res;
-	UINT bytesWritten;
-	//lcd_clear();
-	//lcd_home();
-	//lcd_string(PSTR("Saving pattern"));
-	
-	
-	//filename in 8.3  format
-	char filename[13];
-	//sprintf(filename,"p%03d.pat",presetNr);
-	preset_makeFileNameSave(filename,presetNr,FEXT_PAT, savefileNames[presetNr][PATTERN_NAMES]);
-	//open the file
-	res = f_open((FIL*)&SDFile,filename,FA_CREATE_ALWAYS | FA_WRITE);
-	if(res != FR_OK) return res;
-
-	//write preset name to file
-	res = f_write((FIL*)&SDFile,(void*)savefileNames[presetNr][PATTERN_NAMES],8,&bytesWritten);
-	if(res != FR_OK) return res;
-	//write version tag
-	res = f_write((FIL*)&SDFile,"PatV03",6,&bytesWritten);
-	if(res != FR_OK) return res;
-
-	//write pattern data
-	preset_writePatternData(presetNr);
-
-	//close the file
-	f_close((FIL*)&SDFile);
-	
-	//reset the lcd
-	menu_repaintAll();
-	
-	return FR_OK;
+#if USE_SD_CARD
+    char filename[13];
+    FRESULT res;
+    if(presetNr >= NUM_PATTERN) return FR_INVALID_PARAMETER;
+    preset_makeFileNameSave(filename, presetNr, FEXT_PAT, savefileNames[presetNr][PATTERN_NAMES]);
+    res = preset_openShared((FIL*)&SDFile, filename, FA_CREATE_ALWAYS | FA_WRITE);
+    if(res != FR_OK) return res;
+    res = preset_writeChecked(savefileNames[presetNr][PATTERN_NAMES], 8);
+    if(res == FR_OK) res = preset_writeChecked("PatV03", 6);
+    if(res == FR_OK) res = preset_writePatternData(presetNr);
+    res = preset_finishSave(res);
+    if(res == FR_OK) menu_repaintAll();
+    return res;
+#else
+    return FR_NOT_READY;
+#endif
 }
 
 //----------------------------------------------------
@@ -2029,7 +1910,7 @@ FRESULT preset_loadPatternSD(uint8_t presetNr)
 	{
 		//try old filename
 		preset_makeFileNameOld(filename,presetNr,FEXT_PAT);
-		res = f_open((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
+		res = preset_openShared((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
 		if(res!=FR_OK)
 			return res; //file open error... maybe the file does not exist?
 	}
@@ -2072,7 +1953,7 @@ FRESULT preset_loadPatternSD(uint8_t presetNr)
 
 
 closeFile:
-	f_close((FIL*)&SDFile);
+	preset_closeShared((FIL*)&SDFile);
 	
 	//force complete repaint
 	menu_repaintAll();
@@ -2245,6 +2126,8 @@ uint8_t preset_getMorphValue(uint16_t index, uint8_t morph)
 
 FRESULT preset_createProject(uint8_t presetNr)
 {
+    FRESULT shared = preset_sharedStatus();
+    if(shared != FR_OK) return shared;
 
 
 	//create default globals
@@ -2293,6 +2176,8 @@ FRESULT preset_createProject(uint8_t presetNr)
 
 FRESULT preset_saveProject(uint8_t presetNr)
 {
+    FRESULT shared = preset_sharedStatus();
+    if(shared != FR_OK) return shared;
 	FRESULT res;
 	FILINFO fno;
 
@@ -2388,7 +2273,7 @@ FRESULT preset_saveProject(uint8_t presetNr)
 
 	//Save Project name
 	//open the file
-	res = f_open((FIL*)&SDFile,"prj.nfo",FA_CREATE_ALWAYS | FA_WRITE);
+	res = preset_openShared((FIL*)&SDFile,"prj.nfo",FA_CREATE_ALWAYS | FA_WRITE);
 	if(res != FR_OK) {
 		lcd_process_display_interrupt = 1;
 		preset_initTmpFolder();
@@ -2415,7 +2300,7 @@ FRESULT preset_saveProject(uint8_t presetNr)
 		return res;
 	}
 	//close the file
-	f_close((FIL*)&SDFile);
+	preset_closeShared((FIL*)&SDFile);
 
 //	soundengine_process();
 	//save globals
@@ -2571,7 +2456,7 @@ FRESULT preset_loadSongSD(uint8_t songNr)
 	{
 		//try old filename
 		preset_makeFileNameOld(filename,songNr,FEXT_SONG);
-		res = f_open((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
+		res = preset_openShared((FIL*)&SDFile,filename,FA_OPEN_EXISTING | FA_READ);
 		if(res!=FR_OK)
 			return res; //file open error... maybe the file does not exist?
 	}
@@ -2592,45 +2477,28 @@ FRESULT preset_loadSongSD(uint8_t songNr)
 
 	closeFile:
 		//close the file handle
-		f_close((FIL*)&SDFile);
+		preset_closeShared((FIL*)&SDFile);
 		return res;
 }
 //----------------------------------------------------
 FRESULT preset_saveSongSD(uint8_t songNr)
 {
-	FRESULT res;
-	char filename[13];
-	unsigned int bytesWritten;
-	preset_makeFileNameSave(filename,songNr,FEXT_SONG, savefileNames[songNr][SONG_NAMES]);
-	//open the file
-	res = f_open((FIL*)&SDFile,filename,FA_CREATE_ALWAYS | FA_WRITE);
-	if(res!=FR_OK)
-		return res; //file open error... maybe the file does not exist?
-	//write name
-	res = f_write((FIL*)&SDFile,(void*)savefileNames[songNr][SONG_NAMES],8,&bytesWritten);
-	if (res != FR_OK)
-	{
-		f_close((FIL*)&SDFile);
-		return res;
-	}
-
-	//write song data
-	res = f_write((FIL*)&SDFile,seq_songData[songNr],MAX_SONG_LENGTH,&bytesWritten);
-	if (res != FR_OK)
-	{
-		f_close((FIL*)&SDFile);
-		return res;
-	}
-
-	//close the file handle
-	res = f_close((FIL*)&SDFile);
-	return res;
+    char filename[13];
+    FRESULT res;
+    if(songNr >= NUM_KIT_SLOTS) return FR_INVALID_PARAMETER;
+    preset_makeFileNameSave(filename, songNr, FEXT_SONG, savefileNames[songNr][SONG_NAMES]);
+    res = preset_openShared((FIL*)&SDFile, filename, FA_CREATE_ALWAYS | FA_WRITE);
+    if(res != FR_OK) return res;
+    res = preset_writeChecked(savefileNames[songNr][SONG_NAMES], 8);
+    if(res == FR_OK) res = preset_writeChecked(seq_songData[songNr], MAX_SONG_LENGTH);
+    return preset_finishSave(res);
 }
 
 
 //----------------------------------------------------
 void preset_loadProjectNames(uint8_t projectNr)
 {
+    if(preset_sharedStatus() != FR_OK) return;
 	FRESULT res;
 	FILINFO fno;
 
@@ -2653,7 +2521,7 @@ void preset_loadProjectNames(uint8_t projectNr)
 		UINT bytesRead;
 
 		//open the file
-		res = f_open((FIL*)&SDFile,"prj.nfo",FA_OPEN_EXISTING | FA_READ);
+		res = preset_openShared((FIL*)&SDFile,"prj.nfo",FA_OPEN_EXISTING | FA_READ);
 
 		//read preset name
 		//first the preset name
@@ -2666,13 +2534,15 @@ void preset_loadProjectNames(uint8_t projectNr)
 			sprintf((void*)savefileNames[projectNr][PROJECT_NAMES], "Empty");
 		}
 
-		f_close((FIL*)&SDFile);
+		preset_closeShared((FIL*)&SDFile);
 		preset_initTmpFolder();
 
 }
 //----------------------------------------------------
 FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
 {
+    FRESULT shared = preset_sharedStatus();
+    if(shared != FR_OK) return shared;
 	FRESULT res;
 	FILINFO fno;
 	char text[17];
@@ -2755,7 +2625,7 @@ FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
 	UINT bytesRead;
 
 	//open the file
-	res = f_open((FIL*)&SDFile,"prj.nfo",FA_OPEN_EXISTING | FA_READ);
+	res = preset_openShared((FIL*)&SDFile,"prj.nfo",FA_OPEN_EXISTING | FA_READ);
 	if (res != FR_OK)
 	{
 		lcd_process_display_interrupt = 1;
@@ -2779,7 +2649,7 @@ FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
 #else
 
 #endif
-		f_close((FIL*)&SDFile);
+		preset_closeShared((FIL*)&SDFile);
 		return res;
 	}
 
@@ -2803,7 +2673,7 @@ FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
 		sprintf((void*)savefileNames[presetNr][PROJECT_NAMES], "Empty");
 	}
 	//close the file
-	f_close((FIL*)&SDFile);
+	preset_closeShared((FIL*)&SDFile);
 
 	//load globals
 	preset_loadGlobals();
@@ -2821,7 +2691,7 @@ FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
 		{
 			lcd_process_display_interrupt = 1;
 			preset_initTmpFolder();
-			f_close((FIL*)&SDFile);
+			preset_closeShared((FIL*)&SDFile);
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
 			HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
 #else
@@ -2832,7 +2702,7 @@ FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
 		res = preset_loadPatternSD(i);
 		if ((res != FR_OK) && (res != FR_NO_FILE))
 		{
-			f_close((FIL*)&SDFile);
+			preset_closeShared((FIL*)&SDFile);
 			lcd_process_display_interrupt = 1;
 			preset_initTmpFolder();
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
@@ -2845,7 +2715,7 @@ FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
 		res = preset_loadSongSD(i);
 		if ((res != FR_OK) && (res != FR_NO_FILE))
 		{
-			f_close((FIL*)&SDFile);
+			preset_closeShared((FIL*)&SDFile);
 			lcd_process_display_interrupt = 1;
 			preset_initTmpFolder();
 #if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
@@ -2884,7 +2754,8 @@ FRESULT preset_loadProject(uint8_t presetNr, uint8_t clearTmp)
 
 	}
 
-	preset_saveLastUsedProjectNrToSD(presetNr);
+	FRESULT indexResult = preset_saveLastUsedProjectNrToSD(presetNr);
+	if(res == FR_OK && indexResult != FR_OK) res = indexResult;
 
 	//change pattern to pat 1
 	seq_setNextPattern(0);
@@ -3045,6 +2916,8 @@ static void preset_makeFileNameOld(char *buf, uint8_t num, uint8_t type)
 //load uses wildcard to find a name of the pattern 00-*.ext
 static FRESULT preset_makeFileNameLoad(char *buf, uint8_t num, uint8_t type)
 {
+    FRESULT shared = preset_sharedStatus();
+    if(shared != FR_OK) return shared;
 
 	FRESULT fr;     /* Return value */
     DIR dj;         /* Directory object */
@@ -3082,7 +2955,7 @@ static FRESULT preset_makeFileNameLoad(char *buf, uint8_t num, uint8_t type)
     f_closedir(&dj);
     if(fr == FR_OK && fno.fname[0])
     {
-    	fr = f_open((FIL*)&SDFile,fno.fname,FA_OPEN_EXISTING | FA_READ);
+    fr = preset_openShared((FIL*)&SDFile,fno.fname,FA_OPEN_EXISTING | FA_READ);
     	return fr;
     }
 
diff --git a/Core/Src/buttonHandler.c b/Core/Src/buttonHandler.c
index da2703b..a3affbd 100644
--- a/Core/Src/buttonHandler.c
+++ b/Core/Src/buttonHandler.c
@@ -1150,7 +1150,8 @@ static void buttonHandler_seqButtonPressed(uint8_t seqButtonPressed)
 
 
 
-				preset_savePatternRAM(seqButtonPressed + 16 * menu_activePatternBank);
+				FRESULT saveResult = preset_savePatternRAM(seqButtonPressed + 16 * menu_activePatternBank);
+				if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
 
 				//save to tmp folder
 				#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
@@ -1158,7 +1159,7 @@ static void buttonHandler_seqButtonPressed(uint8_t seqButtonPressed)
 				#else
 
 				#endif
-					preset_savePatternSD(seqButtonPressed + 16 * menu_activePatternBank);
+					saveResult = preset_savePatternSD(seqButtonPressed + 16 * menu_activePatternBank);
 				#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
 							HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
 				#else
@@ -1166,6 +1167,7 @@ static void buttonHandler_seqButtonPressed(uint8_t seqButtonPressed)
 				#endif
 
 
+				if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
 				menu_selectedPresetNr[SAVE_TYPE_PATTERN] = seqButtonPressed + 16 * menu_activePatternBank;
 				menu_activePage = PATTERN_SAVE_PAGE_OK;
 				sucessTimer = systick_ticks;
diff --git a/Core/Src/menu/menu.c b/Core/Src/menu/menu.c
index 080e0ba..c6b45d2 100644
--- a/Core/Src/menu/menu.c
+++ b/Core/Src/menu/menu.c
@@ -2035,11 +2035,12 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 						case SAVE_TYPE_SONG:
 							//nothing to do here
 							break;
-						case SAVE_TYPE_PATTERN:
+						case SAVE_TYPE_PATTERN: {
 
 
 
-							preset_savePatternRAM(menu_selectedPresetNr[SAVE_TYPE_PATTERN]);
+							FRESULT saveResult = preset_savePatternRAM(menu_selectedPresetNr[SAVE_TYPE_PATTERN]);
+							if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
 
 							//save to tmp folder
 										#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
@@ -2047,17 +2048,20 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 										#else
 
 										#endif
-											preset_savePatternSD(menu_selectedPresetNr[SAVE_TYPE_PATTERN]);
+											saveResult = preset_savePatternSD(menu_selectedPresetNr[SAVE_TYPE_PATTERN]);
 										#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
 													HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
 										#else
 
 										#endif
+							if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
 							break;
+						}
 
-						case SAVE_TYPE_KIT:
+						case SAVE_TYPE_KIT: {
 
 
+							FRESULT saveResult;
 							preset_saveDrumsetRam(menu_selectedPresetNr[SAVE_TYPE_KIT],0);
 
 							//save to tmp folder
@@ -2066,19 +2070,21 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 							#else
 
 							#endif
-														preset_saveDrumsetSD(menu_selectedPresetNr[SAVE_TYPE_KIT],0);
+														saveResult = preset_saveDrumsetSD(menu_selectedPresetNr[SAVE_TYPE_KIT],0);
 							#if USE_INTERRUPT_ONLY_DURING_SAVE_PROJ
 										HAL_TIM_Base_Stop_IT(&htim2);	//calc sample interrupt - prio 1
 							#else
 
 							#endif
 
+							if(saveResult != FR_OK) { menu_showSdError(saveResult); return; }
 							menu_loadedPresetNr[CURRENT_KIT] = menu_selectedPresetNr[SAVE_TYPE_KIT];
 							menu_activePage = KIT_SAVE_PAGE_OK;
 							sucessTimer = systick_ticks;
 							menu_repaint();
 
 							break;
+						}
 
 						case SAVE_TYPE_MORPH:
 							preset_saveDrumsetRam(menu_selectedPresetNr[SAVE_TYPE_MORPH],1);
@@ -2088,7 +2094,7 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 						{
 							FRESULT res;
 							res = preset_saveGlobals();
-							if(res != FR_OK) menu_showSdError(res);
+							if(res != FR_OK) { menu_showSdError(res); return; }
 						}
 							break;
 
diff --git a/tests/euclidean/host.c b/tests/euclidean/host.c
index d173290..16ac85d 100644
--- a/tests/euclidean/host.c
+++ b/tests/euclidean/host.c
@@ -38,6 +38,8 @@ uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS];
 char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];
 TIM_HandleTypeDef htim2;
 FIL SDFile;
+static FATFS host_filesystem;
+static FRESULT preset_sharedFailure;
 #define STEPS_PER_PATTERN NUM_STEPS
 #define FEXT_PAT 1
 static uint8_t stream[65536], saved_stream[65536];
@@ -45,11 +47,11 @@ static size_t stream_pos, stream_size, error_at = SIZE_MAX;
 static float elapsed = 75.f;
 static unsigned triggers;
 FRESULT f_open(FIL *file, const TCHAR *name, BYTE flags) {
-    (void)name; stream_pos=0; file->fptr=0;
+    (void)name; stream_pos=0; file->fptr=0; file->obj.fs=&host_filesystem;
     if(flags & FA_CREATE_ALWAYS) stream_size=0;
     return FR_OK;
 }
-FRESULT f_close(FIL *file) { (void)file; return FR_OK; }
+FRESULT f_close(FIL *file) { file->obj.fs=0; return FR_OK; }
 FRESULT f_lseek(FIL *file, DWORD offset) {
     assert(offset<=stream_size); stream_pos=offset; file->fptr=offset; return FR_OK;
 }
@@ -93,6 +95,7 @@ void led_setValue(uint8_t value,uint8_t led) { (void)value; (void)led; }
 #include "functions.inc"
 
 static void reset(void) {
+    memset(&SDFile,0,sizeof SDFile); preset_sharedFailure=FR_OK;
     memset(&seq_patternSet,0,sizeof(seq_patternSet));
     for(unsigned p=0;p<NUM_PATTERN;p++) for(unsigned t=0;t<NUM_TRACKS;t++)
         seq_patternSet.length[p][t]=16;
diff --git a/tests/euclidean/run.py b/tests/euclidean/run.py
index a66c836..5670831 100644
--- a/tests/euclidean/run.py
+++ b/tests/euclidean/run.py
@@ -54,6 +54,8 @@ def main():
         'seq_isMainStepActive', 'seq_setMainStep', 'seq_getShiftFlamPattern',
         'seq_processFlam', 'seq_addNote')]
     functions += [(preset, name) for name in (
+        'preset_sharedStatus', 'preset_openShared', 'preset_closeShared',
+        'preset_writeChecked', 'preset_finishSave',
         'preset_readPatternLengths', 'preset_writePatternData',
         'preset_readPatternData', 'preset_savePatternSD', 'preset_loadPatternSD',
         'preset_saveTmpPat', 'preset_LoadTmpPat')]

```

## Appendix B — Exact executed validation logs

Captured output below retains the build system's generic installation text solely as log evidence; it was not executed. No flashing command was run. Final Debug ELF hashes differ from the earlier build only after source indentation/line metadata cleanup; the final image payload hashes remain as reported.

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

### final-00.log

```text
$ python3 tests/save_errors/run.py --verify-baseline
PASS frozen oracle exactly matches pinned S3 production function bodies
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, Morph endpoints/midpoint, identical names/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, stopped background retries; no UI/hardware execution

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

### 12.log

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

### 14.log

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

### 16.log

```text
$ cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-release

EXIT 0
```

### 18.log

```text
$ cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/diagnostics-debug

EXIT 0
```

### final-01.log

```text
$ cmake --build --preset release --parallel 4
[1/4] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[2/4] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      225424 B       992 KB     22.19%
   text	   data	    bss	    dec	    hex	filename
 276576	    852	 341112	 618540	  9702c	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 277436 bytes, checksum 0x8B
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

### final-02.log

```text
$ cmake --build --preset debug --parallel 4
[1/4] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[2/4] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      225472 B       992 KB     22.20%
   text	   data	    bss	    dec	    hex	filename
 276624	    852	 341112	 618588	  9705c	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 277484 bytes, checksum 0x20
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

### final-03.log

```text
$ cmake --build build/diagnostics-release --parallel 4
[1/4] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[2/4] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      225792 B       992 KB     22.23%
   text	   data	    bss	    dec	    hex	filename
 277016	    852	 341184	 619052	  9722c	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 277876 bytes, checksum 0x07
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

### final-04.log

```text
$ cmake --build build/diagnostics-debug --parallel 4
[1/4] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[2/4] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[3/4] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      225840 B       992 KB     22.23%
   text	   data	    bss	    dec	    hex	filename
 277064	    852	 341184	 619100	  9725c	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 277924 bytes, checksum 0xDF
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

### final-05.log

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0x8B", "configuration": "release", "elf_sha256": "08234d93e804e81eecd0ddbf6f77194ec672b930515996bf1166f1f6d9700a03", "image_bytes": 277452, "image_sha256": "3ba0f3f1d455712858f5d7c551b7528d6166a13f556fd04071fc27aba791545e", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224572, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 277436, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x20", "configuration": "debug", "elf_sha256": "da108c8feb0ec6bee35e04d502dd95b3b3aed6b7e9242bd3c1ed418e8ccf1cf9", "image_bytes": 277500, "image_sha256": "6284756fda982152eef935a78d991c3a63741fb0cb4cb7f38696baf51a91aa98", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224620, "ITCM_RAM": 15016, "RAM": 271576}, "payload_bytes": 277484, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0x07", "configuration": "diagnostics-release", "elf_sha256": "cd684374537d526ef201097d14796de59315486eef10cc8fac9187f227d7f255", "image_bytes": 277892, "image_sha256": "f416b68866f55b64be6b6e34e76c3f5664f931db55c74d8baa982ad531536b6d", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224940, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 277876, "reset_vector": "0x08014E4D", "result": "PASS"}
{"checksum": "0xDF", "configuration": "diagnostics-debug", "elf_sha256": "3edd5fe5734854a71b483abd3c35a63b5ca403ed3c1b77de07006c7cb6dcd8ab", "image_bytes": 277940, "image_sha256": "6b2038462795ba0e7f9da0132935012cb927de61a420dff1fd83d9abbaafa0ed", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 224988, "ITCM_RAM": 15088, "RAM": 271648}, "payload_bytes": 277924, "reset_vector": "0x08014E4D", "result": "PASS"}

EXIT 0
```

## Appendix C — Additional artifact hashes and invariant checks

These are hashes of local outputs, not downloadable installation artifacts.

| Configuration | `.bin` SHA-256 | First16 image bytes |
|---|---|---|
|release|`86460da014275e94a1fb8c8d7f01cd7a7e4d23db65594550b375e5e7a5b96e42`|`4c58525632494d47bc3b04008b000000`|
|debug|`327a6d03a64dabb63a98d05febc0ea6ba049841fdb6340a5b6a8cdce89862cfd`|`4c58525632494d47ec3b040020000000`|
|diagnostics-release|`281f5eb8f5bf72acffce6d5bcc7f5bb2510048c1733592abc3379e5747971dbe`|`4c58525632494d47743d040007000000`|
|diagnostics-debug|`6ea09eabca76fbddaeca6cb6a4c71e37d076657a5eb4a4e7b2bdee8870ebf0db`|`4c58525632494d47a43d0400df000000`|

Baseline/final audio object comparison (reverified after the final four builds):

```text
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED 7885756e6a7c84b5a8dcf8cb870bc6571480027eb25eaa6959b3be287f0693f1
build/debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 06db7a571ad5abd61c30d6b74509e2a0c0e15326f0dc9cce6aff7c3cd13a4682
build/debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED b3c0a151f657e4ed3438bdea9686705d729aa7e66dae1293d32f20e1af5e5a3c
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED d6ee7b60ad6a2a1924e096b3631b8f38284349e3ad7c037c348e5ca3e7f17aca
build/release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 10472838499bb9806639fc82153ba880797c3c496ac48e10e6724aeaa2e12ac5
build/release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 01463848afda54efbbeea116af34c9ff35db325406b42b003432ee46fba5c468
build/diagnostics-release/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 66cd4b9de166101afc4fae3970b75e726495517a6de6957ca4b7ef72863fa5d6
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED d1ed2c18ce02443fa1ed7ddae0633a61de9f31e39ba3882bb5f73378ad38dea2
build/diagnostics-release/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 46b4aa19065504b170ae148e4e6d0794856bd512529aa837a4d5e77479c5ab7d
build/diagnostics-debug/Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj: UNCHANGED 9c55bb9c71c07e4bd35ee221dfae028522f877dbae168836196cdd3498ea07c5
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj: UNCHANGED 498f228e10b3e059fcf4a3033150897ebd12ee3005c4d4f9e8eedb78d9bc38a8
build/diagnostics-debug/Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj: UNCHANGED 15922ba42e0b45f1193e5a83e721079d11b3ee0ada756c9113991b707611268c
```

Release failure-state symbol (`arm-none-eabi-nm -S`, bytes):

```text
20025ec4 00000001 b preset_sharedFailure
```

Image/load-section limits use the unchanged linker regions: ITCM0x00000000/16KiB; DTCM0x20000000/128KiB; RAM0x20020000/384KiB; application FLASH0x08008000/992KiB. No runtime capacity was inferred from linker success.

## Appendix D — Publication continuation contract

The source ref was fetched directly from `git/refs/heads/safety/sd-save-errors-s4a`; it resolves to `c87666ff945e2e01cbbf1ceda72fb503638de579`. Its Git commit parent and recursively fetched tree match the published manifest. The source branch was absent initially and no pre-existing branch was reused.

The report must be committed as a single new Markdown file on the new report-only branch from lab-setup. Its post-publication file fetch must equal this report byte-for-byte, and the report commit parent/diff and all pre-existing remote heads must be checked before the final success response. Those checks occur after creation of this immutable report commit; its self-referential commit hash is intentionally not embedded. A future assistant should fetch both branch heads and the report URL in section1 rather than assuming a local temporary file proves publication.
