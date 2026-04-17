#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from datetime import datetime
from pathlib import Path


os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline-epoch", required=True, type=float)
    parser.add_argument("--analysis-log", required=True)
    parser.add_argument(
        "--target",
        action="append",
        default=[],
        help=(
            "Analysis asset to treat as refreshed. Retry start stays blocked until at least one "
            "branch-local analysis asset is passed here and is newer than the baseline."
        ),
    )
    parser.add_argument(
        "--target-from-current-state",
        action="store_true",
        help=(
            "Expand the verifier target set from refresh_evidence in --require-current-state so the "
            "retry workflow only clears when it recognizes a refreshed branch-local analysis asset."
        ),
    )
    parser.add_argument(
        "--require-json-key",
        action="append",
        default=[],
        help="Require a truthy key in a JSON file using path:key syntax",
    )
    parser.add_argument(
        "--latest-failure-report",
        help="Failure report JSON whose attempt/session/timestamp must match the refreshed state",
    )
    parser.add_argument(
        "--latest-failure-breakdown",
        help="Failure breakdown JSON whose attempt/session/timestamp must match the refreshed state",
    )
    parser.add_argument(
        "--require-current-state",
        help="JSON analysis state file that must be marked current for the latest failure",
    )
    parser.add_argument(
        "--require-analysis-session",
        help="Markdown latest_analysis_session note that must match the latest failure and show a post-failure refresh",
    )
    parser.add_argument(
        "--baseline-analysis-session",
        help="Prior baseline analysis-session markdown used to verify localization narrowing",
    )
    parser.add_argument(
        "--require-localization-narrowing",
        action="store_true",
        help="Require the refreshed state to prove at least one narrowed localization dimension",
    )
    return parser.parse_args()


def latest_mtime(paths: list[Path]) -> float:
    mtimes = []
    for path in paths:
        try:
            mtimes.append(path.stat().st_mtime)
        except OSError:
            continue
    return max(mtimes) if mtimes else 0.0


NON_QUALIFYING_TARGET_NAMES = {
    "verify_analysis_refresh.py",
}

RECOGNIZED_BRANCH_LOCAL_ANALYSIS_TARGET_NAMES = (
    "capture_failure_context.py",
    "failure_analysis_playbook.md",
    "failure_analysis_iteration.md",
    "failure_analysis_state.json",
    "launch_retry_loop.sh",
    "prepare_retry_attempt_state.py",
    "refresh_analysis_state.py",
    "restart_retry_loop_after_attempt.sh",
    "run_until_pass_progress40.sh",
    "verify_analysis_refresh.py",
)

RECOGNIZED_SUPPORTING_BRANCH_LOCAL_ANALYSIS_TARGET_NAMES = tuple(
    name
    for name in RECOGNIZED_BRANCH_LOCAL_ANALYSIS_TARGET_NAMES
    if name not in {"failure_analysis_state.json", "verify_analysis_refresh.py"}
)

REQUIRED_REFRESH_EVIDENCE_TIMESTAMPS = (
    "analysis_refresh_timestamp",
    "latest_failure_report_timestamp",
    "latest_failure_breakdown_timestamp",
    "current_failure_timestamp",
)

REQUIRED_REFRESH_EVIDENCE_BOOLEANS = (
    "refreshed_after_failure_report",
    "refreshed_after_failure_breakdown",
    "refreshed_after_current_failure_timestamp",
)

QUALIFYING_REFRESH_ASSET_KEYS = (
    "qualifying_refreshed_assets",
    "refreshed_assets",
)
REQUIRED_FRESHNESS_RECORD_KEYS = (
    "attempt_label",
    "session_id",
    "failure_timestamp",
    "failure_signature",
    "analysis_refresh_timestamp",
    "refreshed_asset",
)


def is_qualifying_target(path: Path) -> bool:
    return path.name not in NON_QUALIFYING_TARGET_NAMES


def branch_local_analysis_root(state_path: Path) -> Path:
    return state_path.resolve().parent


def resolve_recognized_branch_local_analysis_targets(
    state_path: Path,
    *,
    supporting_only: bool = False,
) -> list[Path]:
    analysis_root = branch_local_analysis_root(state_path)
    target_names = (
        RECOGNIZED_SUPPORTING_BRANCH_LOCAL_ANALYSIS_TARGET_NAMES
        if supporting_only
        else RECOGNIZED_BRANCH_LOCAL_ANALYSIS_TARGET_NAMES
    )
    return [analysis_root / name for name in target_names]


def resolve_branch_local_analysis_asset(state_path: Path, asset_path: Path) -> Path | None:
    analysis_root = branch_local_analysis_root(state_path)
    resolved_asset = asset_path.expanduser().resolve()
    try:
        resolved_asset.relative_to(analysis_root)
    except ValueError:
        return None
    return resolved_asset


def append_unique_path(paths: list[Path], candidate: Path) -> None:
    resolved_candidate = candidate.expanduser().resolve()
    if any(existing.expanduser().resolve() == resolved_candidate for existing in paths):
        return
    paths.append(resolved_candidate)


def load_workflow_recognized_targets_from_state(state_path: Path) -> list[Path]:
    state_payload = load_structured_dict(state_path)
    refresh_evidence = state_payload.get("refresh_evidence")
    if not isinstance(refresh_evidence, dict):
        raise ValueError(f"`{state_path}` refresh_evidence is missing or not a JSON object")

    recognized_targets = {
        path.expanduser().resolve()
        for path in resolve_recognized_branch_local_analysis_targets(state_path)
    }
    targets: list[Path] = []
    append_unique_path(targets, state_path.resolve())
    for asset in resolve_qualifying_refresh_assets(state_path, refresh_evidence):
        resolved_asset = resolve_branch_local_analysis_asset(state_path, asset)
        if resolved_asset is None:
            continue
        if resolved_asset not in recognized_targets:
            continue
        append_unique_path(targets, resolved_asset)

    freshness_record = refresh_evidence.get("freshness_record")
    if isinstance(freshness_record, dict):
        refreshed_asset_text = normalize_md_scalar(freshness_record.get("refreshed_asset"))
        if refreshed_asset_text is not None:
            refreshed_asset = resolve_branch_local_analysis_asset(
                state_path,
                Path(refreshed_asset_text),
            )
            if (
                refreshed_asset is not None
                and refreshed_asset in recognized_targets
                and is_qualifying_target(refreshed_asset)
            ):
                append_unique_path(targets, refreshed_asset)

    return targets


