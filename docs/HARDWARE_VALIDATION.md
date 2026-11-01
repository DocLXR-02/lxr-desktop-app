# LXR Firmware Lab — owner hardware observations

Snapshot: 2026-11-01. These are owner reports from the project conversation, not assistant-performed measurements. Exact observation times were not retained; do not invent per-test timestamps.

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

The latest identified image is R11 source 64263ed25f2e9c7f5d880b493d943bfba29271a8, image SHA-256 f1779db7861d4c9a54aded8121eb5659a445256ee64453635fcfa4fb1593d2b0. R11 is an experimental comparison reference, not a final freeze. R4 remains an older retained reference. Installing official firmware before R4 does not prove a later R11 rollback has been tested.

For the current controlled R11 experiments, FILTER_OFF means all six synthesis-path voice filters (three drum voices, snare, cymbal, shared hi-hat), including directly routed paths. It does not mean disabling the internal CLP color/tone/bass filters. This is a manual test restriction, not automatic firmware gating.

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

These describe the original test behavior, not current R11 onset predictions. See [R4 detailed hardware record](R4_HARDWARE_VALIDATION.md) and the [current state](PROJECT_STATUS.md). Hardware WAV recordings were not supplied; synthetic renders are not owner recordings.
