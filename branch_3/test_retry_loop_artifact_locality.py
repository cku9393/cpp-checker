#!/usr/bin/env python3
from __future__ import annotations

import json
import importlib.util
import shlex
import shutil
import tempfile
import subprocess
import sys
import unittest
from pathlib import Path
from unittest import mock

import artifact_paths
from artifact_paths import artifacts_root


REPO_ROOT = Path(__file__).resolve().parent
RUN_UNTIL_PASS_SOURCE = (REPO_ROOT / ".ouroboros" / "run_until_pass_progress40.sh").read_text(
    encoding="utf-8"
)
REQUEST_SOFT_STOP_SOURCE = (REPO_ROOT / ".ouroboros" / "request_soft_stop.py").read_text(
    encoding="utf-8"
)
MONITOR_QUOTA_SOURCE = (REPO_ROOT / ".ouroboros" / "monitor_codex_quota.py").read_text(
    encoding="utf-8"
)
SNAPSHOT_RUNTIME_SOURCE = (REPO_ROOT / ".ouroboros" / "snapshot_retry_runtime.py").read_text(
    encoding="utf-8"
)
SNAPSHOT_INPUTS_SOURCE = (REPO_ROOT / ".ouroboros" / "snapshot_retry_inputs.py").read_text(
    encoding="utf-8"
)
REFRESH_ANALYSIS_STATE_SOURCE = (
    REPO_ROOT / ".ouroboros" / "refresh_analysis_state.py"
).read_text(encoding="utf-8")
CAPTURE_FAILURE_CONTEXT_SOURCE = (
    REPO_ROOT / ".ouroboros" / "capture_failure_context.py"
).read_text(encoding="utf-8")
REPAIR_GIT_AND_PUSH_SOURCE = (REPO_ROOT / ".ouroboros" / "repair_git_and_push.py").read_text(
    encoding="utf-8"
)
PREPARE_RETRY_ATTEMPT_STATE_SOURCE = (
    REPO_ROOT / ".ouroboros" / "prepare_retry_attempt_state.py"
).read_text(encoding="utf-8")
DIRECT_HELPER_SOURCES = {
    relative_path: (REPO_ROOT / relative_path).read_text(encoding="utf-8")
    for relative_path in (
        ".ouroboros/auto_remediate_retry_abort.py",
        ".ouroboros/classify_retry_loop_outcome.py",
        ".ouroboros/git_repo_health.py",
        ".ouroboros/monitor_codex_quota.py",
        ".ouroboros/post_attempt_guard.py",
        ".ouroboros/prepare_retry_attempt_state.py",
        ".ouroboros/refresh_analysis_state.py",
        ".ouroboros/request_soft_stop.py",
        ".ouroboros/resume_from_pause.py",
        ".ouroboros/run_next_probe.py",
        ".ouroboros/snapshot_retry_inputs.py",
        ".ouroboros/snapshot_retry_runtime.py",
        ".ouroboros/repair_git_and_push.py",
    )
}
RESUME_FROM_PAUSE_SOURCE = (REPO_ROOT / ".ouroboros" / "resume_from_pause.py").read_text(
    encoding="utf-8"
)
LAUNCH_RETRY_LOOP_SOURCE = (REPO_ROOT / ".ouroboros" / "launch_retry_loop.sh").read_text(
    encoding="utf-8"
)
RESTART_RETRY_LOOP_SOURCE = (
    REPO_ROOT / ".ouroboros" / "restart_retry_loop_after_attempt.sh"
).read_text(encoding="utf-8")
GIT_REPO_HEALTH_SOURCE = (REPO_ROOT / ".ouroboros" / "git_repo_health.py").read_text(
    encoding="utf-8"
)
POST_ATTEMPT_GUARD_SOURCE = (REPO_ROOT / ".ouroboros" / "post_attempt_guard.py").read_text(
    encoding="utf-8"
)
RUN_NEXT_PROBE_SOURCE = (REPO_ROOT / ".ouroboros" / "run_next_probe.py").read_text(
    encoding="utf-8"
)
CLASSIFY_RETRY_LOOP_OUTCOME_SOURCE = (
    REPO_ROOT / ".ouroboros" / "classify_retry_loop_outcome.py"
).read_text(encoding="utf-8")
PYTEST_INI_SOURCE = (REPO_ROOT / "pytest.ini").read_text(encoding="utf-8")
PROGRESS40_SEED_SOURCE = (
    REPO_ROOT / ".ouroboros" / "seed_branch3_progress40_research_loop.yaml"
).read_text(encoding="utf-8")
FAILURE_ANALYSIS_SEED_SOURCE = (
    REPO_ROOT / ".ouroboros" / "seed_branch3_failure_analysis.yaml"
).read_text(encoding="utf-8")
FAILURE_ANALYSIS_LOOP_SEED_SOURCE = (
    REPO_ROOT / ".ouroboros" / "seed_branch3_failure_analysis_loop.yaml"
).read_text(encoding="utf-8")
ACCEPTANCE_SEED_SOURCE = (REPO_ROOT / ".ouroboros" / "seed_af0ff01c05e9.yaml").read_text(
    encoding="utf-8"
)
HARDCODED_BRANCH_ROOT = (
    "/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3"
)


