# LXR Firmware Lab — Phase C: USB Allocator Reentrancy and Memory Safety

Date: 2026-10-26 UTC. Repository: DocLXR-02/lxr-desktop-app.

## 1. Outcome, scope and exact references

The identified USB class interrupt-context heap dependency is removed. Configuration/deconfiguration now uses a typed fixed-storage object with explicit single-owner lifetime, nonblocking callback re-entry rejection, deterministic initialization/cleanup and conservative retention after cleanup errors. Software tests/builds/images pass. **This is not a deployment-ready or hardware-validated release.** Physical USB operation, interrupt latency, audio timing, stack margins and fault/recovery behavior remain unverified.

No hardware operation, flashing utility, bootloader or option-byte modification, clock/PLL change, startup/linker change, audio algorithm/buffer/sample-rate change, priority change, watchdog activation, global interrupt masking, new wait/lock or binary upload occurred. Only the existing `/workspace/lxr-desktop-app` checkout was used; no new worktree was created. Every pre-existing branch is preserved.

| Reference | Exact value |
|---|---|
| Base branch, preserved | `safety/runtime-diagnostics-b1` |
| Verified base commit / sole source parent | `e4c0fd3ca66ced53413d5e1733e7ac9aa1c0da6e` |
| Phase C branch | `safety/usb-allocator-reentrancy-c1` |
| Phase C source commit | `091f502de6f57f3262ffc68659eafac6f8bdfa19` |
| Phase C tree | `11f5e713e124416ecc13cc788d3123ae4c683d17` |
| Preserved Phase A | `e19bb0544dcc8f07138a9857c78022e8efaecd81` |
| Preserved Bug 001 | `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0` |
| Preserved Euclidean | `67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72` |
| Preserved lab-setup / report base | `5829398cea5d337200393757e39a37d4d8148256` |
| Preserved main | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| New report-only branch | `codex-reports/2026-10-26-phase-c-usb-allocator-safety` |
| Unique report path | `docs/codex-berichte/2026-10-26_phase-c-usb-allocator-safety.md` |

Source URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/safety/usb-allocator-reentrancy-c1

Immutable source commit: https://github.com/DocLXR-02/lxr-desktop-app/commit/091f502de6f57f3262ffc68659eafac6f8bdfa19

CODE = confirmed inspected source/configuration. BUILD = executed test, compilation or binary analysis. HYPOTHESIS = plausible unconfirmed runtime effect. OPEN = unresolved or requiring further evidence/hardware validation.

AGENTS.md was read from remote lab-setup (blob `2f4d121f53a7055c1a30355235384a95ef920f1a`). The complete [Phase B report](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-phase-b-memory-safety-runtime-diagnostics/docs/codex-berichte/2026-10-26_phase-b-memory-safety-runtime-diagnostics.md) was read. The cloud-environment-runtime skill was used to inspect the prepared environment. No agents were delegated. All new comments, tests, commit messages and documentation are English. The task's explicit Phase B source-base instruction governs this branch; the report is independently based on lab-setup.

The current remote Phase B branch matched the exact requested commit. The initial checkout was clean at that commit, and the requested new source branch did not exist. No existing work was discarded or overwritten.

## 2. Baseline reproduced before source modification

The source and relevant USB lifecycle paths were inspected before any source edit. Then the following baseline commands completed successfully:

```sh
export PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH
cd /workspace/lxr-desktop-app
cmake --version
ninja --version
python3 --version
arm-none-eabi-gcc --version
python3 tests/bug001/run.py
python3 tests/euclidean/run.py
python3 tests/safety/run.py
cmake --preset release
cmake --build --preset release --parallel 4
cmake --preset debug
cmake --build --preset debug --parallel 4
cmake --build build/diagnostics-release --parallel 4
cmake --build build/diagnostics-debug --parallel 4
python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
```

BUILD: all tests/builds/checks returned zero; all four image/ELF hashes reproduced Phase B. Existing build directories were reused. Diagnostic caches remained ON, normal caches OFF; instrumented directory existence was not taken as evidence of the mode. No clean-room rebuild is claimed.

Tools: CMake 3.31.10; Ninja 1.13.0.git.kitware.jobserver-pipe-1; Python 3.12.14; Arm GNU 14.2.1 20241119 (15:14.2.rel1-1), newlib-nano; native GCC 14.2.0. No relevant toolchain difference from Phase B was found. Source version remains 1.7; official public v1.71 and private support v1.72 are not asserted equivalent to this source/build.

Baseline image metadata:

```json
[
  {
    "checksum": "0x74",
    "configuration": "release",
    "elf_sha256": "8b2e958c316d8e908d8eafe9f2eb480198629d45376edea311b847e03a69c11b",
    "image_bytes": 275652,
    "image_sha256": "d8968b8717b1ded7782027c1af848d52b4372dd575c6c38afa2cba3ff8c5b2e3",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 222772,
      "ITCM_RAM": 15016,
      "RAM": 271032
    },
    "payload_bytes": 275636,
    "reset_vector": "0x08014E45",
    "result": "PASS"
  },
  {
    "checksum": "0x5D",
    "configuration": "debug",
    "elf_sha256": "c71d0e3895c559eeb37545fbfdd7d279dce859c88ce93f1b1e67b652d5913186",
    "image_bytes": 275700,
    "image_sha256": "9bfc9c5e5391bc18538b1ef38e1679ea3b41236f00bd3b30efd78f87bf44b9cc",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 222820,
      "ITCM_RAM": 15016,
      "RAM": 271032
    },
    "payload_bytes": 275684,
    "reset_vector": "0x08014E45",
    "result": "PASS"
  },
  {
    "checksum": "0xB5",
    "configuration": "diagnostics-release",
    "elf_sha256": "d07a2cef6c75b2f4ba46e42d18ff7dd7ba5c2e627d27abd8f7a7c5e59e31fe1f",
    "image_bytes": 276092,
    "image_sha256": "96ae8f2b2eff0c46556da596845f1ae1eadf4053e831ee872229ed9e95d0d900",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223140,
      "ITCM_RAM": 15088,
      "RAM": 271104
    },
    "payload_bytes": 276076,
    "reset_vector": "0x08014E45",
    "result": "PASS"
  },
  {
    "checksum": "0x2B",
    "configuration": "diagnostics-debug",
    "elf_sha256": "18a6f61ffd44af66383e702a2aa35899f6fca680a5fd50913ad2cbb9f8d2541c",
    "image_bytes": 276140,
    "image_sha256": "e8a0ee0ca1c2a33ed5a6e1f6df7ae8e7a2b3c203a544b891c523679759c4cb9d",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223188,
      "ITCM_RAM": 15088,
      "RAM": 271104
    },
    "payload_bytes": 276124,
    "reset_vector": "0x08014E45",
    "result": "PASS"
  }
]
```

## 3. Actual USB implementation and allocator call paths

### Registered class and instance assumptions

CODE: `USB_DEVICE_MIDI/App/usb_device.c:45,64–88` declares one active `hUsbDeviceFS`, registers `USBD_CDC`, installs `USBD_Interface_fops_FS`, and starts the device. The CDC implementation is this repository's MIDI-adapted file, not an unrelated standard serial-only class. Its existing descriptors/endpoints and MIDI data routines are preserved.

Only one active device/class registration was found. The target config is USB FS with embedded PHY and `dma_enable=DISABLE` (`USB_DEVICE_MIDI/Target/usbd_conf.c:339–342`). HS branches are present in the middleware and covered by host tests as source branches; no physical HS capability test is claimed. Application Init/DeInit callbacks are static and global-device-specific: `usbd_cdc_if.c:155–173` binds static UserTxBufferFS/UserRxBufferFS through the existing setters, then returns USBD_OK; DeInit returns USBD_OK. This is not a composite/multiple-instance application. A legacy `usb_stop()` in `Core/Src/Soundengine/USB/usb_manager.c:29–60` is inside `#if 0`; it is not an active main-context lifecycle caller.

