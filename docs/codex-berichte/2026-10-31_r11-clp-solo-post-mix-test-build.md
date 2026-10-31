# R11 solo post-MIX reference — blocked at source-headroom prerequisite

Date: 2026-10-31 UTC. Repository: https://github.com/DocLXR-02/lxr-desktop-app.

**No R11 firmware candidate was implemented, built or packaged. Do not install the reserved R11 branch as an R11 candidate.** A reproducible production snare conversion failure was found before editing. The requested full-MIX excitation would expose it at positive low MIX, where the corresponding R10 fixture is in range. Resolving it requires an explicit source-conversion/headroom policy; no limiter, source trim, voice exclusion or broader DSP change was added silently.

R11 is experimental, not hardware validated, and blocked before candidate generation.

This follows the task’s instruction: “If safe implementation requires unrelated limiting/voicing changes or broader architectural changes, report the concrete blocker instead of adding them silently.” The blocker is at the existing snare float-to-int16 boundary, upstream of voice distortion, panning and CLP. A post-effect gain cannot repair it.

## Identity, authorization and delivery status

| Item | Result |
| --- | --- |
| Exact source base/reference | **8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7**, corrected R10 |
| Existing comparison branch | fx/clp-multivoice-max-r10, preserved at that commit |
| New source branch | **fx/clp-solo-post-mix-r11**, reserved at the same base commit; **no R11 implementation commit** |
| Report-only base | **5829398cea5d337200393757e39a37d4d8148256**, exact lab-setup commit |
| New report branch | **codex-reports/2026-10-31-r11-clp-solo-post-mix-test-build** |
| CI / Release ARM build | **Not run**: prerequisite failed before candidate implementation |
| LXR_V2_R11.img / LXR_V2_R11.zip | **Not produced**; no sizes, hashes or artifact URLs exist |
| R11 SOURCE.zip / ELF / map / packaged build evidence | **Not produced**; no firmware build exists to associate with them |
| Reproducibility evidence | Three files linked below: native driver, C fixture and recorded results |

