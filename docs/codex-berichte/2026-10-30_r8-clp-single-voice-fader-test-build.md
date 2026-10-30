# R8 experimental CLP single-voice fader test build

Date: 2026-10-30 (UTC).
Repository: https://github.com/DocLXR-02/lxr-desktop-app.
Task: add output-level control for exactly one FX-routed voice, preserving the R7 excitation/feedback experiment.
**R8 is experimental and not hardware validated.** Installation and hardware testing require a separate later decision. Neither occurred here.

## Identity and delivery

| Item | Identity |
| --- | --- |
| Exact source base | **6d7cb7f7763158ed9abc465a72df7f2560f26b57**, R7 / fx/clp-parallel-bass-r7 |
| New source branch | **fx/clp-single-voice-fader-r8** |
| Published source commit | **7681da435cb5a70be3b08654227b5183acec1053** |
| Successful CI | https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41110428325 |
| Job | 124251113481, validate-and-build |
| Image artifact | [lxr-r8-image, 13001719383](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41110428325/artifacts/13001719383) |
| Separate complete source/build evidence | [lxr-r8-source-build-evidence, 13003691370](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41110428325/artifacts/13003691370) |

GitHub comparison confirms one commit ahead of pinned R7, zero behind, merge base exactly R7. Artifact metadata records that exact R8 head. Artifacts were available and unexpired at verification; recorded expiry is 2026-11-29.

| Delivered file | Bytes | SHA-256 |
| --- | ---: | --- |
| **LXR_V2_R8.img** | **289936** | `af2e08aa08177d27ae970cf00e5075bf60721530136ace508fcbf918a7830ce5` |
| **LXR_V2_R8.zip** | **206675** | `134dd30f28142a9cf620663219c0934c7540059ecabe26f2544282a0a5c53b12` |

LXR_V2_R8.zip contains **only LXR_V2_R8.img**, with bytes identical to the separately delivered image. GitHub's outer image artifact ZIP contains those two delivery files; it is a different wrapper.

SOURCE.zip in the separate evidence artifact: **1719189 bytes**, SHA-256
`455ede802a1bd9812f6e436c916042c083f517f741ed4ca3aef9b66a99949a1d`.

Outer artifact ZIP digests, independently recomputed after download:

| Wrapper | Bytes | SHA-256 |
| --- | ---: | --- |
| lxr-r8-image | 420229 | `10a32793ff93e4da9d6571253dce2aa88be426f24519a4954a87e9e296755ad3` |
| lxr-r8-source-build-evidence | 3762990 | `82768207358973b75cbc587aeb4888674f8d9fab344494bb87671cfc490f2adc` |

## Scope and owner evidence

Read root AGENTS.md and the [R7 report](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-30-r7-clp-parallel-bass-test-build/docs/codex-berichte/2026-10-30_r7-clp-parallel-bass-test-build.md). This task's explicit R7 source base and limited authorization control; earlier reports do not broaden permission.

DEVICE evidence supplied by owner: R7 was tested, its sound and retained bass are liked, but lowering the fader exposes isolated feedback followed by abrupt silence at zero. This motivates output gain in addition to the existing input-level timbral transformation. No new R8 device result is claimed.

This prototype supports **exactly one configured FX-routed voice**, whichever of the six voices it is. It is not independent wet-level control for multiple voices sharing one nonlinear bus. No recordings were available; new evidence is source inspection, synthetic host processing and CI/build validation.

## Exact gain placement

References below use source commit **7681da435cb5a70be3b08654227b5183acec1053**.

The audible wet calculation is now:

```text
R7 core: same excitation, feedback, filters, drive, slew/rails, diode, tone, vol
       → lastOut (unchanged, unattenuated history)
       → multiply by new per-frame fader gain ─┐
R7 original-input parallel bass × existing vol ├→ existing wet conversion / d/w blend
Original dry input, unchanged ─────────────────┘
```

Core/Src/Soundengine/DSPAudio/FX/Distortion.c:625–634, mono:683–692:

```c
dist->lastOut[0] = dist->lp3_state[0] * dist->p3;
...
int16_t wet = __SSAT((int32_t)(
    (dist->lastOut[0] * (clipGain ? clipGain[i] : 1.f) + bass1) * 32768.0f), 16);
```

