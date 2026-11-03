# R18 — CLP v2.1 200-Hz VOL-coherent output crossover with wet-dependent soft ceiling

Date: 2026-11-03 (Europe/Vienna). One owner-authorized **experimental audition firmware build** in normal ChatGPT CHAT using GitHub connector and GitHub Actions. No Codex or Work task was launched, no merge, no GitHub release, no device flash and no real-device hardware testing.

## Owner authorization and design basis

The owner tested R14, R15, R16 and R17 on LXR-02 hardware. R14's original approximately 7200-Hz CLP feedback core and six-route MAX gain produce the most fluid parameter response but lose kick bass during partial dry/wet mixing. Reducing HP1 to 110, 1000 and 3000 Hz improved bass progressively but adversely changed feedback tonality or continuity. The independent R6 separate-input/return filter approach had also allowed low-frequency input to dominate a shared nonlinear core; R7 parallel bass and original dry/wet caused possible phase cancellation. R18 is an expressly authorized **single test** of the previously completed CLP v2.1 offline architecture, not an accepted production replacement.

Prior offline evidence reused: `CLP_V2_1_Architecture_Study_2026-11-03.zip` and `CLP_V2_1_Extended_Validation_2026-11-03.zip` (private conversation attachments, not GitHub-archived), 81/44 compiled C-vs.-Python reference cases, additional rapid/smooth automation corrections and headroom surveys. These are **inherited HOST evidence**, not measured MCU timing or physical audio. The private KnowledgeBase v1.4.0 is an older 2026-11-01 snapshot, not the canonical source for R14–R18. The current documentation branch `docs/lab-project-state` was read, including `PROJECT_STATUS.md`, `WORKFLOW.md`, `EVIDENCE_INDEX.md` and `RECORDKEEPING_STANDARD.md`; `AGENTS.md` was checked at the exact R14 source. No Codex quota was used; the current live agent budget is unknown.

## Exact source, run and artifacts

