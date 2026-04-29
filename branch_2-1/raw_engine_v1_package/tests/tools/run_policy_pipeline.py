#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import runtime_gate_lib as runtime_gate


EXIT_OK = 0
EXIT_WARN = 10
EXIT_ACTION_REQUIRED = 20
EXIT_FAIL = 30

SEVERITY_RANK = {"OK": 0, "WARN": 1, "ACTION_REQUIRED": 2, "FAIL": 3}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the exact-shadow policy lifecycle pipeline.")
    parser.add_argument(
        "--mode",
        required=True,
        choices=["quick", "nightly", "full_local", "bundle_only", "rebaseline_candidate", "matrix"],
    )
    parser.add_argument("--baseline-manifest", required=True)
    parser.add_argument("--current-manifest", required=True)
    parser.add_argument("--refresh-manifest", required=True)
    parser.add_argument("--rerun-plan", required=True)
    parser.add_argument("--runtime-baseline-manifest", default=None)
    parser.add_argument("--runtime-baseline-registry", default=None)
    parser.add_argument("--runtime-current-manifest", default=None)
    parser.add_argument("--runtime-refresh-manifest", default=None)
    parser.add_argument("--runtime-rerun-plan", default=None)
    parser.add_argument("--runtime-history-index", default=None)
    parser.add_argument("--runtime-proposal", default=None)
    parser.add_argument("--runtime-comparability-triage", default=None)
    parser.add_argument("--runtime-rebaseline-action-plan", default=None)
    parser.add_argument("--runtime-rebaseline-proposal", default=None)
    parser.add_argument("--runtime-rebaseline-proposal-gate", default=None)
    parser.add_argument("--runtime-rebaseline-runbook", default=None)
    parser.add_argument("--runtime-registry-selection-sanity", default=None)
    parser.add_argument("--runtime-registry-selection-repair-plan", default=None)
    parser.add_argument("--runtime-watch-current", default=None)
    parser.add_argument("--runtime-watch-refresh", default=None)
    parser.add_argument("--runtime-watch-history-index", default=None)
    parser.add_argument("--runtime-watch-registry", default=None)
    parser.add_argument("--runtime-registry-health", default=None)
    parser.add_argument("--publication-health", default=None)
    parser.add_argument("--source-snapshot-manifest", default=None)
    parser.add_argument("--include-source-health", action="store_true")
    parser.add_argument("--source-health-preflight", default=None)
    parser.add_argument("--source-health-action-plan", default=None)
    parser.add_argument("--verification-preflight-gate", default=None)
    parser.add_argument("--staged-overlay-completeness", default=None)
    parser.add_argument("--overlay-policy", default=None)
    parser.add_argument("--verification-preflight-gate-v2", default=None)
    parser.add_argument("--staged-materialization", default=None)
    parser.add_argument("--source-root", default=None)
    parser.add_argument("--source-health-simulate-dataless-count", type=int, default=0)
    parser.add_argument("--source-health-simulate-missing-git-object", action="store_true")
    parser.add_argument("--staged-mirror-manifest", default=None)
    parser.add_argument("--staged-mirror-verify", default=None)
    parser.add_argument("--ctest-inventory-release", default=None)
    parser.add_argument("--ctest-inventory-debug", default=None)
    parser.add_argument("--ctest-inventory-asan", default=None)
    parser.add_argument("--verification-release", default=None)
    parser.add_argument("--verification-debug", default=None)
    parser.add_argument("--verification-asan", default=None)
    parser.add_argument("--published-snapshot-manifest", default=None)
    parser.add_argument("--verification-closeout", default=None)
    parser.add_argument("--ops-summary", default=None)
    parser.add_argument("--approved-known-summary", action="append", default=[])
    parser.add_argument("--foreign-import-summary", action="append", default=[])
    parser.add_argument("--known-env-governance-policy", default=None)
    parser.add_argument("--known-env-age-tick", default=None)
    parser.add_argument("--known-env-reverify-plan", default=None)
    parser.add_argument("--known-env-reverify-gate", default=None)
    parser.add_argument("--known-env-reverify-apply", default=None)
    parser.add_argument("--known-env-retire-plan", default=None)
    parser.add_argument("--known-env-retire-apply", default=None)
    parser.add_argument("--known-env-retire-metadata", default=None)
    parser.add_argument("--current-env-governance-policy", default=None)
    parser.add_argument("--current-env-guardrail-policy", default=None)
    parser.add_argument("--current-env-watch-current", default=None)
    parser.add_argument("--current-env-watch-refresh", default=None)
    parser.add_argument("--current-env-watch-history", default=None)
    parser.add_argument("--current-env-age-tick", default=None)
    parser.add_argument("--current-env-watch-plan", default=None)
    parser.add_argument("--current-env-trigger-gate", default=None)
    parser.add_argument("--runtime-budget-current", default=None)
    parser.add_argument("--runtime-budget-proposal", default=None)
    parser.add_argument("--runtime-budget-proposal-gate", default=None)
    parser.add_argument("--runtime-budget-baseline", default=None)
    parser.add_argument("--runtime-budget-refresh", default=None)
    parser.add_argument("--runtime-budget-reproposal-history", default=None)
    parser.add_argument("--runtime-budget-registry-summary", default=None)
    parser.add_argument("--matrix-config", default=None)
    parser.add_argument("--pipeline-matrix-summary", default=None)
    parser.add_argument("--synthetic-runtime-fixture", default=None)
    parser.add_argument("--runtime-budget-config", default=None)
    parser.add_argument("--runtime-history-compact", default=None)
    parser.add_argument("--artifact-root", required=True)
    parser.add_argument("--report-out", default=None)
    parser.add_argument("--summary-out", default=None)
    parser.add_argument("--pipeline-phase", default="auto")
    parser.add_argument("--raw-engine-tests", default=None)
    parser.add_argument("--python-bin", default=None)
    parser.add_argument("--zip-out", default=None)
    parser.add_argument("--curated-zip", default=None)
    parser.add_argument("--light-ops-zip", default=None)
    parser.add_argument("--use-published-snapshot", action="store_true")
    parser.add_argument("--published-root", default=None)
    parser.add_argument("--allow-empty-plan", action="store_true")
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--skip-policy-stage", action="store_true")
    parser.add_argument("--skip-runtime-stage", action="store_true")
    parser.add_argument("--skip-bundle-build", action="store_true")
    parser.add_argument("--prune-artifacts", action="store_true")
    parser.add_argument("--max-bundles", type=int, default=4)
    parser.add_argument("--max-nightly-runs", type=int, default=4)
    parser.add_argument("--keep-approved", action="store_true")
    parser.add_argument("--runtime-stage", action="append", default=[], help="Additional runtime stage name=seconds.")
    parser.add_argument("--synthetic-hash-drift", default=None)
    parser.add_argument("--synthetic-applicability-drift", default=None)
    parser.add_argument("--synthetic-diagnostic-promotion", default=None)
    parser.add_argument("--runtime-runner-tag", default="")
    parser.add_argument("--runtime-synthetic-fingerprint-mismatch", default=None)
    parser.add_argument("--runtime-synthetic-inflate", action="append", default=[])
    return parser.parse_args()


def timestamp_utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def atomic_write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = path.with_name(f".{path.name}.tmp.{os.getpid()}.{time.time_ns()}")
    temp_path.write_text(text, encoding="utf-8")
    os.replace(temp_path, path)


def write_json(path: Path, data: dict[str, Any]) -> None:
    atomic_write_text(path, json.dumps(data, indent=2) + "\n")


def write_text(path: Path, text: str) -> None:
    atomic_write_text(path, text)


def resolve_repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def resolve_python_bin(explicit: str | None) -> str:
    return explicit or sys.executable or "/usr/bin/python3"


def resolve_raw_engine_tests(repo_root: Path, explicit: str | None) -> Path:
    candidates: list[Path] = []
    if explicit:
        candidates.append(Path(explicit))
    env_value = os.environ.get("RAW_ENGINE_TESTS")
    if env_value:
        candidates.append(Path(env_value))
    candidates.extend(sorted(repo_root.glob("build-release-phase*/tests/raw_engine_tests"), reverse=True))
    candidates.extend(sorted(repo_root.glob("build-debug-phase*/tests/raw_engine_tests"), reverse=True))
    candidates.extend(sorted(repo_root.glob("build-asan-phase*/tests/raw_engine_tests"), reverse=True))
    candidates.extend(
        [
            repo_root / "build-release" / "tests" / "raw_engine_tests",
            repo_root / "build-debug" / "tests" / "raw_engine_tests",
            repo_root / "build-asan" / "tests" / "raw_engine_tests",
        ]
    )
    for candidate in candidates:
        if candidate.exists() and candidate.is_file():
            return candidate.resolve()
    raise FileNotFoundError("raw_engine_tests executable not found; pass --raw-engine-tests")


def manifest_json_path(value: str | None, default_path: Path | None = None) -> Path:
    if value is None:
        if default_path is None:
            raise ValueError("manifest path missing and no default provided")
        path = default_path.resolve()
    else:
        path = Path(value).resolve()
    return path if path.suffix == ".json" else path.with_suffix(".json")


def default_summary_path(artifact_root: Path, phase: str, mode: str) -> Path:
    return artifact_root / "manifests" / f"policy_pipeline_{mode}_{phase}.json"


def default_report_path(repo_root: Path, phase: str) -> Path:
    date_suffix = datetime.now(timezone.utc).astimezone().strftime("%Y%m%d")
    return repo_root / f"{phase.upper()}_STABILIZATION_REPORT_{date_suffix}.txt"


def default_runtime_path(artifact_root: Path, phase: str, kind: str) -> Path:
    return artifact_root / "manifests" / f"policy_runtime_{kind}_{phase}.json"


def default_runtime_registry_path(artifact_root: Path) -> Path:
    preferred = artifact_root / "manifests" / "runtime_baseline_registry_v2.json"
    if preferred.exists():
        return preferred
    return artifact_root / "manifests" / "runtime_baseline_registry_v1.json"


def default_runtime_history_index_path(artifact_root: Path) -> Path:
    return artifact_root / "manifests" / "runtime_history_index_v1.json"


def default_runtime_proposal_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"runtime_rebaseline_proposal_{phase}.json"


def default_runtime_comparability_triage_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"runtime_comparability_triage_{phase}.json"


def default_runtime_rebaseline_action_plan_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"runtime_rebaseline_action_plan_{phase}.json"


def default_runtime_rebaseline_proposal_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"runtime_rebaseline_action_proposal_{phase}.json"


def default_runtime_rebaseline_proposal_gate_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"runtime_rebaseline_action_proposal_gate_{phase}.json"


def default_runtime_rebaseline_runbook_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"runtime_rebaseline_runbook_{phase}.json"


def default_runtime_registry_selection_sanity_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"runtime_registry_selection_sanity_{phase}.json"


def default_runtime_registry_selection_repair_plan_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"runtime_registry_selection_repair_plan_{phase}.json"


def default_runtime_watch_path(artifact_root: Path, phase: str, kind: str) -> Path:
    return artifact_root / "manifests" / f"runtime_watch_{kind}_{phase}.json"


def default_runtime_watch_history_index_path(artifact_root: Path) -> Path:
    return artifact_root / "manifests" / "runtime_watch_history_index_v1.json"


def default_runtime_watch_registry_path(artifact_root: Path) -> Path:
    return artifact_root / "manifests" / "runtime_watch_registry_v1.json"


def default_runtime_registry_health_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"runtime_registry_health_{phase}.json"


def default_publication_health_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"publication_health_{phase}.json"


def default_source_snapshot_manifest_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"source_snapshot_manifest_{phase}.json"


def default_source_health_preflight_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"source_health_preflight_{phase}.json"


def default_source_health_action_plan_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"source_health_action_plan_{phase}.json"


def default_staged_materialization_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"staged_materialization_{phase}.json"


def default_verification_preflight_gate_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"verification_preflight_gate_{phase}.json"


def default_staged_overlay_completeness_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"staged_overlay_completeness_{phase}.json"


def default_verification_preflight_gate_v2_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"verification_preflight_gate_{phase}_v2.json"


def default_staged_mirror_manifest_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"staged_mirror_manifest_{phase}.json"


def default_staged_mirror_verify_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"staged_mirror_verify_{phase}.json"


def default_ctest_inventory_path(artifact_root: Path, phase: str, mode: str) -> Path:
    return artifact_root / "manifests" / f"ctest_inventory_{mode}_{phase}.json"


def default_verification_path(artifact_root: Path, phase: str, mode: str) -> Path:
    return artifact_root / "manifests" / f"verification_{phase}_{mode}.json"


def default_published_snapshot_manifest_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"{phase}_published_snapshot.json"


def default_verification_closeout_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"{phase}_verification_closeout.json"


def default_ops_summary_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"policy_ops_summary_{phase}.json"


def default_matrix_summary_path(artifact_root: Path, phase: str) -> Path:
    return artifact_root / "manifests" / f"policy_pipeline_matrix_{phase}.json"


def default_zip_path(repo_root: Path, phase: str, curated: bool) -> Path:
    date_suffix = datetime.now(timezone.utc).astimezone().strftime("%Y%m%d")
    stem = f"raw_engine_{phase}_stabilization"
    if curated:
        stem += "_curated"
    return repo_root / f"{stem}_{date_suffix}.zip"


def default_light_ops_zip_path(repo_root: Path, phase: str) -> Path:
    date_suffix = datetime.now(timezone.utc).astimezone().strftime("%Y%m%d")
    return repo_root / f"raw_engine_{phase}_ops_light_{date_suffix}.zip"


def default_published_bundle_root(artifact_root: Path, phase: str) -> Path:
    return artifact_root / f"{phase}_evidence_bundle_published"


def default_runtime_history_compact_path(artifact_root: Path) -> Path:
    return artifact_root / "manifests" / "runtime_history_index_v1_compacted.json"


def infer_pipeline_phase(args: argparse.Namespace) -> str:
    if args.pipeline_phase and str(args.pipeline_phase).strip().lower() not in {"", "auto"}:
        return args.pipeline_phase
    for candidate in (
        args.summary_out,
        args.report_out,
        args.zip_out,
        args.curated_zip,
        args.runtime_watch_current,
        args.runtime_watch_refresh,
    ):
        if not candidate:
            continue
        match = re.search(r"(phase\d+)", str(candidate), re.IGNORECASE)
        if match:
            return match.group(1).lower()
    for candidate in (
        args.runtime_watch_current,
        args.runtime_watch_refresh,
        args.runtime_current_manifest,
        args.runtime_refresh_manifest,
    ):
        if not candidate:
            continue
        path = Path(candidate).resolve()
        if path.exists():
            try:
                embedded_phase = str(read_json(path).get("phase", "")).strip().lower()
                if embedded_phase.startswith("phase"):
                    return embedded_phase
            except Exception:
                pass
    for candidate in (
        args.runtime_current_manifest,
        args.runtime_refresh_manifest,
        args.runtime_proposal,
    ):
        if not candidate:
            continue
        match = re.search(r"(phase\d+)", str(candidate), re.IGNORECASE)
        if match:
            return match.group(1).lower()
    return args.pipeline_phase


def run_command(stage_name: str, command: list[str], cwd: Path) -> dict[str, Any]:
    start = time.monotonic()
    completed = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    duration = round(time.monotonic() - start, 3)
    note = completed.stdout.strip() or completed.stderr.strip()
    if completed.returncode != 0:
        raise RuntimeError(
            json.dumps(
                {
                    "stage": stage_name,
                    "command": command,
                    "returncode": completed.returncode,
                    "output": note,
                },
                ensure_ascii=False,
            )
        )
    return {
        "name": stage_name,
        "status": "PASS",
        "duration_seconds": duration,
        "command": command,
        "note": note,
    }


def skipped_stage(stage_name: str, command: list[str], reason: str) -> dict[str, Any]:
    return {
        "name": stage_name,
        "status": "SKIPPED",
        "duration_seconds": 0.0,
        "command": command,
        "note": reason,
    }


def ensure_report(report_path: Path, phase: str, mode: str, recommendation: str) -> None:
    if report_path.exists():
        return
    write_text(
        report_path,
        "\n".join(
            [
                f"{phase.upper()} policy pipeline operator report",
                f"timestamp_utc={timestamp_utc_now()}",
                f"mode={mode}",
                f"recommended_next_action={recommendation}",
                "",
            ]
        ),
    )


def parse_runtime_stage(text: str) -> tuple[str, float]:
    if "=" not in text:
        raise ValueError(f"runtime stage must be name=seconds: {text}")
    name, value = text.split("=", 1)
    return name.strip(), float(value.strip())


def load_json_if_exists(path: Path | None) -> dict[str, Any]:
    if path is None or not path.exists():
        return {}
    return read_json(path)


def load_runtime_history_summary(runtime_history_index_path: Path | None) -> dict[str, Any]:
    if runtime_history_index_path is None:
        return {}
    summary_path = runtime_history_index_path.with_name(f"{runtime_history_index_path.stem}_summary.json")
    return load_json_if_exists(summary_path)


def load_runtime_watch_history_summary(runtime_watch_history_index_path: Path | None) -> dict[str, Any]:
    if runtime_watch_history_index_path is None:
        return {}
    summary_path = runtime_watch_history_index_path.with_name(f"{runtime_watch_history_index_path.stem}_summary.json")
    return load_json_if_exists(summary_path)


def effective_runtime_baseline_manifest_path(
    configured_path: Path | None,
    runtime_refresh_manifest_path: Path | None,
    runtime_baseline_registry_path: Path | None,
) -> Path | None:
    refresh_manifest = load_json_if_exists(runtime_refresh_manifest_path)
    for key in ("selected_runtime_baseline_manifest_path", "baseline_runtime_manifest_path"):
        value = str(refresh_manifest.get(key, "")).strip()
        if value:
            candidate = Path(value).resolve()
            if candidate.exists():
                return candidate
    if configured_path is not None and configured_path.exists():
        return configured_path
    registry = load_json_if_exists(runtime_baseline_registry_path)
    for entry in registry.get("entries", []):
        if str(entry.get("status", "")).strip().lower() != "active":
            continue
        value = str(entry.get("runtime_baseline_manifest_path", "")).strip()
        if value:
            candidate = Path(value).resolve()
            if candidate.exists():
                return candidate
    return configured_path


