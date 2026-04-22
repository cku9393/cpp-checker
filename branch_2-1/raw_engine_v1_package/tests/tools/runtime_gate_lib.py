#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import argparse
import json
import os
import platform
import re
import shutil
import statistics
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


STATUS_OK = "OK"
STATUS_WARN = "WARN"
STATUS_FAIL = "FAIL"
STATUS_INFO = "INFO_ONLY"

FRESHNESS_FRESH = "FRESH"
FRESHNESS_STALE = "STALE"
FRESHNESS_REQUIRES_RERUN = "REQUIRES_RERUN"
FRESHNESS_NOT_COMPARABLE = "NOT_COMPARABLE"
FRESHNESS_REBASELINE_REQUIRED = "REBASELINE_REQUIRED"
FRESHNESS_INFO_ONLY = "INFO_ONLY"

COMPARABLE = "COMPARABLE"
NOT_COMPARABLE = "NOT_COMPARABLE"
REBASELINE_REQUIRED = "REBASELINE_REQUIRED"
INFO_ONLY = "INFO_ONLY"
INFO_ONLY = "INFO_ONLY"

VERDICT_PASS = "PASS"
VERDICT_WARN = "WARN"
VERDICT_FAIL = "FAIL"

REGISTRY_STATUS_ACTIVE = "active"
REGISTRY_STATUS_RETIRED = "retired"

TREND_STABLE = "stable"
TREND_NOISY = "noisy"
TREND_REGRESSING = "regressing"
TREND_IMPROVED = "improved"
TREND_INSUFFICIENT = "insufficient_history"

ROLE_PRODUCTION_CRITICAL = "production_critical"
ROLE_DIAGNOSTIC = "diagnostic"
ROLE_OPERATOR = "operator"

WATCH_CLEAR = "CLEAR"
WATCH_WATCH = "WATCH"
WATCH_STABLE = "WATCH_STABLE"
WATCH_ESCALATE = "WATCH_ESCALATE"
WATCH_REBASELINE_CANDIDATE = "REBASELINE_CANDIDATE"
WATCH_REBASELINE_REQUIRED = "REBASELINE_REQUIRED"
WATCH_FAIL = "FAIL"


def timestamp_utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def sha256_file(path: Path | None) -> str | None:
    if path is None or not path.exists() or not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalized_evidence_source(value: str | None) -> str:
    normalized = str(value or "").strip().lower()
    if normalized in {"", "real", "same_fingerprint", "imported_real"}:
        return "real"
    if normalized in {"fixture", "matrix_fixture", "imported_fixture"}:
        return "fixture"
    if normalized in {"replay", "imported_replay"}:
        return "replay"
    return normalized or "real"


def read_json(path: Path) -> dict[str, Any]:
    last_error: Exception | None = None
    for attempt in range(60):
        try:
            return json.loads(path.read_text(encoding="utf-8"))
        except (FileNotFoundError, json.JSONDecodeError) as exc:
            last_error = exc
            if attempt == 59:
                raise
            time.sleep(0.1)
    raise last_error if last_error is not None else RuntimeError(f"failed to read json: {path}")


def atomic_write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = path.with_name(f".{path.name}.tmp.{os.getpid()}.{time.time_ns()}")
    temp_path.write_text(text, encoding="utf-8")
    if not temp_path.exists():
        # Rare file-provider/tmp races should not make artifact generation fail.
        path.write_text(text, encoding="utf-8")
        return
    try:
        os.replace(temp_path, path)
    except FileNotFoundError:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")


def write_json(path: Path, data: dict[str, Any]) -> None:
    atomic_write_text(path, json.dumps(data, indent=2) + "\n")


def write_text(path: Path, text: str) -> None:
    atomic_write_text(path, text)


def normalize_manifest_path(path_value: str | Path | None) -> Path | None:
    if path_value is None:
        return None
    path = Path(path_value).resolve()
    return path if path.suffix == ".json" else path.with_suffix(".json")


def load_runtime_budget_config(path_value: str | Path | None) -> dict[str, dict[str, float]]:
    out: dict[str, dict[str, float]] = {}
    profile = load_runtime_budget_profile(path_value)
    for execution_class, entry in profile.get("entries", {}).items():
        raw_threshold = dict(entry.get("thresholds", {}))
        out[str(execution_class)] = {
            "soft_seconds": float(raw_threshold.get("soft_seconds", raw_threshold.get("softSeconds", 0.0))),
            "hard_seconds": float(raw_threshold.get("hard_seconds", raw_threshold.get("hardSeconds", 0.0))),
            "soft_delta_percent": float(
                raw_threshold.get("soft_delta_percent", raw_threshold.get("softDeltaPercent", 0.0))
            ),
            "hard_delta_percent": float(
                raw_threshold.get("hard_delta_percent", raw_threshold.get("hardDeltaPercent", 0.0))
            ),
            "soft_delta_floor_sec": float(
                raw_threshold.get("soft_delta_floor_sec", raw_threshold.get("softDeltaFloorSec", 0.0))
            ),
            "hard_delta_floor_sec": float(
                raw_threshold.get("hard_delta_floor_sec", raw_threshold.get("hardDeltaFloorSec", 0.0))
            ),
        }
    return out


def toolchain_fingerprint_hash(toolchain: dict[str, Any]) -> str:
    return sha256_text(
        "|".join(
            [
                str(toolchain.get("compiler_command", "")),
                str(toolchain.get("compiler_id", "")),
                str(toolchain.get("compiler_version", "")),
            ]
        )
    )


def runtime_execution_classes(manifest: dict[str, Any]) -> list[str]:
    return sorted(
        {
            str(entry.get("execution_class", "")).strip()
            for entry in manifest.get("entries", [])
            if isinstance(entry, dict) and str(entry.get("execution_class", "")).strip()
        }
    )


def runtime_execution_signatures(manifest: dict[str, Any]) -> list[str]:
    signatures: list[str] = []
    for entry in manifest.get("entries", []):
        if not isinstance(entry, dict):
            continue
        execution_class = str(entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        signatures.append(
            "|".join(
                [
                    execution_class,
                    str(entry.get("build_type", "")).strip(),
                    str(entry.get("sanitizer_flags", "")).strip(),
                ]
            )
        )
    return sorted(signatures)


def runtime_execution_signature_map(manifest: dict[str, Any]) -> dict[str, str]:
    signature_map: dict[str, str] = {}
    for entry in manifest.get("entries", []):
        if not isinstance(entry, dict):
            continue
        execution_class = str(entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        signature_map[execution_class] = "|".join(
            [
                str(entry.get("build_type", "")).strip(),
                str(entry.get("sanitizer_flags", "")).strip(),
            ]
        )
    return signature_map


def runtime_fingerprint_key(
    host_fingerprint_data: dict[str, Any],
    toolchain_fingerprint_data: dict[str, Any],
    execution_signatures: list[str],
) -> str:
    return sha256_text(
        "|".join(
            [
                str(host_fingerprint_data.get("fingerprint_hash", "")),
                toolchain_fingerprint_hash(toolchain_fingerprint_data),
                ",".join(sorted(execution_signatures)),
            ]
        )
    )


def normalize_runtime_registry_path(path_value: str | Path | None) -> Path | None:
    if path_value is None:
        return None
    path = Path(path_value).resolve()
    return path if path.suffix == ".json" else path.with_suffix(".json")


def normalize_runtime_budget_registry_path(path_value: str | Path | None) -> Path | None:
    return normalize_runtime_registry_path(path_value)


def default_runtime_selection_path(current_manifest_path: Path) -> Path:
    return current_manifest_path.with_name(f"{current_manifest_path.stem}_baseline_selection.json")


def runtime_registry_version_for_path(path: Path | None) -> str:
    if path is not None and "_v2" in path.stem:
        return "runtime_baseline_registry_v2"
    return "runtime_baseline_registry_v1"


def runtime_registry_summary_payload(registry: dict[str, Any]) -> dict[str, Any]:
    entries = [dict(entry) for entry in registry.get("entries", []) if isinstance(entry, dict)]
    active_entries = [entry for entry in entries if str(entry.get("status", REGISTRY_STATUS_RETIRED)) == REGISTRY_STATUS_ACTIVE]
    active_by_fingerprint: dict[str, list[dict[str, Any]]] = {}
    lineage_history_by_fingerprint: dict[str, dict[str, Any]] = {}
    for entry in active_entries:
        fingerprint_key = str(entry.get("runtime_fingerprint_key", entry.get("fingerprint_key", ""))).strip()
        if not fingerprint_key:
            continue
        active_by_fingerprint.setdefault(fingerprint_key, []).append(entry)
    for entry in entries:
        fingerprint_key = str(entry.get("runtime_fingerprint_key", entry.get("fingerprint_key", ""))).strip()
        if not fingerprint_key:
            continue
        bucket = lineage_history_by_fingerprint.setdefault(
            fingerprint_key,
            {
                "active_count": 0,
                "retired_count": 0,
                "entries": [],
            },
        )
        status = str(entry.get("status", REGISTRY_STATUS_RETIRED))
        if status == REGISTRY_STATUS_ACTIVE:
            bucket["active_count"] += 1
        elif status == REGISTRY_STATUS_RETIRED:
            bucket["retired_count"] += 1
        bucket["entries"].append(
            {
                "baseline_id": entry.get("baseline_id"),
                "baseline_tag": entry.get("baseline_tag"),
                "status": status,
                "approval_timestamp_utc": entry.get("approval_timestamp_utc"),
                "previous_active_baseline_id": entry.get("previous_active_baseline_id"),
                "supersedes_baseline_ids": entry.get("supersedes_baseline_ids", []),
                "superseded_by_baseline_id": entry.get("superseded_by_baseline_id"),
                "retired_reason": entry.get("retired_reason"),
                "runtime_baseline_manifest_path": entry.get("runtime_baseline_manifest_path"),
            }
        )
    for bucket in lineage_history_by_fingerprint.values():
        bucket["entries"] = sorted(bucket["entries"], key=runtime_selection_sort_key, reverse=True)
    return {
        "manifest_version": "runtime_baseline_registry_summary_v2",
        "generated_at_utc": registry.get("generated_at_utc", timestamp_utc_now()),
        "registry_version": registry.get("registry_version", "runtime_baseline_registry_v1"),
        "entry_count": len(entries),
        "active_entry_count": len(active_entries),
        "retired_entry_count": sum(
            1 for entry in entries if str(entry.get("status", REGISTRY_STATUS_ACTIVE)) == REGISTRY_STATUS_RETIRED
        ),
        "fingerprint_count": len(
            {
                str(entry.get("runtime_fingerprint_key", entry.get("fingerprint_key", "")))
                for entry in entries
                if str(entry.get("runtime_fingerprint_key", entry.get("fingerprint_key", ""))).strip()
            }
        ),
        "active_fingerprint_count": len(active_by_fingerprint),
        "active_baselines_by_fingerprint": {
            fingerprint_key: [
                {
                    "baseline_id": item.get("baseline_id"),
                    "baseline_tag": item.get("baseline_tag"),
                    "status": item.get("status"),
                    "runtime_baseline_manifest_path": item.get("runtime_baseline_manifest_path"),
                }
                for item in sorted(items, key=runtime_selection_sort_key, reverse=True)
            ]
            for fingerprint_key, items in sorted(active_by_fingerprint.items())
        },
        "lineage_history_by_fingerprint": {
            fingerprint_key: lineage_history_by_fingerprint[fingerprint_key]
            for fingerprint_key in sorted(lineage_history_by_fingerprint)
        },
        "active_baseline_ids": [str(entry.get("baseline_id", "")) for entry in active_entries if str(entry.get("baseline_id", "")).strip()],
    }


def default_runtime_history_summary_path(history_index_path: Path) -> Path:
    return history_index_path.with_name(f"{history_index_path.stem}_summary.json")


def default_runtime_proposal_path(current_manifest_path: Path) -> Path:
    return current_manifest_path.with_name(f"{current_manifest_path.stem}_rebaseline_proposal.json")


def default_runtime_proposal_gate_path(current_manifest_path: Path) -> Path:
    return current_manifest_path.with_name(f"{current_manifest_path.stem}_proposal_gate.json")


def default_runtime_approval_metadata_path(baseline_manifest_path: Path) -> Path:
    return baseline_manifest_path.with_name(f"{baseline_manifest_path.stem}_approval_metadata.json")


def default_runtime_budget_current_path(current_manifest_path: Path) -> Path:
    stem = current_manifest_path.stem
    if stem.startswith("policy_runtime_current_"):
        return current_manifest_path.with_name(f"runtime_budget_current_{stem.removeprefix('policy_runtime_current_')}.json")
    return current_manifest_path.with_name(f"{stem}_budget_current.json")


def default_runtime_budget_refresh_path(current_manifest_path: Path) -> Path:
    stem = current_manifest_path.stem
    if stem.startswith("policy_runtime_current_"):
        return current_manifest_path.with_name(f"runtime_budget_refresh_{stem.removeprefix('policy_runtime_current_')}.json")
    return current_manifest_path.with_name(f"{stem}_budget_refresh.json")


def default_runtime_budget_rerun_path(current_manifest_path: Path) -> Path:
    stem = current_manifest_path.stem
    if stem.startswith("policy_runtime_current_"):
        return current_manifest_path.with_name(f"runtime_budget_rerun_{stem.removeprefix('policy_runtime_current_')}.json")
    return current_manifest_path.with_name(f"{stem}_budget_rerun.json")


def default_runtime_budget_proposal_path(current_manifest_path: Path) -> Path:
    stem = current_manifest_path.stem
    if stem.startswith("policy_runtime_current_"):
        return current_manifest_path.with_name(f"runtime_budget_proposal_{stem.removeprefix('policy_runtime_current_')}.json")
    return current_manifest_path.with_name(f"{stem}_budget_proposal.json")


def default_runtime_budget_proposal_gate_path(current_manifest_path: Path) -> Path:
    stem = current_manifest_path.stem
    if stem.startswith("policy_runtime_current_"):
        return current_manifest_path.with_name(f"runtime_budget_proposal_gate_{stem.removeprefix('policy_runtime_current_')}.json")
    return current_manifest_path.with_name(f"{stem}_budget_proposal_gate.json")


def runtime_selection_manifest_text(selection: dict[str, Any]) -> str:
    return (
        "\n".join(
            [
                f"selection_version={selection.get('selection_version', '')}",
                f"comparability_verdict={selection.get('comparability_verdict', '')}",
                f"selected_baseline_id={selection.get('selected_baseline_id', '')}",
                f"selected_baseline_tag={selection.get('selected_baseline_tag', '')}",
                f"candidate_count={selection.get('candidate_count', 0)}",
                f"exact_match_count={selection.get('exact_match_count', 0)}",
                f"compatible_match_count={selection.get('compatible_match_count', 0)}",
                f"retired_match_count={selection.get('retired_match_count', 0)}",
                f"comparability_reason={selection.get('comparability_reason', '')}",
            ]
        )
        + "\n"
    )


def runtime_selection_manifest_summary(selection: dict[str, Any]) -> str:
    return (
        "runtime_baseline_selection_summary"
        + f" comparability_verdict={selection.get('comparability_verdict', '')}"
        + f" selected_baseline_id={selection.get('selected_baseline_id', '')}"
        + f" candidate_count={selection.get('candidate_count', 0)}"
        + f" exact_match_count={selection.get('exact_match_count', 0)}"
        + f" compatible_match_count={selection.get('compatible_match_count', 0)}"
        + f" retired_match_count={selection.get('retired_match_count', 0)}"
        + f" reason={selection.get('comparability_reason', '')}\n"
    )


def write_runtime_selection_outputs(json_path: Path, selection: dict[str, Any]) -> None:
    write_json(json_path, selection)
    write_text(json_path.with_suffix(".txt"), runtime_selection_manifest_text(selection))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_selection_manifest_summary(selection))


def detect_compiler() -> dict[str, str]:
    compiler_command = os.environ.get("CXX") or shutil.which("c++") or shutil.which("clang++") or shutil.which("g++") or ""
    compiler_id = "unknown"
    compiler_version = "unknown"
    if compiler_command:
        try:
            completed = subprocess.run(
                [compiler_command, "--version"],
                capture_output=True,
                text=True,
                check=False,
            )
            first_line = (completed.stdout or completed.stderr).splitlines()[0] if (completed.stdout or completed.stderr) else ""
            lowered = first_line.lower()
            if "apple clang" in lowered:
                compiler_id = "appleclang"
            elif "clang" in lowered:
                compiler_id = "clang"
            elif "gcc" in lowered or "g++" in lowered:
                compiler_id = "gcc"
            compiler_version = first_line.strip() or "unknown"
        except (OSError, IndexError):
            pass
    return {
        "compiler_command": compiler_command,
        "compiler_id": compiler_id,
        "compiler_version": compiler_version,
    }


def host_fingerprint(runner_tag: str = "") -> dict[str, str]:
    compiler = detect_compiler()
    data = {
        "os": platform.system(),
        "os_release": platform.release(),
        "arch": platform.machine(),
        "runner_tag": runner_tag,
        **compiler,
    }
    data["fingerprint_hash"] = sha256_text(
        "|".join(
            [
                data["os"],
                data["os_release"],
                data["arch"],
                data["runner_tag"],
                data["compiler_id"],
                data["compiler_version"],
            ]
        )
    )
    return data


def default_runtime_threshold(execution_class: str) -> dict[str, float]:
    if execution_class == "release_full":
        return {"soft_seconds": 260.0, "hard_seconds": 520.0, "soft_delta_percent": 25.0, "hard_delta_percent": 60.0, "soft_delta_floor_sec": 0.0, "hard_delta_floor_sec": 0.0}
    if execution_class == "debug_full":
        return {"soft_seconds": 900.0, "hard_seconds": 1800.0, "soft_delta_percent": 25.0, "hard_delta_percent": 60.0, "soft_delta_floor_sec": 0.0, "hard_delta_floor_sec": 0.0}
    if execution_class == "asan_full":
        return {"soft_seconds": 2400.0, "hard_seconds": 4200.0, "soft_delta_percent": 25.0, "hard_delta_percent": 60.0, "soft_delta_floor_sec": 0.0, "hard_delta_floor_sec": 0.0}
    if execution_class in {"policy_core", "policy_refresh"}:
        return {"soft_seconds": 30.0, "hard_seconds": 90.0, "soft_delta_percent": 25.0, "hard_delta_percent": 75.0, "soft_delta_floor_sec": 0.25, "hard_delta_floor_sec": 0.5}
    if execution_class in {"policy_nightly", "compare_campaign"}:
        return {"soft_seconds": 180.0, "hard_seconds": 600.0, "soft_delta_percent": 25.0, "hard_delta_percent": 75.0, "soft_delta_floor_sec": 0.5, "hard_delta_floor_sec": 1.0}
    return {"soft_seconds": 60.0, "hard_seconds": 180.0, "soft_delta_percent": 25.0, "hard_delta_percent": 75.0, "soft_delta_floor_sec": 0.0, "hard_delta_floor_sec": 0.0}


def default_runtime_role(execution_class: str) -> str:
    if execution_class in {"release_full", "debug_full"}:
        return ROLE_PRODUCTION_CRITICAL
    if execution_class in {"asan_full", "compare_campaign"}:
        return ROLE_DIAGNOSTIC
    return ROLE_OPERATOR


def default_runtime_watch_policy(execution_class: str, role: str) -> dict[str, float | int]:
    base = {
        "min_samples": 3,
        "clear_window": 2,
        "stable_overrun_threshold": 3,
        "rebaseline_candidate_threshold": 5,
        "max_bounded_jitter_percent": 15.0,
        "near_baseline_delta_percent": 5.0,
    }
    if role == ROLE_DIAGNOSTIC:
        base["rebaseline_candidate_threshold"] = 6
        base["max_bounded_jitter_percent"] = 20.0
    elif role == ROLE_OPERATOR:
        base["max_bounded_jitter_percent"] = 30.0
    elif role == ROLE_PRODUCTION_CRITICAL:
        base["stable_overrun_threshold"] = 2
        base["rebaseline_candidate_threshold"] = 4
        base["max_bounded_jitter_percent"] = 12.0
    if execution_class == "asan_full":
        base["rebaseline_candidate_threshold"] = 5
    return base


def known_runtime_execution_classes() -> list[str]:
    return [
        "release_full",
        "debug_full",
        "asan_full",
        "policy_core",
        "policy_refresh",
        "policy_nightly",
        "compare_campaign",
    ]


def load_runtime_budget_profile(
    path_value: str | Path | dict[str, Any] | None,
    execution_classes: list[str] | None = None,
) -> dict[str, Any]:
    classes = sorted(set(execution_classes or known_runtime_execution_classes()))
    raw: dict[str, Any] = {}
    path: Path | None = None
    if isinstance(path_value, dict):
        raw = json.loads(json.dumps(path_value))
    elif path_value is not None:
        path = Path(path_value).resolve()
        raw = read_json(path)

    raw_entries = raw.get("entries", raw.get("execution_classes", {}))
    if not isinstance(raw_entries, dict):
        raw_entries = {}
    profile_id = str(raw.get("profile_id", path.stem if path is not None else "runtime-budget-profile-v1"))
    version = int(raw.get("version", 1))
    created_at = str(raw.get("created_at", timestamp_utc_now()))

    profile_entries: dict[str, Any] = {}
    for execution_class in classes:
        entry_raw = raw_entries.get(execution_class, {})
        if not isinstance(entry_raw, dict):
            entry_raw = {}
        nested_thresholds = entry_raw.get("thresholds", {})
        if not isinstance(nested_thresholds, dict):
            nested_thresholds = {}
        role = str(entry_raw.get("role", default_runtime_role(execution_class)))
        thresholds = {
            "soft_seconds": float(
                entry_raw.get(
                    "soft_seconds",
                    entry_raw.get(
                        "softSeconds",
                        nested_thresholds.get("soft_seconds", nested_thresholds.get("softSeconds", default_runtime_threshold(execution_class)["soft_seconds"])),
                    ),
                )
            ),
            "hard_seconds": float(
                entry_raw.get(
                    "hard_seconds",
                    entry_raw.get(
                        "hardSeconds",
                        nested_thresholds.get("hard_seconds", nested_thresholds.get("hardSeconds", default_runtime_threshold(execution_class)["hard_seconds"])),
                    ),
                )
            ),
            "soft_delta_percent": float(
                entry_raw.get(
                    "soft_delta_percent",
                    entry_raw.get(
                        "softDeltaPercent",
                        nested_thresholds.get("soft_delta_percent", nested_thresholds.get("softDeltaPercent", default_runtime_threshold(execution_class)["soft_delta_percent"])),
                    ),
                )
            ),
            "hard_delta_percent": float(
                entry_raw.get(
                    "hard_delta_percent",
                    entry_raw.get(
                        "hardDeltaPercent",
                        nested_thresholds.get("hard_delta_percent", nested_thresholds.get("hardDeltaPercent", default_runtime_threshold(execution_class)["hard_delta_percent"])),
                    ),
                )
            ),
            "soft_delta_floor_sec": float(
                entry_raw.get(
                    "soft_delta_floor_sec",
                    entry_raw.get(
                        "softDeltaFloorSec",
                        nested_thresholds.get("soft_delta_floor_sec", nested_thresholds.get("softDeltaFloorSec", default_runtime_threshold(execution_class)["soft_delta_floor_sec"])),
                    ),
                )
            ),
            "hard_delta_floor_sec": float(
                entry_raw.get(
                    "hard_delta_floor_sec",
                    entry_raw.get(
                        "hardDeltaFloorSec",
                        nested_thresholds.get("hard_delta_floor_sec", nested_thresholds.get("hardDeltaFloorSec", default_runtime_threshold(execution_class)["hard_delta_floor_sec"])),
                    ),
                )
            ),
        }
        watch_policy = dict(default_runtime_watch_policy(execution_class, role))
        raw_watch_policy = entry_raw.get("watch_policy", {})
        if isinstance(raw_watch_policy, dict):
            for key, value in raw_watch_policy.items():
                watch_policy[str(key)] = value
        profile_entries[execution_class] = {
            "execution_class": execution_class,
            "role": role,
            "thresholds": thresholds,
            "watch_policy": watch_policy,
        }

    profile = {
        "profile_version": "runtime_budget_profile_v1",
        "profile_id": profile_id,
        "version": version,
        "created_at": created_at,
        "entries": profile_entries,
    }
    profile["profile_hash"] = sha256_text(json.dumps(profile, sort_keys=True))
    return profile


def runtime_budget_profile_entry(profile: dict[str, Any] | None, execution_class: str) -> dict[str, Any]:
    if not profile:
        role = default_runtime_role(execution_class)
        return {
            "execution_class": execution_class,
            "role": role,
            "thresholds": default_runtime_threshold(execution_class),
            "watch_policy": default_runtime_watch_policy(execution_class, role),
        }
    entry = dict(profile.get("entries", {}).get(execution_class, {}))
    if not entry:
        role = default_runtime_role(execution_class)
        return {
            "execution_class": execution_class,
            "role": role,
            "thresholds": default_runtime_threshold(execution_class),
            "watch_policy": default_runtime_watch_policy(execution_class, role),
        }
    return entry


def runtime_budget_profile_for_manifest(
    manifest: dict[str, Any],
    path_value: str | Path | dict[str, Any] | None = None,
) -> dict[str, Any]:
    profile_source: str | Path | dict[str, Any] | None = path_value
    if profile_source is None:
        embedded_profile = manifest.get("runtime_budget_profile")
        if not isinstance(embedded_profile, dict) or not embedded_profile:
            embedded_profile = manifest.get("budget_profile")
        if isinstance(embedded_profile, dict) and embedded_profile:
            profile_source = embedded_profile
    return load_runtime_budget_profile(profile_source, runtime_execution_classes(manifest))


def default_runtime_budget_profile_output_path(watch_manifest_path: Path) -> Path:
    stem = re.sub(r"[^A-Za-z0-9_.-]+", "_", watch_manifest_path.stem).strip("_")
    if stem:
        return watch_manifest_path.with_name(f"runtime_budget_profile_{stem}.json")
    return watch_manifest_path.with_name("runtime_budget_profile_v1.json")


def runtime_budget_profile_text(profile: dict[str, Any]) -> str:
    lines = [
        f"profile_version={profile.get('profile_version', '')}",
        f"profile_id={profile.get('profile_id', '')}",
        f"version={profile.get('version', 0)}",
        f"created_at={profile.get('created_at', '')}",
    ]
    for execution_class, entry in sorted(dict(profile.get("entries", {})).items()):
        thresholds = dict(entry.get("thresholds", {}))
        lines.append(
            "runtime_budget_profile_entry="
            + f"execution_class={execution_class}"
            + f" role={entry.get('role', '')}"
            + f" soft_seconds={thresholds.get('soft_seconds', 0.0)}"
            + f" hard_seconds={thresholds.get('hard_seconds', 0.0)}"
        )
    return "\n".join(lines) + "\n"


def runtime_budget_profile_summary(profile: dict[str, Any]) -> str:
    return (
        "runtime_budget_profile_summary"
        + f" profile_id={profile.get('profile_id', '')}"
        + f" version={profile.get('version', 0)}"
        + f" execution_class_count={len(profile.get('entries', {}))}\n"
    )


def write_runtime_budget_profile_outputs(json_path: Path, profile: dict[str, Any]) -> None:
    write_json(json_path, profile)
    write_text(json_path.with_suffix(".txt"), runtime_budget_profile_text(profile))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_budget_profile_summary(profile))


def budget_confidence_rank(value: str) -> int:
    return {"LOW": 0, "MEDIUM": 1, "HIGH": 2}.get(str(value).strip().upper(), 0)


def classify_runtime_budget_confidence(entry: dict[str, Any]) -> tuple[str, str]:
    role = str(entry.get("execution_role", entry.get("role", ROLE_OPERATOR))).strip() or ROLE_OPERATOR
    sample_count = int(entry.get("sample_count", 0))
    real_sample_count = int(entry.get("real_sample_count", 0))
    stable_overrun_count = int(entry.get("stable_overrun_count", 0))
    hard_over_budget_count = int(entry.get("hard_over_budget_count", 0))
    jitter_estimate_percent = float(entry.get("jitter_estimate_percent", 0.0))
    history_depth = max(sample_count, int(entry.get("history_depth", sample_count)))
    high_real_required = {
        ROLE_PRODUCTION_CRITICAL: 5,
        ROLE_DIAGNOSTIC: 8,
        ROLE_OPERATOR: 3,
    }.get(role, 5)
    medium_real_required = {
        ROLE_PRODUCTION_CRITICAL: 2,
        ROLE_DIAGNOSTIC: 3,
        ROLE_OPERATOR: 1,
    }.get(role, 1)
    bounded_jitter_limit = {
        ROLE_PRODUCTION_CRITICAL: 12.0,
        ROLE_DIAGNOSTIC: 20.0,
        ROLE_OPERATOR: 30.0,
    }.get(role, 15.0)
    bounded_jitter = jitter_estimate_percent <= bounded_jitter_limit
    if hard_over_budget_count > 0:
        if real_sample_count >= medium_real_required:
            return "HIGH", f"{role} budget evidence includes repeated same-fingerprint hard breaches"
        return "MEDIUM", f"{role} budget evidence includes hard breaches but repeated real samples are still limited"
    if (
        real_sample_count >= high_real_required
        and sample_count >= high_real_required
        and history_depth >= high_real_required
        and bounded_jitter
    ):
        if stable_overrun_count > 0:
            return "HIGH", f"{role} budget evidence is backed by repeated real samples with stable overrun depth {stable_overrun_count}"
        return "HIGH", f"{role} budget evidence is backed by repeated same-fingerprint real samples with bounded jitter"
    if real_sample_count >= medium_real_required:
        return "MEDIUM", f"{role} budget evidence is real-observed but history depth is not yet high-confidence"
    return "LOW", f"{role} budget evidence is still sparse"


def empty_runtime_budget_registry() -> dict[str, Any]:
    return {
        "registry_version": "runtime_budget_registry_v1",
        "generated_at_utc": timestamp_utc_now(),
        "entries": [],
        "active_entry_count": 0,
        "retired_entry_count": 0,
    }


def finalize_runtime_budget_registry(registry: dict[str, Any]) -> dict[str, Any]:
    registry["generated_at_utc"] = timestamp_utc_now()
    registry["active_entry_count"] = sum(
        1 for entry in registry.get("entries", []) if str(entry.get("status", REGISTRY_STATUS_RETIRED)) == REGISTRY_STATUS_ACTIVE
    )
    registry["retired_entry_count"] = sum(
        1 for entry in registry.get("entries", []) if str(entry.get("status", REGISTRY_STATUS_ACTIVE)) == REGISTRY_STATUS_RETIRED
    )
    registry["registry_hash"] = sha256_text(json.dumps(registry, sort_keys=True))
    return registry


