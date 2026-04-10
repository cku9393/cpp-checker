#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from retry_artifact_io import copy_output_file, prepare_output_dir, write_text_output


LATEST_SNAPSHOT_NAMES = {
    "solver_seed": "latest_solver_seed.snapshot",
    "analysis_seed": "latest_analysis_seed.snapshot",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Snapshot retry-loop seed inputs into branch-local artifacts for reproducible reruns."
    )
    parser.add_argument("--branch-root", required=True)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument("--seed-file", required=True)
    parser.add_argument("--analysis-seed-file", default="")
    parser.add_argument("--attempt-number", type=int)
    return parser.parse_args()


def _load_artifact_guard(branch_root: Path):
    sys.path.insert(0, str(branch_root))
    from artifact_paths import ensure_under_artifacts  # type: ignore

    return ensure_under_artifacts


def resolve_branch_path(branch_root: Path, value: str) -> Path:
    path = Path(value).expanduser()
    resolved = path if path.is_absolute() else (branch_root / path).resolve()
    try:
        resolved.relative_to(branch_root)
    except ValueError as exc:
        raise ValueError(f"seed input must stay under {branch_root}: {resolved}") from exc
    return resolved


def resolve_artifact_path(branch_root: Path, ensure_under_artifacts, value: str) -> Path:
    path = Path(value).expanduser()
    resolved = path if path.is_absolute() else (branch_root / path).resolve()
    return ensure_under_artifacts(resolved)


def describe_file(path: Path) -> dict[str, Any]:
    data = path.read_bytes()
    return {
        "source_path": str(path),
        "size_bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def snapshot_suffix_for(source_path: Path) -> str:
    return "".join(source_path.suffixes)


def remove_snapshot_family_variants(target_path: Path, *, family_prefix: str) -> None:
    for candidate in sorted(target_path.parent.glob(f"{family_prefix}*"), key=lambda path: path.name):
        if candidate.resolve() == target_path.resolve():
            continue
        if candidate.is_dir() and not candidate.is_symlink():
            shutil.rmtree(candidate, ignore_errors=True)
            continue
        try:
            candidate.unlink()
        except FileNotFoundError:
            continue
        except IsADirectoryError:
            shutil.rmtree(candidate, ignore_errors=True)


def snapshot_one(
    *,
    label: str,
    source_path: Path,
    attempt_dir: Path,
    report_root: Path,
    ensure_under_artifacts,
) -> dict[str, Any]:
    suffix = snapshot_suffix_for(source_path)
    attempt_snapshot = ensure_under_artifacts((attempt_dir / f"{label}.snapshot{suffix}").resolve())
    latest_name = LATEST_SNAPSHOT_NAMES.get(label, f"latest_{label}.snapshot")
    latest_snapshot = ensure_under_artifacts((report_root / f"{latest_name}{suffix}").resolve())
    prepare_output_dir(attempt_snapshot.parent)
    prepare_output_dir(latest_snapshot.parent)
    remove_snapshot_family_variants(attempt_snapshot, family_prefix=f"{label}.snapshot")
    remove_snapshot_family_variants(latest_snapshot, family_prefix=latest_name)

    copy_output_file(source_path, attempt_snapshot)
    copy_output_file(source_path, latest_snapshot)

    payload = describe_file(source_path)
    payload.update(
        {
            "attempt_snapshot": str(attempt_snapshot),
            "latest_snapshot": str(latest_snapshot),
        }
    )
    return payload


def render_markdown(payload: dict[str, Any]) -> str:
    lines = [
        "# Retry Input Snapshot",
        "",
        f"- Captured at: `{payload['captured_at']}`",
        f"- Attempt number: `{payload.get('attempt_number') or 'unknown'}`",
        f"- Attempt dir: `{payload['attempt_dir']}`",
        f"- Report root: `{payload['report_root']}`",
        "",
        "## Inputs",
        "",
    ]
    for label, entry in payload["inputs"].items():
        lines.extend(
            [
                f"### {label}",
                "",
                f"- Source path: `{entry['source_path']}`",
                f"- Size bytes: `{entry['size_bytes']}`",
                f"- SHA256: `{entry['sha256']}`",
                f"- Attempt snapshot: `{entry['attempt_snapshot']}`",
                f"- Latest snapshot: `{entry['latest_snapshot']}`",
                "",
            ]
        )
    return "\n".join(lines) + "\n"


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()
    ensure_under_artifacts = _load_artifact_guard(branch_root)
    attempt_dir = resolve_artifact_path(branch_root, ensure_under_artifacts, args.attempt_dir)
    report_root = resolve_artifact_path(branch_root, ensure_under_artifacts, args.report_root)
    prepare_output_dir(attempt_dir)
    prepare_output_dir(report_root)

    seed_sources: list[tuple[str, Path]] = [("solver_seed", resolve_branch_path(branch_root, args.seed_file))]
    if args.analysis_seed_file:
        seed_sources.append(("analysis_seed", resolve_branch_path(branch_root, args.analysis_seed_file)))

    for _, source_path in seed_sources:
        if not source_path.is_file():
            raise FileNotFoundError(f"retry input snapshot source is missing: {source_path}")

    payload = {
        "captured_at": datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z"),
        "attempt_number": args.attempt_number,
        "attempt_dir": str(attempt_dir),
        "report_root": str(report_root),
        "inputs": {},
    }
    for label, source_path in seed_sources:
        payload["inputs"][label] = snapshot_one(
            label=label,
            source_path=source_path,
            attempt_dir=attempt_dir,
            report_root=report_root,
            ensure_under_artifacts=ensure_under_artifacts,
        )

    attempt_json = attempt_dir / "retry_inputs_snapshot.json"
    attempt_md = attempt_dir / "retry_inputs_snapshot.md"
    latest_json = report_root / "latest_retry_inputs_snapshot.json"
    latest_md = report_root / "latest_retry_inputs_snapshot.md"
    write_text_output(attempt_json, json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    write_text_output(attempt_md, render_markdown(payload), encoding="utf-8")
    write_text_output(latest_json, attempt_json.read_text(encoding="utf-8"), encoding="utf-8")
    write_text_output(latest_md, attempt_md.read_text(encoding="utf-8"), encoding="utf-8")

    print(attempt_json)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
