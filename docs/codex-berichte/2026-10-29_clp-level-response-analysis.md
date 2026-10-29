# CLP input-level response and musical control

Date: 2026-10-29 (UTC).
Repository: https://github.com/DocLXR-02/lxr-desktop-app.
Analyzed source: **af3562fece090288a83727524c1035b79f4cf20b**, branch fx/clp-input-silence.
Report branch: codex-reports/2026-10-29-clp-level-response-analysis.
Exact report base: lab-setup **5829398cea5d337200393757e39a37d4d8148256**.
Scope: analysis and this report only. No candidate, production/test/workflow edit, firmware build or hardware operation.

## Outcome

The physical fader reduces the signal **before CLP**, with a squared electrical gain law. It does not directly reduce CLP's internally generated feedback. CLP also strongly attenuates bass before a very large drive gain and a nonlinear, regenerative loop. These facts explain why lowering the fader can change the sound rather than simply make it quieter. They support, but do not by themselves prove, the acoustic explanation below.

The useful next direction is to provide **separate control over bass contribution and excitation of the existing screaming loop**, initially in an offline audio comparison. A global fader remap or a literal correction of the “7.2 Hz” comment is poorly targeted. The smallest first comparison is a fixed CLP input trim plus a restrained parallel bass contribution, preserving the current loop filters. Test a longer idle envelope separately so that changes remain attributable.

No constant can yet be claimed to place feedback reliably at 80–90% travel across kits. Real audio is required to choose it and to judge whether bass and screaming feedback coexist musically.

## Evidence status

