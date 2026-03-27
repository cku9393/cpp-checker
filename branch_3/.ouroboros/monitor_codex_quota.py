#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import time
from datetime import datetime
from pathlib import Path
from typing import Any


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Watch Codex quota usage and request a soft stop when thresholds are crossed."
    )
    parser.add_argument("--branch-root", required=True)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument(
        "--soft-stop-file",
        default=".ouroboros/soft_stop_request.json",
        help="Path relative to branch root or absolute path.",
    )
    parser.add_argument(
        "--codex-sessions-root",
        default="~/.codex/sessions",
        help="Codex session log root.",
    )
    parser.add_argument(
        "--auth-file",
        default="~/.codex/auth.json",
        help="Codex auth file used to detect account refresh/account switching.",
    )
    parser.add_argument("--poll-seconds", type=int, default=2)
    parser.add_argument("--primary-remaining-threshold", type=float, default=1.0)
    parser.add_argument("--secondary-remaining-threshold", type=float, default=1.0)
    parser.add_argument("--once", action="store_true")
    return parser.parse_args()


def resolve_path(branch_root: Path, value: str) -> Path:
    path = Path(value).expanduser()
    return path if path.is_absolute() else (branch_root / path).resolve()


def parse_iso8601_timestamp(value: str | None) -> float | None:
    if not value:
        return None
    normalized = value.replace("Z", "+00:00")
    try:
        return datetime.fromisoformat(normalized).timestamp()
    except ValueError:
        return None


def load_auth_refresh_epoch(auth_file: Path) -> float | None:
    if not auth_file.exists():
        return None
    try:
        payload = json.loads(auth_file.read_text(encoding="utf-8"))
    except Exception:  # noqa: BLE001
        return None
    refresh_value = payload.get("last_refresh")
    return parse_iso8601_timestamp(refresh_value)


def read_tail_lines(path: Path, max_bytes: int = 524288) -> list[str]:
    try:
        with path.open("rb") as handle:
            handle.seek(0, 2)
            size = handle.tell()
            handle.seek(max(0, size - max_bytes))
            raw = handle.read()
    except OSError:
        return []
    return raw.decode("utf-8", errors="replace").splitlines()


def parse_session_meta(path: Path) -> dict[str, Any] | None:
    try:
        first_line = path.read_text(encoding="utf-8", errors="replace").splitlines()[0]
        payload = json.loads(first_line)
    except Exception:  # noqa: BLE001
        return None
    if payload.get("type") != "session_meta":
        return None
    meta = payload.get("payload")
    return meta if isinstance(meta, dict) else None


def load_current_auth_context(auth_file: Path) -> dict[str, Any]:
    if not auth_file.exists():
        return {}
    try:
        payload = json.loads(auth_file.read_text(encoding="utf-8"))
    except Exception:  # noqa: BLE001
        return {}
    tokens = payload.get("tokens")
    return {
        "auth_mode": payload.get("auth_mode"),
        "last_refresh": payload.get("last_refresh"),
        "account_id": tokens.get("account_id") if isinstance(tokens, dict) else None,
    }


def candidate_matches_branch_root(meta: dict[str, Any] | None, branch_root: Path) -> bool:
    if not meta:
        return False
    if meta.get("cwd") != str(branch_root):
        return False
    return meta.get("source") == "exec"


def parse_recent_rate_limit_events(path: Path, limit: int = 6) -> list[dict[str, Any]]:
    events: list[dict[str, Any]] = []
    for line in reversed(read_tail_lines(path)):
        if '"type":"token_count"' not in line and '"type": "token_count"' not in line:
            continue
        try:
            record = json.loads(line)
        except json.JSONDecodeError:
            continue
        payload = record.get("payload", {})
        rate_limits = payload.get("rate_limits")
        if not isinstance(rate_limits, dict):
            continue
        limit_id = rate_limits.get("limit_id")
        if limit_id and limit_id != "codex":
            continue
        primary = rate_limits.get("primary") or {}
        secondary = rate_limits.get("secondary") or {}
        if primary.get("window_minutes") != 300 or secondary.get("window_minutes") != 10080:
            continue
        events.append(
            {
                "session_log": str(path),
                "session_log_mtime": datetime.fromtimestamp(path.stat().st_mtime).astimezone().strftime(
                    "%Y-%m-%d %H:%M:%S %Z"
                ),
                "session_started_at": parse_session_meta(path).get("timestamp"),
                "event_timestamp": record.get("timestamp"),
                "rate_limits": rate_limits,
            }
        )
        if len(events) >= limit:
            break
    return list(reversed(events))


