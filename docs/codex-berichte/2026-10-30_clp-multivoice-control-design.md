# CLP multi-voice output-control design, preserving R9

Date: 2026-10-30 (UTC). Repository: https://github.com/DocLXR-02/lxr-desktop-app.
Reviewed source: **e2d7963d97cd2d5918e3b80b88156fbbfaa60f47**, **fx/clp-fader-curve-r9**.
Report-only base: **5829398cea5d337200393757e39a37d4d8148256**, lab-setup.
Report branch: **codex-reports/2026-10-30-clp-multivoice-control-design**.

## Recommendation and unresolved product boundary

Prefer **maximum mapped fader gain** for a minimal, separately authorized multi-voice prototype. It retains the approved R9 solo curve, keeps its collective half-travel knee independent of voice count, and makes adding a zero-fader route neutral. Replay the existing live smoothers privately and alter only the already separate audible core gain. Keep the shared core, bass, dry path and detector unchanged.

**A material compromise must be accepted before treating this as the successful stage to freeze:** a configured but silent high-fader voice still holds the shared gain up. Maximum gain removes the automatic *route-count* switch to unity; it does not distinguish silence from playing. Neither position-only policy examined here meets the stronger requirement “follow only currently sounding contributors, regardless of other routed fader positions.” In that counterexample, the quieter playing voice can again expose unattenuated feedback until the unchanged inactivity detector closes after its input disappears. Eventual silence is retained, but the approved solo-style fade of that voice is not retained.

Thus the recommended next implementation task has a clear position-based contract: **all routed faders can collectively fade the shared core; an individual fader removes that voice's new excitation but cannot independently remove its processed contribution or control the whole return while another routed fader remains higher.** If the owner requires the stronger activity-based contract, that product decision is unresolved and this minimal prototype should not be presented as satisfying it. Do not silently add activity tracking to resolve the difference.

No firmware candidate was implemented or built. Existing sources/tests are unchanged. Only this report and two offline reproducibility scripts are published.

## Evidence, prior work and permissions

Read root AGENTS.md and the [R9 build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-30-r9-clp-fader-curve-test-build/docs/codex-berichte/2026-10-30_r9-clp-fader-curve-test-build.md). That report's installation-pending status was historical. **New owner DEVICE evidence:** R9 has now been tested; effect and fader response are very close to the desired result, and the owner is satisfied. This establishes the sound/solo-response reference for this investigation. It is owner listening evidence, not a new comprehensive validation of every R9 path. Bass balance and parameter refinement remain deferred.

Before experiments, enumerated all **65 existing remote branch heads**, searched local filenames, fetched the report refs, and searched the contents of **11 CLP/R5–R9 report branches** for multi-voice, aggregate and fader-policy work. No completed multi-voice implementation/design report was found. The R8/R9 reports explicitly retain the zero/multiple-route unity fallback. Remote R9 and lab-setup heads matched the requested commits. The source checkout was clean before and after the experiments.

Authorization here is analysis and limited native experiments only. No production/test/workflow file was edited; no ARM build, firmware installation, hardware test, release, merge, force push or setting change occurred. Previous build/package audits were not repeated.

## Confirmed production path

References below all use pinned R9, not a proposed implementation.

