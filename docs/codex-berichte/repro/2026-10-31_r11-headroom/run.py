"""Reproduce the R11 prerequisite blocker, without modifying/building firmware.

An expected production conversion failure is evidence of a blocker, not a passing
R11 candidate. Only a native snare component executable is compiled.
"""
from pathlib import Path
import argparse, hashlib, io, json, os, re, subprocess, tarfile, tempfile

PIN = '8082cc5e79496941ca126bc90dc3ac4e2d9a1ea7'
HERE = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--repo', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
repo, output = args.repo.resolve(), args.output.resolve()
assert not output.is_relative_to(repo), 'Evidence output must be outside source checkout'
output.mkdir(parents=True, exist_ok=True)

def git(*items):
    return subprocess.check_output(['git', '-C', str(repo), *items])

def body(source, name):
    pattern = r'^ *(?:IN_ITCM_RAM )?(?:static __inline )?(?:void|int16_t|uint32_t|uint8_t|int|float) '
    match = re.search(pattern + name + r'\s*\([^;]*?\)[^\n{]*\s*\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

status = git('status', '--porcelain=v1')
env = os.environ.copy()
env.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
with tempfile.TemporaryDirectory(prefix='r11-headroom-') as temp:
    work = Path(temp)
    source = work / 'source'
    source.mkdir()
    with tarfile.open(fileobj=io.BytesIO(git('archive', '--format=tar', PIN))) as archive:
        archive.extractall(source, filter='data')
    inc = work / 'include'
    inc.mkdir()
    (inc / 'main.h').write_text('#include "Soundengine/globals.h"\n')
    (inc / 'stm32f7xx_hal.h').write_text('''#ifndef R11_HOST_HAL_H
#define R11_HOST_HAL_H
#include <stdint.h>
#include <assert.h>
typedef struct { int unused; } RNG_HandleTypeDef;
static inline int32_t r11_unused_sub(int32_t a,int32_t b){(void)a;(void)b;assert(0);return 0;}
#define __QSUB16(a,b) r11_unused_sub(a,b)
#endif
''')
    dsp = source / 'Core/Src/Soundengine/DSPAudio'
    oscillator = (dsp / 'Oscillator.c').read_text()
    names = ['freq2PhaseIncr', 'freq2PhaseIncr1024', 'freq2PhaseIncr32767',
             'fast_log2', 'freqToMidiNote', 'freqToTableIndex', 'calcSineBlock',
             'calcNoiseBlock', 'calcNextOscSampleBlock', 'osc_calcSineFreq',
             'osc_calcNoiseFreq', 'osc_calcWavetableFreq', 'osc_calcUserSampleFreq',
             'osc_setFreq', 'osc_setBaseNote']
    parser_source = (source / 'Core/Src/Soundengine/MIDI/MidiParser.c').read_text()
    constants = []
    for name, text in [('SEMITONE_UP', parser_source),
                       ('SEQ_DEFAULT_NOTE', (source / 'Core/Inc/Soundengine/Sequencer/sequencer.h').read_text())]:
        constants.append(re.search(r'^#define ' + name + r'\s+[^\n]+', text, re.M).group())
    (work / 'osc.inc').write_text('\n'.join(constants) + '\n' +
        body(parser_source, 'midiParser_calcDetune') + '\n' +
        '\n'.join(body(oscillator, name) for name in names))
    config = (source / 'Core/Inc/Soundengine/config.h').read_text()
    definitions = dict(NUM_DRUM_VOICES=3, OUTPUT_DMA_SIZE=64, USE_BIG_SAMPLE_BUFFER=1,
                       INTERPOLATE_OSC=1, SMOOTH_MIXER_VOL_PAN_DRIVE=1)
    for name, value in definitions.items():
        assert re.search(r'^#define ' + name + r'\s+' + str(value) + r'\b', config, re.M)
    flags = [os.environ.get('CC', 'cc'), '-std=c11', '-D_DEFAULT_SOURCE', '-O2',
             '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
             '-fsanitize=address,undefined,bounds,float-cast-overflow',
             '-fno-sanitize-recover=all', '-Werror=implicit-function-declaration']
    flags += ['-D' + name + '=' + str(value) for name, value in definitions.items()]
    flags += ['-I' + str(path) for path in [inc, work, source / 'tests/fx_clip/include', source / 'Core/Inc']]
    files = [dsp / (name + '.c') for name in ['Snare', 'transientGenerator',
             'transientTables', 'wavetable', 'SlopeEg2', 'Decay', 'snapEg',
             '1PoleLp', 'distortion', 'ResonantFilter']]
    command = flags + [str(HERE / 'headroom.c')] + list(map(str, files))
    command += ['-Wl,--gc-sections', '-lm', '-o', str(work / 'headroom')]
    (output / 'command.json').write_text(json.dumps(command, indent=2) + '\n')
    result = subprocess.run(command, text=True, capture_output=True, env=env)
    (output / 'compile.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
    cases = []
    for mix in [0, 1, 5, 10, 20, 127]:
        for mode in [0, 1]:
            for transient in [0, 127]:
                run = subprocess.run([str(work / 'headroom'), str(mix), str(mode), str(transient)],
                                     text=True, capture_output=True, env=env)
                expected_blocker = mix == 127 and transient == 127
                log = f'mix{mix}-velocity{mode}-transient{transient}.log'
                (output / log).write_text(run.stdout + run.stderr)
                if expected_blocker:
                    assert run.returncode != 0 and "34647 is outside the range" in run.stderr
                    assert f'Snare.c:{196 if mode else 221}:' in run.stderr
                else:
                    run.check_returncode()
                    assert 'PASS native conversion sanitizer' in run.stdout
                cases.append(dict(mix=mix, velocity_mode=mode, transient=transient,
                                  returncode=run.returncode, expected_blocker=expected_blocker,
                                  stdout=run.stdout, log=log))
    hashes = {str(path.relative_to(source)): hashlib.sha256(path.read_bytes()).hexdigest()
              for path in files + [dsp / 'Oscillator.c', source / 'Core/Src/Soundengine/MIDI/MidiParser.c']}
assert git('status', '--porcelain=v1') == status, 'Source checkout changed'
summary = dict(source_commit=PIN, status='BLOCKED: production snare conversion exceeds int16',
               compiler=subprocess.check_output([flags[0], '--version'], text=True).splitlines()[0],
               cases=cases, passed_controls=22, confirmed_failures=2, source_sha256=hashes,
               scope='Native production snare component, mocked control/hardware services; not R11 firmware or full-engine validation.')
(output / 'results.json').write_text(json.dumps(summary, indent=2) + '\n')
print('BLOCKER CONFIRMED: 22 controls pass; both full-MIX/full-transient velocity modes fail conversion. No R11 candidate.')
