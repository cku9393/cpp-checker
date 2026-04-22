#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SNAPSHOT_ROOT_FILES = ("CMakeLists.txt", "README.md")
SNAPSHOT_GLOB_PATTERNS = (
    "include/**/*",
    "src/**/*",
    "tests/*",
    "tests/campaigns/**/*",
    "tests/tools/*.py",
    "tests/regressions/*.txt",
)
VERIFICATION_RELEVANT_FILES = (
    "CMakeLists.txt",
    "src/raw_planner.cpp",
    "src/CMakeLists.txt",
    "tests/CMakeLists.txt",
    "tests/raw_engine_cases.cpp",
    "tests/tools/run_ctest_staged.py",
)
PASS_THROUGH_NAMES = ("counterexamples",)
PASS_THROUGH_GLOBS: tuple[str, ...] = ()
PASS_THROUGH_RELATIVE_PATHS: tuple[str, ...] = ()
STAGED_ARTIFACT_SEED_RELATIVE_PATHS = (
    "phase22_campaigns_compare/split_tie_organic/logs/phase22_split_tie_organic_aggregate.summary.txt",
    "phase21_campaigns_compare/split_tie_organic/logs/phase21_split_tie_organic_aggregate.summary.txt",
    "phase19_campaigns_compare/split_tie_organic/logs/phase19_split_tie_organic_aggregate.summary.txt",
    "phase18_campaigns_compare/split_tie_organic/logs/phase18_split_tie_organic_aggregate.summary.txt",
    "phase17_campaigns_compare/split_tie_organic/logs/phase17_split_tie_organic_aggregate.summary.txt",
    "phase22_campaigns_compare/automorphism_probe/logs/phase22_automorphism_probe_aggregate.summary.txt",
    "phase21_campaigns_compare/automorphism_probe/logs/phase21_automorphism_probe_aggregate.summary.txt",
    "phase19_campaigns_compare/automorphism_probe/logs/phase19_automorphism_probe_aggregate.summary.txt",
    "phase18_campaigns_compare/automorphism_probe/logs/phase18_automorphism_probe_aggregate.summary.txt",
    "phase17_campaigns_compare/automorphism_probe/logs/phase17_automorphism_probe_aggregate.summary.txt",
    "phase22_campaigns_compare/planner_tie_mixed_organic_gap_audit/logs/phase22_planner_tie_mixed_organic_gap_audit_aggregate.summary.txt",
    "phase19_campaigns_compare/planner_tie_mixed_organic_gap_audit/logs/phase19_planner_tie_mixed_organic_gap_audit_aggregate.summary.txt",
    "phase18_campaigns_compare/planner_tie_mixed_organic_gap_audit/logs/phase18_planner_tie_mixed_organic_gap_audit_aggregate.summary.txt",
    "phase17_campaigns_compare/planner_tie_mixed_organic_gap_audit/logs/phase17_planner_tie_mixed_organic_gap_audit_aggregate.summary.txt",
    "phase22_campaigns_compare/planner_tie_mixed_organic_compare_ready/logs/phase22_planner_tie_mixed_organic_compare_ready_aggregate.summary.txt",
    "phase19_campaigns_compare/planner_tie_mixed_organic_compare_ready/logs/phase19_planner_tie_mixed_organic_compare_ready_aggregate.summary.txt",
    "phase18_campaigns_compare/planner_tie_mixed_organic_compare_ready/logs/phase18_planner_tie_mixed_organic_compare_ready_aggregate.summary.txt",
    "phase17_campaigns_compare/planner_tie_mixed_organic_compare_ready/logs/phase17_planner_tie_mixed_organic_compare_ready_aggregate.summary.txt",
    "phase22_applicability/logs/planner_tie_mixed_organic_applicability_audit.summary.txt",
    "phase21_applicability/logs/planner_tie_mixed_organic_applicability_audit.summary.txt",
    "phase20_applicability/logs/planner_tie_mixed_organic_applicability_audit.summary.txt",
    "phase19_applicability/logs/planner_tie_mixed_organic_applicability_audit.summary.txt",
    "phase18_final_applicability/logs/planner_tie_mixed_organic_applicability_audit.summary.txt",
    "phase17_final_applicability/logs/planner_tie_mixed_organic_applicability_audit.summary.txt",
    "phase22_lineage/logs/compare_ready_lineage_audit.summary.txt",
    "phase21_lineage/logs/compare_ready_lineage_audit.summary.txt",
    "phase20_lineage/logs/compare_ready_lineage_audit.summary.txt",
    "phase19_lineage/logs/compare_ready_lineage_audit.summary.txt",
    "phase18_final_lineage/logs/compare_ready_lineage_audit.summary.txt",
    "phase17_final_lineage/logs/compare_ready_lineage_audit.summary.txt",
)
WORKTREE_PRIORITY_FILES = {
    "README.md",
    "tests/CMakeLists.txt",
    "tests/raw_engine_cases.cpp",
    "tests/test_harness.hpp",
    "tests/tools/build_evidence_bundle.py",
    "tests/tools/create_source_snapshot.py",
    "tests/tools/materialize_staged_mirror.py",
    "tests/tools/run_ctest_staged.py",
    "tests/tools/run_policy_pipeline.py",
    "tests/tools/runtime_gate_lib.py",
    "tests/tools/runtime_watch_ops.py",
    "tests/tools/staged_verification_lib.py",
    "tests/tools/verification_closeout.py",
    "tests/tools/verify_staged_mirror.py",
}
WORKTREE_PRIORITY_PREFIXES: tuple[str, ...] = ()
EXPECTED_PHASE35_TESTS = (
    "raw_engine_runtime_registry_health_smoke",
    "raw_engine_runtime_known_env_stale_smoke",
    "raw_engine_runtime_known_env_reverify_required_smoke",
    "raw_engine_runtime_known_env_retire_candidate_smoke",
    "raw_engine_runtime_known_env_state_machine_smoke",
    "raw_engine_runtime_import_known_env_evidence_smoke",
    "raw_engine_runtime_known_env_age_tick_smoke",
    "raw_engine_runtime_known_env_plan_reverify_smoke",
    "raw_engine_runtime_known_env_reverify_gate_smoke",
    "raw_engine_runtime_known_env_apply_reverify_smoke",
    "raw_engine_runtime_known_env_plan_retire_smoke",
    "raw_engine_runtime_known_env_apply_retire_smoke",
    "raw_engine_runtime_known_env_retired_smoke",
    "raw_engine_runtime_registry_health_v2_smoke",
    "raw_engine_runtime_registry_health_v3_smoke",
    "raw_engine_runtime_registry_same_fingerprint_supersession_smoke",
    "raw_engine_runtime_registry_retired_not_selected_smoke",
    "raw_engine_runtime_foreign_env_state_machine_smoke",
    "raw_engine_publication_health_smoke",
    "raw_engine_policy_ops_summary_v4_smoke",
    "raw_engine_policy_ops_summary_v5_smoke",
    "raw_engine_policy_ops_summary_v6_smoke",
    "raw_engine_policy_ops_summary_known_vs_foreign_smoke",
    "raw_engine_policy_ops_summary_publication_health_smoke",
    "raw_engine_evidence_bundle_registry_health_smoke",
    "raw_engine_evidence_bundle_known_env_governance_smoke",
    "raw_engine_evidence_bundle_known_env_timeline_smoke",
    "raw_engine_evidence_bundle_publication_health_smoke",
    "raw_engine_light_ops_bundle_operator_summary_v4_smoke",
)
WRAPPER_TAIL_TESTS = (
    "raw_engine_compare_campaign_checkpoint_smoke",
    "raw_engine_campaign_resume_smoke",
    "raw_engine_campaign_partial_merge_smoke",
)