| Stage | Source reference and consequence |
| --- | --- |
| Physical control | Core/Src/IO/adcPots.c:256–277 normalizes ADC travel u, snaps u<.01 to zero, stores u², and retains hysteresis/polling. main.c:966 polls faders in the foreground. Position is not measured audio activity. |
| Existing smoothing | DSPAudio/DrumVoice.c:54–55, 306–308 uses r=.01 for voices 1–3. Snare.c:36–37, 196/221; CymbalVoice.c:37–38, 205/234; HiHat.c:37–38, 213/241 use r=.1 for voices 4–6. Sync processing advances each voice's filter once per frame, including silent voice paths. Voice processing and placement of its fader relative to voice distortion differ; a synthetic source×q model is not the entire six-voice engine. |
| Smoother arithmetic | Core/Src/Soundengine/DSPAudio/1PoleLp.c:27–32 calls fmaf on b/r/input and stores the float result in b. This is the function replayed, not an independently invented smoother. |
| Configured routing | mixer_audioRouting[6], mixer.c:36; enum in DSPAudio/mixer.h:30–39. MidiParser.c:1313–1320 writes each route. FX-bus route is returned directly by mixer_checkOutJackAvailable, mixer.c:98–102; physical output-jack fallback does not turn a direct route into an FX route. |
| Capture ownership | mixer.c:549–562 gathers actual filter pointers and squared targets for all six voices. Lines 618–622 capture before sync voice processing; line 732 passes the frame array synchronously to FxBus. FxBus.c:51–65 counts configured FX routes, copies the sole filter, and replays it privately. It does not test velocity, amplitude, mute or current notes. |
| Summing and clipping | mixer.c:439–467 pans each mono voice buffer, converts components to int16 and sequentially saturating-adds them into one interleaved stereo fxBusData buffer with __QADD16. Voices are mixed in order, then FxBus processes the combined buffer once. USE_TRIGGERED_PAN=1 in mixer.h:22 selects cached triggered pan gains in the production call sites; the harness uses explicitly synthetic fixed pans. Pre-FX saturation can make sums order-dependent and loses recoverable per-voice information. |
| Shared bus/state | FxBus.h:42–55 holds one MasterDistortion, not one per routed voice. FxBus.c:78–102 selects stereo or mono output from fxBus.audioRouting; line 141 calls the distortion once. Distortion.h:33–61 stores per-channel filters/lastOut/bass plus a linked inactivity state, with no voice identities. Mono CLP consumes the original left interleaved sample; a panned-away contribution may therefore be absent in mono even though its configured fader still counts. This existing behavior is not changed here. |
| Core input and feedback | Distortion.c:498–511 derives bass from original consumed bus input, trims excitation by .22, detects inactivity on the original int16 bus and adds unattenuated lastOut feedback. Existing HP/LP, drive, slew/rails, diode and tone follow at 535–625. The mono equivalent is 645–688. |
| Audible gain | Distortion.c:625–636 / 688–693 computes lastOut first, then wet=lastOut×gain+bass, followed by the existing dry/wet blend and output accumulation. The added gain is never written into history. Bass is the unchanged 150 Hz LP contribution with weight −.25 and existing VOL, lines 202–216. Dry is the original already summed/clipped bus input. |
| Idle and reset | Distortion.c:157–179 queues/owns CLP history reset at audio entry. Lines 182–199 detect either consumed channel exceeding ±1 LSB, hold 441 frames then reduce feedback over approximately 222 float-subtraction steps. First usable returning input reopens immediately. Stereo activity is linked. FxBus.c:172–175 queues resets on FX/kit preparation; presetManager.c:1164–1166 starts recall that way. Recall remains an interleaved parameter resend, not an atomic final-state transaction. |

Soundengine.c:81–90 disables TIM2 update while foreground audio processing runs, preserving the existing single-audio-writer arrangement; ADC/control work occurs in its existing service paths. Retaining that contract is essential for coherent snapshots. This is source reasoning, not a new executed interrupt-interleaving proof.

There is only one nonlinear wet result per consumed channel. In general:

```text
shared_core(xA + xB, shared_history) != core_A(xA) + core_B(xB)
```

Cross-modulation, clipping and regenerative history depend on the combination. A global output multiplier cannot recover or separately fade “voice A's distorted stem.” At A's fader zero, new A input/dry and its input-fed bass tail disappear, but its previous influence on shared history need not disappear while B continues exciting the loop. Even where the bass filter is linear, its input is the already clipped bus sum; it is not generally a sum of independently recoverable unclipped stems.

## Policies compared

For routed set R, let qi[n] be voice i's private replayed **smoothed squared** fader value for frame n. Define bi=clamp(qi,0,1) and the existing R9 map:

```text
h(q) = 0                  q <= 0
       2*sqrt(q)          0 < q < .25
       1                  q >= .25
```

All policies apply a single linked gain to **audible core only**, with wet=h_control×lastOut+bass; none changes feedback or bass/dry. For an empty routed set the comparisons retain R9's **unity fallback**, not a new no-route gate.

