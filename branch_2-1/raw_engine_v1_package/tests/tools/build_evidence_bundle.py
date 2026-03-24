#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import time
import zipfile
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build a phase evidence bundle.")
    parser.add_argument("--phase", required=True, help="Phase tag, e.g. phase18")
    parser.add_argument("--artifact-root", required=True, help="Artifact root directory")
    parser.add_argument("--report-out", required=True, help="Existing report file to include")
    parser.add_argument("--zip-out", required=True, help="Output zip path")
    parser.add_argument(
        "--delivery-zip",
        default=None,
        help="Optional output zip path for a delivery package that contains the top-level report/manifests/bundles.",
    )
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
    parser.add_argument(
        "--baseline-manifest",
        default=None,
        help="Optional baseline manifest path used for freshness/hash metadata.",
    )
    parser.add_argument(
        "--current-manifest",
        default=None,
        help="Optional current manifest path used for freshness/hash metadata. Defaults to --policy-manifest.",
    )
    parser.add_argument(
        "--refresh-manifest",
        default=None,
        help="Optional refresh manifest path used for freshness/rerun metadata.",
    )
    parser.add_argument(
        "--rerun-plan",
        default=None,
        help="Optional rerun plan path (.json or .txt) to include in the bundle metadata.",
    )
    parser.add_argument(
        "--pipeline-summary",
        default=None,
        help="Optional policy pipeline summary path (.json or .txt) to include in the bundle metadata.",
    )
    parser.add_argument(
        "--pipeline-quick-summary",
        default=None,
        help="Optional quick policy pipeline summary path (.json or .txt) to include in the bundle metadata.",
    )
    parser.add_argument(
        "--pipeline-matrix-summary",
        default=None,
        help="Optional policy pipeline matrix summary path (.json or .txt) to include in the bundle metadata.",
    )
    parser.add_argument(
        "--runtime-manifest",
        default=None,
        help="Optional runtime manifest path (.json or .txt) to include in the bundle metadata.",
    )
    parser.add_argument("--runtime-baseline-manifest", default=None, help="Optional runtime baseline manifest path.")
    parser.add_argument("--runtime-current-manifest", default=None, help="Optional runtime current manifest path.")
    parser.add_argument("--runtime-refresh-manifest", default=None, help="Optional runtime refresh manifest path.")
    parser.add_argument("--runtime-rerun-plan", default=None, help="Optional runtime rerun plan path.")
    parser.add_argument("--runtime-registry", default=None, help="Optional runtime baseline registry path.")
    parser.add_argument("--runtime-history-index", default=None, help="Optional runtime history index path.")
    parser.add_argument("--runtime-proposal", default=None, help="Optional runtime rebaseline proposal path.")
    parser.add_argument("--runtime-watch-current", default=None, help="Optional runtime watch current manifest path.")
    parser.add_argument("--runtime-watch-refresh", default=None, help="Optional runtime watch refresh manifest path.")
    parser.add_argument("--runtime-watch-history-index", default=None, help="Optional runtime watch history index path.")
    parser.add_argument("--prune-artifacts", action="store_true", help="Prune retained bundle/nightly artifacts after indexing.")
    parser.add_argument("--max-bundles", type=int, default=5, help="Maximum number of root bundle zips to retain when pruning.")
    parser.add_argument(
        "--max-nightly-runs",
        type=int,
        default=5,
        help="Maximum number of nightly refresh artifact directories to retain when pruning.",
    )
    parser.add_argument(
        "--keep-approved",
        action="store_true",
        help="Retain approved baseline manifests when pruning bundle-related artifacts.",
    )
    parser.add_argument(
        "--bundle-index-out",
        default=None,
        help="Optional explicit bundle index path. Defaults to <artifact-root>/bundle_index.json.",
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


def atomic_write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = path.with_name(f".{path.name}.tmp.{os.getpid()}.{time.time_ns()}")
    temp_path.write_text(text, encoding="utf-8")
    os.replace(temp_path, path)


def latest_matching_file(root: Path, pattern: str) -> Path | None:
    matches = [path for path in root.rglob(pattern) if path.is_file()]
    if not matches:
        return None
    return max(matches, key=lambda path: path.stat().st_mtime)


def is_nested_evidence_bundle_path(path: Path) -> bool:
    return any(parent.name.endswith("_evidence_bundle") for parent in path.parents)


def resolve_manifest_json(path: Path | None) -> Path | None:
    if path is None:
        return None
    if path.suffix == ".json":
        return path
    candidate = path.with_suffix(".json")
    if candidate.exists():
        return candidate
    return path


def read_json_if_exists(path: Path | None) -> dict:
    if path is None or not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}


