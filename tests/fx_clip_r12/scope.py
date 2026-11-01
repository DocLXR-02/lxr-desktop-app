"""Fail if the bounded CLP experiment changes unrelated production algorithms."""
from pathlib import Path
import subprocess,hashlib,json
ROOT=Path(__file__).resolve().parents[2];BASE='64263ed25f2e9c7f5d880b493d943bfba29271a8'
allowed={'Core/Inc/Soundengine/FxBus.h','Core/Inc/Soundengine/SoloMix.h','Core/Src/Soundengine/FxBus.c','Core/Src/Soundengine/SoloMix.c','Core/Src/Soundengine/DSPAudio/mixer.c','Core/Src/main.c'}
changed=subprocess.check_output(['git','diff',BASE,'--name-only','--','Core','Drivers','Middlewares','FATFS','USB_DEVICE_MIDI','STM32F765VGTX_FLASH.ld'],cwd=ROOT,text=True).splitlines();assert set(changed)<=allowed,changed
paths=['Core/Src/Soundengine/DSPAudio/'+n+'.c' for n in ['DrumVoice','Snare','CymbalVoice','HiHat','Oscillator','transientGenerator','ResonantFilter','1PoleLp','distortion']]+['Core/Src/Soundengine/DSPAudio/FX/'+n+'.c' for n in ['Distortion','Ringmod','Compressor','Delay']]
for p in paths:assert (ROOT/p).read_bytes()==subprocess.check_output(['git','show',BASE+':'+p],cwd=ROOT),p
main=(ROOT/'Core/Src/main.c').read_bytes();assert main==subprocess.check_output(['git','show',BASE+':Core/Src/main.c'],cwd=ROOT).replace(b'LXR LAB R11',b'LXR LAB R12')
s=(ROOT/'Core/Src/Soundengine/SoloMix.c').read_text();assert 'paths[6]' in s and 'calcOnePole(&faderReplay[voice]' in s and 'FxBus_clipFaderGains(' not in s
out=ROOT/'build/r12-scope';out.mkdir(parents=True,exist_ok=True);result=dict(base=BASE,allowed_production_changes=changed,byte_identical_production=paths,diagnostics='unchanged/OFF',source_scope='CLP metadata/routing plus splash only');(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS scope: voice, oscillator/filter, CLP DSP, bass, detector and other FX production files unchanged; splash-only main')
