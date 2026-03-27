#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path


BRANCH_ROOT = Path(__file__).resolve().parent
DEFAULT_ARTIFACTS_ROOT = (BRANCH_ROOT / "artifacts").resolve()
ARTIFACTS_ROOT = DEFAULT_ARTIFACTS_ROOT
BRANCH_TMP_SUBPATH = ("lca_tree_stress_v5", ".tmp")

DEFAULT_OUTPUT_SUBPATHS: dict[str, tuple[str, ...]] = {
    "boj28350_build": ("boj28350_resume", "build"),
    "boj28350_direct_solver_aux": ("boj28350_resume", "direct_solver_aux"),
    "boj28350_smoke": ("boj28350_resume", "smoke"),
    "branch_certify_suite": ("lca_tree_stress_v5", "certify_suite"),
    "branch_gen_case_aux": ("lca_tree_stress_v5", "gen_case_aux"),
    "branch_run_case": ("lca_tree_stress_v5", "run_case"),
    "lca_smoke": ("lca_tree_stress_v5", "smoke"),
    "lca_smoke_target": ("lca_tree_stress_v5", "smoke_target"),
    "lca_smoke_repeatability": ("lca_tree_stress_v5", "smoke_repeatability"),
    "lca_required_repeatability": ("lca_tree_stress_v5", "required_repeatability"),
    "lca_strong_gate": ("lca_tree_stress_v5", "strong_gate"),
    "lca_rebuttal_gate": ("lca_tree_stress_v5", "rebuttal_gate"),
    "lca_boj3s_gate": ("lca_tree_stress_v5", "boj3s_gate"),
    "lca_hunt": ("lca_tree_stress_v5", "hunt"),
}
ARTIFACT_NAMESPACE_ROOTS = frozenset(parts[0] for parts in DEFAULT_OUTPUT_SUBPATHS.values() if parts)


def artifacts_root() -> Path:
    return ARTIFACTS_ROOT


def ensure_under_artifacts(path_like: str | Path) -> Path:
    return _ensure_under_artifacts(Path(path_like).resolve())


def ensure_resolved_under_artifacts(path_like: str | Path) -> Path:
    return _ensure_under_artifacts(Path(path_like).expanduser().resolve())


def branch_tmp_root() -> Path:
    return _ensure_under_artifacts(ARTIFACTS_ROOT.joinpath(*BRANCH_TMP_SUBPATH).resolve())


def configure_branch_process_env() -> Path:
    purge_non_artifact_bytecode()
    raw_override = os.environ.get("BRANCH_ARTIFACT_TMP_ROOT")
    try:
        tmp_root = resolve_tmp_path(raw_override or None)
    except ValueError as exc:
        raise ValueError(f"invalid BRANCH_ARTIFACT_TMP_ROOT override: {exc}") from exc
    tmp_root.mkdir(parents=True, exist_ok=True)
    os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
    sys.dont_write_bytecode = True
    for key in ("BRANCH_ARTIFACT_TMP_ROOT", "TMPDIR", "TMP", "TEMP"):
        os.environ[key] = str(tmp_root)
    process_env_paths = {
        "HOME": tmp_root / "home",
        "XDG_CONFIG_HOME": tmp_root / "xdg_config",
        "XDG_CACHE_HOME": tmp_root / "xdg_cache",
        "XDG_STATE_HOME": tmp_root / "xdg_state",
        "PYTHONPYCACHEPREFIX": tmp_root / "pycache",
    }
    for key, default_path in process_env_paths.items():
        target = _resolve_existing_artifact_env_path(os.environ.get(key))
        if target is None:
            target = _ensure_under_artifacts(default_path.resolve())
        target.mkdir(parents=True, exist_ok=True)
        os.environ[key] = str(target)
    return tmp_root


