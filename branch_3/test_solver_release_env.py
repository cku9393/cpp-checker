#!/usr/bin/env python3
from __future__ import annotations

import json
import os
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
        for key, value in payload.items():
            if key == "PYTHONDONTWRITEBYTECODE":
                continue
            self.assertTrue(value, msg=f"{key} should not be empty")
            self.assert_under_artifacts(value)

        self.assertEqual(non_artifact_bytecode_paths(), [])


if __name__ == "__main__":
    unittest.main()
