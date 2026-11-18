#!/usr/bin/env python3
"""Validate R23 production MIDI fixes through extracted C source and sanitizers.

This suite executes exact edited statements and the actual voice trigger body.
It does not emulate UART interrupt timing, real MIDI cables or device audio.
"""
from pathlib import Path
import hashlib
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/"build/r23-midi-hardening"
OUT.mkdir(parents=True,exist_ok=True)
paths={
    "midi":ROOT/"Core/Src/Soundengine/MIDI/MidiParser.c",
    "front":ROOT/"Core/Src/frontPanelParser.c",
    "voice":ROOT/"Core/Src/Soundengine/MIDI/MidiVoiceControl.c",
    "seq":ROOT/"Core/Src/Soundengine/Sequencer/sequencer.c",
    "menu":ROOT/"Core/Src/menu/menu.c",
    "names":ROOT/"Core/Inc/menu/MenuText.h",
    "preset":ROOT/"Core/Src/Preset/presetManager.c",
    "distortion":ROOT/"Core/Src/Soundengine/DSPAudio/FX/Distortion.c",
    "delay":ROOT/"Core/Src/Soundengine/DSPAudio/FX/Delay.c",
}
src={k:v.read_text(encoding="utf-8") for k,v in paths.items()}
(OUT/"INPUT_SHA256SUMS.txt").write_text("".join(
    f"{hashlib.sha256(paths[k].read_bytes()).hexdigest()}  {paths[k].relative_to(ROOT)}\n"
    for k in sorted(paths)
),encoding="utf-8")

def single(pattern,text,label,flags=re.S):
    result=list(re.finditer(pattern,text,flags))
    if len(result)!=1:
        raise AssertionError(f"{label}: expected exactly one production source match, got {len(result)}")
    return result[0].group(0)

def count(pattern,text,expected,label,flags=re.S):
    actual=len(re.findall(pattern,text,flags))
    if actual!=expected:raise AssertionError(f"{label}: expected {expected}, got {actual}")

cc0=single(
    r'const uint8_t kitNr = frontParser_midiMsg\.data2 % NUM_KIT_SLOTS;\s*'
    r'menu_selectedPresetNr\[SAVE_TYPE_KIT\] = kitNr;\s*'
    r'preset_loadDrumsetRam\(kitNr,0, 0\);',
    src["front"],"CC0 valid kit selection")
assert src["front"].index("const uint8_t kitNr = ") < src["front"].index("menu_selectedPresetNr[SAVE_TYPE_KIT] = kitNr;")
fxguard=single(
    r'#if USE_REVERB\s*if\(msg\.data1 == CC2_FX_TYPE && value\.itg > FX_REVERB\) return;'
    r'\s*#else\s*if\(msg\.data1 == CC2_FX_TYPE && value\.itg > FX_DELAY\) return;'
    r'\s*#endif',src["midi"],"FX type guard")
cache=src["midi"].index("midiParser_originalCcValues[paramNr] = value.itg;")
assert src["midi"].index("if(msg.data1 == CC2_FX_TYPE && value.itg > FX_DELAY) return;") < cache
fxmenu=single(
    r'case MENU_FX:\s*/\*.*?\*/\s*p = curParmVal < fxNames\[0\]\[0\] \? fxNames\[curParmVal\+1\] : menuText_dash;\s*break;',
    src["menu"],"FX menu bounds")
names=single(r'const char fxNames\[\]\[4\]\s*=\s*\{.*?\};',src["names"],"FX names")
assert src["preset"].count("if(activeParameters[PAR_FX_TYPE] >= FX_PARAMETER_BANK_COUNT)") == 1
stored_type=single(
    r'if\(activeParameters\[PAR_FX_TYPE\] >= FX_PARAMETER_BANK_COUNT\)\s*'
    r'activeParameters\[PAR_FX_TYPE\] = FX_OFF;',
    src["preset"],"stored Kit FX type sanitization")
realtime=single(
    r'if\(data >= 0xF8u\)\s*\{\s*'
    r'MidiMsg realtime = midiMsg_tmp;\s*'
    r'realtime\.status = data;\s*'
    r'realtime\.bits\.length = 0;\s*'
    r'realtime\.bits\.sysxbyte = 0;\s*'
    r'realtime\.bits\.source = midiSourceMIDI;\s*'
    r'midiParser_parseMidiMessage\(realtime\);\s*return;\s*\}',
    src["midi"],"realtime independent dispatch")
assert src["midi"].index("if(data >= 0xF8u)") < src["midi"].index("if(data & 0x80)",src["midi"].index("void midiParser_parseUartData("))
cc_channel=single(
    r'\(\(chanonly == midi_MidiChannels\[voiceNr-1\]\) \|\| '
    r'\(chanonly == midi_MidiChannels\[7\]\)\s*\)',
    src["midi"],"voice channel 1-based to 0-based mapping")
