#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Any

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from research_review_gate import load_research_review_gate
from retry_artifact_io import prepare_output_dir, resolve_artifact_output_path, write_text_output

from verify_analysis_refresh import (
    load_latest_failure_refresh_baseline,
    load_analysis_session_metadata,
    resolve_recognized_branch_local_analysis_targets,
    load_workflow_recognized_targets_from_state,
    load_structured_dict,
    verify_analysis_session,
    verify_current_state,
    verify_post_failure_refresh_asset_freshness,
    verify_recognized_refresh_targets,
)

RETRY_REPORT_VOLATILE_NAMES = {
    "soft_stop_request.json",
    "quota_pause_state.json",
    "latest_workflow.log",
    "latest_runtime_snapshot.json",
    "latest_runtime_snapshot.md",
    "latest_quota_pause.json",
    "latest_quota_pause.md",
    "latest_quota_watch_status.json",
    "latest_quota_watch_status.md",
    "latest_manual_pause.json",
    "latest_manual_pause.md",
}
RETRY_REPORT_VOLATILE_GLOBS = {
    "latest_retry_inputs_snapshot*",
    "latest_solver_seed.snapshot*",
    "latest_analysis_seed.snapshot*",
}
LEGACY_NON_ARTIFACT_VOLATILE_RELATIVE_PATHS = {
    Path(".ouroboros") / "soft_stop_request.json",
    Path(".ouroboros") / "quota_pause_state.json",
    Path(".ouroboros") / "latest_workflow.log",
    Path(".ouroboros") / "latest_runtime_snapshot.json",
    Path(".ouroboros") / "latest_runtime_snapshot.md",
    Path(".ouroboros") / "latest_quota_pause.json",
    Path(".ouroboros") / "latest_quota_pause.md",
    Path(".ouroboros") / "latest_quota_watch_status.json",
    Path(".ouroboros") / "latest_quota_watch_status.md",
    Path(".ouroboros") / "latest_manual_pause.json",
    Path(".ouroboros") / "latest_manual_pause.md",
    Path(".ouroboros") / "latest_retry_inputs_snapshot.json",
    Path(".ouroboros") / "latest_retry_inputs_snapshot.md",
    Path(".ouroboros") / "latest_solver_seed.snapshot",
    Path(".ouroboros") / "latest_solver_seed.snapshot.yaml",
    Path(".ouroboros") / "latest_analysis_seed.snapshot",
    Path(".ouroboros") / "latest_analysis_seed.snapshot.yaml",
    Path(".ouroboros") / "latest_analysis_refresh.log",
}
LEGACY_NON_ARTIFACT_VOLATILE_GLOBS = (
    Path(".ouroboros") / "analysis_refresh_attempt_*.log",
)
ARCHIVABLE_TOP_LEVEL_NAMES = {
    "smoke_latest_failure",
    "smoke_launcher_latest_failure",
    "smoke_latest_status",
}
LATEST_FAILURE_REPORT_NAME = "latest_failure_report.md"
LATEST_FAILURE_BREAKDOWN_NAME = "latest_failure_breakdown.md"
LATEST_ANALYSIS_SESSION_NAME = "latest_analysis_session.md"
ANALYSIS_STATE_RELATIVE_PATH = Path(".ouroboros") / "failure_analysis_state.json"
LIVE_GATE_LOCK_WAIT_SECONDS = 20.0
LIVE_GATE_LOCK_POLL_SECONDS = 1.0


@dataclass
class LockState:
    lock_dir: str
    pid: int | None
    status: str
    ps: str | None = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Clear volatile branch-local lca_tree_stress_v5 state before a retry-loop attempt."
    )
    parser.add_argument("--branch-root", required=True)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    return parser.parse_args()


