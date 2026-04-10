#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
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
BUILD_METADATA_SCHEMA = "boj28350_build_metadata_v2"
LOCAL_INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*"([^"]+)"')


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
        # On macOS, Apple clang is the reproducible fast path for the current
        # BOJ 28350 branch; preferring `g++` here can resolve to a slower driver
        # path and materially change gate outcomes.
        if sys.platform == "darwin":
            order = ("clang++", "g++", "c++")
        else:
            order = ("g++", "clang++", "c++")
        for name in order:
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
    thinlto_tuned: List[str] | None = None
    thinlto_normal: List[str] | None = None
    if sys.platform == "darwin":
        thinlto_common = list(common)
        thinlto_common.append("-flto=thin")
        thinlto_normal = thinlto_common + [str(source), "-o", str(output)]
        thinlto_tuned_base = list(tuned)
        thinlto_tuned_base.append("-flto=thin")
        thinlto_tuned = thinlto_tuned_base + [str(source), "-o", str(output)]
    if static_mode == "never" or IS_WINDOWS or sys.platform == "darwin":
        cmds: List[List[str]] = []
        if thinlto_tuned is not None:
            cmds.append(thinlto_tuned)
        if tuned_normal != normal:
            cmds.append(tuned_normal)
        if thinlto_normal is not None and thinlto_normal != normal:
            cmds.append(thinlto_normal)
        cmds.append(normal)
        deduped: List[List[str]] = []
        seen = set()
        for cmd in cmds:
            key = tuple(cmd)
            if key in seen:
                continue
            seen.add(key)
            deduped.append(cmd)
        return deduped

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


def build_metadata_path(output: Path) -> Path:
    return output.parent / f"{output.name}.build_meta.json"


def _path_relative_to_root(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return str(path.resolve())


def _source_dependency_fingerprints(source: Path) -> list[dict[str, object]]:
    visited: set[Path] = set()
    fingerprints: list[dict[str, object]] = []

    def record(path: Path) -> None:
        resolved = path.resolve()
        if resolved in visited:
            return
        visited.add(resolved)

        try:
            data = resolved.read_bytes()
        except OSError:
            fingerprints.append(
                {
                    "path": _path_relative_to_root(resolved),
                    "exists": False,
                }
            )
            return

        fingerprints.append(
            {
                "path": _path_relative_to_root(resolved),
                "exists": True,
                "size_bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
            }
        )

        text = data.decode("utf-8", errors="ignore")
        for line in text.splitlines():
            match = LOCAL_INCLUDE_PATTERN.match(line)
            if not match:
                continue
            include_path = (resolved.parent / match.group(1)).resolve()
            record(include_path)

    record(source)
    fingerprints.sort(key=lambda entry: str(entry["path"]))
    return fingerprints


def _build_metadata_payload(
    *,
    source: Path,
    output: Path,
    compiler: str,
    command: list[str],
    requested_compiler: str | None,
    static_mode: str,
    defines: list[str],
) -> dict[str, object]:
    return {
        "schema": BUILD_METADATA_SCHEMA,
        "source": _path_relative_to_root(source),
        "output": _path_relative_to_root(output),
        "compiler": compiler,
        "command": command,
        "requested_compiler": requested_compiler or "",
        "cxx_env": os.environ.get("CXX", ""),
        "static_mode": static_mode,
        "defines": list(defines),
        "dependencies": _source_dependency_fingerprints(source),
    }


def _write_build_metadata(
    output: Path,
    *,
    source: Path,
    compiler: str,
    command: list[str],
    requested_compiler: str | None,
    static_mode: str,
    defines: list[str],
) -> None:
    metadata_path = build_metadata_path(output)
    fd, raw_temp_path = tempfile.mkstemp(
        prefix=f".{metadata_path.name}.",
        suffix=".tmp",
        dir=metadata_path.parent,
    )
    os.close(fd)
    temp_path = Path(raw_temp_path)
    try:
        payload = _build_metadata_payload(
            source=source,
            output=output,
            compiler=compiler,
            command=command,
            requested_compiler=requested_compiler,
            static_mode=static_mode,
            defines=defines,
        )
        temp_path.write_text(
            json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        os.replace(temp_path, metadata_path)
    finally:
        temp_path.unlink(missing_ok=True)


def _existing_build_is_current(
    *,
    source: Path,
    output: Path,
    requested_compiler: str | None,
    static_mode: str,
    defines: list[str],
) -> bool:
    if not output.exists():
        return False
    metadata_path = build_metadata_path(output)
    if not metadata_path.exists():
        return False
    try:
        payload = json.loads(metadata_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return False
    if payload.get("schema") != BUILD_METADATA_SCHEMA:
        return False
    if payload.get("source") != _path_relative_to_root(source):
        return False
    if payload.get("output") != _path_relative_to_root(output):
        return False
    if payload.get("requested_compiler") != (requested_compiler or ""):
        return False
    if payload.get("cxx_env") != os.environ.get("CXX", ""):
        return False
    if payload.get("static_mode") != static_mode:
        return False
    if payload.get("defines") != list(defines):
        return False
    if payload.get("dependencies") != _source_dependency_fingerprints(source):
        return False
    return True


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

    if _existing_build_is_current(
        source=source,
        output=output,
        requested_compiler=args.compiler,
        static_mode=args.static,
        defines=list(args.define),
    ):
        print(f"[build] up-to-date=1 output={output}")
        if args.define:
            print(f"[build] defines={','.join(args.define)}")
        return 0

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
                last = subprocess.run(cmd, cwd=output.parent, capture_output=True, text=True)
                if last.returncode == 0:
                    os.replace(temp_path, output)
                    _write_build_metadata(
                        output,
                        source=source,
                        compiler=compiler,
                        command=display_cmd,
                        requested_compiler=args.compiler,
                        static_mode=args.static,
                        defines=list(args.define),
                    )
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