The multiplier is applied **after lastOut is computed**, only to its audible contribution. No attenuated value is written back. The R7 bass branch receives no new gain, and the dry input/buffer and integer dry/wet arithmetic are unchanged. vol retains its existing influence inside the core and on R7 bass. d/w retains its existing position outside the wet sum.

R7 constants are retained: .22 core excitation trim, 150 Hz parallel LP and bass weight −.25. All feedback coefficients, nonlinear stages, controls, serialized formats and original-input detector/linked stereo/short hold/fade remain R7. No history reset is requested by ordinary fader motion. No long release, normalization, envelope gate, limiter or UI parameter is added.

Existing Distortion_process and FxBus_process APIs wrap new WithClipGain variants with NULL, meaning unity. The live mixer uses the new explicit variants. No DSP/bus object stores a gain pointer or new gain history; MasterDistortion remains 176 bytes on the native fixture. Tube/Fold ignore the gain argument; other bus effects retain their original processing calls.

## All six voices and coherent smoothing

Inspected the actual six voice paths:

| Voices | Existing smoother setup / use | Current coefficient |
| --- | --- | ---: |
| 1–3 | DrumVoice.c:54–55, 301–310, same function parameterized by voiceNr | .01 |
| 4, snare | Snare.c:36–37, 181–223; either velocity branch | .1 |
| 5, cymbal | CymbalVoice.c:37–38, 187–235; either velocity branch | .1 |
| 6, hi-hat | HiHat.c:37–38, 195–242; either velocity branch | .1 |

Every current sync path advances FaderVolumeFilter once per sample, including both velocity branches; none skips it merely because its envelope or output is silent. Voices 1–3 apply the fader after voice distortion; the percussion paths apply it before their final voice distortion. These existing synthesis orders are unchanged. The new gain follows the physical fader control, not voice envelope, velocity, voice VOL, pan or measured audio energy.

adcPots.c:256–278 already squares the normalized electrical fader value, with the existing update hysteresis and near-zero snap. adc_getFaderVolume returns that squared target. R8 neither squares it again nor changes that ADC law.

Production integration:

1. mixer.c:618–619 captures before any sync voice processing advances its live filter, after the existing async/modulation work.
2. mixer.c:549–562 selects CLP only, gathers all six actual FaderVolumeFilter pointers and existing squared ADC targets.
3. FxBus.c:41–56 selects the sole configured route and copies its actual **b and r** into a local OnePoleFilter.
4. The helper replays the unchanged production calcOnePole on that **private copy**, once per output frame, producing the same updated value that the live voice uses for that frame. It does not advance or write any live voice filter.
5. mixer.c:732 passes the frame array synchronously through FxBus_processWithClipGain to the DSP.

calcOnePole (1PoleLp.c:27–32) uses fmaf and stores its result back into b. R8 calls this same implementation with the copied coefficient, rather than duplicating constants in production. There is no endpoint interpolation, one-block gain lag or separately drifting persistent smoother. Frame zero uses the value after one replay update, matching the live frame-zero update.

At nominal 44140 Hz, .01 has a ~2.254 ms time constant, .1 ~.215 ms. These are existing smoothing speeds, not new release times. The added gain buffer is **64 floats / 256 bytes in DTCM**, audio-owned and overwritten before eligible use. No heap allocation occurs. Only small filter-copy/pointer/target locals are added to the stack; stack high-water was not measured.

Ownership reasoning: the existing engine calculates audio from TIM2 or foreground soundengine_process with TIM2 disabled during its calculation; the fader target producer is the foreground ADC polling path. Under this existing single-audio-writer schedule, targets/live states are stable during the block replay and subsequent voice processing. UART IRQ buffers data; its audio-control parsing occurs through the existing service paths. This is a code-based scheduling assessment, not an executed interrupt-interleaving proof. Future reentrant audio processing or asynchronous writes to live fader histories would require revisiting the capture contract.

## Routing eligibility and transitions

