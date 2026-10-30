# R9 experimental CLP delayed-onset output attenuation

Date: 2026-10-30 (UTC). Repository: https://github.com/DocLXR-02/lxr-desktop-app.

**R9 is experimental and not hardware validated. Installation and listening remain pending separate owner approval.** No flashing or hardware testing occurred in this task.

## Identity and delivery

| Item | Identity |
| --- | --- |
| Exact source base | `7681da435cb5a70be3b08654227b5183acec1053`, R8 / fx/clp-single-voice-fader-r8 |
| New source branch | `fx/clp-fader-curve-r9` |
| Published source head | `e2d7963d97cd2d5918e3b80b88156fbbfaa60f47` |
| CI | https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41120104018 |
| CI job | 124284355851, validate-and-build |
| Image artifact | [lxr-r9-image, 13008391709](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41120104018/artifacts/13008391709) |
| Separate complete source/build evidence | [lxr-r9-source-build-evidence, 13008241923](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41120104018/artifacts/13008241923) |

CI run 41120104018 completed **successfully**, attempt 1. Artifacts were unexpired and available at verification; recorded expiry is 2026-11-29.

| Delivered file | Bytes | SHA-256 |
| --- | ---: | --- |
| **LXR_V2_R9.img** | **290008** | `b128d5387f421f9e20a64850f6aeac179770dbe12a2382a3fdd7e8845591516c` |
| **LXR_V2_R9.zip** | **206719** | `16ec895c7610bf1c19ed60ddf4fa5e940f06ec041e0fa5a4003bc2f7864c7db3` |

The named LXR_V2_R9.zip contains **only LXR_V2_R9.img**, byte-identical to the separately delivered image. GitHub's image artifact ZIP is a separate outer wrapper containing both delivery files.

SOURCE.zip: **1732826 bytes**, SHA-256 `ad0726ab967ff18613814c898fc9a9b78580cd2732b35b381d20eda8d05b052c`.

Downloaded outer artifact ZIPs match the digests published by GitHub upload metadata and logs:

| Wrapper | Bytes | SHA-256 |
| --- | ---: | --- |
| lxr-r9-image | 420323 | `aac11dee2f7c07bb7642040635b2a2fc78dfd1e4df3784d2a9d97f5e09ec6c88` |
| lxr-r9-source-build-evidence | 4357840 | `e004c11ca9c3f0b4d5e7bb20e5ec332c5fb42474710a1f5154277df5135e56b5` |

## Objective, prior evidence and scope

Read root AGENTS.md and the [R8 handoff](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-30-r8-clp-single-voice-fader-test-build/docs/codex-berichte/2026-10-30_r8-clp-single-voice-fader-test-build.md). The task's explicit pinned source base controls; the report-only branch uses the separately specified lab-setup base.

**Owner-supplied DEVICE evidence for R8:** added attenuation works and is the right direction, but sound becomes quiet too early. The owner wants more physical travel through distortion, scream and steadier whistle. Controls work well together, and adding a second routed voice behaves as previously explained. Bass falls away more than desired; bass adjustment is expressly deferred. These observations do not validate R9.

Before edits, `git branch --list '*r9*'`, a workspace filename search and `git ls-remote --heads origin '*r9*' '*r8*' 'lab-setup'` found no existing R9 work. The remote R8 head and lab-setup base matched the requested commits. The new worktree was created directly from pinned R8. Remote comparison after publication confirms **one commit ahead, zero behind, merge base exactly pinned R8**.

This task changes only R8's additional audible CLP-core output-gain curve and splash. It introduces no time delay, long release, persistent smoother, bass compensation, control, serialization change or multi-voice policy.

## Implemented mapping and source references

All source references below refer to head `e2d7963d97cd2d5918e3b80b88156fbbfaa60f47`.

Core/Src/Soundengine/FxBus.c:42–48:

```c
static inline float FxBus_clipCoreGain(float q)
{
    if(q <= 0.f) return 0.f;
    if(q >= .25f) return 1.f;
    return 2.f * sqrtf(q);
}
```

