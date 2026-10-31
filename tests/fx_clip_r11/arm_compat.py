"""Compare actual native corrected readers with recorded delivered-R10 ELF probes.
Oracle provenance: d0215ef5 triage, ELF 3cc3b788... (not device execution).
Seam differences are declared; arithmetic.py separately checks their periodic result.
"""
from pathlib import Path
import argparse,json,os,subprocess
from common import ROOT,compile_run
H=Path(__file__).parent;p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'build/r11-arm-compat');a=p.parse_args();exe=compile_run(a.output,H/'arm_compat.c')
lines=subprocess.check_output([exe],text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')).splitlines();fm=json.loads((H/'oracles/r10-fm.json').read_text());pwm=json.loads((H/'oracles/r10-pwm.json').read_text());exceptions=[]
for line,old in zip(lines[:35],fm,strict=True):
 _,w,m,y=line.split();assert int(m)==old['mod'] and int(y)==old['elf'],(line,old)
for line,old in zip(lines[35:],pwm,strict=True):
 _,r,phase,p,q,y=line.split();r=int(r);phase=int(phase);q=int(q);y=int(y)
 assert (r,phase,float(p),q)==(old['row'],int(old['phase'],16),old['pwm'],int(old['elf_phase2'],16))
 if y!=old['elf_sample']:
  assert r==10 and (phase>>22==1023 or q>>22==1023),(line,old)
  exceptions.append(dict(row=r,phase=hex(phase),pwm=float(p),R10=old['elf_sample'],corrected=y))
result=dict(fm_matches=35,pwm_phase_matches=42,pwm_sample_matches=42-len(exceptions),declared_periodic_seam_exceptions=exceptions,reference_elf_sha256='3cc3b78888aaacf4341f9775efcb7c675cb9e11a11954e3501b289f869418f0b')
(a.output/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