MARKDOWN_ATTEMPT_RE = re.compile(r"^# Failure (?:Report|Breakdown): Attempt (\d+)\s*$", re.MULTILINE)
ANALYSIS_SESSION_HEADING_RE = re.compile(r"^# Analysis Session Summary\s*$", re.MULTILINE)
ITERATION_LEDGER_HEADING_RE = re.compile(r"^# Failure Analysis Iteration Ledger\s*$", re.MULTILINE)
MARKDOWN_META_RE = re.compile(r"^- (?P<key>[^:]+): `(?P<value>[^`]*)`$", re.MULTILINE)
MARKDOWN_FAILED_ACS_RE = re.compile(r"^- Failed ACs: (?P<value>.+)$", re.MULTILINE)
MARKDOWN_FAILED_BREAKDOWN_RE = re.compile(r"^### AC (\d+):(?: \[FAIL\])?\s+", re.MULTILINE)
PATH_RANGE_RE = re.compile(r"^(?P<path>.+):(?P<start>\d+)-(?P<end>\d+)$")
FOCUS_RANGE_RE = re.compile(r"^(?P<start>\d+)-(?P<end>\d+)$")
LOCALIZATION_DIMENSIONS = {
    "line_span",
    "symbol_scope",
    "wrapper_section",
    "quoted_code_excerpt",
}


def normalize_md_scalar(value: str | None) -> str | None:
    if value is None:
        return None
    text = value.strip()
    if not text or text.lower() in {"none", "unknown", "unknown-session", "unknown-execution"}:
        return None
    return text


def normalize_string_list(values: object) -> list[str]:
    normalized: list[str] = []
    if not isinstance(values, list):
        return normalized
    for item in values:
        text = normalize_md_scalar(str(item) if item is not None else None)
        if text and text not in normalized:
            normalized.append(text)
    return normalized


def parse_branch_timestamp(value: object) -> datetime | None:
    text = normalize_md_scalar(str(value) if value is not None else None)
    if not text or len(text) < 19:
        return None
    try:
        return datetime.strptime(text[:19], "%Y-%m-%d %H:%M:%S")
    except ValueError:
        return None


def load_structured_dict(path: Path) -> dict:
    text = path.read_text()
    stripped = text.lstrip()
    if stripped.startswith("{") or stripped.startswith("["):
        payload = json.loads(text)
        if not isinstance(payload, dict):
            raise ValueError("payload is not a JSON object")
        return payload

    attempt_match = MARKDOWN_ATTEMPT_RE.search(text)
    if not attempt_match:
        raise ValueError("payload is neither JSON nor a supported failure markdown")

    metadata = {match.group("key"): match.group("value") for match in MARKDOWN_META_RE.finditer(text)}
    failed_acs = []
    failed_line = MARKDOWN_FAILED_ACS_RE.search(text)
    if failed_line:
        raw_value = failed_line.group("value").strip()
        if raw_value.lower() != "none found":
            failed_acs = re.findall(r"\b\d+\b", raw_value)
    if not failed_acs:
        failed_acs = MARKDOWN_FAILED_BREAKDOWN_RE.findall(text)

    return {
        "attempt": int(attempt_match.group(1)),
        "session_id": normalize_md_scalar(metadata.get("Session ID")),
        "execution_id": normalize_md_scalar(metadata.get("Execution ID")),
        "timestamp": normalize_md_scalar(metadata.get("Timestamp")),
        "failed_acs": failed_acs,
        "failed_ac_breakdowns": [{"ac_index": item} for item in failed_acs],
    }


def load_analysis_session_metadata(path: Path) -> dict[str, str]:
    text = path.read_text()
    if not ANALYSIS_SESSION_HEADING_RE.search(text):
        raise ValueError(f"`{path}` is not a supported analysis-session markdown summary")
    metadata = {match.group("key"): match.group("value") for match in MARKDOWN_META_RE.finditer(text)}
    if not metadata:
        raise ValueError(f"`{path}` does not expose any analysis-session metadata bullets")
    return metadata


def load_iteration_metadata(path: Path) -> dict[str, str]:
    text = path.read_text()
    if not ITERATION_LEDGER_HEADING_RE.search(text):
        raise ValueError(f"`{path}` is not a supported failure-analysis iteration ledger")
    metadata = {match.group("key"): match.group("value") for match in MARKDOWN_META_RE.finditer(text)}
    if not metadata:
        raise ValueError(f"`{path}` does not expose any failure-analysis metadata bullets")
    return metadata


def format_attempt_label(value: object) -> str | None:
    if value is None:
        return None
    if isinstance(value, str) and value.startswith("attempt_"):
        return value
    try:
        return f"attempt_{int(value):03d}"
    except (TypeError, ValueError):
        text = str(value).strip()
        return text or None


def parse_repo_range(value: object) -> tuple[str, int, int] | None:
    text = normalize_md_scalar(str(value) if value is not None else None)
    if not text:
        return None
    match = PATH_RANGE_RE.match(text)
    if not match:
        return None
    return (
        match.group("path"),
        int(match.group("start")),
        int(match.group("end")),
    )


def parse_focus_range(value: object) -> tuple[int, int] | None:
    text = normalize_md_scalar(str(value) if value is not None else None)
    if not text:
        return None
    match = FOCUS_RANGE_RE.match(text)
    if not match:
        return None
    return int(match.group("start")), int(match.group("end"))


def normalize_failed_acs(values: object) -> list[str]:
    normalized: list[str] = []
    if not isinstance(values, list):
        return normalized
    for item in values:
        if isinstance(item, dict):
            item = item.get("ac_index") or item.get("ac")
        elif isinstance(item, (list, tuple)):
            item = item[0] if item else None
        if item is None:
            continue
        text = str(item).strip()
        if text and text not in normalized:
            normalized.append(text)
    return normalized


def build_failure_signature(report_payload: dict, breakdown_payload: dict) -> dict[str, object]:
    attempt_label = (
        format_attempt_label(breakdown_payload.get("attempt"))
        or format_attempt_label(report_payload.get("attempt"))
    )
    session_id = breakdown_payload.get("session_id") or report_payload.get("session_id")
    execution_id = breakdown_payload.get("execution_id") or report_payload.get("execution_id")
    timestamp = breakdown_payload.get("timestamp") or report_payload.get("timestamp")
    failed_acs = normalize_failed_acs(report_payload.get("failed_acs"))
    if not failed_acs:
        failed_acs = normalize_failed_acs(
            [item.get("ac_index") for item in breakdown_payload.get("failed_ac_breakdowns") or []]
        )
    signature = "|".join(
        [
            attempt_label or "unknown-attempt",
            session_id or "unknown-session",
            timestamp or "unknown-timestamp",
            ",".join(failed_acs) or "none",
        ]
    )
    return {
        "attempt_label": attempt_label,
        "session_id": session_id,
        "execution_id": execution_id,
        "timestamp": timestamp,
        "failed_acs": failed_acs,
        "failure_signature": signature,
    }