Line 64 maps **every** result from the existing private replay:

```c
gains[frame] = FxBus_clipCoreGain(calcOnePole(&replay, targets[sole]));
```

This implements `min(1, 2*sqrt(clamp(q,0,1)))` for finite smoother values. The early unity branch also bounds values above one without evaluating sqrt. At steady state q=u², so the knee is **q=.25, physical u=.5**, not q=.5.

| Physical travel u | Steady q | Additional audible core gain |
| ---: | ---: | ---: |
| 100% | 1 | 1 |
| 75% | .5625 | 1 |
| 50% | .25 | 1 |
| 40% | .16 | .8 |
| 25% | .0625 | .5 |
| 10% | .01 | .2 |
| 0% | 0 | 0 |

The production mapping passes these points within 1e-7 float tolerance. The underlying ADC law remains unchanged: adcPots.c:256–277 normalizes the ADC, snaps travel below .01 to zero, stores u² and retains its existing update hysteresis. The theoretical table is not a new physical calibration; near-zero physical travel is still subject to that existing snap and quantization.

### Ownership, smoothing and gain placement

Core/Src/Soundengine/DSPAudio/mixer.c:549–562, 618–622 and 732 are byte-identical to R8. They capture all six actual filter pointers and existing squared targets before any voice advances its live fader, then synchronously pass the mapped frame array to the FX bus. FxBus.c:51–65 retains route counting and copies the sole live filter's **b and r** into a local replay. The live filter is never advanced or modified to obtain gain metadata.

Voice code and 1PoleLp.c are unchanged. Voices 1–3 use r=.01 (DrumVoice.c:54–55, 306–308); snare/cymbal/hat use r=.1 in both velocity paths (Snare.c:36–37, 196/221; CymbalVoice.c:37–38, 205/234; HiHat.c:37–38, 213/241). Replay still calls actual calcOnePole once per frame, with the copied coefficient. No interpolation or second persistent smoother is introduced. The existing 64-float audio-owned DTCM gain buffer is reused.

The map operates **after** smoothing q. It does not smooth a newly mapped target. A transformed curve necessarily changes the audible trajectory: gain remains unity until replayed q falls below .25 and grows as sqrt(q) below the knee. Ordinary existing smoother lag remains; no deliberate time delay or trailing-control mechanism is added. Near zero, the curve has a steep slope; transformed attacks/last audible fade steps need listening. Host subnormal behavior need not match the device's existing flush-to-zero mode.

Core/Src/Soundengine/DSPAudio/FX/Distortion.c is byte-identical to R8. At stereo lines 625–634 / mono 683–692, lastOut is computed first, unattenuated, and only its audible contribution is multiplied:

```text
wet = lastOut * mapped_gain + unchanged_R7_parallel_bass
output = existing dry/wet blend with original dry input
```

The attenuated value is never stored into feedback history. The .22 excitation trim, feedback topology/coefficient set, nonlinear stages, tone/VOL behavior, −.25 parallel bass with 150 Hz LP, dry signal and integer mixing remain R8/R7. Original-input inactivity detection, linked stereo and short hold/fade are unchanged. Bass receives no new gain and no compensation. Ordinary fader motion requests no CLP reset.

### Eligibility and lifecycle

Exactly one **configured** FX-routed voice is eligible, regardless of which voice or its current activity. Zero and multiple routed voices retain NULL/unity fallback. A silent second routed voice counts. Capture remains CLP-only; other effects and distortion algorithms do not receive the new curve.

Every block selects fresh live data. No gain pointer or persistent gain history is added. Init, same-type/kit reset, effect exit/re-entry and routing changes retain R8's audio-owned lifecycle. Removing all routes or adding a second restores unity immediately; switching sole voices immediately selects the new sole voice. These policy changes may create level steps, as documented for R8. They are preserved, not crossfaded. The existing non-atomic kit parameter resend remains unchanged; host tests do not emulate live interrupt scheduling during full recall.

