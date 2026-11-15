# R22 — Owner device acceptance and stable designation

**Decision date:** 2026-11-15 (Europe/Vienna).  
**Evidence class:** OWNER HARDWARE / OWNER DECISION (self-reported).  
**Source:** [R22 TEST03 commit](https://github.com/DocLXR-02/lxr-desktop-app/commit/0e37942250f79306627702061d851731bdfdbbc8), branch `release/r22`.  
**Exact existing firmware:** `LXR_V2_R22_TEST03.img`, 297,680 bytes, SHA-256 `da1c882f6600c96fa5926ad4797d618ce13eefc16ab5563b8a161686f1909c0b`.  
**Existing one-image distribution:** `LXR_V2_R22_TEST03.zip`, 213,625 bytes, SHA-256 `1426b2e4fab334ef852b0fda053a035e862e71b05bcbd496a277e2863d901466`.  
**Build evidence:** [CI 43707621969](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43707621969) and [TEST03 build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/2dfe56d8eb71b29a1931a04345c3421f9e12ac4f/docs/codex-berichte/2026-11-15_r22-kit-index-display-test03.md).

## What the owner reported

1. During TEST02 hardware use, the owner reported that Drum-to-Drum copying functioned, while the destination-kit label was one higher than the actual kit slot. The code/test correction was then implemented in TEST03.
2. After TEST03 had passed the independent source/host/CI build verification, the owner reported **"R22 funktioniert und ist von meiner Seite abgenommen"** (R22 works and is accepted by the owner).
3. The owner then explicitly directed that **R22 be treated as the current stable release**. This is a functional acceptance and stable-designation decision for the **specific TEST03 binary and source above**.

The owner did **not** supply a complete device-test matrix, audio recordings, instrument-side firmware checksum readback, testing duration or detailed results for every control or persistence scenario. These are therefore **NOT independently verified hardware measurements**. The host/CI matrix, which includes all 448 slot-label combinations and eight FatFs project round trips, is documented separately in the build report; it is **not** a physical-device test.

## Release boundary and preservation

- R22 TEST03 is **CURRENT STABLE** as of 2026-11-15. R20 is the **PREVIOUS stable recovery reference**; R19 is an older accepted release/recovery reference. The original R18 CLP sound acceptance is a protected *musical reference*, not an active development task.
- No firmware rebuild, rename or repackaging is needed or authorized by this documentation update. The approved ZIP retains the name `LXR_V2_R22_TEST03.zip`; the boot string remains `LXR LAB R22`.
- No `main` merge, GitHub Release, tag, bootloader change, device flashing or new hardware test is performed by this acceptance-record update.
- The TEST03 build, exact source, CI log, report, and the earlier TEST03 STOP report/failed fixture remain independently identifiable. Preserve the successful correction and the failed-test diagnosis for later reuse.
- CPU/WCET, IRQ/DMA deadlines, actual stack high-water, long-run reliability, generalized acoustic equivalence and hardware stress coverage remain **UNMEASURED**. Owner acceptance does not convert these to PASS.
- Any post-closure firmware change requires a new numbered release and explicit owner authorization; no further R22 edits are implied.

**This file records the owner's later decision; historical TEST03 reports correctly describe TEST03 as unaccepted *at the time of build* and must not be rewritten retroactively.**
