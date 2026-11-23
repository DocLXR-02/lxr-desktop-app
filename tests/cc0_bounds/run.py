#!/usr/bin/env python3
"""Source-extracted real CC0 handler + Kit-name access host ASan/UBSan checks.

Negative control deliberately runs the accepted R22 handler and MUST report
out-of-range Kit-name access. Candidate executes every MIDI CC0 value 0..127.
This does NOT claim whole-firmware runtime or physical-device validation.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASE = "0e37942250f79306627702061d851731bdfdbbc8"
PANEL = "Core/Src/frontPanelParser.c"
MENU = "Core/Src/menu/menu.c"

def production_function(source, function):
    pattern = r"(?m)^void\s+" + re.escape(function) + r"\s*\([^;]*?\)\s*\{"
    m = re.search(pattern, source)
    if m is None:
        raise AssertionError("Real function not found: " + function)
    depth = 1
    end = m.end()
    while depth:
        if end >= len(source):
            raise AssertionError("Unbalanced function: " + function)
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[m.start():end]

PREAMBLE = r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NUM_KIT_SLOTS 64
#define NUM_CHARS 76
#define END_OF_SOUND_PARAMETERS 228
#define PRESET_KIT_TIMER 0
#define NRPN_DATA_ENTRY_COARSE 110
#define NRPN_FINE 111
#define NRPN_COARSE 112
#define DISABLE_SIGN_WARNING
#define END_DISABLE_WARNING

typedef struct { uint8_t data1, data2; } MidiMsg;
volatile MidiMsg frontParser_midiMsg;
static uint16_t frontParser_nrpnNr;
uint8_t activeParameters[512];
uint8_t menu_selectedPresetNr[6];
uint8_t menu_loadedPresetNr[4];
char nameEditBuffer[8];
char savefileNames[NUM_KIT_SLOTS][4][8];
const uint8_t characterOrder[NUM_CHARS] = { 32 };
static unsigned loads, touches, repaints, nrpns;
static uint8_t last_loaded;

void preset_loadDrumsetRam(uint8_t kit, uint8_t morph, uint8_t immediate)
{
    assert(morph == 0 && immediate == 0);
    // Mirror accepted R22's independent defensive bound guard.
    if(kit >= NUM_KIT_SLOTS) return;
    loads++;
    last_loaded = kit;
    menu_loadedPresetNr[0] = kit;
}
void preset_touchTimer(int type) { assert(type == PRESET_KIT_TIMER); touches++; }
void menu_repaint(void) { repaints++; }
void frontParser_parseNrpn(uint8_t v) { (void)v; nrpns++; }
"""

