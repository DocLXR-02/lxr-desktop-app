# LXR Firmware Lab — Bug 001 firmware compatibility audit

Date: 2026-10-26, Europe/Vienna

Repository: https://github.com/DocLXR-02/lxr-desktop-app

Investigated correction: `bug001/filter-drive-fix`, commit `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`

Official source baseline: `erica-synths/lxr-desktop-app`, `main` at `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`

Report-only branch: `codex-reports/2026-10-26-bug001-compatibility-audit`

Report path: `docs/codex-berichte/2026-10-26_bug001-firmware-compatibility-audit.md`

Report branch base: current `lab-setup`, `5829398cea5d337200393757e39a37d4d8148256`

## 1. Outcome and scope

**Binary/source equivalence is not established.** The manufacturer currently lists public LXR-02 v1.71 with a Euclidean-sequencer maintenance fix, while the public source's CMake metadata is 1.7. Public Git history contains no v1.71 tag, release, or identifiable Euclidean fix commit. Version labels alone cannot determine whether a specific fix was included in an imported snapshot.

More importantly, the released source still has a concrete Euclidean length inconsistency: generating a length-16 pattern stores a zero length, but an active playback path uses that stored value as a modulo divisor. This source defect is established by the code. Its relationship to the unspecified official v1.71 fix is a **plausible inference**, not a proven mapping to the official binary. It is sufficient reason to avoid declaring the source or our patched build a fully compatible v1.71 replacement.

The existing Bug 001 correction changes only reserved-slot sweep dispatch and CC/CC2/NRPN bookkeeping, plus its tests. It neither changes nor repairs Euclidean processing. No extra source change is proposed or implemented here. Its direct compatibility footprint is narrow, but installing its image replaces the whole application and therefore inherits all unresolved differences between the source snapshot and the official release.

This audit was read-only for firmware source, tests, configuration, artifacts, and all existing branches. No build, regression suite, hardware validation, flash, PR, merge, source commit, or binary publication was performed. Only a new Markdown report is authorized for publication on a new branch based on `lab-setup`. All hardware procedures below are proposals, not executed instructions or test results.

## 2. Instructions, identities and evidence classification

