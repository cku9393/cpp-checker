#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Summarize or execute resume commands from quota_pause_state.json."
    )
    parser.add_argument("--branch-root", required=True)
    parser.add_argument(
        "--pause-state-file",
        default=".ouroboros/quota_pause_state.json",
        help="Path relative to branch root or absolute path.",
    )
    parser.add_argument(
        "--action",
        choices=("summary", "resume", "restart"),
        default="summary",
    )
    return parser.parse_args()


def resolve_path(branch_root: Path, value: str) -> Path:
    path = Path(value).expanduser()
    return path if path.is_absolute() else (branch_root / path).resolve()


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()
    pause_state_file = resolve_path(branch_root, args.pause_state_file)
    if not pause_state_file.exists():
        raise SystemExit(f"pause state not found: {pause_state_file}")

    payload = json.loads(pause_state_file.read_text(encoding="utf-8"))
    resume_command = payload.get("resume_workflow_command")
    restart_command = payload.get("restart_retry_loop_command")

    if args.action == "summary":
        print(f"pause_state: {pause_state_file}")
        print(f"captured_at: {payload.get('captured_at')}")
        print(f"pause_recorded_at: {payload.get('pause_recorded_at')}")
        print(f"session_id: {payload.get('session_id')}")
        print(f"execution_id: {payload.get('execution_id')}")
        print(f"status_label: {payload.get('status_label')}")
        print(f"pause_reason: {(payload.get('pause_request') or {}).get('reason')}")
        print(f"resume_workflow_command: {resume_command}")
        print(f"restart_retry_loop_command: {restart_command}")
        return 0

    command = resume_command if args.action == "resume" else restart_command
    if not command:
        raise SystemExit(f"no command recorded for action={args.action}")

    result = subprocess.run(
        command,
        cwd=branch_root,
        shell=True,
        executable="/bin/zsh",
        check=False,
    )
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
