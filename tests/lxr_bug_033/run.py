#!/usr/bin/env python3
"""LXR-BUG-033: source-extracted, baseline-vs-candidate MIDI event regression.

Runs actual production function bodies in a host harness, NOT STM32 firmware.
Baseline must reproduce duplicate Note On; candidate must emit exactly one.
No physical MIDI, interrupt scheduling, USB transport, or audio is simulated.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build" / "lxr-bug-033-evidence"
OUT.mkdir(parents=True, exist_ok=True)
BASE = "0e37942250f79306627702061d851731bdfdbbc8"
PATHS = {
    "voice": "Core/Src/Soundengine/MIDI/MidiVoiceControl.c",
    "header": "Core/Inc/Soundengine/MIDI/MidiVoiceControl.h",
    "seq": "Core/Src/Soundengine/Sequencer/sequencer.c",
    "parser": "Core/Src/Soundengine/MIDI/MidiParser.c",
}

def source(mode, path):
    if mode == "r22":
        return subprocess.check_output(["git", "show", f"{BASE}:{path}"], cwd=ROOT).decode()
    return (ROOT / path).read_text(encoding="utf-8")

def function(text, name):
    pattern = r"(?m)^[ \t]*(?:static[ \t]+)?void[ \t]+" + re.escape(name) + r"[ \t]*\([^;]*?\)[ \t]*\{"
    matches = list(re.finditer(pattern, text))
    assert len(matches) == 1, (name, len(matches))
    m = matches[0]
    i, depth = m.end(), 1
    while depth:
        assert i < len(text), name
        depth += (text[i] == "{") - (text[i] == "}")
        i += 1
    return text[m.start():i]

PRELUDE = r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NUM_DRUM_VOICES 7
#define USE_PATTERN_EDIT_BUFFER 0
#define STEP_VOLUME_MASK 127u
#define PAR_MIDI_NOTE1 16u
#define MIDI_FILTER_NOTES 1u
#define NOTE_ON 0x90u
#define NOTE_OFF 0x80u
typedef struct { unsigned length:2; } MidiBits;
typedef struct { uint8_t status,data1,data2; MidiBits bits; } MidiMsg;
typedef struct { uint8_t volume; } Step;
static uint8_t active_voices, midi_MidiChannels[8], activeParameters[64];
static uint8_t seq_stepIndex[7],seq_activePattern,midi_chan_notes[16];
static uint16_t midi_notes_on;
static uint8_t midiParser_txRxFilter;
static struct { Step seq_subStepPattern[1][7][1]; } seq_patternSet;
static MidiMsg usb_log[256],uart_log[256];
static unsigned usb_count,uart_count,synth[7],panel_bytes;
static void usb_sendMidi(MidiMsg msg) { assert(usb_count < 256); usb_log[usb_count++]=msg; }
static void uart_sendMidi(MidiMsg msg) { assert(uart_count < 256); uart_log[uart_count++]=msg; }
static void uart_sendFrontpanelByte(uint8_t x) { (void)x; panel_bytes++; }
static void Drum_trigger(uint8_t v,uint8_t vel,uint8_t note) { assert(v<3 && vel && note); synth[v]++; }
static void Snare_trigger(uint8_t vel,uint8_t note) { assert(vel && note); synth[3]++; }
static void Cymbal_trigger(uint8_t vel,uint8_t note) { assert(vel && note); synth[4]++; }
static void HiHat_trigger(uint8_t vel,uint8_t hh,uint8_t note) { assert(vel && note && hh<2); synth[hh+5]++; }
static void seq_sendMidi(MidiMsg);
void seq_sendMidiNoteOn(const uint8_t,const uint8_t,const uint8_t);
void seq_midiNoteOff(uint8_t);
#if CANDIDATE
void voiceControl_noteOn(uint8_t,uint8_t,uint8_t,uint8_t);
#else
void voiceControl_noteOn(uint8_t,uint8_t,uint8_t);
#endif
void voiceControl_noteOff(uint8_t);
static void reset_fixture(void) {
    active_voices=0; midi_notes_on=0; midiParser_txRxFilter=MIDI_FILTER_NOTES<<4;
    memset(midi_chan_notes,0,sizeof midi_chan_notes);
    memset(activeParameters,0,sizeof activeParameters);
    memset(synth,0,sizeof synth);
    memset(usb_log,0,sizeof usb_log); memset(uart_log,0,sizeof uart_log);
    usb_count=uart_count=panel_bytes=0;
    seq_activePattern=0; memset(seq_stepIndex,0,sizeof seq_stepIndex);
    for(unsigned v=0;v<7;v++) {
        midi_MidiChannels[v]=(uint8_t)v;
        seq_patternSet.seq_subStepPattern[0][v][0].volume=100;
    }
}
static void expect_count(unsigned n) {
    assert(usb_count==n && uart_count==n);
    for(unsigned i=0;i<n;i++) {
        assert(usb_log[i].status==uart_log[i].status);
        assert(usb_log[i].data1==uart_log[i].data1);
        assert(usb_log[i].data2==uart_log[i].data2);
    }
}
static void expect(unsigned i,unsigned status,unsigned note) {
    assert(i<usb_count);
    assert(usb_log[i].status==status && usb_log[i].data1==note);
    assert(uart_log[i].status==status && uart_log[i].data1==note);
}
"""

