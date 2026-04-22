#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path

from staged_verification_lib import (
    PASS_THROUGH_GLOBS,
    PASS_THROUGH_NAMES,
    PASS_THROUGH_RELATIVE_PATHS,
    STAGED_ARTIFACT_SEED_RELATIVE_PATHS,
    copy_path_with_timeout,
    copy_file_with_timeout,
    ensure_clean_dir,
    sha256_file_local,
    synthetic_root_cmakelists,
    synthetic_src_cmakelists,
    timestamp_utc_now,
    write_git_head_file,
    write_json,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Materialize a staged mirror from a source snapshot manifest.")
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--snapshot-manifest", required=True)
    parser.add_argument("--staged-root", required=True)
    parser.add_argument("--out", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    source_root = Path(args.source_root).resolve()
    snapshot_manifest_path = Path(args.snapshot_manifest).resolve()
    staged_root = Path(args.staged_root).resolve()
    out_path = Path(args.out).resolve()

    snapshot = json.loads(snapshot_manifest_path.read_text())
    ensure_clean_dir(staged_root)

    pass_through_entries: list[str] = []
    for name in PASS_THROUGH_NAMES:
        src = source_root / name
        if src.exists():
            ok, verdict = copy_path_with_timeout(src, staged_root / name)
            if ok:
                pass_through_entries.append(f"{name}:copy:{verdict}")
            else:
                pass_through_entries.append(f"{name}:copy_failed:{verdict}")
    for pattern in PASS_THROUGH_GLOBS:
        for path in sorted(source_root.glob(pattern)):
            target = staged_root / path.name
            if target.exists():
                continue
            ok, verdict = copy_path_with_timeout(path, target)
            if ok:
                pass_through_entries.append(f"{path.name}:copy:{verdict}")
            else:
                pass_through_entries.append(f"{path.name}:copy_failed:{verdict}")
    for relative_path in PASS_THROUGH_RELATIVE_PATHS:
        src = source_root / relative_path
        if not src.exists():
            continue
        target = staged_root / relative_path
        ok, verdict = copy_path_with_timeout(src, target)
        if ok:
            pass_through_entries.append(f"{relative_path}:copy:{verdict}")
        else:
            pass_through_entries.append(f"{relative_path}:copy_failed:{verdict}")

    copied_file_count = 0
    missing_file_count = 0
    hash_mismatch_count = 0
    entries: list[dict] = []

    for item in snapshot.get("files", []):
        relative_path = str(item.get("relative_path", ""))
        expected_sha = str(item.get("expected_sha256") or "")
        materialization_mode = str(item.get("materialization_mode", "copy"))
        staged_path = staged_root / relative_path
        staged_path.parent.mkdir(parents=True, exist_ok=True)

        if materialization_mode == "synthetic_bootstrap":
            staged_path.write_text(synthetic_root_cmakelists(), encoding="utf-8")
            copy_verdict = "synthetic_bootstrap"
        elif materialization_mode == "synthetic_src_bootstrap":
            staged_path.write_text(synthetic_src_cmakelists(), encoding="utf-8")
            copy_verdict = "synthetic_src_bootstrap"
        elif materialization_mode == "git_head":
            ok, copy_verdict = write_git_head_file(source_root, relative_path, staged_path)
            if not ok:
                missing_file_count += 1
        else:
            ok, copy_verdict = copy_file_with_timeout(source_root / relative_path, staged_path)
            if not ok:
                missing_file_count += 1
        staged_sha = sha256_file_local(staged_path) if staged_path.exists() and staged_path.is_file() else None
        if expected_sha and staged_sha and expected_sha != staged_sha:
            hash_mismatch_count += 1
        copied_file_count += 1 if staged_path.exists() else 0
        entries.append(
            {
                "relative_path": relative_path,
                "materialization_mode": materialization_mode,
                "copy_verdict": copy_verdict,
                "staged_exists": staged_path.exists(),
                "expected_sha256": expected_sha or None,
                "staged_sha256": staged_sha,
            }
        )

    staged_artifact_seed_entries: list[str] = []
    for relative_path in STAGED_ARTIFACT_SEED_RELATIVE_PATHS:
        src = source_root / "artifacts" / relative_path
        if not src.exists():
            staged_artifact_seed_entries.append(f"{relative_path}:missing")
            continue
        target = staged_root / "artifacts" / relative_path
        ok, verdict = copy_file_with_timeout(src, target, timeout_seconds=15)
        if ok:
            staged_artifact_seed_entries.append(f"{relative_path}:copy:{verdict}")
        else:
            staged_artifact_seed_entries.append(f"{relative_path}:copy_failed:{verdict}")

    verdict = "PASS" if missing_file_count == 0 and hash_mismatch_count == 0 else "FAIL"
    payload = {
        "manifest_version": "staged_mirror_manifest_v1",
        "generated_at_utc": timestamp_utc_now(),
        "source_snapshot_manifest": str(snapshot_manifest_path),
        "source_snapshot_hash": snapshot.get("snapshot_hash"),
        "source_root": str(source_root),
        "staged_root": str(staged_root),
        "copied_file_count": copied_file_count,
        "missing_file_count": missing_file_count,
        "hash_mismatch_count": hash_mismatch_count,
        "materialize_verdict": verdict,
        "pass_through_entries": pass_through_entries,
        "staged_artifact_seed_entries": staged_artifact_seed_entries,
        "entries": entries,
    }
    write_json(out_path, payload)
    print(out_path)
    return 0 if verdict == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
