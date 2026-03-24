#!/usr/bin/env python3
import csv, json, os, subprocess, sys, time, pathlib
from pathlib import Path

def run_case(mode,n,seed,sl,sq,timeout_s,solver,outdir):
    outdir=Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    meta=outdir/'meta.json'
    parent=outdir/'hidden_parent.txt'
    inp=outdir/'in.txt'
    out=outdir/'out.txt'
    tfile=outdir/'time.txt'
    gen=['python3','gen_case.py','--mode',mode,'--n',str(n),'--seed',str(seed),'--meta',str(meta),'--parent-out',str(parent)]
    if str(sl)=='1': gen.append('--shuffle-labels')
    if str(sq)=='1': gen.append('--shuffle-queries')
    with open(inp,'w') as f:
        subprocess.run(gen, check=True, stdout=f)
    t0=time.time()
    with open(out,'w') as fo, open(tfile,'w') as ft:
        proc=subprocess.run(['/usr/bin/time','-f','%e %M','-o',str(tfile),'timeout',str(timeout_s),solver], stdin=open(inp,'r'), stdout=fo, stderr=subprocess.DEVNULL)
    elapsed=time.time()-t0
    rc=proc.returncode
    validator=''
    valid=None
    if rc==0:
        vproc=subprocess.run(['python3','validator.py',str(inp),str(out)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        validator=vproc.stdout.strip().replace('\n',' | ')
        valid='OK' if vproc.returncode==0 else 'FAIL'
    else:
        validator=''
        valid='TIMEOUT' if rc==124 else f'RC{rc}'
    elapsed_s=''
    mem_kb=''
    if tfile.exists():
        try:
            txt=tfile.read_text().strip().split()
            if len(txt)>=2:
                elapsed_s, mem_kb = txt[0], txt[1]
        except Exception:
            pass
    return {'mode':mode,'n':n,'seed':seed,'shuffle_labels':sl,'shuffle_queries':sq,'timeout_s':timeout_s,'rc':rc,'validator':valid,'validator_msg':validator,'elapsed_s':elapsed_s,'mem_kb':mem_kb,'outdir':str(outdir)}

if __name__=='__main__':
    tsv_in, solver, base_out, tsv_out = sys.argv[1:5]
    rows=[]
    with open(tsv_in) as f:
        r=csv.DictReader(f, delimiter='\t')
        for row in r:
            od=Path(base_out)/f"{row['stage']}_{row['mode']}_{row['n']}_s{row['seed']}"
            res=run_case(row['mode'], int(row['n']), int(row['seed']), int(row['shuffle_labels']), int(row['shuffle_queries']), float(row['timeout_s']), solver, od)
            res['stage']=row['stage']
            rows.append(res)
            print(f"[{len(rows)}] {row['stage']} {row['mode']} n={row['n']} s={row['seed']} rc={res['rc']} valid={res['validator']} t={res['elapsed_s']}")
            sys.stdout.flush()
    with open(tsv_out,'w',newline='') as f:
        w=csv.DictWriter(f, fieldnames=['stage','mode','n','seed','shuffle_labels','shuffle_queries','timeout_s','rc','validator','validator_msg','elapsed_s','mem_kb','outdir'], delimiter='\t')
        w.writeheader(); w.writerows(rows)
