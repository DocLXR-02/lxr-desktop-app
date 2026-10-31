# CLP MIX/VOL decoupling design, preserving R10 as reference

Date: 2026-10-31 (UTC). Repository: https://github.com/DocLXR-02/lxr-desktop-app.
Exact reviewed source: **8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7**, **fx/clp-multivoice-max-r10**.
Report-only base: **5829398cea5d337200393757e39a37d4d8148256** (lab-setup).
Report branch: **codex-reports/2026-10-31-clp-mix-vol-decoupling-design**.

## Recommendation and product choice

The requested separation is feasible as a **single-routed-voice reference arrangement**: retain the physical fader in its original voice path and R10 output-control role, generate that voice's source as if MIX/VOL were 127, and apply its selected MIX/VOL after the processed route. Obtain a proper source tap; do not divide finished samples by volume.

For multiple voices, **one shared nonlinear return cannot guarantee independent post-effect voice balances**. Removing every MIX/VOL from excitation and adding one output multiplier loses their relative balance. Keeping relative input weights preserves some balance but leaves input-dependent timbre coupled to those weights. Separate cores provide identifiable wet streams but change the shared intermodulation/feedback instrument. No scalar-policy trick guarantees all three: full excitation independent of each MIX/VOL, independent final processed balances, and the same shared R10 interaction.

**Before a multi-voice implementation, the concrete product choice is whether shared interaction or independent post-effect wet balance takes priority.** Bounded common-gain factoring is a possible partial compromise, not a solution to the owner’s low-volume voice alongside a louder voice. Do not default to AGC, activity weighting, inverse audio gain or six cores.

The smallest useful next prototype is a **separately authorized solo-routed diagnostic reference**, using one existing CLP core and the existing controls, with explicit zero behavior and no bass/parameter refinement. It can establish whether full excitation plus a post level is the desired sound at MIX/VOL 5–20. It must not be sold as the completed multi-voice solution. The bounded task and listening criteria are below; no firmware implementation is authorized or performed by this report.

## Evidence and scope

Read root AGENTS.md and both R10 reports:

- [Original R10 Maximum build](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-31-r10-clp-multivoice-max-test-build/docs/codex-berichte/2026-10-31_r10-clp-multivoice-max-test-build.md).
- [Startup-display correction](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-31-r10-startup-display-correction/docs/codex-berichte/2026-10-31_r10-startup-display-correction.md), which identifies corrected source 8082cc5 and supersedes the earlier delivery for owner use.

Their installation-pending statements were historical. **New owner listening evidence:** R10 voice additions/removals sound plausible and playable. This is not comprehensive hardware validation. The owner balances MIX/VOL at full physical faders, using the mechanical upper stops as chosen maximum performance levels. Voice 6 at MIX/VOL 5–20 can sound predominantly regenerative; raising MIX/VOL restores stronger drive but disrupts balance. No owner WAV or matching new kit is available, so no synthetic result here identifies that kit’s precise oscillator/filter/velocity/modulation conditions or proves a whistle classification.

Before experiments, checked **69 existing remote heads**, local filenames and the contents of **14 CLP/R5–R10 report branches** for related work. No completed MIX/VOL-decoupling design/implementation was found; an older generic loudness/feedback-decoupling mention was not this feature. R10 remote/local heads matched the requested pin. Source checkout remained clean and unchanged. Only this English report and three small offline scripts are published on a new report branch. No production/test/workflow changes, firmware build, installation, hardware tests, merge, release, force push, settings change or development freeze occurred. Prior full R10 regressions/build audits were not repeated.

## MIX/VOL mapping and all six production paths

All references use the pinned commit. Paths abbreviated DSPAudio/... are under Core/Src/Soundengine/DSPAudio.

MIX-page VOL is **PAR_VOL1 through PAR_VOL6**, displayed as TEXT_VOLUME with DTYPE_0B127 (Core/Inc/menu/menuPages.h:74–75, 128–129, 182–183, 225–226, 271–272, 319–320; Core/Src/menu/menu.c:347–352). It is not the snare noise/oscillator `.mix` control. MidiParser.c:147–165 converts an ordinary parameter value v to **m=v/127**, with no squaring or perceptual volume shaper. Cases at 494–495, 530–531, 567–577, 629–630 and 787–788 assign the six `.vol` fields. ParameterArray.c:371–382 exposes them as TYPE_FLT.

