# Minimal Load Kit / Load Morph Kit Success Feedback

Date: 2026-10-28 UTC. Repository: DocLXR-02/lxr-desktop-app.

## Outcome and exact scope

**IMPLEMENTED; all required software gates passed. NOT FLASHED — NOT HARDWARE VALIDATED.** Only two existing manual RAM load actions gain a transient two-line confirmation:

```text
Load Kit
success!
```

```text
Load Morph Kit
success!
```

The software expiry deadline is 750 milliseconds. No blocking delay, busy loop, file operation, allocation, new physical control or error screen was added. Existing Save Kit/Morph Kit, Load/Save Pattern, Song, Settings and Project behavior/feedback and silent autosave remain unchanged. No broader feedback candidate was published with this patch. No binaries were published. No DSP, Morph interpolation, MIDI, bootloader, updater, clocks, linker, audio DMA or IRQ source changes occurred.

## Baseline, branch and preservation

- Approved baseline branch: controls/morph-range-127.
- Exact baseline: **83c5ec164d6c3dab439deb2f32a8edf1138b5b77**; tree b2fe39851f16490fef27f1daf84a55c1322b8fd7.
- New implementation branch: **ui/kit-load-confirmation**.
- Implementation commit: **c870f7249d7ee0d65423bc8b7bfdb422dcc74482**, sole parent the exact approved baseline.
- Implementation tree: b89db2323a1e399e583ccb724a38adf29ce75e18.
- Source URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/ui/kit-load-confirmation
- Commit URL: https://github.com/DocLXR-02/lxr-desktop-app/commit/c870f7249d7ee0d65423bc8b7bfdb422dcc74482
- Report-only branch: codex-reports/2026-10-28-kit-load-confirmation.
- Report path: docs/codex-berichte/2026-10-28_kit-load-confirmation.md.
- Report base: unchanged lab-setup, 5829398cea5d337200393757e39a37d4d8148256.

AGENTS.md was read completely. The remote baseline head was verified. Existing checkout /workspace/lxr-desktop-app was used, with no new worktree. Main and all pre-existing branches were preserved; no merge, reset or force push was used.

At entry the checkout contained the unapproved broad candidate on ui/manual-load-save-feedback, with seven modified tracked files and two untracked test locations. The complete tracked diff was backed up to /tmp/load-confirm/broad-candidate.patch. `git stash push --include-untracked` preserved tracked and untracked work together in local stash **be96d66a51e7cfab3268bbd25d16d89652a495c7**, message “Preserved unapproved broad feedback candidate before narrow kit-load task”. That stash is retained and not applied to the new branch. To inspect it, use `git stash show --include-untracked` or its Git objects; do not blindly apply it over this patch. The broad branch ref itself was not advanced. Its previous report remains intact.

The user's physical validation of baseline 83c5ec1 is user-supplied hardware evidence. This task performed no physical validation of the changed revision. Source version remains 1.7; equivalence to manufacturer v1.71 is not claimed.

## Source audit and truthful completion

CODE: `Core/Src/Preset/presetManager.c:808`, preset_loadDrumsetRam(slot,isMorph,sendImmediate), returns immediately for slot>=NUM_KIT_SLOTS (64). For a valid slot it synchronously calls existing foreground sound servicing, then either copies exactly END_OF_SOUND_PARAMETERS sound bytes into activeParameters (normal kit) or selects morphParameters=mainParameters[slot] (Morph destination), services sound again and sends existing parameter updates. It is RAM-only and returns void. There is no further recoverable RAM-load error result to check. A valid completed copy/pointer selection establishes the requested RAM action; this does not establish SD loading, persistence or audible settling.

The menu invokes it in the enabled `#if 1` Load Kit and Load Morph Kit cases. Baseline indices were assumed valid by callers; the new guards validate the selected byte before invoking the loader or publishing any success. Other metadata updates, pattern-associated-kit assignment and autosave dirty-touch logic stay as before.

Final source references:

