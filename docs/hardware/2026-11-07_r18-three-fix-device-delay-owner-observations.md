# R18 Three-Fix Candidate — Owner Hardware Delay Test

**Recorded date:** 2026-11-07 (Europe/Vienna); actual device test time not supplied.  
**Evidence class:** Firsthand owner report of physical LXR-02 listening/operation, not instrumented measurement or a test executed by ChatGPT.  
**Status:** **PASS — owner-reported, within the configurations named below.** Remaining USB-MIDI and Compressor device checks are not covered by this observation.

## Candidate and prior observation

- Firmware reported installed successfully by the owner: `LXR_V2_TEST.img`.
- Exact integrated source commit: `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6`.
- Previously verified distributed image: 288,184 bytes; SHA-256 `50b2638aa8928e1bcaea79d1101a46bb0e71c7e5a203f0ebdde9a4fd1ba56dc7`. No physical-device post-flash byte readback has been reported.
- Accepted original R18 source remains `41b364067bce0d8cde624c184933d9cb88e2725b`.
- [Earlier successful installation, FX-CLP sound, multi-voice routing and filter ON owner observations](2026-11-07_r18-three-fix-device-clp-initial-owner-observations.md).
- [Full device test and recovery plan](../plans/2026-11-05_r18-three-fix-device-test-backup-recovery-plan.md).

## Owner-reported Delay findings

The owner reported no anomalies in the following tested device settings:

| Dimension | Actual owner report | Classification |
|---|---|---|
| Delay type | `Delay/typ Del` | **PASS — no audible anomaly reported** |
| Delay type | `Delay/typ Pp` | **PASS — no audible anomaly reported** |
| Concurrent voices | **Six voices playing simultaneously** | **PASS — sound described as completely normal** |
| Output module | `FX/out St1` | **PASS — no audible anomaly reported** |
| Output module | `FX/out L1` | **PASS — no audible anomaly reported** |
| Output module | `FX/out L2` | **PASS — no audible anomaly reported** |

The owner described all six simultaneous voices as sounding completely normal in the stated output modes. This is strong evidence that the integrated build has not introduced a readily audible Delay routing regression in those exercised settings. The report does **not** enumerate a formal Cartesian cross-check of every type/output combination, precise voice identities, sample paths, feedback percentages, dry/wet settings, duration, signal recording, electrical measurements or abnormal power states. Do not silently infer those details.

## Technical scope and release gate

The Delay candidate's previously fixed source-level defect was misaligned mono-right 32-bit access; corrected tests and a standalone ARM validation had passed earlier. This device observation supports normal operation in the tested configurations but **does not, by itself, establish a measured alignment guarantee, full-channel coverage, DMA/IRQ behavior or CPU/WCET margin**.

**Not yet reported as hardware passes:** Actual USB-MIDI computer-host traffic, bursts, queue congestion, Note-On/Note-Off correctness and reconnect behavior; Compressor mono make-up gain saturation and dry/wet listening; long-term reliability or prolonged audio/load stress. They remain separate gates.

**Suggested next targeted test:** USB-MIDI with an actual computer/DAW or MIDI monitoring host connected to the **USB** port (not solely DIN MIDI). Begin with low audio level and ordinary note/clock reception, then consider bounded bursts and reconnect checks if the device behaves normally. Stop the test for stuck notes, unexpected sustained sound, excessive level, device freezing or boot/restart anomalies.

**Still UNMEASURED:** CPU/WCET, IRQ/DMA deadlines, stack high-water and sustained stability. No R19 release is created or authorized by this record. The TEST firmware continues to carry the legacy built-in R18 startup label; original owner-accepted R18 remains frozen.

**Repository action:** Documentation only. No build, code modification, merge, additional hardware action, or release was performed by the assistant.