def _load_artifact_helpers(branch_root: Path) -> tuple[Any, Any, Any]:
    sys.path.insert(0, str(branch_root))
    import artifact_paths as artifact_guard  # type: ignore

    return (
        artifact_guard.artifacts_root,
        artifact_guard.ensure_under_artifacts,
        getattr(artifact_guard, "resolve_branch_artifact_path", None),
    )


def _resolve_artifact_cli_path(
    branch_root: Path,
    ensure_under_artifacts,
    value: str,
    shared_resolver=None,
) -> Path:
    if shared_resolver is not None:
        return shared_resolver(value)
    return resolve_artifact_output_path(branch_root, value, ensure_under_artifacts)


def _pid_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True


def _ps_summary(pid: int) -> str:
    try:
        result = subprocess.run(
            ["ps", "-p", str(pid), "-o", "pid=,ppid=,etime=,command="],
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as exc:
        return f"ps unavailable: {exc}"
    text = (result.stdout or result.stderr or "").strip()
    return text or "ps returned no details"


def _remove_path(path: Path) -> None:
    if not path.exists() and not path.is_symlink():
        return
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path, ignore_errors=True)
        return
    try:
        path.unlink()
    except FileNotFoundError:
        return


def _normalize_volatile_root(path: Path) -> list[str]:
    removed: list[str] = []
    if not path.exists() and not path.is_symlink():
        return removed
    if path.is_dir() and not path.is_symlink():
        return removed
    _remove_path(path)
    removed.append(str(path))
    return removed


def _cleanup_lock_root(lock_root: Path) -> tuple[list[str], list[LockState]]:
    removed: list[str] = []
    active: list[LockState] = []
    if not lock_root.exists():
        return removed, active

    for child in sorted(lock_root.iterdir(), key=lambda entry: entry.name):
        if not child.is_dir():
            _remove_path(child)
            removed.append(str(child))
            continue

        pid_file = child / "pid"
        pid: int | None = None
        try:
            pid = int(pid_file.read_text(encoding="utf-8").strip())
        except (FileNotFoundError, OSError, ValueError):
            pid = None

        if pid is not None and _pid_alive(pid):
            active.append(
                LockState(
                    lock_dir=str(child),
                    pid=pid,
                    status="active_pid",
                    ps=_ps_summary(pid),
                )
            )
            continue

        _remove_path(child)
        removed.append(str(child))

    if not active and lock_root.exists():
        try:
            lock_root.rmdir()
            removed.append(str(lock_root))
        except OSError:
            pass

    return removed, active


def _wait_for_active_locks(lock_root: Path) -> tuple[list[str], list[LockState]]:
    removed_paths: list[str] = []
    seen_removed: set[str] = set()
    deadline = time.monotonic() + LIVE_GATE_LOCK_WAIT_SECONDS

    while True:
        removed, active = _cleanup_lock_root(lock_root)
        for entry in removed:
            if entry in seen_removed:
                continue
            seen_removed.add(entry)
            removed_paths.append(entry)
        if not active:
            return removed_paths, active
        if time.monotonic() >= deadline:
            return removed_paths, active
        time.sleep(LIVE_GATE_LOCK_POLL_SECONDS)


def _cleanup_tmp_root(tmp_root: Path) -> tuple[list[str], list[str]]:
    removed: list[str] = []
    preserved: list[str] = []
    if not tmp_root.exists():
        return removed, preserved

    for child in sorted(tmp_root.iterdir(), key=lambda entry: entry.name):
        if child.name == "case_cache":
            preserved.append(str(child))
            continue
        _remove_path(child)
        removed.append(str(child))

    if not preserved and tmp_root.exists():
        try:
            tmp_root.rmdir()
            removed.append(str(tmp_root))
        except OSError:
            pass

    return removed, preserved


