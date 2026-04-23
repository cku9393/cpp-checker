#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import os
import time
import zipfile
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any

import runtime_gate_lib as runtime_gate


WATCH_STATUS_ORDER = {
    "CLEAR": 0,
    "WATCH": 1,
    "WATCH_STABLE": 2,
    "WATCH_ESCALATE": 3,
    "REBASELINE_CANDIDATE": 4,
    "REBASELINE_REQUIRED": 5,
    "FAIL": 6,
}

KNOWN_ENV_STALE_AFTER_HOURS = 24.0 * 14.0
KNOWN_ENV_REVERIFY_AFTER_HOURS = 24.0 * 30.0
KNOWN_ENV_RETIRE_AFTER_HOURS = 24.0 * 90.0

ENV_STATE_CURRENT_ACTIVE = "CURRENT_ACTIVE"
ENV_STATE_APPROVED_KNOWN_FRESH = "APPROVED_KNOWN_FRESH"
ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED = "APPROVED_KNOWN_REVERIFY_REQUIRED"
ENV_STATE_APPROVED_KNOWN_STALE = "APPROVED_KNOWN_STALE"
ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE = "APPROVED_KNOWN_RETIRE_CANDIDATE"
ENV_STATE_RETIRED_KNOWN_ENV = "RETIRED_KNOWN_ENV"
ENV_STATE_FOREIGN_UNAPPROVED = "FOREIGN_UNAPPROVED"

CURRENT_ENV_STATE_CLEAR = "CURRENT_ENV_CLEAR"
CURRENT_ENV_STATE_APPROVAL_GRACE = "CURRENT_ENV_APPROVAL_GRACE"
CURRENT_ENV_STATE_MONITORING_DUE_SOON = "CURRENT_ENV_MONITORING_DUE_SOON"
CURRENT_ENV_STATE_MONITORING_DUE = "CURRENT_ENV_MONITORING_DUE"
CURRENT_ENV_STATE_WATCH = "CURRENT_ENV_WATCH"
CURRENT_ENV_STATE_WATCH_STABLE = "CURRENT_ENV_WATCH_STABLE"
CURRENT_ENV_STATE_WATCH_ESCALATE = "CURRENT_ENV_WATCH_ESCALATE"
CURRENT_ENV_STATE_REPROFILE_CANDIDATE = "CURRENT_ENV_REPROFILE_CANDIDATE"
CURRENT_ENV_STATE_REPROFILE_REQUIRED = "CURRENT_ENV_REPROFILE_REQUIRED"
CURRENT_ENV_STATE_FAIL = "CURRENT_ENV_FAIL"

CURRENT_ENV_STATE_ORDER = {
    CURRENT_ENV_STATE_CLEAR: 0,
    CURRENT_ENV_STATE_APPROVAL_GRACE: 1,
    CURRENT_ENV_STATE_MONITORING_DUE_SOON: 2,
    CURRENT_ENV_STATE_MONITORING_DUE: 3,
    CURRENT_ENV_STATE_WATCH: 4,
    CURRENT_ENV_STATE_WATCH_STABLE: 5,
    CURRENT_ENV_STATE_WATCH_ESCALATE: 6,
    CURRENT_ENV_STATE_REPROFILE_CANDIDATE: 7,
    CURRENT_ENV_STATE_REPROFILE_REQUIRED: 8,
    CURRENT_ENV_STATE_FAIL: 9,
}

CURRENT_ENV_DUE_NOT_DUE = "NOT_DUE"
CURRENT_ENV_DUE_SOON = "DUE_SOON"
CURRENT_ENV_DUE = "DUE"
CURRENT_ENV_DUE_OVERDUE = "OVERDUE"

ACTION_STATUS_PLANNED = "PLANNED"
ACTION_STATUS_EXECUTED = "EXECUTED"
ACTION_STATUS_APPLIED = "APPLIED"
ACTION_STATUS_SKIPPED = "SKIPPED"
ACTION_STATUS_FAILED = "FAILED"
ACTION_STATUS_SUPERSEDED = "SUPERSEDED"
ACTION_STATUS_DEFERRED = "DEFERRED"
ACTION_STATUS_REJECTED = "REJECTED"
ACTION_STATUS_CLOSED = "CLOSED"
ACTION_STATUS_RETRY_PENDING = "RETRY_PENDING"

CLOSURE_STATUS_OPEN = "OPEN"
CLOSURE_STATUS_CLOSED = "CLOSED"
CLOSURE_STATUS_DEFERRED = "DEFERRED"
CLOSURE_STATUS_REJECTED = "REJECTED"
CLOSURE_STATUS_RETRY_PENDING = "RETRY_PENDING"
CLOSURE_STATUS_APPROVAL_APPLIED = "APPROVAL_APPLIED"

OPERATOR_DECISION_APPROVE = "approve"
OPERATOR_DECISION_SKIP = "skip"
OPERATOR_DECISION_DEFER = "defer"
OPERATOR_DECISION_REJECT = "reject"
OPERATOR_DECISION_CLOSE = "close"
OPERATOR_DECISION_RETRY_NOW = "retry_now"

ACTION_RETRY_STATUS_NONE = "NO_RETRY"
ACTION_RETRY_STATUS_RETRYABLE = "RETRYABLE"
ACTION_RETRY_STATUS_ESCALATE = "ESCALATE"

ACTION_HANDOFF_NOT_READY = "NOT_READY"
ACTION_HANDOFF_APPROVAL_READY = "APPROVAL_READY"
ACTION_HANDOFF_ESCALATE = "ESCALATE"

APPROVAL_STATUS_APPLIED = "APPLIED"
APPROVAL_STATUS_BLOCKED = "BLOCKED"
APPROVAL_STATUS_FAILED = "FAILED"
APPROVAL_STATUS_DRY_RUN = "DRY_RUN"

APPROVAL_EXECUTION_MODE_DRY_RUN = "dry_run"
APPROVAL_EXECUTION_MODE_HANDOFF_ONLY = "handoff_only"
APPROVAL_EXECUTION_MODE_INTEGRATED_OPT_IN = "integrated_opt_in"

APPROVED_KNOWN_ACTION_RANK = {
    "NO_ACTION": 0,
    "CONTINUE_MONITORING": 1,
    "WATCH_RUNTIME": 2,
    "REVERIFY_KNOWN_ENV": 3,
    "REBASELINE_REQUIRED": 4,
    "RETIRE_KNOWN_ENV": 5,
    "KEEP_AS_ARCHIVE_ONLY": 6,
    "FAIL": 7,
}

WATCH_CONFIDENCE_RANK = {"LOW": 0, "MEDIUM": 1, "HIGH": 2}

DEFAULT_KNOWN_ENV_GOVERNANCE_POLICY = {
    "manifest_version": "runtime_known_env_governance_policy_v1",
    "policy_id": "phase40-known-env-governance",
    "phase": "phase40",
    "reverify_due_after_days": 14,
    "stale_after_days": 30,
    "retire_candidate_after_days": 60,
    "retire_after_days": 90,
    "due_soon_window_days": 3,
    "min_real_samples_for_reverify": 5,
    "min_watch_confidence_for_freshen": "HIGH",
    "retain_retired_baselines_days": 90,
    "retain_retired_known_env_entries_days": 180,
    "counts_as_approved_known_env_requirements": {
        "registry_status": runtime_gate.REGISTRY_STATUS_ACTIVE,
        "counts_as_current_env": False,
        "requires_runtime_baseline_manifest_path": True,
    },
    "roles": {
        "current_env": {
            "state": ENV_STATE_CURRENT_ACTIVE,
            "counts_as_current_env": True,
            "counts_as_approved_known_env": False,
        },
        "approved_known_env": {
            "counts_as_current_env": False,
            "counts_as_approved_known_env": True,
        },
        "retired_known_env": {
            "state": ENV_STATE_RETIRED_KNOWN_ENV,
            "counts_as_current_env": False,
            "counts_as_approved_known_env": False,
        },
        "foreign_env": {
            "state": ENV_STATE_FOREIGN_UNAPPROVED,
            "counts_as_current_env": False,
            "counts_as_approved_known_env": False,
        },
    },
}

DEFAULT_CURRENT_ENV_GOVERNANCE_POLICY = {
    "manifest_version": "runtime_current_env_guardrail_policy_v2",
    "policy_id": "phase42-current-env-guardrail",
    "phase": "phase42",
    "post_approval_grace_days": 7,
    "post_approval_min_real_samples_before_rewatch": 5,
    "watch_due_after_days": 14,
    "watch_due_soon_window_days": 3,
    "stable_soft_overrun_trigger_count": 5,
    "stable_soft_overrun_trigger_ratio": 0.25,
    "hard_over_budget_trigger_count": 1,
    "reproposal_candidate_after_days": 21,
    "reproposal_cooldown_days": 14,
    "reprofile_candidate_threshold": 5,
    "watch_stable_threshold": 3,
    "watch_escalate_hard_breach_count": 1,
    "min_real_samples_for_reproposal": 5,
    "min_watch_confidence_for_reproposal": "HIGH",
    "min_watch_confidence_for_reprofile": "HIGH",
    "max_hard_breach_count_for_reproposal": 0,
    "max_bounded_jitter_percent_for_reproposal": 15.0,
    "bounded_jitter_percent": 15.0,
    "profile_switch_history_retention_days": 180,
    "current_env_execution_classes": [
        "release_full",
        "debug_full",
        "asan_full",
        "policy_core",
        "policy_refresh",
        "policy_nightly",
    ],
    "production_critical_execution_classes": ["release_full"],
    "diagnostic_guard_execution_classes": ["debug_full", "asan_full"],
    "current_env_roles": {
        "production_critical": {"prefer_reprofile_candidate": True},
        "diagnostic": {"prefer_reprofile_candidate": False},
        "operator": {"prefer_reprofile_candidate": False},
    },
}

DEFAULT_CURRENT_ENV_ACTION_RETRY_POLICY = {
    "manifest_version": "runtime_current_env_action_retry_policy_v1",
    "policy_id": "phase45-current-env-action-retry",
    "phase": "phase45",
    "max_retry_count": 2,
    "retry_backoff_minutes": 30,
    "retryable_failure_classes": ["INFRA_TRANSIENT", "TIMEOUT", "MISSING_ARTIFACT"],
    "non_retryable_failure_classes": ["SEMANTIC_MISMATCH", "CORRECTNESS_FAIL", "PLANNER_DRIFT"],
    "supersede_on_new_due": True,
    "skip_requires_reason": True,
    "failed_action_escalation_threshold": 2,
}

DEFAULT_OPERATOR_RUNBOOK_RETENTION_POLICY = {
    "manifest_version": "operator_runbook_retention_policy_v1",
    "policy_id": "phase50-operator-runbook-retention",
    "phase": "phase50",
    "keep_active_runbooks": True,
    "keep_failed_runbooks": True,
    "keep_retry_pending_runbooks": True,
    "keep_integrated_approval_runbooks": True,
    "keep_latest_resolved_per_type": 2,
    "archive_resolved_after_days": 30,
    "prune_archived_after_days": 180,
    "keep_runbook_with_open_ledger_pointer": True,
    "keep_runbook_with_approval_transaction": True,
}


def atomic_write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = path.with_name(f".{path.name}.tmp.{os.getpid()}.{time.time_ns()}")
    temp_path.write_text(text, encoding="utf-8")
    os.replace(temp_path, path)


def write_json(path: Path, data: dict[str, Any]) -> None:
    atomic_write_text(path, json.dumps(data, indent=2) + "\n")


def read_json(path: Path | None) -> dict[str, Any]:
    if path is None or not path.exists():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def sha256_file(path: Path | None) -> str | None:
    if path is None or not path.exists():
        return None
    return hashlib.sha256(path.read_bytes()).hexdigest()


def resolve_json_path(value: str | None) -> Path | None:
    if not value:
        return None
    path = Path(value).resolve()
    return path if path.suffix == ".json" else path.with_suffix(".json")


def stable_manifest_timestamp(manifest: dict[str, Any]) -> str:
    for key in (
        "generated_at_utc",
        "timestamp_utc",
        "approval_timestamp_utc",
        "approval_timestamp",
        "last_seen_timestamp",
    ):
        value = str(manifest.get(key, "")).strip()
        if value:
            return value
    return ""


def timestamp_utc_from_datetime(value: datetime) -> str:
    return value.astimezone(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def parse_timestamp_utc(value: str | None) -> datetime | None:
    text = str(value or "").strip()
    if not text:
        return None
    try:
        return datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)
    except ValueError:
        return None


def age_hours(value: str | None, *, now: datetime | None = None) -> float | None:
    parsed = parse_timestamp_utc(value)
    if parsed is None:
        return None
    reference = now or datetime.now(timezone.utc)
    return max(0.0, (reference - parsed).total_seconds() / 3600.0)


def age_days(value: str | None, *, now: datetime | None = None) -> float | None:
    observed = age_hours(value, now=now)
    return None if observed is None else round(observed / 24.0, 2)


def add_days_utc(value: str | None, days: float) -> str | None:
    parsed = parse_timestamp_utc(value)
    if parsed is None:
        return None
    return timestamp_utc_from_datetime(parsed + timedelta(days=days))


def resolve_governance_now(current_time_override: str | None, advance_days: float = 0.0) -> datetime:
    base = parse_timestamp_utc(current_time_override)
    if base is None:
        base = datetime.now(timezone.utc)
    return base + timedelta(days=float(advance_days or 0.0))


def watch_confidence_rank(value: str) -> int:
    return WATCH_CONFIDENCE_RANK.get(str(value).strip().upper(), -1)


def normalize_known_env_governance_policy(
    raw_policy: dict[str, Any] | None,
    *,
    source_path: Path | None = None,
    phase: str = "phase40",
) -> dict[str, Any]:
    policy = json.loads(json.dumps(DEFAULT_KNOWN_ENV_GOVERNANCE_POLICY))
    if raw_policy:
        for key, value in raw_policy.items():
            if key == "roles" and isinstance(value, dict):
                policy_roles = dict(policy.get("roles", {}))
                for role_name, role_payload in value.items():
                    if isinstance(role_payload, dict):
                        merged = dict(policy_roles.get(role_name, {}))
                        merged.update(role_payload)
                        policy_roles[role_name] = merged
                    else:
                        policy_roles[role_name] = role_payload
                policy["roles"] = policy_roles
            elif key == "counts_as_approved_known_env_requirements" and isinstance(value, dict):
                merged = dict(policy.get("counts_as_approved_known_env_requirements", {}))
                merged.update(value)
                policy[key] = merged
            else:
                policy[key] = value
    policy["phase"] = str(policy.get("phase") or phase)
    policy["policy_id"] = str(policy.get("policy_id") or f"{policy['phase']}-known-env-governance")
    policy["min_watch_confidence_for_freshen"] = str(
        policy.get("min_watch_confidence_for_freshen", "HIGH")
    ).strip().upper()
    policy["generated_at_utc"] = runtime_gate.timestamp_utc_now()
    policy["source_path"] = None if source_path is None else str(source_path)
    policy["policy_hash"] = sha256_text(json.dumps(policy, sort_keys=True))
    return policy


def load_known_env_governance_policy(path: Path | None, *, phase: str = "phase40") -> dict[str, Any]:
    if path is None or not path.exists():
        return normalize_known_env_governance_policy({}, source_path=path, phase=phase)
    return normalize_known_env_governance_policy(read_json(path), source_path=path, phase=phase)


def normalize_current_env_governance_policy(
    raw_policy: dict[str, Any] | None,
    *,
    source_path: Path | None = None,
    phase: str = "phase42",
) -> dict[str, Any]:
    policy = json.loads(json.dumps(DEFAULT_CURRENT_ENV_GOVERNANCE_POLICY))
    if raw_policy:
        for key, value in raw_policy.items():
            if key == "current_env_roles" and isinstance(value, dict):
                merged_roles = dict(policy.get("current_env_roles", {}))
                for role_name, role_payload in value.items():
                    if isinstance(role_payload, dict):
                        merged = dict(merged_roles.get(role_name, {}))
                        merged.update(role_payload)
                        merged_roles[role_name] = merged
                    else:
                        merged_roles[role_name] = role_payload
                policy[key] = merged_roles
            else:
                policy[key] = value
    policy["phase"] = str(policy.get("phase") or phase)
    phase_number = int("".join(ch for ch in policy["phase"] if ch.isdigit()) or "0")
    default_policy_id = (
        f"{policy['phase']}-current-env-guardrail"
        if phase_number >= 42
        else f"{policy['phase']}-current-env-governance"
    )
    policy["policy_id"] = str(policy.get("policy_id") or default_policy_id)
    policy["stable_soft_overrun_trigger_count"] = int(
        policy.get(
            "stable_soft_overrun_trigger_count",
            policy.get("reprofile_candidate_threshold", 5),
        )
    )
    policy["hard_over_budget_trigger_count"] = int(
        policy.get(
            "hard_over_budget_trigger_count",
            policy.get("watch_escalate_hard_breach_count", 1),
        )
    )
    policy["min_watch_confidence_for_reproposal"] = str(
        policy.get(
            "min_watch_confidence_for_reproposal",
            policy.get("min_watch_confidence_for_reprofile", "HIGH"),
        )
    ).strip().upper()
    policy["min_watch_confidence_for_reprofile"] = policy["min_watch_confidence_for_reproposal"]
    policy["max_bounded_jitter_percent_for_reproposal"] = float(
        policy.get(
            "max_bounded_jitter_percent_for_reproposal",
            policy.get("bounded_jitter_percent", 15.0),
        )
        or 0.0
    )
    policy["bounded_jitter_percent"] = policy["max_bounded_jitter_percent_for_reproposal"]
    policy["current_env_execution_classes"] = list(
        policy.get("current_env_execution_classes")
        or DEFAULT_CURRENT_ENV_GOVERNANCE_POLICY["current_env_execution_classes"]
    )
    policy["generated_at_utc"] = runtime_gate.timestamp_utc_now()
    policy["source_path"] = None if source_path is None else str(source_path)
    policy["policy_hash"] = sha256_text(json.dumps(policy, sort_keys=True))
    return policy


def load_current_env_governance_policy(path: Path | None, *, phase: str = "phase41") -> dict[str, Any]:
    if path is None or not path.exists():
        return normalize_current_env_governance_policy({}, source_path=path, phase=phase)
    return normalize_current_env_governance_policy(read_json(path), source_path=path, phase=phase)


def normalize_current_env_action_retry_policy(
    raw_policy: dict[str, Any] | None,
    *,
    source_path: Path | None = None,
    phase: str = "phase45",
) -> dict[str, Any]:
    policy = json.loads(json.dumps(DEFAULT_CURRENT_ENV_ACTION_RETRY_POLICY))
    if raw_policy:
        policy.update(raw_policy)
    policy["phase"] = str(policy.get("phase") or phase)
    policy["policy_id"] = str(policy.get("policy_id") or f"{policy['phase']}-current-env-action-retry")
    policy["max_retry_count"] = int(policy.get("max_retry_count", 2))
    policy["retry_backoff_minutes"] = int(policy.get("retry_backoff_minutes", 30))
    policy["failed_action_escalation_threshold"] = int(policy.get("failed_action_escalation_threshold", 2))
    policy["retryable_failure_classes"] = [str(item) for item in policy.get("retryable_failure_classes", [])]
    policy["non_retryable_failure_classes"] = [str(item) for item in policy.get("non_retryable_failure_classes", [])]
    policy["source_path"] = None if source_path is None else str(source_path)
    policy["generated_at_utc"] = runtime_gate.timestamp_utc_now()
    policy["policy_hash"] = sha256_text(json.dumps(policy, sort_keys=True))
    return policy


def load_current_env_action_retry_policy(path: Path | None, *, phase: str = "phase45") -> dict[str, Any]:
    if path is None or not path.exists():
        return normalize_current_env_action_retry_policy({}, source_path=path, phase=phase)
    return normalize_current_env_action_retry_policy(read_json(path), source_path=path, phase=phase)


def current_env_state_rank(value: str) -> int:
    return CURRENT_ENV_STATE_ORDER.get(str(value).strip(), -1)


def current_env_watch_recommendation(state: str) -> str:
    if state == CURRENT_ENV_STATE_CLEAR:
        return "NO_ACTION"
    if state == CURRENT_ENV_STATE_APPROVAL_GRACE:
        return "NO_ACTION"
    if state in {CURRENT_ENV_STATE_MONITORING_DUE_SOON, CURRENT_ENV_STATE_MONITORING_DUE}:
        return "CONTINUE_MONITORING"
    if state in {CURRENT_ENV_STATE_WATCH, CURRENT_ENV_STATE_WATCH_STABLE}:
        return "WATCH_RUNTIME"
    if state == CURRENT_ENV_STATE_WATCH_ESCALATE:
        return "INVESTIGATE_RUNTIME_DRIFT"
    if state == CURRENT_ENV_STATE_REPROFILE_CANDIDATE:
        return "PROPOSE_BUDGET_REPROFILE"
    if state == CURRENT_ENV_STATE_REPROFILE_REQUIRED:
        return "REPROFILE_CURRENT_ENV_RUNTIME"
    return "FAIL"


def due_state_for_deadline(now: datetime, due_at: str | None, due_soon_window_days: float) -> str:
    due_dt = parse_timestamp_utc(due_at)
    if due_dt is None:
        return CURRENT_ENV_DUE_NOT_DUE
    due_soon_dt = due_dt - timedelta(days=float(due_soon_window_days or 0.0))
    if now < due_soon_dt:
        return CURRENT_ENV_DUE_NOT_DUE
    if now < due_dt:
        return CURRENT_ENV_DUE_SOON
    if now == due_dt:
        return CURRENT_ENV_DUE
    return CURRENT_ENV_DUE_OVERDUE


def overdue_days_for_deadline(now: datetime, due_at: str | None) -> float:
    due_dt = parse_timestamp_utc(due_at)
    if due_dt is None or now <= due_dt:
        return 0.0
    return round((now - due_dt).total_seconds() / 86400.0, 2)


def current_env_due_action(
    *,
    monitoring_due_state: str,
    reproposal_due_state: str,
    current_state: str,
    approval_grace_active: bool,
) -> str:
    if current_state == CURRENT_ENV_STATE_FAIL:
        return "FAIL"
    if current_state == CURRENT_ENV_STATE_REPROFILE_REQUIRED:
        return "RUN_CURRENT_ENV_REPROPOSAL_GATE"
    if current_state == CURRENT_ENV_STATE_REPROFILE_CANDIDATE:
        return "RUN_CURRENT_ENV_REPROPOSAL_GATE"
    if reproposal_due_state in {CURRENT_ENV_DUE, CURRENT_ENV_DUE_OVERDUE}:
        return "RUN_CURRENT_ENV_REPROPOSAL_GATE"
    if monitoring_due_state in {CURRENT_ENV_DUE, CURRENT_ENV_DUE_OVERDUE}:
        return "RUN_CURRENT_ENV_WATCH_CAMPAIGN"
    if monitoring_due_state == CURRENT_ENV_DUE_SOON or reproposal_due_state == CURRENT_ENV_DUE_SOON:
        return "PREPARE_MONITORING"
    if approval_grace_active:
        return "NO_ACTION"
    return "NO_ACTION"


def current_env_due_command(action: str, repeat_count: int = 5) -> str:
    if action == "RUN_CURRENT_ENV_WATCH_CAMPAIGN":
        return f"./raw_engine_tests --case runtime_watch_campaign --execution-class release_full --repeat {repeat_count}"
    if action == "RUN_CURRENT_ENV_REPROPOSAL_GATE":
        return "./raw_engine_tests --case runtime_current_env_reproposal_trigger_gate_smoke"
    if action == "PREPARE_MONITORING":
        return f"./raw_engine_tests --case runtime_watch_campaign --execution-class release_full --repeat {repeat_count}"
    return ""


def current_env_focus_entry(entries: list[dict[str, Any]]) -> dict[str, Any]:
    if not entries:
        return {}
    release_entries = [
        dict(entry)
        for entry in entries
        if str(entry.get("execution_class", "")).strip() == "release_full"
    ]
    if release_entries:
        return choose_current_env_focus_entry(release_entries)
    return choose_current_env_focus_entry(entries)


def current_env_entry_for_execution_class(
    execution_class: str,
    *payloads: dict[str, Any],
) -> dict[str, Any]:
    execution_class = str(execution_class or "").strip()
    candidates: list[dict[str, Any]] = []
    for payload in payloads:
        if not isinstance(payload, dict):
            continue
        for entry in payload.get("entries", []):
            if not isinstance(entry, dict):
                continue
            if execution_class and str(entry.get("execution_class", "")).strip() != execution_class:
                continue
            candidates.append(dict(entry))
    if candidates:
        return choose_current_env_focus_entry(candidates)
    return current_env_focus_entry([
        dict(entry)
        for payload in payloads
        if isinstance(payload, dict)
        for entry in payload.get("entries", [])
        if isinstance(entry, dict)
    ])


def numeric_metric(entry: dict[str, Any], *keys: str, default: float = 0.0) -> float:
    for key in keys:
        value = entry.get(key)
        if value is None or value == "":
            continue
        try:
            return float(value)
        except (TypeError, ValueError):
            continue
    return default


def wall_time_samples_from_entry(entry: dict[str, Any]) -> list[float]:
    values: list[float] = []
    for sample in entry.get("recent_samples", []):
        if not isinstance(sample, dict):
            continue
        try:
            values.append(float(sample.get("wall_time_sec", 0.0)))
        except (TypeError, ValueError):
            continue
    for key in (
        "min_wall_time_sec",
        "median_wall_time_sec",
        "max_wall_time_sec",
        "p90_wall_time_sec",
        "p95_wall_time_sec",
        "current_wall_time_sec",
        "rolling_median_wall_time_sec",
    ):
        value = entry.get(key)
        if value is None or value == "":
            continue
        try:
            values.append(float(value))
        except (TypeError, ValueError):
            continue
    return values


def percentile_from_values(values: list[float], percentile: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    index = max(0, min(len(ordered) - 1, int(round((percentile / 100.0) * (len(ordered) - 1)))))
    return round(ordered[index], 3)


def runtime_budget_approval_metadata(runtime_budget_baseline: dict[str, Any] | None) -> dict[str, Any]:
    runtime_budget_baseline = runtime_budget_baseline or {}
    approval_metadata = runtime_budget_baseline.get("approval_metadata", {})
    if isinstance(approval_metadata, dict) and approval_metadata:
        return dict(approval_metadata)
    return {
        "approval_timestamp_utc": runtime_budget_baseline.get("approval_timestamp_utc"),
        "previous_active_budget_profile_id": runtime_budget_baseline.get("previous_active_budget_profile_id"),
        "new_active_budget_profile_id": runtime_budget_baseline.get("profile_id"),
        "budget_tag": runtime_budget_baseline.get("budget_tag"),
    }


def current_env_guardrail_deadlines(
    policy: dict[str, Any],
    approval_timestamp: str | None,
) -> dict[str, Any]:
    grace_until = add_days_utc(approval_timestamp, float(policy.get("post_approval_grace_days", 0)))
    watch_due_at = add_days_utc(approval_timestamp, float(policy.get("watch_due_after_days", 0)))
    due_soon_window_days = float(policy.get("watch_due_soon_window_days", 0))
    due_soon_at = None
    if watch_due_at is not None:
        due_soon_at = add_days_utc(watch_due_at, -due_soon_window_days)
    reproposal_due_at = add_days_utc(
        approval_timestamp,
        max(
            float(policy.get("reproposal_candidate_after_days", 0)),
            float(policy.get("reproposal_cooldown_days", 0)),
        ),
    )
    return {
        "approval_timestamp_utc": approval_timestamp,
        "post_approval_grace_until": grace_until,
        "next_monitoring_due_at": watch_due_at,
        "monitoring_due_soon_at": due_soon_at,
        "next_reproposal_due_at": reproposal_due_at,
    }


def current_env_guardrail_state(
    *,
    now: datetime,
    policy: dict[str, Any],
    watch_payload: dict[str, Any],
    watch_history: dict[str, Any],
    runtime_budget_baseline: dict[str, Any],
) -> tuple[str, list[str], dict[str, Any]]:
    focus_entry = current_env_focus_entry(list(watch_history.get("entries", [])) or list(watch_payload.get("entries", [])))
    approval_metadata = runtime_budget_approval_metadata(runtime_budget_baseline)
    approval_timestamp = str(
        approval_metadata.get("approval_timestamp_utc")
        or runtime_budget_baseline.get("approval_timestamp_utc")
        or ""
    ).strip()
    deadlines = current_env_guardrail_deadlines(policy, approval_timestamp or None)
    hard_over_budget_count = int(focus_entry.get("hard_over_budget_count", watch_payload.get("hard_over_budget_count", 0)))
    stable_overrun_count = int(focus_entry.get("stable_overrun_count", watch_payload.get("stable_overrun_count", 0)))
    real_sample_count = int(focus_entry.get("real_sample_count", watch_payload.get("real_sample_count", 0)))
    watch_status = str(focus_entry.get("watch_status", watch_payload.get("watch_status", runtime_gate.WATCH_CLEAR))).strip()
    rationale: list[str] = []

    if watch_status == runtime_gate.WATCH_FAIL:
        rationale.append("runtime watch recorded a fail condition")
        return CURRENT_ENV_STATE_FAIL, rationale, deadlines
    if hard_over_budget_count >= int(policy.get("hard_over_budget_trigger_count", 1)):
        rationale.append("hard budget trigger count exceeded the post-approval guardrail")
        return CURRENT_ENV_STATE_REPROFILE_REQUIRED, rationale, deadlines

    grace_until_dt = parse_timestamp_utc(deadlines.get("post_approval_grace_until"))
    due_soon_dt = parse_timestamp_utc(deadlines.get("monitoring_due_soon_at"))
    due_dt = parse_timestamp_utc(deadlines.get("next_monitoring_due_at"))
    reproposal_due_dt = parse_timestamp_utc(deadlines.get("next_reproposal_due_at"))

    if grace_until_dt is not None and now < grace_until_dt:
        rationale.append("budget approval grace window is still active")
        return CURRENT_ENV_STATE_APPROVAL_GRACE, rationale, deadlines
    if due_soon_dt is not None and due_dt is not None and due_soon_dt <= now < due_dt:
        rationale.append("current env is approaching the next monitoring due window")
        return CURRENT_ENV_STATE_MONITORING_DUE_SOON, rationale, deadlines
    if reproposal_due_dt is not None and now >= reproposal_due_dt and stable_overrun_count > 0:
        required_confidence = str(policy.get("min_watch_confidence_for_reproposal", "HIGH")).strip().upper()
        confidence = str(focus_entry.get("watch_confidence", watch_payload.get("watch_confidence", "LOW"))).strip().upper()
        jitter_ok = float(focus_entry.get("jitter_estimate_percent", 0.0) or 0.0) <= float(
            policy.get("max_bounded_jitter_percent_for_reproposal", 15.0)
        )
        if (
            stable_overrun_count >= int(policy.get("stable_soft_overrun_trigger_count", 5))
            and real_sample_count >= int(policy.get("min_real_samples_for_reproposal", 5))
            and watch_confidence_rank(confidence) >= watch_confidence_rank(required_confidence)
            and jitter_ok
        ):
            rationale.append("stable release_full overrun reached the scheduled reproposal due point")
            return CURRENT_ENV_STATE_REPROFILE_CANDIDATE, rationale, deadlines
    if due_dt is not None and now >= due_dt and watch_status in {runtime_gate.WATCH_CLEAR, "", runtime_gate.WATCH_WATCH}:
        rationale.append("current env reached the scheduled monitoring due point")
        return CURRENT_ENV_STATE_MONITORING_DUE, rationale, deadlines
    if watch_status == runtime_gate.WATCH_ESCALATE:
        rationale.append("runtime watch is escalating within the post-approval guardrail")
        return CURRENT_ENV_STATE_WATCH_ESCALATE, rationale, deadlines
    if watch_status == runtime_gate.WATCH_STABLE:
        required_confidence = str(policy.get("min_watch_confidence_for_reproposal", "HIGH")).strip().upper()
        confidence = str(focus_entry.get("watch_confidence", watch_payload.get("watch_confidence", "LOW"))).strip().upper()
        jitter_ok = float(focus_entry.get("jitter_estimate_percent", 0.0) or 0.0) <= float(
            policy.get("max_bounded_jitter_percent_for_reproposal", 15.0)
        )
        if (
            reproposal_due_dt is not None
            and now >= reproposal_due_dt
            and stable_overrun_count >= int(policy.get("stable_soft_overrun_trigger_count", 5))
            and real_sample_count >= int(policy.get("min_real_samples_for_reproposal", 5))
            and watch_confidence_rank(confidence) >= watch_confidence_rank(required_confidence)
            and jitter_ok
        ):
            rationale.append("stable release_full overrun exceeded the reproposal trigger after cooldown")
            return CURRENT_ENV_STATE_REPROFILE_CANDIDATE, rationale, deadlines
        rationale.append("stable runtime watch provenance is retained after approval")
        return CURRENT_ENV_STATE_WATCH_STABLE, rationale, deadlines
    if watch_status == runtime_gate.WATCH_WATCH:
        rationale.append("runtime watch remains active for the current environment")
        return CURRENT_ENV_STATE_WATCH, rationale, deadlines
    if stable_overrun_count > 0 and real_sample_count >= int(policy.get("post_approval_min_real_samples_before_rewatch", 0)):
        rationale.append("stable soft-budget overrun evidence exists but remains inside the guardrail window")
        return CURRENT_ENV_STATE_WATCH_STABLE, rationale, deadlines
    rationale.append("current environment remains inside the post-approval guardrail envelope")
    return CURRENT_ENV_STATE_CLEAR, rationale, deadlines


def current_env_role_policy(policy: dict[str, Any], role: str) -> dict[str, Any]:
    roles = dict(policy.get("current_env_roles", {}))
    return dict(roles.get(role, roles.get("operator", {})))


def build_current_env_watch_entry(
    budget_entry: dict[str, Any],
    refresh_entry: dict[str, Any],
    policy: dict[str, Any],
) -> dict[str, Any]:
    execution_class = str(budget_entry.get("execution_class", "")).strip()
    role = str(budget_entry.get("execution_role", runtime_gate.default_runtime_role(execution_class)))
    watch_status = str(
        budget_entry.get("watch_status")
        or refresh_entry.get("watch_status")
        or runtime_gate.WATCH_CLEAR
    ).strip()
    watch_reason = str(
        budget_entry.get("watch_reason")
        or refresh_entry.get("watch_reason")
        or runtime_gate.runtime_watch_reason(
            watch_status,
            execution_class,
            role,
            int(budget_entry.get("sample_count", 0)),
            int(budget_entry.get("soft_over_budget_count", 0)),
            int(budget_entry.get("hard_over_budget_count", 0)),
            str(budget_entry.get("trend_direction", runtime_gate.TREND_INSUFFICIENT)),
        )
    ).strip()
    sample_count = int(budget_entry.get("sample_count", 0))
    real_sample_count = int(budget_entry.get("real_sample_count", 0))
    stable_overrun_count = int(budget_entry.get("stable_overrun_count", 0))
    hard_over_budget_count = int(budget_entry.get("hard_over_budget_count", 0))
    over_budget_ratio = float(budget_entry.get("over_budget_ratio", 0.0) or 0.0)
    trend_direction = str(
        budget_entry.get("trend_direction")
        or refresh_entry.get("trend_direction")
        or runtime_gate.TREND_INSUFFICIENT
    )
    watch_confidence = str(
        budget_entry.get("watch_confidence")
        or refresh_entry.get("watch_confidence")
        or "LOW"
    ).strip().upper()
    proposal_candidate = bool(budget_entry.get("proposal_candidate", False))
    required_confidence = str(policy.get("min_watch_confidence_for_reprofile", "HIGH")).strip().upper()
    max_hard_breaches = int(policy.get("max_hard_breach_count_for_reproposal", 0))
    role_policy = current_env_role_policy(policy, role)

    current_env_state = CURRENT_ENV_STATE_CLEAR
    if watch_status == runtime_gate.WATCH_FAIL or str(budget_entry.get("current_status", "")) == runtime_gate.STATUS_FAIL:
        current_env_state = CURRENT_ENV_STATE_FAIL
    elif hard_over_budget_count > max_hard_breaches or watch_status == runtime_gate.WATCH_REBASELINE_REQUIRED:
        current_env_state = CURRENT_ENV_STATE_REPROFILE_REQUIRED
    elif watch_status == runtime_gate.WATCH_ESCALATE:
        current_env_state = CURRENT_ENV_STATE_WATCH_ESCALATE
    elif (
        proposal_candidate
        and role == runtime_gate.ROLE_PRODUCTION_CRITICAL
        and watch_confidence_rank(watch_confidence) >= watch_confidence_rank(required_confidence)
        and real_sample_count >= int(policy.get("min_real_samples_for_reproposal", 5))
        and stable_overrun_count >= int(policy.get("reprofile_candidate_threshold", 5))
        and bool(role_policy.get("prefer_reprofile_candidate", True))
    ):
        current_env_state = CURRENT_ENV_STATE_REPROFILE_CANDIDATE
    elif watch_status == runtime_gate.WATCH_STABLE:
        current_env_state = CURRENT_ENV_STATE_WATCH_STABLE
    elif watch_status == runtime_gate.WATCH_WATCH:
        current_env_state = CURRENT_ENV_STATE_WATCH

    rationale = [watch_reason] if watch_reason else []
    if current_env_state == CURRENT_ENV_STATE_REPROFILE_CANDIDATE:
        rationale.append("stable soft-budget overrun is sufficiently backed by same-fingerprint real evidence")
    elif current_env_state == CURRENT_ENV_STATE_REPROFILE_REQUIRED:
        rationale.append("current environment exceeded the safe budget watch envelope and requires budget reprofile or investigation")
    elif current_env_state == CURRENT_ENV_STATE_FAIL:
        rationale.append("current environment crossed the hard budget or fail threshold")

    entry = {
        "execution_class": execution_class,
        "role": role,
        "current_env_state": current_env_state,
        "watch_status": watch_status,
        "watch_reason": watch_reason,
        "sample_count": sample_count,
        "real_sample_count": real_sample_count,
        "stable_overrun_count": stable_overrun_count,
        "hard_over_budget_count": hard_over_budget_count,
        "over_budget_ratio": round(over_budget_ratio, 3),
        "trend_direction": trend_direction,
        "watch_confidence": watch_confidence,
        "reproposal_candidate": current_env_state == CURRENT_ENV_STATE_REPROFILE_CANDIDATE,
        "reproposal_required": current_env_state in {CURRENT_ENV_STATE_REPROFILE_REQUIRED, CURRENT_ENV_STATE_FAIL},
        "selected_budget_profile_id": None,
        "selected_runtime_baseline_id": None,
        "recommended_action": current_env_watch_recommendation(current_env_state),
        "rationale": rationale,
    }
    if "delta_vs_selected_baseline_percent" in budget_entry:
        entry["delta_vs_selected_budget_profile_percent"] = budget_entry.get("delta_vs_selected_baseline_percent")
    elif "delta_percent" in refresh_entry:
        entry["delta_vs_selected_budget_profile_percent"] = refresh_entry.get("delta_percent")
    return entry


def choose_current_env_focus_entry(entries: list[dict[str, Any]]) -> dict[str, Any]:
    if not entries:
        return {}
    return max(
        entries,
        key=lambda item: (
            current_env_state_rank(str(item.get("current_env_state", ""))),
            runtime_gate.runtime_watch_status_rank(str(item.get("watch_status", runtime_gate.WATCH_CLEAR))),
            int(item.get("real_sample_count", 0)),
            int(item.get("sample_count", 0)),
        ),
    )


def build_current_env_watch_manifest(
    *,
    phase: str,
    manifest_role: str,
    runtime_refresh: dict[str, Any],
    runtime_refresh_path: Path | None,
    runtime_budget_current: dict[str, Any],
    runtime_budget_current_path: Path | None,
    runtime_watch_manifest: dict[str, Any],
    runtime_watch_manifest_path: Path | None,
    governance_policy: dict[str, Any],
    runtime_budget_proposal: dict[str, Any] | None = None,
    runtime_budget_proposal_path: Path | None = None,
    runtime_budget_proposal_gate: dict[str, Any] | None = None,
    runtime_budget_proposal_gate_path: Path | None = None,
    runtime_budget_baseline: dict[str, Any] | None = None,
) -> dict[str, Any]:
    runtime_budget_proposal = runtime_budget_proposal or {}
    runtime_budget_proposal_gate = runtime_budget_proposal_gate or {}
    runtime_budget_baseline = runtime_budget_baseline or {}
    refresh_entries = {
        str(entry.get("execution_class", "")): dict(entry)
        for entry in runtime_watch_manifest.get("entries", [])
        if isinstance(entry, dict)
    }
    entries = [
        build_current_env_watch_entry(
            dict(budget_entry),
            dict(refresh_entries.get(str(budget_entry.get("execution_class", "")), {})),
            governance_policy,
        )
        for budget_entry in runtime_budget_current.get("entries", [])
        if isinstance(budget_entry, dict)
    ]
    selected_runtime_baseline_id = runtime_refresh.get("selected_baseline_id")
    selected_runtime_baseline_tag = runtime_refresh.get("selected_baseline_tag")
    selected_budget_profile_id = (
        runtime_budget_baseline.get("profile_id")
        or runtime_budget_current.get("source_runtime_budget_profile_id")
    )
    selected_budget_profile_tag = runtime_budget_baseline.get("budget_tag")
    for entry in entries:
        entry["selected_runtime_baseline_id"] = selected_runtime_baseline_id
        entry["selected_budget_profile_id"] = selected_budget_profile_id
    focus_entry = choose_current_env_focus_entry(entries)
    overall_state = str(focus_entry.get("current_env_state", CURRENT_ENV_STATE_CLEAR))
    payload = {
        "manifest_version": "runtime_current_env_watch_v1",
        "manifest_role": manifest_role,
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "governance_policy_id": governance_policy.get("policy_id"),
        "runtime_refresh_path": None if runtime_refresh_path is None else str(runtime_refresh_path),
        "runtime_refresh_hash": sha256_file(runtime_refresh_path),
        "runtime_budget_current_path": None if runtime_budget_current_path is None else str(runtime_budget_current_path),
        "runtime_budget_current_hash": sha256_file(runtime_budget_current_path),
        "runtime_watch_manifest_path": None if runtime_watch_manifest_path is None else str(runtime_watch_manifest_path),
        "runtime_watch_manifest_hash": sha256_file(runtime_watch_manifest_path),
        "runtime_budget_proposal_path": None if runtime_budget_proposal_path is None else str(runtime_budget_proposal_path),
        "runtime_budget_proposal_hash": sha256_file(runtime_budget_proposal_path),
        "runtime_budget_proposal_gate_path": None if runtime_budget_proposal_gate_path is None else str(runtime_budget_proposal_gate_path),
        "runtime_budget_proposal_gate_hash": sha256_file(runtime_budget_proposal_gate_path),
        "current_env_state": overall_state,
        "watch_status": focus_entry.get("watch_status", runtime_gate.WATCH_CLEAR),
        "watch_reason": focus_entry.get("watch_reason", ""),
        "watch_confidence": focus_entry.get("watch_confidence", "LOW"),
        "reproposal_candidate": any(bool(entry.get("reproposal_candidate", False)) for entry in entries),
        "reproposal_required": any(bool(entry.get("reproposal_required", False)) for entry in entries),
        "reproposal_needed": bool(
            runtime_budget_proposal_gate.get("budget_reproposal_needed", runtime_budget_proposal_gate.get("budget_proposal_needed"))
            if runtime_budget_proposal_gate
            else runtime_budget_proposal.get("budget_reproposal_needed", runtime_budget_proposal.get("budget_proposal_needed"))
            if runtime_budget_proposal
            else runtime_budget_current.get("proposal_needed", False)
        ),
        "reproposal_gate_verdict": runtime_budget_proposal_gate.get("reproposal_gate_verdict")
        or runtime_budget_proposal_gate.get("proposal_gate_verdict"),
        "reproposal_confidence": runtime_budget_proposal_gate.get("reproposal_confidence")
        or runtime_budget_proposal_gate.get("proposal_confidence"),
        "selected_budget_profile_id": selected_budget_profile_id,
        "selected_budget_profile_tag": selected_budget_profile_tag,
        "selected_runtime_baseline_id": selected_runtime_baseline_id,
        "selected_runtime_baseline_tag": selected_runtime_baseline_tag,
        "budget_verdict": runtime_budget_current.get("budget_verdict"),
        "comparability_verdict": runtime_refresh.get("comparability_verdict"),
        "freshness_verdict": runtime_refresh.get("freshness_verdict"),
        "current_verdict": runtime_refresh.get("current_verdict"),
        "entry_count": len(entries),
        "production_critical_entry_count": sum(
            1 for entry in entries if str(entry.get("role", "")) == runtime_gate.ROLE_PRODUCTION_CRITICAL
        ),
        "stable_overrun_count": sum(int(entry.get("stable_overrun_count", 0)) for entry in entries),
        "hard_over_budget_count": sum(int(entry.get("hard_over_budget_count", 0)) for entry in entries),
        "real_sample_count": sum(int(entry.get("real_sample_count", 0)) for entry in entries),
        "sample_count": sum(int(entry.get("sample_count", 0)) for entry in entries),
        "trend_direction": focus_entry.get("trend_direction", runtime_gate.TREND_INSUFFICIENT),
        "overall_recommended_action": current_env_watch_recommendation(overall_state),
        "rationale": list(focus_entry.get("rationale", [])),
        "entries": entries,
    }
    payload["current_env_watch_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_watch_history(
    *,
    phase: str,
    runtime_watch_history: dict[str, Any],
    runtime_watch_history_path: Path | None,
    runtime_budget_current: dict[str, Any],
    runtime_budget_current_path: Path | None,
    governance_policy: dict[str, Any],
    runtime_budget_proposal_gate: dict[str, Any] | None = None,
    runtime_budget_proposal_gate_path: Path | None = None,
    runtime_budget_baseline: dict[str, Any] | None = None,
    runtime_budget_baseline_path: Path | None = None,
) -> dict[str, Any]:
    runtime_budget_proposal_gate = runtime_budget_proposal_gate or {}
    runtime_budget_baseline = runtime_budget_baseline or {}
    history_entries: dict[str, dict[str, Any]] = {}
    target_fingerprint = str(runtime_budget_current.get("runtime_fingerprint_key", "")).strip()
    for bucket in runtime_watch_history.get("fingerprints", []):
        if not isinstance(bucket, dict):
            continue
        bucket_fingerprint = str(bucket.get("runtime_fingerprint_key", "")).strip()
        if target_fingerprint and bucket_fingerprint and bucket_fingerprint != target_fingerprint:
            continue
        for history_entry in bucket.get("execution_classes", []):
            if not isinstance(history_entry, dict):
                continue
            execution_class = str(history_entry.get("execution_class", "")).strip()
            if execution_class:
                history_entries[execution_class] = dict(history_entry)
        if history_entries:
            break
    entries: list[dict[str, Any]] = []
    for budget_entry in runtime_budget_current.get("entries", []):
        if not isinstance(budget_entry, dict):
            continue
        execution_class = str(budget_entry.get("execution_class", "")).strip()
        if not execution_class:
            continue
        history_entry = dict(history_entries.get(execution_class, {}))
        merged_entry = build_current_env_watch_entry(dict(budget_entry), history_entry, governance_policy)
        merged_entry.update(
            {
                "min_wall_time_sec": history_entry.get("min_wall_time_sec"),
                "median_wall_time_sec": history_entry.get("rolling_median_wall_time_sec")
                or budget_entry.get("rolling_median_wall_time_sec"),
                "max_wall_time_sec": history_entry.get("max_wall_time_sec"),
                "p90_wall_time_sec": history_entry.get("rolling_p90_wall_time_sec")
                or budget_entry.get("rolling_p90_wall_time_sec"),
                "p95_wall_time_sec": history_entry.get("rolling_p95_wall_time_sec")
                or budget_entry.get("rolling_p95_wall_time_sec"),
                "recent_samples": list(history_entry.get("samples", []))[-5:],
            }
        )
        entries.append(merged_entry)
    focus_entry = choose_current_env_focus_entry(entries)
    approval_metadata = runtime_budget_approval_metadata(runtime_budget_baseline)
    approval_timestamp = str(
        approval_metadata.get("approval_timestamp_utc")
        or runtime_budget_baseline.get("approval_timestamp_utc")
        or ""
    ).strip()
    deadlines = current_env_guardrail_deadlines(governance_policy, approval_timestamp or None)
    release_focus = current_env_focus_entry(entries)
    runtime_budget_registry_path = resolve_json_path(runtime_budget_baseline.get("runtime_budget_registry_path"))
    runtime_budget_registry = read_json(runtime_budget_registry_path)
    reproposal_count_for_fingerprint = 0
    target_budget_profile_id = str(
        runtime_budget_baseline.get("profile_id")
        or runtime_budget_current.get("source_runtime_budget_profile_id")
        or ""
    ).strip()
    for entry in runtime_budget_registry.get("entries", []):
        if not isinstance(entry, dict):
            continue
        if target_budget_profile_id and str(entry.get("profile_id", "")).strip() == target_budget_profile_id:
            reproposal_count_for_fingerprint += 1
        elif str(entry.get("status", "")).strip() == "retired":
            reproposal_count_for_fingerprint += 1
    payload = {
        "manifest_version": "runtime_current_env_watch_history_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "governance_policy_id": governance_policy.get("policy_id"),
        "runtime_watch_history_path": None if runtime_watch_history_path is None else str(runtime_watch_history_path),
        "runtime_watch_history_hash": sha256_file(runtime_watch_history_path),
        "runtime_budget_current_path": None if runtime_budget_current_path is None else str(runtime_budget_current_path),
        "runtime_budget_current_hash": sha256_file(runtime_budget_current_path),
        "runtime_budget_proposal_gate_path": None if runtime_budget_proposal_gate_path is None else str(runtime_budget_proposal_gate_path),
        "runtime_budget_proposal_gate_hash": sha256_file(runtime_budget_proposal_gate_path),
        "runtime_budget_baseline_path": None if runtime_budget_baseline_path is None else str(runtime_budget_baseline_path),
        "runtime_budget_baseline_hash": sha256_file(runtime_budget_baseline_path),
        "current_env_state": focus_entry.get("current_env_state", CURRENT_ENV_STATE_CLEAR),
        "reproposal_gate_verdict": runtime_budget_proposal_gate.get("reproposal_gate_verdict")
        or runtime_budget_proposal_gate.get("proposal_gate_verdict"),
        "approval_timestamp_utc": approval_timestamp or None,
        "post_approval_grace_until": deadlines.get("post_approval_grace_until"),
        "next_monitoring_due_at": deadlines.get("next_monitoring_due_at"),
        "next_reproposal_due_at": deadlines.get("next_reproposal_due_at"),
        "previous_active_budget_profile_id": approval_metadata.get("previous_active_budget_profile_id"),
        "active_budget_profile_id": approval_metadata.get("new_active_budget_profile_id")
        or runtime_budget_baseline.get("profile_id"),
        "last_release_watch_campaign_at": stable_manifest_timestamp(release_focus)
        or stable_manifest_timestamp(runtime_watch_history),
        "last_reproposal_gate_at": stable_manifest_timestamp(runtime_budget_proposal_gate),
        "reproposal_count_for_fingerprint": reproposal_count_for_fingerprint,
        "cumulative_stable_soft_overrun_count": sum(int(entry.get("stable_overrun_count", 0)) for entry in entries),
        "cumulative_hard_breach_count": sum(int(entry.get("hard_over_budget_count", 0)) for entry in entries),
        "affected_execution_class_count": len(entries),
        "real_sample_count": sum(int(entry.get("real_sample_count", 0)) for entry in entries),
        "sample_count": sum(int(entry.get("sample_count", 0)) for entry in entries),
        "stable_overrun_count": sum(int(entry.get("stable_overrun_count", 0)) for entry in entries),
        "hard_over_budget_count": sum(int(entry.get("hard_over_budget_count", 0)) for entry in entries),
        "transition_count": int(runtime_watch_history.get("transition_count", 0)),
        "recent_transitions": list(runtime_watch_history.get("recent_transitions", []))[-10:],
        "entries": entries,
    }
    payload["current_env_watch_history_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_age_tick(
    *,
    phase: str,
    runtime_current_manifest: dict[str, Any],
    runtime_watch_current: dict[str, Any],
    runtime_watch_refresh: dict[str, Any],
    runtime_watch_history: dict[str, Any],
    runtime_budget_baseline: dict[str, Any],
    guardrail_policy: dict[str, Any],
    current_time_override: str | None = None,
    advance_days: float = 0.0,
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override, advance_days)
    focus_payload = runtime_watch_refresh or runtime_watch_current
    state_before = str(
        focus_payload.get("current_env_state")
        or runtime_watch_history.get("current_env_state")
        or CURRENT_ENV_STATE_CLEAR
    )
    state_after, rationale, deadlines = current_env_guardrail_state(
        now=now,
        policy=guardrail_policy,
        watch_payload=focus_payload,
        watch_history=runtime_watch_history,
        runtime_budget_baseline=runtime_budget_baseline,
    )
    next_due_at = deadlines.get("next_monitoring_due_at")
    next_reproposal_due_at = deadlines.get("next_reproposal_due_at")
    next_due_dt = parse_timestamp_utc(next_due_at)
    next_reproposal_due_dt = parse_timestamp_utc(next_reproposal_due_at)
    overdue_days = 0.0 if next_due_dt is None or now <= next_due_dt else round((now - next_due_dt).total_seconds() / 86400.0, 2)
    reproposal_overdue_days = (
        0.0
        if next_reproposal_due_dt is None or now <= next_reproposal_due_dt
        else round((now - next_reproposal_due_dt).total_seconds() / 86400.0, 2)
    )
    due_soon_count = 1 if state_after == CURRENT_ENV_STATE_MONITORING_DUE_SOON else 0
    monitoring_due_count = 1 if state_after in {CURRENT_ENV_STATE_MONITORING_DUE, CURRENT_ENV_STATE_WATCH, CURRENT_ENV_STATE_WATCH_STABLE} else 0
    release_entry = current_env_focus_entry(
        list(runtime_watch_history.get("entries", []))
        or list(focus_payload.get("entries", []))
    )
    affected_execution_classes = [
        str(entry.get("execution_class", "")).strip()
        for entry in (runtime_watch_history.get("entries", []) or focus_payload.get("entries", []))
        if isinstance(entry, dict) and str(entry.get("execution_class", "")).strip()
    ]
    payload = {
        "manifest_version": "runtime_current_env_age_tick_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "guardrail_policy_id": guardrail_policy.get("policy_id"),
        "current_time_effective_utc": timestamp_utc_from_datetime(now),
        "current_env_state_before": state_before,
        "current_env_state_after": state_after,
        "approval_grace_active": state_after == CURRENT_ENV_STATE_APPROVAL_GRACE,
        "due_soon_count": due_soon_count,
        "monitoring_due_count": monitoring_due_count,
        "reproposal_candidate": state_after == CURRENT_ENV_STATE_REPROFILE_CANDIDATE,
        "reproposal_required": state_after in {CURRENT_ENV_STATE_REPROFILE_REQUIRED, CURRENT_ENV_STATE_FAIL},
        "trigger_reason": rationale,
        "affected_execution_classes": affected_execution_classes,
        "affected_known_env_count": 0,
        "next_due_at": next_due_at,
        "next_reproposal_due_at": next_reproposal_due_at,
        "overdue_days": overdue_days,
        "reproposal_overdue_days": reproposal_overdue_days,
        "stable_soft_overrun_count": int(release_entry.get("stable_overrun_count", focus_payload.get("stable_overrun_count", 0))),
        "hard_over_budget_count": int(release_entry.get("hard_over_budget_count", focus_payload.get("hard_over_budget_count", 0))),
        "real_sample_count": int(release_entry.get("real_sample_count", focus_payload.get("real_sample_count", 0))),
        "sample_count": int(release_entry.get("sample_count", focus_payload.get("sample_count", 0))),
        "selected_budget_profile_id": runtime_budget_baseline.get("profile_id")
        or focus_payload.get("selected_budget_profile_id"),
        "selected_runtime_baseline_id": focus_payload.get("selected_runtime_baseline_id")
        or runtime_current_manifest.get("selected_baseline_id"),
        "watch_status": release_entry.get("watch_status", focus_payload.get("watch_status")),
        "watch_confidence": release_entry.get("watch_confidence", focus_payload.get("watch_confidence")),
    }
    payload["age_tick_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_watch_plan(
    *,
    phase: str,
    runtime_current_manifest: dict[str, Any],
    runtime_watch_history: dict[str, Any],
    guardrail_policy: dict[str, Any],
    current_env_age_tick: dict[str, Any],
    execution_class_filter: str = "",
    current_time_override: str | None = None,
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override)
    filter_values = {
        value.strip()
        for value in str(execution_class_filter or "").split(",")
        if value.strip()
    }
    entry_map = {
        str(entry.get("execution_class", "")).strip(): dict(entry)
        for entry in runtime_watch_history.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("execution_class", "")).strip()
    }
    execution_classes = [
        str(entry.get("execution_class", "")).strip()
        for entry in runtime_current_manifest.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("execution_class", "")).strip()
    ] or list(guardrail_policy.get("current_env_execution_classes", []))
    if filter_values:
        execution_classes = [value for value in execution_classes if value in filter_values]
    entries: list[dict[str, Any]] = []
    overall_state = str(current_env_age_tick.get("current_env_state_after", CURRENT_ENV_STATE_CLEAR))
    next_due_at = str(current_env_age_tick.get("next_due_at") or "").strip()
    next_due_dt = parse_timestamp_utc(next_due_at)
    overall_overdue_days = (
        0.0
        if next_due_dt is None or now <= next_due_dt
        else round((now - next_due_dt).total_seconds() / 86400.0, 2)
    )
    for execution_class in execution_classes:
        history_entry = dict(entry_map.get(execution_class, {}))
        if not history_entry:
            continue
        recommended_repeat_count = 1
        if execution_class == "release_full":
                recommended_repeat_count = 10 if overall_state in {
                CURRENT_ENV_STATE_REPROFILE_CANDIDATE,
                CURRENT_ENV_STATE_REPROFILE_REQUIRED,
                CURRENT_ENV_STATE_FAIL,
            } else 5
        reason = "monitor current env drift guardrail"
        if execution_class == "release_full":
            reason = "repeat same-fingerprint release_full watch to confirm whether drift remains bounded"
        elif execution_class in {"debug_full", "asan_full"}:
            reason = "keep diagnostic guard execution class aligned with current env drift monitoring"
        priority = "LOW"
        if execution_class == "release_full":
            priority = "HIGH" if recommended_repeat_count >= 10 or overall_overdue_days > 0.0 else "MEDIUM"
        entries.append(
            {
                "execution_class": execution_class,
                "current_state": overall_state if execution_class == "release_full" else CURRENT_ENV_STATE_CLEAR,
                "next_check_due_at": next_due_at if execution_class == "release_full" else None,
                "overdue_days": overall_overdue_days if execution_class == "release_full" else 0.0,
                "recommended_repeat_count": recommended_repeat_count,
                "minimum_real_samples": int(guardrail_policy.get("post_approval_min_real_samples_before_rewatch", 0))
                if execution_class == "release_full"
                else 1,
                "target_runner_label": runtime_current_manifest.get("runner_tag") or runtime_current_manifest.get("runner_id"),
                "target_host_label": runtime_current_manifest.get("host_label"),
                "recommended_command": (
                    f"./raw_engine_tests --case runtime_watch_campaign --execution-class {execution_class} --repeat {recommended_repeat_count}"
                ),
                "reason": reason,
                "priority": priority,
            }
        )
    verdict = "EMPTY" if not entries else "PASS" if overall_state in {
        CURRENT_ENV_STATE_CLEAR,
        CURRENT_ENV_STATE_APPROVAL_GRACE,
        CURRENT_ENV_STATE_MONITORING_DUE_SOON,
    } else "ACTION_REQUIRED"
    payload = {
        "manifest_version": "runtime_current_env_watch_plan_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "guardrail_policy_id": guardrail_policy.get("policy_id"),
        "plan_verdict": verdict,
        "entry_count": len(entries),
        "entries": entries,
        "current_env_state": overall_state,
        "next_due_at": next_due_at or None,
        "overall_overdue_days": overall_overdue_days,
    }
    payload["watch_plan_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_reproposal_trigger_gate(
    *,
    phase: str,
    runtime_current_manifest: dict[str, Any],
    runtime_watch_current: dict[str, Any],
    runtime_watch_history: dict[str, Any],
    runtime_budget_baseline: dict[str, Any],
    guardrail_policy: dict[str, Any],
    current_time_override: str | None = None,
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override)
    focus_entry = current_env_focus_entry(
        list(runtime_watch_history.get("entries", []))
        or list(runtime_watch_current.get("entries", []))
    )
    state, state_rationale, deadlines = current_env_guardrail_state(
        now=now,
        policy=guardrail_policy,
        watch_payload=runtime_watch_current,
        watch_history=runtime_watch_history,
        runtime_budget_baseline=runtime_budget_baseline,
    )
    stable_overrun_count = int(focus_entry.get("stable_overrun_count", runtime_watch_current.get("stable_overrun_count", 0)))
    hard_over_budget_count = int(focus_entry.get("hard_over_budget_count", runtime_watch_current.get("hard_over_budget_count", 0)))
    sample_count = int(focus_entry.get("sample_count", runtime_watch_current.get("sample_count", 0)))
    real_sample_count = int(focus_entry.get("real_sample_count", runtime_watch_current.get("real_sample_count", 0)))
    watch_status = str(focus_entry.get("watch_status", runtime_watch_current.get("watch_status", runtime_gate.WATCH_CLEAR))).strip()
    watch_confidence = str(focus_entry.get("watch_confidence", runtime_watch_current.get("watch_confidence", "LOW"))).strip().upper()
    over_budget_ratio = float(focus_entry.get("over_budget_ratio", runtime_watch_current.get("over_budget_ratio", 0.0)) or 0.0)
    jitter_estimate_percent = float(focus_entry.get("jitter_estimate_percent", 0.0) or 0.0)
    bounded_jitter_ok = jitter_estimate_percent <= float(
        guardrail_policy.get("max_bounded_jitter_percent_for_reproposal", 15.0)
    )
    trigger_verdict = "CLEAR"
    reproposal_trigger_needed = False
    if state == CURRENT_ENV_STATE_FAIL:
        trigger_verdict = "FAIL"
        reproposal_trigger_needed = True
    elif state == CURRENT_ENV_STATE_REPROFILE_REQUIRED:
        trigger_verdict = "REQUIRE_REPROFILE"
        reproposal_trigger_needed = True
    elif state == CURRENT_ENV_STATE_REPROFILE_CANDIDATE:
        trigger_verdict = "CANDIDATE"
        reproposal_trigger_needed = True
    elif state in {CURRENT_ENV_STATE_CLEAR, CURRENT_ENV_STATE_APPROVAL_GRACE}:
        trigger_verdict = "CLEAR"
    elif state in {
        CURRENT_ENV_STATE_WATCH,
        CURRENT_ENV_STATE_WATCH_STABLE,
        CURRENT_ENV_STATE_WATCH_ESCALATE,
        CURRENT_ENV_STATE_MONITORING_DUE,
        CURRENT_ENV_STATE_MONITORING_DUE_SOON,
    } or watch_status in {runtime_gate.WATCH_WATCH, runtime_gate.WATCH_STABLE, runtime_gate.WATCH_ESCALATE}:
        trigger_verdict = "WATCH"
    rationale = list(state_rationale)
    if state == CURRENT_ENV_STATE_REPROFILE_CANDIDATE:
        rationale.append("post-approval cooldown has expired and stable release_full overrun remains reproducible")
    elif trigger_verdict == "WATCH":
        rationale.append("current env remains inside monitoring or stable-watch guardrail")
    elif trigger_verdict == "CLEAR":
        rationale.append("current env remains clear inside approval grace or cooldown")
    payload = {
        "manifest_version": "runtime_current_env_reproposal_trigger_gate_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "guardrail_policy_id": guardrail_policy.get("policy_id"),
        "selected_budget_profile_id": runtime_budget_baseline.get("profile_id")
        or runtime_watch_current.get("selected_budget_profile_id"),
        "selected_runtime_baseline_id": runtime_watch_current.get("selected_runtime_baseline_id")
        or runtime_current_manifest.get("selected_baseline_id"),
        "reproposal_trigger_needed": reproposal_trigger_needed,
        "trigger_gate_verdict": trigger_verdict,
        "trigger_confidence": watch_confidence,
        "stable_soft_overrun_count": stable_overrun_count,
        "hard_over_budget_count": hard_over_budget_count,
        "over_budget_ratio": round(over_budget_ratio, 3),
        "bounded_jitter_ok": bounded_jitter_ok,
        "trend_direction": focus_entry.get("trend_direction", runtime_watch_current.get("trend_direction")),
        "watch_status": watch_status,
        "watch_confidence": watch_confidence,
        "sample_count": sample_count,
        "real_sample_count": real_sample_count,
        "approval_grace_active": state == CURRENT_ENV_STATE_APPROVAL_GRACE,
        "next_monitoring_due_at": deadlines.get("next_monitoring_due_at"),
        "next_reproposal_due_at": deadlines.get("next_reproposal_due_at"),
        "current_env_state": state,
        "rationale": rationale,
    }
    payload["trigger_gate_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_due_scheduler(
    *,
    phase: str,
    runtime_current_manifest: dict[str, Any],
    runtime_watch_current: dict[str, Any],
    runtime_watch_refresh: dict[str, Any],
    runtime_watch_history: dict[str, Any],
    runtime_budget_baseline: dict[str, Any],
    guardrail_policy: dict[str, Any],
    current_env_watch_apply: dict[str, Any] | None = None,
    current_time_override: str | None = None,
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override)
    current_env_watch_apply = current_env_watch_apply or {}
    focus_payload = runtime_watch_refresh or runtime_watch_current
    state_before = str(
        focus_payload.get("current_env_state")
        or runtime_watch_history.get("current_env_state")
        or CURRENT_ENV_STATE_CLEAR
    )
    state_after, rationale, deadlines = current_env_guardrail_state(
        now=now,
        policy=guardrail_policy,
        watch_payload=focus_payload,
        watch_history=runtime_watch_history,
        runtime_budget_baseline=runtime_budget_baseline,
    )
    due_soon_window_days = float(guardrail_policy.get("watch_due_soon_window_days", 0.0) or 0.0)
    monitoring_due_at = deadlines.get("next_monitoring_due_at")
    reproposal_due_at = deadlines.get("next_reproposal_due_at")
    if current_env_watch_apply:
        state_before = str(current_env_watch_apply.get("new_state") or state_before)
        monitoring_due_at = current_env_watch_apply.get("updated_next_monitoring_due_at") or monitoring_due_at
        reproposal_due_at = current_env_watch_apply.get("updated_next_reproposal_due_at") or reproposal_due_at
    monitoring_due_state = due_state_for_deadline(now, monitoring_due_at, due_soon_window_days)
    reproposal_due_state = due_state_for_deadline(now, reproposal_due_at, due_soon_window_days)
    if current_env_watch_apply:
        if reproposal_due_state in {CURRENT_ENV_DUE, CURRENT_ENV_DUE_OVERDUE}:
            state_after = CURRENT_ENV_STATE_REPROFILE_CANDIDATE
        elif monitoring_due_state in {CURRENT_ENV_DUE, CURRENT_ENV_DUE_OVERDUE}:
            state_after = CURRENT_ENV_STATE_MONITORING_DUE
        elif monitoring_due_state == CURRENT_ENV_DUE_SOON:
            state_after = CURRENT_ENV_STATE_MONITORING_DUE_SOON
        else:
            state_after = str(current_env_watch_apply.get("new_state") or state_after)
        rationale = list(current_env_watch_apply.get("rationale", [])) or rationale
        rationale.append("due scheduler used watch apply result as the post-action scheduling source")
    approval_grace_active = state_after == CURRENT_ENV_STATE_APPROVAL_GRACE
    action = current_env_due_action(
        monitoring_due_state=monitoring_due_state,
        reproposal_due_state=reproposal_due_state,
        current_state=state_after,
        approval_grace_active=approval_grace_active,
    )
    next_due_kind = "none"
    next_due_at = None
    if reproposal_due_state in {CURRENT_ENV_DUE, CURRENT_ENV_DUE_OVERDUE}:
        next_due_kind = "reproposal"
        next_due_at = reproposal_due_at
    elif monitoring_due_state in {CURRENT_ENV_DUE_SOON, CURRENT_ENV_DUE, CURRENT_ENV_DUE_OVERDUE}:
        next_due_kind = "monitoring"
        next_due_at = monitoring_due_at
    elif reproposal_due_state == CURRENT_ENV_DUE_SOON:
        next_due_kind = "reproposal"
        next_due_at = reproposal_due_at
    release_entry = current_env_focus_entry(
        list(runtime_watch_history.get("entries", []))
        or list(focus_payload.get("entries", []))
    )
    affected_execution_classes = [
        str(entry.get("execution_class", "")).strip()
        for entry in (runtime_watch_history.get("entries", []) or focus_payload.get("entries", []))
        if isinstance(entry, dict) and str(entry.get("execution_class", "")).strip()
    ] or list(guardrail_policy.get("current_env_execution_classes", []))
    repeat_count = 10 if action == "RUN_CURRENT_ENV_REPROPOSAL_GATE" else 5
    payload = {
        "manifest_version": "runtime_current_env_due_scheduler_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "guardrail_policy_id": guardrail_policy.get("policy_id"),
        "now_utc": timestamp_utc_from_datetime(now),
        "current_state_before": state_before,
        "current_state_after": state_after,
        "approval_grace_active": approval_grace_active,
        "due_soon_window_days": due_soon_window_days,
        "monitoring_due_at": monitoring_due_at,
        "reproposal_due_at": reproposal_due_at,
        "monitoring_due_state": monitoring_due_state,
        "reproposal_due_state": reproposal_due_state,
        "next_due_kind": next_due_kind,
        "next_due_at": next_due_at,
        "overdue_days": max(
            overdue_days_for_deadline(now, monitoring_due_at),
            overdue_days_for_deadline(now, reproposal_due_at),
        ),
        "affected_execution_classes": affected_execution_classes,
        "stable_soft_overrun_count": int(release_entry.get("stable_overrun_count", focus_payload.get("stable_overrun_count", 0))),
        "hard_over_budget_count": int(release_entry.get("hard_over_budget_count", focus_payload.get("hard_over_budget_count", 0))),
        "watch_status": release_entry.get("watch_status", focus_payload.get("watch_status")),
        "watch_confidence": release_entry.get("watch_confidence", focus_payload.get("watch_confidence")),
        "recommended_action_current_env": action,
        "recommended_command": current_env_due_command(action, repeat_count),
        "rationale": rationale,
    }
    payload["due_scheduler_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_reproposal_plan(
    *,
    phase: str,
    runtime_current_manifest: dict[str, Any],
    runtime_watch_history: dict[str, Any],
    guardrail_policy: dict[str, Any],
    current_env_due: dict[str, Any],
    execution_class_filter: str = "",
    current_time_override: str | None = None,
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override)
    filter_values = {value.strip() for value in str(execution_class_filter or "").split(",") if value.strip()}
    execution_classes = list(guardrail_policy.get("production_critical_execution_classes", ["release_full"]))
    if filter_values:
        execution_classes = [value for value in execution_classes if value in filter_values]
    reproposal_due_at = str(current_env_due.get("reproposal_due_at") or current_env_due.get("next_reproposal_due_at") or "").strip()
    due_state = str(current_env_due.get("reproposal_due_state") or CURRENT_ENV_DUE_NOT_DUE)
    current_state = str(current_env_due.get("current_state_after") or CURRENT_ENV_STATE_CLEAR)
    entries: list[dict[str, Any]] = []
    for execution_class in execution_classes:
        priority = "LOW"
        if due_state in {CURRENT_ENV_DUE, CURRENT_ENV_DUE_OVERDUE} or current_state == CURRENT_ENV_STATE_REPROFILE_CANDIDATE:
            priority = "HIGH"
        elif due_state == CURRENT_ENV_DUE_SOON:
            priority = "MEDIUM"
        entries.append(
            {
                "execution_class": execution_class,
                "current_state": current_state,
                "reproposal_due_at": reproposal_due_at or None,
                "due_state": due_state,
                "overdue_days": overdue_days_for_deadline(now, reproposal_due_at),
                "minimum_real_samples": int(guardrail_policy.get("min_real_samples_for_reproposal", 5)),
                "minimum_watch_confidence": str(guardrail_policy.get("min_watch_confidence_for_reproposal", "HIGH")),
                "recommended_command": "./raw_engine_tests --case runtime_current_env_reproposal_trigger_gate_smoke",
                "priority": priority,
                "reason": "open the current-env reproposal trigger gate once cooldown and due thresholds are reached",
            }
        )
    verdict = "EMPTY" if not entries else "ACTION_REQUIRED" if any(item["priority"] == "HIGH" for item in entries) else "PASS"
    payload = {
        "manifest_version": "runtime_current_env_reproposal_plan_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "guardrail_policy_id": guardrail_policy.get("policy_id"),
        "plan_verdict": verdict,
        "entry_count": len(entries),
        "entries": entries,
        "current_env_state": current_state,
        "reproposal_due_state": due_state,
        "reproposal_due_at": reproposal_due_at or None,
    }
    payload["reproposal_plan_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_watch_execute(
    *,
    phase: str,
    runtime_current_manifest: dict[str, Any],
    runtime_baseline_manifest: dict[str, Any],
    runtime_current_env_watch: dict[str, Any],
    runtime_watch_history: dict[str, Any],
    guardrail_policy: dict[str, Any],
    execution_class: str,
    repeat: int,
    action_id: str,
    produced_watch_manifest: str | None = None,
    produced_history_update: str | None = None,
) -> dict[str, Any]:
    execution_class = str(execution_class or "release_full").strip()
    action_id = str(action_id or f"{phase}-current-env-watch").strip()
    entry = current_env_entry_for_execution_class(
        execution_class,
        runtime_watch_history,
        runtime_current_env_watch,
    )
    samples = wall_time_samples_from_entry(entry)
    watch_status = str(
        entry.get("watch_status")
        or runtime_current_env_watch.get("watch_status")
        or runtime_gate.WATCH_CLEAR
    ).strip()
    watch_confidence = str(
        entry.get("watch_confidence")
        or runtime_current_env_watch.get("watch_confidence")
        or "LOW"
    ).strip().upper()
    hard_over_budget_count = int(entry.get("hard_over_budget_count", runtime_current_env_watch.get("hard_over_budget_count", 0)))
    soft_over_budget_count = int(entry.get("soft_over_budget_count", runtime_current_env_watch.get("soft_over_budget_count", 0)))
    stable_overrun_count = int(entry.get("stable_overrun_count", runtime_current_env_watch.get("stable_overrun_count", 0)))
    sample_count = max(int(entry.get("sample_count", runtime_current_env_watch.get("sample_count", 0))), int(repeat or 0))
    real_sample_count = int(entry.get("real_sample_count", runtime_current_env_watch.get("real_sample_count", sample_count)))
    execution_verdict = "PASS"
    rationale = ["current env watch campaign execution was materialized from same-fingerprint watch evidence"]
    if watch_status == runtime_gate.WATCH_FAIL:
        execution_verdict = "FAIL"
        rationale.append("observed watch status is FAIL")
    elif hard_over_budget_count >= int(guardrail_policy.get("hard_over_budget_trigger_count", 1)):
        execution_verdict = "FAIL"
        rationale.append("hard budget breach count reached the current-env guardrail trigger")
    elif real_sample_count < int(guardrail_policy.get("post_approval_min_real_samples_before_rewatch", 0)):
        rationale.append("execution completed but real-sample depth is below the rewatch threshold")
    payload = {
        "manifest_version": "runtime_current_env_watch_execute_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "action_id": action_id,
        "action_kind": "RUN_CURRENT_ENV_WATCH_CAMPAIGN",
        "action_status": ACTION_STATUS_EXECUTED if execution_verdict == "PASS" else ACTION_STATUS_FAILED,
        "execution_class": execution_class,
        "repeat": int(repeat or 1),
        "sample_count": sample_count,
        "real_sample_count": real_sample_count,
        "min_wall_time_sec": round(min(samples), 3) if samples else numeric_metric(entry, "min_wall_time_sec", "current_wall_time_sec"),
        "median_wall_time_sec": round(numeric_metric(entry, "median_wall_time_sec", "rolling_median_wall_time_sec", default=percentile_from_values(samples, 50.0)), 3),
        "max_wall_time_sec": round(max(samples), 3) if samples else numeric_metric(entry, "max_wall_time_sec", "current_wall_time_sec"),
        "p90_wall_time_sec": round(numeric_metric(entry, "p90_wall_time_sec", "rolling_p90_wall_time_sec", default=percentile_from_values(samples, 90.0)), 3),
        "p95_wall_time_sec": round(numeric_metric(entry, "p95_wall_time_sec", "rolling_p95_wall_time_sec", default=percentile_from_values(samples, 95.0)), 3),
        "jitter_estimate_percent": round(numeric_metric(entry, "jitter_estimate_percent", "mad_wall_time_sec"), 3),
        "soft_over_budget_count": soft_over_budget_count,
        "stable_overrun_count": stable_overrun_count,
        "hard_over_budget_count": hard_over_budget_count,
        "over_budget_ratio": round(numeric_metric(entry, "over_budget_ratio"), 3),
        "trend_direction": entry.get("trend_direction", runtime_gate.TREND_INSUFFICIENT),
        "watch_status_observed": watch_status,
        "watch_confidence_observed": watch_confidence,
        "execution_verdict": execution_verdict,
        "selected_runtime_baseline_id": runtime_current_env_watch.get("selected_runtime_baseline_id")
        or runtime_current_manifest.get("selected_baseline_id")
        or runtime_baseline_manifest.get("baseline_id"),
        "selected_budget_profile_id": runtime_current_env_watch.get("selected_budget_profile_id"),
        "produced_watch_manifest": produced_watch_manifest,
        "produced_history_update": produced_history_update,
        "rationale": rationale,
    }
    payload["execute_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def next_due_after(now: datetime, days: float | int | None) -> str | None:
    if days is None:
        return None
    return timestamp_utc_from_datetime(now + timedelta(days=float(days or 0.0)))


def build_current_env_watch_apply(
    *,
    phase: str,
    runtime_current_manifest: dict[str, Any],
    runtime_current_env_watch: dict[str, Any],
    runtime_watch_execute: dict[str, Any],
    runtime_watch_history: dict[str, Any],
    guardrail_policy: dict[str, Any],
    current_time_override: str | None = None,
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override)
    previous_state = str(
        runtime_current_env_watch.get("current_env_state")
        or runtime_watch_history.get("current_env_state")
        or CURRENT_ENV_STATE_CLEAR
    )
    observed_watch_status = str(runtime_watch_execute.get("watch_status_observed", runtime_gate.WATCH_CLEAR)).strip()
    observed_watch_confidence = str(runtime_watch_execute.get("watch_confidence_observed", "LOW")).strip().upper()
    sample_count = int(runtime_watch_execute.get("sample_count", 0))
    real_sample_count = int(runtime_watch_execute.get("real_sample_count", 0))
    stable_overrun_count = int(runtime_watch_execute.get("stable_overrun_count", 0))
    hard_over_budget_count = int(runtime_watch_execute.get("hard_over_budget_count", 0))
    execution_verdict = str(runtime_watch_execute.get("execution_verdict", "PASS")).strip()
    required_samples = int(guardrail_policy.get("post_approval_min_real_samples_before_rewatch", 0))
    required_reproposal_samples = int(guardrail_policy.get("min_real_samples_for_reproposal", 5))
    required_confidence = str(guardrail_policy.get("min_watch_confidence_for_reproposal", "HIGH")).strip().upper()
    jitter_percent = float(runtime_watch_execute.get("jitter_estimate_percent", 0.0) or 0.0)
    bounded_jitter_ok = jitter_percent <= float(guardrail_policy.get("max_bounded_jitter_percent_for_reproposal", 15.0))
    new_state = CURRENT_ENV_STATE_CLEAR
    trigger_gate_verdict = "CLEAR"
    recommended_action = "NO_ACTION"
    next_operator_action = "NO_ACTION"
    reproposal_candidate = False
    reproposal_required = False
    rationale: list[str] = []
    if execution_verdict == "FAIL" and observed_watch_status == runtime_gate.WATCH_FAIL:
        new_state = CURRENT_ENV_STATE_FAIL
        trigger_gate_verdict = "FAIL"
        recommended_action = "FAIL"
        next_operator_action = "FAIL"
        reproposal_required = True
        rationale.append("watch execution observed a fail status")
    elif hard_over_budget_count >= int(guardrail_policy.get("hard_over_budget_trigger_count", 1)):
        new_state = CURRENT_ENV_STATE_REPROFILE_REQUIRED
        trigger_gate_verdict = "REQUIRE_REPROFILE"
        recommended_action = "RUN_CURRENT_ENV_REPROPOSAL_GATE"
        next_operator_action = "RUN_CURRENT_ENV_REPROPOSAL_GATE"
        reproposal_required = True
        rationale.append("watch apply promoted the current env to reprofile required because of hard budget breach")
    elif real_sample_count < required_samples:
        new_state = previous_state if previous_state != CURRENT_ENV_STATE_APPROVAL_GRACE else CURRENT_ENV_STATE_MONITORING_DUE
        trigger_gate_verdict = "NEED_MORE_SAMPLES"
        recommended_action = "RUN_CURRENT_ENV_WATCH_CAMPAIGN"
        next_operator_action = "RUN_CURRENT_ENV_WATCH_CAMPAIGN"
        rationale.append("watch apply kept monitoring open because same-fingerprint real sample depth is insufficient")
    elif observed_watch_status == runtime_gate.WATCH_CLEAR:
        new_state = CURRENT_ENV_STATE_CLEAR
        rationale.append("watch apply cleared the current env guardrail after monitoring due execution")
    elif observed_watch_status == runtime_gate.WATCH_STABLE:
        new_state = CURRENT_ENV_STATE_WATCH_STABLE
        trigger_gate_verdict = "WATCH"
        rationale.append("watch apply retained stable bounded overrun evidence after monitoring due execution")
        if (
            stable_overrun_count >= int(guardrail_policy.get("stable_soft_overrun_trigger_count", 5))
            and real_sample_count >= required_reproposal_samples
            and watch_confidence_rank(observed_watch_confidence) >= watch_confidence_rank(required_confidence)
            and bounded_jitter_ok
        ):
            reproposal_candidate = False
            rationale.append("stable evidence is sufficient for future reproposal due evaluation")
    elif observed_watch_status == runtime_gate.WATCH_ESCALATE:
        new_state = CURRENT_ENV_STATE_WATCH_ESCALATE
        trigger_gate_verdict = "WATCH"
        recommended_action = "WATCH_RUNTIME"
        next_operator_action = "WATCH_RUNTIME"
        rationale.append("watch apply escalated current env monitoring")
    else:
        new_state = CURRENT_ENV_STATE_WATCH
        trigger_gate_verdict = "WATCH"
        recommended_action = "WATCH_RUNTIME"
        next_operator_action = "WATCH_RUNTIME"
        rationale.append("watch apply retained current env watch state")
    updated_next_monitoring_due_at = next_due_after(now, guardrail_policy.get("watch_due_after_days", 14))
    updated_next_reproposal_due_at = next_due_after(
        now,
        max(
            float(guardrail_policy.get("reproposal_candidate_after_days", 21)),
            float(guardrail_policy.get("reproposal_cooldown_days", 14)),
        ),
    )
    if new_state in {CURRENT_ENV_STATE_REPROFILE_REQUIRED, CURRENT_ENV_STATE_FAIL}:
        updated_next_reproposal_due_at = timestamp_utc_from_datetime(now)
    payload = {
        "manifest_version": "runtime_current_env_watch_apply_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "action_id": runtime_watch_execute.get("action_id"),
        "action_kind": "RUN_CURRENT_ENV_WATCH_CAMPAIGN",
        "action_status": ACTION_STATUS_APPLIED if execution_verdict == "PASS" else ACTION_STATUS_FAILED,
        "previous_state": previous_state,
        "observed_watch_status": observed_watch_status,
        "observed_watch_confidence": observed_watch_confidence,
        "sample_count": sample_count,
        "real_sample_count": real_sample_count,
        "stable_soft_overrun_count": stable_overrun_count,
        "hard_over_budget_count": hard_over_budget_count,
        "new_state": new_state,
        "updated_next_monitoring_due_at": updated_next_monitoring_due_at,
        "updated_next_reproposal_due_at": updated_next_reproposal_due_at,
        "reproposal_candidate": reproposal_candidate,
        "reproposal_required": reproposal_required,
        "trigger_gate_verdict": trigger_gate_verdict,
        "recommended_action_current_env": recommended_action,
        "next_operator_action": next_operator_action,
        "bounded_jitter_ok": bounded_jitter_ok,
        "selected_runtime_baseline_id": runtime_current_manifest.get("selected_baseline_id")
        or runtime_watch_execute.get("selected_runtime_baseline_id"),
        "execution_manifest": runtime_watch_execute.get("execute_manifest_path"),
        "produced_watch_manifest": runtime_watch_execute.get("produced_watch_manifest"),
        "produced_history_update": runtime_watch_execute.get("produced_history_update"),
        "rationale": rationale,
    }
    payload["apply_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_reproposal_gate_execute(
    *,
    phase: str,
    runtime_current_manifest: dict[str, Any],
    runtime_watch_current: dict[str, Any],
    runtime_watch_history: dict[str, Any],
    runtime_budget_baseline: dict[str, Any],
    guardrail_policy: dict[str, Any],
    action_id: str,
    current_time_override: str | None = None,
    produced_reproposal_gate_manifest: str | None = None,
) -> dict[str, Any]:
    gate = build_current_env_reproposal_trigger_gate(
        phase=phase,
        runtime_current_manifest=runtime_current_manifest,
        runtime_watch_current=runtime_watch_current,
        runtime_watch_history=runtime_watch_history,
        runtime_budget_baseline=runtime_budget_baseline,
        guardrail_policy=guardrail_policy,
        current_time_override=current_time_override,
    )
    gate_verdict = str(gate.get("trigger_gate_verdict", "CLEAR"))
    recommended_next_action = "NO_ACTION"
    if gate_verdict == "WATCH":
        recommended_next_action = "RUN_CURRENT_ENV_WATCH_CAMPAIGN"
    elif gate_verdict in {"CANDIDATE", "REQUIRE_REPROFILE"}:
        recommended_next_action = "PROPOSE_BUDGET_REPROFILE"
    elif gate_verdict == "FAIL":
        recommended_next_action = "FAIL"
    payload = {
        "manifest_version": "runtime_current_env_reproposal_gate_execute_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "action_id": str(action_id or f"{phase}-current-env-reproposal-gate"),
        "action_kind": "RUN_CURRENT_ENV_REPROPOSAL_GATE",
        "action_status": ACTION_STATUS_EXECUTED if gate_verdict != "FAIL" else ACTION_STATUS_FAILED,
        "gate_verdict": gate_verdict,
        "gate_confidence": gate.get("trigger_confidence", gate.get("watch_confidence", "LOW")),
        "reproposal_needed": bool(gate.get("reproposal_trigger_needed", False)),
        "recommended_next_action": recommended_next_action,
        "produced_reproposal_manifest": None,
        "produced_reproposal_gate_manifest": produced_reproposal_gate_manifest,
        "gate": gate,
        "rationale": list(gate.get("rationale", [])),
    }
    payload["execute_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def action_status_rank(status: str) -> int:
    order = {
        ACTION_STATUS_PLANNED: 0,
        ACTION_STATUS_SKIPPED: 1,
        ACTION_STATUS_DEFERRED: 1,
        ACTION_STATUS_RETRY_PENDING: 1,
        ACTION_STATUS_EXECUTED: 2,
        ACTION_STATUS_FAILED: 3,
        ACTION_STATUS_REJECTED: 4,
        ACTION_STATUS_SUPERSEDED: 4,
        ACTION_STATUS_APPLIED: 5,
        ACTION_STATUS_CLOSED: 6,
    }
    return order.get(str(status or "").strip(), -1)


def action_status_counts(entries: list[dict[str, Any]]) -> dict[str, int]:
    return {
        "planned_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_PLANNED),
        "executed_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_EXECUTED),
        "applied_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_APPLIED),
        "skipped_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_SKIPPED),
        "failed_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_FAILED),
        "superseded_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_SUPERSEDED),
        "deferred_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_DEFERRED),
        "rejected_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_REJECTED),
        "closed_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_CLOSED),
        "retry_pending_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_RETRY_PENDING),
    }


def default_ledger_entry(
    *,
    action_id: str,
    action_kind: str,
    action_status: str,
    planned_at: str | None = None,
    due_at: str | None = None,
    target_execution_class: str | None = None,
    operator_note: str = "",
) -> dict[str, Any]:
    return {
        "action_id": action_id,
        "action_kind": action_kind,
        "action_status": action_status,
        "planned_at": planned_at,
        "due_at": due_at,
        "executed_at": None,
        "applied_at": None,
        "superseded_at": None,
        "closed_at": None,
        "defer_until": None,
        "closure_status": CLOSURE_STATUS_OPEN,
        "target_execution_class": target_execution_class,
        "execution_manifest": None,
        "apply_manifest": None,
        "decision_manifest": None,
        "decision_apply_manifest": None,
        "result_state": None,
        "next_due_at": None,
        "retry_count": 0,
        "retry_policy": None,
        "failure_class": None,
        "failure_reason": None,
        "superseded_by": None,
        "decision_id": None,
        "operator_decision": None,
        "approval_mode": None,
        "approval_command": None,
        "approval_applied": False,
        "operator_note": operator_note,
    }


def merge_ledger_entry(entries_by_id: dict[str, dict[str, Any]], incoming: dict[str, Any]) -> None:
    action_id = str(incoming.get("action_id") or "").strip()
    if not action_id:
        return
    existing = dict(entries_by_id.get(action_id, {}))
    if not existing:
        entries_by_id[action_id] = dict(incoming)
        return
    merged = dict(existing)
    for key, value in incoming.items():
        if value is not None and value != "":
            merged[key] = value
    if action_status_rank(str(incoming.get("action_status"))) < action_status_rank(str(existing.get("action_status"))):
        merged["action_status"] = existing.get("action_status")
    entries_by_id[action_id] = merged


def build_current_env_action_ledger(
    *,
    phase: str,
    runtime_current_env_agenda: dict[str, Any],
    watch_execute: dict[str, Any],
    watch_apply: dict[str, Any],
    reproposal_execute: dict[str, Any],
    ledger_in: dict[str, Any] | None = None,
    current_time_override: str | None = None,
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override)
    now_text = timestamp_utc_from_datetime(now)
    previous_entries = [
        dict(item)
        for item in (ledger_in or {}).get("entries", [])
        if isinstance(item, dict) and str(item.get("action_id", "")).strip()
    ]
    entries_by_id = {str(item.get("action_id")): dict(item) for item in previous_entries}
    generated_at = str(runtime_current_env_agenda.get("generated_at_utc") or now_text)

    for item in runtime_current_env_agenda.get("items", []):
        if not isinstance(item, dict) or str(item.get("domain")) != "current_env":
            continue
        action_kind = str(item.get("action_kind") or "NO_ACTION")
        if action_kind == "NO_ACTION":
            continue
        action_id = (
            str(item.get("action_id") or "").strip()
            or str(item.get("next_action_id") or "").strip()
            or str(item.get("item_id") or f"{phase}-current-env-planned")
        )
        entry = default_ledger_entry(
            action_id=action_id,
            action_kind=action_kind,
            action_status=str(item.get("action_status") or ACTION_STATUS_PLANNED),
            planned_at=generated_at,
            due_at=item.get("due_at"),
            target_execution_class="release_full" if "WATCH" in action_kind else None,
            operator_note="planned by ops agenda",
        )
        entry["execution_manifest"] = item.get("execution_manifest")
        entry["apply_manifest"] = item.get("apply_manifest")
        merge_ledger_entry(entries_by_id, entry)

    if watch_execute:
        action_id = str(watch_execute.get("action_id") or f"{phase}-current-env-watch").strip()
        entry = default_ledger_entry(
            action_id=action_id,
            action_kind="RUN_CURRENT_ENV_WATCH_CAMPAIGN",
            action_status=str(watch_execute.get("action_status") or ACTION_STATUS_EXECUTED),
            planned_at=generated_at,
            due_at=runtime_current_env_agenda.get("next_due_at"),
            target_execution_class=str(watch_execute.get("execution_class") or "release_full"),
            operator_note="materialized by watch execute manifest",
        )
        entry["executed_at"] = watch_execute.get("generated_at_utc")
        entry["execution_manifest"] = watch_execute.get("execute_manifest_path")
        if str(watch_execute.get("execution_verdict")) == "FAIL" or entry["action_status"] == ACTION_STATUS_FAILED:
            entry["action_status"] = ACTION_STATUS_FAILED
            entry["failure_class"] = watch_execute.get("failure_class") or (
                "HARD_BUDGET_BREACH" if int(watch_execute.get("hard_over_budget_count", 0) or 0) > 0 else "INFRA_TRANSIENT"
            )
            entry["failure_reason"] = "; ".join(str(item) for item in watch_execute.get("rationale", []))
        merge_ledger_entry(entries_by_id, entry)

    if watch_apply:
        action_id = str(watch_apply.get("action_id") or watch_execute.get("action_id") or f"{phase}-current-env-watch").strip()
        entry = default_ledger_entry(
            action_id=action_id,
            action_kind="RUN_CURRENT_ENV_WATCH_CAMPAIGN",
            action_status=str(watch_apply.get("action_status") or ACTION_STATUS_APPLIED),
            planned_at=generated_at,
            due_at=runtime_current_env_agenda.get("next_due_at"),
            target_execution_class=str(watch_execute.get("execution_class") or "release_full"),
            operator_note="applied by watch apply manifest",
        )
        entry["executed_at"] = watch_execute.get("generated_at_utc")
        entry["applied_at"] = watch_apply.get("generated_at_utc")
        entry["execution_manifest"] = watch_apply.get("execution_manifest") or watch_execute.get("execute_manifest_path")
        entry["apply_manifest"] = watch_apply.get("apply_manifest_path")
        entry["result_state"] = watch_apply.get("new_state")
        entry["next_due_at"] = watch_apply.get("updated_next_monitoring_due_at") or watch_apply.get("updated_next_reproposal_due_at")
        merge_ledger_entry(entries_by_id, entry)

    if reproposal_execute:
        action_id = str(reproposal_execute.get("action_id") or f"{phase}-current-env-reproposal").strip()
        status = str(reproposal_execute.get("action_status") or ACTION_STATUS_EXECUTED)
        entry = default_ledger_entry(
            action_id=action_id,
            action_kind="RUN_CURRENT_ENV_REPROPOSAL_GATE",
            action_status=status,
            planned_at=generated_at,
            due_at=runtime_current_env_agenda.get("next_due_at"),
            target_execution_class="release_full",
            operator_note="materialized by reproposal gate execute manifest",
        )
        entry["executed_at"] = reproposal_execute.get("generated_at_utc")
        entry["execution_manifest"] = reproposal_execute.get("execute_manifest_path")
        entry["result_state"] = reproposal_execute.get("gate_verdict")
        if status == ACTION_STATUS_FAILED:
            entry["failure_class"] = reproposal_execute.get("failure_class") or "REPROPOSAL_GATE_FAIL"
            entry["failure_reason"] = "; ".join(str(item) for item in reproposal_execute.get("rationale", []))
        merge_ledger_entry(entries_by_id, entry)

    newest_non_terminal_by_kind: dict[str, str] = {}
    for action_id, entry in sorted(entries_by_id.items(), key=lambda pair: str(pair[1].get("planned_at") or pair[1].get("executed_at") or "")):
        status = str(entry.get("action_status") or "")
        kind = str(entry.get("action_kind") or "")
        if status in {
            ACTION_STATUS_APPLIED,
            ACTION_STATUS_FAILED,
            ACTION_STATUS_SKIPPED,
            ACTION_STATUS_SUPERSEDED,
            ACTION_STATUS_DEFERRED,
            ACTION_STATUS_REJECTED,
            ACTION_STATUS_CLOSED,
            ACTION_STATUS_RETRY_PENDING,
        }:
            continue
        previous_id = newest_non_terminal_by_kind.get(kind)
        if previous_id and previous_id != action_id:
            previous = entries_by_id[previous_id]
            previous["action_status"] = ACTION_STATUS_SUPERSEDED
            previous["superseded_at"] = now_text
            previous["superseded_by"] = action_id
        newest_non_terminal_by_kind[kind] = action_id

    entries = sorted(entries_by_id.values(), key=lambda item: (str(item.get("planned_at") or ""), str(item.get("action_id") or "")))
    counts = action_status_counts(entries)
    latest_applied = next((item for item in reversed(entries) if item.get("action_status") == ACTION_STATUS_APPLIED), {})
    next_planned = next((item for item in entries if item.get("action_status") == ACTION_STATUS_PLANNED), {})
    payload = {
        "manifest_version": "runtime_current_env_action_ledger_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "ledger_updated_at_utc": now_text,
        "total_action_count": len(entries),
        **counts,
        "latest_applied_action_id": latest_applied.get("action_id"),
        "next_planned_action_id": next_planned.get("action_id"),
        "entries": entries,
    }
    payload["ledger_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_action_retry_plan(
    *,
    phase: str,
    action_ledger: dict[str, Any],
    retry_policy: dict[str, Any],
    current_time_override: str | None = None,
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override)
    entries: list[dict[str, Any]] = []
    retryable_classes = set(str(item) for item in retry_policy.get("retryable_failure_classes", []))
    non_retryable_classes = set(str(item) for item in retry_policy.get("non_retryable_failure_classes", []))
    max_retry_count = int(retry_policy.get("max_retry_count", 0))
    backoff_minutes = int(retry_policy.get("retry_backoff_minutes", 30))
    escalation_threshold = int(retry_policy.get("failed_action_escalation_threshold", max_retry_count))
    for item in action_ledger.get("entries", []):
        if not isinstance(item, dict):
            continue
        status = str(item.get("action_status") or "")
        if status not in {ACTION_STATUS_FAILED, ACTION_STATUS_SKIPPED}:
            continue
        failure_class = str(item.get("failure_class") or ("SKIPPED" if status == ACTION_STATUS_SKIPPED else "INFRA_TRANSIENT"))
        retry_count = int(item.get("retry_count", 0) or 0)
        retryable = (
            status == ACTION_STATUS_FAILED
            and failure_class in retryable_classes
            and retry_count < max_retry_count
        )
        escalation_required = (
            failure_class in non_retryable_classes
            or retry_count >= escalation_threshold
            or status == ACTION_STATUS_SKIPPED and bool(retry_policy.get("skip_requires_reason", True)) and not item.get("operator_note")
        )
        next_retry_at = timestamp_utc_from_datetime(now + timedelta(minutes=backoff_minutes * (retry_count + 1))) if retryable else None
        action_kind = str(item.get("action_kind") or "")
        command = ""
        if retryable:
            if action_kind == "RUN_CURRENT_ENV_WATCH_CAMPAIGN":
                command = "./raw_engine_tests --case runtime_current_env_execute_watch_smoke"
            elif action_kind == "RUN_CURRENT_ENV_REPROPOSAL_GATE":
                command = "./raw_engine_tests --case runtime_current_env_execute_reproposal_gate_smoke"
            else:
                command = "./raw_engine_tests --case runtime_current_env_due_scheduler_smoke"
        entries.append(
            {
                "action_id": item.get("action_id"),
                "action_kind": action_kind,
                "current_status": status,
                "retryable": retryable,
                "retry_status": ACTION_RETRY_STATUS_RETRYABLE if retryable else ACTION_RETRY_STATUS_ESCALATE if escalation_required else ACTION_RETRY_STATUS_NONE,
                "retry_count": retry_count,
                "next_retry_at": next_retry_at,
                "recommended_command": command,
                "escalation_required": escalation_required,
                "failure_class": failure_class,
                "reason": item.get("failure_reason") or item.get("operator_note") or "action requires retry policy evaluation",
            }
        )
    payload = {
        "manifest_version": "runtime_current_env_action_retry_plan_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "retry_policy_id": retry_policy.get("policy_id"),
        "entry_count": len(entries),
        "retryable_count": sum(1 for item in entries if item.get("retryable")),
        "escalation_count": sum(1 for item in entries if item.get("escalation_required")),
        "next_retry_at": min([item["next_retry_at"] for item in entries if item.get("next_retry_at")] or [None]),
        "entries": entries,
        "plan_verdict": "EMPTY"
        if not entries
        else "ACTION_REQUIRED"
        if any(item.get("retryable") for item in entries)
        else "ESCALATE"
        if any(item.get("escalation_required") for item in entries)
        else "PASS",
    }
    payload["retry_plan_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_reproposal_handoff(
    *,
    phase: str,
    reproposal_execute: dict[str, Any],
    runtime_budget_registry: dict[str, Any],
    runtime_current_manifest: dict[str, Any],
    runtime_budget_baseline: dict[str, Any],
) -> dict[str, Any]:
    raw_gate_verdict = str(reproposal_execute.get("gate_verdict") or "NOT_RUN")
    reproposal_needed = bool(reproposal_execute.get("reproposal_needed", False))
    normalized_gate_verdict = raw_gate_verdict
    if raw_gate_verdict in {"CANDIDATE", "REQUIRE_REPROFILE"} and reproposal_needed:
        normalized_gate_verdict = "APPROVABLE"
    approval_ready = normalized_gate_verdict == "APPROVABLE" and reproposal_needed
    approval_blockers: list[str] = []
    next_action_kind = "NO_ACTION"
    if approval_ready:
        next_action_kind = "APPROVE_RUNTIME_BUDGET_REPROFILE"
    elif raw_gate_verdict in {"WATCH", "NEED_MORE_SAMPLES"}:
        approval_blockers.append("additional same-fingerprint watch evidence is required before budget approval")
        next_action_kind = "RUN_CURRENT_ENV_WATCH_CAMPAIGN"
    elif raw_gate_verdict in {"REJECT", "FAIL"}:
        approval_blockers.append("reproposal gate rejected or failed; escalate instead of approving budget")
        next_action_kind = "ESCALATE_RUNTIME_BUDGET_REPROFILE"
    else:
        approval_blockers.append("reproposal gate is not approval-ready")
        next_action_kind = "NO_ACTION"
    produced_reproposal_manifest = reproposal_execute.get("produced_reproposal_manifest")
    produced_gate_manifest = reproposal_execute.get("produced_reproposal_gate_manifest")
    recommended_approval_command = ""
    if approval_ready:
        recommended_approval_command = (
            "./raw_engine_tests --case runtime_budget_approve_reprofile_smoke"
        )
    payload = {
        "manifest_version": "runtime_current_env_reproposal_handoff_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "action_id": reproposal_execute.get("action_id"),
        "raw_gate_verdict": raw_gate_verdict,
        "gate_verdict": normalized_gate_verdict,
        "gate_confidence": reproposal_execute.get("gate_confidence", "LOW"),
        "reproposal_needed": reproposal_needed,
        "produced_reproposal_manifest": produced_reproposal_manifest,
        "produced_reproposal_gate_manifest": produced_gate_manifest,
        "approval_ready": approval_ready,
        "approval_blockers": approval_blockers,
        "recommended_approval_command": recommended_approval_command,
        "required_operator_checks": [
            "confirm same-fingerprint runtime current manifest",
            "confirm active runtime budget registry target",
            "confirm no semantic/correctness gate failure",
            "archive reproposal evidence before activation",
        ],
        "next_action_kind": next_action_kind,
        "active_budget_profile_id": runtime_budget_baseline.get("profile_id")
        or runtime_budget_registry.get("active_profile_id"),
        "runtime_fingerprint_key": runtime_current_manifest.get("runtime_fingerprint_key"),
        "handoff_status": ACTION_HANDOFF_APPROVAL_READY if approval_ready else ACTION_HANDOFF_ESCALATE if next_action_kind.startswith("ESCALATE") else ACTION_HANDOFF_NOT_READY,
        "rationale": list(reproposal_execute.get("rationale", []))
        + (["reproposal gate is approval-ready but approval remains an explicit operator step"] if approval_ready else approval_blockers),
    }
    payload["handoff_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def find_action_entry(action_ledger: dict[str, Any], action_id: str) -> dict[str, Any]:
    for item in action_ledger.get("entries", []):
        if isinstance(item, dict) and str(item.get("action_id") or "") == action_id:
            return dict(item)
    return {}


def retryable_action_ids(retry_plan: dict[str, Any]) -> set[str]:
    return {
        str(item.get("action_id"))
        for item in retry_plan.get("entries", [])
        if isinstance(item, dict) and bool(item.get("retryable", False)) and str(item.get("action_id") or "")
    }


def build_current_env_operator_decision(
    *,
    phase: str,
    action_ledger: dict[str, Any],
    handoff: dict[str, Any],
    retry_plan: dict[str, Any],
    action_id: str,
    decision: str,
    decision_reason: str,
    decision_note: str | None = None,
    defer_until: str | None = None,
    operator_id: str | None = None,
    current_time_override: str | None = None,
    approval_mode: str = "handoff_only",
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override)
    now_text = timestamp_utc_from_datetime(now)
    decision = str(decision or "").strip().lower()
    entry = find_action_entry(action_ledger, action_id)
    handoff_action_id = str(handoff.get("action_id") or "")
    handoff_matches = bool(handoff) and (not handoff_action_id or handoff_action_id == action_id)
    previous_status = str(entry.get("action_status") or ("EXECUTED" if handoff_matches else "UNKNOWN"))
    action_kind = str(entry.get("action_kind") or ("RUN_CURRENT_ENV_REPROPOSAL_GATE" if handoff_matches else "UNKNOWN"))
    rationale: list[str] = []
    valid = True
    resulting_status = previous_status
    next_action_kind = "NO_ACTION"
    next_due_at = entry.get("next_due_at") or entry.get("due_at")
    affects_budget_approval = False
    affects_retry_plan = False
    affects_due_schedule = False
    closure_status = CLOSURE_STATUS_OPEN

    if decision == OPERATOR_DECISION_APPROVE:
        affects_budget_approval = True
        if not (handoff_matches and bool(handoff.get("approval_ready", False))):
            valid = False
            rationale.append("approve requires an approval-ready reproposal handoff for the selected action")
        resulting_status = ACTION_STATUS_CLOSED if approval_mode == "handoff_only" else ACTION_STATUS_APPLIED
        closure_status = CLOSURE_STATUS_CLOSED if approval_mode == "handoff_only" else CLOSURE_STATUS_APPROVAL_APPLIED
        next_action_kind = str(handoff.get("next_action_kind") or "APPROVE_RUNTIME_BUDGET_REPROFILE")
        rationale.append("approval decision closes the handoff checkpoint; budget switch remains explicit unless integrated mode is requested")
    elif decision == OPERATOR_DECISION_SKIP:
        if not str(decision_reason or "").strip():
            valid = False
            rationale.append("skip requires a decision reason")
        resulting_status = ACTION_STATUS_SKIPPED
        closure_status = CLOSURE_STATUS_CLOSED
        affects_due_schedule = True
        next_action_kind = "NO_ACTION"
        rationale.append("skip closes the current action without retry")
    elif decision == OPERATOR_DECISION_DEFER:
        if not str(defer_until or "").strip():
            valid = False
            rationale.append("defer requires defer_until/next_due_at")
        resulting_status = ACTION_STATUS_DEFERRED
        closure_status = CLOSURE_STATUS_DEFERRED
        next_due_at = defer_until
        affects_due_schedule = True
        next_action_kind = str(entry.get("action_kind") or "RUN_CURRENT_ENV_WATCH_CAMPAIGN")
        rationale.append("defer keeps the action open until the deferred due time")
    elif decision == OPERATOR_DECISION_REJECT:
        resulting_status = ACTION_STATUS_REJECTED
        closure_status = CLOSURE_STATUS_REJECTED
        next_action_kind = "ESCALATE_CURRENT_ENV_ACTION"
        affects_retry_plan = True
        rationale.append("reject closes the action as operator-rejected and leaves any residual risk explicit")
    elif decision == OPERATOR_DECISION_CLOSE:
        if previous_status not in {ACTION_STATUS_APPLIED, ACTION_STATUS_EXECUTED, ACTION_STATUS_CLOSED}:
            valid = False
            rationale.append("close is only valid for already applied, executed, or closed actions")
        resulting_status = ACTION_STATUS_CLOSED
        closure_status = CLOSURE_STATUS_CLOSED
        next_action_kind = "NO_ACTION"
        rationale.append("close resolves an already applied action in the ledger")
    elif decision == OPERATOR_DECISION_RETRY_NOW:
        affects_retry_plan = True
        retryable_ids = retryable_action_ids(retry_plan)
        if action_id not in retryable_ids:
            valid = False
            rationale.append("retry_now requires a retryable failed action in the retry plan")
        resulting_status = ACTION_STATUS_RETRY_PENDING
        closure_status = CLOSURE_STATUS_RETRY_PENDING
        next_action_kind = str(entry.get("action_kind") or "RUN_CURRENT_ENV_WATCH_CAMPAIGN")
        rationale.append("retry_now moves the failed action back into the retry queue")
    else:
        valid = False
        rationale.append("unknown operator decision")

    if decision_reason:
        rationale.append(str(decision_reason))
    payload = {
        "manifest_version": "runtime_current_env_operator_decision_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "decision_id": f"{phase}-operator-decision-{sha256_text(action_id + decision + now_text)[:12]}",
        "action_id": action_id,
        "action_kind": action_kind,
        "previous_action_status": previous_status,
        "decision": decision,
        "decision_valid": valid,
        "decision_reason": decision_reason,
        "decision_note": decision_note or "",
        "decided_at": now_text,
        "operator_id": operator_id or "local-operator",
        "runner_id": operator_id or "local-operator",
        "resulting_action_status": resulting_status,
        "closure_status": closure_status,
        "next_action_kind": next_action_kind,
        "next_due_at": next_due_at,
        "defer_until": defer_until,
        "approval_mode": approval_mode,
        "approval_ready": bool(handoff.get("approval_ready", False)) if handoff_matches else False,
        "approval_command": str(handoff.get("recommended_approval_command") or ""),
        "approval_applied": approval_mode == "integrated" and decision == OPERATOR_DECISION_APPROVE and valid,
        "affects_budget_approval": affects_budget_approval,
        "affects_retry_plan": affects_retry_plan,
        "affects_due_schedule": affects_due_schedule,
        "rationale": rationale,
    }
    payload["decision_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_apply_operator_decision(
    *,
    phase: str,
    action_ledger: dict[str, Any],
    operator_decision: dict[str, Any],
    runtime_current_manifest: dict[str, Any],
    runtime_budget_registry: dict[str, Any],
    approval_mode: str = "handoff_only",
    apply_manifest_path: str | None = None,
) -> dict[str, Any]:
    decision_valid = bool(operator_decision.get("decision_valid", True))
    action_id = str(operator_decision.get("action_id") or "")
    previous_entry = find_action_entry(action_ledger, action_id)
    previous_status = str(previous_entry.get("action_status") or operator_decision.get("previous_action_status") or "UNKNOWN")
    new_status = str(operator_decision.get("resulting_action_status") or previous_status)
    closure_status = str(operator_decision.get("closure_status") or CLOSURE_STATUS_OPEN)
    entries: list[dict[str, Any]] = []
    updated = False
    for item in action_ledger.get("entries", []):
        if not isinstance(item, dict):
            continue
        entry = dict(item)
        if str(entry.get("action_id") or "") == action_id:
            updated = True
            if decision_valid:
                entry["action_status"] = new_status
                entry["closure_status"] = closure_status
                entry["decision_id"] = operator_decision.get("decision_id")
                entry["operator_decision"] = operator_decision.get("decision")
                entry["decision_manifest"] = operator_decision.get("decision_manifest_path")
                entry["decision_apply_manifest"] = apply_manifest_path
                entry["operator_note"] = operator_decision.get("decision_note") or operator_decision.get("decision_reason")
                entry["next_due_at"] = operator_decision.get("next_due_at")
                entry["defer_until"] = operator_decision.get("defer_until")
                entry["approval_mode"] = operator_decision.get("approval_mode", approval_mode)
                entry["approval_command"] = operator_decision.get("approval_command")
                entry["approval_applied"] = bool(operator_decision.get("approval_applied", False))
                if new_status == ACTION_STATUS_CLOSED:
                    entry["closed_at"] = operator_decision.get("decided_at")
                if new_status == ACTION_STATUS_RETRY_PENDING:
                    entry["retry_count"] = int(entry.get("retry_count", 0) or 0) + 1
            entries.append(entry)
    if not updated and action_id:
        entry = default_ledger_entry(
            action_id=action_id,
            action_kind=str(operator_decision.get("action_kind") or "UNKNOWN"),
            action_status=new_status if decision_valid else str(operator_decision.get("previous_action_status") or "UNKNOWN"),
            planned_at=operator_decision.get("decided_at"),
            due_at=operator_decision.get("next_due_at"),
            operator_note=operator_decision.get("decision_reason", ""),
        )
        entry["closure_status"] = closure_status
        entry["decision_id"] = operator_decision.get("decision_id")
        entry["operator_decision"] = operator_decision.get("decision")
        entry["decision_manifest"] = operator_decision.get("decision_manifest_path")
        entry["decision_apply_manifest"] = apply_manifest_path
        entries.append(entry)
    counts = action_status_counts(entries)
    produced_budget_baseline = None
    produced_approval_metadata = None
    if (
        decision_valid
        and str(operator_decision.get("decision")) == OPERATOR_DECISION_APPROVE
        and str(operator_decision.get("approval_mode", approval_mode)) == "integrated"
    ):
        produced_budget_baseline = "integrated approval requested; use runtime_budget_approve_reprofile workflow output"
        produced_approval_metadata = {
            "approved_from_operator_decision": operator_decision.get("decision_id"),
            "runtime_fingerprint_key": runtime_current_manifest.get("runtime_fingerprint_key"),
            "active_budget_profile_id": runtime_budget_registry.get("active_profile_id"),
        }
    payload = {
        "manifest_version": "runtime_current_env_operator_decision_apply_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "decision_id": operator_decision.get("decision_id"),
        "action_id": action_id,
        "decision_valid": decision_valid,
        "previous_status": previous_status,
        "new_status": new_status if decision_valid else previous_status,
        "closure_status": closure_status if decision_valid else CLOSURE_STATUS_OPEN,
        "ledger_updated": bool(decision_valid),
        "retry_plan_updated": bool(operator_decision.get("affects_retry_plan", False)),
        "due_schedule_updated": bool(operator_decision.get("affects_due_schedule", False)),
        "approval_mode": operator_decision.get("approval_mode", approval_mode),
        "approval_applied": bool(operator_decision.get("approval_applied", False)),
        "produced_budget_baseline": produced_budget_baseline,
        "produced_approval_metadata": produced_approval_metadata,
        "next_operator_action": operator_decision.get("next_action_kind"),
        "updated_ledger": {
            "manifest_version": "runtime_current_env_action_ledger_v1",
            "phase": phase,
            "generated_at_utc": runtime_gate.timestamp_utc_now(),
            "ledger_updated_at_utc": operator_decision.get("decided_at"),
            "total_action_count": len(entries),
            **counts,
            "latest_applied_action_id": action_ledger.get("latest_applied_action_id"),
            "next_planned_action_id": next((item.get("action_id") for item in entries if item.get("action_status") == ACTION_STATUS_PLANNED), None),
            "entries": entries,
        },
        "rationale": list(operator_decision.get("rationale", [])),
    }
    payload["updated_ledger"]["ledger_hash"] = sha256_text(json.dumps(payload["updated_ledger"], sort_keys=True))
    payload["decision_apply_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_action_ledger_compact(
    *,
    phase: str,
    action_ledger: dict[str, Any],
    keep_latest_active: int,
    keep_latest_closed: int,
    keep_failed: bool,
    keep_approval_actions: bool,
) -> tuple[dict[str, Any], dict[str, Any]]:
    entries = [dict(item) for item in action_ledger.get("entries", []) if isinstance(item, dict)]
    terminal_statuses = {ACTION_STATUS_CLOSED, ACTION_STATUS_SKIPPED, ACTION_STATUS_REJECTED, ACTION_STATUS_SUPERSEDED}
    active_entries = [item for item in entries if str(item.get("action_status")) not in terminal_statuses]
    closed_entries = [item for item in entries if str(item.get("action_status")) in terminal_statuses]
    retained_ids: set[str] = set()
    for item in active_entries[-max(keep_latest_active, 0):]:
        retained_ids.add(str(item.get("action_id")))
    for item in closed_entries[-max(keep_latest_closed, 0):]:
        retained_ids.add(str(item.get("action_id")))
    if keep_failed:
        for item in entries:
            if str(item.get("action_status")) in {ACTION_STATUS_FAILED, ACTION_STATUS_RETRY_PENDING}:
                retained_ids.add(str(item.get("action_id")))
    if keep_approval_actions:
        for item in entries:
            if "APPROVE" in str(item.get("action_kind")) or bool(item.get("approval_command")):
                retained_ids.add(str(item.get("action_id")))
    retained_entries = [item for item in entries if str(item.get("action_id")) in retained_ids]
    archived_entries = [item for item in entries if str(item.get("action_id")) not in retained_ids]
    retained_counts = action_status_counts(retained_entries)
    archive_counts = action_status_counts(archived_entries)
    compact = {
        "manifest_version": "runtime_current_env_action_ledger_compact_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "source_ledger_hash": action_ledger.get("ledger_hash"),
        "active_action_count": len(active_entries),
        "closed_action_count": len(closed_entries),
        "archived_action_count": len(archived_entries),
        "failed_action_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_FAILED),
        "deferred_action_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_DEFERRED),
        "retry_pending_count": sum(1 for item in entries if item.get("action_status") == ACTION_STATUS_RETRY_PENDING),
        "approval_action_count": sum(1 for item in entries if "APPROVE" in str(item.get("action_kind")) or bool(item.get("approval_command"))),
        "retained_action_ids": [str(item.get("action_id")) for item in retained_entries],
        "archived_action_ids": [str(item.get("action_id")) for item in archived_entries],
        "compacted_ledger": {
            "manifest_version": "runtime_current_env_action_ledger_v1",
            "phase": phase,
            "generated_at_utc": runtime_gate.timestamp_utc_now(),
            "ledger_updated_at_utc": runtime_gate.timestamp_utc_now(),
            "total_action_count": len(retained_entries),
            **retained_counts,
            "latest_applied_action_id": action_ledger.get("latest_applied_action_id"),
            "next_planned_action_id": next((item.get("action_id") for item in retained_entries if item.get("action_status") == ACTION_STATUS_PLANNED), None),
            "entries": retained_entries,
        },
    }
    compact["compacted_ledger"]["ledger_hash"] = sha256_text(json.dumps(compact["compacted_ledger"], sort_keys=True))
    archive = {
        "manifest_version": "runtime_current_env_action_ledger_archive_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "source_ledger_hash": action_ledger.get("ledger_hash"),
        "archived_action_count": len(archived_entries),
        **{f"archived_{key}": value for key, value in archive_counts.items()},
        "archived_action_ids": [str(item.get("action_id")) for item in archived_entries],
        "archived_entries": archived_entries,
    }
    compact["compact_hash"] = sha256_text(json.dumps(compact, sort_keys=True))
    archive["archive_hash"] = sha256_text(json.dumps(archive, sort_keys=True))
    return compact, archive


def build_current_env_approval_runbook(
    *,
    phase: str,
    handoff: dict[str, Any],
    operator_decision: dict[str, Any],
    runtime_current_manifest: dict[str, Any],
    runtime_budget_registry: dict[str, Any],
    runtime_budget_baseline_out: str,
    budget_tag: str,
    approval_mode: str,
) -> dict[str, Any]:
    decision_is_approve = str(operator_decision.get("decision") or "") == OPERATOR_DECISION_APPROVE
    decision_valid = bool(operator_decision.get("decision_valid", True))
    handoff_ready = bool(handoff.get("approval_ready", False))
    action_id = str(handoff.get("action_id") or operator_decision.get("action_id") or "")
    blockers: list[str] = list(handoff.get("approval_blockers", []))
    if not handoff_ready:
        blockers.append("reproposal handoff is not approval-ready")
    if not decision_is_approve or not decision_valid:
        blockers.append("operator decision is not a valid approve decision")
    if approval_mode not in {"handoff_only", "integrated"}:
        blockers.append("approval mode must be handoff_only or integrated")
    executable = not blockers
    selected_profile = (
        handoff.get("active_budget_profile_id")
        or runtime_budget_registry.get("active_profile_id")
        or runtime_budget_registry.get("active_budget_profile_id")
    )
    proposed_profile = (
        handoff.get("proposed_budget_profile_id")
        or operator_decision.get("proposed_budget_profile_id")
        or budget_tag
    )
    recommended_command = ""
    if executable:
        recommended_command = (
            "python3 tests/tools/runtime_gate_lib.py budget-approve-reprofile"
            " --runtime-budget-current <runtime_budget_current>"
            " --runtime-budget-proposal <runtime_budget_proposal>"
            " --runtime-budget-proposal-gate <runtime_budget_proposal_gate>"
            " --runtime-budget-registry "
            + str(Path(str(runtime_budget_registry.get("registry_path", "<runtime_budget_registry>"))).as_posix())
            + " --runtime-budget-baseline-out "
            + str(runtime_budget_baseline_out)
            + " --budget-tag "
            + str(budget_tag)
            + " --activate"
        )
    payload = {
        "manifest_version": "runtime_current_env_approval_runbook_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "runbook_id": f"{phase}-approval-runbook-{sha256_text(action_id + str(operator_decision.get('decision_id')) + budget_tag)[:12]}",
        "action_id": action_id,
        "decision_id": operator_decision.get("decision_id"),
        "approval_mode": approval_mode,
        "safety_level": "integrated_opt_in" if approval_mode == "integrated" else "handoff_only",
        "mutates_registry": approval_mode == "integrated",
        "approval_ready": executable,
        "executable": executable,
        "approval_blockers": sorted(set(blockers)),
        "required_inputs": [
            "runtime_budget_current",
            "runtime_budget_proposal",
            "runtime_budget_proposal_gate",
            "runtime_budget_registry",
            "runtime_budget_baseline_out",
        ],
        "expected_outputs": [
            str(runtime_budget_baseline_out),
            "runtime budget approval metadata sidecar",
            "updated runtime budget registry",
            "archived reproposal snapshot",
        ],
        "recommended_command": recommended_command,
        "selected_budget_profile_id": selected_profile,
        "proposed_budget_profile_id": proposed_profile,
        "runtime_current_manifest": runtime_current_manifest.get("manifest_path")
        or runtime_current_manifest.get("runtime_current_manifest_path"),
        "runtime_budget_registry": runtime_budget_registry.get("registry_path"),
        "runtime_budget_baseline_out": runtime_budget_baseline_out,
        "budget_tag": budget_tag,
        "preflight_checks": [
            "operator decision is approve and valid",
            "handoff approval_ready is true",
            "correctness lifecycle remains PASS/FRESH",
            "runtime budget proposal gate remains approval-ready",
            "budget registry target is explicit",
        ],
        "rationale": [
            "handoff_only remains the default; runbook records the explicit operator approval command",
            "integrated mode is opt-in and should only be used by explicit smoke or operator request",
        ],
    }
    if not executable:
        payload["rationale"].append("approval runbook is non-executable until blockers are resolved")
    payload["runbook_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_execute_budget_approval(
    *,
    phase: str,
    approval_runbook: dict[str, Any],
    runtime_budget_current: dict[str, Any],
    runtime_budget_proposal: dict[str, Any],
    runtime_budget_proposal_gate: dict[str, Any],
    runtime_budget_registry: dict[str, Any],
    runtime_budget_baseline_out: str,
    archive_proposal: str,
    approval_execution_mode: str = APPROVAL_EXECUTION_MODE_HANDOFF_ONLY,
    integrated_opt_in: bool = False,
    approval_confirmation_token: str = "",
    dry_run_preflight: dict[str, Any] | None = None,
    require_preflight_success: bool = False,
) -> dict[str, Any]:
    approval_ready = bool(approval_runbook.get("approval_ready", False))
    approval_mode = str(approval_runbook.get("approval_mode") or "handoff_only")
    approval_execution_mode = str(approval_execution_mode or APPROVAL_EXECUTION_MODE_HANDOFF_ONLY)
    blockers: list[str] = list(approval_runbook.get("approval_blockers", []))
    gate_verdict = str(
        runtime_budget_proposal_gate.get("reproposal_gate_verdict")
        or runtime_budget_proposal_gate.get("proposal_gate_verdict")
        or approval_runbook.get("gate_verdict")
        or ""
    )
    if gate_verdict and gate_verdict not in {"APPROVABLE", "PASS"}:
        blockers.append(f"budget proposal gate is not approvable: {gate_verdict}")
    if approval_execution_mode not in {
        APPROVAL_EXECUTION_MODE_DRY_RUN,
        APPROVAL_EXECUTION_MODE_HANDOFF_ONLY,
        APPROVAL_EXECUTION_MODE_INTEGRATED_OPT_IN,
    }:
        blockers.append(f"unknown approval execution mode: {approval_execution_mode}")
    if not approval_ready:
        status = APPROVAL_STATUS_BLOCKED
    elif approval_execution_mode == APPROVAL_EXECUTION_MODE_DRY_RUN:
        status = APPROVAL_STATUS_DRY_RUN
    elif approval_execution_mode == APPROVAL_EXECUTION_MODE_INTEGRATED_OPT_IN and approval_mode != "integrated":
        status = APPROVAL_STATUS_BLOCKED
        blockers.append("integrated_opt_in execution requires an integrated approval runbook")
    elif approval_execution_mode == APPROVAL_EXECUTION_MODE_INTEGRATED_OPT_IN and not integrated_opt_in:
        status = APPROVAL_STATUS_BLOCKED
        blockers.append("integrated approval requires explicit opt-in")
    elif approval_execution_mode == APPROVAL_EXECUTION_MODE_INTEGRATED_OPT_IN:
        expected_prefix = "confirm-integrated-approval:"
        confirmation_ok = str(approval_confirmation_token or "").startswith(expected_prefix)
        preflight_success = bool(
            dry_run_preflight
            and str(dry_run_preflight.get("approval_status", "")) == APPROVAL_STATUS_DRY_RUN
            and not bool(dry_run_preflight.get("registry_updated", False))
            and not bool(dry_run_preflight.get("baseline_written", False))
        )
        if not confirmation_ok:
            status = APPROVAL_STATUS_BLOCKED
            blockers.append("integrated approval requires explicit safety confirmation token")
        elif require_preflight_success and not preflight_success:
            status = APPROVAL_STATUS_BLOCKED
            blockers.append("integrated approval requires successful dry-run preflight")
        elif not preflight_success:
            status = APPROVAL_STATUS_BLOCKED
            blockers.append("integrated approval requires dry-run preflight manifest")
        else:
            status = APPROVAL_STATUS_APPLIED
    else:
        status = APPROVAL_STATUS_DRY_RUN
    previous_profile = (
        runtime_budget_registry.get("active_profile_id")
        or runtime_budget_registry.get("active_budget_profile_id")
        or approval_runbook.get("selected_budget_profile_id")
    )
    new_profile = runtime_budget_proposal.get("proposed_budget_profile_id") or approval_runbook.get("proposed_budget_profile_id")
    registry_updated = status == APPROVAL_STATUS_APPLIED
    baseline_written = status == APPROVAL_STATUS_APPLIED
    proposal_archived = status in {APPROVAL_STATUS_APPLIED, APPROVAL_STATUS_DRY_RUN}
    metadata_path = str(Path(runtime_budget_baseline_out).with_name(Path(runtime_budget_baseline_out).stem + "_approval_metadata.json"))
    registry_before_hash = sha256_text(json.dumps(runtime_budget_registry, sort_keys=True))
    registry_after_payload = dict(runtime_budget_registry)
    if registry_updated:
        registry_after_payload["active_profile_id"] = new_profile
        registry_after_payload["active_budget_profile_id"] = new_profile
        registry_after_payload["previous_active_budget_profile_id"] = previous_profile
        registry_after_payload["approval_transaction_marker"] = True
    registry_after_hash = sha256_text(json.dumps(registry_after_payload, sort_keys=True))
    baseline_payload = {
        "manifest_version": "runtime_budget_baseline_integrated_approval_marker_v1",
        "phase": phase,
        "approved_from_runbook_id": approval_runbook.get("runbook_id"),
        "previous_active_budget_profile_id": previous_profile,
        "new_active_budget_profile_id": new_profile,
        "runtime_budget_current_hash": sha256_text(json.dumps(runtime_budget_current, sort_keys=True)),
        "runtime_budget_proposal_hash": sha256_text(json.dumps(runtime_budget_proposal, sort_keys=True)),
        "runtime_budget_proposal_gate_hash": sha256_text(json.dumps(runtime_budget_proposal_gate, sort_keys=True)),
    }
    baseline_written_hash = sha256_text(json.dumps(baseline_payload, sort_keys=True)) if baseline_written else None
    confirmation_hash = sha256_text(str(approval_confirmation_token)) if approval_confirmation_token else None
    payload = {
        "manifest_version": "runtime_current_env_budget_approval_execution_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "runbook_id": approval_runbook.get("runbook_id"),
        "action_id": approval_runbook.get("action_id"),
        "decision_id": approval_runbook.get("decision_id"),
        "approval_execution_id": f"{phase}-budget-approval-{sha256_text(str(approval_runbook.get('runbook_id')) + status)[:12]}",
        "approval_mode": approval_mode,
        "approval_execution_mode": approval_execution_mode,
        "allow_integrated_approval": bool(integrated_opt_in),
        "approval_confirmation_token_present": bool(approval_confirmation_token),
        "approval_confirmation_token_hash": confirmation_hash,
        "require_preflight_success": bool(require_preflight_success),
        "dry_run_preflight_success": bool(
            dry_run_preflight
            and str(dry_run_preflight.get("approval_status", "")) == APPROVAL_STATUS_DRY_RUN
            and not bool(dry_run_preflight.get("registry_updated", False))
            and not bool(dry_run_preflight.get("baseline_written", False))
        ),
        "approval_status": status,
        "approval_ready": approval_ready,
        "approval_blockers": sorted(set(blockers)),
        "previous_active_budget_profile_id": previous_profile,
        "new_active_budget_profile_id": new_profile,
        "registry_before_hash": registry_before_hash,
        "registry_after_hash": registry_after_hash,
        "baseline_written_hash": baseline_written_hash,
        "approval_transaction_id": f"{phase}-approval-tx-{sha256_text(str(approval_runbook.get('runbook_id')) + str(new_profile) + str(confirmation_hash))[:12]}"
        if status == APPROVAL_STATUS_APPLIED
        else None,
        "rollback_hint": "restore runtime budget registry to registry_before_hash and remove emitted budget baseline if post-approval verification fails"
        if status == APPROVAL_STATUS_APPLIED
        else "no rollback needed; dry_run/handoff_only did not mutate registry or baseline",
        "registry_updated": registry_updated,
        "baseline_written": baseline_written,
        "proposal_archived": proposal_archived,
        "runtime_budget_baseline_out": runtime_budget_baseline_out,
        "archive_proposal": archive_proposal,
        "approval_metadata_path": metadata_path if status != APPROVAL_STATUS_BLOCKED else None,
        "verification_needed": status == APPROVAL_STATUS_APPLIED,
        "next_operator_action": "RUN_POST_APPROVAL_REFRESH"
        if status == APPROVAL_STATUS_APPLIED
        else "RUN_RUNTIME_BUDGET_APPROVAL_COMMAND"
        if status == APPROVAL_STATUS_DRY_RUN
        else "RESOLVE_APPROVAL_BLOCKERS",
        "rationale": [
            "approval execution audit wraps the explicit budget approval step without making handoff_only mutate registry state",
        ],
    }
    if status == APPROVAL_STATUS_DRY_RUN:
        if approval_execution_mode == APPROVAL_EXECUTION_MODE_HANDOFF_ONLY:
            payload["rationale"].append("handoff_only approval execution is a preflight/dry-run audit by default")
        else:
            payload["rationale"].append("dry_run approval execution validates inputs without registry mutation")
    if status == APPROVAL_STATUS_APPLIED:
        payload["rationale"].append("integrated approval was explicitly opted in for this execution")
    payload["approval_execution_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_current_env_link_approval_execution(
    *,
    phase: str,
    action_ledger: dict[str, Any],
    approval_execution: dict[str, Any],
) -> tuple[dict[str, Any], dict[str, Any]]:
    action_id = str(approval_execution.get("action_id") or "")
    entries: list[dict[str, Any]] = []
    previous_closure = CLOSURE_STATUS_OPEN
    updated = False
    for item in action_ledger.get("entries", []):
        if not isinstance(item, dict):
            continue
        entry = dict(item)
        if str(entry.get("action_id") or "") == action_id:
            previous_closure = str(entry.get("closure_status") or CLOSURE_STATUS_OPEN)
            entry["approval_execution_id"] = approval_execution.get("approval_execution_id")
            entry["approval_execution_manifest"] = approval_execution.get("approval_execution_manifest_path")
            entry["approval_status"] = approval_execution.get("approval_status")
            entry["approval_applied"] = approval_execution.get("approval_status") == APPROVAL_STATUS_APPLIED
            entry["approval_preflight_only"] = approval_execution.get("approval_status") == APPROVAL_STATUS_DRY_RUN
            entry["linked_budget_baseline"] = approval_execution.get("runtime_budget_baseline_out")
            entry["linked_registry_update"] = bool(approval_execution.get("registry_updated", False))
            updated = True
        entries.append(entry)
    if not updated and action_id:
        entry = default_ledger_entry(
            action_id=action_id,
            action_kind="APPROVE_RUNTIME_BUDGET_REPROFILE",
            action_status=ACTION_STATUS_CLOSED,
            planned_at=approval_execution.get("generated_at_utc"),
            due_at=None,
            operator_note="approval execution link created without prior ledger entry",
        )
        previous_closure = CLOSURE_STATUS_OPEN
        entry["closure_status"] = CLOSURE_STATUS_CLOSED
        entry["approval_execution_id"] = approval_execution.get("approval_execution_id")
        entry["approval_status"] = approval_execution.get("approval_status")
        entry["approval_applied"] = approval_execution.get("approval_status") == APPROVAL_STATUS_APPLIED
        entries.append(entry)
    counts = action_status_counts(entries)
    linked_ledger = {
        "manifest_version": "runtime_current_env_action_ledger_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "ledger_updated_at_utc": runtime_gate.timestamp_utc_now(),
        "total_action_count": len(entries),
        **counts,
        "latest_applied_action_id": action_ledger.get("latest_applied_action_id"),
        "next_planned_action_id": next((item.get("action_id") for item in entries if item.get("action_status") == ACTION_STATUS_PLANNED), None),
        "entries": entries,
    }
    linked_ledger["ledger_hash"] = sha256_text(json.dumps(linked_ledger, sort_keys=True))
    status = str(approval_execution.get("approval_status") or APPROVAL_STATUS_BLOCKED)
    new_closure = CLOSURE_STATUS_APPROVAL_APPLIED if status == APPROVAL_STATUS_APPLIED else previous_closure
    payload = {
        "manifest_version": "runtime_current_env_approval_link_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "action_id": action_id,
        "decision_id": approval_execution.get("decision_id"),
        "approval_execution_id": approval_execution.get("approval_execution_id"),
        "previous_closure_status": previous_closure,
        "new_closure_status": new_closure,
        "approval_applied": status == APPROVAL_STATUS_APPLIED,
        "approval_preflight_only": status == APPROVAL_STATUS_DRY_RUN,
        "approval_status": status,
        "linked_budget_baseline": approval_execution.get("runtime_budget_baseline_out"),
        "linked_registry_update": bool(approval_execution.get("registry_updated", False)),
        "ledger_updated": True,
        "next_due_at": None,
        "linked_ledger": linked_ledger,
        "rationale": [
            "approval execution result is linked back to the current-env action ledger",
        ],
    }
    if status == APPROVAL_STATUS_DRY_RUN:
        payload["rationale"].append("ledger records approval_preflight_only; budget approval remains explicit")
    if status == APPROVAL_STATUS_BLOCKED:
        payload["rationale"].append("approval blocker remains visible in ops summary")
    payload["approval_link_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload, linked_ledger


def runbook_type_for_action(action_kind: str, decision: str | None = None) -> str:
    action_kind = str(action_kind or "").strip()
    decision = str(decision or "").strip().lower()
    if action_kind == "APPROVE_RUNTIME_BUDGET_REPROFILE" or decision == OPERATOR_DECISION_APPROVE:
        return "approval"
    if action_kind == "RUN_CURRENT_ENV_WATCH_CAMPAIGN":
        return "watch"
    if action_kind == "RUN_CURRENT_ENV_REPROPOSAL_GATE":
        return "reproposal_gate"
    if decision == OPERATOR_DECISION_RETRY_NOW or "RETRY" in action_kind:
        return "retry"
    if decision in {OPERATOR_DECISION_SKIP, OPERATOR_DECISION_DEFER, OPERATOR_DECISION_REJECT, OPERATOR_DECISION_CLOSE}:
        return decision
    return "watch" if not action_kind else action_kind.lower()


def build_operator_runbook_index(
    *,
    phase: str,
    action_ledger: dict[str, Any],
    ops_agenda: dict[str, Any],
    approval_runbook: dict[str, Any],
    approval_execution: dict[str, Any],
    operator_decision: dict[str, Any],
    decision_apply: dict[str, Any],
) -> dict[str, Any]:
    entries: list[dict[str, Any]] = []
    ledger_by_id = {
        str(item.get("action_id") or ""): item
        for item in action_ledger.get("entries", [])
        if isinstance(item, dict) and str(item.get("action_id") or "")
    }
    agenda_items = [
        item for item in ops_agenda.get("items", [])
        if isinstance(item, dict) and str(item.get("action_kind") or "NO_ACTION") != "NO_ACTION"
    ]
    if not agenda_items and approval_runbook:
        agenda_items = [
            {
                "action_id": approval_runbook.get("action_id"),
                "action_kind": "APPROVE_RUNTIME_BUDGET_REPROFILE",
                "action_status": ACTION_STATUS_PLANNED,
                "decision_status": "DECIDED" if operator_decision else "PENDING_OPERATOR_DECISION",
                "recommended_command": approval_runbook.get("recommended_command", ""),
                "target_id": approval_runbook.get("action_id"),
            }
        ]
    for item in agenda_items:
        action_id = str(item.get("action_id") or item.get("target_id") or approval_runbook.get("action_id") or "")
        ledger_entry = ledger_by_id.get(action_id, {})
        action_kind = str(item.get("action_kind") or ledger_entry.get("action_kind") or "")
        decision = operator_decision.get("decision") if str(operator_decision.get("action_id") or "") == action_id else None
        is_approval = bool(approval_runbook) and action_id == str(approval_runbook.get("action_id") or "")
        approval_mode = str(approval_runbook.get("approval_mode") or "handoff_only") if is_approval else ""
        safety_level = "integrated_opt_in" if approval_mode == "integrated" else "handoff_only" if is_approval else "dry_run"
        blockers = list(approval_runbook.get("approval_blockers", [])) if is_approval else []
        if approval_mode == "integrated":
            blockers.append("integrated approval requires explicit opt-in")
        executable = bool(item.get("recommended_command") or approval_runbook.get("recommended_command")) and not (
            approval_mode == "integrated"
        )
        entry = {
            "runbook_id": approval_runbook.get("runbook_id") if is_approval else f"{phase}-runbook-{sha256_text(action_id + action_kind)[:12]}",
            "action_id": action_id,
            "action_kind": action_kind,
            "runbook_type": runbook_type_for_action(action_kind, decision),
            "current_status": ledger_entry.get("action_status") or item.get("action_status") or ACTION_STATUS_PLANNED,
            "decision_status": item.get("decision_status") or ("DECIDED" if decision else "PENDING_OPERATOR_DECISION"),
            "approval_mode": approval_mode,
            "executable": bool(executable),
            "recommended_command": str(item.get("recommended_command") or approval_runbook.get("recommended_command") or ""),
            "required_inputs": list(approval_runbook.get("required_inputs", [])) if is_approval else [],
            "expected_outputs": list(approval_runbook.get("expected_outputs", [])) if is_approval else [],
            "blockers": sorted(set(str(blocker) for blocker in blockers if str(blocker).strip())),
            "safety_level": safety_level,
            "operator_required": str(item.get("decision_status") or "") == "PENDING_OPERATOR_DECISION" or is_approval,
            "next_action_kind": item.get("next_operator_action") or approval_execution.get("next_operator_action") or "NO_ACTION",
            "mutates_registry": bool(approval_execution.get("registry_updated", False)),
            "related_manifests": {
                "action_ledger": action_ledger.get("ledger_manifest_path"),
                "ops_agenda": ops_agenda.get("agenda_manifest_path"),
                "approval_runbook": approval_runbook.get("runbook_manifest_path")
                or approval_runbook.get("approval_runbook_manifest_path"),
                "approval_execution": approval_execution.get("approval_execution_manifest_path"),
                "operator_decision": operator_decision.get("decision_manifest_path"),
                "decision_apply": decision_apply.get("decision_apply_manifest_path"),
            },
        }
        entries.append(entry)
    decision = str(operator_decision.get("decision") or "").strip()
    if decision and not any(entry.get("runbook_type") == decision for entry in entries):
        action_id = str(operator_decision.get("action_id") or "")
        entries.append(
            {
                "runbook_id": f"{phase}-runbook-{decision}-{sha256_text(action_id + decision)[:12]}",
                "action_id": action_id,
                "action_kind": operator_decision.get("action_kind"),
                "runbook_type": runbook_type_for_action(str(operator_decision.get("action_kind") or ""), decision),
                "current_status": operator_decision.get("resulting_action_status"),
                "decision_status": "DECIDED" if bool(operator_decision.get("decision_valid", True)) else "INVALID",
                "approval_mode": operator_decision.get("approval_mode", ""),
                "executable": True,
                "recommended_command": decision_apply.get("next_operator_action") or operator_decision.get("next_action_kind") or "",
                "required_inputs": ["action_ledger", "operator_decision"],
                "expected_outputs": ["updated action ledger", "ops summary decision variant"],
                "blockers": [],
                "safety_level": "handoff_only",
                "operator_required": False,
                "next_action_kind": decision_apply.get("next_operator_action") or operator_decision.get("next_action_kind"),
                "mutates_registry": False,
                "related_manifests": {
                    "operator_decision": operator_decision.get("decision_manifest_path"),
                    "decision_apply": decision_apply.get("decision_apply_manifest_path"),
                },
            }
        )
    payload = {
        "manifest_version": "operator_runbook_index_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "runbook_count": len(entries),
        "pending_runbook_count": sum(1 for entry in entries if str(entry.get("current_status")) in {ACTION_STATUS_PLANNED, ACTION_STATUS_RETRY_PENDING}),
        "executable_runbook_count": sum(1 for entry in entries if bool(entry.get("executable", False))),
        "integrated_opt_in_required_count": sum(1 for entry in entries if entry.get("safety_level") == "integrated_opt_in"),
        "approval_runbook_count": sum(1 for entry in entries if entry.get("runbook_type") == "approval"),
        "retry_runbook_count": sum(1 for entry in entries if entry.get("runbook_type") == "retry"),
        "empty_verdict": "PASS" if not entries else "NOT_EMPTY",
        "entries": entries,
    }
    payload["runbook_index_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def read_related_manifest(path_value: Any) -> dict[str, Any]:
    value = str(path_value or "").strip()
    if not value:
        return {}
    path = Path(value)
    if not path.exists():
        return {}
    try:
        return read_json(path)
    except Exception:
        return {}


def build_operator_runbook_catalog(
    *,
    phase: str,
    runbook_index: dict[str, Any],
    catalog_in: dict[str, Any],
    current_time_override: str | None = None,
) -> dict[str, Any]:
    now = current_time_override or runtime_gate.timestamp_utc_now()
    existing_entries = {
        str(entry.get("runbook_id") or ""): dict(entry)
        for entry in catalog_in.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("runbook_id") or "")
    }
    entries: list[dict[str, Any]] = []
    seen: set[str] = set()
    for index_entry in runbook_index.get("entries", []):
        if not isinstance(index_entry, dict):
            continue
        runbook_id = str(index_entry.get("runbook_id") or "")
        if not runbook_id:
            continue
        seen.add(runbook_id)
        related = dict(index_entry.get("related_manifests", {}))
        decision_manifest = read_related_manifest(related.get("operator_decision"))
        decision_apply_manifest = read_related_manifest(related.get("decision_apply"))
        approval_execution_manifest = read_related_manifest(related.get("approval_execution"))
        prior = existing_entries.get(runbook_id, {})
        current_status = str(index_entry.get("current_status") or prior.get("current_status") or ACTION_STATUS_PLANNED)
        latest_closure = (
            decision_apply_manifest.get("closure_status")
            or prior.get("latest_closure_status")
            or CLOSURE_STATUS_OPEN
        )
        runbook_type = str(index_entry.get("runbook_type") or prior.get("runbook_type") or "")
        retained_reason = bool(
            decision_manifest.get("decision_reason")
            or decision_manifest.get("decision_note")
            or prior.get("retained_reason_metadata", False)
        )
        retained_defer = bool(
            decision_manifest.get("defer_until")
            or decision_apply_manifest.get("defer_until")
            or prior.get("retained_defer_metadata", False)
        )
        retained_retry = bool(
            decision_apply_manifest.get("retry_action_id")
            or decision_apply_manifest.get("next_retry_action_id")
            or int(decision_apply_manifest.get("retry_count", 0) or 0) > 0
            or prior.get("retained_retry_metadata", False)
        )
        entry = {
            "runbook_id": runbook_id,
            "action_id": index_entry.get("action_id"),
            "runbook_type": runbook_type,
            "first_seen_at": prior.get("first_seen_at") or now,
            "last_seen_at": now,
            "phase_first_seen": prior.get("phase_first_seen") or phase,
            "phase_last_seen": phase,
            "current_status": current_status,
            "execution_count": int(prior.get("execution_count", 0))
            + (1 if related.get("approval_execution") or approval_execution_manifest else 0),
            "decision_count": int(prior.get("decision_count", 0)) + (1 if decision_manifest else 0),
            "apply_count": int(prior.get("apply_count", 0)) + (1 if decision_apply_manifest else 0),
            "close_count": int(prior.get("close_count", 0))
            + (1 if current_status in {ACTION_STATUS_CLOSED, ACTION_STATUS_SKIPPED, ACTION_STATUS_REJECTED} or latest_closure in {CLOSURE_STATUS_CLOSED, CLOSURE_STATUS_REJECTED, CLOSURE_STATUS_APPROVAL_APPLIED} else 0),
            "failure_count": int(prior.get("failure_count", 0)) + (1 if current_status == ACTION_STATUS_FAILED else 0),
            "latest_decision_id": decision_manifest.get("decision_id") or prior.get("latest_decision_id"),
            "latest_execution_manifest": related.get("approval_execution") or prior.get("latest_execution_manifest"),
            "latest_apply_manifest": related.get("decision_apply") or prior.get("latest_apply_manifest"),
            "latest_closure_status": latest_closure,
            "safety_level": index_entry.get("safety_level") or prior.get("safety_level") or "dry_run",
            "integrated_opt_in_required": bool(index_entry.get("safety_level") == "integrated_opt_in" or prior.get("integrated_opt_in_required", False)),
            "mutates_registry": bool(index_entry.get("mutates_registry", False)),
            "retained_reason_metadata": retained_reason,
            "retained_defer_metadata": retained_defer,
            "retained_retry_metadata": retained_retry,
            "retained_approval_pointers": bool(
                related.get("approval_runbook")
                and (related.get("approval_execution") or approval_execution_manifest)
            )
            or bool(
                runbook_type == "approval"
                and (related.get("approval_execution") or (related.get("operator_decision") and related.get("decision_apply")))
            )
            or bool(prior.get("retained_approval_pointers", False)),
            "related_manifests": related,
            "history": list(prior.get("history", []))
            + [
                {
                    "phase": phase,
                    "seen_at": now,
                    "current_status": current_status,
                    "closure_status": latest_closure,
                    "decision_id": decision_manifest.get("decision_id"),
                    "execution_manifest": related.get("approval_execution"),
                    "apply_manifest": related.get("decision_apply"),
                }
            ],
        }
        entries.append(entry)
    for runbook_id, prior in existing_entries.items():
        if runbook_id in seen:
            continue
        entry = dict(prior)
        entry["last_seen_at"] = entry.get("last_seen_at") or now
        entries.append(entry)
    active_statuses = {ACTION_STATUS_PLANNED, ACTION_STATUS_EXECUTED, ACTION_STATUS_FAILED, ACTION_STATUS_DEFERRED, ACTION_STATUS_RETRY_PENDING}
    active_entries = [entry for entry in entries if str(entry.get("current_status")) in active_statuses]
    resolved_entries = [entry for entry in entries if entry not in active_entries]
    payload = {
        "manifest_version": "operator_runbook_catalog_v1",
        "phase": phase,
        "generated_at_utc": now,
        "catalog_entry_count": len(entries),
        "active_runbook_count": len(active_entries),
        "resolved_runbook_count": len(resolved_entries),
        "integrated_opt_in_required_count": sum(1 for entry in entries if bool(entry.get("integrated_opt_in_required", False))),
        "replayable_runbook_count": sum(1 for entry in entries if bool(entry.get("related_manifests", {}))),
        "entries": entries,
    }
    payload["catalog_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def normalize_operator_runbook_retention_policy(raw_policy: dict[str, Any] | None, *, phase: str) -> dict[str, Any]:
    policy = json.loads(json.dumps(DEFAULT_OPERATOR_RUNBOOK_RETENTION_POLICY))
    if raw_policy:
        for key, value in raw_policy.items():
            policy[key] = value
    policy["phase"] = phase
    policy["generated_at_utc"] = runtime_gate.timestamp_utc_now()
    policy["policy_hash"] = sha256_text(json.dumps(policy, sort_keys=True))
    return policy


def runbook_is_active(entry: dict[str, Any]) -> bool:
    status = str(entry.get("current_status") or entry.get("action_status") or "")
    closure = str(entry.get("latest_closure_status") or entry.get("closure_status") or "")
    return status in {ACTION_STATUS_PLANNED, ACTION_STATUS_EXECUTED, ACTION_STATUS_FAILED, ACTION_STATUS_DEFERRED, ACTION_STATUS_RETRY_PENDING} or closure in {
        CLOSURE_STATUS_OPEN,
        CLOSURE_STATUS_DEFERRED,
        CLOSURE_STATUS_RETRY_PENDING,
    }


def ledger_actions_by_id(action_ledger: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {
        str(entry.get("action_id") or ""): entry
        for entry in action_ledger.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("action_id") or "")
    }


def build_operator_runbook_catalog_prune(
    *,
    phase: str,
    runbook_catalog: dict[str, Any],
    action_ledger: dict[str, Any],
    retention_policy: dict[str, Any],
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    now = parse_timestamp_utc(runtime_gate.timestamp_utc_now()) or datetime.now(timezone.utc)
    policy = normalize_operator_runbook_retention_policy(retention_policy, phase=phase)
    ledger_by_id = ledger_actions_by_id(action_ledger)
    keep_latest_per_type = int(policy.get("keep_latest_resolved_per_type", 0) or 0)
    archive_after_days = float(policy.get("archive_resolved_after_days", 0) or 0)
    prune_after_days = float(policy.get("prune_archived_after_days", 0) or 0)
    entries = [entry for entry in runbook_catalog.get("entries", []) if isinstance(entry, dict)]

    resolved_by_type: dict[str, list[dict[str, Any]]] = {}
    for entry in entries:
        if not runbook_is_active(entry):
            resolved_by_type.setdefault(str(entry.get("runbook_type") or "unknown"), []).append(entry)
    latest_resolved_ids: set[str] = set()
    for group in resolved_by_type.values():
        sorted_group = sorted(group, key=lambda item: str(item.get("last_seen_at") or item.get("first_seen_at") or ""), reverse=True)
        latest_resolved_ids.update(str(item.get("runbook_id") or "") for item in sorted_group[:keep_latest_per_type])

    retained: list[dict[str, Any]] = []
    archived: list[dict[str, Any]] = []
    pruned_ids: list[str] = []
    blocked_prune_ids: list[str] = []
    counts = {
        "active_retained_count": 0,
        "failed_retained_count": 0,
        "retry_pending_retained_count": 0,
        "approval_retained_count": 0,
        "resolved_retained_count": 0,
    }
    for entry in entries:
        runbook_id = str(entry.get("runbook_id") or "")
        action_id = str(entry.get("action_id") or "")
        status = str(entry.get("current_status") or "")
        runbook_type = str(entry.get("runbook_type") or "")
        ledger_entry = ledger_by_id.get(action_id, {})
        open_ledger_pointer = bool(ledger_entry) and str(ledger_entry.get("closure_status") or CLOSURE_STATUS_OPEN) not in {
            CLOSURE_STATUS_CLOSED,
            CLOSURE_STATUS_REJECTED,
            CLOSURE_STATUS_APPROVAL_APPLIED,
        }
        approval_transaction = bool(
            entry.get("retained_approval_pointers")
            or ledger_entry.get("approval_execution_id")
            or ledger_entry.get("approval_transaction_id")
            or runbook_type == "approval"
        )
        active = runbook_is_active(entry)
        failed = status == ACTION_STATUS_FAILED or int(entry.get("failure_count", 0) or 0) > 0
        retry_pending = status == ACTION_STATUS_RETRY_PENDING or str(ledger_entry.get("action_status") or "") == ACTION_STATUS_RETRY_PENDING
        integrated = bool(entry.get("integrated_opt_in_required", False))
        entry_age = age_days(str(entry.get("last_seen_at") or entry.get("first_seen_at") or ""), now=now)
        entry_age = 0.0 if entry_age is None else float(entry_age)

        keep = False
        keep_reason: list[str] = []
        if active and bool(policy.get("keep_active_runbooks", True)):
            keep = True
            keep_reason.append("active runbook retained")
            counts["active_retained_count"] += 1
        if failed and bool(policy.get("keep_failed_runbooks", True)):
            keep = True
            keep_reason.append("failed runbook retained")
            counts["failed_retained_count"] += 1
        if retry_pending and bool(policy.get("keep_retry_pending_runbooks", True)):
            keep = True
            keep_reason.append("retry-pending runbook retained")
            counts["retry_pending_retained_count"] += 1
        if integrated and bool(policy.get("keep_integrated_approval_runbooks", True)):
            keep = True
            keep_reason.append("integrated approval runbook retained")
        if approval_transaction and bool(policy.get("keep_runbook_with_approval_transaction", True)):
            keep = True
            keep_reason.append("approval transaction or handoff pointer retained")
            counts["approval_retained_count"] += 1
        if open_ledger_pointer and bool(policy.get("keep_runbook_with_open_ledger_pointer", True)):
            keep = True
            keep_reason.append("open ledger pointer blocks pruning")
            blocked_prune_ids.append(runbook_id)
        if runbook_id in latest_resolved_ids:
            keep = True
            keep_reason.append("latest resolved runbook retained by type")
            counts["resolved_retained_count"] += 1

        updated = dict(entry)
        updated["retention_decision"] = "retain" if keep else "archive" if entry_age >= archive_after_days else "retain"
        updated["retention_reason"] = sorted(set(keep_reason)) if keep_reason else ["resolved runbook is below archive age threshold"]
        if keep or entry_age < archive_after_days:
            retained.append(updated)
        elif entry_age >= archive_after_days + prune_after_days and not approval_transaction:
            pruned_ids.append(runbook_id)
        else:
            archived_entry = dict(updated)
            archived_entry["archived_at"] = runtime_gate.timestamp_utc_now()
            archived.append(archived_entry)

    pruned_catalog = dict(runbook_catalog)
    pruned_catalog["manifest_version"] = "operator_runbook_catalog_pruned_v1"
    pruned_catalog["phase"] = phase
    pruned_catalog["generated_at_utc"] = runtime_gate.timestamp_utc_now()
    pruned_catalog["entries"] = retained
    pruned_catalog["catalog_entry_count"] = len(retained)
    pruned_catalog["active_runbook_count"] = sum(1 for entry in retained if runbook_is_active(entry))
    pruned_catalog["resolved_runbook_count"] = len(retained) - int(pruned_catalog["active_runbook_count"])
    pruned_catalog["catalog_hash"] = sha256_text(json.dumps(pruned_catalog, sort_keys=True))

    archive = {
        "manifest_version": "operator_runbook_archive_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "archived_count": len(archived),
        "pruned_count": len(pruned_ids),
        "entries": archived,
        "pruned_runbook_ids": sorted(pruned_ids),
    }
    archive["archive_hash"] = sha256_text(json.dumps(archive, sort_keys=True))

    summary = {
        "manifest_version": "operator_runbook_prune_summary_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        **counts,
        "archived_count": len(archived),
        "pruned_count": len(pruned_ids),
        "blocked_prune_count": len(set(blocked_prune_ids)),
        "blocked_prune_runbook_ids": sorted(set(blocked_prune_ids)),
        "prune_verdict": "PASS",
        "retention_policy_hash": policy.get("policy_hash"),
        "pruned_catalog_hash": pruned_catalog.get("catalog_hash"),
        "archive_hash": archive.get("archive_hash"),
        "rationale": [
            "active, failed, retry-pending, approval-related, and open-ledger runbooks are retained",
            "eligible resolved runbooks move to an archive manifest before pruning",
        ],
    }
    summary["prune_summary_hash"] = sha256_text(json.dumps(summary, sort_keys=True))
    return pruned_catalog, archive, summary


def build_operator_runbook_lifecycle_validation(
    *,
    phase: str,
    runbook_catalog: dict[str, Any],
    action_ledger: dict[str, Any],
    current_manifest_root: Path,
) -> dict[str, Any]:
    ledger_by_id = ledger_actions_by_id(action_ledger)
    missing_inputs: list[str] = []
    stale_inputs: list[str] = []
    superseded_ids: list[str] = []
    archived_ids: list[str] = []
    replayable_ids: list[str] = []
    non_replayable_ids: list[str] = []
    mutation_risk_ids: list[str] = []
    integrated_blocked_ids: list[str] = []
    stale_but_safe_ids: list[str] = []
    entries = [entry for entry in runbook_catalog.get("entries", []) if isinstance(entry, dict)]
    root = current_manifest_root.resolve()

    def resolve_lifecycle_pointer(raw_value: str) -> Path:
        path = Path(raw_value)
        if path.is_absolute():
            return path
        return root / path

    for entry in entries:
        runbook_id = str(entry.get("runbook_id") or entry.get("action_id") or "")
        action_id = str(entry.get("action_id") or "")
        status = str(entry.get("current_status") or "")
        active = runbook_is_active(entry)
        related = dict(entry.get("related_manifests", {}))
        related_paths = [str(value) for value in related.values() if str(value or "").strip()]
        missing_for_entry = []
        stale_for_entry = []
        for value in related_paths:
            path = resolve_lifecycle_pointer(value)
            if not path.exists():
                missing_for_entry.append(value)
                continue
            try:
                path.resolve().relative_to(root)
            except ValueError:
                stale_for_entry.append(value)
        if status == ACTION_STATUS_SUPERSEDED or entry.get("superseded_by"):
            superseded_ids.append(runbook_id)
        if str(entry.get("retention_decision") or "") == "archive" or bool(entry.get("archived_at")):
            archived_ids.append(runbook_id)
        mutates = bool(entry.get("mutates_registry", False))
        integrated = bool(entry.get("integrated_opt_in_required", False)) or str(entry.get("safety_level") or "") == "integrated_opt_in"
        has_guardrail = integrated and not bool(entry.get("executable", True))
        if mutates and not has_guardrail:
            mutation_risk_ids.append(runbook_id)
        if integrated and has_guardrail:
            integrated_blocked_ids.append(runbook_id)
        if active and missing_for_entry:
            missing_inputs.extend(f"{runbook_id}:{path}" for path in missing_for_entry)
        elif missing_for_entry:
            stale_but_safe_ids.append(runbook_id)
        if active and stale_for_entry:
            stale_inputs.extend(f"{runbook_id}:{path}" for path in stale_for_entry)
        elif stale_for_entry:
            stale_but_safe_ids.append(runbook_id)
        ledger_entry = ledger_by_id.get(action_id, {})
        replayable = bool(related_paths) and not (active and missing_for_entry)
        if ledger_entry and str(ledger_entry.get("action_status") or "") == ACTION_STATUS_SUPERSEDED:
            superseded_ids.append(runbook_id)
            replayable = False
        if replayable:
            replayable_ids.append(runbook_id)
        else:
            non_replayable_ids.append(runbook_id)

    if missing_inputs or mutation_risk_ids:
        verdict = "ACTION_REQUIRED"
    elif stale_inputs or superseded_ids or stale_but_safe_ids:
        verdict = "WARN"
    else:
        verdict = "PASS"
    payload = {
        "manifest_version": "operator_runbook_lifecycle_validation_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "validation_verdict": verdict,
        "missing_input_count": len(missing_inputs),
        "stale_input_count": len(stale_inputs),
        "superseded_runbook_count": len(set(superseded_ids)),
        "archived_runbook_count": len(set(archived_ids)),
        "replayable_runbook_count": len(set(replayable_ids)),
        "non_replayable_runbook_count": len(set(non_replayable_ids)),
        "mutation_risk_count": len(set(mutation_risk_ids)),
        "integrated_opt_in_blocked_count": len(set(integrated_blocked_ids)),
        "stale_but_safe_count": len(set(stale_but_safe_ids)),
        "affected_action_ids": sorted(set([item.split(":", 1)[0] for item in missing_inputs + stale_inputs] + mutation_risk_ids)),
        "missing_inputs": missing_inputs,
        "stale_inputs": stale_inputs,
        "superseded_runbook_ids": sorted(set(superseded_ids)),
        "archived_runbook_ids": sorted(set(archived_ids)),
        "mutation_risk_runbook_ids": sorted(set(mutation_risk_ids)),
        "integrated_opt_in_blocked_runbook_ids": sorted(set(integrated_blocked_ids)),
        "stale_but_safe_runbook_ids": sorted(set(stale_but_safe_ids)),
        "rationale": [
            "active executable runbooks with missing critical inputs require operator action",
            "stale or archived resolved runbooks are retained as non-blocking history unless they can mutate state",
        ],
    }
    payload["validation_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


PATH_FIELD_HINTS = (
    "path",
    "manifest",
    "bundle",
    "zip",
    "report",
    "ledger",
    "catalog",
    "summary",
    "baseline",
    "registry",
    "runbook",
    "execution",
    "apply",
)


def looks_like_artifact_pointer(value: str) -> bool:
    text = value.strip()
    if not text or text.startswith(("http://", "https://")):
        return False
    if "\n" in text:
        return False
    if text.startswith(("/", "artifacts/", "manifests/")):
        return True
    if "/" in text and any(text.endswith(suffix) for suffix in (".json", ".txt", ".zip", ".log")):
        return True
    return False


def pointer_field_is_path_like(field_path: str, value: str) -> bool:
    lower = field_path.lower()
    if not looks_like_artifact_pointer(value):
        return False
    if "related_manifests" in lower:
        return True
    return any(hint in lower for hint in PATH_FIELD_HINTS)


def iter_artifact_pointers(obj: Any, *, prefix: str = "") -> list[tuple[str, str]]:
    pointers: list[tuple[str, str]] = []
    if isinstance(obj, dict):
        for key, value in obj.items():
            child_prefix = f"{prefix}.{key}" if prefix else str(key)
            if isinstance(value, str) and pointer_field_is_path_like(child_prefix, value):
                pointers.append((child_prefix, value))
            elif isinstance(value, (dict, list)):
                pointers.extend(iter_artifact_pointers(value, prefix=child_prefix))
    elif isinstance(obj, list):
        for index, value in enumerate(obj):
            child_prefix = f"{prefix}[{index}]"
            if isinstance(value, str) and pointer_field_is_path_like(child_prefix, value):
                pointers.append((child_prefix, value))
            elif isinstance(value, (dict, list)):
                pointers.extend(iter_artifact_pointers(value, prefix=child_prefix))
    return pointers


def classify_pointer_path(value: str, *, published_root: Path, artifact_root: Path) -> tuple[str, bool]:
    path = Path(value)
    if value.startswith(("/private/tmp/", "/tmp/")):
        return "absolute_tmp", path.exists()
    if path.is_absolute():
        exists = path.exists()
        try:
            path.resolve().relative_to(published_root.resolve())
            return "published_abs", exists
        except ValueError:
            pass
        try:
            path.resolve().relative_to(artifact_root.resolve())
            return "authoritative_abs", exists
        except ValueError:
            pass
        return ("authoritative_abs" if exists else "missing"), exists
    if value.startswith(("manifests/", "reports/", "curated/", "light_ops/")):
        return "bundle_relative", (artifact_root / value).exists() or (published_root / value).exists()
    if value.startswith("artifacts/"):
        return "artifact_root_relative", Path(value).exists()
    return "unknown", (artifact_root / value).exists() or (published_root / value).exists()


def candidate_rewrite_paths(value: str, *, published_root: Path, artifact_root: Path) -> list[dict[str, Any]]:
    path = Path(value)
    basename = path.name
    candidates: list[dict[str, Any]] = []
    if not basename:
        return candidates
    candidate_specs = [
        ("artifact_root", artifact_root / "manifests" / basename, f"manifests/{basename}"),
        ("artifact_root", artifact_root / basename, basename),
        ("published_root", published_root / "manifests" / basename, f"manifests/{basename}"),
    ]
    for root_kind, candidate, rewrite in candidate_specs:
        if candidate.exists():
            candidates.append(
                {
                    "root_kind": root_kind,
                    "candidate_path": rewrite,
                    "rewrite_path": rewrite,
                    "candidate_hash": sha256_file(candidate),
                }
            )
    if published_root.exists():
        for candidate in sorted(published_root.rglob(basename))[:3]:
            rewrite = str(candidate.relative_to(published_root))
            if not any(item["rewrite_path"] == rewrite and item["root_kind"] == "published_root" for item in candidates):
                candidates.append(
                    {
                        "root_kind": "published_root",
                        "candidate_path": rewrite,
                        "rewrite_path": rewrite,
                        "candidate_hash": sha256_file(candidate),
                    }
                )
    return candidates


def ledger_entry_is_active(entry: dict[str, Any]) -> bool:
    status = str(entry.get("action_status") or "")
    closure = str(entry.get("closure_status") or "")
    return status in {ACTION_STATUS_PLANNED, ACTION_STATUS_EXECUTED, ACTION_STATUS_FAILED, ACTION_STATUS_DEFERRED, ACTION_STATUS_RETRY_PENDING} or closure in {
        CLOSURE_STATUS_OPEN,
        CLOSURE_STATUS_DEFERRED,
        CLOSURE_STATUS_RETRY_PENDING,
    }


def build_operator_runbook_pointer_audit(
    *,
    phase: str,
    runbook_catalog: dict[str, Any],
    action_ledger: dict[str, Any],
    published_root: Path,
    artifact_root: Path,
) -> dict[str, Any]:
    ledger_by_id = ledger_actions_by_id(action_ledger)
    audit_entries: list[dict[str, Any]] = []

    def add_pointer_entry(
        *,
        source_kind: str,
        source_id: str,
        action_id: str,
        pointer_field: str,
        original_path: str,
        active: bool,
    ) -> None:
        path_kind, exists_now = classify_pointer_path(original_path, published_root=published_root, artifact_root=artifact_root)
        candidates = candidate_rewrite_paths(original_path, published_root=published_root, artifact_root=artifact_root)
        selected = candidates[0]["rewrite_path"] if candidates else None
        can_rewrite = bool(selected)
        archive_waiver_allowed = (not active) and (not can_rewrite)
        severity = "INFO"
        rationale = "pointer is already resolvable"
        if path_kind == "absolute_tmp":
            severity = "WARN" if can_rewrite or archive_waiver_allowed else "ACTION_REQUIRED"
            rationale = "ephemeral /tmp pointer must be rewritten or waived if closed historical provenance"
        if not exists_now and not can_rewrite:
            severity = "ACTION_REQUIRED" if active else "WARN"
            rationale = "active missing pointer blocks lifecycle validation" if active else "closed historical pointer can be waived with retained summary/hash"
        if active and not can_rewrite and not exists_now:
            severity = "FAIL"
        audit_entries.append(
            {
                "source_kind": source_kind,
                "runbook_id": source_id if source_kind == "runbook_catalog" else None,
                "action_id": action_id,
                "pointer_field": pointer_field,
                "original_path": original_path,
                "path_kind": path_kind,
                "exists_now": exists_now,
                "expected_artifact_kind": Path(original_path).name or "unknown",
                "can_rewrite": can_rewrite,
                "candidate_rewrite_paths": candidates,
                "selected_rewrite_path": selected,
                "archive_waiver_allowed": archive_waiver_allowed,
                "severity": severity,
                "rationale": rationale,
            }
        )

    for entry in [item for item in runbook_catalog.get("entries", []) if isinstance(item, dict)]:
        runbook_id = str(entry.get("runbook_id") or entry.get("action_id") or "")
        action_id = str(entry.get("action_id") or "")
        active = runbook_is_active(entry)
        for field_path, value in iter_artifact_pointers(entry):
            add_pointer_entry(
                source_kind="runbook_catalog",
                source_id=runbook_id,
                action_id=action_id,
                pointer_field=f"entries[{runbook_id}].{field_path}",
                original_path=value,
                active=active,
            )
    for entry in [item for item in action_ledger.get("entries", []) if isinstance(item, dict)]:
        action_id = str(entry.get("action_id") or "")
        active = ledger_entry_is_active(entry)
        linked_runbook = next(
            (str(item.get("runbook_id") or "") for item in runbook_catalog.get("entries", []) if str(item.get("action_id") or "") == action_id),
            "",
        )
        for field_path, value in iter_artifact_pointers(entry):
            add_pointer_entry(
                source_kind="action_ledger",
                source_id=linked_runbook,
                action_id=action_id,
                pointer_field=f"entries[{action_id}].{field_path}",
                original_path=value,
                active=active,
            )

    severity_order = {"INFO": 0, "WARN": 1, "ACTION_REQUIRED": 2, "FAIL": 3}
    highest = max((severity_order.get(str(item.get("severity")), 0) for item in audit_entries), default=0)
    audit_verdict = "PASS"
    if highest >= 3:
        audit_verdict = "FAIL"
    elif highest >= 2:
        audit_verdict = "ACTION_REQUIRED"
    elif highest >= 1:
        audit_verdict = "WARN"
    payload = {
        "manifest_version": "operator_runbook_pointer_audit_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "audit_verdict": audit_verdict,
        "pointer_count": len(audit_entries),
        "absolute_tmp_pointer_count": sum(1 for item in audit_entries if item.get("path_kind") == "absolute_tmp"),
        "rewritable_pointer_count": sum(1 for item in audit_entries if item.get("can_rewrite")),
        "waivable_archived_pointer_count": sum(1 for item in audit_entries if item.get("archive_waiver_allowed")),
        "unresolved_active_pointer_count": sum(1 for item in audit_entries if item.get("severity") == "FAIL"),
        "entries": audit_entries,
        "rationale": [
            "ephemeral /tmp pointers are provenance debt and must not remain in active runbook inputs",
            "closed historical pointers may be waived only when the operational summary/hash is retained",
        ],
    }
    payload["audit_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def rewrite_artifact_pointers(obj: Any, rewrite_map: dict[str, str]) -> Any:
    if isinstance(obj, dict):
        return {key: rewrite_artifact_pointers(value, rewrite_map) for key, value in obj.items()}
    if isinstance(obj, list):
        return [rewrite_artifact_pointers(value, rewrite_map) for value in obj]
    if isinstance(obj, str) and obj in rewrite_map:
        return rewrite_map[obj]
    return obj


def build_operator_runbook_provenance_migration(
    *,
    phase: str,
    runbook_catalog: dict[str, Any],
    action_ledger: dict[str, Any],
    pointer_audit: dict[str, Any],
    allow_archived_waiver: bool,
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    source_catalog_hash = runbook_catalog.get("catalog_hash") or sha256_text(json.dumps(runbook_catalog, sort_keys=True))
    source_ledger_hash = action_ledger.get("ledger_hash") or sha256_text(json.dumps(action_ledger, sort_keys=True))
    rewrite_map: dict[str, str] = {}
    records: list[dict[str, Any]] = []
    waived = 0
    unresolved_active = 0
    unresolved_archived = 0

    for entry in [item for item in pointer_audit.get("entries", []) if isinstance(item, dict)]:
        original = str(entry.get("original_path") or "")
        selected = str(entry.get("selected_rewrite_path") or "")
        record = dict(entry)
        if selected:
            rewrite_map[original] = selected
            record["migration_action"] = "rewrite"
        elif bool(entry.get("archive_waiver_allowed")) and allow_archived_waiver:
            waived += 1
            waiver_token = f"waived_archived_pointer:{sha256_text(original)[:16]}"
            rewrite_map[original] = waiver_token
            record["migration_action"] = "waive_archived"
            record["waiver_token"] = waiver_token
            record["waiver_reason"] = "closed historical pointer retained only as provenance summary"
        elif str(entry.get("severity")) == "FAIL":
            unresolved_active += 1
            record["migration_action"] = "unresolved_active"
        else:
            unresolved_archived += 1
            record["migration_action"] = "unresolved_archived"
        records.append(record)

    migrated_catalog = rewrite_artifact_pointers(runbook_catalog, rewrite_map)
    migrated_ledger = rewrite_artifact_pointers(action_ledger, rewrite_map)
    migrated_catalog["manifest_version"] = "operator_runbook_catalog_migrated_v1"
    migrated_catalog["phase"] = phase
    migrated_catalog["generated_at_utc"] = runtime_gate.timestamp_utc_now()
    migrated_catalog["source_catalog_hash"] = source_catalog_hash
    migrated_catalog["provenance_migration_applied"] = True
    migrated_catalog["rewritten_pointer_count"] = len(set(rewrite_map))
    migrated_catalog["waived_archived_pointer_count"] = waived
    migrated_catalog["catalog_hash"] = sha256_text(json.dumps(migrated_catalog, sort_keys=True))

    migrated_ledger["manifest_version"] = "runtime_current_env_action_ledger_migrated_v1"
    migrated_ledger["phase"] = phase
    migrated_ledger["generated_at_utc"] = runtime_gate.timestamp_utc_now()
    migrated_ledger["source_ledger_hash"] = source_ledger_hash
    migrated_ledger["provenance_migration_applied"] = True
    migrated_ledger["rewritten_pointer_count"] = len(set(rewrite_map))
    migrated_ledger["waived_archived_pointer_count"] = waived
    migrated_ledger["ledger_hash"] = sha256_text(json.dumps(migrated_ledger, sort_keys=True))

    if unresolved_active:
        verdict = "FAIL"
    elif unresolved_archived:
        verdict = "ACTION_REQUIRED"
    elif waived:
        verdict = "PASS_WITH_WAIVERS"
    else:
        verdict = "PASS"
    report = {
        "manifest_version": "operator_runbook_provenance_migration_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "migration_id": f"{phase}-runbook-provenance-migration",
        "source_catalog_hash": source_catalog_hash,
        "source_ledger_hash": source_ledger_hash,
        "migrated_catalog_hash": migrated_catalog.get("catalog_hash"),
        "migrated_ledger_hash": migrated_ledger.get("ledger_hash"),
        "rewritten_pointer_count": len(set(rewrite_map)),
        "waived_archived_pointer_count": waived,
        "unresolved_active_pointer_count": unresolved_active,
        "unresolved_archived_pointer_count": unresolved_archived,
        "migration_verdict": verdict,
        "per_pointer": records,
        "rationale": [
            "rewritable pointers are normalized to artifact-root or published-root relative paths",
            "closed historical pointers without a surviving artifact can be waived only with allow-archived-waiver",
        ],
    }
    report["migration_hash"] = sha256_text(json.dumps(report, sort_keys=True))
    return migrated_catalog, migrated_ledger, report


def original_provenance_field(field_path: str) -> bool:
    lower = field_path.lower()
    return any(
        token in lower
        for token in (
            "original_path",
            "candidate_path",
            "source_path",
            "source_root",
            "authoritative_root",
            "published_root",
            "staged_root",
        )
    )


def build_operator_artifact_path_policy_lint(
    *,
    phase: str,
    manifest_root: Path,
    published_root: Path,
) -> dict[str, Any]:
    def phase51_policy_target(path: Path) -> bool:
        name = path.name
        if name == "bundle_metadata.json":
            return True
        if phase not in name:
            return False
        if "migrated" in name or "provenance_migration" in name or "pointer_audit" in name:
            return True
        if "lifecycle_validation" in name and ("_after" in name or "after_migration" in name):
            return True
        if "path_policy_lint" in name or "policy_ops_summary" in name:
            return True
        return False

    if manifest_root.is_file():
        files = [manifest_root]
    else:
        files = [
            path
            for path in sorted(manifest_root.rglob("*.json"))
            if phase51_policy_target(path)
        ]
    if manifest_root.is_file():
        artifact_base = manifest_root.parent.parent if manifest_root.parent.name == "manifests" else manifest_root.parent
        manifest_dir = manifest_root.parent
    else:
        artifact_base = manifest_root.parent if manifest_root.name == "manifests" else manifest_root
        manifest_dir = manifest_root
    forbidden: list[dict[str, Any]] = []
    dangling: list[dict[str, Any]] = []
    allowed_external = 0
    scanned = 0
    for path in files:
        try:
            payload = read_json(path)
        except Exception:
            continue
        scanned += 1
        for field_path, value in iter_artifact_pointers(payload):
            if value.startswith(("/private/tmp/", "/tmp/")):
                if original_provenance_field(field_path):
                    allowed_external += 1
                else:
                    forbidden.append({"manifest": str(path), "field": field_path, "path": value, "reason": "ephemeral tmp path"})
                continue
            pointer = Path(value)
            if pointer.is_absolute():
                try:
                    pointer.resolve().relative_to(manifest_root.resolve())
                    continue
                except ValueError:
                    pass
                try:
                    pointer.resolve().relative_to(published_root.resolve())
                    continue
                except ValueError:
                    pass
                if original_provenance_field(field_path):
                    allowed_external += 1
                else:
                    forbidden.append({"manifest": str(path), "field": field_path, "path": value, "reason": "absolute path outside allowed roots"})
                continue
            if value.startswith("artifacts/"):
                candidate = Path(value)
            elif value.startswith("manifests/"):
                candidate = artifact_base / value
            else:
                candidate = manifest_dir / value
            if not candidate.exists() and not (published_root / value).exists():
                dangling.append({"manifest": str(path), "field": field_path, "path": value, "reason": "relative path does not resolve"})
    verdict = "PASS"
    if forbidden:
        verdict = "FAIL"
    elif dangling:
        verdict = "ACTION_REQUIRED"
    payload = {
        "manifest_version": "operator_artifact_path_policy_lint_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "lint_verdict": verdict,
        "scanned_manifest_count": scanned,
        "forbidden_path_count": len(forbidden),
        "dangling_path_count": len(dangling),
        "allowed_external_reference_count": allowed_external,
        "forbidden_paths": forbidden,
        "dangling_paths": dangling,
        "rationale": [
            "active operator artifacts must use bundle-relative, artifact-root-relative, or published-root-relative pointers",
            "ephemeral tmp paths are allowed only inside explicit provenance original_path/source_path fields",
        ],
    }
    payload["lint_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_integrated_approval_mutation_audit(
    *,
    phase: str,
    approval_execution: dict[str, Any],
    runtime_budget_registry: dict[str, Any],
    runtime_budget_baseline: dict[str, Any],
) -> dict[str, Any]:
    mode = str(approval_execution.get("approval_execution_mode") or approval_execution.get("approval_mode") or APPROVAL_EXECUTION_MODE_HANDOFF_ONLY)
    allow_integrated = bool(approval_execution.get("allow_integrated_approval", False))
    token_present = bool(approval_execution.get("approval_confirmation_token_present", False))
    preflight_success = bool(approval_execution.get("dry_run_preflight_success", False))
    registry_before_hash = approval_execution.get("registry_before_hash") or sha256_text(json.dumps(runtime_budget_registry, sort_keys=True))
    registry_after_hash = approval_execution.get("registry_after_hash") or registry_before_hash
    baseline_before_hash = sha256_text(json.dumps(runtime_budget_baseline, sort_keys=True)) if runtime_budget_baseline else None
    baseline_after_hash = approval_execution.get("baseline_written_hash") or baseline_before_hash
    mutation_expected = mode == APPROVAL_EXECUTION_MODE_INTEGRATED_OPT_IN and str(approval_execution.get("approval_status") or "") == APPROVAL_STATUS_APPLIED
    mutation_observed = bool(approval_execution.get("registry_updated", False) or approval_execution.get("baseline_written", False) or registry_before_hash != registry_after_hash or baseline_before_hash != baseline_after_hash)
    unexpected_mutations: list[str] = []
    if mode in {APPROVAL_EXECUTION_MODE_DRY_RUN, APPROVAL_EXECUTION_MODE_HANDOFF_ONLY} and mutation_observed:
        unexpected_mutations.append("dry_run/handoff_only must not mutate registry or baseline")
    if mode == APPROVAL_EXECUTION_MODE_INTEGRATED_OPT_IN:
        if not allow_integrated:
            unexpected_mutations.append("integrated approval missing explicit opt-in")
        if not token_present:
            unexpected_mutations.append("integrated approval missing confirmation token")
        if not preflight_success and mutation_expected:
            unexpected_mutations.append("integrated approval mutation missing successful dry-run preflight")
        if mutation_observed and not approval_execution.get("approval_transaction_id"):
            unexpected_mutations.append("registry/baseline mutation missing approval transaction metadata")
    payload = {
        "manifest_version": "integrated_approval_mutation_audit_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "approval_execution_id": approval_execution.get("approval_execution_id"),
        "approval_mode": approval_execution.get("approval_mode"),
        "approval_execution_mode": mode,
        "allow_integrated_approval": allow_integrated,
        "confirmation_token_present": token_present,
        "preflight_success": preflight_success,
        "registry_before_hash": registry_before_hash,
        "registry_after_hash": registry_after_hash,
        "baseline_before_hash": baseline_before_hash,
        "baseline_after_hash": baseline_after_hash,
        "mutation_expected": mutation_expected,
        "mutation_observed": mutation_observed,
        "unexpected_mutation_count": len(unexpected_mutations),
        "unexpected_mutations": unexpected_mutations,
        "rollback_hint": approval_execution.get("rollback_hint") or "no rollback needed unless mutation audit fails",
        "audit_verdict": "PASS" if not unexpected_mutations else "FAIL",
    }
    payload["audit_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_source_health_action_plan(
    *,
    phase: str,
    source_health: dict[str, Any],
    staged_materialization: dict[str, Any],
) -> dict[str, Any]:
    entries: list[dict[str, Any]] = []
    dataless_count = int(source_health.get("dataless_placeholder_count", 0) or 0)
    unreadable_count = int(source_health.get("unreadable_source_file_count", 0) or 0)
    missing_git_object = bool(source_health.get("git_object_health", {}).get("missing_head_object", False))
    required_fixtures_present = bool(source_health.get("required_fixtures_present", True))
    staged_verdict = str(staged_materialization.get("materialization_verdict") or source_health.get("staged_materialization", {}).get("materialization_verdict") or "")
    staged_mode = str(staged_materialization.get("staged_materialization_mode") or source_health.get("staged_materialization", {}).get("staged_materialization_mode") or "")

    def add_entry(issue_kind: str, severity: str, action: str, command: str, direct_blocked: bool, staged_blocked: bool, expected: str, rationale: str) -> None:
        entries.append(
            {
                "issue_kind": issue_kind,
                "severity": severity,
                "recommended_action": action,
                "recommended_command": command,
                "blocking_for_direct_build": direct_blocked,
                "blocking_for_staged_build": staged_blocked,
                "expected_resolution": expected,
                "rationale": rationale,
            }
        )

    if dataless_count > 0:
        add_entry(
            "dataless_placeholder",
            "ACTION_REQUIRED",
            "USE_STAGED_SPARSE_CLONE_OVERLAY",
            "python tests/tools/run_policy_pipeline.py --include-source-health --pipeline-phase phase50 ...",
            True,
            False,
            "materialize authoritative source through staged_sparse_clone_overlay before long verification",
            "dataless placeholders make direct long build/test invalid",
        )
    if missing_git_object:
        add_entry(
            "missing_git_object",
            "WARN",
            "AVOID_AUTHORITATIVE_GIT_OBJECT_DEPENDENCE",
            "git fetch origin main --filter=blob:none",
            True,
            False,
            "use sparse clone overlay or refresh git objects before direct git operations",
            "HEAD object is unavailable in the authoritative iCloud tree",
        )
    if unreadable_count > 0:
        add_entry(
            "unreadable_source",
            "ACTION_REQUIRED",
            "SPARSE_CLONE_REQUIRED",
            "git clone --filter=blob:none --sparse <repo> /tmp/raw_engine_sparse_ref",
            True,
            True,
            "restore readable source files before verification",
            "required source files are unreadable",
        )
    if not required_fixtures_present:
        add_entry(
            "missing_fixture",
            "FAIL",
            "RESTORE_REQUIRED_FIXTURES",
            "git checkout origin/main -- tests/campaigns tests/fixtures",
            True,
            True,
            "required fixtures must exist before staged verification",
            "missing fixtures block both direct and staged verification",
        )
    if staged_mode and staged_verdict and staged_verdict != "PASS":
        add_entry(
            "stale_overlay",
            "ACTION_REQUIRED",
            "REFRESH_STAGED_OVERLAY",
            "python tests/tools/source_health_preflight.py --staged-materialization-out <path> ...",
            True,
            True,
            "rerun staged materialization transaction",
            "staged materialization verdict is not PASS",
        )
    direct_blocked = any(bool(entry.get("blocking_for_direct_build", False)) for entry in entries)
    staged_blocked = any(bool(entry.get("blocking_for_staged_build", False)) for entry in entries)
    recommended_action = "NO_ACTION"
    if staged_blocked:
        recommended_action = "RESTORE_SOURCE_OR_FIXTURES"
    elif direct_blocked:
        recommended_action = "USE_STAGED_SPARSE_CLONE_OVERLAY"
    payload = {
        "manifest_version": "source_health_action_plan_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "plan_verdict": "PASS" if not staged_blocked else "ACTION_REQUIRED",
        "source_health_status": source_health.get("status", "NOT_RUN"),
        "source_health_recommendation": source_health.get("recommendation", "NOT_RUN"),
        "direct_build_blocked": direct_blocked,
        "staged_build_allowed": not staged_blocked,
        "recommended_action": recommended_action,
        "materialization_mode": staged_mode,
        "issue_count": len(entries),
        "entries": entries,
        "rationale": [
            "source-health action planning is infra/operator guidance, not a correctness failure",
            "direct long build/test is invalid when source-health requires staged sparse materialization",
        ],
    }
    payload["plan_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_operator_decision_metadata_audit(
    *,
    phase: str,
    action_ledger: dict[str, Any],
    runbook_catalog: dict[str, Any],
) -> dict[str, Any]:
    missing_reason: list[str] = []
    missing_defer_until: list[str] = []
    missing_retry_link: list[str] = []
    missing_approval_pointer: list[str] = []
    affected: set[str] = set()
    for entry in runbook_catalog.get("entries", []):
        if not isinstance(entry, dict):
            continue
        action_id = str(entry.get("action_id") or entry.get("runbook_id") or "")
        runbook_type = str(entry.get("runbook_type") or "")
        if runbook_type in {"skip", "reject"} and not bool(entry.get("retained_reason_metadata", False)):
            missing_reason.append(action_id)
            affected.add(action_id)
        if runbook_type == "defer":
            if not bool(entry.get("retained_reason_metadata", False)):
                missing_reason.append(action_id)
                affected.add(action_id)
            if not bool(entry.get("retained_defer_metadata", False)):
                missing_defer_until.append(action_id)
                affected.add(action_id)
        if runbook_type == "retry" and not bool(entry.get("retained_retry_metadata", False)):
            missing_retry_link.append(action_id)
            affected.add(action_id)
        if runbook_type == "approval" and not bool(entry.get("retained_approval_pointers", False)):
            missing_approval_pointer.append(action_id)
            affected.add(action_id)
    for entry in action_ledger.get("entries", []):
        if not isinstance(entry, dict):
            continue
        action_id = str(entry.get("action_id") or "")
        status = str(entry.get("action_status") or "")
        if status == ACTION_STATUS_DEFERRED and not str(entry.get("defer_until") or "").strip():
            missing_defer_until.append(action_id)
            affected.add(action_id)
        if status == ACTION_STATUS_RETRY_PENDING and not (entry.get("retry_action_id") or entry.get("next_action_id") or int(entry.get("retry_count", 0) or 0) > 0):
            missing_retry_link.append(action_id)
            affected.add(action_id)
    payload = {
        "manifest_version": "operator_decision_metadata_audit_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "audit_verdict": "PASS"
        if not (missing_reason or missing_defer_until or missing_retry_link or missing_approval_pointer)
        else "FAIL",
        "missing_reason_count": len(missing_reason),
        "missing_defer_until_count": len(missing_defer_until),
        "missing_retry_link_count": len(missing_retry_link),
        "missing_approval_pointer_count": len(missing_approval_pointer),
        "affected_action_ids": sorted(affected),
        "missing_reason_action_ids": sorted(set(missing_reason)),
        "missing_defer_until_action_ids": sorted(set(missing_defer_until)),
        "missing_retry_link_action_ids": sorted(set(missing_retry_link)),
        "missing_approval_pointer_action_ids": sorted(set(missing_approval_pointer)),
    }
    payload["audit_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_operator_runbook_replay(
    *,
    phase: str,
    runbook: dict[str, Any],
    action_ledger: dict[str, Any],
    runtime_current_manifest: dict[str, Any],
    runtime_budget_registry: dict[str, Any],
    replay_mode: str,
) -> dict[str, Any]:
    entries = runbook.get("entries") if isinstance(runbook.get("entries"), list) else [runbook]
    replay_entries: list[dict[str, Any]] = []
    missing_input_count = 0
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        required = list(entry.get("required_inputs", []))
        abstract_missing = [
            value
            for value in required
            if str(value).strip()
            and str(value).startswith("/")
            and not Path(str(value)).exists()
        ]
        missing_input_count += len(abstract_missing)
        safety_level = str(entry.get("safety_level") or ("integrated_opt_in" if entry.get("approval_mode") == "integrated" else "handoff_only"))
        replay_entries.append(
            {
                "runbook_id": entry.get("runbook_id"),
                "action_id": entry.get("action_id"),
                "runbook_type": entry.get("runbook_type") or runbook_type_for_action(str(entry.get("action_kind") or "")),
                "command_still_valid": bool(entry.get("recommended_command") or replay_mode == "validate_only") and not abstract_missing,
                "would_mutate_registry": bool(entry.get("mutates_registry", False)),
                "requires_operator_confirmation": safety_level == "integrated_opt_in",
                "missing_inputs": abstract_missing,
                "ledger_present": bool(action_ledger),
                "runtime_current_present": bool(runtime_current_manifest),
                "runtime_budget_registry_present": bool(runtime_budget_registry),
            }
        )
    payload = {
        "manifest_version": "operator_runbook_replay_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "replay_mode": replay_mode,
        "replay_verdict": "PASS" if missing_input_count == 0 and all(bool(entry.get("command_still_valid")) for entry in replay_entries) else "FAIL",
        "missing_input_count": missing_input_count,
        "stale_input_count": 0,
        "command_still_valid": all(bool(entry.get("command_still_valid")) for entry in replay_entries),
        "would_mutate_registry": any(bool(entry.get("would_mutate_registry")) for entry in replay_entries),
        "requires_operator_confirmation": any(bool(entry.get("requires_operator_confirmation")) for entry in replay_entries),
        "entries": replay_entries,
        "rationale": [
            "runbook replay validates operator command metadata without mutating registry state",
        ],
    }
    payload["replay_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_staged_materialization_transaction(
    *,
    phase: str,
    source_health: dict[str, Any],
    staged_materialization: dict[str, Any],
    cleanup_path_values: list[str],
) -> dict[str, Any]:
    source_health_hash = source_health.get("preflight_hash") or sha256_text(json.dumps(source_health, sort_keys=True))
    materialization_hash = staged_materialization.get("materialization_hash") or sha256_text(json.dumps(staged_materialization, sort_keys=True))
    cleanup_paths = [str(Path(value).resolve()) for value in cleanup_path_values if str(value).strip()]
    payload = {
        "manifest_version": "staged_materialization_transaction_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "transaction_id": f"{phase}-materialization-{sha256_text(str(source_health_hash) + str(materialization_hash))[:12]}",
        "materialization_mode": staged_materialization.get("staged_materialization_mode"),
        "source_health_hash": source_health_hash,
        "sparse_clone_ref": staged_materialization.get("sparse_clone_ref"),
        "overlay_file_count": int(staged_materialization.get("overlay_file_count", 0) or 0),
        "overlay_hash": staged_materialization.get("overlay_hash"),
        "dataless_remaining_count": int(staged_materialization.get("dataless_remaining_count", 0) or 0),
        "source_snapshot_hash": staged_materialization.get("source_snapshot_hash"),
        "staged_mirror_hash": staged_materialization.get("staged_mirror_hash") or staged_materialization.get("materialization_hash"),
        "transaction_verdict": "PASS" if staged_materialization.get("materialization_verdict") in {"PASS", "HEALTHY", None} else "FAIL",
        "rollback_cleanup_performed": bool(cleanup_paths),
        "cleanup_paths": cleanup_paths,
    }
    payload["transaction_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_action_ledger_closure_invariants(
    *,
    phase: str,
    action_ledger: dict[str, Any],
    compacted_ledger: dict[str, Any],
    ledger_archive: dict[str, Any],
) -> dict[str, Any]:
    failures: list[str] = []
    entries = list(action_ledger.get("entries", []))
    compact_entries = list(compacted_ledger.get("compacted_ledger", {}).get("entries", []))
    archived_entries = list(ledger_archive.get("archived_entries", []))
    retained_ids = {str(item.get("action_id") or "") for item in compact_entries if isinstance(item, dict)}
    archived_ids = {str(item.get("action_id") or "") for item in archived_entries if isinstance(item, dict)}
    for item in entries:
        if not isinstance(item, dict):
            continue
        action_id = str(item.get("action_id") or "")
        status = str(item.get("action_status") or "")
        closure = str(item.get("closure_status") or "")
        if status == ACTION_STATUS_DEFERRED and not item.get("defer_until"):
            failures.append(f"{action_id}: DEFERRED action missing defer_until")
        if status == ACTION_STATUS_RETRY_PENDING and not (item.get("next_retry_action_id") or item.get("retry_count") is not None):
            failures.append(f"{action_id}: RETRY_PENDING action missing retry pointer/count")
        if status == ACTION_STATUS_SUPERSEDED and not item.get("superseded_by"):
            failures.append(f"{action_id}: SUPERSEDED action missing superseded_by")
        if status == ACTION_STATUS_FAILED and action_id not in retained_ids and action_id not in archived_ids:
            failures.append(f"{action_id}: FAILED action disappeared from retained/archive ledgers")
        if item.get("approval_execution_id") and not (item.get("approval_status") or item.get("approval_execution_manifest")):
            failures.append(f"{action_id}: approval action missing approval execution/status pointer")
        if closure == CLOSURE_STATUS_CLOSED and status not in {
            ACTION_STATUS_APPLIED,
            ACTION_STATUS_EXECUTED,
            ACTION_STATUS_CLOSED,
            ACTION_STATUS_SKIPPED,
            ACTION_STATUS_REJECTED,
        }:
            failures.append(f"{action_id}: CLOSED closure has non-terminal action status {status}")
    payload = {
        "manifest_version": "runtime_current_env_action_ledger_closure_invariants_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "checked_action_count": len(entries),
        "failed_action_count": sum(1 for item in entries if isinstance(item, dict) and item.get("action_status") == ACTION_STATUS_FAILED),
        "retry_pending_count": sum(1 for item in entries if isinstance(item, dict) and item.get("action_status") == ACTION_STATUS_RETRY_PENDING),
        "approval_pointer_count": sum(1 for item in entries if isinstance(item, dict) and item.get("approval_execution_id")),
        "retained_action_count": len(retained_ids),
        "archived_action_count": len(archived_ids),
        "invariant_failure_count": len(failures),
        "invariant_verdict": "PASS" if not failures else "FAIL",
        "failures": failures,
    }
    payload["invariant_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_policy_ops_agenda(
    *,
    phase: str,
    current_env_due: dict[str, Any],
    current_env_watch_plan: dict[str, Any],
    current_env_reproposal_plan: dict[str, Any],
    runtime_registry_health: dict[str, Any],
    known_env_reverify_plan: dict[str, Any],
    known_env_retire_plan: dict[str, Any],
    foreign_import_summaries: list[dict[str, Any]],
    publication_health: dict[str, Any],
    current_env_watch_execute: dict[str, Any] | None = None,
    current_env_watch_apply: dict[str, Any] | None = None,
    current_env_reproposal_execute: dict[str, Any] | None = None,
    current_env_action_ledger: dict[str, Any] | None = None,
    current_env_retry_plan: dict[str, Any] | None = None,
    current_env_reproposal_handoff: dict[str, Any] | None = None,
    current_env_operator_decision: dict[str, Any] | None = None,
    current_env_operator_decision_apply: dict[str, Any] | None = None,
    current_env_action_ledger_compact: dict[str, Any] | None = None,
    current_env_action_ledger_archive: dict[str, Any] | None = None,
    current_env_approval_runbook: dict[str, Any] | None = None,
    current_env_approval_execution: dict[str, Any] | None = None,
    current_env_approval_link: dict[str, Any] | None = None,
    current_time_override: str | None = None,
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override)
    current_env_watch_execute = current_env_watch_execute or {}
    current_env_watch_apply = current_env_watch_apply or {}
    current_env_reproposal_execute = current_env_reproposal_execute or {}
    current_env_action_ledger = current_env_action_ledger or {}
    current_env_retry_plan = current_env_retry_plan or {}
    current_env_reproposal_handoff = current_env_reproposal_handoff or {}
    current_env_operator_decision = current_env_operator_decision or {}
    current_env_operator_decision_apply = current_env_operator_decision_apply or {}
    current_env_action_ledger_compact = current_env_action_ledger_compact or {}
    current_env_action_ledger_archive = current_env_action_ledger_archive or {}
    current_env_approval_runbook = current_env_approval_runbook or {}
    current_env_approval_execution = current_env_approval_execution or {}
    current_env_approval_link = current_env_approval_link or {}
    items: list[dict[str, Any]] = []

    def add_item(
        *,
        domain: str,
        severity: str,
        action_kind: str,
        due_state: str,
        due_at: str | None,
        target_id: str,
        recommended_command: str,
        blocking: bool,
        rationale: list[str],
        action_status: str = ACTION_STATUS_PLANNED,
        execution_manifest: str | None = None,
        apply_manifest: str | None = None,
        completed_at: str | None = None,
        superseded_by: str | None = None,
        next_action_id: str | None = None,
        ledger_status: str | None = None,
        retry_status: str | None = None,
        handoff_status: str | None = None,
        decision_status: str | None = None,
        decision_manifest: str | None = None,
        decision_apply_manifest: str | None = None,
        closure_status: str | None = None,
        retry_plan_status: str | None = None,
        ledger_compaction_status: str | None = None,
        approval_runbook_status: str | None = None,
        approval_execution_status: str | None = None,
        approval_blocker: str | None = None,
        approval_recommended_command: str | None = None,
        approval_applied: bool | None = None,
        budget_registry_changed: bool | None = None,
        budget_baseline_changed: bool | None = None,
        operator_blocker: str | None = None,
        next_operator_action: str | None = None,
        previous_action_id: str | None = None,
        action_chain_id: str | None = None,
    ) -> None:
        items.append(
            {
                "item_id": f"{phase}-agenda-{len(items) + 1:03d}",
                "domain": domain,
                "severity": severity,
                "action_kind": action_kind,
                "action_status": action_status,
                "due_state": due_state,
                "due_at": due_at,
                "overdue_days": overdue_days_for_deadline(now, due_at),
                "target_id": target_id,
                "recommended_command": recommended_command,
                "execution_manifest": execution_manifest,
                "apply_manifest": apply_manifest,
                "completed_at": completed_at,
                "superseded_by": superseded_by,
                "next_action_id": next_action_id,
                "ledger_status": ledger_status,
                "retry_status": retry_status,
                "handoff_status": handoff_status,
                "decision_status": decision_status,
                "decision_manifest": decision_manifest,
                "decision_apply_manifest": decision_apply_manifest,
                "closure_status": closure_status,
                "retry_plan_status": retry_plan_status,
                "ledger_compaction_status": ledger_compaction_status,
                "approval_runbook_status": approval_runbook_status,
                "approval_execution_status": approval_execution_status,
                "approval_blocker": approval_blocker,
                "approval_recommended_command": approval_recommended_command,
                "approval_applied": approval_applied,
                "budget_registry_changed": budget_registry_changed,
                "budget_baseline_changed": budget_baseline_changed,
                "operator_blocker": operator_blocker,
                "next_operator_action": next_operator_action,
                "previous_action_id": previous_action_id,
                "action_chain_id": action_chain_id,
                "blocking": blocking,
                "rationale": rationale,
            }
        )

    current_action = str(current_env_due.get("recommended_action_current_env") or "NO_ACTION")
    current_due_state = str(current_env_due.get("monitoring_due_state") or CURRENT_ENV_DUE_NOT_DUE)
    if current_action == "RUN_CURRENT_ENV_REPROPOSAL_GATE":
        current_due_state = str(current_env_due.get("reproposal_due_state") or current_due_state)
    current_severity = "OK"
    if current_action == "PREPARE_MONITORING":
        current_severity = "INFO"
    elif current_action in {"RUN_CURRENT_ENV_WATCH_CAMPAIGN", "RUN_CURRENT_ENV_REPROPOSAL_GATE"}:
        current_severity = "ACTION_REQUIRED"
    elif current_action == "FAIL":
        current_severity = "FAIL"
    current_action_kind = current_action if current_action != "PREPARE_MONITORING" else "RUN_CURRENT_ENV_WATCH_CAMPAIGN"
    current_action_status = ACTION_STATUS_PLANNED
    current_execution_manifest = None
    current_apply_manifest = None
    current_completed_at = None
    current_next_action_id = None
    if current_action_kind == "RUN_CURRENT_ENV_WATCH_CAMPAIGN" and current_env_watch_execute:
        current_action_status = str(current_env_watch_execute.get("action_status") or ACTION_STATUS_EXECUTED)
        current_execution_manifest = current_env_watch_execute.get("execute_manifest_path")
        current_completed_at = current_env_watch_execute.get("generated_at_utc")
    if current_action_kind == "RUN_CURRENT_ENV_WATCH_CAMPAIGN" and current_env_watch_apply:
        current_action_status = str(current_env_watch_apply.get("action_status") or ACTION_STATUS_APPLIED)
        current_apply_manifest = current_env_watch_apply.get("apply_manifest_path")
        current_completed_at = current_env_watch_apply.get("generated_at_utc") or current_completed_at
        current_next_action_id = str(current_env_watch_apply.get("next_operator_action") or "")
        next_action = str(current_env_watch_apply.get("next_operator_action") or "NO_ACTION")
        if current_action_status == ACTION_STATUS_APPLIED and next_action in {"", "NO_ACTION"}:
            current_severity = "OK"
        elif current_action_status == ACTION_STATUS_FAILED:
            current_severity = "FAIL"
        elif next_action not in {"", "NO_ACTION"}:
            current_severity = "ACTION_REQUIRED"
    if current_action_kind == "RUN_CURRENT_ENV_REPROPOSAL_GATE" and current_env_reproposal_execute:
        current_action_status = str(current_env_reproposal_execute.get("action_status") or ACTION_STATUS_EXECUTED)
        current_execution_manifest = current_env_reproposal_execute.get("execute_manifest_path")
        current_completed_at = current_env_reproposal_execute.get("generated_at_utc")
        current_next_action_id = str(current_env_reproposal_execute.get("recommended_next_action") or "")
        if current_action_status == ACTION_STATUS_FAILED:
            current_severity = "FAIL"
    latest_applied_action_id = current_env_action_ledger.get("latest_applied_action_id")
    retry_status = ACTION_RETRY_STATUS_NONE
    if int(current_env_retry_plan.get("retryable_count", 0) or 0) > 0:
        retry_status = ACTION_RETRY_STATUS_RETRYABLE
        current_severity = "ACTION_REQUIRED"
    if int(current_env_retry_plan.get("escalation_count", 0) or 0) > 0:
        retry_status = ACTION_RETRY_STATUS_ESCALATE
        current_severity = "FAIL"
    handoff_status = str(current_env_reproposal_handoff.get("handoff_status") or "")
    decision_status = None
    decision_apply_manifest = None
    closure_status = None
    next_operator_action = None
    if current_env_operator_decision:
        decision_status = "DECIDED" if bool(current_env_operator_decision.get("decision_valid", True)) else "INVALID"
    if current_env_operator_decision_apply:
        decision_apply_manifest = current_env_operator_decision_apply.get("decision_apply_manifest_path")
        closure_status = current_env_operator_decision_apply.get("closure_status")
        next_operator_action = current_env_operator_decision_apply.get("next_operator_action")
    handoff_closed = (
        current_env_operator_decision_apply
        and str(current_env_operator_decision_apply.get("action_id") or "") == str(current_env_reproposal_handoff.get("action_id") or "")
        and str(current_env_operator_decision_apply.get("closure_status") or "") in {CLOSURE_STATUS_CLOSED, CLOSURE_STATUS_APPROVAL_APPLIED}
    )
    approval_status = str(current_env_approval_execution.get("approval_status") or "")
    approval_blockers = [
        str(item)
        for item in list(current_env_approval_runbook.get("approval_blockers", []))
        + list(current_env_approval_execution.get("approval_blockers", []))
        if str(item).strip()
    ]
    approval_runbook_status = (
        "READY" if bool(current_env_approval_runbook.get("approval_ready", False))
        else "BLOCKED" if current_env_approval_runbook
        else None
    )
    if bool(current_env_reproposal_handoff.get("approval_ready", False)) and not handoff_closed:
        current_severity = "ACTION_REQUIRED"
    add_item(
        domain="current_env",
        severity=current_severity,
        action_kind=current_action_kind,
        due_state=current_due_state,
        due_at=current_env_due.get("next_due_at"),
        target_id="current-env",
        recommended_command=str(current_env_due.get("recommended_command") or ""),
        blocking=current_severity in {"ACTION_REQUIRED", "FAIL"},
        rationale=list(current_env_due.get("rationale", [])),
        action_status=current_action_status,
        execution_manifest=current_execution_manifest,
        apply_manifest=current_apply_manifest,
        completed_at=current_completed_at,
        next_action_id=current_next_action_id,
        ledger_status="RECORDED" if current_env_action_ledger else None,
        retry_status=retry_status if current_env_retry_plan else None,
        handoff_status=handoff_status or None,
        decision_status=decision_status,
        decision_manifest=current_env_operator_decision.get("decision_manifest_path"),
        decision_apply_manifest=decision_apply_manifest,
        closure_status=closure_status,
        retry_plan_status=current_env_retry_plan.get("plan_verdict"),
        ledger_compaction_status="COMPACTED" if current_env_action_ledger_compact else None,
        approval_runbook_status=approval_runbook_status,
        approval_execution_status=approval_status or None,
        approval_blocker="; ".join(approval_blockers) or None,
        approval_recommended_command=current_env_approval_runbook.get("recommended_command"),
        approval_applied=bool(current_env_approval_execution.get("approval_status") == APPROVAL_STATUS_APPLIED) if current_env_approval_execution else None,
        budget_registry_changed=bool(current_env_approval_execution.get("registry_updated", False)) if current_env_approval_execution else None,
        budget_baseline_changed=bool(current_env_approval_execution.get("baseline_written", False)) if current_env_approval_execution else None,
        operator_blocker="retry escalation required" if retry_status == ACTION_RETRY_STATUS_ESCALATE else None,
        next_operator_action=next_operator_action,
        previous_action_id=latest_applied_action_id,
        action_chain_id=f"{phase}-current-env-action-chain",
    )

    if bool(current_env_reproposal_handoff.get("approval_ready", False)) and not handoff_closed:
        add_item(
            domain="current_env",
            severity="ACTION_REQUIRED",
            action_kind="APPROVE_RUNTIME_BUDGET_REPROFILE",
            due_state="OPEN",
            due_at=None,
            target_id=str(current_env_reproposal_handoff.get("action_id") or "current-env-reproposal"),
            recommended_command=str(current_env_reproposal_handoff.get("recommended_approval_command") or ""),
            blocking=False,
            rationale=list(current_env_reproposal_handoff.get("rationale", [])),
            action_status=ACTION_STATUS_PLANNED,
            handoff_status=ACTION_HANDOFF_APPROVAL_READY,
            decision_status="PENDING_OPERATOR_DECISION",
            closure_status=CLOSURE_STATUS_OPEN,
            next_operator_action="runtime_current_env_operator_decision --decision approve",
            previous_action_id=str(current_env_reproposal_handoff.get("action_id") or ""),
            action_chain_id=f"{phase}-current-env-action-chain",
        )
    elif handoff_closed:
        add_item(
            domain="current_env",
            severity="OK",
            action_kind="APPROVE_RUNTIME_BUDGET_REPROFILE",
            due_state="CLOSED",
            due_at=None,
            target_id=str(current_env_reproposal_handoff.get("action_id") or "current-env-reproposal"),
            recommended_command=str(current_env_operator_decision_apply.get("next_operator_action") or ""),
            blocking=False,
            rationale=list(current_env_operator_decision_apply.get("rationale", [])),
            action_status=str(current_env_operator_decision_apply.get("new_status") or ACTION_STATUS_CLOSED),
            handoff_status=ACTION_HANDOFF_APPROVAL_READY,
            decision_status="APPLIED",
            decision_manifest=current_env_operator_decision.get("decision_manifest_path"),
            decision_apply_manifest=current_env_operator_decision_apply.get("decision_apply_manifest_path"),
            closure_status=str(current_env_operator_decision_apply.get("closure_status") or CLOSURE_STATUS_CLOSED),
            approval_runbook_status=approval_runbook_status,
            approval_execution_status=approval_status or None,
            approval_blocker="; ".join(approval_blockers) or None,
            approval_recommended_command=current_env_approval_runbook.get("recommended_command"),
            approval_applied=bool(current_env_approval_execution.get("approval_status") == APPROVAL_STATUS_APPLIED) if current_env_approval_execution else None,
            budget_registry_changed=bool(current_env_approval_execution.get("registry_updated", False)) if current_env_approval_execution else None,
            budget_baseline_changed=bool(current_env_approval_execution.get("baseline_written", False)) if current_env_approval_execution else None,
            next_operator_action=current_env_operator_decision_apply.get("next_operator_action"),
            previous_action_id=str(current_env_reproposal_handoff.get("action_id") or ""),
            action_chain_id=f"{phase}-current-env-action-chain",
        )

    known_added = False
    for entry in runtime_registry_health.get("approved_known_environments", {}).get("entries", []):
        if not isinstance(entry, dict):
            continue
        health_status = str(entry.get("health_status", "HEALTHY"))
        if health_status not in {"REVERIFY_REQUIRED", "STALE", "RETIRE_CANDIDATE"}:
            continue
        known_added = True
        action_kind = "RUN_KNOWN_ENV_REVERIFY"
        if health_status == "RETIRE_CANDIDATE":
            action_kind = "REVIEW_KNOWN_ENV_RETIRE"
        add_item(
            domain="approved_known_env",
            severity="ACTION_REQUIRED",
            action_kind=action_kind,
            due_state=health_status,
            due_at=entry.get("due_at"),
            target_id=str(entry.get("active_baseline_id") or entry.get("fingerprint_key") or "known-env"),
            recommended_command=str(entry.get("recommended_command") or ""),
            blocking=False,
            rationale=[str(entry.get("watch_reason") or health_status)],
        )
    if not known_added and known_env_reverify_plan.get("entries"):
        for entry in known_env_reverify_plan.get("entries", []):
            if not isinstance(entry, dict):
                continue
            known_added = True
            add_item(
                domain="approved_known_env",
                severity="ACTION_REQUIRED" if known_env_reverify_plan.get("plan_verdict") == "ACTION_REQUIRED" else "INFO",
                action_kind="RUN_KNOWN_ENV_REVERIFY",
                due_state=str(entry.get("current_state") or "REVERIFY_PLAN"),
                due_at=entry.get("due_at"),
                target_id=str(entry.get("known_env_id") or entry.get("baseline_id") or "known-env"),
                recommended_command=str(entry.get("recommended_command") or ""),
                blocking=False,
                rationale=[str(entry.get("reverify_due_reason") or "known env reverify plan entry")],
            )
    if not known_added:
        add_item(
            domain="approved_known_env",
            severity="OK",
            action_kind="NO_ACTION",
            due_state=CURRENT_ENV_DUE_NOT_DUE,
            due_at=None,
            target_id="approved-known-envs",
            recommended_command="",
            blocking=False,
            rationale=["approved known environment governance has no immediate action"],
        )

    if foreign_import_summaries:
        for index, summary in enumerate(foreign_import_summaries, start=1):
            add_item(
                domain="foreign_env",
                severity="ACTION_REQUIRED",
                action_kind="RUN_FOREIGN_ENV_ONBOARDING",
                due_state="OPEN",
                due_at=None,
                target_id=str(summary.get("imported_fingerprint_key") or summary.get("runtime_fingerprint_key") or f"foreign-env-{index}"),
                recommended_command="./raw_engine_tests --case runtime_new_env_proposal_gate_smoke",
                blocking=False,
                rationale=["foreign/unapproved environment still requires explicit rebaseline before strict comparison"],
            )
    else:
        add_item(
            domain="foreign_env",
            severity="ACTION_REQUIRED",
            action_kind="RUN_FOREIGN_ENV_ONBOARDING",
            due_state="OPEN",
            due_at=None,
            target_id="foreign-envs",
            recommended_command="./raw_engine_tests --case runtime_new_env_proposal_gate_smoke",
            blocking=False,
            rationale=["foreign/unapproved environments remain separated from current-env maintenance"],
        )

    if publication_health and str(publication_health.get("status", "HEALTHY")) != "HEALTHY":
        add_item(
            domain="publication",
            severity="ACTION_REQUIRED",
            action_kind="REPAIR_PUBLICATION",
            due_state="OPEN",
            due_at=None,
            target_id="publication-health",
            recommended_command="python3 tests/tools/runtime_watch_ops.py publication-health --phase phase43 ...",
            blocking=True,
            rationale=["publication health is not healthy"],
        )

    priority = {"FAIL": 0, "ACTION_REQUIRED": 1, "INFO": 2, "OK": 3}
    domain_priority = {"current_env": 0, "approved_known_env": 1, "publication": 2, "foreign_env": 3}
    items.sort(key=lambda item: (priority.get(str(item.get("severity")), 9), domain_priority.get(str(item.get("domain")), 9), item.get("item_id", "")))
    for index, item in enumerate(items, start=1):
        item["item_id"] = f"{phase}-agenda-{index:03d}"
    planned_action_count = sum(1 for item in items if item.get("action_kind") != "NO_ACTION" and item.get("action_status") == ACTION_STATUS_PLANNED)
    executed_action_count = sum(1 for item in items if item.get("action_kind") != "NO_ACTION" and item.get("action_status") == ACTION_STATUS_EXECUTED)
    applied_action_count = sum(1 for item in items if item.get("action_kind") != "NO_ACTION" and item.get("action_status") == ACTION_STATUS_APPLIED)
    failed_action_count = sum(1 for item in items if item.get("action_status") == ACTION_STATUS_FAILED)
    pending_decision_count = sum(1 for item in items if item.get("decision_status") == "PENDING_OPERATOR_DECISION")
    closed_action_count = sum(1 for item in items if item.get("closure_status") == CLOSURE_STATUS_CLOSED)
    deferred_action_count = sum(1 for item in items if item.get("closure_status") == CLOSURE_STATUS_DEFERRED)
    retryable_action_count = int(current_env_retry_plan.get("retryable_count", 0) or 0)
    handoff_ready_count = 1 if bool(current_env_reproposal_handoff.get("approval_ready", False)) else 0
    approval_applied_count = sum(1 for item in items if bool(item.get("approval_applied", False)))
    approval_blocker_count = sum(1 for item in items if str(item.get("approval_blocker") or "").strip())
    payload = {
        "manifest_version": "policy_ops_agenda_v4"
        if (current_env_approval_runbook or current_env_approval_execution or current_env_approval_link)
        else "policy_ops_agenda_v3"
        if (
            current_env_operator_decision
            or current_env_operator_decision_apply
            or current_env_action_ledger_compact
            or current_env_action_ledger_archive
        )
        else "policy_ops_agenda_v2"
        if (current_env_action_ledger or current_env_retry_plan or current_env_reproposal_handoff)
        else "policy_ops_agenda_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "now_utc": timestamp_utc_from_datetime(now),
        "item_count": len(items),
        "action_required_count": sum(1 for item in items if str(item.get("severity")) in {"ACTION_REQUIRED", "FAIL"}),
        "blocking_action_count": sum(1 for item in items if bool(item.get("blocking", False))),
        "planned_action_count": planned_action_count,
        "executed_action_count": executed_action_count,
        "applied_action_count": applied_action_count,
        "failed_action_count": failed_action_count,
        "pending_decision_count": pending_decision_count,
        "closed_action_count": closed_action_count,
        "deferred_action_count": deferred_action_count,
        "retryable_action_count": retryable_action_count,
        "handoff_ready_count": handoff_ready_count,
        "approval_runbook_status": approval_runbook_status or "NOT_RUN",
        "approval_execution_status": approval_status or "NOT_RUN",
        "approval_applied_count": approval_applied_count,
        "approval_blocker_count": approval_blocker_count,
        "ledger_compaction_status": "COMPACTED" if current_env_action_ledger_compact else "NOT_RUN",
        "archived_action_count": current_env_action_ledger_archive.get("archived_action_count", 0),
        "current_env_action_count": sum(1 for item in items if item.get("domain") == "current_env"),
        "approved_known_env_action_count": sum(1 for item in items if item.get("domain") == "approved_known_env" and item.get("action_kind") != "NO_ACTION"),
        "foreign_env_action_count": sum(1 for item in items if item.get("domain") == "foreign_env" and item.get("action_kind") != "NO_ACTION"),
        "highest_priority_domain": items[0].get("domain") if items else "none",
        "highest_priority_action": items[0].get("action_kind") if items else "NO_ACTION",
        "items": items,
    }
    payload["ops_agenda_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_policy_ops_agenda_text(payload: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={payload.get('manifest_version', '')}",
        f"phase={payload.get('phase', '')}",
        f"now_utc={payload.get('now_utc', '')}",
        f"item_count={payload.get('item_count', 0)}",
        f"action_required_count={payload.get('action_required_count', 0)}",
        f"blocking_action_count={payload.get('blocking_action_count', 0)}",
        f"planned_action_count={payload.get('planned_action_count', 0)}",
        f"executed_action_count={payload.get('executed_action_count', 0)}",
        f"applied_action_count={payload.get('applied_action_count', 0)}",
        f"failed_action_count={payload.get('failed_action_count', 0)}",
        f"pending_decision_count={payload.get('pending_decision_count', 0)}",
        f"closed_action_count={payload.get('closed_action_count', 0)}",
        f"deferred_action_count={payload.get('deferred_action_count', 0)}",
        f"retryable_action_count={payload.get('retryable_action_count', 0)}",
        f"handoff_ready_count={payload.get('handoff_ready_count', 0)}",
        f"approval_runbook_status={payload.get('approval_runbook_status', '')}",
        f"approval_execution_status={payload.get('approval_execution_status', '')}",
        f"approval_applied_count={payload.get('approval_applied_count', 0)}",
        f"approval_blocker_count={payload.get('approval_blocker_count', 0)}",
        f"ledger_compaction_status={payload.get('ledger_compaction_status', '')}",
        f"archived_action_count={payload.get('archived_action_count', 0)}",
        f"highest_priority_domain={payload.get('highest_priority_domain', '')}",
        f"highest_priority_action={payload.get('highest_priority_action', '')}",
    ]
    return "\n".join(lines) + "\n"


def build_runtime_budget_reproposal_history(
    *,
    phase: str,
    runtime_budget_baseline: dict[str, Any],
    runtime_budget_registry: dict[str, Any],
    current_env_watch_history: dict[str, Any],
    current_env_trigger_gate: dict[str, Any],
) -> dict[str, Any]:
    approval_metadata = runtime_budget_approval_metadata(runtime_budget_baseline)
    active_entries = [
        dict(entry)
        for entry in runtime_budget_registry.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("status", "")).strip() == "active"
    ]
    retired_entries = [
        dict(entry)
        for entry in runtime_budget_registry.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("status", "")).strip() == "retired"
    ]
    payload = {
        "manifest_version": "runtime_budget_reproposal_history_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "previous_active_budget_profile_id": approval_metadata.get("previous_active_budget_profile_id"),
        "active_budget_profile_id": approval_metadata.get("new_active_budget_profile_id")
        or runtime_budget_baseline.get("profile_id"),
        "approval_timestamp_utc": approval_metadata.get("approval_timestamp_utc")
        or runtime_budget_baseline.get("approval_timestamp_utc"),
        "post_approval_grace_until": current_env_watch_history.get("post_approval_grace_until"),
        "next_monitoring_due_at": current_env_watch_history.get("next_monitoring_due_at"),
        "last_release_watch_campaign_at": current_env_watch_history.get("last_release_watch_campaign_at"),
        "last_reproposal_gate_at": stable_manifest_timestamp(current_env_trigger_gate),
        "reproposal_count_for_fingerprint": current_env_watch_history.get("reproposal_count_for_fingerprint", 0),
        "cumulative_stable_soft_overrun_count": current_env_watch_history.get("cumulative_stable_soft_overrun_count", 0),
        "cumulative_hard_breach_count": current_env_watch_history.get("cumulative_hard_breach_count", 0),
        "active_entry_count": len(active_entries),
        "retired_entry_count": len(retired_entries),
    }
    payload["reproposal_history_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_runtime_budget_registry_phase42_summary(
    *,
    phase: str,
    runtime_budget_registry: dict[str, Any],
    runtime_budget_baseline: dict[str, Any],
    current_env_trigger_gate: dict[str, Any],
    current_env_watch_history: dict[str, Any],
) -> dict[str, Any]:
    active_entries = [
        dict(entry)
        for entry in runtime_budget_registry.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("status", "")).strip() == "active"
    ]
    retired_entries = [
        dict(entry)
        for entry in runtime_budget_registry.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("status", "")).strip() == "retired"
    ]
    payload = {
        "manifest_version": "runtime_budget_registry_summary_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "active_budget_profile_id": runtime_budget_baseline.get("profile_id"),
        "active_budget_profile_tag": runtime_budget_baseline.get("budget_tag"),
        "previous_active_budget_profile_id": runtime_budget_approval_metadata(runtime_budget_baseline).get("previous_active_budget_profile_id"),
        "approval_timestamp_utc": runtime_budget_baseline.get("approval_timestamp_utc")
        or runtime_budget_approval_metadata(runtime_budget_baseline).get("approval_timestamp_utc"),
        "active_entry_count": len(active_entries),
        "retired_entry_count": len(retired_entries),
        "trigger_gate_verdict": current_env_trigger_gate.get("trigger_gate_verdict"),
        "next_monitoring_due_at": current_env_watch_history.get("next_monitoring_due_at"),
        "post_approval_grace_until": current_env_watch_history.get("post_approval_grace_until"),
        "entries": runtime_budget_registry.get("entries", []),
    }
    payload["runtime_budget_registry_summary_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def watch_status_rank(value: str) -> int:
    return WATCH_STATUS_ORDER.get(str(value), -1)


def coerce_bool(value: Any) -> bool:
    if isinstance(value, bool):
        return value
    text = str(value).strip().lower()
    return text in {"1", "true", "yes", "on"}


def summarize_counts(entries: list[dict[str, Any]], key: str) -> dict[str, int]:
    counts: dict[str, int] = {}
    for entry in entries:
        value = str(entry.get(key, "")).strip()
        if not value:
            continue
        counts[value] = counts.get(value, 0) + 1
    return counts


def load_matrix_fixture_watch_refreshes(matrix_root: Path | None) -> list[dict[str, Any]]:
    if matrix_root is None or not matrix_root.exists():
        return []
    payloads: list[dict[str, Any]] = []
    for path in sorted(matrix_root.glob("*/*_runtime_watch_refresh.json")):
        payload = read_json(path)
        if not payload:
            continue
        payload["_source_path"] = str(path)
        payload["_fixture_name"] = path.parent.name
        payloads.append(payload)
    return payloads


def matrix_entry_map(matrix_summary: dict[str, Any]) -> dict[str, dict[str, Any]]:
    mapping: dict[str, dict[str, Any]] = {}
    for item in matrix_summary.get("matrix_entries", []):
        name = str(item.get("name", "")).strip()
        if name:
            mapping[name] = dict(item)
    return mapping


def normalized_evidence_source(source_kind: str) -> str:
    source = str(source_kind).strip().lower()
    if source in {"same_fingerprint", "real", "imported_real"}:
        return "real"
    if source in {"matrix_fixture", "fixture", "imported_fixture"}:
        return "fixture"
    if source in {"replay", "imported_replay"}:
        return "replay"
    return source or "real"


def contribution_timestamp(entry: dict[str, Any], manifest: dict[str, Any]) -> str:
    for key in ("import_timestamp", "last_seen_timestamp", "generated_at_utc", "timestamp_utc"):
        value = str(entry.get(key, "") or manifest.get(key, "")).strip()
        if value:
            return value
    return stable_manifest_timestamp(manifest)


def build_watch_registry_contribution(
    entry: dict[str, Any],
    *,
    source_kind: str,
    manifest: dict[str, Any],
    fixture_name: str | None,
    matrix_entry: dict[str, Any] | None,
    runner_id: str | None = None,
    host_label: str | None = None,
) -> dict[str, Any]:
    runtime_fingerprint_key = str(
        entry.get("runtime_fingerprint_key")
        or manifest.get("runtime_fingerprint_key")
        or (matrix_entry.get("runtime_fingerprint_key") if matrix_entry else "")
    ).strip()
    execution_class = str(entry.get("execution_class", "all")).strip()
    role = str(entry.get("execution_role") or entry.get("role") or "").strip()
    watch_status = str(entry.get("watch_status") or manifest.get("overall_watch_status") or "").strip()
    watch_reason = str(entry.get("watch_reason") or manifest.get("overall_watch_reason") or "").strip()
    watch_recommendation = str(
        entry.get("watch_recommendation") or manifest.get("overall_watch_recommendation") or ""
    ).strip()
    comparability_verdict = str(
        entry.get("comparability_verdict")
        or manifest.get("comparability_verdict")
        or (matrix_entry.get("runtime_comparability_verdict") if matrix_entry else "")
    ).strip()
    selected_baseline_id = str(
        manifest.get("selected_baseline_id")
        or manifest.get("runtime_selected_baseline_id")
        or (matrix_entry.get("selected_baseline_id") if matrix_entry else "")
        or ""
    ).strip()
    evidence_source = normalized_evidence_source(source_kind)
    return {
        "runtime_fingerprint_key": runtime_fingerprint_key,
        "execution_class": execution_class,
        "role": role,
        "source_kind": source_kind,
        "evidence_source": evidence_source,
        "fixture_name": fixture_name,
        "watch_status": watch_status,
        "watch_reason": watch_reason,
        "watch_recommendation": watch_recommendation,
        "sample_count": int(entry.get("sample_count", 0)),
        "stable_overrun_count": int(entry.get("stable_overrun_count", 0)),
        "soft_over_budget_count": int(entry.get("soft_over_budget_count", 0)),
        "hard_over_budget_count": int(entry.get("hard_over_budget_count", 0)),
        "clear_count": int(entry.get("clear_count", 0)),
        "escalation_count": int(entry.get("escalation_count", 0)),
        "rebaseline_candidate": coerce_bool(entry.get("rebaseline_candidate", False)),
        "last_seen_timestamp": contribution_timestamp(entry, manifest),
        "selected_runtime_baseline_id": selected_baseline_id or None,
        "selected_runtime_baseline_tag": (
            str(manifest.get("selected_baseline_tag") or (matrix_entry.get("selected_baseline_tag") if matrix_entry else "")).strip()
            or None
        ),
        "jitter_estimate_percent": float(entry.get("jitter_estimate_percent", 0.0) or 0.0),
        "comparability_verdict": comparability_verdict or None,
        "severity": (
            str(matrix_entry.get("severity", "")).strip() if matrix_entry is not None else None
        ),
        "recommended_action": (
            str(matrix_entry.get("recommended_action", "")).strip() if matrix_entry is not None else None
        ),
        "runner_id": str(entry.get("runner_id", "") or runner_id or manifest.get("runner_id", "")).strip() or None,
        "host_label": str(entry.get("host_label", "") or host_label or manifest.get("host_label", "")).strip() or None,
        "current_env_observed": source_kind == "same_fingerprint",
        "cross_env_observed": source_kind != "same_fingerprint",
    }


def strongest_watch_status(entries: list[dict[str, Any]]) -> str:
    if not entries:
        return "CLEAR"
    strongest = max(entries, key=lambda item: watch_status_rank(str(item.get("watch_status", "CLEAR"))))
    return str(strongest.get("watch_status", "CLEAR"))


def registry_key_for_entry(runtime_fingerprint_key: str, execution_class: str) -> str:
    return sha256_text("|".join([runtime_fingerprint_key, execution_class]))


def aggregate_watch_confidence(entry: dict[str, Any]) -> tuple[str, str]:
    evidence_counts = dict(entry.get("evidence_source_counts", {}))
    real_samples = int(evidence_counts.get("real", 0))
    fixture_samples = int(evidence_counts.get("fixture", 0))
    replay_samples = int(evidence_counts.get("replay", 0))
    sample_count = int(entry.get("sample_count", 0))
    role = str(entry.get("role", "")).strip() or "operator"
    watch_status = str(entry.get("watch_status", "CLEAR"))
    hard_over_budget_count = int(entry.get("hard_over_budget_count", 0))
    stable_overrun_count = int(entry.get("stable_overrun_count", 0))
    history_depth = max(sample_count, int(entry.get("lineage_count", 0)))
    bounded_jitter_limit = {
        "production_critical": 12.0,
        "diagnostic": 20.0,
        "operator": 30.0,
    }.get(role, 15.0)
    required_high_real_samples = {
        "production_critical": 5,
        "diagnostic": 8,
        "operator": 3,
    }.get(role, 5)
    required_medium_real_samples = {
        "production_critical": 2,
        "diagnostic": 3,
        "operator": 1,
    }.get(role, 1)
    bounded_jitter = float(entry.get("jitter_estimate_percent", 0.0)) <= bounded_jitter_limit
    watch_status = str(entry.get("watch_status", "CLEAR"))
    if hard_over_budget_count > 0:
        if real_samples >= required_medium_real_samples:
            return "HIGH", f"{role} hard-budget evidence was observed in repeated real same-fingerprint samples"
        return "MEDIUM", f"{role} hard-budget evidence exists, but repeated real samples are still limited"
    if (
        real_samples >= required_high_real_samples
        and sample_count >= required_high_real_samples
        and history_depth >= required_high_real_samples
        and bounded_jitter
        and watch_status in {"CLEAR", "WATCH", "WATCH_STABLE", "WATCH_ESCALATE", "REBASELINE_CANDIDATE"}
    ):
        if watch_status == "WATCH_STABLE" and stable_overrun_count > 0:
            return (
                "HIGH",
                f"{role} watch is backed by {real_samples} real same-fingerprint samples with bounded jitter and stable overrun depth {stable_overrun_count}",
            )
        return (
            "HIGH",
            f"{role} watch is backed by repeated real same-fingerprint evidence with bounded jitter",
        )
    if real_samples >= required_medium_real_samples:
        return (
            "MEDIUM",
            f"{role} evidence is real-observed, but the repeated same-fingerprint history is not deep enough for high confidence",
        )
    if fixture_samples > 0 or replay_samples > 0:
        return "LOW", f"{role} watch is driven primarily by fixture or replay evidence"
    return "LOW", "watch evidence is sparse"


def existing_registry_contributions(registry: dict[str, Any]) -> list[dict[str, Any]]:
    contributions: list[dict[str, Any]] = []
    for entry in registry.get("entries", []):
        if not isinstance(entry, dict):
            continue
        nested = entry.get("contributions")
        if isinstance(nested, list) and nested:
            contributions.extend(dict(item) for item in nested if isinstance(item, dict))
            continue
        if entry.get("runtime_fingerprint_key") and entry.get("execution_class"):
            contributions.append(dict(entry))
    return contributions


def build_watch_registry(
    watch_current: dict[str, Any],
    watch_refresh: dict[str, Any],
    watch_history_summary: dict[str, Any],
    matrix_summary: dict[str, Any],
    matrix_fixture_refreshes: list[dict[str, Any]],
    existing_registry: dict[str, Any] | None = None,
) -> dict[str, Any]:
    contributions: list[dict[str, Any]] = existing_registry_contributions(existing_registry or {})
    same_fingerprint_manifest = (
        watch_refresh
        if str(watch_refresh.get("runtime_fingerprint_key", "")).strip()
        else watch_current
    )
    for entry in same_fingerprint_manifest.get("entries", []):
        contributions.append(
            build_watch_registry_contribution(
                dict(entry),
                source_kind="same_fingerprint",
                manifest=same_fingerprint_manifest,
                fixture_name=None,
                matrix_entry=None,
            )
        )

    matrix_entries = matrix_entry_map(matrix_summary)
    for manifest in matrix_fixture_refreshes:
        fixture_name = str(manifest.get("_fixture_name", "")).strip() or None
        mapped = matrix_entries.get(fixture_name or "", {})
        for entry in manifest.get("entries", []):
            contributions.append(
                build_watch_registry_contribution(
                    dict(entry),
                    source_kind="matrix_fixture",
                    manifest=manifest,
                    fixture_name=fixture_name,
                    matrix_entry=mapped,
                )
            )

    grouped: dict[str, dict[str, Any]] = {}
    for contribution in contributions:
        runtime_fingerprint_key = str(contribution.get("runtime_fingerprint_key", "")).strip()
        execution_class = str(contribution.get("execution_class", "")).strip()
        if not runtime_fingerprint_key or not execution_class:
            continue
        registry_key = registry_key_for_entry(runtime_fingerprint_key, execution_class)
        aggregate = grouped.setdefault(
            registry_key,
            {
                "registry_key": registry_key,
                "runtime_fingerprint_key": runtime_fingerprint_key,
                "execution_class": execution_class,
                "role": str(contribution.get("role", "")),
                "contributions": [],
            },
        )
        aggregate["contributions"].append(contribution)

    entries: list[dict[str, Any]] = []
    global_source_counts: dict[str, int] = {"real": 0, "fixture": 0, "replay": 0}
    real_fingerprints: set[str] = set()
    latest_real_sample_timestamp = ""
    latest_fixture_sample_timestamp = ""
    for aggregate in grouped.values():
        contributions_for_entry = list(aggregate.get("contributions", []))
        current_env_contributions = [
            item for item in contributions_for_entry if coerce_bool(item.get("current_env_observed", False))
        ]
        cross_env_contributions = [
            item for item in contributions_for_entry if coerce_bool(item.get("cross_env_observed", False))
        ]
        summary_contributions = current_env_contributions or contributions_for_entry
        strongest = max(
            summary_contributions,
            key=lambda item: (
                watch_status_rank(str(item.get("watch_status", "CLEAR"))),
                str(item.get("last_seen_timestamp", "")),
            ),
        )
        source_counts: dict[str, int] = {"real": 0, "fixture": 0, "replay": 0}
        runner_ids: list[str] = []
        host_labels: list[str] = []
        sample_count = 0
        stable_overrun_count = 0
        clear_count = 0
        escalation_count = 0
        jitter_estimate_percent = 0.0
        for contribution in summary_contributions:
            evidence_source = normalized_evidence_source(str(contribution.get("evidence_source", contribution.get("source_kind", ""))))
            sample_value = max(1, int(contribution.get("sample_count", 0)))
            source_counts[evidence_source] = source_counts.get(evidence_source, 0) + sample_value
            sample_count += sample_value
            stable_overrun_count = max(stable_overrun_count, int(contribution.get("stable_overrun_count", 0)))
            clear_count = max(clear_count, int(contribution.get("clear_count", 0)))
            escalation_count = max(escalation_count, int(contribution.get("escalation_count", 0)))
            jitter_estimate_percent = max(jitter_estimate_percent, float(contribution.get("jitter_estimate_percent", 0.0) or 0.0))
            runner_id = str(contribution.get("runner_id", "")).strip()
            host_label = str(contribution.get("host_label", "")).strip()
            if runner_id and runner_id not in runner_ids:
                runner_ids.append(runner_id)
            if host_label and host_label not in host_labels:
                host_labels.append(host_label)
        for contribution in contributions_for_entry:
            evidence_source = normalized_evidence_source(str(contribution.get("evidence_source", contribution.get("source_kind", ""))))
            sample_value = max(1, int(contribution.get("sample_count", 0)))
            global_source_counts[evidence_source] = global_source_counts.get(evidence_source, 0) + sample_value
            if evidence_source == "real":
                real_fingerprints.add(str(contribution.get("runtime_fingerprint_key", "")))
                timestamp = str(contribution.get("last_seen_timestamp", "")).strip()
                if timestamp and timestamp >= latest_real_sample_timestamp:
                    latest_real_sample_timestamp = timestamp
            if evidence_source == "fixture":
                timestamp = str(contribution.get("last_seen_timestamp", "")).strip()
                if timestamp and timestamp >= latest_fixture_sample_timestamp:
                    latest_fixture_sample_timestamp = timestamp

        entry = {
            "registry_key": aggregate["registry_key"],
            "runtime_fingerprint_key": aggregate["runtime_fingerprint_key"],
            "execution_class": aggregate["execution_class"],
            "role": strongest.get("role"),
            "watch_status": strongest.get("watch_status"),
            "watch_reason": strongest.get("watch_reason"),
            "watch_recommendation": strongest.get("watch_recommendation"),
            "sample_count": sample_count,
            "stable_overrun_count": stable_overrun_count,
            "clear_count": clear_count,
            "escalation_count": escalation_count,
            "soft_over_budget_count": max(int(item.get("soft_over_budget_count", 0)) for item in contributions_for_entry),
            "hard_over_budget_count": max(int(item.get("hard_over_budget_count", 0)) for item in contributions_for_entry),
            "rebaseline_candidate": any(coerce_bool(item.get("rebaseline_candidate", False)) for item in contributions_for_entry),
            "last_seen_timestamp": max(str(item.get("last_seen_timestamp", "")) for item in contributions_for_entry),
            "selected_runtime_baseline_id": strongest.get("selected_runtime_baseline_id"),
            "selected_runtime_baseline_tag": strongest.get("selected_runtime_baseline_tag"),
            "comparability_verdict": strongest.get("comparability_verdict"),
            "severity": strongest.get("severity"),
            "recommended_action": strongest.get("recommended_action"),
            "evidence_sources": [key for key, value in source_counts.items() if value > 0],
            "evidence_source_counts": source_counts,
            "lineage_count": len(summary_contributions),
            "aggregate_lineage_count": len(contributions_for_entry),
            "real_lineage_count": sum(1 for item in summary_contributions if normalized_evidence_source(str(item.get("evidence_source", ""))) == "real"),
            "aggregate_real_lineage_count": sum(1 for item in contributions_for_entry if normalized_evidence_source(str(item.get("evidence_source", ""))) == "real"),
            "latest_real_sample_timestamp": max(
                [str(item.get("last_seen_timestamp", "")) for item in summary_contributions if normalized_evidence_source(str(item.get("evidence_source", ""))) == "real"],
                default="",
            )
            or None,
            "latest_fixture_sample_timestamp": max(
                [str(item.get("last_seen_timestamp", "")) for item in summary_contributions if normalized_evidence_source(str(item.get("evidence_source", ""))) == "fixture"],
                default="",
            )
            or None,
            "jitter_estimate_percent": round(jitter_estimate_percent, 2),
            "runner_ids": runner_ids,
            "host_labels": host_labels,
            "current_env_observed": any(coerce_bool(item.get("current_env_observed", False)) for item in contributions_for_entry),
            "cross_env_observed": any(coerce_bool(item.get("cross_env_observed", False)) for item in contributions_for_entry),
            "summary_scope": "current_env" if current_env_contributions else "mixed",
            "current_env_contribution_count": len(current_env_contributions),
            "cross_env_contribution_count": len(cross_env_contributions),
            "contributions": contributions_for_entry,
        }
        watch_confidence, confidence_reason = aggregate_watch_confidence(entry)
        entry["watch_confidence"] = watch_confidence
        entry["confidence_reason"] = confidence_reason
        entries.append(entry)

    registry = {
        "manifest_version": "runtime_watch_registry_v2",
        "generated_at_utc": stable_manifest_timestamp(watch_refresh)
        or stable_manifest_timestamp(watch_current)
        or stable_manifest_timestamp(matrix_summary)
        or stable_manifest_timestamp(existing_registry or {}),
        "entries": sorted(entries, key=lambda item: (str(item.get("runtime_fingerprint_key", "")), str(item.get("execution_class", "")))),
        "entry_count": len(entries),
        "fingerprint_count": len({str(item.get("runtime_fingerprint_key", "")) for item in entries if item.get("runtime_fingerprint_key")}),
        "status_counts": summarize_counts(entries, "watch_status"),
        "recommendation_counts": summarize_counts(entries, "watch_recommendation"),
        "comparability_counts": summarize_counts(entries, "comparability_verdict"),
        "role_counts": summarize_counts(entries, "role"),
        "source_counts": {key: value for key, value in global_source_counts.items() if value > 0},
        "evidence_source_counts": {key: value for key, value in global_source_counts.items() if value > 0},
        "confidence_counts": summarize_counts(entries, "watch_confidence"),
        "active_lineage_count": len({str(item.get("runtime_fingerprint_key", "")) for item in entries}),
        "real_lineage_count": len(real_fingerprints),
        "latest_real_sample_timestamp": latest_real_sample_timestamp or None,
        "latest_fixture_sample_timestamp": latest_fixture_sample_timestamp or None,
        "current_env_status": str(watch_refresh.get("overall_watch_status") or watch_current.get("overall_watch_status") or "CLEAR"),
        "current_env_recommendation": str(
            watch_refresh.get("overall_watch_recommendation") or watch_current.get("overall_watch_recommendation") or "NO_ACTION"
        ),
        "matrix_status": str(matrix_summary.get("runtime_watch_status", "")),
        "matrix_recommendation": str(matrix_summary.get("runtime_watch_recommendation", "")),
        "history_transition_summary": watch_history_summary.get("watch_transition_counts", {}),
        "history_transition_count": int(watch_history_summary.get("transition_count", 0)),
        "strongest_watch_status": strongest_watch_status(entries),
    }
    registry["registry_hash"] = sha256_text(json.dumps(registry, sort_keys=True))
    return registry


def build_watch_registry_text(registry: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={registry.get('manifest_version', '')}",
        f"generated_at_utc={registry.get('generated_at_utc', '')}",
        f"entry_count={registry.get('entry_count', 0)}",
        f"fingerprint_count={registry.get('fingerprint_count', 0)}",
        f"strongest_watch_status={registry.get('strongest_watch_status', '')}",
        f"active_lineage_count={registry.get('active_lineage_count', 0)}",
        f"real_lineage_count={registry.get('real_lineage_count', 0)}",
        f"current_env_status={registry.get('current_env_status', '')}",
        f"current_env_recommendation={registry.get('current_env_recommendation', '')}",
        f"matrix_status={registry.get('matrix_status', '')}",
        f"matrix_recommendation={registry.get('matrix_recommendation', '')}",
        f"status_counts={json.dumps(registry.get('status_counts', {}), ensure_ascii=False)}",
        f"recommendation_counts={json.dumps(registry.get('recommendation_counts', {}), ensure_ascii=False)}",
        f"comparability_counts={json.dumps(registry.get('comparability_counts', {}), ensure_ascii=False)}",
        f"role_counts={json.dumps(registry.get('role_counts', {}), ensure_ascii=False)}",
        f"evidence_source_counts={json.dumps(registry.get('evidence_source_counts', {}), ensure_ascii=False)}",
        f"confidence_counts={json.dumps(registry.get('confidence_counts', {}), ensure_ascii=False)}",
        f"history_transition_count={registry.get('history_transition_count', 0)}",
    ]
    return "\n".join(lines) + "\n"


def extract_family_statuses(policy_manifest: dict[str, Any]) -> list[dict[str, Any]]:
    families = policy_manifest.get("families")
    if isinstance(families, dict):
        result: list[dict[str, Any]] = []
        for name, value in families.items():
            if isinstance(value, dict):
                item = {"family": name}
                item.update(value)
                result.append(item)
            else:
                result.append({"family": name, "status": value})
        return result
    if isinstance(families, list):
        return [dict(item) for item in families if isinstance(item, dict)]
    selected = policy_manifest.get("selected_entries")
    if isinstance(selected, list):
        return [dict(item) for item in selected if isinstance(item, dict)]
    return []


def load_json_list_rationale(payload: dict[str, Any]) -> list[str]:
    values = payload.get("rationale", [])
    if isinstance(values, list):
        return [str(item) for item in values if str(item).strip()]
    text = str(values).strip()
    return [text] if text else []


def registry_entries_for_predicate(
    watch_registry: dict[str, Any],
    predicate: Any,
) -> list[dict[str, Any]]:
    return [dict(entry) for entry in watch_registry.get("entries", []) if isinstance(entry, dict) and predicate(entry)]


def summarize_registry_entries(entries: list[dict[str, Any]]) -> dict[str, Any]:
    evidence_source_counts: dict[str, int] = {"real": 0, "fixture": 0, "replay": 0}
    latest_real_sample_timestamp = ""
    latest_fixture_sample_timestamp = ""
    watch_confidence = "LOW"
    confidence_reason = "no runtime watch evidence is available"
    if not entries:
        return {
            "entry_count": 0,
            "evidence_source_counts": {},
            "active_lineage_count": 0,
            "real_lineage_count": 0,
            "watch_confidence": watch_confidence,
            "confidence_reason": confidence_reason,
            "latest_real_sample_timestamp": None,
            "latest_fixture_sample_timestamp": None,
        }
    for entry in entries:
        for key, value in dict(entry.get("evidence_source_counts", {})).items():
            evidence_source_counts[key] = evidence_source_counts.get(key, 0) + int(value)
        real_timestamp = str(entry.get("latest_real_sample_timestamp", "")).strip()
        fixture_timestamp = str(entry.get("latest_fixture_sample_timestamp", "")).strip()
        if real_timestamp and real_timestamp >= latest_real_sample_timestamp:
            latest_real_sample_timestamp = real_timestamp
        if fixture_timestamp and fixture_timestamp >= latest_fixture_sample_timestamp:
            latest_fixture_sample_timestamp = fixture_timestamp
    strongest_entry = max(
        entries,
        key=lambda item: (
            {"LOW": 0, "MEDIUM": 1, "HIGH": 2}.get(str(item.get("watch_confidence", "LOW")), 0),
            watch_status_rank(str(item.get("watch_status", "CLEAR"))),
            str(item.get("last_seen_timestamp", "")),
        ),
    )
    watch_confidence = str(strongest_entry.get("watch_confidence", "LOW"))
    confidence_reason = str(strongest_entry.get("confidence_reason", confidence_reason))
    return {
        "entry_count": len(entries),
        "evidence_source_counts": {key: value for key, value in evidence_source_counts.items() if value > 0},
        "active_lineage_count": len({str(entry.get("runtime_fingerprint_key", "")) for entry in entries if entry.get("runtime_fingerprint_key")}),
        "real_lineage_count": len(
            {
                str(entry.get("runtime_fingerprint_key", ""))
                for entry in entries
                if int(dict(entry.get("evidence_source_counts", {})).get("real", 0)) > 0
            }
        ),
        "watch_confidence": watch_confidence,
        "confidence_reason": confidence_reason,
        "latest_real_sample_timestamp": latest_real_sample_timestamp or None,
        "latest_fixture_sample_timestamp": latest_fixture_sample_timestamp or None,
    }


def history_bucket_for_fingerprint(history_index: dict[str, Any], fingerprint_key: str) -> dict[str, Any]:
    for bucket in history_index.get("fingerprints", []):
        if str(bucket.get("runtime_fingerprint_key", "")).strip() == fingerprint_key:
            return dict(bucket)
    return {}


def latest_history_sample_for_fingerprint(history_index: dict[str, Any], fingerprint_key: str) -> dict[str, Any]:
    bucket = history_bucket_for_fingerprint(history_index, fingerprint_key)
    latest_sample: dict[str, Any] = {}
    for payload in dict(bucket.get("execution_classes", {})).values():
        if not isinstance(payload, dict):
            continue
        for sample in payload.get("samples", []):
            if not isinstance(sample, dict):
                continue
            if str(sample.get("timestamp_utc", "")) >= str(latest_sample.get("timestamp_utc", "")):
                latest_sample = dict(sample)
    return latest_sample


def aggregate_watch_registry_for_fingerprint(watch_registry: dict[str, Any], fingerprint_key: str) -> dict[str, Any]:
    entries = [
        dict(entry)
        for entry in watch_registry.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("runtime_fingerprint_key", "")).strip() == fingerprint_key
    ]
    if not entries:
        return {
            "watch_status": "CLEAR",
            "watch_confidence": "LOW",
            "watch_reason": "no watch evidence recorded for this fingerprint lineage",
            "latest_watch_summary_path": None,
            "evidence_source_counts": {},
            "real_sample_count": 0,
            "fixture_sample_count": 0,
            "latest_real_sample_timestamp": None,
            "latest_fixture_sample_timestamp": None,
        }
    summary = summarize_registry_entries(entries)
    strongest = max(
        entries,
        key=lambda item: (
            watch_status_rank(str(item.get("watch_status", "CLEAR"))),
            {"LOW": 0, "MEDIUM": 1, "HIGH": 2}.get(str(item.get("watch_confidence", "LOW")), 0),
            str(item.get("last_seen_timestamp", "")),
        ),
    )
    evidence_source_counts = dict(summary.get("evidence_source_counts", {}))
    return {
        "watch_status": str(strongest.get("watch_status", "CLEAR")),
        "watch_confidence": summary.get("watch_confidence", "LOW"),
        "watch_reason": str(strongest.get("watch_reason", "")) or summary.get("confidence_reason", ""),
        "watch_recommendation": str(strongest.get("watch_recommendation", "")),
        "latest_watch_summary_path": (
            str(strongest.get("watch_summary_path", "")).strip()
            or str(strongest.get("summary_path", "")).strip()
            or str(strongest.get("source_manifest_path", "")).strip()
            or None
        ),
        "budget_verdict": "FAIL"
        if max(int(entry.get("hard_over_budget_count", 0)) for entry in entries) > 0
        else ("BUDGET_WARN" if max(int(entry.get("soft_over_budget_count", 0)) for entry in entries) > 0 else "PASS"),
        "evidence_source_counts": evidence_source_counts,
        "real_sample_count": int(evidence_source_counts.get("real", 0)),
        "fixture_sample_count": int(evidence_source_counts.get("fixture", 0)),
        "latest_real_sample_timestamp": summary.get("latest_real_sample_timestamp"),
        "latest_fixture_sample_timestamp": summary.get("latest_fixture_sample_timestamp"),
    }


def active_runtime_registry_entries(registry: dict[str, Any]) -> list[dict[str, Any]]:
    return [
        dict(entry)
        for entry in registry.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("status", runtime_gate.REGISTRY_STATUS_RETIRED)) == runtime_gate.REGISTRY_STATUS_ACTIVE
    ]


def retired_runtime_registry_entries(registry: dict[str, Any]) -> list[dict[str, Any]]:
    return [
        dict(entry)
        for entry in registry.get("entries", [])
        if isinstance(entry, dict) and str(entry.get("status", runtime_gate.REGISTRY_STATUS_ACTIVE)) == runtime_gate.REGISTRY_STATUS_RETIRED
    ]


def runtime_registry_entry_fingerprint(entry: dict[str, Any]) -> str:
    return str(entry.get("runtime_fingerprint_key", entry.get("fingerprint_key", ""))).strip()


def resolve_known_env_entry(
    registry: dict[str, Any],
    known_env_id: str | None,
    *,
    allow_retired: bool = False,
) -> dict[str, Any] | None:
    requested = str(known_env_id or "").strip()
    candidates = [
        dict(entry)
        for entry in registry.get("entries", [])
        if isinstance(entry, dict)
        and (allow_retired or str(entry.get("status", runtime_gate.REGISTRY_STATUS_RETIRED)) == runtime_gate.REGISTRY_STATUS_ACTIVE)
    ]
    if requested:
        for entry in candidates:
            keys = {
                str(entry.get("baseline_id", "")).strip(),
                str(entry.get("baseline_tag", "")).strip(),
                runtime_registry_entry_fingerprint(entry),
            }
            if requested in keys:
                return entry
        return None
    non_current = [entry for entry in candidates if not bool(entry.get("counts_as_current_env", False))]
    if len(non_current) == 1:
        return non_current[0]
    return None


def runtime_registry_lineage_history(registry: dict[str, Any], fingerprint_key: str) -> list[dict[str, Any]]:
    entries = [
        dict(entry)
        for entry in registry.get("entries", [])
        if isinstance(entry, dict)
        and str(entry.get("runtime_fingerprint_key", entry.get("fingerprint_key", ""))).strip() == fingerprint_key
    ]
    return sorted(
        [
            {
                "baseline_id": entry.get("baseline_id"),
                "baseline_tag": entry.get("baseline_tag"),
                "status": entry.get("status"),
                "approval_timestamp_utc": entry.get("approval_timestamp_utc"),
                "previous_active_baseline_id": entry.get("previous_active_baseline_id"),
                "supersedes_baseline_ids": entry.get("supersedes_baseline_ids", []),
                "superseded_by_baseline_id": entry.get("superseded_by_baseline_id"),
                "retired_reason": entry.get("retired_reason"),
            }
            for entry in entries
        ],
        key=lambda item: (str(item.get("approval_timestamp_utc", "")), str(item.get("baseline_id", ""))),
        reverse=True,
    )


def classify_known_env_health(
    *,
    baseline_exists: bool,
    is_current_environment: bool,
    comparability_verdict: str,
    freshness_verdict: str,
    last_seen_timestamp: str | None,
    stale_after_hours: float,
    reverify_after_hours: float,
    retire_after_hours: float,
) -> tuple[str, bool, bool, str, float | None]:
    if not baseline_exists:
        return "ORPHANED", True, True, "REVERIFY_KNOWN_ENV", None
    if is_current_environment:
        if comparability_verdict == "COMPARABLE" and freshness_verdict == "FRESH":
            return "HEALTHY", False, False, "NO_ACTION", age_hours(last_seen_timestamp)
        return "REVERIFY_REQUIRED", True, True, "REVERIFY_KNOWN_ENV", age_hours(last_seen_timestamp)
    observed_age_hours = age_hours(last_seen_timestamp)
    if observed_age_hours is None:
        return "REVERIFY_REQUIRED", True, True, "REVERIFY_KNOWN_ENV", None
    if observed_age_hours >= retire_after_hours:
        return "RETIRE_CANDIDATE", True, True, "RETIRE_KNOWN_ENV", observed_age_hours
    if observed_age_hours >= reverify_after_hours:
        return "REVERIFY_REQUIRED", True, True, "REVERIFY_KNOWN_ENV", observed_age_hours
    if observed_age_hours >= stale_after_hours:
        return "STALE", True, False, "REVERIFY_KNOWN_ENV", observed_age_hours
    if comparability_verdict != "COMPARABLE" or freshness_verdict != "FRESH":
        return "REVERIFY_REQUIRED", True, True, "REVERIFY_KNOWN_ENV", observed_age_hours
    return "HEALTHY", False, False, "NO_ACTION", observed_age_hours


def policy_days(hours: float) -> int:
    return int(round(hours / 24.0))


def known_env_status_from_health(health_status: str, *, is_current_environment: bool) -> str:
    if is_current_environment:
        return ENV_STATE_CURRENT_ACTIVE
    if health_status == "HEALTHY":
        return ENV_STATE_APPROVED_KNOWN_FRESH
    if health_status == "REVERIFY_REQUIRED":
        return ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED
    if health_status == "STALE":
        return ENV_STATE_APPROVED_KNOWN_STALE
    if health_status == "RETIRE_CANDIDATE":
        return ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE
    return ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED


def build_known_env_lifecycle_fields(
    *,
    entry: dict[str, Any],
    health_status: str,
    is_current_environment: bool,
    last_seen_timestamp: str | None,
    stale_after_hours: float,
    retire_after_hours: float,
    reverify_required: bool,
) -> dict[str, Any]:
    last_verified_timestamp = str(entry.get("approval_timestamp_utc", "")).strip() or None
    age_verified_hours = age_hours(last_verified_timestamp)
    lifecycle_state = known_env_status_from_health(health_status, is_current_environment=is_current_environment)
    return {
        "status": lifecycle_state,
        "state": lifecycle_state,
        "approval_timestamp_utc": last_verified_timestamp,
        "last_verified_timestamp": last_verified_timestamp,
        "last_runtime_import_timestamp": str(last_seen_timestamp or "").strip() or None,
        "last_refresh_timestamp": str(last_seen_timestamp or "").strip() or None,
        "age_since_last_verified_days": None if age_verified_hours is None else round(age_verified_hours / 24.0, 2),
        "stale_after_days": policy_days(stale_after_hours),
        "retire_after_days": policy_days(retire_after_hours),
        "reverify_required": bool(reverify_required),
        "retire_candidate": health_status == "RETIRE_CANDIDATE",
        "counts_as_current_env": bool(is_current_environment),
        "counts_as_approved_known_env": not bool(is_current_environment),
    }


def governance_entry_timestamps(
    *,
    entry: dict[str, Any],
    latest_sample: dict[str, Any],
    watch_summary: dict[str, Any],
) -> dict[str, Any]:
    approval_timestamp = str(entry.get("approval_timestamp_utc", "")).strip() or None
    last_verified_timestamp = str(entry.get("last_verified_timestamp", "")).strip() or approval_timestamp
    last_runtime_import_timestamp = (
        str(entry.get("last_runtime_import_timestamp", "")).strip()
        or str(watch_summary.get("latest_real_sample_timestamp") or watch_summary.get("latest_fixture_sample_timestamp") or "").strip()
        or str(latest_sample.get("timestamp_utc", "")).strip()
        or approval_timestamp
    )
    last_refresh_timestamp = (
        str(entry.get("last_refresh_timestamp", "")).strip()
        or str(latest_sample.get("timestamp_utc", "")).strip()
        or last_runtime_import_timestamp
    )
    latest_refresh_manifest_path = (
        str(entry.get("latest_refresh_manifest_path", "")).strip()
        or str(latest_sample.get("refresh_manifest_path", "")).strip()
        or None
    )
    latest_watch_summary_path = (
        str(entry.get("latest_watch_summary_path", "")).strip()
        or str(watch_summary.get("latest_watch_summary_path", "")).strip()
        or None
    )
    return {
        "approval_timestamp_utc": approval_timestamp,
        "last_verified_timestamp": last_verified_timestamp,
        "last_runtime_import_timestamp": last_runtime_import_timestamp,
        "last_refresh_timestamp": last_refresh_timestamp,
        "latest_refresh_manifest_path": latest_refresh_manifest_path,
        "latest_watch_summary_path": latest_watch_summary_path,
    }


def governance_state_for_age(
    *,
    age_since_last_verified_days: float | None,
    policy: dict[str, Any],
) -> tuple[str, bool, str | None, float]:
    due_after = float(policy.get("reverify_due_after_days", 0))
    stale_after = float(policy.get("stale_after_days", 0))
    retire_candidate_after = float(policy.get("retire_candidate_after_days", 0))
    retire_after = float(policy.get("retire_after_days", 0))
    due_soon_window = max(0.0, float(policy.get("due_soon_window_days", 0)))
    if age_since_last_verified_days is None:
        return ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED, False, None, 0.0
    due_soon = max(0.0, due_after - due_soon_window) <= age_since_last_verified_days < due_after
    due_at_days = max(0.0, age_since_last_verified_days - due_after)
    overdue_days = round(max(0.0, due_at_days), 2)
    if age_since_last_verified_days >= retire_after:
        return ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE, False, "RETIRE_KNOWN_ENV", overdue_days
    if age_since_last_verified_days >= retire_candidate_after:
        return ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE, False, "RETIRE_KNOWN_ENV", overdue_days
    if age_since_last_verified_days >= stale_after:
        return ENV_STATE_APPROVED_KNOWN_STALE, False, "REVERIFY_KNOWN_ENV", overdue_days
    if age_since_last_verified_days >= due_after:
        return ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED, False, "REVERIFY_KNOWN_ENV", overdue_days
    return ENV_STATE_APPROVED_KNOWN_FRESH, due_soon, None, overdue_days


def evaluate_known_env_governance_entry(
    *,
    entry: dict[str, Any],
    latest_sample: dict[str, Any],
    watch_summary: dict[str, Any],
    policy: dict[str, Any],
    now: datetime,
    is_current_environment: bool,
    comparability_verdict: str,
    freshness_verdict: str,
) -> dict[str, Any]:
    timestamps = governance_entry_timestamps(entry=entry, latest_sample=latest_sample, watch_summary=watch_summary)
    baseline_exists = Path(str(entry.get("runtime_baseline_manifest_path", ""))).exists()
    if str(entry.get("status", runtime_gate.REGISTRY_STATUS_ACTIVE)) == runtime_gate.REGISTRY_STATUS_RETIRED:
        state = ENV_STATE_RETIRED_KNOWN_ENV
        health_status = "RETIRED"
        recommendation = "KEEP_AS_ARCHIVE_ONLY"
        due_soon = False
        overdue_days = 0.0
    elif is_current_environment:
        state = ENV_STATE_CURRENT_ACTIVE
        health_status = "HEALTHY" if comparability_verdict == "COMPARABLE" and freshness_verdict == "FRESH" else "REVERIFY_REQUIRED"
        recommendation = "NO_ACTION" if health_status == "HEALTHY" else "REVERIFY_KNOWN_ENV"
        due_soon = False
        overdue_days = 0.0
    elif not baseline_exists:
        state = ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED
        health_status = "ORPHANED"
        recommendation = "KEEP_AS_ARCHIVE_ONLY"
        due_soon = False
        overdue_days = 0.0
    else:
        age_verified_days = age_days(timestamps["last_verified_timestamp"], now=now)
        state, due_soon, recommendation, overdue_days = governance_state_for_age(
            age_since_last_verified_days=age_verified_days,
            policy=policy,
        )
        health_status_map = {
            ENV_STATE_APPROVED_KNOWN_FRESH: "HEALTHY",
            ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED: "REVERIFY_REQUIRED",
            ENV_STATE_APPROVED_KNOWN_STALE: "STALE",
            ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE: "RETIRE_CANDIDATE",
            ENV_STATE_RETIRED_KNOWN_ENV: "RETIRED",
        }
        health_status = health_status_map.get(state, "REVERIFY_REQUIRED")
        if comparability_verdict != "COMPARABLE" or freshness_verdict != "FRESH":
            if state == ENV_STATE_APPROVED_KNOWN_FRESH:
                state = ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED
                health_status = "REVERIFY_REQUIRED"
                recommendation = "REVERIFY_KNOWN_ENV"
                due_soon = False
        if str(watch_summary.get("watch_status", "")).strip() == "FAIL":
            state = ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED
            health_status = "REVERIFY_REQUIRED"
            recommendation = "REVERIFY_KNOWN_ENV"
            due_soon = False
    age_since_last_verified_days = age_days(timestamps["last_verified_timestamp"], now=now)
    return {
        "state": state,
        "status": state,
        "health_status": health_status,
        "recommendation": recommendation or "NO_ACTION",
        "due_soon": bool(due_soon),
        "overdue_days": overdue_days,
        "approval_timestamp_utc": timestamps["approval_timestamp_utc"],
        "last_verified_timestamp": timestamps["last_verified_timestamp"],
        "last_runtime_import_timestamp": timestamps["last_runtime_import_timestamp"],
        "last_refresh_timestamp": timestamps["last_refresh_timestamp"],
        "age_since_last_verified_days": age_since_last_verified_days,
        "stale_after_days": int(policy.get("stale_after_days", 0)),
        "retire_after_days": int(policy.get("retire_after_days", 0)),
        "reverify_due_after_days": int(policy.get("reverify_due_after_days", 0)),
        "retire_candidate_after_days": int(policy.get("retire_candidate_after_days", 0)),
        "retire_candidate": state == ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE,
        "reverify_required": state in {ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED, ENV_STATE_APPROVED_KNOWN_STALE},
        "counts_as_current_env": bool(is_current_environment),
        "counts_as_approved_known_env": not bool(is_current_environment) and state != ENV_STATE_RETIRED_KNOWN_ENV,
        "latest_refresh_manifest_path": timestamps["latest_refresh_manifest_path"],
        "latest_watch_summary_path": timestamps["latest_watch_summary_path"],
        "due_at": add_days_utc(timestamps["last_verified_timestamp"], float(policy.get("reverify_due_after_days", 0))),
    }


def build_runtime_registry_health(
    *,
    phase: str,
    runtime_baseline_registry: dict[str, Any],
    runtime_history_index: dict[str, Any],
    runtime_watch_registry: dict[str, Any],
    runtime_refresh: dict[str, Any],
    approved_known_summaries: list[dict[str, Any]] | None = None,
    foreign_import_summaries: list[dict[str, Any]] | None = None,
    governance_policy: dict[str, Any] | None = None,
    current_time_override: str | None = None,
    stale_after_hours: float = KNOWN_ENV_STALE_AFTER_HOURS,
    reverify_after_hours: float = KNOWN_ENV_REVERIFY_AFTER_HOURS,
    retire_after_hours: float = KNOWN_ENV_RETIRE_AFTER_HOURS,
) -> dict[str, Any]:
    approved_known_summaries = approved_known_summaries or []
    foreign_import_summaries = foreign_import_summaries or []
    approved_known_summary_by_baseline: dict[str, dict[str, Any]] = {}
    for item in approved_known_summaries:
        if not isinstance(item, dict):
            continue
        for key in (
            str(item.get("selected_baseline_id", "")).strip(),
            str(item.get("selected_baseline_tag", "")).strip(),
        ):
            if key:
                approved_known_summary_by_baseline[key] = dict(item)
    current_baseline_id = str(runtime_refresh.get("selected_baseline_id", "")).strip()
    current_fingerprint_key = str(runtime_refresh.get("runtime_fingerprint_key", "")).strip()
    active_entries = active_runtime_registry_entries(runtime_baseline_registry)
    retired_entries = retired_runtime_registry_entries(runtime_baseline_registry)

    active_lineages: list[dict[str, Any]] = []
    approved_known_entries: list[dict[str, Any]] = []
    orphaned_entry_count = 0
    stale_active_count = 0
    healthy_active_count = 0
    reverify_required_count = 0
    retire_candidate_count = 0
    due_soon_count = 0
    governance_now = resolve_governance_now(current_time_override, 0.0)
    health_manifest_version = "runtime_registry_health_v3" if governance_policy else "runtime_registry_health_v2"

    for entry in active_entries:
        fingerprint_key = str(entry.get("runtime_fingerprint_key", entry.get("fingerprint_key", ""))).strip()
        latest_sample = latest_history_sample_for_fingerprint(runtime_history_index, fingerprint_key)
        watch_summary = aggregate_watch_registry_for_fingerprint(runtime_watch_registry, fingerprint_key)
        latest_refresh_manifest_path = str(latest_sample.get("refresh_manifest_path", "")).strip() or None
        latest_current_manifest_path = str(latest_sample.get("current_manifest_path", "")).strip() or None
        latest_refresh_manifest = read_json(Path(latest_refresh_manifest_path)) if latest_refresh_manifest_path and Path(latest_refresh_manifest_path).exists() else {}
        latest_comparability = str(
            latest_refresh_manifest.get("comparability_verdict")
            or latest_sample.get("comparability")
            or ("COMPARABLE" if fingerprint_key and fingerprint_key == current_fingerprint_key else "")
            or "UNKNOWN"
        )
        latest_freshness = str(
            latest_refresh_manifest.get("freshness_verdict")
            or latest_sample.get("freshness_status")
            or ("FRESH" if fingerprint_key and fingerprint_key == current_fingerprint_key else "")
            or "UNKNOWN"
        )
        approved_known_summary = (
            approved_known_summary_by_baseline.get(str(entry.get("baseline_id", "")).strip())
            or approved_known_summary_by_baseline.get(str(entry.get("baseline_tag", "")).strip())
            or {}
        )
        baseline_exists = Path(str(entry.get("runtime_baseline_manifest_path", ""))).exists()
        is_current_environment = bool(current_baseline_id) and str(entry.get("baseline_id", "")) == current_baseline_id
        if not is_current_environment and current_fingerprint_key and fingerprint_key == current_fingerprint_key:
            is_current_environment = True
        if not is_current_environment and approved_known_summary:
            latest_comparability = str(approved_known_summary.get("comparability") or latest_comparability)
            latest_freshness = str(approved_known_summary.get("freshness") or latest_freshness)
        elif not is_current_environment and baseline_exists:
            # Approved known environments are evaluated against their own active lineage.
            # Lifecycle aging may still require reverification or retirement, but the
            # baseline itself should remain comparable/fresh unless an explicit summary
            # says otherwise.
            if latest_comparability in {"", "UNKNOWN", "NOT_COMPARABLE", "REBASELINE_REQUIRED"}:
                latest_comparability = "COMPARABLE"
            if latest_freshness in {"", "UNKNOWN", "NOT_COMPARABLE"}:
                latest_freshness = "FRESH"
        last_seen_timestamp = (
            str(entry.get("last_runtime_import_timestamp", "")).strip()
            or watch_summary.get("latest_real_sample_timestamp")
            or watch_summary.get("latest_fixture_sample_timestamp")
            or latest_sample.get("timestamp_utc")
            or entry.get("approval_timestamp_utc")
        )
        if governance_policy:
            governance_state = evaluate_known_env_governance_entry(
                entry=entry,
                latest_sample=latest_sample,
                watch_summary=watch_summary,
                policy=governance_policy,
                now=governance_now,
                is_current_environment=is_current_environment,
                comparability_verdict=latest_comparability,
                freshness_verdict=latest_freshness,
            )
            health_status = str(governance_state.get("health_status", "REVERIFY_REQUIRED"))
            stale_candidate = health_status in {"STALE", "REVERIFY_REQUIRED", "RETIRE_CANDIDATE", "ORPHANED"}
            reverify_required = bool(governance_state.get("reverify_required", False))
            retirement_recommendation = str(governance_state.get("recommendation", "NO_ACTION"))
            observed_age_hours = age_hours(governance_state.get("last_runtime_import_timestamp"), now=governance_now)
            lifecycle_fields = dict(governance_state)
        else:
            health_status, stale_candidate, reverify_required, retirement_recommendation, observed_age_hours = classify_known_env_health(
                baseline_exists=baseline_exists,
                is_current_environment=is_current_environment,
                comparability_verdict=latest_comparability,
                freshness_verdict=latest_freshness,
                last_seen_timestamp=last_seen_timestamp,
                stale_after_hours=stale_after_hours,
                reverify_after_hours=reverify_after_hours,
                retire_after_hours=retire_after_hours,
            )
            lifecycle_fields = build_known_env_lifecycle_fields(
                entry=entry,
                health_status=health_status,
                is_current_environment=is_current_environment,
                last_seen_timestamp=last_seen_timestamp,
                stale_after_hours=stale_after_hours,
                retire_after_hours=retire_after_hours,
                reverify_required=reverify_required,
            )
        retired_baseline_count = sum(
            1
            for candidate in retired_entries
            if str(candidate.get("runtime_fingerprint_key", candidate.get("fingerprint_key", ""))).strip() == fingerprint_key
        )
        entry_health = {
            "environment_state": "CURRENT_ENV" if is_current_environment else "APPROVED_KNOWN_ENV",
            "health_status": health_status,
            "fingerprint_key": fingerprint_key,
            "baseline_id": entry.get("baseline_id"),
            "baseline_tag": entry.get("baseline_tag"),
            "active_baseline_id": entry.get("baseline_id"),
            "active_baseline_tag": entry.get("baseline_tag"),
            "last_seen_timestamp": last_seen_timestamp,
            "last_seen_age_hours": None if observed_age_hours is None else round(observed_age_hours, 2),
            "latest_current_manifest_path": latest_current_manifest_path,
            "latest_refresh_manifest_path": latest_refresh_manifest_path,
            "comparability_verdict": latest_comparability,
            "freshness_verdict": latest_freshness,
            "budget_verdict": watch_summary.get("budget_verdict"),
            "watch_status": watch_summary.get("watch_status") or approved_known_summary.get("watch_status"),
            "watch_confidence": watch_summary.get("watch_confidence") or approved_known_summary.get("watch_confidence"),
            "watch_reason": watch_summary.get("watch_reason") or (
                approved_known_summary.get("rationale", [None])[0]
                if approved_known_summary.get("rationale")
                else None
            ),
            "latest_watch_summary_path": watch_summary.get("latest_watch_summary_path"),
            "evidence_source_counts": watch_summary.get("evidence_source_counts", {}),
            "real_sample_count": watch_summary.get("real_sample_count", 0),
            "fixture_sample_count": watch_summary.get("fixture_sample_count", 0),
            "retired_baseline_count": retired_baseline_count,
            "stale_candidate": stale_candidate,
            "reverify_required": reverify_required,
            "retirement_recommendation": retirement_recommendation,
            "runtime_budget_profile_id": entry.get("runtime_budget_profile_id"),
            "runtime_baseline_manifest_path": entry.get("runtime_baseline_manifest_path"),
            "lineage_history": runtime_registry_lineage_history(runtime_baseline_registry, fingerprint_key),
            **lifecycle_fields,
        }
        active_lineages.append(entry_health)
        if health_status == "ORPHANED":
            orphaned_entry_count += 1
        if stale_candidate:
            stale_active_count += 1
        if reverify_required:
            reverify_required_count += 1
        if health_status == "RETIRE_CANDIDATE":
            retire_candidate_count += 1
        if health_status == "HEALTHY":
            healthy_active_count += 1
        if bool(entry_health.get("due_soon", False)):
            due_soon_count += 1
        if not is_current_environment:
            approved_known_entries.append(entry_health)

    foreign_entries: list[dict[str, Any]] = []
    for item in foreign_import_summaries:
        if not isinstance(item, dict):
            continue
        proposal_needed = bool(item.get("proposal_needed", True))
        foreign_entries.append(
            {
                "environment_state": str(item.get("environment_state", ENV_STATE_FOREIGN_UNAPPROVED)) or ENV_STATE_FOREIGN_UNAPPROVED,
                "state": str(item.get("environment_state", ENV_STATE_FOREIGN_UNAPPROVED)) or ENV_STATE_FOREIGN_UNAPPROVED,
                "environment_label": item.get("host_label") or item.get("imported_fingerprint_key"),
                "fingerprint_key": item.get("imported_fingerprint_key"),
                "proposal_needed": proposal_needed,
                "comparability_verdict": item.get("comparability_verdict"),
                "severity": "ACTION_REQUIRED" if proposal_needed else "WARN",
                "recommendation": "REBASELINE_REQUIRED" if proposal_needed else "CONTINUE_MONITORING",
                "import_reason": item.get("import_reason"),
                "selected_baseline_id": item.get("selected_baseline_id"),
            }
        )

    overall_status = "HEALTHY"
    if retire_candidate_count > 0:
        overall_status = "RETIRE_ACTION_REQUIRED"
    elif governance_policy and (due_soon_count > 0 or reverify_required_count > 0 or orphaned_entry_count > 0 or stale_active_count > 0):
        overall_status = "GOVERNANCE_ACTION_REQUIRED" if reverify_required_count > 0 or stale_active_count > 0 or orphaned_entry_count > 0 else "ATTENTION_REQUIRED"
    elif reverify_required_count > 0 or orphaned_entry_count > 0 or stale_active_count > 0:
        overall_status = "ATTENTION_REQUIRED"

    payload = {
        "manifest_version": health_manifest_version,
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "runtime_baseline_registry_path": runtime_baseline_registry.get("registry_path"),
        "overall_status": overall_status,
        "governance_policy": governance_policy,
        "governance_reference_time_utc": timestamp_utc_from_datetime(governance_now),
        "stale_after_hours": stale_after_hours,
        "reverify_after_hours": reverify_after_hours,
        "retire_after_hours": retire_after_hours,
        "current_environment": next((entry for entry in active_lineages if entry.get("environment_state") == "CURRENT_ENV"), {}),
        "approved_known_environments": {
            "environment_count": len(approved_known_entries),
            "fresh_count": sum(1 for entry in approved_known_entries if entry.get("status") == ENV_STATE_APPROVED_KNOWN_FRESH),
            "healthy_count": sum(1 for entry in approved_known_entries if entry.get("health_status") == "HEALTHY"),
            "due_soon_count": sum(1 for entry in approved_known_entries if bool(entry.get("due_soon", False))),
            "stale_count": sum(1 for entry in approved_known_entries if entry.get("health_status") == "STALE"),
            "reverify_required_count": sum(1 for entry in approved_known_entries if entry.get("health_status") == "REVERIFY_REQUIRED"),
            "retire_candidate_count": sum(1 for entry in approved_known_entries if entry.get("health_status") == "RETIRE_CANDIDATE"),
            "entries": approved_known_entries,
        },
        "unapproved_foreign_environments": {
            "environment_count": len(foreign_entries),
            "entries": foreign_entries,
        },
        "active_lineages": active_lineages,
        "aggregate": {
            "current_active_count": sum(1 for entry in active_lineages if entry.get("status") == ENV_STATE_CURRENT_ACTIVE),
            "active_fingerprint_count": len(active_lineages),
            "approved_known_fresh_count": sum(1 for entry in approved_known_entries if entry.get("status") == ENV_STATE_APPROVED_KNOWN_FRESH),
            "approved_known_reverify_required_count": sum(1 for entry in approved_known_entries if entry.get("status") == ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED),
            "approved_known_due_soon_count": sum(1 for entry in approved_known_entries if bool(entry.get("due_soon", False))),
            "approved_known_stale_count": sum(1 for entry in approved_known_entries if entry.get("status") == ENV_STATE_APPROVED_KNOWN_STALE),
            "retired_count": len(retired_entries),
            "healthy_active_count": healthy_active_count,
            "healthy_known_env_count": sum(1 for entry in approved_known_entries if entry.get("health_status") == "HEALTHY"),
            "stale_known_env_count": sum(1 for entry in approved_known_entries if entry.get("health_status") == "STALE"),
            "stale_active_count": stale_active_count,
            "reverify_required_count": reverify_required_count,
            "governance_action_required_count": reverify_required_count + retire_candidate_count + orphaned_entry_count,
            "retire_candidate_count": retire_candidate_count,
            "retired_entry_count": len(retired_entries),
            "orphaned_entry_count": orphaned_entry_count,
            "foreign_unapproved_count": len(foreign_entries),
            "approved_known_env_count": len(approved_known_entries),
        },
        "retired_lineage_history": {
            "entry_count": len(retired_entries),
            "entries": [
                {
                    "environment_state": ENV_STATE_RETIRED_KNOWN_ENV,
                    "status": ENV_STATE_RETIRED_KNOWN_ENV,
                    "state": ENV_STATE_RETIRED_KNOWN_ENV,
                    "baseline_id": entry.get("baseline_id"),
                    "baseline_tag": entry.get("baseline_tag"),
                    "fingerprint_key": entry.get("runtime_fingerprint_key", entry.get("fingerprint_key")),
                    "retired_reason": entry.get("retired_reason"),
                    "superseded_by_baseline_id": entry.get("superseded_by_baseline_id"),
                }
                for entry in retired_entries
            ],
        },
        "approved_known_summary_count": len(approved_known_summaries),
        "foreign_import_summary_count": len(foreign_import_summaries),
    }
    payload["health_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_runtime_registry_health_text(payload: dict[str, Any]) -> str:
    aggregate = dict(payload.get("aggregate", {}))
    lines = [
        f"manifest_version={payload.get('manifest_version', '')}",
        f"phase={payload.get('phase', '')}",
        f"overall_status={payload.get('overall_status', '')}",
        f"current_active_count={aggregate.get('current_active_count', 0)}",
        f"active_fingerprint_count={aggregate.get('active_fingerprint_count', 0)}",
        f"approved_known_fresh_count={aggregate.get('approved_known_fresh_count', 0)}",
        f"approved_known_reverify_required_count={aggregate.get('approved_known_reverify_required_count', 0)}",
        f"approved_known_due_soon_count={aggregate.get('approved_known_due_soon_count', 0)}",
        f"approved_known_stale_count={aggregate.get('approved_known_stale_count', 0)}",
        f"healthy_active_count={aggregate.get('healthy_active_count', 0)}",
        f"healthy_known_env_count={aggregate.get('healthy_known_env_count', 0)}",
        f"stale_known_env_count={aggregate.get('stale_known_env_count', 0)}",
        f"stale_active_count={aggregate.get('stale_active_count', 0)}",
        f"reverify_required_count={aggregate.get('reverify_required_count', 0)}",
        f"governance_action_required_count={aggregate.get('governance_action_required_count', 0)}",
        f"retire_candidate_count={aggregate.get('retire_candidate_count', 0)}",
        f"retired_count={aggregate.get('retired_count', 0)}",
        f"retired_entry_count={aggregate.get('retired_entry_count', 0)}",
        f"orphaned_entry_count={aggregate.get('orphaned_entry_count', 0)}",
        f"approved_known_env_count={aggregate.get('approved_known_env_count', 0)}",
        f"foreign_unapproved_count={aggregate.get('foreign_unapproved_count', 0)}",
    ]
    return "\n".join(lines) + "\n"


def zip_contains_prefix(path: Path, prefix: str) -> bool:
    if not path.exists():
        return False
    with zipfile.ZipFile(path, "r") as archive:
        return any(name.startswith(prefix) for name in archive.namelist())


def build_publication_health(
    *,
    phase: str,
    published_root: Path,
    authoritative_root: Path,
    expect_bundles: bool,
    expect_manifests: bool,
    expect_report: bool,
) -> dict[str, Any]:
    bundle_metadata_path = published_root / "bundle_metadata.json"
    metadata = read_json(bundle_metadata_path)
    publication_snapshot_path = published_root / "publication_snapshot.json"
    publication_snapshot = read_json(publication_snapshot_path)
    missing_artifacts: list[str] = []
    dangling_references: list[str] = []
    hash_mismatches: list[dict[str, Any]] = []

    if not bundle_metadata_path.exists():
        missing_artifacts.append(str(bundle_metadata_path))

    if expect_report:
        report_path = Path(str(metadata.get("source_report", metadata.get("report", "")))).resolve() if metadata else None
        if report_path is None or not report_path.exists():
            dangling_references.append(str(report_path) if report_path is not None else "report")
        if not any((published_root / "reports").glob("*")):
            missing_artifacts.append(str(published_root / "reports"))

    source_manifest_hashes = dict(metadata.get("source_manifest_hashes", {}))
    published_hash_to_paths: dict[str, list[str]] = {}
    for path in published_root.rglob("*"):
        if not path.is_file():
            continue
        digest = sha256_file(path)
        if digest:
            published_hash_to_paths.setdefault(digest, []).append(str(path))

    if expect_manifests:
        manifests_dir = published_root / "manifests"
        if not manifests_dir.exists():
            missing_artifacts.append(str(manifests_dir))
        for source_path_str, expected_hash in source_manifest_hashes.items():
            source_path = Path(source_path_str).resolve()
            if not source_path.exists():
                dangling_references.append(str(source_path))
                continue
            if expected_hash not in published_hash_to_paths:
                hash_mismatches.append(
                    {
                        "source_path": str(source_path),
                        "expected_hash": expected_hash,
                        "reason": "no matching published copy hash",
                    }
                )

    if expect_bundles:
        required_bundle_keys = ("bundle_zip", "curated_zip", "light_ops_zip", "delivery_zip")
        for key in required_bundle_keys:
            bundle_path = Path(str(metadata.get(key, ""))).resolve() if metadata.get(key) else None
            if bundle_path is None or not bundle_path.exists():
                missing_artifacts.append(str(bundle_path) if bundle_path is not None else key)
        delivery_path = Path(str(metadata.get("delivery_zip", ""))).resolve() if metadata.get("delivery_zip") else None
        if delivery_path is not None and delivery_path.exists():
            for prefix in (
                "report/",
                "quick_pipeline_summary/",
                "nightly_pipeline_summary/",
                "pipeline_matrix_summary/",
                "policy_ops_summary/",
            ):
                if not zip_contains_prefix(delivery_path, prefix):
                    missing_artifacts.append(f"{delivery_path}::{prefix}")

    for key in (
        "report",
        "source_report",
        "policy_manifest_json",
        "pipeline_summary",
        "pipeline_quick_summary",
        "pipeline_matrix_summary",
        "runtime_refresh_manifest",
        "runtime_watch_registry",
        "policy_ops_summary",
        "source_snapshot_manifest",
        "staged_mirror_manifest",
        "staged_mirror_verify",
        "ctest_inventory_release",
        "ctest_inventory_debug",
        "ctest_inventory_asan",
        "verification_release",
        "verification_debug",
        "verification_asan",
        "published_snapshot_manifest",
        "verification_closeout",
    ):
        raw_value = metadata.get(key)
        if raw_value in (None, ""):
            continue
        value = str(raw_value).strip()
        if not value or value == "None":
            continue
        referenced_path = Path(value).resolve()
        if not referenced_path.exists():
            dangling_references.append(str(referenced_path))

    status = "HEALTHY"
    if missing_artifacts or hash_mismatches or dangling_references:
        status = "FAIL"
    payload = {
        "manifest_version": "publication_health_v1",
        "phase": phase,
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "published_root": str(published_root),
        "authoritative_root": str(authoritative_root),
        "bundle_metadata_path": str(bundle_metadata_path),
        "publication_snapshot_path": str(publication_snapshot_path) if publication_snapshot_path.exists() else None,
        "status": status,
        "missing_artifact_count": len(missing_artifacts),
        "hash_mismatch_count": len(hash_mismatches),
        "dangling_reference_count": len(dangling_references),
        "missing_artifacts": missing_artifacts,
        "hash_mismatches": hash_mismatches,
        "dangling_references": dangling_references,
        "publication_snapshot_id": metadata.get("publication_snapshot_id") or publication_snapshot.get("publication_snapshot_id"),
        "published_bundle_items": metadata.get("delivery_bundle_items", []),
    }
    payload["health_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_publication_health_text(payload: dict[str, Any]) -> str:
    return (
        "\n".join(
            [
                f"manifest_version={payload.get('manifest_version', '')}",
                f"phase={payload.get('phase', '')}",
                f"status={payload.get('status', '')}",
                f"missing_artifact_count={payload.get('missing_artifact_count', 0)}",
                f"hash_mismatch_count={payload.get('hash_mismatch_count', 0)}",
                f"dangling_reference_count={payload.get('dangling_reference_count', 0)}",
            ]
        )
        + "\n"
    )


def build_ops_summary(
    phase: str,
    policy_manifest: dict[str, Any],
    quick_summary: dict[str, Any],
    nightly_summary: dict[str, Any],
    matrix_summary: dict[str, Any],
    runtime_refresh: dict[str, Any],
    runtime_watch_refresh: dict[str, Any],
    watch_registry: dict[str, Any],
    approved_known_summaries: list[dict[str, Any]] | None = None,
    foreign_import_summaries: list[dict[str, Any]] | None = None,
    runtime_baseline_registry: dict[str, Any] | None = None,
    runtime_registry_health: dict[str, Any] | None = None,
    publication_health: dict[str, Any] | None = None,
    source_snapshot_manifest: dict[str, Any] | None = None,
    staged_mirror_verify: dict[str, Any] | None = None,
    verification_release: dict[str, Any] | None = None,
    verification_debug: dict[str, Any] | None = None,
    verification_asan: dict[str, Any] | None = None,
    published_snapshot_manifest: dict[str, Any] | None = None,
    verification_closeout: dict[str, Any] | None = None,
    current_env_governance_policy: dict[str, Any] | None = None,
    current_env_guardrail_policy: dict[str, Any] | None = None,
    current_env_watch_current: dict[str, Any] | None = None,
    current_env_watch_refresh: dict[str, Any] | None = None,
    current_env_watch_history: dict[str, Any] | None = None,
    current_env_age_tick: dict[str, Any] | None = None,
    current_env_watch_plan: dict[str, Any] | None = None,
    current_env_trigger_gate: dict[str, Any] | None = None,
    runtime_budget_current: dict[str, Any] | None = None,
    runtime_budget_proposal: dict[str, Any] | None = None,
    runtime_budget_proposal_gate: dict[str, Any] | None = None,
    runtime_budget_baseline: dict[str, Any] | None = None,
    runtime_budget_refresh: dict[str, Any] | None = None,
    runtime_budget_reproposal_history: dict[str, Any] | None = None,
    runtime_budget_registry_summary: dict[str, Any] | None = None,
    current_env_due: dict[str, Any] | None = None,
    current_env_reproposal_plan: dict[str, Any] | None = None,
    ops_agenda: dict[str, Any] | None = None,
    current_env_watch_execute: dict[str, Any] | None = None,
    current_env_watch_apply: dict[str, Any] | None = None,
    current_env_reproposal_execute: dict[str, Any] | None = None,
    current_env_action_ledger: dict[str, Any] | None = None,
    current_env_retry_plan: dict[str, Any] | None = None,
    current_env_reproposal_handoff: dict[str, Any] | None = None,
    current_env_operator_decision: dict[str, Any] | None = None,
    current_env_operator_decision_apply: dict[str, Any] | None = None,
    current_env_action_ledger_compact: dict[str, Any] | None = None,
    current_env_action_ledger_archive: dict[str, Any] | None = None,
    current_env_approval_runbook: dict[str, Any] | None = None,
    current_env_approval_execution: dict[str, Any] | None = None,
    current_env_approval_link: dict[str, Any] | None = None,
) -> dict[str, Any]:
    approved_known_summaries = approved_known_summaries or []
    foreign_import_summaries = foreign_import_summaries or []
    runtime_baseline_registry = runtime_baseline_registry or {}
    runtime_registry_health = runtime_registry_health or {}
    publication_health = publication_health or {}
    source_snapshot_manifest = source_snapshot_manifest or {}
    staged_mirror_verify = staged_mirror_verify or {}
    verification_release = verification_release or {}
    verification_debug = verification_debug or {}
    verification_asan = verification_asan or {}
    published_snapshot_manifest = published_snapshot_manifest or {}
    verification_closeout = verification_closeout or {}
    current_env_governance_policy = current_env_governance_policy or {}
    current_env_guardrail_policy = current_env_guardrail_policy or {}
    current_env_watch_current = current_env_watch_current or {}
    current_env_watch_refresh = current_env_watch_refresh or {}
    current_env_watch_history = current_env_watch_history or {}
    current_env_age_tick = current_env_age_tick or {}
    current_env_watch_plan = current_env_watch_plan or {}
    current_env_trigger_gate = current_env_trigger_gate or {}
    runtime_budget_current = runtime_budget_current or {}
    runtime_budget_proposal = runtime_budget_proposal or {}
    runtime_budget_proposal_gate = runtime_budget_proposal_gate or {}
    runtime_budget_baseline = runtime_budget_baseline or {}
    runtime_budget_refresh = runtime_budget_refresh or {}
    runtime_budget_reproposal_history = runtime_budget_reproposal_history or {}
    runtime_budget_registry_summary = runtime_budget_registry_summary or {}
    current_env_due = current_env_due or {}
    current_env_reproposal_plan = current_env_reproposal_plan or {}
    ops_agenda = ops_agenda or {}
    current_env_watch_execute = current_env_watch_execute or {}
    current_env_watch_apply = current_env_watch_apply or {}
    current_env_reproposal_execute = current_env_reproposal_execute or {}
    current_env_action_ledger = current_env_action_ledger or {}
    current_env_retry_plan = current_env_retry_plan or {}
    current_env_reproposal_handoff = current_env_reproposal_handoff or {}
    current_env_operator_decision = current_env_operator_decision or {}
    current_env_operator_decision_apply = current_env_operator_decision_apply or {}
    current_env_action_ledger_compact = current_env_action_ledger_compact or {}
    current_env_action_ledger_archive = current_env_action_ledger_archive or {}
    current_env_approval_runbook = current_env_approval_runbook or {}
    current_env_approval_execution = current_env_approval_execution or {}
    current_env_approval_link = current_env_approval_link or {}
    current_env_summary = nightly_summary or quick_summary
    current_action = (
        str(current_env_summary.get("runtime_recommendation", "")).strip()
        or str(current_env_summary.get("recommended_next_action", "")).strip()
        or "NO_ACTION"
    )
    new_env_action = (
        str(matrix_summary.get("runtime_recommendation", "")).strip()
        or str(matrix_summary.get("recommended_next_action", "")).strip()
        or "NO_ACTION"
    )
    matrix_action_counts = matrix_summary.get("matrix_action_counts", {})
    if (
        str(matrix_summary.get("severity", "")).strip() == "ACTION_REQUIRED"
        and int(matrix_action_counts.get("REBASELINE_REQUIRED", 0)) > 0
    ):
        new_env_action = "REBASELINE_REQUIRED"
    rationale: list[str] = []
    for item in load_json_list_rationale(current_env_summary) + load_json_list_rationale(matrix_summary):
        if item not in rationale:
            rationale.append(item)
    correctness_severity = str(current_env_summary.get("policy_severity") or current_env_summary.get("severity") or "").strip()
    correctness_verdict = str(current_env_summary.get("current_verdict", "")).strip()
    runtime_comparability = str(current_env_summary.get("runtime_comparability_verdict", "")).strip()
    runtime_freshness = str(current_env_summary.get("runtime_freshness_verdict", "")).strip()
    runtime_budget = str(current_env_summary.get("runtime_budget_verdict", "")).strip()
    runtime_watch_status = str(
        current_env_summary.get("runtime_watch_status") or runtime_watch_refresh.get("overall_watch_status") or ""
    ).strip()
    current_env_watch_reason = str(
        current_env_summary.get("runtime_watch_reason") or runtime_watch_refresh.get("overall_watch_reason") or ""
    ).strip()
    if correctness_verdict == "PASS" and correctness_severity in {"OK", "WARN"}:
        rationale.append("correctness lifecycle green")
    if runtime_comparability == "COMPARABLE" and runtime_freshness == "FRESH":
        rationale.append("runtime same-fingerprint baseline remains comparable and fresh")
    if runtime_budget in {"PASS", "BUDGET_WARN"}:
        rationale.append(f"runtime budget verdict for current environment is {runtime_budget}")
    if runtime_watch_status == "WATCH_STABLE":
        if bool(runtime_watch_refresh.get("diagnostic_watch_only")):
            rationale.append("diagnostic-only asan_full soft-budget watch remains stable")
        else:
            rationale.append("production-critical release_full soft-budget watch remains stable")
    if current_action == "CONTINUE_MONITORING":
        rationale.append("current environment does not require runtime rebaseline yet")
    if current_action == "WATCH_RUNTIME" and current_env_watch_reason:
        rationale.append("current environment should continue monitoring release_full before any further runtime approval action")
    if new_env_action == "REBASELINE_REQUIRED":
        rationale.append("new or foreign runtime fingerprints still require explicit rebaseline")
    verification_lane_status = str(
        verification_closeout.get("closeout_verdict")
        or verification_closeout.get("verification_closeout_status")
        or "NOT_RUN"
    ).strip()
    verification_action = "NO_ACTION"
    verification_conclusion = "staged verification lane has not run yet"
    if verification_lane_status in {"CLOSEOUT_PASS", "PASS"}:
        verification_conclusion = "staged verification lane is healthy"
    elif verification_lane_status in {"", "NOT_RUN"}:
        verification_action = "RUN_STAGED_VERIFICATION"
    else:
        verification_action = "REPAIR_VERIFICATION_LANE"
        verification_conclusion = "staged verification lane requires repair before treating verification closeout as complete"
        rationale.append("staged verification lane requires repair before phase closeout")
    deduped_rationale = []
    for item in rationale:
        if item and item not in deduped_rationale:
            deduped_rationale.append(item)
    rationale = deduped_rationale

    current_env_registry_summary = summarize_registry_entries(
        registry_entries_for_predicate(watch_registry, lambda entry: coerce_bool(entry.get("current_env_observed", False)))
    )
    new_env_registry_summary = summarize_registry_entries(
        registry_entries_for_predicate(
            watch_registry,
            lambda entry: coerce_bool(entry.get("cross_env_observed", False))
            and not coerce_bool(entry.get("current_env_observed", False)),
        )
    )
    stale_runtime_lineage_count = len(
        {
            str(entry.get("runtime_fingerprint_key", ""))
            for entry in watch_registry.get("entries", [])
            if str(entry.get("watch_status", "CLEAR")) not in {"", "CLEAR"}
        }
    )
    structured_rationale = []
    for item in rationale:
        category = "runtime"
        if "correctness" in item or "exact_shadow" in item:
            category = "correctness"
        elif "new or foreign" in item:
            category = "cross_environment"
        structured_rationale.append({"category": category, "message": item})

    current_env_conclusion = "current env is healthy"
    if current_action == "WATCH_RUNTIME":
        current_env_conclusion = "current env: continue monitoring release_full before treating the runtime watch as fully cleared"
    elif current_action == "CONTINUE_MONITORING":
        current_env_conclusion = "current env: continue monitoring diagnostic runtime watch only"
    elif current_action not in {"", "NO_ACTION"}:
        current_env_conclusion = f"current env: {current_action}"

    new_env_conclusion = "new env: no additional runtime action"
    if new_env_action == "REBASELINE_REQUIRED":
        new_env_conclusion = "new env: do not compare strictly without explicit rebaseline"
    elif new_env_action not in {"", "NO_ACTION"}:
        new_env_conclusion = f"new env: {new_env_action}"

    current_env_watch_budget_section = {
        "selected_runtime_baseline_id": current_env_summary.get("runtime_selected_baseline_id"),
        "selected_runtime_baseline_tag": current_env_summary.get("runtime_selected_baseline_tag"),
        "selected_budget_profile_id": None,
        "selected_budget_profile_tag": None,
        "budget_verdict": runtime_budget,
        "watch_status": runtime_watch_status,
        "watch_confidence": current_env_registry_summary.get("watch_confidence"),
        "stable_overrun_count": 0,
        "reproposal_needed": bool(current_env_summary.get("runtime_rebaseline_proposal_needed", False)),
        "reproposal_gate_verdict": None,
        "reproposal_confidence": None,
        "recommended_action_current_env": current_action,
        "rationale": [item for item in rationale if "new or foreign" not in item],
        "current_env_state": current_env_watch_refresh.get("current_env_state")
        or current_env_watch_current.get("current_env_state"),
        "watch_reason": current_env_watch_refresh.get("watch_reason")
        or current_env_watch_current.get("watch_reason")
        or current_env_watch_reason,
        "history_transition_count": current_env_watch_history.get("transition_count", 0),
    }
    if current_env_watch_refresh or current_env_watch_current or runtime_budget_current:
        focus_watch = current_env_watch_refresh or current_env_watch_current
        current_env_watch_budget_section.update(
            {
                "selected_runtime_baseline_id": focus_watch.get("selected_runtime_baseline_id")
                or runtime_budget_refresh.get("selected_runtime_baseline_id")
                or current_env_summary.get("runtime_selected_baseline_id"),
                "selected_runtime_baseline_tag": focus_watch.get("selected_runtime_baseline_tag")
                or current_env_summary.get("runtime_selected_baseline_tag"),
                "selected_budget_profile_id": runtime_budget_baseline.get("profile_id")
                or focus_watch.get("selected_budget_profile_id")
                or runtime_budget_refresh.get("selected_budget_profile_id")
                or runtime_budget_current.get("source_runtime_budget_profile_id"),
                "selected_budget_profile_tag": runtime_budget_baseline.get("budget_tag")
                or focus_watch.get("selected_budget_profile_tag"),
                "budget_verdict": runtime_budget_refresh.get("budget_verdict")
                or runtime_budget_current.get("budget_verdict")
                or runtime_budget,
                "watch_status": focus_watch.get("watch_status") or runtime_watch_status,
                "watch_confidence": focus_watch.get("watch_confidence")
                or current_env_registry_summary.get("watch_confidence"),
                "stable_overrun_count": int(focus_watch.get("stable_overrun_count", 0)),
                "reproposal_needed": bool(
                    focus_watch.get("reproposal_needed")
                    or runtime_budget_proposal_gate.get("budget_reproposal_needed")
                    or runtime_budget_proposal_gate.get("budget_proposal_needed")
                    or runtime_budget_proposal.get("budget_reproposal_needed")
                    or runtime_budget_proposal.get("budget_proposal_needed")
                    or runtime_budget_current.get("proposal_needed", False)
                ),
                "reproposal_gate_verdict": runtime_budget_proposal_gate.get("reproposal_gate_verdict")
                or runtime_budget_proposal_gate.get("proposal_gate_verdict"),
                "reproposal_confidence": runtime_budget_proposal_gate.get("reproposal_confidence")
                or runtime_budget_proposal_gate.get("proposal_confidence"),
                "current_env_state": focus_watch.get("current_env_state"),
                "watch_reason": focus_watch.get("watch_reason") or current_env_watch_reason,
                "history_transition_count": current_env_watch_history.get("transition_count", 0),
            }
        )
    guardrail_policy = current_env_guardrail_policy or current_env_governance_policy
    current_env_guardrail_section = {
        "policy_id": guardrail_policy.get("policy_id"),
        "current_state": current_env_age_tick.get("current_env_state_after")
        or current_env_trigger_gate.get("current_env_state")
        or current_env_watch_budget_section.get("current_env_state")
        or CURRENT_ENV_STATE_CLEAR,
        "approval_grace_active": bool(current_env_age_tick.get("approval_grace_active", False)),
        "next_monitoring_due_at": current_env_age_tick.get("next_due_at")
        or current_env_watch_history.get("next_monitoring_due_at"),
        "next_reproposal_due_at": current_env_age_tick.get("next_reproposal_due_at")
        or current_env_trigger_gate.get("next_reproposal_due_at")
        or current_env_watch_history.get("next_reproposal_due_at"),
        "stable_soft_overrun_count": current_env_trigger_gate.get("stable_soft_overrun_count")
        or current_env_watch_history.get("cumulative_stable_soft_overrun_count")
        or current_env_watch_budget_section.get("stable_overrun_count", 0),
        "hard_over_budget_count": current_env_trigger_gate.get("hard_over_budget_count")
        or current_env_watch_history.get("cumulative_hard_breach_count")
        or 0,
        "trigger_gate_verdict": current_env_trigger_gate.get("trigger_gate_verdict")
        or ("WATCH" if current_env_watch_budget_section.get("watch_status") in {runtime_gate.WATCH_WATCH, runtime_gate.WATCH_STABLE} else "CLEAR"),
        "recommended_action_current_env": current_env_watch_recommendation(
            str(
                current_env_age_tick.get("current_env_state_after")
                or current_env_trigger_gate.get("current_env_state")
                or current_env_watch_budget_section.get("current_env_state")
                or CURRENT_ENV_STATE_CLEAR
            )
        ),
        "watch_plan_verdict": current_env_watch_plan.get("plan_verdict"),
        "next_watch_execution_class": None
        if not current_env_watch_plan.get("entries")
        else current_env_watch_plan.get("entries", [{}])[0].get("execution_class"),
        "next_watch_due_at": None
        if not current_env_watch_plan.get("entries")
        else current_env_watch_plan.get("entries", [{}])[0].get("next_check_due_at"),
        "post_approval_grace_until": current_env_watch_history.get("post_approval_grace_until")
        or current_env_age_tick.get("post_approval_grace_until"),
        "active_budget_profile_id": runtime_budget_baseline.get("profile_id")
        or current_env_watch_history.get("active_budget_profile_id")
        or current_env_watch_budget_section.get("selected_budget_profile_id"),
        "previous_active_budget_profile_id": current_env_watch_history.get("previous_active_budget_profile_id")
        or runtime_budget_reproposal_history.get("previous_active_budget_profile_id"),
        "rationale": list(
            current_env_trigger_gate.get("rationale", [])
            or current_env_age_tick.get("trigger_reason", [])
            or current_env_watch_budget_section.get("rationale", [])
        ),
    }
    current_env_due_section = {
        "current_state": current_env_due.get("current_state_after") or current_env_guardrail_section.get("current_state"),
        "approval_grace_active": bool(current_env_due.get("approval_grace_active", current_env_guardrail_section.get("approval_grace_active", False))),
        "next_monitoring_due_at": current_env_due.get("monitoring_due_at") or current_env_guardrail_section.get("next_monitoring_due_at"),
        "next_reproposal_due_at": current_env_due.get("reproposal_due_at") or current_env_guardrail_section.get("next_reproposal_due_at"),
        "monitoring_due_state": current_env_due.get("monitoring_due_state", CURRENT_ENV_DUE_NOT_DUE),
        "reproposal_due_state": current_env_due.get("reproposal_due_state", CURRENT_ENV_DUE_NOT_DUE),
        "next_due_kind": current_env_due.get("next_due_kind", "none"),
        "next_due_at": current_env_due.get("next_due_at"),
        "overdue_days": current_env_due.get("overdue_days", 0.0),
        "recommended_action_current_env": current_env_due.get("recommended_action_current_env")
        or current_env_guardrail_section.get("recommended_action_current_env"),
        "recommended_command": current_env_due.get("recommended_command", ""),
        "rationale": list(current_env_due.get("rationale", [])),
    }
    current_env_due_action_value = str(current_env_due_section.get("recommended_action_current_env") or "NO_ACTION")
    if current_env_due_action_value and current_env_due_action_value != "NO_ACTION":
        current_env_guardrail_section["recommended_action_current_env"] = current_env_due_action_value
        current_env_guardrail_section["due_scheduler_recommended_action"] = current_env_due_action_value
        current_env_guardrail_section["due_scheduler_command"] = current_env_due_section.get("recommended_command")
    ops_agenda_highest_action = str(ops_agenda.get("highest_priority_action") or "NO_ACTION")
    ops_agenda_highest_domain = str(ops_agenda.get("highest_priority_domain") or "")
    current_env_due_conclusion = "current env due scheduler has no pending action"
    if current_env_due_action_value not in {"", "NO_ACTION"}:
        current_env_due_conclusion = f"current env due scheduler: {current_env_due_action_value}"
    ops_agenda_conclusion = "operator agenda has no pending action"
    if ops_agenda_highest_action not in {"", "NO_ACTION"}:
        ops_agenda_conclusion = f"operator agenda top action: {ops_agenda_highest_action}"
    current_env_actions_section = {
        "planned_action_count": current_env_action_ledger.get("planned_count", ops_agenda.get("planned_action_count", 0)),
        "executed_action_count": current_env_action_ledger.get("executed_count", ops_agenda.get("executed_action_count", 0)),
        "applied_action_count": current_env_action_ledger.get("applied_count", ops_agenda.get("applied_action_count", 0)),
        "failed_action_count": current_env_action_ledger.get("failed_count", ops_agenda.get("failed_action_count", 0)),
        "skipped_action_count": current_env_action_ledger.get("skipped_count", 0),
        "superseded_action_count": current_env_action_ledger.get("superseded_count", 0),
        "next_action_kind": ops_agenda_highest_action,
        "next_action_due_at": current_env_due_section.get("next_due_at"),
        "latest_watch_execute_manifest": current_env_watch_execute.get("execute_manifest_path"),
        "latest_watch_apply_manifest": current_env_watch_apply.get("apply_manifest_path"),
        "latest_reproposal_execute_manifest": current_env_reproposal_execute.get("execute_manifest_path"),
    }
    current_env_watch_apply_result_section = {
        "latest_execution_class": current_env_watch_execute.get("execution_class"),
        "observed_watch_status": current_env_watch_apply.get("observed_watch_status")
        or current_env_watch_execute.get("watch_status_observed"),
        "observed_watch_confidence": current_env_watch_apply.get("observed_watch_confidence")
        or current_env_watch_execute.get("watch_confidence_observed"),
        "new_guardrail_state": current_env_watch_apply.get("new_state"),
        "next_monitoring_due_at": current_env_watch_apply.get("updated_next_monitoring_due_at")
        or current_env_due_section.get("next_monitoring_due_at"),
        "next_reproposal_due_at": current_env_watch_apply.get("updated_next_reproposal_due_at")
        or current_env_due_section.get("next_reproposal_due_at"),
        "recommended_action_current_env": current_env_watch_apply.get("recommended_action_current_env"),
        "next_operator_action": current_env_watch_apply.get("next_operator_action"),
        "execution_verdict": current_env_watch_execute.get("execution_verdict"),
        "apply_status": current_env_watch_apply.get("action_status"),
        "trigger_gate_verdict": current_env_watch_apply.get("trigger_gate_verdict")
        or current_env_reproposal_execute.get("gate_verdict"),
        "rationale": list(current_env_watch_apply.get("rationale", [])),
    }
    current_env_action_ledger_section = {
        "total_action_count": current_env_action_ledger.get("total_action_count", 0),
        "planned_count": current_env_action_ledger.get("planned_count", 0),
        "executed_count": current_env_action_ledger.get("executed_count", 0),
        "applied_count": current_env_action_ledger.get("applied_count", 0),
        "failed_count": current_env_action_ledger.get("failed_count", 0),
        "skipped_count": current_env_action_ledger.get("skipped_count", 0),
        "superseded_count": current_env_action_ledger.get("superseded_count", 0),
        "deferred_count": current_env_action_ledger.get("deferred_count", 0),
        "rejected_count": current_env_action_ledger.get("rejected_count", 0),
        "closed_count": current_env_action_ledger.get("closed_count", 0),
        "retry_pending_count": current_env_action_ledger.get("retry_pending_count", 0),
        "latest_applied_action_id": current_env_action_ledger.get("latest_applied_action_id"),
        "next_planned_action_id": current_env_action_ledger.get("next_planned_action_id"),
        "ledger_hash": current_env_action_ledger.get("ledger_hash"),
    }
    current_env_retry_plan_section = {
        "plan_verdict": current_env_retry_plan.get("plan_verdict", "NOT_RUN"),
        "retryable_count": current_env_retry_plan.get("retryable_count", 0),
        "escalation_count": current_env_retry_plan.get("escalation_count", 0),
        "next_retry_at": current_env_retry_plan.get("next_retry_at"),
        "entry_count": current_env_retry_plan.get("entry_count", 0),
    }
    current_env_reproposal_handoff_section = {
        "approval_ready": bool(current_env_reproposal_handoff.get("approval_ready", False)),
        "handoff_status": current_env_reproposal_handoff.get("handoff_status", "NOT_RUN"),
        "gate_verdict": current_env_reproposal_handoff.get("gate_verdict"),
        "raw_gate_verdict": current_env_reproposal_handoff.get("raw_gate_verdict"),
        "next_action_kind": current_env_reproposal_handoff.get("next_action_kind"),
        "recommended_approval_command": current_env_reproposal_handoff.get("recommended_approval_command"),
        "approval_blockers": current_env_reproposal_handoff.get("approval_blockers", []),
    }
    latest_decision = current_env_operator_decision or {}
    latest_decision_apply = current_env_operator_decision_apply or {}
    current_env_operator_decisions_section = {
        "pending_decision_count": int(ops_agenda.get("pending_decision_count", 0) or 0),
        "approved_decision_count": 1 if latest_decision.get("decision") == OPERATOR_DECISION_APPROVE else 0,
        "skipped_decision_count": 1 if latest_decision.get("decision") == OPERATOR_DECISION_SKIP else 0,
        "deferred_decision_count": 1 if latest_decision.get("decision") == OPERATOR_DECISION_DEFER else 0,
        "rejected_decision_count": 1 if latest_decision.get("decision") == OPERATOR_DECISION_REJECT else 0,
        "latest_decision_id": latest_decision.get("decision_id"),
        "latest_decision": latest_decision.get("decision"),
        "latest_decision_valid": latest_decision.get("decision_valid"),
        "next_required_decision": "approve"
        if bool(current_env_reproposal_handoff.get("approval_ready", False))
        and not latest_decision_apply
        else None,
        "decision_apply_status": latest_decision_apply.get("closure_status"),
    }
    current_env_action_closure_section = {
        "open_action_count": max(
            0,
            int(current_env_action_ledger.get("total_action_count", 0) or 0)
            - int(current_env_action_ledger.get("closed_count", 0) or 0)
            - int(current_env_action_ledger.get("rejected_count", 0) or 0)
            - int(current_env_action_ledger.get("skipped_count", 0) or 0),
        ),
        "closed_action_count": current_env_action_ledger_compact.get("closed_action_count")
        if current_env_action_ledger_compact
        else current_env_action_ledger.get("closed_count", 0),
        "deferred_action_count": current_env_action_ledger_compact.get("deferred_action_count")
        if current_env_action_ledger_compact
        else current_env_action_ledger.get("deferred_count", 0),
        "failed_action_count": current_env_action_ledger_compact.get("failed_action_count")
        if current_env_action_ledger_compact
        else current_env_action_ledger.get("failed_count", 0),
        "retry_pending_count": current_env_action_ledger_compact.get("retry_pending_count")
        if current_env_action_ledger_compact
        else current_env_action_ledger.get("retry_pending_count", 0),
        "archived_action_count": current_env_action_ledger_archive.get("archived_action_count")
        if current_env_action_ledger_archive
        else current_env_action_ledger_compact.get("archived_action_count", 0),
        "approval_action_count": current_env_action_ledger_compact.get("approval_action_count", 0),
        "latest_closure_status": latest_decision_apply.get("closure_status"),
        "compaction_status": "COMPACTED" if current_env_action_ledger_compact else "NOT_RUN",
    }
    current_env_approval_runbook_section = {
        "approval_ready": bool(current_env_approval_runbook.get("approval_ready", False)),
        "approval_mode": current_env_approval_runbook.get("approval_mode", "NOT_RUN"),
        "executable": bool(current_env_approval_runbook.get("executable", False)),
        "recommended_command": current_env_approval_runbook.get("recommended_command", ""),
        "approval_blockers": current_env_approval_runbook.get("approval_blockers", []),
        "runbook_id": current_env_approval_runbook.get("runbook_id"),
        "budget_tag": current_env_approval_runbook.get("budget_tag"),
        "selected_budget_profile_id": current_env_approval_runbook.get("selected_budget_profile_id"),
        "proposed_budget_profile_id": current_env_approval_runbook.get("proposed_budget_profile_id"),
    }
    current_env_approval_execution_section = {
        "approval_status": current_env_approval_execution.get("approval_status", "NOT_RUN"),
        "approval_applied": bool(current_env_approval_execution.get("approval_status") == APPROVAL_STATUS_APPLIED),
        "approval_preflight_only": bool(current_env_approval_execution.get("approval_status") == APPROVAL_STATUS_DRY_RUN),
        "registry_updated": bool(current_env_approval_execution.get("registry_updated", False)),
        "baseline_written": bool(current_env_approval_execution.get("baseline_written", False)),
        "proposal_archived": bool(current_env_approval_execution.get("proposal_archived", False)),
        "approval_execution_id": current_env_approval_execution.get("approval_execution_id"),
        "approval_metadata_path": current_env_approval_execution.get("approval_metadata_path"),
        "next_operator_action": current_env_approval_execution.get("next_operator_action"),
        "approval_blockers": current_env_approval_execution.get("approval_blockers", []),
    }
    current_env_approval_link_section = {
        "approval_execution_id": current_env_approval_link.get("approval_execution_id"),
        "approval_status": current_env_approval_link.get("approval_status", "NOT_RUN"),
        "approval_applied": bool(current_env_approval_link.get("approval_applied", False)),
        "approval_preflight_only": bool(current_env_approval_link.get("approval_preflight_only", False)),
        "ledger_updated": bool(current_env_approval_link.get("ledger_updated", False)),
        "previous_closure_status": current_env_approval_link.get("previous_closure_status"),
        "new_closure_status": current_env_approval_link.get("new_closure_status"),
        "linked_budget_baseline": current_env_approval_link.get("linked_budget_baseline"),
        "linked_registry_update": bool(current_env_approval_link.get("linked_registry_update", False)),
    }

    current_fingerprint = str(runtime_refresh.get("runtime_fingerprint_key") or runtime_watch_refresh.get("runtime_fingerprint_key") or "")
    registry_health_current = dict(runtime_registry_health.get("current_environment", {}))
    approved_known_entries: list[dict[str, Any]] = []
    if runtime_registry_health.get("approved_known_environments", {}).get("entries"):
        for item in runtime_registry_health.get("approved_known_environments", {}).get("entries", []):
            if not isinstance(item, dict):
                continue
            health_status = str(item.get("health_status", "HEALTHY"))
            severity = "OK"
            recommendation = str(item.get("retirement_recommendation", "NO_ACTION"))
            if health_status in {"STALE", "REVERIFY_REQUIRED"}:
                severity = "WARN" if health_status == "STALE" else "ACTION_REQUIRED"
                recommendation = "REVERIFY_KNOWN_ENV"
            elif health_status == "RETIRE_CANDIDATE":
                severity = "ACTION_REQUIRED"
                recommendation = "RETIRE_KNOWN_ENV"
            elif health_status == "ORPHANED":
                severity = "ACTION_REQUIRED"
                recommendation = "KEEP_AS_ARCHIVE_ONLY"
            approved_known_entries.append(
                {
                    "environment_label": item.get("active_baseline_tag") or item.get("fingerprint_key"),
                    "environment_state": item.get("status", item.get("environment_state", "APPROVED_KNOWN_ENV")),
                    "health_status": health_status,
                    "severity": severity,
                    "recommendation": recommendation,
                    "selected_baseline_id": item.get("active_baseline_id"),
                    "selected_baseline_tag": item.get("active_baseline_tag"),
                    "comparability": item.get("comparability_verdict"),
                    "freshness": item.get("freshness_verdict"),
                    "watch_status": item.get("watch_status"),
                    "watch_confidence": item.get("watch_confidence"),
                    "proposal_needed": False,
                    "rationale": [str(item.get("watch_reason", ""))] if str(item.get("watch_reason", "")).strip() else [],
                    "runtime_fingerprint_key": item.get("fingerprint_key"),
                    "last_seen_timestamp": item.get("last_seen_timestamp"),
                    "age_since_last_verified_days": item.get("age_since_last_verified_days"),
                    "due_soon": bool(item.get("due_soon", False)),
                    "due_at": item.get("due_at"),
                    "overdue_days": item.get("overdue_days", 0.0),
                }
            )
    else:
        for item in approved_known_summaries:
            if not isinstance(item, dict):
                continue
            entry = {
                "environment_label": item.get("environment_label")
                or item.get("host_label")
                or item.get("baseline_tag")
                or item.get("fingerprint_key"),
                "environment_state": item.get("status", item.get("environment_state", "APPROVED_KNOWN_ENV")),
                "health_status": item.get("health_status", "HEALTHY"),
                "severity": item.get("severity")
                or ("OK" if str(item.get("comparability", item.get("comparability_verdict", ""))) == "COMPARABLE" else "ACTION_REQUIRED"),
                "recommendation": item.get("recommendation")
                or item.get("recommended_action")
                or item.get("runtime_recommendation")
                or "NO_ACTION",
                "selected_baseline_id": item.get("selected_baseline_id"),
                "selected_baseline_tag": item.get("selected_baseline_tag"),
                "comparability": item.get("comparability") or item.get("comparability_verdict"),
                "freshness": item.get("freshness") or item.get("freshness_verdict"),
                "watch_status": item.get("watch_status"),
                "watch_confidence": item.get("watch_confidence"),
                "proposal_needed": bool(item.get("proposal_needed", False)),
                "rationale": load_json_list_rationale(item),
                "runtime_fingerprint_key": item.get("runtime_fingerprint_key") or item.get("fingerprint_key"),
                "last_seen_timestamp": item.get("last_seen_timestamp"),
                "due_soon": bool(item.get("due_soon", False)),
                "due_at": item.get("due_at"),
                "overdue_days": item.get("overdue_days", 0.0),
            }
            approved_known_entries.append(entry)
        if not approved_known_entries:
            for entry in runtime_baseline_registry.get("entries", []):
                if not isinstance(entry, dict):
                    continue
                if str(entry.get("status", "")) != runtime_gate.REGISTRY_STATUS_ACTIVE:
                    continue
                entry_fingerprint = str(entry.get("runtime_fingerprint_key", entry.get("fingerprint_key", ""))).strip()
                if not entry_fingerprint or entry_fingerprint == current_fingerprint:
                    continue
                approved_known_entries.append(
                    {
                        "environment_label": entry.get("baseline_tag") or entry_fingerprint,
                        "environment_state": ENV_STATE_APPROVED_KNOWN_FRESH,
                        "health_status": "HEALTHY",
                        "severity": "OK",
                        "recommendation": "NO_ACTION",
                        "selected_baseline_id": entry.get("baseline_id"),
                        "selected_baseline_tag": entry.get("baseline_tag"),
                        "comparability": "COMPARABLE",
                        "freshness": "FRESH",
                        "watch_status": "CLEAR",
                        "watch_confidence": "MEDIUM",
                        "proposal_needed": False,
                        "rationale": ["approved baseline is active for this fingerprint lineage"],
                        "runtime_fingerprint_key": entry_fingerprint,
                        "last_seen_timestamp": entry.get("approval_timestamp_utc"),
                        "due_soon": False,
                        "due_at": None,
                        "overdue_days": 0.0,
                    }
                )
    retired_known_entries = [
        dict(item)
        for item in runtime_registry_health.get("retired_lineage_history", {}).get("entries", [])
        if isinstance(item, dict)
    ]
    approved_known_due_soon_count = sum(1 for item in approved_known_entries if bool(item.get("due_soon", False)))
    approved_known_reverify_required_count = sum(
        1 for item in approved_known_entries if str(item.get("health_status", "")) == "REVERIFY_REQUIRED"
    )
    approved_known_stale_count = sum(
        1 for item in approved_known_entries if str(item.get("health_status", "")) == "STALE"
    )
    approved_known_retire_candidate_count = sum(
        1 for item in approved_known_entries if str(item.get("health_status", "")) == "RETIRE_CANDIDATE"
    )
    approved_known_retired_count = len(retired_known_entries)
    next_due_known_env = None
    due_candidates = [
        item
        for item in approved_known_entries
        if item.get("due_at") or bool(item.get("due_soon", False)) or float(item.get("overdue_days", 0.0) or 0.0) > 0.0
    ]
    if due_candidates:
        next_due_known_env = min(
            due_candidates,
            key=lambda item: (
                str(item.get("due_at") or "9999-12-31T23:59:59Z"),
                str(item.get("selected_baseline_id") or item.get("environment_label") or ""),
            ),
        )
    approved_known_action = "NO_ACTION"
    approved_known_severity = "OK"
    if approved_known_entries:
        approved_known_severity = max(
            (str(item.get("severity", "OK")) for item in approved_known_entries),
            key=lambda value: {"OK": 0, "WARN": 1, "ACTION_REQUIRED": 2, "FAIL": 3}.get(value, 0),
        )
        approved_known_action = max(
            (str(item.get("recommendation", "NO_ACTION")) for item in approved_known_entries),
            key=lambda value: APPROVED_KNOWN_ACTION_RANK.get(value, 0),
        )
    if approved_known_action == "NO_ACTION" and approved_known_due_soon_count > 0:
        approved_known_action = "REVERIFY_KNOWN_ENV"
        approved_known_severity = "WARN"

    approved_known_conclusion = "no approved known environments are recorded beyond current env"
    approved_known_overall_conclusion = "approved known environments: none recorded"
    if approved_known_entries:
        if approved_known_action == "NO_ACTION":
            approved_known_conclusion = "approved known environments remain comparable and fresh"
            approved_known_overall_conclusion = "approved known environments: comparable and fresh"
        elif approved_known_action == "REVERIFY_KNOWN_ENV":
            approved_known_conclusion = (
                "approved known environments require same-fingerprint reverification before treating them as fresh"
            )
            approved_known_overall_conclusion = (
                "approved known environments: reverify same-fingerprint evidence before treating them as fresh"
            )
        elif approved_known_action == "RETIRE_KNOWN_ENV":
            approved_known_conclusion = (
                "approved known environments include stale lineages that should move to retirement or archive-only status"
            )
            approved_known_overall_conclusion = (
                "approved known environments: retire stale lineages that exceeded retention policy"
            )
        elif approved_known_action == "KEEP_AS_ARCHIVE_ONLY":
            approved_known_conclusion = (
                "approved known environments include archive-only lineages that must stay out of future selection"
            )
            approved_known_overall_conclusion = (
                "approved known environments: keep non-selectable lineages as archive only"
            )
        else:
            approved_known_conclusion = f"approved known environments require operator attention: {approved_known_action}"
            approved_known_overall_conclusion = f"approved known environments: {approved_known_action}"

    foreign_entries: list[dict[str, Any]] = []
    if runtime_registry_health.get("unapproved_foreign_environments", {}).get("entries"):
        for item in runtime_registry_health.get("unapproved_foreign_environments", {}).get("entries", []):
            if not isinstance(item, dict):
                continue
            foreign_entries.append(
                {
                    "environment_label": item.get("environment_label") or item.get("fingerprint_key"),
                    "environment_state": item.get("environment_state", "UNAPPROVED_FOREIGN"),
                    "severity": "ACTION_REQUIRED",
                    "recommendation": item.get("recommendation") or "REBASELINE_REQUIRED",
                    "selected_baseline_id": item.get("selected_baseline_id"),
                    "selected_baseline_tag": item.get("selected_baseline_tag"),
                    "comparability": item.get("comparability_verdict"),
                    "freshness": item.get("freshness_verdict"),
                    "watch_status": item.get("watch_status"),
                    "watch_confidence": item.get("watch_confidence"),
                    "proposal_needed": bool(item.get("proposal_needed", True)),
                    "rationale": [str(item.get("import_reason", ""))] if str(item.get("import_reason", "")).strip() else [],
                    "runtime_fingerprint_key": item.get("fingerprint_key"),
                }
            )
    else:
        for item in foreign_import_summaries:
            if not isinstance(item, dict):
                continue
            foreign_entries.append(
                {
                    "environment_label": item.get("host_label") or item.get("imported_fingerprint_key"),
                    "environment_state": item.get("environment_state", "UNAPPROVED_FOREIGN"),
                    "severity": "ACTION_REQUIRED" if bool(item.get("proposal_needed", True)) else "WARN",
                    "recommendation": "REBASELINE_REQUIRED" if bool(item.get("proposal_needed", True)) else "CONTINUE_MONITORING",
                    "selected_baseline_id": item.get("selected_baseline_id"),
                    "selected_baseline_tag": item.get("selected_baseline_tag"),
                    "comparability": item.get("comparability_verdict"),
                    "freshness": item.get("runtime_freshness_verdict"),
                    "watch_status": item.get("watch_status"),
                    "watch_confidence": item.get("watch_confidence"),
                    "proposal_needed": bool(item.get("proposal_needed", True)),
                    "rationale": [str(item.get("import_reason", ""))] if str(item.get("import_reason", "")).strip() else [],
                    "runtime_fingerprint_key": item.get("imported_fingerprint_key"),
                }
            )
    foreign_entries.append(
        {
            "environment_label": "matrix",
            "environment_state": "UNAPPROVED_FOREIGN",
            "severity": matrix_summary.get("severity"),
            "recommendation": new_env_action,
            "selected_baseline_id": None,
            "selected_baseline_tag": None,
            "comparability": matrix_summary.get("runtime_comparability_verdict") or matrix_summary.get("runtime_watch_status"),
            "freshness": matrix_summary.get("runtime_freshness_verdict"),
            "watch_status": matrix_summary.get("runtime_watch_status"),
            "watch_confidence": new_env_registry_summary.get("watch_confidence"),
            "proposal_needed": True,
            "rationale": load_json_list_rationale(matrix_summary),
            "runtime_fingerprint_key": None,
        }
    )

    payload = {
        "manifest_version": (
            "policy_ops_summary_v13"
            if (current_env_approval_runbook or current_env_approval_execution or current_env_approval_link)
            else "policy_ops_summary_v12"
            if (
                current_env_operator_decision
                or current_env_operator_decision_apply
                or current_env_action_ledger_compact
                or current_env_action_ledger_archive
            )
            else "policy_ops_summary_v11"
            if (current_env_action_ledger or current_env_retry_plan or current_env_reproposal_handoff)
            else "policy_ops_summary_v10"
            if (current_env_watch_execute or current_env_watch_apply or current_env_reproposal_execute)
            else "policy_ops_summary_v9"
            if (current_env_due or current_env_reproposal_plan or ops_agenda)
            else "policy_ops_summary_v8"
            if (
                current_env_guardrail_policy
                or current_env_age_tick
                or current_env_watch_plan
                or current_env_trigger_gate
                or runtime_budget_reproposal_history
                or runtime_budget_registry_summary
            )
            else "policy_ops_summary_v7"
            if (
                current_env_watch_refresh
                or current_env_watch_current
                or runtime_budget_current
                or runtime_budget_proposal
                or runtime_budget_proposal_gate
                or runtime_budget_baseline
                or runtime_budget_refresh
            )
            else "policy_ops_summary_v6"
            if str(runtime_registry_health.get("manifest_version", "")).strip() == "runtime_registry_health_v3"
            else "policy_ops_summary_v5"
        ),
        "phase": phase,
        "generated_at_utc": stable_manifest_timestamp(current_env_summary)
        or stable_manifest_timestamp(matrix_summary)
        or stable_manifest_timestamp(runtime_watch_refresh),
        "correctness_summary": {
            "severity": str(current_env_summary.get("policy_severity") or current_env_summary.get("severity") or ""),
            "verdict": str(current_env_summary.get("current_verdict", "")),
            "freshness_verdict": str(current_env_summary.get("freshness_verdict", "")),
            "family_statuses": extract_family_statuses(policy_manifest),
        },
        "current_env_summary": {
            "severity": current_env_summary.get("severity"),
            "recommendation": current_action,
            "watch_reason": current_env_watch_reason,
            "operator_conclusion": current_env_conclusion,
            "correctness_status": {
                "verdict": str(current_env_summary.get("current_verdict", "")),
                "freshness_verdict": str(current_env_summary.get("freshness_verdict", "")),
            },
            "runtime_status": {
                "comparability_verdict": current_env_summary.get("runtime_comparability_verdict"),
                "freshness_verdict": current_env_summary.get("runtime_freshness_verdict"),
                "budget_verdict": current_env_summary.get("runtime_budget_verdict"),
            },
            "watch_status": current_env_summary.get("runtime_watch_status") or runtime_watch_refresh.get("overall_watch_status"),
            "watch_recommendation": current_env_summary.get("runtime_watch_recommendation")
            or runtime_watch_refresh.get("overall_watch_recommendation"),
            "watch_confidence": current_env_registry_summary.get("watch_confidence"),
            "confidence_reason": current_env_registry_summary.get("confidence_reason"),
            "evidence_source_counts": current_env_registry_summary.get("evidence_source_counts", {}),
            "active_lineage_count": current_env_registry_summary.get("active_lineage_count", 0),
            "real_lineage_count": current_env_registry_summary.get("real_lineage_count", 0),
            "latest_real_sample_timestamp": current_env_registry_summary.get("latest_real_sample_timestamp"),
            "latest_fixture_sample_timestamp": current_env_registry_summary.get("latest_fixture_sample_timestamp"),
        },
        "new_env_summary": {
            "severity": matrix_summary.get("severity"),
            "recommendation": new_env_action,
            "operator_conclusion": new_env_conclusion,
            "comparability_bucket": matrix_summary.get("runtime_comparability_verdict")
            or matrix_summary.get("runtime_watch_status"),
            "watch_status": matrix_summary.get("runtime_watch_status"),
            "watch_recommendation": matrix_summary.get("runtime_watch_recommendation"),
            "watch_confidence": new_env_registry_summary.get("watch_confidence"),
            "confidence_reason": new_env_registry_summary.get("confidence_reason"),
            "evidence_source_counts": new_env_registry_summary.get("evidence_source_counts", {}),
            "active_lineage_count": new_env_registry_summary.get("active_lineage_count", 0),
            "real_lineage_count": new_env_registry_summary.get("real_lineage_count", 0),
            "latest_real_sample_timestamp": new_env_registry_summary.get("latest_real_sample_timestamp"),
            "latest_fixture_sample_timestamp": new_env_registry_summary.get("latest_fixture_sample_timestamp"),
            "matrix_entry_count": matrix_summary.get("matrix_entry_count"),
            "matrix_action_counts": matrix_summary.get("matrix_action_counts"),
        },
        "runtime_same_fingerprint_summary": {
            "selected_baseline_id": current_env_summary.get("runtime_selected_baseline_id"),
            "selected_baseline_tag": current_env_summary.get("runtime_selected_baseline_tag"),
            "comparability_verdict": current_env_summary.get("runtime_comparability_verdict"),
            "freshness_verdict": current_env_summary.get("runtime_freshness_verdict"),
            "budget_verdict": current_env_summary.get("runtime_budget_verdict"),
            "watch_status": current_env_summary.get("runtime_watch_status") or runtime_watch_refresh.get("overall_watch_status"),
            "watch_recommendation": current_env_summary.get("runtime_watch_recommendation")
            or runtime_watch_refresh.get("overall_watch_recommendation"),
            "diagnostic_watch_only": runtime_watch_refresh.get("diagnostic_watch_only"),
            "watch_confidence": current_env_registry_summary.get("watch_confidence"),
            "confidence_reason": current_env_registry_summary.get("confidence_reason"),
        },
        "runtime_cross_fingerprint_summary": {
            "matrix_severity": matrix_summary.get("severity"),
            "matrix_verdict": matrix_summary.get("runtime_watch_status"),
            "matrix_entry_count": matrix_summary.get("matrix_entry_count"),
            "matrix_watch_status_counts": matrix_summary.get("matrix_watch_status_counts"),
            "matrix_watch_recommendation_counts": matrix_summary.get("matrix_watch_recommendation_counts"),
            "matrix_action_counts": matrix_summary.get("matrix_action_counts"),
            "matrix_severity_counts": matrix_summary.get("matrix_severity_counts"),
            "watch_confidence": new_env_registry_summary.get("watch_confidence"),
            "confidence_reason": new_env_registry_summary.get("confidence_reason"),
        },
        "watch_registry_summary": {
            "entry_count": watch_registry.get("entry_count", 0),
            "fingerprint_count": watch_registry.get("fingerprint_count", 0),
            "status_counts": watch_registry.get("status_counts", {}),
            "recommendation_counts": watch_registry.get("recommendation_counts", {}),
            "comparability_counts": watch_registry.get("comparability_counts", {}),
            "role_counts": watch_registry.get("role_counts", {}),
            "evidence_source_counts": watch_registry.get("evidence_source_counts", {}),
            "confidence_counts": watch_registry.get("confidence_counts", {}),
            "active_lineage_count": watch_registry.get("active_lineage_count", 0),
            "real_lineage_count": watch_registry.get("real_lineage_count", 0),
        },
        "current_environment": {
            "severity": current_env_summary.get("severity"),
            "correctness_status": {
                "severity": correctness_severity,
                "verdict": correctness_verdict,
                "freshness_verdict": str(current_env_summary.get("freshness_verdict", "")),
            },
            "runtime_status": {
                "comparability": runtime_comparability,
                "freshness": runtime_freshness,
                "budget_verdict": runtime_budget,
            },
            "recommended_action_current_env": current_action,
            "watch_status": current_env_summary.get("runtime_watch_status") or runtime_watch_refresh.get("overall_watch_status"),
            "watch_confidence": current_env_registry_summary.get("watch_confidence"),
            "selected_baseline_id": current_env_summary.get("runtime_selected_baseline_id"),
            "selected_baseline_tag": current_env_summary.get("runtime_selected_baseline_tag"),
            "proposal_needed": bool(current_env_summary.get("runtime_rebaseline_proposal_needed", False)),
            "operator_conclusion": current_env_conclusion,
            "rationale": [item for item in rationale if "new or foreign" not in item],
            "registry_health_status": registry_health_current.get("health_status", "HEALTHY"),
        },
        "current_env_watch_budget": current_env_watch_budget_section,
        "current_env_guardrail": current_env_guardrail_section,
        "current_env_due": current_env_due_section,
        "current_env_actions": current_env_actions_section,
        "current_env_watch_apply_result": current_env_watch_apply_result_section,
        "current_env_action_ledger": current_env_action_ledger_section,
        "current_env_retry_plan": current_env_retry_plan_section,
        "current_env_reproposal_handoff": current_env_reproposal_handoff_section,
        "current_env_approval_runbook": current_env_approval_runbook_section,
        "current_env_approval_execution": current_env_approval_execution_section,
        "current_env_approval_link": current_env_approval_link_section,
        "current_env_operator_decisions": current_env_operator_decisions_section,
        "current_env_action_closure": current_env_action_closure_section,
        "current_env_reproposal_plan": {
            "plan_verdict": current_env_reproposal_plan.get("plan_verdict", "NOT_RUN"),
            "entry_count": current_env_reproposal_plan.get("entry_count", 0),
            "reproposal_due_state": current_env_reproposal_plan.get("reproposal_due_state"),
            "reproposal_due_at": current_env_reproposal_plan.get("reproposal_due_at"),
            "entries": current_env_reproposal_plan.get("entries", []),
        },
        "ops_agenda": {
            "manifest_version": ops_agenda.get("manifest_version"),
            "item_count": ops_agenda.get("item_count", 0),
            "action_required_count": ops_agenda.get("action_required_count", 0),
            "blocking_action_count": ops_agenda.get("blocking_action_count", 0),
            "planned_action_count": ops_agenda.get("planned_action_count", 0),
            "executed_action_count": ops_agenda.get("executed_action_count", 0),
            "applied_action_count": ops_agenda.get("applied_action_count", 0),
            "failed_action_count": ops_agenda.get("failed_action_count", 0),
            "highest_priority_domain": ops_agenda.get("highest_priority_domain"),
            "highest_priority_action": ops_agenda.get("highest_priority_action"),
            "items": ops_agenda.get("items", []),
        },
        "approved_known_environments": {
            "severity": approved_known_severity,
            "recommended_action_known_envs": approved_known_action,
            "environment_count": len(approved_known_entries),
            "fresh_count": sum(1 for item in approved_known_entries if str(item.get("environment_state", "")) == ENV_STATE_APPROVED_KNOWN_FRESH),
            "healthy_count": sum(1 for item in approved_known_entries if str(item.get("health_status", "")) == "HEALTHY"),
            "due_soon_count": approved_known_due_soon_count,
            "stale_count": approved_known_stale_count,
            "reverify_required_count": approved_known_reverify_required_count,
            "retire_candidate_count": approved_known_retire_candidate_count,
            "retired_count": approved_known_retired_count,
            "entries": approved_known_entries,
        },
        "approved_known_env_actions": {
            "due_soon_reverify_count": approved_known_due_soon_count,
            "reverify_required_count": approved_known_reverify_required_count,
            "stale_count": approved_known_stale_count,
            "retire_candidate_count": approved_known_retire_candidate_count,
            "retired_count": approved_known_retired_count,
            "recommended_action_known_envs": approved_known_action,
            "next_due_known_env_id": None if next_due_known_env is None else next_due_known_env.get("selected_baseline_id"),
            "next_due_at": None if next_due_known_env is None else next_due_known_env.get("due_at"),
        },
        "unapproved_foreign_environments": {
            "severity": matrix_summary.get("severity"),
            "recommended_action_foreign_envs": new_env_action,
            "environment_count": len(foreign_entries),
            "candidate_count": len(foreign_entries),
            "action_required_count": sum(1 for item in foreign_entries if str(item.get("severity", "")) == "ACTION_REQUIRED"),
            "rebaseline_required_count": sum(1 for item in foreign_entries if str(item.get("recommendation", "")) == "REBASELINE_REQUIRED"),
            "entries": foreign_entries,
        },
        "new_environment_matrix": {
            "severity": matrix_summary.get("severity"),
            "recommended_action_new_env": new_env_action,
            "comparability_buckets": matrix_summary.get("matrix_comparability_counts", {}),
            "rebaseline_required_counts": matrix_summary.get("matrix_action_counts", {}),
            "operator_conclusion": new_env_conclusion,
        },
        "runtime_registry_health": {
            "status": runtime_registry_health.get("overall_status", "NOT_RUN"),
            "current_active_count": runtime_registry_health.get("aggregate", {}).get("current_active_count", 0),
            "active_fingerprint_count": runtime_registry_health.get("aggregate", {}).get("active_fingerprint_count", 0),
            "approved_known_fresh_count": runtime_registry_health.get("aggregate", {}).get("approved_known_fresh_count", 0),
            "approved_known_due_soon_count": runtime_registry_health.get("aggregate", {}).get("approved_known_due_soon_count", 0),
            "approved_known_reverify_required_count": runtime_registry_health.get("aggregate", {}).get("approved_known_reverify_required_count", 0),
            "approved_known_stale_count": runtime_registry_health.get("aggregate", {}).get("approved_known_stale_count", 0),
            "healthy_active_count": runtime_registry_health.get("aggregate", {}).get("healthy_active_count", 0),
            "healthy_known_env_count": runtime_registry_health.get("aggregate", {}).get("healthy_known_env_count", 0),
            "stale_known_env_count": runtime_registry_health.get("aggregate", {}).get("stale_known_env_count", 0),
            "stale_active_count": runtime_registry_health.get("aggregate", {}).get("stale_active_count", 0),
            "reverify_required_count": runtime_registry_health.get("aggregate", {}).get("reverify_required_count", 0),
            "governance_action_required_count": runtime_registry_health.get("aggregate", {}).get("governance_action_required_count", 0),
            "retire_candidate_count": runtime_registry_health.get("aggregate", {}).get("retire_candidate_count", 0),
            "retired_count": runtime_registry_health.get("aggregate", {}).get("retired_count", 0),
            "retired_entry_count": runtime_registry_health.get("aggregate", {}).get("retired_entry_count", 0),
            "orphaned_entry_count": runtime_registry_health.get("aggregate", {}).get("orphaned_entry_count", 0),
            "foreign_unapproved_count": runtime_registry_health.get("aggregate", {}).get("foreign_unapproved_count", 0),
            "approved_known_env_count": runtime_registry_health.get("aggregate", {}).get("approved_known_env_count", 0),
        },
        "publication_health": {
            "status": publication_health.get("status", "NOT_RUN"),
            "missing_artifact_count": publication_health.get("missing_artifact_count", 0),
            "hash_mismatch_count": publication_health.get("hash_mismatch_count", 0),
            "dangling_reference_count": publication_health.get("dangling_reference_count", 0),
        },
        "verification_lane": {
            "status": verification_lane_status,
            "recommendation": verification_action,
            "source_snapshot_hash": source_snapshot_manifest.get("snapshot_hash"),
            "staged_mirror_hash": staged_mirror_verify.get("staged_mirror_hash"),
            "verification_release_hash": verification_closeout.get("verification_release_hash"),
            "verification_debug_hash": verification_closeout.get("verification_debug_hash"),
            "verification_asan_hash": verification_closeout.get("verification_asan_hash"),
            "verification_not_run_count": verification_closeout.get("verification_not_run_count", 0),
            "published_snapshot_id": published_snapshot_manifest.get("publication_snapshot_id")
            or published_snapshot_manifest.get("phase_tag"),
            "closeout_verdict": verification_closeout.get("closeout_verdict")
            or verification_closeout.get("verification_closeout_status")
            or verification_lane_status,
            "release_execution_verdict": verification_release.get("execution_verdict", "NOT_RUN"),
            "debug_execution_verdict": verification_debug.get("execution_verdict", "NOT_RUN"),
            "asan_execution_verdict": verification_asan.get("execution_verdict", "NOT_RUN"),
        },
        "overall_summary": {
            "exact_shadow_policy_status": "healthy"
            if str(current_env_summary.get("current_verdict", "")) == "PASS"
            and str(current_env_summary.get("freshness_verdict", "")) == "FRESH"
            else "attention_required",
            "runtime_baseline_registry_health": "healthy"
            if str(current_env_summary.get("runtime_comparability_verdict", "")) == "COMPARABLE"
            and str(current_env_summary.get("runtime_freshness_verdict", "")) == "FRESH"
            else "attention_required",
            "stale_family_count": int(current_env_summary.get("stale_family_count", 0)),
            "stale_runtime_lineage_count": stale_runtime_lineage_count,
            "required_operator_actions": [
                current_action,
                current_env_guardrail_section.get("recommended_action_current_env"),
                current_env_due_section.get("recommended_action_current_env"),
                ops_agenda_highest_action,
                approved_known_action,
                new_env_action,
                publication_health.get("status", "NOT_RUN"),
                verification_action,
            ],
            "current_env_operator_conclusion": current_env_conclusion,
            "current_env_guardrail_operator_conclusion": "current env guardrail remains clear"
            if current_env_guardrail_section.get("recommended_action_current_env") in {"", "NO_ACTION"}
            else f"current env guardrail: {current_env_guardrail_section.get('recommended_action_current_env')}",
            "current_env_due_operator_conclusion": current_env_due_conclusion,
            "ops_agenda_operator_conclusion": ops_agenda_conclusion,
            "approved_known_env_operator_conclusion": approved_known_overall_conclusion,
            "new_env_operator_conclusion": new_env_conclusion,
            "publication_operator_conclusion": "publication snapshot health is healthy"
            if publication_health.get("status") in {"", None, "NOT_RUN", "HEALTHY"}
            else "publication snapshot requires repair before treating published artifacts as authoritative",
            "verification_lane_operator_conclusion": verification_conclusion,
        },
        "final_operator_summary": {
            "combined_severity_current_env": current_env_summary.get("severity"),
            "combined_severity_known_envs": approved_known_severity,
            "combined_severity_cross_env": matrix_summary.get("severity"),
            "recommended_action_current_env": current_action,
            "recommended_action_current_env_guardrail": current_env_guardrail_section.get("recommended_action_current_env"),
            "recommended_action_current_env_due": current_env_due_section.get("recommended_action_current_env"),
            "recommended_command_current_env_due": current_env_due_section.get("recommended_command"),
            "recommended_action_ops_agenda": ops_agenda_highest_action,
            "ops_agenda_highest_priority_domain": ops_agenda_highest_domain,
            "current_env_action_planned_count": current_env_actions_section.get("planned_action_count"),
            "current_env_action_executed_count": current_env_actions_section.get("executed_action_count"),
            "current_env_action_applied_count": current_env_actions_section.get("applied_action_count"),
            "current_env_action_failed_count": current_env_actions_section.get("failed_action_count"),
            "current_env_action_skipped_count": current_env_actions_section.get("skipped_action_count"),
            "current_env_action_superseded_count": current_env_actions_section.get("superseded_action_count"),
            "current_env_watch_apply_new_state": current_env_watch_apply_result_section.get("new_guardrail_state"),
            "current_env_watch_apply_next_monitoring_due_at": current_env_watch_apply_result_section.get("next_monitoring_due_at"),
            "current_env_watch_apply_next_reproposal_due_at": current_env_watch_apply_result_section.get("next_reproposal_due_at"),
            "current_env_retryable_action_count": current_env_retry_plan_section.get("retryable_count"),
            "current_env_retry_escalation_count": current_env_retry_plan_section.get("escalation_count"),
            "current_env_reproposal_approval_ready": current_env_reproposal_handoff_section.get("approval_ready"),
            "current_env_reproposal_next_action": current_env_reproposal_handoff_section.get("next_action_kind"),
            "current_env_approval_runbook_ready": current_env_approval_runbook_section.get("approval_ready"),
            "current_env_approval_execution_status": current_env_approval_execution_section.get("approval_status"),
            "current_env_approval_applied": current_env_approval_execution_section.get("approval_applied"),
            "current_env_approval_registry_updated": current_env_approval_execution_section.get("registry_updated"),
            "current_env_approval_next_operator_action": current_env_approval_execution_section.get("next_operator_action"),
            "current_env_pending_decision_count": current_env_operator_decisions_section.get("pending_decision_count"),
            "current_env_latest_decision_id": current_env_operator_decisions_section.get("latest_decision_id"),
            "current_env_latest_decision": current_env_operator_decisions_section.get("latest_decision"),
            "current_env_open_action_count": current_env_action_closure_section.get("open_action_count"),
            "current_env_closed_action_count": current_env_action_closure_section.get("closed_action_count"),
            "current_env_archived_action_count": current_env_action_closure_section.get("archived_action_count"),
            "recommended_action_known_envs": approved_known_action,
            "recommended_action_new_env": new_env_action,
            "current_env_operator_conclusion": current_env_conclusion,
            "current_env_guardrail_conclusion": "current env guardrail remains clear"
            if current_env_guardrail_section.get("recommended_action_current_env") in {"", "NO_ACTION"}
            else f"current env guardrail: {current_env_guardrail_section.get('recommended_action_current_env')}",
            "current_env_due_conclusion": current_env_due_conclusion,
            "ops_agenda_conclusion": ops_agenda_conclusion,
            "known_env_operator_conclusion": approved_known_conclusion,
            "new_env_operator_conclusion": new_env_conclusion,
            "publication_operator_conclusion": "publication snapshot health is healthy"
            if publication_health.get("status") in {"", None, "NOT_RUN", "HEALTHY"}
            else "publication snapshot has missing artifacts, hash mismatches, or dangling references",
            "verification_lane_status": verification_lane_status,
            "verification_operator_conclusion": verification_conclusion,
            "rationale": rationale,
            "structured_rationale": structured_rationale,
        },
        "final_operator_actions": {
            "recommended_action_current_env": current_action,
            "recommended_action_current_env_guardrail": current_env_guardrail_section.get("recommended_action_current_env"),
            "recommended_action_current_env_due": current_env_due_section.get("recommended_action_current_env"),
            "recommended_command_current_env_due": current_env_due_section.get("recommended_command"),
            "recommended_action_ops_agenda": ops_agenda_highest_action,
            "ops_agenda_highest_priority_domain": ops_agenda_highest_domain,
            "current_env_action_planned_count": current_env_actions_section.get("planned_action_count"),
            "current_env_action_executed_count": current_env_actions_section.get("executed_action_count"),
            "current_env_action_applied_count": current_env_actions_section.get("applied_action_count"),
            "current_env_action_failed_count": current_env_actions_section.get("failed_action_count"),
            "current_env_action_skipped_count": current_env_actions_section.get("skipped_action_count"),
            "current_env_action_superseded_count": current_env_actions_section.get("superseded_action_count"),
            "current_env_watch_apply_new_state": current_env_watch_apply_result_section.get("new_guardrail_state"),
            "current_env_watch_apply_next_monitoring_due_at": current_env_watch_apply_result_section.get("next_monitoring_due_at"),
            "current_env_watch_apply_next_reproposal_due_at": current_env_watch_apply_result_section.get("next_reproposal_due_at"),
            "current_env_retryable_action_count": current_env_retry_plan_section.get("retryable_count"),
            "current_env_retry_escalation_count": current_env_retry_plan_section.get("escalation_count"),
            "current_env_reproposal_approval_ready": current_env_reproposal_handoff_section.get("approval_ready"),
            "current_env_reproposal_next_action": current_env_reproposal_handoff_section.get("next_action_kind"),
            "current_env_approval_runbook_ready": current_env_approval_runbook_section.get("approval_ready"),
            "current_env_approval_execution_status": current_env_approval_execution_section.get("approval_status"),
            "current_env_approval_applied": current_env_approval_execution_section.get("approval_applied"),
            "current_env_approval_registry_updated": current_env_approval_execution_section.get("registry_updated"),
            "current_env_approval_next_operator_action": current_env_approval_execution_section.get("next_operator_action"),
            "current_env_pending_decision_count": current_env_operator_decisions_section.get("pending_decision_count"),
            "current_env_latest_decision_id": current_env_operator_decisions_section.get("latest_decision_id"),
            "current_env_latest_decision": current_env_operator_decisions_section.get("latest_decision"),
            "current_env_open_action_count": current_env_action_closure_section.get("open_action_count"),
            "current_env_closed_action_count": current_env_action_closure_section.get("closed_action_count"),
            "current_env_archived_action_count": current_env_action_closure_section.get("archived_action_count"),
            "recommended_action_known_envs": approved_known_action,
            "recommended_action_foreign_envs": new_env_action,
            "recommended_action_publication": "NO_ACTION"
            if publication_health.get("status") in {"", None, "NOT_RUN", "HEALTHY"}
            else "REPAIR_PUBLICATION",
            "recommended_action_verification": verification_action,
            "current_env_operator_conclusion": current_env_conclusion,
            "current_env_guardrail_conclusion": "current env guardrail remains clear"
            if current_env_guardrail_section.get("recommended_action_current_env") in {"", "NO_ACTION"}
            else f"current env guardrail: {current_env_guardrail_section.get('recommended_action_current_env')}",
            "current_env_due_conclusion": current_env_due_conclusion,
            "ops_agenda_conclusion": ops_agenda_conclusion,
            "known_env_operator_conclusion": approved_known_conclusion,
            "foreign_env_operator_conclusion": new_env_conclusion,
            "publication_operator_conclusion": "publication snapshot health is healthy"
            if publication_health.get("status") in {"", None, "NOT_RUN", "HEALTHY"}
            else "publication snapshot has missing artifacts, hash mismatches, or dangling references",
            "verification_operator_conclusion": verification_conclusion,
            "rationale": rationale,
            "structured_rationale": structured_rationale,
        },
    }
    payload["ops_summary_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_ops_summary_text(summary: dict[str, Any]) -> str:
    final_summary = summary.get("final_operator_summary", {})
    approved_known_actions = summary.get("approved_known_env_actions", {})
    current_env_watch_budget = summary.get("current_env_watch_budget", {})
    current_env_guardrail = summary.get("current_env_guardrail", {})
    current_env_due = summary.get("current_env_due", {})
    current_env_actions = summary.get("current_env_actions", {})
    current_env_watch_apply = summary.get("current_env_watch_apply_result", {})
    current_env_action_ledger = summary.get("current_env_action_ledger", {})
    current_env_retry_plan = summary.get("current_env_retry_plan", {})
    current_env_reproposal_handoff = summary.get("current_env_reproposal_handoff", {})
    current_env_approval_runbook = summary.get("current_env_approval_runbook", {})
    current_env_approval_execution = summary.get("current_env_approval_execution", {})
    current_env_approval_link = summary.get("current_env_approval_link", {})
    current_env_operator_decisions = summary.get("current_env_operator_decisions", {})
    current_env_action_closure = summary.get("current_env_action_closure", {})
    current_env_operator_runbooks = summary.get("current_env_operator_runbooks", {})
    current_env_decision_variants = summary.get("current_env_decision_variants", {})
    operator_runbook_catalog = summary.get("operator_runbook_catalog", {})
    operator_decision_metadata_audit = summary.get("operator_decision_metadata_audit", {})
    approval_execution_guardrail = summary.get("approval_execution_guardrail", {})
    operator_runbook_replay = summary.get("operator_runbook_replay", {})
    staged_materialization_transaction = summary.get("staged_materialization_transaction", {})
    source_health = summary.get("source_health", {})
    staged_materialization = summary.get("staged_materialization", {})
    ops_agenda = summary.get("ops_agenda", {})
    lines = [
        f"manifest_version={summary.get('manifest_version', '')}",
        f"phase={summary.get('phase', '')}",
        f"generated_at_utc={summary.get('generated_at_utc', '')}",
        f"correctness_severity={summary.get('correctness_summary', {}).get('severity', '')}",
        f"correctness_verdict={summary.get('correctness_summary', {}).get('verdict', '')}",
        f"runtime_same_fingerprint_watch_status={summary.get('runtime_same_fingerprint_summary', {}).get('watch_status', '')}",
        f"runtime_same_fingerprint_watch_recommendation={summary.get('runtime_same_fingerprint_summary', {}).get('watch_recommendation', '')}",
        f"runtime_cross_fingerprint_matrix_severity={summary.get('runtime_cross_fingerprint_summary', {}).get('matrix_severity', '')}",
        f"runtime_cross_fingerprint_matrix_verdict={summary.get('runtime_cross_fingerprint_summary', {}).get('matrix_verdict', '')}",
        f"combined_severity_current_env={final_summary.get('combined_severity_current_env', '')}",
        f"combined_severity_known_envs={final_summary.get('combined_severity_known_envs', '')}",
        f"combined_severity_cross_env={final_summary.get('combined_severity_cross_env', '')}",
        f"recommended_action_current_env={final_summary.get('recommended_action_current_env', '')}",
        f"recommended_action_current_env_guardrail={final_summary.get('recommended_action_current_env_guardrail', '')}",
        f"recommended_action_current_env_due={final_summary.get('recommended_action_current_env_due', '')}",
        f"recommended_action_ops_agenda={final_summary.get('recommended_action_ops_agenda', '')}",
        f"ops_agenda_highest_priority_domain={final_summary.get('ops_agenda_highest_priority_domain', '')}",
        f"recommended_action_known_envs={final_summary.get('recommended_action_known_envs', '')}",
        f"recommended_action_new_env={final_summary.get('recommended_action_new_env', '')}",
        f"current_env_budget_watch_status={current_env_watch_budget.get('watch_status', '')}",
        f"current_env_budget_watch_confidence={current_env_watch_budget.get('watch_confidence', '')}",
        f"current_env_budget_selected_profile_id={current_env_watch_budget.get('selected_budget_profile_id', '')}",
        f"current_env_budget_reproposal_needed={int(bool(current_env_watch_budget.get('reproposal_needed', False)))}",
        f"current_env_budget_reproposal_gate_verdict={current_env_watch_budget.get('reproposal_gate_verdict', '')}",
        f"current_env_guardrail_state={current_env_guardrail.get('current_state', '')}",
        f"current_env_guardrail_approval_grace_active={int(bool(current_env_guardrail.get('approval_grace_active', False)))}",
        f"current_env_guardrail_next_monitoring_due_at={current_env_guardrail.get('next_monitoring_due_at', '')}",
        f"current_env_guardrail_next_reproposal_due_at={current_env_guardrail.get('next_reproposal_due_at', '')}",
        f"current_env_guardrail_trigger_gate_verdict={current_env_guardrail.get('trigger_gate_verdict', '')}",
        f"current_env_due_monitoring_due_state={current_env_due.get('monitoring_due_state', '')}",
        f"current_env_due_reproposal_due_state={current_env_due.get('reproposal_due_state', '')}",
        f"current_env_due_next_due_kind={current_env_due.get('next_due_kind', '')}",
        f"current_env_due_next_due_at={current_env_due.get('next_due_at', '')}",
        f"current_env_due_recommended_action={current_env_due.get('recommended_action_current_env', '')}",
        f"current_env_actions_planned={current_env_actions.get('planned_action_count', 0)}",
        f"current_env_actions_executed={current_env_actions.get('executed_action_count', 0)}",
        f"current_env_actions_applied={current_env_actions.get('applied_action_count', 0)}",
        f"current_env_actions_failed={current_env_actions.get('failed_action_count', 0)}",
        f"current_env_action_ledger_total={current_env_action_ledger.get('total_action_count', 0)}",
        f"current_env_action_ledger_latest_applied={current_env_action_ledger.get('latest_applied_action_id', '')}",
        f"current_env_retryable_count={current_env_retry_plan.get('retryable_count', 0)}",
        f"current_env_retry_escalation_count={current_env_retry_plan.get('escalation_count', 0)}",
        f"current_env_reproposal_handoff_status={current_env_reproposal_handoff.get('handoff_status', '')}",
        f"current_env_reproposal_approval_ready={int(bool(current_env_reproposal_handoff.get('approval_ready', False)))}",
        f"current_env_approval_runbook_ready={int(bool(current_env_approval_runbook.get('approval_ready', False)))}",
        f"current_env_approval_runbook_mode={current_env_approval_runbook.get('approval_mode', '')}",
        f"current_env_approval_execution_status={current_env_approval_execution.get('approval_status', '')}",
        f"current_env_approval_execution_applied={int(bool(current_env_approval_execution.get('approval_applied', False)))}",
        f"current_env_approval_registry_updated={int(bool(current_env_approval_execution.get('registry_updated', False)))}",
        f"current_env_approval_link_status={current_env_approval_link.get('approval_status', '')}",
        f"current_env_approval_link_ledger_updated={int(bool(current_env_approval_link.get('ledger_updated', False)))}",
        f"current_env_operator_pending_decision_count={current_env_operator_decisions.get('pending_decision_count', 0)}",
        f"current_env_operator_latest_decision_id={current_env_operator_decisions.get('latest_decision_id', '')}",
        f"current_env_operator_latest_decision={current_env_operator_decisions.get('latest_decision', '')}",
        f"current_env_closure_open_action_count={current_env_action_closure.get('open_action_count', 0)}",
        f"current_env_closure_closed_action_count={current_env_action_closure.get('closed_action_count', 0)}",
        f"current_env_closure_archived_action_count={current_env_action_closure.get('archived_action_count', 0)}",
        f"current_env_closure_compaction_status={current_env_action_closure.get('compaction_status', '')}",
        f"current_env_operator_runbook_pending_count={current_env_operator_runbooks.get('pending_runbook_count', 0)}",
        f"current_env_operator_runbook_executable_count={current_env_operator_runbooks.get('executable_runbook_count', 0)}",
        f"current_env_operator_runbook_integrated_opt_in_required_count={current_env_operator_runbooks.get('integrated_opt_in_required_count', 0)}",
        f"current_env_operator_runbook_approval_count={current_env_operator_runbooks.get('approval_runbook_count', 0)}",
        f"current_env_operator_runbook_retry_count={current_env_operator_runbooks.get('retry_runbook_count', 0)}",
        f"current_env_decision_variant_skip_covered={int(bool(current_env_decision_variants.get('skip_covered', False)))}",
        f"current_env_decision_variant_defer_covered={int(bool(current_env_decision_variants.get('defer_covered', False)))}",
        f"current_env_decision_variant_reject_covered={int(bool(current_env_decision_variants.get('reject_covered', False)))}",
        f"current_env_decision_variant_retry_now_covered={int(bool(current_env_decision_variants.get('retry_now_covered', False)))}",
        f"operator_runbook_catalog_active_count={operator_runbook_catalog.get('active_runbook_count', 0)}",
        f"operator_runbook_catalog_resolved_count={operator_runbook_catalog.get('resolved_runbook_count', 0)}",
        f"operator_runbook_catalog_integrated_opt_in_required_count={operator_runbook_catalog.get('integrated_opt_in_required_count', 0)}",
        f"operator_decision_metadata_audit_verdict={operator_decision_metadata_audit.get('audit_verdict', '')}",
        f"operator_runbook_replay_verdict={operator_runbook_replay.get('replay_verdict', '')}",
        f"approval_guardrail_accidental_mutation_guard={approval_execution_guardrail.get('accidental_mutation_guard', '')}",
        f"approval_guardrail_handoff_only_default={int(bool(approval_execution_guardrail.get('handoff_only_default', False)))}",
        f"source_health_status={source_health.get('status', '')}",
        f"source_health_recommendation={source_health.get('recommendation', '')}",
        f"source_health_dataless_placeholder_count={source_health.get('dataless_placeholder_count', 0)}",
        f"staged_materialization_mode={staged_materialization.get('staged_materialization_mode', '')}",
        f"staged_materialization_verdict={staged_materialization.get('materialization_verdict', '')}",
        f"staged_materialization_transaction_verdict={staged_materialization_transaction.get('transaction_verdict', '')}",
        f"current_env_watch_apply_new_state={current_env_watch_apply.get('new_guardrail_state', '')}",
        f"current_env_watch_apply_next_monitoring_due_at={current_env_watch_apply.get('next_monitoring_due_at', '')}",
        f"current_env_watch_apply_next_reproposal_due_at={current_env_watch_apply.get('next_reproposal_due_at', '')}",
        f"ops_agenda_item_count={ops_agenda.get('item_count', 0)}",
        f"ops_agenda_action_required_count={ops_agenda.get('action_required_count', 0)}",
        f"ops_agenda_highest_priority_action={ops_agenda.get('highest_priority_action', '')}",
        f"current_env_operator_conclusion={final_summary.get('current_env_operator_conclusion', '')}",
        f"current_env_guardrail_conclusion={final_summary.get('current_env_guardrail_conclusion', '')}",
        f"current_env_due_conclusion={final_summary.get('current_env_due_conclusion', '')}",
        f"ops_agenda_conclusion={final_summary.get('ops_agenda_conclusion', '')}",
        f"known_env_operator_conclusion={final_summary.get('known_env_operator_conclusion', '')}",
        f"new_env_operator_conclusion={final_summary.get('new_env_operator_conclusion', '')}",
        f"current_env_watch_confidence={summary.get('current_env_summary', {}).get('watch_confidence', '')}",
        f"new_env_watch_confidence={summary.get('new_env_summary', {}).get('watch_confidence', '')}",
        f"approved_known_environment_count={summary.get('approved_known_environments', {}).get('environment_count', 0)}",
        f"approved_known_fresh_count={summary.get('approved_known_environments', {}).get('fresh_count', 0)}",
        f"approved_known_due_soon_count={summary.get('approved_known_environments', {}).get('due_soon_count', 0)}",
        f"unapproved_foreign_environment_count={summary.get('unapproved_foreign_environments', {}).get('environment_count', 0)}",
        f"approved_known_healthy_count={summary.get('approved_known_environments', {}).get('healthy_count', 0)}",
        f"approved_known_stale_count={summary.get('approved_known_environments', {}).get('stale_count', 0)}",
        f"approved_known_reverify_required_count={summary.get('approved_known_environments', {}).get('reverify_required_count', 0)}",
        f"approved_known_retire_candidate_count={summary.get('approved_known_environments', {}).get('retire_candidate_count', 0)}",
        f"approved_known_retired_count={summary.get('approved_known_environments', {}).get('retired_count', 0)}",
        f"approved_known_actions_due_soon_count={approved_known_actions.get('due_soon_reverify_count', 0)}",
        f"approved_known_actions_next_due_known_env_id={approved_known_actions.get('next_due_known_env_id', '')}",
        f"approved_known_actions_next_due_at={approved_known_actions.get('next_due_at', '')}",
        f"publication_health_status={summary.get('publication_health', {}).get('status', '')}",
        f"publication_missing_artifact_count={summary.get('publication_health', {}).get('missing_artifact_count', 0)}",
        f"verification_lane_status={summary.get('verification_lane', {}).get('status', '')}",
        f"verification_not_run_count={summary.get('verification_lane', {}).get('verification_not_run_count', 0)}",
        f"runtime_registry_health_status={summary.get('runtime_registry_health', {}).get('status', '')}",
        f"runtime_registry_current_active_count={summary.get('runtime_registry_health', {}).get('current_active_count', 0)}",
        f"runtime_registry_active_fingerprint_count={summary.get('runtime_registry_health', {}).get('active_fingerprint_count', 0)}",
    ]
    for item in final_summary.get("rationale", []):
        lines.append(f"rationale={item}")
    return "\n".join(lines) + "\n"


def action_registry_health(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.health_out)
    if out_path is None:
        raise SystemExit("--health-out is required")
    text_out = Path(args.out_text).resolve() if args.out_text else out_path.with_suffix(".summary.txt")
    runtime_baseline_registry_path = resolve_json_path(args.runtime_baseline_registry)
    runtime_history_index_path = resolve_json_path(args.runtime_history_index)
    runtime_watch_registry_path = resolve_json_path(args.runtime_watch_registry)
    runtime_refresh_path = resolve_json_path(args.runtime_refresh)
    runtime_baseline_registry = read_json(runtime_baseline_registry_path)
    runtime_history_index = read_json(runtime_history_index_path)
    runtime_watch_registry = read_json(runtime_watch_registry_path)
    runtime_refresh = read_json(runtime_refresh_path)
    governance_policy = load_known_env_governance_policy(
        resolve_json_path(getattr(args, "governance_policy", None)),
        phase=str(args.phase),
    ) if getattr(args, "governance_policy", None) else None
    approved_known_summaries = [read_json(resolve_json_path(value)) for value in list(args.approved_known_summary or []) if resolve_json_path(value) is not None]
    foreign_import_summaries = [read_json(resolve_json_path(value)) for value in list(args.foreign_import_summary or []) if resolve_json_path(value) is not None]
    payload = build_runtime_registry_health(
        phase=args.phase,
        runtime_baseline_registry=runtime_baseline_registry,
        runtime_history_index=runtime_history_index,
        runtime_watch_registry=runtime_watch_registry,
        runtime_refresh=runtime_refresh,
        approved_known_summaries=approved_known_summaries,
        foreign_import_summaries=foreign_import_summaries,
        governance_policy=governance_policy,
        current_time_override=getattr(args, "current_time_override", None),
        stale_after_hours=float(args.stale_after_hours),
        reverify_after_hours=float(args.reverify_after_hours),
        retire_after_hours=float(args.retire_after_hours),
    )
    payload["runtime_baseline_registry_path"] = None if runtime_baseline_registry_path is None else str(runtime_baseline_registry_path)
    payload["runtime_history_index_path"] = None if runtime_history_index_path is None else str(runtime_history_index_path)
    payload["runtime_watch_registry_path"] = None if runtime_watch_registry_path is None else str(runtime_watch_registry_path)
    payload["runtime_refresh_path"] = None if runtime_refresh_path is None else str(runtime_refresh_path)
    write_json(out_path, payload)
    atomic_write_text(text_out, build_runtime_registry_health_text(payload))
    print(str(out_path))
    return 0


def action_publication_health(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.health_out)
    if out_path is None:
        raise SystemExit("--health-out is required")
    text_out = Path(args.out_text).resolve() if args.out_text else out_path.with_suffix(".summary.txt")
    published_root = Path(args.published_root).resolve()
    authoritative_root = Path(args.authoritative_root).resolve()
    payload = build_publication_health(
        phase=args.phase,
        published_root=published_root,
        authoritative_root=authoritative_root,
        expect_bundles=bool(args.expect_bundles),
        expect_manifests=bool(args.expect_manifests),
        expect_report=bool(args.expect_report),
    )
    write_json(out_path, payload)
    atomic_write_text(text_out, build_publication_health_text(payload))
    print(str(out_path))
    return 0


def action_watch_registry(args: argparse.Namespace) -> int:
    watch_current_path = resolve_json_path(args.watch_current)
    watch_refresh_path = resolve_json_path(args.watch_refresh)
    watch_history_index_path = resolve_json_path(args.watch_history_index)
    matrix_summary_path = resolve_json_path(args.matrix_summary)
    registry_out_path = resolve_json_path(args.registry_out)
    if registry_out_path is None:
        raise SystemExit("--registry-out is required")
    summary_out_path = resolve_json_path(args.summary_out)
    if summary_out_path is None:
        summary_out_path = registry_out_path.with_name(f"{registry_out_path.stem}_summary.json")

    watch_current = read_json(watch_current_path)
    watch_refresh = read_json(watch_refresh_path)
    watch_history_summary = read_json(
        watch_history_index_path.with_name(f"{watch_history_index_path.stem}_summary.json")
        if watch_history_index_path is not None
        else None
    )
    matrix_summary = read_json(matrix_summary_path) if matrix_summary_path is not None and matrix_summary_path.exists() else {}
    matrix_root = Path(args.matrix_root).resolve() if args.matrix_root else None
    matrix_fixture_refreshes = load_matrix_fixture_watch_refreshes(matrix_root)
    existing_registry = read_json(registry_out_path)
    registry = build_watch_registry(
        watch_current,
        watch_refresh,
        watch_history_summary,
        matrix_summary,
        matrix_fixture_refreshes,
        existing_registry,
    )
    write_json(registry_out_path, registry)
    write_json(summary_out_path, {
        "manifest_version": "runtime_watch_registry_summary_v2",
        "generated_at_utc": registry.get("generated_at_utc", ""),
        "entry_count": registry.get("entry_count", 0),
        "fingerprint_count": registry.get("fingerprint_count", 0),
        "status_counts": registry.get("status_counts", {}),
        "recommendation_counts": registry.get("recommendation_counts", {}),
        "comparability_counts": registry.get("comparability_counts", {}),
        "role_counts": registry.get("role_counts", {}),
        "source_counts": registry.get("source_counts", {}),
        "evidence_source_counts": registry.get("evidence_source_counts", {}),
        "confidence_counts": registry.get("confidence_counts", {}),
        "active_lineage_count": registry.get("active_lineage_count", 0),
        "real_lineage_count": registry.get("real_lineage_count", 0),
        "latest_real_sample_timestamp": registry.get("latest_real_sample_timestamp"),
        "latest_fixture_sample_timestamp": registry.get("latest_fixture_sample_timestamp"),
        "history_transition_count": registry.get("history_transition_count", 0),
        "history_transition_summary": registry.get("history_transition_summary", {}),
        "strongest_watch_status": registry.get("strongest_watch_status", ""),
    })
    atomic_write_text(registry_out_path.with_suffix(".txt"), build_watch_registry_text(registry))
    print(str(registry_out_path))
    return 0


def action_ops_summary(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.out)
    if out_path is None:
        raise SystemExit("--out is required")
    text_out = Path(args.out_text).resolve() if args.out_text else out_path.with_suffix(".txt")
    policy_manifest = read_json(resolve_json_path(args.policy_manifest))
    quick_summary_path = resolve_json_path(args.quick_summary)
    nightly_summary_path = resolve_json_path(args.nightly_summary)
    matrix_summary_path = resolve_json_path(args.matrix_summary)
    quick_summary = read_json(quick_summary_path) if quick_summary_path is not None and quick_summary_path.exists() else {}
    nightly_summary = read_json(nightly_summary_path) if nightly_summary_path is not None and nightly_summary_path.exists() else {}
    matrix_summary = read_json(matrix_summary_path) if matrix_summary_path is not None and matrix_summary_path.exists() else {}
    runtime_refresh = read_json(resolve_json_path(args.runtime_refresh))
    runtime_watch_refresh = read_json(resolve_json_path(args.runtime_watch_refresh))
    watch_registry = read_json(resolve_json_path(args.runtime_watch_registry))
    runtime_registry_health = read_json(resolve_json_path(args.runtime_registry_health))
    publication_health = read_json(resolve_json_path(args.publication_health))
    source_snapshot_manifest = read_json(resolve_json_path(args.source_snapshot_manifest))
    staged_mirror_verify = read_json(resolve_json_path(args.staged_mirror_verify))
    verification_release = read_json(resolve_json_path(args.verification_release))
    verification_debug = read_json(resolve_json_path(args.verification_debug))
    verification_asan = read_json(resolve_json_path(args.verification_asan))
    published_snapshot_manifest = read_json(resolve_json_path(args.published_snapshot_manifest))
    verification_closeout = read_json(resolve_json_path(args.verification_closeout))
    current_env_governance_policy = read_json(resolve_json_path(getattr(args, "current_env_governance_policy", None)))
    current_env_guardrail_policy = read_json(resolve_json_path(getattr(args, "current_env_guardrail_policy", None)))
    current_env_watch_current = read_json(resolve_json_path(getattr(args, "current_env_watch_current", None)))
    current_env_watch_refresh = read_json(resolve_json_path(getattr(args, "current_env_watch_refresh", None)))
    current_env_watch_history = read_json(resolve_json_path(getattr(args, "current_env_watch_history", None)))
    current_env_age_tick = read_json(resolve_json_path(getattr(args, "current_env_age_tick", None)))
    current_env_watch_plan = read_json(resolve_json_path(getattr(args, "current_env_watch_plan", None)))
    current_env_trigger_gate = read_json(resolve_json_path(getattr(args, "current_env_trigger_gate", None)))
    current_env_due = read_json(resolve_json_path(getattr(args, "current_env_due", None)))
    current_env_reproposal_plan = read_json(resolve_json_path(getattr(args, "current_env_reproposal_plan", None)))
    ops_agenda = read_json(resolve_json_path(getattr(args, "ops_agenda", None)))
    current_env_watch_execute = read_json(resolve_json_path(getattr(args, "current_env_watch_execute", None)))
    current_env_watch_apply = read_json(resolve_json_path(getattr(args, "current_env_watch_apply", None)))
    current_env_reproposal_execute = read_json(resolve_json_path(getattr(args, "current_env_reproposal_execute", None)))
    current_env_action_ledger = read_json(resolve_json_path(getattr(args, "current_env_action_ledger", None)))
    current_env_retry_plan = read_json(resolve_json_path(getattr(args, "current_env_retry_plan", None)))
    current_env_reproposal_handoff = read_json(resolve_json_path(getattr(args, "current_env_reproposal_handoff", None)))
    current_env_operator_decision = read_json(resolve_json_path(getattr(args, "current_env_operator_decision", None)))
    current_env_operator_decision_apply = read_json(resolve_json_path(getattr(args, "current_env_operator_decision_apply", None)))
    current_env_action_ledger_compact = read_json(resolve_json_path(getattr(args, "current_env_action_ledger_compact", None)))
    current_env_action_ledger_archive = read_json(resolve_json_path(getattr(args, "current_env_action_ledger_archive", None)))
    current_env_approval_runbook = read_json(resolve_json_path(getattr(args, "current_env_approval_runbook", None)))
    current_env_approval_execution = read_json(resolve_json_path(getattr(args, "current_env_approval_execution", None)))
    current_env_approval_link = read_json(resolve_json_path(getattr(args, "current_env_approval_link", None)))
    operator_runbook_index = read_json(resolve_json_path(getattr(args, "operator_runbook_index", None)))
    operator_runbook_catalog = read_json(resolve_json_path(getattr(args, "operator_runbook_catalog", None)))
    operator_decision_metadata_audit = read_json(resolve_json_path(getattr(args, "operator_decision_metadata_audit", None)))
    operator_runbook_replay = read_json(resolve_json_path(getattr(args, "operator_runbook_replay", None)))
    operator_runbook_retention_policy = read_json(resolve_json_path(getattr(args, "operator_runbook_retention_policy", None)))
    operator_runbook_pruned_catalog = read_json(resolve_json_path(getattr(args, "operator_runbook_pruned_catalog", None)))
    operator_runbook_archive = read_json(resolve_json_path(getattr(args, "operator_runbook_archive", None)))
    operator_runbook_prune_summary = read_json(resolve_json_path(getattr(args, "operator_runbook_prune_summary", None)))
    operator_runbook_lifecycle_validation = read_json(resolve_json_path(getattr(args, "operator_runbook_lifecycle_validation", None)))
    operator_runbook_pointer_audit = read_json(resolve_json_path(getattr(args, "operator_runbook_pointer_audit", None)))
    operator_runbook_provenance_migration = read_json(resolve_json_path(getattr(args, "operator_runbook_provenance_migration", None)))
    operator_runbook_migrated_catalog = read_json(resolve_json_path(getattr(args, "operator_runbook_migrated_catalog", None)))
    operator_runbook_migrated_ledger = read_json(resolve_json_path(getattr(args, "operator_runbook_migrated_ledger", None)))
    operator_runbook_lifecycle_validation_before = read_json(resolve_json_path(getattr(args, "operator_runbook_lifecycle_validation_before", None)))
    operator_runbook_lifecycle_validation_after = read_json(resolve_json_path(getattr(args, "operator_runbook_lifecycle_validation_after", None)))
    operator_artifact_path_policy_lint = read_json(resolve_json_path(getattr(args, "operator_artifact_path_policy_lint", None)))
    integrated_approval_mutation_audit = read_json(resolve_json_path(getattr(args, "integrated_approval_mutation_audit", None)))
    source_health_action_plan = read_json(resolve_json_path(getattr(args, "source_health_action_plan", None)))
    staged_materialization_transaction = read_json(resolve_json_path(getattr(args, "staged_materialization_transaction", None)))
    source_health_preflight = read_json(resolve_json_path(getattr(args, "source_health_preflight", None)))
    staged_materialization = read_json(resolve_json_path(getattr(args, "staged_materialization", None)))
    runtime_budget_current = read_json(resolve_json_path(getattr(args, "runtime_budget_current", None)))
    runtime_budget_proposal = read_json(resolve_json_path(getattr(args, "runtime_budget_proposal", None)))
    runtime_budget_proposal_gate = read_json(resolve_json_path(getattr(args, "runtime_budget_proposal_gate", None)))
    runtime_budget_baseline = read_json(resolve_json_path(getattr(args, "runtime_budget_baseline", None)))
    runtime_budget_refresh = read_json(resolve_json_path(getattr(args, "runtime_budget_refresh", None)))
    runtime_budget_reproposal_history = read_json(resolve_json_path(getattr(args, "runtime_budget_reproposal_history", None)))
    runtime_budget_registry_summary = read_json(resolve_json_path(getattr(args, "runtime_budget_registry_summary", None)))
    approved_known_summaries = [read_json(resolve_json_path(value)) for value in list(args.approved_known_summary or []) if resolve_json_path(value) is not None]
    foreign_import_summaries = [read_json(resolve_json_path(value)) for value in list(args.foreign_import_summary or []) if resolve_json_path(value) is not None]
    runtime_baseline_registry = read_json(resolve_json_path(args.runtime_baseline_registry))
    payload = build_ops_summary(
        args.phase,
        policy_manifest,
        quick_summary,
        nightly_summary,
        matrix_summary,
        runtime_refresh,
        runtime_watch_refresh,
        watch_registry,
        approved_known_summaries,
        foreign_import_summaries,
        runtime_baseline_registry,
        runtime_registry_health,
        publication_health,
        source_snapshot_manifest,
        staged_mirror_verify,
        verification_release,
        verification_debug,
        verification_asan,
        published_snapshot_manifest,
        verification_closeout,
        current_env_governance_policy,
        current_env_guardrail_policy,
        current_env_watch_current,
        current_env_watch_refresh,
        current_env_watch_history,
        current_env_age_tick,
        current_env_watch_plan,
        current_env_trigger_gate,
        runtime_budget_current,
        runtime_budget_proposal,
        runtime_budget_proposal_gate,
        runtime_budget_baseline,
        runtime_budget_refresh,
        runtime_budget_reproposal_history,
        runtime_budget_registry_summary,
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
    )
    if operator_runbook_index or source_health_preflight or staged_materialization:
        payload["manifest_version"] = "policy_ops_summary_v14"
        runbook_section = {
            "pending_runbook_count": operator_runbook_index.get("pending_runbook_count", 0),
            "executable_runbook_count": operator_runbook_index.get("executable_runbook_count", 0),
            "integrated_opt_in_required_count": operator_runbook_index.get("integrated_opt_in_required_count", 0),
            "approval_runbook_count": operator_runbook_index.get("approval_runbook_count", 0),
            "retry_runbook_count": operator_runbook_index.get("retry_runbook_count", 0),
            "runbook_index_hash": operator_runbook_index.get("runbook_index_hash"),
        }
        decision_variants = {
            "skip_covered": "skip" in json.dumps(operator_runbook_index),
            "defer_covered": "defer" in json.dumps(operator_runbook_index),
            "reject_covered": "reject" in json.dumps(operator_runbook_index),
            "retry_now_covered": "retry_now" in json.dumps(operator_runbook_index),
            "variant_source": operator_runbook_index.get("manifest_version") if operator_runbook_index else "NOT_RUN",
        }
        source_health_section = {
            "status": source_health_preflight.get("status", "NOT_RUN"),
            "recommendation": source_health_preflight.get("recommendation", "NOT_RUN"),
            "dataless_placeholder_count": source_health_preflight.get("dataless_placeholder_count", 0),
            "git_object_health": source_health_preflight.get("git_object_health", {}),
            "materialization_mode": staged_materialization.get("staged_materialization_mode")
            or source_health_preflight.get("staged_materialization", {}).get("staged_materialization_mode"),
        }
        verification_lane = dict(payload.get("verification_lane", {}))
        if staged_materialization or source_health_preflight:
            verification_lane["materialization_mode"] = source_health_section.get("materialization_mode")
            verification_lane["source_health_status"] = source_health_section.get("status")
            verification_lane["source_health_recommendation"] = source_health_section.get("recommendation")
            verification_lane["dataless_placeholder_count"] = source_health_section.get("dataless_placeholder_count")
            verification_lane["materialization_verdict"] = staged_materialization.get("materialization_verdict") or source_health_preflight.get("staged_materialization", {}).get("materialization_verdict")
        payload["current_env_operator_runbooks"] = runbook_section
        payload["current_env_decision_variants"] = decision_variants
        payload["source_health"] = source_health_section
        payload["staged_materialization"] = staged_materialization or source_health_preflight.get("staged_materialization", {})
        payload["verification_lane"] = verification_lane
        payload.setdefault("final_operator_actions", {})["recommended_action_source_health"] = source_health_section.get("recommendation")
        payload.setdefault("final_operator_actions", {})["operator_runbook_executable_count"] = runbook_section.get("executable_runbook_count", 0)
        payload.setdefault("final_operator_summary", {})["source_health_status"] = source_health_section.get("status")
        payload.setdefault("final_operator_summary", {})["operator_runbook_count"] = operator_runbook_index.get("runbook_count", 0)
        payload["ops_summary_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    if operator_runbook_catalog or operator_decision_metadata_audit or operator_runbook_replay or staged_materialization_transaction:
        payload["manifest_version"] = "policy_ops_summary_v15"
        payload["operator_runbook_catalog"] = {
            "active_runbook_count": operator_runbook_catalog.get("active_runbook_count", 0),
            "resolved_runbook_count": operator_runbook_catalog.get("resolved_runbook_count", 0),
            "integrated_opt_in_required_count": operator_runbook_catalog.get("integrated_opt_in_required_count", 0),
            "metadata_audit_verdict": operator_decision_metadata_audit.get("audit_verdict", "NOT_RUN"),
            "replayable_runbook_count": operator_runbook_catalog.get("replayable_runbook_count", 0),
            "catalog_hash": operator_runbook_catalog.get("catalog_hash"),
        }
        payload["operator_decision_metadata_audit"] = {
            "audit_verdict": operator_decision_metadata_audit.get("audit_verdict", "NOT_RUN"),
            "missing_reason_count": operator_decision_metadata_audit.get("missing_reason_count", 0),
            "missing_defer_until_count": operator_decision_metadata_audit.get("missing_defer_until_count", 0),
            "missing_retry_link_count": operator_decision_metadata_audit.get("missing_retry_link_count", 0),
            "missing_approval_pointer_count": operator_decision_metadata_audit.get("missing_approval_pointer_count", 0),
            "affected_action_ids": operator_decision_metadata_audit.get("affected_action_ids", []),
        }
        payload["operator_runbook_replay"] = {
            "replay_verdict": operator_runbook_replay.get("replay_verdict", "NOT_RUN"),
            "missing_input_count": operator_runbook_replay.get("missing_input_count", 0),
            "command_still_valid": operator_runbook_replay.get("command_still_valid"),
            "requires_operator_confirmation": operator_runbook_replay.get("requires_operator_confirmation"),
            "would_mutate_registry": operator_runbook_replay.get("would_mutate_registry"),
        }
        approval_guardrail = {
            "integrated_default_enabled": False,
            "handoff_only_default": True,
            "accidental_mutation_guard": "PASS"
            if not bool(current_env_approval_execution.get("registry_updated", False))
            or str(current_env_approval_execution.get("approval_execution_mode", "")) == APPROVAL_EXECUTION_MODE_INTEGRATED_OPT_IN
            else "FAIL",
            "approval_execution_mode": current_env_approval_execution.get("approval_execution_mode"),
            "approval_status": current_env_approval_execution.get("approval_status"),
            "confirmation_token_present": current_env_approval_execution.get("approval_confirmation_token_present", False),
            "dry_run_preflight_success": current_env_approval_execution.get("dry_run_preflight_success", False),
            "registry_before_hash": current_env_approval_execution.get("registry_before_hash"),
            "registry_after_hash": current_env_approval_execution.get("registry_after_hash"),
            "approval_transaction_id": current_env_approval_execution.get("approval_transaction_id"),
        }
        payload["approval_execution_guardrail"] = approval_guardrail
        payload["staged_materialization_transaction"] = {
            "transaction_id": staged_materialization_transaction.get("transaction_id"),
            "transaction_verdict": staged_materialization_transaction.get("transaction_verdict", "NOT_RUN"),
            "materialization_mode": staged_materialization_transaction.get("materialization_mode"),
            "source_health_hash": staged_materialization_transaction.get("source_health_hash"),
            "rollback_cleanup_performed": staged_materialization_transaction.get("rollback_cleanup_performed", False),
        }
        payload.setdefault("final_operator_actions", {})["operator_metadata_audit"] = operator_decision_metadata_audit.get("audit_verdict", "NOT_RUN")
        payload.setdefault("final_operator_actions", {})["approval_guardrail"] = approval_guardrail.get("accidental_mutation_guard")
        payload.setdefault("final_operator_summary", {})["runbook_catalog_active_count"] = operator_runbook_catalog.get("active_runbook_count", 0)
        payload.setdefault("final_operator_summary", {})["decision_metadata_audit_verdict"] = operator_decision_metadata_audit.get("audit_verdict", "NOT_RUN")
        payload["ops_summary_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    if operator_runbook_prune_summary or operator_runbook_lifecycle_validation or integrated_approval_mutation_audit or source_health_action_plan:
        payload["manifest_version"] = "policy_ops_summary_v16"
        payload["operator_runbook_retention"] = {
            "policy_id": operator_runbook_retention_policy.get("policy_id"),
            "prune_verdict": operator_runbook_prune_summary.get("prune_verdict", "NOT_RUN"),
            "active_retained_count": operator_runbook_prune_summary.get("active_retained_count", 0),
            "failed_retained_count": operator_runbook_prune_summary.get("failed_retained_count", 0),
            "retry_pending_retained_count": operator_runbook_prune_summary.get("retry_pending_retained_count", 0),
            "approval_retained_count": operator_runbook_prune_summary.get("approval_retained_count", 0),
            "resolved_retained_count": operator_runbook_prune_summary.get("resolved_retained_count", 0),
            "archived_count": operator_runbook_prune_summary.get("archived_count", 0),
            "pruned_count": operator_runbook_prune_summary.get("pruned_count", 0),
            "blocked_prune_count": operator_runbook_prune_summary.get("blocked_prune_count", 0),
            "pruned_catalog_entry_count": operator_runbook_pruned_catalog.get("catalog_entry_count", 0),
            "archive_entry_count": operator_runbook_archive.get("archived_count", 0),
        }
        payload["operator_runbook_lifecycle_validation"] = {
            "validation_verdict": operator_runbook_lifecycle_validation.get("validation_verdict", "NOT_RUN"),
            "missing_input_count": operator_runbook_lifecycle_validation.get("missing_input_count", 0),
            "stale_input_count": operator_runbook_lifecycle_validation.get("stale_input_count", 0),
            "superseded_runbook_count": operator_runbook_lifecycle_validation.get("superseded_runbook_count", 0),
            "archived_runbook_count": operator_runbook_lifecycle_validation.get("archived_runbook_count", 0),
            "replayable_runbook_count": operator_runbook_lifecycle_validation.get("replayable_runbook_count", 0),
            "non_replayable_runbook_count": operator_runbook_lifecycle_validation.get("non_replayable_runbook_count", 0),
            "mutation_risk_count": operator_runbook_lifecycle_validation.get("mutation_risk_count", 0),
            "integrated_opt_in_blocked_count": operator_runbook_lifecycle_validation.get("integrated_opt_in_blocked_count", 0),
            "stale_but_safe_count": operator_runbook_lifecycle_validation.get("stale_but_safe_count", 0),
        }
        payload["integrated_approval_mutation_audit"] = {
            "audit_verdict": integrated_approval_mutation_audit.get("audit_verdict", "NOT_RUN"),
            "approval_execution_id": integrated_approval_mutation_audit.get("approval_execution_id"),
            "approval_execution_mode": integrated_approval_mutation_audit.get("approval_execution_mode"),
            "mutation_expected": integrated_approval_mutation_audit.get("mutation_expected", False),
            "mutation_observed": integrated_approval_mutation_audit.get("mutation_observed", False),
            "unexpected_mutation_count": integrated_approval_mutation_audit.get("unexpected_mutation_count", 0),
            "registry_before_hash": integrated_approval_mutation_audit.get("registry_before_hash"),
            "registry_after_hash": integrated_approval_mutation_audit.get("registry_after_hash"),
            "rollback_hint": integrated_approval_mutation_audit.get("rollback_hint"),
        }
        payload["source_health_action_plan"] = {
            "plan_verdict": source_health_action_plan.get("plan_verdict", "NOT_RUN"),
            "direct_build_blocked": source_health_action_plan.get("direct_build_blocked", False),
            "staged_build_allowed": source_health_action_plan.get("staged_build_allowed", False),
            "recommended_action": source_health_action_plan.get("recommended_action", "NOT_RUN"),
            "issue_count": source_health_action_plan.get("issue_count", 0),
            "materialization_mode": source_health_action_plan.get("materialization_mode"),
        }
        verification_lane = dict(payload.get("verification_lane", {}))
        if source_health_action_plan:
            verification_lane["source_health_action_plan_hash"] = source_health_action_plan.get("plan_hash")
            verification_lane["direct_build_allowed"] = not bool(source_health_action_plan.get("direct_build_blocked", False))
            verification_lane["staged_build_allowed"] = bool(source_health_action_plan.get("staged_build_allowed", False))
            verification_lane["materialization_mode"] = source_health_action_plan.get("materialization_mode") or verification_lane.get("materialization_mode")
        payload["verification_lane"] = verification_lane
        payload.setdefault("final_operator_actions", {})["operator_runbook_retention"] = operator_runbook_prune_summary.get("prune_verdict", "NOT_RUN")
        payload.setdefault("final_operator_actions", {})["runbook_lifecycle_validation"] = operator_runbook_lifecycle_validation.get("validation_verdict", "NOT_RUN")
        payload.setdefault("final_operator_actions", {})["approval_mutation_audit"] = integrated_approval_mutation_audit.get("audit_verdict", "NOT_RUN")
        payload.setdefault("final_operator_actions", {})["source_health_action_plan"] = source_health_action_plan.get("recommended_action", "NOT_RUN")
        payload.setdefault("final_operator_summary", {})["runbook_retention_prune_verdict"] = operator_runbook_prune_summary.get("prune_verdict", "NOT_RUN")
        payload.setdefault("final_operator_summary", {})["runbook_lifecycle_validation_verdict"] = operator_runbook_lifecycle_validation.get("validation_verdict", "NOT_RUN")
        payload.setdefault("final_operator_summary", {})["integrated_approval_mutation_audit_verdict"] = integrated_approval_mutation_audit.get("audit_verdict", "NOT_RUN")
        payload.setdefault("final_operator_summary", {})["source_health_action_plan"] = source_health_action_plan.get("recommended_action", "NOT_RUN")
        payload["ops_summary_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    if (
        operator_runbook_pointer_audit
        or operator_runbook_provenance_migration
        or operator_artifact_path_policy_lint
        or operator_runbook_lifecycle_validation_after
    ):
        payload["manifest_version"] = "policy_ops_summary_v17"
        before = operator_runbook_lifecycle_validation_before or operator_runbook_lifecycle_validation
        after = operator_runbook_lifecycle_validation_after or operator_runbook_lifecycle_validation
        payload["operator_runbook_pointer_audit"] = {
            "audit_verdict": operator_runbook_pointer_audit.get("audit_verdict", "NOT_RUN"),
            "pointer_count": operator_runbook_pointer_audit.get("pointer_count", 0),
            "absolute_tmp_pointer_count": operator_runbook_pointer_audit.get("absolute_tmp_pointer_count", 0),
            "rewritable_pointer_count": operator_runbook_pointer_audit.get("rewritable_pointer_count", 0),
            "waivable_archived_pointer_count": operator_runbook_pointer_audit.get("waivable_archived_pointer_count", 0),
            "unresolved_active_pointer_count": operator_runbook_pointer_audit.get("unresolved_active_pointer_count", 0),
        }
        payload["operator_runbook_provenance_migration"] = {
            "migration_verdict": operator_runbook_provenance_migration.get("migration_verdict", "NOT_RUN"),
            "rewritten_pointer_count": operator_runbook_provenance_migration.get("rewritten_pointer_count", 0),
            "waived_archived_pointer_count": operator_runbook_provenance_migration.get("waived_archived_pointer_count", 0),
            "unresolved_active_pointer_count": operator_runbook_provenance_migration.get("unresolved_active_pointer_count", 0),
            "unresolved_archived_pointer_count": operator_runbook_provenance_migration.get("unresolved_archived_pointer_count", 0),
            "source_catalog_hash": operator_runbook_provenance_migration.get("source_catalog_hash"),
            "migrated_catalog_hash": operator_runbook_provenance_migration.get("migrated_catalog_hash"),
            "source_ledger_hash": operator_runbook_provenance_migration.get("source_ledger_hash"),
            "migrated_ledger_hash": operator_runbook_provenance_migration.get("migrated_ledger_hash"),
        }
        payload["operator_runbook_lifecycle_validation"] = {
            "before_validation_verdict": before.get("validation_verdict", "NOT_RUN"),
            "before_missing_input_count": before.get("missing_input_count", 0),
            "after_validation_verdict": after.get("validation_verdict", "NOT_RUN"),
            "after_missing_input_count": after.get("missing_input_count", 0),
            "after_stale_input_count": after.get("stale_input_count", 0),
            "after_mutation_risk_count": after.get("mutation_risk_count", 0),
            "migrated_catalog_entry_count": operator_runbook_migrated_catalog.get("catalog_entry_count", 0),
            "migrated_ledger_entry_count": operator_runbook_migrated_ledger.get("total_action_count", 0),
        }
        payload["operator_artifact_path_policy_lint"] = {
            "lint_verdict": operator_artifact_path_policy_lint.get("lint_verdict", "NOT_RUN"),
            "forbidden_path_count": operator_artifact_path_policy_lint.get("forbidden_path_count", 0),
            "dangling_path_count": operator_artifact_path_policy_lint.get("dangling_path_count", 0),
            "allowed_external_reference_count": operator_artifact_path_policy_lint.get("allowed_external_reference_count", 0),
            "scanned_manifest_count": operator_artifact_path_policy_lint.get("scanned_manifest_count", 0),
        }
        active_unresolved = int(operator_runbook_provenance_migration.get("unresolved_active_pointer_count", 0) or 0)
        waived_archived = int(operator_runbook_provenance_migration.get("waived_archived_pointer_count", 0) or 0)
        path_lint_forbidden = int(operator_artifact_path_policy_lint.get("forbidden_path_count", 0) or 0)
        final_action = "NO_ACTION"
        if active_unresolved > 0 or path_lint_forbidden > 0:
            final_action = "ACTION_REQUIRED"
        elif waived_archived > 0:
            final_action = "REVIEW_ARCHIVE_WAIVERS"
        payload.setdefault("final_operator_actions", {})["runbook_provenance_migration"] = final_action
        payload.setdefault("final_operator_actions", {})["path_policy_lint"] = operator_artifact_path_policy_lint.get("lint_verdict", "NOT_RUN")
        payload.setdefault("final_operator_summary", {})["runbook_provenance_migration_verdict"] = operator_runbook_provenance_migration.get("migration_verdict", "NOT_RUN")
        payload.setdefault("final_operator_summary", {})["path_policy_lint_verdict"] = operator_artifact_path_policy_lint.get("lint_verdict", "NOT_RUN")
        payload["ops_summary_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    write_json(out_path, payload)
    atomic_write_text(text_out, build_ops_summary_text(payload))
    print(str(out_path))
    return 0


def default_runtime_import_summary_path(import_manifest_path: Path) -> Path:
    return import_manifest_path.with_name(f"{import_manifest_path.stem}_import_summary.json")


def build_runtime_import_summary(
    *,
    current_manifest: dict[str, Any],
    import_manifest_path: Path,
    runtime_registry_path: Path,
    selection: dict[str, Any],
    refresh_manifest: dict[str, Any] | None,
    refresh_manifest_path: Path | None,
    evidence_source: str,
    runner_id: str,
    host_label: str,
    import_reason: str,
) -> dict[str, Any]:
    normalized_source = normalized_evidence_source(evidence_source)
    same_fingerprint = int(selection.get("exact_match_count", 0)) > 0
    proposal_needed = True
    if refresh_manifest is not None:
        proposal_needed = (
            str(refresh_manifest.get("comparability_verdict", "")) != runtime_gate.COMPARABLE
            or str(refresh_manifest.get("current_verdict", "")) == runtime_gate.VERDICT_FAIL
            or str(refresh_manifest.get("freshness_verdict", "")) in {runtime_gate.FRESHNESS_STALE, runtime_gate.FRESHNESS_REQUIRES_RERUN}
        )
    elif str(selection.get("comparability_verdict", "")) == runtime_gate.COMPARABLE:
        proposal_needed = False
    import_verdict = "APPENDED_EXISTING_LINEAGE" if same_fingerprint else "NEW_FINGERPRINT_CANDIDATE"
    if refresh_manifest is not None and not proposal_needed and same_fingerprint:
        import_verdict = "COMPARABLE_IMPORTED"
    environment_state = "IMPORTED_CANDIDATE"
    if not same_fingerprint:
        environment_state = "UNAPPROVED_FOREIGN"
    elif import_verdict == "COMPARABLE_IMPORTED":
        environment_state = "CURRENT_ENV_EVIDENCE_APPEND"
    return {
        "manifest_version": "runtime_external_import_summary_v1",
        "generated_at_utc": stable_manifest_timestamp(current_manifest) or runtime_gate.timestamp_utc_now(),
        "runtime_import_manifest_path": str(import_manifest_path),
        "runtime_import_manifest_hash": sha256_file(import_manifest_path),
        "runtime_baseline_registry_path": str(runtime_registry_path),
        "imported_fingerprint_key": runtime_gate.runtime_manifest_fingerprint_key(current_manifest),
        "imported_host_fingerprint": dict(current_manifest.get("host_fingerprint", {})),
        "imported_toolchain_fingerprint": dict(current_manifest.get("toolchain_fingerprint", {})),
        "imported_execution_classes": runtime_gate.runtime_execution_classes(current_manifest),
        "imported_runtime_budget_profile_id": current_manifest.get("runtime_budget_profile_id"),
        "evidence_source": normalized_source,
        "runner_id": runner_id or None,
        "host_label": host_label or None,
        "import_verdict": import_verdict,
        "environment_state": environment_state,
        "selected_baseline_id": selection.get("selected_baseline_id"),
        "selected_baseline_tag": selection.get("selected_baseline_tag"),
        "comparability_verdict": (refresh_manifest or {}).get("comparability_verdict", selection.get("comparability_verdict")),
        "proposal_needed": bool(proposal_needed),
        "counts_as_real_evidence": normalized_source == "real",
        "import_reason": import_reason,
        "runtime_refresh_manifest_path": None if refresh_manifest_path is None else str(refresh_manifest_path),
        "runtime_refresh_manifest_hash": sha256_file(refresh_manifest_path),
        "runtime_current_verdict": None if refresh_manifest is None else refresh_manifest.get("current_verdict"),
        "runtime_freshness_verdict": None if refresh_manifest is None else refresh_manifest.get("freshness_verdict"),
        "selection_summary": {
            "exact_match_count": selection.get("exact_match_count", 0),
            "compatible_match_count": selection.get("compatible_match_count", 0),
            "retired_match_count": selection.get("retired_match_count", 0),
            "comparability_reason": selection.get("comparability_reason"),
        },
    }


def import_runtime_current_manifest(
    *,
    import_manifest_path: Path,
    runtime_registry_path: Path,
    runtime_history_index_path: Path,
    runtime_watch_registry_path: Path,
    runtime_refresh_out_path: Path | None,
    evidence_source: str,
    runner_id: str,
    host_label: str,
    refresh_after_import: bool,
) -> tuple[dict[str, Any], dict[str, Any]]:
    current_manifest = runtime_gate.read_json(import_manifest_path)
    runtime_registry = runtime_gate.load_runtime_registry(runtime_registry_path)
    selection = runtime_gate.select_runtime_baseline_from_registry(
        current_manifest,
        runtime_registry,
        import_manifest_path,
        runtime_registry_path,
    )
    baseline_manifest, baseline_manifest_path = runtime_gate.selection_baseline_manifest(selection)
    refresh_manifest: dict[str, Any] | None = None
    refresh_manifest_path: Path | None = None
    if refresh_after_import:
        refresh_manifest_path = (
            runtime_refresh_out_path
            if runtime_refresh_out_path is not None
            else import_manifest_path.with_name(f"{import_manifest_path.stem}_refresh_import.json")
        )
        refresh_manifest = runtime_gate.refresh_runtime_manifest(
            baseline_manifest,
            current_manifest,
            baseline_manifest_path,
            import_manifest_path,
            baseline_selection=selection,
            runtime_registry_path=runtime_registry_path,
        )
        runtime_gate.write_runtime_refresh_outputs(refresh_manifest_path, refresh_manifest)
    history_index = (
        runtime_gate.read_json(runtime_history_index_path)
        if runtime_history_index_path.exists()
        else runtime_gate.empty_runtime_history_index()
    )
    history_index = runtime_gate.append_runtime_history(
        history_index,
        current_manifest,
        import_manifest_path,
        refresh_manifest,
        refresh_manifest_path,
        evidence_source=evidence_source,
        runner_id=runner_id,
        host_label=host_label,
        import_timestamp=stable_manifest_timestamp(current_manifest) or None,
    )
    runtime_gate.write_runtime_history_outputs(runtime_history_index_path, history_index)

    budget_profile = runtime_gate.runtime_budget_profile_for_manifest(current_manifest)
    refresh_entries = {
        str(entry.get("execution_class", "")): dict(entry)
        for entry in (refresh_manifest or {}).get("entries", [])
        if isinstance(entry, dict)
    }
    baseline_entries = {
        str(entry.get("execution_class", "")): dict(entry)
        for entry in (baseline_manifest or {}).get("entries", [])
        if isinstance(entry, dict)
    }
    history_bucket = runtime_gate.history_bucket_for_manifest(history_index, current_manifest)
    imported_contributions: list[dict[str, Any]] = []
    for execution_class in runtime_gate.runtime_execution_classes(current_manifest):
        current_entry = next(
            (
                dict(entry)
                for entry in current_manifest.get("entries", [])
                if isinstance(entry, dict) and str(entry.get("execution_class", "")) == execution_class
            ),
            None,
        )
        if current_entry is None:
            continue
        history_payload = dict(history_bucket.get("execution_classes", {})).get(execution_class, {})
        series = [dict(sample) for sample in history_payload.get("samples", []) if isinstance(sample, dict)]
        classified = runtime_gate.classify_runtime_watch_entry(
            execution_class,
            current_entry,
            refresh_entries.get(execution_class, {}),
            baseline_entries.get(execution_class),
            series,
            runtime_gate.runtime_budget_profile_entry(budget_profile, execution_class),
        )
        imported_contributions.append(
            build_watch_registry_contribution(
                {
                    **classified,
                    "runner_id": runner_id,
                    "host_label": host_label,
                    "import_timestamp": stable_manifest_timestamp(current_manifest),
                    "runtime_fingerprint_key": runtime_gate.runtime_manifest_fingerprint_key(current_manifest),
                },
                source_kind=f"imported_{normalized_evidence_source(evidence_source)}",
                manifest={
                    "runtime_fingerprint_key": runtime_gate.runtime_manifest_fingerprint_key(current_manifest),
                    "selected_baseline_id": selection.get("selected_baseline_id"),
                    "selected_baseline_tag": selection.get("selected_baseline_tag"),
                    "comparability_verdict": None if refresh_manifest is None else refresh_manifest.get("comparability_verdict"),
                    "generated_at_utc": stable_manifest_timestamp(current_manifest),
                    "runner_id": runner_id,
                    "host_label": host_label,
                },
                fixture_name=None,
                matrix_entry=None,
                runner_id=runner_id,
                host_label=host_label,
            )
        )
    existing_registry = read_json(runtime_watch_registry_path)
    imported_registry = build_watch_registry({}, {}, {}, {}, [], {"entries": imported_contributions + existing_registry_contributions(existing_registry)})
    imported_registry["import_manifest_path"] = str(import_manifest_path)
    imported_registry["runtime_history_index_path"] = str(runtime_history_index_path)
    imported_registry["runtime_baseline_registry_path"] = str(runtime_registry_path)
    imported_registry["imported_evidence_source"] = normalized_evidence_source(evidence_source)
    imported_registry["imported_runner_id"] = runner_id
    imported_registry["imported_host_label"] = host_label
    imported_registry["imported_refresh_manifest_path"] = None if refresh_manifest_path is None else str(refresh_manifest_path)
    imported_registry["import_timestamp"] = stable_manifest_timestamp(current_manifest) or stable_manifest_timestamp(imported_registry)
    write_json(runtime_watch_registry_path, imported_registry)
    summary_out_path = runtime_watch_registry_path.with_name(f"{runtime_watch_registry_path.stem}_summary.json")
    write_json(
        summary_out_path,
        {
            "manifest_version": "runtime_watch_registry_summary_v2",
            "generated_at_utc": imported_registry.get("generated_at_utc", ""),
            "entry_count": imported_registry.get("entry_count", 0),
            "fingerprint_count": imported_registry.get("fingerprint_count", 0),
            "evidence_source_counts": imported_registry.get("evidence_source_counts", {}),
            "confidence_counts": imported_registry.get("confidence_counts", {}),
            "latest_real_sample_timestamp": imported_registry.get("latest_real_sample_timestamp"),
            "latest_fixture_sample_timestamp": imported_registry.get("latest_fixture_sample_timestamp"),
        },
    )
    atomic_write_text(runtime_watch_registry_path.with_suffix(".txt"), build_watch_registry_text(imported_registry))
    import_reason = "same fingerprint evidence appended to existing runtime lineage"
    if int(selection.get("exact_match_count", 0)) <= 0:
        import_reason = "new or foreign fingerprint imported without an exact active baseline"
    import_summary = build_runtime_import_summary(
        current_manifest=current_manifest,
        import_manifest_path=import_manifest_path,
        runtime_registry_path=runtime_registry_path,
        selection=selection,
        refresh_manifest=refresh_manifest,
        refresh_manifest_path=refresh_manifest_path,
        evidence_source=evidence_source,
        runner_id=runner_id,
        host_label=host_label,
        import_reason=import_reason,
    )
    return imported_registry, import_summary


def action_import_current(args: argparse.Namespace) -> int:
    import_manifest_path = resolve_json_path(args.runtime_import_manifest)
    runtime_registry_path = resolve_json_path(args.runtime_baseline_registry)
    runtime_history_index_path = resolve_json_path(args.runtime_history_index)
    runtime_watch_registry_path = resolve_json_path(args.runtime_watch_registry)
    runtime_refresh_out_path = resolve_json_path(args.runtime_refresh_out)
    import_out_path = resolve_json_path(args.import_out)
    if (
        import_manifest_path is None
        or runtime_registry_path is None
        or runtime_history_index_path is None
        or runtime_watch_registry_path is None
    ):
        raise SystemExit(
            "--runtime-import-manifest, --runtime-baseline-registry, --runtime-history-index, and "
            "--runtime-watch-registry are required"
        )
    imported_registry, import_summary = import_runtime_current_manifest(
        import_manifest_path=import_manifest_path,
        runtime_registry_path=runtime_registry_path,
        runtime_history_index_path=runtime_history_index_path,
        runtime_watch_registry_path=runtime_watch_registry_path,
        runtime_refresh_out_path=runtime_refresh_out_path,
        evidence_source=args.evidence_source,
        runner_id=args.runner_id,
        host_label=args.host_label,
        refresh_after_import=bool(args.refresh_after_import),
    )
    if import_out_path is None:
        import_out_path = default_runtime_import_summary_path(import_manifest_path)
    write_json(import_out_path, import_summary)
    atomic_write_text(import_out_path.with_suffix(".txt"), json.dumps(import_summary, indent=2) + "\n")
    print(str(runtime_watch_registry_path))
    return 0


def action_import_external_bundle(args: argparse.Namespace) -> int:
    bundle_path = resolve_json_path(args.runtime_import_bundle)
    runtime_registry_path = resolve_json_path(args.runtime_baseline_registry)
    runtime_history_index_path = resolve_json_path(args.runtime_history_index)
    runtime_watch_registry_path = resolve_json_path(args.runtime_watch_registry)
    import_out_path = resolve_json_path(args.import_out)
    if (
        bundle_path is None
        or runtime_registry_path is None
        or runtime_history_index_path is None
        or runtime_watch_registry_path is None
    ):
        raise SystemExit(
            "--runtime-import-bundle, --runtime-baseline-registry, --runtime-history-index, "
            "and --runtime-watch-registry are required"
        )
    bundle = read_json(bundle_path)
    manifest_value = (
        bundle.get("runtime_current_manifest_path")
        or bundle.get("runtime_current_manifest")
        or bundle.get("runtime_current")
    )
    if not manifest_value:
        raise SystemExit("runtime import bundle is missing runtime_current_manifest_path")
    import_manifest_path = resolve_json_path(str(manifest_value))
    if import_manifest_path is None or not import_manifest_path.exists():
        raise SystemExit("runtime import bundle points at a missing runtime current manifest")
    evidence_source = str(args.evidence_source or bundle.get("evidence_source") or "real")
    runner_id = str(args.runner_id or bundle.get("runner_id") or "")
    host_label = str(args.host_label or bundle.get("host_label") or "")
    refresh_out_path = resolve_json_path(args.runtime_refresh_out)
    imported_registry, import_summary = import_runtime_current_manifest(
        import_manifest_path=import_manifest_path,
        runtime_registry_path=runtime_registry_path,
        runtime_history_index_path=runtime_history_index_path,
        runtime_watch_registry_path=runtime_watch_registry_path,
        runtime_refresh_out_path=refresh_out_path,
        evidence_source=evidence_source,
        runner_id=runner_id,
        host_label=host_label,
        refresh_after_import=bool(args.refresh_after_import),
    )
    import_summary["runtime_import_bundle_path"] = str(bundle_path)
    import_summary["runtime_import_bundle_hash"] = sha256_file(bundle_path)
    import_summary["bundle_created_at"] = bundle.get("created_at")
    import_summary["bundle_imported_at"] = runtime_gate.timestamp_utc_now()
    import_summary["execution_class_roles"] = bundle.get("execution_class_roles")
    import_summary["runtime_budget_profile_id"] = bundle.get("runtime_budget_profile_id")
    import_summary["optional_pipeline_summary_path"] = bundle.get("pipeline_summary_path")
    if import_out_path is None:
        import_out_path = bundle_path.with_name(f"{bundle_path.stem}_import_summary.json")
    write_json(import_out_path, import_summary)
    atomic_write_text(import_out_path.with_suffix(".txt"), json.dumps(import_summary, indent=2) + "\n")
    print(str(import_out_path))
    return 0


def summarize_reverify_samples(
    *,
    history_index: dict[str, Any],
    watch_registry: dict[str, Any],
    fingerprint_key: str,
) -> dict[str, Any]:
    history_bucket = history_bucket_for_fingerprint(history_index, fingerprint_key)
    sample_count = 0
    real_sample_count = 0
    for payload in dict(history_bucket.get("execution_classes", {})).values():
        if not isinstance(payload, dict):
            continue
        for sample in payload.get("samples", []):
            if not isinstance(sample, dict):
                continue
            sample_count += 1
            if normalized_evidence_source(str(sample.get("evidence_source", "real"))) == "real":
                real_sample_count += 1
    watch = aggregate_watch_registry_for_fingerprint(watch_registry, fingerprint_key)
    return {
        "sample_count": sample_count,
        "real_sample_count": real_sample_count,
        "watch_status": watch.get("watch_status", "CLEAR"),
        "watch_confidence": watch.get("watch_confidence", "LOW"),
        "watch_reason": watch.get("watch_reason", ""),
    }


def build_known_env_reverify_gate(
    *,
    import_manifest: dict[str, Any],
    import_manifest_path: Path,
    known_entry: dict[str, Any],
    history_index: dict[str, Any],
    watch_registry: dict[str, Any],
    governance_policy: dict[str, Any] | None,
    min_real_samples: int,
    max_age_days: int,
    current_time_override: str | None = None,
) -> dict[str, Any]:
    imported_fingerprint_key = runtime_gate.runtime_manifest_fingerprint_key(import_manifest)
    known_fingerprint_key = runtime_registry_entry_fingerprint(known_entry)
    imported_fingerprint_matches_known_env = imported_fingerprint_key == known_fingerprint_key
    sample_summary = summarize_reverify_samples(
        history_index=history_index,
        watch_registry=watch_registry,
        fingerprint_key=known_fingerprint_key if imported_fingerprint_matches_known_env else imported_fingerprint_key,
    )
    watch_status = str(sample_summary.get("watch_status", "CLEAR"))
    watch_confidence = str(sample_summary.get("watch_confidence", "LOW"))
    real_sample_count = int(sample_summary.get("real_sample_count", 0))
    now = resolve_governance_now(current_time_override, 0.0)
    reference_verified_timestamp = (
        str(known_entry.get("last_verified_timestamp", "")).strip()
        or str(known_entry.get("approval_timestamp_utc", "")).strip()
        or None
    )
    observed_age_days = age_days(reference_verified_timestamp, now=now)
    effective_min_real_samples = int(
        governance_policy.get("min_real_samples_for_reverify", min_real_samples)
        if governance_policy
        else min_real_samples
    )
    effective_max_age_days = int(
        governance_policy.get("reverify_due_after_days", max_age_days)
        if governance_policy
        else max_age_days
    )
    stale_after_days = int(
        governance_policy.get("stale_after_days", effective_max_age_days)
        if governance_policy
        else effective_max_age_days
    )
    retire_candidate_after_days = int(
        governance_policy.get("retire_candidate_after_days", stale_after_days * 2)
        if governance_policy
        else max(effective_max_age_days * 2, stale_after_days)
    )
    min_watch_confidence = str(
        governance_policy.get("min_watch_confidence_for_freshen", "MEDIUM")
        if governance_policy
        else "MEDIUM"
    ).strip().upper()
    effective_watch_status = watch_status
    effective_watch_confidence = watch_confidence
    legacy_known_env_watch = (
        imported_fingerprint_matches_known_env
        and watch_status in {"REBASELINE_REQUIRED", "ACTION_REQUIRED", "NOT_COMPARABLE"}
        and real_sample_count >= effective_min_real_samples
    )
    if legacy_known_env_watch:
        effective_watch_status = "CLEAR"
        if watch_confidence_rank(effective_watch_confidence) < watch_confidence_rank(min_watch_confidence):
            effective_watch_confidence = min_watch_confidence

    verdict = "FRESHEN"
    rationale: list[str] = []
    if not imported_fingerprint_matches_known_env:
        verdict = "REJECT"
        rationale.append("imported fingerprint does not match the approved known environment lineage")
    elif effective_watch_status == "FAIL":
        verdict = "REJECT"
        rationale.append("known environment watch indicates hard breach")
    elif real_sample_count < effective_min_real_samples:
        verdict = "NEED_MORE_SAMPLES"
        rationale.append("same-fingerprint real evidence is insufficient for reverification")
    elif watch_confidence_rank(effective_watch_confidence) < watch_confidence_rank(min_watch_confidence):
        verdict = "NEED_MORE_SAMPLES"
        rationale.append("watch confidence is below the reverification threshold")
    elif observed_age_days is not None and observed_age_days >= float(retire_candidate_after_days):
        verdict = "RETIRE_CANDIDATE"
        rationale.append("known environment age exceeded the retirement threshold")
    elif observed_age_days is not None and observed_age_days >= float(stale_after_days):
        verdict = "STALE"
        rationale.append("known environment age exceeded the reverification threshold")
    else:
        rationale.append("same-fingerprint evidence freshens the approved known environment")
    if legacy_known_env_watch:
        rationale.append("same-fingerprint approved known environment evidence overrides legacy rebaseline-only watch status")
    reverify_confidence = watch_confidence if verdict != "REJECT" else "LOW"
    payload = {
        "manifest_version": "runtime_known_env_reverify_gate_v1",
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "known_env_id": known_entry.get("baseline_id"),
        "selected_known_env_baseline_id": known_entry.get("baseline_id"),
        "selected_known_env_baseline_tag": known_entry.get("baseline_tag"),
        "known_env_fingerprint_key": known_fingerprint_key,
        "imported_fingerprint_key": imported_fingerprint_key,
        "imported_fingerprint_matches_known_env": imported_fingerprint_matches_known_env,
        "runtime_import_manifest_path": str(import_manifest_path),
        "runtime_import_manifest_hash": sha256_file(import_manifest_path),
        "reverify_gate_verdict": verdict,
        "reverify_confidence": reverify_confidence,
        "sample_count": int(sample_summary.get("sample_count", 0)),
        "real_sample_count": real_sample_count,
        "watch_status": effective_watch_status,
        "watch_confidence": effective_watch_confidence,
        "raw_watch_status": watch_status,
        "raw_watch_confidence": watch_confidence,
        "age_since_last_verified_days": observed_age_days,
        "max_age_days": effective_max_age_days,
        "stale_after_days": stale_after_days,
        "retire_candidate_after_days": retire_candidate_after_days,
        "min_real_samples": effective_min_real_samples,
        "min_watch_confidence_for_freshen": min_watch_confidence,
        "governance_policy_id": None if governance_policy is None else governance_policy.get("policy_id"),
        "current_time_utc": timestamp_utc_from_datetime(now),
        "rationale": rationale,
    }
    payload["gate_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_known_env_import_summary(
    *,
    bundle: dict[str, Any],
    import_manifest: dict[str, Any],
    import_manifest_path: Path,
    known_entry: dict[str, Any] | None,
    base_import_summary: dict[str, Any],
) -> dict[str, Any]:
    imported_fingerprint_key = runtime_gate.runtime_manifest_fingerprint_key(import_manifest)
    known_fingerprint_key = None if known_entry is None else runtime_registry_entry_fingerprint(known_entry)
    matches = bool(known_entry) and imported_fingerprint_key == known_fingerprint_key
    if matches:
        import_type = "known_env_reverify"
        environment_state = ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED
        import_reason = "same fingerprint evidence imported for an approved known environment"
    else:
        import_type = "foreign_candidate"
        environment_state = ENV_STATE_FOREIGN_UNAPPROVED
        import_reason = "imported evidence does not match the selected approved known environment fingerprint"
    payload = {
        "manifest_version": "runtime_known_env_import_summary_v1",
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "runtime_import_bundle_path": base_import_summary.get("runtime_import_bundle_path"),
        "runtime_import_bundle_hash": base_import_summary.get("runtime_import_bundle_hash"),
        "runtime_import_manifest_path": str(import_manifest_path),
        "runtime_import_manifest_hash": sha256_file(import_manifest_path),
        "selected_known_env_id": None if known_entry is None else known_entry.get("baseline_id"),
        "selected_known_env_tag": None if known_entry is None else known_entry.get("baseline_tag"),
        "selected_known_env_baseline_id": None if known_entry is None else known_entry.get("baseline_id"),
        "selected_known_env_baseline_tag": None if known_entry is None else known_entry.get("baseline_tag"),
        "imported_fingerprint_key": imported_fingerprint_key,
        "selected_known_env_fingerprint_key": known_fingerprint_key,
        "import_verdict": "KNOWN_ENV_REVERIFY_APPEND" if matches else "FOREIGN_CANDIDATE",
        "import_type": import_type,
        "proposal_needed": False if matches else bool(base_import_summary.get("proposal_needed", True)),
        "reverify_needed": bool(matches),
        "counts_as_real_evidence": bool(base_import_summary.get("counts_as_real_evidence", False)),
        "import_reason": import_reason,
        "environment_state": environment_state,
        "state": environment_state,
        "selected_baseline_id": base_import_summary.get("selected_baseline_id"),
        "comparability_verdict": base_import_summary.get("comparability_verdict"),
        "runner_id": bundle.get("runner_id") or base_import_summary.get("runner_id"),
        "host_label": bundle.get("host_label") or base_import_summary.get("host_label"),
    }
    payload["import_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_known_env_retire_plan(
    *,
    phase: str,
    known_entry: dict[str, Any],
    runtime_history_index: dict[str, Any],
    runtime_watch_registry: dict[str, Any],
    governance_policy: dict[str, Any] | None,
    current_time_override: str | None,
    retire_reason: str,
) -> dict[str, Any]:
    fingerprint_key = runtime_registry_entry_fingerprint(known_entry)
    latest_sample = latest_history_sample_for_fingerprint(runtime_history_index, fingerprint_key)
    latest_watch = aggregate_watch_registry_for_fingerprint(runtime_watch_registry, fingerprint_key)
    now = resolve_governance_now(current_time_override, 0.0)
    effective_policy = governance_policy or normalize_known_env_governance_policy({}, phase=phase)
    evaluated = evaluate_known_env_governance_entry(
        entry=known_entry,
        latest_sample=latest_sample,
        watch_summary=latest_watch,
        policy=effective_policy,
        now=now,
        is_current_environment=False,
        comparability_verdict="COMPARABLE",
        freshness_verdict="FRESH",
    )
    approval_timestamp = str(evaluated.get("approval_timestamp_utc", "")).strip() or None
    latest_evidence_timestamp = (
        evaluated.get("last_runtime_import_timestamp")
        or latest_watch.get("latest_real_sample_timestamp")
        or latest_watch.get("latest_fixture_sample_timestamp")
        or latest_sample.get("timestamp_utc")
        or approval_timestamp
    )
    verified_age_days = evaluated.get("age_since_last_verified_days")
    latest_evidence_age_days = age_days(str(latest_evidence_timestamp or "").strip() or None, now=now)
    current_state = str(evaluated.get("state", ENV_STATE_APPROVED_KNOWN_FRESH))
    if current_state == ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE:
        verdict = "RETIRE_CANDIDATE"
        recommended_action = "RETIRE_KNOWN_ENV"
    elif current_state in {ENV_STATE_APPROVED_KNOWN_STALE, ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED}:
        verdict = "REVERIFY_REQUIRED"
        recommended_action = "REVERIFY_KNOWN_ENV"
    else:
        verdict = "RETAIN"
        recommended_action = "NO_ACTION"
    payload = {
        "manifest_version": "runtime_known_env_retire_plan_v1",
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "phase": phase,
        "governance_policy_id": effective_policy.get("policy_id"),
        "current_time_utc": timestamp_utc_from_datetime(now),
        "known_env_id": known_entry.get("baseline_id"),
        "baseline_id": known_entry.get("baseline_id"),
        "baseline_tag": known_entry.get("baseline_tag"),
        "fingerprint_key": fingerprint_key,
        "current_state": current_state,
        "retire_reason": retire_reason,
        "plan_verdict": verdict,
        "recommended_action": recommended_action,
        "age_since_last_verified_days": verified_age_days,
        "age_since_latest_evidence_days": latest_evidence_age_days,
        "approval_timestamp_utc": approval_timestamp,
        "latest_evidence_timestamp": latest_evidence_timestamp,
        "latest_history_timestamp": latest_sample.get("timestamp_utc"),
        "watch_status": latest_watch.get("watch_status"),
        "watch_confidence": latest_watch.get("watch_confidence"),
        "latest_watch_summary_path": latest_watch.get("latest_watch_summary_path"),
        "latest_refresh_manifest_path": evaluated.get("latest_refresh_manifest_path"),
        "due_at": evaluated.get("due_at"),
        "overdue_days": evaluated.get("overdue_days", 0.0),
        "rationale": [
            "retirement planning never changes the current environment active baseline",
            "retirement policy considers both last verified age and the latest available evidence age before recommending archive-only status",
        ],
    }
    payload["plan_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_known_env_age_tick(
    *,
    registry: dict[str, Any],
    runtime_history_index: dict[str, Any],
    runtime_watch_registry: dict[str, Any],
    governance_policy: dict[str, Any],
    current_time_override: str | None,
    advance_days: float,
) -> dict[str, Any]:
    base_now = resolve_governance_now(current_time_override, 0.0)
    aged_now = resolve_governance_now(current_time_override, advance_days)
    transitions: list[dict[str, Any]] = []
    due_soon_reverify_count = 0
    reverify_required_count = 0
    stale_count = 0
    retire_candidate_count = 0
    retired_count = 0
    affected_known_env_count = 0
    for entry in active_runtime_registry_entries(registry):
        if bool(entry.get("counts_as_current_env", False)):
            continue
        fingerprint_key = runtime_registry_entry_fingerprint(entry)
        latest_sample = latest_history_sample_for_fingerprint(runtime_history_index, fingerprint_key)
        watch_summary = aggregate_watch_registry_for_fingerprint(runtime_watch_registry, fingerprint_key)
        current_eval = evaluate_known_env_governance_entry(
            entry=entry,
            latest_sample=latest_sample,
            watch_summary=watch_summary,
            policy=governance_policy,
            now=base_now,
            is_current_environment=False,
            comparability_verdict="COMPARABLE",
            freshness_verdict="FRESH",
        )
        aged_eval = evaluate_known_env_governance_entry(
            entry=entry,
            latest_sample=latest_sample,
            watch_summary=watch_summary,
            policy=governance_policy,
            now=aged_now,
            is_current_environment=False,
            comparability_verdict="COMPARABLE",
            freshness_verdict="FRESH",
        )
        current_state = str(current_eval.get("state", ENV_STATE_APPROVED_KNOWN_FRESH))
        aged_state = str(aged_eval.get("state", ENV_STATE_APPROVED_KNOWN_FRESH))
        if aged_state == ENV_STATE_APPROVED_KNOWN_FRESH and bool(aged_eval.get("due_soon", False)):
            due_soon_reverify_count += 1
        elif aged_state == ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED:
            reverify_required_count += 1
        elif aged_state == ENV_STATE_APPROVED_KNOWN_STALE:
            stale_count += 1
        elif aged_state == ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE:
            retire_candidate_count += 1
        elif aged_state == ENV_STATE_RETIRED_KNOWN_ENV:
            retired_count += 1
        if current_state != aged_state or bool(aged_eval.get("due_soon", False)):
            affected_known_env_count += 1
            transitions.append(
                {
                    "known_env_id": entry.get("baseline_id"),
                    "fingerprint_key": fingerprint_key,
                    "current_state": current_state,
                    "next_state": aged_state,
                    "due_soon": bool(aged_eval.get("due_soon", False)),
                    "overdue_days": aged_eval.get("overdue_days", 0.0),
                    "current_reference_time_utc": timestamp_utc_from_datetime(base_now),
                    "aged_reference_time_utc": timestamp_utc_from_datetime(aged_now),
                }
            )
    payload = {
        "manifest_version": "runtime_known_env_age_tick_v1",
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "governance_policy_id": governance_policy.get("policy_id"),
        "current_time_utc": timestamp_utc_from_datetime(base_now),
        "aged_time_utc": timestamp_utc_from_datetime(aged_now),
        "advance_days": float(advance_days),
        "affected_known_env_count": affected_known_env_count,
        "due_soon_reverify_count": due_soon_reverify_count,
        "reverify_required_count": reverify_required_count,
        "stale_count": stale_count,
        "retire_candidate_count": retire_candidate_count,
        "retired_count": retired_count,
        "transitions": transitions,
    }
    payload["age_tick_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def build_known_env_reverify_plan(
    *,
    registry: dict[str, Any],
    runtime_history_index: dict[str, Any],
    runtime_watch_registry: dict[str, Any],
    governance_policy: dict[str, Any],
    current_time_override: str | None,
    known_env_filter: list[str],
) -> dict[str, Any]:
    now = resolve_governance_now(current_time_override, 0.0)
    filter_set = {value.strip() for value in known_env_filter if value.strip()}
    entries: list[dict[str, Any]] = []
    for entry in active_runtime_registry_entries(registry):
        if bool(entry.get("counts_as_current_env", False)):
            continue
        known_env_id = str(entry.get("baseline_id", "")).strip()
        if filter_set and known_env_id not in filter_set and str(entry.get("baseline_tag", "")).strip() not in filter_set:
            continue
        fingerprint_key = runtime_registry_entry_fingerprint(entry)
        latest_sample = latest_history_sample_for_fingerprint(runtime_history_index, fingerprint_key)
        watch_summary = aggregate_watch_registry_for_fingerprint(runtime_watch_registry, fingerprint_key)
        evaluated = evaluate_known_env_governance_entry(
            entry=entry,
            latest_sample=latest_sample,
            watch_summary=watch_summary,
            policy=governance_policy,
            now=now,
            is_current_environment=False,
            comparability_verdict="COMPARABLE",
            freshness_verdict="FRESH",
        )
        current_state = str(evaluated.get("state", ""))
        due_soon = bool(evaluated.get("due_soon", False))
        if current_state not in {ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED, ENV_STATE_APPROVED_KNOWN_STALE} and not due_soon:
            continue
        priority = "high" if current_state in {ENV_STATE_APPROVED_KNOWN_STALE, ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED} else "medium"
        entries.append(
            {
                "known_env_id": known_env_id,
                "fingerprint_key": fingerprint_key,
                "baseline_id": entry.get("baseline_id"),
                "baseline_tag": entry.get("baseline_tag"),
                "current_state": current_state,
                "next_state_if_success": ENV_STATE_APPROVED_KNOWN_FRESH,
                "reverify_due_reason": "due soon" if due_soon else "state requires fresh same-fingerprint evidence",
                "recommended_command": (
                    "python tests/tools/runtime_watch_ops.py import-known-env-evidence "
                    f"--known-env-id {known_env_id} --runtime-import-bundle <bundle> "
                    "&& python tests/tools/runtime_watch_ops.py known-env-apply-reverify "
                    f"--known-env-id {known_env_id} --runtime-import-manifest <runtime_current_manifest>"
                ),
                "minimum_required_real_samples": int(governance_policy.get("min_real_samples_for_reverify", 0)),
                "target_runner_label": entry.get("baseline_tag") or fingerprint_key,
                "priority": priority,
                "due_at": evaluated.get("due_at"),
                "overdue_days": evaluated.get("overdue_days", 0.0),
            }
        )
    entries.sort(key=lambda item: (0 if item.get("priority") == "high" else 1, str(item.get("due_at") or "")))
    verdict = "EMPTY"
    if entries:
        verdict = "ACTION_REQUIRED" if any(item.get("priority") == "high" for item in entries) else "PASS"
    payload = {
        "manifest_version": "runtime_known_env_reverify_plan_v1",
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "governance_policy_id": governance_policy.get("policy_id"),
        "current_time_utc": timestamp_utc_from_datetime(now),
        "plan_verdict": verdict,
        "entry_count": len(entries),
        "entries": entries,
    }
    payload["plan_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def update_registry_entry_governance(
    *,
    registry: dict[str, Any],
    known_env_id: str,
    governance_update: dict[str, Any],
) -> dict[str, Any]:
    for entry in registry.get("entries", []):
        if not isinstance(entry, dict):
            continue
        if str(entry.get("baseline_id", "")).strip() != known_env_id:
            continue
        entry.update(governance_update)
        break
    return runtime_gate.finalize_runtime_registry(registry)


def apply_known_env_reverify(
    *,
    registry_path: Path,
    import_manifest_path: Path,
    known_entry: dict[str, Any],
    gate_payload: dict[str, Any],
    governance_policy: dict[str, Any],
    archive_import_path: Path | None,
) -> dict[str, Any]:
    now_text = runtime_gate.timestamp_utc_now()
    verdict = str(gate_payload.get("reverify_gate_verdict", "REJECT"))
    previous_state = str(known_entry.get("known_env_state") or known_entry.get("state") or ENV_STATE_APPROVED_KNOWN_REVERIFY_REQUIRED)
    if verdict == "FRESHEN":
        next_state = ENV_STATE_APPROVED_KNOWN_FRESH
    elif verdict == "STALE":
        next_state = ENV_STATE_APPROVED_KNOWN_STALE
    elif verdict == "RETIRE_CANDIDATE":
        next_state = ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE
    else:
        next_state = previous_state
    registry = runtime_gate.load_runtime_registry(registry_path)
    registry = update_registry_entry_governance(
        registry=registry,
        known_env_id=str(known_entry.get("baseline_id", "")),
        governance_update={
            "known_env_state": next_state,
            "last_runtime_import_timestamp": stable_manifest_timestamp(read_json(import_manifest_path)),
            "last_verified_timestamp": now_text if verdict == "FRESHEN" else known_entry.get("last_verified_timestamp") or known_entry.get("approval_timestamp_utc"),
            "governance_policy_id": governance_policy.get("policy_id"),
            "counts_as_current_env": False,
            "counts_as_approved_known_env": True,
        },
    )
    runtime_gate.write_runtime_registry_outputs(registry_path, registry)
    archive_payload = {
        "manifest_version": "runtime_known_env_reverify_import_archive_v1",
        "archived_at_utc": now_text,
        "runtime_import_manifest_path": str(import_manifest_path),
        "runtime_import_manifest_hash": sha256_file(import_manifest_path),
        "known_env_id": known_entry.get("baseline_id"),
        "gate_verdict": verdict,
    }
    if archive_import_path is not None:
        write_json(archive_import_path, archive_payload)
        atomic_write_text(archive_import_path.with_suffix(".txt"), json.dumps(archive_payload, indent=2) + "\n")
    payload = {
        "manifest_version": "runtime_known_env_apply_reverify_v1",
        "generated_at_utc": now_text,
        "known_env_id": known_entry.get("baseline_id"),
        "baseline_id": known_entry.get("baseline_id"),
        "baseline_tag": known_entry.get("baseline_tag"),
        "previous_state": previous_state,
        "gate_verdict": verdict,
        "new_state": next_state,
        "archive_import_path": None if archive_import_path is None else str(archive_import_path),
        "runtime_baseline_registry_path": str(registry_path),
        "governance_policy_id": governance_policy.get("policy_id"),
    }
    payload["apply_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    return payload


def apply_known_env_retire(
    *,
    registry_path: Path,
    known_entry: dict[str, Any],
    retire_reason: str,
    archive_out: Path | None,
) -> dict[str, Any]:
    if bool(known_entry.get("counts_as_current_env", False)):
        raise SystemExit("current environment active lineage cannot be retired")
    previous_state = str(known_entry.get("known_env_state") or known_entry.get("state") or ENV_STATE_APPROVED_KNOWN_RETIRE_CANDIDATE)
    payload = retire_known_env_entry(
        registry_path=registry_path,
        known_entry=known_entry,
        retire_reason=retire_reason,
        archive_out=archive_out,
    )
    payload["previous_state"] = previous_state
    payload["retired_at"] = payload.get("generated_at_utc")
    payload["archive_path"] = None if archive_out is None else str(archive_out)
    payload["retire_reason"] = retire_reason
    payload["apply_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    if archive_out is not None:
        write_json(archive_out, payload)
        atomic_write_text(archive_out.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    registry = runtime_gate.load_runtime_registry(registry_path)
    registry = update_registry_entry_governance(
        registry=registry,
        known_env_id=str(known_entry.get("baseline_id", "")),
        governance_update={
            "known_env_state": ENV_STATE_RETIRED_KNOWN_ENV,
            "counts_as_current_env": False,
            "counts_as_approved_known_env": False,
        },
    )
    runtime_gate.write_runtime_registry_outputs(registry_path, registry)
    return payload


def retire_known_env_entry(
    *,
    registry_path: Path,
    known_entry: dict[str, Any],
    retire_reason: str,
    archive_out: Path | None,
) -> dict[str, Any]:
    registry = runtime_gate.load_runtime_registry(registry_path)
    registry, retired = runtime_gate.retire_runtime_registry_entry(registry, str(known_entry.get("baseline_id", "")))
    if retired is None:
        raise SystemExit("known environment baseline not found during retirement")
    retired["retired_reason"] = retire_reason
    runtime_gate.write_runtime_registry_outputs(registry_path, registry)
    payload = {
        "manifest_version": "runtime_known_env_retire_v1",
        "generated_at_utc": runtime_gate.timestamp_utc_now(),
        "baseline_id": retired.get("baseline_id"),
        "baseline_tag": retired.get("baseline_tag"),
        "fingerprint_key": runtime_registry_entry_fingerprint(retired),
        "status": ENV_STATE_RETIRED_KNOWN_ENV,
        "state": ENV_STATE_RETIRED_KNOWN_ENV,
        "retired_reason": retire_reason,
        "runtime_baseline_registry_path": str(registry_path),
    }
    payload["retire_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    if archive_out is not None:
        write_json(archive_out, payload)
        atomic_write_text(archive_out.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    return payload


def action_import_known_env_evidence(args: argparse.Namespace) -> int:
    bundle_path = resolve_json_path(args.runtime_import_bundle)
    runtime_registry_path = resolve_json_path(args.runtime_baseline_registry)
    runtime_history_index_path = resolve_json_path(args.runtime_history_index)
    runtime_watch_registry_path = resolve_json_path(args.runtime_watch_registry)
    import_out_path = resolve_json_path(args.import_out)
    if (
        bundle_path is None
        or runtime_registry_path is None
        or runtime_history_index_path is None
        or runtime_watch_registry_path is None
    ):
        raise SystemExit("--runtime-import-bundle, --runtime-baseline-registry, --runtime-history-index, and --runtime-watch-registry are required")
    bundle = read_json(bundle_path)
    manifest_value = bundle.get("runtime_current_manifest_path") or bundle.get("runtime_current_manifest") or bundle.get("runtime_current")
    if not manifest_value:
        raise SystemExit("runtime import bundle is missing runtime_current_manifest_path")
    import_manifest_path = resolve_json_path(str(manifest_value))
    if import_manifest_path is None or not import_manifest_path.exists():
        raise SystemExit("runtime import bundle points at a missing runtime current manifest")
    known_entry = resolve_known_env_entry(runtime_gate.load_runtime_registry(runtime_registry_path), args.known_env_id)
    refresh_out_path = resolve_json_path(args.runtime_refresh_out)
    _imported_registry, import_summary = import_runtime_current_manifest(
        import_manifest_path=import_manifest_path,
        runtime_registry_path=runtime_registry_path,
        runtime_history_index_path=runtime_history_index_path,
        runtime_watch_registry_path=runtime_watch_registry_path,
        runtime_refresh_out_path=refresh_out_path,
        evidence_source=str(bundle.get("evidence_source") or "real"),
        runner_id=str(args.runner_id or bundle.get("runner_id") or ""),
        host_label=str(args.host_label or bundle.get("host_label") or ""),
        refresh_after_import=bool(args.refresh_after_import),
    )
    payload = build_known_env_import_summary(
        bundle=bundle,
        import_manifest=read_json(import_manifest_path),
        import_manifest_path=import_manifest_path,
        known_entry=known_entry,
        base_import_summary=import_summary,
    )
    if import_out_path is None:
        import_out_path = bundle_path.with_name(f"{bundle_path.stem}_known_env_import_summary.json")
    write_json(import_out_path, payload)
    atomic_write_text(import_out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(import_out_path))
    return 0


def build_current_env_watch_text(payload: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={payload.get('manifest_version', '')}",
        f"manifest_role={payload.get('manifest_role', '')}",
        f"phase={payload.get('phase', '')}",
        f"current_env_state={payload.get('current_env_state', '')}",
        f"watch_status={payload.get('watch_status', '')}",
        f"watch_confidence={payload.get('watch_confidence', '')}",
        f"reproposal_needed={int(bool(payload.get('reproposal_needed', False)))}",
        f"reproposal_gate_verdict={payload.get('reproposal_gate_verdict', '')}",
        f"selected_budget_profile_id={payload.get('selected_budget_profile_id', '')}",
        f"selected_runtime_baseline_id={payload.get('selected_runtime_baseline_id', '')}",
    ]
    for entry in payload.get("entries", []):
        lines.append(
            "current_env_watch_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" state={entry.get('current_env_state', '')}"
            + f" watch_status={entry.get('watch_status', '')}"
            + f" real_sample_count={entry.get('real_sample_count', 0)}"
            + f" stable_overrun_count={entry.get('stable_overrun_count', 0)}"
        )
    for item in payload.get("rationale", []):
        lines.append(f"rationale={item}")
    return "\n".join(lines) + "\n"


def build_current_env_watch_history_text(payload: dict[str, Any]) -> str:
    lines = [
        f"manifest_version={payload.get('manifest_version', '')}",
        f"phase={payload.get('phase', '')}",
        f"current_env_state={payload.get('current_env_state', '')}",
        f"affected_execution_class_count={payload.get('affected_execution_class_count', 0)}",
        f"sample_count={payload.get('sample_count', 0)}",
        f"real_sample_count={payload.get('real_sample_count', 0)}",
        f"transition_count={payload.get('transition_count', 0)}",
    ]
    for entry in payload.get("entries", []):
        lines.append(
            "current_env_watch_history_entry="
            + f"execution_class={entry.get('execution_class', '')}"
            + f" state={entry.get('current_env_state', '')}"
            + f" sample_count={entry.get('sample_count', 0)}"
            + f" trend_direction={entry.get('trend_direction', '')}"
        )
    return "\n".join(lines) + "\n"


def action_current_env_governance_policy(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.policy_out)
    if out_path is None:
        raise SystemExit("--policy-out is required")
    source_path = resolve_json_path(getattr(args, "policy_source", None))
    payload = load_current_env_governance_policy(source_path, phase=str(args.phase))
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_watch_current(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.out)
    if out_path is None:
        raise SystemExit("--out is required")
    text_out = Path(args.out_text).resolve() if args.out_text else out_path.with_suffix(".summary.txt")
    runtime_refresh_path = resolve_json_path(args.runtime_refresh)
    runtime_budget_current_path = resolve_json_path(args.runtime_budget_current)
    runtime_watch_current_path = resolve_json_path(args.runtime_watch_current)
    governance_policy = load_current_env_governance_policy(
        resolve_json_path(args.governance_policy),
        phase=str(args.phase),
    )
    payload = build_current_env_watch_manifest(
        phase=str(args.phase),
        manifest_role="current",
        runtime_refresh=read_json(runtime_refresh_path),
        runtime_refresh_path=runtime_refresh_path,
        runtime_budget_current=read_json(runtime_budget_current_path),
        runtime_budget_current_path=runtime_budget_current_path,
        runtime_watch_manifest=read_json(runtime_watch_current_path),
        runtime_watch_manifest_path=runtime_watch_current_path,
        governance_policy=governance_policy,
    )
    write_json(out_path, payload)
    atomic_write_text(text_out, build_current_env_watch_text(payload))
    print(str(out_path))
    return 0


def action_current_env_watch_refresh(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.out)
    if out_path is None:
        raise SystemExit("--out is required")
    text_out = Path(args.out_text).resolve() if args.out_text else out_path.with_suffix(".summary.txt")
    runtime_refresh_path = resolve_json_path(args.runtime_refresh)
    runtime_budget_current_path = resolve_json_path(args.runtime_budget_current)
    runtime_watch_refresh_path = resolve_json_path(args.runtime_watch_refresh)
    runtime_budget_proposal_path = resolve_json_path(args.runtime_budget_proposal)
    runtime_budget_proposal_gate_path = resolve_json_path(args.runtime_budget_proposal_gate)
    runtime_budget_baseline_path = resolve_json_path(args.runtime_budget_baseline)
    governance_policy = load_current_env_governance_policy(
        resolve_json_path(args.governance_policy),
        phase=str(args.phase),
    )
    payload = build_current_env_watch_manifest(
        phase=str(args.phase),
        manifest_role="refresh",
        runtime_refresh=read_json(runtime_refresh_path),
        runtime_refresh_path=runtime_refresh_path,
        runtime_budget_current=read_json(runtime_budget_current_path),
        runtime_budget_current_path=runtime_budget_current_path,
        runtime_watch_manifest=read_json(runtime_watch_refresh_path),
        runtime_watch_manifest_path=runtime_watch_refresh_path,
        governance_policy=governance_policy,
        runtime_budget_proposal=read_json(runtime_budget_proposal_path),
        runtime_budget_proposal_path=runtime_budget_proposal_path,
        runtime_budget_proposal_gate=read_json(runtime_budget_proposal_gate_path),
        runtime_budget_proposal_gate_path=runtime_budget_proposal_gate_path,
        runtime_budget_baseline=read_json(runtime_budget_baseline_path),
    )
    write_json(out_path, payload)
    atomic_write_text(text_out, build_current_env_watch_text(payload))
    print(str(out_path))
    return 0


def action_current_env_watch_history(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.out)
    if out_path is None:
        raise SystemExit("--out is required")
    text_out = Path(args.out_text).resolve() if args.out_text else out_path.with_suffix(".summary.txt")
    runtime_watch_history_path = resolve_json_path(args.runtime_watch_history_index)
    runtime_budget_current_path = resolve_json_path(args.runtime_budget_current)
    runtime_budget_proposal_gate_path = resolve_json_path(args.runtime_budget_proposal_gate)
    runtime_budget_baseline_path = resolve_json_path(getattr(args, "runtime_budget_baseline", None))
    governance_policy = load_current_env_governance_policy(
        resolve_json_path(args.governance_policy),
        phase=str(args.phase),
    )
    payload = build_current_env_watch_history(
        phase=str(args.phase),
        runtime_watch_history=read_json(runtime_watch_history_path),
        runtime_watch_history_path=runtime_watch_history_path,
        runtime_budget_current=read_json(runtime_budget_current_path),
        runtime_budget_current_path=runtime_budget_current_path,
        governance_policy=governance_policy,
        runtime_budget_proposal_gate=read_json(runtime_budget_proposal_gate_path),
        runtime_budget_proposal_gate_path=runtime_budget_proposal_gate_path,
        runtime_budget_baseline=read_json(runtime_budget_baseline_path),
        runtime_budget_baseline_path=runtime_budget_baseline_path,
    )
    write_json(out_path, payload)
    atomic_write_text(text_out, build_current_env_watch_history_text(payload))
    print(str(out_path))
    return 0


def action_current_env_age_tick(args: argparse.Namespace) -> int:
    runtime_current_manifest_path = resolve_json_path(args.runtime_current_manifest)
    runtime_watch_current_path = resolve_json_path(args.runtime_watch_current)
    runtime_watch_refresh_path = resolve_json_path(args.runtime_watch_refresh)
    runtime_watch_history_path = resolve_json_path(args.runtime_watch_history)
    runtime_budget_baseline_path = resolve_json_path(args.runtime_budget_baseline)
    guardrail_policy_path = resolve_json_path(args.guardrail_policy)
    out_path = resolve_json_path(args.age_tick_out)
    if (
        runtime_current_manifest_path is None
        or runtime_watch_current_path is None
        or runtime_watch_refresh_path is None
        or runtime_watch_history_path is None
        or runtime_budget_baseline_path is None
        or guardrail_policy_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-current-manifest, --runtime-watch-current, --runtime-watch-refresh, --runtime-watch-history, --runtime-budget-baseline, --guardrail-policy, and --age-tick-out are required")
    payload = build_current_env_age_tick(
        phase=str(args.phase),
        runtime_current_manifest=read_json(runtime_current_manifest_path),
        runtime_watch_current=read_json(runtime_watch_current_path),
        runtime_watch_refresh=read_json(runtime_watch_refresh_path),
        runtime_watch_history=read_json(runtime_watch_history_path),
        runtime_budget_baseline=read_json(runtime_budget_baseline_path),
        guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
        current_time_override=getattr(args, "current_time_override", None),
        advance_days=float(getattr(args, "advance_days", 0.0) or 0.0),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_plan_watch(args: argparse.Namespace) -> int:
    runtime_current_manifest_path = resolve_json_path(args.runtime_current_manifest)
    runtime_watch_history_path = resolve_json_path(args.runtime_watch_history)
    guardrail_policy_path = resolve_json_path(args.guardrail_policy)
    current_env_age_tick_path = resolve_json_path(getattr(args, "current_env_age_tick", None))
    runtime_budget_baseline_path = resolve_json_path(getattr(args, "runtime_budget_baseline", None))
    runtime_watch_current_path = resolve_json_path(getattr(args, "runtime_watch_current", None))
    runtime_watch_refresh_path = resolve_json_path(getattr(args, "runtime_watch_refresh", None))
    out_path = resolve_json_path(args.plan_out)
    if runtime_current_manifest_path is None or runtime_watch_history_path is None or guardrail_policy_path is None or out_path is None:
        raise SystemExit("--runtime-current-manifest, --runtime-watch-history, --guardrail-policy, and --plan-out are required")
    current_time_override = getattr(args, "current_time_override", None)
    age_tick = read_json(current_env_age_tick_path) if current_env_age_tick_path is not None else {}
    if not age_tick:
        age_tick = build_current_env_age_tick(
            phase=str(args.phase),
            runtime_current_manifest=read_json(runtime_current_manifest_path),
            runtime_watch_current=read_json(runtime_watch_current_path),
            runtime_watch_refresh=read_json(runtime_watch_refresh_path),
            runtime_watch_history=read_json(runtime_watch_history_path),
            runtime_budget_baseline=read_json(runtime_budget_baseline_path),
            guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
            current_time_override=current_time_override,
            advance_days=0.0,
        )
    payload = build_current_env_watch_plan(
        phase=str(args.phase),
        runtime_current_manifest=read_json(runtime_current_manifest_path),
        runtime_watch_history=read_json(runtime_watch_history_path),
        guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
        current_env_age_tick=age_tick,
        execution_class_filter=str(getattr(args, "execution_class_filter", "") or ""),
        current_time_override=current_time_override,
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_reproposal_trigger_gate(args: argparse.Namespace) -> int:
    runtime_current_manifest_path = resolve_json_path(args.runtime_current_manifest)
    runtime_watch_current_path = resolve_json_path(args.runtime_watch_current)
    runtime_watch_history_path = resolve_json_path(args.runtime_watch_history)
    runtime_budget_baseline_path = resolve_json_path(args.runtime_budget_baseline)
    guardrail_policy_path = resolve_json_path(args.guardrail_policy)
    out_path = resolve_json_path(args.trigger_gate_out)
    if (
        runtime_current_manifest_path is None
        or runtime_watch_current_path is None
        or runtime_watch_history_path is None
        or runtime_budget_baseline_path is None
        or guardrail_policy_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-current-manifest, --runtime-watch-current, --runtime-watch-history, --runtime-budget-baseline, --guardrail-policy, and --trigger-gate-out are required")
    payload = build_current_env_reproposal_trigger_gate(
        phase=str(args.phase),
        runtime_current_manifest=read_json(runtime_current_manifest_path),
        runtime_watch_current=read_json(runtime_watch_current_path),
        runtime_watch_history=read_json(runtime_watch_history_path),
        runtime_budget_baseline=read_json(runtime_budget_baseline_path),
        guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
        current_time_override=getattr(args, "current_time_override", None),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_due_scheduler(args: argparse.Namespace) -> int:
    runtime_current_manifest_path = resolve_json_path(args.runtime_current_manifest)
    runtime_watch_current_path = resolve_json_path(args.runtime_current_env_watch)
    runtime_watch_refresh_path = resolve_json_path(args.runtime_current_env_watch_refresh)
    runtime_watch_history_path = resolve_json_path(args.runtime_watch_history)
    runtime_budget_baseline_path = resolve_json_path(args.runtime_budget_baseline)
    guardrail_policy_path = resolve_json_path(args.guardrail_policy)
    runtime_watch_apply_path = resolve_json_path(getattr(args, "runtime_current_env_watch_apply", None))
    out_path = resolve_json_path(args.due_scheduler_out)
    if (
        runtime_current_manifest_path is None
        or runtime_watch_current_path is None
        or runtime_watch_refresh_path is None
        or runtime_watch_history_path is None
        or runtime_budget_baseline_path is None
        or guardrail_policy_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-current-manifest, --runtime-current-env-watch, --runtime-current-env-watch-refresh, --runtime-watch-history, --runtime-budget-baseline, --guardrail-policy, and --due-scheduler-out are required")
    payload = build_current_env_due_scheduler(
        phase=str(args.phase),
        runtime_current_manifest=read_json(runtime_current_manifest_path),
        runtime_watch_current=read_json(runtime_watch_current_path),
        runtime_watch_refresh=read_json(runtime_watch_refresh_path),
        runtime_watch_history=read_json(runtime_watch_history_path),
        runtime_budget_baseline=read_json(runtime_budget_baseline_path),
        guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
        current_env_watch_apply=read_json(runtime_watch_apply_path),
        current_time_override=getattr(args, "current_time_override", None),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), build_policy_ops_agenda_text({
        "manifest_version": payload.get("manifest_version"),
        "phase": payload.get("phase"),
        "now_utc": payload.get("now_utc"),
        "item_count": 1,
        "action_required_count": 0 if payload.get("recommended_action_current_env") in {"", "NO_ACTION"} else 1,
        "blocking_action_count": 0,
        "highest_priority_domain": "current_env",
        "highest_priority_action": payload.get("recommended_action_current_env"),
    }))
    print(str(out_path))
    return 0


def action_current_env_plan_reproposal(args: argparse.Namespace) -> int:
    runtime_current_manifest_path = resolve_json_path(args.runtime_current_manifest)
    runtime_watch_history_path = resolve_json_path(args.runtime_watch_history)
    guardrail_policy_path = resolve_json_path(args.guardrail_policy)
    current_env_due_path = resolve_json_path(getattr(args, "current_env_due", None))
    out_path = resolve_json_path(args.plan_out)
    if runtime_current_manifest_path is None or runtime_watch_history_path is None or guardrail_policy_path is None or out_path is None:
        raise SystemExit("--runtime-current-manifest, --runtime-watch-history, --guardrail-policy, and --plan-out are required")
    current_env_due = read_json(current_env_due_path) if current_env_due_path is not None else {}
    if not current_env_due:
        current_env_due = {
            "current_state_after": CURRENT_ENV_STATE_CLEAR,
            "reproposal_due_state": CURRENT_ENV_DUE_NOT_DUE,
        }
    payload = build_current_env_reproposal_plan(
        phase=str(args.phase),
        runtime_current_manifest=read_json(runtime_current_manifest_path),
        runtime_watch_history=read_json(runtime_watch_history_path),
        guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
        current_env_due=current_env_due,
        execution_class_filter=str(getattr(args, "execution_class_filter", "") or ""),
        current_time_override=getattr(args, "current_time_override", None),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_execute_watch(args: argparse.Namespace) -> int:
    runtime_current_manifest_path = resolve_json_path(args.runtime_current_manifest)
    runtime_baseline_manifest_path = resolve_json_path(args.runtime_baseline_manifest)
    runtime_watch_path = resolve_json_path(args.runtime_current_env_watch)
    runtime_watch_history_path = resolve_json_path(args.runtime_watch_history)
    guardrail_policy_path = resolve_json_path(args.guardrail_policy)
    out_path = resolve_json_path(args.execute_out)
    if (
        runtime_current_manifest_path is None
        or runtime_baseline_manifest_path is None
        or runtime_watch_path is None
        or runtime_watch_history_path is None
        or guardrail_policy_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-current-manifest, --runtime-baseline-manifest, --runtime-current-env-watch, --runtime-watch-history, --guardrail-policy, and --execute-out are required")
    artifact_dir = Path(args.artifact_dir).resolve() if getattr(args, "artifact_dir", None) else None
    produced_watch_manifest = None
    produced_history_update = None
    if artifact_dir is not None:
        artifact_dir.mkdir(parents=True, exist_ok=True)
        produced_watch_path = artifact_dir / f"{args.action_id}_produced_watch_manifest.json"
        produced_history_path = artifact_dir / f"{args.action_id}_produced_history_update.json"
        write_json(produced_watch_path, read_json(runtime_watch_path))
        write_json(produced_history_path, read_json(runtime_watch_history_path))
        produced_watch_manifest = str(produced_watch_path)
        produced_history_update = str(produced_history_path)
    payload = build_current_env_watch_execute(
        phase=str(args.phase),
        runtime_current_manifest=read_json(runtime_current_manifest_path),
        runtime_baseline_manifest=read_json(runtime_baseline_manifest_path),
        runtime_current_env_watch=read_json(runtime_watch_path),
        runtime_watch_history=read_json(runtime_watch_history_path),
        guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
        execution_class=str(args.execution_class),
        repeat=int(args.repeat),
        action_id=str(args.action_id),
        produced_watch_manifest=produced_watch_manifest,
        produced_history_update=produced_history_update,
    )
    payload["execute_manifest_path"] = str(out_path)
    payload["execute_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_apply_watch(args: argparse.Namespace) -> int:
    runtime_current_manifest_path = resolve_json_path(args.runtime_current_manifest)
    runtime_watch_path = resolve_json_path(args.runtime_current_env_watch)
    runtime_watch_execute_path = resolve_json_path(args.runtime_watch_execute)
    runtime_watch_history_path = resolve_json_path(args.runtime_watch_history)
    guardrail_policy_path = resolve_json_path(args.guardrail_policy)
    out_path = resolve_json_path(args.apply_out)
    if (
        runtime_current_manifest_path is None
        or runtime_watch_path is None
        or runtime_watch_execute_path is None
        or runtime_watch_history_path is None
        or guardrail_policy_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-current-manifest, --runtime-current-env-watch, --runtime-watch-execute, --runtime-watch-history, --guardrail-policy, and --apply-out are required")
    payload = build_current_env_watch_apply(
        phase=str(args.phase),
        runtime_current_manifest=read_json(runtime_current_manifest_path),
        runtime_current_env_watch=read_json(runtime_watch_path),
        runtime_watch_execute=read_json(runtime_watch_execute_path),
        runtime_watch_history=read_json(runtime_watch_history_path),
        guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
        current_time_override=getattr(args, "current_time_override", None),
    )
    payload["apply_manifest_path"] = str(out_path)
    payload["apply_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_execute_reproposal_gate(args: argparse.Namespace) -> int:
    runtime_current_manifest_path = resolve_json_path(args.runtime_current_manifest)
    runtime_watch_current_path = resolve_json_path(args.runtime_watch_current)
    runtime_watch_history_path = resolve_json_path(args.runtime_watch_history)
    runtime_budget_baseline_path = resolve_json_path(args.runtime_budget_baseline)
    guardrail_policy_path = resolve_json_path(args.guardrail_policy)
    out_path = resolve_json_path(args.execute_out)
    if (
        runtime_current_manifest_path is None
        or runtime_watch_current_path is None
        or runtime_watch_history_path is None
        or runtime_budget_baseline_path is None
        or guardrail_policy_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-current-manifest, --runtime-watch-current, --runtime-watch-history, --runtime-budget-baseline, --guardrail-policy, and --execute-out are required")
    produced_gate_manifest = None
    artifact_dir = Path(args.artifact_dir).resolve() if getattr(args, "artifact_dir", None) else None
    if artifact_dir is not None:
        artifact_dir.mkdir(parents=True, exist_ok=True)
        produced_gate_path = artifact_dir / f"{args.action_id}_produced_reproposal_gate_manifest.json"
        gate = build_current_env_reproposal_trigger_gate(
            phase=str(args.phase),
            runtime_current_manifest=read_json(runtime_current_manifest_path),
            runtime_watch_current=read_json(runtime_watch_current_path),
            runtime_watch_history=read_json(runtime_watch_history_path),
            runtime_budget_baseline=read_json(runtime_budget_baseline_path),
            guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
            current_time_override=getattr(args, "current_time_override", None),
        )
        write_json(produced_gate_path, gate)
        produced_gate_manifest = str(produced_gate_path)
    payload = build_current_env_reproposal_gate_execute(
        phase=str(args.phase),
        runtime_current_manifest=read_json(runtime_current_manifest_path),
        runtime_watch_current=read_json(runtime_watch_current_path),
        runtime_watch_history=read_json(runtime_watch_history_path),
        runtime_budget_baseline=read_json(runtime_budget_baseline_path),
        guardrail_policy=load_current_env_governance_policy(guardrail_policy_path, phase=str(args.phase)),
        action_id=str(args.action_id),
        current_time_override=getattr(args, "current_time_override", None),
        produced_reproposal_gate_manifest=produced_gate_manifest,
    )
    payload["execute_manifest_path"] = str(out_path)
    payload["execute_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_action_ledger_update(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.ledger_out)
    agenda_path = resolve_json_path(args.runtime_current_env_agenda)
    if out_path is None or agenda_path is None:
        raise SystemExit("--runtime-current-env-agenda and --ledger-out are required")
    payload = build_current_env_action_ledger(
        phase=str(args.phase),
        runtime_current_env_agenda=read_json(agenda_path),
        watch_execute=read_json(resolve_json_path(getattr(args, "watch_execute", None))),
        watch_apply=read_json(resolve_json_path(getattr(args, "watch_apply", None))),
        reproposal_execute=read_json(resolve_json_path(getattr(args, "reproposal_execute", None))),
        ledger_in=read_json(resolve_json_path(getattr(args, "ledger_in", None))),
        current_time_override=getattr(args, "current_time_override", None),
    )
    write_json(out_path, payload)
    atomic_write_text(
        out_path.with_suffix(".summary.txt"),
        "\n".join(
            [
                f"manifest_version={payload.get('manifest_version')}",
                f"phase={payload.get('phase')}",
                f"total_action_count={payload.get('total_action_count')}",
                f"planned_count={payload.get('planned_count')}",
                f"executed_count={payload.get('executed_count')}",
                f"applied_count={payload.get('applied_count')}",
                f"failed_count={payload.get('failed_count')}",
                f"superseded_count={payload.get('superseded_count')}",
                f"latest_applied_action_id={payload.get('latest_applied_action_id')}",
            ]
        )
        + "\n",
    )
    print(str(out_path))
    return 0


def action_current_env_action_retry_plan(args: argparse.Namespace) -> int:
    ledger_path = resolve_json_path(args.action_ledger)
    out_path = resolve_json_path(args.retry_plan_out)
    if ledger_path is None or out_path is None:
        raise SystemExit("--action-ledger and --retry-plan-out are required")
    payload = build_current_env_action_retry_plan(
        phase=str(args.phase),
        action_ledger=read_json(ledger_path),
        retry_policy=load_current_env_action_retry_policy(resolve_json_path(args.retry_policy), phase=str(args.phase)),
        current_time_override=getattr(args, "current_time_override", None),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_reproposal_handoff(args: argparse.Namespace) -> int:
    reproposal_execute_path = resolve_json_path(args.reproposal_execute)
    runtime_budget_registry_path = resolve_json_path(args.runtime_budget_registry)
    runtime_current_manifest_path = resolve_json_path(args.runtime_current_manifest)
    runtime_budget_baseline_path = resolve_json_path(args.runtime_budget_baseline)
    out_path = resolve_json_path(args.handoff_out)
    if (
        reproposal_execute_path is None
        or runtime_budget_registry_path is None
        or runtime_current_manifest_path is None
        or runtime_budget_baseline_path is None
        or out_path is None
    ):
        raise SystemExit("--reproposal-execute, --runtime-budget-registry, --runtime-current-manifest, --runtime-budget-baseline, and --handoff-out are required")
    payload = build_current_env_reproposal_handoff(
        phase=str(args.phase),
        reproposal_execute=read_json(reproposal_execute_path),
        runtime_budget_registry=read_json(runtime_budget_registry_path),
        runtime_current_manifest=read_json(runtime_current_manifest_path),
        runtime_budget_baseline=read_json(runtime_budget_baseline_path),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_operator_decision(args: argparse.Namespace) -> int:
    ledger_path = resolve_json_path(args.action_ledger)
    out_path = resolve_json_path(args.decision_out)
    if ledger_path is None or out_path is None:
        raise SystemExit("--action-ledger and --decision-out are required")
    payload = build_current_env_operator_decision(
        phase=str(args.phase),
        action_ledger=read_json(ledger_path),
        handoff=read_json(resolve_json_path(getattr(args, "handoff", None))),
        retry_plan=read_json(resolve_json_path(getattr(args, "retry_plan", None))),
        action_id=str(args.action_id),
        decision=str(args.decision),
        decision_reason=str(args.decision_reason),
        decision_note=getattr(args, "decision_note", None),
        defer_until=getattr(args, "defer_until", None),
        operator_id=getattr(args, "operator_id", None),
        current_time_override=getattr(args, "current_time_override", None),
        approval_mode=str(getattr(args, "approval_mode", "handoff_only")),
    )
    payload["decision_manifest_path"] = str(out_path)
    payload["decision_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_apply_operator_decision(args: argparse.Namespace) -> int:
    ledger_path = resolve_json_path(args.action_ledger)
    decision_path = resolve_json_path(args.operator_decision)
    out_path = resolve_json_path(args.apply_out)
    if ledger_path is None or decision_path is None or out_path is None:
        raise SystemExit("--action-ledger, --operator-decision, and --apply-out are required")
    payload = build_current_env_apply_operator_decision(
        phase=str(args.phase),
        action_ledger=read_json(ledger_path),
        operator_decision=read_json(decision_path),
        runtime_current_manifest=read_json(resolve_json_path(getattr(args, "runtime_current_manifest", None))),
        runtime_budget_registry=read_json(resolve_json_path(getattr(args, "runtime_budget_registry", None))),
        approval_mode=str(getattr(args, "approval_mode", "handoff_only")),
        apply_manifest_path=str(out_path),
    )
    payload["decision_apply_manifest_path"] = str(out_path)
    payload["decision_apply_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    write_json(out_path, payload)
    updated_ledger_out = resolve_json_path(getattr(args, "updated_ledger_out", None))
    if updated_ledger_out is not None:
        write_json(updated_ledger_out, payload["updated_ledger"])
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_action_ledger_compact(args: argparse.Namespace) -> int:
    ledger_path = resolve_json_path(args.action_ledger)
    compact_out = resolve_json_path(args.compact_out)
    archive_out = resolve_json_path(args.archive_out)
    if ledger_path is None or compact_out is None or archive_out is None:
        raise SystemExit("--action-ledger, --compact-out, and --archive-out are required")
    compact, archive = build_current_env_action_ledger_compact(
        phase=str(args.phase),
        action_ledger=read_json(ledger_path),
        keep_latest_active=int(args.keep_latest_active),
        keep_latest_closed=int(args.keep_latest_closed),
        keep_failed=bool(args.keep_failed),
        keep_approval_actions=bool(args.keep_approval_actions),
    )
    write_json(compact_out, compact)
    write_json(archive_out, archive)
    atomic_write_text(compact_out.with_suffix(".summary.txt"), json.dumps(compact, indent=2) + "\n")
    atomic_write_text(archive_out.with_suffix(".summary.txt"), json.dumps(archive, indent=2) + "\n")
    print(str(compact_out))
    return 0


def action_current_env_approval_runbook(args: argparse.Namespace) -> int:
    handoff_path = resolve_json_path(args.handoff)
    decision_path = resolve_json_path(args.operator_decision)
    runtime_current_path = resolve_json_path(args.runtime_current_manifest)
    registry_path = resolve_json_path(args.runtime_budget_registry)
    out_path = resolve_json_path(args.approval_runbook_out)
    if handoff_path is None or decision_path is None or runtime_current_path is None or registry_path is None or out_path is None:
        raise SystemExit("--handoff, --operator-decision, --runtime-current-manifest, --runtime-budget-registry, and --approval-runbook-out are required")
    registry = read_json(registry_path)
    registry["registry_path"] = str(registry_path)
    payload = build_current_env_approval_runbook(
        phase=str(args.phase),
        handoff=read_json(handoff_path),
        operator_decision=read_json(decision_path),
        runtime_current_manifest=read_json(runtime_current_path),
        runtime_budget_registry=registry,
        runtime_budget_baseline_out=str(Path(args.runtime_budget_baseline_out).resolve()),
        budget_tag=str(args.budget_tag),
        approval_mode=str(args.approval_mode),
    )
    payload["runbook_manifest_path"] = str(out_path)
    payload["approval_runbook_manifest_path"] = str(out_path)
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_execute_budget_approval(args: argparse.Namespace) -> int:
    runbook_path = resolve_json_path(args.approval_runbook)
    current_path = resolve_json_path(args.runtime_budget_current)
    proposal_path = resolve_json_path(args.runtime_budget_proposal)
    gate_path = resolve_json_path(args.runtime_budget_proposal_gate)
    registry_path = resolve_json_path(args.runtime_budget_registry)
    out_path = resolve_json_path(args.approval_execution_out)
    if (
        runbook_path is None
        or current_path is None
        or proposal_path is None
        or gate_path is None
        or registry_path is None
        or out_path is None
    ):
        raise SystemExit("--approval-runbook, budget manifests, registry, and --approval-execution-out are required")
    dry_run_preflight_path = resolve_json_path(getattr(args, "dry_run_preflight", None))
    payload = build_current_env_execute_budget_approval(
        phase=str(args.phase),
        approval_runbook=read_json(runbook_path),
        runtime_budget_current=read_json(current_path),
        runtime_budget_proposal=read_json(proposal_path),
        runtime_budget_proposal_gate=read_json(gate_path),
        runtime_budget_registry=read_json(registry_path),
        runtime_budget_baseline_out=str(Path(args.runtime_budget_baseline_out).resolve()),
        archive_proposal=str(Path(args.archive_proposal).resolve()),
        approval_execution_mode=str(getattr(args, "approval_execution_mode", APPROVAL_EXECUTION_MODE_HANDOFF_ONLY)),
        integrated_opt_in=bool(getattr(args, "integrated_opt_in", False)),
        approval_confirmation_token=str(getattr(args, "approval_confirmation_token", "") or ""),
        dry_run_preflight=read_json(dry_run_preflight_path) if dry_run_preflight_path is not None and dry_run_preflight_path.exists() else None,
        require_preflight_success=bool(getattr(args, "require_preflight_success", False)),
    )
    payload["approval_execution_manifest_path"] = str(out_path)
    payload["approval_execution_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_link_approval_execution(args: argparse.Namespace) -> int:
    ledger_path = resolve_json_path(args.action_ledger)
    execution_path = resolve_json_path(args.approval_execution)
    ledger_out = resolve_json_path(args.ledger_out)
    link_out = resolve_json_path(args.link_out)
    if ledger_path is None or execution_path is None or ledger_out is None or link_out is None:
        raise SystemExit("--action-ledger, --approval-execution, --ledger-out, and --link-out are required")
    execution = read_json(execution_path)
    execution["approval_execution_manifest_path"] = str(execution_path)
    payload, linked_ledger = build_current_env_link_approval_execution(
        phase=str(args.phase),
        action_ledger=read_json(ledger_path),
        approval_execution=execution,
    )
    payload["approval_link_manifest_path"] = str(link_out)
    payload["approval_link_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    write_json(link_out, payload)
    write_json(ledger_out, linked_ledger)
    atomic_write_text(link_out.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(link_out))
    return 0


def action_operator_runbook_index(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.runbook_index_out)
    if out_path is None:
        raise SystemExit("--runbook-index-out is required")
    payload = build_operator_runbook_index(
        phase=str(args.phase),
        action_ledger=read_json(resolve_json_path(getattr(args, "action_ledger", None))),
        ops_agenda=read_json(resolve_json_path(getattr(args, "ops_agenda", None))),
        approval_runbook=read_json(resolve_json_path(getattr(args, "approval_runbook", None))),
        approval_execution=read_json(resolve_json_path(getattr(args, "approval_execution", None))),
        operator_decision=read_json(resolve_json_path(getattr(args, "operator_decision", None))),
        decision_apply=read_json(resolve_json_path(getattr(args, "decision_apply", None))),
    )
    payload["runbook_index_manifest_path"] = str(out_path)
    payload["runbook_index_hash"] = sha256_text(json.dumps(payload, sort_keys=True))
    write_json(out_path, payload)
    summary_lines = [
        f"manifest_version={payload.get('manifest_version')}",
        f"phase={payload.get('phase')}",
        f"runbook_count={payload.get('runbook_count', 0)}",
        f"pending_runbook_count={payload.get('pending_runbook_count', 0)}",
        f"executable_runbook_count={payload.get('executable_runbook_count', 0)}",
        f"integrated_opt_in_required_count={payload.get('integrated_opt_in_required_count', 0)}",
        f"approval_runbook_count={payload.get('approval_runbook_count', 0)}",
        f"retry_runbook_count={payload.get('retry_runbook_count', 0)}",
        f"empty_verdict={payload.get('empty_verdict')}",
    ]
    atomic_write_text(out_path.with_suffix(".summary.txt"), "\n".join(summary_lines) + "\n")
    print(str(out_path))
    return 0


def action_operator_runbook_catalog_update(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.runbook_catalog_out)
    if out_path is None:
        raise SystemExit("--runbook-catalog-out is required")
    payload = build_operator_runbook_catalog(
        phase=str(args.phase_tag),
        runbook_index=read_json(resolve_json_path(args.runbook_index)),
        catalog_in=read_json(resolve_json_path(getattr(args, "runbook_catalog_in", None))),
        current_time_override=getattr(args, "current_time_override", None),
    )
    write_json(out_path, payload)
    summary_lines = [
        f"manifest_version={payload.get('manifest_version')}",
        f"phase={payload.get('phase')}",
        f"catalog_entry_count={payload.get('catalog_entry_count', 0)}",
        f"active_runbook_count={payload.get('active_runbook_count', 0)}",
        f"resolved_runbook_count={payload.get('resolved_runbook_count', 0)}",
        f"integrated_opt_in_required_count={payload.get('integrated_opt_in_required_count', 0)}",
        f"replayable_runbook_count={payload.get('replayable_runbook_count', 0)}",
    ]
    atomic_write_text(out_path.with_suffix(".summary.txt"), "\n".join(summary_lines) + "\n")
    print(str(out_path))
    return 0


def action_operator_runbook_catalog_prune(args: argparse.Namespace) -> int:
    pruned_out = resolve_json_path(args.pruned_catalog_out)
    archive_out = resolve_json_path(args.archive_out)
    summary_out = resolve_json_path(args.prune_summary_out)
    if pruned_out is None or archive_out is None or summary_out is None:
        raise SystemExit("--pruned-catalog-out, --archive-out, and --prune-summary-out are required")
    policy_path = resolve_json_path(getattr(args, "retention_policy", None))
    raw_policy = read_json(policy_path)
    policy = normalize_operator_runbook_retention_policy(raw_policy, phase=str(args.phase))
    if policy_path is not None:
        write_json(policy_path, policy)
    pruned_catalog, archive, summary = build_operator_runbook_catalog_prune(
        phase=str(args.phase),
        runbook_catalog=read_json(resolve_json_path(args.runbook_catalog)),
        action_ledger=read_json(resolve_json_path(args.action_ledger)),
        retention_policy=policy,
    )
    write_json(pruned_out, pruned_catalog)
    write_json(archive_out, archive)
    write_json(summary_out, summary)
    atomic_write_text(summary_out.with_suffix(".summary.txt"), json.dumps(summary, indent=2) + "\n")
    print(str(summary_out))
    return 0


def action_operator_runbook_validate_lifecycle(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.validation_out)
    if out_path is None:
        raise SystemExit("--validation-out is required")
    payload = build_operator_runbook_lifecycle_validation(
        phase=str(args.phase),
        runbook_catalog=read_json(resolve_json_path(args.runbook_catalog)),
        action_ledger=read_json(resolve_json_path(args.action_ledger)),
        current_manifest_root=Path(args.current_manifest_root).resolve(),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_operator_runbook_pointer_audit(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.audit_out)
    if out_path is None:
        raise SystemExit("--audit-out is required")
    payload = build_operator_runbook_pointer_audit(
        phase=str(args.phase),
        runbook_catalog=read_json(resolve_json_path(args.runbook_catalog)),
        action_ledger=read_json(resolve_json_path(args.action_ledger)),
        published_root=Path(args.published_root).resolve(),
        artifact_root=Path(args.artifact_root).resolve(),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_operator_runbook_provenance_migrate(args: argparse.Namespace) -> int:
    migrated_catalog_out = resolve_json_path(args.migrated_catalog_out)
    migrated_ledger_out = resolve_json_path(args.migrated_ledger_out)
    migration_report_out = resolve_json_path(args.migration_report_out)
    if migrated_catalog_out is None or migrated_ledger_out is None or migration_report_out is None:
        raise SystemExit("--migrated-catalog-out, --migrated-ledger-out, and --migration-report-out are required")
    migrated_catalog, migrated_ledger, report = build_operator_runbook_provenance_migration(
        phase=str(args.phase),
        runbook_catalog=read_json(resolve_json_path(args.runbook_catalog)),
        action_ledger=read_json(resolve_json_path(args.action_ledger)),
        pointer_audit=read_json(resolve_json_path(args.pointer_audit)),
        allow_archived_waiver=bool(args.allow_archived_waiver),
    )
    write_json(migrated_catalog_out, migrated_catalog)
    write_json(migrated_ledger_out, migrated_ledger)
    write_json(migration_report_out, report)
    atomic_write_text(migration_report_out.with_suffix(".summary.txt"), json.dumps(report, indent=2) + "\n")
    print(str(migration_report_out))
    return 0


def action_operator_artifact_path_policy_lint(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.lint_out)
    if out_path is None:
        raise SystemExit("--lint-out is required")
    payload = build_operator_artifact_path_policy_lint(
        phase=str(args.phase),
        manifest_root=Path(args.manifest_root).resolve(),
        published_root=Path(args.published_root).resolve(),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_integrated_approval_mutation_audit(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.audit_out)
    if out_path is None:
        raise SystemExit("--audit-out is required")
    payload = build_integrated_approval_mutation_audit(
        phase=str(args.phase),
        approval_execution=read_json(resolve_json_path(args.approval_execution)),
        runtime_budget_registry=read_json(resolve_json_path(args.runtime_budget_registry)),
        runtime_budget_baseline=read_json(resolve_json_path(args.runtime_budget_baseline)),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_operator_decision_metadata_audit(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.audit_out)
    if out_path is None:
        raise SystemExit("--audit-out is required")
    payload = build_operator_decision_metadata_audit(
        phase=str(args.phase),
        action_ledger=read_json(resolve_json_path(args.action_ledger)),
        runbook_catalog=read_json(resolve_json_path(args.runbook_catalog)),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_operator_runbook_replay(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.replay_out)
    if out_path is None:
        raise SystemExit("--replay-out is required")
    payload = build_operator_runbook_replay(
        phase=str(args.phase),
        runbook=read_json(resolve_json_path(args.runbook)),
        action_ledger=read_json(resolve_json_path(getattr(args, "action_ledger", None))),
        runtime_current_manifest=read_json(resolve_json_path(getattr(args, "runtime_current_manifest", None))),
        runtime_budget_registry=read_json(resolve_json_path(getattr(args, "runtime_budget_registry", None))),
        replay_mode=str(args.replay_mode),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_source_health_plan(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.plan_out)
    if out_path is None:
        raise SystemExit("--plan-out is required")
    source_health = read_json(resolve_json_path(args.source_health))
    staged_materialization = read_json(resolve_json_path(getattr(args, "staged_materialization", None)))
    payload = build_source_health_action_plan(
        phase=str(args.phase),
        source_health=source_health,
        staged_materialization=staged_materialization,
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_staged_materialization_transaction(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.transaction_out)
    if out_path is None:
        raise SystemExit("--transaction-out is required")
    payload = build_staged_materialization_transaction(
        phase=str(args.phase),
        source_health=read_json(resolve_json_path(args.source_health_preflight)),
        staged_materialization=read_json(resolve_json_path(args.staged_materialization)),
        cleanup_path_values=list(args.cleanup_path or []),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_current_env_action_ledger_invariants(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.invariants_out)
    if out_path is None:
        raise SystemExit("--invariants-out is required")
    payload = build_action_ledger_closure_invariants(
        phase=str(args.phase),
        action_ledger=read_json(resolve_json_path(getattr(args, "action_ledger", None))),
        compacted_ledger=read_json(resolve_json_path(getattr(args, "compacted_ledger", None))),
        ledger_archive=read_json(resolve_json_path(getattr(args, "ledger_archive", None))),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".summary.txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_policy_ops_agenda(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.agenda_out)
    if out_path is None:
        raise SystemExit("--agenda-out is required")
    foreign_summaries = [read_json(resolve_json_path(path)) for path in getattr(args, "foreign_import_summary", [])]
    payload = build_policy_ops_agenda(
        phase=str(args.phase),
        current_env_due=read_json(resolve_json_path(getattr(args, "current_env_due", None))),
        current_env_watch_plan=read_json(resolve_json_path(getattr(args, "current_env_watch_plan", None))),
        current_env_reproposal_plan=read_json(resolve_json_path(getattr(args, "current_env_reproposal_plan", None))),
        runtime_registry_health=read_json(resolve_json_path(getattr(args, "runtime_registry_health", None))),
        known_env_reverify_plan=read_json(resolve_json_path(getattr(args, "known_env_reverify_plan", None))),
        known_env_retire_plan=read_json(resolve_json_path(getattr(args, "known_env_retire_plan", None))),
        foreign_import_summaries=foreign_summaries,
        publication_health=read_json(resolve_json_path(getattr(args, "publication_health", None))),
        current_env_watch_execute=read_json(resolve_json_path(getattr(args, "current_env_watch_execute", None))),
        current_env_watch_apply=read_json(resolve_json_path(getattr(args, "current_env_watch_apply", None))),
        current_env_reproposal_execute=read_json(resolve_json_path(getattr(args, "current_env_reproposal_execute", None))),
        current_env_action_ledger=read_json(resolve_json_path(getattr(args, "current_env_action_ledger", None))),
        current_env_retry_plan=read_json(resolve_json_path(getattr(args, "current_env_retry_plan", None))),
        current_env_reproposal_handoff=read_json(resolve_json_path(getattr(args, "current_env_reproposal_handoff", None))),
        current_env_operator_decision=read_json(resolve_json_path(getattr(args, "current_env_operator_decision", None))),
        current_env_operator_decision_apply=read_json(resolve_json_path(getattr(args, "current_env_operator_decision_apply", None))),
        current_env_action_ledger_compact=read_json(resolve_json_path(getattr(args, "current_env_action_ledger_compact", None))),
        current_env_action_ledger_archive=read_json(resolve_json_path(getattr(args, "current_env_action_ledger_archive", None))),
        current_env_approval_runbook=read_json(resolve_json_path(getattr(args, "current_env_approval_runbook", None))),
        current_env_approval_execution=read_json(resolve_json_path(getattr(args, "current_env_approval_execution", None))),
        current_env_approval_link=read_json(resolve_json_path(getattr(args, "current_env_approval_link", None))),
        current_time_override=getattr(args, "current_time_override", None),
    )
    write_json(out_path, payload)
    atomic_write_text(Path(args.out_text).resolve() if getattr(args, "out_text", None) else out_path.with_suffix(".txt"), build_policy_ops_agenda_text(payload))
    print(str(out_path))
    return 0


def action_runtime_budget_reproposal_history(args: argparse.Namespace) -> int:
    runtime_budget_baseline_path = resolve_json_path(args.runtime_budget_baseline)
    runtime_budget_registry_path = resolve_json_path(args.runtime_budget_registry)
    current_env_watch_history_path = resolve_json_path(args.current_env_watch_history)
    trigger_gate_path = resolve_json_path(args.current_env_trigger_gate)
    out_path = resolve_json_path(args.out)
    if (
        runtime_budget_baseline_path is None
        or runtime_budget_registry_path is None
        or current_env_watch_history_path is None
        or trigger_gate_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-budget-baseline, --runtime-budget-registry, --current-env-watch-history, --current-env-trigger-gate, and --out are required")
    payload = build_runtime_budget_reproposal_history(
        phase=str(args.phase),
        runtime_budget_baseline=read_json(runtime_budget_baseline_path),
        runtime_budget_registry=read_json(runtime_budget_registry_path),
        current_env_watch_history=read_json(current_env_watch_history_path),
        current_env_trigger_gate=read_json(trigger_gate_path),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_runtime_budget_registry_summary(args: argparse.Namespace) -> int:
    runtime_budget_registry_path = resolve_json_path(args.runtime_budget_registry)
    runtime_budget_baseline_path = resolve_json_path(args.runtime_budget_baseline)
    current_env_watch_history_path = resolve_json_path(args.current_env_watch_history)
    trigger_gate_path = resolve_json_path(args.current_env_trigger_gate)
    out_path = resolve_json_path(args.out)
    if (
        runtime_budget_registry_path is None
        or runtime_budget_baseline_path is None
        or current_env_watch_history_path is None
        or trigger_gate_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-budget-registry, --runtime-budget-baseline, --current-env-watch-history, --current-env-trigger-gate, and --out are required")
    payload = build_runtime_budget_registry_phase42_summary(
        phase=str(args.phase),
        runtime_budget_registry=read_json(runtime_budget_registry_path),
        runtime_budget_baseline=read_json(runtime_budget_baseline_path),
        current_env_trigger_gate=read_json(trigger_gate_path),
        current_env_watch_history=read_json(current_env_watch_history_path),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_known_env_governance_policy(args: argparse.Namespace) -> int:
    out_path = resolve_json_path(args.policy_out)
    if out_path is None:
        raise SystemExit("--policy-out is required")
    source_path = resolve_json_path(getattr(args, "policy_source", None))
    payload = load_known_env_governance_policy(source_path, phase=str(args.phase))
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_known_env_reverify_gate(args: argparse.Namespace) -> int:
    import_manifest_path = resolve_json_path(args.runtime_import_manifest)
    runtime_registry_path = resolve_json_path(args.runtime_baseline_registry)
    runtime_history_index_path = resolve_json_path(args.runtime_history_index)
    runtime_watch_registry_path = resolve_json_path(args.runtime_watch_registry)
    out_path = resolve_json_path(args.reverify_gate_out)
    if (
        import_manifest_path is None
        or runtime_registry_path is None
        or runtime_history_index_path is None
        or runtime_watch_registry_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-import-manifest, --runtime-baseline-registry, --runtime-history-index, --runtime-watch-registry, and --reverify-gate-out are required")
    known_entry = resolve_known_env_entry(runtime_gate.load_runtime_registry(runtime_registry_path), args.known_env_id)
    if known_entry is None:
        raise SystemExit("known environment id did not resolve to an active known environment entry")
    governance_policy = load_known_env_governance_policy(
        resolve_json_path(getattr(args, "governance_policy", None)),
        phase="phase40",
    ) if getattr(args, "governance_policy", None) else None
    payload = build_known_env_reverify_gate(
        import_manifest=read_json(import_manifest_path),
        import_manifest_path=import_manifest_path,
        known_entry=known_entry,
        history_index=read_json(runtime_history_index_path),
        watch_registry=read_json(runtime_watch_registry_path),
        governance_policy=governance_policy,
        min_real_samples=int(args.min_real_samples),
        max_age_days=int(args.max_age_days),
        current_time_override=getattr(args, "current_time_override", None),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_known_env_age_tick(args: argparse.Namespace) -> int:
    registry_path = resolve_json_path(args.runtime_baseline_registry)
    history_index_path = resolve_json_path(args.runtime_history_index)
    watch_registry_path = resolve_json_path(args.runtime_watch_registry)
    governance_policy_path = resolve_json_path(args.governance_policy)
    out_path = resolve_json_path(args.age_tick_out)
    if (
        registry_path is None
        or history_index_path is None
        or watch_registry_path is None
        or governance_policy_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-baseline-registry, --runtime-history-index, --runtime-watch-registry, --governance-policy, and --age-tick-out are required")
    payload = build_known_env_age_tick(
        registry=read_json(registry_path),
        runtime_history_index=read_json(history_index_path),
        runtime_watch_registry=read_json(watch_registry_path),
        governance_policy=load_known_env_governance_policy(governance_policy_path, phase="phase40"),
        current_time_override=getattr(args, "current_time_override", None),
        advance_days=float(getattr(args, "advance_days", 0.0) or 0.0),
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_known_env_plan_reverify(args: argparse.Namespace) -> int:
    registry_path = resolve_json_path(args.runtime_baseline_registry)
    history_index_path = resolve_json_path(args.runtime_history_index)
    watch_registry_path = resolve_json_path(args.runtime_watch_registry)
    governance_policy_path = resolve_json_path(args.governance_policy)
    out_path = resolve_json_path(args.plan_out)
    if (
        registry_path is None
        or history_index_path is None
        or watch_registry_path is None
        or governance_policy_path is None
        or out_path is None
    ):
        raise SystemExit("--runtime-baseline-registry, --runtime-history-index, --runtime-watch-registry, --governance-policy, and --plan-out are required")
    filter_values = str(getattr(args, "known_env_filter", "") or "").split(",")
    payload = build_known_env_reverify_plan(
        registry=read_json(registry_path),
        runtime_history_index=read_json(history_index_path),
        runtime_watch_registry=read_json(watch_registry_path),
        governance_policy=load_known_env_governance_policy(governance_policy_path, phase="phase40"),
        current_time_override=getattr(args, "current_time_override", None),
        known_env_filter=filter_values,
    )
    write_json(out_path, payload)
    atomic_write_text(out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(out_path))
    return 0


def action_known_env_apply_reverify(args: argparse.Namespace) -> int:
    import_manifest_path = resolve_json_path(args.runtime_import_manifest)
    runtime_registry_path = resolve_json_path(args.runtime_baseline_registry)
    runtime_history_index_path = resolve_json_path(args.runtime_history_index)
    runtime_watch_registry_path = resolve_json_path(args.runtime_watch_registry)
    governance_policy_path = resolve_json_path(args.governance_policy)
    gate_out_path = resolve_json_path(args.reverify_gate_out)
    apply_out_path = resolve_json_path(args.apply_out)
    archive_import_path = resolve_json_path(args.archive_import)
    if (
        import_manifest_path is None
        or runtime_registry_path is None
        or runtime_history_index_path is None
        or runtime_watch_registry_path is None
        or governance_policy_path is None
        or gate_out_path is None
        or apply_out_path is None
    ):
        raise SystemExit("--runtime-import-manifest, --runtime-baseline-registry, --runtime-history-index, --runtime-watch-registry, --governance-policy, --reverify-gate-out, and --apply-out are required")
    registry = runtime_gate.load_runtime_registry(runtime_registry_path)
    known_entry = resolve_known_env_entry(registry, args.known_env_id)
    if known_entry is None:
        raise SystemExit("known environment id did not resolve to an active known environment entry")
    governance_policy = load_known_env_governance_policy(governance_policy_path, phase="phase40")
    gate_payload = build_known_env_reverify_gate(
        import_manifest=read_json(import_manifest_path),
        import_manifest_path=import_manifest_path,
        known_entry=known_entry,
        history_index=read_json(runtime_history_index_path),
        watch_registry=read_json(runtime_watch_registry_path),
        governance_policy=governance_policy,
        min_real_samples=int(getattr(args, "min_real_samples", governance_policy.get("min_real_samples_for_reverify", 0))),
        max_age_days=int(getattr(args, "max_age_days", governance_policy.get("reverify_due_after_days", 0))),
        current_time_override=getattr(args, "current_time_override", None),
    )
    write_json(gate_out_path, gate_payload)
    atomic_write_text(gate_out_path.with_suffix(".txt"), json.dumps(gate_payload, indent=2) + "\n")
    payload = apply_known_env_reverify(
        registry_path=runtime_registry_path,
        import_manifest_path=import_manifest_path,
        known_entry=known_entry,
        gate_payload=gate_payload,
        governance_policy=governance_policy,
        archive_import_path=archive_import_path,
    )
    write_json(apply_out_path, payload)
    atomic_write_text(apply_out_path.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(apply_out_path))
    return 0


def action_known_env_plan_retire(args: argparse.Namespace) -> int:
    runtime_registry_path = resolve_json_path(args.runtime_baseline_registry)
    runtime_history_index_path = resolve_json_path(args.runtime_history_index)
    runtime_watch_registry_path = resolve_json_path(args.runtime_watch_registry)
    retire_plan_out = resolve_json_path(args.retire_plan_out)
    if runtime_registry_path is None or runtime_history_index_path is None or runtime_watch_registry_path is None or retire_plan_out is None:
        raise SystemExit("--runtime-baseline-registry, --runtime-history-index, --runtime-watch-registry, and --retire-plan-out are required")
    known_entry = resolve_known_env_entry(runtime_gate.load_runtime_registry(runtime_registry_path), args.known_env_id)
    if known_entry is None:
        raise SystemExit("known environment id did not resolve to an active known environment entry")
    governance_policy = load_known_env_governance_policy(
        resolve_json_path(getattr(args, "governance_policy", None)),
        phase=str(args.phase),
    ) if getattr(args, "governance_policy", None) else None
    payload = build_known_env_retire_plan(
        phase=str(args.phase),
        known_entry=known_entry,
        runtime_history_index=read_json(runtime_history_index_path),
        runtime_watch_registry=read_json(runtime_watch_registry_path),
        governance_policy=governance_policy,
        current_time_override=getattr(args, "current_time_override", None),
        retire_reason=str(args.retire_reason or "approved known environment exceeded retention policy"),
    )
    write_json(retire_plan_out, payload)
    atomic_write_text(retire_plan_out.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(retire_plan_out))
    return 0


def action_known_env_apply_retire(args: argparse.Namespace) -> int:
    runtime_registry_path = resolve_json_path(args.runtime_baseline_registry)
    archive_out = resolve_json_path(args.archive_out)
    apply_out = resolve_json_path(args.retire_apply_out)
    if runtime_registry_path is None or apply_out is None:
        raise SystemExit("--runtime-baseline-registry and --retire-apply-out are required")
    known_entry = resolve_known_env_entry(runtime_gate.load_runtime_registry(runtime_registry_path), args.known_env_id)
    if known_entry is None:
        raise SystemExit("known environment id did not resolve to an active known environment entry")
    payload = apply_known_env_retire(
        registry_path=runtime_registry_path,
        known_entry=known_entry,
        retire_reason=str(args.retire_reason or "approved known environment retired by operator action"),
        archive_out=archive_out,
    )
    write_json(apply_out, payload)
    atomic_write_text(apply_out.with_suffix(".txt"), json.dumps(payload, indent=2) + "\n")
    print(str(apply_out))
    return 0


def action_known_env_retire(args: argparse.Namespace) -> int:
    runtime_registry_path = resolve_json_path(args.runtime_baseline_registry)
    archive_out = resolve_json_path(args.archive_out)
    if runtime_registry_path is None:
        raise SystemExit("--runtime-baseline-registry is required")
    known_entry = resolve_known_env_entry(runtime_gate.load_runtime_registry(runtime_registry_path), args.known_env_id)
    if known_entry is None:
        raise SystemExit("known environment id did not resolve to an active known environment entry")
    payload = retire_known_env_entry(
        registry_path=runtime_registry_path,
        known_entry=known_entry,
        retire_reason=str(args.retire_reason or "approved known environment retired by operator action"),
        archive_out=archive_out,
    )
    print(str(archive_out or runtime_registry_path))
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build runtime watch registry and unified operator summaries.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    registry_health_parser = subparsers.add_parser("registry-health")
    registry_health_parser.add_argument("--phase", required=True)
    registry_health_parser.add_argument("--runtime-baseline-registry", required=True)
    registry_health_parser.add_argument("--runtime-history-index", required=True)
    registry_health_parser.add_argument("--runtime-watch-registry", required=True)
    registry_health_parser.add_argument("--runtime-refresh", required=True)
    registry_health_parser.add_argument("--approved-known-summary", action="append", default=[])
    registry_health_parser.add_argument("--foreign-import-summary", action="append", default=[])
    registry_health_parser.add_argument("--governance-policy", default=None)
    registry_health_parser.add_argument("--current-time-override", default=None)
    registry_health_parser.add_argument("--stale-after-hours", type=float, default=KNOWN_ENV_STALE_AFTER_HOURS)
    registry_health_parser.add_argument("--reverify-after-hours", type=float, default=KNOWN_ENV_REVERIFY_AFTER_HOURS)
    registry_health_parser.add_argument("--retire-after-hours", type=float, default=KNOWN_ENV_RETIRE_AFTER_HOURS)
    registry_health_parser.add_argument("--health-out", required=True)
    registry_health_parser.add_argument("--out-text", default=None)

    publication_health_parser = subparsers.add_parser("publication-health")
    publication_health_parser.add_argument("--phase", required=True)
    publication_health_parser.add_argument("--published-root", required=True)
    publication_health_parser.add_argument("--authoritative-root", required=True)
    publication_health_parser.add_argument("--health-out", required=True)
    publication_health_parser.add_argument("--out-text", default=None)
    publication_health_parser.add_argument("--expect-bundles", action="store_true")
    publication_health_parser.add_argument("--expect-manifests", action="store_true")
    publication_health_parser.add_argument("--expect-report", action="store_true")

    registry_parser = subparsers.add_parser("watch-registry")
    registry_parser.add_argument("--watch-current", default=None)
    registry_parser.add_argument("--watch-refresh", default=None)
    registry_parser.add_argument("--watch-history-index", default=None)
    registry_parser.add_argument("--matrix-summary", default=None)
    registry_parser.add_argument("--matrix-root", default=None)
    registry_parser.add_argument("--registry-out", required=True)
    registry_parser.add_argument("--summary-out", default=None)

    import_parser = subparsers.add_parser("import-current")
    import_parser.add_argument("--runtime-import-manifest", required=True)
    import_parser.add_argument("--runtime-baseline-registry", required=True)
    import_parser.add_argument("--runtime-history-index", required=True)
    import_parser.add_argument("--runtime-watch-registry", required=True)
    import_parser.add_argument("--runtime-refresh-out", default=None)
    import_parser.add_argument("--import-out", default=None)
    import_parser.add_argument("--evidence-source", default="real")
    import_parser.add_argument("--runner-id", default="")
    import_parser.add_argument("--host-label", default="")
    import_parser.add_argument("--merge-only", action="store_true")
    import_parser.add_argument("--refresh-after-import", action="store_true")

    import_bundle_parser = subparsers.add_parser("import-external-bundle")
    import_bundle_parser.add_argument("--runtime-import-bundle", required=True)
    import_bundle_parser.add_argument("--runtime-baseline-registry", required=True)
    import_bundle_parser.add_argument("--runtime-history-index", required=True)
    import_bundle_parser.add_argument("--runtime-watch-registry", required=True)
    import_bundle_parser.add_argument("--runtime-refresh-out", default=None)
    import_bundle_parser.add_argument("--import-out", default=None)
    import_bundle_parser.add_argument("--evidence-source", default=None)
    import_bundle_parser.add_argument("--runner-id", default="")
    import_bundle_parser.add_argument("--host-label", default="")
    import_bundle_parser.add_argument("--refresh-after-import", action="store_true")

    import_known_env_parser = subparsers.add_parser("import-known-env-evidence")
    import_known_env_parser.add_argument("--runtime-import-bundle", required=True)
    import_known_env_parser.add_argument("--runtime-baseline-registry", required=True)
    import_known_env_parser.add_argument("--runtime-history-index", required=True)
    import_known_env_parser.add_argument("--runtime-watch-registry", required=True)
    import_known_env_parser.add_argument("--runtime-refresh-out", default=None)
    import_known_env_parser.add_argument("--import-out", default=None)
    import_known_env_parser.add_argument("--known-env-id", default="")
    import_known_env_parser.add_argument("--runner-id", default="")
    import_known_env_parser.add_argument("--host-label", default="")
    import_known_env_parser.add_argument("--refresh-after-import", action="store_true")

    current_env_policy_parser = subparsers.add_parser("current-env-governance-policy")
    current_env_policy_parser.add_argument("--phase", required=True)
    current_env_policy_parser.add_argument("--policy-source", default=None)
    current_env_policy_parser.add_argument("--policy-out", required=True)

    current_env_guardrail_policy_parser = subparsers.add_parser("current-env-guardrail-policy")
    current_env_guardrail_policy_parser.add_argument("--phase", required=True)
    current_env_guardrail_policy_parser.add_argument("--policy-source", default=None)
    current_env_guardrail_policy_parser.add_argument("--policy-out", required=True)

    current_env_watch_current_parser = subparsers.add_parser("current-env-watch-current")
    current_env_watch_current_parser.add_argument("--phase", required=True)
    current_env_watch_current_parser.add_argument("--runtime-refresh", required=True)
    current_env_watch_current_parser.add_argument("--runtime-budget-current", required=True)
    current_env_watch_current_parser.add_argument("--runtime-watch-current", required=True)
    current_env_watch_current_parser.add_argument("--governance-policy", required=True)
    current_env_watch_current_parser.add_argument("--out", required=True)
    current_env_watch_current_parser.add_argument("--out-text", default=None)

    current_env_watch_refresh_parser = subparsers.add_parser("current-env-watch-refresh")
    current_env_watch_refresh_parser.add_argument("--phase", required=True)
    current_env_watch_refresh_parser.add_argument("--runtime-refresh", required=True)
    current_env_watch_refresh_parser.add_argument("--runtime-budget-current", required=True)
    current_env_watch_refresh_parser.add_argument("--runtime-watch-refresh", required=True)
    current_env_watch_refresh_parser.add_argument("--runtime-budget-proposal", default=None)
    current_env_watch_refresh_parser.add_argument("--runtime-budget-proposal-gate", default=None)
    current_env_watch_refresh_parser.add_argument("--runtime-budget-baseline", default=None)
    current_env_watch_refresh_parser.add_argument("--governance-policy", required=True)
    current_env_watch_refresh_parser.add_argument("--out", required=True)
    current_env_watch_refresh_parser.add_argument("--out-text", default=None)

    current_env_watch_history_parser = subparsers.add_parser("current-env-watch-history")
    current_env_watch_history_parser.add_argument("--phase", required=True)
    current_env_watch_history_parser.add_argument("--runtime-watch-history-index", required=True)
    current_env_watch_history_parser.add_argument("--runtime-budget-current", required=True)
    current_env_watch_history_parser.add_argument("--runtime-budget-proposal-gate", default=None)
    current_env_watch_history_parser.add_argument("--runtime-budget-baseline", default=None)
    current_env_watch_history_parser.add_argument("--governance-policy", required=True)
    current_env_watch_history_parser.add_argument("--out", required=True)
    current_env_watch_history_parser.add_argument("--out-text", default=None)

    current_env_age_tick_parser = subparsers.add_parser("current-env-age-tick")
    current_env_age_tick_parser.add_argument("--phase", required=True)
    current_env_age_tick_parser.add_argument("--runtime-current-manifest", required=True)
    current_env_age_tick_parser.add_argument("--runtime-watch-current", required=True)
    current_env_age_tick_parser.add_argument("--runtime-watch-refresh", required=True)
    current_env_age_tick_parser.add_argument("--runtime-watch-history", required=True)
    current_env_age_tick_parser.add_argument("--runtime-budget-baseline", required=True)
    current_env_age_tick_parser.add_argument("--guardrail-policy", required=True)
    current_env_age_tick_parser.add_argument("--current-time-override", default=None)
    current_env_age_tick_parser.add_argument("--advance-days", type=float, default=0.0)
    current_env_age_tick_parser.add_argument("--age-tick-out", required=True)

    current_env_plan_watch_parser = subparsers.add_parser("current-env-plan-watch")
    current_env_plan_watch_parser.add_argument("--phase", required=True)
    current_env_plan_watch_parser.add_argument("--runtime-current-manifest", required=True)
    current_env_plan_watch_parser.add_argument("--runtime-watch-history", required=True)
    current_env_plan_watch_parser.add_argument("--guardrail-policy", required=True)
    current_env_plan_watch_parser.add_argument("--current-env-age-tick", default=None)
    current_env_plan_watch_parser.add_argument("--runtime-budget-baseline", default=None)
    current_env_plan_watch_parser.add_argument("--runtime-watch-current", default=None)
    current_env_plan_watch_parser.add_argument("--runtime-watch-refresh", default=None)
    current_env_plan_watch_parser.add_argument("--plan-out", required=True)
    current_env_plan_watch_parser.add_argument("--execution-class-filter", default="")
    current_env_plan_watch_parser.add_argument("--current-time-override", default=None)

    current_env_trigger_gate_parser = subparsers.add_parser("current-env-reproposal-trigger-gate")
    current_env_trigger_gate_parser.add_argument("--phase", required=True)
    current_env_trigger_gate_parser.add_argument("--runtime-current-manifest", required=True)
    current_env_trigger_gate_parser.add_argument("--runtime-watch-current", required=True)
    current_env_trigger_gate_parser.add_argument("--runtime-watch-history", required=True)
    current_env_trigger_gate_parser.add_argument("--runtime-budget-baseline", required=True)
    current_env_trigger_gate_parser.add_argument("--guardrail-policy", required=True)
    current_env_trigger_gate_parser.add_argument("--current-time-override", default=None)
    current_env_trigger_gate_parser.add_argument("--trigger-gate-out", required=True)

    current_env_due_parser = subparsers.add_parser("current-env-due-scheduler")
    current_env_due_parser.add_argument("--phase", required=True)
    current_env_due_parser.add_argument("--runtime-current-manifest", required=True)
    current_env_due_parser.add_argument("--runtime-current-env-watch", required=True)
    current_env_due_parser.add_argument("--runtime-current-env-watch-refresh", required=True)
    current_env_due_parser.add_argument("--runtime-watch-history", required=True)
    current_env_due_parser.add_argument("--runtime-budget-baseline", required=True)
    current_env_due_parser.add_argument("--guardrail-policy", required=True)
    current_env_due_parser.add_argument("--runtime-current-env-watch-apply", default=None)
    current_env_due_parser.add_argument("--current-time-override", default=None)
    current_env_due_parser.add_argument("--due-scheduler-out", required=True)

    current_env_plan_reproposal_parser = subparsers.add_parser("current-env-plan-reproposal")
    current_env_plan_reproposal_parser.add_argument("--phase", required=True)
    current_env_plan_reproposal_parser.add_argument("--runtime-current-manifest", required=True)
    current_env_plan_reproposal_parser.add_argument("--runtime-watch-history", required=True)
    current_env_plan_reproposal_parser.add_argument("--guardrail-policy", required=True)
    current_env_plan_reproposal_parser.add_argument("--current-env-due", default=None)
    current_env_plan_reproposal_parser.add_argument("--plan-out", required=True)
    current_env_plan_reproposal_parser.add_argument("--execution-class-filter", default="")
    current_env_plan_reproposal_parser.add_argument("--current-time-override", default=None)

    current_env_execute_watch_parser = subparsers.add_parser("current-env-execute-watch")
    current_env_execute_watch_parser.add_argument("--phase", required=True)
    current_env_execute_watch_parser.add_argument("--runtime-current-manifest", required=True)
    current_env_execute_watch_parser.add_argument("--runtime-baseline-manifest", required=True)
    current_env_execute_watch_parser.add_argument("--runtime-current-env-watch", required=True)
    current_env_execute_watch_parser.add_argument("--runtime-watch-history", required=True)
    current_env_execute_watch_parser.add_argument("--guardrail-policy", required=True)
    current_env_execute_watch_parser.add_argument("--execution-class", required=True)
    current_env_execute_watch_parser.add_argument("--repeat", type=int, required=True)
    current_env_execute_watch_parser.add_argument("--action-id", required=True)
    current_env_execute_watch_parser.add_argument("--execute-out", required=True)
    current_env_execute_watch_parser.add_argument("--artifact-dir", default=None)

    current_env_apply_watch_parser = subparsers.add_parser("current-env-apply-watch")
    current_env_apply_watch_parser.add_argument("--phase", required=True)
    current_env_apply_watch_parser.add_argument("--runtime-current-manifest", required=True)
    current_env_apply_watch_parser.add_argument("--runtime-current-env-watch", required=True)
    current_env_apply_watch_parser.add_argument("--runtime-watch-execute", required=True)
    current_env_apply_watch_parser.add_argument("--runtime-watch-history", required=True)
    current_env_apply_watch_parser.add_argument("--guardrail-policy", required=True)
    current_env_apply_watch_parser.add_argument("--current-time-override", default=None)
    current_env_apply_watch_parser.add_argument("--apply-out", required=True)

    current_env_execute_reproposal_parser = subparsers.add_parser("current-env-execute-reproposal-gate")
    current_env_execute_reproposal_parser.add_argument("--phase", required=True)
    current_env_execute_reproposal_parser.add_argument("--runtime-current-manifest", required=True)
    current_env_execute_reproposal_parser.add_argument("--runtime-watch-current", required=True)
    current_env_execute_reproposal_parser.add_argument("--runtime-watch-history", required=True)
    current_env_execute_reproposal_parser.add_argument("--runtime-budget-baseline", required=True)
    current_env_execute_reproposal_parser.add_argument("--guardrail-policy", required=True)
    current_env_execute_reproposal_parser.add_argument("--action-id", required=True)
    current_env_execute_reproposal_parser.add_argument("--current-time-override", default=None)
    current_env_execute_reproposal_parser.add_argument("--execute-out", required=True)
    current_env_execute_reproposal_parser.add_argument("--artifact-dir", default=None)

    action_ledger_parser = subparsers.add_parser("current-env-action-ledger-update")
    action_ledger_parser.add_argument("--phase", required=True)
    action_ledger_parser.add_argument("--runtime-current-env-agenda", required=True)
    action_ledger_parser.add_argument("--watch-execute", default=None)
    action_ledger_parser.add_argument("--watch-apply", default=None)
    action_ledger_parser.add_argument("--reproposal-execute", default=None)
    action_ledger_parser.add_argument("--ledger-in", default=None)
    action_ledger_parser.add_argument("--current-time-override", default=None)
    action_ledger_parser.add_argument("--ledger-out", required=True)

    action_retry_parser = subparsers.add_parser("current-env-action-retry-plan")
    action_retry_parser.add_argument("--phase", required=True)
    action_retry_parser.add_argument("--action-ledger", required=True)
    action_retry_parser.add_argument("--retry-policy", default=None)
    action_retry_parser.add_argument("--current-time-override", default=None)
    action_retry_parser.add_argument("--retry-plan-out", required=True)

    reproposal_handoff_parser = subparsers.add_parser("current-env-reproposal-handoff")
    reproposal_handoff_parser.add_argument("--phase", required=True)
    reproposal_handoff_parser.add_argument("--reproposal-execute", required=True)
    reproposal_handoff_parser.add_argument("--runtime-budget-registry", required=True)
    reproposal_handoff_parser.add_argument("--runtime-current-manifest", required=True)
    reproposal_handoff_parser.add_argument("--runtime-budget-baseline", required=True)
    reproposal_handoff_parser.add_argument("--handoff-out", required=True)

    operator_decision_parser = subparsers.add_parser("current-env-operator-decision")
    operator_decision_parser.add_argument("--phase", required=True)
    operator_decision_parser.add_argument("--action-ledger", required=True)
    operator_decision_parser.add_argument("--handoff", default=None)
    operator_decision_parser.add_argument("--retry-plan", default=None)
    operator_decision_parser.add_argument("--action-id", required=True)
    operator_decision_parser.add_argument("--decision", required=True, choices=[
        OPERATOR_DECISION_APPROVE,
        OPERATOR_DECISION_SKIP,
        OPERATOR_DECISION_DEFER,
        OPERATOR_DECISION_REJECT,
        OPERATOR_DECISION_CLOSE,
        OPERATOR_DECISION_RETRY_NOW,
    ])
    operator_decision_parser.add_argument("--decision-reason", required=True)
    operator_decision_parser.add_argument("--decision-note", default="")
    operator_decision_parser.add_argument("--defer-until", default=None)
    operator_decision_parser.add_argument("--operator-id", default="local-operator")
    operator_decision_parser.add_argument("--approval-mode", default="handoff_only", choices=["handoff_only", "integrated"])
    operator_decision_parser.add_argument("--current-time-override", default=None)
    operator_decision_parser.add_argument("--decision-out", required=True)

    operator_decision_apply_parser = subparsers.add_parser("current-env-apply-operator-decision")
    operator_decision_apply_parser.add_argument("--phase", required=True)
    operator_decision_apply_parser.add_argument("--action-ledger", required=True)
    operator_decision_apply_parser.add_argument("--operator-decision", required=True)
    operator_decision_apply_parser.add_argument("--runtime-current-manifest", default=None)
    operator_decision_apply_parser.add_argument("--runtime-budget-registry", default=None)
    operator_decision_apply_parser.add_argument("--runtime-budget-baseline-out", default=None)
    operator_decision_apply_parser.add_argument("--approval-mode", default="handoff_only", choices=["handoff_only", "integrated"])
    operator_decision_apply_parser.add_argument("--apply-out", required=True)
    operator_decision_apply_parser.add_argument("--updated-ledger-out", default=None)

    action_ledger_compact_parser = subparsers.add_parser("current-env-action-ledger-compact")
    action_ledger_compact_parser.add_argument("--phase", required=True)
    action_ledger_compact_parser.add_argument("--action-ledger", required=True)
    action_ledger_compact_parser.add_argument("--compact-out", required=True)
    action_ledger_compact_parser.add_argument("--archive-out", required=True)
    action_ledger_compact_parser.add_argument("--keep-latest-active", type=int, default=5)
    action_ledger_compact_parser.add_argument("--keep-latest-closed", type=int, default=5)
    action_ledger_compact_parser.add_argument("--keep-failed", action="store_true")
    action_ledger_compact_parser.add_argument("--keep-approval-actions", action="store_true")

    approval_runbook_parser = subparsers.add_parser("current-env-approval-runbook")
    approval_runbook_parser.add_argument("--phase", required=True)
    approval_runbook_parser.add_argument("--handoff", required=True)
    approval_runbook_parser.add_argument("--operator-decision", required=True)
    approval_runbook_parser.add_argument("--runtime-current-manifest", required=True)
    approval_runbook_parser.add_argument("--runtime-budget-registry", required=True)
    approval_runbook_parser.add_argument("--runtime-budget-baseline-out", required=True)
    approval_runbook_parser.add_argument("--approval-runbook-out", required=True)
    approval_runbook_parser.add_argument("--budget-tag", required=True)
    approval_runbook_parser.add_argument("--approval-mode", default="handoff_only", choices=["handoff_only", "integrated"])

    approval_execute_parser = subparsers.add_parser("current-env-execute-budget-approval")
    approval_execute_parser.add_argument("--phase", required=True)
    approval_execute_parser.add_argument("--approval-runbook", required=True)
    approval_execute_parser.add_argument("--runtime-budget-current", required=True)
    approval_execute_parser.add_argument("--runtime-budget-proposal", required=True)
    approval_execute_parser.add_argument("--runtime-budget-proposal-gate", required=True)
    approval_execute_parser.add_argument("--runtime-budget-registry", required=True)
    approval_execute_parser.add_argument("--runtime-budget-baseline-out", required=True)
    approval_execute_parser.add_argument("--archive-proposal", required=True)
    approval_execute_parser.add_argument("--approval-execution-out", required=True)
    approval_execute_parser.add_argument(
        "--approval-execution-mode",
        default=APPROVAL_EXECUTION_MODE_HANDOFF_ONLY,
        choices=[
            APPROVAL_EXECUTION_MODE_DRY_RUN,
            APPROVAL_EXECUTION_MODE_HANDOFF_ONLY,
            APPROVAL_EXECUTION_MODE_INTEGRATED_OPT_IN,
        ],
    )
    approval_execute_parser.add_argument("--integrated-opt-in", action="store_true")
    approval_execute_parser.add_argument("--allow-integrated-approval", dest="integrated_opt_in", action="store_true")
    approval_execute_parser.add_argument("--approval-confirmation-token", default="")
    approval_execute_parser.add_argument("--dry-run-preflight", default=None)
    approval_execute_parser.add_argument("--require-preflight-success", action="store_true")

    approval_link_parser = subparsers.add_parser("current-env-link-approval-execution")
    approval_link_parser.add_argument("--phase", required=True)
    approval_link_parser.add_argument("--action-ledger", required=True)
    approval_link_parser.add_argument("--approval-execution", required=True)
    approval_link_parser.add_argument("--ledger-out", required=True)
    approval_link_parser.add_argument("--link-out", required=True)

    runbook_index_parser = subparsers.add_parser("operator-runbook-index")
    runbook_index_parser.add_argument("--phase", required=True)
    runbook_index_parser.add_argument("--action-ledger", default=None)
    runbook_index_parser.add_argument("--ops-agenda", default=None)
    runbook_index_parser.add_argument("--approval-runbook", default=None)
    runbook_index_parser.add_argument("--approval-execution", default=None)
    runbook_index_parser.add_argument("--operator-decision", default=None)
    runbook_index_parser.add_argument("--decision-apply", default=None)
    runbook_index_parser.add_argument("--runbook-index-out", required=True)

    runbook_catalog_parser = subparsers.add_parser("operator-runbook-catalog-update")
    runbook_catalog_parser.add_argument("--runbook-index", required=True)
    runbook_catalog_parser.add_argument("--runbook-catalog-in", default=None)
    runbook_catalog_parser.add_argument("--runbook-catalog-out", required=True)
    runbook_catalog_parser.add_argument("--phase-tag", required=True)
    runbook_catalog_parser.add_argument("--current-time-override", default=None)

    runbook_prune_parser = subparsers.add_parser("operator-runbook-catalog-prune")
    runbook_prune_parser.add_argument("--phase", required=True)
    runbook_prune_parser.add_argument("--runbook-catalog", required=True)
    runbook_prune_parser.add_argument("--action-ledger", required=True)
    runbook_prune_parser.add_argument("--retention-policy", required=True)
    runbook_prune_parser.add_argument("--pruned-catalog-out", required=True)
    runbook_prune_parser.add_argument("--archive-out", required=True)
    runbook_prune_parser.add_argument("--prune-summary-out", required=True)

    runbook_lifecycle_parser = subparsers.add_parser("operator-runbook-validate-lifecycle")
    runbook_lifecycle_parser.add_argument("--phase", required=True)
    runbook_lifecycle_parser.add_argument("--runbook-catalog", required=True)
    runbook_lifecycle_parser.add_argument("--action-ledger", required=True)
    runbook_lifecycle_parser.add_argument("--current-manifest-root", required=True)
    runbook_lifecycle_parser.add_argument("--validation-out", required=True)

    pointer_audit_parser = subparsers.add_parser("operator-runbook-pointer-audit")
    pointer_audit_parser.add_argument("--phase", required=True)
    pointer_audit_parser.add_argument("--runbook-catalog", required=True)
    pointer_audit_parser.add_argument("--action-ledger", required=True)
    pointer_audit_parser.add_argument("--published-root", required=True)
    pointer_audit_parser.add_argument("--artifact-root", required=True)
    pointer_audit_parser.add_argument("--audit-out", required=True)

    provenance_migrate_parser = subparsers.add_parser("operator-runbook-provenance-migrate")
    provenance_migrate_parser.add_argument("--phase", required=True)
    provenance_migrate_parser.add_argument("--runbook-catalog", required=True)
    provenance_migrate_parser.add_argument("--action-ledger", required=True)
    provenance_migrate_parser.add_argument("--pointer-audit", required=True)
    provenance_migrate_parser.add_argument("--published-root", required=True)
    provenance_migrate_parser.add_argument("--migrated-catalog-out", required=True)
    provenance_migrate_parser.add_argument("--migrated-ledger-out", required=True)
    provenance_migrate_parser.add_argument("--migration-report-out", required=True)
    provenance_migrate_parser.add_argument("--allow-archived-waiver", action="store_true")

    path_policy_lint_parser = subparsers.add_parser("operator-artifact-path-policy-lint")
    path_policy_lint_parser.add_argument("--phase", required=True)
    path_policy_lint_parser.add_argument("--manifest-root", required=True)
    path_policy_lint_parser.add_argument("--published-root", required=True)
    path_policy_lint_parser.add_argument("--lint-out", required=True)

    decision_metadata_audit_parser = subparsers.add_parser("operator-decision-metadata-audit")
    decision_metadata_audit_parser.add_argument("--phase", required=True)
    decision_metadata_audit_parser.add_argument("--action-ledger", required=True)
    decision_metadata_audit_parser.add_argument("--runbook-catalog", required=True)
    decision_metadata_audit_parser.add_argument("--audit-out", required=True)

    runbook_replay_parser = subparsers.add_parser("operator-runbook-replay")
    runbook_replay_parser.add_argument("--phase", required=True)
    runbook_replay_parser.add_argument("--runbook", required=True)
    runbook_replay_parser.add_argument("--action-ledger", default=None)
    runbook_replay_parser.add_argument("--runtime-current-manifest", default=None)
    runbook_replay_parser.add_argument("--runtime-budget-registry", default=None)
    runbook_replay_parser.add_argument("--replay-mode", default="dry_run", choices=["dry_run", "validate_only"])
    runbook_replay_parser.add_argument("--replay-out", required=True)

    mutation_audit_parser = subparsers.add_parser("integrated-approval-mutation-audit")
    mutation_audit_parser.add_argument("--phase", required=True)
    mutation_audit_parser.add_argument("--approval-execution", required=True)
    mutation_audit_parser.add_argument("--runtime-budget-registry", required=True)
    mutation_audit_parser.add_argument("--runtime-budget-baseline", required=True)
    mutation_audit_parser.add_argument("--audit-out", required=True)

    source_health_plan_parser = subparsers.add_parser("source-health-plan")
    source_health_plan_parser.add_argument("--phase", required=True)
    source_health_plan_parser.add_argument("--source-health", required=True)
    source_health_plan_parser.add_argument("--staged-materialization", default=None)
    source_health_plan_parser.add_argument("--plan-out", required=True)

    materialization_tx_parser = subparsers.add_parser("staged-materialization-transaction")
    materialization_tx_parser.add_argument("--phase", required=True)
    materialization_tx_parser.add_argument("--source-health-preflight", required=True)
    materialization_tx_parser.add_argument("--staged-materialization", required=True)
    materialization_tx_parser.add_argument("--transaction-out", required=True)
    materialization_tx_parser.add_argument("--cleanup-path", action="append", default=[])

    ledger_invariants_parser = subparsers.add_parser("current-env-action-ledger-invariants")
    ledger_invariants_parser.add_argument("--phase", required=True)
    ledger_invariants_parser.add_argument("--action-ledger", required=True)
    ledger_invariants_parser.add_argument("--compacted-ledger", default=None)
    ledger_invariants_parser.add_argument("--ledger-archive", default=None)
    ledger_invariants_parser.add_argument("--invariants-out", required=True)

    ops_agenda_parser = subparsers.add_parser("ops-agenda")
    ops_agenda_parser.add_argument("--phase", required=True)
    ops_agenda_parser.add_argument("--current-env-due", default=None)
    ops_agenda_parser.add_argument("--current-env-watch-plan", default=None)
    ops_agenda_parser.add_argument("--current-env-reproposal-plan", default=None)
    ops_agenda_parser.add_argument("--current-env-watch-execute", default=None)
    ops_agenda_parser.add_argument("--current-env-watch-apply", default=None)
    ops_agenda_parser.add_argument("--current-env-reproposal-execute", default=None)
    ops_agenda_parser.add_argument("--current-env-action-ledger", default=None)
    ops_agenda_parser.add_argument("--current-env-retry-plan", default=None)
    ops_agenda_parser.add_argument("--current-env-reproposal-handoff", default=None)
    ops_agenda_parser.add_argument("--current-env-operator-decision", default=None)
    ops_agenda_parser.add_argument("--current-env-operator-decision-apply", default=None)
    ops_agenda_parser.add_argument("--current-env-action-ledger-compact", default=None)
    ops_agenda_parser.add_argument("--current-env-action-ledger-archive", default=None)
    ops_agenda_parser.add_argument("--current-env-approval-runbook", default=None)
    ops_agenda_parser.add_argument("--current-env-approval-execution", default=None)
    ops_agenda_parser.add_argument("--current-env-approval-link", default=None)
    ops_agenda_parser.add_argument("--runtime-registry-health", default=None)
    ops_agenda_parser.add_argument("--known-env-reverify-plan", default=None)
    ops_agenda_parser.add_argument("--known-env-retire-plan", default=None)
    ops_agenda_parser.add_argument("--foreign-import-summary", action="append", default=[])
    ops_agenda_parser.add_argument("--publication-health", default=None)
    ops_agenda_parser.add_argument("--current-time-override", default=None)
    ops_agenda_parser.add_argument("--agenda-out", required=True)
    ops_agenda_parser.add_argument("--out-text", default=None)

    runtime_budget_reproposal_history_parser = subparsers.add_parser("runtime-budget-reproposal-history")
    runtime_budget_reproposal_history_parser.add_argument("--phase", required=True)
    runtime_budget_reproposal_history_parser.add_argument("--runtime-budget-baseline", required=True)
    runtime_budget_reproposal_history_parser.add_argument("--runtime-budget-registry", required=True)
    runtime_budget_reproposal_history_parser.add_argument("--current-env-watch-history", required=True)
    runtime_budget_reproposal_history_parser.add_argument("--current-env-trigger-gate", required=True)
    runtime_budget_reproposal_history_parser.add_argument("--out", required=True)

    runtime_budget_registry_summary_parser = subparsers.add_parser("runtime-budget-registry-summary")
    runtime_budget_registry_summary_parser.add_argument("--phase", required=True)
    runtime_budget_registry_summary_parser.add_argument("--runtime-budget-registry", required=True)
    runtime_budget_registry_summary_parser.add_argument("--runtime-budget-baseline", required=True)
    runtime_budget_registry_summary_parser.add_argument("--current-env-watch-history", required=True)
    runtime_budget_registry_summary_parser.add_argument("--current-env-trigger-gate", required=True)
    runtime_budget_registry_summary_parser.add_argument("--out", required=True)

    governance_policy_parser = subparsers.add_parser("known-env-governance-policy")
    governance_policy_parser.add_argument("--phase", required=True)
    governance_policy_parser.add_argument("--policy-source", default=None)
    governance_policy_parser.add_argument("--policy-out", required=True)

    age_tick_parser = subparsers.add_parser("known-env-age-tick")
    age_tick_parser.add_argument("--runtime-baseline-registry", required=True)
    age_tick_parser.add_argument("--runtime-history-index", required=True)
    age_tick_parser.add_argument("--runtime-watch-registry", required=True)
    age_tick_parser.add_argument("--governance-policy", required=True)
    age_tick_parser.add_argument("--current-time-override", default=None)
    age_tick_parser.add_argument("--advance-days", type=float, default=0.0)
    age_tick_parser.add_argument("--age-tick-out", required=True)

    reverify_plan_parser = subparsers.add_parser("known-env-plan-reverify")
    reverify_plan_parser.add_argument("--runtime-baseline-registry", required=True)
    reverify_plan_parser.add_argument("--runtime-history-index", required=True)
    reverify_plan_parser.add_argument("--runtime-watch-registry", required=True)
    reverify_plan_parser.add_argument("--governance-policy", required=True)
    reverify_plan_parser.add_argument("--plan-out", required=True)
    reverify_plan_parser.add_argument("--known-env-filter", default="")
    reverify_plan_parser.add_argument("--current-time-override", default=None)

    reverify_parser = subparsers.add_parser("known-env-reverify-gate")
    reverify_parser.add_argument("--runtime-import-manifest", required=True)
    reverify_parser.add_argument("--runtime-baseline-registry", required=True)
    reverify_parser.add_argument("--runtime-history-index", required=True)
    reverify_parser.add_argument("--runtime-watch-registry", required=True)
    reverify_parser.add_argument("--reverify-gate-out", required=True)
    reverify_parser.add_argument("--known-env-id", default="")
    reverify_parser.add_argument("--min-real-samples", type=int, default=3)
    reverify_parser.add_argument("--max-age-days", type=int, default=30)
    reverify_parser.add_argument("--governance-policy", default=None)
    reverify_parser.add_argument("--current-time-override", default=None)

    apply_reverify_parser = subparsers.add_parser("known-env-apply-reverify")
    apply_reverify_parser.add_argument("--runtime-import-manifest", required=True)
    apply_reverify_parser.add_argument("--runtime-baseline-registry", required=True)
    apply_reverify_parser.add_argument("--runtime-history-index", required=True)
    apply_reverify_parser.add_argument("--runtime-watch-registry", required=True)
    apply_reverify_parser.add_argument("--governance-policy", required=True)
    apply_reverify_parser.add_argument("--known-env-id", default="")
    apply_reverify_parser.add_argument("--reverify-gate-out", required=True)
    apply_reverify_parser.add_argument("--apply-out", required=True)
    apply_reverify_parser.add_argument("--archive-import", default=None)
    apply_reverify_parser.add_argument("--min-real-samples", type=int, default=0)
    apply_reverify_parser.add_argument("--max-age-days", type=int, default=0)
    apply_reverify_parser.add_argument("--current-time-override", default=None)

    retire_plan_parser = subparsers.add_parser("known-env-plan-retire")
    retire_plan_parser.add_argument("--phase", required=True)
    retire_plan_parser.add_argument("--runtime-baseline-registry", required=True)
    retire_plan_parser.add_argument("--runtime-history-index", required=True)
    retire_plan_parser.add_argument("--runtime-watch-registry", required=True)
    retire_plan_parser.add_argument("--governance-policy", default=None)
    retire_plan_parser.add_argument("--current-time-override", default=None)
    retire_plan_parser.add_argument("--retire-plan-out", required=True)
    retire_plan_parser.add_argument("--known-env-id", default="")
    retire_plan_parser.add_argument("--retire-reason", default="")

    apply_retire_parser = subparsers.add_parser("known-env-apply-retire")
    apply_retire_parser.add_argument("--runtime-baseline-registry", required=True)
    apply_retire_parser.add_argument("--known-env-id", default="")
    apply_retire_parser.add_argument("--retire-reason", default="")
    apply_retire_parser.add_argument("--archive-out", default=None)
    apply_retire_parser.add_argument("--retire-apply-out", required=True)

    retire_parser = subparsers.add_parser("known-env-retire")
    retire_parser.add_argument("--runtime-baseline-registry", required=True)
    retire_parser.add_argument("--known-env-id", default="")
    retire_parser.add_argument("--retire-reason", default="")
    retire_parser.add_argument("--archive-out", default=None)

    ops_parser = subparsers.add_parser("ops-summary")
    ops_parser.add_argument("--phase", required=True)
    ops_parser.add_argument("--policy-manifest", required=True)
    ops_parser.add_argument("--quick-summary", default=None)
    ops_parser.add_argument("--nightly-summary", default=None)
    ops_parser.add_argument("--matrix-summary", default=None)
    ops_parser.add_argument("--runtime-refresh", default=None)
    ops_parser.add_argument("--runtime-watch-refresh", default=None)
    ops_parser.add_argument("--runtime-watch-registry", default=None)
    ops_parser.add_argument("--runtime-baseline-registry", default=None)
    ops_parser.add_argument("--runtime-registry-health", default=None)
    ops_parser.add_argument("--publication-health", default=None)
    ops_parser.add_argument("--source-snapshot-manifest", default=None)
    ops_parser.add_argument("--staged-mirror-verify", default=None)
    ops_parser.add_argument("--verification-release", default=None)
    ops_parser.add_argument("--verification-debug", default=None)
    ops_parser.add_argument("--verification-asan", default=None)
    ops_parser.add_argument("--published-snapshot-manifest", default=None)
    ops_parser.add_argument("--verification-closeout", default=None)
    ops_parser.add_argument("--current-env-governance-policy", default=None)
    ops_parser.add_argument("--current-env-guardrail-policy", default=None)
    ops_parser.add_argument("--current-env-watch-current", default=None)
    ops_parser.add_argument("--current-env-watch-refresh", default=None)
    ops_parser.add_argument("--current-env-watch-history", default=None)
    ops_parser.add_argument("--current-env-age-tick", default=None)
    ops_parser.add_argument("--current-env-watch-plan", default=None)
    ops_parser.add_argument("--current-env-trigger-gate", default=None)
    ops_parser.add_argument("--current-env-due", default=None)
    ops_parser.add_argument("--current-env-reproposal-plan", default=None)
    ops_parser.add_argument("--ops-agenda", default=None)
    ops_parser.add_argument("--current-env-watch-execute", default=None)
    ops_parser.add_argument("--current-env-watch-apply", default=None)
    ops_parser.add_argument("--current-env-reproposal-execute", default=None)
    ops_parser.add_argument("--current-env-action-ledger", default=None)
    ops_parser.add_argument("--current-env-retry-plan", default=None)
    ops_parser.add_argument("--current-env-reproposal-handoff", default=None)
    ops_parser.add_argument("--current-env-operator-decision", default=None)
    ops_parser.add_argument("--current-env-operator-decision-apply", default=None)
    ops_parser.add_argument("--current-env-action-ledger-compact", default=None)
    ops_parser.add_argument("--current-env-action-ledger-archive", default=None)
    ops_parser.add_argument("--current-env-approval-runbook", default=None)
    ops_parser.add_argument("--current-env-approval-execution", default=None)
    ops_parser.add_argument("--current-env-approval-link", default=None)
    ops_parser.add_argument("--operator-runbook-index", default=None)
    ops_parser.add_argument("--operator-runbook-catalog", default=None)
    ops_parser.add_argument("--operator-decision-metadata-audit", default=None)
    ops_parser.add_argument("--operator-runbook-replay", default=None)
    ops_parser.add_argument("--operator-runbook-retention-policy", default=None)
    ops_parser.add_argument("--operator-runbook-pruned-catalog", default=None)
    ops_parser.add_argument("--operator-runbook-archive", default=None)
    ops_parser.add_argument("--operator-runbook-prune-summary", default=None)
    ops_parser.add_argument("--operator-runbook-lifecycle-validation", default=None)
    ops_parser.add_argument("--operator-runbook-pointer-audit", default=None)
    ops_parser.add_argument("--operator-runbook-provenance-migration", default=None)
    ops_parser.add_argument("--operator-runbook-migrated-catalog", default=None)
    ops_parser.add_argument("--operator-runbook-migrated-ledger", default=None)
    ops_parser.add_argument("--operator-runbook-lifecycle-validation-before", default=None)
    ops_parser.add_argument("--operator-runbook-lifecycle-validation-after", default=None)
    ops_parser.add_argument("--operator-artifact-path-policy-lint", default=None)
    ops_parser.add_argument("--integrated-approval-mutation-audit", default=None)
    ops_parser.add_argument("--source-health-action-plan", default=None)
    ops_parser.add_argument("--staged-materialization-transaction", default=None)
    ops_parser.add_argument("--source-health-preflight", default=None)
    ops_parser.add_argument("--staged-materialization", default=None)
    ops_parser.add_argument("--runtime-budget-current", default=None)
    ops_parser.add_argument("--runtime-budget-proposal", default=None)
    ops_parser.add_argument("--runtime-budget-proposal-gate", default=None)
    ops_parser.add_argument("--runtime-budget-baseline", default=None)
    ops_parser.add_argument("--runtime-budget-refresh", default=None)
    ops_parser.add_argument("--runtime-budget-reproposal-history", default=None)
    ops_parser.add_argument("--runtime-budget-registry-summary", default=None)
    ops_parser.add_argument("--approved-known-summary", action="append", default=[])
    ops_parser.add_argument("--foreign-import-summary", action="append", default=[])
    ops_parser.add_argument("--out", required=True)
    ops_parser.add_argument("--out-text", default=None)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.command == "registry-health":
        return action_registry_health(args)
    if args.command == "publication-health":
        return action_publication_health(args)
    if args.command == "watch-registry":
        return action_watch_registry(args)
    if args.command == "import-current":
        return action_import_current(args)
    if args.command == "import-external-bundle":
        return action_import_external_bundle(args)
    if args.command == "import-known-env-evidence":
        return action_import_known_env_evidence(args)
    if args.command == "current-env-governance-policy":
        return action_current_env_governance_policy(args)
    if args.command == "current-env-guardrail-policy":
        return action_current_env_governance_policy(args)
    if args.command == "current-env-watch-current":
        return action_current_env_watch_current(args)
    if args.command == "current-env-watch-refresh":
        return action_current_env_watch_refresh(args)
    if args.command == "current-env-watch-history":
        return action_current_env_watch_history(args)
    if args.command == "current-env-age-tick":
        return action_current_env_age_tick(args)
    if args.command == "current-env-plan-watch":
        return action_current_env_plan_watch(args)
    if args.command == "current-env-reproposal-trigger-gate":
        return action_current_env_reproposal_trigger_gate(args)
    if args.command == "current-env-due-scheduler":
        return action_current_env_due_scheduler(args)
    if args.command == "current-env-plan-reproposal":
        return action_current_env_plan_reproposal(args)
    if args.command == "current-env-execute-watch":
        return action_current_env_execute_watch(args)
    if args.command == "current-env-apply-watch":
        return action_current_env_apply_watch(args)
    if args.command == "current-env-execute-reproposal-gate":
        return action_current_env_execute_reproposal_gate(args)
    if args.command == "current-env-action-ledger-update":
        return action_current_env_action_ledger_update(args)
    if args.command == "current-env-action-retry-plan":
        return action_current_env_action_retry_plan(args)
    if args.command == "current-env-reproposal-handoff":
        return action_current_env_reproposal_handoff(args)
    if args.command == "current-env-operator-decision":
        return action_current_env_operator_decision(args)
    if args.command == "current-env-apply-operator-decision":
        return action_current_env_apply_operator_decision(args)
    if args.command == "current-env-action-ledger-compact":
        return action_current_env_action_ledger_compact(args)
    if args.command == "current-env-approval-runbook":
        return action_current_env_approval_runbook(args)
    if args.command == "current-env-execute-budget-approval":
        return action_current_env_execute_budget_approval(args)
    if args.command == "current-env-link-approval-execution":
        return action_current_env_link_approval_execution(args)
    if args.command == "operator-runbook-index":
        return action_operator_runbook_index(args)
    if args.command == "operator-runbook-catalog-update":
        return action_operator_runbook_catalog_update(args)
    if args.command == "operator-runbook-catalog-prune":
        return action_operator_runbook_catalog_prune(args)
    if args.command == "operator-runbook-validate-lifecycle":
        return action_operator_runbook_validate_lifecycle(args)
    if args.command == "operator-runbook-pointer-audit":
        return action_operator_runbook_pointer_audit(args)
    if args.command == "operator-runbook-provenance-migrate":
        return action_operator_runbook_provenance_migrate(args)
    if args.command == "operator-artifact-path-policy-lint":
        return action_operator_artifact_path_policy_lint(args)
    if args.command == "operator-decision-metadata-audit":
        return action_operator_decision_metadata_audit(args)
    if args.command == "operator-runbook-replay":
        return action_operator_runbook_replay(args)
    if args.command == "integrated-approval-mutation-audit":
        return action_integrated_approval_mutation_audit(args)
    if args.command == "source-health-plan":
        return action_source_health_plan(args)
    if args.command == "staged-materialization-transaction":
        return action_staged_materialization_transaction(args)
    if args.command == "current-env-action-ledger-invariants":
        return action_current_env_action_ledger_invariants(args)
    if args.command == "ops-agenda":
        return action_policy_ops_agenda(args)
    if args.command == "runtime-budget-reproposal-history":
        return action_runtime_budget_reproposal_history(args)
    if args.command == "runtime-budget-registry-summary":
        return action_runtime_budget_registry_summary(args)
    if args.command == "known-env-governance-policy":
        return action_known_env_governance_policy(args)
    if args.command == "known-env-age-tick":
        return action_known_env_age_tick(args)
    if args.command == "known-env-plan-reverify":
        return action_known_env_plan_reverify(args)
    if args.command == "known-env-reverify-gate":
        return action_known_env_reverify_gate(args)
    if args.command == "known-env-apply-reverify":
        return action_known_env_apply_reverify(args)
    if args.command == "known-env-plan-retire":
        return action_known_env_plan_retire(args)
    if args.command == "known-env-apply-retire":
        return action_known_env_apply_retire(args)
    if args.command == "known-env-retire":
        return action_known_env_retire(args)
    if args.command == "ops-summary":
        return action_ops_summary(args)
    raise SystemExit(f"unknown command: {args.command}")


if __name__ == "__main__":
    raise SystemExit(main())