Eligibility uses mixer_audioRouting entries equal to MIXER_ROUTING_FX_BUS. It does not depend on amplitude, velocity, mute, envelope, selected UI voice or a hardcoded Voice 1. The existing routing resolver returns the FX-bus destination directly; direct-output jack fallback does not turn a direct route into an FX route.

| Current configuration | New audible core gain |
| --- | --- |
| CLP and exactly one FX-routed voice | That voice's replayed smoothed squared fader |
| CLP and zero routed voices | NULL/unity, exact R7 fallback |
| CLP and two or more routed voices | NULL/unity, exact R7 fallback |
| Other distortion algorithms/effects | Unity / no feature |

A silent routed voice still counts. No aggregate fader policy is introduced.

Every block recomputes eligibility and uses the current sole voice's actual live smoother. It cannot retain another voice's stale gain. NULL never reads old gain-buffer contents. The caller owns storage for the synchronous call; no pointer escapes into persistent state.

Initialization follows the existing zero-initialized live filters. There is no additional output-smoother state to initialize/reset. Kit/FX reset requests preserve their R7 DSP-history ownership; the next gain capture uses the current physical fader trajectory, not an invented reset-to-zero or reset-to-one gain. Switching away from CLP disables capture; returning selects fresh current data. Ordinary fader motion leaves CLP history intact.

**Eligibility changes take effect immediately.** Adding a second route restores full R7 core output immediately, even if that voice is silent. Removing all routes also restores R7 gain; it can briefly expose the existing R7 idle tail rather than retaining a stale zero gain. Switching sole voices selects the new voice's current trajectory immediately. These are explicit policy-driven level steps, not a crossfade or aggregate policy. They can sound abrupt and require listening. This experiment prevents stale/unintended muting; it does not promise click-free routing changes.

Kit recall remains the original parameter resend sequence, with reset requested at its start, not an atomic final parameter boundary. Intermediate route counts therefore follow the same fresh selection/fallback rules. Native tests exercise the actual reset request and routing helper; they do not emulate full live recall scheduling.

## Actual host checks and results

Native compilers: local Debian GCC 14.2.0; CI Ubuntu GCC 13.3.0. NumPy 2.3.5 locally and pinned in CI. Flags include -O2, -fno-strict-aliasing, ASan, UBSan, bounds and float-cast-overflow checks, with nonrecovering failures.

Tests compile actual R8 DSP, exact pinned R7 DSP, actual FxBus and actual 1PoleLp source. They also compile the **verbatim actual mixer capture function** with filter-state fixtures representing all six voice objects. Native tests do not synthesize the entire six-voice engine; the full mixer and voices compile in the ARM build.

Passed locally and in CI:

- Unmodified inherited DSP unit tests, R7 state/headroom/idle tests and all-six-output-route bus assertions.
- **216000 identical-input DSP steps** across CLP/Tube/Fold, mono/stereo and d/w values: complete MasterDistortion contents match the unity reference after each step despite gain values 1/.25/0/near-zero/.7. All feedback, filter, slew, detector and bass histories remain unchanged. Separate audible-component checks verify that gain touches only core output.
- **318 paired unity renders** compare explicit per-frame unity R8 output against pinned R7: complete output equality and core-return fingerprints match. These include kicks, low/mixed tones, squared input-level sweeps, natural quantized decays, sparse/returning notes, full-scale alternation, impulses, DC and regeneration/control extremes.
- Every principal unity case reaches exact quantized silence for its final .5 s. Maximum measured time after the actual last nonzero consumed sample: **36.701405 ms**. This is a synthetic-case result, not a universal acoustic deadline.
- **72 moving-fader pairs**, including physical input-plus-output fade and identical-input/output-only controls, drum/percussion smoothing rates, mono/identical/asymmetric stereo, kicks/returning notes and a low-level sustained-regeneration probe. Core-return fingerprints match R7 for identical incoming samples.
- Moving-fader maximum settling time **30.493883 ms**; all checked idle tails settle, with no persistent output. Moving renders have peak at most **25805 LSB** and maximum sample step **37103 LSB**.
- Static targets including 0, .01, .1, .5, .85 and 1; **54 zero-VOL paired probes** over three d/w amounts and **18 zero-wet paired probes** match R7. The baseline integer-mix behavior at endpoints is retained, not replaced with an idealized new zero property.
- Tube/Fold outputs remain identical with nonunity gains; seeded return-only CLP with cleared bass history remains identical at unity.
- All **64 route masks**, all **36 sole-voice/output-route combinations**, unchanged live filter state during capture, zero/multiple-voice fallback and silent extra voices.
- Actual bus-output probes compare R7/R8 over 108 configuration pairs; **72 fallback/unity pairs require and pass byte-identical output**. The remaining sole-voice nonunity cases exercise the intended attenuation.
- Initialization, FX disable/re-entry, same-type/kit reset request, route-count changes and switching sole voices are exercised.
- Replay values match sequential live calcOnePole values exactly; selected gain and waveform probes match at **1/7/64-frame partitions**. Target changes occur at fixed frame events with partitions split at those events.
- Input preservation, finite values, wet bounds, buffer guard words and sanitizers pass.