def _cleanup_retry_report_root(report_root: Path) -> list[str]:
    removed: list[str] = []
    if not report_root.exists():
        return removed

    seen_paths: set[Path] = set()
    for name in sorted(RETRY_REPORT_VOLATILE_NAMES):
        candidate = report_root / name
        if not candidate.exists() and not candidate.is_symlink():
            continue
        seen_paths.add(candidate.resolve())
        _remove_path(candidate)
        removed.append(str(candidate))
    for pattern in sorted(RETRY_REPORT_VOLATILE_GLOBS):
        for candidate in sorted(report_root.glob(pattern), key=lambda path: path.name):
            resolved_candidate = candidate.resolve()
            if resolved_candidate in seen_paths:
                continue
            seen_paths.add(resolved_candidate)
            _remove_path(candidate)
            removed.append(str(candidate))
    return removed


def _cleanup_legacy_non_artifact_volatiles(branch_root: Path) -> list[str]:
    removed: list[str] = []
    seen: set[Path] = set()
    for relative_path in sorted(LEGACY_NON_ARTIFACT_VOLATILE_RELATIVE_PATHS):
        candidate = (branch_root / relative_path).resolve()
        if not candidate.exists() and not candidate.is_symlink():
            continue
        seen.add(candidate)
        _remove_path(candidate)
        removed.append(str(candidate))
    for relative_glob in LEGACY_NON_ARTIFACT_VOLATILE_GLOBS:
        for candidate in sorted(branch_root.glob(relative_glob.as_posix())):
            resolved_candidate = candidate.resolve()
            if resolved_candidate in seen:
                continue
            if not candidate.exists() and not candidate.is_symlink():
                continue
            seen.add(resolved_candidate)
            _remove_path(candidate)
            removed.append(str(resolved_candidate))
    return removed


def _pre_rewrite_research_review_gate(branch_root: Path) -> dict[str, Any]:
    try:
        gate_payload = load_research_review_gate(branch_root)
    except Exception as exc:  # noqa: BLE001
        return {
            "status": "blocked",
            "reason": f"pre-rewrite research review gate is stale: {exc}",
        }

    return {
        "status": "ok",
        "reason": "branch-local notes and bundled progress40 materials have recorded review completions",
        **gate_payload,
    }


def _latest_failure_analysis_gate(branch_root: Path, report_root: Path) -> dict[str, Any]:
    latest_failure_report = report_root / LATEST_FAILURE_REPORT_NAME
    latest_failure_breakdown = report_root / LATEST_FAILURE_BREAKDOWN_NAME
    latest_analysis_session = report_root / LATEST_ANALYSIS_SESSION_NAME
    analysis_state = (branch_root / ANALYSIS_STATE_RELATIVE_PATH).resolve()

    if not latest_failure_report.exists() and not latest_failure_breakdown.exists():
        return {
            "status": "not_applicable",
            "reason": "no latest failed-attempt report/breakdown published yet",
            "verified_markers": [],
        }

    missing_paths = [
        str(path)
        for path in (latest_failure_report, latest_failure_breakdown, analysis_state)
        if not path.exists()
    ]
    if missing_paths:
        return {
            "status": "blocked",
            "reason": "latest failed-attempt analysis refresh is incomplete",
            "missing_paths": missing_paths,
            "verified_markers": [],
        }

    analysis_session_warning: str | None = None
    recognized_targets = resolve_recognized_branch_local_analysis_targets(analysis_state)
    try:
        verified_markers = verify_current_state(
            analysis_state,
            latest_failure_report,
            latest_failure_breakdown,
            recognized_targets=recognized_targets,
        )
        state_payload = load_structured_dict(analysis_state)
        refresh_evidence = state_payload.get("refresh_evidence")
        if not isinstance(refresh_evidence, dict):
            raise ValueError(f"`{analysis_state}` refresh_evidence is missing or not a JSON object")
        latest_failure_epoch, latest_failure_label = load_latest_failure_refresh_baseline(
            latest_failure_report,
            latest_failure_breakdown,
        )
        verified_markers.extend(
            verify_post_failure_refresh_asset_freshness(
                analysis_state,
                refresh_evidence,
                latest_failure_epoch,
                latest_failure_label,
                require_iteration_metadata=False,
            )
        )
        recognized_targets = load_workflow_recognized_targets_from_state(analysis_state)
        verified_markers.extend(
            verify_recognized_refresh_targets(
                analysis_state,
                refresh_evidence,
                recognized_targets,
                baseline_epoch=0.0,
                latest_failure_epoch=latest_failure_epoch,
                latest_failure_label=latest_failure_label,
            )
        )
        if latest_analysis_session.exists():
            try:
                load_analysis_session_metadata(latest_analysis_session)
            except Exception:  # noqa: BLE001
                pass
            else:
                try:
                    verified_markers.extend(
                        verify_analysis_session(
                            latest_analysis_session,
                            latest_failure_report,
                            latest_failure_breakdown,
                            analysis_state,
                            latest_failure_epoch,
                            latest_failure_label,
                        )
                    )
                except Exception as exc:  # noqa: BLE001
                    analysis_session_warning = str(exc)
    except Exception as exc:  # noqa: BLE001
        return {
            "status": "blocked",
            "reason": f"latest failed-attempt analysis refresh is stale: {exc}",
            "verified_markers": [],
        }

    return {
        "status": "ok",
        "reason": "latest failed-attempt analysis state is current",
        "verified_markers": verified_markers,
        "latest_failure_report": str(latest_failure_report),
        "latest_failure_breakdown": str(latest_failure_breakdown),
        "analysis_state": str(analysis_state),
        "latest_analysis_session": str(latest_analysis_session) if latest_analysis_session.exists() else None,
        "analysis_session_warning": analysis_session_warning,
    }