## Actual host tests and results

Native checks compile actual production DSP, FxBus and 1PoleLp and pinned R8 reference sources. The verbatim mixer capture function is compiled with six filter-state fixture objects. This is not a native execution of the full voice engine; the Release ARM build compiles that integration. Native flags include -O2, -fno-strict-aliasing, ASan, UBSan, bounds and float-cast-overflow, with nonrecovering failures.

Passed locally and in CI:

- Production curve through FxBus_clipFaderGains: physical table, q=0/.01/.0625/.16/.25/1, .5625, negative and over-unity clamps, both adjacent float neighbors at the knee, smallest positive host float and a **100001-point** bounded/monotonic sweep. A double-precision independent oracle checks float tolerance. All measured gains come from production, not the oracle.
- **144000 per-frame complete MasterDistortion memcmp checks** using actual mapped R9 gain versus the R8 replay value, all six voices and mono/stereo. DSP is byte-identical to pinned R8, and both instances receive identical inputs. All internal feedback, filter, bass, slew and detector histories match after every step.
- Unmodified inherited unit/state/component fixtures: **216000** additional gain-varying DSP steps, unchanged bass/dry component arithmetic, unity, zero VOL, zero D/W, Tube/Fold, input preservation, finite/bounded wet output and guard words. Existing R7 state fixture still checks maximum regeneration, detector reopening and idle/reset behavior.
- All **64 route masks**, all **36 sole-voice/output-route combinations**, complete live-filter contents untouched by capture, all-six mapped trajectories, zero/multiple fallback including silent extra routes, init, FX/kit reset, re-entry and sole-voice switching.
- **108 actual bus configuration pairs**, of which **72 fallback/unity pairs** require byte-identical pinned-R8 output. The other 36 exercise the intended changed single-voice curve.
- **318 paired unity renders** sample-identical to R8, including kicks, low/mixed tones, squared input-level sweeps, natural/sparse/returning notes, DC/full-scale/impulse and regeneration extremes. Final .5 s is exactly quantized silence. Maximum settling after last nonzero consumed input: **36.701405 ms** in this matrix.
- **72 moving-fader pairs**, physical input-plus-output and output-only controls; two existing rate groups, mono/identical/asymmetric stereo, kicks/returning notes and a low-level sustained regenerative probe. Complete DSP object fingerprints match pinned R8 at matching block boundaries. R9 average additional gain is at least R8's for every paired trajectory. Maximum R9 idle settling is **31.671953 ms**; no persistent idle output.
- Static fader gains, **54 zero-VOL** pairs, **18 zero-D/W** pairs and other distortion algorithms match R8 under their unchanged-output conditions. Seeded return-only CLP with cleared bass history matches at unity.
- Gain trajectories and selected moving waveforms are identical across **1/7/64-frame** partitions. Piecewise target events split partitions at the same frame boundaries.

Curve-specific R8 expectations were changed explicitly in new R9 fixtures: compare replay against the mapped live update rather than raw q; q=.25/.64 now requires unity; renders use actual production replay/mapping for both versions. No inherited R8 test or assertion was edited. Static checks require DSP/mixer and other existing production/config sources to match R8, and recover the original bus by removing only the mapper/include/call transform.

Relevant regressions all passed locally and in CI: bug001 (CC/NRPN, Drive/mutes/bounds, 256 morph positions and cadence); kit_load (notification/interleaving and 512 RAM/menu cases); project_storage (production failure/containment and RAM FatFs groups); euclidean (all six sanitizer groups, mask/playback/persistence/malformed/protocol). Storage/menu audio callbacks are mocked and do not validate device recall audio continuity.

### Paired renders, trajectories and discontinuities

Synthetic physical travel is 1 until .2 s, decreases linearly to zero at .7 s, holds zero to .9 s and returns to 1 by 1.2 s. The original squared target drives actual calcOnePole for input and the private replay for output. Production ADC targets are captured once per block; these audio renders deliberately use per-sample synthetic targets. Separate piecewise trajectory tests cover the production-style block contract.

