#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import tempfile
import textwrap
import time
import unittest
from pathlib import Path


WRAPPER_PATH = Path(__file__).resolve().parent / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
WRAPPER_SOURCE = WRAPPER_PATH.read_text(encoding="utf-8")


class LcaBoj3sGateWrapperRegressionTests(unittest.TestCase):
    def write_text(self, path: Path, content: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def make_executable(self, path: Path) -> None:
        path.chmod(path.stat().st_mode | 0o111)

    def make_fake_branch(self, temp_root: Path) -> Path:
        branch_root = temp_root / "branch"
        tooling_root = temp_root / "lca_tree_stress_v5" / "tooling"
        self.write_text(branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh", WRAPPER_SOURCE)
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
                    "lca_boj3s_gate": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "boj3s_gate",
                }


                def main() -> int:
                    parser = argparse.ArgumentParser()
                    parser.add_argument("key", nargs="?")
                    parser.add_argument("path", nargs="?")
                    parser.add_argument("--ensure", dest="ensure", default=None)
                    parser.add_argument("--snapshot-non-artifact-tree", dest="snapshot", default=None)
                    parser.add_argument("--verify-non-artifact-tree", nargs=3)
                    args = parser.parse_args()
                    if args.ensure is not None:
                        print(Path(args.ensure).resolve())
                        return 0
                    if args.snapshot is not None:
                        snapshot_path = Path(args.snapshot)
                        snapshot_path.parent.mkdir(parents=True, exist_ok=True)
                        snapshot_path.write_text("{}\\n", encoding="utf-8")
                        return 0
                    if args.verify_non_artifact_tree is not None:
                        _, current_path, report_path = args.verify_non_artifact_tree
                        Path(current_path).write_text("{}\\n", encoding="utf-8")
                        Path(report_path).write_text("ok\\n", encoding="utf-8")
                        return 0
                    out = OUTPUTS[args.key]
                    if args.path:
                        out = out / args.path
                    print(out)
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "build.sh",
            textwrap.dedent(
                """
                #!/usr/bin/env bash
                set -euo pipefail

                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                OUT="$SCRIPT_DIR/artifacts/boj28350_resume/build/solve"
                mkdir -p "$(dirname "$OUT")"
                printf '#!/usr/bin/env bash\\nexit 0\\n' > "$OUT"
                chmod +x "$OUT"
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

                : "${BRANCH_ARTIFACT_TMP_ROOT:?missing}"
                mkdir -p "$BRANCH_ARTIFACT_TMP_ROOT"
                """
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "branch_certify_suite.py",
            textwrap.dedent(
                """
                #!/usr/bin/env python3
                from __future__ import annotations

                import argparse
                import json
                from pathlib import Path


                def main() -> int:
                    parser = argparse.ArgumentParser()
                    parser.add_argument("--solver", required=True)
                    parser.add_argument("--preset", required=True)
                    parser.add_argument("--out", required=True)
                    parser.add_argument("--limit-scale", required=True)
                    args = parser.parse_args()

                    out_dir = Path(args.out)
                    case_dir = out_dir / "runs" / "final_gate" / "stub_mode" / "n1" / "seed1_L1_Q1"
                    case_dir.mkdir(parents=True, exist_ok=True)
                    (case_dir / "time.txt").write_text("0.100000 123\\n", encoding="utf-8")

                    payload = {
                        "verdict": "PASS",
                        "preset": "stub_boj3s_gate",
                        "reasons": [],
                        "stages": [
                            {
                                "name": "required_final",
                                "status": "PASS",
                                "cases": 1,
                                "timeouts": 0,
                                "re_wa": 0,
                                "limit_scale": 1.0,
                                "scale_fail": [],
                            }
                        ],
                    }
                    (out_dir / "certify.json").write_text(json.dumps(payload) + "\\n", encoding="utf-8")
                    (out_dir / "certify_summary.md").write_text("# ok\\n", encoding="utf-8")
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "branch_outer_certify.py",
            textwrap.dedent(
                """
                #!/usr/bin/env python3
                from __future__ import annotations
                """
            ).strip()
            + "\n",
        )
        self.write_text(
            tooling_root / "certify_suite.py",
            textwrap.dedent(
                """
                #!/usr/bin/env python3
                from __future__ import annotations
                """
            ).strip()
            + "\n",
        )
        self.write_text(branch_root / "branch_gen_case.py", "# legacy stub\n")
        self.write_text(branch_root / "branch_validator.py", "# legacy stub\n")
        self.write_text(branch_root / "branch_gen_case_local.py", "# local stub\n")
        self.write_text(branch_root / "branch_validator_local.py", "# local stub\n")
        self.write_text(branch_root / "branch_gen_case_local.py", "# stub\n")
        self.write_text(branch_root / "branch_validator_local.py", "# stub\n")
        self.write_text(branch_root / "branch_generators_local" / "__init__.py", "# stub\n")
        self.write_text(
            branch_root / "boj28350_resume.py",
            textwrap.dedent(
                """
                #!/usr/bin/env python3
                from __future__ import annotations


                def main() -> int:
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """
            ).strip()
            + "\n",
        )
        self.write_text(branch_root / "boj28350_resume" / "boj28350_branch_3_solver.cpp", "// stub solver\n")
        self.write_text(
            branch_root / "suite_presets" / "boj_3s_hard_gate.json",
            '{"name":"boj_3s_hard_gate","stages":[{"name":"required_final","modes":["stub_mode"],"sizes":[1],"seeds":[1],"shuffle_labels":[1],"shuffle_queries":[1],"must_pass":true}]}\n',
        )

        for rel_path in (
            Path("outer_suite_wrappers/lca_boj3s_gate.sh"),
            Path("artifact_paths.py"),
            Path("build.sh"),
            Path("solver_release_env.sh"),
            Path("branch_certify_suite.py"),
            Path("branch_outer_certify.py"),
            Path("branch_gen_case_local.py"),
            Path("branch_validator_local.py"),
            Path("boj28350_resume.py"),
            Path("../lca_tree_stress_v5/tooling/certify_suite.py"),
        ):
            self.make_executable((branch_root / rel_path).resolve())

        return branch_root

    def test_non_artifact_output_locality_scan_is_captured_and_reported(self) -> None:
        self.assertIn(
            'NON_ARTIFACT_BASELINE="$WORKDIR/non_artifact_tree_baseline.json"',
            WRAPPER_SOURCE,
            msg="boj3s gate must keep its non-artifact baseline in the staging workdir",
        )
        self.assertIn(
            'capture_output_locality_baseline',
            WRAPPER_SOURCE,
            msg="boj3s gate must capture a branch-root baseline before build and certify run",
        )
        self.assertIn(
            '--snapshot-non-artifact-tree "$NON_ARTIFACT_BASELINE"',
            WRAPPER_SOURCE,
            msg="boj3s gate must persist a non-artifact baseline manifest under artifacts",
        )
        self.assertIn(
            '--verify-non-artifact-tree',
            WRAPPER_SOURCE,
            msg="boj3s gate must compare the post-run tree against the non-artifact baseline",
        )
        self.assertIn(
            'write_failure_summary "output_locality" "$locality_rc" "non-artifact output locality verification failed"',
            WRAPPER_SOURCE,
            msg="boj3s gate must fail explicitly when a run writes outside branch-local artifacts",
        )
        self.assertIn(
            'non-artifact output locality report after cleanup: $FAILED_ROOT/$(basename "$NON_ARTIFACT_REPORT")',
            WRAPPER_SOURCE,
            msg="boj3s failure hints must advertise the preserved non-artifact locality report",
        )

    def test_runtime_artifacts_live_under_staging_workdir(self) -> None:
        self.assertIn(
            'CASE_RUN_TMP_ROOT="$WORKDIR/.case_runs_tmp"',
            WRAPPER_SOURCE,
            msg="boj3s gate must bind case-run temp roots under the active staging workdir",
        )
        self.assertIn(
            'CASE_CACHE_ROOT="$WORKDIR/.case_cache"',
            WRAPPER_SOURCE,
            msg="boj3s gate must bind case cache roots under the active staging workdir",
        )
        self.assertIn(
            'CASE_CACHE_TMP_ROOT="$WORKDIR/.case_cache_tmp"',
            WRAPPER_SOURCE,
            msg="boj3s gate must bind case cache tmp roots under the active staging workdir",
        )
        self.assertIn(
            'PRECHECK_MANIFEST="$WORKDIR/preflight_manifest.tsv"',
            WRAPPER_SOURCE,
            msg="preflight manifest must be emitted into the boj3s staging workdir",
        )
        self.assertIn(
            'ENV_SNAPSHOT="$WORKDIR/runtime_env.txt"',
            WRAPPER_SOURCE,
            msg="runtime env snapshot must be emitted into the boj3s staging workdir",
        )
        self.assertIn(
            'CERTIFY_STDOUT_LOG="$WORKDIR/certify.stdout.txt"',
            WRAPPER_SOURCE,
            msg="certify stdout must be emitted into the boj3s staging workdir",
        )
        self.assertIn(
            'CERTIFY_STDERR_LOG="$WORKDIR/certify.stderr.txt"',
            WRAPPER_SOURCE,
            msg="certify stderr must be emitted into the boj3s staging workdir",
        )
        self.assertIn(
            'PRESET_SNAPSHOT_PATH="$WORKDIR/selected_preset.json"',
            WRAPPER_SOURCE,
            msg="selected preset snapshot must stay inside the boj3s staging workdir",
        )
        self.assertIn(
            'CERTIFY_FAILURE_DETAILS_PATH="$WORKDIR/certify_failure_details.tsv"',
            WRAPPER_SOURCE,
            msg="certify-stage diagnostics must stay inside the boj3s staging workdir",
        )
        self.assertIn(
            'FILTERDIR="$WORKDIR/.stage_filter"',
            WRAPPER_SOURCE,
            msg="stage-filtered boj3s presets must be materialized under the staging workdir, not the transient runtime tmpdir",
        )
        self.assertIn(
            'SNAPSHOT_ROOT="$ARTIFACTS_ROOT/.solver_snapshots/lca_boj3s_gate"',
            WRAPPER_SOURCE,
            msg="boj3s gate must keep solver snapshots in a gate-specific artifact namespace",
        )
        self.assertIn(
            'SOLVER_SNAPSHOT="$(mktemp "$SNAPSHOT_ROOT/lca_boj3s_gate.solver.XXXXXX")"',
            WRAPPER_SOURCE,
            msg="the certify solver snapshot must be materialized through a unique artifact-root file",
        )
        self.assertIn(
            'clear_invalid_root_path "$SNAPSHOT_ROOT" "solver snapshot root"',
            WRAPPER_SOURCE,
            msg="stale solver snapshot roots must be cleared before each boj3s retry",
        )

    def test_preflight_validates_selected_preset_and_certify_imports(self) -> None:
        self.assertIn(
            'STAGE_FILTER="${LCA_STAGE_FILTER:-}"',
            WRAPPER_SOURCE,
            msg="boj3s gate must accept an optional stage filter for lighter harness probes",
        )
        self.assertIn(
            'PRESET_SOURCE_MATERIALIZED=""',
            WRAPPER_SOURCE,
            msg="boj3s gate must track any recovered preset source copy separately from the source path",
        )
        self.assertIn(
            'if check_selected_preset_source_ready; then',
            WRAPPER_SOURCE,
            msg="boj3s gate must validate and recover the selected preset source before loading the runtime env",
        )
        self.assertIn(
            'if prepare_selected_preset; then',
            WRAPPER_SOURCE,
            msg="boj3s gate must materialize the exact preset payload it will hand to certify",
        )
        self.assertIn(
            'check_json_file_recorded "$PRESET" "selected preset json"',
            WRAPPER_SOURCE,
            msg="boj3s preflight must validate the selected preset JSON before certify starts",
        )
        self.assertIn(
            'check_python_entrypoint_recorded "$CERTIFY_HELPER" "branch-local certify helper imports"',
            WRAPPER_SOURCE,
            msg="boj3s preflight must validate that the branch-local certify helper imports cleanly",
        )
        self.assertIn(
            'python3 "$CERTIFY_HELPER" --solver "$SOLVER_SNAPSHOT" --preset "$PRESET" --out "$WORKDIR" --limit-scale "$LIMIT_SCALE" >"$CERTIFY_STDOUT_LOG" 2>"$CERTIFY_STDERR_LOG" &',
            WRAPPER_SOURCE,
            msg="boj3s certify invocations must use the prepared preset path and preserve raw certify stdout/stderr for failures",
        )
        self.assertIn(
            'check_artifact_path_recorded "$CASE_RUN_TMP_ROOT" "case_run_tmp_root"',
            WRAPPER_SOURCE,
            msg="boj3s preflight must validate the run-local case temp root path",
        )
        self.assertIn(
            'check_artifact_path_recorded "$CASE_CACHE_ROOT" "case_cache_root"',
            WRAPPER_SOURCE,
            msg="boj3s preflight must validate the run-local case cache root path",
        )
        self.assertIn(
            'check_artifact_path_recorded "$CASE_CACHE_TMP_ROOT" "case_cache_tmp_root"',
            WRAPPER_SOURCE,
            msg="boj3s preflight must validate the run-local case cache tmp root path",
        )

    def test_preflight_rejects_runtime_env_paths_outside_artifacts(self) -> None:
        for env_name in (
            "BRANCH_ARTIFACT_TMP_ROOT",
            "TMPDIR",
            "HOME",
            "XDG_CACHE_HOME",
            "PYTHONPYCACHEPREFIX",
        ):
            self.assertIn(
                env_name,
                WRAPPER_SOURCE,
                msg=f"boj3s preflight must validate {env_name} as part of the runtime artifact envelope",
            )
        self.assertIn(
            'check_artifact_path_recorded "$env_var_value" "$env_var_name"',
            WRAPPER_SOURCE,
            msg="runtime env validation must route through artifact-path checks",
        )
        self.assertIn(
            'echo "enable_prev_state_writeback_opt=${ENABLE_PREV_STATE_WRITEBACK_OPT:-}"',
            WRAPPER_SOURCE,
            msg="boj3s runtime snapshots must record the writeback toggle so repeated runs can compare the full solver config",
        )
        self.assertIn(
            'echo "solver_env_scrub=compiler_and_ENABLE_PROFILE_DENSE_RUN_TAG"',
            WRAPPER_SOURCE,
            msg="boj3s runtime snapshots must record the scrub policy that stabilizes ambient solver env handling",
        )

    def test_missing_preset_is_reported_through_preflight_instead_of_eager_fail(self) -> None:
        self.assertIn(
            'PRESET_SOURCE="$(resolve_preset || true)"',
            WRAPPER_SOURCE,
            msg="preset resolution must stay soft until the wrapper can publish a failure bundle",
        )
        self.assertIn(
            'record_preflight_check "path" "branch_preset_candidate" "ok" "$BRANCH_PRESET"',
            WRAPPER_SOURCE,
            msg="preflight must record the branch-local preset candidate path",
        )
        self.assertIn(
            'record_preflight_check "path" "outer_preset_candidate" "ok" "$OUTER_PRESET"',
            WRAPPER_SOURCE,
            msg="preflight must record the outer preset fallback path",
        )
        self.assertIn(
            'record_preflight_check "file" "selected preset source" "missing" "$BRANCH_PRESET | $OUTER_PRESET"',
            WRAPPER_SOURCE,
            msg="missing preset failures must point at both candidate preset paths",
        )

    def test_build_and_certify_failures_write_failure_reports_before_cleanup(self) -> None:
        self.assertIn(
            'if "$BUILD_WRAPPER" >"$BUILD_STDOUT_LOG" 2>"$BUILD_STDERR_LOG"; then',
            WRAPPER_SOURCE,
            msg="build failures must capture stdout/stderr without losing the real exit code to shell negation",
        )
        self.assertIn(
            'write_failure_summary "build" "$build_rc" "build wrapper failed"',
            WRAPPER_SOURCE,
            msg="build failures must write a boj3s failure summary before cleanup",
        )
        self.assertIn(
            'if run_certify_suite; then',
            WRAPPER_SOURCE,
            msg="certify failure handling must preserve the real certify exit code instead of the shell-negated status",
        )
        self.assertIn(
            'write_failure_summary "certify" "$certify_rc" "certify suite failed"',
            WRAPPER_SOURCE,
            msg="certify failures must write a boj3s failure summary before cleanup",
        )
        self.assertIn(
            'capture_certify_failure_details "$phase"',
            WRAPPER_SOURCE,
            msg="certify failures must derive exact stage details before writing the preserved failure bundle",
        )
        self.assertIn(
            'echo "- Exact failure stage: \\`$exact_failure_stage\\`"',
            WRAPPER_SOURCE,
            msg="boj3s failure reports must surface the exact failing certify stage",
        )
        self.assertIn(
            'failure snapshot root after cleanup: $FAILED_ROOT',
            WRAPPER_SOURCE,
            msg="stderr failure hints must point at the final preserved latest_failure root",
        )
        self.assertIn(
            'failure report after cleanup: $FAILED_ROOT/$(basename "$FAILURE_REPORT_PATH")',
            WRAPPER_SOURCE,
            msg="failure hints must advertise the final failure-report path after the staging dir is moved",
        )
        self.assertIn(
            'certify failure details after cleanup: $FAILED_ROOT/$(basename "$CERTIFY_FAILURE_DETAILS_PATH")',
            WRAPPER_SOURCE,
            msg="certify failure hints must advertise the preserved stage-details snapshot after cleanup",
        )

    def test_certify_failure_bundle_reports_exact_stage_and_preserves_certify_logs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            self.write_text(
                branch_root / "branch_certify_suite.py",
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import argparse
                    import csv
                    import json
                    import sys
                    import textwrap
                    from pathlib import Path


                    def main() -> int:
                        parser = argparse.ArgumentParser()
                        parser.add_argument("--solver", required=True)
                        parser.add_argument("--preset", required=True)
                        parser.add_argument("--out", required=True)
                        parser.add_argument("--limit-scale", required=True)
                        args = parser.parse_args()

                        out_dir = Path(args.out)
                        out_dir.mkdir(parents=True, exist_ok=True)
                        print("certify stdout marker")
                        print("certify stderr marker", file=sys.stderr)

                        payload = {
                            "verdict": "FAIL",
                            "preset": "stub_boj3s_gate",
                            "reasons": [
                                "required_final: 1 failing cases",
                                "secondary_stage: 2 failing cases",
                            ],
                            "stages": [
                                {
                                    "name": "required_final",
                                    "status": "FAIL",
                                    "cases": 3,
                                    "timeouts": 1,
                                    "re_wa": 0,
                                    "limit_scale": 1.0,
                                    "scale_fail": [],
                                },
                                {
                                    "name": "secondary_stage",
                                    "status": "FAIL",
                                    "cases": 4,
                                    "timeouts": 2,
                                    "re_wa": 1,
                                    "limit_scale": 1.0,
                                    "scale_fail": [],
                                },
                            ],
                        }
                        (out_dir / "certify.json").write_text(json.dumps(payload) + "\\n", encoding="utf-8")
                        (out_dir / "certify_summary.md").write_text(
                            textwrap.dedent(
                                \"\"\"
                                # Certification summary

                                overall verdict: **FAIL**

                                ## Reasons

                                - required_final: 1 failing cases
                                - secondary_stage: 2 failing cases

                                ## Stage: required_final

                                status: **FAIL**  
                                cases: 3  
                                timeouts: 1  
                                re/wa: 0  

                                ## Stage: secondary_stage

                                status: **FAIL**  
                                cases: 4  
                                timeouts: 2  
                                re/wa: 1  
                                \"\"\"
                            ).strip()
                            + "\\n",
                            encoding="utf-8",
                        )
                        with (out_dir / "certify_rows.csv").open("w", encoding="utf-8", newline="") as handle:
                            writer = csv.writer(handle)
                            writer.writerow(["stage", "mode", "n", "seed", "shuffle_labels", "shuffle_queries", "gen_ok", "solver_rc", "timed_out", "val_ok", "sec", "rss_kb", "case_dir"])
                            writer.writerow(["required_final", "stub_mode", "1", "1", "1", "1", "1", "124", "1", "0", "", "", str(out_dir / "runs" / "required_final")])
                        return 19


                    if __name__ == "__main__":
                        raise SystemExit(main())
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "branch_certify_suite.py")

            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"},
                check=False,
            )

            self.assertEqual(result.returncode, 19, msg=result.stderr)

            failed_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate.latest_failure"
            failure_summary = (failed_root / "failure_summary.txt").read_text(encoding="utf-8")
            failure_report = (failed_root / "latest_failure_report.md").read_text(encoding="utf-8")
            certify_stdout = (failed_root / "certify.stdout.txt").read_text(encoding="utf-8")
            certify_stderr = (failed_root / "certify.stderr.txt").read_text(encoding="utf-8")
            certify_details = (failed_root / "certify_failure_details.tsv").read_text(encoding="utf-8")

            self.assertIn("exact_failure_stage=certify:required_final", failure_summary)
            self.assertIn(
                f"certify_failure_details={failed_root / 'certify_failure_details.tsv'}",
                failure_summary,
            )
            self.assertIn(
                f"certify_rows={failed_root / 'certify_rows.csv'}",
                failure_summary,
            )
            self.assertIn("certify_primary_failed_stage=required_final", failure_summary)
            self.assertIn("certify_failed_stages=required_final,secondary_stage", failure_summary)

            self.assertIn("certify stdout marker", certify_stdout)
            self.assertIn("certify stderr marker", certify_stderr)
            self.assertIn("exact_failure_stage\tcertify:required_final", certify_details)
            self.assertIn("primary_failed_stage\trequired_final", certify_details)
            self.assertIn("failed_stages\trequired_final,secondary_stage", certify_details)
            self.assertIn(f"certify_rows\t{failed_root / 'certify_rows.csv'}", certify_details)

            self.assertIn("- Exact failure stage: `certify:required_final`", failure_report)
            self.assertIn("## Certify Failure Details", failure_report)
            self.assertIn("- Primary failed stage: `required_final`", failure_report)
            self.assertIn("- Failing stages: `required_final,secondary_stage`", failure_report)
            self.assertIn(f"- Certify rows: `{failed_root / 'certify_rows.csv'}`", failure_report)
            self.assertIn("## Certify stdout tail", failure_report)
            self.assertIn("## Certify stderr tail", failure_report)

            self.assertIn("certify stdout after cleanup", result.stderr)
            self.assertIn("certify stderr after cleanup", result.stderr)
            self.assertIn("certify failure details after cleanup", result.stderr)

    def test_stale_lock_cleanup_matches_required_gate_repeatability_expectations(self) -> None:
        self.assertIn(
            'STALE_LOCK_SECONDS="${LCA_STALE_LOCK_SECONDS:-60}"',
            WRAPPER_SOURCE,
            msg="boj3s gate must age out stale locks after a bounded no-activity window",
        )
        self.assertIn(
            'clearing stale lock held by pid $holder',
            WRAPPER_SOURCE,
            msg="boj3s gate must recover from a stale no-activity lock without manual cleanup",
        )
        self.assertIn(
            'fail "another lca_boj3s_gate.sh run is active (pid $holder)"',
            WRAPPER_SOURCE,
            msg="boj3s gate must preserve a live holder lock instead of clearing it mid-run",
        )
        self.assertIn(
            'if find "$LOCK_ACTIVITY_PATHS_FILE" -newer "$LOCK_PID_FILE" -print -quit 2>/dev/null | grep -q .; then',
            WRAPPER_SOURCE,
            msg="boj3s stale-lock checks must treat a refreshed lock activity file as live holder activity",
        )
        self.assertIn(
            'touch "$LOCK_ACTIVITY_PATHS_FILE" 2>/dev/null || true',
            WRAPPER_SOURCE,
            msg="boj3s gate must refresh the lock activity heartbeat while a long-running holder is alive",
        )

    def test_recent_lock_activity_file_keeps_live_holder_lock(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            lock_dir = branch_root / "artifacts" / "lca_tree_stress_v5" / ".locks" / "lca_boj3s_gate"
            lock_dir.mkdir(parents=True, exist_ok=True)
            holder = subprocess.Popen(["sleep", "30"])
            try:
                (lock_dir / "pid").write_text(f"{holder.pid}\n", encoding="utf-8")
                activity_file = lock_dir / "runtime_paths.tsv"
                activity_file.write_text("workdir\t/stale/path\n", encoding="utf-8")
                time.sleep(0.05)
                activity_file.touch()

                result = subprocess.run(
                    [str(wrapper_path)],
                    cwd=branch_root,
                    capture_output=True,
                    text=True,
                    env={**os.environ, "LCA_STALE_LOCK_SECONDS": "0"},
                    check=False,
                )

                self.assertNotEqual(
                    result.returncode,
                    0,
                    msg="wrapper should not clear a live holder lock when the activity file was refreshed recently",
                )
                self.assertIn(
                    "another lca_boj3s_gate.sh run is active",
                    result.stderr,
                    msg="wrapper should preserve the live holder lock instead of clearing it as stale",
                )
            finally:
                holder.terminate()
                try:
                    holder.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    holder.kill()
                    holder.wait(timeout=5)

    def test_release_env_uses_run_local_runtime_roots(self) -> None:
        self.assertIn(
            'RUN_TMP_TEMPLATE="lca_boj3s_gate.env.XXXXXX"',
            WRAPPER_SOURCE,
            msg="boj3s gate must allocate a dedicated runtime tmp namespace per run",
        )
        self.assertIn(
            'export HOME="$RUN_TMPDIR/home"',
            WRAPPER_SOURCE,
            msg="boj3s gate must isolate HOME under the per-run runtime tmpdir before sourcing release env",
        )
        self.assertIn(
            'export XDG_CACHE_HOME="$RUN_TMPDIR/xdg_cache"',
            WRAPPER_SOURCE,
            msg="boj3s gate must isolate XDG cache state under the per-run runtime tmpdir",
        )
        self.assertIn(
            'export PYTHONPYCACHEPREFIX="$RUN_TMPDIR/pycache"',
            WRAPPER_SOURCE,
            msg="boj3s gate must isolate Python bytecode under the per-run runtime tmpdir",
        )
        self.assertGreaterEqual(
            WRAPPER_SOURCE.count('export BRANCH_ARTIFACT_TMP_ROOT="$RUN_TMPDIR"'),
            2,
            msg="boj3s gate must rebind BRANCH_ARTIFACT_TMP_ROOT before and after sourcing release env",
        )
        self.assertIn(
            'if load_release_environment; then',
            WRAPPER_SOURCE,
            msg="boj3s gate must load the release environment through the run-local rebinding helper",
        )
        self.assertIn(
            'sanitize_solver_environment',
            WRAPPER_SOURCE,
            msg="boj3s gate must scrub ambient solver controls before sourcing the release env",
        )
        self.assertIn(
            'done < <(compgen -v)',
            WRAPPER_SOURCE,
            msg="boj3s gate must scrub non-exported shell PROFILE_/ENABLE_ vars before sourcing the release env",
        )
        self.assertIn(
            'unset CC CXX CPPFLAGS CFLAGS CXXFLAGS LDFLAGS SDKROOT MACOSX_DEPLOYMENT_TARGET LOCAL_SKIP_SELF_TEST',
            WRAPPER_SOURCE,
            msg="boj3s gate must clear ambient compiler and solver-toggle vars before loading release env",
        )
        self.assertIn(
            'ENABLE_*|PROFILE_*|DENSE_*|RUN_TAG',
            WRAPPER_SOURCE,
            msg="boj3s gate must scrub ambient profiling and solver feature toggles from the host shell",
        )
        self.assertIn(
            'assert_runtime_environment',
            WRAPPER_SOURCE,
            msg="boj3s gate must validate that release env stayed inside the run-local runtime roots",
        )

    def test_heartbeat_counts_published_cases_alongside_live_case_tmpdirs(self) -> None:
        self.assertIn(
            'local published_count=0',
            WRAPPER_SOURCE,
            msg="boj3s heartbeat must track cases already published into the staging workdir",
        )
        self.assertIn(
            'local active_count=0',
            WRAPPER_SOURCE,
            msg="boj3s heartbeat must track currently active tmp case runs separately",
        )
        self.assertIn(
            "find \"$WORKDIR/runs\" -type f \\( -name 'run_case_result.json' -o -name 'time.txt' \\) 2>/dev/null |",
            WRAPPER_SOURCE,
            msg="boj3s heartbeat must count published case result markers, not only successful time.txt files",
        )
        self.assertIn(
            "find \"$CASE_RUN_TMP_ROOT\" -type f \\( -name 'run_case_result.json' -o -name 'time.txt' \\) -newer \"$LOCK_PID_FILE\" 2>/dev/null |",
            WRAPPER_SOURCE,
            msg="boj3s heartbeat must treat timed-out tmp cases as completed once they publish a result marker",
        )
        self.assertIn(
            "sed 's#/[^/]*$##' |",
            WRAPPER_SOURCE,
            msg="boj3s heartbeat must deduplicate per-case result markers so PASS cases with both files count once",
        )
        self.assertIn(
            'printf \'%s\\n\' "$(( published_count + active_count ))"',
            WRAPPER_SOURCE,
            msg="boj3s heartbeat must report total completed cases instead of hiding published progress while the lock is held",
        )
        self.assertIn(
            'BRANCH_CERTIFY_CASE_RUN_TMP_ROOT="$CASE_RUN_TMP_ROOT" \\',
            WRAPPER_SOURCE,
            msg="boj3s gate must pass the run-local case-run tmp root into the certify helper",
        )
        self.assertIn(
            'BRANCH_CERTIFY_CASE_CACHE_ROOT="$CASE_CACHE_ROOT" \\',
            WRAPPER_SOURCE,
            msg="boj3s gate must pass the run-local case-cache root into the certify helper",
        )
        self.assertIn(
            'BRANCH_CERTIFY_CASE_CACHE_TMP_ROOT="$CASE_CACHE_TMP_ROOT" \\',
            WRAPPER_SOURCE,
            msg="boj3s gate must pass the run-local case-cache tmp root into the certify helper",
        )

    def test_wrapper_executes_successfully_when_certify_reports_pass(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"},
                check=False,
            )

            self.assertEqual(
                result.returncode,
                0,
                msg=f"wrapper should exit 0 after a branch-local certify PASS, stderr was:\n{result.stderr}",
            )
            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate"
            self.assertTrue((outroot / "certify.json").exists(), msg="successful boj3s runs must publish certify.json")
            self.assertTrue((outroot / "certify_summary.md").exists(), msg="successful boj3s runs must publish certify_summary.md")
            self.assertTrue((outroot / "preflight_manifest.tsv").exists(), msg="successful boj3s runs must publish the preflight manifest")
            self.assertTrue((outroot / "runtime_env.txt").exists(), msg="successful boj3s runs must publish the runtime env snapshot")
            self.assertTrue((outroot / "selected_preset.json").exists(), msg="successful boj3s runs must publish the selected preset snapshot")
            self.assertTrue(
                (outroot / "repeatability_gate_manifest.txt").exists(),
                msg="successful boj3s runs must publish the current-run repeatability manifest",
            )
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate.latest_failure").exists(),
                msg="successful boj3s runs must not preserve a latest_failure bundle",
            )

    def test_wrapper_republishes_cleanly_across_two_successful_invocations(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            env = {**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"}

            first = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env=env,
                check=False,
            )
            second = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env=env,
                check=False,
            )

            self.assertEqual(first.returncode, 0, msg=first.stderr)
            self.assertEqual(second.returncode, 0, msg=second.stderr)

            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate"
            self.assertTrue((outroot / "certify.json").exists(), msg="reruns must republish certify.json at the fixed boj3s outroot")
            self.assertTrue((outroot / "certify_summary.md").exists(), msg="reruns must republish certify_summary.md at the fixed boj3s outroot")
            self.assertTrue((outroot / "preflight_manifest.tsv").exists(), msg="reruns must republish the current-run preflight manifest")
            self.assertTrue((outroot / "runtime_env.txt").exists(), msg="reruns must republish the current-run runtime env snapshot")
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate.previous").exists(),
                msg="successful reruns must not require a lingering manual-cleanup backup tree",
            )
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate.latest_failure").exists(),
                msg="successful reruns must not leave a stale latest_failure bundle behind",
            )

    def test_wrapper_scrubs_stale_solver_snapshots_without_manual_cleanup(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            env = {**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"}
            snapshot_parent = branch_root / "artifacts" / "lca_tree_stress_v5" / ".solver_snapshots"
            snapshot_root = snapshot_parent / "lca_boj3s_gate"
            snapshot_root.mkdir(parents=True, exist_ok=True)
            (snapshot_root / "lca_boj3s_gate.solver.stale").write_text("stale\n", encoding="utf-8")
            (snapshot_parent / "lca_boj3s_gate.solver.legacy").write_text("legacy\n", encoding="utf-8")

            first = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env=env,
                check=False,
            )
            second = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env=env,
                check=False,
            )

            self.assertEqual(first.returncode, 0, msg=first.stderr)
            self.assertEqual(second.returncode, 0, msg=second.stderr)
            self.assertFalse(
                snapshot_root.exists(),
                msg="successful reruns must clean the gate-specific solver snapshot root instead of leaving stale snapshot files behind",
            )
            self.assertFalse(
                (snapshot_parent / "lca_boj3s_gate.solver.legacy").exists(),
                msg="successful reruns must scrub legacy misplaced solver snapshots without manual cleanup",
            )

    def test_wrapper_stage_filter_prepares_and_invokes_a_filtered_preset(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            self.write_text(
                branch_root / "suite_presets" / "boj_3s_hard_gate.json",
                textwrap.dedent(
                    """
                    {
                      "name": "boj_3s_hard_gate",
                      "stages": [
                        {
                          "name": "required_final",
                          "modes": ["stub_mode"],
                          "sizes": [1],
                          "seeds": [1],
                          "shuffle_labels": [1],
                          "shuffle_queries": [1],
                          "must_pass": true
                        },
                        {
                          "name": "secondary_stage",
                          "modes": ["unused_mode"],
                          "sizes": [2],
                          "seeds": [2],
                          "shuffle_labels": [0],
                          "shuffle_queries": [0],
                          "must_pass": true
                        }
                      ]
                    }
                    """
                ).strip()
                + "\n",
            )
            self.write_text(
                branch_root / "branch_certify_suite.py",
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import argparse
                    import json
                    from pathlib import Path


                    def main() -> int:
                        parser = argparse.ArgumentParser()
                        parser.add_argument("--solver", required=True)
                        parser.add_argument("--preset", required=True)
                        parser.add_argument("--out", required=True)
                        parser.add_argument("--limit-scale", required=True)
                        args = parser.parse_args()

                        preset = json.loads(Path(args.preset).read_text(encoding="utf-8"))
                        stage_names = [stage["name"] for stage in preset.get("stages", [])]
                        if stage_names != ["required_final"]:
                            raise SystemExit(f"unexpected preset stages: {stage_names}")

                        out_dir = Path(args.out)
                        out_dir.mkdir(parents=True, exist_ok=True)
                        (out_dir / "received_preset_path.txt").write_text(args.preset + "\\n", encoding="utf-8")
                        (out_dir / "certify.json").write_text(
                            json.dumps(
                                {
                                    "verdict": "PASS",
                                    "preset": preset.get("name", "filtered"),
                                    "reasons": [],
                                    "stages": [{"name": "required_final", "status": "PASS", "cases": 1, "timeouts": 0, "re_wa": 0, "limit_scale": 1.0, "scale_fail": []}],
                                }
                            )
                            + "\\n",
                            encoding="utf-8",
                        )
                        (out_dir / "certify_summary.md").write_text("# ok\\n", encoding="utf-8")
                        return 0


                    if __name__ == "__main__":
                        raise SystemExit(main())
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "branch_certify_suite.py")

            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01", "LCA_STAGE_FILTER": "required_final"},
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)

            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate"
            runtime_env = (outroot / "runtime_env.txt").read_text(encoding="utf-8")
            self.assertIn("stage_filter=required_final", runtime_env)

            received_preset_path = (outroot / "received_preset_path.txt").read_text(encoding="utf-8").strip()
            self.assertNotEqual(
                received_preset_path,
                str(branch_root / "suite_presets" / "boj_3s_hard_gate.json"),
                msg="stage-filtered boj3s runs must invoke certify with the prepared filtered preset path",
            )
            self.assertNotIn(
                "/.tmp/lca_boj3s_gate.env.",
                received_preset_path,
                msg="stage-filtered boj3s runs must not hand certify a preset path under the transient runtime tmpdir",
            )
            self.assertIn(
                "/.stage_filter/preset.json",
                received_preset_path,
                msg="stage-filtered boj3s runs must hand certify the staging-workdir filtered preset",
            )

            selected_preset = (outroot / "selected_preset.json").read_text(encoding="utf-8")
            self.assertIn('"name": "required_final"', selected_preset)
            self.assertNotIn('"name": "secondary_stage"', selected_preset)

    def test_dataless_selected_preset_is_materialized_before_certify_runs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            fake_bin = branch_root / "fake_bin"
            fake_bin.mkdir()
            self.write_text(
                fake_bin / "stat",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    if [[ "$#" -ge 3 && "$1" == "-f" && "$2" == "%Sf" && "$3" == *"/suite_presets/boj_3s_hard_gate.json" ]]; then
                      printf 'compressed,dataless\\n'
                      exit 0
                    fi

                    exec /usr/bin/stat "$@"
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(fake_bin / "stat")
            self.write_text(
                branch_root / "branch_certify_suite.py",
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import argparse
                    import json
                    from pathlib import Path


                    def main() -> int:
                        parser = argparse.ArgumentParser()
                        parser.add_argument("--solver", required=True)
                        parser.add_argument("--preset", required=True)
                        parser.add_argument("--out", required=True)
                        parser.add_argument("--limit-scale", required=True)
                        args = parser.parse_args()

                        json.loads(Path(args.preset).read_text(encoding="utf-8"))
                        out_dir = Path(args.out)
                        out_dir.mkdir(parents=True, exist_ok=True)
                        (out_dir / "received_preset_path.txt").write_text(args.preset + "\\n", encoding="utf-8")
                        (out_dir / "certify.json").write_text(
                            json.dumps(
                                {
                                    "verdict": "PASS",
                                    "preset": "stub_boj3s_gate",
                                    "reasons": [],
                                    "stages": [{"name": "required_final", "status": "PASS", "cases": 1, "timeouts": 0, "re_wa": 0, "limit_scale": 1.0, "scale_fail": []}],
                                }
                            )
                            + "\\n",
                            encoding="utf-8",
                        )
                        (out_dir / "certify_summary.md").write_text("# ok\\n", encoding="utf-8")
                        return 0


                    if __name__ == "__main__":
                        raise SystemExit(main())
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "branch_certify_suite.py")

            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={**os.environ, "PATH": f"{fake_bin}{os.pathsep}{os.environ['PATH']}", "LCA_HEARTBEAT_INTERVAL": "0.01"},
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)

            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate"
            received_preset_path = (outroot / "received_preset_path.txt").read_text(encoding="utf-8").strip()
            self.assertNotEqual(
                received_preset_path,
                str(branch_root / "suite_presets" / "boj_3s_hard_gate.json"),
                msg="recoverable dataless presets should be materialized into a run-local preset path before certify",
            )

            runtime_env = (outroot / "runtime_env.txt").read_text(encoding="utf-8")
            self.assertIn("selected_preset_source=", runtime_env)
            self.assertIn("selected_preset_source_materialized=", runtime_env)

            preflight_manifest = (outroot / "preflight_manifest.tsv").read_text(encoding="utf-8")
            self.assertIn(
                "materialization\tselected preset source\trecovered\t",
                preflight_manifest,
                msg="boj3s preflight must record when a dataless preset source was recovered into a run-local copy",
            )

    def test_artifact_preset_fallback_rejects_newer_stage_filtered_snapshot(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            (branch_root / "suite_presets" / "boj_3s_hard_gate.json").unlink()

            full_snapshot_root = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "boj3s_gate.failure_archive"
                / "full_snapshot"
            )
            filtered_snapshot_root = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "boj3s_gate.failure_archive"
                / "filtered_snapshot"
            )

            full_preset = {
                "name": "boj_3s_hard_gate",
                "stages": [
                    {
                        "name": "required_final",
                        "modes": ["stub_mode"],
                        "sizes": [1],
                        "seeds": [1],
                        "shuffle_labels": [1],
                        "shuffle_queries": [1],
                        "must_pass": True,
                    },
                    {
                        "name": "secondary_stage",
                        "modes": ["unused_mode"],
                        "sizes": [2],
                        "seeds": [2],
                        "shuffle_labels": [0],
                        "shuffle_queries": [0],
                        "must_pass": True,
                    },
                ],
            }
            filtered_preset = {
                "name": "boj_3s_hard_gate",
                "stages": [full_preset["stages"][0]],
            }

            self.write_text(
                full_snapshot_root / "selected_preset.json",
                json.dumps(full_preset, indent=2) + "\n",
            )
            self.write_text(full_snapshot_root / "runtime_env.txt", "stage_filter=\n")
            os.utime(full_snapshot_root / "selected_preset.json", (1, 1))
            os.utime(full_snapshot_root / "runtime_env.txt", (1, 1))

            self.write_text(
                filtered_snapshot_root / "selected_preset.json",
                json.dumps(filtered_preset, indent=2) + "\n",
            )
            self.write_text(filtered_snapshot_root / "runtime_env.txt", "stage_filter=required_final\n")
            os.utime(filtered_snapshot_root / "selected_preset.json", (2, 2))
            os.utime(filtered_snapshot_root / "runtime_env.txt", (2, 2))

            self.write_text(
                branch_root / "branch_certify_suite.py",
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import argparse
                    import json
                    from pathlib import Path


                    def main() -> int:
                        parser = argparse.ArgumentParser()
                        parser.add_argument("--solver", required=True)
                        parser.add_argument("--preset", required=True)
                        parser.add_argument("--out", required=True)
                        parser.add_argument("--limit-scale", required=True)
                        args = parser.parse_args()

                        preset = json.loads(Path(args.preset).read_text(encoding="utf-8"))
                        stage_names = [stage["name"] for stage in preset.get("stages", [])]
                        if stage_names != ["required_final", "secondary_stage"]:
                            raise SystemExit(f"unexpected preset stages: {stage_names}")

                        out_dir = Path(args.out)
                        out_dir.mkdir(parents=True, exist_ok=True)
                        (out_dir / "received_preset_path.txt").write_text(args.preset + "\\n", encoding="utf-8")
                        (out_dir / "certify.json").write_text(
                            json.dumps(
                                {
                                    "verdict": "PASS",
                                    "preset": preset.get("name", "fallback"),
                                    "reasons": [],
                                    "stages": [
                                        {
                                            "name": "required_final",
                                            "status": "PASS",
                                            "cases": 1,
                                            "timeouts": 0,
                                            "re_wa": 0,
                                            "limit_scale": 1.0,
                                            "scale_fail": [],
                                        }
                                    ],
                                }
                            )
                            + "\\n",
                            encoding="utf-8",
                        )
                        (out_dir / "certify_summary.md").write_text("# ok\\n", encoding="utf-8")
                        return 0


                    if __name__ == "__main__":
                        raise SystemExit(main())
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "branch_certify_suite.py")

            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"},
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)

            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate"
            received_preset_path = (outroot / "received_preset_path.txt").read_text(encoding="utf-8").strip()
            self.assertEqual(
                received_preset_path,
                str(full_snapshot_root / "selected_preset.json"),
                msg="artifact fallback must reject newer stage-filtered preset snapshots and reuse only a full-gate snapshot",
            )

    def test_repeated_failed_reruns_archive_the_prior_failure_bundle(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            self.write_text(
                branch_root / "branch_certify_suite.py",
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import argparse
                    from pathlib import Path


                    def main() -> int:
                        parser = argparse.ArgumentParser()
                        parser.add_argument("--out", required=True)
                        parser.add_argument("--preset", required=True)
                        parser.add_argument("--solver", required=True)
                        parser.add_argument("--limit-scale", required=True)
                        args = parser.parse_args()
                        out_dir = Path(args.out)
                        out_dir.mkdir(parents=True, exist_ok=True)
                        (out_dir / "certify.partial.txt").write_text("intentional failure\\n", encoding="utf-8")
                        return 11


                    if __name__ == "__main__":
                        raise SystemExit(main())
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "branch_certify_suite.py")

            run_tag = "retry_loop/subac2_failed_rerun"
            failed_root = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "boj3s_gate"
                / "retry_loop"
                / "subac2_failed_rerun.latest_failure"
            )
            archive_root = failed_root.with_name("subac2_failed_rerun.failure_archive")

            first = subprocess.run(
                [str(wrapper_path), run_tag],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"},
                check=False,
            )
            self.assertEqual(first.returncode, 11, msg=first.stderr)
            self.assertTrue((failed_root / "failure_summary.txt").exists())

            second = subprocess.run(
                [str(wrapper_path), run_tag],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"},
                check=False,
            )
            self.assertEqual(second.returncode, 11, msg=second.stderr)
            self.assertTrue((failed_root / "failure_summary.txt").exists())
            archived_dirs = sorted(archive_root.glob("subac2_failed_rerun.latest_failure*"))
            self.assertEqual(len(archived_dirs), 1, msg=f"unexpected archives: {archived_dirs}")
            self.assertTrue((archived_dirs[0] / "failure_summary.txt").exists())

    def test_incomplete_previous_output_is_archived_instead_of_carried_forward_on_failure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            self.write_text(
                branch_root / "branch_certify_suite.py",
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import argparse
                    from pathlib import Path


                    def main() -> int:
                        parser = argparse.ArgumentParser()
                        parser.add_argument("--out", required=True)
                        parser.add_argument("--preset", required=True)
                        parser.add_argument("--solver", required=True)
                        parser.add_argument("--limit-scale", required=True)
                        args = parser.parse_args()
                        out_dir = Path(args.out)
                        out_dir.mkdir(parents=True, exist_ok=True)
                        (out_dir / "certify.partial.txt").write_text("intentional failure\\n", encoding="utf-8")
                        return 11


                    if __name__ == "__main__":
                        raise SystemExit(main())
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "branch_certify_suite.py")

            run_tag = "retry_loop/subac4_incomplete_previous"
            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "boj3s_gate" / "retry_loop" / "subac4_incomplete_previous"
            failed_root = outroot.with_name("subac4_incomplete_previous.latest_failure")
            archive_root = outroot.with_name("subac4_incomplete_previous.failure_archive")
            outroot.mkdir(parents=True, exist_ok=True)
            (outroot / "certify.json").write_text('{"verdict":"PASS"}\n', encoding="utf-8")
            (outroot / "certify_summary.md").write_text("# stale prior summary\n", encoding="utf-8")

            result = subprocess.run(
                [str(wrapper_path), run_tag],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"},
                check=False,
            )

            self.assertEqual(result.returncode, 11, msg=result.stderr)
            self.assertFalse(
                outroot.exists(),
                msg="failed reruns must not keep an incomplete legacy published output tree at the live outroot",
            )
            archived_dirs = sorted(archive_root.glob("subac4_incomplete_previous.incomplete_published.*"))
            self.assertEqual(len(archived_dirs), 1, msg=f"unexpected archives: {archived_dirs}")
            self.assertTrue((archived_dirs[0] / "certify.json").exists())
            self.assertTrue((failed_root / "failure_summary.txt").exists())
            self.assertIn("archived incomplete published output", result.stderr)

    def test_live_holder_lock_is_not_cleared_while_recent_branch_local_activity_continues(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_boj3s_gate.sh"
            self.write_text(
                branch_root / "branch_certify_suite.py",
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import argparse
                    import json
                    import os
                    import time
                    from pathlib import Path


                    def main() -> int:
                        parser = argparse.ArgumentParser()
                        parser.add_argument("--solver", required=True)
                        parser.add_argument("--preset", required=True)
                        parser.add_argument("--out", required=True)
                        parser.add_argument("--limit-scale", required=True)
                        args = parser.parse_args()

                        out_dir = Path(args.out)
                        out_dir.mkdir(parents=True, exist_ok=True)
                        case_tmp_root = Path(os.environ["BRANCH_CERTIFY_CASE_RUN_TMP_ROOT"])
                        heartbeat = case_tmp_root / "holder_live" / "time.txt"
                        heartbeat.parent.mkdir(parents=True, exist_ok=True)
                        for idx in range(12):
                            heartbeat.write_text(f"{idx}\\n", encoding="utf-8")
                            time.sleep(0.2)

                        payload = {
                            "verdict": "PASS",
                            "preset": "stub_boj3s_gate",
                            "reasons": [],
                            "stages": [
                                {
                                    "name": "required_final",
                                    "status": "PASS",
                                    "cases": 1,
                                    "timeouts": 0,
                                    "re_wa": 0,
                                    "limit_scale": 1.0,
                                    "scale_fail": [],
                                }
                            ],
                        }
                        (out_dir / "certify.json").write_text(json.dumps(payload) + "\\n", encoding="utf-8")
                        (out_dir / "certify_summary.md").write_text("# ok\\n", encoding="utf-8")
                        return 0


                    if __name__ == "__main__":
                        raise SystemExit(main())
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "branch_certify_suite.py")

            env = {**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01", "LCA_STALE_LOCK_SECONDS": "1"}
            first = subprocess.Popen(
                [str(wrapper_path)],
                cwd=branch_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                env=env,
            )
            try:
                lock_meta = (
                    branch_root
                    / "artifacts"
                    / "lca_tree_stress_v5"
                    / ".locks"
                    / "lca_boj3s_gate"
                    / "runtime_paths.tsv"
                )
                deadline = time.time() + 10.0
                while time.time() < deadline and not lock_meta.exists():
                    time.sleep(0.05)
                self.assertTrue(lock_meta.exists(), msg="holder must publish tracked activity paths into the lock")

                time.sleep(1.2)
                second = subprocess.run(
                    [str(wrapper_path)],
                    cwd=branch_root,
                    capture_output=True,
                    text=True,
                    env=env,
                    check=False,
                )
                self.assertNotEqual(second.returncode, 0, msg="concurrent rerun must not clear a live active holder lock")
                self.assertIn("another lca_boj3s_gate.sh run is active", second.stderr)
            finally:
                first_stdout, first_stderr = first.communicate(timeout=15)

            self.assertEqual(first.returncode, 0, msg=f"holder run should finish successfully, stderr was:\n{first_stderr}")


if __name__ == "__main__":
    unittest.main()
