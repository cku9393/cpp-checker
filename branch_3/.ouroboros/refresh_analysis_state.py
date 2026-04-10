#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True


ITERATION_FAILURE_SIGNATURE_RE = re.compile(
    r"^- Current failure signature: `(?P<value>[^`]+)`\s*$",
    re.MULTILINE,
)
ITERATION_FAILURE_POINT_HEADER_RE = re.compile(r"^(?:\d+\.\s+|-\s+)`(?P<anchor>[^`]+)`\s*$")
ITERATION_FAILURE_POINT_DETAIL_RE = re.compile(
    r"^\s+(?P<label>Statement|Symbol|Evidence|Role):\s*`?(?P<body>.+?)`?\s*$"
)
ITERATION_FAILURE_POINT_ANCHOR_RE = re.compile(
    r"^(?P<path>.+?)(?:::(?P<label>.+?))?\s*\[(?P<focus>\d+-\d+)\]\s*$"
)
PINNED_SYMBOL_RE = re.compile(
    r"^(?P<path>.+?)(?:::(?P<label>.+?))?\s*\[(?P<focus>\d+-\d+)\]\s*$"
)


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


def parse_branch_timestamp(value: Any) -> datetime | None:
    if value is None:
        return None
    text = str(value).strip()
    if len(text) < 19:
        return None
    try:
        return datetime.strptime(text[:19], "%Y-%m-%d %H:%M:%S")
    except ValueError:
        return None


def build_refresh_evidence(
    now: str,
    current_failure: dict[str, Any],
    report_payload: dict[str, Any],
    breakdown_payload: dict[str, Any],
    attempt_dir: Path,
    analysis_log: Path,
    state_path: Path,
    iteration_path: Path,
) -> dict[str, Any]:
    designated_refresh_asset = iteration_path.resolve()
    helper_path = Path(__file__).resolve()
    playbook_path = helper_path.with_name("failure_analysis_playbook.md")
    refresh_dt = parse_branch_timestamp(now)
    report_ts = report_payload.get("timestamp")
    breakdown_ts = breakdown_payload.get("timestamp")
    current_ts = current_failure.get("timestamp")
    report_dt = parse_branch_timestamp(report_ts)
    breakdown_dt = parse_branch_timestamp(breakdown_ts)
    current_dt = parse_branch_timestamp(current_ts)
    return {
        "analysis_refresh_timestamp": now,
        "latest_failure_report_timestamp": report_ts,
        "latest_failure_breakdown_timestamp": breakdown_ts,
        "current_failure_timestamp": current_ts,
        "evidence_source_attempt_dir": str(attempt_dir),
        "analysis_log": str(analysis_log),
        "qualifying_refreshed_assets": [
            str(state_path.resolve()),
            str(helper_path),
            str(playbook_path),
            str(designated_refresh_asset),
        ],
        "freshness_record": {
            "attempt_label": current_failure.get("attempt_label"),
            "session_id": current_failure.get("session_id"),
            "execution_id": current_failure.get("execution_id"),
            "failure_timestamp": current_ts,
            "failure_signature": current_failure.get("failure_signature"),
            "analysis_refresh_timestamp": now,
            "refreshed_asset": str(designated_refresh_asset),
        },
        "refreshed_after_failure_report": bool(refresh_dt and report_dt and refresh_dt >= report_dt),
        "refreshed_after_failure_breakdown": bool(refresh_dt and breakdown_dt and refresh_dt >= breakdown_dt),
        "refreshed_after_current_failure_timestamp": bool(refresh_dt and current_dt and refresh_dt >= current_dt),
    }


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


def previous_state_matches_current_failure(
    previous_state: dict[str, Any],
    current_failure: dict[str, Any],
) -> bool:
    if not previous_state.get("current_for_latest_failure"):
        return False
    previous_signature = str(previous_state.get("current_failure_signature") or "").strip()
    current_signature = str(current_failure.get("failure_signature") or "").strip()
    return bool(previous_signature and previous_signature == current_signature)


