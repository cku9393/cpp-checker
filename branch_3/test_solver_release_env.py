#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import unittest
from pathlib import Path

os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
sys.dont_write_bytecode = True

import artifact_paths
from artifact_paths import artifacts_root


ROOT = Path(__file__).resolve().parent


def non_artifact_bytecode_paths() -> list[Path]:
    found: list[Path] = []
    artifact_root = artifacts_root()
    for current_root, dirnames, filenames in os.walk(ROOT):
        current_path = Path(current_root).resolve()
        try:
            current_path.relative_to(artifact_root)
        except ValueError:
            pass
        else:
            dirnames[:] = []
            continue

        for dirname in list(dirnames):
            child = (current_path / dirname).resolve()
            try:
                child.relative_to(artifact_root)
            except ValueError:
                pass
            else:
                dirnames.remove(dirname)
                continue
            if dirname == "__pycache__":
                found.append(child)
                dirnames.remove(dirname)

        for filename in filenames:
            if filename.endswith((".pyc", ".pyo")):
                found.append(current_path / filename)
    return sorted(found)


class SolverReleaseEnvArtifactLocalityTests(unittest.TestCase):
    def assert_under_artifacts(self, value: str | Path) -> None:
        path = Path(value).resolve()
        try:
            path.relative_to(artifacts_root())
        except ValueError:
            self.fail(f"path escaped artifacts root: {path}")

    def test_sourcing_from_clean_shell_keeps_env_and_bytecode_under_artifacts(self) -> None:
        artifact_paths.purge_non_artifact_bytecode()
        self.assertEqual(non_artifact_bytecode_paths(), [])

        env = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": os.environ.get("TERM", "dumb"),
        }
        command = r"""
unset BRANCH_ARTIFACT_TMP_ROOT TMPDIR TMP TEMP HOME XDG_CONFIG_HOME XDG_CACHE_HOME XDG_STATE_HOME PYTHONPYCACHEPREFIX PYTHONDONTWRITEBYTECODE
source ./solver_release_env.sh
python3 - <<'PY'
import json
import os

keys = [
    "PYTHONDONTWRITEBYTECODE",
    "BRANCH_ARTIFACT_TMP_ROOT",
    "TMPDIR",
    "TMP",
    "TEMP",
    "HOME",
    "XDG_CONFIG_HOME",
    "XDG_CACHE_HOME",
    "XDG_STATE_HOME",
    "PYTHONPYCACHEPREFIX",
    "AC3_SUPPORT_REUSE_MAX_TOUCHED",
]
print(json.dumps({key: os.environ.get(key, "") for key in keys}, sort_keys=True))
PY
"""
        result = subprocess.run(
            ["bash", "-lc", command],
            cwd=ROOT,
            env=env,
            capture_output=True,
            text=True,
        )

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        payload = json.loads(result.stdout.strip())
        self.assertEqual(payload["PYTHONDONTWRITEBYTECODE"], "1")
        self.assertEqual(payload["AC3_SUPPORT_REUSE_MAX_TOUCHED"], "100000")
        for key, value in payload.items():
            if key in {"PYTHONDONTWRITEBYTECODE", "AC3_SUPPORT_REUSE_MAX_TOUCHED"}:
                continue
            self.assertTrue(value, msg=f"{key} should not be empty")
            self.assert_under_artifacts(value)

        self.assertEqual(non_artifact_bytecode_paths(), [])

    def test_configure_branch_process_env_uses_tool_local_clean_state_without_override(self) -> None:
        env = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "TERM": os.environ.get("TERM", "dumb"),
        }
        command = r"""
unset BRANCH_ARTIFACT_TMP_ROOT TMPDIR TMP TEMP HOME XDG_CONFIG_HOME XDG_CACHE_HOME XDG_STATE_HOME PYTHONPYCACHEPREFIX PYTHONDONTWRITEBYTECODE
python3 - <<'PY'
import json
import os

from artifact_paths import configure_branch_process_env

configure_branch_process_env()
keys = [
    "BRANCH_ARTIFACT_TMP_ROOT",
    "HOME",
    "XDG_CONFIG_HOME",
    "XDG_CACHE_HOME",
    "XDG_STATE_HOME",
    "PYTHONPYCACHEPREFIX",
]
print(json.dumps({key: os.environ.get(key, "") for key in keys}, sort_keys=True))
PY
"""
        result = subprocess.run(
            ["bash", "-lc", command],
            cwd=ROOT,
            env=env,
            capture_output=True,
            text=True,
        )

        self.assertEqual(result.returncode, 0, msg=result.stderr)
        payload = json.loads(result.stdout.strip())
        tmp_root = Path(payload["BRANCH_ARTIFACT_TMP_ROOT"]).resolve()
        self.assertEqual(tmp_root, artifacts_root() / "lca_tree_stress_v5" / ".tmp")
        for key in ("HOME", "XDG_CONFIG_HOME", "XDG_CACHE_HOME", "XDG_STATE_HOME", "PYTHONPYCACHEPREFIX"):
            value = payload[key]
            self.assertTrue(value, msg=f"{key} should not be empty")
            self.assert_under_artifacts(value)
            self.assertIn(
                str((artifacts_root() / "lca_tree_stress_v5" / ".tmp" / ".process_state").resolve()),
                str(Path(value).resolve()),
                msg=f"{key} should live under the tool-local process state root",
            )

    def test_configure_branch_process_env_recovers_file_shaped_xdg_targets_under_artifacts(self) -> None:
        test_root = artifacts_root() / "lca_tree_stress_v5" / ".tmp" / "unit_env_dir_recovery"
        polluted_home = test_root / "home"
        polluted_xdg_config = test_root / "xdg_config"
        polluted_cache = test_root / "xdg_cache"
        polluted_state = test_root / "xdg_state"
        polluted_pycache = test_root / "pycache"

        shutil.rmtree(test_root, ignore_errors=True)
        test_root.mkdir(parents=True, exist_ok=True)
        for path in (
            polluted_home,
            polluted_xdg_config,
            polluted_cache,
            polluted_state,
            polluted_pycache,
        ):
            path.write_text("polluted\n", encoding="utf-8")

        original_env = {
            key: os.environ.get(key)
            for key in (
                "BRANCH_ARTIFACT_TMP_ROOT",
                "TMPDIR",
                "TMP",
                "TEMP",
                "HOME",
                "XDG_CONFIG_HOME",
                "XDG_CACHE_HOME",
                "XDG_STATE_HOME",
                "PYTHONPYCACHEPREFIX",
            )
        }
        try:
            os.environ["BRANCH_ARTIFACT_TMP_ROOT"] = str(test_root)
            os.environ["TMPDIR"] = str(test_root)
            os.environ["TMP"] = str(test_root)
            os.environ["TEMP"] = str(test_root)
            os.environ["HOME"] = str(polluted_home)
            os.environ["XDG_CONFIG_HOME"] = str(polluted_xdg_config)
            os.environ["XDG_CACHE_HOME"] = str(polluted_cache)
            os.environ["XDG_STATE_HOME"] = str(polluted_state)
            os.environ["PYTHONPYCACHEPREFIX"] = str(polluted_pycache)

            artifact_paths.configure_branch_process_env()

            for path in (
                polluted_home,
                polluted_xdg_config,
                polluted_cache,
                polluted_state,
                polluted_pycache,
            ):
                self.assertTrue(path.is_dir(), msg=f"{path} should be recovered as a directory")
                self.assert_under_artifacts(path)
        finally:
            for key, value in original_env.items():
                if value is None:
                    os.environ.pop(key, None)
                else:
                    os.environ[key] = value
            shutil.rmtree(test_root, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
