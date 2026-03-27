#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from datetime import datetime
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a soft-stop request for the branch_3 retry loop."
    )
    parser.add_argument("--branch-root", required=True)
    parser.add_argument(
        "--soft-stop-file",
        default=".ouroboros/soft_stop_request.json",
        help="Path relative to branch root or absolute path.",
    )
    parser.add_argument("--trigger", default="manual")
    parser.add_argument("--reason", default="manual_soft_stop")
    parser.add_argument("--limit-kind", default="")
    parser.add_argument("--remaining-percent", type=float)
    parser.add_argument("--note", default="")
    return parser.parse_args()


def resolve_path(branch_root: Path, value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (branch_root / path).resolve()


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()
    soft_stop_file = resolve_path(branch_root, args.soft_stop_file)
    soft_stop_file.parent.mkdir(parents=True, exist_ok=True)

    payload = {
        "requested_at": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
        "trigger": args.trigger,
        "reason": args.reason,
        "limit_kind": args.limit_kind or None,
        "remaining_percent": args.remaining_percent,
        "note": args.note or None,
    }

    soft_stop_file.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(soft_stop_file)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