def synthesize_current_state_failure(
    previous_state: dict[str, Any],
    current_failure: dict[str, Any],
) -> dict[str, Any]:
    if not previous_state.get("current_for_latest_failure"):
        return {}
    if previous_state.get("current_failure_attempt") != current_failure.get("attempt_label"):
        return {}
    if previous_state.get("current_failure_timestamp") != current_failure.get("timestamp"):
        return {}
    previous_session = previous_state.get("current_failure_session_id")
    current_session = current_failure.get("session_id")
    if current_session and previous_session and previous_session != current_session:
        return {}
    primary_axis = previous_state.get("pinned_primary_axis")
    next_probe_command = previous_state.get("next_probe_command")
    if not primary_axis or not next_probe_command:
        return {}

    pinned_acs = previous_state.get("pinned_acs") or []
    implied_failed_ac = str(pinned_acs[0]).strip() if pinned_acs else None
    lane = "guarded-nominal-pass"
    if implied_failed_ac in {"3", "4"}:
        lane = "correctness-proof"
    elif implied_failed_ac in {"5", "6"}:
        lane = "performance-profile"

    structural_focus: list[dict[str, Any]] = []
    for path in previous_state.get("pinned_paths") or []:
        structural_focus.append({"path": path, "enclosing_symbols": []})
    for symbol in previous_state.get("pinned_symbols") or []:
        structural_focus.append({"path": None, "enclosing_symbols": [symbol]})

    return {
        "title": "Guard-Rejected Nominal PASS",
        "ac_index": implied_failed_ac,
        "failure_type": "guard-rejected-nominal-pass",
        "failure_family": "analysis_guard_rejected_nominal_pass",
        "interpretation_lane": lane,
        "primary_axis": primary_axis,
        "secondary_axis": previous_state.get("pinned_secondary_axis"),
        "profile_mode": None,
        "last_progress_checkpoint_phase": None,
        "last_release_diag_phase": None,
        "next_probe_command": next_probe_command,
        "current_summary_pivot": previous_state.get("next_narrowing_target") or primary_axis,
        "structural_focus": structural_focus,
    }


def choose_primary_failure(payload: dict[str, Any]) -> dict[str, Any]:
    failures = payload.get("failed_ac_breakdowns") or []
    if failures:
        return failures[0]
    fallback = payload.get("fallback_failure_breakdown")
    if isinstance(fallback, dict):
        return fallback
    return {}


def synthesize_non_ac_failure(
    report_payload: dict[str, Any],
    previous_state: dict[str, Any],
) -> dict[str, Any]:
    log_tail = report_payload.get("workflow_log_tail") or []
    if not isinstance(log_tail, list):
        return {}
    text = "\n".join(str(item) for item in log_tail)
    lowered = text.lower()
    if "traceback" not in lowered and "valueerror" not in lowered and "invalid seed format" not in lowered:
        return {}

    failure_family = "retry_loop_pre_ac_exception"
    if "soft_stop_request.json" in lowered and "output path must stay under" in lowered:
        failure_family = "analysis_preflight_artifact_path_guard"
        if ".ouroboros/soft_stop_request.json" in text:
            failure_family = "analysis_preflight_soft_stop_argv_provenance"
    if "invalid seed format" in lowered:
        failure_family = "analysis_preflight_seed_validation"
        if "soft_stop_request.json" in lowered and "output path must stay under" in lowered:
            failure_family = "analysis_preflight_artifact_path_and_seed"
            if ".ouroboros/soft_stop_request.json" in text:
                failure_family = "analysis_preflight_soft_stop_argv_provenance"

    return {
        "title": "Pre-AC Failure: retry-orchestration preflight",
        "failure_type": "orchestration-preflight",
        "failure_family": failure_family,
        "interpretation_lane": "retry-preflight",
        "primary_axis": previous_state.get("pinned_primary_axis") or "zero_span_fastpath",
        "secondary_axis": previous_state.get("pinned_secondary_axis"),
        "profile_mode": None,
        "last_progress_checkpoint_phase": None,
        "last_release_diag_phase": None,
        "next_probe_command": previous_state.get("next_probe_command"),
        "current_summary_pivot": previous_state.get("next_narrowing_target"),
        "structural_focus": [],
    }


def infer_paths(failure: dict[str, Any]) -> list[str]:
    paths: list[str] = []
    for anchor in normalize_retry_statement_anchors(failure, {}):
        path = anchor.get("path")
        if path and path not in paths:
            paths.append(path)
    for item in failure.get("structural_focus") or []:
        path = item.get("path")
        if path and path not in paths:
            paths.append(path)
    return paths[:4]


