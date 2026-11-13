static uint8_t paramToModTarget[END_OF_SOUND_PARAMETERS];
static uint8_t activeParameters[NUM_PARAMS];
static uint8_t menu_activePage, visibleCursor, display_needs_refresh;
static uint8_t display_needs_clear_all, display_refresh_state, screensaver_Active;
static uint8_t copyClear_showClearMenu, rec_led;
static char editDisplayBuffer[2][17], currentDisplayBuffer[2][16];
static uint32_t now;
static int tick_interrupt;
static uint32_t irq_mask;
static uint32_t __get_PRIMASK(void) { return irq_mask; }
static void (*before_irq_mask)(void);
static void __disable_irq(void)
{
    if (before_irq_mask && !irq_mask) {
        void (*callback)(void) = before_irq_mask;
        before_irq_mask = NULL;
        callback();
    }
    irq_mask = 1;
}
static void __set_PRIMASK(uint32_t previous) { irq_mask = previous; }
static uint8_t seq_autoMode, seq_autoSession, seq_autoBlocked, seq_running, seq_recordActive;
static uint8_t seq_autoTrack, seq_autoPattern, seq_autoLane;
static uint8_t seq_activePattern, seq_activeAutomTrack, seq_selectedStep;
static uint8_t frontParser_activeTrack, frontParser_shownPattern, frontParser_rxCnt;
static int8_t seq_armedArmedAutomationStep = -1, seq_armedArmedAutomationTrack = -1;
static uint8_t midiParser_originalCcValues[END_OF_SOUND_PARAMETERS];
static int trigger_clockReceived, seq_prescaleCounter, frontParser_sysexActive;
static AutomationCapture seq_autoCapture;
static volatile uint32_t seq_autoReservationRevision;
static struct { uint8_t status, data1, data2; } cortex_midiMsg;
static struct {
    Step seq_subStepPattern[NUM_PATTERN][NUM_TRACKS][NUM_STEPS];
    uint8_t length[NUM_PATTERN][NUM_TRACKS];
} seq_patternSet;
static void menu_tick(void);
static uint32_t HAL_GetTick(void)
{
    uint32_t value = now;
    if (tick_interrupt && !irq_mask) {
        tick_interrupt = 0;
        ++now;
        menu_tick();
    }
    return value;
}
static void led_setValue(uint8_t value, int led) { rec_led = value; }
static void buttonHandler_setMode(int mode) {}
static void led_clearAllBlinkLeds(void) {}
static void led_setMode2(int mode) {}
static void led_setActiveSelectButton(int mode) {}
static uint8_t menu_getActiveVoice(void) { return VOICE1_PAGE; }
static uint8_t menu_getSubPage(void) { return 0; }
static void menu_switchPage(uint8_t page) { menu_activePage = page; display_needs_clear_all = 1; }
static void menu_switchSubPage(int a, int b) {}
static void menu_resetActiveParameter(void) {}
static int buttonHandler_getShift(void) { return 0; }
static int preset_storageError(void) { return FR_OK; }
static int preset_autosaveIsPaused(void) { return 0; }
static void preset_touchTimer(int timer) {}
static void seq_sendRealtime(int msg) {}
static void trigger_reset(int enabled) {}
static volatile uint8_t refresh_i, refresh_j;
static unsigned lcd_steps;
static int lcd_turnOn_nonblock(int on, int cursor) { return lcd_steps++ % 2; }
static int lcd_setcursor_nonBlock(int column, int row) { return lcd_steps++ % 2; }
static int lcd_data_nonBlock(int value) { return lcd_steps++ % 2; }
static void menu_repaintLoadSavePage(void) { memcpy(editDisplayBuffer[0], "LOAD/SAVE", 9); }
static void menu_repaintGeneric(void) { memcpy(editDisplayBuffer[0], "VOICE PAGE", 10); }
static void uart_clearFrontFifo(void) {}
static void uart_sendFrontpanelSysExByte(int x) {}
static void frontParser_handleSysexData(int x) {}
#define midiParser_ccHandler(msg, a, b, c) do { \
    uint8_t id = (msg).data1 + ((msg).status == FRONT_CC_2 ? 128 : 0); \
    midiParser_originalCcValues[id] = (msg).data2; \
} while (0)
static void seq_setStepIndexToStart(void) {}

static uint8_t seq_stepIndex[NUM_TRACKS], seq_stepToSkip[NUM_TRACKS], menu_activeBar;
static int note_set;
static float seq_getElapsedStepTimePercent(void) { return 0.0f; }
static uint8_t seq_determineNextPattern(int unused) { return seq_activePattern; }
static void seq_setMainStep(int pattern, int track, int step, int active) { note_set += active; }

/* Adapt the three-byte return path to the same menu cache used by the panel. */
static void uart_sendFrontpanelByte(uint8_t data)
{
    static uint8_t status, high, position;
    if (data & 0x80) {
        status = data;
        position = 0;
    } else if (!position) {
        high = data;
        position = 1;
    } else {
        uint8_t destination = (high << 7) | data;
        if (destination == NO_AUTOMATION) destination = PAR_NONE;
        assert(status == SET_P1_DEST || status == SET_P2_DEST);
        activeParameters[status == SET_P1_DEST ? PAR_P1_DEST : PAR_P2_DEST] = paramToModTarget[destination];
        display_needs_clear_all = 1;
        position = 0;
    }
}
