from pathlib import Path
import os,runpy
R=Path(os.environ.get('LXR_SOURCE_ROOT','/workspace/lxr-desktop-app'))
runpy.run_path(str(R/'tests/r21_voice_copy/run.py'),run_name='__main__',
 init_globals={'EXTRA_TEST_SOURCE':str(Path(__file__).with_suffix('.c'))})
