#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent
HELPER_PATH = REPO_ROOT / ".ouroboros" / "snapshot_retry_inputs.py"


class SnapshotRetryInputsTests(unittest.TestCase):
    def write_research_review_baseline(
        self,
        branch_root: Path,
        *,
        source_set_a_complete: bool = True,
        source_set_b_complete: bool = True,
    ) -> None:
        baseline_files = {
            Path("README.md"): "# branch workspace guide\n",
            Path("boj28350_resume/README.md"): "resume readme\n",
            Path("boj28350_resume/current_state_summary.md"): "current state\n",
            Path("boj28350_resume/next_session_briefing.md"): "briefing\n",
            Path("boj28350_resume/progress40_derived_reference.md"): "progress40 reference\n",
            Path("boj28350_complete_master_document_partA_raw.md"): "master doc\n",
            Path("boj28350_integrated_technical_history.md"): "integrated history\n",
            Path("boj28350_literature_progress7_bcdecomp_report.md"): "progress7 report\n",
            Path("literature_grade_proof_package.md"): "proof package\n",
            Path(
                "boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp"
            ): "int main() { return 0; }\n",
            Path(
                "boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md"
            ): "progress40 report\n",
            Path("boj28350_bundle_archive/boj28350_progress40_results_merged.json"): "{}\n",
        }
        checkpoint_text = textwrap.dedent(
            f"""
            # Pre-rewrite checkpoint
            reviewed source set A: {'COMPLETE' if source_set_a_complete else 'pending'}
            reviewed source set B: {'COMPLETE' if source_set_b_complete else 'pending'}
            """
        ).strip() + "\n"
        baseline_files[Path("boj28350_resume/pre_rewrite_checkpoint.md")] = checkpoint_text
        baseline_files[Path("boj28350_resume/pre_rewrite_synthesis_note.md")] = checkpoint_text
        for relative_path, content in baseline_files.items():
            path = branch_root / relative_path
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")

    def make_fake_branch(self, temp_root: Path) -> tuple[Path, Path]:
        branch_root = temp_root / "branch"
        artifacts_root = branch_root / "artifacts"
        branch_root.mkdir(parents=True, exist_ok=True)
        artifacts_root.mkdir(parents=True, exist_ok=True)
        (branch_root / "artifact_paths.py").write_text(
            (
                "from pathlib import Path\n"
                "BRANCH_ROOT = Path(__file__).resolve().parent\n"
                "ARTIFACTS_ROOT = (BRANCH_ROOT / 'artifacts').resolve()\n"
                "def artifacts_root():\n"
                "    return ARTIFACTS_ROOT\n"
                "def ensure_under_artifacts(path_like):\n"
                "    path = Path(path_like).resolve()\n"
                "    try:\n"
                "        path.relative_to(ARTIFACTS_ROOT)\n"
                "    except ValueError as exc:\n"
                "        raise ValueError(f'output path must stay under {ARTIFACTS_ROOT}: {path}') from exc\n"
                "    return path\n"
            ),
            encoding="utf-8",
        )
        self.write_research_review_baseline(branch_root)
        return branch_root, artifacts_root

    def run_helper(self, *args: str, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(HELPER_PATH), *args],
            check=False,
            capture_output=True,
            text=True,
            cwd=cwd,
        )

    def test_snapshot_copies_seed_inputs_into_attempt_and_latest_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_001"
            solver_seed = branch_root / ".ouroboros" / "seed_branch3_progress40_research_loop.yaml"
            analysis_seed = branch_root / ".ouroboros" / "seed_branch3_failure_analysis.yaml"
            solver_seed.parent.mkdir(parents=True, exist_ok=True)
            solver_seed.write_text("solver: 1\nseed: alpha\n", encoding="utf-8")
            analysis_seed.write_text("analysis: 1\nseed: beta\n", encoding="utf-8")

            result = self.run_helper(
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--seed-file",
                str(solver_seed),
                "--analysis-seed-file",
                str(analysis_seed),
                "--attempt-number",
                "1",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            payload = json.loads((attempt_dir / "retry_inputs_snapshot.json").read_text(encoding="utf-8"))
            self.assertEqual(payload["attempt_number"], 1)
            self.assertEqual(payload["research_review_gate"]["status"], "validated")
            self.assertTrue(payload["research_review_gate"]["source_set_a_paths"])
            self.assertTrue(payload["research_review_gate"]["source_set_b_paths"])

            solver_entry = payload["inputs"]["solver_seed"]
            analysis_entry = payload["inputs"]["analysis_seed"]
            self.assertEqual(
                Path(solver_entry["attempt_snapshot"]).read_text(encoding="utf-8"),
                solver_seed.read_text(encoding="utf-8"),
            )
            self.assertEqual(
                Path(analysis_entry["attempt_snapshot"]).read_text(encoding="utf-8"),
                analysis_seed.read_text(encoding="utf-8"),
            )
            self.assertEqual(
                solver_entry["sha256"],
                hashlib.sha256(solver_seed.read_bytes()).hexdigest(),
            )
            self.assertTrue((report_root / "latest_retry_inputs_snapshot.json").exists())
            self.assertTrue((report_root / "latest_solver_seed.snapshot.yaml").exists())
            self.assertTrue((report_root / "latest_analysis_seed.snapshot.yaml").exists())

    def test_snapshot_rejects_seed_source_outside_branch_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_002"
            external_seed = temp_root / "external_seed.yaml"
            external_seed.write_text("seed: escape\n", encoding="utf-8")

            result = self.run_helper(
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--seed-file",
                str(external_seed),
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertIn("seed input must stay under", result.stderr)
            self.assertFalse((attempt_dir / "retry_inputs_snapshot.json").exists())

    def test_snapshot_resolves_relative_attempt_and_report_roots_from_branch_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_003"
            solver_seed = branch_root / ".ouroboros" / "seed_branch3_progress40_research_loop.yaml"
            solver_seed.parent.mkdir(parents=True, exist_ok=True)
            solver_seed.write_text("solver: 1\nseed: gamma\n", encoding="utf-8")

            result = self.run_helper(
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                "artifacts/lca_tree_stress_v5/retry_loop/attempt_003",
                "--report-root",
                "artifacts/lca_tree_stress_v5/retry_loop",
                "--seed-file",
                ".ouroboros/seed_branch3_progress40_research_loop.yaml",
                cwd=temp_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue((attempt_dir / "retry_inputs_snapshot.json").exists())
            self.assertTrue((report_root / "latest_retry_inputs_snapshot.json").exists())

    def test_snapshot_accepts_branch_prefixed_paths_without_shared_resolver(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_003_branch_prefixed"
            solver_seed = branch_root / ".ouroboros" / "seed_branch3_progress40_research_loop.yaml"
            solver_seed.parent.mkdir(parents=True, exist_ok=True)
            solver_seed.write_text("solver: 1\nseed: gamma-branch-prefixed\n", encoding="utf-8")

            result = self.run_helper(
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                f"{branch_root.name}/artifacts/lca_tree_stress_v5/retry_loop/attempt_003_branch_prefixed",
                "--report-root",
                f"{branch_root.name}/artifacts/lca_tree_stress_v5/retry_loop",
                "--seed-file",
                f"{branch_root.name}/.ouroboros/seed_branch3_progress40_research_loop.yaml",
                cwd=temp_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue((attempt_dir / "retry_inputs_snapshot.json").exists())
            self.assertTrue((report_root / "latest_retry_inputs_snapshot.json").exists())

    def test_snapshot_republishes_over_directory_poisoned_targets(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_004"
            solver_seed = branch_root / ".ouroboros" / "seed_branch3_progress40_research_loop.yaml"
            analysis_seed = branch_root / ".ouroboros" / "seed_branch3_failure_analysis.yaml"
            solver_seed.parent.mkdir(parents=True, exist_ok=True)
            solver_seed.write_text("solver: 1\nseed: delta\n", encoding="utf-8")
            analysis_seed.write_text("analysis: 1\nseed: epsilon\n", encoding="utf-8")

            poisoned_paths = (
                attempt_dir / "solver_seed.snapshot.yaml",
                attempt_dir / "analysis_seed.snapshot.yaml",
                attempt_dir / "retry_inputs_snapshot.json",
                attempt_dir / "retry_inputs_snapshot.md",
                report_root / "latest_solver_seed.snapshot.yaml",
                report_root / "latest_analysis_seed.snapshot.yaml",
                report_root / "latest_retry_inputs_snapshot.json",
                report_root / "latest_retry_inputs_snapshot.md",
            )
            for path in poisoned_paths:
                path.mkdir(parents=True, exist_ok=True)

            result = self.run_helper(
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--seed-file",
                str(solver_seed),
                "--analysis-seed-file",
                str(analysis_seed),
                "--attempt-number",
                "4",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            for path in poisoned_paths:
                self.assertTrue(path.is_file(), msg=f"{path} should be republished as a file")

    def test_snapshot_replaces_stale_legacy_snapshot_variants_without_manual_cleanup(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_005"
            solver_seed = branch_root / ".ouroboros" / "seed_branch3_progress40_research_loop.yaml"
            analysis_seed = branch_root / ".ouroboros" / "seed_branch3_failure_analysis.yaml"
            solver_seed.parent.mkdir(parents=True, exist_ok=True)
            solver_seed.write_text("solver: 1\nseed: zeta\n", encoding="utf-8")
            analysis_seed.write_text("analysis: 1\nseed: eta\n", encoding="utf-8")

            stale_variants = (
                report_root / "latest_solver_seed.snapshot",
                report_root / "latest_analysis_seed.snapshot",
                attempt_dir / "solver_seed.snapshot",
                attempt_dir / "analysis_seed.snapshot",
            )
            for path in stale_variants:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("stale legacy snapshot\n", encoding="utf-8")

            result = self.run_helper(
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--seed-file",
                str(solver_seed),
                "--analysis-seed-file",
                str(analysis_seed),
                "--attempt-number",
                "5",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            for path in stale_variants:
                self.assertFalse(path.exists(), msg=f"{path} should be removed as a stale legacy variant")
            self.assertTrue((report_root / "latest_solver_seed.snapshot.yaml").exists())
            self.assertTrue((report_root / "latest_analysis_seed.snapshot.yaml").exists())
            self.assertTrue((attempt_dir / "solver_seed.snapshot.yaml").exists())
            self.assertTrue((attempt_dir / "analysis_seed.snapshot.yaml").exists())

    def test_snapshot_blocks_when_pre_rewrite_review_checkpoint_is_incomplete(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_006"
            solver_seed = branch_root / ".ouroboros" / "seed_branch3_progress40_research_loop.yaml"
            solver_seed.parent.mkdir(parents=True, exist_ok=True)
            solver_seed.write_text("solver: 1\nseed: theta\n", encoding="utf-8")
            self.write_research_review_baseline(branch_root, source_set_b_complete=False)

            result = self.run_helper(
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--seed-file",
                str(solver_seed),
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertIn(
                "pre-rewrite research review evidence must record both",
                result.stderr,
            )
            self.assertFalse((attempt_dir / "retry_inputs_snapshot.json").exists())


if __name__ == "__main__":
    unittest.main()
