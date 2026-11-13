#include <assert.h>

#define CELL(p,v,s) seq_patternSet.seq_subStepPattern[p][v][s]

static void expect_row(int row, const char *expected)
{
    char padded[17];
    int length = snprintf(padded, sizeof(padded), "%-16s", expected);
    assert(length == 16);
    if (memcmp(currentDisplayBuffer[row], padded, 16)) {
        fprintf(stderr, "LCD row %d: expected [%s], actual [%.*s]\n", row, padded,
                16, currentDisplayBuffer[row]);
        abort();
    }
}

static void time_to(uint32_t tick)
{
    now = tick;
    for (unsigned pass = 0; pass < 512; ++pass) menu_tick();
}

static void reset_fixture(void)
{
    memset(&seq_patternSet, 0, sizeof(seq_patternSet));
    for (int p = 0; p < NUM_PATTERN; ++p) {
        for (int v = 0; v < NUM_TRACKS; ++v) {
            seq_patternSet.length[p][v] = 4;
            for (int s = 0; s < NUM_STEPS; ++s) {
                CELL(p,v,s).param1Nr = NO_AUTOMATION;
                CELL(p,v,s).param2Nr = NO_AUTOMATION;
            }
        }
    }
    seq_autoMode = seq_autoSession = seq_autoBlocked = seq_recordActive = 0;
    seq_running = 1;
    seq_activePattern = frontParser_shownPattern = frontParser_activeTrack = seq_activeAutomTrack = 0;
    memset(activeParameters, 0, sizeof(activeParameters));
    seq_armedArmedAutomationStep = seq_armedArmedAutomationTrack = -1;
    memset(&seq_autoCapture, 0, sizeof(seq_autoCapture));
    memset(midiParser_originalCcValues, 0, sizeof(midiParser_originalCcValues));
    buttonHandler_syncRecordingState(0);
    menu_activePage = VOICE1_PAGE;
    menu_autoNotice = menu_kitLoadNotice = menu_sdError = 0;
    visibleCursor = display_refresh_state = 0;
    tick_interrupt = 0;
    menu_repaintAll();
    time_to(100);
}

static void knob(uint8_t id, uint8_t value)
{
    if (id >= 128) frontPanel_sendData(FRONT_CC_LIVE_HIGH, id - 128, value);
    else frontPanel_sendData(FRONT_CC_LIVE_LOW, id - 1, value);
}

static void arm(void)
{
    frontPanel_sendData(SEQ_CC, FRONT_SEQ_SET_REC_MODE, 1);
    press_rec();
}

static void finish_loop(void)
{
    for (uint8_t step = 1; step < 4; ++step) seq_autoRecordStepTick(step);
    seq_autoRecordStepTick(0);
}