MAIN = r"""
int main(void) {
    unsigned cases=0;
    for(unsigned v=0;v<7;v++) {
        reset_fixture();
        activeParameters[PAR_MIDI_NOTE1+v]=48;
        seq_triggerVoice((uint8_t)v,100,63);
        assert(synth[v]==1 && (active_voices&(1u<<v)) && panel_bytes==3);
#if CANDIDATE
        expect_count(1); expect(0,NOTE_ON|v,48);
#else
        expect_count(2); expect(0,NOTE_ON|v,63); expect(1,NOTE_ON|v,48);
#endif
        voiceControl_noteOff((uint8_t)v);
#if CANDIDATE
        expect_count(2); expect(1,NOTE_OFF|v,48);
#else
        expect_count(3); expect(2,NOTE_OFF|v,48);
        /* Historical defect: Note 63 did NOT get Note Off. */
#endif
        assert((active_voices&(1u<<v))==0);
        cases++;

        reset_fixture();
        seq_triggerVoice((uint8_t)v,100,63); /* MIDI note = any */
#if CANDIDATE
        expect_count(1); expect(0,NOTE_ON|v,63);
#else
        expect_count(2); expect(0,NOTE_ON|v,63); expect(1,NOTE_ON|v,63);
#endif
        voiceControl_noteOff((uint8_t)v);
#if CANDIDATE
        expect_count(2); expect(1,NOTE_OFF|v,63);
#else
        expect_count(3); expect(2,NOTE_OFF|v,63);
#endif
        cases++;

        reset_fixture();
#if CANDIDATE
        voiceControl_noteOn((uint8_t)v,63,100,1); /* direct MIDI input */
#else
        voiceControl_noteOn((uint8_t)v,63,100);
#endif
        expect_count(1); expect(0,NOTE_ON|v,63);
        voiceControl_noteOff((uint8_t)v);
        expect_count(2); expect(1,NOTE_OFF|v,63);
        assert(synth[v]==1 && panel_bytes==3);
        cases++;
    }
    reset_fixture();
    activeParameters[PAR_MIDI_NOTE1]=48;
    seq_triggerVoice(0,100,63);
    seq_triggerVoice(0,100,63);
#if CANDIDATE
    expect_count(3);
    expect(0,NOTE_ON,48); expect(1,NOTE_OFF,48); expect(2,NOTE_ON,48);
#else
    expect_count(5);
    expect(0,NOTE_ON,63); expect(1,NOTE_ON,48);
    expect(2,NOTE_OFF,48); expect(3,NOTE_ON,63); expect(4,NOTE_ON,48);
#endif
    voiceControl_noteOff(0xff); /* STOP / all notes off */
#if CANDIDATE
    expect_count(4); expect(3,NOTE_OFF,48);
#else
    expect_count(6); expect(5,NOTE_OFF,48);
#endif
    cases++;

    reset_fixture();
    midiParser_txRxFilter=0;
    activeParameters[PAR_MIDI_NOTE1]=48;
    seq_triggerVoice(0,100,63); voiceControl_noteOff(0);
    expect_count(0); assert(synth[0]==1 && panel_bytes==3);
    cases++;

    reset_fixture();
    seq_triggerVoice(7,100,63);
    expect_count(0); assert(panel_bytes==0);
    cases++;

    reset_fixture();
    activeParameters[PAR_MIDI_NOTE1]=48;
    activeParameters[PAR_MIDI_NOTE1+1]=49;
    seq_triggerVoice(0,100,63); seq_triggerVoice(1,100,64);
    voiceControl_noteOff(0xff);
#if CANDIDATE
    expect_count(4);
    expect(0,NOTE_ON,48); expect(1,NOTE_ON|1,49);
    expect(2,NOTE_OFF,48); expect(3,NOTE_OFF|1,49);
#else
    expect_count(6);
    expect(0,NOTE_ON,63); expect(1,NOTE_ON,48);
    expect(2,NOTE_ON|1,64); expect(3,NOTE_ON|1,49);
    expect(4,NOTE_OFF,48); expect(5,NOTE_OFF|1,49);
#endif
    cases++;
    printf("PASS LXR-BUG-033 %s: %u source-extracted host cases; MIDI DIN and USB mirrored\n",
           CANDIDATE?"fixed-candidate":"original-R22-baseline",cases);
    return 0;
}
"""