| Policy, nonempty R | Exact formula | Behavior |
| --- | --- | --- |
| Pinned R9 | h(qi) for exactly one route; otherwise 1 | Adding even a zero/silent second route immediately restores unity. |
| Maximum, preferred prototype | M=max(i∈R) h(bi) = h(max(i∈R) bi) | Highest smoothed fader position controls the shared return. Lower faders do not cumulatively hold it higher. A new zero-fader route is neutral. |
| Bounded squared-sum | S=h(min(1, Σ(i∈R) bi)) | Root-sum-square of fader positions below saturation. Several low faders keep more shared output. It is not actual signal energy, a normalized mean, or min(1,Σh(qi)). |

For settled physical positions, maximum is min(1,2×max ui); sum is min(1,2×sqrt(Σui²)). Both exactly reduce to R9 for one routed voice and both are bounded. M≤S. Because h saturates already at .25, clipping the sum at 1 only bounds its argument; the audible saturation condition is **Σui²≥.25**.

| Settled physical positions | M | S |
| --- | ---: | ---: |
| One .25 | .500000 | .500000 |
| Two .25 | .500000 | .707107 |
| Six .25 | .500000 | 1.000000 |
| Two .20 | .400000 | .565685 |
| Six .10 | .200000 | .489898 |
| .40 and .20 | .800000 | .894427 |
| .20 and zero | .400000 | .400000 |
| .20 and silent .10 | .400000 | .447214 |
| .20 and silent .80 | 1.000000 | 1.000000 |

With N equal faders, M's knee stays u=.5; S's knee is u=.5/sqrt(N), approximately .354 for two and .204 for six. Sum therefore retreats toward the early-loss-of-attenuation behavior as route count grows. A silent low-fader route also raises S, although it contributes no audio. Maximum better preserves R9's approved collective travel and is less sensitive to irrelevant lower positions. Neither is an audio-level normalizer.

For fixed nonempty routing and continuous qi, both gains are continuous, including when two maxima exchange dominance. Maximum has a slope change there; neither needs a new persistent smoother. A route-set change can still step gain: adding/removing the highest fader changes M immediately. Removing all routes retains the old unity fallback and can briefly expose a residual idle tail. That exception must be listened to and tested; changing the empty-set policy would be a separate explicit decision.

## Silent high fader and individual fading

Consider q=(.64,.04), corresponding to physical positions (.8,.2). It may represent either (A silent, B playing) or (A playing, B playing). Fader/routing metadata is identical in both worlds, so a position-only policy cannot decide differently. Both M and S are unity, despite B's solo R9 gain being .4. If B fades while A remains high but silent, neither aggregate follows B's audible-core fade. When all actual input vanishes, the unchanged R9 detector still suppresses feedback eventually; it does not provide the missing continuous output fade during B's low input.

If A's fader is also lowered, M/S can fade the common core smoothly and R9's idle mechanism remains the final silence backstop. “All routed faders down” and “all currently sounding contributors down, other silent faders untouched” are different product contracts. This distinction is not solved by merely counting notes or ignoring a silent route.

While B continues playing, fading A can achieve removal of new A excitation and its incoming dry/bass contribution. Maximum may remain unchanged while A is below B, then change the **whole shared** return if A becomes dominant. Sum generally changes the whole return whenever A's position changes below saturation, including B's audible core. Both retain nonlinear timbral interaction from changing A's input. Neither promises that B's timbre or level remains fixed, or that A's prior regenerative influence is separately faded.

## Why activity weighting is not the minimal recommendation

No signal-dependent policy was implemented or tested. It could address the silent-high-fader distinction only by adding information and behavior absent from R9:

- Per-voice pre-sum energy/envelope tracking needs sampling points, thresholds, time constants and rules for quiet tails. Post-sum measurement cannot identify which voice contributed. A fader high with a low-velocity sound, a long natural decay and an absent note are different cases.
- Hard activity selection can change gain between sparse steps or during returning attacks. Smoothed selection can pump/lag and introduces a new release interaction. The desired regenerative sound may outlive its excitation; reducing its gain because an envelope fell below a threshold can suppress exactly the scream/whistle being preserved.
- Signed weighting can cancel between voices/channels. Absolute/RMS weighting avoids signed cancellation in the weights but still misrepresents phase cancellation and pre-FX clipping in the actual bus. Ratios based on small sums can be unstable and need bounds/extra rules.
- Mute/velocity/note flags do not reliably describe current audio or filter tails. Unmuting a high fader could suddenly promote it; crossing a threshold during a returning note could alter the shared attack. Stereo activity decisions must remain linked, without new pan-dependent pumping.

