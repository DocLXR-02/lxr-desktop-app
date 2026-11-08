# R19 Candidate — Owner-Reported Successful Device Installation and Functional Test

**Recorded date:** 2026-11-08 (Europe/Vienna). The owner's exact physical test times, test duration, and equipment configuration were not supplied.  
**Evidence class:** Owner firsthand physical LXR-02 confirmation, not independently instrumented or assistant-operated device test.  
**Status:** **OWNER-REPORTED PASS — R19 candidate successfully installed and tested without reported problems.** Final public/release approval remains **NOT GRANTED**.

## Candidate identity and build evidence

| Field | Value |
|---|---|
| Repository | `DocLXR-02/lxr-desktop-app` |
| Candidate source branch | `candidate/r19-integrated-three-fix-2026-11-07` |
| **Exact R19 candidate source SHA** | **`638b6eaae5b1624637d20cbc11a4a3e0175edca2`** |
| Parent technical integration source | `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` |
| Preserved original accepted R18 source | `41b364067bce0d8cde624c184933d9cb88e2725b` |
| CI | [Run 42407160858](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42407160858) — SUCCESS |
| R19 candidate firmware file | `LXR_V2_R19_CANDIDATE.img` |
| Candidate image size | 288,184 bytes |
| Candidate image SHA-256 | `a1d53435ce922c4ef57ce595bf8c180b5225340780b6ad3241b6340af652e57b` |
| Startup marker embedded in candidate image | `LXR LAB R19` |
| Verified build report | [2026-11-07 R19 version-only candidate ARM build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/ce2d87d499a5708881f38b0fe0ffa6b0e50c28f0/docs/codex-berichte/2026-11-07_r19-version-only-candidate-arm-build.md) |

The build report independently established that the R19 candidate IMG differs from the earlier integrated TEST IMG by only the embedded startup marker byte and corresponding image-checksum byte. All production DSP and routing code is preserved; resource sections and compiler command JSON are identical to the integrated reference. The firmware image hash above is the **distribution image**, not a physical device readback hash.

## Owner hardware report on 2026-11-08

The owner stated:

> “R19 Kandidat wurde installiert und erfolgreich ohne probleme getestet.”

This establishes the following observations, and no more:

1. **Installation completed successfully** on the owner's real LXR-02, according to the owner. **PASS — owner-reported.**
2. **A functional test completed without problems noted** by the owner. **PASS — owner-reported, test specifics not supplied.**
3. **No new fault reported** during the owner's completed R19 candidate testing.

The owner did not specify which FX parameters, voices, output combinations, project save/reload sequences, test duration, volume settings, physical ports, MIDI equipment, or long-run stress conditions were used in this latest R19 session. Do **not** silently treat the previously tested three-fix TEST image's CLP and Delay observations as new per-case R19 measurements. The separate earlier TEST observations are still relevant historical evidence. The transient multi-effect clicking noted on 2026-11-07 disappeared after reloading another project and then the originally affected project; its exact transient root cause remains UNKNOWN, and no active recurrence was reported with R19.

## Open limits, authorization and release decision

- Physical USB-MIDI validation remains **NOT RUN / BLOCKED by unavailable test equipment**. The owner explicitly accepted its absence as **not a blocker for R19 candidate preparation**; that is not a device MIDI PASS.
- Live Cortex-M7 CPU/WCET, IRQ/DMA/audio deadlines, actual stack high-water, thermal and long-term reliability remain **UNMEASURED**. A short successful functional test cannot establish them.
- The owner has **not yet explicitly authorized a final R19 release, public GitHub release, merging to protected `main`, a bootloader change, or edits to accepted R18**. This owner report is **not** automatic permission to promote the candidate.
- No new sound modification, source change, build, hardware action, flash operation, GitHub merge, or release was performed by ChatGPT to record this report. The accepted R18 source and the three-fix integration source remain immutable.

**Next decision:** Obtain a separate, explicit owner final-acceptance/release authorization identifying the precise candidate SHA/IMG and explicitly accepting the known unmeasured and untested limits. Only after approval consider a controlled publication of that **same validated image** (not rebuilding/renaming image bytes without independent verification), with a separate verified release record and unchanged protected branches.
