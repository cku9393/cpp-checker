#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import shutil
import unittest
from pathlib import Path
from unittest import mock

import branch_certify_suite


class BranchCertifySuiteCacheSignatureTests(unittest.TestCase):
    def setUp(self) -> None:
        self.out_dir = branch_certify_suite._normalize_artifact_out_dir(
            "certify_suite/unit/cache_signature_tests"
        )
        self.case_dir = branch_certify_suite._ensure_under_lca_tree_artifacts(
            branch_certify_suite._case_run_tmp_root() / "unit_cache_signature_case"
        )
        self.cache_dir = branch_certify_suite._cache_dir(self.out_dir, "comb_core", 8, 1, 0, 0)
        for path in (self.case_dir, self.cache_dir, self.out_dir):
            shutil.rmtree(path, ignore_errors=True)

    def tearDown(self) -> None:
        for path in (self.case_dir, self.cache_dir, self.out_dir):
            shutil.rmtree(path, ignore_errors=True)

    def _write_complete_cache_payload(self) -> None:
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        (self.cache_dir / "in.txt").write_text("8 0\n", encoding="utf-8")
        (self.cache_dir / "meta.json").write_text("{}\n", encoding="utf-8")
        (self.cache_dir / "hidden_parent.txt").write_text("0 1 2 3 4 5 6 7\n", encoding="utf-8")
        (self.cache_dir / "gen_stderr.txt").write_text("", encoding="utf-8")

    def test_cache_ready_requires_current_generator_signature(self) -> None:
        self._write_complete_cache_payload()
        self.assertFalse(
            branch_certify_suite._cache_ready(self.cache_dir),
            msg="cache entries without a generator signature must be rejected",
        )

        branch_certify_suite._write_cache_signature(self.cache_dir)
        self.assertTrue(
            branch_certify_suite._cache_ready(self.cache_dir),
            msg="cache entries with the current generator signature should be reusable",
        )

        (self.cache_dir / branch_certify_suite.GENERATOR_SIGNATURE_NAME).write_text(
            json.dumps({"schema": "wrong", "generator_digest": "stale", "files": []}) + "\n",
            encoding="utf-8",
        )
        self.assertFalse(
            branch_certify_suite._cache_ready(self.cache_dir),
            msg="cache entries with stale generator signatures must be invalidated",
        )

    def test_ensure_generated_case_rebuilds_stale_cache_entries(self) -> None:
        self._write_complete_cache_payload()
        (self.cache_dir / "in.txt").write_text("stale cache\n", encoding="utf-8")

        ok = branch_certify_suite._ensure_generated_case(
            self.out_dir, self.case_dir, "comb_core", 8, 1, 0, 0
        )

        self.assertTrue(ok)
        self.assertTrue(
            branch_certify_suite._cache_ready(self.cache_dir),
            msg="regenerated cache entries must carry the current generator signature",
        )
        self.assertTrue((self.case_dir / "in.txt").exists())
        self.assertNotEqual(
            (self.case_dir / "in.txt").read_text(encoding="utf-8"),
            "stale cache\n",
            msg="stale cached inputs must not be reused once the signature check fails",
        )

    def test_build_solver_env_snapshot_tracks_solver_and_profile_env(self) -> None:
        solver = branch_certify_suite.BRANCH_ROOT / "boj28350_resume" / "solve"
        env = {
            "PROFILE_MODE": "PROFILE_SAMPLED",
            "PROFILE_PROGRESS_STRIDE": "16",
            "ENABLE_LAYOUT_SIGNATURE_GATE_OPT": "1",
            "DENSE_SHADOW_CASE_MODE": "comb_core",
            "RUN_TAG": "certify_case",
        }

        snapshot = branch_certify_suite._build_solver_env_snapshot(solver, env)

        self.assertEqual(snapshot["schema"], "branch_certify_suite_solver_env_snapshot_v1")
        self.assertEqual(snapshot["solver"]["path"], str(solver))
        self.assertEqual(snapshot["tracked_env"]["PROFILE_MODE"], "PROFILE_SAMPLED")
        self.assertEqual(snapshot["tracked_env"]["ENABLE_LAYOUT_SIGNATURE_GATE_OPT"], "1")
        self.assertEqual(snapshot["tracked_env"]["RUN_TAG"], "certify_case")

    def test_write_solver_env_snapshot_rejects_non_artifact_case_dir(self) -> None:
        solver = branch_certify_suite.BRANCH_ROOT / "boj28350_resume" / "solve"
        env = {
            "PROFILE_MODE": "PROFILE_SAMPLED",
            "ENABLE_LAYOUT_SIGNATURE_GATE_OPT": "1",
            "RUN_TAG": "certify_case",
        }

        with self.assertRaisesRegex(ValueError, "must stay under"):
            branch_certify_suite._write_solver_env_snapshot(
                Path("/tmp/branch_certify_suite_snapshot_escape"),
                solver,
                env,
            )

    def test_write_case_result_rejects_non_artifact_case_dir(self) -> None:
        with self.assertRaisesRegex(ValueError, "must stay under"):
            branch_certify_suite._write_case_result(
                Path("/tmp/branch_certify_suite_result_escape"),
                status="pass",
                category="validator",
                exit_code=0,
                message="ok",
            )

    def test_write_cache_signature_rejects_non_artifact_cache_dir(self) -> None:
        with self.assertRaisesRegex(ValueError, "must stay under"):
            branch_certify_suite._write_cache_signature(
                Path("/tmp/branch_certify_suite_cache_escape")
            )

    def test_case_temp_roots_can_be_overridden_per_run(self) -> None:
        case_run_root = self.out_dir / ".case_runs_tmp"
        case_cache_root = self.out_dir / ".case_cache"
        case_cache_tmp_root = self.out_dir / ".case_cache_tmp"

        with mock.patch.dict(
            os.environ,
            {
                branch_certify_suite.BRANCH_CERTIFY_CASE_RUN_TMP_ROOT_ENV: str(case_run_root),
                branch_certify_suite.BRANCH_CERTIFY_CASE_CACHE_ROOT_ENV: str(case_cache_root),
                branch_certify_suite.BRANCH_CERTIFY_CASE_CACHE_TMP_ROOT_ENV: str(case_cache_tmp_root),
            },
            clear=False,
        ):
            self.assertEqual(branch_certify_suite._case_run_tmp_root(), case_run_root.resolve())
            self.assertEqual(branch_certify_suite._cache_root(self.out_dir), case_cache_root.resolve())
            self.assertEqual(
                branch_certify_suite._cache_tmp_root(self.out_dir),
                case_cache_tmp_root.resolve(),
            )

    def test_publish_case_dir_falls_back_to_copy_when_replace_errors(self) -> None:
        src_dir = self.out_dir / ".case_runs_tmp" / "publish_fallback_src"
        dst_dir = self.out_dir / "runs" / "correctness_fuzz" / "comb_rect_dense" / "n64" / "seed1_L0_Q0"
        shutil.rmtree(src_dir, ignore_errors=True)
        shutil.rmtree(dst_dir, ignore_errors=True)
        src_dir.mkdir(parents=True, exist_ok=True)
        (src_dir / "in.txt").write_text("64 0\n", encoding="utf-8")
        (src_dir / "out.txt").write_text("0\n", encoding="utf-8")

        original_replace = Path.replace

        def flaky_replace(self: Path, target: Path) -> Path:
            if self == src_dir:
                raise FileNotFoundError("simulated APFS rename failure")
            return original_replace(self, target)

        with mock.patch.object(Path, "replace", autospec=True, side_effect=flaky_replace):
            branch_certify_suite._publish_case_dir(src_dir, dst_dir)

        self.assertFalse(src_dir.exists(), msg="fallback publish should clean up the staged temp directory")
        self.assertTrue((dst_dir / "in.txt").exists(), msg="fallback publish should preserve generated inputs")
        self.assertTrue((dst_dir / "out.txt").exists(), msg="fallback publish should preserve solver outputs")

    def test_run_one_case_writes_directly_into_published_case_dir(self) -> None:
        out_dir = branch_certify_suite._normalize_artifact_out_dir(
            "certify_suite/unit/direct_case_dir"
        )
        case_run_root = out_dir / ".case_runs_tmp"
        case_dir = branch_certify_suite._ensure_under_lca_tree_artifacts(
            out_dir / "runs" / "correctness_fuzz" / "comb_core" / "n8" / "seed1_L0_Q0"
        )
        shutil.rmtree(out_dir, ignore_errors=True)
        solver = branch_certify_suite.BRANCH_ROOT / "boj28350_resume" / "solve"

        def fake_generate(
            _out_dir: Path,
            work_dir: Path,
            _mode: str,
            _n: int,
            _seed: int,
            _shuffle_labels: int,
            _shuffle_queries: int,
        ) -> bool:
            (work_dir / "in.txt").write_text("8 0\n", encoding="utf-8")
            (work_dir / "meta.json").write_text("{}\n", encoding="utf-8")
            (work_dir / "hidden_parent.txt").write_text("0 1 2 3 4 5 6 7\n", encoding="utf-8")
            (work_dir / "gen_stderr.txt").write_text("", encoding="utf-8")
            return True

        def fake_run_solver(
            _solver: Path,
            _in_path: Path,
            out_path: Path,
            time_path: Path,
            stderr_path: Path,
            _timeout: float | None,
            env: dict[str, str] | None = None,
            cwd: Path | None = None,
        ) -> tuple[int, bool, float, int]:
            self.assertEqual(cwd, case_dir)
            if env is not None:
                self.assertEqual(env["DENSE_PROFILE_OUTDIR"], str(case_dir))
            out_path.write_text("0\n", encoding="utf-8")
            time_path.write_text("0.100000 123\n", encoding="utf-8")
            stderr_path.write_text("", encoding="utf-8")
            return 0, False, 0.1, 123

        with mock.patch.dict(
            os.environ,
            {branch_certify_suite.BRANCH_CERTIFY_CASE_RUN_TMP_ROOT_ENV: str(case_run_root)},
            clear=False,
        ), mock.patch.object(
            branch_certify_suite, "_ensure_generated_case", side_effect=fake_generate
        ), mock.patch.object(
            branch_certify_suite, "branch_run_solver_with_time", side_effect=fake_run_solver
        ), mock.patch.object(
            branch_certify_suite, "validate_case", return_value=(True, "")
        ):
            row = branch_certify_suite.run_one_case(
                solver, out_dir, "correctness_fuzz", "comb_core", 8, 1, 0, 0, 2.0
            )

        self.assertEqual(row.case_dir, str(case_dir))
        self.assertTrue((case_dir / "in.txt").exists())
        self.assertTrue((case_dir / "out.txt").exists())
        self.assertTrue((case_dir / "time.txt").exists())
        self.assertTrue((case_dir / "run_case_result.json").exists())
        self.assertTrue((case_dir / "solver_env_snapshot.json").exists())
        result_payload = json.loads((case_dir / "run_case_result.json").read_text(encoding="utf-8"))
        self.assertEqual(result_payload["status"], "pass")
        self.assertTrue(result_payload["validator_ok"])
        self.assertFalse(case_run_root.exists(), msg="direct case execution should not depend on staging temp case dirs")
        shutil.rmtree(out_dir, ignore_errors=True)

    def test_run_one_case_publishes_result_marker_for_timeout_cases(self) -> None:
        out_dir = branch_certify_suite._normalize_artifact_out_dir(
            "certify_suite/unit/direct_case_timeout"
        )
        case_dir = branch_certify_suite._ensure_under_lca_tree_artifacts(
            out_dir / "runs" / "boj_3s_large_mix" / "balanced_dense" / "n99999" / "seed1_L1_Q1"
        )
        shutil.rmtree(out_dir, ignore_errors=True)
        solver = branch_certify_suite.BRANCH_ROOT / "boj28350_resume" / "solve"

        def fake_generate(
            _out_dir: Path,
            work_dir: Path,
            _mode: str,
            _n: int,
            _seed: int,
            _shuffle_labels: int,
            _shuffle_queries: int,
        ) -> bool:
            (work_dir / "in.txt").write_text("8 0\n", encoding="utf-8")
            (work_dir / "meta.json").write_text("{}\n", encoding="utf-8")
            (work_dir / "hidden_parent.txt").write_text("0 1 2 3 4 5 6 7\n", encoding="utf-8")
            (work_dir / "gen_stderr.txt").write_text("", encoding="utf-8")
            return True

        def fake_run_solver(
            _solver: Path,
            _in_path: Path,
            _out_path: Path,
            _time_path: Path,
            stderr_path: Path,
            _timeout: float | None,
            env: dict[str, str] | None = None,
            cwd: Path | None = None,
        ) -> tuple[int, bool, float | None, int | None]:
            self.assertEqual(cwd, case_dir)
            if env is not None:
                self.assertEqual(env["DENSE_PROFILE_OUTDIR"], str(case_dir))
            stderr_path.write_text("timed out\n", encoding="utf-8")
            return -9, True, None, None

        with mock.patch.object(
            branch_certify_suite, "_ensure_generated_case", side_effect=fake_generate
        ), mock.patch.object(
            branch_certify_suite, "branch_run_solver_with_time", side_effect=fake_run_solver
        ):
            row = branch_certify_suite.run_one_case(
                solver, out_dir, "boj_3s_large_mix", "balanced_dense", 99999, 1, 1, 1, 3.0
            )

        self.assertEqual(row.case_dir, str(case_dir))
        self.assertEqual(row.timed_out, 1)
        self.assertFalse((case_dir / "time.txt").exists())
        payload = json.loads((case_dir / "run_case_result.json").read_text(encoding="utf-8"))
        self.assertEqual(payload["status"], "solver_timeout")
        self.assertTrue(payload["timed_out"])
        self.assertFalse(payload["validator_ok"])
        shutil.rmtree(out_dir, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