def load_latest_failure_refresh_baseline(
    report_path: Path | None,
    breakdown_path: Path | None,
    state_path: Path | None = None,
) -> tuple[float | None, str | None]:
    sources: list[tuple[float, str]] = []
    payload_specs: list[tuple[str, Path | None, str | tuple[str, str]]] = [
        ("latest failure report", report_path, "timestamp"),
        ("latest failure breakdown", breakdown_path, "timestamp"),
        ("current failure state", state_path, ("current_failure", "timestamp")),
    ]
    for label, path, key in payload_specs:
        if path is None:
            continue
        payload = load_structured_dict(path)
        value: object | None
        if isinstance(key, tuple):
            outer, inner = key
            nested = payload.get(outer)
            if isinstance(nested, dict):
                value = nested.get(inner)
            else:
                value = payload.get(f"{outer}_{inner}")
        else:
            value = payload.get(key)
        dt = parse_branch_timestamp(value)
        if dt is None:
            raise ValueError(f"`{path}` is missing a parseable {label} timestamp")
        sources.append((dt.timestamp(), str(value).strip()))
    if not sources:
        return None, None
    latest_epoch, latest_label = max(sources, key=lambda item: item[0])
    return latest_epoch, latest_label


def verify_refresh_asset_evidence(state_path: Path, refresh_evidence: dict[str, object]) -> list[str]:
    qualifying_assets = resolve_qualifying_refresh_assets(state_path, refresh_evidence)
    return [f"{state_path}:refresh_asset={asset}" for asset in qualifying_assets]


def verify_analysis_session(
    analysis_session_path: Path,
    report_path: Path,
    breakdown_path: Path,
    state_path: Path | None,
    latest_failure_epoch: float | None,
    latest_failure_label: str | None,
) -> list[str]:
    metadata = load_analysis_session_metadata(analysis_session_path)
    expected = build_failure_signature(
        load_structured_dict(report_path),
        load_structured_dict(breakdown_path),
    )

    verification = normalize_md_scalar(metadata.get("Verification"))
    if verification != "refreshed analysis assets linked to latest failure":
        raise ValueError(
            f"`{analysis_session_path}` verification marker `{verification}` does not confirm a refreshed latest-failure analysis session"
        )

    current_flag = normalize_md_scalar(metadata.get("Current for latest failure"))
    if current_flag is None or current_flag.lower() not in {"yes", "true"}:
        raise ValueError(f"`{analysis_session_path}` is not marked current for latest failure")

    current_attempt = format_attempt_label(metadata.get("Current failure attempt"))
    if current_attempt != expected.get("attempt_label"):
        raise ValueError(
            f"`{analysis_session_path}` current failure attempt `{current_attempt}` does not match latest failure "
            f"`{expected.get('attempt_label')}`"
        )

    current_signature = normalize_md_scalar(metadata.get("Current failure signature"))
    if current_signature != expected.get("failure_signature"):
        raise ValueError(
            f"`{analysis_session_path}` current failure signature `{current_signature}` does not match latest failure "
            f"`{expected.get('failure_signature')}`"
        )

    note_timestamp = normalize_md_scalar(metadata.get("Timestamp"))
    dt = parse_branch_timestamp(note_timestamp)
    if dt is None:
        raise ValueError(f"`{analysis_session_path}` is missing a parseable analysis-session timestamp")
    if latest_failure_epoch is not None and dt.timestamp() <= latest_failure_epoch:
        failure_label = latest_failure_label or "unknown latest failure timestamp"
        raise ValueError(
            f"`{analysis_session_path}` timestamp `{note_timestamp}` is not newer than latest failed attempt timestamp "
            f"`{failure_label}`"
        )

    if state_path is not None:
        state_payload = load_structured_dict(state_path)
        state_signature = normalize_md_scalar(str(state_payload.get("current_failure_signature") or ""))
        if state_signature and current_signature != state_signature:
            raise ValueError(
                f"`{analysis_session_path}` current failure signature `{current_signature}` does not match "
                f"`{state_path}` current failure signature `{state_signature}`"
            )

    return [
        f"{analysis_session_path}:analysis_session.current_for_latest_failure=yes",
        f"{analysis_session_path}:analysis_session.current_failure_attempt={current_attempt}",
        f"{analysis_session_path}:analysis_session.current_failure_signature={current_signature}",
        f"{analysis_session_path}:analysis_session.verification={verification}",
        f"{analysis_session_path}:analysis_session.post_failure_timestamp={note_timestamp}",
    ]


def verify_freshness_record(
    state_path: Path,
    refresh_evidence: dict[str, object],
    current_failure: dict[str, object],
) -> list[str]:
    refreshed_asset_resolved = resolve_freshness_record_asset(
        state_path,
        refresh_evidence,
        current_failure,
    )
    return [f"{state_path}:freshness_record.refreshed_asset={refreshed_asset_resolved}"]