- `Core/Src/menu/menu.c:675`: private notice kind and millisecond timestamp.
- `:1826`: transient load renderer participates in existing repaint infrastructure.
- `:1942`: menu_showKitLoad(), called only after the actual RAM load returns.
- `:1948`: menu_renderKitLoad(), two padded 16-character rows, existing visible-cursor and refresh flags.
- `:1967`: expiry added to the existing notification-expiry service; generic error expiry remains 2000ms and its renderer/producer are unchanged.
- `:2015`: a new explicit Load/Save-menu button action cancels only this transient notice before its command executes. This prevents a preceding confirmation from covering subsequent manual/project work; it does not change those commands, success pages, error states or file operations.
- `:2227`/`:2231`: normal slot guard, real loader, confirmation publication.
- `:2253`/`:2256`: Morph slot guard, real loader, confirmation publication.

No additional confirmation is emitted from automatic associated-kit changes, startup, SD readers, project loading, pattern selection or autosave. The two private show calls are the sole publishers. Invalid manual selections return silently, with no new error screen and no false success. A new explicit command clears an earlier notice first. Existing no-op Save Song and Load Pattern commands remain untouched and gain no feedback.

## Display lifecycle and concurrency

The two scalar state objects are volatile: an 8-bit kind (0 inactive, 1 normal, 2 Morph) and a 32-bit timestamp. Publication writes timestamp first, then kind, then queues repaint. Expiry uses unsigned `(uint32_t)(HAL_GetTick()-since) >= 750u`, so wraparound is safe. There is no DWT use/reset.

Both normal rendering and the existing expiry service reject expired notices. Expiry queues repaint of the current page; it does not store/restore an old page, change focus, process input, stop transport or discard edits. Existing encoder/button processing remains available while the notice is visible. A repeat valid load refreshes the timer. Generic SD errors retain their original state/lifetime; a load notice can temporarily overlay a pre-existing generic error but never clears its state or fault latch. Once expired, normal renderer priority resumes.

The existing TIM14 display service can preempt foreground rendering. Renderer snapshots the notice kind, copies two rows using memcpy, and queues a clean repaint if expiry changed the kind during those copies. The deterministic test injects expiry at a real copy, then confirms the next refresh removes stale text. Only the foreground publishes notices; ISR expiry clears them. No lock or interrupt masking was added.

CODE/OPEN distinction: this is a 750ms **software eligibility deadline**, not a measured physical LCD duration. Existing refresh cadence, LCD readiness and higher-priority execution determine when pixels update. A 250ms margin below one second was chosen; physical display timing still needs acceptance observation. No claim of a hard wall-clock pixel guarantee during a pre-existing stalled display/peripheral condition is made.

When inactive, the new paths perform bounded state checks without a new timing read in the renderer/expiry condition. When active, HAL tick checks and two fixed-size row copies run through the existing display architecture. TIM14 display work can preempt audio as it already did; the patch adds bounded display work, not audio-path work. No actual ISR-cycle or audio-continuity measurement was performed.

## Complete changed-file list

1. Core/Src/menu/menu.c — sole production file; 38 added lines, two load guards/publications and private transient rendering/expiry.
2. tests/kit_load/run.py — new actual-production-body RAM/menu and UI tests.
3. tests/kit_load/README.md — reproduction and mock/hardware limitations.
4. tests/kit_load/menu.sha256 — pins the reviewed menu source for the silent-autosave boundary check.
5. tests/live_storage/run.py — supplies the new private dependencies while retaining the unchanged generic-error tests/assertions.
6. tests/morph_range/run.py — retains exact Morph input/global/display-function and datatype checks instead of rejecting every unrelated menu-source change. Interpolation/serialization and exhaustive functional tests remain.
7. tests/silent_autosave/run.py — uses the new reviewed menu digest; all original non-menu audio/storage/IRQ hashes remain unchanged.

Diff: 149 insertions, 5 deletions across seven files. No public API/header changes. CRLF preserved in menu.c. `git -c core.whitespace=cr-at-eol diff --check` passed.

The focused static check compares the entire Load/Save handler to baseline after substituting back only the two changed load cases and removing notice cancellation. Save/Pattern/Song/Settings/Project bodies must remain exact. Existing simple-page success renderer, generic SD error producer/renderer and menu_tick() body are also compared exactly. No old success-page timing was changed.

## Previous failed test and corrected fixture

