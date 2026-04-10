#!/usr/bin/env python3
from __future__ import annotations

import os
import random
import shutil
import sys
import tempfile
import time
import hashlib
import json
from contextlib import contextmanager
from pathlib import Path

os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
sys.dont_write_bytecode = True

from artifact_paths import (
    artifacts_root,
    configure_branch_process_env,
    ensure_under_artifacts,
    resolve_artifact_path,
    resolve_output_path,
)
import branch_gen_case_local as branch_gen_case
from branch_validator_local import validate_case
import branch_outer_certify as outer_certify
import suite_utils as branch_suite_utils


def _debug_log(message: str) -> None:
    if os.environ.get("BRANCH_CERTIFY_DEBUG_TRACE") == "1":
        print(f"[branch_certify_suite] {message}", file=sys.stderr, flush=True)


configure_branch_process_env()
_debug_log("module init: branch process environment configured")


BRANCH_ROOT = Path(__file__).resolve().parent
branch_run_solver_with_time = branch_suite_utils.run_solver_with_time
_debug_log("module init: suite_utils and outer certify imported")

BRANCH_GEN = BRANCH_ROOT / "branch_gen_case_local.py"
GEN_FILES = ("in.txt", "meta.json", "hidden_parent.txt", "gen_stderr.txt")
LCA_TREE_STRESS_ARTIFACTS_ROOT = ensure_under_artifacts(artifacts_root() / "lca_tree_stress_v5")
GENERATOR_SIGNATURE_NAME = ".generator_signature.json"
GENERATOR_SIGNATURE_SCHEMA = "branch_case_generator_signature_v1"
GENERATOR_SIGNATURE_FILES = (BRANCH_GEN, *sorted((BRANCH_ROOT / "branch_generators_local").rglob("*.py")))
_GENERATOR_SIGNATURE_CACHE: dict[str, object] | None = None
_debug_log(f"module init: generator signature file count={len(GENERATOR_SIGNATURE_FILES)}")


def _ensure_under_lca_tree_artifacts(path: str | Path) -> Path:
    resolved = ensure_under_artifacts(path)
    try:
        resolved.relative_to(LCA_TREE_STRESS_ARTIFACTS_ROOT)
    except ValueError as exc:
        raise ValueError(
            f"certify helper path must stay under {LCA_TREE_STRESS_ARTIFACTS_ROOT}: {resolved}"
        ) from exc
    return resolved


BRANCH_CERTIFY_CASE_RUN_TMP_ROOT_ENV = "BRANCH_CERTIFY_CASE_RUN_TMP_ROOT"
BRANCH_CERTIFY_CASE_CACHE_ROOT_ENV = "BRANCH_CERTIFY_CASE_CACHE_ROOT"
BRANCH_CERTIFY_CASE_CACHE_TMP_ROOT_ENV = "BRANCH_CERTIFY_CASE_CACHE_TMP_ROOT"
DEFAULT_CASE_RUN_TMP_ROOT = _ensure_under_lca_tree_artifacts(
    LCA_TREE_STRESS_ARTIFACTS_ROOT / ".tmp" / "case_runs"
)
DEFAULT_CASE_CACHE_ROOT = _ensure_under_lca_tree_artifacts(
    LCA_TREE_STRESS_ARTIFACTS_ROOT / ".tmp" / "case_cache"
)
DEFAULT_CASE_CACHE_TMP_ROOT = _ensure_under_lca_tree_artifacts(
    LCA_TREE_STRESS_ARTIFACTS_ROOT / ".tmp" / "case_cache_tmp"
)
SOLVER_ENV_SNAPSHOT_NAME = "solver_env_snapshot.json"
RUN_CASE_RESULT_NAME = "run_case_result.json"


def _resolve_case_temp_root(default_path: Path, env_name: str) -> Path:
    return _ensure_under_lca_tree_artifacts(
        resolve_artifact_path(os.environ.get(env_name), default_path=default_path)
    )


def _normalize_artifact_out_dir(path_like: str | Path | None) -> Path:
    return _ensure_under_lca_tree_artifacts(resolve_output_path(path_like, default_key="branch_certify_suite"))


def _normalize_cli_out_arg(argv: list[str]) -> list[str]:
    normalized = list(argv)
    for idx, token in enumerate(normalized[1:], start=1):
        if token == "--out":
            if idx + 1 >= len(normalized):
                raise ValueError("--out requires a value")
            normalized[idx + 1] = str(_normalize_artifact_out_dir(normalized[idx + 1]))
            return normalized
        if token.startswith("--out="):
            raw = token.split("=", 1)[1]
            normalized[idx] = f"--out={_normalize_artifact_out_dir(raw)}"
            return normalized
    normalized.extend(["--out", str(_normalize_artifact_out_dir(None))])
    return normalized


