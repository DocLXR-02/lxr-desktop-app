# R3 — Verified Morph 0–127 Firmware Packaging

Date: 2026-10-28 UTC. Repository: DocLXR-02/lxr-desktop-app.

## Decision and boundaries

**PASS for package preparation and review. NOT FLASHED — NOT HARDWARE VALIDATED.** All 13 host suites and a clean normal Release rebuild passed; the image exactly matches the approved implementation report SHA-256. No source, tests, build configuration or expectations were modified. No physical hardware or SD card was accessed. No main/lab-setup/implementation branch was changed, merged or force-pushed. Previous delivery artifacts remain intact. No GitHub Release was created.

This is packaging, not installation approval. Daniel must separately authorize the exact image and controlled hardware session after checking backups and recovery prerequisites. The user's hardware-tested-baseline description is external evidence; no hardware performance was measured here.

## Exact revisions and instructions

- Source branch: controls/morph-range-127.
- Approved and actual clean HEAD: **83c5ec164d6c3dab439deb2f32a8edf1138b5b77**.
- Sole parent / previous user-reported hardware-validated baseline: **5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527**.
- Source tree: b2fe39851f16490fef27f1daf84a55c1322b8fd7.
- Source URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/83c5ec164d6c3dab439deb2f32a8edf1138b5b77
- Delivery branch: delivery/lxr-lab-r3-morph-127-2026-10-28.
- Delivery commit: **510dc2ad5fa79435864c36e0e4ab45d8ca8f53dc**, based directly on the approved source, adding only delivery/LXR_LAB_R3_MORPH_0-127.zip and delivery/PACKAGE_SHA256SUMS.txt.
- Report-only branch: codex-reports/2026-10-28-r3-morph-firmware-package.
- Report base: lab-setup, 5829398cea5d337200393757e39a37d4d8148256.
- Report path: docs/codex-berichte/2026-10-28_r3-morph-firmware-package.md.

AGENTS.md was read completely from lab-setup. Existing checkout /workspace/lxr-desktop-app was used without a new worktree. Its source ref and parent were checked with Git; the current remote source head was checked through GitHub refs. The independent report publication contains no implementation changes. Remote branch heads and downloaded contents are verified before completion is claimed.

## Independent production diff review

The approved commit changes only two production files:

- Core/Src/menu/menu.c:513: PAR_MORPH datatype DTYPE_0B255 becomes DTYPE_0B127, using existing knob/encoder clamps and display formatting on both PERFORMANCE and EUCLIDEAN pages.
- Core/Src/Preset/presetManager.c:1143: interpolation returns destination for x>=127, otherwise uses fraction `(x*256+63)/127` and the original eight-bit fixed-point rounding. At :2079 preset_morph stores a saturated 0–127 amount and retains the same sweep scheduling.

The other nine files in the approved commit are focused tests/docs and independent old-math save-oracle support. R3 does not edit them. Full approved production diff inspected:

```diff
diff --git a/Core/Src/Preset/presetManager.c b/Core/Src/Preset/presetManager.c
index 0b171c3..af496f1 100644
--- a/Core/Src/Preset/presetManager.c
+++ b/Core/Src/Preset/presetManager.c
@@ -1138,11 +1138,13 @@ error:
 #endif
 }
 //----------------------------------------------------
-//val is interpolation value between 0 and 255
-//uses bankers rounding
+// Morph control is 0..127; retain the existing 8-bit fractional precision
+// and intermediate rounding. The destination endpoint is exact.
 static uint8_t interpolate(uint8_t a, uint8_t b, uint8_t x)
 {
-	uint16_t fixedPointValue = (uint16_t)(((a*256) + (b-a)*x));
+	if(x >= 127) return b; // Saturate invalid internal amounts as well.
+	const uint16_t fraction = ((uint16_t)x * 256u + 63u) / 127u;
+	uint16_t fixedPointValue = (uint16_t)((a*256) + (b-a)*fraction);
 	uint8_t result = (uint8_t)(fixedPointValue/256);
 
 	return (uint8_t)((fixedPointValue&0xff) < 0x7f ? result : result+1);
@@ -2076,6 +2078,8 @@ void preset_morph_tick()
 // to the back
 void preset_morph(uint8_t morph)
 {
+	// Runtime-only control: no kit, pattern or project field is rescaled.
+	activeParameters[PAR_MORPH] = morph > 127 ? 127 : morph;
 #if 1
 	//initialize new morph
 	morph_state = 1; //start morph
diff --git a/Core/Src/menu/menu.c b/Core/Src/menu/menu.c
index 737cf6a..0715d15 100644
--- a/Core/Src/menu/menu.c
+++ b/Core/Src/menu/menu.c
@@ -510,7 +510,7 @@ const enum Datatypes PROGMEM parameter_dtypes[NUM_PARAMS] = {
 
 
 	    /*PAR_ROLL*/ 			DTYPE_MENU | (MENU_ROLL_RATES<<8),
-	    /*PAR_MORPH*/ 			DTYPE_0B255,
+	    /*PAR_MORPH*/ 			DTYPE_0B127,
 	    /*PAR_ACTIVE_STEP */ 	DTYPE_0B127,							//230
 	    /*PAR_STEP_VOLUME*/ 	DTYPE_0B127,
 	    /*PAR_STEP_PROB*/ 		DTYPE_0B100,

```

CODE: zero is exact source, 127 is exact destination for morphable parameters; 63/64 straddle the midpoint. Existing routing and target-selection exceptions remain. PAR_MORPH=248 is outside sound serialization [0,247) and globals [272,302); no stored legacy amount requires migration. Baked Morph Kits retain sound bytes. No file format change is present. BUG001 reserved-slot and CC0/CC2/NRPN guards remain unchanged. DSP, audio/interrupt scheduling, USB, feedback, silent autosave and storage transactions are unchanged.

## Environment and reproducible commands

Cloud runtime skill was used to check enforced network policy and current readiness. No credential values were inspected. Toolchain PATH in the shell:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

Tool versions: CMake 3.31.10; Ninja 1.13.0.git.kitware.jobserver-pipe-1; Python 3.12.14; ARM GCC 14.2.1 20241119. These match the prepared implementation/R2 environment.

Every command below returned exit 0:

| Command | Exit |
|---|---:|
| `cmake --version` | 0 |
| `ninja --version` | 0 |
| `python3 --version` | 0 |
| `arm-none-eabi-gcc --version` | 0 |
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

Cleaning removed only generated Release outputs. The complete Release firmware was rebuilt, not merely accepted as an existing binary. Runtime diagnostics remained OFF in the actual cache/compile configuration; bootloader inputs were empty. No Debug/diagnostics rebuild was required for R3; those variants' previous results are not represented as new R3 tests.

### Actual host results

Production bodies/tables are used where specified by each suite; hardware/register/filesystem effects are mocked. Whole FatFs tests run actual filesystem code against RAM block media. ASan/UBSan remain active where supported; LeakSanitizer is disabled under the traced executor. These are not physical USB/SD/audio tests.

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

## Independent image safety gates

Existing checker plus a temporary verifier derived from the earlier app-image verifier were executed. The expected source/hash/size were changed only in that temporary verification script, not in repository tests. main.c was preprocessed using its actual Release compile command to check effective UPDATE_BOOTLOADER=0. Every ALLOC VMA and LOAD LMA was checked against configured regions and the exact binary span. All assertions passed.

```json
{
  "source_commit": "83c5ec164d6c3dab439deb2f32a8edf1138b5b77",
  "source_branch": "controls/morph-range-127",
  "git_tree": "b2fe39851f16490fef27f1daf84a55c1322b8fd7",
  "configuration": "Release",
  "diagnostics": false,
  "update_bootloader": 0,
  "bootloader_inputs_configured": false,
  "filename": "LXR_V2_V1.7.img",
  "image_bytes": 288836,
  "image_sha256": "adb30f63c6c35080611f9d749bd7ea329e9f8c67f0bf61f024d77294fdec1642",
  "magic": "LXRV2IMG",
  "header_hex": "4c58525632494d47346804009f000000",
  "payload_bytes": 288820,
  "checksum": "0x9f",
  "payload_matches_bin": true,
  "bin_sha256": "7fae85357c7d038cf3d860187b5584676ab890a856b5b9656309d5c6ced78a95",
  "elf_sha256": "fee80aff6ea1357f3c7435527a472a1246b3f009ca60bdfd1b93c72b3d376d52",
  "initial_msp": "0x20080000",
  "reset_vector": "0x80154e1",
  "application_origin": "0x8008000",
  "highest_flash_load_exclusive": "0x804e834",
  "flash_headroom": 726988,
  "memory_section_high_water": {
    "ITCM": 15016,
    "DTCM": 107392,
    "RAM": 274640,
    "FLASH": 235956
  },
  "tracked_worktree_clean": true
}
```

