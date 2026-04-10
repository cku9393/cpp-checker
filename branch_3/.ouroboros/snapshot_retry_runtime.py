#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import sys
from collections import deque
from datetime import datetime
from pathlib import Path
from typing import Any

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from retry_artifact_io import prepare_output_dir, write_text_output


SESSION_RE = re.compile(r"(?:session_id.?=.?|Session ID:\s+)([A-Za-z0-9_]+)")
EXECUTION_RE = re.compile(r"(?:execution_id.?=.?|Execution ID:\s+)([A-Za-z0-9_]+)")
SESSION_TOKEN_RE = re.compile(r"orch_[A-Za-z0-9]+")
EXECUTION_TOKEN_RE = re.compile(r"exec_[A-Za-z0-9]+")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Write a runtime snapshot for the active branch_3 retry-loop attempt."
    )
    parser.add_argument("--branch-root", required=True)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument("--attempt-log", required=True)
    parser.add_argument("--seed-file", required=True)
    parser.add_argument("--analysis-seed-file", default="")
    parser.add_argument("--current-log", default="")
    parser.add_argument("--status-label", default="heartbeat")
    parser.add_argument("--loop-pid", type=int)
    parser.add_argument("--workflow-pid", type=int)
    parser.add_argument("--quota-watchdog-pid", type=int)
    parser.add_argument("--screen-session", default="")
    parser.add_argument("--soft-stop-file", default="")
    parser.add_argument(
        "--pause-state-file",
        default="artifacts/lca_tree_stress_v5/retry_loop/quota_pause_state.json",
        help="Path relative to branch root or absolute path.",
    )
    parser.add_argument("--write-pause-state", action="store_true")
    return parser.parse_args()


def _load_artifact_guard(branch_root: Path):
    sys.path.insert(0, str(branch_root))
    from artifact_paths import ensure_under_artifacts  # type: ignore

    return ensure_under_artifacts


def resolve_path(branch_root: Path, value: str) -> Path | None:
    if not value:
        return None
    path = Path(value).expanduser()
    return path if path.is_absolute() else (branch_root / path).resolve()


def resolve_artifact_path(branch_root: Path, ensure_under_artifacts, value: str) -> Path | None:
    resolved = resolve_path(branch_root, value)
    if resolved is None:
        return None
    return ensure_under_artifacts(resolved)


