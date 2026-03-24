#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build a phase evidence bundle.")
    parser.add_argument("--phase", required=True, help="Phase tag, e.g. phase18")
    parser.add_argument("--artifact-root", required=True, help="Artifact root directory")
    parser.add_argument("--report-out", required=True, help="Existing report file to include")
    parser.add_argument("--zip-out", required=True, help="Output zip path")
    parser.add_argument(
        "--policy-manifest",
        default=None,
        help="Policy manifest path (.json or .txt). Defaults to <artifact-root>/manifests/policy_gate.json",
    )
    parser.add_argument(
        "--curated-zip",
        default=None,
        help="Optional curated zip to copy into the bundle even if artifact-root/curated is empty",
    )
    return parser.parse_args()


def copy_tree_if_exists(src: Path, dst: Path) -> list[str]:
    copied: list[str] = []
    if not src.exists():
        return copied
    if src.is_file():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        copied.append(str(dst))
        return copied
    shutil.copytree(src, dst, dirs_exist_ok=True)
    for path in dst.rglob("*"):
        if path.is_file():
            copied.append(str(path))
    return copied


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[2]
    artifact_root = Path(args.artifact_root).resolve()
    report_path = Path(args.report_out).resolve()
    zip_out = Path(args.zip_out).resolve()
    manifest_path = Path(args.policy_manifest).resolve() if args.policy_manifest else (
        artifact_root / "manifests" / "policy_gate.json"
    )
    curated_zip = Path(args.curated_zip).resolve() if args.curated_zip else None

    if not report_path.exists():
        raise SystemExit(f"report file not found: {report_path}")
    if manifest_path.suffix == ".json":
        manifest_json = manifest_path
        manifest_txt = manifest_path.with_suffix(".txt")
    else:
        manifest_txt = manifest_path
        manifest_json = manifest_path.with_suffix(".json")
    manifest_summary = manifest_json.with_name(f"{manifest_json.stem}.summary.txt")

    bundle_root = artifact_root / f"{args.phase}_evidence_bundle"
    if bundle_root.exists():
        shutil.rmtree(bundle_root)

    reports_dir = bundle_root / "reports"
    manifests_dir = bundle_root / "manifests"
    curated_dir = bundle_root / "curated"
    regressions_dir = bundle_root / "regressions"
    logs_dir = bundle_root / "logs"
    for path in (reports_dir, manifests_dir, curated_dir, regressions_dir, logs_dir):
        path.mkdir(parents=True, exist_ok=True)

    copied: dict[str, list[str]] = {
        "reports": [],
        "manifests": [],
        "curated": [],
        "regressions": [],
        "logs": [],
    }

    copied["reports"].extend(copy_tree_if_exists(report_path, reports_dir / report_path.name))
    copied["manifests"].extend(copy_tree_if_exists(manifest_json, manifests_dir / manifest_json.name))
    copied["manifests"].extend(copy_tree_if_exists(manifest_txt, manifests_dir / manifest_txt.name))
    copied["manifests"].extend(copy_tree_if_exists(manifest_summary, manifests_dir / manifest_summary.name))

    for source in sorted(artifact_root.rglob("logs")):
        if source == logs_dir or not source.is_dir():
            continue
        relative = source.relative_to(artifact_root)
        copied["logs"].extend(copy_tree_if_exists(source, logs_dir / relative))

    copied["curated"].extend(copy_tree_if_exists(artifact_root / "curated", curated_dir))
    if curated_zip is not None:
        copied["curated"].extend(copy_tree_if_exists(curated_zip, curated_dir / curated_zip.name))
    copied["regressions"].extend(copy_tree_if_exists(artifact_root / "regressions", regressions_dir))
    if not copied["regressions"]:
        copied["regressions"].extend(copy_tree_if_exists(repo_root / "tests" / "regressions", regressions_dir))

    summary_line = ""
    if manifest_summary.exists():
        summary_lines = manifest_summary.read_text(encoding="utf-8").splitlines()
        summary_line = summary_lines[0] if summary_lines else ""

    metadata = {
        "phase": args.phase,
        "timestamp_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "artifact_root": str(artifact_root),
        "report": str(report_path),
        "policy_manifest_json": str(manifest_json),
        "policy_manifest_txt": str(manifest_txt),
        "policy_manifest_summary": str(manifest_summary),
        "bundle_root": str(bundle_root),
        "zip_out": str(zip_out),
        "gate_summary": summary_line,
        "copied_counts": {key: len(value) for key, value in copied.items()},
    }
    (bundle_root / "bundle_metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

    zip_out.parent.mkdir(parents=True, exist_ok=True)
    archive_base = zip_out.with_suffix("")
    temp_zip = Path(shutil.make_archive(str(archive_base), "zip", root_dir=bundle_root.parent, base_dir=bundle_root.name))
    if temp_zip != zip_out:
        shutil.move(str(temp_zip), str(zip_out))

    print(str(zip_out))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