Header bytes are `4c58525632494d47346804009f000000`: LXRV2IMG, little-endian 288820-byte payload, checksum 0x9f and three zero padding bytes. Image has exactly 16 header bytes plus the .bin payload, byte-identical to the full generated binary. Checksum is `0xff-(sum(payload)&0xff)`.

The .isr_vector load address is 0x08008000. All loadable sections fit application flash through exclusive end 0x0804E834. Thus the first 32 KiB bootloader region is excluded. No bootloader input, full-device target, option-byte operation or flashing utility was used. Initial MSP=0x20080000 is aligned at general SRAM top; reset=0x080154E1 is Thumb and within application flash. Existing startup subsequently uses DTCM for its runtime stack. These are plausibility/source checks, not an inspection of physical protection state or proof of every runtime pointer.

## Resource comparison and timing risk

Baseline measurements come from the preserved R2 build record for exact source 5ab5e6b and its known image, not a fresh baseline rebuild in R3. Current measurements are from the clean R3 ELF/map. Same prepared compiler/configuration makes the comparison useful.

| Measure | Capacity | Baseline | R3 | Delta | R3 remaining |
|---|---:|---:|---:|---:|---:|
| ITCM | 16384 | 15016 | 15016 | 0 | 1368 |
| DTCM static high-water | 131072 | 107392 | 107392 | 0 | 23680 |
| General SRAM high-water including minimum heap reserve | 393216 | 274640 | 274640 | 0 | 118576 |
| Full Flash load span | 1015808 | 288596 | 288820 | +224 | 726988 |
| Linker FLASH summary | 1015808 | 236584 | 236808 | +224 | 779000 (not full load span) |

SRAM consists of initialized data, BSS and fixed heap reservation; these are static bounds, not measured dynamic heap or stack use. Linker FLASH summary and VMA-only FLASH high-water 235956 exclude some TCM load images; the complete payload span is the authoritative conservative flash bound. Linker minimum heap/stack reservations remain 512/1024 bytes. No runtime stack high-water or CPU timing was measured.

CODE: the extra constant-divisor interpolation arithmetic runs in the existing foreground Morph parameter sweep/getter; it adds no per-sample DSP, ISR hook, SD access or allocation. OPEN: actual peak foreground latency, audio clicks and performance under Morph plus demanding synthesis still require physical observation. Prior static stack analysis showed getter frame +4 bytes, not a worst-case stack proof; no new stack measurement is claimed here. ITCM headroom remains limited but unchanged.

## Package and verified delivery

Direct immutable ZIP URL:
https://raw.githubusercontent.com/DocLXR-02/lxr-desktop-app/510dc2ad5fa79435864c36e0e4ab45d8ca8f53dc/delivery/LXR_LAB_R3_MORPH_0-127.zip

Delivery branch:
https://github.com/DocLXR-02/lxr-desktop-app/tree/delivery/lxr-lab-r3-morph-127-2026-10-28

```json
{
  "zip_bytes": 208846,
  "zip_sha256": "7fb5fe0849940579d7e486eca33ea7cd734c4d4599726ebf015db6f2d8725bf2",
  "image_bytes": 288836,
  "image_sha256": "adb30f63c6c35080611f9d749bd7ea329e9f8c67f0bf61f024d77294fdec1642"
}
```

