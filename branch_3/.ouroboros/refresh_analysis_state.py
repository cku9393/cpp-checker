#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from datetime import datetime
from pathlib import Path
from typing import Any


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--attempt", required=True, type=int)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument("--analysis-log", required=True)
    parser.add_argument("--analysis-round", required=True, type=int)
    parser.add_argument("--state-file", required=True)
    parser.add_argument("--iteration-file", required=True)
    return parser.parse_args()


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text()) if path.exists() else {}


def format_attempt_label(value: Any) -> str | None:
    if value is None:
        return None
    if isinstance(value, str) and value.startswith("attempt_"):
        return value
    try:
        return f"attempt_{int(value):03d}"
    except (TypeError, ValueError):
        text = str(value).strip()
        return text or None


def normalize_failed_acs(values: Any) -> list[str]:
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


def build_current_failure_context(
    report_payload: dict[str, Any],
    breakdown_payload: dict[str, Any],
    fallback_attempt: int,
) -> dict[str, Any]:
    attempt_label = (
        format_attempt_label(breakdown_payload.get("attempt"))
        or format_attempt_label(report_payload.get("attempt"))
        or format_attempt_label(fallback_attempt)
        or f"attempt_{fallback_attempt:03d}"
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


def choose_primary_failure(payload: dict[str, Any]) -> dict[str, Any]:
    failures = payload.get("failed_ac_breakdowns") or []
    if failures:
        return failures[0]
    return {}


def infer_paths(failure: dict[str, Any]) -> list[str]:
    paths: list[str] = []
    for item in failure.get("structural_focus") or []:
        path = item.get("path")
        if path and path not in paths:
            paths.append(path)
    return paths[:4]


def infer_symbols(failure: dict[str, Any]) -> list[str]:
    symbols: list[str] = []
    for item in failure.get("structural_focus") or []:
        for symbol in item.get("enclosing_symbols") or []:
            if symbol and symbol not in symbols:
                symbols.append(symbol)
    return symbols[:6]


def merge_unique(items: list[str], new_items: list[str], limit: int = 8) -> list[str]:
    merged: list[str] = []
    for value in [*items, *new_items]:
        if value and value not in merged:
            merged.append(value)
    return merged[:limit]


def summarize_repeat_signals(history_path: Path, primary_axis: str, failure_families: list[str]) -> str:
    if not history_path.exists():
        return "No earlier failure history recorded."
    try:
        history = json.loads(history_path.read_text())
    except Exception:
        return "Failure history exists but could not be parsed."
    axis_hits = 0
    family_hits = 0
    for item in history if isinstance(history, list) else []:
        for failure in item.get("failed_ac_breakdowns") or []:
            if primary_axis and failure.get("primary_axis") == primary_axis:
                axis_hits += 1
            if any(failure.get("failure_family") == fam for fam in failure_families):
                family_hits += 1
    return (
        f"Primary axis `{primary_axis or 'unknown'}` recurred {axis_hits} times; "
        f"current failure families recurred {family_hits} times in prior captured failures."
    )


def build_why_this_axis(failure: dict[str, Any], primary_axis: str, secondary_axis: str | None) -> str:
    failure_family = failure.get("failure_family") or "unknown"
    lane = failure.get("interpretation_lane") or "unknown"
    summary_pivot = failure.get("current_summary_pivot") or "unknown"
    if secondary_axis:
        return (
            f"Selected `{primary_axis}` as the primary progress40 axis because the latest "
            f"`{failure_family}` failure stayed in the `{lane}` lane and the bundled summary "
            f"still names `{summary_pivot}` as the safest next pivot; `{secondary_axis}` remains "
            "a secondary cross-check axis only because the newer evidence narrows work inside "
            "the same pivot instead of proving an unrelated axis shift. Do not broaden into "
            "other progress40 axes unless later solver/runtime/profile evidence contradicts "
            "this baseline."
        )
    return (
        f"Selected `{primary_axis}` as the primary progress40 axis because the latest "
        f"`{failure_family}` failure stayed in the `{lane}` lane and the bundled summary still "
        f"names `{summary_pivot}` as the safest next pivot. Keep this as the only progress40 "
        "axis until later solver/runtime/profile evidence contradicts that baseline rather "
        "than merely adding wrapper, lock, or tooling noise."
    )


def main() -> int:
    args = parse_args()
    attempt_dir = Path(args.attempt_dir)
    report_root = Path(args.report_root)
    state_path = Path(args.state_file)
    iteration_path = Path(args.iteration_file)
    analysis_log = Path(args.analysis_log)
    report_path = attempt_dir / "failure_report.json"
    breakdown_path = attempt_dir / "failure_breakdown.json"
    if not report_path.exists():
        print(f"analysis refresh sync failed: missing `{report_path}`")
        return 1
    if not breakdown_path.exists():
        print(f"analysis refresh sync failed: missing `{breakdown_path}`")
        return 1

    report_payload = load_json(report_path)
    breakdown = load_json(breakdown_path)
    failure = choose_primary_failure(breakdown)
    if not failure:
        print("analysis refresh sync failed: no failed_ac_breakdowns available")
        return 1
    current_failure = build_current_failure_context(report_payload, breakdown, args.attempt)
    if not current_failure.get("session_id"):
        print("analysis refresh sync failed: missing current failure session_id")
        return 1
    if not current_failure.get("timestamp"):
        print("analysis refresh sync failed: missing current failure timestamp")
        return 1

    now = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")
    previous_state = load_json(state_path)
    primary_axis = failure.get("primary_axis") or previous_state.get("pinned_primary_axis") or "zero_span_fastpath"
    secondary_axis = failure.get("secondary_axis") or previous_state.get("pinned_secondary_axis")
    next_probe_command = failure.get("next_probe_command") or previous_state.get("next_probe_command")
    if not next_probe_command:
        print("analysis refresh sync failed: no next_probe_command available")
        return 1

    pinned_paths = infer_paths(failure)
    pinned_symbols = infer_symbols(failure)
    pinned_acs = [failure.get("ac_index")] if failure.get("ac_index") else []
    failure_families = [failure.get("failure_family")] if failure.get("failure_family") else []
    profile_modes = [failure.get("profile_mode")] if failure.get("profile_mode") else []
    why_this_axis = build_why_this_axis(failure, primary_axis, secondary_axis)
    next_narrowing_target = ", ".join(pinned_symbols[:2] or pinned_paths[:2]) or primary_axis
    notes = list(
        previous_state.get("notes")
        or [
            "This file is updated by the mandatory analysis-only mini-session between failed solver attempts.",
            "Use pinned paths and symbols to force narrower structural focus on repeated failures.",
            "Use pinned_primary_axis and pinned_secondary_axis to keep solver retries aligned with the current progress40 pivot.",
        ]
    )
    current_marker_note = (
        "Retry preflight must reject stale analysis unless current_failure_signature matches the latest captured failure report and breakdown."
    )
    if current_marker_note not in notes:
        notes.append(current_marker_note)

    state = {
        "analysis_revision": int(previous_state.get("analysis_revision") or 0) + 1,
        "last_updated": now,
        "last_failed_attempt": current_failure["attempt_label"],
        "last_failed_session_id": current_failure["session_id"],
        "current_for_latest_failure": True,
        "current_failure_attempt": current_failure["attempt_label"],
        "current_failure_session_id": current_failure["session_id"],
        "current_failure_execution_id": current_failure["execution_id"],
        "current_failure_timestamp": current_failure["timestamp"],
        "current_failure_failed_acs": current_failure["failed_acs"],
        "current_failure_signature": current_failure["failure_signature"],
        "current_failure": current_failure,
        "pinned_acs": pinned_acs,
        "pinned_primary_axis": primary_axis,
        "pinned_secondary_axis": secondary_axis,
        "pinned_paths": pinned_paths,
        "pinned_symbols": pinned_symbols,
        "recent_axes": merge_unique(previous_state.get("recent_axes") or [], [primary_axis, secondary_axis or ""]),
        "recent_failure_families": merge_unique(previous_state.get("recent_failure_families") or [], failure_families),
        "recent_profile_modes": merge_unique(previous_state.get("recent_profile_modes") or [], profile_modes),
        "next_probe_command": next_probe_command,
        "why_this_axis": why_this_axis,
        "failure_families": failure_families,
        "next_narrowing_target": next_narrowing_target,
        "notes": notes,
    }
    state_path.write_text(json.dumps(state, indent=2) + "\n")

    history_path = report_root / "failure_history.json"
    iteration_text = "\n".join(
        [
            "# Failure Analysis Iteration Ledger",
            "",
            f"- Timestamp: `{now}`",
            f"- Failed attempt: `{current_failure['attempt_label']}`",
            f"- Analysis round: `{args.analysis_round}`",
            f"- Analysis log: `{analysis_log}`",
            f"- Current for latest failure: `yes`",
            f"- Current failure session: `{current_failure['session_id']}`",
            f"- Current failure execution: `{current_failure['execution_id'] or 'unknown'}`",
            f"- Current failure timestamp: `{current_failure['timestamp']}`",
            f"- Current failure failed ACs: `{', '.join(current_failure['failed_acs']) or 'none'}`",
            f"- Current failure signature: `{current_failure['failure_signature']}`",
            f"- Primary axis: `{primary_axis}`",
            f"- Secondary axis: `{secondary_axis or 'none'}`",
            f"- Pinned ACs: `{', '.join(pinned_acs) or 'none'}`",
            f"- Pinned paths: `{', '.join(pinned_paths) or 'none'}`",
            f"- Pinned symbols: `{', '.join(pinned_symbols) or 'none'}`",
            f"- Failure families: `{', '.join(failure_families) or 'none'}`",
            f"- Next probe command: `{next_probe_command}`",
            f"- Why this axis: `{why_this_axis}`",
            f"- Next narrowing target: `{next_narrowing_target}`",
            "",
            "## Repeat Signal Summary",
            summarize_repeat_signals(history_path, primary_axis, failure_families),
            "",
            "## Refreshed Assets",
            f"- `{state_path}`",
            f"- `{iteration_path}`",
            "",
            "## Retry Gate Requirement",
            "- The next solver retry must stay blocked unless `.ouroboros/failure_analysis_state.json` still carries this exact current-failure signature.",
        ]
    )
    iteration_path.write_text(iteration_text + "\n")

    print(state_path)
    print(iteration_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
