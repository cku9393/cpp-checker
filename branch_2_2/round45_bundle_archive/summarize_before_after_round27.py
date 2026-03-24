#!/usr/bin/env python3
import csv, glob, json, os, statistics, sys
from pathlib import Path

def time_from_dir(d):
    try:
        txt=Path(d,'time.txt').read_text().strip().split()
        return float(txt[0]), int(txt[1])
    except Exception:
        return None, None

def rc_from_dir(d):
    try: return int(Path(d,'rc.txt').read_text().strip())
    except: return None

def valid_from_dir(d):
    if Path(d,'validator.txt').exists(): return 'OK'
    rc=rc_from_dir(d)
    if rc==124: return 'TIMEOUT'
    if rc is None:
        try:
            out=Path(d,'out.txt')
            if out.exists():
                return 'UNKNOWN'
        except: pass
    return 'UNKNOWN'

def collect(prefix):
    rows=[]
    for path in sorted(Path(prefix).glob('*')):
        name=path.name
        # mode_n_sseed
        parts=name.split('_s')
        if len(parts)!=2: continue
        left,seed=parts
        toks=left.split('_')
        n=int(toks[-1]); mode='_'.join(toks[:-1])
        t,mem=time_from_dir(path)
        rows.append({'mode':mode,'n':n,'seed':int(seed),'elapsed_s':t,'mem_kb':mem,'validator':valid_from_dir(path)})
    return rows
before=collect(sys.argv[1]); after=collect(sys.argv[2])
# stage TSVs maybe additional
with open(sys.argv[3],'w',newline='') as f:
    fieldnames=['mode','n','seed','before_elapsed_s','after_elapsed_s','delta_s','before_validator','after_validator']
    w=csv.DictWriter(f, fieldnames=fieldnames, delimiter='\t'); w.writeheader()
    key_to_after={(r['mode'],r['n'],r['seed']):r for r in after}
    for b in before:
        a=key_to_after.get((b['mode'],b['n'],b['seed']),{})
        be=b['elapsed_s']; ae=a.get('elapsed_s')
        delta=(ae-be) if (be is not None and ae is not None) else None
        w.writerow({'mode':b['mode'],'n':b['n'],'seed':b['seed'],'before_elapsed_s':be,'after_elapsed_s':ae,'delta_s':delta,'before_validator':b['validator'],'after_validator':a.get('validator')})
