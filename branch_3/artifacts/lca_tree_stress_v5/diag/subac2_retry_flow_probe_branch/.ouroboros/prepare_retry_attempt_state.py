#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Any


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


def _load_artifact_helpers(branch_root: Path) -> tuple[Any, Any]:
    sys.path.insert(0, str(branch_root))
    from artifact_paths import artifacts_root, ensure_under_artifacts  # type: ignore

    return artifacts_root, ensure_under_artifacts


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


def _cleanup_top_level(lca_root: Path) -> list[str]:
    removed: list[str] = []
    removable_names = {
        "smoke_latest_failure",
        "smoke_setup",
        ".repeatability_stage",
    }
    for child in sorted(lca_root.iterdir(), key=lambda entry: entry.name):
        name = child.name
        if name in {".tmp", ".locks", "retry_loop"}:
            continue
        if (
            name in removable_names
            or name.endswith(".latest_failure")
            or name.endswith(".previous")
            or "_in_progress." in name
        ):
            _remove_path(child)
            removed.append(str(child))
    return removed


def _write_reports(
    attempt_dir: Path,
    report_root: Path,
    payload: dict[str, Any],
) -> None:
    attempt_dir.mkdir(parents=True, exist_ok=True)
    report_root.mkdir(parents=True, exist_ok=True)

    report_json = attempt_dir / "pre_attempt_cleanup.json"
    report_md = attempt_dir / "pre_attempt_cleanup.md"
    latest_json = report_root / "latest_pre_attempt_cleanup.json"
    latest_md = report_root / "latest_pre_attempt_cleanup.md"

    report_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    active_lines = [
        f"- `{item['lock_dir']}` pid=`{item['pid']}` status=`{item['status']}` ps=`{item['ps'] or 'n/a'}`"
        for item in payload["active_locks"]
    ]
    removed_lines = [f"- `{path}`" for path in payload["removed_paths"]] or ["- none"]
    preserved_lines = [f"- `{path}`" for path in payload["preserved_paths"]] or ["- none"]

    report_md.write_text(
        "\n".join(
            [
                "# Pre-attempt Cleanup Report",
                "",
                f"- Timestamp: `{payload['timestamp']}`",
                f"- Artifacts root: `{payload['artifacts_root']}`",
                f"- Cleared stale state: `{'yes' if payload['status'] == 'ok' else 'no'}`",
                f"- Active lock blockers: `{len(payload['active_locks'])}`",
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
            ]
        )
        + "\n",
        encoding="utf-8",
    )

    latest_json.write_text(report_json.read_text(encoding="utf-8"), encoding="utf-8")
    latest_md.write_text(report_md.read_text(encoding="utf-8"), encoding="utf-8")


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()
    attempt_dir = Path(args.attempt_dir).resolve()
    report_root = Path(args.report_root).resolve()

    artifacts_root_fn, ensure_under_artifacts = _load_artifact_helpers(branch_root)
    artifacts_root = ensure_under_artifacts(Path(artifacts_root_fn()))
    lca_root = ensure_under_artifacts((artifacts_root / "lca_tree_stress_v5").resolve())
    ensure_under_artifacts(attempt_dir)
    ensure_under_artifacts(report_root)

    lock_root = ensure_under_artifacts((lca_root / ".locks").resolve())
    tmp_root = ensure_under_artifacts((lca_root / ".tmp").resolve())

    removed_paths: list[str] = []
    preserved_paths: list[str] = []

    removed_locks, active_locks = _cleanup_lock_root(lock_root)
    removed_paths.extend(removed_locks)

    if active_locks:
        payload = {
            "timestamp": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
            "status": "blocked_active_lock",
            "artifacts_root": str(lca_root),
            "removed_paths": removed_paths,
            "preserved_paths": preserved_paths,
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
    removed_paths.extend(_cleanup_top_level(lca_root))

    payload = {
        "timestamp": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
        "status": "ok",
        "artifacts_root": str(lca_root),
        "removed_paths": removed_paths,
        "preserved_paths": preserved_paths,
        "active_locks": [],
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
