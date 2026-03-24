#!/usr/bin/env python3
import argparse, json, os
from pathlib import Path

def atomic_write_json(path: Path, obj):
    tmp = path.with_suffix(path.suffix + '.tmp')
    tmp.write_text(json.dumps(obj, indent=2, ensure_ascii=False))
    tmp.replace(path)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--case-dir', required=True)
    ap.add_argument('--journal', default='/mnt/data/progress33_case_journal.jsonl')
    args = ap.parse_args()
    d = Path(args.case_dir)
    status_path = d/'status.json'; hb_path = d/'heartbeat.json'; result_path = d/'result.json'; err_path = d/'stderr.txt'; out_path = d/'stdout.txt'
    status = json.loads(status_path.read_text()) if status_path.exists() else {}
    hb = json.loads(hb_path.read_text()) if hb_path.exists() else {}
    if result_path.exists():
        print('result already exists')
        return
    stderr_text = err_path.read_text(errors='ignore') if err_path.exists() else ''
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
        'validator_ok': False,
        'validator_msg': 'finalize-only synthetic row',
        'stdout_empty': (not out_path.exists()) or out_path.stat().st_size == 0,
        'stderr_empty': (not err_path.exists()) or err_path.stat().st_size == 0,
        'elapsed_sec': status.get('elapsed_sec'),
        'summary_kv': {},
        'heartbeat': hb,
        'synthetic_failure_row': True,
    }
    atomic_write_json(result_path, result)
    status['state']='finished'; status['phase']='finalized_failure'
    atomic_write_json(status_path, status)
    with open(args.journal,'a',encoding='utf-8') as f:
        f.write(json.dumps({'event':'finalize_only', **status}, ensure_ascii=False)+'\n')
    print('finalized synthetic row')
if __name__ == '__main__':
    main()
