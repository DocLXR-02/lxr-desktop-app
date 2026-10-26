# LXR Firmware Lab — Bug 001: validated correction branch publication

Date: 2026-10-26, Europe/Vienna

Repository: https://github.com/DocLXR-02/lxr-desktop-app

Firmware branch: https://github.com/DocLXR-02/lxr-desktop-app/tree/bug001/filter-drive-fix

Firmware commit: `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`

Firmware/report base: current `lab-setup`, `5829398cea5d337200393757e39a37d4d8148256`

Report-only branch: `codex-reports/2026-10-26-bug001-validated-publication`

Report path: `docs/codex-berichte/2026-10-26_bug001-validated-correction-publication.md`

## 1. Outcome, scope and authorization

The already implemented minimal correction and its five-file regression suite were recovered intact from the existing checkout, verified against the complete previous handoff, retested, rebuilt in both configurations, and published on the specifically authorized development branch. All seven remote files were read back and matched the validated local contents and Git blob hashes exactly. The firmware commit's sole parent is the exact current `lab-setup` base above. The local checkout was synchronized to that same published commit and is clean.

The user explicitly authorized committing and pushing these firmware changes and tests in this task. They required a branch based on current `lab-setup`, no changes to `main`, no PR or merge, no flashing, no published firmware binaries/build artifacts, no unrelated modifications, and no purchases or paid services. All these boundaries were observed. Publication used the connected GitHub API because native Git could not reach its proxy; no native `git push` command was executed. The resulting branch and files are actually available on GitHub, not merely prepared locally.

The source commit changes exactly two production files and five Bug 001 test files. It contains no report, generated artifact, workflow change or binary. This separate report is Markdown-only and is published on a report-only branch from the same `lab-setup` base. It does not overwrite the previous report or modify `lab-setup` itself.

Successful source/control-path tests, image-format checks and firmware builds do **not** establish that hardware audio is corrected. Hardware validation remains unperformed and separately subject to approval.

## 2. Instructions and starting state