def runtime_history_bucket(history_summary: dict[str, Any], runtime_current: dict[str, Any]) -> dict[str, Any]:
    fingerprint_key = runtime_gate.runtime_manifest_fingerprint_key(runtime_current) if runtime_current else ""
    fingerprint_hash = str(runtime_current.get("host_fingerprint", {}).get("fingerprint_hash", ""))
    toolchain = dict(runtime_current.get("toolchain_fingerprint", {}))
    for bucket in history_summary.get("fingerprints", []):
        if fingerprint_key and str(bucket.get("runtime_fingerprint_key", "")) == fingerprint_key:
            return bucket
    for bucket in history_summary.get("fingerprints", []):
        if str(bucket.get("fingerprint_hash", "")) == fingerprint_hash:
            bucket_toolchain = dict(bucket.get("toolchain_fingerprint", {}))
            if (
                str(bucket_toolchain.get("compiler_id", "")) == str(toolchain.get("compiler_id", ""))
                and str(bucket_toolchain.get("compiler_version", "")) == str(toolchain.get("compiler_version", ""))
            ):
                return bucket
    return {}


def build_runtime_trend_summary(history_summary: dict[str, Any], runtime_current: dict[str, Any]) -> dict[str, Any]:
    bucket = runtime_history_bucket(history_summary, runtime_current)
    execution_classes = [dict(entry) for entry in bucket.get("execution_classes", []) if isinstance(entry, dict)]
    strongest_trend = "stable"
    trend_rank = {
        "insufficient_history": 0,
        "stable": 1,
        "improved": 1,
        "noisy": 2,
        "regressing": 3,
    }
    for entry in execution_classes:
        trend = str(entry.get("trend_direction", "insufficient_history"))
        if trend_rank.get(trend, 0) > trend_rank.get(strongest_trend, 0):
            strongest_trend = trend
    return {
        "fingerprint_hash": bucket.get("fingerprint_hash"),
        "trend_counts": history_summary.get("trend_counts", {}),
        "execution_classes": execution_classes,
        "strongest_trend": strongest_trend if execution_classes else "insufficient_history",
    }


