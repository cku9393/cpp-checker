#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


WRAPPER_PATH = Path(__file__).resolve().parent / "outer_suite_wrappers" / "lca_strong_gate.sh"
WRAPPER_SOURCE = WRAPPER_PATH.read_text(encoding="utf-8")


class LcaStrongGateWrapperRegressionTests(unittest.TestCase):
    def test_branch_local_materialized_strong_gate_preset_cache_stays_available(self) -> None:
        repo_root = Path(__file__).resolve().parent
        preset_cache = repo_root / "artifacts" / "lca_tree_stress_v5" / ".preset_cache" / "lca_strong_gate.json"

        self.assertTrue(
            preset_cache.is_file(),
            msg="branch_3 should ship a materialized strong-gate preset cache so dataless iCloud presets do not block AC3 preflight",
        )

        payload = json.loads(preset_cache.read_text(encoding="utf-8"))
        self.assertEqual(payload.get("name"), "strong_gate")
        self.assertEqual(
            [stage.get("name") for stage in payload.get("stages", [])],
            ["correctness_fuzz", "hard_scaling", "max_n_mix"],
            msg="the branch-local preset cache must preserve the full strong-gate stage surface, not a narrowed probe snapshot",
        )

    def test_wrapper_ignores_sighup_during_long_gate_runs(self) -> None:
        self.assertIn(
            "trap '' HUP",
            WRAPPER_SOURCE,
            msg="strong gate wrapper should ignore SIGHUP so long certify runs are not torn down by parent-session hangups",
        )

    def write_text(self, path: Path, content: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def make_executable(self, path: Path) -> None:
        path.chmod(path.stat().st_mode | 0o111)

    def make_fake_branch(self, temp_root: Path) -> Path:
        branch_root = temp_root / "branch"
        tooling_root = temp_root / "lca_tree_stress_v5" / "tooling"
        self.write_text(branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh", WRAPPER_SOURCE)
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
                    "lca_strong_gate": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "strong_gate",
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
                META="${OUT}.build_meta.json"
                mkdir -p "$(dirname "$OUT")"
                printf '#!/usr/bin/env bash\\nexit 0\\n' > "$OUT"
                chmod +x "$OUT"
                cat > "$META" <<'EOF'
                {
                  "schema": "boj28350_build_metadata_v1",
                  "source": "boj28350_resume/boj28350_branch_3_solver.cpp",
                  "output": "artifacts/boj28350_resume/build/solve",
                  "compiler": "stub",
                  "command": ["stub"],
                  "requested_compiler": "",
                  "cxx_env": "",
                  "static_mode": "auto",
                  "defines": []
                }
                EOF
                printf '[build] output=%s\\n' "$OUT"
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
                if [[ -n "${CXX:-}" ]]; then
                  echo "ambient CXX leaked into solver_release_env" >&2
                  return 17 2>/dev/null || exit 17
                fi
                if [[ -n "${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-}" ]]; then
                  echo "ambient ENABLE_STATE_LOAD_MATERIALIZATION_OPT leaked into solver_release_env" >&2
                  return 18 2>/dev/null || exit 18
                fi
                mkdir -p "$BRANCH_ARTIFACT_TMP_ROOT"
                export PROFILE_MODE="${PROFILE_MODE:-PROFILE_NONE}"
                export LOCAL_SKIP_SELF_TEST="${LOCAL_SKIP_SELF_TEST:-1}"
                export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-0}"
                export ENABLE_PREV_STATE_WRITEBACK_OPT="${ENABLE_PREV_STATE_WRITEBACK_OPT:-1}"
                export ENABLE_LAYOUT_SIGNATURE_GATE_OPT="${ENABLE_LAYOUT_SIGNATURE_GATE_OPT:-1}"
                export ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT="${ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT:-1}"
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
                import csv
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
                    case_dir = out_dir / "runs" / "correctness_fuzz" / "stub_mode" / "n1" / "seed1_L1_Q1"
                    case_dir.mkdir(parents=True, exist_ok=True)
                    (case_dir / "time.txt").write_text("0.100000 123\\n", encoding="utf-8")

                    payload = {
                        "verdict": "PASS",
                        "preset": "stub_strong_gate",
                        "reasons": [],
                        "stages": [
                            {
                                "name": "correctness_fuzz",
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
                    with (out_dir / "certify_rows.csv").open("w", encoding="utf-8", newline="") as f:
                        writer = csv.writer(f)
                        writer.writerow(
                            ["stage", "mode", "n", "seed", "shuffle_labels", "shuffle_queries", "ran", "solver_rc", "timed_out", "val_ok", "sec", "rss_kb", "case_dir"]
                        )
                        writer.writerow(["correctness_fuzz", "stub_mode", 1, 1, 1, 1, 1, 0, 0, 1, 0.1, 123, str(case_dir)])
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
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
        self.write_text(branch_root / "branch_gen_case.py", "# stub\n")
        self.write_text(branch_root / "branch_validator.py", "# stub\n")
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
            branch_root / "suite_presets" / "strong_gate.json",
            textwrap.dedent(
                """
                {
                  "name": "strong_gate",
                  "stages": [
                    {
                      "name": "correctness_fuzz",
                      "modes": ["stub_mode"],
                      "sizes": [1],
                      "seeds": [1],
                      "shuffle_labels": [1],
                      "shuffle_queries": [1],
                      "must_pass": true
                    },
                    {
                      "name": "hard_scaling",
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

        for rel_path in (
            Path("outer_suite_wrappers/lca_strong_gate.sh"),
            Path("artifact_paths.py"),
            Path("build.sh"),
            Path("solver_release_env.sh"),
            Path("branch_certify_suite.py"),
            Path("boj28350_resume.py"),
            Path("../lca_tree_stress_v5/tooling/certify_suite.py"),
        ):
            self.make_executable((branch_root / rel_path).resolve())

        return branch_root

    def test_gate_builds_under_run_local_env_and_executes_a_frozen_solver_snapshot(self) -> None:
        self.assertIn(
            'SNAPSHOT_ROOT="$ARTIFACTS_ROOT/.solver_snapshots/lca_strong_gate"',
            WRAPPER_SOURCE,
            msg="strong gate must keep stale-snapshot cleanup scoped to a strong-gate-specific artifact namespace",
        )
        self.assertIn(
            'SOLVER_SNAPSHOT="$WORKDIR/solver_snapshot"',
            WRAPPER_SOURCE,
            msg="strong gate must freeze a per-run solver snapshot inside the active staging workdir",
        )
        self.assertIn(
            'python3 "$CERTIFY_HELPER" --solver "$SOLVER_SNAPSHOT" --preset "$PRESET" --out "$WORKDIR" --limit-scale "$LIMIT_SCALE" >"$CERTIFY_STDOUT_LOG" 2>"$CERTIFY_STDERR_LOG"',
            WRAPPER_SOURCE,
            msg="strong gate must hand the certify helper the frozen solver snapshot instead of the shared build output",
        )
        self.assertIn(
            'cp "$SOLVER_SNAPSHOT" "$FAILED_ROOT/solver_snapshot" 2>/dev/null || true',
            WRAPPER_SOURCE,
            msg="strong gate failures must preserve the exact frozen solver snapshot for replay",
        )
        self.assertIn(
            'remove_path_retry "$SOLVER_SNAPSHOT"',
            WRAPPER_SOURCE,
            msg="strong gate cleanup must remove the run-local solver snapshot on exit",
        )
        self.assertIn(
            'BUILD_METADATA_SNAPSHOT="$root/solver_build_meta.json"',
            WRAPPER_SOURCE,
            msg="strong gate must snapshot the built solver metadata alongside the frozen solver binary",
        )
        self.assertIn(
            'bind_runtime_artifacts_to_root "$WORKDIR"',
            WRAPPER_SOURCE,
            msg="strong gate must bind runtime artifacts under the active staging root before the run starts",
        )
        self.assertLess(
            WRAPPER_SOURCE.rfind("if load_release_environment; then"),
            WRAPPER_SOURCE.rfind("if run_build_wrapper; then"),
            msg="strong gate must load its run-local release environment before building the solver for that run",
        )

    def test_non_artifact_output_locality_scan_is_enforced_before_publish(self) -> None:
        self.assertIn(
            'NON_ARTIFACT_BASELINE="$root/non_artifact_tree_baseline.json"',
            WRAPPER_SOURCE,
            msg="strong gate must store its non-artifact baseline inside the staging workdir",
        )
        self.assertIn(
            '--snapshot-non-artifact-tree "$NON_ARTIFACT_BASELINE"',
            WRAPPER_SOURCE,
            msg="strong gate must capture a branch-root baseline before the run mutates files",
        )
        self.assertIn(
            'NON_ARTIFACT_CURRENT="$root/non_artifact_tree_current.json"',
            WRAPPER_SOURCE,
            msg="strong gate must keep the post-run non-artifact tree snapshot inside the staging workdir",
        )
        self.assertIn(
            'NON_ARTIFACT_REPORT="$root/non_artifact_tree_report.txt"',
            WRAPPER_SOURCE,
            msg="strong gate must keep the non-artifact locality report inside the staging workdir",
        )
        self.assertIn(
            'ensure_under_artifacts "$NON_ARTIFACT_CURRENT"',
            WRAPPER_SOURCE,
            msg="strong gate must reject non-artifact tree snapshots that escape branch-local artifacts",
        )
        self.assertIn(
            'ensure_under_artifacts "$NON_ARTIFACT_REPORT"',
            WRAPPER_SOURCE,
            msg="strong gate must reject non-artifact locality reports that escape branch-local artifacts",
        )
        self.assertIn(
            '--verify-non-artifact-tree',
            WRAPPER_SOURCE,
            msg="strong gate must compare the post-run tree against the non-artifact baseline",
        )
        self.assertIn(
            'non-artifact output locality violation report: $NON_ARTIFACT_REPORT',
            WRAPPER_SOURCE,
            msg="strong gate must surface the preserved non-artifact locality report on failure",
        )

    def test_runtime_artifacts_live_under_staging_workdir(self) -> None:
        self.assertIn(
            'PRECHECK_MANIFEST="$root/preflight_manifest.tsv"',
            WRAPPER_SOURCE,
            msg="strong gate must emit a preflight manifest into the staging workdir",
        )
        self.assertIn(
            'ENV_SNAPSHOT="$root/runtime_env.txt"',
            WRAPPER_SOURCE,
            msg="strong gate must emit a runtime env snapshot into the staging workdir",
        )
        self.assertIn(
            'BUILD_STDOUT_LOG="$root/build.stdout.txt"',
            WRAPPER_SOURCE,
            msg="strong gate must capture build stdout in the staging workdir",
        )
        self.assertIn(
            'BUILD_STDERR_LOG="$root/build.stderr.txt"',
            WRAPPER_SOURCE,
            msg="strong gate must capture build stderr in the staging workdir",
        )
        self.assertIn(
            'CERTIFY_STDOUT_LOG="$root/certify.stdout.txt"',
            WRAPPER_SOURCE,
            msg="strong gate must capture certify stdout in the staging workdir",
        )
        self.assertIn(
            'CERTIFY_STDERR_LOG="$root/certify.stderr.txt"',
            WRAPPER_SOURCE,
            msg="strong gate must capture certify stderr in the staging workdir",
        )
        self.assertIn(
            'PRESET_SNAPSHOT_PATH="$root/selected_preset.json"',
            WRAPPER_SOURCE,
            msg="strong gate must preserve the exact selected preset inside the staging workdir",
        )
        self.assertIn(
            'SUITE_CONFIG_PATH="$root/suite_config.txt"',
            WRAPPER_SOURCE,
            msg="strong gate must publish a deterministic suite config alongside the selected preset",
        )
        self.assertIn(
            'SUITE_PLAN_PATH="$root/suite_plan.tsv"',
            WRAPPER_SOURCE,
            msg="strong gate must publish a deterministic suite plan alongside the selected preset",
        )
        self.assertIn(
            'CASE_RUN_TMP_ROOT="$root/.case_runs_tmp"',
            WRAPPER_SOURCE,
            msg="strong gate must bind case-run temp roots under the active staging workdir",
        )
        self.assertIn(
            'SHARED_CASE_CACHE_ROOT="$TMP_PARENT/case_cache"',
            WRAPPER_SOURCE,
            msg="strong gate must reserve a shared branch-local case cache root for certify generation reuse",
        )
        self.assertIn(
            'SHARED_CASE_CACHE_TMP_ROOT="$TMP_PARENT/case_cache_tmp"',
            WRAPPER_SOURCE,
            msg="strong gate must reserve a shared branch-local case cache tmp root for certify generation reuse",
        )
        self.assertIn(
            'CASE_CACHE_ROOT="$SHARED_CASE_CACHE_ROOT"',
            WRAPPER_SOURCE,
            msg="strong gate must point certify case caching at the shared branch-local cache root",
        )
        self.assertIn(
            'CASE_CACHE_TMP_ROOT="$SHARED_CASE_CACHE_TMP_ROOT"',
            WRAPPER_SOURCE,
            msg="strong gate must point certify case-cache temp writes at the shared branch-local tmp root",
        )
        self.assertIn(
            'FAILURE_SUMMARY_PATH="$root/failure_summary.txt"',
            WRAPPER_SOURCE,
            msg="strong gate must synthesize a failure summary inside the staging workdir",
        )
        self.assertIn(
            'FAILURE_REPORT_PATH="$root/latest_failure_report.md"',
            WRAPPER_SOURCE,
            msg="strong gate must synthesize a failure report inside the staging workdir",
        )
        self.assertIn(
            'write_suite_input_bundle',
            WRAPPER_SOURCE,
            msg="strong gate should materialize a deterministic suite input bundle before certify starts",
        )
        self.assertIn(
            'bind_runtime_artifacts_to_root()',
            WRAPPER_SOURCE,
            msg="strong gate must centralize staging artifact binding through a helper",
        )

    def test_preflight_validates_selected_preset_and_certify_imports(self) -> None:
        self.assertIn(
            'check_json_file_recorded "$PRESET" "selected preset json"',
            WRAPPER_SOURCE,
            msg="strong gate preflight must validate the actual selected preset JSON before certify starts",
        )
        self.assertIn(
            'check_python_entrypoint_recorded "$CERTIFY_HELPER" "branch-local certify helper imports"',
            WRAPPER_SOURCE,
            msg="strong gate preflight must validate that the branch-local certify helper imports cleanly",
        )
        self.assertIn(
            'write_runtime_environment_snapshot',
            WRAPPER_SOURCE,
            msg="strong gate preflight must capture a runtime env snapshot for reproducibility",
        )

    def test_release_env_scrubs_ambient_solver_controls_before_sourcing_release_env(self) -> None:
        self.assertIn(
            'sanitize_solver_environment',
            WRAPPER_SOURCE,
            msg="strong gate must scrub ambient solver controls before sourcing the release env",
        )
        self.assertIn(
            'unset CC CXX CPPFLAGS CFLAGS CXXFLAGS LDFLAGS SDKROOT MACOSX_DEPLOYMENT_TARGET LOCAL_SKIP_SELF_TEST',
            WRAPPER_SOURCE,
            msg="strong gate must clear ambient compiler and solver-toggle vars before loading release env",
        )
        self.assertIn(
            'ENABLE_*|PROFILE_*|DENSE_*|RUN_TAG',
            WRAPPER_SOURCE,
            msg="strong gate must scrub ambient profiling and solver feature toggles from the host shell",
        )
        self.assertIn(
            'apply_strong_gate_release_profile_overrides',
            WRAPPER_SOURCE,
            msg="strong gate must reapply its branch-local AC3 release profile after scrubbing the host env",
        )
        self.assertNotIn(
            'export ENABLE_PREV_STATE_WRITEBACK_OPT="${ENABLE_PREV_STATE_WRITEBACK_OPT:-0}"',
            WRAPPER_SOURCE,
            msg="strong gate must not pin a stale writeback override ahead of the branch-local release env",
        )

    def test_build_and_certify_failures_write_failure_reports_before_cleanup(self) -> None:
        self.assertIn(
            'if script -q /dev/null "$BUILD_WRAPPER" >"$BUILD_STDOUT_LOG" 2>"$BUILD_STDERR_LOG"; then',
            WRAPPER_SOURCE,
            msg="build failures must capture stdout/stderr without losing the real exit code to shell negation",
        )
        self.assertIn(
            'write_failure_summary "build" "$build_rc" "build wrapper failed"',
            WRAPPER_SOURCE,
            msg="build failures must write a strong-gate failure summary before cleanup",
        )
        self.assertIn(
            'write_failure_summary "certify" "$certify_rc" "certify suite failed"',
            WRAPPER_SOURCE,
            msg="certify failures must write a strong-gate failure summary before cleanup",
        )
        self.assertIn(
            'python3 "$CERTIFY_HELPER" --solver "$SOLVER_SNAPSHOT" --preset "$PRESET" --out "$WORKDIR" --limit-scale "$LIMIT_SCALE" >"$CERTIFY_STDOUT_LOG" 2>"$CERTIFY_STDERR_LOG"',
            WRAPPER_SOURCE,
            msg="certify helper stdout/stderr must be captured into the staging workdir for failure diagnosis",
        )
        self.assertIn(
            'echo "[lca_strong_gate] certify stderr after cleanup: $FAILED_ROOT/$(basename "$CERTIFY_STDERR_LOG")" >&2',
            WRAPPER_SOURCE,
            msg="certify failure hints must advertise the preserved certify stderr path after cleanup",
        )
        self.assertIn(
            'failure snapshot root after cleanup: $FAILED_ROOT',
            WRAPPER_SOURCE,
            msg="stderr failure hints must point at the final preserved latest_failure root",
        )
        self.assertIn(
            'failure report after cleanup: $FAILED_ROOT/$(basename "$FAILURE_REPORT_PATH")',
            WRAPPER_SOURCE,
            msg="failure hints must advertise the final strong-gate failure-report path after cleanup",
        )
        self.assertIn(
            'solver build metadata after cleanup: $FAILED_ROOT/$(basename "$BUILD_METADATA_SNAPSHOT")',
            WRAPPER_SOURCE,
            msg="failure hints must advertise the preserved solver build metadata path after cleanup",
        )

    def test_release_env_uses_run_local_runtime_roots(self) -> None:
        self.assertIn(
            'RUN_TMP_TEMPLATE="lca_strong_gate.env.XXXXXX"',
            WRAPPER_SOURCE,
            msg="strong gate must allocate a dedicated runtime tmp namespace per run",
        )
        self.assertIn(
            'export HOME="$RUN_TMPDIR/home"',
            WRAPPER_SOURCE,
            msg="strong gate must isolate HOME under the per-run runtime tmpdir before sourcing release env",
        )
        self.assertIn(
            'export XDG_CONFIG_HOME="$RUN_TMPDIR/xdg_config"',
            WRAPPER_SOURCE,
            msg="strong gate must isolate XDG config state under the per-run runtime tmpdir",
        )
        self.assertIn(
            'export XDG_CACHE_HOME="$RUN_TMPDIR/xdg_cache"',
            WRAPPER_SOURCE,
            msg="strong gate must isolate XDG cache state under the per-run runtime tmpdir",
        )
        self.assertIn(
            'export XDG_STATE_HOME="$RUN_TMPDIR/xdg_state"',
            WRAPPER_SOURCE,
            msg="strong gate must isolate XDG state under the per-run runtime tmpdir",
        )
        self.assertIn(
            'export PYTHONPYCACHEPREFIX="$RUN_TMPDIR/pycache"',
            WRAPPER_SOURCE,
            msg="strong gate must isolate Python bytecode under the per-run runtime tmpdir",
        )
        self.assertGreaterEqual(
            WRAPPER_SOURCE.count('export BRANCH_ARTIFACT_TMP_ROOT="$RUN_TMPDIR"'),
            2,
            msg="strong gate must rebind BRANCH_ARTIFACT_TMP_ROOT before and after sourcing release env",
        )
        self.assertIn(
            'load_release_environment',
            WRAPPER_SOURCE,
            msg="strong gate must route release env setup through a run-local rebinding helper",
        )
        self.assertIn(
            'assert_runtime_environment',
            WRAPPER_SOURCE,
            msg="strong gate must validate that release env stayed inside the run-local runtime roots",
        )
        self.assertIn(
            'ensure_under_artifacts "$RUN_TMPDIR"',
            WRAPPER_SOURCE,
            msg="strong gate must keep the runtime tmpdir itself under branch-local artifacts",
        )
        self.assertIn(
            '[[ "$TMPDIR" == "$RUN_TMPDIR" ]] || fail "TMPDIR drifted from strong gate runtime tmpdir"',
            WRAPPER_SOURCE,
            msg="strong gate must reject TMPDIR drift outside the run-local tmp root",
        )
        self.assertIn(
            '[[ "$TMP" == "$RUN_TMPDIR" ]] || fail "TMP drifted from strong gate runtime tmpdir"',
            WRAPPER_SOURCE,
            msg="strong gate must reject TMP drift outside the run-local tmp root",
        )
        self.assertIn(
            '[[ "$TEMP" == "$RUN_TMPDIR" ]] || fail "TEMP drifted from strong gate runtime tmpdir"',
            WRAPPER_SOURCE,
            msg="strong gate must reject TEMP drift outside the run-local tmp root",
        )
        self.assertIn(
            '[[ "$XDG_STATE_HOME" == "$expected_state" ]] || fail "XDG_STATE_HOME drifted from strong gate runtime state root"',
            WRAPPER_SOURCE,
            msg="strong gate must reject XDG state drift outside the run-local runtime roots",
        )

    def test_preflight_recording_rebinds_runtime_artifacts_to_live_workdir(self) -> None:
        self.assertIn(
            'WORKDIR="$(mktemp -d "$OUTPARENT/$RUN_WORK_TEMPLATE")"',
            WRAPPER_SOURCE,
            msg="strong gate should stage runs under the stable artifact parent so certify output survives .tmp volatility",
        )
        self.assertIn(
            'ensure_workdir_runtime_artifacts_bound',
            WRAPPER_SOURCE,
            msg="strong gate should expose a helper that rebinds runtime artifact paths to the live staging workdir",
        )
        self.assertIn(
            'bind_runtime_artifacts_to_root "$WORKDIR"',
            WRAPPER_SOURCE,
            msg="strong gate should recover stale runtime artifact paths by rebinding them to the current workdir",
        )
        self.assertIn(
            'if [[ ! -f "$PRECHECK_MANIFEST" ]]; then',
            WRAPPER_SOURCE,
            msg="strong gate should recreate the preflight manifest header when a stale path drift removed the old file",
        )
        self.assertIn(
            'mkdir -p "$WORKDIR"',
            WRAPPER_SOURCE,
            msg="strong gate should recreate a vanished staging workdir before rebinding runtime artifact paths",
        )
        self.assertIn(
            'mkdir -p "$(dirname "$PRECHECK_MANIFEST")"',
            WRAPPER_SOURCE,
            msg="record_preflight_check should recreate the manifest parent before appending rows",
        )
        self.assertIn(
            'ensure_workdir_runtime_artifacts_bound',
            WRAPPER_SOURCE,
            msg="record_preflight_check should heal stale manifest bindings before appending diagnostic rows",
        )
        self.assertIn(
            'printf \'%s\\t%s\\t%s\\t%s\\n\' "$kind" "$label" "$status" "$value" >> "$PRECHECK_MANIFEST"',
            WRAPPER_SOURCE,
            msg="record_preflight_check should still append the normalized preflight rows after the rebinding guard runs",
        )
        self.assertIn(
            'write_runtime_environment_snapshot() {\n  ensure_workdir_runtime_artifacts_bound',
            WRAPPER_SOURCE,
            msg="runtime environment snapshots should use the same live-workdir rebinding guard as preflight recording",
        )

    def test_clear_stale_state_and_cleanup_cover_runtime_tmp_roots(self) -> None:
        self.assertIn(
            '"$TMP_PARENT"/lca_strong_gate.env.*',
            WRAPPER_SOURCE,
            msg="strong gate stale-state cleanup must remove abandoned runtime tmp roots",
        )
        self.assertIn(
            'if [[ -n "${RUN_TMPDIR:-}" && -e "$RUN_TMPDIR" ]]; then',
            WRAPPER_SOURCE,
            msg="strong gate cleanup must own the run-local runtime tmpdir teardown",
        )
        self.assertIn(
            'remove_path_retry "$RUN_TMPDIR"',
            WRAPPER_SOURCE,
            msg="strong gate cleanup must remove the run-local runtime tmpdir on exit",
        )
        self.assertIn(
            'rmdir "$TMP_PARENT" 2>/dev/null || true',
            WRAPPER_SOURCE,
            msg="strong gate cleanup must collapse the branch-local tmp parent when it becomes empty",
        )
        self.assertIn(
            'release_lock',
            WRAPPER_SOURCE,
            msg="strong gate cleanup must release the branch-local lock before exiting",
        )

    def test_heartbeat_counts_published_cases_alongside_live_case_tmpdirs(self) -> None:
        self.assertIn(
            'local published_count=0',
            WRAPPER_SOURCE,
            msg="strong gate heartbeat must track already-published cases in the staging workdir",
        )
        self.assertIn(
            'local active_count=0',
            WRAPPER_SOURCE,
            msg="strong gate heartbeat must track the currently active tmp case separately",
        )
        self.assertIn(
            'find "$WORKDIR/runs" -type f \\( -name \'run_case_result.json\' -o -name \'time.txt\' \\) 2>/dev/null |',
            WRAPPER_SOURCE,
            msg="strong gate heartbeat must count published case result markers, not only successful time.txt files",
        )
        self.assertIn(
            'find "$CASE_RUN_TMP_ROOT" -type f \\( -name \'run_case_result.json\' -o -name \'time.txt\' \\) -newer "$LOCK_PID_FILE" 2>/dev/null |',
            WRAPPER_SOURCE,
            msg="strong gate heartbeat must treat timed-out tmp cases as completed once they publish a result marker",
        )
        self.assertIn(
            "sed 's#/[^/]*$##' |",
            WRAPPER_SOURCE,
            msg="strong gate heartbeat must deduplicate per-case result markers so PASS cases with both files count once",
        )
        self.assertIn(
            'printf \'%s\\n\' "$(( published_count + active_count ))"',
            WRAPPER_SOURCE,
            msg="strong gate heartbeat must report total completed cases instead of hiding published progress",
        )
        self.assertIn(
            'BRANCH_CERTIFY_CASE_RUN_TMP_ROOT="$CASE_RUN_TMP_ROOT"',
            WRAPPER_SOURCE,
            msg="strong gate must tell the certify helper to keep run tmp cases under the staging workdir",
        )
        self.assertIn(
            'BRANCH_CERTIFY_CASE_CACHE_ROOT="$CASE_CACHE_ROOT"',
            WRAPPER_SOURCE,
            msg="strong gate must tell the certify helper to reuse the shared branch-local case cache root",
        )
        self.assertIn(
            'BRANCH_CERTIFY_CASE_CACHE_TMP_ROOT="$CASE_CACHE_TMP_ROOT"',
            WRAPPER_SOURCE,
            msg="strong gate must tell the certify helper to reuse the shared branch-local case-cache tmp root",
        )

    def test_wrapper_executes_successfully_when_certify_reports_pass_and_scrubs_ambient_solver_controls(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh"
            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={
                    **os.environ,
                    "CXX": "/usr/bin/false",
                    "ENABLE_STATE_LOAD_MATERIALIZATION_OPT": "1",
                    "LCA_HEARTBEAT_INTERVAL": "0.01",
                    "LCA_STAGE_FILTER": "correctness_fuzz",
                },
                check=False,
            )

            self.assertEqual(
                result.returncode,
                0,
                msg=f"wrapper should exit 0 after a branch-local certify PASS, stderr was:\n{result.stderr}",
            )
            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate"
            self.assertTrue((outroot / "certify.json").exists(), msg="successful strong-gate runs must publish certify.json")
            self.assertTrue((outroot / "certify_summary.md").exists(), msg="successful strong-gate runs must publish certify_summary.md")
            self.assertTrue((outroot / "certify_rows.csv").exists(), msg="successful strong-gate runs must publish certify_rows.csv")
            self.assertTrue((outroot / "preflight_manifest.tsv").exists(), msg="successful strong-gate runs must publish the preflight manifest")
            self.assertTrue((outroot / "runtime_env.txt").exists(), msg="successful strong-gate runs must publish the runtime env snapshot")
            self.assertTrue((outroot / "selected_preset.json").exists(), msg="successful strong-gate runs must publish the selected preset snapshot")
            self.assertTrue((outroot / "suite_config.txt").exists(), msg="successful strong-gate runs must publish the deterministic suite config")
            self.assertTrue((outroot / "suite_plan.tsv").exists(), msg="successful strong-gate runs must publish the deterministic suite plan")
            self.assertTrue((outroot / "repeatability_gate_manifest.txt").exists(), msg="successful strong-gate runs must publish the repeatability manifest")
            self.assertTrue((outroot / "build.stdout.txt").exists(), msg="successful strong-gate runs must publish build stdout")
            self.assertTrue((outroot / "build.stderr.txt").exists(), msg="successful strong-gate runs must publish build stderr")
            self.assertTrue((outroot / "solver_build_meta.json").exists(), msg="successful strong-gate runs must publish the build metadata snapshot")
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate.latest_failure").exists(),
                msg="successful strong-gate runs must not preserve a latest_failure bundle",
            )

            runtime_env = (outroot / "runtime_env.txt").read_text(encoding="utf-8")
            self.assertIn("stage_filter=correctness_fuzz", runtime_env)
            self.assertIn("cxx=", runtime_env)
            self.assertIn("enable_state_load_materialization_opt=1", runtime_env)
            self.assertIn("strong_gate_release_profile=progress40_defaults+ac3_state_materialization", runtime_env)
            self.assertIn(
                f"case_cache_root={branch_root / 'artifacts' / 'lca_tree_stress_v5' / '.tmp' / 'case_cache'}",
                runtime_env,
            )
            self.assertIn(
                f"case_cache_tmp_root={branch_root / 'artifacts' / 'lca_tree_stress_v5' / '.tmp' / 'case_cache_tmp'}",
                runtime_env,
            )

            selected_preset = (outroot / "selected_preset.json").read_text(encoding="utf-8")
            self.assertIn('"name": "correctness_fuzz"', selected_preset)
            self.assertNotIn('"name": "hard_scaling"', selected_preset)

            suite_config = (outroot / "suite_config.txt").read_text(encoding="utf-8")
            self.assertIn("config_schema=lca_strong_gate_suite_config_v1", suite_config)
            self.assertIn("stage_filter=correctness_fuzz", suite_config)
            self.assertIn("case_path_policy=runs/{stage}/{mode}/n{n}/seed{seed}_L{shuffle_labels}_Q{shuffle_queries}", suite_config)
            self.assertIn("solver_env_contract=DENSE_PROFILE_OUTDIR=work_case_dir;", suite_config)

            suite_plan = (outroot / "suite_plan.tsv").read_text(encoding="utf-8")
            self.assertIn("case_artifact_subpath", suite_plan)
            self.assertIn("correctness_fuzz\tstub_mode\t1\t1\t1\t1", suite_plan)
            self.assertNotIn("hard_scaling", suite_plan)

            repeatability_manifest = (outroot / "repeatability_gate_manifest.txt").read_text(encoding="utf-8")
            self.assertIn("suite_config_sha256=", repeatability_manifest)
            self.assertIn("suite_plan_sha256=", repeatability_manifest)
            self.assertIn("suite_plan_case_count=1", repeatability_manifest)

    def test_wrapper_recovers_if_build_removes_the_staging_workdir(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh"
            self.write_text(
                branch_root / "build.sh",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                    OUT="$SCRIPT_DIR/artifacts/boj28350_resume/build/solve"
                    META="${OUT}.build_meta.json"
                    mkdir -p "$(dirname "$OUT")"
                    printf '#!/usr/bin/env bash\\nexit 0\\n' > "$OUT"
                    chmod +x "$OUT"
                    cat > "$META" <<'EOF'
                    {
                      "schema": "boj28350_build_metadata_v1",
                      "source": "boj28350_resume/boj28350_branch_3_solver.cpp",
                      "output": "artifacts/boj28350_resume/build/solve",
                      "compiler": "stub",
                      "command": ["stub"],
                      "requested_compiler": "",
                      "cxx_env": "",
                      "static_mode": "auto",
                      "defines": []
                    }
                    EOF
                    rm -rf "$SCRIPT_DIR"/artifacts/lca_tree_stress_v5/lca_strong_gate.run.*
                    printf '[build] output=%s\\n' "$OUT"
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "build.sh")
            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={
                    **os.environ,
                    "LCA_HEARTBEAT_INTERVAL": "0.01",
                },
                check=False,
            )

            self.assertEqual(
                result.returncode,
                0,
                msg=f"wrapper should recover after the build removes the staging workdir, stderr was:\n{result.stderr}",
            )
            self.assertIn(
                "rebuilding staging artifacts after build removed",
                result.stderr,
                msg="wrapper should log when it rebuilds the staging artifact tree after an external cleanup",
            )
            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate"
            self.assertTrue((outroot / "certify.json").exists(), msg="recovered runs must still publish certify.json")
            self.assertTrue((outroot / "runtime_env.txt").exists(), msg="recovered runs must still publish runtime_env.txt")
            self.assertTrue((outroot / "solver_build_meta.json").exists(), msg="recovered runs must still snapshot build metadata")
            self.assertTrue((outroot / "suite_config.txt").exists(), msg="recovered runs must still republish the suite config")
            self.assertTrue((outroot / "suite_plan.tsv").exists(), msg="recovered runs must still republish the suite plan")
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate.latest_failure").exists(),
                msg="successful recovery must not leave a latest_failure bundle behind",
            )

    def test_wrapper_recovery_re_materializes_dataless_preset_after_build_clears_run_tmp(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh"
            fake_bin = branch_root / "fake_bin"
            fake_bin.mkdir()
            self.write_text(
                fake_bin / "stat",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    if [[ "$#" -ge 3 && "$1" == "-f" && "$2" == "%Sf" && "$3" == *"/suite_presets/strong_gate.json" ]]; then
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
                branch_root / "build.sh",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                    OUT="$SCRIPT_DIR/artifacts/boj28350_resume/build/solve"
                    META="${OUT}.build_meta.json"
                    mkdir -p "$(dirname "$OUT")"
                    printf '#!/usr/bin/env bash\\nexit 0\\n' > "$OUT"
                    chmod +x "$OUT"
                    cat > "$META" <<'EOF'
                    {
                      "schema": "boj28350_build_metadata_v1",
                      "source": "boj28350_resume/boj28350_branch_3_solver.cpp",
                      "output": "artifacts/boj28350_resume/build/solve",
                      "compiler": "stub",
                      "command": ["stub"],
                      "requested_compiler": "",
                      "cxx_env": "",
                      "static_mode": "auto",
                      "defines": []
                    }
                    EOF
                    rm -rf "$SCRIPT_DIR"/artifacts/lca_tree_stress_v5/lca_strong_gate.run.*
                    rm -rf "$SCRIPT_DIR"/artifacts/lca_tree_stress_v5/.tmp/lca_strong_gate.env.*
                    printf '[build] output=%s\\n' "$OUT"
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "build.sh")

            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={
                    **os.environ,
                    "PATH": f"{fake_bin}{os.pathsep}{os.environ['PATH']}",
                    "LCA_HEARTBEAT_INTERVAL": "0.01",
                    "LCA_STAGE_FILTER": "correctness_fuzz",
                },
                check=False,
            )

            self.assertEqual(
                result.returncode,
                0,
                msg=f"wrapper should recover after build clears both the staging workdir and run-local preset materialization, stderr was:\n{result.stderr}",
            )
            self.assertIn(
                "rebuilding staging artifacts after build removed",
                result.stderr,
                msg="wrapper should log recovery when build clears the staging roots",
            )
            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate"
            self.assertTrue((outroot / "certify.json").exists(), msg="recovered runs must still publish certify.json")
            self.assertTrue((outroot / "runtime_env.txt").exists(), msg="recovered runs must still publish runtime_env.txt")
            self.assertTrue((outroot / "selected_preset.json").exists(), msg="recovered runs must still publish the selected preset snapshot")
            preflight_manifest = (outroot / "preflight_manifest.tsv").read_text(encoding="utf-8")
            self.assertIn(
                "materialization\tselected preset source\trecovered\t",
                preflight_manifest,
                msg="recovered runs must re-materialize the dataless preset after build-side tmp cleanup",
            )
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate.latest_failure").exists(),
                msg="successful recovery must not leave a latest_failure bundle behind",
            )

    def test_wrapper_republishes_cleanly_across_two_successful_invocations_without_stale_solver_snapshots(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh"
            env = {**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"}
            snapshot_parent = branch_root / "artifacts" / "lca_tree_stress_v5" / ".solver_snapshots"
            snapshot_root = snapshot_parent / "lca_strong_gate"
            snapshot_root.mkdir(parents=True, exist_ok=True)
            (snapshot_root / "lca_strong_gate.solver.stale").write_text("stale\n", encoding="utf-8")
            (snapshot_parent / "lca_strong_gate.solver.legacy").write_text("legacy\n", encoding="utf-8")

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

            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate"
            self.assertTrue((outroot / "certify.json").exists(), msg="reruns must republish certify.json at the fixed strong-gate outroot")
            self.assertTrue((outroot / "certify_summary.md").exists(), msg="reruns must republish certify_summary.md at the fixed strong-gate outroot")
            self.assertTrue((outroot / "runtime_env.txt").exists(), msg="reruns must republish the current-run runtime env snapshot")
            self.assertTrue((outroot / "solver_build_meta.json").exists(), msg="reruns must republish the current-run build metadata snapshot")
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate.previous").exists(),
                msg="successful reruns must not require a lingering manual-cleanup backup tree",
            )
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate.latest_failure").exists(),
                msg="successful reruns must not leave a stale latest_failure bundle behind",
            )
            self.assertFalse(
                snapshot_root.exists(),
                msg="successful reruns must clean the gate-specific solver snapshot root instead of leaving stale snapshot files behind",
            )
            self.assertFalse(
                (snapshot_parent / "lca_strong_gate.solver.legacy").exists(),
                msg="successful reruns must scrub legacy misplaced solver snapshots without manual cleanup",
            )

    def test_repeated_failed_reruns_archive_the_prior_failure_bundle(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh"
            self.write_text(
                branch_root / "branch_certify_suite.py",
                textwrap.dedent(
                    """
                    #!/usr/bin/env python3
                    from __future__ import annotations

                    import argparse
                    import sys
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
                        print("stub certify stdout", flush=True)
                        print("stub certify stderr", file=sys.stderr, flush=True)
                        return 9


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
                / "strong_gate"
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
            self.assertEqual(first.returncode, 9, msg=first.stderr)
            self.assertTrue((failed_root / "failure_summary.txt").exists())
            self.assertTrue((failed_root / "certify.stdout.txt").exists())
            self.assertTrue((failed_root / "certify.stderr.txt").exists())
            self.assertIn("stub certify stdout", (failed_root / "certify.stdout.txt").read_text(encoding="utf-8"))
            self.assertIn("stub certify stderr", (failed_root / "certify.stderr.txt").read_text(encoding="utf-8"))

            second = subprocess.run(
                [str(wrapper_path), run_tag],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={**os.environ, "LCA_HEARTBEAT_INTERVAL": "0.01"},
                check=False,
            )
            self.assertEqual(second.returncode, 9, msg=second.stderr)
            self.assertTrue((failed_root / "failure_summary.txt").exists())
            self.assertTrue((failed_root / "certify.stdout.txt").exists())
            self.assertTrue((failed_root / "certify.stderr.txt").exists())
            archived_dirs = sorted(archive_root.glob("subac2_failed_rerun.latest_failure*"))
            self.assertEqual(len(archived_dirs), 1, msg=f"unexpected archives: {archived_dirs}")
            self.assertTrue((archived_dirs[0] / "failure_summary.txt").exists())
            self.assertTrue((archived_dirs[0] / "certify.stdout.txt").exists())
            self.assertTrue((archived_dirs[0] / "certify.stderr.txt").exists())
            self.assertIn("archived previous failure snapshot", second.stderr)

    def test_dataless_selected_preset_is_materialized_into_run_local_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh"
            fake_bin = branch_root / "fake_bin"
            fake_bin.mkdir()
            self.write_text(
                fake_bin / "stat",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    if [[ "$#" -ge 3 && "$1" == "-f" && "$2" == "%Sf" && "$3" == *"/suite_presets/strong_gate.json" ]]; then
                      printf 'compressed,dataless\\n'
                      exit 0
                    fi

                    exec /usr/bin/stat "$@"
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(fake_bin / "stat")

            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={
                    **os.environ,
                    "PATH": f"{fake_bin}{os.pathsep}{os.environ['PATH']}",
                    "LCA_HEARTBEAT_INTERVAL": "0.01",
                    "LCA_STAGE_FILTER": "correctness_fuzz",
                },
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)

            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate"
            self.assertTrue((outroot / "certify.json").exists(), msg="recoverable dataless presets should no longer block the strong gate")
            self.assertTrue((outroot / "selected_preset.json").exists(), msg="successful runs must still publish the selected preset snapshot")

            runtime_env = (outroot / "runtime_env.txt").read_text(encoding="utf-8")
            self.assertIn("selected_preset_source=", runtime_env)
            self.assertIn("selected_preset_source_materialized=", runtime_env)

            preflight_manifest = (outroot / "preflight_manifest.tsv").read_text(encoding="utf-8")
            self.assertIn(
                "materialization\tselected preset source\trecovered\t",
                preflight_manifest,
                msg="the preserved preflight manifest must record when a dataless preset source was recovered into a run-local copy",
            )

    def test_dataless_live_presets_fall_back_to_branch_local_preset_cache(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh"
            fake_bin = branch_root / "fake_bin"
            fake_bin.mkdir()
            self.write_text(
                fake_bin / "stat",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    if [[ "$#" -ge 3 && "$1" == "-f" && "$2" == "%Sf" && "$3" == *"/suite_presets/strong_gate.json" ]]; then
                      printf 'compressed,dataless\\n'
                      exit 0
                    fi

                    exec /usr/bin/stat "$@"
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(fake_bin / "stat")

            preset_cache = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / ".preset_cache" / "lca_strong_gate.json"
            )
            self.write_text(
                preset_cache,
                (branch_root / "suite_presets" / "strong_gate.json").read_text(encoding="utf-8"),
            )

            result = subprocess.run(
                [str(wrapper_path)],
                cwd=branch_root,
                capture_output=True,
                text=True,
                env={
                    **os.environ,
                    "PATH": f"{fake_bin}{os.pathsep}{os.environ['PATH']}",
                    "LCA_HEARTBEAT_INTERVAL": "0.01",
                    "LCA_STAGE_FILTER": "correctness_fuzz",
                },
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)

            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate"
            self.assertTrue((outroot / "certify.json").exists(), msg="cached preset fallback should keep the strong gate runnable")
            runtime_env = (outroot / "runtime_env.txt").read_text(encoding="utf-8")
            self.assertIn(f"preset_cache_path={preset_cache}", runtime_env)
            self.assertIn(f"selected_preset_source={preset_cache}", runtime_env)
            preflight_manifest = (outroot / "preflight_manifest.tsv").read_text(encoding="utf-8")
            self.assertIn(
                f"path\tpreset_cache_path\tok\t{preset_cache}",
                preflight_manifest,
                msg="preflight should record the branch-local preset cache path used for reproducibility",
            )

    def test_artifact_preset_fallback_rejects_newer_stage_filtered_snapshot(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh"
            (branch_root / "suite_presets" / "strong_gate.json").unlink()

            full_snapshot_root = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "strong_gate.failure_archive"
                / "full_snapshot"
            )
            filtered_snapshot_root = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "strong_gate.failure_archive"
                / "filtered_snapshot"
            )

            full_preset = {
                "name": "strong_gate",
                "stages": [
                    {
                        "name": "correctness_fuzz",
                        "modes": ["stub_mode"],
                        "sizes": [1],
                        "seeds": [1],
                        "shuffle_labels": [1],
                        "shuffle_queries": [1],
                        "must_pass": True,
                    },
                    {
                        "name": "hard_scaling",
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
                "name": "strong_gate",
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
            self.write_text(filtered_snapshot_root / "runtime_env.txt", "stage_filter=correctness_fuzz\n")
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
                        if stage_names != ["correctness_fuzz", "hard_scaling"]:
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
                                            "name": "correctness_fuzz",
                                            "status": "PASS",
                                            "cases": 1,
                                            "timeouts": 0,
                                            "re_wa": 0,
                                            "limit_scale": 1.0,
                                            "scale_fail": [],
                                        },
                                        {
                                            "name": "hard_scaling",
                                            "status": "PASS",
                                            "cases": 1,
                                            "timeouts": 0,
                                            "re_wa": 0,
                                            "limit_scale": 1.0,
                                            "scale_fail": [],
                                        },
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

            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate"
            received_preset_path = (outroot / "received_preset_path.txt").read_text(encoding="utf-8").strip()
            self.assertEqual(
                received_preset_path,
                str(full_snapshot_root / "selected_preset.json"),
                msg="artifact fallback must reject newer stage-filtered preset snapshots and reuse only a full-gate snapshot",
            )

    def test_failed_rerun_archives_incomplete_previous_published_output(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_strong_gate.sh"
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
                        return 9


                    if __name__ == "__main__":
                        raise SystemExit(main())
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "branch_certify_suite.py")

            run_tag = "retry_loop/subac4_incomplete_previous"
            outroot = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate" / "retry_loop" / "subac4_incomplete_previous"
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

            self.assertEqual(result.returncode, 9, msg=result.stderr)
            self.assertFalse(
                outroot.exists(),
                msg="failed reruns must not keep an incomplete legacy published output tree at the live outroot",
            )
            archived_dirs = sorted(archive_root.glob("subac4_incomplete_previous.incomplete_published.*"))
            self.assertEqual(len(archived_dirs), 1, msg=f"unexpected archives: {archived_dirs}")
            self.assertTrue((archived_dirs[0] / "certify.json").exists())
            self.assertTrue((failed_root / "failure_summary.txt").exists())
            self.assertIn("archived incomplete published output", result.stderr)


if __name__ == "__main__":
    unittest.main()
