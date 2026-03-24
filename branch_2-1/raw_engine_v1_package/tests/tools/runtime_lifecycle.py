#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from runtime_gate_lib import (
    FRESHNESS_NOT_COMPARABLE,
    FRESHNESS_REBASELINE_REQUIRED,
    FRESHNESS_REQUIRES_RERUN,
    FRESHNESS_STALE,
    STATUS_FAIL,
    STATUS_WARN,
    build_runtime_current_manifest,
    build_runtime_rerun_plan,
    default_runtime_threshold,
    host_fingerprint,
    promote_runtime_baseline,
    read_json,
    refresh_runtime_manifest,
    runtime_manifest_summary as current_manifest_summary,
    runtime_manifest_text as current_manifest_text,
    runtime_refresh_summary,
    runtime_refresh_text,
    runtime_rerun_plan_summary,
    runtime_rerun_plan_text,
    write_runtime_manifest_outputs,
    write_runtime_refresh_outputs,
    write_runtime_rerun_plan_outputs,
)


def load_budget_config(path: Path | None) -> dict[str, dict[str, float]]:
    if path is None or not path.exists():
        return {}
    data = json.loads(path.read_text(encoding="utf-8"))
    values = data.get("execution_classes", data)
    out: dict[str, dict[str, float]] = {}
    for execution_class, threshold in values.items():
        if not isinstance(threshold, dict):
            continue
        out[str(execution_class)] = {str(key): float(value) for key, value in threshold.items()}
    return out


def default_entry_fields(execution_class: str) -> tuple[str, str]:
    if execution_class == "release_full":
        return "Release", "none"
    if execution_class == "debug_full":
        return "Debug", "none"
    if execution_class == "asan_full":
        return "ASan", "asan+ubsan"
    return "Tool", "none"


def parse_runtime_entry_spec(
    spec: str,
    host: dict[str, str],
    budget_config: dict[str, dict[str, float]] | None = None,
    test_counts: dict[str, int] | None = None,
) -> dict[str, Any]:
    if "=" not in spec:
        raise ValueError(f"runtime entry must be execution_class=wall_time[:build_type[:sanitizer_flags]]: {spec}")
    execution_class, payload = spec.split("=", 1)
    parts = [part.strip() for part in payload.split(":")]
    if not parts or not parts[0]:
        raise ValueError(f"runtime entry wall time missing: {spec}")
    default_build_type, default_sanitizer = default_entry_fields(execution_class.strip())
    test_count = 0 if test_counts is None else int(test_counts.get(execution_class.strip(), 0))
    return {
        "execution_class": execution_class.strip(),
        "wall_time_sec": round(float(parts[0]), 3),
        "test_count": test_count,
        "build_type": parts[1] if len(parts) > 1 and parts[1] else default_build_type,
        "sanitizer_flags": parts[2] if len(parts) > 2 and parts[2] else default_sanitizer,
        "budget_thresholds": dict((budget_config or {}).get(execution_class.strip(), default_runtime_threshold(execution_class.strip()))),
        "runner_tag": host.get("runner_tag", ""),
    }


def synthesize_runtime_current_manifest(
    phase: str,
    artifact_root: Path,
    entries: list[dict[str, Any]],
    runtime_baseline_manifest: Path | None = None,
    baseline_tag: str = "",
    runner_tag: str = "",
) -> dict[str, Any]:
    return build_runtime_current_manifest(
        phase,
        str(artifact_root),
        entries,
        runner_tag=runner_tag,
        baseline_tag=baseline_tag,
    )


