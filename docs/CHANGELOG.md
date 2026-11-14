# LXR Firmware Lab — Changelog

**As of:** 2026-11-14 · **Owner-accepted FX-CLP sound reference: R18 (CLP v2.1).** R4, R11 and R14 remain historical reference versions. Scope: implemented lab changes and their real source/build/hardware evidence, not every reported or suspected bug. [R18 owner acceptance](CLP_R18_SONIC_ACCEPTANCE.md). See [current state](PROJECT_STATUS.md).

> [!CAUTION]
> **Experimental firmware — no general safety clearance.** The lab revisions have owner listening/short-term testing on **one** device; the historical R11 investigation used an agreed FILTER-OFF setup. R18 has now been **musically accepted on that device**, but target CPU deadline, stack high-water, long-term operation, compatibility across LXR-02 units, and risks to connected audio equipment/hearing have **not** been independently assessed. Malfunctions, lost data, and damage cannot be ruled out. Use at your own risk. Back up projects externally, retain official manufacturer firmware as a recovery option, and start audio testing at low volume. See [R4 hardware validation](R4_HARDWARE_VALIDATION.md).

## Numbered firmware revisions

| Firmware revision | Scope and documented status |
|---|---|
| **R19** | Approved stable distribution. Combined Delay mono-safe access, USB-MIDI buffer ownership and Compressor mono saturation fixes. [Release record](releases/2026-11-08_r19_final_owner_approved_distribution.md) |
| **R20** | Live two-lane automation and FIX8 playback, REC/LCD feedback and conflict protection, STEP synchronization and target name correction. Current test candidate [`b01537282047bda5299cdb1bedeab5e5d5b9bcca`](https://github.com/DocLXR-02/lxr-desktop-app/commit/b01537282047bda5299cdb1bedeab5e5d5b9bcca). Owner confirmed the displayed assignments and closed active automation work. [Owner record](hardware/2026-11-14_r20-automation-owner-disposition.md) |
| **R21** | Next sequential number for a newly authorized change. Assignment starts with the Codex task. [Workflow](WORKFLOW.md) |

## Verified R4 changes

`LXR-###` is our **single sequential public change identifier**. The separate type describes the nature of the change: `FIX` = bug fix, `UX` = usability/design change, `ENH` = technical enhancement or hardening, and `FEAT` = new feature. Type labels do not imply that the original manufacturer functionality is unchanged. Historical internal bug IDs are retained only as cross-references.

| ID | Type | Change | Evidence |
|---|---|---|---|
| **LXR-001** | **FIX** | **Kit morph / Filter Drive:** Addressed the audible click and Drive-value jump via a correction in the NRPN/CC0 path (internal case `BUG001`; [source branch](https://github.com/DocLXR-02/lxr-desktop-app/tree/bug001/filter-drive-fix), commit `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`). | **HARDWARE:** The previously reproducible symptom was confirmed absent on the maintainer's R3/R4 builds. |
| **LXR-002** | **UX** | **Morph control travel:** Reduced the range from 255 to **127** so the target kit can be reached with less knob rotation. Intentional design choice, **not a bug fix**. | **HARDWARE:** The shorter travel was confirmed much more comfortable to use. |
| **LXR-003** | **UX** | **Load confirmation:** Both `Load Kit` and `Load Morph Kit` now briefly display `success!` (internal case `BUG011`). Other save/load screens were not redesigned. | **HARDWARE:** Both confirmation messages were observed. |
| **LXR-004** | **ENH** | **Project save/recovery (S4):** Hardened selected save and recovery paths, including host tests for full cards, unrelated files, a complete first-save staging state, and unambiguous transaction ownership. | **HOST TESTS** reported by Codex; **HARDWARE** checks covered normal kit/pattern/project saves and repeated reboots. **No general power-loss guarantee.** |

**Related observation, not separately numbered as a change:** Internal case `BUG002` (Filter Drive sometimes sounding unchanged after a pattern switch until its knob was moved) was **not reproducible** with AutoSave both on and off in the maintainer's test. No independent causal fix has been established, so this is **not** claimed as a separately completed code fix.

## CLP development after R4

IDs continue the existing public sequence. Revisions are builds, not change IDs. These intentional sound/control changes are not all manufacturer bug fixes; hardware observations apply only to the reported setups.

| ID | Type | Implemented change | Evidence / boundary |
|---|---|---|---|
| **LXR-005** | **ENH** | R5 idle-input feedback suppression and bounded diode endpoint | Owner reports unwanted idle whistle absent. Intentional departure from documented original self-oscillation. Short hold/fade; no0.5–1s release. |
| **LXR-006** | **FEAT** | R7 parallel bass outside the CLP feedback core | Owner selected this after rejecting R6's bass-through-feedback variant. |
| **LXR-007** | **UX** | R8 audible core follows fader; R9 revised attenuation knee | Owner accepted R9. Settled extra core gain=min(1,2u), full from50% physical travel. |
| **LXR-008** | **ENH** | R10 maximum configured routed-fader control of the shared core | Owner reports playable additions/removals. A silent high routed fader can hold the core open; no separate wet stems. |
| **LXR-009** | **UX** | Corrected R10 startup display / future naming convention | Line1 only LXR LAB R<N>; line2 blank. Corrected R10 artifact supersedes original R10 despite identical filenames. |
| **LXR-010** | **FIX** | R11 finite phase conversion, row-local periodic interpolation and transient arithmetic prerequisites | Source/host/ARM-artifact evidence; declared waveform-seam differences, not universal sound identity. |
| **LXR-011** | **UX** | R11 solo CLP post-return MIX/VOL; eligible snare full-source saturation included | Owner confirms intended level control with all six filters OFF. Exactly one configured CLP route; multi-route/direct/other-FX fallback limitations remain. |
| **LXR-012** | **ENH** | R13–R14 shared CLP feedback-fader refinement, including R14 MAX across configured FX-routed paths | [R14 source and build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/c3892715079b76f4f4e4c952e07cf8a8e1729e79/docs/codex-berichte/2026-11-03_r14-clp-max-feedback-optimization-build.md); owner found multi-voice feedback fluid and playable. R14 remained bass-deficient at partial dry/wet. |
| **LXR-013** | **ENH** | **R18 CLP v2.1:** preserve original 7200-Hz feedback core; replace R7 parallel bass with 200-Hz VOL-coherent differential output crossover and wet-dependent rational soft ceiling | [R18 build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/1eee9ed318b592cec54379bcc255927968b936a3/docs/codex-berichte/2026-11-03_r18-clp-v21-soft-build.md); [owner listening acceptance](CLP_R18_SONIC_ACCEPTANCE.md): kick bass, expressive feedback and soft ceiling all approved. CPU/stack measurements outstanding. |

The arithmetic prerequisite is a separate commit from solo post-MIX. The snare clamp is limited to that eligible R11 path; it is not a global snare fix. R6 was an evaluated alternative, not a retained cumulative stage. R15 (110 Hz), R16 (1000 Hz), and R17 (3000 Hz) were distinct, documented tuning experiments, **not additive steps in the final R18 source**. R18 was built from pinned R14 at `00ea59d38de6f006dfdbcd472c2cfe5040a440c3`. No standalone public ID is assigned to an unimplemented audit finding.

## Historical R11 audit and R18 musical acceptance

The [final R11 build handoff](https://github.com/DocLXR-02/lxr-desktop-app/blob/84bc8a8c81fbc3d348c509ee5a2c7d5329d4cf8c/docs/codex-berichte/2026-10-31_r11-arithmetic-and-filter-off-post-mix.md) is now available. The [resource/reliability audit](https://github.com/DocLXR-02/lxr-desktop-app/blob/fa3c5b711733d022301669f0bdacd650cde5d6d9/docs/codex-berichte/2026-11-01_r11-resource-reliability-audit.md) and [input reconciliation](https://github.com/DocLXR-02/lxr-desktop-app/blob/3ca14c236a2301b3883b6ebd377f9dee68385d4e/docs/codex-berichte/2026-11-01_r11-resource-audit-knowledgebase-addendum.md) add evidence and a proposed measurement plan; they do not change or freeze firmware. Native endurance is qualified, and actual device CPU/timing/stack/thermal margins remain unmeasured. R18's matched linker estimates are 1,176 B nominal free ITCM and 23,648 B nominal DTCM gap before real stack use. **The owner has no debugging hardware**, so the CLP chapter is closed for sound only, with the objective reliability measurements recorded as unknown. Open historical USB/Delay/Compressor/CLZ/filter/timing findings remain separately listed in [PROJECT_STATUS.md](PROJECT_STATUS.md).

**Chapter closure:** [owner-accepted R18 sonic reference and pinned SHA](CLP_R18_SONIC_ACCEPTANCE.md). No new firmware, unrelated issue or post-CLP change is initiated in this documentation synchronization. **The next changelog issue starts in a new chat**, after new scoped authorization; preserve R18 sound unless a change is separately approved.

## References

- **R4 source:** [`ui/kit-load-confirmation`](https://github.com/DocLXR-02/lxr-desktop-app/tree/ui/kit-load-confirmation), commit `c870f7249d7ee0d65423bc8b7bfdb422dcc74482`.
- **R4 delivery:** [`delivery/lxr-lab-r4-kit-load-2026-10-28`](https://github.com/DocLXR-02/lxr-desktop-app/tree/delivery/lxr-lab-r4-kit-load-2026-10-28), commit `eea1949f0641e375f8cc6082971242549aeed1d1`.
- **Firmware image:** `LXR_V2_V1.7.img`, **288,676 bytes**, SHA-256 `1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877` (**not** proof of identity with manufacturer firmware v1.71).
- **Hardware validation and limitations:** [`R4_HARDWARE_VALIDATION.md`](R4_HARDWARE_VALIDATION.md).
- **Transparency:** This independent firmware experiment involved assistance from OpenAI ChatGPT and Codex. The maintainer determined musical requirements and conducted hands-on hardware tests. See this branch's `README.md`.