def resolve_freshness_record_asset(
    state_path: Path,
    refresh_evidence: dict[str, object],
    current_failure: dict[str, object],
) -> Path:
    freshness_record = refresh_evidence.get("freshness_record")
    if not isinstance(freshness_record, dict):
        raise ValueError(f"`{state_path}` refresh_evidence.freshness_record is missing or not a JSON object")

    for key in REQUIRED_FRESHNESS_RECORD_KEYS:
        if not normalize_md_scalar(freshness_record.get(key)):
            raise ValueError(f"`{state_path}` refresh_evidence.freshness_record.{key} is missing or empty")

    recorded_attempt = format_attempt_label(freshness_record.get("attempt_label"))
    if recorded_attempt != current_failure.get("attempt_label"):
        raise ValueError(
            f"`{state_path}` refresh_evidence.freshness_record.attempt_label `{recorded_attempt}` "
            f"does not match latest failure `{current_failure.get('attempt_label')}`"
        )
    recorded_session = normalize_md_scalar(freshness_record.get("session_id"))
    if recorded_session != current_failure.get("session_id"):
        raise ValueError(
            f"`{state_path}` refresh_evidence.freshness_record.session_id `{recorded_session}` "
            f"does not match latest failure `{current_failure.get('session_id')}`"
        )
    recorded_execution = normalize_md_scalar(freshness_record.get("execution_id"))
    if current_failure.get("execution_id") and recorded_execution != current_failure.get("execution_id"):
        raise ValueError(
            f"`{state_path}` refresh_evidence.freshness_record.execution_id `{recorded_execution}` "
            f"does not match latest failure `{current_failure.get('execution_id')}`"
        )
    recorded_failure_timestamp = normalize_md_scalar(freshness_record.get("failure_timestamp"))
    if recorded_failure_timestamp != current_failure.get("timestamp"):
        raise ValueError(
            f"`{state_path}` refresh_evidence.freshness_record.failure_timestamp "
            f"`{recorded_failure_timestamp}` does not match latest failure `{current_failure.get('timestamp')}`"
        )
    recorded_signature = normalize_md_scalar(freshness_record.get("failure_signature"))
    if recorded_signature != current_failure.get("failure_signature"):
        raise ValueError(
            f"`{state_path}` refresh_evidence.freshness_record.failure_signature `{recorded_signature}` "
            f"does not match latest failure `{current_failure.get('failure_signature')}`"
        )
    recorded_refresh_timestamp = normalize_md_scalar(freshness_record.get("analysis_refresh_timestamp"))
    refresh_timestamp = normalize_md_scalar(refresh_evidence.get("analysis_refresh_timestamp"))
    if recorded_refresh_timestamp != refresh_timestamp:
        raise ValueError(
            f"`{state_path}` refresh_evidence.freshness_record.analysis_refresh_timestamp "
            f"`{recorded_refresh_timestamp}` does not match refresh_evidence.analysis_refresh_timestamp "
            f"`{refresh_timestamp}`"
        )

    refreshed_asset_text = normalize_md_scalar(freshness_record.get("refreshed_asset"))
    if refreshed_asset_text is None:
        raise ValueError(f"`{state_path}` refresh_evidence.freshness_record.refreshed_asset is missing")
    refreshed_asset = Path(refreshed_asset_text)
    refreshed_asset_resolved = resolve_branch_local_analysis_asset(state_path, refreshed_asset)
    if refreshed_asset_resolved is None:
        analysis_root = branch_local_analysis_root(state_path)
        raise ValueError(
            f"`{state_path}` refresh_evidence.freshness_record.refreshed_asset `{refreshed_asset}` "
            f"is not a branch-local analysis asset under `{analysis_root}`"
        )
    if not refreshed_asset_resolved.exists():
        raise ValueError(
            f"`{state_path}` refresh_evidence.freshness_record.refreshed_asset `{refreshed_asset}` does not exist"
        )
    supporting_assets = resolve_supporting_refresh_assets(state_path, refresh_evidence)
    if not any(asset.resolve() == refreshed_asset_resolved for asset in supporting_assets):
        raise ValueError(
            f"`{state_path}` refresh_evidence.freshness_record.refreshed_asset `{refreshed_asset}` "
            "is not one of the supporting refreshed analysis assets"
        )

    return refreshed_asset_resolved


def resolve_qualifying_refresh_assets(
    state_path: Path,
    refresh_evidence: dict[str, object],
) -> list[Path]:
    recorded_assets: list[str] = []
    for key in QUALIFYING_REFRESH_ASSET_KEYS:
        recorded_assets.extend(normalize_string_list(refresh_evidence.get(key)))

    if not recorded_assets:
        raise ValueError(
            f"`{state_path}` refresh_evidence must record qualifying_refreshed_assets "
            "or refreshed_assets for the latest failure"
        )

    qualifying_assets: list[str] = []
    non_qualifying_assets: list[str] = []
    analysis_root = branch_local_analysis_root(state_path)
    for asset in recorded_assets:
        asset_path = Path(asset)
        if resolve_branch_local_analysis_asset(state_path, asset_path) is None:
            continue
        if is_qualifying_target(asset_path):
            if asset not in qualifying_assets:
                qualifying_assets.append(asset)
            continue
        if asset not in non_qualifying_assets:
            non_qualifying_assets.append(asset)

    if not qualifying_assets:
        details = ", ".join(non_qualifying_assets) or "none recorded under branch-local analysis root"
        raise ValueError(
            f"`{state_path}` refresh_evidence does not record any qualifying refreshed analysis asset "
            f"under `{analysis_root}` (only non-qualifying assets: {details})"
        )

    return [Path(asset) for asset in qualifying_assets]


def resolve_supporting_refresh_assets(
    state_path: Path,
    refresh_evidence: dict[str, object],
) -> list[Path]:
    qualifying_assets = resolve_qualifying_refresh_assets(state_path, refresh_evidence)
    state_real_path = state_path.resolve()
    supporting_assets = [asset for asset in qualifying_assets if asset.resolve() != state_real_path]

    if supporting_assets:
        return supporting_assets

    raise ValueError(
        f"`{state_path}` refresh_evidence must record at least one supporting refreshed branch-local "
        "analysis helper or note besides failure_analysis_state.json"
    )


def resolve_recognized_branch_local_targets(
    state_path: Path,
    targets: list[Path],
) -> list[Path]:
    recognized_target_roots = {
        path.expanduser().resolve()
        for path in resolve_recognized_branch_local_analysis_targets(state_path)
    }
    recognized_targets: list[Path] = []
    seen: set[Path] = set()
    for target in targets:
        resolved_target = resolve_branch_local_analysis_asset(state_path, target)
        if (
            resolved_target is None
            or resolved_target not in recognized_target_roots
            or resolved_target in seen
        ):
            continue
        seen.add(resolved_target)
        recognized_targets.append(resolved_target)
    return recognized_targets


def verify_freshness_record_recognized_by_targets(
    state_path: Path,
    refresh_evidence: dict[str, object],
    current_failure: dict[str, object],
    targets: list[Path],
) -> list[str]:
    recognized_targets = resolve_recognized_branch_local_targets(state_path, targets)
    if not recognized_targets:
        analysis_root = branch_local_analysis_root(state_path)
        rendered_targets = ", ".join(str(path) for path in targets) or "none"
        raise ValueError(
            f"`{state_path}` retry-start target set does not include any branch-local analysis asset under "
            f"`{analysis_root}` (targets: {rendered_targets})"
        )

    refreshed_asset = resolve_freshness_record_asset(
        state_path,
        refresh_evidence,
        current_failure,
    )
    recognized_target_set = {path.resolve() for path in recognized_targets}
    if refreshed_asset.resolve() not in recognized_target_set:
        rendered_targets = ", ".join(str(path) for path in recognized_targets)
        raise ValueError(
            f"`{state_path}` freshness_record.refreshed_asset `{refreshed_asset}` is not one of the "
            f"workflow-recognized branch-local retry-start targets ({rendered_targets})"
        )

    return [f"{state_path}:recognized_refresh_target={refreshed_asset}"]


