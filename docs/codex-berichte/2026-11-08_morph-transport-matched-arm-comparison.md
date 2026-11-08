# Matched build comparison — kit Morph transport command guard

Date: 2026-11-08, Europe/Vienna. **SUCCESS.**

## Sources and authorized scope

- Official upstream GitHub source SHA: `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`.
- Isolated Morph candidate SHA: `c433f0ecc68ba66dc885784ad03905c8be18cf39`.
- Candidate patch is 21 insertions / 0 deletions in `Core/Src/Preset/presetManager.c` with host regressions, no MIDI-parser edit.
- Separate CI infrastructure commit: `3a4e846c99f14e62a5b56ce8c6e9d22831f26823`; changes only workflow and comparator script relative to candidate.
- Both builds: Arm GNU 14.3.Rel1, Ubuntu 24.04, CMake Release `-O3`.
- Verified GitHub Actions run: https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42531589268. CI job `matched-arm-build`: SUCCESS.
- Independent two-phase host Morph control-path CI: [42530896384](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42530896384), PASS.

## Matched static results

| Metric (bytes) | Upstream | Candidate | Candidate minus upstream |
|---|---:|---:|---:|
| ITCM_RAM | 15000 | 15008 | +8 |
| DTCM | 107392 | 107392 | 0 |
| RAM | 271036 | 271036 | 0 |
| FLASH (linker region) | 222692 | 222716 | +24 |
| Full IMG | 274704 | 274736 | +32 |
| IMG payload | 274688 | 274720 | +32 |
| GNU text | 273828 | 273860 | +32 |
| GNU data | 852 | 852 | 0 |
| GNU BSS | 340572 | 340572 | 0 |

Both built IMG files passed format, checksum, payload equality and vector checks. The nominal ITCM remaining capacity is 1376 bytes in the candidate; that number is a **static linker gap, not CPU or stack headroom**.

## Image hashes

- Upstream IMG SHA-256: `b34c0c3a946d02c5cee624351a44c2597694b4832fdaa9cbaeb3627f84979afe`.
- Candidate IMG SHA-256: `b503f43a22201cc0eac1fdb3a6cb98f75d15e66a849615ab154f3692152a3143`.
- Images were generated temporarily for CI validation; neither was uploaded as a firmware release or included in the retained evidence ZIP.

## Retained primary evidence

- CI log https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42531589268
- Evidence artifact ID `13636764067`, SHA-256 `28aacae068518c1c1d637a0c4b4c6b1d413acf575c4c6614bce43c27ccbec3cf`, valid ZIP independently inspected.
- GitHub Actions artifact expiry: 2026-11-15 22:43:30 UTC.
- Separate immutable English reports on this documentation-only branch:
  - `docs/codex-berichte/2026-11-08_morph-transport-upstream-arm-build.md`
  - `docs/codex-berichte/2026-11-08_morph-transport-candidate-arm-build.md`
- Comparative logs, MAPs, section summaries and JSON manifests were verified from archive; historical material is not rewritten.

## Technical and safety boundaries

The observed Morph problem dates to the owner's original v1.71 installation. Public source/binary equivalence to the manufacturer's v1.71 firmware has **not** been established; no unverified official release labeling is inferred.

A previous owner hardware test supports a combined laboratory correction; it does not establish isolated-patch audio success. This task did not flash firmware, test a physical instrument, run audio DSP equivalence, or measure worst-case CPU, IRQ/DMA deadlines, live stack use, and long-duration stability. All remain NOT TESTED / UNMEASURED.

No upstream PR, merge, firmware binary delivery, protected reference edit or bootloader modification occurred.

This source/test/report workflow was substantially assisted by OpenAI ChatGPT. The owner-identified failure and earlier physical-device observations are distinguished from CI and inferred evidence.
