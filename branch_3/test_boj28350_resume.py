#!/usr/bin/env python3
from __future__ import annotations

import shutil
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

import boj28350_resume
from artifact_paths import artifacts_root


class Boj28350ResumeArtifactLocalityTests(unittest.TestCase):
    def assert_under_artifacts(self, value: str | Path) -> Path:
        path = Path(value).resolve()
        try:
            path.relative_to(artifacts_root())
        except ValueError:
            self.fail(f"path escaped artifacts root: {path}")
        return path

    def test_build_solver_always_passes_artifact_rooted_output(self) -> None:
        with mock.patch.object(
            boj28350_resume.subprocess,
            "run",
            return_value=SimpleNamespace(returncode=0),
        ) as run_mock:
            rc = boj28350_resume.build_solver(None, "auto", [], None, None)

        self.assertEqual(rc, 0)
        cmd = run_mock.call_args.args[0]
        out_path = Path(cmd[cmd.index("--out") + 1])
        self.assertEqual(
            out_path,
            artifacts_root() / "boj28350_resume" / "build" / boj28350_resume.DEFAULT_SOLVER.name,
        )
        self.assert_under_artifacts(out_path)

    def test_build_solver_executes_from_artifact_build_directory(self) -> None:
        build_root = artifacts_root() / "boj28350_resume" / "build"
        shutil.rmtree(build_root, ignore_errors=True)
        with mock.patch.object(
            boj28350_resume.subprocess,
            "run",
            return_value=SimpleNamespace(returncode=0),
        ) as run_mock:
            rc = boj28350_resume.build_solver(None, "auto", [], None, None)

        self.assertEqual(rc, 0)
        self.assertEqual(run_mock.call_args.kwargs["cwd"], build_root)
        self.assertTrue(build_root.is_dir())
        self.assert_under_artifacts(build_root)
        shutil.rmtree(build_root, ignore_errors=True)

    def test_resolve_smoke_out_dir_accepts_branch_root_prefixed_artifact_path(self) -> None:
        branch_name = boj28350_resume.ROOT.name
        resolved = boj28350_resume.resolve_smoke_out_dir(
            f"{branch_name}/artifacts/boj28350_resume/smoke/unit_branch_prefixed"
        )

        self.assertEqual(
            resolved,
            artifacts_root() / "boj28350_resume" / "smoke" / "unit_branch_prefixed",
        )
        self.assert_under_artifacts(resolved)

    def test_resolve_smoke_case_dir_sanitizes_manifest_tokens(self) -> None:
        tag, case_dir = boj28350_resume.resolve_smoke_case_dir(
            "unit/resume_case_dir_guard",
            {
                "stage": "../escape stage",
                "mode": "comb/core",
                "n": "4",
                "seed": "7",
                "shuffle_labels": "0",
                "shuffle_queries": "1",
                "timeout_s": "2.5",
            },
            5.0,
        )

        self.assertEqual(tag, "escape_stage_comb_core_4_s7_L0_Q1_t2p5")
        self.assertEqual(case_dir.name, tag)
        self.assertNotIn("..", tag)
        self.assert_under_artifacts(case_dir)

    def test_resolve_smoke_case_dir_timeout_override_retags_case_dir(self) -> None:
        tag, case_dir = boj28350_resume.resolve_smoke_case_dir(
            "unit/resume_case_dir_timeout_override",
            {
                "stage": "correctness_fuzz",
                "mode": "comb_rect_dense",
                "n": "1024",
                "seed": "1",
                "shuffle_labels": "0",
                "shuffle_queries": "0",
                "timeout_s": "2.0",
            },
            5.0,
            8.0,
        )

        self.assertEqual(tag, "correctness_fuzz_comb_rect_dense_1024_s1_L0_Q0_t8")
        self.assertEqual(case_dir.name, tag)
        self.assert_under_artifacts(case_dir)

    def test_run_case_roots_relative_base_out_and_solver_profile_dir_under_artifacts(self) -> None:
        base_out = boj28350_resume.resolve_smoke_out_dir("unit/resume_artifact_locality")
        shutil.rmtree(base_out, ignore_errors=True)
        row = {
            "stage": "unit stage",
            "mode": "comb_core",
            "n": "4",
            "seed": "7",
            "shuffle_labels": "0",
            "shuffle_queries": "0",
        }
        stale_case_dir = base_out / "unit_stage_comb_core_4_s7_L0_Q0_t5"
        stale_case_dir.mkdir(parents=True, exist_ok=True)
        (stale_case_dir / "stale.txt").write_text("old\n", encoding="utf-8")
        with (
            mock.patch.object(
                boj28350_resume.branch_gen_case,
                "build_mode",
                return_value=([0, 0, 1, 1, 3], [(2, 4, 1)], {"mode": "comb_core"}),
            ),
            mock.patch.object(
                boj28350_resume,
                "run_solver_with_time",
                return_value=(0, False, 0.01, 64),
            ) as run_mock,
            mock.patch.object(
                boj28350_resume,
                "validate_case",
                return_value=(True, "ok"),
            ),
        ):
            result = boj28350_resume.run_case(row, Path("/bin/true"), Path("unit/resume_artifact_locality"), 5.0)

        case_dir = self.assert_under_artifacts(result["case_dir"])
        self.assertEqual(case_dir.parent, base_out)
        self.assertFalse((case_dir / "stale.txt").exists())
        self.assertEqual(run_mock.call_args.kwargs["cwd"], case_dir)
        self.assertEqual(run_mock.call_args.kwargs["env"]["DENSE_PROFILE_OUTDIR"], str(case_dir))
        self.assertEqual(run_mock.call_args.kwargs["env"]["DENSE_SHADOW_CASE_SHUFFLE_LABELS"], "0")
        self.assertEqual(run_mock.call_args.kwargs["env"]["DENSE_SHADOW_CASE_SHUFFLE_QUERIES"], "0")
        shutil.rmtree(base_out, ignore_errors=True)

    def test_run_case_timeout_override_applies_without_mutating_manifest_row(self) -> None:
        base_out = boj28350_resume.resolve_smoke_out_dir("unit/resume_timeout_override")
        shutil.rmtree(base_out, ignore_errors=True)
        row = {
            "stage": "correctness_fuzz",
            "mode": "caterpillar_rect_dense",
            "n": "512",
            "seed": "1",
            "shuffle_labels": "1",
            "shuffle_queries": "0",
            "timeout_s": "2.0",
        }
        with (
            mock.patch.object(
                boj28350_resume.branch_gen_case,
                "build_mode",
                return_value=([0, 0, 1, 2, 3], [(2, 4, 1)], {"mode": "caterpillar_rect_dense"}),
            ),
            mock.patch.object(
                boj28350_resume,
                "run_solver_with_time",
                return_value=(0, False, 0.25, 128),
            ) as run_mock,
            mock.patch.object(
                boj28350_resume,
                "validate_case",
                return_value=(True, "ok"),
            ),
        ):
            result = boj28350_resume.run_case(
                row,
                Path("/bin/true"),
                Path("unit/resume_timeout_override"),
                5.0,
                8.0,
            )

        self.assertEqual(row["timeout_s"], "2.0")
        self.assertEqual(result["timeout_s"], "8")
        self.assertEqual(run_mock.call_args.kwargs["timeout"], 8.0)
        self.assertIn("_t8", Path(result["case_dir"]).name)
        shutil.rmtree(base_out, ignore_errors=True)

    def test_write_summary_roots_relative_summary_paths_under_smoke_artifacts(self) -> None:
        summary_root = boj28350_resume.resolve_smoke_out_dir("unit/resume_summary_locality")
        shutil.rmtree(summary_root, ignore_errors=True)
        try:
            boj28350_resume.write_summary(
                [
                    {
                        "stage": "unit",
                        "mode": "comb_core",
                        "n": "4",
                        "seed": "1",
                        "shuffle_labels": "0",
                        "shuffle_queries": "0",
                        "timeout_s": "5",
                        "rc": "0",
                        "validator": "OK",
                        "validator_msg": "ok",
                        "elapsed_s": "0.01",
                        "mem_kb": "64",
                        "case_dir": str(summary_root / "unit_case"),
                    }
                ],
                Path("unit/resume_summary_locality/custom_summary.tsv"),
            )
            summary_path = self.assert_under_artifacts(summary_root / "custom_summary.tsv")
            self.assertTrue(summary_path.exists())
        finally:
            shutil.rmtree(summary_root, ignore_errors=True)

    def test_snapshot_smoke_cases_copies_manifest_under_smoke_output_root(self) -> None:
        summary_root = boj28350_resume.resolve_smoke_out_dir("unit/resume_cases_snapshot")
        source_path = summary_root.parent / "source_cases.tsv"
        shutil.rmtree(summary_root, ignore_errors=True)
        source_path.parent.mkdir(parents=True, exist_ok=True)
        source_path.write_text("stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n", encoding="utf-8")
        try:
            snapshot_path = boj28350_resume.snapshot_smoke_cases(source_path, summary_root)
            self.assertEqual(snapshot_path, summary_root / boj28350_resume.DEFAULT_SMOKE_CASES_SNAPSHOT_NAME)
            self.assert_under_artifacts(snapshot_path)
            self.assertEqual(snapshot_path.read_text(encoding="utf-8"), source_path.read_text(encoding="utf-8"))
        finally:
            shutil.rmtree(summary_root, ignore_errors=True)
            source_path.unlink(missing_ok=True)


