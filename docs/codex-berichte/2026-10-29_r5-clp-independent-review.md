# Independent review of the existing R5 CLP test candidate

Date: 2026-10-29 (UTC). Repository: https://github.com/DocLXR-02/lxr-desktop-app.
Task: independent source and evidence review; no new candidate.
Decision: **ready for a separately authorized, controlled hardware evaluation**, with the limitations and report corrections below. This is not approval to install it, release it, or treat it as hardware validated.

## Scope and verified identity

Read the repository root AGENTS.md, the existing [implementation report](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r5-clp-test-build/docs/codex-berichte/2026-10-29_r5-clp-test-build.md), both commit changes, DSP/bus/preset/MIDI scheduling paths, host fixtures, workflow and downloaded CI evidence. No previous conversation was assumed. The original report describes implementation work produced through ChatGPT orchestration; it is not a separate independent review. Its earlier authorization statements do not authorize corrections in this task.

Remote heads resolved through GitHub on this review date:

| Ref | Commit |
| --- | --- |
| main | a43cd4fcda23ec3d577dc425261c3eb28bd5abff |
| lab-setup (new report branch base) | 5829398cea5d337200393757e39a37d4d8148256 |
| ui/kit-load-confirmation (R4) | c870f7249d7ee0d65423bc8b7bfdb422dcc74482 |
| fx/clp-input-silence (R5) | af3562fece090288a83727524c1035b79f4cf20b |

GitHub comparison reports ahead_by=2, behind_by=0, merge base exactly R4. Git commit objects establish the linear parent chain:
R4 → 792087d6cba31ae20d64fb63c1e165f8cb5ae6f4 (diode endpoint only) → af3562fece090288a83727524c1035b79f4cf20b (detector, integration, splash, tests and workflow).
R5 tree is 316127985571e2545a4cafb0954f9c7e9c63b53d. No branch/pin/two-commit discrepancy was found.

Source references below are to **af3562fece090288a83727524c1035b79f4cf20b**, unless explicitly stated otherwise; paths and line numbers are precise references into that immutable tree.

## Findings and necessary report corrections, ordered by severity

### 1. Medium: preservation during musical playing remains substantially unproven

Source: Core/Src/Soundengine/DSPAudio/FX/Distortion.c:181–198, 480–482, 619–620.
Tests: tests/fx_clip/render.c:11–24; tests/fx_clip/run.py:25–41; tests/fx_clip/unit.c:18–33.

The detector restores full return gain on any consumed sample outside [-1,1]. It does not gate wet output. Therefore sufficiently quiet but nonzero input can lose the feedback contribution after the hold, while a single sample of magnitude 2 can reopen it immediately. Sparse pulses less than roughly 10 ms apart can keep full feedback continuously; longer gaps can fade and reopen it repeatedly. Threshold chatter and sudden restoration of residual loop energy are possible audible behaviors, not demonstrated clicks.

The 72 renders are 12 control rows × 3 scenarios × 2 channel modes, not 72 musical kits. Only 24 continuous-input renders are compared sample-for-sample with the bounded R4 oracle. Stereo render channels are identical. The decay scenario is forcibly cut to zero at 250 ms; it does not reach its natural quantized end. There is no rendered input/silence/restart sequence, fader ramp, sparse sequence, independent stereo content, or full instrument/CLP-1 voice render. A unit test checks reopening gain, not preservation of the resumed attack or intermodulation spectrum.

Correction: retain the narrow continuous-synthetic-input equality claim; do not generalize it to preserving all playing/tails/attacks. The previous report's returning-kick attack observation has no quantitative supporting result in the delivered JSON or committed render suite; treat it as an uncorroborated earlier observation, not independent evidence.

This is a material evidence gap for product acceptance, but an appropriate question for a controlled experimental hardware evaluation. No new host test or implementation was added here.

### 2. Medium: reset is a request at resend start, not an atomic completed-recall history boundary

