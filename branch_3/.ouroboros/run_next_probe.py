#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from retry_artifact_io import prepare_output_dir, reset_output_dir, write_text_output


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--state-file", required=True)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument("--branch-root", required=True)
    parser.add_argument("--timeout-seconds", type=int, default=180)
    return parser.parse_args()


def normalize_output(value: str | bytes | None) -> str:
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return value


def _load_artifact_guard(branch_root: Path):
    sys.path.insert(0, str(branch_root))
    from artifact_paths import ensure_under_artifacts  # type: ignore

    return ensure_under_artifacts


def _resolve_branch_path(branch_root: Path, value: str) -> Path:
    path = Path(value).expanduser()
    return path if path.is_absolute() else (branch_root / path).resolve()


def _resolve_artifact_path(branch_root: Path, ensure_under_artifacts, value: str) -> Path:
    return ensure_under_artifacts(_resolve_branch_path(branch_root, value))


def _build_probe_runtime_env(attempt_dir: Path, ensure_under_artifacts) -> tuple[dict[str, str], dict[str, str]]:
    runtime_root = ensure_under_artifacts((attempt_dir / ".probe_runtime_env").resolve())
    tmp_root = ensure_under_artifacts((runtime_root / "tmp").resolve())
    home_root = ensure_under_artifacts((runtime_root / "home").resolve())
    xdg_config_root = ensure_under_artifacts((runtime_root / "xdg_config").resolve())
    xdg_cache_root = ensure_under_artifacts((runtime_root / "xdg_cache").resolve())
    xdg_state_root = ensure_under_artifacts((runtime_root / "xdg_state").resolve())
    pycache_root = ensure_under_artifacts((runtime_root / "pycache").resolve())
    reset_output_dir(runtime_root)
    prepare_output_dir(tmp_root)
    prepare_output_dir(home_root)
    prepare_output_dir(xdg_config_root)
    prepare_output_dir(xdg_cache_root)
    prepare_output_dir(xdg_state_root)
    prepare_output_dir(pycache_root)

    runtime_paths = {
        "runtime_root": str(runtime_root),
        "tmp_root": str(tmp_root),
        "home_root": str(home_root),
        "xdg_config_root": str(xdg_config_root),
        "xdg_cache_root": str(xdg_cache_root),
        "xdg_state_root": str(xdg_state_root),
        "pycache_root": str(pycache_root),
    }
    env = dict(os.environ)
    env.update(
        {
            "BRANCH_ARTIFACT_TMP_ROOT": runtime_paths["tmp_root"],
            "TMPDIR": runtime_paths["tmp_root"],
            "TMP": runtime_paths["tmp_root"],
            "TEMP": runtime_paths["tmp_root"],
            "HOME": runtime_paths["home_root"],
            "XDG_CONFIG_HOME": runtime_paths["xdg_config_root"],
            "XDG_CACHE_HOME": runtime_paths["xdg_cache_root"],
            "XDG_STATE_HOME": runtime_paths["xdg_state_root"],
            "PYTHONPYCACHEPREFIX": runtime_paths["pycache_root"],
            "PYTHONDONTWRITEBYTECODE": "1",
        }
    )
    return env, runtime_paths


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).resolve()
    ensure_under_artifacts = _load_artifact_guard(branch_root)
    state_file = _resolve_branch_path(branch_root, args.state_file)
    attempt_dir = _resolve_artifact_path(branch_root, ensure_under_artifacts, args.attempt_dir)
    report_root = _resolve_artifact_path(branch_root, ensure_under_artifacts, args.report_root)
    prepare_output_dir(attempt_dir)
    prepare_output_dir(report_root)
    runtime_env, runtime_paths = _build_probe_runtime_env(attempt_dir, ensure_under_artifacts)

    try:
        state = json.loads(state_file.read_text())
    except Exception as exc:  # noqa: BLE001
        print(f"probe skipped: could not read state ({exc})")
        return 1

    command = state.get("next_probe_command")
    primary_axis = state.get("pinned_primary_axis")
    secondary_axis = state.get("pinned_secondary_axis")
    why_this_axis = state.get("why_this_axis")
    if not command:
        print("probe skipped: next_probe_command is empty")
        return 1

    started = time.time()
    timed_out = False
    try:
        result = subprocess.run(
            command,
            cwd=branch_root,
            shell=True,
            executable="/bin/zsh",
            capture_output=True,
            timeout=args.timeout_seconds,
            env=runtime_env,
        )
        exit_code = result.returncode
        stdout = normalize_output(result.stdout)
        stderr = normalize_output(result.stderr)
    except subprocess.TimeoutExpired as exc:
        timed_out = True
        exit_code = 124
        stdout = normalize_output(exc.stdout)
        stderr = normalize_output(exc.stderr)
    elapsed = round(time.time() - started, 3)

    payload = {
        "command": command,
        "primary_axis": primary_axis,
        "secondary_axis": secondary_axis,
        "why_this_axis": why_this_axis,
        "timeout_seconds": args.timeout_seconds,
        "elapsed_seconds": elapsed,
        "exit_code": exit_code,
        "timed_out": timed_out,
        "runtime_env": runtime_paths,
    }

    report_json = attempt_dir / "next_probe_result.json"
    report_md = attempt_dir / "next_probe_result.md"
    report_stdout = attempt_dir / "next_probe.stdout.log"
    report_stderr = attempt_dir / "next_probe.stderr.log"
    latest_json = report_root / "latest_next_probe_result.json"
    latest_md = report_root / "latest_next_probe_result.md"
    latest_stdout = report_root / "latest_next_probe.stdout.log"
    latest_stderr = report_root / "latest_next_probe.stderr.log"

    write_text_output(report_json, json.dumps(payload, indent=2) + "\n")
    write_text_output(report_stdout, stdout)
    write_text_output(report_stderr, stderr)
    write_text_output(
        report_md,
        "\n".join(
            [
                "# Next Probe Result",
                "",
                f"- Command: `{command}`",
                f"- Primary axis: `{primary_axis or 'unknown'}`",
                f"- Secondary axis: `{secondary_axis or 'none'}`",
                f"- Why this axis: `{why_this_axis or 'not recorded'}`",
                f"- Exit code: `{exit_code}`",
                f"- Timed out: `{'yes' if timed_out else 'no'}`",
                f"- Elapsed seconds: `{elapsed}`",
                f"- Probe tmp root: `{runtime_paths['tmp_root']}`",
                f"- Probe home root: `{runtime_paths['home_root']}`",
                f"- Probe XDG config root: `{runtime_paths['xdg_config_root']}`",
                f"- Probe XDG cache root: `{runtime_paths['xdg_cache_root']}`",
                f"- Probe XDG state root: `{runtime_paths['xdg_state_root']}`",
                f"- Probe pycache root: `{runtime_paths['pycache_root']}`",
                "",
                f"- Stdout log: `{report_stdout}`",
                f"- Stderr log: `{report_stderr}`",
                "",
            ]
        ),
    )
    write_text_output(latest_json, report_json.read_text())
    write_text_output(latest_md, report_md.read_text())
    write_text_output(latest_stdout, stdout)
    write_text_output(latest_stderr, stderr)

    print(report_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
