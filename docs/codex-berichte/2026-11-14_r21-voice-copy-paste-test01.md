# R21 — Voice Copy/Paste Between Kits — TEST01

2026-11-14 UTC · `DocLXR-02/lxr-desktop-app`.

**SOURCE, HOST TEST and CI/BUILD PASS. Owner hardware acceptance is pending.**
The owner authorized implementation, the loaded-kit identity correction, and
Variant A Morph protection. No hardware test, flash, merge or public release ran.
This is the report for one delivered test candidate.

## Source and authority

- Exact stable R20 base: `b01537282047bda5299cdb1bedeab5e5d5b9bcca`, verified on
  `fix/r20-target-names-2026-11-13`. Development branch `release/r21` was absent
  at preflight. Final head: `861c643632e0a88bdc7d60d9410befb3971489fc`.
- Exact ancestry: base → `0fe5c8a730918d7ebce0a104abc22403ae59c749` (retained
  pre-guard draft) → final head. No merge. [Source comparison](https://github.com/DocLXR-02/lxr-desktop-app/compare/b01537282047bda5299cdb1bedeab5e5d5b9bcca...861c643632e0a88bdc7d60d9410befb3971489fc).
- Report-only branch `reports/r21-test01-2026-11-14` is based on unchanged
  `lab-setup` at `5829398cea5d337200393757e39a37d4d8148256`. It is not firmware.
- Read source `AGENTS.md`, private KnowledgeBase v1.5.8 §0.0/§6 and the requested
  status/workflow/evidence/recordkeeping/R20 report at docs SHA
  `9b68be76d80de50cb84fa9882c5b9003bcb6faf8`. The KB was supplied during preflight
  and is not republished here. Model/effort selection, remaining quota and reset
  cannot be independently verified from this task; UNKNOWN. No subagents ran.
- Reused R20 CI43404559276, attempt1, job131815595871, artifact14026826459.
  Retrieved original archive SHA-256
  `ac369072d615d7c7ee478eea0b7c2cbcfc7ef800be0824e07f1211a20d2d7d6b`.
  CRC and pinned ELF/IMG hashes match the R20 report; artifact expires2026-12-13.
  The [upstream Morph PR11](https://github.com/erica-synths/lxr-desktop-app/pull/11)
  was read. Public source/manufacturer v1.71 binary equivalence remains unproven.
- All126 preexisting remote refs were unchanged after source publication/build;
  only authorized `release/r21` was added. Preserve `main`, `lab-setup`, R4/R11/
  R18/R19/R20 and all accepted references. R18 CLP sound remains closed.

## Final controls and persistence

In Voice Edit with transport stopped, select a Voice, hold unshifted COPY and
press DATA. Release both controls, rotate DATA to the destination (`V5 > KIT 02?`),
then press DATA again. COPY cancels. Other panel presses cancel and are consumed,
except START/STOP operates normally and makes running-transport confirmation
reject. Unrelated button releases retain their normal cleanup behavior. Entry,
release and bounce cannot confirm or toggle the normal DATA edit action. A40ms
release/edge guard complements the unchanged physical encoder debounce.

`success!` follows the completed target RAM commit. Cancellation shows `cancelled`,
invalid context/target shows `rejected`. The source remains loaded with its edits,
pattern association and Load/Save cursor intact; the target is never loaded by
transfer. The source snapshot is disposed of at session exit. Both HH buttons
transfer the one shared instrument with both decays; their sequencer tracks stay
separate. Confirmation performs no SD write. Normal SAVE+PROJECT persists both
the target change and source edits, with no separate SAVE+KIT requirement.

**Owner-approved Variant A:** if the destination is the currently referenced Morph
target, reject with `Morph active` when the amount is nonzero OR a sweep is pending,
including zero amount with a pending sweep. Permit zero amount with an idle sweep.
Unrelated targets remain allowed, including during an active sweep. Guard before
staging and again at final commit; rejection preserves Morph values, state/index,
pointer, source/target bytes and engine behavior. No Morph buffer, interpolation
or parameter-ownership redesign was introduced.

## Diagnosis and implementation

Production scope is the preset transfer/validation and identity helpers, menu
selector/rendering, panel interception, one read-only pending-kit-change sequencer
query, headers and the R21 boot string. The22-path patch/path list is retained in
[primary evidence](../evidence/r21-voice-copy-test01/). New tests/workflow and small
existing fixture adaptations are the remaining changes. DSP/FX code, MIDI low CC/
CC2 encoding, file formats and bootloader are unchanged.

**SOURCE and HOST TEST: identity defect.** R20 `preset_writeProjectData` copied the
active buffer to the Load/Save cursor's slot. `preset_saveTmpKit` serialized that
cursor as the buffer identity. The retained real-FatFs reproduction sets loaded
slot9/Kit10, cursor1/Kit2, stored source coarse11, active edit77, target99. Project
save/reload leaves source11 and replaces target99 with77. Temporary save writes
slot1 instead of9. This is a confirmed baseline defect.

R21 project/temporary saves use `menu_loadedPresetNr[CURRENT_KIT]`. RAM loads update
that identity and a revision; temporary recovery restores both selected and loaded
identity. Transfer validates loaded kit/project, revision, active/pending pattern
and transport before transfer and at commit. A/B/A loads invalidate the snapshot.
The new pending query includes a same-pattern queued reload. This is the narrow
identity correction needed by R21, not broader storage replacement.

**SOURCE and HOST TEST: values/LFO/STEP.** Explicit per-instrument saved IDs were
checked against `Parameters.h`, `menuPages.h`, mapping and preset paths. They cover
oscillator/FM/envelope/transient/filter/drive/local sample-rate/volume/pan. Staging
starts from the target record; changed values validate before one RAM commit.
Output routing, FX, velocity modulation, Morph fields, other voices, patterns,
project globals and placeholder/transport fields are preserved.

Only a source own LFO with nonzero amount, a matching target-voice field and an
actual decoded parameter belonging to that instrument transfers. OFF, FX, global,
other-voice and invalid destinations are excluded. All eight saved LFO groups copy.
Other target LFOs owning that exact decoded destination become OFF, including a
zero-amount competitor; their remaining fields are retained. Other destinations
on the same voice remain. Normal saved fields preserve this through project save/
reload, and the unchanged load path applies the updated assignments. Other-track
retrigger selection follows the target pattern's events.

`buttonHandler_copyEditableBase` reads original base values beneath temporary held
STEP locks without editing the active buffer or disarming STEP. Releases needed
for normal lock cleanup remain available. Direct transfer generates no automation,
parameter, FX-preparation, MIDI-transport or SD dispatch.

**Retained Morph failure.** The pre-guard draft reproduced target57 versus source123:
an in-progress Morph sweep read the transferred123 through the target's RAM alias.
The failing assertion and public draft are retained. After owner approval, Variant A
passes active/pending rejection, zero/pending rejection, nonzero/idle rejection,
zero/idle success, unrelated-target success during an active sweep and a sweep
starting at the commit boundary. Source/target bytes, Morph state/pointer, engine
observations, FX counters and SD calls/sectors are checked. Completing the actual
zero-amount sweep before retry permits transfer. No additional buffer was added.

## Executed validation

[Commands, fixtures and retained failures](../evidence/r21-voice-copy-test01/README.md),
[run ledger](../evidence/r21-voice-copy-test01/run-ledger.json), and
[final R21 CI log](../evidence/r21-voice-copy-test01/CI_r21-host.log) provide primary
results. Local environment: GCC14.2.0, Python3.12.14, ASan/UBSan. No random seed;
saved-byte formulas and RAM FAT16/FAT32 fixtures are documented in the evidence.
Exact final CI commands are in [the pinned workflow](https://github.com/DocLXR-02/lxr-desktop-app/blob/861c643632e0a88bdc7d60d9410befb3971489fc/.github/workflows/r21-voice-copy-validation.yml).

| Executed check | Result |
|---|---|
| `python3 tests/r21_voice_copy/run.py` | PASS local/final CI: Kit10 unsaved V5→Kit2, different cursor, zero transfer IO/dispatch, project roundtrip, exact all-kit byte mask for six engines/seven selectors, both HH decays, LFO eligibility/conflicts/roundtrip, edges/cancel/context/clock/commit rejection, short legacy kit, pattern-linked load, CC127/CC2, held STEP base and all Morph cases. |
| Standalone capture compile/run | PASS core capture scenarios. |
| Standalone FIX8 playback compile/run | PASS exhaustive16,384 endpoint pairs and106,792 intermediate changes. |
| `tests/r20_ui/run.py` and integration `probe.py` | PASS REC/STEP, actual target names, LCD notices/timing, shared ownership and MIDI interleaving. |
| `tests/project_storage/run.py` | PASS ASan/UBSan public-index/legacy/containment/error groups and real-FatFs gate. |
| `tests/project_transaction/run.py` | PASS normal replacement;118 fault/retry cases,50 logical cuts,16 cross-sector rename cuts,8 held alias states,36 edge/recovery cases across FAT16/FAT32. |
| `tests/bug001/run.py` | PASS production parser,256 Morph positions, immediate loads, reserved transport exclusions, restart and audio-service cadence. |
| Delay, USB MIDI and Compressor regression scripts | PASS, retaining expected original-source reproductions. |
| `tests/fx_clip_r18/compare.py` | PASS135 unchanged core return-state comparisons to the pinned comparator; no CLP retuning. |
| Matched ARM Release and `validate_arm.py` | PASS image/ELF identity, updater, R21 boot bytes, static fit and ZIP. |
| Retrieved delivery verification | PASS original archive digest/CRC, source, four output hashes, updater, boot bytes, ELF segments and one-image ZIP equality. |

The fixture executes complete production public panel dispatchers, selector,
transfer/storage bodies, load sweeps, Cortex LFO destination case and LFO retrigger.
HAL/LCD/media, unrelated panel helpers and parameter application/modulation-node
effects have explicit host adapters. This verifies control/data paths and assignment
bytes, not full audible rendering, physical contacts or target timing. Other target
modulation/automation/FX/Morph can shape its eventual sound. No whole-firmware
acoustic-identity claim is made.

Expected failures/corrections are retained: R20 identity and pre-guard Morph57/123;
fixture duplicate/missing adapter/prototype errors; a legacy fixture filename
corrected to8.3; an R20 test boot-label assertion updated to R21. Local native syntax
probing could not model ARM intrinsics and is not ARM validation. Local toolchain
and direct artifact downloads returned403; CI and GitHub connector retrieval
succeeded. GitHub rejected the unpublished draft's private commit email; identical
source-tree bytes were recommitted with the verified account's public noreply
identity before the first successful push. No privacy setting or protected ref
changed. None of these failed attempts is presented as an initial pass.

## CI/BUILD and delivery

[CI43621319613](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43621319613),
attempt1, [job132527438779](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43621319613/job/132527438779),
SUCCESS at exact final head. Ubuntu24.04, ARM GNU14.3.Rel1/GCC14.3.1, CMake Release.
No compiler warning/error matches in the retained configure/build logs.

Artifact14123628907, `r21-voice-copy-build-evidence`, created2026-11-14T17:29:43Z,
expires2026-12-14T17:29:43Z. Original downloaded archive
`LXR_R21_TEST01_CI_Evidence_43621319613.zip`:751,371 bytes, SHA-256
`ff0d61158dc2e89e25f85db92342dc09398b919135456a4b8197d8671ab65963`.

| File | Bytes | SHA-256 |
|---|---:|---|
| `LXR_V2_R21_TEST01.img` | 296,168 | `b7deca158337ae799714116e84e7c6f870e48e912c742611a6d25a8574d3415d` |
| `LXR_V2_R21_TEST01.zip` | 212,504 | `0e129b3d65f72e96c26bbea1b2adee1475c8688c150c7c36251de5f6c3dd1874` |
| `LXR_V2_STM32F765.elf` | 404,040 | `d2f69ec54102f1e83acb5a1f9bc757c804eb87bdababf75e0da7d2827cdb6af0` |
| `LXR_V2_STM32F765.map` | 551,853 | `54a18bd9a86d6cbf2917eb8c05a13bb09a033c5a326b585310c1209902f3b1ed` |

The delivered CI ZIP contains exactly the original-named `LXR_V2_R21_TEST01.img`;
CRC and byte equality to the CI IMG pass. Updater magic `LXRV2IMG`, payload296,152 B,
checksum0x5e, three zero reserved bytes. `LXR LAB R21\0` appears at IMG byte160,264;
see `delivery-verification.json` for the exact boot bytes. This is binary image
evidence, not a device-screen photograph. Source startup retains a cleared second
LCD line. ELF load segments match the IMG byte for byte.

## Matched static resources

| Static bytes | R20 | R21 TEST01 | Delta | Nominal remaining |
|---|---:|---:|---:|---:|
| Flash `.text` |157,064|159,440|+2,376|Included in complete load span|
| ITCM span |15,304|15,304|0|1,080|
| DTCM occupied span |107,424|107,424|0|23,648 before runtime stack|
| General SRAM span including reserved heap |274,884|275,156|+272|118,060|
| Complete Flash load payload |293,488|296,152|+2,664|719,656|
| Updater IMG |293,504|296,168|+2,664|Not a runtime allocation|

All linked regions fit. `.rodata` grows82,744→83,032 (+288), accounting for remaining
Flash growth alongside `.text`. Initialized runtime sections already contribute to
complete Flash load and are not double-counted. The272 B SRAM change is the whole
build's static delta, not an invented per-feature cost. Transfer also uses a bounded
foreground staging array; actual stack high-water is not measured. Variant A adds
no separate Morph buffer. The commit critical section contains validation plus one
bounded RAM copy, restores the prior interrupt mask, and performs no IO/engine work.

CPU/WCET, IRQ/DMA deadline margins, stack high-water and long-term reliability remain
**UNMEASURED**. Static fit and host duration do not establish target timing reserve.

## Retention, limitations and next owner decision

Small primary logs, failure reproducers, deterministic fixture inputs, exact patch,
CI manifests, validation JSON and SHA-256 manifest are retained with this report.
ELF/MAP/IMG/ZIP are in the expiring CI artifact and separately supplied original
archive. An owner-managed permanent binary archive is **NOT VERIFIED**: local files
and chat links are not claimed to be permanent archives. Current project-status/
evidence-index files were not changed; their update remains a documentation gap.

No implementation/build failure remains open in the tested scope. Owner device
acceptance and unmeasured runtime boundaries remain OPEN. Next is independent
review of this exact candidate, followed only by separately approved device work.
No new follow-up task was launched.

Prepared device checklist — NOT RUN and not authorization to flash:

1. After separate installation/recovery approval, confirm the R21 boot label.
2. Kit10 edited V5→Kit2 with COPY/DATA: check source sound/edit/cursor, cancel and
   rejection feedback; SAVE+PROJECT, reload and inspect both kits.
3. Repeat six engines/both HH selectors and decays. Compare retained other voices,
   routing, FX, patterns and automation; check eligible LFO takeover, other same-voice
   destination, former destination base/remaining modulation and target-track retrigger.
4. Check `Morph active` for nonzero amount and pending zero sweep; retry at zero
   after completion, and transfer to an unrelated kit during Morph activity.
5. Check normal COPY/track/pattern/bar, SHIFT+CLEAR, DATA edit, REC/STEP and external
   clock. Use existing R18 CLP listening conditions for a short sound comparison.
   Record actual observations separately; listening does not measure CPU/stack/
   deadlines/reliability.