Read AGENTS.md, the [R10 Maximum build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/43996ab07671446f815cfc04d05a53e31092db28/docs/codex-berichte/2026-10-31_r10-clp-multivoice-max-test-build.md), the [startup correction](https://github.com/DocLXR-02/lxr-desktop-app/blob/57d4df90658244935e297dcc81a5a7b927714330/docs/codex-berichte/2026-10-31_r10-startup-display-correction.md), and the [MIX/VOL design at exact commit ae39a8b5398da1ea36bcb0148d40c8ab8461d184](https://github.com/DocLXR-02/lxr-desktop-app/blob/ae39a8b5398da1ea36bcb0148d40c8ab8461d184/docs/codex-berichte/2026-10-31_clp-mix-vol-decoupling-design.md). Their pending-installation statements are historical: the owner subsequently reports R10 voice additions/removals sound plausible and playable. That is listening evidence, not comprehensive hardware validation and not evidence for R11.

The owner authorized this limited single-configured-FX-voice reference, including the changed low-MIX percussion dry sound. That does not establish a desired sound for newly clipped/wrapped full-source samples. R11 remains an experiment to be implemented after the blocker is resolved; installation/listening would still require separate later approval. Nothing is frozen or finalized.

Before edits, inspected the complete inventory of **70 pre-existing remote heads**, local R11 branches/files and the available report branches. No existing R11 implementation/report was found. Local and remote R10 matched the pin. Created the source worktree directly from that commit. All existing branch heads remain unchanged; only the two new branches above are published. No production, existing test, workflow, parameter, storage or UI file was edited. The source branch therefore still has the **R10** startup label and must not be represented as an R11 build.

## Finding 1 — blocking: full-MIX snare source exceeds int16 before distortion

Precise production references below are at [8082cc5](https://github.com/DocLXR-02/lxr-desktop-app/tree/8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7):

- Core/Src/Soundengine/DSPAudio/Snare.c:148–177 generates noise/filter, oscillator and transient buffers.
- Snare.c:183–196 uses velocity; 210–221 is the velocity-disabled branch. Both add the transient to a floating oscillator/noise sum and assign the multiplied result directly to int16, without saturation.
- Snare.c:231 calls calcDistBlock **after that conversion**. Voice distortion cannot prevent this conversion overflow.
- Snare.c:82–122 sets the trigger phase, velocity, envelope and asynchronous envelope output.
- Oscillator.c:55–82 executes actual sine-table interpolation; 722–802 handles frequency and note selection. MidiParser.c:118–132 computes detune.
- transientGenerator.c:37–61 resets the sample and reads the production transient table, including volume and pitch.
- SlopeEg2.c:95–105 permits a first-block envelope value of one; attack mapping at 137–155 produces attack=1 for parameter zero.
- MidiParser.c:575–625, 981–999, 1055–1065 and 1143–1145 expose volume/envelope/noise mix, transient, filter-type and velocity-mode controls. These are valid control selections, not deliberately out-of-range values.

The relevant code is:

```c
snareVoice.combinedVolume = snareVoice.vol * snareVoice.velo * snareVoice.egValueOscVol;
float tmp = fmaf(buf[j], snareVoice.mix, oscBuf[j]);
tmp += transBuf[j];
buf[j] = snareVoice.combinedVolume * tmp
       * calcOnePole(&snareVoice.FaderVolumeFilter, adc_getFaderVolume(3));
```

The other branch omits only velocity. With physical fader/velocity/envelope all one, removing MIX from excitation exposes the whole `tmp` to int16 conversion. This is not a hypothetical bound based on independent maximum samples: the native experiment generates the offending samples with the actual selected production oscillators, transient table, trigger and envelope path.

### Reproduction settings and decisive values

The fixture calls Snare_init, configures a valid solo snare source, calls Snare_trigger(127,60), Snare_calcAsync and the **unmodified complete Snare_calcSyncBlock** for 64 frames. Settings:

- SINE oscillator, initial coarse/fine tuning 70/64; note 60; pitch-envelope amount zero.
- Snare noise/oscillator MIX=0 (oscillator only); filter OFF. This MIX is distinct from the requested voice MIX/VOL.
- Transient waveform 2 (first sample table), volume 127; transient frequency parameter 25 using the existing `1 + (value/33.9 − .75)` mapping.
- Amp attack 0, decay 100; production envelope reaches one for this first block.
- Physical fader held at full with its existing smoother settled to one; note velocity 127. Both velocity-enabled and disabled modes are tested.
- Existing voice distortion initialization is retained; no CLP or output processing is needed to reach the failure.

Native first-block results:

| Selected MIX/VOL | Maximum absolute preconversion sample, full transient | Native result, both velocity modes |
| ---: | ---: | --- |
| 0 | 0 | Pass; output is explicitly checked zero |
| 1 | 415.645660 | Pass |
| 5 | 2078.228271 | Pass |
| 10 | 4156.456543 | Pass |
| 20 | 8312.913086 | Pass |
| 127 / required full-MIX source | **52787** | **Conversion failure** |

The table’s maxima are diagnostic calculations from the actual generated component buffers; the failure itself occurs in the actual Snare_calcSyncBlock, independently of that diagnostic calculation. At zero-based **frame 1**, oscillator=**2135**, transient=**32512**, sum=**34647**, envelope/fader/velocity=1. Frequency is 466.272888 Hz and production phase increment 45369888.

Actual nonrecovering diagnostics:

```text
Snare.c:196:48: runtime error: 34647 is outside the range of representable values of type 'short int'
Snare.c:221:46: runtime error: 34647 is outside the range of representable values of type 'short int'
```

This is an out-of-range C floating-to-integer conversion. Host sanitizer evidence does **not** establish whether the existing ARM binary wraps, clamps, produces another target-specific result or audibly fails for the owner. No instrument test occurred. It does establish that this source cannot be assumed to provide bounded, defined int16 samples over the supported settings.

The failure already exists in pinned R10 at MIX127 for this fixture. The proposed R11 full-source rule would expose that same boundary at **every positive selected MIX**, including 1/5/10/20, before their final post attenuation. The low-MIX R10 cases pass because MIX currently reduces the value before conversion. This finding does not negate the owner's successful R10 listening or identify the owner’s kit as affected.

### Why no silent workaround was implemented

A later MIX gain, R10 H(qmax), CLP VOL or outer dry/wet blend cannot prevent an earlier invalid conversion. Keeping selected MIX before this conversion defeats full-MIX excitation. Excluding snare violates all-six-voice support. A fixed source trim changes the requested full-MIX reference. Defining wraparound is not a headroom solution. Adding saturation, changing gain staging or extending the voice path’s numeric domain changes how these peaks reach voice distortion and needs an explicit policy.

In particular, the requested unity-reference equivalence cannot be used as proof that an undefined R10 conversion is safe. Any corrective policy must distinguish inputs where R10 conversion is representable from the over-range domain, and specify the accepted behavior there. No R10 repair was bundled into this experiment.

## Integration inspection completed before stopping

Read the full sync/async/trigger implementations for DrumVoice, Snare, CymbalVoice and HiHat, their initialization and all combinedVolume uses. The existing separation points remain as described in the design:

- Voices 1–3: DrumVoice.c:285–310 puts velocity and voice distortion before MIX. Existing MixerPageFilter[0] advances once per frame at r=.01; physical fader is separate. A future full-MIX tap must retain these live updates while using the resolved MIX trajectory for the post return.
- Voices 4–6: volume is part of combinedVolume before final voice distortion, with block-target semantics; physical r=.1 smoothing remains separate. CymbalVoice.c:193–205/222–234 and HiHat.c:201–213/230–241 use saturating source/transient addition before their volume stage. That does not prove their entire engines safe for all modulation/filter settings.
- FxBus.c:51–79 privately replays physical fader states for configured routes. mixer.c:565–622 dispatches modulation/envelopes and captures physical metadata before live advancement; it subsequently decimates, pans and routes each voice, then calls the effect at line 732. The decimator has persistent sample history that would also need explicit consideration for the exact-zero/no-new-excitation endpoint.
- FxBus_processWithClipGain routes/sums directly into destination buffers that can already contain unrelated direct voices. A future post MIX must use an isolated effect return before that addition, not scale those existing destination samples.
- Uppercase FX/Distortion.c:625–634 and 688–693 holds the internal CLP output in lastOut before audible H attenuation, bass addition and dry/wet blend. The proposed final MIX belongs outside **all** these contributions and feedback. No change was made there.

No metadata ownership, modulation capture, isolated-return buffer, new voice API, zero gate, transition/reset behavior or splash change was implemented. Consequently this report does not claim their correctness or the complete requested integration coverage. Zero/multiple-route R10 Maximum policy, existing runtime modulation semantics, source/routing histories, audio-owned reset paths, serialized formats and controls remain untouched because the entire source is unchanged.

## Actual checks, commands and failures

Reproducibility files:

- [run.py](repro/2026-10-31_r11-headroom/run.py): exports the exact pinned Git object into a temporary directory, generates native hardware shims, extracts verbatim production oscillator/frequency/parser functions, compiles actual production translation units and classifies the expected failures.
- [headroom.c](repro/2026-10-31_r11-headroom/headroom.c): control setup, diagnostic copies and actual snare calls.
- [results.json](repro/2026-10-31_r11-headroom/results.json): compiler identity, source hashes and all 24 outcomes. Per-case logs regenerate in the chosen evidence directory.

| Reproduction file | SHA-256 |
| --- | --- |
| headroom.c | `f35332a4510d6f5b970ff6939e9a3c406a5bcc43547fdfa6e23e8b9d7c49a813` |
| run.py | `540272c089425d6b25bf3721756465fa398f50b4af0c20066e2b3dc44cde1612` |
| results.json | `55f2d7b78ec868ae0ea27a56a99680650ca76a1df01e4bad33a03d13bc9500f4` |

Requirements: Python 3.12+, native GCC/libm/ASan/UBSan and the pinned source Git object. NumPy and an ARM toolchain are unnecessary. From the report checkout:

```sh
python3 docs/codex-berichte/repro/2026-10-31_r11-headroom/run.py --repo /path/to/lxr-desktop-app --output /tmp/r11-headroom
```

Actual final invocation used `/workspace/r11-source` and `/workspace/r11-evidence/final-headroom`. Native compiler: **cc (Debian 14.2.0-19) 14.2.0**. Flags: -std=c11 -O2 -fno-strict-aliasing -ffunction-sections -fdata-sections; ASan/UBSan/bounds/float-cast-overflow, nonrecovering; implicit declarations are errors. Definitions NUM_DRUM_VOICES=3, OUTPUT_DMA_SIZE=64, USE_BIG_SAMPLE_BUFFER=1, INTERPOLATE_OSC=1 and SMOOTH_MIXER_VOL_PAN_DRIVE=1 are checked against production config.

**24 component runs: 22 controls pass, two expected production conversion failures confirm the blocker.** MIX0 output zero is asserted. Disabling the transient passes even at MIX127. The driver returns success only when it reproduces this precise blocker and the controls; that is **not** a passing R11 candidate or weakened production assertion. Existing tests remain byte-identical.

The complete unmodified Snare.c, transientGenerator.c, transientTables.c, wavetable.c, SlopeEg2.c, Decay.c, snapEg.c, 1PoleLp.c, lowercase distortion.c and ResonantFilter.c are compiled. Selected oscillator and frequency functions plus detune are verbatim extracted from the pin. Production voice/control headers are used; hardware/main headers and existing native config/globals shims replace target hardware. LFO dispatch and modulation service calls are no-ops, RNG returns zero, unsupported oscillator branches assert if called. Noise weight is zero; the selected SINE and transient paths are genuine production generation, **not synthetic upstream sample buffers** or an idealized source-times-volume model. Pointer layout, interrupts, real modulation scheduling, ADC polling and the complete mixer/CLP engine are not validated by this component test.

Preliminary attempts/failures are recorded in `/workspace/r11-evidence`:

1. Direct native Snare.c compilation with only the existing CLP shims failed on missing HAL types/DWT and unused intrinsics. Temporary hardware header shims were added outside the checkout.
2. Initial temporary shim compilation failed on a missing header guard and unused __QSUB16 declaration; corrected without altering production code.
3. The first reproduction mocked oscillator frequency/note handling and observed a conversion failure at 34013. It was superseded, not treated as the final evidence. Production frequency/note/detune helpers were then extracted; extractor attempts failed on a comment following a signature and whitespace before `(`. Extraction was corrected and guarded.
4. Final parameters were made explicit, including pitch-envelope amount zero and transient frequency parameter 25. Final compilation is warning-free. The final 34647 result and 24-case run above supersede exploratory numeric results.

Other actual commands included git ls-remote --heads origin, git worktree add -b at the exact pins, git show of the reports, rg/nl source inspection, git diff --exit-code and Git blob/hash checks. The R11 source checkout is clean, with no tracked or untracked changes.

## Unrun validation, resources and artifacts

No full-engine R11 integration exists to validate. All-six-voice/velocity matrices, MIX/fader sweeps, sparse/returning/natural decays, D/W/VOL arithmetic, direct-voice isolation, pan/mono/stereo/output destinations, routing/FX/kit transitions, morph/recall, metadata partitioning and full internal-state equivalence remain **unrun for R11**. The single 64-frame snare test is not 1/7/64 full-engine consistency evidence. Earlier extracted-tail analysis does not fill those gaps. No new CLP state, idle-suppression or musical-equivalence claim is made.

Relevant broad regressions were not rerun: there is no production/test/workflow change and the prerequisite already blocks a candidate. No Release configure/build ran locally or in CI. No image was patched. Header, length/checksum, vectors, splash, flash ranges, ELF/image correspondence, ZIP membership and downloaded-artifact checks therefore have **no R11 result**.

Recorded corrected R10 resource figures remain reference data only: ITCM **15608/16384 B**, DTCM **107672/131072 B**, RAM **274648/393216 B**, linker FLASH **237468 B**. There are no R11 resource deltas, new buffer/stack measurements, code-generation results or device CPU/deadline claims. The reserved source branch equals R10; it is not a newly measured build.

## Bounded next decision and listening plan

Resolve the snare source-conversion policy before resuming R11. A narrowly scoped option for subsequent explicit authorization is **defined saturation only at the R11 full-source snare conversion boundary**, in both velocity branches, preserving R10 fallback/direct behavior and requiring unity equivalence wherever the original conversion is representable. This would intentionally change over-range peaks; its effect on voice drive/CLP excitation must be acknowledged and tested. It is a proposal, not implemented or silently accepted here. A different numeric/gain-staging policy requires a separate justified scope decision.

After that decision, retain the originally authorized boundaries: exactly one configured CLP-routed voice, any of six; existing runtime targets and MIX smoothing cadence; no double live-filter advancement; full physical-fader transformation and R10 H; zero target stops new excitation and mutes the isolated return; no history reset on MIX/fader changes; whole-return post MIX before direct-output accumulation; unchanged zero/multiple-route algorithms. Cover decimator-held samples at exact zero and transition histories explicitly. No independent multi-voice wet balance is promised.

The eventual listening priority remains solo Voice 6, full physical fader, MIX5/10/20/127 against R10, then physical-fader sweep, zero/return and D/W changes; repeat a drum and snare. Include the explicit snare headroom case after an accepted conversion policy, and check for attack/drive changes. Parameter refinement, bass redesign and freezing the stage remain outside scope. **No installation/listening is authorized by this blocked report.**

## Publication

Published contributions are this English report and its three small reproduction/evidence files on the report-only branch based exactly on lab-setup. The new source branch is reserved at the unchanged R10 base, without a candidate commit. All 70 pre-existing branch heads, including R10, main, lab-setup and prior reports, are preserved. Remote source/report refs and report file contents are verified after publication.