def timestamp_utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def atomic_write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = path.with_name(f".{path.name}.tmp.{os.getpid()}.{time.time_ns()}")
    temp_path.write_text(text, encoding="utf-8")
    os.replace(temp_path, path)


def write_json(path: Path, payload: dict[str, Any] | list[Any]) -> None:
    atomic_write_text(path, json.dumps(payload, indent=2) + "\n")


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file_local(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def aggregate_hash(entries: list[tuple[str, str]]) -> str:
    normalized = "\n".join(f"{path}:{digest}" for path, digest in sorted(entries)) + "\n"
    return sha256_text(normalized)


def run_command(
    command: list[str],
    *,
    cwd: Path | None = None,
    timeout_seconds: int = 0,
    check: bool = False,
) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        command,
        cwd=None if cwd is None else str(cwd),
        capture_output=True,
        text=True,
        timeout=None if timeout_seconds <= 0 else timeout_seconds,
        check=False,
    )
    if check and completed.returncode != 0:
        raise RuntimeError(
            f"command failed ({completed.returncode}): {' '.join(command)}\nSTDOUT:\n{completed.stdout}\nSTDERR:\n{completed.stderr}"
        )
    return completed


def run_command_bytes(
    command: list[str],
    *,
    cwd: Path | None = None,
    timeout_seconds: int = 0,
    check: bool = False,
) -> subprocess.CompletedProcess[bytes]:
    completed = subprocess.run(
        command,
        cwd=None if cwd is None else str(cwd),
        capture_output=True,
        timeout=None if timeout_seconds <= 0 else timeout_seconds,
        check=False,
    )
    if check and completed.returncode != 0:
        stdout = completed.stdout.decode("utf-8", errors="replace")
        stderr = completed.stderr.decode("utf-8", errors="replace")
        raise RuntimeError(
            f"command failed ({completed.returncode}): {' '.join(command)}\nSTDOUT:\n{stdout}\nSTDERR:\n{stderr}"
        )
    return completed