def load_latest_rate_limits(
    sessions_root: Path,
    branch_root: Path,
    *,
    min_session_start_epoch: float | None = None,
) -> list[dict[str, Any]] | None:
    candidates = sorted(
        sessions_root.rglob("*.jsonl"),
        key=lambda item: item.stat().st_mtime,
        reverse=True,
    )
    merged_events: list[dict[str, Any]] = []
    considered_files = 0
    for candidate in candidates[:64]:
        meta = parse_session_meta(candidate)
        if not candidate_matches_branch_root(meta, branch_root):
            continue
        if min_session_start_epoch is not None:
            session_started_epoch = parse_iso8601_timestamp(meta.get("timestamp"))
            if session_started_epoch is None or session_started_epoch < min_session_start_epoch:
                continue
        events = parse_recent_rate_limit_events(candidate)
        if not events:
            continue
        merged_events.extend(events)
        considered_files += 1
        if considered_files >= 8:
            break
    if not merged_events:
        return None
    merged_events.sort(key=lambda item: item.get("event_timestamp") or item["session_log_mtime"])
    return merged_events[-6:]


def effective_trigger_state(
    remaining_percent: float,
    threshold_percent: float,
    last_step_percent: float,
) -> tuple[bool, str, float]:
    if remaining_percent <= threshold_percent:
        return True, "exact_threshold", 0.0
    guard_band = 0.0
    if last_step_percent > 0:
        # Keep the predictive stop tighter than the raw step size so the loop
        # pauses closer to 1%, while still avoiding common 0% overshoots.
        guard_band = max(0.5, min(2.0, round(last_step_percent / 3.0, 3)))
        if remaining_percent <= threshold_percent + guard_band:
            return True, "projected_overshoot_guard", guard_band
    return False, "none", guard_band