The fixed store supports one active owner. It does not create a generic multi-device allocator or make public core functions thread-safe. Current lifecycle dispatch is serial in USB processing, with startup registration before active operation. Adding runtime main-context lifecycle calls, another device/class, or callbacks with different ownership requires renewed review.

### Baseline allocation/deallocation evidence

CODE in the Phase B revision:

- `USB_DEVICE_MIDI/Target/usbd_conf.h:96,99` maps USBD_malloc/free to malloc/free.
- Baseline `usbd_cdc.c:729` allocates sizeof(USBD_CDC_HandleTypeDef) during class Init; `:789` frees pClassData during DeInit.
- `OTG_FS_IRQHandler` → HAL_PCD_IRQHandler (`Core/Src/stm32f7xx_it.c:376ff`).
- Setup callback (`USB_DEVICE_MIDI/Target/usbd_conf.c:134–139`) → USBD_LL_SetupStage → standard device request dispatch → USBD_SetConfig (`Middlewares/JS/.../Core/Src/usbd_ctlreq.c:560ff`) → USBD_SetClassConfig / USBD_ClrClassConfig → class Init/DeInit (`usbd_core.c:224–253`).
- HAL reset callback → USBD_LL_Reset (`usbd_core.c:440–463`), which calls class DeInit when class data exists.
- HAL disconnect callback → USBD_LL_DevDisconnected (`usbd_core.c:575–580`), which calls class DeInit.
- Core USBD_Stop and USBD_DeInit also call class DeInit. No active application main-context stop/deinit caller was found beyond the disabled legacy block.

SET_CONFIGURATION with value zero deinitializes; a different configured value clears/reinitializes. Repeating the same nonzero configuration is acknowledged by the core without calling class Init again. Bus reset, disconnect and reconnect/configuration sequences can exercise deallocation/allocation. Duplicate direct Init was unsafe in the old implementation: it opened endpoints before allocating and could replace the live pointer without releasing the old allocation. Repeated DeInit could close endpoints even without a live class.

BUILD/CODE: linked `__malloc_lock` and `__malloc_unlock` remain no-op `bx lr` routines. Main-context newlib-nano formatting can allocate through `_svfprintf_r`/`_malloc_r`/`_realloc_r`. Therefore baseline USB interrupt allocation could preempt main allocator work without effective serialization. HYPOTHESIS: that overlap could corrupt allocator metadata or the break; no allocator-corruption crash or physical overlap was observed/reproduced. The correction removes that identified asynchronous heap caller rather than trying to conceal it with a bounds check.

### Other asynchronous allocator paths

A source search of Core, USB_DEVICE_MIDI, the active JS USB library, Drivers and FATFS found no additional active direct allocator caller in the reviewed interrupt paths. USB debug logging is disabled (`USBD_DEBUG_LEVEL=0`); registered Init/DeInit/Control/Receive callbacks contain no allocator or formatting call. The driver/FATFS direct allocator search was empty. Remaining visible formatting calls are main/UI/preset operations; comments and disabled updater/legacy code were distinguished from active paths.

After correction, both native complete-class object and ARM class object have no heap/allocator-lock dependency. The ARM class's undefined symbols are ordinary USB LL/control helpers plus memset. This checks the entire translation unit, not just extracted Init/DeInit bodies. The reviewed LL/HAL helper source introduces no allocator use.

CODE: the known USB-vs-main heap interaction is eliminated. OPEN: this is not a formal whole-program proof for every indirect library callback, future class or future interrupt change. No-op newlib locks do not become intrinsically safe; adding any asynchronous allocator caller would recreate the concern. Other USB data-state concurrency and configured-class API preconditions remain separate questions.

## 4. Correction strategies compared

| Approach | Assessment |
|---|---|
| Typed fixed class storage | Chosen: exact size/alignment follows the production type; deterministic lifetime/initialization; no heap ownership shared with formatting; bounded work and no audio-path change. Explicit single-owner rules needed. |
| Generic target USBD_malloc/free pool | Not sufficient alone: original Init opens endpoints and overwrites pClassData before checking allocation failure; pool hooks lack pdev ownership context. Duplicate/cleanup correctness still requires class lifecycle changes, while global aliases could affect unrelated future classes. |
| Deferred allocation in main | Would require coordinating synchronous USB configuration dispatch, control responses, lifecycle state and scheduling across main/USB contexts. No existing safe queue/completion mechanism was established; broader architecture/USB timeout behavior would change. Not implemented. |
| Allocator synchronization | Spinning/blocking in USB IRQ can deadlock when it preempts the main lock holder. Global masking would delay real-time interrupts and is explicitly unauthorized. Selective masking or allocator redesign would need separate timing/ownership review. Not implemented. |

The selected change is limited to the existing class lifecycle in one production file. No target allocator macro, global lock, IRQ setting, callback registration, descriptor, endpoint number or MIDI message path is modified.

## 5. Implementation and ownership rules

File: `Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c`, starting around line 691. The complete diff is in Appendix A.

New private state:

```c
static USBD_CDC_HandleTypeDef cdc_storage;
static USBD_HandleTypeDef *cdc_owner;
static uint8_t cdc_busy;
static uint8_t cdc_app_started;
```

CODE/BUILD: target object size is **540 bytes**, alignment **4**, matching the header's uint32_t control-data array and pointer/state fields. Host size is 552/alignment 8 due to host pointer width; tests do not mistake that for target size. Typed storage avoids a hardcoded byte count or underaligned pool. The object has static lifetime; it is never heap-freed.

### Initialization

- Reject NULL device, a current owner, callback re-entry, a pre-existing pClassData, invalid Init/DeInit callbacks, or already-used class endpoints before altering live state.
- Claim ownership and mark busy. Clear the whole typed object and publish its pointer before the application callback needs buffer setters.
- Open the same IN, OUT and command endpoints in the same order with unchanged types and speed-dependent packet sizes. Mark each attempted endpoint for cleanup, including partial-open failure cases.
- Call the existing application Init, reject failure or a missing receive buffer, initialize transfer states, and prepare OUT reception.
- On success release the busy flag; ownership remains active.
- On error run bounded cleanup and return USBD_FAIL. If cleanup cannot establish release, retain the owner/object rather than reuse potentially active resources.

The memset operation is a fixed 540-byte clear on target, not a variable-size allocation or unbounded loop. All valid original application callbacks still bind the same buffers. Full clearing now prevents old command/buffer/length/state contents from leaking across class generations; the old malloc path did not guarantee complete zeroing.

### Cleanup and retries

`USBD_CDC_Release` examines exactly three class endpoints. It closes those marked used and clears a used flag only after reported success. If any close fails, ownership and class pointer remain; application DeInit is not called prematurely. If endpoints close but application DeInit fails/has become invalid, ownership is likewise retained. A later DeInit/reset may retry. Successful cleanup calls application DeInit while class data is still valid, then NULLs pClassData, zeroes storage and releases the owner. No arbitrary pointer is passed to free.

`USBD_CDC_DeInit` rejects busy, foreign-owner and mismatched class-data requests. Idle NULL-data DeInit is idempotent, so repeated stop/disconnect does not double-clean. Busy rejects callback-level re-entry without spinning or disabling interrupts. It is **not a thread mutex** or proof of cross-context lifecycle safety.

Existing core error semantics are unchanged: some core cleanup callers ignore the class return value, and failed configuration can leave core state CONFIGURED until deconfiguration/reset. The retained-owner guard prevents silently reusing uncertain storage even in that case. Repeating the same SET_CONFIGURATION value is still core-ACKed without a new Init. Persistent HAL faults are not claimed to recover physically; software quarantine/retry is tested, not real controller recovery.

### Preservation evidence and remaining pointer contract

BUILD: the entire class-file prefix (including all descriptors) before the lifecycle change and the entire suffix starting at USBD_CDC_Setup were byte-compared with Phase B and matched. Registration, endpoints/constants, application callbacks, all data/control handlers, `_sbrk`, diagnostics, startup/linker, CPU/PLL/IRQ files and default configuration were also checked unchanged.

This does not retrofit all pre-existing USB data APIs with unconfigured-state checks. Buffer setters and other data consumers retain their configured-class preconditions. Holding arbitrary external pointers across generations or introducing main-context lifecycle races is not a supported new feature. Stable static storage removes heap use-after-free exposure from this class but does not by itself prove every USB data-state interleaving safe.

