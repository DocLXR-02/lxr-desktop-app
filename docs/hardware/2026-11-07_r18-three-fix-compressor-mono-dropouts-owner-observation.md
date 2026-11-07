# R18 Three-Fix TEST Firmware — Compressor Output Failure (Corrected Owner Observation)

**Recorded:** 2026-11-07 (Europe/Vienna); exact device-test time not supplied.  
**Last revision:** 2026-11-07, incorporating the owner's confirmation that St1 matches L1/R1 and St2 matches L2/R2.  
**Evidence type:** Owner's firsthand listening observations on the LXR-02, **not** oscilloscope/recorded audio, a device memory readback or assistant-performed test.  
**Disposition:** **FAIL / OPEN — all six FX/OUT modes are owner-reported faulty, grouped by output pair rather than stereo versus mono: St1/L1/R1 exhibit intermittent dropouts approximately every four bars, while St2/L2/R2 are nearly silent with occasional very attenuated muffled tones. GAI 0–127 was reported for St1/L1/R1; a separate gain-range confirmation was not supplied for St2/L2/R2.** Stop high-gain Compressor stress pending diagnosis.

## Correction history and superseded description

**Original owner message (superseded):** “stereo funktioniert. mono L1, L2 hat aussetzer.” This was formerly recorded as St1 PASS, L1 FAIL and L2 FAIL.

**Explicit later owner correction (authoritative for current status):** “revidiere die beobachtung. St1, L1, R1 funktionieren alle nicht bei Gain 0-127.”

The earlier **St1 PASS is withdrawn**. The owner's later correction established a malfunction in St1, L1 and R1 throughout the tested GAI 0–127 range, but did not yet describe the sound. The owner next added: **“L2, R2 funktioniert auch nicht.”** The latest owner message, **“R1, L1 haben aussetzer ca jeden 4 takt. bei L2, R2 fast komplette stille. ab und zu nur sehr unterdrückte dumpfe töne.”**, supplies different symptoms for the first and second pairs: L1/R1 have dropouts approximately every four bars, while L2/R2 are almost silent and occasionally produce very subdued dull/muffled tones. The owner then clarified the complete grouping: **“symptomatik ist in den beiden Gruppen gleich: St1=L1=R1; St2=L2=R2”**. This extends the already described intermittent-dropout symptom to St1 and the nearly silent/muffled symptom to St2, making **all six tested FX output modes FAIL** by owner report. The earlier St1-good and St2-untested classifications are expressly superseded.

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
| `St1` stereo | 0–127 | Intermittent dropouts approximately every four bars (same as L1/R1, owner-confirmed group) | **FAIL / OPEN** |
| `L1` mono | 0–127 | Intermittent dropouts, approximately every four bars (owner estimate) | **FAIL / OPEN** |
| `R1` mono | 0–127 | Intermittent dropouts, approximately every four bars (owner estimate) | **FAIL / OPEN** |
| `L2` mono | **Not specified** | Near-complete silence; occasional severely attenuated, dull/muffled tones | **FAIL / OPEN** |
| `R2` mono | **Not specified** | Near-complete silence; occasional severely attenuated, dull/muffled tones | **FAIL / OPEN** |
| `St2` stereo | **Not specified** | Near-complete silence; occasional severely attenuated, dull/muffled tones (same as L2/R2, owner-confirmed group) | **FAIL / OPEN** |

**Evidence limits:** The St1/L1/R1 dropout timing is an approximate owner estimate of roughly four bars, **not a recorded or independently timed periodicity**; the duration, exact onset/offset and dependence on BPM/sequencer state remain unknown. Attack/release, ratio, threshold, dry/wet, voice count/routing, cables, monitoring configuration, other FX parameter settings and physical comparison with original R18 Compressor remain unspecified. The entire GAI 0–127 range is **owner-reported** only for St1, L1 and R1; the second group (St2/L2/R2) is confirmed faulty but has no separately stated GAI interval. No instrumentation-based gain sweep is claimed.

## Read-only source implications — no root cause yet

At integrated source SHA `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6`, `Core/Src/Soundengine/FxBus.c` selects the Compressor from the FX bus and dispatches either stereo or mono based on FX/OUT. `Core/Src/Soundengine/DSPAudio/FX/Compressor.c` has separate stereo and mono loops; the isolated correction replaced **only the mono gain-to-int16 conversion** with `__SSAT(...,16)`, leaving stereo Compressor production code unchanged.

The owner-confirmed fact that both **St1 and St2 stereo modes fail** means the observed problem cannot reasonably be attributed solely to the corrected mono conversion instruction. The matching symptoms within each stereo-and-mono output-pair group suggest an output-pair-dependent interaction, but the shared Compressor DSP, FX routing/parameter selection, source signal and other inherited behavior remain alternative explanations; **no root cause is demonstrated**. The fact that delay and CLP work does not prove the Compressor path is correct.

The original host fixtures specifically exercised bounded source-extracted functions and have not covered the reported actual full-device failure. The ARM image validated file/section/segment integrity; it did not prove hardware audio correctness, DSP CPU/WCET or DMA timing.

## Safety, next diagnostic question and release hold

**Stop high-gain Compressor tests.** Mute or reduce monitoring if the output becomes unexpectedly loud or erratic. Preserve the current test kit and the SD backup; do not flash a speculative fix or overwrite accepted R18. No code changes, builds, reflashes, main merges or firmware releases have been authorized by this observation correction.

**Next step:** No further high-gain hardware reproduction is required to establish the symptom grouping. Perform a separately authorized, read-only review of the exact source/routing paths and historical original R18 behavior before proposing changes. If an existing safe low-volume comparison to the original R18 Compressor is available, record it as a separate observation rather than inferring regression. The approximately four-bar dropout report is not yet evidence of a particular buffer, timer or CPU fault.

**Release status:** R19 is **BLOCKED**, because the integrated test firmware's Compressor hardware behavior is unresolved; no final sonic approval has been given. CLP and Delay remain owner-observed successes; USB-MIDI remains hardware-untested; CPU/WCET, IRQ/DMA deadlines, stack high-water and long-term reliability remain **UNMEASURED**.

**Recordkeeping:** This file replaces the prior interpretation as the **current authoritative owner-observation report** at the same path. Git commit history retains the earlier wording and this explicit correction. The accepted R18 branch, integrated firmware code and canonical documentation branch are unchanged.
