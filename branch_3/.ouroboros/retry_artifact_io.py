#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import uuid
from pathlib import Path

RETRY_HELPER_SUBDIRS = {
    "control": ("control",),
    "logs": ("logs",),
    "runtime": ("runtime",),
    "inputs": ("inputs",),
    "preflight": ("preflight",),
}


def _remove_path(path: Path) -> None:
    if not path.exists() and not path.is_symlink():
        return
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path, ignore_errors=True)
        return
    try:
        path.unlink()
    except FileNotFoundError:
        return


def resolve_branch_path(branch_root: Path, value: str | Path) -> Path:
    raw = Path(value).expanduser()
    if raw.is_absolute():
        return raw.resolve()

    parts = [part for part in raw.parts if part not in ("", ".")]
    artifact_root_name = "artifacts"
    # Retry helpers sometimes round-trip branch-prefixed paths such as
    # branch_3/artifacts/... or branch_3/.ouroboros/... back into a helper even
    # when the minimal artifact guard does not expose the shared resolver.
    artifact_idx = next((idx for idx, part in enumerate(parts) if part == artifact_root_name), None)
    if artifact_idx is not None and artifact_idx > 0:
        branch_prefix = parts[:artifact_idx]
        if all(part == branch_root.name for part in branch_prefix):
            parts = parts[artifact_idx:]
    while parts and parts[0] == branch_root.name:
        parts.pop(0)
    while len(parts) > 1 and parts[0] == artifact_root_name and parts[1] == artifact_root_name:
        parts.pop(0)
    return (branch_root / Path(*parts)).resolve() if parts else branch_root.resolve()


def resolve_artifact_output_path(
    branch_root: Path,
    value: str | Path,
    ensure_under_artifacts,
) -> Path:
    return ensure_under_artifacts(resolve_branch_path(branch_root, value))


def retry_helper_dir(report_root: Path, namespace: str) -> Path:
    try:
        suffix = RETRY_HELPER_SUBDIRS[namespace]
    except KeyError as exc:
        raise ValueError(f"unknown retry helper namespace: {namespace}") from exc
    return report_root.joinpath(*suffix)


def retry_helper_path(report_root: Path, namespace: str, *parts: str) -> Path:
    return retry_helper_dir(report_root, namespace).joinpath(*parts)


def prepare_output_dir(path: Path) -> Path:
    if path.parent != path:
        prepare_output_dir(path.parent)
    if path.exists():
        if path.is_dir() and not path.is_symlink():
            return path
        _remove_path(path)
    path.mkdir(exist_ok=True)
    return path


def reset_output_dir(path: Path) -> Path:
    _remove_path(path)
    return prepare_output_dir(path)


def prepare_output_path(path: Path) -> Path:
    prepare_output_dir(path.parent)
    if path.exists() and (path.is_dir() or path.is_symlink()):
        _remove_path(path)
    return path


def _temp_output_path(path: Path) -> Path:
    while True:
        candidate = path.parent / f".{path.name}.tmp.{os.getpid()}.{uuid.uuid4().hex}"
        if not candidate.exists() and not candidate.is_symlink():
            return candidate
        _remove_path(candidate)


def write_text_output(path: Path, text: str, *, encoding: str = "utf-8") -> Path:
    path = prepare_output_path(path)
    tmp_path = _temp_output_path(path)
    try:
        tmp_path.write_text(text, encoding=encoding)
        tmp_path.replace(path)
    finally:
        _remove_path(tmp_path)
    return path


def copy_output_file(source: Path, destination: Path) -> Path:
    destination = prepare_output_path(destination)
    tmp_path = _temp_output_path(destination)
    try:
        shutil.copy2(source, tmp_path)
        tmp_path.replace(destination)
    finally:
        _remove_path(tmp_path)
    return destination