Normal UI/control targets are 0…127 → 0…1. A float modulation call supplies its runtime value directly; modulationNode.c:176–180 bounds its generated value to [0,1]. Thus the stored UI value is not necessarily the instantaneous `.vol` when modulation targets volume.

| MIX/VOL value | Linear m | Gain before downstream nonlinear processing |
| ---: | ---: | ---: |
| 1 | .007874 | −42.08 dB |
| 5 | .039370 | −28.10 dB |
| 10 | .078740 | −22.08 dB |
| 20 | .157480 | −16.06 dB |
| 127 | 1 | 0 dB |

These are control gains, not predictions of final CLP loudness.

| Voices | Actual volume/distortion order and smoothing |
| --- | --- |
| 1–3 | DrumVoice.c:241–311: source/oscillator and transient mixing → voice filter → amp-envelope gain → optional velocity gain → **voice distortion** → **smoothed MIX/VOL × smoothed physical fader** → int16 buffer. SMOOTH_MIXER_VOL_PAN_DRIVE=1 and USE_FILTER_DRIVE=0 (config.h:33,45). MIX uses MixerPageFilter[0], r=.01, initialized at DrumVoice.c:57–64 and advanced at 305–310. Voice distortion uses calcDistBlockFiltered, whose drive parameter has its own r=.01 filter. MIX is after this distortion. |
| 4, snare | Snare.c:150–236: filtered noise + oscillator + transient → **MIX/VOL × optional velocity × envelope × smoothed physical fader** → int16 → **calcDistBlock**. Lines 183–196 / 210–221 show both velocity branches. There is no corresponding MIX smoother here: `.vol` participates in the block-level combined gain. The scalar fmaf sum plus transient is not explicitly saturated at its float-to-int16 assignment. |
| 5, cymbal | CymbalVoice.c:154–250: oscillator/FM/filter and transient → saturating source/transient addition → **MIX/VOL × optional velocity × envelope × smoothed physical fader** → int16 → **calcDistBlock**. Both branches are at 187–205 / 216–234; no MIX smoother. |
| 6, hi-hat | HiHat.c:166–257: oscillator/FM/filter and transient → saturating addition → **MIX/VOL × optional velocity × envelope × smoothed physical fader** → int16 → **calcDistBlock**. Both branches at 195–213 / 224–241; no MIX smoother. |

At REAL_FS≈44140 Hz, r=.01 has a ~2.25 ms time constant. Percussion MIX changes apply through its current block target rather than that smoother. Physical faders have a separate OnePoleFilter: r=.01 for voices 1–3; r=.1 (~.215 ms) for 4–6. adcPots.c:256–277 stores **qtarget=u²**, with existing ADC hysteresis and a snap below physical .01. Production calcOnePole (1PoleLp.c:27–32) uses fmaf and updates b once per consumed frame.

Velocity is separate: trigger paths set `.velo=velocity/127` (DrumVoice.c:170; Snare.c:102; CymbalVoice.c:117; HiHat.c:120). `.volumeMod` enables its amplitude multiplication; velocity can additionally modulate other destinations. Envelopes shape note level independently of MIX, and percussion `.egValueOscVol` is updated in asynchronous envelope processing. “Full MIX” must not mean removing envelopes, velocity, physical-fader gain, voice drive or filters.

The voice distortion in **DSPAudio/distortion.c:19–57**, distinct from the CLP effect file, has:

```text
s = voice_drive/127 − 1/128
k = 2s/(1−s)
Dk(x) = (1+k)x / (1+k|x|)
```

For voices 4–6, MIX is inside Dk's input. Generally Dk(mz)/m ≠ Dk(z), even before quantization. The ratio varies with |z|. For 1–3, a steady positive m can approximately be factored out of the final linear gain, but integer truncation already loses samples/bits; at changing m, using the knob target instead of its smoothed per-frame value is also wrong. No division reconstructs zero-volume samples, clipped mixtures or discarded low-level information.

### Into the shared CLP effect

