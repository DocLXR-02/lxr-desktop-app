#!/usr/bin/env python3
"""Focused R22 cases run on the existing R21 production-path/FatFs fixture."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2]
runpy.run_path(str(R/'tests/r21_voice_copy/run.py'),run_name='__main__',
               init_globals={'EXTRA_TEST_SOURCE':str(Path(__file__).parent/'cases.c')})
