#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import datetime
from pathlib import Path

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from retry_artifact_io import prepare_output_dir, write_text_output


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a soft-stop request for the branch_3 retry loop."
    )
    parser.add_argument("--branch-root", required=True)
    parser.add_argument(
        "--soft-stop-file",
        default="artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json",
        help="Path relative to branch root or absolute path.",
    )
    parser.add_argument("--trigger", default="manual")
    parser.add_argument("--reason", default="manual_soft_stop")
    parser.add_argument("--limit-kind", default="")
    parser.add_argument("--remaining-percent", type=float)
    parser.add_argument("--note", default="")
    return parser.parse_args()


def _load_artifact_guard(branch_root: Path):
    sys.path.insert(0, str(branch_root))
    from artifact_paths import ensure_under_artifacts  # type: ignore

    return ensure_under_artifacts


def resolve_artifact_path(branch_root: Path, ensure_under_artifacts, value: str) -> Path:
    path = Path(value).expanduser()
    resolved = path if path.is_absolute() else (branch_root / path).resolve()
    return ensure_under_artifacts(resolved)


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()
    ensure_under_artifacts = _load_artifact_guard(branch_root)
    soft_stop_file = resolve_artifact_path(
        branch_root,
        ensure_under_artifacts,
        args.soft_stop_file,
    )
    prepare_output_dir(soft_stop_file.parent)

    payload = {
        "requested_at": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
        "trigger": args.trigger,
        "reason": args.reason,
        "limit_kind": args.limit_kind or None,
        "remaining_percent": args.remaining_percent,
        "note": args.note or None,
    }

    write_text_output(soft_stop_file, json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(soft_stop_file)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