mixer.c:439–467 pans and int16/saturating-adds all FX-routed voices into the common stereo bus; the active build uses cached triggered-pan gains. FxBus holds one MasterDistortion. Distortion.c:498–511 derives the parallel bass from the original consumed bus, trims core excitation by .22, detects original-input inactivity and adds lastOut feedback. Filters/drive/slew/diode/tone follow; lines 625–634 / 688–693 form:

```text
lastOut = tone_output × CLP_VOL
wet = H(qmax) × lastOut + parallel_bass(original_bus, CLP_VOL)
return = existing dry/wet blend(original_bus, wet)
```

CLP VOL is effect parameter p3 **inside the return history** and on bass, not a substitute for independent final voice MIX. D/W is a shared outer blend (MidiParser.c:1389–1393; effect parameters follow); it is not voice volume. R10 H=0 at qmax<=0, 1 at qmax>=.25, otherwise 2sqrt(qmax). FxBus.c:51–79 replays the configured voices’ physical fader filters, not MIX filters. At all physical faders 100%, H is unity even if MIX is tiny or zero. Silent/zero-MIX configured voices still count by physical-fader position, as in approved R10 performance behavior.

**Verified code consequence:** lowering MIX reduces the voice signal reaching CLP, and for percussion also changes its final voice-distortion response. Bass/dry already follow this weighted signal, while internal regenerative energy and H do not scale as a final MIX trim. **Acoustic hypothesis consistent with the owner’s observation:** weaker new excitation lets feedback dominate instead of the strongly driven source. Which waveform becomes a whistle, at what setting and how much bass is perceived depend on the unavailable kit/audio. This is not evidence that the original topology was accidental or faulty.

## One-voice separation and dry/bass consequences

Let Xi(m,q) be the actual finished voice path at MIX m and physical-fader trajectory q, including its existing velocity/envelope/drive processing. Let F denote the unchanged R10 effect, including H, bass, CLP VOL, detector/history and D/W. Current sole-voice behavior is F(Xi(m,q)). The literal post-level reference is:

```text
E = Xi(1,q)                       effective MIX target > 0
E = 0                            effective MIX target = 0
y_reference = Q16(m_post × F(E))
```

m_post follows the existing resolved MIX trajectory: r=.01 replay for 1–3, current block target for 4–6. The experiment makes its exact-zero endpoint mute explicit: if the effective target is zero, excitation and post output are zero. It does not keep injecting a full source behind a muted output. This exact endpoint bypasses the drum MIX smoother’s remaining release at zero and can step; it is an explicit reference contract, not an assertion that current R10 already behaves that way. Existing CLP feedback history may ring down internally under its unchanged idle policy; no new source is fed and no reset is forced.

At MIX 127, the reference can equal R10. For fixed positive MIX, internal excitation follows the **full-MIX R10 fader trajectory**, while only the audible post level changes. Physical q stays in its original position, including before voice distortion for percussion, and H keeps its R10 role. Do not obtain this source by globally overwriting the live `.vol` or by advancing a live smoother twice. For percussion, construct the full-MIX path at the actual pre-volume stage and run the downstream voice distortion correctly. For 1–3, an existing post-voice-distortion/pre-MIX stream is a useful separation point.

Applying m_post to the **whole isolated FX return before it is accumulated with unrelated direct voices** scales core, bass and dry. It must not scale the whole output buffer and attenuate other direct routes. Consequences must be accepted explicitly:

- For percussion, even 100% dry can differ from R10 at low MIX because Dk(mz) is replaced by mDk(z). This is not only a CLP-level change.
- Bass coefficients need not change, but bass level/history can: LP(m(t)x) is not generally m(t)LP(x), and upstream voice distortion/quantization also differ. No bass redesign was tested; preserving every old bass/dry sample is not promised by a literal post-level arrangement.
- Keeping an original level-weighted bus for dry/bass while using a full-MIX core tap is a possible **partial** alternative. It would preserve those paths better but would not be the literal “entire processed voice × final MIX” arrangement. That distinction must not be hidden.
- Positive-MIX normalization **without** the post level removes the intended balance entirely. In the experiments the same full-input output is produced for all positive MIX settings.

An effective runtime zero is not necessarily the same as stored UI zero when a volume-targeting LFO/velocity modulator deliberately raises `.vol`. Preserve/define that existing modulation semantics rather than introducing an undocumented UI-zero override.

