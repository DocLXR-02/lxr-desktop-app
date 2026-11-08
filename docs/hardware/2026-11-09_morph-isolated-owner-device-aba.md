# LXR-02 Kit Morph / Filter Drive — Owner-Reported A–B–A Device Result

**Recorded:** 2026-11-09 (Europe/Vienna)
**Status:** Positive owner-reported physical-device comparison; no independent instrument telemetry.

## A–B–A observation

| Phase | Installed firmware | Owner observation |
|---|---|---|
| A1 | Original manufacturer v1.71 | The same Morph / Filter Drive defect was present. |
| B | Isolated transport-command guard | The reported defect was absent; correction behaved cleanly. |
| A2 | Original manufacturer v1.71 reinstalled | The same defect appeared again. |

The owner personally installed and tested both images on an Erica Synths LXR-02 and supplied the observations in conversation. This is **owner-reported hardware evidence**, not an instrument test performed or independently observed by ChatGPT.

## Image and source identities

- A1/A2: official owner-supplied `LXRV2_update_v1.71(1).img`, 273,544 bytes, SHA-256 `188695ea78cd3da0fb6a3e825732bd7fd365d4491a37126003ec513e163e1576`.
- B: experimental `LXR_V2_V1.7.img`, 274,736 bytes, SHA-256 `b503f43a22201cc0eac1fdb3a6cb98f75d15e66a849615ab154f3692152a3143`; exact source commit `c433f0ecc68ba66dc885784ad03905c8be18cf39`.
- Upstream public-source base: `a43cd4fcda23ec3d577dc425261c3eb28bd5abff`.
- B production change: **21 insertions, zero deletions**, exclusively `Core/Src/Preset/presetManager.c`. Separate MIDI parser bounds hardening excluded.

## Prior independent technical verification

- Upstream negative control and candidate host Morph regressions: **PASS**, [CI 42530896384](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42530896384).
- Same-toolchain matched upstream/candidate ARM Release builds: **PASS**, [CI 42531589268](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42531589268); static ITCM +8 B, DTCM 0 B, RAM 0 B, FLASH region +24 B, complete IMG +32 B.
- Independent clean candidate ARM build, native regressions, binary identity and IMG structural checks: **PASS**, [CI 42532306552](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42532306552).
- Separate [candidate ARM build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/cf223d9b32108d6b5964a00a0b78bde3ac754d70/docs/codex-berichte/2026-11-08_morph-device-test-image-arm-build.md).

## Interpretation and limits

The owner's A1–B–A result strongly supports a practical correction of the reproduced defect: the original v1.71 displayed it in two separate sessions and the isolated test candidate did not.

A detailed per-setting test matrix, exact Filter Drive values, audio recordings, time/duration measurements, device traces, full unrelated regression coverage and second-device replication were **not supplied**. Do not invent those results.

The public GitHub firmware source advertises version 1.7 and is **not verified equivalent** to the official manufacturer v1.71 binary. Therefore A1/B/A2 is a compelling installed-device behavioral comparison but not a controlled binary experiment differing solely in the 21 source lines from the official distributed image. The upstream host negative control independently demonstrates the unintended reserved NRPN command dispatch.

CPU/WCET, IRQ/DMA audio deadlines, real stack high-water and long-term reliability remain **UNMEASURED**. No full safety assurance, firmware release, accepted upstream PR or merge is implied.

## Provenance and next decision

The device observation and original problem identification belong to the LXR-02 owner. Source analysis, patch isolation, test automation and report preparation used substantial assistance from OpenAI ChatGPT/Codex. Any upstream PR should disclose this assistance and the exact evidence boundaries.

Next: a narrowly scoped draft PR and owner review before any external submission. This documentation-only report does **not** modify firmware, protected branches, builds or instrument hardware.