The complete prior audit was read from:
https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-manual-feedback-audit-stop/docs/codex-berichte/2026-10-28_manual-feedback-audit-stop.md

That broad candidate's artificial expiry assertion failed because its memcpy interception never ran against its strcpy/memset renderer. This patch uses fixed-size memcpy row writes; the fixture intercepts an **actual executed production memcpy**, advances mocked time and runs the real menu tick. It retains the safety assertions that expiry clears the notice, requests a clean repaint and removes stale text. No assertion was relaxed to accept stale text.

During new fixture compilation, the preset_touchTimer mock initially used uint8_t instead of its declared PresetTimer argument type. The compiler rejected the conflict. The fixture signature was corrected to the real header declaration; no production logic or safety assertion was changed. All final runs passed; no required safety assertion failed in this task.

## Commands and results

Prepared PATH in each validation shell:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

Tool versions: CMake 3.31.10, Ninja 1.13.0.git.kitware.jobserver-pipe-1, Python 3.12.14, ARM GCC 14.2.1 20241119. Native suites use ASan/UBSan where supported, fatal sanitizer errors and LeakSanitizer disabled in the traced executor. They are not physical audio/SD/USB tests. Full firmware translation units are covered by ARM builds.

All final commands below returned exit 0:

| Command | Exit |
|---|---:|
| `cmake --version` | 0 |
| `ninja --version` | 0 |
| `python3 --version` | 0 |
| `arm-none-eabi-gcc --version` | 0 |
| `python3 tests/kit_load/run.py` | 0 |
| `python3 tests/bug001/run.py` | 0 |
| `python3 tests/euclidean/run.py` | 0 |
| `python3 tests/safety/run.py` | 0 |
| `python3 tests/usb_allocator/run.py` | 0 |
| `python3 tests/sd_storage/run.py` | 0 |
| `python3 tests/sd_hal/run.py` | 0 |
| `python3 tests/fatfs_integrity/run.py` | 0 |
| `python3 tests/save_errors/run.py` | 0 |
| `python3 tests/live_storage/run.py` | 0 |
| `python3 tests/silent_autosave/run.py` | 0 |
| `python3 tests/project_storage/run.py` | 0 |
| `python3 tests/project_transaction/run.py` | 0 |
| `python3 tests/morph_range/run.py` | 0 |
| `cmake --preset release` | 0 |
| `cmake --build --preset release --parallel 4` | 0 |
| `cmake --preset debug` | 0 |
| `cmake --build --preset debug --parallel 4` | 0 |
| `python3 tests/euclidean/check_images.py build/release build/debug` | 0 |

The final focused suite was rerun after strengthening the publication mock to assert that source RAM bytes/destination pointer were already changed when confirmation was requested. It passed again. All 14 suites passed; no relevant suite was skipped. Diagnostics-ON builds were not requested/run for this narrow patch; the default diagnostics OFF setting is unchanged.

### Relevant exact host output

```text
$ python3 tests/kit_load/run.py
PASS actual notification/tick/repaint bodies: queued/busy display, 1999/2000ms and wrap, latest error, all result codes, expiry/copy interleaving, page/RAM independence, no storage unlock or blocking calls
PASS both actual labels, 749/750ms and wrap, repeated notices, current-page restoration, actual memcpy/IRQ-expiry interleaving and unchanged legacy errors
PASS 512 actual RAM load/menu cases: source copy/destination pointer, invalid indices no action/confirmation, no SD access
```

```text
$ python3 tests/bug001/run.py
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence
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
```

```text
$ python3 tests/safety/run.py
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state
```

```text
$ python3 tests/usb_allocator/run.py
PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing
PASS USB failures: 14 rollback/retention/retry scenarios plus invalid requests; no allocator calls
Host object: size=552 alignment=8 (target size/alignment checked separately in ELF)
PASS complete production USB class object: no heap/allocator-lock references
```

