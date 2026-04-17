#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import stat
import shutil
import sys
from pathlib import Path

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True


BRANCH_ROOT = Path(__file__).resolve().parent
DEFAULT_ARTIFACTS_ROOT = (BRANCH_ROOT / "artifacts").resolve()
ARTIFACTS_ROOT = DEFAULT_ARTIFACTS_ROOT
BRANCH_TMP_SUBPATH = ("lca_tree_stress_v5", ".tmp")
NON_ARTIFACT_TREE_STATE_SCHEMA = "branch_non_artifact_tree_state_v1"
NON_ARTIFACT_TREE_VERIFY_ESCAPE_EXIT = 3
NON_ARTIFACT_SCAN_IGNORED_ROOTS = frozenset({".git"})
PROCESS_STATE_SUBPATH = (".process_state",)
NON_ARTIFACT_PURGE_ENV = "BRANCH_NON_ARTIFACT_BYTECODE_PURGED"
NON_ARTIFACT_HASH_MAX_BYTES = int(os.environ.get("BRANCH_NON_ARTIFACT_HASH_MAX_BYTES", "0") or "0")
NON_ARTIFACT_CREATED_SOURCE_WARNING_SUFFIXES = frozenset(
    {
        ".c",
        ".cc",
        ".cpp",
        ".h",
        ".hh",
        ".hpp",
        ".ini",
        ".json",
        ".md",
        ".py",
        ".sh",
        ".toml",
        ".yaml",
        ".yml",
    }
)
NON_ARTIFACT_CREATED_BLOCKING_DIRS = frozenset(
    {
        ".pytest_cache",
        "__pycache__",
        ".mypy_cache",
        ".ruff_cache",
    }
)
NON_ARTIFACT_CREATED_OUROBOROS_LOG_PREFIXES = (
    "analysis_refresh_attempt_",
)

DEFAULT_OUTPUT_SUBPATHS: dict[str, tuple[str, ...]] = {
    "boj28350_build": ("boj28350_resume", "build"),
    "boj28350_direct_solver_aux": ("boj28350_resume", "direct_solver_aux"),
    "boj28350_smoke": ("boj28350_resume", "smoke"),
    "branch_certify_suite": ("lca_tree_stress_v5", "certify_suite"),
    "branch_outer_certify": ("lca_tree_stress_v5", "outer_certify"),
    "branch_gen_case_aux": ("lca_tree_stress_v5", "gen_case_aux"),
    "branch_run_case": ("lca_tree_stress_v5", "run_case"),
    "lca_smoke": ("lca_tree_stress_v5", "smoke"),
    "lca_smoke_target": ("lca_tree_stress_v5", "smoke_target"),
    "lca_smoke_repeatability": ("lca_tree_stress_v5", "smoke_repeatability"),
    "lca_acceptance_repeatability": ("lca_tree_stress_v5", "acceptance_repeatability"),
    "lca_required_repeatability": ("lca_tree_stress_v5", "required_repeatability"),
    "lca_strong_gate": ("lca_tree_stress_v5", "strong_gate"),
    "lca_strong_gate_stage_filter": ("lca_tree_stress_v5", "strong_gate_stage_filter"),
    "lca_rebuttal_gate": ("lca_tree_stress_v5", "rebuttal_gate"),
    "lca_boj3s_gate": ("lca_tree_stress_v5", "boj3s_gate"),
    "lca_boj3s_gate_stage_filter": ("lca_tree_stress_v5", "boj3s_gate_stage_filter"),
    "lca_hunt": ("lca_tree_stress_v5", "hunt"),
}
ARTIFACT_NAMESPACE_ROOTS = frozenset(parts[0] for parts in DEFAULT_OUTPUT_SUBPATHS.values() if parts)


def artifacts_root() -> Path:
    return ARTIFACTS_ROOT


def ensure_under_artifacts(path_like: str | Path) -> Path:
    return _ensure_under_artifacts(Path(path_like).resolve())


