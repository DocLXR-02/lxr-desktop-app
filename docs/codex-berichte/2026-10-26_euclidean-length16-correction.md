# LXR Firmware Lab — Euclidean length-16 correction and validated publication

Date: **2026-10-26 (UTC)**. Repository: **DocLXR-02/lxr-desktop-app**.

## 1. Status, authority and continuation information

The user explicitly authorized implementing, validating, committing and publishing a narrowly scoped Euclidean length-16 correction independently of Bug 001. The correction is published and its remote tree/commit contents have been verified. This report is separately published on a new report-only branch based directly on `lab-setup`; it does not carry the firmware changes itself.

| Item | Exact reference |
|---|---|
| Correction branch | `euclidean/length16-fix` |
| Correction commit | `67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72` |
| Correction tree | `c4a12cac375f5f9cd121a1f7c971f50c4b309183` |
| Sole correction parent and report branch base | `5829398cea5d337200393757e39a37d4d8148256` (`lab-setup`) |
| Main, protected from modification | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Existing Bug 001 branch, protected from modification | `bug001/filter-drive-fix`, `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0` |
| New report-only branch | `codex-reports/2026-10-26-euclidean-length16-correction` |
| New report file | `docs/codex-berichte/2026-10-26_euclidean-length16-correction.md` |
| Original clean checkout, preserved | `/workspace/lxr-desktop-app` on Bug 001 |
| Separate correction worktree | `/workspace/lxr-euclidean-fix` |
| Source version/build output label | CMake version `1.7`; `LXR_V2_V1.7.img` (local only) |

Correction URL: https://github.com/DocLXR-02/lxr-desktop-app/tree/euclidean/length16-fix

Immutable commit URL: https://github.com/DocLXR-02/lxr-desktop-app/commit/67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72