The existing original-bus detector is a deliberately separate idle mechanism, with owner-approved short behavior. It should not be repurposed as a per-voice dominance detector. Activity tracking may be investigated later if the stronger contract is required; it is not assumed necessary or musically beneficial here.

## Compact native comparison: method and results

Reproducibility scripts: [compare.py](repro/2026-10-30_clp-multivoice-control/compare.py) and [compare.c](repro/2026-10-30_clp-multivoice-control/compare.c). They export the exact pinned commit with git archive to a temporary directory outside the source checkout. They compile unchanged production Distortion.c, FxBus.c and 1PoleLp.c, plus the **verbatim extracted production mapper and mixer adder**. Existing host intrinsics/header shims are used. The aggregate formulas exist only in this offline harness.

Synthetic six-voice fixtures have fixed pans, postulated source×smoothed-q samples, sequential actual saturating mixing, and actual R9 core/bass/detector. They are not real kits or the complete voice engine. The jack resolver is stubbed to return the requested destination. Both mono and asymmetric stereo are tested. Targets are refreshed every 64 frames, with a piecewise fader profile: held until .2 s, faded to zero by .7 s, held to .9 s, returned by 1.2 s. This omits physical ADC quantization/hysteresis/snap and is not a calibrated fader movement recording.

Eleven scenarios: sole voice; two unequal faders fading the lower or higher; silent high/zero/low extra fader with identical sounding input; collective two-voice fade/return; sparse alternating notes; a six-voice mixture; route changes; and a deliberately hot six-voice sum. Kick-plus-low-level periodic excitation probes regenerate under the existing controls; they do not establish a particular acoustic whistle. Parameters are comparison fixtures, not proposed refinements: drv/col/ton/vol=63/7/127/127 and maximum 127/127/127/127. Each run has 70656 frames (~1.601 s), including final silence.

**Passed:** 44 principal scenario/parameter/channel runs plus one six-voice zero-wet run, using Debian GCC 14.2.0, NumPy 2.3.5, -O2, -fno-strict-aliasing, ASan/UBSan/bounds/float-cast-overflow and nonrecovering failures.

Internal evidence, separate from audio metrics:

- **6,217,728 main-run complete MasterDistortion memcmp checks** compare M and S against R9 after every processed frame. Feedback, filters, slew, detector and bass histories are identical for identical incoming samples. All checked histories/outputs are finite.
- Replay leaves complete live filters untouched. Advancing the fixture live filters afterwards matches each private qi exactly. The sole-voice gain and resulting output match production R9 exactly.
- Captured input/gain sequences produce sample-identical output and final state across **1/7/64-frame DSP partitions** for all policies/runs. Metadata itself is captured in 64-frame blocks; this does not claim a separate full mixer replay-cadence audit.
- The silent high/zero/low variants have byte-identical consumed bus input, core/bass trajectories and R9 output. Differences in proposed output arise solely from policy metadata. Zero-wet output is unchanged across all policies.
- Every final .1 s is exact quantized silence. Worst settling after last nonzero consumed input is **35.523335 ms**, for each policy in this matrix. No persistent idle output or sanitizer failure occurred. This is a synthetic bound, not a universal hardware deadline.

Selected normalized output RMS, stereo, 63/7/127/127. Windows are .10–.18 s (settled initial positions) or .40–.48 s (fading). RMS includes both output channels, core plus unchanged bass at 100% wet:

| Scenario / window | R9 gain / RMS | Maximum gain / RMS | Sum gain / RMS |
| --- | --- | --- | --- |
| Unequal .4/.2, initial | 1 / .428082 | .799998 / .342463 | .894425 / .382886 |
| Fade lower, fading | 1 / .427634 | .799998 / .342103 | .827392 / .353823 |
| Fade higher, fading | 1 / .419513 | .428068 / .179998 | .581116 / .244072 |
| Silent .8 + sounding .2, initial | 1 / .416499 | 1 / .416499 | 1 / .416499 |
| Silent 0 + same sounding .2, initial | 1 / .416499 | .399999 / .166592 | .399999 / .166592 |
| Silent .1 + same sounding .2, initial | 1 / .416499 | .399999 / .166592 | .447213 / .186257 |
| Both fading collectively | 1 / .419524 | .420769 / .177232 | .496190 / .209002 |
| Sparse/alternating .2/.15, fading-time window | 1 / .416658 | .399999 / .166658 | .499999 / .208325 |
| Six-voice mixture, initial | 1 / .423237 | .599999 / .253947 | .953938 / .403742 |
| Six-voice mixture, fading | 1 / .417973 | .315576 / .132415 | .501136 / .210286 |