def runtime_budget_registry_text(registry: dict[str, Any]) -> str:
    lines = [
        f"registry_version={registry.get('registry_version', '')}",
        f"active_entry_count={registry.get('active_entry_count', 0)}",
        f"retired_entry_count={registry.get('retired_entry_count', 0)}",
    ]
    for entry in registry.get("entries", []):
        lines.append(
            "runtime_budget_registry_entry="
            + f"profile_id={entry.get('profile_id', '')}"
            + f" budget_tag={entry.get('budget_tag', '')}"
            + f" status={entry.get('status', '')}"
            + f" execution_class_count={len(entry.get('execution_classes_covered', []))}"
        )
    return "\n".join(lines) + "\n"


def runtime_budget_registry_summary(registry: dict[str, Any]) -> str:
    return (
        "runtime_budget_registry_summary"
        + f" active_entry_count={registry.get('active_entry_count', 0)}"
        + f" retired_entry_count={registry.get('retired_entry_count', 0)}"
        + f" entry_count={len(registry.get('entries', []))}\n"
    )


def write_runtime_budget_registry_outputs(json_path: Path, registry: dict[str, Any]) -> None:
    write_json(json_path, registry)
    write_text(json_path.with_suffix(".txt"), runtime_budget_registry_text(registry))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_budget_registry_summary(registry))


def load_runtime_budget_registry(path: Path | None) -> dict[str, Any]:
    if path is None or not path.exists():
        return empty_runtime_budget_registry()
    data = read_json(path)
    if not isinstance(data.get("entries", []), list):
        raise RuntimeError(f"invalid runtime budget registry payload: {path}")
    return finalize_runtime_budget_registry(data)


def build_runtime_budget_registry_entry(
    approved_budget_profile: dict[str, Any],
    approved_budget_profile_path: Path,
    budget_tag: str,
    activate: bool,
) -> dict[str, Any]:
    host = dict(approved_budget_profile.get("host_fingerprint", {}))
    toolchain = dict(approved_budget_profile.get("toolchain_fingerprint", {}))
    execution_classes = sorted(str(value) for value in approved_budget_profile.get("execution_classes_covered", []) if str(value))
    if not execution_classes:
        execution_classes = sorted(str(key) for key in dict(approved_budget_profile.get("entries", {})).keys() if str(key))
    approval_timestamp = str(approved_budget_profile.get("approval_timestamp_utc") or timestamp_utc_now())
    profile_id = str(approved_budget_profile.get("profile_id", approved_budget_profile_path.stem) or approved_budget_profile_path.stem)
    return {
        "profile_id": profile_id,
        "version": approved_budget_profile.get("version"),
        "budget_tag": budget_tag,
        "approval_timestamp_utc": approval_timestamp,
        "runtime_budget_manifest_path": str(approved_budget_profile_path),
        "runtime_budget_manifest_hash": sha256_file(approved_budget_profile_path),
        "runtime_budget_profile_id": profile_id,
        "runtime_budget_profile_version": approved_budget_profile.get("version"),
        "host_fingerprint": host,
        "toolchain_fingerprint": toolchain,
        "host_fingerprint_hash": str(host.get("fingerprint_hash", "")),
        "toolchain_fingerprint_hash": toolchain_fingerprint_hash(toolchain),
        "execution_classes_covered": execution_classes,
        "role_counts": dict(approved_budget_profile.get("role_counts", {})),
        "source_runtime_current_manifest_path": approved_budget_profile.get("source_runtime_current_manifest_path"),
        "source_runtime_baseline_manifest_path": approved_budget_profile.get("source_runtime_baseline_manifest_path"),
        "status": REGISTRY_STATUS_ACTIVE if activate else REGISTRY_STATUS_RETIRED,
    }


def promote_runtime_budget_registry(
    registry: dict[str, Any],
    approved_budget_profile: dict[str, Any],
    approved_budget_profile_path: Path,
    budget_tag: str,
    activate: bool,
) -> tuple[dict[str, Any], dict[str, Any]]:
    entry = build_runtime_budget_registry_entry(approved_budget_profile, approved_budget_profile_path, budget_tag, activate)
    existing = None
    for candidate in registry.get("entries", []):
        if (
            str(candidate.get("runtime_budget_manifest_hash", "")) == str(entry.get("runtime_budget_manifest_hash", ""))
            and str(candidate.get("profile_id", "")) == str(entry.get("profile_id", ""))
        ):
            existing = candidate
            break
    if existing is not None:
        existing.update(entry)
        entry = existing
    else:
        registry.setdefault("entries", []).append(entry)

    if activate:
        for candidate in registry.get("entries", []):
            if candidate is entry:
                continue
            if str(candidate.get("status", REGISTRY_STATUS_RETIRED)) == REGISTRY_STATUS_ACTIVE:
                candidate["status"] = REGISTRY_STATUS_RETIRED
                candidate["retired_timestamp_utc"] = timestamp_utc_now()
                candidate["retired_reason"] = f"superseded by {entry['profile_id']}"
    entry["status"] = REGISTRY_STATUS_ACTIVE if activate else str(entry.get("status", REGISTRY_STATUS_RETIRED))
    return finalize_runtime_budget_registry(registry), entry


def active_runtime_budget_profile_path(registry: dict[str, Any]) -> Path | None:
    active_entries = [
        dict(entry)
        for entry in registry.get("entries", [])
        if str(entry.get("status", REGISTRY_STATUS_RETIRED)) == REGISTRY_STATUS_ACTIVE
    ]
    if not active_entries:
        return None
    selected = max(
        active_entries,
        key=lambda item: (
            str(item.get("approval_timestamp_utc", "")),
            str(item.get("profile_id", "")),
        ),
    )
    path_text = str(selected.get("runtime_budget_manifest_path", "")).strip()
    if not path_text:
        return None
    candidate = Path(path_text).resolve()
    return candidate if candidate.exists() else None


def history_samples_for_execution_class(
    history_index: dict[str, Any],
    current_manifest: dict[str, Any],
    execution_class: str,
) -> list[dict[str, Any]]:
    bucket = history_bucket_for_manifest(history_index, current_manifest)
    payload = dict(bucket.get("execution_classes", {})).get(execution_class, {})
    return [dict(sample) for sample in payload.get("samples", []) if isinstance(sample, dict)]


def proposed_runtime_budget_thresholds(
    execution_class: str,
    current_entry: dict[str, Any],
    watch_entry: dict[str, Any],
    budget_profile_entry: dict[str, Any],
) -> dict[str, float]:
    thresholds = dict(budget_profile_entry.get("thresholds", default_runtime_threshold(execution_class)))
    observed_wall = max(
        float(current_entry.get("wall_time_sec", 0.0)),
        float(watch_entry.get("rolling_median_wall_time_sec", current_entry.get("wall_time_sec", 0.0))),
        float(watch_entry.get("rolling_p95_wall_time_sec", current_entry.get("wall_time_sec", 0.0))),
    )
    proposed_soft = max(
        float(thresholds.get("soft_seconds", 0.0)),
        round(max(observed_wall * 1.1, observed_wall + max(5.0, observed_wall * 0.05)), 3),
    )
    proposed_hard = max(
        float(thresholds.get("hard_seconds", 0.0)),
        round(max(proposed_soft * 1.6, proposed_soft + 60.0), 3),
    )
    return {
        "soft_seconds": proposed_soft,
        "hard_seconds": proposed_hard,
        "soft_delta_percent": float(thresholds.get("soft_delta_percent", 0.0)),
        "hard_delta_percent": float(thresholds.get("hard_delta_percent", 0.0)),
        "soft_delta_floor_sec": float(thresholds.get("soft_delta_floor_sec", 0.0)),
        "hard_delta_floor_sec": float(thresholds.get("hard_delta_floor_sec", 0.0)),
    }


def apply_budget_profile_to_runtime_manifest(
    current_manifest: dict[str, Any],
    budget_profile: dict[str, Any],
) -> dict[str, Any]:
    mutated = json.loads(json.dumps(current_manifest))
    mutated["runtime_budget_profile_id"] = budget_profile.get("profile_id")
    mutated["runtime_budget_profile_version"] = budget_profile.get("version")
    mutated["runtime_budget_profile"] = {
        "profile_id": budget_profile.get("profile_id"),
        "version": budget_profile.get("version"),
        "created_at": budget_profile.get("created_at"),
        "entries": {
            execution_class: runtime_budget_profile_entry(budget_profile, execution_class)
            for execution_class in runtime_execution_classes(mutated)
        },
    }
    warn_count = 0
    fail_count = 0
    for entry in mutated.get("entries", []):
        if not isinstance(entry, dict):
            continue
        execution_class = str(entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        budget_entry = runtime_budget_profile_entry(budget_profile, execution_class)
        thresholds = dict(budget_entry.get("thresholds", default_runtime_threshold(execution_class)))
        role = str(budget_entry.get("role", default_runtime_role(execution_class)))
        watch_policy = dict(budget_entry.get("watch_policy", default_runtime_watch_policy(execution_class, role)))
        entry["budget_thresholds"] = thresholds
        entry["execution_role"] = role
        entry["watch_policy"] = watch_policy
        entry["runtime_budget_profile_id"] = budget_profile.get("profile_id")
        entry["runtime_budget_profile_version"] = budget_profile.get("version")
        status, delta_percent, rationale = entry_status(float(entry.get("wall_time_sec", 0.0)), thresholds, None)
        entry["current_status"] = status
        entry["delta_percent"] = delta_percent
        entry["rationale"] = rationale
        if status == STATUS_WARN:
            warn_count += 1
        elif status == STATUS_FAIL:
            fail_count += 1
    mutated["warn_count"] = warn_count
    mutated["fail_count"] = fail_count
    mutated["overall_status"] = STATUS_FAIL if fail_count else STATUS_WARN if warn_count else STATUS_OK
    mutated["current_verdict"] = VERDICT_FAIL if fail_count else VERDICT_WARN if warn_count else VERDICT_PASS
    mutated["overall_budget_verdict"] = "BUDGET_FAIL" if fail_count else "BUDGET_WARN" if warn_count else "PASS"
    mutated["current_runtime_manifest_hash"] = manifest_hash_without_field(mutated, "current_runtime_manifest_hash")
    return mutated


def build_runtime_budget_current(
    current_manifest: dict[str, Any],
    current_manifest_path: Path,
    baseline_manifest: dict[str, Any] | None,
    baseline_manifest_path: Path | None,
    watch_current_manifest: dict[str, Any],
    watch_refresh_manifest: dict[str, Any],
    history_index: dict[str, Any],
    budget_profile: dict[str, Any],
) -> dict[str, Any]:
    watch_current_entries = {
        str(entry.get("execution_class", "")): dict(entry)
        for entry in watch_current_manifest.get("entries", [])
        if isinstance(entry, dict)
    }
    watch_refresh_entries = {
        str(entry.get("execution_class", "")): dict(entry)
        for entry in watch_refresh_manifest.get("entries", [])
        if isinstance(entry, dict)
    }
    baseline_entries = {
        str(entry.get("execution_class", "")): dict(entry)
        for entry in (baseline_manifest or {}).get("entries", [])
        if isinstance(entry, dict)
    }
    entries: list[dict[str, Any]] = []
    proposal_needed = False
    role_counts: dict[str, int] = {}
    for current_entry in current_manifest.get("entries", []):
        if not isinstance(current_entry, dict):
            continue
        execution_class = str(current_entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        watch_entry = dict(watch_refresh_entries.get(execution_class) or watch_current_entries.get(execution_class) or {})
        budget_entry = runtime_budget_profile_entry(budget_profile, execution_class)
        thresholds = dict(budget_entry.get("thresholds", default_runtime_threshold(execution_class)))
        role = str(budget_entry.get("role", current_entry.get("execution_role", default_runtime_role(execution_class))))
        role_counts[role] = role_counts.get(role, 0) + 1
        samples = history_samples_for_execution_class(history_index, current_manifest, execution_class)
        evidence_source_counts: dict[str, int] = {}
        latest_real_sample_timestamp = ""
        for sample in samples:
            source = normalized_evidence_source(sample.get("evidence_source"))
            evidence_source_counts[source] = evidence_source_counts.get(source, 0) + 1
            timestamp = str(sample.get("timestamp_utc", "")).strip()
            if source == "real" and timestamp and timestamp >= latest_real_sample_timestamp:
                latest_real_sample_timestamp = timestamp
        summary = summarize_runtime_sample_series(samples)
        merged_entry = {
            "execution_class": execution_class,
            "execution_role": role,
            "budget_thresholds": thresholds,
            "current_wall_time_sec": round(float(current_entry.get("wall_time_sec", 0.0)), 3),
            "baseline_wall_time_sec": None if execution_class not in baseline_entries else baseline_entries[execution_class].get("wall_time_sec"),
            "current_status": current_entry.get("current_status"),
            "watch_status": watch_entry.get("watch_status", WATCH_CLEAR),
            "watch_recommendation": watch_entry.get("watch_recommendation", "NO_ACTION"),
            "sample_count": max(int(watch_entry.get("sample_count", 0)), int(summary.get("sample_count", 0))),
            "real_sample_count": int(evidence_source_counts.get("real", 0)),
            "evidence_source_counts": evidence_source_counts,
            "stable_overrun_count": int(watch_entry.get("stable_overrun_count", 0)),
            "hard_over_budget_count": int(watch_entry.get("hard_over_budget_count", 0)),
            "soft_over_budget_count": int(watch_entry.get("soft_over_budget_count", 0)),
            "over_budget_ratio": watch_entry.get("over_budget_ratio", 0.0),
            "trend_direction": watch_entry.get("trend_direction", summary.get("trend_direction", TREND_INSUFFICIENT)),
            "rolling_median_wall_time_sec": watch_entry.get("rolling_median_wall_time_sec", summary.get("rolling_median_wall_time_sec")),
            "rolling_p90_wall_time_sec": watch_entry.get("rolling_p90_wall_time_sec", summary.get("rolling_p90_wall_time_sec")),
            "rolling_p95_wall_time_sec": watch_entry.get("rolling_p95_wall_time_sec", summary.get("rolling_p95_wall_time_sec")),
            "mad_wall_time_sec": watch_entry.get("mad_wall_time_sec", summary.get("mad_wall_time_sec")),
            "jitter_estimate_percent": watch_entry.get("jitter_estimate_percent", summary.get("jitter_estimate_percent", 0.0)),
            "history_depth": len(samples),
            "latest_real_sample_timestamp": latest_real_sample_timestamp or None,
        }
        watch_confidence, confidence_reason = classify_runtime_budget_confidence(merged_entry)
        merged_entry["watch_confidence"] = watch_confidence
        merged_entry["confidence_reason"] = confidence_reason
        proposal_candidate = (
            role == ROLE_PRODUCTION_CRITICAL
            and str(merged_entry.get("watch_status", WATCH_CLEAR)) in {WATCH_STABLE, WATCH_REBASELINE_CANDIDATE}
            and int(merged_entry.get("hard_over_budget_count", 0)) == 0
            and int(merged_entry.get("stable_overrun_count", 0)) > 0
            and budget_confidence_rank(watch_confidence) >= budget_confidence_rank("HIGH")
            and str(current_entry.get("current_status", STATUS_OK)) == STATUS_WARN
        )
        merged_entry["proposal_candidate"] = proposal_candidate
        merged_entry["proposed_thresholds"] = (
            proposed_runtime_budget_thresholds(execution_class, current_entry, merged_entry, budget_entry)
            if proposal_candidate
            else None
        )
        proposal_needed = proposal_needed or proposal_candidate
        entries.append(merged_entry)
    current_budget = {
        "manifest_version": "runtime_budget_current_v1",
        "generated_at_utc": timestamp_utc_now(),
        "phase": current_manifest.get("phase", ""),
        "runtime_current_manifest_path": str(current_manifest_path),
        "runtime_current_manifest_hash": sha256_file(current_manifest_path),
        "runtime_baseline_manifest_path": None if baseline_manifest_path is None else str(baseline_manifest_path),
        "runtime_baseline_manifest_hash": sha256_file(baseline_manifest_path),
        "runtime_fingerprint_key": runtime_manifest_fingerprint_key(current_manifest),
        "host_fingerprint": current_manifest.get("host_fingerprint", {}),
        "toolchain_fingerprint": current_manifest.get("toolchain_fingerprint", {}),
        "source_runtime_budget_profile_id": budget_profile.get("profile_id"),
        "source_runtime_budget_profile_version": budget_profile.get("version"),
        "budget_profile": budget_profile,
        "entries": entries,
        "current_verdict": current_manifest.get("current_verdict"),
        "freshness_verdict": FRESHNESS_FRESH,
        "comparability_verdict": COMPARABLE if baseline_manifest_path is not None else NOT_COMPARABLE,
        "budget_verdict": current_manifest.get("overall_budget_verdict"),
        "proposal_needed": proposal_needed,
        "budget_reproposal_needed": proposal_needed,
        "overall_watch_status": max(
            (str(entry.get("watch_status", WATCH_CLEAR)) for entry in entries),
            default=WATCH_CLEAR,
            key=runtime_watch_status_rank,
        ),
        "role_counts": role_counts,
    }
    current_budget["current_budget_hash"] = sha256_text(json.dumps(current_budget, sort_keys=True))
    return current_budget


def runtime_budget_current_text(manifest: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={manifest.get('manifest_version', '')}",
        f"phase={manifest.get('phase', '')}",
        f"current_verdict={manifest.get('current_verdict', '')}",
        f"budget_verdict={manifest.get('budget_verdict', '')}",
        f"proposal_needed={int(bool(manifest.get('proposal_needed', False)))}",
        f"source_runtime_budget_profile_id={manifest.get('source_runtime_budget_profile_id', '')}",
    ]
    for entry in manifest.get("entries", []):
        lines.append(
            "runtime_budget_current_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" role={entry.get('execution_role', '')}"
            + f" current_status={entry.get('current_status', '')}"
            + f" watch_status={entry.get('watch_status', '')}"
            + f" watch_confidence={entry.get('watch_confidence', '')}"
            + f" proposal_candidate={int(bool(entry.get('proposal_candidate', False)))}"
        )
    return "\n".join(lines) + "\n"


def runtime_budget_current_summary(manifest: dict[str, Any]) -> str:
    return (
        "runtime_budget_current_summary"
        + f" current_verdict={manifest.get('current_verdict', '')}"
        + f" budget_verdict={manifest.get('budget_verdict', '')}"
        + f" proposal_needed={int(bool(manifest.get('proposal_needed', False)))}"
        + f" entry_count={len(manifest.get('entries', []))}\n"
    )


def write_runtime_budget_current_outputs(json_path: Path, manifest: dict[str, Any]) -> None:
    write_json(json_path, manifest)
    write_text(json_path.with_suffix(".txt"), runtime_budget_current_text(manifest))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_budget_current_summary(manifest))


def build_runtime_budget_proposal(
    current_budget_manifest: dict[str, Any],
    current_budget_manifest_path: Path,
    budget_tag: str,
) -> dict[str, Any]:
    source_profile = dict(current_budget_manifest.get("budget_profile", {}))
    proposed_profile = json.loads(json.dumps(source_profile))
    relevant_entries = [
        dict(entry)
        for entry in current_budget_manifest.get("entries", [])
        if bool(entry.get("proposal_candidate", False))
    ]
    for entry in relevant_entries:
        execution_class = str(entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        proposed_profile.setdefault("entries", {}).setdefault(execution_class, {})
        proposed_profile["entries"][execution_class].update(
            {
                "execution_class": execution_class,
                "role": entry.get("execution_role"),
                "thresholds": dict(entry.get("proposed_thresholds") or entry.get("budget_thresholds", {})),
                "watch_policy": dict(
                    source_profile.get("entries", {}).get(execution_class, {}).get("watch_policy", default_runtime_watch_policy(execution_class, str(entry.get("execution_role", ROLE_OPERATOR))))
                ),
            }
        )
    proposed_profile["profile_id"] = f"{budget_tag}-profile"
    proposed_profile["version"] = int(source_profile.get("version", 0)) + 1
    proposed_profile["created_at"] = timestamp_utc_now()
    proposal = {
        "proposal_version": "runtime_budget_proposal_v1",
        "generated_at_utc": timestamp_utc_now(),
        "runtime_budget_current_path": str(current_budget_manifest_path),
        "runtime_budget_current_hash": sha256_file(current_budget_manifest_path),
        "runtime_current_manifest_path": current_budget_manifest.get("runtime_current_manifest_path"),
        "runtime_baseline_manifest_path": current_budget_manifest.get("runtime_baseline_manifest_path"),
        "source_runtime_budget_profile_id": current_budget_manifest.get("source_runtime_budget_profile_id"),
        "source_runtime_budget_profile_version": current_budget_manifest.get("source_runtime_budget_profile_version"),
        "budget_proposal_needed": bool(relevant_entries),
        "budget_reproposal_needed": bool(relevant_entries),
        "suggested_budget_tag": budget_tag,
        "recommended_action": "PROPOSE_BUDGET_REPROFILE" if relevant_entries else "NO_ACTION",
        "affected_execution_classes": [entry.get("execution_class") for entry in relevant_entries],
        "proposed_budget_profile": proposed_profile,
        "why_budget_reprofile_is_needed": (
            "stable production-critical soft-budget overrun is now backed by repeated same-fingerprint real evidence"
            if relevant_entries
            else "no execution class currently qualifies for budget reprofile"
        ),
        "per_execution_class": relevant_entries,
    }
    proposal["proposal_hash"] = sha256_text(json.dumps(proposal, sort_keys=True))
    return proposal


def runtime_budget_proposal_text(proposal: dict[str, Any]) -> str:
    lines = [
        f"proposal_version={proposal.get('proposal_version', '')}",
        f"budget_proposal_needed={int(bool(proposal.get('budget_proposal_needed', False)))}",
        f"suggested_budget_tag={proposal.get('suggested_budget_tag', '')}",
        f"recommended_action={proposal.get('recommended_action', '')}",
    ]
    for execution_class in proposal.get("affected_execution_classes", []):
        lines.append(f"affected_execution_class={execution_class}")
    for reason in [proposal.get("why_budget_reprofile_is_needed", "")]:
        if reason:
            lines.append(f"rationale={reason}")
    return "\n".join(lines) + "\n"


def runtime_budget_proposal_summary(proposal: dict[str, Any]) -> str:
    return (
        "runtime_budget_proposal_summary"
        + f" budget_proposal_needed={int(bool(proposal.get('budget_proposal_needed', False)))}"
        + f" affected_execution_class_count={len(proposal.get('affected_execution_classes', []))}\n"
    )


def write_runtime_budget_proposal_outputs(json_path: Path, proposal: dict[str, Any]) -> None:
    write_json(json_path, proposal)
    write_text(json_path.with_suffix(".txt"), runtime_budget_proposal_text(proposal))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_budget_proposal_summary(proposal))


def build_runtime_budget_proposal_gate(
    current_budget_manifest: dict[str, Any],
    current_budget_manifest_path: Path,
    proposal: dict[str, Any],
    proposal_path: Path,
    min_real_samples_release: int,
    max_hard_breach_count: int,
    min_watch_confidence: str,
) -> dict[str, Any]:
    relevant_entries = [
        dict(entry)
        for entry in current_budget_manifest.get("entries", [])
        if bool(entry.get("proposal_candidate", False))
    ]
    rationale: list[str] = []
    verdict = "REJECT"
    confidence = "LOW"
    if not bool(proposal.get("budget_proposal_needed", False)) or not relevant_entries:
        rationale.append("no production-critical execution class qualifies for budget reprofile")
    else:
        verdict = "APPROVABLE"
        confidence = min(
            (str(entry.get("watch_confidence", "LOW")) for entry in relevant_entries),
            default="LOW",
            key=budget_confidence_rank,
        )
        required_confidence_rank = budget_confidence_rank(min_watch_confidence)
        for entry in relevant_entries:
            execution_class = str(entry.get("execution_class", ""))
            real_sample_count = int(entry.get("real_sample_count", 0))
            hard_over_budget_count = int(entry.get("hard_over_budget_count", 0))
            required_real_samples = min_real_samples_release if execution_class == "release_full" else 1
            if hard_over_budget_count > max_hard_breach_count:
                verdict = "REJECT"
                rationale.append(f"{execution_class} recorded hard budget breaches; budget reprofile is not safe")
            elif real_sample_count < required_real_samples:
                if verdict != "REJECT":
                    verdict = "NEED_MORE_SAMPLES"
                rationale.append(
                    f"{execution_class} has only {real_sample_count} real same-fingerprint samples; require {required_real_samples}"
                )
            if budget_confidence_rank(str(entry.get("watch_confidence", "LOW"))) < required_confidence_rank:
                if verdict != "REJECT":
                    verdict = "NEED_MORE_SAMPLES"
                rationale.append(
                    f"{execution_class} watch confidence {entry.get('watch_confidence', 'LOW')} is below required {min_watch_confidence}"
                )
        if verdict == "APPROVABLE":
            rationale.append("release_full stable soft-budget overrun is sufficiently backed by repeated same-fingerprint evidence")
    gate = {
        "proposal_gate_version": "runtime_budget_proposal_gate_v1",
        "generated_at_utc": timestamp_utc_now(),
        "runtime_budget_current_path": str(current_budget_manifest_path),
        "runtime_budget_current_hash": sha256_file(current_budget_manifest_path),
        "runtime_budget_proposal_path": str(proposal_path),
        "runtime_budget_proposal_hash": sha256_file(proposal_path),
        "budget_proposal_needed": bool(proposal.get("budget_proposal_needed", False)),
        "budget_reproposal_needed": bool(proposal.get("budget_proposal_needed", False)),
        "proposal_gate_verdict": verdict,
        "reproposal_gate_verdict": verdict,
        "proposal_confidence": confidence,
        "reproposal_confidence": confidence,
        "recommended_action": (
            "PROPOSE_BUDGET_REPROFILE"
            if verdict == "APPROVABLE" and bool(proposal.get("budget_proposal_needed", False))
            else "CONTINUE_MONITORING"
            if verdict == "NEED_MORE_SAMPLES"
            else "WATCH_RUNTIME"
        ),
        "rationale": rationale,
        "per_execution_class_evidence": relevant_entries,
    }
    focus_entry = relevant_entries[0] if relevant_entries else {}
    gate["selected_budget_profile_id"] = current_budget_manifest.get("source_runtime_budget_profile_id")
    gate["selected_runtime_baseline_id"] = current_budget_manifest.get("runtime_baseline_manifest_path")
    gate["sample_count"] = int(focus_entry.get("sample_count", 0))
    gate["real_sample_count"] = int(focus_entry.get("real_sample_count", 0))
    gate["watch_status"] = focus_entry.get("watch_status")
    gate["watch_confidence"] = focus_entry.get("watch_confidence")
    gate["stable_overrun_count"] = int(focus_entry.get("stable_overrun_count", 0))
    gate["hard_over_budget_count"] = int(focus_entry.get("hard_over_budget_count", 0))
    gate["proposal_gate_hash"] = sha256_text(json.dumps(gate, sort_keys=True))
    return gate


def runtime_budget_proposal_gate_text(gate: dict[str, Any]) -> str:
    lines = [
        f"proposal_gate_version={gate.get('proposal_gate_version', '')}",
        f"budget_proposal_needed={int(bool(gate.get('budget_proposal_needed', False)))}",
        f"proposal_gate_verdict={gate.get('proposal_gate_verdict', '')}",
        f"proposal_confidence={gate.get('proposal_confidence', '')}",
        f"recommended_action={gate.get('recommended_action', '')}",
    ]
    for entry in gate.get("per_execution_class_evidence", []):
        lines.append(
            "runtime_budget_gate_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" real_sample_count={entry.get('real_sample_count', 0)}"
            + f" watch_confidence={entry.get('watch_confidence', '')}"
            + f" stable_overrun_count={entry.get('stable_overrun_count', 0)}"
        )
    for reason in gate.get("rationale", []):
        lines.append(f"rationale={reason}")
    return "\n".join(lines) + "\n"


def runtime_budget_proposal_gate_summary(gate: dict[str, Any]) -> str:
    return (
        "runtime_budget_proposal_gate_summary"
        + f" proposal_gate_verdict={gate.get('proposal_gate_verdict', '')}"
        + f" proposal_confidence={gate.get('proposal_confidence', '')}"
        + f" execution_class_count={len(gate.get('per_execution_class_evidence', []))}\n"
    )


def write_runtime_budget_proposal_gate_outputs(json_path: Path, gate: dict[str, Any]) -> None:
    write_json(json_path, gate)
    write_text(json_path.with_suffix(".txt"), runtime_budget_proposal_gate_text(gate))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_budget_proposal_gate_summary(gate))


def archived_runtime_budget_proposal(
    proposal: dict[str, Any],
    archive_path: Path,
    approval_metadata: dict[str, Any],
) -> dict[str, Any]:
    archived = json.loads(json.dumps(proposal))
    archived["proposal_archived"] = True
    archived["archived_at_utc"] = approval_metadata.get("approval_timestamp_utc", timestamp_utc_now())
    archived["archive_path"] = str(archive_path)
    archived["approved_budget_tag"] = approval_metadata.get("budget_tag")
    archived["approved_budget_profile_id"] = approval_metadata.get("new_active_budget_profile_id")
    archived["proposal_hash"] = sha256_text(json.dumps(archived, sort_keys=True))
    return archived


