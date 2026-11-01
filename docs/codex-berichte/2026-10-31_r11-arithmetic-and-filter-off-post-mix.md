# R11 arithmetic prerequisites and FILTER-OFF solo post-MIX reference

2026-10-31 UTC. Repository: https://github.com/DocLXR-02/lxr-desktop-app.

## Status and boundary

**Experimental R11; software/build evidence does not establish hardware capacity.** At build completion, installation/listening was pending separate owner approval. The owner’s subsequent 2026-11-01 audit request reports successful intended solo CLP MIX/VOL behavior on the device under the agreed FILTER-OFF conditions. This is owner listening evidence, not assistant-run hardware testing or comprehensive validation. This is a single-configured-FX-voice diagnostic reference, not independent multi-voice wet balancing, a filter fix, final acceptance or a frozen stage.

The owner reports R10 voice additions/removals sound plausible and playable. That is R10 listening evidence, not comprehensive hardware validation and not evidence about this R11 image. Later R11 listening is restricted to deliberately selected kits with **ALL SIX voice filters already OFF**. No code disables filters, edits kits or makes eligibility depend on filter settings. The known resonant-filter instability is retained and separately classified as unresolved. Ordinary direct/fallback snare over-range conversion also remains unresolved; the new snare saturation is intentionally limited to the eligible full-source path.

## Source and review record

| Role | Exact commit |
| --- | --- |
| Corrected R10 base | `8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7` |
| Separately reviewable arithmetic prerequisite | `d949b3e132fd8afe5f076e6465aae1ffe507c938` |
| Final R11 feature/build source | `64263ed25f2e9c7f5d880b493d943bfba29271a8` |
| Report-only base | `5829398cea5d337200393757e39a37d4d8148256` |

Source branch: **fx/clp-solo-post-mix-r11**. Two normal commits append to the reserved branch; no intermediate firmware was built. Before editing, recorded all 74 remote heads and verified reserved R11 still equaled corrected R10, with no completed follow-up implementation. Read AGENTS.md and the exact MIX design `ae39a8b`, original blocker `b14e6c5`, all-six audit `b98e5d7`, ARM triage `d0215ef`, and corrected R10 history. Historical reports/reproductions are preserved.

The owner explicitly authorized the four arithmetic corrections, the eligible snare clamp, changed low-MIX percussion dry/drive behavior and the FILTER-OFF listening boundary. Filter recurrence, state reset, coefficients, resonance limits, NaN handling, compiler flags and kits remain unchanged.

## Arithmetic prerequisite and compatibility domains

References below use the final source unless stated otherwise. DSPAudio paths are under Core/Src/Soundengine/DSPAudio.

- **Oscillator.c:26–31:** `osc_finitePhaseU32(x)` returns integer zero for finite x<=0, integer UINT32_MAX for x>=2^32, otherwise truncates to uint32. It deliberately has no new NaN recovery policy. Actual normal-control FM callers multiply signed int16 samples by finite bounded control/envelope factors; PWM adds a float phase and a finite 0…1 offset times 2^31. Selected full-generation fixtures exercise these ranges under float-cast sanitization. This is a tested control domain, not proof against every corrupt/nonfinite control or every synthesis configuration.
- **Oscillator.c:127,150,175,203,241:** FM keeps delivered R10's one-sided negative-to-zero conversion before the existing modular integer phase add/shift. No bipolar/signed FM policy is introduced. Analogous dormant sample-FM expressions use the same helper; their sample playback is not exercised by these fixtures.
- **Oscillator.c:204,675:** PWM retains float multiplication/addition/rounding and explicit upper saturation. Returning an integer MAX avoids a floating MAX rounding back to 2^32. No integer modular PWM rewrite is made.
- **Oscillator.c:184,214,220,250,474,498,687,693:** periodic neighbor `(index+1)&1023` stays within each row, including both PWM phases and scalar readers. int64 difference×fraction and arithmetic right-shift rounding are retained; the interpolated delta stays int32 until the bounded final sum becomes int16. SINE's existing explicit endpoint is unchanged.
- **transientGenerator.c:60,86:** both block scaling and the analogous scalar expression use `(int32_t)sample * 256`, retaining gain, phase and order; no saturation or amplitude change. The scalar's otherwise existing indexing/algorithm is not redesigned.

