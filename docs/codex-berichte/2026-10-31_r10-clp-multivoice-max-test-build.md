# R10 experimental shared CLP maximum-fader test build

Date: 2026-10-31 (UTC). Repository: https://github.com/DocLXR-02/lxr-desktop-app.
**R10 is experimental and not hardware validated. Installation/listening requires separate later owner approval. This policy is not finally accepted and the development stage is not frozen.** No flashing or hardware testing occurred.

## Identity and delivery

| Item | Identity |
| --- | --- |
| Exact source base | `e2d7963d97cd2d5918e3b80b88156fbbfaa60f47`, R9 / fx/clp-fader-curve-r9 |
| New source branch | `fx/clp-multivoice-max-r10` |
| Published source head | `1f7f4e99cf9d5d14ca859538f478ca26eb369238` |
| CI | https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41176085244 |
| CI job | 124463829310, validate-and-build |
| Image artifact | [lxr-r10-image, 13032722413](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41176085244/artifacts/13032722413) |
| Separate complete source/build evidence | [lxr-r10-source-build-evidence, 13032752328](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41176085244/artifacts/13032752328) |

CI run 41176085244 completed **successfully on attempt 1**, at exactly the published source head. It is the only workflow run for the new source branch at verification. Artifacts were available/unexpired; metadata records expiry 2026-11-30.

| Delivered file | Bytes | SHA-256 |
| --- | ---: | --- |
| **LXR_V2_R10.img** | **290144** | `833a906beabff382169fa1a621a8cfb49a5de8c696d85866a0fbdb893d2856bf` |
| **LXR_V2_R10.zip** | **206798** | `633aa06eee836a7837bb208675c244699fb0d5ed0a77c4e29a789f268d46f26b` |

The named ZIP contains **only LXR_V2_R10.img**, byte-identical to the separately delivered image. GitHub's outer image artifact ZIP contains both delivery files and is a separate wrapper.

SOURCE.zip in the separate evidence artifact is **1753026 bytes**, SHA-256 `532df9d572f8ecc6e4173a855eb5fefd032dc69366369ef937a7ec76ea85ce8b`.

Downloaded outer wrappers match the SHA-256 values published by GitHub artifact metadata/upload logs:

| Wrapper | Bytes | SHA-256 |
| --- | ---: | --- |
| lxr-r10-image | 420470 | `d104a98702927b22a0cb069dcc18e059f34eed0548fa7a4daacebdd3063f8d8a` |
| lxr-r10-source-build-evidence | 8940738 | `9608e738758838af30a545cb06039995d9f72b75ce09eb7038fe4bf09079e4ca` |

## Scope, owner evidence and prior-work check

