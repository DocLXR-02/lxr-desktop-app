/* R21 direct transfer selector. Its snapshot exists only during one session. */
static PresetVoiceCopy menu_voiceCopy;
static uint8_t menu_voiceCopyActive, menu_voiceCopyTarget, menu_voiceCopyCopyHeld;
static uint8_t menu_voiceCopyDrain, menu_voiceCopyNotice;
static uint32_t menu_voiceCopyReleased, menu_voiceCopySince;

static void menu_finishVoiceCopy(uint8_t notice)
{
    menu_voiceCopyActive = 0;
    memset(&menu_voiceCopy, 0, sizeof(menu_voiceCopy));
    menu_voiceCopyNotice = notice;
    menu_voiceCopySince = HAL_GetTick();
    menu_voiceCopyDrain = 1;
    menu_voiceCopyReleased = HAL_GetTick();
    menu_repaintAll();
}
/* Called before the normal physical-button dispatcher. COPY cancels; other
 * panel presses cancel and are consumed. START/STOP still operates normally. */
uint8_t menu_voiceTransferButton(uint8_t button, uint8_t pressed)
{
    if(button == BUT_COPY && !pressed && menu_voiceCopyCopyHeld) {
        menu_voiceCopyCopyHeld = 0;
        return 1;
    }
    if(!menu_voiceCopyActive || !pressed) return 0;
    if(button == BUT_START_STOP) return 0;
    if(pressed && !(button == BUT_COPY && menu_voiceCopyCopyHeld))
        menu_finishVoiceCopy(3);
    return 1;
}
static uint8_t menu_voiceTransferEncoder(int8_t inc, uint8_t button)
{
    uint32_t now = HAL_GetTick();
    uint8_t previous = lastEncoderButton;
    uint8_t click = button && !previous;
    if(menu_voiceCopyActive) {
        lastEncoderButton = button;
        if(menu_voiceCopyActive == 1) {
            /* Both entry controls must be released before confirmation arms. */
            if(!button && !buttonHandler_getCopy()) {
                menu_voiceCopyActive = 2;
                menu_voiceCopyReleased = now;
            }
        } else {
            if(inc) {
                int target = (int)menu_voiceCopyTarget + (inc > 0 ? 1 : -1);
                if(target >= 0 && target < NUM_KIT_SLOTS) menu_voiceCopyTarget = target;
            }
            if(click && (uint32_t)(now - menu_voiceCopyReleased) >= 40u) {
                uint8_t result = buttonHandler_getShift() || buttonHandler_getCopy() ?
                    PRESET_VOICE_REJECTED : preset_transferVoice(&menu_voiceCopy, menu_voiceCopyTarget);
                menu_finishVoiceCopy(result == PRESET_VOICE_OK ? 1 :
                    result == PRESET_VOICE_MORPH_ACTIVE ? 4 : 2);
            }
            if(!button && previous) menu_voiceCopyReleased = now;
        }
        menu_repaintAll();
        return 1;
    }
    if(menu_voiceCopyDrain) {
        lastEncoderButton = button;
        if(button) menu_voiceCopyReleased = now;
        else if((uint32_t)(now - menu_voiceCopyReleased) >= 40u) menu_voiceCopyDrain = 0;
        return 1;
    }
    if(click && buttonHandler_getCopy() && !buttonHandler_getShift() &&
       buttonHandler_getMode() == SELECT_MODE_VOICE && menu_activePage < 7 &&
       !screensaver_Active && !seq_isRunning()) {
        lastEncoderButton = button;
        menu_voiceCopyCopyHeld = 1;
        copyClear_reset();
        if(preset_captureVoice(&menu_voiceCopy, menu_getActiveVoice())) {
            menu_voiceCopyTarget = menu_voiceCopy.kit;
            menu_voiceCopyActive = 1;
            menu_voiceCopyNotice = 0;
            menu_repaintAll();
        } else menu_finishVoiceCopy(2);
        return 1;
    }
    return 0;
}
static void menu_expireVoiceTransfer(void)
{
    if(menu_voiceCopyNotice && (uint32_t)(HAL_GetTick() - menu_voiceCopySince) >= 1500u) {
        menu_voiceCopyNotice = 0;
        menu_repaintAll();
    }
}
static uint8_t menu_renderVoiceTransfer(void)
{
    if(!menu_voiceCopyActive && !menu_voiceCopyNotice) return 0;
    char line[17];
    visibleCursor = 0;
    memset(editDisplayBuffer[0], ' ', 16);
    memset(editDisplayBuffer[1], ' ', 16);
    if(menu_voiceCopyActive) {
        snprintf(line, sizeof(line), "V%u > KIT %02u?", (unsigned)menu_voiceCopy.voice + 1u,
                 (unsigned)menu_voiceCopyTarget + 1u);
        memcpy(editDisplayBuffer[0], line, strlen(line));
        memcpy(editDisplayBuffer[1], "DATA yes COPY no", 16);
    } else {
        memcpy(editDisplayBuffer[0], "Voice transfer  ", 16);
        const char *message = menu_voiceCopyNotice == 1 ? "success!        " :
                              menu_voiceCopyNotice == 2 ? "rejected        " :
                              menu_voiceCopyNotice == 4 ? "Morph active    " : "cancelled       ";
        memcpy(editDisplayBuffer[1], message, 16);
    }
    display_needs_refresh = 1;
    return 1;
}
