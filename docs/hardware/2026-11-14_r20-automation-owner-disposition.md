# R20 automation owner observation and disposition

**Recorded:** 2026-11-14 (Europe/Vienna).  
**Exact test candidate:** [`b01537282047bda5299cdb1bedeab5e5d5b9bcca`](https://github.com/DocLXR-02/lxr-desktop-app/commit/b01537282047bda5299cdb1bedeab5e5d5b9bcca), `LXR_V2_R20_NAMES_TEST.img`, SHA-256 `27ebacee348b8dd42d0a2cf35d1513676c712583c2891559483a8a47353e0056`.  
**Evidence:** [ARM Release and host CI 43404559276](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43404559276), [dedicated build report](../codex-berichte/2026-11-13_r20-automation-target-names-build.md).

## Owner hardware observation — 2026-11-13

After testing the corrected candidate on the LXR-02, the owner reported that the automation assignments were now displayed correctly. This confirms the displayed parameter assignments in the reported device use. The complete test matrix and timing were not supplied.

## Owner decision — 2026-11-14

Proceed on the working assumption that the automation functions. Close active R20 automation work and revisit future defects if observed. The full final device test matrix and a separate stable R20 distribution authorization were not reported. R19 remains the approved stable distribution. Host tests confirm the `0sr` conflict correction, which has no separate owner device confirmation in this record.

CPU/WCET, IRQ/DMA deadlines, stack high-water and long-term reliability remain **UNMEASURED**. This documentation synchronization produced no firmware build or additional device test.
