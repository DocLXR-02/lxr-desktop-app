# R7 experimental CLP parallel-bass test build

Date: 2026-10-30 (UTC). Repository: https://github.com/DocLXR-02/lxr-desktop-app.
Task: implement authorized approach A from exact R5, run production-DSP host comparisons and one Release ARM build, and package an audition candidate.
**R7 is experimental and not hardware validated.** No installation or hardware test occurred in this task.

## Identity and delivery

| Item | Identity |
| --- | --- |
| Exact base | af3562fece090288a83727524c1035b79f4cf20b, R5 / fx/clp-input-silence |
| New source branch | fx/clp-parallel-bass-r7 |
| Published source commit | **6d7cb7f7763158ed9abc465a72df7f2560f26b57** |
| Successful CI | https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41098711091 |
| CI job | 124211127269, validate-and-build |
| Image artifact | [lxr-r7-image, 12998050644](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41098711091/artifacts/12998050644) |
| Separate source/build evidence | [lxr-r7-source-build-evidence, 12997750954](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41098711091/artifacts/12997750954) |

GitHub comparison verifies one commit ahead of R5, zero behind, merge base exactly R5. **R7 is not based on R6.** Both artifacts were available and unexpired when verified; recorded expiration is 2026-11-29.

| Delivered file | Bytes | SHA-256 |
| --- | ---: | --- |
| **LXR_V2_R7.img** | **287128** | `991f484b57a3367fe4a93728ff076bdfdd2ebd86e600e392f609bc149958f844` |
| **LXR_V2_R7.zip** | **205512** | `861b8244d7498d4ec30cca213f01caeaa7c2a80f1a516a2968a424f14ff7f6b2` |

LXR_V2_R7.zip contains **only LXR_V2_R7.img**, byte-identical to the separately delivered image. GitHub's outer image artifact ZIP wraps those two delivery files; it is distinct from the named image-only ZIP.

SOURCE.zip in the separate evidence artifact: 1703857 bytes, SHA-256
`13017222e5a90faefa7e4c034243a60f9b82b62b1f8810b59f44a3c632483d44`.

Outer artifact ZIPs were downloaded and checked against GitHub digests:

| Artifact wrapper | Bytes | SHA-256 |
| --- | ---: | --- |
| lxr-r7-image | 417915 | `5bd319a8336eedfa6e980bb7af2e7c9542e770ff31be4f0301cc34c1c221cafb` |
| lxr-r7-source-build-evidence | 2710888 | `35c4a033c79ff94486db8a37b6f623ed96c9db265e109e31ef1ccbd44d3d8848` |

## Context and authorization boundaries

Read root AGENTS.md and the [level-response analysis](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-clp-level-response-analysis/docs/codex-berichte/2026-10-29_clp-level-response-analysis.md), [R5 independent review](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r5-clp-independent-review/docs/codex-berichte/2026-10-29_r5-clp-independent-review.md), and [R6 handoff](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-29-r6-clp-shared-core-test-build/docs/codex-berichte/2026-10-29_r6-clp-shared-core-test-build.md). Current task authorization controls; historical report permissions do not broaden it. The explicit required R5 source base is used, with the report separately based on lab-setup.

DEVICE evidence supplied by owner: R5 suppresses unwanted sustained idle oscillation at fader zero and during missing steps. These specific behaviors were confirmed on hardware. New owner listening evidence: R6 bass strongly suppresses the desired scream, which emerges only near the bottom 10–20% of travel; that voicing misses the intended overlap. No other R6 device result is inferred.

R7 preserves R5's internal feedback voicing and introduces parallel bass outside the loop. The musical targets remain approximate: earlier useful scream around 80–90% physical travel and useful bass toward 50%. These are not calibrated parameter values, universal thresholds or host acceptance criteria. No instrument recordings were available.

## Topology and constants

All source references below refer to **6d7cb7f7763158ed9abc465a72df7f2560f26b57**.

