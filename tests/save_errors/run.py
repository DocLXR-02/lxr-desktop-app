#!/usr/bin/env python3
"""Production save bodies, frozen S3 byte oracle, sanitized fault and UI tests."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
BASE='cbffa2d756658b6f32d8e6417ea40ec2cda828d5'
PRESET='Core/Src/Preset/presetManager.c'
WRITERS=['preset_writeDrumsetData','preset_writeGlobalData','preset_writePatternData',
         'preset_saveTmpKit','preset_saveTmpPat','preset_saveDrumsetSD','preset_saveGlobals',
         'preset_saveLastUsedProjectNrToSD','preset_savePatternSD','preset_saveSongSD']
def extract(text,name):
    masked=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),text,flags=re.S)
    m=re.search(r'^(?:static\s+)?(?:void|FRESULT|u?int(?:8|16|32)_t)\s+'+name+r'\([^;]*?\)\s*\{',masked,re.M)
    if not m:raise RuntimeError('Cannot extract '+name)
    i=m.end();depth=1
    while depth:depth+=(masked[i]=='{')-(masked[i]=='}');i+=1
    return text[m.start():i]+'\n'
def frozen(text):
    license=text[:text.index('*/')+2]
    return '/* Frozen successful-write oracle from S3 '+BASE+'. */\n'+license+'\n'+''.join(extract(text,n) for n in WRITERS)
def case(text,name,next_name):
    start=text.index('case '+name+':');end=text.index('case '+next_name+':',start)
    return text[start:end]
def preset_source():
    text=(ROOT/PRESET).read_bytes().decode('latin1').replace('\r\n','\n')
    include='#include "projectStorage.inc"'
    if include in text:
        text=text.replace(include,(ROOT/'Core/Src/Preset/projectStorage.inc').read_text())
    return text
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--verify-baseline',action='store_true');parser.add_argument('--autosave-regressions',action='store_true');args=parser.parse_args()
    snapshot=(HERE/'baseline_s3.inc').read_text()
    assert hashlib.sha256(snapshot.encode()).hexdigest()==(HERE/'baseline_s3.sha256').read_text().strip()
    if args.verify_baseline:
        base=subprocess.check_output(['git','show',BASE+':'+PRESET],cwd=ROOT).decode('latin1').replace('\r\n','\n')
        assert frozen(base)==snapshot
        print('PASS frozen oracle exactly matches pinned S3 production function bodies',flush=True)
    text=preset_source()
    funcs=['preset_sharedStatus','preset_openShared','preset_closeShared','preset_writeChecked',
           'preset_finishSave','preset_childPath','preset_isTmpFile','interpolate','preset_getMorphValue']+WRITERS+['preset_resetAllSaveTimers','preset_touchTimer','preset_completeAutosave','preset_autosaveCanRetry','preset_autosaveOne','preset_processTimer','preset_initTmpFolder','preset_clearTmpFolder','preset_makeFileNameLoad','preset_loadDefaultGlobals','preset_createGlobals']
    names='\n'.join('#define '+n+' baseline_'+n for n in WRITERS+['interpolate','preset_getMorphValue'])
    unnames='\n'.join('#undef '+n for n in WRITERS+['interpolate','preset_getMorphValue'])
    menu=(ROOT/'Core/Src/menu/menu.c').read_text()
    frozen_morph=(HERE/'baseline_morph_s3.inc').read_text()
    for name,digest in json.loads((HERE/'common_s3.json').read_text()).items():
        source_text=menu if name=='numtostrpu_2Bytes' else frozen_morph if name in ['interpolate','preset_getMorphValue'] else text
        assert hashlib.sha256(extract(source_text,name).encode()).hexdigest()==digest
    assert extract(text,'preset_getMorphValue')==extract(frozen_morph,'preset_getMorphValue')
    source=extract(menu,'numtostrpu_2Bytes')+extract(text,'clearIllegalCharacters')+extract(text,'preset_makeFileNameSave')
    source+=text[text.index('/* Autosave retry state:'):text.index('/* End autosave retry state. */')+len('/* End autosave retry state. */')]+'\n'
    source+=''.join(extract(text,n) for n in funcs)
    source+='\n'+names+'\n#include "baseline_morph_s3.inc"\n#include "baseline_s3.inc"\n'+unnames+'\n'
    menu=(ROOT/'Core/Src/menu/menu.c').read_text()
    area=menu[menu.index('void menu_handleLoadSaveMenu('):]
    source+='void exercise_menu_save(unsigned category) { switch(category) {\n'+case(area,'SAVE_TYPE_PATTERN','SAVE_TYPE_KIT')+case(area,'SAVE_TYPE_KIT','SAVE_TYPE_MORPH')+case(area,'SAVE_TYPE_GLO','SAVE_TYPE_PROJECT')+'} menu_resetSaveParameters(); }\n'
    button=(ROOT/'Core/Src/buttonHandler.c').read_text();start=button.index('if(menu_getActivePage() == PATTERN_SAVE_PAGE)');brace=button.index('{',start);i=brace+1;depth=1
    while depth:depth+=(button[i]=='{')-(button[i]=='}');i+=1
    source+='void exercise_button_save(uint8_t seqButtonPressed) '+button[brace:i]+'\n'
    # Direct safety preconditions precede destructive lifecycle entry points.
    for name in ['preset_createProject','preset_saveProject','preset_loadProject','preset_loadProjectNames','loadTmpFolderFiles','preset_makeFileNameLoad']:
        body=extract(text,name);assert 'preset_sharedStatus()' in body[:body.index('{')+180]
    assert 'f_close(' not in extract(text,'preset_finishSave')
    project=extract(text,'preset_writeProjectData')
    start=project.index('res = preset_saveGlobals();');end=project.index('//save all drumkits',start)
    assert 'if(res != FR_OK) return res;' in project[start:end]
    assert 'preset_clearTmpFolder' not in project[start:end]
    includes=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include',
              'Drivers/STM32F7xx_HAL_Driver/Inc','Middlewares/Third_Party/FatFs/src','FATFS/App','FATFS/Target']
    with tempfile.TemporaryDirectory(prefix='lxr-save-errors-') as directory:
        directory=Path(directory);(directory/'functions.inc').write_text(source)
        arm=shutil.which('arm-none-eabi-gcc') or '/workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc'
        preprocessed=subprocess.check_output([arm,'-E','-DSTM32F765xx','-DUSE_HAL_DRIVER']+['-I'+str(ROOT/p) for p in includes]+[str(ROOT/PRESET)],text=True)
        allowed={}
        for api,owner in [('f_open','preset_openShared'),('f_close','preset_closeShared'),('f_write','preset_writeChecked')]:
            body=extract(text,owner);start=text.index(body.rstrip());line=text.count('\n',0,start)+1
            allowed[api]=set(range(line,line+body.count('\n')+1))
        project=extract(text,'preset_saveProject');start=text.index(project.rstrip());line=text.count('\n',0,start)+1
        allowed['f_write'].update(range(line,line+project.count('\n')+1))
        current='';number=0
        for item in preprocessed.splitlines():
            mark=re.match(r'# (\d+) "([^"]+)"',item)
            if mark:number=int(mark[1]);current=mark[2];continue
            if current.endswith('/presetManager.c'):
                for api,lines in allowed.items():
                    if re.search(r'\b'+api+r'\s*\(',item):assert number in lines,(api,number)
            number+=1
        print('PASS enabled-source ownership scan: only wrappers open/close; raw writes limited to checked helper and deferred project header',flush=True)
        flags=[os.environ.get('CC','cc'),'-std=gnu11','-O1','-g','-DSTM32F765xx','-DUSE_HAL_DRIVER',
               '-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration',
               '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer',
               '-fno-pie','-no-pie','-ffunction-sections','-fdata-sections']
        flags+=['-I'+str(ROOT/p) for p in includes]+['-I'+str(directory),'-I'+str(HERE)]
        binary=directory/'host';env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
        host=ROOT/'tests/silent_autosave/host.c' if args.autosave_regressions else HERE/'host.c'
        subprocess.run(flags+[str(host),'-Wl,--gc-sections','-o',str(binary)],cwd=ROOT,check=True,env=env)
        subprocess.run([str(binary)],cwd=ROOT,check=True,env=env,timeout=40)
if __name__=='__main__':main()