The Phase B safety README was updated because its statement of an active USB heap caller would otherwise be stale. This documentation change is directly related; allocator/diagnostic production code itself is untouched.

## 6. Heap semantics after the correction

CODE/BUILD: Phase B bounded `_sbrk` remains byte-identical. It aligns initial `_end` to 8 bytes, preserves byte-granular positive/negative sbrk requests, rejects invalid bounds/current positions, growth beyond SRAM end, shrink below start and integer wrap, and returns -1/ENOMEM without moving the break on failure. It uses existing `_end` and `_estack`; startup moves the active stack into DTCM, so this general-SRAM heap does not share the active runtime stack.

Current normal `_end=0x200622d8`, instrumented `_end=0x20062320`, exclusive limit `_estack=0x20080000`. Available heap including its 512-byte minimum reservation is 122,152 normal / 122,080 instrumented bytes. It follows the linked BSS movement automatically. The heap loses 544 bytes of available space relative to each Phase B mode, consistent with the measured static-RAM increase.

The boundary mechanism is correct for serialized break use, not a reentrant malloc implementation. Main formatting can still allocate/fail; fragmentation and callers' OOM handling remain relevant. The remaining reviewed call graph no longer contains the USB interrupt heap caller, so the demonstrated need for main-versus-USB allocator synchronization is removed. No new allocator locking or global interrupt masking is justified/introduced here. Future asynchronous callers require renewed review.

## 7. Regression and robustness tests

New files under `tests/usb_allocator/`:

- `host.c`: explicit HAL/USB LL/control-response mocks and lifecycle assertions.
- `run.py`: production-body extraction with original source line annotations, native ASan/UBSan build/run, plus compilation/symbol audit of the complete class TU.
- `README.md`: supported ownership contract, tests, failure behavior and limitations.

Extracted unchanged production logic includes class Init/Release/DeInit, TX/RX setters, actual CDC_Init_FS/CDC_DeInit_FS, core SetClassConfig/ClrClassConfig/Reset/DevDisconnected/Stop and actual standard USBD_SetConfig. Storage/owner declarations are extracted from production rather than reimplemented. Endpoint open/close/receive, LL stop and control status/error are mocked. Heap aliases abort if reached. No target USB or physical register executes.

Coverage:

1. Fresh FS/HS configuration, endpoint order/type/size checks and application buffer binding.
2. Repeated same configuration via actual core handler (no duplicate Init), direct duplicate Init rejection preserving contents.
3. Deconfiguration, reset, disconnect/reconnect and stop/restart through actual core bodies.
4. Two thousand lifecycle cycles, stable pointer reuse only after successful cleanup, full zeroing before callback/after cleanup.
5. Invalid inputs, foreign class data/owner, malformed callbacks, pre-used endpoints and callback re-entry.
6. Fourteen injected failure/retention/retry scenarios: all opens/closes, application init/deinit, missing RX buffer, receive preparation, compound cleanup failures, invalid live pointer and invalid callback table.
7. Complete production class compilation and undefined-symbol check to prevent an omitted source path from hiding malloc/free/calloc/realloc/sbrk/allocator-lock references.

All commands below ran with exit zero on the final same source tree, including after importing the exact published commit:

```sh
python3 tests/bug001/run.py
python3 tests/euclidean/run.py
python3 tests/safety/run.py
python3 tests/usb_allocator/run.py
```

All use existing native ASan/UBSan instrumentation where supported. The new runner uses GNU C11, -O1 -g, ASan+UBSan, no sanitizer recovery, non-PIE and frame pointers; leak detection is disabled for this environment. Compiler/statement failures abort the runner, lifecycle process has a 30-second timeout, and no timeout occurred. Host FS/HS cases are software branch coverage, not evidence of physical FS/HS enumeration or packets.

Exact final stdout:

```text
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence
PASS generation: 2176 exact masks match lab-setup; literal lengths; CLZ unshimmed
PASS bounds: all byte-valued track/pattern/length inputs
PASS playback: 294912 flam/shift ticks; 4160 recording positions; mixed lengths
PASS persistence: 130 full named/autosave roundtrips, unchanged layout/payload
PASS malformed: 10210 truncation/range/I-O/legacy-tail cases; no partial lengths
PASS protocol: 180802 byte-value/counter boundary calls; no wrap
PASS all six ASan/UBSan groups
PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves
PASS diagnostics disabled: hook arguments not evaluated; DMA call evaluated exactly once
PASS diagnostics enabled: wraparound, saturation, budget boundaries, simulated DWT/DMA and no-cycle-counter state
PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing
PASS USB failures: 14 rollback/retention/retry scenarios plus invalid requests; no allocator calls
Host object: size=552 alignment=8 (target size/alignment checked separately in ELF)
PASS complete production USB class object: no heap/allocator-lock references
```

No existing regression group was skipped or failed. Existing Bug 001 and Euclidean production corrections/tests are intact. No allocator-corruption reproducer was run on physical firmware; host tests demonstrate the new ownership and failure contract, not a prior observed hardware crash.

## 8. Four builds, images and exact artifact metadata

Commands executed with the prepared PATH:

```sh
cmake --build --preset release --parallel 4
cmake --build --preset debug --parallel 4
cmake --build build/diagnostics-release --parallel 4
cmake --build build/diagnostics-debug --parallel 4
python3 tests/euclidean/check_images.py build/release build/debug build/diagnostics-release build/diagnostics-debug
```

BUILD: all returned zero, with no warning/error in the normal ARM build logs. Existing OFF/OFF/ON/ON cache modes were retained and verified; normal presets/CMake/Phase B diagnostics are unchanged. Cached builds recompiled the class and relinked; they are not described as clean builds. Final pinned builds succeeded on commit 091f502d with the exact validated tree. Version remains 1.7 and generated filenames remain LXR_V2_V1.7.img and LXR_V2_STM32F765.bin/.elf/.map.

The image checker verified every 16-byte LXRV2IMG header, padding, little-endian payload length, checksum, exact bin/image correspondence, flash limit, aligned initial stack and plausible Thumb reset vector, and all ELF allocated VMA/LMA ranges. Generated artifacts remain in ignored local build directories; no binary or log is uploaded as a release artifact.

```json
[
  {
    "checksum": "0xE6",
    "configuration": "release",
    "elf_sha256": "c31121510d8b3100d1dcdeeb89adffdb15c299837e24ded1e4af8f6926f62b7f",
    "image_bytes": 275964,
    "image_sha256": "06e9859d1ff99e3b80baf121225eb83865b8573e319f96243ccb037b291342a3",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223084,
      "ITCM_RAM": 15016,
      "RAM": 271576
    },
    "payload_bytes": 275948,
    "reset_vector": "0x08014F79",
    "result": "PASS"
  },
  {
    "checksum": "0xFF",
    "configuration": "debug",
    "elf_sha256": "5385d5fcd676f29e00d9e436979ebee55e6b8a1efdc9106b3e7c82930fe572b1",
    "image_bytes": 276012,
    "image_sha256": "da880ed68c4d0e65281e1b49aa9a007270c9ca625bc5d797d8bb0b066e9a2fd1",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223132,
      "ITCM_RAM": 15016,
      "RAM": 271576
    },
    "payload_bytes": 275996,
    "reset_vector": "0x08014F79",
    "result": "PASS"
  },
  {
    "checksum": "0xF3",
    "configuration": "diagnostics-release",
    "elf_sha256": "3e965f9cf60f4ace2511484f7cca774a1b3519b4bd4c3c08c75f2e90e8fe17da",
    "image_bytes": 276404,
    "image_sha256": "de6fe28745cb01377b2bcb7ae1c3f6f660c8050b084f5d005e9770dc117bcd30",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223452,
      "ITCM_RAM": 15088,
      "RAM": 271648
    },
    "payload_bytes": 276388,
    "reset_vector": "0x08014F79",
    "result": "PASS"
  },
  {
    "checksum": "0xBD",
    "configuration": "diagnostics-debug",
    "elf_sha256": "32d65fcd2e5371f68d96aec33fad596440d3a48cdcd22c1b4b56a52b92ff2bbc",
    "image_bytes": 276452,
    "image_sha256": "d4173d4531cd87fe0b11885778f81df22e6c1dcd2677d4393534b167d248b037",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223500,
      "ITCM_RAM": 15088,
      "RAM": 271648
    },
    "payload_bytes": 276436,
    "reset_vector": "0x08014F79",
    "result": "PASS"
  }
]
```