## Multiple routed voices: the unavoidable coupling

For independent sources A and B, even a linear core cannot generally satisfy:

```text
G × (A+B) = mA×A + mB×B
```

for unequal mA/mB with one common G. A-only requires G=mA; B-only requires G=mB; overlap cannot provide independent gains. Shared nonlinear clipping, cross-modulation and regenerative history make attribution harder: F(A+B) is not F(A)+F(B), and old voice influence has no voice tag while other excitation continues. A MIX-zero voice must stop new injection, but its historical influence cannot be independently extracted from the common loop.

A bounded shared compromise can factor out only a **common** amount. For steady effective gains:

```text
r = max routed mi
wi = mi/r if r>0; otherwise wi=0
shared excitation uses each voice at MIX wi
y_common = r × F(sum of those voices)
```

These are bounded parameter ratios, not division of quantized audio; r=0 explicitly means no excitation/output. For one voice wi=1, giving the solo reference. For multiple voices, quieter wi remain below one. If any voice has MIX 127, r=1 and this compromise does **nothing** for another low-MIX voice. It therefore does not solve the owner’s central unequal-balance case. For percussion, wi also remains inside voice distortion, so it cannot promise original relative output ratios. It may help a globally quiet kit but changes common loop drive and couples all voices when the maximum changes. A silent high-MIX configured voice can hold r up. This is why it was compared rather than selected as a default normalization scheme.

Automation makes factoring less transparent: smoothing mi/r is not the same as dividing the already smoothed mi by a smoothed maximum. Near-zero continuous modulation can change ratios sharply, with persistent loop consequences for returning notes. The experiment tests this compromise only at steady controls.

Signed-sample reconstruction ratios can become unbounded at cancellation; absolute/RMS/activity weighting adds thresholds, gaps/release rules, pan sensitivity and possible pumping or suppression of desirable feedback. No such policy or AGC is proposed. The existing R10 input-inactivity detector is not a per-voice balance estimator.

### Separate processing alternative

One core per independently controlled processed stream permits post gains before wet summing. It also replaces the shared intermodulation/feedback instrument with separate histories. Matching R10 solo stereo behavior would generally require respecting its panned stereo input, rather than assuming a mono pre-pan core is equivalent.

The native MasterDistortion object is 176 bytes: two instances nominally add 176 bytes over one; six would total 1056 (+880), before buffers/control/target-layout checks. Code can be reused, but core processing grows with the number of instances. Voice taps, bass/dry routing, reset/recall ownership and transitions also need design. Recorded R10 ITCM free space (776 B) and memory fit prove no device CPU/deadline/stack capacity for this alternative. No ARM build or performance measurement was made. Two cores below are an architectural counterexample, not a proposal to default to six.

## Limited native experiments and actual findings

Scripts: [compare.py](repro/2026-10-31_clp-mix-vol-decoupling/compare.py), [experiment.c](repro/2026-10-31_clp-mix-vol-decoupling/experiment.c), [mixed.c](repro/2026-10-31_clp-mix-vol-decoupling/mixed.c).

They git-archive the exact pin to a temporary directory outside the checkout and compile actual voice distortion, CLP DSP, OnePole and R10 gain helper. They extract **verbatim production sync tails** after upstream synthesis/filter/envelope work, preserving volume, integer conversion, voice-distortion order and both compiled velocity branches. The mixer adder and gain helper are actual production code. Minimal named-field fixture objects replace full voice structs; this is not an ABI/full-engine test.

**Full voice generation was not exercised.** Post-filter/transient/envelope inputs are controlled synthetic buffers, with velocity .8, volume modulation enabled and synthetic envelope factor .8. Real oscillators/FM/noise/filter state, asynchronous EG interpolation, kit recall and modulation scheduling are not reproduced. Identical fixtures can make cymbal/hat tail results identical without implying those instruments sound alike. Source-times-volume alone is not used as proof of production behavior: the actual nonlinear placement/quantization tails execute, but upstream musical behavior remains unproven.

