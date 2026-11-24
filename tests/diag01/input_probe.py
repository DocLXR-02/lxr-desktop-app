#!/usr/bin/env python3
"""Compile real dispatcher/subpage/ADC/dequeue bodies with bounded host adapters.
No hardware, ISR concurrency, full button dispatcher or real voice-transfer engine."""
from pathlib import Path
import re, subprocess, sys
root=Path(__file__).resolve().parents[2]
out=root/"build/r25-diag01-evidence/input-probe"
out.mkdir(parents=True,exist_ok=True)
def func(path,name):
    s=(root/path).read_text()
    m=re.search(r"^(?:void|uint16_t|int8_t)\s+"+name+r"\s*\([^;{}]*\)\s*\{",s,re.M)
    assert m,name
    depth=1
    for i in range(m.end(),len(s)):
        if s[i]=="{":depth+=1
        elif s[i]=="}":
            depth-=1
            if not depth:return s[m.start():i+1]
    raise AssertionError(name)
parts=[("Core/Src/encoder.c","encode_read4"),
       ("Core/Src/menu/menu.c","menu_parseEncoder"),
       ("Core/Src/menu/menu.c","menu_switchSubPage"),
       ("Core/Src/IO/adcPots.c","calcPosition"),
       ("Core/Src/IO/adcPots.c","adc_checkPots")]
(out/"production.inc").write_text("\n\n".join(func(*p) for p in parts)+"\n")
# Use actual menu/encoder source, and ensure fixture reflects CONFIG layout
pages=(root/"Core/Inc/menu/menuPages.h").read_text()
section=pages.split("// MENU_MIDI_PAGE",1)[1].split("//page 8",1)[0]
assert "PAR_BPM" in section and "PAR_FOLLOW" in section
assert re.search(r"PAR_PATTERN_MODE,\s*PAR_PRESCALER_CLOCK_IN,\s*PAR_PRESCALER_CLOCK_OUT1,\s*PAR_NONE,\s*PAR_NONE,\s*PAR_NONE,\s*PAR_NONE,\s*PAR_NONE",section)
encoder=(root/"Core/Src/encoder.c").read_text()
assert re.search(r"#define\s+MAX_PENDING_DETENTS\s+4\b",encoder)
buttons=(root/"Core/Src/buttonHandler.c").read_text()
assert buttons.index("menu_diagClose();",buttons.index("void buttonHandler_buttonPressed")) < buttons.index("switch(buttonNr)",buttons.index("void buttonHandler_buttonPressed"))
assert re.search(r"if\(menu_diagConfig\(\)\) break;\s*#endif\s*menu_switchPage\(MENU_MIDI_PAGE\);",buttons)
# Execute same fixture against old overlay as a negative control.
old=subprocess.check_output(["git","show","348eeb7203c97a86d67f909ec3fcd3773a95efd0:Core/Src/menu/diagnosticsMenu.inc"],cwd=root)
(out/"old").mkdir(exist_ok=True)
(out/"old/diagnosticsMenu.inc").write_bytes(old)
for label,inc in (("fixed",root/"Core/Src/menu"),("old",out/"old")):
    exe=out/label if label=="fixed" else out/"old-probe"
    cmd=["cc","-std=c11","-O2","-Wall","-Wextra","-Werror","-fsanitize=undefined,bounds","-fno-sanitize-recover=all",
         "-DLXR_RUNTIME_DIAGNOSTICS=1","-I"+str(inc),"-I"+str(out),"-I"+str(root/"tests/diag01"),"-I"+str(root/"Core/Inc"),
         str(root/"Core/Src/safety/runtimeDiagnostics.c"),str(root/"tests/diag01/input_paths.c"),"-o",str(exe)]
    print("Compile",label,flush=True)
    subprocess.run(cmd,check=True)
    result=subprocess.run([str(exe)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    print(result.stdout,end="",flush=True)
    if label=="fixed":assert result.returncode==0
    else:assert result.returncode==42 and "OLD QUEUE LEAK" in result.stdout, result
print("PASS independent negative control (old overlay rejected)")
