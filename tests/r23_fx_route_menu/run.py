#!/usr/bin/env python3
"""Validate the R23 voice routing menu and external MIDI bounds against source.

Compiles the real MenuText.h routing labels and SharedFxPool.h routing decoder,
plus source-extracted menu clamp and MIDI range guard. This is a host fixture;
it does not simulate front-panel electronics or STM32 interrupt timing.
"""
from pathlib import Path
import hashlib
import re
import subprocess

root = Path(__file__).resolve().parents[2]
out = root / "build/r23-fx-route-ui"
out.mkdir(parents=True, exist_ok=True)
menu_text = (root / "Core/Inc/menu/MenuText.h").read_text()
menu_source = (root / "Core/Src/menu/menu.c").read_text()
midi_source = (root / "Core/Src/Soundengine/MIDI/MidiParser.c").read_text()
message_source = (root / "Core/Inc/Soundengine/MIDI/MidiMessages.h").read_text()
mixer_source = (root / "Core/Inc/Soundengine/DSPAudio/mixer.h").read_text()

expected = ["St1", "St2", "L1", "R1", "L2", "R2", "FX", "DLY", "RNG", "DRV", "CMP"]
m = re.search(r"const char outputNames\[\]\[7\]\s*=\s*\{(.*?)\};", menu_text, re.S)
assert m, "Production voice output menu table not found"
body = m.group(1)
values = re.findall(r'\{\s*"([^"]*)"\s*\}', body)
meta = re.search(r"\{\s*(\d+)\s*\}", body)
assert meta and int(meta.group(1)) == len(expected), "Menu range is not 0..10"
assert values == expected, f"Unexpected output labels or ordering: {values!r}"
fx_menu = re.search(r"const char outputNamesFx\[\]\[7\]\s*=\s*\{(.*?)\};", menu_text, re.S)
assert fx_menu and re.search(r"\{\s*6\s*\}", fx_menu.group(1)), "Per-FX output must remain 0..5"
assert len(re.findall(r'\{\s*"[^"]+"\s*\}', fx_menu.group(1))) == 6
voice_menu_entries = re.findall(
    r"/\*PAR_AUDIO_OUT[1-6]\*/\s*DTYPE_MENU\s*\|\s*\(MENU_AUDIO_OUT<<8\)",
    menu_source)
assert len(voice_menu_entries) == 6, f"Only {len(voice_menu_entries)} six-voice menu descriptors"
assert re.search(r"case MENU_AUDIO_OUT:\s*return outputNames\[0\]\[0\];", menu_source)
assert re.search(r"case MENU_AUDIO_OUT:\s*p\s*=\s*outputNames\[curParmVal\s*\+\s*1\];", menu_source)
assert "memcpy(buf,p,3);" in menu_source, "Unexpected display-width contract"
assert "MENU_AUDIO_OUT_FX" in menu_source
for constant in ["MIXER_ROUTING_FX_BUS", "MIXER_ROUTING_FX_DELAY", "MIXER_ROUTING_FX_RING",
                 "MIXER_ROUTING_FX_DISTORTION", "MIXER_ROUTING_FX_COMPRESSOR"]:
    assert constant in mixer_source, f"Missing output route enum {constant}"

# Extract the existing menu's encoder clamp, unchanged.
clamp_match = re.search(
    r"(if\(\*paramValue >= numEntries\)\s*\*paramValue = \(uint8_t\)\(numEntries-1\);)",
    menu_source)
assert clamp_match, "Source encoder value clamp not present"
clamp_body = clamp_match.group(1)

# Verify that menu MIDI addresses are adjacent so each voice's CC2 routes
# through the same central dispatch and the new guard.
names = [f"CC2_AUDIO_OUT{i}" for i in range(1, 7)]
enum_sequence = re.search(r"CC2_AUDIO_OUT1\s*,\s*CC2_AUDIO_OUT2\s*,\s*"
                          r"CC2_AUDIO_OUT3\s*,\s*CC2_AUDIO_OUT4\s*,\s*"
                          r"CC2_AUDIO_OUT5\s*,\s*CC2_AUDIO_OUT6\s*,", message_source)
assert enum_sequence, "Voice output CC2 addresses are not consecutive"
guard = re.search(
    r"if\(msg\.data1 >= CC2_AUDIO_OUT1 && msg\.data1 <= CC2_AUDIO_OUT6 &&\s*"
    r"value\.itg > MIXER_ROUTING_FX_COMPRESSOR\) return;",
    midi_source)
assert guard, "Missing MIDI upper-range guard"
guard_index = guard.start()
cache_index = midi_source.find("midiParser_originalCcValues[paramNr] = value.itg;", midi_source.index("else //MIDI_CC2"))
route_index = midi_source.index("mixer_audioRouting[msg.data1-CC2_AUDIO_OUT1] = value.itg;")
assert guard_index < cache_index < route_index, "MIDI guard must precede cached and live writes"
for name in names:
    assert re.search(r"\bcase " + name + r":", midi_source), "Missing MIDI switch case " + name