def purge_non_artifact_bytecode() -> None:
    for current_root, dirnames, filenames in os.walk(BRANCH_ROOT):
        current_path = Path(current_root)
        if _is_under_artifacts(current_path):
            dirnames[:] = []
            continue

        for dirname in list(dirnames):
            child = current_path / dirname
            if dirname == ".git" or _is_under_artifacts(child):
                dirnames.remove(dirname)
                continue
            if dirname == "__pycache__":
                shutil.rmtree(child, ignore_errors=True)
                dirnames.remove(dirname)

        for filename in filenames:
            if not filename.endswith((".pyc", ".pyo")):
                continue
            try:
                (current_path / filename).unlink()
            except FileNotFoundError:
                continue
            except OSError:
                continue


def default_output_path(key: str) -> Path:
    try:
        rel_parts = DEFAULT_OUTPUT_SUBPATHS[key]
    except KeyError as exc:
        raise ValueError(f"unknown artifact output key: {key}") from exc
    return _ensure_under_artifacts(ARTIFACTS_ROOT.joinpath(*rel_parts).resolve())


def resolve_artifact_path(path_like: str | Path | None, *, default_path: str | Path) -> Path:
    default = _ensure_under_artifacts(Path(default_path).resolve())
    if path_like is None or str(path_like) == "":
        return default
    raw = Path(path_like)
    if raw.is_absolute():
        candidate = raw
    else:
        normalized = _normalize_relative_override(raw)
        if _looks_artifact_rooted(normalized):
            candidate = ARTIFACTS_ROOT / normalized
        else:
            candidate = default / normalized
    return _ensure_under_artifacts(candidate.resolve())


def resolve_output_path(path_like: str | Path | None, *, default_key: str) -> Path:
    return resolve_artifact_path(path_like, default_path=default_output_path(default_key))


def resolve_tmp_path(path_like: str | Path | None) -> Path:
    return resolve_artifact_path(path_like, default_path=branch_tmp_root())


def _normalize_relative_override(raw: Path) -> Path:
    parts = list(raw.parts)
    while parts and parts[0] == ARTIFACTS_ROOT.name:
        parts.pop(0)
    return Path(*parts) if parts else Path()


def _looks_artifact_rooted(path: Path) -> bool:
    if not path.parts:
        return False
    return path.parts[0] in ARTIFACT_NAMESPACE_ROOTS


def _resolve_existing_artifact_env_path(raw_value: str | None) -> Path | None:
    if not raw_value:
        return None
    try:
        return _ensure_under_artifacts(Path(raw_value).expanduser().resolve())
    except (OSError, RuntimeError, ValueError):
        return None


def _is_under_artifacts(path: Path) -> bool:
    try:
        path.resolve().relative_to(ARTIFACTS_ROOT)
    except ValueError:
        return False
    return True


def _ensure_under_artifacts(path: Path) -> Path:
    try:
        path.relative_to(ARTIFACTS_ROOT)
    except ValueError as exc:
        raise ValueError(f"output path must stay under {ARTIFACTS_ROOT}: {path}") from exc
    return path


def main() -> int:
    ap = argparse.ArgumentParser(description="Resolve branch-local artifact output paths.")
    ap.add_argument(
        "--artifacts-root",
        action="store_true",
        help="print the shared branch-local artifacts root and exit",
    )
    ap.add_argument(
        "--ensure",
        default=None,
        help="print the canonical path if it resolves under branch-local artifacts, else fail",
    )
    ap.add_argument("key", nargs="?", choices=sorted(DEFAULT_OUTPUT_SUBPATHS))
    ap.add_argument("path", nargs="?", default=None, help="optional output override")
    args = ap.parse_args()

    if args.artifacts_root:
        if args.ensure is not None or args.key is not None or args.path is not None:
            ap.error("--artifacts-root does not accept other arguments")
        print(artifacts_root())
        return 0
    if args.ensure is not None:
        if args.key is not None or args.path is not None:
            ap.error("--ensure does not accept key/path arguments")
        try:
            print(ensure_resolved_under_artifacts(args.ensure))
        except ValueError as exc:
            print(f"[artifact_paths] {exc}", file=sys.stderr)
            return 2
        return 0
    if args.key is None:
        ap.error("the following arguments are required: key")

    try:
        print(resolve_output_path(args.path, default_key=args.key))
    except ValueError as exc:
        print(f"[artifact_paths] {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
