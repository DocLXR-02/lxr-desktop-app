# R23 — Four original DSP effects integrated on host: verified software test

**Date:** 2026-11-18 (Europe/Vienna)  
**Status:** **PASS bounded host test; experimental; NOT FOR FLASHING**.  
**Source:** `DocLXR-02/lxr-desktop-app`, exact production `release/r23` SHA `238389475359f11bf317b446e668e0c82333c231` (**unchanged**).  
**Final isolated test SHA:** `d6fa49933f58a5ab82cd6df2e643eaceb814116c` on `experiment/r23-real-dsp-integration-2026-11-18`.  
**Protected R22 stable:** `0e37942250f79306627702061d851731bdfdbbc8`, unchanged.

## Authorization and test implementation

The owner requested a host integration test with actual **Distortion/CLP, Ringmod, Compressor and Delay** production implementations. The physical LXR-02 was sent to the manufacturer for DATA knob inspection, so there were no hardware, flash, release or listening tests.

Only two **test/CI files** were added on the separate experimental branch: `tests/r23_real_fx/run.py` and `.github/workflows/r23-four-real-dsp-host.yml`. There were no changes to production DSP, accepted R18 CLP, `release/r23`, `release/r22`, `main`, bootloader, kit data, or external parameters.

The generated native C test sources the four exact production DSP translation units with `git show` at the above SHA, dropping only firmware-specific include directives, and compiles the original bodies with host type definitions, original delay/one-pole macros, native `_Float16` buffer and emulated ARM saturating SIMD operations. Production oscillator/wavetable code for Ringmod, `FxBus_processSelected`, the mixer voice routing and FX-return loop are also extracted from the pinned source. **REC and SINE** oscillator waveforms were exercised; other waveforms and real drum voice synthesis were not tested.

Each integrated scene is compared to a separate combination of actual effect outputs from cloned pre-invocation state with original processing functions, plus independent saturating return summation. This tests **combined signal routing and inter-effect interference**, not independent correctness of each DSP mathematical kernel. Host simulator timing, memory/cache and SIMD emulation are not a Cortex-M7 runtime proof.

## Final measured host coverage

[Final GitHub Actions run **44318020924**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44318020924): **SUCCESS**, checkout SHA `d6fa49933f58a5ab82cd6df2e643eaceb814116c`.

- **5,832** deterministic six-voice 64-frame scenes; **1,296** scenes exercising all four real DSP effects simultaneously.
- Six destination routings (both stereo and four mono), three Dry/Wet settings, three Distortion types (Tube/Fold/CLP), two Delay algorithms (normal/ping-pong), two real oscillator waveforms (REC/SINE), nine voice routing patterns and three synthetic data patterns.
- **PASS** all integrated-return-versus-isolated-and-summed comparisons, with the actual DSP implementation executed in both branches of the comparison.
- **PASS** GCC AddressSanitizer and UndefinedBehaviorSanitizer for the exercised host paths; no reported memory or UB finding.
- **PASS** actual Delay normal and ping-pong pause/re-entry sequence: 8 active blocks, 12 blocks inactive while Ringmod ran, then reactivated. Delay's state and history buffer remained identical while excluded from the routing mask; reactivation advanced its cursor and produced retained signal. This is an **observed host behavior**, not an approved tail policy or device sound finding.
- Generated native C unit 350,168 bytes, SHA-256 `f8f61950fa25626eb96184c85cc38407ccfeb2afbaea66564c7377be49a01a4f`.
- Actual ARM CPU cycles, WCET, IRQ/DMA deadlines, stack high-water, hardware/audio stability and long-term reliability remain **UNMEASURED**.

### Original evidence retention

- [Final CI artifact **14434185141**](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44318020924/artifacts/14434185141), 118,422 B, SHA-256 `7948c5bd72081c92e1bb127336a7a98d4b049bb1ff90ae40d231074c0eca9a8f`; ZIP CRC independently verified. It contains original generated C, full raw host compile/run logs, source/test identities and per-module digests, with **no IMG**. GitHub expiry **2026-12-18**.
- Owner-held ZIP `LXR_R23_Real_Four_DSP_Integrated_Host_Evidence_2026-11-18.zip`, 478,517 B, SHA-256 `6e793ecde0937480e9704fcf7de7e5419cb7558fd446c7c84c74834313fe862e`. Its English report, JSON integrity manifest and all eight original CI artifact ZIPs were independently SHA-256/CRC-verified; nested ZIP integrity PASS.
- Failure history preserved as **test-generator-only STOP**, not a firmware bug: [44316682331](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44316682331) post-signature comment extraction; [44316815074](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44316815074) int-return helper; [44316908510](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44316908510) wavetable declaration qualifiers; [44317062911](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44317062911) annotated mixer declaration; [44317168968](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44317168968) missing original Delay/OnePole host macros.
- Intermediate [44317277764](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44317277764) PASS four effects; [44317674284](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44317674284) PASS with Delay resume; final [44318020924](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/44318020924) PASS with all six destinations. Do not overwrite these records.

## Design boundary and next decisions

**PASS** means the host-combined four-real-DSP paths behave consistently for the covered signal and parameter combinations. It does **not** mean that the LXR-02 has adequate CPU headroom to run them simultaneously; the owner reported an earlier five-CLP-device experiment, which is contextual observation, not current per-FX CPU measurement.

The current shared `FxBus` still has a global `parameter[4]`, `dryWet` and `audioRouting`. The owner requires **independent per-effect parameter banks** for the eventual combined FX interface and kit persistence. The owner may drop **Compressor** if later real CPU measurements justify a three-FX system, while keeping Delay, Ringmod and Distortion/CLP. Simplifying CLP's fader-dependent feedback in favor of output-volume attenuation is only a **possible, separately authorized sound change**; the accepted R18 sound must remain protected. None of these changes were made.

**Current technical recommendation:** Keep all four effects provisionally. Separately agree on Delay tail policy and per-effect parameter persistence; evaluate CPU deadlines and sound on the device when it returns. No firmware flash or promotion authorization was requested or exercised by this test.
