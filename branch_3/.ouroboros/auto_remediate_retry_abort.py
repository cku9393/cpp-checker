#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import signal
import shutil
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Any

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from retry_artifact_io import prepare_output_dir, resolve_artifact_output_path, write_text_output


MAX_CONSECUTIVE_AUTO_REMEDIATIONS = 3
ANALYSIS_REFRESH_FAILURE_MARKERS = (
    "failed to refresh mandatory analysis assets",
    "latest failed-attempt analysis refresh is stale",
    "retry start blocked until at least one refreshed branch-local analysis asset is present",
)
OUTPUT_LOCALITY_FAILURE_MARKERS = (
    "generated non-artifact output outside branch-local artifacts",
    "output locality guard detected generated output outside branch-local artifacts",
)
LIVE_GATE_LOCK_FAILURE_MARKERS = (
    "pre-attempt cleanup blocked: live branch-local gate lock detected",
)
TRANSIENT_NON_ARTIFACT_PARTS = frozenset(
    {
        ".pytest_cache",
        "__pycache__",
        ".mypy_cache",
        ".ruff_cache",
    }
)
KNOWN_GATE_LOCK_SCRIPT_NAMES = frozenset(
    {
        "lca_smoke.sh",
        "lca_smoke_repeatability.sh",
        "lca_strong_gate.sh",
        "lca_boj3s_gate.sh",
        "lca_acceptance_repeatability.sh",
        "lca_required_repeatability.sh",
    }
)
LIVE_GATE_LOCK_WAIT_SECONDS = 20.0
LIVE_GATE_LOCK_POLL_SECONDS = 1.0
LIVE_GATE_LOCK_TERM_GRACE_SECONDS = 8.0
ANALYSIS_REFRESH_TARGETS = (
    ".ouroboros/capture_failure_context.py",
    ".ouroboros/failure_analysis_playbook.md",
    ".ouroboros/failure_analysis_iteration.md",
    ".ouroboros/failure_analysis_state.json",
    ".ouroboros/launch_retry_loop.sh",
    ".ouroboros/prepare_retry_attempt_state.py",
    ".ouroboros/refresh_analysis_state.py",
    ".ouroboros/verify_analysis_refresh.py",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Attempt structured auto-remediation after a retry-loop abort, then allow relaunch if safe."
    )
    parser.add_argument("--branch-root", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument("--launch-log", required=True)
    parser.add_argument("--loop-exit-code", type=int, required=True)
    return parser.parse_args()


def timestamp() -> str:
    return datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")


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


def _load_artifact_guard(branch_root: Path):
    sys.path.insert(0, str(branch_root))
    import artifact_paths as artifact_guard  # type: ignore

    return artifact_guard


def _ensure_artifact_path(branch_root: Path, artifact_guard, value: str | Path) -> Path:
    return resolve_artifact_output_path(branch_root, value, artifact_guard.ensure_under_artifacts)


def _run(
    cmd: list[str],
    *,
    cwd: Path,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=str(cwd),
        check=False,
        text=True,
        capture_output=True,
    )


def _write_report(branch_root: Path, report_root: Path, artifact_guard, payload: dict[str, Any]) -> None:
    json_path = _ensure_artifact_path(
        branch_root=branch_root,
        artifact_guard=artifact_guard,
        value=report_root / "latest_auto_remediation.json",
    )
    md_path = _ensure_artifact_path(
        branch_root=branch_root,
        artifact_guard=artifact_guard,
        value=report_root / "latest_auto_remediation.md",
    )
    prepare_output_dir(json_path.parent)
    write_text_output(json_path, json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    lines = [
        "# Auto Remediation",
        "",
        f"- Timestamp: `{payload.get('timestamp')}`",
        f"- Handled: `{str(payload.get('handled', False)).lower()}`",
        f"- Strategy: `{payload.get('strategy') or 'none'}`",
        f"- Reason: `{payload.get('reason') or 'n/a'}`",
        f"- Loop exit code: `{payload.get('loop_exit_code')}`",
    ]
    fingerprint = payload.get("fingerprint")
    if fingerprint:
        lines.append(f"- Fingerprint: `{fingerprint}`")
    details = payload.get("details") or {}
    if isinstance(details, dict) and details:
        lines.append("")
        lines.append("## Details")
        lines.append("")
        for key, value in details.items():
            if isinstance(value, (list, tuple)):
                rendered = ", ".join(str(item) for item in value) if value else "none"
            else:
                rendered = str(value)
            lines.append(f"- {key}: `{rendered}`")
    write_text_output(md_path, "\n".join(lines) + "\n", encoding="utf-8")


def _load_state_file(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:  # noqa: BLE001
        return {}


def _update_remediation_state(state_path: Path, fingerprint: str) -> dict[str, Any]:
    payload = _load_state_file(state_path)
    previous_fingerprint = payload.get("last_fingerprint")
    consecutive_count = int(payload.get("consecutive_count") or 0)
    if previous_fingerprint == fingerprint:
        consecutive_count += 1
    else:
        consecutive_count = 1
    updated = {
        "last_fingerprint": fingerprint,
        "consecutive_count": consecutive_count,
        "updated_at": timestamp(),
    }
    write_text_output(state_path, json.dumps(updated, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return updated


def _parse_launch_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""


def _latest_attempt_dir(report_root: Path) -> Path | None:
    candidates = sorted(
        (
            path
            for path in report_root.glob("attempt_*")
            if path.is_dir() and re.match(r"attempt_\d{3}_\d{8}_\d{6}$", path.name)
        ),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    return candidates[0] if candidates else None


def _parse_kv_sections(path: Path) -> tuple[dict[str, str], dict[str, list[str]]]:
    metadata: dict[str, str] = {}
    sections: dict[str, list[str]] = {}
    current_section: str | None = None
    for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            current_section = line[1:-1]
            sections.setdefault(current_section, [])
            continue
        if current_section is None and "=" in line:
            key, value = line.split("=", 1)
            metadata[key] = value
            continue
        if current_section is not None:
            sections.setdefault(current_section, []).append(line)
    return metadata, sections


def _is_transient_non_artifact(path: str) -> bool:
    rel_path = Path(path)
    return any(part in TRANSIENT_NON_ARTIFACT_PARTS for part in rel_path.parts)


def _purge_transient_non_artifact_paths(branch_root: Path, artifact_guard) -> list[str]:
    removed: list[str] = []
    for root, dirnames, _ in os.walk(branch_root):
        current = Path(root)
        if artifact_guard._is_under_artifacts(current):  # type: ignore[attr-defined]
            dirnames[:] = []
            continue
        for dirname in list(dirnames):
            if dirname not in TRANSIENT_NON_ARTIFACT_PARTS:
                continue
            target = current / dirname
            _remove_path(target)
            removed.append(str(target.relative_to(branch_root)))
            dirnames.remove(dirname)
    return removed


def _verify_preflight(branch_root: Path, report_root: Path) -> subprocess.CompletedProcess[str]:
    temp_attempt_dir = report_root / "auto_remediation_preflight"
    return _run(
        [
            "python3",
            ".ouroboros/prepare_retry_attempt_state.py",
            "--branch-root",
            str(branch_root),
            "--attempt-dir",
            str(temp_attempt_dir),
            "--report-root",
            str(report_root),
        ],
        cwd=branch_root,
    )


def _pid_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True


def _branch_lock_root(branch_root: Path) -> Path:
    return (branch_root / "artifacts" / "lca_tree_stress_v5" / ".locks").resolve()


def _is_branch_local_gate_lock(branch_root: Path, lock_dir: Path) -> bool:
    try:
        lock_dir.resolve().relative_to(_branch_lock_root(branch_root))
    except ValueError:
        return False
    return True


def _parse_live_gate_lock_blockers(text: str) -> list[dict[str, Any]]:
    blockers: list[dict[str, Any]] = []
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if "pid=" not in line:
            continue
        match = re.match(r"^(?P<lock_dir>.+?)\s+pid=(?P<pid>\d+)(?:\s+(?P<ps>.*))?$", line)
        if not match:
            continue
        blockers.append(
            {
                "lock_dir": match.group("lock_dir"),
                "pid": int(match.group("pid")),
                "ps": (match.group("ps") or "").strip() or None,
            }
        )
    return blockers


def _is_known_gate_lock_holder(branch_root: Path, ps_text: str | None) -> bool:
    if not ps_text:
        return False
    normalized = ps_text.replace("\\", "/")
    if str(branch_root).replace("\\", "/") not in normalized:
        return False
    return any(
        f"/{script_name}" in normalized or f"/outer_suite_wrappers/{script_name}" in normalized
        for script_name in KNOWN_GATE_LOCK_SCRIPT_NAMES
    )


def _refresh_live_gate_lock_blockers(
    branch_root: Path,
    blockers: list[dict[str, Any]],
) -> tuple[list[dict[str, Any]], list[str]]:
    remaining: list[dict[str, Any]] = []
    removed_paths: list[str] = []
    seen_removed: set[str] = set()

    for blocker in blockers:
        lock_path = Path(str(blocker["lock_dir"])).resolve()
        pid = int(blocker["pid"])
        pid_alive = _pid_alive(pid)
        lock_exists = lock_path.exists()
        if not pid_alive and lock_exists and _is_branch_local_gate_lock(branch_root, lock_path):
            _remove_path(lock_path)
            lock_exists = lock_path.exists()
            if not lock_exists:
                removed = str(lock_path)
                if removed not in seen_removed:
                    seen_removed.add(removed)
                    removed_paths.append(removed)
        if pid_alive or lock_exists:
            remaining.append(
                {
                    **blocker,
                    "pid_alive": pid_alive,
                    "lock_exists": lock_exists,
                }
            )
    return remaining, removed_paths


def _wait_for_live_gate_lock_clear(
    branch_root: Path,
    blockers: list[dict[str, Any]],
    *,
    timeout_seconds: float,
) -> tuple[list[dict[str, Any]], list[str]]:
    deadline = time.monotonic() + timeout_seconds
    removed_paths: list[str] = []
    seen_removed: set[str] = set()
    remaining = blockers

    while True:
        remaining, newly_removed = _refresh_live_gate_lock_blockers(branch_root, remaining)
        for path in newly_removed:
            if path in seen_removed:
                continue
            seen_removed.add(path)
            removed_paths.append(path)
        if not remaining:
            return remaining, removed_paths
        if time.monotonic() >= deadline:
            return remaining, removed_paths
        time.sleep(LIVE_GATE_LOCK_POLL_SECONDS)


def _terminate_live_gate_lock_blockers(
    branch_root: Path,
    blockers: list[dict[str, Any]],
) -> dict[str, Any]:
    term_sent: list[int] = []
    kill_sent: list[int] = []
    skipped: list[str] = []

    for blocker in blockers:
        lock_path = Path(str(blocker["lock_dir"])).resolve()
        pid = int(blocker["pid"])
        ps_text = blocker.get("ps")
        if not _is_branch_local_gate_lock(branch_root, lock_path):
            skipped.append(f"{lock_path}:unsafe_lock_path")
            continue
        if not _pid_alive(pid):
            continue
        if not _is_known_gate_lock_holder(branch_root, ps_text):
            skipped.append(f"{lock_path}:unrecognized_holder")
            continue
        try:
            os.kill(pid, signal.SIGTERM)
            term_sent.append(pid)
        except OSError:
            continue

    remaining, removed_after_term = _wait_for_live_gate_lock_clear(
        branch_root,
        blockers,
        timeout_seconds=LIVE_GATE_LOCK_TERM_GRACE_SECONDS,
    )

    for blocker in remaining:
        pid = int(blocker["pid"])
        lock_path = Path(str(blocker["lock_dir"])).resolve()
        if not _is_branch_local_gate_lock(branch_root, lock_path):
            continue
        if not _pid_alive(pid):
            continue
        try:
            os.kill(pid, signal.SIGKILL)
            kill_sent.append(pid)
        except OSError:
            continue

    final_remaining, removed_after_kill = _wait_for_live_gate_lock_clear(
        branch_root,
        remaining,
        timeout_seconds=LIVE_GATE_LOCK_TERM_GRACE_SECONDS,
    )

    return {
        "term_sent": sorted(set(term_sent)),
        "kill_sent": sorted(set(kill_sent)),
        "skipped": skipped,
        "removed_paths": removed_after_term + removed_after_kill,
        "remaining": final_remaining,
    }


def _remediate_live_gate_lock(
    branch_root: Path,
    report_root: Path,
    artifact_guard,
    launch_text: str,
) -> tuple[bool, dict[str, Any]]:
    preflight = _verify_preflight(branch_root, report_root)
    blocker_text = preflight.stdout if any(
        marker in preflight.stdout for marker in LIVE_GATE_LOCK_FAILURE_MARKERS
    ) else launch_text

    if preflight.returncode == 0:
        return True, {
            "reason": "transient live gate lock cleared before retry-loop remediation finished",
            "preflight_stdout": preflight.stdout.strip(),
        }

    if not any(marker in blocker_text for marker in LIVE_GATE_LOCK_FAILURE_MARKERS):
        return False, {
            "reason": "no live gate lock marker present in current preflight output",
            "stdout": preflight.stdout.strip(),
            "stderr": preflight.stderr.strip(),
        }

    blockers = _parse_live_gate_lock_blockers(blocker_text)
    if not blockers:
        return False, {"reason": "failed to parse live gate lock blockers from preflight output"}

    remaining, removed_paths = _wait_for_live_gate_lock_clear(
        branch_root,
        blockers,
        timeout_seconds=LIVE_GATE_LOCK_WAIT_SECONDS,
    )
    if remaining:
        termination = _terminate_live_gate_lock_blockers(branch_root, remaining)
        remaining = termination["remaining"]
        removed_paths.extend(termination["removed_paths"])
    else:
        termination = {
            "term_sent": [],
            "kill_sent": [],
            "skipped": [],
        }

    final_preflight = _verify_preflight(branch_root, report_root)
    if final_preflight.returncode == 0:
        return True, {
            "reason": "live gate lock auto-cleared and preflight recovered",
            "removed_paths": removed_paths,
            "term_sent": termination["term_sent"],
            "kill_sent": termination["kill_sent"],
            "skipped": termination["skipped"],
            "preflight_stdout": final_preflight.stdout.strip(),
        }

    return False, {
        "reason": "prepare_retry_attempt_state.py still blocks after live gate lock remediation",
        "removed_paths": removed_paths,
        "term_sent": termination["term_sent"],
        "kill_sent": termination["kill_sent"],
        "skipped": termination["skipped"],
        "stdout": final_preflight.stdout.strip(),
        "stderr": final_preflight.stderr.strip(),
    }


def _latest_failure_attempt_dir(report_root: Path) -> Path | None:
    candidates = sorted(
        (
            path
            for path in report_root.glob("attempt_*")
            if path.is_dir()
            and re.match(r"attempt_\d{3}_\d{8}_\d{6}$", path.name)
            and (path / "failure_report.json").exists()
            and (path / "failure_breakdown.json").exists()
        ),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    return candidates[0] if candidates else None


def _analysis_round_from_log(path: Path) -> int | None:
    match = re.search(r"round_(\d+)\.log$", path.name)
    if match:
        return int(match.group(1))
    return None


def _choose_analysis_log(attempt_dir: Path) -> tuple[Path | None, int | None]:
    logs = sorted(attempt_dir.glob("analysis_workflow_round_*.log"))
    if not logs:
        return None, None
    log_path = logs[-1]
    return log_path, _analysis_round_from_log(log_path)


def _parse_analysis_exit_code(log_text: str) -> str:
    match = re.search(r"workflow exited with code (\d+)", log_text)
    if match:
        return match.group(1)
    if "Execution completed successfully" in log_text:
        return "0"
    return "1"


def _write_analysis_session_summary(
    branch_root: Path,
    report_root: Path,
    artifact_guard,
    *,
    attempt_dir: Path,
    analysis_log: Path,
    analysis_round: int,
    state_payload: dict[str, Any],
) -> None:
    refresh_evidence = state_payload.get("refresh_evidence") or {}
    refresh_timestamp = (
        refresh_evidence.get("analysis_refresh_timestamp")
        or refresh_evidence.get("current_failure_timestamp")
        or timestamp()
    )
    current_attempt = state_payload.get("current_failure_attempt") or "unknown"
    current_signature = state_payload.get("current_failure_signature") or "unknown"
    primary_axis = state_payload.get("pinned_primary_axis") or "unknown"
    secondary_axis = state_payload.get("pinned_secondary_axis") or "none"
    next_probe = state_payload.get("next_probe_command") or "none"
    why_this_axis = state_payload.get("why_this_axis") or "not recorded"
    failed_solver_attempt = re.sub(r"^attempt_", "", str(current_attempt))
    analysis_exit_code = _parse_analysis_exit_code(analysis_log.read_text(encoding="utf-8", errors="replace"))

    lines = [
        "# Analysis Session Summary",
        "",
        f"- Timestamp: `{refresh_timestamp}`",
        f"- Failed solver attempt: `{failed_solver_attempt}`",
        "- Analysis seed: `.ouroboros/seed_branch3_failure_analysis.yaml`",
        f"- Analysis round: `{analysis_round}`",
        f"- Analysis log: `{analysis_log}`",
        f"- Analysis workflow exit code: `{analysis_exit_code}`",
        "- Verification: `refreshed analysis assets linked to latest failure`",
        "- Current for latest failure: `yes`",
        f"- Current failure attempt: `{current_attempt}`",
        f"- Current failure signature: `{current_signature}`",
        f"- Primary axis: `{primary_axis}`",
        f"- Secondary axis: `{secondary_axis}`",
        f"- Next probe command: `{next_probe}`",
        f"- Why this axis: `{why_this_axis}`",
        "",
        "Analysis targets considered refreshed after baseline:",
        "- `.ouroboros/capture_failure_context.py`",
        "- `.ouroboros/failure_analysis_playbook.md`",
        "- `.ouroboros/failure_analysis_iteration.md`",
        "- `.ouroboros/failure_analysis_state.json`",
        "",
        "The retry loop verified that `.ouroboros/failure_analysis_state.json` is marked",
        "current for the latest captured failure before allowing another solver retry.",
        "",
        "Next solver retry must read:",
        "- `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md`",
        "- `artifacts/lca_tree_stress_v5/retry_loop/latest_failure_breakdown.md`",
        "- `artifacts/lca_tree_stress_v5/retry_loop/latest_analysis_session.md`",
        "- `.ouroboros/failure_analysis_iteration.md`",
        "- `.ouroboros/failure_analysis_state.json`",
        "",
        "The next solver retry must stay anchored to the primary/secondary axis above and",
        "must not broaden into an unrelated rewrite unless new evidence disproves them.",
        "",
    ]
    attempt_summary = _ensure_artifact_path(branch_root, artifact_guard, attempt_dir / "latest_analysis_session.md")
    latest_summary = _ensure_artifact_path(branch_root, artifact_guard, report_root / "latest_analysis_session.md")
    write_text_output(attempt_summary, "\n".join(lines), encoding="utf-8")
    write_text_output(latest_summary, "\n".join(lines), encoding="utf-8")


def _remediate_stale_analysis(
    branch_root: Path,
    report_root: Path,
    artifact_guard,
) -> tuple[bool, dict[str, Any]]:
    attempt_dir = _latest_failure_attempt_dir(report_root)
    if attempt_dir is None:
        return False, {"reason": "no failed attempt with failure report/breakdown found"}

    analysis_log, analysis_round = _choose_analysis_log(attempt_dir)
    if analysis_log is None or analysis_round is None:
        return False, {"reason": "no analysis workflow round log available for latest failed attempt"}

    refresh_result = _run(
        [
            "python3",
            ".ouroboros/refresh_analysis_state.py",
            "--attempt",
            str(int(attempt_dir.name.split("_", 2)[1])),
            "--attempt-dir",
            str(attempt_dir),
            "--report-root",
            str(report_root),
            "--analysis-log",
            str(analysis_log),
            "--analysis-round",
            str(analysis_round),
            "--state-file",
            ".ouroboros/failure_analysis_state.json",
            "--iteration-file",
            ".ouroboros/failure_analysis_iteration.md",
        ],
        cwd=branch_root,
    )
    if refresh_result.returncode != 0:
        return False, {
            "reason": "refresh_analysis_state.py failed",
            "stdout": refresh_result.stdout.strip(),
            "stderr": refresh_result.stderr.strip(),
        }

    state_path = branch_root / ".ouroboros" / "failure_analysis_state.json"
    state_payload = _load_state_file(state_path)
    _write_analysis_session_summary(
        branch_root,
        report_root,
        artifact_guard,
        attempt_dir=attempt_dir,
        analysis_log=analysis_log,
        analysis_round=analysis_round,
        state_payload=state_payload,
    )

    verify_result = _run(
        [
            "python3",
            ".ouroboros/verify_analysis_refresh.py",
            "--baseline-epoch",
            "0",
            "--analysis-log",
            str(analysis_log),
            "--target-from-current-state",
            *sum((["--target", target] for target in ANALYSIS_REFRESH_TARGETS), []),
            "--latest-failure-report",
            str(attempt_dir / "failure_report.json"),
            "--latest-failure-breakdown",
            str(attempt_dir / "failure_breakdown.json"),
            "--require-current-state",
            ".ouroboros/failure_analysis_state.json",
            "--require-analysis-session",
            str(report_root / "latest_analysis_session.md"),
        ],
        cwd=branch_root,
    )
    if verify_result.returncode != 0:
        return False, {
            "reason": "verify_analysis_refresh.py failed after state refresh",
            "stdout": verify_result.stdout.strip(),
            "stderr": verify_result.stderr.strip(),
        }

    preflight = _verify_preflight(branch_root, report_root)
    if preflight.returncode != 0:
        return False, {
            "reason": "prepare_retry_attempt_state.py still blocks after analysis refresh repair",
            "stdout": preflight.stdout.strip(),
            "stderr": preflight.stderr.strip(),
        }

    return True, {
        "reason": "analysis refresh state/session re-synced",
        "attempt_dir": str(attempt_dir),
        "analysis_log": str(analysis_log),
        "analysis_round": analysis_round,
        "preflight_stdout": preflight.stdout.strip(),
    }


def _remediate_output_locality(
    branch_root: Path,
    report_root: Path,
    artifact_guard,
) -> tuple[bool, dict[str, Any]]:
    report_candidates = sorted(
        report_root.glob("latest*_non_artifact_tree_report.txt"),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    if not report_candidates:
        return False, {"reason": "no latest non-artifact tree report available"}

    report_path = report_candidates[0]
    metadata, sections = _parse_kv_sections(report_path)
    current_state_path = Path(metadata.get("current_state", "")).expanduser()
    baseline_state_path = Path(metadata.get("baseline_state", "")).expanduser()
    if not current_state_path.exists() or not baseline_state_path.exists():
        return False, {"reason": "non-artifact tree baseline/current snapshot missing"}

    current_payload = _load_state_file(current_state_path)
    baseline_payload = _load_state_file(baseline_state_path)
    current_entries = current_payload.get("entries") or {}
    baseline_entries = baseline_payload.get("entries") or {}
    if not isinstance(current_entries, dict):
        return False, {"reason": "current non-artifact tree snapshot is malformed"}
    if not isinstance(baseline_entries, dict):
        return False, {"reason": "baseline non-artifact tree snapshot is malformed"}

    blocking_created = list(sections.get("created", []))
    blocking_removed = list(sections.get("removed", []))
    removed_paths: list[str] = []
    promoted_warnings: list[str] = []
    unresolved: list[str] = []

    for rel_path in blocking_created:
        entry = current_entries.get(rel_path)
        if _is_transient_non_artifact(rel_path):
            _remove_path(branch_root / rel_path)
            removed_paths.append(rel_path)
            continue
        if artifact_guard._is_advisory_non_artifact_creation(rel_path, entry):
            promoted_warnings.append(rel_path)
            continue
        unresolved.append(rel_path)

    for rel_path in blocking_removed:
        entry = baseline_entries.get(rel_path)
        if artifact_guard._is_advisory_non_artifact_removal(rel_path, entry):
            promoted_warnings.append(rel_path)
            continue
        unresolved.append(rel_path)

    if unresolved:
        return False, {
            "reason": "blocking non-artifact changes remain after structured locality remediation",
            "report_path": str(report_path),
            "unresolved": unresolved,
            "promoted_warnings": promoted_warnings,
            "removed_paths": removed_paths,
        }

    removed_paths.extend(_purge_transient_non_artifact_paths(branch_root, artifact_guard))

    verify_current = _ensure_artifact_path(
        branch_root,
        artifact_guard,
        report_root / "latest_auto_remediation_non_artifact_tree_current.json",
    )
    verify_report = _ensure_artifact_path(
        branch_root,
        artifact_guard,
        report_root / "latest_auto_remediation_non_artifact_tree_report.txt",
    )
    clean = artifact_guard.verify_non_artifact_tree_state(
        baseline_state_path,
        verify_current,
        verify_report,
    )
    if not clean:
        return False, {
            "reason": "current artifact_paths policy still sees a blocking non-artifact escape",
            "report_path": str(verify_report),
        }

    preflight = _verify_preflight(branch_root, report_root)
    if preflight.returncode != 0:
        return False, {
            "reason": "prepare_retry_attempt_state.py blocks after locality remediation",
            "stdout": preflight.stdout.strip(),
            "stderr": preflight.stderr.strip(),
        }

    return True, {
        "reason": "output locality abort reduced to warning-only or transient paths",
        "report_path": str(report_path),
        "promoted_warnings": promoted_warnings,
        "removed_paths": removed_paths,
        "preflight_stdout": preflight.stdout.strip(),
    }


def _ensure_latest_analysis_session_current(
    branch_root: Path,
    report_root: Path,
    artifact_guard,
) -> tuple[bool, dict[str, Any]]:
    attempt_dir = _latest_failure_attempt_dir(report_root)
    if attempt_dir is None:
        return True, {"reason": "no latest failed attempt requires analysis-session verification"}

    analysis_log, analysis_round = _choose_analysis_log(attempt_dir)
    if analysis_log is None or analysis_round is None:
        return False, {"reason": "latest failed attempt has no analysis workflow round log to refresh from"}

    verify_result = _run(
        [
            "python3",
            ".ouroboros/verify_analysis_refresh.py",
            "--baseline-epoch",
            "0",
            "--analysis-log",
            str(analysis_log),
            "--target-from-current-state",
            *sum((["--target", target] for target in ANALYSIS_REFRESH_TARGETS), []),
            "--latest-failure-report",
            str(attempt_dir / "failure_report.json"),
            "--latest-failure-breakdown",
            str(attempt_dir / "failure_breakdown.json"),
            "--require-current-state",
            ".ouroboros/failure_analysis_state.json",
            "--require-analysis-session",
            str(report_root / "latest_analysis_session.md"),
        ],
        cwd=branch_root,
    )
    if verify_result.returncode == 0:
        return True, {
            "reason": "latest analysis session already matched the newest failed attempt",
            "attempt_dir": str(attempt_dir),
            "analysis_log": str(analysis_log),
            "analysis_round": analysis_round,
        }

    handled, details = _remediate_stale_analysis(branch_root, report_root, artifact_guard)
    if handled:
        details = {
            **details,
            "reason": "latest analysis session/state re-synced after remediation",
        }
    return handled, details


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()
    report_root = Path(args.report_root).resolve()
    launch_log = Path(args.launch_log).resolve()
    artifact_guard = _load_artifact_guard(branch_root)
    remediation_state_path = _ensure_artifact_path(
        branch_root,
        artifact_guard,
        report_root / "auto_remediation_state.json",
    )

    launch_text = _parse_launch_text(launch_log)
    strategy = "none"
    handled = False
    details: dict[str, Any] = {}
    fingerprint = ""

    if any(marker in launch_text for marker in LIVE_GATE_LOCK_FAILURE_MARKERS):
        strategy = "live_gate_lock"
        handled, details = _remediate_live_gate_lock(branch_root, report_root, artifact_guard, launch_text)
        blocker_names = [
            Path(str(item["lock_dir"])).name
            for item in _parse_live_gate_lock_blockers(launch_text)
        ]
        fingerprint = "live_gate_lock:" + ",".join(sorted(set(blocker_names)))
    elif args.loop_exit_code in {6, 7} or any(marker in launch_text for marker in OUTPUT_LOCALITY_FAILURE_MARKERS):
        strategy = "output_locality"
        handled, details = _remediate_output_locality(branch_root, report_root, artifact_guard)
        fingerprint = "output_locality:" + ",".join(sorted((details.get("unresolved") or []) + (details.get("promoted_warnings") or [])))
    elif args.loop_exit_code in {2, 4} or any(
        marker in launch_text for marker in ANALYSIS_REFRESH_FAILURE_MARKERS
    ):
        strategy = "analysis_refresh"
        handled, details = _remediate_stale_analysis(branch_root, report_root, artifact_guard)
        fingerprint = f"analysis_refresh:{details.get('attempt_dir') or 'none'}:{details.get('analysis_round') or 'none'}"
    else:
        details = {"reason": "no structured auto-remediation strategy matched the latest loop abort"}

    if handled:
        analysis_synced, analysis_details = _ensure_latest_analysis_session_current(
            branch_root,
            report_root,
            artifact_guard,
        )
        if not analysis_synced:
            handled = False
            details = {
                "reason": "latest analysis session could not be synchronized after remediation",
                "remediation_details": details,
                "analysis_sync_details": analysis_details,
            }
        else:
            details["analysis_sync"] = analysis_details

    if handled:
        remediation_state = _update_remediation_state(remediation_state_path, fingerprint)
        if int(remediation_state.get("consecutive_count") or 0) > MAX_CONSECUTIVE_AUTO_REMEDIATIONS:
            handled = False
            details = {
                "reason": "auto-remediation hit the consecutive retry safety cap",
                "fingerprint": fingerprint,
                "consecutive_count": remediation_state.get("consecutive_count"),
            }

    payload = {
        "timestamp": timestamp(),
        "handled": handled,
        "strategy": strategy,
        "reason": details.get("reason"),
        "loop_exit_code": args.loop_exit_code,
        "fingerprint": fingerprint or None,
        "details": details,
    }
    _write_report(branch_root, report_root, artifact_guard, payload)
    if handled:
        print(json.dumps(payload, ensure_ascii=False))
        return 0
    print(json.dumps(payload, ensure_ascii=False), file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