Source: Core/Src/Preset/presetManager.c:1164–1253, 1257–1292; Core/Src/Soundengine/FxBus.c:140–145; Distortion.c:157–178, 451.
Tests: tests/fx_clip/fxbus.c:16–29 and the no-op callbacks added to tests/bug001/test_sweeps.c and tests/project_storage/host.c.

Both resend functions request reset before sending parameters. The immediate resend explicitly calls soundengine_process() between parameter updates. TIM2 can also process audio while foreground recall proceeds. Thus CLP may consume the request using old or partially updated settings and current nonzero input, then build history again before recall finishes. A same-type recall does not promise a second reset at completion. Loading a morph destination also reaches the resend functions (presetManager.c:809–838), broadening the audible reset scope beyond replacing the active kit alone.

Correction: say “requests a reset at the beginning of full parameter resends”; do not imply atomic recall, guaranteed quiet state at completion, or unchanged transition sound. Clearing lp3/lastOut and all CLP filter states at entry/reset can abruptly remove an existing wet signal. No crossfade bounds this discontinuity. The bus fixture calls prepare directly between blocks; storage/menu tests mock it and cannot establish actual recall audio behavior.

This is a confirmed sequencing limitation, not evidence of memory corruption. Before normal live use, transition behavior needs explicit acceptance. Any proposal to move/coalesce resets or change synchronization requires a subsequent authorized task.

### 3. Medium: inactivity policy cannot guarantee silence merely because one voice fader is zero

Source: Distortion.c:183–198; Core/Src/Soundengine/DSPAudio/mixer.c:591–599, 607–712.

Detection sees the quantized FX input before distortion, not the voice fader position or downstream wet output. Once all consumed input stays within ±1 LSB, it closes feedback independently of the old oscillator, which directly addresses the intended mechanism. However, another routed voice or residual samples >1 LSB can keep it open. In stereo either channel holds both paths open, allowing an idle side's oscillator to persist while the other side plays. Mono deliberately observes only the interleaved channel consumed by the existing mono algorithm.

Correction: describe the fader-zero result as conditional on actual bus inactivity. It is not yet demonstrated with the complete voice/mixer/fader chain. Linked stereo is a deliberate policy requiring musical evaluation. Routing is unchanged; the existing mono/right-input issue is not corrected by R5.

### 4. Low: exact fade duration is overstated by one sample

Source: Distortion.c:154–155, 189–196; tests/fx_clip/unit.c:20–23.

The first 441 inactive frames retain the current gain; fading starts on inactive frame 442. Float32 repeated subtraction of 1.f/221.f leaves approximately 5.86733222e-8 after 221 subtractions; subtraction 222 clamps to zero. The unit test actually performs 222 fading steps (one before the loop plus 221 inside), so it does not prove exact zero after 221.

Correction: call this a nominal ~5 ms fade, reaching exact zero after 222 steps under the checked float32 arithmetic, i.e. 663 inactive frames total from full gain (~15.02 ms at 44140 Hz). This negligible timing difference is not a reason alone to replace the candidate. Wet filters continue decaying after return gain closes; it is not a 15 ms wet-output mute.

### 5. Low: historical baseline and authorization language need explicit correction

The supplied, hardware-confirmed R4 observation is:
at drv=63, ton=127, vol=127, increasing col starts oscillation at 7; once running it continues at 6 and stops at 5. These are distinct start/continuation/stop conditions, not interchangeable thresholds.

The row {63,7,127,127} in render.c:11 starts from cleared state, with synthetic input. There is no 7→6→5 history sweep at these same settings. A different row containing col=6 uses drv=127 and ton=0. Existing evidence therefore does not independently reproduce or refute the R4 hysteresis. R5 intentionally removes loop history after sufficiently long inactivity, so it changes this aspect of no-input behavior by design.

The original manual's intentional no-input self-oscillation is task-supplied context; the manual itself was not independently retrieved in this review. Suppression is a deliberate live-use improvement. The diode out-of-bounds read must not be presented as proof that intentional self-oscillation was accidental.

