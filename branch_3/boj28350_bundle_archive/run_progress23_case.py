#!/usr/bin/env python3
import argparse, json, os, re, subprocess, sys, time
from pathlib import Path
ROOT = Path('/mnt/data/lca_tree_stress_v5')
GEN = ROOT / 'gen_case.py'
VAL = ROOT / 'validator.py'
TIME_BIN = '/usr/bin/time'
KV_RE = re.compile(r'([A-Za-z0-9_./:-]+)=([^\s]+)')

def parse_all_kv(stderr_text: str):
    kv = {}
    for m in KV_RE.finditer(stderr_text):
        kv[m.group(1)] = m.group(2)
    return kv

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--solver', required=True)
    ap.add_argument('--run-tag', required=True)
    ap.add_argument('--mode', required=True)
    ap.add_argument('--n', type=int, required=True)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--profile-mode', default='PROFILE_BASE')
    ap.add_argument('--delta-mode', choices=['connector_only','both_on'], required=True)
    ap.add_argument('--outdir', required=True)
    ap.add_argument('--timeout-sec', type=float, default=0.0)
    ap.add_argument('--env', action='append', default=[])
    args=ap.parse_args()
    outdir=Path(args.outdir); outdir.mkdir(parents=True, exist_ok=True)
    in_path=outdir/'in.txt'; out_path=outdir/'out.txt'; err_path=outdir/'stderr.txt'; time_path=outdir/'time.txt'; meta_path=outdir/'meta.json'; parent_path=outdir/'hidden_parent.txt'; result_path=outdir/'result.json'
    with open(in_path,'w') as f:
        subprocess.run([sys.executable,str(GEN),'--mode',args.mode,'--n',str(args.n),'--seed',str(args.seed),'--meta',str(meta_path),'--parent-out',str(parent_path)], check=True, stdout=f)
    env=os.environ.copy(); env['PROFILE_MODE']=args.profile_mode; env['RUN_TAG']=args.run_tag; env.setdefault('PROFILE_PROGRESS_STRIDE','16')
    if args.delta_mode=='connector_only':
        env['ENABLE_DELTA_PRESERVED_HIT']='0'; env['ENABLE_DELTA_CONNECTOR_HIT']='1'
    else:
        env['ENABLE_DELTA_PRESERVED_HIT']='1'; env['ENABLE_DELTA_CONNECTOR_HIT']='1'
    for item in args.env:
        k,v=item.split('=',1); env[k]=v
    cmd=[TIME_BIN,'-f','%e %M','-o',str(time_path),args.solver]
    start=time.time(); timed_out=False
    with open(in_path,'rb') as fin, open(out_path,'wb') as fout, open(err_path,'wb') as ferr:
        proc=subprocess.Popen(cmd, stdin=fin, stdout=fout, stderr=ferr, env=env)
        try:
            rc=proc.wait(timeout=args.timeout_sec if args.timeout_sec>0 else None)
        except subprocess.TimeoutExpired:
            timed_out=True; proc.kill(); rc=proc.wait()
    elapsed=round(time.time()-start,3)
    val_ok=False; val_msg=''
    if out_path.exists() and out_path.stat().st_size>0:
        val=subprocess.run([sys.executable,str(VAL),str(in_path),str(out_path)], capture_output=True, text=True)
        val_ok=(val.returncode==0); val_msg=val.stdout+val.stderr
    tsec=None; rss=None
    if time_path.exists() and time_path.stat().st_size>0:
        parts=time_path.read_text().strip().split()
        if len(parts)>=2:
            try: tsec=float(parts[0]); rss=int(parts[1])
            except: pass
    stderr_text=err_path.read_text(errors='ignore') if err_path.exists() else ''
    res={
        'run_tag':args.run_tag,'mode':args.mode,'n':args.n,'seed':args.seed,
        'profile_mode':args.profile_mode,'delta_mode':args.delta_mode,
        'rc':rc,'timed_out':timed_out,'validator_ok':val_ok,'validator_msg':val_msg,
        'stdout_empty': (not out_path.exists()) or out_path.stat().st_size==0,
        'stderr_empty': (not err_path.exists()) or err_path.stat().st_size==0,
        'elapsed_sec': tsec if tsec is not None else elapsed,'maxrss_kb':rss,
        'summary_kv': parse_all_kv(stderr_text),
    }
    result_path.write_text(json.dumps(res, indent=2, ensure_ascii=False))
    print(json.dumps({'run_tag':args.run_tag,'rc':rc,'timed_out':timed_out,'validator_ok':val_ok,'elapsed_sec':res['elapsed_sec']}, ensure_ascii=False))
if __name__=='__main__':
    main()
