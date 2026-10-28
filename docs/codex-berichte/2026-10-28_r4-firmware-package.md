# LXR Lab R4 — Verified Release Firmware Package

Date: 2026-10-28 UTC. Repository: DocLXR-02/lxr-desktop-app.

## Result and authorization boundary

**PASS for package preparation and review. NOT FLASHED — NOT HARDWARE VALIDATED.** All 14 relevant host suites passed, a clean normal Release rebuild passed, the complete application image passed validation and its SHA-256 exactly matches the recorded implementation image. The downloaded GitHub ZIP was checked again after publication. No production source, tests, test expectations, build configuration or firmware behavior was modified during R4.

Only the already implemented two manual RAM-load confirmations distinguish the approved source from R3: “Load Kit / success!” and “Load Morph Kit / success!”, with a 750ms software expiry. This task adds no functionality. BUG001 Filter Drive/Morph guards, full-range Morph 0–127, storage transactions, all other Load/Save feedback and silent autosave are preserved. The earlier hardware confirmation is user-reported evidence about prior revisions; R4 itself has not been physically tested.

A verified package is not authorization to install it. Daniel must separately approve this exact image and a controlled session with validated backups, separate test media and a recovery plan.

## Revisions, worktree and publication

- Approved source branch: **ui/kit-load-confirmation**.
- Exact source/build commit: **c870f7249d7ee0d65423bc8b7bfdb422dcc74482**.
- Sole parent / R3 source: **83c5ec164d6c3dab439deb2f32a8edf1138b5b77**.
- Source tree: b89db2323a1e399e583ccb724a38adf29ce75e18.
- Source URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/c870f7249d7ee0d65423bc8b7bfdb422dcc74482
- Delivery branch: **delivery/lxr-lab-r4-kit-load-2026-10-28**.
- Delivery commit: **eea1949f0641e375f8cc6082971242549aeed1d1**, based directly on the approved source, adding only delivery/LXR_LAB_R4_KIT_LOAD_CONFIRMATION.zip and delivery/PACKAGE_SHA256SUMS.txt.
- Delivery branch URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/delivery/lxr-lab-r4-kit-load-2026-10-28
- Report-only branch: **codex-reports/2026-10-28-r4-firmware-package**.
- Report base: unchanged lab-setup, 5829398cea5d337200393757e39a37d4d8148256.
- Report path: docs/codex-berichte/2026-10-28_r4-firmware-package.md.

AGENTS.md was read completely. Existing checkout /workspace/lxr-desktop-app remained on the approved source branch; it was clean before building/packaging. No new worktree, reset, stash apply/pop, merge or force push was performed. Git HEAD/parent and the remote source branch head matched the requested identities. No older delivery branch or main was used as firmware source.

The unapproved broad feedback candidate remains untouched in local stash **be96d66a51e7cfab3268bbd25d16d89652a495c7**. That identity was checked before and during packaging and again at completion. It was not included in the package source. Main, lab-setup, existing source/report/delivery branches and previous artifacts remain unchanged; final publication checks compare all captured pre-existing remote heads.

The report is published separately as Markdown only. Its commit is obtainable from GitHub history; its contents do not attempt a self-referential report commit identifier. No GitHub Release was created.

## Scope/behavior verification

The approved source diff against R3 has seven files: sole production file Core/Src/menu/menu.c plus six focused test/documentation files. Production adds 38 lines only: private notice state around :675, repaint integration :1826, queue/render helpers :1942/:1948, expiry :1967, superseding notice cancellation on a new explicit menu action :2015, and the two guarded manual RAM load cases :2227/:2253. No broad feedback API or Song/Pattern behavior was imported.

The existing preset_loadDrumsetRam() in Core/Src/Preset/presetManager.c:808 accepts valid slots 0–63; normal loading copies sound bytes into active RAM, Morph loading selects the destination pointer. The confirmation is requested after the synchronous RAM operation returns. Invalid slots do not produce success. Tests check the actual RAM state at publication, not just the displayed labels.