static void test_recording(void)
{
    reset_fixture();
    CELL(0,0,63).param2Nr = PAR_COARSE1;
    arm();
    assert(rec_led && seq_autoSession);
    time_to(100);
    expect_row(0, "REC trk1 OFF");
    expect_row(1, "    trk2 COA");
    knob(PAR_FINE1, 5);
    assert(!seq_autoCapture.changed);
    seq_autoRecordStepTick(0);
    knob(PAR_COARSE1, 13);
    time_to(100);
    expect_row(0, "PARAM BLOCKED!");
    expect_row(1, "COA on trk2");
    assert(rec_led && seq_autoSession && midiParser_originalCcValues[PAR_COARSE1] == 13);
    assert(CELL(0,0,63).param2Nr == PAR_COARSE1 && CELL(0,0,0).param1Nr == NO_AUTOMATION);
    time_to(950);
    knob(PAR_COARSE1, 14);
    time_to(2449);
    expect_row(1, "COA on trk2");
    time_to(2450);
    expect_row(0, "VOICE PAGE");

    knob(PAR_FINE1, 60);
    time_to(2450);
    expect_row(0, "REC trk1 FIN");
    knob(PAR_COARSE1, 15);
    time_to(2451);
    expect_row(1, "trk1: FIN ONLY");
    finish_loop();
    for (int s = 0; s < 4; ++s) assert(CELL(0,0,s).param1Nr == PAR_FINE1 && CELL(0,0,s).param1Val == 60);
    knob(PAR_COARSE1, 16);
    time_to(2452);
    expect_row(1, "trk1: FIN ONLY");
    finish_loop();
    assert(CELL(0,0,0).param1Val == 60);
    knob(PAR_FINE1, 80);
    finish_loop();
    assert(CELL(0,0,0).param1Val == 80);
    knob(PAR_FINE1, 90);
    seq_setRunning(0);
    knob(PAR_FINE1, 91);
    seq_setRunning(1);
    seq_autoRecordStepTick(0);
    finish_loop();
    assert(CELL(0,0,0).param1Val == 80 && rec_led);
    press_rec();
    assert(!rec_led && !seq_autoSession);

    reset_fixture();
    seq_activeAutomTrack = 1;
    CELL(0,0,0).param1Nr = PAR_COARSE1;
    CELL(0,0,0).param2Nr = PAR_FILTER_FREQ_1;
    arm();
    time_to(100);
    expect_row(0, "REC trk2 FRQ");
    expect_row(1, "    trk1 COA");
    knob(PAR_FINE1, 10);
    assert(!seq_autoCapture.started && !seq_autoCapture.changed);
    seq_autoRecordStepTick(0);
    knob(PAR_FINE1, 11);
    time_to(101);
    expect_row(1, "trk2: FRQ ONLY");

    reset_fixture();
    CELL(0,0,0).param1Nr = PAR_COARSE1;
    CELL(0,0,63).param1Nr = PAR_FINE1;
    arm();
    time_to(100);
    expect_row(1, "trk1 MULTI PARAM");
    assert(!rec_led && !buttonHandler_stateMemory.seqRecording && !seq_autoSession);
    assert(CELL(0,0,0).param1Nr == PAR_COARSE1 && CELL(0,0,63).param1Nr == PAR_FINE1);
    seq_activeAutomTrack = 1;
    press_rec();
    assert(rec_led && seq_autoSession);
    time_to(100);
    expect_row(1, "    trk1 MUL");
    press_rec();
    seq_clearAutomation(0, 0, 0);
    assert(CELL(0,0,0).param1Nr == NO_AUTOMATION && CELL(0,0,63).param1Nr == NO_AUTOMATION);
    puts("PASS REC: both layouts, OFF/MUL, first assignment, local lock, retry, LED, commits, silence, transport STOP, clear");
}

static void test_global(void)
{
    static const struct { uint8_t id; const char *name; } targets[] = {
        {PAR_VOICE_DECIMATION_ALL,"0sr"}, {PAR_FX_TYPE,"FTY"}, {PAR_FX_ROUTING,"RTE"},
        {PAR_FX_DRY_WET,"D/W"}, {PAR_FX_DIST_TYPE,"DTY"}, {PAR_FX_P1,"P1"},
        {PAR_FX_P2,"P2"}, {PAR_FX_P3,"P3"}, {PAR_FX_P4,"P4"},
        {PAR_FX_RATIO,"RAT"}, {PAR_FX_WAVE,"WAV"}, {PAR_FX_DELAY_TYPE,"DLY"},
    };
    for (unsigned i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
        for (int inverse = 0; inverse < 2; ++inverse) {
            reset_fixture();
            uint8_t id = targets[i].id;
            int owner = inverse ? 0 : 6;
            int voice = inverse ? 6 : 0;
            frontParser_activeTrack = voice;
            seq_activeAutomTrack = inverse ? 0 : 1;
            if (inverse) CELL(0,owner,0).param2Nr = id;
            else CELL(0,owner,63).param1Nr = id;
            arm();
            seq_autoRecordStepTick(0);
            knob(id, 37);
            time_to(100);
            char line[17];
            snprintf(line, sizeof(line), "%s on V%c trk%c", targets[i].name,
                     inverse ? '1' : '7', inverse ? '2' : '1');
            expect_row(0, "PARAM BLOCKED!");
            expect_row(1, line);
            finish_loop();
            assert(CELL(0,voice,0).param1Nr == NO_AUTOMATION && CELL(0,voice,0).param2Nr == NO_AUTOMATION);
            seq_clearAutomation(owner, 0, inverse ? 1 : 0);
            CELL(1,owner,0).param1Nr = id;
            knob(id, 38);
            finish_loop();
            assert((inverse ? CELL(0,voice,0).param1Nr : CELL(0,voice,0).param2Nr) == id);
            assert(CELL(1,owner,0).param1Nr == id);
        }
    }
    reset_fixture();
    CELL(0,0,0).param1Nr = PAR_FX_P4;
    CELL(0,4,63).param2Nr = PAR_FX_P4;
    arm();
    seq_autoRecordStepTick(0);
    knob(PAR_FX_P4, 55);
    finish_loop();
    assert(CELL(0,0,0).param1Val == 55 && CELL(0,4,63).param2Nr == PAR_FX_P4);
    press_rec();
    frontParser_activeTrack = 6;
    arm();
    seq_autoRecordStepTick(0);
    knob(PAR_FX_P4, 56);
    time_to(100);
    expect_row(1, "P4 on V1 trk1");
    assert(!seq_autoCapture.changed);

    reset_fixture();
    CELL(0,4,2).param1Nr = PAR_COARSE1;
    arm();
    seq_autoRecordStepTick(0);
    knob(PAR_COARSE1, 30);
    finish_loop();
    assert(CELL(0,0,0).param1Nr == PAR_COARSE1 && CELL(0,4,2).param1Nr == PAR_COARSE1);
    puts("PASS shared: all 12 IDs across both lanes and boundary voices, conflicts, free controls, existing duplicates and pattern isolation");
}

