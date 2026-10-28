# LXR Firmware Lab — Phase R1: Final Safety Review and First Hardware Test Preparation

Date: **2026-10-28 UTC**. Repository: **DocLXR-02/lxr-desktop-app**.

## 1. Decision

**GO FOR CONTROLLED HARDWARE TEST PREPARATION.**

The focused source/build review found **no blocking defect for the proposed desktop SD-image test path**, under the documented isolated-fault and ordered/atomic-sector simulation model. This is permission to prepare a controlled test, **not authorization to install or operate experimental firmware**. Daniel must explicitly approve installation after the prerequisites below have been satisfied.

All twelve existing regression suites passed again. Release and Debug were rebuilt from clean outputs with diagnostics both OFF and ON; all four builds and complete image checks passed. Firmware hashes exactly reproduce the S4b-2 handoff. No source, test, build configuration or implementation branch was modified.

One important artifact restriction emerged from independent binary inspection: **use only the validated SD `.img` for the proposed installation**. Although actual application sections, `.bin`, `.hex` and `.img` exclude the bootloader region, the ELF's first `PT_LOAD` segment includes a linker-alignment/header prefix at `0x08000000`. ELF/probe loading is excluded from this GO; no programmer behavior was tested. This does not alter the correct SD-image payload.

**NOT FLASHED — NOT HARDWARE VALIDATED.** No physical LXR, SD card, protection setting, bootloader, reset or flashing utility was operated. No firmware binaries are published with this report. No additional implementation phase is recommended by this review.

## 2. Exact scope, source and methodology

| Item | Exact reference |
|---|---|
| Reviewed implementation branch | `safety/sd-project-transaction-s4b2` |
| Reviewed remote/local commit | **`5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527`** |
| Reviewed Git tree | `aca77103ba2509431cc966febc0ec6468defac6a` |
| Immediate source parent / S4b-1 | `e49bb028462c339c4998cd08a845db17e1fc778f` |
| Mandatory instruction/report base | `lab-setup`, `5829398cea5d337200393757e39a37d4d8148256` |
| Preserved main | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Complete S4b-2 report commit | `4f019a9f3cc5b433f9d026d7975c8beb9864658e` |
| S4b-2 report branch | `codex-reports/2026-10-27-phase-s4b2-project-replacement` |
| S4b-2 report file | `docs/codex-berichte/2026-10-27_phase-s4b2-project-replacement.md` |
| New report-only branch | `codex-reports/2026-10-28-phase-r1-hardware-readiness` |
| New report file | `docs/codex-berichte/2026-10-28_phase-r1-hardware-readiness.md` |

Reviewed source: https://github.com/DocLXR-02/lxr-desktop-app/commit/5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527

Previous complete report: https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-27-phase-s4b2-project-replacement/docs/codex-berichte/2026-10-27_phase-s4b2-project-replacement.md