Raw-bin SHA-256:

```text
0d6c96bf0d3e3bc3227ac6b7dd3a3a71d2bf269925741ff2922eb64f43034abe  build/release/LXR_V2_STM32F765.bin
d2cce8d44cbc2860fe6453dd3b2132bd76944fdf1f6c4a98e5a062540142263a  build/debug/LXR_V2_STM32F765.bin
fea2ff5d1e50fb70365f30964a1119c8db1a11a0d6dfd10d18f4bc6075b2bef4  build/diagnostics-release/LXR_V2_STM32F765.bin
1859fd385bd1e0c1be2dffcbaf6aa88b85489af01f4f2d5293dbf03c7b2b933c  build/diagnostics-debug/LXR_V2_STM32F765.bin
```

## 9. Static memory and real-time impact

BUILD, bytes:

| Region/footprint | Phase B normal | Phase C normal | Phase B diagnostic | Phase C diagnostic |
|---|---:|---:|---:|---:|
| ITCM (capacity 16,384) |15,016|15,016|15,088|15,088|
| DTCM (capacity 131,072) |107,392|107,392|107,392|107,392|
| General RAM (capacity 393,216) |271,032|271,576|271,104|271,648|
| Linker FLASH allocation |223,624|223,936|223,992|224,304|
| Complete raw FLASH payload |275,636|275,948|276,076|276,388|

Debug FLASH/payload adds 48 bytes to its corresponding Release mode; ITCM/DTCM/RAM are equal. Phase C adds **544 linked RAM bytes** and **312 FLASH payload bytes** in each mode, with no ITCM or DTCM increase. The new raw objects comprise typed storage 540 bytes, owner pointer 4 and two byte flags, plus linker packing/alignment/garbage-collection effects; the aggregate map delta is the authoritative 544 bytes, not a rounded object-size assumption.

Normal headroom: ITCM 1,368; DTCM 23,680 before runtime stack; general RAM 121,640 after static/minimum heap; full-load FLASH 739,860. Instrumented headroom: ITCM 1,296; DTCM 23,680; general RAM 121,568; full-load FLASH 739,420. These remain within static regions, not runtime-stack or real-time proofs. The FLASH linker report and full payload extent differ because of explicit TCM load-image placement; both are range-checked.

Exact static storage/ownership locations (normal; same locations in instrumented build):

```text
200622d8 B _end
20080000 R _estack
2002094e 00000001 b cdc_app_started
2002094f 00000001 b cdc_busy
20020950 00000004 b cdc_owner
20020954 0000021c b cdc_storage
```

The target storage address 0x20020954 is 4-byte aligned in ordinary SRAM BSS. The type requires 4-byte alignment, including its uint32 control data. USB FS DMA is disabled in the existing PCD initialization; no DMA mode/cache configuration was changed. Runtime data correctness still requires physical validation.

Real-time source review (CODE): no global/USB IRQ masking, lock acquisition, spin/wait, formatting, file I/O, dynamic allocation, audio callback hook, diagnostic export, new interrupt or clock/priority change was added. Work is bounded: clear one fixed object and inspect at most three endpoints during lifecycle transitions. The existing callback registrations and steady-state audio/MIDI paths are unchanged. Compared with heap operations, fixed storage avoids allocator contention and unpredictable heap traversal, but no actual latency reduction or timing margin is claimed. Fixed clearing/error handling adds bounded USB-context work; hardware interrupt duration and audio interaction remain OPEN.

## 10. Deployment assessment and outstanding questions

1. **Relevant USB IRQ allocation removed? CODE/BUILD: yes.** Full class object and source no longer call the allocator, and lifecycle tests preserve ownership and valid cleanup.
2. **Other IRQ allocator callers? CODE:** none found in the reviewed compiled paths; **OPEN:** no formal proof of every indirect/future library call. Adding classes/callbacks or enabling logging requires review.
3. **Bounded sbrk correct? CODE/BUILD:** unchanged, relevant alignment/range/overflow/failure tests pass. It is still not a general concurrent allocator.
4. **Newlib locks still no-ops? BUILD: yes.** They remain relevant as a restriction against future asynchronous allocation. No remaining known USB path requires them for allocator exclusion after this correction.
5. **Repeated lifecycle safe? BUILD:** tested serial lifecycle and callback re-entry/error contracts pass. Physical enumeration/controller recovery is OPEN. Lifetime assumes one active registered class and serialized current dispatch.
6. **Main heap vulnerable to asynchronous access? CODE:** the identified USB caller is removed. **OPEN:** future/unreviewed asynchronous callers, fragmentation and OOM handling are not solved by this patch. No prior corruption is claimed observed.
7. **Audio/configuration preserved? CODE/BUILD: yes**, source diffs and byte comparisons show no audio, startup, linker, bootloader, clock, IRQ, sample-rate or buffer change. Diagnostics remain OFF by default.
8. **Static memory change? BUILD:** RAM +544, full FLASH payload +312, ITCM/DTCM unchanged in both modes.
9. **Unverified without hardware? OPEN:** actual USB reconnect/error recovery, class/data concurrency, USB IRQ duration, audio peaks, stack/heap high-water and fault recovery. No measured runtime utilization, latency, thermal behavior or audio reliability is reported.
10. **Another software-only task?** No further allocator-lock change is justified by the currently identified call graph. Before deployment, a focused review of remaining USB data-API/main-versus-IRQ preconditions, core failure-state recovery, stack/OOM handling and the fault/measurement plan is recommended. Any unresolved issue from that review requires a separate scoped correction. First controlled hardware validation still requires explicit approval, validated backups and a recovery plan; this task does not authorize it.

The correction removes the Phase B allocator-specific blocker in the inspected implementation. It does not declare the complete firmware deployment-ready or certify all USB timing/data races. Stack watermark/fault capture remain design-only from Phase B; watchdog recovery remains unactivated/unestablished. Source CMake 1.7, image-format validity and official v1.71 behavioral compatibility remain separate questions.

## 11. Publication, errors and continuation instructions

Exactly five files changed: one production middleware file, three new USB test/documentation files, and the directly related Phase B safety README correction. No generated artifacts are in the tree. Full list/diff follows.

Publication used authenticated GitHub Git-data APIs rather than claiming a native git push. Every uploaded blob SHA matched local bytes; the remote tree matched 11f5e713 exactly. Readback verified five files and the sole Phase B parent. The exact API-created commit object was hash-verified and imported locally, leaving a clean source checkout. No existing branch was updated or merged. A new report-only branch based on lab-setup adds this unique Markdown file; final verification checks exact content and both branch heads plus every pre-existing ref.

Errors: one new test script initially had an extra blank line at EOF detected by diff-check; it was removed before publication and the tests reran. Some broad search output was truncated and focused reads followed. No implementation/test/build/image failure was suppressed. Host mocks cannot establish controller behavior, callback latency, complete packet compatibility or physical error recovery. New tests deliberately do not execute potentially unsafe firmware on hardware.

Recommended next action: review the single production-file diff, ownership/failure policies and remaining data-path/stack/diagnostic limitations. Agree on any further software review and a separately authorized, instrumented hardware validation plan. Do not flash, merge or publish images based on this report alone.

## Appendix A. Complete correction and test diff

Existing contexts are quoted verbatim as evidence. Production CRLF bytes remain preserved in Git; display line endings below are normalized for Markdown only.