def build_one(mode, sanitizer):
    inputs = {k:source(mode,p) for k,p in PATHS.items()}
    is_candidate = mode == "candidate"
    expected_decl = "void voiceControl_noteOn(uint8_t voice, uint8_t note, uint8_t vel, uint8_t sendMidi);"
    if is_candidate:
        assert expected_decl in inputs["header"]
        assert re.search(r"voiceControl_noteOn\(voice,\s*note,\s*vel,\s*1\);", inputs["parser"])
        assert "voiceControl_noteOn(voiceNr, note, vol, 0);" in inputs["seq"]
    else:
        assert "void voiceControl_noteOn(uint8_t voice, uint8_t note, uint8_t vel);" in inputs["header"]
        assert "voiceControl_noteOn(voiceNr, note, vol);" in inputs["seq"]
    functions = [
        function(inputs["seq"],"seq_sendMidi"),
        function(inputs["seq"],"seq_midiNoteOff"),
        function(inputs["seq"],"seq_sendMidiNoteOn"),
        function(inputs["voice"],"voiceControl_noteOn"),
        function(inputs["voice"],"voiceControl_noteOff"),
        function(inputs["seq"],"seq_triggerVoice"),
    ]
    source_code = "#define CANDIDATE " + str(int(is_candidate)) + "\n" + PRELUDE + "\n".join(functions) + MAIN
    outbase = OUT / f"{mode}-{sanitizer}"
    source_path = outbase.with_suffix(".c")
    source_path.write_text(source_code, encoding="utf-8")
    args = ["cc","-std=gnu11","-O1","-g","-Wall","-Wextra","-Werror",
            "-fno-omit-frame-pointer","-fno-sanitize-recover=all"]
    if sanitizer=="ubsan":
        args += ["-fsanitize=undefined,bounds"]
    elif sanitizer=="asan":
        args += ["-fsanitize=address"]
    else:
        raise AssertionError(sanitizer)
    args += [str(source_path),"-o",str(outbase)]
    compiled = subprocess.run(args, text=True, capture_output=True, cwd=ROOT)
    (OUT/f"{mode}-{sanitizer}-compile.log").write_text(compiled.stdout+compiled.stderr)
    if compiled.returncode:
        raise AssertionError(f"{mode}/{sanitizer} compile failed: {compiled.stderr[-2500:]}")
    executed = subprocess.run([str(outbase)], text=True, capture_output=True, cwd=ROOT)
    (OUT/f"{mode}-{sanitizer}-run.log").write_text(executed.stdout+executed.stderr)
    if executed.returncode:
        raise AssertionError(f"{mode}/{sanitizer} run failed: {executed.stderr[-2500:]}")
    print(executed.stdout.strip(),flush=True)
    return {"mode":mode,"sanitizer":sanitizer,"result":"PASS",
            "harness_sha256":hashlib.sha256(source_path.read_bytes()).hexdigest(),
            "stdout":executed.stdout.strip()}

def main():
    rows=[build_one(m,s) for m in ("r22","candidate") for s in ("ubsan","asan")]
    source_hashes={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in PATHS.values()}
    record={"baseline_sha":BASE,"source_hashes":source_hashes,"tests":rows,
            "device_hardware":"NOT TESTED","MIDI_interrupt_timing":"NOT MEASURED"}
    (OUT/"host-results.json").write_text(json.dumps(record,indent=2)+"\n")
    print("PASS LXR-BUG-033 baseline reproduces the defect; candidate preserves single Note On")

if __name__=="__main__":
    main()
