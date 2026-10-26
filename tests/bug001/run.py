#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run focused production-code regressions using a native C compiler."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def function(source, name):
    # Mask comments/strings before counting braces; return the original text.
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                    lambda m: " " * len(m[0]), source, flags=re.S)
    match = re.search(r"^(?:IN_ITCM_RAM )?(?:static )?(?:uint8_t|void) "
                      + name + r"\([^;]*?\)\s*\{", masked, re.M)
    if not match:
        raise RuntimeError(f"Production function not found: {name}")
    depth = 1
    end = match.end()
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    return source[match.start():end]


def run(*args):
    env = os.environ.copy()
    # LeakSanitizer cannot run in ptraced execution environments. These tests
    # target bounds/UB, not allocation; retain ASan and UBSan instrumentation.
    env.setdefault("ASAN_OPTIONS", "detect_leaks=0")
    subprocess.run(args, cwd=ROOT, env=env, check=True)


def main():
    includes = [".", "Core/Inc", "Drivers/CMSIS/Include",
                "Drivers/CMSIS/Device/ST/STM32F7xx/Include",
                "Drivers/STM32F7xx_HAL_Driver/Inc",
                "Middlewares/Third_Party/FatFs/src",
                "Middlewares/JS/STM32_USB_Device_Library/Core/Inc",
                "Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc",
                "USB_DEVICE_MIDI/Target"]
    # ARM-specific inline DSP intrinsics are not executed by these tests.
    # _Float16 supplies the host spelling of the firmware's half-float type.
    flags = [os.environ.get("CC", "cc"), "-std=gnu11", "-O1", "-g",
             "-DSTM32F765xx", "-DUSE_HAL_DRIVER", "-D__fp16=_Float16",
             "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast",
             "-Wno-implicit-function-declaration", "-Wno-unused-function",
             "-ffunction-sections", "-fdata-sections", "-fno-pie", "-no-pie",
             "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
             "-fno-omit-frame-pointer"]
    flags += ["-I" + str(ROOT / p) for p in includes]
    with tempfile.TemporaryDirectory(prefix="lxr-bug001-") as tmp:
        tmp = Path(tmp)
        parser = tmp / "test_parser"
        run(*flags, str(HERE / "test_parser.c"), str(HERE / "trap_stubs.c"),
            "-Wl,--gc-sections", "-lm", "-o", str(parser))
        run(str(parser))

        source = (ROOT / "Core/Src/Preset/presetManager.c").read_text()
        names = ["interpolate", "preset_isTransportParameter",
                 "preset_sendDrumsetParametersImmediate", "preset_morph_tick",
                 "preset_morph"]
        (tmp / "sweep_functions.inc").write_text(
            "\n\n".join(function(source, name) for name in names))
        sweeps = tmp / "test_sweeps"
        run(*flags, "-I" + str(tmp), str(HERE / "test_sweeps.c"),
            str(ROOT / "Core/Src/Parameters.c"), "-Wl,--gc-sections",
            "-o", str(sweeps))
        run(str(sweeps))


if __name__ == "__main__":
    main()
