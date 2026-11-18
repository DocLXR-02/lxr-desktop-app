#!/usr/bin/env python3
"""R23 CLP continuity across FX editor switches using actual DSP and MIDI bodies.

Requires the inherited real-four-FX integration generator to have run first.
Generates a second host program from that same source-exact real-DSP translation
unit with the production CC2 FX editor case and genuine Kit reset function.
No firmware code, DSP body, timing, or physical audio is simulated as measured.
"""
import hashlib
from pathlib import Path
import re
import subprocess
import sys

root=Path(__file__).resolve().parents[2]
out=root/"build/r23-clp-editor"
out.mkdir(parents=True,exist_ok=True)
midi_file=root/"Core/Src/Soundengine/MIDI/MidiParser.c"
fx_file=root/"Core/Src/Soundengine/FxBus.c"
preset_file=root/"Core/Src/Preset/presetManager.c"
real_file=root/"build/r23-final-four-fx-kit/all_four_real_dsp.c"
for file in [midi_file,fx_file,preset_file,real_file]:
    if not file.exists():
        raise AssertionError("Missing exact-source prerequisite: "+str(file))
    (out/(file.name+".sha256")).write_text(
        hashlib.sha256(file.read_bytes()).hexdigest()+"  "+str(file.relative_to(root))+"\n")
midi=midi_file.read_text()
fx=fx_file.read_text()
preset=preset_file.read_text()
generated=real_file.read_text()
m=re.search(
    r"(?ms)^[ \t]*case CC2_FX_TYPE:\s*\n"
    r"(?P<case>.*?)"
    r"^[ \t]*case CC2_FX_ROUTING:",
    midi,
)
assert m, "Cannot extract the production FX editor CC2 switch case"
case="case CC2_FX_TYPE:\n"+m.group("case")
assert case.count("FxBus_updateParameter();")==1
assert "menu_syncFxParameterView();" in case
assert "menu_switchSubPage(fxBus.fxType, 1 );" in case
assert "FxBus_prepareFxChange" not in case, "STOP: editor-only CLP reset still present"
assert "fxBus.fxType = value.itg;" in case

reset=re.search(
    r"(?ms)^void FxBus_prepareFxChange\(\)\s*\{\s*"
    r".*?Distortion_resetClip\(&fxBus\.distortion\);.*?"
    r"^\}",
    fx,
)
assert reset,"The explicit Kit-state reset must still exist"
assert len(re.findall(r"\bFxBus_prepareFxChange\(\s*\);",preset))==2, (
    "Both real Kit loading pathways must retain their explicit CLP reset")
assert "Distortion_resetClip" in generated
assert generated.count("int main(void) {")==1

