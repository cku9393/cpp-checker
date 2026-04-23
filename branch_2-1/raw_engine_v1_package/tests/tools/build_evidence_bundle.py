#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import time
import tempfile
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
        "--light-ops-zip",
        default=None,
        help="Optional light operator zip containing only the minimal current/baseline/runtime/operator artifacts.",
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
    parser.add_argument("--runtime-registry-health", default=None, help="Optional runtime registry health audit path.")
    parser.add_argument("--runtime-history-index", default=None, help="Optional runtime history index path.")
    parser.add_argument("--runtime-history-compact", default=None, help="Optional compacted runtime history index path.")
    parser.add_argument("--runtime-proposal", default=None, help="Optional runtime rebaseline proposal path.")
    parser.add_argument("--runtime-proposal-gate", default=None, help="Optional runtime proposal gate path.")
    parser.add_argument("--runtime-import-summary", default=None, help="Optional external/new environment import summary path.")
    parser.add_argument("--runtime-new-env-proposal", default=None, help="Optional new environment runtime proposal path.")
    parser.add_argument("--runtime-new-env-proposal-gate", default=None, help="Optional new environment runtime proposal gate path.")
    parser.add_argument("--runtime-new-env-archived-proposal", default=None, help="Optional archived new environment runtime proposal path.")
    parser.add_argument("--runtime-new-env-approved-baseline", default=None, help="Optional approved new environment runtime baseline path.")
    parser.add_argument("--runtime-registry-summary", default=None, help="Optional runtime registry summary path.")
    parser.add_argument("--runtime-budget-current", default=None, help="Optional runtime budget current manifest path.")
    parser.add_argument("--runtime-budget-baseline", default=None, help="Optional approved runtime budget baseline path.")
    parser.add_argument("--runtime-budget-refresh", default=None, help="Optional runtime budget refresh manifest path.")
    parser.add_argument("--runtime-budget-rerun", default=None, help="Optional runtime budget rerun plan path.")
    parser.add_argument("--runtime-budget-proposal", default=None, help="Optional runtime budget proposal path.")
    parser.add_argument("--runtime-budget-proposal-gate", default=None, help="Optional runtime budget proposal gate path.")
    parser.add_argument("--runtime-budget-registry", default=None, help="Optional runtime budget registry path.")
    parser.add_argument("--runtime-watch-current", default=None, help="Optional runtime watch current manifest path.")
    parser.add_argument("--runtime-watch-refresh", default=None, help="Optional runtime watch refresh manifest path.")
    parser.add_argument("--runtime-watch-history-index", default=None, help="Optional runtime watch history index path.")
    parser.add_argument("--runtime-watch-registry", default=None, help="Optional runtime watch registry path.")
    parser.add_argument("--source-snapshot-manifest", default=None, help="Optional source snapshot manifest for staged verification.")
    parser.add_argument("--staged-mirror-manifest", default=None, help="Optional staged mirror manifest for staged verification.")
    parser.add_argument("--staged-mirror-verify", default=None, help="Optional staged mirror verification manifest.")
    parser.add_argument("--ctest-inventory-release", default=None, help="Optional staged release CTest inventory manifest.")
    parser.add_argument("--ctest-inventory-debug", default=None, help="Optional staged debug CTest inventory manifest.")
    parser.add_argument("--ctest-inventory-asan", default=None, help="Optional staged ASan CTest inventory manifest.")
    parser.add_argument("--verification-release", default=None, help="Optional staged release verification manifest.")
    parser.add_argument("--verification-debug", default=None, help="Optional staged debug verification manifest.")
    parser.add_argument("--verification-asan", default=None, help="Optional staged ASan verification manifest.")
    parser.add_argument("--published-snapshot-manifest", default=None, help="Optional published snapshot manifest.")
    parser.add_argument("--verification-closeout", default=None, help="Optional staged verification closeout manifest.")
    parser.add_argument("--publication-health", default=None, help="Optional publication health audit path.")
    parser.add_argument("--ops-summary", default=None, help="Optional unified operator summary path.")
    parser.add_argument(
        "--approved-known-summary",
        action="append",
        default=[],
        help="Optional approved known environment summary path; may be repeated.",
    )
    parser.add_argument(
        "--foreign-import-summary",
        action="append",
        default=[],
        help="Optional unapproved foreign environment import summary path; may be repeated.",
    )
    parser.add_argument("--known-env-import-summary", default=None, help="Optional approved known environment import summary path.")
    parser.add_argument("--known-env-governance-policy", default=None, help="Optional approved known environment governance policy path.")
    parser.add_argument("--known-env-age-tick", default=None, help="Optional approved known environment synthetic age tick summary path.")
    parser.add_argument("--known-env-reverify-plan", default=None, help="Optional approved known environment reverify plan path.")
    parser.add_argument("--known-env-reverify-gate", default=None, help="Optional approved known environment reverify gate path.")
    parser.add_argument("--known-env-reverify-apply", default=None, help="Optional approved known environment reverify apply result path.")
    parser.add_argument("--known-env-retire-plan", default=None, help="Optional approved known environment retire plan path.")
    parser.add_argument("--known-env-retire-apply", default=None, help="Optional approved known environment retire apply result path.")
    parser.add_argument("--known-env-retire-metadata", default=None, help="Optional approved known environment retire metadata path.")
    parser.add_argument("--current-env-governance-policy", default=None, help="Optional current environment governance policy path.")
    parser.add_argument("--current-env-guardrail-policy", default=None, help="Optional current environment post-approval guardrail policy path.")
    parser.add_argument("--current-env-watch-current", default=None, help="Optional current environment watch lifecycle current artifact path.")
    parser.add_argument("--current-env-watch-refresh", default=None, help="Optional current environment watch lifecycle refresh artifact path.")
    parser.add_argument("--current-env-watch-history", default=None, help="Optional current environment watch lifecycle history artifact path.")
    parser.add_argument("--current-env-age-tick", default=None, help="Optional current environment synthetic age tick summary path.")
    parser.add_argument("--current-env-watch-plan", default=None, help="Optional current environment watch plan path.")
    parser.add_argument("--current-env-trigger-gate", default=None, help="Optional current environment reproposal trigger gate path.")
    parser.add_argument("--current-env-due", default=None, help="Optional current environment due scheduler path.")
    parser.add_argument("--current-env-reproposal-plan", default=None, help="Optional current environment reproposal plan path.")
    parser.add_argument("--ops-agenda", default=None, help="Optional unified operator action queue / ops agenda path.")
    parser.add_argument("--current-env-watch-execute", default=None, help="Optional current environment watch execution manifest path.")
    parser.add_argument("--current-env-watch-apply", default=None, help="Optional current environment watch apply manifest path.")
    parser.add_argument("--current-env-reproposal-execute", default=None, help="Optional current environment reproposal gate execution manifest path.")
    parser.add_argument("--current-env-action-ledger", default=None, help="Optional current environment action ledger manifest path.")
    parser.add_argument("--current-env-retry-plan", default=None, help="Optional current environment action retry plan manifest path.")
    parser.add_argument("--current-env-reproposal-handoff", default=None, help="Optional current environment reproposal approval handoff manifest path.")
    parser.add_argument("--current-env-operator-decision", default=None, help="Optional current environment operator decision manifest path.")
    parser.add_argument("--current-env-operator-decision-apply", default=None, help="Optional current environment operator decision apply manifest path.")
    parser.add_argument("--current-env-action-ledger-compact", default=None, help="Optional current environment compacted action ledger manifest path.")
    parser.add_argument("--current-env-action-ledger-archive", default=None, help="Optional current environment action ledger archive manifest path.")
    parser.add_argument("--current-env-approval-runbook", default=None, help="Optional current environment budget approval runbook path.")
    parser.add_argument("--current-env-approval-execution", default=None, help="Optional current environment budget approval execution audit path.")
    parser.add_argument("--current-env-approval-link", default=None, help="Optional current environment approval execution ledger link path.")
    parser.add_argument("--operator-runbook-index", default=None, help="Optional operator runbook index path.")
    parser.add_argument("--operator-runbook-catalog", default=None, help="Optional operator runbook catalog/history path.")
    parser.add_argument("--operator-decision-metadata-audit", default=None, help="Optional operator decision metadata audit path.")
    parser.add_argument("--operator-runbook-replay", default=None, help="Optional operator runbook replay manifest path.")
    parser.add_argument("--operator-runbook-retention-policy", default=None, help="Optional operator runbook retention policy path.")
    parser.add_argument("--operator-runbook-pruned-catalog", default=None, help="Optional pruned operator runbook catalog path.")
    parser.add_argument("--operator-runbook-archive", default=None, help="Optional operator runbook archive path.")
    parser.add_argument("--operator-runbook-prune-summary", default=None, help="Optional operator runbook prune summary path.")
    parser.add_argument("--operator-runbook-lifecycle-validation", default=None, help="Optional operator runbook lifecycle validation path.")
    parser.add_argument("--operator-runbook-pointer-audit", default=None, help="Optional operator runbook pointer audit path.")
    parser.add_argument("--operator-runbook-provenance-migration", default=None, help="Optional operator runbook provenance migration report path.")
    parser.add_argument("--operator-runbook-migrated-catalog", default=None, help="Optional migrated operator runbook catalog path.")
    parser.add_argument("--operator-runbook-migrated-ledger", default=None, help="Optional migrated current-env action ledger path.")
    parser.add_argument("--operator-runbook-lifecycle-validation-before", default=None, help="Optional pre-migration operator runbook lifecycle validation path.")
    parser.add_argument("--operator-runbook-lifecycle-validation-after", default=None, help="Optional post-migration operator runbook lifecycle validation path.")
    parser.add_argument("--operator-artifact-path-policy-lint", default=None, help="Optional operator artifact path policy lint result path.")
    parser.add_argument("--integrated-approval-mutation-audit", default=None, help="Optional integrated approval mutation audit path.")
    parser.add_argument("--staged-materialization-transaction", default=None, help="Optional staged materialization transaction log path.")
    parser.add_argument("--source-health-preflight", default=None, help="Optional source health preflight path.")
    parser.add_argument("--source-health-action-plan", default=None, help="Optional source health action plan path.")
    parser.add_argument("--staged-materialization", default=None, help="Optional staged materialization v2 manifest path.")
    parser.add_argument(
        "--current-env-next-cycle-summary",
        action="append",
        default=[],
        help="Optional current environment next due-cycle synthetic validation summary path; may be repeated.",
    )
    parser.add_argument("--runtime-budget-reproposal-history", default=None, help="Optional runtime budget reproposal history path.")
    parser.add_argument("--runtime-budget-registry-summary", default=None, help="Optional runtime budget registry summary path.")
    parser.add_argument(
        "--use-published-snapshot",
        action="store_true",
        help="Publish the evidence bundle into a published snapshot root instead of the default in-place bundle root.",
    )
    parser.add_argument(
        "--published-root",
        default=None,
        help="Optional explicit published snapshot root. Defaults to <artifact-root>/<phase>_evidence_bundle_published when --use-published-snapshot is set.",
    )
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


def write_json(path: Path, payload: dict | list) -> None:
    atomic_write_text(path, json.dumps(payload, indent=2) + "\n")


def latest_matching_file(root: Path, pattern: str, phase: str | None = None) -> Path | None:
    matches = [path for path in root.rglob(pattern) if path.is_file()]
    if not matches:
        return None
    phase_token = str(phase or "").strip().lower()
    if phase_token:
        phase_scoped_matches = [path for path in matches if phase_token in str(path).lower()]
        if phase_scoped_matches:
            matches = phase_scoped_matches
    return max(matches, key=lambda path: path.stat().st_mtime)


def is_nested_evidence_bundle_path(path: Path) -> bool:
    return any(parent.name.endswith("_evidence_bundle") for parent in path.parents)


def should_copy_log_source(relative: Path, phase: str) -> bool:
    normalized_parts = [part.lower() for part in relative.parts]
    phase_token = str(phase).strip().lower()
    if phase_token and any(phase_token in part for part in normalized_parts):
        return True
    if not normalized_parts:
        return False
    return normalized_parts[0] in {"logs", "matrix", "nightly_runs"}


def should_copy_phase_scoped_name(name: str, phase: str) -> bool:
    phase_token = str(phase).strip().lower()
    return bool(phase_token) and phase_token in name.lower()


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


def parse_manifest_summary_paths(manifest_data: dict, phase: str | None = None) -> list[Path]:
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
    phase_token = str(phase or "").strip().lower()
    if phase_token:
        phase_scoped_paths = [path for path in paths if phase_token in str(path).lower()]
        if phase_scoped_paths:
            return phase_scoped_paths
        return []
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
    runtime_registry_health: Path | None,
    runtime_history_index: Path | None,
    runtime_history_compact: Path | None,
    runtime_proposal: Path | None,
    runtime_proposal_gate: Path | None,
    runtime_watch_current: Path | None,
    runtime_watch_refresh: Path | None,
    runtime_watch_history_index: Path | None,
    runtime_watch_registry: Path | None,
    known_env_import_summary: Path | None,
    known_env_governance_policy: Path | None,
    known_env_age_tick: Path | None,
    known_env_reverify_plan: Path | None,
    known_env_reverify_gate: Path | None,
    known_env_reverify_apply: Path | None,
    known_env_retire_plan: Path | None,
    known_env_retire_apply: Path | None,
    known_env_retire_metadata: Path | None,
    source_snapshot_manifest: Path | None,
    staged_mirror_manifest: Path | None,
    staged_mirror_verify: Path | None,
    ctest_inventory_release: Path | None,
    ctest_inventory_debug: Path | None,
    ctest_inventory_asan: Path | None,
    verification_release: Path | None,
    verification_debug: Path | None,
    verification_asan: Path | None,
    published_snapshot_manifest: Path | None,
    verification_closeout: Path | None,
    publication_health: Path | None,
    ops_summary: Path | None,
    current_env_operator_decision: Path | None,
    current_env_operator_decision_apply: Path | None,
    current_env_action_ledger_compact: Path | None,
    current_env_action_ledger_archive: Path | None,
    current_env_approval_runbook: Path | None,
    current_env_approval_execution: Path | None,
    current_env_approval_link: Path | None,
    operator_runbook_index: Path | None,
    operator_runbook_catalog: Path | None,
    operator_decision_metadata_audit: Path | None,
    operator_runbook_replay: Path | None,
    operator_runbook_retention_policy: Path | None,
    operator_runbook_pruned_catalog: Path | None,
    operator_runbook_archive: Path | None,
    operator_runbook_prune_summary: Path | None,
    operator_runbook_lifecycle_validation: Path | None,
    operator_runbook_pointer_audit: Path | None,
    operator_runbook_provenance_migration: Path | None,
    operator_runbook_migrated_catalog: Path | None,
    operator_runbook_migrated_ledger: Path | None,
    operator_runbook_lifecycle_validation_before: Path | None,
    operator_runbook_lifecycle_validation_after: Path | None,
    operator_artifact_path_policy_lint: Path | None,
    integrated_approval_mutation_audit: Path | None,
    source_health_preflight: Path | None,
    source_health_action_plan: Path | None,
    staged_materialization: Path | None,
    staged_materialization_transaction: Path | None,
    bundle_metadata: Path,
    zip_out: Path,
    curated_zip: Path | None,
    light_ops_zip: Path | None,
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
        ("runtime_registry_health", runtime_registry_health),
        ("runtime_history_index", runtime_history_index),
        ("runtime_history_compact", runtime_history_compact),
        ("runtime_proposal", runtime_proposal),
        ("runtime_proposal_gate", runtime_proposal_gate),
        ("runtime_watch_current", runtime_watch_current),
        ("runtime_watch_refresh", runtime_watch_refresh),
        ("runtime_watch_history_index", runtime_watch_history_index),
        ("runtime_watch_registry", runtime_watch_registry),
        ("known_env_import_summary", known_env_import_summary),
        ("known_env_governance_policy", known_env_governance_policy),
        ("known_env_age_tick", known_env_age_tick),
        ("known_env_reverify_plan", known_env_reverify_plan),
        ("known_env_reverify_gate", known_env_reverify_gate),
        ("known_env_reverify_apply", known_env_reverify_apply),
        ("known_env_retire_plan", known_env_retire_plan),
        ("known_env_retire_apply", known_env_retire_apply),
        ("known_env_retire_metadata", known_env_retire_metadata),
        ("source_snapshot_manifest", source_snapshot_manifest),
        ("staged_mirror_manifest", staged_mirror_manifest),
        ("staged_mirror_verify", staged_mirror_verify),
        ("ctest_inventory_release", ctest_inventory_release),
        ("ctest_inventory_debug", ctest_inventory_debug),
        ("ctest_inventory_asan", ctest_inventory_asan),
        ("verification_release", verification_release),
        ("verification_debug", verification_debug),
        ("verification_asan", verification_asan),
        ("published_snapshot_manifest", published_snapshot_manifest),
        ("verification_closeout", verification_closeout),
        ("publication_health", publication_health),
        ("policy_ops_summary", ops_summary),
        ("current_env_operator_decision", current_env_operator_decision),
        ("current_env_operator_decision_apply", current_env_operator_decision_apply),
        ("current_env_action_ledger_compact", current_env_action_ledger_compact),
        ("current_env_action_ledger_archive", current_env_action_ledger_archive),
        ("current_env_approval_runbook", current_env_approval_runbook),
        ("current_env_approval_execution", current_env_approval_execution),
        ("current_env_approval_link", current_env_approval_link),
        ("operator_runbook_index", operator_runbook_index),
        ("operator_runbook_catalog", operator_runbook_catalog),
        ("operator_decision_metadata_audit", operator_decision_metadata_audit),
        ("operator_runbook_replay", operator_runbook_replay),
        ("operator_runbook_retention_policy", operator_runbook_retention_policy),
        ("operator_runbook_pruned_catalog", operator_runbook_pruned_catalog),
        ("operator_runbook_archive", operator_runbook_archive),
        ("operator_runbook_prune_summary", operator_runbook_prune_summary),
        ("operator_runbook_lifecycle_validation", operator_runbook_lifecycle_validation),
        ("operator_runbook_pointer_audit", operator_runbook_pointer_audit),
        ("operator_runbook_provenance_migration", operator_runbook_provenance_migration),
        ("operator_runbook_migrated_catalog", operator_runbook_migrated_catalog),
        ("operator_runbook_migrated_ledger", operator_runbook_migrated_ledger),
        ("operator_runbook_lifecycle_validation_before", operator_runbook_lifecycle_validation_before),
        ("operator_runbook_lifecycle_validation_after", operator_runbook_lifecycle_validation_after),
        ("operator_artifact_path_policy_lint", operator_artifact_path_policy_lint),
        ("integrated_approval_mutation_audit", integrated_approval_mutation_audit),
        ("source_health_preflight", source_health_preflight),
        ("source_health_action_plan", source_health_action_plan),
        ("staged_materialization", staged_materialization),
        ("staged_materialization_transaction", staged_materialization_transaction),
        ("bundle_metadata", bundle_metadata),
        ("bundle_zip", zip_out),
        ("curated_zip", curated_zip),
        ("light_ops_zip", light_ops_zip),
    ]
    for label, path in optional_entries:
        if path is not None and path.exists():
            entries.append((label, path))
    return entries