static void test_context(void)
{
    for (int change = 0; change < 4; ++change) {
        reset_fixture();
        arm();
        seq_autoRecordStepTick(0);
        knob(PAR_FINE1, 55);
        if (change == 0) frontParser_activeTrack = 1;
        if (change == 1) seq_activeAutomTrack = 1;
        if (change == 2) seq_activePattern = 1;
        if (change == 3) seq_patternSet.length[0][0] = 8;
        seq_autoRecordStepTick(1);
        assert(!seq_autoCapture.active && CELL(0,0,0).param1Nr == NO_AUTOMATION);
        assert(rec_led && seq_autoSession);
        press_rec();
        assert(!rec_led && !seq_autoSession);
    }
    reset_fixture();
    press_rec();
    knob(PAR_COARSE1, 33);
    seq_stepIndex[0] = 2;
    frontParser_shownPattern = 1; /* Omit unrelated LED feedback. */
    note_set = 0;
    seq_addNote(0, 90, 64);
    assert(note_set == 1 && CELL(0,0,2).note == 64);
    assert(CELL(0,0,2).param1Nr == NO_AUTOMATION && CELL(0,0,2).param2Nr == NO_AUTOMATION);
    seq_autoRecordStepTick(0);
    assert(seq_recordActive && rec_led && CELL(0,0,0).param1Nr == NO_AUTOMATION);
    press_rec();
    assert(!seq_recordActive && !rec_led);
    puts("PASS context: voice/lane/pattern/length invalidates pending take; MIDI REC records a note without writing a lane");
    puts("OBSERVED context: REC LED remains on after context invalidation until REC is toggled off");
}