Representative kick-plus-low-level-1400-Hz regenerative probe, drv=63/col=7/ton=127/vol=127, asymmetric stereo, voice-1 rate. Normalized left RMS:

| Window, s | R8 | R9 |
| --- | ---: | ---: |
| .30–.35 | .270399 | .472497 |
| .50–.55 | .055709 | .309026 |
| .65–.70 | .002080 | .051248 |
| .75–.80 | 0 | 0 |
| .95–1.00 | .028004 | .211052 |
| 1.15–1.20 | .406924 | .487209 |

These results show the intended reduction of additional attenuation during this sweep and a returning-note probe. They do not prove distortion/scream/whistle quality. Output-only controls retain unchanged input/bass intentionally. Full sample peak is at most **25805 LSB** in both moving matrices; largest adjacent-sample difference is **37103 LSB R8**, **37764 LSB R9**. These include nonlinear signal steps and are not isolated click measurements. Bounds/sanitizers pass; no promise of click-free sound follows.

For an abrupt synthetic target change from .0001 to .25 at frame 2300, voice 1's existing r=.01 produces q=.002597228 and mapped gain=.101926006 on that first frame; voice 6's r=.1 produces q=.02509 and gain=.316796452. The corresponding previous gains were .019820189 and .019999998. These ~.082106/.296796 per-frame gain steps arise from the approved curve and existing smoothers, not an extra smoothing stage. Smooth physical sweeps have different trajectories. During a zero target, host q approaches zero asymptotically; quantized output silence and the unchanged detector remain decisive.

Evidence includes all-six-voice q/gain CSV and eight WAV files (four R8/R9 pairs). Total loudness need not be strictly monotonic because the core remains nonlinear and phase interaction with dry/bass remains. More audible core can change the perceived bass balance even though bass processing is unchanged.

## One Release ARM build, resources and generated code

Exactly **one final Release ARM build** ran, in successful CI attempt 1. No local ARM build or R8 rebuild was performed. Toolchain: **Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174), arm-none-eabi-gcc 14.3.1 20250623**, same recorded R8 version. The downloaded toolchain archive SHA-256 is `8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd`, identical to the recorded R8 evidence. The version-named download records this digest but does not enforce an independently supplied expected archive hash.

Local native GCC is Debian 14.2.0; CI native GCC is Ubuntu 13.3.0; NumPy 2.3.5. ARM compilation retains the existing -O3, Cortex-M7 hard-float fpv5-d16, -ffast-math and -freciprocal-math settings. Native sanitizer checks do not execute the ARM binary or duplicate its floating-point mode.

| Region | Recorded R8 | R9 CI | Delta | R9 free |
| --- | ---: | ---: | ---: | ---: |
| ITCM / 16384 B | 15608 | **15608** | **0** | **776** |
| DTCM / 131072 B | 107672 | **107672** | **0** | **23400** |
| RAM / 393216 B | 274648 | **274648** | **0** | **118568** |
| Linker FLASH / 1015808 B | 237316 | **237388** | **+72** | **778420** |

.text grows 153320 → **153392 B**; .rodata=82616, .dtcm=36996, .dtcmz=70672 and .itcm_text=15608 remain unchanged. Image grows 289936 → **290008 B**, +72. No unrelated placement/optimization adjustment was made to fit memory.

Reviewed delivered DISASSEMBLY.txt and both recorded R8/R9 ELF symbol tables:

- FxBus_clipFaderGains stays in application FLASH at **0x080266b0**, size **204 → 276 bytes**, matching the 72-byte growth. The private mapper is inlined there; it is not added to ITCM.
- Existing calcOnePole call remains through its veneer. After replay, the generated path compares q to zero and .25 and branches to literal zero/unity as appropriate.
- For 0<q<.25, **0x08026726** executes `vsqrt.f32 s15,s0`, followed at **0x0802672a** by `vadd.f32 s15,s15,s15` (doubling). There is **no sqrtf library call** in this path.
- Added work is per eligible frame: bounded comparisons/branches and, below the knee, one hardware square root plus doubling. At most 64 square roots occur per eligible 64-frame block. Zero/multiple-route fallback and non-CLP capture skip mapping. No new persistent state, allocation or live-filter update is added.

**Memory fit is not evidence of real-time CPU headroom.** No instrument cycle count, deadline/underrun test, interrupt-interleaving proof or stack high-water measurement was made. The existing one-audio-writer scheduling contract is retained from R8. Flash/FPU instruction timing, contention and representative full-engine load require device evidence; host runtime is not a substitute.

## Image, source and downloaded-artifact verification

Both CI package.py and the independent audit of downloaded bytes passed:

- Magic **LXRV2IMG**, payload length **289992 bytes** = file size minus 16-byte header.
- Internal checksum **0x59**, recomputed as 255-(payload sum & 255); all three reserved bytes zero.
- Exact **LXR LAB R9-CLP** splash present; R8 splash absent.
- Initial SP **0x20080000**, 8-byte aligned and within configured SRAM boundary; reset **0x08013555**, Thumb bit set, inside the application load segment.
- Four nonempty ELF physical LOAD ranges, exclusive ends: **0x08008000–0x08041bf8**, **0x08041bf8–0x08041f4c**, **0x08041f4c–0x0804afd0**, **0x0804afd0–0x0804ecc8**. All fit application flash below 0x08100000; no load bytes touch bootloader flash.
- Ranges cover the entire payload contiguously and every loaded ELF byte matches the corresponding image byte. Zero-file-size BSS/heap segments are not flash payload. Linker FLASH figures alone exclude separately loaded DTCM/ITCM bytes; this check includes them.
- Named ZIP integrity and exact single-image membership/identity pass. Image and ZIP sizes/hashes match CI validation, printed logs and SHA256SUMS after download. Both outer wrapper sizes/digests match published artifact metadata.
- SOURCE_COMMIT.txt matches the published head. All **402 source blobs**, and the exact set of source file members (apart from ZIP directory entries), match Git blob hashes at that commit. SOURCE.zip integrity and SHA-256 pass.
- Host JSON base/hash fields match pinned R8 and delivered production DSP/FxBus. Artifact metadata ties both artifacts to the published R9 source head.

The complete evidence artifact includes exact source commit, SOURCE.zip, ELF/map, compile_commands.json, toolchain/native versions, toolchain archive hash, configure/build and test logs, JSON results, q/gain CSV, WAV pairs, full disassembly, load/section reports, image validation and SHA256SUMS. This establishes archive/source and ELF/image correspondence, not a second independently reproduced ARM binary or device performance.

## Changed files, commands and failures

Eight files differ from pinned R8, 532 insertions / 2 deletions:

- Production: Core/Src/Soundengine/FxBus.c (math include, bounded mapper, mapped replay); Core/Src/main.c:354 (LXR LAB R9-CLP).
- New tests/evidence: tests/fx_clip_r9/README.md, integration.c, render.c, run.py, package.py.
- New workflow: .github/workflows/clp-r9-build.yml, triggered only by pushes to the isolated R9 source branch.

Existing voice, DSP, ADC, mixer, storage/UI/format and inherited test files are unchanged. Production CRLF endings are preserved. Actual local commands included:

```sh
git fetch origin fx/clp-single-voice-fader-r8 lab-setup
git worktree add -b fx/clp-fader-curve-r9 /workspace/r9-source 7681da435cb5a70be3b08654227b5183acec1053
python3 tests/fx_clip_r9/run.py --output build/r9-host
python3 tests/bug001/run.py
python3 tests/kit_load/run.py
python3 tests/project_storage/run.py
python3 tests/euclidean/run.py
python3 -m py_compile tests/fx_clip_r9/run.py tests/fx_clip_r9/package.py
git -c core.whitespace=cr-at-eol diff --check
git push -u origin fx/clp-fader-curve-r9
```

