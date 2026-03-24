#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import os
import subprocess
import sys
from pathlib import Path
from typing import Iterable

from suite_utils import ensure_executable, resolve_solver_path, run_solver_with_time


ROOT = Path(__file__).resolve().parent
RESUME_DIR = ROOT / "round45_resume"
SOURCE = RESUME_DIR / "round45_branch_2_2_solver.cpp"
DEFAULT_SOLVER = RESUME_DIR / "solve_prof"
DEFAULT_CASES = RESUME_DIR / "smoke_cases.tsv"
GEN = ROOT / "round45_bundle_archive" / "gen_case.py"
VAL = ROOT / "round45_bundle_archive" / "validator.py"
PROFILE_DEFINES = [
    "DENSE_SHADOW_DIFF_ROUND20_PROFILE=1",
    "DENSE_DECOMPOSESERIES_ROUND38_PROFILE=1",
    "DENSE_SPQR_ROUND16_SHADOWCHECK=1",
]
PROFILE_TSVS = [
    "round20_dense_shadow_census_rows.tsv",
    "round20_dense_shadow_profile_summary.tsv",
    "round38_dense_decomposeseries_census_rows.tsv",
    "round38_dense_decomposeseries_rows.tsv",
    "census_rows.tsv",
    "candidate_rows.tsv",
    "prefilter_rows.tsv",
    "decompose_rows.tsv",
    "round45_dense_stseparator_census_rows.tsv",
    "round45_dense_stseparator_rows.tsv",
    "round45_dense_stseparator_prefilter_rows.tsv",
]


def parse_cases(tsv_path: Path) -> list[dict[str, str]]:
    with tsv_path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f, delimiter="\t"))


def count_lines(path: Path) -> int:
    if not path.exists():
        return 0
    with path.open("r", encoding="utf-8", errors="replace") as f:
        return sum(1 for _ in f)


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


def build_solver(compiler: str | None, static_mode: str, mode: str) -> int:
    out_path = RESUME_DIR / ("solve_prof" if mode == "profile" else "solve")
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
    if mode == "profile":
        for define in PROFILE_DEFINES:
            cmd.extend(["--define", define])
    return subprocess.run(cmd, cwd=ROOT).returncode


def format_generated(case_dir: Path) -> str:
    names = sorted(p.name for p in case_dir.glob("*.tsv"))
    return ",".join(names)


def run_case(row: dict[str, str], solver: Path, base_out: Path, timeout_s: float) -> dict[str, str]:
    mode = row["mode"]
    n = int(row["n"])
    seed = int(row.get("seed", "1"))
    shuffle_labels = int(row.get("shuffle_labels", "1"))
    shuffle_queries = int(row.get("shuffle_queries", "1"))
    stage = row.get("stage", "smoke")
    tag = f"{stage}_{mode}_{n}_s{seed}"
    case_dir = base_out / tag
    case_dir.mkdir(parents=True, exist_ok=True)

    for name in PROFILE_TSVS:
        target = case_dir / name
        if target.exists():
            target.unlink()

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

    env = os.environ.copy()
    env["DENSE_SHADOW_CASE_MODE"] = mode
    env["DENSE_SHADOW_CASE_N"] = str(n)
    env["DENSE_SHADOW_CASE_SEED"] = str(seed)
    env["DENSE_PROFILE_OUTDIR"] = str(case_dir)
    env["DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK"] = "1"

    rc, timed_out, sec, rss_kb = run_solver_with_time(
        solver,
        in_path,
        out_path,
        time_path,
        stderr_path,
        timeout=timeout_s,
        env=env,
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
            [sys.executable, str(VAL), str(in_path), str(out_path)],
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
        "census_rows": str(max(0, count_lines(case_dir / "census_rows.tsv") - 1)),
        "candidate_rows": str(max(0, count_lines(case_dir / "candidate_rows.tsv") - 1)),
        "prefilter_rows": str(max(0, count_lines(case_dir / "prefilter_rows.tsv") - 1)),
        "decompose_rows": str(max(0, count_lines(case_dir / "decompose_rows.tsv") - 1)),
        "round20_census_rows": str(count_lines(case_dir / "round20_dense_shadow_census_rows.tsv")),
        "round20_summary_rows": str(count_lines(case_dir / "round20_dense_shadow_profile_summary.tsv")),
        "round38_rows": str(count_lines(case_dir / "round38_dense_decomposeseries_rows.tsv")),
        "row_gate_passed": str(
            int(
                max(0, count_lines(case_dir / "census_rows.tsv") - 1) > 0
                and max(0, count_lines(case_dir / "candidate_rows.tsv") - 1) > 0
                and max(0, count_lines(case_dir / "prefilter_rows.tsv") - 1) > 0
                and max(0, count_lines(case_dir / "decompose_rows.tsv") - 1) > 0
            )
        ),
        "generated_tsvs": format_generated(case_dir),
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
                "census_rows",
                "candidate_rows",
                "prefilter_rows",
                "decompose_rows",
                "round20_census_rows",
                "round20_summary_rows",
                "round38_rows",
                "row_gate_passed",
                "generated_tsvs",
                "case_dir",
            ],
            delimiter="\t",
        )
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    ap = argparse.ArgumentParser(description="Round 45 resume helper rooted inside branch_2_2.")
    sub = ap.add_subparsers(dest="cmd", required=True)

    build_ap = sub.add_parser("build", help="Build the Round 45 baseline solver.")
    build_ap.add_argument("--compiler", default=None)
    build_ap.add_argument("--static", choices=("auto", "always", "never"), default="auto")
    build_ap.add_argument("--mode", choices=("plain", "profile"), default="profile")

    smoke_ap = sub.add_parser("smoke", help="Run the Round 45 smoke cases with profiling env.")
    smoke_ap.add_argument("--solver", default=str(DEFAULT_SOLVER))
    smoke_ap.add_argument("--cases", default=str(DEFAULT_CASES))
    smoke_ap.add_argument("--out", default=str(ROOT / "artifacts" / "round45_resume" / "smoke"))
    smoke_ap.add_argument("--timeout", type=float, default=45.0)

    args = ap.parse_args()

    if args.cmd == "build":
        return build_solver(args.compiler, args.static, args.mode)

    solver = resolve_branch_solver(args.solver)
    ensure_executable(solver)
    tsv_path = resolve_branch_path(args.cases)
    out_dir = resolve_branch_path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    rows = []
    for row in parse_cases(tsv_path):
        result = run_case(row, solver, out_dir, args.timeout)
        rows.append(result)
        print(
            f"[round45 smoke] stage={result['stage']} mode={result['mode']} "
            f"n={result['n']} seed={result['seed']} rc={result['rc']} "
            f"validator={result['validator']} "
            f"census={result['census_rows']} "
            f"candidate={result['candidate_rows']} "
            f"prefilter={result['prefilter_rows']} "
            f"decompose={result['decompose_rows']} "
            f"row_gate={result['row_gate_passed']} "
            f"r20_rows={result['round20_census_rows']} "
            f"r20_summary={result['round20_summary_rows']} "
            f"r38_rows={result['round38_rows']} "
            f"tsvs={result['generated_tsvs'] or '-'}",
            flush=True,
        )
    write_summary(rows, out_dir / "smoke_summary.tsv")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
