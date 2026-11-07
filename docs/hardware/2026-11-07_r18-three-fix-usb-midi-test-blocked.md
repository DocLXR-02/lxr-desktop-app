# R18 Three-Fix Candidate — USB-MIDI Device Test Blocker

**Recorded date:** 2026-11-07 (Europe/Vienna; report time not independently recorded).  
**Evidence class:** Owner-reported test capability, NOT a hardware test result.  
**Hardware USB-MIDI validation status:** **NOT RUN / BLOCKED — test equipment unavailable**.

## Current evidence

The owner explicitly reported having no means to perform a MIDI test. This does **not** mean that USB-MIDI failed. There was no observed physical USB-MIDI result, successful or unsuccessful, at the LXR-02.

The integrated candidate remains source commit `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6`, tested as `LXR_V2_TEST.img` (distributed image SHA-256 `50b2638aa8928e1bcaea79d1101a46bb0e71c7e5a203f0ebdde9a4fd1ba56dc7`). The previously reported successful device installation, unchanged CLP sound/routing and Delay `Del`/`Pp` operation in `St1`, `L1`, and `L2` output modes remain distinct, owner-observed evidence.

- USB-MIDI source-extracted functional host tests: **PASS**, previously recorded in isolated and integrated CI.
- Matched ARM build and static artifact validation: **PASS**. Integration CI [42069839191](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42069839191).
- Actual physical USB-MIDI Note-On/Off, MIDI clock, bursts, TX BUSY/in-flight behavior, RX full condition, disconnect/reconnect and MCU ISR/CDC interaction: **NOT RUN / BLOCKED**.
- Hardware timing and long-term reliability: **UNMEASURED**.

## Release gate implications

Do not classify USB-MIDI as a physical-device PASS or assume an unexercised USB fix is reliable merely because the integration build passes. Preserve an explicit residual-risk decision before any future R19 approval. Options at that separate decision point include (a) retaining the integrated USB fix with an explicit untested-device limitation accepted by the owner, (b) later arranging a USB-MIDI hardware test, or (c) removing USB changes and doing a new independently reviewed, tested and built candidate. None of these decisions is implied by the present message.

The owner can continue to test unrelated compressor functionality without MIDI test equipment. The next planned physical test is the Compressor, using low monitored volume and ordinary musical material, escalating carefully to mono high-makeup/saturation conditions while watching for excessive output.

**No code change, build, source merge, release, firmware installation, device operation or additional hardware test was performed by ChatGPT in this record.**
