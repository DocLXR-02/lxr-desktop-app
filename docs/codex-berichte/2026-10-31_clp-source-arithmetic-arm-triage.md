# CLP source arithmetic: triage against delivered R10 ARM code

2026-10-31 UTC. Repository: https://github.com/DocLXR-02/lxr-desktop-app.
Source and reserved R11 heads: **8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7**. Analysis only: **no fix, R11 implementation, new Release build, image, installation or hardware execution**.

## Recommendation and five-finding decision table

The five sanitizer findings are not five observed instrument malfunctions. All originate **before voice MIX/VOL** and reproduce independently of MIX; none is caused by moving MIX outside the effect. The already authorized snare full-source clamp is a different, genuinely amplified headroom prerequisite. R11 may make existing source artifacts interact differently with CLP, but it does not change these generators’ defect-triggering controls.

Recommend a small **target-compatible arithmetic prerequisite patch**, not a bipolar/modular synthesis rewrite: periodic row-local interpolation, defined transient ×256 scaling, and explicit unsigned conversion behavior matching this R10 binary. Keep filter work separate: its recurrence can escape to nonfinite state, and a wider intermediate alone is insufficient. An unrestricted candidate remains blocked on that filter policy; a restricted listening reference could track the filter separately **only after explicit acceptance of a tested stable/bypass domain**, without silently changing kits or disabling voices.

| Finding | Delivered ARM result / evidence | Smallest justified policy | R11 disposition |
| --- | --- | --- | --- |
| Wavetable row[1024] | Actual loads cross into the next row/object. ELF seam probes read those bytes; no physical fault is implied when flash is mapped. | Wrap the **neighbor within its own periodic row**; keep interpolation rounding. Verify every reader, including PWM’s two neighbors. | Resolve before a new candidate: illegal reads and layout-dependent sound cannot be a preserved interface. Seam sound changes require acknowledgment. |
| Nonfinite filter | Actual delivered filter code on captured production inputs reaches polynomial overflow, then NaN state. Its nominal NaN cleanup is compiled to identity. | No one-line numerical-only remedy established. Investigate a consistent bounded recurrence, or explicitly constrain the experimental domain. | Additional product/scope decision. Not introduced/amplified upstream by full MIX; unrestricted R11 blocked. A separate tracked defect requires an explicit, validated boundary. |
| Negative FM→uint32 | VCVT.U32.F32 returns zero for negative out-of-range operands; finite positive values truncate. Current target FM is one-sided at this conversion. | Preserve that behavior with an explicit finite-domain zero clamp, then conversion. Do **not** silently select bipolar signed/modular FM. | Small compatibility correction before sanitizer-clean R11 generation checks; no observed device malfunction established. |
| PWM phase overflow | Floating phase addition rounds and saturates to UINT32_MAX; it does not wrap at conversion. | Explicit bounded float-to-uint32 conversion preserving the current float sum/rounding and saturation. Integer modular PWM is a separate voicing choice. | Same: define existing target behavior before R11 validation, not an automatic PWM redesign. |
| Signed transient shift | LDRSB + LSL #8 computes intended signed ×256 in this ELF, including when volume is zero. | `(int32_t)sample * 256`, retaining gain/order and range. | Cheap source-portability/sanitizer prerequisite; no evidence of an audible scaling defect or need for saturation. |

A predictable instruction result does not make undefined C portable. Conversely, a native sanitizer failure does not demonstrate an instrument crash. Successful owner R10 listening remains useful evidence of playability, not numeric correctness across all settings. No owner WAV/matching kit is available to attribute a particular sound to these findings.

## Reference/provenance and preserved scope