The actual Load/Save handler is compared against R3 with only those two cases and notice cancellation permitted to differ. Existing success-page renderer, generic-error producer/renderer and menu_tick body remain exact. A menu digest preserves the reviewed UI boundary while original audio/storage/IRQ hashes remain checked. The new focused suite tests expiry during a real intercepted production memcpy, retaining assertions that expired text is repainted away; the broad candidate's invalid strcpy/memcpy fixture was not reused as proof.

CODE: no new SD call, allocation, busy loop, transport stop, audio/DMA instrumentation, priority change, DWT reset or automatic recovery is present. OPEN: 750ms is the software eligibility deadline; physical LCD cadence, peak display-ISR timing and audible continuity remain unmeasured for R4. No hardware guarantees are inferred from host execution.

## Environment and exact executed commands

Prepared environment activated in validation shell:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

Versions queried with exit 0: CMake 3.31.10, Ninja 1.13.0.git.kitware.jobserver-pipe-1, Python 3.12.14, ARM GCC 14.2.1 20241119. Cloud runtime readiness/network policy were checked through the available cloud-environment skill/tools; no credential values were printed or inspected.

All commands below returned exit 0. The runner stopped on any nonzero result; none occurred:

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
| `cmake --build --preset release --target clean` | 0 |
| `cmake --build --preset release --parallel 4` | 0 |
| `python3 tests/euclidean/check_images.py build/release` | 0 |

The clean target removed only generated Release outputs, then the complete Release firmware rebuilt. Diagnostics remained OFF in the actual cache/compile definitions. No Debug/diagnostics image was included or newly built for R4; R4 specifically packages normal Release. Build targets print updater instructions but no installation procedure was executed.

Native tests use ASan/UBSan where supported; LeakSanitizer remains disabled under the traced executor. Actual production bodies/tables or full FatFs code are used with mocked time, sound sends, HAL/registers, LCD and RAM block media as documented by each suite. These tests are not physical SD, USB or audio tests.

### Exact relevant test output

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

The focused load suite covers all 512 kind/byte-valued-slot cases, completed-RAM-state-before-publication, invalid no-op/no-success, zero SD access, both exact labels, 749/750ms, tick wraparound, repeated operations, current-page restoration and actual copy/expiry interleaving. Existing generic error behavior remains tested. BUG001 parser/transport guards and exhaustive Morph mapping remain covered by their unchanged functional suites.

## Independent image verification

The established image checker was rerun. A temporary read-only verifier also checked the actual cache, preprocessed main.c using its actual compile command, inspected every ALLOC VMA and LOAD LMA, and compared the entire image payload to .bin. Its expected source/size/hash were set for the approved R4 image; no repository test expectation was changed.

```json
{
  "source_commit": "c870f7249d7ee0d65423bc8b7bfdb422dcc74482",
  "source_branch": "ui/kit-load-confirmation",
  "git_tree": "b89db2323a1e399e583ccb724a38adf29ce75e18",
  "configuration": "Release",
  "diagnostics": false,
  "update_bootloader": 0,
  "bootloader_inputs_configured": false,
  "filename": "LXR_V2_V1.7.img",
  "image_bytes": 288676,
  "image_sha256": "1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877",
  "magic": "LXRV2IMG",
  "header_hex": "4c58525632494d47946704007a000000",
  "payload_bytes": 288660,
  "checksum": "0x7a",
  "payload_matches_bin": true,
  "bin_sha256": "60afb51d6b8b53ebce3036a5b4feee9c75bd0a1eb7177c000b233943809b927f",
  "elf_sha256": "da484cf0ea8fce5b6f8c8ddd29f77953735eb96ca6080bf38d1fdc9ccd3c5b1e",
  "initial_msp": "0x20080000",
  "reset_vector": "0x80154e1",
  "application_origin": "0x8008000",
  "highest_flash_load_exclusive": "0x804e794",
  "flash_headroom": 727148,
  "memory_section_high_water": {
    "ITCM": 15016,
    "DTCM": 107392,
    "RAM": 274648,
    "FLASH": 235796
  },
  "tracked_worktree_clean": true
}
```