Static checks additionally recover the original R7 DSP process body by removing only the documented gain parameter/multiplications, and protect the original mixer processing flow outside the capture/call insertion. ADC, all four voice source files covering six voices, 1PoleLp and presetManager are byte-identical to pinned R7. No existing test assertion was weakened. Old source-identity/timbre runners that describe previous candidates are not used as an R8 audible-gain criterion; their suitable unit/state/bus fixtures are reused.

Relevant regressions passed locally and again in CI:

- bug001: CC0/NRPN/Drive/mutes/bounds, all 256 morph positions and immediate sweep/audio cadence.
- kit_load: notification/interleaving and 512 RAM load/menu cases.
- project_storage: production storage/failure/containment groups and whole-FatFs RAM checks.
- euclidean: six groups including 2176 masks, playback, persistence, malformed and protocol bounds.

Storage/menu tests mock audio callbacks. These passes do not establish real device recall continuity.

## Gain trajectories and waveform evidence

Evidence includes gain-trajectories.csv for all six replayed voices and paired synthetic WAVs. In the captured-helper fixture, target is 1 until frame 400, zero until 1900, .0001 until 2300, then .25:

| Voice group | Gain frame 399 | First zero-target update, 400 | Last zero-target frame, 1899 | First .25-target update, 2300 |
| --- | ---: | ---: | ---: | ---: |
| Voices 1–3, example 1 | .983844638 | .974006176 | 2.79074e-7 | .002597228 |
| Voices 4–6, example 6 | .999999762 | .899999797 | 5.60519e-45 | .025090000 |

The corresponding largest fixture gain steps are ~.00983846 and .09999997, consistent with the existing coefficients. Near-zero host subnormal values need not reproduce bit-for-bit on the instrument, whose existing FPU mode flushes tiny values; audible output quantizes away much earlier. The new replay and live filter use the same production function on hardware.

Rendered fade profiles use synthetic per-sample squared targets, whereas real ADC targets update through foreground polling and are captured once per block. Production-style piecewise block targets are covered by the replay fixture. Both should be distinguished from a calibrated physical fader recording.

Representative physical input-plus-output fade, row drv=63/col=7/ton=127/vol=127, asymmetric stereo, voice-1 rate. Fader travel falls linearly from 1 at .2 s to 0 at .7 s, stays zero to .9 s, then rises to 1 by 1.2 s. Selected left-output RMS windows (normalized int16):

| Window, seconds | R7 | R8 |
| --- | ---: | ---: |
| .30–.35 | .472497 | .270399 |
| .50–.55 | .434004 | .055709 |
| .65–.70 | .371767 | .002080 |
| .75–.80 | 0 | 0 |
| .95–1.00 | .425354 | .028004 |
| 1.15–1.20 | .487209 | .406924 |

An output-only control keeps the same incoming signal while moving the new gain: over .75–.80 s, R7 RMS is .494952 and R8 .000174. The small retained R8 signal is expected: the bass input is deliberately unchanged in that control, and the bass branch is not given the extra gain.

These probes demonstrate the intended gain placement and attenuation before zero. **They do not establish musical fade quality, click-free transitions or strictly monotonic total RMS.** The core remains nonlinear, and bass/dry phase interaction remains. For a weak feedforward signal, two fader-dependent stages can make the audible core fall faster (roughly u⁴ in a linear approximation), while regenerated core energy receives the additional u² output factor. Owner listening must judge the useful travel range, return attacks and whether low-level sound becomes too quiet.