Final run: **78 solo cases**, **two D/W probes** and **four two-voice cases**, each 61824 frames (~1.401 s). Native GCC Debian 14.2.0, NumPy 2.3.5, -O2, -fno-strict-aliasing, ASan/UBSan/bounds/float-cast-overflow, nonrecovering failures. SMOOTH_MIXER_VOL_PAN_DRIVE=1 and USE_FILTER_DRIVE=0 are checked against the production config. Actual ARM fast-math/flush-to-zero behavior is not reproduced.

Coverage includes all six tails at MIX 0/1/5/10/20/127 and voice DRIVE 0/100, full physical fader; voice 1/6 fader sweeps at fixed MIX 10/127; positive/zero/returning MIX steps; sparse/returning synthetic notes; asymmetric stereo; unequal voice 1 + voice 6 mixtures; zero/all-zero balance. D/W=0/.4 probes supplement the main 100%-wet cases. Mono is assessed from production code, not rendered by these fixtures.

### Excitation and internal state, separate from output level

- For positive constant MIX and the fader sweeps, full-MIX-reference excitation sample arrays are byte-identical across selected MIX values. **Complete MasterDistortion byte traces at every 64-frame boundary** also match, including bass, detector and feedback history. This proves independence from MIX **within that reference**, not equivalence to low-MIX R10 histories. Current R10 and the new reference deliberately receive different low-MIX inputs.
- At MIX 127 the component-reference input, output and state equal baseline R10 in these fixtures. Physical-fader H trajectories match exactly and live physical filters are untouched by metadata capture. Private MIX replay matches the actual drum MIX update; live filters are not advanced for metadata.
- At cold zero, no excitation or output occurs. During zero/return steps the reference explicitly gates excitation/post output; final .1 s is quantized silence in all checked solo/pair cases. Existing core histories decay under the unchanged idle mechanism rather than an added reset.
- D/W=0/.4 full-reference state traces match the wet reference; D/W affects audible blending, not those histories. All checked states/outputs are finite; sanitizers pass. These are component-case findings, not a hardware or universal idle guarantee.

Representative stereo RMS over .02–.30 s, voice DRIVE=100, CLP drv/col/ton/vol=63/7/127/127, D/W=1:

| Voice / MIX | R10 bus input | Full-MIX input | R10 audible | Input-only full-MIX audible | Full-MIX + post level |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 / 5 | .006520 | .166120 | .465629 | .609898 | .023997 |
| 1 / 10 | .013061 | .166120 | .491836 | .609898 | .048009 |
| 1 / 20 | .026142 | .166120 | .522829 | .609898 | .096032 |
| 1 / 127 | .166120 | .166120 | .609898 | .609898 | .609898 |
| 6 / 5 | .010896 | .168956 | .475834 | .610326 | .024014 |
| 6 / 10 | .021187 | .168956 | .504369 | .610326 | .048042 |
| 6 / 20 | .040042 | .168956 | .535967 | .610326 | .096100 |
| 6 / 127 | .168956 | .168956 | .610326 | .610326 | .610326 |

These fixtures show why pre-effect MIX need not produce proportional final level, why input-only normalization loses balance, and what a literal post level changes. They do not establish a preferred sound or identify a whistle. Core RMS fields in solo.csv are **block-boundary proxies**, not per-sample core RMS; audible/input RMS above uses all samples.

As an offline diagnostic only, for positive fixed MIX the runner compares `finished_low_bus/m` with the actual full-MIX bus. It does not feed this inverse signal into DSP. Relative reconstruction error for MIX 5/10/20 is **.42%/.20%/.10%** for voice 1 at DRIVE=100, versus **70.99%/65.69%/55.87%** for voice 6. At voice DRIVE=0, voice-6 errors are smaller (2.81%/1.41%/.70%) but not zero, including quantization. This is not a universal error bound; it disproves treating final-sample division as a reliable full-volume source.

The dry consequence is measurable: voice 6/MIX10/DRIVE100 at D/W=0 has R10 RMS **.021187**, post-reference **.013292**. At D/W=.4 they are **.206582 / .023387**. The reference does not secretly preserve the original low-MIX percussion dry sound.

### Shared-core comparisons

Voice 1 plus voice 6, same fixtures/controls, static physical faders at full:

| MIX pair | R10 | Input-only full | Bounded common factoring | Two separate cores + individual post gains |
| --- | ---: | ---: | ---: | ---: |
| 10 / 127 | .612108 | .609079 | .612108 | .658163 |
| 5 / 20 | .545314 | .609079 | .096670 | .120045 |
| 0 / 127 | .610326 | .610326 | .610326 | .610326 |
| 0 / 0 | 0 | 0 | 0 | 0 |

At 10/127, r=1: bounded factoring is **sample/state-identical to R10** in the fixture and cannot fix the quieter voice. The two-core waveform differs from a shared-full-core output scaled by the common maximum by RMS **.118321**; at 5/20 the difference is **.030123**. This demonstrates non-equivalence, not musical superiority. The full-input pair produces **2133 mixer saturation events**, versus zero in the level-weighted/factored buses. A later output trim cannot undo that upstream clipping. Per-pair post sums are computed in float before WAV quantization; final sub-LSB details are not a firmware claim.

No total RMS monotonicity or click-free sound is claimed. Largest sampled steps in the solo matrix reach 34357 LSB and include waveform transitions; they are not isolated control-click measurements. Reference zero gating, post-level automation and switching architectures require listening.

## Edge cases and implementation boundaries

- **Zero and near-zero:** current percussion volume zero feeds zero into its voice distortion; drums use the existing short MIX smoother. The shared CLP can still have an idle tail or other voices’ feedback. A normalized reference must explicitly stop new excitation at effective zero, not merely hide an active source after the effect. Positive near-zero post levels retain full internal excitation by design; final output quantization can shorten audible travel. No division by zero or inverse of tiny audio gain is acceptable.
- **Headroom/quantization:** int16 truncation and saturating bus sums are many-to-one. Full-MIX taps can reach clipping that low MIX avoided. Snare’s unsaturated scalar sum/conversion needs particular full-path headroom testing; the bounded synthetic fixtures do not prove safety for all real transient/filter settings. Report a blocker rather than silently add a limiter or alter voicing.
- **Automation/morph/recall:** modulationNode.c:90–103 restores/reassigns runtime targets; 176–207 computes bounded float modulation. Preset morph ticks interpolate and send one parameter at a time (presetManager.c:2001–2073); routing stays with the source kit, while volume can morph. Recall queues the existing FX reset at 1164–1166 and resends parameters through existing service boundaries. New gain metadata must use the resolved audio-owned runtime trajectory, not a stale UI byte, and must not advance smoothers twice or keep stale associations after recalls/routes. The reference exact-zero endpoint and percussion’s unfiltered MIX steps can click. No full recall/automation scheduling proof is provided here.
- **Pan/mono/stereo:** preserve triggered pan and apply post gain to the isolated FX route before output accumulation. R10 mono consumes only the left interleaved input, while stereo activity is linked; a high-fader voice panned away from the consumed mono side still counts in H. The asymmetric stereo fixture does not validate every pan/output destination.
- **Silent routed voices:** retain R10’s configured-route maximum policy, including silent high physical faders. Zero MIX must stop that voice’s new source, but must not secretly remove it from the physical-fader route set or mute other voices with a global minimum-MIX gain.

## Bounded next implementation task — proposed only

After explicit authorization, implement an **isolated single-FX-routed-voice reference** for any of the six voices, anchored to pinned R10:

1. Generate that voice through its proper existing path with MIX removed from excitation, retaining envelope, velocity, drive/filter, physical q, pan and R10 H. Do not normalize final samples or overwrite global volume targets. Keep existing MIX smoother updates/cadence and expose/replay the resulting post trajectory coherently.
2. Apply the selected post MIX to the **whole isolated FX return** (core, bass and dry), outside feedback and before unrelated direct-output accumulation. At MIX127 require full-path R10 equivalence. This intentionally changes low-MIX percussion dry/voice-drive behavior and time-varying bass scaling; no bass coefficients or parameter refinement belong in the prototype. If preserving those old paths is mandatory, settle the partial-core-only alternative before implementation rather than claiming this literal post-level property.
3. Make effective target zero stop new excitation and post output, as in the reference. Preserve idle/reset ownership and do not reset histories on ordinary fader/MIX movements. Returning-note, exact-zero endpoint and modulation behavior must be explicit and tested; no added activity gate, AGC or hidden threshold.
4. Leave zero/multiple-route algorithms at R10 for the diagnostic prototype, without pretending this solves multi-voice balance. Route transitions into/out of the reference can change level/timbre, and carried shared history need not equal an uninterrupted R10 history. Obtain acceptance of that diagnostic scope; test transitions rather than adding silent resets/crossfades.
5. Verify actual full voice paths, especially voice 6 and snare headroom, at MIX 0/1/5/10/20/127, both velocity modes, fixed/full/swept faders, mono/stereo/pan, D/W/VOL, natural/sparse/return notes, automation/morph/recall, guards/finite state and resources. Any later build/install remains separately authorized; no CPU capacity is inferred here.