assert "voiceNr==0 || voiceNr >= 7" in src["midi"]
autocond=r'if\(msg\.data1 != CC_BANK_CHANGE && msg\.data1 != NRPN_DATA_ENTRY_COARSE &&\s*msg\.data1 != NRPN_FINE && msg\.data1 != NRPN_COARSE\)\s*seq_recordAutomation\(frontParser_activeTrack, msg\.data1, msg\.data2\);'
count(autocond,src["midi"],2,"both automation entry points")
guard_pattern=r'msg\.data1 != CC_BANK_CHANGE && msg\.data1 != NRPN_DATA_ENTRY_COARSE &&\s*msg\.data1 != NRPN_FINE && msg\.data1 != NRPN_COARSE'
guards=re.findall(guard_pattern,src["midi"].split("void midiParser_parseMidiMessage(")[1])
assert len(guards)==2 and guards[0]==guards[1], "Both automation guards must agree"
condition=guards[0]

voice_start=src["voice"].index("void voiceControl_noteOn(")
voice_end=src["voice"].index("//------------------------------------------------",voice_start)
voice_fn=src["voice"][voice_start:voice_end]
assert "seq_sendMidiNoteOn" not in voice_fn,"voice trigger must not emit a second MIDI Note On"
seq_start=src["seq"].index("void seq_triggerVoice(")
seq_end=src["seq"].index("//-----------------------------",seq_start)
assert "seq_sendMidiNoteOn(" in src["seq"][seq_start:seq_end], "sequencer must retain its Note On"
assert "voiceControl_noteOn(voiceNr, note, vol);" in src["seq"][seq_start:seq_end]
# Never include or rewrite the DSP bodies in this change.
assert "Distortion_resetClip" in src["distortion"]
assert "Delay_process(" in src["delay"]