The old report's final sentence claiming no additional permission is needed for routine fixes does not apply here. This task authorizes only the review/report; production, test and workflow corrections require a later authorized task.

## Independent code assessment

### Diode endpoint

Commit 792087d6cba31ae20d64fb63c1e165f8cb5ae6f4 changes only Distortion.c. The 257-element table has indices 0…256 (line 18). After finite input is clamped to [-1,1], the computed index can equal 256; interpolation previously evaluated table[257] even with zero fraction. Lines 141–146 now return table[256] first. Negative endpoint still interpolates valid elements 0 and 1; interior finite indices retain the prior calculation. This is a valid, narrowly scoped bound correction.

The tests assert the positive and clamped-high endpoint and finite outputs over 10,000 finite values with bounds/UB sanitizers. They do not establish NaN handling or all float bit patterns. The DSP clamps slew states, so no new NaN policy is required for this fix. The oracle is explicitly R4 source with this same bound added (run.py:14–17): useful for isolating detector changes, but not byte identity or sonic equivalence to the hardware-confirmed R4 binary.

### Detector, state ownership and transitions

The new fields are instance-owned float/uint16/volatile byte runtime state (Core/Inc/Soundengine/DSPAudio/FX/Distortion.h:49–52), not kit fields. Counter saturation at 441 prevents overflow. Cold entry starts gain zero; input is still processed, and the first usable input opens return immediately. Normal inactivity only scales loop return; it does not clear filters. Entry/reset clears hp1/2/3, lp1/2/3, slew and lastOut in audio processing, leaving coefficients and controls intact.

FxBus.c:43–44 invalidates CLP state while other bus effects run. Distortion.c:267 invalidates state for Tube/Fold without clearing their running states; CLP entry clears the shared distortion histories. With USE_REVERB=0 (FxBus.h:26), the existing MIDI effect-change path calls prepare on bus type changes (MidiParser.c:1372–1380). Distortion subtype changes are simple assignments (lines 1396–1398); processing an intervening Tube/Fold block triggers invalidation. If subtype goes away and back between audio blocks, the audio path never observes the excursion and there is no such reset. Ordinary drive/colour/tone/volume and routing updates do not explicitly reset CLP.

Reset callbacks write only clipStateActive=0, avoiding foreground writes to partially processed double-precision history. TIM2 runs seq_tick, sample-block calculation, then UART parsing (stm32f7xx_it.c:268–275). Foreground soundengine_process disables TIM2 while calculating (Soundengine.c:81–108). This supports DSP ownership for the reviewed configured paths; there is no new lock, allocation, logging or storage work in the sample loop. Volatile is visibility, not a general queued-event protocol: arbitrary future higher-priority reset callers could require stronger coordination. No confirmed new lost-reset race was established in the reviewed current call paths; interrupt interleavings were not executed by host fixtures.

The exact R4→R5 diff leaves routing math, mixer, voice synthesis, parameter IDs, storage layouts and DSP coefficients unchanged. It adds reset requests to preset resends, not serialized bytes. No firmware format migration is indicated. Existing routing defects and shared Tube/CLP filter history remain baseline constraints. The changed splash visibly distinguishes R5; it is not a separate bootloader.

## Host and CI evidence

[Run 40910378774](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/40910378774), job 123576420322, reports success for all steps. Artifact metadata ties it to head af3562fece090288a83727524c1035b79f4cf20b, branch fx/clp-input-silence. Decoded job logs confirm checkout SHA, actual test commands/pass output, same-job R4 build and R5 build, image validation and upload.

Workflow .github/workflows/clp-test-build.yml:1–63 checks out source, runs fx_clip, bug001, kit_load, project_storage and euclidean suites, builds pinned R4 in a detached worktree and builds R5 Release with Arm GNU 14.3.Rel1 (14.3.1 20250623). The downloaded toolchain archive is version-named but not checksum-pinned in the workflow, and actions use major tags. These limit reproducibility/supply-chain attestation; they are not observed source mismatches.