def _archive_path(path: Path, archive_root: Path) -> dict[str, str] | None:
    if not path.exists() and not path.is_symlink():
        return None

    archive_root.mkdir(parents=True, exist_ok=True)
    candidate = archive_root / path.name
    suffix = 1
    while candidate.exists() or candidate.is_symlink():
        candidate = archive_root / f"{path.name}.{suffix:02d}"
        suffix += 1

    shutil.move(str(path), str(candidate))
    return {
        "source": str(path),
        "archive": str(candidate),
    }


def _cleanup_top_level(
    lca_root: Path,
    archive_root: Path,
) -> tuple[list[str], list[dict[str, str]]]:
    removed: list[str] = []
    archived: list[dict[str, str]] = []
    if not lca_root.exists():
        return removed, archived

    removable_names = {
        "smoke_setup",
        ".repeatability_stage",
    }
    for child in sorted(lca_root.iterdir(), key=lambda entry: entry.name):
        name = child.name
        if name in {".tmp", ".locks", "retry_loop"}:
            continue
        if (
            name in removable_names
            or name in ARCHIVABLE_TOP_LEVEL_NAMES
            or name.endswith(".latest_failure")
            or name.endswith(".previous")
            or "_in_progress." in name
        ):
            if name in ARCHIVABLE_TOP_LEVEL_NAMES or name.endswith(".latest_failure"):
                archived_entry = _archive_path(child, archive_root)
                if archived_entry is not None:
                    archived.append(archived_entry)
                    removed.append(str(child))
                continue
            _remove_path(child)
            removed.append(str(child))
    return removed, archived