def load_tail(path: Path, limit: int = 40) -> list[str]:
    if not path.exists():
        return []
    tail = deque(maxlen=limit)
    with path.open("r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            tail.append(line.rstrip())
    return list(tail)


def parse_session_fields(lines: list[str]) -> tuple[str | None, str | None]:
    session_id = None
    execution_id = None
    for line in lines:
        if session_id is None:
            match = SESSION_RE.search(line)
            if match:
                session_id = match.group(1)
            else:
                token_match = SESSION_TOKEN_RE.search(line)
                if token_match:
                    session_id = token_match.group(0)
        if execution_id is None:
            match = EXECUTION_RE.search(line)
            if match:
                execution_id = match.group(1)
            else:
                token_match = EXECUTION_TOKEN_RE.search(line)
                if token_match:
                    execution_id = token_match.group(0)
    return session_id, execution_id


def parse_session_fields_from_file(path: Path) -> tuple[str | None, str | None]:
    if not path.exists():
        return None, None
    session_id = None
    execution_id = None
    with path.open("r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if session_id is None:
                match = SESSION_RE.search(line)
                if match:
                    session_id = match.group(1)
                else:
                    token_match = SESSION_TOKEN_RE.search(line)
                    if token_match:
                        session_id = token_match.group(0)
            if execution_id is None:
                match = EXECUTION_RE.search(line)
                if match:
                    execution_id = match.group(1)
                else:
                    token_match = EXECUTION_TOKEN_RE.search(line)
                    if token_match:
                        execution_id = token_match.group(0)
            if session_id and execution_id:
                break
    return session_id, execution_id


def latest_level_line(lines: list[str]) -> str | None:
    for line in reversed(lines):
        if "Level " in line:
            return line.strip()
    return None


def latest_focus_line(lines: list[str]) -> str | None:
    for line in reversed(lines):
        stripped = line.strip()
        if stripped.startswith("AC ") or stripped.startswith("Sub-AC "):
            return stripped
    return None


def read_json(path: Path | None) -> dict[str, Any] | None:
    if path is None or not path.exists():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:  # noqa: BLE001
        return None


def select_active_seed_file(
    *,
    status_label: str,
    attempt_log: Path,
    current_log: Path,
    seed_file: str,
    analysis_seed_file: str | None,
) -> str:
    if not analysis_seed_file:
        return seed_file
    if status_label.startswith("analysis_round_"):
        return analysis_seed_file
    if current_log != attempt_log and current_log.name.startswith("analysis_workflow_round_"):
        return analysis_seed_file
    return seed_file


def shell_join(parts: list[str]) -> str:
    return " ".join(shlex.quote(part) for part in parts)


def write_reports(
    attempt_dir: Path,
    report_root: Path,
    pause_state_path: Path,
    payload: dict[str, Any],
    write_pause_state: bool,
) -> None:
    runtime_json = attempt_dir / "runtime_snapshot.json"
    runtime_md = attempt_dir / "runtime_snapshot.md"
    latest_runtime_json = report_root / "latest_runtime_snapshot.json"
    latest_runtime_md = report_root / "latest_runtime_snapshot.md"

    write_text_output(runtime_json, json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    write_text_output(
        runtime_md,
        "\n".join(
            [
                "# Runtime Snapshot",
                "",
                f"- Captured at: `{payload['captured_at']}`",
                f"- Status: `{payload['status_label']}`",
                f"- Attempt dir: `{payload['attempt_dir']}`",
                f"- Attempt log: `{payload['attempt_log']}`",
                f"- Current log: `{payload.get('current_log') or payload['attempt_log']}`",
                f"- Session ID: `{payload.get('session_id') or 'unknown'}`",
                f"- Execution ID: `{payload.get('execution_id') or 'unknown'}`",
                f"- Loop PID: `{payload.get('loop_pid') or 'unknown'}`",
                f"- Workflow PID: `{payload.get('workflow_pid') or 'unknown'}`",
                f"- Quota watchdog PID: `{payload.get('quota_watchdog_pid') or 'unknown'}`",
                f"- Screen session: `{payload.get('screen_session') or 'unknown'}`",
                f"- Latest level: `{payload.get('latest_level') or 'unknown'}`",
                f"- Current focus: `{payload.get('current_focus') or 'unknown'}`",
                "",
                "## Resume Commands",
                "",
                "```bash",
                payload["resume_workflow_command"],
                "```",
                "",
                "```bash",
                payload["restart_retry_loop_command"],
                "```",
                "",
                "## Workflow Tail",
                "",
                "```text",
                *payload["attempt_log_tail"],
                "```",
                "",
            ]
        )
        + "\n",
        encoding="utf-8",
    )
    write_text_output(latest_runtime_json, runtime_json.read_text(encoding="utf-8"), encoding="utf-8")
    write_text_output(latest_runtime_md, runtime_md.read_text(encoding="utf-8"), encoding="utf-8")

    if write_pause_state:
        quota_pause_json = report_root / "latest_quota_pause.json"
        quota_pause_md = report_root / "latest_quota_pause.md"
        pause_payload = dict(payload)
        pause_request = pause_payload.get("pause_request") or {}
        pause_payload["pause_recorded_at"] = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")
        write_text_output(pause_state_path, json.dumps(pause_payload, indent=2) + "\n", encoding="utf-8")
        write_text_output(quota_pause_json, json.dumps(pause_payload, indent=2) + "\n", encoding="utf-8")
        write_text_output(
            quota_pause_md,
            "\n".join(
                [
                    "# Quota Pause Snapshot",
                    "",
                    f"- Pause recorded at: `{pause_payload['pause_recorded_at']}`",
                    f"- Pause reason: `{pause_request.get('reason') or 'unknown'}`",
                    f"- Trigger: `{pause_request.get('trigger') or 'unknown'}`",
                    f"- Triggered limits: `{', '.join(pause_request.get('triggered_limits') or []) or 'none'}`",
                    f"- Attempt dir: `{pause_payload['attempt_dir']}`",
                    f"- Session ID: `{pause_payload.get('session_id') or 'unknown'}`",
                    f"- Execution ID: `{pause_payload.get('execution_id') or 'unknown'}`",
                    "",
                    "## Resume Commands",
                    "",
                    "```bash",
                    pause_payload["resume_workflow_command"],
                    "```",
                    "",
                    "```bash",
                    pause_payload["restart_retry_loop_command"],
                    "```",
                    "",
                ]
            )
            + "\n",
            encoding="utf-8",
        )


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()
    ensure_under_artifacts = _load_artifact_guard(branch_root)
    attempt_dir = resolve_artifact_path(branch_root, ensure_under_artifacts, args.attempt_dir)
    report_root = resolve_artifact_path(branch_root, ensure_under_artifacts, args.report_root)
    attempt_log = resolve_artifact_path(branch_root, ensure_under_artifacts, args.attempt_log)
    current_log = (
        resolve_artifact_path(branch_root, ensure_under_artifacts, args.current_log)
        if args.current_log
        else attempt_log
    )
    soft_stop_file = (
        resolve_artifact_path(branch_root, ensure_under_artifacts, args.soft_stop_file)
        if args.soft_stop_file
        else None
    )
    pause_state_path = resolve_artifact_path(branch_root, ensure_under_artifacts, args.pause_state_file)

    prepare_output_dir(attempt_dir)
    prepare_output_dir(report_root)
    prepare_output_dir(pause_state_path.parent)

    attempt_tail = load_tail(attempt_log)
    current_tail = load_tail(current_log) if current_log else attempt_tail
    session_id, execution_id = parse_session_fields_from_file(attempt_log)
    if session_id is None or execution_id is None:
        tail_session_id, tail_execution_id = parse_session_fields(attempt_tail)
        session_id = session_id or tail_session_id
        execution_id = execution_id or tail_execution_id

    payload = {
        "captured_at": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
        "status_label": args.status_label,
        "attempt_dir": str(attempt_dir),
        "attempt_log": str(attempt_log),
        "current_log": str(current_log) if current_log else str(attempt_log),
        "seed_file": args.seed_file,
        "analysis_seed_file": args.analysis_seed_file or None,
        "session_id": session_id,
        "execution_id": execution_id,
        "loop_pid": args.loop_pid,
        "workflow_pid": args.workflow_pid,
        "quota_watchdog_pid": args.quota_watchdog_pid,
        "screen_session": args.screen_session or None,
        "latest_level": latest_level_line(attempt_tail),
        "current_focus": latest_focus_line(current_tail),
        "attempt_log_tail": attempt_tail,
        "current_log_tail": current_tail,
        "latest_failure_report": str(report_root / "latest_failure_report.md"),
        "latest_failure_breakdown": str(report_root / "latest_failure_breakdown.md"),
        "latest_analysis_session": str(report_root / "latest_analysis_session.md"),
        "latest_next_probe_result": str(report_root / "latest_next_probe_result.md"),
        "latest_quota_watch_status": str(report_root / "latest_quota_watch_status.md"),
        "resume_workflow_command": (
            f'cd "{branch_root}" && ouroboros run workflow --resume {session_id} "{args.seed_file}" --runtime codex'
            if session_id
            else f'cd "{branch_root}" && echo "no session_id captured yet; use outer restart"'
        ),
        "restart_retry_loop_command": (
            f'cd "{branch_root}" && caffeinate -ims zsh ".ouroboros/run_until_pass_progress40.sh"'
        ),
        "pause_request": read_json(soft_stop_file),
    }

    write_reports(
        attempt_dir=attempt_dir,
        report_root=report_root,
        pause_state_path=pause_state_path,
        payload=payload,
        write_pause_state=args.write_pause_state,
    )
    print(report_root / "latest_runtime_snapshot.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