Read and followed the current [AGENTS.md on lab-setup](https://github.com/DocLXR-02/lxr-desktop-app/blob/5829398cea5d337200393757e39a37d4d8148256/AGENTS.md). Its requirements include protecting main, keeping the bug fix narrow, distinguishing observations/evidence/hypotheses/tests, avoiding unapproved hardware work, producing a self-contained Markdown report, using a unique report path on an allowed branch, excluding secrets/personal data, and verifying publication on GitHub.

Read the complete previous report, including Appendix A's production diff, Appendix B's five exact test files, and Appendix C's complete build logs:

https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-26-bug001-handoff/docs/codex-berichte/2026-10-26_bug001-filter-drive-handoff.md

That report's commit is `9028075917b2bc1a8df43dfb2c6e0d2dab66595d`. Its source investigation used `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`. Its own report branch was based on the older `lab-setup` commit `56593cacc789e4367f7bb7a63ac2511bfdcd1195`. Do not confuse that older report base with this task's required current base.

First operation in this task inspected the existing checkout's status before any branch or file changes:

```text
Repository: /workspace/lxr-desktop-app
Branch: bug001/minimal-correction
HEAD: a43cd4fcda23ec3d577dc425261c3eb28bd5abff
 M Core/Src/Preset/presetManager.c
 M Core/Src/Soundengine/MIDI/MidiParser.c
?? tests/bug001/README.md
?? tests/bug001/run.py
?? tests/bug001/test_parser.c
?? tests/bug001/test_sweeps.c
?? tests/bug001/trap_stubs.c
```

Existing work was not discarded. A local backup was saved at `/tmp/bug001-existing-work-before-publication.tar.gz`, and the starting tracked diff at `/tmp/bug001-before-publication.patch`. Reconstruction from the report was unnecessary because all files were available locally.

Remote discovery found current `lab-setup` at `5829398cea5d337200393757e39a37d4d8148256`. Compared with the original investigation base, it adds only `AGENTS.md` and `.github/workflows/lxr-build.yml`; no firmware source differs. Its tree is `e206244c314757c48254faff4bb663bd1186027c`. Recursive inspection found no nested AGENTS instructions and no existing `docs/codex-berichte/` file on that base. The target development and new report branch names did not already exist.

Created the local development branch in the existing checkout with:

```sh
git switch -c bug001/filter-drive-fix origin/lab-setup
```

The two modified source files and five untracked tests carried over without conflicts. This is a change of base/branch for already implemented work, not a new implementation or redesign.

## 3. Hardware observations and unresolved provenance

The previous investigation recorded the following user observations; they were not independently repeated during this task:

- LXR-02 installed firmware v1.72, BassKick on Voice 1.
- Clicks while moving Morph through its full 0–255 range, including continued movement at either clamped endpoint.
- No clicks with the Morph knob stationary.
- Reproduces with identical kits and in an empty project without LFOs or automation.
- Every filter type similarly affected; filter TYPE OFF suppresses the clicks.
- More Filter Drive causes stronger disturbances; Drive=0 is quiet, Drive=127 severe.
- During movement the kick appears temporarily to lose Drive processing.
- Earlier comparisons suggested Voices 2/3 were unaffected.

No kit dump, reserved-slot byte values, runtime selector trace, audio capture or installed v1.72 binary was supplied. The source still declares version **1.7** in `CMakeLists.txt:23`; this patch does not change version metadata or establish exact correspondence to v1.72.

## 4. Technical findings supporting the correction

### Control path

The complete path is ADC movement → relative menu knob event → range-clamped `activeParameters[PAR_MORPH]` → `menu_parseGlobalParam()` → `preset_morph()` → periodic `preset_morph_tick()` → internal MIDI CC/CC2 handler → Filter Drive setter → filter audio DSP.

Relevant source locations (line references are in the published correction unless identified as original):

| Path | Lines / purpose |
| --- | --- |
| `Core/Src/IO/adcPots.c` | 174–245, especially 232: physical movement produces relative knob events |
| `Core/Src/menu/menu.c` | 3704–3773 clamping; 3917–3951 knob resolution; 4121–4128 storage; 4236–4242 dispatch; 3512–3516 calls Morph |
| `Core/Src/Preset/presetManager.c` | 1253 interpolation; 1264 transport predicate; 1271 immediate kit loading; 2141 Morph tick; 2222 Morph start |
| `Core/Src/Soundengine/Soundengine.c` | 77–114, particularly 112: tick service once per sound-engine processing call |
| `Core/Src/Soundengine/MIDI/MidiParser.c` | 134–143 NRPN helper; 147 CC handler; 170–176 CC0 protection; 905–913 CC2 protection; 1098 Drive cases; 1336 mute cases; 1440 notification; 1719 external MIDI entry |
| `Core/Src/Soundengine/DSPAudio/DrumVoice.c` | 265–295: transient/oscillator mixture through filter, envelope and separate voice distortion |
| `Core/Src/Soundengine/DSPAudio/ResonantFilter.c` | 89–98 Drive mapping; 157–225 filter OFF/input saturation behavior |

The enabled parameter enum in `Core/Inc/Parameters.h` has `END_OF_SOUND_PARAMETERS=247`, so a full Morph sweep processes indices 0–246. `PAR_MORPH=248` is outside that sound range. Obsolete comments/disabled duplicate enums have other counts; they are not the active layout.

### Confirmed reserved-command defect

The original sweep submitted every slot to the MIDI command interpreter. Low indices became `(index+1)&127` CC numbers; indices 128 onward became CC2 number `index-128`. That includes reserved transport slots:

| Array index | Symbol | Result before correction |
| ---: | --- | --- |
| 5 | `PAR_NRPN_DATA_ENTRY_COARSE` | CC6 executes NRPN Data Entry |
| 97 | `PAR_NRPN_FINE` | CC98 changes selected NRPN low bits |
| 98 | `PAR_NRPN_COARSE` | CC99 changes selected NRPN high bits |
| 127 | `PAR_BANK_CHANGE` | Wraps to CC0 |
| 128 | `PAR_FILTER_DRIVE_1` | CC2 number 0 applies intended Voice 1 Drive |

The NRPN selector initially equals 0 (`MidiParser.c:35`). The original NRPN helper narrowed that selector into an 8-bit CC2 number and dispatched it. CC2 number 0 is Voice 1 Filter Drive. Thus, with selector 0 and reserved slot 5 equal to zero, index 5 sets Voice 1 Drive to zero and index 128 restores the intended Drive. This unintended path is confirmed in source. Its exact zero-injection state on the hardware remains conditional on the missing kit/selector values.

Morph restarts at index 0 for every input event, including clamped endpoint events. Frequent events can repeat or prolong the wrong Drive state. This restart behavior is confirmed; actual event/service timing and starvation on the user's hardware are unmeasured. Scheduling is intentionally unchanged by the correction.

### Confirmed CC0 defect

The old handler did this before its CC switch:

```c
const uint16_t paramNr = msg.data1 - 1; // CC0 produces 65535
midiParser_originalCcValues[paramNr + 1] = value.itg;
```

The cache contains 255 bytes, valid indices 0–254. On the 32-bit target `paramNr+1` promotes to 65536, producing an out-of-bounds access with undefined behavior. The old path could also notify modulation with index 65535.

The prior investigation's pre-patch ELF/map showed a cache base of `0x2002653c` and resulting erroneous address `0x2003653c`, inside `seq_patternSet` at offset `0xd528`, not directly inside the filter structure. Those are historical layout observations, not assertions about the rebuilt artifacts or installed v1.72. The direct NRPN-to-Drive path is the stronger explanation for the filter-specific symptoms; sequencer-memory corruption is a separate confirmed defect with potential secondary effects.

External bank changes are forwarded by the outer MIDI parser to the UI. The CC handler's original bank-change switch body was inert/commented out. Early return for CC0 therefore protects bookkeeping while preserving the supported external bank-change forwarding path.

### Confirmed CC2/NRPN bounds defects and mute exception

CC2 bookkeeping derives index `data1+128`, which can exceed the sound range and cache. A 16-bit NRPN selection assigned to the 8-bit message field can truncate: for example, NRPN 256 aliases CC2 number 0 without the guard.

The enum also defines supported external NRPN mute commands 200–206. These are commands, not sound parameters; their derived indices 328–334 cannot enter the sound cache/modulation path. A blanket rejection of all high CC2 numbers would break muting. The correction explicitly preserves these commands while exempting them from invalid bookkeeping.

### DSP and confidence limits

In the enabled configuration, Filter Drive maps normalized input `d` to `0.4 + 6*d*d`; UI values 0 and 127 correspond to multipliers 0.4 and 6.4. Enabled filters use that multiplier before saturation, while Filter OFF returns before the affected processing. The unintended write is a discontinuous Drive change, not an established temporary bypass or filter-state reset. Drive is unsmoothed in this path; the separate voice-distortion smoothing option does not smooth `filter.drive`.

Identical-kit interpolation preserves each byte exactly for all Morph values, so it cannot by itself explain the identical-kit reproduction. DMA underruns and other parameter discontinuities were not established. Unchecked assumptions in broader modulation-destination accessors remain outside this minimal patch.

Likely causal ranking remains: (1) reserved NRPN command changing Drive, (2) the unsmoothed discontinuity making the excursion audible, (3) restart behavior extending/repeating it, (4) independent CC0 corruption, (5) unproven timing/other-parameter alternatives. No audio measurement in this task changes those confidence levels.

## 5. Exact recovered correction and discrepancy analysis

No implementation edits were necessary in this task. All seven local files matched the previous report's SHA-256 entries **byte-for-byte**, all five test-file bodies matched Appendix B exactly, and the complete normalized production diff matched Appendix A exactly. Source CRLF endings were preserved. Moving to current `lab-setup` required no conflicts or changes because its additions do not touch firmware.

The correction:

```c
static uint8_t preset_isTransportParameter(uint8_t paramNr)
{
    return paramNr == PAR_NRPN_DATA_ENTRY_COARSE ||
        paramNr == PAR_NRPN_FINE || paramNr == PAR_NRPN_COARSE ||
        paramNr == PAR_BANK_CHANGE;
}

// Before narrowing the NRPN selector:
if(midiParser_activeNrpnNumber > UINT8_MAX) return;

// Before low-CC subtraction/cache/notification:
if(msg.data1 == CC_BANK_CHANGE || msg.data1 > 127) return;

// Before CC2 sound bookkeeping:
const uint8_t isMuteCommand =
    msg.data1 >= CC2_MUTE_1 && msg.data1 <= CC2_MUTE_7;
if(!isMuteCommand && (paramNr >= END_OF_SOUND_PARAMETERS ||
    paramNr >= sizeof(midiParser_originalCcValues))) return;
```

Both Morph and immediate kit-load sweeps skip those four reserved transport commands. Morph still consumes the reserved slot's original service tick, preserving index progression and restart behavior. The immediate sweep still calls `soundengine_process()` once per slot, including skipped slots. Mutes bypass cache writes and modulation notification but retain their command execution.

No filter algorithm, Drive curve, smoothing, interpolation, scheduling, routing policy, preset format, parameter layout, version metadata, unrelated source or workflow was changed.

**Discrepancy found in the prior report's artifact metadata:** its image byte counts 275,132 and 275,180 were actually raw payload sizes printed by `mkimage.py`, not complete `.img` file sizes. The image includes a 16-byte header. Correct total file sizes are **275,148 Release** and **275,196 Debug**. The new builds' complete-image SHA-256 values are identical to the previous report, so this is a reporting-label correction, not a firmware-content discrepancy. The old report is preserved without alteration.

The old report also correctly described its then-current `lab-setup` base and absence of AGENTS at that time. Current `lab-setup` has advanced to include AGENTS; this task uses and records the new base rather than silently reusing the old one.

## 6. Complete file scope and review

The complete production and test diff was reviewed before publication. The local prepared index tree and GitHub prepared tree both equal `f4b714de371ba006e17fb5e6d29334ae669f260f`. That equality proves the published candidate contains precisely the reviewed local tree, including unchanged base files and exact CRLF content.

| Changed file | Added lines | Deleted lines |
| --- | ---: | ---: |
| `Core/Src/Preset/presetManager.c` | 21 | 0 |
| `Core/Src/Soundengine/MIDI/MidiParser.c` | 14 | 12 |
| `tests/bug001/README.md` | 26 | 0 |
| `tests/bug001/run.py` | 77 | 0 |
| `tests/bug001/test_parser.c` | 157 | 0 |
| `tests/bug001/test_sweeps.c` | 89 | 0 |
| `tests/bug001/trap_stubs.c` | 48 | 0 |
| **Total** | **432** | **12** |

The two production files alone have 35 insertions and 12 deletions; the five-file test suite has 397 lines. The report-only branch adds only this Markdown report relative to `lab-setup`. Complete source/test changes appear in Appendix A for self-contained review/reconstruction; immutable GitHub links use the published commit, so the branch moving later will not invalidate source provenance.

Review decisions: preserve the minimal documented patch rather than redesigning the MIDI abstraction or adding smoothing; explicitly handle supported mute command semantics; preserve existing cache numbering and Morph cadence; do not expand into other unchecked modulation accessors. No source deviation required justification because there was no source deviation.

## 7. Fresh regression run

Actually run in the existing checkout on the new base:

```sh
python3 tests/bug001/run.py
```

Exit status: **0**. Full captured output:

```text
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence
```

The five documented files form one regression suite with two native C executables; the README is documentation, not a fifth independent executable. The runner compiles the complete production MIDI parser directly into its fixture, plus aborting traps for unrelated firmware dependencies. It extracts unmodified production sweep/interpolation bodies and compiles them with the real parameter-order array and a recording MIDI sink. There are no production test hooks.

Coverage exercised:

- CC0 under all cache/float-modulator flag combinations: no invalid cache write or notification and no selector change.
- External bank-change UI forwarding and valid CC/CC127 cache mapping.
- Valid NRPN Drive updates, including the external MIDI routing/channel path.
- Last sound-index cache/notification boundary.
- Eleven selected invalid/high NRPN selections, including truncation aliases and the 14-bit upper end.
- All seven external NRPN mute/unmute commands, plus an external-MIDI mute path.
- Every out-of-sound-range CC2 byte under all four cache/modulator flag combinations, with supported mute commands preserved.
- All 256 Morph values with identical kits and nonzero reserved slots, exactly 243 nontransport dispatches per full sweep.
- Existing tick/index progression, idle behavior, repeated restarts and eventual convergence.
- Immediate kit sweep dispatch and exactly 247 audio-service calls.

AddressSanitizer and UndefinedBehaviorSanitizer are enabled with fail-on-UB recovery disabled. Leak detection defaults to disabled because LeakSanitizer cannot operate under the execution environment's tracing; that does not disable ASan bounds checking or UBSan. The fixture's Drive setter records normalized controls rather than simulating audio. Trap stubs must never be executed; unanticipated paths abort. This is a targeted control-path suite, not hardware or full-DSP validation.

The previous report also records expected failing negative controls against the original parser/sweeps. Those historical negative controls were read but were not rerun in this publication task. They must not be counted as fresh test runs here.

## 8. Fresh clean firmware builds

Both presets were configured and **clean rebuilt**, not merely checked for an up-to-date incremental state. Commands:

```sh
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH \
  cmake --preset release -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH \
  cmake --build --preset release --clean-first --parallel 2

PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH \
  cmake --preset debug -DARM_TOOLCHAIN_DIR=/workspace/.onboarding/toolchain/usr/bin
PATH=/workspace/.onboarding/python/bin:/workspace/.onboarding/toolchain/usr/bin:$PATH \
  cmake --build --preset debug --clean-first --parallel 2
```

Release and Debug configure/build exit statuses: **0**. Each completed all 122 compilation/link/output steps following cleanup. There were no `warning:`, `error:` or `FAILED:` markers in either captured build log. Existing project flags suppress some compiler warnings; this is not a claim of a full warning audit. No bootloader was configured; firmware-only outputs were generated. Both presets retain the project's `-O3` configuration (Debug also has symbols).

Tool versions actually used:

```text
Arm GNU: arm-none-eabi-gcc (15:14.2.rel1-1) 14.2.1 20241119
CMake: 3.31.10
Ninja: 1.13.0.git.kitware.jobserver-pipe-1
Python: 3.12.14
Native C compiler: Debian GCC 14.2.0-19, 14.2.0
```

| Metric | Release | Debug |
| --- | ---: | ---: |
| ITCM | 15,008 / 16,384 bytes, 91.60% | 15,008 / 16,384 bytes, 91.60% |
| DTCM | 107,392 / 131,072 bytes, 81.93% | 107,392 / 131,072 bytes, 81.93% |
| RAM | 271,040 / 393,216 bytes, 68.93% | 271,040 / 393,216 bytes, 68.93% |
| FLASH region | 223,128 / 1,015,808 bytes, 21.97% | 223,176 / 1,015,808 bytes, 21.97% |
| GNU size text | 274,272 | 274,320 |
| GNU size data | 852 | 852 |
| GNU size bss | 340,576 | 340,576 |
| GNU size total | 615,700 (`0x96514`) | 615,748 (`0x96544`) |

All linker regions fit. Region use and GNU section totals measure different things and are not interchangeable. No new unpatched baseline build was performed for size/performance deltas.

## 9. Independent generated-image validation

Read the existing generator `cmake/tools/mkimage.py`, whose format is a 16-byte header followed by the raw objcopy binary:

```text
offset 0:  8 bytes ASCII magic, LXRV2IMG for desktop
offset 8:  4 bytes unsigned payload length, little-endian
offset 12: 1 byte checksum = 0xFF - (sum(payload bytes) & 0xFF)
offset 13: 3 zero padding bytes
offset 16: raw firmware payload
```

Ran the independent validation script reproduced in Appendix B. It asserts magic, zero padding, declared/actual payload length, exactly 16 header bytes/no extra trailing data, nonempty payload within the existing bootloader/generator limit, byte equality with the generated `.bin`, valid additive checksum, raw-not-ELF payload, aligned SRAM initial stack, and an odd Thumb reset vector within the loaded application flash range beginning at `0x08008000`.

Both validations exited **0** and reported **PASS**:

| Property | Release | Debug |
| --- | --- | --- |
| Magic | `LXRV2IMG` | `LXRV2IMG` |
| Complete image bytes | **275,148** | **275,196** |
| Header bytes | 16 | 16 |
| Declared/raw payload bytes | 275,132 | 275,180 |
| Stored checksum | `0x1B` | `0xC5` |
| Initial MSP | `0x20080000` | `0x20080000` |
| Reset vector | `0x08014E5D` | `0x08014E5D` |
| Payload equals generated `.bin` | yes | yes |
| Header/checksum/vector assertions | PASS | PASS |

These structural checks are additional to successful image generation; they do not prove on-device boot or audio behavior.

SHA-256 of complete images:

```text
65441a47a0352d2ba69d814b044c0d014cd76fe9cba130552223489c0e028d7d  build/release/LXR_V2_V1.7.img
418c0510dcf93bfcc7a5fe1832a52f3e930d9611e878b40c01bbbd9c944596ce  build/debug/LXR_V2_V1.7.img
```

SHA-256 of raw payload binaries:

```text
7d4505da7bd59887d0ca690e4464a32099cda4cc3045c34094ee60ae33070553  build/release/LXR_V2_STM32F765.bin
2f91fd5b2225584b3ca08479a5319494f3c8ff1f1d127317760e47d998040c27  build/debug/LXR_V2_STM32F765.bin
```

SHA-256 of ELF files:

```text
5f09d248ca6626ac7dd032204b04c26d8b135c5527f9f19bf47f7190aa40a5fa  build/release/LXR_V2_STM32F765.elf
1adbd7d75124c337fd6d6adfb9d034683506853f2e78583ec08b2556ec9f86f9  build/debug/LXR_V2_STM32F765.elf
```

All binary/ELF/image/map outputs remain local under ignored `build/` directories. No binary or generated artifact was uploaded, attached to a release, committed or pushed. The unchanged base workflow runs only for pushes to `lab-setup`, so these development/report branch publications do not trigger that artifact-upload workflow. No workflow was modified or dispatched.

## 10. Publication mechanism, exact ancestry and remote verification

Native read attempt:

```sh
git fetch origin refs/heads/lab-setup:refs/remotes/origin/lab-setup
```

It exited 128 with:

```text
fatal: unable to access 'https://github.com/DocLXR-02/lxr-desktop-app.git/': Failed to connect to proxy port 8080 after 0 ms: Could not connect to server
```

The connected GitHub tool remained available. It fetched exact commit/tree/blob metadata for the current base and its predecessor. The corresponding Git objects were reconstructed locally and verified against their GitHub SHA-1 object IDs before being written to the local object database; no substitute base commit was invented. Only the remote-tracking `origin/lab-setup` ref was added. The existing branch/work was then carried to the new branch with `git switch`. This was a transport fallback, not a code change or rewrite of remote history.

After validation, explicitly staged only the seven allowed paths. Ran `git -c core.whitespace=cr-at-eol diff --cached --check`; it passed. Reviewed the complete staged production/test diff. Captured the staged tree with `git write-tree`.

Publication sequence through the connected GitHub API:

1. `github_create_tree` with base tree `e206244c314757c48254faff4bb663bd1186027c` and exactly seven UTF-8 source/test entries, retaining CRLF bytes.
2. Asserted returned tree `f4b714de371ba006e17fb5e6d29334ae669f260f` equals the staged local tree.
3. `github_create_commit` with that tree and sole parent `5829398cea5d337200393757e39a37d4d8148256`.
4. `github_create_branch` created the previously absent `bug001/filter-drive-fix` ref at new commit `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`.
5. Read all seven files back from that exact named branch and compared complete contents and blob SHA values against the validated local manifest.
6. Read the commit metadata and branch list, verifying the exact sole parent, seven-file change list, new branch head and unchanged protected-by-instruction branches.
7. Imported the exact published commit into the local object database and advanced only the local new development branch by an old-SHA-checked ref update after confirming ancestry. The index/worktree match that commit and are clean.

No force update, force-push, merge or PR creation was used. At verification:

```text
main                                      a43cd4fcda23ec3d577dc425261c3eb28bd5abff
lab-setup                                 5829398cea5d337200393757e39a37d4d8148256
bug001/filter-drive-fix                    7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0
codex-reports/2026-10-26-bug001-handoff      9028075917b2bc1a8df43dfb2c6e0d2dab66595d
```

The old report branch/file was not modified. This new report's branch is created separately from `lab-setup`, and only the new Markdown file is published there. Final reporting must verify that exact report URL and file content after publication, not treat an attempted write as success.

## 11. Errors, limitations and local records

| Event | Resolution / impact |
| --- | --- |
| Native Git fetch could not connect to execution proxy | Used connected GitHub API, hash-verified exact objects, committed/published through GitHub, read back and verified files. No claim of native `git push` execution. |
| `git branch --set-upstream-to=origin/bug001/filter-drive-fix` rejected the manually imported remote-tracking ref as a tracking start point | This checkout's tracking setup does not recognize that ref for automatic upstream setup. Left configuration unchanged. The local branch and remote ref both have the correct commit; status is clean. Native fetch/push/upstream configuration remains an environment/setup limitation, not a publication failure. |
| Prior report called payload sizes image sizes | Corrected documentation here using independently decoded headers and actual file sizes; no source change and no overwrite of old report. |
| LeakSanitizer unavailable under tracing | Existing runner disables leak detection by default; ASan/UBSan pass. No new workaround changed the documented suite. |
| CRLF source whitespace checks | Used per-command `core.whitespace=cr-at-eol`, preserving original endings; checks pass. |

No regression or firmware build failed in this task. No automatic approval rejection occurred. Historical environment/compiler errors in the old report were not repeated here except the explicitly described native Git transport issue. GitHub API publication and seven-file verification succeeded.

Local records (ephemeral and not pushed):

```text
/tmp/bug001-existing-work-before-publication.tar.gz
/tmp/bug001-before-publication.patch
/tmp/bug001-publish-previous-report.md
/tmp/bug001-publish-tests.log
/tmp/bug001-publish-release-configure.log
/tmp/bug001-publish-debug-configure.log
/tmp/bug001-publish-release-build.log
/tmp/bug001-publish-debug-build.log
/tmp/bug001-publish-image-validation.log
/tmp/bug001-publish-images.json
/tmp/bug001-publish-manifest.json
/tmp/bug001-published-complete.patch
/tmp/bug001-validate-images.py
```

Do not publish temporary Git object metadata files: they are implementation details of the transport fallback and may contain ordinary commit author identity metadata. This report deliberately contains no author emails, credentials, tokens or other personal/secret values.

## 12. Remaining risks and recommended continuation

- Hardware audio and on-device boot remain untested. No hardware was flashed or directly exercised.
- The exact v1.72 source/binary correspondence, BassKick reserved-slot values and runtime NRPN selector remain unknown.
- Real-time event cadence, audio deadlines and physical endpoint behavior have no new on-device measurements. The original restart policy and unsmoothed legitimate parameter steps remain intentionally unchanged.
- The host suite stubs audio/hardware effects and extracts selected sweep functions. It is appropriate for the targeted dispatch/bounds correction, not a substitute for MCU integration/audio testing.
- Broader modulation-destination bounds assumptions noted in the previous investigation are outside this patch.
- ITCM remains within limits at 91.60% utilization; no timing/performance measurement was made.
- The inherited GitHub workflow targets `lab-setup` only. This publication has local build/test/image validation, not a claim of a new GitHub Actions run for the development branch.

Next action is review of the immutable firmware commit and this report. Do not create a PR, merge, flash or direct hardware tests based on this publication authorization alone. Hardware testing needs explicit approval and the backup/recovery prerequisites in AGENTS.md. If approved later, exercise identical/different kits, Drive 0/64/127, all filters/OFF, full-range and endpoint Morph movement, Voices 1–6, external bank changes/NRPN Drive/mutes, and immediate/deferred kit loading. Capture actual Drive writes and audio together before considering additional DSP or scheduling changes.

For any future source change or rebase, rerun the suite, both builds and image validation. Do not silently broaden scope while reviewing the remaining risks.

## Appendix A — Complete published source/test diff

This is the complete seven-file commit diff against current `lab-setup`; active firmware files in the commit retain CRLF, while Markdown display normalizes line endings. It includes the full five-file regression suite for continuation without needing the previous conversation.

```diff
diff --git a/Core/Src/Preset/presetManager.c b/Core/Src/Preset/presetManager.c
index 41ef096..b13a536 100644
--- a/Core/Src/Preset/presetManager.c
+++ b/Core/Src/Preset/presetManager.c
@@ -1259,6 +1259,14 @@ static uint8_t interpolate(uint8_t a, uint8_t b, uint8_t x)
 	//return ((a*255) + (b-a)*x)/255;
 }
 
+//----------------------------------------------------
+// These slots store MIDI transport state, not sound parameters.
+static uint8_t preset_isTransportParameter(uint8_t paramNr)
+{
+	return paramNr == PAR_NRPN_DATA_ENTRY_COARSE ||
+		paramNr == PAR_NRPN_FINE || paramNr == PAR_NRPN_COARSE ||
+		paramNr == PAR_BANK_CHANGE;
+}
 //----------------------------------------------------
 IN_ITCM_RAM void preset_sendDrumsetParametersImmediate()
 {
@@ -1305,6 +1313,11 @@ IN_ITCM_RAM void preset_sendDrumsetParametersImmediate()
 		{
 
 			uint8_t paramNumber = parameterOrder[i];
+			if(preset_isTransportParameter(paramNumber))
+			{
+				soundengine_process();
+				continue;
+			}
 
 			//const uint8_t morph = activeParameters[PAR_MORPH];
 
@@ -2132,6 +2145,14 @@ void preset_morph_tick()
 	if(morph_state)
 	{
 
+		// Consume the reserved slot's tick without executing a MIDI command.
+		// All transport slots precede the final sound parameter.
+		if(preset_isTransportParameter(morph_index))
+		{
+			morph_index++;
+			return;
+		}
+
 		uint8_t morph;
 
 		//Do not morph audio routing
diff --git a/Core/Src/Soundengine/MIDI/MidiParser.c b/Core/Src/Soundengine/MIDI/MidiParser.c
index bb68cb4..d119e9f 100644
--- a/Core/Src/Soundengine/MIDI/MidiParser.c
+++ b/Core/Src/Soundengine/MIDI/MidiParser.c
@@ -133,6 +133,9 @@ float midiParser_calcDetune(uint8_t value)
 //-----------------------------------------------------------
 static void midiParser_nrpnHandler(uint16_t value)
 {
+	// Reject unsupported 14-bit selections before narrowing to a MIDI byte.
+	if(midiParser_activeNrpnNumber > UINT8_MAX) return;
+
 	MidiMsg msg2;
 	msg2.status = MIDI_CC2;
 	msg2.data1 = midiParser_activeNrpnNumber;
@@ -164,6 +167,10 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 	if((msg.status &0xF0) == MIDI_CC)
 	{
 
+		// Bank changes are forwarded to the UI by parseMidiMessage(). They have
+		// no sound parameter and must not enter the cache or modulation path.
+		if(msg.data1 == CC_BANK_CHANGE || msg.data1 > 127) return;
+
 		const uint16_t paramNr = msg.data1-1;
 		if(updateOriginalValue && !isFloatModulator) {
 			midiParser_originalCcValues[paramNr+1] = value.itg;
@@ -176,16 +183,6 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 
 			break;
 
-		case CC_BANK_CHANGE:
-			// bank change (coarse) selects kit (sound)
-			/*
-			 * already send in parseMidiMessage()
-			uart_sendFrontpanelByte(MIDI_CC);
-			uart_sendFrontpanelByte(CC_BANK_CHANGE);
-			uart_sendFrontpanelByte(value.itg);
-			*/
-			break;
-
 		case NRPN_DATA_ENTRY_COARSE:
 			midiParser_nrpnHandler(value.itg);
 			return;
@@ -906,8 +903,13 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 	else //MIDI_CC2
 	{
 		const uint16_t paramNr = msg.data1 + 1 + 127;
+		// External NRPN mute commands are not sound parameters and have no
+		// cache entry or modulation target. Preserve their command handling.
+		const uint8_t isMuteCommand = msg.data1 >= CC2_MUTE_1 && msg.data1 <= CC2_MUTE_7;
+		if(!isMuteCommand && (paramNr >= END_OF_SOUND_PARAMETERS ||
+			paramNr >= sizeof(midiParser_originalCcValues))) return;
 
-		if(updateOriginalValue && !isFloatModulator) {
+		if(updateOriginalValue && !isFloatModulator && !isMuteCommand) {
 			midiParser_originalCcValues[paramNr] = value.itg;
 		}
 		switch(msg.data1)
@@ -1433,7 +1435,7 @@ void midiParser_ccHandler(MidiMsg msg, uint8_t updateOriginalValue, uint8_t isFl
 			default:
 				break;
 		}
-		if(!isFloatModulator)
+		if(!isFloatModulator && !isMuteCommand)
 		{
 			modNode_originalValueChanged(paramNr);
 		}
diff --git a/tests/bug001/README.md b/tests/bug001/README.md
new file mode 100644
index 0000000..8f22d91
--- /dev/null
+++ b/tests/bug001/README.md
@@ -0,0 +1,26 @@
+# Bug 001 regressions
+
+Run `python3 tests/bug001/run.py` from the repository root. Requires a native
+GNU-compatible C compiler with AddressSanitizer/UndefinedBehaviorSanitizer.
+Temporary binaries and generated files are removed after the run.
+
+The parser test compiles the complete production `MidiParser.c`. It observes
+Drive writes, modulation notifications, mute commands and external bank-change
+forwarding. Unrelated DSP/hardware calls abort through link stubs. The Drive stub
+records the normalized input; it does not model the filter or its Drive curve.
+
+The sweep test compiles the unmodified production interpolation, reserved-slot
+predicate, immediate kit sweep, Morph tick and Morph start functions extracted
+by the runner, plus the real parameter-order array. The MIDI sink records every
+dispatch and rejects transport commands. This avoids bringing SD-card/hardware
+code into the host process without adding test hooks to firmware.
+
+Coverage includes all 256 Morph positions with identical kits, nonzero reserved
+slots, existing restart/tick behavior, kit-load audio service cadence, CC0 with
+all cache/modulator flags, valid CC/CC2/NRPN mapping, all seven external NRPN
+mutes, invalid/high NRPN selectors, and every out-of-range CC2 byte with all
+cache/modulator flags. Sanitizers check exercised memory accesses.
+Leak detection is disabled by default because LeakSanitizer cannot run under
+ptrace; AddressSanitizer and UndefinedBehaviorSanitizer remain enabled.
+
+These are control-path tests, not an audio simulation or on-device validation.
diff --git a/tests/bug001/run.py b/tests/bug001/run.py
new file mode 100644
index 0000000..d91117d
--- /dev/null
+++ b/tests/bug001/run.py
@@ -0,0 +1,77 @@
+#!/usr/bin/env python3
+# SPDX-License-Identifier: GPL-3.0-only
+"""Run focused production-code regressions using a native C compiler."""
+import os
+from pathlib import Path
+import re
+import subprocess
+import tempfile
+
+ROOT = Path(__file__).resolve().parents[2]
+HERE = Path(__file__).resolve().parent
+
+
+def function(source, name):
+    # Mask comments/strings before counting braces; return the original text.
+    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
+                    lambda m: " " * len(m[0]), source, flags=re.S)
+    match = re.search(r"^(?:IN_ITCM_RAM )?(?:static )?(?:uint8_t|void) "
+                      + name + r"\([^;]*?\)\s*\{", masked, re.M)
+    if not match:
+        raise RuntimeError(f"Production function not found: {name}")
+    depth = 1
+    end = match.end()
+    while depth:
+        depth += (masked[end] == "{") - (masked[end] == "}")
+        end += 1
+    return source[match.start():end]
+
+
+def run(*args):
+    env = os.environ.copy()
+    # LeakSanitizer cannot run in ptraced execution environments. These tests
+    # target bounds/UB, not allocation; retain ASan and UBSan instrumentation.
+    env.setdefault("ASAN_OPTIONS", "detect_leaks=0")
+    subprocess.run(args, cwd=ROOT, env=env, check=True)
+
+
+def main():
+    includes = [".", "Core/Inc", "Drivers/CMSIS/Include",
+                "Drivers/CMSIS/Device/ST/STM32F7xx/Include",
+                "Drivers/STM32F7xx_HAL_Driver/Inc",
+                "Middlewares/Third_Party/FatFs/src",
+                "Middlewares/JS/STM32_USB_Device_Library/Core/Inc",
+                "Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc",
+                "USB_DEVICE_MIDI/Target"]
+    # ARM-specific inline DSP intrinsics are not executed by these tests.
+    # _Float16 supplies the host spelling of the firmware's half-float type.
+    flags = [os.environ.get("CC", "cc"), "-std=gnu11", "-O1", "-g",
+             "-DSTM32F765xx", "-DUSE_HAL_DRIVER", "-D__fp16=_Float16",
+             "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast",
+             "-Wno-implicit-function-declaration", "-Wno-unused-function",
+             "-ffunction-sections", "-fdata-sections", "-fno-pie", "-no-pie",
+             "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
+             "-fno-omit-frame-pointer"]
+    flags += ["-I" + str(ROOT / p) for p in includes]
+    with tempfile.TemporaryDirectory(prefix="lxr-bug001-") as tmp:
+        tmp = Path(tmp)
+        parser = tmp / "test_parser"
+        run(*flags, str(HERE / "test_parser.c"), str(HERE / "trap_stubs.c"),
+            "-Wl,--gc-sections", "-lm", "-o", str(parser))
+        run(str(parser))
+
+        source = (ROOT / "Core/Src/Preset/presetManager.c").read_text()
+        names = ["interpolate", "preset_isTransportParameter",
+                 "preset_sendDrumsetParametersImmediate", "preset_morph_tick",
+                 "preset_morph"]
+        (tmp / "sweep_functions.inc").write_text(
+            "\n\n".join(function(source, name) for name in names))
+        sweeps = tmp / "test_sweeps"
+        run(*flags, "-I" + str(tmp), str(HERE / "test_sweeps.c"),
+            str(ROOT / "Core/Src/Parameters.c"), "-Wl,--gc-sections",
+            "-o", str(sweeps))
+        run(str(sweeps))
+
+
+if __name__ == "__main__":
+    main()
diff --git a/tests/bug001/test_parser.c b/tests/bug001/test_parser.c
new file mode 100644
index 0000000..3ca4dbd
--- /dev/null
+++ b/tests/bug001/test_parser.c
@@ -0,0 +1,157 @@
+// SPDX-License-Identifier: GPL-3.0-only
+// Compile the complete production parser, including its static NRPN state.
+#include <assert.h>
+#include <stdio.h>
+#include <string.h>
+#include "Core/Src/Soundengine/MIDI/MidiParser.c"
+
+DrumVoice voiceArray[NUM_DRUM_VOICES];
+SnareVoice snareVoice;
+HiHatVoice hatVoice;
+CymbalVoice cymbalVoice;
+FxBus fxBus;
+ModulationNode velocityModulators[6];
+uint8_t activeParameters[NUM_PARAMS];
+uint8_t mixer_audioRouting[6];
+float mixer_decimation_rate[7];
+uint8_t frontParser_activeTrack, menu_activePage;
+PatternSet seq_patternSet;
+volatile uint32_t systick_ticks;
+
+static unsigned notifications, drive_writes, mute_writes, forwarded;
+static uint8_t mute_voice, mute_value, forwarded_bytes[3];
+static uint16_t last_notification;
+
+void modNode_originalValueChanged(uint16_t idx)
+{
+    assert(idx < END_OF_SOUND_PARAMETERS);
+    last_notification = idx;
+    notifications++;
+}
+void SVF_setDrive(ResonantFilter *filter, float drive)
+{
+    filter->drive = drive;
+    drive_writes++;
+}
+void seq_setMute(uint8_t voice, uint8_t muted)
+{
+    assert(voice < 7);
+    mute_voice = voice;
+    mute_value = muted;
+    mute_writes++;
+}
+void seq_recordAutomation(uint8_t track, uint8_t param, uint8_t value)
+{
+    (void)track; (void)param; (void)value;
+}
+void uart_sendFrontpanelByte(uint8_t value)
+{
+    assert(forwarded < sizeof(forwarded_bytes));
+    forwarded_bytes[forwarded++] = value;
+}
+
+static void cc(uint8_t number, uint8_t value)
+{
+    MidiMsg msg = {.status = MIDI_CC, .data1 = number, .data2 = value};
+    midiParser_ccHandler(msg, 1, 0, 0.f);
+}
+static void nrpn(uint16_t number, uint8_t value)
+{
+    cc(NRPN_FINE, number & 127);
+    cc(NRPN_COARSE, (number >> 7) & 127);
+    cc(NRPN_DATA_ENTRY_COARSE, value);
+}
+static void external_cc(uint8_t number, uint8_t value)
+{
+    MidiMsg msg = {.status = MIDI_CC, .data1 = number, .data2 = value};
+    forwarded = 0;
+    midiParser_parseMidiMessage(msg);
+    assert(forwarded == 3 && forwarded_bytes[0] == MIDI_CC);
+    assert(forwarded_bytes[1] == number && forwarded_bytes[2] == value);
+}
+
+int main(void)
+{
+    uint8_t cache[sizeof(midiParser_originalCcValues)];
+    memset(midiParser_originalCcValues, 0xa5, sizeof(cache));
+    memcpy(cache, midiParser_originalCcValues, sizeof(cache));
+
+    // CC0 must not touch the cache, notify modulators, or change the selector.
+    midiParser_activeNrpnNumber = 3;
+    for(unsigned flags = 0; flags < 4; flags++) {
+        MidiMsg msg = {.status = MIDI_CC, .data1 = 0, .data2 = 127};
+        midiParser_ccHandler(msg, flags & 1, (flags >> 1) & 1, 0.5f);
+    }
+    assert(!notifications && !memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
+    assert(midiParser_activeNrpnNumber == 3);
+
+    // Bank change still reaches the UI through the external MIDI entry point.
+    MidiMsg bank = {.status = MIDI_CC, .data1 = CC_BANK_CHANGE, .data2 = 17};
+    midiParser_parseMidiMessage(bank);
+    assert(forwarded == 3 && forwarded_bytes[0] == MIDI_CC);
+    assert(forwarded_bytes[1] == CC_BANK_CHANGE && forwarded_bytes[2] == 17);
+    assert(!notifications && !memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
+
+    // Valid low CC, CC127 and CC2 Drive retain their cache/notification mapping.
+    cc(CC_MOD_WHEEL, 23);
+    assert(midiParser_originalCcValues[1] == 23 && last_notification == 0);
+    cc(AMOUNT_LFO6, 31);
+    assert(midiParser_originalCcValues[127] == 31 && last_notification == 126);
+    nrpn(CC2_FILTER_DRIVE_1, 127);
+    assert(drive_writes == 1 && voiceArray[0].filter.drive == 1.f);
+    assert(midiParser_originalCcValues[128] == 127 && last_notification == 128);
+    nrpn(CC2_FILTER_DRIVE_3, 64);
+    assert(drive_writes == 2 && voiceArray[2].filter.drive == 64 / 127.f);
+    nrpn(END_OF_SOUND_PARAMETERS - 129, 42);
+    assert(last_notification == END_OF_SOUND_PARAMETERS - 1);
+    assert(midiParser_originalCcValues[END_OF_SOUND_PARAMETERS - 1] == 42);
+
+    // Valid NRPN also works through the external MIDI routing/channel path.
+    external_cc(NRPN_FINE, CC2_FILTER_DRIVE_2);
+    external_cc(NRPN_COARSE, 0);
+    external_cc(NRPN_DATA_ENTRY_COARSE, 31);
+    assert(drive_writes == 3 && voiceArray[1].filter.drive == 31 / 127.f);
+
+    // A high 14-bit selector must never alias a low parameter or mute command.
+    const uint16_t invalid[] = {119, 126, 127, 128, 199, 207, 255, 256, 258, 456, 16383};
+    for(unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
+        cc(NRPN_FINE, invalid[i] & 127);
+        cc(NRPN_COARSE, invalid[i] >> 7);
+        unsigned before = notifications;
+        cc(NRPN_DATA_ENTRY_COARSE, 0);
+        assert(notifications == before && drive_writes == 3 && !mute_writes);
+    }
+
+    // Special external NRPN mutes work, without cache access or notification.
+    for(unsigned voice = 0; voice < 7; voice++) {
+        cc(NRPN_FINE, (CC2_MUTE_1 + voice) & 127);
+        cc(NRPN_COARSE, (CC2_MUTE_1 + voice) >> 7);
+        unsigned before = notifications;
+        cc(NRPN_DATA_ENTRY_COARSE, 127);
+        assert(notifications == before && mute_voice == voice && mute_value == 1);
+        cc(NRPN_DATA_ENTRY_COARSE, 0);
+        assert(notifications == before && mute_value == 0);
+    }
+    assert(mute_writes == 14);
+    external_cc(NRPN_FINE, CC2_MUTE_7 & 127);
+    external_cc(NRPN_COARSE, CC2_MUTE_7 >> 7);
+    unsigned before = notifications;
+    external_cc(NRPN_DATA_ENTRY_COARSE, 127);
+    assert(mute_writes == 15 && mute_voice == 6 && mute_value == 1);
+    assert(notifications == before);
+
+    // Exhaust every CC2 byte for all cache/modulator flag combinations.
+    for(unsigned flags = 0; flags < 4; flags++) {
+        for(unsigned number = 0; number < 256; number++) {
+            unsigned param = number + 128;
+            if(param < END_OF_SOUND_PARAMETERS) continue; // normal DSP covered by firmware builds
+            MidiMsg msg = {.status = MIDI_CC2, .data1 = number, .data2 = 0};
+            unsigned before = notifications;
+            memcpy(cache, midiParser_originalCcValues, sizeof(cache));
+            midiParser_ccHandler(msg, flags & 1, (flags >> 1) & 1, 0.f);
+            assert(notifications == before);
+            assert(!memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
+        }
+    }
+    puts("PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds");
+}
diff --git a/tests/bug001/test_sweeps.c b/tests/bug001/test_sweeps.c
new file mode 100644
index 0000000..b06a004
--- /dev/null
+++ b/tests/bug001/test_sweeps.c
@@ -0,0 +1,89 @@
+// SPDX-License-Identifier: GPL-3.0-only
+#include <assert.h>
+#include <stdio.h>
+#include <string.h>
+#include "menu/menu.h"
+#include "frontPanelParser_ui.h"
+#include "Soundengine/MIDI/MidiParser.h"
+
+uint8_t activeParameters[NUM_PARAMS];
+uint8_t mainParameters[NUM_KIT_SLOTS][END_OF_SOUND_PARAMETERS];
+uint8_t *morphParameters = mainParameters[0];
+const ModTarg modTargets[] = {{0, 0}};
+static uint8_t morph_state, morph_index;
+static unsigned seen[END_OF_SOUND_PARAMETERS], dispatched, audio_calls;
+
+void midiParser_ccHandler(MidiMsg msg, uint8_t update, uint8_t floating, float v)
+{
+    (void)v;
+    unsigned param;
+    assert(update == 1 && floating == 0);
+    if(msg.status == MIDI_CC) {
+        assert(msg.data1 != CC_BANK_CHANGE);
+        assert(msg.data1 != NRPN_DATA_ENTRY_COARSE);
+        assert(msg.data1 != NRPN_FINE && msg.data1 != NRPN_COARSE);
+        param = msg.data1 - 1;
+    } else {
+        assert(msg.status == CC_2);
+        param = msg.data1 + 128;
+    }
+    assert(param < END_OF_SOUND_PARAMETERS);
+    assert(msg.data2 == activeParameters[param]); // identical kits must stay identical
+    seen[param]++;
+    dispatched++;
+}
+void frontPanel_sendData(uint8_t status, uint8_t data1, uint8_t data2)
+{
+    (void)data1; (void)data2;
+    assert(status == CC_VELO_TARGET || status == CC_LFO_TARGET);
+}
+void soundengine_process(void) { audio_calls++; }
+
+// The runner inserts unmodified production function bodies here.
+#include "sweep_functions.inc"
+
+static void check_sweep(void)
+{
+    assert(dispatched == END_OF_SOUND_PARAMETERS - 4);
+    for(unsigned i = 0; i < END_OF_SOUND_PARAMETERS; i++)
+        assert(seen[i] == (preset_isTransportParameter(i) ? 0u : 1u));
+    memset(seen, 0, sizeof(seen));
+    dispatched = 0;
+}
+
+int main(void)
+{
+    // Nonzero transport slots ensure their values cannot silently act as commands.
+    memset(activeParameters, 73, END_OF_SOUND_PARAMETERS);
+    for(unsigned i = 0; i < 6; i++) {
+        activeParameters[PAR_VEL_DEST_1 + i] = 0;
+        activeParameters[PAR_TARGET_LFO1 + i] = 0;
+        activeParameters[PAR_VOICE_LFO1 + i] = 1;
+    }
+    memcpy(morphParameters, activeParameters, END_OF_SOUND_PARAMETERS);
+    for(unsigned morph = 0; morph < 256; morph++) {
+        activeParameters[PAR_MORPH] = morph;
+        preset_morph(morph);
+        for(unsigned tick = 0; tick < END_OF_SOUND_PARAMETERS; tick++) {
+            assert(morph_state && morph_index == tick);
+            preset_morph_tick();
+        }
+        assert(!morph_state);
+        preset_morph_tick(); // idle must not dispatch
+        check_sweep();
+    }
+    // Repeated events retain the existing restart behavior and final convergence.
+    for(unsigned event = 0; event < 16; event++) {
+        preset_morph(255);
+        for(unsigned tick = 0; tick < 110; tick++) preset_morph_tick();
+    }
+    memset(seen, 0, sizeof(seen)); dispatched = 0;
+    preset_morph(255);
+    while(morph_state) preset_morph_tick();
+    check_sweep();
+
+    preset_sendDrumsetParametersImmediate();
+    check_sweep();
+    assert(audio_calls == END_OF_SOUND_PARAMETERS);
+    puts("PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence");
+}
diff --git a/tests/bug001/trap_stubs.c b/tests/bug001/trap_stubs.c
new file mode 100644
index 0000000..300a072
--- /dev/null
+++ b/tests/bug001/trap_stubs.c
@@ -0,0 +1,48 @@
+// SPDX-License-Identifier: GPL-3.0-only
+// Unrelated firmware dependencies needed to link the complete production parser.
+// These deliberately have no production headers or ABI assumptions: they must
+// never be called. An unexpected path aborts instead of silently passing.
+#include <stdio.h>
+#include <stdlib.h>
+#define TRAP(name) void name(void) { fputs("Unexpected call: " #name "\n", stderr); abort(); }
+TRAP(Compressor_setRatio)
+TRAP(Cymbal_setPan)
+TRAP(DecayEg_setDecay)
+TRAP(DecayEg_setSlope)
+TRAP(FxBus_prepareFxChange)
+TRAP(FxBus_setParameter)
+TRAP(FxBus_updateParameter)
+TRAP(HiHat_setPan)
+TRAP(Ringmod_setWave)
+TRAP(SVF_directSetFilterValue)
+TRAP(SVF_setReso)
+TRAP(Snare_setPan)
+TRAP(lfo_setFreq)
+TRAP(lfo_setSync)
+TRAP(menu_getActiveVoice)
+TRAP(menu_getSubPage)
+TRAP(menu_getViewedPattern)
+TRAP(menu_repaint)
+TRAP(menu_switchSubPage)
+TRAP(osc_recalcFreq)
+TRAP(seq_addNote)
+TRAP(seq_getExtSync)
+TRAP(seq_isRunning)
+TRAP(seq_isTrackMuted)
+TRAP(seq_midiNoteOff)
+TRAP(seq_sendMidiNoteOn)
+TRAP(seq_setNextPattern)
+TRAP(seq_setRunning)
+TRAP(seq_sync)
+TRAP(setDistortionShape)
+TRAP(setPan)
+TRAP(slopeEg2_calcDecay)
+TRAP(slopeEg2_setAttack)
+TRAP(slopeEg2_setDecay)
+TRAP(slopeEg2_setSlope)
+TRAP(sync_midiResetPosition)
+TRAP(sync_midiStartStop)
+TRAP(transient_setWaveform)
+TRAP(uart_sendMidi)
+TRAP(usb_sendMidi)
+TRAP(voiceControl_noteOn)
```

## Appendix B — Independent image validator actually run

This temporary script was not added to the firmware branch; it records the exact validation performed and may be saved locally for reproduction.

```python
import hashlib
import json
from pathlib import Path
import struct

root = Path('/workspace/lxr-desktop-app')
metadata = {}
for preset in ('release', 'debug'):
    directory = root / 'build' / preset
    image = (directory / 'LXR_V2_V1.7.img').read_bytes()
    binary = (directory / 'LXR_V2_STM32F765.bin').read_bytes()
    assert len(image) >= 16
    magic, declared, checksum, padding = struct.unpack('<8sIB3s', image[:16])
    payload = image[16:]
    assert magic == b'LXRV2IMG', 'wrong desktop magic'
    assert padding == b'\0\0\0', 'nonzero header padding'
    assert declared == len(payload) == len(binary), 'declared payload length mismatch'
    assert len(image) == declared + 16, 'unexpected trailing or missing bytes'
    assert 0 < declared < (0x080FFFFF - 0x08008000), 'bootloader payload size limit'
    assert payload == binary, 'image payload differs from objcopy binary'
    assert checksum == 0xFF - (sum(payload) & 0xFF), 'bad checksum'
    assert (sum(payload) + checksum) & 0xFF == 0xFF
    assert not payload.startswith(b'\x7fELF'), 'ELF is not a raw firmware payload'
    msp, reset = struct.unpack('<II', payload[:8])
    assert 0x20000000 < msp <= 0x20080000 and msp % 8 == 0, 'invalid initial stack'
    assert reset & 1 and 0x08008000 <= (reset & ~1) < 0x08008000 + declared, 'invalid Thumb reset vector'
    metadata[preset] = {
        'image_bytes': len(image), 'header_bytes': 16, 'payload_bytes': declared,
        'magic': magic.decode(), 'checksum': f'0x{checksum:02X}',
        'initial_msp': f'0x{msp:08X}', 'reset_vector': f'0x{reset:08X}',
        'sha256': hashlib.sha256(image).hexdigest(),
        'binary_sha256': hashlib.sha256(binary).hexdigest(),
        'elf_sha256': hashlib.sha256((directory / 'LXR_V2_STM32F765.elf').read_bytes()).hexdigest(),
        'result': 'PASS',
    }
    print(json.dumps({'preset': preset, **metadata[preset]}, sort_keys=True))
Path('/tmp/bug001-publish-images.json').write_text(json.dumps(metadata, indent=2) + '\n')
```

## Appendix C — Complete fresh validation output

Generated flashing instructions in build output are not evidence of execution; no flashing occurred.

### Regression suite

```text
PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds
PASS: all 256 Morph positions, unchanged tick/restart behavior, immediate kit sweep and audio service cadence
```

### Release configure

```text
Preset CMake variables:

  CMAKE_BUILD_TYPE="Release"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/release
```

### Release clean build

```text
[1/2] Cleaning additional files...
[2/2] Cleaning all built files...
Cleaning... 124 files.
[1/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c.obj
[2/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c.obj
[3/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c.obj
[4/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c.obj
[5/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c.obj
[6/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c.obj
[7/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c.obj
[8/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c.obj
[9/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c.obj
[10/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c.obj
[11/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c.obj
[12/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c.obj
[13/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c.obj
[14/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c.obj
[15/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c.obj
[16/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c.obj
[17/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c.obj
[18/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c.obj
[19/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c.obj
[20/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c.obj
[21/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj
[22/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c.obj
[23/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c.obj
[24/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c.obj
[25/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c.obj
[26/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c.obj
[27/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c.obj
[28/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c.obj
[29/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[30/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj
[31/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff_gen_drv.c.obj
[32/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/option/syscall.c.obj
[33/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/App/fatfs.c.obj
[34/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[35/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/fatfs_platform.c.obj
[36/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[37/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[38/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[39/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[40/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[41/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[42/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usb_device.c.obj
[43/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_cdc_if.c.obj
[44/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_desc.c.obj
[45/122] Building ASM object Core/CMakeFiles/lxr_platform.dir/Startup/startup_stm32f765vgtx.s.obj
[46/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[47/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/Target/usbd_conf.c.obj
[48/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[49/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/system_stm32f7xx.c.obj
[50/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/syscalls.c.obj
[51/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/sysmem.c.obj
[52/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_hal_msp.c.obj
[53/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/delay.c.obj
[54/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/adcPots.c.obj
[55/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/flash_if.c.obj
[56/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/DIO.c.obj
[57/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/uart.c.obj
[58/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/encoder.c.obj
[59/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[60/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/lcd.c.obj
[61/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/ledHandler.c.obj
[62/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/graphic.c.obj
[63/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/Parameters.c.obj
[64/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/Cc2Text.c.obj
[65/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/frontPanelParser.c.obj
[66/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/copyClearTools.c.obj
[67/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/screensaver.c.obj
[68/122] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/initKit.c.obj
[69/122] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[70/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/1PoleLp.c.obj
[71/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/automationNode.c.obj
[72/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[73/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/BufferTools.c.obj
[74/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/CymbalVoice.c.obj
[75/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/distortion.c.obj
[76/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Decay.c.obj
[77/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/dither.c.obj
[78/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/DrumVoice.c.obj
[79/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/HiHat.c.obj
[80/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/mixer.c.obj
[81/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/lfo.c.obj
[82/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/modulationNode.c.obj
[83/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/random.c.obj
[84/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/ResonantFilter.c.obj
[85/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Samples.c.obj
[86/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/SlopeEg2.c.obj
[87/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/snapEg.c.obj
[88/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Oscillator.c.obj
[89/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Snare.c.obj
[90/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/squareRootLut.c.obj
[91/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/TiltEq.c.obj
[92/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientTables.c.obj
[93/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientGenerator.c.obj
[94/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Compressor.c.obj
[95/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/wavetable.c.obj
[96/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Distortion.c.obj
[97/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Delay.c.obj
[98/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c.obj
[99/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Ringmod.c.obj
[100/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Reverb.c.obj
[101/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c.obj
[102/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c.obj
[103/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c.obj
[104/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c.obj
[105/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/AudioCodecManager.c.obj
[106/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FIFO.c.obj
[107/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[108/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FxBus.c.obj
[109/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[110/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/TriggerOut.c.obj
[111/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiMessages.c.obj
[112/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[113/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiVoiceControl.c.obj
[114/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/ParameterArray.c.obj
[115/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/Uart.c.obj
[116/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[117/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/clockSync.c.obj
[118/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[119/122] Building C object Core/CMakeFiles/lxr_usb_app.dir/Src/Soundengine/USB/usb_manager.c.obj
[120/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[121/122] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15008 B        16 KB     91.60%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      223128 B       992 KB     21.97%
   text	   data	    bss	    dec	    hex	filename
 274272	    852	 340576	 615700	  96514	/workspace/lxr-desktop-app/build/release/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/release/LXR_V2_V1.7.img: 275132 bytes, checksum 0x1B
[122/122] Firmware update instructions

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
```

### Debug configure

```text
Preset CMake variables:

  CMAKE_BUILD_TYPE="Debug"
  CMAKE_EXPORT_COMPILE_COMMANDS="ON"
  CMAKE_TOOLCHAIN_FILE:FILEPATH="/workspace/lxr-desktop-app/cmake/arm-none-eabi.cmake"

-- No bootloader configured: building the firmware only (no full-flash image, no flash_full). Set LXR_BOOTLOADER_HEX or LXR_BOOTLOADER_DIR for blank devices.
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: /workspace/lxr-desktop-app/build/debug
```

### Debug clean build

```text
[1/2] Cleaning additional files...
[2/2] Cleaning all built files...
Cleaning... 124 files.
[1/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c.obj
[2/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc.c.obj
[3/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_adc_ex.c.obj
[4/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c.obj
[5/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c.obj
[6/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma_ex.c.obj
[7/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_exti.c.obj
[8/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c.obj
[9/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c.obj
[10/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c.obj
[11/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c_ex.c.obj
[12/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2s.c.obj
[13/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd.c.obj
[14/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_i2c.c.obj
[15/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pcd_ex.c.obj
[16/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c.obj
[17/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c.obj
[18/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c.obj
[19/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c.obj
[20/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rng.c.obj
[21/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c.obj
[22/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi_ex.c.obj
[23/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_spi.c.obj
[24/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c.obj
[25/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c.obj
[26/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c.obj
[27/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c.obj
[28/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c.obj
[29/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/diskio.c.obj
[30/122] Building C object Drivers/CMakeFiles/stm32f7_hal.dir/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_usb.c.obj
[31/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff_gen_drv.c.obj
[32/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/option/syscall.c.obj
[33/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/App/fatfs.c.obj
[34/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/bsp_driver_sd.c.obj
[35/122] Building C object Middlewares/CMakeFiles/fatfs.dir/Third_Party/FatFs/src/ff.c.obj
[36/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/fatfs_platform.c.obj
[37/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c.obj
[38/122] Building C object Middlewares/CMakeFiles/fatfs.dir/__/FATFS/Target/sd_diskio.c.obj
[39/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c.obj
[40/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c.obj
[41/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c.obj
[42/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usb_device.c.obj
[43/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_cdc_if.c.obj
[44/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/App/usbd_desc.c.obj
[45/122] Building ASM object Core/CMakeFiles/lxr_platform.dir/Startup/startup_stm32f765vgtx.s.obj
[46/122] Building C object Middlewares/CMakeFiles/usb_midi.dir/__/USB_DEVICE_MIDI/Target/usbd_conf.c.obj
[47/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/main.c.obj
[48/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_it.c.obj
[49/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/stm32f7xx_hal_msp.c.obj
[50/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/syscalls.c.obj
[51/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/sysmem.c.obj
[52/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/system_stm32f7xx.c.obj
[53/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/delay.c.obj
[54/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/adcPots.c.obj
[55/122] Building C object Core/CMakeFiles/lxr_platform.dir/Src/flash_if.c.obj
[56/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/DIO.c.obj
[57/122] Building C object Core/CMakeFiles/lxr_io.dir/Src/IO/uart.c.obj
[58/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/encoder.c.obj
[59/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/lcd.c.obj
[60/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/buttonHandler.c.obj
[61/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/ledHandler.c.obj
[62/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/graphic.c.obj
[63/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/frontPanelParser.c.obj
[64/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/Parameters.c.obj
[65/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/Cc2Text.c.obj
[66/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/copyClearTools.c.obj
[67/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/screensaver.c.obj
[68/122] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/initKit.c.obj
[69/122] Building C object Core/CMakeFiles/lxr_ui.dir/Src/menu/menu.c.obj
[70/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/1PoleLp.c.obj
[71/122] Building C object Core/CMakeFiles/lxr_preset.dir/Src/Preset/presetManager.c.obj
[72/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/BufferTools.c.obj
[73/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/automationNode.c.obj
[74/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Decay.c.obj
[75/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/CymbalVoice.c.obj
[76/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/distortion.c.obj
[77/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/dither.c.obj
[78/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/DrumVoice.c.obj
[79/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/HiHat.c.obj
[80/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/lfo.c.obj
[81/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/mixer.c.obj
[82/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/modulationNode.c.obj
[83/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/random.c.obj
[84/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Oscillator.c.obj
[85/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Samples.c.obj
[86/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/ResonantFilter.c.obj
[87/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/SlopeEg2.c.obj
[88/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/snapEg.c.obj
[89/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/Snare.c.obj
[90/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/squareRootLut.c.obj
[91/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/TiltEq.c.obj
[92/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientGenerator.c.obj
[93/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/transientTables.c.obj
[94/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/wavetable.c.obj
[95/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Compressor.c.obj
[96/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Delay.c.obj
[97/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Distortion.c.obj
[98/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/FxRamBuffer.c.obj
[99/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Reverb.c.obj
[100/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/Ringmod.c.obj
[101/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c.obj
[102/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c.obj
[103/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c.obj
[104/122] Building C object Core/CMakeFiles/lxr_dsp.dir/Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c.obj
[105/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/AudioCodecManager.c.obj
[106/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/cs4344_cs5343.c.obj
[107/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FIFO.c.obj
[108/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/FxBus.c.obj
[109/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/Soundengine.c.obj
[110/122] Building C object Core/CMakeFiles/lxr_engine.dir/Src/Soundengine/TriggerOut.c.obj
[111/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiMessages.c.obj
[112/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/CortexParser.c.obj
[113/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiVoiceControl.c.obj
[114/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/MidiParser.c.obj
[115/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/Uart.c.obj
[116/122] Building C object Core/CMakeFiles/lxr_midi.dir/Src/Soundengine/MIDI/ParameterArray.c.obj
[117/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/clockSync.c.obj
[118/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/EuklidGenerator.c.obj
[119/122] Building C object Core/CMakeFiles/lxr_usb_app.dir/Src/Soundengine/USB/usb_manager.c.obj
[120/122] Building C object Core/CMakeFiles/lxr_sequencer.dir/Src/Soundengine/Sequencer/sequencer.c.obj
[121/122] Linking C executable LXR_V2_STM32F765.elf
Memory region         Used Size  Region Size  %age Used
        ITCM_RAM:       15008 B        16 KB     91.60%
            DTCM:      107392 B       128 KB     81.93%
             RAM:      271040 B       384 KB     68.93%
           FLASH:      223176 B       992 KB     21.97%
   text	   data	    bss	    dec	    hex	filename
 274320	    852	 340576	 615748	  96544	/workspace/lxr-desktop-app/build/debug/LXR_V2_STM32F765.elf
/workspace/lxr-desktop-app/build/debug/LXR_V2_V1.7.img: 275180 bytes, checksum 0xC5
[122/122] Firmware update instructions

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
```

### Independent image validation

```text
{"binary_sha256": "7d4505da7bd59887d0ca690e4464a32099cda4cc3045c34094ee60ae33070553", "checksum": "0x1B", "elf_sha256": "5f09d248ca6626ac7dd032204b04c26d8b135c5527f9f19bf47f7190aa40a5fa", "header_bytes": 16, "image_bytes": 275148, "initial_msp": "0x20080000", "magic": "LXRV2IMG", "payload_bytes": 275132, "preset": "release", "reset_vector": "0x08014E5D", "result": "PASS", "sha256": "65441a47a0352d2ba69d814b044c0d014cd76fe9cba130552223489c0e028d7d"}
{"binary_sha256": "2f91fd5b2225584b3ca08479a5319494f3c8ff1f1d127317760e47d998040c27", "checksum": "0xC5", "elf_sha256": "1adbd7d75124c337fd6d6adfb9d034683506853f2e78583ec08b2556ec9f86f9", "header_bytes": 16, "image_bytes": 275196, "initial_msp": "0x20080000", "magic": "LXRV2IMG", "payload_bytes": 275180, "preset": "debug", "reset_vector": "0x08014E5D", "result": "PASS", "sha256": "418c0510dcf93bfcc7a5fe1832a52f3e930d9611e878b40c01bbbd9c944596ce"}
```

## Appendix D — Exact published file fingerprints

| Path | Git blob SHA-1 | SHA-256 |
| --- | --- | --- |
| `Core/Src/Preset/presetManager.c` | `b13a5360f31bfb94e4e5178fc788a725906d9eeb` | `ac0901a2a422473de1e175807d1f808ad1878f99ede345e1c40926386e89419c` |
| `Core/Src/Soundengine/MIDI/MidiParser.c` | `d119e9f4bcb90cc4934d1e1d3e7288602747f727` | `076c8129e7d159f5e80f70584e78c2c745e15c457907539df691fb0d5ed04fab` |
| `tests/bug001/README.md` | `8f22d91bf60f746098c42064425c8eea00478c72` | `9ec9584e570db0a4f43d82e9959c021cba4fb2ccff225273092f065efb7ea403` |
| `tests/bug001/run.py` | `d91117d598e8caac1d975ef7f926e1da05911348` | `3fa50d94e04cbf44c82146831c76f50e4d14a9f598cf899f61c0b586385c7e8e` |
| `tests/bug001/test_parser.c` | `3ca4dbd51194d72ae25e9988eb3a0b894073faeb` | `df7c35a41140afa0d8dee2ba0b73a1b3d3c25f6dfb693f9bbdeba5418582f062` |
| `tests/bug001/test_sweeps.c` | `b06a0040b7c6c741230643943fb782b21f588afc` | `f086d64b13af4c58b188d9840bbf71ffc6aaf3ef3f502c1b00cd7fba3dc08800` |
| `tests/bug001/trap_stubs.c` | `300a072791092b57fe64556ebbec64cc70bf6bba` | `ef8d444b079087949190ee3baa1966d032672ddb385eb6019673b8e41ae7f78d` |

All seven remote blob IDs and complete UTF-8 contents were read back and matched these validated local files. Production CRLF bytes were retained.

---

End of Bug 001 validated correction publication handoff. The firmware commit is published and verified; no PR, merge, hardware flash, generated artifact publication or unrelated project change was performed.
