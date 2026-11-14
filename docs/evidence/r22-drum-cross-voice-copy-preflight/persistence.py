from pathlib import Path
import importlib.util,json,os,subprocess,sys,time
sys.dont_write_bytecode=True
root=Path(os.environ.get('LXR_SOURCE_ROOT','/workspace/lxr-desktop-app'));out=Path(__file__).resolve().parent
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()=='861c643632e0a88bdc7d60d9410befb3971489fc', 'Use the exact pinned R21 base'
sp=importlib.util.spec_from_file_location('fixture',root/'tests/project_storage/run.py');f=importlib.util.module_from_spec(sp);sp.loader.exec_module(f)
(out/'functions.inc').write_text(f.production_source())
s=(root/'tests/project_transaction/whole.c').read_text();s=s[:s.index('int main(int argc,char **argv)')]
s+=r'''
int main(void)
{
 for(unsigned both=0;both<2;both++){
  seed(0);menu_loadedPresetNr[CURRENT_KIT]=9;menu_selectedPresetNr[SAVE_TYPE_KIT]=23;
  activeParameters[PAR_COARSE1]=123;activeParameters[PAR_COARSE2]=77;
  mainParameters[9][PAR_COARSE1]=10;mainParameters[9][PAR_COARSE2]=11;
  /* Deliberately naive proposed R22 strategy, not an existing R21 gesture. */
  mainParameters[9][PAR_COARSE2]=activeParameters[PAR_COARSE1];
  if(both)activeParameters[PAR_COARSE2]=activeParameters[PAR_COARSE1];
  assert(preset_saveProject(0)==FR_OK);assert(preset_loadProject(0,0)==FR_OK);
  printf("%s: stored source=%u stored target=%u (wanted123), saved kit bytes=%u\n",
   both?"active+stored negative control":"stored-only expected failure",
   mainParameters[9][PAR_COARSE1],mainParameters[9][PAR_COARSE2],END_OF_SOUND_PARAMETERS);
  assert(mainParameters[9][PAR_COARSE1]==123);
  assert(mainParameters[9][PAR_COARSE2]==(both?123:77));
 }
 puts("Storage-only strategy rejected; updating active+stored can preserve RAM persistence but does not resolve live LFO assignment containment.");
 return 0;
}
'''
(out/'persistence.c').write_text(s)
flags=f.flags+['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration']
flags+=['-I'+str(root/p) for p in f.includes]+['-I'+str(out)]
cmd=flags+[str(out/'persistence.c'),str(root/'Core/Src/Preset/initKit.c'),'-o',str(out/'persistence')]
env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0';records=[]
with (out/'persistence.log').open('w') as log:
 for args in [cmd,[str(out/'persistence')]]:
  start=time.monotonic();r=subprocess.run(args,cwd=root,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
  log.write(r.stdout);print(r.stdout,end='');records.append({'argv':args,'exit_code':r.returncode,'host_elapsed_seconds':time.monotonic()-start})
  if r.returncode:break
(out/'persistence-results.json').write_text(json.dumps({'source_sha':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),'runs':records},indent=2)+'\n')
sys.exit(records[-1]['exit_code'])
