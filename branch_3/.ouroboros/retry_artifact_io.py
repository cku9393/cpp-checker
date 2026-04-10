#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import uuid
from pathlib import Path


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