The existing `/workspace/lxr-desktop-app` checkout was clean at the requested commit before review. Remote source and report heads matched exactly. No worktree or implementation branch was created or changed. The complete `AGENTS.md` from lab-setup was read (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`), the complete S4b-2 report was fetched, and the preceding design/source lineage was consulted where necessary.

This is a fresh source-first review, not an assertion that prior tests prove safety or that a separate human certification occurred. Inspection followed ownership and destructive-operation preconditions, then compared code with the published claims and tests. All native suites and clean ARM builds were independently re-executed. No new production tests or speculative features were written. Auxiliary read-only analysis scripts/logs live in `/tmp/phase-r1`; build outputs remain ignored in `build/`.

Evidence labels:

- **CODE:** confirmed in the exact inspected production source.
- **BUILD/HOST:** confirmed by commands run in this review; simulations and mocks are identified.
- **OPEN:** prerequisite or behavior requiring physical observation. It is not a measured defect or implicit request for a new development phase.

## 3. Independent project-storage findings

All implementation line references below refer to commit `5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527`.

### 3.1 Ownership and bounded staging

**CODE:** `Core/Src/Preset/projectStorage.inc:6` fixes exactly one `/LXRSTG`, one `/LXRBAK` and one `/LXRTRANS.DAT` globally. Normal projects remain `/proj00` through `/proj63`. Names fit FAT 8.3. No permanent generations, background scan or general transaction database exists.

`preset_txnPrepare()` (`:342`) rejects unowned stage/backup paths when no record exists; it refuses a record for another slot (`FR_LOCKED`). `preset_txnRead()` (`:224`) validates schema, slot, lengths, reserved bytes and CRCs. The immutable32-byte header records the previous directory root/count/digest before stage creation; the16-byte footer seals the complete new directory. An incomplete footer does not authorize publication. A corrupt header does not authorize cleanup. These checks establish a bounded local ownership protocol, **not protection against a malicious external writer forging the record**; exclusive card ownership is an explicit assumption.

The validator (`:146`) requires the exact194-file new-project profile, reads all bytes through a fixed512-byte buffer, verifies file sizes, current pattern header/length/associated-kit ranges and expected short filenames, and obtains a framed CRC32. The old-project profile deliberately supports valid legacy/missing optional files while validating every present file; it is not an indiscriminate requirement to migrate old projects. The CRC fingerprints readable structurally valid content; it is not an independent expected-output checksum, authentication or full-volume integrity check. This limitation does not invalidate the checked serializers/readback contract for the first controlled test.

`preset_txnSpace()` (`:407`) checks space for a full replacement before creating a record. Existing data is not deleted to make room. Ordinary full-space rejection remains retryable. Free-space accounting is a preflight, not a reservation or physical durability guarantee.

### 3.2 Previous-version preservation and publication order

**CODE:** `preset_txnRun()` (`:425`) validates the existing canonical project, writes/closes/readbacks the ownership header, creates an absent stage, executes existing checked serializers, validates the entire stage and seals the result **before** any rename of the canonical project.

Decisive sequence (`:451` onward):

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
```

After publication it re-reads directory IDs, requires the validated new root at the canonical name, an absent stage, the expected backup root, and a correctly typed/sized canonical `prj.nfo`. It avoids a second full read because this same-parent directory rename does not rewrite child payloads. No cleanup of the old backup is authorized before that confirmation. A failure returns immediately; it is not converted to success.

`Core/Src/Preset/presetManager.c:2172` writes globals, then64 kit/pattern/song triplets, then `prj.nfo` last using the existing checked serializers. Output remains194 files /255,398 payload bytes, with unchanged kit/pattern/song/settings formats. `preset_saveProject()` (`:2197`) preserves first errors, runs temporary-file cleanup only after a successful transaction, and returns cleanup/CWD restoration errors honestly. Failed operations do not falsely clear autosave dirty flags or discard the working RAM project.

### 3.3 FAT rename and aliases

**CODE:** Actual `Middlewares/Third_Party/FatFs/src/ff.c:4724` registers/copies a destination directory entry, removes the original entry, then synchronizes (`:4809`). These operations can touch different sectors. Rename is not assumed atomic. Both publication renames use the same root parent, avoiding a changed `..` directory parent field, but this does not make the entry updates transactional.

`preset_txnPrepare()` (`projectStorage.inc:357`) rejects duplicate nonzero directory-root identities before any normalization/cleanup:

```c
if((c && (c == s || c == b)) || (s && s == b)) return FR_INT_ERR;
```

This is essential: `f_unlink()` frees a directory/file chain, so deleting a directory alias could destroy data still reachable through the other name. The code does not attempt such repair. `preset_txnRun()` also checks expected distinct roots/stage disappearance after publication. **HOST:** real FatFs16/32 sector-cut tests reproduce8 alias states; those remain readable and reject project writes/deletion.

### 3.4 Cleanup safety and “only valid copy” invariant

`preset_txnRemove()` (`:317`) inventories a flat owned directory, closes DIR, verifies its expected root and rejects unknown files, duplicate slot names or subdirectories before unlinking anything. It is not recursive. There is no call to the old destructive `empty_directory(currentFolder)` or `delete_node()` in the project save path.

The destructive-call authorization cases were traced independently:

| Case authorizing cleanup | Complete version that must already remain |
|---|---|
| Delete a stage after interrupted replacement with old canonical still valid | Matching old canonical, verified by root/count/CRC |
| Delete redundant backup after publication | Validated new directory under canonical name, expected ID confirmed |
| Resume partial backup cleanup on a later explicit Save | Full new canonical must still match sealed root/count/CRC |
| First-ever save with incomplete stage, no old/canonical/backup | No previously complete project existed; cleanup only if strict validation confirms it is incomplete and inventory is recognized |
| Sole complete first-save stage | It is selected, revalidated and sealed/published before another Save; it is not discarded as incomplete |
| Unknown contents, unexpected root or duplicate alias | No destructive cleanup authorized |

**Conclusion within the documented normal failure model:** no inspected path deletes the only **validated** complete project. The old directory is untouched during staging. Publication makes a complete new directory available before old-copy deletion. Explicit normalization validates a surviving reference before deleting redundant contents. Partial cleanup does not make an incomplete backup count as complete because the immutable old digest/count still apply.

This is not a proof against arbitrary torn FAT sectors, shared-chain corruption, external media mutation, undetected payload corruption after validation or all possible card-controller failures. Those are the same documented limits, not reasons to invent a new recovery architecture for the first test.

### 3.5 Recovery selection, startup and honest errors

`preset_txnChoose()` (`:285`) prefers a fully matching new canonical, then the matching old canonical/backup, then sealed stage. A strictly complete, owned, unsealed first-save stage is available when no older copy ever existed. If validation cannot identify a complete reference, the load fails rather than choosing incomplete data. Communication errors are not treated as ordinary absent files.

`preset_txnLoadFolder()` (`:480`) is read-only. Interrupted loads skip `/proj.idx` writes and temporary cleanup/creation/overlay (`preset_loadProject()`, `presetManager.c:2297`). Main does not create `/proj00` in a rename gap or create default globals during a pending transaction (`Core/Src/main.c:447`); it uses RAM defaults where needed. `:882` skips stale tmp overlays during recovery, including failed recovery. There is no boot-time rename, automatic repair or payload replay.

Only a later **explicit Save** can normalize a clearly owned non-alias transaction. It first restores/publishes a validated reference if canonical is absent, confirms root IDs/source absence, and only then removes redundant contents and writes current RAM. Another slot's pending transaction remains an honest lock, not an unbounded second transaction.

Serious mutating errors retain existing shared ownership protection (`preset_txnMutation()`, `:26`), while logical precondition failures remain retryable. S1 physical faults cannot be cleared by ordinary status/init/remount; ambiguous write/short-write/close failures remain held. A cleanup error is reported even if a new project has already been published. Dirty RAM is retained; a later safe retry saves current RAM, not a stale stage mislabeled as current data.

Loading still applies individual files incrementally: an actual later read failure can leave partially changed RAM, with an error. Individual temporary/named file replacement remains direct rather than transactional. Neither is hidden by the GO decision; the first session uses disposable data and stops on storage errors instead of testing speculative recovery.

### 3.6 S4b-1 memory-safety protections

`preset_projectPath()` (`presetManager.c:174`) requires capacity for `/proj00` plus NUL and slot0–63. `preset_childPath()` (`:196`) bounds the folder/name scan, validates a single FAT8.3 component and requires capacity before writing. Root strings are not appended to `currentFolder[13]`; maximum children use21-byte buffers. `preset_txnKey()` returns only0–193 and rejects invalid/duplicate names before copying into `[194][13]`. Streaming prefix/tail access is guarded by size/version/length validation; invalid sizes abort before data/index processing.

Project save/load and individual pattern/song/kit indices retain the S4b-1 range guards. Metadata and songs require complete reads, associated-kit indices are bounded, and checked writes/required closes remain in place. Existing sanitized all-byte-value/path/short-read regressions pass again. No new recursive delete or unchecked child-path write was found.

## 4. Audio, real-time and live-performance review

**CODE:** Project I/O and autosave attempts run in foreground. `preset_processTimer()` is called from the main loop (`Core/Src/main.c:980`), not from audio/DMA callbacks. The existing manual Save/load paths start the existing TIM2 rendering mechanism around synchronous storage; no SD servicing is recursively injected into rendering.

`TIM2_IRQHandler()` (`Core/Src/stm32f7xx_it.c:272`) calls sequencer tick, block rendering and UART processing, with optional fixed diagnostic hooks. `HAL_I2S_TxCpltCallback()` (`Core/Src/Soundengine/cs4344_cs5343.c:87`) handles readiness, buffer selection and one HAL rearm per stream. No transaction, FatFs, file formatting, SD export, heap call, new lock or background scan was added there. No new per-sample computation, DMA layout, priority or clock change was found.

The audio config remains64 stereo frames per DMA transfer (`Core/Inc/Soundengine/config.h:31`), with requested44.1kHz I2S. Existing PLL/prescaler assumptions yield about44.140625kHz, so the nominal64-frame interval is approximately1.450ms /313,181 CPU cycles at216MHz. This is a calculated configuration threshold, not a measured deadline, CPU utilization or actual crystal frequency. No runtime sample-rate/timing result is claimed.

Diagnostics remain OFF by default (`CMakeLists.txt:72`). Disabled hook arguments are not evaluated and the HAL rearm still executes once (`Core/Inc/safety/runtimeDiagnostics.h:36`). Enabled metrics use fixed saturating counters, Flash helpers and DWT cycle differences; no logging/allocation/file I/O is added. `Core/Src/safety/runtimeDiagnostics.c:5` enables DWT without resetting CYCCNT, preserving the shared LCD/delay timebase. Existing enabled instrumentation consumes72 more ITCM bytes and72 SRAM bytes versus OFF; its real overhead is unmeasured.

**BUILD:** seven Release relocatable objects, recompiled against the S4b-1 source/header, remain byte-for-byte identical: IRQ dispatch, soundengine, I2S/DMA, sequencer, S1 disk driver, heap and diagnostics implementation. The only ISR changes since lab-setup are the previously approved default-OFF diagnostic hooks. GPIO/DMA/I2S/timer setup and priorities compare unchanged to lab-setup, as detailed in the source audit appendix. All DSPAudio/filter source files remain unchanged.

Silent autosave retains its normal2s inactivity delay and safe per-type2/4/8/16/32s bounded retry cooldown. Payload/close/physical ambiguity is not replayed automatically. New edits do not force a tight retry loop. Manual errors use queued expiry-based feedback (`menu.c:1970`), not a notification delay; no new UI phase was implemented. These errors do not deliberately stop transport, audio or RAM editing.

**OPEN / material test limits:** synchronous FAT traversal, validation and SD commands can delay foreground UI/MIDI/USB handling and add bus/cache interference even while audio interrupts run. The old display-buffer wait remains. No cloud test establishes glitch-free audio, worst-case render time or MIDI losslessness during Save. The existing error counter in the I2S callback counts observed not-ready state; it is not a complete underrun monitor. Enabled nominal-budget/long-interval counters are observations, not proof of audible underruns or precise CPU load.

Fault handlers still loop indefinitely (`stm32f7xx_it.c:112` onward); no new fault reporting/reset behavior is present and no active watchdog initialization was found. A freeze may require operator recovery; no automatic-watchdog recovery is assumed. This is disclosed for the first-test stop procedure, not an unrequested instrumentation development task.

## 5. Bootloader, target and image safety

### 5.1 Application configuration

**CODE:** `cmake/stm32f765.cmake:19` selects Cortex-M7/Thumb/FPv5-D16/hard-float and `STM32F765xx`, matching the STM32F765VG desktop project. `STM32F765VGTX_FLASH.ld:38` defines ITCM16KiB, DTCM128KiB, general RAM384KiB and application Flash992KiB beginning **0x08008000**. Startup uses the bootloader-compatible vector MSP then moves runtime SP to the existing DTCM stack. Vectors, linker layout and hardware clock setup were not changed by the integrated corrections.

Exact comparisons since lab-setup passed for startup, linker script, MCU flags, image-generation CMake, system/VTOR source and HAL MSP initialization. Separate function comparisons confirm unchanged `SystemClock_Config`, GPIO, DMA, SDMMC, I2S2/3 and TIM2 setup. `UPDATE_BOOTLOADER` remains **0** at `Core/Src/main.c:17`; disabled updater branches are not activated. Bootloader configuration paths are empty in all four CMake caches, and no full-device target was built. Physical protection/option-byte state is unknown; source exclusion does not establish device write protection.

### 5.2 Actual images and vectors

**BUILD:** all four files contain16-byte headers: eight-byte `LXRV2IMG`, LE32 payload length, one-byte checksum and three zero padding bytes. Payload exactly equals the complete generated `.bin`. Checksum is `0xFF - (sum(payload) & 0xFF)`. All ALLOC sections fit exactly one linker region; all LOAD section LMAs are within application Flash and the maximum LMA end equals the full binary span. Initial MSP **0x20080000**, reset vector **0x080154E1**, Thumb bit set and within application code, in all four builds. Payload lengths are divisible by4, matching the published bootloader's word writer.

| Configuration | Payload `.bin` | `.img` bytes | Checksum | Full flash-span headroom |
|---|---:|---:|---|---:|
| Release OFF | 288,596 | **288,612** | BB | 727,212 |
| Debug OFF | 288,644 | 288,660 | BD | 727,164 |
| Release ON | 289,036 | 289,052 | 4D | 726,772 |
| Debug ON | 289,084 | 289,100 | DA | 726,724 |

The first-test candidate is specifically **Release, diagnostics OFF**, local path `build/release/LXR_V2_V1.7.img`, SHA-256:

```text
00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76
```

It is recorded here for review/identification, **not published or authorized for installation**. Source version1.7 is not evidence of equivalence to official v1.71 or private support-test v1.72. The version screen alone cannot distinguish these lab changes; retain commit and image hash in test notes.

### 5.3 ELF segment caveat — SD image only

Additional `readelf -l`/direct ELF-header inspection found the first ELF `PT_LOAD` segment starts at file offset0 and physical/virtual **0x08000000**, alignment0x10000. Its file span includes ELF headers/padding before the actual `.isr_vector` at0x08008000. This is an artifact distinction missed by a section-only boot-region check. A segment-based programming tool could treat that prefix differently from `objcopy`'s section-derived binary.

The `.bin`/`.img` contain no such prefix; every HEX data record was independently checked to start at or above0x08008000 and end within Flash. This GO covers **only the checked SD `.img` through the device updater**. Do not load the ELF, raw binary, HEX, `flash`, `flash_full`, or any probe/programmer target for the first session. Actual behavior of those programming tools was neither executed nor validated. No linker/bootloader modification is required to keep this planned SD path separate, and none was made.

### 5.4 Published bootloader and manufacturer procedure

Primary sources actually consulted on2026-10-28:

1. [Manufacturer product/download page](https://www.ericasynths.lv/drum-synthesizer-lxr-02-1132/) still lists desktop **v1.71**, dated2025-05-23, as the newest public download.
2. [Manufacturer v1.7 update instructions](https://www.ericasynths.lv/news/lxr17) describe root-card image placement, encoder-held power-up, image/checksum validation and uninterrupted installation.
3. [Manufacturer open-source notice](https://www.ericasynths.lv/news/post/lxropen/) distinguishes supported official firmware from custom code and warns that bootloader damage can require manufacturer repair.
4. Published [desktop bootloader source](https://github.com/erica-synths/lxr-desktop-bootloader/tree/47ed4a437a1b808581447ee01ae24b4af26c8937), pinned commit **`47ed4a437a1b808581447ee01ae24b4af26c8937`**; `Core/Src/main.c` blob `e2b296e474de88e9aa9c713c07cc038e45713bec`, `Core/Inc/ImageFile.h` blob `dd528dff9d119ce3a98aec199f4ab9d0b7430934` and `Core/Inc/flash_if.h` blob `d1a82c9fb4da1ee8bd13d9a5d830ce2e283ea01d`.

Independent source observations in that **published** bootloader:

- Encoder entry at `Core/Src/main.c:205`, updater display at215 and first `LXR*.img` selection at251. Keep only one intended desktop update image at the test-card root.
- Header check at314, payload range/checksum from326; image header structure matches the16-byte application format.
- Active erase path begins at Flash sector1 (`:378`),6 sectors (`:379`), not sector0. This covers the reviewed image's end near0x0804E754. The nearby “1-7” comment is imprecise; no larger image/alternate updater guarantee is inferred from it.
- Writes begin at `APPLICATION_ADDRESS=0x08008000`, in64-byte input chunks and4-byte words; `Core/Src/flash_if.c:85` programs/readchecks words.
- `Update OK!` at482 then a bootloader-controlled system reset; normal jump checks the MSP pattern at499 and sets MSP/VTOR before application entry at531.
- Existing bootloader code also conditionally applies its own sector0 protection/option-byte policy (`main.c:144`). That is not an application change or an operation executed here. Device protection settings were not read or modified.

This published source is **not proven identical to Daniel's installed bootloader**. The latter must be confirmed compatible before installation. Source inspection supports a normal SD application-update/retry path while the existing bootloader remains intact; it does not guarantee recovery after bootloader corruption, hardware failure, unsuitable media or power problems. The published updater has limited read/error verification and a weak additive checksum, so offline full-image validation and copy-back SHA-256 are meaningful prerequisites; do not rely solely on its acceptance screen.

The product page's current manual/firmware download endpoints returned unsupported binary content to the browsing tool; two old indexed PDF URLs returned404. The official update blog and pinned manufacturer bootloader/source documentation were readable. **No official v1.71 binary was obtained or examined in R1.** Earlier user-supplied measurements (273,544 bytes, SHA-256 `188695ea78cd3da0fb6a3e825732bd7fd365d4491a37126003ec513e163e1576`) remain external measurements, not a locally verified recovery image. Obtaining/validating a known manufacturer recovery image is still a prerequisite before flashing.

## 6. Static resources and unmeasured runtime limits

All four rebuilt ELFs/maps match the S4b-2 resource measurements:

| Region/metric | OFF | ON | Capacity | Remaining OFF / ON |
|---|---:|---:|---:|---:|
| ITCM | 15,016 | 15,088 | 16,384 | **1,368 /1,296** |
| DTCM static high-water | 107,392 | 107,392 | 131,072 | **23,680 /23,680** |
| General RAM linker usage | 274,640 | 274,712 | 393,216 | **118,576 /118,504** |
| Release full Flash load span | 288,596 | 289,036 | 1,015,808 | **727,212 /726,772** |

Debug requires48 more Flash bytes than corresponding Release. Release linker-reported FLASH usage is236,584 OFF /236,952 ON; full span is larger because of explicit TCM initializer load addresses. Full span, not just `.text` or a VMA high-water value, was used for conservative image fit. All section boundaries and load addresses pass.

Compared with S4b-1, S4b-2 adds8,328 Flash bytes and3,040 general SRAM bytes, with **zero ITCM or DTCM increase**. The fixed additional filename index is2,522 bytes, validation buffer512, recovery flag1, plus alignment. No dynamic project buffer exists. `_Min_Heap_Size=512` and `_Min_Stack_Size=1024` are unchanged linker minima, not actual worst-case consumption.

Release `-fstack-usage` remeasurement: Save frame256 bytes versus104 at S4b-1; Load160 unchanged; tmp-kit272 unchanged; validator part136; inventory144; ID query104; directory inspection96; seal88; remove56; choose48. Several private functions are inlined. No recursion was introduced. These individual frames do not bound the complete FatFs/formatting/interrupt call chain, lazy FP exception frames or stack high-water. Runtime stack grows down from DTCM top; its23,680-byte static headroom is not an observed runtime margin.

All static regions fit with the prepared compiler. Limited ITCM reserve and unknown worst-case stack/audio timing are practical observation targets for the first session, not evidence of a demonstrated overflow. No new optimization or instrumentation was introduced to manufacture a larger reported margin.

## 7. Bug001 — Filter Drive Morph integration

**CODE/BUILD:** original correction commit **`7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`** is preserved functionally in the integrated revision. Exact source comparisons, not just ancestry assumptions, established:

- Complete `Core/Src/Soundengine/MIDI/MidiParser.c` is byte-identical to Bug001.
- `interpolate`, `preset_isTransportParameter`, `preset_sendDrumsetParametersImmediate`, `preset_morph_tick` and `preset_morph` bodies match the original correction exactly.
- All `Core/Src/Soundengine/DSPAudio` sources, including filter algorithms, are unchanged since lab-setup.

`preset_isTransportParameter()` (`presetManager.c:1154`) excludes reserved NRPN transport and bank slots from immediate kit sweep (`:1206`) and Morph tick (`:2005`). CC0 is rejected before cache subtraction/indexing and modulation notification (`MidiParser.c:171`). NRPN selectors above255 are rejected before narrowing (`:134`); CC2/cache bounds are validated and supported external mute commands bypass inappropriate cache/notification while retaining commands (`:903`). External bank forwarding is still handled by the MIDI parser entry path. Morph restart/scheduling and Drive curves were not changed.

Actual `tests/bug001` runs the complete production parser and extracted real sweep bodies under ASan/UBSan. It checks CC0/no cache or modulation access, all256 Morph values, repeated partial-cycle restart, immediate sweep, Drive parameters, reserved transport exclusion, CC2/NRPN boundaries and external mutes/bank forwarding. All passed. The original mechanism is still guarded; nothing in later storage code bypasses it.

**OPEN:** no-click behavior is an acoustic/hardware result and has not been verified. The first session must reproduce the original controlled Bass Kick/identical-kit/Drive/Morph test; host stub Drive updates do not execute the full physical audio chain or prove that every click source is absent.

## 8. Exact R1 regression/build execution

Environment activation in each shell:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
```

Tools: CMake3.31.10; Ninja1.13.0.git.kitware.jobserver-pipe-1; Python3.12.14; ARM GCC14.2.1 20241119 (15:14.2.rel1-1); native GCC14.2.0. No toolchain upgrade or configuration edit.

| Command | Exit | Result |
|---|---:|---|
| `python3 tests/bug001/run.py` | 0 | Parser/transport/Morph boundary tests pass |
| `python3 tests/euclidean/run.py` | 0 | Six sanitizer groups pass; generation/length/playback/persistence/protocol |
| `python3 tests/safety/run.py` | 0 | Bounded heap and diagnostics OFF/ON logic pass |
| `python3 tests/usb_allocator/run.py` | 0 | 2,000 lifecycle cycles,14 failure scenarios, no heap references |
| `python3 tests/sd_storage/run.py` | 0 | Actual S1 driver128-sector bounds, finite readiness and latch tests |
| `python3 tests/sd_hal/run.py` | 0 | Actual/extracted SD command/HAL/BSP error/timeout tests |
| `python3 tests/fatfs_integrity/run.py` | 0 | Actual FatFs/public sync/close/initialization and S1 integration |
| `python3 tests/save_errors/run.py` | 0 | 1,536 byte-equivalent saves,2,544 write faults and manual result handling |
| `python3 tests/live_storage/run.py` | 0 | Nonblocking feedback and safe logical retries |
| `python3 tests/silent_autosave/run.py` | 0 | Silent bounded retry/dirty-state/failure holds |
| `python3 tests/project_storage/run.py` | 0 | S4b-1 paths, indices, legacy data,38 project failures and whole-FatFs gate |
| `python3 tests/project_transaction/run.py` | 0 | Production/FatFs16/32 replacement,220 fault/edge/interruption cases |

All twelve are actual reruns; no skipped suite or weakened assertion. Native sanitizer tests use actual production functions/complete translation units or explicitly extracted bodies, with hardware dependencies mocked. New transaction suite uses real FatFs/generic disk/registration and a fail-closed RAM-sector driver; it is not a physical SD test or a complete execution of the target HAL. Its16 publication-sector cuts assume atomic ordered512-byte writes;50 logical cuts,118 failure cases and36 edge cases do not cover arbitrary controller failure chains. Forty-two normal new/repeated saves preserve194-file layout and constant allocated-cluster count. Exact final output is appended.

All configure/build commands below exited0; `--clean-first` forced recompilation rather than relying on cached prior outputs:

```sh
cmake --preset release
cmake --build --preset release --parallel 4 --clean-first
cmake --preset debug
cmake --build --preset debug --parallel 4 --clean-first
cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON
cmake --build build/diagnostics-release --parallel 4 --clean-first
cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON
cmake --build build/diagnostics-debug --parallel 4 --clean-first
python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
```

Four version commands,12 suites,8 configure/build commands and the image check total25 logged commands, all exit0. Auxiliary read-only ELF/HEX, all-LMA, protected-source, Bug001 equivalence and object/stack checks also passed. No new compiler error/warning appeared in the ARM builds. The existing host warning is unused `lun` in `ff_gen_drv.c:80`, in configured/read-only FatFs fixtures; tests pass and source was not changed to silence it. Initial case-sensitive/path lookups for unavailable files were corrected during inspection; they were not test failures. Manual download errors are reported in section5.

## 9. First controlled hardware session — proposed procedure for Daniel

**This section is a plan only. None of it was executed. Do not begin installation without Daniel's explicit approval.** Use stable power, low monitoring levels and disposable test content. Do not conduct deliberate power-loss, hot-removal, full-media, fault injection, repair or bootloader experiments in this first session.

### A. Prerequisites — all must be confirmed before approval

1. Identify the desktop LXR-02 and its existing updater/update history from reliable records; confirm the documented encoder/SD procedure applies. The installed private v1.72 and installed bootloader are not assumed equal to public source.
2. Prepare a **separate healthy FAT32 test card** using known-good disposable factory/test data, with ample room for an extra full project. Leave the original card unchanged and out of the instrument during the experiment. Do not bring a pending LXRSTG/LXRBAK/LXRTRANS transaction or the unexplained PROJ00 anomaly onto the test card.
3. Verify an independent backup of the original card, including what can actually be read. The reported invisible/uncopyable PROJ00 and iPhone error-36 mean a partial file copy is **not** evidence of a complete backup. No corruption/repair is established here. If the required backup cannot be verified, stop before installation and retain the original without destructive experiments.
4. Obtain an authentic manufacturer desktop v1.71 recovery image from the official download page, keep it off the test card until needed, and validate/download-hash it. Confirm its size/header/checksum before relying on it. R1 has not obtained this binary; earlier supplied measurements are only a comparison reference. If the private support image is available, archive it separately; returning to public1.71 will not recreate every unknown private1.72 behavior.
5. Identify exactly the **Release/OFF** candidate in section5: commit5ab5e6b1, image288,612 bytes, SHA-25600b75f…79d76. Verify the entire file again after copying it to the test card (copy back/read hash), safely eject, and keep only that one intended `LXR*.img` at root. No EURORACK image, historical multi-image bootloader package, ELF, raw binary or `_full` image.
6. Agree the stop/recovery checklist below and record original firmware/update-screen details, audio routing, MIDI setup and backups. Daniel then explicitly approves the exact image/session. A GO report, repository availability or successful build does not substitute for this approval.

### B. Installation and initial startup

After approval only, follow the [manufacturer's SD/encoder procedure](https://www.ericasynths.lv/news/lxr17): device off, test card inserted, encoder held while powering up; release when updater entry is confirmed. Keep power stable throughout checking/programming. Let a compatible updater reset automatically or follow its explicit completion/reboot prompt; never interrupt installation to match a guessed delay.

Record updater text, completion, displayed firmware version and first boot behavior. Normal version text is1.7 and is insufficient alone to prove the lab revision; retain the verified hash. Stop on a validation/programming error or unexpected startup behavior.

At low output level, verify display, encoder/buttons/faders and page navigation, a known kick/voice, main audio outputs and transport start/stop. Run a simple16-step pattern for several minutes; edit RAM parameters while playing. Check basic DIN note/clock transport and USB MIDI with an already available host, one interface at a time where practical. Avoid an uncontrolled MIDI clock/echo loop. Log freezes, spontaneous resets, missing controls, stuck notes or obvious noise. Do not attempt large project saves until this basic operation is stable.

### C. Bug001 controlled reproduction

Use an empty disposable project with **Bass Kick on Voice1**, four hits across16 steps at fixed tempo. No LFOs, automation or extra FX/modulation. Source and destination kits must be identical; keep them identical in both test variants.

1. Filter OFF control: compare stationary Morph and small/full-range movements, including continued rotation at0 and255.
2. Select an enabled filter mode and Drive0. Hold Morph stationary for a baseline, then make small increments throughout0–255 and full sweeps. Keep turning against both endpoints. Return stationary.
3. Repeat with both identical kits at **Drive127**, maintaining the same gain/output routing. Expect stronger intended Drive character, not transient Drive loss or new clicks tied to knob movement.
4. Repeat relevant enabled filter modes one at a time. Record which modes were actually tested; do not generalize an untested mode. Voices2/3 can be a useful comparison but are not required proof of the Voice1 correction.

Expected result: no reproducible **additional movement-correlated** click, severe disturbance or temporary loss of Drive when morphing identical kits. Knob stationary is a separate baseline. A click or Drive dropout tied to small movements/endpoints is a test failure; preserve the exact settings/audio notes, stop expansion and report. Do not compensate by changing filter code or silently reducing Drive. If recording equipment is already available, capture stationary/moving segments at matched gain. Do not claim bit-identical public/private firmware sound.

### D. Project storage — disposable data only

1. Use a clearly unused test slot (for example10, only after confirming unused). Set distinctive project/kit names, Voice1 sound and a simple pattern. Save Project with transport initially stopped; note completion/error and elapsed time.
2. After activity has settled, perform an ordinary completed-operation reboot. **Explicitly reload that slot through the project-load menu**, then confirm names, pattern hits and sound. A temporary autosave restored at startup alone does not prove canonical project persistence.
3. Change the project to a clearly distinguishable second state (pattern and a sound parameter), save to **the same slot**, reboot normally and explicitly reload. Verify second state, not the first. Repeat once more to check ordinary repeated replacement.
4. Preserve a second disposable project as an unrelated sentinel; confirm it remains readable. After a completed save and safe power-down/eject, inspect the test card on a computer: canonical PROJxx exists with expected files, and `/LXRSTG`, `/LXRBAK`, `/LXRTRANS.DAT` do not accumulate after success. Copy/read the canonical files if practical. Do not rename/delete reserved leftovers as a repair shortcut.
5. Only after stable stopped-transport trials, repeat one normal Save/load during modest playback/RAM use. Listen for discontinuity and watch responsiveness; increase workload conservatively. Record actual elapsed times rather than inventing a fixed universal deadline. Do not assume stopped transport means the audio engine is inactive.
6. Any reported storage failure, leftover transaction, missing data or inability to reload is a failure to investigate, not evidence that a successful-looking display or tmp file makes the save good. Stop storage tests, retain RAM/settings evidence and preserve the card. No intentional interruption or automatic retry experimentation in this session.

### E. Practical performance observations

| Available immediately, no new instrument modification | Requires additional equipment/access and separate agreement |
|---|---|
| Listen at low matched gain for dropouts/clicks during playback, Morph and normal Save | Audio interface/recorder/DAW for waveform and timed comparison, if already available |
| Stopwatch/video for save/load duration and display/control response | Logic/scope observation of existing TIM2 GPIO pulse if safely accessible; no new firmware hook |
| Log freezes, unexpected reboot displays and transport discontinuity | Debugger reads of existing `error` or enabled `lxr_diagnostics`; avoid halt/resume when assessing real-time continuity |
| Basic USB/DIN trigger/clock checks with available equipment | Coherent debugger sampling/cycle calibration if later authorized; no inferred CPU percentage |

Start with diagnostics OFF. There is no implemented diagnostic page or SD exporter. ON builds expose fixed counters only through an appropriate debugger; do not imply they are visible on the LXR display. Render/TIM2 maxima, counts, nominal over-budget observations, DMA not-ready/rearm errors and long intervals have limited meanings. Debugger halting invalidates audio timing, live multiword snapshots are not automatically coherent, and zeros do not prove no underruns. No stack high-water/dynamic RAM measurement exists in current firmware. Initial listening/UI observations are useful but are not worst-case audio/CPU certification. No extra equipment purchase is needed or authorized by R1.

### F. Stop and recovery

- On abnormal audio, lower/mute the **external monitor**, stop further storage commands and log exact screen, last action, project/Drive/Morph settings and whether transport/UI still works. Do not keep retrying uncertain writes, clear fault state, hot-remove the card, remount, format or repair it.
- Never interrupt a progressing firmware update. For explicit updater validation/open/checksum error screens, follow the manufacturer's correction-and-update procedure using a verified card/image; do not guess at a stalled flash-programming operation. If programming status cannot be established, keep power stable and seek manufacturer guidance.
- If an installed application is unusable **after update completion**, the supported recovery route is the existing encoder-held SD updater with the known manufacturer desktop image on a prepared card, following the same verified procedure and separate approval. Keep the experimental `.img` off the recovery card so the updater cannot select it again. Preserve disposable test-card evidence separately. Do not use the original anomalous card to recover.
- That route is conditional on the intact compatible bootloader still entering and reading the card. If it will not enter, or repeatedly reports erase/write failure despite a verified suitable image/media, **stop**; manufacturer support/repair guidance is appropriate. No bootloader overwrite, protection disabling, option-byte editing, OpenOCD, ST-Link/full-flash or guaranteed unbrick procedure is authorized here.

## 10. Remaining uncertainties and final assessment

| Matter | Classification | Effect on first-test preparation |
|---|---|---|
| Transaction order, owned redundant cleanup, bounds and result propagation | CODE + HOST | No blocking defect found under the stated model |
| All12 suites /4 clean builds /4 complete image checks | BUILD | Passed; source remains unchanged |
| First ELF load-segment boot-region prefix | BUILD | Restrict first test to verified SD `.img`; no probe/ELF use |
| Actual installed bootloader/version/protection state | OPEN prerequisite | Confirm compatible procedure before installation; no guaranteed recovery claim |
| Authentic recovery binary locally available/validated | OPEN prerequisite | Must be satisfied before approval; R1 did not download it |
| Complete original-card backup, especially anomalous PROJ00 | OPEN prerequisite | Do not equate partial copying with verified backup |
| Audio clicks, SD latency, UI/MIDI continuity, stack high-water, media behavior | OPEN hardware observations | Test cautiously on disposable data after approval; no source speculation |
| Torn/reordered sectors, arbitrary FAT corruption and physical recovery | Documented model limits | No absolute guarantee; no destructive first-session tests |

The practical next step is to review this report, confirm the checklist and ask Daniel to approve the exact first SD-image installation/session. **No new development phase, speculative recovery feature, U1 feedback change or diagnostic exporter is required by the present findings.** If a controlled test fails, preserve evidence and determine the smallest specific correction from that failure rather than broadening now.

The GO decision is for **preparation only**. It does not certify the entire SD stack, official firmware equivalence, the installed bootloader, or continuous audio under every workload. It does not authorize flashing. No firmware source was changed or new firmware branch published in R1.

## Appendix A. Exact rerun test output and exit codes

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

## Appendix B. Build commands, exit codes and resource output

```json
[
  {
    "index": 0,
    "command": "cmake --version",
    "exit": 0
  },
  {
    "index": 1,
    "command": "ninja --version",
    "exit": 0
  },
  {
    "index": 2,
    "command": "python3 --version",
    "exit": 0
  },
  {
    "index": 3,
    "command": "arm-none-eabi-gcc --version",
    "exit": 0
  },
  {
    "index": 4,
    "command": "python3 tests/bug001/run.py",
    "exit": 0
  },
  {
    "index": 5,
    "command": "python3 tests/euclidean/run.py",
    "exit": 0
  },
  {
    "index": 6,
    "command": "python3 tests/safety/run.py",
    "exit": 0
  },
  {
    "index": 7,
    "command": "python3 tests/usb_allocator/run.py",
    "exit": 0
  },
  {
    "index": 8,
    "command": "python3 tests/sd_storage/run.py",
    "exit": 0
  },
  {
    "index": 9,
    "command": "python3 tests/sd_hal/run.py",
    "exit": 0
  },
  {
    "index": 10,
    "command": "python3 tests/fatfs_integrity/run.py",
    "exit": 0
  },
  {
    "index": 11,
    "command": "python3 tests/save_errors/run.py",
    "exit": 0
  },
  {
    "index": 12,
    "command": "python3 tests/live_storage/run.py",
    "exit": 0
  },
  {
    "index": 13,
    "command": "python3 tests/silent_autosave/run.py",
    "exit": 0
  },
  {
    "index": 14,
    "command": "python3 tests/project_storage/run.py",
    "exit": 0
  },
  {
    "index": 15,
    "command": "python3 tests/project_transaction/run.py",
    "exit": 0
  },
  {
    "index": 16,
    "command": "cmake --preset release",
    "exit": 0
  },
  {
    "index": 17,
    "command": "cmake --build --preset release --parallel 4 --clean-first",
    "exit": 0
  },
  {
    "index": 18,
    "command": "cmake --preset debug",
    "exit": 0
  },
  {
    "index": 19,
    "command": "cmake --build --preset debug --parallel 4 --clean-first",
    "exit": 0
  },
  {
    "index": 20,
    "command": "cmake -S . -B build/diagnostics-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release -DLXR_RUNTIME_DIAGNOSTICS=ON",
    "exit": 0
  },
  {
    "index": 21,
    "command": "cmake --build build/diagnostics-release --parallel 4 --clean-first",
    "exit": 0
  },
  {
    "index": 22,
    "command": "cmake -S . -B build/diagnostics-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug -DLXR_RUNTIME_DIAGNOSTICS=ON",
    "exit": 0
  },
  {
    "index": 23,
    "command": "cmake --build build/diagnostics-debug --parallel 4 --clean-first",
    "exit": 0
  },
  {
    "index": 24,
    "command": "python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug",
    "exit": 0
  }
]
```

```text
$ cmake --build --preset release --parallel 4 --clean-first
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      274640 B       384 KB     69.84%
           FLASH:      236584 B       992 KB     23.29%
   text	   data	    bss	    dec	    hex	filename
 287736	    852	 344176	 632764	  9a7bc	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 288596 bytes, checksum 0xBB
[123/123] Firmware update instructions

EXIT 0
```

```text
$ cmake --build --preset debug --parallel 4 --clean-first
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      274640 B       384 KB     69.84%
           FLASH:      236632 B       992 KB     23.29%
   text	   data	    bss	    dec	    hex	filename
 287784	    852	 344176	 632812	  9a7ec	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 288644 bytes, checksum 0xBD
[123/123] Firmware update instructions

EXIT 0
```

```text
$ cmake --build build/diagnostics-release --parallel 4 --clean-first
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      274712 B       384 KB     69.86%
           FLASH:      236952 B       992 KB     23.33%
   text	   data	    bss	    dec	    hex	filename
 288176	    852	 344248	 633276	  9a9bc	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 289036 bytes, checksum 0x4D
[123/123] Firmware update instructions

EXIT 0
```

```text
$ cmake --build build/diagnostics-debug --parallel 4 --clean-first
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      274712 B       384 KB     69.86%
           FLASH:      237000 B       992 KB     23.33%
   text	   data	    bss	    dec	    hex	filename
 288224	    852	 344248	 633324	  9a9ec	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 289084 bytes, checksum 0xDA
[123/123] Firmware update instructions

EXIT 0
```

## Appendix C. Exact artifact identities

These are local build artifacts only, not download links or installation authorization. Hashes match the prior S4b-2 report exactly.

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

Image/header/vector/section validation output:

```text
$ python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
{"checksum": "0xBB", "configuration": "release", "elf_sha256": "57758e60214c3865d16ccb86303c4346f7a51c268f2c5b77b286daebe4bf6fed", "image_bytes": 288612, "image_sha256": "00b75f737c8f2c3d8beafb51d082a271752cb3ebe3ee9e9e5c6511ec2c079d76", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 235732, "ITCM_RAM": 15016, "RAM": 274640}, "payload_bytes": 288596, "reset_vector": "0x080154E1", "result": "PASS"}
{"checksum": "0xBD", "configuration": "debug", "elf_sha256": "a32668d2e00d4ebe5aa10c6a8b84a16ee945ffe8017d001261d1d03777e35d27", "image_bytes": 288660, "image_sha256": "ae9a61736732ee65d347c796ee28c8fc9ce9ac442d7ad5bc1f1e36612a306fa6", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 235780, "ITCM_RAM": 15016, "RAM": 274640}, "payload_bytes": 288644, "reset_vector": "0x080154E1", "result": "PASS"}
{"checksum": "0x4D", "configuration": "diagnostics-release", "elf_sha256": "0305a5f6870e32d6ebdd4054aec019a71c116303cd29e5cc2ddfcc4b74ab80f5", "image_bytes": 289052, "image_sha256": "c1853483a567f9d41e0e1338564440489f9f1ba7a616c7ff4d8cf8b8f74556d8", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 236100, "ITCM_RAM": 15088, "RAM": 274712}, "payload_bytes": 289036, "reset_vector": "0x080154E1", "result": "PASS"}
{"checksum": "0xDA", "configuration": "diagnostics-debug", "elf_sha256": "84a4477982a7eadec254b7cf8a14430b8a6efeb1259a21c7ca2bc9cb1255841e", "image_bytes": 289100, "image_sha256": "5404c31c128d8d42c9b45b0de80637e6abe17918b998cc4a4602840159accb33", "initial_msp": "0x20080000", "magic": "LXRV2IMG", "memory_high_water_bytes": {"DTCM": 107392, "FLASH": 236148, "ITCM_RAM": 15088, "RAM": 274712}, "payload_bytes": 289084, "reset_vector": "0x080154E1", "result": "PASS"}

EXIT 0

```

ELF segment versus HEX address audit:

```json
[
  {
    "configuration": "release",
    "elf_file_load_segments": [
      {
        "offset": 0,
        "vma": "0x8000000",
        "lma": "0x8000000",
        "file_bytes": 268500,
        "memory_bytes": 268500,
        "alignment": 65536
      },
      {
        "offset": 327680,
        "vma": "0x20020000",
        "lma": "0x80418d4",
        "file_bytes": 852,
        "memory_bytes": 852,
        "alignment": 65536
      },
      {
        "offset": 393216,
        "vma": "0x20000000",
        "lma": "0x8041c28",
        "file_bytes": 36996,
        "memory_bytes": 36996,
        "alignment": 65536
      },
      {
        "offset": 458752,
        "vma": "0x0",
        "lma": "0x804acac",
        "file_bytes": 15016,
        "memory_bytes": 15016,
        "alignment": 65536
      }
    ],
    "hex_first_data": "0x8008000",
    "hex_last_exclusive": "0x804e754",
    "hex_sha256": "d8c69416c5bcbf5158ebc4565627a20e2c1e93ee03b7258705679d42d051c6e6"
  },
  {
    "configuration": "debug",
    "elf_file_load_segments": [
      {
        "offset": 0,
        "vma": "0x8000000",
        "lma": "0x8000000",
        "file_bytes": 268548,
        "memory_bytes": 268548,
        "alignment": 65536
      },
      {
        "offset": 327680,
        "vma": "0x20020000",
        "lma": "0x8041904",
        "file_bytes": 852,
        "memory_bytes": 852,
        "alignment": 65536
      },
      {
        "offset": 393216,
        "vma": "0x20000000",
        "lma": "0x8041c58",
        "file_bytes": 36996,
        "memory_bytes": 36996,
        "alignment": 65536
      },
      {
        "offset": 458752,
        "vma": "0x0",
        "lma": "0x804acdc",
        "file_bytes": 15016,
        "memory_bytes": 15016,
        "alignment": 65536
      }
    ],
    "hex_first_data": "0x8008000",
    "hex_last_exclusive": "0x804e784",
    "hex_sha256": "953cda5c2357ddfe6e54eb84f98cf2327b2477718bb2bc754224cf2bbe4d28ca"
  },
  {
    "configuration": "diagnostics-release",
    "elf_file_load_segments": [
      {
        "offset": 0,
        "vma": "0x8000000",
        "lma": "0x8000000",
        "file_bytes": 268868,
        "memory_bytes": 268868,
        "alignment": 65536
      },
      {
        "offset": 327680,
        "vma": "0x20020000",
        "lma": "0x8041a44",
        "file_bytes": 852,
        "memory_bytes": 852,
        "alignment": 65536
      },
      {
        "offset": 393216,
        "vma": "0x20000000",
        "lma": "0x8041d98",
        "file_bytes": 36996,
        "memory_bytes": 36996,
        "alignment": 65536
      },
      {
        "offset": 458752,
        "vma": "0x0",
        "lma": "0x804ae1c",
        "file_bytes": 15088,
        "memory_bytes": 15088,
        "alignment": 65536
      }
    ],
    "hex_first_data": "0x8008000",
    "hex_last_exclusive": "0x804e90c",
    "hex_sha256": "5ae028712fc085c97b57a5e962deaa90553295734b9ebe91732ba17610044f9c"
  },
  {
    "configuration": "diagnostics-debug",
    "elf_file_load_segments": [
      {
        "offset": 0,
        "vma": "0x8000000",
        "lma": "0x8000000",
        "file_bytes": 268916,
        "memory_bytes": 268916,
        "alignment": 65536
      },
      {
        "offset": 327680,
        "vma": "0x20020000",
        "lma": "0x8041a74",
        "file_bytes": 852,
        "memory_bytes": 852,
        "alignment": 65536
      },
      {
        "offset": 393216,
        "vma": "0x20000000",
        "lma": "0x8041dc8",
        "file_bytes": 36996,
        "memory_bytes": 36996,
        "alignment": 65536
      },
      {
        "offset": 458752,
        "vma": "0x0",
        "lma": "0x804ae4c",
        "file_bytes": 15088,
        "memory_bytes": 15088,
        "alignment": 65536
      }
    ],
    "hex_first_data": "0x8008000",
    "hex_last_exclusive": "0x804e93c",
    "hex_sha256": "0ee3264573a61313b1ce0212428417b8173525c2351de05e928bfd41e5cfae1e"
  }
]
```

## Appendix D. Protected-source and Bug001 equivalence audit

```json
[
  {
    "path": "Core/Startup/startup_stm32f765vgtx.s",
    "unchanged_since_lab_setup": true,
    "sha256": "bd8132df9ffdefbc53e4e11d2b69b7b46e99fd95a08c030a9c4b7e097c3cda19"
  },
  {
    "path": "STM32F765VGTX_FLASH.ld",
    "unchanged_since_lab_setup": true,
    "sha256": "c52da818f885a82cb96f0f8deb5ce92e6448fd78cbb3eaa85b10b4f3ec98399d"
  },
  {
    "path": "cmake/stm32f765.cmake",
    "unchanged_since_lab_setup": true,
    "sha256": "2e4fc25737abf0a4c26908bcdeef3f70cd441b37588fdb5553c5e56fa8bc005a"
  },
  {
    "path": "cmake/firmware_outputs.cmake",
    "unchanged_since_lab_setup": true,
    "sha256": "96af055e8c810b0648e52dbd293ec6221a4626a2b60283ed2a7d0b5e9d2abbc2"
  },
  {
    "path": "Core/Src/system_stm32f7xx.c",
    "unchanged_since_lab_setup": true,
    "sha256": "076c84b8045fbb11d84028394e3ad618081ddc1a11bb87155e507841ede5767a"
  },
  {
    "path": "Core/Src/stm32f7xx_hal_msp.c",
    "unchanged_since_lab_setup": true,
    "sha256": "9c7a8aabf828471f56cbc23067cc65cc00dde6fa13ba31adeafa8d2cd18ca88a"
  },
  {
    "function": "SystemClock_Config",
    "unchanged_since_lab_setup": true,
    "sha256": "4a476f5312b0ce5d26201e663b933edc273f7692c6cb7af8a5c4d04b9347ee5a"
  },
  {
    "function": "MX_GPIO_Init",
    "unchanged_since_lab_setup": true,
    "sha256": "cfc17583045446b3c4823babdf62ff3c44873f7432166552729bc3ae3b669746"
  },
  {
    "function": "MX_DMA_Init",
    "unchanged_since_lab_setup": true,
    "sha256": "fd1ad708d15b1de4b3e2ee74fcf7a45450ce1f9523986ba0989e267394bfec6d"
  },
  {
    "function": "MX_SDMMC1_SD_Init",
    "unchanged_since_lab_setup": true,
    "sha256": "4008868533b212d57611319b9b6f6e45af4ab8187bde4a2f13f1ddc2e5fe26f6"
  },
  {
    "function": "MX_I2S2_Init",
    "unchanged_since_lab_setup": true,
    "sha256": "851e71b45248842df7e6f7708c47fde032a752f013c34238cf52906daa0fced8"
  },
  {
    "function": "MX_I2S3_Init",
    "unchanged_since_lab_setup": true,
    "sha256": "382c1c4c2abc0b68b3a06813ea207ce18cbd00163327a46963b11e79d61584c0"
  },
  {
    "function": "MX_TIM2_Init",
    "unchanged_since_lab_setup": true,
    "sha256": "86c1852a575036ac991d52a850fdc5799e5e7d55ff38133cd934e1fcf651480a"
  }
]
```

```json
[
  {
    "function": "interpolate",
    "identical_to_bug001": true,
    "sha256": "6ae94d63a2a287015a7ff60d8497e464d9b8374a300cd8dec5f69bfe855ccc0f"
  },
  {
    "function": "preset_isTransportParameter",
    "identical_to_bug001": true,
    "sha256": "65ed4f2e0dd8a5ede124166e2054167e9672c5be435cc0eb79f1542476ad9d4d"
  },
  {
    "function": "preset_sendDrumsetParametersImmediate",
    "identical_to_bug001": true,
    "sha256": "e1c00c50f4b2ae7dd5ff5ef1aa7308bd6f6e1423f3eaf352e79509057bd74988"
  },
  {
    "function": "preset_morph_tick",
    "identical_to_bug001": true,
    "sha256": "a728cd3f7f85ba34db22bb4d8e3825fe53a2574c0e80b5037dc65af83f4f54de"
  },
  {
    "function": "preset_morph",
    "identical_to_bug001": true,
    "sha256": "cc04914f0eef73bd65bd2e116f5e5b23d8118a8d718b619843a791809815ef66"
  },
  {
    "path": "Core/Src/Soundengine/MIDI/MidiParser.c",
    "complete_file_identical_to_bug001": true
  },
  {
    "filter_and_all_DSPAudio_sources_unchanged_since_lab_setup": true
  }
]
```

## Appendix E. Relocatable object hashes and static frames

Baseline here is S4b-1 e49bb028. These comparisons concern software bytes, not physical timing.

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

## Appendix F. Reproducible auxiliary read-only scripts

The established image checker is committed at the reviewed source revision. These additional scripts were run from temporary storage and are reproduced for continuation; they do not edit firmware.

### image_audit.py

```python
from pathlib import Path
import subprocess,re,json,hashlib,struct
root=Path('/workspace/lxr-desktop-app');p=Path('/tmp/phase-r1')
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
root=Path('/workspace/lxr-desktop-app');work=Path('/tmp/phase-r1/objects');work.mkdir(exist_ok=True)
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

## Appendix G. Publication and unchanged-source receipt

Only this new Markdown file is published on the new report-only branch, with lab-setup as its one parent. The review publisher verifies remote source/report heads, exact report URL/content and all36 pre-existing branch heads after publication. The containing report commit SHA is available from GitHub metadata/final handoff; a future self-commit hash is not embedded recursively in its own content. The implementation checkout remains clean at5ab5e6b128fdc8d1c5aa14d8501a6d98f8311527. No source/test/configuration file, existing report, implementation branch, main or lab-setup was modified. No firmware binary, hardware operation, flashing command, merge or force-push was performed.