```text
$ python3 tests/sd_storage/run.py
PASS all 1..128 sector read/write counts; exact range edge and 5000ms HAL argument
PASS finite readiness, permanent busy deadline, tick wrap success/timeout; persistent reboot-only fault
PASS BSP read/write/init failure, ambiguous write blocks subsequent I/O; removal before/during transfer and reinsertion cannot clear latch
PASS CTRL_SYNC ready/delayed/timeout/fault and explicit BSP status error; status cannot clear or bypass fault
PASS invalid pointers/counts/LUN/overflowing ranges/ioctls; invalid input does not poison valid operation
PASS IRQ/masked/suspended tick rejection; no reset/remount/retry dependencies in linked driver
```

```text
$ python3 tests/sd_hal/run.py
PASS actual R1/R2/R3/R6/R7/CMD0 bodies: success, missing/active response, hardware timeout, CRC exception, index mismatch, wrap, pre-tick, zero/huge timeout, bounded STOP
PASS actual voltage negotiation: ready, command failure, bounded never-ready and tick wrap
PASS actual HAL read/write: timeout/error bits, retained context/non-ready state, direct retry rejection, FIFO single/multiblock success and failed STOP without replay/reset
PASS actual BSP/HAL status: error sentinel, busy/ready/error distinction, historical error preservation and HAL_TIMEOUT propagation
```

```text
$ python3 tests/fatfs_integrity/run.py
Configuration: configured
PASS actual sync_window: 1..4 FATs, all primary/later failures, stop ordering, dirty retention, no internal replay; non-FAT/clean windows
PASS actual sync_fs: exact FSInfo layout, write/error/pending flag, explicit retry distinction, no-update/other FAT, CTRL_SYNC and no success masking
PASS whole public sync/close through S1: mirror failure latches driver, first FR_DISK_ERR then invalid-object close, no extra physical writes
PASS full production f_sync/f_close: primary/mirror/FSInfo/data/sync errors stay sticky, modified state and handle retained, no implicit replay with healthy stub; success closes normally
PASS whole generic disk/registration/S1: successful/failed/cache status, absent then available, protected, healthy unlink/relink and all invalid drive bytes; serious SD latch survives init/remount with no physical replay
PASS actual f_mount: valid RAM FAT16 geometry, healthy repeated mount/cache status, absent then available without fabricated initialization
PASS actual f_read and read-only file close: payload unchanged, no metadata writes
Configuration: readonly
PASS whole generic disk/registration/S1: successful/failed/cache status, absent then available, protected, healthy unlink/relink and all invalid drive bytes; serious SD latch survives init/remount with no physical replay
PASS actual f_mount: valid RAM FAT16 geometry, healthy repeated mount/cache status, absent then available without fabricated initialization
PASS actual f_read and read-only file close: payload unchanged, no metadata writes
```

```text
$ python3 tests/save_errors/run.py
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, mapped Morph source/mid/near-target weights (full endpoint tested separately), identical resolved /tmp targets/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, silent controlled background retries; no UI/hardware execution
PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held
```

```text
$ python3 tests/live_storage/run.py
PASS actual notification/tick/repaint bodies: queued/busy display, 1999/2000ms and wrap, latest error, all result codes, expiry/copy interleaving, page/RAM independence, no storage unlock or blocking calls
PASS complete production FatFs: nine missing-path/invalid-name/existing-file/readonly-target/full-root failures make zero disk writes, preserve all RAM media/valid file bytes, then explicit create/write/close succeeds
```

```text
$ python3 tests/silent_autosave/run.py
PASS static boundary: manual feedback/audio/IRQ/S1 sources (only Morph dtype differs) and edit hook unchanged; autosave helpers have no waits, allocation, notifications, polling or loops
PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header
PASS 1536 byte-for-byte S3 oracle saves: eight variants, all 64 slots, mapped Morph source/mid/near-target weights (full endpoint tested separately), identical resolved /tmp targets/write counts and balanced timer hooks
PASS 2544 injected write/short-write positions: every payload call, full-count errors, open/close failures, first-error preservation, no payload/close/reuse/cleanup after quarantine
PASS actual menu case/button bodies and timer service: success-only state, write/close/RAM error UI, silent controlled background retries; no UI/hardware execution
PASS S4-L: 936 recoverable open failures then 104 successful explicit saves; no accumulated latch/data loss; ambiguous TIMEOUT and all payload failures still held
PASS silent recovery: 216 safe failed opens then 24 tenth-attempt successes; independent types; 2/4/8/16/32s cap and tick wrap
PASS edits/cooldown independence: 4000 edit/tick calls cause no storm; current RAM retained; kit/pattern deadlines separate; manual save does not clear pending backups
PASS production completion logic: same-tick edits during payload and exclusive store survive; reservation-loss cooldown; explicit pattern-start cancellation preserved; no ISR edit hook added
PASS unsafe/check-required holds: NOT_READY and ownership/mount failures do not loop; partial/write/close errors never replay; serious latches remain; completed explicit save re-arms only a healthy check-required hold
PASS complete production FatFs path resolution: actual autosave paths stay under /tmp while CWD=/proj; current directory, project entries and existing kit/pattern bytes preserved
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
```

