#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path

from staged_verification_lib import hash_file_with_timeout, timestamp_utc_now, write_json


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Close out a staged verification lane and emit published snapshot metadata.")
    parser.add_argument("--phase-tag", required=True)
    parser.add_argument("--source-snapshot", required=True)
    parser.add_argument("--staged-mirror-manifest", required=True)
    parser.add_argument("--staged-mirror-verify", required=True)
    parser.add_argument("--staged-verification-release", required=True)
    parser.add_argument("--staged-verification-debug", required=True)
    parser.add_argument("--staged-verification-asan", required=True)
    parser.add_argument("--ctest-inventory-release", required=True)
    parser.add_argument("--ctest-inventory-debug", required=True)
    parser.add_argument("--ctest-inventory-asan", required=True)
    parser.add_argument("--published-root", required=True)
    parser.add_argument("--publication-health", required=True)
    parser.add_argument("--report", required=True)
    parser.add_argument("--full-bundle", required=True)
    parser.add_argument("--curated-bundle", required=True)
    parser.add_argument("--light-bundle", required=True)
    parser.add_argument("--delivery-bundle", required=True)
    parser.add_argument("--published-snapshot-out", required=True)
    parser.add_argument("--closeout-out", required=True)
    return parser.parse_args()


def read_json(path: str) -> dict:
    return json.loads(Path(path).resolve().read_text())


def sha(path: str) -> str | None:
    digest, verdict = hash_file_with_timeout(Path(path).resolve())
    return digest if verdict == "ok" else None


def main() -> int:
    args = parse_args()
    source_snapshot = read_json(args.source_snapshot)
    staged_manifest = read_json(args.staged_mirror_manifest)
    staged_verify = read_json(args.staged_mirror_verify)
    release = read_json(args.staged_verification_release)
    debug = read_json(args.staged_verification_debug)
    asan = read_json(args.staged_verification_asan)
    publication_health = read_json(args.publication_health)

    copied_artifacts = [
        args.report,
        args.full_bundle,
        args.curated_bundle,
        args.light_bundle,
        args.delivery_bundle,
        args.source_snapshot,
        args.staged_mirror_manifest,
        args.staged_mirror_verify,
        args.staged_verification_release,
        args.staged_verification_debug,
        args.staged_verification_asan,
        args.ctest_inventory_release,
        args.ctest_inventory_debug,
        args.ctest_inventory_asan,
        args.publication_health,
    ]
    published_payload = {
        "manifest_version": "published_snapshot_v1",
        "phase_tag": args.phase_tag,
        "generated_at_utc": timestamp_utc_now(),
        "source_snapshot_hash": source_snapshot.get("snapshot_hash"),
        "staged_mirror_hash": staged_verify.get("staged_mirror_hash"),
        "verification_release_hash": sha(args.staged_verification_release),
        "verification_debug_hash": sha(args.staged_verification_debug),
        "verification_asan_hash": sha(args.staged_verification_asan),
        "published_root": str(Path(args.published_root).resolve()),
        "copied_artifact_list": [str(Path(path).resolve()) for path in copied_artifacts if Path(path).exists()],
        "publication_health_verdict": publication_health.get("status"),
        "copied_report": str(Path(args.report).resolve()),
        "copied_bundles": {
            "full": str(Path(args.full_bundle).resolve()),
            "curated": str(Path(args.curated_bundle).resolve()),
            "light": str(Path(args.light_bundle).resolve()),
            "delivery": str(Path(args.delivery_bundle).resolve()),
        },
    }
    write_json(Path(args.published_snapshot_out).resolve(), published_payload)

    not_run_total = int(release.get("not_run_count", 0)) + int(debug.get("not_run_count", 0)) + int(asan.get("not_run_count", 0))
    verdict = "CLOSEOUT_PASS"
    rationale: list[str] = []
    if staged_manifest.get("materialize_verdict") != "PASS":
        verdict = "CLOSEOUT_FAIL"
        rationale.append("staged mirror materialization did not pass")
    if staged_verify.get("manifest_match_verdict") != "PASS":
        verdict = "CLOSEOUT_FAIL"
        rationale.append("staged mirror verify did not pass")
    for name, payload in (("release", release), ("debug", debug), ("asan", asan)):
        if payload.get("execution_verdict") != "PASS":
            verdict = "CLOSEOUT_FAIL"
            rationale.append(f"{name} staged verification did not pass")
    if publication_health.get("status") != "HEALTHY":
        verdict = "CLOSEOUT_FAIL"
        rationale.append("publication health is not healthy")
    if not_run_total != 0:
        verdict = "CLOSEOUT_FAIL"
        rationale.append("one or more staged verification lanes reported Not Run tests")
    if not rationale:
        rationale.append("staged mirror verification and publication closeout are healthy")

    closeout_payload = {
        "manifest_version": "verification_closeout_v1",
        "phase_tag": args.phase_tag,
        "generated_at_utc": timestamp_utc_now(),
        "closeout_verdict": verdict,
        "rationale": rationale,
        "source_snapshot_hash": source_snapshot.get("snapshot_hash"),
        "staged_mirror_hash": staged_verify.get("staged_mirror_hash"),
        "verification_release_hash": sha(args.staged_verification_release),
        "verification_debug_hash": sha(args.staged_verification_debug),
        "verification_asan_hash": sha(args.staged_verification_asan),
        "verification_not_run_count": not_run_total,
        "published_snapshot_manifest": str(Path(args.published_snapshot_out).resolve()),
        "published_root": str(Path(args.published_root).resolve()),
        "publication_health_verdict": publication_health.get("status"),
        "verification_closeout_status": verdict,
    }
    write_json(Path(args.closeout_out).resolve(), closeout_payload)
    print(Path(args.closeout_out).resolve())
    return 0 if verdict == "CLOSEOUT_PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
