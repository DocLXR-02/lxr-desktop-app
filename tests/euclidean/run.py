#!/usr/bin/env python3
"""Sanitized host regression of original source bodies. No firmware execution."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
BASE = '5829398cea5d337200393757e39a37d4d8148256'
GENERATOR = 'Core/Src/Soundengine/Sequencer/EuklidGenerator.c'
# SHA-256 of 2176 exact length,pulse,rotation,mask lines from lab-setup,
# with only host CLZ(0)=32 emulation. --record-baseline reproduces the oracle.
EXPECTED_MASKS = '212cd0c7e3da155464507d1bf22e90c0e50d38f32700cfbfefcde8f3307ac883'

def extract(path, name):
    source = (ROOT / path).read_text(errors='replace')
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                    lambda m: ' ' * len(m[0]), source, flags=re.S)
    match = re.search(r'^(?:IN_ITCM_RAM\s+)?(?:static\s+)?(?:inline\s+)?'
                      r'(?:void|float|FRESULT|uint\d+_t|int\d+_t)\s+'
                      + name + r'\([^;]*?\)\s*\{', masked, re.M)
    if not match:
        raise RuntimeError('Cannot extract ' + name)
    depth, end = 1, match.end()
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    line = source.count('\n', 0, match.start()) + 1
    return f'#line {line} "{path}"\n' + source[match.start():end] + '\n'

def run(command, **kwargs):
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                            timeout=120, **kwargs)
    if result.returncode:
        raise RuntimeError(f'{command[0]} exited {result.returncode}\n'
                           + result.stdout + result.stderr)
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record-baseline', action='store_true',
                        help='requires the pinned base git object; print oracle hash only')
    args = parser.parse_args()
    seq = 'Core/Src/Soundengine/Sequencer/sequencer.c'
    preset = 'Core/Src/Preset/presetManager.c'
    functions = [(seq, name) for name in (
        'seq_setTrackLength', 'seq_getTrackLength', 'seq_getLongestTrack',
        'seq_copyPatternToBackupBuffer', 'seq_copyBackupBufferToPattern',
        'getShiftValue', 'flamIdxToPattern', 'seq_intIsMainStepActive',
        'seq_isMainStepActive', 'seq_setMainStep', 'seq_getShiftFlamPattern',
        'seq_processFlam', 'seq_addNote')]
    functions += [(preset, name) for name in (
        'preset_sharedStatus', 'preset_openShared', 'preset_closeShared',
        'preset_writeChecked', 'preset_finishSave',
        'preset_readPatternLengths', 'preset_writePatternData',
        'preset_readPatternData', 'preset_savePatternSD', 'preset_loadPatternSD',
        'preset_saveTmpPat', 'preset_LoadTmpPat')]
    functions += [('Core/Src/Soundengine/MIDI/CortexParser.c', 'frontParser_handleSysexData')]
    includes = ['.', 'Core/Inc', 'Drivers/CMSIS/Include',
                'Drivers/CMSIS/Device/ST/STM32F7xx/Include',
                'Drivers/STM32F7xx_HAL_Driver/Inc',
                'Middlewares/Third_Party/FatFs/src', 'FATFS/App', 'FATFS/Target']
    with tempfile.TemporaryDirectory(prefix='lxr-euclidean-') as directory:
        temporary = Path(directory)
        (temporary / 'functions.inc').write_text(''.join(extract(*f) for f in functions))
        compiler = os.environ.get('CC', 'cc')
        flags = [compiler, '-std=gnu11', '-O1', '-g', '-DSTM32F765xx', '-DUSE_HAL_DRIVER',
                 '-Wno-int-to-pointer-cast', '-Wno-pointer-to-int-cast',
                 '-Werror=implicit-function-declaration', '-ffunction-sections',
                 '-fdata-sections', '-fno-pie', '-no-pie', '-fsanitize=address,undefined',
                 '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
        flags += ['-I' + str(ROOT / p) for p in includes] + ['-I' + directory]
        if args.record_baseline:
            original = run(['git', 'show', BASE + ':' + GENERATOR]).stdout
            (temporary / 'baseline-generator.inc').write_text(original)
            flags += ['-DBASELINE_GENERATOR']
        executable = temporary / 'host'
        build = run(flags + [str(HERE / 'host.c'), '-Wl,--gc-sections', '-o', str(executable)])
        if build.stderr:
            print(build.stderr, end='')
        env = os.environ.copy()
        env['ASAN_OPTIONS'] = 'detect_leaks=0'
        result = run([str(executable), 'generation'], env=env)
        digest = hashlib.sha256(result.stdout.encode()).hexdigest()
        assert len(result.stdout.splitlines()) == 2176
        if args.record_baseline:
            print('BASELINE_MASKS=' + digest)
            return
        assert digest == EXPECTED_MASKS, f'Rhythm masks changed: {digest}'
        print('PASS generation: 2176 exact masks match lab-setup; literal lengths; CLZ unshimmed')
        for scenario in ('bounds', 'playback', 'roundtrips', 'malformed', 'protocol'):
            result = run([str(executable), scenario], env=env)
            print(result.stdout, end='')
            assert not result.stderr, result.stderr
        print('PASS all six ASan/UBSan groups')

if __name__ == '__main__':
    main()