```diff
diff --git a/Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c b/Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
index 8025bee..d193b9e 100644
--- a/Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
+++ b/Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
@@ -688,108 +688,88 @@ __ALIGN_BEGIN uint8_t USBD_CDC_OtherSpeedCfgDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIG
   * @param  cfgidx: Configuration index
   * @retval status
   */
-static uint8_t  USBD_CDC_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
+/* One registered device/class instance: typed storage supplies size and alignment.
+ * The USB core serializes lifecycle calls. busy rejects callback re-entry; it is
+ * not a thread lock. Never allocate, free, mask interrupts or wait here.
+ */
+static USBD_CDC_HandleTypeDef cdc_storage;
+static USBD_HandleTypeDef *cdc_owner;
+static uint8_t cdc_busy;
+static uint8_t cdc_app_started;
+
+static uint8_t USBD_CDC_Release(USBD_HandleTypeDef *pdev)
 {
-  uint8_t ret = 0U;
-  USBD_CDC_HandleTypeDef   *hcdc;
-
-  if(pdev->dev_speed == USBD_SPEED_HIGH)
-  {
-    /* Open EP IN */
-    USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK,
-                   CDC_DATA_HS_IN_PACKET_SIZE);
-
-    pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 1U;
-
-    /* Open EP OUT */
-    USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK,
-                   CDC_DATA_HS_OUT_PACKET_SIZE);
-
-    pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 1U;
-
-  }
-  else
-  {
-    /* Open EP IN */
-    USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK,
-                   CDC_DATA_FS_IN_PACKET_SIZE);
-
-    pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 1U;
-
-    /* Open EP OUT */
-    USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK,
-                   CDC_DATA_FS_OUT_PACKET_SIZE);
-
-    pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 1U;
-  }
-  /* Open Command IN EP */
-  USBD_LL_OpenEP(pdev, CDC_CMD_EP, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE);
-  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 1U;
-
-  pdev->pClassData = USBD_malloc(sizeof (USBD_CDC_HandleTypeDef));
-
-  if(pdev->pClassData == NULL)
-  {
-    ret = 1U;
-  }
-  else
+  const uint8_t endpoints[3] = {CDC_IN_EP, CDC_OUT_EP, CDC_CMD_EP};
+  uint8_t failed = 0U;
+  for(uint8_t i = 0U; i < 3U; ++i)
   {
-    hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
-
-    /* Init  physical Interface components */
-    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Init();
-
-    /* Init Xfer states */
-    hcdc->TxState = 0U;
-    hcdc->RxState = 0U;
-
-    if(pdev->dev_speed == USBD_SPEED_HIGH)
-    {
-      /* Prepare Out endpoint to receive next packet */
-      USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, hcdc->RxBuffer,
-                             CDC_DATA_HS_OUT_PACKET_SIZE);
-    }
-    else
+    uint8_t ep = endpoints[i];
+    uint8_t used = (ep & 0x80U) ? pdev->ep_in[ep & 0x0FU].is_used : pdev->ep_out[ep & 0x0FU].is_used;
+    if(used)
     {
-      /* Prepare Out endpoint to receive next packet */
-      USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, hcdc->RxBuffer,
-                             CDC_DATA_FS_OUT_PACKET_SIZE);
+      if(USBD_LL_CloseEP(pdev, ep) != USBD_OK) failed = 1U;
+      else if(ep & 0x80U) pdev->ep_in[ep & 0x0FU].is_used = 0U;
+      else pdev->ep_out[ep & 0x0FU].is_used = 0U;
     }
   }
-  return ret;
+  /* Retain the object if resources may still be active. DeInit may retry. */
+  if(failed) return USBD_FAIL;
+  if(cdc_app_started)
+  {
+    USBD_CDC_ItfTypeDef *fops = (USBD_CDC_ItfTypeDef *)pdev->pUserData;
+    if(fops == NULL || fops->DeInit == NULL || fops->DeInit() != 0) return USBD_FAIL;
+    cdc_app_started = 0U;
+  }
+  pdev->pClassData = NULL;
+  USBD_memset(&cdc_storage, 0, sizeof(cdc_storage));
+  cdc_owner = NULL;
+  return USBD_OK;
 }
 
-/**
-  * @brief  USBD_CDC_Init
-  *         DeInitialize the CDC layer
-  * @param  pdev: device instance
-  * @param  cfgidx: Configuration index
-  * @retval status
-  */
-static uint8_t  USBD_CDC_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
+static uint8_t USBD_CDC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
 {
-  uint8_t ret = 0U;
-
-  /* Close EP IN */
-  USBD_LL_CloseEP(pdev, CDC_IN_EP);
-  pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 0U;
-
-  /* Close EP OUT */
-  USBD_LL_CloseEP(pdev, CDC_OUT_EP);
-  pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 0U;
-
-  /* Close Command IN EP */
-  USBD_LL_CloseEP(pdev, CDC_CMD_EP);
-  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 0U;
-
-  /* DeInit  physical Interface components */
-  if(pdev->pClassData != NULL)
-  {
-    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->DeInit();
-    USBD_free(pdev->pClassData);
-    pdev->pClassData = NULL;
-  }
+  (void)cfgidx;
+  if(pdev == NULL || cdc_owner != NULL || cdc_busy || pdev->pClassData != NULL) return USBD_FAIL;
+  USBD_CDC_ItfTypeDef *fops = (USBD_CDC_ItfTypeDef *)pdev->pUserData;
+  if(fops == NULL || fops->Init == NULL || fops->DeInit == NULL) return USBD_FAIL;
+  if(pdev->ep_in[CDC_IN_EP & 0xFU].is_used || pdev->ep_out[CDC_OUT_EP & 0xFU].is_used ||
+     pdev->ep_in[CDC_CMD_EP & 0xFU].is_used) return USBD_FAIL;
+
+  cdc_busy = 1U;
+  cdc_owner = pdev;
+  USBD_memset(&cdc_storage, 0, sizeof(cdc_storage));
+  pdev->pClassData = &cdc_storage;
+  uint16_t packet = pdev->dev_speed == USBD_SPEED_HIGH ? CDC_DATA_HS_IN_PACKET_SIZE : CDC_DATA_FS_IN_PACKET_SIZE;
+  pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 1U; /* Include a partially opened endpoint in cleanup. */
+  if(USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK, packet) != USBD_OK) goto fail;
+  packet = pdev->dev_speed == USBD_SPEED_HIGH ? CDC_DATA_HS_OUT_PACKET_SIZE : CDC_DATA_FS_OUT_PACKET_SIZE;
+  pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 1U; /* Include a partially opened endpoint in cleanup. */
+  if(USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK, packet) != USBD_OK) goto fail;
+  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 1U; /* Include a partially opened endpoint in cleanup. */
+  if(USBD_LL_OpenEP(pdev, CDC_CMD_EP, USBD_EP_TYPE_INTR, CDC_CMD_PACKET_SIZE) != USBD_OK) goto fail;
+
+  cdc_app_started = 1U; /* Failed application init may still need cleanup. */
+  if(fops->Init() != 0 || cdc_storage.RxBuffer == NULL) goto fail;
+  cdc_storage.TxState = 0U;
+  cdc_storage.RxState = 0U;
+  if(USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, cdc_storage.RxBuffer, packet) != USBD_OK) goto fail;
+  cdc_busy = 0U;
+  return USBD_OK;
+fail:
+  (void)USBD_CDC_Release(pdev);
+  cdc_busy = 0U;
+  return USBD_FAIL;
+}
 
+static uint8_t USBD_CDC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
+{
+  (void)cfgidx;
+  if(pdev == NULL || cdc_busy) return USBD_FAIL;
+  if(cdc_owner == NULL) return pdev->pClassData == NULL ? USBD_OK : USBD_FAIL;
+  if(cdc_owner != pdev || pdev->pClassData != &cdc_storage) return USBD_FAIL;
+  cdc_busy = 1U;
+  uint8_t ret = USBD_CDC_Release(pdev);
+  cdc_busy = 0U;
   return ret;
 }
 
diff --git a/tests/safety/README.md b/tests/safety/README.md
index ca1e980..02beb24 100644
--- a/tests/safety/README.md
+++ b/tests/safety/README.md
@@ -13,11 +13,11 @@ granular, as required by sbrk; malloc owns chunk alignment. Invalid growth,
 shrink below start, corrupted bounds and integer wrap are rejected with
 `(caddr_t)-1` and `errno=ENOMEM`, leaving the break unchanged. This does not make
 newlib reentrant or make formatting on allocation failure safe. No allocator or
-formatting call is added to interrupt handlers. Existing USB configuration can allocate/free in USB interrupt context while main
-formatting may allocate. Newlib locks remain no-ops: concurrent allocator access
-is an unresolved deployment blocker. This patch bounds the program break only;
-it does not make allocator metadata or `_sbrk` calls reentrant. Do not enable
-concurrent allocation workloads without a separately reviewed solution.
+formatting call is added to interrupt handlers. The Phase B baseline used heap allocation during USB configuration in interrupt
+context, while main formatting could allocate with no-op newlib locks. The fixed
+USB class storage introduced in Phase C removes that identified asynchronous heap
+caller. This bounds-checking allocator itself is still not reentrant: do not add
+new concurrent allocation workloads without a separately reviewed solution.
 
 Diagnostics default OFF in CMake (`LXR_RUNTIME_DIAGNOSTICS`) and in the header.
 Normal presets remain unchanged. Example isolated build:
diff --git a/tests/usb_allocator/README.md b/tests/usb_allocator/README.md
new file mode 100644
index 0000000..4918fb1
--- /dev/null
+++ b/tests/usb_allocator/README.md
@@ -0,0 +1,51 @@
+# Phase C USB class storage regression
+
+Run `python3 tests/usb_allocator/run.py` using native GCC, Python 3 and nm.
+ASan/UBSan must be available. No hardware, USB device, target register, flashing
+utility or network access is used. Temporary outputs are outside the repository.
+
+The runner extracts unchanged production bodies at runtime: class init/release/
+deinit and buffer setters; application Init/DeInit callbacks; core configuration,
+reset, disconnect and stop; and standard SET_CONFIGURATION handling. It uses real
+project USB/STM32 headers. LL endpoint open/close/receive, stop and control responses
+are explicit host mocks. USB heap aliases abort if called. The complete class TU
+is also compiled and its undefined symbols checked for allocator dependencies.
+
+Coverage includes 2,000 simulated FS/HS lifecycle cycles with duplicate config,
+deconfig, reset, disconnect/reconnect, stop/restart, stable pointer/typed alignment,
+complete zeroing on fresh configuration/cleanup, foreign ownership, callback
+re-entry, malformed inputs and fourteen rollback/retention/retry failure scenarios.
+Errors are injected at all three opens/closes, application init/deinit and receive
+preparation, including compound cleanup failures and missing application buffers.
+These are source regressions, not physical USB enumeration or packet tests.
+
+Production owns one typed `USBD_CDC_HandleTypeDef` static object for the single
+registered device/class. Its target size is 540 bytes, alignment 4; host pointer
+width makes it 552 bytes/alignment 8. The typed object prevents size/alignment drift.
+One owner pointer and two byte flags track lifetime and nonblocking re-entry.
+Existing core lifecycle dispatch is serialized in the USB context; the byte flag
+is not a mutex or a cross-context lifecycle lock. Application buffer callbacks
+remain unchanged and use hUsbDeviceFS, matching the registered single FS device.
+
+A direct duplicate Init fails before touching live endpoints/state. The USB core
+still ACKs repeated SET_CONFIGURATION with the same value without calling Init.
+DeInit with no active owner and NULL class data is idempotent. Foreign/invalid
+pointers are never freed. Cleanup closes the fixed three endpoints before calling
+application DeInit, then clears pClassData and storage and releases ownership.
+Failed endpoint/app cleanup retains storage and ownership; Init rejects reuse until
+a later DeInit/reset can finish. Core functions that ignore a class cleanup return
+are unchanged: a host reset/deconfig may be needed after failure. Permanent HAL
+failure is not claimed to recover physically. No allocator or unbounded wait is
+used by this correction.
+
+Descriptors, endpoints, class registration and all class code following lifecycle
+functions remain unchanged. Global USBD_malloc/free aliases remain definitions but
+have no compiled class users. Adding another class/device or an asynchronous
+lifecycle caller requires ownership and allocator review. Data APIs retain their
+existing configured-class preconditions; this patch is not a complete USB data-path
+concurrency redesign. It removes the identified USB-vs-formatting allocator path,
+not all possible future library reentrancy risks.
+
+Run the Bug 001, Euclidean and safety suites as well. Build normal and diagnostic
+Release/Debug and validate all images with tests/euclidean/check_images.py.
+LXR_RUNTIME_DIAGNOSTICS remains OFF by default. No image is uploaded by these tests.
diff --git a/tests/usb_allocator/host.c b/tests/usb_allocator/host.c
new file mode 100644
index 0000000..039bcb8
--- /dev/null
+++ b/tests/usb_allocator/host.c
@@ -0,0 +1,177 @@
+/* Simulated lifecycle, not a physical USB test. Production bodies are extracted. */
+#include <assert.h>
+#include <stdint.h>
+#include <stdio.h>
+#include <stdlib.h>
+#include <string.h>
+#include "usbd_core.h"
+#include "usbd_cdc.h"
+#include "usbd_ctlreq.h"
+#include "usbd_ioreq.h"
+
+USBD_HandleTypeDef hUsbDeviceFS;
+USBD_ClassTypeDef USBD_CDC;
+uint8_t UserRxBufferFS[2048], UserTxBufferFS[2048];
+static unsigned opens, closes, starts, stops, prepares, errors, acknowledgements;
+static int fail_open, fail_close, fail_prepare, fail_init, fail_deinit, missing_rx;
+static unsigned open_sequence[3], open_sizes[3], reenter;
+static uint8_t USBD_CDC_Init(USBD_HandleTypeDef *,uint8_t);
+static uint8_t USBD_CDC_DeInit(USBD_HandleTypeDef *,uint8_t);
+static int8_t test_init(void);
+static int8_t test_deinit(void);
+static USBD_CDC_ItfTypeDef fops={.Init=test_init,.DeInit=test_deinit};
+
+static void *forbidden_allocate(size_t size) { (void)size; abort(); }
+static void forbidden_free(void *p) { (void)p; abort(); }
+#undef USBD_malloc
+#undef USBD_free
+#define USBD_malloc forbidden_allocate
+#define USBD_free forbidden_free
+USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *p,uint8_t ep,uint8_t type,uint16_t size) {
+    (void)p; (void)type;
+    if(!(ep&15)) return USBD_OK; /* EP0 opened by actual reset body. */
+    open_sequence[opens%3]=ep; open_sizes[opens%3]=size; opens++;
+    if(fail_open && --fail_open==0) return USBD_FAIL;
+    return USBD_OK;
+}
+USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *p,uint8_t ep) {
+    (void)p; (void)ep; closes++;
+    if(fail_close && --fail_close==0) return USBD_FAIL;
+    return USBD_OK;
+}
+USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *p,uint8_t ep,uint8_t *buffer,uint16_t size) {
+    assert(p==&hUsbDeviceFS && ep==CDC_OUT_EP && buffer==UserRxBufferFS);
+    assert(size==(p->dev_speed==USBD_SPEED_HIGH?CDC_DATA_HS_OUT_PACKET_SIZE:CDC_DATA_FS_OUT_PACKET_SIZE));
+    prepares++; return fail_prepare?USBD_FAIL:USBD_OK;
+}
+USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *p) { (void)p; return USBD_OK; }
+void USBD_CtlError(USBD_HandleTypeDef *p,USBD_SetupReqTypedef *r) { (void)p;(void)r;errors++; }
+USBD_StatusTypeDef USBD_CtlSendStatus(USBD_HandleTypeDef *p) { (void)p;acknowledgements++;return USBD_OK; }
+#include "functions.inc"
+
+static void check_zero(void) {
+    unsigned char expected[sizeof(cdc_storage)]={0};
+    assert(!memcmp(expected,&cdc_storage,sizeof(cdc_storage)));
+}
+static int8_t test_init(void) {
+    assert(hUsbDeviceFS.pClassData==&cdc_storage && cdc_owner==&hUsbDeviceFS && cdc_busy);
+    assert((uintptr_t)&cdc_storage % _Alignof(USBD_CDC_HandleTypeDef)==0);
+    check_zero(); starts++;
+    if(reenter) {
+        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
+        assert(USBD_CDC_DeInit(&hUsbDeviceFS,1)==USBD_FAIL);
+    }
+    int8_t ret=CDC_Init_FS();
+    if(missing_rx) cdc_storage.RxBuffer=NULL;
+    return fail_init?USBD_FAIL:ret;
+}
+static int8_t test_deinit(void) {
+    assert(hUsbDeviceFS.pClassData==&cdc_storage && cdc_owner==&hUsbDeviceFS && cdc_busy);
+    stops++;
+    if(reenter) {
+        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
+        assert(USBD_CDC_DeInit(&hUsbDeviceFS,1)==USBD_FAIL);
+    }
+    return fail_deinit?USBD_FAIL:CDC_DeInit_FS();
+}
+static void idle(void) {
+    assert(cdc_owner==NULL && hUsbDeviceFS.pClassData==NULL && !cdc_busy && !cdc_app_started);
+    assert(!hUsbDeviceFS.ep_in[CDC_IN_EP&15].is_used && !hUsbDeviceFS.ep_in[CDC_CMD_EP&15].is_used && !hUsbDeviceFS.ep_out[CDC_OUT_EP&15].is_used);
+    check_zero();
+}
+static void reset_device(unsigned speed) {
+    assert(!cdc_owner);
+    memset(&hUsbDeviceFS,0,sizeof(hUsbDeviceFS));
+    hUsbDeviceFS.pClass=&USBD_CDC; hUsbDeviceFS.pUserData=&fops;
+    hUsbDeviceFS.dev_speed=speed;hUsbDeviceFS.dev_state=USBD_STATE_ADDRESSED;
+    opens=closes=starts=stops=prepares=errors=acknowledgements=0;
+    fail_open=fail_close=fail_prepare=fail_init=fail_deinit=missing_rx=0; reenter=1;
+}
+static void configure(unsigned value) {
+    USBD_SetupReqTypedef req={.wValue=value};USBD_SetConfig(&hUsbDeviceFS,&req);
+}
+int main(void) {
+    (void)forbidden_allocate; (void)forbidden_free;
+    USBD_CDC.Init=USBD_CDC_Init;USBD_CDC.DeInit=USBD_CDC_DeInit;
+    reset_device(USBD_SPEED_FULL);
+    assert(USBD_CDC_Init(NULL,1)==USBD_FAIL && USBD_CDC_DeInit(NULL,1)==USBD_FAIL);
+    hUsbDeviceFS.pUserData=NULL; assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
+    hUsbDeviceFS.pUserData=&fops;fops.Init=NULL;assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);fops.Init=test_init;
+    fops.DeInit=NULL;assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);fops.DeInit=test_deinit;
+    hUsbDeviceFS.pClassData=UserRxBufferFS;
+    assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL && USBD_CDC_DeInit(&hUsbDeviceFS,1)==USBD_FAIL);
+    assert(hUsbDeviceFS.pClassData==UserRxBufferFS);hUsbDeviceFS.pClassData=NULL;
+    hUsbDeviceFS.ep_in[CDC_IN_EP&15].is_used=1;assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
+    hUsbDeviceFS.ep_in[CDC_IN_EP&15].is_used=0;assert(!opens && !starts);idle();
+    for(unsigned speed=0;speed<2;speed++) for(unsigned cycle=0;cycle<1000;cycle++) {
+        reset_device(speed?USBD_SPEED_HIGH:USBD_SPEED_FULL);
+        configure(1); assert(!errors && starts==1 && opens==3 && prepares==1);
+        assert(open_sequence[0]==CDC_IN_EP && open_sequence[1]==CDC_OUT_EP && open_sequence[2]==CDC_CMD_EP);
+        assert(open_sizes[0]==(speed?CDC_DATA_HS_IN_PACKET_SIZE:CDC_DATA_FS_IN_PACKET_SIZE));
+        assert(open_sizes[1]==(speed?CDC_DATA_HS_OUT_PACKET_SIZE:CDC_DATA_FS_OUT_PACKET_SIZE));
+        assert(open_sizes[2]==CDC_CMD_PACKET_SIZE);
+        assert(cdc_storage.RxBuffer==UserRxBufferFS && cdc_storage.TxBuffer==UserTxBufferFS);
+        configure(1);assert(opens==3 && starts==1); /* core ACKs unchanged configuration */
+        void *live=hUsbDeviceFS.pClassData;
+        cdc_storage.CmdOpCode=37;cdc_storage.TxState=1;cdc_storage.RxLength=100;
+        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL && hUsbDeviceFS.pClassData==live);
+        assert(cdc_storage.CmdOpCode==37 && cdc_storage.TxState==1 && cdc_storage.RxLength==100);
+        USBD_HandleTypeDef other={.pClass=&USBD_CDC,.pUserData=&fops};
+        assert(USBD_CDC_Init(&other,1)==USBD_FAIL && USBD_CDC_DeInit(&other,1)==USBD_FAIL && !other.pClassData);
+        if(cycle%4==0) configure(0);
+        else if(cycle%4==1) assert(USBD_LL_Reset(&hUsbDeviceFS)==USBD_OK);
+        else if(cycle%4==2) assert(USBD_LL_DevDisconnected(&hUsbDeviceFS)==USBD_OK);
+        else assert(USBD_Stop(&hUsbDeviceFS)==USBD_OK);
+        idle();assert(closes==3 && stops==1);
+        assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK && closes==3 && stops==1);
+        hUsbDeviceFS.dev_state=USBD_STATE_ADDRESSED;configure(1);
+        assert(starts==2 && hUsbDeviceFS.pClassData==live); /* stable storage, freshly zeroed */
+        configure(0);idle();
+    }
+    puts("PASS USB lifecycle: 2000 FS/HS cycles through actual configuration/reset/disconnect/stop bodies; duplicate/re-entry/foreign ownership and full zeroing");
+    unsigned failures=0;
+    for(unsigned kind=0;kind<6;kind++) {
+        reset_device(USBD_SPEED_FULL);
+        if(kind<3) fail_open=kind+1;
+        if(kind==3) fail_init=1;
+        if(kind==4) missing_rx=1;
+        if(kind==5) fail_prepare=1;
+        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);idle();
+        assert(opens==closes);fail_open=fail_init=missing_rx=fail_prepare=0;
+        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);
+        assert(USBD_CDC_DeInit(&hUsbDeviceFS,1)==USBD_OK);idle();failures++;
+    }
+    for(unsigned close=1;close<=3;close++) {
+        reset_device(USBD_SPEED_FULL);assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);
+        void *live=hUsbDeviceFS.pClassData;fail_close=close;
+        assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_FAIL && cdc_owner && hUsbDeviceFS.pClassData==live);
+        assert(!cdc_busy && !stops && cdc_app_started);
+        assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
+        assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
+    }
+    reset_device(USBD_SPEED_FULL);assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);fail_deinit=1;
+    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_FAIL && cdc_owner && hUsbDeviceFS.pClassData);
+    assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
+    fail_deinit=0;assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
+    reset_device(USBD_SPEED_FULL);fail_prepare=1;fail_close=1;
+    assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL && cdc_owner && hUsbDeviceFS.pClassData);
+    fail_prepare=0;assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL);
+    assert(USBD_LL_Reset(&hUsbDeviceFS)==USBD_OK);idle();failures++;
+    reset_device(USBD_SPEED_FULL);assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);
+    hUsbDeviceFS.pClassData=UserRxBufferFS;
+    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_FAIL && !closes && cdc_owner);
+    hUsbDeviceFS.pClassData=&cdc_storage;
+    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
+    reset_device(USBD_SPEED_FULL);assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_OK);
+    hUsbDeviceFS.pUserData=NULL;
+    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_FAIL && cdc_owner && hUsbDeviceFS.pClassData);
+    hUsbDeviceFS.pUserData=&fops;
+    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
+    reset_device(USBD_SPEED_FULL);fail_init=fail_deinit=1;
+    assert(USBD_CDC_Init(&hUsbDeviceFS,1)==USBD_FAIL && cdc_owner && cdc_app_started);
+    fail_init=fail_deinit=0;
+    assert(USBD_CDC_DeInit(&hUsbDeviceFS,0)==USBD_OK);idle();failures++;
+    reset_device(USBD_SPEED_FULL);configure(USBD_MAX_NUM_CONFIGURATION+1);assert(errors==1 && !opens);idle();
+    printf("PASS USB failures: %u rollback/retention/retry scenarios plus invalid requests; no allocator calls\n",failures);
+    printf("Host object: size=%zu alignment=%zu (target size/alignment checked separately in ELF)\n",sizeof(cdc_storage),_Alignof(USBD_CDC_HandleTypeDef));
+}
diff --git a/tests/usb_allocator/run.py b/tests/usb_allocator/run.py
new file mode 100644
index 0000000..81e8f76
--- /dev/null
+++ b/tests/usb_allocator/run.py
@@ -0,0 +1,45 @@
+#!/usr/bin/env python3
+"""ASan/UBSan tests of extracted production USB lifecycle, core and app functions."""
+import os
+from pathlib import Path
+import re
+import subprocess
+import tempfile
+ROOT=Path(__file__).resolve().parents[2]
+HERE=Path(__file__).resolve().parent
+CLASS='Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c'
+CORE='Middlewares/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c'
+REQUEST='Middlewares/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c'
+APP='USB_DEVICE_MIDI/App/usbd_cdc_if.c'
+def extract(path,name):
+    source=(ROOT/path).read_text(errors='replace')
+    masked=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),source,flags=re.S)
+    match=re.search(r'^(?:static\s+)?(?:uint8_t|int8_t|void|USBD_StatusTypeDef)\s+'+name+r'\s*\([^;]*?\)\s*\{',masked,re.M)
+    if not match: raise RuntimeError('Missing production function '+name)
+    depth,end=1,match.end()
+    while depth:
+        depth+=(masked[end]=='{')-(masked[end]=='}');end+=1
+    line=source.count('\n',0,match.start())+1
+    return f'#line {line} "{path}"\n'+source[match.start():end]+'\n'
+source=(ROOT/CLASS).read_text()
+# Keep actual typed storage/owner declarations, not a copied test implementation.
+preamble=source[source.index('/* One registered device/class instance:'):source.index('static uint8_t USBD_CDC_Release(')]
+body=preamble+''.join(extract(CLASS,n) for n in ['USBD_CDC_Release','USBD_CDC_Init','USBD_CDC_DeInit','USBD_CDC_SetTxBuffer','USBD_CDC_SetRxBuffer'])
+body+=''.join(extract(APP,n) for n in ['CDC_Init_FS','CDC_DeInit_FS'])
+body+=''.join(extract(CORE,n) for n in ['USBD_SetClassConfig','USBD_ClrClassConfig','USBD_LL_Reset','USBD_LL_DevDisconnected','USBD_Stop'])
+body+=extract(REQUEST,'USBD_SetConfig')
+includes=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc','USB_DEVICE_MIDI/Target','Middlewares/JS/STM32_USB_Device_Library/Core/Inc','Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc']
+with tempfile.TemporaryDirectory(prefix='lxr-usb-c1-') as directory:
+    temp=Path(directory);(temp/'functions.inc').write_text(body)
+    flags=[os.environ.get('CC','cc'),'-std=gnu11','-O1','-g','-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
+    flags+=['-I'+str(ROOT/p) for p in includes]+['-I'+directory]
+    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
+    subprocess.run(flags+[str(HERE/'host.c'),'-o',str(temp/'test')],check=True,cwd=ROOT,env=env)
+    subprocess.run([str(temp/'test')],check=True,cwd=ROOT,env=env,timeout=30)
+    # Compile the complete class TU too, so omitted/extracted paths cannot hide a heap reference.
+    obj=temp/'class.o'
+    subprocess.run(flags+['-c',str(ROOT/CLASS),'-o',str(obj)],check=True,cwd=ROOT,env=env)
+    symbols=subprocess.check_output(['nm','-u',str(obj)],text=True)
+    forbidden={'malloc','free','calloc','realloc','_sbrk','_malloc_r','_free_r','__malloc_lock'}
+    assert not forbidden.intersection(line.split()[-1] for line in symbols.splitlines()), symbols
+    print('PASS complete production USB class object: no heap/allocator-lock references')

```

