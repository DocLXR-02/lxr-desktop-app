# R11 experimental solo post-MIX checks

These native tests execute the production generators, helpers, CLP and a verbatim
production mixer block with deterministic HAL/ADC/RNG/control-service shims.
They are not a firmware boot, full modulation/storage engine or device timing test.
Oscillator functions are extracted verbatim; sample playback is unsupported by
this fixture. FILTER-OFF fixtures explicitly select bypass without changing kits.

Run `scope.py`, `arithmetic.py`, `arm_compat.py`, `component.py`, `voices.py`,
`supplement.py`, `renders.py --baseline build/r11-voices/prerequisite`, then
`inherited.py`. Each script accepts `--output`. Requirements: native C compiler,
ASan/UBSan, Python 3.12+, NumPy 2.3.5 and the prerequisite Git object.

`voices.py` checks all 128 MIX values for eight selected configurations, both
velocity modes and all six voices. It preserves the baseline's 98 snare overflow
failures as expected failures, and separately reproduces the unresolved active
filter failures and legacy direct/fallback snare failures. A successful runner
means those failures were classified, not repaired. `supplement.py` adds all-128
checks at actual velocity32 and low physical fader, zero-velocity triggers, real
direct voices and a resolved-runtime-target modulation shim. Real LFO/morph
scheduling and kit storage are not simulated by that shim.

The inherited runner retains R10's numeric assertions and fixtures; only its
scope/display guards are updated explicitly for the approved R11 integration.
The original R10 test files are unchanged. Partition checks cover the new helper,
post-return and CLP paths; complete voice synthesis uses production 64-frame
blocks, whose envelope interpolation is not claimed partition invariant.

Recorded ARM compatibility oracles come from report d0215ef5b090313ff31ce2ae0da88d72dd2100b2,
using corrected-R10 ELF SHA-256
`3cc3b78888aaacf4341f9775efcb7c675cb9e11a11954e3501b289f869418f0b`.
These are isolated instruction-model observations, not device execution.
Periodic seam exceptions are explicit; no bipolar FM or modular PWM is selected.

No test establishes musical success, universal numerical safety or device CPU,
stack or interrupt-deadline capacity. The retained filter defect requires later
listening kits to have ALL SIX voice filters already OFF. Installation/listening
requires separate approval. Legacy snare over-range paths remain unresolved.