Read and followed [AGENTS.md at the report base](https://github.com/DocLXR-02/lxr-desktop-app/blob/5829398cea5d337200393757e39a37d4d8148256/AGENTS.md). This includes protecting `main`, keeping the investigation narrow, distinguishing evidence from hypotheses, requiring separate approval/validated backup/recovery preparation for hardware work, and publishing/verifying a unique self-contained Markdown handoff without secrets or personal data.

Starting checkout: `/workspace/lxr-desktop-app`, clean, on `bug001/filter-drive-fix` at `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`. No local branch switch or ref update was needed for this audit. Remote discovery confirmed:

| Ref | SHA |
| --- | --- |
| Manufacturer public source `main` | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Fork `main` | `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` |
| Fork `lab-setup` | `5829398cea5d337200393757e39a37d4d8148256` |
| Fork `bug001/filter-drive-fix` | `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0` |

GitHub repository metadata identifies this repository as a fork of `erica-synths/lxr-desktop-app`. The report base tree is `e206244c314757c48254faff4bb663bd1186027c`; recursive inspection found only the root AGENTS instructions and no existing report directory on that base.

Evidence labels used throughout:

- **Proven/read:** directly observed in source/history, official web material, Git metadata, or bytes actually available in this workspace.
- **Externally supplied:** measurements or history provided by the user; not independently repeated.
- **Inference:** a conclusion consistent with observations but lacking the official binary/source mapping or runtime proof.
- **Unresolved/not run:** a missing comparison, measurement, or proposed hardware test.

## 3. Correct public reference and official release information

The user's previously installed **v1.72 was private test firmware supplied during a support ticket**. That is externally supplied provenance. It is not the public reference and must not be described as a publicly released successor, a download available to all users, or an independently inspected image. No private image was obtained or published in this audit.

The [manufacturer's LXR-02 product page](https://www.ericasynths.lv/drum-synthesizer-lxr-02-1132/) was opened during the audit. Its Update section lists public **1.71**, dated **May 23, 2025**, described as **“Euclidian sequencer fix.”** It also lists 1.7 dated April 23, 2025 and older versions. No public 1.72 entry appeared in the inspected listing. This establishes the public reference as listed at access time, not a claim about unpublished support builds.

The linked official download endpoints observed from that page are:

- v1.71: https://www.ericasynths.lv/service/file/download/product_id/754/file_id/589/
- v1.7: https://www.ericasynths.lv/service/file/download/product_id/754/file_id/580/

The manufacturer's [v1.7 maintenance announcement](https://www.ericasynths.lv/news/lxr17) describes MIDI/Octatrack compatibility, two editable per-step parameters, retaining the Morph target when changing the source kit, a right-channel delay fix, MIDI clock/start improvements, and separate MIDI/analog sync selection. It does not provide a source commit mapping for v1.71 or explain the precise Euclidean defect/fix. These are published change descriptions, not a fresh functional test of the source implementation.

No detailed official v1.71 source diff, reproducible-build manifest, release-source archive or build-toolchain provenance was found in the inspected primary sources. The short Euclidean description is not enough to identify which source lines the official fix changed.

### Official v1.71 binary: supplied data, not inspected bytes

The user supplied:

| Property | Externally supplied value |
| --- | --- |
| Complete image size | 273,544 bytes |
| Magic/header identifier | `LXRV2IMG` |
| Image length/checksum | Reported valid |
| SHA-256 | `188695ea78cd3da0fb6a3e825732bd7fd365d4491a37126003ec513e163e1576` |

The browsing tool attempted the official download link but returned `Unsupported content-type: application/octet-stream`. It did not supply binary bytes to this agent. The v1.7 image and manual link likewise returned unsupported binary content. **The official v1.71 binary was not obtained, hashed, parsed, disassembled, compared byte-for-byte, or executed by this audit.** Its supplied size/header/checksum/hash remain explicitly external evidence.

Source/binary equality cannot be inferred from a common `LXRV2IMG` magic or valid checksum. Those establish an update-container identity/integrity property, not implementation identity, release completeness, preset compatibility, correct boot behavior, or audio equivalence.

## 4. Public source history and version metadata

The manufacturer repository API currently exposes one branch, `main` at `a43cd4f`, no release objects, and no tag refs. Local Git history plus the upstream commit API show exactly five commits:

| Commit | UTC date | Subject / significance |
| --- | --- | --- |
| `e6cfbe904ec88d3f9f7571a3dfeb3d57d81957f7` | 2026-10-20 | Initial open-source import; message describes a latest codebase but gives no v1.71 source mapping |
| `8c8be75fac54615ef527290d8ed3269a1cb59ac9` | 2026-10-23 | README update |
| `3af326204bb62848295fa52d09a04afbdc6f0d72` | 2026-10-23 | License and notice |
| `1d1ac2ba125b6bdcd1568e18880e4ac1b589961c` | 2026-10-23 | CMake project restructuring |
| `a43cd4fcda23ec3d577dc425261c3eb28bd5abff` | 2026-10-23 | Compiler jump-threading workaround for ITCM fit |

Evidence: [manufacturer commit history](https://github.com/erica-synths/lxr-desktop-app/commits/main/), [initial import](https://github.com/erica-synths/lxr-desktop-app/commit/e6cfbe904ec88d3f9f7571a3dfeb3d57d81957f7), [build-workaround commit](https://github.com/erica-synths/lxr-desktop-app/commit/a43cd4fcda23ec3d577dc425261c3eb28bd5abff).

`git diff --exit-code e6cfbe9 a43cd4f -- Core/Src Core/Inc` returned 0 with no differences. Therefore all firmware C sources/headers under those directories are unchanged between the imported snapshot and current manufacturer `main`. Later source-history searches for `EuklidGenerator.c`, `sequencer.c` and `menu.c` identify only the initial import. There is no publicly visible pre-import 1.7→1.71 development history in this repository.

This proves **lack of an identifiable public fix commit**. It does not prove that every fix predating the import is missing: a snapshot could include fixes without their earlier history. Conversely, an import later than the official release and a commit message saying “latest” do not prove the snapshot includes all official fixes.

### Labels are build-selected, not release provenance

| Source | Read evidence |
| --- | --- |
| `CMakeLists.txt:22–25` | `project(... VERSION 1.7 ...)` |
| Initial import `CMakeLists.txt` | Also used `VERSION 1.7` |
| `Core/CMakeLists.txt:32–35` | Sets `FIRMWARE_VERSION` from `${PROJECT_VERSION}` for `main.c` |
| `Core/Src/main.c:56–59` | Non-CMake fallback is `FIRMWARE_VERSION " 2.00"` |
| `Core/Src/main.c:353–358` | Displays the selected string on the boot screen |
| `CMakeLists.txt:88` | Output image name derives from project version |

The ordinary CMake build therefore identifies itself as 1.7; the unrelated fallback 2.00 illustrates why a display string cannot certify the actual release feature set. Neither value is evidence that the official public firmware is 2.00, nor grounds to rename this build 1.71. No metadata was changed.

The public build also includes compiler-specific options such as `-fno-thread-jumps` on `ResonantFilter.c` (`Core/CMakeLists.txt:182–185`) and configurable ITCM optimization. The exact production compiler, libraries, flags, linker behavior and source revision for official v1.71 are unknown. Build-output differences can arise without feature differences, and matching labels would not remove that uncertainty.

### Evidence of some v1.7-era intentions in the source

The released source contains separate clock-mode branches (`Core/Src/Soundengine/MIDI/CortexParser.c:648–663`), two parameter-lock fields per step (`Core/Inc/Preset/SeqStep.h:22–35`), and an ordinary kit-load path that does not reassign the Morph target (`Core/Src/Preset/presetManager.c:834–850`). These are consistent with parts of the official v1.7 announcement. They do not establish the correctness of every announced fix, full functional equivalence to 1.7, or inclusion of the later Euclidean correction.

## 5. Euclidean fix investigation: concrete source evidence

The following code is identical in the initial import, manufacturer `main`, and our correction:

- `Core/Src/Soundengine/Sequencer/EuklidGenerator.c`, blob `a9c2f2a16a491ea73b9529e6dbc2bb439baaf0c6`
- `Core/Src/Soundengine/Sequencer/sequencer.c`, blob `9b51d96e4bdf8f5fd22ae6b8ea6696cc9d7e979b`

Immutable references:

- [Euclidean generator](https://github.com/erica-synths/lxr-desktop-app/blob/a43cd4fcda23ec3d577dc425261c3eb28bd5abff/Core/Src/Soundengine/Sequencer/EuklidGenerator.c#L221)
- [Sequencer length handling and playback](https://github.com/erica-synths/lxr-desktop-app/blob/a43cd4fcda23ec3d577dc425261c3eb28bd5abff/Core/Src/Soundengine/Sequencer/sequencer.c#L1091)

### Reachable length-16 path

1. `Core/Src/menu/menu.c:518–520` defines Euclidean length/steps as `DTYPE_1B16` and rotation as `DTYPE_0B15`. Length 16 is a normal UI setting, not a malformed MIDI-only input. Clamping appears at `menu.c:3780–3789`.
2. `menu.c:3577–3596` sends the chosen length to `euklid_setLength()` and calls `euklid_setSteps()`. Step/rotation edits also regenerate the pattern (`3601–3628`).
3. `EuklidGenerator.c:180–194` stores the length, clamps step count and calls generation.
4. `euklid_generate()` calls `euklid_transferPattern()` (`142–166`).
5. The transfer function converts length 16 to zero and stores it in the pattern length array:

```c
// Core/Src/Soundengine/Sequencer/EuklidGenerator.c:221–240
uint8_t len = euklid_length[trackNr];
// ... copy generated steps ...
if(len == 16)
    len = 0;
#if USE_PATTERN_EDIT_BUFFER
patternBackupBuffer.length[trackNr] = len;
#else
seq_patternSet.length[patternNr][trackNr] = len;
#endif
```

`Core/Inc/Soundengine/Sequencer/sequencer.h:137` defines `USE_PATTERN_EDIT_BUFFER 0`, so the direct pattern-array assignment is the enabled path. When the edited pattern is the active pattern, normal subsequent playback can encounter that zero.

### Inconsistent consumers

Some code understands the old zero-as-16 sentinel:

```c
// sequencer.c:1357–1359; similar handling exists at 1271–1273
seqlen = seq_patternSet.length[seq_activePattern][i];
if(!seqlen)
    seqlen = 16;
```

But the enabled `seq_processFlam()` path has no such normalization:

```c
// sequencer.c:1091–1112, inside the active #if 1 branch
if(!seq_running) return;
// for each unmuted track:
const uint8_t length = seq_patternSet.length[seq_activePattern][track];
const uint8_t currentStep = seq_stepIndex[track];
const uint8_t nextStep = (seq_stepIndex[track] + 1) % length;
const uint8_t prevStep = seq_stepIndex[track] != 0
    ? seq_stepIndex[track] - 1 : length - 1;
```

For stored length zero, the modulo expression has undefined behavior in C. At current step zero, `prevStep` also becomes 255 after narrowing. Subsequent step access is another concern. The sequencer tick calls this routine (`sequencer.c:1730`, `1762`, `1831`); it is not merely unused legacy code or the disabled lookahead at `945–947`. The exact MCU/compiler consequence is not measured here: do not claim a guaranteed hard fault, a specific audible effect or an inevitable crash from source alone.

Ordinary track-length setters/getters no longer perform the old conversion (`sequencer.c:274–305`, conversion code commented out). MIDI recording also has a direct modulo by stored length (`sequencer.c:2402`). The active pattern loader reads length bytes into the same array without normalizing that sentinel (`presetManager.c:1962–1970`). These observations reinforce that the convention is inconsistent; they do not prove a different preset format in the official image.

### What can and cannot be concluded

**Proven:** the public source has this Euclidean zero-length inconsistency, it is reachable through a normal length-16 generation operation, and Bug 001 leaves it unchanged. The exact official v1.71 fix is not identifiable from public history or a detailed official patch description.

**Inference:** this looks like a plausible candidate for the official Euclidean maintenance fix, and an unresolved source-baseline compatibility risk. The source snapshot may omit or incompletely reflect the official correction. This is stronger evidence than the mere 1.7 label but still not a binary comparison.

**Not proven:** that official v1.71 fixes precisely these lines; that every part of its fix is absent; that the source is exactly official v1.70; that v1.71 has no other changes; or that identical input produces a particular hardware failure in these builds.

An independent community review in [upstream issue 3](https://github.com/erica-synths/lxr-desktop-app/issues/3) also reports length 16 being stored as zero. That is corroborating third-party source analysis, not an official changelog or manufacturer confirmation of the v1.71 implementation. The inspected collaborator reply discusses expectations for incremental, documented, real-device-tested contributions; it does not map v1.71 to source. Other issue allegations were not adopted as confirmed findings in this audit.

No fix to this problem was implemented. Do not silently fold a Euclidean change into the already published Bug 001 patch.

## 6. Established versus possible compatibility differences

| Topic | Established evidence | Unresolved / inference |
| --- | --- | --- |
| Release identity | Official product listing says 1.71; CMake source says 1.7; private 1.72 is not public reference | Labels do not establish source ancestry or full feature differences |
| Source history | Five public commits, initial snapshot plus docs/build work; no tags/releases or explicit Euclidean fix commit found | Fixes might have been imported without history; no reproducible source-to-v1.71 mapping |
| Euclidean behavior | Source stores 16 as 0 and has unguarded consumers | Official implementation and exact fix unknown; potential inherited regression relative to v1.71 |
| Image container | Source generator uses a 16-byte desktop `LXRV2IMG` header; official magic/validity supplied externally | Common container does not prove application compatibility or on-device boot |
| Binary identity | Prior patched Release image read here has a different size/hash from externally supplied official measurements | Does not isolate patch size from baseline/compiler differences; no unpatched-source binary comparison performed |
| Preset/project data | Bug 001 changes no format tags, serializers, enum layout or step structures; source writes `PatV03` (`presetManager.c:1695–1696`) | Official v1.71 reader/writer semantics and private 1.72 data behavior unverified |
| Audio/DSP | Bug 001 changes no filter/DSP/Drive curve/smoothing | Full official-versus-source audio equivalence is not established |
| MIDI | Supported source engine mute commands 200–206 remain explicit exceptions; invalid aliases rejected | Official undocumented mappings and UI/engine mute coherence not established |
| Toolchain/build | Public build has selected optimization/ITCM workarounds; previous local build used ARM GCC 14.2.1 | Official production toolchain/options unknown; generated code/timing/layout can differ |
| Hardware evidence | Earlier click observations came from a private test build | Public v1.71 may reproduce differently; no public-reference hardware A/B performed |

### Existing patched artifact read in this audit

Only the already generated patched Release image from the previous task was read; it was not rebuilt:

```text
Path: build/release/LXR_V2_V1.7.img
Complete file bytes: 275148
Magic: LXRV2IMG
Declared payload bytes: 275132
Actual payload bytes: 275132
Stored checksum: 0x1B
Checksum recomputation: valid
SHA-256: 65441a47a0352d2ba69d814b044c0d014cd76fe9cba130552223489c0e028d7d
```

Relative to the supplied official total size of 273,544, this existing patched image is 1,604 bytes larger and has a different supplied-versus-observed hash. That is **not** a 1,604-byte estimate of Bug 001's contribution: no matched-toolchain unpatched source image was built here, and the official binary was not inspected. It does not by itself prove a feature mismatch in the unpatched source.

The earlier validated-publication report has the historical build/regression/image results and corrected complete-image sizes:

https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-bug001-validated-publication/docs/codex-berichte/2026-10-26_bug001-validated-correction-publication.md

Those results are historical evidence, not fresh test runs in this audit.

## 7. Compatibility review of the existing minimal patch

Reviewed `git show 7d9fe99` and the complete production diff against `lab-setup`. Exactly seven paths differ: two production files plus the five documented regression files. Production changes total 35 insertions and 12 deletions; no new source changes were made here.

| Production path / corrected lines | Intended behavior | Compatibility implication |
| --- | --- | --- |
| `Core/Src/Preset/presetManager.c:1264–1269` | Recognizes four reserved transport slots | Does not renumber or change stored bytes |
| Same file: 1316–1320 | Immediate load skips executing them, retaining audio-service call | Removes accidental commands but preserves sweep service cadence |
| Same file: 2148–2154 | Morph consumes those slots' existing ticks without MIDI dispatch | Restart/index cadence unchanged |
| `Core/Src/Soundengine/MIDI/MidiParser.c:134–143` | Rejects NRPN >255 before narrowing | Stops invalid aliases; high-bit malformed/unsupported numbers no longer affect an unrelated low parameter |
| Same file: 170–176 | CC0 returns before parameter subtraction/cache/notification | Protects bookkeeping; outer external bank forwarding remains |
| Same file: 905–913, 1438–1440 | Bounds-checks sound CC2 indices, exempts mutes 200–206 from cache/notification | Preserves source-supported mute engine commands while blocking invalid writes |

Read-only diff checks confirm no changes to `Core/Src/Soundengine/Sequencer`, its headers, `Core/Inc/Parameters.h`, `Core/Src/main.c`, `Core/Inc/Soundengine/config.h`, or `CMakeLists.txt` relative to manufacturer source. The Euclidean files have the exact same blobs before and after Bug 001.

### Intentional behavior differences that merit future integration checks

1. Morph/kit loading no longer overwrites NRPN selector state from reserved kit slots or executes Data Entry. External controllers should explicitly select the NRPN they intend to write, rather than rely on an accidental reset after loading a kit. This is intentional removal of erroneous behavior, not a new parameter mapping.
2. Unsupported high selectors or malformed CC numbers that previously aliased parameters or corrupted memory are rejected. A setup relying on those effects changes behavior by design; that is not evidence of supported official compatibility.
3. The supported engine mute command range 200–206 is preserved. Source also has a separate UI NRPN range 1000–1006 in `Core/Src/frontPanelParser.c:44–45,94–98`. The outer MIDI handler still forwards bytes to that UI parser after the engine rejects an oversized selector (`MidiParser.c:1879–1912`; `Uart.c:137–141`). `buttonHandler_muteVoice()` updates UI mute state/LEDs (`buttonHandler.c:502–517`), while the 200–206 handler calls `seq_setMute()` in the engine (`MidiParser.c:1336–1355`). Do not claim the patch creates unified UI/audio muting for both ranges. That pre-existing split is outside Bug 001, and the earlier host fixture stubs UI forwarding rather than proving full front-panel integration. If a controller uses either range, future approved testing should observe both audio and LEDs, not just bytes.
4. External CC0 forwarding is preserved, but the patch does not broadly validate all downstream bank indices or fix every pre-existing bank-change/UI issue. Do not interpret the cache fix as a comprehensive external-MIDI audit.
5. Whole-application replacement can inherit unrelated missing official fixes, including the demonstrated Euclidean concern, even though the patch itself does not touch those areas. A small diff is not proof that the resulting firmware is a drop-in v1.71 replacement.

Source SHA-256 checks in this audit matched the documented correction:

```text
ac0901a2a422473de1e175807d1f808ad1878f99ede345e1c40926386e89419c  Core/Src/Preset/presetManager.c
076c8129e7d159f5e80f70584e78c2c745e15c457907539df691fb0d5ed04fab  Core/Src/Soundengine/MIDI/MidiParser.c
```

No additional compatibility defect caused by the patch was established. The intentional command-state changes and the unproven official baseline remain the integration risks. The patch's regression tests establish its targeted control-path behavior under their fixtures, not release-wide compatibility.

## 8. Proposed minimal A/B validation — not executed

### Preconditions and release gate

This is a proposal only. AGENTS requires separate explicit hardware authorization, a validated backup and a recovery plan. This audit authorizes none of those actions. Given the source Euclidean inconsistency, **do not present the patched image as an unqualified public v1.71 replacement or proceed automatically to a broad hardware trial**. First seek an exact manufacturer fix/source mapping or arrange a separately approved controlled experiment that explicitly accepts the known source risk.

Before any approved trial:

- Retain the official recovery image and verify its manufacturer provenance, full hash, header/length/checksum locally. The supplied hash is a reference to verify, not a substitute for obtaining the image.
- Back up the entire SD contents, including projects, global settings, indexes and `/tmp`; validate the backup by listing/hashing files and a non-destructive restore to a disposable copy. Preserve any private v1.72 image/state locally; do not publish it.
- Use existing removable media or restore the same validated disposable card between runs; no purchase is necessary. Keep an untouched master backup. Each firmware starts from a separate identical clone, not from the state last written by another firmware.
- Account for autosave and boot-time `/tmp` restoration: `main.c:896–904` loads temporary kit/pattern state over project data. Even without a manual save, files/state can differ between trials. Prepare controlled disposable projects rather than resaving valuable originals under experimental firmware.
- Confirm the specific application-only artifact, exact commit/hash and previously validated toolchain. Recheck `UPDATE_BOOTLOADER=0` (`main.c:17`); exclude `flash_full`, bootloader images and debug-probe bootloader writes. Do not rely on the common displayed 1.7 string to distinguish source builds.
- Use the approved manufacturer application-update procedure and stable power; one intended update image per test card root avoids selecting another image. Do not interrupt installation. Have the validated official rollback path ready. Approval to test is not approval to overwrite the bootloader.

### Three identities, two controlled comparisons

A direct official-versus-patched A/B is useful for user-visible compatibility but cannot isolate the patch from the source baseline. The minimal causal design therefore uses three identities as two A/B comparisons:

| Identity | Role |
| --- | --- |
| A | Verified public official v1.71, the public behavior reference |
| B | Unmodified public source at `a43cd4f`, built with the same toolchain/options as C |
| C | Existing Bug 001 correction at `7d9fe99`, equivalent toolchain/options |

Compare **A versus B** for source-baseline compatibility, then **B versus C** for the isolated correction. B has not been built or validated in this audit; preparing it requires a separately authorized task without altering the existing correction branch. The private v1.72 is not A.

If only two images are authorized, A versus C can establish a practical difference but cannot attribute it solely to Bug 001. Record that limitation explicitly. Plan the fewest approved transitions and restore A at the end if that is the agreed rollback; no transition schedule was executed here.

### Core Morph fixture

Use a clean disposable project with manually established nonzero track lengths, **no Euclidean generation**, a single repeating Voice 1 BassKick, fixed velocity/tempo, identical source/target kits, no LFOs or automation, fixed faders/routing and unchanged output/recording gain. Keep monitoring quiet with sufficient recording headroom; abrupt transients are expected in the reproduction.

For each identity under the same fixture:

1. Record the firmware artifact hash, boot result, project/card snapshot identity and all relevant settings. Check normal manual-pattern playback first.
2. Capture a stationary-knob reference at Drive=0 and Drive=127.
3. With a representative enabled filter, capture slow 0→255→0 Morph movement and a short, repeatable set of physical movements at each clamped endpoint. Repeat at Drive=0/127 with the same gesture rate.
4. Repeat the high-Drive movement with Filter OFF as the diagnostic control. A second enabled filter/other voices can be added only if the minimal case is stable and the approved scope permits it.
5. Use the same manually sequenced kicks, monitor level, recording chain and timing notes throughout. Compare click events aligned with knob movement, stationary references, obvious Drive loss and peak/transient behavior; do not demand bit-identical audio from independent oscillator/RNG/timing states.

Interpretation: a difference between A and B indicates a source-baseline difference; improvement between B and C isolates the patch within that source baseline. An A-versus-C difference alone cannot distinguish these causes. Do not transfer conclusions from private v1.72 to public A without observing A.

### Small supported-MIDI/persistence check, if separately approved

After the core case is stable, select valid NRPN Drive explicitly, exercise engine mutes 200–206, and observe audio/UI before and after Morph and ordinary kit loading. If the controller actually uses 1000–1006, inspect both UI and audio rather than assuming equivalent semantics. Check a valid bank selection and disposable kit/pattern save/reload. Keep all NRPN/kit/track inputs in documented ranges; malformed-data hardware testing is not needed for the minimal A/B.

### Euclidean compatibility is a separate gated discriminator

The normal Morph fixture deliberately avoids the known zero-length producer. That does not waive Euclidean compatibility. First compare stopped-transport generation and resulting disposable saved-pattern data for lengths 15 and 16, with a modest pulse count and rotation 0/1. Inspect stored lengths and patterns without relying only on display labels. The official release may internally support a zero sentinel; raw data alone is not proof of a defect in A.

Do not automatically start B/C playback of a generated length-16 pattern merely to force the identified undefined operation. Resolve the source inconsistency or obtain explicit approval for that specific controlled-risk test first. If playback is authorized, test on disposable state with recording/monitor safeguards, stop at an unexpected response after installation is complete, and follow the agreed recovery plan; never interrupt a firmware installation as a test response. Compare wrap length, pulse/rotation results, save/reload and any freeze or timing change. A source-baseline failure with healthy A remains an inherited compatibility blocker, not evidence that Bug 001 introduced it.

### Acceptance and stop criteria

- Require normal boot, stable manual playback and intact disposable project round trips before broader tests.
- For Bug 001, seek reduced movement-correlated disturbances in C relative to B while stationary controls and intended Drive behavior remain intact; no exact numerical threshold was established in this audit.
- For broad v1.71 compatibility, require resolving or explicitly characterizing Euclidean differences and other observed A/B differences. Passing the Morph case alone is insufficient.
- Stop and record unexpected output/state, crashes or corruption; do not save experimental state over a master backup or silently broaden the patch. Escalate each independent issue as a separately scoped investigation.

## 9. Read-only actions, exact results, errors and unrun work

Actual actions:

1. Inspected clean local Git status, refs and commit history; read current AGENTS from remote `lab-setup`.
2. Read manufacturer product/update listing and v1.7 announcement through the web tool; attempted its linked downloads.
3. Queried upstream branch/commit/release/tag-ref and Euclidean-file history APIs; inspected relevant upstream issue discussion as corroboration only.
4. Read source paths/lines above, active preprocessor branches and all production hunks in the existing correction.
5. Ran history/blob equality and existing-file SHA-256 checks; read existing patched image metadata/hash without rebuilding it.
6. Prepared this Markdown report outside the working checkout; documentation-only publication uses a new report branch, leaving existing refs untouched.

Representative commands actually run, all read-only with respect to repository content:

```sh
git status --short --branch
git rev-parse HEAD
git log --all --oneline -12
git log --all --format='%H %aI %s'
git rev-list --count a43cd4f
git log --follow -- Core/Src/Soundengine/Sequencer/EuklidGenerator.c
git diff --name-status e6cfbe9 a43cd4f
git diff --exit-code e6cfbe9 a43cd4f -- Core/Src Core/Inc
git diff --stat 5829398 7d9fe99
git diff --exit-code a43cd4f 7d9fe99 -- Core/Src/Soundengine/Sequencer Core/Inc/Soundengine/Sequencer Core/Inc/Parameters.h Core/Src/main.c CMakeLists.txt Core/Inc/Soundengine/config.h
git show --format= 7d9fe99 -- Core/Src/Preset/presetManager.c Core/Src/Soundengine/MIDI/MidiParser.c
git rev-parse e6cfbe9:Core/Src/Soundengine/Sequencer/EuklidGenerator.c
git rev-parse HEAD:Core/Src/Soundengine/Sequencer/EuklidGenerator.c
git for-each-ref --format='%(refname) %(objectname)' refs/heads refs/remotes
```

Results: public source history count 5; equality checks returned 0; Euclidean generator and sequencer blobs are unchanged since the initial import; correction diff is exactly the existing two source/five test files; source hashes match the previously documented patch; local status remained clean. Existing patched Release bytes independently yielded the metadata in section 6. No claim is made that these static checks are firmware regression execution.

Errors/limitations:

- Official image/manual download attempts through the web reader returned unsupported `application/octet-stream`; no official binary/manual content was inspected. The official page and 1.7 announcement were readable.
- A generic upstream `/tags?per_page=100` endpoint request was rejected by the connector's URL allowlist. The supported `/git/matching-refs/tags` request returned an empty array, so tag absence is based on that successful read, not the error.
- Some broad discovery outputs were truncated; decisive Euclidean/source sections were re-read in narrower ranges and checked against active branches. No finding relies on a truncated compiler/test result.
- No new build, executable regression suite, sanitizer run, hardware boot, audio test, flash, downgrade, restore trial, official binary disassembly or unpatched-source binary generation was performed in this audit.
- Prior tasks' successful regression/build results remain historical and are not proof of public v1.71 compatibility.

No tests, build files or firmware files were edited. No existing branch was updated. No paid service or purchase was used. The only new remote content intended by this task is the report itself.

## 10. Open questions and next actions

1. Obtain the exact public v1.71 image through an approved binary-capable route and independently verify the supplied measurements. Do not replace external provenance with an assumption.
2. Ask for the exact source change/revision behind the official Euclidean fix, preferably its source diff and build provenance. No message to the manufacturer was sent in this task; contacting others would require authorization.
3. Determine whether the source length-16 inconsistency is the official fixed defect or an independent remaining problem. A focused host test/source investigation could establish runtime consequences without a hardware flash, but adding tests or fixing it is a separate authorized task.
4. Establish official preset/NRPN/UI semantics where release-to-source differences matter. Common names and container magic are insufficient.
5. Prepare matched B/C builds and a separately approved backed-up A/B plan only after the source-baseline gate is addressed or explicitly accepted. Do not create a source change just to relabel the output 1.71.
6. Keep Bug 001 narrow: no Euclidean repair, source synchronization, smoothing or protocol redesign should be silently added to its published commit.

Decision at handoff: **retain the existing correction as a validated source-level Bug 001 change, but withhold claims of full official-v1.71 equivalence/compatibility.** The public-reference version is corrected to v1.71, and the private v1.72 observation remains a separate historical reproduction context.

## 11. Publication and continuation record

This report is a documentation-only addition under `docs/codex-berichte/` on a new branch based on exact `lab-setup` commit `5829398cea5d337200393757e39a37d4d8148256`. It does not change the existing correction, its tests, configuration, any old report, or existing branch heads. The report path and branch named at the beginning are unique; creation must not overwrite any existing report.

After committing/publishing the Markdown file, verify its complete read-back content, its exact GitHub URL, the report commit's sole parent and single-file change list, and unchanged `main`, `lab-setup`, and `bug001/filter-drive-fix` refs. Final response must report actual verification, not merely an attempted publication. No firmware binary or generated artifact belongs in that report commit.

Primary references for continuation:

- [Manufacturer product/releases](https://www.ericasynths.lv/drum-synthesizer-lxr-02-1132/)
- [Manufacturer v1.7 announcement](https://www.ericasynths.lv/news/lxr17)
- [Official public source](https://github.com/erica-synths/lxr-desktop-app/tree/a43cd4fcda23ec3d577dc425261c3eb28bd5abff)
- [Published minimal correction](https://github.com/DocLXR-02/lxr-desktop-app/commit/7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0)
- [Previous investigation/implementation handoff](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-bug001-handoff/docs/codex-berichte/2026-10-26_bug001-filter-drive-handoff.md)
- [Previous validated-publication handoff](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-bug001-validated-publication/docs/codex-berichte/2026-10-26_bug001-validated-correction-publication.md)
- [Applicable AGENTS instructions](https://github.com/DocLXR-02/lxr-desktop-app/blob/5829398cea5d337200393757e39a37d4d8148256/AGENTS.md)

End of self-contained compatibility audit. Official binary measurements are externally supplied; all proposed hardware validation remains unexecuted.