def approve_runtime_budget_reprofile(
    runtime_budget_current: dict[str, Any],
    runtime_budget_current_path: Path,
    proposal: dict[str, Any],
    proposal_path: Path,
    proposal_gate: dict[str, Any],
    proposal_gate_path: Path,
    registry: dict[str, Any],
    registry_path: Path,
    baseline_out_path: Path,
    budget_tag: str,
    activate: bool,
    archive_proposal_path: Path | None,
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any], dict[str, Any], dict[str, Any] | None]:
    gate_verdict = str(proposal_gate.get("proposal_gate_verdict", "")).strip()
    if gate_verdict != "APPROVABLE":
        raise RuntimeError(f"runtime budget approval rejected proposal gate verdict={gate_verdict or 'missing'}")
    if not bool(proposal.get("budget_proposal_needed", False)):
        raise RuntimeError("runtime budget approval rejected proposal that does not request reprofile")
    runtime_current_path = Path(str(runtime_budget_current.get("runtime_current_manifest_path", ""))).resolve()
    if not runtime_current_path.exists():
        raise RuntimeError("runtime budget approval rejected missing runtime current manifest")
    runtime_current_manifest = read_json(runtime_current_path)
    approved_profile = dict(proposal.get("proposed_budget_profile", {}))
    if not approved_profile:
        raise RuntimeError("runtime budget approval rejected empty proposed budget profile")
    approved_profile.setdefault("profile_id", f"{budget_tag}-profile")
    approved_profile["created_at"] = timestamp_utc_now()
    updated_current_manifest = apply_budget_profile_to_runtime_manifest(runtime_current_manifest, approved_profile)
    previous_active = active_runtime_budget_profile_path(registry)
    previous_active_entry = None
    if previous_active is not None:
        for candidate in registry.get("entries", []):
            if Path(str(candidate.get("runtime_budget_manifest_path", ""))).resolve() == previous_active:
                previous_active_entry = dict(candidate)
                break

    approval_metadata = {
        "approval_version": "runtime_budget_reprofile_approval_v1",
        "approval_status": "approved",
        "approved_from_runtime_current": str(runtime_current_path),
        "approved_from_runtime_current_hash": sha256_file(runtime_current_path),
        "approved_from_budget_current": str(runtime_budget_current_path),
        "approved_from_budget_current_hash": sha256_file(runtime_budget_current_path),
        "approved_from_budget_proposal": str(proposal_path),
        "approved_from_budget_proposal_hash": sha256_file(proposal_path),
        "approved_from_budget_gate": str(proposal_gate_path),
        "approved_from_budget_gate_hash": sha256_file(proposal_gate_path),
        "approved_from_budget_gate_verdict": gate_verdict,
        "proposal_confidence": proposal_gate.get("proposal_confidence"),
        "previous_active_budget_profile_id": None if previous_active_entry is None else previous_active_entry.get("profile_id"),
        "previous_active_budget_profile_tag": None if previous_active_entry is None else previous_active_entry.get("budget_tag"),
        "approval_timestamp_utc": timestamp_utc_now(),
        "budget_tag": budget_tag,
        "evidence_summary": {
            "affected_execution_classes": proposal.get("affected_execution_classes", []),
            "rationale": proposal_gate.get("rationale", []),
        },
    }
    approved_budget_baseline = {
        "manifest_version": "runtime_budget_baseline_v1",
        "manifest_role": "baseline",
        "profile_id": approved_profile.get("profile_id"),
        "version": approved_profile.get("version"),
        "created_at": approved_profile.get("created_at"),
        "entries": approved_profile.get("entries", {}),
        "budget_tag": budget_tag,
        "approval_timestamp_utc": approval_metadata["approval_timestamp_utc"],
        "source_runtime_current_manifest_path": str(runtime_current_path),
        "source_runtime_current_manifest_hash": sha256_file(runtime_current_path),
        "source_runtime_baseline_manifest_path": runtime_budget_current.get("runtime_baseline_manifest_path"),
        "host_fingerprint": updated_current_manifest.get("host_fingerprint", {}),
        "toolchain_fingerprint": updated_current_manifest.get("toolchain_fingerprint", {}),
        "execution_classes_covered": runtime_execution_classes(updated_current_manifest),
        "role_counts": {},
        "approval_metadata": approval_metadata,
        "current_verdict": updated_current_manifest.get("current_verdict"),
        "overall_budget_verdict": updated_current_manifest.get("overall_budget_verdict"),
    }
    role_counts: dict[str, int] = {}
    for execution_class, entry in dict(approved_profile.get("entries", {})).items():
        role = str(entry.get("role", default_runtime_role(str(execution_class))))
        role_counts[role] = role_counts.get(role, 0) + 1
    approved_budget_baseline["role_counts"] = role_counts
    write_runtime_budget_profile_outputs(baseline_out_path, approved_budget_baseline)
    updated_registry, registry_entry = promote_runtime_budget_registry(
        registry,
        approved_budget_baseline,
        baseline_out_path,
        budget_tag,
        activate,
    )
    approval_metadata["new_active_budget_profile_id"] = registry_entry.get("profile_id")
    approval_metadata["new_active_budget_profile_tag"] = registry_entry.get("budget_tag")
    approval_metadata["new_active_budget_profile_hash"] = sha256_file(baseline_out_path)
    approval_metadata["budget_transition_status"] = (
        "ACTIVE_SWITCHED"
        if approval_metadata.get("previous_active_budget_profile_id")
        and approval_metadata.get("previous_active_budget_profile_id") != approval_metadata.get("new_active_budget_profile_id")
        else "ACTIVE_CONFIRMED"
    )
    approved_budget_baseline["approval_metadata"] = approval_metadata
    write_runtime_budget_profile_outputs(baseline_out_path, approved_budget_baseline)
    updated_current_manifest["runtime_budget_approval_metadata"] = approval_metadata
    updated_current_manifest["runtime_budget_profile_path"] = str(baseline_out_path)
    updated_current_manifest["runtime_budget_transition_status"] = approval_metadata["budget_transition_status"]
    updated_current_manifest["current_runtime_manifest_hash"] = manifest_hash_without_field(updated_current_manifest, "current_runtime_manifest_hash")
    archived_proposal = None
    if archive_proposal_path is not None:
        archived_proposal = archived_runtime_budget_proposal(proposal, archive_proposal_path, approval_metadata)
    return updated_current_manifest, approved_budget_baseline, updated_registry, approval_metadata, archived_proposal


def build_runtime_budget_refresh(
    current_manifest: dict[str, Any],
    current_manifest_path: Path,
    budget_baseline_manifest: dict[str, Any] | None,
    budget_baseline_manifest_path: Path | None,
) -> dict[str, Any]:
    entries: list[dict[str, Any]] = []
    warn_count = 0
    fail_count = 0
    for current_entry in current_manifest.get("entries", []):
        if not isinstance(current_entry, dict):
            continue
        execution_class = str(current_entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        current_status = str(current_entry.get("current_status", STATUS_OK))
        if current_status == STATUS_WARN:
            warn_count += 1
        elif current_status == STATUS_FAIL:
            fail_count += 1
        entries.append(
            {
                "execution_class": execution_class,
                "execution_role": current_entry.get("execution_role"),
                "current_status": current_status,
                "budget_thresholds": current_entry.get("budget_thresholds", {}),
                "wall_time_sec": current_entry.get("wall_time_sec"),
                "watch_policy": current_entry.get("watch_policy", {}),
                "rationale": current_entry.get("rationale"),
            }
        )
    current_verdict = VERDICT_FAIL if fail_count else VERDICT_WARN if warn_count else VERDICT_PASS
    refresh = {
        "manifest_version": "runtime_budget_refresh_v1",
        "generated_at_utc": timestamp_utc_now(),
        "phase": current_manifest.get("phase", ""),
        "runtime_current_manifest_path": str(current_manifest_path),
        "runtime_current_manifest_hash": sha256_file(current_manifest_path),
        "runtime_budget_baseline_manifest_path": None if budget_baseline_manifest_path is None else str(budget_baseline_manifest_path),
        "runtime_budget_baseline_manifest_hash": sha256_file(budget_baseline_manifest_path),
        "current_verdict": current_verdict,
        "freshness_verdict": FRESHNESS_FRESH,
        "comparability_verdict": COMPARABLE if budget_baseline_manifest_path is not None else NOT_COMPARABLE,
        "budget_verdict": current_manifest.get("overall_budget_verdict"),
        "proposal_needed": current_verdict != VERDICT_PASS,
        "selected_budget_profile_id": None if budget_baseline_manifest is None else budget_baseline_manifest.get("profile_id"),
        "selected_budget_profile_tag": None if budget_baseline_manifest is None else budget_baseline_manifest.get("budget_tag"),
        "warn_count": warn_count,
        "fail_count": fail_count,
        "entries": entries,
    }
    refresh["refresh_hash"] = sha256_text(json.dumps(refresh, sort_keys=True))
    return refresh


def runtime_budget_refresh_text(manifest: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={manifest.get('manifest_version', '')}",
        f"current_verdict={manifest.get('current_verdict', '')}",
        f"freshness_verdict={manifest.get('freshness_verdict', '')}",
        f"comparability_verdict={manifest.get('comparability_verdict', '')}",
        f"budget_verdict={manifest.get('budget_verdict', '')}",
        f"selected_budget_profile_id={manifest.get('selected_budget_profile_id', '')}",
    ]
    for entry in manifest.get("entries", []):
        lines.append(
            "runtime_budget_refresh_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" current_status={entry.get('current_status', '')}"
        )
    return "\n".join(lines) + "\n"


def runtime_budget_refresh_summary(manifest: dict[str, Any]) -> str:
    return (
        "runtime_budget_refresh_summary"
        + f" current_verdict={manifest.get('current_verdict', '')}"
        + f" budget_verdict={manifest.get('budget_verdict', '')}"
        + f" entry_count={len(manifest.get('entries', []))}\n"
    )


def write_runtime_budget_refresh_outputs(json_path: Path, manifest: dict[str, Any]) -> None:
    write_json(json_path, manifest)
    write_text(json_path.with_suffix(".txt"), runtime_budget_refresh_text(manifest))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_budget_refresh_summary(manifest))


def build_runtime_budget_rerun_plan(refresh_manifest: dict[str, Any]) -> dict[str, Any]:
    selected_entries = [
        {
            "execution_class": entry.get("execution_class"),
            "reason": entry.get("rationale", "runtime budget entry requires rerun"),
        }
        for entry in refresh_manifest.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("current_status", STATUS_OK)) in {STATUS_WARN, STATUS_FAIL}
    ]
    plan = {
        "plan_version": "runtime_budget_rerun_plan_v1",
        "generated_at_utc": timestamp_utc_now(),
        "selected_entry_count": len(selected_entries),
        "summary_verdict": "PASS" if not selected_entries else "ACTION_REQUIRED",
        "entries": selected_entries,
    }
    plan["plan_hash"] = sha256_text(json.dumps(plan, sort_keys=True))
    return plan


def runtime_budget_rerun_plan_text(plan: dict[str, Any]) -> str:
    lines = [
        f"plan_version={plan.get('plan_version', '')}",
        f"summary_verdict={plan.get('summary_verdict', '')}",
        f"selected_entry_count={plan.get('selected_entry_count', 0)}",
    ]
    for entry in plan.get("entries", []):
        lines.append(
            "runtime_budget_rerun_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" reason={entry.get('reason', '')}"
        )
    return "\n".join(lines) + "\n"


def runtime_budget_rerun_plan_summary(plan: dict[str, Any]) -> str:
    return (
        "runtime_budget_rerun_plan_summary"
        + f" summary_verdict={plan.get('summary_verdict', '')}"
        + f" selected_entry_count={plan.get('selected_entry_count', 0)}\n"
    )


def write_runtime_budget_rerun_plan_outputs(json_path: Path, plan: dict[str, Any]) -> None:
    write_json(json_path, plan)
    write_text(json_path.with_suffix(".txt"), runtime_budget_rerun_plan_text(plan))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_budget_rerun_plan_summary(plan))


def parse_runtime_entry_text(text: str) -> dict[str, Any]:
    values: dict[str, str] = {}
    for token in text.split(","):
        token = token.strip()
        if not token:
            continue
        if "=" not in token:
            raise ValueError(f"invalid runtime entry token: {token}")
        key, value = token.split("=", 1)
        values[key.strip()] = value.strip()
    execution_class = values.get("execution_class") or values.get("stage")
    if not execution_class:
        raise ValueError(f"runtime entry missing execution_class: {text}")
    return {
        "execution_class": execution_class,
        "wall_time_sec": float(values.get("wall_time_sec", values.get("seconds", "0"))),
        "test_count": int(values.get("test_count", values.get("tests", "0"))),
        "build_type": values.get("build_type", values.get("build", execution_class.split("_", 1)[0].capitalize())),
        "sanitizer_flags": values.get("sanitizer_flags", values.get("sanitizers", "none")),
        "runner_tag": values.get("runner_tag", ""),
    }


def entry_status(
    wall_time_sec: float,
    threshold: dict[str, float],
    baseline_wall_time_sec: float | None = None,
) -> tuple[str, float, str]:
    delta_percent = 0.0
    delta_seconds = 0.0
    if baseline_wall_time_sec and baseline_wall_time_sec > 0.0:
        delta_seconds = wall_time_sec - baseline_wall_time_sec
        delta_percent = ((wall_time_sec - baseline_wall_time_sec) / baseline_wall_time_sec) * 100.0
    hard_seconds = threshold.get("hard_seconds", 0.0)
    soft_seconds = threshold.get("soft_seconds", 0.0)
    hard_delta = threshold.get("hard_delta_percent", 0.0)
    soft_delta = threshold.get("soft_delta_percent", 0.0)
    hard_delta_floor = threshold.get("hard_delta_floor_sec", 0.0)
    soft_delta_floor = threshold.get("soft_delta_floor_sec", 0.0)
    if (hard_seconds > 0.0 and wall_time_sec > hard_seconds) or (
        baseline_wall_time_sec and delta_percent > hard_delta > 0.0 and delta_seconds > hard_delta_floor
    ):
        return STATUS_FAIL, round(delta_percent, 2), "runtime exceeded hard budget"
    if (soft_seconds > 0.0 and wall_time_sec > soft_seconds) or (
        baseline_wall_time_sec and delta_percent > soft_delta > 0.0 and delta_seconds > soft_delta_floor
    ):
        return STATUS_WARN, round(delta_percent, 2), "runtime exceeded soft budget"
    return STATUS_OK, round(delta_percent, 2), "runtime remained within budget"


def build_runtime_current_manifest(
    phase: str,
    artifact_root: str,
    entries: list[dict[str, Any]],
    runner_tag: str = "",
    baseline_tag: str = "",
    budget_config: dict[str, dict[str, float]] | None = None,
    budget_profile: dict[str, Any] | None = None,
) -> dict[str, Any]:
    host = host_fingerprint(runner_tag)
    normalized_entries: list[dict[str, Any]] = []
    warn_count = 0
    fail_count = 0
    for entry in entries:
        execution_class = str(entry["execution_class"])
        budget_profile_entry = runtime_budget_profile_entry(budget_profile, execution_class)
        threshold = dict(
            entry.get("budget_thresholds")
            or budget_profile_entry.get("thresholds")
            or (budget_config or {}).get(execution_class)
            or default_runtime_threshold(execution_class)
        )
        execution_role = str(entry.get("execution_role", budget_profile_entry.get("role", default_runtime_role(execution_class))))
        watch_policy = dict(entry.get("watch_policy") or budget_profile_entry.get("watch_policy") or default_runtime_watch_policy(execution_class, execution_role))
        status, delta_percent, rationale = entry_status(float(entry["wall_time_sec"]), threshold, None)
        if status == STATUS_WARN:
            warn_count += 1
        elif status == STATUS_FAIL:
            fail_count += 1
        build_type = str(entry.get("build_type", "unknown"))
        sanitizer_flags = str(entry.get("sanitizer_flags", "none"))
        normalized_entries.append(
            {
                "execution_class": execution_class,
                "wall_time_sec": round(float(entry["wall_time_sec"]), 3),
                "test_count": int(entry.get("test_count", 0)),
                "build_type": build_type,
                "sanitizer_flags": sanitizer_flags,
                "budget_thresholds": threshold,
                "execution_role": execution_role,
                "watch_policy": watch_policy,
                "runtime_budget_profile_id": None if budget_profile is None else budget_profile.get("profile_id"),
                "runtime_budget_profile_version": None if budget_profile is None else budget_profile.get("version"),
                "current_status": status,
                "freshness_status": FRESHNESS_FRESH,
                "delta_percent": delta_percent,
                "rationale": rationale,
                "fingerprint_hash": sha256_text(
                    "|".join([host["fingerprint_hash"], execution_class, build_type, sanitizer_flags])
                ),
            }
        )
    overall_status = STATUS_FAIL if fail_count else STATUS_WARN if warn_count else STATUS_OK
    current_verdict = VERDICT_FAIL if fail_count else VERDICT_WARN if warn_count else VERDICT_PASS
    manifest = {
        "manifest_version": "runtime_current_manifest_v1",
        "manifest_role": "current",
        "generated_at_utc": timestamp_utc_now(),
        "phase": phase,
        "artifact_root": artifact_root,
        "baseline_tag": baseline_tag,
        "host_fingerprint": host,
        "toolchain_fingerprint": {
            "compiler_command": host["compiler_command"],
            "compiler_id": host["compiler_id"],
            "compiler_version": host["compiler_version"],
        },
        "runtime_budget_profile_id": None if budget_profile is None else budget_profile.get("profile_id"),
        "runtime_budget_profile_version": None if budget_profile is None else budget_profile.get("version"),
        "runtime_budget_profile": None
        if budget_profile is None
        else {
            "profile_id": budget_profile.get("profile_id"),
            "version": budget_profile.get("version"),
            "created_at": budget_profile.get("created_at"),
            "entries": {
                execution_class: runtime_budget_profile_entry(budget_profile, execution_class)
                for execution_class in runtime_execution_classes({"entries": normalized_entries})
            },
        },
        "entries": normalized_entries,
        "warn_count": warn_count,
        "fail_count": fail_count,
        "overall_status": overall_status,
        "current_verdict": current_verdict,
        "overall_budget_verdict": "BUDGET_FAIL" if overall_status == STATUS_FAIL else "BUDGET_WARN" if overall_status == STATUS_WARN else "PASS",
    }
    manifest["current_runtime_manifest_hash"] = manifest_hash_without_field(manifest, "current_runtime_manifest_hash")
    return manifest


def runtime_entry_fingerprint(host_fingerprint_hash: str, entry: dict[str, Any]) -> str:
    return sha256_text(
        "|".join(
            [
                host_fingerprint_hash,
                str(entry.get("execution_class", "")),
                str(entry.get("build_type", "")),
                str(entry.get("sanitizer_flags", "")),
            ]
        )
    )


def manifest_hash_without_field(manifest: dict[str, Any], field_name: str) -> str:
    normalized = json.loads(json.dumps(manifest))
    normalized.pop(field_name, None)
    return sha256_text(json.dumps(normalized, sort_keys=True))


def recompute_host_fingerprint_hash(host: dict[str, Any]) -> None:
    host["fingerprint_hash"] = sha256_text(
        "|".join(
            [
                str(host.get("os", "")),
                str(host.get("os_release", "")),
                str(host.get("arch", "")),
                str(host.get("runner_tag", "")),
                str(host.get("compiler_id", "")),
                str(host.get("compiler_version", "")),
            ]
        )
    )


def normalize_runtime_current_manifest(mutated: dict[str, Any]) -> dict[str, Any]:
    host = mutated.setdefault("host_fingerprint", {})
    recompute_host_fingerprint_hash(host)
    warn_count = 0
    fail_count = 0
    budget_profile = runtime_budget_profile_for_manifest(mutated)
    mutated["runtime_budget_profile_id"] = budget_profile.get("profile_id")
    mutated["runtime_budget_profile_version"] = budget_profile.get("version")
    mutated["runtime_budget_profile"] = {
        "profile_id": budget_profile.get("profile_id"),
        "version": budget_profile.get("version"),
        "created_at": budget_profile.get("created_at"),
        "entries": {},
    }
    for entry in mutated.get("entries", []):
        if not isinstance(entry, dict):
            continue
        execution_class = str(entry.get("execution_class", ""))
        budget_profile_entry = runtime_budget_profile_entry(budget_profile, execution_class)
        threshold = dict(entry.get("budget_thresholds") or budget_profile_entry.get("thresholds") or default_runtime_threshold(execution_class))
        entry["execution_role"] = str(entry.get("execution_role", budget_profile_entry.get("role", default_runtime_role(execution_class))))
        entry["watch_policy"] = dict(entry.get("watch_policy") or budget_profile_entry.get("watch_policy") or default_runtime_watch_policy(execution_class, str(entry.get("execution_role", default_runtime_role(execution_class)))))
        entry["runtime_budget_profile_id"] = mutated["runtime_budget_profile_id"]
        entry["runtime_budget_profile_version"] = mutated["runtime_budget_profile_version"]
        status, delta_percent, rationale = entry_status(
            float(entry.get("wall_time_sec", 0.0)),
            threshold,
            None,
        )
        entry["current_status"] = status
        entry["delta_percent"] = delta_percent
        entry["rationale"] = rationale
        entry["fingerprint_hash"] = runtime_entry_fingerprint(str(host.get("fingerprint_hash", "")), entry)
        mutated["runtime_budget_profile"]["entries"][execution_class] = runtime_budget_profile_entry(budget_profile, execution_class)
        if status == STATUS_WARN:
            warn_count += 1
        elif status == STATUS_FAIL:
            fail_count += 1
    mutated["warn_count"] = warn_count
    mutated["fail_count"] = fail_count
    mutated["overall_status"] = STATUS_FAIL if fail_count else STATUS_WARN if warn_count else STATUS_OK
    mutated["current_verdict"] = VERDICT_FAIL if fail_count else VERDICT_WARN if warn_count else VERDICT_PASS
    mutated["overall_budget_verdict"] = "BUDGET_FAIL" if fail_count else "BUDGET_WARN" if warn_count else "PASS"
    mutated["current_runtime_manifest_hash"] = manifest_hash_without_field(mutated, "current_runtime_manifest_hash")
    return mutated


def runtime_manifest_text(manifest: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={manifest.get('manifest_version', '')}",
        f"manifest_role={manifest.get('manifest_role', '')}",
        f"phase={manifest.get('phase', '')}",
        f"artifact_root={manifest.get('artifact_root', '')}",
        f"baseline_tag={manifest.get('baseline_tag', '')}",
        f"overall_status={manifest.get('overall_status', '')}",
        f"overall_budget_verdict={manifest.get('overall_budget_verdict', '')}",
        f"warn_count={manifest.get('warn_count', 0)}",
        f"fail_count={manifest.get('fail_count', 0)}",
        f"host_fingerprint_hash={manifest.get('host_fingerprint', {}).get('fingerprint_hash', '')}",
    ]
    for entry in manifest.get("entries", []):
        lines.append(
            "runtime_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" wall_time_sec={entry.get('wall_time_sec', 0)}"
            + f" test_count={entry.get('test_count', 0)}"
            + f" build_type={entry.get('build_type', '')}"
            + f" sanitizer_flags={entry.get('sanitizer_flags', '')}"
            + f" current_status={entry.get('current_status', '')}"
            + f" freshness_status={entry.get('freshness_status', '')}"
        )
    return "\n".join(lines) + "\n"


def runtime_manifest_summary(manifest: dict[str, Any]) -> str:
    return (
        "runtime_manifest_summary"
        + f" role={manifest.get('manifest_role', '')}"
        + f" overall_status={manifest.get('overall_status', '')}"
        + f" overall_budget_verdict={manifest.get('overall_budget_verdict', '')}"
        + f" entry_count={len(manifest.get('entries', []))}"
        + f" warn_count={manifest.get('warn_count', 0)}"
        + f" fail_count={manifest.get('fail_count', 0)}\n"
    )


def write_runtime_manifest_outputs(json_path: Path, manifest: dict[str, Any]) -> None:
    write_json(json_path, manifest)
    write_text(json_path.with_suffix(".txt"), runtime_manifest_text(manifest))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_manifest_summary(manifest))


def default_runtime_approval_metadata_path(baseline_manifest_path: Path) -> Path:
    return baseline_manifest_path.with_name(f"{baseline_manifest_path.stem}_approval_metadata.json")


def write_runtime_approval_metadata_outputs(json_path: Path, approval_metadata: dict[str, Any]) -> None:
    previous_id = approval_metadata.get("selected_previous_baseline_id", approval_metadata.get("selected_previous_baseline", ""))
    write_json(json_path, approval_metadata)
    write_text(
        json_path.with_suffix(".txt"),
        "\n".join(
            [
                f"approval_timestamp_utc={approval_metadata.get('approval_timestamp_utc', '')}",
                f"baseline_tag={approval_metadata.get('baseline_tag', '')}",
                f"previous_active_runtime_baseline_id={previous_id}",
                f"new_active_runtime_baseline_id={approval_metadata.get('new_active_runtime_baseline_id', '')}",
                f"approval_status={approval_metadata.get('approval_status', '')}",
                f"runtime_transition_status={approval_metadata.get('runtime_transition_status', '')}",
            ]
        )
        + "\n",
    )
    write_text(
        json_path.with_name(f"{json_path.stem}.summary.txt"),
        "runtime_rebaseline_approval_summary"
        + f" baseline_tag={approval_metadata.get('baseline_tag', '')}"
        + f" previous_active_runtime_baseline_id={previous_id}"
        + f" new_active_runtime_baseline_id={approval_metadata.get('new_active_runtime_baseline_id', '')}\n",
    )


def runtime_baseline_embedded_approval_metadata(approval_metadata: dict[str, Any]) -> dict[str, Any]:
    embedded = json.loads(json.dumps(approval_metadata))
    # Avoid self-referential hashes inside the approved baseline manifest.
    embedded.pop("new_active_runtime_baseline_manifest_hash", None)
    return embedded


def validate_runtime_baseline_manifest(
    baseline_manifest: dict[str, Any],
    baseline_manifest_path: Path,
) -> None:
    if str(baseline_manifest.get("manifest_role", "")) != "baseline":
        raise RuntimeError(f"runtime registry promotion requires manifest_role=baseline: {baseline_manifest_path}")
    if not bool(baseline_manifest.get("provenance_frozen", False)):
        raise RuntimeError(f"runtime registry promotion requires provenance_frozen baseline: {baseline_manifest_path}")


def runtime_registry_entry_manifest_status(entry: dict[str, Any]) -> tuple[bool, str]:
    manifest_path_text = str(entry.get("runtime_baseline_manifest_path", "")).strip()
    if not manifest_path_text:
        return False, "runtime registry entry is missing runtime_baseline_manifest_path"
    manifest_path = Path(manifest_path_text).resolve()
    if not manifest_path.exists():
        return False, "runtime registry baseline manifest path no longer exists"
    manifest_hash = sha256_file(manifest_path)
    expected_raw_hash = entry.get("runtime_baseline_manifest_hash")
    expected_hash = "" if expected_raw_hash in {None, ""} else str(expected_raw_hash).strip()
    if expected_hash and manifest_hash != expected_hash:
        return False, "runtime registry baseline manifest hash drifted after approval"
    manifest = read_json(manifest_path)
    if str(manifest.get("manifest_role", "")) != "baseline":
        return False, "runtime registry entry points at a non-baseline manifest"
    if not bool(manifest.get("provenance_frozen", False)):
        return False, "runtime registry entry points at a mutable runtime manifest"
    return True, ""


def promote_runtime_baseline(
    current_manifest: dict[str, Any],
    source_manifest_path: Path,
    baseline_tag: str,
    require_acceptable_status: bool,
) -> dict[str, Any]:
    if require_acceptable_status and current_manifest.get("overall_status") == STATUS_FAIL:
        raise RuntimeError("runtime baseline promotion rejected FAIL current manifest")
    baseline = json.loads(json.dumps(current_manifest))
    baseline["manifest_version"] = "runtime_baseline_manifest_v1"
    baseline["manifest_role"] = "baseline"
    baseline["baseline_tag"] = baseline_tag
    baseline["approval_timestamp_utc"] = timestamp_utc_now()
    baseline["promoted_from_manifest"] = str(source_manifest_path)
    baseline["provenance_frozen"] = True
    baseline["current_runtime_manifest_hash"] = manifest_hash_without_field(baseline, "current_runtime_manifest_hash")
    return baseline


def archived_runtime_proposal(
    proposal: dict[str, Any],
    archive_path: Path,
    approval_metadata: dict[str, Any],
) -> dict[str, Any]:
    archived = json.loads(json.dumps(proposal))
    archived["proposal_archived"] = True
    archived["proposal_archive_path"] = str(archive_path)
    archived["archived_at_utc"] = approval_metadata.get("approval_timestamp_utc", timestamp_utc_now())
    archived["approval_metadata"] = approval_metadata
    archived["approved_runtime_baseline_path"] = approval_metadata.get("approved_runtime_baseline_path")
    archived["approved_runtime_baseline_hash"] = approval_metadata.get("new_active_runtime_baseline_manifest_hash")
    archived["approved_runtime_baseline_id"] = approval_metadata.get("new_active_runtime_baseline_id")
    archived["approved_runtime_baseline_tag"] = approval_metadata.get("baseline_tag")
    archived["previous_active_runtime_baseline_id"] = approval_metadata.get("selected_previous_baseline")
    archived["proposal_hash"] = sha256_text(json.dumps(archived, sort_keys=True))
    return archived


def watch_confidence_rank(value: str) -> int:
    return {"LOW": 0, "MEDIUM": 1, "HIGH": 2}.get(str(value).strip().upper(), -1)


def runtime_history_bucket_for_manifest(
    history_index: dict[str, Any],
    current_manifest: dict[str, Any],
) -> dict[str, Any]:
    runtime_key = runtime_manifest_fingerprint_key(current_manifest)
    fingerprint_hash = str(current_manifest.get("host_fingerprint", {}).get("fingerprint_hash", ""))
    toolchain = dict(current_manifest.get("toolchain_fingerprint", {}))
    for bucket in history_index.get("fingerprints", []):
        if str(bucket.get("runtime_fingerprint_key", "")) == runtime_key:
            return bucket
    for bucket in history_index.get("fingerprints", []):
        if str(bucket.get("fingerprint_hash", "")) != fingerprint_hash:
            continue
        bucket_toolchain = dict(bucket.get("toolchain_fingerprint", {}))
        if (
            str(bucket_toolchain.get("compiler_id", "")) == str(toolchain.get("compiler_id", ""))
            and str(bucket_toolchain.get("compiler_version", "")) == str(toolchain.get("compiler_version", ""))
        ):
            return bucket
    return {}


def proposal_gate_thresholds(args: Any) -> dict[str, int]:
    return {
        "release_full": max(int(getattr(args, "min_real_samples_release", 1) or 1), 1),
        "debug_full": max(int(getattr(args, "min_real_samples_debug", 1) or 1), 1),
        "asan_full": max(int(getattr(args, "min_real_samples_asan", 5) or 1), 1),
    }


def normalize_watch_confidence_requirement(value: str | None) -> str:
    normalized = str(value or "MEDIUM").strip().upper()
    if normalized not in {"LOW", "MEDIUM", "HIGH"}:
        raise RuntimeError(f"invalid min watch confidence: {value}")
    return normalized


def classify_proposal_watch_confidence(
    *,
    sample_count: int,
    real_sample_count: int,
    watch_status: str,
    jitter_estimate_percent: float,
) -> tuple[str, str]:
    bounded_jitter = jitter_estimate_percent <= 15.0
    if (
        real_sample_count >= 5
        and sample_count >= 5
        and bounded_jitter
        and watch_status in {
            WATCH_CLEAR,
            WATCH_WATCH,
            WATCH_STABLE,
            WATCH_REBASELINE_CANDIDATE,
            WATCH_REBASELINE_REQUIRED,
        }
    ):
        if watch_status == WATCH_REBASELINE_REQUIRED:
            return "HIGH", "repeated same-fingerprint real evidence is available and the runtime is ready for a new fingerprint-specific baseline approval"
        return "HIGH", "repeated same-fingerprint real evidence is available with bounded jitter"
    if real_sample_count >= 1 and sample_count >= 1:
        return "MEDIUM", "same-fingerprint real evidence exists, but repeated coverage is still limited"
    return "LOW", "same-fingerprint real evidence is sparse"


