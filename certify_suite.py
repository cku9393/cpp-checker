#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import statistics
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional

from suite_utils import (
    default_solver_path,
    ensure_executable,
    fit_loglog_slope,
    geometric_median_growth,
    markdown_table,
    resolve_solver_path,
    run_cmd,
    run_solver_with_time,
)

ROOT = Path(__file__).resolve().parent
GEN = ROOT / "gen_case.py"
VAL = ROOT / "validator.py"
PRESETS = ROOT / "suite_presets"


@dataclass
class Row:
    stage: str
    mode: str
    n: int
    seed: int
    shuffle_labels: int
    shuffle_queries: int
    gen_ok: int
    solver_rc: int
    timed_out: int
    val_ok: int
    sec: Optional[float]
    rss_kb: Optional[int]
    case_dir: str


def _to_list_int(x, default=None):
    if x is None:
        return [] if default is None else list(default)
    if isinstance(x, list):
        return [int(v) for v in x]
    return [int(x)]


def load_preset(path_or_name: str) -> Dict:
    p = Path(path_or_name)
    if not p.exists():
        p = PRESETS / path_or_name
    if not p.exists():
        raise FileNotFoundError(path_or_name)
    with open(p, 'r', encoding='utf-8') as f:
        return json.load(f)


def mode_list_from_generator() -> List[str]:
    import subprocess

    res = subprocess.run([sys.executable, str(GEN), '--list-modes'], capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(res.stderr)
    return [ln.strip() for ln in res.stdout.splitlines() if ln.strip()]


def stage_rows_to_scaling(rows: List[Row]) -> Dict[str, Dict]:
    grouped: Dict[str, Dict[int, List[float]]] = defaultdict(lambda: defaultdict(list))
    for r in rows:
        if r.val_ok == 1 and r.sec is not None:
            grouped[r.mode][r.n].append(r.sec)

    out: Dict[str, Dict] = {}
    for mode, mp in grouped.items():
        med_points = []
        for n, vals in sorted(mp.items()):
            med_points.append((n, float(statistics.median(vals))))
        out[mode] = {
            'points': med_points,
            'alpha': fit_loglog_slope(med_points),
            'ratio': geometric_median_growth(med_points),
            'max_sec': max((t for _, t in med_points), default=None),
        }
    return out


def run_one_case(solver: Path, out_dir: Path, stage_name: str, mode: str, n: int, seed: int,
                 shuffle_labels: int, shuffle_queries: int, timeout: Optional[float]) -> Row:
    case_dir = out_dir / 'runs' / stage_name / mode / f'n{n}' / f'seed{seed}_L{shuffle_labels}_Q{shuffle_queries}'
    case_dir.mkdir(parents=True, exist_ok=True)

    in_path = case_dir / 'in.txt'
    out_path = case_dir / 'out.txt'
    meta_path = case_dir / 'meta.json'
    hid_path = case_dir / 'hidden_parent.txt'
    time_path = case_dir / 'time.txt'
    sol_stderr = case_dir / 'solver_stderr.txt'
    gen_stderr = case_dir / 'gen_stderr.txt'
    val_stderr = case_dir / 'val_stderr.txt'

    gen_cmd = [sys.executable, str(GEN), '--mode', mode, '--n', str(n), '--seed', str(seed),
               '--meta', str(meta_path), '--parent-out', str(hid_path)]
    if shuffle_labels:
        gen_cmd.append('--shuffle-labels')
    if shuffle_queries:
        gen_cmd.append('--shuffle-queries')

    rc_gen, to_gen, _ = run_cmd(gen_cmd, stdout_path=in_path, stderr_path=gen_stderr, timeout=None)
    gen_ok = 1 if (rc_gen == 0 and not to_gen and in_path.exists() and in_path.stat().st_size > 0) else 0

    if not gen_ok:
        return Row(stage_name, mode, n, seed, shuffle_labels, shuffle_queries, 0, 127, 0, 0, None, None, str(case_dir))

    rc_sol, to_sol, sec, rss = run_solver_with_time(solver, in_path, out_path, time_path, sol_stderr, timeout)

    val_ok = 0
    if (rc_sol == 0) and (not to_sol) and out_path.exists():
        rc_val, to_val, _ = run_cmd([sys.executable, str(VAL), str(in_path), str(out_path), '--quiet'],
                                    stderr_path=val_stderr, timeout=30.0)
        val_ok = 1 if (rc_val == 0 and not to_val) else 0

    return Row(stage_name, mode, n, seed, shuffle_labels, shuffle_queries, 1, rc_sol, 1 if to_sol else 0,
               val_ok, sec, rss, str(case_dir))


def main() -> int:
    ap = argparse.ArgumentParser(description='Run certification-grade stress/benchmark suite for BOJ 28350.')
    ap.add_argument('--solver', default=str(default_solver_path(ROOT)))
    ap.add_argument('--preset', default='strong_gate.json')
    ap.add_argument('--out', default='cert_out')
    ap.add_argument('--timeout-scale', type=float, default=1.0, help='Backward-compatible scale for time-based limits; applied together with --limit-scale.')
    ap.add_argument('--limit-scale', type=float, default=1.0, help='Scale all time-based limits (timeout, sec_max, case_sec_max).')
    args = ap.parse_args()

    solver = resolve_solver_path(args.solver, root=ROOT)
    ensure_executable(solver)
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)

    preset = load_preset(args.preset)
    rows: List[Row] = []
    limit_scale = float(args.timeout_scale) * float(args.limit_scale)

    for stage in preset['stages']:
        stage_name = stage['name']
        modes = stage.get('modes') or mode_list_from_generator()
        sizes = _to_list_int(stage.get('sizes'), [])
        seeds = _to_list_int(stage.get('seeds'), [1])
        shuf_l = _to_list_int(stage.get('shuffle_labels'), [0])
        shuf_q = _to_list_int(stage.get('shuffle_queries'), [0])
        timeout = stage.get('timeout')
        if timeout is not None:
            timeout = float(timeout) * limit_scale

        for mode in modes:
            for n in sizes:
                for seed in seeds:
                    for sl in shuf_l:
                        for sq in shuf_q:
                            rows.append(run_one_case(solver, out, stage_name, mode, n, seed, sl, sq, timeout))

    # Raw CSV
    csv_path = out / 'certify_rows.csv'
    with open(csv_path, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['stage', 'mode', 'n', 'seed', 'shuffle_labels', 'shuffle_queries', 'gen_ok', 'solver_rc',
                    'timed_out', 'val_ok', 'sec', 'rss_kb', 'case_dir'])
        for r in rows:
            w.writerow([r.stage, r.mode, r.n, r.seed, r.shuffle_labels, r.shuffle_queries, r.gen_ok, r.solver_rc,
                        r.timed_out, r.val_ok, '' if r.sec is None else f'{r.sec:.6f}',
                        '' if r.rss_kb is None else str(r.rss_kb), r.case_dir])

    reasons: List[str] = []
    overall = 'PASS'
    stage_blocks = []
    stage_json = []

    for stage in preset['stages']:
        name = stage['name']
        must_pass = bool(stage.get('must_pass', True))
        alpha_max = stage.get('alpha_max')
        ratio_max = stage.get('ratio_max')
        sec_max = stage.get('sec_max')
        case_sec_max = stage.get('case_sec_max')
        if sec_max is not None:
            sec_max = float(sec_max) * limit_scale
        if case_sec_max is not None:
            case_sec_max = float(case_sec_max) * limit_scale
        srows = [r for r in rows if r.stage == name]
        bad = [r for r in srows if not (r.gen_ok == 1 and r.solver_rc == 0 and r.timed_out == 0 and r.val_ok == 1)]
        timeouts = sum(r.timed_out for r in srows)
        wa_or_re = sum(1 for r in srows if r.gen_ok == 1 and (r.solver_rc != 0 or r.val_ok == 0) and r.timed_out == 0)
        scaling = stage_rows_to_scaling(srows)
        scale_fail = []
        worst_case_by_mode: Dict[str, Optional[float]] = {}
        succ_rows_by_mode: Dict[str, List[Row]] = defaultdict(list)
        for r in srows:
            if r.val_ok == 1 and r.sec is not None:
                succ_rows_by_mode[r.mode].append(r)
        for mode, vals in succ_rows_by_mode.items():
            worst_case_by_mode[mode] = max(r.sec for r in vals if r.sec is not None)

        for mode, info in sorted(scaling.items()):
            alpha = info['alpha']
            ratio = info['ratio']
            mx = info['max_sec']
            wc = worst_case_by_mode.get(mode)
            if alpha_max is not None and alpha is not None and alpha > float(alpha_max):
                scale_fail.append(f'{mode}: alpha={alpha:.3f} > {float(alpha_max):.3f}')
            if ratio_max is not None and ratio is not None and ratio > float(ratio_max):
                scale_fail.append(f'{mode}: ratio={ratio:.3f} > {float(ratio_max):.3f}')
            if sec_max is not None and mx is not None and mx > float(sec_max):
                scale_fail.append(f'{mode}: max_median_sec={mx:.3f} > {float(sec_max):.3f}')
            if case_sec_max is not None and wc is not None and wc > float(case_sec_max):
                scale_fail.append(f'{mode}: worst_case_sec={wc:.3f} > {float(case_sec_max):.3f}')

        stage_status = 'PASS'
        if bad or scale_fail:
            stage_status = 'FAIL' if must_pass else 'WARN'
        if stage_status == 'FAIL':
            overall = 'FAIL'
            if bad:
                reasons.append(f'{name}: {len(bad)} failing cases')
            reasons.extend([f'{name}: {msg}' for msg in scale_fail[:10]])
        elif stage_status == 'WARN' and overall == 'PASS':
            overall = 'WARN'
            if bad:
                reasons.append(f'{name}: {len(bad)} soft failures')
            reasons.extend([f'{name}: {msg}' for msg in scale_fail[:10]])

        table_rows = []
        for mode in sorted(set(r.mode for r in srows)):
            info = scaling.get(mode, {'points': [], 'alpha': None, 'ratio': None, 'max_sec': None})
            pts = ', '.join(f'{n}:{t:.3f}' for n, t in info['points']) if info['points'] else '-'
            alpha_txt = '-' if info['alpha'] is None else f"{info['alpha']:.3f}"
            ratio_txt = '-' if info['ratio'] is None else f"{info['ratio']:.3f}"
            mx_txt = '-' if info['max_sec'] is None else f"{info['max_sec']:.3f}"
            wc = worst_case_by_mode.get(mode)
            wc_txt = '-' if wc is None else f"{wc:.3f}"
            bad_cnt = sum(1 for r in srows if r.mode == mode and not (r.gen_ok == 1 and r.solver_rc == 0 and r.timed_out == 0 and r.val_ok == 1))
            table_rows.append([mode, str(bad_cnt), alpha_txt, ratio_txt, mx_txt, wc_txt, pts])

        stage_blocks.append(f"## Stage: {name}\n\n"
                            f"status: **{stage_status}**  \n"
                            f"cases: {len(srows)}  \n"
                            f"timeouts: {timeouts}  \n"
                            f"re/wa: {wa_or_re}  \n\n" +
                            markdown_table(['mode', 'bad_cases', 'alpha', 'worst_ratio', 'max_median_sec', 'worst_case_sec', 'median_sec_by_n'], table_rows))
        if scale_fail:
            stage_blocks.append('Scale check hits:\n\n' + '\n'.join(f'- {msg}' for msg in scale_fail) + '\n')

        stage_json.append({
            'name': name,
            'status': stage_status,
            'cases': len(srows),
            'timeouts': timeouts,
            're_wa': wa_or_re,
            'limit_scale': limit_scale,
            'sec_max': sec_max,
            'case_sec_max': case_sec_max,
            'scale_fail': scale_fail,
        })

    slowest = sorted([r for r in rows if r.sec is not None], key=lambda x: x.sec, reverse=True)[:20]
    slow_rows = [[r.stage, r.mode, str(r.n), str(r.seed), f'{r.sec:.3f}', '-' if r.rss_kb is None else str(r.rss_kb), r.case_dir] for r in slowest]

    summary_md = out / 'certify_summary.md'
    with open(summary_md, 'w', encoding='utf-8') as f:
        f.write(f"# Certification summary\n\n")
        f.write(f"overall verdict: **{overall}**\n\n")
        if reasons:
            f.write('## Reasons\n\n')
            for msg in reasons:
                f.write(f'- {msg}\n')
            f.write('\n')
        f.write('이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.\n\n')
        for block in stage_blocks:
            f.write(block)
            f.write('\n')
        f.write('## Top slow cases\n\n')
        f.write(markdown_table(['stage', 'mode', 'n', 'seed', 'sec', 'rss_kb', 'case_dir'], slow_rows))

    with open(out / 'certify.json', 'w', encoding='utf-8') as f:
        json.dump({
            'verdict': overall,
            'reasons': reasons,
            'preset': preset.get('name', args.preset),
            'stages': stage_json,
        }, f, ensure_ascii=False, indent=2)

    print(f'[certify_suite] verdict={overall}')
    print(f'[certify_suite] rows={csv_path}')
    print(f'[certify_suite] summary={summary_md}')
    return 0 if overall == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
