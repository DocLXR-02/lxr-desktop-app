# R11 continuation: all-six-voice MIX/VOL/headroom audit

Date: 2026-10-31 UTC. Repository: https://github.com/DocLXR-02/lxr-desktop-app.

**BLOCKED after the feasible all-six audit. No R11 firmware was implemented, built or packaged.** The authorized snare clamp addresses the previously reported conversion problem in the temporary reference. Five additional prerequisite classes remain: wavetable boundary reads, nonfinite filter arithmetic, negative FM-to-unsigned conversion, PWM phase conversion overflow and signed transient shifts. They reproduce in unchanged R10 and remain in the proposed full-source path. No unrelated fix, blanket saturation, normalization, source trim or voice exclusion was added.

R10 remains the hardware/listening comparison reference. The owner reports plausible/playable voice additions/removals; that is listening evidence, not comprehensive validation. R11 is an experimental solo diagnostic reference, not independent multi-voice wet mixing. Nothing is frozen. No installation or hardware testing occurred.

## Identity and source status

| Item | Exact status |
| --- | --- |
| Requested source base | **8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7** |
| Existing source branch | **fx/clp-solo-post-mix-r11**, still at that base; **no implementation commit** |
| Preserved R10 branch | fx/clp-multivoice-max-r10, same 8082cc5 head |
| New report-only base | **5829398cea5d337200393757e39a37d4d8148256**, exact lab-setup commit |
| New report branch | **codex-reports/2026-10-31-r11-all-voices-headroom-and-post-mix** |
| CI / Release ARM build | **Not run**, prerequisites failed before candidate implementation |
| LXR_V2_R11.img / LXR_V2_R11.zip | **Not produced**; no firmware hashes, sizes or artifact URLs exist |
| R11 SOURCE.zip / ELF / map / build evidence artifact | **Not produced**; published native audit evidence is listed below |

