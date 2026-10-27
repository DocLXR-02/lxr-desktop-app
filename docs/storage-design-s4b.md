# Project storage safety principles

The LXR-02 is a live performance instrument. Storage handling must preserve
audio, sequencing, MIDI and RAM editing, with small, understandable mechanisms
and minimal CPU, RAM, flash and SD activity. Autosave stays silent.

Report success only after required writes, byte counts and closes succeed under
the filesystem API contract. Keep recoverable errors retryable; do not replay an
uncertain write, discard dirty RAM, or bypass the established storage fault and
file-ownership safeguards. Do not reset, remount or repair a card to hide errors.

Full-project saves retain conventional `PROJ00`–`PROJ63` slots and unchanged
payload formats. Replacement uses one global `/LXRSTG`, one `/LXRBAK`, and a small
`/LXRTRANS.DAT` ownership/completion record. No generations accumulate. A single
foreground transaction writes and validates the stage before moving the old
project aside. Only known, independently redundant contents can be cleaned up.
Unknown files, reserved-name collisions and ambiguous directory aliases are held
rather than deleted. Additional free space for one complete replacement is required.

Interrupted transactions select a validated complete old or new directory for
read-only loading. Boot does not rename, clean up, replay writes or overlay stale
temporary files during recovery. An explicit Save can normalize a clearly owned
state before writing current RAM. It cannot repair FAT metadata or clear physical
storage/file-ownership fault holds. Failed saves retain dirty RAM; cleanup errors
are reported even when the new canonical version has already been published.

FAT directory rename is not atomic. Tested sector-boundary aliases remain
read-only; they need controlled external attention, not automatic deletion.
This deliberately bounded workflow does not provide filesystem transactions,
sector-tear protection, a snapshot of concurrently edited RAM, or automatic repair.
Individual named saves/autosave retain their separate existing replacement policies.

Manufacturer firmware compatibility must be checked separately; source version
1.7 is not proven equivalent to public firmware v1.71. Reasonable protection from
interrupted saves is useful, but FAT rename and SD writes are not an absolute
power-loss guarantee. An independent SD-card backup remains the user's responsibility.

**Software validation is not hardware approval. Controlled hardware preparation
and separate explicit authorization remain required before installation.**
