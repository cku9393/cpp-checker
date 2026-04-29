#!/usr/bin/env python3
"""Benchmark runner that outputs CSV + a compact Markdown table.

Runs a full loop:
  generate -> solve -> validate

It records:
  - elapsed seconds and best-effort max RSS (kB)
  - exit codes / timeouts
  - validator pass/fail

Artifacts are stored under --out (inputs, outputs, meta, logs), and
results are written to:
  - <out>/bench.csv
  - <out>/bench_summary.md
  - <out>/bench_pivot.csv

Example:
  python bench_report.py --solver ./solve --out bench_out \
    --modes comb_core,comb_dense --sizes 9999,19999,39999 --seeds 1,2,3 \
    --shuffle-labels --shuffle-queries --timeout 2.0
"""

from __future__ import annotations

import argparse
import csv
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional

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
GEN = ROOT / "gen_case.py"
VAL = ROOT / "validator.py"


def parse_int_list(s: str) -> List[int]:
    return parse_int_list_csv(s)


def parse_str_list(s: str) -> List[str]:
    return parse_str_list_csv(s)


@dataclass
class Row:
    mode: str
    n: int
    seed: int
    shuffle_labels: int
    shuffle_queries: int
    solver_rc: int
    timed_out: int
    gen_ok: int
    val_ok: int
    sec: Optional[float]
    rss_kb: Optional[int]
    case_dir: str


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--solver", default=str(default_solver_path(ROOT)), help="path to solver executable")
    ap.add_argument("--out", default="bench_out", help="output directory")
    ap.add_argument("--modes", default="", help="comma-separated modes; empty=all")
    ap.add_argument("--sizes", default="9999,19999,39999,79999,99999", help="comma-separated N")
    ap.add_argument("--seeds", default="1", help="comma-separated seeds")
    ap.add_argument("--shuffle-labels", action="store_true")
    ap.add_argument("--shuffle-queries", action="store_true")
    ap.add_argument("--timeout", type=float, default=None, help="per-run timeout (sec)")
    ap.add_argument("--keep", action="store_true", help="keep artifacts even if all-pass")
    args = ap.parse_args()

    solver = resolve_solver_path(args.solver, root=ROOT)
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)

    ensure_executable(solver)

    sizes = parse_int_list(args.sizes)
    seeds = parse_int_list(args.seeds)

    if args.modes.strip():
        modes = parse_str_list(args.modes)
    else:
        # Pull from generator.
        res = subprocess.run([sys.executable, str(GEN), "--list-modes"], capture_output=True, text=True)
        if res.returncode != 0:
            print(res.stderr, file=sys.stderr)
            return 2
        modes = [ln.strip() for ln in res.stdout.splitlines() if ln.strip() and not ln.startswith("[")]

    shuffle_labels = 1 if args.shuffle_labels else 0
    shuffle_queries = 1 if args.shuffle_queries else 0

    rows: List[Row] = []

    for mode in modes:
        for n in sizes:
            for seed in seeds:
                case_dir = out / "runs" / mode / f"n{n}" / f"seed{seed}_L{shuffle_labels}_Q{shuffle_queries}"
                case_dir.mkdir(parents=True, exist_ok=True)

                in_path = case_dir / "in.txt"
                out_path = case_dir / "out.txt"
                meta_path = case_dir / "meta.json"
                hid_path = case_dir / "hidden_parent.txt"
                time_path = case_dir / "time.txt"
                sol_stderr = case_dir / "solver_stderr.txt"
                gen_stderr = case_dir / "gen_stderr.txt"
                val_stderr = case_dir / "val_stderr.txt"

                gen_cmd = [sys.executable, str(GEN), "--mode", mode, "--n", str(n), "--seed", str(seed),
                           "--meta", str(meta_path), "--parent-out", str(hid_path)]
                if shuffle_labels:
                    gen_cmd.append("--shuffle-labels")
                if shuffle_queries:
                    gen_cmd.append("--shuffle-queries")

                rc_gen, to_gen, _ = run_cmd(gen_cmd, stdout_path=in_path, stderr_path=gen_stderr, timeout=None)
                gen_ok = 1 if (rc_gen == 0 and not to_gen and in_path.exists() and in_path.stat().st_size > 0) else 0

                if not gen_ok:
                    rows.append(Row(mode, n, seed, shuffle_labels, shuffle_queries, 127, 0, 0, 0, None, None, str(case_dir)))
                    continue

                rc_sol, to_sol, sec, rss = run_solver_with_time(
                    solver, in_path, out_path, time_path, sol_stderr, args.timeout
                )

                val_ok = 0
                if (rc_sol == 0) and (not to_sol) and out_path.exists():
                    rc_val, to_val, _ = run_cmd(
                        [sys.executable, str(VAL), str(in_path), str(out_path)],
                        stdout_path=None,
                        stderr_path=val_stderr,
                        timeout=30.0,
                    )
                    val_ok = 1 if (rc_val == 0 and not to_val) else 0

                rows.append(
                    Row(
                        mode=mode,
                        n=n,
                        seed=seed,
                        shuffle_labels=shuffle_labels,
                        shuffle_queries=shuffle_queries,
                        solver_rc=rc_sol,
                        timed_out=1 if to_sol else 0,
                        gen_ok=gen_ok,
                        val_ok=val_ok,
                        sec=sec,
                        rss_kb=rss,
                        case_dir=str(case_dir),
                    )
                )

    # Write CSV
    csv_path = out / "bench.csv"
    with open(csv_path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow([
            "mode",
            "n",
            "seed",
            "shuffle_labels",
            "shuffle_queries",
            "gen_ok",
            "solver_rc",
            "timed_out",
            "val_ok",
            "sec",
            "rss_kb",
            "case_dir",
        ])
        for r in rows:
            w.writerow([
                r.mode,
                r.n,
                r.seed,
                r.shuffle_labels,
                r.shuffle_queries,
                r.gen_ok,
                r.solver_rc,
                r.timed_out,
                r.val_ok,
                "" if r.sec is None else f"{r.sec:.6f}",
                "" if r.rss_kb is None else str(r.rss_kb),
                r.case_dir,
            ])

    # Pivot summary: for each (mode,n) take min sec over seeds among val_ok runs.
    pivot = {}
    for r in rows:
        key = (r.mode, r.n)
        if r.val_ok != 1 or r.sec is None:
            continue
        pivot.setdefault(key, []).append(r.sec)

    pivot_rows = []
    for mode in modes:
        for n in sizes:
            vals = pivot.get((mode, n), [])
            pivot_rows.append((mode, n, (min(vals) if vals else None)))

    pivot_csv = out / "bench_pivot.csv"
    with open(pivot_csv, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["mode", "n", "min_sec_over_seeds"])
        for mode, n, v in pivot_rows:
            w.writerow([mode, n, "" if v is None else f"{v:.6f}"])

    # Markdown table per mode, columns=sizes.
    md_path = out / "bench_summary.md"
    header = ["mode"] + [str(n) for n in sizes]
    md_rows: List[List[str]] = []
    for mode in modes:
        row = [mode]
        for n in sizes:
            v = next((x[2] for x in pivot_rows if x[0] == mode and x[1] == n), None)
            if v is None:
                row.append("(fail/timeout)")
            else:
                row.append(f"{v*1000:.1f}ms")
        md_rows.append(row)

    md_path.write_text(
        "# Benchmark summary (min over seeds)\n\n"
        f"solver: `{solver}`\n\n"
        f"cmd: `{shlex.join(sys.argv)}`\n\n"
        + markdown_table(header, md_rows)
    )

    print(f"[bench] wrote: {csv_path}")
    print(f"[bench] wrote: {md_path}")
    print(f"[bench] wrote: {pivot_csv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
