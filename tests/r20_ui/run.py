#!/usr/bin/env python3
"""Compile and execute the real R20 conflict lookup with synthetic pattern data."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
sequencer = (root / "Core/Src/Soundengine/Sequencer/sequencer.c").read_text()
menu = (root / "Core/Src/menu/menu.c").read_text()
menu_h = (root / "Core/Inc/menu/menu.h").read_text()
button = (root / "Core/Src/buttonHandler.c").read_text()
main = (root / "Core/Src/main.c").read_text()

def extract(name):
    match = re.search(r"static\s+uint8_t\s+" + re.escape(name) + r"\s*\(", sequencer)
    if not match:
        raise AssertionError(f"Missing function {name}")
    start = match.start()
    open_brace = sequencer.index("{", match.end())
    depth = 0
    for pos in range(open_brace, len(sequencer)):
        if sequencer[pos] == "{":
            depth += 1
        elif sequencer[pos] == "}":
            depth -= 1
            if depth == 0:
                return sequencer[start:pos + 1]
    raise AssertionError(f"Unclosed function {name}")

assert "#define DISPLAY_NOTICE_DURATION_MS 1500u" in menu_h
assert "HAL_GetTick() - sucessTimer" in menu
assert "sucessTimer = HAL_GetTick();" in menu
assert "sucessTimer = HAL_GetTick();" in button
assert "systick_ticks - sucessTimer" not in menu
assert ">= 750u" not in menu
assert 'lcd_string("LXR LAB R20");' in main
assert "menu_renderAutomationNotice()) return;" in menu
assert "buttonHandler_syncRecordingState(0);" in sequencer
assert "menu_showAutomationBlocked" in sequencer
assert "menu_showAutomationOverview" in sequencer
for line in ("PARAM BLOCKED!", "trk1: COA ONLY", "trk1 MULTI PARAM",
             "P4 on V5 trk1", "REC trk1 OFF", "    trk2 COA"):
    assert len(line) <= 16, (line, len(line))

shared = re.search(r"static\s+uint8_t\s+seq_isSharedAutomationTarget\b.*?^\}", sequencer, re.S | re.M)
assert shared is not None
assert len(re.findall(r"^\s*case PAR_", shared.group(), re.M)) == 12

preamble = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NO_AUTOMATION 255u
#define NUM_PATTERN 1u
#define NUM_TRACKS 7u
#define NUM_STEPS 64u
#define MENU_AUTO_BLOCK_OTHER_LANE 1u
#define MENU_AUTO_BLOCK_LANE_LOCKED 2u
#define MENU_AUTO_BLOCK_MULTI 3u
#define MENU_AUTO_BLOCK_GLOBAL 4u
enum {
    PAR_COARSE1 = 20, PAR_FINE1 = 21, PAR_COARSE2 = 22,
    PAR_VOICE_DECIMATION_ALL = 200,
    PAR_FX_TYPE, PAR_FX_ROUTING, PAR_FX_DRY_WET, PAR_FX_DIST_TYPE,
    PAR_FX_P1, PAR_FX_P2, PAR_FX_P3, PAR_FX_P4, PAR_FX_RATIO,
    PAR_FX_WAVE, PAR_FX_DELAY_TYPE
};
typedef struct { uint8_t param1Nr, param2Nr; } Step;
static struct { Step seq_subStepPattern[NUM_PATTERN][NUM_TRACKS][NUM_STEPS]; } seq_patternSet;
"""
tests = r"""
static void reset_pattern(void)
{
    for(unsigned v = 0; v < NUM_TRACKS; ++v)
        for(unsigned s = 0; s < NUM_STEPS; ++s) {
            seq_patternSet.seq_subStepPattern[0][v][s].param1Nr = NO_AUTOMATION;
            seq_patternSet.seq_subStepPattern[0][v][s].param2Nr = NO_AUTOMATION;
        }
}
static int check(uint8_t dest, uint8_t want, uint8_t expectedVoice,
                 uint8_t expectedLane, uint8_t expectedTarget)
{
    uint8_t voice = 77, lane = 77, locked = NO_AUTOMATION;
    const uint8_t got = seq_findAutomationConflict(0, 0, 0, dest,
                                                   &voice, &lane, &locked);
    if(got != want || (want == MENU_AUTO_BLOCK_GLOBAL &&
       (voice != expectedVoice || lane != expectedLane)) ||
       (want == MENU_AUTO_BLOCK_OTHER_LANE && lane != expectedLane) ||
       (want == MENU_AUTO_BLOCK_LANE_LOCKED && locked != expectedTarget)) {
        fprintf(stderr, "Conflict mismatch got=%u want=%u voice=%u lane=%u lock=%u\n",
                got, want, voice, lane, locked);
        return 0;
    }
    return 1;
}
int main(void)
{
    const uint8_t global_targets[] = {
        PAR_VOICE_DECIMATION_ALL, PAR_FX_TYPE, PAR_FX_ROUTING,
        PAR_FX_DRY_WET, PAR_FX_DIST_TYPE, PAR_FX_P1, PAR_FX_P2,
        PAR_FX_P3, PAR_FX_P4, PAR_FX_RATIO, PAR_FX_WAVE, PAR_FX_DELAY_TYPE
    };
    for(unsigned i = 0; i < sizeof(global_targets); ++i) {
        reset_pattern();
        if(!seq_isSharedAutomationTarget(global_targets[i])) return 1;
        seq_patternSet.seq_subStepPattern[0][4][63].param2Nr = global_targets[i];
        if(!check(global_targets[i], MENU_AUTO_BLOCK_GLOBAL, 4, 1, 255)) return 2;
    }
    if(seq_isSharedAutomationTarget(PAR_COARSE1)) return 3;
    reset_pattern();
    if(!check(PAR_COARSE1, 0, 0, 0, 255)) return 4;
    seq_patternSet.seq_subStepPattern[0][4][10].param1Nr = PAR_COARSE1;
    if(!check(PAR_COARSE1, 0, 0, 0, 255)) return 5;
    seq_patternSet.seq_subStepPattern[0][0][20].param2Nr = PAR_COARSE1;
    if(!check(PAR_COARSE1, MENU_AUTO_BLOCK_OTHER_LANE, 0, 1, 255)) return 6;
    reset_pattern();
    seq_patternSet.seq_subStepPattern[0][0][0].param1Nr = PAR_COARSE1;
    if(!check(PAR_FINE1, MENU_AUTO_BLOCK_LANE_LOCKED, 0, 0, PAR_COARSE1)) return 7;
    if(seq_autoDisplayLaneTarget(0, 0, 0) != PAR_COARSE1) return 8;
    seq_patternSet.seq_subStepPattern[0][0][1].param1Nr = PAR_FINE1;
    if(seq_autoDisplayLaneTarget(0, 0, 0) != 254u) return 9;
    reset_pattern();
    seq_patternSet.seq_subStepPattern[0][4][2].param2Nr = PAR_FX_P4;
    seq_patternSet.seq_subStepPattern[0][0][0].param1Nr = PAR_FX_P4;
    if(!check(PAR_FX_P4, 0, 0, 0, 255)) return 10;
    reset_pattern();
    if(seq_autoDisplayLaneTarget(0, 0, 0) != NO_AUTOMATION) return 11;
    puts("R20 actual-source conflict checks: PASS");
    return 0;
}
"""
test_src = preamble + "\n".join(extract(name) for name in (
    "seq_isSharedAutomationTarget",
    "seq_findAutomationConflict",
    "seq_autoDisplayLaneTarget"
)) + "\n" + tests

with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / "r20_conflicts.c"
    output = Path(directory) / "r20_conflicts"
    source.write_text(test_src)
    subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=undefined,bounds", "-fno-sanitize-recover=all",
                    str(source), "-o", str(output)], check=True)
    subprocess.run([str(output)], check=True)
print("R20 notice duration, LCD bounds and live source assertions: PASS")