def build_status(
    events: list[dict[str, Any]],
    primary_threshold: float,
    secondary_threshold: float,
    *,
    auth_context: dict[str, Any] | None = None,
) -> dict[str, Any]:
    payload = events[-1]
    previous = events[-2] if len(events) >= 2 else None
    rate_limits = payload["rate_limits"]
    primary_used = float(rate_limits["primary"].get("used_percent", 0.0))
    secondary_used = float(rate_limits["secondary"].get("used_percent", 0.0))
    primary_remaining = max(0.0, round(100.0 - primary_used, 3))
    secondary_remaining = max(0.0, round(100.0 - secondary_used, 3))
    previous_primary_remaining = None
    previous_secondary_remaining = None
    if previous is not None:
        previous_primary_remaining = max(0.0, round(100.0 - float(previous["rate_limits"]["primary"].get("used_percent", 0.0)), 3))
        previous_secondary_remaining = max(0.0, round(100.0 - float(previous["rate_limits"]["secondary"].get("used_percent", 0.0)), 3))
    primary_last_step = (
        max(0.0, round((previous_primary_remaining or primary_remaining) - primary_remaining, 3))
        if previous_primary_remaining is not None
        else 0.0
    )
    secondary_last_step = (
        max(0.0, round((previous_secondary_remaining or secondary_remaining) - secondary_remaining, 3))
        if previous_secondary_remaining is not None
        else 0.0
    )
    primary_triggered, primary_trigger_mode, primary_guard_band = effective_trigger_state(
        remaining_percent=primary_remaining,
        threshold_percent=primary_threshold,
        last_step_percent=primary_last_step,
    )
    secondary_triggered, secondary_trigger_mode, secondary_guard_band = effective_trigger_state(
        remaining_percent=secondary_remaining,
        threshold_percent=secondary_threshold,
        last_step_percent=secondary_last_step,
    )

    triggered_limits: list[str] = []
    if primary_triggered:
        triggered_limits.append("primary_5h")
    if secondary_triggered:
        triggered_limits.append("secondary_1w")

    return {
        "captured_at": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
        "selection_mode": "auth_refresh_scoped_current_account",
        "auth_context": auth_context or {},
        "session_log": payload["session_log"],
        "session_log_mtime": payload["session_log_mtime"],
        "session_started_at": payload.get("session_started_at"),
        "event_timestamp": payload.get("event_timestamp"),
        "plan_type": rate_limits.get("plan_type"),
        "events_considered": len(events),
        "primary": {
            "window_minutes": rate_limits["primary"].get("window_minutes"),
            "used_percent": primary_used,
            "remaining_percent": primary_remaining,
            "previous_remaining_percent": previous_primary_remaining,
            "last_step_percent": primary_last_step,
            "guard_band_percent": primary_guard_band,
            "threshold_percent": primary_threshold,
            "resets_at": rate_limits["primary"].get("resets_at"),
            "triggered": primary_triggered,
            "trigger_mode": primary_trigger_mode,
        },
        "secondary": {
            "window_minutes": rate_limits["secondary"].get("window_minutes"),
            "used_percent": secondary_used,
            "remaining_percent": secondary_remaining,
            "previous_remaining_percent": previous_secondary_remaining,
            "last_step_percent": secondary_last_step,
            "guard_band_percent": secondary_guard_band,
            "threshold_percent": secondary_threshold,
            "resets_at": rate_limits["secondary"].get("resets_at"),
            "triggered": secondary_triggered,
            "trigger_mode": secondary_trigger_mode,
        },
        "triggered_limits": triggered_limits,
    }


def write_status(attempt_dir: Path, report_root: Path, payload: dict[str, Any]) -> None:
    report_json = attempt_dir / "quota_watch_status.json"
    report_md = attempt_dir / "quota_watch_status.md"
    latest_json = report_root / "latest_quota_watch_status.json"
    latest_md = report_root / "latest_quota_watch_status.md"

    report_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    report_md.write_text(
        "\n".join(
            [
                "# Quota Watch Status",
                "",
                f"- Captured at: `{payload['captured_at']}`",
                f"- Selection mode: `{payload.get('selection_mode') or 'unknown'}`",
                f"- Current auth account id: `{(payload.get('auth_context') or {}).get('account_id') or 'unknown'}`",
                f"- Current auth last refresh: `{(payload.get('auth_context') or {}).get('last_refresh') or 'unknown'}`",
                f"- Session log: `{payload['session_log']}`",
                f"- Session log mtime: `{payload['session_log_mtime']}`",
                f"- Session started at: `{payload.get('session_started_at') or 'unknown'}`",
                f"- Event timestamp: `{payload.get('event_timestamp') or 'unknown'}`",
                f"- Plan type: `{payload.get('plan_type') or 'unknown'}`",
                "",
                "## Primary 5h Window",
                f"- Used percent: `{payload['primary']['used_percent']}`",
                f"- Remaining percent: `{payload['primary']['remaining_percent']}`",
                f"- Previous remaining percent: `{payload['primary']['previous_remaining_percent']}`",
                f"- Last step percent: `{payload['primary']['last_step_percent']}`",
                f"- Guard band percent: `{payload['primary']['guard_band_percent']}`",
                f"- Threshold percent: `{payload['primary']['threshold_percent']}`",
                f"- Triggered: `{'yes' if payload['primary']['triggered'] else 'no'}`",
                f"- Trigger mode: `{payload['primary']['trigger_mode']}`",
                "",
                "## Secondary 1w Window",
                f"- Used percent: `{payload['secondary']['used_percent']}`",
                f"- Remaining percent: `{payload['secondary']['remaining_percent']}`",
                f"- Previous remaining percent: `{payload['secondary']['previous_remaining_percent']}`",
                f"- Last step percent: `{payload['secondary']['last_step_percent']}`",
                f"- Guard band percent: `{payload['secondary']['guard_band_percent']}`",
                f"- Threshold percent: `{payload['secondary']['threshold_percent']}`",
                f"- Triggered: `{'yes' if payload['secondary']['triggered'] else 'no'}`",
                f"- Trigger mode: `{payload['secondary']['trigger_mode']}`",
                "",
                f"- Triggered limits: `{', '.join(payload['triggered_limits']) or 'none'}`",
                "",
            ]
        )
        + "\n",
        encoding="utf-8",
    )
    latest_json.write_text(report_json.read_text(encoding="utf-8"), encoding="utf-8")
    latest_md.write_text(report_md.read_text(encoding="utf-8"), encoding="utf-8")


