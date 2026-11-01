# R12 primary evidence index

Source: ff47d945d283fd3def4af3aff974bdcafb69ffd3. CI41355689765/job124995110571,
attempt1, success. Report: [six-voice mono candidate](../../codex-berichte/2026-11-01_six-voice-mono-clp-candidate.md).
This directory contains compact actual primary evidence, not just summaries or
hashes of unavailable local files. SHA256SUMS identifies every other member.
No private KnowledgeBase or hardware data is included.

- run-manifest.json: exact source/tree, CI/artifact identities, retention boundary.
- download-verification.json and verify_downloads.py: independently checked downloaded
  image/ZIP, all444 Git blobs, archive membership,52 manifest members,126 vectors,
  runtime-SP relocation, ELF/image consistency, memory extents and symbol allocations.
- r12-checks-results.json.gz: all1476 final CI main-case inputs/commands, exits,
  output/source/state/return hashes, raw metrics and guards/check counters.
- r12-checks-fallbacks.json.gz:192 exact output comparisons with pinned R11 outside CLP.
- r12-checks-filters.json: four selected moderate-filter cases, not a filter fix.
- r12-checks-known-failures.json: full stderr/inputs for six unchanged legacy snare
  float-to-int16 expected failures, distinctly not passing candidate-domain cases.
- compile-{candidate,baseline,optimized}-{command.json,compile.log}: actual native
  source list, flags, compiler arguments and compile diagnostics for reproducibility.
- r12-regressions-results.json: actual unchanged numeric CLP unit/state test results.
- component.log.gz: actual six-route/mapping/saturation/partition/lifecycle assertions.
- r12-renders-results.json:20 paired synthetic render results, per-channel comparisons.
  The corresponding40 four-channel WAVs and individual render commands are in the
  separate expiring source/build artifact, not claimed permanently mirrored here.
- stack-reports.json and stack.log: eight actual compile-only .su reports with
  original-object executable-section identity checks; no additional firmware link.
- compiler-comparison.json: all122 R11/R12 production compile commands match after
  normalizing only the compiler executable path.
- symbols.json: target ELF endpoint sizes; code bytes are not CPU timings.
- build/configure/compiler/toolchain/regression logs: exact successful CI outputs. Some exact raw logs have trailing blank lines and are gzip-compressed
  without altering their uncompressed bytes.
- ci-job.log.gz: decoded GitHub job log, already masked by GitHub; not model reasoning.
- local-* logs (some gzip-compressed): earlier successful local aggregate runs. Matrix expansion1356→1404→1476
  adds focused coverage. Their repetitive successful per-case files were superseded
  locally; only final primary CI per-case data is retained. No unexpected failure
  was dropped or silently converted into a pass.

Reproduce from the candidate source with native GCC/Python3.12+/NumPy2.3.5:

    python3 tests/fx_clip_r12/scope.py
    python3 tests/fx_clip_r12/component.py
    python3 tests/fx_clip_r12/run.py
    python3 tests/fx_clip_r12/regressions.py
    python3 tests/fx_clip_r12/renders.py

Then the unchanged regression commands listed in the report. Tests build the
pinned R11 reference from Git themselves. Seed1 and hardware/control substitutions
are in the committed fixture/build driver; native CLZ(0) uses the already disclosed
ARM-compatible result32, and half-float history uses _Float16. No sanitizer waiver
for newly modified arithmetic. LeakSanitizer is not exercised. Unsupported sample
playback aborts, not an invented audio source. No full SD/morph/IRQ/device proof.

To verify downloaded build evidence without compiling firmware:

    python3 -m pip install pyelftools==0.32
    python3 verify_downloads.py --repo /path/to/repo --evidence /path/to/extracted/evidence --image /path/to/extracted/candidate --output verification.json

Source/build artifact13103142005 also contains SOURCE.zip,ELF,map,compile_commands,
disassembly,toolchain identity, full host results/WAV archive and .su objects.
Candidate artifact13103162036 contains the IMG and owner ZIP. Both expire
2027-01-30T22:09:35Z under current metadata. Those full binaries/WAVs are NOT ARCHIVED
permanently in Git; do not infer a Library upload or later availability. The compact
records and reproducing source/tests here remain durable at this report commit.

No firmware installation, flashing, target performance measurement or independent
external review was performed by this evidence verification. All device capacity
gates in the main report remain UNMEASURED.