def sha256_file(path: Path | None) -> str | None:
    if path is None or not path.exists() or not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def stable_bundle_timestamp(report_path: Path) -> str:
    stat = report_path.stat()
    return datetime.fromtimestamp(stat.st_mtime, tz=timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def manifest_sidecars(path: Path | None) -> list[Path]:
    if path is None:
        return []
    if path.suffix == ".json":
        return [path, path.with_suffix(".txt"), path.with_name(f"{path.stem}.summary.txt")]
    if path.suffix == ".txt":
        return [path, path.with_suffix(".json"), path.with_name(f"{path.stem}.summary.txt")]
    return [path]


def parse_manifest_summary_paths(manifest_data: dict) -> list[Path]:
    paths: list[Path] = []
    seen: set[Path] = set()
    for family in manifest_data.get("families", []):
        if not isinstance(family, dict):
            continue
        source_summary = str(family.get("source_summary_path", "")).strip()
        if not source_summary:
            continue
        for token in source_summary.split(";"):
            value = token.split("=", 1)[1] if "=" in token else token
            value = value.strip()
            if not value:
                continue
            path = Path(value)
            if path.exists() and path not in seen:
                seen.add(path)
                paths.append(path)
    return paths


def copy_manifest_bundle(path: Path | None, dst_dir: Path, prefix: str) -> list[str]:
    copied: list[str] = []
    for sidecar in manifest_sidecars(path):
        if sidecar.exists():
            copied.extend(copy_tree_if_exists(sidecar, dst_dir / f"{prefix}{sidecar.name}"))
    if path is not None and path.suffix == ".json":
        approval_metadata = path.with_name(f"{path.stem}_approval_metadata.json")
        if approval_metadata.exists():
            copied.extend(copy_manifest_bundle(approval_metadata, dst_dir, f"{prefix}{approval_metadata.stem}_"))
    return copied


def collect_sorted_paths(paths: list[Path]) -> list[dict[str, object]]:
    ordered = sorted(
        [path for path in paths if path.exists()],
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    return [
        {
            "path": str(path),
            "mtime_epoch": path.stat().st_mtime,
            "is_dir": path.is_dir(),
            "size_bytes": path.stat().st_size if path.is_file() else 0,
        }
        for path in ordered
    ]


def collect_bundle_paths(root: Path, pattern: str) -> list[Path]:
    return [path for path in root.glob(pattern) if path.is_file()]


def default_delivery_zip(zip_out: Path) -> Path:
    dated_match = re.match(r"^(?P<prefix>.+)_(?P<date>20\d{6})$", zip_out.stem)
    if dated_match:
        return zip_out.with_name(
            f"{dated_match.group('prefix')}_delivery_{dated_match.group('date')}{zip_out.suffix}"
        )
    return zip_out.with_name(f"{zip_out.stem}_delivery{zip_out.suffix}")


def build_delivery_entries(
    report_path: Path,
    current_manifest: Path | None,
    baseline_manifest: Path | None,
    refresh_manifest: Path | None,
    rerun_plan: Path | None,
    pipeline_summary: Path | None,
    pipeline_quick_summary: Path | None,
    pipeline_matrix_summary: Path | None,
    runtime_baseline_manifest: Path | None,
    runtime_current_manifest: Path | None,
    runtime_refresh_manifest: Path | None,
    runtime_rerun_plan: Path | None,
    runtime_registry: Path | None,
    runtime_history_index: Path | None,
    runtime_proposal: Path | None,
    runtime_watch_current: Path | None,
    runtime_watch_refresh: Path | None,
    runtime_watch_history_index: Path | None,
    bundle_metadata: Path,
    zip_out: Path,
    curated_zip: Path | None,
) -> list[tuple[str, Path]]:
    entries: list[tuple[str, Path]] = [("report", report_path)]
    optional_entries = [
        ("current_manifest", current_manifest),
        ("approved_baseline", baseline_manifest),
        ("refresh_manifest", refresh_manifest),
        ("rerun_plan", rerun_plan),
        ("nightly_pipeline_summary", pipeline_summary),
        ("quick_pipeline_summary", pipeline_quick_summary),
        ("pipeline_matrix_summary", pipeline_matrix_summary),
        ("runtime_baseline_manifest", runtime_baseline_manifest),
        ("runtime_current_manifest", runtime_current_manifest),
        ("runtime_refresh_manifest", runtime_refresh_manifest),
        ("runtime_rerun_plan", runtime_rerun_plan),
        ("runtime_registry", runtime_registry),
        ("runtime_history_index", runtime_history_index),
        ("runtime_proposal", runtime_proposal),
        ("runtime_watch_current", runtime_watch_current),
        ("runtime_watch_refresh", runtime_watch_refresh),
        ("runtime_watch_history_index", runtime_watch_history_index),
        ("bundle_metadata", bundle_metadata),
        ("bundle_zip", zip_out),
        ("curated_zip", curated_zip),
    ]
    for label, path in optional_entries:
        if path is not None and path.exists():
            entries.append((label, path))
    return entries


def create_delivery_zip(delivery_zip: Path, entries: list[tuple[str, Path]]) -> None:
    delivery_zip.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(delivery_zip, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for label, path in entries:
            archive.write(path, arcname=f"{label}/{path.name}")


def collect_nightly_run_paths(artifact_root: Path) -> list[Path]:
    roots: list[Path] = []
    if artifact_root.exists():
        roots.extend(path for path in artifact_root.iterdir() if path.is_dir() and "nightly" in path.name)
        nightly_runs_root = artifact_root / "nightly_runs"
        if nightly_runs_root.exists():
            roots.extend(path for path in nightly_runs_root.iterdir() if path.exists())
        refresh_root = artifact_root / "nightly_reruns"
        if refresh_root.exists():
            roots.extend(path for path in refresh_root.iterdir() if path.is_dir())
    return roots


def update_indexes(
    repo_root: Path,
    artifact_root: Path,
    bundle_root: Path,
    zip_out: Path,
    delivery_zip: Path | None,
    curated_zip: Path | None,
    pipeline_summary: Path | None,
    pipeline_quick_summary: Path | None,
    runtime_manifest: Path | None,
    runtime_registry: Path | None,
    runtime_history_index: Path | None,
    runtime_proposal: Path | None,
    bundle_index_out: Path | None,
) -> tuple[Path, Path]:
    index_dir = (bundle_index_out.parent if bundle_index_out is not None else artifact_root / "indexes")
    index_dir.mkdir(parents=True, exist_ok=True)
    bundle_parent_root = zip_out.parent

    reports = collect_sorted_paths(
        list(bundle_root.glob("reports/*")) + list(repo_root.glob("PHASE*_STABILIZATION_REPORT*.txt"))
    )
    manifests = collect_sorted_paths(
        [path for path in (artifact_root / "manifests").rglob("*.json")]
        + [path for path in (artifact_root / "manifests").rglob("*.txt")]
    )
    bundles = collect_sorted_paths(
        [
            path
            for path in collect_bundle_paths(bundle_parent_root, "raw_engine_phase*_stabilization*.zip")
            if "_curated" not in path.name and "_delivery" not in path.name
        ]
    )
    delivery_bundles = collect_sorted_paths(collect_bundle_paths(bundle_parent_root, "raw_engine_phase*_delivery*.zip"))
    curated_bundles = collect_sorted_paths(
        collect_bundle_paths(bundle_parent_root, "raw_engine_phase*_stabilization_curated*.zip")
    )
    regressions = collect_sorted_paths([artifact_root / "regressions"])
    logs = collect_sorted_paths([path for path in artifact_root.rglob("logs") if path.is_dir() and not is_nested_evidence_bundle_path(path)])
    checkpoints = collect_sorted_paths([path for path in artifact_root.rglob("checkpoints") if path.is_dir()])
    nightly_runs = collect_sorted_paths(collect_nightly_run_paths(artifact_root))
    pipeline_summaries = collect_sorted_paths(
        [path for path in (pipeline_summary, pipeline_quick_summary) if path is not None]
    )
    runtime_manifests = collect_sorted_paths([runtime_manifest] if runtime_manifest is not None else [])
    runtime_registries = collect_sorted_paths([runtime_registry] if runtime_registry is not None else [])
    runtime_histories = collect_sorted_paths([runtime_history_index] if runtime_history_index is not None else [])
    runtime_proposals = collect_sorted_paths([runtime_proposal] if runtime_proposal is not None else [])
    evidence_bundles = collect_sorted_paths([path for path in artifact_root.glob("phase*_evidence_bundle") if path.is_dir()])
    approved_baselines = collect_sorted_paths(list((artifact_root / "manifests").glob("policy_graduation_manifest_phase*_approved_v1.json")))

    artifact_index = {
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "artifact_root": str(artifact_root),
        "reports": reports,
        "manifests": manifests,
        "bundles": bundles,
        "delivery_bundles": delivery_bundles,
        "curated_bundles": curated_bundles,
        "evidence_bundles": evidence_bundles,
        "regressions": regressions,
        "logs": logs,
        "checkpoints": checkpoints,
        "nightly_runs": nightly_runs,
        "pipeline_summaries": pipeline_summaries,
        "runtime_manifests": runtime_manifests,
        "runtime_registries": runtime_registries,
        "runtime_histories": runtime_histories,
        "runtime_proposals": runtime_proposals,
        "approved_baselines": approved_baselines,
    }
    artifact_index_json = index_dir / "artifact_index.json"
    artifact_index_txt = index_dir / "artifact_index.txt"
    artifact_index_json.write_text(json.dumps(artifact_index, indent=2) + "\n", encoding="utf-8")
    artifact_index_txt.write_text(
        "\n".join(
            [
                f"artifact_root={artifact_root}",
                f"report_count={len(reports)}",
                f"manifest_count={len(manifests)}",
                f"bundle_count={len(bundles)}",
                f"delivery_bundle_count={len(delivery_bundles)}",
                f"curated_bundle_count={len(curated_bundles)}",
                f"evidence_bundle_count={len(evidence_bundles)}",
                f"nightly_run_count={len(nightly_runs)}",
                f"approved_baseline_count={len(approved_baselines)}",
            ]
        )
        + "\n",
        encoding="utf-8",
    )

    bundle_index = {
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "latest_bundle": str(zip_out),
        "latest_delivery_bundle": None if delivery_zip is None else str(delivery_zip),
        "latest_curated_bundle": None if curated_zip is None else str(curated_zip),
        "bundle_root": str(bundle_root),
        "bundles": bundles,
        "delivery_bundles": delivery_bundles,
        "curated_bundles": curated_bundles,
        "pipeline_summary": None if pipeline_summary is None else str(pipeline_summary),
        "runtime_manifest": None if runtime_manifest is None else str(runtime_manifest),
        "runtime_registry": None if runtime_registry is None else str(runtime_registry),
        "runtime_history_index": None if runtime_history_index is None else str(runtime_history_index),
        "runtime_proposal": None if runtime_proposal is None else str(runtime_proposal),
    }
    bundle_index_json = bundle_index_out if bundle_index_out is not None else index_dir / "bundle_index.json"
    bundle_index_txt = index_dir / "bundle_index.txt"
    bundle_index_json.write_text(json.dumps(bundle_index, indent=2) + "\n", encoding="utf-8")
    bundle_index_txt.write_text(
        "\n".join(
            [
                f"latest_bundle={bundle_index['latest_bundle']}",
                f"latest_delivery_bundle={bundle_index['latest_delivery_bundle']}",
                f"latest_curated_bundle={bundle_index['latest_curated_bundle']}",
                f"bundle_count={len(bundles)}",
                f"delivery_bundle_count={len(delivery_bundles)}",
                f"curated_bundle_count={len(curated_bundles)}",
            ]
        )
        + "\n",
        encoding="utf-8",
    )
    return artifact_index_json, bundle_index_json


def prune_retained_artifacts(
    bundle_parent_root: Path,
    artifact_root: Path,
    max_bundles: int,
    max_nightly_runs: int,
    keep_approved: bool,
) -> dict[str, list[str]]:
    pruned: dict[str, list[str]] = {
        "bundles": [],
        "delivery_bundles": [],
        "curated_bundles": [],
        "evidence_bundles": [],
        "nightly_runs": [],
    }
    bundle_candidates = sorted(
        [
            path
            for path in bundle_parent_root.glob("raw_engine_phase*_stabilization*.zip")
            if path.is_file() and "_curated" not in path.name and "_delivery" not in path.name
        ],
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    for path in bundle_candidates[max_bundles:]:
        path.unlink(missing_ok=True)
        pruned["bundles"].append(str(path))

    delivery_candidates = sorted(
        [path for path in bundle_parent_root.glob("raw_engine_phase*_delivery*.zip") if path.is_file()],
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    for path in delivery_candidates[max_bundles:]:
        path.unlink(missing_ok=True)
        pruned["delivery_bundles"].append(str(path))

    curated_candidates = sorted(
        [path for path in bundle_parent_root.glob("raw_engine_phase*_stabilization_curated*.zip") if path.is_file()],
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    for path in curated_candidates[max_bundles:]:
        path.unlink(missing_ok=True)
        pruned["curated_bundles"].append(str(path))

    bundle_dirs = sorted(
        [path for path in artifact_root.glob("phase*_evidence_bundle") if path.is_dir()],
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    for path in bundle_dirs[max_bundles:]:
        shutil.rmtree(path, ignore_errors=True)
        pruned["evidence_bundles"].append(str(path))

    nightly_candidates = sorted(
        collect_nightly_run_paths(artifact_root),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    for path in nightly_candidates[max_nightly_runs:]:
        shutil.rmtree(path, ignore_errors=True)
        pruned["nightly_runs"].append(str(path))

    if keep_approved:
        approved_dir = artifact_root / "manifests"
        approved_dir.mkdir(parents=True, exist_ok=True)
        # Approved baselines are intentionally left untouched; this branch documents the retention policy explicitly.
    return pruned


def infer_default_freshness_status(baseline_hash: str | None, current_hash: str | None) -> str:
    if baseline_hash and current_hash:
        return "FRESH" if baseline_hash == current_hash else "STALE"
    return "UNKNOWN"


def summarize_manifest_freshness(
    manifest_data: dict,
    baseline_hash: str | None,
    current_hash: str | None,
) -> tuple[dict, list[dict[str, object]], list[str], list[str]]:
    families = manifest_data.get("families", [])
    default_freshness = infer_default_freshness_status(baseline_hash, current_hash)
    family_summaries: list[dict[str, object]] = []
    drift_flags: list[str] = []
    reclassification_needed: list[str] = []
    freshness_counts: Counter[str] = Counter()

    for family in families:
        if not isinstance(family, dict):
            continue
        family_name = str(family.get("family", ""))
        current_status = str(family.get("current_status", family.get("status", "")))
        freshness_status = str(family.get("freshness_status", default_freshness))
        drift_flag = bool(family.get("drift_flag", family.get("drift_detected", False)))
        reclassify_required = bool(
            family.get("reclassify_required", family.get("reclassification_required", False))
        )
        counts_as_production_evidence = bool(family.get("counts_as_production_evidence", False))

        freshness_counts[freshness_status] += 1
        if drift_flag and family_name:
            drift_flags.append(family_name)
        if reclassify_required and family_name:
            reclassification_needed.append(family_name)

        family_summaries.append(
            {
                "family": family_name,
                "current_status": current_status,
                "freshness_status": freshness_status,
                "drift_flag": drift_flag,
                "reclassify_required": reclassify_required,
                "counts_as_production_evidence": counts_as_production_evidence,
            }
        )

    freshness_summary = {
        "default_freshness_status": default_freshness,
        "manifest_freshness_status": manifest_data.get("freshness_status"),
        "manifest_freshness_verdict": manifest_data.get("freshness_verdict"),
        "family_status_counts": dict(freshness_counts),
        "stale_family_count": manifest_data.get(
            "stale_family_count",
            freshness_counts.get("STALE", 0) + freshness_counts.get("REQUIRES_RERUN", 0),
        ),
        "revalidated_family_count": manifest_data.get("revalidated_family_count"),
        "reclassify_required_count": manifest_data.get(
            "reclassify_required_count",
            len(reclassification_needed),
        ),
    }
    return freshness_summary, family_summaries, drift_flags, reclassification_needed


def summarize_family_statuses(manifest_data: dict) -> list[dict[str, object]]:
    family_summaries: list[dict[str, object]] = []
    for family in manifest_data.get("families", []):
        if not isinstance(family, dict):
            continue
        family_summaries.append(
            {
                "family": str(family.get("family", "")),
                "current_status": str(family.get("current_status", family.get("status", ""))),
                "freshness_status": str(family.get("freshness_status", "")),
                "counts_as_production_evidence": bool(family.get("counts_as_production_evidence", False)),
            }
        )
    return family_summaries


def extract_refresh_rollup(refresh_manifest_data: dict) -> dict[str, int | str | None]:
    families = refresh_manifest_data.get("families", [])
    stale_family_count = 0
    requires_rerun_family_count = 0
    reclassify_required_count = 0
    for family in families:
        if not isinstance(family, dict):
            continue
        freshness_status = str(family.get("freshness_status", ""))
        if freshness_status == "STALE":
            stale_family_count += 1
        if freshness_status == "REQUIRES_RERUN":
            requires_rerun_family_count += 1
        if bool(family.get("reclassify_required", family.get("reclassification_required", False))):
            reclassify_required_count += 1
    return {
        "freshness_verdict": refresh_manifest_data.get("freshness_verdict"),
        "current_verdict": refresh_manifest_data.get("current_verdict"),
        "stale_family_count": int(refresh_manifest_data.get("stale_family_count", stale_family_count)),
        "requires_rerun_family_count": int(
            refresh_manifest_data.get("requires_rerun_family_count", requires_rerun_family_count)
        ),
        "reclassify_required_count": int(
            refresh_manifest_data.get("reclassify_required_count", reclassify_required_count)
        ),
    }


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[2]
    artifact_root = Path(args.artifact_root).resolve()
    report_path = Path(args.report_out).resolve()
    zip_out = Path(args.zip_out).resolve()
    delivery_zip = Path(args.delivery_zip).resolve() if args.delivery_zip else default_delivery_zip(zip_out)
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
    current_manifest = resolve_manifest_json(
        Path(args.current_manifest).resolve() if args.current_manifest else manifest_json
    )
    refresh_manifest = resolve_manifest_json(
        Path(args.refresh_manifest).resolve() if args.refresh_manifest else None
    )
    if refresh_manifest is None and current_manifest is not None and current_manifest != manifest_json:
        stem = current_manifest.stem.lower()
        if "refresh" in stem:
            refresh_manifest = current_manifest
            current_manifest = manifest_json
    baseline_manifest = resolve_manifest_json(
        Path(args.baseline_manifest).resolve() if args.baseline_manifest else current_manifest
    )
    rerun_plan = resolve_manifest_json(Path(args.rerun_plan).resolve() if args.rerun_plan else None)
    pipeline_summary = resolve_manifest_json(Path(args.pipeline_summary).resolve() if args.pipeline_summary else None)
    pipeline_quick_summary = resolve_manifest_json(
        Path(args.pipeline_quick_summary).resolve() if args.pipeline_quick_summary else None
    )
    pipeline_matrix_summary = resolve_manifest_json(
        Path(args.pipeline_matrix_summary).resolve() if args.pipeline_matrix_summary else None
    )
    runtime_manifest = resolve_manifest_json(Path(args.runtime_manifest).resolve() if args.runtime_manifest else None)
    runtime_current_manifest = resolve_manifest_json(
        Path(args.runtime_current_manifest).resolve() if args.runtime_current_manifest else runtime_manifest
    )
    runtime_baseline_manifest = resolve_manifest_json(
        Path(args.runtime_baseline_manifest).resolve() if args.runtime_baseline_manifest else None
    )
    runtime_refresh_manifest = resolve_manifest_json(
        Path(args.runtime_refresh_manifest).resolve() if args.runtime_refresh_manifest else None
    )
    runtime_rerun_plan = resolve_manifest_json(
        Path(args.runtime_rerun_plan).resolve() if args.runtime_rerun_plan else None
    )
    runtime_registry = resolve_manifest_json(Path(args.runtime_registry).resolve() if args.runtime_registry else None)
    runtime_history_index = resolve_manifest_json(
        Path(args.runtime_history_index).resolve() if args.runtime_history_index else None
    )
    runtime_proposal = resolve_manifest_json(Path(args.runtime_proposal).resolve() if args.runtime_proposal else None)
    runtime_watch_current = resolve_manifest_json(
        Path(args.runtime_watch_current).resolve() if args.runtime_watch_current else None
    )
    runtime_watch_refresh = resolve_manifest_json(
        Path(args.runtime_watch_refresh).resolve() if args.runtime_watch_refresh else None
    )
    runtime_watch_history_index = resolve_manifest_json(
        Path(args.runtime_watch_history_index).resolve() if args.runtime_watch_history_index else None
    )
    bundle_index_out = Path(args.bundle_index_out).resolve() if args.bundle_index_out else None
    current_manifest_data = read_json_if_exists(current_manifest)
    refresh_manifest_data = read_json_if_exists(refresh_manifest)
    pipeline_summary_data = read_json_if_exists(pipeline_summary)
    pipeline_quick_summary_data = read_json_if_exists(pipeline_quick_summary)
    pipeline_matrix_summary_data = read_json_if_exists(pipeline_matrix_summary)
    runtime_manifest_data = read_json_if_exists(runtime_current_manifest or runtime_manifest)
    runtime_baseline_data = read_json_if_exists(runtime_baseline_manifest)
    runtime_refresh_data = read_json_if_exists(runtime_refresh_manifest)
    runtime_rerun_plan_data = read_json_if_exists(runtime_rerun_plan)
    runtime_registry_data = read_json_if_exists(runtime_registry)
    runtime_history_summary_manifest = (
        runtime_history_index.with_name(f"{runtime_history_index.stem}_summary.json")
        if runtime_history_index is not None
        else None
    )
    runtime_history_data = read_json_if_exists(runtime_history_summary_manifest)
    runtime_proposal_data = read_json_if_exists(runtime_proposal)
    runtime_watch_current_data = read_json_if_exists(runtime_watch_current)
    runtime_watch_refresh_data = read_json_if_exists(runtime_watch_refresh)
    runtime_watch_history_summary_manifest = (
        runtime_watch_history_index.with_name(f"{runtime_watch_history_index.stem}_summary.json")
        if runtime_watch_history_index is not None
        else None
    )
    runtime_watch_history_data = read_json_if_exists(runtime_watch_history_summary_manifest)
    runtime_approval_metadata_path = (
        runtime_baseline_manifest.with_name(f"{runtime_baseline_manifest.stem}_approval_metadata.json")
        if runtime_baseline_manifest is not None
        else None
    )
    runtime_approval_metadata = read_json_if_exists(runtime_approval_metadata_path)

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
    for path in manifest_sidecars(manifest_json):
        copied["manifests"].extend(copy_tree_if_exists(path, manifests_dir / path.name))
    if baseline_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(baseline_manifest, manifests_dir, "baseline_"))
    if current_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_manifest, manifests_dir, "current_"))
    if refresh_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(refresh_manifest, manifests_dir, "refresh_"))
    if rerun_plan is not None:
        copied["manifests"].extend(copy_manifest_bundle(rerun_plan, manifests_dir, "rerun_"))
    if pipeline_summary is not None:
        copied["manifests"].extend(copy_manifest_bundle(pipeline_summary, manifests_dir, "pipeline_"))
    if pipeline_quick_summary is not None:
        copied["manifests"].extend(copy_manifest_bundle(pipeline_quick_summary, manifests_dir, "pipeline_quick_"))
    if pipeline_matrix_summary is not None:
        copied["manifests"].extend(copy_manifest_bundle(pipeline_matrix_summary, manifests_dir, "pipeline_matrix_"))
    if runtime_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_manifest, manifests_dir, "runtime_legacy_"))
    if runtime_baseline_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_baseline_manifest, manifests_dir, "runtime_baseline_"))
    if runtime_current_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_current_manifest, manifests_dir, "runtime_current_"))
    if runtime_refresh_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_refresh_manifest, manifests_dir, "runtime_refresh_"))
    if runtime_rerun_plan is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_rerun_plan, manifests_dir, "runtime_rerun_"))
    if runtime_registry is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_registry, manifests_dir, "runtime_registry_"))
    if runtime_history_index is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_history_index, manifests_dir, "runtime_history_"))
        history_summary = runtime_history_index.with_name(f"{runtime_history_index.stem}_summary.json")
        copied["manifests"].extend(copy_manifest_bundle(history_summary, manifests_dir, "runtime_history_summary_"))
    if runtime_proposal is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_proposal, manifests_dir, "runtime_proposal_"))
    if runtime_watch_current is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_watch_current, manifests_dir, "runtime_watch_current_"))
        runtime_budget_profile = read_json_if_exists(runtime_watch_current).get("runtime_budget_profile_path")
        if runtime_budget_profile:
            copied["manifests"].extend(copy_manifest_bundle(resolve_manifest_json(Path(str(runtime_budget_profile))), manifests_dir, "runtime_budget_profile_"))
    if runtime_watch_refresh is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_watch_refresh, manifests_dir, "runtime_watch_refresh_"))
    if runtime_watch_history_index is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_watch_history_index, manifests_dir, "runtime_watch_history_"))
        watch_history_summary = runtime_watch_history_index.with_name(f"{runtime_watch_history_index.stem}_summary.json")
        copied["manifests"].extend(copy_manifest_bundle(watch_history_summary, manifests_dir, "runtime_watch_history_summary_"))
    if runtime_approval_metadata_path is not None and runtime_approval_metadata_path.exists():
        copied["manifests"].extend(copy_manifest_bundle(runtime_approval_metadata_path, manifests_dir, "runtime_approval_"))

    for source in sorted(artifact_root.rglob("logs")):
        if source == logs_dir or not source.is_dir():
            continue
        if is_nested_evidence_bundle_path(source):
            continue
        relative = source.relative_to(artifact_root)
        copied["logs"].extend(copy_tree_if_exists(source, logs_dir / relative))

    copied["curated"].extend(copy_tree_if_exists(artifact_root / "curated", curated_dir))
    copied["curated"].extend(copy_tree_if_exists(manifest_summary, curated_dir / manifest_summary.name))
    if baseline_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(baseline_manifest, curated_dir, "baseline_"))
    if current_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(current_manifest, curated_dir, "current_"))
    if refresh_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(refresh_manifest, curated_dir, "refresh_"))
    if rerun_plan is not None:
        copied["curated"].extend(copy_manifest_bundle(rerun_plan, curated_dir, "rerun_"))
    if pipeline_summary is not None:
        copied["curated"].extend(copy_manifest_bundle(pipeline_summary, curated_dir, "pipeline_"))
    if pipeline_quick_summary is not None:
        copied["curated"].extend(copy_manifest_bundle(pipeline_quick_summary, curated_dir, "pipeline_quick_"))
    if pipeline_matrix_summary is not None:
        copied["curated"].extend(copy_manifest_bundle(pipeline_matrix_summary, curated_dir, "pipeline_matrix_"))
    if runtime_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_manifest, curated_dir, "runtime_legacy_"))
    if runtime_baseline_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_baseline_manifest, curated_dir, "runtime_baseline_"))
    if runtime_current_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_current_manifest, curated_dir, "runtime_current_"))
    if runtime_refresh_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_refresh_manifest, curated_dir, "runtime_refresh_"))
    if runtime_rerun_plan is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_rerun_plan, curated_dir, "runtime_rerun_"))
    if runtime_registry is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_registry, curated_dir, "runtime_registry_"))
    if runtime_history_index is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_history_index, curated_dir, "runtime_history_"))
        history_summary = runtime_history_index.with_name(f"{runtime_history_index.stem}_summary.json")
        copied["curated"].extend(copy_manifest_bundle(history_summary, curated_dir, "runtime_history_summary_"))
    if runtime_proposal is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_proposal, curated_dir, "runtime_proposal_"))
    if runtime_watch_current is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_watch_current, curated_dir, "runtime_watch_current_"))
        runtime_budget_profile = read_json_if_exists(runtime_watch_current).get("runtime_budget_profile_path")
        if runtime_budget_profile:
            copied["curated"].extend(copy_manifest_bundle(resolve_manifest_json(Path(str(runtime_budget_profile))), curated_dir, "runtime_budget_profile_"))
    if runtime_watch_refresh is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_watch_refresh, curated_dir, "runtime_watch_refresh_"))
    if runtime_watch_history_index is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_watch_history_index, curated_dir, "runtime_watch_history_"))
        watch_history_summary = runtime_watch_history_index.with_name(f"{runtime_watch_history_index.stem}_summary.json")
        copied["curated"].extend(copy_manifest_bundle(watch_history_summary, curated_dir, "runtime_watch_history_summary_"))
    if runtime_approval_metadata_path is not None and runtime_approval_metadata_path.exists():
        copied["curated"].extend(copy_manifest_bundle(runtime_approval_metadata_path, curated_dir, "runtime_approval_"))
    refresh_summary = None
    if refresh_manifest is not None and refresh_manifest.suffix == ".json":
        refresh_summary = refresh_manifest.with_name(f"{refresh_manifest.stem}.summary.txt")
        copied["curated"].extend(copy_tree_if_exists(refresh_summary, curated_dir / refresh_summary.name))
    manifest_linked_summaries = parse_manifest_summary_paths(current_manifest_data)
    if not manifest_linked_summaries:
        applicability_summary = latest_matching_file(
            artifact_root, "planner_tie_mixed_organic_applicability_audit.summary.txt"
        )
        compare_ready_summary = latest_matching_file(artifact_root, "compare_ready_lineage_audit.summary.txt")
        manifest_linked_summaries = [
            path for path in (applicability_summary, compare_ready_summary) if path is not None
        ]
    for summary_path in manifest_linked_summaries:
        copied["curated"].extend(copy_tree_if_exists(summary_path, curated_dir / summary_path.name))
    copied["regressions"].extend(copy_tree_if_exists(artifact_root / "regressions", regressions_dir))
    if not copied["regressions"]:
        copied["regressions"].extend(copy_tree_if_exists(repo_root / "tests" / "regressions", regressions_dir))

    summary_line = ""
    if manifest_summary.exists():
        summary_lines = manifest_summary.read_text(encoding="utf-8").splitlines()
        summary_line = summary_lines[0] if summary_lines else ""

    baseline_manifest_hash = sha256_file(baseline_manifest)
    current_manifest_hash = sha256_file(current_manifest)
    refresh_manifest_hash = sha256_file(refresh_manifest)
    rerun_plan_hash = sha256_file(rerun_plan)
    freshness_summary, family_freshness, drift_flags, reclassification_needed = summarize_manifest_freshness(
        refresh_manifest_data if refresh_manifest_data else current_manifest_data,
        baseline_manifest_hash,
        current_manifest_hash,
    )
    refresh_rollup = extract_refresh_rollup(refresh_manifest_data) if refresh_manifest_data else {}
    family_status_summary = summarize_family_statuses(current_manifest_data)
    current_verdict = str(current_manifest_data.get("current_verdict", "UNKNOWN"))
    freshness_verdict = str(
        refresh_rollup.get("freshness_verdict")
        or refresh_manifest_data.get("freshness_verdict")
        or current_manifest_data.get("freshness_verdict")
        or "UNKNOWN"
    )
    stale_family_count = int(refresh_rollup.get("stale_family_count", freshness_summary.get("stale_family_count", 0)))
    requires_rerun_family_count = int(refresh_rollup.get("requires_rerun_family_count", 0))
    reclassify_required_count = int(
        refresh_rollup.get("reclassify_required_count", freshness_summary.get("reclassify_required_count", 0))
    )
    pipeline_severity = str(pipeline_summary_data.get("severity", ""))
    if stale_family_count == 0 and requires_rerun_family_count == 0 and reclassify_required_count == 0:
        policy_summary = (
            "exact_shadow policy lifecycle healthy: no stale families, no rerun-required families, "
            "no reclassification required"
        )
        if pipeline_severity == "OK":
            policy_summary += "; pipeline severity=OK"
    else:
        policy_summary = (
            "exact_shadow policy lifecycle requires attention: "
            f"stale={stale_family_count}, requires_rerun={requires_rerun_family_count}, "
            f"reclassify_required={reclassify_required_count}"
        )
        if pipeline_severity:
            policy_summary += f"; pipeline severity={pipeline_severity}"
    atomic_write_text(bundle_root / "policy_summary.txt", policy_summary + "\n")
    atomic_write_text(curated_dir / "policy_summary.txt", policy_summary + "\n")

    runtime_watch_transition_summary = pipeline_summary_data.get("runtime_watch_transition_summary") or {
        "transition_count": runtime_watch_history_data.get("transition_count", 0),
        "watch_transition_counts": runtime_watch_history_data.get("watch_transition_counts", {}),
        "recent_transitions": runtime_watch_history_data.get("recent_transitions", []),
        "strongest_watch_status": runtime_watch_history_data.get("strongest_watch_status"),
    }
    runtime_watch_matrix_summary = {
        "matrix_entry_count": pipeline_matrix_summary_data.get("matrix_entry_count", 0),
        "matrix_watch_status_counts": pipeline_matrix_summary_data.get("matrix_watch_status_counts", {}),
        "matrix_watch_recommendation_counts": pipeline_matrix_summary_data.get("matrix_watch_recommendation_counts", {}),
        "matrix_action_counts": pipeline_matrix_summary_data.get("matrix_action_counts", {}),
        "matrix_severity_counts": pipeline_matrix_summary_data.get("matrix_severity_counts", {}),
    }
    runtime_watch_fingerprint_count = max(
        int(runtime_watch_history_data.get("fingerprint_count", 0)),
        int(runtime_watch_matrix_summary.get("matrix_entry_count", 0)),
    )

    metadata = {
        "phase": args.phase,
        "timestamp_utc": stable_bundle_timestamp(report_path),
        "artifact_root": str(artifact_root),
        "report": str(report_path),
        "policy_manifest_json": str(manifest_json),
        "policy_manifest_txt": str(manifest_txt),
        "policy_manifest_summary": str(manifest_summary),
        "bundle_root": str(bundle_root),
        "zip_out": str(zip_out),
        "delivery_zip": str(delivery_zip),
        "gate_summary": summary_line,
        "policy_summary": policy_summary,
        "current_verdict": current_verdict,
        "freshness_verdict": freshness_verdict,
        "gate_verdict": current_verdict,
        "baseline_manifest": str(baseline_manifest) if baseline_manifest is not None else None,
        "current_manifest": str(current_manifest) if current_manifest is not None else None,
        "refresh_manifest": str(refresh_manifest) if refresh_manifest is not None else None,
        "rerun_plan": str(rerun_plan) if rerun_plan is not None else None,
        "pipeline_summary": str(pipeline_summary) if pipeline_summary is not None else None,
        "pipeline_quick_summary": str(pipeline_quick_summary) if pipeline_quick_summary is not None else None,
        "pipeline_matrix_summary": str(pipeline_matrix_summary) if pipeline_matrix_summary is not None else None,
        "runtime_manifest": str(runtime_manifest) if runtime_manifest is not None else None,
        "runtime_baseline_manifest": str(runtime_baseline_manifest) if runtime_baseline_manifest is not None else None,
        "runtime_current_manifest": str(runtime_current_manifest) if runtime_current_manifest is not None else None,
        "runtime_refresh_manifest": str(runtime_refresh_manifest) if runtime_refresh_manifest is not None else None,
        "runtime_rerun_plan": str(runtime_rerun_plan) if runtime_rerun_plan is not None else None,
        "runtime_registry": str(runtime_registry) if runtime_registry is not None else None,
        "runtime_history_index": str(runtime_history_index) if runtime_history_index is not None else None,
        "runtime_proposal": str(runtime_proposal) if runtime_proposal is not None else None,
        "runtime_watch_current": str(runtime_watch_current) if runtime_watch_current is not None else None,
        "runtime_watch_refresh": str(runtime_watch_refresh) if runtime_watch_refresh is not None else None,
        "runtime_watch_history_index": str(runtime_watch_history_index) if runtime_watch_history_index is not None else None,
        "runtime_approval_metadata": str(runtime_approval_metadata_path) if runtime_approval_metadata_path is not None and runtime_approval_metadata_path.exists() else None,
        "runtime_host_fingerprint": runtime_manifest_data.get("host_fingerprint") or runtime_refresh_data.get("host_fingerprint"),
        "runtime_toolchain_fingerprint": runtime_manifest_data.get("toolchain_fingerprint") or runtime_refresh_data.get("toolchain_fingerprint"),
        "runtime_current_verdict": pipeline_summary_data.get("runtime_current_verdict")
        or runtime_refresh_data.get("current_verdict")
        or runtime_manifest_data.get("current_verdict"),
        "runtime_freshness_verdict": pipeline_summary_data.get("runtime_freshness_verdict")
        or runtime_refresh_data.get("freshness_verdict"),
        "runtime_comparability_verdict": pipeline_summary_data.get("runtime_comparability_verdict")
        or runtime_refresh_data.get("comparability_verdict"),
        "runtime_budget_verdict": pipeline_summary_data.get("runtime_budget_verdict")
        or runtime_refresh_data.get("overall_budget_verdict")
        or runtime_manifest_data.get("overall_budget_verdict"),
        "selected_runtime_baseline_id": pipeline_summary_data.get("runtime_selected_baseline_id")
        or runtime_refresh_data.get("selected_baseline_id"),
        "selected_runtime_baseline_tag": pipeline_summary_data.get("runtime_selected_baseline_tag")
        or runtime_refresh_data.get("selected_baseline_tag"),
        "pipeline_severity": pipeline_summary_data.get("severity"),
        "runtime_severity": pipeline_summary_data.get("runtime_severity"),
        "runtime_recommendation": pipeline_summary_data.get("runtime_recommendation"),
        "pipeline_exit_code": pipeline_summary_data.get("exit_code"),
        "pipeline_recommended_next_action": pipeline_summary_data.get("recommended_next_action"),
        "baseline_manifest_hash": baseline_manifest_hash,
        "current_manifest_hash": current_manifest_hash,
        "refresh_manifest_hash": refresh_manifest_hash,
        "rerun_plan_hash": rerun_plan_hash,
        "pipeline_summary_hash": sha256_file(pipeline_summary),
        "pipeline_quick_summary_hash": sha256_file(pipeline_quick_summary),
        "runtime_manifest_hash": sha256_file(runtime_manifest),
        "runtime_baseline_manifest_hash": sha256_file(runtime_baseline_manifest),
        "runtime_current_manifest_hash": sha256_file(runtime_current_manifest),
        "runtime_refresh_manifest_hash": sha256_file(runtime_refresh_manifest),
        "runtime_rerun_plan_hash": sha256_file(runtime_rerun_plan),
        "runtime_registry_hash": sha256_file(runtime_registry),
        "runtime_history_index_hash": sha256_file(runtime_history_index),
        "runtime_proposal_hash": sha256_file(runtime_proposal),
        "runtime_watch_current_hash": sha256_file(runtime_watch_current),
        "runtime_watch_refresh_hash": sha256_file(runtime_watch_refresh),
        "runtime_watch_history_index_hash": sha256_file(runtime_watch_history_index),
        "approved_runtime_baseline_hash": sha256_file(runtime_baseline_manifest),
        "proposal_archive_hash": sha256_file(runtime_proposal),
        "freshness_summary": freshness_summary,
        "refresh_summary": refresh_rollup,
        "pipeline_summary_data": pipeline_summary_data,
        "pipeline_quick_summary_data": pipeline_quick_summary_data,
        "runtime_summary": runtime_manifest_data,
        "runtime_baseline_summary": runtime_baseline_data,
        "runtime_refresh_summary": runtime_refresh_data,
        "runtime_rerun_plan_summary": runtime_rerun_plan_data,
        "runtime_registry_summary": runtime_registry_data,
        "runtime_history_summary": runtime_history_data,
        "runtime_proposal_summary": runtime_proposal_data,
        "runtime_watch_current_summary": runtime_watch_current_data,
        "runtime_watch_refresh_summary": runtime_watch_refresh_data,
        "runtime_watch_history_summary": runtime_watch_history_data,
        "runtime_approval_metadata_summary": runtime_approval_metadata,
        "pipeline_matrix_summary_data": pipeline_matrix_summary_data,
        "runtime_budget_summary": {
            "current_verdict": pipeline_summary_data.get("runtime_current_verdict")
            or runtime_refresh_data.get("current_verdict")
            or runtime_manifest_data.get("current_verdict"),
            "freshness_verdict": pipeline_summary_data.get("runtime_freshness_verdict")
            or runtime_refresh_data.get("freshness_verdict"),
            "comparability_verdict": pipeline_summary_data.get("runtime_comparability_verdict")
            or runtime_refresh_data.get("comparability_verdict"),
            "budget_verdict": pipeline_summary_data.get("runtime_budget_verdict")
            or runtime_refresh_data.get("overall_budget_verdict")
            or runtime_manifest_data.get("overall_budget_verdict"),
            "runtime_severity": pipeline_summary_data.get("runtime_severity"),
            "warn_count": runtime_refresh_data.get("warn_count", runtime_manifest_data.get("warn_count")),
            "fail_count": runtime_refresh_data.get("fail_count", runtime_manifest_data.get("fail_count")),
            "stale_entry_count": runtime_refresh_data.get("stale_entry_count"),
            "requires_rerun_entry_count": runtime_refresh_data.get("requires_rerun_entry_count"),
            "rebaseline_required_count": runtime_refresh_data.get("rebaseline_required_count"),
            "not_comparable_count": runtime_refresh_data.get("not_comparable_count"),
            "selected_baseline_id": runtime_refresh_data.get("selected_baseline_id"),
            "selected_baseline_tag": runtime_refresh_data.get("selected_baseline_tag"),
        },
        "runtime_watch_status": runtime_watch_refresh_data.get("overall_watch_status")
        or runtime_watch_current_data.get("overall_watch_status"),
        "runtime_watch_reason": runtime_watch_refresh_data.get("overall_watch_reason")
        or runtime_watch_current_data.get("overall_watch_reason"),
        "runtime_watch_sample_count": runtime_watch_refresh_data.get("runtime_watch_sample_count")
        or runtime_watch_current_data.get("watch_sample_count"),
        "runtime_watch_recommendation": runtime_watch_refresh_data.get("overall_watch_recommendation")
        or runtime_watch_current_data.get("overall_watch_recommendation"),
        "runtime_watch_fingerprint_count": runtime_watch_fingerprint_count,
        "runtime_watch_history_status_counts": runtime_watch_history_data.get("watch_status_counts", {}),
        "runtime_watch_transition_summary": runtime_watch_transition_summary,
        "runtime_watch_multi_fingerprint_summary": runtime_watch_matrix_summary,
        "runtime_budget_profile_id": runtime_watch_refresh_data.get("runtime_budget_profile_id")
        or runtime_watch_current_data.get("runtime_budget_profile_id"),
        "diagnostic_watch_only": bool(runtime_watch_refresh_data.get("diagnostic_watch_only", False)),
        "combined_summary": {
            "severity": pipeline_summary_data.get("severity"),
            "runtime_severity": pipeline_summary_data.get("runtime_severity"),
            "runtime_recommendation": pipeline_summary_data.get("runtime_recommendation"),
            "recommended_next_action": pipeline_summary_data.get("recommended_next_action"),
            "runtime_selected_baseline_id": pipeline_summary_data.get("runtime_selected_baseline_id"),
            "runtime_selected_baseline_tag": pipeline_summary_data.get("runtime_selected_baseline_tag"),
            "runtime_rebaseline_proposal_needed": pipeline_summary_data.get("runtime_rebaseline_proposal_needed"),
            "runtime_watch_status": pipeline_summary_data.get("runtime_watch_status"),
            "runtime_watch_recommendation": pipeline_summary_data.get("runtime_watch_recommendation"),
        },
        "previous_active_runtime_baseline_id": runtime_approval_metadata.get("previous_active_runtime_baseline_id")
        or runtime_approval_metadata.get("selected_previous_baseline"),
        "new_active_runtime_baseline_id": runtime_approval_metadata.get("new_active_runtime_baseline_id"),
        "runtime_transition_status": runtime_approval_metadata.get("runtime_transition_status"),
        "combined_pipeline_status_after_rebaseline": pipeline_summary_data.get("severity"),
        "runtime_recommendation": pipeline_summary_data.get("runtime_recommendation"),
        "rebaseline_proposal_needed": pipeline_summary_data.get("runtime_rebaseline_proposal_needed"),
        "runtime_trend_summary": runtime_history_data.get("trend_counts", {}),
        "selected_runtime_baseline_id": pipeline_summary_data.get("runtime_selected_baseline_id"),
        "selected_runtime_baseline_tag": pipeline_summary_data.get("runtime_selected_baseline_tag"),
        "drift_flags": drift_flags,
        "reclassification_needed_families": reclassification_needed,
        "family_status_summary": family_status_summary,
        "family_status_table": family_status_summary,
        "family_freshness": family_freshness,
        "stale_family_count": stale_family_count,
        "requires_rerun_family_count": requires_rerun_family_count,
        "reclassify_required_count": reclassify_required_count,
        "copied_counts": {key: len(value) for key, value in copied.items()},
    }
    atomic_write_text(bundle_root / "bundle_metadata.json", json.dumps(metadata, indent=2) + "\n")

    artifact_index_json, bundle_index_json = update_indexes(
        repo_root,
        artifact_root,
        bundle_root,
        zip_out,
        delivery_zip,
        curated_zip,
        pipeline_summary,
        pipeline_quick_summary,
        runtime_current_manifest or runtime_manifest,
        runtime_registry,
        runtime_history_index,
        runtime_proposal,
        bundle_index_out,
    )
    pruned = {"bundles": [], "curated_bundles": [], "evidence_bundles": [], "nightly_runs": []}
    if args.prune_artifacts:
        pruned = prune_retained_artifacts(
            zip_out.parent,
            artifact_root,
            max(1, args.max_bundles),
            max(1, args.max_nightly_runs),
            args.keep_approved,
        )
        artifact_index_json, bundle_index_json = update_indexes(
            repo_root,
            artifact_root,
            bundle_root,
            zip_out,
            delivery_zip,
            curated_zip,
            pipeline_summary,
            pipeline_quick_summary,
            runtime_current_manifest or runtime_manifest,
            runtime_registry,
            runtime_history_index,
            runtime_proposal,
            bundle_index_out,
        )

    metadata["artifact_index"] = str(artifact_index_json)
    metadata["bundle_index"] = str(bundle_index_json)
    metadata["retention"] = {
        "prune_artifacts": bool(args.prune_artifacts),
        "max_bundles": args.max_bundles,
        "max_nightly_runs": args.max_nightly_runs,
        "keep_approved": bool(args.keep_approved),
        "pruned": pruned,
    }
    atomic_write_text(bundle_root / "bundle_metadata.json", json.dumps(metadata, indent=2) + "\n")
    copied["manifests"].extend(copy_tree_if_exists(artifact_index_json, manifests_dir / artifact_index_json.name))
    copied["manifests"].extend(copy_tree_if_exists(bundle_index_json, manifests_dir / bundle_index_json.name))
    copied["curated"].extend(copy_tree_if_exists(artifact_index_json, curated_dir / artifact_index_json.name))
    copied["curated"].extend(copy_tree_if_exists(bundle_index_json, curated_dir / bundle_index_json.name))
    metadata["copied_counts"] = {key: len(value) for key, value in copied.items()}
    atomic_write_text(bundle_root / "bundle_metadata.json", json.dumps(metadata, indent=2) + "\n")

    zip_out.parent.mkdir(parents=True, exist_ok=True)
    archive_base = zip_out.with_suffix("")
    temp_zip = Path(shutil.make_archive(str(archive_base), "zip", root_dir=bundle_root.parent, base_dir=bundle_root.name))
    if temp_zip != zip_out:
        shutil.move(str(temp_zip), str(zip_out))

    if curated_zip is not None:
        curated_zip.parent.mkdir(parents=True, exist_ok=True)
        curated_archive_base = curated_zip.with_suffix("")
        temp_curated_zip = Path(
            shutil.make_archive(str(curated_archive_base), "zip", root_dir=curated_dir.parent, base_dir=curated_dir.name)
        )
        if temp_curated_zip != curated_zip:
            shutil.move(str(temp_curated_zip), str(curated_zip))

    delivery_entries = build_delivery_entries(
        report_path,
        current_manifest,
        baseline_manifest,
        refresh_manifest,
        rerun_plan,
        pipeline_summary,
        pipeline_quick_summary,
        pipeline_matrix_summary,
        runtime_baseline_manifest,
        runtime_current_manifest or runtime_manifest,
        runtime_refresh_manifest,
        runtime_rerun_plan,
        runtime_registry,
        runtime_history_index,
        runtime_proposal,
        runtime_watch_current,
        runtime_watch_refresh,
        runtime_watch_history_index,
        bundle_root / "bundle_metadata.json",
        zip_out,
        curated_zip,
    )
    metadata["delivery_bundle_items"] = [
        {"label": label, "path": str(path), "sha256": sha256_file(path)} for label, path in delivery_entries
    ]
    metadata["delivery_zip"] = str(delivery_zip)
    atomic_write_text(bundle_root / "bundle_metadata.json", json.dumps(metadata, indent=2) + "\n")
    create_delivery_zip(delivery_zip, delivery_entries)
    metadata["delivery_zip_hash"] = sha256_file(delivery_zip)
    atomic_write_text(bundle_root / "delivery_zip.sha256", f"{metadata['delivery_zip_hash']}  {delivery_zip.name}\n")
    atomic_write_text(bundle_root / "bundle_metadata.json", json.dumps(metadata, indent=2) + "\n")

    print(str(zip_out))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
