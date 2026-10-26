# S4a checked-save and shared-file regression tests

Run `python3 tests/save_errors/run.py` with the prepared toolchain. For a checkout
containing the pinned S3 Git object, also run `--verify-baseline`. The default
suite is reproducible without fetching history: baseline_s3.inc is a frozen
copy of the actual S3 writer bodies at cbffa2d756658b6f32d8e6417ea40ec2cda828d5,
with vendor/project license retained. Its committed SHA-256 is checked. The
optional flag reconstructs it from that exact Git object and compares the text.
Common serializer/naming/interpolation helpers have pinned source digests too.

The native suite extracts actual current save/data/ownership/timer functions,
and actual kit/pattern/global menu cases and pattern-button branch bodies.
It compiles them using production headers and ASan/UBSan. The complete production
translation units are separately compiled by the ARM firmware builds. Hardware,
FatFs file operations, RAM-editor callbacks and UI notifications are mocks;
this is not a physical card, LCD, audio or complete menu-state simulation.

Coverage includes 1536 direct byte-for-byte comparisons with the S3 oracle:
eight save variants, all 64 slots, Morph 0/127/255, identical filenames and write
call counts. Every payload position is tested with an error, error plus full
byte count, short successful write, and disk error. Open/close failures, invalid
indices, balanced existing timer hooks, first-error retention, object ownership,
no further payload/close/reuse/directory work after quarantine, success-only UI
state and background retry suppression are checked. A configured-source
preprocessor scan ensures only ownership wrappers call f_open/f_close and raw
writes remain only in the checked helper and the explicitly deferred project
header. Project lifecycle guards are checked before destructive entry work.

A failed write-mode open, any write failure/short write, or failed close latches
the shared-file error until reboot. Read-mode no-file/path fallback stays usable;
serious read-open or close failures also quarantine. There is no production latch
clear, FIL.err reset, automatic retry/reset/remount, or forced file-lock release.
Short writes are mapped to FR_DISK_ERR. The policy deliberately does not close
a partially written file: close can flush further payload/FAT/directory data even
when the short write returned FR_OK and FIL.err is still zero. Normal saves close
exactly once. The retained object/lock and even known write-open errors such as
missing paths require reboot; this conservative restriction is intentional.

The existing Euclidean tests have only extraction/ownership mocks updated for
the new helper dependencies and FatFs open/close ownership contract; no assertions
are removed. Actual S1/S2/S3 behavior remains covered by the established suites.
LeakSanitizer is disabled due to executor tracing; ASan/UBSan remain fatal. Native
execution has a 40-second process deadline. Test-only session resets simulate
reboots and are not firmware recovery APIs.

Complete project-directory replacement remains S4b work. The old destination
emptying, unchecked project-header short write/close, ignored global-save result,
and temp cleanup results are not certified safe by these individual-save tests.