The committed tests compile actual DSP and bus code with native CMSIS shims and UBSan/bounds. They provide useful evidence for finite lookup bounds, cold silence, instance state, gross hold/fade behavior, consumed-channel detection, sequential reset dispatch and block partition invariance. They share the implementation's ±1-LSB and timing assumptions; those assertions do not justify those choices musically. Only block sizes 1 and 64 are compared. All renders use dryWet=.4, and other effects in the bus fixture are stubs. The all-six-routes fixture verifies activity/reset and silent output, not asymmetric channel routing fidelity.

The delivered JSON reports max_settle_ms=24.830086… for its finite synthetic cases. run.py:35 asserts quantized zero after cut+50 ms; lines 36–38 measure the last >32-LSB sample and require <35 ms. This is not a universal acoustic decay deadline. No spectral intermodulation metric, click limit, device cycles, stack high-water, underrun measurement, or listening result is supplied.

Regression log passes include CC/NRPN bounds/mutes, morph behavior, 512 RAM load/menu cases, storage/FatFs groups and six Euclidean groups (2176 masks). They do not substitute for a full audio integration test.

No existing host suite was rerun: source inspection and delivered logs resolved what the assertions prove, and rerunning unchanged tests would not fill their missing scenarios. No firmware build was repeated.

## Artifact and memory verification

Downloaded artifact 12912683549, lxr-lab-r5-clp-test, was available and unexpired at review. Independently recomputed:

| Item | Bytes | SHA-256 |
| --- | ---: | --- |
| Artifact ZIP | 2127249 | 10123f1193d7729f7091fb449a8d4de45ceebdd758e3313380be33faf8c2160c |
| SOURCE.zip | 1691979 | 12b6262082f29cfd39d727a11cbf25cc13513b5a2fb03bd8e1d020bf2f22234c |
| LXR_LAB_R5_CLP_TEST.img | 286880 | 90caf0cc55203a828f02a108e0bd62f4ac8396c94e823e9680ee433991284a3f |

Both ZIPs extracted successfully; SOURCE.zip integrity check passed. SOURCE_COMMIT.txt identifies pinned R5. GitHub's complete recursive tree was not truncated: all 382 source archive file contents matched their Git blob hashes in pinned tree 316127985571e2545a4cafb0954f9c7e9c63b53d. This verifies the archived source contents independently of the prior report.

Ran the existing image validator solely to verify delivered container structure: magic, payload size, checksum, reserved zeroes, splash and initial vectors passed. Additionally parsed ELF32 program headers independently and compared image payload bytes at each physical flash offset with every nonempty LOAD segment. All matched, and all lay within application flash:
0x08008000…0x080411b8, 0x080411b8…0x0804150c, 0x0804150c…0x0804a590, 0x0804a590…0x0804e090 (exclusive ends).
This verifies ELF/image consistency and no load bytes in bootloader flash. It does not prove source-to-binary reproducibility through an independent rebuild or that hardware will accept the update.

CI linker logs agree with delivered section reports and the prior report:

| Region | R4 rebuild | R5 | Delta | R5 free |
| --- | ---: | ---: | ---: | ---: |
| ITCM | 15016 | 15104 | +88 | 1280 |
| DTCM | 107392 | 107400 | +8 | 23672 |
| RAM | 274648 | 274648 | 0 | 118568 |
| Linker-reported FLASH | 234100 | 234764 | +664 | 781044 |

DTCM includes alignment: .dtcm=36996 and .dtcmz=70392→70400. RAM includes alignment/heap reservation beyond .data/.bss. FLASH reporting excludes separately loaded ITCM/DTCM data; program-header inspection above covers that limitation. The R4 ELF was not delivered, so its sizes are corroborated by section text and CI log, not independently read from its ELF here. The prior flashed R4 image hash/size and external backup availability were not reverified; the same-toolchain R4 rebuild is not that binary.

## Actual checks, commands and limitations

Evidence workspace: /workspace/r5-evidence; extracted source: /workspace/r5-source. Original checkout was clean on local work, containing only origin/main. No source files there were changed.

