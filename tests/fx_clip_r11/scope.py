"""Protect the explicitly bounded production diff and audio-owned scheduling."""
from pathlib import Path
import subprocess
from common import ROOT,body
BASE='8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7'
def old(p):return subprocess.check_output(['git','show',BASE+':'+p],cwd=ROOT)
allowed={'Core/CMakeLists.txt','Core/Inc/Soundengine/SoloMix.h','Core/Src/Soundengine/SoloMix.c','Core/Src/main.c'}|{'Core/Src/Soundengine/DSPAudio/'+n+'.c' for n in ['Oscillator','transientGenerator','DrumVoice','Snare','CymbalVoice','HiHat','mixer']}
changed=set(subprocess.check_output(['git','diff',BASE,'--name-only','--','Core','Drivers','cmake','CMakeLists.txt','CMakePresets.json'],cwd=ROOT,text=True).splitlines());assert changed<=allowed,changed-allowed
for p in ['Core/Src/Soundengine/FxBus.c','Core/Src/Soundengine/DSPAudio/FX/Distortion.c','Core/Src/Soundengine/DSPAudio/ResonantFilter.c','Core/Src/Soundengine/DSPAudio/1PoleLp.c','Core/Src/Soundengine/MIDI/MidiParser.c','Core/Src/Soundengine/DSPAudio/modulationNode.c','Core/Src/Soundengine/MIDI/ParameterArray.c','Core/Inc/Soundengine/config.h']:
 assert (ROOT/p).read_bytes()==old(p),p
p='Core/Src/main.c';assert (ROOT/p).read_bytes()==old(p).replace(b'LXR LAB R10',b'LXR LAB R11')
s=(ROOT/'Core/Src/Soundengine/DSPAudio/mixer.c').read_text();block=body(s,'mixer_calcNextSampleBlock')
assert block.index('modNode_resetTargets')<block.index('lfo_dispatchNextValue')<block.index('Snare_calcAsync')<block.index('mixer_soloMixForBlock()')<block.index('calcDrumVoiceSyncBlock(0')
for v in range(6):assert block.index(f'mixer_decimateBlock({v},sampleData)')<block.index(f'SoloMix_gate({v}, sampleData')<block.index(f'mixer_addDataToOutput(mixer_audioRouting[{v}]')
p='Core/Src/Soundengine/DSPAudio/mixer.c'
assert body(s,'mixer_clipFaderForBlock')==body(old(p).decode().replace('\r\n','\n'),'mixer_clipFaderForBlock')
assert body(s,'mixer_decimateBlock')==body(old(p).decode().replace('\r\n','\n'),'mixer_decimateBlock')
assert body(s,'mixer_addDataToOutput')==body(old(p).decode().replace('\r\n','\n'),'mixer_addDataToOutput')
print('PASS production scope, unchanged CLP/filter/fader/storage/control mappings, capture/update order and post-decimator gate placement')
