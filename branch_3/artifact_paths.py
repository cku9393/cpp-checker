#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path


BRANCH_ROOT = Path(__file__).resolve().parent
ARTIFACTS_ROOT = (BRANCH_ROOT / "artifacts").resolve()

DEFAULT_OUTPUT_SUBPATHS: dict[str, tuple[str, ...]] = {
    "boj28350_smoke": ("boj28350_resume", "smoke"),
    "lca_smoke": ("lca_tree_stress_v5", "smoke"),
    "lca_strong_gate": ("lca_tree_stress_v5", "strong_gate"),
    "lca_rebuttal_gate": ("lca_tree_stress_v5", "rebuttal_gate"),
    "lca_boj3s_gate": ("lca_tree_stress_v5", "boj3s_gate"),
    "lca_hunt": ("lca_tree_stress_v5", "hunt"),
}


def artifacts_root() -> Path:
    return ARTIFACTS_ROOT


def default_output_path(key: str) -> Path:
    try:
        rel_parts = DEFAULT_OUTPUT_SUBPATHS[key]
    except KeyError as exc:
        raise ValueError(f"unknown artifact output key: {key}") from exc
    return _ensure_under_artifacts(ARTIFACTS_ROOT.joinpath(*rel_parts).resolve())


def resolve_output_path(path_like: str | Path | None, *, default_key: str) -> Path:
    if path_like is None or str(path_like) == "":
        return default_output_path(default_key)
    raw = Path(path_like)
    candidate = raw if raw.is_absolute() else ARTIFACTS_ROOT / raw
    return _ensure_under_artifacts(candidate.resolve())


def _ensure_under_artifacts(path: Path) -> Path:
    try:
        path.relative_to(ARTIFACTS_ROOT)
    except ValueError as exc:
        raise ValueError(f"output path must stay under {ARTIFACTS_ROOT}: {path}") from exc
    return path


def main() -> int:
    ap = argparse.ArgumentParser(description="Resolve branch-local artifact output paths.")
    ap.add_argument("key", choices=sorted(DEFAULT_OUTPUT_SUBPATHS))
    ap.add_argument("path", nargs="?", default=None, help="optional output override")
    args = ap.parse_args()

    try:
        print(resolve_output_path(args.path, default_key=args.key))
    except ValueError as exc:
        print(f"[artifact_paths] {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
