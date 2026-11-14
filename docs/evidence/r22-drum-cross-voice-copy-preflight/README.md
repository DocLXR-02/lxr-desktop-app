# R22 same-kit preflight evidence

Firmware base and unchanged head: `861c643632e0a88bdc7d60d9410befb3971489fc`.
This bundle reproduces the explicit R22 stop-gate conditions. It is not an R22
feature test pass, firmware patch, or test candidate. No random inputs are used.

## Reproduce

Use a full checkout of the pinned R21 source. Copy this evidence directory outside
that checkout, then run from the evidence directory:

```sh
LXR_SOURCE_ROOT=/path/to/lxr-desktop-app python3 run.py
LXR_SOURCE_ROOT=/path/to/lxr-desktop-app python3 persistence.py
```

Both scripts reject a different source HEAD. Their compiler/run arguments and exit
codes are in `results.json` / `persistence-results.json`. Execution exits0 when the
expected containment/persistence failures and negative controls are reproduced.
`probe.log` records the violating state changes; this is not a successful R22
implementation. Generated `.inc` files, stub adapter translation unit and native
executables are reconstructed beside the script, not downloaded dependencies.
The native binaries are not archived because the pinned sources reproduce them.

Environment: GCC14.2.0 (Debian), Python3.12.14, ASan/UBSan,
`ASAN_OPTIONS=detect_leaks=0`. These checks do not run ARM instructions or hardware.
`source-inputs.json` identifies the unchanged production/fixture inputs.
`SHA256SUMS.txt` covers this evidence bundle.

## Production path and adapters

`probe.c` compiles the full actual `MidiParser.c`, `ParameterArray.c` and
`modulationNode.c`, with real engine structures and `parameterArray_init` pointer
mapping. Two wrappers count dispatches/reset attempts and immediately invoke the
original bodies. `Cc2Text.c` supplies the actual decoded destination tables. The
Cortex LFO destination case and shared `FxBus_setParameter` / `FxBus_updateParameter`
bodies are extracted verbatim by the runner. The four selected distortion leaf
setters are observed adapters; other FX leaves and unrelated parser dependencies
trap if unexpectedly executed. No effect renderer, audio DMA, clock IRQ or physical
sound is modeled. The wrappers rename symbols for observation; they do not replace
parameter decoding, reset behavior, voice volume writes or shared FX value writes.

The saved source D1 own LFO has amount64, target voice1 and the real VOL1 table
index; its decoded destination maps semantically to VOL2 on D2. Unrelated D3 and
Snare LFOs target D3 volume and shared FX P1. Their saved target indices/voice fields
are populated from the actual table. Engine amounts are64/127. Base values are
D1 VOL32, D3 VOL16 and FX P1=25; LFO sample1 establishes the modulated state.
An internal D2 base-volume update to80 is the negative control and leaves all three
outside values unchanged. Installing D2's mapped own-LFO destination then performs
12 reset attempts,3 actual parser dispatches and4 distortion leaf-setter calls.
Active saved parameter bytes remain unchanged during that assignment.

The mixer normally resets targets and reapplies modulation at each block. The
probe confirms cached-value modulation replay restores the observed fields; it does
not establish an audible artifact, its duration or actual IRQ/DMA timing. It proves
that the existing assignment operation is not isolated to the intended destination.

`persistence.py` reuses the actual project/FatFs RAM-media fixture and production
serializers on FAT16. A deliberately naive proposed stored-only same-kit patch is
lost after normal SAVE+PROJECT/reload: target123 becomes unsaved active target77.
The active+stored control preserves123. This is not an existing R21 same-kit gesture;
R21 correctly rejects same-kit transfers. HAL/LCD/engine sends are storage-fixture
adapters, and real media sectors are RAM, with no SD hardware access.

`adapter-link-failure.*` retain the initial missing SVF/mute link adapters, corrected
with trapping adapters before the decisive successful execution. No production
change was made to repair a harness link failure.

## Inherited evidence

`inherited-r21-validation.json` is reused R21 evidence, not a new R22 measurement.
R21 CI43621319613, attempt1, job132527438779, artifact14123628907 was verified
successful at the exact base. See the immutable prior build report at
`30ab66c09217330d16e595f149bcab239b9bfad2` for hashes and tests. Its binary artifact
expires2026-12-14; no new durable binary archive was made by this preflight.