The R7 timbral transformation remains inside the core for the same incoming samples. Audible balance and attacks intentionally change at nonunity gain. A held physical fader during missing sequencer steps still uses R7's short idle mechanism; this build does not add a release envelope.

## One Release ARM build and resources

CI performed exactly one Release ARM build of the final source. No local ARM build or R7 rebuild was performed.

**Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174), arm-none-eabi-gcc 14.3.1 20250623**, matching the recorded R7 compiler version and unchanged Release CMake settings. The evidence captures native/ARM versions and toolchain archive SHA-256. The version-named archive download does not enforce an independently supplied expected archive digest.

| Region | Recorded R7 | R8 CI | Delta | R8 free |
| --- | ---: | ---: | ---: | ---: |
| ITCM, 16384 B | 15104 | 15608 | +504 | **776** |
| DTCM, 131072 B | 107416 | 107672 | +256 | 23400 |
| RAM, 393216 B | 274648 | 274648 | 0 | 118568 |
| Linker FLASH, 1015808 B | 235012 | 237316 | +2304 | 778492 |

R8 .text=153320, .rodata=82616, .dtcm=36996, .dtcmz=70672, .itcm_text=15608. Image grows from recorded R7 287128 to **289936 bytes**, +2808, including separately loaded ITCM code. R7 figures are recorded evidence, not a new rebuild.

ITCM fits but is tight. **Memory fit is not proof of real-time CPU headroom, stack high-water, underrun behavior or interrupt timing.** New replay calls and output arithmetic add work. Native runtime is not a device benchmark.

## Artifact and source verification

CI package.py and an independent local audit of downloaded artifacts passed:

- LXRV2IMG magic; payload length **289920 bytes** equals file size minus 16-byte header.
- Internal checksum **0x2C**, computed as 255-(payload byte sum & 255); reserved bytes zero.
- Exact **LXR LAB R8-CLP** splash present.
- SP **0x20080000**, aligned/in configured SRAM boundary; reset vector **0x08013555**, Thumb bit set and in application flash.
- Four nonempty ELF LOAD segments fit application flash and exactly match image bytes at their physical offsets:
  0x08008000…0x08041bb0;
  0x08041bb0…0x08041f04;
  0x08041f04…0x0804af88;
  0x0804af88…0x0804ec80 (exclusive ends).
- No nonempty load bytes touch bootloader flash. Linker FLASH accounting alone omits separately loaded ITCM/DTCM bytes; this check includes them.
- Named image-only ZIP membership/integrity/image identity and both outer artifact digests pass.
- SOURCE_COMMIT.txt is the published source commit. All **396 source blobs** in SOURCE.zip match its Git blob hashes; archive integrity passes.
- Host-results source SHA-256 matches delivered production Distortion.c.

Separate evidence contains full source, exact commit, ELF/map, compile_commands.json, section report, ARM/native compiler versions and archive hash, configure/build logs, host/regression logs, JSON results, all-six-voice gain CSV, selected WAV pairs, validation JSON and checksums. No report or unrelated file is inserted into the named image-only ZIP.

These checks establish source/archive and ELF/image consistency, not a second independently reproduced ARM binary or a hardware installation result.

## Affected files, actual commands and failures

Fourteen files differ from pinned R7:

- Production: Core/Inc/Soundengine/DSPAudio/FX/Distortion.h; Core/Inc/Soundengine/FxBus.h; Core/Src/Soundengine/DSPAudio/FX/Distortion.c; Core/Src/Soundengine/DSPAudio/mixer.c; Core/Src/Soundengine/FxBus.c; Core/Src/main.c.
- New tests/evidence: tests/fx_clip_r8/README.md, bus_probe.c, integration.c, package.py, render.c, run.py, state.c.
- New workflow: .github/workflows/clp-r8-build.yml, scoped only to the new source branch.

Existing voice, ADC, preset/storage, parameter-format and other-effect sources are unchanged. Production CRLF endings are retained.

