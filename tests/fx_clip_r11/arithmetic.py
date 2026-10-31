from pathlib import Path
import argparse,subprocess,os
from common import ROOT,compile_run
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'build/r11-arithmetic');a=p.parse_args()
exe=compile_run(a.output,Path(__file__).with_suffix('.c'))
subprocess.run([exe],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