All safety gates passed:

- Intended STM32F765 Desktop target/configuration remains unchanged. Payload is for the existing compatible Desktop SD updater, not a full-device programmer.
- 16-byte header: LXRV2IMG, little-endian payload length, one-byte checksum, three zero padding bytes.
- Image is exactly 288676 bytes: header plus **288660-byte** binary payload.
- Header checksum is 0x7A and equals `0xff-(sum(payload)&0xff)`.
- Image payload equals the complete LXR_V2_STM32F765.bin byte for byte.
- .isr_vector LMA is 0x08008000. All loadable bytes start at/after that address and fit the 992KiB application region through 0x08100000. Highest load end matches the full binary span. First 32KiB bootloader region is excluded.
- Effective UPDATE_BOOTLOADER=0; no bootloader directory/HEX input is configured. No bootloader/startup/linker/clock/updater changes, option-byte operations or flashing utilities were used.
- Initial MSP 0x20080000 is aligned at permitted general-SRAM top; reset vector 0x080154E1 is Thumb and inside the application. Startup's DTCM stack arrangement is unchanged.
- Every allocated section fits its linker-defined RAM/TCM/Flash region. These checks establish format and source/build compatibility, not the installed bootloader version, physical write-protection state or guaranteed hardware recovery.

Image SHA-256 **1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877** matches the previous implementation report exactly. No discrepancy or substitute image was accepted. Source version remains 1.7; manufacturer public v1.71 is a recovery reference, not claimed binary-equivalent.

## Resources and unresolved timing

Baseline R3 figures are from its preserved verified build record; R4 figures are from the clean build and ELF. Toolchain/configuration match.

| Measure | Capacity | R3 | R4 | Delta | R4 remaining |
|---|---:|---:|---:|---:|---:|
| ITCM | 16384 | 15016 | 15016 | 0 | 1368 |
| DTCM static high-water | 131072 | 107392 | 107392 | 0 | 23680 |
| General SRAM static high-water | 393216 | 274640 | 274648 | +8 | 118568 |
| Full Flash load span | 1015808 | 288820 | 288660 | -160 | 727148 |
| Linker FLASH summary | 1015808 | 236808 | 236648 | -160 | 779160 (summary only) |

VMA-only FLASH high-water is 235796 bytes; initialized TCM/load images make the complete load span larger. The full payload span, not the linker summary, is the conservative flash occupancy. Static SRAM includes existing reservations; no runtime stack high-water, dynamic heap use or actual CPU load was measured. The -160-byte Flash result is the same compiler inlining/layout outcome recorded in implementation validation; it does not prove lower latency. R4 adds no code beyond that approved source.

Remaining practical risk is unmeasured target behavior, including display-service preemption cost, physical notice lifetime and audio/MIDI responsiveness while loading. No successful compile establishes glitch-free operation. No new claim of stack or thermal safety is made.

## Delivery and download verification

Immutable direct ZIP URL:
https://raw.githubusercontent.com/DocLXR-02/lxr-desktop-app/eea1949f0641e375f8cc6082971242549aeed1d1/delivery/LXR_LAB_R4_KIT_LOAD_CONFIRMATION.zip

```json
{
  "image_name": "LXR_V2_V1.7.img",
  "image_bytes": 288676,
  "image_sha256": "1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877",
  "zip_name": "LXR_LAB_R4_KIT_LOAD_CONFIRMATION.zip",
  "zip_bytes": 209245,
  "zip_sha256": "c83290aad78ffa469cd65cec009273c6c29ba029877274d3f59ee44373411a73"
}
```

The archive contains exactly:

1. LXR_V2_V1.7.img — normal Release updater image, original required filename retained.
2. VERIFICATION.txt — English provenance, hardware scope, image identity, precautions, proposed tests and rollback notes.
3. SHA256SUMS.txt — SHA-256 of the image and verification notes.

