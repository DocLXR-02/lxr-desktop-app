# R18 Three-Fix Candidate — Initial Owner Device Observations

**Recorded:** 2026-11-07 (Europe/Vienna). Exact physical test timestamps were not supplied.  
**Evidence class:** Owner's direct device observation, reported in conversation; not assistant-performed measurement.  
**Status:** Initial installation and CLP listening/routing checks **reported PASS**; remaining device tests **NOT REPORTED**.

## Exact candidate

- Installed file reported by owner: `LXR_V2_TEST.img`.
- Integrated source commit: `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` (Delay mono access, USB-MIDI ring ownership, Compressor mono saturation).
- Packaged, previously verified IMG: 288,184 bytes; SHA-256 `50b2638aa8928e1bcaea79d1101a46bb0e71c7e5a203f0ebdde9a4fd1ba56dc7`. The owner has **not independently reported a post-flash readback or device hash**; these hashes identify the distributed test artifact, not a verified device readback.
- Accepted sonic reference remains original R18, source `41b364067bce0d8cde624c184933d9cb88e2725b`, unchanged.
- Backup: Before the installation, the owner reported that the SD card was backed up. Backup readability/restoration has **not** been independently verified by the assistant.
- Approval record: [Owner authorization addendum](../plans/2026-11-05_r18-three-fix-device-test-install-authorization.md); the separately retained [device test and backup/recovery plan](../plans/2026-11-05_r18-three-fix-device-test-backup-recovery-plan.md).

## Owner-reported results

On 2026-11-07, the owner reported:

1. **Firmware installation:** `LXR_V2_TEST.img` successfully flashed to the actual LXR-02. **PASS, owner-reported.**
2. **FX-CLP sonic character:** The characteristic sound of the CLP effect continues to sound unchanged and satisfactory compared with the previously accepted version. **PASS, subjective listening assessment.**
3. **Multi-voice CLP routing:** All voices routed through CLP continue to work. **PASS, owner-reported.** The message does not supply enumerated voice-by-voice measurements, electrical outputs or duration.
4. **CLP routing with filter enabled:** No problems observed with the filter **ON**. **PASS, owner-reported**, for the conditions actually tried.

These observations do **not** establish bit-for-bit acoustic equivalence, all filter settings or all signal paths, quantitative gain behavior or absence of rare audio defects. Preserve the difference between the owner hearing no problems and an instrumentation-based pass.

## Outstanding gates and next logical device checks

**Not yet reported:** Targeted Delay normal/ping-pong mono-left/mono-right behavior; real USB-MIDI clock/Note-On/Note-Off, busy, bursts, buffer-capacity and reconnect behavior; Compressor mono at high make-up gain and dry/wet; longer multi-voice and soak/stress operation. Do not infer that successful CLP listening validates these fixes.

**UNMEASURED:** CPU/WCET, IRQ/DMA deadlines, stack high-water and long-term reliability. These require actual suitable instruments or instrumentation and cannot be inferred from host tests, static ARM resource reports or short listening checks.

**Immediate suggested next test:** Follow the existing device plan's Delay section, beginning at low output gain and with a saved kit/project; exercise normal and ping-pong operation and mono-left/mono-right routing, listening for discontinuities, channel loss, unexpected feedback or failure. Stop and recover using the approved plan if any fault occurs.

## Release identity

The installed file remains a **TEST** candidate whose current built-in startup text is `LXR LAB R18`. It is **not** a released R19. The requested naming policy is: original accepted R18 stays R18; if all remaining device checks and final approvals succeed, produce a separately validated release with R19 file and startup identifiers. No such release, boot-string source edit or release build was made by this observation record.

**Repository action:** Documentation-only update on the existing plan branch. No production changes, merges, test runs, builds, device flashing or releases were performed by ChatGPT for this record.
