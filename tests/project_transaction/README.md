# Bounded project replacement tests

Run `python3 tests/project_transaction/run.py` from the repository root with a
native C compiler and ASan/UBSan. Optional arguments select `normal`, `faults`,
`boundaries`, `sector` or `edge`. No physical card, USB device, LXR or audio is used.

The fixture compiles complete extracted **production** project transaction,
save/load, path, reader and serializer bodies against the **whole production**
FatFs, generic disk and registration translation units. Extraction is shared with
`tests/project_storage/run.py`; it does not rewrite transaction algorithms. HAL
TIM2 hooks, LCD and sound sends are mocked. A RAM sector driver models fail-closed
I/O; it is not the real S1 driver. Existing suites test the actual S1/S2 path.
Public API fault seams retain the real FatFs functions for non-injected calls.

The two synthetic volumes use FAT16/FAT32, 512-byte sectors, one-sector clusters,
two FAT copies, existing project content and unrelated root entries. Tests cover:

- New saves and twenty replacements per volume, exact 194-file inventory,
  unchanged serializers, known CRC vector and constant available clusters.
- Physical read/write faults, write errors before/after acceptance, API open,
  short-write, close, sync, rename, directory-close and cleanup faults; strict
  stage rejection and safe repeated logical retries.
- Cuts before/after ownership record, file groups, metadata, seal, renames,
  cleanup, and repeatedly interrupted explicit recovery. Reboot reinitializes
  FatFs and volatile application state; recovery selection/load must be read-only
  and retain a complete old/new fingerprint.
- Cuts before/after every sector write of the two cross-sector publication
  renames. Alias entries are detected and held without deletion or further
  project writes. The simulated sector write itself is atomic; torn sectors,
  controller caching and electrical failures are **not** proven safe.
- Full media rejected before transaction creation, retry after space is freed,
  unknown data/reserved collisions, malformed records/indices, failed-recovery
  tmp-overlay guard and first-save interruption/rescue.

A complete staged fingerprint proves the bytes were readable and structurally
valid under this model; it is not a cryptographic signature or an independent
checksum of intended RAM state. The pre-existing S3 oracle in `tests/save_errors`
compares successful serializer output byte-for-byte. No test proves wall-clock
latency, interrupt load, real media durability or glitch-free hardware audio.
