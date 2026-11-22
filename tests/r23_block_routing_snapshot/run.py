#!/usr/bin/env python3
"""Source-extracted R23 routing snapshot host and ARM contract validation.

This compiles the unchanged production FX route helpers plus the exact new
block-capture statements, with small voice/fader stubs. It deliberately does
NOT claim to build or simulate the full sound engine / ARM firmware.
Run from a repository checkout with python3, GCC and Clang ARM target support.
"""
from pathlib import Path
import hashlib
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
src_file = root / "Core/Src/Soundengine/DSPAudio/mixer.c"
pool_header = root / "Core/Inc/Soundengine/SharedFxPool.h"
mixer = src_file.read_text()
assert pool_header.is_file(), "Missing production SharedFxPool.h"
header = pool_header.read_text()
assert "SharedFxPool_activeMask" in header and "SharedFxPool_typeForRoute" in header

function_start = "IN_ITCM_RAM void mixer_calcNextSampleBlock("
before, render = mixer.split(function_start, 1)
assert "mixer_clipMaxRoutedFaderGain" not in mixer, "Obsolete CLP fader helper remains"
assert "clpFeedbackActive" not in mixer, "Fader-controlled CLP gate remains"
clp_setting = re.search(
    r"(?s)(/\* R24: retain CLP's input/output levels and idle gate;.*?\*/\s*"
    r"fxBus\.distortion\.clipFaderFeedbackFirst = 1\.f;\s*"
    r"fxBus\.distortion\.clipFaderFeedbackStep = 0\.f;)",
    render,
)
assert clp_setting, "Production CLP feedback setting is not constant"
clp_setting = clp_setting.group(1)
voice = (root / "Core/Src/Soundengine/DSPAudio/DrumVoice.c").read_text()
assert "buf[i] *= voiceArray[voiceNr].combinedVolume;" in voice, "Voice fader volume path changed"
dsp = (root / "Core/Src/Soundengine/DSPAudio/FX/Distortion.c").read_text()
assert "CLIP_IDLE_HOLD_SAMPLES 441u" in dsp, "CLP inactivity hold changed"
assert "CLIP_FEEDBACK_FADE_STEP (1.f / 221.f)" in dsp, "CLP inactivity fade changed"

match = re.search(
    r"(?s)(uint8_t routesForBlock\[FX_POOL_VOICE_COUNT\];\s*"
    r"for \(uint8_t voice = 0; voice < FX_POOL_VOICE_COUNT; \+\+voice\)\s*"
    r"routesForBlock\[voice\] = mixer_audioRouting\[voice\];\s*"
    r"const uint8_t fxPoolMask = SharedFxPool_activeMask\(routesForBlock\);)",
    render,
)
assert match, "Production six-route snapshot not found"
capture = match.group(1)
assert render.count("mixer_audioRouting[") == 1, "Audio block still rereads a live voice route"
assert len(re.findall(r"routesForBlock\[[0-5]\]", render)) == 23, "Voice dispatches not all using snapshot"
assert render.count("fxBus.distortion.clipFaderFeedbackFirst = 1.f;") == 1
assert render.count("fxBus.distortion.clipFaderFeedbackStep = 0.f;") == 1
assert render.count("SharedFxPool_activeMask(routesForBlock)") == 1
assert "fxBuffer = fxPoolInputs[selectedType];" in mixer
assert "fxBuffer += (uint32_t)selectedType" not in mixer

