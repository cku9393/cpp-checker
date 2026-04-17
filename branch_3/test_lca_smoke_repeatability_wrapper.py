#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


ENTRYPOINT_PATH = Path(__file__).resolve().parent / "lca_smoke_repeatability.sh"
WRAPPER_PATH = Path(__file__).resolve().parent / "outer_suite_wrappers" / "lca_smoke_repeatability.sh"
WRAPPER_SOURCE = WRAPPER_PATH.read_text(encoding="utf-8")
HOST_TMP_ROOT = Path("/tmp/cpp-checker-branch3-smoke-tests/repeatability-host")
shutil.rmtree(HOST_TMP_ROOT, ignore_errors=True)
HOST_TMP_ROOT.mkdir(parents=True, exist_ok=True)
# Keep host-side fake-branch creation independent from any inherited retry-loop
# TMPDIR so direct wrapper execution tests only exercise smoke repeatability.
tempfile.tempdir = str(HOST_TMP_ROOT)


class LcaSmokeRepeatabilityWrapperRegressionTests(unittest.TestCase):
    def write_text(self, path: Path, content: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def make_executable(self, path: Path) -> None:
        path.chmod(path.stat().st_mode | 0o111)

    def symlink_file(self, target: Path, link_path: Path) -> None:
        link_path.parent.mkdir(parents=True, exist_ok=True)
        link_path.symlink_to(target)

    def smoke_stub_source(self, mode: str) -> str:
        if mode == "pass":
            body = """
            write_status_bundle() {
              local status_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"

              rm -rf "$status_root"
              mkdir -p "$status_root"
              cat >"$status_root/summary.txt" <<EOF
            public_status=PASS
            result_family=none
            normalized_exit_code=0
            raw_exit_code=0
            normalized_outcome=pass
            outcome_source=inner_wrapper
            EOF
              printf '# PASS status\\n' > "$status_root/latest_status_report.md"
            }

            write_pass_snapshot() {
              local snapshot_root="${LCA_SMOKE_EXPORT_SNAPSHOT_ROOT:?}"

              rm -rf "$snapshot_root"
              mkdir -p "$snapshot_root/case_alpha"
              cat >"$snapshot_root/case_alpha/run_case.stdout.txt" <<EOF
            stable-prefix
            [run_case] mode=comb_dense time=0.10 mem=1234
            [run_case] artifacts: /tmp/transient/path
            stable-suffix
            EOF
              cat >"$snapshot_root/case_alpha/run_case_result.json" <<EOF
            {
              "verdict": "PASS",
              "sec": 0.10,
              "rss_kb": 1234,
              "case_tag": "alpha"
            }
            EOF
              printf '0.10\\n' > "$snapshot_root/case_alpha/time.txt"
              printf 'external_snapshot_root=%s\\nstable=ok\\n' "$snapshot_root" > "$snapshot_root/case_alpha/environment_validation.txt"
              cat >"$snapshot_root/case_alpha/solver_env_snapshot.json" <<EOF
            {
              "schema": "branch_run_case_solver_env_snapshot_v1",
              "solver": {
                "exists": true,
                "mtime_ns": 101,
                "path": "$snapshot_root/volatile/solve",
                "sha256": "$(basename "$snapshot_root")",
                "size_bytes": 777
              },
              "tracked_env": {
                "DENSE_PROFILE_OUTDIR": "$snapshot_root/volatile/case_alpha",
                "ENABLE_LAYOUT_SIGNATURE_GATE_OPT": "1"
              }
            }
            EOF
              printf 'solver ok\\n' > "$snapshot_root/case_alpha/out.txt"
              printf '[lca_smoke] PASS fixture\\n' >&2
            }

            write_status_bundle
            write_pass_snapshot
            exit 0
            """
        elif mode == "fail":
            body = """
            write_status_bundle() {
              local status_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"

              rm -rf "$status_root"
              mkdir -p "$status_root"
              cat >"$status_root/summary.txt" <<EOF
            public_status=FAIL
            result_family=solver
            normalized_exit_code=1
            raw_exit_code=1
            normalized_outcome=reproducible_solver_failure
            outcome_source=inner_wrapper
            source_failure_kind=acceptance
            source_failure_origin=solver
            source_failure_retryable=0
            triage_stage_scope=inner_wrapper_case
            triage_stage=smoke
            EOF
              printf '# solver FAIL status\\n' > "$status_root/latest_status_report.md"
            }

            write_failure_bundle() {
              local failure_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"

              rm -rf "$failure_root"
              mkdir -p "$failure_root"
              cat >"$failure_root/failure_summary.txt" <<EOF
            exit_code=1
            helper_exit_code=1
            failure_kind=acceptance
            failure_origin=solver
            failure_retryable=0
            failure_summary=stable smoke failure
            solver_exit_code=1
            solver_signal=0
            failed_stage=smoke_case
            failed_case_index=1
            failed_mode=comb_dense
            failed_n=256
            failed_seed=7
            failed_shuffle_labels=0
            failed_shuffle_queries=1
            failed_timeout_s=12
            failed_case_tag=stable_case
            manifest_row=comb_dense\\t256\\t7\\t0\\t1\\t12\\tstable_case
            EOF
              cat >"$failure_root/latest_failure_report.md" <<EOF
            # Stable smoke failure
            EOF
              printf '[lca_smoke] FAIL fixture\\n' >&2
            }

            write_status_bundle
            write_failure_bundle
            exit 1
            """
        elif mode == "harness_fail":
            body = """
            write_status_bundle() {
              local status_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"

              rm -rf "$status_root"
              mkdir -p "$status_root"
              cat >"$status_root/summary.txt" <<EOF
            public_status=FAIL
            result_family=harness
            normalized_exit_code=70
            raw_exit_code=70
            normalized_outcome=harness_infrastructure_failure
            outcome_source=launcher
            triage_stage_scope=launcher
            triage_stage=bootstrap
            EOF
              printf '# harness FAIL status\\n' > "$status_root/latest_status_report.md"
            }

            rm -rf "$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
            write_status_bundle
            printf '[lca_smoke] HARNESS FAIL fixture\\n' >&2
            exit 70
            """
        elif mode == "flip":
            body = """
            COUNTER_PATH="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_flip_counter.txt"

            write_pass_status_bundle() {
              local status_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"

              rm -rf "$status_root"
              mkdir -p "$status_root"
              cat >"$status_root/summary.txt" <<EOF
            public_status=PASS
            result_family=none
            normalized_exit_code=0
            raw_exit_code=0
            normalized_outcome=pass
            outcome_source=inner_wrapper
            EOF
              printf '# PASS status\\n' > "$status_root/latest_status_report.md"
            }

            write_fail_status_bundle() {
              local status_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"

              rm -rf "$status_root"
              mkdir -p "$status_root"
              cat >"$status_root/summary.txt" <<EOF
            public_status=FAIL
            result_family=solver
            normalized_exit_code=1
            raw_exit_code=1
            normalized_outcome=reproducible_solver_failure
            outcome_source=inner_wrapper
            source_failure_kind=acceptance
            source_failure_origin=solver
            source_failure_retryable=0
            triage_stage_scope=inner_wrapper_case
            triage_stage=smoke
            EOF
              printf '# solver FAIL status\\n' > "$status_root/latest_status_report.md"
            }

            write_pass_snapshot() {
              local snapshot_root="${LCA_SMOKE_EXPORT_SNAPSHOT_ROOT:?}"

              rm -rf "$snapshot_root"
              mkdir -p "$snapshot_root/case_alpha"
              printf 'stable line\\n' > "$snapshot_root/case_alpha/out.txt"
              printf '0.10\\n' > "$snapshot_root/case_alpha/time.txt"
            }

            write_failure_bundle() {
              local failure_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"

              rm -rf "$failure_root"
              mkdir -p "$failure_root"
              cat >"$failure_root/failure_summary.txt" <<EOF
            helper_exit_code=1
            failure_kind=acceptance
            failure_origin=solver
            failure_summary=flipped smoke failure
            EOF
              cat >"$failure_root/latest_failure_report.md" <<EOF
            # Flipped smoke failure
            EOF
            }

            count=0
            if [[ -f "$COUNTER_PATH" ]]; then
              read -r count < "$COUNTER_PATH"
            fi
            count="$(( count + 1 ))"
            printf '%s\\n' "$count" > "$COUNTER_PATH"

            if (( count == 1 )); then
              write_pass_status_bundle
              write_pass_snapshot
              exit 0
            fi

            write_fail_status_bundle
            write_failure_bundle
            exit 1
            """
        elif mode == "stale_status":
            body = """
            COUNTER_PATH="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_stale_status_counter.txt"

            write_pass_status_bundle() {
              local status_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"

              rm -rf "$status_root"
              mkdir -p "$status_root"
              cat >"$status_root/summary.txt" <<EOF
            public_status=PASS
            result_family=none
            normalized_exit_code=0
            raw_exit_code=0
            normalized_outcome=pass
            outcome_source=inner_wrapper
            EOF
              printf '# PASS status\\n' > "$status_root/latest_status_report.md"
            }

            write_pass_snapshot() {
              local snapshot_root="${LCA_SMOKE_EXPORT_SNAPSHOT_ROOT:?}"

              rm -rf "$snapshot_root"
              mkdir -p "$snapshot_root/case_alpha"
              printf 'stable line\\n' > "$snapshot_root/case_alpha/out.txt"
              printf '0.10\\n' > "$snapshot_root/case_alpha/time.txt"
            }

            count=0
            if [[ -f "$COUNTER_PATH" ]]; then
              read -r count < "$COUNTER_PATH"
            fi
            count="$(( count + 1 ))"
            printf '%s\\n' "$count" > "$COUNTER_PATH"

            if (( count == 1 )); then
              write_pass_status_bundle
            fi
            write_pass_snapshot
            exit 0
            """
        elif mode == "stale_published_pass_output":
            body = """
            COUNTER_PATH="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_stale_output_counter.txt"

            write_pass_status_bundle() {
              local status_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"

              rm -rf "$status_root"
              mkdir -p "$status_root"
              cat >"$status_root/summary.txt" <<EOF
            public_status=PASS
            result_family=none
            normalized_exit_code=0
            raw_exit_code=0
            normalized_outcome=pass
            outcome_source=inner_wrapper
            EOF
              printf '# PASS status\\n' > "$status_root/latest_status_report.md"
            }

            write_live_output() {
              local output_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"

              rm -rf "$output_root"
              mkdir -p "$output_root/case_alpha"
              cat >"$output_root/suite_config.txt" <<EOF
            case_count=1
            EOF
              cat >"$output_root/suite_plan.tsv" <<EOF
            case_index\\tcase_tag
            1\\tcase_alpha
            EOF
              cat >"$output_root/environment_validation.txt" <<EOF
            external_snapshot_root=<unset>
            stable=ok
            EOF
              printf 'stable line\\n' > "$output_root/case_alpha/out.txt"
              printf '0.10\\n' > "$output_root/case_alpha/time.txt"
            }

            count=0
            if [[ -f "$COUNTER_PATH" ]]; then
              read -r count < "$COUNTER_PATH"
            fi
            count="$(( count + 1 ))"
            printf '%s\\n' "$count" > "$COUNTER_PATH"

            write_pass_status_bundle
            if (( count == 1 )); then
              write_live_output
            fi
            exit 0
            """
        elif mode == "stale_solver_failure_bundle":
            body = """
            COUNTER_PATH="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_stale_failure_counter.txt"

            write_fail_status_bundle() {
              local status_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"

              rm -rf "$status_root"
              mkdir -p "$status_root"
              cat >"$status_root/summary.txt" <<EOF
            public_status=FAIL
            result_family=solver
            normalized_exit_code=1
            raw_exit_code=1
            normalized_outcome=reproducible_solver_failure
            outcome_source=inner_wrapper
            source_failure_kind=acceptance
            source_failure_origin=solver
            source_failure_retryable=0
            triage_stage_scope=inner_wrapper_case
            triage_stage=smoke
            EOF
              printf '# solver FAIL status\\n' > "$status_root/latest_status_report.md"
            }

            write_failure_bundle() {
              local failure_root="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"

              rm -rf "$failure_root"
              mkdir -p "$failure_root"
              cat >"$failure_root/failure_summary.txt" <<EOF
            helper_exit_code=1
            failure_kind=acceptance
            failure_origin=solver
            failure_summary=stable smoke failure
            EOF
              cat >"$failure_root/latest_failure_report.md" <<EOF
            # Stable smoke failure
            EOF
            }

            count=0
            if [[ -f "$COUNTER_PATH" ]]; then
              read -r count < "$COUNTER_PATH"
            fi
            count="$(( count + 1 ))"
            printf '%s\\n' "$count" > "$COUNTER_PATH"

            write_fail_status_bundle
            if (( count == 1 )); then
              write_failure_bundle
            fi
            exit 1
            """
        else:
            raise AssertionError(f"unsupported smoke stub mode: {mode}")

        return (
            textwrap.dedent(
                f"""
                #!/usr/bin/env bash
                set -euo pipefail

                SCRIPT_DIR="$(cd -- "$(dirname -- "${{BASH_SOURCE[0]}}")" && pwd -P)"
                BRANCH_ROOT="$SCRIPT_DIR"
                mkdir -p "$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
                {body}
                """
            ).strip()
            + "\n"
        )

    def make_fake_branch(self, temp_root: Path, smoke_mode: str) -> Path:
        branch_root = temp_root / "branch"
        self.write_text(
            branch_root / "artifact_paths.py",
            textwrap.dedent(
                """
                #!/usr/bin/env python3
                from __future__ import annotations

                import argparse
                from pathlib import Path

                BRANCH_ROOT = Path(__file__).resolve().parent
                ARTIFACTS_ROOT = (BRANCH_ROOT / "artifacts").resolve()
                OUTPUTS = {
                    "lca_smoke": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "smoke",
                    "lca_smoke_repeatability": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "smoke_repeatability",
                }


                def main() -> int:
                    parser = argparse.ArgumentParser()
                    parser.add_argument("key", choices=sorted(OUTPUTS))
                    args = parser.parse_args()
                    print(OUTPUTS[args.key])
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "solver_release_env.sh",
            textwrap.dedent(
                """
                #!/usr/bin/env bash
                set -euo pipefail

                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                : "${BRANCH_ARTIFACT_TMP_ROOT:=$SCRIPT_DIR/artifacts/lca_tree_stress_v5/.tmp/release_env}"
                mkdir -p "$BRANCH_ARTIFACT_TMP_ROOT"
                """
            ).strip()
            + "\n",
        )
        self.symlink_file(ENTRYPOINT_PATH, branch_root / "lca_smoke_repeatability.sh")
        self.symlink_file(WRAPPER_PATH, branch_root / "outer_suite_wrappers" / "lca_smoke_repeatability.sh")
        self.write_text(branch_root / "lca_smoke.sh", self.smoke_stub_source(smoke_mode))

        for rel_path in (
            Path("solver_release_env.sh"),
            Path("lca_smoke.sh"),
        ):
            self.make_executable(branch_root / rel_path)
        return branch_root

    def test_branch_local_entrypoints_are_executable(self) -> None:
        self.assertTrue(
            os.access(ENTRYPOINT_PATH, os.X_OK),
            msg="the branch-local smoke repeatability entrypoint must be directly runnable",
        )
        self.assertTrue(
            os.access(WRAPPER_PATH, os.X_OK),
            msg="the inner smoke repeatability wrapper must remain executable",
        )

    def test_repeatability_targets_the_public_branch_local_smoke_entrypoint(self) -> None:
        self.assertIn(
            'SMOKE_WRAPPER="$BRANCH_ROOT/lca_smoke.sh"',
            WRAPPER_SOURCE,
            msg="smoke repeatability must rerun the stabilized public ./lca_smoke.sh entrypoint",
        )
        self.assertNotIn(
            'SMOKE_WRAPPER="$SCRIPT_DIR/lca_smoke.sh"',
            WRAPPER_SOURCE,
            msg="smoke repeatability must not bypass the public smoke launcher by targeting the inner wrapper directly",
        )

    def test_executes_matching_pass_reruns_and_republishes_without_manual_cleanup(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp), "pass")
            wrapper_path = branch_root / "lca_smoke_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_repeatability"
            stage_root = branch_root / "artifacts" / "lca_tree_stress_v5" / ".repeatability_stage"
            backup_root = repeatability_root.with_name("smoke_repeatability.previous")

            first = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(first.returncode, 0, msg=first.stderr)
            self.assertTrue(repeatability_root.is_dir())
            self.assertFalse(stage_root.exists(), msg="staging dir should be cleaned after a successful publish")
            self.assertFalse(backup_root.exists(), msg="backup output should be removed after publish")

            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("status=PASS", summary)
            self.assertIn("requested_runs=2", summary)
            self.assertIn("completed_runs=2", summary)
            self.assertIn("check_target=./lca_smoke.sh", summary)
            self.assertIn("reproducibility_scope=consecutive_same_worktree_runs", summary)
            self.assertIn("supports_solver_iteration=1", summary)
            self.assertIn("solver_iteration_basis=stable_green_smoke", summary)
            self.assertIn("normalized_files=run_case.stdout.txt,solver_env_snapshot.json", summary)
            self.assertIn(
                "normalized_solver_env_snapshot_fields=solver.mtime_ns,solver.sha256,solver.path,tracked_env.DENSE_PROFILE_OUTDIR",
                summary,
            )
            self.assertIn("baseline_run=run01", summary)
            self.assertIn("baseline_outcome=PASS:0", summary)
            self.assertIn("latest_outcome=PASS:0", summary)
            self.assertIn("outcome_consistency=matching", summary)
            self.assertIn("baseline_result_family=none", summary)
            self.assertIn("latest_result_family=none", summary)
            self.assertTrue((repeatability_root / "runs" / "run01" / "manifest.tsv").is_file())
            self.assertTrue((repeatability_root / "runs" / "run02" / "manifest.tsv").is_file())
            self.assertTrue((repeatability_root / "runs" / "run01" / "status_signature.txt").is_file())
            self.assertTrue((repeatability_root / "runs" / "run02" / "status_signature.txt").is_file())

            second = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(second.returncode, 0, msg=second.stderr)
            self.assertTrue(repeatability_root.is_dir())
            self.assertFalse(stage_root.exists(), msg="staging dir should still be cleaned on rerun")
            self.assertFalse(backup_root.exists(), msg="backup output should not survive a clean rerun publish")
            self.assertIn("status=PASS", (repeatability_root / "summary.txt").read_text(encoding="utf-8"))

    def test_executes_matching_fail_reruns_and_publishes_consistent_fail_summary(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp), "fail")
            wrapper_path = branch_root / "lca_smoke_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_repeatability"
            stage_root = branch_root / "artifacts" / "lca_tree_stress_v5" / ".repeatability_stage"

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0, msg="consistent failing reruns should keep a non-zero public exit")
            self.assertTrue(repeatability_root.is_dir())
            self.assertFalse(stage_root.exists(), msg="staging dir should be cleaned after a published consistent failure")

            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("status=CONSISTENT_FAIL", summary)
            self.assertIn("requested_runs=2", summary)
            self.assertIn("completed_runs=2", summary)
            self.assertIn("check_target=./lca_smoke.sh", summary)
            self.assertIn("reproducibility_scope=consecutive_same_worktree_runs", summary)
            self.assertIn("supports_solver_iteration=1", summary)
            self.assertIn("solver_iteration_basis=stable_solver_failure_signal", summary)
            self.assertIn("normalized_files=run_case.stdout.txt,solver_env_snapshot.json", summary)
            self.assertIn("baseline_outcome=FAIL:1", summary)
            self.assertIn("latest_outcome=FAIL:1", summary)
            self.assertIn("outcome_consistency=matching", summary)
            self.assertIn("baseline_result_family=solver", summary)
            self.assertIn("latest_result_family=solver", summary)
            self.assertTrue((repeatability_root / "runs" / "run01" / "failure_signature.txt").is_file())
            self.assertTrue((repeatability_root / "runs" / "run02" / "failure_signature.txt").is_file())
            self.assertTrue((repeatability_root / "runs" / "run01" / "status_signature.txt").is_file())
            self.assertTrue((repeatability_root / "runs" / "run02" / "status_signature.txt").is_file())

    def test_rejects_consistent_harness_failures_as_solver_iteration_signal(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp), "harness_fail")
            wrapper_path = branch_root / "lca_smoke_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_repeatability"

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0, msg="repeated harness failures must still block solver iteration")
            self.assertTrue(repeatability_root.is_dir())

            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("status=FAIL", summary)
            self.assertNotIn("status=CONSISTENT_FAIL", summary)
            self.assertIn("check_target=./lca_smoke.sh", summary)
            self.assertIn("reproducibility_scope=consecutive_same_worktree_runs", summary)
            self.assertIn("supports_solver_iteration=0", summary)
            self.assertIn("solver_iteration_basis=stable_non_solver_failure", summary)
            self.assertIn("normalized_files=run_case.stdout.txt,solver_env_snapshot.json", summary)
            self.assertIn("baseline_outcome=FAIL:70", summary)
            self.assertIn("latest_outcome=FAIL:70", summary)
            self.assertIn("outcome_consistency=matching", summary)
            self.assertIn("baseline_result_family=harness", summary)
            self.assertIn("latest_result_family=harness", summary)
            self.assertIn("failure_reason=smoke repeated a non-solver failure across 2 runs (FAIL:70)", summary)
            self.assertTrue((repeatability_root / "runs" / "run01" / "status_signature.txt").is_file())
            self.assertTrue((repeatability_root / "runs" / "run02" / "status_signature.txt").is_file())

    def test_rejects_outcome_divergence_between_reruns(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp), "flip")
            wrapper_path = branch_root / "lca_smoke_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_repeatability"

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0, msg="mixed smoke outcomes must fail the repeatability check")
            self.assertTrue(repeatability_root.is_dir())

            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("status=FAIL", summary)
            self.assertIn("check_target=./lca_smoke.sh", summary)
            self.assertIn("reproducibility_scope=consecutive_same_worktree_runs", summary)
            self.assertIn("supports_solver_iteration=0", summary)
            self.assertIn("solver_iteration_basis=diverged_back_to_back_runs", summary)
            self.assertIn("normalized_files=run_case.stdout.txt,solver_env_snapshot.json", summary)
            self.assertIn("baseline_outcome=PASS:0", summary)
            self.assertIn("latest_outcome=FAIL:1", summary)
            self.assertIn("outcome_consistency=diverged", summary)
            self.assertIn("baseline_result_family=none", summary)
            self.assertIn("latest_result_family=solver", summary)
            self.assertIn("failure_reason=smoke outcome divergence between run01 (PASS:0) and run02 (FAIL:1)", summary)

    def test_continues_collecting_iterations_after_first_outcome_divergence(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp), "flip")
            wrapper_path = branch_root / "lca_smoke_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_repeatability"
            counter_path = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_flip_counter.txt"

            result = subprocess.run(
                [str(wrapper_path), "3"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )

            self.assertNotEqual(result.returncode, 0, msg="diverged smoke outcomes must still fail repeatability")
            self.assertTrue(repeatability_root.is_dir())
            self.assertEqual("3\n", counter_path.read_text(encoding="utf-8"))
            self.assertTrue((repeatability_root / "runs" / "run03" / "outcome.txt").is_file())

            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("requested_runs=3", summary)
            self.assertIn("completed_runs=3", summary)
            self.assertIn("first_failed_run=run02", summary)
            self.assertIn("first_failure_kind=outcome_divergence", summary)
            self.assertIn("failure_count=2", summary)
            self.assertIn("failure_events=failure_events.tsv", summary)
            self.assertIn("failure_reason=smoke outcome divergence between run01 (PASS:0) and run02 (FAIL:1)", summary)

            failure_events = (repeatability_root / "failure_events.tsv").read_text(encoding="utf-8")
            self.assertIn("run02\toutcome_divergence\t", failure_events)
            self.assertIn("run03\toutcome_divergence\t", failure_events)

    def test_rejects_stale_status_bundle_reuse_even_when_smoke_stays_green(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp), "stale_status")
            wrapper_path = branch_root / "lca_smoke_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_repeatability"

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0, msg="stale status reuse must fail repeatability")
            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("status=FAIL", summary)
            self.assertIn("baseline_outcome=PASS:0", summary)
            self.assertIn("latest_outcome=PASS:0", summary)
            self.assertIn("outcome_consistency=matching", summary)
            self.assertIn("supports_solver_iteration=0", summary)
            self.assertIn("failure_reason=smoke status bundle was not regenerated for runs/run02", summary)
            freshness_report = repeatability_root / "runs" / "run02" / "status_bundle_freshness.txt"
            self.assertTrue(freshness_report.is_file())
            self.assertIn("status=stale_or_missing_current_run_artifacts", freshness_report.read_text(encoding="utf-8"))

    def test_rejects_stale_published_output_fallback_reuse(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp), "stale_published_pass_output")
            wrapper_path = branch_root / "lca_smoke_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_repeatability"

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0, msg="stale published smoke output must fail repeatability")
            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("status=FAIL", summary)
            self.assertIn("baseline_outcome=PASS:0", summary)
            self.assertIn("latest_outcome=PASS:0", summary)
            self.assertIn("supports_solver_iteration=0", summary)
            self.assertIn("failure_reason=smoke pass reused stale published output for runs/run02", summary)
            freshness_report = repeatability_root / "runs" / "run02" / "output_bundle_freshness.txt"
            self.assertTrue(freshness_report.is_file())
            self.assertIn("bundle=published smoke output", freshness_report.read_text(encoding="utf-8"))

    def test_rejects_stale_solver_failure_bundle_reuse(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp), "stale_solver_failure_bundle")
            wrapper_path = branch_root / "lca_smoke_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_repeatability"

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(result.returncode, 0, msg="stale solver failure bundles must fail repeatability")
            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("status=FAIL", summary)
            self.assertIn("baseline_outcome=FAIL:1", summary)
            self.assertIn("latest_outcome=FAIL:1", summary)
            self.assertIn("supports_solver_iteration=0", summary)
            self.assertIn("failure_reason=smoke solver-failure bundle was not regenerated for runs/run02", summary)
            freshness_report = repeatability_root / "runs" / "run02" / "failure_bundle_freshness.txt"
            self.assertTrue(freshness_report.is_file())
            self.assertIn("bundle=smoke_latest_failure", freshness_report.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
