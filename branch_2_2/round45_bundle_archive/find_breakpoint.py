#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import subprocess
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
GEN = ROOT / "gen_case.py"
VAL = ROOT / "validator.py"


def run_once(solver: str, mode: str, n: int, seed: int, timeout: float, shuffle_labels: bool, shuffle_queries: bool, validate: bool):
    with tempfile.TemporaryDirectory(prefix="boj28350_") as td:
        td = Path(td)
        in_path = td / "in.txt"
        out_path = td / "out.txt"

        gen_cmd = [
            "python3",
            str(GEN),
            "--mode",
            mode,
            "--n",
            str(n),
            "--seed",
            str(seed),
        ]
        if shuffle_labels:
            gen_cmd.append("--shuffle-labels")
        if shuffle_queries:
            gen_cmd.append("--shuffle-queries")

        with in_path.open("wb") as fout:
            subprocess.run(gen_cmd, check=True, stdout=fout)

        start = time.perf_counter()
        try:
            with in_path.open("rb") as fin, out_path.open("wb") as fout:
                subprocess.run([solver], stdin=fin, stdout=fout, check=True, timeout=timeout)
        except subprocess.TimeoutExpired:
            return False, timeout, "timeout"
        except subprocess.CalledProcessError as e:
            return False, time.perf_counter() - start, f"solver_exit={e.returncode}"

        elapsed = time.perf_counter() - start
        if validate:
            try:
                subprocess.run(["python3", str(VAL), str(in_path), str(out_path), "--quiet"], check=True)
            except subprocess.CalledProcessError:
                return False, elapsed, "wrong_answer"
        return True, elapsed, "ok"


def all_pass(solver: str, mode: str, n: int, seeds, timeout: float, shuffle_labels: bool, shuffle_queries: bool, validate: bool):
    worst = 0.0
    for seed in seeds:
        ok, elapsed, reason = run_once(solver, mode, n, seed, timeout, shuffle_labels, shuffle_queries, validate)
        worst = max(worst, elapsed)
        if not ok:
            return False, worst, reason
    return True, worst, "ok"


def main() -> int:
    parser = argparse.ArgumentParser(description="Find largest n that survives a timeout under a generator mode.")
    parser.add_argument("--solver", default=str(ROOT / "solve"))
    parser.add_argument("--mode", required=True)
    parser.add_argument("--timeout", type=float, default=2.0)
    parser.add_argument("--start", type=int, default=1024)
    parser.add_argument("--n-max", type=int, default=100000)
    parser.add_argument("--seeds", default="1")
    parser.add_argument("--shuffle-labels", action="store_true")
    parser.add_argument("--shuffle-queries", action="store_true")
    parser.add_argument("--no-validate", action="store_true")
    args = parser.parse_args()

    seeds = [int(x) for x in args.seeds.split(",") if x.strip()]
    validate = not args.no_validate

    lo = 0
    hi = min(max(1, args.start), args.n_max)

    ok, t, reason = all_pass(args.solver, args.mode, hi, seeds, args.timeout, args.shuffle_labels, args.shuffle_queries, validate)
    print(f"probe n={hi}: ok={ok} time={t:.4f}s reason={reason}")
    if not ok:
        hi_fail = hi
        lo_pass = 0
    else:
        lo_pass = hi
        cur = hi
        hi_fail = args.n_max + 1
        while cur < args.n_max:
            nxt = min(args.n_max, cur * 2)
            ok, t, reason = all_pass(args.solver, args.mode, nxt, seeds, args.timeout, args.shuffle_labels, args.shuffle_queries, validate)
            print(f"probe n={nxt}: ok={ok} time={t:.4f}s reason={reason}")
            if ok:
                lo_pass = nxt
                cur = nxt
            else:
                hi_fail = nxt
                break
        if hi_fail == args.n_max + 1:
            print(f"[find_breakpoint] survived up to n={args.n_max}")
            return 0

    l, r = lo_pass, hi_fail
    while l + 1 < r:
        mid = (l + r) // 2
        ok, t, reason = all_pass(args.solver, args.mode, mid, seeds, args.timeout, args.shuffle_labels, args.shuffle_queries, validate)
        print(f"probe n={mid}: ok={ok} time={t:.4f}s reason={reason}")
        if ok:
            l = mid
        else:
            r = mid

    print(f"[find_breakpoint] max passing n = {l}, first failing n = {r}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
