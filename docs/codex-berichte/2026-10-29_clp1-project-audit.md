# CLP-1 project audit and corrected hardware-reference comparison

Date: 2026-10-29 (Europe/Vienna)
Task: Inspect the supplied CLP-1.zip project, supersede earlier threshold transcriptions, and compare the saved cases with the existing bounded host DSP reference.
Firmware source: DocLXR-02/lxr-desktop-app, R4 commit c870f7249d7ee0d65423bc8b7bfdb422dcc74482.
Report-only branch: codex-reports/2026-10-29-clp1-project-audit
Report base: lab-setup commit 5829398cea5d337200393757e39a37d4d8148256.
Report head: the commit containing this report.

## 1. Authoritative device observations

The owner's latest message supersedes all conflicting values from earlier lists. Order is drv / col / ton / vol. d/w remains 40.

| Kit | Stored drv | Stored col | Stored ton | Stored vol | Latest reported behavior |
| --- | --- | --- | --- | --- | --- |
| 0 | 127 | 0 | 0 | 127 | No whistle, per earlier unchanged observation |
| 1 | 127 | 0 | 127 | 127 | No whistle, per earlier unchanged observation |
| 2 | 127 | 6 | 0 | 127 | Starts at col 6 |
| 3 | 127 | 3 | 127 | 127 | Starts at col 3 |
| 4 | 63 | 6 | 127 | 127 | Explicit exception: starts at 7; stops on descent at 5 |
| 5 | 63 | 21 | 0 | 63 | Starts at col 21 |
| 6 | 63 | 11 | 127 | 63 | Starts at col 11 |
| 7 | 63 | 12 | 63 | 63 | Starts at col 12 |
| 8 | 32 | 81 | 0 | 32 | Starts at col 81 |
| 9 | 32 | 44 | 63 | 32 | Starts at col 44 |

DEVICE: for ordinary threshold cases, the prior clarification says the tone ends one step below. Kit 4 is now explicitly excluded from that generalization: col 6 is a history-dependent intermediate setting.

The ZIP stores Kit 4 at 6, exactly as the owner listed; the separate explicit onset/stop statement takes precedence when interpreting that record. Saving at 6 is consistent with a sustaining intermediate setting, but a kit file does not itself record whether the effect was oscillating at save time.

## 2. Archive and kit audit

Read the attachment locally, without changing it or extracting arbitrary paths. Python zipfile.testzip() returned None.
The archive has 394 entries and 799697 uncompressed bytes. It includes PROJ00, PROJ01, an image, index and other root entries. Scope of kit inspection is PROJ00 only.

- PROJ00/PRJ.NFO contains the padded name CLP-1.
- PROJ.IDX is byte 0, selecting PROJ00.
- PROJ00 contains 194 regular files; none have zero length.
- Kits 00-KIT0_.SND through 09-KIT9_.SND are each 255 bytes.
- Source confirms an 8-byte kit name followed by 247 parameter bytes.
- All ten stored FX tuples match the latest list, including stored Kit 4 col=6.
- Across the ten kits, all non-name payload differences are confined to the four FX controls at file offsets 246 through 249. Voice sound settings are otherwise byte-identical.

Parameter indices were derived from enum ParamEnums in Core/Inc/Parameters.h. Source locations:
- FX enum: approximately lines 318-326.
- END_OF_SOUND_PARAMETERS: line 339; enum value 247.
- presetManager.c:883-884 writes 8 name bytes then drumset data.
- presetManager.c:1039 reads the sound parameter payload.

Exact mapping, zero-based:
| Field | Parameter index | File offset |
| --- | --- | --- |
| FX type | 234 | 242 |
| FX out | 235 | 243 |
| d/w | 236 | 244 |
| Drive algorithm | 237 | 245 |
| drv | 238 | 246 |
| col | 239 | 247 |
| ton | 240 | 248 |
| vol | 241 | 249 |

Shared saved settings:
- FX type 1 (distortion); algorithm 2 (internal RAT / visible CLP).
- d/w 40.
- FX routing 0 (MIXER_ROUTING_DAC1_STEREO).
- Voice 1 output 6 (MIXER_ROUTING_FX_BUS); voices 2-6 output 0.
- Voice volumes: 127,0,0,0,0,0.
- All six saved LFO targets are 0 and amounts are 0.
- All six saved velocity-modulation amounts are 0.

The routing names follow Core/Inc/Soundengine/DSPAudio/mixer.h:32-38. They are source enum names, not a new physical jack measurement. Physical fader positions and live DSP state are not established by these kit bytes. Pattern automation was not fully decoded in this task; do not extend the kit LFO findings into a blanket claim that every runtime modulation source is absent.

The image included in the attachment is 288676 bytes, SHA-256:
1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877
This matches the established R4 image hash. It identifies the supplied file, not proof of what was flashed at any particular test moment.

## 3. Host comparison