def ensure_resolved_under_artifacts(path_like: str | Path) -> Path:
    return _ensure_under_artifacts(Path(path_like).expanduser().resolve())


def resolve_branch_artifact_path(path_like: str | Path) -> Path:
    raw = Path(path_like).expanduser()
    if raw.is_absolute():
        candidate = raw
    else:
        normalized = _normalize_relative_override(raw)
        if _looks_artifact_rooted(normalized):
            candidate = ARTIFACTS_ROOT / normalized
        else:
            candidate = BRANCH_ROOT / normalized
    return _ensure_under_artifacts(candidate.resolve())


def branch_tmp_root() -> Path:
    return _ensure_under_artifacts(ARTIFACTS_ROOT.joinpath(*BRANCH_TMP_SUBPATH).resolve())


def configure_branch_process_env() -> Path:
    if os.environ.get(NON_ARTIFACT_PURGE_ENV) != "1":
        purge_non_artifact_bytecode()
        os.environ[NON_ARTIFACT_PURGE_ENV] = "1"
    raw_override = os.environ.get("BRANCH_ARTIFACT_TMP_ROOT")
    try:
        tmp_root = resolve_tmp_path(raw_override or None)
    except ValueError as exc:
        raise ValueError(f"invalid BRANCH_ARTIFACT_TMP_ROOT override: {exc}") from exc
    _ensure_artifact_process_dir(tmp_root)
    os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
    sys.dont_write_bytecode = True
    for key in ("BRANCH_ARTIFACT_TMP_ROOT", "TMPDIR", "TMP", "TEMP"):
        os.environ[key] = str(tmp_root)
    if raw_override:
        process_env_paths = {
            "HOME": tmp_root / "home",
            "XDG_CONFIG_HOME": tmp_root / "xdg_config",
            "XDG_CACHE_HOME": tmp_root / "xdg_cache",
            "XDG_STATE_HOME": tmp_root / "xdg_state",
            "PYTHONPYCACHEPREFIX": tmp_root / "pycache",
        }
    else:
        state_root = _default_process_state_root(tmp_root)
        _reset_path(state_root)
        process_env_paths = {
            "HOME": state_root / "home",
            "XDG_CONFIG_HOME": state_root / "xdg_config",
            "XDG_CACHE_HOME": state_root / "xdg_cache",
            "XDG_STATE_HOME": state_root / "xdg_state",
            "PYTHONPYCACHEPREFIX": state_root / "pycache",
        }
    for key, default_path in process_env_paths.items():
        target = _resolve_existing_artifact_env_path(os.environ.get(key)) if raw_override else None
        if target is None:
            target = _ensure_under_artifacts(default_path.resolve())
        _ensure_artifact_process_dir(target)
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


def _default_process_state_root(tmp_root: Path) -> Path:
    return _ensure_under_artifacts(
        tmp_root.joinpath(*PROCESS_STATE_SUBPATH, _process_state_namespace()).resolve()
    )


def _process_state_namespace() -> str:
    raw = Path(sys.argv[0] or "python").stem or "python"
    sanitized = "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in raw).strip("._-")
    return sanitized or "python"


def _normalize_relative_override(raw: Path) -> Path:
    parts = [part for part in raw.parts if part not in ("", ".")]
    artifact_root_name = ARTIFACTS_ROOT.name
    # Callers sometimes forward branch-relative artifact paths such as
    # branch_3/artifacts/... or branch_3/branch_3/artifacts/... back into the
    # resolver on retries or replays. Collapse any leading branch-root prefix
    # chain so already-rooted artifact paths stay canonical.
    artifact_idx = next((idx for idx, part in enumerate(parts) if part == artifact_root_name), None)
    if artifact_idx is not None and artifact_idx > 0:
        branch_prefix = parts[:artifact_idx]
        if all(part == BRANCH_ROOT.name for part in branch_prefix):
            parts = parts[artifact_idx:]
    while parts and parts[0] == artifact_root_name:
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