Instructions were read from [AGENTS.md on lab-setup](https://github.com/DocLXR-02/lxr-desktop-app/blob/lab-setup/AGENTS.md), blob `2f4d121f53a7055c1a30355235384a95ef920f1a`, and from the new worktree. The rules require protecting main, narrow diagnosis-led changes, separating hardware observations/source evidence/tests, and a self-contained verified handoff. The cloud-environment-runtime skill was used to inspect provisioned tools and network policy. No sub-agents were used.

Reference investigation, read before implementation: [2026-10-26 Euclidean length-16 investigation](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-euclidean-length16-investigation/docs/codex-berichte/2026-10-26_euclidean-length16-investigation.md), report commit `2a781a446e0f105d7864087205dc370ef3ab5bbb`.

No hardware was flashed or operated. No firmware executable was run on hardware or an emulator. Host tests execute extracted source functions with explicit host seams. No binary/build artifact was published, no PR or merge was created, no existing report was overwritten, no paid service was used, and no existing remote branch was moved. No claim of official v1.71 binary/source equivalence is made. The private support v1.72 remains outside the public reference.

## 2. Outcome and evidence classification

**Confirmed source defect before this correction:** Euclidean generation stored length16 as0, although `PatternSet.length` is a byte and ordinary track lengths are literal1…64. Main step advance treated0 as16, but flam/shift processing and note recording divided by the raw value; longest-track comparison and last-step UI also saw raw0. Both ordinary pattern and autosave files persisted zero and reintroduced it on load. Internal length setters/protocol writes lacked relevant bounds.

**Implemented:** all identified active length-ingestion paths now produce literal1…64, Euclidean output uses literal16, supported legacy0 input/file values decode to16, malformed length tails are not installed, and affected indices/counters are checked. A two-line CLZ zero guard removes the separately identified undefined builtin input. Valid rhythm masks and serialization layout remain unchanged in the tested domain.

**Executed validation:** all six ASan/UBSan groups pass; all 2,176 exact generator masks match the pinned pre-fix source baseline; complete current-format save/load/save and malformed-input matrices pass; Release and Debug ARM builds pass without warnings; generated local images and ELF memory layouts pass validation.

**Not established:** real LXR audio behavior, official v1.71/private1.72 behavior, worst-case timing/stack use, every historical file format, actual SD-card failure behavior, or complete transactional pattern loading. Those limitations are detailed below. No hardware observation was added in this task.

## 3. Technical reasoning and implemented scope

Line references in this report use **correction commit `67aaf037…`**. For a stable source link, append `<path>#L<number>` to `https://github.com/DocLXR-02/lxr-desktop-app/blob/67aaf0371fbd4083a0d5cdd88bdd1f1b2c413d72/`.

### 3.1 Canonical representation and ingress policy

`Core/Inc/Soundengine/Sequencer/sequencer.h` defines64 patterns,7 tracks,64 steps and4 bars; `PatternSet.length[64][7]` is `uint8_t`. Initialization/clear already writes literal16. The adopted invariant after initialization and supported ingestion is a literal1…64 per track. No new field, sentinel, format tag, file migration or getter-based dual representation was introduced.

| Input / context | Defined behavior |
|---|---|
| Euclidean length1…16 | Preserved literally in RAM;16 stays16 |
| Euclidean setter0 / >16 | Existing lower clamp0→1 retained; upper clamp to16 restored to the documented UI range |
| General setter / private length stream / file value0 | Legacy encoding decoded to16 |
| General length1…64 | Preserved |
| General setter65…255 | Ignore without changing existing valid length |
| File length65…255 | Fail load without installing any of the new length/kit tail |
| Private SysEx length65…255 | Consume this destination slot, do not write it; preserve alignment for subsequent bytes |
| Invalid affected track/pattern index | Reject writes; getters return safe defaults without indexing |
| Receive counter448 or greater | Ignore further length data without incrementing/wrapping; ordinary mode selection resets the stream |
| Completely absent V1 length tail | Accept the otherwise complete old body and set all seven lengths to16 |
| Partial V1 tail1…6 bytes | Fail safely instead of copying the last read byte into missing tracks |
| V2/V3/tmp missing length or kit data | Fail safely; do not install a partial length/kit tail |

The implementation assumes normal `seq_init()` initialization and valid state produced through the supported paths. It does not scan arbitrary corrupted memory for zeros, nor does it validate every unrelated direct structure write in the firmware.

### 3.2 Euclidean generator and explicit CLZ treatment

File: `Core/Src/Soundengine/Sequencer/EuklidGenerator.c`.

- `getObjSize():53–60` returns1 immediately when `group==0`, before `__CLZ`. The included CMSIS implementation maps `__CLZ` to `__builtin_clz`; zero is undefined for that C builtin, even though the ARM CLZ instruction's zero result is32. The existing intended path was `32-32=0`, then return1. The new guard gives that exact result without invoking the undefined input.
- `euklid_generate():144–170` and `euklid_transferPattern():232–254` validate track/pattern destinations and private length1…16 before indexing/transferring. Accessors and setters around `:173–217` guard track indices; pattern-taking setters guard pattern indices. The direct rotation helper `:220–230` rejects lengths outside the Euclidean domain before modulo/shift operations.
- `euklid_setLength():186–192` restores its old commented upper bound to16, preserving all documented1…16 UI inputs.
- The former `if(len==16) len=0` conversion is removed from transfer. The four generated16-bit masks are transferred as before; literal length is stored at `:248–251` for the current `USE_PATTERN_EDIT_BUFFER=0` configuration. The alternate disabled edit-buffer destination also receives the literal value.

The recursive rhythm algorithm body is unchanged, and a separate comparison verified that `euklid_calcRecursive()` matches the base source exactly. The smallest CLZ guard was necessary for a reliable unshimmed sanitizer regression and removes undefined C input on target too. The tests do not pretend the investigation's CLZ shim was a production fix: **candidate tests have no CLZ shim**. Only the optional old-baseline oracle build emulates ARM CLZ(0)=32, as described in the investigation.

### 3.3 General length setter/getter

File: `Core/Src/Soundengine/Sequencer/sequencer.c`.

`seq_setTrackLength():274–288` rejects track>=7 or shown-pattern>=64 before indexing, maps0→16, rejects>64, and otherwise stores the literal value. `seq_getTrackLength():291ff` rejects invalid indices and returns a safe16 default; for valid indices it returns the canonical stored value. No fallback is needed for ordinary initialized/decoded state.

Main playback step advance, longest-track selection, flam/shift processing and note recording algorithms were not modified. Fixing their producers/loaders corrects the raw-zero inconsistency without changing sequencer scheduling, quantization, flam/shift curves or unrelated MIDI behavior. Host tests exercise these original consumer bodies directly, including immediately after both legacy-file loaders.

### 3.4 Pattern length tail staging and I/O validation

File: `Core/Src/Preset/presetManager.c`.

New static helper `preset_readPatternLengths():303–330` reads the seven-byte length tail into a local array. It checks the actual read result and byte count, applies the V1 absence policy, decodes each present0 to16, rejects any value>64, and requires the associated kit byte for versions>1. **Only after the whole required tail is valid** does it assign the seven new lengths and optional kit byte.

Both active readers use this same helper:

- Temporary/autosave reader `preset_LoadTmpPat():332ff`, call at`:457` with version3.
- Active normal body reader `preset_readPatternData():1704ff`, call at`:1944` with parsed version. The obsolete `#if0` reader remains untouched.

The source readers previously checked many byte counts without checking `FRESULT`. Those specific active pattern-body reads (step data, timing, main masks, shuffle) now check both. Otherwise an I/O error with a full-looking/stale count could allow the parser to reach and install a tail after an error. This is limited to the two affected pattern loaders, not a broad SD subsystem rewrite.

The temporary reader now reads its pattern-index prefix into local `loadedPattern` (`:359–373`), checks I/O/count and range, then publishes it to UI/playback selection. An invalid/error prefix cannot place255 into `menu_playedPattern`. Valid selection behavior is preserved.

The named-file wrapper `preset_loadPatternSD():1997ff` rejects pattern indices>=64 before filename/name-array access. It validates both the eight-byte name read and six-byte version-probe read before parsing (`:2026`,`:2031`), preventing truncated prefix data from being parsed as an initialized tag.

Only lengths/associated kit are staged as a group. As in the base, step/mask/shuffle payloads are loaded progressively; a failed load can leave partial earlier payload changes, and a valid temporary prefix can select that pattern before the later body load fails. The correction guarantees that invalid/partial **lengths** are not installed; it does not claim all-or-nothing loading of an entire pattern or a system-wide SD error transaction.

### 3.5 Private/internal SysEx length stream

File: `Core/Src/Soundengine/MIDI/CortexParser.c:255–309`.

The active `SYSEX_RECEIVE_PAT_LEN_DATA` case now checks the16-bit receive counter against `NUM_PATTERN*NUM_TRACKS` (448) **before** dividing/narrowing it into pattern/track indices. At448 it stops advancing; values beyond448, including65535, are ignored unchanged rather than wrapping into an earlier pattern. Derived indices use `NUM_TRACKS` instead of literal7. A new mode selection retains the original counter-reset behavior.

Length0 decodes16;1…64 is accepted;>64 is ignored while advancing one legal slot so later bytes remain aligned. There is no new protocol ACK/error message or change to standard external MIDI CC/NRPN processing. This is the internal front-panel/Cortex transport identified in the investigation, not a claim that external DIN/USB SysEx routes into it. The rest of the handler's unrelated step-data protocol was not redesigned.

### 3.6 Changes deliberately avoided and independent Bug 001 status

The correction is directly based on `lab-setup`, not on the Bug 001 branch. There is exactly one correction parent and no merge. `MidiParser.c` is byte-for-byte identical to lab-setup. The existing Bug 001 branch remains at `7d9fe99…`; its tests and fixes were neither copied nor changed. This independent branch therefore still contains any unrelated baseline bugs, including the separate Morph/Drive defect addressed on Bug 001.

A source-body comparison against the base confirmed these are unchanged:

```text
sequencer.c: seq_nextStep, seq_processFlam, seq_addNote, seq_getLongestTrack
presetManager.c: preset_writePatternData, preset_savePatternSD, preset_saveTmpPat
EuklidGenerator.c: euklid_calcRecursive
Whole files: MidiParser.c, ResonantFilter.c, CMakeLists.txt,
             CMakePresets.json, STM32F765VGTX_FLASH.ld
```

There are no filter DSP, Drive curve, smoothing, Morph, clock scheduling or test changes on Bug 001. A future combination of the independently reviewed branches would require separate validation; no combined branch was built or published here.

## 4. File compatibility and serialization guarantees

The existing save functions are unmodified. Named current files remain:

```text
8-byte name + 6-byte "PatV03" + serialized body
body:
  7 tracks × [64 × sizeof(Step=7) + 64 timing bytes] = 3584 bytes
  7 tracks × 4 bars × uint16 main-step masks        =   56 bytes
  shuffle                                           1 byte
  track lengths                                     7 bytes
  associated kit                                    1 byte
body total                                       3649 bytes
named file total                                 3663 bytes
named length offset                              3655 (zero-based)
```

The temporary file remains one pattern-index byte plus that body: **3650 bytes**, length offset **3642**. No struct padding, file version bump, field reordering, migration pass or rewrite-on-load was introduced. Existing present0 bytes decode16; ordinary later saves write16 naturally. The host tests assert exact file equality after load/save, except the seven deliberate legacy-zero-to16 bytes; step/timing data, masks, shuffle, kit, name and tag are preserved.

V1/V2 compatibility tests use **synthetic source-defined bodies**, not captured user/official files. Versions1/2 omit V3 timing, and version2 requires a kit byte. A V1 body ending after shuffle now explicitly defaults lengths16; a partial tail is rejected rather than previously succeeding with reused bytes. This is an intentional safe behavior change for incomplete files. A malformed required current-format tail returns failure with prior valid lengths/kit retained. No universal compatibility claim is made for older layouts described only by comments or official v1.71/private1.72 serializers.

## 5. Automated host tests: exact commands, seams and results

New repository files:

- `tests/euclidean/host.c`: bounded host seams plus test scenarios around original/extracted production bodies.
- `tests/euclidean/run.py`: runtime extraction/build/runner; no Git/network required for ordinary regression execution.
- `tests/euclidean/check_images.py`: read-only image/header/vector/ELF bounds validation.
- `tests/euclidean/README.md`: usage, policies, fixture limits and baseline provenance.

Actually executed, final source state:

```sh
python3 tests/euclidean/run.py --record-baseline
python3 tests/euclidean/run.py
```

Both exited0. The optional baseline command requires the pinned Git base object and builds its old generator with the architectural-zero CLZ seam. Its exact2176-row mask transcript SHA-256 is:

```text
212cd0c7e3da155464507d1bf22e90c0e50d38f32700cfbfefcde8f3307ac883
```

The production candidate uses the same complete generator source without any intrinsic override. The ordinary runner checks all2176 output rows against that pinned hash, checks pulse counts and mask bounds, and checks literal lengths independently. The recursive algorithm and ordinary valid rotation behavior are preserved across the complete tested UI input domain, including the original retained-rotation behavior for rotation inputs rejected relative to a shorter length.

Native compiler: `cc (Debian 14.2.0-19) 14.2.0`. Python: `3.12.14`. Main flags: GNU C11, `-O1 -g`, real STM32/FatFs/project headers, `-fsanitize=address,undefined -fno-sanitize-recover=all`, frame pointers, `-fno-pie -no-pie`, function/data section GC and `-Werror=implicit-function-declaration`. Environment seam: `ASAN_OPTIONS=detect_leaks=0`. Each scenario uses a new process with a120-second timeout; no timeout, sanitizer report, native build warning or assertion failure occurred in final validation. Temporary host output directories are automatically removed.

Original bodies are extracted with original-source `#line` annotations. Included/extracted code covers generator, setter/getter, backup/restore, longest selection, main-step activity/setter, flam/shift, note recording, normal body and named wrappers, autosave wrappers, shared tail reader and private SysEx receiver. HAL timer, voice output, UI/routing and FatFs calls are host seams. FatFs is an in-memory stream that can deliberately return EOF or an I/O error with a full-looking byte count. No physical I/O or timer interrupt executes.

Final stdout:

```text
PASS generation: 2176 exact masks match lab-setup; literal lengths; CLZ unshimmed
PASS bounds: all byte-valued track/pattern/length inputs
PASS playback: 294912 flam/shift ticks; 4160 recording positions; mixed lengths
PASS persistence: 130 full named/autosave roundtrips, unchanged layout/payload
PASS malformed: 10210 truncation/range/I-O/legacy-tail cases; no partial lengths
PASS protocol: 180802 byte-value/counter boundary calls; no wrap
PASS all six ASan/UBSan groups
```

| Group | Coverage and evidence |
|---|---|
| Generation | Lengths1…16, pulses1…length, all16 UI rotation input values:2176 exact masks plus literal-length checks; no candidate CLZ shim |
| Bounds | All byte-valued invalid track/pattern indices, all byte-valued length setter inputs, valid Euclidean destinations across7 tracks×64 patterns, invalid direct rotation lengths |
| Playback/recording | All lengths1…64, first/last-step lookahead and previous-step wrap, flam0…5, all16 encoded shifts,24 sub-ticks:294912 calls; every step at25%/75% quantization:4160 recorded positions; mixed longest-track comparison |
| Persistence |65 length cases (legacy0 plus1…64)×named/tmp=130 full roundtrips; exact file bytes, masks/payloads, sizes/offsets; both legacy loaders followed directly by formerly failing playback/recording and mixed-length selection |
| Malformed |Every named/tmp byte truncation (7313), invalid65…255 in each length slot (2674),12 I/O cases,18 synthetic V1/V2 tails,193 invalid/error tmp-prefix cases=10210 |
| Protocol |All256 byte values over448 valid destinations plus extras, all counters448…65535 and mode restart=180802 handler calls; no wrap, illegal store, divide-by-zero or OOB |

The host playback matrix executes actual `seq_processFlam()` lookahead/wrap arithmetic and actual `seq_addNote()` quantization. It does **not** run a full `seq_tick()`/clock/song simulation or prove interrupt timing. UI decrement is covered indirectly by canonical getter data and a small arithmetic check; the full display/encoder event loop is not linked. V1/V2 named wrapper probing is not tested with actual historical files. These are explicit limits, not assumed passes.

The old expected UBSan failures from the investigation are no longer accepted as success here: **every candidate scenario must exit0**. The regression stops on any unexpected compiler/sanitizer/image failure.

## 6. Release/Debug ARM builds and image validation

Toolchain and build tools:

```text
arm-none-eabi-gcc (15:14.2.rel1-1) 14.2.1 20241119
CMake 3.31.10
Ninja 1.13.0.git.kitware.jobserver-pipe-1
```

Provisioned paths are used rather than installing tools or paid services. Actual clean-build commands, each exit0:

```sh
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH   cmake --preset release -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH   cmake --build --preset release --clean-first --parallel 2
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH   cmake --preset debug -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH   cmake --build --preset debug --clean-first --parallel 2
```

Both use the unchanged project optimization configuration (`-O3` for Release and Debug; Debug has debug metadata/DEBUG). After final diff review removed one unintended edit inside an obsolete commented function, both configurations were rebuilt incrementally, again exit0. That comment-only cleanup changed neither final image nor ELF hash. The final source tree is the published tree. Both complete clean build logs and the final rebuild logs contain **no `warning:` or `error:` lines**. All122 Ninja steps completed for each clean build. There is no claim that old Bug 001 build logs validate this branch.

Actually executed, exit0:

```sh
python3 tests/euclidean/check_images.py   --objdump /workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-objdump   build/release build/debug
```

The checker verifies:

- Eight-byte magic `LXRV2IMG`,16-byte header and zero reserved padding.
- Declared little-endian payload length, exact image length, and identical raw `.bin` payload.
- Checksum `0xFF - (sum(payload) & 0xFF)`.
- Payload fits the existing bootloader limit and load addresses fit the application flash interval.
- Initial stack alignment/range and Thumb reset-vector range.
- Every allocated ELF section fits its assigned ITCM/DTCM/RAM/FLASH address region; load sections fit the raw image; static minimum stack space remains.
- Required vector/code/data/TCM sections exist. The unchanged linker assertions also pass.

| Measurement | Release | Debug |
|---|---:|---:|
| Complete image bytes (header included) |276700|276748|
| Payload bytes |276684|276732|
| Header bytes |16|16|
| Checksum |`0xE9`|`0x2C`|
| Initial MSP |`0x20080000`|`0x20080000`|
| Reset vector |`0x08014E5D`|`0x08014E5D`|
| ITCM used / capacity |15008 /16384|15008 /16384|
| DTCM used / capacity |107392 /131072|107392 /131072|
| RAM used / capacity |271040 /393216|271040 /393216|
| Linker-reported FLASH allocation / capacity |224680 /1015808|224728 /1015808|

ITCM headroom is **1376 bytes** (91.60% used). DTCM headroom before minimum stack reserve is23680 bytes; RAM headroom before dynamic main-stack use is122176 bytes. The checker reserves at least1024 bytes beyond each corresponding static high-water. These are static/linker constraints, not measured worst-case call-stack or real-time execution guarantees.

The linker-reported FLASH use is not the complete raw payload footprint: explicit RAM/TCM load addresses contribute to the image. Both the full payload size and every allocated load section are separately range-checked. The image is much smaller than the992KiB application flash region. Do not confuse mkimage's printed payload byte count with complete image size.

Exact final checker JSON (local artifacts only):

```json
[
  {
    "checksum": "0xE9",
    "configuration": "release",
    "elf_sha256": "a8aae7deec978fdb1dc621fe8fdbc8718286663b977ce512110e99e80a04e305",
    "image_bytes": 276700,
    "image_sha256": "616ad1326d3def1eb19686591c5eb1cb29288bae7f627173dde3fe17bf98214b",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223828,
      "ITCM_RAM": 15008,
      "RAM": 271040
    },
    "payload_bytes": 276684,
    "reset_vector": "0x08014E5D",
    "result": "PASS"
  },
  {
    "checksum": "0x2C",
    "configuration": "debug",
    "elf_sha256": "49c828aca9bfd43aae323e5ee5334e83a57e1c9b4da1031ef0c298dcca82ae4c",
    "image_bytes": 276748,
    "image_sha256": "58fd059ba57173087d709fb058cd586df7a095562cd06a524489bcedd1811627",
    "initial_msp": "0x20080000",
    "magic": "LXRV2IMG",
    "memory_high_water_bytes": {
      "DTCM": 107392,
      "FLASH": 223876,
      "ITCM_RAM": 15008,
      "RAM": 271040
    },
    "payload_bytes": 276732,
    "reset_vector": "0x08014E5D",
    "result": "PASS"
  }
]
```

No `.img`, `.bin`, `.elf`, `.map`, cache, build log or generated function include is committed or uploaded. Generated files remain under ignored `build/release` and `build/debug`. The unchanged GitHub workflow triggers only on pushes to `lab-setup`, so correction/report branch publication does not trigger its artifact upload job. No workflow dispatch was performed. The build prints stock installation instructions; none was executed and they are not authorization to operate hardware.

## 7. Exact change list, review and publication

The complete reviewed diff is in Appendix A, including all new host-test sources. There are **eight files: four production and four test/documentation files**. Firmware build configuration, linker script and DSP are unchanged.

```text
 Core/Src/Preset/presetManager.c                  | 119 ++++-----
 Core/Src/Soundengine/MIDI/CortexParser.c         |  14 +-
 Core/Src/Soundengine/Sequencer/EuklidGenerator.c |  20 +-
 Core/Src/Soundengine/Sequencer/sequencer.c       |   9 +-
 tests/euclidean/README.md                        |  60 +++++
 tests/euclidean/check_images.py                  |  62 +++++
 tests/euclidean/host.c                           | 326 +++++++++++++++++++++++
 tests/euclidean/run.py                           | 100 +++++++
 8 files changed, 628 insertions(+), 82 deletions(-)
```

Per-file byte identity manifest:

| File | Git blob SHA | SHA-256 of bytes |
|---|---|---|
| `Core/Src/Preset/presetManager.c` | `3ee6814a35a9c798e849af5e967637eb15cc3b3f` | `2cafd8321a9ddc37e530a4ac83fd0e8e1e9f299cf34748ef331adb7010cd853d` |
| `Core/Src/Soundengine/MIDI/CortexParser.c` | `269b61760997c9d4d24e4243fc16a78e22ab1a03` | `dc3433d019bc8f764ef37a336d38028d8eb9ddcff5d04eb95df4d54767c78e68` |
| `Core/Src/Soundengine/Sequencer/EuklidGenerator.c` | `e5fcd00959b88e14f018138d1676b82cd7bd4a61` | `d81b456049f40993bbbc9179ef9bde22c02fc08354bcb0acb6c0dc64fe6b1e11` |
| `Core/Src/Soundengine/Sequencer/sequencer.c` | `be480d5febe55f63ba17e55bd441e4b6d514f45f` | `598570bbe61fba5f1432b93c2063a402cf89adda8d5b652b3315972f3960866a` |
| `tests/euclidean/README.md` | `465695458af420e31b924c60e99f711f471677c8` | `d0dd8c25b83fd3296650685622c1d47f60e4be4251d1eb22da3f14f1e393c17f` |
| `tests/euclidean/check_images.py` | `c971b0afd2572a440718d9565809cade18d45a88` | `76a72f7918309c91b4ae21caf1e3477f6160c38f6b2c7dc3839525fcc4ac48ee` |
| `tests/euclidean/host.c` | `d173290b6bf6ca0e2d089d5565e46fc10fbd4fb8` | `e05aad246fb6c5d6c14e2f0603db86376317a26ac7c19abadb00f307c299ed4f` |
| `tests/euclidean/run.py` | `a66c8366eb80bc6ef8ddc9a31cc0094b68275d73` | `f6c31b3cf25be76a1be4d6d04f6c6ff5831880e0f197720405bd5ce3c4c8a2bf` |

Publication used the connected GitHub authenticated Git-data API, not native `git push`; the native environment had no configured GitHub outbound identity. Validation completed before remote commit/branch creation. Base64 blob upload preserved the production files' CRLF endings and pre-existing non-UTF8 comment bytes exactly. Every returned blob SHA matched the local byte hash. The GitHub tree built on the exact lab-setup tree `e206244c314757c48254faff4bb663bd1186027c` matched local staged tree `c4a12cac…` exactly. A single-parent commit was created, then a new correction branch was created pointing to it. No existing ref update, force operation or merge was used.

The GitHub commit API and recursive tree were read back: exactly the eight allowed files changed, each remote blob matched its validated local counterpart, and the sole parent is lab-setup. The exact GitHub commit object was imported locally after verifying its hash, then only the new local correction branch was moved to that object; both worktrees are clean. The original Bug 001 worktree remains on its original commit.

For this report, a new report-only branch is based on `5829398…`, with only this new Markdown path added. The final publication process verifies exact report content readback, its one-parent/one-file commit, both branch heads, and every pre-existing remote ref against the before-publication snapshot. The report's own commit SHA is obtained during that operation and is supplied in the final response; the correction SHA above is fixed and independently verified.

## 8. Errors encountered, resolved limitations and unrun work

No candidate regression, firmware build or image check failed in final validation. Routine implementation/publication errors were resolved as follows:

- An editing-script assertion initially selected an inner disabled-reader `#else` instead of the active normal-reader branch. It stopped before writing `presetManager.c`; the anchor was corrected. The intended other three source changes were retained.
- `git diff --check` initially interpreted CRLF carriage returns as trailing whitespace. Production line endings were preserved; `git -c core.whitespace=cr-at-eol diff --check` passes. This was not repaired by rewriting entire source files.
- A guessed workflow filename did not exist; the actual unchanged workflow is `.github/workflows/lxr-build.yml`.
- `ninja` was initially queried outside its provisioned PATH. The CMake/Ninja tool directory was already used for successful builds; querying the absolute executable returned the version above.
- Final review found one broad text replacement also affected an obsolete commented loader. That edit was removed. Both configurations and host tests were rerun; artifacts were unchanged.
- A combined base64 manifest exceeded one command-output limit. It was read in bounded chunks, with exact sizes and Git blob SHA checks before publication.
- The recursive-tree connector response echoed the requested commit SHA in its top-level `sha` field. Verification instead uses the actual `commit.tree.sha`, Git-data commit tree SHA, and every listed blob; all match the local staged tree. No content mismatch was found.
- The first local reconstruction assumed a trailing newline in the API-created commit message; hash verification rejected it before any local branch moved. Reconstructing the exact message without an added newline matched `67aaf037…`; import then succeeded. No branch or work was discarded.

Remaining limitations and risks:

1. **No hardware validation:** audible behavior, actual sequencer timing, live editing, clocks and recording on the LXR remain unmeasured. Separate explicit authorization, validated backup and recovery plan are still required before hardware operation.
2. **Official equivalence unknown:** source CMake1.7 and these builds are not established as equivalent to official public1.71 or private support1.72. No official binary was acquired/examined in this task.
3. **Historical file coverage limited:** V1/V2 test bodies are synthetic. Real older/official user files may contain undocumented length-layout variants. Malformed partial legacy tails now fail intentionally. No users' files are rewritten as a migration.
4. **Partial payload loads remain possible:** lengths/kit are staged safely, but existing progressive step/mask/shuffle loading and temporary pattern selection are not a full transaction. General SD/power-loss behavior, filename failure paths, arbitrary associated-kit values, and unrelated parser fields are not comprehensively hardened here.
5. **Concurrency/real-time behavior not proven:** live generation still writes masks then length. No synchronization redesign or schedule change was made. No worst-case timing, heap/stack stress or interrupt-interleaving test occurred.
6. **Source consumer assumptions:** the invariant relies on initialized state and affected ingestion boundaries. Arbitrary memory corruption or unrelated unchecked indices elsewhere can still break consumer assumptions; this patch is not a general bounds audit of the entire firmware.
7. **Bug 001 remains separate:** this independent branch does not contain the Morph/Drive correction. Its coexistence can be reviewed/tested later without altering the preserved Bug 001 branch.
8. **ITCM space is finite:**1376 bytes remain with this toolchain. Successful build bounds checks do not reserve space for future features or establish performance on a different compiler.

No additional significant risk requiring forced publication was accepted. The targeted known defects are corrected and all requested software validations pass; the limitations above bound what has actually been validated.

Recommended next action: review the published source diff, length/file policies, exact regression evidence and independent branch relationship. Hardware execution remains a separately authorized later step. No PR, merge or hardware action is implied by publication.

## Appendix A. Complete approved production and regression diff

This is the exact tested eight-file change relative to lab-setup. It is source/Markdown only. Existing production CRLF bytes are normalized only for display inside this report; Git blob hashes above identify the exact published bytes. Pre-existing non-UTF8 comment bytes are preserved in the actual repository blobs.

```diff
diff --git a/Core/Src/Preset/presetManager.c b/Core/Src/Preset/presetManager.c
index 41ef096..3ee6814 100644
--- a/Core/Src/Preset/presetManager.c
+++ b/Core/Src/Preset/presetManager.c
@@ -299,6 +299,35 @@ FRESULT preset_LoadTmpKit()
 		return res;
 }
 
+// Read the existing length/kit tail without exposing partial or invalid lengths.
+static uint8_t preset_readPatternLengths(uint8_t patternNr, uint8_t dataVersion)
+{
+	if(patternNr >= NUM_PATTERN) return 0;
+	uint8_t lengths[NUM_TRACKS];
+	uint8_t kit = 0;
+	UINT bytesRead;
+	if(f_read((FIL*)&SDFile, lengths, sizeof(lengths), &bytesRead) != FR_OK) return 0;
+	if(dataVersion == 1 && bytesRead == 0)
+	{
+		// Untagged V1 files may have no length tail at all.
+		for(uint8_t trk=0; trk<NUM_TRACKS; trk++) lengths[trk] = 16;
+	}
+	else if(bytesRead != sizeof(lengths)) return 0;
+	for(uint8_t trk=0; trk<NUM_TRACKS; trk++)
+	{
+		if(lengths[trk] == 0) lengths[trk] = 16;
+		if(lengths[trk] > NUM_STEPS) return 0;
+	}
+	if(dataVersion > 1)
+	{
+		if(f_read((FIL*)&SDFile, &kit, sizeof(kit), &bytesRead) != FR_OK || bytesRead != sizeof(kit)) return 0;
+	}
+	for(uint8_t trk=0; trk<NUM_TRACKS; trk++)
+		seq_patternSet.length[patternNr][trk] = lengths[trk];
+	if(dataVersion > 1) seq_patternSet.kitNr[patternNr] = kit;
+	return 1;
+}
+
 FRESULT preset_LoadTmpPat()
 {
 
@@ -326,21 +355,22 @@ FRESULT preset_LoadTmpPat()
 				return res;
 		}
 
-		//read current preset nr
-		res = f_read((FIL*)&SDFile,(void*)&menu_playedPattern,1,&bytesRead);
-		if(bytesRead != 1)
+		// Validate the stored pattern index before publishing it to UI/playback state.
+		uint8_t loadedPattern;
+		res = f_read((FIL*)&SDFile,(void*)&loadedPattern,1,&bytesRead);
+		if(res != FR_OK || bytesRead != 1)
 		{
 			success = 0;
 			goto closeFile;
 		}
 
-		if(menu_playedPattern >= NUM_PATTERN)
+		if(loadedPattern >= NUM_PATTERN)
 		{
-			menu_playedPattern = 0;
 			success = 0;
 			goto closeFile;
 		}
 
+		menu_playedPattern = loadedPattern;
 		menu_shownPattern = menu_playedPattern;
 		seq_activePattern = menu_playedPattern;
 		seq_pendingPattern = menu_playedPattern;
@@ -358,7 +388,6 @@ FRESULT preset_LoadTmpPat()
 		// This means that loading old patterns with non-standard length, the length will be
 		// ignored and a standard length used. I think it's acceptable as long as users are aware of this.
 
-		uint8_t next=0;
 
 		// ---------- Load step data first
 
@@ -369,9 +398,7 @@ FRESULT preset_LoadTmpPat()
 		for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
 		{
 
-			f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead);
-
-			if(bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
+			if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
 			{
 				success=0;
 				break; // file error, or we ran out of file (eof)
@@ -380,9 +407,7 @@ FRESULT preset_LoadTmpPat()
 			//Pattern V3 and up => flam added
 			//if(dataVersion > 2)
 			{
-				f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead);
-
-				if(bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
+				if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
 				{
 					success=0;
 					break; // file error, or we ran out of file (eof)
@@ -401,9 +426,7 @@ FRESULT preset_LoadTmpPat()
 			{
 				for(uint8_t bar=0;bar<NUM_BARS; bar++)
 				{
-					f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead);
-
-					if( bytesRead==sizeof(uint16_t)) {
+					if(f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead) == FR_OK && bytesRead==sizeof(uint16_t)) {
 						seq_patternSet.seq_mainSteps[bar][patternNr][trk] = mainStepData;
 					}
 					 else {
@@ -418,9 +441,7 @@ FRESULT preset_LoadTmpPat()
 		// ------------ shuffle settings
 		if(success) {
 			//load the shuffle settings
-			f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead);
-			//and update on cortex
-			if(bytesRead)
+			if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead) == FR_OK && bytesRead == sizeof(uint8_t))
 			{
 
 				//activeParameters[PAR_SHUFFLE] = seq_patternSet.shuffle[patternNr];
@@ -433,25 +454,7 @@ FRESULT preset_LoadTmpPat()
 			}
 		}
 
-		// ----------- Pattern/track lengths
-		// -- AS this might not exist in the saved data file, we still want success
-		if(success) {
-
-				for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
-				{
-					f_read((FIL*)&SDFile,(void*)&next,sizeof(uint8_t),&bytesRead);
-					seq_patternSet.length[patternNr][trk] = next;
-				}
-
-			success=1; // just to document that we don't consider this a failure, we just didn't read the bytes for this
-		}
-
-		//if(dataVersion > 1)
-		{
-			f_read((FIL*)&SDFile,(void*)&seq_patternSet.kitNr[patternNr],sizeof(uint8_t),&bytesRead);
-			if(bytesRead == 0) success = 0;
-
-		}
+		if(success) success = preset_readPatternLengths(patternNr, 3);
 
 		if( success == 0)
 		{
@@ -1699,6 +1702,7 @@ FRESULT preset_savePatternSD(uint8_t presetNr)
 // returns 1 on success
 static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 {
+	if(patternNr >= NUM_PATTERN) return 0;
 
 #if 0
 
@@ -1871,7 +1875,6 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 
 	UINT bytesRead;
 	uint8_t success=1; // start off succeeding
-	uint8_t next=0;
 
 	// ---------- Load step data first
 
@@ -1882,9 +1885,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 	for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
 	{
 
-		f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead);
-
-		if(bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
+		if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_subStepPattern[patternNr][trk][0],sizeof(Step)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(Step)*(STEPS_PER_PATTERN))
 		{
 			success=0;
 			break; // file error, or we ran out of file (eof)
@@ -1893,9 +1894,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 		//Pattern V3 and up => flam added
 		if(dataVersion > 2)
 		{
-			f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead);
-
-			if(bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
+			if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.seq_stepTiming[patternNr][trk][0],sizeof(uint8_t)*STEPS_PER_PATTERN,&bytesRead) != FR_OK || bytesRead!=sizeof(uint8_t)*(STEPS_PER_PATTERN))
 			{
 				success=0;
 				break; // file error, or we ran out of file (eof)
@@ -1914,9 +1913,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 		{
 			for(uint8_t bar=0;bar<NUM_BARS; bar++)
 			{
-				f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead);
-
-				if( bytesRead==sizeof(uint16_t)) {
+				if(f_read((FIL*)&SDFile,(void*)&mainStepData,sizeof(uint16_t),&bytesRead) == FR_OK && bytesRead==sizeof(uint16_t)) {
 					seq_patternSet.seq_mainSteps[bar][patternNr][trk] = mainStepData;
 				}
 				 else {
@@ -1931,9 +1928,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 	// ------------ shuffle settings
 	if(success) {
 		//load the shuffle settings
-		f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead);
-		//and update on cortex
-		if(bytesRead)
+		if(f_read((FIL*)&SDFile,(void*)&seq_patternSet.shuffle[patternNr],sizeof(uint8_t),&bytesRead) == FR_OK && bytesRead == sizeof(uint8_t))
 		{
 
 			//activeParameters[PAR_SHUFFLE] = seq_patternSet.shuffle[patternNr];
@@ -1946,25 +1941,7 @@ static uint8_t preset_readPatternData(uint8_t patternNr, uint8_t dataVersion)
 		}
 	}
 
-	// ----------- Pattern/track lengths
-	// -- AS this might not exist in the saved data file, we still want success
-	if(success) {
-
-			for(uint8_t trk=0; trk<NUM_TRACKS;trk++)
-			{
-				f_read((FIL*)&SDFile,(void*)&next,sizeof(uint8_t),&bytesRead);
-				seq_patternSet.length[patternNr][trk] = next;
-			}
-
-		success=1; // just to document that we don't consider this a failure, we just didn't read the bytes for this
-	}
-
-	if(dataVersion > 1)
-	{
-		f_read((FIL*)&SDFile,(void*)&seq_patternSet.kitNr[patternNr],sizeof(uint8_t),&bytesRead);
-		if(bytesRead == 0) success = 0;
-
-	}
+	if(success) success = preset_readPatternLengths(patternNr, dataVersion);
 
 	return success;
 #endif
@@ -2019,6 +1996,7 @@ frontPanel_sendData(PRESET,PATTERN_LOAD,presetNr);
 //----------------------------------------------------
 FRESULT preset_loadPatternSD(uint8_t presetNr)
 {
+	if(presetNr >= NUM_PATTERN) return FR_INVALID_PARAMETER;
 #if USE_SD_CARD		
 	//filename in 8.3  format
 	uint8_t fileVersion = 1;
@@ -2045,11 +2023,12 @@ FRESULT preset_loadPatternSD(uint8_t presetNr)
 	
 	//read the preset name
 	res = f_read((FIL*)&SDFile,(void*)savefileNames[presetNr][PATTERN_NAMES],8,&bytesRead);
-	if(bytesRead != 8)
+	if(res != FR_OK || bytesRead != 8)
 		goto closeFile;
 	
 	char buf[7];
 	res = f_read((FIL*)&SDFile,buf,6,&bytesRead);
+	if(res != FR_OK || bytesRead != 6) goto closeFile;
 	//zero termination
 	buf[6]=0;
 	fileVersion = atoi(&buf[4]);
diff --git a/Core/Src/Soundengine/MIDI/CortexParser.c b/Core/Src/Soundengine/MIDI/CortexParser.c
index 5a06962..269b617 100644
--- a/Core/Src/Soundengine/MIDI/CortexParser.c
+++ b/Core/Src/Soundengine/MIDI/CortexParser.c
@@ -255,10 +255,20 @@ static void frontParser_handleSysexData(unsigned char data)
 	case SYSEX_RECEIVE_PAT_LEN_DATA:
 		// same as above but we are receiving length data for each pattern
 		{
+			// Saturate at the end of the length stream; never wrap the receive counter.
+			if(frontParser_sysexSeqStepNr >= NUM_PATTERN * NUM_TRACKS) break;
 			//calculate the step pattern and track indices
-			const uint8_t currentPattern	= frontParser_sysexSeqStepNr / 7;
-			const uint8_t currentTrack  	= frontParser_sysexSeqStepNr - currentPattern*7;
+			const uint8_t currentPattern	= frontParser_sysexSeqStepNr / NUM_TRACKS;
+			const uint8_t currentTrack  	= frontParser_sysexSeqStepNr - currentPattern*NUM_TRACKS;
+
 
+			if(data == 0) data = 16; // legacy length encoding
+			if(data > NUM_STEPS)
+			{
+				frontParser_sysexSeqStepNr++;
+				frontParser_rxCnt = 0;
+				break;
+			}
 
 /*
 			if( (currentPattern == seq_activePattern) && seq_isRunning() )
diff --git a/Core/Src/Soundengine/Sequencer/EuklidGenerator.c b/Core/Src/Soundengine/Sequencer/EuklidGenerator.c
index a9c2f2a..e5fcd00 100644
--- a/Core/Src/Soundengine/Sequencer/EuklidGenerator.c
+++ b/Core/Src/Soundengine/Sequencer/EuklidGenerator.c
@@ -52,6 +52,8 @@ void euklid_init()
  */
 static inline uint8_t getObjSize(uint16_t group)
 {
+	// The C builtin underlying __CLZ is undefined for zero.
+	if(group == 0) return 1;
 	uint8_t ret = 32 - __CLZ((uint32_t)group);
 	if(ret==0) return 1;
 	return ret;
@@ -141,6 +143,8 @@ void euklid_calcRecursive(uint8_t length, uint8_t steps, uint8_t iteration, uint
 //-----------------------------------------------------
 void euklid_generate(uint8_t trackNr, uint8_t patternNr)
 {
+	if(trackNr >= NUM_TRACKS || patternNr >= NUM_PATTERN) return;
+	if(euklid_length[trackNr] < 1 || euklid_length[trackNr] > 16) return;
 	uint8_t length,steps,rotation;
 	length = euklid_length[trackNr];
 	steps = euklid_steps[trackNr];
@@ -168,24 +172,28 @@ void euklid_generate(uint8_t trackNr, uint8_t patternNr)
 //-----------------------------------------------------
 uint8_t euklid_getLength(uint8_t trackNr)
 {
+	if(trackNr >= NUM_TRACKS) return 16;
 	return euklid_length[trackNr];
 }
 //-----------------------------------------------------
 uint8_t euklid_getSteps(uint8_t trackNr)
 {
+	if(trackNr >= NUM_TRACKS) return 1;
 	return euklid_steps[trackNr];
 };
 
 //-----------------------------------------------------
 void euklid_setLength(uint8_t trackNr, uint8_t value)
 {
+	if(trackNr >= NUM_TRACKS) return;
 	if(value<=0)value=1;
-	//else if(value>16)value=16;
+	else if(value>16)value=16;
 	euklid_length[trackNr] = value;
 }
 //-----------------------------------------------------
 void euklid_setSteps(uint8_t trackNr, uint8_t value, uint8_t patternNr)
 {
+	if(trackNr >= NUM_TRACKS || patternNr >= NUM_PATTERN) return;
 	if(value<=0)value=1;
 
 	if(value>euklid_length[trackNr]) value= euklid_length[trackNr];
@@ -196,11 +204,13 @@ void euklid_setSteps(uint8_t trackNr, uint8_t value, uint8_t patternNr)
 //-----------------------------------------------------
 uint8_t euklid_getRotation(uint8_t trackNr)
 {
+	if(trackNr >= NUM_TRACKS) return 0;
 	return euklid_rotation[trackNr];
 }
 //-----------------------------------------------------
 void euklid_setRotation(uint8_t trackNr, uint8_t value, uint8_t patternNr)
 {
+	if(trackNr >= NUM_TRACKS || patternNr >= NUM_PATTERN) return;
 	if(value>euklid_length[trackNr]) value = euklid_rotation[trackNr];
 
 	euklid_rotation[trackNr] = value;
@@ -209,6 +219,7 @@ void euklid_setRotation(uint8_t trackNr, uint8_t value, uint8_t patternNr)
 //-----------------------------------------------------
 void euklid_rotatePattern(uint8_t length, uint8_t amount)
 {
+	if(length < 1 || length > 16) return;
 	if( amount==0 || length==1 || amount%length==0  ) return; //Nothing to be done
 
 	amount = amount%length;
@@ -220,6 +231,8 @@ void euklid_rotatePattern(uint8_t length, uint8_t amount)
 //-----------------------------------------------------
 void euklid_transferPattern(uint8_t trackNr, uint8_t patternNr)
 {
+	if(trackNr >= NUM_TRACKS || patternNr >= NUM_PATTERN) return;
+	if(euklid_length[trackNr] < 1 || euklid_length[trackNr] > 16) return;
 	uint8_t len=euklid_length[trackNr];
 	for(int i=0; i<NUM_BARS; i++)
 	{
@@ -230,10 +243,7 @@ void euklid_transferPattern(uint8_t trackNr, uint8_t patternNr)
 #endif
 
 	}
-// **PATROT - pattern end is now stored differently
-
-	if(len == 16)
-		len=0;
+	// Track lengths are literal 1..64 values, including length 16.
 #if USE_PATTERN_EDIT_BUFFER
 	patternBackupBuffer.length[trackNr]=len;
 #else
diff --git a/Core/Src/Soundengine/Sequencer/sequencer.c b/Core/Src/Soundengine/Sequencer/sequencer.c
index 9b51d96..be480d5 100644
--- a/Core/Src/Soundengine/Sequencer/sequencer.c
+++ b/Core/Src/Soundengine/Sequencer/sequencer.c
@@ -273,11 +273,9 @@ void seq_setShuffle(float shuffle)
 //------------------------------------------------------------------------------
 void seq_setTrackLength(uint8_t trackNr, uint8_t length)
 {
-	//set new end marker
-	/*
-	if(length == 16)
-		length=0;
-	*/
+	if(trackNr >= NUM_TRACKS || frontParser_shownPattern >= NUM_PATTERN) return;
+	if(length == 0) length = 16; // legacy input encoding
+	if(length > NUM_STEPS) return;
 	// **PATROT this was changed from setting length on seq_activePattern to shown (or edited) pattern
 
 #if USE_PATTERN_EDIT_BUFFER
@@ -292,6 +290,7 @@ void seq_setTrackLength(uint8_t trackNr, uint8_t length)
 //------------------------------------------------------------------------------
 uint8_t seq_getTrackLength(uint8_t trackNr)
 {
+	if(trackNr >= NUM_TRACKS || frontParser_shownPattern >= NUM_PATTERN) return 16;
 	// **PATROT this was changed from getting seq_activePattern to shown (or edited) pattern
 #if USE_PATTERN_EDIT_BUFFER
 	uint8_t r=patternBackupBuffer.length[trackNr];
diff --git a/tests/euclidean/README.md b/tests/euclidean/README.md
new file mode 100644
index 0000000..4656954
--- /dev/null
+++ b/tests/euclidean/README.md
@@ -0,0 +1,60 @@
+# Euclidean length regression
+
+Run from the repository root:
+
+```sh
+python3 tests/euclidean/run.py
+```
+
+Requires Python 3 and a native GCC-compatible C compiler with ASan/UBSan (`CC`
+selects the compiler). No firmware or hardware executes. Temporary build outputs
+are automatically removed outside the repository. The candidate generator uses
+the original, unshimmed CMSIS intrinsic and its production zero guard.
+
+The fixture extracts original function bodies at runtime with source line
+annotations. HAL, audio output, display/routing and FatFs have bounded host seams.
+The normal named-file and autosave wrappers execute against an in-memory byte
+stream. No SD card, target interrupts or full clock/song engine is simulated.
+
+The six groups cover:
+
+- 2,176 exact rhythm masks for lengths 1–16, pulses 1–length, rotation inputs
+  0–15 (including the original retained-rotation behavior for rejected inputs).
+  The pinned SHA-256 oracle comes from `lab-setup` commit
+  `5829398cea5d337200393757e39a37d4d8148256`, with ARM CLZ(0)=32 emulation only
+  for that old baseline. To reproduce it where the base Git object is available,
+  run `python3 tests/euclidean/run.py --record-baseline`. Normal tests do not need
+  Git history or networking. Review the oracle before changing it.
+- All byte-valued track/pattern/length boundary inputs, literal lengths 1–64,
+  legacy zero decoding, and invalid setter rejection.
+- Playback next/previous wrap arithmetic across all lengths, flam 0–5, all
+  16 encoded shifts, both boundary steps and all 24 sub-ticks; recording at
+  every step before/after half-step quantization; mixed longest-track selection.
+- Full named PatV03 and temporary pattern save/load/save, including legacy zero,
+  all lengths 1–64, payload/mask preservation, exact bytes and layout. Named
+  file size stays 3663 with length offset 3655; tmp size stays 3650, offset 3642.
+- Every current-format byte truncation, invalid lengths 65–255 on every track,
+  I/O errors with full-looking counts, invalid temporary pattern prefixes, and
+  synthetic V1/V2 length tails. The loader installs no partial length/kit tail.
+  A wholly absent V1 length tail defaults to 16; partial tails fail. V2/V3/tmp
+  require all seven lengths and the kit byte. Real historical files still need
+  separate compatibility validation.
+- All byte-valued internal SysEx lengths across 448 legal slots, extra bytes,
+  every counter value 448–65535, and mode restart. Invalid length values consume
+  their slot without writing; counters stop advancing at the end of the stream.
+
+After building Release and Debug, validate the local images without uploading:
+
+```sh
+python3 tests/euclidean/check_images.py build/release build/debug
+```
+
+Use `--objdump /path/to/arm-none-eabi-objdump` when absent from PATH. This checks
+header/magic, payload length, checksum, raw-bin identity, vectors, allocated ELF
+section bounds, load addresses and minimum stack space. Linker checks remain
+active. Static memory checks do not establish worst-case runtime stack use or
+real-time audio performance. Do not flash or publish binaries through these tests.
+
+Both scripts stop on any unexpected compiler, sanitizer, assertion or image error.
+The host fixture is source regression coverage, not an equivalence claim for
+public official v1.71 or private support firmware.
diff --git a/tests/euclidean/check_images.py b/tests/euclidean/check_images.py
new file mode 100644
index 0000000..c971b0a
--- /dev/null
+++ b/tests/euclidean/check_images.py
@@ -0,0 +1,62 @@
+#!/usr/bin/env python3
+"""Read-only desktop update-image and ELF memory validation; never flash/upload."""
+import argparse
+import hashlib
+import json
+from pathlib import Path
+import re
+import struct
+import subprocess
+
+REGIONS = {'ITCM_RAM': (0, 16*1024), 'DTCM': (0x20000000, 128*1024),
+           'RAM': (0x20020000, 384*1024), 'FLASH': (0x08008000, 992*1024)}
+
+def check(directory, objdump):
+    image = (directory / 'LXR_V2_V1.7.img').read_bytes()
+    binary = (directory / 'LXR_V2_STM32F765.bin').read_bytes()
+    elf = directory / 'LXR_V2_STM32F765.elf'
+    magic, length, checksum, padding = struct.unpack('<8sIB3s', image[:16])
+    payload = image[16:]
+    assert magic == b'LXRV2IMG' and padding == b'\0\0\0'
+    assert length == len(payload) == len(binary) and payload == binary
+    assert len(image) == length+16 and 0 < length < 0x080fffff-0x08008000
+    assert checksum == 0xff-(sum(payload) & 0xff)
+    msp, reset = struct.unpack('<II', payload[:8])
+    assert 0x20020000 < msp <= 0x20080000 and msp % 8 == 0
+    assert reset & 1 and 0x08008000 <= (reset & ~1) < 0x08008000+length
+    listing = subprocess.run([objdump, '-h', str(elf)], check=True,
+                             capture_output=True, text=True).stdout
+    high = {name: 0 for name in REGIONS}
+    sections = []
+    for match in re.finditer(r'^\s*\d+\s+(\S+)\s+([\da-f]+)\s+([\da-f]+)\s+([\da-f]+)[^\n]*\n([^\n]*)', listing, re.M):
+        name, size, vma, lma, flags = match.groups()
+        size, vma, lma = (int(x,16) for x in (size,vma,lma))
+        if not size or 'ALLOC' not in flags:
+            continue
+        regions = [(label,base,capacity) for label,(base,capacity) in REGIONS.items()
+                   if base <= vma and vma+size <= base+capacity]
+        assert len(regions)==1, f'{name} outside memory regions'
+        label,base,capacity = regions[0]
+        high[label] = max(high[label],vma+size-base)
+        if 'LOAD' in flags:
+            assert 0x08008000 <= lma and lma+size <= 0x08008000+length
+        sections.append(name)
+    assert {'.isr_vector','.text','.data','.bss','.itcm_text','.dtcm','.dtcmz'} <= set(sections)
+    assert high['DTCM']+0x400 <= REGIONS['DTCM'][1], 'minimum DTCM stack reserve'
+    assert high['RAM']+0x400 <= REGIONS['RAM'][1], 'minimum main stack reserve'
+    return dict(configuration=directory.name, magic=magic.decode(), image_bytes=len(image),
+                payload_bytes=length, checksum=f'0x{checksum:02X}', initial_msp=f'0x{msp:08X}',
+                reset_vector=f'0x{reset:08X}', memory_high_water_bytes=high,
+                image_sha256=hashlib.sha256(image).hexdigest(),
+                elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest(), result='PASS')
+
+def main():
+    parser = argparse.ArgumentParser(description=__doc__)
+    parser.add_argument('--objdump', default='arm-none-eabi-objdump')
+    parser.add_argument('directories', nargs='+', type=Path)
+    args = parser.parse_args()
+    for directory in args.directories:
+        print(json.dumps(check(directory,args.objdump),sort_keys=True))
+
+if __name__ == '__main__':
+    main()
diff --git a/tests/euclidean/host.c b/tests/euclidean/host.c
new file mode 100644
index 0000000..d173290
--- /dev/null
+++ b/tests/euclidean/host.c
@@ -0,0 +1,326 @@
+// Host seams around original source bodies; never execute firmware or touch hardware.
+#include <assert.h>
+#include <inttypes.h>
+#include <stdio.h>
+#include <stdlib.h>
+#include <string.h>
+#include "Soundengine/Sequencer/sequencer.h"
+#include "Soundengine/MIDI/CortexParser.h"
+#include "Soundengine/MIDI/MidiMessages.h"
+#include "Soundengine/MIDI/MidiVoiceControl.h"
+#include "Preset/PresetManager.h"
+#include "menu/menu.h"
+#include "frontPanelParser_ui.h"
+#include "ledHandler.h"
+#ifdef BASELINE_GENERATOR
+// Oracle only: model ARM CLZ(0)=32 for the pre-fix builtin. Candidate has no shim.
+#undef __CLZ
+#define __CLZ(x) ((uint32_t)(x) ? __builtin_clz((uint32_t)(x)) : 32)
+#include "baseline-generator.inc"
+#else
+#include "Core/Src/Soundengine/Sequencer/EuklidGenerator.c"
+#endif
+
+PatternSet seq_patternSet;
+PatternBackupBuffer patternBackupBuffer;
+uint8_t seq_activePattern, seq_pendingPattern, seq_running, seq_recordActive;
+static uint8_t seq_mutedTracks;
+static int8_t seq_stepIndex[NUM_TRACKS];
+int8_t seq_stepToSkip[NUM_TRACKS];
+uint8_t lastWasNoteOn[NUM_TRACKS], chokeCurrent, chokeNext;
+struct { uint8_t prev, current, next; } seq_rndValue[NUM_TRACKS];
+uint8_t frontParser_shownPattern, frontParser_activeTrack;
+uint8_t frontParser_sysexActive, frontParser_rxCnt;
+uint16_t frontParser_sysexSeqStepNr, frontParser_twoByteData;
+static uint8_t frontParser_sysexBuffer[7];
+uint8_t menu_activeBar, menu_activePage, menu_playedPattern, menu_shownPattern;
+uint8_t menu_selectedPresetNr[NUM_PRESET_LOCATIONS];
+char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];
+TIM_HandleTypeDef htim2;
+FIL SDFile;
+#define STEPS_PER_PATTERN NUM_STEPS
+#define FEXT_PAT 1
+static uint8_t stream[65536], saved_stream[65536];
+static size_t stream_pos, stream_size, error_at = SIZE_MAX;
+static float elapsed = 75.f;
+static unsigned triggers;
+FRESULT f_open(FIL *file, const TCHAR *name, BYTE flags) {
+    (void)name; stream_pos=0; file->fptr=0;
+    if(flags & FA_CREATE_ALWAYS) stream_size=0;
+    return FR_OK;
+}
+FRESULT f_close(FIL *file) { (void)file; return FR_OK; }
+FRESULT f_lseek(FIL *file, DWORD offset) {
+    assert(offset<=stream_size); stream_pos=offset; file->fptr=offset; return FR_OK;
+}
+FRESULT f_read(FIL *file, void *dst, UINT count, UINT *actual) {
+    assert(stream_pos<=stream_size);
+    size_t old=stream_pos;
+    size_t n=count < stream_size-stream_pos ? count : stream_size-stream_pos;
+    memcpy(dst,stream+stream_pos,n); stream_pos+=n; file->fptr=stream_pos; *actual=n;
+    // Deliberately return an error with a full byte count too: callers must check both.
+    return old>=error_at ? FR_DISK_ERR : FR_OK;
+}
+FRESULT f_write(FIL *file, const void *src, UINT count, UINT *actual) {
+    assert(stream_pos+count<=sizeof(stream)); memcpy(stream+stream_pos,src,count);
+    stream_pos+=count; file->fptr=stream_pos;
+    if(stream_pos>stream_size) stream_size=stream_pos;
+    *actual=count; return FR_OK;
+}
+static FRESULT preset_makeFileNameLoad(char *buf,uint8_t num,uint8_t type) {
+    (void)buf; (void)num; (void)type; return f_open(&SDFile,"pattern",FA_READ);
+}
+static void preset_makeFileNameSave(char *buf,uint8_t num,uint8_t type,char *name) {
+    (void)num; (void)type; (void)name; strcpy(buf,"pattern");
+}
+static void preset_makeFileNameOld(char *buf,uint8_t num,uint8_t type) {
+    (void)num; (void)type; strcpy(buf,"pattern");
+}
+HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *timer) { (void)timer; return HAL_OK; }
+HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *timer) { (void)timer; return HAL_OK; }
+void menu_repaintAll(void) {}
+void seq_sendStepInfoToFront(uint16_t step) { (void)step; }
+void frontPanel_sendData(uint8_t a,uint8_t b,uint8_t c) {
+    if(a==SEQ_CC && b==SEQ_SET_SHOWN_PATTERN) frontParser_shownPattern=c;
+}
+void seq_triggerVoice(uint8_t voice,uint8_t vel,uint8_t note) {
+    assert(voice<NUM_TRACKS && vel<=127 && note==60); triggers++;
+}
+void voiceControl_noteOff(uint8_t voice) { assert(voice<NUM_TRACKS); }
+float seq_getElapsedStepTimePercent(void) { return elapsed; }
+uint8_t seq_determineNextPattern(uint8_t change) { (void)change; abort(); }
+void led_setValue(uint8_t value,uint8_t led) { (void)value; (void)led; }
+#include "functions.inc"
+
+static void reset(void) {
+    memset(&seq_patternSet,0,sizeof(seq_patternSet));
+    for(unsigned p=0;p<NUM_PATTERN;p++) for(unsigned t=0;t<NUM_TRACKS;t++)
+        seq_patternSet.length[p][t]=16;
+    seq_activePattern=seq_pendingPattern=frontParser_shownPattern=menu_playedPattern=0;
+    frontParser_activeTrack=menu_activeBar=menu_shownPattern=0;
+    menu_activePage=PERF_PAGE; seq_running=seq_recordActive=1; seq_mutedTracks=0x7e;
+    memset(seq_stepIndex,0,sizeof(seq_stepIndex)); error_at=SIZE_MAX;
+    stream_pos=stream_size=0; elapsed=75.f; triggers=0; euklid_init();
+}
+static void lengths_unchanged(unsigned pattern) {
+    for(unsigned t=0;t<NUM_TRACKS;t++) assert(seq_patternSet.length[pattern][t]==t+1);
+}
+static void old_lengths(unsigned pattern) {
+    for(unsigned t=0;t<NUM_TRACKS;t++) seq_patternSet.length[pattern][t]=t+1;
+}
+static void generation(void) {
+    for(unsigned length=1;length<=16;length++) for(unsigned pulses=1;pulses<=length;pulses++) {
+        euklid_init(); euklid_setLength(0,length); euklid_setSteps(0,pulses,0);
+        for(unsigned rotation=0;rotation<16;rotation++) {
+            euklid_setRotation(0,rotation,0);
+#ifndef BASELINE_GENERATOR
+            assert(seq_patternSet.length[0][0]==length);
+#endif
+            assert(__builtin_popcountll(euklid_patternBuffer.quadword)==pulses);
+            assert((euklid_patternBuffer.quadword>>length)==0);
+            printf("%u,%u,%u,%016" PRIx64 "\n",length,pulses,rotation,euklid_patternBuffer.quadword);
+        }
+    }
+}
+static void bounds(void) {
+    for(unsigned t=0;t<NUM_TRACKS;t++) for(unsigned p=0;p<NUM_PATTERN;p++) {
+        euklid_setLength(t,16); euklid_setSteps(t,4,p);
+        assert(seq_patternSet.length[p][t]==16);
+    }
+    for(unsigned bad=NUM_TRACKS;bad<=255;bad++) {
+        euklid_setLength(bad,255); euklid_setSteps(bad,3,0); euklid_setRotation(bad,1,0);
+        euklid_generate(bad,0); euklid_transferPattern(bad,0);
+        assert(euklid_getLength(bad)==16 && euklid_getSteps(bad)==1 && euklid_getRotation(bad)==0);
+        seq_setTrackLength(bad,5); assert(seq_getTrackLength(bad)==16);
+    }
+    for(unsigned bad=NUM_PATTERN;bad<=255;bad++) {
+        euklid_setSteps(0,3,bad); euklid_setRotation(0,1,bad);
+        euklid_generate(0,bad); euklid_transferPattern(0,bad);
+        frontParser_shownPattern=bad; seq_setTrackLength(0,3); assert(seq_getTrackLength(0)==16);
+        assert(!preset_readPatternData(bad,3)); assert(preset_loadPatternSD(bad)==FR_INVALID_PARAMETER);
+    }
+    frontParser_shownPattern=0;
+    for(unsigned value=0;value<=255;value++) {
+        euklid_setLength(0,value);
+        assert(euklid_getLength(0)==(value==0?1:value>16?16:value));
+        seq_patternSet.length[0][0]=9; seq_setTrackLength(0,value);
+        assert(seq_patternSet.length[0][0]==(value==0?16:value<=64?value:9));
+    }
+    euklid_rotatePattern(0,1); euklid_rotatePattern(64,1);
+    puts("PASS bounds: all byte-valued track/pattern/length inputs");
+}
+static void playback(void) {
+    unsigned ticks=0,notes=0;
+    for(unsigned length=1;length<=64;length++) {
+        seq_setTrackLength(0,length);
+        for(unsigned i=0;i<64;i++) {
+            seq_setMainStep(0,0,i,1);
+            seq_patternSet.seq_subStepPattern[0][0][i].note=60;
+            seq_patternSet.seq_subStepPattern[0][0][i].volume=100;
+            seq_patternSet.seq_subStepPattern[0][0][i].prob=100;
+        }
+        for(unsigned flam=0;flam<=5;flam++) for(unsigned shift=0;shift<16;shift++) {
+            for(unsigned i=0;i<64;i++) {
+                seq_patternSet.seq_stepTiming[0][0][i].flam=flam;
+                seq_patternSet.seq_stepTiming[0][0][i].shift=shift;
+            }
+            for(unsigned side=0;side<2;side++) {
+                seq_stepIndex[0]=side?length-1:0;
+                for(unsigned tick=0;tick<24;tick++) { seq_processFlam(tick); ticks++; }
+            }
+        }
+        for(unsigned step=0;step<length;step++) for(unsigned phase=0;phase<2;phase++) {
+            elapsed=phase?75.f:25.f; seq_stepIndex[0]=step;
+            unsigned expected=(step+phase)%length;
+            memset(seq_patternSet.seq_subStepPattern[0][0],0,sizeof(seq_patternSet.seq_subStepPattern[0][0]));
+            seq_addNote(0,100,60);
+            assert(seq_patternSet.seq_subStepPattern[0][0][expected].note==60);
+            assert(seq_isMainStepActive(0,expected,0)); notes++;
+        }
+    }
+    seq_setTrackLength(0,0); assert(seq_getTrackLength(0)==16);
+    uint8_t last=seq_getTrackLength(0); last-=1; seq_setTrackLength(0,last); assert(seq_getTrackLength(0)==15);
+    seq_patternSet.length[0][0]=16; seq_patternSet.length[0][1]=8;
+    for(unsigned t=2;t<7;t++) seq_patternSet.length[0][t]=1;
+    assert(seq_getLongestTrack()==0);
+    seq_patternSet.length[0][6]=64; assert(seq_getLongestTrack()==6);
+    assert(triggers>0);
+    printf("PASS playback: %u flam/shift ticks; %u recording positions; mixed lengths\n",ticks,notes);
+}
+static void fill_pattern(unsigned p,unsigned length) {
+    for(unsigned t=0;t<7;t++) {
+        seq_patternSet.length[p][t]=length;
+        for(unsigned i=0;i<64;i++) {
+            Step *step=&seq_patternSet.seq_subStepPattern[p][t][i];
+            memset(step,(i+t)%128,sizeof(*step));
+            seq_patternSet.seq_stepTiming[p][t][i].flam=(i+t)%6;
+            seq_patternSet.seq_stepTiming[p][t][i].shift=(i+t)%15;
+        }
+        for(unsigned b=0;b<4;b++) seq_patternSet.seq_mainSteps[b][p][t]=0xa55a^(b*7+t);
+    }
+    seq_patternSet.shuffle[p]=23; seq_patternSet.kitNr[p]=5;
+    memcpy(savefileNames[p][PATTERN_NAMES],"HOSTTEST",8);
+}
+static void roundtrips(void) {
+    unsigned cases=0;
+    for(unsigned tmp=0;tmp<2;tmp++) for(unsigned length=0;length<=64;length++) {
+        unsigned p=tmp?63:0; menu_playedPattern=p; frontParser_shownPattern=p;
+        fill_pattern(p,length?length:16);
+        assert((tmp?preset_saveTmpPat():preset_savePatternSD(p))==FR_OK);
+        size_t size=stream_size,offset=tmp?3642:3655;
+        assert(size==(tmp?3650:3663));
+        if(!tmp) assert(!memcmp(stream,"HOSTTESTPatV03",14));
+        assert(stream[offset]==(length?length:16));
+        // Construct a legacy zero-bearing file without using an invalid RAM state.
+        if(length==0) memset(stream+offset,0,NUM_TRACKS);
+        memcpy(saved_stream,stream,size);
+        Step saved_steps[NUM_TRACKS][NUM_STEPS]; StepTiming saved_timing[NUM_TRACKS][NUM_STEPS];
+        memcpy(saved_steps,seq_patternSet.seq_subStepPattern[p],sizeof(saved_steps));
+        memcpy(saved_timing,seq_patternSet.seq_stepTiming[p],sizeof(saved_timing));
+        old_lengths(p); stream_pos=0; error_at=SIZE_MAX;
+        assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(p))==FR_OK);
+        for(unsigned t=0;t<7;t++) assert(seq_patternSet.length[p][t]==(length?length:16));
+        assert(!memcmp(saved_steps,seq_patternSet.seq_subStepPattern[p],sizeof(saved_steps)));
+        assert(!memcmp(saved_timing,seq_patternSet.seq_stepTiming[p],sizeof(saved_timing)));
+        for(unsigned t=0;t<7;t++) for(unsigned b=0;b<4;b++) assert(seq_patternSet.seq_mainSteps[b][p][t]==(0xa55a^(b*7+t)));
+        assert(seq_patternSet.shuffle[p]==23 && seq_patternSet.kitNr[p]==5);
+        assert((tmp?preset_saveTmpPat():preset_savePatternSD(p))==FR_OK);
+        if(length==0) memset(saved_stream+offset,16,NUM_TRACKS);
+        assert(stream_size==size && !memcmp(stream,saved_stream,size)); cases++;
+        if(length==0) {
+            // Exercise the formerly failing consumers directly after each legacy loader.
+            seq_activePattern=p;
+            for(unsigned i=0;i<64;i++) seq_patternSet.seq_subStepPattern[p][0][i].note=60;
+            seq_stepIndex[0]=0; seq_processFlam(0);
+            seq_stepIndex[0]=15; elapsed=75.f; seq_addNote(0,100,60);
+            assert(seq_patternSet.seq_subStepPattern[p][0][0].note==60);
+            seq_patternSet.length[p][1]=8;
+            for(unsigned t=2;t<7;t++) seq_patternSet.length[p][t]=1;
+            assert(seq_getLongestTrack()==0);
+        }
+    }
+    printf("PASS persistence: %u full named/autosave roundtrips, unchanged layout/payload\n",cases);
+}
+static void malformed(void) {
+    unsigned cases=0;
+    for(unsigned tmp=0;tmp<2;tmp++) {
+        reset(); fill_pattern(0,16);
+        assert((tmp?preset_saveTmpPat():preset_savePatternSD(0))==FR_OK);
+        size_t size=stream_size,offset=tmp?3642:3655; memcpy(saved_stream,stream,size);
+        for(size_t cut=0;cut<size;cut++) {
+            stream_size=cut; old_lengths(0);
+            assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(0))!=FR_OK);
+            lengths_unchanged(0); cases++;
+        }
+        stream_size=size;
+        for(unsigned t=0;t<7;t++) for(unsigned bad=65;bad<=255;bad++) {
+            memcpy(stream,saved_stream,size); stream[offset+t]=bad; old_lengths(0);
+            assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(0))!=FR_OK);
+            lengths_unchanged(0); cases++;
+        }
+        memcpy(stream,saved_stream,size);
+        // Error injection covers every read start, including successful-looking counts.
+        size_t points[]={0,tmp?1:14,offset-57,offset-1,offset,offset+7};
+        for(unsigned i=0;i<sizeof(points)/sizeof(points[0]);i++) {
+            error_at=points[i]; old_lengths(0);
+            assert((tmp?preset_LoadTmpPat():preset_loadPatternSD(0))!=FR_OK);
+            lengths_unchanged(0); cases++;
+        }
+        error_at=SIZE_MAX;
+    }
+    // V1/V2 synthetic bodies use the source-defined no-timing layout.
+    const size_t old_offset=7*64*sizeof(Step)+7*4*2+1;
+    for(unsigned version=1;version<=2;version++) for(unsigned tail=0;tail<=8;tail++) {
+        memset(stream,0,sizeof(stream)); stream_size=old_offset+tail; stream_pos=0;
+        if(tail) memset(stream+old_offset,8,tail);
+        old_lengths(0); unsigned ok=preset_readPatternData(0,version);
+        unsigned expected=(version==1 && (tail==0 || tail>=7)) || (version==2 && tail==8);
+        assert(!!ok==!!expected);
+        if(expected) for(unsigned t=0;t<7;t++) assert(seq_patternSet.length[0][t]==(tail?8:16));
+        else lengths_unchanged(0);
+        cases++;
+    }
+    menu_playedPattern=menu_shownPattern=seq_activePattern=seq_pendingPattern=3;
+    for(unsigned invalid=64;invalid<=255;invalid++) {
+        stream[0]=invalid; stream_size=1; assert(preset_LoadTmpPat()!=FR_OK);
+        assert(menu_playedPattern==3 && menu_shownPattern==3 && seq_activePattern==3 && seq_pendingPattern==3);
+    }
+    error_at=0; stream[0]=255; assert(preset_LoadTmpPat()!=FR_OK);
+    assert(menu_playedPattern==3 && seq_activePattern==3); error_at=SIZE_MAX;
+    printf("PASS malformed: %u truncation/range/I-O/legacy-tail cases; no partial lengths\n",cases+193);
+}
+static void protocol(void) {
+    unsigned calls=0;
+    for(unsigned input=0;input<=255;input++) {
+        frontParser_sysexActive=SYSEX_RECEIVE_PAT_LEN_DATA; frontParser_sysexSeqStepNr=0;
+        for(unsigned slot=0;slot<NUM_PATTERN*NUM_TRACKS;slot++) {
+            unsigned p=slot/7,t=slot%7; seq_patternSet.length[p][t]=9;
+            frontParser_handleSysexData(input);
+            assert(seq_patternSet.length[p][t]==(input==0?16:input<=64?input:9));
+            assert(frontParser_sysexSeqStepNr==slot+1); calls++;
+        }
+        for(unsigned extra=0;extra<4;extra++) {
+            frontParser_handleSysexData(1); assert(frontParser_sysexSeqStepNr==448); calls++;
+        }
+    }
+    for(unsigned counter=448;counter<=65535;counter++) {
+        frontParser_sysexSeqStepNr=counter; frontParser_handleSysexData(16);
+        assert(frontParser_sysexSeqStepNr==counter); calls++;
+    }
+    // Existing mode-selection mechanism restarts a new legal stream.
+    frontParser_sysexActive=SYSEX_ACTIVE_MODE_NONE; frontParser_handleSysexData(SYSEX_RECEIVE_PAT_LEN_DATA);
+    assert(frontParser_sysexSeqStepNr==0); frontParser_handleSysexData(0);
+    assert(frontParser_sysexSeqStepNr==1 && seq_patternSet.length[0][0]==16);
+    printf("PASS protocol: %u byte-value/counter boundary calls; no wrap\n",calls+2);
+}
+int main(int argc,char **argv) {
+    assert(argc==2); reset();
+    if(!strcmp(argv[1],"generation")) generation();
+    else if(!strcmp(argv[1],"bounds")) bounds();
+    else if(!strcmp(argv[1],"playback")) playback();
+    else if(!strcmp(argv[1],"roundtrips")) roundtrips();
+    else if(!strcmp(argv[1],"malformed")) malformed();
+    else if(!strcmp(argv[1],"protocol")) protocol();
+    else abort();
+    return 0;
+}
diff --git a/tests/euclidean/run.py b/tests/euclidean/run.py
new file mode 100644
index 0000000..a66c836
--- /dev/null
+++ b/tests/euclidean/run.py
@@ -0,0 +1,100 @@
+#!/usr/bin/env python3
+"""Sanitized host regression of original source bodies. No firmware execution."""
+import argparse
+import hashlib
+import os
+from pathlib import Path
+import re
+import subprocess
+import tempfile
+
+ROOT = Path(__file__).resolve().parents[2]
+HERE = Path(__file__).resolve().parent
+BASE = '5829398cea5d337200393757e39a37d4d8148256'
+GENERATOR = 'Core/Src/Soundengine/Sequencer/EuklidGenerator.c'
+# SHA-256 of 2176 exact length,pulse,rotation,mask lines from lab-setup,
+# with only host CLZ(0)=32 emulation. --record-baseline reproduces the oracle.
+EXPECTED_MASKS = '212cd0c7e3da155464507d1bf22e90c0e50d38f32700cfbfefcde8f3307ac883'
+
+def extract(path, name):
+    source = (ROOT / path).read_text(errors='replace')
+    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
+                    lambda m: ' ' * len(m[0]), source, flags=re.S)
+    match = re.search(r'^(?:IN_ITCM_RAM\s+)?(?:static\s+)?(?:inline\s+)?'
+                      r'(?:void|float|FRESULT|uint\d+_t|int\d+_t)\s+'
+                      + name + r'\([^;]*?\)\s*\{', masked, re.M)
+    if not match:
+        raise RuntimeError('Cannot extract ' + name)
+    depth, end = 1, match.end()
+    while depth:
+        depth += (masked[end] == '{') - (masked[end] == '}')
+        end += 1
+    line = source.count('\n', 0, match.start()) + 1
+    return f'#line {line} "{path}"\n' + source[match.start():end] + '\n'
+
+def run(command, **kwargs):
+    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
+                            timeout=120, **kwargs)
+    if result.returncode:
+        raise RuntimeError(f'{command[0]} exited {result.returncode}\n'
+                           + result.stdout + result.stderr)
+    return result
+
+def main():
+    parser = argparse.ArgumentParser(description=__doc__)
+    parser.add_argument('--record-baseline', action='store_true',
+                        help='requires the pinned base git object; print oracle hash only')
+    args = parser.parse_args()
+    seq = 'Core/Src/Soundengine/Sequencer/sequencer.c'
+    preset = 'Core/Src/Preset/presetManager.c'
+    functions = [(seq, name) for name in (
+        'seq_setTrackLength', 'seq_getTrackLength', 'seq_getLongestTrack',
+        'seq_copyPatternToBackupBuffer', 'seq_copyBackupBufferToPattern',
+        'getShiftValue', 'flamIdxToPattern', 'seq_intIsMainStepActive',
+        'seq_isMainStepActive', 'seq_setMainStep', 'seq_getShiftFlamPattern',
+        'seq_processFlam', 'seq_addNote')]
+    functions += [(preset, name) for name in (
+        'preset_readPatternLengths', 'preset_writePatternData',
+        'preset_readPatternData', 'preset_savePatternSD', 'preset_loadPatternSD',
+        'preset_saveTmpPat', 'preset_LoadTmpPat')]
+    functions += [('Core/Src/Soundengine/MIDI/CortexParser.c', 'frontParser_handleSysexData')]
+    includes = ['.', 'Core/Inc', 'Drivers/CMSIS/Include',
+                'Drivers/CMSIS/Device/ST/STM32F7xx/Include',
+                'Drivers/STM32F7xx_HAL_Driver/Inc',
+                'Middlewares/Third_Party/FatFs/src', 'FATFS/App', 'FATFS/Target']
+    with tempfile.TemporaryDirectory(prefix='lxr-euclidean-') as directory:
+        temporary = Path(directory)
+        (temporary / 'functions.inc').write_text(''.join(extract(*f) for f in functions))
+        compiler = os.environ.get('CC', 'cc')
+        flags = [compiler, '-std=gnu11', '-O1', '-g', '-DSTM32F765xx', '-DUSE_HAL_DRIVER',
+                 '-Wno-int-to-pointer-cast', '-Wno-pointer-to-int-cast',
+                 '-Werror=implicit-function-declaration', '-ffunction-sections',
+                 '-fdata-sections', '-fno-pie', '-no-pie', '-fsanitize=address,undefined',
+                 '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
+        flags += ['-I' + str(ROOT / p) for p in includes] + ['-I' + directory]
+        if args.record_baseline:
+            original = run(['git', 'show', BASE + ':' + GENERATOR]).stdout
+            (temporary / 'baseline-generator.inc').write_text(original)
+            flags += ['-DBASELINE_GENERATOR']
+        executable = temporary / 'host'
+        build = run(flags + [str(HERE / 'host.c'), '-Wl,--gc-sections', '-o', str(executable)])
+        if build.stderr:
+            print(build.stderr, end='')
+        env = os.environ.copy()
+        env['ASAN_OPTIONS'] = 'detect_leaks=0'
+        result = run([str(executable), 'generation'], env=env)
+        digest = hashlib.sha256(result.stdout.encode()).hexdigest()
+        assert len(result.stdout.splitlines()) == 2176
+        if args.record_baseline:
+            print('BASELINE_MASKS=' + digest)
+            return
+        assert digest == EXPECTED_MASKS, f'Rhythm masks changed: {digest}'
+        print('PASS generation: 2176 exact masks match lab-setup; literal lengths; CLZ unshimmed')
+        for scenario in ('bounds', 'playback', 'roundtrips', 'malformed', 'protocol'):
+            result = run([str(executable), scenario], env=env)
+            print(result.stdout, end='')
+            assert not result.stderr, result.stderr
+        print('PASS all six ASan/UBSan groups')
+
+if __name__ == '__main__':
+    main()
```

## Appendix B. Complete configure output and build summaries

### Release configure (exit0)

```text
Preset CMake variables:

  CMAKE_BUILD_TYPE="Release"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-euclidean-fix/cmake/arm-none-eabi.cmake"

-- The C compiler identification is GNU 14.2.1
-- The ASM compiler identification is GNU
-- Found assembler: /workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.3s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-euclidean-fix/build/release
```

### Debug configure (exit0)

```text
Preset CMake variables:

  CMAKE_BUILD_TYPE="Debug"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-euclidean-fix/cmake/arm-none-eabi.cmake"

-- The C compiler identification is GNU 14.2.1
-- The ASM compiler identification is GNU
-- Found assembler: /workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-euclidean-fix/build/debug
```

### Release clean build and final rebuild summaries (both exit0)

Repeated compiler progress lines and stock flashing instructions are omitted; link/memory/size/image output follows exactly. Full logs remain local at `/tmp/euclidean-release-build.log` and `/tmp/euclidean-debug-build.log`, and contain no warnings/errors.

```text
[121/122] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15008 B        16 KB     91.60%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      224680 B       992 KB     22.12%
   text	   data	    bss	    dec	    hex	filename
 275824	    852	 340576	 617252	  96b24	/workspace/lxr-euclidean-fix/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-euclidean-fix/build/release/LXR_V2_V1.7.img: 276684 bytes, checksum 0xE9
[122/122] Firmware update instructions
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15008 B        16 KB     91.60%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      224680 B       992 KB     22.12%
   text	   data	    bss	    dec	    hex	filename
 275824	    852	 340576	 617252	  96b24	/workspace/lxr-euclidean-fix/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-euclidean-fix/build/release/LXR_V2_V1.7.img: 276684 bytes, checksum 0xE9
[3/3] Firmware update instructions
```

### Debug clean build and final rebuild summaries (both exit0)

```text
[121/122] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15008 B        16 KB     91.60%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      224728 B       992 KB     22.12%
   text	   data	    bss	    dec	    hex	filename
 275872	    852	 340576	 617300	  96b54	/workspace/lxr-euclidean-fix/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-euclidean-fix/build/debug/LXR_V2_V1.7.img: 276732 bytes, checksum 0x2C
[122/122] Firmware update instructions
[2/3] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15008 B        16 KB     91.60%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      224728 B       992 KB     22.12%
   text	   data	    bss	    dec	    hex	filename
 275872	    852	 340576	 617300	  96b54	/workspace/lxr-euclidean-fix/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-euclidean-fix/build/debug/LXR_V2_V1.7.img: 276732 bytes, checksum 0x2C
[3/3] Firmware update instructions
```

## Appendix C. Reproduction and review commands for a future assistant

After obtaining the source branch (do not merge it into any protected branch):

```sh
git rev-parse HEAD
git status --short --branch
python3 tests/euclidean/run.py
# Use the provisioned ARM toolchain and unchanged presets as documented above.
python3 tests/euclidean/check_images.py --objdump /path/to/arm-none-eabi-objdump build/release build/debug
git diff 5829398cea5d337200393757e39a37d4d8148256 HEAD --stat
git -c core.whitespace=cr-at-eol diff 5829398cea5d337200393757e39a37d4d8148256 HEAD --check
```

Do not change the pinned mask digest to make a failure pass without investigating the actual mask difference and preserving approved valid behavior. No invocation of `flash`, `flash_full`, OpenOCD, hardware clock/note scripts or binary upload is part of the software validation commands.