Reused the previous complete Distortion_process host build with simulated CMSIS saturation/packing intrinsics, 64-sample blocks, and nominal 44140 Hz. The sole DSP adaptation remains the positive diode-table endpoint guard. It defines the formerly invalid table read and is not a firmware patch.

Inputs: exact silence or the previously documented synthetic decaying sine kick followed by silence. This is not an original-device voice reconstruction.

The three previously mismatching tuples were corrected to the new authoritative data:
- Kit 6: col 11, ton 127.
- Kit 7: col 12, ton 63.
- Kit 9: col 44, ton 63.
These now match the existing simulated boundaries. There is no remaining evidence of the earlier three discrepancies; those arose from superseded reference values.

probe_clp1.c tests all ten cases, mono/stereo and fresh-silent/kick-seeded states. It uses reported onset 7 for Kit 4 while retaining stored col=6 separately in the audit JSON. Per-stage duration is approximately 3 seconds; final approximately 1 second RMS is recorded. For control kits with col=0, the lower setting is clamped to 0.

HOST: both col=0 controls are silent after input stops. The seven ordinary active thresholds (Kits 2,3,5,6,7,8,9) match in kick-seeded tests: active at the stated value and quantized-silent one col step below. Kit 4 needs a history-aware sequence instead of an independent initialization at each value.

### Kit 4: ascending and descending sequence

probe_kit4.c preserves DSP state between stages, at drv=63, ton=127, vol=127. Each stage lasts approximately 10 seconds. It tests both mono and stereo, with no initial input or an initial synthetic kick at the first stage.

| col in sequence | Final output RMS at d/w=40, representative mono kick-seeded run | State |
| --- | --- | --- |
| 5 | 0 | Silent |
| 6 | 0 | Silent |
| 7 | 0.166638637 | Sustained tone |
| 6 | 0.144139671 | Sustained tone continues |
| 5 | 0 | Silent |

The same sequence classification occurs in all four channel/seed combinations.

HOST: the reported 7-on / 5-off behavior is reproduced with a continuous state history. An independently initialized or separately kick-seeded run can instead oscillate at col=6. These observations show why reporting a single threshold independently of history was insufficient for this case.

This is a reproduction under a particular staged protocol. It does not establish that every possible live history has the same boundaries, or identify a unique analytical mechanism. In particular, no separate test has established whether finite-precision lookup behavior, a nonlinear basin of attraction, residual excitation, or a combination explains the detailed history dependence.

## 4. Commands and verification

Executed with cc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0:
```sh
cc -std=c11 -O2 -fno-strict-aliasing -fsanitize=undefined,bounds -fno-sanitize-recover=all tmp/clp-host/probe_kit4.c -lm -o /tmp/lxr_clp_kit4
/tmp/lxr_clp_kit4 > tmp/clp-host/kit4_sweep.csv
cc -std=c11 -O2 -fno-strict-aliasing -fsanitize=undefined,bounds -fno-sanitize-recover=all tmp/clp-host/probe_clp1.c -lm -o /tmp/lxr_clp1
/tmp/lxr_clp1 > tmp/clp-host/clp1_results.csv
```
All compile/run commands exited 0. No sanitizer diagnostic occurred in the bounded host reference. Local Python archive, payload-comparison, enum-parsing and kit-value assertions completed successfully.

Outputs: clp1_kit_audit.json, clp1_results.csv, kit4_sweep.csv, and reproducible diagnostic host sources in the accompanying analysis bundle.

Not executed: ARM build, firmware flash, new physical-device tests, full-engine render from the supplied kit/pattern, physical audio capture, prototype suppression design, CPU/resource measurement.

## 5. Interpretation and next step

The corrected kit data plus the state-preserving Kit 4 sweep resolve the initial reference disagreements. The bounded DSP model is now a useful reference for the supplied silence/onset cases. Matching boundaries does not prove bit-identical or perceptually identical sound.

A prospective solution must be tested against:
1. The seven ordinary onset/extinction pairs.
2. Kit 4 starting from silence and Kit 4 already oscillating at col=6.
3. Active kick sound, quiet input/tails, input disappearance and input return.
4. Both stereo channels, without coupling unrelated routed voices incorrectly.

The next implementation candidate remains a signal-dependent reduction of feedback during input loss, first as an isolated offline experiment. It must be compared with the unchanged active-input reference. Its release time, threshold, and any alternative output muting are not yet selected. The input-filter coefficient must remain separate during that comparison.

No source-code change to the firmware is authorized or made by this audit. No new change-list ID or hardware-fix claim is assigned.

## 6. Publication and handoff

Only this English report is committed to the dedicated report branch based on lab-setup. main and R4 source/image remain unchanged. The owner's uploaded archive remains unchanged and is not republished in the public report branch. The analysis ZIP contains derived audit data and diagnostic test sources with upstream licensing/provenance.

Historical reports remain unchanged; use this report and the owner's latest message for current thresholds.
