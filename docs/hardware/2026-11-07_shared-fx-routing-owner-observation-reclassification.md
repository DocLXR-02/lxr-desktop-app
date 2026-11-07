# LXR-02 Shared FX Routing — Owner's Revised Listening Evidence


## Latest device result — original project also works after reloading

**Owner follow-up, 2026-11-07:** After first loading a different project and observing that the clicks vanished, the owner **reloaded the originally affected project**. The clicking **also disappeared in that same original project** after reloading. The owner did not report an edit to, repair of, or file replacement for that project before the successful reload.

**Current evidence-based conclusion:** The clicking was **transient/reload-sensitive**, not a confirmed persistent defect in the original project's stored file contents. Reloading a project may reinitialize runtime state, but the specific mechanism — effects' internal history, sequencer event state, routing state, unsaved temporary settings, or some other interaction — has **not** been measured or proven. The previous suspicion that the original project was permanently misconfigured or corrupted is **not supported by this latest observation**. Both the new project and the reloaded original project are currently owner-reported free of clicking.

**Status:** SYMPTOM NOT REPRODUCIBLE after project reload; CAUSE UNKNOWN. All four effects function without this clicking according to the owner. Preserve the chronology and exact earlier test observations, which remain real reports of the transient condition, not confirmed Compressor-specific regressions. No new source changes, flashing, builds, fault repair, or audio measurements were made. USB-MIDI hardware validation remains NOT RUN/BLOCKED; CPU/WCET, IRQ/DMA deadlines, stack high-water and long-term reliability remain UNMEASURED; R19 release not approved.

## Latest device result — clicking disappears after project change

**Owner report, 2026-11-07:** The owner loaded a **different project** and reported that the clicking disappeared entirely, that **none of the four effects** exhibited clicking, and that **the Compressor works normally** with the firmware they described as the current R18 version. The owner could not establish what had happened to the previous project. No audio captures, original-versus-new project parameter diff, or exact installed-image hash were supplied.

**Current classification:** The earlier shared-FX clicking and Compressor failure reports are **historical symptoms observed with the previous project**, not an active confirmed defect in the currently used setup. The change in project is a strong discriminator for project-/kit-/pattern-dependent behavior, but **does not prove project-file corruption, an accidental parameter change, a specific step-automation issue, or a repaired hardware problem**. Do not claim that the earlier observations were false; record that they ceased in a different test state.

**Limits of the new pass:** The owner confirmed the disappearance of clicks across four FX types and stated the Compressor is healthy, but did not supply a controlled repeat of the previously failing St1/L1/R1 and St2/L2/R2 combinations with all other settings identical. The current source image is described by the owner as the current R18; earlier TEST image shares the boot label `LXR LAB R18`, so exact device image identity is not reconfirmed by that label alone. Do not infer a firmware swap occurred at the time of the project change.

**Practical disposition:** Suspend speculative Compressor/FX-bus code edits, CPU-overrun claims and further risky repeat testing. Keep the original accepted R18 sonic reference frozen and retain the test images, backup and historical failure notes. USB-MIDI device operation remains NOT RUN (equipment unavailable), and CPU/WCET, IRQ/DMA deadlines, stack high-water and long-run reliability remain UNMEASURED. No R19 release authorization follows solely from this recovery observation.
**Recorded date:** 2026-11-07 (Europe/Vienna; exact device test time not supplied).  
**Evidence class:** Owner-reported, approximate listening observations; no captured audio, measured step timing, or independent hardware verification.  
**Status:** **Owner-reported symptom RESOLVED after loading a different project.** Clicking is absent with all four effects in the newly loaded project; no active Compressor failure is reported. The previous project's triggering configuration and underlying mechanism are **UNKNOWN**. No firmware or hardware fault has been established. No code changes or builds are authorized by this report.

## Revised stimulus and owner observations

The owner refined the test setup: a **16-step pattern**, a note trigger at **step 1** with envelope decay long enough to sustain audibly through the 16-step cycle; the owner reports using a comparable FX test for each voice. The owner reports no LFO active and the same note values written across steps. Presence of note values in step data does **not** itself establish active triggers on all those steps.

Reported approximate observations:

- When a voice's `MIX/OUT` is set directly to `St1`, the long tone is heard continuously in the owner's setup.
- When `MIX/OUT` is set to `FX`, the owner hears irregular clicks, apparent interruptions and perceived pitch/note changes. Some clicks subjectively align with steps **3, 9, 10, 11, 12, 13, 14 and 15**. All timing and spatialization are explicitly approximate and uncertain.
- With `FX/OUT = St1`, the owner hears clicks that temporarily mute the otherwise sustained tone, with apparent left/right variation.
- With `FX/OUT = St2`, the tone is nearly silent except around the clicks.
- With `FX/OUT = L1/R1`, the owner hears clicking alternating or varying across the monitored sides and interruptions to the tone.
- With `FX/OUT = L2/R2`, there is near silence with brief sounds around clicks.
- **Crucial new claim:** The owner reports the same general phenomenon with **all four effect types** and **all tested firmware releases, including the original `LXR_V2_V1.7.img`**. The exact version-specific images and controlled test parameter sets have not been independently verified in this record.
- The owner expressly qualifies all listening descriptions as estimates without exact certainty and is concerned that a setup mistake or device damage may have occurred.

## Important interpretation and limits

