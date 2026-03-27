#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path


CHECKS = (
    ("rev_parse_root", ["git", "rev-parse", "--show-toplevel"], 10),
    ("head_commit", ["git", "rev-parse", "--short", "HEAD"], 10),
    ("status", ["git", "-c", "status.showUntrackedFiles=no", "status", "--short", "--untracked-files=no"], 10),
    ("fsck_connectivity", ["git", "fsck", "--connectivity-only", "--no-dangling", "--no-progress"], 20),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--branch-root", required=True)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument("--phase", required=True)
    return parser.parse_args()


def normalize_output(value: str | bytes | None) -> str:
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace").strip()
    return value.strip()


def run_check(branch_root: Path, name: str, cmd: list[str], timeout: int) -> dict:
    try:
        result = subprocess.run(
            cmd,
            cwd=branch_root,
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False,
        )
        return {
            "name": name,
            "command": cmd,
            "exit_code": result.returncode,
            "timed_out": False,
            "stdout": normalize_output(result.stdout),
            "stderr": normalize_output(result.stderr),
        }
    except subprocess.TimeoutExpired as exc:
        return {
            "name": name,
            "command": cmd,
            "exit_code": 124,
            "timed_out": True,
            "stdout": normalize_output(exc.stdout),
            "stderr": normalize_output(exc.stderr),
        }
    except Exception as exc:  # noqa: BLE001
        return {
            "name": name,
            "command": cmd,
            "exit_code": -1,
            "timed_out": False,
            "stdout": "",
            "stderr": str(exc),
        }


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root)
    attempt_dir = Path(args.attempt_dir)
    report_root = Path(args.report_root)
    attempt_dir.mkdir(parents=True, exist_ok=True)
    report_root.mkdir(parents=True, exist_ok=True)

    checks = [run_check(branch_root, name, cmd, timeout) for name, cmd, timeout in CHECKS]
    healthy = all(item["exit_code"] == 0 for item in checks)
    payload = {"phase": args.phase, "healthy": healthy, "checks": checks}

    report_json = attempt_dir / f"git_repo_health_{args.phase}.json"
    report_md = attempt_dir / f"git_repo_health_{args.phase}.md"
    latest_json = report_root / "latest_git_repo_health.json"
    latest_md = report_root / "latest_git_repo_health.md"
    report_json.write_text(json.dumps(payload, indent=2) + "\n")

    lines = [
        "# Git Repo Health",
        "",
        f"- Phase: `{args.phase}`",
        f"- Healthy: `{'yes' if healthy else 'no'}`",
        "",
    ]
    for item in checks:
        lines.append(f"## {item['name']}")
        lines.append("")
        lines.append(f"- Exit code: `{item['exit_code']}`")
        lines.append(f"- Timed out: `{'yes' if item['timed_out'] else 'no'}`")
        if item["stdout"]:
            lines.append(f"- Stdout: `{item['stdout'][:400]}`")
        if item["stderr"]:
            lines.append(f"- Stderr: `{item['stderr'][:400]}`")
        lines.append("")
    report_md.write_text("\n".join(lines))
    latest_json.write_text(report_json.read_text())
    latest_md.write_text(report_md.read_text())

    print(report_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