static void test_manual(void)
{
    for (int lane = 0; lane < 2; ++lane) {
        reset_fixture();
        const uint16_t dest = lane ? PAR_P2_DEST : PAR_P1_DEST;
        const uint16_t val = lane ? PAR_P2_VAL : PAR_P1_VAL;
        activeParameters[PAR_ACTIVE_STEP] = 3;
        manual_menu(dest, paramToModTarget[PAR_FX_P3]);
        manual_menu(val, 14);
        Step *step = &CELL(0,0,3);
        assert((lane ? step->param2Nr : step->param1Nr) == PAR_FX_P3);
        assert((lane ? step->param2Val : step->param1Val) == 14);
        CELL(0,4,0).param1Nr = PAR_FX_P4;
        assert(!fix8_canAssignAutomationDestination(0, 0, lane, PAR_FX_P4));
        activeParameters[dest] = paramToModTarget[PAR_FX_P4];
        manual_menu(dest, activeParameters[dest]);
        time_to(100);
        expect_row(1, lane ? "trk2: P3 ONLY" : "trk1: P3 ONLY");
        assert((lane ? step->param2Nr : step->param1Nr) == PAR_FX_P3);
        manual_menu(val, 77);
        assert(activeParameters[dest] == paramToModTarget[PAR_FX_P3]);
        assert((lane ? step->param2Val : step->param1Val) == 77);
        activeParameters[dest] = paramToModTarget[PAR_NONE];
        manual_menu(dest, activeParameters[dest]);
        assert((lane ? step->param2Nr : step->param1Nr) == PAR_NONE);
        assert(activeParameters[dest] == paramToModTarget[PAR_NONE]);
        assert(fix8_canAssignAutomationDestination(0, 0, lane, PAR_FX_P4));
        activeParameters[dest] = paramToModTarget[PAR_FX_P4];
        manual_menu(dest, activeParameters[dest]);
        time_to(100);
        expect_row(1, "P4 on V5 trk1");
        assert((lane ? step->param2Nr : step->param1Nr) == PAR_NONE);
        assert(activeParameters[dest] == paramToModTarget[PAR_NONE]);
    }
    reset_fixture();
    seq_armedArmedAutomationStep = 2;
    seq_armedArmedAutomationTrack = 0;
    seq_activeAutomTrack = 1;
    knob(PAR_FINE1, 50);
    assert(CELL(0,0,2).param2Nr == PAR_FINE1 && CELL(0,0,2).param2Val == 50);
    knob(PAR_COARSE1, 90);
    time_to(100);
    expect_row(1, "trk2: FIN ONLY");
    assert(CELL(0,0,2).param2Nr == PAR_FINE1 && CELL(0,0,2).param2Val == 50);
    puts("BASELINE FIX8 guard rejects a different target on an occupied lane and accepts P4 on a free lane despite a cross-voice owner");
    puts("PASS manual controls: accepted writes, rejected destination preserves stored bytes at rejection, clear/OFF, shared conflict, hold-STEP block");
}

static void test_timing(void)
{
    for (int wrap = 0; wrap < 2; ++wrap) {
        const uint32_t start = wrap ? UINT32_MAX - 600 : 500;
        for (int notice = 0; notice < 10; ++notice) {
            reset_fixture();
            now = start;
            if (notice < 2) menu_showAutomationOverview(notice, PAR_COARSE1, NO_AUTOMATION);
            else if (notice < 6) menu_showAutomationBlocked(notice-1, PAR_COARSE1, 0, 6, 1);
            else if (notice < 8) menu_showKitLoad(notice-6);
            else menu_showSaveSuccess(notice == 8 ? KIT_SAVE_PAGE_OK : PATTERN_SAVE_PAGE_OK);
            time_to(start);
            char initial[2][16];
            memcpy(initial, currentDisplayBuffer, sizeof(initial));
            assert(memcmp(initial[0], "VOICE PAGE", 10) != 0);
            time_to(start + 1499);
            assert(!memcmp(initial, currentDisplayBuffer, sizeof(initial)));
            time_to(start + 1500);
            expect_row(0, "VOICE PAGE");
        }
    }
    reset_fixture();
    menu_showAutomationBlocked(MENU_AUTO_BLOCK_GLOBAL, PAR_FX_P4, 0, 4, 0);
    now = 400;
    menu_showSdError(FR_NO_FILE);
    time_to(400);
    expect_row(0, "SD error");
    time_to(2400);
    expect_row(0, "VOICE PAGE");

    for (int pattern = 0; pattern < 2; ++pattern) {
        reset_fixture();
        menu_showAutomationBlocked(MENU_AUTO_BLOCK_GLOBAL, PAR_FX_P4, 0, 4, 0);
        now = 400;
        const char *success = pattern ? "Save Pattern" : "Save Kit";
        menu_showSaveSuccess(pattern ? PATTERN_SAVE_PAGE_OK : KIT_SAVE_PAGE_OK);
        time_to(400);
        expect_row(0, success);
        now = 800;
        menu_showAutomationBlocked(MENU_AUTO_BLOCK_GLOBAL, PAR_FX_P4, 0, 4, 0);
        time_to(1899);
        expect_row(0, success);
        time_to(1900);
        expect_row(0, "PARAM BLOCKED!");
        time_to(2300);
        expect_row(0, "VOICE PAGE");
    }
    reset_fixture();
    menu_showAutomationBlocked(MENU_AUTO_BLOCK_GLOBAL, PAR_FX_P4, 0, 4, 0);
    now = 400;
    menu_showKitLoad(1);
    time_to(400);
    expect_row(0, "Load Morph Kit");
    time_to(1899);
    expect_row(0, "Load Morph Kit");
    time_to(1900);
    expect_row(0, "VOICE PAGE");

    puts("PASS timer: all six REC variants and four save/load success display paths at 1499/1500 ms with and without wrap; kit-load/REC and SD/REC overlap controls restore without resurrection");
}

