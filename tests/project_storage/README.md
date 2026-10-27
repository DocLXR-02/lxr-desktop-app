# S4b-1 project containment regression tests

Run `python3 tests/project_storage/run.py` from the repository root. A native C
compiler and the existing target headers are required. No card or hardware is used.

`host.c` runs complete extracted production bodies, including project save/load,
readers, checked writers and validation. The filesystem, timer, LCD and sound-send
operations are deterministic mocks. It checks capacity boundaries, all byte-valued
invalid indices and associated kits, nonrecursive temporary cleanup, project
overwrite rejection, successful absent-slot output, failures and legacy reads.
The production initialization kit and unchanged pattern reset/copy bodies are used.

`filesystem.c` uses the whole production FatFs/generic disk/S1 translation units
from the existing RAM-media fixture. It tests the actual project-folder reservation
gate and path helpers on a synthetic FAT16 volume. This separately establishes
zero sector writes on repeated overwrite rejection. Other application tests use
mocked file APIs; neither fixture is a physical SD or audio test.

Both groups run with ASan/UBSan. Existing S1–S4-L2 suites remain required. Existing
Euclidean tests cover V1/V2/V3 patterns and the unchanged successful byte layout.

Temporary restriction: full-project saves require an absent directory. An existing
empty, partial or complete path returns `FR_EXIST`. No replacement, automatic
rollback, card repair or forced file-handle recovery is implemented.