code=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define NUM_KIT_SLOTS 64
#define SAVE_TYPE_KIT 0
#define CC2_FX_TYPE 33
#define FX_DELAY 4
#define FX_REVERB 5
#define FX_OFF 0
#define FX_PARAMETER_BANK_COUNT 5
#define PAR_FX_TYPE 9
#define USE_REVERB 0
#define CC_BANK_CHANGE 0
#define NRPN_DATA_ENTRY_COARSE 6
#define NRPN_FINE 98
#define NRPN_COARSE 99
#define NOTE_ON 0x90
#define MIDI_CLOCK 0xF8
static uint8_t menu_selectedPresetNr[1],loaded;
static unsigned saveCount;
static struct {uint8_t data2;} frontParser_midiMsg;
static void preset_loadDrumsetRam(uint8_t kitNr,uint8_t morph,uint8_t immediate)
{assert(!morph && !immediate);loaded=kitNr;saveCount++;}
static void process_cc0(uint8_t cc) {
    frontParser_midiMsg.data2=cc;
''' + cc0 + r'''
}
static bool accept_fx(uint8_t index,uint8_t v) {
    struct {uint8_t data1;} msg={index};
    struct {uint8_t itg;} value={v};
''' + fxguard.replace("return;","return false;") + r'''
    return true;
}
''' + names + r'''
static const char menuText_dash[]="---";
static const char* fx_label(uint8_t curParmVal) {
    const void *p;
''' + fxmenu.replace("case MENU_FX:","").replace("break;","") + r'''
    return (const char*)p;
}
static uint8_t activeParameters[16];
static void sanitize_stored_fx(void) {
''' + stored_type + r'''
}
enum {midiSourceMIDI,midiSourceUSB};
typedef struct { unsigned source:1,sysxbyte:1,length:2; } MidiBits;
typedef struct {uint8_t status,lastNonSysStatus,data1,data2; MidiBits bits;} MidiMsg;
static MidiMsg midiMsg_tmp,captured;
static unsigned realtimeCount,nonRealtimePath;
static void midiParser_parseMidiMessage(MidiMsg m) {captured=m;realtimeCount++;}
static void process_realtime(uint8_t data) {
''' + realtime + r'''
    nonRealtimePath++;
}
static uint8_t midi_MidiChannels[8];
static bool cc_voice_channel(uint8_t voiceNr,uint8_t chanonly) {
    return ''' + cc_channel + r''';
}
static bool automation(uint8_t controller) {
    struct {uint8_t data1;} msg={controller};
    return ''' + condition + r''';
}
static uint8_t active_voices;
static unsigned synth[7],panelWrites,outgoingMidi;
static void Drum_trigger(uint8_t voice,uint8_t vel,uint8_t note) {assert(voice<3 && vel && note);synth[voice]++;}
static void Snare_trigger(uint8_t vel,uint8_t note) {assert(vel&&note);synth[3]++;}
static void Cymbal_trigger(uint8_t vel,uint8_t note) {assert(vel&&note);synth[4]++;}
static void HiHat_trigger(uint8_t vel,uint8_t hh,uint8_t note) {assert(vel&&note&&hh<2);synth[hh+5]++;}
static void uart_sendFrontpanelByte(uint8_t data) {(void)data;panelWrites++;}
static void seq_sendMidiNoteOn(uint8_t channel,uint8_t note,uint8_t vel)
{(void)channel;(void)note;(void)vel;outgoingMidi++;}
''' + voice_fn + r'''
int main(void) {
    unsigned long cases=0;
    for(unsigned n=0;n<128;n++) {
        process_cc0((uint8_t)n);
        assert(menu_selectedPresetNr[0]==(n%64) && loaded==(n%64));
        cases++;
    }
    assert(saveCount==128);
    for(unsigned val=0;val<128;val++) {
        assert(accept_fx(CC2_FX_TYPE,(uint8_t)val)==(val<=FX_DELAY));
        assert(accept_fx(CC2_FX_TYPE-1,(uint8_t)val));
        assert(!memcmp(fx_label((uint8_t)val),val<5?fxNames[val+1]:"---",3));
        activeParameters[PAR_FX_TYPE]=(uint8_t)val;
        sanitize_stored_fx();
        assert(activeParameters[PAR_FX_TYPE]==(val<5?val:FX_OFF));
        assert(automation((uint8_t)val) != (val==0 || val==6 || val==98 || val==99));
        cases+=5;
    }
    for(unsigned v=1;v<=6;v++) {
        for(unsigned j=0;j<8;j++)midi_MidiChannels[j]=(uint8_t)(j+1);
        midi_MidiChannels[7]=15;
        for(unsigned chan=0;chan<16;chan++) {
            bool want=(chan==v || chan==15);
            assert(cc_voice_channel((uint8_t)v,(uint8_t)chan)==want);
            cases++;
        }
    }
    for(unsigned byte=0xf8;byte<=0xff;byte++) {
        memset(&midiMsg_tmp,0,sizeof(midiMsg_tmp));
        midiMsg_tmp.status=0x90;
        midiMsg_tmp.lastNonSysStatus=0xB0;
        midiMsg_tmp.data1=60;
        midiMsg_tmp.data2=100;
        midiMsg_tmp.bits.source=midiSourceUSB;
        midiMsg_tmp.bits.length=2;
        midiMsg_tmp.bits.sysxbyte=1;
        MidiMsg saved=midiMsg_tmp;
        process_realtime((uint8_t)byte);
        assert(memcmp(&midiMsg_tmp,&saved,sizeof(saved))==0);
        assert(realtimeCount==(byte-0xf8+1) && nonRealtimePath==0);
        assert(captured.status==byte && captured.bits.length==0 && captured.bits.sysxbyte==0);
        assert(captured.bits.source==midiSourceMIDI);
        cases++;
    }
    for(unsigned voice=0;voice<7;voice++) {
        unsigned prev=outgoingMidi;
        voiceControl_noteOn((uint8_t)voice,60,100);
        assert(outgoingMidi==prev);
        assert(synth[voice]==1 && (active_voices&(1u<<voice)));
        cases++;
    }
    assert(panelWrites==21);
    seq_sendMidiNoteOn(0,60,100);
    assert(outgoingMidi==1);
    printf("PASS R23 MIDI extracted source: %lu CC0/FX/Kit/menu/automation/channel/realtime/voice cases\n",cases);
    return 0;
}
'''
f=OUT/"source_extract.c"
f.write_text(code,encoding="utf-8")
for variant,flags in (
    ("ubsan",["-fsanitize=undefined,bounds"]),
    ("asan",["-fsanitize=address"]),
):
    output=OUT/variant
    cmd=["cc","-std=gnu11","-O1","-g","-Wall","-Wextra","-Werror",
         "-fno-omit-frame-pointer","-fno-sanitize-recover=all",*flags,
         str(f),"-o",str(output)]
    built=subprocess.run(cmd,text=True,capture_output=True)
    (OUT/(variant+"_build.log")).write_text(built.stdout+built.stderr)
    if built.returncode:
        print("STOP",variant,"compile",built.stderr[-6500:])
        raise SystemExit(built.returncode)
    run=subprocess.run([str(output)],text=True,capture_output=True)
    (OUT/(variant+"_run.log")).write_text(run.stdout+run.stderr)
    print(variant,run.returncode,run.stdout,run.stderr[-3000:],flush=True)
    if run.returncode:raise SystemExit(run.returncode)
print("PASS R23 MIDI source extraction under both sanitizers")
