# S3 FatFs integrity and disk-initialization tests

Run `python3 tests/fatfs_integrity/run.py` from the repository root. Native
ASan/UBSan tests include complete production `ff.c`, `diskio.c`, `ff_gen_drv.c`
and `sd_diskio.c` in one translation unit. Helpers and public `f_sync`, `f_close`,
`f_read` execute their actual implementation. This is not a rewritten algorithm.
Fixed-width target integer typedefs replace only integer.h's LP64-incompatible
host widths. A main.h stub and the existing S1 CMSIS/HAL shim supply unused or
simulated hardware declarations. Real FatFs configuration, structures and macros
are used, with a temporary read-only configuration variant (FS_READONLY=1,
FS_LOCK=0, USE_MKFS=0). No production configuration is modified.

RAM-sector callbacks record exact write order/content and inject failures. The
metadata stub intentionally does **not** latch failures: it proves first-call
error propagation and the file's persistent error on subsequent sync/close,
independent of the production SD latch. An explicit separate sync_fs call can
retry pending FSInfo with this stub; that is not automatic retry inside the
function. In production, serious errors additionally latch the S1 driver, which
rejects subsequent physical I/O. Tests wire that actual driver through actual
registration/disk initialization to prove remount cannot clear the latch.

Coverage: primary/secondary/later FAT failures and success for 1–4 synthetic FAT
copies; dirty retention; FSInfo bytes, pending flag, no-update and sync errors;
public sync/close persistent errors without replay; normal file close/read;
failed/absent/successful/protected initialization, cached status and healthy
unlink/relink; all invalid drive-byte values; SD latch surviving repeated init
and unlink/relink. Three/four FAT fixtures test the internal generic loop, not
acceptance of such volumes by the unchanged mount parser (which accepts 1/2).
No physical filesystem is mounted or formatted. Actual f_mount decodes a valid
RAM FAT16 boot sector for healthy/absent/remount tests; constructed valid
in-memory file objects exercise synchronization without a physical card.

The runner uses a 20-second execution deadline per configuration, ASan/UBSan fatal
errors, and disables LeakSanitizer because of executor tracing limitations.
Unused static helper warnings are disabled because the whole vendor module is
included. The existing unused `lun` parameter warning in ff_gen_drv.c remains
visible. No existing tests are weakened. This does not prove electrical timing,
card recovery, metadata atomicity, power-loss durability or audio behavior.

After a metadata failure, fs->wflag or fs->fsi_flag preserves pending work as
appropriate. Some earlier sectors may already have succeeded. No rollback or
new retry is attempted. After a public disk synchronization failure, fp->err
retains FR_DISK_ERR and FA_MODIFIED is not cleared; later sync/close returns an
error and retains the handle/lock. S4 must handle save errors and failed-handle
ownership deliberately. This test does not implement that recovery policy.
