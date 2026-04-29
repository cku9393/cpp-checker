#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

from suite_utils import default_solver_path, ensure_executable, resolve_solver_path, run_solver_with_time


ROOT = Path(__file__).resolve().parent
GEN = ROOT / "gen_case.py"
VAL = ROOT / "validator.py"


def parse_env_assignments(items: list[str]) -> dict[str, str]:
    env = {}
    for item in items:
        key, sep, value = item.partition("=")
        if not sep or not key:
            raise ValueError(f"invalid --env assignment: {item!r}")
        env[key] = value
    return env


def build_case_solver_env(outdir: Path, mode: str, n: int, seed: int) -> dict[str, str]:
    env = os.environ.copy()
    env["DENSE_PROFILE_OUTDIR"] = str(outdir)
    env["DENSE_SHADOW_CASE_MODE"] = mode
    env["DENSE_SHADOW_CASE_N"] = str(n)
    env["DENSE_SHADOW_CASE_SEED"] = str(seed)
    return env


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate, run, and validate one stress case.")
    ap.add_argument("mode")
    ap.add_argument("n", type=int)
    ap.add_argument("seed", nargs="?", type=int, default=1)
    ap.add_argument("shuffle_labels", nargs="?", type=int, default=0)
    ap.add_argument("shuffle_queries", nargs="?", type=int, default=0)
    ap.add_argument("solver", nargs="?", default=str(default_solver_path(ROOT)))
    ap.add_argument("outdir", nargs="?", default="_case")
    ap.add_argument("--timeout", type=float, default=None)
    ap.add_argument("--env", action="append", default=[], help="repeatable KEY=VALUE assignment passed to the solver")
    args = ap.parse_args()

    solver = resolve_solver_path(args.solver, root=ROOT)
    ensure_executable(solver)

    outdir = Path(args.outdir)
    if not outdir.is_absolute():
        outdir = (ROOT / outdir).resolve()
    else:
        outdir = outdir.resolve()
    outdir.mkdir(parents=True, exist_ok=True)

    in_path = outdir / "in.txt"
    out_path = outdir / "out.txt"
    meta_path = outdir / "meta.json"
    hid_path = outdir / "hidden_parent.txt"
    time_path = outdir / "time.txt"
    sol_stderr = outdir / "solver_stderr.txt"
    solver_env = build_case_solver_env(outdir, args.mode, args.n, args.seed)
    solver_env.update(parse_env_assignments(args.env))

    gen_cmd = [
        sys.executable,
        str(GEN),
        "--mode",
        args.mode,
        "--n",
        str(args.n),
        "--seed",
        str(args.seed),
        "--meta",
        str(meta_path),
        "--parent-out",
        str(hid_path),
    ]
    if args.shuffle_labels:
        gen_cmd.append("--shuffle-labels")
    if args.shuffle_queries:
        gen_cmd.append("--shuffle-queries")

    with in_path.open("wb") as fout:
        subprocess.run(gen_cmd, check=True, stdout=fout)

    rc_sol, timed_out, sec, rss_kb = run_solver_with_time(
        solver,
        in_path,
        out_path,
        time_path,
        sol_stderr,
        timeout=args.timeout,
        env=solver_env,
        cwd=outdir,
    )
    if timed_out:
        print(f"[run_case] solver timed out after {args.timeout}s", file=sys.stderr)
        return 124
    if rc_sol != 0:
        print(f"[run_case] solver exited with code {rc_sol}", file=sys.stderr)
        return rc_sol

    rc_val = subprocess.run([sys.executable, str(VAL), str(in_path), str(out_path)]).returncode
    if rc_val != 0:
        return rc_val

    rss_text = "n/a" if rss_kb is None or rss_kb < 0 else f"{rss_kb}KB"
    print(f"[run_case] mode={args.mode} n={args.n} seed={args.seed} time={sec:.6f}s mem={rss_text}")
    print(f"[run_case] artifacts: {in_path} {out_path} {meta_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