static void test_display_preemption(void)
{
    reset_fixture();
    menu_showAutomationBlocked(MENU_AUTO_BLOCK_GLOBAL, PAR_FX_P4, 0, 4, 0);
    time_to(1599);
    expect_row(1, "P4 on V5 trk1");
    /* Interrupt after foreground read of the HAL tick and before it renders.
       The simulated TIM14 tick expires REC and starts restoring the voice page. */
    tick_interrupt = 1;
    menu_repaint();
    time_to(1600);
    assert(menu_autoNotice == 0);
    expect_row(0, "VOICE PAGE");
    time_to(1900);
    expect_row(0, "VOICE PAGE");
    puts("PASS display preemption: expired REC screen does not remain after foreground resumes");
}

static void test_commit_conflict(void)
{
    reset_fixture();
    arm();
    seq_autoRecordStepTick(0);
    knob(PAR_FX_P4, 30);
    assert(seq_autoCapture.changed && CELL(0,0,0).param1Nr == NO_AUTOMATION);
    /* Pause before the loop commits and retain the pending target. The STEP
       destination path can run from another selected voice before the next tick. */
    frontPanel_sendData(SEQ_CC, FRONT_SEQ_SET_ACTIVE_TRACK, 4);
    activeParameters[PAR_ACTIVE_STEP] = 0;
    manual_menu(PAR_P1_DEST, paramToModTarget[PAR_FX_P4]);
    assert(CELL(0,4,0).param1Nr == NO_AUTOMATION);
    time_to(100);
    expect_row(1, "P4 on V1 trk1");
    frontPanel_sendData(SEQ_CC, FRONT_SEQ_SET_ACTIVE_TRACK, 0);
    finish_loop();
    assert(CELL(0,0,0).param1Nr == PAR_FX_P4);
    assert(CELL(0,4,0).param1Nr == NO_AUTOMATION);
    puts("PASS commit reservation: a competing manual STEP assignment is refused before the live take commits");
}

static void interrupt_first_movement(void)
{
    seq_recordAutomation(0, PAR_FX_P4, 42);
}

static void test_reservation_during_step_validation(void)
{
    reset_fixture();
    arm();
    seq_autoRecordStepTick(0);
    frontPanel_sendData(SEQ_CC, FRONT_SEQ_SET_ACTIVE_TRACK, 4);
    activeParameters[PAR_P1_DEST] = paramToModTarget[PAR_FX_P4];
    before_irq_mask = interrupt_first_movement;
    manual_menu(PAR_P1_DEST, activeParameters[PAR_P1_DEST]);
    assert(!before_irq_mask && seq_autoCapture.changed);
    assert(CELL(0,4,0).param1Nr == NO_AUTOMATION);
    assert(activeParameters[PAR_P1_DEST] == paramToModTarget[PAR_NONE]);
    time_to(100);
    expect_row(1, "P4 on V1 trk1");
    frontPanel_sendData(SEQ_CC, FRONT_SEQ_SET_ACTIVE_TRACK, 0);
    finish_loop();
    assert(CELL(0,0,0).param1Nr == PAR_FX_P4);
    puts("PASS commit reservation: retry after a simulated MIDI interrupt acquires a target during STEP validation");
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    paramToModTargetInit();
    test_recording();
    test_global();
    test_context();
    test_manual();
    test_timing();
    test_display_preemption();
    test_commit_conflict();
    test_reservation_during_step_validation();
    puts("R20 integration regressions: PASS");
    return 0;
}
