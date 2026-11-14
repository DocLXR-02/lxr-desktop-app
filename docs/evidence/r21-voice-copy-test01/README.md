# R21 TEST01 primary evidence

Source: `861c643632e0a88bdc7d60d9410befb3971489fc`, based on exact R20
`b01537282047bda5299cdb1bedeab5e5d5b9bcca`. The source branch has two commits;
`0fe5c8a730918d7ebce0a104abc22403ae59c749` is the retained pre-guard draft.

`CI_*` files are unmodified small files extracted from the verified original
GitHub Actions artifact, run 43621319613, attempt 1, job 132527438779, artifact
14123628907. The complete original archive SHA-256 is
`ff0d61158dc2e89e25f85db92342dc09398b919135456a4b8197d8671ab65963`.
It contains ELF/MAP/IMG/ZIP and expires on 2026-12-14. Binary evidence is provided
separately for owner download, not committed to this report branch. An ongoing,
owner-managed durable binary archive is NOT VERIFIED. `SHA256SUMS.txt` covers
these small retained evidence files; `CI_validation.json` covers the binaries.

The source fixtures are in `tests/r21_voice_copy` on the firmware source branch.
Use a full clone, since existing regressions reference immutable older commits:

```sh
git checkout 861c643632e0a88bdc7d60d9410befb3971489fc
python3 tests/r21_voice_copy/run.py
python3 tests/r21_voice_copy/run.py --morph-alias
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

The pinned CI workflow supplies exact capture/playback compile commands and the
ARM GNU14.3.Rel1 download hash, CMake Release commands, image validation and
artifact retention. No random generator is used. R21 fixture saved bytes are
source `(id*7+11)%128`, target `(id*3+9)%128`, original stored source
`(id*5+7)%128`; all track lengths begin at16. This gives the decisive Morph
parameter values57 and123. Real production FatFs runs on RAM FAT16/FAT32 media.
HAL, LCD, unrelated panel helpers and DSP application calls are explicit host
adapters. Full audio render, physical contacts and target timing are not measured.

## Retained expected failures and fixture corrections

- `reproduction.log` / `results.json`: exact R20 baseline project-save identity
  defect, expected wrong destination and lost source edit after project reload.
  The reproducer exits0 when it confirms that defect. Reproduce using a separate
  checkout of R20 and `LXR_SOURCE_ROOT=/path/to/R20 python3 reproduce-r20-identity.py`.
  Its outputs are written beside the script. Do not run against the R21 source.
- `morph-alias-failure.log` and `morph-alias-head-failure.log`: pre-guard Morph
  assertion failure, previous57 versus dispatched123. The public pre-guard draft
  above has the same source tree as the original local `f892ae0` commit mentioned
  during diagnosis; its commit metadata was corrected before initial publication
  to respect GitHub email privacy. Run the draft with `--morph-alias` to reproduce.
- `compile-failure.*`, `r21-first-compile.log`, `r21-compile2.log` and
  `r21-engine.log`: fixture construction failures (duplicate extracted function,
  missing/duplicate adapters and prototype), followed by passing final fixtures.
- `r21-final-host.log`: the first legacy fixture filename exceeded8.3; the fixture
  was corrected to `02-LEGCY.snd`. It was not a firmware fault.
- `r20-ui-label-failure.log`: old test asserted the R20 boot label; updated to the
  required R21 label, with passing rerun. Notification fixture adapters represent
  the R20 case where no R21 selector is open; the R21 suite exercises the selector.
- `CI_r21-host.log`: final Variant A, all standard R21 checks plus all six Morph
  cases and successful retry after actual zero-amount sweep completion.

`initial-regression-results.json` and `regression-results.json` retain local
commands and exit codes before the final Morph guard; the final source was tested
again by CI. `verify-delivery.py` independently checked retrieved artifact bytes
and writes `delivery-verification.json`. Its two local input paths can be changed
for another downloaded copy. `source-diff.patch` and `source-paths.txt` pin scope.