```text
Original consumed bus input ─┬─ raw input detector (unchanged R5)
                            ├─ unchanged dry path ────────────────────┐
                            ├─ floating trim .22 → R5 CLP core ─┐     │
                            └─ LP 150 Hz → weight −.25 → vol ───┤     │
                                                 wet sum ──────┘     │
                                        existing outer d/w blend ────┘

Inside R5 core: trimmed excitation + lastOut × col × .125 × idle gain
 → unchanged HP/LP filters → drv → slew/rails → diode → ton → vol
 → lastOut (core only)
```

Core/Src/Soundengine/DSPAudio/FX/Distortion.c:202–216 defines:

```c
#define CLIP_INPUT_TRIM 0.22
#define CLIP_BASS_LP_ALPHA 0.021125671293435455134
#define CLIP_BASS_WEIGHT -0.25
...
return dist->clipBassLp_state[channel] * CLIP_BASS_WEIGHT * dist->p3;
```

The LP coefficient is 1-exp(-2*pi*150/44140), with a 150 Hz matched pole and ~1.06 ms time constant. It processes the **original untrimmed normalized int16 input**. This is a low-pass, not a new bass envelope or long release; it also passes DC. DC endpoints and eventual decay after their removal are tested.

Input trim is applied only as floating multiplication, after reading the original sample. Stereo integration is at lines 493–506; mono at 640–647. No integer input is replaced or re-quantized before detection. The original FX buffer is unchanged, and the original dry/wet arithmetic and routing remain.

The wet bass is added in double precision to the core output **after lastOut has been assigned**, at lines 620–629 (stereo) and 681–687 (mono). Only the resulting wet sum is converted through the existing int16 conversion. It cannot enter the core return. No integer wet-component sum can overflow before conversion.

The R5 core topology, coefficients, nonlinear stages and feedback multiplier are unchanged. No R6 split input filter is carried over. The original high-pass coefficient remains, including its historical misleading “7.2Hz” comment; the analysis already established the coefficient's much higher pole frequency. This task does not correct or retune that core.

The .22 trim is about -13.15 dB. With the unchanged squared fader law, u=.85 supplies core excitation equivalent in amplitude to about .399 of untrimmed electrical travel. This motivates the starting constant; feedback phase, input quantization, kit/velocity levels and detector history prevent a universal physical fader mapping.

## Bass weight selection, phase and headroom

**The shipped bass polarity is inverted: weight −0.25.** This is a fixed experimental coefficient, not an adaptive process or a new control.

Initial positive-weight comparisons exposed cancellation with the filtered/nonlinear core. Host-only variants of the actual production source changed just the fixed weight: 0, ±.125, ±.25 and ±.30. Seven variants × three signal types × three levels give 63 comparisons. Their core-return fingerprints remain equal to the shipped core; changing bass weight does not alter regeneration internally.

Representative mono kick, drv=63/col=7/ton=127/vol=127, fully wet. Body low-band RMS is 30–150 Hz over the first 180 ms:

| Bass weight | At 85% travel proxy | At 50% travel proxy | Peak at 85%, LSB |
| --- | ---: | ---: | ---: |
| 0, trimmed core only | .245164 | .090178 | 21216 |
| +.125 | .236324 | .087142 | 21746 |
| +.25 | .227547 | .084134 | 22537 |
| +.30 | .224059 | .082939 | 22965 |
| −.125 | .254050 | .093240 | 23200 |
| **−.25** | **.262931** | **.096324** | **24960** |
| −.30 | .266484 | .097563 | 25613 |

The negative sign also improved measured low-band energy relative to the trimmed-only core for the selected 60 Hz and mixed-tone probes. At 50%, mixed-tone low RMS over 480 ms was .047550 with zero weight, .032854 with +.25, and .064220 with −.25.

The −.25 magnitude gives a useful measured contribution while retaining more headroom than −.30. No listening result or universal phase alignment is inferred. Different kicks, controls, tone settings and partial dry mixes can reinforce or cancel differently. In particular, the inverted low-pass component can interact with the original dry path. No phase tracker, automatic normalization, compressor, added limiter or compensation EQ was introduced.