c = r"""
#include <stdint.h>
extern float sqrtf(float);
#include "Soundengine/SharedFxPool.h"
#define FX_DISTORTION FX_POOL_TYPE_DISTORTION
typedef struct { struct { float b; } FaderVolumeFilter; } TestVoice;
TestVoice voiceArray[3], snareVoice, cymbalVoice, hatVoice;
uint8_t mixer_audioRouting[FX_POOL_VOICE_COUNT];
"""
typedef struct { float clipFaderFeedbackFirst, clipFaderFeedbackStep; } TestDistortion;
struct { TestDistortion distortion; } fxBus;
void r24_update_clip_control(void) {
""" + clp_setting + r"""
}
""" + r"""
uint8_t snapshot_capture(uint8_t out[FX_POOL_VOICE_COUNT]) {
""" + capture + r"""
    for (uint8_t i=0;i<FX_POOL_VOICE_COUNT;++i) out[i]=routesForBlock[i];
    return fxPoolMask;
}
#ifdef LXR_HOST_CHECK
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
static uint32_t rnd=0xabad1deau;
static uint32_t next_rnd(void) { rnd^=rnd<<13; rnd^=rnd>>17; rnd^=rnd<<5; return rnd; }
static void check(int yes, const char *what) {
    if(!yes) { fprintf(stderr, "FAIL %s\n", what); exit(1); }
}
static uint8_t expected_mask(const uint8_t routes[6]) {
    uint8_t m=0;
    for(unsigned i=0;i<6;i++) {
        switch(routes[i]) {
            case 7: m|=16; break;
            case 8: m|=4; break;
            case 9: m|=2; break;
            case 10: m|=8; break;
        }
    }
    return m;
}
int main(void) {
    const uint8_t choices[]={0,1,2,3,4,5,7,8,9,10};
    uint8_t snapshot[6]; unsigned negative=0;
    for(unsigned test=0;test<100000;test++) {
        for(unsigned i=0;i<6;i++)
            mixer_audioRouting[i]=choices[next_rnd()%10];
        uint8_t previous[6]; memcpy(previous,mixer_audioRouting,6);
        const uint8_t mask=snapshot_capture(snapshot);
        check(memcmp(previous,snapshot,6)==0,"six-byte copy");
        check(mask==expected_mask(snapshot),"independent FX mask oracle");
        for(unsigned i=0;i<6;i++)
            mixer_audioRouting[i]=choices[next_rnd()%10];
        check(mask==expected_mask(snapshot),"post-capture route stability");
        if(mask!=expected_mask(mixer_audioRouting)) ++negative;
    }
    check(negative>1000,"negative control did not detect live reread");
    /* Independent oracle: extracted production feedback coefficient remains
       unity across all fader positions, routing and 64-frame samples. */
    const float faders[]={0.f, .01f, .0625f, .25f, .5f, 1.f};
    for(unsigned routed=0;routed<2;routed++) {
        for(unsigned k=0;k<sizeof(faders)/sizeof(faders[0]);k++) {
            for(unsigned i=0;i<6;i++)mixer_audioRouting[i]=routed ? 9 : 0;
            snapshot_capture(snapshot);
            voiceArray[0].FaderVolumeFilter.b=faders[k];
            snareVoice.FaderVolumeFilter.b=1.f-faders[k];
            fxBus.distortion.clipFaderFeedbackFirst=-1.f;
            fxBus.distortion.clipFaderFeedbackStep=99.f;
            r24_update_clip_control();
            check(fxBus.distortion.clipFaderFeedbackFirst==1.f,
                  "CLP feedback unity at every Voice fader position");
            check(fxBus.distortion.clipFaderFeedbackStep==0.f,
                  "CLP feedback no longer ramps with Voice faders");
            for(unsigned sample=0;sample<64;sample++)
                check(fxBus.distortion.clipFaderFeedbackFirst+
                      sample*fxBus.distortion.clipFaderFeedbackStep==1.f,
                      "CLP feedback constant within audio block");
        }
    }
    puts("PASS: 100000 source-extracted snapshot/mask cases, CLP feedback independent of fader, negative control");
    printf("negative control detected %u post-capture divergences\n",negative);
    return 0;
}
#endif
"""

with tempfile.TemporaryDirectory(prefix="r23-snapshot-") as tmpdir:
    tmp = Path(tmpdir)
    source = tmp / "production_snapshot_contract.c"
    source.write_text(c)
    host = tmp / "snapshot-host"
    arm = tmp / "snapshot-cortex-m7.o"
    subprocess.run([
        os.environ.get("CC", "gcc"), "-std=c11", "-O1", "-g",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-Wall", "-Wextra", "-Werror", "-DLXR_HOST_CHECK",
        "-I", str(root / "Core/Inc"), str(source), "-lm", "-o", str(host)
    ], check=True)
    subprocess.run([str(host)], check=True, env={**os.environ, "ASAN_OPTIONS":"detect_leaks=0"})
    subprocess.run([
        os.environ.get("CLANG", "clang"), "-target", "arm-none-eabi",
        "-mcpu=cortex-m7", "-mthumb", "-O2", "-ffreestanding",
        "-fno-builtin", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
        "-I", str(root / "Core/Inc"), "-c", str(source), "-o", str(arm)
    ], check=True)
    assert arm.read_bytes()[:4] == b"\x7fELF", "Expected ARM ELF object"
    print("PASS: Cortex-M7 target compiler object (not full firmware ARM build)")
    print("host source SHA256:",hashlib.sha256(source.read_bytes()).hexdigest())
    print("ARM object SHA256:",hashlib.sha256(arm.read_bytes()).hexdigest())

print("SCOPE: production source-extracted contract test only; full ARM build/CI/device NOT run")