def build_runtime_proposal_gate(
    current_manifest: dict[str, Any],
    current_manifest_path: Path,
    proposal: dict[str, Any],
    proposal_path: Path,
    history_index: dict[str, Any],
    watch_current: dict[str, Any],
    watch_refresh: dict[str, Any],
    thresholds: dict[str, int],
    min_watch_confidence: str,
) -> dict[str, Any]:
    current_manifest_hash = sha256_file(current_manifest_path)
    history_bucket = runtime_history_bucket_for_manifest(history_index, current_manifest)
    watch_entry_map: dict[str, dict[str, Any]] = {}
    for manifest in (watch_current, watch_refresh):
        for entry in manifest.get("entries", []):
            if not isinstance(entry, dict):
                continue
            execution_class = str(entry.get("execution_class", "")).strip()
            if execution_class:
                watch_entry_map[execution_class] = dict(entry)

    relevant_execution_classes = [
        execution_class
        for execution_class in runtime_execution_classes(current_manifest)
        if execution_class in {"release_full", "debug_full", "asan_full"}
    ]
    if not relevant_execution_classes:
        relevant_execution_classes = runtime_execution_classes(current_manifest)

    rationale: list[str] = []
    reject_reasons: list[str] = []
    need_more_sample_reasons: list[str] = []
    per_execution_class: list[dict[str, Any]] = []
    confidence_levels: list[str] = []
    confidence_reasons: list[str] = []

    proposal_current_manifest_path = str(proposal.get("runtime_current_manifest_path", "")).strip()
    if proposal_current_manifest_path and proposal_current_manifest_path != str(current_manifest_path):
        reject_reasons.append("current/proposal mismatch: runtime current manifest path changed")
    proposal_current_manifest_hash = str(proposal.get("runtime_current_manifest_hash", "")).strip()
    if proposal_current_manifest_hash and proposal_current_manifest_hash != str(current_manifest_hash):
        reject_reasons.append("current/proposal mismatch: runtime current manifest hash changed")
    if not bool(proposal.get("proposal_needed", False)):
        reject_reasons.append("proposal does not request a runtime rebaseline")

    if not history_bucket:
        need_more_sample_reasons.append("history 부족: same-fingerprint runtime history bucket is missing")

    execution_payloads = dict(history_bucket.get("execution_classes", {}))
    for execution_class in relevant_execution_classes:
        payload = dict(execution_payloads.get(execution_class, {}))
        samples = [dict(sample) for sample in payload.get("samples", []) if isinstance(sample, dict)]
        sample_count = len(samples)
        real_sample_count = sum(
            1 for sample in samples if normalized_evidence_source(sample.get("evidence_source")) == "real"
        )
        soft_over_budget_count = sum(
            1
            for sample in samples
            if str(sample.get("current_status", STATUS_OK)) in {STATUS_WARN, STATUS_FAIL}
        )
        hard_over_budget_count = sum(
            1 for sample in samples if str(sample.get("current_status", STATUS_OK)) == STATUS_FAIL
        )
        over_budget_ratio = round(soft_over_budget_count / sample_count, 3) if sample_count else 0.0
        summary = dict(payload.get("summary", summarize_runtime_sample_series(samples)))
        watch_entry = dict(watch_entry_map.get(execution_class, {}))
        watch_status = str(watch_entry.get("watch_status", WATCH_WATCH if sample_count else WATCH_CLEAR))
        watch_confidence, watch_confidence_reason = classify_proposal_watch_confidence(
            sample_count=sample_count,
            real_sample_count=real_sample_count,
            watch_status=watch_status,
            jitter_estimate_percent=float(
                watch_entry.get("jitter_estimate_percent", summary.get("jitter_estimate_percent", 0.0)) or 0.0
            ),
        )
        threshold = thresholds.get(execution_class, 1)
        threshold_reason = None
        if real_sample_count < threshold:
            threshold_reason = (
                f"same_fingerprint_real_sample_count 부족: {execution_class} real samples {real_sample_count} < required {threshold}"
            )
            need_more_sample_reasons.append(threshold_reason)
        if watch_confidence_rank(watch_confidence) < watch_confidence_rank(min_watch_confidence):
            need_more_sample_reasons.append(
                f"watch stability 부족: {execution_class} watch confidence {watch_confidence} < required {min_watch_confidence}"
            )
        if hard_over_budget_count > 0 or watch_status in {WATCH_FAIL, WATCH_ESCALATE}:
            reject_reasons.append(
                f"hard breach 존재: {execution_class} reported hard_over_budget_count={hard_over_budget_count} watch_status={watch_status}"
            )
        per_execution_class.append(
            {
                "execution_class": execution_class,
                "sample_count": sample_count,
                "real_sample_count": real_sample_count,
                "required_real_sample_count": threshold,
                "watch_status": watch_status,
                "watch_confidence": watch_confidence,
                "watch_confidence_reason": watch_confidence_reason,
                "trend_direction": watch_entry.get("trend_direction", summary.get("trend_direction", TREND_INSUFFICIENT)),
                "over_budget_ratio": over_budget_ratio,
                "hard_over_budget_count": hard_over_budget_count,
                "soft_over_budget_count": soft_over_budget_count,
                "stable_overrun_count": int(watch_entry.get("stable_overrun_count", 0)),
                "jitter_estimate_percent": watch_entry.get(
                    "jitter_estimate_percent", summary.get("jitter_estimate_percent", 0.0)
                ),
                "latest_wall_time_sec": summary.get("latest_wall_time_sec"),
                "latest_baseline_tag": samples[-1].get("baseline_tag") if samples else None,
                "evidence_source_counts": dict(summary.get("evidence_source_counts", {})),
            }
        )
        confidence_levels.append(watch_confidence)
        confidence_reasons.append(f"{execution_class}: {watch_confidence_reason}")

    if reject_reasons:
        proposal_gate_verdict = "REJECT"
        proposal_confidence = "LOW"
        rationale.extend(reject_reasons)
    elif need_more_sample_reasons:
        proposal_gate_verdict = "NEED_MORE_SAMPLES"
        proposal_confidence = "LOW" if not confidence_levels else min(
            confidence_levels, key=lambda value: watch_confidence_rank(value)
        )
        rationale.extend(dict.fromkeys(need_more_sample_reasons))
    else:
        proposal_gate_verdict = "APPROVABLE"
        proposal_confidence = "HIGH" if all(
            watch_confidence_rank(value) >= watch_confidence_rank("HIGH") for value in confidence_levels
        ) else "MEDIUM"
        rationale.append("same-fingerprint real evidence satisfies the configured proposal gate")
        if confidence_reasons:
            rationale.extend(confidence_reasons)

    gate = {
        "proposal_gate_version": "runtime_proposal_gate_v1",
        "generated_at_utc": timestamp_utc_now(),
        "runtime_current_manifest_path": str(current_manifest_path),
        "runtime_current_manifest_hash": current_manifest_hash,
        "runtime_proposal_path": str(proposal_path),
        "runtime_proposal_hash": sha256_file(proposal_path),
        "runtime_history_index_path": str(history_index.get("runtime_history_index_path", "")) or None,
        "runtime_watch_current_path": str(watch_current.get("runtime_current_manifest_path", "")) or None,
        "runtime_watch_refresh_path": str(watch_refresh.get("runtime_refresh_manifest_path", "")) or None,
        "selected_baseline_id": proposal.get("selected_baseline_id"),
        "selected_baseline_tag": proposal.get("selected_baseline_tag"),
        "proposal_confidence": proposal_confidence,
        "proposal_gate_verdict": proposal_gate_verdict,
        "min_watch_confidence": min_watch_confidence,
        "required_real_sample_thresholds": thresholds,
        "rationale": rationale,
        "per_execution_class_evidence": per_execution_class,
        "evidence_summary": {
            "relevant_execution_class_count": len(relevant_execution_classes),
            "total_real_sample_count": sum(int(entry.get("real_sample_count", 0)) for entry in per_execution_class),
            "total_sample_count": sum(int(entry.get("sample_count", 0)) for entry in per_execution_class),
            "hard_breach_execution_class_count": sum(
                1 for entry in per_execution_class if int(entry.get("hard_over_budget_count", 0)) > 0
            ),
            "watch_status_counts": {
                status: sum(1 for entry in per_execution_class if str(entry.get("watch_status", "")) == status)
                for status in sorted({str(entry.get("watch_status", "")) for entry in per_execution_class})
                if status
            },
        },
    }
    gate["proposal_gate_hash"] = sha256_text(json.dumps(gate, sort_keys=True))
    return gate


def runtime_proposal_gate_text(gate: dict[str, Any]) -> str:
    lines = [
        f"proposal_gate_version={gate.get('proposal_gate_version', '')}",
        f"proposal_confidence={gate.get('proposal_confidence', '')}",
        f"proposal_gate_verdict={gate.get('proposal_gate_verdict', '')}",
        f"selected_baseline_id={gate.get('selected_baseline_id', '')}",
        f"selected_baseline_tag={gate.get('selected_baseline_tag', '')}",
    ]
    for entry in gate.get("per_execution_class_evidence", []):
        lines.append(
            "runtime_proposal_gate_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" sample_count={entry.get('sample_count', 0)}"
            + f" real_sample_count={entry.get('real_sample_count', 0)}"
            + f" watch_status={entry.get('watch_status', '')}"
            + f" watch_confidence={entry.get('watch_confidence', '')}"
        )
    for reason in gate.get("rationale", []):
        lines.append(f"rationale={reason}")
    return "\n".join(lines) + "\n"


def runtime_proposal_gate_summary(gate: dict[str, Any]) -> str:
    return (
        "runtime_proposal_gate_summary"
        + f" proposal_gate_verdict={gate.get('proposal_gate_verdict', '')}"
        + f" proposal_confidence={gate.get('proposal_confidence', '')}"
        + f" execution_class_count={len(gate.get('per_execution_class_evidence', []))}\n"
    )


def write_runtime_proposal_gate_outputs(json_path: Path, gate: dict[str, Any]) -> None:
    write_json(json_path, gate)
    write_text(json_path.with_suffix(".txt"), runtime_proposal_gate_text(gate))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_proposal_gate_summary(gate))


def approve_runtime_rebaseline(
    current_manifest: dict[str, Any],
    current_manifest_path: Path,
    proposal: dict[str, Any],
    proposal_path: Path,
    proposal_gate: dict[str, Any],
    proposal_gate_path: Path,
    registry: dict[str, Any],
    registry_path: Path,
    baseline_out_path: Path,
    baseline_tag: str,
    activate: bool,
    archive_proposal_path: Path | None,
    require_acceptable_status: bool,
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any], dict[str, Any], dict[str, Any] | None]:
    current_manifest = normalize_runtime_current_manifest(json.loads(json.dumps(current_manifest)))
    if require_acceptable_status and current_manifest.get("overall_status") == STATUS_FAIL:
        raise RuntimeError("runtime rebaseline approval rejected FAIL current manifest")
    gate_verdict = str(proposal_gate.get("proposal_gate_verdict", "")).strip()
    if gate_verdict != "APPROVABLE":
        raise RuntimeError(f"runtime rebaseline approval rejected proposal gate verdict={gate_verdict or 'missing'}")
    if str(proposal.get("runtime_current_manifest_path", "")) not in {"", str(current_manifest_path)}:
        raise RuntimeError("runtime rebaseline approval rejected mismatched current manifest path")
    current_manifest_hash = sha256_file(current_manifest_path)
    if not bool(proposal.get("proposal_needed", True)):
        raise RuntimeError("runtime rebaseline approval rejected proposal that does not request rebaseline")
    proposal_current_manifest_hash = str(proposal.get("runtime_current_manifest_hash", "")).strip()
    if proposal_current_manifest_hash and proposal_current_manifest_hash != current_manifest_hash:
        raise RuntimeError("runtime rebaseline approval rejected stale current manifest hash")

    selection = select_runtime_baseline_from_registry(current_manifest, registry, current_manifest_path, registry_path)
    proposal_selected_id = str(proposal.get("selected_baseline_id", "")).strip()
    proposal_selected_tag = str(proposal.get("selected_baseline_tag", "")).strip()
    if proposal_selected_id and proposal_selected_id != str(selection.get("selected_baseline_id", "")):
        raise RuntimeError("runtime rebaseline approval rejected stale selected baseline id")
    if proposal_selected_tag and proposal_selected_tag != str(selection.get("selected_baseline_tag", "")):
        raise RuntimeError("runtime rebaseline approval rejected stale selected baseline tag")

    previous_active_entry = None if not selection.get("selected_baseline_id") else {
        "baseline_id": selection.get("selected_baseline_id"),
        "baseline_tag": selection.get("selected_baseline_tag"),
        "runtime_baseline_manifest_path": selection.get("selected_runtime_baseline_manifest_path"),
        "runtime_baseline_manifest_hash": selection.get("selected_runtime_baseline_manifest_hash"),
    }

    approved_baseline = promote_runtime_baseline(
        current_manifest,
        current_manifest_path,
        baseline_tag,
        require_acceptable_status,
    )
    approval_metadata = {
        "approval_version": "runtime_rebaseline_approval_v1",
        "approval_status": "approved",
        "approved_from_current_manifest": str(current_manifest_path),
        "approved_from_current_manifest_hash": current_manifest_hash,
        "approved_from_proposal": str(proposal_path),
        "approved_from_proposal_hash": sha256_file(proposal_path),
        "approved_from_proposal_gate": str(proposal_gate_path),
        "approved_from_proposal_gate_hash": sha256_file(proposal_gate_path),
        "approved_from_proposal_gate_verdict": gate_verdict,
        "proposal_confidence": proposal_gate.get("proposal_confidence"),
        "selected_previous_baseline": None if previous_active_entry is None else previous_active_entry.get("baseline_id"),
        "selected_previous_baseline_id": None if previous_active_entry is None else previous_active_entry.get("baseline_id"),
        "selected_previous_baseline_tag": None if previous_active_entry is None else previous_active_entry.get("baseline_tag"),
        "selected_previous_baseline_manifest_path": None if previous_active_entry is None else previous_active_entry.get("runtime_baseline_manifest_path"),
        "selected_previous_baseline_manifest_hash": None if previous_active_entry is None else previous_active_entry.get("runtime_baseline_manifest_hash"),
        "approval_timestamp_utc": approved_baseline.get("approval_timestamp_utc", timestamp_utc_now()),
        "baseline_tag": baseline_tag,
        "host_fingerprint": approved_baseline.get("host_fingerprint", {}),
        "toolchain_fingerprint": approved_baseline.get("toolchain_fingerprint", {}),
        "execution_classes_covered": runtime_execution_classes(approved_baseline),
        "registry_path": str(registry_path),
        "approved_runtime_baseline_path": str(baseline_out_path),
        "proposal_archive_path": None if archive_proposal_path is None else str(archive_proposal_path),
        "proposal_hash": proposal.get("proposal_hash"),
        "proposal_current_manifest_hash": proposal.get("runtime_current_manifest_hash"),
        "approval_current_manifest_hash": current_manifest_hash,
        "activate": bool(activate),
        "evidence_summary": dict(proposal_gate.get("evidence_summary", {})),
    }
    approved_baseline["approval_metadata"] = runtime_baseline_embedded_approval_metadata(approval_metadata)
    write_runtime_manifest_outputs(baseline_out_path, approved_baseline)

    updated_registry, registry_entry = promote_runtime_baseline_registry(
        registry,
        approved_baseline,
        baseline_out_path,
        baseline_tag,
        activate,
    )
    approval_metadata["new_active_runtime_baseline_id"] = registry_entry.get("baseline_id")
    approval_metadata["new_active_runtime_baseline_tag"] = registry_entry.get("baseline_tag")
    approval_metadata["new_active_runtime_baseline_manifest_hash"] = sha256_file(baseline_out_path)
    registry_entry["runtime_baseline_manifest_hash"] = approval_metadata["new_active_runtime_baseline_manifest_hash"]
    approval_metadata["previous_active_baseline_id_for_same_fingerprint"] = registry_entry.get("previous_active_baseline_id")
    approval_metadata["previous_active_runtime_baseline_id"] = approval_metadata.get("selected_previous_baseline")
    approval_metadata["runtime_transition_status"] = (
        "ACTIVE_SWITCHED"
        if approval_metadata.get("previous_active_baseline_id_for_same_fingerprint")
        and approval_metadata.get("previous_active_baseline_id_for_same_fingerprint") != approval_metadata.get("new_active_runtime_baseline_id")
        else "ACTIVE_CONFIRMED"
    )
    approved_baseline["approval_metadata"] = runtime_baseline_embedded_approval_metadata(approval_metadata)

    current_manifest["baseline_tag"] = baseline_tag
    current_manifest["approved_runtime_baseline_path"] = str(baseline_out_path)
    write_runtime_manifest_outputs(baseline_out_path, approved_baseline)
    finalized_baseline_hash = sha256_file(baseline_out_path)
    approval_metadata["new_active_runtime_baseline_manifest_hash"] = finalized_baseline_hash
    for candidate in updated_registry.get("entries", []):
        if str(candidate.get("baseline_id", "")) != str(approval_metadata.get("new_active_runtime_baseline_id", "")):
            continue
        candidate["runtime_baseline_manifest_hash"] = finalized_baseline_hash
        candidate["approval_timestamp_utc"] = approval_metadata["approval_timestamp_utc"]
        candidate["runtime_baseline_manifest_path"] = str(baseline_out_path)
        break
    updated_registry = finalize_runtime_registry(updated_registry)

    current_manifest["approved_runtime_baseline_hash"] = finalized_baseline_hash
    current_manifest["approval_metadata"] = approval_metadata
    current_manifest["approval_timestamp_utc"] = approval_metadata["approval_timestamp_utc"]
    current_manifest["selected_baseline_id"] = approval_metadata.get("new_active_runtime_baseline_id")
    current_manifest["selected_baseline_tag"] = approval_metadata.get("new_active_runtime_baseline_tag")
    current_manifest["current_runtime_manifest_hash"] = manifest_hash_without_field(current_manifest, "current_runtime_manifest_hash")

    archived_proposal = None
    if archive_proposal_path is not None:
        archived_proposal = archived_runtime_proposal(proposal, archive_proposal_path, approval_metadata)
    return current_manifest, approved_baseline, updated_registry, approval_metadata, archived_proposal


def empty_runtime_registry(registry_version: str = "runtime_baseline_registry_v1") -> dict[str, Any]:
    return {
        "registry_version": registry_version,
        "generated_at_utc": timestamp_utc_now(),
        "entries": [],
        "active_entry_count": 0,
        "retired_entry_count": 0,
    }


def finalize_runtime_registry(registry: dict[str, Any]) -> dict[str, Any]:
    registry["generated_at_utc"] = timestamp_utc_now()
    registry["active_entry_count"] = sum(
        1 for entry in registry.get("entries", []) if str(entry.get("status", REGISTRY_STATUS_RETIRED)) == REGISTRY_STATUS_ACTIVE
    )
    registry["retired_entry_count"] = sum(
        1 for entry in registry.get("entries", []) if str(entry.get("status", REGISTRY_STATUS_ACTIVE)) == REGISTRY_STATUS_RETIRED
    )
    registry["registry_hash"] = sha256_text(json.dumps(registry, sort_keys=True))
    return registry


def runtime_registry_text(registry: dict[str, Any]) -> str:
    lines = [
        f"registry_version={registry.get('registry_version', '')}",
        f"active_entry_count={registry.get('active_entry_count', 0)}",
        f"retired_entry_count={registry.get('retired_entry_count', 0)}",
    ]
    for entry in registry.get("entries", []):
        lines.append(
            "runtime_registry_entry="
            + f"baseline_id={entry.get('baseline_id', '')}"
            + f" baseline_tag={entry.get('baseline_tag', '')}"
            + f" status={entry.get('status', '')}"
            + f" host_fingerprint_hash={entry.get('host_fingerprint_hash', '')}"
            + f" build_classes={','.join(entry.get('build_classes_covered', []))}"
        )
    return "\n".join(lines) + "\n"


def runtime_registry_summary(registry: dict[str, Any]) -> str:
    return (
        "runtime_baseline_registry_summary"
        + f" active_entry_count={registry.get('active_entry_count', 0)}"
        + f" retired_entry_count={registry.get('retired_entry_count', 0)}"
        + f" entry_count={len(registry.get('entries', []))}\n"
    )


def write_runtime_registry_outputs(json_path: Path, registry: dict[str, Any]) -> None:
    write_json(json_path, registry)
    write_text(json_path.with_suffix(".txt"), runtime_registry_text(registry))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_registry_summary(registry))
    write_json(json_path.with_name(f"{json_path.stem}_summary.json"), runtime_registry_summary_payload(registry))


def load_runtime_registry(path: Path | None) -> dict[str, Any]:
    if path is None or not path.exists():
        return empty_runtime_registry(runtime_registry_version_for_path(path))
    data = read_json(path)
    if not isinstance(data.get("entries", []), list):
        raise RuntimeError(f"invalid runtime registry payload: {path}")
    if not str(data.get("registry_version", "")).strip():
        data["registry_version"] = runtime_registry_version_for_path(path)
    return finalize_runtime_registry(data)


def build_runtime_registry_entry(
    baseline_manifest: dict[str, Any],
    baseline_manifest_path: Path,
    baseline_tag: str,
    activate: bool,
) -> dict[str, Any]:
    validate_runtime_baseline_manifest(baseline_manifest, baseline_manifest_path)
    host = dict(baseline_manifest.get("host_fingerprint", {}))
    toolchain = dict(baseline_manifest.get("toolchain_fingerprint", {}))
    execution_classes = runtime_execution_classes(baseline_manifest)
    execution_signatures = runtime_execution_signatures(baseline_manifest)
    execution_signature_map = runtime_execution_signature_map(baseline_manifest)
    approval_timestamp = str(baseline_manifest.get("approval_timestamp_utc") or timestamp_utc_now())
    host_hash = str(host.get("fingerprint_hash", ""))
    toolchain_hash = toolchain_fingerprint_hash(toolchain)
    baseline_id = (
        f"{baseline_tag}-{host_hash[:8] or 'host'}-{approval_timestamp.replace(':', '').replace('-', '').replace('T', '_').replace('Z', '')}"
    )
    return {
        "baseline_id": baseline_id,
        "baseline_tag": baseline_tag,
        "approval_timestamp_utc": approval_timestamp,
        "runtime_baseline_manifest_path": str(baseline_manifest_path),
        "runtime_baseline_manifest_hash": sha256_file(baseline_manifest_path),
        "host_fingerprint": host,
        "toolchain_fingerprint": toolchain,
        "host_fingerprint_hash": host_hash,
        "toolchain_fingerprint_hash": toolchain_hash,
        "runtime_fingerprint_key": runtime_fingerprint_key(host, toolchain, execution_signatures),
        "fingerprint_key": runtime_fingerprint_key(host, toolchain, execution_signatures),
        "execution_signature_hash": sha256_text(",".join(execution_signatures)),
        "build_classes_covered": execution_classes,
        "execution_classes_covered": execution_classes,
        "execution_class_signatures": execution_signature_map,
        "runtime_budget_profile_id": str(
            baseline_manifest.get("runtime_budget_profile_id", baseline_manifest.get("baseline_tag", baseline_tag))
            or baseline_tag
        ),
        "runtime_budget_profile_version": baseline_manifest.get("runtime_budget_profile_version"),
        "manifest_role": str(baseline_manifest.get("manifest_role", "")),
        "provenance_frozen": bool(baseline_manifest.get("provenance_frozen", False)),
        "approved_from_current_manifest": str(baseline_manifest.get("promoted_from_manifest", "")) or None,
        "previous_active_baseline_id": None,
        "supersedes_baseline_ids": [],
        "superseded_by_baseline_id": None,
        "status": REGISTRY_STATUS_ACTIVE if activate else REGISTRY_STATUS_RETIRED,
    }


def promote_runtime_baseline_registry(
    registry: dict[str, Any],
    baseline_manifest: dict[str, Any],
    baseline_manifest_path: Path,
    baseline_tag: str,
    activate: bool,
) -> tuple[dict[str, Any], dict[str, Any]]:
    entry = build_runtime_registry_entry(baseline_manifest, baseline_manifest_path, baseline_tag, activate)
    retired_same_fingerprint_ids: list[str] = []
    existing = None
    for candidate in registry.get("entries", []):
        if (
            str(candidate.get("runtime_baseline_manifest_hash", "")) == str(entry.get("runtime_baseline_manifest_hash", ""))
            and str(candidate.get("runtime_fingerprint_key", "")) == str(entry.get("runtime_fingerprint_key", ""))
        ):
            existing = candidate
            break
    if existing is not None:
        existing.update(entry)
        entry = existing
    else:
        registry.setdefault("entries", []).append(entry)

    if activate:
        for candidate in registry.get("entries", []):
            if candidate is entry:
                continue
            if (
                str(candidate.get("status", REGISTRY_STATUS_RETIRED)) == REGISTRY_STATUS_ACTIVE
                and str(candidate.get("runtime_fingerprint_key", "")) == str(entry.get("runtime_fingerprint_key", ""))
            ):
                retired_same_fingerprint_ids.append(str(candidate.get("baseline_id", "")))
                candidate["status"] = REGISTRY_STATUS_RETIRED
                candidate["retired_timestamp_utc"] = timestamp_utc_now()
                candidate["retired_reason"] = f"superseded by {entry['baseline_id']}"
                candidate["superseded_by_baseline_id"] = entry["baseline_id"]
    if retired_same_fingerprint_ids:
        entry["previous_active_baseline_id"] = retired_same_fingerprint_ids[-1]
        entry["supersedes_baseline_ids"] = retired_same_fingerprint_ids
    entry["status"] = REGISTRY_STATUS_ACTIVE if activate else str(entry.get("status", REGISTRY_STATUS_RETIRED))
    return finalize_runtime_registry(registry), entry


def retire_runtime_registry_entry(
    registry: dict[str, Any],
    baseline_id: str,
) -> tuple[dict[str, Any], dict[str, Any] | None]:
    retired_entry = None
    for entry in registry.get("entries", []):
        if str(entry.get("baseline_id", "")) != baseline_id:
            continue
        entry["status"] = REGISTRY_STATUS_RETIRED
        entry["retired_timestamp_utc"] = timestamp_utc_now()
        entry.setdefault("retired_reason", "retired by operator action")
        retired_entry = entry
        break
    return finalize_runtime_registry(registry), retired_entry


def runtime_candidate_supports_current(entry: dict[str, Any], current_manifest: dict[str, Any]) -> bool:
    candidate_classes = set(str(value) for value in entry.get("build_classes_covered", []))
    current_classes = set(runtime_execution_classes(current_manifest))
    return current_classes.issubset(candidate_classes)


def runtime_candidate_compatible(entry: dict[str, Any], current_manifest: dict[str, Any]) -> bool:
    host = dict(entry.get("host_fingerprint", {}))
    toolchain = dict(entry.get("toolchain_fingerprint", {}))
    current_host = dict(current_manifest.get("host_fingerprint", {}))
    current_toolchain = dict(current_manifest.get("toolchain_fingerprint", {}))
    return (
        runtime_candidate_supports_current(entry, current_manifest)
        and str(host.get("os", "")) == str(current_host.get("os", ""))
        and str(host.get("arch", "")) == str(current_host.get("arch", ""))
        and str(toolchain.get("compiler_id", "")) == str(current_toolchain.get("compiler_id", ""))
    )


def runtime_compatibility_reason(entry: dict[str, Any], current_manifest: dict[str, Any]) -> str:
    host = dict(entry.get("host_fingerprint", {}))
    toolchain = dict(entry.get("toolchain_fingerprint", {}))
    current_host = dict(current_manifest.get("host_fingerprint", {}))
    current_toolchain = dict(current_manifest.get("toolchain_fingerprint", {}))
    if str(host.get("os", "")) != str(current_host.get("os", "")) or str(host.get("arch", "")) != str(current_host.get("arch", "")):
        return "cross-host runtime baseline candidate"
    if str(toolchain.get("compiler_id", "")) != str(current_toolchain.get("compiler_id", "")):
        return "runtime baseline candidate uses a different compiler family"
    if str(toolchain.get("compiler_version", "")) != str(current_toolchain.get("compiler_version", "")):
        return "same host runtime baseline candidate differs by compiler version"
    if str(host.get("runner_tag", "")) != str(current_host.get("runner_tag", "")):
        return "same host runtime baseline candidate differs by runner tag"
    if str(entry.get("execution_signature_hash", "")) != sha256_text(",".join(runtime_execution_signatures(current_manifest))):
        return "same host runtime baseline candidate differs by build or sanitizer signature"
    return "compatible active runtime baseline differs in host/toolchain details"


def runtime_selection_sort_key(entry: dict[str, Any]) -> tuple[str, str]:
    return (
        str(entry.get("approval_timestamp_utc", "")),
        str(entry.get("baseline_id", "")),
    )