## Appendix B. Exact changed-file summary

```text
 .../Class/CDC/Src/usbd_cdc.c                       | 166 +++++++++----------
 tests/safety/README.md                             |  10 +-
 tests/usb_allocator/README.md                      |  51 ++++++
 tests/usb_allocator/host.c                         | 177 +++++++++++++++++++++
 tests/usb_allocator/run.py                         |  45 ++++++
 5 files changed, 351 insertions(+), 98 deletions(-)
```

## Appendix C. Complete ARM class undefined-symbol audit and allocator locks

```text
         U USBD_CtlError
         U USBD_CtlPrepareRx
         U USBD_CtlSendData
         U USBD_LL_CloseEP
         U USBD_LL_GetRxDataSize
         U USBD_LL_OpenEP
         U USBD_LL_PrepareReceive
         U USBD_LL_Transmit
         U memset

build/release/LXR_V2_STM32F765.elf:     file format elf32-littlearm


Disassembly of section .text:

08009280 <__malloc_lock>:
 8009280:	4770      	bx	lr

Disassembly of section .itcm_text:

build/release/LXR_V2_STM32F765.elf:     file format elf32-littlearm


Disassembly of section .text:

08009284 <__malloc_unlock>:
 8009284:	4770      	bx	lr

Disassembly of section .itcm_text:

```