def verify_recognized_refresh_targets(
    state_path: Path,
    refresh_evidence: dict[str, object],
    targets: list[Path],
    *,
    baseline_epoch: float,
    latest_failure_epoch: float | None,
    latest_failure_label: str | None,
) -> list[str]:
    supporting_assets = resolve_supporting_refresh_assets(state_path, refresh_evidence)
    supporting_by_path = {asset.resolve(): asset for asset in supporting_assets}
    freshness_record = refresh_evidence.get("freshness_record")
    designated_asset: Path | None = None
    if isinstance(freshness_record, dict):
        refreshed_asset_text = normalize_md_scalar(freshness_record.get("refreshed_asset"))
        if refreshed_asset_text is not None:
            designated_asset = resolve_branch_local_analysis_asset(
                state_path,
                Path(refreshed_asset_text),
            )

    recognized_targets: list[Path] = []
    seen: set[Path] = set()
    for target in targets:
        try:
            resolved_target = target.expanduser().resolve()
            mtime = target.stat().st_mtime
        except OSError:
            continue
        if resolved_target not in supporting_by_path:
            continue
        if mtime < baseline_epoch:
            continue
        if latest_failure_epoch is not None and mtime <= latest_failure_epoch:
            continue
        canonical_target = supporting_by_path[resolved_target]
        if canonical_target in seen:
            continue
        seen.add(canonical_target)
        recognized_targets.append(canonical_target)

    if recognized_targets:
        if designated_asset is None or designated_asset.resolve() not in {
            target.resolve() for target in recognized_targets
        }:
            designated_label = str(designated_asset) if designated_asset is not None else "missing"
            raise ValueError(
                f"`{state_path}` freshness_record.refreshed_asset `{designated_label}` is not one of the "
                "workflow-recognized refreshed analysis targets that cleared retry preflight"
            )
        return [
            *[f"{state_path}:recognized_refresh_target={target}" for target in recognized_targets],
            f"{state_path}:recognized_freshness_record_asset={designated_asset}",
        ]

    recorded_targets = ", ".join(str(asset) for asset in supporting_assets) or "none"
    failure_label = latest_failure_label or "unknown latest failure timestamp"
    raise ValueError(
        f"`{state_path}` does not have any supporting refreshed analysis asset that is both newer than "
        f"baseline/latest failure and recognized by the retry-start workflow targets "
        f"(latest failure `{failure_label}`; recorded supporting assets: {recorded_targets})"
    )


def verify_post_failure_refresh_asset_freshness(
    state_path: Path,
    refresh_evidence: dict[str, object],
    latest_failure_epoch: float | None,
    latest_failure_label: str | None,
    *,
    require_iteration_metadata: bool = True,
) -> list[str]:
    state_payload = load_structured_dict(state_path)
    current_payload = state_payload.get("current_failure")
    if current_payload is not None and not isinstance(current_payload, dict):
        raise ValueError(f"`current_failure` in `{state_path}` is not a JSON object")
    current_failure = {
        "attempt_label": state_payload.get("current_failure_attempt")
        or (current_payload or {}).get("attempt_label")
        or state_payload.get("last_failed_attempt"),
        "session_id": state_payload.get("current_failure_session_id")
        or (current_payload or {}).get("session_id")
        or state_payload.get("last_failed_session_id"),
        "execution_id": state_payload.get("current_failure_execution_id")
        or (current_payload or {}).get("execution_id"),
        "timestamp": state_payload.get("current_failure_timestamp")
        or (current_payload or {}).get("timestamp"),
        "failure_signature": state_payload.get("current_failure_signature")
        or (current_payload or {}).get("failure_signature"),
    }
    refreshed_asset = resolve_freshness_record_asset(
        state_path,
        refresh_evidence,
        current_failure,
    )
    if latest_failure_epoch is None:
        return []

    try:
        mtime = refreshed_asset.stat().st_mtime
    except OSError as exc:
        raise ValueError(
            f"`{state_path}` freshness_record.refreshed_asset `{refreshed_asset}` could not be stat'ed"
        ) from exc
    if mtime > latest_failure_epoch:
        markers = [f"{state_path}:post_failure_refresh_asset={refreshed_asset}"]
        if require_iteration_metadata and refreshed_asset.name == "failure_analysis_iteration.md":
            markers.extend(
                verify_iteration_refresh_asset(
                    refreshed_asset,
                    current_failure,
                    latest_failure_epoch,
                    latest_failure_label,
                )
            )
        return markers

    failure_label = latest_failure_label or "unknown latest failure timestamp"
    raise ValueError(
        "no designated analysis asset is newer than latest failed attempt timestamp "
        f"`{failure_label}` (freshness_record.refreshed_asset={refreshed_asset}; "
        "designated asset must be branch-local)"
    )


def verify_iteration_refresh_asset(
    iteration_path: Path,
    current_failure: dict[str, object],
    latest_failure_epoch: float | None,
    latest_failure_label: str | None,
) -> list[str]:
    metadata = load_iteration_metadata(iteration_path)

    current_flag = normalize_md_scalar(metadata.get("Current for latest failure"))
    if current_flag is None or current_flag.lower() not in {"yes", "true"}:
        raise ValueError(f"`{iteration_path}` is not marked current for latest failure")

    failed_attempt = format_attempt_label(metadata.get("Failed attempt"))
    if failed_attempt != current_failure.get("attempt_label"):
        raise ValueError(
            f"`{iteration_path}` failed attempt `{failed_attempt}` does not match latest failure "
            f"`{current_failure.get('attempt_label')}`"
        )

    failure_session = normalize_md_scalar(metadata.get("Current failure session"))
    if failure_session != current_failure.get("session_id"):
        raise ValueError(
            f"`{iteration_path}` current failure session `{failure_session}` does not match latest failure "
            f"`{current_failure.get('session_id')}`"
        )

    expected_execution = normalize_md_scalar(str(current_failure.get("execution_id") or ""))
    failure_execution = normalize_md_scalar(metadata.get("Current failure execution"))
    if expected_execution and failure_execution != expected_execution:
        raise ValueError(
            f"`{iteration_path}` current failure execution `{failure_execution}` does not match latest failure "
            f"`{expected_execution}`"
        )

    failure_timestamp = normalize_md_scalar(metadata.get("Current failure timestamp"))
    if failure_timestamp != current_failure.get("timestamp"):
        raise ValueError(
            f"`{iteration_path}` current failure timestamp `{failure_timestamp}` does not match latest failure "
            f"`{current_failure.get('timestamp')}`"
        )

    failure_signature = normalize_md_scalar(metadata.get("Current failure signature"))
    if failure_signature != current_failure.get("failure_signature"):
        raise ValueError(
            f"`{iteration_path}` current failure signature `{failure_signature}` does not match latest failure "
            f"`{current_failure.get('failure_signature')}`"
        )

    refresh_timestamp = normalize_md_scalar(metadata.get("Analysis refresh timestamp"))
    refresh_dt = parse_branch_timestamp(refresh_timestamp)
    if refresh_dt is None:
        raise ValueError(f"`{iteration_path}` is missing a parseable analysis refresh timestamp")
    if latest_failure_epoch is not None and refresh_dt.timestamp() <= latest_failure_epoch:
        failure_label = latest_failure_label or "unknown latest failure timestamp"
        raise ValueError(
            f"`{iteration_path}` analysis refresh timestamp `{refresh_timestamp}` is not newer than latest failed "
            f"attempt timestamp `{failure_label}`"
        )

    freshness_asset = normalize_md_scalar(metadata.get("Freshness record asset"))
    if freshness_asset and Path(freshness_asset).expanduser().resolve() != iteration_path.resolve():
        raise ValueError(
            f"`{iteration_path}` freshness record asset `{freshness_asset}` does not point back to the designated "
            "iteration ledger"
        )

    freshness_signature = normalize_md_scalar(metadata.get("Freshness record failure signature"))
    if freshness_signature and freshness_signature != current_failure.get("failure_signature"):
        raise ValueError(
            f"`{iteration_path}` freshness record failure signature `{freshness_signature}` does not match latest "
            f"failure `{current_failure.get('failure_signature')}`"
        )

    return [
        f"{iteration_path}:current_for_latest_failure=yes",
        f"{iteration_path}:failed_attempt={failed_attempt}",
        f"{iteration_path}:current_failure_signature={failure_signature}",
        f"{iteration_path}:analysis_refresh_timestamp={refresh_timestamp}",
    ]