def write_soft_stop_request(soft_stop_file: Path, payload: dict[str, Any]) -> None:
    request_payload = {
        "requested_at": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
        "trigger": "quota_threshold",
        "reason": "codex_quota_remaining_at_or_below_threshold",
        "triggered_limits": payload["triggered_limits"],
        "primary_remaining_percent": payload["primary"]["remaining_percent"],
        "primary_trigger_mode": payload["primary"]["trigger_mode"],
        "primary_last_step_percent": payload["primary"]["last_step_percent"],
        "primary_guard_band_percent": payload["primary"]["guard_band_percent"],
        "secondary_remaining_percent": payload["secondary"]["remaining_percent"],
        "secondary_trigger_mode": payload["secondary"]["trigger_mode"],
        "secondary_last_step_percent": payload["secondary"]["last_step_percent"],
        "secondary_guard_band_percent": payload["secondary"]["guard_band_percent"],
        "primary_threshold_percent": payload["primary"]["threshold_percent"],
        "secondary_threshold_percent": payload["secondary"]["threshold_percent"],
        "primary_resets_at": payload["primary"].get("resets_at"),
        "secondary_resets_at": payload["secondary"].get("resets_at"),
        "session_log": payload["session_log"],
        "event_timestamp": payload.get("event_timestamp"),
    }
    soft_stop_file.parent.mkdir(parents=True, exist_ok=True)
    soft_stop_file.write_text(json.dumps(request_payload, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()
    attempt_dir = Path(args.attempt_dir).resolve()
    report_root = Path(args.report_root).resolve()
    soft_stop_file = resolve_path(branch_root, args.soft_stop_file)
    sessions_root = Path(args.codex_sessions_root).expanduser().resolve()
    auth_file = Path(args.auth_file).expanduser().resolve()

    attempt_dir.mkdir(parents=True, exist_ok=True)
    report_root.mkdir(parents=True, exist_ok=True)

    while True:
        auth_context = load_current_auth_context(auth_file)
        auth_refresh_epoch = parse_iso8601_timestamp(auth_context.get("last_refresh"))
        events = load_latest_rate_limits(
            sessions_root,
            branch_root,
            min_session_start_epoch=auth_refresh_epoch,
        )
        if events:
            status = build_status(
                events,
                primary_threshold=args.primary_remaining_threshold,
                secondary_threshold=args.secondary_remaining_threshold,
                auth_context=auth_context,
            )
            write_status(attempt_dir, report_root, status)
            if status["triggered_limits"] and not soft_stop_file.exists():
                write_soft_stop_request(soft_stop_file, status)
                print(
                    f"soft stop requested: {','.join(status['triggered_limits'])} "
                    f"(primary_remaining={status['primary']['remaining_percent']}, "
                    f"secondary_remaining={status['secondary']['remaining_percent']})"
                )
                return 0
        if args.once:
            return 0 if events else 1
        time.sleep(max(1, args.poll_seconds))


if __name__ == "__main__":
    raise SystemExit(main())