fixture=r'''
/* The CC2 selection case below is pasted unchanged from production source. */
#define CC2_FX_TYPE 80
#define CC2_FX_ROUTING 81
#define FX_MODE_PAGE 4
static uint8_t menu_activePage, editorPage;
static unsigned menuRefreshes, menuSwitches;
static void menu_syncFxParameterView(void) { menuRefreshes++; }
static uint8_t menu_getSubPage(void) { return editorPage; }
static void menu_switchSubPage(uint8_t page, uint8_t toggle) {
    if(toggle!=1)abort();
    editorPage=page;
    menuSwitches++;
}
''' + reset.group(0) + r'''
static void select_fx_editor(uint8_t requested) {
    struct { uint8_t itg; } value={requested};
    switch(CC2_FX_TYPE) {
''' + case + r'''
    case CC2_FX_ROUTING:
        break;
    default:
        abort();
    }
    if(fxBus.fxType!=requested || fxBus.parameterBankType!=requested)
        abort();
    if(menu_activePage==FX_MODE_PAGE && editorPage!=requested)
        abort();
}

#define EDITOR_BLOCKS 22
struct EditorTrace {
    MasterDistortion clip[EDITOR_BLOCKS];
    int16_t dac1[EDITOR_BLOCKS][OUTPUT_DMA_SIZE*2];
    int16_t dac2[EDITOR_BLOCKS][OUTPUT_DMA_SIZE*2];
};
static void render_editor_trace(bool switching, bool editorVisible,struct EditorTrace *trace) {
    static const uint8_t pages[]={FX_RING,FX_DELAY,FX_OFF,FX_COMPRESSOR,FX_DISTORTION,FX_RING};
    unsigned pageIndex=0;
    state_init(0,DELAY_NORMAL,REC);
    seed=0x1badc0deu;
    menu_activePage=editorVisible?FX_MODE_PAGE:0;
    editorPage=FX_DISTORTION;
    menuRefreshes=menuSwitches=0;
    fxBus.fxType=FX_DISTORTION;
    fxBus.parameterBankType=FX_DISTORTION;
    fxBus.bankParameters[FX_DISTORTION][0]=fxBus.distortion.drive;
    fxBus.bankParameters[FX_DISTORTION][1]=fxBus.distortion.p1;
    fxBus.bankParameters[FX_DISTORTION][2]=fxBus.distortion.p2;
    fxBus.bankParameters[FX_DISTORTION][3]=fxBus.distortion.p3;
    memcpy(fxBus.parameter,fxBus.bankParameters[FX_DISTORTION],sizeof(fxBus.parameter));
    fxBus.dryWet=fxBus.bankDryWet[FX_DISTORTION];
    fxBus.audioRouting=fxBus.bankAudioRouting[FX_DISTORTION];
    const uint8_t mask=voice_routes(1);
    if(mask!=(1u<<FX_DISTORTION))abort();
    for(unsigned block=0;block<EDITOR_BLOCKS;block++) {
        if(switching && block>=3 && block<=18 && ((block-3)%3)==0) {
            select_fx_editor(pages[pageIndex++]);
        }
        make_inputs(1,(int)block,8000);
        process_active(mask,stereoOut2,stereoOut1);
        if(!fxBus.distortion.clipStateActive || fxBus.distortion.clipFeedbackGain<=0.f) {
            fprintf(stderr,"STOP CLP not active block=%u\n",block);abort();
        }
        trace->clip[block]=fxBus.distortion;
        memcpy(trace->dac1[block],stereoOut1,sizeof(stereoOut1));
        memcpy(trace->dac2[block],stereoOut2,sizeof(stereoOut2));
    }
    if(switching) {
        if(pageIndex!=6)abort();
        if(editorVisible) {
            if(menuRefreshes!=6 || menuSwitches!=6)abort();
        } else if(menuRefreshes!=0 || menuSwitches!=0)abort();
    }
}
static void verify_editor_continuity(void) {
    unsigned long scenes=0;
    for(unsigned visible=0;visible<2;visible++) {
        struct EditorTrace *reference=calloc(1,sizeof(*reference));
        struct EditorTrace *switched=calloc(1,sizeof(*switched));
        if(!reference || !switched)abort();
        render_editor_trace(false,visible!=0,reference);
        render_editor_trace(true,visible!=0,switched);
        for(unsigned n=0;n<EDITOR_BLOCKS;n++) {
            if(memcmp(&reference->clip[n],&switched->clip[n],sizeof(MasterDistortion)) ||
                memcmp(reference->dac1[n],switched->dac1[n],sizeof(reference->dac1[n])) ||
                memcmp(reference->dac2[n],switched->dac2[n],sizeof(reference->dac2[n]))) {
                fprintf(stderr,"STOP editor switched CLP runtime/output block=%u visible=%u\n",n,visible);
                exit(1);
            }
            scenes++;
        }
        free(reference);free(switched);
    }
    /* Genuine Kit reload retains its independent explicit reset operation. */
    if(!fxBus.distortion.clipStateActive)abort();
    FxBus_prepareFxChange();
    if(fxBus.distortion.clipStateActive) {
        fputs("STOP Kit reset no longer stops CLP feedback state\n",stderr);
        exit(1);
    }
    printf("PASS R23 editor/CLP: %lu paired real-DSP 64-frame snapshots; "
           "visible and background editor; explicit Kit reset retained\n",scenes);
}
'''
augmented=generated.replace("int main(void) {",
    fixture+"\nint main(void) {\n verify_editor_continuity();\n",1)
cfile=out/"clp_editor_real_dsp.c"
cfile.write_text(augmented)
(out/"PIN.txt").write_text(
    subprocess.check_output(["git","rev-parse","HEAD"],cwd=root,text=True).strip()+
    "\nActual R23 MIDI FX editor case, FxBus Kit reset and four original FX DSP bodies.\n")
print("Generated editor CLP integration source",len(augmented.encode()),"bytes",
      hashlib.sha256(augmented.encode()).hexdigest(),flush=True)
for name,flags in [
    ("ubsan",["-fsanitize=undefined,bounds","-fno-sanitize-recover=all"]),
    ("asan",["-fsanitize=address","-fno-sanitize-recover=all"]),
]:
    exe=out/("clp_editor_"+name)
    cmd=["cc","-std=gnu11","-O1","-g","-fno-strict-aliasing","-fno-omit-frame-pointer",
         "-Wno-unused-parameter","-Wno-unused-function","-I"+str(root/"Core/Inc"),
         *flags,str(cfile),"-lm","-o",str(exe)]
    compiled=subprocess.run(cmd,text=True,capture_output=True)
    (out/(name+"_build.log")).write_text(compiled.stdout+compiled.stderr)
    if compiled.returncode:
        print("STOP compile",name,compiled.stderr[-6500:])
        sys.exit(compiled.returncode)
    run=subprocess.run([str(exe)],text=True,capture_output=True)
    (out/(name+"_run.log")).write_text(run.stdout+run.stderr)
    print(name,"rc",run.returncode,run.stdout,run.stderr[-3500:],flush=True)
    if run.returncode:sys.exit(run.returncode)
print("PASS source-exact editor navigation preserves actual R18 CLP output and runtime under both sanitizers")