```text
$ python3 tests/morph_range/run.py
PASS 8388608 exhaustive 8-bit endpoint-pair/interior combinations, monotonicity and <=1 LSB ideal error; exact endpoints, midpoint pair and all invalid amounts
PASS both actual Morph pages: encoder/knob 0..127, 8000 endpoint movements, normal/edit display, saturated direct legacy/invalid requests and bounded sweeps
PASS 128 actual non-identical-kit sweeps: full transition, source routing retained and every reserved slot excluded
PASS actual automation table excludes Morph; 256 held-step edit/disarm restorations preserve the control without recording a Morph target; no MIDI/global mapping added
Parameter boundaries: END_OF_SOUND_PARAMETERS=247 PAR_MORPH=248 PAR_BEGINNING_OF_GLOBALS=272 NUM_PARAMS=302
PASS 128 actual RAM/SD Morph Kit save/load amounts (including exact target), mod-target exceptions and unchanged 255-byte layout; 256 runtime/legacy amounts absent from temp persistence; legacy sound bytes >127 retained
PASS Morph amount is not serialized or an automation target; serializers/loaders, MIDI/NRPN guards and UI feedback remain unchanged
```

The new RAM fixture executes 512 cases (both types at every byte-valued slot), real extracted loaders/menu branches, source-copy/destination selection, invalid no-action/no-success, correct publication ordering and zero SD calls. Sound-service and send routines are mocks. UI tests exercise actual display/tick/repaint bodies with mocked LCD/time: both labels, success text, 749/750ms, UINT32 wrap, ten repeats per kind, current-page restoration, and expiry during a real copy. Whole legacy generic-error tests still pass.

The original FatFs fixture emits its existing unused lun parameter warning. No production warning was suppressed or unrelated test changed to obtain a pass.

## Firmware images and resources

Image checker command above validated both normal Release and Debug: 16-byte header, eight-byte LXRV2IMG magic, little-endian payload length, checksum `0xff-(sum(payload)&0xff)`, zero header padding, exact .bin payload correspondence, aligned plausible initial MSP, Thumb reset vector in the application and all ALLOC/LOAD sections inside configured regions. Application origin remains 0x08008000; bootloader's first 32 KiB remain excluded. No bootloader/updater/configuration source changed. Firmware version remains 1.7.

```text
$ python3 tests/euclidean/check_images.py build/release build/debug
{"checksum": "0x7A", "configuration": "release", "elf_sha256": "da484cf0ea8fce5b6f8c8ddd29f77953735eb96ca6080bf38d1fdc9ccd3c5b1e", "image_bytes": 288676, "image_sha256": "1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 235796, "ITCM_RAM": 15016, "RAM": 274648}, "payload_bytes": 288660, "reset_vector": "0x080154E1", "result": "PASS"}
{"checksum": "0x24", "configuration": "debug", "elf_sha256": "d1a3f94999733c6b7aff399ff2178a501888ca4cf0d72b514e6b3772b87e7b46", "image_bytes": 288724, "image_sha256": "2a61f4c41277cbc75e7143a08e867704069f1d6b7ef64e543e3ede58373f6320", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 235844, "ITCM_RAM": 15016, "RAM": 274648}, "payload_bytes": 288708, "reset_vector": "0x080154E1", "result": "PASS"}

EXIT 0

```