def select_runtime_baseline_from_registry(
    current_manifest: dict[str, Any],
    registry: dict[str, Any],
    current_manifest_path: Path,
    registry_path: Path,
) -> dict[str, Any]:
    current_host = dict(current_manifest.get("host_fingerprint", {}))
    current_toolchain = dict(current_manifest.get("toolchain_fingerprint", {}))
    current_classes = runtime_execution_classes(current_manifest)
    current_signatures = runtime_execution_signatures(current_manifest)
    current_fingerprint_key = runtime_fingerprint_key(current_host, current_toolchain, current_signatures)

    exact_matches: list[dict[str, Any]] = []
    compatible_matches: list[dict[str, Any]] = []
    retired_matches: list[dict[str, Any]] = []
    candidate_details: list[dict[str, Any]] = []
    for entry in registry.get("entries", []):
        if not isinstance(entry, dict):
            continue
        manifest_valid, validation_reason = runtime_registry_entry_manifest_status(entry)
        if not manifest_valid:
            candidate_details.append(
                {
                    "baseline_id": entry.get("baseline_id"),
                    "baseline_tag": entry.get("baseline_tag"),
                    "status": entry.get("status"),
                    "match_kind": "invalid",
                    "match_reason": validation_reason,
                }
            )
            continue
        entry_status = str(entry.get("status", REGISTRY_STATUS_RETIRED))
        entry_key = str(entry.get("runtime_fingerprint_key", ""))
        if entry_status == REGISTRY_STATUS_ACTIVE and entry_key == current_fingerprint_key:
            exact_matches.append(entry)
            candidate_details.append(
                {
                    "baseline_id": entry.get("baseline_id"),
                    "baseline_tag": entry.get("baseline_tag"),
                    "status": entry_status,
                    "match_kind": "exact",
                    "match_reason": "active exact runtime fingerprint match",
                }
            )
            continue
        if entry_status == REGISTRY_STATUS_ACTIVE and runtime_candidate_compatible(entry, current_manifest):
            compatible_matches.append(entry)
            candidate_details.append(
                {
                    "baseline_id": entry.get("baseline_id"),
                    "baseline_tag": entry.get("baseline_tag"),
                    "status": entry_status,
                    "match_kind": "compatible",
                    "match_reason": runtime_compatibility_reason(entry, current_manifest),
                }
            )
            continue
        if entry_status == REGISTRY_STATUS_RETIRED and runtime_candidate_compatible(entry, current_manifest):
            retired_matches.append(entry)
            candidate_details.append(
                {
                    "baseline_id": entry.get("baseline_id"),
                    "baseline_tag": entry.get("baseline_tag"),
                    "status": entry_status,
                    "match_kind": "retired",
                    "match_reason": runtime_compatibility_reason(entry, current_manifest),
                }
            )

    selected_entry = None
    comparability_verdict = REBASELINE_REQUIRED
    comparability_reason = "no compatible active runtime baseline in registry"

    if exact_matches:
        selected_entry = max(exact_matches, key=runtime_selection_sort_key)
        comparability_verdict = COMPARABLE
        comparability_reason = "active exact runtime fingerprint match"
    elif compatible_matches:
        selected_entry = max(compatible_matches, key=runtime_selection_sort_key)
        comparability_verdict = NOT_COMPARABLE
        comparability_reason = runtime_compatibility_reason(selected_entry, current_manifest)
    elif retired_matches:
        selected_entry = max(retired_matches, key=runtime_selection_sort_key)
        comparability_verdict = REBASELINE_REQUIRED
        comparability_reason = "only retired runtime baselines match the current host/toolchain"

    selection = {
        "selection_version": "runtime_baseline_selection_v1",
        "generated_at_utc": timestamp_utc_now(),
        "runtime_current_manifest_path": str(current_manifest_path),
        "runtime_baseline_registry_path": str(registry_path),
        "candidate_count": len(exact_matches) + len(compatible_matches) + len(retired_matches),
        "exact_match_count": len(exact_matches),
        "compatible_match_count": len(compatible_matches),
        "retired_match_count": len(retired_matches),
        "current_host_fingerprint": current_host,
        "current_toolchain_fingerprint": current_toolchain,
        "current_execution_classes": current_classes,
        "current_execution_signatures": current_signatures,
        "current_fingerprint_key": current_fingerprint_key,
        "comparability_verdict": comparability_verdict,
        "comparability_reason": comparability_reason,
        "selected_baseline_id": None if selected_entry is None else selected_entry.get("baseline_id"),
        "selected_baseline_tag": None if selected_entry is None else selected_entry.get("baseline_tag"),
        "selected_runtime_baseline_manifest_path": None
        if selected_entry is None
        else selected_entry.get("runtime_baseline_manifest_path"),
        "selected_runtime_baseline_manifest_hash": None
        if selected_entry is None
        else selected_entry.get("runtime_baseline_manifest_hash"),
        "selected_entry_status": None if selected_entry is None else selected_entry.get("status"),
        "selected_execution_signature_hash": None if selected_entry is None else selected_entry.get("execution_signature_hash"),
        "candidates": candidate_details,
    }
    selection["selection_hash"] = sha256_text(json.dumps(selection, sort_keys=True))
    return selection


def selection_baseline_manifest(selection: dict[str, Any] | None) -> tuple[dict[str, Any] | None, Path | None]:
    if not selection:
        return None, None
    baseline_path_text = str(selection.get("selected_runtime_baseline_manifest_path", "")).strip()
    if not baseline_path_text:
        return None, None
    baseline_path = Path(baseline_path_text).resolve()
    if not baseline_path.exists():
        return None, None
    baseline_manifest = read_json(baseline_path)
    try:
        validate_runtime_baseline_manifest(baseline_manifest, baseline_path)
    except RuntimeError:
        return None, None
    selected_hash_value = selection.get("selected_runtime_baseline_manifest_hash")
    selected_hash = "" if selected_hash_value in {None, "", "None"} else str(selected_hash_value).strip()
    if selected_hash and selected_hash != sha256_file(baseline_path):
        return None, None
    return baseline_manifest, baseline_path


def refresh_runtime_manifest(
    baseline_manifest: dict[str, Any] | None,
    current_manifest: dict[str, Any],
    baseline_manifest_path: Path | None,
    current_manifest_path: Path,
    baseline_selection: dict[str, Any] | None = None,
    runtime_registry_path: Path | None = None,
) -> dict[str, Any]:
    baseline_entries = {}
    baseline_host_hash = ""
    if baseline_manifest:
        baseline_entries = {
            str(entry.get("execution_class")): entry for entry in baseline_manifest.get("entries", []) if entry.get("execution_class")
        }
        baseline_host_hash = str(baseline_manifest.get("host_fingerprint", {}).get("fingerprint_hash", ""))
    current_host_hash = str(current_manifest.get("host_fingerprint", {}).get("fingerprint_hash", ""))

    entries: list[dict[str, Any]] = []
    stale_count = 0
    requires_rerun_count = 0
    rebaseline_required_count = 0
    not_comparable_count = 0
    current_statuses: list[str] = []
    selected_baseline_id = None if baseline_selection is None else baseline_selection.get("selected_baseline_id")
    selected_baseline_tag = None if baseline_selection is None else baseline_selection.get("selected_baseline_tag")
    selection_reason = "" if baseline_selection is None else str(baseline_selection.get("comparability_reason", ""))
    selection_verdict = "" if baseline_selection is None else str(baseline_selection.get("comparability_verdict", ""))

    for current_entry in current_manifest.get("entries", []):
        execution_class = str(current_entry.get("execution_class", ""))
        baseline_entry = baseline_entries.get(execution_class)
        current_status = str(current_entry.get("current_status", STATUS_OK))
        current_statuses.append(current_status)
        comparability = COMPARABLE
        freshness_status = FRESHNESS_FRESH
        rationale = "runtime entry remained within baseline budget"
        baseline_wall = None
        delta_percent = None

        if baseline_manifest is None or baseline_entry is None:
            if selection_verdict == NOT_COMPARABLE:
                comparability = NOT_COMPARABLE
                freshness_status = FRESHNESS_NOT_COMPARABLE
                rationale = selection_reason or "runtime baseline is only compatible, not directly comparable"
                not_comparable_count += 1
            else:
                comparability = FRESHNESS_REBASELINE_REQUIRED
                freshness_status = FRESHNESS_REBASELINE_REQUIRED
                rationale = selection_reason or "runtime baseline missing matching execution class"
                rebaseline_required_count += 1
                not_comparable_count += 1
        else:
            baseline_entry_fingerprint = str(baseline_entry.get("fingerprint_hash", ""))
            current_entry_fingerprint = str(current_entry.get("fingerprint_hash", ""))
            same_host = baseline_host_hash == current_host_hash and baseline_host_hash != ""
            same_build = str(baseline_entry.get("build_type", "")) == str(current_entry.get("build_type", ""))
            same_sanitizer = str(baseline_entry.get("sanitizer_flags", "")) == str(current_entry.get("sanitizer_flags", ""))
            if selection_verdict == REBASELINE_REQUIRED:
                comparability = FRESHNESS_REBASELINE_REQUIRED
                freshness_status = FRESHNESS_REBASELINE_REQUIRED
                rationale = selection_reason or "runtime baseline selection requires rebaseline"
                rebaseline_required_count += 1
                not_comparable_count += 1
            elif selection_verdict == NOT_COMPARABLE:
                comparability = NOT_COMPARABLE
                freshness_status = FRESHNESS_NOT_COMPARABLE
                rationale = selection_reason or runtime_compatibility_reason(
                    {
                        "host_fingerprint": baseline_manifest.get("host_fingerprint", {}) if baseline_manifest else {},
                        "toolchain_fingerprint": baseline_manifest.get("toolchain_fingerprint", {}) if baseline_manifest else {},
                        "execution_signature_hash": sha256_text(",".join(runtime_execution_signatures(baseline_manifest or {}))),
                    },
                    current_manifest,
                )
                not_comparable_count += 1
            elif not baseline_host_hash or not current_host_hash or not baseline_entry_fingerprint or not current_entry_fingerprint:
                comparability = FRESHNESS_NOT_COMPARABLE
                freshness_status = FRESHNESS_NOT_COMPARABLE
                rationale = "runtime fingerprint provenance is incomplete"
                not_comparable_count += 1
            elif not (same_host and same_build and same_sanitizer):
                if selection_verdict == REBASELINE_REQUIRED and not same_host:
                    comparability = FRESHNESS_REBASELINE_REQUIRED
                    freshness_status = FRESHNESS_REBASELINE_REQUIRED
                    rationale = selection_reason or "runtime host fingerprint mismatch requires runtime rebaseline"
                    rebaseline_required_count += 1
                    not_comparable_count += 1
                else:
                    comparability = NOT_COMPARABLE
                    freshness_status = FRESHNESS_NOT_COMPARABLE
                    rationale = selection_reason or (
                        "runtime build or sanitizer profile is not directly comparable"
                        if not (same_build and same_sanitizer)
                        else "runtime toolchain or runner fingerprint mismatch is not directly comparable"
                    )
                    not_comparable_count += 1
            else:
                baseline_wall = float(baseline_entry.get("wall_time_sec", 0.0))
                threshold = dict(current_entry.get("budget_thresholds") or default_runtime_threshold(execution_class))
                delta_percent = 0.0 if baseline_wall == 0.0 else ((float(current_entry.get("wall_time_sec", 0.0)) - baseline_wall) / baseline_wall) * 100.0
                delta_seconds = float(current_entry.get("wall_time_sec", 0.0)) - baseline_wall
                if (
                    delta_percent > float(threshold.get("hard_delta_percent", 0.0))
                    and delta_seconds > float(threshold.get("hard_delta_floor_sec", 0.0))
                ):
                    freshness_status = FRESHNESS_REQUIRES_RERUN
                    rationale = "runtime delta exceeded hard budget"
                    requires_rerun_count += 1
                elif (
                    delta_percent > float(threshold.get("soft_delta_percent", 0.0))
                    and delta_seconds > float(threshold.get("soft_delta_floor_sec", 0.0))
                ):
                    freshness_status = FRESHNESS_STALE
                    rationale = "runtime delta exceeded soft budget"
                    stale_count += 1
                else:
                    freshness_status = FRESHNESS_FRESH
                    rationale = "runtime remained within baseline budget"
                current_entry["delta_percent"] = round(delta_percent, 2)
                current_entry["rationale"] = rationale

        entries.append(
            {
                "execution_class": execution_class,
                "wall_time_sec": current_entry.get("wall_time_sec", 0.0),
                "baseline_wall_time_sec": baseline_wall,
                "test_count": current_entry.get("test_count", 0),
                "build_type": current_entry.get("build_type", ""),
                "sanitizer_flags": current_entry.get("sanitizer_flags", ""),
                "current_status": current_entry.get("current_status", current_status),
                "freshness_status": freshness_status,
                "comparability": comparability,
                "delta_percent": delta_percent,
                "budget_thresholds": current_entry.get("budget_thresholds", {}),
                "rationale": rationale,
            }
        )

    if rebaseline_required_count:
        freshness_verdict = FRESHNESS_REBASELINE_REQUIRED
    elif not_comparable_count:
        freshness_verdict = FRESHNESS_NOT_COMPARABLE
    elif requires_rerun_count:
        freshness_verdict = FRESHNESS_REQUIRES_RERUN
    elif stale_count:
        freshness_verdict = FRESHNESS_STALE
    else:
        freshness_verdict = FRESHNESS_FRESH

    if STATUS_FAIL in current_statuses:
        current_verdict = VERDICT_FAIL
    elif STATUS_WARN in current_statuses:
        current_verdict = VERDICT_WARN
    else:
        current_verdict = VERDICT_PASS

    if rebaseline_required_count:
        comparability_verdict = FRESHNESS_REBASELINE_REQUIRED
    elif not_comparable_count:
        comparability_verdict = FRESHNESS_NOT_COMPARABLE
    else:
        comparability_verdict = COMPARABLE

    if current_verdict == VERDICT_FAIL:
        runtime_severity = STATUS_FAIL
        runtime_recommendation = "runtime exceeded the hard budget; inspect wall-clock regression before approval"
    elif comparability_verdict in {FRESHNESS_REBASELINE_REQUIRED, FRESHNESS_NOT_COMPARABLE}:
        runtime_severity = "ACTION_REQUIRED"
        runtime_recommendation = (
            "runtime baseline is not comparable; rerun on a matching host/toolchain or promote a new runtime baseline"
        )
    elif freshness_verdict in {FRESHNESS_STALE, FRESHNESS_REQUIRES_RERUN}:
        runtime_severity = "ACTION_REQUIRED"
        runtime_recommendation = "runtime refresh requires rerun or revalidation before treating the ops state as healthy"
    elif current_verdict == VERDICT_WARN:
        runtime_severity = STATUS_WARN
        runtime_recommendation = "runtime exceeded the soft budget; correctness is stable but the run is slower than baseline"
    else:
        runtime_severity = STATUS_OK
        runtime_recommendation = "runtime budget is within the approved baseline"

    refresh_manifest = {
        "manifest_version": "runtime_refresh_manifest_v1",
        "manifest_role": "refresh",
        "generated_at_utc": timestamp_utc_now(),
        "baseline_runtime_manifest_path": None if baseline_manifest_path is None else str(baseline_manifest_path),
        "current_runtime_manifest_path": str(current_manifest_path),
        "runtime_baseline_registry_path": None if runtime_registry_path is None else str(runtime_registry_path),
        "baseline_runtime_manifest_hash": sha256_file(baseline_manifest_path),
        "current_runtime_manifest_hash": sha256_file(current_manifest_path),
        "phase": current_manifest.get("phase", ""),
        "artifact_root": current_manifest.get("artifact_root", ""),
        "baseline_tag": current_manifest.get("baseline_tag", ""),
        "host_fingerprint": current_manifest.get("host_fingerprint", {}),
        "toolchain_fingerprint": current_manifest.get("toolchain_fingerprint", {}),
        "current_verdict": current_verdict,
        "freshness_verdict": freshness_verdict,
        "comparability_verdict": comparability_verdict,
        "selected_baseline_id": selected_baseline_id,
        "selected_baseline_tag": selected_baseline_tag,
        "selected_runtime_baseline_manifest_hash": None if baseline_manifest_path is None else sha256_file(baseline_manifest_path),
        "selection_candidate_count": 0 if baseline_selection is None else int(baseline_selection.get("candidate_count", 0)),
        "selection_exact_match_count": 0 if baseline_selection is None else int(baseline_selection.get("exact_match_count", 0)),
        "selection_compatible_match_count": 0 if baseline_selection is None else int(baseline_selection.get("compatible_match_count", 0)),
        "selection_retired_match_count": 0 if baseline_selection is None else int(baseline_selection.get("retired_match_count", 0)),
        "comparability_reason": selection_reason,
        "overall_budget_verdict": current_manifest.get("overall_budget_verdict", "UNKNOWN"),
        "runtime_severity": runtime_severity,
        "recommended_next_action": runtime_recommendation,
        "stale_entry_count": stale_count,
        "requires_rerun_entry_count": requires_rerun_count,
        "rebaseline_required_count": rebaseline_required_count,
        "not_comparable_count": not_comparable_count,
        "warn_count": sum(1 for value in current_statuses if value == STATUS_WARN),
        "fail_count": sum(1 for value in current_statuses if value == STATUS_FAIL),
        "entries": entries,
    }
    refresh_manifest["refresh_manifest_hash"] = manifest_hash_without_field(refresh_manifest, "refresh_manifest_hash")
    return refresh_manifest


def runtime_refresh_text(manifest: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={manifest.get('manifest_version', '')}",
        f"phase={manifest.get('phase', '')}",
        f"current_verdict={manifest.get('current_verdict', '')}",
        f"freshness_verdict={manifest.get('freshness_verdict', '')}",
        f"comparability_verdict={manifest.get('comparability_verdict', '')}",
        f"selected_baseline_id={manifest.get('selected_baseline_id', '')}",
        f"selected_baseline_tag={manifest.get('selected_baseline_tag', '')}",
        f"overall_budget_verdict={manifest.get('overall_budget_verdict', '')}",
        f"runtime_severity={manifest.get('runtime_severity', '')}",
        f"stale_entry_count={manifest.get('stale_entry_count', 0)}",
        f"requires_rerun_entry_count={manifest.get('requires_rerun_entry_count', 0)}",
        f"rebaseline_required_count={manifest.get('rebaseline_required_count', 0)}",
        f"not_comparable_count={manifest.get('not_comparable_count', 0)}",
    ]
    for entry in manifest.get("entries", []):
        lines.append(
            "runtime_refresh_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" current_status={entry.get('current_status', '')}"
            + f" freshness_status={entry.get('freshness_status', '')}"
            + f" comparability={entry.get('comparability', '')}"
        )
    return "\n".join(lines) + "\n"


def runtime_refresh_summary(manifest: dict[str, Any]) -> str:
    return (
        "runtime_refresh_summary"
        + f" current_verdict={manifest.get('current_verdict', '')}"
        + f" freshness_verdict={manifest.get('freshness_verdict', '')}"
        + f" comparability_verdict={manifest.get('comparability_verdict', '')}"
        + f" selected_baseline_id={manifest.get('selected_baseline_id', '')}"
        + f" overall_budget_verdict={manifest.get('overall_budget_verdict', '')}"
        + f" runtime_severity={manifest.get('runtime_severity', '')}"
        + f" stale_entry_count={manifest.get('stale_entry_count', 0)}"
        + f" requires_rerun_entry_count={manifest.get('requires_rerun_entry_count', 0)}"
        + f" rebaseline_required_count={manifest.get('rebaseline_required_count', 0)}\n"
    )


def write_runtime_refresh_outputs(json_path: Path, manifest: dict[str, Any]) -> None:
    write_json(json_path, manifest)
    write_text(json_path.with_suffix(".txt"), runtime_refresh_text(manifest))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_refresh_summary(manifest))


def rerun_kind_for_execution_class(execution_class: str) -> str:
    if execution_class in {"release_full", "debug_full", "asan_full", "compare_campaign"}:
        return f"{execution_class}_rerun"
    if execution_class in {"policy_core", "policy_refresh", "policy_nightly"}:
        return f"{execution_class}_rerun"
    return "runtime_entry_rerun"


def recommended_runtime_command(execution_class: str) -> str:
    if execution_class == "release_full":
        return "ctest --test-dir build-release-phase23 --output-on-failure"
    if execution_class == "debug_full":
        return "ctest --test-dir build-debug-phase23 --output-on-failure"
    if execution_class == "asan_full":
        return "ctest --test-dir build-asan-phase23 --output-on-failure"
    if execution_class == "policy_core":
        return "python tests/tools/run_policy_pipeline.py --mode quick --strict"
    if execution_class in {"policy_refresh", "policy_nightly"}:
        return "python tests/tools/run_policy_pipeline.py --mode nightly --strict"
    if execution_class == "compare_campaign":
        return "./raw_engine_tests --case campaign --stop-when-gate-passes"
    return f"re-run {execution_class}"


def build_runtime_rerun_plan(
    refresh_manifest: dict[str, Any],
    baseline_manifest_path: Path | None,
    current_manifest_path: Path,
    refresh_manifest_path: Path,
) -> dict[str, Any]:
    entries: list[dict[str, Any]] = []
    for entry in refresh_manifest.get("entries", []):
        freshness_status = str(entry.get("freshness_status", ""))
        if freshness_status not in {FRESHNESS_STALE, FRESHNESS_REQUIRES_RERUN, FRESHNESS_REBASELINE_REQUIRED}:
            continue
        execution_class = str(entry.get("execution_class", ""))
        rerun_kind = "rebaseline_required" if freshness_status == FRESHNESS_REBASELINE_REQUIRED else rerun_kind_for_execution_class(execution_class)
        entries.append(
            {
                "execution_class": execution_class,
                "current_status": entry.get("current_status", STATUS_OK),
                "freshness_status": freshness_status,
                "comparability": entry.get("comparability", COMPARABLE),
                "rerun_kind": rerun_kind,
                "recommended_command": recommended_runtime_command(execution_class),
                "expected_stop_criteria": "wall_time within soft budget" if freshness_status == FRESHNESS_STALE else "wall_time within hard budget",
                "status_impact": f"{execution_class} runtime requires refresh",
            }
        )
    summary_verdict = "PASS" if not entries else "ACTION_REQUIRED"
    plan = {
        "plan_version": "runtime_rerun_plan_v1",
        "generated_at_utc": timestamp_utc_now(),
        "artifact_root": refresh_manifest.get("artifact_root", ""),
        "baseline_runtime_manifest_path": None if baseline_manifest_path is None else str(baseline_manifest_path),
        "current_runtime_manifest_path": str(current_manifest_path),
        "refresh_runtime_manifest_path": str(refresh_manifest_path),
        "baseline_runtime_manifest_hash": sha256_file(baseline_manifest_path),
        "current_runtime_manifest_hash": sha256_file(current_manifest_path),
        "refresh_runtime_manifest_hash": sha256_file(refresh_manifest_path),
        "stale_entry_count": int(refresh_manifest.get("stale_entry_count", 0)),
        "requires_rerun_entry_count": int(refresh_manifest.get("requires_rerun_entry_count", 0)),
        "rebaseline_required_count": int(refresh_manifest.get("rebaseline_required_count", 0)),
        "selected_entry_count": len(entries),
        "entries": entries,
        "summary_verdict": summary_verdict,
        "rationale": "runtime entries require refresh" if entries else "no stale or over-budget runtime entries",
    }
    return plan


def runtime_rerun_plan_text(plan: dict[str, Any]) -> str:
    lines = [
        f"plan_version={plan.get('plan_version', '')}",
        f"summary_verdict={plan.get('summary_verdict', '')}",
        f"stale_entry_count={plan.get('stale_entry_count', 0)}",
        f"requires_rerun_entry_count={plan.get('requires_rerun_entry_count', 0)}",
        f"rebaseline_required_count={plan.get('rebaseline_required_count', 0)}",
        f"selected_entry_count={plan.get('selected_entry_count', 0)}",
    ]
    for entry in plan.get("entries", []):
        lines.append(
            "runtime_rerun_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" freshness_status={entry.get('freshness_status', '')}"
            + f" rerun_kind={entry.get('rerun_kind', '')}"
        )
    return "\n".join(lines) + "\n"


def runtime_rerun_plan_summary(plan: dict[str, Any]) -> str:
    return (
        "runtime_rerun_plan_summary"
        + f" summary_verdict={plan.get('summary_verdict', '')}"
        + f" selected_entry_count={plan.get('selected_entry_count', 0)}"
        + f" stale_entry_count={plan.get('stale_entry_count', 0)}"
        + f" requires_rerun_entry_count={plan.get('requires_rerun_entry_count', 0)}"
        + f" rebaseline_required_count={plan.get('rebaseline_required_count', 0)}\n"
    )


def write_runtime_rerun_plan_outputs(json_path: Path, plan: dict[str, Any]) -> None:
    write_json(json_path, plan)
    write_text(json_path.with_suffix(".txt"), runtime_rerun_plan_text(plan))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_rerun_plan_summary(plan))


def infer_runtime_refresh_path(current_manifest_path: Path) -> Path | None:
    stem = current_manifest_path.stem
    if "_current_" in stem:
        candidate = current_manifest_path.with_name(stem.replace("_current_", "_refresh_") + ".json")
        if candidate.exists():
            return candidate
    candidate = current_manifest_path.with_name(f"{current_manifest_path.stem}_refresh.json")
    return candidate if candidate.exists() else None


def percentile_value(samples: list[float], percentile: float) -> float:
    if not samples:
        return 0.0
    ordered = sorted(samples)
    index = max(0, min(len(ordered) - 1, int(round((percentile / 100.0) * (len(ordered) - 1)))))
    return ordered[index]


def budget_verdict_for_status(current_status: str) -> str:
    if current_status == STATUS_FAIL:
        return "BUDGET_FAIL"
    if current_status == STATUS_WARN:
        return "BUDGET_WARN"
    return "PASS"


def summarize_runtime_sample_series(samples: list[dict[str, Any]]) -> dict[str, Any]:
    wall_times = [float(sample.get("wall_time_sec", 0.0)) for sample in samples]
    sample_count = len(wall_times)
    median = statistics.median(wall_times) if wall_times else 0.0
    deviations = [abs(value - median) for value in wall_times]
    mad = statistics.median(deviations) if deviations else 0.0
    latest = samples[-1] if samples else {}
    latest_wall = float(latest.get("wall_time_sec", 0.0))
    jitter_percent = 0.0 if median <= 0.0 else round((mad / median) * 100.0, 2)
    latest_delta = latest.get("delta_percent")
    latest_status = str(latest.get("current_status", STATUS_OK))
    latest_budget_verdict = budget_verdict_for_status(latest_status)
    previous_wall = float(samples[-2].get("wall_time_sec", latest_wall)) if sample_count >= 2 else latest_wall
    latest_baseline_tag = str(latest.get("baseline_tag", ""))
    previous_baseline_tag = str(samples[-2].get("baseline_tag", latest_baseline_tag)) if sample_count >= 2 else latest_baseline_tag
    trend = "insufficient_history"
    if sample_count >= 3:
        trend = "stable"
        if latest_baseline_tag != previous_baseline_tag and latest_wall <= median * 0.95:
            trend = "improved"
        elif latest_wall <= previous_wall * 0.9 and latest_wall <= median * 0.95:
            trend = "improved"
        elif latest_wall >= median * 1.2 and latest_wall >= previous_wall:
            trend = "regressing"
        elif jitter_percent > 15.0:
            trend = "noisy"
    return {
        "sample_count": sample_count,
        "latest_wall_time_sec": round(latest_wall, 3),
        "baseline_wall_time_sec": latest.get("baseline_wall_time_sec"),
        "delta_vs_selected_baseline_percent": latest_delta,
        "latest_delta_percent": latest_delta,
        "latest_freshness_status": latest.get("freshness_status"),
        "latest_comparability": latest.get("comparability"),
        "rolling_median_wall_time_sec": round(median, 3),
        "rolling_p90_wall_time_sec": round(percentile_value(wall_times, 90.0), 3),
        "rolling_p95_wall_time_sec": round(percentile_value(wall_times, 95.0), 3),
        "mad_wall_time_sec": round(mad, 3),
        "jitter_estimate_percent": jitter_percent,
        "budget_verdict": latest_budget_verdict,
        "trend_direction": trend,
    }


def empty_runtime_history_index() -> dict[str, Any]:
    return {
        "history_version": "runtime_history_index_v1",
        "generated_at_utc": timestamp_utc_now(),
        "fingerprints": [],
        "transitions": [],
    }


def history_bucket_for_manifest(
    history_index: dict[str, Any],
    current_manifest: dict[str, Any],
) -> dict[str, Any]:
    host = dict(current_manifest.get("host_fingerprint", {}))
    toolchain = dict(current_manifest.get("toolchain_fingerprint", {}))
    fingerprint_hash = str(host.get("fingerprint_hash", ""))
    runtime_key = runtime_manifest_fingerprint_key(current_manifest)
    for bucket in history_index.get("fingerprints", []):
        if str(bucket.get("runtime_fingerprint_key", "")) == runtime_key:
            return bucket
    bucket = {
        "fingerprint_hash": fingerprint_hash,
        "runtime_fingerprint_key": runtime_key,
        "host_fingerprint": host,
        "toolchain_fingerprint": toolchain,
        "execution_classes": {},
    }
    history_index.setdefault("fingerprints", []).append(bucket)
    return bucket


