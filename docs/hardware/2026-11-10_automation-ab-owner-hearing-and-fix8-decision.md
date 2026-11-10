# Owner Hearing — Continuous Automation Playback and FIX8 Acceptance

**Date:** 2026-11-10 (Europe/Vienna)  
**Classification:** OWNER PHYSICAL DEVICE SUBJECTIVE AUDIO OBSERVATION / OWNER DESIGN ACCEPTANCE  
**Actually heard source:** A/B candidate `3ca1256ba62ee5cf4411a68cc0df6c6769ec98b9`, exact image SHA-256 `45604028ba6b655422e33578a24b3d84baedd180f82914d3a9f38a8e5b5abcb9`.  
**Final removal-only source:** `e82fd4f3c239bfcb21400c3f94756f6fced02586` (no owner flash/test observation yet).

## Controlled listening setup as reported by owner

The device was operated at **120 BPM**. A single note was triggered on **Step 1** and continued decaying through **Step 16**. One **COA** control was automated over a wide range in both directions to produce a relatively fast parameter movement.

## Observations

- Step-independent playback changed the continuous decay across silent Steps 2–16: **works very well** per owner.
- `fix` with `res=8x`: parameter motion was already perceived as **flowing/smooth**.
- `fix` with `res=4x`: individual steps remained slightly audible.
- `aut` with `res=8x`: sounded approximately like fixed 4x; original adaptive threshold was insufficiently quality-preserving for this fast COA automation.

These statements reflect audible perception, not a calibrated measurement of stepping, spectral artifacts, CPU, memory, missed deadlines, or device endurance. Other parameters and seven-lane-plus parallel operation were not asserted as physically tested.

## Owner decision and scope

Following the review of the code's theoretical CPU event cost, the owner deliberately selected **FIX 8x as the sole standard**, rejected continued adaptive processing at this stage, instructed removal of the `res` and `alg` menu entries and requested that the adaptive approach be preserved **in documentation only**. The owner explicitly regards the live-recording change sequence as **closed**.

The A/B test source is preserved historically. The revised FIX8-only source passes full host/ARM CI with exhaustive numerical parity but **has not itself been device tested** in this record. No new approved public firmware distribution, merge or release is implied. Prior R19 remains the verified stable firmware baseline; R18 CLP sound remains accepted and protected.

See [archived adaptive design](../design/2026-11-10_adaptive-automation-retained-design.md) and [exact FIX8 final build report](../codex-berichte/2026-11-10_live-automation-fixed8-final-build.md).
