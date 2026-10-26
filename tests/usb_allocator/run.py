#!/usr/bin/env python3
"""ASan/UBSan tests of extracted production USB lifecycle, core and app functions."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
CLASS='Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c'
CORE='Middlewares/JS/STM32_USB_Device_Library/Core/Src/usbd_core.c'
REQUEST='Middlewares/JS/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c'
APP='USB_DEVICE_MIDI/App/usbd_cdc_if.c'
def extract(path,name):
    source=(ROOT/path).read_text(errors='replace')
    masked=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),source,flags=re.S)
    match=re.search(r'^(?:static\s+)?(?:uint8_t|int8_t|void|USBD_StatusTypeDef)\s+'+name+r'\s*\([^;]*?\)\s*\{',masked,re.M)
    if not match: raise RuntimeError('Missing production function '+name)
    depth,end=1,match.end()
    while depth:
        depth+=(masked[end]=='{')-(masked[end]=='}');end+=1
    line=source.count('\n',0,match.start())+1
    return f'#line {line} "{path}"\n'+source[match.start():end]+'\n'
source=(ROOT/CLASS).read_text()
# Keep actual typed storage/owner declarations, not a copied test implementation.
preamble=source[source.index('/* One registered device/class instance:'):source.index('static uint8_t USBD_CDC_Release(')]
body=preamble+''.join(extract(CLASS,n) for n in ['USBD_CDC_Release','USBD_CDC_Init','USBD_CDC_DeInit','USBD_CDC_SetTxBuffer','USBD_CDC_SetRxBuffer'])
body+=''.join(extract(APP,n) for n in ['CDC_Init_FS','CDC_DeInit_FS'])
body+=''.join(extract(CORE,n) for n in ['USBD_SetClassConfig','USBD_ClrClassConfig','USBD_LL_Reset','USBD_LL_DevDisconnected','USBD_Stop'])
body+=extract(REQUEST,'USBD_SetConfig')
includes=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc','USB_DEVICE_MIDI/Target','Middlewares/JS/STM32_USB_Device_Library/Core/Inc','Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc']
with tempfile.TemporaryDirectory(prefix='lxr-usb-c1-') as directory:
    temp=Path(directory);(temp/'functions.inc').write_text(body)
    flags=[os.environ.get('CC','cc'),'-std=gnu11','-O1','-g','-DSTM32F765xx','-DUSE_HAL_DRIVER','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-Werror=implicit-function-declaration','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    flags+=['-I'+str(ROOT/p) for p in includes]+['-I'+directory]
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
    subprocess.run(flags+[str(HERE/'host.c'),'-o',str(temp/'test')],check=True,cwd=ROOT,env=env)
    subprocess.run([str(temp/'test')],check=True,cwd=ROOT,env=env,timeout=30)
    # Compile the complete class TU too, so omitted/extracted paths cannot hide a heap reference.
    obj=temp/'class.o'
    subprocess.run(flags+['-c',str(ROOT/CLASS),'-o',str(obj)],check=True,cwd=ROOT,env=env)
    symbols=subprocess.check_output(['nm','-u',str(obj)],text=True)
    forbidden={'malloc','free','calloc','realloc','_sbrk','_malloc_r','_free_r','__malloc_lock'}
    assert not forbidden.intersection(line.split()[-1] for line in symbols.splitlines()), symbols
    print('PASS complete production USB class object: no heap/allocator-lock references')
