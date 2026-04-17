#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import textwrap
import unittest
from datetime import datetime
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent
HELPER_PATH = REPO_ROOT / ".ouroboros" / "prepare_retry_attempt_state.py"


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


class PrepareRetryAttemptStateTests(unittest.TestCase):
    def make_fake_branch(self, temp_root: Path) -> tuple[Path, Path]:
        branch_root = temp_root / "branch"
        artifacts_root = branch_root / "artifacts"
        write_text(
            branch_root / "artifact_paths.py",
            textwrap.dedent(
                """
                from pathlib import Path

                BRANCH_ROOT = Path(__file__).resolve().parent
                ARTIFACTS_ROOT = (BRANCH_ROOT / "artifacts").resolve()


                def artifacts_root():
                    return ARTIFACTS_ROOT


                def ensure_under_artifacts(path_like):
                    path = Path(path_like).resolve()
                    try:
                        path.relative_to(ARTIFACTS_ROOT)
                    except ValueError as exc:
                        raise ValueError(f"output path must stay under {ARTIFACTS_ROOT}: {path}") from exc
                    return path
                """
            ).strip()
            + "\n",
        )
        return branch_root, artifacts_root / "lca_tree_stress_v5"

    def canonical(self, path: Path) -> str:
        return str(path.resolve())

    def run_helper(
        self,
        branch_root: Path,
        attempt_dir: Path | str,
        report_root: Path | str,
        *,
        cwd: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(HELPER_PATH),
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
            ],
            check=False,
            capture_output=True,
            text=True,
            cwd=cwd,
        )

    def write_latest_failure_markdown(
        self,
        report_root: Path,
        *,
        attempt: int,
        session_id: str,
        execution_id: str,
        report_timestamp: str,
        breakdown_timestamp: str,
        failed_ac: str,
    ) -> None:
        write_text(
            report_root / "latest_failure_report.md",
            textwrap.dedent(
                f"""
                # Failure Report: Attempt {attempt}

                - Timestamp: `{report_timestamp}`
                - Session ID: `{session_id}`
                - Execution ID: `{execution_id}`

                ## Parsed AC Verdicts

                - Failed ACs: {failed_ac}
                """
            ).strip()
            + "\n",
        )
        write_text(
            report_root / "latest_failure_breakdown.md",
            textwrap.dedent(
                f"""
                # Failure Breakdown: Attempt {attempt}

                - Timestamp: `{breakdown_timestamp}`
                - Session ID: `{session_id}`
                - Execution ID: `{execution_id}`

                ## Failure Decomposition

                ### AC {failed_ac}: failure
                """
            ).strip()
            + "\n",
        )

    def write_current_analysis_state(
        self,
        branch_root: Path,
        *,
        attempt_label: str,
        session_id: str,
        execution_id: str,
        report_timestamp: str,
        breakdown_timestamp: str,
        failed_acs: list[str],
        current_for_latest_failure: bool = True,
        qualifying_refreshed_assets: list[Path] | None = None,
    ) -> Path:
        state_path = branch_root / ".ouroboros" / "failure_analysis_state.json"
        failure_signature = f"{attempt_label}|{session_id}|{breakdown_timestamp}|{','.join(failed_acs)}"
        state_real_path = state_path.resolve()
        refreshed_assets = [
            str(path.resolve()) for path in (qualifying_refreshed_assets or [state_path])
        ]
        designated_refresh_asset = next(
            (
                str(path.resolve())
                for path in (qualifying_refreshed_assets or [])
                if path.resolve() != state_real_path
            ),
            str(state_real_path),
        )
        write_text(
            state_path,
            json.dumps(
                {
                    "current_for_latest_failure": current_for_latest_failure,
                    "current_failure_attempt": attempt_label,
                    "current_failure_session_id": session_id,
                    "current_failure_execution_id": execution_id,
                    "current_failure_timestamp": breakdown_timestamp,
                    "current_failure_failed_acs": failed_acs,
                    "current_failure_signature": failure_signature,
                    "current_failure": {
                        "attempt_label": attempt_label,
                        "session_id": session_id,
                        "execution_id": execution_id,
                        "timestamp": breakdown_timestamp,
                        "failed_acs": failed_acs,
                        "failure_signature": failure_signature,
                    },
                    "refresh_evidence": {
                        "analysis_refresh_timestamp": "2026-04-02 12:30:00 KST",
                        "latest_failure_report_timestamp": report_timestamp,
                        "latest_failure_breakdown_timestamp": breakdown_timestamp,
                        "current_failure_timestamp": breakdown_timestamp,
                        "qualifying_refreshed_assets": refreshed_assets,
                        "freshness_record": {
                            "attempt_label": attempt_label,
                            "session_id": session_id,
                            "execution_id": execution_id,
                            "failure_timestamp": breakdown_timestamp,
                            "failure_signature": failure_signature,
                            "analysis_refresh_timestamp": "2026-04-02 12:30:00 KST",
                            "refreshed_asset": designated_refresh_asset,
                        },
                        "refreshed_after_failure_report": True,
                        "refreshed_after_failure_breakdown": True,
                        "refreshed_after_current_failure_timestamp": True,
                    },
                },
                indent=2,
            )
            + "\n",
        )
        return state_path

    def set_branch_timestamp_mtime(self, path: Path, timestamp: str) -> None:
        epoch = datetime.strptime(timestamp[:19], "%Y-%m-%d %H:%M:%S").timestamp()
        os.utime(path, (epoch, epoch))

    def test_pre_attempt_cleanup_clears_stale_gate_artifacts_and_preserves_retry_history(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "subac2_probe_attempt"
            pre_attempt_archive = attempt_dir / "pre_attempt_archive"

            write_text(lca_root / ".tmp" / "case_cache" / "cache.txt", "keep\n")
            write_text(lca_root / ".tmp" / "transient_dir" / "stale.txt", "drop\n")
            write_text(lca_root / ".locks" / "ac7_probe_lock" / "pid", "999999\n")
            write_text(lca_root / ".foo_in_progress.bar", "drop\n")
            write_text(lca_root / ".repeatability_stage" / "marker.txt", "drop\n")
            write_text(lca_root / "smoke_setup" / "stale.txt", "drop\n")
            write_text(lca_root / "smoke_launcher_latest_failure" / "stale.txt", "drop\n")
            write_text(lca_root / "smoke_latest_status" / "latest_status_report.md", "stale\n")
            write_text(lca_root / "strong_gate.latest_failure" / "stale.txt", "drop\n")
            write_text(lca_root / "boj3s_gate.latest_failure" / "stale.txt", "drop\n")
            write_text(lca_root / "probe.previous" / "stale.txt", "drop\n")
            write_text(retry_root / "keepme" / "sentinel.txt", "keep\n")
            write_text(lca_root / "strong_gate" / "certify.json", "{}\n")

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertIn("pre-attempt cleanup ok", result.stdout)
            self.assertTrue((lca_root / ".tmp" / "case_cache" / "cache.txt").exists())
            self.assertTrue((retry_root / "keepme" / "sentinel.txt").exists())
            self.assertTrue((lca_root / "strong_gate" / "certify.json").exists())
            self.assertTrue((pre_attempt_archive / "smoke_launcher_latest_failure" / "stale.txt").exists())
            self.assertTrue((pre_attempt_archive / "smoke_latest_status" / "latest_status_report.md").exists())
            self.assertTrue((pre_attempt_archive / "strong_gate.latest_failure" / "stale.txt").exists())
            self.assertTrue((pre_attempt_archive / "boj3s_gate.latest_failure" / "stale.txt").exists())

            for stale_path in (
                lca_root / ".tmp" / "transient_dir",
                lca_root / ".locks",
                lca_root / ".foo_in_progress.bar",
                lca_root / ".repeatability_stage",
                lca_root / "smoke_setup",
                lca_root / "smoke_launcher_latest_failure",
                lca_root / "smoke_latest_status",
                lca_root / "strong_gate.latest_failure",
                lca_root / "boj3s_gate.latest_failure",
                lca_root / "probe.previous",
            ):
                self.assertFalse(stale_path.exists(), msg=f"{stale_path} should have been removed")

            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            self.assertEqual(payload["status"], "ok")
            self.assertIn(self.canonical(lca_root / ".tmp" / "case_cache"), payload["preserved_paths"])
            self.assertIn(self.canonical(lca_root / "smoke_launcher_latest_failure"), payload["removed_paths"])
            self.assertIn(self.canonical(lca_root / "smoke_latest_status"), payload["removed_paths"])
            self.assertIn(self.canonical(lca_root / "strong_gate.latest_failure"), payload["removed_paths"])
            self.assertIn(self.canonical(lca_root / "boj3s_gate.latest_failure"), payload["removed_paths"])
            archived_sources = {entry["source"]: entry["archive"] for entry in payload["archived_paths"]}
            self.assertEqual(
                archived_sources[self.canonical(lca_root / "smoke_launcher_latest_failure")],
                self.canonical(pre_attempt_archive / "smoke_launcher_latest_failure"),
            )
            self.assertEqual(
                archived_sources[self.canonical(lca_root / "smoke_latest_status")],
                self.canonical(pre_attempt_archive / "smoke_latest_status"),
            )
            self.assertEqual(
                archived_sources[self.canonical(lca_root / "strong_gate.latest_failure")],
                self.canonical(pre_attempt_archive / "strong_gate.latest_failure"),
            )
            self.assertEqual(
                archived_sources[self.canonical(lca_root / "boj3s_gate.latest_failure")],
                self.canonical(pre_attempt_archive / "boj3s_gate.latest_failure"),
            )

    def test_pre_attempt_cleanup_recovers_from_invalid_tmp_and_lock_roots(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_invalid_roots"

            write_text(lca_root / ".tmp", "not a directory\n")
            write_text(lca_root / ".locks", "not a directory\n")
            write_text(lca_root / "strong_gate.latest_failure" / "stale.txt", "drop\n")

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertFalse((lca_root / ".tmp").exists())
            self.assertFalse((lca_root / ".locks").exists())
            self.assertFalse((lca_root / "strong_gate.latest_failure").exists())

            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            self.assertEqual(payload["status"], "ok")
            self.assertIn(self.canonical(lca_root / ".tmp"), payload["removed_paths"])
            self.assertIn(self.canonical(lca_root / ".locks"), payload["removed_paths"])

    def test_pre_attempt_cleanup_republishes_reports_over_directory_poisoned_targets(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_poisoned_reports"

            poisoned_paths = (
                attempt_dir / "pre_attempt_cleanup.json",
                attempt_dir / "pre_attempt_cleanup.md",
                retry_root / "latest_pre_attempt_cleanup.json",
                retry_root / "latest_pre_attempt_cleanup.md",
            )
            for path in poisoned_paths:
                path.mkdir(parents=True, exist_ok=True)

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            for path in poisoned_paths:
                self.assertTrue(path.is_file(), msg=f"{path} should be republished as a file")

    def test_pre_attempt_cleanup_resets_retry_control_files_but_keeps_failure_context_inputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_control_reset"

            for relpath in (
                "soft_stop_request.json",
                "quota_pause_state.json",
                "latest_workflow.log",
                "latest_runtime_snapshot.json",
                "latest_runtime_snapshot.md",
                "latest_quota_pause.json",
                "latest_quota_pause.md",
                "latest_quota_watch_status.json",
                "latest_quota_watch_status.md",
                "latest_manual_pause.json",
                "latest_manual_pause.md",
                "latest_retry_inputs_snapshot.json",
                "latest_retry_inputs_snapshot.md",
                "latest_solver_seed.snapshot",
                "latest_solver_seed.snapshot.yaml",
                "latest_analysis_seed.snapshot",
                "latest_analysis_seed.snapshot.yaml",
            ):
                write_text(retry_root / relpath, "stale control state\n")

            self.write_latest_failure_markdown(
                retry_root,
                attempt=5,
                session_id="orch_control_reset",
                execution_id="exec_control_reset",
                report_timestamp="2026-04-02 09:00:00 KST",
                breakdown_timestamp="2026-04-02 09:00:10 KST",
                failed_ac="4",
            )
            iteration_path = branch_root / ".ouroboros" / "failure_analysis_iteration.md"
            write_text(iteration_path, "# refreshed\n")
            self.write_current_analysis_state(
                branch_root,
                attempt_label="attempt_005",
                session_id="orch_control_reset",
                execution_id="exec_control_reset",
                report_timestamp="2026-04-02 09:00:00 KST",
                breakdown_timestamp="2026-04-02 09:00:10 KST",
                failed_acs=["4"],
                qualifying_refreshed_assets=[
                    branch_root / ".ouroboros" / "failure_analysis_state.json",
                    iteration_path,
                ],
            )
            preserved_inputs = (
                retry_root / "latest_failure_report.md",
                retry_root / "latest_failure_breakdown.md",
                retry_root / "latest_analysis_session.md",
                retry_root / "latest_next_probe_result.md",
                retry_root / "latest_attempt_guard.md",
                retry_root / "latest_git_repo_health.md",
            )
            for path in preserved_inputs[2:]:
                write_text(path, f"keep::{path.name}\n")

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            for relpath in (
                "soft_stop_request.json",
                "quota_pause_state.json",
                "latest_workflow.log",
                "latest_runtime_snapshot.json",
                "latest_runtime_snapshot.md",
                "latest_quota_pause.json",
                "latest_quota_pause.md",
                "latest_quota_watch_status.json",
                "latest_quota_watch_status.md",
                "latest_manual_pause.json",
                "latest_manual_pause.md",
                "latest_retry_inputs_snapshot.json",
                "latest_retry_inputs_snapshot.md",
                "latest_solver_seed.snapshot",
                "latest_solver_seed.snapshot.yaml",
                "latest_analysis_seed.snapshot",
                "latest_analysis_seed.snapshot.yaml",
            ):
                self.assertFalse((retry_root / relpath).exists(), msg=f"{relpath} should be cleared")

            for path in preserved_inputs:
                self.assertTrue(path.exists(), msg=f"{path.name} should be preserved")
            for path in preserved_inputs[2:]:
                self.assertEqual(path.read_text(encoding="utf-8"), f"keep::{path.name}\n")

            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            self.assertIn(self.canonical(retry_root / "soft_stop_request.json"), payload["removed_paths"])
            self.assertIn(self.canonical(retry_root / "latest_runtime_snapshot.json"), payload["removed_paths"])
            self.assertIn(self.canonical(retry_root / "latest_solver_seed.snapshot"), payload["removed_paths"])
            self.assertIn(self.canonical(retry_root / "latest_solver_seed.snapshot.yaml"), payload["removed_paths"])
            self.assertNotIn(self.canonical(retry_root / "latest_failure_report.md"), payload["removed_paths"])

    def test_pre_attempt_cleanup_clears_legacy_non_artifact_runtime_leaks(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_legacy_runtime_cleanup"

            legacy_paths = (
                branch_root / ".ouroboros" / "soft_stop_request.json",
                branch_root / ".ouroboros" / "quota_pause_state.json",
                branch_root / ".ouroboros" / "latest_workflow.log",
                branch_root / ".ouroboros" / "latest_runtime_snapshot.json",
                branch_root / ".ouroboros" / "latest_runtime_snapshot.md",
                branch_root / ".ouroboros" / "latest_analysis_refresh.log",
            )
            for path in legacy_paths:
                write_text(path, f"legacy::{path.name}\n")

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            for path in legacy_paths:
                self.assertFalse(path.exists(), msg=f"{path} should be cleared from .ouroboros")
                self.assertIn(self.canonical(path), payload["removed_paths"])

    def test_pre_attempt_cleanup_clears_legacy_non_artifact_analysis_refresh_attempt_logs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_analysis_refresh_log_cleanup"

            legacy_logs = (
                branch_root / ".ouroboros" / "analysis_refresh_attempt_023_subac2.log",
                branch_root / ".ouroboros" / "analysis_refresh_attempt_025_ac5_subac3.log",
            )
            for path in legacy_logs:
                write_text(path, f"legacy::{path.name}\n")

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            for path in legacy_logs:
                self.assertFalse(path.exists(), msg=f"{path} should be cleared from .ouroboros")
                self.assertIn(self.canonical(path), payload["removed_paths"])

    def test_pre_attempt_cleanup_still_blocks_live_gate_locks(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_active_lock"

            write_text(lca_root / ".locks" / "lca_strong_gate" / "pid", f"{os.getpid()}\n")

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 1)
            self.assertIn("live branch-local gate lock detected", result.stdout)
            self.assertTrue((lca_root / ".locks" / "lca_strong_gate" / "pid").exists())

            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            self.assertEqual(payload["status"], "blocked_active_lock")
            self.assertEqual(len(payload["active_locks"]), 1)
            self.assertEqual(payload["active_locks"][0]["pid"], os.getpid())

    def test_pre_attempt_cleanup_resolves_relative_artifact_roots_from_branch_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_relative_paths"

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir="artifacts/lca_tree_stress_v5/retry_loop/attempt_relative_paths",
                report_root="artifacts/lca_tree_stress_v5/retry_loop",
                cwd=temp_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue((attempt_dir / "pre_attempt_cleanup.json").exists())
            self.assertTrue((retry_root / "latest_pre_attempt_cleanup.json").exists())

    def test_pre_attempt_cleanup_blocks_when_latest_failure_analysis_state_is_stale(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_analysis_refresh_blocked"

            self.write_latest_failure_markdown(
                retry_root,
                attempt=7,
                session_id="orch_current",
                execution_id="exec_current",
                report_timestamp="2026-04-02 11:00:00 KST",
                breakdown_timestamp="2026-04-02 11:00:10 KST",
                failed_ac="4",
            )
            self.write_current_analysis_state(
                branch_root,
                attempt_label="attempt_006",
                session_id="orch_old",
                execution_id="exec_old",
                report_timestamp="2026-04-01 10:00:00 KST",
                breakdown_timestamp="2026-04-01 10:00:10 KST",
                failed_acs=["4"],
            )

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 1)
            self.assertIn("latest failed-attempt analysis refresh is missing or stale", result.stdout)
            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            self.assertEqual(payload["status"], "blocked_analysis_refresh")
            self.assertEqual(payload["analysis_refresh_gate"]["status"], "blocked")
            self.assertIn("does not match latest failure", payload["analysis_refresh_gate"]["reason"])

    def test_pre_attempt_cleanup_allows_retry_when_latest_failure_analysis_state_is_current(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_analysis_refresh_ok"
            state_path = branch_root / ".ouroboros" / "failure_analysis_state.json"
            iteration_path = branch_root / ".ouroboros" / "failure_analysis_iteration.md"

            self.write_latest_failure_markdown(
                retry_root,
                attempt=7,
                session_id="orch_current",
                execution_id="exec_current",
                report_timestamp="2026-04-02 11:00:00 KST",
                breakdown_timestamp="2026-04-02 11:00:10 KST",
                failed_ac="4",
            )
            write_text(iteration_path, "# refreshed\n")
            state_path = self.write_current_analysis_state(
                branch_root,
                attempt_label="attempt_007",
                session_id="orch_current",
                execution_id="exec_current",
                report_timestamp="2026-04-02 11:00:00 KST",
                breakdown_timestamp="2026-04-02 11:00:10 KST",
                failed_acs=["4"],
                qualifying_refreshed_assets=[state_path, iteration_path],
            )
            self.set_branch_timestamp_mtime(state_path, "2026-04-02 10:59:55 KST")
            self.set_branch_timestamp_mtime(iteration_path, "2026-04-02 11:00:20 KST")

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            self.assertEqual(payload["status"], "ok")
            self.assertEqual(payload["analysis_refresh_gate"]["status"], "ok")
            self.assertIn(
                f"{self.canonical(state_path)}:current_for_latest_failure",
                payload["analysis_refresh_gate"]["verified_markers"],
            )
            self.assertIn(
                f"{self.canonical(state_path)}:post_failure_refresh_asset={self.canonical(iteration_path)}",
                payload["analysis_refresh_gate"]["verified_markers"],
            )
            self.assertIn(
                f"{self.canonical(state_path)}:freshness_record.refreshed_asset={self.canonical(iteration_path)}",
                payload["analysis_refresh_gate"]["verified_markers"],
            )

    def test_pre_attempt_cleanup_allows_retry_when_current_state_is_valid_but_latest_analysis_session_is_stale(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_analysis_refresh_summary_stale"
            state_path = branch_root / ".ouroboros" / "failure_analysis_state.json"
            iteration_path = branch_root / ".ouroboros" / "failure_analysis_iteration.md"

            self.write_latest_failure_markdown(
                retry_root,
                attempt=17,
                session_id="orch_current",
                execution_id="exec_current",
                report_timestamp="2026-04-03 23:59:03 KST",
                breakdown_timestamp="2026-04-03 23:59:03 KST",
                failed_ac="3",
            )
            write_text(iteration_path, "# refreshed\n")
            state_path = self.write_current_analysis_state(
                branch_root,
                attempt_label="attempt_017",
                session_id="orch_current",
                execution_id="exec_current",
                report_timestamp="2026-04-03 23:59:03 KST",
                breakdown_timestamp="2026-04-03 23:59:03 KST",
                failed_acs=["3"],
                qualifying_refreshed_assets=[state_path, iteration_path],
            )
            self.set_branch_timestamp_mtime(state_path, "2026-04-03 23:59:04 KST")
            self.set_branch_timestamp_mtime(iteration_path, "2026-04-04 02:42:53 KST")
            write_text(
                retry_root / "latest_analysis_session.md",
                textwrap.dedent(
                    """
                    # Analysis Session Summary

                    - Timestamp: `2026-04-04 02:48:54 KST`
                    - Failed solver attempt: `17`
                    - Analysis seed: `.ouroboros/seed_branch3_failure_analysis.yaml`
                    - Analysis round: `3`
                    - Analysis log: `artifacts/lca_tree_stress_v5/retry_loop/attempt_017/analysis_workflow_round_03.log`
                    - Verification: `failed to refresh mandatory analysis assets`
                    """
                ).strip()
                + "\n",
            )

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            self.assertEqual(payload["status"], "ok")
            self.assertEqual(payload["analysis_refresh_gate"]["status"], "ok")
            self.assertIn(
                "does not confirm a refreshed latest-failure analysis session",
                payload["analysis_refresh_gate"]["analysis_session_warning"],
            )
            self.assertIn(
                f"{self.canonical(state_path)}:current_for_latest_failure",
                payload["analysis_refresh_gate"]["verified_markers"],
            )
            self.assertIn(
                f"{self.canonical(state_path)}:post_failure_refresh_asset={self.canonical(iteration_path)}",
                payload["analysis_refresh_gate"]["verified_markers"],
            )

    def test_pre_attempt_cleanup_blocks_when_current_analysis_state_has_no_designated_asset_newer_than_failure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, lca_root = self.make_fake_branch(temp_root)
            retry_root = lca_root / "retry_loop"
            attempt_dir = retry_root / "attempt_analysis_refresh_mtime_blocked"
            state_path = branch_root / ".ouroboros" / "failure_analysis_state.json"
            iteration_path = branch_root / ".ouroboros" / "failure_analysis_iteration.md"

            self.write_latest_failure_markdown(
                retry_root,
                attempt=7,
                session_id="orch_current",
                execution_id="exec_current",
                report_timestamp="2026-04-02 11:00:00 KST",
                breakdown_timestamp="2026-04-02 11:00:10 KST",
                failed_ac="4",
            )
            write_text(iteration_path, "# stale\n")
            state_path = self.write_current_analysis_state(
                branch_root,
                attempt_label="attempt_007",
                session_id="orch_current",
                execution_id="exec_current",
                report_timestamp="2026-04-02 11:00:00 KST",
                breakdown_timestamp="2026-04-02 11:00:10 KST",
                failed_acs=["4"],
                qualifying_refreshed_assets=[state_path, iteration_path],
            )
            self.set_branch_timestamp_mtime(state_path, "2026-04-02 10:59:55 KST")
            self.set_branch_timestamp_mtime(iteration_path, "2026-04-02 11:00:05 KST")

            result = self.run_helper(
                branch_root=branch_root,
                attempt_dir=attempt_dir,
                report_root=retry_root,
            )

            self.assertEqual(result.returncode, 1)
            self.assertIn("latest failed-attempt analysis refresh is missing or stale", result.stdout)
            payload = json.loads((attempt_dir / "pre_attempt_cleanup.json").read_text(encoding="utf-8"))
            self.assertEqual(payload["status"], "blocked_analysis_refresh")
            self.assertEqual(payload["analysis_refresh_gate"]["status"], "blocked")
            self.assertIn(
                "no designated analysis asset is newer than latest failed attempt timestamp",
                payload["analysis_refresh_gate"]["reason"],
            )


if __name__ == "__main__":
    unittest.main()
