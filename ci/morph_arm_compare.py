#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Validate and compare matched, non-distributed LXR-02 ARM build outputs."""
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

BASE = "a43cd4fcda23ec3d577dc425261c3eb28bd5abff"
CANDIDATE = "c433f0ecc68ba66dc885784ad03905c8be18cf39"
REGIONS = ("ITCM_RAM", "DTCM", "RAM", "FLASH")
CAPACITY = {"ITCM_RAM": 16384, "DTCM": 131072, "RAM": 393216, "FLASH": 1015808}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def examine(label, worktree, output):
    elf = worktree / "build/release/LXR_V2_STM32F765.elf"
    image = worktree / "build/release/LXR_V2_V1.7.img"
    binary = worktree / "build/release/LXR_V2_STM32F765.bin"
    linker_map = worktree / "build/release/LXR_V2_STM32F765.map"
    log = (output / label / "build.log").read_text()
    for path in (elf, image, binary, linker_map):
        if not path.is_file() or path.stat().st_size == 0:
            raise RuntimeError("Missing or empty " + label + " output: " + str(path))
    compiled = image.read_bytes()
    if len(compiled) < 24 or compiled[:8] != b"LXRV2IMG":
        raise RuntimeError("Invalid update image magic/size for " + label)
    payload_len = struct.unpack_from("<I", compiled, 8)[0]
    payload = compiled[16:]
    if (payload_len != len(payload) or compiled[13:16] != b"\0\0\0"
            or compiled[12] != (0xff - (sum(payload) & 0xff))):
        raise RuntimeError("Invalid update image length, padding, or checksum: " + label)
    if payload != binary.read_bytes():
        raise RuntimeError("Image payload disagrees with objcopy .bin for " + label)
    msp, reset = struct.unpack_from("<II", payload, 0)
    if msp % 8 or not (0x20000000 <= msp <= 0x20080000):
        raise RuntimeError("Unaligned/invalid initial stack pointer: " + label)
    if not (reset & 1 and 0x08008000 <= (reset & ~1) < 0x08100000):
        raise RuntimeError("Invalid Thumb reset vector: " + label)

    mem = {}
    for name, used in re.findall(
            r"(?m)^\s*(ITCM_RAM|DTCM|RAM|FLASH):\s+(\d+) B\b", log):
        mem[name] = int(used)
    if set(mem) != set(REGIONS):
        raise RuntimeError("Missing linker region summaries for " + label + ": " + repr(mem))
    for name, used in mem.items():
        if used > CAPACITY[name]:
            raise RuntimeError("Linker region overflow " + name + " in " + label)

    size_txt = subprocess.check_output(
        ["arm-none-eabi-size", "-B", str(elf)], text=True)
    (output / label / "gnu-size.txt").write_text(size_txt, encoding="utf-8")
    values = size_txt.strip().splitlines()[-1].split()
    text, data, bss = [int(v) for v in values[:3]]
    (output / label / "sections.txt").write_text(
        subprocess.check_output(["arm-none-eabi-size", "-A", str(elf)], text=True),
        encoding="utf-8")
    # Archive map text and configure/build logs, not flashable image/ELF files.
    (output / label / "linker.map").write_bytes(linker_map.read_bytes())

    rec = {
        "label": label,
        "source_commit": BASE if label == "upstream" else CANDIDATE,
        "build_type": "Release (-O3)",
        "toolchain": subprocess.check_output(
            ["arm-none-eabi-gcc", "--version"], text=True).splitlines()[0],
        "regions_bytes": mem,
        "gnu_size_bytes": {"text": text, "data": data, "bss": bss},
        "image_bytes": len(compiled),
        "image_payload_bytes": len(payload),
        "image_sha256": sha(image),
        "elf_sha256": sha(elf),
        "map_sha256": sha(linker_map),
        "img_header_checksum": compiled[12],
        "initial_msp": hex(msp),
        "reset_vector": hex(reset),
        "img_payload_matches_bin": True,
        "img_structural_validation": "PASS",
        "arm_link": "PASS",
        "device_hardware": "NOT TESTED",
        "cpu_wcet_irq_dma_stack_long_run": "UNMEASURED",
    }
    (output / label / "manifest.json").write_text(
        json.dumps(rec, indent=2) + "\n", encoding="utf-8")
    lines = [
        "# Matched ARM firmware build verification — " + label,
        "",
        "This is a GitHub Actions validation build, not an approved firmware release or",
        "a physical-device test. The image/ELF remain unarchived on an ephemeral CI",
        "runner; only hashes, raw build logs, linker map, and this report are archived.",
        "",
        "- Source SHA: " + rec["source_commit"],
        "- Config: Release (-O3), default STM32F765 application (no bootloader)",
        "- Toolchain: " + rec["toolchain"],
        "- GitHub Actions run: " + os.environ.get("GITHUB_RUN_ID", "UNKNOWN"),
        "- Config/build: SUCCESS",
        "- Update IMG structural validation: PASS (header, length, checksum, bin parity, vectors)",
        "- Complete IMG bytes: " + str(rec["image_bytes"]),
        "- Raw firmware payload bytes: " + str(rec["image_payload_bytes"]),
        "- Complete IMG SHA-256: " + rec["image_sha256"],
        "- ELF SHA-256: " + rec["elf_sha256"],
        "- Linker MAP SHA-256: " + rec["map_sha256"],
        "",
        "| Linker region | Used bytes | Capacity bytes | Nominal gap bytes |",
        "|---|---:|---:|---:|",
    ]
    for region in REGIONS:
        lines.append("| " + region + " | " + str(mem[region]) + " | "
                     + str(CAPACITY[region]) + " | "
                     + str(CAPACITY[region] - mem[region]) + " |")
    lines += [
        "",
        "| GNU size section | Bytes |",
        "|---|---:|",
        "| text | " + str(text) + " |",
        "| data | " + str(data) + " |",
        "| bss | " + str(bss) + " |",
        "",
        "The linker-region totals and GNU size sections describe distinct",
        "accounting schemes and must not be summed as interchangeable quantities.",
        "These are static linker figures, NOT measured real-time headroom.",
        "",
        "## Verification boundaries",
        "",
        "- Host Morph regression: separately verified on candidate CI run 42530896384;",
        "  not an on-device or audio DSP test.",
        "- Physical firmware installation, flashing, audio quality, USB-MIDI:",
        "  NOT TESTED in this comparison.",
        "- CPU/WCET, IRQ/DMA deadlines, stack high-water, sustained audio:",
        "  UNMEASURED.",
        "- Source/binary identity with the manufacturer's public v1.71",
        "  production image: NOT ESTABLISHED (source declares version 1.7).",
        "- This CI output must NOT be presented as manufacturer firmware.",
        "",
    ]
    (output / (label + "-arm-build-report.md")).write_text(
        "\n".join(lines), encoding="utf-8")
    return rec


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: compare.py UPSTREAM_WORKTREE CANDIDATE_WORKTREE OUTPUT")
    upstream, candidate, out = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    out.mkdir(parents=True, exist_ok=True)
    for label in ("upstream", "candidate"):
        (out / label).mkdir(parents=True, exist_ok=True)
    a = examine("upstream", upstream, out)
    b = examine("candidate", candidate, out)
    keys = list(REGIONS) + ["IMG full", "IMG payload", "GNU text", "GNU data", "GNU bss"]
    left = [a["regions_bytes"][x] for x in REGIONS] + [
        a["image_bytes"], a["image_payload_bytes"]] + [
        a["gnu_size_bytes"][x] for x in ("text", "data", "bss")]
    right = [b["regions_bytes"][x] for x in REGIONS] + [
        b["image_bytes"], b["image_payload_bytes"]] + [
        b["gnu_size_bytes"][x] for x in ("text", "data", "bss")]
    md = [
        "# Matched ARM Release build — isolated Morph transport-command guard",
        "",
        "- Upstream source commit: " + BASE,
        "- Candidate source commit: " + CANDIDATE,
        "- Same Arm GNU 14.3.rel1 toolchain, runner, CMake Release preset",
        "- No source change beyond the previously verified isolated candidate",
        "- These are successful CI builds, not device or audio measurements",
        "",
        "| Metric | Upstream | Candidate | Candidate minus upstream |",
        "|---|---:|---:|---:|",
    ]
    for key, x, y in zip(keys, left, right):
        md.append(f"| {key} (bytes) | {x} | {y} | {y-x:+d} |")
    md += [
        "",
        "## Integrity",
        "",
        "- Upstream IMG SHA-256: " + a["image_sha256"],
        "- Candidate IMG SHA-256: " + b["image_sha256"],
        "- Image format, declared length, checksum, payload parity and vectors: PASS (both)",
        "- Maps, complete configure/build logs, GNU size/sections, source SHAs",
        "  and separate English build reports are present in the CI evidence artifact.",
        "",
        "## Limits",
        "",
        "Physical-device regression of the isolated patch: NOT TESTED.",
        "CPU, worst-case latency, DMA/IRQ and stack reserves: UNMEASURED.",
        "Neither output is identified as official v1.71, and no IMG was distributed.",
        "This build does not authorize flashing or upstream PR submission.",
        "",
    ]
    (out / "matched-arm-build-comparison.md").write_text(
        "\n".join(md), encoding="utf-8")
    (out / "comparison.json").write_text(
        json.dumps({
            "upstream": a, "candidate": b,
            "deltas_bytes": dict(zip(keys, [y-x for x,y in zip(left,right)])),
            "validation": "PASS",
        }, indent=2) + "\n", encoding="utf-8")
    (out / "NOT_FOR_FLASHING.txt").write_text(
        "CI EVIDENCE ONLY. No flashable IMG/ELF is retained in this artifact.\n"
        "Not an official manufacturer firmware image or approved release.\n"
        "No device hardware or timing validation was performed.\n",
        encoding="utf-8")
    print("PASS: both ARM images structurally verified; matched resource deltas:")
    for key,x,y in zip(keys,left,right):
        print(f"  {key}: {x} -> {y} ({y-x:+d})")


if __name__ == "__main__":
    main()
