# R18 Compressor Mono — Saturation Correction and Host Validation

**Date:** 2026-11-05 (Europe/Vienna)
**Status:** Isolated candidate **HOST PASS**. Overflow-range sound policy awaits owner acceptance. **No ARM build, device test, installation, integration, source merge or release.**

## Authorization and source identity

The owner authorized continuing with the isolated Compressor correction after USB-MIDI ARM validation, but did not approve a new sonic reference, a firmware installation or a combined release. The accepted FX-CLP R18 remains frozen.

| Role | Immutable identity |
|---|---|
| Repository | DocLXR-02/lxr-desktop-app |
| Accepted original R18 | 41b364067bce0d8cde624c184933d9cb88e2725b |
| Candidate branch | fix/r18-compressor-mono-saturation |
| Final host-tested candidate SHA | **ff8f7ae5a7217f65928318f23e8c69b1e6a22485** |
| First patch/test commit | 99d9037a84991ed63f684c1fb1ed7d21a92d87f6 |
| Original Compressor.c Git blob | 9e5f34bcecae43a9696cca30658c34a668c8f679 |
| Candidate Compressor.c Git blob | 7bb84666575c0a35e93e10563aa5b5c559432030 |
| Final successful host CI | [42066604418](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42066604418), attempt 1, job 127382278270 |
| Retained CI evidence | Artifact 13434687100, 30 days; expiry 2026-12-05T16:39:17Z |
| Documentation base | 3cdca52d926c220c3bbcf2c5cc32bc05a5681452 |

**Exactly one production line** in Core/Src/Soundengine/DSPAudio/FX/Compressor.c changed, preserving CRLF. Only two non-production files were added: tests/r18_compressor_mono/run.py and .github/workflows/r18-compressor-mono-host.yml. GitHub diff and CI confirm CLP, FX bus, mixer, Delay, USB and boot code are unchanged. The accepted R18 source and image were not edited.

## Fault and correction

Original R18 mono path performs an unchecked floating-point-to-int16 conversion:

    left[i]  = fxBuffer[idx1] * gr * c->markupGain;//gain;

With legal sample +32767, unity ratio and make-up gain 3, the calculated value is +98301, outside signed 16-bit range. The original function triggers the float-cast-overflow sanitizer. Input -32768 at the same gain reproduces the symmetric issue. The reproduced defect is a **host source-level numerical failure**, not an observed physical instrument malfunction.

Candidate mono conversion:

    left[i]  = __SSAT(fxBuffer[idx1] * gr * c->markupGain, 16);//gain;

This applies the signed 16-bit saturation already used by the existing stereo Compressor path. It does not alter threshold, ratio, make-up control mapping, envelope, stereo algorithm, dry/wet or output accumulation. Previously undefined out-of-range mono behavior now has a deterministic saturation policy. **An audible difference under overdrive is possible and NOT owner-accepted.** In-range sample parity is supported only by bounded fixtures.

## Executed validation

Environment: Ubuntu 24.04 native GCC; flags include -std=gnu11 -O1 -g -fno-strict-aliasing -fsanitize=undefined,alignment,bounds,float-cast-overflow -fno-sanitize-recover=all. The test runner extracts actual original and candidate functions, asserts the original Git blob, and requires that the candidate source bytes equal the original with precisely one line replaced. ARM __SSAT and packed-add operations are host-modeled; **no ARM code was executed**.

Executed command:

    python3 tests/r18_compressor_mono/run.py build/r18-compressor-mono-host

**10 classified process fixtures, all as expected:**
- Original and candidate normal-range cases: both PASS.
- Original positive and negative mono over-range cases: both **expected sanitizer failures**, with float-cast-overflow text verified.
- Candidate positive and negative mono over-range: both PASS and saturate to +32767 and -32768.
- Original and candidate positive/negative stereo cases: all PASS; the pre-existing stereo saturation behavior is preserved.

A separate sample-stream comparison covered **144 cases** (2 channel configurations x 6 compression ratios x 3 makeup levels x 4 dry/wet settings), 64 frames each. The original and candidate output sample streams were **byte-identical for these bounded in-range fixtures**. These results do not certify acoustic equivalence for every setting or over-range trajectory.

