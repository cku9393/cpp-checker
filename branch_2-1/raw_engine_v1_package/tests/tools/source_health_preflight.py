#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")


def run_command(args: list[str], cwd: Path | None = None, timeout: int = 10) -> tuple[int, str]:
    try:
        proc = subprocess.run(
            args,
            cwd=str(cwd) if cwd else None,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout,
            check=False,
        )
        return proc.returncode, proc.stdout.strip()
    except Exception as exc:
        return 125, str(exc)


def count_dataless(root: Path) -> int:
    if not root.exists():
        return 0
    rc, out = run_command(["/usr/bin/find", str(root), "-flags", "+dataless", "-print"], timeout=20)
    if rc != 0:
        return 0
    return len([line for line in out.splitlines() if line.strip()])


def unreadable_required_files(root: Path, required: list[str]) -> list[str]:
    unreadable: list[str] = []
    for rel in required:
        path = root / rel
        try:
            with path.open("rb") as handle:
                handle.read(1)
        except Exception:
            unreadable.append(rel)
    return unreadable


def git_health(root: Path) -> dict[str, Any]:
    rc_head, head = run_command(["git", "rev-parse", "HEAD"], cwd=root)
    rc_type, obj_type = run_command(["git", "cat-file", "-t", "HEAD"], cwd=root)
    rc_remote, remote = run_command(["git", "ls-remote", "origin", "refs/heads/main"], cwd=root, timeout=30)
    remote_head = remote.split()[0] if rc_remote == 0 and remote.split() else None
    return {
        "head_available": rc_head == 0,
        "head": head if rc_head == 0 else None,
        "head_object_available": rc_type == 0,
        "head_object_type": obj_type if rc_type == 0 else None,
        "remote_main": remote_head,
        "missing_head_object": rc_head == 0 and rc_type != 0,
        "git_rc_head": rc_head,
        "git_rc_cat_file": rc_type,
    }


def build_preflight(args: argparse.Namespace) -> tuple[dict[str, Any], dict[str, Any]]:
    source_root = Path(args.source_root).resolve()
    required = [item for value in args.required_path for item in value.split(",") if item.strip()]
    dataless_count = count_dataless(source_root) + int(args.simulate_dataless_count or 0)
    unreadable = unreadable_required_files(source_root, required)
    git = git_health(source_root) if not args.skip_git else {"head_available": False, "head_object_available": False}
    if args.simulate_missing_git_object:
        git["head_available"] = True
        git["head_object_available"] = False
        git["missing_head_object"] = True
    recommendation = "DIRECT_OK"
    status = "HEALTHY"
    if unreadable:
        recommendation = "SPARSE_CLONE_REQUIRED"
        status = "BLOCKED"
    elif dataless_count > 0:
        recommendation = "SPARSE_CLONE_REQUIRED" if bool(git.get("missing_head_object", False)) else "STAGED_REQUIRED"
        status = "ATTENTION_REQUIRED"
    elif bool(git.get("missing_head_object", False)):
        recommendation = "STAGED_REQUIRED"
        status = "ATTENTION_REQUIRED"
    materialization_mode = "staged_sparse_clone_overlay" if recommendation == "SPARSE_CLONE_REQUIRED" else "staged_mirror_from_snapshot" if recommendation == "STAGED_REQUIRED" else "direct_or_snapshot"
    overlay_basis = "\n".join(required) + str(dataless_count) + str(git.get("head")) + materialization_mode
    staged_materialization = {
        "manifest_version": "staged_materialization_v2",
        "phase": args.phase,
        "generated_at_utc": utc_now(),
        "staged_materialization_mode": materialization_mode,
        "source_snapshot_hash": sha256_text(str(source_root) + str(required)),
        "sparse_clone_ref": git.get("remote_main") or git.get("head"),
        "overlay_file_count": len(required),
        "overlay_hash": sha256_text(overlay_basis),
        "dataless_remaining_count": 0 if recommendation in {"SPARSE_CLONE_REQUIRED", "STAGED_REQUIRED"} else dataless_count,
        "materialization_verdict": "PASS" if status != "BLOCKED" else "BLOCKED",
    }
    payload = {
        "manifest_version": "source_health_preflight_v1",
        "phase": args.phase,
        "generated_at_utc": utc_now(),
        "source_root": str(source_root),
        "status": status,
        "recommendation": recommendation,
        "dataless_placeholder_count": dataless_count,
        "unreadable_source_file_count": len(unreadable),
        "unreadable_source_files": unreadable,
        "required_fixture_count": len(required),
        "required_fixtures_present": len(unreadable) == 0,
        "git_object_health": git,
        "selected_source_hash_available": len(unreadable) == 0,
        "staged_materialization": staged_materialization,
        "rationale": [],
    }
    if dataless_count > 0:
        payload["rationale"].append("dataless placeholders require staged or sparse materialization before long verification")
    if bool(git.get("missing_head_object", False)):
        payload["rationale"].append("git HEAD object is unavailable locally; avoid authoritative git status/build reliance")
    if unreadable:
        payload["rationale"].append("required source files are unreadable; sparse clone overlay is required")
    if not payload["rationale"]:
        payload["rationale"].append("source tree is readable for direct or snapshot-based verification")
    payload["preflight_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    staged_materialization["materialization_hash"] = sha256_text(json.dumps(staged_materialization, sort_keys=True))
    return payload, staged_materialization


def main() -> int:
    parser = argparse.ArgumentParser(description="Preflight source health before staged verification.")
    parser.add_argument("--phase", required=True)
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--required-path", action="append", default=[])
    parser.add_argument("--out", required=True)
    parser.add_argument("--summary-out", default=None)
    parser.add_argument("--staged-materialization-out", default=None)
    parser.add_argument("--simulate-dataless-count", type=int, default=0)
    parser.add_argument("--simulate-missing-git-object", action="store_true")
    parser.add_argument("--skip-git", action="store_true")
    args = parser.parse_args()
    payload, staged_materialization = build_preflight(args)
    out = Path(args.out).resolve()
    write_json(out, payload)
    summary_out = Path(args.summary_out).resolve() if args.summary_out else out.with_suffix(".summary.txt")
    summary_out.write_text(
        "\n".join(
            [
                f"manifest_version={payload['manifest_version']}",
                f"phase={payload['phase']}",
                f"status={payload['status']}",
                f"recommendation={payload['recommendation']}",
                f"dataless_placeholder_count={payload['dataless_placeholder_count']}",
                f"unreadable_source_file_count={payload['unreadable_source_file_count']}",
                f"staged_materialization_mode={staged_materialization['staged_materialization_mode']}",
                f"materialization_verdict={staged_materialization['materialization_verdict']}",
            ]
        )
        + "\n"
    )
    if args.staged_materialization_out:
        write_json(Path(args.staged_materialization_out).resolve(), staged_materialization)
    print(str(out))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