Headroom bound for ordinary controls in [0,1] and cleared runtime histories:

- The retained bounded diode/tone core has magnitude at most approximately .647479 × vol.
- A first-order LP with coefficient between 0 and 1, fed normalized int16 input, has magnitude at most 1.
- Thus bass magnitude is at most .25 × vol, and combined wet magnitude at most **.897479 × vol**.
- This leaves **.102521 full-scale amplitude** before integer saturation (about .94 dB headroom). Weight magnitude .30 would leave only .052521.
- The sum is performed in double, multiplied by 32768 and converted through the existing path. No new limiter is needed to obtain that bound.

Per-sample full-regeneration stress observed maximum unquantized wet magnitude **.897471368**, below the bound. The principal render matrix's maximum quantized wet output was **29062 LSB**. The bound is not a guarantee against saturation when other routed voices are added downstream; original output accumulation is retained.

## Lifecycle and controls

Core/Inc/Soundengine/DSPAudio/FX/Distortion.h:47–49 adds clipBassLp_state[2], two runtime doubles (**16 bytes**), outside lastOut. No kit parameter or serialized layout changes.

Normal initialization clears both new states (Distortion.c:232–233). Existing audio-owned CLP entry clears them with the other histories (lines 163–179). Reset requests still write only clipStateActive=0 (157–160), so the bass tail is not asynchronously cleared by a foreground request. The next CLP audio entry performs the clear. Tube/Fold/non-distortion processing invalidates entry using the existing paths.

The inactivity detector at lines 182–199 is identical to R5: original consumed input, ±1-LSB tolerance, linked stereo, 441-frame hold and the same decrement 1.f/221.f. Exact zero is reached on subtraction 222 due to float rounding, as in R5. The new bass LP naturally settles after input loss; it is not held open by its output or the core oscillator. No 0.5–1 s release is implemented.

drv/col/ton/vol/d/w and kit formats remain. vol retains its existing role **inside the core loop** and also scales the bass contribution. ton still filters the core; the added bass bypasses that filter by design. Therefore total audible control response changes even though controls and formats do not. Zero-vol and zero-wet behavior are compared against R5, retaining its original integer blend behavior rather than assuming a new exact-zero property at every int16 endpoint.

Global fader law, mixer/routing, voices, other effects and storage algorithms are unchanged. Full-kit/FX reset call sites are byte-identical to R5. The prior recall sequencing limitation remains: reset is requested at resend start, not at an atomic completed-recall boundary. No click-free recall claim follows.

Splash: **LXR LAB R7-CLP**, Core/Src/main.c:354.

## Actual host checks

Native checks compile actual production R7 DSP and exact pinned R5 source from its Git object. They use ASan/UBSan/bounds/float-cast-overflow sanitizers with nonrecovering diagnostics, -O2 and -fno-strict-aliasing. Local native GCC: Debian 14.2.0; CI native GCC: Ubuntu 13.3.0. NumPy 2.3.5 was used locally and pinned in CI.

Final local and CI results passed:

- Unmodified inherited tests/fx_clip/unit.c: diode endpoint/range, cold silence at high controls, independent state, hold/fade/reopen and algorithm re-entry.
- Unmodified inherited tests/fx_clip/fxbus.c: actual bus dispatch/reset across six routes, same-type reset request, FX_OFF and re-entry. Full live kit audio is not simulated by the storage fixture.
- New state fixture: poisoned-instance initialization; deferred reset leaves history until audio entry then clears it; linked/mono consumed-channel reopening at magnitude 2; hold/fade exactness; returns during hold/fade/after settling; Tube/Fold re-entry; input-buffer preservation; per-sample finite/headroom/idle checks with int16 endpoints, sign changes and low tones at maximum regeneration.
- **318 principal R5/R7 render pairs**, each 1.5 seconds at nominal 44140 Hz, including control combinations, seven squared fader-equivalent levels, kicks, 60 Hz and mixed 60/1400 Hz tones, sparse/returning notes, natural decay through int16 zero, downward loss ramps, full-scale alternation, impulses, sustained DC endpoints and low-drive probes; mono, identical stereo and asymmetric stereo.
- All principal R7 renders reach exact quantized silence for the final 0.5 seconds. Maximum time from the actual last nonzero consumed input sample to last nonzero output: **36.701405 ms** for R7, **41.753512 ms** for R5 in this expanded matrix. The suite requires <100 ms for these finite probes; this is not a universal acoustic deadline.
- Nine exact output identity pairs: Tube/Fold and seeded return-only CLP with cleared bass history across mono/identical/asymmetric modes. Zero-wet output is also identical across three modes.
- **45 zero-volume pairs** across five scenarios, three channel modes and three d/w amounts match R5.
- Original input buffer remains byte-identical in every render. Finite-state checks, buffer guard words and sanitizers pass.
- Selected kick/sparse/natural-decay/return renders are output-identical at block sizes 1, 7 and 64.

A third reference, **TrimR5**, is actual pinned R5 with only the same floating .22 input trim inserted; the original detector/buffer remain unchanged. In every principal case, the R7 core-return fingerprint sampled at block boundaries matches TrimR5. This is a block-boundary trace check, not storage of every internal sample. The seeded return-only fixtures additionally compare complete output waveforms exactly. A static recovery assertion removes the documented R7 additions and requires the entire process function to equal R5, protecting the retained filters/nonlinear core against unnoticed edits.

No inherited assertion was weakened. The original R5-vs-bounded-R4 continuous-equality render is not an R7 timbre criterion; suitable original unit/bus fixtures are reused instead.

Relevant existing regressions passed locally and again in CI:

- tests/bug001/run.py: CC0/NRPN/Drive/mutes/bounds, all 256 morph positions and immediate sweep/audio service cadence.
- tests/kit_load/run.py: UI notification/expiry/interleaving plus 512 actual RAM load/menu cases.
- tests/project_storage/run.py: production storage/failure/containment groups and whole-FatFs RAM tests.
- tests/euclidean/run.py: all six groups, including 2176 masks, playback, persistence, malformed and protocol boundary cases.

The kit/menu/storage fixtures mock audio callbacks. Their pass and unchanged reset call sites do not establish live recall continuity or real interrupt scheduling.

## Measurements and remaining musical limits

Evidence r7-host/results.json contains all cases, weight comparisons and metrics. Selected unnormalized WAV pairs cover kick, sparse, natural-decay, return and fader-loss cases. They are synthetic renders, not instrument recordings.

Band RMS uses a Hann window, normalized int16 samples: low 30–150 Hz, high 500–5000 Hz. The original 480 ms window includes much of the feedback tail; new body_ metrics use 180 ms to examine the short kick body. Both are retained. Raw peaks and largest sample steps are unwindowed LSB.

Representative kick at drv=63/col=7/ton=127/vol=127, fully wet, mono:

| Travel proxy | R5 body low RMS | TrimR5 body low RMS | R7 body low RMS | R5 high RMS, 480 ms | R7 high RMS, 480 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| 100% | .508185 | .303527 | .326894 | .156530 | .199577 |
| 85% | .478638 | .245164 | .262931 | .171082 | .202582 |
| 50% | .326715 | .090178 | .096324 | .197932 | .206096 |
| 40% | .246061 | .054542 | .058491 | .202452 | .206360 |

**The bass branch adds measured low-band energy relative to the trimmed core, but this representative kick still has substantially less bass than untrimmed R5.** Thus useful overlap and bass toward 50% are listening questions, not achieved host-test claims. No universal 80–90% screaming threshold is claimed.

At 85%, R7's high-band RMS is .202582 with col=7 versus .000622 with col=0 at otherwise identical controls. This shows strong regenerative influence in that probe. High-band energy alone cannot distinguish desirable screaming from unwanted harshness. Unlike R6, the parallel bass cannot internally suppress the core by entering its nonlinear feedback path, but it can perceptually mask the scream and interact in the output sum.

