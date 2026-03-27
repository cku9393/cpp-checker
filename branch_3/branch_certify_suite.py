#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import sys
import tempfile
import time
import importlib.util
from contextlib import contextmanager
from pathlib import Path

os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
sys.dont_write_bytecode = True

from artifact_paths import (
    artifacts_root,
    configure_branch_process_env,
    ensure_under_artifacts,
    resolve_output_path,
)
from branch_validator import validate_case


configure_branch_process_env()


BRANCH_ROOT = Path(__file__).resolve().parent
OUTER_ROOT = BRANCH_ROOT.parent
TOOLING_ROOT = OUTER_ROOT / "lca_tree_stress_v5" / "tooling"

_branch_suite_utils_spec = importlib.util.spec_from_file_location(
    "branch_suite_utils", BRANCH_ROOT / "suite_utils.py"
)
assert _branch_suite_utils_spec is not None and _branch_suite_utils_spec.loader is not None
branch_suite_utils = importlib.util.module_from_spec(_branch_suite_utils_spec)
sys.modules["branch_suite_utils"] = branch_suite_utils
_branch_suite_utils_spec.loader.exec_module(branch_suite_utils)
branch_run_solver_with_time = branch_suite_utils.run_solver_with_time

# Keep the branch-local helper implementations authoritative when loading the
# outer certify logic. The outer suite_utils copy can be unreadable in this
# workspace, while the branch-local copy exposes the same interface.
sys.modules["suite_utils"] = branch_suite_utils
_outer_certify_spec = importlib.util.spec_from_file_location(
    "outer_certify_suite", TOOLING_ROOT / "certify_suite.py"
)
assert _outer_certify_spec is not None and _outer_certify_spec.loader is not None
outer_certify = importlib.util.module_from_spec(_outer_certify_spec)
sys.modules["outer_certify_suite"] = outer_certify
_outer_certify_spec.loader.exec_module(outer_certify)

BRANCH_GEN = BRANCH_ROOT / "branch_gen_case.py"
GEN_FILES = ("in.txt", "meta.json", "hidden_parent.txt", "gen_stderr.txt")
LCA_TREE_STRESS_ARTIFACTS_ROOT = ensure_under_artifacts(artifacts_root() / "lca_tree_stress_v5")


def _ensure_under_lca_tree_artifacts(path: str | Path) -> Path:
    resolved = ensure_under_artifacts(path)
    try:
        resolved.relative_to(LCA_TREE_STRESS_ARTIFACTS_ROOT)
    except ValueError as exc:
        raise ValueError(
            f"certify helper path must stay under {LCA_TREE_STRESS_ARTIFACTS_ROOT}: {resolved}"
        ) from exc
    return resolved


CASE_RUN_TMP_ROOT = _ensure_under_lca_tree_artifacts(LCA_TREE_STRESS_ARTIFACTS_ROOT / ".tmp" / "case_runs")
CASE_CACHE_ROOT = _ensure_under_lca_tree_artifacts(LCA_TREE_STRESS_ARTIFACTS_ROOT / ".tmp" / "case_cache")
CASE_CACHE_TMP_ROOT = _ensure_under_lca_tree_artifacts(LCA_TREE_STRESS_ARTIFACTS_ROOT / ".tmp" / "case_cache_tmp")


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
    return CASE_CACHE_ROOT


def _cache_tmp_root(out_dir: Path) -> Path:
    return CASE_CACHE_TMP_ROOT


def _case_run_tmp_root() -> Path:
    return CASE_RUN_TMP_ROOT


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
    return all((cache_dir / name).exists() and (cache_dir / name).stat().st_size > 0
               for name in GEN_FILES if name != "gen_stderr.txt") and (cache_dir / "gen_stderr.txt").exists()


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
    dst_dir.parent.mkdir(parents=True, exist_ok=True)
    _remove_path(dst_dir)
    src_dir.replace(dst_dir)


def _promote_generated_case(src_dir: Path, dst_dir: Path) -> bool:
    dst_dir.parent.mkdir(parents=True, exist_ok=True)
    if dst_dir.exists() and not _cache_ready(dst_dir):
        _invalidate_cache_dir(dst_dir)

    if not dst_dir.exists():
        try:
            src_dir.rename(dst_dir)
            return True
        except OSError:
            # Another writer may have materialized the cache entry between the
            # existence check and rename; accept it if it is complete.
            if _cache_ready(dst_dir):
                return True

    if _cache_ready(dst_dir):
        return True

    _invalidate_cache_dir(dst_dir)
    return _copy_generated_case(src_dir, dst_dir) and _cache_ready(dst_dir)


