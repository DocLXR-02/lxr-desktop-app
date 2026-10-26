// SPDX-License-Identifier: GPL-3.0-only
// Compile the complete production parser, including its static NRPN state.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "Core/Src/Soundengine/MIDI/MidiParser.c"

DrumVoice voiceArray[NUM_DRUM_VOICES];
SnareVoice snareVoice;
HiHatVoice hatVoice;
CymbalVoice cymbalVoice;
FxBus fxBus;
ModulationNode velocityModulators[6];
uint8_t activeParameters[NUM_PARAMS];
uint8_t mixer_audioRouting[6];
float mixer_decimation_rate[7];
uint8_t frontParser_activeTrack, menu_activePage;
PatternSet seq_patternSet;
volatile uint32_t systick_ticks;

static unsigned notifications, drive_writes, mute_writes, forwarded;
static uint8_t mute_voice, mute_value, forwarded_bytes[3];
static uint16_t last_notification;

void modNode_originalValueChanged(uint16_t idx)
{
    assert(idx < END_OF_SOUND_PARAMETERS);
    last_notification = idx;
    notifications++;
}
void SVF_setDrive(ResonantFilter *filter, float drive)
{
    filter->drive = drive;
    drive_writes++;
}
void seq_setMute(uint8_t voice, uint8_t muted)
{
    assert(voice < 7);
    mute_voice = voice;
    mute_value = muted;
    mute_writes++;
}
void seq_recordAutomation(uint8_t track, uint8_t param, uint8_t value)
{
    (void)track; (void)param; (void)value;
}
void uart_sendFrontpanelByte(uint8_t value)
{
    assert(forwarded < sizeof(forwarded_bytes));
    forwarded_bytes[forwarded++] = value;
}

static void cc(uint8_t number, uint8_t value)
{
    MidiMsg msg = {.status = MIDI_CC, .data1 = number, .data2 = value};
    midiParser_ccHandler(msg, 1, 0, 0.f);
}
static void nrpn(uint16_t number, uint8_t value)
{
    cc(NRPN_FINE, number & 127);
    cc(NRPN_COARSE, (number >> 7) & 127);
    cc(NRPN_DATA_ENTRY_COARSE, value);
}
static void external_cc(uint8_t number, uint8_t value)
{
    MidiMsg msg = {.status = MIDI_CC, .data1 = number, .data2 = value};
    forwarded = 0;
    midiParser_parseMidiMessage(msg);
    assert(forwarded == 3 && forwarded_bytes[0] == MIDI_CC);
    assert(forwarded_bytes[1] == number && forwarded_bytes[2] == value);
}

int main(void)
{
    uint8_t cache[sizeof(midiParser_originalCcValues)];
    memset(midiParser_originalCcValues, 0xa5, sizeof(cache));
    memcpy(cache, midiParser_originalCcValues, sizeof(cache));

    // CC0 must not touch the cache, notify modulators, or change the selector.
    midiParser_activeNrpnNumber = 3;
    for(unsigned flags = 0; flags < 4; flags++) {
        MidiMsg msg = {.status = MIDI_CC, .data1 = 0, .data2 = 127};
        midiParser_ccHandler(msg, flags & 1, (flags >> 1) & 1, 0.5f);
    }
    assert(!notifications && !memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
    assert(midiParser_activeNrpnNumber == 3);

    // Bank change still reaches the UI through the external MIDI entry point.
    MidiMsg bank = {.status = MIDI_CC, .data1 = CC_BANK_CHANGE, .data2 = 17};
    midiParser_parseMidiMessage(bank);
    assert(forwarded == 3 && forwarded_bytes[0] == MIDI_CC);
    assert(forwarded_bytes[1] == CC_BANK_CHANGE && forwarded_bytes[2] == 17);
    assert(!notifications && !memcmp(cache, midiParser_originalCcValues, sizeof(cache)));

    // Valid low CC, CC127 and CC2 Drive retain their cache/notification mapping.
    cc(CC_MOD_WHEEL, 23);
    assert(midiParser_originalCcValues[1] == 23 && last_notification == 0);
    cc(AMOUNT_LFO6, 31);
    assert(midiParser_originalCcValues[127] == 31 && last_notification == 126);
    nrpn(CC2_FILTER_DRIVE_1, 127);
    assert(drive_writes == 1 && voiceArray[0].filter.drive == 1.f);
    assert(midiParser_originalCcValues[128] == 127 && last_notification == 128);
    nrpn(CC2_FILTER_DRIVE_3, 64);
    assert(drive_writes == 2 && voiceArray[2].filter.drive == 64 / 127.f);
    nrpn(END_OF_SOUND_PARAMETERS - 129, 42);
    assert(last_notification == END_OF_SOUND_PARAMETERS - 1);
    assert(midiParser_originalCcValues[END_OF_SOUND_PARAMETERS - 1] == 42);

    // Valid NRPN also works through the external MIDI routing/channel path.
    external_cc(NRPN_FINE, CC2_FILTER_DRIVE_2);
    external_cc(NRPN_COARSE, 0);
    external_cc(NRPN_DATA_ENTRY_COARSE, 31);
    assert(drive_writes == 3 && voiceArray[1].filter.drive == 31 / 127.f);

    // A high 14-bit selector must never alias a low parameter or mute command.
    const uint16_t invalid[] = {119, 126, 127, 128, 199, 207, 255, 256, 258, 456, 16383};
    for(unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        cc(NRPN_FINE, invalid[i] & 127);
        cc(NRPN_COARSE, invalid[i] >> 7);
        unsigned before = notifications;
        cc(NRPN_DATA_ENTRY_COARSE, 0);
        assert(notifications == before && drive_writes == 3 && !mute_writes);
    }

    // Special external NRPN mutes work, without cache access or notification.
    for(unsigned voice = 0; voice < 7; voice++) {
        cc(NRPN_FINE, (CC2_MUTE_1 + voice) & 127);
        cc(NRPN_COARSE, (CC2_MUTE_1 + voice) >> 7);
        unsigned before = notifications;
        cc(NRPN_DATA_ENTRY_COARSE, 127);
        assert(notifications == before && mute_voice == voice && mute_value == 1);
        cc(NRPN_DATA_ENTRY_COARSE, 0);
        assert(notifications == before && mute_value == 0);
    }
    assert(mute_writes == 14);
    external_cc(NRPN_FINE, CC2_MUTE_7 & 127);
    external_cc(NRPN_COARSE, CC2_MUTE_7 >> 7);
    unsigned before = notifications;
    external_cc(NRPN_DATA_ENTRY_COARSE, 127);
    assert(mute_writes == 15 && mute_voice == 6 && mute_value == 1);
    assert(notifications == before);

    // Exhaust every CC2 byte for all cache/modulator flag combinations.
    for(unsigned flags = 0; flags < 4; flags++) {
        for(unsigned number = 0; number < 256; number++) {
            unsigned param = number + 128;
            if(param < END_OF_SOUND_PARAMETERS) continue; // normal DSP covered by firmware builds
            MidiMsg msg = {.status = MIDI_CC2, .data1 = number, .data2 = 0};
            unsigned before = notifications;
            memcpy(cache, midiParser_originalCcValues, sizeof(cache));
            midiParser_ccHandler(msg, flags & 1, (flags >> 1) & 1, 0.f);
            assert(notifications == before);
            assert(!memcmp(cache, midiParser_originalCcValues, sizeof(cache)));
        }
    }
    puts("PASS: CC0, external bank forwarding, CC/NRPN Drive, NRPN mutes, invalid selectors and CC2 bounds");
}
