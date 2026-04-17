#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from retry_artifact_io import prepare_output_dir, resolve_artifact_output_path, write_text_output


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


def _load_artifact_guard(branch_root: Path):
    sys.path.insert(0, str(branch_root))
    import artifact_paths as artifact_guard  # type: ignore

    return artifact_guard.ensure_under_artifacts, getattr(
        artifact_guard, "resolve_branch_artifact_path", None
    )


def _resolve_artifact_path(
    branch_root: Path,
    ensure_under_artifacts,
    value: str,
    shared_resolver=None,
) -> Path:
    if shared_resolver is not None:
        return shared_resolver(value)
    return resolve_artifact_output_path(branch_root, value, ensure_under_artifacts)


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
    branch_root = Path(args.branch_root).resolve()
    ensure_under_artifacts, shared_resolver = _load_artifact_guard(branch_root)
    attempt_dir = _resolve_artifact_path(
        branch_root, ensure_under_artifacts, args.attempt_dir, shared_resolver
    )
    report_root = _resolve_artifact_path(
        branch_root, ensure_under_artifacts, args.report_root, shared_resolver
    )
    prepare_output_dir(attempt_dir)
    prepare_output_dir(report_root)

    checks = [run_check(branch_root, name, cmd, timeout) for name, cmd, timeout in CHECKS]
    healthy = all(item["exit_code"] == 0 for item in checks)
    payload = {"phase": args.phase, "healthy": healthy, "checks": checks}

    report_json = attempt_dir / f"git_repo_health_{args.phase}.json"
    report_md = attempt_dir / f"git_repo_health_{args.phase}.md"
    latest_json = report_root / "latest_git_repo_health.json"
    latest_md = report_root / "latest_git_repo_health.md"
    write_text_output(report_json, json.dumps(payload, indent=2) + "\n")

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
    write_text_output(report_md, "\n".join(lines))
    write_text_output(latest_json, report_json.read_text())
    write_text_output(latest_md, report_md.read_text())

    print(report_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
