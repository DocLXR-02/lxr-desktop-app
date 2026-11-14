int main(void)
{
 menu_activePage=SAVE_PAGE;menu_saveOptions.category=SAVE_TYPE_KIT;menu_saveOptions.nameBuffer=KIT_NAMES;
 memcpy(savefileNames[0][KIT_NAMES],"ABCDEFGH",8);copyNameToEditBuffer(0,KIT_NAMES);
 menu_saveOptions.state=SAVE_STATE_EDIT_NAME1;editModeActive=0;
 menu_handleLoadSaveMenu(1,0);assert(menu_saveOptions.state==SAVE_STATE_EDIT_NAME2&&nameEditBuffer[0]==1);
 visibleCursor=0;menu_repaintLoadSavePage();assert(visibleCursor==VIS_CURS(1,6,1));
 editModeActive=1;menu_handleLoadSaveMenu(1,1);assert(menu_saveOptions.state==SAVE_STATE_EDIT_NAME2&&nameEditBuffer[1]==3);
 menu_repaintLoadSavePage();assert(editDisplayBuffer[1][5]=='['&&editDisplayBuffer[1][6]=='C'&&editDisplayBuffer[1][7]==']');
 editModeActive=0;menu_handleLoadSaveMenu(0,1);menu_handleLoadSaveMenu(-1,0);assert(menu_saveOptions.state==SAVE_STATE_EDIT_NAME1);
 for(unsigned n=0;n<20;n++)menu_handleLoadSaveMenu(1,0);
 assert(menu_saveOptions.state==SAVE_STATE_OK);menu_repaintLoadSavePage();assert(editDisplayBuffer[1][13]=='>');
 editModeActive=0;menu_handleLoadSaveMenu(-1,0);assert(menu_saveOptions.state==SAVE_STATE_EDIT_NAME8);
 editModeActive=1;nameEditBuffer[7]=0;menu_handleLoadSaveMenu(-1,0);assert(nameEditBuffer[7]==0);
 nameEditBuffer[7]=NUM_CHARS-1;menu_handleLoadSaveMenu(1,0);assert(nameEditBuffer[7]==NUM_CHARS-1);
 menu_saveOptions.state=SAVE_STATE_EDIT_PRESET_NR;menu_selectedPresetNr[SAVE_TYPE_KIT]=0;editModeActive=1;
 menu_handleLoadSaveMenu(-1,0);assert(menu_selectedPresetNr[SAVE_TYPE_KIT]==0);
 menu_handleLoadSaveMenu(1,0);assert(menu_selectedPresetNr[SAVE_TYPE_KIT]==1);
 menu_activePage=LOAD_PAGE;editModeActive=1;menu_handleLoadSaveMenu(-1,0);assert(menu_selectedPresetNr[SAVE_TYPE_KIT]==0);
 copyEditBufferToName(1,KIT_NAMES);assert(savefileNames[1][KIT_NAMES][1]=='B');
 puts("PASS TEST02 supplementary real unchanged save/load handler + name renderer: navigate/edit character/exit/OK/back, brackets/underline, character bounds and slot selection; no storage/engine adapter called");
}
