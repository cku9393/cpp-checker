#!/usr/bin/env python3
from __future__ import annotations

import unittest
from pathlib import Path

import branch_run_case
from artifact_paths import artifacts_root


class BranchRunCaseArtifactLocalityTests(unittest.TestCase):
    def assert_under_artifacts(self, value: str | Path) -> None:
        path = Path(value).resolve()
        try:
            path.relative_to(artifacts_root())
        except ValueError:
            self.fail(f"path escaped artifacts root: {path}")

    def test_default_branch_solver_uses_checked_in_launcher(self) -> None:
        self.assertEqual(
            branch_run_case.default_branch_solver(),
            branch_run_case.ROOT / "boj28350_resume" / "solve",
        )

    def test_build_case_solver_env_keeps_process_tmpdirs_under_artifacts(self) -> None:
        outdir = branch_run_case.resolve_case_outdir("unit/default_solver_env")
        env = branch_run_case.build_case_solver_env(outdir, "comb_core", 16, 7)

        self.assertEqual(env["DENSE_PROFILE_OUTDIR"], str(outdir))
        self.assertEqual(env["DENSE_SHADOW_CASE_MODE"], "comb_core")
        self.assertEqual(env["DENSE_SHADOW_CASE_N"], "16")
        self.assertEqual(env["DENSE_SHADOW_CASE_SEED"], "7")

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
        ):
            self.assertIn(key, env)
            self.assert_under_artifacts(env[key])

    def test_apply_solver_env_overrides_rejects_artifact_routing_overrides(self) -> None:
        base_env = branch_run_case.build_case_solver_env(
            branch_run_case.resolve_case_outdir("unit/override_guard"),
            "comb_core",
            8,
            3,
        )
        with self.assertRaisesRegex(ValueError, "cannot override branch-local artifact routing"):
            branch_run_case.apply_solver_env_overrides(
                base_env,
                {
                    "TMPDIR": "/tmp/not-allowed",
                    "DENSE_PROFILE_OUTDIR": "/tmp/not-allowed",
                },
            )


if __name__ == "__main__":
    unittest.main()
