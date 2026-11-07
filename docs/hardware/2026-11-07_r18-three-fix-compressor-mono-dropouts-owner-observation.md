# R18 Three-Fix TEST Firmware — Compressor Mono Output Dropouts

**Recorded:** 2026-11-07 (Europe/Vienna). Actual device test time was not supplied.  
**Evidence type:** Owner's direct listening observation on the physical LXR-02; not an assistant-performed test or measured waveform.  
**Disposition:** **FAIL / OPEN — audible dropouts reported on both tested Compressor mono output modes. STOP further high-gain Compressor stress pending controlled diagnosis.**

## Firmware and known prior test evidence

- Owner confirmed successful installation of `LXR_V2_TEST.img`, the three-fix validation candidate based on source SHA **`cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6`**.
- Previously verified distributed test IMG: 288,184 bytes, SHA-256 `50b2638aa8928e1bcaea79d1101a46bb0e71c7e5a203f0ebdde9a4fd1ba56dc7`. There was **no independent readback hash from the actual MCU**.
- Owner previously reported CLP sound/routing with all voices and filter ON functioning, and Delay `Del`/`Pp` functioning with six simultaneous voices in `St1`, `L1`, `L2` output modes.
- The Compressor one-line saturation correction passed focused source-extracted host tests and isolated ARM build; all three integrated fixes passed host regression tests and combined ARM compilation. **These are not physical mono-audio validation**.
- Original accepted R18 sound source SHA `41b364067bce0d8cde624c184933d9cb88e2725b` remains protected.

## New owner-reported Compressor behavior

The owner stated, in German: “stereo funktioniert. mono L1, L2 hat aussetzer.”

| FX output routing | Reported behavior | Classification |
|---|---|---|
| `St1` stereo | Works | **PASS** for the heard test |
| `L1` mono | Dropouts | **FAIL / OPEN** |
| `L2` mono | Dropouts | **FAIL / OPEN** |

**Not supplied:** Exact Compressor `rat`, `gai`, threshold, attack/release, dry/wet values; voice count or voice configuration in this specific Compressor test; whether dropouts occur at low gain or only with larger makeup; occurrence frequency/duration, relation to sequencer step timing, actual signal recording, comparison to original R18 Compressor mono, and whether physical channel cables or the same monitoring chain were controlled. Therefore neither a narrow numerical overflow cause nor regression introduced by the single-line fix is established.

## Read-only source review; hypotheses, NOT root-cause evidence

At exact source SHA `cdc294cbb89d92d6b7d2dc466f2b65f1677e16d6`:

1. `Core/Src/Soundengine/FxBus.c` chooses `channels=1` for the mono FX routing enums, selects an output halfword pointer for each physical left/right destination, and dispatches to `Compressor_process` when Compressor is selected.
2. `Core/Src/Soundengine/DSPAudio/FX/Compressor.c` has separate stereo and mono paths. In mono it reads even interleaved input samples (`fxBuffer[i*2]`), converts the gain-adjusted sample with the owner-approved `__SSAT(...,16)`, applies dry/wet, accumulates to the destination halfword, then increments the destination pointer by two halfwords. The preceding R18 source had an unchecked float-to-int16 conversion instead. The code is read-only reviewed here, not modified.
3. These source features establish a real branch/routing distinction; **they do not identify the defect**. The owner has not yet provided settings sufficient to distinguish compressor envelope/gain behavior from routing, buffer, DSP deadlines or a pre-existing issue.

No production code changes, rebuilds, hardware actions or reflashes were authorized or executed in connection with this finding.

## Safety, diagnostic next step, and release gate

**Stop further high-gain Compressor stress** while audible gaps remain unexplained; reduce monitoring gain and mute on unexpected loud or persistent output. Preserve a copy of the current kit/project and owner observations; do not write over the accepted R18 reference or replace the test image. No automatic rollback, bootloader action, source merge or R19 release is justified by this observation alone.

**Single first follow-up question:** Did the gaps already appear with low/moderate Compressor makeup gain (`gai`) and ordinary dry/wet, or only after `gai` was increased? Record actual tested settings if readily available; avoid escalating gain merely to reproduce an unsafe output. This discriminates whether to investigate only the over-range saturation case or the ordinary mono processing/routing path.

**Release status:** Three-fix TEST candidate is **NOT ACCEPTED** for R19. CLP and Delay are owner-reported positive; USB-MIDI hardware testing is **NOT RUN / BLOCKED** for lack of equipment; Compressor mono is **FAIL / OPEN**. CPU/WCET, IRQ/DMA deadlines, actual stack high-water and long-term reliability remain **UNMEASURED**.

**Record placement:** Separate owner hardware observation on the existing test-plan documentation branch, not the firmware source branch or the canonical status branch. Do not overwrite the accepted R18 sonic closure.