Read AGENTS.md and the reports at the requested pins: [corrected R10, 57d4df9](https://github.com/DocLXR-02/lxr-desktop-app/blob/57d4df90658244935e297dcc81a5a7b927714330/docs/codex-berichte/2026-10-31_r10-startup-display-correction.md), [MIX/VOL design, ae39a8b](https://github.com/DocLXR-02/lxr-desktop-app/blob/ae39a8b5398da1ea36bcb0148d40c8ab8461d184/docs/codex-berichte/2026-10-31_clp-mix-vol-decoupling-design.md), and [blocked R11, b14e6c5](https://github.com/DocLXR-02/lxr-desktop-app/blob/b14e6c5777ac318cdcaa43d856ba48b713db1586/docs/codex-berichte/2026-10-31_r11-clp-solo-post-mix-test-build.md), including its reproduction. Those reports and reproduction remain unchanged. The new snare-clamp authorization supersedes only that earlier policy blocker.

Verified all **72 pre-existing remote heads** before work. The reserved R11 branch and R10 both matched the exact base. No completed follow-up branch/report was found in the remote inventory, local branches or workspace. All 72 heads are preserved; only this new report branch is published. The source checkout remained clean, with no production, existing test, workflow, UI, storage or serialization change. It still displays R10, and must not be installed or represented as an R11 candidate.

## Source paths and what was actually exercised

References use pinned 8082cc5. DSP paths below are under Core/Src/Soundengine/DSPAudio.

- **Voices 1–3:** complete DrumVoice.c initialization, trigger, async and sync paths. Oscillator/source mixing and transient saturation → filter → envelope interpolation → optional velocity → voice distortion → smoothed MIX × smoothed physical fader (239–310). MIX pole r=.01 advances once per frame at 305; physical pole is independent.
- **Snare:** complete Snare.c paths. Noise/filter and oscillator/transient sum → block MIX × envelope × optional velocity × physical pole → int16 → voice distortion (148–231). Both velocity branches execute.
- **Cymbal/hi-hat:** complete CymbalVoice.c and HiHat.c paths, including modulator generation, FM/noise dispatch, filter, transient addition, block volume, optional velocity, physical pole and final voice distortion. The existing source/transient additions use saturating halfword arithmetic. Percussion physical poles use r=.1; no MIX pole is invented.
- Actual transientGenerator.c/tables, wavetable.c, SlopeEg2.c, Decay.c, snapEg.c, 1PoleLp.c, lowercase distortion.c and ResonantFilter.c are compiled as production translation units. Actual BufferTools.h algorithms are used, rather than its empty CLP-test shim.
- Selected oscillator and frequency/note functions and midiParser_calcDetune are extracted **verbatim** from production, including SINE, TRI, noise, PWM and FM dispatch/interpolation. The actual production mixer_decimateBlock body is separately extracted for transition checks.

MIX is the resolved runtime `.vol`, m=value/127 for ordinary values. This differs from physical q=u², envelope/velocity, voice drive, CLP VOL and outer D/W. The audit holds actual `.vol` at each selected target rather than replacing it with one globally. A temporary exported-copy full-source expression uses m>0 ? 1 : 0 at the existing MIX separation points. Drum MIX smoothers still advance once per frame; their result is bypassed only for the temporary excitation calculation. The snare temporary path adds the authorized finite-checked clamp before its int16 conversion in both branches. These copies exist outside the source checkout and are **audit fixtures, not an implemented routing-eligible firmware candidate**.

Hardware/control shims replace HAL/main, ADC reads, RNG, LFO/modulation services and unused sample playback. RNG is a deterministic 32-bit LCG, so the generated noise is synthetic but the production noise oscillator/filter processing is real. Modulation callbacks are no-ops. Actual voice/header layouts compile for the native host; this does not establish target ABI, interrupt or scheduling behavior. Unsupported sample playback asserts if selected. REC/SAW-specific sounds, sample playback, full modulation/morph/kit scheduling, every filter type and every parameter combination are not covered. Selected configurations avoid a zero operand to the host CLZ helper; no claim is made for untested low-frequency wavetable selection. No oscillator/filter/envelope buffers are replaced by idealized input-times-volume fixtures.

## Audit matrix: all 128 MIX values, selected configurations

Each configuration is swept over **0–127**, all six voices, both velocity modes, original R10 and temporary full-source/snare-clamped paths: **39,936 isolated native runs**. Every run has a fresh initial state. Physical fader and drum MIX poles are seeded to their settled target for the static matrix. Normally each run processes 128 × 64 frames (8192 frames, ~185.6 ms); C1/C7 intentionally use one 64-frame peak block to isolate conversion/filter findings before later sample-table failures.

Common setup: valid SINE/modulator controls unless listed, pitch-envelope amount zero, coarse/fine 70/64, fast attack 0 and decay 100, matched pan-independent mono voice generation. Voice DRIVE is 0, 100 or 127 as listed. No CLP effect is applied in this prerequisite matrix.

| ID | Selected source/control configuration | Temporary full-source result by voice 1 / 2 / 3 / 4 / 5 / 6 |
| --- | --- | --- |
| C0 | SINE, no FM/transient, filter OFF, note36, full fader, drive0 | P / P / P / P / P / P |
| C1 | Exact blocked snare source controls: note60, transient waveform2/vol127/frequency25, filter OFF, full fader, drive0; first block | P / P / P / P* / P / P |
| C2 | Note60, SINE FM with modulator gain .5, filter OFF, full fader, drive100; snare remains its non-FM tonal source | F / F / F / P / F / F |
| C3 | TRI, note96, zero FM excitation, no transient, filter OFF, drive127 | T / T / T / T / T / T |
| C4 | NOISE source, LP cutoff setter .35/resonance .3/filter drive .4, note60, voice drive127 | P / P / P / P / P / P |
| C5 | Transient waveform3/vol127, SINE, LP ordinary settings, note60, full fader | X / X / X / X / X / X |
| C6 | SINE, LP ordinary settings, note96, physical fader zero | P / P / P / P / P / P |
| C7 | SINE + transient2/vol127, LP cutoff setter .9/resonance1/filter drive1, note36, voice drive127; first block | N / N / N / P* / P / P |
| C8 | SINE, LP ordinary settings, physical u=.1 (q=.01), note60, velocity32 | P / P / P / P / P / P |
| C9 | SINE, filter OFF, no transient/FM, note60, voice drive127 | P / P / P / P / P / P |
| C10 | PWM, PWM=1, zero FM excitation, note60, filter OFF, full fader | U / U / U / U / U / U |
| C11 | SINE, no transient, LP cutoff from actual valueShaperF2F(126/127,−.9), resonance1/filter drive1, note60 | P / P / P / N / P / P |
| C12 | SINE, filter OFF, waveform2 selected with **transient volume0**, note60 | X / X / X / X / X / X |

P means the selected static sweep passes sanitizers/guards, not universal safety or musical success. * means original R10 has the snare MIX threshold below, while the temporary clamp passes. F=negative FM-to-unsigned conversion; T=1024-row interpolation boundary; X=signed transient shift; N=nonfinite filter conversion; U=PWM unsigned phase range. All listed failures occur across all 128 MIX targets in both velocity modes except the two explicit R10 snare thresholds. Velocity is 127 except C8’s 32, so C8 also distinguishes enabled/disabled gain behavior.

| Voice | Passing runs | Failing runs |
| --- | ---: | ---: |
| Drum 1 | 3584 | 3072 |
| Drum 2 | 3584 | 3072 |
| Drum 3 | 3584 | 3072 |
| Snare 4 | 3974 | 2682 |
| Cymbal 5 | 4096 | 2560 |
| Hi-hat 6 | 4096 | 2560 |
| **Total** | **22918** | **17018** |

Each voice has 6656 runs; identical configured DrumVoice instances are exercised separately. The temporary full-source path passes 11520 and fails 8448; unchanged R10 passes 11398 and fails 8570. Expected original failures are retained as failures, isolated per process so they cannot terminate the rest of the matrix. No sanitizer was disabled to continue.

[Compact group summary](repro/2026-10-31_r11-all-voices/audit-summary.csv) covers every voice/configuration/mode/path and failed MIX interval. [Full numerical sweep, gzip CSV](repro/2026-10-31_r11-all-voices/all-mix-values.csv.gz) contains all 39936 outcomes, successful metrics and first-failure diagnostics. Output paths are normalized to AUDIT_OUTPUT in that CSV. This is audit data, not a firmware artifact. Nonrecovering sanitizer termination means downstream stages after a failure were not examined in that run; other configurations isolate the additional findings.

## Findings and dispositions

### 1. Blocking: wavetable interpolation reads beyond a 1024-sample row

Oscillator.c:450–464 increments index 1023 and then reads table[row][1024]. The FM variant does the same at 169–174. C3 reproduces this in all six voice paths, in original and full-source runs, even at MIX0. The data may be contiguous with another row, but this is outside the declared int16_t[1024] subarray; contiguity is not a valid interpolation-boundary policy.

Disposition: **unresolved, outside snare-only authorization**. Defining wrap/end interpolation changes a shared oscillator implementation and potentially its seam sound. No mask, endpoint duplication, coefficient or source substitution was introduced.

### 2. Blocking: filter arithmetic becomes nonfinite before sample conversion

ResonantFilter.c:263–285 evolves the internal states; 299 converts fastTanh(y1)×32767 to int32 before its existing __SSAT. In C7, all three drums report `-nan is outside the range ... int` in the first peak block. C11 independently reproduces that diagnostic in the snare with no transient and a cutoff produced from ordinary frequency value126 through the actual value shaper. Thus the finding is not solely a hypothetical .9 setter value or a downstream snare sum overflow. The snare still calculates its noise filter even when its noise mix weight is zero.

Controls are finite and bounded; high resonance maps q to .02 and filter drive1 maps to 6.4 (ResonantFilter.c:24–27, 89–103). This host result establishes an invalid arithmetic/conversion case, not a measured ARM failure or complete diagnosis of filter stability. A saturator after NaN does not make the preceding conversion valid.

Disposition: **unresolved**. Do not substitute zero for invalid values, reset/limit filter state silently, alter voicing or skip a currently processed source to make a test pass. A finite-state/filter policy and suitable source tests require a separate scope decision.

### 3. Blocking: negative FM input is cast directly to unsigned phase

Oscillator.c:117 (SINE), 165 (wavetable FM) and 193 (PWM FM) cast modBuffer×fmMod directly to uint32_t before shifting. C2 reaches negative finite products: e.g. −8.91 in a drum and −32766 in percussion. Native float-cast-overflow reports out-of-range unsigned conversion. C2 reproduces this for voices 1–3, 5 and 6; snare has no such FM path in this selected configuration. It precedes MIX and remains at MIX0.

Disposition: **unresolved**. A defined signed/modular phase-conversion policy affects shared synthesis and could change how the target currently treats negative inputs. No signed-cast workaround or sanitizer exemption was applied. Host diagnostics are not proof of the exact installed ARM instruction result.

### 4. Blocking: PWM phase addition overflows the float-to-unsigned domain

Oscillator.c:665 adds pwm×(UINT32_MAX/2) to the phase using floating arithmetic, then converts the result to uint32_t; the FM equivalent is at 194. C10 with PWM=1 reaches approximately 4.32524e9, outside the unsigned range. All six paths fail independently of selected MIX. This is distinct from negative FM conversion and occurs with zero FM excitation.

Disposition: **unresolved**. Defining modular PWM phase addition is a shared oscillator change, not an authorized snare clamp. No PWM mode or voice was excluded.

### 5. Blocking: signed transient data is shifted before volume multiplication

transientGenerator.c:60 evaluates `transientData[...][phase] << 8`. C5 reaches negative signed 8-bit data (e.g. −1); a negative signed left shift is undefined. C12 proves that selecting waveform2 with transient volume0 does not avoid the evaluation. All six paths fail, including selected MIX0. This was masked by the earlier short positive-sample reproduction.

Disposition: **unresolved**. Defined scaling could be a focused shared-generator change, but it is beyond the only authorized numeric change. No sanitizer suppression, sample substitution or blanket clamp was added.

### 6. Authorized snare conversion: addressed in the temporary reference, not firmware

Snare.c:183–196 / 210–221 forms the floating combined sample before int16 conversion and voice distortion. In C1, unchanged R10 passes MIX0–78 and fails **79–127** in both velocity modes. The exact previous reproduction remains: frame1 oscillator2135 + transient32512 = **34647**; maximum first-block source sum **52787**. The temporary new-path clamp saturates **38/64 samples** for every positive selected MIX; at target zero it saturates none and generates zero. C7 has a second R10 conversion threshold, **116–127**, resolved by **33/64** temporary full-source saturations.

| MIX | C1 R10 post-voice peak / result | Full-source peak / saturations |
| ---: | --- | --- |
| 0 | 0 / pass | 0 / 0 |
| 1 | 408 / pass | 32767 / 38 |
| 5 | 2047 / pass | 32767 / 38 |
| 10 | 4099 / pass | 32767 / 38 |
| 20 | 8215 / pass | 32767 / 38 |
| 30 | 12348 / pass | 32767 / 38 |
| 60 | 24844 / pass | 32767 / 38 |
| 78 | 32414 / pass | 32767 / 38 |
| 79 / 80 / 90 / 127 | original conversion fails | 32767 / 38 |

These are excitation samples after voice distortion, **before any post MIX or CLP effect**. They are not final mix-level measurements. C7’s 115/116/117 threshold neighbors and every other integer are in the linked sweep.

The temporary helper asserts finite input, clamps to [−32768,32767] and then converts; both production velocity branches call it. An additional sanitizer check compares **100001 values in the original defined-conversion domain**, including fractional values just outside the clamp rails whose truncation remains representable, and nine signed boundary/over-range controls. Conversion results match where originally defined. It intentionally does not emulate wraparound. Ordinary R10/direct/fallback files are untouched.

Disposition: **accepted policy works for the tested finite snare boundary**. Clipped peaks will alter drive/CLP excitation and require listening if a candidate is later possible. This does not repair upstream nonfinite filters, transient shifts or oscillator failures.

## Detailed transitions, zero and retained state

A separate **720-run** production-generation/decimator comparison uses MIX **0,1,5,10,20,30,60,78,79,80,90,115,116,117,127**, all six voices, both velocity modes, original/temporary full-source paths, two profiles and 2048×64 frames (~2.97 s). It uses C0’s supported SINE/no-transient/filter-OFF source, decay30, returning note triggers, real voice envelope and physical poles. See [transition results](repro/2026-10-31_r11-all-voices/transitions-results.json).

- MIX profile: zero target at blocks8–31 while the note is active, return to selected target, then zero from block768. Physical fader stays full. The actual decimator is deliberately frozen at rate0 during that first zero interval; elsewhere rate=.125.
- Fader profile: physical u moves 1→0, holds, returns to 1; q=u² is supplied to the existing pole. MIX stays fixed. Returning note triggers at blocks512 and1024; physical rates .01/.1 stay unchanged.
- All 720 runs complete with guards/sanitizers. Positive selected MIX yields identical **64-bit source fingerprints** within each full-reference voice/mode/profile group. This is source-generation evidence, not complete CLP state invariance or independent wet-level control.
- All 90 successful static full-reference groups also have one positive-MIX source fingerprint. **86 successful MIX127 original/reference pairs** match fingerprints under the settled conditions. This is a fingerprint comparison, not a claim of full-engine byte/state equivalence. Over-range snare pairs intentionally fail in original R10 and are not counted as equivalent.
- At MIX20, each demonstrated voice can retain **1536 nonzero decimated samples** during the frozen zero-target interval, including the temporary source whose newly generated samples are zero. Drum1 R10 also has 285 nonzero raw samples during its smoother release. A source-stage zero alone therefore does not satisfy the proposed bus-zero endpoint.
- A temporary **post-decimator zero gate** makes the would-be injected samples exactly zero, without resetting decimator/filter history; its output is checked in the fixture. This is a necessary endpoint placement demonstration, not implemented mixer/CLP integration. It can step and may reveal old held samples on return; ordinary histories must not be silently reset.
- The selected natural/returning sources settle: latest nonzero transition sample is frame 81471 of 131072. This is source silence, **not** a new CLP idle-suppression result. Numeric waveform steps are recorded but do not establish click-free or musically preferred fades.

Runtime target steps model automation but bypass real MIDI/LFO/morph/recall scheduling. No stale UI/kit byte is used; no live smoother is advanced twice. Persistent filter histories are exercised through actual generation. Failed filter configurations are separately recorded rather than hidden by reset. Full route/FX/kit lifecycle ownership remains unimplemented/unverified for R11.

## Reproduction, commands and preliminary failures

Published native audit files: [prepare.py](repro/2026-10-31_r11-all-voices/prepare.py), [audit.c](repro/2026-10-31_r11-all-voices/audit.c), [metrics.py](repro/2026-10-31_r11-all-voices/metrics.py), [transition.c](repro/2026-10-31_r11-all-voices/transition.c), [transitions.py](repro/2026-10-31_r11-all-voices/transitions.py) and [clamp.c](repro/2026-10-31_r11-all-voices/clamp.c). They export the pin and write native executables/copied audit sources/results outside the checkout. No firmware build runs.

Requirements: Python 3.12+, native GCC/libm/ASan/UBSan and the pinned Git object. No NumPy or ARM compiler is required. Complete reproduction:

```sh
python3 docs/codex-berichte/repro/2026-10-31_r11-all-voices/prepare.py --repo /path/to/lxr-desktop-app --output /tmp/r11-all-voices --run
```

Native compiler: **cc (Debian 14.2.0-19) 14.2.0**. Flags retain ASan/UBSan/bounds/float-cast-overflow, nonrecovering errors, -O2, -fno-strict-aliasing and implicit-declaration errors. Production config definitions are checked. [Native metadata](repro/2026-10-31_r11-all-voices/native-metadata.json) records exact compiler commands and production source hashes. Original translation units remain unchanged; the temporary full-source patch is generated with asserted exact replacements. The copied BufferTools header changes only its include spelling to reach the actual dither header through host include paths.

Actual full runs used `/workspace/r11-all-audit/prepare.py`, then metrics.py and transitions.py. The packaged portable reproducer was independently exercised with `--smoke` against `/workspace/r11-source`, output `/workspace/r11-all-audit/repro-smoke`: **936 cases**, 536 pass/400 fail. Every classification and every successful metric matches the corresponding full-sweep case. It does not replace the all-128 sweep. The clamp arithmetic check passes. The script’s success means reproduction completed, **not** that failing candidate prerequisites passed.

Preliminary issues and corrections:

1. Initial compilation found that the inherited CLP BufferTools shim is empty for drum/hat needs and that an extracted PWM-FM definition followed its caller. The actual header was selected and function extraction order corrected; no production algorithm/assertion was weakened.
2. Initial long runs used a sampled transient even at volume0, so its signed shift masked otherwise independent paths. Clean controls now select snap mode with zero amount; separate C5/C12 retain and reproduce the original failure. C1/C7 peak-block runs isolate snare/filter behavior. No transient sanitization was added.
3. A runner draft passed Python boolean text to C atoi; this was corrected to 0/1 before the full 39936-run matrix. Exploratory figures are not the final matrix.
4. Initial transition zero occurred after some notes were nearly quantized away, and hat retriggers restored a longer decay. The final fixture moves zero into the active note, deliberately freezes the decimator and sets the same short hat decay for retriggers; final results are the 720-run set above. The earlier 576-run transition set is superseded, not added to final coverage counts.
5. Creating the new report worktree initially used its not-yet-existing directory as the command working directory; the process did not launch. It was rerun from the existing source worktree.

The initial publication whitespace check flagged standard CRLF record endings in the summary CSV. The summary was normalized to LF in a normal documentation follow-up commit; parsed cells were compared and remain identical. Its size/hash below reflect that formatting correction. No numerical result or sanitizer assertion changed.

Other actual checks: git ls-remote --heads; git show of the three pinned reports; full voice/combinedVolume/source/filter reads with rg/nl; clean source/tree comparisons; native script syntax checks; compressed CSV integrity and row count; remote publication/ref/blob verification. No source commit, regression assertion, sanitizer flag or production fix was hidden to deliver a passing firmware.

## What remains unrun and why no build occurred

The audit completed the feasible selected-configuration sweep and isolated the independent findings. It does not enumerate every parameter combination. These five additional prerequisite policies would affect shared synthesis/filter code beyond the expressly authorized new snare boundary. The task requires consolidated reporting and stopping before candidate implementation/build in that situation.

No R11 routing eligibility, isolated whole-effect return, post-MIX buffer, physical H capture integration, other-effect/direct isolation, D/W/VOL arithmetic, mono/stereo/bus destination behavior, 1/7/64 full-engine partition checks, FX/kit lifecycle, actual morph/recall or CLP state/idle tests were implemented or run. Those are **pending**, not passed by the generation fixtures. Existing broad regressions were not repeated for an unchanged source tree. Earlier extracted-tail/CLP reports do not establish complete R11 engine coverage.

There was **no Release ARM build**, image patch, package, CI workflow change/run or downloadable R11 firmware artifact. Header/length/checksum/reserved bytes, vectors, startup label, application flash boundaries, ELF/image consistency, ZIP membership and downloaded-firmware checks therefore have no R11 result. There are no invented artifact identities.

Recorded corrected R10 resources remain reference data only: ITCM 15608/16384 B; DTCM 107672/131072 B; RAM 274648/393216 B; linker FLASH 237468 B. No R11 resource delta, new stack/buffer requirement or code-generation result is measured. Audit subprocess memory/timing proves nothing about device CPU deadlines or stack capacity. The source still has the correct historical R10 splash; no candidate exists requiring an R11 splash change yet.

## Next scope decision and eventual listening

First authorize and define a separate bounded source-arithmetic prerequisite task: wavetable seam/index behavior, signed FM and modular PWM conversion, defined transient scaling, and finite filter behavior. Those choices need comparisons against preserved R10 and explicit sound implications. They must not be bundled silently into “move MIX,” treated as proven acoustically neutral or addressed by disabling modes/voices. No default six-core architecture, normalization, new threshold, limiter or bass redesign is proposed.

After prerequisites are resolved, resume the approved solo reference on the reserved branch: exactly one configured CLP route, any of six; full source for positive runtime MIX; existing MIX and physical smoother ownership; post MIX on the entire isolated core+bass+dry return before unrelated output accumulation; explicit zero after decimation; unchanged R10 zero/multiple-route algorithms and Maximum/H; no history reset or hidden routing crossfade. Carried history after eligibility changes need not equal uninterrupted R10.

If a later candidate passes, listening still starts with Voice 6 at full physical fader and MIX 5/10/20/127 against R10, then physical sweeps, zero/return and D/W; repeat drums/snare and the accepted over-range snare case. Evaluate saturation attacks, bass/dry differences, level and timbre separately. Installation/listening remains subject to separate later approval. This report is not development-stage acceptance or a freeze.

## Publication and audit-data identities

Only this report and native reproduction/evidence files are added under docs on the new exact-base report branch. All 72 existing heads, including reserved R11, R10 and the historical blocked report, remain unchanged. Remote branch/report file and evidence blobs are verified after pushing.

| Audit data (not firmware) | Bytes | SHA-256 |
| --- | ---: | --- |
| all-mix-values.csv.gz | 266318 | `de3e4e746e68c0233be1ee2d5e3ffef58eb5f4a55bb0c59475e97bfbb4698568` |
| audit-summary.csv | 11401 | `24253fd2b410784688f70c3f72faa2a59b156cd1fbe7dbb17d9677c31980e145` |
| transitions-results.json | 289695 | `247563821a060ea6ce6fc6d46b9f528cae39beae41b655f6bc656d7e10fffb3d` |

The compressed sweep expands to 4211944 bytes and 39936 data rows. These files are native audit evidence, not an R11 image/ZIP, SOURCE.zip or firmware build artifact.
