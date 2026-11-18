#!/usr/bin/env python3
"""Compile and execute bounded C format codec tests under ASan and UBSan."""
import os,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="r23-kit-extension-") as temp:
    for label,flag in [("ubsan","undefined,bounds"),("asan","address")]:
        output=Path(temp)/label
        build=subprocess.run(["cc","-std=c11","-O1","-g","-Wall","-Wextra","-Werror",
            "-fsanitize="+flag,"-fno-sanitize-recover=all",
            "-I",str(root/"Core/Inc"),
            str(root/"tests/r23_fx_kit_format/codec.c"),"-o",str(output)],capture_output=True,text=True)
        assert build.returncode == 0,build.stderr
        run=subprocess.run([str(output)],capture_output=True,text=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})
        assert run.returncode==0,run.stdout+"\n"+run.stderr
        print(label,run.stdout.strip())
print("PASS standalone FX Kit v1 extension codec")