def _write_reports(
    attempt_dir: Path,
    report_root: Path,
    payload: dict[str, Any],
) -> None:
    prepare_output_dir(attempt_dir)
    prepare_output_dir(report_root)

    report_json = attempt_dir / "pre_attempt_cleanup.json"
    report_md = attempt_dir / "pre_attempt_cleanup.md"
    latest_json = report_root / "latest_pre_attempt_cleanup.json"
    latest_md = report_root / "latest_pre_attempt_cleanup.md"

    write_text_output(report_json, json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    active_lines = [
        f"- `{item['lock_dir']}` pid=`{item['pid']}` status=`{item['status']}` ps=`{item['ps'] or 'n/a'}`"
        for item in payload["active_locks"]
    ]
    archived_lines = [
        f"- `{item['source']}` -> `{item['archive']}`"
        for item in payload.get("archived_paths", [])
    ] or ["- none"]
    removed_lines = [f"- `{path}`" for path in payload["removed_paths"]] or ["- none"]
    preserved_lines = [f"- `{path}`" for path in payload["preserved_paths"]] or ["- none"]
    research_review_gate = payload.get("research_review_gate") or {}
    research_gate_status = research_review_gate.get("status", "unknown")
    research_gate_reason = research_review_gate.get("reason", "not recorded")
    research_gate_checkpoints = [
        f"- `{item}`" for item in research_review_gate.get("checkpoint_files", [])
    ] or ["- none"]
    research_gate_source_set_a = [
        f"- `{item}`" for item in research_review_gate.get("source_set_a_paths", [])
    ] or ["- none"]
    research_gate_source_set_b = [
        f"- `{item}`" for item in research_review_gate.get("source_set_b_paths", [])
    ] or ["- none"]
    analysis_refresh_gate = payload.get("analysis_refresh_gate") or {}
    analysis_gate_status = analysis_refresh_gate.get("status", "unknown")
    analysis_gate_reason = analysis_refresh_gate.get("reason", "not recorded")
    analysis_gate_verified = [
        f"- `{item}`" for item in analysis_refresh_gate.get("verified_markers", [])
    ] or ["- none"]
    analysis_gate_missing = [
        f"- `{item}`" for item in analysis_refresh_gate.get("missing_paths", [])
    ] or ["- none"]

    write_text_output(
        report_md,
        "\n".join(
            [
                "# Pre-attempt Cleanup Report",
                "",
                f"- Timestamp: `{payload['timestamp']}`",
                f"- Artifacts root: `{payload['artifacts_root']}`",
                f"- Cleared stale state: `{'yes' if payload['status'] == 'ok' else 'no'}`",
                f"- Active lock blockers: `{len(payload['active_locks'])}`",
                "",
                "## Archived Paths",
                *archived_lines,
                "",
                "## Removed Paths",
                *removed_lines,
                "",
                "## Preserved Paths",
                *preserved_lines,
                "",
                "## Active Locks",
                *(active_lines or ["- none"]),
                "",
                "## Pre-Rewrite Research Review Gate",
                f"- Status: `{research_gate_status}`",
                f"- Reason: `{research_gate_reason}`",
                "- Checkpoint files:",
                *research_gate_checkpoints,
                "- Source set A paths:",
                *research_gate_source_set_a,
                "- Source set B paths:",
                *research_gate_source_set_b,
                "",
                "## Latest Failure Analysis Gate",
                f"- Status: `{analysis_gate_status}`",
                f"- Reason: `{analysis_gate_reason}`",
                "- Verified markers:",
                *analysis_gate_verified,
                "- Missing paths:",
                *analysis_gate_missing,
                "",
            ]
        )
        + "\n",
        encoding="utf-8",
    )

    write_text_output(latest_json, report_json.read_text(encoding="utf-8"), encoding="utf-8")
    write_text_output(latest_md, report_md.read_text(encoding="utf-8"), encoding="utf-8")


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()

    artifacts_root_fn, ensure_under_artifacts, shared_resolver = _load_artifact_helpers(branch_root)
    artifacts_root = ensure_under_artifacts(Path(artifacts_root_fn()))
    lca_root = ensure_under_artifacts((artifacts_root / "lca_tree_stress_v5").resolve())
    attempt_dir = _resolve_artifact_cli_path(
        branch_root, ensure_under_artifacts, args.attempt_dir, shared_resolver
    )
    report_root = _resolve_artifact_cli_path(
        branch_root, ensure_under_artifacts, args.report_root, shared_resolver
    )

    lock_root = lca_root / ".locks"
    tmp_root = lca_root / ".tmp"
    pre_attempt_archive_root = attempt_dir / "pre_attempt_archive"

    removed_paths: list[str] = []
    preserved_paths: list[str] = []
    archived_paths: list[dict[str, str]] = []

    removed_paths.extend(_normalize_volatile_root(lock_root))
    removed_paths.extend(_normalize_volatile_root(tmp_root))

    removed_locks, active_locks = _wait_for_active_locks(lock_root)
    removed_paths.extend(removed_locks)

    if active_locks:
        payload = {
            "timestamp": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
            "status": "blocked_active_lock",
            "artifacts_root": str(lca_root),
            "removed_paths": removed_paths,
            "preserved_paths": preserved_paths,
            "archived_paths": archived_paths,
            "active_locks": [asdict(item) for item in active_locks],
        }
        _write_reports(attempt_dir, report_root, payload)
        print("pre-attempt cleanup blocked: live branch-local gate lock detected")
        for item in active_locks:
            print(f"  {item.lock_dir} pid={item.pid} {item.ps or ''}".rstrip())
        return 1

    removed_tmp, preserved_tmp = _cleanup_tmp_root(tmp_root)
    removed_paths.extend(removed_tmp)
    preserved_paths.extend(preserved_tmp)
    removed_top_level, archived_top_level = _cleanup_top_level(lca_root, pre_attempt_archive_root)
    removed_paths.extend(removed_top_level)
    archived_paths.extend(archived_top_level)
    removed_paths.extend(_cleanup_retry_report_root(report_root))
    removed_paths.extend(_cleanup_legacy_non_artifact_volatiles(branch_root))
    research_review_gate = _pre_rewrite_research_review_gate(branch_root)
    analysis_refresh_gate = _latest_failure_analysis_gate(branch_root, report_root)

    if research_review_gate["status"] == "blocked":
        payload = {
            "timestamp": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
            "status": "blocked_research_review",
            "artifacts_root": str(lca_root),
            "removed_paths": removed_paths,
            "preserved_paths": preserved_paths,
            "archived_paths": archived_paths,
            "active_locks": [],
            "research_review_gate": research_review_gate,
            "analysis_refresh_gate": analysis_refresh_gate,
        }
        _write_reports(attempt_dir, report_root, payload)
        print("pre-attempt cleanup blocked: pre-rewrite research review gate is missing or stale")
        print(f"  {research_review_gate['reason']}")
        return 1

    if analysis_refresh_gate["status"] == "blocked":
        payload = {
            "timestamp": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
            "status": "blocked_analysis_refresh",
            "artifacts_root": str(lca_root),
            "removed_paths": removed_paths,
            "preserved_paths": preserved_paths,
            "archived_paths": archived_paths,
            "active_locks": [],
            "research_review_gate": research_review_gate,
            "analysis_refresh_gate": analysis_refresh_gate,
        }
        _write_reports(attempt_dir, report_root, payload)
        print("pre-attempt cleanup blocked: latest failed-attempt analysis refresh is missing or stale")
        print(f"  {analysis_refresh_gate['reason']}")
        for path in analysis_refresh_gate.get("missing_paths", []):
            print(f"  missing={path}")
        return 1

    payload = {
        "timestamp": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
        "status": "ok",
        "artifacts_root": str(lca_root),
        "removed_paths": removed_paths,
        "preserved_paths": preserved_paths,
        "archived_paths": archived_paths,
        "active_locks": [],
        "research_review_gate": research_review_gate,
        "analysis_refresh_gate": analysis_refresh_gate,
    }
    _write_reports(attempt_dir, report_root, payload)

    print("pre-attempt cleanup ok")
    print(f"  artifacts_root={lca_root}")
    print(f"  removed_paths={len(removed_paths)}")
    if preserved_paths:
        print(f"  preserved_paths={len(preserved_paths)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
