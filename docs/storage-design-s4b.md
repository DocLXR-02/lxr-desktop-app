# Project storage safety principles

The LXR-02 is a live performance instrument. Storage handling must preserve
audio, sequencing, MIDI and RAM editing, with small, understandable mechanisms
and minimal CPU, RAM, flash and SD activity. Autosave stays silent.

Report success only after required writes, byte counts and closes succeed under
the filesystem API contract. Keep recoverable errors retryable; do not replay an
uncertain write, discard dirty RAM, or bypass the established storage fault and
file-ownership safeguards. Do not reset, remount or repair a card to hide errors.

S4b-1 permits full-project saving only to an absent `PROJ00`–`PROJ63` directory.
An existing path, including an empty or incomplete directory, returns `FR_EXIST`
without replacement. Failed new saves can leave partial directories; no automatic
cleanup or retry overwrites them. This is temporary containment, not a transaction.

S4b-2 should review one bounded staging/previous-copy replacement workflow,
explicit outcomes and simple interruption handling. Preserve conventional project
slot semantics and existing payload formats. Do not accumulate unlimited project
generations, introduce a recovery database or silently migrate user data.

Manufacturer firmware compatibility must be checked separately; source version
1.7 is not proven equivalent to public firmware v1.71. Reasonable protection from
interrupted saves is useful, but FAT rename and SD writes are not an absolute
power-loss guarantee. An independent SD-card backup remains the user's responsibility.

**NOT READY FOR HARDWARE INSTALLATION until S4b-2 is completed and reviewed.**