def _generation_ready(in_path: Path, meta_path: Path, hidden_parent_path: Path) -> bool:
    paths = (in_path, meta_path, hidden_parent_path)
    return all(path.exists() and path.stat().st_size > 0 for path in paths)


def _cache_root(out_dir: Path) -> Path:
    return _resolve_case_temp_root(DEFAULT_CASE_CACHE_ROOT, BRANCH_CERTIFY_CASE_CACHE_ROOT_ENV)


def _cache_tmp_root(out_dir: Path) -> Path:
    return _resolve_case_temp_root(DEFAULT_CASE_CACHE_TMP_ROOT, BRANCH_CERTIFY_CASE_CACHE_TMP_ROOT_ENV)


def _case_run_tmp_root() -> Path:
    return _resolve_case_temp_root(DEFAULT_CASE_RUN_TMP_ROOT, BRANCH_CERTIFY_CASE_RUN_TMP_ROOT_ENV)


def _cache_dir(out_dir: Path, mode: str, n: int, seed: int,
               shuffle_labels: int, shuffle_queries: int) -> Path:
    return _ensure_under_lca_tree_artifacts(
        _cache_root(out_dir) / mode / f"n{n}" / f"seed{seed}_L{shuffle_labels}_Q{shuffle_queries}"
    )


def _report_out_dir(out_dir: Path) -> Path:
    raw = os.environ.get("BRANCH_CERTIFY_REPORT_OUTDIR")
    if not raw:
        return _normalize_artifact_out_dir(out_dir)
    return _normalize_artifact_out_dir(raw)


def _cache_ready(cache_dir: Path) -> bool:
    base_ready = all(
        (cache_dir / name).exists() and (cache_dir / name).stat().st_size > 0
        for name in GEN_FILES
        if name != "gen_stderr.txt"
    ) and (cache_dir / "gen_stderr.txt").exists()
    return base_ready and _cache_signature_matches(cache_dir)


def _generator_signature_payload() -> dict[str, object]:
    global _GENERATOR_SIGNATURE_CACHE
    if _GENERATOR_SIGNATURE_CACHE is not None:
        return _GENERATOR_SIGNATURE_CACHE

    digest = hashlib.sha256()
    files_payload: list[dict[str, str]] = []
    for path in GENERATOR_SIGNATURE_FILES:
        rel = path.relative_to(BRANCH_ROOT).as_posix()
        content = path.read_bytes()
        file_sha = hashlib.sha256(content).hexdigest()
        digest.update(rel.encode("utf-8"))
        digest.update(b"\0")
        digest.update(file_sha.encode("ascii"))
        digest.update(b"\0")
        files_payload.append({"path": rel, "sha256": file_sha})

    _GENERATOR_SIGNATURE_CACHE = {
        "schema": GENERATOR_SIGNATURE_SCHEMA,
        "generator_digest": digest.hexdigest(),
        "files": files_payload,
    }
    return _GENERATOR_SIGNATURE_CACHE


def _cache_signature_path(cache_dir: Path) -> Path:
    cache_dir = _ensure_under_lca_tree_artifacts(cache_dir)
    return cache_dir / GENERATOR_SIGNATURE_NAME


