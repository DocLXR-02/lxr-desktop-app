# R22 — Drum 1–3 Cross-Voice Copy and Same-Kit Transfer — stop gate

Date: 2026-11-14 UTC. Repository: `DocLXR-02/lxr-desktop-app`.

**STOP before firmware implementation/candidate delivery.** The unchanged existing
LFO assignment path replays source, unrelated voice and shared FX parameters. A
transfer-specific contained live-application design remains to be agreed before
R22 can meet its same-kit requirements. No firmware, repository test or CI workflow
was changed. No R22 IMG/ZIP/ELF/MAP was produced or renamed from R21.

## Authority, identity and inherited evidence

The supplied R22 task authorizes implementation, all nine cross-kit Drum1–3 pairs,
six same-kit different-Drum pairs, source-LFO override and the loaded-kit identity
correction. It also expressly requires stopping on uncontained live-engine effects
and forbids inventing a workaround without owner approval. The decisive instruction:

> If the current architecture cannot safely and narrowly synchronize active kit
> RAM, stored kit RAM and the destination Drum's live engine without broad replay,
> unsafe intermediate state, a significant audio/Morph behavioral change, or a
> materially larger architectural rewrite, stop and present a concrete minimal
> reproduction, exact alternatives and measured costs.

The existing setter demonstrably performs broad modulation-target replay, including
shared FX. R22 also requires the original source/other voices to remain unchanged
and no unintended FX processing. I applied the stop gate rather than changing
modulation reset/reapplication semantics or accepting those extra writes implicitly.
This identifies a missing isolated application operation; it does not establish that
all possible R22 designs are impossible or that the owner heard an artifact.

- Exact base and unchanged firmware head:
  `861c643632e0a88bdc7d60d9410befb3971489fc`, tree
  `4792cf3e65aa59409bf9246b893d89287f20643d`, remote `release/r21`.
  The working tree is clean. Authorized `release/r22` was absent and remains
  uncreated; there is no R22 source commit, source diff or feature CI run.
- Report-only branch: `reports/r22-same-kit-stop-2026-11-14`, based on unchanged
  `lab-setup` at `5829398cea5d337200393757e39a37d4d8148256`. Only this report and
  `docs/evidence/r22-drum-cross-voice-copy-preflight/` are published.
- Read source `AGENTS.md`, active private KnowledgeBase v1.5.8 §§0.0/6 and current
  `docs/lab-project-state` status/workflow/evidence/recordkeeping. That docs ref
  still resolves to `9b68be76d80de50cb84fa9882c5b9003bcb6faf8`; it is not firmware.
  The private KB is not published here. No subagents were used. Model/effort and
  account quota/reset are not independently observable; UNKNOWN.