Listening should start with solo voice 6 at physical 100%, compare R10 versus full-excitation/post-level at MIX5/10/20 and MIX127, then volume-match deliberately by ear as a listening comparison—not an automatic firmware normalizer. Sweep the physical fader at each fixed MIX, listen to low-frequency balance, scream/whistle transition, last audible fade, zero/return attack and dry/wet changes. Repeat a drum voice and snare. Rebalance stored kit levels if needed: unchanged serialization does not imply unchanged gain semantics. Only after this reference is judged useful should the owner choose shared coupled interaction versus separate processed-stream balance for multiple voices. Do not silently change the shared sound or freeze the stage.

## Reproduction, commands, failures and publication

The three scripts total **198 lines / 15840 bytes**. They write only temporary/evidence files outside the source checkout. Python 3.12+, native GCC/libm/sanitizers and NumPy are required. From a report checkout with the pinned source object available:

```sh
git fetch origin fx/clp-multivoice-max-r10
python3 -m pip install numpy==2.3.5
python3 docs/codex-berichte/repro/2026-10-31_clp-mix-vol-decoupling/compare.py --repo /path/to/lxr-desktop-app --output /tmp/clp-mix-vol-results
```

Actual final invocation:

```sh
python3 /workspace/clp-mix-vol-evidence/compare.py --repo /workspace/r10-source --output /workspace/clp-mix-vol-evidence/results
```

Other checks: git status --short; git diff --exit-code; git ls-remote --heads origin; fetch/report git ls-tree/git show plus text searches; rg/nl source reads; native compilation with the sanitizer/config flags above. No CMake/ARM command ran. Outputs are solo.csv, results.json and **13 synthetic WAVs**; raw per-block state traces are compared in the temporary directory during the run. Large outputs are not committed. results.json records the pin, compiler/NumPy versions, source hashes and pair/D/W metrics. Temporary paths and host floating-point details are not portable measurement guarantees.

| File | SHA-256 |
| --- | --- |
| compare.py | `8f524745695335763f6f9cb56af1efa9e3475b5bed86ba7a0d99218662048a58` |
| experiment.c | `5e8eac229c3bea6257ccebfe437a1f2cffef1ac42d1910d6adf99892b692ff86` |
| mixed.c | `b8ab113db77f9f525729889d1e8f0d01a6f6fef5841a8dc52e39a4becf84a0b7` |
| Local solo.csv | `5e74d5d5dff636f2dd6383ec989ce9ab2fe37155eac5a83ee78543bb5d947d38` |
| Local results.json | `6caf9364cb651ac8296ad46489830b5c52bc21601bb44c4d7f3634845b705560` |

Initial extraction expected bufferTool_addGain in BufferTools.c; it is an inline function in BufferTools.h:103–110. The extraction guard failed before compilation, then was corrected to the actual header (attempt1.log). A first complete run passed but emitted redundant-brace initializer warnings in the mixed harness. Those initializers were cleaned up, D/W probes were added, and the final compact suite passed without those warnings. No production code or unrelated assertion was changed; no DSP, finite-state, zero-output or final sanitizer assertion failed. Logs/results remain under /workspace/clp-mix-vol-evidence; the source checkout’s status is unchanged.

All **69 pre-existing remote branch heads**, including R10, are preserved. Only this new report-only branch is published, based exactly on lab-setup **5829398cea5d337200393757e39a37d4d8148256**. Its diff contains this Markdown report and three reproduction scripts under docs/codex-berichte/repro. There is no firmware candidate, image, build workflow or production/test change. Remote branch/file contents and exact report URL are verified after publication.
