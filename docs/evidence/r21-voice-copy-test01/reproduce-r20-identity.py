from pathlib import Path
import hashlib, importlib.util, json, os, subprocess, sys
sys.dont_write_bytecode = True
root = Path(os.environ.get('LXR_SOURCE_ROOT', '/workspace/lxr-desktop-app'))
out = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('fixture', root/'tests/project_storage/run.py')
f = importlib.util.module_from_spec(spec); spec.loader.exec_module(f)
(out/'functions.inc').write_text(f.production_source())
whole = (root/'tests/project_transaction/whole.c').read_text()
whole = whole[:whole.index('int main(int argc,char **argv)')]
whole += r'''
int main(void) {
 seed(0);
 menu_loadedPresetNr[CURRENT_KIT] = 9;
 menu_selectedPresetNr[SAVE_TYPE_KIT] = 1;
 mainParameters[9][PAR_COARSE5] = 11;
 activeParameters[PAR_COARSE5] = 77;
 mainParameters[1][PAR_COARSE5] = 99;
 assert(preset_saveTmpKit() == FR_OK);
 FIL file = {0}; BYTE slot; UINT count;
 assert(f_open(&file, "/tmp/tmp.snd", FA_READ) == FR_OK);
 assert(f_read(&file, &slot, 1, &count) == FR_OK && count == 1);
 assert(f_close(&file) == FR_OK);
 printf("TMP identity: loaded=%u cursor=%u serialized=%u (expected loaded=9)\n",
  menu_loadedPresetNr[CURRENT_KIT], menu_selectedPresetNr[SAVE_TYPE_KIT], slot);
 assert(slot == 1);
 assert(preset_saveProject(0) == FR_OK);
 printf("SAVE identity: loaded source RAM=%u target RAM=%u active edit=%u\n",
  mainParameters[9][PAR_COARSE5], mainParameters[1][PAR_COARSE5], activeParameters[PAR_COARSE5]);
 assert(mainParameters[9][PAR_COARSE5] == 11);
 assert(mainParameters[1][PAR_COARSE5] == 77);
 assert(preset_loadProject(0, 0) == FR_OK);
 printf("ROUNDTRIP: source stored=%u (wanted77), target stored=%u (wanted99)\n",
  mainParameters[9][PAR_COARSE5], mainParameters[1][PAR_COARSE5]);
 assert(mainParameters[9][PAR_COARSE5] == 11 && mainParameters[1][PAR_COARSE5] == 77);
 puts("EXPECTED BASELINE DEFECT REPRODUCED: cursor redirects source capture and overwrites target; no R21 implementation under test.");
 return 0;
}
'''
(out/'probe.c').write_text(whole)
cmd=f.flags+['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration']
cmd+=['-I'+str(root/p) for p in f.includes]+['-I'+str(out),str(out/'probe.c'),str(root/'Core/Src/Preset/initKit.c'),'-o',str(out/'probe')]
env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
records=[]
with (out/'reproduction.log').open('w') as log:
 for args in [cmd,[str(out/'probe')]]:
  r=subprocess.run(args,cwd=root,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
  log.write(r.stdout);print(r.stdout,end='')
  records.append({'argv':args,'exit_code':r.returncode})
  if r.returncode: break
(out/'results.json').write_text(json.dumps({'source_sha':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),'runs':records},indent=2)+'\n')
sys.exit(records[-1]['exit_code'])