def create_delivery_zip(delivery_zip: Path, entries: list[tuple[str, Path]]) -> None:
    delivery_zip.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(prefix=f"{delivery_zip.stem}.", suffix=".zip", dir=tempfile.gettempdir(), delete=False) as handle:
        staged_zip = Path(handle.name)
    try:
        with zipfile.ZipFile(staged_zip, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for label, path in entries:
                archive.write(path, arcname=f"{label}/{path.name}")
        if delivery_zip.exists():
            delivery_zip.unlink()
        shutil.move(str(staged_zip), str(delivery_zip))
    finally:
        if staged_zip.exists():
            staged_zip.unlink(missing_ok=True)


def create_directory_zip(zip_path: Path, directory: Path) -> None:
    zip_path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(prefix=f"{zip_path.stem}.", suffix=".zip", dir=tempfile.gettempdir(), delete=False) as handle:
        staged_zip = Path(handle.name)
    try:
        with zipfile.ZipFile(staged_zip, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(directory.rglob("*")):
                if not path.is_file():
                    continue
                archive.write(path, arcname=str(path.relative_to(directory.parent)))
        if zip_path.exists():
            zip_path.unlink()
        shutil.move(str(staged_zip), str(zip_path))
    finally:
        if staged_zip.exists():
            staged_zip.unlink(missing_ok=True)


def default_published_bundle_root(artifact_root: Path, phase: str) -> Path:
    return artifact_root / f"{phase}_evidence_bundle_published"


def publication_snapshot_id(report_path: Path, phase: str) -> str:
    timestamp = stable_bundle_timestamp(report_path).replace(":", "").replace("-", "")
    return f"{phase}-published-{timestamp}"


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
    staging_root = artifact_root
    report_path = Path(args.report_out).resolve()
    zip_out = Path(args.zip_out).resolve()
    delivery_zip = Path(args.delivery_zip).resolve() if args.delivery_zip else default_delivery_zip(zip_out)
    light_ops_zip = Path(args.light_ops_zip).resolve() if args.light_ops_zip else None
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
    runtime_registry_health = resolve_manifest_json(
        Path(args.runtime_registry_health).resolve() if args.runtime_registry_health else None
    )
    runtime_history_index = resolve_manifest_json(
        Path(args.runtime_history_index).resolve() if args.runtime_history_index else None
    )
    runtime_history_compact = resolve_manifest_json(
        Path(args.runtime_history_compact).resolve()
        if args.runtime_history_compact
        else (
            runtime_history_index.with_name(f"{runtime_history_index.stem}_compacted.json")
            if runtime_history_index is not None
            else None
        )
    )
    runtime_proposal = resolve_manifest_json(Path(args.runtime_proposal).resolve() if args.runtime_proposal else None)
    runtime_proposal_gate = resolve_manifest_json(
        Path(args.runtime_proposal_gate).resolve() if args.runtime_proposal_gate else None
    )
    runtime_import_summary = resolve_manifest_json(
        Path(args.runtime_import_summary).resolve() if args.runtime_import_summary else None
    )
    runtime_new_env_proposal = resolve_manifest_json(
        Path(args.runtime_new_env_proposal).resolve() if args.runtime_new_env_proposal else None
    )
    runtime_new_env_proposal_gate = resolve_manifest_json(
        Path(args.runtime_new_env_proposal_gate).resolve() if args.runtime_new_env_proposal_gate else None
    )
    runtime_new_env_archived_proposal = resolve_manifest_json(
        Path(args.runtime_new_env_archived_proposal).resolve() if args.runtime_new_env_archived_proposal else None
    )
    runtime_new_env_approved_baseline = resolve_manifest_json(
        Path(args.runtime_new_env_approved_baseline).resolve() if args.runtime_new_env_approved_baseline else None
    )
    known_env_import_summary = resolve_manifest_json(
        Path(args.known_env_import_summary).resolve() if args.known_env_import_summary else None
    )
    known_env_governance_policy = resolve_manifest_json(
        Path(args.known_env_governance_policy).resolve() if args.known_env_governance_policy else None
    )
    known_env_age_tick = resolve_manifest_json(
        Path(args.known_env_age_tick).resolve() if args.known_env_age_tick else None
    )
    known_env_reverify_plan = resolve_manifest_json(
        Path(args.known_env_reverify_plan).resolve() if args.known_env_reverify_plan else None
    )
    known_env_reverify_gate = resolve_manifest_json(
        Path(args.known_env_reverify_gate).resolve() if args.known_env_reverify_gate else None
    )
    known_env_reverify_apply = resolve_manifest_json(
        Path(args.known_env_reverify_apply).resolve() if args.known_env_reverify_apply else None
    )
    known_env_retire_plan = resolve_manifest_json(
        Path(args.known_env_retire_plan).resolve() if args.known_env_retire_plan else None
    )
    known_env_retire_apply = resolve_manifest_json(
        Path(args.known_env_retire_apply).resolve() if args.known_env_retire_apply else None
    )
    known_env_retire_metadata = resolve_manifest_json(
        Path(args.known_env_retire_metadata).resolve() if args.known_env_retire_metadata else None
    )
    current_env_governance_policy = resolve_manifest_json(
        Path(args.current_env_governance_policy).resolve() if args.current_env_governance_policy else None
    )
    current_env_guardrail_policy = resolve_manifest_json(
        Path(args.current_env_guardrail_policy).resolve() if args.current_env_guardrail_policy else None
    )
    current_env_watch_current = resolve_manifest_json(
        Path(args.current_env_watch_current).resolve() if args.current_env_watch_current else None
    )
    current_env_watch_refresh = resolve_manifest_json(
        Path(args.current_env_watch_refresh).resolve() if args.current_env_watch_refresh else None
    )
    current_env_watch_history = resolve_manifest_json(
        Path(args.current_env_watch_history).resolve() if args.current_env_watch_history else None
    )
    current_env_age_tick = resolve_manifest_json(
        Path(args.current_env_age_tick).resolve() if args.current_env_age_tick else None
    )
    current_env_watch_plan = resolve_manifest_json(
        Path(args.current_env_watch_plan).resolve() if args.current_env_watch_plan else None
    )
    current_env_trigger_gate = resolve_manifest_json(
        Path(args.current_env_trigger_gate).resolve() if args.current_env_trigger_gate else None
    )
    current_env_due = resolve_manifest_json(
        Path(args.current_env_due).resolve() if args.current_env_due else None
    )
    current_env_reproposal_plan = resolve_manifest_json(
        Path(args.current_env_reproposal_plan).resolve() if args.current_env_reproposal_plan else None
    )
    ops_agenda = resolve_manifest_json(
        Path(args.ops_agenda).resolve() if args.ops_agenda else None
    )
    current_env_watch_execute = resolve_manifest_json(
        Path(args.current_env_watch_execute).resolve() if args.current_env_watch_execute else None
    )
    current_env_watch_apply = resolve_manifest_json(
        Path(args.current_env_watch_apply).resolve() if args.current_env_watch_apply else None
    )
    current_env_reproposal_execute = resolve_manifest_json(
        Path(args.current_env_reproposal_execute).resolve() if args.current_env_reproposal_execute else None
    )
    current_env_action_ledger = resolve_manifest_json(
        Path(args.current_env_action_ledger).resolve() if args.current_env_action_ledger else None
    )
    current_env_retry_plan = resolve_manifest_json(
        Path(args.current_env_retry_plan).resolve() if args.current_env_retry_plan else None
    )
    current_env_reproposal_handoff = resolve_manifest_json(
        Path(args.current_env_reproposal_handoff).resolve() if args.current_env_reproposal_handoff else None
    )
    current_env_operator_decision = resolve_manifest_json(
        Path(args.current_env_operator_decision).resolve() if args.current_env_operator_decision else None
    )
    current_env_operator_decision_apply = resolve_manifest_json(
        Path(args.current_env_operator_decision_apply).resolve() if args.current_env_operator_decision_apply else None
    )
    current_env_action_ledger_compact = resolve_manifest_json(
        Path(args.current_env_action_ledger_compact).resolve() if args.current_env_action_ledger_compact else None
    )
    current_env_action_ledger_archive = resolve_manifest_json(
        Path(args.current_env_action_ledger_archive).resolve() if args.current_env_action_ledger_archive else None
    )
    current_env_approval_runbook = resolve_manifest_json(
        Path(args.current_env_approval_runbook).resolve() if args.current_env_approval_runbook else None
    )
    current_env_approval_execution = resolve_manifest_json(
        Path(args.current_env_approval_execution).resolve() if args.current_env_approval_execution else None
    )
    current_env_approval_link = resolve_manifest_json(
        Path(args.current_env_approval_link).resolve() if args.current_env_approval_link else None
    )
    operator_runbook_index = resolve_manifest_json(
        Path(args.operator_runbook_index).resolve() if args.operator_runbook_index else None
    )
    operator_runbook_catalog = resolve_manifest_json(
        Path(args.operator_runbook_catalog).resolve() if args.operator_runbook_catalog else None
    )
    operator_decision_metadata_audit = resolve_manifest_json(
        Path(args.operator_decision_metadata_audit).resolve() if args.operator_decision_metadata_audit else None
    )
    operator_runbook_replay = resolve_manifest_json(
        Path(args.operator_runbook_replay).resolve() if args.operator_runbook_replay else None
    )
    operator_runbook_retention_policy = resolve_manifest_json(
        Path(args.operator_runbook_retention_policy).resolve() if args.operator_runbook_retention_policy else None
    )
    operator_runbook_pruned_catalog = resolve_manifest_json(
        Path(args.operator_runbook_pruned_catalog).resolve() if args.operator_runbook_pruned_catalog else None
    )
    operator_runbook_archive = resolve_manifest_json(
        Path(args.operator_runbook_archive).resolve() if args.operator_runbook_archive else None
    )
    operator_runbook_prune_summary = resolve_manifest_json(
        Path(args.operator_runbook_prune_summary).resolve() if args.operator_runbook_prune_summary else None
    )
    operator_runbook_lifecycle_validation = resolve_manifest_json(
        Path(args.operator_runbook_lifecycle_validation).resolve() if args.operator_runbook_lifecycle_validation else None
    )
    operator_runbook_pointer_audit = resolve_manifest_json(
        Path(args.operator_runbook_pointer_audit).resolve() if args.operator_runbook_pointer_audit else None
    )
    operator_runbook_provenance_migration = resolve_manifest_json(
        Path(args.operator_runbook_provenance_migration).resolve() if args.operator_runbook_provenance_migration else None
    )
    operator_runbook_migrated_catalog = resolve_manifest_json(
        Path(args.operator_runbook_migrated_catalog).resolve() if args.operator_runbook_migrated_catalog else None
    )
    operator_runbook_migrated_ledger = resolve_manifest_json(
        Path(args.operator_runbook_migrated_ledger).resolve() if args.operator_runbook_migrated_ledger else None
    )
    operator_runbook_lifecycle_validation_before = resolve_manifest_json(
        Path(args.operator_runbook_lifecycle_validation_before).resolve() if args.operator_runbook_lifecycle_validation_before else None
    )
    operator_runbook_lifecycle_validation_after = resolve_manifest_json(
        Path(args.operator_runbook_lifecycle_validation_after).resolve() if args.operator_runbook_lifecycle_validation_after else None
    )
    operator_artifact_path_policy_lint = resolve_manifest_json(
        Path(args.operator_artifact_path_policy_lint).resolve() if args.operator_artifact_path_policy_lint else None
    )
    integrated_approval_mutation_audit = resolve_manifest_json(
        Path(args.integrated_approval_mutation_audit).resolve() if args.integrated_approval_mutation_audit else None
    )
    staged_materialization_transaction = resolve_manifest_json(
        Path(args.staged_materialization_transaction).resolve() if args.staged_materialization_transaction else None
    )
    source_health_preflight = resolve_manifest_json(
        Path(args.source_health_preflight).resolve() if args.source_health_preflight else None
    )
    source_health_action_plan = resolve_manifest_json(
        Path(args.source_health_action_plan).resolve() if args.source_health_action_plan else None
    )
    staged_materialization = resolve_manifest_json(
        Path(args.staged_materialization).resolve() if args.staged_materialization else None
    )
    current_env_next_cycle_summaries = [
        path
        for path in (
            resolve_manifest_json(Path(value).resolve()) for value in list(args.current_env_next_cycle_summary or [])
        )
        if path is not None
    ]
    runtime_registry_summary = resolve_manifest_json(
        Path(args.runtime_registry_summary).resolve()
        if args.runtime_registry_summary
        else (runtime_registry.with_name(f"{runtime_registry.stem}_summary.json") if runtime_registry is not None else None)
    )
    runtime_budget_current = resolve_manifest_json(
        Path(args.runtime_budget_current).resolve() if args.runtime_budget_current else None
    )
    runtime_budget_baseline = resolve_manifest_json(
        Path(args.runtime_budget_baseline).resolve() if args.runtime_budget_baseline else None
    )
    runtime_budget_refresh = resolve_manifest_json(
        Path(args.runtime_budget_refresh).resolve() if args.runtime_budget_refresh else None
    )
    runtime_budget_rerun = resolve_manifest_json(
        Path(args.runtime_budget_rerun).resolve() if args.runtime_budget_rerun else None
    )
    runtime_budget_proposal = resolve_manifest_json(
        Path(args.runtime_budget_proposal).resolve() if args.runtime_budget_proposal else None
    )
    runtime_budget_proposal_gate = resolve_manifest_json(
        Path(args.runtime_budget_proposal_gate).resolve() if args.runtime_budget_proposal_gate else None
    )
    runtime_budget_registry = resolve_manifest_json(
        Path(args.runtime_budget_registry).resolve() if args.runtime_budget_registry else None
    )
    runtime_budget_reproposal_history = resolve_manifest_json(
        Path(args.runtime_budget_reproposal_history).resolve() if args.runtime_budget_reproposal_history else None
    )
    runtime_budget_registry_summary = resolve_manifest_json(
        Path(args.runtime_budget_registry_summary).resolve() if args.runtime_budget_registry_summary else None
    )
    runtime_watch_current = resolve_manifest_json(
        Path(args.runtime_watch_current).resolve() if args.runtime_watch_current else None
    )
    runtime_watch_refresh = resolve_manifest_json(
        Path(args.runtime_watch_refresh).resolve() if args.runtime_watch_refresh else None
    )
    runtime_watch_history_index = resolve_manifest_json(
        Path(args.runtime_watch_history_index).resolve() if args.runtime_watch_history_index else None
    )
    runtime_watch_registry = resolve_manifest_json(
        Path(args.runtime_watch_registry).resolve() if args.runtime_watch_registry else None
    )
    source_snapshot_manifest = resolve_manifest_json(
        Path(args.source_snapshot_manifest).resolve() if args.source_snapshot_manifest else None
    )
    staged_mirror_manifest = resolve_manifest_json(
        Path(args.staged_mirror_manifest).resolve() if args.staged_mirror_manifest else None
    )
    staged_mirror_verify = resolve_manifest_json(
        Path(args.staged_mirror_verify).resolve() if args.staged_mirror_verify else None
    )
    ctest_inventory_release = resolve_manifest_json(
        Path(args.ctest_inventory_release).resolve() if args.ctest_inventory_release else None
    )
    ctest_inventory_debug = resolve_manifest_json(
        Path(args.ctest_inventory_debug).resolve() if args.ctest_inventory_debug else None
    )
    ctest_inventory_asan = resolve_manifest_json(
        Path(args.ctest_inventory_asan).resolve() if args.ctest_inventory_asan else None
    )
    verification_release = resolve_manifest_json(
        Path(args.verification_release).resolve() if args.verification_release else None
    )
    verification_debug = resolve_manifest_json(
        Path(args.verification_debug).resolve() if args.verification_debug else None
    )
    verification_asan = resolve_manifest_json(
        Path(args.verification_asan).resolve() if args.verification_asan else None
    )
    published_snapshot_manifest = resolve_manifest_json(
        Path(args.published_snapshot_manifest).resolve() if args.published_snapshot_manifest else None
    )
    verification_closeout = resolve_manifest_json(
        Path(args.verification_closeout).resolve() if args.verification_closeout else None
    )
    publication_health = resolve_manifest_json(
        Path(args.publication_health).resolve() if args.publication_health else None
    )
    ops_summary = resolve_manifest_json(
        Path(args.ops_summary).resolve() if args.ops_summary else None
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
    runtime_registry_health_data = read_json_if_exists(runtime_registry_health)
    runtime_history_summary_manifest = (
        runtime_history_index.with_name(f"{runtime_history_index.stem}_summary.json")
        if runtime_history_index is not None
        else None
    )
    runtime_history_data = read_json_if_exists(runtime_history_summary_manifest)
    runtime_history_compact_data = read_json_if_exists(runtime_history_compact)
    runtime_proposal_data = read_json_if_exists(runtime_proposal)
    runtime_proposal_gate_data = read_json_if_exists(runtime_proposal_gate)
    runtime_import_summary_data = read_json_if_exists(runtime_import_summary)
    runtime_new_env_proposal_data = read_json_if_exists(runtime_new_env_proposal)
    runtime_new_env_proposal_gate_data = read_json_if_exists(runtime_new_env_proposal_gate)
    runtime_new_env_archived_proposal_data = read_json_if_exists(runtime_new_env_archived_proposal)
    runtime_new_env_approved_baseline_data = read_json_if_exists(runtime_new_env_approved_baseline)
    known_env_import_summary_data = read_json_if_exists(known_env_import_summary)
    known_env_governance_policy_data = read_json_if_exists(known_env_governance_policy)
    known_env_age_tick_data = read_json_if_exists(known_env_age_tick)
    known_env_reverify_plan_data = read_json_if_exists(known_env_reverify_plan)
    known_env_reverify_gate_data = read_json_if_exists(known_env_reverify_gate)
    known_env_reverify_apply_data = read_json_if_exists(known_env_reverify_apply)
    known_env_retire_plan_data = read_json_if_exists(known_env_retire_plan)
    known_env_retire_apply_data = read_json_if_exists(known_env_retire_apply)
    known_env_retire_metadata_data = read_json_if_exists(known_env_retire_metadata)
    current_env_governance_policy_data = read_json_if_exists(current_env_governance_policy)
    current_env_guardrail_policy_data = read_json_if_exists(current_env_guardrail_policy)
    current_env_watch_current_data = read_json_if_exists(current_env_watch_current)
    current_env_watch_refresh_data = read_json_if_exists(current_env_watch_refresh)
    current_env_watch_history_data = read_json_if_exists(current_env_watch_history)
    current_env_age_tick_data = read_json_if_exists(current_env_age_tick)
    current_env_watch_plan_data = read_json_if_exists(current_env_watch_plan)
    current_env_trigger_gate_data = read_json_if_exists(current_env_trigger_gate)
    current_env_due_data = read_json_if_exists(current_env_due)
    current_env_reproposal_plan_data = read_json_if_exists(current_env_reproposal_plan)
    ops_agenda_data = read_json_if_exists(ops_agenda)
    current_env_watch_execute_data = read_json_if_exists(current_env_watch_execute)
    current_env_watch_apply_data = read_json_if_exists(current_env_watch_apply)
    current_env_reproposal_execute_data = read_json_if_exists(current_env_reproposal_execute)
    current_env_action_ledger_data = read_json_if_exists(current_env_action_ledger)
    current_env_retry_plan_data = read_json_if_exists(current_env_retry_plan)
    current_env_reproposal_handoff_data = read_json_if_exists(current_env_reproposal_handoff)
    current_env_operator_decision_data = read_json_if_exists(current_env_operator_decision)
    current_env_operator_decision_apply_data = read_json_if_exists(current_env_operator_decision_apply)
    current_env_action_ledger_compact_data = read_json_if_exists(current_env_action_ledger_compact)
    current_env_action_ledger_archive_data = read_json_if_exists(current_env_action_ledger_archive)
    current_env_approval_runbook_data = read_json_if_exists(current_env_approval_runbook)
    current_env_approval_execution_data = read_json_if_exists(current_env_approval_execution)
    current_env_approval_link_data = read_json_if_exists(current_env_approval_link)
    operator_runbook_index_data = read_json_if_exists(operator_runbook_index)
    operator_runbook_catalog_data = read_json_if_exists(operator_runbook_catalog)
    operator_decision_metadata_audit_data = read_json_if_exists(operator_decision_metadata_audit)
    operator_runbook_replay_data = read_json_if_exists(operator_runbook_replay)
    operator_runbook_retention_policy_data = read_json_if_exists(operator_runbook_retention_policy)
    operator_runbook_pruned_catalog_data = read_json_if_exists(operator_runbook_pruned_catalog)
    operator_runbook_archive_data = read_json_if_exists(operator_runbook_archive)
    operator_runbook_prune_summary_data = read_json_if_exists(operator_runbook_prune_summary)
    operator_runbook_lifecycle_validation_data = read_json_if_exists(operator_runbook_lifecycle_validation)
    operator_runbook_pointer_audit_data = read_json_if_exists(operator_runbook_pointer_audit)
    operator_runbook_provenance_migration_data = read_json_if_exists(operator_runbook_provenance_migration)
    operator_runbook_migrated_catalog_data = read_json_if_exists(operator_runbook_migrated_catalog)
    operator_runbook_migrated_ledger_data = read_json_if_exists(operator_runbook_migrated_ledger)
    operator_runbook_lifecycle_validation_before_data = read_json_if_exists(operator_runbook_lifecycle_validation_before)
    operator_runbook_lifecycle_validation_after_data = read_json_if_exists(operator_runbook_lifecycle_validation_after)
    operator_artifact_path_policy_lint_data = read_json_if_exists(operator_artifact_path_policy_lint)
    integrated_approval_mutation_audit_data = read_json_if_exists(integrated_approval_mutation_audit)
    staged_materialization_transaction_data = read_json_if_exists(staged_materialization_transaction)
    source_health_preflight_data = read_json_if_exists(source_health_preflight)
    source_health_action_plan_data = read_json_if_exists(source_health_action_plan)
    staged_materialization_data = read_json_if_exists(staged_materialization)
    current_env_next_cycle_summary_data = [read_json_if_exists(path) for path in current_env_next_cycle_summaries]
    runtime_registry_summary_data = read_json_if_exists(runtime_registry_summary)
    runtime_budget_current_data = read_json_if_exists(runtime_budget_current)
    runtime_budget_baseline_data = read_json_if_exists(runtime_budget_baseline)
    runtime_budget_refresh_data = read_json_if_exists(runtime_budget_refresh)
    runtime_budget_rerun_data = read_json_if_exists(runtime_budget_rerun)
    runtime_budget_proposal_data = read_json_if_exists(runtime_budget_proposal)
    runtime_budget_proposal_gate_data = read_json_if_exists(runtime_budget_proposal_gate)
    runtime_budget_registry_data = read_json_if_exists(runtime_budget_registry)
    runtime_budget_reproposal_history_data = read_json_if_exists(runtime_budget_reproposal_history)
    runtime_budget_registry_summary_data = read_json_if_exists(runtime_budget_registry_summary)
    runtime_watch_current_data = read_json_if_exists(runtime_watch_current)
    runtime_watch_refresh_data = read_json_if_exists(runtime_watch_refresh)
    runtime_watch_history_summary_manifest = (
        runtime_watch_history_index.with_name(f"{runtime_watch_history_index.stem}_summary.json")
        if runtime_watch_history_index is not None
        else None
    )
    runtime_watch_history_data = read_json_if_exists(runtime_watch_history_summary_manifest)
    runtime_watch_registry_data = read_json_if_exists(runtime_watch_registry)
    source_snapshot_manifest_data = read_json_if_exists(source_snapshot_manifest)
    staged_mirror_manifest_data = read_json_if_exists(staged_mirror_manifest)
    staged_mirror_verify_data = read_json_if_exists(staged_mirror_verify)
    ctest_inventory_release_data = read_json_if_exists(ctest_inventory_release)
    ctest_inventory_debug_data = read_json_if_exists(ctest_inventory_debug)
    ctest_inventory_asan_data = read_json_if_exists(ctest_inventory_asan)
    verification_release_data = read_json_if_exists(verification_release)
    verification_debug_data = read_json_if_exists(verification_debug)
    verification_asan_data = read_json_if_exists(verification_asan)
    published_snapshot_manifest_data = read_json_if_exists(published_snapshot_manifest)
    verification_closeout_data = read_json_if_exists(verification_closeout)
    publication_health_data = read_json_if_exists(publication_health)
    ops_summary_data = read_json_if_exists(ops_summary)
    approved_known_summary_paths = [
        resolve_manifest_json(Path(value).resolve())
        for value in list(args.approved_known_summary or [])
        if value
    ]
    approved_known_summaries = [read_json_if_exists(path) for path in approved_known_summary_paths]
    foreign_import_summary_paths = [
        resolve_manifest_json(Path(value).resolve())
        for value in list(args.foreign_import_summary or [])
        if value
    ]
    foreign_import_summaries = [read_json_if_exists(path) for path in foreign_import_summary_paths]
    runtime_approval_metadata_path = (
        runtime_baseline_manifest.with_name(f"{runtime_baseline_manifest.stem}_approval_metadata.json")
        if runtime_baseline_manifest is not None
        else None
    )
    runtime_approval_metadata = read_json_if_exists(runtime_approval_metadata_path)
    runtime_budget_approval_metadata_path = (
        runtime_budget_baseline.with_name(f"{runtime_budget_baseline.stem}_approval_metadata.json")
        if runtime_budget_baseline is not None
        else None
    )
    runtime_budget_approval_metadata = read_json_if_exists(runtime_budget_approval_metadata_path)
    active_runtime_baselines_list = {
        "manifest_version": "active_runtime_baselines_list_v1",
        "generated_at_utc": runtime_registry_summary_data.get("generated_at_utc")
        or runtime_registry_data.get("generated_at_utc")
        or datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "entries": runtime_registry_summary_data.get("active_baselines_by_fingerprint")
        or {
            str(entry.get("runtime_fingerprint_key", entry.get("fingerprint_key", ""))): [
                {
                    "baseline_id": entry.get("baseline_id"),
                    "baseline_tag": entry.get("baseline_tag"),
                    "runtime_baseline_manifest_path": entry.get("runtime_baseline_manifest_path"),
                }
            ]
            for entry in runtime_registry_data.get("entries", [])
            if isinstance(entry, dict) and str(entry.get("status", "")) == "active"
        },
    }
    pending_runtime_proposals_list = {
        "manifest_version": "pending_runtime_proposals_list_v1",
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "entries": [
            {
                "proposal_path": str(path),
                "proposal_needed": bool(data.get("proposal_needed", True)),
                "selected_baseline_id": data.get("selected_baseline_id"),
                "selected_baseline_tag": data.get("selected_baseline_tag"),
                "proposal_gate_verdict": data.get("proposal_gate_verdict"),
            }
            for path, data in [
                (runtime_proposal, runtime_proposal_data),
                (runtime_new_env_proposal, runtime_new_env_proposal_data),
            ]
            if path is not None and data
        ],
    }

    published_root = (
        Path(args.published_root).resolve()
        if args.published_root
        else default_published_bundle_root(artifact_root, args.phase)
    )
    bundle_root = published_root if args.use_published_snapshot else artifact_root / f"{args.phase}_evidence_bundle"
    if bundle_root.exists():
        shutil.rmtree(bundle_root)

    reports_dir = bundle_root / "reports"
    manifests_dir = bundle_root / "manifests"
    curated_dir = bundle_root / "curated"
    light_ops_dir = bundle_root / "light_ops"
    regressions_dir = bundle_root / "regressions"
    logs_dir = bundle_root / "logs"
    for path in (reports_dir, manifests_dir, curated_dir, light_ops_dir, regressions_dir, logs_dir):
        path.mkdir(parents=True, exist_ok=True)

    copied: dict[str, list[str]] = {
        "reports": [],
        "manifests": [],
        "curated": [],
        "light_ops": [],
        "regressions": [],
        "logs": [],
    }
    active_runtime_baselines_manifest = manifests_dir / "active_runtime_baselines_list.json"
    pending_runtime_proposals_manifest = manifests_dir / "pending_runtime_proposals_list.json"
    write_json(active_runtime_baselines_manifest, active_runtime_baselines_list)
    write_json(pending_runtime_proposals_manifest, pending_runtime_proposals_list)
    copied["manifests"].append(str(active_runtime_baselines_manifest))
    copied["manifests"].append(str(pending_runtime_proposals_manifest))
    active_runtime_baselines_light = light_ops_dir / "active_runtime_baselines_list.json"
    pending_runtime_proposals_light = light_ops_dir / "pending_runtime_proposals_list.json"
    write_json(active_runtime_baselines_light, active_runtime_baselines_list)
    write_json(pending_runtime_proposals_light, pending_runtime_proposals_list)
    copied["light_ops"].append(str(active_runtime_baselines_light))
    copied["light_ops"].append(str(pending_runtime_proposals_light))

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
    if runtime_registry_health is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_registry_health, manifests_dir, "runtime_registry_health_"))
    if runtime_history_compact is not None and runtime_history_compact.exists():
        copied["manifests"].extend(copy_manifest_bundle(runtime_history_compact, manifests_dir, "runtime_history_compact_"))
        compact_history_summary = runtime_history_compact.with_name(f"{runtime_history_compact.stem}_summary.json")
        copied["manifests"].extend(copy_manifest_bundle(compact_history_summary, manifests_dir, "runtime_history_compact_summary_"))
    elif runtime_history_index is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_history_index, manifests_dir, "runtime_history_"))
        history_summary = runtime_history_index.with_name(f"{runtime_history_index.stem}_summary.json")
        copied["manifests"].extend(copy_manifest_bundle(history_summary, manifests_dir, "runtime_history_summary_"))
    if runtime_proposal is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_proposal, manifests_dir, "runtime_proposal_"))
    if runtime_proposal_gate is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_proposal_gate, manifests_dir, "runtime_proposal_gate_"))
    if runtime_import_summary is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_import_summary, manifests_dir, "runtime_import_summary_"))
    if runtime_new_env_proposal is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_new_env_proposal, manifests_dir, "runtime_new_env_proposal_"))
    if runtime_new_env_proposal_gate is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_new_env_proposal_gate, manifests_dir, "runtime_new_env_proposal_gate_"))
    if runtime_new_env_archived_proposal is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_new_env_archived_proposal, manifests_dir, "runtime_new_env_archived_proposal_"))
    if runtime_new_env_approved_baseline is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_new_env_approved_baseline, manifests_dir, "runtime_new_env_approved_baseline_"))
    if runtime_registry_summary is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_registry_summary, manifests_dir, "runtime_registry_summary_"))
    if runtime_budget_current is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_current, manifests_dir, "runtime_budget_current_"))
    if runtime_budget_baseline is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_baseline, manifests_dir, "runtime_budget_baseline_"))
    if runtime_budget_refresh is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_refresh, manifests_dir, "runtime_budget_refresh_"))
    if runtime_budget_rerun is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_rerun, manifests_dir, "runtime_budget_rerun_"))
    if runtime_budget_proposal is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_proposal, manifests_dir, "runtime_budget_proposal_"))
    if runtime_budget_proposal_gate is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_proposal_gate, manifests_dir, "runtime_budget_proposal_gate_"))
    if runtime_budget_registry is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_registry, manifests_dir, "runtime_budget_registry_"))
    if runtime_budget_reproposal_history is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_reproposal_history, manifests_dir, "runtime_budget_reproposal_history_"))
    if runtime_budget_registry_summary is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_registry_summary, manifests_dir, "runtime_budget_registry_summary_"))
    if runtime_watch_current is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_watch_current, manifests_dir, "runtime_watch_current_"))
    if runtime_watch_refresh is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_watch_refresh, manifests_dir, "runtime_watch_refresh_"))
    if runtime_watch_history_index is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_watch_history_index, manifests_dir, "runtime_watch_history_"))
        watch_history_summary = runtime_watch_history_index.with_name(f"{runtime_watch_history_index.stem}_summary.json")
        copied["manifests"].extend(copy_manifest_bundle(watch_history_summary, manifests_dir, "runtime_watch_history_summary_"))
    if runtime_watch_registry is not None:
        copied["manifests"].extend(copy_manifest_bundle(runtime_watch_registry, manifests_dir, "runtime_watch_registry_"))
    if source_snapshot_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(source_snapshot_manifest, manifests_dir, "source_snapshot_"))
    if staged_mirror_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(staged_mirror_manifest, manifests_dir, "staged_mirror_"))
    if staged_mirror_verify is not None:
        copied["manifests"].extend(copy_manifest_bundle(staged_mirror_verify, manifests_dir, "staged_mirror_verify_"))
    if ctest_inventory_release is not None:
        copied["manifests"].extend(copy_manifest_bundle(ctest_inventory_release, manifests_dir, "ctest_inventory_release_"))
    if ctest_inventory_debug is not None:
        copied["manifests"].extend(copy_manifest_bundle(ctest_inventory_debug, manifests_dir, "ctest_inventory_debug_"))
    if ctest_inventory_asan is not None:
        copied["manifests"].extend(copy_manifest_bundle(ctest_inventory_asan, manifests_dir, "ctest_inventory_asan_"))
    if verification_release is not None:
        copied["manifests"].extend(copy_manifest_bundle(verification_release, manifests_dir, "verification_release_"))
    if verification_debug is not None:
        copied["manifests"].extend(copy_manifest_bundle(verification_debug, manifests_dir, "verification_debug_"))
    if verification_asan is not None:
        copied["manifests"].extend(copy_manifest_bundle(verification_asan, manifests_dir, "verification_asan_"))
    if published_snapshot_manifest is not None:
        copied["manifests"].extend(copy_manifest_bundle(published_snapshot_manifest, manifests_dir, "published_snapshot_"))
    if verification_closeout is not None:
        copied["manifests"].extend(copy_manifest_bundle(verification_closeout, manifests_dir, "verification_closeout_"))
    if publication_health is not None:
        copied["manifests"].extend(copy_manifest_bundle(publication_health, manifests_dir, "publication_health_"))
    if ops_summary is not None:
        copied["manifests"].extend(copy_manifest_bundle(ops_summary, manifests_dir, "policy_ops_summary_"))
    for index, source_path in enumerate(approved_known_summary_paths):
        if source_path is not None and source_path.exists():
            copied["manifests"].extend(copy_manifest_bundle(source_path, manifests_dir, f"approved_known_summary_{index + 1}_"))
    for index, source_path in enumerate(foreign_import_summary_paths):
        if source_path is not None and source_path.exists():
            copied["manifests"].extend(copy_manifest_bundle(source_path, manifests_dir, f"foreign_import_summary_{index + 1}_"))
    if known_env_import_summary is not None:
        copied["manifests"].extend(copy_manifest_bundle(known_env_import_summary, manifests_dir, "known_env_import_summary_"))
    if known_env_governance_policy is not None:
        copied["manifests"].extend(copy_manifest_bundle(known_env_governance_policy, manifests_dir, "known_env_governance_policy_"))
    if known_env_age_tick is not None:
        copied["manifests"].extend(copy_manifest_bundle(known_env_age_tick, manifests_dir, "known_env_age_tick_"))
    if known_env_reverify_plan is not None:
        copied["manifests"].extend(copy_manifest_bundle(known_env_reverify_plan, manifests_dir, "known_env_reverify_plan_"))
    if known_env_reverify_gate is not None:
        copied["manifests"].extend(copy_manifest_bundle(known_env_reverify_gate, manifests_dir, "known_env_reverify_gate_"))
    if known_env_reverify_apply is not None:
        copied["manifests"].extend(copy_manifest_bundle(known_env_reverify_apply, manifests_dir, "known_env_reverify_apply_"))
    if known_env_retire_plan is not None:
        copied["manifests"].extend(copy_manifest_bundle(known_env_retire_plan, manifests_dir, "known_env_retire_plan_"))
    if known_env_retire_apply is not None:
        copied["manifests"].extend(copy_manifest_bundle(known_env_retire_apply, manifests_dir, "known_env_retire_apply_"))
    if known_env_retire_metadata is not None:
        copied["manifests"].extend(copy_manifest_bundle(known_env_retire_metadata, manifests_dir, "known_env_retire_metadata_"))
    if current_env_governance_policy is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_governance_policy, manifests_dir, "current_env_governance_policy_"))
    if current_env_guardrail_policy is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_guardrail_policy, manifests_dir, "current_env_guardrail_policy_"))
    if current_env_watch_current is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_watch_current, manifests_dir, "current_env_watch_current_"))
    if current_env_watch_refresh is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_watch_refresh, manifests_dir, "current_env_watch_refresh_"))
    if current_env_watch_history is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_watch_history, manifests_dir, "current_env_watch_history_"))
    if current_env_age_tick is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_age_tick, manifests_dir, "current_env_age_tick_"))
    if current_env_watch_plan is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_watch_plan, manifests_dir, "current_env_watch_plan_"))
    if current_env_trigger_gate is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_trigger_gate, manifests_dir, "current_env_trigger_gate_"))
    if current_env_due is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_due, manifests_dir, "current_env_due_"))
    if current_env_reproposal_plan is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_reproposal_plan, manifests_dir, "current_env_reproposal_plan_"))
    if ops_agenda is not None:
        copied["manifests"].extend(copy_manifest_bundle(ops_agenda, manifests_dir, "ops_agenda_"))
    if current_env_watch_execute is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_watch_execute, manifests_dir, "current_env_watch_execute_"))
    if current_env_watch_apply is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_watch_apply, manifests_dir, "current_env_watch_apply_"))
    if current_env_reproposal_execute is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_reproposal_execute, manifests_dir, "current_env_reproposal_execute_"))
    if current_env_action_ledger is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_action_ledger, manifests_dir, "current_env_action_ledger_"))
    if current_env_retry_plan is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_retry_plan, manifests_dir, "current_env_retry_plan_"))
    if current_env_reproposal_handoff is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_reproposal_handoff, manifests_dir, "current_env_reproposal_handoff_"))
    if current_env_operator_decision is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_operator_decision, manifests_dir, "current_env_operator_decision_"))
    if current_env_operator_decision_apply is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_operator_decision_apply, manifests_dir, "current_env_operator_decision_apply_"))
    if current_env_action_ledger_compact is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_action_ledger_compact, manifests_dir, "current_env_action_ledger_compact_"))
    if current_env_action_ledger_archive is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_action_ledger_archive, manifests_dir, "current_env_action_ledger_archive_"))
    if current_env_approval_runbook is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_approval_runbook, manifests_dir, "current_env_approval_runbook_"))
    if current_env_approval_execution is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_approval_execution, manifests_dir, "current_env_approval_execution_"))
    if current_env_approval_link is not None:
        copied["manifests"].extend(copy_manifest_bundle(current_env_approval_link, manifests_dir, "current_env_approval_link_"))
    if operator_runbook_index is not None:
        copied["manifests"].extend(copy_manifest_bundle(operator_runbook_index, manifests_dir, "operator_runbook_index_"))
    if operator_runbook_catalog is not None:
        copied["manifests"].extend(copy_manifest_bundle(operator_runbook_catalog, manifests_dir, "operator_runbook_catalog_"))
    if operator_decision_metadata_audit is not None:
        copied["manifests"].extend(copy_manifest_bundle(operator_decision_metadata_audit, manifests_dir, "operator_decision_metadata_audit_"))
    if operator_runbook_replay is not None:
        copied["manifests"].extend(copy_manifest_bundle(operator_runbook_replay, manifests_dir, "operator_runbook_replay_"))
    if operator_runbook_retention_policy is not None:
        copied["manifests"].extend(copy_manifest_bundle(operator_runbook_retention_policy, manifests_dir, "operator_runbook_retention_policy_"))
    if operator_runbook_pruned_catalog is not None:
        copied["manifests"].extend(copy_manifest_bundle(operator_runbook_pruned_catalog, manifests_dir, "operator_runbook_pruned_catalog_"))
    if operator_runbook_archive is not None:
        copied["manifests"].extend(copy_manifest_bundle(operator_runbook_archive, manifests_dir, "operator_runbook_archive_"))
    if operator_runbook_prune_summary is not None:
        copied["manifests"].extend(copy_manifest_bundle(operator_runbook_prune_summary, manifests_dir, "operator_runbook_prune_summary_"))
    if operator_runbook_lifecycle_validation is not None:
        copied["manifests"].extend(copy_manifest_bundle(operator_runbook_lifecycle_validation, manifests_dir, "operator_runbook_lifecycle_validation_"))
    for path, prefix in (
        (operator_runbook_pointer_audit, "operator_runbook_pointer_audit_"),
        (operator_runbook_provenance_migration, "operator_runbook_provenance_migration_"),
        (operator_runbook_migrated_catalog, "operator_runbook_migrated_catalog_"),
        (operator_runbook_migrated_ledger, "operator_runbook_migrated_ledger_"),
        (operator_runbook_lifecycle_validation_before, "operator_runbook_lifecycle_validation_before_"),
        (operator_runbook_lifecycle_validation_after, "operator_runbook_lifecycle_validation_after_"),
        (operator_artifact_path_policy_lint, "operator_artifact_path_policy_lint_"),
    ):
        if path is not None:
            copied["manifests"].extend(copy_manifest_bundle(path, manifests_dir, prefix))
    if integrated_approval_mutation_audit is not None:
        copied["manifests"].extend(copy_manifest_bundle(integrated_approval_mutation_audit, manifests_dir, "integrated_approval_mutation_audit_"))
    if staged_materialization_transaction is not None:
        copied["manifests"].extend(copy_manifest_bundle(staged_materialization_transaction, manifests_dir, "staged_materialization_transaction_"))
    if source_health_preflight is not None:
        copied["manifests"].extend(copy_manifest_bundle(source_health_preflight, manifests_dir, "source_health_preflight_"))
    if source_health_action_plan is not None:
        copied["manifests"].extend(copy_manifest_bundle(source_health_action_plan, manifests_dir, "source_health_action_plan_"))
    if staged_materialization is not None:
        copied["manifests"].extend(copy_manifest_bundle(staged_materialization, manifests_dir, "staged_materialization_"))
    for current_env_next_cycle_summary in current_env_next_cycle_summaries:
        copied["manifests"].extend(copy_manifest_bundle(current_env_next_cycle_summary, manifests_dir, "current_env_next_cycle_"))
    if runtime_approval_metadata_path is not None and runtime_approval_metadata_path.exists():
        copied["manifests"].extend(copy_manifest_bundle(runtime_approval_metadata_path, manifests_dir, "runtime_approval_"))
    if runtime_budget_approval_metadata_path is not None and runtime_budget_approval_metadata_path.exists():
        copied["manifests"].extend(copy_manifest_bundle(runtime_budget_approval_metadata_path, manifests_dir, "runtime_budget_approval_"))

    # Published snapshot mode is intentionally manifest-first. Copying the full
    # recursive log tree into an iCloud-backed published root can stall on large
    # nightly artifacts, while the operator workflows in this phase only need
    # the summarized manifests and bundle indexes.
    if not args.use_published_snapshot:
        for source in sorted(artifact_root.rglob("logs")):
            if source == logs_dir or not source.is_dir():
                continue
            if is_nested_evidence_bundle_path(source):
                continue
            relative = source.relative_to(artifact_root)
            if not should_copy_log_source(relative, args.phase):
                continue
            copied["logs"].extend(copy_tree_if_exists(source, logs_dir / relative))

    curated_root = artifact_root / "curated"
    if curated_root.exists():
        for child in sorted(curated_root.iterdir()):
            if not should_copy_phase_scoped_name(child.name, args.phase):
                continue
            copied["curated"].extend(copy_tree_if_exists(child, curated_dir / child.name))
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
    if runtime_registry_health is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_registry_health, curated_dir, "runtime_registry_health_"))
    if runtime_history_compact is not None and runtime_history_compact.exists():
        copied["curated"].extend(copy_manifest_bundle(runtime_history_compact, curated_dir, "runtime_history_compact_"))
        compact_history_summary = runtime_history_compact.with_name(f"{runtime_history_compact.stem}_summary.json")
        copied["curated"].extend(copy_manifest_bundle(compact_history_summary, curated_dir, "runtime_history_compact_summary_"))
    elif runtime_history_index is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_history_index, curated_dir, "runtime_history_"))
        history_summary = runtime_history_index.with_name(f"{runtime_history_index.stem}_summary.json")
        copied["curated"].extend(copy_manifest_bundle(history_summary, curated_dir, "runtime_history_summary_"))
    if runtime_proposal is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_proposal, curated_dir, "runtime_proposal_"))
    if runtime_proposal_gate is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_proposal_gate, curated_dir, "runtime_proposal_gate_"))
    if runtime_import_summary is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_import_summary, curated_dir, "runtime_import_summary_"))
    if runtime_new_env_proposal is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_new_env_proposal, curated_dir, "runtime_new_env_proposal_"))
    if runtime_new_env_proposal_gate is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_new_env_proposal_gate, curated_dir, "runtime_new_env_proposal_gate_"))
    if runtime_new_env_archived_proposal is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_new_env_archived_proposal, curated_dir, "runtime_new_env_archived_proposal_"))
    if runtime_new_env_approved_baseline is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_new_env_approved_baseline, curated_dir, "runtime_new_env_approved_baseline_"))
    if runtime_registry_summary is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_registry_summary, curated_dir, "runtime_registry_summary_"))
    if runtime_budget_current is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_current, curated_dir, "runtime_budget_current_"))
    if runtime_budget_baseline is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_baseline, curated_dir, "runtime_budget_baseline_"))
    if runtime_budget_refresh is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_refresh, curated_dir, "runtime_budget_refresh_"))
    if runtime_budget_rerun is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_rerun, curated_dir, "runtime_budget_rerun_"))
    if runtime_budget_proposal is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_proposal, curated_dir, "runtime_budget_proposal_"))
    if runtime_budget_proposal_gate is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_proposal_gate, curated_dir, "runtime_budget_proposal_gate_"))
    if runtime_budget_registry is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_registry, curated_dir, "runtime_budget_registry_"))
    if runtime_budget_reproposal_history is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_reproposal_history, curated_dir, "runtime_budget_reproposal_history_"))
    if runtime_budget_registry_summary is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_registry_summary, curated_dir, "runtime_budget_registry_summary_"))
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
    if runtime_watch_registry is not None:
        copied["curated"].extend(copy_manifest_bundle(runtime_watch_registry, curated_dir, "runtime_watch_registry_"))
    if source_snapshot_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(source_snapshot_manifest, curated_dir, "source_snapshot_"))
    if staged_mirror_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(staged_mirror_manifest, curated_dir, "staged_mirror_"))
    if staged_mirror_verify is not None:
        copied["curated"].extend(copy_manifest_bundle(staged_mirror_verify, curated_dir, "staged_mirror_verify_"))
    if ctest_inventory_release is not None:
        copied["curated"].extend(copy_manifest_bundle(ctest_inventory_release, curated_dir, "ctest_inventory_release_"))
    if ctest_inventory_debug is not None:
        copied["curated"].extend(copy_manifest_bundle(ctest_inventory_debug, curated_dir, "ctest_inventory_debug_"))
    if ctest_inventory_asan is not None:
        copied["curated"].extend(copy_manifest_bundle(ctest_inventory_asan, curated_dir, "ctest_inventory_asan_"))
    if verification_release is not None:
        copied["curated"].extend(copy_manifest_bundle(verification_release, curated_dir, "verification_release_"))
    if verification_debug is not None:
        copied["curated"].extend(copy_manifest_bundle(verification_debug, curated_dir, "verification_debug_"))
    if verification_asan is not None:
        copied["curated"].extend(copy_manifest_bundle(verification_asan, curated_dir, "verification_asan_"))
    if published_snapshot_manifest is not None:
        copied["curated"].extend(copy_manifest_bundle(published_snapshot_manifest, curated_dir, "published_snapshot_"))
    if verification_closeout is not None:
        copied["curated"].extend(copy_manifest_bundle(verification_closeout, curated_dir, "verification_closeout_"))
    if publication_health is not None:
        copied["curated"].extend(copy_manifest_bundle(publication_health, curated_dir, "publication_health_"))
    if ops_summary is not None:
        copied["curated"].extend(copy_manifest_bundle(ops_summary, curated_dir, "policy_ops_summary_"))
    for index, source_path in enumerate(approved_known_summary_paths):
        if source_path is not None and source_path.exists():
            copied["curated"].extend(copy_manifest_bundle(source_path, curated_dir, f"approved_known_summary_{index + 1}_"))
    for index, source_path in enumerate(foreign_import_summary_paths):
        if source_path is not None and source_path.exists():
            copied["curated"].extend(copy_manifest_bundle(source_path, curated_dir, f"foreign_import_summary_{index + 1}_"))
    if known_env_import_summary is not None:
        copied["curated"].extend(copy_manifest_bundle(known_env_import_summary, curated_dir, "known_env_import_summary_"))
    if known_env_governance_policy is not None:
        copied["curated"].extend(copy_manifest_bundle(known_env_governance_policy, curated_dir, "known_env_governance_policy_"))
    if known_env_age_tick is not None:
        copied["curated"].extend(copy_manifest_bundle(known_env_age_tick, curated_dir, "known_env_age_tick_"))
    if known_env_reverify_plan is not None:
        copied["curated"].extend(copy_manifest_bundle(known_env_reverify_plan, curated_dir, "known_env_reverify_plan_"))
    if known_env_reverify_gate is not None:
        copied["curated"].extend(copy_manifest_bundle(known_env_reverify_gate, curated_dir, "known_env_reverify_gate_"))
    if known_env_reverify_apply is not None:
        copied["curated"].extend(copy_manifest_bundle(known_env_reverify_apply, curated_dir, "known_env_reverify_apply_"))
    if known_env_retire_plan is not None:
        copied["curated"].extend(copy_manifest_bundle(known_env_retire_plan, curated_dir, "known_env_retire_plan_"))
    if known_env_retire_apply is not None:
        copied["curated"].extend(copy_manifest_bundle(known_env_retire_apply, curated_dir, "known_env_retire_apply_"))
    if known_env_retire_metadata is not None:
        copied["curated"].extend(copy_manifest_bundle(known_env_retire_metadata, curated_dir, "known_env_retire_metadata_"))
    if current_env_governance_policy is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_governance_policy, curated_dir, "current_env_governance_policy_"))
    if current_env_guardrail_policy is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_guardrail_policy, curated_dir, "current_env_guardrail_policy_"))
    if current_env_watch_current is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_watch_current, curated_dir, "current_env_watch_current_"))
    if current_env_watch_refresh is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_watch_refresh, curated_dir, "current_env_watch_refresh_"))
    if current_env_watch_history is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_watch_history, curated_dir, "current_env_watch_history_"))
    if current_env_age_tick is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_age_tick, curated_dir, "current_env_age_tick_"))
    if current_env_watch_plan is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_watch_plan, curated_dir, "current_env_watch_plan_"))
    if current_env_trigger_gate is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_trigger_gate, curated_dir, "current_env_trigger_gate_"))
    if current_env_due is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_due, curated_dir, "current_env_due_"))
    if current_env_reproposal_plan is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_reproposal_plan, curated_dir, "current_env_reproposal_plan_"))
    if ops_agenda is not None:
        copied["curated"].extend(copy_manifest_bundle(ops_agenda, curated_dir, "ops_agenda_"))
    if current_env_watch_execute is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_watch_execute, curated_dir, "current_env_watch_execute_"))
    if current_env_watch_apply is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_watch_apply, curated_dir, "current_env_watch_apply_"))
    if current_env_reproposal_execute is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_reproposal_execute, curated_dir, "current_env_reproposal_execute_"))
    if current_env_action_ledger is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_action_ledger, curated_dir, "current_env_action_ledger_"))
    if current_env_retry_plan is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_retry_plan, curated_dir, "current_env_retry_plan_"))
    if current_env_reproposal_handoff is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_reproposal_handoff, curated_dir, "current_env_reproposal_handoff_"))
    if current_env_operator_decision is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_operator_decision, curated_dir, "current_env_operator_decision_"))
    if current_env_operator_decision_apply is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_operator_decision_apply, curated_dir, "current_env_operator_decision_apply_"))
    if current_env_action_ledger_compact is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_action_ledger_compact, curated_dir, "current_env_action_ledger_compact_"))
    if current_env_action_ledger_archive is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_action_ledger_archive, curated_dir, "current_env_action_ledger_archive_"))
    if current_env_approval_runbook is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_approval_runbook, curated_dir, "current_env_approval_runbook_"))
    if current_env_approval_execution is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_approval_execution, curated_dir, "current_env_approval_execution_"))
    if current_env_approval_link is not None:
        copied["curated"].extend(copy_manifest_bundle(current_env_approval_link, curated_dir, "current_env_approval_link_"))
    if operator_runbook_index is not None:
        copied["curated"].extend(copy_manifest_bundle(operator_runbook_index, curated_dir, "operator_runbook_index_"))
    if operator_runbook_catalog is not None:
        copied["curated"].extend(copy_manifest_bundle(operator_runbook_catalog, curated_dir, "operator_runbook_catalog_"))
    if operator_decision_metadata_audit is not None:
        copied["curated"].extend(copy_manifest_bundle(operator_decision_metadata_audit, curated_dir, "operator_decision_metadata_audit_"))
    if operator_runbook_replay is not None:
        copied["curated"].extend(copy_manifest_bundle(operator_runbook_replay, curated_dir, "operator_runbook_replay_"))
    if operator_runbook_retention_policy is not None:
        copied["curated"].extend(copy_manifest_bundle(operator_runbook_retention_policy, curated_dir, "operator_runbook_retention_policy_"))
    if operator_runbook_pruned_catalog is not None:
        copied["curated"].extend(copy_manifest_bundle(operator_runbook_pruned_catalog, curated_dir, "operator_runbook_pruned_catalog_"))
    if operator_runbook_archive is not None:
        copied["curated"].extend(copy_manifest_bundle(operator_runbook_archive, curated_dir, "operator_runbook_archive_"))
    if operator_runbook_prune_summary is not None:
        copied["curated"].extend(copy_manifest_bundle(operator_runbook_prune_summary, curated_dir, "operator_runbook_prune_summary_"))
    if operator_runbook_lifecycle_validation is not None:
        copied["curated"].extend(copy_manifest_bundle(operator_runbook_lifecycle_validation, curated_dir, "operator_runbook_lifecycle_validation_"))
    for path, prefix in (
        (operator_runbook_pointer_audit, "operator_runbook_pointer_audit_"),
        (operator_runbook_provenance_migration, "operator_runbook_provenance_migration_"),
        (operator_runbook_migrated_catalog, "operator_runbook_migrated_catalog_"),
        (operator_runbook_migrated_ledger, "operator_runbook_migrated_ledger_"),
        (operator_runbook_lifecycle_validation_before, "operator_runbook_lifecycle_validation_before_"),
        (operator_runbook_lifecycle_validation_after, "operator_runbook_lifecycle_validation_after_"),
        (operator_artifact_path_policy_lint, "operator_artifact_path_policy_lint_"),
    ):
        if path is not None:
            copied["curated"].extend(copy_manifest_bundle(path, curated_dir, prefix))
    if integrated_approval_mutation_audit is not None:
        copied["curated"].extend(copy_manifest_bundle(integrated_approval_mutation_audit, curated_dir, "integrated_approval_mutation_audit_"))
    if staged_materialization_transaction is not None:
        copied["curated"].extend(copy_manifest_bundle(staged_materialization_transaction, curated_dir, "staged_materialization_transaction_"))
    if source_health_preflight is not None:
        copied["curated"].extend(copy_manifest_bundle(source_health_preflight, curated_dir, "source_health_preflight_"))
    if source_health_action_plan is not None:
        copied["curated"].extend(copy_manifest_bundle(source_health_action_plan, curated_dir, "source_health_action_plan_"))
    if staged_materialization is not None:
        copied["curated"].extend(copy_manifest_bundle(staged_materialization, curated_dir, "staged_materialization_"))
    for current_env_next_cycle_summary in current_env_next_cycle_summaries:
        copied["curated"].extend(copy_manifest_bundle(current_env_next_cycle_summary, curated_dir, "current_env_next_cycle_"))
    if runtime_approval_metadata_path is not None and runtime_approval_metadata_path.exists():
        copied["curated"].extend(copy_manifest_bundle(runtime_approval_metadata_path, curated_dir, "runtime_approval_"))
    if runtime_budget_approval_metadata_path is not None and runtime_budget_approval_metadata_path.exists():
        copied["curated"].extend(copy_manifest_bundle(runtime_budget_approval_metadata_path, curated_dir, "runtime_budget_approval_"))
    refresh_summary = None
    if refresh_manifest is not None and refresh_manifest.suffix == ".json":
        refresh_summary = refresh_manifest.with_name(f"{refresh_manifest.stem}.summary.txt")
        copied["curated"].extend(copy_tree_if_exists(refresh_summary, curated_dir / refresh_summary.name))
    manifest_linked_summaries = parse_manifest_summary_paths(current_manifest_data, args.phase)
    if not manifest_linked_summaries:
        applicability_summary = latest_matching_file(
            artifact_root, "planner_tie_mixed_organic_applicability_audit.summary.txt", args.phase
        )
        compare_ready_summary = latest_matching_file(
            artifact_root, "compare_ready_lineage_audit.summary.txt", args.phase
        )
        manifest_linked_summaries = [
            path for path in (applicability_summary, compare_ready_summary) if path is not None
        ]
    for summary_path in manifest_linked_summaries:
        if artifact_root not in summary_path.parents and repo_root not in summary_path.parents:
            continue
        copied["curated"].extend(copy_tree_if_exists(summary_path, curated_dir / summary_path.name))

    copied["light_ops"].extend(copy_tree_if_exists(report_path, light_ops_dir / report_path.name))
    if baseline_manifest is not None:
        copied["light_ops"].extend(copy_manifest_bundle(baseline_manifest, light_ops_dir, "baseline_"))
    if current_manifest is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_manifest, light_ops_dir, "current_"))
    if refresh_manifest is not None:
        copied["light_ops"].extend(copy_manifest_bundle(refresh_manifest, light_ops_dir, "refresh_"))
    if rerun_plan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(rerun_plan, light_ops_dir, "rerun_"))
    if pipeline_quick_summary is not None:
        copied["light_ops"].extend(copy_manifest_bundle(pipeline_quick_summary, light_ops_dir, "pipeline_quick_"))
    if pipeline_summary is not None:
        copied["light_ops"].extend(copy_manifest_bundle(pipeline_summary, light_ops_dir, "pipeline_"))
    if pipeline_matrix_summary is not None:
        copied["light_ops"].extend(copy_manifest_bundle(pipeline_matrix_summary, light_ops_dir, "pipeline_matrix_"))
    if runtime_baseline_manifest is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_baseline_manifest, light_ops_dir, "runtime_baseline_"))
    if runtime_current_manifest is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_current_manifest, light_ops_dir, "runtime_current_"))
    if runtime_refresh_manifest is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_refresh_manifest, light_ops_dir, "runtime_refresh_"))
    if runtime_rerun_plan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_rerun_plan, light_ops_dir, "runtime_rerun_"))
    if runtime_registry is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_registry, light_ops_dir, "runtime_registry_"))
    if runtime_registry_health is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_registry_health, light_ops_dir, "runtime_registry_health_"))
    if runtime_history_compact is not None and runtime_history_compact.exists():
        copied["light_ops"].extend(copy_manifest_bundle(runtime_history_compact, light_ops_dir, "runtime_history_compact_"))
        compact_history_summary = runtime_history_compact.with_name(f"{runtime_history_compact.stem}_summary.json")
        copied["light_ops"].extend(copy_manifest_bundle(compact_history_summary, light_ops_dir, "runtime_history_compact_summary_"))
    elif runtime_history_summary_manifest is not None and runtime_history_summary_manifest.exists():
        copied["light_ops"].extend(copy_manifest_bundle(runtime_history_summary_manifest, light_ops_dir, "runtime_history_summary_"))
    if runtime_proposal is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_proposal, light_ops_dir, "runtime_proposal_"))
    if runtime_proposal_gate is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_proposal_gate, light_ops_dir, "runtime_proposal_gate_"))
    if runtime_import_summary is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_import_summary, light_ops_dir, "runtime_import_summary_"))
    if runtime_new_env_proposal is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_new_env_proposal, light_ops_dir, "runtime_new_env_proposal_"))
    if runtime_new_env_proposal_gate is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_new_env_proposal_gate, light_ops_dir, "runtime_new_env_proposal_gate_"))
    if runtime_new_env_archived_proposal is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_new_env_archived_proposal, light_ops_dir, "runtime_new_env_archived_proposal_"))
    if runtime_new_env_approved_baseline is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_new_env_approved_baseline, light_ops_dir, "runtime_new_env_approved_baseline_"))
    if runtime_registry_summary is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_registry_summary, light_ops_dir, "runtime_registry_summary_"))
    if runtime_budget_current is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_current, light_ops_dir, "runtime_budget_current_"))
    if runtime_budget_baseline is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_baseline, light_ops_dir, "runtime_budget_baseline_"))
    if runtime_budget_refresh is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_refresh, light_ops_dir, "runtime_budget_refresh_"))
    if runtime_budget_rerun is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_rerun, light_ops_dir, "runtime_budget_rerun_"))
    if runtime_budget_proposal is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_proposal, light_ops_dir, "runtime_budget_proposal_"))
    if runtime_budget_proposal_gate is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_proposal_gate, light_ops_dir, "runtime_budget_proposal_gate_"))
    if runtime_budget_registry is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_registry, light_ops_dir, "runtime_budget_registry_"))
    if runtime_budget_reproposal_history is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_reproposal_history, light_ops_dir, "runtime_budget_reproposal_history_"))
    if runtime_budget_registry_summary is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_registry_summary, light_ops_dir, "runtime_budget_registry_summary_"))
    if runtime_watch_current is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_watch_current, light_ops_dir, "runtime_watch_current_"))
    if runtime_watch_refresh is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_watch_refresh, light_ops_dir, "runtime_watch_refresh_"))
    if runtime_watch_registry is not None:
        copied["light_ops"].extend(copy_manifest_bundle(runtime_watch_registry, light_ops_dir, "runtime_watch_registry_"))
    if source_snapshot_manifest is not None:
        copied["light_ops"].extend(copy_manifest_bundle(source_snapshot_manifest, light_ops_dir, "source_snapshot_"))
    if staged_mirror_manifest is not None:
        copied["light_ops"].extend(copy_manifest_bundle(staged_mirror_manifest, light_ops_dir, "staged_mirror_"))
    if staged_mirror_verify is not None:
        copied["light_ops"].extend(copy_manifest_bundle(staged_mirror_verify, light_ops_dir, "staged_mirror_verify_"))
    if ctest_inventory_release is not None:
        copied["light_ops"].extend(copy_manifest_bundle(ctest_inventory_release, light_ops_dir, "ctest_inventory_release_"))
    if ctest_inventory_debug is not None:
        copied["light_ops"].extend(copy_manifest_bundle(ctest_inventory_debug, light_ops_dir, "ctest_inventory_debug_"))
    if ctest_inventory_asan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(ctest_inventory_asan, light_ops_dir, "ctest_inventory_asan_"))
    if verification_release is not None:
        copied["light_ops"].extend(copy_manifest_bundle(verification_release, light_ops_dir, "verification_release_"))
    if verification_debug is not None:
        copied["light_ops"].extend(copy_manifest_bundle(verification_debug, light_ops_dir, "verification_debug_"))
    if verification_asan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(verification_asan, light_ops_dir, "verification_asan_"))
    if published_snapshot_manifest is not None:
        copied["light_ops"].extend(copy_manifest_bundle(published_snapshot_manifest, light_ops_dir, "published_snapshot_"))
    if verification_closeout is not None:
        copied["light_ops"].extend(copy_manifest_bundle(verification_closeout, light_ops_dir, "verification_closeout_"))
    if publication_health is not None:
        copied["light_ops"].extend(copy_manifest_bundle(publication_health, light_ops_dir, "publication_health_"))
    if ops_summary is not None:
        copied["light_ops"].extend(copy_manifest_bundle(ops_summary, light_ops_dir, "policy_ops_summary_"))
    for index, source_path in enumerate(approved_known_summary_paths):
        if source_path is not None and source_path.exists():
            copied["light_ops"].extend(copy_manifest_bundle(source_path, light_ops_dir, f"approved_known_summary_{index + 1}_"))
    if known_env_import_summary is not None:
        copied["light_ops"].extend(copy_manifest_bundle(known_env_import_summary, light_ops_dir, "known_env_import_summary_"))
    if known_env_governance_policy is not None:
        copied["light_ops"].extend(copy_manifest_bundle(known_env_governance_policy, light_ops_dir, "known_env_governance_policy_"))
    if known_env_age_tick is not None:
        copied["light_ops"].extend(copy_manifest_bundle(known_env_age_tick, light_ops_dir, "known_env_age_tick_"))
    if known_env_reverify_plan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(known_env_reverify_plan, light_ops_dir, "known_env_reverify_plan_"))
    if known_env_reverify_gate is not None:
        copied["light_ops"].extend(copy_manifest_bundle(known_env_reverify_gate, light_ops_dir, "known_env_reverify_gate_"))
    if known_env_reverify_apply is not None:
        copied["light_ops"].extend(copy_manifest_bundle(known_env_reverify_apply, light_ops_dir, "known_env_reverify_apply_"))
    if known_env_retire_plan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(known_env_retire_plan, light_ops_dir, "known_env_retire_plan_"))
    if known_env_retire_apply is not None:
        copied["light_ops"].extend(copy_manifest_bundle(known_env_retire_apply, light_ops_dir, "known_env_retire_apply_"))
    if known_env_retire_metadata is not None:
        copied["light_ops"].extend(copy_manifest_bundle(known_env_retire_metadata, light_ops_dir, "known_env_retire_metadata_"))
    if current_env_governance_policy is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_governance_policy, light_ops_dir, "current_env_governance_policy_"))
    if current_env_guardrail_policy is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_guardrail_policy, light_ops_dir, "current_env_guardrail_policy_"))
    if current_env_watch_current is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_watch_current, light_ops_dir, "current_env_watch_current_"))
    if current_env_watch_refresh is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_watch_refresh, light_ops_dir, "current_env_watch_refresh_"))
    if current_env_watch_history is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_watch_history, light_ops_dir, "current_env_watch_history_"))
    if current_env_age_tick is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_age_tick, light_ops_dir, "current_env_age_tick_"))
    if current_env_watch_plan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_watch_plan, light_ops_dir, "current_env_watch_plan_"))
    if current_env_trigger_gate is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_trigger_gate, light_ops_dir, "current_env_trigger_gate_"))
    if current_env_due is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_due, light_ops_dir, "current_env_due_"))
    if current_env_reproposal_plan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_reproposal_plan, light_ops_dir, "current_env_reproposal_plan_"))
    if ops_agenda is not None:
        copied["light_ops"].extend(copy_manifest_bundle(ops_agenda, light_ops_dir, "ops_agenda_"))
    if current_env_watch_execute is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_watch_execute, light_ops_dir, "current_env_watch_execute_"))
    if current_env_watch_apply is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_watch_apply, light_ops_dir, "current_env_watch_apply_"))
    if current_env_reproposal_execute is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_reproposal_execute, light_ops_dir, "current_env_reproposal_execute_"))
    if current_env_action_ledger is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_action_ledger, light_ops_dir, "current_env_action_ledger_"))
    if current_env_retry_plan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_retry_plan, light_ops_dir, "current_env_retry_plan_"))
    if current_env_reproposal_handoff is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_reproposal_handoff, light_ops_dir, "current_env_reproposal_handoff_"))
    if current_env_operator_decision is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_operator_decision, light_ops_dir, "current_env_operator_decision_"))
    if current_env_operator_decision_apply is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_operator_decision_apply, light_ops_dir, "current_env_operator_decision_apply_"))
    if current_env_action_ledger_compact is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_action_ledger_compact, light_ops_dir, "current_env_action_ledger_compact_"))
    if current_env_action_ledger_archive is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_action_ledger_archive, light_ops_dir, "current_env_action_ledger_archive_"))
    if current_env_approval_runbook is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_approval_runbook, light_ops_dir, "current_env_approval_runbook_"))
    if current_env_approval_execution is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_approval_execution, light_ops_dir, "current_env_approval_execution_"))
    if current_env_approval_link is not None:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_approval_link, light_ops_dir, "current_env_approval_link_"))
    if operator_runbook_index is not None:
        copied["light_ops"].extend(copy_manifest_bundle(operator_runbook_index, light_ops_dir, "operator_runbook_index_"))
    if operator_runbook_catalog is not None:
        copied["light_ops"].extend(copy_manifest_bundle(operator_runbook_catalog, light_ops_dir, "operator_runbook_catalog_"))
    if operator_decision_metadata_audit is not None:
        copied["light_ops"].extend(copy_manifest_bundle(operator_decision_metadata_audit, light_ops_dir, "operator_decision_metadata_audit_"))
    if operator_runbook_replay is not None:
        copied["light_ops"].extend(copy_manifest_bundle(operator_runbook_replay, light_ops_dir, "operator_runbook_replay_"))
    if operator_runbook_retention_policy is not None:
        copied["light_ops"].extend(copy_manifest_bundle(operator_runbook_retention_policy, light_ops_dir, "operator_runbook_retention_policy_"))
    if operator_runbook_pruned_catalog is not None:
        copied["light_ops"].extend(copy_manifest_bundle(operator_runbook_pruned_catalog, light_ops_dir, "operator_runbook_pruned_catalog_"))
    if operator_runbook_archive is not None:
        copied["light_ops"].extend(copy_manifest_bundle(operator_runbook_archive, light_ops_dir, "operator_runbook_archive_"))
    if operator_runbook_prune_summary is not None:
        copied["light_ops"].extend(copy_manifest_bundle(operator_runbook_prune_summary, light_ops_dir, "operator_runbook_prune_summary_"))
    if operator_runbook_lifecycle_validation is not None:
        copied["light_ops"].extend(copy_manifest_bundle(operator_runbook_lifecycle_validation, light_ops_dir, "operator_runbook_lifecycle_validation_"))
    for path, prefix in (
        (operator_runbook_pointer_audit, "operator_runbook_pointer_audit_"),
        (operator_runbook_provenance_migration, "operator_runbook_provenance_migration_"),
        (operator_runbook_migrated_catalog, "operator_runbook_migrated_catalog_"),
        (operator_runbook_migrated_ledger, "operator_runbook_migrated_ledger_"),
        (operator_runbook_lifecycle_validation_before, "operator_runbook_lifecycle_validation_before_"),
        (operator_runbook_lifecycle_validation_after, "operator_runbook_lifecycle_validation_after_"),
        (operator_artifact_path_policy_lint, "operator_artifact_path_policy_lint_"),
    ):
        if path is not None:
            copied["light_ops"].extend(copy_manifest_bundle(path, light_ops_dir, prefix))
    if integrated_approval_mutation_audit is not None:
        copied["light_ops"].extend(copy_manifest_bundle(integrated_approval_mutation_audit, light_ops_dir, "integrated_approval_mutation_audit_"))
    if staged_materialization_transaction is not None:
        copied["light_ops"].extend(copy_manifest_bundle(staged_materialization_transaction, light_ops_dir, "staged_materialization_transaction_"))
    if source_health_preflight is not None:
        copied["light_ops"].extend(copy_manifest_bundle(source_health_preflight, light_ops_dir, "source_health_preflight_"))
    if source_health_action_plan is not None:
        copied["light_ops"].extend(copy_manifest_bundle(source_health_action_plan, light_ops_dir, "source_health_action_plan_"))
    if staged_materialization is not None:
        copied["light_ops"].extend(copy_manifest_bundle(staged_materialization, light_ops_dir, "staged_materialization_"))
    for current_env_next_cycle_summary in current_env_next_cycle_summaries:
        copied["light_ops"].extend(copy_manifest_bundle(current_env_next_cycle_summary, light_ops_dir, "current_env_next_cycle_"))
    if runtime_budget_approval_metadata_path is not None and runtime_budget_approval_metadata_path.exists():
        copied["light_ops"].extend(copy_manifest_bundle(runtime_budget_approval_metadata_path, light_ops_dir, "runtime_budget_approval_"))
    if args.use_published_snapshot:
        regression_roots = []
        artifact_regressions_root = artifact_root / "regressions"
        repo_regressions_root = repo_root / "tests" / "regressions"
        if artifact_regressions_root.exists():
            regression_roots.append(artifact_regressions_root)
        elif repo_regressions_root.exists():
            regression_roots.append(repo_regressions_root)
        regression_files = collect_sorted_paths(
            [path for root in regression_roots for path in root.rglob("*") if path.is_file()]
        )
        if regression_files:
            regressions_index_path = manifests_dir / "regressions_index.json"
            write_json(
                regressions_index_path,
                {
                    "phase": args.phase,
                    "published_snapshot_mode": True,
                    "regression_roots": [str(path) for path in regression_roots],
                    "regression_files": regression_files,
                },
            )
            copied["manifests"].append(str(regressions_index_path))
    else:
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
        int(runtime_watch_registry_data.get("fingerprint_count", 0)),
    )
    published_runtime_history_source = (
        runtime_history_compact
        if args.use_published_snapshot and runtime_history_compact is not None and runtime_history_compact.exists()
        else runtime_history_index
    )

    metadata = {
        "phase": args.phase,
        "timestamp_utc": stable_bundle_timestamp(report_path),
        "staging_root": str(staging_root),
        "published_root": str(bundle_root) if args.use_published_snapshot else None,
        "publication_snapshot_mode": bool(args.use_published_snapshot),
        "publication_snapshot_id": publication_snapshot_id(report_path, args.phase) if args.use_published_snapshot else None,
        "artifact_root": str(artifact_root),
        "report": str(report_path),
        "source_report": str(report_path),
        "policy_manifest_json": str(manifest_json),
        "policy_manifest_txt": str(manifest_txt),
        "policy_manifest_summary": str(manifest_summary),
        "bundle_root": str(bundle_root),
        "zip_out": str(zip_out),
        "bundle_zip": str(zip_out),
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
        "pipeline_nightly_summary": str(pipeline_summary) if pipeline_summary is not None else None,
        "pipeline_quick_summary": str(pipeline_quick_summary) if pipeline_quick_summary is not None else None,
        "pipeline_matrix_summary": str(pipeline_matrix_summary) if pipeline_matrix_summary is not None else None,
        "runtime_manifest": str(runtime_manifest) if runtime_manifest is not None else None,
        "runtime_baseline_manifest": str(runtime_baseline_manifest) if runtime_baseline_manifest is not None else None,
        "runtime_current_manifest": str(runtime_current_manifest) if runtime_current_manifest is not None else None,
        "runtime_refresh_manifest": str(runtime_refresh_manifest) if runtime_refresh_manifest is not None else None,
        "runtime_rerun_plan": str(runtime_rerun_plan) if runtime_rerun_plan is not None else None,
        "runtime_registry": str(runtime_registry) if runtime_registry is not None else None,
        "runtime_registry_health": str(runtime_registry_health) if runtime_registry_health is not None else None,
        "runtime_history_index": str(runtime_history_index) if runtime_history_index is not None else None,
        "runtime_history_compact": str(runtime_history_compact) if runtime_history_compact is not None else None,
        "runtime_proposal": str(runtime_proposal) if runtime_proposal is not None else None,
        "runtime_proposal_gate": str(runtime_proposal_gate) if runtime_proposal_gate is not None else None,
        "runtime_import_summary": str(runtime_import_summary) if runtime_import_summary is not None else None,
        "runtime_new_env_proposal": str(runtime_new_env_proposal) if runtime_new_env_proposal is not None else None,
        "runtime_new_env_proposal_gate": str(runtime_new_env_proposal_gate) if runtime_new_env_proposal_gate is not None else None,
        "runtime_new_env_archived_proposal": str(runtime_new_env_archived_proposal) if runtime_new_env_archived_proposal is not None else None,
        "runtime_new_env_approved_baseline": str(runtime_new_env_approved_baseline) if runtime_new_env_approved_baseline is not None else None,
        "known_env_import_summary": str(known_env_import_summary) if known_env_import_summary is not None else None,
        "known_env_governance_policy": str(known_env_governance_policy) if known_env_governance_policy is not None else None,
        "known_env_age_tick": str(known_env_age_tick) if known_env_age_tick is not None else None,
        "known_env_reverify_plan": str(known_env_reverify_plan) if known_env_reverify_plan is not None else None,
        "known_env_reverify_gate": str(known_env_reverify_gate) if known_env_reverify_gate is not None else None,
        "known_env_reverify_apply": str(known_env_reverify_apply) if known_env_reverify_apply is not None else None,
        "known_env_retire_plan": str(known_env_retire_plan) if known_env_retire_plan is not None else None,
        "known_env_retire_apply": str(known_env_retire_apply) if known_env_retire_apply is not None else None,
        "known_env_retire_metadata": str(known_env_retire_metadata) if known_env_retire_metadata is not None else None,
        "current_env_governance_policy": str(current_env_governance_policy) if current_env_governance_policy is not None else None,
        "current_env_guardrail_policy": str(current_env_guardrail_policy) if current_env_guardrail_policy is not None else None,
        "current_env_watch_current": str(current_env_watch_current) if current_env_watch_current is not None else None,
        "current_env_watch_refresh": str(current_env_watch_refresh) if current_env_watch_refresh is not None else None,
        "current_env_watch_history": str(current_env_watch_history) if current_env_watch_history is not None else None,
        "current_env_age_tick": str(current_env_age_tick) if current_env_age_tick is not None else None,
        "current_env_watch_plan": str(current_env_watch_plan) if current_env_watch_plan is not None else None,
        "current_env_trigger_gate": str(current_env_trigger_gate) if current_env_trigger_gate is not None else None,
        "current_env_due": str(current_env_due) if current_env_due is not None else None,
        "current_env_reproposal_plan": str(current_env_reproposal_plan) if current_env_reproposal_plan is not None else None,
        "ops_agenda": str(ops_agenda) if ops_agenda is not None else None,
        "current_env_watch_execute": str(current_env_watch_execute) if current_env_watch_execute is not None else None,
        "current_env_watch_apply": str(current_env_watch_apply) if current_env_watch_apply is not None else None,
        "current_env_reproposal_execute": str(current_env_reproposal_execute) if current_env_reproposal_execute is not None else None,
        "current_env_action_ledger": str(current_env_action_ledger) if current_env_action_ledger is not None else None,
        "current_env_retry_plan": str(current_env_retry_plan) if current_env_retry_plan is not None else None,
        "current_env_reproposal_handoff": str(current_env_reproposal_handoff) if current_env_reproposal_handoff is not None else None,
        "current_env_operator_decision": str(current_env_operator_decision) if current_env_operator_decision is not None else None,
        "current_env_operator_decision_apply": str(current_env_operator_decision_apply) if current_env_operator_decision_apply is not None else None,
        "current_env_action_ledger_compact": str(current_env_action_ledger_compact) if current_env_action_ledger_compact is not None else None,
        "current_env_action_ledger_archive": str(current_env_action_ledger_archive) if current_env_action_ledger_archive is not None else None,
        "current_env_approval_runbook": str(current_env_approval_runbook) if current_env_approval_runbook is not None else None,
        "current_env_approval_execution": str(current_env_approval_execution) if current_env_approval_execution is not None else None,
        "current_env_approval_link": str(current_env_approval_link) if current_env_approval_link is not None else None,
        "operator_runbook_index": str(operator_runbook_index) if operator_runbook_index is not None else None,
        "operator_runbook_catalog": str(operator_runbook_catalog) if operator_runbook_catalog is not None else None,
        "operator_decision_metadata_audit": str(operator_decision_metadata_audit) if operator_decision_metadata_audit is not None else None,
        "operator_runbook_replay": str(operator_runbook_replay) if operator_runbook_replay is not None else None,
        "operator_runbook_retention_policy": str(operator_runbook_retention_policy) if operator_runbook_retention_policy is not None else None,
        "operator_runbook_pruned_catalog": str(operator_runbook_pruned_catalog) if operator_runbook_pruned_catalog is not None else None,
        "operator_runbook_archive": str(operator_runbook_archive) if operator_runbook_archive is not None else None,
        "operator_runbook_prune_summary": str(operator_runbook_prune_summary) if operator_runbook_prune_summary is not None else None,
        "operator_runbook_lifecycle_validation": str(operator_runbook_lifecycle_validation) if operator_runbook_lifecycle_validation is not None else None,
        "operator_runbook_pointer_audit": str(operator_runbook_pointer_audit) if operator_runbook_pointer_audit is not None else None,
        "operator_runbook_provenance_migration": str(operator_runbook_provenance_migration) if operator_runbook_provenance_migration is not None else None,
        "operator_runbook_migrated_catalog": str(operator_runbook_migrated_catalog) if operator_runbook_migrated_catalog is not None else None,
        "operator_runbook_migrated_ledger": str(operator_runbook_migrated_ledger) if operator_runbook_migrated_ledger is not None else None,
        "operator_runbook_lifecycle_validation_before": str(operator_runbook_lifecycle_validation_before) if operator_runbook_lifecycle_validation_before is not None else None,
        "operator_runbook_lifecycle_validation_after": str(operator_runbook_lifecycle_validation_after) if operator_runbook_lifecycle_validation_after is not None else None,
        "operator_artifact_path_policy_lint": str(operator_artifact_path_policy_lint) if operator_artifact_path_policy_lint is not None else None,
        "integrated_approval_mutation_audit": str(integrated_approval_mutation_audit) if integrated_approval_mutation_audit is not None else None,
        "staged_materialization_transaction": str(staged_materialization_transaction) if staged_materialization_transaction is not None else None,
        "source_health_preflight": str(source_health_preflight) if source_health_preflight is not None else None,
        "source_health_action_plan": str(source_health_action_plan) if source_health_action_plan is not None else None,
        "staged_materialization": str(staged_materialization) if staged_materialization is not None else None,
        "current_env_next_cycle_summaries": [str(path) for path in current_env_next_cycle_summaries],
        "runtime_registry_summary": str(runtime_registry_summary) if runtime_registry_summary is not None else None,
        "runtime_budget_current": str(runtime_budget_current) if runtime_budget_current is not None else None,
        "runtime_budget_baseline": str(runtime_budget_baseline) if runtime_budget_baseline is not None else None,
        "runtime_budget_refresh": str(runtime_budget_refresh) if runtime_budget_refresh is not None else None,
        "runtime_budget_rerun": str(runtime_budget_rerun) if runtime_budget_rerun is not None else None,
        "runtime_budget_proposal": str(runtime_budget_proposal) if runtime_budget_proposal is not None else None,
        "runtime_budget_proposal_gate": str(runtime_budget_proposal_gate) if runtime_budget_proposal_gate is not None else None,
        "runtime_budget_registry": str(runtime_budget_registry) if runtime_budget_registry is not None else None,
        "runtime_budget_reproposal_history": str(runtime_budget_reproposal_history) if runtime_budget_reproposal_history is not None else None,
        "runtime_budget_registry_summary": str(runtime_budget_registry_summary) if runtime_budget_registry_summary is not None else None,
        "runtime_watch_current": str(runtime_watch_current) if runtime_watch_current is not None else None,
        "runtime_watch_refresh": str(runtime_watch_refresh) if runtime_watch_refresh is not None else None,
        "runtime_watch_history_index": str(runtime_watch_history_index) if runtime_watch_history_index is not None else None,
        "runtime_watch_registry": str(runtime_watch_registry) if runtime_watch_registry is not None else None,
        "source_snapshot_manifest": str(source_snapshot_manifest) if source_snapshot_manifest is not None else None,
        "staged_mirror_manifest": str(staged_mirror_manifest) if staged_mirror_manifest is not None else None,
        "staged_mirror_verify": str(staged_mirror_verify) if staged_mirror_verify is not None else None,
        "ctest_inventory_release": str(ctest_inventory_release) if ctest_inventory_release is not None else None,
        "ctest_inventory_debug": str(ctest_inventory_debug) if ctest_inventory_debug is not None else None,
        "ctest_inventory_asan": str(ctest_inventory_asan) if ctest_inventory_asan is not None else None,
        "verification_release": str(verification_release) if verification_release is not None else None,
        "verification_debug": str(verification_debug) if verification_debug is not None else None,
        "verification_asan": str(verification_asan) if verification_asan is not None else None,
        "published_snapshot_manifest": str(published_snapshot_manifest) if published_snapshot_manifest is not None else None,
        "verification_closeout": str(verification_closeout) if verification_closeout is not None else None,
        "publication_health": str(publication_health) if publication_health is not None else None,
        "policy_ops_summary": str(ops_summary) if ops_summary is not None else None,
        "runtime_approval_metadata": str(runtime_approval_metadata_path) if runtime_approval_metadata_path is not None and runtime_approval_metadata_path.exists() else None,
        "runtime_budget_approval_metadata": str(runtime_budget_approval_metadata_path) if runtime_budget_approval_metadata_path is not None and runtime_budget_approval_metadata_path.exists() else None,
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
        "runtime_registry_health_hash": sha256_file(runtime_registry_health),
        "runtime_history_index_hash": sha256_file(runtime_history_index),
        "runtime_history_compact_hash": sha256_file(runtime_history_compact),
        "runtime_proposal_hash": sha256_file(runtime_proposal),
        "proposal_gate_hash": sha256_file(runtime_proposal_gate),
        "runtime_import_summary_hash": sha256_file(runtime_import_summary),
        "runtime_new_env_proposal_hash": sha256_file(runtime_new_env_proposal),
        "runtime_new_env_proposal_gate_hash": sha256_file(runtime_new_env_proposal_gate),
        "runtime_new_env_archived_proposal_hash": sha256_file(runtime_new_env_archived_proposal),
        "runtime_new_env_approved_baseline_hash": sha256_file(runtime_new_env_approved_baseline),
        "known_env_import_summary_hash": sha256_file(known_env_import_summary),
        "known_env_governance_policy_hash": sha256_file(known_env_governance_policy),
        "known_env_age_tick_hash": sha256_file(known_env_age_tick),
        "known_env_reverify_plan_hash": sha256_file(known_env_reverify_plan),
        "known_env_reverify_gate_hash": sha256_file(known_env_reverify_gate),
        "known_env_reverify_apply_hash": sha256_file(known_env_reverify_apply),
        "known_env_retire_plan_hash": sha256_file(known_env_retire_plan),
        "known_env_retire_apply_hash": sha256_file(known_env_retire_apply),
        "known_env_retire_metadata_hash": sha256_file(known_env_retire_metadata),
        "current_env_governance_policy_hash": sha256_file(current_env_governance_policy),
        "current_env_guardrail_policy_hash": sha256_file(current_env_guardrail_policy),
        "current_env_watch_current_hash": sha256_file(current_env_watch_current),
        "current_env_watch_refresh_hash": sha256_file(current_env_watch_refresh),
        "current_env_watch_history_hash": sha256_file(current_env_watch_history),
        "current_env_age_tick_hash": sha256_file(current_env_age_tick),
        "current_env_watch_plan_hash": sha256_file(current_env_watch_plan),
        "current_env_trigger_gate_hash": sha256_file(current_env_trigger_gate),
        "current_env_due_hash": sha256_file(current_env_due),
        "current_env_reproposal_plan_hash": sha256_file(current_env_reproposal_plan),
        "ops_agenda_hash": sha256_file(ops_agenda),
        "current_env_watch_execute_hash": sha256_file(current_env_watch_execute),
        "current_env_watch_apply_hash": sha256_file(current_env_watch_apply),
        "current_env_reproposal_execute_hash": sha256_file(current_env_reproposal_execute),
        "current_env_action_ledger_hash": sha256_file(current_env_action_ledger),
        "current_env_retry_plan_hash": sha256_file(current_env_retry_plan),
        "current_env_reproposal_handoff_hash": sha256_file(current_env_reproposal_handoff),
        "current_env_operator_decision_hash": sha256_file(current_env_operator_decision),
        "current_env_operator_decision_apply_hash": sha256_file(current_env_operator_decision_apply),
        "current_env_action_ledger_compact_hash": sha256_file(current_env_action_ledger_compact),
        "current_env_action_ledger_archive_hash": sha256_file(current_env_action_ledger_archive),
        "current_env_approval_runbook_hash": sha256_file(current_env_approval_runbook),
        "current_env_approval_execution_hash": sha256_file(current_env_approval_execution),
        "current_env_approval_link_hash": sha256_file(current_env_approval_link),
        "operator_runbook_index_hash": sha256_file(operator_runbook_index),
        "operator_runbook_catalog_hash": sha256_file(operator_runbook_catalog),
        "operator_decision_metadata_audit_hash": sha256_file(operator_decision_metadata_audit),
        "operator_runbook_replay_hash": sha256_file(operator_runbook_replay),
        "operator_runbook_retention_policy_hash": sha256_file(operator_runbook_retention_policy),
        "operator_runbook_pruned_catalog_hash": sha256_file(operator_runbook_pruned_catalog),
        "operator_runbook_archive_hash": sha256_file(operator_runbook_archive),
        "operator_runbook_prune_summary_hash": sha256_file(operator_runbook_prune_summary),
        "operator_runbook_lifecycle_validation_hash": sha256_file(operator_runbook_lifecycle_validation),
        "operator_runbook_pointer_audit_hash": sha256_file(operator_runbook_pointer_audit),
        "operator_runbook_provenance_migration_hash": sha256_file(operator_runbook_provenance_migration),
        "operator_runbook_migrated_catalog_hash": sha256_file(operator_runbook_migrated_catalog),
        "operator_runbook_migrated_ledger_hash": sha256_file(operator_runbook_migrated_ledger),
        "operator_runbook_lifecycle_validation_before_hash": sha256_file(operator_runbook_lifecycle_validation_before),
        "operator_runbook_lifecycle_validation_after_hash": sha256_file(operator_runbook_lifecycle_validation_after),
        "operator_artifact_path_policy_lint_hash": sha256_file(operator_artifact_path_policy_lint),
        "integrated_approval_mutation_audit_hash": sha256_file(integrated_approval_mutation_audit),
        "staged_materialization_transaction_hash": sha256_file(staged_materialization_transaction),
        "source_health_preflight_hash": sha256_file(source_health_preflight),
        "source_health_action_plan_hash": sha256_file(source_health_action_plan),
        "staged_materialization_hash": sha256_file(staged_materialization),
        "current_env_next_cycle_summary_hashes": [sha256_file(path) for path in current_env_next_cycle_summaries],
        "runtime_registry_summary_hash": sha256_file(runtime_registry_summary),
        "runtime_budget_current_hash": sha256_file(runtime_budget_current),
        "runtime_budget_baseline_hash": sha256_file(runtime_budget_baseline),
        "runtime_budget_refresh_hash": sha256_file(runtime_budget_refresh),
        "runtime_budget_rerun_hash": sha256_file(runtime_budget_rerun),
        "runtime_budget_proposal_hash": sha256_file(runtime_budget_proposal),
        "runtime_budget_proposal_gate_hash": sha256_file(runtime_budget_proposal_gate),
        "runtime_budget_registry_hash": sha256_file(runtime_budget_registry),
        "runtime_budget_reproposal_history_hash": sha256_file(runtime_budget_reproposal_history),
        "runtime_budget_registry_summary_hash": sha256_file(runtime_budget_registry_summary),
        "runtime_watch_current_hash": sha256_file(runtime_watch_current),
        "runtime_watch_refresh_hash": sha256_file(runtime_watch_refresh),
        "runtime_watch_history_index_hash": sha256_file(runtime_watch_history_index),
        "runtime_watch_registry_hash": sha256_file(runtime_watch_registry),
        "source_snapshot_manifest_hash": sha256_file(source_snapshot_manifest),
        "staged_mirror_manifest_hash": sha256_file(staged_mirror_manifest),
        "staged_mirror_verify_hash": sha256_file(staged_mirror_verify),
        "ctest_inventory_release_hash": sha256_file(ctest_inventory_release),
        "ctest_inventory_debug_hash": sha256_file(ctest_inventory_debug),
        "ctest_inventory_asan_hash": sha256_file(ctest_inventory_asan),
        "verification_release_hash": sha256_file(verification_release),
        "verification_debug_hash": sha256_file(verification_debug),
        "verification_asan_hash": sha256_file(verification_asan),
        "published_snapshot_manifest_hash": sha256_file(published_snapshot_manifest),
        "verification_closeout_hash": sha256_file(verification_closeout),
        "publication_health_hash": sha256_file(publication_health),
        "policy_ops_summary_hash": sha256_file(ops_summary),
        "verification_lane_status": verification_closeout_data.get("closeout_verdict")
        or verification_closeout_data.get("verification_closeout_status")
        or "NOT_RUN",
        "verification_source_snapshot_hash": source_snapshot_manifest_data.get("snapshot_hash"),
        "staged_mirror_hash": staged_mirror_verify_data.get("staged_mirror_hash")
        or staged_mirror_manifest_data.get("staged_mirror_hash"),
        "verification_not_run_count": int(verification_release_data.get("not_run_count", 0))
        + int(verification_debug_data.get("not_run_count", 0))
        + int(verification_asan_data.get("not_run_count", 0)),
        "published_snapshot_id": published_snapshot_manifest_data.get("publication_snapshot_id")
        or published_snapshot_manifest_data.get("phase_tag")
        or (publication_snapshot_id(report_path, args.phase) if args.use_published_snapshot else None),
        "verification_closeout_status": verification_closeout_data.get("verification_closeout_status")
        or verification_closeout_data.get("closeout_verdict")
        or "NOT_RUN",
        "current_env_watch_confidence": ops_summary_data.get("current_env_summary", {}).get("watch_confidence"),
        "new_env_watch_confidence": ops_summary_data.get("new_env_summary", {}).get("watch_confidence"),
        "approved_runtime_baseline_hash": sha256_file(runtime_baseline_manifest),
        "proposal_archive_hash": sha256_file(runtime_proposal),
        "proposal_gate_verdict": runtime_proposal_gate_data.get("proposal_gate_verdict"),
        "proposal_confidence": runtime_proposal_gate_data.get("proposal_confidence"),
        "freshness_summary": freshness_summary,
        "refresh_summary": refresh_rollup,
        "pipeline_summary_data": pipeline_summary_data,
        "pipeline_quick_summary_data": pipeline_quick_summary_data,
        "runtime_summary": runtime_manifest_data,
        "runtime_baseline_summary": runtime_baseline_data,
        "runtime_refresh_summary": runtime_refresh_data,
        "runtime_rerun_plan_summary": runtime_rerun_plan_data,
        "runtime_registry_summary": runtime_registry_data,
        "runtime_registry_health_summary": runtime_registry_health_data,
        "runtime_history_summary": runtime_history_data,
        "runtime_history_compact_summary": runtime_history_compact_data,
        "runtime_proposal_summary": runtime_proposal_data,
        "runtime_proposal_gate_summary": runtime_proposal_gate_data,
        "runtime_import_summary_data": runtime_import_summary_data,
        "runtime_new_env_proposal_summary": runtime_new_env_proposal_data,
        "runtime_new_env_proposal_gate_summary": runtime_new_env_proposal_gate_data,
        "runtime_new_env_archived_proposal_summary": runtime_new_env_archived_proposal_data,
        "runtime_new_env_approved_baseline_summary": runtime_new_env_approved_baseline_data,
        "known_env_import_summary_data": known_env_import_summary_data,
        "known_env_governance_policy_data": known_env_governance_policy_data,
        "known_env_age_tick_summary": known_env_age_tick_data,
        "known_env_reverify_plan_summary": known_env_reverify_plan_data,
        "known_env_reverify_gate_summary": known_env_reverify_gate_data,
        "known_env_reverify_apply_summary": known_env_reverify_apply_data,
        "known_env_retire_plan_summary": known_env_retire_plan_data,
        "known_env_retire_apply_summary": known_env_retire_apply_data,
        "known_env_retire_metadata_summary": known_env_retire_metadata_data,
        "current_env_governance_policy_data": current_env_governance_policy_data,
        "current_env_guardrail_policy_data": current_env_guardrail_policy_data,
        "current_env_watch_current_summary": current_env_watch_current_data,
        "current_env_watch_refresh_summary": current_env_watch_refresh_data,
        "current_env_watch_history_summary": current_env_watch_history_data,
        "current_env_age_tick_summary": current_env_age_tick_data,
        "current_env_watch_plan_summary": current_env_watch_plan_data,
        "current_env_trigger_gate_summary": current_env_trigger_gate_data,
        "current_env_due_summary": current_env_due_data,
        "current_env_reproposal_plan_summary": current_env_reproposal_plan_data,
        "ops_agenda_summary": ops_agenda_data,
        "current_env_watch_execute_summary": current_env_watch_execute_data,
        "current_env_watch_apply_summary": current_env_watch_apply_data,
        "current_env_reproposal_execute_summary": current_env_reproposal_execute_data,
        "current_env_action_ledger_summary": current_env_action_ledger_data,
        "current_env_retry_plan_summary": current_env_retry_plan_data,
        "current_env_reproposal_handoff_summary": current_env_reproposal_handoff_data,
        "current_env_operator_decision_summary": current_env_operator_decision_data,
        "current_env_operator_decision_apply_summary": current_env_operator_decision_apply_data,
        "current_env_action_ledger_compact_summary": current_env_action_ledger_compact_data,
        "current_env_action_ledger_archive_summary": current_env_action_ledger_archive_data,
        "current_env_approval_runbook_summary": current_env_approval_runbook_data,
        "current_env_approval_execution_summary": current_env_approval_execution_data,
        "current_env_approval_link_summary": current_env_approval_link_data,
        "operator_runbook_index_summary": operator_runbook_index_data,
        "operator_runbook_catalog_summary": operator_runbook_catalog_data,
        "operator_decision_metadata_audit_summary": operator_decision_metadata_audit_data,
        "operator_runbook_replay_summary": operator_runbook_replay_data,
        "operator_runbook_retention_policy_summary": operator_runbook_retention_policy_data,
        "operator_runbook_pruned_catalog_summary": operator_runbook_pruned_catalog_data,
        "operator_runbook_archive_summary": operator_runbook_archive_data,
        "operator_runbook_prune_summary": operator_runbook_prune_summary_data,
        "operator_runbook_lifecycle_validation_summary": operator_runbook_lifecycle_validation_data,
        "operator_runbook_pointer_audit_summary": operator_runbook_pointer_audit_data,
        "operator_runbook_provenance_migration_summary": operator_runbook_provenance_migration_data,
        "operator_runbook_migrated_catalog_summary": operator_runbook_migrated_catalog_data,
        "operator_runbook_migrated_ledger_summary": operator_runbook_migrated_ledger_data,
        "operator_runbook_lifecycle_validation_before_summary": operator_runbook_lifecycle_validation_before_data,
        "operator_runbook_lifecycle_validation_after_summary": operator_runbook_lifecycle_validation_after_data,
        "operator_artifact_path_policy_lint_summary": operator_artifact_path_policy_lint_data,
        "integrated_approval_mutation_audit_summary": integrated_approval_mutation_audit_data,
        "source_health_preflight_summary": source_health_preflight_data,
        "source_health_action_plan_summary": source_health_action_plan_data,
        "staged_materialization_summary": staged_materialization_data,
        "staged_materialization_transaction_summary": staged_materialization_transaction_data,
        "current_env_next_cycle_summaries": current_env_next_cycle_summary_data,
        "runtime_registry_compact_summary": runtime_registry_summary_data,
        "runtime_budget_current_summary": runtime_budget_current_data,
        "runtime_budget_baseline_summary": runtime_budget_baseline_data,
        "runtime_budget_refresh_summary": runtime_budget_refresh_data,
        "runtime_budget_rerun_summary": runtime_budget_rerun_data,
        "runtime_budget_proposal_summary": runtime_budget_proposal_data,
        "runtime_budget_proposal_gate_summary": runtime_budget_proposal_gate_data,
        "runtime_budget_registry_summary": runtime_budget_registry_data,
        "runtime_budget_reproposal_history_summary": runtime_budget_reproposal_history_data,
        "runtime_budget_registry_phase_summary": runtime_budget_registry_summary_data,
        "runtime_watch_current_summary": runtime_watch_current_data,
        "runtime_watch_refresh_summary": runtime_watch_refresh_data,
        "runtime_watch_history_summary": runtime_watch_history_data,
        "runtime_watch_registry_summary": runtime_watch_registry_data,
        "source_snapshot_manifest_summary": source_snapshot_manifest_data,
        "staged_mirror_manifest_summary": staged_mirror_manifest_data,
        "staged_mirror_verify_summary": staged_mirror_verify_data,
        "ctest_inventory_release_summary": ctest_inventory_release_data,
        "ctest_inventory_debug_summary": ctest_inventory_debug_data,
        "ctest_inventory_asan_summary": ctest_inventory_asan_data,
        "verification_release_summary": verification_release_data,
        "verification_debug_summary": verification_debug_data,
        "verification_asan_summary": verification_asan_data,
        "published_snapshot_manifest_summary": published_snapshot_manifest_data,
        "verification_closeout_summary": verification_closeout_data,
        "publication_health_summary": publication_health_data,
        "policy_ops_summary_data": ops_summary_data,
        "approved_known_summary_count": len(approved_known_summaries),
        "foreign_import_summary_count": len(foreign_import_summaries),
        "runtime_approval_metadata_summary": runtime_approval_metadata,
        "runtime_budget_approval_metadata_summary": runtime_budget_approval_metadata,
        "pipeline_matrix_summary_data": pipeline_matrix_summary_data,
        "runtime_budget_summary": {
            "current_verdict": runtime_budget_refresh_data.get("current_verdict")
            or pipeline_summary_data.get("runtime_current_verdict")
            or runtime_refresh_data.get("current_verdict")
            or runtime_manifest_data.get("current_verdict"),
            "freshness_verdict": runtime_budget_refresh_data.get("freshness_verdict")
            or pipeline_summary_data.get("runtime_freshness_verdict")
            or runtime_refresh_data.get("freshness_verdict"),
            "comparability_verdict": runtime_budget_refresh_data.get("comparability_verdict")
            or pipeline_summary_data.get("runtime_comparability_verdict")
            or runtime_refresh_data.get("comparability_verdict"),
            "budget_verdict": runtime_budget_refresh_data.get("budget_verdict")
            or pipeline_summary_data.get("runtime_budget_verdict")
            or runtime_refresh_data.get("overall_budget_verdict")
            or runtime_manifest_data.get("overall_budget_verdict"),
            "runtime_severity": pipeline_summary_data.get("runtime_severity"),
            "warn_count": runtime_budget_refresh_data.get("warn_count", runtime_refresh_data.get("warn_count", runtime_manifest_data.get("warn_count"))),
            "fail_count": runtime_budget_refresh_data.get("fail_count", runtime_refresh_data.get("fail_count", runtime_manifest_data.get("fail_count"))),
            "stale_entry_count": runtime_refresh_data.get("stale_entry_count"),
            "requires_rerun_entry_count": runtime_refresh_data.get("requires_rerun_entry_count"),
            "rebaseline_required_count": runtime_refresh_data.get("rebaseline_required_count"),
            "not_comparable_count": runtime_refresh_data.get("not_comparable_count"),
            "selected_baseline_id": runtime_budget_refresh_data.get("selected_budget_profile_id") or runtime_refresh_data.get("selected_baseline_id"),
            "selected_baseline_tag": runtime_budget_refresh_data.get("selected_budget_profile_tag") or runtime_refresh_data.get("selected_baseline_tag"),
        },
        "current_env_watch_lifecycle": {
            "state": current_env_watch_refresh_data.get("current_env_state")
            or current_env_watch_current_data.get("current_env_state"),
            "watch_status": current_env_watch_refresh_data.get("watch_status")
            or current_env_watch_current_data.get("watch_status"),
            "watch_confidence": current_env_watch_refresh_data.get("watch_confidence")
            or current_env_watch_current_data.get("watch_confidence"),
            "reproposal_needed": current_env_watch_refresh_data.get("reproposal_needed")
            or current_env_watch_current_data.get("reproposal_needed"),
            "reproposal_gate_verdict": current_env_watch_refresh_data.get("reproposal_gate_verdict"),
            "history_transition_count": current_env_watch_history_data.get("transition_count", 0),
            "selected_budget_profile_id": current_env_watch_refresh_data.get("selected_budget_profile_id")
            or current_env_watch_current_data.get("selected_budget_profile_id"),
            "selected_runtime_baseline_id": current_env_watch_refresh_data.get("selected_runtime_baseline_id")
            or current_env_watch_current_data.get("selected_runtime_baseline_id"),
        },
        "current_env_guardrail_lifecycle": {
            "policy_id": current_env_guardrail_policy_data.get("policy_id")
            or current_env_governance_policy_data.get("policy_id"),
            "state": current_env_age_tick_data.get("current_env_state_after")
            or current_env_trigger_gate_data.get("current_env_state"),
            "approval_grace_active": bool(current_env_age_tick_data.get("approval_grace_active", False)),
            "next_monitoring_due_at": current_env_age_tick_data.get("next_due_at")
            or current_env_watch_history_data.get("next_monitoring_due_at"),
            "next_reproposal_due_at": current_env_age_tick_data.get("next_reproposal_due_at")
            or current_env_trigger_gate_data.get("next_reproposal_due_at"),
            "trigger_gate_verdict": current_env_trigger_gate_data.get("trigger_gate_verdict"),
            "watch_plan_verdict": current_env_watch_plan_data.get("plan_verdict"),
            "monitoring_due_state": current_env_due_data.get("monitoring_due_state"),
            "reproposal_due_state": current_env_due_data.get("reproposal_due_state"),
            "next_due_kind": current_env_due_data.get("next_due_kind"),
            "next_due_at": current_env_due_data.get("next_due_at"),
            "due_scheduler_action": current_env_due_data.get("recommended_action_current_env"),
            "reproposal_plan_verdict": current_env_reproposal_plan_data.get("plan_verdict"),
            "ops_agenda_highest_priority_action": ops_agenda_data.get("highest_priority_action"),
            "watch_execute_status": current_env_watch_execute_data.get("action_status"),
            "watch_execute_verdict": current_env_watch_execute_data.get("execution_verdict"),
            "watch_apply_status": current_env_watch_apply_data.get("action_status"),
            "watch_apply_new_state": current_env_watch_apply_data.get("new_state"),
            "watch_apply_next_monitoring_due_at": current_env_watch_apply_data.get("updated_next_monitoring_due_at"),
            "watch_apply_next_reproposal_due_at": current_env_watch_apply_data.get("updated_next_reproposal_due_at"),
            "reproposal_execute_gate_verdict": current_env_reproposal_execute_data.get("gate_verdict"),
            "action_ledger_total_action_count": current_env_action_ledger_data.get("total_action_count", 0),
            "action_ledger_latest_applied_action_id": current_env_action_ledger_data.get("latest_applied_action_id"),
            "retry_plan_verdict": current_env_retry_plan_data.get("plan_verdict"),
            "retryable_action_count": current_env_retry_plan_data.get("retryable_count", 0),
            "retry_escalation_count": current_env_retry_plan_data.get("escalation_count", 0),
            "reproposal_handoff_status": current_env_reproposal_handoff_data.get("handoff_status"),
            "reproposal_handoff_approval_ready": current_env_reproposal_handoff_data.get("approval_ready", False),
            "reproposal_handoff_next_action": current_env_reproposal_handoff_data.get("next_action_kind"),
            "operator_decision_id": current_env_operator_decision_data.get("decision_id"),
            "operator_decision": current_env_operator_decision_data.get("decision"),
            "operator_decision_apply_closure_status": current_env_operator_decision_apply_data.get("closure_status"),
            "approval_runbook_ready": current_env_approval_runbook_data.get("approval_ready"),
            "approval_runbook_mode": current_env_approval_runbook_data.get("approval_mode"),
            "approval_execution_status": current_env_approval_execution_data.get("approval_status"),
            "approval_execution_applied": current_env_approval_execution_data.get("approval_status") == "APPLIED",
            "approval_registry_updated": current_env_approval_execution_data.get("registry_updated", False),
            "approval_link_status": current_env_approval_link_data.get("approval_status"),
            "approval_link_ledger_updated": current_env_approval_link_data.get("ledger_updated", False),
            "operator_runbook_count": operator_runbook_index_data.get("runbook_count", 0),
            "operator_runbook_executable_count": operator_runbook_index_data.get("executable_runbook_count", 0),
            "operator_runbook_catalog_active_count": operator_runbook_catalog_data.get("active_runbook_count", 0),
            "operator_runbook_catalog_resolved_count": operator_runbook_catalog_data.get("resolved_runbook_count", 0),
            "operator_decision_metadata_audit_verdict": operator_decision_metadata_audit_data.get("audit_verdict"),
            "operator_runbook_replay_verdict": operator_runbook_replay_data.get("replay_verdict"),
            "operator_runbook_prune_verdict": operator_runbook_prune_summary_data.get("prune_verdict"),
            "operator_runbook_lifecycle_validation_verdict": operator_runbook_lifecycle_validation_data.get("validation_verdict"),
            "integrated_approval_mutation_audit_verdict": integrated_approval_mutation_audit_data.get("audit_verdict"),
            "staged_materialization_transaction_verdict": staged_materialization_transaction_data.get("transaction_verdict"),
            "source_health_status": source_health_preflight_data.get("status"),
            "source_health_recommendation": source_health_preflight_data.get("recommendation"),
            "source_health_plan_recommended_action": source_health_action_plan_data.get("recommended_action"),
            "source_health_plan_direct_build_blocked": source_health_action_plan_data.get("direct_build_blocked"),
            "source_health_plan_staged_build_allowed": source_health_action_plan_data.get("staged_build_allowed"),
            "staged_materialization_mode": staged_materialization_data.get("staged_materialization_mode"),
            "staged_materialization_verdict": staged_materialization_data.get("materialization_verdict"),
            "ledger_compact_archived_action_count": current_env_action_ledger_compact_data.get("archived_action_count", 0),
            "next_cycle_summary_count": len(current_env_next_cycle_summary_data),
            "action_timeline_status": {
                "planned_action_count": current_env_action_ledger_data.get("planned_count", ops_agenda_data.get("planned_action_count")),
                "executed_action_count": current_env_action_ledger_data.get("executed_count", ops_agenda_data.get("executed_action_count")),
                "applied_action_count": current_env_action_ledger_data.get("applied_count", ops_agenda_data.get("applied_action_count")),
                "failed_action_count": current_env_action_ledger_data.get("failed_count", ops_agenda_data.get("failed_action_count")),
                "skipped_action_count": current_env_action_ledger_data.get("skipped_count", 0),
                "superseded_action_count": current_env_action_ledger_data.get("superseded_count", 0),
                "closed_action_count": current_env_action_ledger_data.get("closed_count", 0),
                "deferred_action_count": current_env_action_ledger_data.get("deferred_count", 0),
                "rejected_action_count": current_env_action_ledger_data.get("rejected_count", 0),
                "retryable_action_count": current_env_retry_plan_data.get("retryable_count", 0),
                "retry_escalation_count": current_env_retry_plan_data.get("escalation_count", 0),
                "handoff_ready_count": 1 if bool(current_env_reproposal_handoff_data.get("approval_ready", False)) else 0,
                "operator_decision_count": 1 if current_env_operator_decision_data else 0,
                "closure_status": current_env_operator_decision_apply_data.get("closure_status"),
                "archived_action_count": current_env_action_ledger_archive_data.get("archived_action_count", 0),
            },
            "next_watch_execution_class": None
            if not current_env_watch_plan_data.get("entries")
            else current_env_watch_plan_data.get("entries", [{}])[0].get("execution_class"),
            "recommended_action_current_env": ops_summary_data.get("current_env_guardrail", {}).get("recommended_action_current_env"),
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
        "runtime_watch_registry_evidence_source_counts": runtime_watch_registry_data.get("evidence_source_counts", {}),
        "runtime_budget_profile_id": runtime_watch_refresh_data.get("runtime_budget_profile_id")
        or runtime_watch_current_data.get("runtime_budget_profile_id"),
        "runtime_budget_profile_registry_id": runtime_budget_baseline_data.get("profile_id")
        or runtime_budget_refresh_data.get("selected_budget_profile_id")
        or runtime_budget_current_data.get("source_runtime_budget_profile_id"),
        "diagnostic_watch_only": bool(runtime_watch_refresh_data.get("diagnostic_watch_only", False)),
        "current_env_operator_action": ops_summary_data.get("final_operator_summary", {}).get("recommended_action_current_env"),
        "current_env_due_operator_action": ops_summary_data.get("final_operator_summary", {}).get("recommended_action_current_env_due"),
        "ops_agenda_operator_action": ops_summary_data.get("final_operator_summary", {}).get("recommended_action_ops_agenda"),
        "known_env_operator_action": ops_summary_data.get("final_operator_summary", {}).get("recommended_action_known_envs"),
        "new_env_operator_action": ops_summary_data.get("final_operator_summary", {}).get("recommended_action_new_env"),
        "publication_operator_action": ops_summary_data.get("final_operator_actions", {}).get("recommended_action_publication"),
        "current_env_operator_conclusion": ops_summary_data.get("final_operator_summary", {}).get("current_env_operator_conclusion"),
        "current_env_due_operator_conclusion": ops_summary_data.get("final_operator_summary", {}).get("current_env_due_conclusion"),
        "ops_agenda_operator_conclusion": ops_summary_data.get("final_operator_summary", {}).get("ops_agenda_conclusion"),
        "known_env_operator_conclusion": ops_summary_data.get("final_operator_summary", {}).get("known_env_operator_conclusion"),
        "new_env_operator_conclusion": ops_summary_data.get("final_operator_summary", {}).get("new_env_operator_conclusion"),
        "publication_operator_conclusion": ops_summary_data.get("final_operator_actions", {}).get("publication_operator_conclusion"),
        "runtime_registry_health_status": runtime_registry_health_data.get("overall_status") or runtime_registry_health_data.get("status"),
        "publication_health_status": publication_health_data.get("status"),
        "publication_missing_artifact_count": publication_health_data.get("missing_artifact_count", 0),
        "publication_hash_mismatch_count": publication_health_data.get("hash_mismatch_count", 0),
        "publication_dangling_reference_count": publication_health_data.get("dangling_reference_count", 0),
        "active_runtime_baselines_list": str(active_runtime_baselines_manifest),
        "pending_runtime_proposals_list": str(pending_runtime_proposals_manifest),
        "active_runtime_baselines_count": len(active_runtime_baselines_list.get("entries", {})),
        "pending_runtime_proposals_count": len(pending_runtime_proposals_list.get("entries", [])),
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
        "post_approval_runtime_recommendation": pipeline_summary_data.get("runtime_recommendation"),
        "budget_transition_status": runtime_budget_approval_metadata.get("budget_transition_status"),
        "runtime_budget_reproposal_needed": runtime_budget_proposal_gate_data.get("budget_reproposal_needed")
        or runtime_budget_proposal_data.get("budget_reproposal_needed")
        or runtime_budget_current_data.get("budget_reproposal_needed"),
        "runtime_budget_reproposal_gate_verdict": runtime_budget_proposal_gate_data.get("reproposal_gate_verdict")
        or runtime_budget_proposal_gate_data.get("proposal_gate_verdict"),
        "runtime_budget_trigger_gate_verdict": current_env_trigger_gate_data.get("trigger_gate_verdict"),
        "post_approval_budget_recommendation": ops_summary_data.get("current_env_summary", {}).get("recommendation")
        or pipeline_summary_data.get("runtime_recommendation")
        or runtime_budget_proposal_gate_data.get("recommended_action"),
        "runtime_recommendation": pipeline_summary_data.get("runtime_recommendation"),
        "rebaseline_proposal_needed": pipeline_summary_data.get("runtime_rebaseline_proposal_needed"),
        "runtime_trend_summary": runtime_history_data.get("trend_counts", {}),
        "selected_runtime_baseline_id": pipeline_summary_data.get("runtime_selected_baseline_id"),
        "selected_runtime_baseline_tag": pipeline_summary_data.get("runtime_selected_baseline_tag"),
        "policy_ops_rationale": ops_summary_data.get("final_operator_summary", {}).get("rationale", []),
        "drift_flags": drift_flags,
        "reclassification_needed_families": reclassification_needed,
        "family_status_summary": family_status_summary,
        "family_status_table": family_status_summary,
        "family_freshness": family_freshness,
        "stale_family_count": stale_family_count,
        "requires_rerun_family_count": requires_rerun_family_count,
        "reclassify_required_count": reclassify_required_count,
        "source_manifests": [
            str(path)
            for path in (
                manifest_json,
                baseline_manifest,
                current_manifest,
                refresh_manifest,
                rerun_plan,
                pipeline_summary,
                pipeline_quick_summary,
                pipeline_matrix_summary,
                runtime_baseline_manifest,
                runtime_current_manifest,
                runtime_refresh_manifest,
                runtime_rerun_plan,
                runtime_registry,
                runtime_registry_health,
                published_runtime_history_source,
                runtime_history_compact,
                runtime_proposal,
                runtime_proposal_gate,
                runtime_import_summary,
                runtime_new_env_proposal,
                runtime_new_env_proposal_gate,
                runtime_new_env_archived_proposal,
                runtime_new_env_approved_baseline,
                runtime_registry_summary,
                runtime_budget_current,
                runtime_budget_baseline,
                runtime_budget_refresh,
                runtime_budget_rerun,
                runtime_budget_proposal,
                runtime_budget_proposal_gate,
                runtime_budget_registry,
                runtime_budget_reproposal_history,
                runtime_budget_registry_summary,
                current_env_governance_policy,
                current_env_guardrail_policy,
                current_env_watch_current,
                current_env_watch_refresh,
                current_env_watch_history,
                current_env_age_tick,
                current_env_watch_plan,
                current_env_trigger_gate,
                current_env_due,
                current_env_reproposal_plan,
                ops_agenda,
                current_env_watch_execute,
                current_env_watch_apply,
                current_env_reproposal_execute,
                current_env_action_ledger,
                current_env_retry_plan,
                current_env_reproposal_handoff,
                current_env_operator_decision,
                current_env_operator_decision_apply,
                current_env_action_ledger_compact,
                current_env_action_ledger_archive,
                current_env_approval_runbook,
                current_env_approval_execution,
                current_env_approval_link,
                operator_runbook_index,
                operator_runbook_catalog,
                operator_decision_metadata_audit,
                operator_runbook_replay,
                operator_runbook_retention_policy,
                operator_runbook_pruned_catalog,
                operator_runbook_archive,
                operator_runbook_prune_summary,
                operator_runbook_lifecycle_validation,
                integrated_approval_mutation_audit,
                runtime_watch_current,
                runtime_watch_refresh,
                runtime_watch_history_index,
                runtime_watch_registry,
                source_health_preflight,
                source_health_action_plan,
                staged_materialization,
                staged_materialization_transaction,
                source_snapshot_manifest,
                staged_mirror_manifest,
                staged_mirror_verify,
                ctest_inventory_release,
                ctest_inventory_debug,
                ctest_inventory_asan,
                verification_release,
                verification_debug,
                verification_asan,
                published_snapshot_manifest,
                verification_closeout,
                publication_health,
                ops_summary,
                *[resolve_manifest_json(Path(value).resolve()) for value in list(args.approved_known_summary or []) if value],
                *[resolve_manifest_json(Path(value).resolve()) for value in list(args.foreign_import_summary or []) if value],
            )
            if path is not None and path.exists()
        ],
        "source_manifest_hashes": {
            str(path): sha256_file(path)
            for path in (
                manifest_json,
                baseline_manifest,
                current_manifest,
                refresh_manifest,
                rerun_plan,
                pipeline_summary,
                pipeline_quick_summary,
                pipeline_matrix_summary,
                runtime_baseline_manifest,
                runtime_current_manifest,
                runtime_refresh_manifest,
                runtime_rerun_plan,
                runtime_registry,
                runtime_registry_health,
                published_runtime_history_source,
                runtime_history_compact,
                runtime_proposal,
                runtime_proposal_gate,
                runtime_budget_current,
                runtime_budget_baseline,
                runtime_budget_refresh,
                runtime_budget_rerun,
                runtime_budget_proposal,
                runtime_budget_proposal_gate,
                runtime_budget_registry,
                runtime_budget_reproposal_history,
                runtime_budget_registry_summary,
                current_env_governance_policy,
                current_env_guardrail_policy,
                current_env_watch_current,
                current_env_watch_refresh,
                current_env_watch_history,
                current_env_age_tick,
                current_env_watch_plan,
                current_env_trigger_gate,
                current_env_due,
                current_env_reproposal_plan,
                ops_agenda,
                current_env_watch_execute,
                current_env_watch_apply,
                current_env_reproposal_execute,
                current_env_action_ledger,
                current_env_retry_plan,
                current_env_reproposal_handoff,
                current_env_operator_decision,
                current_env_operator_decision_apply,
                current_env_action_ledger_compact,
                current_env_action_ledger_archive,
                current_env_approval_runbook,
                current_env_approval_execution,
                current_env_approval_link,
                operator_runbook_index,
                operator_runbook_catalog,
                operator_decision_metadata_audit,
                operator_runbook_replay,
                runtime_watch_current,
                runtime_watch_refresh,
                runtime_watch_history_index,
                runtime_watch_registry,
                source_health_preflight,
                staged_materialization,
                staged_materialization_transaction,
                source_snapshot_manifest,
                staged_mirror_manifest,
                staged_mirror_verify,
                ctest_inventory_release,
                ctest_inventory_debug,
                ctest_inventory_asan,
                verification_release,
                verification_debug,
                verification_asan,
                published_snapshot_manifest,
                verification_closeout,
                publication_health,
                ops_summary,
                *[resolve_manifest_json(Path(value).resolve()) for value in list(args.approved_known_summary or []) if value],
                *[resolve_manifest_json(Path(value).resolve()) for value in list(args.foreign_import_summary or []) if value],
            )
            if path is not None and path.exists()
        },
        "copied_counts": {key: len(value) for key, value in copied.items()},
    }
    atomic_write_text(bundle_root / "bundle_metadata.json", json.dumps(metadata, indent=2) + "\n")
    if args.use_published_snapshot:
        publication_metadata = {
            "publication_snapshot_id": metadata.get("publication_snapshot_id"),
            "publish_timestamp_utc": metadata.get("timestamp_utc"),
            "staging_root": str(staging_root),
            "published_root": str(bundle_root),
            "source_report": str(report_path),
            "source_manifest_hashes": metadata.get("source_manifest_hashes", {}),
            "verification_lane_status": metadata.get("verification_lane_status"),
            "verification_closeout_status": metadata.get("verification_closeout_status"),
            "verification_source_snapshot_hash": metadata.get("verification_source_snapshot_hash"),
            "staged_mirror_hash": metadata.get("staged_mirror_hash"),
            "verification_release_hash": metadata.get("verification_release_hash"),
            "verification_debug_hash": metadata.get("verification_debug_hash"),
            "verification_asan_hash": metadata.get("verification_asan_hash"),
            "copied_artifact_list": copied,
        }
        atomic_write_text(bundle_root / "publication_snapshot.json", json.dumps(publication_metadata, indent=2) + "\n")

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

    create_directory_zip(zip_out, bundle_root)

    if curated_zip is not None:
        create_directory_zip(curated_zip, curated_dir)

    if light_ops_zip is not None:
        create_directory_zip(light_ops_zip, light_ops_dir)
        metadata["light_ops_zip"] = str(light_ops_zip)
        metadata["light_ops_zip_hash"] = sha256_file(light_ops_zip)
        atomic_write_text(bundle_root / "bundle_metadata.json", json.dumps(metadata, indent=2) + "\n")

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
        runtime_registry_health,
        runtime_history_index,
        runtime_history_compact,
        runtime_proposal,
        runtime_proposal_gate,
        runtime_watch_current,
        runtime_watch_refresh,
        runtime_watch_history_index,
        runtime_watch_registry,
        known_env_import_summary,
        known_env_governance_policy,
        known_env_age_tick,
        known_env_reverify_plan,
        known_env_reverify_gate,
        known_env_reverify_apply,
        known_env_retire_plan,
        known_env_retire_apply,
        known_env_retire_metadata,
        source_snapshot_manifest,
        staged_mirror_manifest,
        staged_mirror_verify,
        ctest_inventory_release,
        ctest_inventory_debug,
        ctest_inventory_asan,
        verification_release,
        verification_debug,
        verification_asan,
        published_snapshot_manifest,
        verification_closeout,
        publication_health,
        ops_summary,
        current_env_operator_decision,
        current_env_operator_decision_apply,
        current_env_action_ledger_compact,
        current_env_action_ledger_archive,
        current_env_approval_runbook,
        current_env_approval_execution,
        current_env_approval_link,
        operator_runbook_index,
        operator_runbook_catalog,
        operator_decision_metadata_audit,
        operator_runbook_replay,
        operator_runbook_retention_policy,
        operator_runbook_pruned_catalog,
        operator_runbook_archive,
        operator_runbook_prune_summary,
        operator_runbook_lifecycle_validation,
        operator_runbook_pointer_audit,
        operator_runbook_provenance_migration,
        operator_runbook_migrated_catalog,
        operator_runbook_migrated_ledger,
        operator_runbook_lifecycle_validation_before,
        operator_runbook_lifecycle_validation_after,
        operator_artifact_path_policy_lint,
        integrated_approval_mutation_audit,
        source_health_preflight,
        source_health_action_plan,
        staged_materialization,
        staged_materialization_transaction,
        bundle_root / "bundle_metadata.json",
        zip_out,
        curated_zip,
        light_ops_zip,
    )
    metadata["delivery_bundle_items"] = [
        {"label": label, "path": str(path), "sha256": sha256_file(path)} for label, path in delivery_entries
    ]
    metadata["bundle_zip"] = str(zip_out)
    metadata["bundle_zip_hash"] = sha256_file(zip_out)
    metadata["curated_zip"] = None if curated_zip is None else str(curated_zip)
    metadata["curated_zip_hash"] = sha256_file(curated_zip)
    metadata["delivery_zip"] = str(delivery_zip)
    metadata["light_ops_zip"] = None if light_ops_zip is None else str(light_ops_zip)
    metadata["light_ops_zip_hash"] = sha256_file(light_ops_zip)
    metadata["published_snapshot_delivery_source"] = str(bundle_root) if args.use_published_snapshot else None
    atomic_write_text(bundle_root / "bundle_metadata.json", json.dumps(metadata, indent=2) + "\n")
    create_delivery_zip(delivery_zip, delivery_entries)
    metadata["delivery_zip_hash"] = sha256_file(delivery_zip)
    atomic_write_text(bundle_root / "delivery_zip.sha256", f"{metadata['delivery_zip_hash']}  {delivery_zip.name}\n")
    atomic_write_text(bundle_root / "bundle_metadata.json", json.dumps(metadata, indent=2) + "\n")

    print(str(zip_out))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