Commands/checks actually used, with outcomes:

- `git status --short`, `git remote -v`, `git branch -a`: clean local work, origin URL correct.
- `git fetch origin main lab-setup ui/kit-load-confirmation fx/clp-input-silence codex-reports/2026-10-29-r5-clp-test-build`: failed before fetching, “Failed to connect to proxy port 8080”. No attempt to bypass network policy. GitHub connector GETs supplied branch commits, comparison, both Git commit parent objects, recursive tree, report, jobs, logs and artifacts instead.
- `unzip -q /workspace/attachments/1238c011-f981-484a-9e43-23292abec0b3/github-actions-artifact-12912683549.zip -d /workspace/r5-evidence`
- `unzip -q /workspace/r5-evidence/SOURCE.zip -d /workspace/r5-source`
- `sha256sum /workspace/attachments/1238c011-f981-484a-9e43-23292abec0b3/github-actions-artifact-12912683549.zip /workspace/r5-evidence/SOURCE.zip /workspace/r5-evidence/LXR_LAB_R5_CLP_TEST.img`: hashes above.
- `unzip -t /workspace/r5-evidence/SOURCE.zip`: no errors.
- From extracted source, `python3 tests/fx_clip/check_image.py /workspace/r5-evidence/LXR_LAB_R5_CLP_TEST.img`: PASS; reason was independent delivered-image validation, not a DSP-suite rerun.
- `readelf -l /workspace/r5-evidence/LXR_V2_STM32F765.elf`: seven headers, four nonempty LOAD segments as listed.
- Inline Python audit: for each GitHub tree blob, read archive file and assert SHA1(b"blob "+decimal_length+b"\\0"+file_bytes) equals GitHub SHA: PASS 382 files.
- Inline Python ELF audit: unpack ELF header offset 28, program header entry size/count at 42; for each type=1/filesz>0 header unpacked as <8I, assert physical range inside [0x08008000,0x08100000) and image[16+paddr-0x08008000 : 16+paddr-0x08008000+filesz] equals ELF[offset:offset+filesz]: PASS all four.
- Inline Python float32 audit using struct.pack/unpack('f') at every subtraction: gain at steps 220/221/222 = .0045249457471 / 5.86733222e-8 / 0. This resolves the exact-fade claim without changing or adding test code.
- Source inspection used rg, nl and sed for the referenced files. Some exploratory rg globs named nonexistent directories; these returned path errors and were replaced by searches of actual Core/Src paths, not treated as evidence of missing functionality.

No flashing, hardware test, full rebuild, production/test/workflow edit, merge, release, force push or setting change occurred. The only repository publication is this Markdown report on a new branch based on lab-setup.

## Recommendation and handoff

No confirmed memory-safety regression or artifact mismatch was found that blocks controlled evaluation of the existing experimental candidate. The diode fix is sound for the finite DSP domain; input-dependent loop reduction plausibly suppresses the sustained no-input tone while keeping feedback active during sufficiently strong, continuous playing. That plausibility and host evidence do not establish musical success.

The recommendation is readiness for a **separately authorized hardware evaluation**, not normal use or hardware validation. The report corrections above define the limits of that recommendation. Physical backup availability and a recovery plan still need to be established in that separate authorization; previous reported archive checks do not establish them here.

Proposed evaluation questions only, not instructions to conduct tests now: actual CLP-1/fader-zero bus silence, quiet natural tails and sparse playing, restart transients, useful distortion/intermodulation during performance, independent stereo activity, same-type active/morph kit recalls, other-effect transitions, and real-time headroom. Preserve the R4 7-start/6-continue/5-stop distinction in any comparison record.

Any implementation or test correction is deferred to a subsequent authorized task. R4 remains the hardware-confirmed baseline. R5 has never been tested on the instrument.

Report publication branch: codex-reports/2026-10-29-r5-clp-independent-review.
Report base: 5829398cea5d337200393757e39a37d4d8148256.
Report head: the report-only commit containing this file. No source candidate is included on this report branch.