In the silent .8/.2 initial window, internal core RMS is the same **0.416512** whether the silent fader is high, low or zero; the complete-state assertions are stronger than this rounded metric. Audio-only activity has not been inferred from the fader position. At .75–.82 s the collective/two/six fades are silent and return notes produce valid output again. Sparse-note gaps do not change M/S selection; the unchanged R9 idle mechanism can still close/reopen the core during gaps.

Route fixture: adding a silent **zero-fader** route at frame 13248 (~.300136 s) changes R9 gain .399999→1, while M/S stay .399999. Raising that silent fader later to .4 makes settled M=.799998 and S=.894425 despite no new input from it. Removing it restores .399999; removing all routes at frame 44160 restores unity for all policies. Those transitions can have audible level steps and are not promised click-free.

All principal policies reach a peak of at most **27535 LSB** and maximum adjacent-sample step **37050 LSB** in this matrix. These waveform steps are not isolated control-click measurements. The hot mixture has up to **115820 per-add/channel mixer saturation events** per run; this intentional fixture confirms that upstream clipping remains shared and cannot be repaired by a downstream aggregate gain. There is no new limiter/normalization. Bounded gain does not imply strictly monotonic total RMS or per-sample output magnitude: bass/dry phase cancellation can change as the core multiplier changes.

No musical superiority follows from these RMS tables. The evidence demonstrates gain behavior and internal invariance; owner listening must assess the shared scream, fades, transitions and attacks with real kits.

## Architectural alternative: independent wet contributions

Separate nonlinear cores per voice would retain a distinguishable processed stream that could be attenuated independently. It would also replace F(xA+xB,history) with separately evolving results, losing shared intermodulation and changing the multi-voice feedback instrument. It is not a transparent extension of the approved shared R9 sound.

The native MasterDistortion object is **176 bytes**: six equivalent instances nominally require 1056 bytes, +880 over one, before buffers/control bookkeeping and target-ABI verification. Code could be reused, so six instances do not imply six copies of instruction text, but processing may approach six core executions per frame plus summing. Mono/stereo topology, filters, detector/reset ownership, route moves and kit recall would all need new design/testing. Recorded R9 memory fit (ITCM only 776 B free; DTCM 23400 B free) proves no device CPU/stack/IRQ capacity for this alternative. No ARM build or timing measurement was made here. It is outside the recommended scope.

## Focused next task and listening plan — proposed, not performed

Subject to explicit acceptance of the silent-high-fader compromise, implement **M=h(max routed qi)** from pinned R9 in an isolated prototype branch:

1. Retain existing capture before live voice advancement. Copy each configured routed voice's actual b/r and replay production calcOnePole once per frame on private state. Aggregate bounded qi, then invoke the unchanged h once per frame. Do not update live filters twice, interpolate endpoints, add a persistent smoother or infer activity.
2. Keep exactly-one behavior sample-identical to R9; for two through six routes use M instead of route-count unity. Keep zero-route unity fallback explicit. Keep stereo gain linked and capture CLP-only. A silent route is included by position; it is never secretly removed.
3. Keep DSP feedback/history, excitation/voicing, bass/dry, detector/short timing, controls, global fader law, storage/formats and UI unchanged. Reuse the existing gain buffer. No reset on fader movements or dominance crossover. Use fresh snapshots after route/FX/kit transitions; preserve audio reset ownership.
4. Expect up to six smoother replays per frame instead of R9's one (solo) or none (multi fallback). Six OnePoleFilter copies are nominally 96 bytes of temporary state; do not claim stack fit or realtime performance from that estimate. Maximum before mapping permits one sqrt at most per frame, rather than mapping six times. Device cycle/deadline/underrun checks remain necessary in a later authorized evaluation.