def write_manifest_sidecars(path: Path, manifest: dict[str, Any], text: str, summary: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    path.with_suffix(".txt").write_text(text, encoding="utf-8")
    path.with_name(f"{path.stem}.summary.txt").write_text(summary, encoding="utf-8")


def runtime_manifest_text(manifest: dict[str, Any]) -> str:
    role = str(manifest.get("manifest_role", ""))
    if role == "refresh":
        return runtime_refresh_text(manifest)
    if "summary_verdict" in manifest:
        return runtime_rerun_plan_text(manifest)
    return current_manifest_text(manifest)


def runtime_manifest_summary(manifest: dict[str, Any]) -> str:
    role = str(manifest.get("manifest_role", ""))
    if role == "refresh":
        return runtime_refresh_summary(manifest)
    if "summary_verdict" in manifest:
        return runtime_rerun_plan_summary(manifest)
    return current_manifest_summary(manifest)


def apply_synthetic_runtime_mutations(
    current_manifest: dict[str, Any],
    inflate_specs: dict[str, str] | dict[str, float] | None,
    fingerprint_mismatch: str | None,
) -> dict[str, Any]:
    mutated = json.loads(json.dumps(current_manifest))
    if inflate_specs:
        for entry in mutated.get("entries", []):
            execution_class = str(entry.get("execution_class", ""))
            if execution_class not in inflate_specs:
                continue
            value = inflate_specs[execution_class]
            factor = 1.0
            if isinstance(value, str):
                lowered = value.lower()
                if lowered == "warn":
                    factor = 1.5
                elif lowered == "fail":
                    factor = 10.0
                else:
                    factor = float(value)
            else:
                factor = float(value)
            entry["wall_time_sec"] = round(float(entry.get("wall_time_sec", 0.0)) * factor, 3)
        mutated["current_runtime_manifest_hash"] = ""
    if fingerprint_mismatch:
        mutated.setdefault("host_fingerprint", {})
        mutated["host_fingerprint"]["runner_tag"] = fingerprint_mismatch
        mutated["host_fingerprint"]["fingerprint_hash"] = str(
            mutated["host_fingerprint"].get("fingerprint_hash", "")
        ) + "|synthetic"
    return mutated


def determine_runtime_severity(refresh_manifest: dict[str, Any]) -> str:
    current_verdict = str(refresh_manifest.get("current_verdict", "PASS"))
    comparability_verdict = str(refresh_manifest.get("comparability_verdict", "COMPARABLE"))
    freshness_verdict = str(refresh_manifest.get("freshness_verdict", "FRESH"))
    if current_verdict == "FAIL":
        return "FAIL"
    if comparability_verdict in {FRESHNESS_REBASELINE_REQUIRED, FRESHNESS_NOT_COMPARABLE}:
        return "ACTION_REQUIRED"
    if freshness_verdict in {FRESHNESS_STALE, FRESHNESS_REQUIRES_RERUN}:
        return "ACTION_REQUIRED"
    if current_verdict == "WARN":
        return "WARN"
    return "OK"


def parse_count_specs(specs: list[str]) -> dict[str, int]:
    out: dict[str, int] = {}
    for spec in specs:
        if "=" not in spec:
            raise ValueError(f"runtime test count must be execution_class=count: {spec}")
        execution_class, value = spec.split("=", 1)
        out[execution_class.strip()] = int(value.strip())
    return out


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Manage runtime lifecycle manifests.")
    parser.add_argument("subcommand", choices=["write-current", "promote-baseline", "refresh", "plan-rerun"])
    parser.add_argument("--phase", default="phase23")
    parser.add_argument("--artifact-root", default="artifacts")
    parser.add_argument("--runtime-current-manifest", default=None)
    parser.add_argument("--runtime-baseline-manifest", default=None)
    parser.add_argument("--runtime-refresh-manifest", default=None)
    parser.add_argument("--runtime-rerun-plan", default=None)
    parser.add_argument("--runtime-budget-config", default=None)
    parser.add_argument("--baseline-tag", default="")
    parser.add_argument("--runner-tag", default="")
    parser.add_argument("--require-acceptable-status", action="store_true")
    parser.add_argument("--entry", action="append", default=[])
    parser.add_argument("--test-count", action="append", default=[])
    parser.add_argument("--synthetic-inflate", action="append", default=[])
    parser.add_argument("--synthetic-fingerprint-mismatch", default=None)
    return parser.parse_args()


def parse_inflate_specs(specs: list[str]) -> dict[str, str]:
    out: dict[str, str] = {}
    for spec in specs:
        if "=" not in spec:
            raise ValueError(f"synthetic inflate must be execution_class=warn|fail|factor: {spec}")
        execution_class, value = spec.split("=", 1)
        out[execution_class.strip()] = value.strip()
    return out


def main() -> int:
    args = parse_args()
    artifact_root = Path(args.artifact_root).resolve()
    current_path = Path(args.runtime_current_manifest).resolve() if args.runtime_current_manifest else None
    baseline_path = Path(args.runtime_baseline_manifest).resolve() if args.runtime_baseline_manifest else None
    refresh_path = Path(args.runtime_refresh_manifest).resolve() if args.runtime_refresh_manifest else None
    rerun_path = Path(args.runtime_rerun_plan).resolve() if args.runtime_rerun_plan else None
    budget_config = load_budget_config(Path(args.runtime_budget_config).resolve()) if args.runtime_budget_config else {}

    if args.subcommand == "write-current":
        if current_path is None:
            raise SystemExit("--runtime-current-manifest is required")
        host = host_fingerprint(args.runner_tag)
        entries = [
            parse_runtime_entry_spec(
                spec,
                host,
                budget_config=budget_config,
                test_counts=parse_count_specs(args.test_count),
            )
            for spec in args.entry
        ]
        manifest = synthesize_runtime_current_manifest(
            args.phase,
            artifact_root,
            entries,
            runtime_baseline_manifest=baseline_path,
            baseline_tag=args.baseline_tag,
            runner_tag=args.runner_tag,
        )
        write_runtime_manifest_outputs(current_path, manifest)
        print(str(current_path))
        return 0

    if args.subcommand == "promote-baseline":
        if current_path is None or baseline_path is None:
            raise SystemExit("--runtime-current-manifest and --runtime-baseline-manifest are required")
        current_manifest = read_json(current_path)
        baseline = promote_runtime_baseline(
            current_manifest,
            current_path,
            args.baseline_tag or f"{args.phase}-runtime-approved",
            args.require_acceptable_status,
        )
        write_runtime_manifest_outputs(baseline_path, baseline)
        print(str(baseline_path))
        return 0

    if args.subcommand == "refresh":
        if current_path is None or refresh_path is None:
            raise SystemExit("--runtime-current-manifest and --runtime-refresh-manifest are required")
        current_manifest = read_json(current_path)
        baseline_manifest = read_json(baseline_path) if baseline_path is not None and baseline_path.exists() else None
        current_manifest = apply_synthetic_runtime_mutations(
            current_manifest,
            parse_inflate_specs(args.synthetic_inflate),
            args.synthetic_fingerprint_mismatch,
        )
        refresh = refresh_runtime_manifest(baseline_manifest, current_manifest, baseline_path, current_path)
        refresh["runtime_severity"] = determine_runtime_severity(refresh)
        write_runtime_refresh_outputs(refresh_path, refresh)
        print(str(refresh_path))
        return 0

    if args.subcommand == "plan-rerun":
        if refresh_path is None or rerun_path is None or current_path is None:
            raise SystemExit("--runtime-refresh-manifest, --runtime-current-manifest, and --runtime-rerun-plan are required")
        refresh_manifest = read_json(refresh_path)
        plan = build_runtime_rerun_plan(refresh_manifest, baseline_path, current_path, refresh_path)
        write_runtime_rerun_plan_outputs(rerun_path, plan)
        print(str(rerun_path))
        return 0

    raise SystemExit(f"unsupported subcommand: {args.subcommand}")


if __name__ == "__main__":
    raise SystemExit(main())
