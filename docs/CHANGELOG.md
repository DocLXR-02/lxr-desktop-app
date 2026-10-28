# LXR Firmware Lab — Changelog

**As of:** 2026-10-28 · **R4:** unofficial experimental LXR-02 firmware · **Scope:** only changes we have made and the next planned investigation. This is **not** a public inventory of every reported, suspected, or discussed bug.

> [!CAUTION]
> **Experimental firmware — no general safety clearance.** R4 has only undergone short-term testing on **one** device. Long-term operation, compatibility across LXR-02 units, and risks to the instrument, connected audio equipment, and hearing have **not** been independently assessed. Malfunctions, lost data, and damage cannot be ruled out. Use at your own risk. Back up projects externally, retain official manufacturer firmware as a recovery option, and start audio testing at low volume. See [R4 hardware validation](R4_HARDWARE_VALIDATION.md).

## Verified R4 changes

`LXR-###` is our **single sequential public change identifier**. The separate type describes the nature of the change: `FIX` = bug fix, `UX` = usability/design change, `ENH` = technical enhancement or hardening, and `FEAT` = new feature. Type labels do not imply that the original manufacturer functionality is unchanged. Historical internal bug IDs are retained only as cross-references.

| ID | Type | Change | Evidence |
|---|---|---|---|
| **LXR-001** | **FIX** | **Kit morph / Filter Drive:** Addressed the audible click and Drive-value jump via a correction in the NRPN/CC0 path (internal case `BUG001`; [source branch](https://github.com/DocLXR-02/lxr-desktop-app/tree/bug001/filter-drive-fix), commit `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`). | **HARDWARE:** The previously reproducible symptom was confirmed absent on the maintainer's R3/R4 builds. |
| **LXR-002** | **UX** | **Morph control travel:** Reduced the range from 255 to **127** so the target kit can be reached with less knob rotation. Intentional design choice, **not a bug fix**. | **HARDWARE:** The shorter travel was confirmed much more comfortable to use. |
| **LXR-003** | **UX** | **Load confirmation:** Both `Load Kit` and `Load Morph Kit` now briefly display `success!` (internal case `BUG011`). Other save/load screens were not redesigned. | **HARDWARE:** Both confirmation messages were observed. |
| **LXR-004** | **ENH** | **Project save/recovery (S4):** Hardened selected save and recovery paths, including host tests for full cards, unrelated files, a complete first-save staging state, and unambiguous transaction ownership. | **HOST TESTS** reported by Codex; **HARDWARE** checks covered normal kit/pattern/project saves and repeated reboots. **No general power-loss guarantee.** |

**Related observation, not separately numbered as a change:** Internal case `BUG002` (Filter Drive sometimes sounding unchanged after a pattern switch until its knob was moved) was **not reproducible** with AutoSave both on and off in the maintainer's test. No independent causal fix has been established, so this is **not** claimed as a separately completed code fix.

## Next investigation — preview only

**FX bus `DRV/CLP`:** Certain parameter combinations can produce a loud, persistent high-pitched whistle, even after muting Voice 1 with its fader and stopping the sequencer. `D/W` changes its level; lowering certain FX parameters below a threshold stops it. **Observed on hardware; root cause unknown; no fix implemented yet.**

Before modifying the firmware, review related community GitHub issues, pull requests, and fixes, then establish the smallest reproducible test. **Exercise particular caution with monitoring levels.** A new `LXR-` ID will be assigned only when a change is actually implemented.

## References

- **R4 source:** [`ui/kit-load-confirmation`](https://github.com/DocLXR-02/lxr-desktop-app/tree/ui/kit-load-confirmation), commit `c870f7249d7ee0d65423bc8b7bfdb422dcc74482`.
- **R4 delivery:** [`delivery/lxr-lab-r4-kit-load-2026-10-28`](https://github.com/DocLXR-02/lxr-desktop-app/tree/delivery/lxr-lab-r4-kit-load-2026-10-28), commit `eea1949f0641e375f8cc6082971242549aeed1d1`.
- **Firmware image:** `LXR_V2_V1.7.img`, **288,676 bytes**, SHA-256 `1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877` (**not** proof of identity with manufacturer firmware v1.71).
- **Hardware validation and limitations:** [`R4_HARDWARE_VALIDATION.md`](R4_HARDWARE_VALIDATION.md).
- **Transparency:** This independent firmware experiment involved assistance from OpenAI ChatGPT and Codex. The maintainer determined musical requirements and conducted hands-on hardware tests. See this branch's `README.md`.
