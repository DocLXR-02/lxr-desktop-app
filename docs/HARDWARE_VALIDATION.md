# LXR Firmware Lab — owner hardware observations

Snapshot: 2026-11-03 (Europe/Vienna). These are owner reports from the project conversation, **not assistant-performed measurements**. Historical per-test times were not retained; do not invent per-test timestamps. CLP R18 owner's positive listening acceptance was recorded on 2026-11-03. See [R18 chapter closure](CLP_R18_SONIC_ACCEPTANCE.md).

| Stage | Owner observation | What remains unproven |
|---|---|---|
| R4 | Original morph/Filter Drive symptom absent; shorter Morph travel and load feedback accepted; ordinary saves and repeated reboots successful | General power-loss safety, all older projects and long-run capacity |
| R5 | After an initial checksum rejection, SD and iPhone file hashes matched and installation succeeded. Unwanted idle whistle absent at fader0 and missing steps | Cause of the earlier failed update; a filename change alone was not proven causal |
| R6 | Bass strongly suppresses desired scream, which remains mostly near lowest10–20% of fader travel | Not selected for continuing musical design |
| R7 | Enjoyable playing; bass/sub retained over much of travel | Objective acoustic measurement or all parameter combinations |
| R8 | Whistle can fade, but attenuation begins too strongly/early | No complete linear loudness claim |
| R9 | Revised fader response described as perfect for current goal; parameter fine tuning deferred | Finalization or general long-run validation |
| R10 | Multi-voice additions/removals sound plausible and playable | Every silent-high-route scenario or independent wet mixing |
| R11 | MIX/VOL adjusts level without the unwanted CLP excitation/timbre shift in the agreed solo/FILTER-OFF test | All seven tracks independently tested, multiple-route post-MIX, enabled filters, target timing/stack/thermal bounds |
| R14 | Owner considers six-voice MAX feedback and broad controls playable with fluid high-frequency feedback; problematic bass loss during partial dry/wet | Actual CPU/stack/runtime reserve; general clip/headroom guarantees |
| R15 (HP1 110 Hz) | Kick bass fully retained; feedback changed drastically, becoming lower in pitch and more step-like | Generalization across all kits/settings, hardware resource profiling |
| R16 (HP1 1000 Hz) | More parameter-control latitude, but feedback progression remained partly stepped | Whole parameter grid and physical frequency/phase measurements |
| R17 (HP1 3000 Hz) | Feedback still insufficiently fluid compared with 7200-Hz original while bass already weakened | Complete objective acoustic comparison |
| **R18 (CLP v2.1)** | **Accepted as musically complete on the actual device:** all three listening checks positive — satisfactory kick bass, fluid expressive feedback, and no audible objection to soft output ceiling | **CPU/WCET, DMA/IRQ timing, stack high-water, long-run/thermal/multi-device reliability not measured; no debugging hardware available** |

The **current musically accepted CLP image is R18**, source `41b364067bce0d8cde624c184933d9cb88e2725b`; image SHA-256 `c94e07d7c0daa40b7b17f0bb6271270f7b8b7eabc8eff69ad63f6a68c5eb50f1`. This is a **subjective sound freeze only**, not a public release or a runtime reliability certificate. R14–R17 and R11 remain historical comparison branches; R4 is an earlier retained reference. Installing official firmware before R4 does not establish a tested rollback from R18.

For the historical controlled R11 experiments, FILTER_OFF means all six synthesis-path voice filters (three drum voices, snare, cymbal, shared hi-hat), including directly routed paths. It does not mean disabling the internal CLP color/tone/bass filters. This is a manual test restriction, not automatic firmware gating.

R5 parameter evidence: project PROJ00 named CLP-1. Columns are drv/col/ton/vol. Latest owner corrections supersede earlier lists. D/W stayed40 for these onset tests; sequencer stop/play made no difference. The tuple denotes onset at zero voice fader except the no-tone references.

| Kit | drv | col | ton | vol | Observation |
|---|---:|---:|---:|---:|---|
|0|127|0|0|127|No idle tone|
|1|127|0|127|127|No idle tone|
|2|127|6|0|127|Onset; one COL step lower stops|
|3|127|3|127|127|Onset|
|4|63|6|127|127|Stored tuple COL6; later confirmed onset7, cessation5: hysteresis, not a single hard threshold|
|5|63|21|0|63|Onset|
|6|63|11|127|63|Onset|
|7|63|12|63|63|Onset|
|8|32|81|0|32|Onset|
|9|32|44|63|32|Onset|

These describe the original test behavior, not current R18 onset predictions. See [R4 detailed hardware record](R4_HARDWARE_VALIDATION.md) and the [current state](PROJECT_STATUS.md). Hardware WAV recordings were not supplied; synthetic renders are not owner recordings.
