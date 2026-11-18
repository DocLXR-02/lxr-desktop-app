#!/usr/bin/env python3
"""Host-check exact-source R23 legacy Kit route upgrade and import gates.

Uses the actual preset manager migration function, parameter IDs and LXFX
codec. File-handling and transaction behavior are covered by inherited
FatFs/project suites; this fixture adds bytewise route conversion oracles.
"""
from pathlib import Path
import hashlib
import re
import subprocess

root=Path(__file__).resolve().parents[2]
out=root/"build/r23-legacy-kit-upgrade"
out.mkdir(parents=True,exist_ok=True)
preset=(root/"Core/Src/Preset/presetManager.c").read_text()
header=(root/"Core/Inc/Soundengine/SharedFxPool.h").read_text()
param=(root/"Core/Inc/Parameters.h").read_text()
codec=(root/"Core/Inc/Preset/FxKitFormat.h").read_text()
mixer=(root/"Core/Src/Soundengine/DSPAudio/mixer.c").read_text()
for name,body in (("presetManager.c",preset),("SharedFxPool.h",header),
                  ("Parameters.h",param),("FxKitFormat.h",codec),("mixer.c",mixer)):
    (out/(name+".sha256")).write_text(hashlib.sha256(body.encode()).hexdigest()+"  "+name+"\n")
name="preset_fxUpgradeStoredVoiceRoutes"
f=re.search(r"static\s+void\s+"+name+r"\s*\(uint8_t \*parameters\)\s*\{",preset)
assert f, "Expected actual in-place Kit migration implementation"
braces=0
body=None
for i in range(preset.index("{",f.start()),len(preset)):
    if preset[i]=="{": braces+=1
    if preset[i]=="}":
        braces-=1
        if braces==0:
            body=preset[f.start():i+1]
            break
assert body

# Every entry path must upgrade before publishing RAM and MIDI control.
assert re.search(r"preset_fxUpgradeStoredVoiceRoutes\(preset_fxReadBuffer\);\s*const uint8_t nmt=",preset)
assert re.search(r"preset_fxUpgradeStoredVoiceRoutes\(data\);\s*memcpy\(activeParameters,data",preset)
assert re.search(r"preset_fxUpgradeStoredVoiceRoutes\(mainParameters\[slot\]\);\s*preset_fxFromLegacy",preset)
assert re.search(r"preset_fxUpgradeStoredVoiceRoutes\(mainParameters\[presetNr\]\);\s*memcpy\(activeParameters",preset)
assert re.search(r"preset_fxUpgradeStoredVoiceRoutes\(activeParameters\);\s*/\* Never publish unsupported archived FX type values to the menu/MIDI view\. \*/\s*if\(activeParameters\[PAR_FX_TYPE\] >= FX_PARAMETER_BANK_COUNT\)\s*activeParameters\[PAR_FX_TYPE\] = FX_OFF;\s*const uint8_t type=",preset)
assert "preset_fxReadTail((FIL*)&SDFile,size,values,preset_fxReadBuffer)" in preset
assert "fxKit_validate(block)" in preset
# Morph tick never interpolates voice routing. Both save variants must preserve source routing.
assert "if((morph_index >= PAR_AUDIO_OUT1) && (morph_index <= PAR_AUDIO_OUT6))" in preset
assert len(re.findall(r"\(i>=PAR_AUDIO_OUT1 && i<=PAR_AUDIO_OUT6\)",preset))==2
assert "MIXER_ROUTING_FX_BUS" not in mixer
assert "FX_POOL_ROUTE_LEGACY" not in header

