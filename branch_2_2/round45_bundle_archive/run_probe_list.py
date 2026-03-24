#!/usr/bin/env python3
import csv, os, sys, subprocess, time
from pathlib import Path

def run_one(mode,n,seed,solver,base_out):
    indir=Path('probe_inputs')/f'{mode}_{n}_s{seed}'
    outdir=Path(base_out)/f'{mode}_{n}_s{seed}'
    outdir.mkdir(parents=True, exist_ok=True)
    tfile=outdir/'time.txt'; out=outdir/'out.txt'
    with open(out,'w') as fo:
        proc=subprocess.run(['/usr/bin/time','-f','%e %M','-o',str(tfile),'timeout','12',solver], stdin=open(indir/'in.txt','r'), stdout=fo, stderr=subprocess.DEVNULL)
    rc=proc.returncode
    (outdir/'rc.txt').write_text(str(rc))
    if rc==0:
        subprocess.run(['python3','validator.py',str(indir/'in.txt'),str(out)], stdout=open(outdir/'validator.txt','w'), stderr=subprocess.STDOUT)
    return rc

if __name__=='__main__':
    solver, outdir, tsvout = sys.argv[1:4]
    cases=[]
    for name in sorted(os.listdir('probe_inputs')):
        if '_s' not in name: continue
        left,seed=name.rsplit('_s',1)
        toks=left.split('_')
        n=int(toks[-1]); mode='_'.join(toks[:-1]); seed=int(seed)
        cases.append((mode,n,seed))
    rows=[]
    for idx,(mode,n,seed) in enumerate(cases,1):
        rc=run_one(mode,n,seed,solver,outdir)
        time_txt=(Path(outdir)/f'{mode}_{n}_s{seed}'/'time.txt').read_text().strip().split()
        elapsed=time_txt[0] if time_txt else ''
        valid='OK' if (Path(outdir)/f'{mode}_{n}_s{seed}'/'validator.txt').exists() else ('TIMEOUT' if rc==124 else f'RC{rc}')
        rows.append({'mode':mode,'n':n,'seed':seed,'rc':rc,'elapsed_s':elapsed,'validator':valid,'outdir':str(Path(outdir)/f'{mode}_{n}_s{seed}')})
        print(f'[{idx}/{len(cases)}] {mode} n={n} s={seed} rc={rc} valid={valid} t={elapsed}')
        sys.stdout.flush()
    with open(tsvout,'w',newline='') as f:
        w=csv.DictWriter(f, fieldnames=['mode','n','seed','rc','elapsed_s','validator','outdir'], delimiter='\t'); w.writeheader(); w.writerows(rows)