def load_auto_remediation_module():
    module_path = REPO_ROOT / ".ouroboros" / "auto_remediate_retry_abort.py"
    spec = importlib.util.spec_from_file_location("auto_remediate_retry_abort", module_path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


class ArtifactPathCanonicalizationTests(unittest.TestCase):
    def assert_under_artifacts(self, value: str | Path) -> None:
        path = Path(value).resolve()
        try:
            path.relative_to(artifacts_root())
        except ValueError:
            self.fail(f"path escaped artifacts root: {path}")

    def test_resolve_output_path_collapses_prefixed_artifact_roots(self) -> None:
        expected = artifacts_root() / "lca_tree_stress_v5" / "hunt" / "retry_loop" / "probe"
        prefixed_inputs = (
            "artifacts/lca_tree_stress_v5/hunt/retry_loop/probe",
            f"{artifact_paths.BRANCH_ROOT.name}/artifacts/lca_tree_stress_v5/hunt/retry_loop/probe",
            "artifacts/artifacts/lca_tree_stress_v5/hunt/retry_loop/probe",
            f"{artifact_paths.BRANCH_ROOT.name}/{artifact_paths.BRANCH_ROOT.name}/artifacts/lca_tree_stress_v5/hunt/retry_loop/probe",
        )

        for raw in prefixed_inputs:
            with self.subTest(raw=raw):
                resolved = artifact_paths.resolve_output_path(raw, default_key="lca_hunt")
                self.assertEqual(resolved, expected)
                self.assert_under_artifacts(resolved)


class NonArtifactTreeVerificationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.branch_root = Path(self.tempdir.name) / "branch"
        self.artifacts_root = self.branch_root / "artifacts"
        self.branch_root.mkdir(parents=True, exist_ok=True)
        self.artifacts_root.mkdir(parents=True, exist_ok=True)
        (self.branch_root / ".git").mkdir(parents=True, exist_ok=True)
        (self.branch_root / ".git" / "HEAD").write_text("ref: refs/heads/main\n", encoding="utf-8")
        (self.branch_root / "notes.txt").write_text("baseline\n", encoding="utf-8")

    def tearDown(self) -> None:
        self.tempdir.cleanup()

    def test_compare_detects_created_and_modified_non_artifact_files(self) -> None:
        baseline = artifact_paths.collect_non_artifact_tree_state(
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )

        (self.branch_root / "escape.txt").write_text("escaped\n", encoding="utf-8")
        (self.branch_root / "notes.txt").write_text("modified\n", encoding="utf-8")
        (self.artifacts_root / "allowed.txt").write_text("artifact output\n", encoding="utf-8")

        current = artifact_paths.collect_non_artifact_tree_state(
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )
        diff = artifact_paths.compare_non_artifact_tree_states(baseline, current)

        self.assertEqual(diff["created"], ["escape.txt"])
        self.assertEqual(diff["modified"], ["notes.txt"])
        self.assertEqual(diff["removed"], [])
        self.assertNotIn(".git/HEAD", current["entries"])
        self.assertNotIn("artifacts/allowed.txt", current["entries"])

    def test_verify_writes_clean_and_escape_reports(self) -> None:
        baseline_path = self.artifacts_root / "baseline.json"
        current_path = self.artifacts_root / "current.json"
        report_path = self.artifacts_root / "report.txt"

        artifact_paths.write_non_artifact_tree_state(
            baseline_path,
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )
        (self.artifacts_root / "allowed.txt").write_text("artifact output\n", encoding="utf-8")
        self.assertTrue(
            artifact_paths.verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        )
        self.assertIn("status=clean", report_path.read_text(encoding="utf-8"))

        (self.branch_root / "escape.txt").write_text("escaped\n", encoding="utf-8")
        self.assertFalse(
            artifact_paths.verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        )
        report_text = report_path.read_text(encoding="utf-8")
        self.assertIn("status=escape_detected", report_text)
        self.assertIn("[created]", report_text)
        self.assertIn("escape.txt", report_text)

    def test_verify_detects_created_empty_non_artifact_directories(self) -> None:
        baseline_path = self.artifacts_root / "baseline.json"
        current_path = self.artifacts_root / "current.json"
        report_path = self.artifacts_root / "report.txt"

        artifact_paths.write_non_artifact_tree_state(
            baseline_path,
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )

        (self.branch_root / "escape_dir").mkdir()

        self.assertFalse(
            artifact_paths.verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        )
        report_text = report_path.read_text(encoding="utf-8")
        self.assertIn("status=escape_detected", report_text)
        self.assertIn("[created]", report_text)
        self.assertIn("escape_dir", report_text)

    def test_verify_treats_modified_existing_non_artifact_files_as_warning_only(self) -> None:
        baseline_path = self.artifacts_root / "baseline.json"
        current_path = self.artifacts_root / "current.json"
        report_path = self.artifacts_root / "report.txt"

        artifact_paths.write_non_artifact_tree_state(
            baseline_path,
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )

        (self.branch_root / "notes.txt").write_text("modified\n", encoding="utf-8")

        self.assertTrue(
            artifact_paths.verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        )
        report_text = report_path.read_text(encoding="utf-8")
        self.assertIn("status=modified_only_warning", report_text)
        self.assertIn("[modified_warning]", report_text)
        self.assertIn("notes.txt", report_text)

    def test_verify_treats_created_source_like_files_as_warning_only(self) -> None:
        baseline_path = self.artifacts_root / "baseline.json"
        current_path = self.artifacts_root / "current.json"
        report_path = self.artifacts_root / "report.txt"
        ouroboros_dir = self.branch_root / ".ouroboros"
        ouroboros_dir.mkdir()

        artifact_paths.write_non_artifact_tree_state(
            baseline_path,
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )

        (self.branch_root / "helper.py").write_text("print('ok')\n", encoding="utf-8")
        (self.branch_root / "script.sh").write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        (ouroboros_dir / "new_helper.py").write_text("VALUE = 1\n", encoding="utf-8")

        self.assertTrue(
            artifact_paths.verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        )
        report_text = report_path.read_text(encoding="utf-8")
        self.assertIn("status=modified_only_warning", report_text)
        self.assertIn("[created_warning]", report_text)
        self.assertIn("helper.py", report_text)
        self.assertIn("script.sh", report_text)
        self.assertIn(".ouroboros/new_helper.py", report_text)

    def test_verify_treats_created_ini_files_as_warning_only(self) -> None:
        baseline_path = self.artifacts_root / "baseline.json"
        current_path = self.artifacts_root / "current.json"
        report_path = self.artifacts_root / "report.txt"

        artifact_paths.write_non_artifact_tree_state(
            baseline_path,
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )

        (self.branch_root / "pytest.ini").write_text("[pytest]\naddopts = -q\n", encoding="utf-8")

        self.assertTrue(
            artifact_paths.verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        )
        report_text = report_path.read_text(encoding="utf-8")
        self.assertIn("status=modified_only_warning", report_text)
        self.assertIn("[created_warning]", report_text)
        self.assertIn("pytest.ini", report_text)

    def test_verify_treats_created_analysis_refresh_logs_as_warning_only(self) -> None:
        baseline_path = self.artifacts_root / "baseline.json"
        current_path = self.artifacts_root / "current.json"
        report_path = self.artifacts_root / "report.txt"
        ouroboros_dir = self.branch_root / ".ouroboros"
        ouroboros_dir.mkdir()

        artifact_paths.write_non_artifact_tree_state(
            baseline_path,
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )

        (
            ouroboros_dir / "analysis_refresh_attempt_023_subac2.log"
        ).write_text("manual analysis refresh note\n", encoding="utf-8")

        self.assertTrue(
            artifact_paths.verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        )
        report_text = report_path.read_text(encoding="utf-8")
        self.assertIn("status=modified_only_warning", report_text)
        self.assertIn("[created_warning]", report_text)
        self.assertIn(".ouroboros/analysis_refresh_attempt_023_subac2.log", report_text)

    def test_verify_treats_removed_analysis_refresh_logs_as_warning_only(self) -> None:
        baseline_path = self.artifacts_root / "baseline.json"
        current_path = self.artifacts_root / "current.json"
        report_path = self.artifacts_root / "report.txt"
        ouroboros_dir = self.branch_root / ".ouroboros"
        ouroboros_dir.mkdir()
        refresh_log = ouroboros_dir / "analysis_refresh_attempt_023_subac2.log"
        refresh_log.write_text("manual analysis refresh note\n", encoding="utf-8")

        artifact_paths.write_non_artifact_tree_state(
            baseline_path,
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )

        refresh_log.unlink()

        self.assertTrue(
            artifact_paths.verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        )
        report_text = report_path.read_text(encoding="utf-8")
        self.assertIn("status=modified_only_warning", report_text)
        self.assertIn("[removed_warning]", report_text)
        self.assertIn(".ouroboros/analysis_refresh_attempt_023_subac2.log", report_text)

    def test_verify_treats_removed_transient_cache_dirs_as_warning_only(self) -> None:
        baseline_path = self.artifacts_root / "baseline.json"
        current_path = self.artifacts_root / "current.json"
        report_path = self.artifacts_root / "report.txt"
        cache_dir = self.branch_root / ".pytest_cache" / "v" / "cache"
        cache_dir.mkdir(parents=True)
        (cache_dir / "nodeids").write_text("[]\n", encoding="utf-8")

        artifact_paths.write_non_artifact_tree_state(
            baseline_path,
            branch_root=self.branch_root,
            artifacts_root=self.artifacts_root,
        )

        shutil.rmtree(self.branch_root / ".pytest_cache")

        self.assertTrue(
            artifact_paths.verify_non_artifact_tree_state(baseline_path, current_path, report_path)
        )
        report_text = report_path.read_text(encoding="utf-8")
        self.assertIn("status=modified_only_warning", report_text)
        self.assertIn("[removed_warning]", report_text)
        self.assertIn(".pytest_cache", report_text)

    def test_snapshot_recreates_parent_after_collection_if_temp_root_disappears(self) -> None:
        baseline_path = self.artifacts_root / "volatile" / "baseline.json"
        original_collect = artifact_paths.collect_non_artifact_tree_state

        def collect_and_drop_parent(*, branch_root: Path, artifacts_root: Path) -> dict[str, object]:
            payload = original_collect(branch_root=branch_root, artifacts_root=artifacts_root)
            baseline_path.parent.rmdir()
            return payload

        baseline_path.parent.mkdir(parents=True, exist_ok=True)
        with mock.patch.object(
            artifact_paths,
            "collect_non_artifact_tree_state",
            side_effect=collect_and_drop_parent,
        ):
            written = artifact_paths.write_non_artifact_tree_state(
                baseline_path,
                branch_root=self.branch_root,
                artifacts_root=self.artifacts_root,
            )

        self.assertEqual(written, baseline_path.resolve())
        self.assertTrue(baseline_path.exists(), msg="snapshot writes should recreate the parent directory if it disappears mid-call")


class RetryLoopArtifactLocalityRegressionTests(unittest.TestCase):
    def test_pytest_disables_repo_local_cacheprovider(self) -> None:
        self.assertIn(
            "addopts = -p no:cacheprovider",
            PYTEST_INI_SOURCE,
            msg="pytest must disable the repo-root cache provider so validation does not recreate .pytest_cache outside artifacts",
        )

    def test_direct_retry_helpers_disable_repo_local_bytecode_outputs(self) -> None:
        for relative_path, source in DIRECT_HELPER_SOURCES.items():
            with self.subTest(path=relative_path):
                self.assertIn(
                    'os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")',
                    source,
                    msg=f"{relative_path} must suppress direct-entrypoint bytecode writes outside artifacts",
                )
                self.assertIn(
                    "sys.dont_write_bytecode = True",
                    source,
                    msg=f"{relative_path} must suppress direct-entrypoint __pycache__ writes outside artifacts",
                )

    def test_prepare_retry_attempt_state_enables_bytecode_suppression_before_local_helper_imports(self) -> None:
        py_dont_write_idx = PREPARE_RETRY_ATTEMPT_STATE_SOURCE.index(
            'os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")'
        )
        dont_write_idx = PREPARE_RETRY_ATTEMPT_STATE_SOURCE.index("sys.dont_write_bytecode = True")
        local_import_idx = PREPARE_RETRY_ATTEMPT_STATE_SOURCE.index("from verify_analysis_refresh import (")

        self.assertLess(
            py_dont_write_idx,
            local_import_idx,
            msg="prepare_retry_attempt_state must suppress bytecode writes before importing sibling retry helpers",
        )
        self.assertLess(
            dont_write_idx,
            local_import_idx,
            msg="prepare_retry_attempt_state must set sys.dont_write_bytecode before importing sibling retry helpers",
        )

    def test_repo_has_no_legacy_non_artifact_retry_runtime_outputs(self) -> None:
        for relative_path in (
            ".ouroboros/quota_pause_state.json",
            ".ouroboros/latest_workflow.log",
            ".ouroboros/latest_analysis_refresh.log",
        ):
            with self.subTest(path=relative_path):
                self.assertFalse(
                    (REPO_ROOT / relative_path).exists(),
                    msg=f"legacy retry-loop runtime output must not live at repo root: {relative_path}",
                )

    def test_retry_loop_soft_stop_state_defaults_live_under_retry_artifacts(self) -> None:
        self.assertIn(
            'soft_stop_file="$report_root/soft_stop_request.json"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop orchestration must keep soft-stop requests under the retry artifact root",
        )
        self.assertIn(
            'default="artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json"',
            REQUEST_SOFT_STOP_SOURCE,
            msg="manual soft-stop requests must default to the retry artifact root",
        )
        self.assertIn(
            'default="artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json"',
            MONITOR_QUOTA_SOURCE,
            msg="quota watchdog soft-stop requests must default to the retry artifact root",
        )

    def test_retry_loop_pause_snapshots_default_under_retry_artifacts(self) -> None:
        self.assertIn(
            'default="artifacts/lca_tree_stress_v5/retry_loop/quota_pause_state.json"',
            SNAPSHOT_RUNTIME_SOURCE,
            msg="runtime snapshot pause-state output must default to the retry artifact root",
        )
        self.assertIn(
            'default="artifacts/lca_tree_stress_v5/retry_loop/quota_pause_state.json"',
            RESUME_FROM_PAUSE_SOURCE,
            msg="pause resume helper must read the retry artifact-rooted pause snapshot by default",
        )

    def test_git_repair_helper_stages_restore_downloads_under_branch_artifacts(self) -> None:
        self.assertIn(
            'ARTIFACT_TMP_ROOT = (BRANCH_ROOT / "artifacts" / "lca_tree_stress_v5" / ".tmp" / "git_repair").resolve()',
            REPAIR_GIT_AND_PUSH_SOURCE,
            msg="git repair helper must stage restore downloads under branch-local artifacts",
        )
        self.assertIn(
            "return ARTIFACT_TMP_ROOT / f\"{Path(rel).name}.{digest}.restore\"",
            REPAIR_GIT_AND_PUSH_SOURCE,
            msg="git repair helper must derive restore download paths from the branch-local artifact tmp root",
        )
        self.assertNotIn(
            'Path("/tmp")',
            REPAIR_GIT_AND_PUSH_SOURCE,
            msg="git repair helper must not spill restore downloads into /tmp",
        )
        self.assertIn(
            "ROOT = BRANCH_ROOT.parent",
            REPAIR_GIT_AND_PUSH_SOURCE,
            msg="git repair helper must derive the checkout root from the active branch rather than a pinned absolute path",
        )
        self.assertNotIn(
            HARDCODED_BRANCH_ROOT,
            REPAIR_GIT_AND_PUSH_SOURCE,
            msg="git repair helper must not hardcode a user-specific checkout root",
        )

    def test_manual_launcher_normalizes_logs_under_retry_artifacts(self) -> None:
        self.assertIn(
            'retry_log_root="$branch_root/artifacts/lca_tree_stress_v5/retry_loop"',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual retry-loop launches must default their logs under the retry artifact root",
        )
        self.assertIn(
            'python3 "$artifact_resolver" --ensure "$candidate_log_file"',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual retry-loop launches must reject log destinations outside branch-local artifacts",
        )
        self.assertIn(
            ': > "$log_file"',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual retry-loop relaunches must truncate an existing branch-local log instead of appending stale content",
        )
        self.assertIn(
            '--require-json-key "${analysis_state_file}:current_for_latest_failure"',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual retry-loop launch verification must brace zsh variables before appending require-json-key suffixes",
        )
        self.assertIn(
            'branch_root="$(cd -- "$script_dir/.." && pwd -P)"',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual retry-loop launcher must derive the active branch root from the script location",
        )
        self.assertNotIn(
            HARDCODED_BRANCH_ROOT,
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual retry-loop launcher must not pin outputs to a user-specific checkout path",
        )
        self.assertIn(
            'auto_remediation_helper="$branch_root/.ouroboros/auto_remediate_retry_abort.py"',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual retry-loop launcher must expose a structured auto-remediation helper for restartable aborts",
        )
        self.assertIn(
            'python3 "$auto_remediation_helper" \\',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual retry-loop launcher must invoke the structured auto-remediation helper before giving up on a stopped loop",
        )
        self.assertIn(
            "auto-remediation handled loop exit code",
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual retry-loop launcher must log when a stopped loop was automatically repaired and relaunched",
        )

    def test_retry_restart_watcher_replaces_prior_attempt_log_state(self) -> None:
        self.assertIn(
            'artifact_resolver="$branch_root/artifact_paths.py"',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must use the branch-local artifact resolver before opening its log file",
        )
        self.assertIn(
            'watch_log="$(ensure_artifact_path "$watch_log")"',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must canonicalize its per-attempt log under branch-local artifacts",
        )
        self.assertIn(
            ': > "$watch_log"',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher relaunches must reset their per-attempt log file before writing fresh state",
        )
        self.assertIn(
            'attempt_log="$(ensure_artifact_path "$attempt_log")"',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must reject attempt-log handoffs that escape branch-local artifacts",
        )
        self.assertIn(
            'branch_root="$(cd -- "$script_dir/.." && pwd -P)"',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must derive the active branch root from the script location",
        )
        self.assertNotIn(
            HARDCODED_BRANCH_ROOT,
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must not pin logs or relaunches to a user-specific checkout path",
        )

    def test_retry_restart_watcher_only_restarts_retryable_failures(self) -> None:
        self.assertIn(
            "should_restart=0",
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must track whether a retryable intermediate failure was actually detected",
        )
        self.assertIn(
            'if [[ "$should_restart" != "1" ]]; then',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must refuse blind restarts when no retryable failure signal exists",
        )
        self.assertIn(
            "no retryable intermediate failure detected",
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must leave the old loop in place when the attempt was terminal or unclassified",
        )

    def test_retry_restart_watcher_preserves_seed_and_attempt_handoff(self) -> None:
        self.assertIn(
            'analysis_seed_file="${6:-.ouroboros/seed_branch3_failure_analysis.yaml}"',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must preserve the analysis seed when it relaunches the retry loop",
        )
        self.assertIn(
            'next_attempt_number=$(( attempt_number + 1 ))',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must carry the next attempt number forward instead of resetting to attempt 1",
        )
        self.assertIn(
            'RETRY_LOOP_START_ATTEMPT="$next_attempt_number"',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must relaunch the retry loop with a monotonic attempt counter",
        )
        self.assertIn(
            'zsh "$loop_script" "$seed_file" "$analysis_seed_file"',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart watcher must pass both workflow seeds back into the retry loop",
        )
        self.assertIn(
            'start_attempt_raw="${3:-${RETRY_LOOP_START_ATTEMPT:-}}"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop entry must accept the carried-forward attempt number from its relaunch environment",
        )
        self.assertIn(
            'attempt="$start_attempt_raw"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop entry must honor the preserved attempt number instead of always resetting to 1",
        )
        self.assertIn(
            'attempt="$(next_attempt_number "$report_root")"',
            RUN_UNTIL_PASS_SOURCE,
            msg="fresh retry-loop launches must still derive the next monotonic attempt number from the retry artifact root",
        )

    def test_retry_loop_recaptures_failure_context_after_analysis_refreshes_tooling(self) -> None:
        self.assertIn(
            'python3 .ouroboros/capture_failure_context.py',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop analysis verification must rerun failure capture so refreshed analysis tooling updates the current attempt artifacts",
        )
        self.assertIn(
            '--workflow-log "$attempt_log"',
            RUN_UNTIL_PASS_SOURCE,
            msg="analysis-side failure recapture must target the same attempt workflow log",
        )
        self.assertIn(
            '--exit-code "$exit_code" >> "$analysis_log" 2>&1 \\',
            RUN_UNTIL_PASS_SOURCE,
            msg="analysis-side failure recapture must preserve the original solver exit code and record its refresh in the analysis log",
        )
        self.assertIn(
            '&& python3 .ouroboros/refresh_analysis_state.py \\',
            RUN_UNTIL_PASS_SOURCE,
            msg="failure recapture must happen before refresh_analysis_state consumes the refreshed breakdown",
        )

    def test_retry_loop_entry_shells_bind_runtime_environment_under_artifacts(self) -> None:
        self.assertIn(
            'branch_root="$(cd -- "$script_dir/.." && pwd -P)"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop entry must derive its branch root dynamically before computing artifact outputs",
        )
        self.assertNotIn(
            HARDCODED_BRANCH_ROOT,
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop entry must not pin its artifact outputs to a user-specific checkout path",
        )
        self.assertIn(
            'retry_tmp_parent="$branch_root/artifacts/lca_tree_stress_v5/.tmp"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop runtime env must allocate temp state under branch-local artifacts",
        )
        self.assertIn(
            'retry_runtime_env_root="$(mktemp -d "$retry_tmp_parent/retry_loop.runtime.env.XXXXXX")"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop runtime env must allocate a dedicated artifact-rooted temp namespace",
        )
        self.assertIn(
            'export BRANCH_ARTIFACT_TMP_ROOT="$retry_tmpdir"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop runtime env must route BRANCH_ARTIFACT_TMP_ROOT under artifacts",
        )
        self.assertIn(
            'export TMPDIR="$retry_tmpdir"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop runtime env must route TMPDIR under artifacts before workflow launch",
        )
        self.assertIn(
            'export PYTHONPYCACHEPREFIX="$retry_pycache_root"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop runtime env must isolate Python bytecode under artifacts",
        )
        self.assertIn(
            "Codex auth/session roots while routing",
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop runtime env must document why HOME is preserved while scratch output moves under artifacts",
        )
        self.assertIn(
            'launcher_runtime_root="$(mktemp -d "$retry_tmp_parent/retry_loop.launcher.env.XXXXXX")"',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual launcher must allocate its own artifact-rooted temp namespace",
        )
        self.assertIn(
            'export BRANCH_ARTIFACT_TMP_ROOT="$launcher_tmpdir"',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual launcher must route BRANCH_ARTIFACT_TMP_ROOT under artifacts before dispatch",
        )
        self.assertIn(
            'export TMPDIR="$launcher_tmpdir"',
            LAUNCH_RETRY_LOOP_SOURCE,
            msg="manual launcher must route TMPDIR under artifacts before starting the retry loop",
        )

    def test_retry_loop_verifies_each_workflow_run_for_non_artifact_output_escapes(self) -> None:
        self.assertIn(
            'locality_root="$(ensure_artifact_path "$attempt_dir/.workflow_output_locality/$locality_label")"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop workflow runs must stage output-locality snapshots under the attempt artifact root",
        )
        self.assertIn(
            'python3 "$artifact_resolver" --snapshot-non-artifact-tree "$non_artifact_baseline"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop workflow runs must capture a non-artifact baseline before dispatch",
        )
        self.assertIn(
            '--verify-non-artifact-tree \\',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop workflow runs must verify the post-run tree against the saved baseline",
        )
        self.assertIn(
            "generated non-artifact output outside branch-local artifacts; aborting retry loop",
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop orchestration must fail closed when solver or analysis runs escape the artifact root",
        )
        self.assertIn(
            'latest_${label}_non_artifact_tree_report.txt',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop output-locality failures must publish a stable latest report under retry artifacts",
        )

    def test_retry_loop_rebuilds_runtime_workspace_after_pre_attempt_cleanup(self) -> None:
        loop_idx = RUN_UNTIL_PASS_SOURCE.index("while true; do")
        cleanup_call_idx = RUN_UNTIL_PASS_SOURCE.index("cleanup_retry_runtime_environment", loop_idx)
        prepare_idx = RUN_UNTIL_PASS_SOURCE.index('python3 "$prepare_state_helper"', loop_idx)
        configure_call_idx = RUN_UNTIL_PASS_SOURCE.index("configure_retry_runtime_environment", prepare_idx)
        start_idx = RUN_UNTIL_PASS_SOURCE.index(
            "printf '[%s] attempt %d start: %s\\n'",
            configure_call_idx,
        )
        self.assertLess(
            cleanup_call_idx,
            prepare_idx,
            msg="retry-loop must drop the previous attempt runtime workspace before pre-attempt cleanup inspects .tmp",
        )
        self.assertLess(
            prepare_idx,
            configure_call_idx,
            msg="retry-loop must recreate its runtime workspace only after pre-attempt cleanup has cleared stale .tmp state",
        )
        self.assertLess(
            configure_call_idx,
            start_idx,
            msg="retry-loop must not start a new attempt until its fresh per-attempt runtime workspace is ready",
        )

    def test_prepare_retry_attempt_state_waits_for_live_gate_locks_before_blocking(self) -> None:
        self.assertIn(
            "LIVE_GATE_LOCK_WAIT_SECONDS = 20.0",
            PREPARE_RETRY_ATTEMPT_STATE_SOURCE,
            msg="pre-attempt cleanup must give live gate locks a grace window before declaring the loop blocked",
        )
        self.assertIn(
            "time.sleep(LIVE_GATE_LOCK_POLL_SECONDS)",
            PREPARE_RETRY_ATTEMPT_STATE_SOURCE,
            msg="pre-attempt cleanup must poll for short-lived gate locks before failing the attempt",
        )
        self.assertIn(
            "_wait_for_active_locks(lock_root)",
            PREPARE_RETRY_ATTEMPT_STATE_SOURCE,
            msg="pre-attempt cleanup must retry active lock collection instead of immediately aborting the loop",
        )


class AutoRemediationGateLockTests(unittest.TestCase):
    def test_parse_live_gate_lock_blockers_handles_spaces_in_paths(self) -> None:
        module = load_auto_remediation_module()
        sample = (
            "pre-attempt cleanup blocked: live branch-local gate lock detected\n"
            "  /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/"
            "branch_3/artifacts/lca_tree_stress_v5/.locks/lca_smoke pid=21408 "
            "21408 21407 00:18 /bin/bash /Users/free_1/Library/Mobile Documents/"
            "iCloud~md~obsidian/Documents/cpp-checker/branch_3/outer_suite_wrappers/lca_smoke.sh\n"
        )

        blockers = module._parse_live_gate_lock_blockers(sample)

        self.assertEqual(len(blockers), 1)
        self.assertTrue(blockers[0]["lock_dir"].endswith(".locks/lca_smoke"))
        self.assertEqual(blockers[0]["pid"], 21408)
        self.assertIn("outer_suite_wrappers/lca_smoke.sh", blockers[0]["ps"])

    def test_recognizes_known_branch_local_gate_holder_commands(self) -> None:
        module = load_auto_remediation_module()
        ps_text = (
            f"21408 21407 00:18 /bin/bash {REPO_ROOT}/outer_suite_wrappers/lca_smoke.sh"
        )
        self.assertTrue(module._is_known_gate_lock_holder(REPO_ROOT, ps_text))
        self.assertFalse(module._is_known_gate_lock_holder(REPO_ROOT, "21408 21407 00:18 sleep 60"))

    def test_live_gate_lock_remediation_waits_out_short_lived_wrapper_and_rechecks_preflight(self) -> None:
        module = load_auto_remediation_module()
        with tempfile.TemporaryDirectory() as tempdir:
            branch_root = Path(tempdir) / "branch_3"
            report_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "retry_loop"
            lock_dir = branch_root / "artifacts" / "lca_tree_stress_v5" / ".locks" / "lca_smoke"
            wrapper = branch_root / "outer_suite_wrappers" / "lca_smoke.sh"
            report_root.mkdir(parents=True, exist_ok=True)
            lock_dir.mkdir(parents=True, exist_ok=True)
            wrapper.parent.mkdir(parents=True, exist_ok=True)
            wrapper.write_text("#!/bin/bash\nsleep 0.2\n", encoding="utf-8")
            wrapper.chmod(0o755)

            proc = subprocess.Popen(["/bin/bash", str(wrapper)])
            try:
                (lock_dir / "pid").write_text(f"{proc.pid}\n", encoding="utf-8")
                sample_stdout = (
                    "pre-attempt cleanup blocked: live branch-local gate lock detected\n"
                    f"  {lock_dir} pid={proc.pid} {proc.pid} 1 00:00 /bin/bash {wrapper}\n"
                )

                def fake_verify_preflight(_branch_root: Path, _report_root: Path):
                    if lock_dir.exists():
                        return subprocess.CompletedProcess(
                            ["python3", ".ouroboros/prepare_retry_attempt_state.py"],
                            1,
                            stdout=sample_stdout,
                            stderr="",
                        )
                    return subprocess.CompletedProcess(
                        ["python3", ".ouroboros/prepare_retry_attempt_state.py"],
                        0,
                        stdout="pre-attempt cleanup ok\n",
                        stderr="",
                    )

                with mock.patch.object(module, "_verify_preflight", side_effect=fake_verify_preflight), mock.patch.object(
                    module, "LIVE_GATE_LOCK_WAIT_SECONDS", 1.0
                ), mock.patch.object(module, "LIVE_GATE_LOCK_POLL_SECONDS", 0.05), mock.patch.object(
                    module, "LIVE_GATE_LOCK_TERM_GRACE_SECONDS", 0.2
                ), mock.patch.object(
                    module,
                    "_pid_alive",
                    side_effect=lambda pid: pid == proc.pid and proc.poll() is None,
                ):
                    handled, details = module._remediate_live_gate_lock(
                        branch_root,
                        report_root,
                        artifact_paths,
                        sample_stdout,
                    )
            finally:
                proc.wait(timeout=5)

        self.assertTrue(handled)
        self.assertIn("live gate lock auto-cleared", details["reason"])
        self.assertIn(str(lock_dir.resolve()), details["removed_paths"])

    def test_retry_loop_resets_attempt_dir_before_new_attempt_logging(self) -> None:
        self.assertIn(
            "reset_attempt_dir()",
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop must define a dedicated attempt-dir reset step before each attempt",
        )
        loop_idx = RUN_UNTIL_PASS_SOURCE.index("while true; do")
        reset_idx = RUN_UNTIL_PASS_SOURCE.index("reset_attempt_dir", loop_idx)
        prepare_idx = RUN_UNTIL_PASS_SOURCE.index('python3 "$prepare_state_helper"', loop_idx)
        self.assertLess(
            reset_idx,
            prepare_idx,
            msg="retry-loop must clear any stale attempt workspace before pre-attempt cleanup writes the new attempt log bundle",
        )

    def test_retry_loop_output_helpers_guard_artifact_boundaries(self) -> None:
        helper_sources = {
            "capture_failure_context.py": CAPTURE_FAILURE_CONTEXT_SOURCE,
            "request_soft_stop.py": REQUEST_SOFT_STOP_SOURCE,
            "monitor_codex_quota.py": MONITOR_QUOTA_SOURCE,
            "refresh_analysis_state.py": REFRESH_ANALYSIS_STATE_SOURCE,
            "snapshot_retry_runtime.py": SNAPSHOT_RUNTIME_SOURCE,
            "snapshot_retry_inputs.py": SNAPSHOT_INPUTS_SOURCE,
            "git_repo_health.py": GIT_REPO_HEALTH_SOURCE,
            "post_attempt_guard.py": POST_ATTEMPT_GUARD_SOURCE,
            "run_next_probe.py": RUN_NEXT_PROBE_SOURCE,
        }
        for name, source in helper_sources.items():
            with self.subTest(helper=name):
                self.assertIn(
                    "ensure_under_artifacts",
                    source,
                    msg=f"{name} must enforce branch-local artifact routing before writing outputs",
                )
        self.assertIn(
            '--branch-root "$PWD" \\',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop analysis refresh must pass the active branch root before resolving attempt/report outputs",
        )

    def test_capture_failure_context_defines_attempt_label_helper_before_state_reuse_check(self) -> None:
        helper_idx = CAPTURE_FAILURE_CONTEXT_SOURCE.index("def format_attempt_label(")
        state_match_idx = CAPTURE_FAILURE_CONTEXT_SOURCE.index("def analysis_state_attempt_matches(")
        self.assertLess(
            helper_idx,
            state_match_idx,
            msg="capture_failure_context must define format_attempt_label before analysis_state_attempt_matches reuses analysis-state attempt labels",
        )

    def test_retry_seed_configs_avoid_hardcoded_user_specific_branch_paths(self) -> None:
        seed_sources = {
            "seed_af0ff01c05e9.yaml": ACCEPTANCE_SEED_SOURCE,
            "seed_branch3_progress40_research_loop.yaml": PROGRESS40_SEED_SOURCE,
            "seed_branch3_failure_analysis.yaml": FAILURE_ANALYSIS_SEED_SOURCE,
            "seed_branch3_failure_analysis_loop.yaml": FAILURE_ANALYSIS_LOOP_SEED_SOURCE,
        }
        for name, source in seed_sources.items():
            with self.subTest(path=name):
                self.assertNotIn(
                    HARDCODED_BRANCH_ROOT,
                    source,
                    msg=f"{name} must keep branch-local paths relocatable instead of pinning a user-specific checkout root",
                )
                self.assertTrue(
                    "  - path: ." in source or "  - path: ./" in source,
                    msg=f"{name} should use branch-relative context references",
                )

    def test_retry_loop_marks_retryable_intermediate_failures_as_refinement_cycles(self) -> None:
        self.assertIn(
            'retry_outcome_helper=".ouroboros/classify_retry_loop_outcome.py"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop orchestration must classify failed attempts before deciding the next loop action",
        )
        self.assertIn(
            "recorded a retryable intermediate acceptance failure",
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop orchestration must log that a failed intermediate acceptance starts a refinement cycle",
        )
        self.assertIn(
            "nominal workflow success still contained retryable intermediate acceptance failure",
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop orchestration must refuse to stop on a nominal success when the workflow log still reports failed or blocked acceptances",
        )
        self.assertIn(
            'outcome_helper="$branch_root/.ouroboros/classify_retry_loop_outcome.py"',
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart helper must use the shared retry-outcome classifier instead of only a late failure marker",
        )
        self.assertIn(
            "detected retryable intermediate acceptance failure",
            RESTART_RETRY_LOOP_SOURCE,
            msg="restart helper must treat failed intermediate acceptances as restartable refinement cycles",
        )
        self.assertIn(
            "terminal_retry_abort",
            CLASSIFY_RETRY_LOOP_OUTCOME_SOURCE,
            msg="retry-outcome classification must distinguish terminal retry-loop aborts from retryable failures",
        )
        self.assertIn(
            "blocked_acceptance_summary",
            CLASSIFY_RETRY_LOOP_OUTCOME_SOURCE,
            msg="retry-outcome classification must keep blocked acceptance summaries inside the refinement loop instead of treating them as terminal success",
        )

    def test_retry_loop_snapshots_seed_inputs_under_retry_artifacts_before_each_attempt(self) -> None:
        self.assertIn(
            'retry_input_snapshot_helper=".ouroboros/snapshot_retry_inputs.py"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop orchestration must snapshot the exact solver and analysis seed inputs for each attempt",
        )
        self.assertIn(
            "retry-input snapshot failed; aborting retry loop before workflow start",
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop orchestration must fail closed when it cannot preserve reproducible attempt inputs",
        )
        self.assertIn(
            "latest_retry_inputs_snapshot.json",
            SNAPSHOT_INPUTS_SOURCE,
            msg="retry input snapshots must publish a latest artifact copy for same-tree reproduction",
        )
        self.assertIn(
            "latest_solver_seed.snapshot",
            SNAPSHOT_INPUTS_SOURCE,
            msg="retry input snapshots must preserve the active solver seed payload under artifacts",
        )

    def test_retry_loop_republishes_stable_latest_files_without_manual_cleanup(self) -> None:
        self.assertIn(
            'copy_artifact_file "$attempt_log" "$report_root/latest_workflow.log"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop must republish latest_workflow.log through a self-cleaning file-target helper",
        )
        self.assertIn(
            'prepare_artifact_file_target "$attempt_dir/latest_analysis_session.md"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop must prune stale analysis-session target collisions before rewriting the attempt-local summary",
        )
        self.assertIn(
            'copy_artifact_file "$attempt_dir/latest_analysis_session.md" "$report_root/latest_analysis_session.md"',
            RUN_UNTIL_PASS_SOURCE,
            msg="retry-loop must republish latest_analysis_session.md through the same self-cleaning file-target helper",
        )

    def test_retry_loop_helper_writers_use_self_cleaning_artifact_io(self) -> None:
        expected_tokens = {
            "snapshot_retry_inputs.py": (SNAPSHOT_INPUTS_SOURCE, "copy_output_file", "write_text_output"),
            "snapshot_retry_runtime.py": (SNAPSHOT_RUNTIME_SOURCE, "write_text_output", "prepare_output_dir"),
            "monitor_codex_quota.py": (MONITOR_QUOTA_SOURCE, "write_text_output", "prepare_output_dir"),
            "refresh_analysis_state.py": (REFRESH_ANALYSIS_STATE_SOURCE, "write_text_output", "prepare_output_dir"),
            "request_soft_stop.py": (REQUEST_SOFT_STOP_SOURCE, "write_text_output", "prepare_output_dir"),
            "git_repo_health.py": (GIT_REPO_HEALTH_SOURCE, "write_text_output", "prepare_output_dir"),
            "post_attempt_guard.py": (POST_ATTEMPT_GUARD_SOURCE, "write_text_output", "prepare_output_dir"),
            "run_next_probe.py": (RUN_NEXT_PROBE_SOURCE, "write_text_output", "reset_output_dir"),
            "capture_failure_context.py": (CAPTURE_FAILURE_CONTEXT_SOURCE, "copy_output_file", "write_text_output"),
        }

        for name, (source, *tokens) in expected_tokens.items():
            with self.subTest(helper=name):
                self.assertIn(
                    "from retry_artifact_io import",
                    source,
                    msg=f"{name} must import the shared self-cleaning artifact writer helpers",
                )
                for token in tokens:
                    self.assertIn(
                        token,
                        source,
                        msg=f"{name} must route stable artifact publication through `{token}`",
                    )


class RetryLoopArtifactGuardRuntimeTests(unittest.TestCase):
    def make_fake_branch(
        self,
        temp_root: Path,
        *,
        include_shared_resolver: bool = True,
    ) -> tuple[Path, Path]:
        branch_root = temp_root / "branch"
        artifacts_root = branch_root / "artifacts"
        branch_root.mkdir(parents=True, exist_ok=True)
        artifacts_root.mkdir(parents=True, exist_ok=True)
        resolver_source = (
            "def resolve_branch_artifact_path(path_like):\n"
            "    raw = Path(path_like).expanduser()\n"
            "    if raw.is_absolute():\n"
            "        candidate = raw.resolve()\n"
            "    else:\n"
            "        parts = [part for part in raw.parts if part not in ('', '.')]\n"
            "        artifact_idx = next((idx for idx, part in enumerate(parts) if part == 'artifacts'), None)\n"
            "        if artifact_idx is not None and artifact_idx > 0 and all(part == BRANCH_ROOT.name for part in parts[:artifact_idx]):\n"
            "            parts = parts[artifact_idx:]\n"
            "        while parts and parts[0] == 'artifacts':\n"
            "            parts.pop(0)\n"
            "        candidate = (ARTIFACTS_ROOT / Path(*parts)).resolve() if parts and parts[0] == 'lca_tree_stress_v5' else (BRANCH_ROOT / Path(*parts)).resolve()\n"
            "    return ensure_under_artifacts(candidate)\n"
            if include_shared_resolver
            else ""
        )
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
                f"{resolver_source}"
            ),
            encoding="utf-8",
        )
        return branch_root, artifacts_root

    def run_helper(self, helper_name: str, *args: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(REPO_ROOT / ".ouroboros" / helper_name), *args],
            check=False,
            capture_output=True,
            text=True,
        )

    def test_request_soft_stop_rejects_non_artifact_output_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, _ = self.make_fake_branch(temp_root)
            escape_path = temp_root / "escape.json"
            result = self.run_helper(
                "request_soft_stop.py",
                "--branch-root",
                str(branch_root),
                "--soft-stop-file",
                str(escape_path),
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(escape_path.exists())
            self.assertIn("output path must stay under", result.stderr)

    def test_request_soft_stop_republishes_over_directory_poisoned_target(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            soft_stop_path = artifacts_root / "lca_tree_stress_v5" / "retry_loop" / "soft_stop_request.json"
            soft_stop_path.mkdir(parents=True, exist_ok=True)

            result = self.run_helper(
                "request_soft_stop.py",
                "--branch-root",
                str(branch_root),
                "--soft-stop-file",
                str(soft_stop_path),
                "--note",
                "retry hygiene",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(soft_stop_path.is_file())
            payload = json.loads(soft_stop_path.read_text(encoding="utf-8"))
            self.assertEqual(payload["note"], "retry hygiene")

    def test_request_soft_stop_accepts_branch_prefixed_artifact_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            soft_stop_rel = (
                f"{branch_root.name}/artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json"
            )
            soft_stop_path = artifacts_root / "lca_tree_stress_v5" / "retry_loop" / "soft_stop_request.json"

            result = self.run_helper(
                "request_soft_stop.py",
                "--branch-root",
                str(branch_root),
                "--soft-stop-file",
                soft_stop_rel,
                "--note",
                "branch-prefixed",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(soft_stop_path.is_file())
            payload = json.loads(soft_stop_path.read_text(encoding="utf-8"))
            self.assertEqual(payload["note"], "branch-prefixed")

    def test_request_soft_stop_accepts_branch_prefixed_artifact_path_without_shared_resolver(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(
                temp_root,
                include_shared_resolver=False,
            )
            soft_stop_rel = (
                f"{branch_root.name}/artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json"
            )
            soft_stop_path = artifacts_root / "lca_tree_stress_v5" / "retry_loop" / "soft_stop_request.json"

            result = self.run_helper(
                "request_soft_stop.py",
                "--branch-root",
                str(branch_root),
                "--soft-stop-file",
                soft_stop_rel,
                "--note",
                "fallback-branch-prefixed",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(soft_stop_path.is_file())
            payload = json.loads(soft_stop_path.read_text(encoding="utf-8"))
            self.assertEqual(payload["note"], "fallback-branch-prefixed")

    def test_refresh_analysis_state_rejects_non_artifact_attempt_dir_when_branch_root_is_provided(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            report_root.mkdir(parents=True, exist_ok=True)
            escape_attempt_dir = temp_root / "escape_attempt"
            analysis_log = report_root / "analysis.log"
            analysis_log.write_text("analysis ok\n", encoding="utf-8")
            ouroboros_root = branch_root / ".ouroboros"
            ouroboros_root.mkdir(parents=True, exist_ok=True)
            state_path = ouroboros_root / "failure_analysis_state.json"
            iteration_path = ouroboros_root / "failure_analysis_iteration.md"

            result = self.run_helper(
                "refresh_analysis_state.py",
                "--branch-root",
                str(branch_root),
                "--attempt",
                "1",
                "--attempt-dir",
                str(escape_attempt_dir),
                "--report-root",
                str(report_root),
                "--analysis-log",
                str(analysis_log),
                "--analysis-round",
                "1",
                "--state-file",
                str(state_path),
                "--iteration-file",
                str(iteration_path),
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(escape_attempt_dir.exists())
            self.assertIn("output path must stay under", result.stderr)

    def test_request_soft_stop_collapses_redundant_artifact_prefix_without_shared_resolver(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(
                temp_root,
                include_shared_resolver=False,
            )
            soft_stop_rel = (
                f"{branch_root.name}/artifacts/artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json"
            )
            soft_stop_path = artifacts_root / "lca_tree_stress_v5" / "retry_loop" / "soft_stop_request.json"

            result = self.run_helper(
                "request_soft_stop.py",
                "--branch-root",
                str(branch_root),
                "--soft-stop-file",
                soft_stop_rel,
                "--note",
                "redundant-artifact-prefix",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(soft_stop_path.is_file())
            payload = json.loads(soft_stop_path.read_text(encoding="utf-8"))
            self.assertEqual(payload["note"], "redundant-artifact-prefix")

    def test_snapshot_retry_runtime_rejects_non_artifact_report_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            attempt_dir = artifacts_root / "lca_tree_stress_v5" / "retry_loop" / "attempt_001"
            attempt_log = attempt_dir / "workflow.log"
            attempt_dir.mkdir(parents=True, exist_ok=True)
            attempt_log.write_text("session_id=orch_test\nexecution_id=exec_test\n", encoding="utf-8")
            escape_root = temp_root / "escape_reports"

            result = self.run_helper(
                "snapshot_retry_runtime.py",
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(escape_root),
                "--attempt-log",
                str(attempt_log),
                "--seed-file",
                ".ouroboros/seed_branch3_progress40_research_loop.yaml",
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(escape_root.exists())
            self.assertIn("output path must stay under", result.stderr)

    def test_snapshot_retry_runtime_republishes_over_directory_poisoned_targets(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_002"
            attempt_log = attempt_dir / "workflow.log"
            attempt_log.parent.mkdir(parents=True, exist_ok=True)
            attempt_log.write_text("session_id=orch_test\nexecution_id=exec_test\n", encoding="utf-8")
            pause_state = report_root / "quota_pause_state.json"

            poisoned_paths = (
                attempt_dir / "runtime_snapshot.json",
                attempt_dir / "runtime_snapshot.md",
                report_root / "latest_runtime_snapshot.json",
                report_root / "latest_runtime_snapshot.md",
                pause_state,
                report_root / "latest_quota_pause.json",
                report_root / "latest_quota_pause.md",
            )
            for path in poisoned_paths:
                path.mkdir(parents=True, exist_ok=True)

            result = self.run_helper(
                "snapshot_retry_runtime.py",
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--attempt-log",
                str(attempt_log),
                "--seed-file",
                ".ouroboros/seed_branch3_progress40_research_loop.yaml",
                "--pause-state-file",
                str(pause_state),
                "--write-pause-state",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            for path in poisoned_paths:
                self.assertTrue(path.is_file(), msg=f"{path} should be republished as a file")

    def test_monitor_codex_quota_rejects_non_artifact_attempt_dir(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            escape_attempt_dir = temp_root / "escape_attempt"

            result = self.run_helper(
                "monitor_codex_quota.py",
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(escape_attempt_dir),
                "--report-root",
                str(report_root),
                "--codex-sessions-root",
                str(temp_root / "missing_sessions"),
                "--auth-file",
                str(temp_root / "missing_auth.json"),
                "--once",
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(escape_attempt_dir.exists())
            self.assertIn("output path must stay under", result.stderr)

    def test_monitor_codex_quota_republishes_over_directory_poisoned_targets(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_003"
            poisoned_paths = (
                attempt_dir / "quota_watch_status.json",
                attempt_dir / "quota_watch_status.md",
                report_root / "latest_quota_watch_status.json",
                report_root / "latest_quota_watch_status.md",
            )
            for path in poisoned_paths:
                path.mkdir(parents=True, exist_ok=True)

            result = self.run_helper(
                "monitor_codex_quota.py",
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--codex-sessions-root",
                str(temp_root / "missing_sessions"),
                "--auth-file",
                str(temp_root / "missing_auth.json"),
                "--once",
            )

            self.assertEqual(result.returncode, 1)
            for path in poisoned_paths:
                self.assertTrue(path.is_file(), msg=f"{path} should be republished as a file")

    def test_monitor_codex_quota_normalizes_stale_soft_stop_path_back_under_report_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_001"
            escape_soft_stop = branch_root / ".ouroboros" / "soft_stop_request.json"

            result = self.run_helper(
                "monitor_codex_quota.py",
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--soft-stop-file",
                str(escape_soft_stop),
                "--codex-sessions-root",
                str(temp_root / "missing_sessions"),
                "--auth-file",
                str(temp_root / "missing_auth.json"),
                "--once",
            )

            self.assertEqual(result.returncode, 1)
            self.assertFalse(escape_soft_stop.exists())
            self.assertTrue((attempt_dir / "quota_watch_status.json").exists())
            self.assertTrue((report_root / "latest_quota_watch_status.json").exists())
            self.assertIn("normalized stale soft-stop path", result.stdout)

    def test_post_attempt_guard_rejects_non_artifact_report_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            attempt_dir = artifacts_root / "lca_tree_stress_v5" / "retry_loop" / "attempt_001"
            workflow_log = attempt_dir / "workflow.log"
            attempt_dir.mkdir(parents=True, exist_ok=True)
            workflow_log.write_text("", encoding="utf-8")
            escape_root = temp_root / "escape_guard"

            result = self.run_helper(
                "post_attempt_guard.py",
                "--branch-root",
                str(branch_root),
                "--workflow-log",
                str(workflow_log),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(escape_root),
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(escape_root.exists())
            self.assertIn("output path must stay under", result.stderr)

    def test_post_attempt_guard_republishes_over_directory_poisoned_targets(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_004"
            workflow_log = attempt_dir / "workflow.log"
            workflow_log.parent.mkdir(parents=True, exist_ok=True)
            workflow_log.write_text("", encoding="utf-8")

            poisoned_paths = (
                attempt_dir / "attempt_guard.json",
                attempt_dir / "attempt_guard.md",
                report_root / "latest_attempt_guard.json",
                report_root / "latest_attempt_guard.md",
            )
            for path in poisoned_paths:
                path.mkdir(parents=True, exist_ok=True)

            result = self.run_helper(
                "post_attempt_guard.py",
                "--branch-root",
                str(branch_root),
                "--workflow-log",
                str(workflow_log),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            for path in poisoned_paths:
                self.assertTrue(path.is_file(), msg=f"{path} should be republished as a file")

    def test_git_repo_health_republishes_over_directory_poisoned_targets(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_005"

            poisoned_paths = (
                attempt_dir / "git_repo_health_pre_attempt.json",
                attempt_dir / "git_repo_health_pre_attempt.md",
                report_root / "latest_git_repo_health.json",
                report_root / "latest_git_repo_health.md",
            )
            for path in poisoned_paths:
                path.mkdir(parents=True, exist_ok=True)

            result = self.run_helper(
                "git_repo_health.py",
                "--branch-root",
                str(branch_root),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--phase",
                "pre_attempt",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            for path in poisoned_paths:
                self.assertTrue(path.is_file(), msg=f"{path} should be republished as a file")

    def test_capture_failure_context_rejects_non_artifact_workflow_log(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            escape_workflow_log = temp_root / "escape_workflow.log"

            result = self.run_helper(
                "capture_failure_context.py",
                "--branch-root",
                str(branch_root),
                "--attempt",
                "1",
                "--seed-file",
                ".ouroboros/seed_branch3_progress40_research_loop.yaml",
                "--workflow-log",
                str(escape_workflow_log),
                "--report-root",
                str(report_root),
                "--exit-code",
                "1",
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(escape_workflow_log.exists())
            self.assertIn("output path must stay under", result.stderr)

    def test_capture_failure_context_rejects_non_artifact_report_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            attempt_dir = artifacts_root / "lca_tree_stress_v5" / "retry_loop" / "attempt_001"
            workflow_log = attempt_dir / "workflow.log"
            workflow_log.parent.mkdir(parents=True, exist_ok=True)
            workflow_log.write_text("", encoding="utf-8")
            escape_root = temp_root / "escape_failure_reports"

            result = self.run_helper(
                "capture_failure_context.py",
                "--branch-root",
                str(branch_root),
                "--attempt",
                "1",
                "--seed-file",
                ".ouroboros/seed_branch3_progress40_research_loop.yaml",
                "--workflow-log",
                str(workflow_log),
                "--report-root",
                str(escape_root),
                "--exit-code",
                "1",
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(escape_root.exists())
            self.assertIn("output path must stay under", result.stderr)

    def test_capture_failure_context_accepts_prefixed_artifact_inputs_without_shared_resolver(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(
                temp_root,
                include_shared_resolver=False,
            )
            seed_file = branch_root / ".ouroboros" / "seed_branch3_progress40_research_loop.yaml"
            seed_file.parent.mkdir(parents=True, exist_ok=True)
            seed_file.write_text("goal: retry locality\n", encoding="utf-8")

            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_007"
            workflow_log = attempt_dir / "workflow.log"
            workflow_log.parent.mkdir(parents=True, exist_ok=True)
            workflow_log.write_text(
                "\n".join(
                    [
                        "session_id=orch_test",
                        "execution_id=exec_test",
                        "Traceback (most recent call last):",
                        "ValueError: invalid seed format",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            result = self.run_helper(
                "capture_failure_context.py",
                "--branch-root",
                str(branch_root),
                "--attempt",
                "7",
                "--seed-file",
                str(seed_file.relative_to(branch_root)),
                "--workflow-log",
                f"{branch_root.name}/artifacts/artifacts/lca_tree_stress_v5/retry_loop/attempt_007/workflow.log",
                "--report-root",
                f"{branch_root.name}/artifacts/artifacts/lca_tree_stress_v5/retry_loop",
                "--exit-code",
                "1",
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue((attempt_dir / "failure_report.md").is_file())
            self.assertTrue((attempt_dir / "failure_breakdown.md").is_file())
            self.assertTrue((report_root / "latest_failure_report.md").is_file())
            self.assertTrue((report_root / "latest_failure_breakdown.md").is_file())

    def test_run_next_probe_rejects_non_artifact_report_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_001"
            state_file = branch_root / ".ouroboros" / "failure_analysis_state.json"
            state_file.parent.mkdir(parents=True, exist_ok=True)
            state_file.write_text(json.dumps({"next_probe_command": "true"}) + "\n", encoding="utf-8")
            escape_root = temp_root / "escape_probe_reports"

            result = self.run_helper(
                "run_next_probe.py",
                "--state-file",
                str(state_file),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(escape_root),
                "--branch-root",
                str(branch_root),
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(escape_root.exists())
            self.assertIn("output path must stay under", result.stderr)

    def test_run_next_probe_republishes_over_directory_poisoned_targets(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_006"
            state_file = branch_root / ".ouroboros" / "failure_analysis_state.json"
            state_file.parent.mkdir(parents=True, exist_ok=True)
            state_file.write_text(json.dumps({"next_probe_command": "true"}) + "\n", encoding="utf-8")

            poisoned_paths = (
                attempt_dir / "next_probe_result.json",
                attempt_dir / "next_probe_result.md",
                attempt_dir / "next_probe.stdout.log",
                attempt_dir / "next_probe.stderr.log",
                report_root / "latest_next_probe_result.json",
                report_root / "latest_next_probe_result.md",
                report_root / "latest_next_probe.stdout.log",
                report_root / "latest_next_probe.stderr.log",
            )
            for path in poisoned_paths:
                path.mkdir(parents=True, exist_ok=True)
            probe_runtime_root = attempt_dir / ".probe_runtime_env"
            probe_runtime_root.parent.mkdir(parents=True, exist_ok=True)
            probe_runtime_root.write_text("stale\n", encoding="utf-8")

            result = self.run_helper(
                "run_next_probe.py",
                "--state-file",
                str(state_file),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--branch-root",
                str(branch_root),
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(probe_runtime_root.is_dir(), msg="probe runtime root should be rebuilt as a directory")
            for path in poisoned_paths:
                self.assertTrue(path.is_file(), msg=f"{path} should be republished as a file")

    def test_run_next_probe_binds_probe_runtime_env_under_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root, artifacts_root = self.make_fake_branch(temp_root)
            report_root = artifacts_root / "lca_tree_stress_v5" / "retry_loop"
            attempt_dir = report_root / "attempt_001"
            state_file = branch_root / ".ouroboros" / "failure_analysis_state.json"
            state_file.parent.mkdir(parents=True, exist_ok=True)
            command = " ".join(
                [
                    shlex.quote(sys.executable),
                    "-c",
                    shlex.quote(
                        "import json, os, pathlib; "
                        "tmp = pathlib.Path(os.environ['TMPDIR']).resolve(); "
                        "artifact_tmp = pathlib.Path(os.environ['BRANCH_ARTIFACT_TMP_ROOT']).resolve(); "
                        "home = pathlib.Path(os.environ['HOME']).resolve(); "
                        "xdg_config = pathlib.Path(os.environ['XDG_CONFIG_HOME']).resolve(); "
                        "xdg_cache = pathlib.Path(os.environ['XDG_CACHE_HOME']).resolve(); "
                        "xdg_state = pathlib.Path(os.environ['XDG_STATE_HOME']).resolve(); "
                        "pycache = pathlib.Path(os.environ['PYTHONPYCACHEPREFIX']).resolve(); "
                        "(tmp / 'probe_tmp.txt').write_text('ok', encoding='utf-8'); "
                        "(home / 'probe_home.txt').write_text('ok', encoding='utf-8'); "
                        "(xdg_config / 'probe_config.txt').write_text('ok', encoding='utf-8'); "
                        "(xdg_cache / 'probe_cache.txt').write_text('ok', encoding='utf-8'); "
                        "(xdg_state / 'probe_state.txt').write_text('ok', encoding='utf-8'); "
                        "print(json.dumps({"
                        "'TMPDIR': str(tmp), "
                        "'BRANCH_ARTIFACT_TMP_ROOT': str(artifact_tmp), "
                        "'HOME': str(home), "
                        "'XDG_CONFIG_HOME': str(xdg_config), "
                        "'XDG_CACHE_HOME': str(xdg_cache), "
                        "'XDG_STATE_HOME': str(xdg_state), "
                        "'PYTHONPYCACHEPREFIX': str(pycache)"
                        "}, sort_keys=True))"
                    ),
                ]
            )
            state_file.write_text(
                json.dumps(
                    {
                        "next_probe_command": command,
                        "pinned_primary_axis": "artifact_locality",
                        "pinned_secondary_axis": "retry_loop_probe_env",
                        "why_this_axis": "AC8 probe subprocess routing",
                    }
                )
                + "\n",
                encoding="utf-8",
            )

            result = self.run_helper(
                "run_next_probe.py",
                "--state-file",
                str(state_file),
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--branch-root",
                str(branch_root),
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            payload = json.loads((attempt_dir / "next_probe_result.json").read_text(encoding="utf-8"))
            self.assertEqual(payload["exit_code"], 0)
            self.assertEqual(
                payload["runtime_env"]["runtime_root"],
                str((attempt_dir / ".probe_runtime_env").resolve()),
            )

            env_payload = json.loads((attempt_dir / "next_probe.stdout.log").read_text(encoding="utf-8").strip())
            self.assertEqual(env_payload["TMPDIR"], env_payload["BRANCH_ARTIFACT_TMP_ROOT"])
            for key in (
                "TMPDIR",
                "HOME",
                "XDG_CONFIG_HOME",
                "XDG_CACHE_HOME",
                "XDG_STATE_HOME",
                "PYTHONPYCACHEPREFIX",
            ):
                path = Path(env_payload[key]).resolve()
                try:
                    path.relative_to(artifacts_root.resolve())
                except ValueError:
                    self.fail(f"path escaped artifacts root: {path}")
            self.assertTrue((Path(env_payload["TMPDIR"]) / "probe_tmp.txt").is_file())
            self.assertTrue((Path(env_payload["HOME"]) / "probe_home.txt").is_file())
            self.assertTrue((Path(env_payload["XDG_CONFIG_HOME"]) / "probe_config.txt").is_file())
            self.assertTrue((Path(env_payload["XDG_CACHE_HOME"]) / "probe_cache.txt").is_file())
            self.assertTrue((Path(env_payload["XDG_STATE_HOME"]) / "probe_state.txt").is_file())
            self.assertTrue((report_root / "latest_next_probe_result.json").exists())


if __name__ == "__main__":
    unittest.main()