Read root AGENTS.md, the [R9 handoff](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-30-r9-clp-fader-curve-test-build/docs/codex-berichte/2026-10-30_r9-clp-fader-curve-test-build.md), and the [multi-voice design at exact report commit d8740b4133185ce61e38e67f0889207f9417ecd0](https://github.com/DocLXR-02/lxr-desktop-app/blob/d8740b4133185ce61e38e67f0889207f9417ecd0/docs/codex-berichte/2026-10-30_clp-multivoice-control-design.md). This task's explicit pinned source base controls; the separate report branch uses exact lab-setup.

**DEVICE evidence supplied by owner:** R9 was tested on the instrument and its solo sound/fader response is approved as the reference. This supersedes the historical R9 build report's installation-pending status; it does not establish R10 behavior on hardware or comprehensive validation of every R9 path.

The owner explicitly authorized this Maximum prototype to judge useful/disruptive performance cases, including the **silent-high-fader compromise**: a configured but silent, muted or velocity-zero FX-routed voice can hold the entire shared core output open by its fader position. R10 does not identify currently sounding contributors. An individual voice's new excitation can be faded, but no independent processed wet stem can be recovered from the shared nonlinear/feedback bus. This experiment does not resolve that architectural limitation or add activity tracking.

Before editing, searched workspace filenames, local branches and the complete remote branch inventory for R10. No existing R10 work was found. Saved all **66 pre-existing remote heads**; R9, design-report and lab-setup refs matched the requested commits. Created the new worktree directly from pinned R9. Remote source comparison confirms one commit ahead, zero behind and merge base exactly R9. No existing branch head is modified.

## Implementation and precise references

References below use head `1f7f4e99cf9d5d14ca859538f478ca26eb369238`.

Only two production files change:

- Core/Src/Soundengine/FxBus.c:51–79 replaces the multiple-route unity fallback with a maximum over private smoother replays. The existing map at lines 42–48 is byte-identical to R9.
- Core/Src/main.c:354 changes splash to **LXR LAB R10-CLP** (15 characters).

```text
For configured routed voices i, each frame n:
qi[n] = calcOnePole(private_copy_of_actual_live_filter_i, captured_squared_target_i)
qmax[n] = max_i clamp(qi[n], 0, 1)
gain[n] = h(qmax[n])

h(q) = 0                 q <= 0
       1                 q >= .25
       2 * sqrtf(q)      otherwise

No configured FX route: return NULL, preserving unity fallback.
```

The helper packs up to six actual OnePoleFilter copies and their voice indices in automatic local arrays. Only routed voices are copied/replayed. Clamping changes the returned frame value, not the private or live filter history. For each frame it bounds each result, takes the maximum and invokes h **once after aggregation**. At most one square root is needed per frame. There is no summed-fader rule, normalization, new activity/threshold logic, persistent smoother or live-filter write. A zero-size request performs no frame writes; zero routed voices returns NULL.

| Settled physical faders | Expected additional core gain | Production check |
| --- | ---: | --- |
| Two .25/.25 | .5 | Passed within 1e-7 |
| Six at .25 | .5 | Passed within 1e-7 |
| .4/.2 | .8 | Passed within 1e-7 |
| Sounding .2, silent .8 | 1 | Passed |
| Sounding .2, silent zero | .4 | Passed within 1e-7 |

At steady state qi=ui², hence gain=min(1,2×max ui): full gain from highest physical travel .5 upward, independent of route count. Real ADC snap/quantization/hysteresis and existing smoother transients still apply. A lower silent fader below the current maximum has no effect; a silent high fader does. Exactly one routed voice retains R9's map and sample trajectory.

### Preserved ownership and signal path

Core/Src/Soundengine/DSPAudio/mixer.c is unchanged: actual live filter pointers and existing squared targets are captured at 549–562; capture at 618–622 precedes sync voice advancement; line 732 consumes the existing 64-float gain buffer synchronously. No pointer is retained. All six voice sources and 1PoleLp.c are unchanged: r=.01 for voices 1–3, r=.1 for snare/cymbal/hat, with actual calcOnePole cadence preserved. The existing ADC law, voice synthesis, routing/saturating summing, serialized parameters, storage and UI remain unchanged.

**Distortion.c is byte-identical to pinned R9.** Stereo lines 625–634 and mono 688–693 compute unattenuated lastOut, then form audible wet=lastOut×gain+bass before the existing dry/wet blend. Feedback histories never receive the attenuated output. R7/R9 bass (150 Hz LP, weight −.25, existing VOL), dry input, .22 excitation trim, filters, nonlinear stages and all controls are preserved. Original-input ±1-LSB inactivity policy, linked stereo, 441-frame hold and existing short float fade remain unchanged. First usable returning input reopens through the same detector. No history reset is added for fader motion or maximum-dominance crossovers.

The gain remains linked for mono/stereo and captured only for CLP; other effects/distortion types retain their dispatch and processing. Fresh per-block routing selection prevents stale gain. FX/kit changes retain the existing queued/audio-owned reset lifecycle. Full kit recall still resends parameters over existing service boundaries rather than atomically committing a new audio state; native fixtures do not establish actual recall scheduling continuity.

**Routing transitions are immediate.** Adding/removing a dominating fader can step the shared level; changing dominant voice during continuous trajectories preserves gain continuity but can change its slope. Removing the last route restores the existing NULL/unity fallback and can expose a brief residual idle tail. No crossfade or empty-set gate was added. Silent routed voices are never reclassified as absent. The existing single-audio-writer/foreground target scheduling contract is retained; no new interrupt-interleaving proof is claimed.

## Actual host checks and results

Native tests compile actual R10 DSP/helper/pole and pinned R9 references. The actual mixer capture function is extracted verbatim into six filter-state fixture objects; the full engine compiles in the ARM build. Multi-voice renders use the actual helper, an extracted pinned-R9 helper with renamed symbols, and the verbatim mixer adder. Synthetic sources use fixed pans and source×smoothed-q samples; they do not reproduce the entire voice engine or physical ADC polling/hysteresis.

Flags include -O2, -fno-strict-aliasing, ASan/UBSan/bounds/float-cast-overflow and nonrecovering failures. Local native GCC is Debian 14.2.0; CI native GCC/version is recorded below. NumPy is pinned to 2.3.5.

Passed locally and in final CI:

- Unmodified inherited diode/detector/reset/headroom fixtures and **216000** gain-varying DSP/component steps: complete histories, original input, bass/dry arithmetic, VOL/D/W endpoints, unity and Tube/Fold remain unchanged.
- Actual production map: physical table, knee neighbors, 100001-point monotonic/bounded mapping sweep, negative/over-unity bounds and smallest positive host float. Host subnormals are not a promise of device flush-to-zero behavior.
- **144000 per-frame complete DSP and output equality checks** for all six solo voices against pinned R9 helper trajectories, mono/stereo and both actual smoothing rates. No live filter is advanced for metadata.
- All **64 routing masks × six bus destinations (384 combinations)**: capture eligibility now means nonempty routed set; complete live-filter contents stay unchanged; independent oracle gains and actual bus dispatch/output/state match expected results.
- Every mask's metadata matches across **1/7/64-frame partitions**, with fixed target-event boundaries, near-zero values and trajectories that exercise changes in maximum dominance. Gain guard words pass. All 36 sole-voice/output lifecycle cases cover resets, FX exit/re-entry, no-route fallback, silent high/zero extra routes and new sole associations.
- **108 pinned-R9/R10 bus pairs**, with **96 solo/empty/unity pairs** requiring byte-identical output. The new 384-combination tests exercise changed multi-voice gain rather than retaining the old multi-route fallback assertion.
- **318 unity paired renders** sample-identical to pinned R9, with matching full-state fingerprints: kicks, low/mixed tones, natural decays, sparse/returning notes, squared input-level sweeps, mono/asymmetric stereo, impulses/DC and regeneration extremes. Maximum settling after last nonzero consumed input is **36.701405 ms**, with final .5 s exactly silent.
- **72 moving solo pairs** match pinned R9 output and mean gain exactly, with matching complete-state fingerprints. Static targets, **54 zero-VOL** pairs, **18 zero-D/W** pairs and other distortion probes retain baseline behavior. Selected waveforms match 1/7/64 partitions.
- **44 principal multi-voice runs**, two control sets and mono/asymmetric stereo: unequal faders fading either voice; silent high/low/zero routes; collective fades/returns; sparse/alternating notes; six-voice and hot mixtures; and route changes. **6,217,728 per-frame complete-state comparisons** against R9/unity references pass despite different audible gains. Four additional endpoint runs cover zero D/W and zero VOL at D/W=0/.4/1. All captured-input waveforms/final states match 1/7/64 DSP partitions.
- Multi-voice final .1 s is exact quantized silence; worst settling in that matrix is **35.523335 ms**. No persistent idle output, invalid history or sanitizer failure is observed. These are synthetic-case bounds, not universal hardware deadlines.

Existing regression commands all pass locally and in CI: bug001 (CC/NRPN/Drive/mutes/bounds, all 256 morph positions and cadence); kit_load (notification/interleaving and 512 RAM/menu cases); project_storage (production failure/containment and RAM FatFs groups); euclidean (all six sanitizer groups). Storage/menu fixtures mock audio callbacks and do not validate live recall sound.

No inherited test file or unrelated assertion is weakened. New R10 expectations explicitly replace count==1 eligibility with nonempty/max behavior, retain direct solo equality, and stop treating multiple routes as a required unity fallback. Static checks allow only helper/splash production changes, recover the entire original bus outside the helper and require unchanged R9 map/mixer/DSP.

### Audible metrics versus internal invariance

Representative normalized stereo RMS at drv/col/ton/vol=63/7/127/127, 100% wet. Fading windows are .40–.48 s; silent-route windows .10–.18 s. These are synthetic comparisons, not musical acceptance:

| Scenario | R9 gain / output RMS | R10 gain / output RMS |
| --- | --- | --- |
| Fade lower of .4/.2 | 1 / .427634 | .799998 / .342103 |
| Fade higher of .4/.2 | 1 / .419513 | .428068 / .179998 |
| Silent .8, sounding .2 | 1 / .416499 | 1 / .416499 |
| Silent zero, sounding .2 | 1 / .416499 | .399999 / .166592 |
| Silent .1, sounding .2 | 1 / .416499 | .399999 / .166592 |
| Collective two-voice fade | 1 / .419524 | .420769 / .177232 |
| Sparse/alternating .2/.15 | 1 / .416658 | .399999 / .166658 |
| Six-voice mixture fading | 1 / .417973 | .315576 / .132415 |

Internal histories/bass trajectories stay identical between policies for identical consumed samples; these audible differences do not change regeneration internally. The high-silent case demonstrates the owner-authorized prototype compromise rather than a newly detected activity condition. In route probes, adding a silent zero route no longer changes .399999 gain to unity; raising that silent route above the current maximum does raise shared gain.

The principal multi-voice matrix peaks at **27535 LSB** with maximum adjacent-sample step **37050 LSB** for R9 and R10. The deliberately hot mixture records up to **115820 per-add/channel saturations** in the unchanged upstream mixer. Downstream gain cannot undo that clipping. These steps include nonlinear signal transitions and do not isolate clicks. Bounded gain does not guarantee monotonic total RMS or per-sample magnitude because of dry/bass phase interaction. No normalization, compression or new limiter is added.

Evidence includes JSON/CSV results, all-mask gain trajectories and paired synthetic WAVs. Musical quality, useful versus disruptive silent-high-fader cases, return attacks and full-load device performance remain listening/device questions.

## One Release ARM build, resources and stack/work inspection

Exactly **one final Release ARM build** ran, in CI attempt 1. No local ARM build or R9 rebuild was performed. Compiler: **Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174), arm-none-eabi-gcc 14.3.1 20250623**, matching recorded R9. Toolchain archive SHA-256 `8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd` matches the downloaded R9 evidence. The workflow records that digest but does not enforce an independently supplied expected archive hash.