ZIP contains exactly one firmware .img: LXR_V2_V1.7.img, plus VERIFICATION.txt and SHA256SUMS.txt. Original updater filename is preserved. VERIFICATION.txt is concise English ASCII with CRLF for common readers. Inner SHA256SUMS checks image and verification text; outer PACKAGE_SHA256SUMS checks ZIP. ZIP timestamps are fixed for reproducible packaging. No ELF/BIN/HEX/bootloader is included. No workflow/source/build modification was needed; a new isolated delivery branch was used.

The immutable raw GitHub URL was anonymously downloaded with curl --fail --location, TLS verification enabled: HTTP 200, 208846 bytes, TLS verification result 0. Download bytes exactly matched the local ZIP. ZIP CRC, exact member list, both inner hashes and approved firmware SHA were independently rechecked after download. This establishes delivery availability from the cloud, not an actual iPhone-device test. In Safari download the ZIP; tap it in Files to extract, then verify SHA using a trusted tool/computer. Do not install the ZIP itself or infer validity from a filename alone.

The previous delivery/lxr-lab-r1-2026-10-28 and its artifacts were not overwritten. Baseline image is 288612 bytes, SHA-256 00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76. Both old and new firmware use the same .img filename; keep them in separate clearly labeled folders and verify content hashes.

## Conservative manual test and rollback plan — proposed, not executed

PREREQUISITES: Separate prepared test SD, verified independent backup of
original card, verified old firmware and manufacturer recovery image.
Do not touch the original card. Installation requires Daniel's separate
explicit approval for this exact image. Package preparation is not approval.

PROPOSED SHORT TEST (NOT EXECUTED):
1. After separately approved installation, check boot, controls, audio,
   sequencing and basic MIDI/USB. Stop on freezes, resets or unusual audio.
2. Both PERFORMANCE and EUCLIDEAN Morph pages: check 0,63,64,127;
   source at 0, destination at 127, midpoint transition at 63/64.
3. Empty project, Voice 1 Bass Kick, no LFO/automation. Identical kits,
   Drive 0 then 127: stationary vs swept Morph, all relevant filters,
   repeated outward turns at both endpoints. Repeat with different kits.
4. Check no new clicks or lost Drive; editing and transport stay responsive.
   Retain disposable data only; do not perform power-cut experiments.
5. Return to the old verified firmware and confirm boot/audio operation.

ROLLBACK: Keep the earlier R2 package intact, outside this ZIP:
https://github.com/DocLXR-02/lxr-desktop-app/tree/delivery/lxr-lab-r1-2026-10-28
Old LXR_V2_V1.7.img: 288612 bytes; SHA-256:
00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76
After separate approval, use the established Desktop SD updater procedure
with ONLY the verified old image eligible on spare test media (same filename).
Documented procedure: image at card root; hold encoder while powering on,
wait for LXR Updater and Update OK. Never interrupt an active update.
If updater does not appear or complete, stop; do not attempt full-device
flashing, option-byte changes or bootloader repair. Seek manufacturer support.
Manufacturer public v1.71 is the independent recovery reference:
https://www.ericasynths.lv/drum-synthesizer-lxr-02-1132/
Obtain/verify it beforehand; it is not included. Source 1.7 is not claimed
binary-equivalent to v1.71. Recovery depends on a working compatible bootloader.
Do not promise recovery from every fault. No automatic rollback is provided.

## Errors, limitations and final recommendation

No safety gate failed. The native FatFs suite emits the existing unused lun parameter warning; no test was weakened. The clean Release build completed without new warnings/errors. All file writes in R3 were temporary build/package/report data, not firmware source. No physical rollback was attempted, and no recovery guarantee is made for an inoperative bootloader, media corruption or electrical faults. Manufacturer v1.71 is the public recovery reference; source version 1.7 is not claimed equivalent.

Recommended next action: review this manifest and checklist, independently retain a verified baseline/recovery image and original-card backup, then request Daniel's explicit authorization for a controlled session on separate test media. Stop on new clicks, lost Drive, freezes, resets or updater anomalies. Do not perform deliberate power-loss tests in the first session.

No feature development or additional speculative safety phase is proposed by this packaging task. The package is prepared for review; no device was flashed and no hardware validation is claimed.
