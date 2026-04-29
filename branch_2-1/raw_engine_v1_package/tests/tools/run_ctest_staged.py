#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in os.sys.path:
    os.sys.path.insert(0, str(SCRIPT_DIR))

from staged_verification_lib import EXPECTED_PHASE35_TESTS, WRAPPER_TAIL_TESTS, aggregate_hash, sha256_file_local


def timestamp_utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def atomic_write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = path.with_name(f".{path.name}.tmp.{os.getpid()}.{time.time_ns()}")
    temp_path.write_text(text, encoding="utf-8")
    os.replace(temp_path, path)


def write_json(path: Path, payload: dict[str, Any]) -> None:
    atomic_write_text(path, json.dumps(payload, indent=2) + "\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run a raw_engine test binary or CTest in a non-iCloud staging directory and classify infra vs semantic failures."
    )
    parser.add_argument("--binary", default=None, help="Absolute path to raw_engine_tests for staged binary execution.")
    parser.add_argument("--build-dir", default=None, help="CTest build directory for staged ctest execution.")
    parser.add_argument("--ctest-bin", default="ctest")
    parser.add_argument("--cmake-bin", default="cmake")
    parser.add_argument("--working-directory", default=None)
    parser.add_argument("--stage-root", default=None)
    parser.add_argument("--stage-name", default="staged_run")
    parser.add_argument("--result-json", default=None)
    parser.add_argument("--keep-stage", action="store_true")
    parser.add_argument("--retry-on-infra-failure", type=int, default=0)
    parser.add_argument("--timeout-seconds", type=int, default=0)
    parser.add_argument("--mode", choices=["release", "debug", "asan"], default=None)
    parser.add_argument("--staged-root", default=None)
    parser.add_argument("--snapshot-manifest", default=None)
    parser.add_argument("--inventory-out", default=None)
    parser.add_argument("--verification-out", default=None)
    parser.add_argument("--published-root", default=None)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument("--require-zero-not-run", action="store_true")
    parser.add_argument("--ctest-jobs", type=int, default=0)
    parser.add_argument("--keep-build-dir", action="store_true")
    parser.add_argument("args", nargs=argparse.REMAINDER)
    return parser.parse_args()


def normalize_passthrough_args(values: list[str]) -> list[str]:
    if values and values[0] == "--":
        return values[1:]
    return values


def ensure_stage_root(stage_root: str | None, stage_name: str) -> Path:
    if stage_root:
        root = Path(stage_root).resolve()
        root.mkdir(parents=True, exist_ok=True)
        return root
    return Path(tempfile.mkdtemp(prefix=f"raw_engine_{stage_name}_"))


def classify_failure(output: str, returncode: int) -> str:
    lowered = output.lower()
    if (
        "could not find executable" in lowered
        or "unable to find executable" in lowered
        or "no such file or directory" in lowered
        or "not run" in lowered
        or returncode == 127
    ):
        return "wrapper_infra_failure"
    if returncode != 0:
        return "semantic_failure"
    return "pass"


def run_once(command: list[str], cwd: Path, env: dict[str, str], timeout_seconds: int) -> tuple[subprocess.CompletedProcess[str] | None, str | None]:
    try:
        completed = subprocess.run(
            command,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=None if timeout_seconds <= 0 else timeout_seconds,
            env=env,
            check=False,
        )
        return completed, None
    except FileNotFoundError as exc:
        return None, str(exc)
    except subprocess.TimeoutExpired as exc:
        return None, f"timeout expired after {timeout_seconds} seconds: {exc}"


def build_command(args: argparse.Namespace, passthrough_args: list[str]) -> tuple[list[str], str]:
    if args.binary:
        return [str(Path(args.binary).resolve()), *passthrough_args], "binary"
    if args.build_dir:
        command = [
            args.ctest_bin,
            "--test-dir",
            str(Path(args.build_dir).resolve()),
            "--output-on-failure",
            "--force-new-ctest-process",
        ]
        command.extend(passthrough_args)
        return command, "ctest"
    raise SystemExit("either --binary or --build-dir is required")


def run_capture(command: list[str], cwd: Path, env: dict[str, str], timeout_seconds: int = 0) -> tuple[subprocess.CompletedProcess[str] | None, str | None]:
    try:
        completed = subprocess.run(
            command,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=None if timeout_seconds <= 0 else timeout_seconds,
            env=env,
            check=False,
        )
        return completed, None
    except FileNotFoundError as exc:
        return None, str(exc)
    except subprocess.TimeoutExpired as exc:
        return None, f"timeout expired after {timeout_seconds} seconds: {exc}"


def mode_build_dir(staged_root: Path, mode: str) -> Path:
    return staged_root / f"build-{mode}"