CI native GCC is Ubuntu 13.3.0. ARM settings remain the established -O3, Cortex-M7, hard-float fpv5-d16, -ffast-math and -freciprocal-math. Native sanitizer processing does not execute those ARM instructions or reproduce device flush-to-zero mode. The local/CI multi-voice metrics CSV files are byte-identical.

| Region | Recorded R9 | R10 CI | Delta | R10 free |
| --- | ---: | ---: | ---: | ---: |
| ITCM / 16384 B | 15608 | **15608** | **0** | **776** |
| DTCM / 131072 B | 107672 | **107672** | **0** | **23400** |
| RAM / 393216 B | 274648 | **274648** | **0** | **118568** |
| Linker FLASH / 1015808 B | 237388 | **237524** | **+136** | **778284** |

.text=153528 B (+136); .rodata=82616, .dtcm=36996, .dtcmz=70672 and .itcm_text=15608 are unchanged. Image size 290008→**290144 B**, +136. No placement or unrelated optimization change was used to fit resources.

Inspected delivered full disassembly and R9/R10 ELF symbol tables:

- FxBus_clipFaderGains remains in application FLASH at **0x080266b0**, size **276→412 B**, +136. The existing ITCM call veneer remains 8 bytes; the helper is not moved into ITCM.
- Its fixed own stack frame is **48 B R9 → 168 B R10**, **+120 B**. R10 prologue saves nine general registers (36 B), d8–d9 (16 B), and reserves 116 B. R9 saved six general registers (24 B), d8 (8 B), and reserved 16 B. These are this function's frame sizes, excluding caller buffers, callee/interrupt frames and whole-engine high-water. The fixed prologue also runs on the empty-route path.
- Local filter storage is six 16-byte OnePoleFilter copies (**96 B**, versus R9's one 16-byte copy), plus six route indices and bookkeeping/padding. No persistent DSP/bus state, heap allocation or new DTCM buffer is added.
- The per-voice loop calls production calcOnePole through its veneer, then uses hardware `vminnm.f32` / `vmaxnm.f32` for bounds and maximum (0x080267e0–0x080267e8). The mapper occurs after that inner loop.
- At **0x08026806**, one `vsqrt.f32` handles 0<qmax<.25; **0x0802680a** doubles via `vadd.f32`. Zero/unity branches bypass sqrt. There is no sqrtf library call in this path and no per-voice mapping.

For six routed voices, each 64-frame block now needs up to **384 private smoother calls** and **64 hardware square roots**, plus selection/copy/bounds/maximum work. R9's multiple-route fallback did no such replay work; solo still needs one replay per frame with added loop/bounds overhead. Non-CLP capture and empty-route fallback do not run the replay/map loops. Every routed copy advances even if another already reaches unity; no early exit leaves a later frame's private history stale.

**Memory fit and host runtime do not prove device CPU/deadline or stack capacity.** No device cycle count, underrun/load test, stack high-water or interrupt-interleaving measurement was made. The extra fixed frame and repeated replays are material performance changes to assess after separate installation approval. ITCM remains tight. No new diagnostic or architectural workaround was introduced.

## Artifact/source correspondence and downloaded-byte validation

Both CI package.py and an independent audit of downloaded artifacts passed:

- **LXRV2IMG** magic; payload **290128 bytes**, equal to image size minus the 16-byte header.
- Internal checksum **0x99**, recomputed as 255-(payload byte sum & 255); three reserved bytes zero.
- Exact **LXR LAB R10-CLP** splash present, R9 splash absent.
- Initial SP **0x20080000**, 8-byte aligned and within configured SRAM boundary; reset **0x08013555**, Thumb bit set and inside application load bytes.
- Nonempty ELF physical LOAD ranges (exclusive ends): **0x08008000–0x08041c80**, **0x08041c80–0x08041fd4**, **0x08041fd4–0x0804b058**, **0x0804b058–0x0804ed50**. All fit application flash below 0x08100000; no bootloader load bytes occur.
- Those segments cover the whole payload contiguously, and every ELF load byte equals its image byte. Zero-file-size BSS/heap sections are not image payload. This includes separately loaded DTCM/ITCM bytes, which linker FLASH usage alone omits.
- Named ZIP integrity, exact single-image membership and byte identity pass. Image/ZIP sizes and hashes match printed CI package logs, validation JSON and SHA256SUMS after download. Outer wrapper sizes/hashes match GitHub metadata.
- SOURCE_COMMIT.txt and artifact metadata identify the published source head. All **410 source blobs**, their Git hashes and exact file-member set (excluding ZIP directory entries) match that head. SOURCE.zip integrity/size/SHA-256 pass.
- Host reference fields identify pinned R9; source hashes in principal/multi-voice results match delivered DSP/helper/pole/mixer. No source/artifact correspondence discrepancy was found.

Separate evidence contains SOURCE_COMMIT.txt, SOURCE.zip, ELF/map, compile_commands.json, native/ARM versions, toolchain archive hash, configure/build/test logs, JSON/CSV/WAV results, disassembly, ELF load/section reports, package validation and SHA256SUMS. Correspondence checks are not a second independently reproduced ARM binary or hardware validation.

## Files, actual commands and failures

Ten files differ from pinned R9: 788 insertions / 7 deletions.

- Production: Core/Src/Soundengine/FxBus.c; Core/Src/main.c.
- New tests/evidence: tests/fx_clip_r10/README.md, integration.c, multivoice.c, multivoice.py, package.py, render.c, run.py.
- New branch-scoped workflow: .github/workflows/clp-r10-build.yml.

Production CRLF endings are retained. Existing source/test/workflow files outside the two production edits are unchanged. Actual local commands:

```sh
git ls-remote --heads origin
git fetch origin fx/clp-fader-curve-r9 lab-setup codex-reports/2026-10-30-clp-multivoice-control-design
git worktree add -b fx/clp-multivoice-max-r10 /workspace/r10-source e2d7963d97cd2d5918e3b80b88156fbbfaa60f47
python3 -m py_compile tests/fx_clip_r10/run.py tests/fx_clip_r10/multivoice.py tests/fx_clip_r10/package.py
python3 tests/fx_clip_r10/run.py --output build/r10-host
python3 tests/bug001/run.py
python3 tests/kit_load/run.py
python3 tests/project_storage/run.py
python3 tests/euclidean/run.py
git -c core.whitespace=cr-at-eol diff --cached --check
git push -u origin fx/clp-multivoice-max-r10
```

Native host/regression suites each ran once locally, then again on the final source in CI. The README/workflow step-label completion after local checks does not change DSP or assertions. Local logs: /workspace/r10-evidence/host.log, bug001.log, kit_load.log, project_storage.log, euclidean.log.

CI's checked-in workflow repeats those suites and records exactly one:

```sh
cmake --preset release
cmake --build --preset release --parallel 2
python3 tests/fx_clip_r10/package.py
arm-none-eabi-objdump -d build/release/LXR_V2_STM32F765.elf
arm-none-eabi-readelf -l build/release/LXR_V2_STM32F765.elf
arm-none-eabi-size -A build/release/LXR_V2_STM32F765.elf
arm-none-eabi-gcc --version
git archive --format=zip --output=build/r10-evidence/SOURCE.zip HEAD
```

Actual artifact audit commands:

```sh
unzip -q /workspace/attachments/2d141d27-7178-4f00-954c-0bb0ec39d35b/lxr-r10-image.zip -d /workspace/r10-downloads/image
unzip -q /workspace/attachments/e49ca185-9b3f-44b5-bd9c-95d5128cd623/lxr-r10-source-build-evidence.zip -d /workspace/r10-downloads/evidence
python3 /workspace/r10-evidence/verify-download.py
readelf -sW /workspace/r10-downloads/evidence/LXR_V2_STM32F765.elf
readelf -sW /workspace/r9-downloads/evidence/LXR_V2_STM32F765.elf
```

The temporary verifier recomputes published wrapper/file/source hashes, archive integrity, all Git source blobs, header/checksum/reserved/vector/splash fields, application ranges and full ELF/image payload identity. Its successful result is /workspace/r10-evidence/download-verification.json.

**Failed attempts: none.** The first local host suite, all four local regressions, final CI attempt 1, Release build, resource fit, packaging and downloaded-byte audit passed. No test weakening, retry build, placement change or force push was needed. No unresolved implementation blocker was observed in these checks.

## Handoff and focused listening plan

No tested idle-persistence, invalid-output, bounds or resource-fit blocker was found. The candidate is packaged for a **separate later installation/listening decision**. The owner authorized hearing Maximum, not final policy acceptance or freezing this stage.

After separate approval, compare R10 solo against R9 first, then two voices at unequal levels: fade the lower voice, fade the dominant voice, fade both together; repeat with silent high/low/zero routed voices and muted/velocity-zero configurations. Try sparse/alternating steps, natural tails, returning notes, asymmetric stereo, six-voice kits and busy maximum-regeneration conditions. Judge whether the common-return behavior is useful, whether an inactive high fader holding gain open is disruptive, and whether maximum crossovers/routing/kit steps or return attacks are acceptable. Check device deadline/underrun/stack concerns under representative load; memory fit and native runtime do not settle them. Bass/parameter refinement remains deferred.

All **66 pre-existing remote heads** are preserved; only the new R10 source and report branches are added. No main/lab-setup write, merge, release, force push, repository setting change, flashing or hardware test occurred.

Report-only branch: **codex-reports/2026-10-31-r10-clp-multivoice-max-test-build**.
Exact report base: **5829398cea5d337200393757e39a37d4d8148256**.
The report head is the documentation-only commit containing this file; it includes no candidate source changes. Remote contents and the exact branch/file URL are verified after publication.
