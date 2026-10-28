#!/usr/bin/env python3
"""S4-L production notification and full FatFs RAM-media tests. No hardware."""
from pathlib import Path
import importlib.util,os,re,subprocess,sys,tempfile
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2];HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('save_helpers',ROOT/'tests/save_errors/run.py')
helpers=importlib.util.module_from_spec(spec);spec.loader.exec_module(helpers)
text=(ROOT/'Core/Src/menu/menu.c').read_text()
functions=['menu_expireSdError','menu_renderSdError','menu_showSdError','menu_tick','menu_repaintAll','menu_repaint']
functions=['menu_showKitLoad','menu_renderKitLoad']+functions
source='static volatile uint8_t menu_kitLoadNotice; static volatile uint32_t menu_kitLoadSince;\n'+''.join(helpers.extract(text,n) for n in functions)
for name in functions[:3]:
    body=helpers.extract(text,name)
    assert not re.search(r'\b(?:HAL_Delay|HAL_TIM_\w+|lcd_\w+|sendDisplayBuffer|f_\w+|soundengine_process)\s*\(',body)
    assert not re.search(r'\b(?:while|for)\s*\(',body)
assert text.count('menu_expireSdError();')==1 and 'if(menu_renderSdError()) return;' in helpers.extract(text,'menu_repaint')
includes=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc','Middlewares/Third_Party/FatFs/src','FATFS/App','FATFS/Target']
with tempfile.TemporaryDirectory(prefix='lxr-live-storage-') as work:
    work=Path(work);(work/'functions.inc').write_text(source)
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
    flags=[os.environ.get('CC','cc'),'-std=gnu11','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie','-ffunction-sections','-fdata-sections','-Wl,--gc-sections']
    cmd=flags+['-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast']+['-I'+str(ROOT/p) for p in includes]+['-I'+str(work),str(HERE/'notifications.c'),'-o',str(work/'notifications')]
    subprocess.run(cmd,cwd=ROOT,env=env,check=True);subprocess.run([str(work/'notifications')],check=True,env=env,timeout=20)
    cmd=flags+['-I'+str(ROOT/'tests/fatfs_integrity'),'-I'+str(ROOT/'FATFS/Target'),'-I'+str(ROOT),'-I'+str(ROOT/'Middlewares/Third_Party/FatFs/src'),'-I'+str(ROOT/'tests/sd_storage'),str(HERE/'filesystem.c'),'-o',str(work/'filesystem')]
    subprocess.run(cmd,cwd=ROOT,env=env,check=True);subprocess.run([str(work/'filesystem')],check=True,env=env,timeout=20)