class Boj28350ResumeRegressionManifestTests(unittest.TestCase):
    REGRESSION_MANIFEST = (
        boj28350_resume.ROOT
        / "artifacts"
        / "lca_tree_stress_v5"
        / "retry_loop"
        / "ac3_timeout_regression_cases.tsv"
    )
    REGRESSION_FIXTURE_MANIFEST = (
        boj28350_resume.ROOT
        / "artifacts"
        / "lca_tree_stress_v5"
        / "retry_loop"
        / "ac3_timeout_regression_runs_v2"
        / "smoke_cases.snapshot.tsv"
    )
    REGRESSION_FIXTURE_DIR = (
        boj28350_resume.ROOT
        / "artifacts"
        / "lca_tree_stress_v5"
        / "retry_loop"
        / "ac3_timeout_regression_runs_v2"
        / "correctness_fuzz_caterpillar_rect_dense_512_s1_L1_Q0_t2"
    )

    def test_ac3_timeout_regression_manifest_tracks_current_timeout_corridor(self) -> None:
        rows = boj28350_resume.parse_cases(self.REGRESSION_MANIFEST)

        self.assertEqual(len(rows), 7)
        self.assertTrue(all(row["stage"] == "correctness_fuzz" for row in rows))
        self.assertTrue(all(row["timeout_s"] == "2.0" for row in rows))
        self.assertTrue(all(row["source_outcome"] == "timeout" for row in rows))

        actual = {
            (
                row["mode"],
                int(row["n"]),
                int(row["seed"]),
                int(row["shuffle_labels"]),
                int(row["shuffle_queries"]),
                row["cluster_role"],
            )
            for row in rows
        }
        expected = {
            ("comb_rect_dense", 1024, 1, 0, 0, "primary_confirmed_timeout"),
            ("comb_rect_dense", 1024, 1, 1, 1, "primary_confirmed_timeout"),
            ("multi_comb_rect", 1024, 1, 1, 0, "primary_confirmed_timeout"),
            ("multi_comb_rect", 1024, 1, 1, 1, "primary_confirmed_timeout"),
            ("caterpillar_rect_dense", 1024, 1, 0, 0, "primary_confirmed_timeout"),
            ("caterpillar_rect_dense", 1024, 1, 1, 1, "primary_confirmed_timeout"),
            ("caterpillar_rect_dense", 512, 1, 1, 0, "secondary_label_sensitive_timeout"),
        }

        self.assertEqual(actual, expected)

        roles = {row["cluster_role"] for row in rows}
        self.assertEqual(roles, {"primary_confirmed_timeout", "secondary_label_sensitive_timeout"})
        primary_source_dirs = {
            Path(row["source_case_dir"]).as_posix()
            for row in rows
            if row["cluster_role"] == "primary_confirmed_timeout"
        }
        self.assertTrue(
            all(
                source_dir.startswith(
                    "artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_probe_v3.latest_failure/runs/"
                )
                for source_dir in primary_source_dirs
            )
        )
        self.assertTrue(
            all((boj28350_resume.ROOT / Path(row["source_case_dir"])).exists() for row in rows),
            msg="timeout regression rows must point at preserved in-tree failure bundles",
        )
        label_sensitive_row = next(row for row in rows if row["cluster_role"] == "secondary_label_sensitive_timeout")
        self.assertEqual(
            Path(label_sensitive_row["source_case_dir"]).as_posix(),
            "artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_probe_v3.latest_failure/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed1_L1_Q0",
        )
        self.assertIn("smallest label-sensitive timeout", label_sensitive_row["why_selected"])

    def test_saved_ac3_timeout_fixture_snapshot_covers_label_sensitive_caterpillar_case(self) -> None:
        rows = boj28350_resume.parse_cases(self.REGRESSION_FIXTURE_MANIFEST)

        matches = [
            row
            for row in rows
            if (
                row["mode"],
                row["n"],
                row["seed"],
                row["shuffle_labels"],
                row["shuffle_queries"],
                row["cluster_role"],
            )
            == ("caterpillar_rect_dense", "512", "1", "1", "0", "secondary_label_sensitive_timeout")
        ]
        self.assertEqual(len(matches), 1)
        self.assertTrue(self.REGRESSION_FIXTURE_DIR.exists())
        for leaf in ("hidden_parent.txt", "in.txt", "meta.json", "out.txt", "solver_stderr.txt"):
            self.assertTrue((self.REGRESSION_FIXTURE_DIR / leaf).is_file(), msg=f"missing saved regression fixture file: {leaf}")