def hydrate_with_brctl(path: Path) -> None:
    try:
        run_command(["brctl", "download", str(path)], timeout_seconds=30, check=False)
    except Exception:
        pass


def git_repo_root(source_root: Path) -> Path | None:
    try:
        completed = run_command(["git", "-C", str(source_root), "rev-parse", "--show-toplevel"], timeout_seconds=10, check=True)
    except Exception:
        return None
    repo_root = completed.stdout.strip()
    return Path(repo_root).resolve() if repo_root else None


def git_object_path(source_root: Path, relative_path: str) -> str | None:
    repo_root = git_repo_root(source_root)
    if repo_root is None:
        return None
    absolute_path = (source_root / relative_path).resolve()
    try:
        repo_relative = absolute_path.relative_to(repo_root)
    except ValueError:
        return None
    return repo_relative.as_posix()


def should_prefer_worktree(relative_path: str) -> bool:
    if relative_path in WORKTREE_PRIORITY_FILES:
        return True
    return any(relative_path.startswith(prefix) for prefix in WORKTREE_PRIORITY_PREFIXES)


def read_git_head_bytes(source_root: Path, relative_path: str, *, timeout_seconds: int = 20) -> tuple[bytes | None, str]:
    object_path = git_object_path(source_root, relative_path)
    if not object_path:
        return None, "git_path_unavailable"
    try:
        completed = run_command_bytes(
            ["git", "-C", str(source_root), "show", f"HEAD:{object_path}"],
            timeout_seconds=timeout_seconds,
            check=True,
        )
    except subprocess.TimeoutExpired:
        return None, "git_timeout"
    except Exception as exc:
        return None, str(exc)
    return completed.stdout, "git_ok"


def hash_file_with_timeout(path: Path, *, timeout_seconds: int = 20) -> tuple[str | None, str]:
    hydrate_with_brctl(path)
    try:
        completed = run_command(["shasum", "-a", "256", str(path)], timeout_seconds=timeout_seconds, check=True)
    except subprocess.TimeoutExpired:
        return None, "timeout"
    except Exception as exc:
        return None, str(exc)
    parts = completed.stdout.strip().split()
    if not parts:
        return None, "empty_hash_output"
    return parts[0], "ok"


def hash_worktree_bytes_with_timeout(path: Path, *, timeout_seconds: int = 20) -> tuple[str | None, str]:
    hydrate_with_brctl(path)
    command = [
        "python3",
        "-c",
        (
            "import hashlib, pathlib, sys; "
            "data = pathlib.Path(sys.argv[1]).read_bytes(); "
            "print(hashlib.sha256(data).hexdigest())"
        ),
        str(path),
    ]
    try:
        completed = run_command(command, timeout_seconds=timeout_seconds, check=True)
    except subprocess.TimeoutExpired:
        return None, "timeout"
    except Exception as exc:
        return None, str(exc)
    digest = completed.stdout.strip()
    return (digest or None), ("ok" if digest else "empty_hash_output")


def copy_file_with_timeout(src: Path, dst: Path, *, timeout_seconds: int = 30) -> tuple[bool, str]:
    dst.parent.mkdir(parents=True, exist_ok=True)
    hydrate_with_brctl(src)
    try:
        run_command(
            [
                "python3",
                "-c",
                (
                    "from pathlib import Path; import sys; "
                    "src = Path(sys.argv[1]); dst = Path(sys.argv[2]); "
                    "dst.parent.mkdir(parents=True, exist_ok=True); "
                    "dst.write_bytes(src.read_bytes())"
                ),
                str(src),
                str(dst),
            ],
            timeout_seconds=timeout_seconds,
            check=True,
        )
        return True, "ok"
    except subprocess.TimeoutExpired:
        return False, "timeout"
    except Exception as exc:
        return False, str(exc)


