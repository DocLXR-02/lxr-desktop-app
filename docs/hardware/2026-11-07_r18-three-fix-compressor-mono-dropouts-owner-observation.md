# R18 Three-Fix TEST Firmware — Compressor Output Failure (Corrected Owner Observation)

**Recorded:** 2026-11-07 (Europe/Vienna); exact device-test time not supplied.  
**Last revision:** 2026-11-07, incorporating the owner's subsequent L2 and R2 failure report.  
**Evidence type:** Owner's firsthand listening observations on the LXR-02, **not** oscilloscope/recorded audio, a device memory readback or assistant-performed test.  
**Disposition:** **FAIL / OPEN — the Compressor does not function in the owner's tested St1, L1, R1, L2 and R2 FX output modes. GAI 0–127 is confirmed for St1/L1/R1 only; the gain range for L2/R2 is not specified. This is NOT a mono-only fault.** Stop high-gain Compressor stress until the failure is characterized.

## Correction history and superseded description

**Original owner message (superseded):** “stereo funktioniert. mono L1, L2 hat aussetzer.” This was formerly recorded as St1 PASS, L1 FAIL and L2 FAIL.

**Explicit later owner correction (authoritative for current status):** “revidiere die beobachtung. St1, L1, R1 funktionieren alle nicht bei Gain 0-127.”

The earlier **St1 PASS is withdrawn**. The earlier L1 description as mere “dropouts” is not carried forward as a proven symptom; the corrected wording is simply **“does not function”**, whose precise acoustic meaning (silence, intermittent dropouts, distorted sound or other) has **not** yet been specified. The earlier L2 'dropout' description was superseded as an exact symptom description. The owner subsequently added: **“L2, R2 funktioniert auch nicht.”** Thus both L2 and R2 are now directly reported as nonfunctional, but their specific GAI range and the exact acoustic symptom were not supplied. Neither the most recent message nor the preceding correction reports testing **St2 stereo**.

Retaining the prior quotation here documents why earlier Git revisions and the conversation differ; the table below is the current authoritative classification.

## Candidate identity and other established results

- Test firmware successfully flashed by the owner: `LXR_V2_TEST.img`.
- Combined Delay/USB-MIDI/Compressor source commit: `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6`.
- Verified distributed IMG: 288,184 bytes; SHA-256 `50b2638aa8928e1bcaea79d1101a46bb0e71c7e5a203f0ebdde9a4fd1ba56dc7`. An independently validated post-flash MCU image readback has not been reported.
- CLP musical character, multi-voice FX routing and filter ON were reported satisfactory after the installation.
- Delay `Del`/`Pp` were reported normal with six simultaneous voices in `St1`, `L1`, `L2` output modes. **These successes concern Delay/CLP, not the Compressor.**
- Compressor saturation correction passed source-extracted host reproduction/comparison suites and the isolated and integrated ARM builds, but **no complete physical-device Compressor PASS** has been established.
- USB-MIDI actual device test remains NOT RUN/BLOCKED for lack of suitable equipment.
- Original musical R18 source `41b364067bce0d8cde624c184933d9cb88e2725b` remains untouched.

## Corrected Compressor hardware observation

| FX/OUT setting | Owner-reported `GAI` test interval | Latest result | Classification |
|---|---|---|---|
| `St1` stereo | 0–127 | Does not function | **FAIL / OPEN** |
| `L1` mono | 0–127 | Does not function | **FAIL / OPEN** |
| `R1` mono | 0–127 | Does not function | **FAIL / OPEN** |
| `L2` mono | **Not specified** | Does not function (latest owner report) | **FAIL / OPEN** |
| `R2` mono | **Not specified** | Does not function (latest owner report) | **FAIL / OPEN** |
| `St2` stereo | **Not reported** | No observation supplied | **NOT TESTED / UNKNOWN** |

**Evidence limits:** The precise nature of “does not function,” frequency/duration of any dropouts, attack/release, ratio, threshold, dry/wet, voice count/routing, cables, monitoring configuration, other FX parameter settings and physical comparison with original R18 Compressor remain unspecified. The entire GAI 0–127 range is **owner-reported** only for St1, L1 and R1; L2 and R2 fail by owner report but have no separately reported GAI interval. St2 has not been reported. No instrumentation-based gain sweep is claimed.

## Read-only source implications — no root cause yet

At integrated source SHA `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6`, `Core/Src/Soundengine/FxBus.c` selects the Compressor from the FX bus and dispatches either stereo or mono based on FX/OUT. `Core/Src/Soundengine/DSPAudio/FX/Compressor.c` has separate stereo and mono loops; the isolated correction replaced **only the mono gain-to-int16 conversion** with `__SSAT(...,16)`, leaving stereo Compressor production code unchanged.

The new reported **St1 failure means the observed problem cannot reasonably be attributed solely to the corrected mono conversion instruction**. It may involve shared effect enablement, parameters, input signal, common DSP/FX routing or another pre-existing or newly exposed interaction; **no cause is demonstrated**. The fact that delay and CLP work does not prove the Compressor path is correct.

The original host fixtures specifically exercised bounded source-extracted functions and have not covered the reported actual full-device failure. The ARM image validated file/section/segment integrity; it did not prove hardware audio correctness, DSP CPU/WCET or DMA timing.

## Safety, next diagnostic question and release hold

**Stop high-gain Compressor tests.** Mute or reduce monitoring if the output becomes unexpectedly loud or erratic. Preserve the current test kit and the SD backup; do not flash a speculative fix or overwrite accepted R18. No code changes, builds, reflashes, main merges or firmware releases have been authorized by this observation correction.

**One minimal clarification before further debugging:** By “does not function” in the affected output modes, does the owner mean complete silence, intermittent audio dropouts, or an unchanged dry signal with no audible compression? Do not increase GAI simply to reproduce an unsafe condition. This distinction is decisive before treating the report as a buffer/timing fault.

**Release status:** R19 is **BLOCKED**, because the integrated test firmware's Compressor hardware behavior is unresolved; no final sonic approval has been given. CLP and Delay remain owner-observed successes; USB-MIDI remains hardware-untested; CPU/WCET, IRQ/DMA deadlines, stack high-water and long-term reliability remain **UNMEASURED**.

**Recordkeeping:** This file replaces the prior interpretation as the **current authoritative owner-observation report** at the same path. Git commit history retains the earlier wording and this explicit correction. The accepted R18 branch, integrated firmware code and canonical documentation branch are unchanged.
