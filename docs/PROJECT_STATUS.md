# LXR Firmware Lab — current project state

Snapshot: 2026-11-05 (Europe/Vienna). Synchronization ID: **LXR-SYNC-2026-11-05-R18-TECHNICAL-CONSOLIDATION**. This record supersedes the 2026-11-04 status snapshot; historical reports remain unchanged.

This is the current public entry point for continuing the lab. Read this file, [hardware observations](HARDWARE_VALIDATION.md), the [resource ledger](BUILD_RESOURCE_LEDGER.md), [evidence index](EVIDENCE_INDEX.md), [workflow](WORKFLOW.md) and [recordkeeping standard](RECORDKEEPING_STANDARD.md) before proposing another task. Reports are historical evidence at their pinned commits. A report branch is not a firmware source branch.

This documentation branch descends from historical lab-setup `5829398cea5d337200393757e39a37d4d8148256`. **Never build the documentation branch to produce firmware.** Use the exact R18 source SHA below. R18 CLP **sound** is owner-accepted; this is not a source merge, public release, hardware timing certification, or measured stack reserve.

## Chat-first continuation and retention

Normal CHAT is the default place for source-level investigation, GitHub evidence reading, design discussion and verification. Codex is reserved for justified, narrowly authorized build/test/implementation work after the [mandatory efficiency gate](WORKFLOW.md#chat-first-execution-and-mandatory-codex-efficiency-gate). Work is used only for autonomous app/browser/computer workflows with demonstrated incremental value. The private KnowledgeBase and project instructions contain the owner's plan/model/budget preferences; do not place them or the internal language rule into the public README.

The [recordkeeping standard](RECORDKEEPING_STANDARD.md) requires a reasoned task report **and** durable reproducible critical evidence. Prior R11 audit Markdown reports and hashes are published, but their listed local-only analysis scripts and JSON/log outputs were **not** all archived to GitHub. This gap remains open and should not trigger an indiscriminate rerun. CI logs/artifacts expire; preserve decisive evidence, commands, inputs and hashes with provenance. The next Codex task must explicitly link this documentation branch because the existing R11 firmware source branch does not inherit new documentation-branch instructions.

Documentation-only synchronization records the owner's **new R18 CLP musical acceptance**. No production source change, build, flash, hardware profiling or release was performed by this synchronization.

## DATA encoder — owner support handoff (2026-11-04)

**BUG 006:** Intermittent missed DATA encoder push confirmations across menus, kit operations, renaming, and firmware-update confirmation (owner's rough estimate ~5%). Increased pressure can trigger an otherwise missed confirmation; off-center downward pressing clicks audibly/tactilely without confirmation. These observations prioritize a mechanical/electrical contact hypothesis, without proving hardware failure or excluding firmware debounce. **Seller return authorization received; owner intends technical-support examination without modifying hardware.** Return shipment, support findings and repair are pending/unverified. No further firmware diagnostic/build task is authorized. **BUG 007:** Skipped rotary states remain a separate open issue. Full observations and evidence boundaries: [DATA input status](INPUT_CONTROL_STATUS.md). R18 source and accepted CLP sound remain unchanged.

## Current firmware identity — owner-accepted R18

**FX-CLP SOUND CLOSED — R18 CLP v2.1 (2026-11-03).** The owner confirmed all three requested listening outcomes on the LXR-02: kick bass fully satisfactory across problematic dry/wet positions, fluid expressive high-frequency feedback, and no objectionable soft-ceiling coloration. The owner requests no more CLP sound changes. See [R18 sonic acceptance/closure](CLP_R18_SONIC_ACCEPTANCE.md) and the separately preserved [R18 technical build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/1eee9ed318b592cec54379bcc255927968b936a3/docs/codex-berichte/2026-11-03_r18-clp-v21-soft-build.md).

| Role | Exact identity |
|---|---|
| **Accepted R18 firmware source** | [`fx/clp-v21-200hz-soft-r18`](https://github.com/DocLXR-02/lxr-desktop-app/tree/fx/clp-v21-200hz-soft-r18), **`41b364067bce0d8cde624c184933d9cb88e2725b`** |
| **R18 application IMG** | `LXR_V2_R18.img`, **287,944 B**, SHA-256 `c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1` |
| **R18 delivery ZIP** | `LXR_V2_R18.zip`, **206,022 B**, SHA-256 `2f3ab9501a858e030babb499a9116fcc21fa0149fb38aa41f46a81a078205588`; one IMG |
| **R18 ARM CI** | [Success 41725066117](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41725066117), attempt 1, job `126227576135`, exact R18 source |
| **Source baseline** | R14 `00ea59d38de6f006dfdbcd472c2cfe5040a440c3`; R18 was built from this source, not accumulated from R15–R17 |
| **Other experimental refs** | R15 `03bcb4f4e21d368d7a5eda2691ad619bb595d994`; R16 `37dbf75625f10e5bc2de6529e0f601e74063cb79`; R17 `1be1951c3a5bb271d7eadd17524343a6c37de1a0` |
| **Current static ITCM** | 15,208 / 16,384 B; **1,176 B nominal free** |
| **Current static DTCM** | 107,424 / 131,072 B; **23,648 B nominal free before real stack high-water** |
| **Runtime evidence** | **CPU deadlines, DMA/IRQ underruns, stack high-water, thermal/soak reliability UNMEASURED**; owner has no debugger/profiler available |

R18 preserves R14's single shared **7200-Hz HP1 feedback core**, R5 idle gate and R14 MAX fader feedback. It **replaces** R7's negative parallel bass with a **200-Hz VOL-coherent output difference filter** and wet-dependent rational soft ceiling. Full R14 sound/feedback was subjectively restored while maintaining the desirable bass. Only the musical sound/UX chapter is accepted; system-level reliability remains separately unmeasured. No protected branch was merged, overwritten, or released.

## R18 technical consolidation — November 4–5, 2026

**Scope and evidence class:** A read-only R18-versus-R11 source audit, one test-only R18 reproduction suite, and two **independent** correction candidates were created after the November 3 CLP sonic closure. This is source/host/CI evidence, **not** owner hardware testing, a combined firmware image, a merged release, or authorization to install a candidate. The canonical accepted CLP R18 reference is still \`41b364067bce0d8cde624c184933d9cb88e2725b\`; no branch below changes it. The documentation sync makes no production-source change.

| Area | Exact candidate / evidence | Verified result and boundary |
|---|---|---|
| **Read-only R18 consolidation audit** | Owner-supplied \`LXR_R18_Technical_Consolidation_ReadOnly_Audit_2026-11-04.md\` (local conversation ZIP; **not published on GitHub**) | Compared R11 arithmetic fixes and inherited R18 risks; source-only, no new execution/build. R11 FM/PWM/wavetable/interpolation/transient fixes and its limited SoloMix/snare treatment were **not** silently inherited by R18. |
| **R18 failure reproduction** | Test branch \`test/r18-delay-usb-compressor-repro-2026-11-04\` at \`1da25aafeaa8de8c53313a402dfe58324ed2d74f\`; [host CI 41925616784](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41925616784) | **8/8** classified fixture outcomes, including expected original Delay mono-right alignment failure, USB TX BUSY loss and wrap over-read, USB RX full-as-empty, and Compressor mono over-range float-to-int16 conversion. Production code unchanged. Evidence ZIP SHA-256 \`ef21bd3c3c2a738e8027fa985608c545070e0498d2c714f97690671e6ecafee4\`; report/evidence held in owner-supplied ZIP, not a canonical GitHub report. |
| **Delay mono-safe access** | Independent \`fix/r18-delay-mono-safe-access\` at **\`08bc165500e21153eeb28ddbf2fe2d84626a82ca\`**; production fix \`c0ade6400479ced32260a799a4974eef4ae48b93\` | Only production file \`Core/Src/Soundengine/DSPAudio/FX/Delay.c\` changed: aligned 16-bit access for normal and ping-pong mono output. **Host PASS** [41965755184](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41965755184); **ARM Release validation PASS** [41972428668](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41972428668) (source-extracted tests, 144 output comparisons, CLP regressions and IMG/ELF checks). [Host report](https://github.com/DocLXR-02/lxr-desktop-app/blob/2c8d7138951e0c7c46b88de602ccc385d8e8ec6b/docs/codex-berichte/2026-11-05_r18-delay-mono-safe-access.md); [per-build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/bdbe8857e16eda7cecf0bfdeee048cede6ff6297/docs/codex-berichte/2026-11-05_r18-delay-mono-arm-validation.md). No hardware flash/listening. |
| **USB-MIDI FIFO/CDC ownership** | Independent \`fix/r18-usb-midi-ring-ownership\` at **\`b800669caf209067c47447fb6376abfbe7eaa7b3\`**; production fix \`220a9d82b9e819b38e1bdb7232751bb1439f1fa3\` | Fixes bounded 4-byte TX wrap submission, BUSY/in-flight ownership, RX capacity and full-as-empty handling; drops newest event under sustained overload and counts drops. **15/15 classified host cases PASS**, including expected original failures and R18 CLP regressions; [CI 42003303720](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42003303720); [report](https://github.com/DocLXR-02/lxr-desktop-app/blob/e776e0d3b7c244c3cc0493a89ec3c37cd93e1e28/docs/codex-berichte/2026-11-05_r18-usb-midi-transport-ownership-host.md). **ARM build, MCU USB traffic, timing and device test NOT RUN.** |
| **Compressor mono** | Original R18 \`FX/Compressor.c\`; reproduction suite above | For the selected mono input 32,767, makeup 3, ratio 1, unsaturated conversion attempts 98,301 before int16; **known numerical error reproduced**. **No production correction, candidate, ARM build or sound-acceptance test yet.** |

### Delay ARM resource delta and non-release identity

Matched against the accepted R18 using the same Arm GNU 14.3.Rel1/GCC 14.3.1 Release toolchain, the **Delay-only** candidate changed ordinary-flash \`.text\` by **+16 B** (151,784 to 151,800 B). ITCM **15,208 B**, DTCM section sum **107,420 B**, and general-SRAM section sum **274,644 B** were unchanged. The accepted status table above uses a historical region aggregate that includes four more bytes in the RAM aggregates; **both calculation conventions agree on zero added static RAM**. ITCM nominal free remains **1,176 B**. Candidate validation-only \`IMG\`: **287,960 B**, SHA-256 **\`3ab7f99a893df91474f862ac4f6158c5d11bed896499937a21c50f563cf39abc\`**; no firmware delivery package or installation authorization was issued. A parallel legacy host workflow [41972428701](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41972428701) failed **preflight allowlist only**, before tests; the authorized ARM workflow passed. A source-code-size delta is **not** a CPU/WCET measurement.

### USB-MIDI behavior and evidence limits

TX holds accepted asynchronous transfers until CDC completion; BUSY keeps events queued. The existing 128-byte TX ring holds up to **31 complete four-byte events** by leaving one slot unused; the RX ring holds **63 messages** by the same rule. New events are dropped deterministically at capacity and counted, so lossless MIDI, guaranteed Note-Off delivery under indefinite overload, and exactly-once transmission around disconnect are **not established**. Host mocks and compiler sanitizers do not certify target ISR/DMA/cache concurrency. The [USB-MIDI report](https://github.com/DocLXR-02/lxr-desktop-app/blob/e776e0d3b7c244c3cc0493a89ec3c37cd93e1e28/docs/codex-berichte/2026-11-05_r18-usb-midi-transport-ownership-host.md) explicitly requests a **separately authorized ARM build** next.

### Integration boundary and next decision

1. Preserve the original accepted **R18 sonic firmware**, all protected references and their hashes; neither candidate is automatically a new accepted version.
2. **USB-MIDI:** Owner approval is required for an ARM validation build from exact candidate SHA \`b800669caf209067c47447fb6376abfbe7eaa7b3\`, plus compiler/ELF/IMG/resource checks and a separate English build report.
3. **Compressor:** A source correction requires distinct approval. Select an explicit mono saturation/headroom policy, reproduce over-range and normal-range behavior, and separately evaluate any possible sound differences.
4. **Integration:** Only after authorization, combine the independently verified Delay/USB/Compressor changes on a new branch from pinned R18, rerun focused regressions and matched ARM build/resource checks. Avoid redundant device flashes; **hardware testing/installation requires additional explicit approval** and backup/recovery planning.
5. **Unmeasured:** Actual Cortex-M7 CPU/WCET, IRQ/DMA timing margin, stack high-water, long-duration/audio stability and physical MIDI/Compressor/Delay performance remain **UNMEASURED**. GitHub Actions artifact ZIPs have **30-day retention**; uploaded local ZIP copies are owner-provided evidence, not permanently published GitHub binaries.

This is a documentation synchronization only. No post-R18 completed public \`LXR-014\` change or R19 release is claimed. The two candidates are **separate and unmerged**; the protected CLP sound is unchanged.

## Historical R11 firmware identity (snapshot 2026-11-01)

| Role | Exact identity |
|---|---|
| Current owner-tested experimental image | LXR_V2_R11.img; 291340 bytes |
| Image SHA-256 | f1779db7861d4c9a54aded8121eb5659a445256ee64453635fcfa4fb1593d2b0 |
| Owner delivery ZIP | LXR_V2_R11.zip; 207732 bytes; contains only LXR_V2_R11.img |
| ZIP SHA-256 | b31c93b5742732e1c280a4789b5ebde75e4354e7acf44cead54ec23a3fce8a30 |
| Firmware source branch / commit | fx/clp-solo-post-mix-r11 / 64263ed25f2e9c7f5d880b493d943bfba29271a8 |
| Source tree | ad5d81b94ede2af0ff776254e539c50be24c76e1 |
| Arithmetic prerequisite | d949b3e132fd8afe5f076e6465aae1ffe507c938 |
| Corrected R10 comparator | 8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7 |
| Existing successful CI | [41214348060](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/41214348060), exact R11 source |
| Final R11 handoff | [Recovered publication](https://github.com/DocLXR-02/lxr-desktop-app/blob/84bc8a8c81fbc3d348c509ee5a2c7d5329d4cf8c/docs/codex-berichte/2026-10-31_r11-arithmetic-and-filter-off-post-mix.md) |
| Current offline audit | [Report](https://github.com/DocLXR-02/lxr-desktop-app/blob/fa3c5b711733d022301669f0bdacd650cde5d6d9/docs/codex-berichte/2026-11-01_r11-resource-reliability-audit.md), [evidence](https://github.com/DocLXR-02/lxr-desktop-app/blob/fa3c5b711733d022301669f0bdacd650cde5d6d9/docs/codex-berichte/2026-11-01_r11-resource-audit-evidence.md), [measurement design](https://github.com/DocLXR-02/lxr-desktop-app/blob/fa3c5b711733d022301669f0bdacd650cde5d6d9/docs/codex-berichte/2026-11-01_r11-resource-measurement-plan.md), [input reconciliation](https://github.com/DocLXR-02/lxr-desktop-app/blob/3ca14c236a2301b3883b6ebd377f9dee68385d4e/docs/codex-berichte/2026-11-01_r11-resource-audit-knowledgebase-addendum.md) |

The final R11 report gap is now closed. Earlier R11 blocker/design reports remain valid accounts of their own earlier stage; they are not the final-build report. The latest audit changed no production source. Its proposed diagnostics are not implemented or delivered.

## Historical R5–R11 accepted behavior and actual scope

The following bullets reproduce the earlier R11-era findings and restrictions. They must not be mistaken for a current R18 test matrix or merged R18 source behavior.


- The owner confirmed that the original unwanted idle whistle disappeared with the R5 policy, both with a voice fader at zero and during missing steps. This intentionally changes the manual's original feedback behavior; it is not evidence of a hardware-protection feature being removed.
- R6's bass-through-feedback experiment was not selected: bass strongly suppressed the desired scream. R7's parallel bass approach was selected and positively evaluated. R6 is not an extra stage in the selected lineage.
- R8 added audible core attenuation with the voice fader. R9 moved its knee to 50% physical travel: settled additional core gain is min(1, 2u), where u is normalized physical travel. This preserves internal feedback rather than scaling its history. It is an additional core gain, not a claim that the whole nonlinear sound has linear loudness.
- R10 uses the maximum fader among configured FX-routed synthesis paths. A routed but silent/high-fader voice can hold the shared core open. This owner-authorized compromise remains; no activity detector or independently separable wet stems were added.
- R11 moves MIX/VOL outside the nonlinear effect for **exactly one configured CLP route**. Positive MIX changes the post-return level while the eligible source remains full-level relative to MIX. Physical fader, envelopes and velocity still act. The owner reports that this works audibly as intended.
- Zero or multiple configured routes fall back to the arithmetic-prerequisite/R10 shared-bus behavior. Silent, muted and zero-velocity routes still count as configured routes. R11 is **not** independent multi-voice post-MIX balancing.
- Seven musical tracks map to six synthesis/mixer paths, including shared open/closed hi-hat handling. Software coverage of six paths does not establish seven independent insert processors.
- Current listening boundary: manually set **all six voice filters to FILTER_OFF**, including directly routed voices. No code turns filters off. This avoids known unresolved filter failures; it does not fix them.
- The eligible snare source has a local saturation policy. Ordinary/direct/other-FX/multi-route snare over-range conversion is still unresolved. R11 arithmetic also deliberately corrects waveform-row seams; exact sound identity to R10 is not claimed for those cases.
- A 0.5–1 second idle release was discussed, but is **not implemented**. Existing short hold/fade remains. Fader-curve refinement did not add a long time release.

## Current R18 validation gates

| Gate | Result / boundary |
|---|---|
| Owner's actual instrument listening | **ACCEPTED:** Bass, fluid feedback and soft-ceiling sound all satisfactory; one device |
| Source/ARM image/ZIP | Pinned source, verified ELF/IMG checksum and CI success with own build report |
| R14 core and mixer host checks | 135 core-state comparisons, 75,000 differential mix calculations, 16,384 MAX-selector cases, existing FX-bus and idle-gate tests PASS |
| ITCM/DTCM static fit | 1,176 B ITCM and 23,648 B DTCM nominal free; not physical runtime reserve measurements |
| Actual Cortex-M7 audio CPU/deadlines and stack | **UNMEASURED — owner lacks debugging hardware** |
| Long hardware soak/multiunit/unrelated bugs | **UNMEASURED / NOT CLOSED** |

## Historical R11 validation gates

| Gate | Current result / limit |
|---|---|
| Source/build provenance and static fit | Identified R11 image/ELF and successful existing CI; no new build in synchronization |
| Intended solo CLP MIX/VOL behavior | Owner-confirmed listening under FILTER-OFF conditions; no complete per-voice device test matrix claimed |
| Numerical robustness | PARTIAL; disclosed inherited failures remain |
| Native endurance | QUALIFIED PASS: twelve separate ~120-second processes, ~24 simulated minutes aggregate; not one uninterrupted soak or instrument uptime |
| Native continuation restrictions | All filters OFF; ARM-compatible host CLZ(0) substitution; Delay stereo only; Compressor unity makeup |
| Whole-engine stack bound | INCOMPLETE; compiler frames do not include all indirect/library/interrupt paths |
| Actual render deadlines / buffer freshness | UNMEASURED on the instrument |
| Actual stack high-water / temperature / long-run audio | UNMEASURED |
| More simultaneous effect processors | Not established or approved by current capacity evidence |

No device damage, device underrun or general absence of clicks was demonstrated by the audit. Source defects can produce incorrect audio or memory accesses without implying physical hardware damage. No unconditional hardware-safety or lifetime guarantee follows.

## Open technical findings — historical R11 audit scope

The inherited findings originated in the R11 audit. The November 4–5 work reproduced selected faults on the pinned **original R18** in host fixtures, and separately produced isolated Delay and USB-MIDI candidates. This does **not** constitute a global R18 repair or hardware validation. Consult the technical consolidation section above for the current per-branch status.


These are audit findings, not newly assigned public completed-change IDs. Inherited source defects must not be attributed to CLP changes without evidence.

| Finding | Evidence and next boundary |
|---|---|
| Resonant-filter instability | Retained failing selected fixtures; all-six FILTER_OFF is a workaround. Separate diagnosis required. |
| Legacy non-solo snare conversion | Still fails for selected over-range inputs; eligible R11 snare clamp does not cover fallback paths. |
| USB TX wrap / ownership and RX full ambiguity | Source/extracted tests and isolated ARM probe; e.g. 128-byte ring yields 132-byte send length at read124/write0. Busy/ownership and unread-ring handling need a bounded transport task. No physical USB failure measured. |
| Delay mono-right word-width read | Native sanitizer and delivered instruction evidence; potential final-array overread. Full emulation hits an unsupported instruction, which is a tool limit, not a device fault. |
| Compressor mono float-to-int16 conversion | Valid controls produce about33683.1 before conversion; stereo path already saturates. Future saturation/headroom policy needs a sound decision. |
| CLZ(0) in full FX initialization | Undefined C input domain, but delivered ARM initialization completes; no device boot failure inferred. Host substitution is not a production fix. |
| Audio readiness versus generation freshness | Injected late-render schedule can consume partial/stale data; no actual target deadline miss measured. Measure both DMA streams before scheduler redesign. |
| Long-uptime timing and counters | Float absolute ticks lose single-tick precision around70min at ~4kHz; not a 70-minute crash. Counter-wrap/service-load paths need their own bounded review. |

## Next work and product direction

After the R18 musical closure, separately owner-authorized, isolated R18 test/correction tasks **did** produce a Delay validation candidate and USB-MIDI host-tested candidate on November 4–5, as recorded above. **This synchronization only** updates documentation: no additional tests, source changes, builds, merges, firmware delivery, device flashing, hardware profiling or release are authorized by it. The next executable step needs its own explicit approval. Do not start an integration or further correction campaign automatically.

The product goal remains a larger effect library, different effects per musical voice and multiple serial/parallel effects per voice. A one/two/four-slot pool is an alternative architecture with different capabilities, not an equivalent implementation. Current timing evidence cannot certify any added instance count. Independent hi-hat processing and global Delay-history ownership need explicit design.

Two-pass gate: **R18 CLP functional/musical acceptance is complete**. Objective efficiency, CPU/IRQ/DMA and stack/reliability measurement remains **incomplete** until appropriate hardware instrumentation and a separate owner authorization exist. Preserve the owner's accepted R18 source and artifact; do not alter sound without comparable measurements and explicit permission.

Delivery convention: LXR_V2_R<N>.zip contains only LXR_V2_R<N>.img. Keep technical evidence separately. Normal startup: only LXR LAB R<N> on line1; line2 blank. No effect suffix.

## Separate input investigation

[DATA input status](INPUT_CONTROL_STATUS.md): earlier research is available; the separate encoder audit has not started. Its old blocked-R11 context is superseded. No encoder fix was made.