Existing R18 CLP unit, state, mixer, FX-bus, core and MAX-control regressions passed, including the established 75,000 differential mixer calculations, 135 core snapshots, 16,384 MAX selectors and 4,096 ramp checks. BUG001, kit load, project storage and Euclidean tests also passed.

## Failed attempts and resolution — retain provenance

1. [42066372469](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42066372469) failed because the **new test harness** misclassified mode name "negative" as "normal." This was a test-dispatch error, not a demonstrated candidate compressor failure.
2. [42066479180](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42066479180) passed all functional fixtures and the 144-case comparison, but failed the evidence manifest check because it hashed a RUN_SUMMARY.log concurrently written by tee. This was **an evidence recording race**, not a production failure.
3. [42066604418](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42066604418) passed all functional, regression, source-boundary and manifest checks after fixing the runner hash exclusion. The **one-line production patch did not change across these CI runs**.

## Independently verified archive

The final CI artifact was downloaded and checked outside the CI job.

| Item | Independently verified result |
|---|---|
| GitHub Actions artifact | 13434687100, r18-compressor-mono-host-evidence |
| Downloaded evidence ZIP | **200,091 bytes** |
| ZIP SHA-256 | **59d4ca8a91bfee899d75b515cebf8b1fa7ee1ca4312814641c302bf7ed69c8b2** |
| ZIP CRC | PASS, 41 entries |
| Runner checksum manifest | **23/23 PASS** |
| CI record checksum manifest | **36/36 PASS** |
| Result JSON | all_expected_outcomes = true; 10 process cases; 144 normal-domain configurations |

[Final CI artifact](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42066604418/artifacts/13434687100). A matching evidence ZIP copy is stored in the owner's **private Library** under LXR Firmware Lab/LXR_R18_Compressor_Mono_Host_Evidence_2026-11-05.zip. GitHub Actions retention is 30 days; the private Library copy is not published on GitHub.

The archive preserves source-extracted C programs, native binaries, original failure diagnostics, per-case logs, raw normal-range comparisons, regression outputs, compiler information, manifests and results JSON. Failure-run provenance is separately linked above.

## Boundaries and next gate

**NOT EXECUTED:** ARM cross-build, ELF/MAP/IMG resource comparison, firmware delivery, merging fixes, flashing, physical listening, installed USB tests or physical compressor A/B.

**UNMEASURED:** Cortex-M7 CPU/WCET, IRQ/DMA buffer deadlines, actual stack high-water, sustained device reliability and any physical audio response.

**Sound acceptance required:** 16-bit saturation of over-range mono samples is a defensible policy matching stereo, but it changes formerly undefined output values and could be audible. Owner approval of that policy is required before a wider accepted candidate. The tested in-range cases matched exactly; this is **not** a universal sound-neutrality guarantee.

**Next independently authorized task:** If the owner accepts the proposed saturation policy, an isolated ARM Release validation build from exact SHA **ff8f7ae5a7217f65928318f23e8c69b1e6a22485** may be approved. Verify ELF/IMG/header, static resource deltas against R18 and publish a separate English per-build report. Do not integrate Delay and USB, flash, release or accept sound automatically.

## Immutable references

- [R18-to-Compressor patch](https://github.com/DocLXR-02/lxr-desktop-app/compare/41b364067bce0d8cde624c184933d9cb88e2725b...ff8f7ae5a7217f65928318f23e8c69b1e6a22485)
- [Pinned host runner](https://github.com/DocLXR-02/lxr-desktop-app/blob/ff8f7ae5a7217f65928318f23e8c69b1e6a22485/tests/r18_compressor_mono/run.py)
- [Successful host CI](https://github.com/DocLXR-02/lxr-desktop-app/actions/runs/42066604418)
- [Accepted original R18 source](https://github.com/DocLXR-02/lxr-desktop-app/tree/41b364067bce0d8cde624c184933d9cb88e2725b)
- [Canonical project status](https://github.com/DocLXR-02/lxr-desktop-app/blob/docs/lab-project-state/docs/PROJECT_STATUS.md)

**Publication:** Separate report-only branch based on canonical documentation; neither protected reference nor accepted original R18 has been changed.