stub=out/"include"
stub.mkdir(exist_ok=True)
(stub/"stm32f7xx_hal.h").write_text("/* Host-only declaration stub. No HAL code linked. */\n")
program=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "Parameters.h"
#include "Soundengine/SharedFxPool.h"
#include "Preset/FxKitFormat.h"
_Static_assert(PAR_AUDIO_OUT6-PAR_AUDIO_OUT1==5, "Voice route IDs must be contiguous");
_Static_assert(FX_KIT_PAYLOAD_BYTES==24, "Existing extension layout must not change");
''' + body + r'''
static uint8_t oracle(uint8_t route,uint8_t fx,uint8_t phys)
{
 if(route==6) {
  if(fx==1)return 9;
  if(fx==2)return 8;
  if(fx==3)return 10;
  if(fx==4)return 7;
  return phys<=5?phys:0;
 }
 return route<=10?route:0;
}
int main(void) {
 unsigned long cases=0,layouts=0;
 for(unsigned fx=0;fx<256;fx++)
 for(unsigned physical=0;physical<8;physical++)
 for(unsigned offset=0;offset<64;offset++) {
    uint8_t original[END_OF_SOUND_PARAMETERS]={0};
    uint8_t migrated[END_OF_SOUND_PARAMETERS]={0};
    original[PAR_FX_TYPE]=(uint8_t)fx;
    original[PAR_FX_ROUTING]=(uint8_t)physical;
    original[PAR_FX_DRY_WET]=83;
    original[PAR_FX_P1]=99;
    for(unsigned v=0;v<6;v++) {
      /* 6, all four new effects, physical or corrupt input, across 64 Kits. */
      uint8_t route=(uint8_t)((offset+v)%13);
      original[PAR_AUDIO_OUT1+v]=route;
    }
    memcpy(migrated,original,sizeof(original));
    preset_fxUpgradeStoredVoiceRoutes(migrated);
    for(unsigned v=0;v<6;v++) {
      uint8_t expected=oracle(original[PAR_AUDIO_OUT1+v],(uint8_t)fx,(uint8_t)physical);
      if(migrated[PAR_AUDIO_OUT1+v]!=expected || expected==6) return 10;
      cases++;
    }
    /* Stable on second load; all non-route fields unchanged byte for byte. */
    uint8_t copy[sizeof(migrated)];
    memcpy(copy,migrated,sizeof(copy));
    preset_fxUpgradeStoredVoiceRoutes(copy);
    if(memcmp(copy,migrated,sizeof(copy)))return 11;
    for(unsigned i=0;i<sizeof(migrated);i++) {
      if(i>=PAR_AUDIO_OUT1 && i<=PAR_AUDIO_OUT6)continue;
      if(migrated[i]!=original[i])return 12;
    }
    uint8_t mask=SharedFxPool_activeMask(migrated+PAR_AUDIO_OUT1);
    for(unsigned v=0;v<6;v++)
      if(migrated[PAR_AUDIO_OUT1+v]==6)return 13;
    (void)mask;
    layouts++;
 }
 uint8_t ext[FX_KIT_EXTENSION_BYTES]={0},payload[FX_KIT_PAYLOAD_BYTES]={0};
 for(unsigned i=0;i<FX_KIT_PAYLOAD_BYTES;i++)payload[i]=(uint8_t)(i%6==4?75:i%6==5?3:99);
 fxKit_writeBlock(ext,payload);
 if(!fxKit_validate(ext))return 14;
 ext[7]^=1u;
 if(fxKit_validate(ext))return 15;
 /* No migration is done on the SD file itself: it is an RAM-only operation. */
 printf("PASS actual preset Kit upgrade: %lu voice checks, %lu Kit layouts, idempotent, unrelated bytes preserved\n",cases,layouts);
 puts("PASS 32-byte LXFX codec remains valid and corruption rejected; old FX bus absent from mixer");
 return 0;
}
'''
file=out/"exact_preset_migration.c"
file.write_text(program)
for label,flags in (("ubsan",["-fsanitize=undefined,bounds"]),("asan",["-fsanitize=address"])):
    exe=out/("migration_"+label)
    cmd=["cc","-std=gnu11","-O1","-g","-Wall","-Wextra","-Werror",
         "-fno-sanitize-recover=all","-fno-omit-frame-pointer",
         "-I"+str(stub),"-I"+str(root/"Core/Inc"),*flags,str(file),"-o",str(exe)]
    built=subprocess.run(cmd,text=True,capture_output=True)
    (out/(label+"_build.log")).write_text(built.stdout+built.stderr)
    if built.returncode:
       print("STOP",label,"compile",built.stderr[-8000:])
       raise SystemExit(built.returncode)
    run=subprocess.run([str(exe)],capture_output=True,text=True)
    (out/(label+"_run.log")).write_text(run.stdout+run.stderr)
    print(label,run.returncode,run.stdout,run.stderr[-2000:],flush=True)
    if run.returncode:raise SystemExit(run.returncode)
print("PASS exact-source archived voice routing conversion on both host sanitizers")
