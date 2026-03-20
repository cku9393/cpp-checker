#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List

from suite_utils import IS_WINDOWS, default_solver_path


ROOT = Path(__file__).resolve().parent


def _compiler_candidates(preferred: str | None) -> List[str]:
    out: List[str] = []
    seen = set()

    def add(name: str | None) -> None:
        if not name or name in seen:
            return
        seen.add(name)
        out.append(name)

    add(preferred)
    add(os.environ.get("CXX"))
    if IS_WINDOWS:
        for name in ("clang++", "g++", "cl"):
            add(name)
    else:
        for name in ("g++", "clang++", "c++"):
            add(name)
    return out


def _compiler_path(name: str) -> str | None:
    p = Path(name)
    if p.is_absolute() or p.parent != Path("."):
        return str(p) if p.exists() else None
    return shutil.which(name)


def _build_commands(compiler: str, source: Path, output: Path, static_mode: str) -> List[List[str]]:
    base = Path(compiler).name.lower()
    if base in {"cl", "cl.exe"}:
        return [[compiler, "/O2", "/std:c++17", "/EHsc", "/nologo", f"/Fe{output}", str(source)]]

    common = [compiler, "-O2", "-std=c++17"]
    if not IS_WINDOWS:
        common.append("-pipe")

    normal = common + [str(source), "-o", str(output)]
    if static_mode == "never" or IS_WINDOWS or sys.platform == "darwin":
        return [normal]

    static_cmd = common + ["-static", str(source), "-o", str(output)]
    if static_mode == "always":
        return [static_cmd]
    return [static_cmd, normal]


def main() -> int:
    ap = argparse.ArgumentParser(description="Portable build wrapper for solve.cpp.")
    ap.add_argument("--compiler", default=None, help="compiler executable to use")
    ap.add_argument("--source", default="solve.cpp")
    ap.add_argument("--out", default=None, help="output binary path")
    ap.add_argument(
        "--static",
        choices=("auto", "always", "never"),
        default="auto",
        help="request static linking when supported",
    )
    args = ap.parse_args()

    source = (ROOT / args.source).resolve()
    if not source.exists():
        print(f"[build] missing source: {source}", file=sys.stderr)
        return 2

    output = Path(args.out).resolve() if args.out else default_solver_path(ROOT)
    output.parent.mkdir(parents=True, exist_ok=True)

    tried: List[str] = []
    last: subprocess.CompletedProcess[str] | None = None
    for candidate in _compiler_candidates(args.compiler):
        compiler = _compiler_path(candidate)
        if not compiler:
            continue
        for cmd in _build_commands(compiler, source, output, args.static):
            tried.append(" ".join(cmd))
            last = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
            if last.returncode == 0:
                print(f"[build] compiler={compiler}")
                print(f"[build] output={output}")
                return 0

    print("[build] failed to compile solve.cpp", file=sys.stderr)
    if tried:
        print("[build] attempted commands:", file=sys.stderr)
        for cmd in tried:
            print(f"  {cmd}", file=sys.stderr)
    if last is not None:
        if last.stdout.strip():
            print(last.stdout, file=sys.stderr)
        if last.stderr.strip():
            print(last.stderr, file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