A separate delivery/PACKAGE_SHA256SUMS.txt checks the ZIP. No debug/second firmware, ELF, BIN, HEX or bootloader image is included. Deterministic ZIP member timestamps and fixed names were used. Source/license links are included (LICENSE and NOTICE.md, GPL-3.0-only terms).

Anonymous curl --fail --location downloaded the immutable R4 URL with TLS verification: **HTTP 200, 209245 bytes, ssl_verify_result=0, exit 0**. Downloaded bytes exactly matched the local archive. ZIP CRC, exact member list, inner checksums and approved image hash were checked after download. This is verified GitHub delivery, not an actual iPhone-device test. Safari/Files can download/extract the ZIP; a trusted checksum tool/computer should verify the image before approved installation. Do not install the ZIP itself or trust filename alone.

## R3 rollback preservation

R3 immutable ZIP was independently downloaded again: HTTP 200, 208846 bytes, TLS verified. Its ZIP SHA-256 remains **7fb5fe0849940579d7e486eca33ea7cd734c4d4599726ebf015db6f2d8725bf2**. Its inner image SHA remains **adb30f63c6c35080611f9d749bd7ea329e9f8c67f0bf61f024d77294fdec1642**, image size 288836 bytes. R3 delivery branch/commit and all earlier artifacts are preserved.

https://raw.githubusercontent.com/DocLXR-02/lxr-desktop-app/510dc2ad5fa79435864c36e0e4ab45d8ca8f53dc/delivery/LXR_LAB_R3_MORPH_0-127.zip

Both revisions use the same firmware filename. Keep extracted images in separate labeled folders and verify their content hashes when selecting one. R3 is deliberately not bundled as a second ambiguous firmware image in R4.

## Proposed controlled hardware acceptance and recovery — not executed

Before any installation: explicit approval for this exact R4 SHA, verified independent original-card backup, separate prepared test SD, verified R3 rollback image and known manufacturer recovery image. Do not modify the user's original card.

- Check first boot, controls, audio, sequencer and basic MIDI/USB. Stop on resets/freezes/unusual audio.
- Manually Load Kit / Load Morph Kit; confirm exact two-line text, approximately 750ms visible lifetime, current display restoration, repeated actions and navigation/editing during the notice.
- Verify all other Load/Save feedback remains as before and autosave remains silent. Use disposable data; no deliberate power-loss tests.
- Recheck Morph 0/63/64/127 on both pages, identical/different kits, Drive 0/127, stationary/moving knob and endpoint over-turns. Listen for regressions; host tests do not prove absence of clicks.
- Retain R3 for return to the prior firmware. After separate approval, use the established Desktop SD updater with only the intended verified image eligible on spare media: image at card root, hold encoder while powering on until LXR Updater, wait for Update OK, never interrupt an active update. These are notes, not commands executed by this task.
- If updater does not appear/complete, stop and seek manufacturer support. Do not attempt full-device flashing, bootloader repair or option-byte changes. Recovery depends on a working compatible bootloader; no guarantee is made for every fault.

Manufacturer public v1.71 recovery reference: https://www.ericasynths.lv/drum-synthesizer-lxr-02-1132/ . Obtain and verify it beforehand; it is not included here. Do not confuse private test firmware or repository version 1.7 with established binary equivalence.

## Errors and final status

No required regression, build, identity or image safety gate failed. Existing native FatFs compilation emits the pre-existing unused lun warning; it was not suppressed by source/test edits. Temporary verification/package scripts and generated outputs remained outside tracked firmware source. Debug/diagnostics variants were not rerun because this task packages only the established normal Release.

The source worktree remained clean and the broad-candidate stash identity stayed unchanged. Only the isolated delivery additions and this separate Markdown report were published. No merge, force push, physical SD-card access, hardware test, bootloader change or flashing occurred.

**Ready for package review and separately user-authorized controlled hardware testing. NOT FLASHED — NOT HARDWARE VALIDATED.**
