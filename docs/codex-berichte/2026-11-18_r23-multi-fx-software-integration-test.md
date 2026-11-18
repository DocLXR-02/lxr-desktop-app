# R23 P17-B — Software-only multi-FX integration and safety gate

**Date:** 2026-11-18 (Europe/Vienna)  
**Disposition:** **PASS for tested FX control-plane; production-DSP multi-effect integration still OPEN; no device available; NOT FOR FLASHING.**  
**Exact firmware source (unchanged):** `release/r23` at `238389475359f11bf317b446e668e0c82333c231`.  
**Owner-accepted protected R22 source:** `0e37942250f79306627702061d851731bdfdbbc8`.  
**Test-only branch:** `experiment/r23-integration-host-2026-11-18`, tested head `d5455a7ac1ecac7579d464a103071b5330ea7c9c`.  
**Test implementation:** `tests/r23_fx_integration/prod_fx_return.c` and `.github/workflows/r23-fx-integration-host.yml`. No production source changed.

## Approval and owner design notes

Owner explicitly approved host/source-only integration, memory safety, routing transition and FX-tail investigation. LXR-02 is with the manufacturer for DATA encoder examination; physical testing is impossible now.

Owner notes to retain for a separately authorized design decision: (1) try all four effect types if CPU budget permits, but **drop Compressor if needed**, keeping Distortion/CLP, Delay and Ring; (2) optionally consider removing CLP's fader-controlled *feedback* reduction while retaining fader-controlled **output volume**, explicitly accepting that this would alter sound; (3) future multi-FX **must have independently adjustable parameter banks per effect**, not one global four-parameter array. The owner's historical report that five CLP instances ran and six caused trouble is contextual owner observation, **not current STM32 CPU/WCET evidence**. None of these proposed sound or parameter changes were implemented.

## Reused existing CI and new exact-source tests

The preceding R23-A selection [CI 44302818179](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44302818179) independently passed host regressions and ARM Release build; static ITCM occupancy **13,968 B**, nominal free **2,416 B**, DTCM region **109,216 B**, general SRAM **275,164 B**. It includes **280,032 real SharedFxPool selector cases**, inherited isolated real-DSP Delay/Compressor/CLP regressions, USB-MIDI, R21/R22 voice/kit copy and storage checks. These were **reused, not rerun** in the new task.

For the new integration test, the actual `FxBus_processSelected()` from `FxBus.c` and the multi-FX output return loop from `mixer.c` were copied from the exact pinned source into an isolated C harness, with an independent CI guard comparing both source spans **byte-for-byte** to the corresponding `git show` output. The **four DSP processing bodies are deterministic stubs**, not the production DSP algorithms. A separately written saturated PCM oracle checks output routing and effect summation.

[Final GitHub Actions CI 44313310168](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44313310168) **PASS**, test SHA `d5455a7ac1ecac7579d464a103071b5330ea7c9c`:

- **1,728 scenarios** = 32 masks × 6 output destinations × 3 Dry/Wet levels × 3 source sample profiles; 64 stereo frames per scenario.
- **PASS** source-extracted FX dispatcher plus stereo/mono output summation versus independently calculated saturated reference; exactly one DSP-stub invocation per active effect per block; common output and Dry/Wet propagation confirmed.
- **PASS** GCC AddressSanitizer and UndefinedBehaviorSanitizer for this tested dispatcher/return path. No buffer or undefined behavior finding on the exercised paths. Not a whole-firmware sanitizer result.
- **PASS observed** routing activation sequence Delay → Ring → Delay produces Delay processor call count `1 -> 1 -> 2`: Delay is not processed while it is not routed. It is not reset by the tested return loop. Actual residual sound requires production Delay tests.
- [Original CI evidence artifact 14432005329](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44313310168/artifacts/14432005329), ZIP SHA-256 **`69ae7ee9e53506cc3686fe7b43de61d8dbf6cfdfc1e27955244b6005e6455551`**, original ZIP CRC PASS; expires 2026-12-18. Preserved with owner report in verified `LXR_R23_FX_Integration_Software_Test_Evidence_2026-11-18.zip`, SHA-256 `36107b7553a6707c59569a704a897b127911cbc439e5e04630a54ba1f6cf0a5e`.

Initial CI [44313138718](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44313138718) **FAIL/STOP** because the test incorrectly expected a CLP state reset *inside* the isolated return loop. The real mixer performs that reset immediately **before** this loop, outside the extracted test. Only the unsupported assertion was removed on the test-only branch; production code was untouched. The failure is retained, not reclassified as a firmware bug.

## Confirmed source facts / non-proven risks

1. **Independent FX parameters NOT implemented:** `FxBus` has one global `fxType`, one `audioRouting`, one `dryWet` and one `parameter[4]`. Each effect has a separate runtime processor state, but the parameter updater targets only the globally selected effect. Current `ParameterArray.c`, MIDI control and kit/automation paths still refer to those global fields. The new route IDs 7–10 are not fully wired into menu and persistence.
2. **Delay tails:** With no voice routed to Delay, its processor is not scheduled, while the old global delay ring buffer is not cleared by the routing loop. A later re-enable could expose old state. Exact audible behavior is UNKNOWN; define tail/clear policy before shipping.
3. **Possible mid-block routing consistency hazard:** FX active mask is computed before per-voice rendering, but routing is reread later in the render process. A simultaneous MIDI/menu update could cause mismatch between cleared/processed FX buffers and the new voice target. Whether production scheduling permits that exact interleaving is **UNPROVEN**; investigate rather than label it a confirmed race.
4. **Mixed-output saturation:** multi-FX returns use `__QADD16` in effect order; clipping/order-dependent summation under high levels is a design property, not by itself a defect. A future full-DSP host mix test must cover peaks.
5. **CLP fader feedback:** accepted existing code still computes max-routed fader feedback. Whether removing it saves meaningful CPU remains **UNMEASURED**, and sonic impact needs separate authorization/listening.
6. **Compiler:** six existing R23 mixer warnings remain (five enum-compare, one misleading-indentation). No source cleanup was performed.

## Known limits, safety and next gate

**NOT DONE:** real simultaneous Distortion/Ring/Compressor/Delay DSP invocation in one true end-to-end host audio graph; actual Delay tail waveform on route changes; production MIDI ISR interleaving; per-effect parameter storage design implementation; CPU/WCET, DMA/IRQ deadlines, stack high-water, multi-hour reliability or physical audio testing. These are still OPEN/UNMEASURED. The test uses DSP stubs and **must not** be described as proof of four-real-FX capacity or musical acceptance.

No actual firmware build, production edit, merge, hardware test, flash, release, bootloader edit or change to stable R22, main or accepted R18 sound. A suitable follow-up with new scope approval is a production-DSP-aware host integrated test and exact tail-transition contract. Only after evidence should owner select independent per-effect parameter bank design and any three-FX/CLP sonic alternatives.

**Final status:** Shared FX routing and return control-plane test PASS; complete integration and real-time/sonic acceptance OPEN. Do not flash or promote R23.
