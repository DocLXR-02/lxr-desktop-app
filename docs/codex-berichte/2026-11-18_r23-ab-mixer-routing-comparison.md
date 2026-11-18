# R23 A/B — Mixer routing experiment comparison and continuation handoff

**Date:** 2026-11-18 (Europe/Vienna)  
**Scope:** Owner-authorized isolated A/B changes to the shared FX pool mixer, source/build identity review and host regression testing; **no device testing** while the LXR-02 is at the manufacturer for DATA-encoder inspection.  
**Firmware repository:** `DocLXR-02/lxr-desktop-app`.  
**Owner-approved stable reference:** R22 TEST03 `0e37942250f79306627702061d851731bdfdbbc8`.  
**Unmodified experimental baseline:** `release/r23` at `d0b49dff26b6fcf11e0367249d329a6682bfefa4`.

## Results

| Exact candidate | SHA | ITCM occupied | ITCM free | Host / ARM |
|---|---|---:|---:|---|
| Accepted R22 source (comparison only) | `0e379422` | 15,304 B | 1,080 B | Prior owner stable |
| R23 current prototype | `d0b49dff` | 15,960 B | 424 B | Prior CI PASS |
| **R23-A noinline mixer** | **`a6e0cf675837ad01f94e52d7813b2d49e5795814`** | **13,968 B** | **2,416 B** | **PASS / PASS** |
| R23-B block-prepared routing | `fee14e391e804150948d6084d34d1fe3e6be92d9` | 14,376 B | 2,008 B | PASS / PASS |

- A is **1,992 B smaller in ITCM** than the unchanged R23 prototype and **1,336 B below** accepted R22 ITCM occupancy. It modifies only the inline decision for `mixer_addDataToOutput()`.
- B inherits A but adds one-block routing snapshots and FX pointer precomputation. B is **408 B larger** than A in ITCM and can subtly change routing behavior on mid-block parameter or jack state updates. No convincing advantage has been demonstrated.
- Both retain DTCM region 109,216 B and general SRAM 275,164 B. Linker/ELF/MAP are static evidence; they do not measure CPU cycles or live stack margin.
- **Recommendation:** keep A as the *preferred isolated experiment*, B as an archived alternative. Do **not** merge to `release/r23`, release, flash or claim audio stability without a separately authorized selection and validation step.

## Verified CI and original evidence

- [R23-A CI 44291774697](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44291774697) PASS; ARM artifact 14422395538, SHA-256 `42e91e391de02d595fb193d9acf2a4bb55277fa6dfdb9117babf5106e5595f44`; host artifact 14421468446, SHA-256 `ed33e9b73aec6340665787a5351b132c74755a79ab107d52400b64d078153d08`.
- [R23-B initial CI 44292091682](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44292091682) **STOP** on whitespace gate; its failure remains preserved; [final CI 44292230152](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44292230152) PASS; ARM artifact 14421737163, SHA-256 `40cd3012b2853818acadbbf92d61534d3435f33c900ab69fc24bb1f23073388d`; host artifact 14422565663, SHA-256 `d613e7482edec26069b334441c8e4d27aaf4e9d193a3c0610990db9c6d7d2990`.
- All four original CI evidence ZIP files were independently downloaded and CRC/SHA-256 verified. They include full ELF/MAP and host logs, **not flashable IMG**; CI-reported IMG bytes/hashes are documented separately in the one-build reports. GitHub artifact expiry: 2026-12-18 UTC.
- Dedicated A and B English build records on this documentation branch contain full image/ELF/MAP identities, exact source and changes.

## Limitations and next authorization

No actual hardware operation was performed because the device remains at the manufacturer. The executed host suite covers the standalone shared FX selector and inherited R18/R21/R22 tests, **not** an integrated R23 simultaneous-effects production-audio test. CPU/WCET, IRQ/DMA deadlines, high-water stack, long-run reliability and actual LXR-02 audio remain **UNMEASURED**. The shared FX pool still needs effect-tail rules, independent parameter banks, OUT menu and persistent kit design before it can be offered for owner testing.

**Decision boundary:** This task authorized comparison changes only. Selection of A for `release/r23`, further code/test work, development firmware distribution and device tests require new owner approval. R18 accepted sound, R22 stable and `main` remain unchanged.
