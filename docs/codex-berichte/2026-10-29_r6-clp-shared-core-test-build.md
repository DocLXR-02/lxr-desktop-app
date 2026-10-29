# R6 experimental CLP shared-core test build

Date: 2026-10-29 (UTC).
Repository: https://github.com/DocLXR-02/lxr-desktop-app.
Task: implement the authorized approach B experiment, compare against pinned R5, build and package an application image. **R6 is experimental and not hardware validated.**
Installation and hardware testing require a separate later decision; neither occurred here.

## Identity and delivery

- Exact source base: **af3562fece090288a83727524c1035b79f4cf20b**, fx/clp-input-silence (R5).
- New source branch: **fx/clp-bass-feedback-r6**.
- Published source commit: **a743444c01c622dd6a7417a36fb7faa317147310**.
- GitHub comparison verifies one commit ahead, zero behind, merge base exactly pinned R5.
- Successful CI: https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/40955325500
- CI job: 123733818843, validate-and-build.
- Image artifact: [lxr-r6-image, ID 12932832918](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/40955325500/artifacts/12932832918).
- Separate full-source/build evidence: [lxr-r6-source-build-evidence, ID 12932334613](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/40955325500/artifacts/12932334613).
- Artifacts were available and unexpired at verification; their recorded expiration is 2026-11-28. No GitHub release was created.

| Delivered file | Bytes | SHA-256 |
| --- | ---: | --- |
| LXR_V2_R6.img | **287104** | `898a235f31d1fd314322ddca1e4b565898f3a5af3302c8cd2f8dabd6ce7f1ac6` |
| LXR_V2_R6.zip | **205551** | `baf00fe26a012d5fa196fc979beffa4853349f7f0363bb6ee6e1d0c2bedca6e2` |

**LXR_V2_R6.zip contains exactly one entry: LXR_V2_R6.img**, with bytes identical to the separately delivered image. GitHub's outer image artifact ZIP contains those two delivery files; it is a separate wrapper, not the named image-only ZIP.

Full SOURCE.zip in the evidence artifact: 1701782 bytes, SHA-256
`d546e7552a054136004aff27481537c81dec6634c02a720bd90534a24c142765`.

Downloaded outer artifact ZIPs were also checked against GitHub's digests:

| Artifact wrapper | Bytes | SHA-256 |
| --- | ---: | --- |
| lxr-r6-image | 418013 | `1d647bcc740041ca3127cebfdd1df5ba3b93e7ff732438ffcdfbb9298912d016` |
| lxr-r6-source-build-evidence | 2652614 | `f4063dc9cf532651926f6aa3d58b3445241417610ed88d160612f94bc136df34` |

## Context, scope and evidence boundaries

Read root AGENTS.md and all three reports: [level-response analysis](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-clp-level-response-analysis/docs/codex-berichte/2026-10-29_clp-level-response-analysis.md), [R5 implementation](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r5-clp-test-build/docs/codex-berichte/2026-10-29_r5-clp-test-build.md), and [independent R5 review](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r5-clp-independent-review/docs/codex-berichte/2026-10-29_r5-clp-independent-review.md). This task's explicit authorization controls; historical permissions do not expand it.

DEVICE, supplied by owner: exact R5 image SHA-256 90caf0cc55203a828f02a108e0bd62f4ac8396c94e823e9680ee433991284a3f was installed successfully. Unwanted sustained idle oscillation disappears at voice-fader zero and during missing sequencer steps. These are confirmed R5 behaviors, not an R6 result or comprehensive R5 validation.

Musical target: more useful overlap of bass and screaming feedback, with feedback useful around 80–90% physical travel and bass toward 50%. These approximate positions are not parameter bytes or calibrated universal boundaries. The owner accepts that bass may influence the scream. No instrument recordings were available; all new audio measurements below use synthetic inputs and host processing.