| Identity | Value |
| --- | --- |
| Repository | `DocLXR-02/lxr-desktop-app` |
| **R14 source base** | **`00ea59d38de6f006dfdbcd472c2cfe5040a440c3`** |
| R18 firmware branch | `fx/clp-v21-200hz-soft-r18` |
| **R18 tested firmware commit** | **`41b364067bce0d8cde624c184933d9cb88e2725b`** |
| Exact diff | [R14 → R18](https://github.com/DocLXR-02/lxr-desktop-app/compare/00ea59d38de6f006dfdbcd472c2cfe5040a440c3...41b364067bce0d8cde624c184933d9cb88e2725b) |
| Build run | [GitHub Actions 41725066117](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41725066117), attempt 1, job `126227576135`, **success** |
| ARM compiler | Arm GNU Toolchain 14.3.Rel1, `arm-none-eabi-gcc` 14.3.1 (2025-06-23), Release `-O3` |
| Report-only branch | `reports/2026-11-03-r18-clp-v21-soft-build` (not a firmware source branch) |

## Production change: three files only

1. `Core/Src/Soundengine/DSPAudio/FX/Distortion.c`: **unchanged nonlinear/feedback calculations** from R14, with the same 7200-Hz HP1 coefficient `0.64116641920131193922` in mono and stereo. Correct the explanatory HP1 comment, which historically mislabels this coefficient as 7.2 Hz, **without changing HP1's value**. Preserve `CLIP_INPUT_TRIM=0.22`, gain/slew/diode/TON/VOL, R5 input-loss feedback gate, R13 feedback interpolation and R14 MAX fader selection. Remove R7's 150-Hz parallel bass lowpass, negative `-0.25` bass injection, and its output summation.
2. `Core/Inc/Soundengine/DSPAudio/FX/Distortion.h`: **replace** R7's two `double clipBassLp_state[2]` histories with two `double clipV2DifferenceLow[2]` histories. Per-channel output state resets on existing audio-owned CLP begin/reset; it updates even while D/W=0. Runtime filter-state allocation remains **16 bytes**, matching R14.
3. `Core/Src/main.c`: boot identification `LXR LAB R18`.

New post-core, VOL-coherent differential mixer, applied to the *true int16-quantized CLP core return*, before the old intermediate dry/wet difference limiter (which is no longer used in CLP):

```text
alpha = 1-exp(-2*pi*200/44140) = 0.028067915544701783
D = dry_input / 32768; W = int16_quantized_CLP_core_return / 32768
q = W - CLP_VOL * D
L[n] = L[n-1] + alpha*(q - L[n-1])
raw = (1-DW)*D + DW*(W-L[n])
T = 1-0.08*DW
if DW > 0 and abs(raw) > T:
  excess = abs(raw)-T
  raw = sign(raw)*(T+(1-T)*excess/(excess+1-T))
result = saturating_int16_conversion(raw)
```

For steady constant VOL, this preserves `VOL * LP200(dry)` at fully wet plus the high-passed true CLP return; varying VOL follows the exact recurrence, which has short filter-memory transients. There is only **one CLP core** and a single additional state per channel. The outgoing mixed/limited result is **never** fed back into `lastOut`. The knee reaches 0.92 FS at D/W=1 and approaches 1 at D/W=0, retaining exact 0-Wet Dry identity. This rational soft ceiling introduces potentially audible additional nonlinear harmonics and one floating-point division when active; it is **not a measured mastering/PA safety limiter**.

Unmodified: mixer.c and the original shared six-voice MAX control, all native voice faders and FX routing, FX reset ownership, kit/preset storage, all non-CLP effect types, source MIDI/UI handling. The separate future candidate to scale audible solo feedback output at low faders is **not** included. Dense simultaneous six-voice overload remains outside scope.

## Executed validation (not merely planned)

- Local preflight on *exact extracted R14 source* after identifying and rejecting a wrongly assumed R14 source archive whose boot string was still R13. Pinned GitHub/R14 CI `SOURCE.zip` was used instead. Actual R18 production ELF/source blob provenance later matched GitHub exactly.
- Local sanitizer-enabled R18 HOST unit/state tests: **PASS** (R5 linked input gate 441-frame hold/decay/reopen, diode endpoints, deferred reset, mono/stereo, Tube/Fold return).
- Six direct waveform checks against previously validated R18 v2.1 C HOST prototype: **sample-identical waveform and core-return hashes**. The old HOST prototype had stale 3000/110-Hz descriptive comments; production R18 comments correctly state the *actual* 7200-Hz coefficient.
- CI **75,000 independent long-double mixer calculations**: **PASS** within 1 int16 LSB; exact D/W=0 dry identity; bounded output for representative D/W>0; reset and cleared full-Wet VOL=0 behavior. This verifies arithmetic under examined bounded controls, not a full live audio automation or CPU deadline.
- CI **135 source-derived DSP render cases**, comparing the **actual R18 CLP internal feedback return snapshots** against exact pinned R14 with only the old R7 bass contribution muted in the reference: **PASS**. Tested five synthetic histories, three DRV/COL rows, mono/identical/asymmetric stereo, D/W 0/50/100. The final audible output deliberately differs, and these blockwise hashes are not every-sample internal instrumentation.
- CI inherited unit/state and actual six-route FX-bus reset/dispatch: **PASS**; existing bug001, kit_load, project_storage and euclidean regressions: **PASS**.
- CI R14 source-extracted MAX test: **16,384 routing/fader combinations and 4,096 ramp endpoints PASS**, using byte-identical R14 `mixer.c`.
- CI source guard: the **three production files' exact Git blob hashes** compared with the locally tested bytes; all protected R14 mixer/FX-bus/voice/fader files compared byte-identically against pinned R14 base SHA: **PASS**.
- One ARM Release build, link, region fit, image/ZIP checks and artifact uploads: **PASS**. Independently downloaded all exact build artifacts and rechecked source SHA, blob hashes, IMG checksum/header/vectors, full ELF load-segment byte correspondence, the ZIP containing only the expected image and SHA-256: **PASS**.

**Historical test timing distinction:** The inherited `tests/fx_clip_r18/unit.c` derives from R15's test variant, allowing 250ms of DSP output settling after feedback gate closure. R18 itself preserves the fast R14 HP1 core but adds the 200-Hz output filter. This is a conservative regression fixture; **250ms is not a measured audible physical decay**. The original short-tail assumption was not declared unchanged.

**Explicitly unmeasured:** Real-device hardware sound and phase, kickbass on owner kits, effectiveness of wet 0.92 ceiling at downstream multi-route sums, CPU cycle worst-case / DMA ISR deadlines / underruns, stack highwater, full-length live playing, transient click audibility from fast D/W/VOL changes, firmware return after switching effects, new soft-limiter harmonic coloring, or whether the 200-Hz crossover sounds musically superior. Offline fast-control stress had previously exposed short filter-memory transients; R18's live UI smoothing was not directly measured. Hardware listening is a **required next user-controlled stage**, not a completed acceptance.

## Matched linker/resource ledger (R14 → R18)

| Measure | R14 | R18 | Difference |
| --- | ---: | ---: | ---: |
| ITCM executable used | 15,208 B | **15,208 B** | 0; **1,176 B free** |
| DTCM static used (project region ledger) | 107,424 B | **107,424 B** | 0 |
| General SRAM static | 274,648 B | **274,648 B** | 0 |
| `.text` | 151,416 B | **151,784 B** | +368 B |
| Linker FLASH | 235,356 B | **235,724 B** | +368 B |
| Complete application flash payload, excluding IMG header | 287,560 B | **287,928 B** | +368 B |
| Firmware IMG with header | 287,576 B | **287,944 B** | +368 B |

Static 16-byte output-history replacement balances the old parallel bass history, so DTCM is unchanged. **Static file-size equality elsewhere is not evidence of CPU/WCET equality.** The rational soft ceiling can increase CPU use because of double arithmetic and division, especially on a Cortex-M7 single-precision FPU. There is no measured real-device runtime margin.

## Independently verified delivery identities

| Item | Bytes | SHA-256 |
| --- | ---: | --- |
| `LXR_V2_R18.img` | **287,944** | `c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1` |
| `LXR_V2_R18.zip` | **206,022** | `2f3ab9501a858e030babb499a9116fcc21fa0149fb38aa41f46a81a078205588` |
| Build `LXR_V2_STM32F765.elf` | 391,712 | `c4b3e73e1c1aec13800536b4ef020597848259c421a0b3bf9855e0ddabad8b87` |
| CI evidence `SOURCE.zip` | 1,720,884 | `c6dc64ac33e5b033ac7b322c5fa6f9c0a43e9afa046a7fa2f6ba2fe703d57d15` |

- [R18 complete image/ZIP artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41725066117/artifacts/13274151339).
- [R18 source, ELF/MAP, exact compiler evidence and all test logs](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41725066117/artifacts/13274031362).

**Retention:** CI artifact ZIPs have a 30-day configured retention and will expire; they are **NOT PERMANENTLY ARCHIVED** in an owner-approved persistent binary location. The source, scripts, run metadata and this report remain on distinct source/report branches; user-delivered downloadable ZIP and IMG files were verified locally but are not a promised durable binary store.

After CI, the protected source references were independently checked unchanged: `main` `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`; R4 `c870f7249d7ee0d65423bc8b7bfdb422dcc74482`; R11 `64263ed25f2e9c7f5d880b493d943bfba29271a8`; R14 `00ea59d38de6f006dfdbcd472c2cfe5040a440c3`; R15 `03bcb4f4e21d368d7a5eda2691ad619bb595d994`; R16 `37dbf75625f10e5bc2de6529e0f601e74063cb79`; R17 `1be1951c3a5bb271d7eadd17524343a6c37de1a0`.

## Audition protocol and stop criteria

Owner-controlled only; no device action by assistant. Preserve R14 and R15 known working images and recovery instructions. At first listen, use **low monitor/output level**. Compare the same voice-1 kick and kit at D/W 0/25/50/75/100, CLP VOL 0/63/127 and several COL/DRV/TON positions, then one, three and six FX-routed voices with physical fader transitions. Listen for kick weight, natural high-frequency screaming character, click/transient at empty steps and parameter jumps, strong self-oscillation, unexpected dropouts/underruns and output saturation. **Stop** if audio becomes uncontrollably loud, repeats, freezes or drops out; revert to validated R14 rather than continuing stress. Do not infer that a successful CI build proves on-device safety. No follow-up firmware job is approved by this report.