def infer_symbols(failure: dict[str, Any]) -> list[str]:
    symbols: list[str] = []
    for anchor in normalize_retry_statement_anchors(failure, {}):
        symbol = anchor.get("symbol")
        if symbol and symbol not in symbols:
            symbols.append(symbol)
    for item in failure.get("structural_focus") or []:
        for symbol in item.get("enclosing_symbols") or []:
            if symbol and symbol not in symbols:
                symbols.append(symbol)
    return symbols[:6]


def infer_focus_ac(failure: dict[str, Any]) -> str | None:
    for key in ("ac_index", "focused_ac_index"):
        value = failure.get(key)
        if value is None:
            continue
        text = str(value).strip()
        if text:
            return text
    return None


def merge_unique(items: list[str], new_items: list[str], limit: int = 8) -> list[str]:
    merged: list[str] = []
    for value in [*items, *new_items]:
        if value and value not in merged:
            merged.append(value)
    return merged[:limit]


def markdown_section_lines(text: str, heading: str) -> list[str]:
    lines = text.splitlines()
    section: list[str] = []
    in_section = False
    for line in lines:
        stripped = line.strip()
        if not in_section:
            if stripped == heading:
                in_section = True
            continue
        if stripped.startswith("## "):
            break
        section.append(line)
    return section


def load_existing_iteration_retry_anchors(
    iteration_path: Path,
    current_failure_signature: str | None,
) -> list[dict[str, Any]]:
    if not iteration_path.exists():
        return []
    try:
        text = iteration_path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return []

    if current_failure_signature:
        match = ITERATION_FAILURE_SIGNATURE_RE.search(text)
        if not match or match.group("value").strip() != current_failure_signature:
            return []

    section_lines = markdown_section_lines(text, "## Latest Retry Failure Points")
    if not section_lines:
        return []

    entries: list[dict[str, str]] = []
    current: dict[str, str] | None = None
    for raw_line in section_lines:
        header_match = ITERATION_FAILURE_POINT_HEADER_RE.match(raw_line.strip())
        if header_match:
            if current is not None:
                entries.append(current)
            current = {"anchor": header_match.group("anchor").strip()}
            continue
        if current is None:
            continue
        detail_match = ITERATION_FAILURE_POINT_DETAIL_RE.match(raw_line)
        if detail_match:
            current[detail_match.group("label").lower()] = detail_match.group("body").strip()
    if current is not None:
        entries.append(current)

    anchors: list[dict[str, Any]] = []
    for entry in entries:
        anchor_text = str(entry.get("anchor") or "").strip()
        anchor_match = ITERATION_FAILURE_POINT_ANCHOR_RE.match(anchor_text)
        if not anchor_match:
            continue
        statement = str(entry.get("statement") or "").strip()
        symbol = str(entry.get("symbol") or "").strip()
        evidence = str(entry.get("evidence") or "").strip()
        role = str(entry.get("role") or "").strip()
        anchors.append(
            {
                "path": anchor_match.group("path").strip(),
                "label": (anchor_match.group("label") or f"focus {anchor_match.group('focus')}").strip(),
                "focus_range": anchor_match.group("focus"),
                "symbol": None if symbol.lower() == "none" else (symbol or None),
                "excerpt": None if statement.lower() == "none" else statement,
                "evidence": None if evidence.lower() == "none" else evidence,
                "role": role or "retry-anchor focus",
            }
        )
    return anchors[:8]


def summarize_anchor_excerpt(anchor: dict[str, Any]) -> str | None:
    statement_excerpt = str(anchor.get("statement_excerpt") or "").strip()
    if statement_excerpt:
        return statement_excerpt

    code_excerpt = str(anchor.get("code_excerpt") or "").strip()
    if not code_excerpt:
        return None
    for raw_line in code_excerpt.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if ":" in line:
            _, maybe_code = line.split(":", 1)
            line = maybe_code.strip() or line
        if line:
            return line[:240]
    return None