MAIN = r"""
int main(void)
{
#ifdef NEGATIVE_CONTROL
    memset(savefileNames, ' ', sizeof savefileNames);
    menu_selectedPresetNr[0] = 9;
    menu_loadedPresetNr[0] = 9;
    frontParser_midiMsg.data1 = 0;
    frontParser_midiMsg.data2 = 67;
    frontPanel_ccHandler();
    // Production R22 Kit loader guards its array, but selected index remains 67.
    assert(menu_selectedPresetNr[0] == 67);
    copyNameToEditBuffer(menu_selectedPresetNr[0], 0);
    puts("UNEXPECTED: original R22 UI out-of-bounds access undetected");
    return 0;
#else
    unsigned valid = 0, rejected = 0;
    for(unsigned cc = 0; cc < 128; cc++) {
        memset(savefileNames, ' ', sizeof savefileNames);
        memset(nameEditBuffer, 0, sizeof nameEditBuffer);
        memset(activeParameters, 0x7b, sizeof activeParameters);
        menu_selectedPresetNr[0] = 9;
        menu_loadedPresetNr[0] = 9;
        last_loaded = 9;
        loads = touches = repaints = nrpns = 0;
        frontParser_midiMsg.data1 = 0;
        frontParser_midiMsg.data2 = (uint8_t) cc;
        frontPanel_ccHandler();
        if(cc < NUM_KIT_SLOTS) {
            assert(menu_selectedPresetNr[0] == cc);
            assert(menu_loadedPresetNr[0] == cc);
            assert(last_loaded == cc);
            assert(loads == 1 && touches == 1 && repaints == 1 && nrpns == 0);
            valid++;
        } else {
            assert(menu_selectedPresetNr[0] == 9);
            assert(menu_loadedPresetNr[0] == 9);
            assert(last_loaded == 9);
            assert(loads == 0 && touches == 0 && repaints == 0 && nrpns == 0);
            rejected++;
        }
        for(unsigned i = 0; i < sizeof activeParameters; i++)
            assert(activeParameters[i] == 0x7b);
        // These are exact production Kit-name functions; ASan+UBSan instrument
        // the real array indexing, including all invalid MIDI input cases.
        copyNameToEditBuffer(menu_selectedPresetNr[0], 0);
        copyEditBufferToName(menu_selectedPresetNr[0], 0);
        assert(menu_selectedPresetNr[0] < NUM_KIT_SLOTS);
    }
    assert(valid == 64 && rejected == 64);
    // Neighboring ordinary CC still reaches its regular parameter handler.
    loads = touches = repaints = 0;
    frontParser_midiMsg.data1 = 1;
    frontParser_midiMsg.data2 = 37;
    frontPanel_ccHandler();
    assert(activeParameters[0] == 37 && loads == 0 && touches == 0 && repaints == 1);
    puts("PASS real frontPanel_ccHandler: 64 valid + 64 rejected CC0 values;"
         " no invalid UI Kit-name read/write, state or autosave side effects;"
         " ordinary adjacent CC preserved (ASan+UBSan)");
    return 0;
#endif
}
"""

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    assert re.search(r"(?m)^#define NUM_KIT_SLOTS 64\s*$",
                     (ROOT / "Core/Inc/Parameters.h").read_text())
    menu = (ROOT / MENU).read_text()
    assert "char savefileNames[NUM_KIT_SLOTS][NUM_SAVEFILE_NAMES][8];" in (
        ROOT / "Core/Src/Preset/presetManager.c").read_text()
    originals = subprocess.check_output(
        ["git", "show", BASE + ":" + PANEL], cwd=ROOT).decode()
    candidate = (ROOT / PANEL).read_text()
    assert "if(kitNr >= NUM_KIT_SLOTS) return;" in candidate
    assert "if(kitNr >= NUM_KIT_SLOTS) return;" not in originals
    actual_ui = "\n".join(production_function(menu, n)
        for n in ("copyNameToEditBuffer", "copyEditBufferToName"))
    results = {}
    with tempfile.TemporaryDirectory(prefix="lxr-cc0-host-") as folder:
        tmp = Path(folder)
        for name, src, control in (
            ("r22_negative", originals, True),
            ("candidate", candidate, False),
        ):
            actual_handler = production_function(src, "frontPanel_ccHandler")
            text = PREAMBLE + "\n" + actual_handler + "\n" + actual_ui + "\n" + MAIN
            cfile = tmp / (name + ".c")
            binary = tmp / name
            cfile.write_text(text)
            cmd = ["cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
                   "-Werror", "-fno-omit-frame-pointer",
                   "-fsanitize=address,undefined,bounds",
                   "-fno-sanitize-recover=all",
                   *(["-DNEGATIVE_CONTROL"] if control else []),
                   str(cfile), "-o", str(binary)]
            build = subprocess.run(cmd, text=True, capture_output=True)
            (args.out / (name + "-compile.log")).write_text(
                "$ " + " ".join(cmd) + "\n" + build.stdout + build.stderr)
            if build.returncode:
                raise RuntimeError("HOST_COMPILE_FAILED " + name + " " + build.stderr)
            run = subprocess.run([str(binary)], text=True, capture_output=True,
                                 env={"ASAN_OPTIONS":"detect_leaks=0:abort_on_error=1",
                                      "UBSAN_OPTIONS":"print_stacktrace=1:halt_on_error=1"})
            (args.out / (name + "-run.log")).write_text(
                "exit_code=" + str(run.returncode) + "\n" + run.stdout + run.stderr)
            if control:
                if run.returncode == 0 or not any(x in run.stderr for x in
                    ("AddressSanitizer", "runtime error:", "out of bounds")):
                    raise AssertionError("R22 NEGATIVE CONTROL did not detect UI OOB")
                results[name] = "EXPECTED_FAIL_INVALID_KIT_UI_ACCESS_DETECTED"
            else:
                if run.returncode:
                    raise RuntimeError("CANDIDATE HOST FAIL: " + run.stderr)
                if "PASS real frontPanel_ccHandler" not in run.stdout:
                    raise AssertionError("Candidate success oracle missing")
                results[name] = "PASS_128_CC0_AND_UI_ASAN_UBSAN"
    results.update({
        "source_file": PANEL, "base_sha": BASE,
        "source_sha": subprocess.check_output(["git", "rev-parse", "HEAD"],
                                                cwd=ROOT, text=True).strip(),
        "candidate_handler_sha256": hashlib.sha256(
            production_function(candidate, "frontPanel_ccHandler").encode()).hexdigest(),
        "device_test": "NOT_RUN", "complete_firmware_test": "NOT_RUN",
    })
    (args.out / "host-result.json").write_text(
        json.dumps(results, indent=2, sort_keys=True) + "\n")
    print(json.dumps(results, indent=2, sort_keys=True))

if __name__ == "__main__":
    main()
