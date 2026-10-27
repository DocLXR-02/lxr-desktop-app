# S4-L live storage policy tests

Run with the prepared toolchain on PATH:

```
python3 tests/save_errors/run.py --verify-baseline
python3 tests/live_storage/run.py
```

The first suite retains the S3 byte oracle, 1536 exact-success comparisons and
2544 injected write positions. It extends the actual application writer bodies
with nine recoverable open failures then one success for 13 result classes across
eight variants, no silent object/valid-byte reuse, retained RAM/dirty flags and
paused background retries. Only assertions for intentionally changed logical-open
quarantine/dirty-flag policy are updated; uncertain-write/close tests remain.

The second suite extracts actual menu_showSdError, notification expiry/renderer,
menu_tick, menu_repaintAll and menu_repaint. Native mocks replace HAL time,
normal menu painters and the LCD sender. Tests cover two-second expiry (1999/2000),
wraparound, busy display queuing, latest-message replacement, every FatFs result,
page state retention and no storage-latch clear on dismissal. Static checks reject
waits, delay, timer/LCD/storage/audio calls in the new notification helpers.
Existing menu_tick/sendDisplayBuffer architecture is not a physical LCD test.

filesystem.c includes the entire production FatFs, generic disk and S1 units via
the existing integrity fixture. A RAM-only FAT16 volume verifies nine repetitions
of actual missing-path, invalid-name and exclusive-create conflicts make zero
physical mock writes, preserve all RAM-sector bytes and an existing file, and
permit a subsequent successful create/write/close. Read-only-target and full
SFN-root denial are also verified without disk writes. Boot-sector bytes are fixture
initialization, not filesystem formatting or physical media operations. The fixture
has an artificial pre-opened lock; only this test fixture clears it before mounting.
No production file-lock release or hardware recovery is implemented.

Both native executables use fatal ASan/UBSan and 20-second process deadlines.
LeakSanitizer is disabled for executor tracing. Full translation units are compiled
by ARM builds. No card programming latency, audio continuity, physical persistence,
interrupt timing or live hardware behavior is established. S4b overwrite/project
lifecycle and shared-volume recovery remain necessary follow-up work.