def copy_path_with_timeout(src: Path, dst: Path, *, timeout_seconds: int = 900) -> tuple[bool, str]:
    if not src.exists():
        return False, "missing_source"
    if src.is_file():
        return copy_file_with_timeout(src, dst, timeout_seconds=timeout_seconds)
    try:
        dst.mkdir(parents=True, exist_ok=True)
        for child in sorted(src.rglob("*")):
            relative = child.relative_to(src)
            target = dst / relative
            if child.is_dir():
                target.mkdir(parents=True, exist_ok=True)
                continue
            ok, verdict = copy_file_with_timeout(child, target, timeout_seconds=timeout_seconds)
            if not ok:
                return False, verdict
        return True, "ok"
    except Exception as exc:
        return False, str(exc)


def write_git_head_file(source_root: Path, relative_path: str, dst: Path, *, timeout_seconds: int = 20) -> tuple[bool, str]:
    payload, verdict = read_git_head_bytes(source_root, relative_path, timeout_seconds=timeout_seconds)
    if payload is None:
        return False, verdict
    dst.parent.mkdir(parents=True, exist_ok=True)
    temp_path = dst.with_name(f".{dst.name}.tmp.git.{os.getpid()}.{time.time_ns()}")
    temp_path.write_bytes(payload)
    os.replace(temp_path, dst)
    return True, "git_ok"


def ensure_clean_dir(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def iter_snapshot_files(source_root: Path) -> list[str]:
    relative_paths: set[str] = set()
    for root_file in SNAPSHOT_ROOT_FILES:
        if (source_root / root_file).exists():
            relative_paths.add(root_file)
    for pattern in SNAPSHOT_GLOB_PATTERNS:
        for child in sorted(source_root.glob(pattern)):
            if child.is_file():
                relative_paths.add(str(child.relative_to(source_root)))
    return sorted(relative_paths)


def synthetic_root_cmakelists() -> str:
    return """cmake_minimum_required(VERSION 3.20)
project(raw_engine_v1_package LANGUAGES CXX)

option(RAW_ENGINE_ENABLE_ASAN "Enable ASan+UBSan for staged verification builds" OFF)

function(raw_engine_apply_common_flags target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(RAW_ENGINE_ENABLE_ASAN)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    endif()
endfunction()

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
"""


def synthetic_src_cmakelists() -> str:
    return """add_library(raw_engine_lib
    raw_core.cpp
    raw_validators.cpp
    raw_primitives.cpp
    raw_planner.cpp
)

target_include_directories(raw_engine_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

raw_engine_apply_common_flags(raw_engine_lib)
"""


def build_snapshot_entry(source_root: Path, relative_path: str) -> dict[str, Any]:
    path = source_root / relative_path
    if relative_path == "CMakeLists.txt":
        synthetic_text = synthetic_root_cmakelists()
        return {
            "relative_path": relative_path,
            "size_bytes": len(synthetic_text.encode("utf-8")),
            "expected_sha256": sha256_text(synthetic_text),
            "materialization_mode": "synthetic_bootstrap",
            "authoritative_read_verdict": "SKIPPED_FILE_PROVIDER_BOOTSTRAP",
            "hash_source": "synthetic_bootstrap",
        }
    if relative_path == "src/CMakeLists.txt":
        synthetic_text = synthetic_src_cmakelists()
        return {
            "relative_path": relative_path,
            "size_bytes": len(synthetic_text.encode("utf-8")),
            "expected_sha256": sha256_text(synthetic_text),
            "materialization_mode": "synthetic_src_bootstrap",
            "authoritative_read_verdict": "SKIPPED_FILE_PROVIDER_BOOTSTRAP",
            "hash_source": "synthetic_bootstrap",
        }
    digest, verdict = hash_worktree_bytes_with_timeout(path)
    if digest is not None:
        return {
            "relative_path": relative_path,
            "size_bytes": path.stat().st_size if path.exists() else None,
            "expected_sha256": digest,
            "materialization_mode": "copy",
            "authoritative_read_verdict": verdict,
            "hash_source": "authoritative_worktree",
        }
    if not should_prefer_worktree(relative_path):
        git_payload, git_verdict = read_git_head_bytes(source_root, relative_path)
        if git_payload is not None:
            return {
                "relative_path": relative_path,
                "size_bytes": len(git_payload),
                "expected_sha256": sha256_bytes(git_payload),
                "materialization_mode": "git_head",
                "authoritative_read_verdict": git_verdict,
                "hash_source": "git_head",
            }
    return {
        "relative_path": relative_path,
        "size_bytes": path.stat().st_size if path.exists() else None,
        "expected_sha256": None,
        "materialization_mode": "copy",
        "authoritative_read_verdict": verdict,
        "hash_source": "unavailable",
    }


def make_temp_dir(prefix: str) -> Path:
    return Path(tempfile.mkdtemp(prefix=prefix))