R6 changes only the incoming CLP voicing and the necessary runtime histories. It preserves the original-bus detector, linked stereo and existing hold/fade. It does not implement the previously discussed 0.5–1 s release. There is no clean bass addition after distortion, automatic normalization, new UI control, global fader change, routing redesign or kit-format change.

## Topology and selected constants

Precise source references in this report are to **a743444c01c622dd6a7417a36fb7faa317147310**.

Only the first common high-pass stage is split:

```text
original bus input x → new input HP (120 Hz pole) → 1/32 weight ─┐
                                                            sum
lastOut × col × .125 × unchanged idle gain → R5 return HP ─────┘
  → shared LP1 → shared HP3 → shared LP2
  → shared drv gain → shared slew and ±4.2 rails
  → shared diode clipping → shared ton LP → shared vol → lastOut
  → existing outer d/w mix and routing
```

All additional or retained bass enters the same slew/rail/diode/tone chain as the feedback. The existing d/w dry contribution remains the original one; no new clean wet bass path exists.

Core/Src/Soundengine/DSPAudio/FX/Distortion.c:202–220 adds the small helper:

```c
#define CLIP_INPUT_HP_ALPHA 0.016936545811725878252
#define CLIP_INPUT_WEIGHT (1. / 32.)
...
return (input - dist->clipInputHp_state[channel]) * CLIP_INPUT_WEIGHT +
       (loopReturn - dist->hp1_state[channel]);
```

The new input coefficient is 1-exp(-2*pi*120/44140), a matched pole at 120 Hz. The feedback HP retains R5's exact update coefficient .64116641920131193922, whose pole corresponds to 7200 Hz; the mistaken old “7.2 Hz” comment was removed from the replaced common block. No literal 7.2 Hz substitution was made. Subsequent LP1/HP3/LP2 coefficients are unchanged, including the existing 60 Hz high-pass.

Input weighting of 1/32 is -30.10 dB relative to an unfiltered unity branch, but that number alone does not describe the combined change. Compared with R5's strongly bass-rejecting first HP, the new weighted input branch has these linear relative magnitudes:

| Input frequency | R6 vs R5 feedforward response |
| --- | ---: |
| 30 Hz | +9.95 dB |
| 60 Hz | +9.25 dB |
| 100 Hz | +7.92 dB |
| 200 Hz | +4.45 dB |
| 500 Hz | -2.40 dB |
| 1000 Hz | -8.18 dB |
| 2000 Hz | -13.92 dB |

These ratios are before the shared nonlinear stages; they are not output gain promises. At drv=63 the unchanged shared gain is 1143.929, so even trimmed input can strongly saturate the core. The design adds low-frequency excitation while reducing upper-frequency forcing, rather than indiscriminately increasing bass by roughly 46 dB through a literal coefficient correction.

The global fader still supplies approximately u² amplitude for normalized electrical position u. At u=.5 that is -12.04 dB. The +9.25 dB linear feedforward improvement at 60 Hz offsets much of that reduction relative to full-level R5, but nonlinear behavior, kit levels, velocity, other voices and phase prevent a universal travel mapping. The ~14 dB reduction near 2 kHz is a bounded first experiment toward letting the return compete at higher travel; it does not prove earlier desirable feedback.

Stereo integration: Distortion.c:499–503. Mono:608–610. Both combine the two branches before the existing common filter/core. HP1 is the only split stage; the full chain is not duplicated.

## Runtime ownership, controls and unaffected scope

Core/Inc/Soundengine/DSPAudio/FX/Distortion.h:39–41 adds two doubles, clipInputHp_state[2], **16 runtime bytes**. Existing hp1_state now holds return-branch HP history. No serialized structure or parameter definition changes.

Distortion.c:238–239 initializes both new histories during normal initialization. Lines 163–179 clear them at the existing audio-owned CLP entry along with the other filter/slew/return histories. The reset request at lines 157–160 still only sets clipStateActive=0; it does not write DSP histories from a different context.

