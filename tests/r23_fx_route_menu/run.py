#!/usr/bin/env python3
"""R23 direct-FX voice menu, encoder, pot and external CC2 control tests.

The test extracts actual production guards/limits and compiles them with the
real routing and label headers. STM32 timing and physical controls are not
simulated. It must not accept the reserved on-disk legacy routing number 6.
"""
from pathlib import Path
import hashlib
import re
import subprocess

root = Path(__file__).resolve().parents[2]
out = root / "build/r23-fx-route-ui"
out.mkdir(parents=True,exist_ok=True)
inputs = {name:(root/name).read_text() for name in (
    "Core/Inc/menu/MenuText.h",
    "Core/Src/menu/menu.c",
    "Core/Src/Soundengine/MIDI/MidiParser.c",
    "Core/Inc/Soundengine/MIDI/MidiMessages.h",
    "Core/Inc/Soundengine/DSPAudio/mixer.h",
    "Core/Src/Soundengine/DSPAudio/mixer.c",
    "Core/Inc/Soundengine/SharedFxPool.h"
)}
menu_text=inputs["Core/Inc/menu/MenuText.h"]
menu=inputs["Core/Src/menu/menu.c"]
midi=inputs["Core/Src/Soundengine/MIDI/MidiParser.c"]
messages=inputs["Core/Inc/Soundengine/MIDI/MidiMessages.h"]
mixer=inputs["Core/Src/Soundengine/DSPAudio/mixer.c"]
expected=["St1","St2","L1","R1","L2","R2","---","DLY","RNG","DRV","CMP"]
table=re.search(r"const char outputNames\[\]\[7\]\s*=\s*\{(.*?)\};",menu_text,re.S)
assert table
values=re.findall(r'\{\s*"([^"]*)"\s*\}',table.group(1))
assert values==expected and re.search(r"\{\s*11\s*\}",table.group(1))
fx_table=re.search(r"const char outputNamesFx\[\]\[7\]\s*=\s*\{(.*?)\};",menu_text,re.S)
assert fx_table and re.search(r"\{\s*6\s*\}",fx_table.group(1))
assert len(re.findall(r'\{\s*"[^"]+"\s*\}',fx_table.group(1)))==6
assert "MIXER_ROUTING_FX_BUS" not in mixer
assert "FX_POOL_ROUTE_LEGACY" not in inputs["Core/Inc/Soundengine/SharedFxPool.h"]
assert re.search(r"case MENU_AUDIO_OUT:\s*p\s*=\s*\(curParmVal == 6u \|\| curParmVal >= outputNames\[0\]\[0\]\)",menu)
assert len(re.findall(r"/\*PAR_AUDIO_OUT[1-6]\*/\s*DTYPE_MENU\s*\|\s*\(MENU_AUDIO_OUT<<8\)",menu))==6
assert re.search(r"CC2_AUDIO_OUT1\s*,\s*CC2_AUDIO_OUT2\s*,\s*CC2_AUDIO_OUT3\s*,\s*CC2_AUDIO_OUT4\s*,\s*CC2_AUDIO_OUT5\s*,\s*CC2_AUDIO_OUT6\s*,",messages)

# Harvest one literal encoder clamp+skip sequence from the firmware.
enc=re.search(r"(if\(\*paramValue >= numEntries\)\s*\*paramValue = \(uint8_t\)\(numEntries-1\);\s*"
    r"/\* Keep direct FX route IDs 7\.\.10; skip archived destination 6\. \*/\s*"
    r"if \(menuId == MENU_AUDIO_OUT && \*paramValue == 6u\)\s*"
    r"\*paramValue = inc > 0 \? 7u : 5u;)",menu)
assert enc, "Production encoder must skip 6 in both directions"
pot=re.search(r"(if\(newValue > \(numEntries-1\)\) newValue = \(numEntries-1\);\s*"
    r"if\(menuId == MENU_AUDIO_OUT && newValue == 6u\)\s*"
    r"newValue = diff > 0 \? 7u : 5u;)",menu)
assert pot, "Production pot must skip 6 in both directions"
guard=re.search(r"(if\(msg\.data1 >= CC2_AUDIO_OUT1 && msg\.data1 <= CC2_AUDIO_OUT6 &&\s*"
    r"\(value\.itg == 6u \|\| value\.itg > MIXER_ROUTING_FX_COMPRESSOR\)\) return;)",midi)