class Boj28350ResumeStrongGateRegressionCaseTests(unittest.TestCase):
    REGRESSION_MANIFEST = (
        boj28350_resume.ROOT
        / "artifacts"
        / "lca_tree_stress_v5"
        / "retry_loop"
        / "ac3_strong_gate_regression_case.tsv"
    )

    def test_ac3_strong_gate_regression_case_stays_pinned_to_archived_timeout_row(self) -> None:
        rows = boj28350_resume.parse_cases(self.REGRESSION_MANIFEST)

        self.assertEqual(len(rows), 1)
        row = rows[0]
        self.assertEqual(row["stage"], "correctness_fuzz")
        self.assertEqual(row["mode"], "comb_rect_dense")
        self.assertEqual(row["n"], "1024")
        self.assertEqual(row["seed"], "1")
        self.assertEqual(row["shuffle_labels"], "0")
        self.assertEqual(row["shuffle_queries"], "0")
        self.assertEqual(row["timeout_s"], "2.0")
        self.assertEqual(row["cluster_role"], "primary_previous_gate_timeout")
        self.assertEqual(row["source_outcome"], "timeout")
        self.assertEqual(
            Path(row["source_failure_report"]).as_posix(),
            "artifacts/lca_tree_stress_v5/strong_gate.failure_archive/strong_gate.latest_failure.20260402_225011/latest_failure_report.md",
        )
        self.assertEqual(
            Path(row["source_certify_rows"]).as_posix(),
            "artifacts/lca_tree_stress_v5/strong_gate.failure_archive/strong_gate.latest_failure.20260402_225011/certify_rows.csv",
        )
        self.assertEqual(
            Path(row["source_case_dir"]).as_posix(),
            "artifacts/lca_tree_stress_v5/strong_gate.failure_archive/strong_gate.latest_failure.20260402_225011/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L0_Q0",
        )
        self.assertEqual(
            Path(row["saved_fixture_dir"]).as_posix(),
            "artifacts/lca_tree_stress_v5/retry_loop/ac3_strong_gate_regression_current/correctness_fuzz_comb_rect_dense_1024_s1_L0_Q0_t2",
        )
        self.assertTrue((boj28350_resume.ROOT / Path(row["source_failure_report"])).exists())
        self.assertTrue((boj28350_resume.ROOT / Path(row["source_certify_rows"])).exists())
        self.assertTrue((boj28350_resume.ROOT / Path(row["source_case_dir"])).exists())
        self.assertTrue((boj28350_resume.ROOT / Path(row["saved_fixture_dir"])).exists())
        self.assertIn("saved retry-loop fixture", row["why_selected"])
        self.assertIn("smallest saved repro", row["why_selected"])


if __name__ == "__main__":
    unittest.main()
