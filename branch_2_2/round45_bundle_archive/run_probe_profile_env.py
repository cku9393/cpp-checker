#!/usr/bin/env python3
import os, pathlib, subprocess, sys
solver, indir, outdir = sys.argv[1:4]
indir = pathlib.Path(indir)
outdir = pathlib.Path(outdir)
outdir.mkdir(parents=True, exist_ok=True)
parts = indir.name.split('_')
seed = parts[-1][1:] if parts[-1].startswith('s') else ''
n = parts[-2]
mode = '_'.join(parts[:-2])
env = os.environ.copy()
env['DENSE_SHADOW_CASE_MODE'] = mode
env['DENSE_SHADOW_CASE_N'] = n
env['DENSE_SHADOW_CASE_SEED'] = seed
env['DENSE_PROFILE_OUTDIR'] = str(outdir)
env['DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK'] = '1'
with open(indir/'in.txt','rb') as fin, open(outdir/'out.txt','wb') as fout:
    proc = subprocess.run(['timeout','45',solver], stdin=fin, stdout=fout, stderr=subprocess.DEVNULL, env=env)
(outdir/'rc.txt').write_text(str(proc.returncode))
if proc.returncode == 0:
    res = subprocess.run(['python3','validator.py',str(indir/'in.txt'),str(outdir/'out.txt')], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (outdir/'validator.txt').write_text(res.stdout)