def mode_configure_args(args: argparse.Namespace, mode: str, staged_root: Path, build_dir: Path) -> list[str]:
    command = [
        args.cmake_bin,
        "-S",
        str(staged_root),
        "-B",
        str(build_dir),
        "-DCMAKE_BUILD_TYPE=" + ("Release" if mode == "release" else "Debug"),
        "-DRAW_ENGINE_REGISTER_NIGHTLY_TESTS=ON",
    ]
    if mode == "asan":
        command.append("-DRAW_ENGINE_ENABLE_ASAN=ON")
    return command


def parse_ctest_inventory(stdout: str) -> tuple[list[str], dict[str, int]]:
    names: list[str] = []
    for line in stdout.splitlines():
        match = re.search(r"Test\s+#\d+:\s+(.+)$", line)
        if match:
            names.append(match.group(1).strip())
    return names, {}


def discover_inventory(args: argparse.Namespace, build_dir: Path, env: dict[str, str]) -> dict[str, Any]:
    json_command = [args.ctest_bin, "--test-dir", str(build_dir), "--show-only=json-v1"]
    completed, _ = run_capture(json_command, build_dir, env, 0)
    names: list[str] = []
    label_counts: dict[str, int] = {}
    if completed is not None and completed.returncode == 0:
        try:
            payload = json.loads(completed.stdout)
            tests = payload.get("tests", [])
            for item in tests:
                name = str(item.get("name", "")).strip()
                if name:
                    names.append(name)
                for label in item.get("properties", {}).get("LABELS", []):
                    label_counts[str(label)] = label_counts.get(str(label), 0) + 1
        except Exception:
            names = []
    if not names:
        fallback = [args.ctest_bin, "--test-dir", str(build_dir), "-N", "--force-new-ctest-process"]
        completed, error_text = run_capture(fallback, build_dir, env, 0)
        if completed is None:
            raise RuntimeError(error_text or "failed to discover ctest inventory")
        names, label_counts = parse_ctest_inventory(completed.stdout)
    expected = list(EXPECTED_PHASE35_TESTS)
    missing_expected = [name for name in expected if name not in names]
    wrapper_tail_missing = [name for name in WRAPPER_TAIL_TESTS if name not in names]
    inventory_verdict = "PASS" if not missing_expected and not wrapper_tail_missing else "FAIL"
    return {
        "manifest_version": "ctest_inventory_v1",
        "generated_at_utc": timestamp_utc_now(),
        "build_dir": str(build_dir),
        "discovered_test_count": len(names),
        "discovered_test_names": names,
        "missing_expected_tests": missing_expected,
        "extra_unexpected_tests": [],
        "label_counts": label_counts,
        "phase35_expected_smokes_present": len(missing_expected) == 0,
        "phase36_specific_smokes_present": False,
        "wrapper_tail_tests_present": len(wrapper_tail_missing) == 0,
        "wrapper_tail_missing_tests": wrapper_tail_missing,
        "inventory_verdict": inventory_verdict,
    }


def parse_ctest_counts(output: str, total: int) -> tuple[int, int, int]:
    fail_count = len(re.findall(r"\*{3}Failed", output))
    not_run_count = len(re.findall(r"\*{3}Not Run", output))
    pass_count = max(0, total - fail_count - not_run_count)
    return pass_count, fail_count, not_run_count