def append_runtime_history(
    history_index: dict[str, Any],
    current_manifest: dict[str, Any],
    current_manifest_path: Path,
    refresh_manifest: dict[str, Any] | None,
    refresh_manifest_path: Path | None,
    evidence_source: str = "real",
    runner_id: str = "",
    host_label: str = "",
    import_timestamp: str | None = None,
) -> dict[str, Any]:
    bucket = history_bucket_for_manifest(history_index, current_manifest)
    refresh_entries = {
        str(entry.get("execution_class", "")): entry for entry in (refresh_manifest or {}).get("entries", []) if isinstance(entry, dict)
    }
    for current_entry in current_manifest.get("entries", []):
        if not isinstance(current_entry, dict):
            continue
        execution_class = str(current_entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        latest_cross_bucket_sample = None
        latest_cross_bucket_fingerprint = ""
        for existing_bucket in history_index.get("fingerprints", []):
            existing_execution_classes = dict(existing_bucket.get("execution_classes", {}))
            existing_payload = existing_execution_classes.get(execution_class)
            if not isinstance(existing_payload, dict):
                continue
            samples = list(existing_payload.get("samples", []))
            if not samples:
                continue
            candidate_sample = samples[-1]
            candidate_timestamp = str(candidate_sample.get("timestamp_utc", ""))
            latest_timestamp = "" if latest_cross_bucket_sample is None else str(latest_cross_bucket_sample.get("timestamp_utc", ""))
            if latest_cross_bucket_sample is None or candidate_timestamp >= latest_timestamp:
                latest_cross_bucket_sample = candidate_sample
                latest_cross_bucket_fingerprint = str(existing_bucket.get("fingerprint_hash", ""))
        history_entry = bucket.setdefault("execution_classes", {}).setdefault(execution_class, {"samples": [], "summary": {}})
        refresh_entry = refresh_entries.get(execution_class, {})
        previous_sample = history_entry.get("samples", [])[-1] if history_entry.get("samples") else None
        sample_timestamp = import_timestamp or timestamp_utc_now()
        sample = {
            "timestamp_utc": sample_timestamp,
            "phase": current_manifest.get("phase", ""),
            "baseline_tag": current_manifest.get("baseline_tag", ""),
            "current_manifest_path": str(current_manifest_path),
            "refresh_manifest_path": None if refresh_manifest_path is None else str(refresh_manifest_path),
            "wall_time_sec": round(float(current_entry.get("wall_time_sec", 0.0)), 3),
            "test_count": int(current_entry.get("test_count", 0)),
            "current_status": str(current_entry.get("current_status", STATUS_OK)),
            "freshness_status": str(refresh_entry.get("freshness_status", current_entry.get("freshness_status", FRESHNESS_FRESH))),
            "comparability": str(refresh_entry.get("comparability", COMPARABLE)),
            "baseline_wall_time_sec": refresh_entry.get("baseline_wall_time_sec"),
            "delta_percent": refresh_entry.get("delta_percent", current_entry.get("delta_percent")),
            "evidence_source": normalized_evidence_source(evidence_source),
            "runner_id": runner_id or str(current_manifest.get("host_fingerprint", {}).get("runner_tag", "")),
            "host_label": host_label
            or "|".join(
                [
                    str(current_manifest.get("host_fingerprint", {}).get("os", "")),
                    str(current_manifest.get("host_fingerprint", {}).get("arch", "")),
                ]
            ).strip("|"),
            "import_timestamp": sample_timestamp,
        }
        transition_source_sample = previous_sample if previous_sample is not None else latest_cross_bucket_sample
        if transition_source_sample is not None:
            previous_tag = str(transition_source_sample.get("baseline_tag", ""))
            current_tag = str(sample.get("baseline_tag", ""))
            previous_transition_fingerprint = (
                str(bucket.get("fingerprint_hash", ""))
                if previous_sample is not None
                else latest_cross_bucket_fingerprint
            )
            if previous_tag and current_tag and previous_tag != current_tag:
                history_index.setdefault("transitions", []).append(
                    {
                        "timestamp_utc": sample["timestamp_utc"],
                        "fingerprint_hash": str(bucket.get("fingerprint_hash", "")),
                        "previous_fingerprint_hash": previous_transition_fingerprint,
                        "execution_class": execution_class,
                        "previous_baseline_tag": previous_tag,
                        "new_baseline_tag": current_tag,
                        "current_manifest_path": str(current_manifest_path),
                        "refresh_manifest_path": None if refresh_manifest_path is None else str(refresh_manifest_path),
                    }
                )
        history_entry.setdefault("samples", []).append(sample)
        history_entry["summary"] = summarize_runtime_sample_series(history_entry["samples"])
    history_index["generated_at_utc"] = timestamp_utc_now()
    history_index["history_hash"] = sha256_text(json.dumps(history_index, sort_keys=True))
    return history_index


def runtime_history_summary(history_index: dict[str, Any], history_index_path: Path) -> dict[str, Any]:
    fingerprints: list[dict[str, Any]] = []
    trend_counts = {
        "stable": 0,
        "noisy": 0,
        "regressing": 0,
        "improved": 0,
        "insufficient_history": 0,
    }
    evidence_source_counts: dict[str, int] = {}
    latest_real_sample_timestamp = ""
    latest_fixture_sample_timestamp = ""
    for bucket in history_index.get("fingerprints", []):
        execution_classes: list[dict[str, Any]] = []
        for execution_class, payload in sorted(dict(bucket.get("execution_classes", {})).items()):
            summary = dict(payload.get("summary", summarize_runtime_sample_series(payload.get("samples", []))))
            trend = str(summary.get("trend_direction", "insufficient_history"))
            trend_counts[trend] = trend_counts.get(trend, 0) + 1
            sample_source_counts: dict[str, int] = {}
            for sample in payload.get("samples", []):
                if not isinstance(sample, dict):
                    continue
                source = normalized_evidence_source(sample.get("evidence_source"))
                sample_source_counts[source] = sample_source_counts.get(source, 0) + 1
                evidence_source_counts[source] = evidence_source_counts.get(source, 0) + 1
                timestamp = str(sample.get("timestamp_utc", "")).strip()
                if source == "real" and timestamp and timestamp >= latest_real_sample_timestamp:
                    latest_real_sample_timestamp = timestamp
                if source == "fixture" and timestamp and timestamp >= latest_fixture_sample_timestamp:
                    latest_fixture_sample_timestamp = timestamp
            execution_classes.append({"execution_class": execution_class, **summary})
            execution_classes[-1]["evidence_source_counts"] = sample_source_counts
        fingerprints.append(
            {
                "fingerprint_hash": str(bucket.get("fingerprint_hash", "")),
                "runtime_fingerprint_key": str(bucket.get("runtime_fingerprint_key", "")),
                "host_fingerprint": bucket.get("host_fingerprint", {}),
                "toolchain_fingerprint": bucket.get("toolchain_fingerprint", {}),
                "execution_classes": execution_classes,
                "transition_count": sum(
                    1
                    for transition in history_index.get("transitions", [])
                    if str(transition.get("fingerprint_hash", "")) == str(bucket.get("fingerprint_hash", ""))
                ),
            }
        )
    summary = {
        "summary_version": "runtime_history_summary_v1",
        "generated_at_utc": timestamp_utc_now(),
        "runtime_history_index_path": str(history_index_path),
        "fingerprint_count": len(fingerprints),
        "trend_counts": trend_counts,
        "evidence_source_counts": evidence_source_counts,
        "latest_real_sample_timestamp": latest_real_sample_timestamp or None,
        "latest_fixture_sample_timestamp": latest_fixture_sample_timestamp or None,
        "fingerprints": fingerprints,
        "transition_count": len(history_index.get("transitions", [])),
        "recent_transitions": list(history_index.get("transitions", []))[-10:],
    }
    summary["summary_hash"] = sha256_text(json.dumps(summary, sort_keys=True))
    return summary


def runtime_history_summary_text(summary: dict[str, Any]) -> str:
    lines = [
        f"summary_version={summary.get('summary_version', '')}",
        f"fingerprint_count={summary.get('fingerprint_count', 0)}",
    ]
    for fingerprint in summary.get("fingerprints", []):
        lines.append(
            "runtime_history_fingerprint="
            + f"fingerprint_hash={fingerprint.get('fingerprint_hash', '')}"
            + f" execution_class_count={len(fingerprint.get('execution_classes', []))}"
        )
        for entry in fingerprint.get("execution_classes", []):
            lines.append(
                "runtime_history_entry="
                + f"execution_class={entry.get('execution_class', '')}"
                + f" sample_count={entry.get('sample_count', 0)}"
                + f" trend_direction={entry.get('trend_direction', '')}"
                + f" latest_wall_time_sec={entry.get('latest_wall_time_sec', 0)}"
            )
    return "\n".join(lines) + "\n"


def runtime_history_summary_short(summary: dict[str, Any]) -> str:
    total_entries = sum(len(bucket.get("execution_classes", [])) for bucket in summary.get("fingerprints", []))
    return (
        "runtime_history_summary"
        + f" fingerprint_count={summary.get('fingerprint_count', 0)}"
        + f" execution_class_count={total_entries}\n"
    )


def write_runtime_history_outputs(history_index_path: Path, history_index: dict[str, Any]) -> Path:
    write_json(history_index_path, history_index)
    history_summary_path = default_runtime_history_summary_path(history_index_path)
    summary = runtime_history_summary(history_index, history_index_path)
    write_json(history_summary_path, summary)
    write_text(history_index_path.with_suffix(".txt"), runtime_history_summary_text(summary))
    write_text(history_index_path.with_name(f"{history_index_path.stem}.summary.txt"), runtime_history_summary_short(summary))
    write_text(history_summary_path.with_suffix(".txt"), runtime_history_summary_text(summary))
    write_text(history_summary_path.with_name(f"{history_summary_path.stem}.summary.txt"), runtime_history_summary_short(summary))
    return history_summary_path


def compact_runtime_history_index(
    history_index: dict[str, Any],
    keep_latest_per_fingerprint: int,
    keep_anchors: int,
    keep_transitions: bool,
    prune_old_fixture_history: bool,
) -> dict[str, Any]:
    compacted = json.loads(json.dumps(history_index))
    transition_keys: set[tuple[str, str, str]] = set()
    if keep_transitions:
        for transition in history_index.get("transitions", []):
            if not isinstance(transition, dict):
                continue
            transition_keys.add(
                (
                    str(transition.get("fingerprint_hash", "")),
                    str(transition.get("execution_class", "")),
                    str(transition.get("timestamp_utc", "")),
                )
            )

    for bucket in compacted.get("fingerprints", []):
        fingerprint_hash = str(bucket.get("fingerprint_hash", ""))
        execution_classes = dict(bucket.get("execution_classes", {}))
        for execution_class, payload in execution_classes.items():
            samples = [dict(sample) for sample in payload.get("samples", []) if isinstance(sample, dict)]
            if not samples:
                payload["compaction"] = {
                    "original_sample_count": 0,
                    "retained_sample_count": 0,
                }
                continue
            selected_indexes: set[int] = set()
            anchors = max(0, keep_anchors)
            latest_keep = max(1, keep_latest_per_fingerprint)

            for index in range(min(anchors, len(samples))):
                selected_indexes.add(index)
            for index in range(max(0, len(samples) - anchors), len(samples)):
                selected_indexes.add(index)

            real_indexes = [
                index
                for index, sample in enumerate(samples)
                if normalized_evidence_source(sample.get("evidence_source")) != "fixture"
            ]
            fixture_indexes = [
                index
                for index, sample in enumerate(samples)
                if normalized_evidence_source(sample.get("evidence_source")) == "fixture"
            ]
            for index in real_indexes[-latest_keep:]:
                selected_indexes.add(index)
            fixture_keep = latest_keep if not prune_old_fixture_history else min(latest_keep, 2)
            for index in fixture_indexes[-fixture_keep:]:
                selected_indexes.add(index)

            if keep_transitions:
                for index, sample in enumerate(samples):
                    key = (
                        fingerprint_hash,
                        str(execution_class),
                        str(sample.get("timestamp_utc", "")),
                    )
                    if key in transition_keys:
                        selected_indexes.add(index)

            retained = [samples[index] for index in sorted(selected_indexes)]
            payload["samples"] = retained
            payload["summary"] = summarize_runtime_sample_series(retained)
            payload["compaction"] = {
                "original_sample_count": len(samples),
                "retained_sample_count": len(retained),
                "keep_latest_per_fingerprint": latest_keep,
                "keep_anchors": anchors,
                "keep_transitions": bool(keep_transitions),
                "prune_old_fixture_history": bool(prune_old_fixture_history),
            }
    compacted["history_version"] = "runtime_history_index_v1_compacted"
    compacted["generated_at_utc"] = timestamp_utc_now()
    compacted["history_hash"] = sha256_text(json.dumps(compacted, sort_keys=True))
    compacted["compaction_policy"] = {
        "keep_latest_per_fingerprint": max(1, keep_latest_per_fingerprint),
        "keep_anchors": max(0, keep_anchors),
        "keep_transitions": bool(keep_transitions),
        "prune_old_fixture_history": bool(prune_old_fixture_history),
    }
    return compacted


def default_runtime_watch_history_summary_path(history_index_path: Path) -> Path:
    return history_index_path.with_name(f"{history_index_path.stem}_summary.json")


def runtime_manifest_fingerprint_key(manifest: dict[str, Any]) -> str:
    return runtime_fingerprint_key(
        dict(manifest.get("host_fingerprint", {})),
        dict(manifest.get("toolchain_fingerprint", {})),
        runtime_execution_signatures(manifest),
    )


def runtime_watch_sample_from_manifests(
    current_manifest: dict[str, Any],
    current_manifest_path: Path,
    refresh_manifest: dict[str, Any] | None,
    refresh_manifest_path: Path | None,
    execution_class: str,
) -> dict[str, Any] | None:
    current_entry = None
    for entry in current_manifest.get("entries", []):
        if isinstance(entry, dict) and str(entry.get("execution_class", "")) == execution_class:
            current_entry = entry
            break
    if current_entry is None:
        return None
    refresh_entry = {}
    if refresh_manifest is not None:
        for candidate in refresh_manifest.get("entries", []):
            if isinstance(candidate, dict) and str(candidate.get("execution_class", "")) == execution_class:
                refresh_entry = candidate
                break
    return {
        "timestamp_utc": timestamp_utc_now(),
        "phase": current_manifest.get("phase", ""),
        "baseline_tag": current_manifest.get("baseline_tag", ""),
        "current_manifest_path": str(current_manifest_path),
        "refresh_manifest_path": None if refresh_manifest_path is None else str(refresh_manifest_path),
        "execution_class": execution_class,
        "wall_time_sec": round(float(current_entry.get("wall_time_sec", 0.0)), 3),
        "test_count": int(current_entry.get("test_count", 0)),
        "current_status": str(current_entry.get("current_status", STATUS_OK)),
        "freshness_status": str(refresh_entry.get("freshness_status", current_entry.get("freshness_status", FRESHNESS_FRESH))),
        "comparability": str(refresh_entry.get("comparability", COMPARABLE)),
        "baseline_wall_time_sec": refresh_entry.get("baseline_wall_time_sec"),
        "delta_percent": refresh_entry.get("delta_percent", current_entry.get("delta_percent")),
        "execution_role": str(current_entry.get("execution_role", default_runtime_role(execution_class))),
    }


def same_runtime_sample(sample: dict[str, Any], current_sample: dict[str, Any]) -> bool:
    return (
        str(sample.get("current_manifest_path", "")) == str(current_sample.get("current_manifest_path", ""))
        and str(sample.get("execution_class", "")) == str(current_sample.get("execution_class", ""))
        and float(sample.get("wall_time_sec", 0.0)) == float(current_sample.get("wall_time_sec", 0.0))
        and str(sample.get("baseline_tag", "")) == str(current_sample.get("baseline_tag", ""))
    )


def runtime_watch_status_rank(status: str) -> int:
    order = {
        WATCH_CLEAR: 0,
        WATCH_WATCH: 1,
        WATCH_STABLE: 2,
        WATCH_REBASELINE_CANDIDATE: 3,
        WATCH_ESCALATE: 4,
        WATCH_REBASELINE_REQUIRED: 5,
        WATCH_FAIL: 6,
    }
    return order.get(status, 0)


def runtime_watch_recommendation(status: str, role: str) -> str:
    if status == WATCH_CLEAR:
        return "NO_ACTION"
    if status == WATCH_WATCH:
        return "WATCH_RUNTIME"
    if status == WATCH_STABLE:
        return "CONTINUE_MONITORING" if role == ROLE_DIAGNOSTIC else "WATCH_RUNTIME"
    if status == WATCH_REBASELINE_CANDIDATE:
        return "PROPOSE_RUNTIME_REBASELINE"
    if status == WATCH_ESCALATE:
        return "INVESTIGATE_RUNTIME_DRIFT"
    if status == WATCH_REBASELINE_REQUIRED:
        return "REBASELINE_REQUIRED"
    return "FAIL"


def runtime_watch_reason(
    status: str,
    execution_class: str,
    role: str,
    sample_count: int,
    soft_count: int,
    hard_count: int,
    trend: str,
) -> str:
    if status == WATCH_CLEAR:
        return f"{execution_class} is within budget and recent same-fingerprint samples are clear"
    if status == WATCH_WATCH:
        return f"{execution_class} has a soft-budget warning with limited history ({sample_count} samples)"
    if status == WATCH_STABLE:
        return f"{execution_class} is a stable {role} soft-budget overrun with bounded jitter across {soft_count}/{sample_count} samples"
    if status == WATCH_REBASELINE_CANDIDATE:
        return f"{execution_class} stayed over the soft budget long enough to consider a runtime rebaseline proposal"
    if status == WATCH_ESCALATE:
        return f"{execution_class} exceeded the hard budget or severe regression threshold ({hard_count} hard samples, trend={trend})"
    if status == WATCH_REBASELINE_REQUIRED:
        return f"{execution_class} is not strictly comparable to the selected runtime baseline"
    return f"{execution_class} exceeded the hard budget in a {role} execution class"


def trailing_count(values: list[bool]) -> int:
    count = 0
    for value in reversed(values):
        if not value:
            break
        count += 1
    return count


def classify_runtime_watch_entry(
    execution_class: str,
    current_entry: dict[str, Any],
    refresh_entry: dict[str, Any],
    baseline_entry: dict[str, Any] | None,
    series: list[dict[str, Any]],
    budget_profile_entry: dict[str, Any],
) -> dict[str, Any]:
    thresholds = dict(budget_profile_entry.get("thresholds", default_runtime_threshold(execution_class)))
    role = str(budget_profile_entry.get("role", current_entry.get("execution_role", default_runtime_role(execution_class))))
    watch_policy = dict(budget_profile_entry.get("watch_policy", default_runtime_watch_policy(execution_class, role)))
    soft_limit = float(thresholds.get("soft_seconds", 0.0))
    hard_limit = float(thresholds.get("hard_seconds", 0.0))

    normalized_series: list[dict[str, Any]] = []
    soft_flags: list[bool] = []
    hard_flags: list[bool] = []
    for sample in series:
        wall_time = round(float(sample.get("wall_time_sec", 0.0)), 3)
        current_status = str(sample.get("current_status", STATUS_OK))
        soft = (soft_limit > 0.0 and wall_time > soft_limit) or (
            soft_limit <= 0.0 and current_status in {STATUS_WARN, STATUS_FAIL}
        )
        hard = (hard_limit > 0.0 and wall_time > hard_limit) or (
            hard_limit <= 0.0 and current_status == STATUS_FAIL
        )
        sample_copy = dict(sample)
        sample_copy["wall_time_sec"] = wall_time
        sample_copy["soft_over_budget"] = soft
        sample_copy["hard_over_budget"] = hard
        normalized_series.append(sample_copy)
        soft_flags.append(soft)
        hard_flags.append(hard)

    summary = summarize_runtime_sample_series(normalized_series)
    sample_count = len(normalized_series)
    soft_over_budget_count = sum(1 for value in soft_flags if value)
    hard_over_budget_count = sum(1 for value in hard_flags if value)
    stable_overrun_count = trailing_count([soft and not hard for soft, hard in zip(soft_flags, hard_flags)])
    clear_count = trailing_count([not soft and not hard for soft, hard in zip(soft_flags, hard_flags)])
    escalation_count = trailing_count(hard_flags)
    over_budget_ratio = 0.0 if sample_count == 0 else round(soft_over_budget_count / sample_count, 3)
    trend = str(summary.get("trend_direction", TREND_INSUFFICIENT))
    comparability = str(refresh_entry.get("comparability", COMPARABLE))

    baseline_wall = None
    if baseline_entry is not None:
        baseline_wall = float(baseline_entry.get("wall_time_sec", 0.0))
    elif refresh_entry.get("baseline_wall_time_sec") is not None:
        baseline_wall = float(refresh_entry.get("baseline_wall_time_sec", 0.0))

    baseline_soft = bool(baseline_wall is not None and soft_limit > 0.0 and baseline_wall > soft_limit)
    near_baseline = False
    if baseline_wall is not None and baseline_wall > 0.0:
        near_baseline = abs(float(summary.get("rolling_median_wall_time_sec", 0.0)) - baseline_wall) <= max(
            1.0,
            baseline_wall * (float(watch_policy.get("near_baseline_delta_percent", 5.0)) / 100.0),
        )
    bounded_jitter = float(summary.get("jitter_estimate_percent", 0.0)) <= float(
        watch_policy.get("max_bounded_jitter_percent", 15.0)
    )
    min_samples = int(watch_policy.get("min_samples", 3))
    clear_window = int(watch_policy.get("clear_window", 2))
    stable_threshold = int(watch_policy.get("stable_overrun_threshold", 3))
    candidate_threshold = int(watch_policy.get("rebaseline_candidate_threshold", 5))

    if comparability in {FRESHNESS_NOT_COMPARABLE, NOT_COMPARABLE, FRESHNESS_REBASELINE_REQUIRED, REBASELINE_REQUIRED}:
        watch_status = WATCH_REBASELINE_REQUIRED
    elif hard_over_budget_count > 0 or str(current_entry.get("current_status", STATUS_OK)) == STATUS_FAIL:
        watch_status = WATCH_FAIL if role == ROLE_PRODUCTION_CRITICAL else WATCH_ESCALATE
    elif sample_count >= min_samples and clear_count >= clear_window:
        watch_status = WATCH_CLEAR
    elif (
        sample_count >= min_samples
        and stable_overrun_count >= candidate_threshold
        and soft_over_budget_count > 0
        and (not baseline_soft or not near_baseline)
    ):
        watch_status = WATCH_REBASELINE_CANDIDATE
    elif sample_count >= min_samples and stable_overrun_count >= stable_threshold and soft_over_budget_count > 0 and bounded_jitter:
        watch_status = WATCH_STABLE
    elif soft_over_budget_count > 0:
        watch_status = WATCH_WATCH
    else:
        watch_status = WATCH_CLEAR

    watch_reason = runtime_watch_reason(
        watch_status,
        execution_class,
        role,
        sample_count,
        soft_over_budget_count,
        hard_over_budget_count,
        trend,
    )
    recommendation = runtime_watch_recommendation(watch_status, role)
    return {
        "execution_class": execution_class,
        "execution_role": role,
        "budget_thresholds": thresholds,
        "watch_policy": watch_policy,
        "watch_status": watch_status,
        "watch_reason": watch_reason,
        "watch_recommendation": recommendation,
        "sample_count": sample_count,
        "stable_overrun_count": stable_overrun_count,
        "clear_count": clear_count,
        "escalation_count": escalation_count,
        "soft_over_budget_count": soft_over_budget_count,
        "hard_over_budget_count": hard_over_budget_count,
        "over_budget_ratio": over_budget_ratio,
        "rebaseline_candidate": watch_status == WATCH_REBASELINE_CANDIDATE,
        "trend_direction": trend,
        "rolling_median_wall_time_sec": summary.get("rolling_median_wall_time_sec"),
        "rolling_p90_wall_time_sec": summary.get("rolling_p90_wall_time_sec"),
        "rolling_p95_wall_time_sec": summary.get("rolling_p95_wall_time_sec"),
        "mad_wall_time_sec": summary.get("mad_wall_time_sec"),
        "jitter_estimate_percent": summary.get("jitter_estimate_percent"),
        "latest_wall_time_sec": summary.get("latest_wall_time_sec"),
        "baseline_wall_time_sec": baseline_wall,
        "delta_vs_selected_baseline_percent": summary.get("delta_vs_selected_baseline_percent"),
        "runtime_current_status": current_entry.get("current_status"),
        "runtime_freshness_status": refresh_entry.get("freshness_status"),
        "runtime_comparability": refresh_entry.get("comparability"),
        "samples": normalized_series,
    }


def build_runtime_watch_current(
    baseline_manifest: dict[str, Any] | None,
    baseline_manifest_path: Path | None,
    current_manifest: dict[str, Any],
    current_manifest_path: Path,
    refresh_manifest: dict[str, Any] | None,
    refresh_manifest_path: Path | None,
    history_index: dict[str, Any] | None,
    watch_history_index_path: Path | None,
    execution_classes: list[str],
    repeat: int,
    budget_profile: dict[str, Any],
    watch_manifest_path: Path,
) -> dict[str, Any]:
    refresh_entries = {
        str(entry.get("execution_class", "")): entry
        for entry in (refresh_manifest or {}).get("entries", [])
        if isinstance(entry, dict)
    }
    baseline_entries = {
        str(entry.get("execution_class", "")): entry
        for entry in (baseline_manifest or {}).get("entries", [])
        if isinstance(entry, dict)
    }
    bucket = {}
    if history_index is not None:
        current_key = runtime_manifest_fingerprint_key(current_manifest)
        for candidate in history_index.get("fingerprints", []):
            if str(candidate.get("runtime_fingerprint_key", "")) == current_key:
                bucket = candidate
                break

    entries: list[dict[str, Any]] = []
    for execution_class in execution_classes:
        current_entry = None
        for entry in current_manifest.get("entries", []):
            if isinstance(entry, dict) and str(entry.get("execution_class", "")) == execution_class:
                current_entry = entry
                break
        if current_entry is None:
            continue
        history_payload = dict(bucket.get("execution_classes", {})).get(execution_class, {})
        history_samples = [dict(sample) for sample in history_payload.get("samples", []) if isinstance(sample, dict)]
        current_sample = runtime_watch_sample_from_manifests(
            current_manifest,
            current_manifest_path,
            refresh_manifest,
            refresh_manifest_path,
            execution_class,
        )
        if current_sample is not None and (not history_samples or not same_runtime_sample(history_samples[-1], current_sample)):
            history_samples.append(current_sample)
        series = history_samples[-repeat:] if repeat > 0 else history_samples
        entries.append(
            classify_runtime_watch_entry(
                execution_class,
                current_entry,
                refresh_entries.get(execution_class, {}),
                baseline_entries.get(execution_class),
                series,
                runtime_budget_profile_entry(budget_profile, execution_class),
            )
        )

    strongest_entry = max(entries, key=lambda entry: runtime_watch_status_rank(str(entry.get("watch_status", WATCH_CLEAR)))) if entries else {}
    watch_current = {
        "manifest_version": "runtime_watch_current_v1",
        "generated_at_utc": timestamp_utc_now(),
        "phase": current_manifest.get("phase", ""),
        "artifact_root": current_manifest.get("artifact_root", ""),
        "runtime_baseline_manifest_path": None if baseline_manifest_path is None else str(baseline_manifest_path),
        "runtime_current_manifest_path": str(current_manifest_path),
        "runtime_refresh_manifest_path": None if refresh_manifest_path is None else str(refresh_manifest_path),
        "runtime_watch_history_index_path": None if watch_history_index_path is None else str(watch_history_index_path),
        "runtime_budget_profile_path": str(default_runtime_budget_profile_output_path(watch_manifest_path)),
        "runtime_budget_profile_id": budget_profile.get("profile_id"),
        "runtime_budget_profile_version": budget_profile.get("version"),
        "budget_profile": budget_profile,
        "runtime_baseline_manifest_hash": sha256_file(baseline_manifest_path),
        "runtime_current_manifest_hash": sha256_file(current_manifest_path),
        "runtime_refresh_manifest_hash": sha256_file(refresh_manifest_path),
        "runtime_fingerprint_key": runtime_manifest_fingerprint_key(current_manifest),
        "host_fingerprint": current_manifest.get("host_fingerprint", {}),
        "toolchain_fingerprint": current_manifest.get("toolchain_fingerprint", {}),
        "execution_class_filter": execution_classes,
        "repeat": repeat,
        "entries": entries,
        "overall_watch_status": strongest_entry.get("watch_status", WATCH_CLEAR),
        "overall_watch_recommendation": strongest_entry.get("watch_recommendation", "NO_ACTION"),
        "overall_watch_reason": strongest_entry.get("watch_reason", "runtime watch is clear"),
        "watch_sample_count": max((int(entry.get("sample_count", 0)) for entry in entries), default=0),
    }
    watch_current["watch_manifest_hash"] = manifest_hash_without_field(watch_current, "watch_manifest_hash")
    return watch_current


def runtime_watch_current_text(manifest: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={manifest.get('manifest_version', '')}",
        f"phase={manifest.get('phase', '')}",
        f"overall_watch_status={manifest.get('overall_watch_status', '')}",
        f"overall_watch_recommendation={manifest.get('overall_watch_recommendation', '')}",
        f"runtime_budget_profile_id={manifest.get('runtime_budget_profile_id', '')}",
        f"watch_sample_count={manifest.get('watch_sample_count', 0)}",
    ]
    for entry in manifest.get("entries", []):
        lines.append(
            "runtime_watch_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" role={entry.get('execution_role', '')}"
            + f" watch_status={entry.get('watch_status', '')}"
            + f" sample_count={entry.get('sample_count', 0)}"
        )
    return "\n".join(lines) + "\n"


def runtime_watch_current_summary(manifest: dict[str, Any]) -> str:
    return (
        "runtime_watch_current_summary"
        + f" overall_watch_status={manifest.get('overall_watch_status', '')}"
        + f" overall_watch_recommendation={manifest.get('overall_watch_recommendation', '')}"
        + f" entry_count={len(manifest.get('entries', []))}"
        + f" watch_sample_count={manifest.get('watch_sample_count', 0)}\n"
    )


def write_runtime_watch_current_outputs(json_path: Path, manifest: dict[str, Any]) -> None:
    write_json(json_path, manifest)
    write_text(json_path.with_suffix(".txt"), runtime_watch_current_text(manifest))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_watch_current_summary(manifest))
    budget_profile_path = Path(str(manifest.get("runtime_budget_profile_path", default_runtime_budget_profile_output_path(json_path)))).resolve()
    write_runtime_budget_profile_outputs(budget_profile_path, dict(manifest.get("budget_profile", {})))


def empty_runtime_watch_history_index() -> dict[str, Any]:
    return {
        "history_version": "runtime_watch_history_index_v1",
        "generated_at_utc": timestamp_utc_now(),
        "fingerprints": [],
    }


def watch_history_bucket_for_manifest(
    watch_history_index: dict[str, Any],
    current_manifest: dict[str, Any],
) -> dict[str, Any]:
    runtime_key = runtime_manifest_fingerprint_key(current_manifest)
    for bucket in watch_history_index.get("fingerprints", []):
        if str(bucket.get("runtime_fingerprint_key", "")) == runtime_key:
            return bucket
    bucket = {
        "runtime_fingerprint_key": runtime_key,
        "host_fingerprint": current_manifest.get("host_fingerprint", {}),
        "toolchain_fingerprint": current_manifest.get("toolchain_fingerprint", {}),
        "execution_classes": {},
    }
    watch_history_index.setdefault("fingerprints", []).append(bucket)
    return bucket