def _ensure_artifact_process_dir(path: Path) -> Path:
    resolved = _ensure_under_artifacts(path.resolve())
    if resolved.exists() and not resolved.is_dir():
        if resolved.is_symlink() or resolved.is_file():
            resolved.unlink()
        else:
            shutil.rmtree(resolved, ignore_errors=True)
    try:
        resolved.mkdir(parents=True, exist_ok=True)
    except FileExistsError:
        if not resolved.is_dir():
            _reset_path(resolved)
            resolved.mkdir(parents=True, exist_ok=True)
    return resolved


def _is_under_artifacts(path: Path) -> bool:
    return _is_relative_to_root(path.resolve(), ARTIFACTS_ROOT)


def _ensure_under_artifacts(path: Path) -> Path:
    return _ensure_under_root(path, ARTIFACTS_ROOT, "output path")


def _is_relative_to_root(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
    except ValueError:
        return False
    return True


def _ensure_under_root(path: Path, root: Path, label: str) -> Path:
    try:
        path.relative_to(root)
    except ValueError as exc:
        raise ValueError(f"{label} must stay under {root}: {path}") from exc
    return path


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _iter_non_artifact_paths(branch_root: Path, artifacts_root: Path):
    branch_root = branch_root.resolve()
    artifacts_root = artifacts_root.resolve()

    for current_root, dirnames, filenames in os.walk(branch_root):
        current_path = Path(current_root)

        for dirname in list(dirnames):
            child = current_path / dirname
            if dirname in NON_ARTIFACT_SCAN_IGNORED_ROOTS:
                dirnames.remove(dirname)
                continue
            if _is_relative_to_root(child.resolve(), artifacts_root):
                dirnames.remove(dirname)
                continue
            if child.is_symlink():
                dirnames.remove(dirname)
                yield child
                continue
            yield child

        for filename in filenames:
            child = current_path / filename
            if _is_relative_to_root(child.resolve(), artifacts_root):
                continue
            yield child


def _describe_non_artifact_path(path: Path) -> dict[str, object]:
    metadata = path.lstat()
    mode = metadata.st_mode
    if stat.S_ISLNK(mode):
        return {
            "kind": "symlink",
            "target": os.readlink(path),
        }
    if stat.S_ISDIR(mode):
        return {
            "kind": "dir",
            "mode": stat.S_IMODE(mode),
        }
    if stat.S_ISREG(mode):
        payload: dict[str, object] = {
            "kind": "file",
            "size": metadata.st_size,
            "mtime_ns": metadata.st_mtime_ns,
            "mode": stat.S_IMODE(mode),
        }
        if NON_ARTIFACT_HASH_MAX_BYTES > 0 and metadata.st_size <= NON_ARTIFACT_HASH_MAX_BYTES:
            payload["sha256"] = _sha256_file(path)
        return payload
    return {
        "kind": "other",
        "mode": stat.S_IFMT(mode),
    }


def collect_non_artifact_tree_state(
    *,
    branch_root: Path = BRANCH_ROOT,
    artifacts_root: Path = ARTIFACTS_ROOT,
) -> dict[str, object]:
    branch_root = Path(branch_root).resolve()
    artifacts_root = Path(artifacts_root).resolve()
    entries: dict[str, dict[str, object]] = {}
    for path in _iter_non_artifact_paths(branch_root, artifacts_root):
        rel = path.relative_to(branch_root).as_posix()
        entries[rel] = _describe_non_artifact_path(path)
    return {
        "schema": NON_ARTIFACT_TREE_STATE_SCHEMA,
        "branch_root": str(branch_root),
        "artifacts_root": str(artifacts_root),
        "entries": entries,
    }


def write_non_artifact_tree_state(
    output_path: str | Path,
    *,
    branch_root: Path = BRANCH_ROOT,
    artifacts_root: Path = ARTIFACTS_ROOT,
) -> Path:
    branch_root = Path(branch_root).resolve()
    artifacts_root = Path(artifacts_root).resolve()
    output = _ensure_under_root(Path(output_path).resolve(), artifacts_root, "non-artifact tree state")
    output.parent.mkdir(parents=True, exist_ok=True)
    payload = collect_non_artifact_tree_state(branch_root=branch_root, artifacts_root=artifacts_root)
    # Wrapper temp roots can be recreated or cleared between the initial mkdir
    # and the final write on this iCloud-backed workspace, so re-establish the
    # parent directory immediately before publishing the snapshot.
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    return output


def load_non_artifact_tree_state(path: str | Path) -> dict[str, object]:
    with Path(path).open("r", encoding="utf-8") as handle:
        payload = json.load(handle)
    if payload.get("schema") != NON_ARTIFACT_TREE_STATE_SCHEMA:
        raise ValueError(f"unexpected non-artifact tree schema: {payload.get('schema')!r}")
    return payload


def compare_non_artifact_tree_states(
    baseline_state: dict[str, object],
    current_state: dict[str, object],
) -> dict[str, list[str]]:
    baseline_entries = baseline_state.get("entries", {})
    current_entries = current_state.get("entries", {})
    if not isinstance(baseline_entries, dict) or not isinstance(current_entries, dict):
        raise ValueError("non-artifact tree state entries must be JSON objects")

    baseline_paths = set(baseline_entries)
    current_paths = set(current_entries)
    created = sorted(current_paths - baseline_paths)
    removed = sorted(baseline_paths - current_paths)
    modified = sorted(
        path
        for path in (baseline_paths & current_paths)
        if baseline_entries[path] != current_entries[path]
    )
    return {
        "created": created,
        "modified": modified,
        "removed": removed,
    }


def verify_non_artifact_tree_state(
    baseline_path: str | Path,
    current_path: str | Path,
    report_path: str | Path,
) -> bool:
    baseline = load_non_artifact_tree_state(baseline_path)
    branch_root = Path(str(baseline["branch_root"])).resolve()
    artifacts_root = Path(str(baseline["artifacts_root"])).resolve()

    current_output = _ensure_under_root(
        Path(current_path).resolve(),
        artifacts_root,
        "non-artifact tree verification state",
    )
    report_output = _ensure_under_root(
        Path(report_path).resolve(),
        artifacts_root,
        "non-artifact tree verification report",
    )
    current_output.parent.mkdir(parents=True, exist_ok=True)
    report_output.parent.mkdir(parents=True, exist_ok=True)

    current = collect_non_artifact_tree_state(branch_root=branch_root, artifacts_root=artifacts_root)
    current_output.write_text(json.dumps(current, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    diff = compare_non_artifact_tree_states(baseline, current)
    current_entries = current.get("entries", {})
    if not isinstance(current_entries, dict):
        raise ValueError("current non-artifact tree entries must be a JSON object")
    advisory_created: list[str] = []
    blocking_created: list[str] = []
    for path in diff["created"]:
        if _is_advisory_non_artifact_creation(path, current_entries.get(path)):
            advisory_created.append(path)
        else:
            blocking_created.append(path)
    baseline_entries = baseline.get("entries", {})
    if not isinstance(baseline_entries, dict):
        raise ValueError("baseline non-artifact tree entries must be a JSON object")
    advisory_removed: list[str] = []
    blocking_removed: list[str] = []
    for path in diff["removed"]:
        if _is_advisory_non_artifact_removal(path, baseline_entries.get(path)):
            advisory_removed.append(path)
        else:
            blocking_removed.append(path)
    advisory_modified = diff["modified"]
    clean = not (blocking_created or blocking_removed)

    if clean:
        status = "clean" if not (advisory_created or advisory_modified or advisory_removed) else "modified_only_warning"
    else:
        status = "escape_detected"

    lines = [
        f"status={status}",
        f"schema={NON_ARTIFACT_TREE_STATE_SCHEMA}",
        f"branch_root={branch_root}",
        f"artifacts_root={artifacts_root}",
        f"baseline_state={Path(baseline_path).resolve()}",
        f"current_state={current_output}",
        f"created_count={len(blocking_created)}",
        f"created_warning_count={len(advisory_created)}",
        f"modified_count={len(advisory_modified)}",
        f"removed_warning_count={len(advisory_removed)}",
        f"removed_count={len(blocking_removed)}",
    ]
    for section, entries in (
        ("created", blocking_created),
        ("created_warning", advisory_created),
        ("modified_warning", advisory_modified),
        ("removed_warning", advisory_removed),
        ("removed", blocking_removed),
    ):
        if not entries:
            continue
        lines.append(f"[{section}]")
        lines.extend(entries)
    report_output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return clean


def _is_advisory_non_artifact_creation(path: str, entry: object) -> bool:
    if not isinstance(entry, dict):
        return False
    if entry.get("kind") != "file":
        return False

    rel_path = Path(path)
    if any(part in NON_ARTIFACT_CREATED_BLOCKING_DIRS for part in rel_path.parts):
        return False
    if (
        rel_path.parts
        and rel_path.parts[0] == ".ouroboros"
        and rel_path.suffix.lower() == ".log"
        and rel_path.name.startswith(NON_ARTIFACT_CREATED_OUROBOROS_LOG_PREFIXES)
    ):
        return True
    if rel_path.name.startswith(".") and (not rel_path.parts or rel_path.parts[0] != ".ouroboros"):
        return False
    return rel_path.suffix.lower() in NON_ARTIFACT_CREATED_SOURCE_WARNING_SUFFIXES


def _is_advisory_non_artifact_removal(path: str, entry: object) -> bool:
    if not isinstance(entry, dict):
        return False

    rel_path = Path(path)
    if any(part in NON_ARTIFACT_CREATED_BLOCKING_DIRS for part in rel_path.parts):
        return True
    return (
        rel_path.parts
        and rel_path.parts[0] == ".ouroboros"
        and rel_path.suffix.lower() == ".log"
        and rel_path.name.startswith(NON_ARTIFACT_CREATED_OUROBOROS_LOG_PREFIXES)
    )


def _reset_path(path: Path) -> None:
    if not path.exists():
        return
    if path.is_dir():
        shutil.rmtree(path, ignore_errors=True)
        return
    try:
        path.unlink()
    except FileNotFoundError:
        return
    except OSError:
        return


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
    ap.add_argument(
        "--snapshot-non-artifact-tree",
        default=None,
        help="write a baseline manifest for files under the branch root outside artifacts",
    )
    ap.add_argument(
        "--verify-non-artifact-tree",
        nargs=3,
        metavar=("BASELINE", "CURRENT", "REPORT"),
        help="compare the current non-artifact tree against BASELINE and write CURRENT and REPORT",
    )
    ap.add_argument("key", nargs="?", choices=sorted(DEFAULT_OUTPUT_SUBPATHS))
    ap.add_argument("path", nargs="?", default=None, help="optional output override")
    args = ap.parse_args()

    if args.snapshot_non_artifact_tree is not None:
        if (
            args.verify_non_artifact_tree is not None
            or args.artifacts_root
            or args.ensure is not None
            or args.key is not None
            or args.path is not None
        ):
            ap.error("--snapshot-non-artifact-tree does not accept other arguments")
        try:
            print(write_non_artifact_tree_state(args.snapshot_non_artifact_tree))
        except ValueError as exc:
            print(f"[artifact_paths] {exc}", file=sys.stderr)
            return 2
        return 0

    if args.verify_non_artifact_tree is not None:
        if args.artifacts_root or args.ensure is not None or args.key is not None or args.path is not None:
            ap.error("--verify-non-artifact-tree does not accept other arguments")
        baseline_path, current_path, report_path = args.verify_non_artifact_tree
        try:
            clean = verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        except ValueError as exc:
            print(f"[artifact_paths] {exc}", file=sys.stderr)
            return 2
        return 0 if clean else NON_ARTIFACT_TREE_VERIFY_ESCAPE_EXIT

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
