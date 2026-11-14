# R20 stable firmware approval and automation observations

**Recorded:** 2026-11-14 (Europe/Vienna).  
**Exact approved build:** [`b01537282047bda5299cdb1bedeab5e5d5b9bcca`](https://github.com/DocLXR-02/lxr-desktop-app/commit/b01537282047bda5299cdb1bedeab5e5d5b9bcca), `LXR_V2_R20_NAMES_TEST.img`, SHA-256 `27ebacee348b8dd42d0a2cf35d1513676c712583c2891559483a8a47353e0056`.  
**Evidence:** [ARM Release and host CI 43404559276](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43404559276), [dedicated build report](../codex-berichte/2026-11-13_r20-automation-target-names-build.md).

## Owner hardware observation — 2026-11-13

After testing the corrected candidate on the LXR-02, the owner reported that the automation assignments were now displayed correctly. This confirms the displayed parameter assignments in the reported device use. The complete test matrix and timing were not supplied.

## Owner decisions — 2026-11-14

The owner first chose to proceed on the working assumption that automation functions, close active R20 automation work, and revisit future defects if observed. The owner then explicitly approved the corrected build above as the lab's new stable firmware. The full device test matrix was not reported. Host tests confirm the `0sr` collision correction. There is no separate owner device result for that correction in this record.

CPU/WCET, IRQ/DMA deadlines, stack high-water and long-term reliability remain **UNMEASURED**. The stable designation applies to the original binary. The documentation update produced no firmware build, additional device test, or native GitHub release.