The comparison oracle is the previously provenance-checked corrected-R10 delivered ELF, SHA-256 `3cc3b78888aaacf4341f9775efcb7c675cb9e11a11954e3501b289f869418f0b`, from [CI 41177267846](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41177267846), artifact13032864252. Its cached bytes were rehashed; no R10 rebuild was performed. The triage's isolated ARM/VFP instruction-model observations are **not device execution**. Compact recorded oracles are committed with the tests and identify their original report/source.

Native corrected readers match **35/35 recorded FM samples** and **42/42 PWM converted phases**. PWM output matches 35/42 recorded samples; the other seven are declared final-row seam changes, independently checked against periodic interpolation. Example: row10 phase0x7fffffff/PWM1 changes −6191→1; row10 phase0xc0000000/PWM1 changes −32768→−32766. The old final SAW neighbor depended on unrelated flash bytes. All-row periodic tests are stronger than these selected comparisons, but none establishes acoustic neutrality. Bright REC/PWM/high-row sounds require listening. Invalid-conversion exception flags need not match R10; audio code does not use them as a policy.

## Solo integration and exact zero

New ordinary-flash helper: [SoloMix.c](https://github.com/DocLXR-02/lxr-desktop-app/blob/64263ed25f2e9c7f5d880b493d943bfba29271a8/Core/Src/Soundengine/SoloMix.c), declared in SoloMix.h.

1. **mixer.c:566–571,628:** after existing runtime target reset/velocity/LFO dispatch, coefficient updates and async voice work, snapshot the six resolved runtime `.vol` values. `SoloMix_begin` scans configured routing freshly each block. Only CLP with exactly one route is eligible; silent, muted and velocity-zero routes still count. Stored UI/kit zero does not override deliberate runtime modulation that raises `.vol`.
2. **DrumVoice.c:302–308; SoloMix.c:34–43:** source remains after voice distortion, retaining envelope/velocity/physical fader. The live MIX pole advances exactly once/frame in the eligible helper; its actual output is recorded as post gain. The physical pole also advances once at its existing cadence/coefficient. No private additional persistent smoother, double advancement or global `.vol` overwrite occurs.
3. **Snare.c:151–152,187–227; CymbalVoice.c:160,192,221; HiHat.c:168,200,229:** percussion uses the resolved block target for post MIX and effective source MIX1 for a positive target. This source passes through the existing downstream voice distortion. Physical fader, velocity, EG, filter and pan remain in the actual path. Low-MIX dry/drive behavior intentionally changes because percussion MIX moves outside nonlinear processing.
4. **SoloMix.c:45–51:** only eligible snare full-source conversion is saturated to [-32768,32767] in both velocity branches, before voice distortion. Finite upstream samples are asserted by the host wrapper, not repaired by a NaN-to-zero policy. Originally defined conversions match, including fractional values whose truncation is representable. The known first-block peak example sums oscillator2135+transient32512=34647, maximum52787; **38/64 full-source samples saturate**. This is an explicit attack/voicing exception, not unity equivalence to invalid legacy conversion.
5. **mixer.c:634,660,681,701,717,734; SoloMix.c:53–57:** target exactly0 generates no new source and gates after the unchanged decimator, blocking held-sample injection. The whole return's post gain is also exactly0, bypassing remaining drum MIX-pole release at that endpoint. The live pole still advances. No CLP, oscillator, decimator or filter history is reset. The endpoint may step; on return a frozen decimator can reveal a previously held full-source sample. There is no hidden positive threshold, envelope, normalization or activity detector.
6. **SoloMix.c:59–89; mixer.c:747:** run unchanged FxBus/CLP into a cleared isolated stereo buffer, multiply its quantized whole dry+wet return by post MIX, then saturating-add to the selected DAC destination. Thus audible core, parallel bass and dry all scale before unrelated direct voices accumulate with that return. The CLP core/bass/detector histories and lastOut are never post-scaled. Mono right destinations use the isolated mono channel correctly. Direct voices are not multiplied.

R10's physical-fader source processing, capture-before-live-advance ownership, Maximum/H(qmax), CLP coefficients/trim/nonlinear stages, VOL, D/W, linked stereo, original-input detector and short hold/fade are unchanged. FxBus.c, FX/Distortion.c, ResonantFilter.c, 1PoleLp.c, runtime parameter mappings, formats/storage and global fader law are byte-identical to corrected R10. Whole-return post MIX necessarily changes time-varying bass samples without changing bass coefficients.

Zero/multiple configured routes, other effects and directly routed voices use the **prerequisite baseline** algorithm. They share its arithmetic seam corrections, so no universal byte-equivalence to old R10 is claimed. Metadata is overwritten at each audio block and cleared after eligible synchronous processing; it is not serialized or retained across FX/kit operations. Existing audio-owned FX/kit reset handling is unchanged. Ordinary fader/MIX motion and maximum crossovers add no history resets. Eligibility/route changes are immediate and may step or carry different CLP, bass or decimator history; no reset/crossfade or immediate waveform equivalence is promised.

At settled MIX127 with matching unity MIX trajectory and DSP state, defined-domain R11 and prerequisite-baseline input, CLP state and output match. Startup/MIX transitions whose original smoother is nonunity are intentional differences, not unconditional equivalence cases.

## Actual validation and limits

Native GCC14.2/O2, -fno-strict-aliasing, ASan/UBSan/bounds/float-cast-overflow, nonrecovering diagnostics; production config macros are checked, including REAL_FS44140, 64-frame synthesis, interpolation/smoothing and AMP_EG_SYNC0/USE_FILTER_DRIVE0. This native code generation is not the ARM fast-math build.

| Check | Actual result |
| --- | --- |
| Production arithmetic | 775068 reader/PWM/transient comparisons: all33 table rows, every1024 index at five fractions, scalar/block readers, both PWM phases, all12 transient tables/all128 transient gains; finite conversion boundaries pass. |
| Recorded ARM compatibility | 35 FM sample matches;42 PWM phase matches;35 sample matches+7 declared seam exceptions. Prior exhaustive signed-byte transient instruction evidence retained; new actual production transient checks pass. |
| All-MIX full generation + actual mixer | 24576 cases:24478 pass,98 expected prerequisite-only legacy snare conversion failures. All12288 R11 cases pass. Eight configurations×six voices×both velocity modes×all128 MIX×two paths. |
| Fixed positive MIX invariance | 96 groups: identical excitation and complete MasterDistortion fingerprints across positive MIX; output level changes separately. Drum MIX-control pole is allowed to differ. Successful MIX127 baseline pairs compare exact input/state/output fingerprints; over-range snare is excluded explicitly. |
| Long transitions |360 actual-generator/mixer cases, 2048×64 frames each: zero/return with frozen decimator, squared physical-fader sweeps, returning/natural notes, MIX automation, route/FX-reset transitions, runtime modulation shim. Zero input after the real gate and eventual silence checked. |
| Focused integration |1764 cases: all128 MIX at actual velocity32/low physical fader for all six/both modes, zero-velocity configured routes, real second direct voice, six bus destinations, resolved modulation target raised despite initial zero. |
| Post arithmetic/component |648 cases covering six solo voices×six destinations×D/W0/.5/1×VOL0/1×MIX0/1/127: exact reference post arithmetic, complete CLP-state equality for identical input, direct buffer isolation/guards. |
| Eligibility/lifecycle/partitions |All64 routing masks, five FX types, three distortion types; fresh selection/fallback, no stale gain. Actual new helper/gate/return/CLP output and state match at1/7/64 partitions for both pole groups. |
| Inherited numeric CLP suite |Retained assertions:318 unity R9-reference pairs,72 moving-fader pairs, all routing masks/output destinations, zero VOL/DW, stereo/asymmetry, reset/re-entry, guards and complete state comparisons. Maximum measured idle tail36.701 ms. |
| Existing regressions |bug001, kit_load, project_storage and euclidean pass unchanged. |

The eight fixed configurations retain prior audit identities C0(SINE note36), C1(accepted snare transient peak, first64 frames), C2(FM+drive), C3(TRI/high note+drive), C6(fader0/high note), C9(max voice drive), C10(PWM1), C12(sampled transient at volume0). All primary fixtures explicitly select FILTER_OFF; C8 supplementary fixtures also bypass while retaining their actual velocity32/low-fader controls. Selected parameters cover ordinary and extreme source/transient/drive cases, not every combination. The previous39936-case audit was reused to choose decisive configurations rather than repeated wholesale.

These are real production trigger/async/sync paths for all three DrumVoice instances, snare, cymbal and hi-hat, with the actual mixer block, decimator, pan/summing and effects. Oscillator functions are extracted verbatim; sample playback is unsupported and asserted unused. HAL/jack availability, ADC, RNG, LFO/velocity/control services are deterministic shims. Real USB/MIDI interrupt concurrency, hardware noise, full modulation/morph/kit scheduling and the instrument DAC are not exercised. Static ownership/source checks and unchanged existing regression fixtures supplement, not replace, that limitation. Actual full voice synthesis stays at64 frames; partition claims apply to metadata/new return/CLP components, not arbitrary odd-sized whole-engine synthesis.

Fingerprint comparisons are complete-object byte fingerprints, not a mathematical collision-proof equivalence proof. Independent component/inherited tests also use actual byte memcmp for core state. Selected output guard checks and nonrecovering sanitizers do not establish universal safety.

### Retained unresolved failures

Active-filter C7 drum and C11 snare reproduce nonfinite ResonantFilter.c:299 conversion in both prerequisite and R11. These are recorded as failed executions, not passing FILTER-OFF cases; no filter workaround or flags change is made. The triage traces earlier recurrence growth/polynomial overflow. FILTER_OFF is an explicit later listening boundary, not proof all enabled settings are safe.

Legacy snare C1 fails from MIX79 through127 in both velocity branches in the prerequisite. R11 direct, other-effect and multi-route paths deliberately retain that conversion policy; targeted failures remain in the evidence. Only eligible full-source snare is fixed by the authorized saturation. Do not use the accepted over-range snare case on those fallback paths and interpret a successful host runner as an unrestricted firmware safety certificate.

### Paired renders

72 paired production-path synthetic renders cover voices1/4/6, MIX5/10/20/127, static controls, zero/return and physical-fader sweep, with216 output/spectral windows and24 representative WAVs. They use deliberately synthetic FILTER-OFF SINE sources and maximum CLP regeneration/controls; they are not owner recordings or a matching kit. Baseline is the separately committed arithmetic prerequisite.

For Voice6's first8192-frame static window, baseline normalized RMS is about.640 at MIX5/10/20/127. R11 is approximately.02518/.05038/.10077/.64000 respectively; peaks906/1813/3626/23027. This demonstrates post-level control in this strongly regenerative fixture, not a preferred timbre or bass balance. Low-band fractions are recorded, but the selected bright steady core dominates these windows; no bass redesign or success claim follows. R11 positive-MIX source/state invariance and unity equality are assessed separately from these audible-output metrics. Total loudness during physical sweeps need not be strictly monotonic because the core remains nonlinear.

### Commands and development failures

From the source checkout, actual commands (local output paths under /workspace/r11-final; CI uses build defaults):

```sh
python3 tests/fx_clip_r11/scope.py
python3 tests/fx_clip_r11/arithmetic.py --output /workspace/r11-final/arithmetic
python3 tests/fx_clip_r11/arm_compat.py --output /workspace/r11-final/arm-compat
python3 tests/fx_clip_r11/component.py --output /workspace/r11-final/component
python3 tests/fx_clip_r11/voices.py --output /workspace/r11-final/voices
python3 tests/fx_clip_r11/supplement.py --output /workspace/r11-final/supplement
python3 tests/fx_clip_r11/renders.py --output /workspace/r11-final/renders --baseline /workspace/r11-final/voices/prerequisite
python3 tests/fx_clip_r11/inherited.py --output /workspace/r11-final/inherited
python3 tests/bug001/run.py
python3 tests/kit_load/run.py
python3 tests/project_storage/run.py
python3 tests/euclidean/run.py
```

Final CI repeats the relevant host checks on the exact source commit before its single ARM build. The inherited runner changes only directly invalidated scope/display guards; all inherited numeric assertions and original R10 test files remain unchanged. The new scope guard protects unrelated production files and verifies modulation/capture order and the gate after decimation.

Development failures/corrections: the initial extraction regex missed a leading-space oscillator setter; corrected the fixture parser. A clamp-equivalence fixture accidentally included positive32768 and correctly failed sanitization; its defined-domain loop excludes that endpoint, with over-range saturation tested separately. The first component fixture reused its fader target as the combined-volume output pointer; separated them before final checks. A mono-right scratch-buffer indexing issue was corrected before candidate validation; all six destination checks pass. Git's default whitespace check treats existing CRLF as trailing space; checks use cr-at-eol without changing existing file line endings. No sanitizer was disabled and no unrelated numeric assertion was weakened to obtain a candidate.

## Build, generated code and resources

[CI41214348060](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41214348060), push attempt1, **success**, exact final source64263ed. One final Release ARM build only:

```sh
cmake --preset release
cmake --build --preset release --parallel 2
python3 tests/fx_clip_r11/package.py
```

Arm GNU Toolchain14.3.Rel1, GCC14.3.1 20250623, same archive SHA-256 as corrected R10: `8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd`. All121 common compile commands are identical; the only new TU is SoloMix.c. Cortex-M7/fpv5-d16/hard-float/O3/fast-math remain, with the existing ResonantFilter.c -fno-thread-jumps. CI host GCC13.3 differs from local GCC14.2; both native runs pass their specified checks. No production optimization flags or existing routine placements changed.

| Linker region | Corrected R10 | R11 | Delta | R11 free |
| --- | ---: | ---: | ---: | ---: |
| ITCM /16384 |15608|15560|−48|824|
| DTCM /131072 |107672|108184|+512|22888|
| RAM /393216 |274648|274656|+8|118560|
| Linker FLASH /1015808 |237468|238768|+1300|777040|

Linker FLASH is not the entire image load span. R11's payload is291324 bytes versus R10's290072 (+1252); initializers for RAM/DTCM and copied ITCM code also occupy flash. Physical application span is0x08008000…0x0804f1fc, leaving724484 bytes to0x08100000. The printed RAM figure includes the512-byte heap reservation; actual data/BSS is274144. DTCM free is a static gap, **not a measured runtime stack margin**. Startup/runtime SP relocation and whole-engine stack capacity require further analysis/measurement; vector SP alone is not that result.

New persistent allocation: postMix64 floats256 B + isolated128 int16 samples256 B in .dtcmz; selected4 B .data and target4 B .bss in ordinary SRAM. No new persistent smoother, history or per-voice core. The six-target snapshot is24 stack bytes. Disassembly shows mixer fixed frame unchanged at488 bytes including saved registers, before callees; its existing audio arrays remain on stack. New helper local frames include snapshot32 B, drum48 B and return64 B before callees; drum sync's fixed overhead increases16 B (64→80) plus its unchanged128-byte production VLA. These prologue observations are not a complete interrupt-nested/whole-engine stack bound or stack-capacity measurement.

New helper functions are ordinary flash at0x080269e8 onward; snapshot at0x08022a4c. ITCM uses additional flash-call veneers. calcDrumVoiceSyncBlock648→704 B, mixer block3348→3400 B; finite clamps grow individual FM/PWM readers. GCC changes wavetable-FM dispatch inlining: calcNextOscSampleFmBlock792→444 B now calls calcFmBlock; with other symbol/alignment effects total ITCM falls48 B. This is code-size evidence, not a CPU saving claim.

Generated PWM still performs float phase conversion/addition; UBFX masks periodic neighbors and SMULL retains wide interpolation. FM/PWM use explicit zero/upper-result branches, though GCC speculatively emits VCVT before some comparisons; exception flags are not promised unchanged. Transient remains LDRSB+LSL#8 (216-byte function), now sourced from defined multiplication. Snare helper branches to integer32767/−32768 before defined-domain conversion. No NaN workaround is introduced.

After downloading, isolated **actual R10/R11 ELF** probes pass35 FM samples,42 PWM outputs (seven declared seam changes),99 periodic seams,1536 transient function comparisons and nine snare clamp boundaries. Model: Unicorn2.1.4 Cortex-A15 VFP, pyelftools0.32, with the same limitations documented by ARM triage; **not device execution or timing**. Scripts/results and selected original disassembly excerpts are attached to this report.

Processing costs include the existing physical replay, six per-block source/eligibility queries, six gate checks, eligible drum post-gain stores, eligible snare per-sample clamp calls and a64-frame stereo return clear/multiply/saturating-add pass. Memory fit, native/emulator time and these local frame observations establish no Cortex-M7 CPU, interrupt deadline, stack or long-run capacity. Those remain UNMEASURED.

## Delivery integrity

Both GitHub artifact wrappers were independently downloaded and matched live published SHA-256 digests. All432 SOURCE.zip Git blobs match final64263ed; all29 manifest entries match downloaded files. ZIP integrity/membership, LXRV2IMG magic,291324-byte payload length, internal checksum0x0d, three reserved zero bytes, startup label, all126 vector entries (115 nonreserved handler entries), initial SP0x20080000, Thumb reset0x08013555, ELF entry, every application-only PT_LOAD byte and contiguous physical range were verified. Source startup checks confirm the cleared second line and unchanged bootloader update path. No existing binary was patched.

[Image artifact13047418960](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41214348060/artifacts/13047418960): lxr-r11-image;422404 bytes, SHA-256 `566e335f41546c43313f96b4648e86e7f2dacc82eb67d99e2fd2d47d49d7cbcc`. It contains the image and the owner ZIP, whose membership is ONLY the image.

[Separate evidence artifact13047144663](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41214348060/artifacts/13047144663): lxr-r11-source-build-evidence;8719802 bytes, SHA-256 `341324a0b404bfd1775c7eccc77bac4dc9686a94ac335b63b31eb67b64899d83`. Includes SOURCE_COMMIT, SOURCE.zip, ELF/map, compile commands, toolchain/archive metadata, full disassembly/loads/sections, build/configure/test logs, complete compressed host results, paired WAVs and SHA256SUMS. Available until2026-11-30 per inspected GitHub metadata.

| File | Bytes | SHA-256 |
| --- | ---: | --- |
|LXR_V2_R11.img|291340|`f1779db7861d4c9a54aded8121eb5659a445256ee64453635fcfa4fb1593d2b0`|
|LXR_V2_R11.zip|207732|`b31c93b5742732e1c280a4789b5ebde75e4354e7acf44cead54ec23a3fce8a30`|
|SOURCE.zip|1787323|`4c1b17d51fe290d0a42accadef9d7d30947b2f485910cea5c148cc838f17b9cf`|
|LXR_V2_STM32F765.elf|397620|`350696b2365c973d07b2670a57c72950a65f9a818090e3599c8487f59d313043`|
|LXR_V2_STM32F765.map|544145|`2b7edab25607c147b06fa2b98d23375732b995a74c09adaaad7261ef114d7d28`|
|HOST_RESULTS.tar.gz|6154837|`3db412a4f3353ee40452031b286c23f72f95bd2742a7df516f94a768c30bd121`|

[Reproduction and post-download evidence](repro/2026-10-31_r11-arithmetic-and-filter-off-post-mix/) include the independent verifier, ELF probes, focused pan probe, results and disassembly excerpts. A direct signed-URL download was denied by the managed network proxy; the supported file materialization downloaded the artifact successfully. No verification requirement was waived.

Additional post-build native pan checks execute864 production voice/mixer runs: pan0/64/127, all six voices, six destinations and both velocity modes. Unity equality and positive-MIX source/CLP-state invariance pass. These are separate report reproduction checks; they do not change the built source or imply a second firmware build.

## Changed files and publication

Prerequisite production changes: Oscillator.c, transientGenerator.c. Feature production changes: new SoloMix.c/SoloMix.h, DrumVoice.c, Snare.c, CymbalVoice.c, HiHat.c, mixer.c, one Core/CMakeLists.txt source entry and the single normal splash literal in main.c. Normal first line is **LXR LAB R11**, second remains cleared/blank; bootloader messages, splash timing and FIRMWARE_VERSION are unchanged.

Tests/reproducibility: tests/fx_clip_r11 contains bounded source guards, arithmetic/oracle checks, native full voice fixture/build driver, selected-MIX/transition/supplement tests, post-return component tests, inherited runner, paired renders, package validator and README. One new branch-specific workflow .github/workflows/clp-r11-build.yml runs them and the established regressions, builds once, validates and uploads separate artifacts. Existing workflows/tests are unchanged.

Report-only branch: **codex-reports/2026-10-31-r11-arithmetic-and-filter-off-post-mix**, based exactly on5829398. Only this English report and directly relevant reproducibility/evidence summaries are published there. Source branch has the two normal commits above. All73 other pre-existing heads, main, lab-setup, R4–R10 and historical reports are preserved; remote heads and exact report blob are verified after publication. No merge, force push, release, setting change, flashing or assistant-run hardware test occurred.

## Focused later listening, only after separate approval

Select existing kits with **ALL SIX voice filters already OFF**, checking all six even when silent; do not silently change kit settings. Start exactly one CLP-routed Voice6 at full physical fader and MIX5/10/20/127, compare timbre and selected level to preserved corrected R10, then sweep physical fader, zero/return and D/W. Repeat drum/snare, including the accepted saturated snare peak only in eligible solo operation. Include bright wavetable/PWM sounds to assess periodic seam changes. Listen for attack/drive differences, bass/dry scaling, zero-endpoint steps, held samples on return, immediate routing-policy steps and switching back to multiple-route R10 processing. Assess output level and timbre separately.

No further installation/listening is authorized by this handoff. Device CPU/deadline/stack/thermal measurements and final acceptance remain pending; the subsequent owner’s limited listening observation does not close those gates. Filter correction, parameter/bass refinement, independent multi-voice wet stems and freezing the development stage are outside this task.
