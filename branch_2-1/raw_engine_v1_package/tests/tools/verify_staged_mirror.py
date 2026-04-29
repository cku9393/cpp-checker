#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path

from staged_verification_lib import aggregate_hash, sha256_file_local, timestamp_utc_now, write_json


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Verify a staged mirror against a source snapshot manifest.")
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--staged-root", required=True)
    parser.add_argument("--snapshot-manifest", required=True)
    parser.add_argument("--out", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    source_root = Path(args.source_root).resolve()
    staged_root = Path(args.staged_root).resolve()
    snapshot_manifest_path = Path(args.snapshot_manifest).resolve()
    out_path = Path(args.out).resolve()

    snapshot = json.loads(snapshot_manifest_path.read_text())
    comparison_entries: list[dict] = []
    hash_entries: list[tuple[str, str]] = []
    missing = 0
    mismatched = 0
    for item in snapshot.get("files", []):
        relative_path = str(item.get("relative_path", ""))
        expected_sha = str(item.get("expected_sha256") or "")
        staged_path = staged_root / relative_path
        staged_exists = staged_path.exists()
        staged_sha = sha256_file_local(staged_path) if staged_exists and staged_path.is_file() else None
        if not staged_exists:
            missing += 1
        elif expected_sha and staged_sha != expected_sha:
            mismatched += 1
        if staged_sha:
            hash_entries.append((relative_path, staged_sha))
        comparison_entries.append(
            {
                "relative_path": relative_path,
                "expected_sha256": expected_sha or None,
                "staged_sha256": staged_sha,
                "staged_exists": staged_exists,
                "source_root": str(source_root / relative_path),
                "staged_root": str(staged_path),
            }
        )

    verdict = "PASS" if missing == 0 and mismatched == 0 else "FAIL"
    payload = {
        "manifest_version": "staged_mirror_verify_v1",
        "generated_at_utc": timestamp_utc_now(),
        "source_root": str(source_root),
        "staged_root": str(staged_root),
        "source_snapshot_manifest": str(snapshot_manifest_path),
        "source_snapshot_hash": snapshot.get("snapshot_hash"),
        "staged_mirror_hash": aggregate_hash(hash_entries),
        "source_vs_staged_hash_equality": {
            "compared_file_count": len(comparison_entries),
            "missing_file_count": missing,
            "hash_mismatch_count": mismatched,
        },
        "ignored_generated_file_policy": {
            "generated_bootstrap_files": ["CMakeLists.txt"],
            "pass_through_entries_are_not_hash_compared": True,
        },
        "manifest_match_verdict": verdict,
        "publication_eligibility": "ELIGIBLE" if verdict == "PASS" else "INELIGIBLE",
        "entries": comparison_entries,
    }
    write_json(out_path, payload)
    print(out_path)
    return 0 if verdict == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