def append_runtime_watch_history(
    watch_history_index: dict[str, Any],
    current_manifest: dict[str, Any],
    watch_refresh_manifest: dict[str, Any],
    watch_current_path: Path | None,
    watch_refresh_path: Path,
) -> dict[str, Any]:
    bucket = watch_history_bucket_for_manifest(watch_history_index, current_manifest)
    for entry in watch_refresh_manifest.get("entries", []):
        if not isinstance(entry, dict):
            continue
        execution_class = str(entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        payload = bucket.setdefault("execution_classes", {}).setdefault(execution_class, {"samples": [], "summary": {}})
        payload.setdefault("samples", []).append(
            {
                "timestamp_utc": timestamp_utc_now(),
                "phase": current_manifest.get("phase", ""),
                "baseline_tag": current_manifest.get("baseline_tag", ""),
                "watch_current_path": None if watch_current_path is None else str(watch_current_path),
                "watch_refresh_path": str(watch_refresh_path),
                "watch_status": entry.get("watch_status"),
                "watch_reason": entry.get("watch_reason"),
                "watch_recommendation": entry.get("watch_recommendation"),
                "sample_count": entry.get("sample_count"),
                "stable_overrun_count": entry.get("stable_overrun_count"),
                "clear_count": entry.get("clear_count"),
                "escalation_count": entry.get("escalation_count"),
                "execution_role": entry.get("execution_role"),
            }
        )
        payload["summary"] = dict(entry)
    watch_history_index["generated_at_utc"] = timestamp_utc_now()
    watch_history_index["history_hash"] = sha256_text(json.dumps(watch_history_index, sort_keys=True))
    return watch_history_index


def runtime_watch_history_summary(
    watch_history_index: dict[str, Any],
    watch_history_index_path: Path,
) -> dict[str, Any]:
    buckets: list[dict[str, Any]] = []
    status_counts: dict[str, int] = {}
    transition_counts: dict[str, int] = {}
    recent_transitions: list[dict[str, Any]] = []
    for bucket in watch_history_index.get("fingerprints", []):
        execution_classes: list[dict[str, Any]] = []
        for execution_class, payload in sorted(dict(bucket.get("execution_classes", {})).items()):
            summary = dict(payload.get("summary", {}))
            status = str(summary.get("watch_status", WATCH_CLEAR))
            status_counts[status] = status_counts.get(status, 0) + 1
            previous_status = None
            for sample in payload.get("samples", []):
                if not isinstance(sample, dict):
                    continue
                sample_status = str(sample.get("watch_status", WATCH_CLEAR))
                if previous_status is not None and sample_status != previous_status:
                    transition_key = f"{previous_status}->{sample_status}"
                    transition_counts[transition_key] = transition_counts.get(transition_key, 0) + 1
                    recent_transitions.append(
                        {
                            "runtime_fingerprint_key": bucket.get("runtime_fingerprint_key"),
                            "execution_class": execution_class,
                            "from_status": previous_status,
                            "to_status": sample_status,
                            "timestamp_utc": sample.get("timestamp_utc"),
                            "watch_refresh_path": sample.get("watch_refresh_path"),
                        }
                    )
                previous_status = sample_status
            execution_classes.append(
                {
                    "execution_class": execution_class,
                    "watch_status": status,
                    "watch_recommendation": summary.get("watch_recommendation"),
                    "sample_count": summary.get("sample_count"),
                    "stable_overrun_count": summary.get("stable_overrun_count"),
                    "clear_count": summary.get("clear_count"),
                    "escalation_count": summary.get("escalation_count"),
                    "execution_role": summary.get("execution_role"),
                }
            )
        buckets.append(
            {
                "runtime_fingerprint_key": bucket.get("runtime_fingerprint_key"),
                "host_fingerprint": bucket.get("host_fingerprint", {}),
                "toolchain_fingerprint": bucket.get("toolchain_fingerprint", {}),
                "execution_classes": execution_classes,
            }
        )
    summary = {
        "summary_version": "runtime_watch_history_summary_v1",
        "generated_at_utc": timestamp_utc_now(),
        "runtime_watch_history_index_path": str(watch_history_index_path),
        "fingerprint_count": len(buckets),
        "watch_status_counts": status_counts,
        "transition_count": len(recent_transitions),
        "watch_transition_counts": transition_counts,
        "recent_transitions": recent_transitions[-10:],
        "strongest_watch_status": (
            max(status_counts, key=lambda value: runtime_watch_status_rank(value))
            if status_counts
            else WATCH_CLEAR
        ),
        "fingerprints": buckets,
    }
    summary["summary_hash"] = sha256_text(json.dumps(summary, sort_keys=True))
    return summary


def runtime_watch_history_text(summary: dict[str, Any]) -> str:
    lines = [
        f"summary_version={summary.get('summary_version', '')}",
        f"fingerprint_count={summary.get('fingerprint_count', 0)}",
        f"transition_count={summary.get('transition_count', 0)}",
    ]
    for bucket in summary.get("fingerprints", []):
        lines.append(
            "runtime_watch_history_bucket="
            + f"runtime_fingerprint_key={bucket.get('runtime_fingerprint_key', '')}"
            + f" execution_class_count={len(bucket.get('execution_classes', []))}"
        )
        for entry in bucket.get("execution_classes", []):
            lines.append(
                "runtime_watch_history_entry="
                + f"execution_class={entry.get('execution_class', '')}"
                + f" watch_status={entry.get('watch_status', '')}"
                + f" sample_count={entry.get('sample_count', 0)}"
            )
    return "\n".join(lines) + "\n"


def runtime_watch_history_short(summary: dict[str, Any]) -> str:
    return (
        "runtime_watch_history_summary"
        + f" fingerprint_count={summary.get('fingerprint_count', 0)}"
        + f" transition_count={summary.get('transition_count', 0)}"
        + f" watch_status_counts={json.dumps(summary.get('watch_status_counts', {}), sort_keys=True)}\n"
    )


def write_runtime_watch_history_outputs(history_index_path: Path, watch_history_index: dict[str, Any]) -> Path:
    write_json(history_index_path, watch_history_index)
    summary_path = default_runtime_watch_history_summary_path(history_index_path)
    summary = runtime_watch_history_summary(watch_history_index, history_index_path)
    write_json(summary_path, summary)
    write_text(history_index_path.with_suffix(".txt"), runtime_watch_history_text(summary))
    write_text(history_index_path.with_name(f"{history_index_path.stem}.summary.txt"), runtime_watch_history_short(summary))
    write_text(summary_path.with_suffix(".txt"), runtime_watch_history_text(summary))
    write_text(summary_path.with_name(f"{summary_path.stem}.summary.txt"), runtime_watch_history_short(summary))
    return summary_path


def build_runtime_watch_refresh(
    baseline_manifest: dict[str, Any] | None,
    baseline_manifest_path: Path | None,
    current_manifest: dict[str, Any],
    current_manifest_path: Path,
    refresh_manifest: dict[str, Any],
    refresh_manifest_path: Path,
    watch_current_manifest: dict[str, Any],
    watch_current_path: Path,
) -> dict[str, Any]:
    baseline_entries = {
        str(entry.get("execution_class", "")): entry
        for entry in (baseline_manifest or {}).get("entries", [])
        if isinstance(entry, dict)
    }
    refresh_entries = {
        str(entry.get("execution_class", "")): entry
        for entry in refresh_manifest.get("entries", [])
        if isinstance(entry, dict)
    }
    watch_entries = {
        str(entry.get("execution_class", "")): entry
        for entry in watch_current_manifest.get("entries", [])
        if isinstance(entry, dict)
    }

    budget_profile = dict(watch_current_manifest.get("budget_profile", {}))
    entries: list[dict[str, Any]] = []
    for current_entry in current_manifest.get("entries", []):
        if not isinstance(current_entry, dict):
            continue
        execution_class = str(current_entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        watch_entry = dict(watch_entries.get(execution_class, {}))
        if not watch_entry:
            budget_entry = runtime_budget_profile_entry(budget_profile, execution_class)
            watch_entry = {
                "execution_class": execution_class,
                "execution_role": budget_entry.get("role", default_runtime_role(execution_class)),
                "watch_status": WATCH_CLEAR,
                "watch_reason": f"{execution_class} has no active runtime watch signal",
                "watch_recommendation": "NO_ACTION",
                "sample_count": 0,
                "stable_overrun_count": 0,
                "clear_count": 0,
                "escalation_count": 0,
                "soft_over_budget_count": 0,
                "hard_over_budget_count": 0,
                "over_budget_ratio": 0.0,
                "rebaseline_candidate": False,
                "trend_direction": TREND_INSUFFICIENT,
                "rolling_median_wall_time_sec": float(current_entry.get("wall_time_sec", 0.0)),
                "rolling_p90_wall_time_sec": float(current_entry.get("wall_time_sec", 0.0)),
                "rolling_p95_wall_time_sec": float(current_entry.get("wall_time_sec", 0.0)),
                "mad_wall_time_sec": 0.0,
                "jitter_estimate_percent": 0.0,
                "latest_wall_time_sec": float(current_entry.get("wall_time_sec", 0.0)),
                "baseline_wall_time_sec": None if execution_class not in baseline_entries else baseline_entries[execution_class].get("wall_time_sec"),
                "delta_vs_selected_baseline_percent": refresh_entries.get(execution_class, {}).get("delta_percent"),
                "runtime_current_status": current_entry.get("current_status"),
                "runtime_freshness_status": refresh_entries.get(execution_class, {}).get("freshness_status"),
                "runtime_comparability": refresh_entries.get(execution_class, {}).get("comparability"),
                "budget_thresholds": budget_entry.get("thresholds", {}),
                "watch_policy": budget_entry.get("watch_policy", {}),
            }
        entries.append(watch_entry)

    strongest_entry = max(entries, key=lambda entry: runtime_watch_status_rank(str(entry.get("watch_status", WATCH_CLEAR)))) if entries else {}
    non_clear_entries = [entry for entry in entries if str(entry.get("watch_status", WATCH_CLEAR)) != WATCH_CLEAR]
    diagnostic_watch_only = bool(non_clear_entries) and all(
        str(entry.get("execution_role", "")) == ROLE_DIAGNOSTIC for entry in non_clear_entries
    )
    watch_status_counts: dict[str, int] = {}
    for entry in entries:
        status = str(entry.get("watch_status", WATCH_CLEAR))
        watch_status_counts[status] = watch_status_counts.get(status, 0) + 1

    refresh = {
        "manifest_version": "runtime_watch_refresh_v1",
        "generated_at_utc": timestamp_utc_now(),
        "phase": current_manifest.get("phase", ""),
        "artifact_root": current_manifest.get("artifact_root", ""),
        "runtime_fingerprint_key": runtime_manifest_fingerprint_key(current_manifest),
        "host_fingerprint": dict(current_manifest.get("host_fingerprint", {})),
        "toolchain_fingerprint": dict(current_manifest.get("toolchain_fingerprint", {})),
        "runtime_baseline_manifest_path": None if baseline_manifest_path is None else str(baseline_manifest_path),
        "runtime_current_manifest_path": str(current_manifest_path),
        "runtime_refresh_manifest_path": str(refresh_manifest_path),
        "runtime_watch_current_path": str(watch_current_path),
        "runtime_baseline_manifest_hash": sha256_file(baseline_manifest_path),
        "runtime_current_manifest_hash": sha256_file(current_manifest_path),
        "runtime_refresh_manifest_hash": sha256_file(refresh_manifest_path),
        "runtime_watch_current_hash": sha256_file(watch_current_path),
        "runtime_budget_profile_id": watch_current_manifest.get("runtime_budget_profile_id"),
        "runtime_budget_profile_version": watch_current_manifest.get("runtime_budget_profile_version"),
        "selected_baseline_id": refresh_manifest.get("selected_baseline_id"),
        "selected_baseline_tag": refresh_manifest.get("selected_baseline_tag"),
        "overall_watch_status": strongest_entry.get("watch_status", WATCH_CLEAR),
        "overall_watch_reason": strongest_entry.get("watch_reason", "runtime watch is clear"),
        "overall_watch_recommendation": strongest_entry.get("watch_recommendation", "NO_ACTION"),
        "diagnostic_watch_only": diagnostic_watch_only,
        "watch_status_counts": watch_status_counts,
        "runtime_watch_sample_count": max((int(entry.get("sample_count", 0)) for entry in entries), default=0),
        "entries": entries,
    }
    refresh["watch_refresh_hash"] = manifest_hash_without_field(refresh, "watch_refresh_hash")
    return refresh


def runtime_watch_refresh_text(manifest: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={manifest.get('manifest_version', '')}",
        f"phase={manifest.get('phase', '')}",
        f"overall_watch_status={manifest.get('overall_watch_status', '')}",
        f"overall_watch_recommendation={manifest.get('overall_watch_recommendation', '')}",
        f"diagnostic_watch_only={int(bool(manifest.get('diagnostic_watch_only', False)))}",
        f"runtime_watch_sample_count={manifest.get('runtime_watch_sample_count', 0)}",
    ]
    for entry in manifest.get("entries", []):
        lines.append(
            "runtime_watch_refresh_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" role={entry.get('execution_role', '')}"
            + f" watch_status={entry.get('watch_status', '')}"
            + f" recommendation={entry.get('watch_recommendation', '')}"
        )
    return "\n".join(lines) + "\n"


def runtime_watch_refresh_summary(manifest: dict[str, Any]) -> str:
    return (
        "runtime_watch_refresh_summary"
        + f" overall_watch_status={manifest.get('overall_watch_status', '')}"
        + f" overall_watch_recommendation={manifest.get('overall_watch_recommendation', '')}"
        + f" diagnostic_watch_only={int(bool(manifest.get('diagnostic_watch_only', False)))}"
        + f" entry_count={len(manifest.get('entries', []))}\n"
    )


def write_runtime_watch_refresh_outputs(json_path: Path, manifest: dict[str, Any]) -> None:
    write_json(json_path, manifest)
    write_text(json_path.with_suffix(".txt"), runtime_watch_refresh_text(manifest))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_watch_refresh_summary(manifest))


def build_runtime_rebaseline_proposal(
    current_manifest: dict[str, Any],
    current_manifest_path: Path,
    registry: dict[str, Any],
    registry_path: Path,
    selection: dict[str, Any],
    refresh_manifest: dict[str, Any] | None,
    history_summary: dict[str, Any] | None = None,
) -> dict[str, Any]:
    refresh_entries = {
        str(entry.get("execution_class", "")): entry for entry in (refresh_manifest or {}).get("entries", []) if isinstance(entry, dict)
    }
    affected_execution_classes: list[dict[str, Any]] = []
    for current_entry in current_manifest.get("entries", []):
        if not isinstance(current_entry, dict):
            continue
        execution_class = str(current_entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        refresh_entry = refresh_entries.get(execution_class, {})
        affected_execution_classes.append(
            {
                "execution_class": execution_class,
                "wall_time_sec": current_entry.get("wall_time_sec"),
                "current_status": current_entry.get("current_status"),
                "freshness_status": refresh_entry.get("freshness_status"),
                "comparability": refresh_entry.get("comparability"),
                "delta_percent": refresh_entry.get("delta_percent"),
            }
        )

    comparable_candidates: list[dict[str, Any]] = []
    for entry in registry.get("entries", []):
        if not isinstance(entry, dict):
            continue
        if not runtime_candidate_compatible(entry, current_manifest):
            continue
        comparable_candidates.append(
            {
                "baseline_id": entry.get("baseline_id"),
                "baseline_tag": entry.get("baseline_tag"),
                "status": entry.get("status"),
                "approval_timestamp_utc": entry.get("approval_timestamp_utc"),
                "comparability_reason": runtime_compatibility_reason(entry, current_manifest),
            }
        )

    recent_trend: list[dict[str, Any]] = []
    current_fingerprint = str(current_manifest.get("host_fingerprint", {}).get("fingerprint_hash", ""))
    for fingerprint in (history_summary or {}).get("fingerprints", []):
        if str(fingerprint.get("fingerprint_hash", "")) != current_fingerprint:
            continue
        for entry in fingerprint.get("execution_classes", []):
            recent_trend.append(
                {
                    "execution_class": entry.get("execution_class"),
                    "sample_count": entry.get("sample_count"),
                    "trend_direction": entry.get("trend_direction"),
                    "latest_wall_time_sec": entry.get("latest_wall_time_sec"),
                    "delta_vs_selected_baseline_percent": entry.get("delta_vs_selected_baseline_percent"),
                    "budget_verdict": entry.get("budget_verdict"),
                }
            )
        break

    proposal_needed = False
    why = "current runtime remains within the selected baseline"
    why_selected_baseline_is_insufficient = "selected baseline remains acceptable"
    why_no_other_compatible_baseline_works = "selected baseline is sufficient"
    selected_baseline_id = selection.get("selected_baseline_id")
    selected_baseline_tag = selection.get("selected_baseline_tag")
    selected_runtime_baseline_manifest_path = selection.get("selected_runtime_baseline_manifest_path")
    selected_runtime_baseline_manifest_hash = selection.get("selected_runtime_baseline_manifest_hash")
    comparability_verdict = str(selection.get("comparability_verdict", REBASELINE_REQUIRED))
    if refresh_manifest is not None:
        refresh_matches_current = (
            str(refresh_manifest.get("current_runtime_manifest_hash", "")) == str(sha256_file(current_manifest_path))
        )
        refresh_selected_hash = str(refresh_manifest.get("selected_runtime_baseline_manifest_hash", "")).strip()
        if refresh_matches_current and refresh_manifest.get("selected_baseline_id"):
            selected_baseline_id = refresh_manifest.get("selected_baseline_id")
        if refresh_matches_current and refresh_manifest.get("selected_baseline_tag"):
            selected_baseline_tag = refresh_manifest.get("selected_baseline_tag")
        if (
            refresh_matches_current
            and refresh_manifest.get("baseline_runtime_manifest_path")
            and (
                not refresh_selected_hash
                or not selected_runtime_baseline_manifest_hash
                or refresh_selected_hash == str(selected_runtime_baseline_manifest_hash)
            )
        ):
            selected_runtime_baseline_manifest_path = refresh_manifest.get("baseline_runtime_manifest_path")
            selected_runtime_baseline_manifest_hash = refresh_selected_hash or selected_runtime_baseline_manifest_hash
        if refresh_matches_current:
            comparability_verdict = str(refresh_manifest.get("comparability_verdict", comparability_verdict))
    if comparability_verdict != COMPARABLE:
        proposal_needed = True
        why = str(
            (refresh_manifest or {}).get(
                "comparability_reason",
                selection.get("comparability_reason", "runtime registry has no comparable active baseline"),
            )
        )
        why_selected_baseline_is_insufficient = why
        why_no_other_compatible_baseline_works = (
            "no active comparable registry baseline matched strictly"
            if not comparable_candidates
            else "only non-strict or retired compatible baselines were available"
        )
    elif refresh_manifest is not None and (
        str(refresh_manifest.get("current_verdict", VERDICT_PASS)) == VERDICT_FAIL
        or str(refresh_manifest.get("freshness_verdict", FRESHNESS_FRESH)) in {FRESHNESS_STALE, FRESHNESS_REQUIRES_RERUN}
    ):
        proposal_needed = True
        why = "runtime drift persisted relative to the selected baseline"
        why_selected_baseline_is_insufficient = why
        why_no_other_compatible_baseline_works = "registry did not contain a fresher compatible baseline than the selected active entry"

    proposal = {
        "proposal_version": "runtime_rebaseline_proposal_v1",
        "generated_at_utc": timestamp_utc_now(),
        "runtime_current_manifest_path": str(current_manifest_path),
        "runtime_baseline_registry_path": str(registry_path),
        "runtime_current_manifest_hash": sha256_file(current_manifest_path),
        "selected_baseline_id": selected_baseline_id,
        "selected_baseline_tag": selected_baseline_tag,
        "selected_runtime_baseline_manifest_path": selected_runtime_baseline_manifest_path,
        "selected_runtime_baseline_manifest_hash": selected_runtime_baseline_manifest_hash,
        "current_host_fingerprint": current_manifest.get("host_fingerprint", {}),
        "current_toolchain_fingerprint": current_manifest.get("toolchain_fingerprint", {}),
        "comparability_verdict": comparability_verdict,
        "proposal_needed": proposal_needed,
        "why_rebaseline_is_needed": why,
        "why_selected_baseline_is_insufficient": why_selected_baseline_is_insufficient,
        "why_no_other_compatible_baseline_works": why_no_other_compatible_baseline_works,
        "affected_execution_classes": affected_execution_classes,
        "comparable_candidates": comparable_candidates,
        "registry_history": {
            "entry_count": len(registry.get("entries", [])),
            "active_entry_count": sum(1 for entry in registry.get("entries", []) if str(entry.get("status", "")) == REGISTRY_STATUS_ACTIVE),
            "retired_entry_count": sum(1 for entry in registry.get("entries", []) if str(entry.get("status", "")) == REGISTRY_STATUS_RETIRED),
        },
        "recent_trend": recent_trend,
        "measured_runtime_deltas": {
            entry["execution_class"]: entry.get("delta_percent")
            for entry in affected_execution_classes
        },
        "suggested_new_baseline_tag": str(current_manifest.get("phase", "runtime")) + "-runtime-approved",
        "suggested_next_command": (
            "none"
            if not proposal_needed
            else (
                f"./raw_engine_tests --case runtime_proposal_gate "
                f"--runtime-current-manifest {current_manifest_path} "
                f"--runtime-proposal {default_runtime_proposal_path(current_manifest_path)} "
                f"--runtime-history-index {current_manifest_path.parent / 'runtime_history_index_v1.json'} "
                f"--runtime-watch-current {current_manifest_path.parent / (str(current_manifest.get('phase', 'runtime')) and ('runtime_watch_current_' + str(current_manifest.get('phase', 'runtime')) + '.json'))} "
                f"--runtime-watch-refresh {current_manifest_path.parent / (str(current_manifest.get('phase', 'runtime')) and ('runtime_watch_refresh_' + str(current_manifest.get('phase', 'runtime')) + '.json'))} "
                f"--proposal-gate-out {default_runtime_proposal_gate_path(current_manifest_path)} "
                f"--min-real-samples-release 1 --min-real-samples-debug 1 --min-real-samples-asan 5 --min-watch-confidence MEDIUM "
                f"&& ./raw_engine_tests --case runtime_approve_rebaseline "
                f"--runtime-current-manifest {current_manifest_path} "
                f"--runtime-proposal {default_runtime_proposal_path(current_manifest_path)} "
                f"--runtime-proposal-gate {default_runtime_proposal_gate_path(current_manifest_path)} "
                f"--runtime-baseline-registry {registry_path} "
                f"--runtime-baseline-out {current_manifest_path.parent / ('policy_runtime_baseline_' + str(current_manifest.get('phase', 'runtime')) + '_approved.json')} "
                f"--baseline-tag {current_manifest.get('phase', 'runtime')}-runtime-approved --activate "
                f"--archive-proposal {default_runtime_proposal_path(current_manifest_path).with_name(default_runtime_proposal_path(current_manifest_path).stem + '_archived.json')}"
            )
        ),
        "approval_checklist": [
            "confirm the host/toolchain fingerprint is the intended long-lived runtime baseline target",
            "confirm correctness lifecycle remains PASS/FRESH before approving a runtime rebaseline",
            "confirm the runtime proposal gate verdict is APPROVABLE before promoting a new baseline",
            "confirm the affected execution classes are stable across at least one clean rerun",
        ],
        "registry_entry_count": len(registry.get("entries", [])),
    }
    proposal["proposal_hash"] = sha256_text(json.dumps(proposal, sort_keys=True))
    return proposal


def runtime_rebaseline_proposal_text(proposal: dict[str, Any]) -> str:
    lines = [
        f"proposal_version={proposal.get('proposal_version', '')}",
        f"proposal_needed={int(bool(proposal.get('proposal_needed', False)))}",
        f"comparability_verdict={proposal.get('comparability_verdict', '')}",
        f"selected_baseline_id={proposal.get('selected_baseline_id', '')}",
        f"why_rebaseline_is_needed={proposal.get('why_rebaseline_is_needed', '')}",
        f"why_selected_baseline_is_insufficient={proposal.get('why_selected_baseline_is_insufficient', '')}",
        f"why_no_other_compatible_baseline_works={proposal.get('why_no_other_compatible_baseline_works', '')}",
        f"suggested_new_baseline_tag={proposal.get('suggested_new_baseline_tag', '')}",
        f"comparable_candidate_count={len(proposal.get('comparable_candidates', []))}",
    ]
    for entry in proposal.get("affected_execution_classes", []):
        lines.append(
            "runtime_proposal_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" current_status={entry.get('current_status', '')}"
            + f" freshness_status={entry.get('freshness_status', '')}"
        )
    return "\n".join(lines) + "\n"


def runtime_rebaseline_proposal_summary(proposal: dict[str, Any]) -> str:
    return (
        "runtime_rebaseline_proposal_summary"
        + f" proposal_needed={int(bool(proposal.get('proposal_needed', False)))}"
        + f" comparability_verdict={proposal.get('comparability_verdict', '')}"
        + f" selected_baseline_id={proposal.get('selected_baseline_id', '')}"
        + f" comparable_candidate_count={len(proposal.get('comparable_candidates', []))}\n"
    )


def write_runtime_rebaseline_proposal_outputs(json_path: Path, proposal: dict[str, Any]) -> None:
    write_json(json_path, proposal)
    write_text(json_path.with_suffix(".txt"), runtime_rebaseline_proposal_text(proposal))
    write_text(json_path.with_name(f"{json_path.stem}.summary.txt"), runtime_rebaseline_proposal_summary(proposal))


def duplicate_active_runtime_registry_entry(registry: dict[str, Any], suffix: str) -> dict[str, Any]:
    mutated = json.loads(json.dumps(registry))
    active_entries = [
        entry for entry in mutated.get("entries", []) if str(entry.get("status", REGISTRY_STATUS_RETIRED)) == REGISTRY_STATUS_ACTIVE
    ]
    if not active_entries:
        return mutated
    template = json.loads(json.dumps(active_entries[0]))
    template["baseline_id"] = f"{template.get('baseline_id', 'runtime')}-{suffix}"
    template["baseline_tag"] = f"{template.get('baseline_tag', 'runtime')}-{suffix}"
    template["approval_timestamp_utc"] = "2099-01-01T00:00:00Z"
    template["runtime_baseline_manifest_hash"] = sha256_text(
        str(template.get("runtime_baseline_manifest_hash", "")) + "|" + suffix
    )
    mutated.setdefault("entries", []).append(template)
    return finalize_runtime_registry(mutated)


def apply_runtime_registry_fixture(
    registry: dict[str, Any],
    fixture_name: str | None,
) -> dict[str, Any]:
    if not fixture_name:
        return finalize_runtime_registry(json.loads(json.dumps(registry)))
    mutated = json.loads(json.dumps(registry))
    fixture = fixture_name.strip()
    if fixture == "retired_only":
        for entry in mutated.get("entries", []):
            if str(entry.get("status", REGISTRY_STATUS_RETIRED)) == REGISTRY_STATUS_ACTIVE:
                entry["status"] = REGISTRY_STATUS_RETIRED
                entry["retired_timestamp_utc"] = timestamp_utc_now()
                entry["retired_reason"] = "synthetic retired_only fixture"
        return finalize_runtime_registry(mutated)
    if fixture == "multiple_candidates":
        return duplicate_active_runtime_registry_entry(mutated, "synthetic-candidate")
    return finalize_runtime_registry(mutated)


def apply_runtime_environment_fixture(
    current_manifest: dict[str, Any],
    registry: dict[str, Any] | None,
    fixture_name: str | None,
) -> tuple[dict[str, Any], dict[str, Any] | None]:
    mutated_current = json.loads(json.dumps(current_manifest))
    mutated_registry = None if registry is None else apply_runtime_registry_fixture(registry, fixture_name)
    if not fixture_name:
        return normalize_runtime_current_manifest(mutated_current), mutated_registry
    fixture = fixture_name.strip()
    host = mutated_current.setdefault("host_fingerprint", {})
    toolchain = mutated_current.setdefault("toolchain_fingerprint", {})
    if fixture == "same_host_compiler_bump":
        bumped = str(toolchain.get("compiler_version", "")) + " synthetic-minor"
        host["compiler_version"] = bumped
        toolchain["compiler_version"] = bumped
    elif fixture == "sanitizer_change":
        for entry in mutated_current.get("entries", []):
            if not isinstance(entry, dict):
                continue
            if str(entry.get("execution_class", "")) == "release_full":
                entry["sanitizer_flags"] = "asan-lite"
                break
    elif fixture == "runner_tag_change":
        host["runner_tag"] = str(host.get("runner_tag", "")) + "-alt"
    elif fixture == "cross_host":
        host["os"] = "Linux" if str(host.get("os", "")) != "Linux" else "Darwin"
        host["arch"] = "x86_64" if str(host.get("arch", "")) != "x86_64" else "arm64"
    elif fixture in {"retired_only", "multiple_candidates", "exact_match"}:
        pass
    else:
        raise ValueError(f"unknown runtime fixture: {fixture}")
    return normalize_runtime_current_manifest(mutated_current), mutated_registry


def apply_runtime_synthetic_drift(
    current_manifest: dict[str, Any],
    inflate_specs: list[str],
    fingerprint_mismatch: str | None,
) -> dict[str, Any]:
    mutated = json.loads(json.dumps(current_manifest))
    inflate_map: dict[str, float] = {}
    for spec in inflate_specs:
        if "=" not in spec:
            raise ValueError(f"invalid runtime inflate spec: {spec}")
        key, value = spec.split("=", 1)
        inflate_map[key.strip()] = float(value.strip())
    for entry in mutated.get("entries", []):
        execution_class = str(entry.get("execution_class", ""))
        if execution_class in inflate_map:
            factor = inflate_map[execution_class]
            entry["wall_time_sec"] = round(float(entry.get("wall_time_sec", 0.0)) * factor, 3)
            status, delta_percent, rationale = entry_status(
                float(entry["wall_time_sec"]),
                dict(entry.get("budget_thresholds") or default_runtime_threshold(execution_class)),
                None,
            )
            entry["current_status"] = status
            entry["delta_percent"] = delta_percent
            entry["rationale"] = rationale
    if fingerprint_mismatch:
        mutated.setdefault("host_fingerprint", {})
        mutated["host_fingerprint"]["fingerprint_hash"] = sha256_text(
            mutated["host_fingerprint"].get("fingerprint_hash", "") + "|synthetic|" + fingerprint_mismatch
        )
        mutated["host_fingerprint"]["synthetic_mismatch"] = fingerprint_mismatch
    return normalize_runtime_current_manifest(mutated)


def parse_args() -> Any:
    parser = argparse.ArgumentParser(description="Manage runtime lifecycle manifests.")
    parser.add_argument(
        "action",
        choices=[
            "write-current",
            "current",
            "promote-baseline",
            "promote_baseline",
            "registry-promote-baseline",
            "registry_promote_baseline",
            "select-baseline",
            "select_baseline",
            "refresh",
            "plan-rerun",
            "plan_rerun",
            "history-append",
            "history_append",
            "history-summary",
            "history_summary",
            "history-compact",
            "history_compact",
            "propose-rebaseline",
            "propose_rebaseline",
            "proposal-gate",
            "proposal_gate",
            "new-env-proposal-gate",
            "new_env_proposal_gate",
            "approve-rebaseline",
            "approve_rebaseline",
            "approve-new-env-baseline",
            "approve_new_env_baseline",
            "budget-proposal-gate",
            "budget_proposal_gate",
            "budget-approve-reprofile",
            "budget_approve_reprofile",
            "budget-refresh",
            "budget_refresh",
            "budget-plan-rerun",
            "budget_plan_rerun",
            "watch-campaign",
            "watch_campaign",
            "watch-refresh",
            "watch_refresh",
        ],
    )
    parser.add_argument("--phase", default="phase23")
    parser.add_argument("--artifact-root", default="artifacts")
    parser.add_argument("--runtime-current-manifest", default=None)
    parser.add_argument("--runtime-baseline-manifest", default=None)
    parser.add_argument("--runtime-baseline-out", default=None)
    parser.add_argument("--runtime-baseline-registry", default=None)
    parser.add_argument("--runtime-refresh-manifest", default=None)
    parser.add_argument("--runtime-rerun-plan", default=None)
    parser.add_argument("--runtime-history-index", default=None)
    parser.add_argument("--runtime-watch-current", default=None)
    parser.add_argument("--runtime-watch-out", default=None)
    parser.add_argument("--runtime-watch-refresh", default=None)
    parser.add_argument("--runtime-watch-history-index", default=None)
    parser.add_argument("--watch-history-out", default=None)
    parser.add_argument("--runtime-budget-config", default=None)
    parser.add_argument("--runtime-budget-current", default=None)
    parser.add_argument("--runtime-budget-baseline-manifest", default=None)
    parser.add_argument("--runtime-budget-baseline-out", default=None)
    parser.add_argument("--runtime-budget-refresh", default=None)
    parser.add_argument("--runtime-budget-rerun", default=None)
    parser.add_argument("--runtime-budget-registry", default=None)
    parser.add_argument("--runtime-budget-proposal", default=None)
    parser.add_argument("--runtime-budget-proposal-gate", default=None)
    parser.add_argument("--baseline-tag", default="")
    parser.add_argument("--budget-tag", default="")
    parser.add_argument("--retire-baseline", default=None)
    parser.add_argument("--activate", action="store_true")
    parser.add_argument("--runner-tag", default="")
    parser.add_argument("--require-acceptable-status", action="store_true")
    parser.add_argument("--proposal-out", default=None)
    parser.add_argument("--runtime-proposal", default=None)
    parser.add_argument("--proposal-gate-out", default=None)
    parser.add_argument("--runtime-proposal-gate", default=None)
    parser.add_argument("--archive-proposal", default=None)
    parser.add_argument("--min-real-samples-release", type=int, default=1)
    parser.add_argument("--min-real-samples-debug", type=int, default=1)
    parser.add_argument("--min-real-samples-asan", type=int, default=5)
    parser.add_argument("--max-hard-breach-count", type=int, default=0)
    parser.add_argument("--min-watch-confidence", default="MEDIUM")
    parser.add_argument("--execution-class", default="all")
    parser.add_argument("--entry", action="append", default=[])
    parser.add_argument("--runtime-entry", action="append", default=[])
    parser.add_argument("--test-count", action="append", default=[])
    parser.add_argument("--repeat", type=int, default=1)
    parser.add_argument("--sample-window", type=int, default=0)
    parser.add_argument("--stop-on-watch-stable", action="store_true")
    parser.add_argument("--stop-on-escalate", action="store_true")
    parser.add_argument("--synthetic-inflate", action="append", default=[])
    parser.add_argument("--synthetic-fingerprint-mismatch", default=None)
    parser.add_argument("--synthetic-runtime-fixture", default=None)
    parser.add_argument("--compact-out", default=None)
    parser.add_argument("--keep-latest-per-fingerprint", type=int, default=4)
    parser.add_argument("--keep-anchors", type=int, default=1)
    parser.add_argument("--keep-transitions", action="store_true")
    parser.add_argument("--prune-old-fixture-history", action="store_true")
    parser.add_argument("--compact-watch-history", action="store_true")
    parser.add_argument("--evidence-source", default="real")
    parser.add_argument("--runner-id", default="")
    parser.add_argument("--host-label", default="")
    parser.add_argument("--merge-only", action="store_true")
    parser.add_argument("--refresh-after-import", action="store_true")
    return parser.parse_args()


def parse_key_value_map(values: list[str]) -> dict[str, str]:
    out: dict[str, str] = {}
    for value in values:
        if "=" not in value:
            raise ValueError(f"expected key=value: {value}")
        key, mapped = value.split("=", 1)
        out[key.strip()] = mapped.strip()
    return out


def action_write_current(args: Any) -> int:
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    if current_path is None:
        raise SystemExit("--runtime-current-manifest is required")
    artifact_root = Path(args.artifact_root).resolve()
    test_counts = {key: int(value) for key, value in parse_key_value_map(args.test_count).items()}
    raw_specs = list(args.entry) + list(args.runtime_entry)
    entries = []
    for spec in raw_specs:
        entry = parse_runtime_entry_text(spec)
        execution_class = str(entry.get("execution_class", ""))
        if execution_class in test_counts:
            entry["test_count"] = test_counts[execution_class]
        entries.append(entry)
    budget_config = load_runtime_budget_config(args.runtime_budget_config)
    budget_profile = load_runtime_budget_profile(
        args.runtime_budget_config,
        [str(entry.get("execution_class", "")) for entry in entries if str(entry.get("execution_class", ""))],
    )
    manifest = build_runtime_current_manifest(
        args.phase,
        str(artifact_root),
        entries,
        runner_tag=args.runner_tag,
        baseline_tag=args.baseline_tag,
        budget_config=budget_config,
        budget_profile=budget_profile,
    )
    write_runtime_manifest_outputs(current_path, manifest)
    print(str(current_path))
    return 0


def action_promote_baseline(args: Any) -> int:
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    baseline_path = normalize_manifest_path(args.runtime_baseline_out or args.runtime_baseline_manifest)
    if current_path is None or baseline_path is None:
        raise SystemExit("--runtime-current-manifest and --runtime-baseline-manifest are required")
    current_manifest = read_json(current_path)
    baseline = promote_runtime_baseline(
        current_manifest,
        current_path,
        args.baseline_tag or f"{args.phase}-runtime-approved",
        bool(args.require_acceptable_status),
    )
    write_runtime_manifest_outputs(baseline_path, baseline)
    print(str(baseline_path))
    return 0


def action_registry_promote_baseline(args: Any) -> int:
    registry_path = normalize_runtime_registry_path(args.runtime_baseline_registry)
    baseline_path = normalize_manifest_path(args.runtime_baseline_out or args.runtime_baseline_manifest)
    if registry_path is None:
        raise SystemExit("--runtime-baseline-registry is required")
    registry = load_runtime_registry(registry_path)
    if args.retire_baseline:
        registry, retired = retire_runtime_registry_entry(registry, str(args.retire_baseline))
        if retired is None:
            raise SystemExit(f"runtime baseline id not found in registry: {args.retire_baseline}")
        write_runtime_registry_outputs(registry_path, registry)
        print(str(registry_path))
        return 0
    if baseline_path is None:
        raise SystemExit("--runtime-baseline-manifest is required when promoting a runtime registry baseline")
    baseline_manifest = read_json(baseline_path)
    registry, _entry = promote_runtime_baseline_registry(
        registry,
        baseline_manifest,
        baseline_path,
        args.baseline_tag or str(baseline_manifest.get("baseline_tag", "runtime-approved")),
        bool(args.activate),
    )
    write_runtime_registry_outputs(registry_path, registry)
    print(str(registry_path))
    return 0


def action_select_baseline(args: Any) -> int:
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    registry_path = normalize_runtime_registry_path(args.runtime_baseline_registry)
    if current_path is None or registry_path is None:
        raise SystemExit("--runtime-current-manifest and --runtime-baseline-registry are required")
    current_manifest = read_json(current_path)
    registry = load_runtime_registry(registry_path)
    current_manifest, registry = apply_runtime_environment_fixture(current_manifest, registry, args.synthetic_runtime_fixture)
    selection = select_runtime_baseline_from_registry(current_manifest, registry, current_path, registry_path)
    selection_path = default_runtime_selection_path(current_path)
    write_runtime_selection_outputs(selection_path, selection)
    print(str(selection_path))
    return 0


def action_refresh(args: Any) -> int:
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    refresh_path = normalize_manifest_path(args.runtime_refresh_manifest)
    baseline_path = normalize_manifest_path(args.runtime_baseline_manifest)
    registry_path = normalize_runtime_registry_path(args.runtime_baseline_registry)
    if current_path is None or refresh_path is None:
        raise SystemExit("--runtime-current-manifest and --runtime-refresh-manifest are required")
    current_manifest = read_json(current_path)
    current_manifest = apply_runtime_synthetic_drift(
        current_manifest,
        list(args.synthetic_inflate),
        args.synthetic_fingerprint_mismatch,
    )
    registry = load_runtime_registry(registry_path) if registry_path is not None else None
    current_manifest, registry = apply_runtime_environment_fixture(current_manifest, registry, args.synthetic_runtime_fixture)
    selection = None
    baseline_manifest = read_json(baseline_path) if baseline_path is not None and baseline_path.exists() else None
    if registry_path is not None:
        selection = select_runtime_baseline_from_registry(current_manifest, registry, current_path, registry_path)
        selection_path = default_runtime_selection_path(current_path)
        write_runtime_selection_outputs(selection_path, selection)
        selected_manifest, selected_path = selection_baseline_manifest(selection)
        if selected_manifest is not None and selected_path is not None:
            baseline_manifest = selected_manifest
            baseline_path = selected_path
        else:
            baseline_manifest = None
            baseline_path = None
    refresh = refresh_runtime_manifest(
        baseline_manifest,
        current_manifest,
        baseline_path if baseline_path is not None and baseline_path.exists() else None,
        current_path,
        baseline_selection=selection,
        runtime_registry_path=registry_path,
    )
    write_runtime_refresh_outputs(refresh_path, refresh)
    print(str(refresh_path))
    return 0


def action_plan_rerun(args: Any) -> int:
    refresh_path = normalize_manifest_path(args.runtime_refresh_manifest)
    rerun_path = normalize_manifest_path(args.runtime_rerun_plan)
    baseline_path = normalize_manifest_path(args.runtime_baseline_manifest)
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    if refresh_path is None or rerun_path is None or current_path is None:
        raise SystemExit("--runtime-refresh-manifest, --runtime-rerun-plan, and --runtime-current-manifest are required")
    refresh_manifest = read_json(refresh_path)
    plan = build_runtime_rerun_plan(refresh_manifest, baseline_path, current_path, refresh_path)
    write_runtime_rerun_plan_outputs(rerun_path, plan)
    print(str(rerun_path))
    return 0


def action_history_append(args: Any) -> int:
    history_path = normalize_manifest_path(args.runtime_history_index)
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    refresh_path = normalize_manifest_path(args.runtime_refresh_manifest)
    if history_path is None or current_path is None:
        raise SystemExit("--runtime-history-index and --runtime-current-manifest are required")
    if refresh_path is None:
        refresh_path = infer_runtime_refresh_path(current_path)
    history_index = read_json(history_path) if history_path.exists() else empty_runtime_history_index()
    current_manifest = read_json(current_path)
    refresh_manifest = read_json(refresh_path) if refresh_path is not None and refresh_path.exists() else None
    history_index = append_runtime_history(
        history_index,
        current_manifest,
        current_path,
        refresh_manifest,
        refresh_path,
        evidence_source=args.evidence_source,
        runner_id=args.runner_id,
        host_label=args.host_label,
    )
    history_summary_path = write_runtime_history_outputs(history_path, history_index)
    print(str(history_summary_path))
    return 0


def action_history_summary(args: Any) -> int:
    history_path = normalize_manifest_path(args.runtime_history_index)
    if history_path is None:
        raise SystemExit("--runtime-history-index is required")
    history_index = read_json(history_path) if history_path.exists() else empty_runtime_history_index()
    history_summary_path = write_runtime_history_outputs(history_path, history_index)
    print(str(history_summary_path))
    return 0


def action_history_compact(args: Any) -> int:
    history_path = normalize_manifest_path(args.runtime_history_index)
    compact_out = normalize_manifest_path(args.compact_out)
    if history_path is None or compact_out is None:
        raise SystemExit("--runtime-history-index and --compact-out are required")
    history_index = read_json(history_path) if history_path.exists() else empty_runtime_history_index()
    compacted = compact_runtime_history_index(
        history_index,
        int(args.keep_latest_per_fingerprint),
        int(args.keep_anchors),
        bool(args.keep_transitions),
        bool(args.prune_old_fixture_history),
    )
    compact_summary_path = write_runtime_history_outputs(compact_out, compacted)
    if bool(args.compact_watch_history):
        watch_history_path = history_path.parent / "runtime_watch_history_index_v1.json"
        if watch_history_path.exists():
            watch_history = read_json(watch_history_path)
            write_json(
                watch_history_path.with_name(f"{watch_history_path.stem}_compacted.json"),
                {
                    "history_version": "runtime_watch_history_index_v1_compacted",
                    "generated_at_utc": timestamp_utc_now(),
                    "source_runtime_watch_history_index": str(watch_history_path),
                    "fingerprint_count": len(watch_history.get("fingerprints", [])),
                    "summary_path": str(default_runtime_watch_history_summary_path(watch_history_path)),
                },
            )
    print(str(compact_summary_path))
    return 0


def action_watch_campaign(args: Any) -> int:
    baseline_path = normalize_manifest_path(args.runtime_baseline_manifest)
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    refresh_path = normalize_manifest_path(args.runtime_refresh_manifest)
    watch_current_path = normalize_manifest_path(args.runtime_watch_current or args.runtime_watch_out)
    watch_history_index_path = normalize_manifest_path(args.runtime_watch_history_index or args.watch_history_out)
    history_index_path = normalize_manifest_path(args.runtime_history_index)
    if baseline_path is None or current_path is None or watch_current_path is None:
        raise SystemExit("--runtime-baseline-manifest, --runtime-current-manifest, and --runtime-watch-current/--runtime-watch-out are required")
    if refresh_path is None:
        refresh_path = infer_runtime_refresh_path(current_path)
    current_manifest = read_json(current_path)
    current_manifest = apply_runtime_synthetic_drift(
        current_manifest,
        list(args.synthetic_inflate),
        args.synthetic_fingerprint_mismatch,
    )
    current_manifest, _registry = apply_runtime_environment_fixture(current_manifest, None, args.synthetic_runtime_fixture)
    baseline_manifest = read_json(baseline_path) if baseline_path.exists() else None
    refresh_manifest = read_json(refresh_path) if refresh_path is not None and refresh_path.exists() else None
    history_index = read_json(history_index_path) if history_index_path is not None and history_index_path.exists() else empty_runtime_history_index()
    execution_classes = runtime_execution_classes(current_manifest)
    if args.execution_class and str(args.execution_class).strip().lower() != "all":
        execution_classes = [value.strip() for value in str(args.execution_class).split(",") if value.strip()]
    budget_profile = runtime_budget_profile_for_manifest(current_manifest, args.runtime_budget_config)
    sample_window = int(args.sample_window) if int(args.sample_window) > 0 else max(int(args.repeat), 1)
    watch_current = build_runtime_watch_current(
        baseline_manifest,
        baseline_path,
        current_manifest,
        current_path,
        refresh_manifest,
        refresh_path,
        history_index,
        watch_history_index_path,
        execution_classes,
        sample_window,
        budget_profile,
        watch_current_path,
    )
    watch_current["requested_repeat"] = max(int(args.repeat), 1)
    watch_current["sample_window"] = sample_window
    watch_current["stop_on_watch_stable"] = bool(args.stop_on_watch_stable)
    watch_current["stop_on_escalate"] = bool(args.stop_on_escalate)
    write_runtime_watch_current_outputs(watch_current_path, watch_current)
    print(str(watch_current_path))
    return 0


def action_watch_refresh(args: Any) -> int:
    baseline_path = normalize_manifest_path(args.runtime_baseline_manifest)
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    refresh_path = normalize_manifest_path(args.runtime_refresh_manifest)
    watch_current_path = normalize_manifest_path(args.runtime_watch_current or args.runtime_watch_out)
    watch_refresh_path = normalize_manifest_path(args.runtime_watch_refresh)
    watch_history_index_path = normalize_manifest_path(args.runtime_watch_history_index or args.watch_history_out)
    if current_path is None or refresh_path is None or watch_current_path is None or watch_refresh_path is None:
        raise SystemExit("--runtime-current-manifest, --runtime-refresh-manifest, --runtime-watch-current/--runtime-watch-out, and --runtime-watch-refresh are required")
    current_manifest = read_json(current_path)
    current_manifest = apply_runtime_synthetic_drift(
        current_manifest,
        list(args.synthetic_inflate),
        args.synthetic_fingerprint_mismatch,
    )
    current_manifest, _registry = apply_runtime_environment_fixture(current_manifest, None, args.synthetic_runtime_fixture)
    refresh_manifest = read_json(refresh_path)
    baseline_manifest = read_json(baseline_path) if baseline_path is not None and baseline_path.exists() else None
    watch_current_manifest = read_json(watch_current_path)
    watch_refresh = build_runtime_watch_refresh(
        baseline_manifest,
        baseline_path,
        current_manifest,
        current_path,
        refresh_manifest,
        refresh_path,
        watch_current_manifest,
        watch_current_path,
    )
    write_runtime_watch_refresh_outputs(watch_refresh_path, watch_refresh)
    if watch_history_index_path is not None:
        watch_history_index = (
            read_json(watch_history_index_path)
            if watch_history_index_path.exists()
            else empty_runtime_watch_history_index()
        )
        watch_history_index = append_runtime_watch_history(
            watch_history_index,
            current_manifest,
            watch_refresh,
            watch_current_path,
            watch_refresh_path,
        )
        write_runtime_watch_history_outputs(watch_history_index_path, watch_history_index)
    print(str(watch_refresh_path))
    return 0


def action_propose_rebaseline(args: Any) -> int:
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    registry_path = normalize_runtime_registry_path(args.runtime_baseline_registry)
    history_path = normalize_manifest_path(args.runtime_history_index)
    proposal_arg = args.runtime_proposal or args.proposal_out
    proposal_path = normalize_manifest_path(proposal_arg) if proposal_arg else None
    if current_path is None or registry_path is None:
        raise SystemExit("--runtime-current-manifest and --runtime-baseline-registry are required")
    if proposal_path is None:
        proposal_path = default_runtime_proposal_path(current_path)
    current_manifest = read_json(current_path)
    registry = load_runtime_registry(registry_path)
    current_manifest = apply_runtime_synthetic_drift(
        current_manifest,
        [],
        args.synthetic_fingerprint_mismatch,
    )
    current_manifest, registry = apply_runtime_environment_fixture(current_manifest, registry, args.synthetic_runtime_fixture)
    selection = select_runtime_baseline_from_registry(current_manifest, registry, current_path, registry_path)
    refresh_path = normalize_manifest_path(args.runtime_refresh_manifest) or infer_runtime_refresh_path(current_path)
    refresh_manifest = read_json(refresh_path) if refresh_path is not None and refresh_path.exists() else None
    history_summary = None
    if history_path is not None and history_path.exists():
        history_summary_path = default_runtime_history_summary_path(history_path)
        if history_summary_path.exists():
            history_summary = read_json(history_summary_path)
    proposal = build_runtime_rebaseline_proposal(
        current_manifest,
        current_path,
        registry,
        registry_path,
        selection,
        refresh_manifest,
        history_summary,
    )
    write_runtime_rebaseline_proposal_outputs(proposal_path, proposal)
    print(str(proposal_path))
    return 0


def action_proposal_gate(args: Any) -> int:
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    proposal_path = normalize_manifest_path(args.runtime_proposal or args.proposal_out)
    history_path = normalize_manifest_path(args.runtime_history_index)
    watch_current_path = normalize_manifest_path(args.runtime_watch_current or args.runtime_watch_out)
    watch_refresh_path = normalize_manifest_path(args.runtime_watch_refresh)
    gate_path = normalize_manifest_path(args.runtime_proposal_gate or args.proposal_gate_out)
    if (
        current_path is None
        or proposal_path is None
        or history_path is None
        or watch_current_path is None
        or watch_refresh_path is None
    ):
        raise SystemExit(
            "--runtime-current-manifest, --runtime-proposal/--proposal-out, --runtime-history-index, "
            "--runtime-watch-current/--runtime-watch-out, --runtime-watch-refresh, and "
            "--runtime-proposal-gate/--proposal-gate-out are required"
        )
    if gate_path is None:
        gate_path = default_runtime_proposal_gate_path(current_path)
    current_manifest = read_json(current_path)
    proposal = read_json(proposal_path)
    history_index = read_json(history_path) if history_path.exists() else empty_runtime_history_index()
    history_index["runtime_history_index_path"] = str(history_path)
    watch_current = read_json(watch_current_path)
    watch_current["runtime_watch_current_path"] = str(watch_current_path)
    watch_refresh = read_json(watch_refresh_path)
    watch_refresh["runtime_watch_refresh_path"] = str(watch_refresh_path)
    gate = build_runtime_proposal_gate(
        current_manifest,
        current_path,
        proposal,
        proposal_path,
        history_index,
        watch_current,
        watch_refresh,
        proposal_gate_thresholds(args),
        normalize_watch_confidence_requirement(args.min_watch_confidence),
    )
    write_runtime_proposal_gate_outputs(gate_path, gate)
    print(str(gate_path))
    return 0


def action_approve_rebaseline(args: Any) -> int:
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    proposal_path = normalize_manifest_path(args.runtime_proposal or args.proposal_out)
    proposal_gate_path = normalize_manifest_path(args.runtime_proposal_gate or args.proposal_gate_out)
    registry_path = normalize_runtime_registry_path(args.runtime_baseline_registry)
    baseline_path = normalize_manifest_path(args.runtime_baseline_out or args.runtime_baseline_manifest)
    archive_proposal_path = normalize_manifest_path(args.archive_proposal) if args.archive_proposal else None
    if (
        current_path is None
        or proposal_path is None
        or registry_path is None
        or baseline_path is None
    ):
        raise SystemExit(
            "--runtime-current-manifest, --runtime-proposal/--proposal-out, --runtime-proposal-gate/--proposal-gate-out, "
            "--runtime-baseline-registry, and "
            "--runtime-baseline-out/--runtime-baseline-manifest are required"
        )
    if proposal_gate_path is None:
        proposal_gate_path = default_runtime_proposal_gate_path(current_path)
    current_manifest = read_json(current_path)
    proposal = read_json(proposal_path)
    proposal_gate = read_json(proposal_gate_path)
    registry = load_runtime_registry(registry_path)
    baseline_tag = args.baseline_tag or str(proposal.get("suggested_new_baseline_tag", f"{args.phase}-runtime-approved"))
    (
        updated_current,
        approved_baseline,
        updated_registry,
        approval_metadata,
        archived_proposal,
    ) = approve_runtime_rebaseline(
        current_manifest,
        current_path,
        proposal,
        proposal_path,
        proposal_gate,
        proposal_gate_path,
        registry,
        registry_path,
        baseline_path,
        baseline_tag,
        bool(args.activate),
        archive_proposal_path,
        bool(args.require_acceptable_status),
    )
    write_runtime_manifest_outputs(current_path, updated_current)
    write_runtime_manifest_outputs(baseline_path, approved_baseline)
    final_baseline_hash = sha256_file(baseline_path)
    approval_metadata["new_active_runtime_baseline_manifest_hash"] = final_baseline_hash
    approved_baseline["approval_metadata"] = approval_metadata
    write_runtime_manifest_outputs(baseline_path, approved_baseline)
    final_baseline_hash = sha256_file(baseline_path)
    approval_metadata["new_active_runtime_baseline_manifest_hash"] = final_baseline_hash
    for entry in updated_registry.get("entries", []):
        if str(entry.get("baseline_id", "")) == str(approval_metadata.get("new_active_runtime_baseline_id", "")):
            entry["runtime_baseline_manifest_hash"] = final_baseline_hash
            entry["runtime_baseline_manifest_path"] = str(baseline_path)
            break
    updated_registry = finalize_runtime_registry(updated_registry)
    approval_metadata_path = default_runtime_approval_metadata_path(baseline_path)
    write_runtime_approval_metadata_outputs(approval_metadata_path, approval_metadata)
    write_runtime_registry_outputs(registry_path, updated_registry)
    if archive_proposal_path is not None and archived_proposal is not None:
        write_runtime_rebaseline_proposal_outputs(archive_proposal_path, archived_proposal)
    print(str(baseline_path))
    return 0


def action_budget_proposal_gate(args: Any) -> int:
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    baseline_path = normalize_manifest_path(args.runtime_baseline_manifest or args.runtime_budget_baseline_manifest)
    watch_current_path = normalize_manifest_path(args.runtime_watch_current or args.runtime_watch_out)
    watch_refresh_path = normalize_manifest_path(args.runtime_watch_refresh)
    history_path = normalize_manifest_path(args.runtime_history_index)
    proposal_path = normalize_manifest_path(args.runtime_budget_proposal or args.proposal_out)
    gate_path = normalize_manifest_path(args.runtime_budget_proposal_gate or args.proposal_gate_out)
    current_budget_path = normalize_manifest_path(args.runtime_budget_current)
    if current_path is None or watch_current_path is None or watch_refresh_path is None:
        raise SystemExit(
            "--runtime-current-manifest, --runtime-watch-current/--runtime-watch-out, and --runtime-watch-refresh are required"
        )
    if current_budget_path is None:
        current_budget_path = default_runtime_budget_current_path(current_path)
    if proposal_path is None:
        proposal_path = default_runtime_budget_proposal_path(current_path)
    if gate_path is None:
        gate_path = default_runtime_budget_proposal_gate_path(current_path)
    current_manifest = read_json(current_path)
    baseline_manifest = read_json(baseline_path) if baseline_path is not None and baseline_path.exists() else None
    watch_current = read_json(watch_current_path)
    watch_refresh = read_json(watch_refresh_path)
    history_index = read_json(history_path) if history_path is not None and history_path.exists() else empty_runtime_history_index()
    budget_profile = runtime_budget_profile_for_manifest(current_manifest, args.runtime_budget_config)
    budget_current = build_runtime_budget_current(
        current_manifest,
        current_path,
        baseline_manifest,
        baseline_path if baseline_path is not None and baseline_path.exists() else None,
        watch_current,
        watch_refresh,
        history_index,
        budget_profile,
    )
    write_runtime_budget_current_outputs(current_budget_path, budget_current)
    proposal = build_runtime_budget_proposal(
        budget_current,
        current_budget_path,
        args.budget_tag or f"{args.phase}-runtime-budget-approved",
    )
    write_runtime_budget_proposal_outputs(proposal_path, proposal)
    gate = build_runtime_budget_proposal_gate(
        budget_current,
        current_budget_path,
        proposal,
        proposal_path,
        int(args.min_real_samples_release),
        int(args.max_hard_breach_count),
        normalize_watch_confidence_requirement(args.min_watch_confidence),
    )
    write_runtime_budget_proposal_gate_outputs(gate_path, gate)
    print(str(gate_path))
    return 0


def action_budget_approve_reprofile(args: Any) -> int:
    current_budget_path = normalize_manifest_path(args.runtime_budget_current)
    proposal_path = normalize_manifest_path(args.runtime_budget_proposal or args.proposal_out)
    gate_path = normalize_manifest_path(args.runtime_budget_proposal_gate or args.proposal_gate_out)
    registry_path = normalize_runtime_budget_registry_path(args.runtime_budget_registry)
    baseline_out_path = normalize_manifest_path(args.runtime_budget_baseline_out or args.runtime_budget_baseline_manifest)
    archive_proposal_path = normalize_manifest_path(args.archive_proposal) if args.archive_proposal else None
    if (
        current_budget_path is None
        or proposal_path is None
        or gate_path is None
        or registry_path is None
        or baseline_out_path is None
    ):
        raise SystemExit(
            "--runtime-budget-current, --runtime-budget-proposal, --runtime-budget-proposal-gate, "
            "--runtime-budget-registry, and --runtime-budget-baseline-out/--runtime-budget-baseline-manifest are required"
        )
    runtime_budget_current = read_json(current_budget_path)
    proposal = read_json(proposal_path)
    proposal_gate = read_json(gate_path)
    registry = load_runtime_budget_registry(registry_path)
    (
        updated_current_manifest,
        approved_budget_baseline,
        updated_registry,
        approval_metadata,
        archived_proposal,
    ) = approve_runtime_budget_reprofile(
        runtime_budget_current,
        current_budget_path,
        proposal,
        proposal_path,
        proposal_gate,
        gate_path,
        registry,
        registry_path,
        baseline_out_path,
        args.budget_tag or f"{args.phase}-runtime-budget-approved",
        bool(args.activate),
        archive_proposal_path,
    )
    runtime_current_path = Path(str(runtime_budget_current.get("runtime_current_manifest_path", ""))).resolve()
    write_runtime_manifest_outputs(runtime_current_path, updated_current_manifest)
    write_runtime_budget_profile_outputs(baseline_out_path, approved_budget_baseline)
    approval_metadata_path = default_runtime_approval_metadata_path(baseline_out_path)
    write_runtime_approval_metadata_outputs(approval_metadata_path, approval_metadata)
    write_runtime_budget_registry_outputs(registry_path, updated_registry)
    if archive_proposal_path is not None and archived_proposal is not None:
        write_runtime_budget_proposal_outputs(archive_proposal_path, archived_proposal)
    print(str(baseline_out_path))
    return 0


def action_budget_refresh(args: Any) -> int:
    current_path = normalize_manifest_path(args.runtime_current_manifest)
    refresh_path = normalize_manifest_path(args.runtime_budget_refresh)
    baseline_path = normalize_manifest_path(args.runtime_budget_baseline_manifest)
    registry_path = normalize_runtime_budget_registry_path(args.runtime_budget_registry)
    if current_path is None or refresh_path is None:
        raise SystemExit("--runtime-current-manifest and --runtime-budget-refresh are required")
    if baseline_path is None and registry_path is not None:
        registry = load_runtime_budget_registry(registry_path)
        baseline_path = active_runtime_budget_profile_path(registry)
    current_manifest = read_json(current_path)
    budget_baseline_manifest = read_json(baseline_path) if baseline_path is not None and baseline_path.exists() else None
    refresh = build_runtime_budget_refresh(
        current_manifest,
        current_path,
        budget_baseline_manifest,
        baseline_path if baseline_path is not None and baseline_path.exists() else None,
    )
    write_runtime_budget_refresh_outputs(refresh_path, refresh)
    print(str(refresh_path))
    return 0


def action_budget_plan_rerun(args: Any) -> int:
    refresh_path = normalize_manifest_path(args.runtime_budget_refresh)
    rerun_path = normalize_manifest_path(args.runtime_budget_rerun)
    if refresh_path is None or rerun_path is None:
        raise SystemExit("--runtime-budget-refresh and --runtime-budget-rerun are required")
    refresh_manifest = read_json(refresh_path)
    plan = build_runtime_budget_rerun_plan(refresh_manifest)
    write_runtime_budget_rerun_plan_outputs(rerun_path, plan)
    print(str(rerun_path))
    return 0


def main() -> int:
    args = parse_args()
    if args.action in {"write-current", "current"}:
        return action_write_current(args)
    if args.action in {"promote-baseline", "promote_baseline"}:
        return action_promote_baseline(args)
    if args.action in {"registry-promote-baseline", "registry_promote_baseline"}:
        return action_registry_promote_baseline(args)
    if args.action in {"select-baseline", "select_baseline"}:
        return action_select_baseline(args)
    if args.action == "refresh":
        return action_refresh(args)
    if args.action in {"plan-rerun", "plan_rerun"}:
        return action_plan_rerun(args)
    if args.action in {"history-append", "history_append"}:
        return action_history_append(args)
    if args.action in {"history-summary", "history_summary"}:
        return action_history_summary(args)
    if args.action in {"history-compact", "history_compact"}:
        return action_history_compact(args)
    if args.action in {"watch-campaign", "watch_campaign"}:
        return action_watch_campaign(args)
    if args.action in {"watch-refresh", "watch_refresh"}:
        return action_watch_refresh(args)
    if args.action in {"propose-rebaseline", "propose_rebaseline"}:
        return action_propose_rebaseline(args)
    if args.action in {"proposal-gate", "proposal_gate", "new-env-proposal-gate", "new_env_proposal_gate"}:
        return action_proposal_gate(args)
    if args.action in {"approve-rebaseline", "approve_rebaseline", "approve-new-env-baseline", "approve_new_env_baseline"}:
        return action_approve_rebaseline(args)
    if args.action in {"budget-proposal-gate", "budget_proposal_gate"}:
        return action_budget_proposal_gate(args)
    if args.action in {"budget-approve-reprofile", "budget_approve_reprofile"}:
        return action_budget_approve_reprofile(args)
    if args.action in {"budget-refresh", "budget_refresh"}:
        return action_budget_refresh(args)
    if args.action in {"budget-plan-rerun", "budget_plan_rerun"}:
        return action_budget_plan_rerun(args)
    raise SystemExit(f"unsupported action: {args.action}")


if __name__ == "__main__":
    raise SystemExit(main())