def infer_anchor_role(anchor: dict[str, Any]) -> str:
    explicit_role = str(anchor.get("role") or "").strip()
    if explicit_role:
        return explicit_role

    label = str(anchor.get("label") or "").lower()
    note = str(anchor.get("note") or "").lower()
    path = str(anchor.get("path") or "")
    if "publication" in label or "persisted" in note:
        return "published timeout record"
    if "row summary" in label or "corroboration" in note:
        return "artifact-side corroboration"
    if "handoff" in label or "timed solver" in label or "ingress" in note or "first live" in note:
        return "launcher-side ingress"
    if "wrapper" in path or "heartbeat" in label or "helper launch" in label:
        return "wrapper trust-boundary corroboration"
    if path.endswith((".cpp", ".cc", ".cxx", ".hpp")):
        return "solver-side primary-axis owner"
    return "retry-anchor focus"


def normalize_retry_statement_anchors(
    failure: dict[str, Any],
    previous_state: dict[str, Any],
    *,
    iteration_path: Path | None = None,
    current_failure_signature: str | None = None,
) -> list[dict[str, Any]]:
    raw_anchors: Any = None
    if iteration_path is not None:
        raw_anchors = load_existing_iteration_retry_anchors(
            iteration_path,
            current_failure_signature,
        )
    if not isinstance(raw_anchors, list) or not raw_anchors:
        raw_anchors = failure.get("retry_critical_anchors")
    if not isinstance(raw_anchors, list) or not raw_anchors:
        raw_anchors = previous_state.get("latest_retry_statement_anchors")
    if not isinstance(raw_anchors, list):
        return []

    anchors: list[dict[str, Any]] = []
    seen: set[tuple[str, str, str]] = set()
    for raw_anchor in raw_anchors:
        if not isinstance(raw_anchor, dict):
            continue
        path = str(raw_anchor.get("path") or "").strip()
        focus_range = str(raw_anchor.get("focus_range") or "").strip()
        label = str(raw_anchor.get("label") or "").strip()
        if not path or not focus_range:
            continue
        key = (path, focus_range, label)
        if key in seen:
            continue
        seen.add(key)
        evidence_lines: list[str] = []
        for raw_line in raw_anchor.get("evidence_lines") or []:
            line = str(raw_line).strip()
            if line and line not in evidence_lines:
                evidence_lines.append(line)
        note = str(raw_anchor.get("note") or "").strip()
        anchors.append(
            {
                "path": path,
                "label": label or f"focus {focus_range}",
                "focus_range": focus_range,
                "symbol": str(raw_anchor.get("symbol") or "").strip() or None,
                "excerpt": summarize_anchor_excerpt(raw_anchor),
                "evidence": evidence_lines[0] if evidence_lines else (note or None),
                "role": infer_anchor_role(raw_anchor),
            }
        )
    return anchors[:8]


def format_anchor_reference(anchor: dict[str, Any]) -> str:
    path = str(anchor.get("path") or "").strip()
    label = str(anchor.get("label") or "").strip()
    focus_range = str(anchor.get("focus_range") or "").strip()
    reference = path
    if label:
        reference += f"::{label}"
    if focus_range:
        reference += f" [{focus_range}]"
    return reference


def resolve_pinned_anchor_path(path_hint: str, pinned_paths: list[str]) -> str:
    text = path_hint.strip()
    if not text:
        return text
    candidate = Path(text)
    if candidate.is_absolute() and candidate.exists():
        return str(candidate)
    for pinned_path in pinned_paths:
        if pinned_path == text or Path(pinned_path).name == text:
            return pinned_path
    return text


def summarize_source_excerpt(path_text: str, focus_range: str) -> str | None:
    path = Path(path_text)
    if not path.exists():
        return None
    try:
        start_text, end_text = focus_range.split("-", 1)
        start = int(start_text)
        end = int(end_text)
    except ValueError:
        return None
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return None
    if start < 1 or end < start or start > len(lines):
        return None
    snippet = " ".join(line.strip() for line in lines[start - 1 : min(end, len(lines))] if line.strip())
    return snippet[:240] if snippet else None


