# Two manual RAM kit-load confirmations

Run `python3 tests/kit_load/run.py`. Requires native GCC, ASan/UBSan and the
approved baseline Git object 83c5ec164d6c3dab439deb2f32a8edf1138b5b77.

The fixture extracts actual menu cases and actual production RAM loader bodies.
Hardware, sound sends, time and LCD transfer are mocks. All 256 slot bytes for
both load types verify source-copy/destination-pointer effects, invalid no-op
behavior, confirmation ordering and absence of SD calls. No target audio runs.

The UI fixture executes actual queue/render/expiry/tick/repaint functions. It
checks both labels, success text, 749/750ms expiry, UINT32 wrap, repeats and
current-page restoration. Expiry is injected through an actual intercepted
memcpy used by the renderer; the previous broad candidate's fixture incorrectly
expected memcpy injection while its renderer used strcpy. The assertion that
expired text is cleared remains intact. Existing generic-error tests also run.

Static comparisons preserve the complete command handler except the two load
cases and notice cancellation on a new explicit command; existing success-page
renderer, generic error producer/renderer and menu_tick remain byte-identical.
menu.sha256 pins the reviewed menu for the silent-autosave boundary test; all
other original audio/storage/IRQ hashes remain unchanged. Morph tests retain
exact checks of their input/interpolation/formatting functions and datatype.

The timer limits logical notice eligibility to 750ms. Physical LCD update
latency, ISR cycles and live audio continuity need separately authorized hardware
observation. No tests here flash hardware or prove electrical display timing.