# The tested C helper is the exact production boolean guard, with only the
# early return adapted from 'void' to 'bool' for the host acceptance oracle.
host_guard = guard.group(0).replace("return;", "return false;")
source = r'''
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "menu/MenuText.h"
#include "Soundengine/SharedFxPool.h"
#define CC2_AUDIO_OUT1 20
#define CC2_AUDIO_OUT6 25
#define MIXER_ROUTING_FX_COMPRESSOR FX_POOL_ROUTE_COMPRESSOR
static bool midi_accept(uint8_t index,uint8_t v) {
    struct {uint8_t data1;} msg={index};
    struct {uint8_t itg;} value={v};
''' + host_guard + r'''
    return true;
}
static uint8_t menu_bound(uint8_t value) {
    uint8_t *paramValue=&value;
    const uint8_t numEntries=outputNames[0][0];
''' + clamp_body + r'''
    return value;
}
int main(void) {
    const char* labels[]={"St1","St2","L1","R1","L2","R2","FX","DLY","RNG","DRV","CMP"};
    const uint8_t types[]={255,255,255,255,255,255,FX_POOL_TYPE_DISTORTION,
                           FX_POOL_TYPE_DELAY,FX_POOL_TYPE_RING,
                           FX_POOL_TYPE_DISTORTION,FX_POOL_TYPE_COMPRESSOR};
    unsigned checks=0;
    if (sizeof(outputNames)/sizeof(outputNames[0]) != 12 ||
        outputNames[0][0] != 11 || outputNamesFx[0][0] != 6) return 10;
    for(unsigned route=0;route<=10;route++) {
        if(strncmp(outputNames[route+1],labels[route],3)) return 11;
        if(SharedFxPool_typeForRoute((uint8_t)route,FX_POOL_TYPE_DISTORTION) != types[route]) return 12;
        checks+=2;
    }
    for(unsigned value=0;value<128;value++) {
        uint8_t bounded=(uint8_t)(value<=10?value:10);
        if(menu_bound((uint8_t)value)!=bounded) return 13;
        checks++;
        for(unsigned index=20;index<=25;index++) {
            if(midi_accept((uint8_t)index,(uint8_t)value)!=(value<=10)) return 14;
            checks++;
        }
        if(!midi_accept(19,(uint8_t)value) || !midi_accept(26,(uint8_t)value)) return 15;
        checks+=2;
    }
    uint8_t routes[6]={FX_POOL_ROUTE_DELAY,FX_POOL_ROUTE_RING,
       FX_POOL_ROUTE_DISTORTION,FX_POOL_ROUTE_COMPRESSOR,FX_POOL_ROUTE_LEGACY,0};
    if(SharedFxPool_activeMask(routes,FX_POOL_TYPE_DISTORTION)!=0x1e) return 16;
    puts("PASS: 11 voice routing values 0..10, four direct-FX names, per-FX outputs unchanged 0..5");
    printf("PASS: %u routing/menu/CC2 checks, six voice channels, invalid external values rejected\n",checks);
    return 0;
}
'''
test_path = out / "voice_route_menu_exact_source.c"
test_path.write_text(source)
(out / "INPUT_SHA256SUMS.txt").write_text("\n".join(
    f"{hashlib.sha256(t.encode()).hexdigest()}  {name}"
    for name,t in [("MenuText.h",menu_text),("menu.c",menu_source),
                   ("MidiParser.c",midi_source),("MidiMessages.h",message_source),
                   ("mixer.h",mixer_source)]
) + "\n")
for sanitizer,flags in (("ubsan",["-fsanitize=undefined,bounds"]),
                        ("asan",["-fsanitize=address"])):
    exe = out / sanitizer
    cmd = ["cc","-std=gnu11","-O1","-g","-Wall","-Wextra","-Werror",
           "-fno-omit-frame-pointer","-fno-sanitize-recover=all",
           "-I" + str(root / "Core/Inc"),*flags,str(test_path),"-o",str(exe)]
    build = subprocess.run(cmd,capture_output=True,text=True)
    (out / (sanitizer+"_build.log")).write_text(build.stdout+build.stderr)
    if build.returncode:
        print("STOP compile:",sanitizer,build.stderr[-8000:])
        raise SystemExit(build.returncode)
    run = subprocess.run([str(exe)],capture_output=True,text=True)
    (out / (sanitizer+"_run.log")).write_text(run.stdout+run.stderr)
    print(sanitizer,"exit",run.returncode,run.stdout,run.stderr[-3500:],flush=True)
    if run.returncode:raise SystemExit(run.returncode)
print("PASS exact-source R23 route menu/MIDI value-domain host fixtures")