def run_mode_verification(args: argparse.Namespace) -> int:
    if not args.staged_root or not args.snapshot_manifest or not args.inventory_out or not args.verification_out:
        raise SystemExit("--mode requires --staged-root, --snapshot-manifest, --inventory-out, and --verification-out")
    staged_root = Path(args.staged_root).resolve()
    snapshot_manifest_path = Path(args.snapshot_manifest).resolve()
    snapshot_manifest = json.loads(snapshot_manifest_path.read_text())
    build_dir = mode_build_dir(staged_root, args.mode)
    if build_dir.exists() and not args.keep_build_dir:
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env["TMPDIR"] = str(staged_root / "tmp")
    env["TMP"] = env["TMPDIR"]
    env["TEMP"] = env["TMPDIR"]
    Path(env["TMPDIR"]).mkdir(parents=True, exist_ok=True)

    configure_command = mode_configure_args(args, args.mode, staged_root, build_dir)
    total_start = time.time()
    configure_completed, configure_error = run_capture(configure_command, staged_root, env, args.timeout_seconds)
    if configure_completed is None:
        raise RuntimeError(configure_error or "cmake configure failed")
    if configure_completed.returncode != 0:
        raise RuntimeError(f"cmake configure failed\n{configure_completed.stdout}\n{configure_completed.stderr}")

    build_command = [args.cmake_bin, "--build", str(build_dir), "--target", "raw_engine_tests"]
    build_completed, build_error = run_capture(build_command, staged_root, env, args.timeout_seconds)
    if build_completed is None:
        raise RuntimeError(build_error or "cmake build failed")
    if build_completed.returncode != 0:
        raise RuntimeError(f"cmake build failed\n{build_completed.stdout}\n{build_completed.stderr}")

    inventory = discover_inventory(args, build_dir, env)
    write_json(Path(args.inventory_out).resolve(), inventory)
    inventory_hash = sha256_file_local(Path(args.inventory_out).resolve())
    inventory_ok = inventory.get("inventory_verdict") == "PASS"

    ctest_command = [
        args.ctest_bin,
        "--test-dir",
        str(build_dir),
        "--output-on-failure",
        "--force-new-ctest-process",
    ]
    if args.ctest_jobs and args.ctest_jobs > 0:
        ctest_command.extend(["-j", str(args.ctest_jobs)])
    ctest_completed, ctest_error = run_capture(ctest_command, build_dir, env, args.timeout_seconds)
    wall_time = round(time.time() - total_start, 2)
    if ctest_completed is None:
        raise RuntimeError(ctest_error or "ctest failed to execute")
    combined_output = "\n".join(filter(None, [ctest_completed.stdout, ctest_completed.stderr]))
    pass_count, fail_count, not_run_count = parse_ctest_counts(combined_output, int(inventory["discovered_test_count"]))
    execution_verdict = (
        "PASS"
        if ctest_completed.returncode == 0
        and fail_count == 0
        and inventory_ok
        and (not args.require_zero_not_run or not_run_count == 0)
        else "FAIL"
    )
    verification = {
        "manifest_version": "staged_verification_v1",
        "generated_at_utc": timestamp_utc_now(),
        "mode": args.mode,
        "staged_root": str(staged_root),
        "build_dir": str(build_dir),
        "ctest_command": ctest_command,
        "pass_count": pass_count,
        "fail_count": fail_count,
        "not_run_count": not_run_count,
        "total_test_count": inventory["discovered_test_count"],
        "total_wall_time_seconds": wall_time,
        "staged_source_snapshot_hash": snapshot_manifest.get("snapshot_hash"),
        "ctest_inventory_path": str(Path(args.inventory_out).resolve()),
        "ctest_inventory_hash": inventory_hash,
        "ctest_inventory_verdict": inventory.get("inventory_verdict"),
        "published_root": args.published_root,
        "execution_verdict": execution_verdict,
        "stdout_tail": ctest_completed.stdout[-4000:],
        "stderr_tail": ctest_completed.stderr[-4000:],
    }
    write_json(Path(args.verification_out).resolve(), verification)
    if args.strict and execution_verdict != "PASS":
        return 1
    return 0 if execution_verdict == "PASS" else 1


def main() -> int:
    args = parse_args()
    if args.mode:
        return run_mode_verification(args)
    passthrough_args = normalize_passthrough_args(args.args)
    command, mode = build_command(args, passthrough_args)
    stage_root = ensure_stage_root(args.stage_root, args.stage_name)
    cwd = Path(args.working_directory).resolve() if args.working_directory else stage_root
    cwd.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env["TMPDIR"] = str(stage_root / "tmp")
    env["TMP"] = str(stage_root / "tmp")
    env["TEMP"] = str(stage_root / "tmp")
    env["RAW_ENGINE_STAGE_ROOT"] = str(stage_root)
    Path(env["TMPDIR"]).mkdir(parents=True, exist_ok=True)

    attempts: list[dict[str, Any]] = []
    retries_left = max(0, int(args.retry_on_infra_failure))
    final_status = "wrapper_infra_failure"
    final_returncode = 1

    while True:
        completed, error_text = run_once(command, cwd, env, args.timeout_seconds)
        if completed is None:
            output = error_text or ""
            failure_kind = "wrapper_infra_failure"
            attempt = {
                "timestamp_utc": timestamp_utc_now(),
                "returncode": None,
                "stdout": "",
                "stderr": output,
                "failure_kind": failure_kind,
            }
            attempts.append(attempt)
            final_status = failure_kind
            break

        combined_output = "\n".join(filter(None, [completed.stdout, completed.stderr]))
        failure_kind = classify_failure(combined_output, completed.returncode)
        attempt = {
            "timestamp_utc": timestamp_utc_now(),
            "returncode": completed.returncode,
            "stdout": completed.stdout,
            "stderr": completed.stderr,
            "failure_kind": failure_kind,
        }
        attempts.append(attempt)
        final_returncode = completed.returncode
        final_status = failure_kind
        if failure_kind != "wrapper_infra_failure" or retries_left <= 0:
            break
        retries_left -= 1

    payload = {
        "manifest_version": "staged_ctest_result_v1",
        "generated_at_utc": timestamp_utc_now(),
        "mode": mode,
        "stage_name": args.stage_name,
        "staging_root": str(stage_root),
        "working_directory": str(cwd),
        "command": command,
        "attempt_count": len(attempts),
        "status": final_status,
        "returncode": final_returncode,
        "attempts": attempts,
    }
    if args.result_json:
        write_json(Path(args.result_json).resolve(), payload)

    if not args.keep_stage:
        shutil.rmtree(stage_root, ignore_errors=True)

    if final_status == "pass":
        return 0
    return 2 if final_status == "wrapper_infra_failure" else 1


if __name__ == "__main__":
    raise SystemExit(main())
