# Bug 001 regressions

Run `python3 tests/bug001/run.py` from the repository root. Requires a native
GNU-compatible C compiler with AddressSanitizer/UndefinedBehaviorSanitizer.
Temporary binaries and generated files are removed after the run.

The parser test compiles the complete production `MidiParser.c`. It observes
Drive writes, modulation notifications, mute commands and external bank-change
forwarding. Unrelated DSP/hardware calls abort through link stubs. The Drive stub
records the normalized input; it does not model the filter or its Drive curve.

The sweep test compiles the unmodified production interpolation, reserved-slot
predicate, immediate kit sweep, Morph tick and Morph start functions extracted
by the runner, plus the real parameter-order array. The MIDI sink records every
dispatch and rejects transport commands. This avoids bringing SD-card/hardware
code into the host process without adding test hooks to firmware.

Coverage includes all 256 Morph positions with identical kits, nonzero reserved
slots, existing restart/tick behavior, kit-load audio service cadence, CC0 with
all cache/modulator flags, valid CC/CC2/NRPN mapping, all seven external NRPN
mutes, invalid/high NRPN selectors, and every out-of-range CC2 byte with all
cache/modulator flags. Sanitizers check exercised memory accesses.
Leak detection is disabled by default because LeakSanitizer cannot run under
ptrace; AddressSanitizer and UndefinedBehaviorSanitizer remain enabled.

These are control-path tests, not an audio simulation or on-device validation.
