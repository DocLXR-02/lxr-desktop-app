/* DIAG 01-only CONFIG overlay. No new parameter IDs or saved state.
 * Included by menu.c after its private menu/voice-transfer state. */
#if LXR_RUNTIME_DIAGNOSTICS
static volatile uint8_t menu_diagVisible, menu_diagPotRebase, menu_diagDrain;
static uint8_t menu_diagPage, menu_diagConfirm;
static char menu_diagRows[2][17];
enum { DIAG_LAST_PAGE = 18 };

static uint8_t menu_diagConflict(void)
{
    return menu_voiceCopyActive || menu_voiceCopyDrain || menu_voiceCopyNotice ||
           menu_voiceCopyModeHeld || menu_voiceCopyCopyHeld || menu_voiceCopyVoiceHeld ||
           copyClear_Mode != MODE_NONE || copyClear_showClearMenu ||
           menu_autoNotice || menu_kitLoadNotice || menu_sdError != FR_OK ||
           screensaver_Active;
}

/* Foreground-only text formatting; every field fits the physical 16-column LCD. */
static void menu_diagLine(char line[17], const char *text)
{
    uint8_t i = 0;
    while(i < 16u && text[i]) { line[i] = text[i]; ++i; }
    while(i < 16u) line[i++] = ' ';
    line[16] = '\0';
}

static void menu_diagNumber(char line[17], uint32_t value, uint8_t valid,
                            uint8_t saturating, uint8_t limit)
{
    if(!valid) { menu_diagLine(line, "N/A"); return; }
    char digits[10];
    uint8_t n = 0;
    uint32_t rest = value;
    do {
        digits[n++] = (char)('0' + rest % 10u);
        rest /= 10u;
    } while(rest && n < sizeof(digits));
    menu_diagLine(line, "");
    for(uint8_t i = 0; i < n; ++i) line[i] = digits[n - 1u - i];
    if(saturating && value == UINT32_MAX) memcpy(line + n, " SAT", 4);
    else if(limit) memcpy(line + n, " LIMIT", 6);
}

static void menu_diagPublish(void)
{
    char lines[2][17];
    const LxrDiagSessionState state = lxr_diag_session_state();
    const LxrDiagResult *res = lxr_diag_session_result();
    if(!res || menu_diagPage == 0u) {
        menu_diagLine(lines[0], state == LXR_DIAG_RUNNING ? "DIAG 01 RUNNING" :
                     state == LXR_DIAG_FROZEN ? "DIAG 01 FROZEN" : "DIAG 01 READY");
        menu_diagLine(lines[1], state == LXR_DIAG_RUNNING ? "DATA: STOP" :
                     state == LXR_DIAG_FROZEN ? (menu_diagConfirm ? "DATA: CONFIRM" : "DATA: NEW?") :
                     "DATA: START");
    } else {
        const LxrDiagnostics *d = &res->data;
        const uint8_t timed = d->cycle_counter_enabled != 0u;
        const char *label = "";
        uint32_t value = 0;
        uint8_t valid = 1, sat = 1, limit = 0;
        switch(menu_diagPage) {
        case 1: label = "SESSION NUMBER"; value = res->number; break;
        case 2: label = "ELAPSED ms"; value = res->elapsed_ms;
                sat = 0; limit = res->duration_limit; break;
        case 3: label = "DWT ENABLED"; value = timed; sat = 0; break;
        case 4: label = "NOM BUDGET cyc"; value = LXR_DIAG_BUDGET; sat = 0; break;
        case 5: label = "DSP PEAK cyc"; value = d->render.max_cycles; valid = timed; sat = 0; break;
        case 6: label = "DSP BLOCKS"; value = d->render.count; valid = timed; break;
        case 7: label = "DSP OVER BUDGET"; value = d->render.over_budget; valid = timed; break;
        case 8: label = "TIM2 PEAK cyc"; value = d->timer_isr.max_cycles; valid = timed; sat = 0; break;
        case 9: label = "TIM2 COUNT"; value = d->timer_isr.count; valid = timed; break;
        case 10: label = "TIM2 OVER BUDGET"; value = d->timer_isr.over_budget; valid = timed; break;
        case 11: label = "DMA1 CALLBACKS"; value = d->dma[0].callbacks; break;
        case 12: label = "DMA1 NOT READY"; value = d->dma[0].not_ready; break;
        case 13: label = "DMA1 REARM ERR"; value = d->dma[0].rearm_errors; break;
        case 14: label = "DMA1 LONG GAPS"; value = d->dma[0].long_intervals; valid = timed; break;
        case 15: label = "DMA2 CALLBACKS"; value = d->dma[1].callbacks; break;
        case 16: label = "DMA2 NOT READY"; value = d->dma[1].not_ready; break;
        case 17: label = "DMA2 REARM ERR"; value = d->dma[1].rearm_errors; break;
        case 18: label = "DMA2 LONG GAPS"; value = d->dma[1].long_intervals; valid = timed; break;
        default: label = "DIAG PAGE ERROR"; valid = 0; break;
        }
        menu_diagLine(lines[0], label);
        menu_diagNumber(lines[1], value, valid, sat, limit);
    }
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    memcpy(menu_diagRows, lines, sizeof(menu_diagRows));
    __set_PRIMASK(primask);
    menu_repaintAll();
}

