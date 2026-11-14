# R22 TEST01 evidence — Alternative C

Exact source `ace7ad95081aac6a015f11b87b6c540ce100b344`, base
`861c643632e0a88bdc7d60d9410befb3971489fc`. `source-diff.patch` and
`source-paths.txt` identify all12 changed paths. `unchanged-source-inputs.json`
pins relevant untouched engine/DSP/format/input files. No same-kit application,
modulation-engine, audio-boundary or DSP change is included.

`CI_*` files are unchanged small files from successful run43639972875, attempt1,
job132585174084, artifact14130863884. The original artifact archive is753,462 B,
SHA-256 `3cdd0595699f83affe13619a14f74984e9d862a3c305c0298e0041808457e379`.
It expires2026-12-14T20:55:26Z. The separately supplied original archive contains
ELF/MAP/IMG/one-image ZIP. Owner-managed permanent binary retention is NOT ARCHIVED /
NOT VERIFIED; these small files are retained on the report branch.

## Reproduce feature and regressions

Use a full clone at the exact source (existing fixtures access older commits):

```sh
python3 tests/r22_drum_copy/run.py
python3 tests/r21_voice_copy/run.py
python3 tests/project_storage/run.py
python3 tests/project_transaction/run.py
python3 tests/r20_ui/run.py
python3 tests/r20_ui/integration/probe.py . build/r20-ui-host
python3 tests/bug001/run.py
python3 tests/r18_delay_mono/run.py build/delay
python3 tests/r18_usb_midi/run.py build/usb
python3 tests/r18_compressor_mono/run.py build/compressor
python3 tests/fx_clip_r18/compare.py
```

Exact standalone capture/playback, compiler and ARM CMake commands are in the
source's dedicated `.github/workflows/r22-drum-copy-validation.yml`. The R22 runner
reuses the R21 fixture's optional extra-case entry point. No test harness is cloned.
The production menu field schema independently checks25 semantic base fields in all
nine pairs, and real target tables check306 eligible LFO destination translations.
The case file is retained in the source branch at `tests/r22_drum_copy/cases.c`.

No random seed. Existing source bytes are `(id*7+11)%128`, target bytes
`(id*3+9)%128`, old stored source `(id*5+7)%128`; this preserves the57/123 Morph
fixture. Explicit tests replace LFO amount/targets/retrigger and STEP base values.
Native GCC14.2.0/Python3.12.14, ASan/UBSan, `detect_leaks=0`. HAL/LCD/physical IO,
unrelated panel helpers and parameter application effects are adapters; real
FatFs RAM media, production selector/transfer/dispatch/load bodies and destination
tables run. These checks do not measure physical sound or target timing.

`former-destination.py` is an additional18-case check on the same source using the
same fixture entry point. Set `LXR_SOURCE_ROOT=/path/to/checkout` if needed and run
it beside `former-destination.c`. It checks non-OFF destination LFO override, the
former destination's loaded base value and retained other ownership, with zero
transfer-time dispatch/SD. That check passed locally after CI without source changes.

`stack-entry.py` decodes the verified ARM images' fixed Thumb PUSH.W/SUB-SP entry
sequences: R21=312 B and R22=320 B (+8). Its local paths can be adjusted for the
two retrieved CI archives. This is an entry allocation, excluding caller/callee,
interrupt nesting and hardware high-water. It is not a whole-firmware stack bound.
`verify-delivery.py` independently verifies the retrieved archive/IMG/ELF/ZIP bytes.

## Retained failures and reuse

All implementation host and CI runs passed without a corrective source/build
iteration. The initial R22 preflight stop is retained separately, unchanged:
`reports/r22-same-kit-stop-2026-11-14`, commit
`6ba37b023857a02b63ace0f1faa2fde24b4dd33a`, under
`docs/evidence/r22-drum-cross-voice-copy-preflight/`. Its real live-engine reset and
naive same-kit persistence failures informed the owner's explicit Alternative C.
They were not rerun or relabelled fixed by changing modulation code. R22 rejects
every same-kit transfer and leaves that engine path unchanged.

The inherited R21 pre-guard57->123 failure and corrected guard are retained at
`30ab66c09217330d16e595f149bcab239b9bfad2`,
`docs/evidence/r21-voice-copy-test01/`. Final R21 and R22 Morph cases pass in this CI.
`regression-results.json` records all11 local script commands, exits and host times;
those times are not Cortex-M7 CPU or quota measurements. Raw CI logs are preserved
separately from local logs. `SHA256SUMS.txt` covers all small evidence files.
