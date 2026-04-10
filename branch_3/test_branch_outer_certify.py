#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import shutil
import unittest
from pathlib import Path
from unittest import mock

import branch_outer_certify
from artifact_paths import artifacts_root


class BranchOuterCertifyArtifactLocalityTests(unittest.TestCase):
    def assert_under_artifacts(self, value: str | Path) -> None:
        path = Path(value).resolve()
        try:
            path.relative_to(artifacts_root())
        except ValueError:
            self.fail(f"path escaped artifacts root: {path}")

    def test_default_output_root_stays_under_branch_artifacts(self) -> None:
        out_dir = branch_outer_certify.resolve_certify_out_dir("cert_out")

        self.assert_under_artifacts(out_dir)
        self.assertEqual(
            out_dir,
            artifacts_root() / "lca_tree_stress_v5" / "outer_certify" / "cert_out",
        )

    def test_case_outdir_rejects_non_artifact_absolute_paths(self) -> None:
        with self.assertRaisesRegex(ValueError, "must stay under"):
            branch_outer_certify.ensure_case_outdir(Path("/tmp/branch_outer_certify_escape"))

    def test_build_case_solver_env_keeps_profile_output_under_artifacts(self) -> None:
        case_dir = branch_outer_certify.resolve_certify_out_dir("unit/env_probe")
        env = branch_outer_certify.build_case_solver_env(case_dir, "comb_core", 32, 11)

        self.assertEqual(env["DENSE_PROFILE_OUTDIR"], str(case_dir))
        self.assertEqual(env["DENSE_SHADOW_CASE_MODE"], "comb_core")
        self.assertEqual(env["DENSE_SHADOW_CASE_N"], "32")
        self.assertEqual(env["DENSE_SHADOW_CASE_SEED"], "11")
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

    def test_mode_list_from_generator_uses_artifact_rooted_cwd(self) -> None:
        out_dir = branch_outer_certify.resolve_certify_out_dir("unit/mode_list_cwd")
        with mock.patch.object(
            subprocess,
            "run",
            return_value=mock.Mock(returncode=0, stdout="comb_core\n"),
        ) as run_mock:
            modes = branch_outer_certify.mode_list_from_generator(cwd=out_dir)

        self.assertEqual(modes, ["comb_core"])
        self.assertEqual(run_mock.call_args.kwargs["cwd"], out_dir)
        self.assert_under_artifacts(run_mock.call_args.kwargs["cwd"])

    def test_run_one_case_executes_generator_and_validator_from_case_dir(self) -> None:
        out_dir = branch_outer_certify.resolve_certify_out_dir("unit/run_one_case_cwd")
        case_dir = out_dir / "runs" / "correctness_fuzz" / "comb_core" / "n8" / "seed1_L0_Q0"
        shutil.rmtree(out_dir, ignore_errors=True)
        solver = Path("/bin/true")
        run_cmd_calls: list[dict[str, object]] = []

        def fake_run_cmd(cmd, *, stdout_path=None, stderr_path=None, timeout=None, env=None, cwd=None):
            run_cmd_calls.append(
                {
                    "cmd": list(cmd),
                    "stdout_path": stdout_path,
                    "stderr_path": stderr_path,
                    "timeout": timeout,
                    "cwd": cwd,
                }
            )
            if stdout_path is not None:
                Path(stdout_path).write_text("8 0\n", encoding="utf-8")
            if stderr_path is not None:
                Path(stderr_path).write_text("", encoding="utf-8")
            if "--meta" in cmd:
                meta_path = Path(cmd[cmd.index("--meta") + 1])
                parent_path = Path(cmd[cmd.index("--parent-out") + 1])
                meta_path.write_text("{}\n", encoding="utf-8")
                parent_path.write_text("0 1 2 3 4 5 6 7\n", encoding="utf-8")
            return 0, False, 0.01

        def fake_run_solver(
            _solver: Path,
            _in_path: Path,
            out_path: Path,
            time_path: Path,
            stderr_path: Path,
            _timeout: float | None,
            env=None,
            cwd=None,
        ) -> tuple[int, bool, float, int]:
            self.assertEqual(cwd, case_dir)
            self.assertEqual(env["DENSE_PROFILE_OUTDIR"], str(case_dir))
            out_path.write_text("0\n", encoding="utf-8")
            time_path.write_text("0.100000 123\n", encoding="utf-8")
            stderr_path.write_text("", encoding="utf-8")
            return 0, False, 0.1, 123

        with (
            mock.patch.object(branch_outer_certify, "run_cmd", side_effect=fake_run_cmd),
            mock.patch.object(branch_outer_certify, "run_solver_with_time", side_effect=fake_run_solver),
        ):
            row = branch_outer_certify.run_one_case(
                solver,
                out_dir,
                "correctness_fuzz",
                "comb_core",
                8,
                1,
                0,
                0,
                2.0,
            )

        self.assertEqual(row.case_dir, str(case_dir))
        self.assertEqual(len(run_cmd_calls), 2)
        self.assertEqual(run_cmd_calls[0]["cwd"], case_dir)
        self.assertEqual(run_cmd_calls[1]["cwd"], case_dir)
        self.assert_under_artifacts(run_cmd_calls[0]["cwd"])
        self.assert_under_artifacts(run_cmd_calls[1]["cwd"])
        shutil.rmtree(out_dir, ignore_errors=True)

    def test_main_passes_artifact_out_dir_to_mode_listing(self) -> None:
        out_dir = branch_outer_certify.resolve_certify_out_dir("unit/main_mode_listing")
        shutil.rmtree(out_dir, ignore_errors=True)
        preset = {
            "name": "unit",
            "stages": [
                {
                    "name": "correctness_fuzz",
                    "sizes": [8],
                    "seeds": [1],
                    "shuffle_labels": [0],
                    "shuffle_queries": [0],
                }
            ],
        }

        with (
            mock.patch.object(branch_outer_certify, "load_preset", return_value=preset),
            mock.patch.object(branch_outer_certify, "resolve_solver_path", return_value=Path("/bin/true")),
            mock.patch.object(branch_outer_certify, "ensure_executable"),
            mock.patch.object(branch_outer_certify, "mode_list_from_generator", return_value=["comb_core"]) as mode_mock,
            mock.patch.object(
                branch_outer_certify,
                "run_one_case",
                return_value=branch_outer_certify.Row(
                    "correctness_fuzz",
                    "comb_core",
                    8,
                    1,
                    0,
                    0,
                    1,
                    0,
                    0,
                    1,
                    0.1,
                    123,
                    str(out_dir / "runs" / "correctness_fuzz" / "comb_core" / "n8" / "seed1_L0_Q0"),
                ),
            ),
            mock.patch.object(
                branch_outer_certify.sys,
                "argv",
                ["branch_outer_certify.py", "--out", "unit/main_mode_listing"],
            ),
        ):
            rc = branch_outer_certify.main()

        self.assertEqual(rc, 0)
        self.assertEqual(mode_mock.call_args.kwargs["cwd"], out_dir)
        self.assert_under_artifacts(mode_mock.call_args.kwargs["cwd"])
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
