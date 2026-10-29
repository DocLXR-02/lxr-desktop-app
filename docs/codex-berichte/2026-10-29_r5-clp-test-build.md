# R5 CLP input-loss control: implementation, validation and test-build handoff

Date: 2026-10-29 (Europe/Vienna).
Task: implement the approved offline candidate on a separate R4-based branch and produce a verified, clearly identified test build.
Repository: https://github.com/DocLXR-02/lxr-desktop-app
Source branch: fx/clp-input-silence
Exact R4 base: c870f7249d7ee0d65423bc8b7bfdb422dcc74482
Diode-only commit: 792087d6cba31ae20d64fb63c1e165f8cb5ae6f4
Feature/build commit: af3562fece090288a83727524c1035b79f4cf20b
Feature tree: 316127985571e2545a4cafb0954f9c7e9c63b53d
Report branch: codex-reports/2026-10-29-r5-clp-test-build
Report branch base: lab-setup 5829398cea5d337200393757e39a37d4d8148256.
Report head: the commit containing this report.

## Authorization and current status

The owner explicitly approved the isolated R4-based implementation, build and checks, after reviewing the offline candidate. Hardware installation was a separate later decision. No firmware has been flashed and no new device-test instructions have been issued in this task.

DEVICE: the owner additionally confirmed repeatedly seeing Kit 4's differing start/stop boundaries. Its previous apparent inconsistency was understood as history dependence. This supports the existing device record; it is not a device validation of R5.

CODE / HOST / BUILD: implementation, host checks, GitHub CI, ARM build and independent image checks completed successfully. HARDWARE: pending.

## 1. Implemented behavior

CLP inspects input before distortion. Values of magnitude <=1 int16 LSB count as inactive. After 441 inactive frames (approximately 10 ms at nominal REAL_FS=44140), the returned feedback gain decreases linearly over 221 frames (approximately 5 ms). The first usable input sample restores full feedback without an attack fade.

Stereo detection is linked: activity in either channel keeps both feedback paths active. Mono detection uses only the channel actually consumed by the existing mono algorithm. This intentionally does not fix the separate original mono-routing/right-channel issue.

The mechanism controls loop return, not wet-output volume. drv/col/ton/vol/d/w controls, filter coefficients, voice synthesis and serialized kit format remain unchanged.

The detector has instance-owned runtime fields (8 bytes added to MasterDistortion). It begins closed on CLP entry. Existing CLP filter/feedback history is cleared at the audio-processing entry, preventing an old oscillator state from being revived at entry. A reset request only clears the volatile active flag; DSP history itself is cleared by the processing entry.

Explicit full kit resends, including immediate resend, request an FX-history reset. Ordinary knob changes and morph steps do not request that reset. Off/non-distortion processing invalidates CLP entry state. Tube/Fold processing invalidates the detector state without clearing their active DSP states; re-entering CLP starts with cleared CLP history.

The first splash line is changed to "LXR LAB R5-CLP", making the firmware visibly modified. The normal image magic and application offset are retained.

The positive diode-table endpoint is independently bounded in its own commit:
```c
if(index >= 256.f) return table_1n914[256];
```
This removes the confirmed table_1n914[257] access. It is not represented as the cause of intentional self-oscillation.

Feedback integration:
```c
const float feedback = Distortion_clipFeedback(dist, inputL, inputR);
signal1 += dist->lastOut[0] * dist->p1 * 0.125 * feedback;
signal2 += dist->lastOut[1] * dist->p1 * 0.125 * feedback;
```

## 2. Changed files and scope

Production changes:
- Core/Inc/Soundengine/DSPAudio/FX/Distortion.h: typed init prototype, reset API, runtime-only detector fields.
- Core/Src/Soundengine/DSPAudio/FX/Distortion.c: bounded diode endpoint, detector, quiet entry, feedback multiplier.
- Core/Src/Soundengine/FxBus.c: reset at effect changes/inactive dispatch.
- Core/Src/Preset/presetManager.c: reset request at the two full-kit resend entries; no storage-format or storage-I/O algorithm changes.
- Core/Src/main.c: modified splash identity only.

