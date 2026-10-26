#!/usr/bin/env python3
"""Extract actual SD production functions; compile native register/HAL simulations."""
from pathlib import Path
import os,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
LL=ROOT/'Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_ll_sdmmc.c'
HAL=ROOT/'Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_sd.c'
BSP=ROOT/'FATFS/Target/bsp_driver_sd.c'
def extract(path,name):
    text=path.read_text();match=re.search(r'^(?:static |__weak )?(?:uint32_t|uint8_t|HAL_StatusTypeDef|HAL_SD_CardStateTypeDef) '+name+r'\([^;]*?\)\s*\{',text,re.M)
    assert match,name
    start=match.start();pos=text.index('{',match.start());depth=1;end=pos+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end].replace('__weak ','')
names=['SDMMC_WaitCommand','SDMMC_GetCmdResp1','SDMMC_GetCmdResp2','SDMMC_GetCmdResp3','SDMMC_GetCmdResp6','SDMMC_GetCmdResp7','SDMMC_GetCmdError','SDMMC_CmdStopTransfer']
functions='\n'.join(extract(LL,n) for n in names)
functions+='\n'+extract(HAL,'SD_PowerON')+'\n'+extract(HAL,'SD_SendStatus')+'\n'+extract(HAL,'HAL_SD_GetCardState')
functions+='\n'+extract(HAL,'HAL_SD_ReadBlocks')+'\n'+extract(HAL,'HAL_SD_WriteBlocks')
functions+='\n'+'\n'.join(extract(BSP,n) for n in ['BSP_SD_ReadBlocks','BSP_SD_WriteBlocks','BSP_SD_GetCardState'])
# Use target preprocessing to obtain the real constant values without ARM inline assembly.
# The mock header supplies only register/handle shapes and deliberately simulated functions.
compiler=shutil.which('arm-none-eabi-gcc') or '/workspace/.onboarding/toolchain/usr/bin/arm-none-eabi-gcc'
includes=['Core/Inc','Drivers/STM32F7xx_HAL_Driver/Inc','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/CMSIS/Include']
cmd=[compiler,'-E','-dM','-x','c','-DSTM32F765xx','-DUSE_HAL_DRIVER']+['-I'+str(ROOT/i) for i in includes]+['-include','stm32f7xx_hal.h','/dev/null']
dump=subprocess.check_output(cmd,text=True)
macros={m[1]:m[2] for m in re.finditer(r'^#define (\w+) (.+)$',dump,re.M)}
# Local BSP result definitions are also production values.
for m in re.finditer(r'^#define\s+(\w+)\s+(.+)$',(ROOT/'FATFS/Target/bsp_driver_sd.h').read_text(),re.M):
    macros[m[1]]=m[2]
wanted=set()
def add(name):
    if name in wanted or name not in macros:return
    wanted.add(name)
    for token in re.findall(r'\b[A-Za-z_]\w*\b',macros[name]):add(token)
for name in re.findall(r'\b[A-Za-z_]\w*\b',functions+'\n'+(HERE/'host.c').read_text()):
    if name.startswith(('SDMMC_','SD_SWITCH_','HAL_SD_ERROR_','HAL_SD_CARD_','SD_CONTEXT_','CARD_','MSD_','SD_TRANSFER_')) or name=='BLOCKSIZE':add(name)
constants='\n'.join('#define '+n+' '+macros[n] for n in sorted(wanted))
with tempfile.TemporaryDirectory(prefix='lxr-sd-hal-') as directory:
    directory=Path(directory)
    (directory/'constants.h').write_text(constants+'\n')
    (directory/'production.c').write_text(functions+'\n')
    env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=0'
    binary=directory/'host'
    flags=[os.environ.get('CC','cc'),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
           '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie',
           '-I'+str(directory),'-I'+str(HERE)]
    subprocess.run(flags+[str(HERE/'host.c'),'-o',str(binary)],check=True,env=env)
    subprocess.run([str(binary)],check=True,env=env,timeout=20)
