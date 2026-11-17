#!/usr/bin/env python3
"""Compile and run the actual R23 routing header on the host (no firmware build)."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="r23-fx-pool-") as tmp:
    exe = Path(tmp) / "r23-route-test"
    cmd = ["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
           "-fsanitize=undefined,bounds", "-fno-sanitize-recover=all",
           "-I" + str(root / "Core/Inc"),
           str(root / "tests/fx_pool_r23/route.c"),
           "-o", str(exe)]
    subprocess.run(cmd, check=True, cwd=root)
    subprocess.run([str(exe)], check=True, cwd=root)