Tests/build:
- tests/fx_clip/: actual-DSP tests, actual FX-bus dispatch/reset fixture, host intrinsics, bounded R4 oracle, image validator.
- tests/bug001/test_sweeps.c and tests/project_storage/host.c: no-op FX reset callbacks for their existing MIDI/storage mocks.
- .github/workflows/clp-test-build.yml: scoped branch workflow, host regressions, same-toolchain R4 comparison build, R5 build, image validation and source-inclusive artifact.

No bootloader change, new UI page, kit parameter, logging loop, gate control parameter, compressor, limiter or filter redesign is included.

## 3. Materialization and source provenance

The available local checkout was the original a43cd4f baseline. R4 was materialized from its exact tree using the GitHub connector. All 371 blob hashes and the complete tree hash b89db2323a1e399e583ccb724a38adf29ce75e18 were verified before editing.

An initial large transfer exceeded the shell argument limit; three chunked files were initially incomplete. Hash verification caught every mismatch, and they were fully retransferred and verified before any source edit. The exact R4 commit object was reconstructed from GitHub metadata, including original timestamp timezone, and used as the local shallow boundary. No guessed or partial source served as the implementation base.

Local and GitHub commit hashes differ because their commit metadata differ; the final local tree and remote feature tree both equal 316127985571e2545a4cafb0954f9c7e9c63b53d. The downloaded CI source archive was independently compared byte-for-byte against all five local production files.

The source and report were published through GitHub API tools. No force push or merge was used. main was independently verified at a43cd4fcda23ec3d577dc425261c3eb28bd5abff after publication. R4's source and image were not modified.

## 4. Tests actually executed

Local:
- python3 tests/fx_clip/run.py --output build/fx_clip_results.json
- python3 tests/bug001/run.py
- python3 tests/project_storage/run.py
- CRLF-aware git diff --check
All final commands passed.

New tests compile the actual production Distortion source, not a reimplemented detector model. Native compiler: Ubuntu GCC 13.3.0, -O2, -fno-strict-aliasing, UBSan/bounds, nonrecovering diagnostics.

Coverage:
- Diode positive endpoint, clamped high input and 10000 finite lookup inputs.
- Cold silence at high gain/feedback; independent instances.
- 441-frame hold, fade, +/-1-LSB tolerance, first usable sample reopening.
- Queued reset, Tube/Fold/CLP re-entry, correct mono input consumption.
- Actual FX-bus dispatch and reset for all six output routes.
- 72 synthetic mono/stereo renders across controls, threshold cases and maximum-drive/feedback stress rows.
- Sample-at-a-time versus 64-frame processing: identical output.
- Continuous test input: sample-identical to bounded R4.
- After input loss: quantized silence after settling; maximum time below the 32-LSB peak criterion 24.830086 ms.
- The reference is exact pinned R4 with only the diode endpoint defined; it is not the original undefined-read binary.

CI additionally passed:
- Existing CC0, NRPN/Drive/mute and morph sweep regressions.
- Kit-load feedback and 512 actual RAM load/menu cases.
- Project/storage/failure and complete FatFs regression groups.
- Six Euclidean groups, including 2176 masks and sequencer/persistence/protocol boundary coverage.

Harness development caught two issues, corrected before the final passes: a unit assertion failed to allow the pre-existing filter tail after feedback reached zero, and a render writer compared its write count with 2 instead of 2*size. Legacy mocks initially lacked the newly referenced reset callback; their fixtures were updated. These were test/harness integration failures, not unexplained final test failures.

## 5. GitHub build and resource comparison

Successful run:
https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/40910378774
Job: 123576420322.
Artifact: 12912683549, lxr-lab-r5-clp-test, 2127249 bytes.
Artifact SHA-256:
10123f1193d7729f7091fb449a8d4de45ceebdd758e3313380be33faf8c2160c

