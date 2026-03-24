#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import subprocess
import sys
from pathlib import Path
from typing import Iterable

from artifact_paths import resolve_output_path
from suite_utils import ensure_executable, resolve_solver_path, run_solver_with_time


ROOT = Path(__file__).resolve().parent
RESUME_DIR = ROOT / "boj28350_resume"
SOURCE = RESUME_DIR / "boj28350_branch_3_solver.cpp"
DEFAULT_SOLVER = RESUME_DIR / "solve"
DEFAULT_CASES = RESUME_DIR / "smoke_cases.tsv"
STRESS_DIR = ROOT / "lca_tree_stress_v5"
if not STRESS_DIR.exists():
    STRESS_DIR = ROOT.parent / "lca_tree_stress_v5"
GEN = STRESS_DIR / "gen_case.py"
VAL = STRESS_DIR / "validator.py"


def parse_cases(tsv_path: Path) -> list[dict[str, str]]:
    with tsv_path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f, delimiter="\t"))


def resolve_branch_path(value: str) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = ROOT / path
    return path.resolve()


def resolve_branch_solver(value: str) -> Path:
    solver = resolve_solver_path(value, root=ROOT)
    if solver.exists():
        return solver
    return resolve_solver_path(value, root=ROOT.parent)


def build_solver(compiler: str | None, static_mode: str, defines: list[str]) -> int:
    out_path = RESUME_DIR / "solve"
    cmd = [
        sys.executable,
        str(ROOT / "build.py"),
        "--source",
        str(SOURCE.relative_to(ROOT)),
        "--out",
        str(out_path),
        "--static",
        static_mode,
    ]
    if compiler:
        cmd.extend(["--compiler", compiler])
    for define in defines:
        cmd.extend(["--define", define])
    return subprocess.run(cmd, cwd=ROOT).returncode


def run_case(row: dict[str, str], solver: Path, base_out: Path, default_timeout_s: float) -> dict[str, str]:
    mode = row["mode"]
    n = int(row["n"])
    seed = int(row.get("seed", "1"))
    shuffle_labels = int(row.get("shuffle_labels", "1"))
    shuffle_queries = int(row.get("shuffle_queries", "1"))
    stage = row.get("stage", "smoke")
    timeout_s = float(row.get("timeout_s", default_timeout_s))
    tag = f"{stage}_{mode}_{n}_s{seed}"
    case_dir = base_out / tag
    case_dir.mkdir(parents=True, exist_ok=True)

    in_path = case_dir / "in.txt"
    out_path = case_dir / "out.txt"
    meta_path = case_dir / "meta.json"
    hidden_parent_path = case_dir / "hidden_parent.txt"
    time_path = case_dir / "time.txt"
    stderr_path = case_dir / "solver_stderr.txt"

    gen_cmd = [
        sys.executable,
        str(GEN),
        "--mode",
        mode,
        "--n",
        str(n),
        "--seed",
        str(seed),
        "--meta",
        str(meta_path),
        "--parent-out",
        str(hidden_parent_path),
    ]
    if shuffle_labels:
        gen_cmd.append("--shuffle-labels")
    if shuffle_queries:
        gen_cmd.append("--shuffle-queries")

    with in_path.open("wb") as fout:
        subprocess.run(gen_cmd, check=True, stdout=fout, cwd=ROOT)

    rc, timed_out, sec, rss_kb = run_solver_with_time(
        solver,
        in_path,
        out_path,
        time_path,
        stderr_path,
        timeout=timeout_s,
        cwd=case_dir,
    )

    validator_status = ""
    validator_msg = ""
    if timed_out:
        validator_status = "TIMEOUT"
    elif rc != 0:
        validator_status = f"RC{rc}"
    else:
        res = subprocess.run(
            [sys.executable, str(VAL), str(in_path), str(out_path), "--quiet"],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        validator_msg = (res.stdout + res.stderr).strip().replace("\n", " | ")
        validator_status = "OK" if res.returncode == 0 else "FAIL"
        (case_dir / "validator.txt").write_text(res.stdout + res.stderr, encoding="utf-8")

    return {
        "stage": stage,
        "mode": mode,
        "n": str(n),
        "seed": str(seed),
        "shuffle_labels": str(shuffle_labels),
        "shuffle_queries": str(shuffle_queries),
        "timeout_s": f"{timeout_s:g}",
        "rc": "124" if timed_out else str(rc),
        "validator": validator_status,
        "validator_msg": validator_msg,
        "elapsed_s": "" if sec is None else f"{sec:.6f}",
        "mem_kb": "" if rss_kb is None else str(rss_kb),
        "case_dir": str(case_dir),
    }


def write_summary(rows: Iterable[dict[str, str]], out_path: Path) -> None:
    rows = list(rows)
    with out_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "stage",
                "mode",
                "n",
                "seed",
                "shuffle_labels",
                "shuffle_queries",
                "timeout_s",
                "rc",
                "validator",
                "validator_msg",
                "elapsed_s",
                "mem_kb",
                "case_dir",
            ],
            delimiter="\t",
        )
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    ap = argparse.ArgumentParser(description="BOJ 28350 branch-local helper rooted inside branch_3.")
    sub = ap.add_subparsers(dest="cmd", required=True)

    build_ap = sub.add_parser("build", help="Build the current BOJ 28350 solver.")
    build_ap.add_argument("--compiler", default=None)
    build_ap.add_argument("--static", choices=("auto", "always", "never"), default="auto")
    build_ap.add_argument("--define", action="append", default=[])

    smoke_ap = sub.add_parser("smoke", help="Run branch-local BOJ 28350 smoke cases.")
    smoke_ap.add_argument("--solver", default=str(DEFAULT_SOLVER))
    smoke_ap.add_argument("--cases", default=str(DEFAULT_CASES))
    smoke_ap.add_argument("--out", default=None)
    smoke_ap.add_argument("--timeout", type=float, default=5.0)

    args = ap.parse_args()

    if args.cmd == "build":
        return build_solver(args.compiler, args.static, list(args.define))

    solver = resolve_branch_solver(args.solver)
    ensure_executable(solver)
    tsv_path = resolve_branch_path(args.cases)
    out_dir = resolve_output_path(args.out, default_key="boj28350_smoke")
    out_dir.mkdir(parents=True, exist_ok=True)

    rows = []
    for row in parse_cases(tsv_path):
        result = run_case(row, solver, out_dir, args.timeout)
        rows.append(result)
        print(
            f"[boj28350 smoke] stage={result['stage']} mode={result['mode']} "
            f"n={result['n']} seed={result['seed']} rc={result['rc']} "
            f"validator={result['validator']} elapsed={result['elapsed_s'] or '-'} "
            f"mem_kb={result['mem_kb'] or '-'}",
            flush=True,
        )
    write_summary(rows, out_dir / "smoke_summary.tsv")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