def verify_current_state(
    state_path: Path,
    report_path: Path,
    breakdown_path: Path,
    *,
    recognized_targets: list[Path] | None = None,
) -> list[str]:
    report_payload = load_structured_dict(report_path)
    breakdown_payload = load_structured_dict(breakdown_path)
    state_payload = load_structured_dict(state_path)
    expected = build_failure_signature(report_payload, breakdown_payload)

    report_attempt = format_attempt_label(report_payload.get("attempt"))
    breakdown_attempt = format_attempt_label(breakdown_payload.get("attempt"))
    if report_attempt and breakdown_attempt and report_attempt != breakdown_attempt:
        raise ValueError(
            f"latest failure attempt mismatch between `{report_path}` ({report_attempt}) and "
            f"`{breakdown_path}` ({breakdown_attempt})"
        )
    if report_payload.get("session_id") and breakdown_payload.get("session_id"):
        if report_payload["session_id"] != breakdown_payload["session_id"]:
            raise ValueError(
                f"latest failure session mismatch between `{report_path}` and `{breakdown_path}`"
            )
    current_payload = state_payload.get("current_failure")
    if current_payload is not None and not isinstance(current_payload, dict):
        raise ValueError(f"`current_failure` in `{state_path}` is not a JSON object")
    refresh_evidence = state_payload.get("refresh_evidence")
    if not isinstance(refresh_evidence, dict):
        raise ValueError(f"`refresh_evidence` in `{state_path}` is missing or not a JSON object")

    actual = {
        "current_for_latest_failure": state_payload.get("current_for_latest_failure"),
        "attempt_label": state_payload.get("current_failure_attempt")
        or (current_payload or {}).get("attempt_label")
        or state_payload.get("last_failed_attempt"),
        "session_id": state_payload.get("current_failure_session_id")
        or (current_payload or {}).get("session_id")
        or state_payload.get("last_failed_session_id"),
        "execution_id": state_payload.get("current_failure_execution_id")
        or (current_payload or {}).get("execution_id"),
        "timestamp": state_payload.get("current_failure_timestamp")
        or (current_payload or {}).get("timestamp"),
        "failed_acs": state_payload.get("current_failure_failed_acs")
        or (current_payload or {}).get("failed_acs")
        or [],
        "failure_signature": state_payload.get("current_failure_signature")
        or (current_payload or {}).get("failure_signature"),
    }

    if actual["current_for_latest_failure"] is not True:
        raise ValueError(f"`{state_path}` is not marked current_for_latest_failure=true")
    if normalize_failed_acs(actual["failed_acs"]) != expected["failed_acs"]:
        raise ValueError(
            f"`{state_path}` failed AC marker {normalize_failed_acs(actual['failed_acs'])} "
            f"does not match latest failure {expected['failed_acs']}"
        )
    for key in ("attempt_label", "session_id", "timestamp", "failure_signature"):
        if actual.get(key) != expected.get(key):
            raise ValueError(
                f"`{state_path}` {key} `{actual.get(key)}` does not match latest failure "
                f"`{expected.get(key)}`"
            )
    if expected.get("execution_id") and actual.get("execution_id") != expected.get("execution_id"):
        raise ValueError(
            f"`{state_path}` execution_id `{actual.get('execution_id')}` does not match latest failure "
            f"`{expected.get('execution_id')}`"
        )
    for key in REQUIRED_REFRESH_EVIDENCE_TIMESTAMPS:
        if not normalize_md_scalar(refresh_evidence.get(key)):
            raise ValueError(f"`{state_path}` refresh_evidence.{key} is missing or empty")
    if refresh_evidence.get("latest_failure_report_timestamp") != report_payload.get("timestamp"):
        raise ValueError(
            f"`{state_path}` refresh_evidence.latest_failure_report_timestamp "
            f"`{refresh_evidence.get('latest_failure_report_timestamp')}` does not match latest failure "
            f"report `{report_payload.get('timestamp')}`"
        )
    if refresh_evidence.get("latest_failure_breakdown_timestamp") != breakdown_payload.get("timestamp"):
        raise ValueError(
            f"`{state_path}` refresh_evidence.latest_failure_breakdown_timestamp "
            f"`{refresh_evidence.get('latest_failure_breakdown_timestamp')}` does not match latest failure "
            f"breakdown `{breakdown_payload.get('timestamp')}`"
        )
    if refresh_evidence.get("current_failure_timestamp") != actual.get("timestamp"):
        raise ValueError(
            f"`{state_path}` refresh_evidence.current_failure_timestamp "
            f"`{refresh_evidence.get('current_failure_timestamp')}` does not match current failure "
            f"`{actual.get('timestamp')}`"
        )
    for key in REQUIRED_REFRESH_EVIDENCE_BOOLEANS:
        if refresh_evidence.get(key) is not True:
            raise ValueError(f"`{state_path}` refresh_evidence.{key} is not true")
    verified_refresh_assets = verify_refresh_asset_evidence(state_path, refresh_evidence)
    freshness_record_markers = verify_freshness_record(state_path, refresh_evidence, actual)
    recognized_refresh_target_markers = []
    if recognized_targets is not None:
        recognized_refresh_target_markers = verify_freshness_record_recognized_by_targets(
            state_path,
            refresh_evidence,
            actual,
            recognized_targets,
        )

    return [
        f"{state_path}:current_for_latest_failure",
        f"{state_path}:current_failure_attempt={expected['attempt_label']}",
        f"{state_path}:current_failure_session_id={expected['session_id']}",
        f"{state_path}:current_failure_signature={expected['failure_signature']}",
        f"{state_path}:refresh_evidence.refreshed_after_failure_breakdown=true",
        *verified_refresh_assets,
        *freshness_record_markers,
        *recognized_refresh_target_markers,
    ]


