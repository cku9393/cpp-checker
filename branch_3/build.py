#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import List

os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
sys.dont_write_bytecode = True

from artifact_paths import configure_branch_process_env, default_output_path, resolve_output_path
from suite_utils import IS_WINDOWS, default_solver_path


configure_branch_process_env()


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


def _build_commands(
    compiler: str,
    source: Path,
    output: Path,
    static_mode: str,
    defines: List[str],
) -> List[List[str]]:
    base = Path(compiler).name.lower()
    if base in {"cl", "cl.exe"}:
        undef_args = ["/ULOCAL"]
        define_args = [f"/D{define}" for define in defines]
        return [[compiler, "/O2", "/std:c++17", "/EHsc", "/nologo", *undef_args, *define_args, f"/Fe{output}", str(source)]]

    undef_args = ["-ULOCAL"]
    define_args = [f"-D{define}" for define in defines]
    common = [compiler, "-Ofast", "-DNDEBUG", "-std=c++17", *undef_args, *define_args]
    if not IS_WINDOWS:
        common.append("-pipe")

    normal = common + [str(source), "-o", str(output)]
    tuned = list(common)
    if not IS_WINDOWS:
        tuned.append("-march=native")
    tuned_normal = tuned + [str(source), "-o", str(output)]
    if static_mode == "never" or IS_WINDOWS or sys.platform == "darwin":
        return [tuned_normal, normal] if tuned_normal != normal else [normal]

    static_cmd = common + ["-static", str(source), "-o", str(output)]
    tuned_static = tuned + ["-static", str(source), "-o", str(output)]
    if static_mode == "always":
        return [tuned_static, static_cmd] if tuned_static != static_cmd else [static_cmd]
    cmds = []
    if tuned_static != static_cmd:
        cmds.append(tuned_static)
    cmds.append(static_cmd)
    if tuned_normal != normal:
        cmds.append(tuned_normal)
    cmds.append(normal)
    return cmds


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Portable build wrapper for boj28350_branch_3_solver.cpp."
    )
    ap.add_argument("--compiler", default=None, help="compiler executable to use")
    ap.add_argument("--source", default="boj28350_resume/boj28350_branch_3_solver.cpp")
    ap.add_argument("--out", default=None, help="output binary path")
    ap.add_argument(
        "--define",
        action="append",
        default=[],
        help="preprocessor define to add to the build, e.g. NAME or NAME=VALUE",
    )
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

    try:
        if args.out:
            output = resolve_output_path(args.out, default_key="boj28350_build")
        else:
            output = default_output_path("boj28350_build") / default_solver_path(ROOT).name
    except ValueError as exc:
        print(f"[build] {exc}", file=sys.stderr)
        return 2
    output.parent.mkdir(parents=True, exist_ok=True)

    tried: List[str] = []
    last: subprocess.CompletedProcess[str] | None = None
    for candidate in _compiler_candidates(args.compiler):
        compiler = _compiler_path(candidate)
        if not compiler:
            continue
        temp_path: Path | None = None
        try:
            fd, raw_temp_path = tempfile.mkstemp(
                prefix=f".{output.name}.",
                suffix=".tmp",
                dir=output.parent,
            )
            os.close(fd)
            temp_path = Path(raw_temp_path)
            temp_path.unlink(missing_ok=True)
            for cmd in _build_commands(compiler, source, temp_path, args.static, args.define):
                display_cmd = [str(output) if part == str(temp_path) else part for part in cmd]
                tried.append(" ".join(display_cmd))
                last = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
                if last.returncode == 0:
                    os.replace(temp_path, output)
                    if not IS_WINDOWS:
                        output.chmod(output.stat().st_mode | 0o111)
                    print(f"[build] compiler={compiler}")
                    print(f"[build] output={output}")
                    if args.define:
                        print(f"[build] defines={','.join(args.define)}")
                    return 0
                temp_path.unlink(missing_ok=True)
        finally:
            if temp_path is not None:
                temp_path.unlink(missing_ok=True)

    print("[build] failed to compile boj28350_branch_3_solver.cpp", file=sys.stderr)
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
