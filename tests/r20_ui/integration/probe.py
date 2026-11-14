#!/usr/bin/env python3
"""Compile selected, unmodified R20 function bodies with bounded host adapters."""
from pathlib import Path
import re
import subprocess
import sys

root = Path(sys.argv[1]).resolve()
here = Path(__file__).resolve().parent
out = Path(sys.argv[2]).resolve()
out.mkdir(parents=True, exist_ok=True)

def source(path):
    return (root / path).read_text()

def block(text, start):
    match = re.search(start, text, re.M)
    if not match:
        raise ValueError(start)
    depth = 1
    for index in range(match.end(), len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if not depth:
                return text[match.start():index + 1]
    raise ValueError(start)

def function(text, name):
    return block(text, rf"^(?:static )?(?:__attribute__\(\(noinline\)\) )?(?:void|uint8_t)\s+{name}\s*\([^;{{}}]*\)\s*\{{")

def interval(text, start, stop):
    i = text.index(start)
    return text[i:text.index(stop, i + len(start))]

def selected(text, names):
    return [function(text, name) for name in names.split()]

def enum(text, name):
    return block(text, rf"^enum {name}\s*\{{") + ";"

menu = source("Core/Src/menu/menu.c")
menu_h = source("Core/Inc/menu/menu.h")
seq = source("Core/Src/Soundengine/Sequencer/sequencer.c")
seq_h = source("Core/Inc/Soundengine/Sequencer/sequencer.h")
cc = source("Core/Src/menu/Cc2Text.c")
cortex = source("Core/Src/Soundengine/MIDI/CortexParser.c")
buttons = source("Core/Src/buttonHandler.c")
pars = source("Core/Inc/Parameters.h")
midi = source("Core/Inc/Soundengine/MIDI/MidiMessages.h")
steps = source("Core/Inc/Preset/SeqStep.h")
menu_text = source("Core/Inc/menu/MenuText.h")
number_defs = "\n".join(re.search(rf"^#define\s+{key}\s+[^\n]+", seq_h if key != "NO_AUTOMATION" else midi, re.M).group()
                        for key in ("NUM_TRACKS", "NUM_PATTERN", "NUM_STEPS", "NO_AUTOMATION"))
notice_defs = re.search(r"^#define DISPLAY_NOTICE_DURATION_MS[^\n]+", menu_h, re.M).group()
tables = [block(menu, r"^const Name valueNames\[NUM_NAMES\]\s*=\s*\{") + ";",
          block(menu_text, r"^const char shortNames\[\]\[4\]\s*=\s*\{") + ";",
          block(cc, r"^const ModTarg modTargets\[\]\s*=\s*\{") + ";"]
type_defs = [block(menu_h, rf"^typedef struct {tag}\s*\{{") + f" {name};"
             for tag, name in (("NameStruct", "Name"), ("ModTargStruct", "ModTarg"))]
type_defs += [block(steps, r"^typedef struct StepStruct\s*\{") + " Step;"]
names = [m.group(1) for m in re.finditer(r"^enum\s+(\w+)", menu_h, re.M)]
print("Menu header enums:", " ".join(names))
menu_enums = [enum(menu_h, name) for name in names
              if name.lower().startswith(("names", "long", "short", "cat")) or name == "PageNames"]
save_cases = interval(menu, "\telse if(menu_activePage == PATTERN_SAVE_PAGE_OK)", "\telse if(menu_activePage == MENU_LAST_STEP_PAGE)")
save_renderer = "static void menu_repaintSimplePage(void) { if(0) {}\n" + save_cases + "\n}"
front_cases = interval(cortex, "    case FRONT_SET_P1_DEST:", "\tcase FRONT_SET_P2_VAL:")
front_cases += interval(cortex, "\tcase FRONT_SET_P2_VAL:", "\tcase FRONT_ARM_AUTOMATION_STEP:")
if "case FRONT_SET_P2_VAL" not in front_cases:
    front_cases += block(cortex, r"^\tcase FRONT_SET_P2_VAL:\s*\{") + "\n break;\n"
front_rec = interval(cortex, "\tcase FRONT_SEQ_REC_ON_OFF:", "\tcase FRONT_SEQ_ERASE_ON_OFF:")
front_rec += interval(cortex, "\tcase FRONT_SEQ_SELECT_ACTIVE_STEP:", "\tcase FRONT_SEQ_SET_AUTOM_TRACK:")
front_rec += interval(cortex, "\tcase FRONT_SEQ_SET_ACTIVE_TRACK:", "\tcase FRONT_SEQ_REQUEST_STEP_PARAMS:")
live_cc = interval(cortex, "\tcase FRONT_CC_LIVE_LOW:", "\tcase FRONT_CC_LFO_TARGET:")
front_dispatch = """static void frontParser_handleMidiMessage(void) {
    if(cortex_midiMsg.status == SEQ_CC) {
        switch(cortex_midiMsg.data1) {
""" + front_rec + """default: abort();
        }
        return;
    }
    switch(cortex_midiMsg.status) {
""" + live_cc + front_cases + """default: abort();
    }
}"""
manual_menu = "static void manual_menu(uint16_t paramNr, uint8_t value) { switch(paramNr) {\n" + interval(menu, "\tcase PAR_P1_DEST:", "/*\n\tcase PAR_QUANTISATION:") + "\n} }"
# Use the panel's original parameter bytes and let CortexParser translate them.
knob_send = """static void knob(uint16_t paramNr, uint8_t dtypeValue) {
    activeParameters[PAR_REC_MODE] = 1;
""" + interval(menu, "\t\tif(paramNr<128) // => Sound Parameter below 128",
               "\t\telse\n\t\t\tmenu_parseGlobalParam(paramNr,dtypeValue);") + "\n}"
rec_button = "static void press_rec(void) { switch(BUT_REC) {\n" + interval(buttons, "\tcase BUT_REC:", "\tcase BUT_COPY:") + "\n} }"
button_vars = block(buttons, r"^volatile struct\s*\{") + " buttonHandler_stateMemory;"
menu_vars = interval(menu, "static volatile uint32_t menu_saveSince;", "uint8_t menu_shownPattern = 0;")
baseline_seq = subprocess.check_output(
    ["git", "-C", str(root), "show",
     "e82fd4f3c239bfcb21400c3f94756f6fced02586:Core/Src/Soundengine/Sequencer/sequencer.c"],
    text=True)
baseline_guard = function(baseline_seq, "seq_canAssignAutomationDestination").replace(
    "seq_canAssignAutomationDestination(", "fix8_canAssignAutomationDestination(", 1)
seq_functions = selected(seq, "seq_isSharedAutomationTarget seq_findAutomationConflict seq_canAssignAutomationDestination seq_assignAutomationStep seq_autoDisplayLaneTarget seq_autoExistingTarget seq_autoRecordStepTick seq_setAutomationRecordMode seq_setRecordingMode seq_isAutomationRecordMode seq_isAutomationStepArmed seq_recordAutomation seq_clearAutomation seq_setRunning seq_addNote")
menu_functions = selected(menu, "upr_three menu_repaintAll menu_autoShortName menu_publishAutomationNotice menu_showSaveSuccess menu_showAutomationOverview menu_showAutomationBlocked menu_renderAutomationNotice menu_showKitLoad menu_renderKitLoad menu_expireSdError menu_renderSdError menu_showSdError menu_repaint sendDisplayBuffer menu_tick")
menu_functions = ["static void upr_three(char *buf);", *menu_functions]
code = "\n\n".join([
    (here / "adapters.h").read_text(), number_defs, notice_defs, enum(pars, "ParamEnums"), enum(source("Core/Inc/buttonHandler.h"), "ButtonNumbers"),
    *menu_enums, enum(menu_h, "MenuAutomationBlock"), *type_defs, *tables, menu_vars,
    button_vars, (here / "state.h").read_text(),
    *selected(cc, "getNumModTargets paramToModTargetInit"),
    "static uint8_t menu_renderVoiceTransfer(void) { return 0; }\nstatic void menu_expireVoiceTransfer(void) {}",
    save_renderer, *menu_functions, *selected(buttons, "buttonHandler_syncRecordingState"),
    *seq_functions, baseline_guard, *selected(cortex, "frontParser_sendAutomationDestination"), front_dispatch, *selected(cortex, "frontParser_parseUartData"),
    *selected(source("Core/Src/frontPanelParser.c"), "frontPanel_sendData"), rec_button, manual_menu, knob_send,
    (here / "cases.c").read_text(),
])
generated = out / "probe.c"
generated.write_text(code)
exe = out / "probe"
command = ["cc", "-std=c11", "-g", "-O1", "-Wall", "-Wextra", "-Werror",
           "-Wno-tautological-compare", "-Wno-unused-function", "-Wno-unused-parameter",
           "-fsanitize=undefined,bounds", "-fno-sanitize-recover=all",
           "-I" + str(here / "host_include"), "-I" + str(root / "Core/Inc"), str(generated),
           str(root / "Core/Src/Soundengine/Sequencer/automationCapture.c"), "-o", str(exe)]
print("Compile:", " ".join(command), flush=True)
subprocess.run(command, check=True)
subprocess.run([str(exe)], check=True)