def verify_localization_refinement(
    state_path: Path,
    baseline_analysis_session: Path | None,
) -> list[str]:
    state_payload = load_structured_dict(state_path)
    refinement = state_payload.get("localization_refinement")
    if not isinstance(refinement, dict):
        raise ValueError(f"`{state_path}` is missing `localization_refinement`")
    if refinement.get("verified_more_precise") is not True:
        raise ValueError(f"`{state_path}` localization_refinement.verified_more_precise is not true")

    baseline_reference = normalize_md_scalar(refinement.get("baseline_reference"))
    if not baseline_reference:
        raise ValueError(f"`{state_path}` localization_refinement.baseline_reference is missing")
    if baseline_analysis_session is not None:
        resolved_baseline = baseline_analysis_session.resolve()
        if not resolved_baseline.exists():
            raise ValueError(f"baseline analysis session `{baseline_analysis_session}` does not exist")
        expected_refs = {
            baseline_analysis_session.name,
            baseline_analysis_session.as_posix(),
            str(resolved_baseline),
        }
        if baseline_reference not in expected_refs and not str(resolved_baseline).endswith(baseline_reference):
            raise ValueError(
                f"`{state_path}` localization_refinement.baseline_reference `{baseline_reference}` "
                f"does not match `{baseline_analysis_session}`"
            )

    narrowed_dimensions = refinement.get("narrowed_dimensions")
    if not isinstance(narrowed_dimensions, list) or not narrowed_dimensions:
        raise ValueError(f"`{state_path}` localization_refinement.narrowed_dimensions is missing or empty")

    latest_statement_anchors = state_payload.get("latest_retry_statement_anchors")
    if not isinstance(latest_statement_anchors, list) or not latest_statement_anchors:
        raise ValueError(
            f"`{state_path}` must keep `latest_retry_statement_anchors` populated for localization verification"
        )

    baseline_localization = state_payload.get("baseline_localization")
    broad_solver_ranges: list[tuple[str, int, int]] = []
    if isinstance(baseline_localization, dict):
        for item in baseline_localization.get("broad_solver_ranges") or []:
            parsed = parse_repo_range(item)
            if parsed is not None:
                broad_solver_ranges.append(parsed)

    verified_dimensions: list[str] = []
    for index, item in enumerate(narrowed_dimensions, start=1):
        if not isinstance(item, dict):
            raise ValueError(
                f"`{state_path}` localization_refinement.narrowed_dimensions[{index}] is not an object"
            )
        dimension = normalize_md_scalar(item.get("dimension"))
        baseline = normalize_md_scalar(item.get("baseline"))
        current = normalize_md_scalar(item.get("current"))
        why_narrower = normalize_md_scalar(item.get("why_narrower"))
        if not dimension or not baseline or not current or not why_narrower:
            raise ValueError(
                f"`{state_path}` localization_refinement.narrowed_dimensions[{index}] is incomplete"
            )
        if baseline == current:
            raise ValueError(
                f"`{state_path}` localization_refinement.narrowed_dimensions[{index}] "
                "must not reuse the same baseline and current description"
            )
        if dimension not in LOCALIZATION_DIMENSIONS:
            raise ValueError(
                f"`{state_path}` localization_refinement.narrowed_dimensions[{index}] "
                f"has unsupported dimension `{dimension}`"
            )

        if dimension == "line_span":
            narrowed_line_span = False
            for anchor in latest_statement_anchors:
                if not isinstance(anchor, dict):
                    continue
                anchor_path = normalize_md_scalar(anchor.get("path"))
                focus_range = parse_focus_range(anchor.get("focus_range"))
                if not anchor_path or focus_range is None:
                    continue
                for base_path, base_start, base_end in broad_solver_ranges:
                    if anchor_path != base_path:
                        continue
                    if base_start <= focus_range[0] <= focus_range[1] <= base_end:
                        if (focus_range[1] - focus_range[0]) < (base_end - base_start):
                            narrowed_line_span = True
                            break
                if narrowed_line_span:
                    break
            if not narrowed_line_span:
                raise ValueError(
                    f"`{state_path}` claims line-span narrowing but no statement anchor is narrower than "
                    "`baseline_localization.broad_solver_ranges`"
                )

        if dimension == "quoted_code_excerpt":
            has_excerpt = any(
                normalize_md_scalar(anchor.get("excerpt"))
                for anchor in latest_statement_anchors
                if isinstance(anchor, dict)
            )
            if not has_excerpt:
                raise ValueError(
                    f"`{state_path}` claims quoted-code narrowing but no statement anchor excerpt is present"
                )

        verified_dimensions.append(dimension)

    return [
        f"{state_path}:localization_refinement.verified_more_precise=true",
        f"{state_path}:localization_refinement.baseline_reference={baseline_reference}",
        f"{state_path}:localization_refinement.narrowed_dimensions={','.join(verified_dimensions)}",
    ]