def build_anchors_from_pinned_symbols(
    pinned_symbols: list[str],
    pinned_paths: list[str],
) -> list[dict[str, Any]]:
    anchors: list[dict[str, Any]] = []
    seen: set[tuple[str, str]] = set()
    for pinned_symbol in pinned_symbols:
        match = PINNED_SYMBOL_RE.match(str(pinned_symbol).strip())
        if not match:
            continue
        resolved_path = resolve_pinned_anchor_path(match.group("path"), pinned_paths)
        resolved_candidate = Path(resolved_path)
        if not resolved_candidate.exists():
            continue
        focus_range = match.group("focus")
        key = (resolved_path, focus_range)
        if key in seen:
            continue
        seen.add(key)
        label = (match.group("label") or f"focus {focus_range}").strip()
        anchors.append(
            {
                "path": resolved_path,
                "label": label,
                "focus_range": focus_range,
                "symbol": None,
                "excerpt": summarize_source_excerpt(resolved_path, focus_range),
                "evidence": "promoted from failure_analysis_state.pinned_symbols after stale breakdown anchor carry-forward",
                "role": infer_anchor_role({"path": resolved_path, "label": label}),
            }
        )
    return anchors[:8]


def anchors_need_pinned_symbol_promotion(
    anchors: list[dict[str, Any]],
    pinned_symbol_anchors: list[dict[str, Any]],
) -> bool:
    if not pinned_symbol_anchors:
        return False
    if not anchors:
        return True
    current_keys = {
        (str(anchor.get("path") or "").strip(), str(anchor.get("focus_range") or "").strip())
        for anchor in anchors
        if isinstance(anchor, dict)
    }
    pinned_keys = [
        (str(anchor.get("path") or "").strip(), str(anchor.get("focus_range") or "").strip())
        for anchor in pinned_symbol_anchors
        if isinstance(anchor, dict)
    ]
    required_matches = min(2, len(pinned_keys))
    match_count = sum(1 for key in pinned_keys if key in current_keys)
    return match_count < required_matches


def build_next_narrowing_target(
    anchors: list[dict[str, Any]],
    pinned_symbols: list[str],
    pinned_paths: list[str],
    primary_axis: str,
) -> str:
    if anchors:
        return ", ".join(format_anchor_reference(anchor) for anchor in anchors[:3])
    return ", ".join(pinned_symbols[:2] or pinned_paths[:2]) or primary_axis


def wrapper_command_for_focus_ac(focus_ac: str | None) -> str | None:
    if focus_ac in {"3", "4"}:
        return "./lca_strong_gate.sh"
    if focus_ac in {"5", "6"}:
        return "./lca_boj3s_gate.sh"
    if focus_ac == "2":
        return "./lca_smoke.sh"
    return None


