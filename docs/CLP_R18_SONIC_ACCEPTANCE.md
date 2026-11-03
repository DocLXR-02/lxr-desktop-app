# FX-CLP — R18 musical acceptance and chapter closure

**Recorded:** 2026-11-03 (Europe/Vienna). **Evidence classification:** owner-reported LXR-02 hardware listening and independently verified pre-existing build evidence. This is **documentation only**; there was no new build, flash, source edit, release, merge, or instrument measurement in this closure task.

## Decision

**FX-CLP is accepted and musically closed at R18 (CLP v2.1)** by the owner. On the actual LXR-02, the owner explicitly affirmed all three prior acceptance questions:

1. The kick bass is satisfactorily preserved, including the previously problematic partial/high dry/wet range.
2. The fluid and expressive high-frequency feedback behavior sought from the R14 7200-Hz core is present.
3. The new output soft ceiling is subjectively acceptable; the owner reports no objectionable audible distortion or other issue and considers the sound complete.

This is positive **subjective sound and usability acceptance on one device**, not a numerical frequency-response, gain, CPU, latency, stack, thermals, hearing-safety, reliability, or long-soak certification. No hardware debugging/profiling equipment is currently available to the owner. **No further CLP sound changes are requested or authorized.**

## Pinned implementation and delivery

| Item | Exact identity |
| --- | --- |
| Repository | `DocLXR-02/lxr-desktop-app` |
| Accepted **R18 source branch** | [`fx/clp-v21-200hz-soft-r18`](https://github.com/DocLXR-02/lxr-desktop-app/tree/fx/clp-v21-200hz-soft-r18) |
| **R18 source commit** | **`41b364067bce0d8cde624c184933d9cb88e2725b`** |
| R14 sound/core baseline | `00ea59d38de6f006dfdbcd472c2cfe5040a440c3` |
| Image | `LXR_V2_R18.img`, **287,944 bytes**, SHA-256 **`c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1`** |
| Delivery ZIP | `LXR_V2_R18.zip`, **206,022 bytes**, SHA-256 **`2f3ab9501a858e030babb499a9116fcc21fa0149fb38aa41f46a81a078205588`**, exactly one IMG |
| CI evidence | [Successful run 41725066117](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41725066117), attempt 1, source `41b3640…` |
| Complete historical build report | [R18 implementation, validation, resources and caveats](https://github.com/DocLXR-02/lxr-desktop-app/blob/1eee9ed318b592cec54379bcc255927968b936a3/docs/codex-berichte/2026-11-03_r18-clp-v21-soft-build.md) |

The original R18 build report documented **pre-listening** limitations as of its build. Its statement that owner musical acceptance was pending remains historically accurate. This separate, later owner listening record supplies that acceptance without rewriting history. GitHub CI artifacts have **30-day retention**; source/test code and reports remain in their pinned branches, but the large binary evidence is **not permanently mirrored** to a separately approved store.

## Sound architecture accepted at R18

- One shared R14-based CLP nonlinear feedback core and its original ~7200-Hz HP1 coefficient; the filter's original misleading `7.2Hz` source comment was corrected, not its pole.
- R5 inactivity detector, bounded diode lookup, reset/hold/fade, R13/R14 shared maximum routed-fader feedback control and interpolation preserved.
- Original R7 negative-weight parallel bass contribution removed. New 200-Hz output-only, VOL-coherent complementary **difference filter**, one history per channel; bass is not injected into internal `lastOut` feedback.
- Dry/wet output reformulated to preserve low-frequency input energy. Fully dry is bit-identical under the validated fixture; fully wet with VOL=0 and cleared history is silent under its validated fixture.
- Wet-dependent rational soft ceiling for final CLP output, knee ~0.92 full scale at 100% wet. It is **not** a system-wide output limiter.
- Other effects, fader routing, kit/preset formats, and bootloader unaffected by R18 implementation.

## Historical contrasts — not the accepted firmware

| Stage | Role and observation |
| --- | --- |
| R6 | Alternative split input/feedback filters: more bass excitation, but undesirable suppression of scream; rejected |
| R7 | Added independent parallel bass; improved weight, but phase cancellation during dry/wet blending remained possible |
| R14 | Acceptable fluid feedback behavior and multi-voice MAX control; bass loss at partial dry/wet |
| R15 (110 Hz) | Full bass retained, changed feedback into lower-frequency/step-like behavior |
| R16 (1000 Hz) | More CLP control room, but still step-like |
| R17 (3000 Hz) | Still insufficiently fluid compared with 7200 Hz, while bass already reduced |
| **R18 (v2.1)** | **Owner accepted both bass and expressive feedback; no audible objections** |

Earlier firmware branches/images are reference material, not cumulative additions after R18. In particular, **R18 was built from pinned R14**, not by layering R15–R17's retuned HP1 into the final image. Historical R11 post-MIX control and R11 FILTER_OFF restrictions are not silently assumed to have been included in R18; separate earlier branches were deliberately not merged. Treat each version by its own pinned source ancestry and tests.

## Resource baseline and unmeasured gates

Matched **R14 → R18** ARM release comparison:

| Static resource | R14 | **R18** |
| --- | ---: | ---: |
| ITCM used | 15,208 B | **15,208 B** |
| ITCM nominal free, 16,384 B region | 1,176 B | **1,176 B** |
| DTCM used | 107,424 B | **107,424 B** |
| DTCM nominal free before actual stack high-water, 131,072 B region | 23,648 B | **23,648 B** |
| General SRAM static | 274,648 B | **274,648 B** |
| Complete image incl. header | 287,576 B | **287,944 B** |

The DTCM gap is **not a measured safe stack reserve**. Cortex-M7 cycle consumption, worst-case audio ISR/codec/DMA deadlines, runtime stack high-water, concurrent-voice peak/output mixing, extended reliability, thermal behavior and a second-unit result are **UNMEASURED**. The absence of an owner-reported audible defect does not replace those gates. The added rational soft limiter can use double-precision operations and division, which requires profiling before claiming CPU headroom.

Given the successful musical test and no debugging equipment, do not invent measurements or reopen CLP sound development merely to address missing instrumentation. Retain this as an explicit **measurement-only technical limitation**, to revisit if instrumentation becomes available or a new reproducible symptom appears.

## Project continuation and guardrails

**Chapter closure** means a frozen accepted **sound reference**, not a merge to `main`, a globally released binary, a validated runtime safety margin, or proof of every previously reported non-CLP bug fix.

1. Preserve R18 image/hash, exact source commit, build report, this hardware acceptance and the earlier R14 baseline.
2. Continue unrelated issues in **a separate new chat and next changelog task**, starting by reading [PROJECT_STATUS.md](PROJECT_STATUS.md), [EVIDENCE_INDEX.md](EVIDENCE_INDEX.md) and [WORKFLOW.md](WORKFLOW.md). No new issue, code change, or change ID is initiated by this closeout.
3. Preserve protected `main`, R4 and R11 and historical R14–R17 sources. If future fixes need to be integrated, plan them from exact commits, check compatibility and obtain approval; do not assume branches can be blindly merged or overwrite accepted audio.
4. No further sound-altering optimization without objective evidence of benefit, an audible comparison and explicit owner approval. No unattended Codex or Work follow-on.
