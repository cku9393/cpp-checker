#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List

from suite_utils import (
    default_solver_path,
    ensure_executable,
    markdown_table,
    parse_int_list_csv,
    parse_str_list_csv,
    resolve_solver_path,
    run_cmd,
    run_solver_with_time,
)

ROOT = Path(__file__).resolve().parent
GEN = ROOT / 'gen_case.py'
VAL = ROOT / 'validator.py'


@dataclass
class Row:
    mode: str
    n: int
    seed: int
    shuffle_labels: int
    shuffle_queries: int
    solver_rc: int
    timed_out: int
    val_ok: int
    sec: float | None
    rss_kb: int | None
    case_dir: str


def list_modes() -> List[str]:
    import subprocess
    res = subprocess.run([sys.executable, str(GEN), '--list-modes'], capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(res.stderr)
    return [ln.strip() for ln in res.stdout.splitlines() if ln.strip()]


def main() -> int:
    ap = argparse.ArgumentParser(description='Search across modes/seeds/shuffles and report the slowest validated cases.')
    ap.add_argument('--solver', default=str(default_solver_path(ROOT)))
    ap.add_argument('--out', default='hunt_out')
    ap.add_argument('--modes', default='')
    ap.add_argument('--sizes', default='12000,24000,48000,99999')
    ap.add_argument('--seeds', default='1,2,3')
    ap.add_argument('--label-flags', default='0,1')
    ap.add_argument('--query-flags', default='0,1')
    ap.add_argument('--top-k', type=int, default=20)
    ap.add_argument('--timeout', type=float, default=8.0)
    args = ap.parse_args()

    solver = resolve_solver_path(args.solver, root=ROOT)
    ensure_executable(solver)
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)

    modes = parse_str_list_csv(args.modes) if args.modes.strip() else list_modes()
    sizes = parse_int_list_csv(args.sizes)
    seeds = parse_int_list_csv(args.seeds)
    label_flags = parse_int_list_csv(args.label_flags)
    query_flags = parse_int_list_csv(args.query_flags)

    rows: List[Row] = []
    for mode in modes:
        for n in sizes:
            for seed in seeds:
                for sl in label_flags:
                    for sq in query_flags:
                        case_dir = out / 'runs' / mode / f'n{n}' / f'seed{seed}_L{sl}_Q{sq}'
                        case_dir.mkdir(parents=True, exist_ok=True)
                        in_path = case_dir / 'in.txt'
                        out_path = case_dir / 'out.txt'
                        meta_path = case_dir / 'meta.json'
                        hid_path = case_dir / 'hidden_parent.txt'
                        time_path = case_dir / 'time.txt'
                        gen_stderr = case_dir / 'gen_stderr.txt'
                        sol_stderr = case_dir / 'solver_stderr.txt'
                        val_stderr = case_dir / 'val_stderr.txt'

                        gen_cmd = [sys.executable, str(GEN), '--mode', mode, '--n', str(n), '--seed', str(seed),
                                   '--meta', str(meta_path), '--parent-out', str(hid_path)]
                        if sl:
                            gen_cmd.append('--shuffle-labels')
                        if sq:
                            gen_cmd.append('--shuffle-queries')
                        rc_gen, to_gen, _ = run_cmd(gen_cmd, stdout_path=in_path, stderr_path=gen_stderr, timeout=None)
                        if rc_gen != 0 or to_gen:
                            rows.append(Row(mode, n, seed, sl, sq, 127, 0, 0, None, None, str(case_dir)))
                            continue

                        rc_sol, to_sol, sec, rss = run_solver_with_time(solver, in_path, out_path, time_path, sol_stderr, args.timeout)
                        val_ok = 0
                        if rc_sol == 0 and not to_sol:
                            rc_val, to_val, _ = run_cmd([sys.executable, str(VAL), str(in_path), str(out_path), '--quiet'],
                                                        stderr_path=val_stderr, timeout=30.0)
                            val_ok = 1 if (rc_val == 0 and not to_val) else 0
                        rows.append(Row(mode, n, seed, sl, sq, rc_sol, 1 if to_sol else 0, val_ok, sec, rss, str(case_dir)))

    csv_path = out / 'hunt.csv'
    with open(csv_path, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['mode', 'n', 'seed', 'shuffle_labels', 'shuffle_queries', 'solver_rc', 'timed_out', 'val_ok', 'sec', 'rss_kb', 'case_dir'])
        for r in rows:
            w.writerow([r.mode, r.n, r.seed, r.shuffle_labels, r.shuffle_queries, r.solver_rc, r.timed_out, r.val_ok,
                        '' if r.sec is None else f'{r.sec:.6f}', '' if r.rss_kb is None else str(r.rss_kb), r.case_dir])

    ranked = sorted([r for r in rows if r.sec is not None], key=lambda x: x.sec, reverse=True)
    top = ranked[:args.top_k]
    summary = out / 'hunt_summary.md'
    with open(summary, 'w', encoding='utf-8') as f:
        f.write('# Hardest-case hunt\n\n')
        f.write('상위 케이스는 현재 solver 기준으로 가장 느리게 측정된 조합이다. 느린 풀이를 반박하려면 이 목록에서 timeout/scale 문제가 없어야 한다.\n\n')
        f.write(markdown_table(
            ['rank', 'mode', 'n', 'seed', 'L', 'Q', 'sec', 'rss_kb', 'val_ok', 'case_dir'],
            [[str(i + 1), r.mode, str(r.n), str(r.seed), str(r.shuffle_labels), str(r.shuffle_queries),
              '-' if r.sec is None else f'{r.sec:.3f}', '-' if r.rss_kb is None else str(r.rss_kb), str(r.val_ok), r.case_dir]
             for i, r in enumerate(top)]
        ))

    print(f'[hunt_hardest] csv={csv_path}')
    print(f'[hunt_hardest] summary={summary}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