def build_latest_retry_summary(
    failure: dict[str, Any],
    anchors: list[dict[str, Any]],
) -> str:
    focus_ac = infer_focus_ac(failure)
    wrapper_command = wrapper_command_for_focus_ac(focus_ac)
    failure_family = str(failure.get("failure_family") or "unknown failure family").strip()
    lane = str(failure.get("interpretation_lane") or "unknown lane").strip()
    summary_prefix = "The newest retry remains"
    if focus_ac and wrapper_command:
        summary_prefix += f" `AC {focus_ac}` on `{wrapper_command}`"
    elif focus_ac:
        summary_prefix += f" `AC {focus_ac}`"
    else:
        summary_prefix += " the current failure"
    summary_prefix += f" in the `{failure_family}` / `{lane}` lane."

    if not anchors:
        return (
            f"{summary_prefix} The latest retry summary is still broad because no exact "
            "statement-level retry anchors were preserved."
        )

    call_path = " -> ".join(f"`{format_anchor_reference(anchor)}`" for anchor in anchors[:3])
    summary = f"{summary_prefix} The smallest confirmed failing call path is {call_path}."

    certify_rows = failure.get("certify_rows_summary")
    if isinstance(certify_rows, dict):
        bucket_counts = certify_rows.get("bucket_counts") or {}
        timeout_total = bucket_counts.get("timeout")
        solver_rc_counts = certify_rows.get("timeout_solver_rc_counts") or []
        solver_rc_note = ""
        if solver_rc_counts:
            first_solver_rc = solver_rc_counts[0]
            solver_rc = first_solver_rc.get("solver_rc")
            solver_rc_count = first_solver_rc.get("count")
            if solver_rc is not None and solver_rc_count is not None:
                solver_rc_note = f", `solver_rc={solver_rc}` x`{solver_rc_count}`"
        if timeout_total is not None:
            summary += (
                f" Attempt-local `certify_rows.csv` corroborates `timeout={timeout_total}`"
                f"{solver_rc_note}."
            )
        full_plateaus = certify_rows.get("full_lq_timeout_plateaus") or []
        if full_plateaus:
            plateau = full_plateaus[0]
            mode = plateau.get("mode")
            n = plateau.get("n")
            plateau_timeout_total = plateau.get("timeout_total")
            if mode is not None and n is not None:
                summary += (
                    f" The first full `L/Q` timeout plateau is `{mode} n={n}`"
                    f" (`timeout_total={plateau_timeout_total}`)."
                )

    summary += " Wrapper-wide or file-wide rereads stay fallback only."
    return summary


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
    previous_state = load_json(state_path)
    current_failure = build_current_failure_context(report_payload, breakdown, args.attempt)
    failure = choose_primary_failure(breakdown)
    if not failure:
        failure = synthesize_non_ac_failure(report_payload, previous_state)
    if not failure:
        failure = synthesize_current_state_failure(previous_state, current_failure)
    if not failure:
        print(
            "analysis refresh sync failed: no failed_ac_breakdowns, fallback_failure_breakdown, "
            "synthesizable non-AC failure, or current-state nominal-pass fallback available"
        )
        return 1
    failure_type = str(failure.get("failure_type") or "")
    is_non_ac_fallback = failure_type == "orchestration-preflight" or "fallback_failure_breakdown" in breakdown
    if not current_failure.get("session_id") and not is_non_ac_fallback:
        print("analysis refresh sync failed: missing current failure session_id")
        return 1
    if not current_failure.get("timestamp"):
        print("analysis refresh sync failed: missing current failure timestamp")
        return 1

    now = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")
    primary_axis = failure.get("primary_axis") or previous_state.get("pinned_primary_axis") or "zero_span_fastpath"
    secondary_axis = failure.get("secondary_axis") or previous_state.get("pinned_secondary_axis")
    same_failure_overlay = previous_state_matches_current_failure(previous_state, current_failure)
    next_probe_command = (
        previous_state.get("next_probe_command")
        if same_failure_overlay and previous_state.get("next_probe_command")
        else failure.get("next_probe_command") or previous_state.get("next_probe_command")
    )
    if not next_probe_command:
        print("analysis refresh sync failed: no next_probe_command available")
        return 1

    pinned_paths = (
        list(previous_state.get("pinned_paths") or [])
        if same_failure_overlay and previous_state.get("pinned_paths")
        else infer_paths(failure)
    )
    pinned_symbols = (
        list(previous_state.get("pinned_symbols") or [])
        if same_failure_overlay and previous_state.get("pinned_symbols")
        else infer_symbols(failure)
    )
    latest_retry_statement_anchors = (
        normalize_retry_statement_anchors(
            failure,
            previous_state,
            iteration_path=iteration_path,
            current_failure_signature=current_failure.get("failure_signature"),
        )
        or (
            list(previous_state.get("latest_retry_statement_anchors") or [])
            if same_failure_overlay and previous_state.get("latest_retry_statement_anchors")
            else []
        )
    )
    pinned_symbol_anchors = build_anchors_from_pinned_symbols(pinned_symbols, pinned_paths)
    if anchors_need_pinned_symbol_promotion(latest_retry_statement_anchors, pinned_symbol_anchors):
        latest_retry_statement_anchors = pinned_symbol_anchors or latest_retry_statement_anchors
    focus_ac = infer_focus_ac(failure)
    pinned_acs = [focus_ac] if focus_ac else []
    failure_families = [failure.get("failure_family")] if failure.get("failure_family") else []
    profile_modes = [failure.get("profile_mode")] if failure.get("profile_mode") else []
    why_this_axis = (
        str(previous_state.get("why_this_axis") or "").strip()
        if same_failure_overlay and previous_state.get("why_this_axis")
        else build_why_this_axis(failure, primary_axis, secondary_axis)
    )
    latest_retry_summary = build_latest_retry_summary(failure, latest_retry_statement_anchors)
    next_narrowing_target = build_next_narrowing_target(
        latest_retry_statement_anchors,
        pinned_symbols,
        pinned_paths,
        primary_axis,
    )
    baseline_localization = (
        previous_state.get("baseline_localization")
        if same_failure_overlay and isinstance(previous_state.get("baseline_localization"), dict)
        else None
    )
    localization_refinement = (
        previous_state.get("localization_refinement")
        if same_failure_overlay and isinstance(previous_state.get("localization_refinement"), dict)
        else None
    )
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
    refresh_evidence = build_refresh_evidence(
        now,
        current_failure,
        report_payload,
        breakdown,
        attempt_dir,
        analysis_log,
        state_path,
        iteration_path,
    )

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
        "refresh_evidence": refresh_evidence,
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
        "latest_retry_summary": latest_retry_summary,
        "latest_retry_statement_anchors": latest_retry_statement_anchors,
        "latest_retry_anchor_ranges": [
            f"{anchor['path']}:{anchor['focus_range']}"
            for anchor in latest_retry_statement_anchors
        ],
        "baseline_localization": baseline_localization,
        "localization_refinement": localization_refinement,
        "notes": notes,
    }
    state_path.write_text(json.dumps(state, indent=2) + "\n")

    history_path = report_root / "failure_history.json"
    iteration_lines = [
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
        "",
        "## Post-Failure Refresh Evidence",
        f"- Latest failure report timestamp: `{report_payload.get('timestamp') or 'unknown'}`",
        f"- Latest failure breakdown timestamp: `{breakdown.get('timestamp') or 'unknown'}`",
        f"- Analysis refresh timestamp: `{now}`",
        f"- Refreshed after failure report: `{'yes' if refresh_evidence['refreshed_after_failure_report'] else 'no'}`",
        f"- Refreshed after failure breakdown: `{'yes' if refresh_evidence['refreshed_after_failure_breakdown'] else 'no'}`",
        f"- Evidence source attempt dir: `{attempt_dir}`",
        f"- Freshness record asset: `{refresh_evidence['freshness_record']['refreshed_asset']}`",
        f"- Freshness record failure signature: `{refresh_evidence['freshness_record']['failure_signature']}`",
        "",
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
        "## Latest Retry Summary",
        latest_retry_summary,
        "",
    ]
    if latest_retry_statement_anchors:
        iteration_lines.extend(
            [
                "## Narrowed Localization",
                "",
            ]
        )
        for anchor in latest_retry_statement_anchors[:4]:
            iteration_lines.append(f"- `{format_anchor_reference(anchor)}`")
            symbol = str(anchor.get("symbol") or "none").strip()
            if symbol:
                iteration_lines.append(f"  Symbol: `{symbol}`")
            statement = str(anchor.get("excerpt") or "none").strip()
            if statement:
                iteration_lines.append(f"  Statement: `{statement}`")
            evidence = str(anchor.get("evidence") or "none").strip()
            if evidence:
                iteration_lines.append(f"  Why now: `{evidence}`")
        iteration_lines.append("")
    iteration_lines.extend(
        [
        "## Repeat Signal Summary",
        summarize_repeat_signals(history_path, primary_axis, failure_families),
        "",
    ])
    if latest_retry_statement_anchors:
        iteration_lines.append("## Latest Retry Failure Points")
        iteration_lines.append("")
        for index, anchor in enumerate(latest_retry_statement_anchors, start=1):
            iteration_lines.append(f"{index}. `{format_anchor_reference(anchor)}`")
            statement = str(anchor.get("excerpt") or "none").strip()
            symbol = str(anchor.get("symbol") or "none").strip()
            evidence = str(anchor.get("evidence") or "none").strip()
            role = str(anchor.get("role") or "retry-anchor focus").strip()
            iteration_lines.append(f"   Statement: `{statement}`")
            iteration_lines.append(f"   Symbol: `{symbol}`")
            iteration_lines.append(f"   Evidence: `{evidence}`")
            iteration_lines.append(f"   Role: `{role}`")
        iteration_lines.append("")
    iteration_lines.extend(
        [
            "## Refreshed Assets",
            f"- `{state_path}`",
            f"- `{iteration_path}`",
            "",
            "## Retry Gate Requirement",
            "- The next solver retry must stay blocked unless `.ouroboros/failure_analysis_state.json` still carries this exact current-failure signature.",
            "- The next solver retry must also stay blocked unless `refresh_evidence.freshness_record.refreshed_asset` itself is a supporting analysis asset newer than the latest failure timestamp; another file cannot satisfy freshness on its behalf.",
        ]
    )
    iteration_text = "\n".join(iteration_lines)
    iteration_path.write_text(iteration_text + "\n")

    print(state_path)
    print(iteration_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
