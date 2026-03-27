#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import random
import sys
import traceback
from pathlib import Path

os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
sys.dont_write_bytecode = True

from artifact_paths import configure_branch_process_env, resolve_output_path
from branch_validator import validate_case


configure_branch_process_env()

import branch_gen_case
from suite_utils import ensure_executable, resolve_solver_path, run_solver_with_time


ROOT = Path(__file__).resolve().parent
DEFAULT_BRANCH_SOLVER = ROOT / "boj28350_resume" / "solve"
RUN_CASE_RESULT_NAME = "run_case_result.json"
RESERVED_SOLVER_ENV_KEYS = frozenset(
    {
        "BRANCH_ARTIFACT_TMP_ROOT",
        "TMPDIR",
        "TMP",
        "TEMP",
        "PYTHONDONTWRITEBYTECODE",
        "DENSE_PROFILE_OUTDIR",
        "DENSE_SHADOW_CASE_MODE",
        "DENSE_SHADOW_CASE_N",
        "DENSE_SHADOW_CASE_SEED",
    }
)
EXIT_USAGE_ERROR = 2
EXIT_SOLVER_FAILURE = 1
EXIT_SOLVER_TIMEOUT = 124
EXIT_SOLVER_RUNTIME_FAILURE = 125
EXIT_HARNESS_FAILURE = 70


def parse_env_assignments(items: list[str]) -> dict[str, str]:
    env: dict[str, str] = {}
    for item in items:
        key, sep, value = item.partition("=")
        if not sep or not key:
            raise ValueError(f"invalid --env assignment: {item!r}")
        env[key] = value
    return env


def apply_solver_env_overrides(base_env: dict[str, str], overrides: dict[str, str]) -> None:
    for key, value in overrides.items():
        if key in RESERVED_SOLVER_ENV_KEYS and base_env.get(key) != value:
            raise ValueError(f"--env {key} cannot override branch-local artifact routing")
        base_env[key] = value


def build_case_solver_env(outdir: Path, mode: str, n: int, seed: int) -> dict[str, str]:
    env = os.environ.copy()
    env["DENSE_PROFILE_OUTDIR"] = str(outdir)
    env["DENSE_SHADOW_CASE_MODE"] = mode
    env["DENSE_SHADOW_CASE_N"] = str(n)
    env["DENSE_SHADOW_CASE_SEED"] = str(seed)
    return env


def resolve_case_outdir(path_like: str | Path | None) -> Path:
    return resolve_output_path(path_like, default_key="branch_run_case")


def default_branch_solver() -> Path:
    # Use the checked-in launcher so ad hoc runs inherit the same artifact-rooted
    # build/profile routing as the gate wrappers instead of looking for ./solve.
    return DEFAULT_BRANCH_SOLVER


def result_path(outdir: Path) -> Path:
    return outdir / RUN_CASE_RESULT_NAME


def normalize_solver_exit_code(rc: int) -> tuple[int, int | None]:
    if rc == 0:
        return 0, None
    if rc > 0:
        return EXIT_SOLVER_RUNTIME_FAILURE, None
    signal_num = -rc
    return 128 + signal_num, signal_num


def write_case_result(path: Path, payload: dict[str, object]) -> None:
    path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def finalize_case_result(
    path: Path,
    *,
    status: str,
    category: str,
    exit_code: int,
    message: str,
    solver_exit_code: int | None = None,
    solver_signal: int | None = None,
    timed_out: bool = False,
    validator_ok: bool | None = None,
    sec: float | None = None,
    rss_kb: int | None = None,
    traceback_text: str | None = None,
) -> int:
    payload: dict[str, object] = {
        "status": status,
        "category": category,
        "exit_code": exit_code,
        "message": message,
        "solver_exit_code": solver_exit_code,
        "solver_signal": solver_signal,
        "timed_out": timed_out,
        "validator_ok": validator_ok,
        "sec": sec,
        "rss_kb": rss_kb,
    }
    if traceback_text:
        payload["traceback"] = traceback_text
    write_case_result(path, payload)
    return exit_code


def generate_case(
    outdir: Path,
    *,
    mode: str,
    n: int,
    seed: int,
    shuffle_labels: int,
    shuffle_queries: int,
    m_cap: int = 100000,
) -> tuple[Path, Path]:
    outdir.mkdir(parents=True, exist_ok=True)
    in_path = outdir / "in.txt"
    meta_path = outdir / "meta.json"
    hidden_parent_path = outdir / "hidden_parent.txt"

    parent, queries, summary = branch_gen_case.build_mode(mode, n, m_cap, seed)
    if shuffle_labels:
        parent, queries = branch_gen_case.permute_preserving_root(
            parent, queries, seed ^ 0x9E3779B1
        )
    if shuffle_queries:
        rng = random.Random(seed ^ 0x85EBCA77)
        rng.shuffle(queries)

    meta_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    branch_gen_case.write_parent_file(str(hidden_parent_path), parent)
    with in_path.open("w", encoding="utf-8") as fout:
        fout.write(f"{n} {len(queries)}\n")
        for u, v, w in queries:
            fout.write(f"{u} {v} {w}\n")

    return in_path, meta_path


