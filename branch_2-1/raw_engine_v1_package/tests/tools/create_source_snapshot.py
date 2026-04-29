#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from staged_verification_lib import (
    SNAPSHOT_ROOT_FILES,
    VERIFICATION_RELEVANT_FILES,
    aggregate_hash,
    build_snapshot_entry,
    iter_snapshot_files,
    timestamp_utc_now,
    write_json,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create a machine-readable source snapshot manifest for staged verification.")
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--out", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    source_root = Path(args.source_root).resolve()
    entries = [build_snapshot_entry(source_root, relative_path) for relative_path in iter_snapshot_files(source_root)]
    unreadable_entries = [entry["relative_path"] for entry in entries if not entry.get("expected_sha256")]
    valid_hash_entries = [
        (str(entry["relative_path"]), str(entry["expected_sha256"]))
        for entry in entries
        if entry.get("expected_sha256")
    ]
    snapshot_verdict = "PASS" if not unreadable_entries else "FAIL"
    payload = {
        "manifest_version": "source_snapshot_manifest_v1",
        "authoritative_root": str(source_root),
        "snapshot_timestamp_utc": timestamp_utc_now(),
        "file_count": len(entries),
        "unreadable_file_count": len(unreadable_entries),
        "unreadable_files": unreadable_entries,
        "snapshot_verdict": snapshot_verdict,
        "snapshot_hash": aggregate_hash(valid_hash_entries),
        "top_level_tracked_files": list(SNAPSHOT_ROOT_FILES),
        "verification_relevant_files": list(VERIFICATION_RELEVANT_FILES),
        "selected_file_hashes": {
            entry["relative_path"]: entry.get("expected_sha256")
            for entry in entries
            if entry["relative_path"] in VERIFICATION_RELEVANT_FILES
        },
        "files": entries,
    }
    write_json(Path(args.out).resolve(), payload)
    print(Path(args.out).resolve())
    return 0 if snapshot_verdict == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
