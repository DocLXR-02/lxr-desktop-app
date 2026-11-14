/* R21 saved parameter transfer. Included by presetManager.c; no engine or SD calls. */
static const uint8_t preset_voiceBaseIds[6][27] = {
    { PAR_COARSE1, PAR_FINE1, PAR_VELOA1, PAR_VELOD1, PAR_VOL_SLOPE1, PAR_FILTER_FREQ_1, PAR_RESO_1, PAR_FILTER_TYPE_1, PAR_FILTER_DRIVE_1, PAR_TRANS1_VOL, PAR_TRANS1_WAVE, PAR_TRANS1_FREQ, PAR_VOL1, PAR_PAN1, PAR_VOICE_DECIMATION1, PAR_PWM_1, PAR_OSC_WAVE_DRUM1, PAR_MOD_WAVE_DRUM1, PAR_MOD_EG1, PAR_MODAMNT1, PAR_PITCH_SLOPE1, PAR_FMAMNT1, PAR_FM_FREQ1, PAR_MIX_MOD_1, PAR_DRIVE1, PAR_NONE },
    { PAR_COARSE2, PAR_FINE2, PAR_VELOA2, PAR_VELOD2, PAR_VOL_SLOPE2, PAR_FILTER_FREQ_2, PAR_RESO_2, PAR_FILTER_TYPE_2, PAR_FILTER_DRIVE_2, PAR_TRANS2_VOL, PAR_TRANS2_WAVE, PAR_TRANS2_FREQ, PAR_VOL2, PAR_PAN2, PAR_VOICE_DECIMATION2, PAR_PWM_2, PAR_OSC_WAVE_DRUM2, PAR_MOD_WAVE_DRUM2, PAR_MOD_EG2, PAR_MODAMNT2, PAR_PITCH_SLOPE2, PAR_FMAMNT2, PAR_FM_FREQ2, PAR_MIX_MOD_2, PAR_DRIVE2, PAR_NONE },
    { PAR_COARSE3, PAR_FINE3, PAR_VELOA3, PAR_VELOD3, PAR_VOL_SLOPE3, PAR_FILTER_FREQ_3, PAR_RESO_3, PAR_FILTER_TYPE_3, PAR_FILTER_DRIVE_3, PAR_TRANS3_VOL, PAR_TRANS3_WAVE, PAR_TRANS3_FREQ, PAR_VOL3, PAR_PAN3, PAR_VOICE_DECIMATION3, PAR_PWM_3, PAR_OSC_WAVE_DRUM3, PAR_MOD_WAVE_DRUM3, PAR_MOD_EG3, PAR_MODAMNT3, PAR_PITCH_SLOPE3, PAR_FMAMNT3, PAR_FM_FREQ3, PAR_MIX_MOD_3, PAR_DRIVE3, PAR_NONE },
    { PAR_COARSE4, PAR_FINE4, PAR_VELOA4, PAR_VELOD4, PAR_VOL_SLOPE4, PAR_FILTER_FREQ_4, PAR_RESO_4, PAR_FILTER_TYPE_4, PAR_FILTER_DRIVE_4, PAR_TRANS4_VOL, PAR_TRANS4_WAVE, PAR_TRANS4_FREQ, PAR_VOL4, PAR_PAN4, PAR_VOICE_DECIMATION4, PAR_PWM_4, PAR_OSC_WAVE_SNARE, PAR_NOISE_FREQ1, PAR_MIX1, PAR_REPEAT4, PAR_MOD_EG4, PAR_MODAMNT4, PAR_PITCH_SLOPE4, PAR_SNARE_DISTORTION, PAR_NONE },
    { PAR_COARSE5, PAR_FINE5, PAR_VELOA5, PAR_VELOD5, PAR_VOL_SLOPE5, PAR_FILTER_FREQ_5, PAR_RESO_5, PAR_FILTER_TYPE_5, PAR_FILTER_DRIVE_5, PAR_TRANS5_VOL, PAR_TRANS5_WAVE, PAR_TRANS5_FREQ, PAR_VOL5, PAR_PAN5, PAR_VOICE_DECIMATION5, PAR_PWM_5, PAR_WAVE1_CYM, PAR_WAVE2_CYM, PAR_WAVE3_CYM, PAR_MOD_OSC_F1_CYM, PAR_MOD_OSC_F2_CYM, PAR_MOD_OSC_GAIN1_CYM, PAR_MOD_OSC_GAIN2_CYM, PAR_REPEAT5, PAR_CYMBAL_DISTORTION, PAR_NONE },
    { PAR_COARSE6, PAR_FINE6, PAR_VELOA6, PAR_VELOD6_CLOSED, PAR_VOL_SLOPE6, PAR_FILTER_FREQ_6, PAR_RESO_6, PAR_FILTER_TYPE_6, PAR_FILTER_DRIVE_6, PAR_TRANS6_VOL, PAR_TRANS6_WAVE, PAR_TRANS6_FREQ, PAR_VOL6, PAR_PAN6, PAR_VOICE_DECIMATION6, PAR_PWM_6, PAR_WAVE1_HH, PAR_WAVE2_HH, PAR_WAVE3_HH, PAR_MOD_OSC_F1, PAR_MOD_OSC_F2, PAR_MOD_OSC_GAIN1, PAR_MOD_OSC_GAIN2, PAR_VELOD6_OPEN, PAR_HAT_DISTORTION, PAR_NONE },
};
static const uint8_t preset_voiceLfoGroups[] = {
    PAR_FREQ_LFO1, PAR_AMOUNT_LFO1, PAR_WAVE_LFO1, PAR_VOICE_LFO1,
    PAR_TARGET_LFO1, PAR_RETRIGGER_LFO1, PAR_SYNC_LFO1, PAR_OFFSET_LFO1
};
static uint8_t preset_voiceOwnsParameter(uint8_t voice, uint16_t id)
{
    if(voice >= 6 || id == PAR_NONE) return 0;
    for(unsigned i = 0; preset_voiceBaseIds[voice][i] != PAR_NONE; ++i)
        if(preset_voiceBaseIds[voice][i] == id) return 1;
    for(unsigned i = 0; i < sizeof(preset_voiceLfoGroups); ++i)
        if(preset_voiceLfoGroups[i] + voice == id) return 1;
    return id == PAR_VEL_DEST_1 + voice || id == PAR_VELO_MOD_AMT_1 + voice ||
           id == PAR_VOLUME_MOD_ON_OFF1 + voice;
}
static uint16_t preset_voiceLfoDestination(const uint8_t *kit, uint8_t lfo)
{
    uint8_t index = kit[PAR_TARGET_LFO1 + lfo];
    if(index >= getNumModTargets()) return PAR_NONE;
    return pgm_read_word(&modTargets[index].param);
}
uint8_t preset_voiceCopyContextValid(const PresetVoiceCopy *copy)
{
    return copy && copy->voice < 6 && copy->kit < NUM_KIT_SLOTS &&
        copy->project == menu_loadedPresetNr[CURRENT_PROJECT] &&
        copy->kit == menu_loadedPresetNr[CURRENT_KIT] &&
        copy->revision == preset_kitRevision && !seq_isRunning() &&
        copy->pattern == seq_activePattern && !seq_hasPendingKitChange();
}
uint8_t preset_captureVoice(PresetVoiceCopy *copy, uint8_t track)
{
    if(!copy || track > 6 || seq_isRunning() || seq_hasPendingKitChange() ||
       menu_loadedPresetNr[CURRENT_KIT] >= NUM_KIT_SLOTS ||
       menu_loadedPresetNr[CURRENT_PROJECT] >= NUM_KIT_SLOTS) return 0;
    copy->voice = track == 6 ? 5 : track;
    copy->kit = menu_loadedPresetNr[CURRENT_KIT];
    copy->project = menu_loadedPresetNr[CURRENT_PROJECT];
    copy->pattern = seq_activePattern;
    copy->revision = preset_kitRevision;
    buttonHandler_copyEditableBase(copy->values, sizeof(copy->values));
    return preset_voiceCopyContextValid(copy);
}
static uint8_t preset_voiceMorphConflict(uint8_t target)
{
    return morphParameters == mainParameters[target] &&
           (activeParameters[PAR_MORPH] != 0 || morph_state != 0);
}
uint8_t preset_transferVoice(const PresetVoiceCopy *copy, uint8_t target)
{
    uint8_t staged[END_OF_SOUND_PARAMETERS];
    if(!preset_voiceCopyContextValid(copy) || target >= NUM_KIT_SLOTS || target == copy->kit)
        return 0;
    if(preset_voiceMorphConflict(target)) return PRESET_VOICE_MORPH_ACTIVE;
    memcpy(staged, mainParameters[target], sizeof(staged));
    uint8_t voice = copy->voice;
    for(unsigned i = 0; preset_voiceBaseIds[voice][i] != PAR_NONE; ++i) {
        uint8_t id = preset_voiceBaseIds[voice][i];
        if(copy->values[id] > 127) return 0;
        staged[id] = copy->values[id];
    }
    uint16_t dest = preset_voiceLfoDestination(copy->values, voice);
    if(copy->values[PAR_AMOUNT_LFO1 + voice] != 0 &&
       copy->values[PAR_VOICE_LFO1 + voice] == voice + 1 &&
       preset_voiceOwnsParameter(voice, dest)) {
        for(unsigned i = 0; i < sizeof(preset_voiceLfoGroups); ++i) {
            uint8_t id = preset_voiceLfoGroups[i] + voice;
            /* Target-list indices legitimately exceed 127. */
            if(preset_voiceLfoGroups[i] != PAR_TARGET_LFO1 && copy->values[id] > 127) return 0;
            staged[id] = copy->values[id];
        }
        for(uint8_t i = 0; i < 6; ++i)
            if(i != voice && preset_voiceLfoDestination(staged, i) == dest)
                staged[PAR_TARGET_LFO1 + i] = 0; /* Existing OFF encoding. */
    }
    /* The sequencer/clock can change context in an interrupt. Guard only the
     * final context check and bounded RAM commit, never staging or IO. */
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    uint8_t result = PRESET_VOICE_REJECTED;
    if(preset_voiceCopyContextValid(copy)) {
        if(preset_voiceMorphConflict(target)) result = PRESET_VOICE_MORPH_ACTIVE;
        else {
            memcpy(mainParameters[target], staged, sizeof(staged));
            result = PRESET_VOICE_OK;
        }
    }
    __set_PRIMASK(mask);
    return result;
}