void menu_diagClose(void)
{
    if(!menu_diagVisible) return;
    menu_diagPotRebase = 1;
    menu_diagDrain = 1;
    menu_diagVisible = 0;
    menu_repaintAll();
}

/* Only the explicit CONFIG-button path calls this; normal encoder wrapping
 * and all real CONFIG pages/parameter indices retain their original behavior. */
uint8_t menu_diagConfig(void)
{
    if(menu_diagVisible) { menu_diagClose(); return 0; }
    if(__get_IPSR() != 0u || menu_activePage != MENU_MIDI_PAGE ||
       (menuIndex & MASK_PAGE) != (1u << PAGE_SHIFT) ||
       (menuIndex & MASK_PARAMETER) >= 4u || menu_diagConflict() ||
       buttonHandler_getCopy()) return 0;
    editModeActive = 0;
    menu_diagPage = 0;
    menu_diagConfirm = 0;
    menu_diagDrain = 1;
    menu_diagPublish();
    menu_diagVisible = 1;
    menu_repaintAll();
    return 1;
}

/* ADC foreground caller: 1 suppress, 2 rebase all four pots, 0 normal.
 * The underlying page/index is never invalidated. */
uint8_t menu_diagPotMode(void)
{
    if(menu_diagVisible) return 1;
    if(menu_diagPotRebase) { menu_diagPotRebase = 0; return 2; }
    return 0;
}

static void menu_diagService(void)
{
    const uint8_t stopped = lxr_diag_session_poll();
    if(menu_diagVisible && (menu_activePage != MENU_MIDI_PAGE || menu_diagConflict()))
        menu_diagClose();
    else if(menu_diagVisible && stopped) {
        menu_diagPage = 0;
        menu_diagConfirm = 0;
        menu_diagPublish();
    }
}

static uint8_t menu_diagEncoder(int8_t inc, uint8_t button)
{
    if(menu_diagDrain) {
        lastEncoderButton = button;
        if(!button && !inc) menu_diagDrain = 0;
        if(button || inc) screensaver_touch();
        return 1;
    }
    if(!menu_diagVisible) return 0;
    const uint8_t click = button && !lastEncoderButton;
    lastEncoderButton = button;
    if(!click && !inc) return 1;
    screensaver_touch();
    if(buttonHandler_getShift() || buttonHandler_getCopy()) return 1;
    if(menu_diagConflict()) { menu_diagClose(); return 1; }

    const LxrDiagSessionState state = lxr_diag_session_state();
    if(click && menu_diagPage == 0u) {
        if(state == LXR_DIAG_RUNNING) {
            (void)lxr_diag_session_stop();
            menu_diagConfirm = 0;
        } else if(state == LXR_DIAG_FROZEN && !menu_diagConfirm) {
            menu_diagConfirm = 1;
        } else if(lxr_diag_session_start()) {
            menu_diagConfirm = 0;
        }
        menu_diagPublish();
    } else if(inc && !button && state == LXR_DIAG_FROZEN) {
        menu_diagConfirm = 0;
        if(inc > 0)
            menu_diagPage = menu_diagPage == DIAG_LAST_PAGE ? 0u : menu_diagPage + 1u;
        else
            menu_diagPage = menu_diagPage == 0u ? DIAG_LAST_PAGE : menu_diagPage - 1u;
        menu_diagPublish();
    }
    return 1;
}

static uint8_t menu_diagRender(void)
{
    if(!menu_diagVisible || menu_activePage != MENU_MIDI_PAGE) return 0;
    /* Existing repaint/refresh path may run in TIM14. Only copy published
     * fixed-width text here; never format or access live diagnostic data. */
    memcpy(editDisplayBuffer, menu_diagRows, sizeof(menu_diagRows));
    visibleCursor = 0;
    display_needs_refresh = 1;
    return 1;
}
#endif
