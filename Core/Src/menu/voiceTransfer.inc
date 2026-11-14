/* R22 TEST02: local DATA navigation/editing; only explicit OK transfers. */
static PresetVoiceCopy menu_voiceCopy;
static uint8_t menu_voiceCopyActive, menu_voiceCopyTarget, menu_voiceCopyCopyHeld;
static uint8_t menu_voiceCopyDrain, menu_voiceCopyNotice;
static uint8_t menu_voiceCopyTargetVoice, menu_voiceCopyVoiceHeld;
static uint8_t menu_voiceCopyFocus, menu_voiceCopyEditing, menu_voiceCopySourcePage;
static uint8_t menu_voiceCopyModeHeld; /* 1 held, 2 released/deglitching. */
static uint32_t menu_voiceCopyReleased, menu_voiceCopySince, menu_voiceCopyModeReleased;
enum { VOICE_COPY_KIT, VOICE_COPY_DRUM, VOICE_COPY_OK };

static void menu_finishVoiceCopy(uint8_t notice)
{
    menu_voiceCopyActive = 0;
    menu_voiceCopyEditing = 0;
    memset(&menu_voiceCopy, 0, sizeof(menu_voiceCopy));
    menu_voiceCopyNotice = notice;
    menu_voiceCopySince = HAL_GetTick();
    menu_voiceCopyDrain = 1;
    menu_voiceCopyReleased = HAL_GetTick();
    menu_repaintAll();
}
static void menu_cancelVoiceCopy(void)
{
    menu_finishVoiceCopy(0);
    /* The modal overlay never changed the source page/voice. Restore only UI
     * state; the normal MODE1 dispatcher would toggle this page into STEP. */
    menu_activePage = menu_voiceCopySourcePage;
    editModeActive = 0;
    visibleCursor = 0;
    buttonHandler_setMode(SELECT_MODE_VOICE);
    led_setMode2(SELECT_MODE_VOICE);
    led_setActiveVoice(menu_getActiveVoice());
    led_setActiveSelectButton(menu_getSubPage());
    menu_resetActiveParameter();
    menu_repaintAll();
}
uint8_t menu_voiceTransferButton(uint8_t button, uint8_t pressed)
{
    uint32_t now = HAL_GetTick();
    if(menu_voiceCopyModeHeld == 2 && (uint32_t)(now - menu_voiceCopyModeReleased) >= 40u)
        menu_voiceCopyModeHeld = 0;
    if(button == BUT_MODE1 && (menu_voiceCopyActive || menu_voiceCopyModeHeld)) {
        if(pressed) {
            menu_voiceCopyModeHeld = 1;
            if(menu_voiceCopyActive) menu_cancelVoiceCopy();
        } else {
            menu_voiceCopyModeHeld = 2;
            menu_voiceCopyModeReleased = now;
        }
        return 1;
    }
    /* COPY has no dialog action, including while editing or focusing OK. */
    if(button == BUT_COPY && (menu_voiceCopyActive || menu_voiceCopyCopyHeld)) {
        menu_voiceCopyCopyHeld = pressed ? 1 : 0;
        if(!pressed && !menu_voiceCopyActive) menu_voiceCopyReleased = now;
        return 1;
    }
    /* Instrument buttons retain cancel/consume behavior, never retargeting.
     * Keep their held edges/releases consumed even after the modal closes. */
    if(button >= BUT_VOICE_1 && button <= BUT_VOICE_7) {
        uint8_t bit = (uint8_t)(1u << (button - BUT_VOICE_1));
        if(menu_voiceCopyVoiceHeld & bit) {
            if(!pressed) {
                menu_voiceCopyVoiceHeld &= (uint8_t)~bit;
                if(!menu_voiceCopyActive) menu_voiceCopyReleased = now;
            }
            return 1;
        }
        if(pressed && menu_voiceCopyActive) {
            menu_voiceCopyVoiceHeld |= bit;
            menu_finishVoiceCopy(3);
            return 1;
        }
    }
    if(!menu_voiceCopyActive || !pressed) return 0;
    if(button == BUT_START_STOP) return 0;
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
            if(!button && !buttonHandler_getCopy()) {
                menu_voiceCopyActive = 2;
                menu_voiceCopyReleased = now;
            }
        } else {
            if(click) {
                if((uint32_t)(now - menu_voiceCopyReleased) >= 40u) {
                    if(menu_voiceCopyFocus != VOICE_COPY_OK)
                        menu_voiceCopyEditing = !menu_voiceCopyEditing;
                    else {
                        uint8_t result = buttonHandler_getShift() ? PRESET_VOICE_REJECTED :
                            preset_transferVoiceTo(&menu_voiceCopy, menu_voiceCopyTarget, menu_voiceCopyTargetVoice);
                        menu_finishVoiceCopy(result == PRESET_VOICE_OK ? 1 :
                            result == PRESET_VOICE_MORPH_ACTIVE ? 4 : 2);
                    }
                }
            } else if(inc) {
                if(menu_voiceCopyEditing) {
                    uint8_t *value = menu_voiceCopyFocus == VOICE_COPY_KIT ?
                        &menu_voiceCopyTarget : &menu_voiceCopyTargetVoice;
                    uint8_t limit = menu_voiceCopyFocus == VOICE_COPY_KIT ? NUM_KIT_SLOTS - 1 : 2;
                    if(inc > 0 && *value < limit) ++*value;
                    if(inc < 0 && *value > 0) --*value;
                } else {
                    if(inc > 0 && menu_voiceCopyFocus < VOICE_COPY_OK) ++menu_voiceCopyFocus;
                    if(inc < 0 && menu_voiceCopyFocus > VOICE_COPY_KIT) --menu_voiceCopyFocus;
                    if(menu_voiceCopy.voice >= 3 && menu_voiceCopyFocus == VOICE_COPY_DRUM)
                        menu_voiceCopyFocus = inc > 0 ? VOICE_COPY_OK : VOICE_COPY_KIT;
                }
            }
            if(!button && previous) menu_voiceCopyReleased = now;
        }
        menu_repaintAll();
        return 1;
    }
    if(menu_voiceCopyDrain) {
        lastEncoderButton = button;
        /* Start the release interval at the falling edge, even after a long hold. */
        if(button || previous || menu_voiceCopyModeHeld == 1 ||
           menu_voiceCopyCopyHeld || menu_voiceCopyVoiceHeld) menu_voiceCopyReleased = now;
        else if((uint32_t)(now - menu_voiceCopyReleased) >= 40u &&
                (menu_voiceCopyModeHeld != 2 || (uint32_t)(now - menu_voiceCopyModeReleased) >= 40u))
            menu_voiceCopyDrain = 0;
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
            menu_voiceCopyTargetVoice = menu_voiceCopy.voice;
            menu_voiceCopySourcePage = menu_activePage;
            menu_voiceCopyFocus = VOICE_COPY_KIT;
            menu_voiceCopyEditing = 0;
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
    visibleCursor = 0;
    if(menu_voiceCopyActive) {
        uint8_t drum = menu_voiceCopy.voice < 3;
        const char *row = drum ? "D1 > K01 D1  OK " : "V4 > K01     OK ";
        if(menu_voiceCopyEditing)
            row = menu_voiceCopyFocus == VOICE_COPY_KIT ?
                (drum ? "D1 > [K01] D1 OK" : "V4 > [K01]   OK ") : "D1 > K01 [D1] OK";
        else if(menu_voiceCopyFocus == VOICE_COPY_OK)
            row = drum ? "D1 > K01 D1 >OK " : "V4 > K01    >OK ";
        memcpy(editDisplayBuffer[0], "copy voice to:  ", 16);
        memcpy(editDisplayBuffer[1], row, 16);
        editDisplayBuffer[1][1] = '1' + menu_voiceCopy.voice;
        uint8_t kitColumn = menu_voiceCopyEditing && menu_voiceCopyFocus == VOICE_COPY_KIT ? 7 : 6;
        uint16_t kit = (uint16_t)menu_voiceCopyTarget + 1;
        editDisplayBuffer[1][kitColumn] = '0' + kit / 10;
        editDisplayBuffer[1][kitColumn + 1] = '0' + kit % 10;
        if(drum) {
            uint8_t column = !menu_voiceCopyEditing ? 10 :
                menu_voiceCopyFocus == VOICE_COPY_KIT ? 12 : 11;
            editDisplayBuffer[1][column] = '1' + menu_voiceCopyTargetVoice;
        }
        if(!menu_voiceCopyEditing && menu_voiceCopyFocus != VOICE_COPY_OK)
            visibleCursor = VIS_CURS(1, menu_voiceCopyFocus == VOICE_COPY_KIT ? 5 : 9, 1);
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
