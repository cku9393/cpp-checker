#!/usr/bin/env python3
import argparse, json, os, re, shutil, subprocess, sys, time
from pathlib import Path

ROOT = Path('/mnt/data/lca_tree_stress_v5')
GEN = ROOT / 'gen_case.py'
VAL = ROOT / 'validator.py'
TIME_BIN = '/usr/bin/time'

SUMMARY_KEYS = {
    'phase','run_tag','delta_mode','profile_mode','sampled','sample_stride','sample_warmup','progress_stride','deletion','x','touched',
    'detailed_sampled','elapsed_ms'
}

KV_RE = re.compile(r'([A-Za-z0-9_./:-]+)=([^\s]+)')

def parse_summary(stderr_text: str):
    # take last line containing phase=summary if present; else merge all key=val pairs
    summary_line = None
    for line in stderr_text.splitlines():
        if 'phase=summary' in line:
            summary_line = line
    target = summary_line or stderr_text
    kv = {}
    for m in KV_RE.finditer(target):
        kv[m.group(1)] = m.group(2)
    return kv


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--solver', required=True)
    ap.add_argument('--run-tag', required=True)
    ap.add_argument('--mode', required=True)
    ap.add_argument('--n', type=int, required=True)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--shuffle-labels', action='store_true')
    ap.add_argument('--shuffle-queries', action='store_true')
    ap.add_argument('--profile-mode', default='PROFILE_BASE')
    ap.add_argument('--delta-mode', choices=['connector_only','both_on'], required=True)
    ap.add_argument('--outdir', required=True)
    ap.add_argument('--timeout-sec', type=float, default=0.0)
    ap.add_argument('--release-diag', action='store_true')
    ap.add_argument('--env', action='append', default=[])
    args = ap.parse_args()

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    in_path = outdir / 'in.txt'
    out_path = outdir / 'out.txt'
    err_path = outdir / 'stderr.txt'
    time_path = outdir / 'time.txt'
    meta_path = outdir / 'meta.json'
    hidden_parent = outdir / 'hidden_parent.txt'
    result_path = outdir / 'result.json'

    gen_cmd = [sys.executable, str(GEN), '--mode', args.mode, '--n', str(args.n), '--seed', str(args.seed), '--meta', str(meta_path), '--parent-out', str(hidden_parent)]
    if args.shuffle_labels:
        gen_cmd.append('--shuffle-labels')
    if args.shuffle_queries:
        gen_cmd.append('--shuffle-queries')
    with open(in_path, 'w') as f:
        subprocess.run(gen_cmd, check=True, stdout=f)

    env = os.environ.copy()
    env['PROFILE_MODE'] = args.profile_mode
    env['RUN_TAG'] = args.run_tag
    env['PROFILE_PROGRESS_STRIDE'] = env.get('PROFILE_PROGRESS_STRIDE', '16')
    if args.delta_mode == 'connector_only':
        env['ENABLE_DELTA_PRESERVED_HIT'] = '0'
        env['ENABLE_DELTA_CONNECTOR_HIT'] = '1'
    elif args.delta_mode == 'both_on':
        env['ENABLE_DELTA_PRESERVED_HIT'] = '1'
        env['ENABLE_DELTA_CONNECTOR_HIT'] = '1'
    if args.release_diag:
        env['ENABLE_COMPACT_RELEASE_DIAG'] = '1'
    for item in args.env:
        k, v = item.split('=', 1)
        env[k] = v

    solver_cmd = [TIME_BIN, '-f', '%e %M', '-o', str(time_path), args.solver]
    rc = None
    timed_out = False
    start = time.time()
    with open(in_path, 'rb') as fin, open(out_path, 'wb') as fout, open(err_path, 'wb') as ferr:
        proc = subprocess.Popen(solver_cmd, stdin=fin, stdout=fout, stderr=ferr, env=env)
        try:
            rc = proc.wait(timeout=args.timeout_sec if args.timeout_sec > 0 else None)
        except subprocess.TimeoutExpired:
            timed_out = True
            proc.kill()
            try:
                rc = proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                rc = -9
    elapsed = round(time.time() - start, 3)

    validator_ok = False
    validator_msg = ''
    if not timed_out and rc == 0 and out_path.exists() and out_path.stat().st_size > 0:
        val = subprocess.run([sys.executable, str(VAL), str(in_path), str(out_path)], capture_output=True, text=True)
        validator_ok = (val.returncode == 0)
        validator_msg = val.stdout + val.stderr
    elif out_path.exists() and out_path.stat().st_size > 0:
        val = subprocess.run([sys.executable, str(VAL), str(in_path), str(out_path)], capture_output=True, text=True)
        validator_ok = (val.returncode == 0)
        validator_msg = val.stdout + val.stderr

    elapsed_sec = None
    maxrss_kb = None
    if time_path.exists():
        parts = time_path.read_text().strip().split()
        if len(parts) >= 2:
            try:
                elapsed_sec = float(parts[0])
                maxrss_kb = int(parts[1])
            except Exception:
                pass

    stderr_text = err_path.read_text(errors='ignore') if err_path.exists() else ''
    summary_kv = parse_summary(stderr_text)

    result = {
        'mode': args.mode,
        'n': args.n,
        'seed': args.seed,
        'shuffle_labels': args.shuffle_labels,
        'shuffle_queries': args.shuffle_queries,
        'solver': args.solver,
        'profile_mode': args.profile_mode,
        'delta_mode': args.delta_mode,
        'run_tag': args.run_tag,
        'release_diag': args.release_diag,
        'rc': rc,
        'timed_out': timed_out,
        'validator_ok': validator_ok,
        'validator_msg': validator_msg,
        'stdout_empty': (not out_path.exists()) or out_path.stat().st_size == 0,
        'stderr_empty': (not err_path.exists()) or err_path.stat().st_size == 0,
        'elapsed_sec': elapsed_sec if elapsed_sec is not None else elapsed,
        'maxrss_kb': maxrss_kb,
        'summary_kv': summary_kv,
    }
    result_path.write_text(json.dumps(result, indent=2, ensure_ascii=False))
    print(json.dumps({'run_tag': args.run_tag, 'rc': rc, 'timed_out': timed_out, 'validator_ok': validator_ok, 'elapsed_sec': result['elapsed_sec']}, ensure_ascii=False))

if __name__ == '__main__':
    main()