def main() -> int:
    ap = argparse.ArgumentParser(description="Branch-local generate, run, and validate one stress case.")
    ap.add_argument("mode")
    ap.add_argument("n", type=int)
    ap.add_argument("seed", nargs="?", type=int, default=1)
    ap.add_argument("shuffle_labels", nargs="?", type=int, default=0)
    ap.add_argument("shuffle_queries", nargs="?", type=int, default=0)
    ap.add_argument("solver", nargs="?", default=str(default_branch_solver()))
    ap.add_argument("outdir", nargs="?", default=None)
    ap.add_argument("--timeout", type=float, default=None)
    ap.add_argument("--env", action="append", default=[], help="repeatable KEY=VALUE assignment passed to the solver")
    args = ap.parse_args()

    try:
        outdir = resolve_case_outdir(args.outdir)
    except ValueError as exc:
        print(f"[run_case] {exc}", file=sys.stderr)
        return EXIT_USAGE_ERROR
    status_path = result_path(outdir)

    def finish(
        *,
        status: str,
        category: str,
        exit_code: int,
        message: str,
        solver_exit_code: int | None = None,
        solver_signal: int | None = None,
        timed_out: bool = False,
        validator_ok: bool | None = None,
        sec: float | None = None,
        rss_kb: int | None = None,
        traceback_text: str | None = None,
    ) -> int:
        try:
            return finalize_case_result(
                status_path,
                status=status,
                category=category,
                exit_code=exit_code,
                message=message,
                solver_exit_code=solver_exit_code,
                solver_signal=solver_signal,
                timed_out=timed_out,
                validator_ok=validator_ok,
                sec=sec,
                rss_kb=rss_kb,
                traceback_text=traceback_text,
            )
        except OSError as exc:
            print(f"[run_case] harness failure: failed to write {status_path}: {exc}", file=sys.stderr)
            return EXIT_HARNESS_FAILURE

    try:
        outdir.mkdir(parents=True, exist_ok=True)
        solver = resolve_solver_path(args.solver, root=ROOT)
        ensure_executable(solver)

        in_path, meta_path = generate_case(
            outdir,
            mode=args.mode,
            n=args.n,
            seed=args.seed,
            shuffle_labels=args.shuffle_labels,
            shuffle_queries=args.shuffle_queries,
        )
        out_path = outdir / "out.txt"
        time_path = outdir / "time.txt"
        sol_stderr = outdir / "solver_stderr.txt"

        solver_env = build_case_solver_env(outdir, args.mode, args.n, args.seed)
        try:
            apply_solver_env_overrides(solver_env, parse_env_assignments(args.env))
        except ValueError as exc:
            print(f"[run_case] {exc}", file=sys.stderr)
            return finish(
                status="harness_usage_failure",
                category="harness",
                exit_code=EXIT_USAGE_ERROR,
                message=str(exc),
            )

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
            return finish(
                status="solver_timeout",
                category="solver",
                exit_code=EXIT_SOLVER_TIMEOUT,
                message=f"solver timed out after {args.timeout}s",
                solver_exit_code=rc_sol,
                timed_out=True,
                validator_ok=False,
            )
        if rc_sol != 0:
            normalized_rc, solver_signal = normalize_solver_exit_code(rc_sol)
            if solver_signal is not None:
                print(
                    f"[run_case] solver terminated by signal {solver_signal} (normalized exit {normalized_rc})",
                    file=sys.stderr,
                )
            else:
                print(
                    f"[run_case] solver exited with code {rc_sol} (normalized exit {normalized_rc})",
                    file=sys.stderr,
                )
            return finish(
                status="solver_runtime_failure",
                category="solver",
                exit_code=normalized_rc,
                message=(
                    f"solver terminated by signal {solver_signal} (normalized exit {normalized_rc})"
                    if solver_signal is not None
                    else f"solver exited with code {rc_sol} (normalized exit {normalized_rc})"
                ),
                solver_exit_code=rc_sol,
                solver_signal=solver_signal,
                validator_ok=False,
                sec=sec,
                rss_kb=rss_kb,
            )

        ok, message = validate_case(in_path, out_path)
        if not ok:
            print(f"[run_case] validator failed: {message}", file=sys.stderr)
            return finish(
                status="solver_acceptance_failure",
                category="solver",
                exit_code=EXIT_SOLVER_FAILURE,
                message=message,
                validator_ok=False,
                sec=sec,
                rss_kb=rss_kb,
            )

        rss_text = "n/a" if rss_kb is None or rss_kb < 0 else f"{rss_kb}KB"
        print(f"[run_case] mode={args.mode} n={args.n} seed={args.seed} time={sec:.6f}s mem={rss_text}")
        print(f"[run_case] artifacts: {in_path} {out_path} {meta_path}")
        return finish(
            status="pass",
            category="pass",
            exit_code=0,
            message="OK",
            validator_ok=True,
            sec=sec,
            rss_kb=rss_kb,
        )
    except OSError as exc:
        print(f"[run_case] harness failure: {exc}", file=sys.stderr)
        return finish(
            status="harness_transient_failure",
            category="harness",
            exit_code=EXIT_HARNESS_FAILURE,
            message=str(exc),
        )
    except Exception as exc:
        tb = traceback.format_exc()
        print(tb, file=sys.stderr, end="")
        print(
            f"[run_case] harness failure: unexpected {type(exc).__name__}: {exc}",
            file=sys.stderr,
        )
        return finish(
            status="harness_transient_failure",
            category="harness",
            exit_code=EXIT_HARNESS_FAILURE,
            message=f"unexpected {type(exc).__name__}: {exc}",
            traceback_text=tb,
        )


if __name__ == "__main__":
    raise SystemExit(main())
