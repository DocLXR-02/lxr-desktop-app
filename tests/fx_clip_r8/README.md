# R8 experimental single-routed-voice CLP output gain

Base is exact R7 `6d7cb7f7763158ed9abc465a72df7f2560f26b57`.
Run `python3 tests/fx_clip_r8/run.py --output build/r8-host` with native GCC
sanitizers and NumPy 2.3.5. R8 is experimental, not hardware validated.

The mixer captures fader information before any voice advances its live smoother.
All six voice implementations are unchanged: voices 1–3 call calcOnePole once
per frame with r=.01; snare/cymbal/hat use r=.1 in either velocity branch.
The capture uses each actual filter's current b and r and the existing squared
ADC target. For exactly one FX-routed voice, FxBus_clipFaderGains replays the
same production calcOnePole on a private copy. It never advances the live filter.
No interpolation or independent persistent output smoother is introduced.

The gain array is audio-owned, 64 float samples in DTCM, and consumed synchronously
by explicit WithClipGain calls. No DSP or bus object retains a pointer or new
gain history. Existing APIs wrap the new calls with NULL (unity). Zero or multiple
routed voices return NULL; a silent routed voice still counts. Non-CLP capture
returns NULL. Each block is freshly selected, so routing/kit/FX transitions cannot
reuse a previous voice's gain. Eligibility changes take effect immediately:
multiple/no-voice fallback is exact R7, rather than an aggregate policy or crossfade.
Routing changes may therefore produce expected level steps requiring listening.

Only audible core output is multiplied: wet = lastOut*gain + R7_bass.
lastOut, feedback/filter/slew histories, detector, .22 excitation trim,
-.25 parallel bass, vol and existing outer dry/wet arithmetic are retained.
Bass and dry receive no extra fader. Ordinary fader moves request no reset.
Kit/FX reset ownership and short idle timing remain R7. No long release is added.

Tests compile actual DSP, bus and 1PoleLp production source and the exact mixer
capture function (the remaining voice engine is represented by filter-state fixtures).
Static checks protect the unchanged voice files, ADC law, storage and mixer flow.
Unmodified inherited DSP/state/bus assertions still run. Unity is compared against
pinned R7; zero/multiple routing fallback is compared through actual bus dispatch.
Gain-varying identical-input tests compare entire DSP state, bass and dry behavior.
All 64 route masks, all 36 sole-voice/output-route combinations, silent extra
voices, initialization, FX/kit/routing changes, near-zero targets and 1/7/64
partitions are checked. gain-trajectories.csv records exact replay for all six voices.

318 unity paired renders cover kicks, tones, squared input-level sweeps, natural
decays, sparse/returning notes, DC and maximum regeneration. 72 moving paired
renders cover physical input+output fade and identical-input/output-only controls;
other probes cover static gains, zero VOL/DW and unchanged Tube/Fold. Rendered
control trajectories are synthetic per-sample targets; real hardware ADC updates
are sampled once per captured block. WAVs, peaks and steps do not prove musical
fade quality or monotonic total RMS. CPU/stack/IRQ performance requires hardware.

CI uses one Release ARM build with the recorded R7 compiler version, packaging
LXR_V2_R8.img and LXR_V2_R8.zip (one image entry only), with complete source/build
evidence separately. Header/checksum/vectors and ELF/image flash bytes are checked.