Source worktree /workspace/r8-source was created from exact R7. Commands actually run included:

```sh
git fetch origin fx/clp-parallel-bass-r7 lab-setup
git worktree add -b fx/clp-single-voice-fader-r8 /workspace/r8-source 6d7cb7f7763158ed9abc465a72df7f2560f26b57
python3 tests/fx_clip_r8/run.py --output build/r8-host
python3 tests/bug001/run.py
python3 tests/kit_load/run.py
python3 tests/project_storage/run.py
python3 tests/euclidean/run.py
git -c core.whitespace=cr-at-eol diff --check
git push -u origin fx/clp-single-voice-fader-r8
```

Local logs: /workspace/scratch/r8-host.log, r8-bug001.log, r8-kitload.log, r8-storage.log, r8-euclidean.log. The host runner was rerun after adding direct bus-fallback comparisons and moving-idle assertions; no production change was needed for those additional checks. CI repeated the final suite/regressions.

Failed preliminary checks:

1. A standalone native integration compile used include name capture.inc while the temporary extraction was named r8-capture.inc. Copying it to the requested include name fixed the compile.
2. The new static function extractor initially omitted “static inline float”, so it failed to find Distortion_clipFeedback. Its signature pattern was corrected; the unchanged-function equality assertions remain.
3. An exploratory source search referenced two nonexistent paths; inspection continued using the actual source locations.

No DSP assertion, sanitizer, final regression, ARM build, resource-fit or packaging failure occurred. No force push or repository setting change was used to resolve a failure.

CI's exact workflow records toolchain installation and:

```sh
python3 -m pip install numpy==2.3.5
cmake --preset release
cmake --build --preset release --parallel 2
python3 tests/fx_clip_r8/package.py
arm-none-eabi-size -A build/release/LXR_V2_STM32F765.elf
arm-none-eabi-gcc --version
git archive --format=zip --output=build/r8-evidence/SOURCE.zip HEAD
```

Downloaded audit commands:

```sh
unzip -q /workspace/attachments/cbf2878e-5afd-487b-aae6-a45d6b2fb327/lxr-r8-image.zip -d /workspace/r8-downloads/image
unzip -q /workspace/attachments/c90c0de1-f578-4a14-97a1-b2e0ec7033b6/lxr-r8-source-build-evidence.zip -d /workspace/r8-downloads/evidence
python3 /workspace/scratch/r8_verify_download.py
readelf -l /workspace/r8-downloads/evidence/LXR_V2_STM32F765.elf
```

The independent temporary verifier recomputes outer/image/ZIP/source hashes, checks ZIP members and integrity, header/checksum/vectors, ELF flash/payload equality, all Git source blobs via git ls-tree, and result/source identity. It completed successfully. Separate Python reads of delivered CSV/JSON/WAV files produced the trajectory/RMS tables above.

## Handoff and publication

No coherent-ownership, idle-persistence, invalid-output, memory-bound or resource-overflow blocker was found in these checks. The files are delivered for a **separate installation/listening decision**. This is not a general multi-voice wet-level solution.

Remaining listening questions: does the extra core gain provide the desired fade while keeping R7's timbral sweep; do percussion's faster existing smoothing and low-level quantization sound acceptable; are returning notes softened too much; are policy-driven routing/kit level steps acceptable; and does device CPU/stack/IRQ headroom remain adequate?

Main, R4, R5, R6, R7 and lab-setup remain unchanged:
main a43cd4fcda23ec3d577dc425261c3eb28bd5abff;
R4 c870f7249d7ee0d65423bc8b7bfdb422dcc74482;
R5 af3562fece090288a83727524c1035b79f4cf20b;
R6 a743444c01c622dd6a7417a36fb7faa317147310;
R7 6d7cb7f7763158ed9abc465a72df7f2560f26b57;
lab-setup 5829398cea5d337200393757e39a37d4d8148256.

No merge, release, force push, setting change, flashing or hardware test occurred.

Report-only branch: **codex-reports/2026-10-30-r8-clp-single-voice-fader-test-build**.
Exact report base: **5829398cea5d337200393757e39a37d4d8148256**.
Report head: the report-only commit containing this file; no source-candidate changes are included on that branch.
