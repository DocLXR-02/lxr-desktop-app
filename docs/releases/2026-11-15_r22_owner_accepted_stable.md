# R22 — Current stable firmware release record

**Owner stable decision:** 2026-11-15 (Europe/Vienna).  
**Designation:** **CURRENT STABLE R22**; exact pre-existing TEST03 firmware only.  
**Release basis:** [Owner device acceptance](../hardware/2026-11-15_r22_owner_device_acceptance.md).  
**Native GitHub Release/tag:** NOT CREATED by this action. This is a GitHub documentary release record, not a repository source merge.

## Exact release identity

| Item | Verified identity |
|---|---|
| Repository | `DocLXR-02/lxr-desktop-app` |
| Firmware source branch | `release/r22` |
| **Accepted source commit** | [`0e37942250f79306627702061d851731bdfdbbc8`](https://github.com/DocLXR-02/lxr-desktop-app/commit/0e37942250f79306627702061d851731bdfdbbc8) |
| Direct TEST02 parent | `4372e60337bf5ce35d67e8b5eb577baa3fffb962` |
| TEST01 ancestor | `ace7ad95081aac6a015f11b87b6c540ce100b344` |
| R21 predecessor | `861c643632e0a88bdc7d60d9410befb3971489fc` |
| Last previous stable | R20, `b01537282047bda5299cdb1bedeab5e5d5b9bcca` |
| Boot label | `LXR LAB R22` |
| Exact approved image | `LXR_V2_R22_TEST03.img` — 297,680 bytes |
| IMG SHA-256 | `da1c882f6600c96fa5926ad4797d618ce13eefc16ab5563b8a161686f1909c0b` |
| Exact approved ZIP | `LXR_V2_R22_TEST03.zip` — 213,625 bytes; exactly one unmodified IMG |
| ZIP SHA-256 | `1426b2e4fab334ef852b0fda053a035e862e71b05bcbd496a277e2863d901466` |
| Build | [CI run 43707621969](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43707621969), attempt 1, SUCCESS, Arm GNU 14.3.Rel1 Release |
| CI artifact | [14159486119](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/43707621969/artifacts/14159486119), expires 2026-12-15 11:11 UTC |
| Complete dedicated build report | [TEST03 report at immutable report commit](https://github.com/DocLXR-02/lxr-desktop-app/blob/2dfe56d8eb71b29a1931a04345c3421f9e12ac4f/docs/codex-berichte/2026-11-15_r22-kit-index-display-test03.md) |
| Owner hardware acceptance | [2026-11-15 record](../hardware/2026-11-15_r22_owner_device_acceptance.md) |

## Scope and development ancestry

R21 introduced Voice Copy/Paste between kits with protected RAM commits, Morph guards, proper source-kit identity and project persistence. R22 extended cross-kit transfers across Drum 1/2/3 and then refined the panel UI. The source lineage is uninterrupted:

1. [R21 TEST01](https://github.com/DocLXR-02/lxr-desktop-app/commit/861c643632e0a88bdc7d60d9410befb3971489fc) — kit-to-kit Voice transfer; [build report](https://github.com/DocLXR-02/lxr-desktop-app/blob/30ab66c09217330d16e595f149bcab239b9bfad2/docs/codex-berichte/2026-11-14_r21-voice-copy-paste-test01.md).
2. [R22 TEST01](https://github.com/DocLXR-02/lxr-desktop-app/commit/ace7ad95081aac6a015f11b87b6c540ce100b344) — nine cross-kit Drum mappings; [report](https://github.com/DocLXR-02/lxr-desktop-app/blob/59628c8e0591d70e1812aeda6f6455aee19688b7/docs/codex-berichte/2026-11-14_r22-drum-cross-voice-copy-test01.md).
3. [R22 TEST02](https://github.com/DocLXR-02/lxr-desktop-app/commit/4372e60337bf5ce35d67e8b5eb577baa3fffb962) — DATA Kit/Drum/OK selection, physical VOICE cancel, COPY ignored; [report](https://github.com/DocLXR-02/lxr-desktop-app/blob/1559b557db69abd4ef6d152fc886a8d3fb8a5cfd/docs/codex-berichte/2026-11-14_r22-voice-copy-encoder-navigation-test02.md).
4. [R22 TEST03 STOP](https://github.com/DocLXR-02/lxr-desktop-app/blob/bf9162ae0b131c97a12d3ea4ab472a45d96f4cc8/docs/codex-berichte/2026-11-15_r22-kit-index-display-test03-stop.md) — new 64-slot oracle failed due to invalid LFO voice IDs in a test fixture; **historic failure preserved, not a shipped candidate**.
5. [R22 TEST03 final](https://github.com/DocLXR-02/lxr-desktop-app/commit/0e37942250f79306627702061d851731bdfdbbc8) — UI label K00–K63, fixture correction; [host/ARM report](https://github.com/DocLXR-02/lxr-desktop-app/blob/2dfe56d8eb71b29a1931a04345c3421f9e12ac4f/docs/codex-berichte/2026-11-15_r22-kit-index-display-test03.md).

**Final functionality:** In stopped Voice Edit, COPY+DATA opens transfer; DATA navigates Kit/Drum/OK and edits the selected Kit/Drum field; only DATA on OK transfers; VOICE mode button cancels without entering STEP; COPY is inert while the dialog is open. Drum1–3 can target Drum1–3 across kits. Other source voices copy to their corresponding target instrument across kits. Same-kit transfers are intentionally rejected. The copy label K00–K63 matches the underlying RAM slot, LOAD/SAVE number and `.snd` filename prefix. Morph, LFO, STEP, project-save/load protection and normal button behavior remain constrained as recorded in the reports.

## Existing validated evidence and measured limits

**CI/HOST PASS** at the accepted source includes 448 literal kit-index/voice cases, eight real FatFs project/file roundtrips for slots 00/01/09/63, seven independent same-kit rejects, nine Drum mappings, 306 eligible LFO destination mappings, inherited R21/R20/storage/UI and DSP-related host regressions, one full ARM Release build, and independent IMG/ELF/ZIP hash and header checks.

**Matched TEST03 static resource amounts:** `.text` 160,712 bytes; `.rodata` 83,272; ITCM 15,304 (1,080 nominal gap); DTCM span 107,424 (23,648 nominal gap *before runtime stack*); SRAM span 275,164 (118,052 nominal gap); flash payload 297,664; IMG 297,680 bytes. Relative to TEST02, complete IMG is 16 bytes smaller, with no change to ITCM/DTCM/SRAM.

**OWNER HARDWARE/DECISION:** The owner reports R22 works and explicitly accepts the exact TEST03 candidate as current stable. No full test matrix, duration or instrument-side readback digest was supplied.

**UNMEASURED:** CPU/WCET, IRQ/DMA deadlines, runtime stack high-water, long-duration/thermal reliability, exhaustive hardware or acoustic behavior. Static headroom does not prove these values.

## Artifact preservation and workflow boundaries

Source SHA, CI run, original hashes and published raw text evidence are durable references. GitHub Actions binary artifacts expire; the original TEST03 ZIP/IMG and its original CI evidence archive should be retained in the owner's private Library with content verification. A report or a transient Codex `/workspace` path alone is **not** a durable binary archive. Record any verified owner Library copies separately.

The release name is **R22**, while the *approved existing binary filename remains* `LXR_V2_R22_TEST03.zip`; changing the name is not required for stability and must not be mistaken for a new build. No `main` merge, release/tag creation, bootloader modification or source changes were made in this acceptance update. R22 source/TEST01–03 and the historic STOP branch must remain reproducible. Public project-wide branch/workflow restructuring is deferred to a separate owner-authorized conceptual task.