def _ensure_generated_case(out_dir: Path, case_dir: Path, mode: str, n: int, seed: int,
                           shuffle_labels: int, shuffle_queries: int) -> bool:
    in_path = case_dir / "in.txt"
    meta_path = case_dir / "meta.json"
    hidden_parent_path = case_dir / "hidden_parent.txt"

    cache_dir = _cache_dir(out_dir, mode, n, seed, shuffle_labels, shuffle_queries)
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
                gen_cmd = [
                    sys.executable,
                    str(BRANCH_GEN),
                    "--mode",
                    mode,
                    "--n",
                    str(n),
                    "--seed",
                    str(seed),
                    "--meta",
                    str(temp_dir / "meta.json"),
                    "--parent-out",
                    str(temp_dir / "hidden_parent.txt"),
                ]
                if shuffle_labels:
                    gen_cmd.append("--shuffle-labels")
                if shuffle_queries:
                    gen_cmd.append("--shuffle-queries")

                rc_gen, to_gen, _ = outer_certify.run_cmd(
                    gen_cmd,
                    stdout_path=temp_dir / "in.txt",
                    stderr_path=temp_dir / "gen_stderr.txt",
                    timeout=None,
                )
                if rc_gen != 0 or to_gen or not _generation_ready(
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


def _clear_case_outputs(case_dir: Path) -> None:
    for name in ("out.txt", "time.txt", "solver_stderr.txt", "val_stderr.txt"):
        path = case_dir / name
        if path.exists():
            path.unlink()


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
    case_run_tmp_root = _case_run_tmp_root()
    case_run_tmp_root.mkdir(parents=True, exist_ok=True)
    work_dir = _ensure_under_lca_tree_artifacts(
        Path(tempfile.mkdtemp(prefix=f"{case_dir.name}.run.", dir=case_run_tmp_root))
    )
    published = False
    try:
        in_path = work_dir / "in.txt"
        out_path = work_dir / "out.txt"
        meta_path = work_dir / "meta.json"
        hidden_parent_path = work_dir / "hidden_parent.txt"
        time_path = work_dir / "time.txt"
        solver_stderr = work_dir / "solver_stderr.txt"
        gen_stderr = work_dir / "gen_stderr.txt"
        val_stderr = work_dir / "val_stderr.txt"

        gen_ok = _ensure_generated_case(out_dir, work_dir, mode, n, seed, shuffle_labels, shuffle_queries)

        if not gen_ok:
            _publish_case_dir(work_dir, case_dir)
            published = True
            return outer_certify.Row(stage_name, mode, n, seed, shuffle_labels, shuffle_queries,
                                     0, 127, 0, 0, None, None, str(reported_case_dir))

        _clear_case_outputs(work_dir)

        solver_env = outer_certify.build_case_solver_env(work_dir, mode, n, seed)
        solver_env["DENSE_SHADOW_CASE_SHUFFLE_LABELS"] = str(shuffle_labels)
        solver_env["DENSE_SHADOW_CASE_SHUFFLE_QUERIES"] = str(shuffle_queries)
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

        _publish_case_dir(work_dir, case_dir)
        published = True
        return outer_certify.Row(stage_name, mode, n, seed, shuffle_labels, shuffle_queries,
                                 1, rc_sol, 1 if to_sol else 0, val_ok, sec, rss, str(reported_case_dir))
    finally:
        if not published and work_dir.exists():
            shutil.rmtree(work_dir, ignore_errors=True)


def main() -> int:
    try:
        sys.argv = _normalize_cli_out_arg(sys.argv)
        raw_report_outdir = os.environ.get("BRANCH_CERTIFY_REPORT_OUTDIR")
        if raw_report_outdir:
            os.environ["BRANCH_CERTIFY_REPORT_OUTDIR"] = str(_normalize_artifact_out_dir(raw_report_outdir))
    except ValueError as exc:
        print(f"[branch_certify_suite] {exc}", file=sys.stderr)
        return 2
    outer_certify.run_one_case = run_one_case
    return outer_certify.main()


if __name__ == "__main__":
    raise SystemExit(main())
