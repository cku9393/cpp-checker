#!/usr/bin/env python3
"""Benchmark runner that outputs CSV + a compact Markdown table.

Runs a full loop:
  generate -> solve -> validate

It records:
  - elapsed seconds and max RSS (kB) from /usr/bin/time
  - exit codes / timeouts
  - validator pass/fail

Artifacts are stored under --out (inputs, outputs, meta, logs), and
results are written to:
  - <out>/bench.csv
  - <out>/bench_summary.md
  - <out>/bench_pivot.csv

Example:
  python3 bench_report.py --solver ./solve --out bench_out \
    --modes comb_core,comb_dense --sizes 9999,19999,39999 --seeds 1,2,3 \
    --shuffle-labels --shuffle-queries --timeout 2.0
"""

from __future__ import annotations

import argparse
import csv
import os
import shlex
import shutil
import signal
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple


ROOT = Path(__file__).resolve().parent
GEN = ROOT / "gen_case.py"
VAL = ROOT / "validator.py"


def parse_int_list(s: str) -> List[int]:
    s = s.strip()
    if not s:
        return []
    return [int(x) for x in s.split(",")]


def parse_str_list(s: str) -> List[str]:
    s = s.strip()
    if not s:
        return []
    return [x.strip() for x in s.split(",") if x.strip()]


def ensure_executable(p: Path) -> None:
    if not p.exists():
        raise FileNotFoundError(str(p))
    if p.is_file() and not os.access(p, os.X_OK):
        raise PermissionError(f"Not executable: {p}")


def run_cmd(
    cmd: List[str],
    *,
    stdin_path: Optional[Path] = None,
    stdout_path: Optional[Path] = None,
    stderr_path: Optional[Path] = None,
    timeout: Optional[float] = None,
) -> Tuple[int, bool, float]:
    """Run command. Returns (exit_code, timed_out, elapsed_sec)."""

    t0 = time.perf_counter()
    stdin_f = open(stdin_path, "rb") if stdin_path else None
    stdout_f = open(stdout_path, "wb") if stdout_path else subprocess.DEVNULL
    stderr_f = open(stderr_path, "wb") if stderr_path else subprocess.DEVNULL

    try:
        # Start in its own process group so we can kill everything on timeout.
        p = subprocess.Popen(
            cmd,
            stdin=stdin_f,
            stdout=stdout_f,
            stderr=stderr_f,
            preexec_fn=os.setsid,
        )
        try:
            p.wait(timeout=timeout)
            timed_out = False
        except subprocess.TimeoutExpired:
            timed_out = True
            try:
                os.killpg(p.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            p.wait()
        rc = int(p.returncode)
    finally:
        if stdin_f:
            stdin_f.close()
        if stdout_f not in (None, subprocess.DEVNULL):
            stdout_f.close()
        if stderr_f not in (None, subprocess.DEVNULL):
            stderr_f.close()

    return rc, timed_out, time.perf_counter() - t0


def run_solver_with_time(
    solver: Path,
    in_path: Path,
    out_path: Path,
    time_path: Path,
    stderr_path: Path,
    timeout: Optional[float],
) -> Tuple[int, bool, Optional[float], Optional[int]]:
    """Run solver wrapped by /usr/bin/time and parse (sec, rss_kb)."""

    # /usr/bin/time writes to -o file, so solver stderr is kept separately.
    cmd = ["/usr/bin/time", "-f", "%e %M", "-o", str(time_path), str(solver)]
    rc, timed_out, _elapsed = run_cmd(
        cmd,
        stdin_path=in_path,
        stdout_path=out_path,
        stderr_path=stderr_path,
        timeout=timeout,
    )

    if timed_out:
        return rc, True, None, None
    if not time_path.exists():
        return rc, False, None, None

    try:
        txt = time_path.read_text().strip().split()
        sec = float(txt[0])
        rss = int(txt[1])
        return rc, False, sec, rss
    except Exception:
        return rc, False, None, None


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


def markdown_table(headers: List[str], rows: List[List[str]]) -> str:
    def esc(x: str) -> str:
        return x.replace("|", "\\|")

    out = []
    out.append("| " + " | ".join(map(esc, headers)) + " |")
    out.append("| " + " | ".join(["---"] * len(headers)) + " |")
    for r in rows:
        out.append("| " + " | ".join(map(esc, r)) + " |")
    return "\n".join(out) + "\n"


def row_all_pass(r: Row) -> bool:
    return (r.gen_ok == 1) and (r.solver_rc == 0) and (r.timed_out == 0) and (r.val_ok == 1)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--solver", default="./solve", help="path to solver executable")
    ap.add_argument("--out", default="bench_out", help="output directory")
    ap.add_argument("--modes", default="", help="comma-separated modes; empty=all")
    ap.add_argument("--sizes", default="9999,19999,39999,79999,99999", help="comma-separated N")
    ap.add_argument("--seeds", default="1", help="comma-separated seeds")
    ap.add_argument("--shuffle-labels", action="store_true")
    ap.add_argument("--shuffle-queries", action="store_true")
    ap.add_argument("--timeout", type=float, default=None, help="per-run timeout (sec)")
    ap.add_argument("--keep", action="store_true", help="keep artifacts even if all-pass")
    args = ap.parse_args()

    solver = Path(args.solver).resolve()
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

    all_pass = len(rows) > 0 and all(row_all_pass(r) for r in rows)
    runs_dir = out / "runs"
    cleaned_runs = False
    if all_pass and (not args.keep) and runs_dir.exists():
        shutil.rmtree(runs_dir)
        cleaned_runs = True

    print(f"[bench] wrote: {csv_path}")
    print(f"[bench] wrote: {md_path}")
    print(f"[bench] wrote: {pivot_csv}")
    if cleaned_runs:
        print(f"[bench] cleaned: {runs_dir} (all runs passed; use --keep to preserve artifacts)")
    elif all_pass:
        print(f"[bench] all runs passed; artifacts kept under: {runs_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