- Reused [R21 report](https://github.com/DocLXR-02/lxr-desktop-app/blob/30ab66c09217330d16e595f149bcab239b9bfad2/docs/codex-berichte/2026-11-14_r21-voice-copy-paste-test01.md),
  primary run ledger, retained57→123 Morph failure, corrected fixture and exact
  R20→R21 source diff. [R21 CI43621319613](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43621319613)
  was checked live: SUCCESS at exact base, attempt1, job132527438779,
  artifact14123628907, ARM GNU14.3.Rel1 Release. Its original archive SHA-256 is
  `ff0d61158dc2e89e25f85db92342dc09398b919135456a4b8197d8671ab65963`.
  Artifact expiry is2026-12-14. No new permanent binary archive was made here;
  permanent binary retention is NOT VERIFIED / NOT ARCHIVED by this task.
- OWNER HARDWARE: the R22 task records that the owner reported R21 TEST01
  successfully tested on2026-11-14. This is owner functional acceptance, with
  matrix, duration, screen photo and measured timing not supplied. It does not
  designate R21/R22 a public stable release or authorize new hardware work.
- `main`, `lab-setup`, accepted R4/R11/R18/R19/R20/R21 and historical refs are
  preserved. Accepted R18 CLP processing was not modified. Public current-stable
  designation/status/evidence index were not changed; synchronization remains OPEN.

## SOURCE — why the existing live path is not contained

All line references are at the exact R21 base:

1. `Core/Src/Soundengine/MIDI/CortexParser.c:491` decodes `FRONT_CC_LFO_TARGET`
   and calls `modNode_setDestination` for the selected LFO.
2. `Core/Src/Soundengine/DSPAudio/modulationNode.c:120` unconditionally calls
   `modNode_resetTargets` before assigning the new destination.
3. `modulationNode.c:90` resets all six velocity nodes and all six LFO nodes:
   **12** calls, regardless of the one intended destination slot.
4. `Core/Src/Soundengine/MIDI/ParameterArray.c:32` uses the current saved
   `activeParameters[id]` to issue a real CC/CC2 parameter application for each
   valid mapped target. Its enabled implementation does not simply restore the
   supplied `originalValue` union.
5. `MidiParser.c:1400` routes FX P1 through `FxBus_setParameter`.
   `Core/Src/Soundengine/FxBus.c:197` writes shared FX state and invokes
   `FxBus_updateParameter`; its distortion branch at148–160 calls all four
   distortion parameter setters.

The existing mixer at `Core/Src/Soundengine/DSPAudio/mixer.c:591` normally resets
and reapplies modulation before rendering each block. Therefore a changed engine
field between blocks is not proof of changed audible output. Additional shared FX
setter calls and unrelated engine writes at the transfer operation are nevertheless
confirmed and conflict with the requested containment/no-extra-FX path. Actual
sound, interleavings and deadlines were not measured.

R21 confirmation only edits an inactive RAM record; it does not install an LFO in
the live engine at confirmation. Its accepted behavior is not contradicted by this
new same-kit requirement. Its host `tests/r21_voice_copy/engine.c` uses an observed
modulation-destination adapter, appropriate to its scope. That adapter cannot prove
R22's stronger live-operation containment. The new diagnostic compiles the actual
setter, reset, pointer mapping and full parameter parser.

## HOST TEST — decisive minimal reproduction

[Primary source-connected fixture, commands and adapters](../evidence/r22-drum-cross-voice-copy-preflight/README.md),
[raw live-path log](../evidence/r22-drum-cross-voice-copy-preflight/probe.log),
[run ledger](../evidence/r22-drum-cross-voice-copy-preflight/run-ledger.json).

Environment: native GCC14.2.0 Debian, Python3.12.14, ASan/UBSan,
`ASAN_OPTIONS=detect_leaks=0`; deterministic inputs, no random seed. Reproduce using
the pinned checkout and `LXR_SOURCE_ROOT=/path/to/repo python3 run.py` from the
retained evidence directory. Full compiler/run arguments, statuses and source hashes
are archived. The scripts refuse a different source HEAD.

Source D1 own LFO has raw amount64, target voice1 and the actual table index for
VOL1. The intended D2 copied destination is the semantic counterpart VOL2. D3's
own LFO modulates D3 volume; another LFO modulates shared FX P1. Engine amounts are
64/127, with sample1. Base bytes are D1 volume32, D3 volume16 and FX P1=25.
A direct internal D2 base-volume update to80 is the negative control: source/D3/FX
are unchanged. Then install only D2's mapped LFO destination through the actual
Cortex case and engine setter.

| Unrelated/source engine field | Before LFO assignment | After assignment |
|---|---:|---:|
| Source D1 volume |0.755905509|0.251968503|
| Unrelated D3 volume |0.629921257|0.125984251|
| Shared FX P1 |0.700787425|0.196850389|

Observed cost of that one assignment:12 reset attempts,3 actual internal parameter
parser dispatches, and4 distortion leaf-setter calls. The intended D2 destination
is installed and saved parameter bytes stay unchanged, but the live operation is
not isolated. Cached-value modulation replay restores all three fields in the
probe; there is no physical audibility claim.

The full actual `MidiParser.c`, `ParameterArray.c`, `modulationNode.c`, real
`parameterArray_init` pointer map and destination tables run. Cortex destination
and shared FX dispatch bodies are extracted verbatim. Wrappers only count and call
through. DSP leaf setters and unrelated dependencies are observed/trapping adapters;
no effect/audio renderer, DMA or physical clock executes. This proves control/data
writes and FX dispatch, not downstream DSP audio changes. An initial missing SVF/
mute link-adapter failure is retained; trapping adapters corrected the fixture,
without changing production code. Probe execution exits0 when it reproduces the
expected contract violation and passes the negative control; that is not an R22 pass.

## HOST TEST — same-kit persistence constraint

[Persistence reproduction](../evidence/r22-drum-cross-voice-copy-preflight/persistence.log)
uses the actual project serializers and production FatFs on RAM FAT16 media.
`preset_writeProjectData` at `presetManager.c:2191` captures the active loaded kit.

Loaded Kit10/slot9, cursor24/slot23, unsaved source D1 coarse123, unsaved destination
D2 coarse77, old stored destination11. The deliberately naive proposed R22 strategy
writes only the stored D2 byte123. Normal SAVE+PROJECT and project reload restore
D2 to77, losing the copy. A control updating both active and stored D2 preserves123
and source123. Each saved kit has247 sound bytes. Both scenarios execute normally
and assert those outcomes. This is a rejected hypothetical strategy, not an existing
R21 gesture; R21 correctly rejects same-kit transfer.

Thus active/stored coherence is feasible in a storage-only control. It does not
resolve isolated live LFO application. The full R22 strategy must also respect
held STEP original-value recovery and preserve all other unsaved edits, with no
engine calls in the interrupt-masked RAM commit. Those feature cases were not
implemented or certified after the live-application stop.

## Alternatives and actual costs

No alternative below was silently implemented or selected.

| Alternative | Required behavior and scope | Cost/evidence status |
|---|---|---|
| Transfer-specific isolated LFO assignment/application | Define an operation that installs only changed LFO nodes, restores only their former destinations, preserves remaining modulation in the existing order without advancing unrelated phase, and defines a safe boundary relative to RAM commit/render interrupts. Keep ordinary controls/R21 unchanged. Requires review of destination/cache/reapplication semantics and same-kit STEP recovery. | No implementation or ARM measurements. Code/RAM/stack/WCET UNKNOWN. Existing-path measured baseline is12 reset attempts/3 parser dispatches/4 FX leaf calls in this fixture. A separately bounded design/validation authorization is needed. |
| Audio-boundary transfer command | Publish a bounded operation and apply affected live state at a defined render boundary; success only after completion, with explicit context/clock cancellation rules. | Adds command/state lifetime, scratch and IRQ workload. Exact storage, latency and CPU/stack costs UNMEASURED; this expands architecture and needs owner approval. |
| Defer same-kit support | Implement only cross-kit remapping under a revised task. | Does not satisfy the current six same-kit pairs; no revised scope authorized or started. |
| Full kit reload/sweep or blanket modulation replay | Reapply broad state after resetting assignments. | Explicitly excluded. Would touch unrelated state/shared FX; not an acceptable workaround here. |

No new production code/static memory was added. No R22 ARM resource delta was
measured because no firmware candidate was built. Inherited matched R21 values are
`.text`159,440 B, ITCM15,304 B (1,080 B nominal gap), DTCM107,424 B (23,648 B before
runtime stack), SRAM275,156 B (118,060 B nominal gap), complete Flash payload296,152 B.
The existing R21 transfer staging array has247 bytes; no new R22 foreground scratch
was introduced. Native probe elapsed times in the ledger include host compilation/
execution and are not M7 runtime costs, deadline evidence or quota estimates.

## Test disposition, artifacts and next decision

| Evidence class/check | Disposition |
|---|---|
| SOURCE base/ref/guidance/R21 diff and live path | EXECUTED; exact base verified, clean source, no R22 branch divergence. |
| HOST TEST live LFO containment | EXECUTED; required contract violation reproduced, targeted base-update control passes. |
| HOST TEST naive persistence and active+stored control | EXECUTED on real FatFs RAM media; stored-only loses the copy, control persists. |
| R21 host/ARM/Morph regression evidence | INHERITED and source/CI identity checked; not relabelled R22 execution. |
| R22 mapping15-pair matrix, full persistence/live/LFO/panel/context/Morph suite | NOT RUN: feature implementation stopped at preflight gate. |
| New full R21/R20/storage/FIX8/BUG001/FX regression campaign | NOT RUN in R22; no production change and no candidate to certify. Existing R21 results reused. |
| R22 ARM GNU14.3.Rel1 Release CI/image/header/ZIP/ELF/MAP checks | NOT RUN; no R22 run/job/artifact or image. IMG/ZIP/ELF/MAP byte counts and hashes: NOT PRODUCED. |
| OWNER HARDWARE for R22 / flashing / merge / release | NOT RUN. |
| CPU/WCET, IRQ/DMA deadlines, stack high-water, reliability, physical sound | UNMEASURED. |

The report and small primary fixtures/logs/manifests are the only published changes.
No new binary needs archiving. Public current-status/evidence-index synchronization
remains a separate gap. No follow-up task was launched.

Next owner/CHAT decision: review this exact existing-path reproduction and choose
whether to authorize a bounded transfer-specific LFO application design, approve a
broader render-boundary design, or revise same-kit scope. Until that decision and
successful mandatory R22 validation, no flash candidate is delivered.