ARM toolchain:
arm-none-eabi-gcc (Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174)) 14.3.1 20250623.
Both R4 and candidate were built in the same job with the release configuration.

| Linker region | R4 rebuild | R5 test | Delta | R5 free |
| --- | ---: | ---: | ---: | ---: |
| ITCM, 16384 B | 15016 | 15104 | +88 | 1280 |
| DTCM, 131072 B | 107392 | 107400 | +8 | 23672 |
| RAM, 393216 B | 274648 | 274648 | 0 | 118568 |
| Reported FLASH, 1015808 B | 234100 | 234764 | +664 | 781044 |

The linker FLASH number omits separately loaded ITCM/DTCM data. Independently inspected ELF load segments all fit application flash:
0x08008000..0x080411b8, 0x080411b8..0x0804150c,
0x0804150c..0x0804a590, 0x0804a590..0x0804e090.
No nonempty load segment touches bootloader flash.

This is a same-toolchain source comparison, not proof of byte identity with the previously flashed R4 image. The established R4 image remains 288676 bytes with SHA-256 1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877. Different rebuild identities must not be conflated.

No device CPU, stack high-water, realtime latency or listening measurement was performed. Memory fit and host timing semantics are not a hardware performance guarantee.

## 6. Test artifact verification

Delivered image: LXR_LAB_R5_CLP_TEST.img
Bytes: 286880
SHA-256:
90caf0cc55203a828f02a108e0bd62f4ac8396c94e823e9680ee433991284a3f

Full source archive SOURCE.zip SHA-256:
12b6262082f29cfd39d727a11cbf25cc13513b5a2fb03bd8e1d020bf2f22234c

CI and independent local validation passed:
- LXRV2IMG magic.
- Payload length equals file size minus 16.
- Checksum equals 255 - (payload byte sum & 255).
- Reserved bytes zero.
- Modified R5-CLP splash string present.
- Application reset vector at or above 0x08008000, Thumb bit set.
- Initial stack pointer within the configured SRAM range.
- All nonempty ELF load segments inside application flash.
- Downloaded CI ZIP digest and ZIP integrity.
- Full source ZIP integrity, source commit and changed production bytes.

The artifact includes the complete source, compiler metadata, ELF/map, checksums and resource reports. The user-facing package adds this report and an English readme.

## 7. What remains open

HARDWARE: actual CLP-1 kick/tails, fader ramps, audible reopening transients, stereo musical behavior, full kit recalls and runtime CPU peaks remain untested on the device. The current host renders use synthetic input, not the complete actual Voice 1/sequence from CLP-1.

History is intentionally removed at full kit recall and CLP re-entry; this may alter a live transition. After silence, a subsequent kick does not inherit the old feedback oscillator. Offline comparisons showed a small attack-level difference on returning synthetic kicks, but no claim of unchanged timbre or click-free behavior follows.

The 1-LSB detector tolerance and linked stereo policy have the documented quiet/sparse-input limits. Residual input above the threshold can keep feedback open. These are focused test-build tradeoffs, not final universal design claims.

No change-list entry is marked hardware-fixed. The build is an experimental R5 candidate, not a release.

## 8. Backup, recovery and next action

The supplied CLP-1 archive was verified in the prior audit and contains the confirmed R4 image. It remains unchanged. That validates the supplied bytes, not the availability of an external physical backup or a guaranteed device rollback.

Proposed recovery remains the ordinary SD application-update path with that known-good R4 image, preserving the bootloader. A rollback from this R5 build has not been hardware-tested.

The next phase is a separately authorized low-monitoring hardware comparison using CLP-1, including ordinary threshold cases, Kit 4's existing oscillation, natural pauses, fader zero, restart, and kit/FX transitions. This report is a concrete handoff for that decision. No additional code/build permission is needed for routine fixes within the already approved implementation scope.