def main() -> int:
    args = parse_args()
    analysis_log = Path(args.analysis_log)
    report_path = Path(args.latest_failure_report) if args.latest_failure_report else None
    breakdown_path = Path(args.latest_failure_breakdown) if args.latest_failure_breakdown else None
    state_path = Path(args.require_current_state) if args.require_current_state else None
    analysis_session_path = Path(args.require_analysis_session) if args.require_analysis_session else None
    baseline_analysis_session = (
        Path(args.baseline_analysis_session) if args.baseline_analysis_session else None
    )
    analysis_root = state_path.resolve().parent if state_path is not None else None
    targets: list[Path] = []

    if args.target_from_current_state:
        if state_path is None:
            print(
                "analysis refresh verification failed: --target-from-current-state also needs "
                "--require-current-state"
            )
            return 1
        try:
            for candidate in load_workflow_recognized_targets_from_state(state_path):
                append_unique_path(targets, candidate)
        except Exception as exc:  # noqa: BLE001
            print(f"analysis refresh verification failed: {exc}")
            return 1

    for item in args.target:
        append_unique_path(targets, Path(item))

    try:
        latest_failure_epoch, latest_failure_label = load_latest_failure_refresh_baseline(
            report_path,
            breakdown_path,
            state_path,
        )
    except Exception as exc:  # noqa: BLE001
        print(f"analysis refresh verification failed: {exc}")
        return 1

    refreshed_targets = []
    refreshed_qualifying_targets = []
    refreshed_supporting_targets = []
    post_failure_qualifying_targets = []
    post_failure_supporting_targets = []
    state_real_path = state_path.resolve() if state_path is not None else None
    if not targets:
        print(
            "analysis refresh verification failed: retry start blocked until at least one refreshed "
            "branch-local analysis asset is supplied via --target"
        )
        return 1
    for path in targets:
        try:
            mtime = path.stat().st_mtime
        except OSError:
            continue
        if mtime >= args.baseline_epoch:
            refreshed_targets.append(path)
            if is_qualifying_target(path):
                refreshed_qualifying_targets.append(path)
                if (
                    state_real_path is None
                    or (
                        analysis_root is not None
                        and path.expanduser().resolve().is_relative_to(analysis_root)
                        and path.resolve() != state_real_path
                    )
                ):
                    refreshed_supporting_targets.append(path)
        if is_qualifying_target(path) and latest_failure_epoch is not None and mtime > latest_failure_epoch:
            post_failure_qualifying_targets.append(path)
            if (
                state_real_path is None
                or (
                    analysis_root is not None
                    and path.expanduser().resolve().is_relative_to(analysis_root)
                    and path.resolve() != state_real_path
                )
            ):
                post_failure_supporting_targets.append(path)

    analysis_log_mtime = latest_mtime([analysis_log])

    if not refreshed_targets:
        print(
            "analysis refresh verification failed: retry start blocked until at least one refreshed "
            "analysis asset is newer than the baseline"
        )
        return 1
    if not refreshed_qualifying_targets:
        refreshed_target_list = ", ".join(str(path) for path in refreshed_targets)
        print(
            "analysis refresh verification failed: no qualifying analysis asset updated after baseline "
            f"(only non-qualifying targets changed: {refreshed_target_list})"
        )
        return 1
    if state_real_path is not None and not refreshed_supporting_targets:
        refreshed_target_list = ", ".join(str(path) for path in refreshed_qualifying_targets) or "none"
        print(
            "analysis refresh verification failed: no supporting branch-local analysis helper or note updated after baseline "
            f"(refreshed qualifying targets: {refreshed_target_list})"
        )
        return 1
    if analysis_log_mtime < args.baseline_epoch:
        print("analysis refresh verification failed: analysis log did not update after baseline")
        return 1
    if latest_failure_epoch is not None and state_real_path is None and not post_failure_qualifying_targets:
        refreshed_target_list = ", ".join(str(path) for path in refreshed_qualifying_targets) or "none"
        print(
            "analysis refresh verification failed: no qualifying analysis asset shows a refresh newer "
            f"than the latest failed attempt timestamp `{latest_failure_label}` "
            f"(refreshed qualifying targets after baseline: {refreshed_target_list})"
        )
        return 1
    if latest_failure_epoch is not None and state_real_path is not None and not post_failure_supporting_targets:
        refreshed_target_list = ", ".join(str(path) for path in refreshed_supporting_targets) or "none"
        print(
            "analysis refresh verification failed: no supporting branch-local analysis helper or note shows a refresh newer "
            f"than the latest failed attempt timestamp `{latest_failure_label}` "
            f"(refreshed supporting targets after baseline: {refreshed_target_list})"
        )
        return 1

    verified_json_keys = []
    for spec in args.require_json_key:
        if ":" not in spec:
            print(f"analysis refresh verification failed: malformed require-json-key `{spec}`")
            return 1
        path_text, key = spec.rsplit(":", 1)
        path = Path(path_text)
        try:
            import json

            payload = json.loads(path.read_text())
        except Exception as exc:  # noqa: BLE001
            print(f"analysis refresh verification failed: could not read `{path}` ({exc})")
            return 1
        value = payload.get(key) if isinstance(payload, dict) else None
        if not value:
            print(f"analysis refresh verification failed: `{key}` missing or empty in `{path}`")
            return 1
        verified_json_keys.append(f"{path}:{key}")

    verified_current_state = []
    if args.require_current_state:
        if not args.latest_failure_report or not args.latest_failure_breakdown:
            print(
                "analysis refresh verification failed: require-current-state also needs "
                "--latest-failure-report and --latest-failure-breakdown"
            )
            return 1
        try:
            assert state_path is not None
            assert report_path is not None
            assert breakdown_path is not None
            verified_current_state = verify_current_state(
                state_path,
                report_path,
                breakdown_path,
                recognized_targets=targets,
            )
            refresh_evidence = load_structured_dict(state_path).get("refresh_evidence")
            if not isinstance(refresh_evidence, dict):
                raise ValueError(f"`{state_path}` refresh_evidence is missing or not a JSON object")
            verified_current_state.extend(
                verify_recognized_refresh_targets(
                    state_path,
                    refresh_evidence,
                    targets,
                    baseline_epoch=args.baseline_epoch,
                    latest_failure_epoch=latest_failure_epoch,
                    latest_failure_label=latest_failure_label,
                )
            )
            verified_current_state.extend(
                verify_post_failure_refresh_asset_freshness(
                    state_path,
                    refresh_evidence,
                    latest_failure_epoch,
                    latest_failure_label,
                )
            )
        except Exception as exc:  # noqa: BLE001
            print(f"analysis refresh verification failed: {exc}")
            return 1

    verified_analysis_session = []
    if args.require_analysis_session:
        if not args.latest_failure_report or not args.latest_failure_breakdown:
            print(
                "analysis refresh verification failed: require-analysis-session also needs "
                "--latest-failure-report and --latest-failure-breakdown"
            )
            return 1
        try:
            assert analysis_session_path is not None
            assert report_path is not None
            assert breakdown_path is not None
            verified_analysis_session = verify_analysis_session(
                analysis_session_path,
                report_path,
                breakdown_path,
                state_path,
                latest_failure_epoch,
                latest_failure_label,
            )
        except Exception as exc:  # noqa: BLE001
            print(f"analysis refresh verification failed: {exc}")
            return 1

    verified_localization_refinement = []
    if args.require_localization_narrowing:
        if state_path is None:
            print(
                "analysis refresh verification failed: require-localization-narrowing also needs "
                "--require-current-state"
            )
            return 1
        try:
            verified_localization_refinement = verify_localization_refinement(
                state_path,
                baseline_analysis_session,
            )
        except Exception as exc:  # noqa: BLE001
            print(f"analysis refresh verification failed: {exc}")
            return 1

    print("analysis refresh verified")
    for path in refreshed_qualifying_targets:
        print(path)
    for path in refreshed_targets:
        if path not in refreshed_qualifying_targets:
            print(f"non_qualifying_target:{path}")
    for item in verified_json_keys:
        print(item)
    for item in verified_current_state:
        print(item)
    for item in verified_analysis_session:
        print(item)
    for item in verified_localization_refinement:
        print(item)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
