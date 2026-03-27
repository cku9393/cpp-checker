#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
import time
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--state-file", required=True)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument("--branch-root", required=True)
    parser.add_argument("--timeout-seconds", type=int, default=180)
    return parser.parse_args()


def normalize_output(value: str | bytes | None) -> str:
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return value


def main() -> int:
    args = parse_args()
    state_file = Path(args.state_file)
    attempt_dir = Path(args.attempt_dir)
    report_root = Path(args.report_root)
    branch_root = Path(args.branch_root)
    attempt_dir.mkdir(parents=True, exist_ok=True)
    report_root.mkdir(parents=True, exist_ok=True)

    try:
        state = json.loads(state_file.read_text())
    except Exception as exc:  # noqa: BLE001
        print(f"probe skipped: could not read state ({exc})")
        return 1

    command = state.get("next_probe_command")
    primary_axis = state.get("pinned_primary_axis")
    secondary_axis = state.get("pinned_secondary_axis")
    why_this_axis = state.get("why_this_axis")
    if not command:
        print("probe skipped: next_probe_command is empty")
        return 1

    started = time.time()
    timed_out = False
    try:
        result = subprocess.run(
            command,
            cwd=branch_root,
            shell=True,
            executable="/bin/zsh",
            capture_output=True,
            timeout=args.timeout_seconds,
            env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"),
        )
        exit_code = result.returncode
        stdout = normalize_output(result.stdout)
        stderr = normalize_output(result.stderr)
    except subprocess.TimeoutExpired as exc:
        timed_out = True
        exit_code = 124
        stdout = normalize_output(exc.stdout)
        stderr = normalize_output(exc.stderr)
    elapsed = round(time.time() - started, 3)

    payload = {
        "command": command,
        "primary_axis": primary_axis,
        "secondary_axis": secondary_axis,
        "why_this_axis": why_this_axis,
        "timeout_seconds": args.timeout_seconds,
        "elapsed_seconds": elapsed,
        "exit_code": exit_code,
        "timed_out": timed_out,
    }

    report_json = attempt_dir / "next_probe_result.json"
    report_md = attempt_dir / "next_probe_result.md"
    report_stdout = attempt_dir / "next_probe.stdout.log"
    report_stderr = attempt_dir / "next_probe.stderr.log"
    latest_json = report_root / "latest_next_probe_result.json"
    latest_md = report_root / "latest_next_probe_result.md"
    latest_stdout = report_root / "latest_next_probe.stdout.log"
    latest_stderr = report_root / "latest_next_probe.stderr.log"

    report_json.write_text(json.dumps(payload, indent=2) + "\n")
    report_stdout.write_text(stdout)
    report_stderr.write_text(stderr)
    report_md.write_text(
        "\n".join(
            [
                "# Next Probe Result",
                "",
                f"- Command: `{command}`",
                f"- Primary axis: `{primary_axis or 'unknown'}`",
                f"- Secondary axis: `{secondary_axis or 'none'}`",
                f"- Why this axis: `{why_this_axis or 'not recorded'}`",
                f"- Exit code: `{exit_code}`",
                f"- Timed out: `{'yes' if timed_out else 'no'}`",
                f"- Elapsed seconds: `{elapsed}`",
                "",
                f"- Stdout log: `{report_stdout}`",
                f"- Stderr log: `{report_stderr}`",
                "",
            ]
        )
    )
    latest_json.write_text(report_json.read_text())
    latest_md.write_text(report_md.read_text())
    latest_stdout.write_text(stdout)
    latest_stderr.write_text(stderr)

    print(report_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