def merge_runtime_entries(
    existing_manifest: dict[str, Any],
    runtime_stage_overrides: dict[str, float],
) -> list[dict[str, Any]]:
    merged: dict[str, dict[str, Any]] = {}
    order: list[str] = []
    for entry in existing_manifest.get("entries", []):
        if not isinstance(entry, dict):
            continue
        execution_class = str(entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        merged[execution_class] = {
            "execution_class": execution_class,
            "wall_time_sec": float(entry.get("wall_time_sec", 0.0)),
            "test_count": int(entry.get("test_count", 0)),
            "build_type": str(entry.get("build_type", "")),
            "sanitizer_flags": str(entry.get("sanitizer_flags", "none")),
        }
        budget_thresholds = entry.get("budget_thresholds")
        if isinstance(budget_thresholds, dict) and budget_thresholds:
            merged[execution_class]["budget_thresholds"] = budget_thresholds
        order.append(execution_class)
    for execution_class, seconds in runtime_stage_overrides.items():
        if execution_class not in merged:
            merged[execution_class] = {
                "execution_class": execution_class,
                "wall_time_sec": float(seconds),
                "test_count": 0,
                "build_type": execution_class.split("_", 1)[0].capitalize(),
                "sanitizer_flags": "none",
            }
            order.append(execution_class)
        else:
            merged[execution_class]["wall_time_sec"] = float(seconds)
    return [merged[execution_class] for execution_class in order]


def runtime_entry_spec(entry: dict[str, Any]) -> str:
    parts = [
        f"execution_class={entry['execution_class']}",
        f"wall_time_sec={float(entry.get('wall_time_sec', 0.0))}",
    ]
    if int(entry.get("test_count", 0)):
        parts.append(f"test_count={int(entry.get('test_count', 0))}")
    build_type = str(entry.get("build_type", "")).strip()
    if build_type:
        parts.append(f"build_type={build_type}")
    sanitizer_flags = str(entry.get("sanitizer_flags", "")).strip()
    if sanitizer_flags:
        parts.append(f"sanitizer_flags={sanitizer_flags}")
    return ",".join(parts)


def synthesize_policy_verdicts(current_manifest: dict[str, Any], refresh_manifest: dict[str, Any]) -> tuple[str, str]:
    current_verdict = str(current_manifest.get("current_verdict", "UNKNOWN"))
    freshness_verdict = str(refresh_manifest.get("freshness_verdict", "UNKNOWN"))
    if freshness_verdict == "UNKNOWN":
        if int(refresh_manifest.get("requires_rerun_family_count", 0)) or int(refresh_manifest.get("reclassify_required_count", 0)):
            freshness_verdict = "REQUIRES_RERUN"
        elif int(refresh_manifest.get("stale_family_count", 0)):
            freshness_verdict = "STALE"
        else:
            freshness_verdict = "FRESH"
    return current_verdict, freshness_verdict


def determine_policy_severity(
    current_manifest: dict[str, Any],
    refresh_manifest: dict[str, Any],
    rerun_plan: dict[str, Any],
) -> tuple[str, str, list[str]]:
    current_verdict, freshness_verdict = synthesize_policy_verdicts(current_manifest, refresh_manifest)
    family_statuses = {
        str(family.get("current_status", family.get("status", "")))
        for family in current_manifest.get("families", [])
        if isinstance(family, dict)
    }
    stale_count = int(refresh_manifest.get("stale_family_count", 0))
    requires_count = int(refresh_manifest.get("requires_rerun_family_count", 0))
    reclassify_count = int(refresh_manifest.get("reclassify_required_count", 0))
    selected_entry_count = int(rerun_plan.get("selected_entry_count", 0))
    if current_verdict != "PASS" or "FAIL" in family_statuses or "INSUFFICIENT_EVIDENCE" in family_statuses:
        return "FAIL", "policy gate failed; inspect failing families before approving exact_shadow", [
            "correctness lifecycle contains FAIL or INSUFFICIENT_EVIDENCE",
        ]
    if str(rerun_plan.get("summary_verdict", "")) == "FAIL":
        return "FAIL", "lifecycle execution failed; inspect manifests, rerun execution, or artifact corruption", [
            "correctness rerun plan generation failed",
        ]
    if freshness_verdict == "REQUIRES_RERUN" or requires_count > 0 or reclassify_count > 0:
        if reclassify_count > 0:
            return "ACTION_REQUIRED", "reclassification required; rerun affected families and review exact_shadow policy retention", [
                "correctness family classification changed",
            ]
        return "ACTION_REQUIRED", "rerun required families before treating the policy state as fresh", [
            "correctness refresh reported requires_rerun",
        ]
    if stale_count > 0:
        if selected_entry_count == 0:
            return "ACTION_REQUIRED", "stale evidence detected without a rerun plan; regenerate manifests and inspect lifecycle inputs", [
                "correctness evidence is stale without a rerun plan",
            ]
        return "WARN", "run policy_nightly_refresh or execute the rerun plan to refresh stale evidence", [
            "correctness evidence is stale",
        ]
    return "OK", "no action required; exact_shadow lifecycle remains healthy", [
        "correctness lifecycle is PASS/FRESH",
    ]


def determine_runtime_triage(
    runtime_refresh: dict[str, Any],
    runtime_plan: dict[str, Any],
    runtime_proposal: dict[str, Any],
    runtime_trend_summary: dict[str, Any],
    runtime_watch_refresh: dict[str, Any],
) -> tuple[str, str, str, list[str]]:
    rationale: list[str] = []
    if not runtime_refresh:
        return (
            "ACTION_REQUIRED",
            "REBASELINE_REQUIRED",
            "runtime lifecycle artifacts are missing; regenerate runtime current/refresh/rerun manifests",
            ["runtime lifecycle artifacts are missing"],
        )
    current_verdict = str(runtime_refresh.get("current_verdict", "PASS"))
    comparability_verdict = str(runtime_refresh.get("comparability_verdict", "COMPARABLE"))
    freshness_verdict = str(runtime_refresh.get("freshness_verdict", "FRESH"))
    strongest_trend = str(runtime_trend_summary.get("strongest_trend", "insufficient_history"))
    if comparability_verdict == "COMPARABLE" and freshness_verdict == "FRESH":
        rationale.append("runtime comparability/freshness healthy")
    if current_verdict == "FAIL":
        rationale.append("runtime exceeded the hard budget")
        return "FAIL", "INVESTIGATE_RUNTIME_DRIFT", "runtime exceeded the hard budget; inspect wall-clock regression before approval", rationale
    if comparability_verdict == "REBASELINE_REQUIRED":
        rationale.append(str(runtime_proposal.get("why_rebaseline_is_needed", "runtime registry has no comparable active baseline")))
        if bool(runtime_proposal.get("proposal_needed", False)):
            rationale.append("runtime proposal should be reviewed before promoting a new baseline")
        return "ACTION_REQUIRED", "REBASELINE_REQUIRED", rationale[0], rationale
    if comparability_verdict == "NOT_COMPARABLE":
        rationale.append("runtime baseline selection found only compatible, non-strict candidates")
        return "ACTION_REQUIRED", "NOT_COMPARABLE", "runtime baseline is informational only on this host/toolchain; strict comparison is disabled", rationale
    if bool(runtime_proposal.get("proposal_needed", False)):
        rationale.append(str(runtime_proposal.get("why_rebaseline_is_needed", "runtime proposal recommends a new baseline")))
        return "ACTION_REQUIRED", "PROPOSE_REBASELINE", rationale[-1], rationale
    if freshness_verdict in {"STALE", "REQUIRES_RERUN"} or int(runtime_plan.get("selected_entry_count", 0)) > 0:
        rationale.append("runtime refresh requires rerun or revalidation")
        return "ACTION_REQUIRED", "INVESTIGATE_RUNTIME_DRIFT", "runtime refresh requires rerun or revalidation before treating the ops state as healthy", rationale
    watch_status = str(runtime_watch_refresh.get("overall_watch_status", ""))
    watch_recommendation = str(runtime_watch_refresh.get("overall_watch_recommendation", ""))
    diagnostic_watch_only = bool(runtime_watch_refresh.get("diagnostic_watch_only", False))
    if watch_status == "FAIL":
        rationale.append(str(runtime_watch_refresh.get("overall_watch_reason", "runtime watch reported FAIL")))
        return "FAIL", "FAIL", rationale[-1], rationale
    if watch_status == "WATCH_ESCALATE":
        rationale.append(str(runtime_watch_refresh.get("overall_watch_reason", "runtime watch requires escalation")))
        return "ACTION_REQUIRED", "INVESTIGATE_RUNTIME_DRIFT", rationale[-1], rationale
    if watch_status == "REBASELINE_REQUIRED":
        rationale.append(str(runtime_watch_refresh.get("overall_watch_reason", "runtime watch requires rebaseline")))
        return "ACTION_REQUIRED", "REBASELINE_REQUIRED", rationale[-1], rationale
    if watch_status == "REBASELINE_CANDIDATE":
        rationale.append(str(runtime_watch_refresh.get("overall_watch_reason", "runtime watch suggests a rebaseline candidate")))
        return "WARN", "PROPOSE_RUNTIME_REBASELINE", rationale[-1], rationale
    if watch_status == "WATCH_STABLE":
        rationale.append(str(runtime_watch_refresh.get("overall_watch_reason", "runtime watch remained stable over repeated samples")))
        if diagnostic_watch_only:
            rationale.append("diagnostic-only stable watch remains active; no rebaseline is required yet")
        return (
            "WARN",
            "CONTINUE_MONITORING" if diagnostic_watch_only else (watch_recommendation or "WATCH_RUNTIME"),
            "diagnostic-only stable watch remains active; continue monitoring"
            if diagnostic_watch_only
            else rationale[-1],
            rationale,
        )
    if watch_status == "WATCH":
        rationale.append(str(runtime_watch_refresh.get("overall_watch_reason", "runtime watch is active")))
        return "WARN", watch_recommendation or "WATCH_RUNTIME", rationale[-1], rationale
    if current_verdict == "WARN":
        rationale.append("runtime exceeded the soft budget")
        return "WARN", "WATCH_RUNTIME", "runtime exceeded the soft budget; correctness is stable but the run is slower than baseline", rationale
    if strongest_trend in {"regressing", "noisy"} and watch_status not in {"", "CLEAR"}:
        rationale.append(
            "runtime history shows regressing/noisy trends"
            if strongest_trend == "regressing"
            else "runtime history shows noisy but bounded behavior"
        )
        return "WARN", "WATCH_RUNTIME", "runtime is still within budget, but history indicates drift that should be monitored", rationale
    if strongest_trend == "improved":
        rationale.append("runtime history shows improvement after recent samples or rebaseline")
    rationale.append("runtime budget is within the approved baseline")
    return "OK", "NO_ACTION", "runtime budget is within the approved baseline", rationale


def combine_severity(
    policy_severity: str,
    runtime_severity: str,
    policy_recommendation: str,
    runtime_recommendation: str,
    policy_rationale: list[str],
    runtime_rationale: list[str],
) -> tuple[str, int, str, list[str]]:
    rationale: list[str] = []
    for entry in [*policy_rationale, *runtime_rationale]:
        if entry and entry not in rationale:
            rationale.append(entry)
    if SEVERITY_RANK[runtime_severity] > SEVERITY_RANK[policy_severity]:
        severity = runtime_severity
        recommendation = runtime_recommendation
    elif (
        SEVERITY_RANK[runtime_severity] == SEVERITY_RANK[policy_severity]
        and runtime_severity != "OK"
        and runtime_recommendation not in {"", "NO_ACTION"}
    ):
        severity = runtime_severity
        recommendation = runtime_recommendation
    else:
        severity = policy_severity
        recommendation = policy_recommendation
    exit_code = {
        "OK": EXIT_OK,
        "WARN": EXIT_WARN,
        "ACTION_REQUIRED": EXIT_ACTION_REQUIRED,
        "FAIL": EXIT_FAIL,
    }[severity]
    return severity, exit_code, recommendation, rationale


def pipeline_summary_text(summary: dict[str, Any]) -> str:
    lines = [
        "policy_pipeline_summary"
        + f" mode={summary.get('mode', '')}"
        + f" severity={summary.get('severity', '')}"
        + f" exit_code={summary.get('exit_code', '')}"
        + f" correctness_exit={summary.get('correctness_exit', '')}"
        + f" runtime_exit={summary.get('runtime_exit', '')}"
        + f" combined_exit={summary.get('combined_exit', '')}"
        + f" current_verdict={summary.get('current_verdict', '')}"
        + f" freshness_verdict={summary.get('freshness_verdict', '')}"
        + f" rerun_plan_verdict={summary.get('rerun_plan_verdict', '')}"
        + f" stale_family_count={summary.get('stale_family_count', '')}"
        + f" requires_rerun_family_count={summary.get('requires_rerun_family_count', '')}"
        + f" reclassify_required_count={summary.get('reclassify_required_count', '')}"
        + f" runtime_current_verdict={summary.get('runtime_current_verdict', '')}"
        + f" runtime_freshness_verdict={summary.get('runtime_freshness_verdict', '')}"
        + f" runtime_comparability_verdict={summary.get('runtime_comparability_verdict', '')}"
        + f" runtime_selected_baseline_id={summary.get('runtime_selected_baseline_id', '')}"
        + f" runtime_rerun_plan_verdict={summary.get('runtime_rerun_plan_verdict', '')}"
        + f" runtime_budget_verdict={summary.get('runtime_budget_verdict', '')}"
        + f" runtime_severity={summary.get('runtime_severity', '')}"
        + f" runtime_recommendation={summary.get('runtime_recommendation', '')}"
        + f" runtime_watch_status={summary.get('runtime_watch_status', '')}"
        + f" runtime_watch_recommendation={summary.get('runtime_watch_recommendation', '')}"
        + f" runtime_watch_fingerprint_count={summary.get('runtime_watch_fingerprint_count', '')}"
        + f" runtime_rebaseline_proposal_needed={int(bool(summary.get('runtime_rebaseline_proposal_needed', False)))}"
        + f" current_env_watch_confidence={summary.get('current_env_watch_confidence', '')}"
        + f" new_env_watch_confidence={summary.get('new_env_watch_confidence', '')}"
        + f" source_health_status={summary.get('source_health_status', '')}"
        + f" source_health_recommendation={summary.get('source_health_recommendation', '')}"
        + f" materialization_mode={summary.get('materialization_mode', '')}",
        f"recommended_next_action={summary.get('recommended_next_action', '')}",
    ]
    for key in (
        "baseline_manifest",
        "current_manifest",
        "refresh_manifest",
        "rerun_plan",
        "runtime_baseline_manifest",
        "runtime_current_manifest",
        "runtime_refresh_manifest",
        "runtime_rerun_plan",
        "runtime_baseline_registry",
        "runtime_history_index",
        "runtime_history_summary",
        "runtime_proposal",
        "runtime_comparability_triage",
        "runtime_rebaseline_action_plan",
        "runtime_rebaseline_runbook",
        "runtime_registry_selection_sanity",
        "runtime_registry_selection_repair_plan",
        "bundle_metadata",
        "runtime_watch_current",
        "runtime_watch_refresh",
        "runtime_watch_history_index",
        "runtime_watch_history_summary",
        "source_health_preflight",
        "staged_materialization",
    ):
        if summary.get(key):
            lines.append(f"{key}={summary[key]}")
    for rationale in summary.get("rationale_list", []):
        lines.append(f"rationale={rationale}")
    for stage in summary.get("stages", []):
        lines.append(
            "stage="
            + f"name={stage['name']}"
            + f" status={stage['status']}"
            + f" duration_seconds={stage['duration_seconds']}"
            + f" note={stage.get('note', '')}"
        )
    return "\n".join(lines) + "\n"


def write_pipeline_summary(path: Path, summary: dict[str, Any]) -> None:
    write_json(path, summary)
    write_text(path.with_suffix(".txt"), pipeline_summary_text(summary))


def enrich_summary_with_operator_artifacts(
    summary: dict[str, Any],
    runtime_watch_registry_path: Path | None,
    ops_summary_path: Path | None,
    runtime_registry_health_path: Path | None = None,
    publication_health_path: Path | None = None,
    source_health_preflight_path: Path | None = None,
    source_health_action_plan_path: Path | None = None,
    verification_preflight_gate_path: Path | None = None,
    staged_overlay_completeness_path: Path | None = None,
    verification_preflight_gate_v2_path: Path | None = None,
    staged_materialization_path: Path | None = None,
    runtime_comparability_triage_path: Path | None = None,
    runtime_rebaseline_action_plan_path: Path | None = None,
    runtime_rebaseline_runbook_path: Path | None = None,
    runtime_registry_selection_sanity_path: Path | None = None,
    runtime_registry_selection_repair_plan_path: Path | None = None,
) -> dict[str, Any]:
    enriched = dict(summary)
    enriched["runtime_watch_registry"] = None if runtime_watch_registry_path is None else str(runtime_watch_registry_path)
    enriched["policy_ops_summary"] = None if ops_summary_path is None else str(ops_summary_path)
    enriched["runtime_registry_health"] = None if runtime_registry_health_path is None else str(runtime_registry_health_path)
    enriched["publication_health"] = None if publication_health_path is None else str(publication_health_path)
    enriched["source_health_preflight"] = None if source_health_preflight_path is None else str(source_health_preflight_path)
    enriched["source_health_action_plan"] = None if source_health_action_plan_path is None else str(source_health_action_plan_path)
    enriched["verification_preflight_gate"] = None if verification_preflight_gate_path is None else str(verification_preflight_gate_path)
    enriched["staged_overlay_completeness"] = None if staged_overlay_completeness_path is None else str(staged_overlay_completeness_path)
    enriched["verification_preflight_gate_v2"] = None if verification_preflight_gate_v2_path is None else str(verification_preflight_gate_v2_path)
    enriched["staged_materialization"] = None if staged_materialization_path is None else str(staged_materialization_path)
    enriched["runtime_comparability_triage"] = None if runtime_comparability_triage_path is None else str(runtime_comparability_triage_path)
    enriched["runtime_rebaseline_action_plan"] = None if runtime_rebaseline_action_plan_path is None else str(runtime_rebaseline_action_plan_path)
    enriched["runtime_rebaseline_runbook"] = None if runtime_rebaseline_runbook_path is None else str(runtime_rebaseline_runbook_path)
    enriched["runtime_registry_selection_sanity"] = None if runtime_registry_selection_sanity_path is None else str(runtime_registry_selection_sanity_path)
    enriched["runtime_registry_selection_repair_plan"] = None if runtime_registry_selection_repair_plan_path is None else str(runtime_registry_selection_repair_plan_path)

    runtime_watch_registry = load_json_if_exists(runtime_watch_registry_path)
    ops_summary = load_json_if_exists(ops_summary_path)
    runtime_registry_health = load_json_if_exists(runtime_registry_health_path)
    publication_health = load_json_if_exists(publication_health_path)
    source_health_preflight = load_json_if_exists(source_health_preflight_path)
    source_health_action_plan = load_json_if_exists(source_health_action_plan_path)
    verification_preflight_gate = load_json_if_exists(verification_preflight_gate_path)
    staged_overlay_completeness = load_json_if_exists(staged_overlay_completeness_path)
    verification_preflight_gate_v2 = load_json_if_exists(verification_preflight_gate_v2_path)
    staged_materialization = load_json_if_exists(staged_materialization_path)
    runtime_comparability_triage = load_json_if_exists(runtime_comparability_triage_path)
    runtime_rebaseline_action_plan = load_json_if_exists(runtime_rebaseline_action_plan_path)
    runtime_rebaseline_runbook = load_json_if_exists(runtime_rebaseline_runbook_path)
    runtime_registry_selection_sanity = load_json_if_exists(runtime_registry_selection_sanity_path)
    runtime_registry_selection_repair_plan = load_json_if_exists(runtime_registry_selection_repair_plan_path)
    final_operator_summary = ops_summary.get("final_operator_summary", {})
    final_operator_actions = ops_summary.get("final_operator_actions", {})

    enriched["runtime_watch_registry_entry_count"] = int(runtime_watch_registry.get("entry_count", 0))
    enriched["runtime_watch_registry_fingerprint_count"] = int(runtime_watch_registry.get("fingerprint_count", 0))
    enriched["runtime_registry_health_status"] = runtime_registry_health.get("overall_status") or runtime_registry_health.get("status")
    enriched["publication_health_status"] = publication_health.get("status")
    enriched["current_env_operator_action"] = final_operator_summary.get("recommended_action_current_env")
    enriched["new_env_operator_action"] = final_operator_summary.get("recommended_action_new_env")
    enriched["operator_rationale_list"] = final_operator_summary.get("rationale", [])
    enriched["final_operator_actions"] = final_operator_actions
    enriched["action_domains"] = {
        "correctness": final_operator_actions.get("correctness"),
        "runtime_comparability": final_operator_actions.get("runtime_comparability"),
        "source_health": final_operator_actions.get("source_health"),
        "archive_governance": final_operator_actions.get("archive_governance")
            or final_operator_actions.get("operator_archive_review_queue"),
        "retention": final_operator_actions.get("published_snapshot_retention_apply_v2")
            or final_operator_actions.get("published_snapshot_retention"),
    }
    enriched["rationale"] = (
        final_operator_actions.get("structured_rationale")
        or final_operator_actions.get("rationale")
        or final_operator_summary.get("rationale", [])
    )
    enriched["current_env_watch_confidence"] = ops_summary.get("current_env_summary", {}).get("watch_confidence")
    enriched["new_env_watch_confidence"] = ops_summary.get("new_env_summary", {}).get("watch_confidence")
    enriched["current_env_evidence_source_counts"] = ops_summary.get("current_env_summary", {}).get("evidence_source_counts", {})
    enriched["new_env_evidence_source_counts"] = ops_summary.get("new_env_summary", {}).get("evidence_source_counts", {})
    enriched["verification_lane_status"] = ops_summary.get("verification_lane", {}).get("status")
    enriched["verification_not_run_count"] = ops_summary.get("verification_lane", {}).get("verification_not_run_count", 0)
    enriched["verification_action"] = ops_summary.get("final_operator_actions", {}).get("recommended_action_verification")
    enriched["verification_preflight_gate_verdict"] = verification_preflight_gate.get("preflight_gate_verdict")
    enriched["verification_preflight_direct_build_allowed"] = verification_preflight_gate.get("direct_build_allowed")
    enriched["verification_preflight_staged_build_allowed"] = verification_preflight_gate.get("staged_build_allowed")
    enriched["verification_preflight_sparse_clone_required"] = verification_preflight_gate.get("sparse_clone_required")
    enriched["staged_overlay_completeness_verdict"] = staged_overlay_completeness.get("completeness_verdict")
    enriched["staged_overlay_missing_count"] = staged_overlay_completeness.get("missing_overlay_count", 0)
    enriched["verification_preflight_gate_v2_verdict"] = verification_preflight_gate_v2.get("preflight_gate_verdict")
    enriched["verification_preflight_v2_staged_build_allowed"] = verification_preflight_gate_v2.get("staged_build_allowed")
    enriched["verification_preflight_v2_sparse_clone_required"] = verification_preflight_gate_v2.get("sparse_clone_required")
    enriched["source_health_status"] = source_health_preflight.get("status")
    enriched["source_health_recommendation"] = source_health_preflight.get("recommendation")
    enriched["source_health_dataless_placeholder_count"] = source_health_preflight.get("dataless_placeholder_count", 0)
    enriched["source_health_missing_head_object"] = source_health_preflight.get("git_object_health", {}).get("missing_head_object")
    enriched["source_health_action_plan_hash"] = source_health_action_plan.get("plan_hash")
    enriched["direct_build_blocked"] = bool(source_health_action_plan.get("direct_build_blocked", False))
    enriched["staged_build_allowed"] = bool(source_health_action_plan.get("staged_build_allowed", False))
    enriched["source_health_plan_recommended_action"] = source_health_action_plan.get("recommended_action")
    enriched["materialization_mode"] = staged_materialization.get("staged_materialization_mode") or source_health_preflight.get("staged_materialization", {}).get("staged_materialization_mode")
    enriched["runtime_comparability_triage_verdict"] = runtime_comparability_triage.get("triage_verdict")
    enriched["runtime_comparability_recommended_action"] = runtime_comparability_triage.get("recommended_action")
    enriched["runtime_rebaseline_action_plan_verdict"] = runtime_rebaseline_action_plan.get("action_plan_verdict")
    enriched["runtime_rebaseline_action"] = final_operator_actions.get("runtime_comparability") or runtime_rebaseline_action_plan.get("action_plan_verdict")
    enriched["runtime_action_verdict"] = enriched["runtime_rebaseline_action"]
    enriched["runtime_rebaseline_proposal_gate_verdict"] = runtime_rebaseline_action_plan.get("proposal_gate_verdict")
    enriched["runtime_rebaseline_runbook_approval_ready"] = runtime_rebaseline_runbook.get("approval_ready")
    enriched["runtime_registry_selection_sanity_verdict"] = runtime_registry_selection_sanity.get("sanity_verdict")
    enriched["runtime_registry_selection_repair_plan_verdict"] = runtime_registry_selection_repair_plan.get("repair_plan_verdict")
    return enriched


def materialize_operator_artifacts(
    *,
    python_bin: str,
    watch_ops_script: Path,
    repo_root: Path,
    artifact_root: Path,
    phase: str,
    policy_manifest_path: Path,
    runtime_refresh_manifest_path: Path,
    runtime_history_index_path: Path | None,
    runtime_watch_current_path: Path | None,
    runtime_watch_refresh_path: Path | None,
    runtime_watch_history_index_path: Path | None,
    runtime_watch_registry_path: Path,
    ops_summary_path: Path,
    runtime_baseline_registry_path: Path | None,
    runtime_registry_health_path: Path,
    publication_health_path: Path | None,
    source_snapshot_manifest_path: Path | None = None,
    source_health_preflight_path: Path | None = None,
    staged_overlay_completeness_path: Path | None = None,
    verification_preflight_gate_v2_path: Path | None = None,
    staged_materialization_path: Path | None = None,
    staged_mirror_verify_path: Path | None = None,
    verification_release_path: Path | None = None,
    verification_debug_path: Path | None = None,
    verification_asan_path: Path | None = None,
    published_snapshot_manifest_path: Path | None = None,
    verification_closeout_path: Path | None = None,
    approved_known_summary_paths: list[Path] | None = None,
    foreign_import_summary_paths: list[Path] | None = None,
    current_env_governance_policy_path: Path | None = None,
    current_env_guardrail_policy_path: Path | None = None,
    current_env_watch_current_path_manifest: Path | None = None,
    current_env_watch_refresh_path_manifest: Path | None = None,
    current_env_watch_history_path_manifest: Path | None = None,
    current_env_age_tick_path: Path | None = None,
    current_env_watch_plan_path: Path | None = None,
    current_env_trigger_gate_path: Path | None = None,
    runtime_budget_current_path_manifest: Path | None = None,
    runtime_budget_proposal_path_manifest: Path | None = None,
    runtime_budget_proposal_gate_path_manifest: Path | None = None,
    runtime_budget_baseline_path_manifest: Path | None = None,
    runtime_budget_refresh_path_manifest: Path | None = None,
    runtime_budget_reproposal_history_path: Path | None = None,
    runtime_budget_registry_summary_path: Path | None = None,
    runtime_comparability_triage_path: Path | None = None,
    runtime_rebaseline_action_plan_path: Path | None = None,
    runtime_rebaseline_proposal_path: Path | None = None,
    runtime_rebaseline_proposal_gate_path: Path | None = None,
    runtime_rebaseline_runbook_path: Path | None = None,
    runtime_registry_selection_sanity_path: Path | None = None,
    runtime_registry_selection_repair_plan_path: Path | None = None,
) -> list[dict[str, Any]]:
    stages: list[dict[str, Any]] = []
    approved_known_summary_paths = approved_known_summary_paths or []
    foreign_import_summary_paths = foreign_import_summary_paths or []
    matrix_summary_path = default_matrix_summary_path(artifact_root, phase)
    quick_summary_path = default_summary_path(artifact_root, phase, "quick")
    nightly_summary_path = default_summary_path(artifact_root, phase, "nightly")

    stages.append(
        run_command(
            "runtime_watch_registry",
            [
                python_bin,
                str(watch_ops_script),
                "watch-registry",
                *(["--watch-current", str(runtime_watch_current_path)] if runtime_watch_current_path is not None else []),
                *(["--watch-refresh", str(runtime_watch_refresh_path)] if runtime_watch_refresh_path is not None else []),
                *(["--watch-history-index", str(runtime_watch_history_index_path)] if runtime_watch_history_index_path is not None else []),
                "--matrix-summary",
                str(matrix_summary_path),
                "--matrix-root",
                str(artifact_root / "matrix"),
                "--registry-out",
                str(runtime_watch_registry_path),
            ],
            repo_root,
        )
    )

    if runtime_baseline_registry_path is not None:
        stages.append(
            run_command(
                "runtime_registry_health",
                [
                    python_bin,
                    str(watch_ops_script),
                    "registry-health",
                    "--phase",
                    phase,
                    "--runtime-baseline-registry",
                    str(runtime_baseline_registry_path),
                    "--runtime-history-index",
                    str(runtime_history_index_path or default_runtime_history_index_path(artifact_root)),
                    "--runtime-watch-registry",
                    str(runtime_watch_registry_path),
                    "--runtime-refresh",
                    str(runtime_refresh_manifest_path),
                    *sum([["--approved-known-summary", str(path)] for path in approved_known_summary_paths], []),
                    *sum([["--foreign-import-summary", str(path)] for path in foreign_import_summary_paths], []),
                    "--health-out",
                    str(runtime_registry_health_path),
                ],
                repo_root,
            )
        )

    stages.append(
        run_command(
            "policy_ops_summary",
            [
                python_bin,
                str(watch_ops_script),
                "ops-summary",
                "--phase",
                phase,
                "--policy-manifest",
                str(policy_manifest_path),
                "--quick-summary",
                str(quick_summary_path),
                "--nightly-summary",
                str(nightly_summary_path),
                "--matrix-summary",
                str(matrix_summary_path),
                "--runtime-refresh",
                str(runtime_refresh_manifest_path),
                *(["--runtime-watch-refresh", str(runtime_watch_refresh_path)] if runtime_watch_refresh_path is not None else []),
                "--runtime-watch-registry",
                str(runtime_watch_registry_path),
                *(["--runtime-baseline-registry", str(runtime_baseline_registry_path)] if runtime_baseline_registry_path is not None else []),
                *(["--runtime-registry-health", str(runtime_registry_health_path)] if runtime_registry_health_path is not None else []),
                *(["--publication-health", str(publication_health_path)] if publication_health_path is not None and publication_health_path.exists() else []),
                *(["--source-snapshot-manifest", str(source_snapshot_manifest_path)] if source_snapshot_manifest_path is not None and source_snapshot_manifest_path.exists() else []),
                *(["--source-health-preflight", str(source_health_preflight_path)] if source_health_preflight_path is not None and source_health_preflight_path.exists() else []),
                *(["--staged-overlay-completeness", str(staged_overlay_completeness_path)] if staged_overlay_completeness_path is not None and staged_overlay_completeness_path.exists() else []),
                *(["--verification-preflight-gate-v2", str(verification_preflight_gate_v2_path)] if verification_preflight_gate_v2_path is not None and verification_preflight_gate_v2_path.exists() else []),
                *(["--staged-materialization", str(staged_materialization_path)] if staged_materialization_path is not None and staged_materialization_path.exists() else []),
                *(["--staged-mirror-verify", str(staged_mirror_verify_path)] if staged_mirror_verify_path is not None and staged_mirror_verify_path.exists() else []),
                *(["--verification-release", str(verification_release_path)] if verification_release_path is not None and verification_release_path.exists() else []),
                *(["--verification-debug", str(verification_debug_path)] if verification_debug_path is not None and verification_debug_path.exists() else []),
                *(["--verification-asan", str(verification_asan_path)] if verification_asan_path is not None and verification_asan_path.exists() else []),
                *(["--published-snapshot-manifest", str(published_snapshot_manifest_path)] if published_snapshot_manifest_path is not None and published_snapshot_manifest_path.exists() else []),
                *(["--verification-closeout", str(verification_closeout_path)] if verification_closeout_path is not None and verification_closeout_path.exists() else []),
                *(["--current-env-governance-policy", str(current_env_governance_policy_path)] if current_env_governance_policy_path is not None and current_env_governance_policy_path.exists() else []),
                *(["--current-env-guardrail-policy", str(current_env_guardrail_policy_path)] if current_env_guardrail_policy_path is not None and current_env_guardrail_policy_path.exists() else []),
                *(["--current-env-watch-current", str(current_env_watch_current_path_manifest)] if current_env_watch_current_path_manifest is not None and current_env_watch_current_path_manifest.exists() else []),
                *(["--current-env-watch-refresh", str(current_env_watch_refresh_path_manifest)] if current_env_watch_refresh_path_manifest is not None and current_env_watch_refresh_path_manifest.exists() else []),
                *(["--current-env-watch-history", str(current_env_watch_history_path_manifest)] if current_env_watch_history_path_manifest is not None and current_env_watch_history_path_manifest.exists() else []),
                *(["--current-env-age-tick", str(current_env_age_tick_path)] if current_env_age_tick_path is not None and current_env_age_tick_path.exists() else []),
                *(["--current-env-watch-plan", str(current_env_watch_plan_path)] if current_env_watch_plan_path is not None and current_env_watch_plan_path.exists() else []),
                *(["--current-env-trigger-gate", str(current_env_trigger_gate_path)] if current_env_trigger_gate_path is not None and current_env_trigger_gate_path.exists() else []),
                *(["--runtime-budget-current", str(runtime_budget_current_path_manifest)] if runtime_budget_current_path_manifest is not None and runtime_budget_current_path_manifest.exists() else []),
                *(["--runtime-budget-proposal", str(runtime_budget_proposal_path_manifest)] if runtime_budget_proposal_path_manifest is not None and runtime_budget_proposal_path_manifest.exists() else []),
                *(["--runtime-budget-proposal-gate", str(runtime_budget_proposal_gate_path_manifest)] if runtime_budget_proposal_gate_path_manifest is not None and runtime_budget_proposal_gate_path_manifest.exists() else []),
                *(["--runtime-budget-baseline", str(runtime_budget_baseline_path_manifest)] if runtime_budget_baseline_path_manifest is not None and runtime_budget_baseline_path_manifest.exists() else []),
                *(["--runtime-budget-refresh", str(runtime_budget_refresh_path_manifest)] if runtime_budget_refresh_path_manifest is not None and runtime_budget_refresh_path_manifest.exists() else []),
                *(["--runtime-budget-reproposal-history", str(runtime_budget_reproposal_history_path)] if runtime_budget_reproposal_history_path is not None and runtime_budget_reproposal_history_path.exists() else []),
                *(["--runtime-budget-registry-summary", str(runtime_budget_registry_summary_path)] if runtime_budget_registry_summary_path is not None and runtime_budget_registry_summary_path.exists() else []),
                *(["--runtime-comparability-triage", str(runtime_comparability_triage_path)] if runtime_comparability_triage_path is not None and runtime_comparability_triage_path.exists() else []),
                *(["--runtime-rebaseline-action-plan", str(runtime_rebaseline_action_plan_path)] if runtime_rebaseline_action_plan_path is not None and runtime_rebaseline_action_plan_path.exists() else []),
                *(["--runtime-rebaseline-proposal", str(runtime_rebaseline_proposal_path)] if runtime_rebaseline_proposal_path is not None and runtime_rebaseline_proposal_path.exists() else []),
                *(["--runtime-rebaseline-proposal-gate", str(runtime_rebaseline_proposal_gate_path)] if runtime_rebaseline_proposal_gate_path is not None and runtime_rebaseline_proposal_gate_path.exists() else []),
                *(["--runtime-rebaseline-runbook", str(runtime_rebaseline_runbook_path)] if runtime_rebaseline_runbook_path is not None and runtime_rebaseline_runbook_path.exists() else []),
                *(["--runtime-registry-selection-sanity", str(runtime_registry_selection_sanity_path)] if runtime_registry_selection_sanity_path is not None and runtime_registry_selection_sanity_path.exists() else []),
                *(["--runtime-registry-selection-repair-plan", str(runtime_registry_selection_repair_plan_path)] if runtime_registry_selection_repair_plan_path is not None and runtime_registry_selection_repair_plan_path.exists() else []),
                *sum([["--approved-known-summary", str(path)] for path in approved_known_summary_paths], []),
                *sum([["--foreign-import-summary", str(path)] for path in foreign_import_summary_paths], []),
                "--out",
                str(ops_summary_path),
            ],
            repo_root,
        )
    )
    return stages


def build_summary(
    mode: str,
    phase: str,
    artifact_root: Path,
    baseline_manifest_path: Path,
    current_manifest_path: Path,
    refresh_manifest_path: Path,
    rerun_plan_path: Path,
    runtime_baseline_manifest_path: Path,
    runtime_baseline_registry_path: Path | None,
    runtime_current_manifest_path: Path,
    runtime_refresh_manifest_path: Path,
    runtime_rerun_plan_path: Path,
    runtime_history_index_path: Path | None,
    runtime_proposal_path: Path | None,
    runtime_watch_current_path: Path | None,
    runtime_watch_refresh_path: Path | None,
    runtime_watch_history_index_path: Path | None,
    bundle_metadata_path: Path | None,
    stages: list[dict[str, Any]],
) -> dict[str, Any]:
    current_manifest = read_json(current_manifest_path)
    refresh_manifest = read_json(refresh_manifest_path)
    rerun_plan = read_json(rerun_plan_path)
    runtime_current = load_json_if_exists(runtime_current_manifest_path)
    runtime_refresh = load_json_if_exists(runtime_refresh_manifest_path)
    runtime_plan = load_json_if_exists(runtime_rerun_plan_path)
    runtime_registry = load_json_if_exists(runtime_baseline_registry_path)
    runtime_history = load_json_if_exists(runtime_history_index_path)
    runtime_history_summary = load_runtime_history_summary(runtime_history_index_path)
    runtime_trend_summary = build_runtime_trend_summary(runtime_history_summary, runtime_current)
    runtime_proposal = load_json_if_exists(runtime_proposal_path)
    runtime_watch_current = load_json_if_exists(runtime_watch_current_path)
    runtime_watch_refresh = load_json_if_exists(runtime_watch_refresh_path)
    runtime_watch_history = load_json_if_exists(runtime_watch_history_index_path)
    runtime_watch_history_summary = load_runtime_watch_history_summary(runtime_watch_history_index_path)
    current_verdict, freshness_verdict = synthesize_policy_verdicts(current_manifest, refresh_manifest)
    policy_severity, policy_recommendation, policy_rationale = determine_policy_severity(current_manifest, refresh_manifest, rerun_plan)
    runtime_severity, runtime_action, runtime_recommendation, runtime_rationale = determine_runtime_triage(
        runtime_refresh,
        runtime_plan,
        runtime_proposal,
        runtime_trend_summary,
        runtime_watch_refresh,
    )
    if policy_severity == "OK" and runtime_severity in {"WARN", "ACTION_REQUIRED"}:
        runtime_rationale = [
            *runtime_rationale,
            "runtime comparability/action-required state is separated from correctness; no semantic failure was detected",
        ]
    severity, exit_code, recommendation, combined_rationale = combine_severity(
        policy_severity,
        runtime_severity,
        policy_recommendation,
        runtime_recommendation,
        policy_rationale,
        runtime_rationale,
    )
    return {
        "summary_version": "policy_pipeline_summary_v4",
        "timestamp_utc": timestamp_utc_now(),
        "phase": phase,
        "mode": mode,
        "artifact_root": str(artifact_root),
        "baseline_manifest": str(baseline_manifest_path),
        "current_manifest": str(current_manifest_path),
        "refresh_manifest": str(refresh_manifest_path),
        "rerun_plan": str(rerun_plan_path),
        "runtime_baseline_manifest": str(runtime_baseline_manifest_path),
        "runtime_baseline_registry": None if runtime_baseline_registry_path is None else str(runtime_baseline_registry_path),
        "runtime_current_manifest": str(runtime_current_manifest_path),
        "runtime_refresh_manifest": str(runtime_refresh_manifest_path),
        "runtime_rerun_plan": str(runtime_rerun_plan_path),
        "runtime_history_index": None if runtime_history_index_path is None else str(runtime_history_index_path),
        "runtime_history_summary": None if runtime_history_index_path is None else str(runtime_history_index_path.with_name(f"{runtime_history_index_path.stem}_summary.json")),
        "runtime_history_compact": None
        if runtime_history_index_path is None
        else str(runtime_history_index_path.with_name("runtime_history_index_v1_compacted.json")),
        "runtime_proposal": None if runtime_proposal_path is None else str(runtime_proposal_path),
        "runtime_watch_current": None if runtime_watch_current_path is None else str(runtime_watch_current_path),
        "runtime_watch_refresh": None if runtime_watch_refresh_path is None else str(runtime_watch_refresh_path),
        "runtime_watch_history_index": None if runtime_watch_history_index_path is None else str(runtime_watch_history_index_path),
        "runtime_watch_history_summary": None if runtime_watch_history_index_path is None else str(runtime_watch_history_index_path.with_name(f"{runtime_watch_history_index_path.stem}_summary.json")),
        "bundle_metadata": None if bundle_metadata_path is None else str(bundle_metadata_path),
        "current_verdict": current_verdict,
        "freshness_verdict": freshness_verdict,
        "rerun_plan_verdict": str(rerun_plan.get("summary_verdict", "UNKNOWN")),
        "stale_family_count": int(refresh_manifest.get("stale_family_count", 0)),
        "requires_rerun_family_count": int(refresh_manifest.get("requires_rerun_family_count", 0)),
        "reclassify_required_count": int(refresh_manifest.get("reclassify_required_count", 0)),
        "runtime_current_verdict": str(runtime_refresh.get("current_verdict", runtime_current.get("current_verdict", "UNKNOWN"))),
        "runtime_freshness_verdict": str(runtime_refresh.get("freshness_verdict", "UNKNOWN")),
        "runtime_comparability_verdict": str(runtime_refresh.get("comparability_verdict", "UNKNOWN")),
        "runtime_selected_baseline_id": runtime_refresh.get("selected_baseline_id") or runtime_proposal.get("selected_baseline_id"),
        "runtime_selected_baseline_tag": runtime_refresh.get("selected_baseline_tag") or runtime_proposal.get("selected_baseline_tag"),
        "runtime_rerun_plan_verdict": str(runtime_plan.get("summary_verdict", "UNKNOWN")),
        "runtime_budget_verdict": str(runtime_refresh.get("overall_budget_verdict", runtime_current.get("overall_budget_verdict", "UNKNOWN"))),
        "runtime_stale_entry_count": int(runtime_refresh.get("stale_entry_count", 0)),
        "runtime_requires_rerun_entry_count": int(runtime_refresh.get("requires_rerun_entry_count", 0)),
        "runtime_rebaseline_required_count": int(runtime_refresh.get("rebaseline_required_count", 0)),
        "runtime_not_comparable_count": int(runtime_refresh.get("not_comparable_count", 0)),
        "runtime_warn_count": int(runtime_refresh.get("warn_count", 0)),
        "runtime_fail_count": int(runtime_refresh.get("fail_count", 0)),
        "runtime_registry_entry_count": len(runtime_registry.get("entries", [])),
        "runtime_history_fingerprint_count": len(runtime_history.get("fingerprints", [])),
        "runtime_history_trend_counts": runtime_history_summary.get("trend_counts", {}),
        "runtime_trend_summary": runtime_trend_summary,
        "runtime_rebaseline_proposal_needed": bool(runtime_proposal.get("proposal_needed", False)),
        "runtime_rebaseline_proposal_reason": runtime_proposal.get("why_rebaseline_is_needed"),
        "runtime_watch_status": runtime_watch_refresh.get("overall_watch_status") or runtime_watch_current.get("overall_watch_status"),
        "runtime_watch_reason": runtime_watch_refresh.get("overall_watch_reason") or runtime_watch_current.get("overall_watch_reason"),
        "runtime_watch_recommendation": runtime_watch_refresh.get("overall_watch_recommendation") or runtime_watch_current.get("overall_watch_recommendation"),
        "runtime_watch_sample_count": runtime_watch_refresh.get("runtime_watch_sample_count") or runtime_watch_current.get("watch_sample_count"),
        "runtime_watch_diagnostic_only": runtime_watch_refresh.get("diagnostic_watch_only"),
        "runtime_watch_fingerprint_count": runtime_watch_history_summary.get("fingerprint_count", 0),
        "runtime_watch_history_fingerprint_count": len(runtime_watch_history.get("fingerprints", [])),
        "runtime_watch_history_status_counts": runtime_watch_history_summary.get("watch_status_counts", {}),
        "runtime_watch_transition_summary": {
            "transition_count": runtime_watch_history_summary.get("transition_count", 0),
            "watch_transition_counts": runtime_watch_history_summary.get("watch_transition_counts", {}),
            "recent_transitions": runtime_watch_history_summary.get("recent_transitions", []),
            "strongest_watch_status": runtime_watch_history_summary.get(
                "strongest_watch_status",
                runtime_watch_refresh.get("overall_watch_status") or runtime_watch_current.get("overall_watch_status"),
            ),
        },
        "runtime_budget_profile_id": runtime_watch_refresh.get("runtime_budget_profile_id")
        or runtime_watch_current.get("runtime_budget_profile_id")
        or runtime_current.get("runtime_budget_profile_id"),
        "correctness_severity": policy_severity,
        "correctness_exit": {
            "OK": EXIT_OK,
            "WARN": EXIT_WARN,
            "ACTION_REQUIRED": EXIT_ACTION_REQUIRED,
            "FAIL": EXIT_FAIL,
        }[policy_severity],
        "runtime_exit": {
            "OK": EXIT_OK,
            "WARN": EXIT_WARN,
            "ACTION_REQUIRED": EXIT_ACTION_REQUIRED,
            "FAIL": EXIT_FAIL,
        }[runtime_severity],
        "combined_exit": exit_code,
        "semantic_failure_detected": policy_severity == "FAIL",
        "correctness_action_domain": "correctness",
        "runtime_action_domain": "runtime_comparability",
        "policy_severity": policy_severity,
        "policy_rationale": policy_rationale,
        "runtime_severity": runtime_severity,
        "runtime_recommendation": runtime_action,
        "runtime_rationale": runtime_rationale,
        "severity": severity,
        "exit_code": exit_code,
        "recommended_next_action": recommendation,
        "rationale_list": combined_rationale,
        "stages": stages,
    }


def runtime_matrix_entry_paths(artifact_root: Path, name: str) -> dict[str, Path]:
    root = artifact_root / "matrix" / name
    root.mkdir(parents=True, exist_ok=True)
    return {
        "refresh": root / f"{name}_runtime_refresh.json",
        "rerun": root / f"{name}_runtime_rerun.json",
        "proposal": root / f"{name}_runtime_proposal.json",
        "watch_current": root / f"{name}_runtime_watch_current.json",
        "watch_refresh": root / f"{name}_runtime_watch_refresh.json",
        "watch_history": root / f"{name}_runtime_watch_history.json",
    }


def build_matrix_entry_summary(
    name: str,
    runtime_refresh: dict[str, Any],
    runtime_plan: dict[str, Any],
    runtime_proposal: dict[str, Any],
    runtime_watch_refresh: dict[str, Any],
    runtime_trend_summary: dict[str, Any],
    expected_comparability: str | None,
    expected_action: str | None,
) -> dict[str, Any]:
    severity, action, recommendation, rationale = determine_runtime_triage(
        runtime_refresh,
        runtime_plan,
        runtime_proposal,
        runtime_trend_summary,
        runtime_watch_refresh,
    )
    observed_comparability = runtime_refresh.get("comparability_verdict")
    verification_errors: list[str] = []
    if expected_comparability and str(observed_comparability) != expected_comparability:
        verification_errors.append(
            f"expected comparability {expected_comparability}, observed {observed_comparability}"
        )
    if expected_action and str(action) != expected_action:
        verification_errors.append(f"expected action {expected_action}, observed {action}")
    return {
        "name": name,
        "runtime_current_verdict": runtime_refresh.get("current_verdict"),
        "runtime_freshness_verdict": runtime_refresh.get("freshness_verdict"),
        "runtime_comparability_verdict": observed_comparability,
        "runtime_budget_verdict": runtime_refresh.get("overall_budget_verdict"),
        "selected_baseline_id": runtime_refresh.get("selected_baseline_id") or runtime_proposal.get("selected_baseline_id"),
        "selected_baseline_tag": runtime_refresh.get("selected_baseline_tag") or runtime_proposal.get("selected_baseline_tag"),
        "expected_comparability": expected_comparability,
        "expected_action": expected_action,
        "severity": severity,
        "recommended_action": action,
        "recommendation_text": recommendation,
        "rationale": rationale,
        "runtime_trend_summary": runtime_trend_summary,
        "runtime_watch_status": runtime_watch_refresh.get("overall_watch_status"),
        "runtime_watch_recommendation": runtime_watch_refresh.get("overall_watch_recommendation"),
        "runtime_watch_reason": runtime_watch_refresh.get("overall_watch_reason"),
        "runtime_watch_diagnostic_only": runtime_watch_refresh.get("diagnostic_watch_only"),
        "runtime_watch_sample_count": runtime_watch_refresh.get("runtime_watch_sample_count"),
        "verification_status": "PASS" if not verification_errors else "FAIL",
        "verification_errors": verification_errors,
    }


def mutate_runtime_current_manifest(
    base_manifest: dict[str, Any],
    fixture_name: str,
) -> dict[str, Any]:
    mutated = json.loads(json.dumps(base_manifest))
    host = mutated.setdefault("host_fingerprint", {})
    toolchain = mutated.setdefault("toolchain_fingerprint", {})
    if fixture_name == "same_host_compiler_bump":
        bumped = str(toolchain.get("compiler_version", "")) + " +phase25"
        toolchain["compiler_version"] = bumped
        host["compiler_version"] = bumped
    elif fixture_name == "sanitizer_change":
        for entry in mutated.get("entries", []):
            if str(entry.get("execution_class", "")) == "asan_full":
                entry["sanitizer_flags"] = "asan,ubsan,tsan"
    elif fixture_name == "runner_tag_change":
        host["runner_tag"] = str(host.get("runner_tag", "")) + "-alt"
    elif fixture_name == "cross_host":
        host["os"] = "Linux" if str(host.get("os", "")) != "Linux" else "Darwin"
        host["arch"] = "x86_64" if str(host.get("arch", "")) != "x86_64" else "arm64"
    return runtime_gate.normalize_runtime_current_manifest(mutated)


def mutate_runtime_registry_for_fixture(
    registry: dict[str, Any],
    fixture_name: str,
) -> dict[str, Any]:
    return runtime_gate.apply_runtime_registry_fixture(
        json.loads(json.dumps(registry)),
        fixture_name,
    )


def default_matrix_entries(
    runtime_current_manifest_path: Path,
    runtime_refresh_manifest_path: Path,
    runtime_rerun_plan_path: Path,
    runtime_proposal_path: Path | None,
) -> list[dict[str, Any]]:
    return [
        {
            "name": "exact_current",
            "runtime_current_manifest": str(runtime_current_manifest_path),
            "runtime_refresh_manifest": str(runtime_refresh_manifest_path),
            "runtime_rerun_plan": str(runtime_rerun_plan_path),
            "runtime_proposal": None if runtime_proposal_path is None else str(runtime_proposal_path),
            "expected_comparability": "COMPARABLE",
        },
        {
            "name": "same_host_compiler_bump",
            "fixture_name": "same_host_compiler_bump",
            "expected_comparability": "NOT_COMPARABLE",
            "expected_action": "NOT_COMPARABLE",
        },
        {
            "name": "sanitizer_change",
            "fixture_name": "sanitizer_change",
            "expected_comparability": "NOT_COMPARABLE",
            "expected_action": "NOT_COMPARABLE",
        },
        {
            "name": "runner_tag_change",
            "fixture_name": "runner_tag_change",
            "expected_comparability": "NOT_COMPARABLE",
            "expected_action": "NOT_COMPARABLE",
        },
        {
            "name": "cross_host",
            "fixture_name": "cross_host",
            "expected_comparability": "REBASELINE_REQUIRED",
            "expected_action": "REBASELINE_REQUIRED",
        },
        {
            "name": "retired_only",
            "fixture_name": "retired_only",
            "expected_comparability": "REBASELINE_REQUIRED",
            "expected_action": "REBASELINE_REQUIRED",
        },
    ]


def main() -> int:
    args = parse_args()
    repo_root = resolve_repo_root()
    artifact_root = Path(args.artifact_root).resolve()
    pipeline_phase = infer_pipeline_phase(args)
    baseline_manifest_path = manifest_json_path(args.baseline_manifest)
    current_manifest_path = manifest_json_path(args.current_manifest)
    refresh_manifest_path = manifest_json_path(args.refresh_manifest)
    rerun_plan_path = manifest_json_path(args.rerun_plan)
    runtime_baseline_manifest_path = manifest_json_path(
        args.runtime_baseline_manifest,
        default_runtime_path(artifact_root, pipeline_phase, "baseline"),
    )
    runtime_baseline_registry_path = manifest_json_path(
        args.runtime_baseline_registry,
        default_runtime_registry_path(artifact_root),
    )
    runtime_current_manifest_path = manifest_json_path(
        args.runtime_current_manifest,
        default_runtime_path(artifact_root, pipeline_phase, "current"),
    )
    runtime_refresh_manifest_path = manifest_json_path(
        args.runtime_refresh_manifest,
        default_runtime_path(artifact_root, pipeline_phase, "refresh"),
    )
    runtime_rerun_plan_path = manifest_json_path(
        args.runtime_rerun_plan,
        default_runtime_path(artifact_root, pipeline_phase, "rerun"),
    )
    runtime_history_index_path = manifest_json_path(
        args.runtime_history_index,
        default_runtime_history_index_path(artifact_root),
    )
    runtime_history_compact_path = manifest_json_path(
        args.runtime_history_compact,
        default_runtime_history_compact_path(artifact_root),
    )
    runtime_proposal_path = manifest_json_path(
        args.runtime_proposal,
        default_runtime_proposal_path(artifact_root, pipeline_phase),
    )
    runtime_comparability_triage_path = manifest_json_path(
        args.runtime_comparability_triage,
        default_runtime_comparability_triage_path(artifact_root, pipeline_phase),
    )
    runtime_rebaseline_action_plan_path = manifest_json_path(
        args.runtime_rebaseline_action_plan,
        default_runtime_rebaseline_action_plan_path(artifact_root, pipeline_phase),
    )
    runtime_rebaseline_proposal_path = manifest_json_path(
        args.runtime_rebaseline_proposal,
        default_runtime_rebaseline_proposal_path(artifact_root, pipeline_phase),
    )
    runtime_rebaseline_proposal_gate_path = manifest_json_path(
        args.runtime_rebaseline_proposal_gate,
        default_runtime_rebaseline_proposal_gate_path(artifact_root, pipeline_phase),
    )
    runtime_rebaseline_runbook_path = manifest_json_path(
        args.runtime_rebaseline_runbook,
        default_runtime_rebaseline_runbook_path(artifact_root, pipeline_phase),
    )
    runtime_registry_selection_sanity_path = manifest_json_path(
        args.runtime_registry_selection_sanity,
        default_runtime_registry_selection_sanity_path(artifact_root, pipeline_phase),
    )
    runtime_registry_selection_repair_plan_path = manifest_json_path(
        args.runtime_registry_selection_repair_plan,
        default_runtime_registry_selection_repair_plan_path(artifact_root, pipeline_phase),
    )
    runtime_watch_current_path = manifest_json_path(
        args.runtime_watch_current,
        default_runtime_watch_path(artifact_root, pipeline_phase, "current"),
    )
    runtime_watch_refresh_path = manifest_json_path(
        args.runtime_watch_refresh,
        default_runtime_watch_path(artifact_root, pipeline_phase, "refresh"),
    )
    runtime_watch_history_index_path = manifest_json_path(
        args.runtime_watch_history_index,
        default_runtime_watch_history_index_path(artifact_root),
    )
    runtime_watch_registry_path = manifest_json_path(
        args.runtime_watch_registry,
        default_runtime_watch_registry_path(artifact_root),
    )
    runtime_registry_health_path = manifest_json_path(
        args.runtime_registry_health,
        default_runtime_registry_health_path(artifact_root, pipeline_phase),
    )
    publication_health_path = manifest_json_path(
        args.publication_health,
        default_publication_health_path(artifact_root, pipeline_phase),
    )
    source_snapshot_manifest_path = manifest_json_path(
        args.source_snapshot_manifest,
        default_source_snapshot_manifest_path(artifact_root, pipeline_phase),
    )
    source_health_preflight_path = manifest_json_path(
        args.source_health_preflight,
        default_source_health_preflight_path(artifact_root, pipeline_phase),
    )
    source_health_action_plan_path = manifest_json_path(
        args.source_health_action_plan,
        default_source_health_action_plan_path(artifact_root, pipeline_phase),
    )
    verification_preflight_gate_path = manifest_json_path(
        args.verification_preflight_gate,
        default_verification_preflight_gate_path(artifact_root, pipeline_phase),
    )
    staged_overlay_completeness_path = manifest_json_path(
        args.staged_overlay_completeness,
        default_staged_overlay_completeness_path(artifact_root, pipeline_phase),
    )
    verification_preflight_gate_v2_path = manifest_json_path(
        args.verification_preflight_gate_v2,
        default_verification_preflight_gate_v2_path(artifact_root, pipeline_phase),
    )
    staged_materialization_path = manifest_json_path(
        args.staged_materialization,
        default_staged_materialization_path(artifact_root, pipeline_phase),
    )
    staged_mirror_manifest_path = manifest_json_path(
        args.staged_mirror_manifest,
        default_staged_mirror_manifest_path(artifact_root, pipeline_phase),
    )
    staged_mirror_verify_path = manifest_json_path(
        args.staged_mirror_verify,
        default_staged_mirror_verify_path(artifact_root, pipeline_phase),
    )
    ctest_inventory_release_path = manifest_json_path(
        args.ctest_inventory_release,
        default_ctest_inventory_path(artifact_root, pipeline_phase, "release"),
    )
    ctest_inventory_debug_path = manifest_json_path(
        args.ctest_inventory_debug,
        default_ctest_inventory_path(artifact_root, pipeline_phase, "debug"),
    )
    ctest_inventory_asan_path = manifest_json_path(
        args.ctest_inventory_asan,
        default_ctest_inventory_path(artifact_root, pipeline_phase, "asan"),
    )
    verification_release_path = manifest_json_path(
        args.verification_release,
        default_verification_path(artifact_root, pipeline_phase, "release"),
    )
    verification_debug_path = manifest_json_path(
        args.verification_debug,
        default_verification_path(artifact_root, pipeline_phase, "debug"),
    )
    verification_asan_path = manifest_json_path(
        args.verification_asan,
        default_verification_path(artifact_root, pipeline_phase, "asan"),
    )
    published_snapshot_manifest_path = manifest_json_path(
        args.published_snapshot_manifest,
        default_published_snapshot_manifest_path(artifact_root, pipeline_phase),
    )
    verification_closeout_path = manifest_json_path(
        args.verification_closeout,
        default_verification_closeout_path(artifact_root, pipeline_phase),
    )
    ops_summary_path = manifest_json_path(
        args.ops_summary,
        default_ops_summary_path(artifact_root, pipeline_phase),
    )
    approved_known_summary_paths = [manifest_json_path(value) for value in list(args.approved_known_summary or [])]
    foreign_import_summary_paths = [manifest_json_path(value) for value in list(args.foreign_import_summary or [])]
    summary_path = (
        Path(args.summary_out).resolve()
        if args.summary_out
        else (default_matrix_summary_path(artifact_root, pipeline_phase) if args.mode == "matrix" else default_summary_path(artifact_root, pipeline_phase, args.mode))
    )
    report_path = Path(args.report_out).resolve() if args.report_out else default_report_path(repo_root, pipeline_phase)
    zip_out = Path(args.zip_out).resolve() if args.zip_out else default_zip_path(repo_root, pipeline_phase, False)
    curated_zip = Path(args.curated_zip).resolve() if args.curated_zip else default_zip_path(repo_root, pipeline_phase, True)
    light_ops_zip = Path(args.light_ops_zip).resolve() if args.light_ops_zip else default_light_ops_zip_path(repo_root, pipeline_phase)
    python_bin = resolve_python_bin(args.python_bin)
    raw_engine_tests = resolve_raw_engine_tests(repo_root, args.raw_engine_tests)
    runtime_gate_script = repo_root / "tests" / "tools" / "runtime_gate.py"
    watch_ops_script = repo_root / "tests" / "tools" / "runtime_watch_ops.py"
    bundle_script = repo_root / "tests" / "tools" / "build_evidence_bundle.py"
    stages: list[dict[str, Any]] = []
    bundle_metadata_path: Path | None = None

    try:
        if not baseline_manifest_path.exists():
            raise FileNotFoundError(f"baseline manifest not found: {baseline_manifest_path}")

        synthetic_flags: list[str] = []
        if args.synthetic_hash_drift:
            synthetic_flags.extend(["--synthetic-hash-drift", args.synthetic_hash_drift])
        if args.synthetic_applicability_drift:
            synthetic_flags.extend(["--synthetic-applicability-drift", args.synthetic_applicability_drift])
        if args.synthetic_diagnostic_promotion:
            synthetic_flags.extend(["--synthetic-diagnostic-promotion", args.synthetic_diagnostic_promotion])

        if args.include_source_health:
            source_health_command = [
                python_bin,
                str(repo_root / "tests" / "tools" / "source_health_preflight.py"),
                "--phase",
                pipeline_phase,
                "--source-root",
                str(Path(args.source_root).resolve() if args.source_root else repo_root),
                "--required-path",
                "CMakeLists.txt,README.md,tests/raw_engine_cases.cpp,tests/tools/runtime_watch_ops.py",
                "--out",
                str(source_health_preflight_path),
                "--summary-out",
                str(source_health_preflight_path.with_suffix(".summary.txt")),
                "--staged-materialization-out",
                str(staged_materialization_path),
            ]
            if int(args.source_health_simulate_dataless_count or 0) > 0:
                source_health_command.extend(["--simulate-dataless-count", str(args.source_health_simulate_dataless_count)])
            if args.source_health_simulate_missing_git_object:
                source_health_command.append("--simulate-missing-git-object")
            stages.append(run_command("source_health_preflight", source_health_command, repo_root))
            source_health_plan_command = [
                python_bin,
                str(watch_ops_script),
                "source-health-plan",
                "--phase",
                pipeline_phase,
                "--source-health",
                str(source_health_preflight_path),
                "--staged-materialization",
                str(staged_materialization_path),
                "--plan-out",
                str(source_health_action_plan_path),
            ]
            stages.append(run_command("source_health_action_plan", source_health_plan_command, repo_root))
            verification_preflight_command = [
                python_bin,
                str(watch_ops_script),
                "verification-preflight-gate",
                "--phase",
                pipeline_phase,
                "--source-health",
                str(source_health_preflight_path),
                "--staged-materialization",
                str(staged_materialization_path),
                "--preflight-gate-out",
                str(verification_preflight_gate_path),
            ]
            stages.append(run_command("verification_preflight_gate", verification_preflight_command, repo_root))
            overlay_command = [
                python_bin,
                str(watch_ops_script),
                "staged-overlay-completeness-manifest",
                "--phase",
                pipeline_phase,
                "--source-root",
                str(Path(args.source_root).resolve() if args.source_root else repo_root),
                "--snapshot-manifest",
                str(source_snapshot_manifest_path),
                "--out",
                str(staged_overlay_completeness_path),
            ]
            if args.overlay_policy:
                overlay_command.extend(["--overlay-policy", str(Path(args.overlay_policy).resolve())])
            stages.append(run_command("staged_overlay_completeness", overlay_command, repo_root))
            verification_preflight_v2_command = [
                python_bin,
                str(watch_ops_script),
                "verification-preflight-gate-v2",
                "--phase",
                pipeline_phase,
                "--source-health",
                str(source_health_preflight_path),
                "--overlay-completeness",
                str(staged_overlay_completeness_path),
                "--preflight-gate-out",
                str(verification_preflight_gate_v2_path),
            ]
            stages.append(run_command("verification_preflight_gate_v2", verification_preflight_v2_command, repo_root))

        if args.mode in {"quick", "rebaseline_candidate", "matrix"}:
            policy_ci_command = [
                str(raw_engine_tests),
                "--case",
                "policy_ci_check",
                "--baseline-manifest",
                str(baseline_manifest_path),
                "--current-manifest",
                str(current_manifest_path),
                "--refresh-manifest",
                str(refresh_manifest_path),
                "--rerun-plan",
                str(rerun_plan_path),
                "--artifact-dir",
                str(artifact_root),
                "--emit-summary",
                str(summary_path.with_suffix(".txt")),
                *synthetic_flags,
                *(["--allow-empty-plan"] if args.allow_empty_plan else []),
            ]
            if args.skip_policy_stage:
                stages.append(
                    skipped_stage(
                        "policy_ci_check",
                        policy_ci_command,
                        "policy stage skipped; pipeline summary was rebuilt from existing manifests",
                    )
                )
            else:
                stages.append(run_command("policy_ci_check", policy_ci_command, repo_root))
        elif args.mode in {"nightly", "full_local"}:
            nightly_command = [
                str(raw_engine_tests),
                "--case",
                "policy_nightly_refresh",
                "--baseline-manifest",
                str(baseline_manifest_path),
                "--current-manifest",
                str(current_manifest_path),
                "--refresh-manifest",
                str(refresh_manifest_path),
                "--rerun-plan",
                str(rerun_plan_path),
                "--artifact-dir",
                str(artifact_root),
                "--emit-summary",
                str(summary_path.with_suffix(".txt")),
                *synthetic_flags,
                *(["--allow-empty-plan"] if args.allow_empty_plan else []),
            ]
            if args.skip_policy_stage:
                stages.append(
                    skipped_stage(
                        "policy_nightly_refresh",
                        nightly_command,
                        "policy nightly stage skipped; pipeline summary was rebuilt from existing manifests",
                    )
                )
            else:
                stages.append(run_command("policy_nightly_refresh", nightly_command, repo_root))

        if args.mode == "matrix":
            matrix_config_path = Path(args.matrix_config).resolve() if args.matrix_config else None
            matrix_config = load_json_if_exists(matrix_config_path)
            entries = list(matrix_config.get("entries", []))
            if not entries:
                entries = default_matrix_entries(
                    runtime_current_manifest_path,
                    runtime_refresh_manifest_path,
                    runtime_rerun_plan_path,
                    runtime_proposal_path,
                )
            base_runtime_current = load_json_if_exists(runtime_current_manifest_path)
            base_runtime_registry = load_json_if_exists(runtime_baseline_registry_path)
            history_summary = load_runtime_history_summary(runtime_history_index_path)
            matrix_entries: list[dict[str, Any]] = []
            for entry in entries:
                name = str(entry.get("name", f"matrix_{len(matrix_entries)}"))
                derived_paths = runtime_matrix_entry_paths(artifact_root, name)
                fixture_name = str(entry.get("fixture_name", "")).strip()
                explicit_current_manifest = str(entry.get("runtime_current_manifest", "")).strip()
                current_path = (
                    manifest_json_path(explicit_current_manifest)
                    if explicit_current_manifest
                    else runtime_current_manifest_path
                )
                refresh_path = manifest_json_path(str(entry.get("runtime_refresh_manifest", derived_paths["refresh"])))
                rerun_path = manifest_json_path(str(entry.get("runtime_rerun_plan", derived_paths["rerun"])))
                proposal_path = manifest_json_path(str(entry.get("runtime_proposal", derived_paths["proposal"]))) if runtime_proposal_path is not None else None
                registry_path_for_entry = runtime_baseline_registry_path
                if fixture_name:
                    if not explicit_current_manifest:
                        current_path = derived_paths["refresh"].with_name(f"{name}_runtime_current.json")
                        write_json(current_path, mutate_runtime_current_manifest(base_runtime_current, fixture_name))
                    if runtime_baseline_registry_path is not None:
                        registry_path_for_entry = derived_paths["refresh"].with_name(f"{name}_runtime_registry.json")
                        write_json(registry_path_for_entry, mutate_runtime_registry_for_fixture(base_runtime_registry, fixture_name))
                refresh_cmd = [
                    python_bin,
                    str(runtime_gate_script),
                    "refresh",
                    "--runtime-current-manifest",
                    str(current_path),
                    "--runtime-refresh-manifest",
                    str(refresh_path),
                ]
                if registry_path_for_entry is not None:
                    refresh_cmd.extend(["--runtime-baseline-registry", str(registry_path_for_entry)])
                else:
                    refresh_cmd.extend(["--runtime-baseline-manifest", str(runtime_baseline_manifest_path)])
                for value in entry.get("runtime_synthetic_inflate", []):
                    refresh_cmd.extend(["--synthetic-inflate", str(value)])
                if entry.get("runtime_synthetic_fingerprint_mismatch"):
                    refresh_cmd.extend(["--synthetic-fingerprint-mismatch", str(entry.get("runtime_synthetic_fingerprint_mismatch"))])
                stages.append(run_command(f"matrix_refresh_{name}", refresh_cmd, repo_root))
                entry_runtime_baseline_manifest_path = effective_runtime_baseline_manifest_path(
                    runtime_baseline_manifest_path,
                    refresh_path,
                    registry_path_for_entry,
                )
                stages.append(
                    run_command(
                        f"matrix_plan_{name}",
                        [
                            python_bin,
                            str(runtime_gate_script),
                            "plan-rerun",
                            "--runtime-current-manifest",
                            str(current_path),
                            "--runtime-refresh-manifest",
                            str(refresh_path),
                            "--runtime-rerun-plan",
                            str(rerun_path),
                        ],
                        repo_root,
                    )
                )
                proposal_data: dict[str, Any] = {}
                if proposal_path is not None and registry_path_for_entry is not None:
                    proposal_cmd = [
                        python_bin,
                        str(runtime_gate_script),
                        "propose-rebaseline",
                        "--runtime-current-manifest",
                        str(current_path),
                        "--runtime-refresh-manifest",
                        str(refresh_path),
                        "--runtime-baseline-registry",
                        str(registry_path_for_entry),
                        "--proposal-out",
                        str(proposal_path),
                    ]
                    if runtime_history_index_path is not None:
                        proposal_cmd.extend(["--runtime-history-index", str(runtime_history_index_path)])
                    stages.append(
                        run_command(
                            f"matrix_proposal_{name}",
                            proposal_cmd,
                            repo_root,
                        )
                    )
                    proposal_data = load_json_if_exists(proposal_path)
                watch_current_path = derived_paths["watch_current"]
                watch_refresh_path = derived_paths["watch_refresh"]
                watch_history_path = derived_paths["watch_history"]
                watch_campaign_cmd = [
                    python_bin,
                    str(runtime_gate_script),
                    "watch-campaign",
                    "--runtime-baseline-manifest",
                    str(entry_runtime_baseline_manifest_path),
                    "--runtime-current-manifest",
                    str(current_path),
                    "--runtime-refresh-manifest",
                    str(refresh_path),
                    "--runtime-watch-current",
                    str(watch_current_path),
                    "--runtime-watch-history-index",
                    str(watch_history_path),
                    "--runtime-history-index",
                    str(runtime_history_index_path),
                    "--execution-class",
                    "all",
                    "--repeat",
                    "3",
                ]
                if args.runtime_budget_config:
                    watch_campaign_cmd.extend(["--runtime-budget-config", str(Path(args.runtime_budget_config).resolve())])
                stages.append(run_command(f"matrix_watch_campaign_{name}", watch_campaign_cmd, repo_root))
                stages.append(
                    run_command(
                        f"matrix_watch_refresh_{name}",
                        [
                            python_bin,
                            str(runtime_gate_script),
                            "watch-refresh",
                            "--runtime-baseline-manifest",
                            str(entry_runtime_baseline_manifest_path),
                            "--runtime-current-manifest",
                            str(current_path),
                            "--runtime-refresh-manifest",
                            str(refresh_path),
                            "--runtime-watch-current",
                            str(watch_current_path),
                            "--runtime-watch-refresh",
                            str(watch_refresh_path),
                            "--runtime-watch-history-index",
                            str(watch_history_path),
                        ],
                        repo_root,
                    )
                )
                matrix_entries.append(
                    build_matrix_entry_summary(
                        name,
                        load_json_if_exists(refresh_path),
                        load_json_if_exists(rerun_path),
                        proposal_data,
                        load_json_if_exists(watch_refresh_path),
                        build_runtime_trend_summary(history_summary, load_json_if_exists(current_path)),
                        str(entry.get("expected_comparability", "")) or None,
                        str(entry.get("expected_action", "")) or None,
                    )
                )

            effective_runtime_baseline_manifest_path_for_matrix = effective_runtime_baseline_manifest_path(
                runtime_baseline_manifest_path,
                runtime_refresh_manifest_path,
                runtime_baseline_registry_path,
            )
            base_summary = build_summary(
                "quick",
                pipeline_phase,
                artifact_root,
                baseline_manifest_path,
                current_manifest_path,
                refresh_manifest_path,
                rerun_plan_path,
                effective_runtime_baseline_manifest_path_for_matrix,
                runtime_baseline_registry_path,
                runtime_current_manifest_path,
                runtime_refresh_manifest_path,
                runtime_rerun_plan_path,
                runtime_history_index_path,
                runtime_proposal_path,
                runtime_watch_current_path,
                runtime_watch_refresh_path,
                runtime_watch_history_index_path,
                bundle_metadata_path,
                stages,
            )
            matrix_summary = dict(base_summary)
            matrix_summary["mode"] = "matrix"
            matrix_summary["matrix_entries"] = matrix_entries
            matrix_summary["matrix_entry_count"] = len(matrix_entries)
            failed_entries = [item for item in matrix_entries if item.get("verification_status") != "PASS"]
            action_counts: dict[str, int] = {}
            severity_counts: dict[str, int] = {}
            watch_status_counts: dict[str, int] = {}
            watch_recommendation_counts: dict[str, int] = {}
            for item in matrix_entries:
                action = str(item.get("recommended_action", ""))
                if action:
                    action_counts[action] = action_counts.get(action, 0) + 1
                severity_name = str(item.get("severity", ""))
                if severity_name:
                    severity_counts[severity_name] = severity_counts.get(severity_name, 0) + 1
                watch_status = str(item.get("runtime_watch_status", ""))
                if watch_status:
                    watch_status_counts[watch_status] = watch_status_counts.get(watch_status, 0) + 1
                watch_recommendation = str(item.get("runtime_watch_recommendation", ""))
                if watch_recommendation:
                    watch_recommendation_counts[watch_recommendation] = watch_recommendation_counts.get(watch_recommendation, 0) + 1
            worst_entry = None if not matrix_entries else max(
                matrix_entries,
                key=lambda item: SEVERITY_RANK.get(str(item.get("severity", "OK")), 0),
            )
            matrix_summary["matrix_fail_count"] = len(failed_entries)
            matrix_summary["matrix_pass_count"] = len(matrix_entries) - len(failed_entries)
            matrix_summary["matrix_verification_status"] = "PASS" if not failed_entries else "FAIL"
            matrix_summary["matrix_action_counts"] = action_counts
            matrix_summary["matrix_severity_counts"] = severity_counts
            matrix_summary["matrix_watch_status_counts"] = watch_status_counts
            matrix_summary["matrix_watch_recommendation_counts"] = watch_recommendation_counts
            matrix_summary["matrix_worst_entry"] = worst_entry
            matrix_summary["runtime_watch_fingerprint_count"] = len(matrix_entries)
            matrix_summary["runtime_watch_multi_fingerprint_summary"] = {
                "matrix_entry_count": len(matrix_entries),
                "matrix_watch_status_counts": watch_status_counts,
                "matrix_watch_recommendation_counts": watch_recommendation_counts,
                "matrix_action_counts": action_counts,
                "matrix_severity_counts": severity_counts,
            }
            if worst_entry is not None:
                matrix_summary["runtime_watch_status"] = worst_entry.get("runtime_watch_status")
                matrix_summary["runtime_watch_recommendation"] = worst_entry.get("runtime_watch_recommendation")
                matrix_summary["runtime_watch_reason"] = worst_entry.get("runtime_watch_reason")
            if failed_entries:
                matrix_summary["severity"] = "FAIL"
                matrix_summary["exit_code"] = EXIT_FAIL
                matrix_summary["runtime_recommendation"] = "FAIL"
                matrix_summary["recommended_next_action"] = "FAIL"
                matrix_summary["rationale_list"] = failed_entries[0].get("verification_errors", [])
            elif worst_entry is not None and SEVERITY_RANK.get(str(worst_entry.get("severity", "OK")), 0) > SEVERITY_RANK.get(str(base_summary.get("severity", "OK")), 0):
                matrix_summary["severity"] = str(worst_entry.get("severity", "OK"))
                matrix_summary["exit_code"] = {
                    "OK": EXIT_OK,
                    "WARN": EXIT_WARN,
                    "ACTION_REQUIRED": EXIT_ACTION_REQUIRED,
                    "FAIL": EXIT_FAIL,
                }[matrix_summary["severity"]]
                matrix_summary["runtime_recommendation"] = str(worst_entry.get("recommended_action", "NO_ACTION"))
                matrix_summary["recommended_next_action"] = str(worst_entry.get("recommendation_text", "NO_ACTION"))
                matrix_summary["rationale_list"] = [
                    f"matrix worst entry={worst_entry.get('name', '')}",
                    *list(worst_entry.get("rationale", [])),
                ]
            else:
                matrix_summary["severity"] = str(base_summary.get("severity", "OK"))
                matrix_summary["exit_code"] = int(base_summary.get("exit_code", EXIT_OK))
                matrix_summary["runtime_recommendation"] = str(base_summary.get("runtime_recommendation", "NO_ACTION"))
                matrix_summary["recommended_next_action"] = str(base_summary.get("recommended_next_action", "NO_ACTION"))
                matrix_summary["rationale_list"] = [
                    "runtime matrix fixtures matched the expected comparability and action outcomes",
                    *list(base_summary.get("rationale_list", [])),
                ]
            write_pipeline_summary(summary_path, matrix_summary)
            print(pipeline_summary_text(matrix_summary), end="")
            return int(matrix_summary["exit_code"])

        effective_runtime_baseline_manifest_path_for_pipeline = runtime_baseline_manifest_path
        if args.mode != "bundle_only" and not args.skip_runtime_stage:
            existing_runtime_current = load_json_if_exists(runtime_current_manifest_path)
            runtime_stage_overrides: dict[str, float] = {}
            for stage in stages:
                if stage["name"] == "policy_ci_check":
                    runtime_stage_overrides["policy_core"] = float(stage["duration_seconds"])
                    runtime_stage_overrides["policy_refresh"] = float(stage["duration_seconds"])
                elif stage["name"] == "policy_nightly_refresh":
                    runtime_stage_overrides["policy_nightly"] = float(stage["duration_seconds"])
            for value in args.runtime_stage:
                name, seconds = parse_runtime_stage(value)
                runtime_stage_overrides[name] = seconds
            if not runtime_stage_overrides and not existing_runtime_current:
                runtime_stage_overrides["policy_core"] = 0.001
            runtime_entries = merge_runtime_entries(existing_runtime_current, runtime_stage_overrides)
            runtime_runner_tag = args.runtime_runner_tag or str(
                existing_runtime_current.get("host_fingerprint", {}).get("runner_tag", "")
            )
            baseline_tag = str(existing_runtime_current.get("baseline_tag", ""))

            runtime_current_cmd = [
                python_bin,
                str(runtime_gate_script),
                "write-current",
                "--phase",
                pipeline_phase,
                "--artifact-root",
                str(artifact_root),
                "--runtime-current-manifest",
                str(runtime_current_manifest_path),
                "--runner-tag",
                runtime_runner_tag,
                *sum([["--entry", runtime_entry_spec(entry)] for entry in runtime_entries], []),
            ]
            if baseline_tag:
                runtime_current_cmd.extend(["--baseline-tag", baseline_tag])
            if args.runtime_budget_config:
                runtime_current_cmd.extend(["--runtime-budget-config", args.runtime_budget_config])
            stages.append(run_command("runtime_write_current", runtime_current_cmd, repo_root))

            runtime_refresh_cmd = [
                python_bin,
                str(runtime_gate_script),
                "refresh",
                "--runtime-current-manifest",
                str(runtime_current_manifest_path),
                "--runtime-refresh-manifest",
                str(runtime_refresh_manifest_path),
                *sum([["--synthetic-inflate", value] for value in args.runtime_synthetic_inflate], []),
            ]
            if runtime_baseline_registry_path is not None:
                runtime_refresh_cmd.extend(["--runtime-baseline-registry", str(runtime_baseline_registry_path)])
            else:
                runtime_refresh_cmd.extend(["--runtime-baseline-manifest", str(runtime_baseline_manifest_path)])
            if args.synthetic_runtime_fixture:
                runtime_refresh_cmd.extend(["--synthetic-runtime-fixture", args.synthetic_runtime_fixture])
            if args.runtime_synthetic_fingerprint_mismatch:
                runtime_refresh_cmd.extend(["--synthetic-fingerprint-mismatch", args.runtime_synthetic_fingerprint_mismatch])
            stages.append(run_command("runtime_gate_refresh", runtime_refresh_cmd, repo_root))
            effective_runtime_baseline_manifest_path_for_pipeline = effective_runtime_baseline_manifest_path(
                runtime_baseline_manifest_path,
                runtime_refresh_manifest_path,
                runtime_baseline_registry_path,
            )

            stages.append(
                run_command(
                    "runtime_gate_plan_rerun",
                    [
                        python_bin,
                        str(runtime_gate_script),
                        "plan-rerun",
                        "--runtime-current-manifest",
                        str(runtime_current_manifest_path),
                        "--runtime-refresh-manifest",
                        str(runtime_refresh_manifest_path),
                        "--runtime-rerun-plan",
                        str(runtime_rerun_plan_path),
                    ],
                    repo_root,
                )
            )
            if runtime_proposal_path is not None and runtime_baseline_registry_path is not None:
                stages.append(
                    run_command(
                        "runtime_propose_rebaseline",
                        [
                            python_bin,
                            str(runtime_gate_script),
                            "propose-rebaseline",
                            "--runtime-current-manifest",
                            str(runtime_current_manifest_path),
                            "--runtime-refresh-manifest",
                            str(runtime_refresh_manifest_path),
                        "--runtime-baseline-registry",
                        str(runtime_baseline_registry_path),
                            "--proposal-out",
                            str(runtime_proposal_path),
                            *(["--synthetic-runtime-fixture", args.synthetic_runtime_fixture] if args.synthetic_runtime_fixture else []),
                        ],
                        repo_root,
                    )
                )
            if args.mode in {"nightly", "full_local"} and runtime_history_index_path is not None:
                stages.append(
                    run_command(
                        "runtime_history_append",
                        [
                            python_bin,
                            str(runtime_gate_script),
                            "history-append",
                            "--runtime-history-index",
                            str(runtime_history_index_path),
                            "--runtime-current-manifest",
                            str(runtime_current_manifest_path),
                            "--runtime-refresh-manifest",
                            str(runtime_refresh_manifest_path),
                        ],
                        repo_root,
                    )
                )

            stages.append(
                run_command(
                    "runtime_watch_campaign",
                    [
                        python_bin,
                        str(runtime_gate_script),
                        "watch-campaign",
                        "--runtime-baseline-manifest",
                        str(effective_runtime_baseline_manifest_path_for_pipeline),
                        "--runtime-current-manifest",
                        str(runtime_current_manifest_path),
                        "--runtime-refresh-manifest",
                        str(runtime_refresh_manifest_path),
                        "--runtime-watch-current",
                        str(runtime_watch_current_path),
                        "--runtime-watch-history-index",
                        str(runtime_watch_history_index_path),
                        "--runtime-history-index",
                        str(runtime_history_index_path),
                        "--execution-class",
                        "all",
                        "--repeat",
                        "5",
                        *(["--runtime-budget-config", args.runtime_budget_config] if args.runtime_budget_config else []),
                    ],
                    repo_root,
                )
            )
            stages.append(
                run_command(
                    "runtime_watch_refresh",
                    [
                        python_bin,
                        str(runtime_gate_script),
                        "watch-refresh",
                        "--runtime-baseline-manifest",
                        str(effective_runtime_baseline_manifest_path_for_pipeline),
                        "--runtime-current-manifest",
                        str(runtime_current_manifest_path),
                        "--runtime-refresh-manifest",
                        str(runtime_refresh_manifest_path),
                        "--runtime-watch-current",
                        str(runtime_watch_current_path),
                        "--runtime-watch-refresh",
                        str(runtime_watch_refresh_path),
                        "--runtime-watch-history-index",
                        str(runtime_watch_history_index_path),
                    ],
                    repo_root,
                )
            )

        elif args.skip_runtime_stage:
            stages.append(
                skipped_stage(
                    "runtime_stage_refresh",
                    [],
                    "runtime stage skipped; pipeline summary reused the supplied runtime current/refresh/rerun/watch artifacts",
                )
            )

        if runtime_watch_registry_path is not None and (
            (runtime_watch_current_path is not None and runtime_watch_current_path.exists())
            or (runtime_watch_refresh_path is not None and runtime_watch_refresh_path.exists())
            or (runtime_watch_history_index_path is not None and runtime_watch_history_index_path.exists())
        ):
            stages.append(
                run_command(
                    "runtime_watch_registry_preflight",
                    [
                        python_bin,
                        str(watch_ops_script),
                        "watch-registry",
                        *(["--watch-current", str(runtime_watch_current_path)] if runtime_watch_current_path is not None else []),
                        *(["--watch-refresh", str(runtime_watch_refresh_path)] if runtime_watch_refresh_path is not None else []),
                        *(["--watch-history-index", str(runtime_watch_history_index_path)] if runtime_watch_history_index_path is not None else []),
                        "--matrix-summary",
                        str(default_matrix_summary_path(artifact_root, pipeline_phase)),
                        "--matrix-root",
                        str(artifact_root / "matrix"),
                        "--registry-out",
                        str(runtime_watch_registry_path),
                    ],
                    repo_root,
                )
            )

        if runtime_baseline_registry_path is not None and runtime_current_manifest_path.exists():
            stages.append(
                run_command(
                    "runtime_comparability_triage",
                    [
                        python_bin,
                        str(watch_ops_script),
                        "runtime-comparability-triage",
                        "--phase",
                        pipeline_phase,
                        "--runtime-current-manifest",
                        str(runtime_current_manifest_path),
                        "--runtime-baseline-registry",
                        str(runtime_baseline_registry_path),
                        *(["--runtime-history-index", str(runtime_history_index_path)] if runtime_history_index_path is not None and runtime_history_index_path.exists() else []),
                        *(["--runtime-watch-registry", str(runtime_watch_registry_path)] if runtime_watch_registry_path.exists() else []),
                        "--triage-out",
                        str(runtime_comparability_triage_path),
                    ],
                    repo_root,
                )
            )
            stages.append(
                run_command(
                    "runtime_rebaseline_action_plan",
                    [
                        python_bin,
                        str(watch_ops_script),
                        "runtime-rebaseline-action-plan",
                        "--phase",
                        pipeline_phase,
                        "--runtime-comparability-triage",
                        str(runtime_comparability_triage_path),
                        "--runtime-current-manifest",
                        str(runtime_current_manifest_path),
                        "--runtime-baseline-registry",
                        str(runtime_baseline_registry_path),
                        *(["--runtime-history-index", str(runtime_history_index_path)] if runtime_history_index_path is not None and runtime_history_index_path.exists() else []),
                        *(["--runtime-watch-registry", str(runtime_watch_registry_path)] if runtime_watch_registry_path.exists() else []),
                        "--proposal-out",
                        str(runtime_rebaseline_proposal_path),
                        "--proposal-gate-out",
                        str(runtime_rebaseline_proposal_gate_path),
                        "--action-plan-out",
                        str(runtime_rebaseline_action_plan_path),
                    ],
                    repo_root,
                )
            )
            stages.append(
                run_command(
                    "runtime_rebaseline_runbook",
                    [
                        python_bin,
                        str(watch_ops_script),
                        "runtime-rebaseline-runbook",
                        "--phase",
                        pipeline_phase,
                        "--runtime-rebaseline-action-plan",
                        str(runtime_rebaseline_action_plan_path),
                        "--runtime-proposal",
                        str(runtime_rebaseline_proposal_path),
                        "--runtime-proposal-gate",
                        str(runtime_rebaseline_proposal_gate_path),
                        "--runtime-baseline-registry",
                        str(runtime_baseline_registry_path),
                        "--runbook-out",
                        str(runtime_rebaseline_runbook_path),
                    ],
                    repo_root,
                )
            )
            stages.append(
                run_command(
                    "runtime_registry_selection_sanity",
                    [
                        python_bin,
                        str(watch_ops_script),
                        "runtime-registry-selection-sanity",
                        "--phase",
                        pipeline_phase,
                        "--runtime-current-manifest",
                        str(runtime_current_manifest_path),
                        "--runtime-baseline-registry",
                        str(runtime_baseline_registry_path),
                        "--sanity-out",
                        str(runtime_registry_selection_sanity_path),
                    ],
                    repo_root,
                )
            )
            stages.append(
                run_command(
                    "runtime_registry_selection_repair_plan",
                    [
                        python_bin,
                        str(watch_ops_script),
                        "runtime-registry-selection-repair-plan",
                        "--phase",
                        pipeline_phase,
                        "--selection-sanity",
                        str(runtime_registry_selection_sanity_path),
                        "--runtime-baseline-registry",
                        str(runtime_baseline_registry_path),
                        "--repair-plan-out",
                        str(runtime_registry_selection_repair_plan_path),
                    ],
                    repo_root,
                )
            )

        summary = build_summary(
            args.mode,
            pipeline_phase,
            artifact_root,
            baseline_manifest_path,
            current_manifest_path,
            refresh_manifest_path,
            rerun_plan_path,
            effective_runtime_baseline_manifest_path_for_pipeline,
            runtime_baseline_registry_path,
            runtime_current_manifest_path,
            runtime_refresh_manifest_path,
            runtime_rerun_plan_path,
            runtime_history_index_path,
            runtime_proposal_path,
            runtime_watch_current_path,
            runtime_watch_refresh_path,
            runtime_watch_history_index_path,
            bundle_metadata_path,
            stages,
        )
        summary = enrich_summary_with_operator_artifacts(
            summary,
            runtime_watch_registry_path,
            ops_summary_path,
            runtime_registry_health_path,
            publication_health_path if publication_health_path.exists() else None,
            source_health_preflight_path if source_health_preflight_path.exists() else None,
            source_health_action_plan_path if source_health_action_plan_path.exists() else None,
            verification_preflight_gate_path if verification_preflight_gate_path.exists() else None,
            staged_overlay_completeness_path if staged_overlay_completeness_path.exists() else None,
            verification_preflight_gate_v2_path if verification_preflight_gate_v2_path.exists() else None,
            staged_materialization_path if staged_materialization_path.exists() else None,
            runtime_comparability_triage_path if runtime_comparability_triage_path.exists() else None,
            runtime_rebaseline_action_plan_path if runtime_rebaseline_action_plan_path.exists() else None,
            runtime_rebaseline_runbook_path if runtime_rebaseline_runbook_path.exists() else None,
            runtime_registry_selection_sanity_path if runtime_registry_selection_sanity_path.exists() else None,
            runtime_registry_selection_repair_plan_path if runtime_registry_selection_repair_plan_path.exists() else None,
        )
        write_pipeline_summary(summary_path, summary)

        stages.extend(
            materialize_operator_artifacts(
                python_bin=python_bin,
                watch_ops_script=watch_ops_script,
                repo_root=repo_root,
                artifact_root=artifact_root,
                phase=pipeline_phase,
                policy_manifest_path=current_manifest_path,
                runtime_refresh_manifest_path=runtime_refresh_manifest_path,
                runtime_history_index_path=runtime_history_index_path,
                runtime_watch_current_path=runtime_watch_current_path,
                runtime_watch_refresh_path=runtime_watch_refresh_path,
                runtime_watch_history_index_path=runtime_watch_history_index_path,
                runtime_watch_registry_path=runtime_watch_registry_path,
                ops_summary_path=ops_summary_path,
                runtime_baseline_registry_path=runtime_baseline_registry_path,
                runtime_registry_health_path=runtime_registry_health_path,
                publication_health_path=publication_health_path,
                source_snapshot_manifest_path=source_snapshot_manifest_path,
                source_health_preflight_path=source_health_preflight_path,
                staged_overlay_completeness_path=staged_overlay_completeness_path,
                verification_preflight_gate_v2_path=verification_preflight_gate_v2_path,
                staged_materialization_path=staged_materialization_path,
                staged_mirror_verify_path=staged_mirror_verify_path,
                verification_release_path=verification_release_path,
                verification_debug_path=verification_debug_path,
                verification_asan_path=verification_asan_path,
                published_snapshot_manifest_path=published_snapshot_manifest_path,
                verification_closeout_path=verification_closeout_path,
                approved_known_summary_paths=approved_known_summary_paths,
                foreign_import_summary_paths=foreign_import_summary_paths,
                current_env_governance_policy_path=Path(args.current_env_governance_policy).resolve() if args.current_env_governance_policy else None,
                current_env_guardrail_policy_path=Path(args.current_env_guardrail_policy).resolve() if args.current_env_guardrail_policy else None,
                current_env_watch_current_path_manifest=Path(args.current_env_watch_current).resolve() if args.current_env_watch_current else None,
                current_env_watch_refresh_path_manifest=Path(args.current_env_watch_refresh).resolve() if args.current_env_watch_refresh else None,
                current_env_watch_history_path_manifest=Path(args.current_env_watch_history).resolve() if args.current_env_watch_history else None,
                current_env_age_tick_path=Path(args.current_env_age_tick).resolve() if args.current_env_age_tick else None,
                current_env_watch_plan_path=Path(args.current_env_watch_plan).resolve() if args.current_env_watch_plan else None,
                current_env_trigger_gate_path=Path(args.current_env_trigger_gate).resolve() if args.current_env_trigger_gate else None,
                runtime_budget_current_path_manifest=Path(args.runtime_budget_current).resolve() if args.runtime_budget_current else None,
                runtime_budget_proposal_path_manifest=Path(args.runtime_budget_proposal).resolve() if args.runtime_budget_proposal else None,
                runtime_budget_proposal_gate_path_manifest=Path(args.runtime_budget_proposal_gate).resolve() if args.runtime_budget_proposal_gate else None,
                runtime_budget_baseline_path_manifest=Path(args.runtime_budget_baseline).resolve() if args.runtime_budget_baseline else None,
                runtime_budget_refresh_path_manifest=Path(args.runtime_budget_refresh).resolve() if args.runtime_budget_refresh else None,
                runtime_budget_reproposal_history_path=Path(args.runtime_budget_reproposal_history).resolve() if args.runtime_budget_reproposal_history else None,
                runtime_budget_registry_summary_path=Path(args.runtime_budget_registry_summary).resolve() if args.runtime_budget_registry_summary else None,
                runtime_comparability_triage_path=runtime_comparability_triage_path if runtime_comparability_triage_path.exists() else None,
                runtime_rebaseline_action_plan_path=runtime_rebaseline_action_plan_path if runtime_rebaseline_action_plan_path.exists() else None,
                runtime_rebaseline_proposal_path=runtime_rebaseline_proposal_path if runtime_rebaseline_proposal_path.exists() else None,
                runtime_rebaseline_proposal_gate_path=runtime_rebaseline_proposal_gate_path if runtime_rebaseline_proposal_gate_path.exists() else None,
                runtime_rebaseline_runbook_path=runtime_rebaseline_runbook_path if runtime_rebaseline_runbook_path.exists() else None,
                runtime_registry_selection_sanity_path=runtime_registry_selection_sanity_path if runtime_registry_selection_sanity_path.exists() else None,
                runtime_registry_selection_repair_plan_path=runtime_registry_selection_repair_plan_path if runtime_registry_selection_repair_plan_path.exists() else None,
            )
        )
        summary = build_summary(
            args.mode,
            pipeline_phase,
            artifact_root,
            baseline_manifest_path,
            current_manifest_path,
            refresh_manifest_path,
            rerun_plan_path,
            effective_runtime_baseline_manifest_path_for_pipeline,
            runtime_baseline_registry_path,
            runtime_current_manifest_path,
            runtime_refresh_manifest_path,
            runtime_rerun_plan_path,
            runtime_history_index_path,
            runtime_proposal_path,
            runtime_watch_current_path,
            runtime_watch_refresh_path,
            runtime_watch_history_index_path,
            bundle_metadata_path,
            stages,
        )
        summary = enrich_summary_with_operator_artifacts(
            summary,
            runtime_watch_registry_path,
            ops_summary_path,
            runtime_registry_health_path,
            publication_health_path if publication_health_path.exists() else None,
            source_health_preflight_path if source_health_preflight_path.exists() else None,
            source_health_action_plan_path if source_health_action_plan_path.exists() else None,
            verification_preflight_gate_path if verification_preflight_gate_path.exists() else None,
            staged_overlay_completeness_path if staged_overlay_completeness_path.exists() else None,
            verification_preflight_gate_v2_path if verification_preflight_gate_v2_path.exists() else None,
            staged_materialization_path if staged_materialization_path.exists() else None,
            runtime_comparability_triage_path if runtime_comparability_triage_path.exists() else None,
            runtime_rebaseline_action_plan_path if runtime_rebaseline_action_plan_path.exists() else None,
            runtime_rebaseline_runbook_path if runtime_rebaseline_runbook_path.exists() else None,
            runtime_registry_selection_sanity_path if runtime_registry_selection_sanity_path.exists() else None,
            runtime_registry_selection_repair_plan_path if runtime_registry_selection_repair_plan_path.exists() else None,
        )
        write_pipeline_summary(summary_path, summary)

        if args.mode in {"nightly", "full_local", "bundle_only"}:
            if runtime_history_index_path is not None and runtime_history_index_path.exists():
                stages.append(
                    run_command(
                        "runtime_history_compact",
                        [
                            python_bin,
                            str(runtime_gate_script),
                            "history-compact",
                            "--runtime-history-index",
                            str(runtime_history_index_path),
                            "--compact-out",
                            str(runtime_history_compact_path),
                            "--keep-latest-per-fingerprint",
                            "4",
                            "--keep-anchors",
                            "1",
                            "--keep-transitions",
                            "--prune-old-fixture-history",
                            "--compact-watch-history",
                        ],
                        repo_root,
                    )
                )
            if args.skip_bundle_build:
                stages.append(
                    skipped_stage(
                        "build_evidence_bundle",
                        [],
                        "bundle stage skipped; summaries were generated without publishing bundles",
                    )
                )
            else:
                ensure_report(report_path, pipeline_phase, args.mode, summary["recommended_next_action"])
                stages.append(
                    run_command(
                        "build_evidence_bundle",
                        [
                            python_bin,
                            str(bundle_script),
                            "--phase",
                            pipeline_phase,
                            "--artifact-root",
                            str(artifact_root),
                            "--report-out",
                            str(report_path),
                            "--policy-manifest",
                            str(current_manifest_path),
                            "--baseline-manifest",
                            str(baseline_manifest_path),
                            "--current-manifest",
                            str(current_manifest_path),
                            "--refresh-manifest",
                            str(refresh_manifest_path),
                            "--rerun-plan",
                            str(rerun_plan_path),
                            "--runtime-baseline-manifest",
                            str(effective_runtime_baseline_manifest_path_for_pipeline),
                            "--runtime-current-manifest",
                            str(runtime_current_manifest_path),
                            "--runtime-refresh-manifest",
                            str(runtime_refresh_manifest_path),
                            "--runtime-rerun-plan",
                            str(runtime_rerun_plan_path),
                            *(["--runtime-registry", str(runtime_baseline_registry_path)] if runtime_baseline_registry_path is not None else []),
                            *(["--runtime-registry-health", str(runtime_registry_health_path)] if runtime_registry_health_path.exists() else []),
                            *(["--runtime-history-index", str(runtime_history_index_path)] if runtime_history_index_path is not None else []),
                            *(["--runtime-history-compact", str(runtime_history_compact_path)] if runtime_history_compact_path.exists() else []),
                            *(["--runtime-proposal", str(runtime_proposal_path)] if runtime_proposal_path is not None else []),
                            "--runtime-watch-current",
                            str(runtime_watch_current_path),
                            "--runtime-watch-refresh",
                            str(runtime_watch_refresh_path),
                            "--runtime-watch-history-index",
                            str(runtime_watch_history_index_path),
                            "--runtime-watch-registry",
                            str(runtime_watch_registry_path),
                            *(["--source-snapshot-manifest", str(source_snapshot_manifest_path)] if source_snapshot_manifest_path.exists() else []),
                            *(["--staged-mirror-manifest", str(staged_mirror_manifest_path)] if staged_mirror_manifest_path.exists() else []),
                            *(["--staged-mirror-verify", str(staged_mirror_verify_path)] if staged_mirror_verify_path.exists() else []),
                            *(["--ctest-inventory-release", str(ctest_inventory_release_path)] if ctest_inventory_release_path.exists() else []),
                            *(["--ctest-inventory-debug", str(ctest_inventory_debug_path)] if ctest_inventory_debug_path.exists() else []),
                            *(["--ctest-inventory-asan", str(ctest_inventory_asan_path)] if ctest_inventory_asan_path.exists() else []),
                            *(["--verification-release", str(verification_release_path)] if verification_release_path.exists() else []),
                            *(["--verification-debug", str(verification_debug_path)] if verification_debug_path.exists() else []),
                            *(["--verification-asan", str(verification_asan_path)] if verification_asan_path.exists() else []),
                            *(["--published-snapshot-manifest", str(published_snapshot_manifest_path)] if published_snapshot_manifest_path.exists() else []),
                            *(["--verification-closeout", str(verification_closeout_path)] if verification_closeout_path.exists() else []),
                            *(["--publication-health", str(publication_health_path)] if publication_health_path.exists() else []),
                            *(["--source-health-preflight", str(source_health_preflight_path)] if source_health_preflight_path.exists() else []),
                            *(["--source-health-action-plan", str(source_health_action_plan_path)] if source_health_action_plan_path.exists() else []),
                            *(["--staged-overlay-completeness", str(staged_overlay_completeness_path)] if staged_overlay_completeness_path.exists() else []),
                            *(["--verification-preflight-gate-v2", str(verification_preflight_gate_v2_path)] if verification_preflight_gate_v2_path.exists() else []),
                            *(["--staged-materialization", str(staged_materialization_path)] if staged_materialization_path.exists() else []),
                            "--ops-summary",
                            str(ops_summary_path),
                            *sum([["--approved-known-summary", str(path)] for path in approved_known_summary_paths], []),
                            *sum([["--foreign-import-summary", str(path)] for path in foreign_import_summary_paths], []),
                            *(["--known-env-governance-policy", str(Path(args.known_env_governance_policy).resolve())] if args.known_env_governance_policy else []),
                            *(["--known-env-age-tick", str(Path(args.known_env_age_tick).resolve())] if args.known_env_age_tick else []),
                            *(["--known-env-reverify-plan", str(Path(args.known_env_reverify_plan).resolve())] if args.known_env_reverify_plan else []),
                            *(["--known-env-reverify-gate", str(Path(args.known_env_reverify_gate).resolve())] if args.known_env_reverify_gate else []),
                            *(["--known-env-reverify-apply", str(Path(args.known_env_reverify_apply).resolve())] if args.known_env_reverify_apply else []),
                            *(["--known-env-retire-plan", str(Path(args.known_env_retire_plan).resolve())] if args.known_env_retire_plan else []),
                            *(["--known-env-retire-apply", str(Path(args.known_env_retire_apply).resolve())] if args.known_env_retire_apply else []),
                            *(["--known-env-retire-metadata", str(Path(args.known_env_retire_metadata).resolve())] if args.known_env_retire_metadata else []),
                            *(["--current-env-governance-policy", str(Path(args.current_env_governance_policy).resolve())] if args.current_env_governance_policy else []),
                            *(["--current-env-guardrail-policy", str(Path(args.current_env_guardrail_policy).resolve())] if args.current_env_guardrail_policy else []),
                            *(["--current-env-watch-current", str(Path(args.current_env_watch_current).resolve())] if args.current_env_watch_current else []),
                            *(["--current-env-watch-refresh", str(Path(args.current_env_watch_refresh).resolve())] if args.current_env_watch_refresh else []),
                            *(["--current-env-watch-history", str(Path(args.current_env_watch_history).resolve())] if args.current_env_watch_history else []),
                            *(["--current-env-age-tick", str(Path(args.current_env_age_tick).resolve())] if args.current_env_age_tick else []),
                            *(["--current-env-watch-plan", str(Path(args.current_env_watch_plan).resolve())] if args.current_env_watch_plan else []),
                            *(["--current-env-trigger-gate", str(Path(args.current_env_trigger_gate).resolve())] if args.current_env_trigger_gate else []),
                            *(["--runtime-budget-current", str(Path(args.runtime_budget_current).resolve())] if args.runtime_budget_current else []),
                            *(["--runtime-budget-proposal", str(Path(args.runtime_budget_proposal).resolve())] if args.runtime_budget_proposal else []),
                            *(["--runtime-budget-proposal-gate", str(Path(args.runtime_budget_proposal_gate).resolve())] if args.runtime_budget_proposal_gate else []),
                            *(["--runtime-budget-baseline", str(Path(args.runtime_budget_baseline).resolve())] if args.runtime_budget_baseline else []),
                            *(["--runtime-budget-refresh", str(Path(args.runtime_budget_refresh).resolve())] if args.runtime_budget_refresh else []),
                            *(["--runtime-budget-reproposal-history", str(Path(args.runtime_budget_reproposal_history).resolve())] if args.runtime_budget_reproposal_history else []),
                            *(["--runtime-budget-registry-summary", str(Path(args.runtime_budget_registry_summary).resolve())] if args.runtime_budget_registry_summary else []),
                            *(["--runtime-comparability-triage", str(runtime_comparability_triage_path)] if runtime_comparability_triage_path.exists() else []),
                            *(["--runtime-rebaseline-action-plan", str(runtime_rebaseline_action_plan_path)] if runtime_rebaseline_action_plan_path.exists() else []),
                            *(["--runtime-rebaseline-proposal", str(runtime_rebaseline_proposal_path)] if runtime_rebaseline_proposal_path.exists() else []),
                            *(["--runtime-rebaseline-proposal-gate", str(runtime_rebaseline_proposal_gate_path)] if runtime_rebaseline_proposal_gate_path.exists() else []),
                            *(["--runtime-rebaseline-runbook", str(runtime_rebaseline_runbook_path)] if runtime_rebaseline_runbook_path.exists() else []),
                            *(["--runtime-registry-selection-sanity", str(runtime_registry_selection_sanity_path)] if runtime_registry_selection_sanity_path.exists() else []),
                            *(["--runtime-registry-selection-repair-plan", str(runtime_registry_selection_repair_plan_path)] if runtime_registry_selection_repair_plan_path.exists() else []),
                            "--pipeline-summary",
                            str(summary_path),
                            *(["--pipeline-quick-summary", str(default_summary_path(artifact_root, pipeline_phase, "quick"))]
                              if default_summary_path(artifact_root, pipeline_phase, "quick").exists()
                              else []),
                            *(["--pipeline-matrix-summary", str(default_matrix_summary_path(artifact_root, pipeline_phase))]
                              if default_matrix_summary_path(artifact_root, pipeline_phase).exists()
                              else []),
                            "--zip-out",
                            str(zip_out),
                            "--curated-zip",
                            str(curated_zip),
                            "--light-ops-zip",
                            str(light_ops_zip),
                            *(["--use-published-snapshot"] if args.use_published_snapshot else []),
                            *(["--published-root", str(Path(args.published_root).resolve())] if args.published_root else []),
                            *(["--prune-artifacts", "--max-bundles", str(args.max_bundles), "--max-nightly-runs", str(args.max_nightly_runs)] if args.prune_artifacts else []),
                            *(["--keep-approved"] if args.prune_artifacts and args.keep_approved else []),
                        ],
                        repo_root,
                    )
                )
            bundle_root = (
                Path(args.published_root).resolve()
                if args.published_root
                else default_published_bundle_root(artifact_root, pipeline_phase)
            ) if args.use_published_snapshot else artifact_root / f"{pipeline_phase}_evidence_bundle"
            if not args.skip_bundle_build:
                stages.append(
                    run_command(
                        "publication_health",
                        [
                            python_bin,
                            str(watch_ops_script),
                            "publication-health",
                            "--phase",
                            pipeline_phase,
                            "--published-root",
                            str(bundle_root),
                            "--authoritative-root",
                            str(repo_root),
                            "--health-out",
                            str(publication_health_path),
                            "--expect-bundles",
                            "--expect-manifests",
                            "--expect-report",
                        ],
                        repo_root,
                    )
                )
                stages.append(
                    run_command(
                        "policy_ops_summary_post_publication",
                        [
                            python_bin,
                            str(watch_ops_script),
                            "ops-summary",
                            "--phase",
                            pipeline_phase,
                            "--policy-manifest",
                            str(current_manifest_path),
                            "--quick-summary",
                            str(default_summary_path(artifact_root, pipeline_phase, "quick")),
                            "--nightly-summary",
                            str(default_summary_path(artifact_root, pipeline_phase, "nightly")),
                            "--matrix-summary",
                            str(default_matrix_summary_path(artifact_root, pipeline_phase)),
                            "--runtime-refresh",
                            str(runtime_refresh_manifest_path),
                            "--runtime-watch-refresh",
                            str(runtime_watch_refresh_path),
                            "--runtime-watch-registry",
                            str(runtime_watch_registry_path),
                            *(["--runtime-baseline-registry", str(runtime_baseline_registry_path)] if runtime_baseline_registry_path is not None else []),
                            "--runtime-registry-health",
                            str(runtime_registry_health_path),
                            "--publication-health",
                            str(publication_health_path),
                            *(["--source-snapshot-manifest", str(source_snapshot_manifest_path)] if source_snapshot_manifest_path.exists() else []),
                            *(["--staged-mirror-verify", str(staged_mirror_verify_path)] if staged_mirror_verify_path.exists() else []),
                            *(["--verification-release", str(verification_release_path)] if verification_release_path.exists() else []),
                            *(["--verification-debug", str(verification_debug_path)] if verification_debug_path.exists() else []),
                            *(["--verification-asan", str(verification_asan_path)] if verification_asan_path.exists() else []),
                            *(["--published-snapshot-manifest", str(published_snapshot_manifest_path)] if published_snapshot_manifest_path.exists() else []),
                            *(["--verification-closeout", str(verification_closeout_path)] if verification_closeout_path.exists() else []),
                            *(["--current-env-governance-policy", str(Path(args.current_env_governance_policy).resolve())] if args.current_env_governance_policy else []),
                            *(["--current-env-guardrail-policy", str(Path(args.current_env_guardrail_policy).resolve())] if args.current_env_guardrail_policy else []),
                            *(["--current-env-watch-current", str(Path(args.current_env_watch_current).resolve())] if args.current_env_watch_current else []),
                            *(["--current-env-watch-refresh", str(Path(args.current_env_watch_refresh).resolve())] if args.current_env_watch_refresh else []),
                            *(["--current-env-watch-history", str(Path(args.current_env_watch_history).resolve())] if args.current_env_watch_history else []),
                            *(["--current-env-age-tick", str(Path(args.current_env_age_tick).resolve())] if args.current_env_age_tick else []),
                            *(["--current-env-watch-plan", str(Path(args.current_env_watch_plan).resolve())] if args.current_env_watch_plan else []),
                            *(["--current-env-trigger-gate", str(Path(args.current_env_trigger_gate).resolve())] if args.current_env_trigger_gate else []),
                            *(["--runtime-budget-current", str(Path(args.runtime_budget_current).resolve())] if args.runtime_budget_current else []),
                            *(["--runtime-budget-proposal", str(Path(args.runtime_budget_proposal).resolve())] if args.runtime_budget_proposal else []),
                            *(["--runtime-budget-proposal-gate", str(Path(args.runtime_budget_proposal_gate).resolve())] if args.runtime_budget_proposal_gate else []),
                            *(["--runtime-budget-baseline", str(Path(args.runtime_budget_baseline).resolve())] if args.runtime_budget_baseline else []),
                            *(["--runtime-budget-refresh", str(Path(args.runtime_budget_refresh).resolve())] if args.runtime_budget_refresh else []),
                            *(["--runtime-budget-reproposal-history", str(Path(args.runtime_budget_reproposal_history).resolve())] if args.runtime_budget_reproposal_history else []),
                            *(["--runtime-budget-registry-summary", str(Path(args.runtime_budget_registry_summary).resolve())] if args.runtime_budget_registry_summary else []),
                            *(["--runtime-comparability-triage", str(runtime_comparability_triage_path)] if runtime_comparability_triage_path.exists() else []),
                            *(["--runtime-rebaseline-action-plan", str(runtime_rebaseline_action_plan_path)] if runtime_rebaseline_action_plan_path.exists() else []),
                            *(["--runtime-rebaseline-proposal", str(runtime_rebaseline_proposal_path)] if runtime_rebaseline_proposal_path.exists() else []),
                            *(["--runtime-rebaseline-proposal-gate", str(runtime_rebaseline_proposal_gate_path)] if runtime_rebaseline_proposal_gate_path.exists() else []),
                            *(["--runtime-rebaseline-runbook", str(runtime_rebaseline_runbook_path)] if runtime_rebaseline_runbook_path.exists() else []),
                            *(["--runtime-registry-selection-sanity", str(runtime_registry_selection_sanity_path)] if runtime_registry_selection_sanity_path.exists() else []),
                            *(["--runtime-registry-selection-repair-plan", str(runtime_registry_selection_repair_plan_path)] if runtime_registry_selection_repair_plan_path.exists() else []),
                            *sum([["--approved-known-summary", str(path)] for path in approved_known_summary_paths], []),
                            *sum([["--foreign-import-summary", str(path)] for path in foreign_import_summary_paths], []),
                            "--out",
                            str(ops_summary_path),
                        ],
                        repo_root,
                    )
                )
                stages.append(
                    run_command(
                        "build_evidence_bundle_post_publication",
                        [
                            python_bin,
                            str(bundle_script),
                            "--phase",
                            pipeline_phase,
                            "--artifact-root",
                            str(artifact_root),
                            "--report-out",
                            str(report_path),
                            "--policy-manifest",
                            str(current_manifest_path),
                            "--baseline-manifest",
                            str(baseline_manifest_path),
                            "--current-manifest",
                            str(current_manifest_path),
                            "--refresh-manifest",
                            str(refresh_manifest_path),
                            "--rerun-plan",
                            str(rerun_plan_path),
                            "--runtime-baseline-manifest",
                            str(effective_runtime_baseline_manifest_path_for_pipeline),
                            "--runtime-current-manifest",
                            str(runtime_current_manifest_path),
                            "--runtime-refresh-manifest",
                            str(runtime_refresh_manifest_path),
                            "--runtime-rerun-plan",
                            str(runtime_rerun_plan_path),
                            *(["--runtime-registry", str(runtime_baseline_registry_path)] if runtime_baseline_registry_path is not None else []),
                            *(["--runtime-registry-health", str(runtime_registry_health_path)] if runtime_registry_health_path.exists() else []),
                            *(["--runtime-history-index", str(runtime_history_index_path)] if runtime_history_index_path is not None else []),
                            *(["--runtime-history-compact", str(runtime_history_compact_path)] if runtime_history_compact_path.exists() else []),
                            *(["--runtime-proposal", str(runtime_proposal_path)] if runtime_proposal_path is not None else []),
                            "--runtime-watch-current",
                            str(runtime_watch_current_path),
                            "--runtime-watch-refresh",
                            str(runtime_watch_refresh_path),
                            "--runtime-watch-history-index",
                            str(runtime_watch_history_index_path),
                            "--runtime-watch-registry",
                            str(runtime_watch_registry_path),
                            *(["--source-snapshot-manifest", str(source_snapshot_manifest_path)] if source_snapshot_manifest_path.exists() else []),
                            *(["--source-health-preflight", str(source_health_preflight_path)] if source_health_preflight_path.exists() else []),
                            *(["--source-health-action-plan", str(source_health_action_plan_path)] if source_health_action_plan_path.exists() else []),
                            *(["--staged-overlay-completeness", str(staged_overlay_completeness_path)] if staged_overlay_completeness_path.exists() else []),
                            *(["--verification-preflight-gate-v2", str(verification_preflight_gate_v2_path)] if verification_preflight_gate_v2_path.exists() else []),
                            *(["--staged-materialization", str(staged_materialization_path)] if staged_materialization_path.exists() else []),
                            *(["--staged-mirror-manifest", str(staged_mirror_manifest_path)] if staged_mirror_manifest_path.exists() else []),
                            *(["--staged-mirror-verify", str(staged_mirror_verify_path)] if staged_mirror_verify_path.exists() else []),
                            *(["--ctest-inventory-release", str(ctest_inventory_release_path)] if ctest_inventory_release_path.exists() else []),
                            *(["--ctest-inventory-debug", str(ctest_inventory_debug_path)] if ctest_inventory_debug_path.exists() else []),
                            *(["--ctest-inventory-asan", str(ctest_inventory_asan_path)] if ctest_inventory_asan_path.exists() else []),
                            *(["--verification-release", str(verification_release_path)] if verification_release_path.exists() else []),
                            *(["--verification-debug", str(verification_debug_path)] if verification_debug_path.exists() else []),
                            *(["--verification-asan", str(verification_asan_path)] if verification_asan_path.exists() else []),
                            *(["--published-snapshot-manifest", str(published_snapshot_manifest_path)] if published_snapshot_manifest_path.exists() else []),
                            *(["--verification-closeout", str(verification_closeout_path)] if verification_closeout_path.exists() else []),
                            "--publication-health",
                            str(publication_health_path),
                            "--ops-summary",
                            str(ops_summary_path),
                            *sum([["--approved-known-summary", str(path)] for path in approved_known_summary_paths], []),
                            *sum([["--foreign-import-summary", str(path)] for path in foreign_import_summary_paths], []),
                            *(["--known-env-governance-policy", str(Path(args.known_env_governance_policy).resolve())] if args.known_env_governance_policy else []),
                            *(["--known-env-age-tick", str(Path(args.known_env_age_tick).resolve())] if args.known_env_age_tick else []),
                            *(["--known-env-reverify-plan", str(Path(args.known_env_reverify_plan).resolve())] if args.known_env_reverify_plan else []),
                            *(["--known-env-reverify-gate", str(Path(args.known_env_reverify_gate).resolve())] if args.known_env_reverify_gate else []),
                            *(["--known-env-reverify-apply", str(Path(args.known_env_reverify_apply).resolve())] if args.known_env_reverify_apply else []),
                            *(["--known-env-retire-plan", str(Path(args.known_env_retire_plan).resolve())] if args.known_env_retire_plan else []),
                            *(["--known-env-retire-apply", str(Path(args.known_env_retire_apply).resolve())] if args.known_env_retire_apply else []),
                            *(["--known-env-retire-metadata", str(Path(args.known_env_retire_metadata).resolve())] if args.known_env_retire_metadata else []),
                            *(["--current-env-governance-policy", str(Path(args.current_env_governance_policy).resolve())] if args.current_env_governance_policy else []),
                            *(["--current-env-guardrail-policy", str(Path(args.current_env_guardrail_policy).resolve())] if args.current_env_guardrail_policy else []),
                            *(["--current-env-watch-current", str(Path(args.current_env_watch_current).resolve())] if args.current_env_watch_current else []),
                            *(["--current-env-watch-refresh", str(Path(args.current_env_watch_refresh).resolve())] if args.current_env_watch_refresh else []),
                            *(["--current-env-watch-history", str(Path(args.current_env_watch_history).resolve())] if args.current_env_watch_history else []),
                            *(["--current-env-age-tick", str(Path(args.current_env_age_tick).resolve())] if args.current_env_age_tick else []),
                            *(["--current-env-watch-plan", str(Path(args.current_env_watch_plan).resolve())] if args.current_env_watch_plan else []),
                            *(["--current-env-trigger-gate", str(Path(args.current_env_trigger_gate).resolve())] if args.current_env_trigger_gate else []),
                            *(["--runtime-budget-current", str(Path(args.runtime_budget_current).resolve())] if args.runtime_budget_current else []),
                            *(["--runtime-budget-proposal", str(Path(args.runtime_budget_proposal).resolve())] if args.runtime_budget_proposal else []),
                            *(["--runtime-budget-proposal-gate", str(Path(args.runtime_budget_proposal_gate).resolve())] if args.runtime_budget_proposal_gate else []),
                            *(["--runtime-budget-baseline", str(Path(args.runtime_budget_baseline).resolve())] if args.runtime_budget_baseline else []),
                            *(["--runtime-budget-refresh", str(Path(args.runtime_budget_refresh).resolve())] if args.runtime_budget_refresh else []),
                            *(["--runtime-budget-reproposal-history", str(Path(args.runtime_budget_reproposal_history).resolve())] if args.runtime_budget_reproposal_history else []),
                            *(["--runtime-budget-registry-summary", str(Path(args.runtime_budget_registry_summary).resolve())] if args.runtime_budget_registry_summary else []),
                            *(["--runtime-comparability-triage", str(runtime_comparability_triage_path)] if runtime_comparability_triage_path.exists() else []),
                            *(["--runtime-rebaseline-action-plan", str(runtime_rebaseline_action_plan_path)] if runtime_rebaseline_action_plan_path.exists() else []),
                            *(["--runtime-rebaseline-proposal", str(runtime_rebaseline_proposal_path)] if runtime_rebaseline_proposal_path.exists() else []),
                            *(["--runtime-rebaseline-proposal-gate", str(runtime_rebaseline_proposal_gate_path)] if runtime_rebaseline_proposal_gate_path.exists() else []),
                            *(["--runtime-rebaseline-runbook", str(runtime_rebaseline_runbook_path)] if runtime_rebaseline_runbook_path.exists() else []),
                            *(["--runtime-registry-selection-sanity", str(runtime_registry_selection_sanity_path)] if runtime_registry_selection_sanity_path.exists() else []),
                            *(["--runtime-registry-selection-repair-plan", str(runtime_registry_selection_repair_plan_path)] if runtime_registry_selection_repair_plan_path.exists() else []),
                            "--pipeline-summary",
                            str(summary_path),
                            *(["--pipeline-quick-summary", str(default_summary_path(artifact_root, pipeline_phase, "quick"))]
                              if default_summary_path(artifact_root, pipeline_phase, "quick").exists()
                              else []),
                            *(["--pipeline-matrix-summary", str(default_matrix_summary_path(artifact_root, pipeline_phase))]
                              if default_matrix_summary_path(artifact_root, pipeline_phase).exists()
                              else []),
                            "--zip-out",
                            str(zip_out),
                            "--curated-zip",
                            str(curated_zip),
                            "--light-ops-zip",
                            str(light_ops_zip),
                            *(["--use-published-snapshot"] if args.use_published_snapshot else []),
                            *(["--published-root", str(Path(args.published_root).resolve())] if args.published_root else []),
                            *(["--prune-artifacts", "--max-bundles", str(args.max_bundles), "--max-nightly-runs", str(args.max_nightly_runs)] if args.prune_artifacts else []),
                            *(["--keep-approved"] if args.prune_artifacts and args.keep_approved else []),
                        ],
                        repo_root,
                    )
                )
            bundle_metadata_path = bundle_root / "bundle_metadata.json"
            summary = build_summary(
                args.mode,
                pipeline_phase,
                artifact_root,
                baseline_manifest_path,
                current_manifest_path,
                refresh_manifest_path,
                rerun_plan_path,
                runtime_baseline_manifest_path,
                runtime_baseline_registry_path,
                runtime_current_manifest_path,
                runtime_refresh_manifest_path,
                runtime_rerun_plan_path,
                runtime_history_index_path,
                runtime_proposal_path,
                runtime_watch_current_path,
                runtime_watch_refresh_path,
                runtime_watch_history_index_path,
                bundle_metadata_path,
                stages,
            )
            summary = enrich_summary_with_operator_artifacts(
                summary,
                runtime_watch_registry_path,
                ops_summary_path,
                runtime_registry_health_path,
                publication_health_path if publication_health_path.exists() else None,
                source_health_preflight_path if source_health_preflight_path.exists() else None,
                source_health_action_plan_path if source_health_action_plan_path.exists() else None,
                verification_preflight_gate_path if verification_preflight_gate_path.exists() else None,
                staged_overlay_completeness_path if staged_overlay_completeness_path.exists() else None,
                verification_preflight_gate_v2_path if verification_preflight_gate_v2_path.exists() else None,
                staged_materialization_path if staged_materialization_path.exists() else None,
                runtime_comparability_triage_path if runtime_comparability_triage_path.exists() else None,
                runtime_rebaseline_action_plan_path if runtime_rebaseline_action_plan_path.exists() else None,
                runtime_rebaseline_runbook_path if runtime_rebaseline_runbook_path.exists() else None,
                runtime_registry_selection_sanity_path if runtime_registry_selection_sanity_path.exists() else None,
                runtime_registry_selection_repair_plan_path if runtime_registry_selection_repair_plan_path.exists() else None,
            )
            write_pipeline_summary(summary_path, summary)

        final_summary = read_json(summary_path)
        print(pipeline_summary_text(final_summary), end="")
        return int(final_summary["exit_code"])
    except Exception as exc:  # noqa: BLE001
        failure = {
            "summary_version": "policy_pipeline_summary_v3",
            "timestamp_utc": timestamp_utc_now(),
            "phase": pipeline_phase,
            "mode": args.mode,
            "artifact_root": str(artifact_root),
            "baseline_manifest": str(baseline_manifest_path),
            "current_manifest": str(current_manifest_path),
            "refresh_manifest": str(refresh_manifest_path),
            "rerun_plan": str(rerun_plan_path),
            "runtime_baseline_manifest": str(runtime_baseline_manifest_path),
            "runtime_baseline_registry": None if runtime_baseline_registry_path is None else str(runtime_baseline_registry_path),
            "runtime_current_manifest": str(runtime_current_manifest_path),
            "runtime_refresh_manifest": str(runtime_refresh_manifest_path),
            "runtime_rerun_plan": str(runtime_rerun_plan_path),
            "runtime_history_index": None if runtime_history_index_path is None else str(runtime_history_index_path),
            "runtime_proposal": None if runtime_proposal_path is None else str(runtime_proposal_path),
            "runtime_watch_registry": str(runtime_watch_registry_path),
            "policy_ops_summary": str(ops_summary_path),
            "severity": "FAIL",
            "runtime_severity": "FAIL",
            "exit_code": EXIT_FAIL,
            "recommended_next_action": "pipeline execution failed; inspect command output and runtime/policy manifests",
            "error": str(exc),
            "stages": stages,
        }
        write_pipeline_summary(summary_path, failure)
        print(pipeline_summary_text(failure), end="")
        return EXIT_FAIL


if __name__ == "__main__":
    raise SystemExit(main())
