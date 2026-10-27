# S4-L2 silent autosave tests

Run `python3 tests/silent_autosave/run.py` with the prepared ARM toolchain on PATH.
The runner uses the same production-body extraction and fatal ASan/UBSan fixture as
`tests/save_errors/run.py --autosave-regressions`. It runs all existing checked-save
cases before the new silent-autosave cases. There is no rewritten retry algorithm.
Production retry declarations and all four helpers are extracted too. Macros replace
only CMSIS byte-exclusive operations for native execution; deterministic callbacks
model edits/cancellation during payload or between exclusive load/store, and loss
of the exclusive reservation. The actual ARM implementation is compiled separately.
No physical interrupt, card, LCD or audio execution occurs in these host tests.

Coverage: ordinary 1999/2000ms inactivity; six selected open failures for kit and
pattern at normal/wrapping ticks; 216 failed opens and 24 tenth-attempt successes;
2/4/8/16/32s capped waits; 4000 new edits during cooldown without storms; current-RAM
serialization; independent pending types; manual operations during cooldown;
same-tick edits during I/O/completion; one-shot reservation loss and spacing;
explicit pattern-start cancellation; check-required/unsafe holds; short/payload/
close failures without replay; successful explicit-save re-arming only after
healthy closure; zero autosave UI calls. Existing S3 byte oracle/2544 injected
writes and manual S4-L retry/UI cases remain.

Pinned hashes protect unchanged manual-feedback, audio/IRQ, S1 sources and the
edit notification hook from scope drift. Static checks reject loops, UI, allocation,
remounts, BSP polling and audio calls in the new helpers. Full ARM objects and
firmware images are checked during the task; source hashes are not runtime timing
measurements. Native tests use mocked file APIs; established S1/S2/S3 suites still
exercise their production layers independently, including public sync/close faults.
LeakSanitizer is disabled for executor tracing; ASan/UBSan remain fatal. Native
execution retains the existing 40-second deadline.

A dirty tag remains nonzero during synchronous I/O. A single foreground exclusive
store clears only that tag after full success, never a concurrent edit/cancellation.
No interrupt mask, lock/spin loop, per-edit generation hook, or extra audio ISR
work is added. Lost reservations retain pending work with ordinary two-second
spacing. Readiness/mount/ownership/invalid-state errors are intentionally not
silently retried. A held S1 or shared-file fault cannot be cleared by this feature.

Only the two autosave writes are rooted at the existing /tmp/tmp.snd and
/tmp/tmp.pat destinations. Manual/project/load paths are untouched. A full
production FatFs RAM-media test changes CWD to /proj, creates autosave output
using paths extracted from the actual writers, and verifies existing project
kit/pattern bytes and directory entries remain unchanged. The baseline oracle's
relative names are compared at their established /tmp CWD; payload, final target
and write counts remain equal. No folder creation, deletion or CWD mutation is
added to the scheduler.

A cached unmounted SDFatFS state is held without even f_open: autosave does not
use retries to initiate FatFs lazy mounting or reinitialization. Existing manual
mounting and S1 fault protection are unchanged.