## Appendix D. Tools and executed build logs

Stock installation instructions printed by the build are omitted; none was executed.

### c-baseline-tools.log (exit 0)

```text
cmake version 3.31.10

CMake suite maintained and supported by Kitware (kitware.com/cmake).
1.13.0.git.kitware.jobserver-pipe-1
Python 3.12.14
arm-none-eabi-gcc (15:14.2.rel1-1) 14.2.1 20241119
Copyright (C) 2024 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


```

### c-base-release-config.log (exit 0)

```text
Preset CMake variables:

  CMAKE_BUILD_TYPE="Release"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/release

```

### c-base-debug-config.log (exit 0)

```text
Preset CMake variables:

  CMAKE_BUILD_TYPE="Debug"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.1s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/debug

```

### c-release-build.log (exit 0)

```text
[1/1] 
```

### c-debug-build.log (exit 0)

```text
[1/3] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15016 B        16 KB     91.65%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271576 B       384 KB     69.07%
           FLASH:      223984 B       992 KB     22.05%
   text	   data	    bss	    dec	    hex	filename
 275136	    852	 341112	 617100	  96a8c	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 275996 bytes, checksum 0xFF
[3/3] 
```

### c-on-release-build.log (exit 0)

```text
[1/3] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      224304 B       992 KB     22.08%
   text	   data	    bss	    dec	    hex	filename
 275528	    852	 341184	 617564	  96c5c	/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-release/LXR_V2_V1.7.img: 276388 bytes, checksum 0xF3
[3/3] 
```

### c-on-debug-build.log (exit 0)

```text
[1/3] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15088 B        16 KB     92.09%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271648 B       384 KB     69.08%
           FLASH:      224352 B       992 KB     22.09%
   text	   data	    bss	    dec	    hex	filename
 275576	    852	 341184	 617612	  96c8c	/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/diagnostics-debug/LXR_V2_V1.7.img: 276436 bytes, checksum 0xBD
[3/3] 
```

### c-release-pinned.log (exit 0)

```text
[1/1] 
```

### c-debug-pinned.log (exit 0)

```text
[1/1] 
```

### c-on-release-pinned.log (exit 0)

```text
[1/1] 
```

### c-on-debug-pinned.log (exit 0)

```text
[1/1] 
```
