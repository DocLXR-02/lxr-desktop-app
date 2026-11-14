from pathlib import Path
import hashlib,importlib.util,json,os,subprocess,sys,time
root=Path(os.environ.get('LXR_SOURCE_ROOT','/workspace/lxr-desktop-app'));out=Path(__file__).resolve().parent
sys.dont_write_bytecode=True
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()=='861c643632e0a88bdc7d60d9410befb3971489fc', 'Use the exact pinned R21 base'
spec=importlib.util.spec_from_file_location('extract',root/'tests/bug001/run.py');f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f)
fx=(root/'Core/Src/Soundengine/FxBus.c').read_text()
(out/'fx.inc').write_text(f.function(fx,'FxBus_updateParameter')+'\n'+f.function(fx,'FxBus_setParameter'))
cortex=(root/'Core/Src/Soundengine/MIDI/CortexParser.c').read_text();a=cortex.index('\tcase FRONT_CC_LFO_TARGET:');b=cortex.index('    case FRONT_SET_P1_DEST:',a)
(out/'destination.inc').write_text('switch(cortex_midiMsg.status){'+cortex[a:b]+'default: assert(0);}')
stubs=(root/'tests/bug001/trap_stubs.c').read_text().replace('TRAP(FxBus_setParameter)\n','').replace('TRAP(FxBus_updateParameter)\n','')
(out/'stubs.c').write_text(stubs)
includes=['.','Core/Inc','Drivers/CMSIS/Include','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/STM32F7xx_HAL_Driver/Inc','Middlewares/Third_Party/FatFs/src','Middlewares/JS/STM32_USB_Device_Library/Core/Inc','Middlewares/JS/STM32_USB_Device_Library/Class/CDC/Inc','USB_DEVICE_MIDI/Target']
cmd=['cc','-std=gnu11','-O1','-g','-DSTM32F765xx','-DUSE_HAL_DRIVER','-D__fp16=_Float16','-Wno-pointer-to-int-cast','-Wno-int-to-pointer-cast','-Wno-implicit-function-declaration','-ffunction-sections','-fdata-sections','-fno-pie','-no-pie','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
cmd+=['-I'+str(root/p) for p in includes]+['-I'+str(out),str(out/'probe.c'),str(out/'stubs.c'),'-Wl,--gc-sections','-lm','-o',str(out/'probe')]
env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0';records=[]
with (out/'probe.log').open('w') as log:
 for args in [cmd,[str(out/'probe')]]:
  start=time.monotonic();r=subprocess.run(args,cwd=root,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
  log.write(r.stdout);print(r.stdout,end='');records.append({'argv':args,'exit_code':r.returncode,'host_elapsed_seconds':time.monotonic()-start})
  if r.returncode:break
(out/'results.json').write_text(json.dumps({'source_sha':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),'runs':records},indent=2)+'\n')
sys.exit(records[-1]['exit_code'])