The detector at lines 182–199 is text-identical to pinned R5. It observes the original consumed bus samples, before input HP or weighting. Either stereo side can hold both returns open. Mono retains the same consumed-channel behavior. Values in [-1,1] count inactive; hold remains 441 frames, followed by the same float decrement 1.f/221.f. As in R5, exact zero is reached on the 222nd subtraction, not a new R6 timing change.

Existing full-kit resend and bus/algorithm re-entry reset paths are reused. FxBus.c and presetManager.c are byte-identical to R5 and checked as such by the new suite. The earlier review's recall sequencing limitation remains: reset is requested at resend start, and audio can process before parameter transmission completes. R6 does not make recall atomic or promise click-free transitions.

drv/col/ton/vol/d/w retain their roles and parameter formats. vol and ton remain inside the feedback loop; d/w remains outside it. The new voicing deliberately changes the audible response of those controls. Global fader scaling, voice synthesis, mixer/routing, storage algorithms, Tube/Fold algorithms and all other effects are unchanged. The splash now reads **LXR LAB R6-CLP** (Core/Src/main.c:354).

Affected files, and only these nine:

- Production: Core/Inc/Soundengine/DSPAudio/FX/Distortion.h; Core/Src/Soundengine/DSPAudio/FX/Distortion.c; Core/Src/main.c.
- Host/evidence: tests/fx_clip_r6/README.md, state.c, render.c, run.py, package.py.
- CI: .github/workflows/clp-r6-build.yml, scoped to the new R6 branch.

## Actual host verification

Actual production Distortion.c was compiled for both R6 and pinned R5, with host CMSIS shims. The reference is exact R5 from its Git object; it is not a newly implemented DSP model. Native flags include -O2, -fno-strict-aliasing, ASan, UBSan, bounds and float-cast-overflow sanitizers, and nonrecovering failures. Local cc was Debian GCC 14.2.0. CI used the Ubuntu 24.04 runner's default native cc; its explicit version was not separately captured. NumPy 2.3.5 was used locally and pinned in CI.

Final local suite and CI both passed:

- Existing actual-DSP unit tests: endpoint/range, cold silence at maximum controls, independent instances, hold/fade/reopen and Tube/Fold re-entry.
- New state checks: poisoned-instance init, deferred reset leaving histories untouched until audio entry, then clearing both new histories; original-bus 2-LSB reopening; linked stereo versus mono; exact inherited hold and fade; long zero input.
- Existing actual FxBus fixture: six output routes, direct same-type reset request, FX_OFF/re-entry. Storage/menu tests separately retain their mocked audio callbacks; no claim of full live recall audio coverage is made.
- **264 R5/R6 render pairs** (528 principal renders), each 1.5 s at nominal 44140 Hz. Four control rows plus col=0 probes; squared input levels corresponding to 100/90/85/80/70/50/40%; mono, identical and asymmetric stereo; kicks, 60 Hz tones, 60+1400 Hz mixed tones, sparse notes, natural decays through int16 zero, returning notes, downward fader-equivalent input-loss ramps, full-scale alternation and impulses.
- Nine unchanged-path pairs: Tube/Fold and seeded return-only CLP across mono/identical/asymmetric modes. Outputs are sample-identical to pinned R5 for those fixtures. This tests the retained return voicing in isolation; incoming audio can still alter regeneration musically.
- Zero-wet output remains identical for all three channel modes.
- Selected sparse/decay/return cases are sample-identical with block sizes 1, 7 and 64.
- Buffer guard words, finite state checks and sanitizers passed. There was no detected invalid output, memory overrun or persistent idle oscillation in the suite.

All principal cases became exact quantized silence for the final 0.5 s of their renders. Maximum measured R6 time from the last nonzero **consumed** input frame to the last nonzero output frame was **43.31672 ms**; R5 maximum in the same matrix was 38.42320 ms. The slightly longer R6 audio settling follows changed filter/history excitation, despite identical detector timing. The test requires <100 ms settling in these finite cases, not as a universal acoustic bound. Natural-decay input ends through quantization, rather than being forcibly cut at a selected decay time.