def _cache_signature_matches(cache_dir: Path) -> bool:
    signature_path = _cache_signature_path(cache_dir)
    try:
        cached_payload = json.loads(signature_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return False
    return cached_payload == _generator_signature_payload()


def _write_cache_signature(cache_dir: Path) -> None:
    signature_path = _cache_signature_path(cache_dir)
    signature_path.write_text(
        json.dumps(_generator_signature_payload(), ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def _remove_path(path: Path) -> None:
    if not path.exists():
        return
    if path.is_dir():
        shutil.rmtree(path, ignore_errors=True)
    else:
        try:
            path.unlink()
        except FileNotFoundError:
            pass


def _invalidate_cache_dir(cache_dir: Path) -> None:
    _remove_path(cache_dir)


@contextmanager
def _cache_lock(cache_dir: Path):
    lock_dir = cache_dir.parent / f".{cache_dir.name}.lock"
    pid_path = lock_dir / "pid"

    while True:
        lock_dir.parent.mkdir(parents=True, exist_ok=True)
        try:
            lock_dir.mkdir(parents=False)
            pid_path.write_text(f"{os.getpid()}\n", encoding="utf-8")
            break
        except FileExistsError:
            holder_pid = None
            try:
                holder_pid = int(pid_path.read_text(encoding="utf-8").strip())
            except (FileNotFoundError, ValueError, OSError):
                holder_pid = None

            stale = False
            if holder_pid is None:
                stale = True
            else:
                try:
                    os.kill(holder_pid, 0)
                except OSError:
                    stale = True

            if stale:
                _remove_path(lock_dir)
                continue

            time.sleep(0.05)
        except FileNotFoundError:
            continue

    try:
        yield
    finally:
        _remove_path(lock_dir)


def _copy_generated_case(src_dir: Path, dst_dir: Path) -> bool:
    src_dir = _ensure_under_lca_tree_artifacts(src_dir)
    dst_dir = _ensure_under_lca_tree_artifacts(dst_dir)
    try:
        if not all((src_dir / name).exists() for name in GEN_FILES):
            return False
    except OSError:
        return False

    dst_dir.parent.mkdir(parents=True, exist_ok=True)
    try:
        dst_dir.mkdir(parents=True, exist_ok=True)
        for name in GEN_FILES:
            src = src_dir / name
            shutil.copyfile(src, dst_dir / name)
    except OSError:
        return False
    return True


def _publish_case_dir(src_dir: Path, dst_dir: Path) -> None:
    src_dir = _ensure_under_lca_tree_artifacts(src_dir)
    dst_dir = _ensure_under_lca_tree_artifacts(dst_dir)
    dst_dir.parent.mkdir(parents=True, exist_ok=True)
    _remove_path(dst_dir)
    try:
        src_dir.replace(dst_dir)
        return
    except OSError:
        # APFS/iCloud-backed temp directories have occasionally raised publish
        # rename errors even though the staged case payload still exists and can
        # be copied safely. Keep rename as the fast path, but fall back to a
        # copy publish instead of aborting the whole certify pass.
        if not src_dir.exists():
            raise

    shutil.copytree(src_dir, dst_dir, dirs_exist_ok=True)
    shutil.rmtree(src_dir, ignore_errors=True)


def _promote_generated_case(src_dir: Path, dst_dir: Path) -> bool:
    dst_dir.parent.mkdir(parents=True, exist_ok=True)
    if dst_dir.exists() and not _cache_ready(dst_dir):
        _invalidate_cache_dir(dst_dir)

    if not dst_dir.exists():
        try:
            src_dir.rename(dst_dir)
            _write_cache_signature(dst_dir)
            return True
        except OSError:
            # Another writer may have materialized the cache entry between the
            # existence check and rename; accept it if it is complete.
            if _cache_ready(dst_dir):
                return True

    if _cache_ready(dst_dir):
        return True

    _invalidate_cache_dir(dst_dir)
    copied = _copy_generated_case(src_dir, dst_dir)
    if not copied:
        return False
    _write_cache_signature(dst_dir)
    return _cache_ready(dst_dir)


def _ensure_generated_case(out_dir: Path, case_dir: Path, mode: str, n: int, seed: int,
                           shuffle_labels: int, shuffle_queries: int) -> bool:
    in_path = case_dir / "in.txt"
    meta_path = case_dir / "meta.json"
    hidden_parent_path = case_dir / "hidden_parent.txt"

    cache_dir = _cache_dir(out_dir, mode, n, seed, shuffle_labels, shuffle_queries)
    _debug_log(
        f"ensure_generated_case: mode={mode} n={n} seed={seed} L={shuffle_labels} Q={shuffle_queries} cache_dir={cache_dir}"
    )
    with _cache_lock(cache_dir):
        copied_from_cache = False
        if _cache_ready(cache_dir):
            copied_from_cache = _copy_generated_case(cache_dir, case_dir)
            if copied_from_cache and _generation_ready(in_path, meta_path, hidden_parent_path):
                return True
            _invalidate_cache_dir(cache_dir)

        if not copied_from_cache:
            cache_tmp_root = _cache_tmp_root(out_dir)
            cache_tmp_root.mkdir(parents=True, exist_ok=True)
            temp_dir = _ensure_under_lca_tree_artifacts(
                Path(tempfile.mkdtemp(prefix="gen_case.", dir=cache_tmp_root))
            )
            try:
                _debug_log(f"ensure_generated_case: generating cache payload into {temp_dir}")
                if not _generate_case_in_process(
                    temp_dir / "in.txt",
                    temp_dir / "meta.json",
                    temp_dir / "hidden_parent.txt",
                    temp_dir / "gen_stderr.txt",
                    mode=mode,
                    n=n,
                    seed=seed,
                    shuffle_labels=shuffle_labels,
                    shuffle_queries=shuffle_queries,
                ) or not _generation_ready(
                    temp_dir / "in.txt", temp_dir / "meta.json", temp_dir / "hidden_parent.txt"
                ):
                    return False

                if _promote_generated_case(temp_dir, cache_dir):
                    temp_dir = None
                else:
                    return False
            finally:
                if temp_dir is not None and temp_dir.exists():
                    shutil.rmtree(temp_dir, ignore_errors=True)

        if not _cache_ready(cache_dir):
            return False

    return _copy_generated_case(cache_dir, case_dir) and _generation_ready(in_path, meta_path, hidden_parent_path)


def _generate_case_in_process(
    in_path: Path,
    meta_path: Path,
    hidden_parent_path: Path,
    gen_stderr_path: Path,
    *,
    mode: str,
    n: int,
    seed: int,
    shuffle_labels: int,
    shuffle_queries: int,
) -> bool:
    try:
        _debug_log(
            f"generate_case_in_process: mode={mode} n={n} seed={seed} L={shuffle_labels} Q={shuffle_queries}"
        )
        parent, queries, summary = branch_gen_case.build_mode(mode, n, 100000, seed)

        if shuffle_labels:
            parent, queries = branch_gen_case.permute_preserving_root(parent, queries, seed ^ 0x9E3779B1)

        if shuffle_queries:
            rng = random.Random(seed ^ 0x85EBCA77)
            rng.shuffle(queries)

        with in_path.open("w", encoding="utf-8") as f:
            f.write(f"{n} {len(queries)}\n")
            for u, v, w in queries:
                f.write(f"{u} {v} {w}\n")

        meta_path.write_text(
            json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
        branch_gen_case.write_parent_file(str(hidden_parent_path), parent)
        gen_stderr_path.write_text("", encoding="utf-8")
        return True
    except Exception as exc:
        gen_stderr_path.write_text(f"[branch_certify_suite] generator failed: {exc}\n", encoding="utf-8")
        return False


def _clear_case_outputs(case_dir: Path) -> None:
    for name in (
        "out.txt",
        "time.txt",
        "solver_stderr.txt",
        "val_stderr.txt",
        SOLVER_ENV_SNAPSHOT_NAME,
        RUN_CASE_RESULT_NAME,
    ):
        path = case_dir / name
        if path.exists():
            path.unlink()


def _solver_file_fingerprint(solver: Path) -> dict[str, object]:
    try:
        data = solver.read_bytes()
    except OSError:
        return {
            "path": str(solver),
            "exists": False,
        }
    stat = solver.stat()
    return {
        "path": str(solver),
        "exists": True,
        "size_bytes": stat.st_size,
        "mtime_ns": stat.st_mtime_ns,
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def _build_solver_env_snapshot(solver: Path, env: dict[str, str]) -> dict[str, object]:
    tracked = {}
    for key in sorted(env):
        if key.startswith("ENABLE_") or key.startswith("PROFILE_") or key.startswith("DENSE_") or key == "RUN_TAG":
            tracked[key] = env[key]
    return {
        "schema": "branch_certify_suite_solver_env_snapshot_v1",
        "solver": _solver_file_fingerprint(solver),
        "tracked_env": tracked,
    }


def _write_solver_env_snapshot(case_dir: Path, solver: Path, env: dict[str, str]) -> None:
    case_dir = _ensure_under_lca_tree_artifacts(case_dir)
    (case_dir / SOLVER_ENV_SNAPSHOT_NAME).write_text(
        json.dumps(_build_solver_env_snapshot(solver, env), ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def _write_case_result(
    case_dir: Path,
    *,
    status: str,
    category: str,
    exit_code: int,
    message: str,
    solver_exit_code: int | None = None,
    timed_out: bool = False,
    validator_ok: bool | None = None,
    sec: float | None = None,
    rss_kb: int | None = None,
) -> None:
    case_dir = _ensure_under_lca_tree_artifacts(case_dir)
    payload = {
        "status": status,
        "category": category,
        "exit_code": exit_code,
        "message": message,
        "solver_exit_code": solver_exit_code,
        "timed_out": timed_out,
        "validator_ok": validator_ok,
        "sec": sec,
        "rss_kb": rss_kb,
    }
    (case_dir / RUN_CASE_RESULT_NAME).write_text(
        json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def run_one_case(solver: Path, out_dir: Path, stage_name: str, mode: str, n: int, seed: int,
                 shuffle_labels: int, shuffle_queries: int, timeout: float | None) -> outer_certify.Row:
    out_dir = _normalize_artifact_out_dir(out_dir)
    case_dir = _ensure_under_lca_tree_artifacts(
        out_dir / "runs" / stage_name / mode / f"n{n}" / f"seed{seed}_L{shuffle_labels}_Q{shuffle_queries}"
    )
    reported_case_dir = (
        _report_out_dir(out_dir)
        / "runs"
        / stage_name
        / mode
        / f"n{n}"
        / f"seed{seed}_L{shuffle_labels}_Q{shuffle_queries}"
    )
    reported_case_dir = _ensure_under_lca_tree_artifacts(reported_case_dir)
    # Write directly into the published case directory so strong-gate reruns do
    # not depend on renaming ephemeral per-case temp roots after validation.
    case_dir.mkdir(parents=True, exist_ok=True)
    work_dir = case_dir
    in_path = work_dir / "in.txt"
    out_path = work_dir / "out.txt"
    meta_path = work_dir / "meta.json"
    hidden_parent_path = work_dir / "hidden_parent.txt"
    time_path = work_dir / "time.txt"
    solver_stderr = work_dir / "solver_stderr.txt"
    gen_stderr = work_dir / "gen_stderr.txt"
    val_stderr = work_dir / "val_stderr.txt"

    _clear_case_outputs(work_dir)
    gen_ok = _ensure_generated_case(out_dir, work_dir, mode, n, seed, shuffle_labels, shuffle_queries)

    if not gen_ok:
        _write_case_result(
            work_dir,
            status="generation_failure",
            category="generator",
            exit_code=127,
            message="failed to generate case payload",
            validator_ok=False,
        )
        return outer_certify.Row(stage_name, mode, n, seed, shuffle_labels, shuffle_queries,
                                 0, 127, 0, 0, None, None, str(reported_case_dir))

    _clear_case_outputs(work_dir)

    solver_env = outer_certify.build_case_solver_env(work_dir, mode, n, seed)
    solver_env["DENSE_SHADOW_CASE_SHUFFLE_LABELS"] = str(shuffle_labels)
    solver_env["DENSE_SHADOW_CASE_SHUFFLE_QUERIES"] = str(shuffle_queries)
    _write_solver_env_snapshot(work_dir, solver, solver_env)
    rc_sol, to_sol, sec, rss = branch_run_solver_with_time(
        solver,
        in_path,
        out_path,
        time_path,
        solver_stderr,
        timeout,
        env=solver_env,
        cwd=work_dir,
    )

    val_ok = 0
    if rc_sol == 0 and not to_sol and out_path.exists():
        ok, message = validate_case(in_path, out_path)
        val_ok = 1 if ok else 0
        val_stderr.write_text("" if ok else f"{message}\n", encoding="utf-8")
    if to_sol:
        _write_case_result(
            work_dir,
            status="solver_timeout",
            category="solver",
            exit_code=124,
            message="solver timed out",
            solver_exit_code=rc_sol,
            timed_out=True,
            validator_ok=False,
            sec=sec,
            rss_kb=rss,
        )
    elif rc_sol != 0:
        _write_case_result(
            work_dir,
            status="solver_runtime_failure",
            category="solver",
            exit_code=rc_sol,
            message="solver exited with a non-zero status",
            solver_exit_code=rc_sol,
            validator_ok=False,
            sec=sec,
            rss_kb=rss,
        )
    elif val_ok != 1:
        _write_case_result(
            work_dir,
            status="validator_failure",
            category="validator",
            exit_code=1,
            message="validator rejected solver output",
            solver_exit_code=rc_sol,
            validator_ok=False,
            sec=sec,
            rss_kb=rss,
        )
    else:
        _write_case_result(
            work_dir,
            status="pass",
            category="pass",
            exit_code=0,
            message="case completed successfully",
            solver_exit_code=rc_sol,
            validator_ok=True,
            sec=sec,
            rss_kb=rss,
        )

    return outer_certify.Row(stage_name, mode, n, seed, shuffle_labels, shuffle_queries,
                             1, rc_sol, 1 if to_sol else 0, val_ok, sec, rss, str(reported_case_dir))


def main() -> int:
    try:
        _debug_log("main: start")
        sys.argv = _normalize_cli_out_arg(sys.argv)
        raw_report_outdir = os.environ.get("BRANCH_CERTIFY_REPORT_OUTDIR")
        if raw_report_outdir:
            os.environ["BRANCH_CERTIFY_REPORT_OUTDIR"] = str(_normalize_artifact_out_dir(raw_report_outdir))
        _debug_log("main: cli/output normalization complete")
    except ValueError as exc:
        print(f"[branch_certify_suite] {exc}", file=sys.stderr)
        return 2
    outer_certify.run_one_case = run_one_case
    _debug_log("main: dispatching to outer_certify.main()")
    return outer_certify.main()


if __name__ == "__main__":
    raise SystemExit(main())