Proposed acceptance checks: actual production helper and integration; all six solo cases matching R9; all 64 route masks and output routes; silent high/low/zero extra routes, muted/velocity-zero but configured voices; collective fade to quantized silence; returning/sparse/asymmetric signals; max-dominance crossovers; route-set gain steps/empty fallback; no stale gain after FX/kit recall; complete identical-input DSP histories, dry/bass, VOL/D/W and other effects; 1/7/64 metadata and waveform consistency, guards/finite state/sanitizers and relevant regressions. A future separately authorized firmware task must check resource fit with the established toolchain, then obtain separate installation approval. None of those future implementation/build/device checks is claimed here.

Focused owner listening should compare pinned R9 solo first, then two voices at unequal levels: fade the lower voice, fade the dominant voice, lower both together, repeat with a silent high-fader route, and repeat with sparse/alternating notes and a six-voice kit. Listen for loss of the approved solo transition, common-return level changes, residual scream at low input, returning-note softness and routing/kit steps. Confirm whether keeping an inactive high-fader voice “in charge” is acceptable. If not, stop and settle the activity/architecture requirement rather than masking it with a new gate. Freeze the successful stage only after this contract and listening result are accepted; no parameter or bass refinements belong in that task.

## Reproduction, actual commands and failures

The two scripts total **186 lines / 12708 bytes**. Run from any clone containing the pinned commit; output must be outside that clone. Python 3.12+ supports the archive extraction used; native GCC and NumPy are required. The scripts perform no network calls or source writes.

```sh
git fetch origin fx/clp-fader-curve-r9
python3 -m pip install numpy==2.3.5
python3 docs/codex-berichte/repro/2026-10-30_clp-multivoice-control/compare.py --repo /path/to/lxr-desktop-app --output /tmp/clp-multivoice-results
```

Actual analysis invocation:

```sh
python3 /workspace/clp-multivoice-evidence/compare.py --repo /workspace/r9-source --output /workspace/clp-multivoice-evidence/results
```

Other actual checks: git status --porcelain=v1; git ls-remote --heads origin; git fetch origin 'refs/heads/codex-reports/*:refs/remotes/origin/codex-reports/*'; git ls-tree/git show over the report refs; rg/nl source reads; git diff --exit-code. The runner executes git archive of the exact pin, compiles unchanged DSP/bus/pole plus extracted mixer/map with native sanitizer flags, checks state and partitions, and emits results.json, metrics.csv, six gain CSVs and **18 WAVs** (six scenarios × three policies). No binary/WAV is committed to the report branch; they are reproducible outputs outside the checkout.

SHA-256:

| Evidence | SHA-256 |
| --- | --- |
| Published compare.c | `4552329e44e319d0a91cce76be738a595d814252f7006dd23f54219fc6009a8d` |
| Published compare.py | `581e6dc1c1f8c128e4ae3ff7cbf22803a4cd6029b775d98974ebcbc604ebeadd` |
| Local results.json | `c787d5664a59432360cce2dabc5b51ca6862b20a9089a18c227207986cdd85a9` |
| Local metrics.csv | `8ae71eeaff9e04131cf611b971c73a63e56bba032c093984de1b58b258cfb43b` |

results.json records compiler/NumPy versions, full compilation command, pinned commit and SHA-256 of the four production files used. Its temporary paths vary on reproduction, so its hash is an identity of this run, not a promised cross-machine hash. Host floating-point/compiler differences may also affect exact measurements. Logs and full outputs remain under /workspace/clp-multivoice-evidence outside source.

Failed attempts: initial exploratory reads used nonexistent tests/fx_clip/include/main.h and a differently capitalized soundEngine.c; actual shim/source paths were then located. The first comparison completed its C assertions/renders but its Python counterexample summary incorrectly compared differently shaped arrays and required unity during the cold-start smoother ramp. The corrected assertion requires both proposed gains to be unity after .02 s settling. The complete compact comparison was rerun to produce final results; no DSP/production code or unrelated assertion was changed. Logs: host-attempt1.log and final host.log. No native compile, DSP state/partition, finite-output, sanitizer or final idle assertion failed.

All **65 pre-existing remote branch heads** are preserved; only the new report-only branch is added. Its diff from exact lab-setup consists of this Markdown report and the two scripts under docs/codex-berichte/repro. No source-candidate changes or inherited tests are published on it. Remote report contents/branch comparison are verified after publication; the final handoff provides the exact URL and branch.