The new cross-effect, cross-version observation **materially weakens the claim that the integrated mono Compressor one-line saturation correction caused the symptom**. The original Compressor hardware record is still valid as a chronology of what was heard, but **not** proof of a Compressor-only regression or an exclusively mono-path problem.

This evidence raises shared FX-bus summing/routing, multi-voice/track interaction, output jack auto-redirection, monitoring/cabling, or kit/pattern state as differential diagnostic possibilities. The long tone appearing to change pitch may be an amplitude, phase or spectrum artifact; this is not proof that the sequencer played different MIDI notes. Observations do **not** prove the absence of a hardware defect, a manufacturing bug, or an FX routing bug, and do **not** prove a CPU/DMA overload.

**Source/manual grounding:**
- LXR owner's manual pp. 14 and 25–27 describes per-voice `MIX/OUT = FX`, one shared FX processor for routed voices, and separate `FX/OUT` selection.
- Manual p. 34 says unconnected physical output jacks can be redirected to the nearest occupied jack; subjective L/R placement is therefore not conclusive for exact DAC path.
- Manual p. 16 says per-step automation is not processed on inactive steps; active trigger state across all voices was not independently established.
- Source `Core/Src/Soundengine/DSPAudio/mixer.c` in integrated SHA `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` mixes voices routed to the shared `fxBusData`, clears per-block FX/output buffers, and calls `FxBus_process` once per block.
- Source `Core/Src/Soundengine/FxBus.c` in the same SHA selects common FX processing and routes to one of the two DAC output-buffer pairs.
- The integrated candidate differs from accepted R18 only in isolated mono Compressor, Delay and USB-MIDI changes; that distinction alone does not identify why the owner's new cross-version test produced clicks.

## Low-risk next diagnostic

**Do not flash another firmware or create a speculative fix at this point.** Preserve the accepted R18 baseline and the SD backup.

On a copy of the kit/project, at a safe low monitoring level, **mute five voices** (do not delete or reprogram the step data), leaving one known step-1-triggered voice audible. Keep the same 16-step sequence, envelope and cabling. Compare that single voice through direct `MIX/OUT = St1` versus `MIX/OUT = FX` with `FX/OUT = St1`, using **a conservative FX setting and D/W = 0** if available. Record simply whether the clicking/interruption persists; avoid high feedback or high makeup gain. This separates multi-voice input/summing or other tracks from a one-voice FX routing fault, although it is not conclusive alone.

One question before more complex experiments: **Does the click pattern persist with only one routed voice and no other voice playing?** Do not infer a physical fault or attempt hardware repair from the current data.

## Follow-up owner observation: clicks with FX type OFF

**Recorded 2026-11-07:** The owner further clarified that audible clicking also occurs when the selected global FX type is `Off` while the voice is routed using `MIX/OUT = FX`. The owner reports that the setup had previously sounded normal through FX before the recent testing and asks whether a project/kit setting was changed or the hardware was damaged. **The owner has not confirmed a physical damage event.**

Read-only source analysis at `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6` confirms that `FxBus_process` has a dedicated `FX_OFF` passthrough summing branch in `Core/Src/Soundengine/FxBus.c`. This bypasses individual effect processing but still uses the **shared, stereo-interleaved FX summing buffer** assembled by `Core/Src/Soundengine/DSPAudio/mixer.c` and selected output routing. Therefore symptoms with FX type `Off` materially deprioritize a Compressor-specific or other individual FX-algorithm explanation. They do **not** prove that the source signal or output path is error-free.

The manual provides additional grounding:
- Mixer `out` routes each voice to a direct stereo/mono output or `FX` (owner manual p. 14).
- Per-voice track length, active steps and two automation lanes may affect sequencing independently of LFO operation (pp. 15–16); identical stored step note values do not prove every step triggers.
- The FX routing is an insert-style common bus shared by selected voices, with settings stored per Kit (p. 25).
- Cable-presence auto-routing can redirect a voice assigned to an unconnected jack to another connected jack (p. 34).

**Safest narrow discriminating check:** In a copied kit/pattern with low monitoring volume, retain just one audible Step-1-triggered voice, mute other tracks, and compare the identical patch and physical output wiring using `MIX/OUT = St1` versus `MIX/OUT = FX`, `FX/typ = Off`, `FX/OUT = St1`. Do not repeat high-feedback/Compressor gain sweeps. If the clicks disappear in the one-voice FX bypass, investigate other tracks, gain summation and automated states. If they persist, examine the common bypass routing, output cable detection and baseline saved settings. This is a suggested controlled device check; **not yet run or observed**.

**No current evidence establishes physical damage, a regression introduced by the integrated test image, a measured DSP timing overrun, or a single root cause.** Preserve all candidate/reference builds and prior observations without automatically flashing, resetting, replacing kit data, or making source changes.

## Status of previous findings

- CLP musical reference R18 remains owner-accepted and frozen.
- TEST firmware previously installed, CLP listening and Delay tested positively under earlier conditions.
- USB-MIDI actual device testing unavailable: NOT RUN/BLOCKED.
- Compressor and common FX-bus behavior **need controlled re-isolation**; previous Compressor-specific FAIL is a historical observed abnormality, not established as a new TEST-only regression.
- R19 remains **not authorized/not accepted**; no release decision follows from this note.
- CPU/WCET, IRQ/DMA timing margins, stack high-water and long-term robustness remain **UNMEASURED**.

**Recordkeeping:** Documentation-only owner observation. No production edits, agent tasks, builds, reflash, release, or device measurement were performed by the assistant.