Read [AGENTS.md](https://github.com/DocLXR-02/lxr-desktop-app/blob/5829398cea5d337200393757e39a37d4d8148256/AGENTS.md), the [R5 implementation report](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r5-clp-test-build/docs/codex-berichte/2026-10-29_r5-clp-test-build.md), and the [independent review](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r5-clp-independent-review/docs/codex-berichte/2026-10-29_r5-clp-independent-review.md).

**New DEVICE evidence supplied by the owner supersedes the reports' earlier hardware-pending statements for two specific behaviors:** the exact R5 image with SHA-256
`90caf0cc55203a828f02a108e0bd62f4ac8396c94e823e9680ee433991284a3f`
was successfully installed, and the unwanted sustained idle tone disappears at voice-fader zero and during missing sequencer steps. This is hardware confirmation of those behaviors, not comprehensive validation of R5, all routes or all transitions.

The owner reports broadly repeatable musical behavior across many settings and kits: useful distortion with strong input; substantial bass loss through approximately 100–70% physical fader travel; bright, bass-poor kick near 50%; desirable screaming feedback below approximately 40%. These are physical positions, not values 100/70/50/40 sent to a 0…127 parameter. They are observations to explain, not exact calibrated boundaries or a proven universal law. This analysis does not require one special kit.

Desired direction: screaming feedback around 80–90% travel, useful bass toward 50%, smooth audible idle decay about 0.5–1 s, eventual silence and clean return. No new recordings, calibrated ADC positions or captured CLP input were available here. There was no new listening test.

The previously verified source archive in /workspace/r5-source was reused; no artifact, CI, diode-bound or branch-history audit was repeated. GitHub reads confirm that the R5 branch still resolves to the pinned commit and lab-setup to the specified report base. All line references below use pinned R5.

## 1. Physical fader → voice → mixer → CLP

| Step | Source and result |
| --- | --- |
| ADC acquisition | Core/Src/main.c:1080–1086 configures 12-bit, left-aligned ADC data; line 337 starts DMA into adc_buf. Core/Inc/IO/adcPots.h:23 enables fader volume. |
| Electrical fader law | Core/Src/IO/adcPots.c:256–278 scans indices 8…13, one fader per call. An update requires a change greater than HYSTERSIS=40 in shifted 12-bit counts (definition at line 20). u=(adc_buf[i]>>4)/4096; u<0.01 snaps to zero; stored gain is u², not u and not a 0…127 preset byte. |
| Update cadence | adc_checkFaders() is called in the foreground loop, main.c:966; all six faders are visited over successive calls. No fixed wall-clock cadence is established by this code alone. Hysteresis is about 0.98% of full ADC scale, before smoothing and component tolerances. |
| Kick volume | Core/Src/Soundengine/DSPAudio/DrumVoice.c:275–310 applies amplitude envelope, optional velocity amplitude, voice distortion, then voice mixer volume times smoothed fader gain. Fader and mixer smoothing use coefficient .01, initialized at lines 54–61. |
| Smoothing | Core/Src/Soundengine/DSPAudio/1PoleLp.c:27–32 computes state += r*(input-state). At nominal 44140 samples/s, r=.01 gives a 2.254 ms time constant, not an audio bass filter on the voice waveform. |
| Voice/mixer volume versus fader | MidiParser.c:147–164 normalizes ordinary control bytes to /127; lines 494–495 set Voice 1's separate vol. The physical fader multiplies this, independently. It is also distinct from CLP's vol. |
| Routing, pan and accumulation | mixer.c:590–607, 616–619 creates/clears the int16 FX bus and routes the generated voice. The enabled triggered-pan path uses gains computed by DrumVoice.c:115–118. mixer.c:438–465 applies those gains, quantizes to int16 and saturating-adds voices into the interleaved FX bus. Optional decimation is upstream (mixer.c:60–74, 602–604). Other voices can change bus level and composition. |
| CLP dispatch | mixer.c:712 calls FxBus_process; FxBus.c:49–72 selects output/channel mode and line 109 calls Distortion_process. Distortion.c:473–482 normalizes the bus to approximately [-1,1] and adds feedback. Mono equivalent: lines 616–620. |

For the kick path, after smoothing settles and ignoring quantization/saturation, the bus contribution is approximately source × voice-volume × u² × pan gain. Other percussion voices also consume adc_getFaderVolume in their own processing; the detailed kick path above is not asserted to describe every envelope implementation.

Illustrative steady-state values, **only if physical travel is approximately proportional to u**:

| Approximate travel / ADC fraction | Fader amplitude multiplier | Relative dB |
| --- | ---: | ---: |
| 100% | 1.0000 | 0.00 |
| 90% | .8100 | -1.83 |
| 85% | .7225 | -2.82 |
| 80% | .6400 | -3.88 |
| 70% | .4900 | -6.20 |
| 50% | .2500 | -12.04 |
| 40% | .1600 | -15.92 |

Actual endpoints, electrical taper and mechanical calibration were not measured. Maximum 12-bit code is 4095, so exact unity is an idealized table endpoint. The owner-observed zero result is stronger evidence for their setup than assumptions about endpoint calibration.

Thus “halfway down” can already mean roughly 12 dB less excitation. The fader is not a post-effect output-level control. Remapping it globally would affect other effects and direct voice routes as well as CLP.

## 2. Why bass and screaming feedback trade places

For one channel, the CLP structure can be written approximately as:

```text
x = quantized FX input / 32768
v[n] = x[n] + (col/127)*0.125*g_idle[n]*y[n-1]
q = G * HP1 → LP1 → HP3 → LP2 applied to v, G=1+2304*(drv/127)
y[n] = (vol/127) * toneLP( diode( slew/railLimit(q) ) )
output ≈ (1-d/w)*x + (d/w)*y, followed by existing integer/saturation mixing
```

Here y is lastOut, not the final dry/wet output. Sources: Distortion.c:460–482, 506–545, 550–605; mono:619–662. The slew limiter limits sample change and rails clamp to ±4.2; the diode table then limits the signal near ±.647. Coefficients are fixed with respect to fader position.

Consequences established by code:

- Lowering the fader scales x; it does not multiply y[n-1] by that same fader. When y remains large through regeneration or clipping, feedback/input ratio rises. It is not necessarily inverse-square in practice because y also changes.
- drv changes gain of **both** the incoming signal and the returned feedback after summation. col changes return strength. ton filters the returned signal as well as the audible wet signal. vol is **inside the loop**, because lastOut is written after vol (lines 593–597); reducing CLP vol also reduces regeneration. d/w is downstream of lastOut and does not directly reduce the internal loop.
- The dry contribution, when present, also falls with the fader. A saturated or self-generated wet contribution can remain comparatively strong. This alone changes perceived balance even without a changing linear filter.
- Fixed linear filters cannot produce a level-dependent EQ curve on their own. Any change in spectral balance with input amplitude requires nonlinear drive/feedback behavior, changing dry/wet balance, source differences, quantization or detector activity.
- R5's idle detector looks at the original consumed int16 input, before this filter/drive chain (lines 181–198, 480, 619). It restores full feedback whenever either consumed channel exceeds ±1 LSB. It is not a deliberate threshold at 40% travel. On ordinary sufficiently strong playing it remains open. Very quiet tails, long near-zero portions or missing steps can invoke it, so it may still affect inter-note behavior.

**Acoustic interpretation, requiring recordings to confirm:** strong kicks force the loop and drive its nonlinear stages; they can mask, synchronize or suppress a freer high-frequency oscillation. As excitation declines, the kick fundamental and any dry bass fall while the regenerative component dominates. The strongly bass-rejecting prefilter below also makes upper harmonics easier to drive into clipping than the fundamental. This plausibly explains useful distorted bass at high input, a bright region while turning down, then exposed screaming feedback. Whether the owner hears predominantly masking, oscillator entrainment, altered saturation, or a combination cannot be diagnosed from source alone.

A rough bound shows why input and feedback need separate consideration: with settled tone state and normalized col=vol=1, the return term can approach .647479*.125 ≈ .08093, equivalent to 2652 int16 LSB before the common filters. At col=7/127 it is about 146 LSB. These are bounds, not measured kick/feedback ratios or fader crossover predictions.

## 3. High-pass coefficients and musical consequences

For state s[n]=s[n-1]+a*(v[n]-s[n-1]) and output h[n]=v[n]-s[n], let b=1-a. The exact transfer is:

`HP(z)=b*(1-z^-1)/(1-b*z^-1)`.

This matters because the code subtracts the **updated** state. The high-frequency gain is 2b/(1+b), not automatically unity. LP(z)=(1-b)/(1-b*z^-1). Calculations used nominal REAL_FS=44140 (config.h:74).

| Stage | Code | Numerical interpretation |
| --- | --- | --- |
| HP1, labelled “input HP 7.2Hz” | Distortion.c:490–511, mono:622–623; a=.6411664192 | b=.3588335808 gives matched-pole frequency **7200 Hz**, not 7.2 Hz. Actual -3 dB point relative to its Nyquist gain is **6194.39 Hz**; Nyquist gain is already **-5.545 dB**. |
| HP3, labelled 60Hz | Lines 530–534, mono:630–632; b=.9914955644 | Matched-pole frequency 60 Hz; relative -3 dB point ≈59.999 Hz. This comment agrees with the coefficient. |
| HP2, labelled 1539Hz | Lines 523–529 | Commented out. It does not act on the audio. |
| LP1 / LP2 | Lines 517–521 and 541–545 | Pole b=.1025353242 corresponds to 16 kHz, and b=.9313004164 to 500 Hz under the exponential pole mapping. Pole labels are not necessarily exact digital -3 dB cutoffs, especially near Nyquist. |

Exact linear magnitudes of the preclip cascade HP1×LP1×HP3×LP2, **before drive, feedback, diode, tone and dry mix**:

| Hz | HP1 dB | HP3 dB | Cascade dB |
| --- | ---: | ---: | ---: |
| 30 | -52.43 | -7.03 | -59.47 |
| 50 | -48.00 | -3.91 | -51.95 |
| 60 | -46.41 | -3.05 | -49.52 |
| 100 | -41.98 | -1.37 | -43.52 |
| 200 | -35.96 | -.41 | -37.01 |
| 500 | -28.02 | -.10 | -31.13 |
| 1000 | -22.06 | -.05 | -29.10 |
| 2000 | -16.28 | -.04 | -28.64 |

At drv=63, G≈1143.93 adds 61.17 dB before nonlinear limiting: the linear 60 Hz gain becomes +11.65 dB, versus +32.53 dB at 2 kHz. These are not predicted output levels in a clipping feedback circuit. They show a roughly 21 dB bias toward 2 kHz relative to 60 Hz in the feedforward cascade.

A literal 7.2 Hz coefficient would be a≈.00102437. Substituting it would add about **46.35 dB at 60 Hz** relative to the existing HP1, before clipping, and would change the phase and gain of the feedback loop too. This is a major sound-design change. It could drive bass into rails, alter the useful scream, produce low-frequency regeneration and change transient/intermodulation behavior. The mismatch is a documented code/comment fact; it is **not** sufficient evidence of a bug or authorization to “fix” the coefficient. The original self-oscillation was intentional, and the present sound is the musical baseline.

## 4. Two focused approaches, for later authorization only

### A. Preserve the current loop; separate its excitation from a parallel bass contribution

Keep all current CLP loop coefficients and col/ton/vol behavior within the existing core. Apply a fixed trim k to **incoming core excitation only**, leaving the return term unchanged. Add a restrained low-pass contribution from the original, untrimmed bus input alongside the core wet output, outside the feedback return. Initially try one low-pass around 150 Hz as a listening reference, not as a selected production value.

The new bass contribution should follow CLP vol and the existing outer d/w mix, decay naturally with its input/filter tail, and never be inserted into lastOut's feedback term. Detector activity should continue to use the original input; moving detection behind a trim would change quiet-tail behavior accidentally. No global fader change or new serialized parameter is needed for an offline experiment.

A dimensional starting estimate is k≈(.40/.85)²≈.221 (-13.1 dB). This makes 85% electrical travel excite the core approximately like the current 40%, **if** everything else and feedback amplitude were unchanged. It is a starting trim to audition, not a validated mapping: changing excitation changes the core output too, and kit level, velocity, pan, other voices, drv/col/ton/vol all matter.

Bass and scream can coexist because bass no longer needs to survive the same strongly high-passed nonlinear path. Tradeoffs: full-fader wet kick character will change; added bass can consume headroom or cancel/reinforce existing bass through phase interaction; the new component is less distorted and bypasses some core coloration. It still falls with u², so it does not guarantee strong bass at 50%. The existing controls retain their roles and kit bytes, but their total audible responses cannot be promised identical. Fixed trim is preferable initially to automatic normalization, which could boost quiet noise, erase velocity dynamics and shift activity thresholds.

This is the preferred first **offline** comparison because the screaming loop remains intact and the two required musical contributions can be balanced explicitly.

### B. Separate feedforward filtering from feedback filtering, with bass passing through the nonlinear core

Split the common preclip filter path into an input branch H_in(x) and a return branch H_fb(beta*y), summing before gain/slew/diode. Retain the current H_fb as the starting feedback voicing; allow only H_in's bass rejection and input weighting to change. The aim is a stronger bass-driven distorted kick while separately weighting excitation against the existing return.

In the linear region, equal filters on both branches reproduce filtering their sum, provided states/initialization are matched. With H_in changed, this becomes a deliberate topology change. Stronger low-frequency excitation can again force or suppress the desired scream; input weighting still needs calibration and no fixed setting guarantees 80–90% across kits. It needs more filter states and more careful reset/re-entry behavior, and can increase headroom/CPU demands. Existing drv/col/ton/vol/d/w roles and formats can remain, but timbre and control ranges will change.

This is the second choice if the parallel bass sounds detached. Do not simultaneously retune both branches or change the fader law; that would obscure which change helped.

A simple global remap is not a third proposed approach: mapping .85 to today's .40 with a pure power gain would require approximately u^11.28 instead of u². At .50 it would leave only about .0004 amplitude. It therefore works against retaining bass and affects unrelated routes.

## 5. A 0.5–1 s idle decay is an audible-envelope requirement

Existing R5: 441 inactive frames (~10 ms) then a nominal ~5 ms return-gain ramp (Distortion.c:154–198). The independent review explains the 222 float-subtraction steps to exact zero. The owner's new DEVICE evidence confirms that this suppresses the unwanted tone in the two reported situations; it does not establish a suitable musical release.

Making that ramp 0.75 s would slowly reduce **loop gain**, not necessarily loudness. A nonlinear oscillator can stay near saturation until a sustaining condition is lost, then collapse rapidly. Threshold and collapse time depend on controls and history, with possible pitch/timbre change or regrowth on renewed input.

A limited numerical illustration used the cascade above, one-sample return delay, tone/vol=127, drv=63 and approximate diode small-signal slope 4.499. A 1 Hz frequency scan gives maximum small-signal open-loop magnitude ≈1.312 at col=7, versus ≈23.80 at col=127. A linear idle multiplier ramp would bring these maxima below unity at about 24% versus 96% of the ramp, respectively. These are **not predictions of nonlinear collapse times**: phase, diode operating point, saturation and hysteresis matter. They show why a fixed feedback-ramp duration cannot be equated to a fixed audible release.

For a controlled output release, examine a wet-output envelope outside lastOut/feedback, with a bounded lifecycle:

1. After confirmed bus inactivity, fade the audible wet component over a defined target such as 0.75 s; a smooth-endpoint curve can avoid a slope discontinuity.
2. Keep enough internal loop energy for a smooth tail during that interval, rather than assuming a simultaneous large loop-gain reduction will sound smooth.
3. At effectively muted output, force return gain to zero and clear/retire CLP runtime history under audio ownership. A finite timeout is necessary; an asymptotic envelope alone is not a guarantee of eventual silence. A new parallel bass filter must also settle or be cleared at an inaudible boundary.

This is a release-design requirement, not an implemented solution. Moving the envelope into lastOut would make it alter regeneration again; its placement must be explicit. The dry input should retain the existing d/w behavior, rather than being faded because the wet oscillator is decaying.

**Returning notes are the difficult part.** R5 currently restores full return on the first >1-LSB sample. With a long tail still running, restoring wet gain abruptly can expose the old oscillator at full level. Resetting it abruptly can cut the tail; ramping the entire output can soften the new attack. A single shared core cannot promise uninterrupted old tail, freshly reset new-note history and an unchanged immediate attack simultaneously. First compare return attacks during early/middle/late release. If a fresh attack plus continuous old tail is necessary, an offline two-state tail/fresh-core crossfade is a way to assess it, but costs state/processing and requires a later design decision. Do not claim clean re-entry merely because the detector opens instantly.

**Linked stereo:** either active channel currently prevents release of both channels. Thus unilateral activity can sustain the opposite side indefinitely; eventual silence applies after all consumed linked input becomes inactive. A longer release does not change that policy. Retain the linked policy in the first experiment to avoid changing stereo image behavior at the same time. If per-channel decay is desired, it needs a separate musical decision. Sparse sequences returning within 0.5–1 s will overlap tails, deliberately changing the presently short missing-step response. Tiny >1-LSB residuals can repeatedly restart release, so quiet-input behavior still needs testing.

## 6. Smallest next experiment and acceptance

**Next recommended step: a separately authorized offline audio comparison, before another firmware candidate.** Obtain or capture actual R5 CLP bus input and associated output at marked travel positions, plus a steady note/sequence reference. If a pre-CLP digital capture is unavailable, a dry-routed R5 audio capture can be a useful proxy, but record the route/output calibration and analog conversion limits. No diagnostic firmware is assumed or requested by this analysis.

Use a small set of ordinary contrasting kicks/kits and at least a lower/higher level or velocity; do not make success depend on one special kit. Fix drv/col/ton/vol/d/w within each sweep and record their values, routing and pan. Include physical 100/90/85/80/70/50/40/0% marks and both downward/upward history. Matching electrical positions requires measurement; do not convert those marks to 0…127 preset values.

First compare unmodified R5 against **approach A only**, one initial k≈.22 and one restrained bass blend, keeping the existing idle behavior. Level-match externally for timbre comparisons and also retain absolute-level files for headroom/dynamics assessment. If it improves overlap, evaluate a 0.75 s output-tail variant separately with active processing otherwise held constant. This isolates voicing from release. No new firmware build is necessary for this first offline decision; constructing such a model/render harness requires subsequent authorization beyond this analysis.

Proposed acceptance tests, **not run here**:

| Goal | Evidence to collect / acceptance |
| --- | --- |
| Earlier useful scream | Owner can use and smoothly reach the desired feedback in the 80–90% region on the representative sweeps, without a violent onset or needing to destroy the strong-input kick. Measure spectrum/envelope and annotate listening; an exact “scream” detector is not assumed. |
| Bass through 50% | Fundamental/low-band energy stays musically useful toward 50% while feedback is present. Compare both absolute low-band level and low/high-band ratio against R5, using bands appropriate to each kick (e.g. 30–150 Hz versus .5–5 kHz). Agree a tolerable bass-loss target from listening; do not invent a universal dB pass limit. |
| Existing controls and dynamics | drv still governs distortion, col regeneration, ton brightness, vol effect level/loop interaction and d/w blend. Check minima/maxima, velocity contrast, direct-route behavior and absence of clipping surprises. No claim of identical sound is required, but unexpected coupling must be documented. |
| Smooth idle decay | Measure short-window wet RMS/peak and listen after input ends. Target an audible tail of roughly .5–1 s, without a sustained plateau followed by abrupt collapse or an objectionable click. Define the envelope threshold used; e.g. report time to -40 dB relative to tail onset and time to final quantized silence, rather than calling a ramp length an audible duration. Not every initially inaudible tail must last .5 s. |
| Eventual silence | From maximum regeneration, fader zero, missing steps and ±1-LSB quiet input, reach exact digital idle silence by a specified finite bound (initial proposal ≤1.1 s after confirmed inactivity), and remain there for several seconds. Threshold and bound are proposed acceptance criteria, not R5 results. |
| Clean re-entry | Return kicks during 50/250/500/750 ms and after full release; vary oscillator phase, level, both fader directions and sparse spacing. No attack fade, residual burst or transient should be audibly objectionable versus the fresh-entry reference. Report waveform peaks as well as listening. |
| Stereo and history | Left-only/right-only/alternating/linked input, asymmetric levels, quiet opposite side, mono consumed channel, ordinary FX/kit recall and parameter changes. Check the chosen linked behavior explicitly and do not interpret activity in one channel as global silence. |
| Practical firmware suitability | Only after a satisfactory offline result and separately authorized implementation/build: on-device headroom, CPU/underruns, attack timing and output listening under real sequencing. Host equations cannot establish these. |

The original R4 drv=63/ton=127/vol=127 history record remains distinct: starts at col=7, continues at 6, stops at 5. A linearized loop estimate here is not a new measurement of that hysteresis and does not supersede it.

## 7. Actual analysis, reproducibility and limits

Commands actually run included:

- Source reads with `nl -ba`, `sed -n` and `rg -n` on adcPots.c, main.c, adcPots.h, DrumVoice.c, Snare.c, CymbalVoice.c, HiHat.c, mixer.c/mixer.h, 1PoleLp.c, MidiParser.c, FxBus.c, Distortion.c and config.h in the available source tree. Exploratory names OnePole.c/SnareVoice.c and some directory globs did not exist; searches were corrected to actual paths.
- `python3 /workspace/scratch/clp_level_analysis.py`: completed successfully; numerical fader table, filter poles/response, smoothing time, trim estimate and small-signal loop bounds reported above.
- `python3 /workspace/scratch/clp_level_analysis.py > /workspace/scratch/clp_level_results.json`: saved the same results outside the checkout.
- Inline Python (`python3 - <<'PY' … PY`) computed return-amplitude bounds from .647479*(col/127)*.125 and the literal-7.2-Hz response ratio: 2652/146 LSB bounds at col=127/7 and +46.3453 dB at 60 Hz. Another calculation applied 1/M to the two scanned loop maxima: .7623/.04202, not nonlinear stability proofs.
- `git status --short` in /workspace/lxr-desktop-app: clean. No tracked checkout file changed.
- GitHub connector reads retrieved both reports and AGENTS.md and resolved the R5 and lab-setup heads. No CI rerun, host-suite run, firmware build, artifact download or hardware operation was performed in this task.

The temporary Python script uses standard-library math/cmath/json only. It evaluates the pinned equations in double precision; it does not execute STM32 firmware, simulate the full voice, implement a candidate or render musical audio. Its calculations are reproducible from:

```python
FS = 44140.0
# z here denotes z^-1 evaluated on the unit circle:
z = exp(-2j*pi*f/FS)
HP(b, f) = b*(1-z)/(1-b*z)
LP(b, f) = (1-b)/(1-b*z)
P(f) = HP(.3588335807986881,f) * LP(.10253532416289118,f) \
     * HP(.9914955643815428,f) * LP(.9313004164294521,f)
pole_Hz(b) = -FS*log(b)/(2*pi)
HP_relative_3dB_Hz(b) = FS/pi*asin((1-b)/sqrt(2*(1+b*b)))
dB(v) = 20*log10(abs(v))
G = 1 + 2304*63/127
# Optional illustrative loop scan: f=1..22069 Hz, col=5,6,7,127:
L(f) = P(f)*LP(1-(.06537948995454452+.9241070173442663),f) \
     * exp(-2j*pi*f/FS)*G*4.499456*(col/127)*.125
```

This is mathematical notation in Python-like form, not a shipped executable/test file. It makes the analyzed approximation explicit, including the one-sample return and approximate near-zero diode slope. The script's maximum occurs near 1874 Hz; it is not an estimate of the owner's measured scream pitch.

Temporary analysis files are /workspace/scratch/clp_level_analysis.py and /workspace/scratch/clp_level_results.json. The report contains the equations/results needed to continue without those files. Source/artifact identity checks from the independent review are reused as prior evidence, not relabelled as checks performed today.

Only this Markdown report is published on the new report-only branch based on the specified lab-setup commit. The report head is the commit containing this file. No changes are made to main, lab-setup, R4, R5, production code, tests or workflows.

## Recommended handoff

The new hardware evidence establishes that R5's idle suppression works in the owner's two reported situations. The next problem is musical level response, not re-proving that audit.

Keep the current loop as a sonic reference, audition controlled bass alongside it before retuning its high-pass, and assess a finite audible idle envelope independently. Use actual audio to choose the trim/bass balance, to determine whether feedback arrives usefully at 80–90%, and to judge release/re-entry. Implementation, tests, firmware builds and further device work all remain for a subsequent authorized task.