Normal Release artifact: build/release/LXR_V2_V1.7.img, **288676 bytes**, SHA-256 **1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877**. Debug image is 288724 bytes, SHA-256 2a61f4c41277cbc75e7143a08e867704069f1d6b7ef64e543e3ede58373f6320. These are local build identities, not installation packages. No generated binaries are included in Git publication.

| Measure | Approved baseline Release | New Release | Delta | Capacity / remaining |
|---|---:|---:|---:|---|
| ITCM | 15016 | 15016 | 0 | 16384 / 1368 |
| DTCM | 107392 | 107392 | 0 | 131072 / 23680 |
| General SRAM static high-water | 274640 | 274648 | +8 | 393216 / 118568 |
| Complete flash load span (.bin) | 288820 | 288660 | -160 | 1015808 / 727148 |
| Linker FLASH summary | 236808 | 236648 | -160 | Not the complete load span |

Debug: ITCM 15016, DTCM 107392, SRAM 274648, linker FLASH 236696, full load span 288708. All regions fit. Baseline values are from the preserved R3 build record; current values are from executed builds, not estimated scalar sizes. Static SRAM includes existing fixed heap reserve; dynamic stack/heap usage is unmeasured.

The smaller Flash result is a compiler optimization/layout outcome, not a claim that an added feature must cost negative code: temporary same-flags compilation shows menu_tick machine-code section shrinking from 676 to 304 bytes after inlining decisions changed, while menu_repaint grows 2144 to 2244 and command handler 1784 to 1840. Compiler-generated size or smaller frame is not proof of lower ISR latency.

Temporary baseline/current Release -fstack-usage builds using the actual menu compile command reported menu_repaint 32→32 bytes, menu_handleLoadSaveMenu 32→32, menu_tick 24→16. Private helpers inline in this optimized build. These are per-function static frames, not complete nested/worst-case stack measurements. No linker/heap/stack reservations changed.

Actual Release object hashes remain byte-identical to recorded baseline for Soundengine.c (d6ee7b60ad6a2a1924e096b3631b8f38284349e3ad7c037c348e5ca3e7f17aca), stm32f7xx_it.c (01463848afda54efbbeea116af34c9ff35db325406b42b003432ee46fba5c468) and audio-DMA implementation cs4344_cs5343.c (10472838499bb9806639fc82153ba880797c3c496ac48e10e6724aeaa2e12ac5). The IRQ object still calls the changed menu display service, so byte identity does not imply zero display-path runtime overhead.

## Limitations and recommended acceptance

CODE/BUILD: confirmations follow valid, completed synchronous RAM loads. No SD persistence is implied. All other command/feedback bodies remain unchanged, silent autosave and fault latches are preserved, and all software regressions pass.

OPEN: physical LCD duration/cadence, encoder/button responsiveness under load, audio glitches and peak ISR latency were not measured. The existing display subsystem must continue servicing refreshes for pixels to restore on time. No successful build proves hardware audio continuity.

Recommended separately authorized hardware check: on disposable/test data, manually load several normal and Morph destination slots; verify exact two-line text, approximately 750ms visible lifetime, current-page restoration, navigation/editing during the notice and repeated loads. Listen while sequencing; check Save Kit/Morph Kit and Pattern/Song/Settings/Project screens remain as before and autosave stays silent. Do not perform destructive SD or deliberate power-loss tests for this UI patch.

No extra functionality or broad feedback redesign is recommended. Review the small source diff and this report before authorizing any hardware deployment. The earlier broad candidate remains safely stashed for reference, not part of this implementation.

## Production diff

