#!/usr/bin/env python3
"""Project containment production-body and whole-FatFs RAM tests; no hardware access."""
from pathlib import Path
import importlib.util, os, re, subprocess, sys, tempfile
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('save_helpers', ROOT/'tests/save_errors/run.py')
h = importlib.util.module_from_spec(spec)
spec.loader.exec_module(h)
source = h.preset_source()
helpers = ['preset_sharedStatus','preset_openShared','preset_closeShared',
           'preset_writeChecked','preset_finishSave','preset_finishRead',
           'preset_projectPath','preset_createProjectFolder','preset_childPath',
           'preset_isTmpFile','preset_defaultKit','preset_initTmpFolder',
           'preset_clearTmpFolder','preset_isNameEmpty','preset_makeFileNameOld',
           'preset_makeFileNameLoad','clearIllegalCharacters','preset_makeFileNameSave',
           'interpolate','preset_getMorphValue','preset_init','preset_loadDrumsetRam',
           'preset_saveDrumsetRam','preset_savePatternRAM','preset_readDrumsetData','preset_loadDrumsetSD',
           'preset_readPatternLengths','preset_readPatternData','preset_loadPatternSD',
           'preset_loadSongSD','preset_LoadTmpKit','preset_LoadTmpPat',
           'preset_loadGlobalFile','preset_loadGlobals','preset_loadDefaultGlobals',
           'preset_loadLastUsedProjectNrFromSD','preset_loadProjectNames',
           'preset_saveProject','preset_loadProject','preset_createProject']
helpers += h.WRITERS
helpers += ['preset_writeProjectData']
transaction = (ROOT/'Core/Src/Preset/projectStorage.inc').read_text()
transaction_names = re.findall(r'^(?:static\s+)?(?:FRESULT|uint8_t|uint32_t|int16_t|void)\s+(preset_\w+)\s*\(', transaction, re.M)
helpers += transaction_names
assert len(helpers) == len(set(helpers))
assert not re.search(r'\b(?:empty_directory|delete_node)\s*\(', source)
for name in ['preset_saveProject','preset_loadProject','preset_clearTmpFolder']:
    code = h.extract(source, name)
    assert not re.search(r'\b(?:f_mount|f_mkfs|f_rename|__disable_irq|malloc|free|HAL_Delay)\s*\(', code)
assert 'f_unlink(' not in h.extract(source, 'preset_saveProject')
assert 'preset_txnRun(presetNr, folder)' in h.extract(source, 'preset_saveProject')
assert 'f_mkdir(' not in h.extract(source, 'preset_loadProject')
assert 'res = preset_writeChecked' not in h.extract(source, 'preset_loadProject')
assert 'if(res == FR_OK) res = preset_loadProject(lastUsedProject, 0);' in (ROOT/'Core/Src/main.c').read_text()
format_body = h.extract((ROOT/'Core/Src/main.c').read_text(),'sd_formatRoutine')
assert 'res = preset_saveProject(0);' in format_body
assert 'lcd_string(res == FR_OK ? "Success!" : "Error-Please try");' in format_body

includes = ['.','Core/Inc','Drivers/CMSIS/Include',
            'Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc',
            'Middlewares/Third_Party/FatFs/src','FATFS/App','FATFS/Target']
env = os.environ.copy()
env['ASAN_OPTIONS'] = 'detect_leaks=0'
flags = [env.get('CC','cc'),'-std=gnu11','-O1','-g','-fsanitize=address,undefined',
         '-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie',
         '-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
def production_source():
    # Prototypes retain source ownership/order without rewriting production bodies.
    prototypes = []
    for n in helpers:
        definition = h.extract(source, n)
        prototypes.append(definition[:definition.index('{')].rstrip()+';\n')
    prefix=transaction[:transaction.index('static FRESULT preset_txnMutation')]
    code = prefix + ''.join(prototypes) + ''.join(h.extract(source,n) for n in helpers)
    # RAM/sequence helpers are unchanged production bodies, not algorithm replicas.
    seq = (ROOT/'Core/Src/Soundengine/Sequencer/sequencer.c').read_text()
    names = ['seq_resetNote','seq_clearTrack','seq_clearPattern','seq_copyPatternToBackupBuffer']
    code += ''.join(h.extract(seq,n) for n in names)
    menu = (ROOT/'Core/Src/menu/menu.c').read_text()
    code = h.extract(menu,'numtostrpu') + h.extract(menu,'numtostrpu_2Bytes') + code
    return code

def main():
    with tempfile.TemporaryDirectory(prefix='lxr-project-storage-') as d:
        d = Path(d)
        code=production_source()
        (d/'functions.inc').write_text(code)
        cmd = flags + ['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast',
                       '-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration']
        cmd += ['-I'+str(ROOT/p) for p in includes] + ['-I'+str(d),str(HERE/'host.c'),
                str(ROOT/'Core/Src/Preset/initKit.c'),'-o',str(d/'host')]
        subprocess.run(cmd,cwd=ROOT,env=env,check=True)
        subprocess.run([str(d/'host')],cwd=ROOT,env=env,check=True,timeout=60)
        names = ['preset_projectPath','preset_createProjectFolder','preset_childPath']
        (d/'gate.inc').write_text(''.join(h.extract(source,n) for n in names))
        cmd = flags + ['-I'+str(p) for p in [d,ROOT,ROOT/'tests/fatfs_integrity',
                ROOT/'FATFS/Target',ROOT/'Middlewares/Third_Party/FatFs/src',ROOT/'tests/sd_storage']]
        cmd += [str(HERE/'filesystem.c'),'-o',str(d/'filesystem')]
        subprocess.run(cmd,cwd=ROOT,env=env,check=True)
        subprocess.run([str(d/'filesystem')],cwd=ROOT,env=env,check=True,timeout=20)
    print('PASS S4b-1 static scope: no recursive deletion or remount/reset; bounded replacement is tested separately; no new filesystem path in ISR sources')

if __name__ == '__main__':
    main()
