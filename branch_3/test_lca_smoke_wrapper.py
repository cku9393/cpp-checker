#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import re
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


WRAPPER_PATH = Path(__file__).resolve().parent / "outer_suite_wrappers" / "lca_smoke.sh"
WRAPPER_SOURCE = WRAPPER_PATH.read_text(encoding="utf-8")
HOST_TMP_ROOT = Path("/tmp/cpp-checker-branch3-smoke-tests/wrapper")
HOST_TMP_ROOT.mkdir(parents=True, exist_ok=True)
# Keep host-side fake-branch creation independent from any inherited retry-loop
# TMPDIR so wrapper regressions are about wrapper behavior, not outer temp state.
tempfile.tempdir = str(HOST_TMP_ROOT)


class LcaSmokeWrapperRegressionTests(unittest.TestCase):
    def write_text(self, path: Path, content: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def make_executable(self, path: Path) -> None:
        path.chmod(path.stat().st_mode | 0o111)

    def symlink_file(self, target: Path, link_path: Path) -> None:
        link_path.parent.mkdir(parents=True, exist_ok=True)
        link_path.symlink_to(target)

    def make_fake_runtime_branch(self, temp_root: Path, *, run_case_body: str) -> Path:
        branch_root = temp_root / "branch"
        self.symlink_file(WRAPPER_PATH, branch_root / "outer_suite_wrappers" / "lca_smoke.sh")
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


                def ensure_path(raw: str) -> str:
                    path = Path(raw)
                    if not path.is_absolute():
                        path = (BRANCH_ROOT / path).resolve()
                    else:
                        path = path.resolve()
                    return str(path)


                def main() -> int:
                    parser = argparse.ArgumentParser()
                    parser.add_argument("--ensure")
                    parser.add_argument("--artifacts-root", action="store_true")
                    parser.add_argument("key", nargs="?")
                    parser.add_argument("subpath", nargs="?")
                    args = parser.parse_args()

                    if args.ensure is not None:
                        print(ensure_path(args.ensure))
                        return 0
                    if args.artifacts_root:
                        print(ARTIFACTS_ROOT)
                        return 0
                    if args.key == "lca_smoke":
                        print((ARTIFACTS_ROOT / "lca_tree_stress_v5" / "smoke").resolve())
                        return 0
                    if args.key == "lca_smoke_target":
                        subpath = args.subpath or "default"
                        print((ARTIFACTS_ROOT / "lca_tree_stress_v5" / "smoke_target" / subpath).resolve())
                        return 0
                    parser.error("unsupported artifact_paths invocation")


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
                out=""
                while (($#)); do
                  case "$1" in
                    --out)
                      out="$2"
                      shift 2
                      ;;
                    *)
                      shift
                      ;;
                  esac
                done
                mkdir -p "$(dirname "$out")"
                cat >"$out" <<'EOF'
                #!/usr/bin/env bash
                exit 0
                EOF
                chmod +x "$out"
                """
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "solver_release_env.sh",
            textwrap.dedent(
                """
                #!/usr/bin/env bash
                export LOCAL_SKIP_SELF_TEST="${LOCAL_SKIP_SELF_TEST:-1}"
                """
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "lca_smoke_target.sh",
            textwrap.dedent(
                """
                #!/usr/bin/env bash
                set -euo pipefail
                exit 0
                """
            ).strip()
            + "\n",
        )
        self.write_text(branch_root / "build.py", "from __future__ import annotations\n")
        self.write_text(branch_root / "branch_validator.py", "from __future__ import annotations\n")
        self.write_text(branch_root / "boj28350_resume.py", "from __future__ import annotations\n")
        self.write_text(
            branch_root / "branch_run_case.py",
            textwrap.dedent(run_case_body).strip() + "\n",
        )
        self.write_text(
            branch_root / "boj28350_resume" / "boj28350_branch_3_solver.cpp",
            "int main() { return 0; }\n",
        )
        self.write_text(
            branch_root / "boj28350_resume" / "smoke_cases.tsv",
            (
                "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                "smoke\tcomb_core\t8\t1\t0\t0\t1\n"
            ),
        )
        for rel_path in (
            Path("outer_suite_wrappers/lca_smoke.sh"),
            Path("build.sh"),
            Path("solver_release_env.sh"),
            Path("lca_smoke_target.sh"),
            Path("branch_run_case.py"),
        ):
            self.make_executable(branch_root / rel_path)
        return branch_root

    def assert_helper_success_branch_returns_zero(self, helper_name: str, status_name: str) -> None:
        pattern = rf"""
            {re.escape(helper_name)}\(\)\s*\{{.*?
            record_setup_check\s+"{re.escape(status_name)}"\s+"\$label"\s+"\$path"\s*
            return\s+0
        """
        self.assertRegex(
            WRAPPER_SOURCE,
            re.compile(pattern, re.DOTALL | re.VERBOSE),
            msg=f"{helper_name} must return success after recording {status_name}",
        )

    def test_file_preflight_helper_returns_success_after_recording_present_file(self) -> None:
        self.assert_helper_success_branch_returns_zero("check_required_file_recorded", "file")

    def test_executable_preflight_helper_returns_success_after_recording_present_executable(self) -> None:
        self.assert_helper_success_branch_returns_zero("check_required_executable_recorded", "executable")

    def test_preserve_failure_artifacts_falls_back_to_copy_when_move_fails(self) -> None:
        self.assertIn(
            'if ! move_path_retry "$WORKDIR" "$FAILURE_ROOT"; then',
            WRAPPER_SOURCE,
            msg="preserve_failure_artifacts must first try to move the staging tree into FAILURE_ROOT",
        )
        self.assertIn(
            'failure_parent="$(dirname "$FAILURE_ROOT")"',
            WRAPPER_SOURCE,
            msg="preserve_failure_artifacts must derive a stable parent for the failure root",
        )
        self.assertIn(
            'if ! mkdir -p "$FAILURE_ROOT"; then',
            WRAPPER_SOURCE,
            msg="preserve_failure_artifacts must recreate FAILURE_ROOT before fallback copies",
        )
        self.assertIn(
            'if ! cp -R "$WORKDIR"/. "$FAILURE_ROOT"/; then',
            WRAPPER_SOURCE,
            msg="preserve_failure_artifacts must snapshot WORKDIR into FAILURE_ROOT even when rename fails",
        )

    def test_exit_cleanup_skips_shared_success_roots_after_explicit_success_cleanup(self) -> None:
        self.assertIn(
            "SHARED_SUCCESS_STATE_CLEANED=0",
            WRAPPER_SOURCE,
            msg="cleanup must track whether shared success roots were already cleaned",
        )
        self.assertIn(
            "local skip_success_teardown=0",
            WRAPPER_SOURCE,
            msg="cleanup must explicitly track whether success teardown should be skipped",
        )
        self.assertIn(
            'if (( rc == 0 )) && (( SHARED_SUCCESS_STATE_CLEANED == 1 )); then',
            WRAPPER_SOURCE,
            msg="cleanup must detect when explicit success cleanup already ran",
        )
        self.assertIn(
            'if (( skip_success_teardown == 0 )) && (( rc == 0 )) && (( SHARED_SUCCESS_STATE_CLEANED == 0 )); then',
            WRAPPER_SOURCE,
            msg="cleanup must still own shared success-root cleanup on the implicit success path",
        )
        self.assertIn(
            'if (( skip_success_teardown == 0 )); then',
            WRAPPER_SOURCE,
            msg="cleanup must gate the volatile tmpdir teardown on the success-cleanup state",
        )
        self.assertIn(
            'release_lock || true',
            WRAPPER_SOURCE,
            msg="cleanup must still release the smoke lock after teardown decisions",
        )

    def test_success_cleanup_marks_shared_state_as_already_cleaned(self) -> None:
        pattern = rf"""
            cleanup_success_state\(\)\s*\{{.*?
            remove_path_retry\s+"\$SETUP_ROOT".*?
            remove_path_retry\s+"\$SESSION_STATE_ROOT".*?
            SHARED_SUCCESS_STATE_CLEANED=1
        """
        self.assertRegex(
            WRAPPER_SOURCE,
            re.compile(pattern, re.DOTALL | re.VERBOSE),
            msg="success cleanup must mark shared setup/session state as already cleaned before lock release",
        )

    def test_deterministic_controls_allow_branch_local_build_timeout_override(self) -> None:
        self.assertIn(
            '${LCA_SMOKE_BUILD_TIMEOUT_S:-$SMOKE_BUILD_TIMEOUT_S}',
            WRAPPER_SOURCE,
            msg="deterministic smoke controls must honor the branch-local build timeout override",
        )
        self.assertIn(
            '"LCA_SMOKE_BUILD_TIMEOUT_S"',
            WRAPPER_SOURCE,
            msg="deterministic smoke controls must validate the build timeout override by name",
        )

    def test_clean_env_bootstrap_preserves_explicit_debug_manifest_override(self) -> None:
        self.assertIn(
            '"TMPDIR=/tmp"',
            WRAPPER_SOURCE,
            msg="smoke clean-env bootstrap must reset TMPDIR before setup tmpdirs are configured",
        )
        self.assertIn(
            '"TMP=/tmp"',
            WRAPPER_SOURCE,
            msg="smoke clean-env bootstrap must reset TMP before setup tmpdirs are configured",
        )
        self.assertIn(
            '"TEMP=/tmp"',
            WRAPPER_SOURCE,
            msg="smoke clean-env bootstrap must reset TEMP before setup tmpdirs are configured",
        )
        self.assertIn(
            'if [[ -n "${LCA_SMOKE_DEBUG_MANIFEST:-}" ]]; then',
            WRAPPER_SOURCE,
            msg="smoke clean-env bootstrap must preserve an explicit debug manifest override",
        )
        self.assertIn(
            'clean_env_args+=("LCA_SMOKE_DEBUG_MANIFEST=$LCA_SMOKE_DEBUG_MANIFEST")',
            WRAPPER_SOURCE,
            msg="smoke clean-env bootstrap must carry the debug manifest override across re-exec",
        )

    def test_suite_metadata_records_manifest_input_policy_for_debug_replays(self) -> None:
        self.assertIn(
            'echo "manifest_input_policy=$SMOKE_MANIFEST_INPUT_POLICY"',
            WRAPPER_SOURCE,
            msg="suite metadata must record whether the smoke loop came from the default or debug manifest",
        )
        self.assertIn(
            'record_setup_check "policy" "smoke_manifest_input_policy" "$SMOKE_MANIFEST_INPUT_POLICY"',
            WRAPPER_SOURCE,
            msg="setup preflight must publish the manifest-input policy for later rerun audits",
        )

    def test_deterministic_controls_pin_single_retry_budgets_for_build_and_case_transients(self) -> None:
        self.assertIn(
            'SMOKE_CASE_RETRY_LIMIT=1',
            WRAPPER_SOURCE,
            msg="smoke must keep one deterministic retry available for harness-transient case failures",
        )
        self.assertIn(
            'SMOKE_RETRY_SLEEP_S="0.05"',
            WRAPPER_SOURCE,
            msg="smoke must keep a fixed retry backoff for harness-transient case failures",
        )
        self.assertIn(
            'SMOKE_BUILD_RETRY_LIMIT=1',
            WRAPPER_SOURCE,
            msg="smoke must keep one deterministic retry available for transient build-wrapper failures",
        )
        self.assertIn(
            'SMOKE_BUILD_RETRY_SLEEP_S="0.05"',
            WRAPPER_SOURCE,
            msg="smoke must keep a fixed retry backoff for transient build-wrapper failures",
        )
        self.assertIn(
            'SMOKE_RETRY_POLICY="harness_transient_only"',
            WRAPPER_SOURCE,
            msg="smoke must describe the case retry policy as harness-transient only",
        )
        self.assertIn(
            'SMOKE_BUILD_RETRY_POLICY="timeout_or_unexpected_exit_once"',
            WRAPPER_SOURCE,
            msg="smoke must describe the build retry policy as one retry for timeouts or unexpected exits",
        )
        self.assertIn(
            'if (( SMOKE_CASE_RETRY_LIMIT != 1 )); then',
            WRAPPER_SOURCE,
            msg="deterministic controls must reject drifting case retry budgets",
        )
        self.assertIn(
            'fail "LCA_SMOKE_CASE_RETRY_LIMIT must stay fixed at 1 for deterministic smoke control flow"',
            WRAPPER_SOURCE,
            msg="deterministic controls must explain the fixed case retry budget",
        )
        self.assertIn(
            'if (( SMOKE_BUILD_RETRY_LIMIT != 1 )); then',
            WRAPPER_SOURCE,
            msg="deterministic controls must reject drifting build retry budgets",
        )
        self.assertIn(
            'fail "LCA_SMOKE_BUILD_RETRY_LIMIT must stay fixed at 1 for deterministic smoke control flow"',
            WRAPPER_SOURCE,
            msg="deterministic controls must explain the fixed build retry budget",
        )

    def test_build_stage_uses_the_isolated_smoke_build_in_active_path(self) -> None:
        self.assertIn(
            'build_cmd=("$BUILD_WRAPPER" "--out" "$BINARY")',
            WRAPPER_SOURCE,
            msg="smoke must anchor its deterministic build contract on the smoke-owned binary path",
        )
        self.assertIn(
            'run_build_command_with_retry \\',
            WRAPPER_SOURCE,
            msg="smoke must execute the isolated smoke build through the explicit build retry helper",
        )
        self.assertIn(
            '"producing the isolated smoke solver" \\',
            WRAPPER_SOURCE,
            msg="the active smoke build path must name the isolated smoke build context for retry classification",
        )
        self.assertIn(
            '"solver binary" \\',
            WRAPPER_SOURCE,
            msg="the active smoke build path must pass the smoke-owned solver binary label into the build retry helper",
        )

    def test_build_stage_records_an_isolated_smoke_build_command(self) -> None:
        self.assertIn(
            'printf \'%s\\n\' "$(quote_command "${build_cmd[@]}")" > "$SETUP_BUILD_COMMAND"',
            WRAPPER_SOURCE,
            msg="smoke setup must snapshot the isolated smoke-build command before execution",
        )
        pattern = rf"""
            build_solver_if_needed\(\)\s*\{{.*?
            build_cmd=\("\$BUILD_WRAPPER"\s+"--out"\s+"\$BINARY"\).*?
            run_build_command_with_retry\s+\\
        """
        self.assertRegex(
            WRAPPER_SOURCE,
            re.compile(pattern, re.DOTALL | re.VERBOSE),
            msg="smoke builds must execute the isolated smoke build directly from build_solver_if_needed",
        )

    def test_setup_environment_enforces_the_deterministic_build_contract(self) -> None:
        self.assertIn(
            "assert_deterministic_setup_build_contract() {",
            WRAPPER_SOURCE,
            msg="smoke setup/build must centralize deterministic path and seed-policy checks",
        )
        self.assertIn(
            'local expected_build_root="$SETUP_ROOT/build"',
            WRAPPER_SOURCE,
            msg="the deterministic setup contract must pin BUILD_ROOT under the smoke setup root",
        )
        self.assertIn(
            'local expected_binary="$expected_build_root/solve"',
            WRAPPER_SOURCE,
            msg="the deterministic setup contract must pin the smoke-owned solver binary path",
        )
        self.assertIn(
            'local expected_manifest_snapshot="$SETUP_ROOT/smoke_cases.snapshot.tsv"',
            WRAPPER_SOURCE,
            msg="the deterministic setup contract must pin the smoke manifest snapshot path",
        )
        self.assertIn(
            'if [[ "$BUILD_OUTPUT_TMP_GLOB" != ".solve.*.tmp" ]]; then',
            WRAPPER_SOURCE,
            msg="the deterministic setup contract must pin the smoke build temp cleanup glob",
        )
        self.assertIn(
            'if [[ "$SMOKE_SEED_POLICY" != "manifest_seed" ]]; then',
            WRAPPER_SOURCE,
            msg="the deterministic setup contract must reject seed-policy drift",
        )
        self.assertRegex(
            WRAPPER_SOURCE,
            re.compile(
                r"assert_setup_environment\(\)\s*\{.*?assert_deterministic_setup_build_contract",
                re.DOTALL,
            ),
            msg="setup validation must execute the deterministic setup/build contract before proceeding",
        )

    def test_setup_environment_snapshot_records_deterministic_build_seed_and_cleanup_fields(self) -> None:
        self.assertIn(
            'echo "build_binary=$BINARY"',
            WRAPPER_SOURCE,
            msg="setup snapshots must record the deterministic smoke build binary path",
        )
        self.assertIn(
            'echo "branch_build_cache_binary=$BRANCH_BUILD_CACHE_BINARY"',
            WRAPPER_SOURCE,
            msg="setup snapshots must record the deterministic branch build cache binary path",
        )
        self.assertIn(
            'echo "smoke_manifest_snapshot=$SMOKE_CASES_SNAPSHOT"',
            WRAPPER_SOURCE,
            msg="setup snapshots must record the deterministic smoke manifest snapshot path",
        )
        self.assertIn(
            'echo "smoke_manifest_sha256=$SMOKE_MANIFEST_SHA256"',
            WRAPPER_SOURCE,
            msg="setup snapshots must record a stable smoke manifest fingerprint for rerun correlation",
        )
        self.assertIn(
            'echo "seed_policy=$SMOKE_SEED_POLICY"',
            WRAPPER_SOURCE,
            msg="setup snapshots must record the manifest-seed policy for repeated runs",
        )
        self.assertIn(
            'echo "cleanup_globs=$LEGACY_TMP_GLOB;$PROBE_TMP_GLOB;$RUN_WORK_GLOB;$RUN_TMP_GLOB;$BUILD_OUTPUT_TMP_GLOB;$BUILD_TMP_GLOB;$BUILD_TMP_TMP_GLOB;$LEGACY_OUT_GLOB"',
            WRAPPER_SOURCE,
            msg="setup snapshots must record the temp-file cleanup contract for repeated runs",
        )
        self.assertIn(
            'echo "branch_artifact_tmp_root=${BRANCH_ARTIFACT_TMP_ROOT:-}"',
            WRAPPER_SOURCE,
            msg="setup snapshots must record the owned branch-local tmp root rather than only derived path labels",
        )
        self.assertIn(
            'echo "tmpdir=${TMPDIR:-}"',
            WRAPPER_SOURCE,
            msg="setup snapshots must record the effective TMPDIR for deterministic rerun audits",
        )
        self.assertIn(
            'echo "term=${TERM:-}"',
            WRAPPER_SOURCE,
            msg="setup snapshots must record the normalized TERM setting for deterministic environment audits",
        )
        self.assertIn(
            'record_setup_check "path" "build_binary" "$BINARY"',
            WRAPPER_SOURCE,
            msg="setup preflight must publish the smoke build binary path into the setup manifest",
        )
        self.assertIn(
            'record_setup_check "fingerprint" "smoke_manifest_sha256" "$SMOKE_MANIFEST_SHA256"',
            WRAPPER_SOURCE,
            msg="setup preflight must publish the smoke manifest fingerprint into the setup manifest",
        )
        self.assertIn(
            'record_setup_check "policy" "seed_policy" "$SMOKE_SEED_POLICY"',
            WRAPPER_SOURCE,
            msg="setup preflight must publish the seed policy into the setup manifest",
        )
        self.assertIn(
            'record_setup_check "policy" "cleanup_globs" "$LEGACY_TMP_GLOB;$PROBE_TMP_GLOB;$RUN_WORK_GLOB;$RUN_TMP_GLOB;$BUILD_OUTPUT_TMP_GLOB;$BUILD_TMP_GLOB;$BUILD_TMP_TMP_GLOB;$LEGACY_OUT_GLOB"',
            WRAPPER_SOURCE,
            msg="setup preflight must publish the cleanup-glob contract into the setup manifest",
        )

    def test_setup_preflight_records_directory_artifact_cwd_and_helper_import_checks(self) -> None:
        self.assertIn(
            'check_required_directory_recorded "$SCRIPT_DIR" "script_dir"',
            WRAPPER_SOURCE,
            msg="setup preflight must validate the wrapper script directory before any solver work starts",
        )
        self.assertIn(
            'check_required_directory_recorded "$BRANCH_ROOT" "branch_root"',
            WRAPPER_SOURCE,
            msg="setup preflight must validate the derived branch root before any solver work starts",
        )
        self.assertIn(
            'check_artifact_path_recorded "$SETUP_ROOT" "setup_root"',
            WRAPPER_SOURCE,
            msg="setup preflight must validate the setup root stays inside the branch-local artifact envelope",
        )
        self.assertIn(
            'check_artifact_path_recorded "$BINARY" "build_binary"',
            WRAPPER_SOURCE,
            msg="setup preflight must validate the smoke build binary path before build execution begins",
        )
        self.assertIn(
            'check_working_directory_recorded "$BRANCH_ROOT" "branch_root_cwd"',
            WRAPPER_SOURCE,
            msg="setup preflight must validate the effective cwd matches the derived branch root",
        )
        self.assertIn(
            'check_required_command_recorded mkdir || preflight_rc=2',
            WRAPPER_SOURCE,
            msg="setup preflight must validate mkdir because setup/build staging depends on it",
        )
        self.assertIn(
            'check_required_command_recorded rmdir || preflight_rc=2',
            WRAPPER_SOURCE,
            msg="setup preflight must validate rmdir because deterministic tmp cleanup depends on it",
        )
        self.assertIn(
            'check_required_command_recorded kill || preflight_rc=2',
            WRAPPER_SOURCE,
            msg="setup preflight must validate kill because signal-driven cleanup depends on it",
        )
        self.assertIn(
            'check_python_entrypoint_recorded "$ARTIFACT_RESOLVER" "artifact resolver imports" || preflight_rc=2',
            WRAPPER_SOURCE,
            msg="setup preflight must validate that the artifact resolver imports cleanly before build/setup writes proceed",
        )
        self.assertIn(
            'check_python_entrypoint_recorded "$RESUME_HELPER" "resume helper imports" || preflight_rc=2',
            WRAPPER_SOURCE,
            msg="setup preflight must validate that the resume helper imports cleanly before solver work starts",
        )
        self.assertIn(
            'preflight_message="setup/build environment or working-directory validation failed"',
            WRAPPER_SOURCE,
            msg="setup preflight must distinguish environment or cwd drift from plain missing-dependency failures",
        )

    def test_build_retry_helper_records_timeouts_but_returns_harness_failure_after_retry_budget(self) -> None:
        pattern = rf"""
            run_build_command_with_retry\(\)\s*\{{.*?
            local\s+max_attempts=\$\(\(\s*SMOKE_BUILD_RETRY_LIMIT\s+\+\s+1\s*\)\).*?
            if\s+\(\(\s*LAST_BOUNDED_COMMAND_TIMED_OUT\s*!=\s*0\s*\)\);\s*then.*?
            write_setup_failure_summary\s+"\$summary_phase"\s+"\$SMOKE_EXIT_SOLVER_TIMEOUT"\s+"\$summary_message".*?
            report_setup_failure_context\s+"\$summary_phase"\s+"\$SMOKE_EXIT_SOLVER_TIMEOUT"\s+"\$summary_message".*?
            return\s+"\$SMOKE_EXIT_HARNESS_FAILURE"
        """
        self.assertRegex(
            WRAPPER_SOURCE,
            re.compile(pattern, re.DOTALL | re.VERBOSE),
            msg="build retry handling must preserve the raw timeout in setup artifacts but normalize the public exit to harness failure",
        )
        self.assertIn(
            'echo "[lca_smoke] retrying transient build timeout: attempt=$(( attempt_index + 1 ))/$max_attempts timeout_s=$SMOKE_BUILD_TIMEOUT_S context=$build_context" >&2',
            WRAPPER_SOURCE,
            msg="build retry handling must log the retry path for bounded build timeouts",
        )
        self.assertIn(
            'echo "[lca_smoke] retrying transient build failure: attempt=$(( attempt_index + 1 ))/$max_attempts exit_code=$build_rc context=$build_context" >&2',
            WRAPPER_SOURCE,
            msg="build retry handling must log retries for unexpected build-wrapper exit codes",
        )

    def test_run_case_invocations_pin_case_identity_env(self) -> None:
        self.assertIn(
            '--env\n    "DENSE_PROFILE_OUTDIR=$case_dir"',
            WRAPPER_SOURCE,
            msg="smoke case execution must pin DENSE_PROFILE_OUTDIR to the case directory",
        )
        self.assertIn(
            '--env\n    "DENSE_SHADOW_CASE_MODE=$mode"',
            WRAPPER_SOURCE,
            msg="smoke case execution must pin the case identity env",
        )
        self.assertIn(
            '--env\n    "DENSE_SHADOW_CASE_N=$n"',
            WRAPPER_SOURCE,
            msg="smoke case execution must pin the case size env",
        )
        self.assertIn(
            '--env\n    "DENSE_SHADOW_CASE_SEED=$seed"',
            WRAPPER_SOURCE,
            msg="smoke case execution must pin the case seed env",
        )
        self.assertIn(
            "--env\n    DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1",
            WRAPPER_SOURCE,
            msg="smoke case execution must still pass the non-routing shadowcheck flag",
        )

    def test_failure_replay_pins_the_preserved_manifest_snapshot(self) -> None:
        self.assertIn(
            '"LCA_SMOKE_TARGET_MANIFEST=$FAILURE_ROOT/smoke_cases_manifest.tsv"',
            WRAPPER_SOURCE,
            msg="failure replay commands must pin smoke_target to the preserved manifest snapshot",
        )
        self.assertIn(
            'export LCA_SMOKE_TARGET_MANIFEST="\\$SCRIPT_DIR/smoke_cases_manifest.tsv"',
            WRAPPER_SOURCE,
            msg="failure replay scripts must restore the preserved manifest snapshot before calling smoke_target",
        )
        self.assertIn(
            'write_failure_structured_context "$failure_case_dir"',
            WRAPPER_SOURCE,
            msg="failure debug bundles must emit a stable structured context before publishing the artifact manifest",
        )

    def test_case_helper_command_keeps_deterministic_shadow_env_assignments(self) -> None:
        self.assertIn(
            '"DENSE_SHADOW_CASE_MODE=$mode"',
            WRAPPER_SOURCE,
            msg="smoke case execution must pin the mode in the helper environment",
        )
        self.assertIn(
            '"DENSE_SHADOW_CASE_N=$n"',
            WRAPPER_SOURCE,
            msg="smoke case execution must pin the size in the helper environment",
        )
        self.assertIn(
            '"DENSE_SHADOW_CASE_SEED=$seed"',
            WRAPPER_SOURCE,
            msg="smoke case execution must pin the seed in the helper environment",
        )
        self.assertIn(
            '"DENSE_PROFILE_OUTDIR=$case_dir"',
            WRAPPER_SOURCE,
            msg="smoke case execution must pin the profile/output directory in the helper environment",
        )

    def test_suite_metadata_write_recreates_or_rejects_missing_staging_dir(self) -> None:
        self.assertIn(
            'mkdir -p "$WORKDIR"',
            WRAPPER_SOURCE,
            msg="suite metadata writes must recreate the smoke staging directory before redirecting files",
        )
        self.assertIn(
            'smoke staging directory disappeared before writing suite metadata',
            WRAPPER_SOURCE,
            msg="suite metadata writes must fail with a targeted harness message if the staging dir is still missing",
        )

    def test_runtime_environment_refreshes_missing_runtime_tmpdir(self) -> None:
        self.assertIn(
            'if [[ -z "${RUN_TMPDIR:-}" || ! -d "$RUN_TMPDIR" ]]; then',
            WRAPPER_SOURCE,
            msg="runtime environment validation must detect a missing runtime tmpdir",
        )
        self.assertIn(
            "configure_runtime_tmpdir",
            WRAPPER_SOURCE,
            msg="runtime environment validation must refresh the runtime tmpdir before asserting it",
        )

    def test_case_execution_rebuilds_the_isolated_solver_if_it_disappears_mid_suite(self) -> None:
        self.assertIn(
            "ensure_solver_ready_for_case_execution() {",
            WRAPPER_SOURCE,
            msg="smoke must define a dedicated recovery helper for a missing per-smoke solver binary",
        )
        self.assertIn(
            'ensure_solver_ready_for_case_execution "$case_tag"',
            WRAPPER_SOURCE,
            msg="each smoke case must verify the isolated solver is still present before launching the case helper",
        )
        self.assertIn(
            'warning: smoke solver disappeared before case execution; rebuilding isolated smoke solver',
            WRAPPER_SOURCE,
            msg="solver-recovery diagnostics must explain why the wrapper is rebuilding mid-suite",
        )

        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import json
                import sys
                from pathlib import Path

                ROOT = Path(__file__).resolve().parent
                COUNTER = ROOT / "artifacts" / "solver_recovery_attempt.txt"
                EXIT_HARNESS_FAILURE = 70


                def write_result(path: Path, payload: dict[str, object]) -> None:
                    path.write_text(json.dumps(payload, sort_keys=True), encoding="utf-8")


                def main() -> int:
                    solver = Path(sys.argv[6])
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    result_path = outdir / "run_case_result.json"

                    if not solver.exists():
                        print(f"[run_case] harness failure: {solver}", file=sys.stderr)
                        write_result(
                            result_path,
                            {
                                "status": "harness_transient_failure",
                                "category": "harness",
                                "exit_code": EXIT_HARNESS_FAILURE,
                                "message": str(solver),
                                "validator_ok": None,
                            },
                        )
                        return EXIT_HARNESS_FAILURE

                    attempt = int(COUNTER.read_text(encoding="utf-8")) + 1 if COUNTER.exists() else 1
                    COUNTER.parent.mkdir(parents=True, exist_ok=True)
                    COUNTER.write_text(str(attempt), encoding="utf-8")

                    (outdir / "in.txt").write_text("1 0\\n", encoding="utf-8")
                    (outdir / "out.txt").write_text("", encoding="utf-8")
                    (outdir / "time.txt").write_text("0.01\\n", encoding="utf-8")
                    write_result(
                        result_path,
                        {
                            "status": "pass",
                            "category": "pass",
                            "exit_code": 0,
                            "message": "OK",
                            "validator_ok": True,
                            "sec": 0.01,
                            "rss_kb": 1,
                        },
                    )

                    if attempt == 1:
                        solver.unlink()
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )
            self.write_text(
                branch_root / "boj28350_resume" / "smoke_cases.tsv",
                (
                    "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                    "smoke\tcomb_core\t8\t1\t0\t0\t1\n"
                    "smoke\tcomb_core\t9\t2\t0\t0\t1\n"
                ),
            )

            completed = subprocess.run(
                [str(branch_root / "outer_suite_wrappers" / "lca_smoke.sh")],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )

            self.assertEqual(completed.returncode, 0, msg=completed.stderr)
            self.assertIn(
                "warning: smoke solver disappeared before case execution; rebuilding isolated smoke solver",
                completed.stderr,
            )
            self.assertTrue(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "case02_smoke_comb_core_n9_s2_L0_Q0_t1").is_dir(),
                msg="the later smoke case must still publish after the wrapper rebuilds the isolated solver",
            )

    def test_failure_bundle_writes_recreate_failure_root_before_debug_artifacts(self) -> None:
        self.assertIn(
            'copy_retry_log_to_failure_root() {\n  mkdir -p "$FAILURE_ROOT"',
            WRAPPER_SOURCE,
            msg="retry-log promotion must recreate the failure root before copying debug artifacts",
        )
        self.assertIn(
            'write_failure_debug_bundle() {\n  local failure_case_dir="$1"',
            WRAPPER_SOURCE,
            msg="failure debug bundle helper must exist for failure-path artifact writes",
        )
        self.assertIn(
            'mkdir -p "$FAILURE_ROOT"\n  printf \'%s\\n\' "$CURRENT_CASE_MANIFEST_ROW" > "$FAILURE_ROOT/failed_case_row.tsv"',
            WRAPPER_SOURCE,
            msg="failure debug bundle writes must recreate the failure root before emitting summary files",
        )

    def test_failure_bundle_exports_expected_output_and_invoked_command_snapshots(self) -> None:
        self.assertIn(
            'local expected_output_txt="$FAILURE_ROOT/expected_output.txt"',
            WRAPPER_SOURCE,
            msg="failure repro exports must allocate a stable expected-output snapshot path",
        )
        self.assertIn(
            'cp "$failure_case_dir/hidden_parent.txt" "$expected_output_txt"',
            WRAPPER_SOURCE,
            msg="failure repro exports must snapshot the preserved hidden-parent tree as expected_output.txt",
        )
        self.assertIn(
            'local invoked_command_txt="$FAILURE_ROOT/invoked_command.txt"',
            WRAPPER_SOURCE,
            msg="failure reporting must allocate a stable invoked-command snapshot path",
        )
        self.assertIn(
            'printf \'%s\\n\' "$CURRENT_CASE_EXEC_COMMAND" > "$invoked_command_txt"',
            WRAPPER_SOURCE,
            msg="failure repro exports must preserve the exact helper invocation in invoked_command.txt",
        )
        self.assertIn(
            'echo "expected_output_path=$expected_output_txt"',
            WRAPPER_SOURCE,
            msg="failure summaries must surface the expected-output snapshot path for the next iteration",
        )
        self.assertIn(
            'echo "invoked_command_path=$invoked_command_txt"',
            WRAPPER_SOURCE,
            msg="failure summaries must surface the invoked-command snapshot path for the next iteration",
        )

    def test_failure_reporting_fallback_synthesizes_actionable_summary_bundle(self) -> None:
        self.assertIn(
            'write_failure_reporting_fallback() {\n  local failure_case_dir="$1"\n  local reporting_warning="$2"',
            WRAPPER_SOURCE,
            msg="smoke must define a fallback failure-reporting helper for degraded summary/report writes",
        )
        self.assertIn(
            'write_failure_repro_exports "$failure_case_dir" || return 1',
            WRAPPER_SOURCE,
            msg="fallback failure reporting must still materialize the exact seed/input/output snapshots before synthesizing summaries",
        )
        self.assertIn(
            'echo "failure_reporting_status=degraded"',
            WRAPPER_SOURCE,
            msg="fallback failure summaries must mark degraded reporting status explicitly",
        )
        self.assertIn(
            'echo "exact_seed_path=$FAILURE_ROOT/seed.txt"',
            WRAPPER_SOURCE,
            msg="fallback failure summaries must surface the exact seed snapshot path for the next iteration",
        )
        self.assertIn(
            'echo "exact_input_path=$FAILURE_ROOT/input.txt"',
            WRAPPER_SOURCE,
            msg="fallback failure summaries must surface the exact input snapshot path for the next iteration",
        )
        self.assertIn(
            'echo "exact_output_path=$FAILURE_ROOT/solver_output.txt"',
            WRAPPER_SOURCE,
            msg="fallback failure summaries must surface the exact actual-output snapshot path for the next iteration",
        )
        self.assertIn(
            'echo "expected_output_path=$FAILURE_ROOT/expected_output.txt"',
            WRAPPER_SOURCE,
            msg="fallback failure summaries must surface the exact expected-output snapshot path for the next iteration",
        )
        self.assertIn(
            'echo "- Failure reporting status: \\`degraded\\`"',
            WRAPPER_SOURCE,
            msg="fallback failure reports must surface degraded reporting status for the next iteration",
        )
        self.assertIn(
            'echo "- Exact seed snapshot: \\`$FAILURE_ROOT/seed.txt\\`"',
            WRAPPER_SOURCE,
            msg="fallback failure reports must point directly at the preserved seed snapshot",
        )
        self.assertIn(
            'echo "- Exact input snapshot: \\`$FAILURE_ROOT/input.txt\\`"',
            WRAPPER_SOURCE,
            msg="fallback failure reports must point directly at the preserved input snapshot",
        )
        self.assertIn(
            'echo "- Exact actual output snapshot: \\`$FAILURE_ROOT/solver_output.txt\\`"',
            WRAPPER_SOURCE,
            msg="fallback failure reports must point directly at the preserved solver output snapshot",
        )
        self.assertIn(
            'echo "- Exact expected output snapshot: \\`$FAILURE_ROOT/expected_output.txt\\`"',
            WRAPPER_SOURCE,
            msg="fallback failure reports must point directly at the preserved expected-output snapshot",
        )
        self.assertIn(
            'write_failure_reporting_fallback_artifact_manifest "$failure_case_dir"',
            WRAPPER_SOURCE,
            msg="fallback failure reporting must still emit an artifact manifest inventory",
        )
        self.assertIn(
            'write_failure_structured_context "$failure_case_dir" || return 1',
            WRAPPER_SOURCE,
            msg="fallback failure reporting must still emit the structured retry handoff before writing summaries",
        )
        self.assertIn(
            'echo "- Start with \\`$FAILURE_ROOT/failure_context.json\\`, \\`$commands_txt\\`, and \\`$artifact_manifest\\` to see the preserved case identity, replay commands, and surviving paths without re-scraping the markdown bundle."',
            WRAPPER_SOURCE,
            msg="fallback failure reports must point the next iteration at the minimal actionable artifacts first",
        )

    def test_launcher_status_bundle_surfaces_structured_failure_context(self) -> None:
        launcher_source = (Path(__file__).resolve().parent / "lca_smoke.sh").read_text(encoding="utf-8")
        self.assertIn(
            'append_launcher_status_diagnostic_entry "source_failure_structured_context" "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH" "machine-readable failure handoff for the next retry iteration"',
            launcher_source,
            msg="launcher diagnostics must publish the structured failure-context path when smoke fails",
        )
        self.assertIn(
            'echo "source_failure_structured_context_path=$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH"',
            launcher_source,
            msg="launcher status summaries must surface the structured failure-context path for downstream tooling",
        )
        self.assertIn(
            'echo "- Source structured context: \\`$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH\\`"',
            launcher_source,
            msg="launcher status reports must point the next iteration at the structured smoke failure handoff",
        )

    def test_launcher_status_bundle_publishes_retry_loop_continuation_control(self) -> None:
        launcher_source = (Path(__file__).resolve().parent / "lca_smoke.sh").read_text(encoding="utf-8")
        self.assertIn(
            'append_launcher_status_diagnostic_entry "status_retry_loop_control" "$LAUNCHER_STATUS_RETRY_LOOP_CONTROL" "machine-readable retry-loop and next-gate continuation control"',
            launcher_source,
            msg="launcher diagnostics must inventory the stable retry-loop control handoff",
        )
        self.assertIn(
            'echo "published_smoke_retry_loop_control_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH"',
            launcher_source,
            msg="launcher status summaries must surface the smoke-root retry-loop control mirror",
        )
        self.assertIn(
            '"retry_loop": {',
            launcher_source,
            msg="standard-gap json must expose retry-loop control as a first-class section",
        )
        self.assertIn(
            '"preferred_command": summary.get("retry_loop_preferred_command")',
            launcher_source,
            msg="retry-loop control json must preserve the preferred continuation command",
        )
        self.assertIn(
            'echo "- Preferred retry-loop command: \\`$LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND\\`"',
            launcher_source,
            msg="launcher status reports must show the preferred retry-loop continuation command",
        )

    def test_primary_failure_summary_records_reporting_status_for_consistent_shape(self) -> None:
        self.assertIn(
            'echo "failure_reporting_status=complete"',
            WRAPPER_SOURCE,
            msg="primary failure summaries must mark complete reporting status so downstream tooling sees a consistent schema",
        )
        self.assertIn(
            'echo "failure_reporting_warning="',
            WRAPPER_SOURCE,
            msg="primary failure summaries must keep a reserved reporting-warning field even on the successful reporting path",
        )
        self.assertIn(
            'echo "- Failure reporting status: \\`complete\\`"',
            WRAPPER_SOURCE,
            msg="primary failure reports must surface the complete reporting status explicitly",
        )
        self.assertIn(
            'echo "structured_context_path=$FAILURE_ROOT/failure_context.json"',
            WRAPPER_SOURCE,
            msg="primary failure summaries must expose the structured retry handoff path in the stable summary schema",
        )

    def test_final_failure_path_retries_reporting_with_fallback_summary_bundle(self) -> None:
        self.assertIn(
            'if ! write_failure_reporting_fallback "$failure_case_dir" "write_failure_summary did not complete; fallback summary/report synthesized from preserved paths"; then',
            WRAPPER_SOURCE,
            msg="final failure handling must retry reporting with the fallback synthesizer when the rich report path fails",
        )
        self.assertIn(
            'echo "[lca_smoke] warning: failed to synthesize fallback failure summary/report; downstream iteration guidance may be incomplete" >&2',
            WRAPPER_SOURCE,
            msg="final failure handling must emit a distinct warning if even the fallback summary path fails",
        )

    def test_final_failure_path_leaves_staging_cleanup_owned_by_exit_teardown_when_promotion_fails(self) -> None:
        self.assertIn(
            'echo "[lca_smoke] warning: failed to promote preserved failure bundle; exporting summary from live case dir $case_dir and leaving staging cleanup to EXIT teardown" >&2',
            WRAPPER_SOURCE,
            msg="final failure handling must keep the live case dir available for summary export while EXIT cleanup reclaims the staging tree",
        )
        self.assertNotIn(
            'echo "[lca_smoke] warning: failed to promote preserved failure bundle; keeping staging tree at $case_dir" >&2\n        WORKDIR=""',
            WRAPPER_SOURCE,
            msg="final failure handling must not drop the staging root handle before EXIT cleanup can reclaim it",
        )

    def test_retry_log_uses_stable_setup_root_instead_of_runtime_tmpdir(self) -> None:
        self.assertIn(
            'SMOKE_RETRY_LOG="$SETUP_ROOT/retry_log.tsv"',
            WRAPPER_SOURCE,
            msg="retry logging must survive runtime tmpdir churn during failure handling",
        )
        self.assertNotIn(
            'SMOKE_RETRY_LOG="$RUN_TMPDIR/lca_smoke_retry_log.tsv"',
            WRAPPER_SOURCE,
            msg="retry logging must not depend on the volatile runtime tmpdir",
        )
        self.assertIn(
            'retry_parent="$(dirname "$SMOKE_RETRY_LOG")"',
            WRAPPER_SOURCE,
            msg="retry logging must derive its parent directory from the stable retry log path",
        )
        self.assertIn(
            'mkdir -p "$retry_parent"',
            WRAPPER_SOURCE,
            msg="retry logging must recreate its parent directory before appending",
        )
        self.assertIn(
            'printf \'case_index\\tcase_tag\\tattempt\\tstage\\tmode\\tn\\tseed\\tshuffle_labels\\tshuffle_queries\\ttimeout_s\\thelper_timeout_bound_s\\tmanifest_sha256\\texit_code\\thelpper_exit_code\\tfailure_kind\\tfailure_origin\\tretryable\\tsummary\\n\' > "$SMOKE_RETRY_LOG"',
            WRAPPER_SOURCE,
            msg="retry logging must preserve full case identity and the manifest fingerprint for post-failure iteration",
        )
        self.assertIn(
            '"$SMOKE_MANIFEST_SHA256"',
            WRAPPER_SOURCE,
            msg="retry log rows must carry the manifest fingerprint alongside the failing case identity",
        )

    def test_suite_metadata_records_build_retry_policy_and_budget(self) -> None:
        self.assertIn(
            'echo "build_retry_policy=$SMOKE_BUILD_RETRY_POLICY"',
            WRAPPER_SOURCE,
            msg="suite metadata must record the build retry policy alongside the case retry policy",
        )
        self.assertIn(
            'echo "build_retry_limit=$SMOKE_BUILD_RETRY_LIMIT"',
            WRAPPER_SOURCE,
            msg="suite metadata must record the build retry budget",
        )
        self.assertIn(
            'echo "build_retry_sleep_s=$SMOKE_BUILD_RETRY_SLEEP_S"',
            WRAPPER_SOURCE,
            msg="suite metadata must record the build retry backoff",
        )

    def test_wrapper_enforced_case_helper_timeout_is_retryable_harness_failure_without_result_json(self) -> None:
        self.assertIn(
            'if (( LAST_BOUNDED_COMMAND_TIMED_OUT != 0 )) && [[ ! -f "$CURRENT_CASE_RESULT_JSON" ]]; then',
            WRAPPER_SOURCE,
            msg="the helper-timeout branch must only trigger when the bounded wrapper timed out and no result JSON was preserved",
        )
        self.assertIn(
            'CURRENT_FAILURE_HELPER_RC="$SMOKE_EXIT_SOLVER_TIMEOUT"',
            WRAPPER_SOURCE,
            msg="the helper-timeout branch must record the raw helper timeout exit code for diagnostics",
        )
        self.assertIn(
            'CURRENT_FAILURE_RC="$SMOKE_EXIT_HARNESS_FAILURE"',
            WRAPPER_SOURCE,
            msg="the helper-timeout branch must normalize the public exit code to harness failure",
        )
        self.assertIn(
            'set_failure_classification \\\n      "harness_transient_failure" \\\n      "harness" \\\n      1 \\',
            WRAPPER_SOURCE,
            msg="the helper-timeout branch must classify the failure as a retryable harness transient",
        )
        self.assertIn(
            '"case helper exceeded wall-clock timeout after ${helper_timeout_s}s before writing ${RUN_CASE_RESULT_NAME}"',
            WRAPPER_SOURCE,
            msg="wrapper-enforced helper timeouts must explain that the helper never wrote its result JSON",
        )

    def test_missing_helper_result_with_solver_like_exit_retries_as_harness_transient(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import json
                import sys
                from pathlib import Path

                ROOT = Path(__file__).resolve().parent
                COUNTER = ROOT / "artifacts" / "helper_attempt.txt"


                def main() -> int:
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    attempt = int(COUNTER.read_text(encoding="utf-8")) + 1 if COUNTER.exists() else 1
                    COUNTER.parent.mkdir(parents=True, exist_ok=True)
                    COUNTER.write_text(str(attempt), encoding="utf-8")
                    if attempt == 1:
                        print("[run_case] harness failure: simulated missing result bundle", file=sys.stderr)
                        return 1

                    (outdir / "in.txt").write_text("1 0\\n", encoding="utf-8")
                    (outdir / "out.txt").write_text("", encoding="utf-8")
                    (outdir / "run_case_result.json").write_text(
                        json.dumps(
                            {
                                "status": "pass",
                                "category": "pass",
                                "exit_code": 0,
                                "message": "OK",
                                "validator_ok": True,
                            }
                        )
                        + "\\n",
                        encoding="utf-8",
                    )
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertIn(
                "retrying harness-transient failure",
                result.stderr,
                msg="missing helper results with solver-like exits must spend the retry budget in the harness lane",
            )
            self.assertIn(
                "helper_exit_code=1 normalized_exit_code=70",
                result.stderr,
                msg="retry logs must surface the raw helper exit and the normalized harness exit separately",
            )
            self.assertEqual(
                (branch_root / "artifacts" / "helper_attempt.txt").read_text(encoding="utf-8").strip(),
                "2",
                msg="a transient no-result helper failure must trigger exactly one retry",
            )
            self.assertTrue(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke").is_dir(),
                msg="the smoke output tree must still publish after the retried helper succeeds",
            )
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure").exists(),
                msg="successful retry recovery must not leave a stale smoke failure bundle behind",
            )

    def test_missing_helper_result_with_usage_exit_stays_non_retryable(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import sys
                from pathlib import Path

                ROOT = Path(__file__).resolve().parent
                COUNTER = ROOT / "artifacts" / "helper_attempt.txt"


                def main() -> int:
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    attempt = int(COUNTER.read_text(encoding="utf-8")) + 1 if COUNTER.exists() else 1
                    COUNTER.parent.mkdir(parents=True, exist_ok=True)
                    COUNTER.write_text(str(attempt), encoding="utf-8")
                    print("[run_case] invalid branch-local invocation", file=sys.stderr)
                    return 2


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 2, msg=result.stderr)
            self.assertNotIn(
                "retrying harness-transient failure",
                result.stderr,
                msg="usage failures without helper results must not consume the transient retry budget",
            )
            self.assertEqual(
                (branch_root / "artifacts" / "helper_attempt.txt").read_text(encoding="utf-8").strip(),
                "1",
                msg="usage failures without helper results must stop after the first attempt",
            )
            failure_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure" / "failure_summary.txt"
            self.assertTrue(
                failure_summary.is_file(),
                msg="non-retryable usage failures must still preserve the failure summary for the next iteration",
            )
            self.assertIn(
                "failure_kind=harness_usage_failure",
                failure_summary.read_text(encoding="utf-8"),
            )

    def test_build_failure_stops_before_case_execution_and_preserves_setup_logs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                from pathlib import Path


                def main() -> int:
                    root = Path(__file__).resolve().parent
                    marker = root / "artifacts" / "run_case_invoked.txt"
                    marker.parent.mkdir(parents=True, exist_ok=True)
                    marker.write_text("unexpected helper invocation\\n", encoding="utf-8")
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )
            self.write_text(
                branch_root / "build.sh",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail
                    echo "[build] forced compile failure for smoke wrapper regression" >&2
                    exit 1
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "build.sh")

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            self.assertIn(
                "setup/build failed before stress start",
                result.stderr,
                msg="build failures must terminate through the setup/build failure path before any case execution",
            )
            self.assertIn(
                "build wrapper failed while producing the isolated smoke solver",
                result.stderr,
                msg="build failures must preserve the isolated-solver build context in stderr",
            )
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure"
            failure_summary = failure_root / "failure_summary.txt"
            build_stderr = failure_root / "setup_build" / "build.stderr.txt"
            self.assertTrue(
                failure_summary.is_file(),
                msg="build failures must preserve the setup/build failure summary bundle",
            )
            self.assertTrue(
                build_stderr.is_file(),
                msg="build failures must preserve the captured build stderr",
            )
            self.assertIn(
                "failure_kind=build",
                failure_summary.read_text(encoding="utf-8"),
            )
            self.assertIn(
                "forced compile failure for smoke wrapper regression",
                build_stderr.read_text(encoding="utf-8"),
            )
            self.assertFalse(
                (branch_root / "artifacts" / "run_case_invoked.txt").exists(),
                msg="build failures must stop before the case helper is ever invoked",
            )

    def test_failed_smoke_run_preserves_expected_output_and_invoked_command_snapshots(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import json
                import sys
                from pathlib import Path


                def main() -> int:
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    (outdir / "in.txt").write_text("3 1\\n2 3 2\\n", encoding="utf-8")
                    (outdir / "out.txt").write_text("0 1 1\\n", encoding="utf-8")
                    (outdir / "hidden_parent.txt").write_text("0 1 2\\n", encoding="utf-8")
                    (outdir / "meta.json").write_text("{\\"case\\": \\"fake\\"}\\n", encoding="utf-8")
                    (outdir / "time.txt").write_text("0.01 1024\\n", encoding="utf-8")
                    (outdir / "solver_stderr.txt").write_text("solver stderr\\n", encoding="utf-8")
                    (outdir / "run_case_result.json").write_text(
                        json.dumps(
                            {
                                "status": "solver_acceptance_failure",
                                "category": "solver",
                                "exit_code": 1,
                                "message": "query #1 mismatch: lca(2, 3)=1, expected 2",
                                "validator_ok": False,
                            }
                        )
                        + "\\n",
                        encoding="utf-8",
                    )
                    print("[run_case] validator failed: query #1 mismatch: lca(2, 3)=1, expected 2", file=sys.stderr)
                    return 1


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 1, msg=result.stderr)
            failure_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure").resolve()
            expected_output = failure_root / "expected_output.txt"
            invoked_command = failure_root / "invoked_command.txt"
            actual_output = failure_root / "solver_output.txt"
            failure_summary = (failure_root / "failure_summary.txt").read_text(encoding="utf-8")
            commands_txt = (failure_root / "commands.txt").read_text(encoding="utf-8")
            artifact_manifest = (failure_root / "artifact_manifest.tsv").read_text(encoding="utf-8")
            failure_context = json.loads((failure_root / "failure_context.json").read_text(encoding="utf-8"))

            self.assertEqual(
                expected_output.read_text(encoding="utf-8"),
                "0 1 2\n",
                msg="failure bundles must preserve the hidden-parent tree as a stable expected-output snapshot",
            )
            self.assertEqual(
                actual_output.read_text(encoding="utf-8"),
                "0 1 1\n",
                msg="failure bundles must still preserve the exact actual solver output",
            )
            self.assertIn(
                "branch_run_case.py",
                invoked_command.read_text(encoding="utf-8"),
                msg="failure bundles must preserve the exact helper invocation in invoked_command.txt",
            )
            self.assertIn(
                f"expected_output_path={expected_output}",
                failure_summary,
                msg="failure summaries must point the next iteration at expected_output.txt",
            )
            self.assertIn(
                f"invoked_command_path={invoked_command}",
                failure_summary,
                msg="failure summaries must point the next iteration at invoked_command.txt",
            )
            self.assertIn(
                "manifest_sha256=",
                failure_summary,
                msg="failure summaries must record the manifest fingerprint that produced the preserved case",
            )
            self.assertIn(
                f"expected_output={expected_output}",
                commands_txt,
                msg="commands.txt must record the stable expected-output snapshot path",
            )
            self.assertIn(
                f"invoked_command={invoked_command}",
                commands_txt,
                msg="commands.txt must record the stable invoked-command snapshot path",
            )
            self.assertIn(
                f"expected_output\t{expected_output}\t1\t",
                artifact_manifest,
                msg="artifact manifests must inventory the expected-output snapshot",
            )
            self.assertIn(
                f"invoked_command\t{invoked_command}\t1\t",
                artifact_manifest,
                msg="artifact manifests must inventory the invoked-command snapshot",
            )
            self.assertIn(
                "failure_context_json",
                artifact_manifest,
                msg="artifact manifests must inventory the structured failure context for the next retry iteration",
            )
            self.assertEqual(
                failure_context["schema"],
                "lca_smoke_failure_context_v1",
                msg="failure bundles must publish a stable structured-context schema for downstream retry tooling",
            )
            self.assertEqual(
                failure_context["failure"]["kind"],
                "solver_acceptance_failure",
                msg="structured failure context must carry the normalized smoke failure classification",
            )
            self.assertEqual(
                failure_context["case"]["tag"],
                "case01_smoke_comb_core_n8_s1_L0_Q0_t1",
                msg="structured failure context must identify the exact failing smoke row",
            )
            self.assertEqual(
                failure_context["helper_result"]["message"],
                "query #1 mismatch: lca(2, 3)=1, expected 2",
                msg="structured failure context must preserve the original validator mismatch message from the helper result",
            )
            self.assertIn(
                "query_mismatch",
                failure_context,
                msg="structured failure context must reserve a stable parsed-mismatch slot even when a minimal fake harness skips the richer checker path",
            )
            self.assertTrue(
                failure_context["artifacts"]["expected_output"]["exists"],
                msg="structured failure context must publish existence for preserved expected-output artifacts",
            )
            self.assertIn(
                "active_solver_replay",
                failure_context["commands"],
                msg="structured failure context must surface the next-iteration replay commands directly",
            )
            self.assertIn(
                "[lca_smoke] exact expected output snapshot:",
                result.stderr,
                msg="stderr failure context must surface the stable expected-output path",
            )
            self.assertIn(
                "[lca_smoke] invoked command snapshot:",
                result.stderr,
                msg="stderr failure context must surface the stable invoked-command path",
            )
            self.assertIn(
                "[lca_smoke] structured context:",
                result.stderr,
                msg="stderr failure context must surface the machine-readable handoff path",
            )

    def test_non_retryable_failure_records_failed_case_and_cleans_runtime_staging_dirs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import json
                import sys
                from pathlib import Path


                def main() -> int:
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    (outdir / "in.txt").write_text("1 0\\n", encoding="utf-8")
                    (outdir / "out.txt").write_text("", encoding="utf-8")
                    (outdir / "run_case_result.json").write_text(
                        json.dumps(
                            {
                                "status": "solver_acceptance_failure",
                                "category": "solver",
                                "exit_code": 1,
                                "message": "forced smoke gate failure for cleanup regression",
                                "validator_ok": False,
                            }
                        )
                        + "\\n",
                        encoding="utf-8",
                    )
                    print("[run_case] forced smoke gate failure for cleanup regression", file=sys.stderr)
                    return 1


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 1, msg=result.stderr)
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure"
            self.assertTrue(
                (failure_root / "failed_case_row.tsv").is_file(),
                msg="non-retryable smoke failures must record the exact manifest row for the failing case",
            )
            self.assertEqual(
                (failure_root / "failed_case_row.tsv").read_text(encoding="utf-8").strip(),
                "smoke\tcomb_core\t8\t1\t0\t0\t1",
                msg="failed_case_row.tsv must preserve the branch-local failing smoke manifest row",
            )
            tmp_root = branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp"
            self.assertEqual(
                list(tmp_root.glob("lca_smoke.run.*")),
                [],
                msg="controlled smoke failure exit must not leave staging run directories behind",
            )
            self.assertEqual(
                list(tmp_root.glob("lca_smoke.tmp.*")),
                [],
                msg="controlled smoke failure exit must not leave runtime tmp directories behind",
            )

    def test_failure_bundle_records_frozen_manifest_hash_in_setup_snapshot_and_preflight_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import json
                import sys
                from pathlib import Path


                def main() -> int:
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    (outdir / "in.txt").write_text("1 0\\n", encoding="utf-8")
                    (outdir / "out.txt").write_text("", encoding="utf-8")
                    (outdir / "run_case_result.json").write_text(
                        json.dumps(
                            {
                                "status": "solver_acceptance_failure",
                                "category": "solver",
                                "exit_code": 1,
                                "message": "force manifest fingerprint preservation check",
                                "validator_ok": False,
                            }
                        )
                        + "\\n",
                        encoding="utf-8",
                    )
                    return 1


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 1, msg=result.stderr)
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure"
            manifest_snapshot = failure_root / "smoke_cases_manifest.tsv"
            setup_env = failure_root / "environment_validation" / "setup_env.txt"
            preflight_manifest = failure_root / "environment_validation" / "preflight_manifest.tsv"
            manifest_sha = hashlib.sha256(manifest_snapshot.read_bytes()).hexdigest()

            self.assertIn(
                f"smoke_manifest_sha256={manifest_sha}",
                setup_env.read_text(encoding="utf-8"),
                msg="preserved setup snapshots must record the frozen smoke-manifest hash after the branch-local snapshot exists",
            )
            self.assertIn(
                f"fingerprint\tsmoke_manifest_sha256\t{manifest_sha}",
                preflight_manifest.read_text(encoding="utf-8"),
                msg="preflight manifests must record the same frozen smoke-manifest hash for post-failure retry analysis",
            )

    def test_setup_preflight_cwd_drift_writes_a_structured_failure_bundle(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(Path(tmp), run_case_body="from __future__ import annotations")
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_smoke.sh"
            wrapper_path.unlink()
            sabotaged_source = WRAPPER_SOURCE.replace(
                '  record_setup_environment_snapshot\n',
                '  cd "$BRANCH_ROOT/.."\n  record_setup_environment_snapshot\n',
                1,
            )
            self.write_text(wrapper_path, sabotaged_source)
            self.make_executable(wrapper_path)

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            self.assertIn(
                "setup/build environment or working-directory validation failed",
                result.stderr,
                msg="cwd drift during setup preflight must fail through the structured setup failure path",
            )
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure"
            failure_summary = failure_root / "failure_summary.txt"
            failure_report = failure_root / "latest_failure_report.md"
            preflight_manifest = failure_root / "setup_build" / "preflight_manifest.tsv"
            setup_env = failure_root / "setup_build" / "setup_env.txt"

            self.assertTrue(
                failure_summary.is_file(),
                msg="setup cwd drift must preserve a stable setup failure summary bundle",
            )
            self.assertTrue(
                failure_report.is_file(),
                msg="setup cwd drift must preserve a stable setup failure report bundle",
            )
            self.assertTrue(
                preflight_manifest.is_file(),
                msg="setup cwd drift must preserve the setup preflight manifest for the next retry",
            )
            self.assertTrue(
                setup_env.is_file(),
                msg="setup cwd drift must preserve the setup environment snapshot for the next retry",
            )
            self.assertIn(
                "failure_kind=preflight",
                failure_summary.read_text(encoding="utf-8"),
            )
            self.assertIn(
                "working_directory_mismatch\tbranch_root_cwd\t",
                preflight_manifest.read_text(encoding="utf-8"),
                msg="setup preflight manifests must record the working-directory mismatch explicitly",
            )
            self.assertIn(
                f"pwd={branch_root.parent.resolve()}",
                setup_env.read_text(encoding="utf-8"),
                msg="the preserved setup environment snapshot must capture the drifted cwd that triggered the preflight failure",
            )

    def test_failure_bundle_copy_fallback_keeps_exit_cleanup_attached_to_live_staging_tree(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import json
                import sys
                from pathlib import Path


                def main() -> int:
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    (outdir / "in.txt").write_text("1 0\\n", encoding="utf-8")
                    (outdir / "out.txt").write_text("", encoding="utf-8")
                    (outdir / "run_case_result.json").write_text(
                        json.dumps(
                            {
                                "status": "solver_acceptance_failure",
                                "category": "solver",
                                "exit_code": 1,
                                "message": "forced smoke gate failure for move-fallback cleanup regression",
                                "validator_ok": False,
                            }
                        )
                        + "\\n",
                        encoding="utf-8",
                    )
                    return 1


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_smoke.sh"
            wrapper_path.unlink()
            move_snippet = '    if mv "$source" "$target" 2>/dev/null; then\n'
            self.assertIn(move_snippet, WRAPPER_SOURCE)
            forced_wrapper_source = WRAPPER_SOURCE.replace(
                move_snippet,
                (
                    '    if [[ "$target" == "$FAILURE_ROOT" ]]; then\n'
                    "      sleep 0.1\n"
                    '    elif mv "$source" "$target" 2>/dev/null; then\n'
                ),
                1,
            )
            self.write_text(wrapper_path, forced_wrapper_source)
            self.make_executable(wrapper_path)

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 1, msg=result.stderr)
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure"
            tmp_root = branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp"
            self.assertTrue(
                (failure_root / "failure_summary.txt").is_file(),
                msg="copy fallback must still preserve the latest failure summary bundle",
            )
            self.assertTrue(
                (failure_root / "failed_case_row.tsv").is_file(),
                msg="copy fallback must still preserve the failed case row under smoke_latest_failure",
            )
            self.assertNotIn(
                "failed to promote preserved failure bundle",
                result.stderr,
                msg="copy fallback should count as a preserved failure bundle instead of a promotion failure",
            )
            self.assertEqual(
                list(tmp_root.glob("lca_smoke.run.*")),
                [],
                msg="copy fallback must keep EXIT cleanup attached to the original staging workdir",
            )
            self.assertEqual(
                list(tmp_root.glob("lca_smoke.tmp.*")),
                [],
                msg="copy fallback must still allow EXIT cleanup to reclaim runtime tmp directories",
            )

    def test_successful_run_recovers_from_invalid_lock_root_and_cleans_it_after_release(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import json
                import sys
                from pathlib import Path


                def main() -> int:
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    (outdir / "in.txt").write_text("1 0\\n", encoding="utf-8")
                    (outdir / "out.txt").write_text("", encoding="utf-8")
                    (outdir / "run_case_result.json").write_text(
                        json.dumps(
                            {
                                "status": "pass",
                                "category": "pass",
                                "exit_code": 0,
                                "message": "OK",
                                "validator_ok": True,
                            }
                        )
                        + "\\n",
                        encoding="utf-8",
                    )
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )
            lock_root = branch_root / "artifacts" / "lca_tree_stress_v5" / ".locks"
            self.write_text(lock_root, "not a directory\\n")

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke").is_dir(),
                msg="smoke should recover from an invalid lock root and still publish a successful run",
            )
            self.assertFalse(
                lock_root.exists(),
                msg="successful cleanup must remove the recovered lock root after releasing the smoke lock",
            )

    def test_successful_inner_run_keeps_launcher_owned_tmpdir_intact(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import json
                import sys
                from pathlib import Path


                def main() -> int:
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    (outdir / "in.txt").write_text("1 0\\n", encoding="utf-8")
                    (outdir / "out.txt").write_text("", encoding="utf-8")
                    (outdir / "run_case_result.json").write_text(
                        json.dumps(
                            {
                                "status": "pass",
                                "category": "pass",
                                "exit_code": 0,
                                "message": "OK",
                                "validator_ok": True,
                            }
                        )
                        + "\\n",
                        encoding="utf-8",
                    )
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )
            launcher_tmpdir = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp" / "lca_smoke.launcher.tmp"
            )
            self.write_text(launcher_tmpdir / "sentinel.txt", "launcher-owned\n")

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(
                (launcher_tmpdir / "sentinel.txt").is_file(),
                msg="inner smoke execution must not delete the launcher-owned tmpdir while the parent wrapper may still need it",
            )
            self.assertTrue(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke").is_dir(),
                msg="the smoke output tree must still publish after preserving the launcher-owned tmpdir",
            )

    def test_missing_main_prerequisite_preserves_stale_smoke_state_until_validation_passes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(Path(tmp), run_case_body="from __future__ import annotations")
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            setup_root = artifacts_root / "smoke_setup"
            session_root = artifacts_root / ".tmp" / "lca_smoke.session"
            setup_tmpdir = artifacts_root / ".tmp" / "lca_smoke.setup.tmp"
            failure_root = artifacts_root / "smoke_latest_failure"
            run_tmpdir = artifacts_root / ".tmp" / "lca_smoke.tmp.stale"
            run_workdir = artifacts_root / ".tmp" / "lca_smoke.run.stale"

            self.write_text(setup_root / "stale.txt", "stale setup state\n")
            self.write_text(session_root / "home" / "stale.txt", "stale session state\n")
            self.write_text(setup_tmpdir / "stale.txt", "stale setup tmp state\n")
            self.write_text(failure_root / "stale.txt", "stale failure state\n")
            self.write_text(run_tmpdir / "stale.txt", "stale runtime tmp state\n")
            self.write_text(run_workdir / "stale.txt", "stale workdir state\n")
            (branch_root / "build.py").unlink()

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            self.assertIn("missing build helper:", result.stderr)
            self.assertTrue(
                (setup_root / "stale.txt").is_file(),
                msg="upfront prerequisite failures must not clear prior setup roots before cleanup ownership begins",
            )
            self.assertTrue(
                (session_root / "home" / "stale.txt").is_file(),
                msg="upfront prerequisite failures must not clear prior session roots before cleanup ownership begins",
            )
            self.assertTrue(
                (setup_tmpdir / "stale.txt").is_file(),
                msg="upfront prerequisite failures must not clear prior setup tmpdirs before cleanup ownership begins",
            )
            self.assertTrue(
                (failure_root / "stale.txt").is_file(),
                msg="upfront prerequisite failures must not clear the last failure bundle before validation passes",
            )
            self.assertTrue(
                (run_tmpdir / "stale.txt").is_file(),
                msg="upfront prerequisite failures must not clear prior runtime tmpdirs before cleanup ownership begins",
            )
            self.assertTrue(
                (run_workdir / "stale.txt").is_file(),
                msg="upfront prerequisite failures must not clear prior workdirs before cleanup ownership begins",
            )

    def test_successful_run_clears_stale_shared_state_and_exports_a_fresh_external_snapshot_tree(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_runtime_branch(
                Path(tmp),
                run_case_body="""
                #!/usr/bin/env python3
                from __future__ import annotations

                import json
                import sys
                from pathlib import Path


                def main() -> int:
                    outdir = Path(sys.argv[7])
                    outdir.mkdir(parents=True, exist_ok=True)
                    (outdir / "in.txt").write_text("1 0\\n", encoding="utf-8")
                    (outdir / "out.txt").write_text("", encoding="utf-8")
                    (outdir / "time.txt").write_text("0.01\\n", encoding="utf-8")
                    (outdir / "run_case_result.json").write_text(
                        json.dumps(
                            {
                                "status": "pass",
                                "category": "pass",
                                "exit_code": 0,
                                "message": "OK",
                                "validator_ok": True,
                                "sec": 0.01,
                                "rss_kb": 1,
                            }
                        )
                        + "\\n",
                        encoding="utf-8",
                    )
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """,
            )
            setup_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_setup"
            session_root = branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp" / "lca_smoke.session"
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure"
            snapshot_root = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_external_snapshot"
            )
            self.write_text(setup_root / "stale.txt", "stale setup state\n")
            self.write_text(session_root / "stale.txt", "stale session state\n")
            self.write_text(failure_root / "stale.txt", "stale failure state\n")
            self.write_text(snapshot_root / "stale.txt", "stale\n")
            self.write_text(snapshot_root / "orphan_case" / "old.txt", "old\n")
            env = os.environ.copy()
            env["LCA_SMOKE_EXPORT_SNAPSHOT_ROOT"] = "artifacts/lca_tree_stress_v5/smoke_external_snapshot"
            hostile_tmpdir = (branch_root / "host_tmpdir").resolve()
            hostile_home = (branch_root / "host_home").resolve()
            hostile_pycache = (branch_root / "host_pycache").resolve()
            hostile_tmpdir.mkdir()
            hostile_home.mkdir()
            hostile_pycache.mkdir()
            env["TERM"] = "xterm-256color"
            env["TMPDIR"] = str(hostile_tmpdir)
            env["TMP"] = str(hostile_tmpdir)
            env["TEMP"] = str(hostile_tmpdir)
            env["HOME"] = str(hostile_home)
            env["PYTHONPYCACHEPREFIX"] = str(hostile_pycache)

            result = subprocess.run(
                ["./outer_suite_wrappers/lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
                env=env,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertFalse(
                setup_root.exists(),
                msg="successful smoke reruns must clear stale setup state without manual cleanup",
            )
            self.assertFalse(
                session_root.exists(),
                msg="successful smoke reruns must clear stale session roots without manual cleanup",
            )
            self.assertFalse(
                failure_root.exists(),
                msg="successful smoke reruns must clear stale failure bundles before the next iteration",
            )
            self.assertFalse((snapshot_root / "stale.txt").exists())
            self.assertFalse((snapshot_root / "orphan_case").exists())
            self.assertTrue(
                (snapshot_root / "suite_config.txt").is_file(),
                msg="external smoke snapshots must preserve suite metadata from the successful staging tree",
            )
            self.assertTrue(
                (snapshot_root / "environment_validation" / "smoke_cases.snapshot.tsv").is_file(),
                msg="external smoke snapshots must preserve the frozen manifest snapshot for exact reruns",
            )
            self.assertTrue(
                (snapshot_root / "case01_smoke_comb_core_n8_s1_L0_Q0_t1").is_dir(),
                msg="external smoke snapshots must preserve the successful case directory for later inspection",
            )
            self.assertTrue(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "suite_config.txt").is_file(),
                msg="snapshot export must not replace the normal published smoke output root",
            )
            exported_environment = (
                snapshot_root / "environment_validation.txt"
            ).read_text(encoding="utf-8")
            self.assertIn(
                f"external_snapshot_root={snapshot_root.resolve()}",
                exported_environment,
                msg="exported smoke snapshots must record which branch-local external snapshot root was used",
            )
            self.assertIn(
                "term=dumb",
                exported_environment,
                msg="successful smoke reruns must pin TERM instead of inheriting the caller terminal type",
            )
            self.assertIn(
                f"home={(branch_root / 'artifacts' / 'lca_tree_stress_v5' / '.tmp' / 'lca_smoke.session' / 'home').resolve()}",
                exported_environment,
                msg="successful smoke reruns must route HOME into the branch-local session state",
            )
            self.assertIn(
                f"setup_tmpdir={(branch_root / 'artifacts' / 'lca_tree_stress_v5' / '.tmp' / 'lca_smoke.setup.tmp').resolve()}",
                exported_environment,
                msg="environment validation must still publish the deterministic setup tmp root",
            )
            runtime_tmp_prefix = (
                str((branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp").resolve())
                + "/lca_smoke.tmp."
            )
            self.assertRegex(
                exported_environment,
                re.compile(rf"branch_artifact_tmp_root={re.escape(runtime_tmp_prefix)}[^\n]+"),
                msg="environment validation must publish the rebound runtime branch-local tmp root",
            )
            self.assertRegex(
                exported_environment,
                re.compile(rf"tmpdir={re.escape(runtime_tmp_prefix)}[^\n]+"),
                msg="environment validation must publish the rebound runtime TMPDIR after setup completes",
            )

    def test_load_release_environment_rebinds_runtime_tmpdir_after_sourcing_release_env(self) -> None:
        self.assertIn(
            'if [[ -z "${RUN_TMPDIR:-}" ]]; then',
            WRAPPER_SOURCE,
            msg="load_release_environment must guard against an unset runtime tmpdir",
        )
        self.assertGreaterEqual(
            WRAPPER_SOURCE.count('mkdir -p "$RUN_TMPDIR"'),
            2,
            msg="load_release_environment must recreate the runtime tmpdir before and after sourcing release env",
        )
        self.assertGreaterEqual(
            WRAPPER_SOURCE.count('export BRANCH_ARTIFACT_TMP_ROOT="$RUN_TMPDIR"'),
            2,
            msg="load_release_environment must rebind BRANCH_ARTIFACT_TMP_ROOT around the release env source step",
        )
        self.assertIn(
            'source "$RELEASE_ENV"',
            WRAPPER_SOURCE,
            msg="load_release_environment must still source the release env wrapper",
        )
        self.assertIn(
            "assert_runtime_environment",
            WRAPPER_SOURCE,
            msg="load_release_environment must validate the rebound runtime environment",
        )

    def test_main_validates_setup_dependencies_before_mutating_smoke_state(self) -> None:
        self.assertIn(
            "require_command mkdir",
            WRAPPER_SOURCE,
            msg="smoke main must validate mkdir before setup cleanup mutates artifact roots",
        )
        self.assertIn(
            "require_command rmdir",
            WRAPPER_SOURCE,
            msg="smoke main must validate rmdir before setup cleanup relies on it",
        )
        self.assertIn(
            "require_command kill",
            WRAPPER_SOURCE,
            msg="smoke main must validate kill before stale-lock recovery runs",
        )
        self.assertIn(
            "require_build_compiler",
            WRAPPER_SOURCE,
            msg="smoke main must validate compiler availability before setup cleanup starts",
        )
        self.assertIn(
            'require_file "$BUILD_HELPER" "build helper"',
            WRAPPER_SOURCE,
            msg="smoke main must validate the build helper before setup cleanup starts",
        )
        self.assertIn(
            'require_file "$RESUME_HELPER" "resume helper"',
            WRAPPER_SOURCE,
            msg="smoke main must validate the resume helper before setup cleanup starts",
        )
        self.assertIn(
            'require_python_entrypoint "$BUILD_HELPER" "build helper imports"',
            WRAPPER_SOURCE,
            msg="smoke main must validate build-helper imports before setup cleanup starts",
        )
        self.assertIn(
            'require_python_entrypoint "$RUN_CASE_HELPER" "run case helper imports"',
            WRAPPER_SOURCE,
            msg="smoke main must validate run-case imports before setup cleanup starts",
        )
        self.assertIn(
            'require_python_entrypoint "$CHECKER_HELPER" "validator helper imports"',
            WRAPPER_SOURCE,
            msg="smoke main must validate validator imports before setup cleanup starts",
        )

    def test_main_validates_repo_layout_before_mutating_smoke_state(self) -> None:
        self.assertIn(
            'require_directory "$SCRIPT_DIR" "outer smoke wrapper directory"',
            WRAPPER_SOURCE,
            msg="smoke main must validate the wrapper directory before cleanup owns smoke state",
        )
        self.assertIn(
            'require_directory "$BRANCH_ROOT" "branch root directory"',
            WRAPPER_SOURCE,
            msg="smoke main must validate the derived branch root before cleanup owns smoke state",
        )
        self.assertIn(
            'require_directory "$RESUME_WORKSPACE_DIR" "resume workspace directory"',
            WRAPPER_SOURCE,
            msg="smoke main must validate the repo-root resume workspace before cleanup owns smoke state",
        )

    def test_exit_cleanup_stays_dormant_until_run_main_owns_smoke_state(self) -> None:
        self.assertIn(
            "SMOKE_CLEANUP_ACTIVE=0",
            WRAPPER_SOURCE,
            msg="smoke cleanup must start dormant until setup/runtime ownership begins",
        )
        self.assertRegex(
            WRAPPER_SOURCE,
            re.compile(
                r"cleanup\(\)\s*\{.*?if \(\( SMOKE_CLEANUP_ACTIVE == 0 \)\); then.*?exit \"\$rc\".*?fi",
                re.DOTALL,
            ),
            msg="EXIT cleanup must short-circuit before mutating smoke state when upfront validation fails",
        )
        self.assertRegex(
            WRAPPER_SOURCE,
            re.compile(r"run_main\(\)\s*\{.*?SMOKE_CLEANUP_ACTIVE=1", re.DOTALL),
            msg="run_main must arm cleanup only after main's upfront validation succeeds",
        )

    def test_resolve_output_roots_normalizes_setup_and_output_paths(self) -> None:
        self.assertIn(
            'normalize_artifact_path() {',
            WRAPPER_SOURCE,
            msg="smoke wrapper must provide a dedicated artifact-path normalization helper",
        )
        self.assertIn(
            'parts[0] == branch_root.name and parts[1] == artifacts_root.name',
            WRAPPER_SOURCE,
            msg="artifact-path normalization must collapse branch-root-prefixed artifact paths before cleanup",
        )
        self.assertIn(
            'candidate = artifacts_root.joinpath(*parts) if artifact_rooted else branch_root.joinpath(*parts)',
            WRAPPER_SOURCE,
            msg="artifact-path normalization must canonicalize relative paths against the branch and artifact roots deterministically",
        )
        self.assertIn(
            'OUTROOT="$(normalize_artifact_path "$resolved_outroot" "smoke output root")"',
            WRAPPER_SOURCE,
            msg="resolve_output_roots must canonicalize the smoke output root before cleanup",
        )
        self.assertIn(
            'TMP_PARENT="$(normalize_artifact_path "$ARTIFACTS_ROOT/.tmp" "smoke tmp parent")"',
            WRAPPER_SOURCE,
            msg="resolve_output_roots must canonicalize the smoke tmp parent before setup cleanup",
        )
        self.assertIn(
            'FAILURE_ROOT="$(normalize_artifact_path "$ARTIFACTS_ROOT/smoke_latest_failure" "smoke failure root")"',
            WRAPPER_SOURCE,
            msg="resolve_output_roots must canonicalize the stable failure root before setup cleanup",
        )
        self.assertIn(
            'SETUP_ROOT="$(normalize_artifact_path "$ARTIFACTS_ROOT/smoke_setup" "smoke setup root")"',
            WRAPPER_SOURCE,
            msg="resolve_output_roots must canonicalize the smoke setup root before setup cleanup",
        )
        self.assertIn(
            'LOCKDIR="$(normalize_artifact_path "$LOCK_ROOT/lca_smoke" "smoke lock directory")"',
            WRAPPER_SOURCE,
            msg="resolve_output_roots must canonicalize the smoke lock directory before lock recovery",
        )


if __name__ == "__main__":
    unittest.main()
