#!/usr/bin/env python3
import argparse, json, os, re, subprocess, sys
from pathlib import Path
ROOT = Path('/mnt/data/lca_tree_stress_v5')
VAL = ROOT / 'validator.py'
KV_RE = re.compile(r'([A-Za-z0-9_./:-]+)=([^\s]+)')

def parse_all_kv(stderr_text: str):
    kv={}
    for m in KV_RE.finditer(stderr_text):
        kv[m.group(1)] = m.group(2)
    return kv

def atomic_write_json(path: Path, obj):
    tmp = path.with_suffix(path.suffix + '.tmp')
    tmp.write_text(json.dumps(obj, indent=2, ensure_ascii=False))
    tmp.replace(path)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--case-dir', required=True)
    ap.add_argument('--journal', default='/mnt/data/progress39_case_journal.jsonl')
    ap.add_argument('--elapsed-sec', type=float, default=None)
    args = ap.parse_args()
    d = Path(args.case_dir)
    status_path = d/'status.json'; hb_path = d/'heartbeat.json'; result_path = d/'result.json'; err_path = d/'stderr.txt'; out_path = d/'stdout.txt'; in_path = d/'in.txt'
    status = json.loads(status_path.read_text()) if status_path.exists() else {}
    hb = json.loads(hb_path.read_text()) if hb_path.exists() else {}
    if result_path.exists():
        print('result already exists')
        return
    stderr_text = err_path.read_text(errors='ignore') if err_path.exists() else ''
    summary = parse_all_kv(stderr_text)
    stdout_empty = (not out_path.exists()) or out_path.stat().st_size == 0
    stderr_empty = (not err_path.exists()) or err_path.stat().st_size == 0
    val_ok = False; val_msg=''
    if in_path.exists() and out_path.exists() and out_path.stat().st_size > 0:
        val = subprocess.run([sys.executable, str(VAL), str(in_path), str(out_path)], capture_output=True, text=True)
        val_ok = (val.returncode == 0)
        val_msg = (val.stdout or '') + (val.stderr or '')
    result = {
        'run_tag': status.get('case_name', d.name),
        'mode': status.get('mode'),
        'n': status.get('n'),
        'seed': status.get('seed'),
        'profile_mode': status.get('profile_mode'),
        'delta_mode': status.get('delta_mode'),
        'rc': status.get('rc', -999),
        'timed_out': bool(status.get('timed_out', False)),
        'interrupted': bool(status.get('interrupted', False)),
        'validator_ok': val_ok,
        'validator_msg': val_msg if val_msg else 'finalize-only row',
        'stdout_empty': stdout_empty,
        'stderr_empty': stderr_empty,
        'elapsed_sec': args.elapsed_sec if args.elapsed_sec is not None else status.get('elapsed_sec'),
        'maxrss_kb': status.get('maxrss_kb'),
        'summary_kv': summary,
        'heartbeat': hb,
        'synthetic_failure_row': True,
        'finalize_only_recovered': True,
    }
    atomic_write_json(result_path, result)
    status['state']='finished'; status['phase']='finalized';
    if args.elapsed_sec is not None: status['elapsed_sec']=args.elapsed_sec
    atomic_write_json(status_path, status)
    with open(args.journal,'a',encoding='utf-8') as f:
        f.write(json.dumps({'event':'finalize_only', **status, 'validator_ok': val_ok}, ensure_ascii=False)+'\n')
    print('finalized recovered row')
if __name__ == '__main__':
    main()
