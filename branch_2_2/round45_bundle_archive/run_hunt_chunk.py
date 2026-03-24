#!/usr/bin/env python3
from __future__ import annotations

import argparse, csv, os, signal, subprocess, shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent
GEN = ROOT / 'gen_case.py'
VAL = ROOT / 'validator.py'

FIELDS = ['mode','n','seed','shuffle_labels','shuffle_queries','solver_rc','timed_out','val_ok','sec','rss_kb','case_dir']

def run_cmd(cmd, *, stdin_path=None, stdout_path=None, stderr_path=None, timeout=None):
    stdin_f = open(stdin_path, 'rb') if stdin_path else None
    stdout_f = open(stdout_path, 'wb') if stdout_path else subprocess.DEVNULL
    stderr_f = open(stderr_path, 'wb') if stderr_path else subprocess.DEVNULL
    try:
        p = subprocess.Popen(cmd, cwd=ROOT, stdin=stdin_f, stdout=stdout_f, stderr=stderr_f, preexec_fn=os.setsid)
        timed_out = False
        try:
            p.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            try:
                os.killpg(p.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            p.wait()
        rc = int(p.returncode)
    finally:
        if stdin_f: stdin_f.close()
        if stdout_f not in (None, subprocess.DEVNULL): stdout_f.close()
        if stderr_f not in (None, subprocess.DEVNULL): stderr_f.close()
    return rc, timed_out


def run_solver_with_time(solver, in_path, out_path, time_path, stderr_path, timeout):
    cmd = ['/usr/bin/time', '-f', '%e %M', '-o', str(time_path), str(solver)]
    rc, timed_out = run_cmd(cmd, stdin_path=in_path, stdout_path=out_path, stderr_path=stderr_path, timeout=timeout)
    if timed_out:
        return rc, True, None, None
    if not time_path.exists():
        return rc, False, None, None
    txt = time_path.read_text().strip().split()
    try:
        sec = float(txt[0])
        rss = int(txt[1])
    except Exception:
        sec = None
        rss = None
    return rc, False, sec, rss


def append_rows(csv_path: Path, rows):
    exists = csv_path.exists()
    with open(csv_path, 'a', newline='') as f:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        if not exists:
            w.writeheader()
        for row in rows:
            w.writerow(row)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--solver', default='./solve')
    ap.add_argument('--out', required=True)
    ap.add_argument('--cases', required=True, help='comma-separated mode:n:seed entries')
    ap.add_argument('--label-flag', type=int, default=1)
    ap.add_argument('--query-flag', type=int, default=1)
    ap.add_argument('--timeout', type=float, default=20.0)
    args = ap.parse_args()

    solver = Path(args.solver).resolve()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    csv_path = out / 'hunt.csv'
    for spec in [x for x in args.cases.split(',') if x.strip()]:
        mode, n_s, seed_s = spec.split(':')
        n = int(n_s)
        seed = int(seed_s)
        case_dir = out / 'runs' / mode / f'n{n}' / f'seed{seed}_L{args.label_flag}_Q{args.query_flag}'
        if case_dir.exists():
            shutil.rmtree(case_dir)
        case_dir.mkdir(parents=True, exist_ok=True)
        in_path = case_dir / 'in.txt'
        out_path = case_dir / 'out.txt'
        meta_path = case_dir / 'meta.json'
        hid_path = case_dir / 'hidden_parent.txt'
        time_path = case_dir / 'time.txt'
        gen_stderr = case_dir / 'gen_stderr.txt'
        sol_stderr = case_dir / 'solver_stderr.txt'
        val_stderr = case_dir / 'val_stderr.txt'

        gen_cmd = ['python3', str(GEN), '--mode', mode, '--n', str(n), '--seed', str(seed), '--meta', str(meta_path), '--parent-out', str(hid_path)]
        if args.label_flag:
            gen_cmd.append('--shuffle-labels')
        if args.query_flag:
            gen_cmd.append('--shuffle-queries')
        rcg, tog = run_cmd(gen_cmd, stdout_path=in_path, stderr_path=gen_stderr, timeout=None)
        if rcg != 0 or tog:
            row = {
                'mode': mode, 'n': n, 'seed': seed, 'shuffle_labels': args.label_flag, 'shuffle_queries': args.query_flag,
                'solver_rc': 127, 'timed_out': 0, 'val_ok': 0, 'sec': '', 'rss_kb': '', 'case_dir': str(case_dir)
            }
            append_rows(csv_path, [row])
            print(f'{spec} GEN_FAIL rc={rcg} to={int(tog)}', flush=True)
            continue

        rcs, tos, sec, rss = run_solver_with_time(solver, in_path, out_path, time_path, sol_stderr, args.timeout)
        val_ok = 0
        if rcs == 0 and not tos:
            rcv, tov = run_cmd(['python3', str(VAL), str(in_path), str(out_path), '--quiet'], stderr_path=val_stderr, timeout=30.0)
            val_ok = 1 if (rcv == 0 and not tov) else 0
        row = {
            'mode': mode, 'n': n, 'seed': seed, 'shuffle_labels': args.label_flag, 'shuffle_queries': args.query_flag,
            'solver_rc': rcs, 'timed_out': 1 if tos else 0, 'val_ok': val_ok,
            'sec': '' if sec is None else f'{sec:.6f}', 'rss_kb': '' if rss is None else str(rss), 'case_dir': str(case_dir)
        }
        append_rows(csv_path, [row])
        print(f'{spec} rc={rcs} to={1 if tos else 0} val={val_ok} sec={"-" if sec is None else f"{sec:.3f}"} rss={"-" if rss is None else rss}', flush=True)

if __name__ == '__main__':
    main()