Read AGENTS.md, the [all-six audit at b98e5d7](https://github.com/DocLXR-02/lxr-desktop-app/blob/b98e5d7184be245deca51891c2720035f5d8af54/docs/codex-berichte/2026-10-31_r11-all-voices-headroom-and-post-mix.md) and its scripts/data, [blocked R11 at b14e6c5](https://github.com/DocLXR-02/lxr-desktop-app/blob/b14e6c5777ac318cdcaa43d856ba48b713db1586/docs/codex-berichte/2026-10-31_r11-clp-solo-post-mix-test-build.md), [MIX design at ae39a8b](https://github.com/DocLXR-02/lxr-desktop-app/blob/ae39a8b5398da1ea36bcb0148d40c8ab8461d184/docs/codex-berichte/2026-10-31_clp-mix-vol-decoupling-design.md), and corrected R10 report at 57d4df9. Historical reports are preserved. Before work, all **73 existing remote heads** were recorded; R10 and reserved R11 matched the pin, and no completed arithmetic-triage follow-up was found.

Verified **existing** [CI run 41177267846](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41177267846), successful attempt1, push on fx/clp-multivoice-max-r10 at exact 8082cc5. GitHub metadata identifies [source/build artifact 13032864252](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41177267846/artifacts/13032864252), available/unexpired at inspection, expiry 2026-11-30. Cached downloaded bytes were reused after matching the live published digest; no equivalent ARM reference was rebuilt.

| Existing reference file | Bytes | SHA-256 |
| --- | ---: | --- |
| Outer evidence artifact ZIP | 8941901 | `e5c6ca009efb0be182fcbd73711ff1e17372be5e3194dae256157d0bfee4839a` |
| SOURCE.zip | 1754089 | `abd31c64a5cb90c4c325c5ef8c23ad787631db393f3acc2f3de1353cfb65ad19` |
| LXR_V2_STM32F765.elf | 392356 | `3cc3b78888aaacf4341f9775efcb7c675cb9e11a11954e3501b289f869418f0b` |
| DISASSEMBLY.txt | 2708055 | `d297538e69d5a694a7bfabc962130ac58bfce53d48273907a277f3452e72217b` |
| ELF map | 541488 | `aafbea4e03b9510bcb82c6e6464e0a6d3c6f227c6f4e6112e7204579abaa594c` |

The outer ZIP passes integrity checks; **every extracted evidence file matches its member bytes**. SOURCE_COMMIT.txt matches the pin, and SOURCE.zip’s exact member set and **all 410 source Git blobs** match that tree. Re-ran the existing independent downloaded-image/ELF verifier: corrected R10 image is 290088 bytes, SHA-256 `3d8670d2d9a3448452108cc3bdd7e3cae7715aa85fc811c3a5532e11e205c119`; its load bytes match this ELF. These checks establish artifact/source correspondence and integrity, not independently reproduced compiler output.

Established compiler: Arm GNU Toolchain 14.3.Rel1, GCC 14.3.1 20250623. compile_commands.json confirms Cortex-M7, fpv5-d16, hard-float, GNU11, O3, **-ffast-math -freciprocal-math** for the relevant translation units; ResonantFilter.c additionally has -fno-thread-jumps. Native audit GCC14.2/O2 is not that code generation.

[Compact provenance, scripts, captures and results](repro/2026-10-31_clp-arm-triage/) include verified-artifact.json, remote-provenance.json, results.json, capture-metadata.json and SHA256.json. [Disassembly excerpts](repro/2026-10-31_clp-arm-triage/disassembly-excerpts.txt) retain original artifact line numbers and addresses. No ELF/firmware is republished as a new candidate.

## Evidence levels and fixture fidelity

- **Source/C:** exact pinned source/control mappings and C-language rules.
- **Delivered code:** actual ELF instructions/data/layout, checked against its archived disassembly.
- **Emulated execution:** isolated functions or instruction sequences copied from that ELF, with mapped runtime sections, ABI arguments and selected inputs. This is **not device execution** or a firmware boot.
- **Native/component:** original voice paths with hardware/control shims; explicit alternative-policy mathematical probes.
- **Unknown:** physical audio, current device FP mode, full engine scheduling, actual kit-specific sound and realtime capacity.

Reused the prior **39936-case** audit; did **not** rerun it. Its selected production voice functions/macros match the build’s AMP_EG_SYNC=0, USE_FILTER_DRIVE=0, smoothing, interpolation and filter macros, 64-frame blocks and REAL_FS=44140. All three DrumVoice instances and percussion paths execute actual trigger/async/sync processing. MIX and physical poles were initialized to settled targets; these were steady-control tests, not cold-start equivalence claims.

The old fixture omitted the mixer’s per-block SVF_recalcFreq call but used the production direct setter, which computes the same coefficient for fixed controls. New native captures explicitly add the fixed-control recalc: **all five repeated input/state captures are byte-identical** to the earlier versions. MIX0 captures for hot drum/snare also exactly match MIX20 inputs/state. Thus missing cadence and MIX initialization do not explain these failures. Varying cutoff scheduling remains untested.

Eight focused captures retain the original source, ASan/UBSan/bounds/float-cast-overflow and nonrecovering diagnostics. Expected failures remain failures, not passing candidates. A wrapper records actual pre-filter data/state and calls the original filter. LFO/modulation callbacks are no-ops; ADC is a controlled target; RNG is deterministic LCG, not recorded hardware noise. A new snare case uses **ordinary UI noise-frequency value3**, in addition to the old initialization frequency440 Hz, to check control reachability. No idealized source-times-volume substitute is used as proof of production generation.

Emulator: **Unicorn 2.1.4**, Capstone 5.0.6, pyelftools 0.32; numerical probes use NumPy 2.3.5. The final scripts use the **Cortex-A15 VFP instruction model**, because the Unicorn Cortex-M7 model rejected the delivered double-precision setter instruction and lost FZ/DN control bits on conversion. Limited M7-model conversion probes agree for reset-mode operands, but its limitations preclude calling it an STM32F765 model. The A15 model executes the relevant Thumb/VFP arithmetic, including fused operations and double-precision setters; it does not model the M7 pipeline, caches, NVIC, DMA or timing. Both FPSCR reset settings and FZ+DN settings were exercised with preserved control bits. Device FP state was not measured.

## 1. Wavetable boundaries: actual bytes, periodic intent and sound differences

Source: Oscillator.c:169–174, 450–464, PWM-FM interpolation at 199–216, PWM at 671–683; corresponding scalar readers have the same boundary idiom. A 1024-row sample index can be1023 and its incremented neighbor becomes1024. SINE has a separate 4097-element table and does not share this missing row endpoint.

Reachability: MidiParser.c:248–286 sets coarse note, 294–343 fine tune; waveform selectors at 346 onward expose TRI/SAW/REC/PWM. Actual ELF osc_setBaseNote/osc_setFreq probes select row5 at coarse70/note60, row8 at coarse70/note96, and **row10 at coarse127/note60/pitch amount10 near pitch-EG peak**. The last has pitch_mod1.196416, base frequency12546.79 Hz and phase_increment1460637696: below a full-cycle-per-sample increment. Row10 is not merely an artificially unreachable array index. Component seam phases are explicitly constructed, not claimed to be recorded note samples.

Delivered loads are unmasked: calcWavetableOscBlock **0x1d54**, neighbor LDRSH at **0x1d7e** (exact listing in excerpts); calcFmBlock **0x1918**, loads at **0x196e/0x1972**; PWM neighbor load **0x1e42**. Native bounds instrumentation correctly flags the C subarray access. Mapped flash may still return deterministic bytes without a fault.

ELF layout: triTable at0x08030870, recTable at0x08036070, sawTable at0x0803b870, each22528 bytes. Rows0–9 read the next row’s first sample. Final TRI reads REC’s first sample; final REC reads SAW’s first sample; final SAW reads **CSWTCH.40**, whose first halfword is6192. Relying on this can change sound when another build changes layout.

**99 delivered-ELF seam probes** cover all33 rows at fractional0, .5 and the largest22-bit fraction. Selected last-interval results near fraction1:

| Table / row | Last | Own first | Physically read next | R10 ELF output | Periodic neighbor | Hold last |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| TRI4 | −32756 | −32767 | −32767 | −32767 | −32767 | −32756 |
| TRI10 | −32766 | −32767 | 0 | −1 | −32767 | −32766 |
| REC8 | −18 | 396 | 2879 | 2878 | 395 | −18 |
| REC9 | 2661 | 2879 | 338 | 338 | 2878 | 2661 |
| SAW4 | −5429 | 0 | 0 | −1 | −1 | −5429 |
| SAW10 | −201 | 0 | 6192 | 6191 | −1 | −201 |

These are waveform sample differences, not listening results. Periodic phase accumulation and SINE’s explicit endpoint support a row-periodic policy; they do not prove original musical intent. Wrapping the neighbor to the same row’s first sample is smallest. Appending that first sample as a guard is mathematically equivalent only with unchanged interpolation, but changes row stride/data declarations and nominally adds66 bytes across the three tables. Holding the last sample avoids the read but creates a flat last interval and a different wrap transition. Neither policy is acoustically neutral relative to all existing seam reads.

Acceptance: all33 rows, both neighbors of both PWM oscillators, boundary fractions and interior values; exact interior compatibility, declared seam exceptions, no bounds failures. Preserve arithmetic-shift/floor interpolation rounding, or explicitly define an equivalent signed64 floor calculation; do not accidentally change negative rounding by substituting truncating division. Keep the final bounded sum wide until int16 conversion. Include bright REC/high-row/PWM listening, since PWM saturation can repeatedly expose the final interval.

## 2. Filter: earliest failure and why numerical widening is insufficient

Source: ResonantFilter.c:24–27, 89–112, 250–285, 299; mixer.c:586–591 updates coefficients before async/sync voice work. Filter state is audio-owned; ordinary MIX movement does not reset it. Drum_trigger resets its filter; percussion triggers do not provide the same reset. This is not CLP feedback state.

Original C7 reaches failure with cutoff setter .9, resonance1 (q=.02), drive1 (6.4), transient peak. More decisively, C11 uses actual **cutoff UI126 through valueShaperF2F(126/127,−.9)**, resonance127, drive127, and no transient. The snare processes its noise filter even with noise mix0. The new UI3 noise-frequency setter produces519.685 Hz and retains the failure. All these operations precede MIX, envelope and physical fader scaling in the relevant paths.

ELF SVF_calcBlockZDF starts **0x275c**. Actual fused operations preserve the runaway: with ELF setters/recalc, hot drum state s1 grows to about8.1e9; the rational denominator first overflows at **0x2b16, VFMA.F32**, processed sample26 (one-based). At sample27, **0x2b26, VDIV.F32** produces NaN. In old snare C11 it occurs by sample127; with ordinary UI3 noise frequency it occurs by sample111. Native coefficient and ELF coefficient variants, and both tested FP control modes, agree on the failure class. Coefficient rounding differs by a few bits, but is not the cause.

The first problem is growth of the recurrence followed by intermediate polynomial overflow, **not an invalid input or merely the final cast**. The rational tanhXdX uses squared arguments and quadratic polynomials; for large finite state its denominator overflows before or with its numerator, then Inf/Inf produces NaN. The later int cast is only the reported sanitizer endpoint.

removeMaliciousSample at **0x0802341c–0x0802342e** contains loads/moves/return, **no NaN test**. Global finite-math assumptions remove the source `x != x` check despite its O0 pragma. Poisoned-state ELF follow-ups with zero input retain NaN state and return zero samples. This predicts a possible lost source component until state is reset; it is not a measured owner malfunction. It does not prove the CLP core’s independent idle behavior wrong.

Temporary recurrence probes, captured input followed by zeros, illustrate remedy limits:

| Diagnostic policy | Hot drum first nonfinite, zero-based | Hot snare first nonfinite | Meaning |
| --- | ---: | ---: | --- |
| Original float rational | 26 | 126 | Reproduces growth/overflow |
| Rational evaluated wider | 86 | 189 | Delays failure; float state still overflows |
| Bounded 2*tanh(state/2) | 84 | 186 | Bounding the shaper alone does not stabilize this recurrence |
| Remove state nonlinearity, keep input shaping | None in4096 samples | None in4096 samples | Changes filter algorithm/voicing; not an accepted fix |

These supplementary NumPy expression probes are not bit-exact ARM replacements or general stability proofs. Their finite checks explicitly classify failures. A large-state linearization explains the result: the hot drum map has radius~2.85 with the rational asymptote, ~2.94 with a bounded-state slope limit, versus~.989 for the linear-state map. The nonlinear state substitution is not consistent with the remaining linear-feedback solution at these coefficients. A normal LP/noise control remains finite in the tested4096 samples.

Smallest justified next filter work is a scoped recurrence/stability investigation with impulse, sustained, stop, return and coefficient-change tests, comparing intended state saturation with the whole feedback solution. No NaN-to-zero guard, reset, resonance cap, wider-only “fix,” changed coefficient or global compiler-flag change is recommended as an unreviewed workaround. The current evidence does not justify a universally safe one-line correction.

This issue is pre-existing and **MIX-independent**, so it need not turn a solo post-MIX experiment into an unrestricted filter rewrite. But tracking it separately needs a concrete accepted test boundary: e.g. kits already using FILTER_OFF, or exact finite parameter/source trajectories validated for the experiment. Do not silently turn filters off, assume a single static passing test defines a safe region, or use owner listening as a finite-state proof. Without that boundary or a separately accepted filter correction, unrestricted R11 remains blocked.

## 3. FM conversion: current target behavior is not bipolar modular FM

Source Oscillator.c:117,165,193. Drum FM amount maps linearly through MidiParser.c:483/519/556 and its pitch envelope; cymbal/hat modulator amounts map at702/706 and822/826. Modulation samples are signed int16; finite active-control examples use factors up to1. Negative products occur naturally. Conversion to unsigned is not the same operation as first converting to signed and then applying unsigned modular arithmetic.

ELF calcFmSineBlock uses **0x18a8 VCVT.U32.F32**, then integer phase addition/shift at0x18b0; calcFmBlock uses **0x1950**; PWM-FM has the corresponding sequence. The phase accumulator’s integer addition wraps, but negative products have already become zero. Probes: −32768 and−8.91→0 with IOC; −.5→0 with inexact;12.9→12;2^32→UINT32_MAX with IOC. These agree with the documented conversion semantics. A NaN probe also returns0, but it is **not** authorization for a new NaN policy.

**35 actual function probes** across SINE/TRI/REC/SAW/PWM match a finite one-sided compatibility oracle. At a selected phase and modulation−100: SINE current−29549 versus signed/modular−29815; TRI−23602 versus−25215; SAW28244 versus29056. The difference can be larger at other phases/amounts. These examples establish changed samples, not audible preference.

Minimal compatibility correction: explicitly return unsigned0 for finite x<=0, convert finite positive x only in range, retain the existing integer shift/add. Do not add signed conversion merely because bipolar FM seems conceptually natural. C operands in(−1,0) already truncate to0; values<=−1 are outside the portable unsigned conversion domain. Define and test the finite caller range, including envelope overshoot/endpoints, rather than inventing AGC or a new modulation limiter.

Acceptance: negative/positive integer and fractional boundaries, zero, modulation extrema, all five waveforms, gains and histories; compare sample/phase output to this ELF under the finite domain. Conversion IOC behavior changes when invalid conversion is avoided; no audio use of those cumulative exception bits was found, but they are not promised bit-identical. A future bipolar policy must be a separate voicing experiment.

## 4. PWM: float rounding and saturation are part of current output

Source Oscillator.c:194 and665. MidiParser.c:1357–1370 maps existing PWM controls to finite0…1. The multiplication constant UINT32_MAX/2 becomes **2^31 in float**. Phase is converted to float, the offset is added in float, then converted back. This loses low phase bits and can exceed2^32 even when the integer phase is just below the mathematical knee.

ELF calcPwmBlock **0x1e08 VCVT.F32.U32**, **0x1e14 VADD.F32**, **0x1e20 VCVT.U32.F32**. Forty-two probes compare two rows, seven phases and three PWM values. At phase0xc0000000/PWM1, target phase2=0xffffffff and output−16450; integer modular phase2=0x40000000 gives−32768. At0x80000000/PWM1, conversion saturates rather than wrapping to0. A raw phase0x7fffffff also rounds the float sum to2^32. This is not just an over-range theoretical cast.

Smallest R10-compatible policy is explicit saturation of the **existing float sum**, retaining its order/rounding. For finite x: x<=0→0; x>=0x1p32f→UINT32_MAX; otherwise cast. Returning an integer MAX before the cast matters: converting UINT32_MAX to float rounds it to2^32, so a floating clamp-to-MAX followed by cast would still be wrong. Positive phase addition in integer modular arithmetic is a credible different PWM design, but changes the saturation plateau and some in-range rounding; do not select it silently.

Acceptance: knee neighbors, largest float below2^32, raw integer phases around that knee, PWM0/.5/1 and UI neighbors, all active PWM helpers; compare both phase2 and samples. Fixing the wavetable neighbor remains a separately declared seam exception even under the conversion-compatible policy.

## 5. Transient shift: define existing scaling without new limiting

Source transientGenerator.c:60, and the analogous scalar expression at85. int8_t negative data promotes to signed int before `<<8`; the C rule does not permit this. Waveform2/3 and zero transient volume are reachable at MidiParser.c:981–999/other voice counterparts. Evaluation happens before volume multiplication, so zero volume does not make the source expression defined.

Delivered transient_calcBlock at **0x08023942 LDRSB**, **0x0802394c LSL #8**, followed by signed float conversion/multiply/convert/store, produces signed ×256. **32768 instruction-sequence comparisons** cover every signed8-bit sample and every ordinary transient volume0–127; all equal defined multiplication followed by the same float/truncation. Twenty-four actual transient function calls cover all12 tables at a negative sample with volume0/1; volume0 gives zero while phase advances normally.

Use `(int32_t)sample * 256` before conversion/gain, with int32 range[−32768,32512]. No saturation, extra gain or phase update change is needed. Preserve the same operation order and verify final generated instructions; the current instruction behavior is an arithmetic equivalence result, not a guarantee about all compiler optimizations. The active block is the priority; make any analogous unused scalar cleanup explicit in the next scope.

## Bounded consolidated prerequisite task and validation

Proposed only; no implementation authorization is inferred from this analysis:

1. **Oscillator.c:** row-local periodic neighbors in every active wavetable/PWM reader, with bounded wide interpolation and existing rounding; finite conversion helpers that preserve this binary’s one-sided FM and saturating float PWM. No new modulation policy, table normalization, source trim or UI/parameter change.
2. **transientGenerator.c:** defined ×256 scaling at the sampled transient boundary, no amplitude/phase change.
3. **ResonantFilter.c:** no unreviewed patch. Obtain explicit acceptance of a validated experimental filter boundary, or authorize a separate coherent recurrence correction and listening comparison. This is the remaining substantive scope/sound decision.
4. Then resume the reserved solo R11 implementation, retaining the approved snare-only eligible-path clamp and the **post-decimator exact-zero endpoint**. Whole-return post MIX, physical fader/Maximum/H, core/bass/dry topology, all six eligibility cases, modulation ownership, direct/multiple-route fallback and reset semantics remain as previously specified. No independent multi-voice wet stems are claimed.

Expected compatibility: transient values should match; FM/PWM conversion outputs should match the finite target domain, including its existing asymmetry/plateau; interior table samples match, while identified seam intervals intentionally change. Filter changes cannot yet be promised equivalent. Add oracle/ELF comparisons for these domains; retain sanitizers and original failure cases, then rerun the relevant full generation/integration suite only after actual edits. Tests must distinguish defined-domain equality from declared exceptions, not weaken unrelated assertions.

No persistent state or table growth is needed for the recommended arithmetic policies. Masks/branches add processing and may change ITCM placement; later code generation and one authorized build must check this. Recorded R10 ITCM 15608/16384 B leaves 776 B; DTCM 107672/131072 B, RAM 274648/393216 B, FLASH 237468 B. No new memory/stack/CPU numbers are measured here. Host timing and emulator instruction counts cannot establish device deadlines.

Smallest missing evidence for a restricted reference is the **exact intended kit/filter settings plus source/automation trajectories** and finite-state replay under their extremes, including note stops/returns. FILTER_OFF already used by a chosen kit is a clear bypass boundary; changing a kit to reach that boundary is a product choice, not preservation. Actual sound differences, FP context and performance still need separately authorized device listening/measurement later. No such action is authorized by this report.

## Commands, reproducibility and failures

The new evidence is compact scripts/results plus selected captures, not another firmware artifact. [SHA256.json](repro/2026-10-31_clp-arm-triage/SHA256.json) records members. Existing artifact download/extraction is the prerequisite; verify it before emulator use:

```sh
python3 verify_artifact.py --artifact /path/to/lxr-r10-corrected-evidence.zip --repo /path/to/lxr-desktop-app --output /tmp/verified-artifact.json
python3 -m pip install --target /tmp/clp-arm-deps unicorn==2.1.4 capstone==5.0.6 pyelftools==0.32 numpy==2.3.5
PYTHONPATH=/tmp/clp-arm-deps python3 run.py --elf /path/to/LXR_V2_STM32F765.elf --output /tmp/clp-arm-triage
```

Run these from the report’s repro/2026-10-31_clp-arm-triage directory. The runner requires the exact ELF SHA-256 and executes isolated functions/instructions only. It writes to the supplied output directory, not production sources. Checked-in inputs allow reproduction without native capture compilation. To regenerate captures, prepare the b98e5d7 audit fixture outside the checkout using its prepare.py **without --run**, then:

```sh
python3 capture_driver.py --audit /tmp/prepared-all-voice-audit --output /tmp/new-captures
```

That compiles only native component capture code and runs eight decisive cases, retaining expected sanitizer failures. capture-metadata.json records its exact command, source pin and each input hash. It adds the fixed-control coefficient update, and the optional UI3 noise-frequency setter, before the unchanged production calls. Alternative-policy probes are explicitly separate from original-source observations.

Actual workspace commands used the same scripts at /workspace/clp-arm-report/docs/codex-berichte/repro/2026-10-31_clp-arm-triage, the cached artifact under /workspace/attachments/65547d33-e1c4-4128-86cf-59704d0f0d51, and output /workspace/clp-arm-triage. Also ran the earlier downloaded-byte verifier, Git source/blob/ref checks, original eight-case native capture, selected ELF setters, and final packaged probes. The old 39936-case sweep was read, not repeated. No ARM compilation occurred, including no standalone ARM compilation; probes reuse existing instruction bytes.

Preliminary failures/limitations are recorded rather than hidden:

- The first Unicorn mem_write call received a bytearray; corrected to bytes.
- Cortex-M7 emulation rejected delivered VCVT.F64.F32 in a setter; its FZ/DN conversion-state behavior was also unsuitable. Final results use the documented A15 instruction-model boundary, not an invented M7 device claim.
- Initial linear Capstone decoding stopped at embedded literal/jump data and missed later executed filter instructions. Decoding at executed halfword boundaries corrected the trace; final results identify actual PCs and both first Inf and first NaN.
- Initial recurrence alternatives used only the first snare capture block, which was all zero before a noise phase wrap. Concatenating both captured blocks reproduces the failure; final results supersede that zero-input-only observation.
- An exploratory register read after return saw a restored callee-saved register rather than the conversion result. Final conversion observations are hooked at the conversion boundary; final waveform probes also compare an independent arithmetic oracle.
- Publication whitespace checking found an extra blank line at the end of generated disassembly excerpts; it was trimmed without changing instruction content.
- One source search used workspace-relative paths from the wrong directory; rerun at the pinned checkout. One Arm documentation fetch exceeded the web reader’s size limit; the accessible Armv7-M primary manual and ST manual were used instead.

## Primary references and publication

C classification uses [WG14 N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf), §§6.3.1.4,6.5.6,6.5.7: unsigned integer modulo rules do not generally apply to floating conversion; negative signed left shift and dereferencing a row’s one-past pointer are not valid C.

Instruction semantics use **Arm-authored DDI0403E.e**, A2.5 FPToFixed/SatQ and A7.7 VCVT ([accessible manual copy](https://www.pjrc.com/teensy/DDI0403Ee_arm_v7m_ref_manual.pdf#page=58)), and [ST PM0253 Rev6 §3.11.4](https://www.st.com/resource/en/programming_manual/pm0253-stm32f7-series-and-stm32h7-series-cortexm7-processor-programming-manual-stmicroelectronics.pdf#page=155). These specify truncation and saturating integer results/exception reporting; emulator observations corroborate the selected sequences. Compiler assumptions use [GCC14.3 optimization documentation](https://gcc.gnu.org/onlinedocs/gcc-14.3.0/gcc/Optimize-Options.html): fast-math includes finite-math assumptions, so a source NaN test is not a reliable guard under these build flags. Disassembly proves what happened in this ELF.

New report branch: **codex-reports/2026-10-31-clp-source-arithmetic-arm-triage**, based exactly on **5829398cea5d337200393757e39a37d4d8148256**. Contributions are this English report and compact reproduction/evidence files under docs. All 73 existing branch heads, historical reports, production sources, tests and workflows are preserved. Remote publication and exact file/blob identities are verified after pushing. Reserved R11 remains 8082cc5 with no implementation. There is **no new firmware, CI build, image or completed correction** from this task.