```diff
diff --git a/Core/Src/menu/menu.c b/Core/Src/menu/menu.c
index 0715d15..ee14d26 100644
--- a/Core/Src/menu/menu.c
+++ b/Core/Src/menu/menu.c
@@ -671,6 +671,11 @@ uint8_t menu_loadedPresetNr[NUM_CURRENT];
 
 uint32_t sucessTimer = 0;
 /* Notification only: no page/input/transport state is changed. Latest error wins. */
+/* Only the two synchronous manual RAM kit loads publish this notice. */
+static volatile uint8_t menu_kitLoadNotice;
+static volatile uint32_t menu_kitLoadSince;
+static void menu_showKitLoad(uint8_t morph);
+static uint8_t menu_renderKitLoad(void);
 static volatile uint8_t menu_sdError;
 static volatile uint8_t menu_sdErrorHeld;
 static volatile uint8_t menu_sdErrorAuto;
@@ -1818,6 +1823,7 @@ void menu_repaintGeneric()
 // repaint the screen
 void menu_repaint()
 {
+    if(menu_renderKitLoad()) return;
     if(menu_renderSdError()) return;
 	if(screensaver_Active) return;
 
@@ -1933,9 +1939,35 @@ void menu_handleSaveScreenKnobValue(uint8_t potNr, uint8_t value)
 
 }
 //-----------------------------------------------------------------
+static void menu_showKitLoad(uint8_t morph)
+{
+    menu_kitLoadSince = HAL_GetTick();
+    menu_kitLoadNotice = morph ? 2 : 1;
+    menu_repaintAll();
+}
+static uint8_t menu_renderKitLoad(void)
+{
+    uint8_t notice = menu_kitLoadNotice;
+    if(!notice) return 0;
+    if((uint32_t)(HAL_GetTick()-menu_kitLoadSince) >= 750u) {
+        menu_kitLoadNotice = 0;
+        return 0;
+    }
+    visibleCursor = 0;
+    memcpy(editDisplayBuffer[0], notice == 2 ? "Load Morph Kit  " : "Load Kit        ", 16);
+    memcpy(editDisplayBuffer[1], "success!        ", 16);
+    display_needs_refresh = 1;
+    /* Expiry may preempt a foreground copy; queue restoration in that case. */
+    if(menu_kitLoadNotice != notice) menu_repaintAll();
+    return 1;
+}
 /* Rendered by the existing bounded display refresh, never by a wait loop. */
 static void menu_expireSdError(void)
 {
+    if(menu_kitLoadNotice && (uint32_t)(HAL_GetTick()-menu_kitLoadSince) >= 750u) {
+        menu_kitLoadNotice = 0;
+        menu_repaintAll();
+    }
     if(menu_sdError != FR_OK && (uint32_t)(HAL_GetTick() - menu_sdErrorSince) >= 2000u) {
         menu_sdError = FR_OK;
         menu_repaintAll();
@@ -1979,6 +2011,8 @@ void menu_showSdError(uint8_t res)
 //-----------------------------------------------------------------
 void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 {
+    /* A new explicit command supersedes only the transient load notice. */
+    if(btnClicked) menu_kitLoadNotice = 0;
 	//this is a special case because the load/save page differs from all the other pages
 	// when this is called, edit mode will have already been set or cleared. this is only
 	// called when something has changed
@@ -2190,9 +2224,11 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 				switch(menu_saveOptions.category) {
 
 				case SAVE_TYPE_KIT:
+                    if(menu_selectedPresetNr[SAVE_TYPE_KIT] >= NUM_KIT_SLOTS) return;
 #if 1
 
 					preset_loadDrumsetRam(menu_selectedPresetNr[menu_saveOptions.category],0, 0);
+                    menu_showKitLoad(0);
 					menu_TargetVoiceGapIndex = getModTargetGapIndex(activeParameters[PAR_TARGET_LFO1 + menu_activeVoice]);
 
 					menu_loadedPresetNr[CURRENT_KIT] = menu_selectedPresetNr[menu_saveOptions.category];
@@ -2214,8 +2250,10 @@ void menu_handleLoadSaveMenu(int8_t inc, uint8_t btnClicked)
 					break;
 
 				case SAVE_TYPE_MORPH:
+                    if(menu_selectedPresetNr[SAVE_TYPE_MORPH] >= NUM_KIT_SLOTS) return;
 #if 1
 					preset_loadDrumsetRam(menu_selectedPresetNr[menu_saveOptions.category],1, 0);
+                    menu_showKitLoad(1);
 					activeParameters[PAR_MORPH_KIT_DEST] = menu_selectedPresetNr[menu_saveOptions.category];
 					menu_loadedPresetNr[CURRENT_MORPH] = menu_selectedPresetNr[menu_saveOptions.category];
 #else

```