The original R5-vs-bounded-R4 continuous equality runner in tests/fx_clip/run.py is deliberately not applied as an R6 musical criterion: R6 changes that voicing. Its still-relevant unit and bus fixtures are executed by the new suite. No existing source regression assertion was weakened.

Existing regressions passed locally and again in CI:

- python3 tests/bug001/run.py: CC0/NRPN/Drive/mutes/bounds and all 256 morph positions/immediate sweep/cadence.
- python3 tests/kit_load/run.py: notification timing/interleaving and 512 actual RAM load/menu cases.
- python3 tests/project_storage/run.py: storage/failure/containment/reader groups and whole-FatFs RAM regressions.
- python3 tests/euclidean/run.py: all six groups, including 2176 masks, playback, persistence, malformed inputs and protocol bounds.

## Synthetic measurements and what they do not prove

Evidence artifact r6-host/results.json contains all measured cases and metrics. Selected unnormalized stereo WAV pairs cover kick, sparse, natural-decay, return and fader-loss cases. These are synthetic host renders, not recordings from the instrument.

Band RMS is calculated on the first 480 ms, left channel, with a Hann window: low band 30–150 Hz; high band 500–5000 Hz. Values are normalized to int16 full scale. Raw peaks and sample steps use unwindowed LSB. Bands cannot uniquely separate regenerated feedback from clipping harmonics.

Representative synthetic kick, drv=63/col=7/ton=127/vol=127, 100% wet, mono:

| Electrical travel proxy | R5 low RMS | R6 low RMS | R5 high RMS | R6 high RMS |
| --- | ---: | ---: | ---: | ---: |
| 100% | .144231 | .217919 | .156530 | .110929 |
| 90% | .128200 | .207446 | .166643 | .114625 |
| 85% | .119864 | .201026 | .171082 | .117093 |
| 80% | .111103 | .193364 | .176228 | .120101 |
| 70% | .093260 | .173834 | .184315 | .133915 |
| 50% | .055851 | .121811 | .197932 | .169653 |
| 40% | .037443 | .091322 | .202452 | .184739 |

R6 retains substantially more measured low-band energy down this synthetic sweep. It also reduces measured high-band energy in these cases. **This does not establish earlier screaming at 80–90%; bass may mask, force or change the oscillator instead.** The owner must judge the tradeoff. No parameter constant was silently retuned after these measurements to claim a musical target had been met.

At 85%, the R6 kick high-band RMS is .117093 with col=7 versus .009655 with col=0 at otherwise identical controls. This supports a material regenerative influence on the rendered signal, but the two nonlinear waveforms cannot be subtracted conceptually into independent “bass” and “feedback” powers.

Maximum R6 wet peak over principal cases: **21216 LSB** (~.64746 full scale), consistent with the retained diode/volume range. Maximum sample-to-sample step: **41708 LSB**. Peak boundedness is not evidence of a smooth waveform or inaudible clicks.

For the selected asymmetric returning-note WAV (row 0, travel .85, return at .6 s), the .60–.68 s window peaks at 21216 LSB in both versions; maximum step is 18398 LSB for R5 versus 22828 for R6. There is no attack fade added by the detector, but the new filter path and remaining loop history alter the attack. These numbers and WAVs require listening; they do not prove clean note re-entry.

Physical fader-to-ADC calibration, complete voice/kit audio, acoustic bass weight, screaming quality, user-perceived onset, live stereo/recall behavior and real-time CPU headroom remain unmeasured. Linked unilateral activity can still sustain the opposite side: silence policy is unchanged and requires inactivity of the consumed linked input.

## One Release ARM build and resource comparison

CI performed exactly one Release ARM build for this R6 source. No local ARM build or R5 ARM rebuild was performed. Compiler:

**arm-none-eabi-gcc, Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174), 14.3.1 20250623**.

This is the same compiler version used for the published R5 artifact and its reported resource measurements. R6 uses the unchanged Release CMake settings. Toolchain archive SHA-256 is captured in the evidence artifact; the workflow downloads a version-named URL, not a separately enforced expected archive digest. There is no claim of independently reproduced compiler provenance.

| Region | R5 recorded build | R6 CI | Delta | R6 free |
| --- | ---: | ---: | ---: | ---: |
| ITCM, 16384 B | 15104 | 15104 | 0 | 1280 |
| DTCM, 131072 B | 107400 | 107416 | +16 | 23656 |
| RAM, 393216 B | 274648 | 274648 | 0 | 118568 |
| Linker FLASH, 1015808 B | 234764 | 234988 | +224 | 780820 |

R6 .text=151016, .rodata=82592, .dtcm=36996, .dtcmz=70416, .itcm_text=15104. R5 baseline values come from its existing verified CI/section evidence, not a new R5 build. Linker FLASH accounting omits separately loaded ITCM/DTCM bytes; ELF verification below covers their load addresses.

The image grows from R5's 286880 to **287104 bytes**, +224. New filter histories account for the +16 DTCM. Memory fit does not measure CPU cycles, stack high-water, underruns or latency. The extra filter arithmetic still needs on-device evaluation.

## Image, source and artifact verification

CI tests/fx_clip_r6/package.py and an independent local audit of downloaded files passed:

- LXRV2IMG magic; payload size **287088** equals file size minus the 16-byte header.
- Internal checksum **0x8B**, computed as 255-(payload byte sum & 255); reserved header bytes zero.
- Exact modified splash present.
- Initial SP **0x20080000**, SRAM boundary and aligned; reset vector **0x08013555**, Thumb bit set and in application flash.
- ELF32 ARM program headers parsed. Every nonempty LOAD segment lies in application flash and matches the corresponding image payload bytes.
- No nonempty load bytes touch bootloader flash. Load ranges, exclusive ends:
  - 0x08008000…0x08041298
  - 0x08041298…0x080415ec
  - 0x080415ec…0x0804a670
  - 0x0804a670…0x0804e170
- Image-only ZIP entry list, integrity and byte identity checked; both outer artifact ZIP digests match GitHub.
- SOURCE_COMMIT.txt is the published R6 commit. All **388 archived source blobs** match the Git blob hashes of that commit; SOURCE.zip integrity passes.
- Host-results source SHA-256 matches the delivered production Distortion.c.

The evidence artifact includes full SOURCE.zip, source commit, ELF/map, compile_commands.json, section sizes, toolchain version/archive hash, configure/build logs, host/regression logs, complete measurement JSON, selected WAVs, validation JSON and checksums. It is separate from the image delivery package.

The local source branch and downloaded source archive correspond exactly to CI's recorded head. This establishes source/archive and ELF/image consistency; it does not constitute an independent second firmware rebuild or a hardware installation test.

## Actual commands, failures and corrections

Working source: /workspace/r6-source, an isolated Git worktree from exact R5. Original /workspace/lxr-desktop-app stayed on its original work branch. Downloaded files: /workspace/r6-downloads/image and /workspace/r6-downloads/evidence.

Key commands actually run:

```sh
git fetch origin fx/clp-input-silence lab-setup
git worktree add -b fx/clp-bass-feedback-r6 /workspace/r6-source af3562fece090288a83727524c1035b79f4cf20b
python3 tests/fx_clip_r6/run.py --output build/r6-host
python3 tests/bug001/run.py
python3 tests/kit_load/run.py
python3 tests/project_storage/run.py
python3 tests/euclidean/run.py
git -c core.whitespace=cr-at-eol diff --check
git push -u origin fx/clp-bass-feedback-r6
```

Local command output was captured under /workspace/scratch/r6-host.log, r6-bug001.log, r6-kitload.log, r6-storage.log and r6-euclidean.log. CI repeated the host/regression commands and ran:

```sh
python3 -m pip install numpy==2.3.5
cmake --preset release
cmake --build --preset release --parallel 2
python3 tests/fx_clip_r6/package.py
arm-none-eabi-size -A build/release/LXR_V2_STM32F765.elf
arm-none-eabi-gcc --version
git archive --format=zip --output=build/r6-evidence/SOURCE.zip HEAD
```

CI's workflow records the full toolchain download/extraction, copying and checksum commands. No ARM build failed or was repeated.

Failed attempts were understood and corrected before the final pass:

1. New return-only asymmetric fixture still injected a right-channel kick, so its R5/R6 identity assertion failed. Scenario 9 was corrected to keep both inputs zero. Production did not change.
2. The new settling timer initially used an assumed left-channel endpoint; an asymmetric impulse case legitimately had a later right kick, causing an apparent R5 settling failure. The renderer now reports the actual last nonzero consumed frame and all timing uses it. Production did not change.
3. Default git diff --check treated existing CRLF line endings on new source lines as whitespace errors. The CRLF-aware invocation above passed; source line endings were preserved.
4. First Git push was rejected by GitHub email-privacy protection (GH007), before a remote source branch existed. The unpublished commit's author/committer metadata was replaced using the account's public noreply identity, then pushed normally. No settings were changed and no force push was used. Final published commit is a743444c01c622dd6a7417a36fb7faa317147310.

Downloaded evidence commands included:

```sh
unzip -q /workspace/attachments/49380e7a-6288-4797-b00d-4d0d8bcb5330/lxr-r6-image.zip -d /workspace/r6-downloads/image
unzip -q /workspace/attachments/ca937dac-cea2-469f-9367-da7711a8ba32/lxr-r6-source-build-evidence.zip -d /workspace/r6-downloads/evidence
sha256sum /workspace/r6-downloads/image/* /workspace/r6-downloads/evidence/SOURCE.zip
readelf -l /workspace/r6-downloads/evidence/LXR_V2_STM32F765.elf
python3 /workspace/scratch/r6_verify_download.py
```

The temporary independent verifier checks outer SHA-256 values/ZIP integrity, all source Git blob hashes via git ls-tree, header/checksum/vectors, exact ZIP membership, ELF LOAD boundaries and payload bytes, and results/source identity. It reported PASS via successful assertions for all 388 blobs and four nonempty LOAD segments. Separate Python calculations evaluated the exact one-pole transfer ratio and extracted the reported band/return metrics; they did not change firmware.

## Publication and remaining listening decision

Source and build evidence are published; the candidate is suitable to hand over for a **separate installation/listening decision**. No host/CI blocker was found in the requested bounds/idle/resource checks. Musical success remains open: improved synthetic bass balance is evidenced, earlier desirable screaming at 80–90% physical travel is not.

Proposed listening questions for a separately authorized phase: does the new bass drive make useful overlap or suppress the scream; is the full-fader kick still desirable; is bass useful toward 50%; how do velocity and varied kits move the balance; do sparse/returning notes and asymmetric stereo sound acceptable; are real-time load and kit/FX transitions acceptable? These are questions, not hardware tests performed or instructions to install now.

R5 remains preserved with its two owner-confirmed idle-suppression behaviors. Main remains a43cd4fcda23ec3d577dc425261c3eb28bd5abff; R4 remains c870f7249d7ee0d65423bc8b7bfdb422dcc74482; R5 remains af3562fece090288a83727524c1035b79f4cf20b. lab-setup remains 5829398cea5d337200393757e39a37d4d8148256. No merge, force push, release, firmware flash, hardware test or repository-setting change occurred.

Report-only branch: **codex-reports/2026-10-29-r6-clp-shared-core-test-build**.
Exact report base: **5829398cea5d337200393757e39a37d4d8148256**.
Report head: the report-only commit containing this file; source changes are not included on that branch.