Largest principal-matrix sample step: **41823 LSB**. In the selected asymmetric return WAV, over .60–.68 s, R5 peak/step are 21216/18398 LSB; R7 peak/step are 24960/8908 LSB. These measurements do not prove click-free or clean re-entry. No attack fade was added; core/filter histories and added bass change the returning waveform.

Still requiring actual listening/device evidence: perceived bass and scream overlap across varied kits/velocities, physical fader calibration, partial dry mixes and phase interaction, real voice-chain tails, early return transients, linked stereo musical behavior, live kit/FX transitions and real-time CPU/stack/underruns. Linked activity on one side still holds both core returns open; eventual idle silence applies after all consumed linked input becomes inactive. The historical R4 start-at-col7 / continue-at6 / stop-at5 distinction is not replaced by these host probes.

## One Release ARM build and resources

CI executed exactly one Release ARM build of the final source. No local ARM build and no R5/R6 ARM rebuild were run.

Compiler: **Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174), arm-none-eabi-gcc 14.3.1 20250623**. This matches the recorded R5 compiler version; Release CMake settings are unchanged. Host compilers differ as documented above. Toolchain archive SHA-256 is captured in the evidence; download is version-named but does not enforce an independently supplied expected archive digest.

| Region | Recorded R5 | R7 CI | Delta | R7 free |
| --- | ---: | ---: | ---: | ---: |
| ITCM, 16384 B | 15104 | 15104 | 0 | 1280 |
| DTCM, 131072 B | 107400 | 107416 | +16 | 23656 |
| RAM, 393216 B | 274648 | 274648 | 0 | 118568 |
| Linker FLASH, 1015808 B | 234764 | 235012 | +248 | 780796 |

R7 .text=151040, .rodata=82592, .dtcm=36996, .dtcmz=70416, .itcm_text=15104. The image grows from R5's recorded 286880 bytes to 287128 (+248). R5 figures are reused recorded build evidence, not a newly rebuilt baseline.

Linker FLASH accounting omits separately loaded ITCM/DTCM bytes; ELF checks cover these too. **Memory fit is not evidence of real-time CPU headroom.** New double-precision LP arithmetic and output mixing need on-device timing assessment.

## Downloaded artifact verification

CI package.py and an independent local audit of downloaded artifacts passed:

- LXRV2IMG magic; 287112-byte payload equals image size minus 16-byte header.
- Internal checksum **0xA3**, equal to 255-(payload byte sum & 255); reserved bytes zero.
- Exact R7 splash identity present.
- Initial SP **0x20080000**, aligned and in configured SRAM boundary; reset vector **0x08013555**, Thumb bit set and in application flash.
- Every nonempty ARM ELF LOAD segment lies in application flash and matches the image payload bytes at its physical load address.
- Four nonempty ranges, exclusive ends: 0x08008000…0x080412b0; 0x080412b0…0x08041604; 0x08041604…0x0804a688; 0x0804a688…0x0804e188. No nonempty load bytes enter bootloader flash.
- Image-only ZIP list, integrity and embedded bytes match the delivered image.
- Both outer artifact ZIP digests match GitHub metadata.
- SOURCE_COMMIT.txt identifies the published commit. All **388 archived source blobs** match its Git blob hashes. SOURCE.zip integrity passes, and results' source SHA-256 matches production Distortion.c.

Source/build evidence includes full SOURCE.zip, exact commit, ELF/map, compile_commands.json, ARM/native toolchain versions, toolchain archive hash, section report, configure/build and test logs, measurement JSON, WAVs, validation JSON and checksums. It is separate from image delivery.

This confirms source/archive and ELF/image consistency, not an independent second ARM rebuild or successful hardware installation.

## Actual commands, changes and failures

Source worktree: /workspace/r7-source; original checkout and R6 worktree were preserved. Relevant commands actually executed:

```sh
git fetch origin fx/clp-input-silence fx/clp-bass-feedback-r6 lab-setup
git worktree add -b fx/clp-parallel-bass-r7 /workspace/r7-source af3562fece090288a83727524c1035b79f4cf20b
python3 tests/fx_clip_r7/run.py --output build/r7-host
python3 tests/fx_clip_r7/run.py --output build/r7-weight-analysis
python3 tests/bug001/run.py
python3 tests/kit_load/run.py
python3 tests/project_storage/run.py
python3 tests/euclidean/run.py
git -c core.whitespace=cr-at-eol diff --check
git push -u origin fx/clp-parallel-bass-r7
```

The host suite ran initially with positive .25, then for signed-weight/body-window comparisons, then again with the final −.25 (same first command). All completed passes; final source and CI include all weight comparisons. The positive-weight result was a **voicing limitation**, not a sanitizer or idle failure. It motivated the documented fixed polarity choice. There were no test assertion failures, build failures, resource overflows or rejected pushes in this task.

Only nine repository files differ from R5:

- Core/Inc/Soundengine/DSPAudio/FX/Distortion.h; Core/Src/Soundengine/DSPAudio/FX/Distortion.c; Core/Src/main.c.
- tests/fx_clip_r7/README.md, state.c, render.c, run.py, package.py.
- .github/workflows/clp-r7-build.yml, scoped only to the new source branch.

Production source CRLF line endings were retained. No old workflow, test, storage, voice or routing source was modified.

CI repeated final host/regression commands and ran:

```sh
python3 -m pip install numpy==2.3.5
cmake --preset release
cmake --build --preset release --parallel 2
python3 tests/fx_clip_r7/package.py
arm-none-eabi-size -A build/release/LXR_V2_STM32F765.elf
arm-none-eabi-gcc --version
git archive --format=zip --output=build/r7-evidence/SOURCE.zip HEAD
```

CI logs show each step succeeded for the exact source commit. Local final logs are /workspace/scratch/r7-host.log, r7-weight-analysis.log, r7-bug001.log, r7-kitload.log, r7-storage.log and r7-euclidean.log.

Downloaded verification commands:

```sh
unzip -q /workspace/attachments/d5e35185-fe12-4c70-a9c4-606fc57fd4ef/lxr-r7-image.zip -d /workspace/r7-downloads/image
unzip -q /workspace/attachments/2eec4185-9220-441f-bde0-636145f07722/lxr-r7-source-build-evidence.zip -d /workspace/r7-downloads/evidence
sha256sum /workspace/r7-downloads/image/* /workspace/r7-downloads/evidence/SOURCE.zip
readelf -l /workspace/r7-downloads/evidence/LXR_V2_STM32F765.elf
python3 /workspace/scratch/r7_verify_download.py
```

The temporary independent verifier asserts ZIP digests/integrity, exact ZIP membership, header/checksum/vectors, ELF load/payload bytes, Git source blob hashes via git ls-tree, and result/source identity. It completed successfully with 388 source blobs and four nonempty flash segments. Separate Python reads of the delivered JSON/WAVs produced the measurement tables and return-window values.

## Publication and handoff

No idle persistence, invalid-output, memory-bound or resource-fit blocker was found in the requested checks. The experimental files and evidence are delivered for a **separate installation/listening decision**. No statement of musical success or authorization to install follows from host/CI passes.

Main, R4, R5, R6 and lab-setup remain unchanged, verified remotely:
main a43cd4fcda23ec3d577dc425261c3eb28bd5abff;
R4 c870f7249d7ee0d65423bc8b7bfdb422dcc74482;
R5 af3562fece090288a83727524c1035b79f4cf20b;
R6 a743444c01c622dd6a7417a36fb7faa317147310;
lab-setup 5829398cea5d337200393757e39a37d4d8148256.

No merge, force push, release, repository-setting change, firmware flash or hardware test occurred.

Report-only branch: **codex-reports/2026-10-30-r7-clp-parallel-bass-test-build**.
Exact report base: **5829398cea5d337200393757e39a37d4d8148256**.
Report head: the commit containing this file; the report branch contains no source-candidate changes.