The first local host run reached the paired renderer compile and failed because its new routing-enum use lacked Soundengine/DSPAudio/mixer.h. Adding that test-only include corrected the failure. Log: /workspace/scratch/r9-host-attempt1.log. The second full host run passed; regression suites each passed once locally. A final CSV formatting addition records both q and mapped gain; final CI runs the committed suite. No production change or weakened assertion was needed to resolve the compile error.

CI repeats the final host/regression commands and executes:

```sh
python3 -m pip install numpy==2.3.5
cmake --preset release
cmake --build --preset release --parallel 2
python3 tests/fx_clip_r9/package.py
arm-none-eabi-objdump -d build/release/LXR_V2_STM32F765.elf
arm-none-eabi-readelf -l build/release/LXR_V2_STM32F765.elf
arm-none-eabi-size -A build/release/LXR_V2_STM32F765.elf
arm-none-eabi-gcc --version
git archive --format=zip --output=build/r9-evidence/SOURCE.zip HEAD
```

Actual downloaded audit commands:

```sh
unzip -q /workspace/attachments/eb449c2b-355e-46bc-9577-211515859535/lxr-r9-image.zip -d /workspace/r9-downloads/image
unzip -q /workspace/attachments/e1de00e8-aaa9-4bd8-ae0a-c841ba4c5973/lxr-r9-source-build-evidence.zip -d /workspace/r9-downloads/evidence
python3 /workspace/scratch/r9_verify_download.py
readelf -sW /workspace/r9-downloads/evidence/LXR_V2_STM32F765.elf
readelf -sW /workspace/r8-downloads/evidence/LXR_V2_STM32F765.elf
```

The temporary verifier's first stricter archive-member-set assertion counted ZIP directory entries as extra source files. Inspection confirmed they were ordinary directories. Filtering directory entries while still requiring the exact source-file set corrected that audit assumption; all 402 file-content hash assertions already passed. The completed verification is /workspace/scratch/r9-download-verification.json. No artifact or source was changed to satisfy this check.

Local final logs: /workspace/scratch/r9-host.log, r9-bug001.log, r9-kitload.log, r9-storage.log and r9-euclidean.log. CI's evidence contains the final committed-suite/build logs. No DSP assertion, sanitizer, regression, ARM build, resource-fit or final packaging failure occurred.

## Handoff and publication

No idle-persistence, invalid-output, memory-bound or resource-fit blocker was found. R9 is packaged for a **separate installation/listening decision**, without claiming musical success or device timing validation.

Main, lab-setup and R4–R8 remote heads were checked before and after source publication and preserved:
main `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`;
lab-setup `5829398cea5d337200393757e39a37d4d8148256`;
R4 `c870f7249d7ee0d65423bc8b7bfdb422dcc74482`;
R5 `af3562fece090288a83727524c1035b79f4cf20b`;
R6 `a743444c01c622dd6a7417a36fb7faa317147310`;
R7 `6d7cb7f7763158ed9abc465a72df7f2560f26b57`;
R8 `7681da435cb5a70be3b08654227b5183acec1053`.

Only the new source and report-only branches were published. No merge, release, force push, repository-setting change, flashing or hardware test occurred.

Remaining listening questions: does full additional gain down to half travel give enough distortion/scream/whistle control; does the lower-half linear physical curve provide the desired final fade; are returning attacks, near-zero steps and the two existing smoothing-rate groups acceptable; is unchanged bass balance acceptable for this experiment; and does device processing meet deadlines under representative load? Bass adjustment and broader multi-voice control remain separate tasks.

Report-only branch: **codex-reports/2026-10-30-r9-clp-fader-curve-test-build**.
Exact report base: **5829398cea5d337200393757e39a37d4d8148256**.
The report head is the documentation-only commit containing this file; it contains no source candidate changes.