assert guard, "External MIDI route 6 must be rejected"
idx=midi.index(guard.group(1))
cache=midi.index("midiParser_originalCcValues[paramNr] = value.itg;",midi.index("else //MIDI_CC2"))
router=midi.index("mixer_audioRouting[msg.data1-CC2_AUDIO_OUT1] = value.itg;")
assert idx<cache<router
for i in range(1,7):assert re.search(r"case CC2_AUDIO_OUT"+str(i)+r":",midi)

c_code = r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "menu/MenuText.h"
#include "Soundengine/SharedFxPool.h"
#define CC2_AUDIO_OUT1 20
#define CC2_AUDIO_OUT6 25
#define MIXER_ROUTING_FX_COMPRESSOR FX_POOL_ROUTE_COMPRESSOR
static bool midi_accept(uint8_t index,uint8_t route) {
 struct {uint8_t data1;} msg={index};
 struct {uint8_t itg;} value={route};
''' + guard.group(1).replace("return;","return false;") + r'''
 return true;
}
static uint8_t encoder_limit(uint8_t route,int8_t inc) {
 uint8_t* paramValue=&route;
 const uint8_t menuId=MENU_AUDIO_OUT;
 const uint8_t numEntries=outputNames[0][0];
''' + enc.group(1) + r'''
 return route;
}
static uint8_t pot_limit(uint8_t newValue,int8_t diff) {
 const uint8_t menuId=MENU_AUDIO_OUT;
 const uint8_t numEntries=outputNames[0][0];
''' + pot.group(1) + r'''
 return newValue;
}
int main(void) {
 const char *labels[]={"St1","St2","L1","R1","L2","R2","---","DLY","RNG","DRV","CMP"};
 const uint8_t types[]={255,255,255,255,255,255,255,4,2,1,3};
 unsigned long cases=0;
 if(outputNames[0][0]!=11 || outputNamesFx[0][0]!=6) return 10;
 for(unsigned i=0;i<11;i++) {
   if(strncmp(outputNames[i+1],labels[i],3) ||
      SharedFxPool_typeForRoute((uint8_t)i)!=types[i]) return 11;
   cases+=2;
 }
 for(unsigned i=0;i<128;i++) {
   for(int direction=-1;direction<=1;direction+=2) {
     uint8_t val=(uint8_t)(i>10?10:i);
     uint8_t expected=(uint8_t)(val==6?(direction>0?7:5):val);
     if(encoder_limit((uint8_t)i,(int8_t)direction)!=expected ||
        pot_limit((uint8_t)i,(int8_t)direction)!=expected) return 12;
     cases+=2;
   }
   for(unsigned index=20;index<=25;index++) {
     bool accepted=(i<=10 && i!=6);
     if(midi_accept((uint8_t)index,(uint8_t)i)!=accepted) return 13;
     cases++;
   }
   if(!midi_accept(19,(uint8_t)i)||!midi_accept(26,(uint8_t)i))return 14;
   cases+=2;
 }
 puts("PASS UI: six physical outputs + four explicit FX targets, archived 6 skipped");
 printf("PASS UI: %lu bounds/MIDI/FX label checks, UBSan/ASan\n",cases);
 return 0;
}
'''
c_file=out/"exact_menu_midi_host.c"
c_file.write_text(c_code)
(out/"SOURCES_SHA256SUMS.txt").write_text("\n".join(
    f"{hashlib.sha256(value.encode()).hexdigest()}  {name}" for name,value in sorted(inputs.items())
)+"\n")
for label,flags in (("ubsan",["-fsanitize=undefined,bounds"]),("asan",["-fsanitize=address"])):
    exe=out/label
    cmd=["cc","-std=gnu11","-O1","-g","-Wall","-Wextra","-Werror",
         "-fno-omit-frame-pointer","-fno-sanitize-recover=all",
         "-I"+str(root/"Core/Inc"),*flags,str(c_file),"-o",str(exe)]
    build=subprocess.run(cmd,text=True,capture_output=True)
    (out/(label+"_build.log")).write_text(build.stdout+build.stderr)
    if build.returncode:
        print("STOP compile",label,build.stderr[-8000:])
        raise SystemExit(build.returncode)
    run=subprocess.run([str(exe)],text=True,capture_output=True)
    (out/(label+"_run.log")).write_text(run.stdout+run.stderr)
    print(label,run.returncode,run.stdout,run.stderr[-3000:],flush=True)
    if run.returncode:raise SystemExit(run.returncode)
print("PASS R23 direct-only routing UI/CC2 source extraction on both sanitizers")
