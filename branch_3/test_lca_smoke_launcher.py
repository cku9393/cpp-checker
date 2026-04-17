#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import signal
import shutil
import subprocess
import tempfile
import textwrap
import time
import unittest
from pathlib import Path


ENTRYPOINT_PATH = Path(__file__).resolve().parent / "lca_smoke.sh"
WRAPPER_PATH = ENTRYPOINT_PATH
WRAPPER_SOURCE = WRAPPER_PATH.read_text(encoding="utf-8")
HOST_TMP_ROOT = Path("/tmp/cpp-checker-branch3-smoke-tests/launcher")
HOST_TMP_ROOT.mkdir(parents=True, exist_ok=True)
# Keep host-side test scaffolding off any inherited retry-loop TMPDIR so these
# tests only exercise the smoke launcher's own environment reset logic.
tempfile.tempdir = str(HOST_TMP_ROOT)


class LcaSmokeLauncherRegressionTests(unittest.TestCase):
    def write_text(self, path: Path, content: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def read_tsv_rows(self, path: Path) -> list[dict[str, str]]:
        lines = path.read_text(encoding="utf-8").splitlines()
        header = lines[0].split("\t")
        rows: list[dict[str, str]] = []
        for line in lines[1:]:
            if not line:
                continue
            values = line.split("\t")
            rows.append(dict(zip(header, values)))
        return rows

    def expected_bash_command(self, path: Path) -> str:
        quoted_path = str(path.resolve()).replace(" ", "\\ ")
        return f"/bin/bash {quoted_path}"

    def make_executable(self, path: Path) -> None:
        path.chmod(path.stat().st_mode | 0o111)

    def symlink_file(self, target: Path, link_path: Path) -> None:
        link_path.parent.mkdir(parents=True, exist_ok=True)
        link_path.symlink_to(target)

    def wait_for_path(self, path: Path, *, timeout_s: float = 5.0) -> None:
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            if path.exists():
                return
            time.sleep(0.05)
        self.fail(f"timed out waiting for path: {path}")

    def wait_for_pid_exit(self, pid: int, *, timeout_s: float = 5.0) -> None:
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            try:
                os.kill(pid, 0)
            except ProcessLookupError:
                return
            except PermissionError:
                pass
            time.sleep(0.05)
        self.fail(f"timed out waiting for pid to exit: {pid}")

    def make_fake_branch(
        self,
        temp_root: Path,
        *,
        artifacts_root_render: str = "print(ARTIFACTS_ROOT)",
        smoke_cases_contents: str | None = None,
        inner_wrapper_body: str | None = None,
    ) -> Path:
        branch_root = temp_root / "branch"
        self.symlink_file(ENTRYPOINT_PATH, branch_root / "lca_smoke.sh")
        self.write_text(
            branch_root / "artifact_paths.py",
            textwrap.dedent(
                f"""
                #!/usr/bin/env python3
                from __future__ import annotations

                import argparse
                from pathlib import Path

                BRANCH_ROOT = Path(__file__).resolve().parent
                ARTIFACTS_ROOT = (BRANCH_ROOT / "artifacts").resolve()


                def main() -> int:
                    parser = argparse.ArgumentParser()
                    parser.add_argument("--artifacts-root", action="store_true")
                    args = parser.parse_args()
                    if not args.artifacts_root:
                        parser.error("expected --artifacts-root")
                    {artifacts_root_render}
                    return 0


                if __name__ == "__main__":
                    raise SystemExit(main())
                """
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "outer_suite_wrappers" / "lca_smoke.sh",
            textwrap.dedent(
                inner_wrapper_body
                or """
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed1\\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
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
                exit 0
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
        for rel_path in (
            Path("branch_run_case.py"),
            Path("branch_validator.py"),
            Path("build.py"),
            Path("boj28350_resume.py"),
            Path("boj28350_resume/boj28350_branch_3_solver.cpp"),
        ):
            self.write_text(branch_root / rel_path, "from __future__ import annotations\n")
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
            branch_root / "boj28350_resume/smoke_cases.tsv",
            smoke_cases_contents
            or (
                "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                "smoke\tcomb_core\t64\t1\t1\t1\t2\n"
            ),
        )
        for rel_path in (
            Path("lca_smoke.sh"),
            Path("outer_suite_wrappers/lca_smoke.sh"),
            Path("build.sh"),
            Path("lca_smoke_target.sh"),
        ):
            self.make_executable(branch_root / rel_path)
        return branch_root

    def test_launcher_wrapper_is_executable_for_branch_local_entrypoint(self) -> None:
        self.assertTrue(
            os.access(WRAPPER_PATH, os.X_OK),
            msg="the branch-local smoke entrypoint must be directly runnable as ./lca_smoke.sh",
        )

    def test_launcher_bootstrap_reexecs_into_clean_env_and_marks_inner_wrapper_clean(self) -> None:
        self.assertIn(
            'LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG="LCA_SMOKE_LAUNCHER_CLEAN_ENV_READY"',
            WRAPPER_SOURCE,
            msg="launcher must define its own clean-env guard before preflight work",
        )
        self.assertIn(
            'LCA_SMOKE_INNER_CLEAN_ENV_FLAG="LCA_SMOKE_CLEAN_ENV_READY"',
            WRAPPER_SOURCE,
            msg="launcher must coordinate with the inner smoke clean-env flag",
        )
        self.assertIn(
            '"$LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG=1"',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must mark its own guard before re-exec",
        )
        self.assertIn(
            '"$LCA_SMOKE_INNER_CLEAN_ENV_FLAG=1"',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must mark the inner wrapper clean-env guard before re-exec",
        )
        self.assertIn(
            '"TMPDIR=/tmp"',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must reset TMPDIR before branch-local tmp roots exist",
        )
        self.assertIn(
            '"TMP=/tmp"',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must reset TMP before branch-local tmp roots exist",
        )
        self.assertIn(
            '"TEMP=/tmp"',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must reset TEMP before branch-local tmp roots exist",
        )
        self.assertIn(
            'for preserved_name in \\',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must iterate over the supported preserved launcher overrides",
        )
        self.assertIn(
            'LCA_SMOKE_EXPORT_SNAPSHOT_ROOT \\',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must preserve the smoke export snapshot override",
        )
        self.assertIn(
            'LCA_SMOKE_DEBUG_MANIFEST \\',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must preserve the explicit debug manifest override",
        )
        self.assertIn(
            'LCA_SMOKE_BUILD_TIMEOUT_S \\',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must preserve the build-timeout override",
        )
        self.assertIn(
            'LCA_SMOKE_LAUNCHER_TIMEOUT_S \\',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must preserve the launcher dispatch-timeout override",
        )
        self.assertIn(
            'LCA_SMOKE_LAUNCHER_LOCK_TIMEOUT_S \\',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must preserve the launcher lock-timeout override",
        )
        self.assertIn(
            'LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND \\',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must preserve the original user-facing invocation command across re-exec",
        )
        self.assertIn(
            'LCA_SMOKE_LAUNCHER_ORIGINAL_PWD; do',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must preserve the original launch cwd across re-exec",
        )
        self.assertIn(
            'LCA_SMOKE_LAUNCHER_REEXEC_ARG="--__lca_smoke_launcher_clean_env_reexec"',
            WRAPPER_SOURCE,
            msg="launcher must define a private reexec marker so fresh runs do not trust stale clean-env flags",
        )
        self.assertIn(
            'if [[ "${launcher_args[0]:-}" == "$LCA_SMOKE_LAUNCHER_REEXEC_ARG" ]]; then',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must only trust the clean-env guard on its own reexec path",
        )
        self.assertIn(
            'unset "$LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG" "$LCA_SMOKE_INNER_CLEAN_ENV_FLAG"',
            WRAPPER_SOURCE,
            msg="launcher clean-env bootstrap must discard stale inherited clean-env guards before re-exec",
        )
        self.assertIn(
            'exec "${clean_env_args[@]}" /usr/bin/env bash "$SELF_PATH" "$LCA_SMOKE_LAUNCHER_REEXEC_ARG" "${launcher_args[@]}"',
            WRAPPER_SOURCE,
            msg="launcher must re-exec itself with a private marker after assembling the clean environment",
        )

    def test_launcher_preflight_failure_snapshot_uses_deterministic_bootstrap_env(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root = self.make_fake_branch(temp_root)
            (branch_root / "build.py").unlink()
            env = os.environ.copy()
            env["HOME"] = str((temp_root / "ambient_home").resolve())
            env["TERM"] = "xterm-256color"
            env["LC_ALL"] = "ko_KR.UTF-8"
            env["LANG"] = "ko_KR.UTF-8"
            env["TZ"] = "Asia/Seoul"
            env["PYTHONHASHSEED"] = "777"
            env["PYTHONNOUSERSITE"] = "0"

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
                env=env,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            env_snapshot = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "smoke_launcher_latest_failure"
                / "launcher_env.txt"
            )
            self.assertTrue(
                env_snapshot.is_file(),
                msg="preflight failures must publish the bootstrap environment snapshot",
            )
            captured = dict(
                line.split("=", 1)
                for line in env_snapshot.read_text(encoding="utf-8").splitlines()
                if "=" in line
            )
            self.assertEqual(captured["HOME"], str(branch_root.resolve()))
            self.assertEqual(captured["TERM"], "dumb")
            self.assertEqual(captured["LC_ALL"], "C")
            self.assertEqual(captured["LANG"], "C")
            self.assertEqual(captured["TZ"], "UTC")
            self.assertEqual(captured["PYTHONHASHSEED"], "0")
            self.assertEqual(captured["PYTHONNOUSERSITE"], "1")

    def test_launcher_environment_setup_and_exit_cleanup_use_launcher_tmpdir(self) -> None:
        self.assertIn(
            'LAUNCHER_TMPDIR="$TMP_PARENT/lca_smoke.launcher.tmp"',
            WRAPPER_SOURCE,
            msg="launcher must dedicate a launcher-local tmpdir under the branch smoke tmp root",
        )
        self.assertIn(
            'if [[ -e "$LAUNCHER_TMPDIR" ]]; then',
            WRAPPER_SOURCE,
            msg="launcher setup must detect stale launcher tmpdir state before reusing it",
        )
        self.assertIn(
            'remove_path_retry "$LAUNCHER_TMPDIR" || fail "failed to clear stale launcher tmpdir: $LAUNCHER_TMPDIR"',
            WRAPPER_SOURCE,
            msg="launcher setup must clear stale launcher tmpdir state before preflight",
        )
        self.assertIn(
            'ensure_launcher_directory "$LAUNCHER_TMPDIR" "launcher tmpdir" || fail "failed to prepare launcher tmpdir: $LAUNCHER_TMPDIR"',
            WRAPPER_SOURCE,
            msg="launcher setup must recreate the stable launcher tmpdir before preflight work starts",
        )
        self.assertIn(
            'export BRANCH_ARTIFACT_TMP_ROOT="$LAUNCHER_TMPDIR"',
            WRAPPER_SOURCE,
            msg="launcher preflight must route tmp usage into the launcher-local tmpdir",
        )
        self.assertIn(
            'export HOME="$LAUNCHER_HOME"',
            WRAPPER_SOURCE,
            msg="launcher preflight must isolate HOME under the launcher-local tmpdir",
        )
        self.assertIn(
            'trap \'cleanup_launcher "$?"\' EXIT',
            WRAPPER_SOURCE,
            msg="launcher must register exit cleanup before doing launcher-owned setup work",
        )
        self.assertIn(
            'trap \'capture_launcher_err "$?" "$LINENO" "$BASH_COMMAND"\' ERR',
            WRAPPER_SOURCE,
            msg="launcher must register an ERR trap so unexpected pre-dispatch failures still preserve the exact failing command",
        )
        self.assertIn(
            "trap - EXIT ERR",
            WRAPPER_SOURCE,
            msg="launcher cleanup must disable both EXIT and ERR traps before tearing down launcher-owned state",
        )
        self.assertIn(
            'remove_path_retry "$LAUNCHER_TMPDIR" || true',
            WRAPPER_SOURCE,
            msg="launcher exit cleanup must remove the launcher-local tmpdir on early exit",
        )

    def test_launcher_run_control_uses_a_dedicated_lock_before_shared_cleanup(self) -> None:
        self.assertIn(
            'LAUNCHER_LOCKDIR="$LOCK_ROOT/lca_smoke.launcher"',
            WRAPPER_SOURCE,
            msg="launcher run control must dedicate its own lock directory under the shared smoke lock root",
        )
        self.assertIn(
            'resolve_launcher_lock_timeout() {',
            WRAPPER_SOURCE,
            msg="launcher run control must validate a bounded lock-wait timeout before retry-safe serialization",
        )
        self.assertIn(
            'acquire_launcher_lock() {',
            WRAPPER_SOURCE,
            msg="launcher run control must serialize repeated invocations before shared cleanup and dispatch",
        )
        self.assertIn(
            'release_launcher_lock() {',
            WRAPPER_SOURCE,
            msg="launcher cleanup must release the launcher run-control lock on every exit path",
        )
        self.assertIn(
            'set_launcher_failure_stage "launcher_lock_acquisition"',
            WRAPPER_SOURCE,
            msg="launcher failures must surface lock acquisition as a distinct pre-dispatch stage",
        )
        self.assertIn(
            'if (( LAUNCHER_LOCK_HELD != 0 )); then',
            WRAPPER_SOURCE,
            msg="launcher cleanup must conditionally release the launcher run-control lock before pruning the lock root",
        )

    def test_launcher_normalizes_cwd_umask_and_status_root_before_preflight(self) -> None:
        self.assertIn(
            "umask 022",
            WRAPPER_SOURCE,
            msg="launcher setup must pin a deterministic umask before writing launcher-owned artifacts",
        )
        self.assertIn(
            "enter_branch_root",
            WRAPPER_SOURCE,
            msg="launcher must normalize into the branch root before preflight work begins",
        )
        self.assertIn(
            'set_launcher_failure_stage "working_directory_normalization"',
            WRAPPER_SOURCE,
            msg="launcher must expose the branch-root normalization stage in failure reporting",
        )
        self.assertIn(
            'set_launcher_failure_stage "stale_status_cleanup"',
            WRAPPER_SOURCE,
            msg="launcher must expose stale status cleanup before the new run owns the public status root",
        )
        self.assertIn(
            'remove_path_retry "$LAUNCHER_STATUS_ROOT" || fail "failed to clear stale launcher status root: $LAUNCHER_STATUS_ROOT"',
            WRAPPER_SOURCE,
            msg="launcher must clear any stale public status bundle before continuing with a fresh run",
        )
        self.assertIn(
            'set_launcher_failure_stage "stale_smoke_root_cleanup"',
            WRAPPER_SOURCE,
            msg="launcher must expose stale smoke-root cleanup before the inner wrapper publishes the next public smoke bundle",
        )
        self.assertIn(
            'remove_path_retry "$SMOKE_OUTPUT_ROOT" || fail "failed to clear stale smoke output root: $SMOKE_OUTPUT_ROOT"',
            WRAPPER_SOURCE,
            msg="launcher must clear the public smoke root so repeated runs cannot inherit stale suite outputs",
        )

    def test_launcher_tracks_dispatch_start_time_in_memory_for_freshness_validation(self) -> None:
        self.assertIn(
            'LAUNCHER_DISPATCH_STARTED_NS=""',
            WRAPPER_SOURCE,
            msg="launcher must track the dispatch start timestamp even if the on-disk marker later disappears",
        )
        self.assertIn(
            'LAUNCHER_DISPATCH_STARTED_NS="$(',
            WRAPPER_SOURCE,
            msg="launcher must capture the dispatch marker mtime immediately after recording it",
        )
        self.assertIn(
            'python3 - "$LAUNCHER_DISPATCH_STARTED_NS" "$artifact"',
            WRAPPER_SOURCE,
            msg="launcher freshness checks must prefer the in-memory dispatch timestamp when validating smoke outputs",
        )
        self.assertIn(
            'if [[ -z "${LAUNCHER_DISPATCH_MARKER:-}" || ! -e "$LAUNCHER_DISPATCH_MARKER" ]]; then',
            WRAPPER_SOURCE,
            msg="launcher freshness checks must still retain the on-disk marker fallback when no in-memory dispatch timestamp exists",
        )

    def test_launcher_preflight_validates_branch_prerequisites_before_dispatch(self) -> None:
        self.assertIn(
            'validate_launcher_repo_root_layout',
            WRAPPER_SOURCE,
            msg="launcher setup must validate the repo-root layout before the deeper branch prerequisite checks",
        )
        self.assertIn(
            'require_directory "$BRANCH_ROOT" "branch root directory"',
            WRAPPER_SOURCE,
            msg="launcher setup must fail fast if the derived branch root directory is misconfigured",
        )
        self.assertIn(
            'require_directory "$OUTER_SUITE_WRAPPERS_DIR" "outer suite wrappers directory"',
            WRAPPER_SOURCE,
            msg="launcher setup must fail fast if the repo-root outer-suite wrappers directory is misconfigured",
        )
        self.assertIn(
            'require_directory "$RESUME_WORKSPACE_DIR" "resume workspace directory"',
            WRAPPER_SOURCE,
            msg="launcher setup must fail fast if the repo-root resume workspace directory is misconfigured",
        )
        self.assertIn(
            'normalize_launcher_prerequisite_paths',
            WRAPPER_SOURCE,
            msg="launcher setup must canonicalize repo-root prerequisite paths before the file/executable checks",
        )
        self.assertIn(
            'require_executable "$INNER_WRAPPER" "outer smoke wrapper"',
            WRAPPER_SOURCE,
            msg="launcher must fail fast if the outer smoke wrapper is not directly executable from the branch root",
        )
        self.assertIn(
            'require_file "$RUN_CASE_HELPER" "branch-local case helper"',
            WRAPPER_SOURCE,
            msg="launcher must fail fast if the branch-local run-case helper is missing",
        )
        self.assertIn(
            'require_file "$CHECKER_HELPER" "branch-local validator"',
            WRAPPER_SOURCE,
            msg="launcher must fail fast if the branch-local validator is missing",
        )
        self.assertIn(
            'require_file "$BUILD_HELPER" "build helper"',
            WRAPPER_SOURCE,
            msg="launcher must fail fast if the branch-local build helper is missing",
        )
        self.assertIn(
            'require_file "$RESUME_HELPER" "resume helper"',
            WRAPPER_SOURCE,
            msg="launcher must fail fast if the branch-local resume helper is missing",
        )
        self.assertIn(
            'require_executable "$BUILD_WRAPPER" "build wrapper"',
            WRAPPER_SOURCE,
            msg="launcher must fail fast if the branch-local build wrapper is not executable",
        )
        self.assertIn(
            'require_executable "$SMOKE_TARGET_WRAPPER" "smoke target wrapper"',
            WRAPPER_SOURCE,
            msg="launcher must fail fast if the smoke target wrapper is not executable",
        )
        self.assertIn(
            "require_command kill",
            WRAPPER_SOURCE,
            msg="launcher must validate kill before dispatch so stale-lock recovery prerequisites do not drift into runtime failures",
        )
        self.assertIn(
            "require_build_compiler",
            WRAPPER_SOURCE,
            msg="launcher must validate compiler availability before dispatch so repeated runs start from a build-capable state",
        )
        self.assertIn(
            'check_python_entrypoint "$ARTIFACT_RESOLVER" "artifact resolver imports"',
            WRAPPER_SOURCE,
            msg="launcher must validate the artifact resolver import path before dispatch",
        )
        self.assertIn(
            'check_python_entrypoint "$BUILD_HELPER" "build helper imports"',
            WRAPPER_SOURCE,
            msg="launcher must validate the build helper import path before dispatch",
        )
        self.assertIn(
            'check_python_entrypoint "$RUN_CASE_HELPER" "run case helper imports"',
            WRAPPER_SOURCE,
            msg="launcher must validate the run-case helper import path before dispatch",
        )
        self.assertIn(
            'check_python_entrypoint "$CHECKER_HELPER" "validator helper imports"',
            WRAPPER_SOURCE,
            msg="launcher must validate the validator helper import path before dispatch",
        )
        self.assertIn(
            'check_python_entrypoint "$RESUME_HELPER" "resume helper imports"',
            WRAPPER_SOURCE,
            msg="launcher must validate the resume helper import path before dispatch",
        )

    def test_launcher_failures_publish_a_branch_local_failure_bundle(self) -> None:
        self.assertIn(
            'LAUNCHER_FAILURE_ROOT_DEFAULT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure"',
            WRAPPER_SOURCE,
            msg="launcher failures must publish into a launcher-specific stable branch-local smoke artifact root",
        )
        self.assertIn(
            'write_launcher_failure_bundle || true',
            WRAPPER_SOURCE,
            msg="launcher cleanup must attempt to preserve a failure bundle before exit",
        )
        self.assertIn(
            'report_launcher_failure_context || true',
            WRAPPER_SOURCE,
            msg="launcher failure cleanup must print the preserved failure-bundle locations",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] launcher failed before inner wrapper dispatch"',
            WRAPPER_SOURCE,
            msg="launcher failure reporting must distinguish pre-dispatch launcher failures from inner smoke failures",
        )

    def test_launcher_preflight_rejects_non_executable_outer_wrapper_before_dispatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            outer_wrapper = branch_root / "outer_suite_wrappers" / "lca_smoke.sh"
            outer_wrapper.chmod(0o644)

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            self.assertIn(
                "missing executable outer smoke wrapper",
                result.stderr,
                msg="launcher must fail in preflight when the delegated outer smoke wrapper loses its executable bit",
            )
            failure_root = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_launcher_latest_failure"
            ).resolve()
            preflight_manifest = (failure_root / "preflight_manifest.tsv").read_text(encoding="utf-8")
            failure_summary = (failure_root / "failure_summary.txt").read_text(encoding="utf-8")
            self.assertIn(
                f"executable\touter smoke wrapper\tnot_executable\t{outer_wrapper.resolve()}\t-",
                preflight_manifest,
                msg="launcher failure manifests must record a non-executable delegated wrapper deterministically",
            )
            self.assertIn(
                f"working_directory={branch_root.resolve()}",
                failure_summary,
                msg="launcher failure bundles must still report the normalized branch-root working directory",
            )

    def test_launcher_failure_bundle_snapshots_command_env_and_preflight_state(self) -> None:
        self.assertIn(
            'LAUNCHER_INVOCATION_COMMAND="${LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND:-$(quote_command "$SELF_PATH" "$@")}"',
            WRAPPER_SOURCE,
            msg="launcher must preserve the original branch-local invocation command across the clean-env re-exec",
        )
        self.assertIn(
            'LAUNCHER_DISPATCH_COMMAND="$(quote_command "${BASH_BIN:-bash}" "$INNER_WRAPPER" "$@")"',
            WRAPPER_SOURCE,
            msg="launcher must seed a fallback dispatch-command snapshot before bash resolution completes",
        )
        self.assertIn(
            'capture_original_launcher_context "${launcher_args[@]}"',
            WRAPPER_SOURCE,
            msg="launcher must capture the original cwd/command before re-exec so failure bundles replay the user-facing invocation",
        )
        self.assertIn(
            'LAUNCHER_DISPATCH_COMMAND="$(quote_command "$BASH_BIN" "$INNER_WRAPPER" "${launcher_args[@]}")"',
            WRAPPER_SOURCE,
            msg="launcher must snapshot the exact inner-wrapper dispatch command before exec",
        )
        self.assertIn(
            'printf \'%s\\n\' "$LAUNCHER_INVOCATION_COMMAND" > "$LAUNCHER_FAILURE_INVOCATION_COMMAND_PATH"',
            WRAPPER_SOURCE,
            msg="launcher failure bundles must persist the invocation command into a stable artifact file",
        )
        self.assertIn(
            'printf \'%s\\n\' "$LAUNCHER_DISPATCH_COMMAND" > "$LAUNCHER_FAILURE_DISPATCH_COMMAND_PATH"',
            WRAPPER_SOURCE,
            msg="launcher failure bundles must persist the would-be dispatch command into a stable artifact file",
        )
        self.assertIn(
            'printf \'%s\\n\' "$message" > "$LAUNCHER_FAILURE_REASON_PATH"',
            WRAPPER_SOURCE,
            msg="launcher failure bundles must persist the exact failure reason into a stable artifact file",
        )
        self.assertIn(
            'printf \'%s\\n\' "$LAUNCHER_FAILURE_COMMAND" > "$LAUNCHER_FAILURE_COMMAND_PATH"',
            WRAPPER_SOURCE,
            msg="launcher failure bundles must persist the exact failing command when the ERR trap captures one",
        )
        self.assertIn(
            'write_launcher_environment_snapshot "$LAUNCHER_FAILURE_ENV_SNAPSHOT" || return 1',
            WRAPPER_SOURCE,
            msg="launcher failure bundles must persist the cleaned runtime environment snapshot",
        )
        self.assertIn(
            'echo "ORIGINAL_LAUNCH_PWD=$LAUNCHER_ORIGINAL_PWD"',
            WRAPPER_SOURCE,
            msg="launcher failure bundles must snapshot the original launch working directory alongside the cleaned environment",
        )
        self.assertIn(
            "write_launcher_preflight_manifest",
            WRAPPER_SOURCE,
            msg="launcher failure bundles must emit a manifest of command and path preflight status plus any related artifact path",
        )
        self.assertIn(
            'append_launcher_manifest_path_status directory "branch root directory" "$BRANCH_ROOT"',
            WRAPPER_SOURCE,
            msg="launcher failure manifests must include the normalized branch-root directory status",
        )
        self.assertIn(
            'append_launcher_manifest_path_status executable "launcher entrypoint" "$SELF_PATH"',
            WRAPPER_SOURCE,
            msg="launcher failure manifests must include the executable status of the branch-local launcher entrypoint",
        )
        self.assertIn(
            'append_launcher_manifest_path_status executable "outer smoke wrapper" "$INNER_WRAPPER"',
            WRAPPER_SOURCE,
            msg="launcher failure manifests must include the executable status of the outer smoke wrapper",
        )
        self.assertIn(
            'append_launcher_manifest_path_status executable "smoke target wrapper" "$SMOKE_TARGET_WRAPPER"',
            WRAPPER_SOURCE,
            msg="launcher failure manifests must include the executable status of the smoke target wrapper",
        )
        self.assertIn(
            "append_launcher_manifest_compiler_status",
            WRAPPER_SOURCE,
            msg="launcher failure manifests must include compiler availability so missing toolchains surface before dispatch",
        )
        self.assertIn(
            'append_launcher_manifest_path_status directory "resume workspace directory" "$RESUME_WORKSPACE_DIR"',
            WRAPPER_SOURCE,
            msg="launcher failure manifests must include the repo-root resume-workspace directory status",
        )
        self.assertIn(
            'echo "- Preflight manifest: \\`$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST\\`"',
            WRAPPER_SOURCE,
            msg="launcher failure reports must point the next iteration at the recorded preflight manifest",
        )
        self.assertIn(
            'echo "- Failure reason: \\`$LAUNCHER_FAILURE_REASON_PATH\\`"',
            WRAPPER_SOURCE,
            msg="launcher failure reports must point the next iteration at the preserved failure reason snapshot",
        )
        self.assertIn(
            'echo "- Artifact manifest: \\`$LAUNCHER_FAILURE_ARTIFACT_MANIFEST\\`"',
            WRAPPER_SOURCE,
            msg="launcher failure reports must point the next iteration at the bundle inventory",
        )
        self.assertIn(
            'echo "- Failure summary: \\`$LAUNCHER_FAILURE_SUMMARY\\`"',
            WRAPPER_SOURCE,
            msg="launcher failure reports must point the next iteration at the summary file that records the reason and artifact paths",
        )
        self.assertIn(
            'echo "## Last Recorded Check"',
            WRAPPER_SOURCE,
            msg="launcher failure reports must surface the most recent failing or validated preflight check",
        )
        self.assertIn(
            'echo "Invocation command:"',
            WRAPPER_SOURCE,
            msg="launcher failure reports must inline the exact invocation command for immediate replay/debugging",
        )
        self.assertIn(
            'echo "Intended inner-wrapper dispatch command:"',
            WRAPPER_SOURCE,
            msg="launcher failure reports must inline the intended dispatch command for pre-dispatch failures",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] invocation command: $LAUNCHER_INVOCATION_COMMAND"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must print the exact invocation command instead of only the snapshot path",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] intended dispatch command: $LAUNCHER_DISPATCH_COMMAND"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must print the exact dispatch command instead of only the snapshot path",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] failure reason snapshot: $LAUNCHER_FAILURE_REASON_PATH"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must point the next iteration at the preserved failure-reason file",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] artifact manifest: $LAUNCHER_FAILURE_ARTIFACT_MANIFEST"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must point the next iteration at the launcher artifact manifest",
        )

    def test_launcher_normalizes_inner_wrapper_outcomes_into_status_artifacts(self) -> None:
        self.assertIn(
            'LAUNCHER_STATUS_ROOT_DEFAULT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"',
            WRAPPER_SOURCE,
            msg="launcher must publish one stable status root for the public ./lca_smoke.sh outcome",
        )
        self.assertIn(
            'LAUNCHER_STATUS_ITERATION_EVIDENCE="$LAUNCHER_STATUS_ROOT/iteration_evidence.txt"',
            WRAPPER_SOURCE,
            msg="launcher must publish a stable stage-labeled iteration-evidence file under the public status root",
        )
        self.assertIn(
            'echo "public_status=$public_status"',
            WRAPPER_SOURCE,
            msg="launcher status summaries must publish a stable PASS/FAIL verdict for downstream repeatability tooling",
        )
        self.assertIn(
            'echo "result_family=$result_family"',
            WRAPPER_SOURCE,
            msg="launcher status summaries must distinguish solver, stress-gate, and harness failures",
        )
        self.assertIn(
            'echo "normalized_exit_code=$normalized_rc"',
            WRAPPER_SOURCE,
            msg="launcher status summaries must record the normalized public exit code",
        )
        self.assertIn(
            'echo "normalized_outcome=$outcome"',
            WRAPPER_SOURCE,
            msg="launcher status summaries must record the normalized public outcome family",
        )
        self.assertIn(
            'echo "outcome_source=$source_kind"',
            WRAPPER_SOURCE,
            msg="launcher status summaries must distinguish launcher failures from inner-wrapper results",
        )
        self.assertIn(
            'set_launcher_status \\',
            WRAPPER_SOURCE,
            msg="launcher must centralize status classification before writing the public status bundle",
        )
        self.assertIn(
            'reproducible_solver_failure',
            WRAPPER_SOURCE,
            msg="launcher must preserve a reproducible solver-failure family for preserved runtime or timeout failures",
        )
        self.assertIn(
            'reproducible_stress_gate_failure',
            WRAPPER_SOURCE,
            msg="launcher must preserve a distinct stress-gate family for preserved validator or acceptance failures",
        )
        self.assertIn(
            '"harness_infrastructure_failure" \\',
            WRAPPER_SOURCE,
            msg="launcher must collapse launcher and harness issues into one infrastructure-failure family",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] public status: ${LAUNCHER_STATUS_PUBLIC_STATUS:-FAIL} family=${LAUNCHER_STATUS_RESULT_FAMILY:-unknown}"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must expose the stable PASS/FAIL verdict together with the solver-vs-harness family",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] normalized outcome: $LAUNCHER_STATUS_OUTCOME"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must surface the normalized outcome family",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] outcome summary: $LAUNCHER_STATUS_MESSAGE"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must print the concise normalized summary without forcing the next iteration to open the status file first",
        )
        self.assertIn(
            'echo "iteration_evidence_path=$LAUNCHER_STATUS_ITERATION_EVIDENCE"',
            WRAPPER_SOURCE,
            msg="launcher status summaries must point directly at the stable iteration-evidence artifact",
        )
        self.assertIn(
            'echo "triage_stage=$triage_stage"',
            WRAPPER_SOURCE,
            msg="launcher failure status summaries must record the normalized failed stage for quick smoke triage",
        )
        self.assertIn(
            'echo "triage_stage_label=$triage_stage_label"',
            WRAPPER_SOURCE,
            msg="launcher failure status summaries must publish one combined stage label for lightweight iteration tooling",
        )
        self.assertIn(
            'echo "triage_retry_command=$triage_retry_command"',
            WRAPPER_SOURCE,
            msg="launcher failure status summaries must publish the next retry command",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] failed stage: $triage_stage scope=$triage_scope"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must surface the failed stage and whether it came from the launcher or the inner smoke case",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] inspect first: $triage_first_artifacts"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must point failed smoke runs at the primary artifacts/logs first",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] retry next: $triage_retry_command"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must show the next retry command explicitly",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] iteration evidence: $LAUNCHER_STATUS_ITERATION_EVIDENCE"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must point the next iteration at the stable evidence bundle without requiring markdown scraping",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] status summary: $LAUNCHER_STATUS_SUMMARY"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must point downstream tooling at the stable status summary",
        )
        self.assertIn(
            'emit_launcher_context_line "[lca_smoke] source failed-case row: $LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH"',
            WRAPPER_SOURCE,
            msg="launcher stderr diagnostics must surface the preserved failed-case row directly when the inner wrapper recorded one",
        )

    def test_launcher_waits_for_inner_wrapper_then_exits_with_normalized_public_code(self) -> None:
        self.assertIn(
            '"$BASH_BIN" "$INNER_WRAPPER" "$@"',
            WRAPPER_SOURCE,
            msg="launcher must keep one dedicated helper that invokes the inner wrapper as a child process",
        )
        self.assertIn(
            "# Keep inner-wrapper failures on the handled normalization path instead of",
            WRAPPER_SOURCE,
            msg=(
                "launcher dispatch must document that handled inner-wrapper failures stay on the "
                "normalization path instead of tripping launcher-side errexit"
            ),
        )
        self.assertIn(
            'if run_inner_wrapper_dispatch "${launcher_args[@]}"; then',
            WRAPPER_SOURCE,
            msg="launcher main must treat inner-wrapper exits as handled child outcomes via the dedicated dispatch helper",
        )
        self.assertIn(
            "inner_wrapper_rc=0",
            WRAPPER_SOURCE,
            msg="launcher must preserve the successful inner-wrapper exit code explicitly in the handled dispatch branch",
        )
        self.assertIn(
            "inner_wrapper_rc=$?",
            WRAPPER_SOURCE,
            msg="launcher must capture the raw inner-wrapper exit code in the handled dispatch branch",
        )
        self.assertIn(
            'if (( LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED == 1 )); then',
            WRAPPER_SOURCE,
            msg="launcher must distinguish timeout-intervened dispatches from ordinary inner-wrapper exits",
        )
        self.assertIn(
            '"dispatch" \\',
            WRAPPER_SOURCE,
            msg="launcher must preserve dispatch bookkeeping through the timeout-aware last-check branch",
        )
        self.assertIn(
            '"returned" \\',
            WRAPPER_SOURCE,
            msg="launcher must preserve the raw inner-wrapper exit code in its last-check context when dispatch completes normally",
        )
        self.assertIn(
            '"timeout" \\',
            WRAPPER_SOURCE,
            msg="launcher must record a stable timeout status when the launcher has to kill a hung inner-wrapper dispatch",
        )
        self.assertIn(
            'classify_inner_wrapper_exit "$inner_wrapper_rc"',
            WRAPPER_SOURCE,
            msg="launcher must normalize the inner-wrapper result before exiting",
        )
        self.assertIn(
            'finalize_launcher_dispatch_result',
            WRAPPER_SOURCE,
            msg="launcher must route handled inner-wrapper results through a dedicated finalization step",
        )
        self.assertIn(
            'LAUNCHER_SKIP_FAILURE_BUNDLE=1',
            WRAPPER_SOURCE,
            msg="launcher must suppress launcher-failure bundling after a handled inner-wrapper result has been normalized",
        )
        self.assertIn(
            'trap - ERR',
            WRAPPER_SOURCE,
            msg="launcher dispatch finalization must disable the ERR trap before best-effort status publishing",
        )
        self.assertIn(
            'write_launcher_status_bundle',
            WRAPPER_SOURCE,
            msg="launcher dispatch finalization must still attempt to publish the normalized status bundle",
        )
        self.assertIn(
            'report_launcher_status_context',
            WRAPPER_SOURCE,
            msg="launcher dispatch finalization must still attempt to print the normalized status context",
        )
        self.assertIn(
            'warning: failed to publish launcher status bundle after dispatch normalization; preserving normalized exit code',
            WRAPPER_SOURCE,
            msg="launcher must warn instead of replacing the classified smoke outcome when post-dispatch status publication fails",
        )
        self.assertIn(
            'warning: failed to print launcher status context after dispatch normalization; preserving normalized exit code',
            WRAPPER_SOURCE,
            msg="launcher must warn instead of replacing the classified smoke outcome when post-dispatch status reporting fails",
        )
        self.assertIn(
            'LAUNCHER_SKIP_FAILURE_BUNDLE=1',
            WRAPPER_SOURCE,
            msg="launcher must suppress launcher-failure bundling after a handled inner-wrapper result has been normalized",
        )
        self.assertIn(
            'exit "$normalized_rc"',
            WRAPPER_SOURCE,
            msg="launcher dispatch finalization must exit with the already-classified public code even if later reporting steps fail",
        )
        self.assertIn(
            'if (( rc != 0 )) && (( LAUNCHER_SKIP_FAILURE_BUNDLE == 0 )); then',
            WRAPPER_SOURCE,
            msg="launcher cleanup must only publish launcher-failure bundles for genuine launcher failures",
        )
        self.assertIn(
            'rc="$LAUNCHER_STATUS_NORMALIZED_RC"',
            WRAPPER_SOURCE,
            msg="launcher cleanup must exit with the normalized public harness code after publishing a launcher failure bundle",
        )
        self.assertIn(
            'finalize_launcher_dispatch_result',
            WRAPPER_SOURCE,
            msg="launcher must delegate final post-dispatch exit handling to the normalized-result finalizer",
        )

    def test_launcher_dispatch_monitor_failure_captures_real_manager_exit_code(self) -> None:
        self.assertNotIn(
            'if ! python3 - "$LAUNCHER_DISPATCH_RESULT_PATH"',
            WRAPPER_SOURCE,
            msg="dispatch monitor failures must not use a negated if-branch that collapses the real monitor exit code",
        )
        self.assertIn(
            'then\n    :\n  else\n    manager_rc=$?',
            WRAPPER_SOURCE,
            msg="dispatch monitor failures must capture the Python helper exit code in the non-negated else branch",
        )
        pattern = re.compile(
            r'run_inner_wrapper_dispatch\(\)\s*\{.*?manager_rc=\$\?.*?set_launcher_failure_stage "dispatch_monitor".*?fail "inner wrapper dispatch monitor failed with exit code \$manager_rc"',
            re.DOTALL,
        )
        self.assertRegex(
            WRAPPER_SOURCE,
            pattern,
            msg="dispatch monitor failures must classify the failing stage after preserving the real manager exit code",
        )

    def test_launcher_surfaces_inner_failure_replay_details_in_status_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed123"
                mkdir -p "$CASE_DIR"
                : > "$FAILURE_ROOT/commands.txt"
                : > "$FAILURE_ROOT/artifact_manifest.tsv"
                : > "$FAILURE_ROOT/rerun_command.txt"
                : > "$FAILURE_ROOT/seed.txt"
                : > "$FAILURE_ROOT/input.txt"
                : > "$FAILURE_ROOT/solver_output.txt"
                : > "$FAILURE_ROOT/expected_output.txt"
                : > "$FAILURE_ROOT/invoked_command.txt"
                printf '{"schema":"lca_smoke_failure_context_v1"}\n' > "$FAILURE_ROOT/failure_context.json"
                printf 'VAR=1\n' > "$FAILURE_ROOT/runtime_env.txt"
                printf 'manifest\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
                printf 'case_count=1\n' > "$FAILURE_ROOT/suite_config.txt"
                printf 'case_index\tcase_tag\n1\tsmoke_comb_core_n64_seed123\n' > "$FAILURE_ROOT/suite_plan.tsv"
                : > "$FAILURE_ROOT/replay_active_manifest_case.sh"
                chmod +x "$FAILURE_ROOT/replay_active_manifest_case.sh"
                {
                  printf '%s\n' 'failure_summary=validator mismatch on preserved query 7'
                  printf '%s\n' 'failure_reporting_status=complete'
                  printf '%s\n' 'failure_reporting_warning='
                  printf '%s\n' 'failed_case_tag=smoke_comb_core_n64_seed123'
                  printf '%s\n' 'failed_stage=smoke'
                  printf '%s\n' 'failed_mode=comb_core'
                  printf '%s\n' 'failed_n=64'
                  printf '%s\n' 'failed_seed=123'
                  printf '%s\n' "failure_root=$FAILURE_ROOT"
                  printf '%s\n' "failure_case_dir=$CASE_DIR"
                  printf '%s\n' "commands_path=$FAILURE_ROOT/commands.txt"
                  printf '%s\n' "artifact_manifest_path=$FAILURE_ROOT/artifact_manifest.tsv"
                  printf '%s\n' "structured_context_path=$FAILURE_ROOT/failure_context.json"
                  printf '%s\n' "rerun_command_path=$FAILURE_ROOT/rerun_command.txt"
                  printf '%s\n' "exact_seed_path=$FAILURE_ROOT/seed.txt"
                  printf '%s\n' "exact_input_path=$FAILURE_ROOT/input.txt"
                  printf '%s\n' "exact_output_path=$FAILURE_ROOT/solver_output.txt"
                  printf '%s\n' "expected_output_path=$FAILURE_ROOT/expected_output.txt"
                  printf '%s\n' "invoked_command_path=$FAILURE_ROOT/invoked_command.txt"
                  printf '%s\n' "runtime_env_path=$FAILURE_ROOT/runtime_env.txt"
                  printf '%s\n' "manifest_snapshot_path=$FAILURE_ROOT/smoke_cases_manifest.tsv"
                  printf '%s\n' "suite_config_path=$FAILURE_ROOT/suite_config.txt"
                  printf '%s\n' "suite_plan_path=$FAILURE_ROOT/suite_plan.tsv"
                  printf '%s\n' "active_solver_replay_script=$FAILURE_ROOT/replay_active_manifest_case.sh"
                } > "$FAILURE_ROOT/failure_summary.txt"
                printf '# inner failure report\\n' > "$FAILURE_ROOT/latest_failure_report.md"
                exit 1
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 1, msg=result.stderr)
            failure_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure").resolve()
            case_dir = (failure_root / "smoke_comb_core_n64_seed123").resolve()
            replay_script = (failure_root / "replay_active_manifest_case.sh").resolve()
            replay_command = self.expected_bash_command(replay_script)
            launcher_failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_launcher_latest_failure"
            status_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status"
            iteration_evidence = (status_root / "iteration_evidence.txt").resolve()
            smoke_iteration_evidence = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "iteration_evidence.txt"
            ).resolve()
            status_summary = (status_root / "summary.txt").read_text(encoding="utf-8")
            status_report = (status_root / "latest_status_report.md").read_text(encoding="utf-8")
            diagnostics_manifest = (status_root / "diagnostics_manifest.tsv").resolve()
            iteration_evidence_text = iteration_evidence.read_text(encoding="utf-8")

            self.assertFalse(
                launcher_failure_root.exists(),
                msg="handled inner-wrapper solver failures must not publish a launcher-pre-dispatch failure bundle",
            )
            self.assertIn("public_status=FAIL", status_summary)
            self.assertIn("acceptance_signal_status=FAIL", status_summary)
            self.assertIn("iteration_support_status=ACTIONABLE", status_summary)
            self.assertIn("iteration_support_next_step=retry", status_summary)
            self.assertIn("result_family=stress_gate", status_summary)
            self.assertIn("failure_partition=solver_test", status_summary)
            self.assertIn("normalized_outcome=reproducible_stress_gate_failure", status_summary)
            self.assertIn("source_failure_summary=validator mismatch on preserved query 7", status_summary)
            self.assertIn(f"iteration_evidence_path={iteration_evidence}", status_summary)
            self.assertIn(
                f"published_smoke_iteration_evidence_path={smoke_iteration_evidence}",
                status_summary,
            )
            self.assertIn("triage_stage_scope=inner_wrapper_case", status_summary)
            self.assertIn("triage_stage=smoke", status_summary)
            self.assertIn("triage_stage_label=inner_wrapper_case:smoke", status_summary)
            self.assertNotIn("triage_stage_scope=launcher_pre_dispatch", status_summary)
            self.assertIn(f"triage_primary_summary={failure_root / 'failure_summary.txt'}", status_summary)
            self.assertIn(f"triage_primary_report={failure_root / 'latest_failure_report.md'}", status_summary)
            self.assertIn(f"triage_primary_manifest={diagnostics_manifest}", status_summary)
            self.assertIn(f"triage_retry_command={replay_command}", status_summary)
            self.assertIn(
                f"triage_retry_hint=reproduce the preserved failing smoke case at stage smoke via {replay_command}",
                status_summary,
            )
            self.assertIn(
                "source_failure_case=tag=smoke_comb_core_n64_seed123 stage=smoke mode=comb_core n=64 seed=123",
                status_summary,
            )
            self.assertIn("source_failure_seed=123", status_summary)
            self.assertIn(
                f"source_failure_replay_command={replay_command}",
                status_summary,
            )
            self.assertIn(
                "source_failure_artifacts="
                f"failure_root={failure_root} "
                f"case_dir={case_dir} "
                f"commands={failure_root / 'commands.txt'} "
                f"rerun_commands={failure_root / 'rerun_command.txt'} "
                f"artifact_manifest={failure_root / 'artifact_manifest.tsv'}",
                status_summary,
            )
            self.assertIn(
                f"runtime_env={failure_root / 'runtime_env.txt'}",
                status_summary,
            )
            self.assertIn(
                f"structured_context={failure_root / 'failure_context.json'}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_expected_output_path={failure_root / 'expected_output.txt'}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_invoked_command_path={failure_root / 'invoked_command.txt'}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_structured_context_path={failure_root / 'failure_context.json'}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_runtime_env_path={failure_root / 'runtime_env.txt'}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_manifest_snapshot_path={failure_root / 'smoke_cases_manifest.tsv'}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_suite_config_path={failure_root / 'suite_config.txt'}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_suite_plan_path={failure_root / 'suite_plan.tsv'}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_active_solver_replay_script={failure_root / 'replay_active_manifest_case.sh'}",
                status_summary,
            )
            self.assertIn("- Public status: `FAIL`", status_report)
            self.assertIn("- Result family: `stress_gate`", status_report)
            self.assertIn("- Failure partition: `solver/test`", status_report)
            self.assertIn("- Normalized outcome: `reproducible_stress_gate_failure`", status_report)
            self.assertIn("## Acceptance Signal", status_report)
            self.assertIn("- Acceptance status: `FAIL`", status_report)
            self.assertIn(
                "- Acceptance summary: `smoke did not satisfy AC2 on this working tree; keep the failure visible and do not treat this run as formal gate closure`",
                status_report,
            )
            self.assertIn("## Iteration Support", status_report)
            self.assertIn("- Iteration support: `ACTIONABLE`", status_report)
            self.assertIn("- Next step: `retry`", status_report)
            self.assertIn(
                "- Iteration summary: `stable smoke status and retry artifacts are published despite the failed acceptance signal; inspect them and continue the next debugging pass`",
                status_report,
            )
            self.assertIn("## Failed Stage", status_report)
            self.assertIn("- Failed stage scope: `inner_wrapper_case`", status_report)
            self.assertIn("- Failed stage: `smoke`", status_report)
            self.assertIn("- Stage label: `inner_wrapper_case:smoke`", status_report)
            self.assertIn(f"- Iteration evidence: `{iteration_evidence}`", status_report)
            self.assertNotIn("launcher_pre_dispatch", status_report)
            self.assertIn(f"- Primary report: `{failure_root / 'latest_failure_report.md'}`", status_report)
            self.assertIn(f"- Primary manifest: `{diagnostics_manifest}`", status_report)
            self.assertIn(
                f"- Smoke iteration-evidence mirror: `{smoke_iteration_evidence}`",
                status_report,
            )
            self.assertIn("- Concise replay summary: `validator mismatch on preserved query 7`", status_report)
            self.assertIn(
                "- Failing case: `tag=smoke_comb_core_n64_seed123 stage=smoke mode=comb_core n=64 seed=123`",
                status_report,
            )
            self.assertIn(f"- Preferred replay command: `{replay_command}`", status_report)
            self.assertIn(f"- Failure root: `{failure_root}`", status_report)
            self.assertIn(f"- Expected output snapshot: `{failure_root / 'expected_output.txt'}`", status_report)
            self.assertIn(f"- Invoked command snapshot: `{failure_root / 'invoked_command.txt'}`", status_report)
            self.assertIn(f"- Artifact manifest: `{failure_root / 'artifact_manifest.tsv'}`", status_report)
            self.assertIn(f"- Active solver replay script: `{failure_root / 'replay_active_manifest_case.sh'}`", status_report)
            self.assertIn(f"- Structured failure context: `{failure_root / 'failure_context.json'}`", status_report)
            self.assertIn(f"- Runtime env snapshot: `{failure_root / 'runtime_env.txt'}`", status_report)
            self.assertIn(f"- Manifest snapshot: `{failure_root / 'smoke_cases_manifest.tsv'}`", status_report)
            self.assertIn(f"- Suite config snapshot: `{failure_root / 'suite_config.txt'}`", status_report)
            self.assertIn(f"- Suite plan snapshot: `{failure_root / 'suite_plan.tsv'}`", status_report)
            self.assertIn("## Retry Next", status_report)
            self.assertIn(f"- Retry command: `{replay_command}`", status_report)
            self.assertIn("stage_scope=inner_wrapper_case", iteration_evidence_text)
            self.assertIn("stage=smoke", iteration_evidence_text)
            self.assertIn("stage_label=inner_wrapper_case:smoke", iteration_evidence_text)
            self.assertIn(f"primary_report={failure_root / 'latest_failure_report.md'}", iteration_evidence_text)
            self.assertIn(f"retry_command={replay_command}", iteration_evidence_text)
            self.assertIn(
                f"published_smoke_iteration_evidence_path={smoke_iteration_evidence}",
                iteration_evidence_text,
            )
            self.assertIn("[lca_smoke] public status: FAIL family=stress_gate", result.stderr)
            self.assertIn("[lca_smoke] acceptance signal: FAIL", result.stderr)
            self.assertIn("[lca_smoke] iteration support: ACTIONABLE next_step=retry", result.stderr)
            self.assertIn("[lca_smoke] normalized outcome: reproducible_stress_gate_failure", result.stderr)
            self.assertIn(
                "[lca_smoke] failure partition: solver/test public_exit=1 raw_exit=1",
                result.stderr,
            )
            self.assertIn(
                "[lca_smoke] outcome summary: inner smoke wrapper failed at stage smoke: validator mismatch on preserved query 7",
                result.stderr,
            )
            self.assertIn("[lca_smoke] failed stage: smoke scope=inner_wrapper_case", result.stderr)
            self.assertIn("[lca_smoke] stage label: inner_wrapper_case:smoke", result.stderr)
            self.assertIn(f"[lca_smoke] primary report: {failure_root / 'latest_failure_report.md'}", result.stderr)
            self.assertIn(f"[lca_smoke] retry next: {replay_command}", result.stderr)
            self.assertIn("[lca_smoke] replay summary: validator mismatch on preserved query 7", result.stderr)
            self.assertIn(f"[lca_smoke] iteration evidence: {iteration_evidence}", result.stderr)
            self.assertNotIn("launcher failed before inner wrapper dispatch", result.stderr)
            self.assertIn(
                "[lca_smoke] replay case: tag=smoke_comb_core_n64_seed123 stage=smoke mode=comb_core n=64 seed=123",
                result.stderr,
            )
            self.assertIn(f"[lca_smoke] replay command: {replay_command}", result.stderr)
            self.assertIn(f"[lca_smoke] commands snapshot: {failure_root / 'commands.txt'}", result.stderr)
            self.assertIn(
                f"[lca_smoke] source artifact manifest: {failure_root / 'artifact_manifest.tsv'}",
                result.stderr,
            )
            self.assertIn(f"[lca_smoke] exact seed snapshot: {failure_root / 'seed.txt'}", result.stderr)
            self.assertIn(f"[lca_smoke] exact input snapshot: {failure_root / 'input.txt'}", result.stderr)
            self.assertIn(f"[lca_smoke] exact output snapshot: {failure_root / 'solver_output.txt'}", result.stderr)
            self.assertIn(
                f"[lca_smoke] expected output snapshot: {failure_root / 'expected_output.txt'}",
                result.stderr,
            )
            self.assertIn(
                f"[lca_smoke] invoked command snapshot: {failure_root / 'invoked_command.txt'}",
                result.stderr,
            )
            self.assertIn(
                f"[lca_smoke] active solver replay script: {failure_root / 'replay_active_manifest_case.sh'}",
                result.stderr,
            )
            self.assertIn(f"[lca_smoke] source runtime env: {failure_root / 'runtime_env.txt'}", result.stderr)
            self.assertIn(
                f"[lca_smoke] source manifest snapshot: {failure_root / 'smoke_cases_manifest.tsv'}",
                result.stderr,
            )
            self.assertIn(f"[lca_smoke] suite config: {failure_root / 'suite_config.txt'}", result.stderr)
            self.assertIn(
                "[lca_smoke] replay artifacts: "
                f"failure_root={failure_root} "
                f"case_dir={case_dir} "
                f"commands={failure_root / 'commands.txt'} "
                f"rerun_commands={failure_root / 'rerun_command.txt'} "
                f"artifact_manifest={failure_root / 'artifact_manifest.tsv'} "
                f"manifest_snapshot={failure_root / 'smoke_cases_manifest.tsv'} "
                f"suite_config={failure_root / 'suite_config.txt'} "
                f"suite_plan={failure_root / 'suite_plan.tsv'} "
                f"exact_seed={failure_root / 'seed.txt'} "
                f"exact_input={failure_root / 'input.txt'} "
                f"exact_output={failure_root / 'solver_output.txt'} "
                f"expected_output={failure_root / 'expected_output.txt'} "
                f"invoked_command={failure_root / 'invoked_command.txt'} "
                f"active_solver_replay_script={failure_root / 'replay_active_manifest_case.sh'} "
                f"runtime_env={failure_root / 'runtime_env.txt'} "
                f"structured_context={failure_root / 'failure_context.json'}",
                result.stderr,
            )

    def test_launcher_canonicalizes_prefixed_failure_artifact_paths_from_inner_summary(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed123"
                mkdir -p "$CASE_DIR"
                : > "$FAILURE_ROOT/commands.txt"
                : > "$FAILURE_ROOT/artifact_manifest.tsv"
                : > "$FAILURE_ROOT/rerun_command.txt"
                : > "$FAILURE_ROOT/seed.txt"
                : > "$FAILURE_ROOT/input.txt"
                : > "$FAILURE_ROOT/solver_output.txt"
                : > "$FAILURE_ROOT/expected_output.txt"
                : > "$FAILURE_ROOT/invoked_command.txt"
                printf '{"schema":"lca_smoke_failure_context_v1"}\n' > "$FAILURE_ROOT/failure_context.json"
                printf 'VAR=1\n' > "$FAILURE_ROOT/runtime_env.txt"
                printf 'manifest\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
                printf 'case_count=1\n' > "$FAILURE_ROOT/suite_config.txt"
                printf 'case_index\tcase_tag\n1\tsmoke_comb_core_n64_seed123\n' > "$FAILURE_ROOT/suite_plan.tsv"
                : > "$FAILURE_ROOT/replay_active_manifest_case.sh"
                chmod +x "$FAILURE_ROOT/replay_active_manifest_case.sh"
                cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
                failure_summary=validator mismatch on prefixed replay paths
                failure_reporting_status=complete
                failure_reporting_warning=
                failed_case_tag=smoke_comb_core_n64_seed123
                failed_stage=smoke
                failed_mode=comb_core
                failed_n=64
                failed_seed=123
                failure_root=artifacts/artifacts/lca_tree_stress_v5/smoke_latest_failure
                failure_case_dir=branch/artifacts/lca_tree_stress_v5/smoke_latest_failure/smoke_comb_core_n64_seed123
                commands_path=branch/artifacts/lca_tree_stress_v5/smoke_latest_failure/commands.txt
                artifact_manifest_path=artifacts/lca_tree_stress_v5/smoke_latest_failure/artifact_manifest.tsv
                structured_context_path=artifacts/artifacts/lca_tree_stress_v5/smoke_latest_failure/failure_context.json
                rerun_command_path=artifacts/artifacts/lca_tree_stress_v5/smoke_latest_failure/rerun_command.txt
                exact_seed_path=branch/artifacts/lca_tree_stress_v5/smoke_latest_failure/seed.txt
                exact_input_path=artifacts/lca_tree_stress_v5/smoke_latest_failure/input.txt
                exact_output_path=artifacts/artifacts/lca_tree_stress_v5/smoke_latest_failure/solver_output.txt
                expected_output_path=branch/artifacts/lca_tree_stress_v5/smoke_latest_failure/expected_output.txt
                invoked_command_path=artifacts/artifacts/lca_tree_stress_v5/smoke_latest_failure/invoked_command.txt
                runtime_env_path=artifacts/lca_tree_stress_v5/smoke_latest_failure/runtime_env.txt
                manifest_snapshot_path=branch/artifacts/lca_tree_stress_v5/smoke_latest_failure/smoke_cases_manifest.tsv
                suite_config_path=artifacts/artifacts/lca_tree_stress_v5/smoke_latest_failure/suite_config.txt
                suite_plan_path=branch/artifacts/lca_tree_stress_v5/smoke_latest_failure/suite_plan.tsv
                active_solver_replay_script=branch/artifacts/lca_tree_stress_v5/smoke_latest_failure/replay_active_manifest_case.sh
                EOF
                printf '# prefixed failure report\\n' > "$FAILURE_ROOT/latest_failure_report.md"
                exit 1
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 1, msg=result.stderr)
            failure_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure").resolve()
            status_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status"
            status_summary = (status_root / "summary.txt").read_text(encoding="utf-8")
            status_report = (status_root / "latest_status_report.md").read_text(encoding="utf-8")
            diagnostics_manifest = (status_root / "diagnostics_manifest.tsv").read_text(encoding="utf-8")
            replay_script = (failure_root / "replay_active_manifest_case.sh").resolve()
            replay_command = self.expected_bash_command(replay_script)

            self.assertIn(
                f"source_failure_artifacts=failure_root={failure_root} ",
                status_summary,
            )
            self.assertIn(
                f"exact_seed={failure_root / 'seed.txt'} "
                f"exact_input={failure_root / 'input.txt'} "
                f"exact_output={failure_root / 'solver_output.txt'} "
                f"expected_output={failure_root / 'expected_output.txt'} "
                f"invoked_command={failure_root / 'invoked_command.txt'}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_expected_output_path={failure_root / 'expected_output.txt'}",
                status_summary,
            )
            self.assertIn(
                f"- Expected output snapshot: `{failure_root / 'expected_output.txt'}`",
                status_report,
            )
            self.assertIn(
                f"- Preferred replay command: `{replay_command}`",
                status_report,
            )
            self.assertIn(
                f"source_failure_expected_output\t{failure_root / 'expected_output.txt'}\t1\t",
                diagnostics_manifest,
            )
            self.assertIn(
                f"[lca_smoke] replay command: {replay_command}",
                result.stderr,
            )
            self.assertNotIn(
                "failure_root=artifacts/artifacts/lca_tree_stress_v5/smoke_latest_failure",
                status_summary,
            )
            self.assertNotIn(
                "source_failure_expected_output_path=branch/artifacts/lca_tree_stress_v5/smoke_latest_failure/expected_output.txt",
                status_summary,
            )

    def test_launcher_complete_timeout_failure_bundle_normalizes_to_solver_failure_without_launcher_abort(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed123"
                mkdir -p "$CASE_DIR"
                : > "$FAILURE_ROOT/commands.txt"
                : > "$FAILURE_ROOT/artifact_manifest.tsv"
                : > "$FAILURE_ROOT/rerun_command.txt"
                : > "$FAILURE_ROOT/seed.txt"
                : > "$FAILURE_ROOT/input.txt"
                : > "$FAILURE_ROOT/solver_output.txt"
                : > "$FAILURE_ROOT/expected_output.txt"
                : > "$FAILURE_ROOT/invoked_command.txt"
                printf '{"schema":"lca_smoke_failure_context_v1"}\n' > "$FAILURE_ROOT/failure_context.json"
                printf 'VAR=1\n' > "$FAILURE_ROOT/runtime_env.txt"
                printf 'manifest\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
                printf 'case_count=1\n' > "$FAILURE_ROOT/suite_config.txt"
                printf 'case_index\tcase_tag\n1\tsmoke_comb_core_n64_seed123\n' > "$FAILURE_ROOT/suite_plan.tsv"
                : > "$FAILURE_ROOT/replay_active_manifest_case.sh"
                chmod +x "$FAILURE_ROOT/replay_active_manifest_case.sh"
                cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
                failure_summary=solver timed out on preserved query 7
                failure_kind=solver_timeout
                failure_origin=solver
                failure_retryable=1
                failure_reporting_status=complete
                failure_reporting_warning=
                failed_case_tag=smoke_comb_core_n64_seed123
                failed_stage=smoke
                failed_mode=comb_core
                failed_n=64
                failed_seed=123
                failure_root=$FAILURE_ROOT
                failure_case_dir=$CASE_DIR
                commands_path=$FAILURE_ROOT/commands.txt
                artifact_manifest_path=$FAILURE_ROOT/artifact_manifest.tsv
                structured_context_path=$FAILURE_ROOT/failure_context.json
                rerun_command_path=$FAILURE_ROOT/rerun_command.txt
                exact_seed_path=$FAILURE_ROOT/seed.txt
                exact_input_path=$FAILURE_ROOT/input.txt
                exact_output_path=$FAILURE_ROOT/solver_output.txt
                expected_output_path=$FAILURE_ROOT/expected_output.txt
                invoked_command_path=$FAILURE_ROOT/invoked_command.txt
                runtime_env_path=$FAILURE_ROOT/runtime_env.txt
                manifest_snapshot_path=$FAILURE_ROOT/smoke_cases_manifest.tsv
                suite_config_path=$FAILURE_ROOT/suite_config.txt
                suite_plan_path=$FAILURE_ROOT/suite_plan.tsv
                active_solver_replay_script=$FAILURE_ROOT/replay_active_manifest_case.sh
                EOF
                printf '# timeout failure report\\n' > "$FAILURE_ROOT/latest_failure_report.md"
                exit 124
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(
                result.returncode,
                124,
                msg="complete smoke timeout bundles must preserve the public solver-timeout exit code",
            )
            status_summary = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            ).read_text(encoding="utf-8")
            self.assertIn("public_status=FAIL", status_summary)
            self.assertIn("result_family=solver", status_summary)
            self.assertIn("raw_exit_code=124", status_summary)
            self.assertIn("normalized_exit_code=124", status_summary)
            self.assertIn("normalized_outcome=reproducible_solver_failure", status_summary)
            self.assertIn("triage_stage_scope=inner_wrapper_case", status_summary)
            self.assertIn("triage_stage=smoke", status_summary)
            replay_script = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "smoke_latest_failure"
                / "replay_active_manifest_case.sh"
            )
            self.assertIn(
                f"triage_retry_command={self.expected_bash_command(replay_script)}",
                status_summary,
            )
            self.assertIn("[lca_smoke] public status: FAIL family=solver", result.stderr)
            self.assertIn("[lca_smoke] normalized outcome: reproducible_solver_failure", result.stderr)
            self.assertIn("[lca_smoke] normalized exit code: 124 raw_exit_code=124 source=inner_wrapper", result.stderr)
            self.assertNotIn("[lca_smoke] launcher failed before inner wrapper dispatch", result.stderr)
            self.assertFalse(
                (
                    branch_root
                    / "artifacts"
                    / "lca_tree_stress_v5"
                    / "smoke_launcher_latest_failure"
                    / "failure_summary.txt"
                ).exists(),
                msg="handled inner-wrapper timeouts must not be reclassified as launcher failures",
            )

    def test_launcher_status_bundle_indexes_pass_diagnostics_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                mkdir -p "$SMOKE_ROOT/environment_validation"
                printf 'case_count=1\\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed1\\n' > "$SMOKE_ROOT/suite_plan.tsv"
                printf 'validation=ok\\n' > "$SMOKE_ROOT/environment_validation.txt"
                printf 'kind\\tlabel\\tstatus\\n' > "$SMOKE_ROOT/environment_validation/preflight_manifest.tsv"
                printf 'PATH=/usr/bin\\n' > "$SMOKE_ROOT/environment_validation/setup_env.txt"
                printf './build.sh --out solve\\n' > "$SMOKE_ROOT/environment_validation/build.command.txt"
                printf 'stage\\tmode\\tn\\tseed\\tshuffle_labels\\tshuffle_queries\\ttimeout_s\\nsmoke\\tcomb_core\\t64\\t1\\t1\\t1\\t2\\n' > "$SMOKE_ROOT/environment_validation/smoke_cases.snapshot.tsv"
                exit 0
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            status_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status"
            diagnostics_manifest = (status_root / "diagnostics_manifest.tsv").resolve()
            iteration_evidence = (status_root / "iteration_evidence.txt").resolve()
            suite_config = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "suite_config.txt").resolve()
            suite_plan = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "suite_plan.tsv").resolve()
            env_report = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "environment_validation.txt").resolve()
            env_manifest = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "environment_validation" / "preflight_manifest.tsv").resolve()
            build_command = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "environment_validation" / "build.command.txt").resolve()
            manifest_snapshot = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "environment_validation" / "smoke_cases.snapshot.tsv").resolve()
            smoke_summary = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "summary.txt").resolve()
            smoke_report = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "status_report.md").resolve()
            smoke_iteration_evidence = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "iteration_evidence.txt").resolve()
            smoke_diagnostics = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "diagnostics_manifest.tsv").resolve()
            smoke_standard_gap = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "standard_gap.json").resolve()
            run_history_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_run_history").resolve()
            run_history_index = (run_history_root / "history.tsv").resolve()
            run_archives = sorted(run_history_root.glob("run.*"))
            self.assertEqual(len(run_archives), 1, msg="each launcher run must preserve one per-run smoke archive")
            run_archive = run_archives[0].resolve()
            run_console = (run_archive / "console.stderr.txt").resolve()
            run_iteration_evidence = (run_archive / "iteration_evidence.txt").resolve()
            run_record = (status_root / "run_record.json").resolve()
            run_comparison = (status_root / "run_comparison.json").resolve()
            smoke_run_record = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "run_record.json").resolve()
            smoke_run_comparison = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "run_comparison.json").resolve()
            run_archive_record = (run_archive / "run_record.json").resolve()
            run_archive_comparison = (run_archive / "run_comparison.json").resolve()
            run_manifest = (run_archive / "artifact_manifest.tsv").resolve()
            status_summary = (status_root / "summary.txt").read_text(encoding="utf-8")
            status_report = (status_root / "latest_status_report.md").read_text(encoding="utf-8")
            diagnostics_text = diagnostics_manifest.read_text(encoding="utf-8")
            smoke_summary_text = smoke_summary.read_text(encoding="utf-8")
            smoke_gap_payload = json.loads(smoke_standard_gap.read_text(encoding="utf-8"))
            smoke_gap_gate_chain = {entry["ac"]: entry for entry in smoke_gap_payload["gate_chain"]}
            run_record_payload = json.loads(run_record.read_text(encoding="utf-8"))
            run_record_gate_chain = {entry["ac"]: entry for entry in run_record_payload["gate_chain"]}
            run_comparison_payload = json.loads(run_comparison.read_text(encoding="utf-8"))

            self.assertIn("required_standard=lca_tree_stress_v5", status_summary)
            self.assertIn("acceptance_signal_status=PASS", status_summary)
            self.assertIn("iteration_support_status=ACTIONABLE", status_summary)
            self.assertIn("iteration_support_next_step=gate_escalation", status_summary)
            self.assertIn("standard_gap_status=ready_for_gate_escalation", status_summary)
            self.assertIn("gate_chain_ac2_status=satisfied", status_summary)
            self.assertIn("gate_chain_ac3_status=ready_to_run", status_summary)
            self.assertIn("gate_chain_ac4_status=pending_after_ac3", status_summary)
            self.assertIn("gate_chain_ac5_status=blocked_by_ac3", status_summary)
            self.assertIn("gate_chain_ac6_status=blocked_by_ac5", status_summary)
            self.assertIn(f"iteration_evidence_path={iteration_evidence}", status_summary)
            self.assertIn(f"status_diagnostics_manifest={diagnostics_manifest}", status_summary)
            self.assertIn(f"run_history_index_path={run_history_index}", status_summary)
            self.assertIn(f"run_record_path={run_record}", status_summary)
            self.assertIn(f"run_comparison_path={run_comparison}", status_summary)
            self.assertIn(f"published_smoke_summary_path={smoke_summary}", status_summary)
            self.assertIn(f"published_smoke_status_report_path={smoke_report}", status_summary)
            self.assertIn(f"published_smoke_iteration_evidence_path={smoke_iteration_evidence}", status_summary)
            self.assertIn(f"published_smoke_diagnostics_manifest_path={smoke_diagnostics}", status_summary)
            self.assertIn(f"published_smoke_standard_gap_json_path={smoke_standard_gap}", status_summary)
            self.assertIn(f"published_smoke_run_record_path={smoke_run_record}", status_summary)
            self.assertIn(f"published_smoke_run_comparison_path={smoke_run_comparison}", status_summary)
            self.assertIn(f"smoke_suite_config_path={suite_config}", status_summary)
            self.assertIn(f"smoke_suite_plan_path={suite_plan}", status_summary)
            self.assertIn(f"smoke_environment_validation_report={env_report}", status_summary)
            self.assertIn(f"smoke_environment_preflight_manifest_path={env_manifest}", status_summary)
            self.assertIn(f"smoke_environment_build_command_path={build_command}", status_summary)
            self.assertIn(f"smoke_manifest_snapshot_path={manifest_snapshot}", status_summary)
            self.assertIn(f"run_history_root={run_history_root}", status_summary)
            self.assertIn(f"run_archive_root={run_archive}", status_summary)
            self.assertIn(f"run_archive_manifest={run_manifest}", status_summary)
            self.assertIn(f"run_console_stderr_path={run_console}", status_summary)
            self.assertIn("standard_gap_status=ready_for_gate_escalation", smoke_summary_text)
            self.assertIn("gate_chain_ac2_status=satisfied", smoke_summary_text)
            self.assertIn("gate_chain_ac3_status=ready_to_run", smoke_summary_text)
            self.assertIn("## Diagnostics", status_report)
            self.assertIn("## Gate Chain", status_report)
            self.assertIn("## Iteration Comparison", status_report)
            self.assertIn("## Standard Gap", status_report)
            self.assertIn("## Acceptance Signal", status_report)
            self.assertIn("- Acceptance status: `PASS`", status_report)
            self.assertIn("## Iteration Support", status_report)
            self.assertIn("- Iteration support: `ACTIONABLE`", status_report)
            self.assertIn("- Next step: `gate_escalation`", status_report)
            self.assertIn(
                "- AC3 (`./lca_strong_gate.sh`): status=`ready_to_run`; depends_on=`AC2`; summary=`smoke is green; run the required prerequisite gate next on the same working tree`",
                status_report,
            )
            self.assertIn(
                "- AC5 (`./lca_boj3s_gate.sh`): status=`blocked_by_ac3`; depends_on=`AC3`; summary=`boj3s gate remains blocked until AC3 records a fresh same-worktree pass`",
                status_report,
            )
            self.assertIn(f"- Iteration evidence: `{iteration_evidence}`", status_report)
            self.assertIn(f"- Run history index: `{run_history_index}`", status_report)
            self.assertIn(f"- Run record json: `{run_record}`", status_report)
            self.assertIn(f"- Run comparison json: `{run_comparison}`", status_report)
            self.assertIn(f"- Diagnostics manifest: `{diagnostics_manifest}`", status_report)
            self.assertIn(f"- Run archive root: `{run_archive}`", status_report)
            self.assertIn(f"- Run archive manifest: `{run_manifest}`", status_report)
            self.assertIn(f"- Launcher console transcript: `{run_console}`", status_report)
            self.assertIn(f"- Smoke summary mirror: `{smoke_summary}`", status_report)
            self.assertIn(f"- Smoke report mirror: `{smoke_report}`", status_report)
            self.assertIn(f"- Smoke iteration-evidence mirror: `{smoke_iteration_evidence}`", status_report)
            self.assertIn(f"- Smoke diagnostics-manifest mirror: `{smoke_diagnostics}`", status_report)
            self.assertIn(f"- Smoke standard-gap json: `{smoke_standard_gap}`", status_report)
            self.assertIn(f"- Smoke run-record mirror: `{smoke_run_record}`", status_report)
            self.assertIn(f"- Smoke run-comparison mirror: `{smoke_run_comparison}`", status_report)
            self.assertIn(f"- Suite config: `{suite_config}`", status_report)
            self.assertIn(f"- Suite plan: `{suite_plan}`", status_report)
            self.assertIn(f"- Environment validation report: `{env_report}`", status_report)
            self.assertIn(f"- Environment preflight manifest: `{env_manifest}`", status_report)
            self.assertIn(f"- Build command snapshot: `{build_command}`", status_report)
            self.assertIn(f"- Manifest snapshot: `{manifest_snapshot}`", status_report)
            self.assertIn("Next iteration anchor: start with", status_report)
            self.assertIn(f"status_run_record\t{run_record}\t1\t", diagnostics_text)
            self.assertIn(f"status_run_comparison\t{run_comparison}\t1\t", diagnostics_text)
            self.assertIn(f"smoke_suite_config\t{suite_config}\t1\t", diagnostics_text)
            self.assertIn(f"smoke_suite_plan\t{suite_plan}\t1\t", diagnostics_text)
            self.assertIn(f"smoke_environment_validation\t{env_report}\t1\t", diagnostics_text)
            self.assertIn(f"smoke_environment_preflight_manifest\t{env_manifest}\t1\t", diagnostics_text)
            self.assertIn(f"smoke_environment_build_command\t{build_command}\t1\t", diagnostics_text)
            self.assertIn(f"smoke_manifest_snapshot\t{manifest_snapshot}\t1\t", diagnostics_text)
            self.assertIn(f"status_iteration_evidence\t{iteration_evidence}\t1\t", diagnostics_text)
            self.assertIn(f"run_history_root\t{run_history_root}\t1\t", diagnostics_text)
            self.assertIn(f"run_history_index\t{run_history_index}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_root\t{run_archive}\t1\t", diagnostics_text)
            self.assertIn(f"run_console_stderr\t{run_console}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_manifest\t{run_manifest}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_iteration_evidence\t{run_iteration_evidence}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_run_record\t{run_archive_record}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_run_comparison\t{run_archive_comparison}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_summary\t{smoke_summary}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_status_report\t{smoke_report}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_iteration_evidence\t{smoke_iteration_evidence}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_diagnostics_manifest\t{smoke_diagnostics}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_standard_gap_json\t{smoke_standard_gap}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_run_record\t{smoke_run_record}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_run_comparison\t{smoke_run_comparison}\t1\t", diagnostics_text)
            self.assertEqual(
                iteration_evidence.read_text(encoding="utf-8"),
                smoke_iteration_evidence.read_text(encoding="utf-8"),
            )
            self.assertEqual(
                iteration_evidence.read_text(encoding="utf-8"),
                run_iteration_evidence.read_text(encoding="utf-8"),
            )
            self.assertIn(f"status_summary\t{run_archive / 'summary.txt'}\tcopy_of_", run_manifest.read_text(encoding="utf-8"))
            self.assertIn(
                f"status_iteration_evidence\t{run_iteration_evidence}\tcopy_of_{iteration_evidence}",
                run_manifest.read_text(encoding="utf-8"),
            )
            self.assertIn(
                f"status_run_record\t{run_archive_record}\tcopy_of_{run_record}",
                run_manifest.read_text(encoding="utf-8"),
            )
            self.assertIn(
                f"status_run_comparison\t{run_archive_comparison}\tcopy_of_{run_comparison}",
                run_manifest.read_text(encoding="utf-8"),
            )
            self.assertIn("launcher_console_transcript", run_manifest.read_text(encoding="utf-8"))
            run_console_text = run_console.read_text(encoding="utf-8")
            self.assertIn(f"[lca_smoke] run archive root: {run_archive}", run_console_text)
            self.assertIn(f"[lca_smoke] launcher console transcript: {run_console}", run_console_text)
            self.assertIn(f"[lca_smoke] iteration evidence: {iteration_evidence}", run_console_text)
            self.assertIn(f"[lca_smoke] run history index: {run_history_index}", run_console_text)
            self.assertIn(f"[lca_smoke] run comparison: {run_comparison}", run_console_text)
            self.assertIn(f"[lca_smoke] suite plan: {suite_plan}", run_console_text)
            self.assertEqual(smoke_summary_text, status_summary)
            self.assertEqual(smoke_report.read_text(encoding="utf-8"), status_report)
            self.assertEqual(smoke_diagnostics.read_text(encoding="utf-8"), diagnostics_text)
            self.assertEqual(smoke_run_record.read_text(encoding="utf-8"), run_record.read_text(encoding="utf-8"))
            self.assertEqual(smoke_run_comparison.read_text(encoding="utf-8"), run_comparison.read_text(encoding="utf-8"))
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "failure_report.md").exists(),
                msg="passing smoke runs must clear any stale smoke-root failure report mirror",
            )
            self.assertEqual(smoke_gap_payload["required_standard"], "lca_tree_stress_v5")
            self.assertFalse(smoke_gap_payload["misses_standard"])
            self.assertEqual(smoke_gap_payload["standard_gap_status"], "ready_for_gate_escalation")
            self.assertEqual(smoke_gap_payload["failure_partition"], "pass")
            self.assertEqual(smoke_gap_payload["failure_partition_label"], "pass")
            self.assertEqual(smoke_gap_gate_chain[2]["status"], "satisfied")
            self.assertEqual(smoke_gap_gate_chain[3]["status"], "ready_to_run")
            self.assertEqual(smoke_gap_gate_chain[4]["status"], "pending_after_ac3")
            self.assertEqual(smoke_gap_gate_chain[5]["status"], "blocked_by_ac3")
            self.assertEqual(smoke_gap_gate_chain[6]["status"], "blocked_by_ac5")
            self.assertEqual(smoke_gap_payload["published_artifacts"]["summary_path"], str(smoke_summary))
            self.assertEqual(
                smoke_gap_payload["published_artifacts"]["iteration_evidence_path"],
                str(smoke_iteration_evidence),
            )
            self.assertEqual(run_record_payload["run"]["public_status"], "PASS")
            self.assertEqual(run_record_payload["run"]["normalized_outcome"], "pass")
            self.assertEqual(run_record_payload["summary"]["failure_partition"], "pass")
            self.assertEqual(run_record_payload["summary"]["failure_partition_label"], "pass")
            self.assertEqual(run_record_payload["summary"]["normalized_exit_code"], 0)
            self.assertEqual(run_record_payload["launch"]["invocation_command"], "./lca_smoke.sh")
            self.assertEqual(Path(run_record_payload["launch"]["working_directory"]).resolve(), branch_root.resolve())
            self.assertEqual(
                Path(run_record_payload["launch"]["original_working_directory"]).resolve(),
                branch_root.resolve(),
            )
            self.assertIn(
                str((branch_root / "outer_suite_wrappers" / "lca_smoke.sh").resolve()),
                run_record_payload["launch"]["dispatch_command"],
            )
            self.assertEqual(Path(run_record_payload["inputs"]["suite_config_path"]).resolve(), suite_config)
            self.assertEqual(Path(run_record_payload["inputs"]["suite_plan_path"]).resolve(), suite_plan)
            self.assertEqual(Path(run_record_payload["inputs"]["manifest_snapshot_path"]).resolve(), manifest_snapshot)
            self.assertEqual(Path(run_record_payload["artifacts"]["run_archive_manifest_path"]).resolve(), run_manifest)
            self.assertEqual(
                Path(run_record_payload["artifacts"]["published_smoke_standard_gap_json_path"]).resolve(),
                smoke_standard_gap,
            )
            self.assertEqual(run_record_gate_chain[3]["status"], "ready_to_run")
            self.assertEqual(run_record_gate_chain[5]["status"], "blocked_by_ac3")
            self.assertEqual(run_record_payload["triage"]["retry_command"], "")
            self.assertEqual(run_comparison_payload["has_previous_run"], False)
            self.assertIn(f"[lca_smoke] suite plan: {suite_plan}", result.stderr)
            self.assertIn(f"[lca_smoke] diagnostics manifest: {diagnostics_manifest}", result.stderr)
            self.assertIn(f"[lca_smoke] iteration evidence: {iteration_evidence}", result.stderr)
            self.assertIn(f"[lca_smoke] run comparison: {run_comparison}", result.stderr)
            self.assertIn(f"[lca_smoke] smoke summary mirror: {smoke_summary}", result.stderr)
            self.assertIn(
                f"[lca_smoke] smoke iteration evidence mirror: {smoke_iteration_evidence}",
                result.stderr,
            )
            self.assertIn(f"[lca_smoke] smoke standard gap json: {smoke_standard_gap}", result.stderr)
            self.assertIn(
                "[lca_smoke] gate chain: AC2=satisfied AC3=ready_to_run AC4=pending_after_ac3 AC5=blocked_by_ac3 AC6=blocked_by_ac5",
                result.stderr,
            )

    def test_launcher_status_bundle_indexes_failure_diagnostics_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed123"
                mkdir -p "$CASE_DIR"
                printf 'commands\\n' > "$FAILURE_ROOT/commands.txt"
                printf 'artifact\\tpath\\n' > "$FAILURE_ROOT/artifact_manifest.tsv"
                printf 'rerun\\n' > "$FAILURE_ROOT/rerun_command.txt"
                printf 'seed\\n' > "$FAILURE_ROOT/seed.txt"
                printf 'input\\n' > "$FAILURE_ROOT/input.txt"
                printf 'output\\n' > "$FAILURE_ROOT/solver_output.txt"
                printf 'expected\\n' > "$FAILURE_ROOT/expected_output.txt"
                printf 'invoked\\n' > "$FAILURE_ROOT/invoked_command.txt"
                printf 'helper stdout\\n' > "$FAILURE_ROOT/helper.stdout.txt"
                printf 'helper stderr\\n' > "$FAILURE_ROOT/helper.stderr.txt"
                printf '{}\\n' > "$FAILURE_ROOT/helper_result.json"
                printf 'checker result\\n' > "$FAILURE_ROOT/checker_result.txt"
                printf 'checker stdout\\n' > "$FAILURE_ROOT/checker_replay.stdout.txt"
                printf 'checker stderr\\n' > "$FAILURE_ROOT/checker_replay.stderr.txt"
                printf 'query #7 expected=4 got=3\\n' > "$FAILURE_ROOT/mismatch_summary.txt"
                printf 'retry\\n' > "$FAILURE_ROOT/retry_log.tsv"
                printf 'VAR=1\\n' > "$FAILURE_ROOT/runtime_env.txt"
                printf 'export VAR=1\\n' > "$FAILURE_ROOT/runtime_env_exports.sh"
                printf 'stage\\tmode\\tn\\tseed\\nsmoke\\tcomb_core\\t64\\t123\\n' > "$FAILURE_ROOT/failed_case_row.tsv"
                printf 'manifest\\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
                printf 'case_count=5\\n' > "$FAILURE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed123\\n' > "$FAILURE_ROOT/suite_plan.tsv"
                printf '{"schema":"lca_smoke_failure_context_v1"}\\n' > "$FAILURE_ROOT/failure_context.json"
                printf '#!/usr/bin/env bash\\nexit 0\\n' > "$FAILURE_ROOT/recheck_preserved_output.sh"
                printf '#!/usr/bin/env bash\\nexit 0\\n' > "$FAILURE_ROOT/repro_from_seed.sh"
                printf '#!/usr/bin/env bash\\nexit 0\\n' > "$FAILURE_ROOT/replay_preserved_input.sh"
                printf '#!/usr/bin/env bash\\nexit 0\\n' > "$FAILURE_ROOT/replay_active_manifest_case.sh"
                chmod +x "$FAILURE_ROOT/recheck_preserved_output.sh" "$FAILURE_ROOT/repro_from_seed.sh" "$FAILURE_ROOT/replay_preserved_input.sh" "$FAILURE_ROOT/replay_active_manifest_case.sh"
                cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
                failure_summary=validator mismatch on preserved query 7
                failure_kind=solver_acceptance_failure
                failure_origin=validator
                failure_retryable=0
                failure_reporting_status=complete
                failure_reporting_warning=
                failed_case_tag=smoke_comb_core_n64_seed123
                failed_stage=smoke
                failed_mode=comb_core
                failed_n=64
                failed_seed=123
                failure_root=$FAILURE_ROOT
                failure_case_dir=$CASE_DIR
                commands_path=$FAILURE_ROOT/commands.txt
                artifact_manifest_path=$FAILURE_ROOT/artifact_manifest.tsv
                structured_context_path=$FAILURE_ROOT/failure_context.json
                rerun_command_path=$FAILURE_ROOT/rerun_command.txt
                exact_seed_path=$FAILURE_ROOT/seed.txt
                exact_input_path=$FAILURE_ROOT/input.txt
                exact_output_path=$FAILURE_ROOT/solver_output.txt
                expected_output_path=$FAILURE_ROOT/expected_output.txt
                invoked_command_path=$FAILURE_ROOT/invoked_command.txt
                helper_stdout=$FAILURE_ROOT/helper.stdout.txt
                helper_stderr=$FAILURE_ROOT/helper.stderr.txt
                helper_result_json=$FAILURE_ROOT/helper_result.json
                checker_result_path=$FAILURE_ROOT/checker_result.txt
                checker_replay_stdout_path=$FAILURE_ROOT/checker_replay.stdout.txt
                checker_replay_stderr_path=$FAILURE_ROOT/checker_replay.stderr.txt
                mismatch_summary_path=$FAILURE_ROOT/mismatch_summary.txt
                retry_log_path=$FAILURE_ROOT/retry_log.tsv
                runtime_env_path=$FAILURE_ROOT/runtime_env.txt
                runtime_env_exports_path=$FAILURE_ROOT/runtime_env_exports.sh
                failed_case_row_path=$FAILURE_ROOT/failed_case_row.tsv
                manifest_snapshot_path=$FAILURE_ROOT/smoke_cases_manifest.tsv
                suite_config_path=$FAILURE_ROOT/suite_config.txt
                suite_plan_path=$FAILURE_ROOT/suite_plan.tsv
                checker_script=$FAILURE_ROOT/recheck_preserved_output.sh
                seed_repro_script=$FAILURE_ROOT/repro_from_seed.sh
                preserved_input_replay_script=$FAILURE_ROOT/replay_preserved_input.sh
                active_solver_replay_script=$FAILURE_ROOT/replay_active_manifest_case.sh
                EOF
                printf '# inner failure report\\n' > "$FAILURE_ROOT/latest_failure_report.md"
                exit 1
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 1, msg=result.stderr)
            failure_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure").resolve()
            status_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status"
            diagnostics_manifest = (status_root / "diagnostics_manifest.tsv").resolve()
            smoke_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke").resolve()
            smoke_summary = (smoke_root / "summary.txt").resolve()
            smoke_report = (smoke_root / "status_report.md").resolve()
            smoke_failure_report = (smoke_root / "failure_report.md").resolve()
            smoke_iteration_evidence = (smoke_root / "iteration_evidence.txt").resolve()
            smoke_diagnostics = (smoke_root / "diagnostics_manifest.tsv").resolve()
            smoke_standard_gap = (smoke_root / "standard_gap.json").resolve()
            mismatch_summary = (failure_root / "mismatch_summary.txt").resolve()
            retry_log = (failure_root / "retry_log.tsv").resolve()
            runtime_env = (failure_root / "runtime_env.txt").resolve()
            suite_config = (failure_root / "suite_config.txt").resolve()
            suite_plan = (failure_root / "suite_plan.tsv").resolve()
            helper_result = (failure_root / "helper_result.json").resolve()
            checker_result = (failure_root / "checker_result.txt").resolve()
            failed_case_row = (failure_root / "failed_case_row.tsv").resolve()
            run_history_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_run_history").resolve()
            run_history_index = (run_history_root / "history.tsv").resolve()
            run_archives = sorted(run_history_root.glob("run.*"))
            self.assertEqual(len(run_archives), 1, msg="failed smoke runs must archive the preserved failure context")
            run_archive = run_archives[0].resolve()
            run_console = (run_archive / "console.stderr.txt").resolve()
            run_iteration_evidence = (run_archive / "iteration_evidence.txt").resolve()
            run_record = (status_root / "run_record.json").resolve()
            run_comparison = (status_root / "run_comparison.json").resolve()
            smoke_run_record = (smoke_root / "run_record.json").resolve()
            smoke_run_comparison = (smoke_root / "run_comparison.json").resolve()
            run_archive_record = (run_archive / "run_record.json").resolve()
            run_archive_comparison = (run_archive / "run_comparison.json").resolve()
            run_manifest = (run_archive / "artifact_manifest.tsv").resolve()
            run_source_snapshot = (run_archive / "source_root_snapshot").resolve()
            iteration_evidence = (status_root / "iteration_evidence.txt").resolve()
            status_summary = (status_root / "summary.txt").read_text(encoding="utf-8")
            status_report = (status_root / "latest_status_report.md").read_text(encoding="utf-8")
            diagnostics_text = diagnostics_manifest.read_text(encoding="utf-8")
            smoke_summary_text = smoke_summary.read_text(encoding="utf-8")
            smoke_gap_payload = json.loads(smoke_standard_gap.read_text(encoding="utf-8"))
            smoke_gap_gate_chain = {entry["ac"]: entry for entry in smoke_gap_payload["gate_chain"]}
            run_record_payload = json.loads(run_record.read_text(encoding="utf-8"))
            run_record_gate_chain = {entry["ac"]: entry for entry in run_record_payload["gate_chain"]}
            run_comparison_payload = json.loads(run_comparison.read_text(encoding="utf-8"))

            self.assertIn("required_standard=lca_tree_stress_v5", status_summary)
            self.assertIn("standard_gap_status=smoke_blocker_detected", status_summary)
            self.assertIn("gate_chain_ac2_status=failed", status_summary)
            self.assertIn("gate_chain_ac3_status=blocked_by_ac2", status_summary)
            self.assertIn("gate_chain_ac4_status=blocked_by_ac2", status_summary)
            self.assertIn("gate_chain_ac5_status=blocked_by_ac2", status_summary)
            self.assertIn("gate_chain_ac6_status=blocked_by_ac2", status_summary)
            self.assertIn(f"iteration_evidence_path={iteration_evidence}", status_summary)
            self.assertIn(f"run_history_index_path={run_history_index}", status_summary)
            self.assertIn(f"run_record_path={run_record}", status_summary)
            self.assertIn(f"run_comparison_path={run_comparison}", status_summary)
            self.assertIn(f"published_smoke_summary_path={smoke_summary}", status_summary)
            self.assertIn(f"published_smoke_status_report_path={smoke_report}", status_summary)
            self.assertIn(f"published_smoke_failure_report_path={smoke_failure_report}", status_summary)
            self.assertIn(f"published_smoke_iteration_evidence_path={smoke_iteration_evidence}", status_summary)
            self.assertIn(f"published_smoke_diagnostics_manifest_path={smoke_diagnostics}", status_summary)
            self.assertIn(f"published_smoke_standard_gap_json_path={smoke_standard_gap}", status_summary)
            self.assertIn(f"published_smoke_run_record_path={smoke_run_record}", status_summary)
            self.assertIn(f"published_smoke_run_comparison_path={smoke_run_comparison}", status_summary)
            self.assertIn("result_family=stress_gate", status_summary)
            self.assertIn("normalized_outcome=reproducible_stress_gate_failure", status_summary)
            self.assertIn("source_failure_kind=solver_acceptance_failure", status_summary)
            self.assertIn("source_failure_origin=validator", status_summary)
            self.assertIn("source_failure_retryable=0", status_summary)
            self.assertIn("source_failure_reporting_status=complete", status_summary)
            self.assertIn(f"source_failure_helper_result_json_path={helper_result}", status_summary)
            self.assertIn(f"source_failure_checker_result_path={checker_result}", status_summary)
            self.assertIn(f"source_failure_mismatch_summary_path={mismatch_summary}", status_summary)
            self.assertIn(f"source_failure_retry_log_path={retry_log}", status_summary)
            self.assertIn(f"source_failure_runtime_env_path={runtime_env}", status_summary)
            self.assertIn(f"source_failure_failed_case_row_path={failed_case_row}", status_summary)
            self.assertIn(f"source_failure_suite_config_path={suite_config}", status_summary)
            self.assertIn(f"source_failure_suite_plan_path={suite_plan}", status_summary)
            self.assertIn(f"run_history_root={run_history_root}", status_summary)
            self.assertIn(f"run_archive_root={run_archive}", status_summary)
            self.assertIn(f"run_archive_manifest={run_manifest}", status_summary)
            self.assertIn(f"run_console_stderr_path={run_console}", status_summary)
            self.assertIn("## Diagnostics", status_report)
            self.assertIn("## Gate Chain", status_report)
            self.assertIn("## Iteration Comparison", status_report)
            self.assertIn("## Standard Gap", status_report)
            self.assertIn("- Result family: `stress_gate`", status_report)
            self.assertIn("- Normalized outcome: `reproducible_stress_gate_failure`", status_report)
            self.assertIn(
                "- AC2 (`./lca_smoke.sh`): status=`failed`; depends_on=`none`; summary=`smoke is the active blocker: inner smoke wrapper failed at stage smoke: validator mismatch on preserved query 7`",
                status_report,
            )
            self.assertIn(
                "- AC3 (`./lca_strong_gate.sh`): status=`blocked_by_ac2`; depends_on=`AC2`; summary=`strong gate is intentionally blocked until smoke publishes a fresh same-worktree pass`",
                status_report,
            )
            self.assertIn(f"- Iteration evidence: `{iteration_evidence}`", status_report)
            self.assertIn(f"- Run history index: `{run_history_index}`", status_report)
            self.assertIn(f"- Run record json: `{run_record}`", status_report)
            self.assertIn(f"- Run comparison json: `{run_comparison}`", status_report)
            self.assertIn(f"- Run archive root: `{run_archive}`", status_report)
            self.assertIn(f"- Run archive manifest: `{run_manifest}`", status_report)
            self.assertIn(f"- Launcher console transcript: `{run_console}`", status_report)
            self.assertIn(f"- Smoke summary mirror: `{smoke_summary}`", status_report)
            self.assertIn(f"- Smoke report mirror: `{smoke_report}`", status_report)
            self.assertIn(f"- Smoke failure-report mirror: `{smoke_failure_report}`", status_report)
            self.assertIn(f"- Smoke iteration-evidence mirror: `{smoke_iteration_evidence}`", status_report)
            self.assertIn(f"- Smoke diagnostics-manifest mirror: `{smoke_diagnostics}`", status_report)
            self.assertIn(f"- Smoke standard-gap json: `{smoke_standard_gap}`", status_report)
            self.assertIn(f"- Smoke run-record mirror: `{smoke_run_record}`", status_report)
            self.assertIn(f"- Smoke run-comparison mirror: `{smoke_run_comparison}`", status_report)
            self.assertIn("- Source failure kind: `solver_acceptance_failure`", status_report)
            self.assertIn("- Source failure origin: `validator`", status_report)
            self.assertIn("- Source failure reporting status: `complete`", status_report)
            self.assertIn(f"- Source helper result json: `{helper_result}`", status_report)
            self.assertIn(f"- Source checker result: `{checker_result}`", status_report)
            self.assertIn(f"- Source mismatch summary: `{mismatch_summary}`", status_report)
            self.assertIn(f"- Source retry log: `{retry_log}`", status_report)
            self.assertIn(f"- Source runtime env: `{runtime_env}`", status_report)
            self.assertIn(f"- Source failed-case row: `{failed_case_row}`", status_report)
            self.assertIn(f"- Suite config: `{suite_config}`", status_report)
            self.assertIn(f"- Suite plan: `{suite_plan}`", status_report)
            self.assertIn("Next iteration anchor: start with", status_report)
            self.assertIn(f"source_failure_mismatch_summary\t{mismatch_summary}\t1\t", diagnostics_text)
            self.assertIn(f"source_failure_retry_log\t{retry_log}\t1\t", diagnostics_text)
            self.assertIn(f"source_failure_runtime_env\t{runtime_env}\t1\t", diagnostics_text)
            self.assertIn(f"source_failure_helper_result_json\t{helper_result}\t1\t", diagnostics_text)
            self.assertIn(f"source_failure_checker_result\t{checker_result}\t1\t", diagnostics_text)
            self.assertIn(f"source_failure_failed_case_row\t{failed_case_row}\t1\t", diagnostics_text)
            self.assertIn(f"status_iteration_evidence\t{iteration_evidence}\t1\t", diagnostics_text)
            self.assertIn(f"status_run_record\t{run_record}\t1\t", diagnostics_text)
            self.assertIn(f"status_run_comparison\t{run_comparison}\t1\t", diagnostics_text)
            self.assertIn(f"run_history_root\t{run_history_root}\t1\t", diagnostics_text)
            self.assertIn(f"run_history_index\t{run_history_index}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_root\t{run_archive}\t1\t", diagnostics_text)
            self.assertIn(f"run_console_stderr\t{run_console}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_manifest\t{run_manifest}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_iteration_evidence\t{run_iteration_evidence}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_run_record\t{run_archive_record}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_run_comparison\t{run_archive_comparison}\t1\t", diagnostics_text)
            self.assertIn(f"run_archive_source_root_snapshot\t{run_source_snapshot}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_summary\t{smoke_summary}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_status_report\t{smoke_report}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_failure_report\t{smoke_failure_report}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_iteration_evidence\t{smoke_iteration_evidence}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_diagnostics_manifest\t{smoke_diagnostics}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_standard_gap_json\t{smoke_standard_gap}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_run_record\t{smoke_run_record}\t1\t", diagnostics_text)
            self.assertIn(f"published_smoke_run_comparison\t{smoke_run_comparison}\t1\t", diagnostics_text)
            self.assertTrue((run_source_snapshot / "mismatch_summary.txt").is_file())
            self.assertEqual(
                (run_source_snapshot / "mismatch_summary.txt").read_text(encoding="utf-8"),
                mismatch_summary.read_text(encoding="utf-8"),
            )
            self.assertEqual(
                iteration_evidence.read_text(encoding="utf-8"),
                smoke_iteration_evidence.read_text(encoding="utf-8"),
            )
            self.assertEqual(
                iteration_evidence.read_text(encoding="utf-8"),
                run_iteration_evidence.read_text(encoding="utf-8"),
            )
            self.assertIn(
                f"status_iteration_evidence\t{run_iteration_evidence}\tcopy_of_{iteration_evidence}",
                run_manifest.read_text(encoding="utf-8"),
            )
            self.assertIn(
                f"status_run_record\t{run_archive_record}\tcopy_of_{run_record}",
                run_manifest.read_text(encoding="utf-8"),
            )
            self.assertIn(
                f"status_run_comparison\t{run_archive_comparison}\tcopy_of_{run_comparison}",
                run_manifest.read_text(encoding="utf-8"),
            )
            self.assertIn(f"source_root_snapshot\t{run_source_snapshot}\tcopy_of_{failure_root}", run_manifest.read_text(encoding="utf-8"))
            run_console_text = run_console.read_text(encoding="utf-8")
            self.assertIn(f"[lca_smoke] run archive root: {run_archive}", run_console_text)
            self.assertIn(f"[lca_smoke] launcher console transcript: {run_console}", run_console_text)
            self.assertIn(f"[lca_smoke] mismatch summary: {mismatch_summary}", run_console_text)
            self.assertIn(f"[lca_smoke] iteration evidence: {iteration_evidence}", run_console_text)
            self.assertIn(f"[lca_smoke] run history index: {run_history_index}", run_console_text)
            self.assertIn(f"[lca_smoke] run comparison: {run_comparison}", run_console_text)
            self.assertEqual(smoke_summary_text, status_summary)
            self.assertEqual(smoke_report.read_text(encoding="utf-8"), status_report)
            self.assertEqual(smoke_failure_report.read_text(encoding="utf-8"), status_report)
            self.assertEqual(smoke_diagnostics.read_text(encoding="utf-8"), diagnostics_text)
            self.assertEqual(smoke_run_record.read_text(encoding="utf-8"), run_record.read_text(encoding="utf-8"))
            self.assertEqual(smoke_run_comparison.read_text(encoding="utf-8"), run_comparison.read_text(encoding="utf-8"))
            self.assertEqual(smoke_gap_payload["required_standard"], "lca_tree_stress_v5")
            self.assertTrue(smoke_gap_payload["misses_standard"])
            self.assertEqual(smoke_gap_payload["standard_gap_status"], "smoke_blocker_detected")
            self.assertEqual(smoke_gap_payload["result_family"], "stress_gate")
            self.assertEqual(smoke_gap_payload["normalized_outcome"], "reproducible_stress_gate_failure")
            self.assertEqual(smoke_gap_gate_chain[2]["status"], "failed")
            self.assertEqual(smoke_gap_gate_chain[3]["status"], "blocked_by_ac2")
            self.assertEqual(smoke_gap_gate_chain[4]["status"], "blocked_by_ac2")
            self.assertEqual(smoke_gap_gate_chain[5]["status"], "blocked_by_ac2")
            self.assertEqual(smoke_gap_gate_chain[6]["status"], "blocked_by_ac2")
            self.assertEqual(smoke_gap_payload["triage"]["stage"], "smoke")
            self.assertEqual(smoke_gap_payload["triage"]["stage_label"], "inner_wrapper_case:smoke")
            self.assertEqual(smoke_gap_payload["triage"]["iteration_evidence_path"], str(iteration_evidence))
            self.assertEqual(smoke_gap_payload["source_failure"]["kind"], "solver_acceptance_failure")
            self.assertEqual(smoke_gap_payload["published_artifacts"]["failure_report_path"], str(smoke_failure_report))
            self.assertEqual(
                smoke_gap_payload["published_artifacts"]["iteration_evidence_path"],
                str(smoke_iteration_evidence),
            )
            self.assertEqual(run_record_payload["run"]["public_status"], "FAIL")
            self.assertEqual(run_record_payload["run"]["normalized_outcome"], "reproducible_stress_gate_failure")
            self.assertEqual(run_record_payload["launch"]["invocation_command"], "./lca_smoke.sh")
            self.assertEqual(
                Path(run_record_payload["inputs"]["source_failure_failed_case_row_path"]).resolve(),
                failed_case_row,
            )
            self.assertEqual(
                Path(run_record_payload["inputs"]["source_failure_exact_input_path"]).resolve(),
                (failure_root / "input.txt").resolve(),
            )
            self.assertEqual(
                run_record_payload["triage"]["retry_command"],
                run_record_payload["source_failure"]["replay_command"],
            )
            self.assertTrue(
                run_record_payload["triage"]["retry_command"].endswith("replay_active_manifest_case.sh"),
            )
            self.assertEqual(
                Path(run_record_payload["source_failure"]["artifacts"]["rerun_command_path"]).resolve(),
                (failure_root / "rerun_command.txt").resolve(),
            )
            self.assertEqual(
                Path(run_record_payload["source_failure"]["artifacts"]["invoked_command_path"]).resolve(),
                (failure_root / "invoked_command.txt").resolve(),
            )
            self.assertEqual(
                Path(run_record_payload["source_failure"]["artifacts"]["artifact_manifest_path"]).resolve(),
                (failure_root / "artifact_manifest.tsv").resolve(),
            )
            self.assertEqual(
                Path(run_record_payload["artifacts"]["published_smoke_failure_report_path"]).resolve(),
                smoke_failure_report,
            )
            self.assertEqual(run_record_gate_chain[2]["status"], "failed")
            self.assertEqual(run_record_gate_chain[3]["status"], "blocked_by_ac2")
            self.assertEqual(run_comparison_payload["has_previous_run"], False)
            self.assertIn(
                "[lca_smoke] public status: FAIL family=stress_gate",
                result.stderr,
            )
            self.assertIn(
                "[lca_smoke] normalized outcome: reproducible_stress_gate_failure",
                result.stderr,
            )
            self.assertIn(
                "[lca_smoke] outcome summary: inner smoke wrapper failed at stage smoke: validator mismatch on preserved query 7",
                result.stderr,
            )
            self.assertIn(f"[lca_smoke] mismatch summary: {mismatch_summary}", result.stderr)
            self.assertIn(f"[lca_smoke] source failed-case row: {failed_case_row}", result.stderr)
            self.assertIn(f"[lca_smoke] diagnostics manifest: {diagnostics_manifest}", result.stderr)
            self.assertIn(f"[lca_smoke] iteration evidence: {iteration_evidence}", result.stderr)
            self.assertIn(f"[lca_smoke] run comparison: {run_comparison}", result.stderr)
            self.assertIn(f"[lca_smoke] smoke summary mirror: {smoke_summary}", result.stderr)
            self.assertIn(
                f"[lca_smoke] smoke iteration evidence mirror: {smoke_iteration_evidence}",
                result.stderr,
            )
            self.assertIn(f"[lca_smoke] smoke standard gap json: {smoke_standard_gap}", result.stderr)
            self.assertIn(
                "[lca_smoke] gate chain: AC2=failed AC3=blocked_by_ac2 AC4=blocked_by_ac2 AC5=blocked_by_ac2 AC6=blocked_by_ac2",
                result.stderr,
            )

    def test_launcher_records_previous_run_comparison_in_history_ledger(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))

            first = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(first.returncode, 0, msg=first.stderr)
            self.write_text(
                branch_root / "outer_suite_wrappers" / "lca_smoke.sh",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail
                    SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                    BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                    FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                    CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed321"
                    mkdir -p "$CASE_DIR"
                    printf 'commands\\n' > "$FAILURE_ROOT/commands.txt"
                    printf 'artifact\\tpath\\n' > "$FAILURE_ROOT/artifact_manifest.tsv"
                    printf 'rerun\\n' > "$FAILURE_ROOT/rerun_command.txt"
                    printf 'seed\\n' > "$FAILURE_ROOT/seed.txt"
                    printf 'input\\n' > "$FAILURE_ROOT/input.txt"
                    printf 'output\\n' > "$FAILURE_ROOT/solver_output.txt"
                    printf 'expected\\n' > "$FAILURE_ROOT/expected_output.txt"
                    printf 'invoked\\n' > "$FAILURE_ROOT/invoked_command.txt"
                    printf 'query #3 expected=2 got=1\\n' > "$FAILURE_ROOT/mismatch_summary.txt"
                    printf '{"schema":"lca_smoke_failure_context_v1"}\\n' > "$FAILURE_ROOT/failure_context.json"
                    printf 'VAR=1\\n' > "$FAILURE_ROOT/runtime_env.txt"
                    printf 'manifest\\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
                    printf 'case_count=1\\n' > "$FAILURE_ROOT/suite_config.txt"
                    printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed321\\n' > "$FAILURE_ROOT/suite_plan.tsv"
                    cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
                    failure_summary=validator mismatch on preserved query 3
                    failure_kind=solver_acceptance_failure
                    failure_origin=validator
                    failure_retryable=0
                    failure_reporting_status=complete
                    failure_reporting_warning=
                    failed_case_tag=smoke_comb_core_n64_seed321
                    failed_stage=smoke
                    failed_mode=comb_core
                    failed_n=64
                    failed_seed=321
                    failure_root=$FAILURE_ROOT
                    failure_case_dir=$CASE_DIR
                    commands_path=$FAILURE_ROOT/commands.txt
                    artifact_manifest_path=$FAILURE_ROOT/artifact_manifest.tsv
                    structured_context_path=$FAILURE_ROOT/failure_context.json
                    rerun_command_path=$FAILURE_ROOT/rerun_command.txt
                    exact_seed_path=$FAILURE_ROOT/seed.txt
                    exact_input_path=$FAILURE_ROOT/input.txt
                    exact_output_path=$FAILURE_ROOT/solver_output.txt
                    expected_output_path=$FAILURE_ROOT/expected_output.txt
                    invoked_command_path=$FAILURE_ROOT/invoked_command.txt
                    mismatch_summary_path=$FAILURE_ROOT/mismatch_summary.txt
                    runtime_env_path=$FAILURE_ROOT/runtime_env.txt
                    manifest_snapshot_path=$FAILURE_ROOT/smoke_cases_manifest.tsv
                    suite_config_path=$FAILURE_ROOT/suite_config.txt
                    suite_plan_path=$FAILURE_ROOT/suite_plan.tsv
                    EOF
                    printf '# inner failure report\\n' > "$FAILURE_ROOT/latest_failure_report.md"
                    exit 1
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "outer_suite_wrappers" / "lca_smoke.sh")

            second = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(second.returncode, 1, msg=second.stderr)
            status_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status").resolve()
            history_index = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_run_history" / "history.tsv").resolve()
            run_record = (status_root / "run_record.json").resolve()
            run_comparison = (status_root / "run_comparison.json").resolve()
            history_rows = self.read_tsv_rows(history_index)
            summary_text = (status_root / "summary.txt").read_text(encoding="utf-8")
            run_record_payload = json.loads(run_record.read_text(encoding="utf-8"))
            run_comparison_payload = json.loads(run_comparison.read_text(encoding="utf-8"))

            self.assertEqual(len(history_rows), 2, msg="the smoke history ledger must retain both iterations")
            self.assertEqual(history_rows[0]["public_status"], "PASS")
            self.assertEqual(history_rows[0]["normalized_outcome"], "pass")
            self.assertEqual(history_rows[1]["public_status"], "FAIL")
            self.assertEqual(history_rows[1]["normalized_outcome"], "reproducible_stress_gate_failure")
            self.assertTrue(history_rows[1]["run_record_path"].endswith("/run_record.json"))
            self.assertTrue(history_rows[1]["run_comparison_path"].endswith("/run_comparison.json"))
            self.assertIn(f"previous_run_id={history_rows[0]['run_id']}", summary_text)
            self.assertIn("previous_run_normalized_outcome=pass", summary_text)
            self.assertIn("run_comparison_summary=changed", summary_text)
            self.assertIn("run_comparison_changed_fields=", summary_text)
            self.assertEqual(run_record_payload["comparison"]["previous_run_id"], history_rows[0]["run_id"])
            self.assertEqual(run_comparison_payload["has_previous_run"], True)
            self.assertEqual(run_comparison_payload["previous_run"]["run_id"], history_rows[0]["run_id"])
            self.assertEqual(run_comparison_payload["previous_run"]["normalized_outcome"], "pass")
            self.assertEqual(run_comparison_payload["current_run"]["run_record_path"], history_rows[1]["run_record_path"])
            self.assertEqual(
                run_comparison_payload["current_run"]["normalized_outcome"],
                "reproducible_stress_gate_failure",
            )
            self.assertIn("public_status", run_comparison_payload["changed_fields"])
            self.assertIn("normalized_outcome", run_comparison_payload["changed_fields"])
            self.assertIn("stage_label", run_comparison_payload["changed_fields"])
            self.assertIn(f"[lca_smoke] run history index: {history_index}", second.stderr)
            self.assertIn(f"[lca_smoke] previous run: id={history_rows[0]['run_id']} outcome=pass", second.stderr)
            self.assertIn(f"[lca_smoke] run comparison: {run_comparison}", second.stderr)

    def test_launcher_normalizes_duplicate_out_of_order_history_before_selecting_previous_run(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            history_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_run_history"
            history_root.mkdir(parents=True, exist_ok=True)
            history_path = history_root / "history.tsv"
            history_path.write_text(
                textwrap.dedent(
                    """\
                    run_id\trun_started_at_utc\trun_finished_at_utc\trun_elapsed_seconds\tpublic_status\tacceptance_signal_status\titeration_support_status\titeration_support_next_step\tresult_family\tnormalized_outcome\tnormalized_exit_code\traw_exit_code\tstage_label\toutcome_source\tsource_failure_case\tsource_failure_kind\trun_archive_root\trun_console_stderr_path\trun_dispatch_result_path\tstatus_summary_path\tstatus_report_path\titeration_evidence_path\tdiagnostics_manifest_path\trun_record_path\trun_comparison_path
                    run.000019\t2026-04-12T01:36:39Z\t2026-04-12T01:37:28Z\t49\tFAIL\tFAIL\tACTIONABLE\trepair_then_retry\tharness\tharness_infrastructure_failure\t70\t0\tinner_wrapper_bundle_validation:bundle_validation\tinner_wrapper\t\t\t/history/run.000019\t/history/run.000019/console.stderr.txt\t/history/run.000019/dispatch_result.txt\t/history/run.000019/summary.txt\t/history/run.000019/latest_status_report.md\t/history/run.000019/iteration_evidence.txt\t/history/run.000019/diagnostics_manifest.tsv\t/history/run.000019/run_record.json\t/history/run.000019/run_comparison.json
                    run.000020\t2026-04-12T01:37:23Z\t2026-04-12T01:37:27Z\t4\tFAIL\tFAIL\tACTIONABLE\trepair_then_retry\tharness\tharness_infrastructure_failure\t70\t70\tlauncher_pre_dispatch:stale_inner_rerun_cleanup\tlauncher\t\t\t/history/run.000020\t/history/run.000020/console.stderr.txt\t/history/run.000020/dispatch_result.txt\t/history/run.000020/summary.txt\t/history/run.000020/latest_status_report.md\t/history/run.000020/iteration_evidence.txt\t/history/run.000020/diagnostics_manifest.tsv\t/history/run.000020/run_record.json\t/history/run.000020/run_comparison.json
                    run.000019\t2026-04-12T01:37:06Z\t2026-04-12T01:37:10Z\t4\tFAIL\tFAIL\tACTIONABLE\trepair_then_retry\tharness\tharness_infrastructure_failure\t70\t70\tlauncher_pre_dispatch:stale_inner_rerun_cleanup\tlauncher\t\t\t/history/run.000019\t/history/run.000019/console.stderr.txt\t/history/run.000019/dispatch_result.txt\t/history/run.000019/summary.txt\t/history/run.000019/latest_status_report.md\t/history/run.000019/iteration_evidence.txt\t/history/run.000019/diagnostics_manifest.tsv\t/history/run.000019/run_record.json\t/history/run.000019/run_comparison.json
                    """
                ),
                encoding="utf-8",
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            status_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status").resolve()
            history_rows = self.read_tsv_rows(history_path)
            status_summary = dict(
                line.split("=", 1)
                for line in (status_root / "summary.txt").read_text(encoding="utf-8").splitlines()
                if "=" in line
            )
            run_record_payload = json.loads((status_root / "run_record.json").read_text(encoding="utf-8"))
            run_comparison_payload = json.loads((status_root / "run_comparison.json").read_text(encoding="utf-8"))

            self.assertEqual(
                [row["run_id"] for row in history_rows],
                ["run.000019", "run.000020", "run.000021"],
                msg="history normalization must keep one row per run id and restore sequential ordering",
            )
            self.assertEqual(status_summary["previous_run_id"], "run.000020")
            self.assertEqual(status_summary["previous_run_stage_label"], "launcher_pre_dispatch:stale_inner_rerun_cleanup")
            self.assertEqual(run_record_payload["comparison"]["previous_run_id"], "run.000020")
            self.assertEqual(run_comparison_payload["previous_run"]["run_id"], "run.000020")
            self.assertIn(
                "[lca_smoke] previous run: id=run.000020 outcome=harness_infrastructure_failure",
                result.stderr,
            )

    def test_launcher_uses_deterministic_sequential_run_archive_paths(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                rm -rf "$SMOKE_ROOT"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed1\\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )
            run_history_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_run_history").resolve()
            run_export_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_runs").resolve()

            first = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )
            second = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(first.returncode, 0, msg=first.stderr)
            self.assertEqual(second.returncode, 0, msg=second.stderr)

            history_rows = self.read_tsv_rows(run_history_root / "history.tsv")
            run_archives = sorted(path.resolve() for path in run_history_root.glob("run.*"))
            run_aliases = sorted(path for path in run_export_root.glob("run-*"))

            self.assertEqual(
                [path.name for path in run_archives],
                ["run.000001", "run.000002"],
                msg="run archives must use deterministic sequential ids for the same workspace history state",
            )
            self.assertEqual(
                [path.name for path in run_aliases],
                ["run-000001", "run-000002"],
                msg="exported smoke-run aliases must stay aligned with the deterministic archive ids",
            )
            self.assertEqual(history_rows[0]["run_id"], "run.000001")
            self.assertEqual(history_rows[1]["run_id"], "run.000002")
            self.assertEqual(Path(history_rows[0]["run_archive_root"]).resolve(), run_archives[0])
            self.assertEqual(Path(history_rows[1]["run_archive_root"]).resolve(), run_archives[1])
            self.assertTrue(run_aliases[0].is_symlink())
            self.assertTrue(run_aliases[1].is_symlink())
            self.assertEqual(run_aliases[0].resolve(), run_archives[0])
            self.assertEqual(run_aliases[1].resolve(), run_archives[1])

            status_summary = dict(
                line.split("=", 1)
                for line in (
                    branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
                ).read_text(encoding="utf-8").splitlines()
                if "=" in line
            )
            self.assertEqual(status_summary["run_id"], "run.000002")
            self.assertEqual(status_summary["previous_run_id"], "run.000001")
            self.assertEqual(Path(status_summary["run_archive_root"]).resolve(), run_archives[1])
            self.assertEqual(Path(status_summary["previous_run_archive_root"]).resolve(), run_archives[0])

    def test_launcher_prunes_orphan_partial_run_archives_before_allocating_next_run(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            run_history_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_run_history").resolve()

            first = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(first.returncode, 0, msg=first.stderr)

            stale_orphan = run_history_root / "run.000002"
            self.write_text(stale_orphan / "stale.txt", "stale partial archive\n")

            second = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(second.returncode, 0, msg=second.stderr)
            history_rows = self.read_tsv_rows(run_history_root / "history.tsv")
            run_archives = sorted(path.resolve() for path in run_history_root.glob("run.*"))
            latest_alias = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_runs" / "run-000002"
            ).resolve()

            self.assertEqual(
                [path.name for path in run_archives],
                ["run.000001", "run.000002"],
                msg="partial orphan run archives must be pruned before the next run id is chosen",
            )
            self.assertEqual(history_rows[0]["run_id"], "run.000001")
            self.assertEqual(history_rows[1]["run_id"], "run.000002")
            self.assertFalse(
                (run_history_root / "run.000002" / "stale.txt").exists(),
                msg="the next run archive must not inherit stale contents from an orphan partial archive",
            )
            self.assertTrue((run_history_root / "run.000002" / "summary.txt").is_file())
            self.assertEqual(latest_alias, run_history_root / "run.000002")

    def test_launcher_reconciles_stale_run_export_aliases_before_next_run(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            run_history_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_run_history").resolve()
            run_export_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_runs").resolve()

            first = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(first.returncode, 0, msg=first.stderr)
            os.unlink(run_export_root / "run-000001")
            self.write_text(run_export_root / "run-000001" / "stale.txt", "stale historical alias\n")
            self.write_text(run_export_root / "run-000099" / "stale.txt", "stale stray alias\n")

            second = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(second.returncode, 0, msg=second.stderr)
            repaired_alias = run_export_root / "run-000001"
            current_alias = run_export_root / "run-000002"
            stray_alias = run_export_root / "run-000099"

            self.assertTrue(repaired_alias.is_symlink(), msg="historical run aliases must be repaired back to symlinks")
            self.assertEqual(repaired_alias.resolve(), run_history_root / "run.000001")
            self.assertTrue(current_alias.is_symlink(), msg="the current run alias must still be published")
            self.assertEqual(current_alias.resolve(), run_history_root / "run.000002")
            self.assertFalse(
                stray_alias.exists() or stray_alias.is_symlink(),
                msg="launcher-owned run export roots must drop stale aliases that are not backed by recorded run archives",
            )

    def test_successful_run_clears_stale_launcher_failure_bundle(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_launcher_latest_failure"
            self.write_text(failure_root / "failure_summary.txt", "stale launcher failure\n")
            self.write_text(failure_root / "failure_reason.txt", "stale launcher failure\n")

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertFalse(
                failure_root.exists(),
                msg="successful launcher runs must clear stale launcher failure bundles before later retries inspect stable smoke artifacts",
            )
            status_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            self.assertTrue(status_summary.is_file(), msg="launcher must still publish the stable smoke status summary")
            status_summary_text = status_summary.read_text(encoding="utf-8")
            self.assertIn("public_status=PASS", status_summary_text)
            self.assertIn("result_family=none", status_summary_text)
            self.assertIn("normalized_outcome=pass", status_summary_text)

    def test_successful_rerun_keeps_previous_stage_metadata_and_empty_case_fields(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                rm -rf "$SMOKE_ROOT"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed1\\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )

            first = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )
            second = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(first.returncode, 0, msg=first.stderr)
            self.assertEqual(second.returncode, 0, msg=second.stderr)
            status_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status").resolve()
            history_index = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_run_history" / "history.tsv").resolve()
            history_rows = self.read_tsv_rows(history_index)
            status_summary = dict(
                line.split("=", 1)
                for line in (status_root / "summary.txt").read_text(encoding="utf-8").splitlines()
                if "=" in line
            )
            run_comparison_payload = json.loads((status_root / "run_comparison.json").read_text(encoding="utf-8"))

            self.assertEqual(len(history_rows), 2, msg="the smoke history ledger must retain both successful iterations")
            self.assertEqual(history_rows[0]["public_status"], "PASS")
            self.assertEqual(history_rows[1]["public_status"], "PASS")
            self.assertEqual(history_rows[0]["stage_label"], "completed:completed")
            self.assertEqual(history_rows[1]["stage_label"], "completed:completed")
            self.assertEqual(history_rows[0]["source_failure_case"], "")
            self.assertEqual(status_summary["previous_run_id"], history_rows[0]["run_id"])
            self.assertEqual(status_summary["previous_run_normalized_outcome"], "pass")
            self.assertEqual(status_summary["previous_run_stage_label"], "completed:completed")
            self.assertEqual(status_summary["previous_run_source_failure_case"], "")
            self.assertEqual(status_summary["previous_run_status_summary_path"], history_rows[0]["status_summary_path"])
            self.assertEqual(
                status_summary["previous_run_iteration_evidence_path"],
                history_rows[0]["iteration_evidence_path"],
            )
            self.assertEqual(status_summary["run_comparison_changed_fields"], "")
            self.assertIn("same normalized outcome as previous run", status_summary["run_comparison_summary"])
            self.assertEqual(run_comparison_payload["changed_fields"], [])
            self.assertEqual(run_comparison_payload["previous_run"]["stage_label"], "completed:completed")
            self.assertEqual(run_comparison_payload["previous_run"]["source_failure_case"], "")
            self.assertIn(
                f"[lca_smoke] previous run: id={history_rows[0]['run_id']} outcome=pass stage=completed:completed archive={history_rows[0]['run_archive_root']}",
                second.stderr,
            )

    def test_successful_rerun_clears_stale_inner_wrapper_state_from_prior_failures(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            failure_root = artifacts_root / "smoke_latest_failure"
            setup_root = artifacts_root / "smoke_setup"
            tmp_root = artifacts_root / ".tmp"
            session_root = tmp_root / "lca_smoke.session"
            setup_tmpdir = tmp_root / "lca_smoke.setup.tmp"
            run_workdir = tmp_root / "lca_smoke.run.stale"
            run_tmpdir = tmp_root / "lca_smoke.tmp.stale"
            probe_tmpdir = tmp_root / "lca_smoke_probe.stale"
            build_tmp = tmp_root / "boj28350_branch_3_solver-stale.o"
            build_tmp_tmp = tmp_root / "boj28350_branch_3_solver-stale.o.tmp"
            backup_root = artifacts_root / "smoke.previous"
            legacy_output_root = artifacts_root / ".lca_smoke_in_progress.stale"

            self.write_text(failure_root / "failure_summary.txt", "stale inner failure\n")
            self.write_text(failure_root / "latest_failure_report.md", "# stale inner failure\n")
            self.write_text(setup_root / "preflight_manifest.tsv", "kind\tname\tstatus\n")
            self.write_text(session_root / "home" / "stale.txt", "stale session state\n")
            self.write_text(setup_tmpdir / "stale.txt", "stale setup tmpdir\n")
            self.write_text(run_workdir / "stale.txt", "stale workdir\n")
            self.write_text(run_tmpdir / "stale.txt", "stale runtime tmpdir\n")
            self.write_text(probe_tmpdir / "stale.txt", "stale probe tmpdir\n")
            self.write_text(build_tmp, "stale build tmp\n")
            self.write_text(build_tmp_tmp, "stale build tmp tmp\n")
            self.write_text(backup_root / "stale.txt", "stale backup output\n")
            self.write_text(legacy_output_root / "stale.txt", "stale legacy output\n")

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertFalse(
                failure_root.exists(),
                msg="successful public smoke reruns must clear stale inner failure bundles before the next iteration",
            )
            self.assertFalse(
                setup_root.exists(),
                msg="successful public smoke reruns must clear stale inner setup roots before the next iteration",
            )
            self.assertFalse(
                session_root.exists(),
                msg="successful public smoke reruns must clear stale inner session roots before the next iteration",
            )
            self.assertFalse(
                setup_tmpdir.exists(),
                msg="successful public smoke reruns must clear stale inner setup tmpdirs before the next iteration",
            )
            self.assertFalse(
                run_workdir.exists(),
                msg="successful public smoke reruns must clear stale inner workdirs before the next iteration",
            )
            self.assertFalse(
                run_tmpdir.exists(),
                msg="successful public smoke reruns must clear stale inner runtime tmpdirs before the next iteration",
            )
            self.assertFalse(
                probe_tmpdir.exists(),
                msg="successful public smoke reruns must clear stale inner probe tmpdirs before the next iteration",
            )
            self.assertFalse(
                build_tmp.exists(),
                msg="successful public smoke reruns must clear stale inner solver build tmp files before the next iteration",
            )
            self.assertFalse(
                build_tmp_tmp.exists(),
                msg="successful public smoke reruns must clear stale inner solver build tmp tmp files before the next iteration",
            )
            self.assertFalse(
                backup_root.exists(),
                msg="successful public smoke reruns must clear stale inner smoke backup roots before the next iteration",
            )
            self.assertFalse(
                legacy_output_root.exists(),
                msg="successful public smoke reruns must clear stale legacy in-progress smoke outputs before the next iteration",
            )
            status_summary = artifacts_root / "smoke_latest_status" / "summary.txt"
            self.assertTrue(status_summary.is_file(), msg="successful reruns must still publish the stable status summary")
            self.assertIn("public_status=PASS", status_summary.read_text(encoding="utf-8"))

    def test_interrupted_dispatch_cleans_partial_inner_state_and_allows_clean_retry(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
                SMOKE_ROOT="$ARTIFACTS_ROOT/smoke"
                FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
                COUNT_PATH="$ARTIFACTS_ROOT/dispatch_count.txt"
                READY_PATH="$ARTIFACTS_ROOT/interrupt_ready.txt"
                SETUP_ROOT="$ARTIFACTS_ROOT/smoke_setup"
                SESSION_ROOT="$ARTIFACTS_ROOT/.tmp/lca_smoke.session/home"
                RUN_TMPDIR="$ARTIFACTS_ROOT/.tmp/lca_smoke.tmp.interrupted"
                RUN_WORKDIR="$ARTIFACTS_ROOT/.tmp/lca_smoke.run.interrupted"
                count=0

                mkdir -p "$ARTIFACTS_ROOT"
                if [[ -f "$COUNT_PATH" ]]; then
                  count="$(cat "$COUNT_PATH")"
                fi
                count=$(( count + 1 ))
                printf '%s\\n' "$count" > "$COUNT_PATH"

                if (( count == 1 )); then
                  rm -rf "$FAILURE_ROOT"
                  mkdir -p "$FAILURE_ROOT" "$SETUP_ROOT" "$SESSION_ROOT" "$RUN_TMPDIR" "$RUN_WORKDIR"
                  printf 'stale setup from interrupted run\\n' > "$SETUP_ROOT/stale.txt"
                  printf 'stale session from interrupted run\\n' > "$SESSION_ROOT/stale.txt"
                  printf 'stale tmpdir from interrupted run\\n' > "$RUN_TMPDIR/stale.txt"
                  printf 'stale workdir from interrupted run\\n' > "$RUN_WORKDIR/stale.txt"
                  printf 'failure_summary=partial interrupted smoke run\\n' > "$FAILURE_ROOT/failure_summary.txt"
                  : > "$READY_PATH"
                  trap '' TERM
                  while :; do
                    sleep 0.05
                  done
                fi

                if [[ -e "$SETUP_ROOT/stale.txt" || -e "$SESSION_ROOT/stale.txt" || -e "$RUN_TMPDIR/stale.txt" || -e "$RUN_WORKDIR/stale.txt" ]]; then
                  rm -rf "$FAILURE_ROOT"
                  mkdir -p "$FAILURE_ROOT"
                  {
                    printf 'failure_summary=stale interrupted rerun state leaked into the next dispatch\\n'
                    printf 'failure_kind=harness_transient_failure\\n'
                    printf 'failure_origin=launcher\\n'
                    printf 'failure_retryable=1\\n'
                  } > "$FAILURE_ROOT/failure_summary.txt"
                  printf '# stale interrupted rerun state leaked\\n' > "$FAILURE_ROOT/latest_failure_report.md"
                  exit 70
                fi

                rm -f "$READY_PATH"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed1\\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            dispatch_count_path = artifacts_root / "dispatch_count.txt"
            status_summary = artifacts_root / "smoke_latest_status" / "summary.txt"
            ready_path = artifacts_root / "interrupt_ready.txt"
            env = os.environ.copy()
            env["LCA_SMOKE_LAUNCHER_TIMEOUT_S"] = "10"

            proc = subprocess.Popen(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                env=env,
            )
            deadline = time.monotonic() + 10.0
            while time.monotonic() < deadline:
                dispatch_started = (
                    dispatch_count_path.exists()
                    and dispatch_count_path.read_text(encoding="utf-8") == "1\n"
                )
                if ready_path.exists() or dispatch_started:
                    break
                if proc.poll() is not None:
                    break
                time.sleep(0.05)
            if not (
                ready_path.exists()
                or (
                    dispatch_count_path.exists()
                    and dispatch_count_path.read_text(encoding="utf-8") == "1\n"
                )
            ):
                _, early_stderr = proc.communicate(timeout=10)
                self.fail(f"the fake inner wrapper never reached the interruptible dispatch state:\n{early_stderr}")

            proc.send_signal(signal.SIGTERM)
            try:
                _, first_stderr = proc.communicate(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
                _, first_stderr = proc.communicate()
                self.fail(f"interrupted launcher did not exit promptly:\\n{first_stderr}")

            self.assertEqual(
                proc.returncode,
                70,
                msg="interrupted dispatches must normalize to a stable harness failure instead of hanging or leaking the child monitor",
            )
            summary_text = status_summary.read_text(encoding="utf-8")
            self.assertIn("public_status=FAIL", summary_text)
            self.assertIn("result_family=harness", summary_text)
            self.assertIn("normalized_exit_code=70", summary_text)
            self.assertIn("normalized_outcome=harness_infrastructure_failure", summary_text)
            self.assertIn("outcome_source=launcher", summary_text)
            self.assertIn("launcher received SIGTERM while waiting for inner smoke wrapper", summary_text)
            self.assertIn(
                "[lca_smoke] outcome summary: launcher received SIGTERM while waiting for inner smoke wrapper",
                first_stderr,
            )

            second = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
                env=env,
            )

            self.assertEqual(
                second.returncode,
                0,
                msg="the next retry must recover from the interrupted dispatch without manual cleanup",
            )
            self.assertEqual(
                (artifacts_root / "dispatch_count.txt").read_text(encoding="utf-8"),
                "2\n",
                msg="the follow-up retry must reach a fresh second dispatch",
            )
            self.assertFalse((artifacts_root / "smoke_setup").exists())
            self.assertFalse((artifacts_root / ".tmp" / "lca_smoke.session").exists())
            self.assertFalse((artifacts_root / ".tmp" / "lca_smoke.tmp.interrupted").exists())
            self.assertFalse((artifacts_root / ".tmp" / "lca_smoke.run.interrupted").exists())
            self.assertIn("public_status=PASS", status_summary.read_text(encoding="utf-8"))

    def test_launcher_lock_blocks_concurrent_reentry_before_inner_dispatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
                SMOKE_ROOT="$ARTIFACTS_ROOT/smoke"
                COUNT_PATH="$ARTIFACTS_ROOT/dispatch_count.txt"
                READY_PATH="$ARTIFACTS_ROOT/launcher_lock_ready.txt"
                count=0

                mkdir -p "$ARTIFACTS_ROOT"
                if [[ -f "$COUNT_PATH" ]]; then
                  count="$(cat "$COUNT_PATH")"
                fi
                count=$(( count + 1 ))
                printf '%s\\n' "$count" > "$COUNT_PATH"

                if (( count == 1 )); then
                  : > "$READY_PATH"
                  sleep 1
                fi

                rm -f "$READY_PATH"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed1\\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            ready_path = artifacts_root / "launcher_lock_ready.txt"
            count_path = artifacts_root / "dispatch_count.txt"
            smoke_suite_config = artifacts_root / "smoke" / "suite_config.txt"
            second_env = os.environ.copy()
            second_env["LCA_SMOKE_LAUNCHER_LOCK_TIMEOUT_S"] = "0.2"

            first = subprocess.Popen(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            deadline = time.monotonic() + 10.0
            while time.monotonic() < deadline:
                if ready_path.exists():
                    break
                time.sleep(0.05)
            self.assertTrue(
                ready_path.exists(),
                msg="the first launcher invocation never reached the concurrent-dispatch hold point",
            )

            second = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
                env=second_env,
            )

            self.assertEqual(
                second.returncode,
                70,
                msg="a concurrent retry must fail fast with a stable harness status instead of re-entering shared cleanup and dispatch",
            )
            self.assertIn(
                "another lca_smoke.sh launcher run is active",
                second.stderr,
            )
            self.assertEqual(
                count_path.read_text(encoding="utf-8"),
                "1\n",
                msg="the concurrent retry must be blocked before it can dispatch a second inner smoke wrapper",
            )

            try:
                _, first_stderr = first.communicate(timeout=10)
            except subprocess.TimeoutExpired:
                first.kill()
                _, first_stderr = first.communicate()
                self.fail(f"the first launcher run did not complete after the concurrent retry check:\\n{first_stderr}")

            self.assertEqual(first.returncode, 0, msg=first_stderr)
            self.assertTrue(
                smoke_suite_config.is_file(),
                msg="the first launcher run must still finish and publish its smoke suite artifacts after a concurrent retry is rejected",
            )
            self.assertIn("case_count=1", smoke_suite_config.read_text(encoding="utf-8"))

    def test_launcher_normalizes_artifact_root_before_repo_relative_checks(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                artifacts_root_render='print(str(ARTIFACTS_ROOT) + "/.")',
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            status_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            self.assertTrue(status_summary.is_file(), msg="launcher must still publish status after normalizing the artifact root")
            self.assertIn(
                f"artifacts_root={(branch_root / 'artifacts' / 'lca_tree_stress_v5').resolve()}",
                status_summary.read_text(encoding="utf-8"),
                msg="status artifacts must record the canonical branch-local artifact root after normalization",
            )

    def test_launcher_resolves_repo_root_from_entrypoint_instead_of_current_working_directory(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root = self.make_fake_branch(temp_root)
            launch_from = temp_root / "outside_launcher_cwd"
            launch_from.mkdir()

            result = subprocess.run(
                [str(branch_root / "lca_smoke.sh")],
                cwd=launch_from,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            status_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            self.assertIn(
                f"branch_root={branch_root.resolve()}",
                status_summary.read_text(encoding="utf-8"),
                msg="launcher status must record the repo root derived from the entrypoint path, not the caller cwd",
            )

    def test_launcher_preflight_failure_runs_from_branch_root_while_preserving_original_launch_cwd(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root = self.make_fake_branch(
                temp_root,
                smoke_cases_contents=(
                    "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                    "smoke\tcomb_core\t64\toops\t1\t1\t2\n"
                ),
            )
            launch_from = temp_root / "outside_launcher_cwd"
            launch_from.mkdir()

            result = subprocess.run(
                [str(branch_root / "lca_smoke.sh")],
                cwd=launch_from,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            expected_workdir = f"working_directory={branch_root.resolve()}"
            expected_original = f"original_launch_working_directory={launch_from.resolve()}"
            status_summary = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "smoke_latest_status"
                / "summary.txt"
            ).read_text(encoding="utf-8")
            failure_summary = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "smoke_launcher_latest_failure"
                / "failure_summary.txt"
            ).read_text(encoding="utf-8")
            self.assertIn(
                expected_workdir,
                status_summary,
                msg="launcher status on preflight failure must reflect the normalized branch-root working directory",
            )
            self.assertIn(
                expected_original,
                status_summary,
                msg="launcher status must still preserve the caller cwd for replay context",
            )
            self.assertIn(
                expected_workdir,
                failure_summary,
                msg="launcher failure bundles must record the normalized branch-root working directory",
            )
            self.assertIn(
                expected_original,
                failure_summary,
                msg="launcher failure bundles must preserve the original caller cwd separately from the normalized branch-root cwd",
            )

    def test_launcher_runtime_preserves_supported_clean_env_overrides_into_inner_wrapper(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root = self.make_fake_branch(
                temp_root,
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                CAPTURE_PATH="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/launcher_env_capture.txt"
                mkdir -p "$(dirname "$CAPTURE_PATH")"
                {
                  printf 'debug_manifest=%s\n' "${LCA_SMOKE_DEBUG_MANIFEST:-}"
                  printf 'build_timeout=%s\n' "${LCA_SMOKE_BUILD_TIMEOUT_S:-}"
                  printf 'launcher_timeout=%s\n' "${LCA_SMOKE_LAUNCHER_TIMEOUT_S:-}"
                  printf 'export_snapshot_root=%s\n' "${LCA_SMOKE_EXPORT_SNAPSHOT_ROOT:-}"
                  printf 'original_command=%s\n' "${LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND:-}"
                  printf 'original_pwd=%s\n' "${LCA_SMOKE_LAUNCHER_ORIGINAL_PWD:-}"
                  printf 'launcher_clean_env=%s\n' "${LCA_SMOKE_LAUNCHER_CLEAN_ENV_READY:-}"
                  printf 'inner_clean_env=%s\n' "${LCA_SMOKE_CLEAN_ENV_READY:-}"
                  printf 'term=%s\n' "${TERM:-}"
                  printf 'tmpdir=%s\n' "${TMPDIR:-}"
                  printf 'home=%s\n' "${HOME:-}"
                  printf 'pycache=%s\n' "${PYTHONPYCACHEPREFIX:-}"
                  printf 'unrelated=%s\n' "${UNRELATED_SHOULD_BE_DROPPED:-}"
                  printf 'pwd=%s\n' "$PWD"
                } > "$CAPTURE_PATH"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\tcase_tag\n1\tsmoke_comb_core_n64_seed1\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )
            launch_from = temp_root / "outside_launcher_cwd"
            launch_from.mkdir()
            launcher_tmpdir = branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp" / "lca_smoke.launcher.tmp"
            self.write_text(launcher_tmpdir / "stale.txt", "stale launcher tmpdir\n")
            export_snapshot_root = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "snapshot_exports" / "latest"
            ).resolve()
            debug_manifest = (branch_root / "debug_smoke_cases.tsv").resolve()
            self.write_text(
                debug_manifest,
                (
                    "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                    "smoke\tcomb_core\t32\t99\t0\t1\t4\n"
                ),
            )
            env = os.environ.copy()
            env["LCA_SMOKE_DEBUG_MANIFEST"] = "debug_smoke_cases.tsv"
            env["LCA_SMOKE_BUILD_TIMEOUT_S"] = "17"
            env["LCA_SMOKE_LAUNCHER_TIMEOUT_S"] = "19"
            env["LCA_SMOKE_EXPORT_SNAPSHOT_ROOT"] = "artifacts/lca_tree_stress_v5/snapshot_exports/latest"
            host_tmpdir = (temp_root / "host_tmp").resolve()
            host_home = (temp_root / "host_home").resolve()
            host_pycache = (temp_root / "host_pycache").resolve()
            host_tmpdir.mkdir()
            host_home.mkdir()
            host_pycache.mkdir()
            env["TERM"] = "xterm-256color"
            env["TMPDIR"] = str(host_tmpdir)
            env["TMP"] = str(host_tmpdir)
            env["TEMP"] = str(host_tmpdir)
            env["HOME"] = str(host_home)
            env["PYTHONPYCACHEPREFIX"] = str(host_pycache)
            env["UNRELATED_SHOULD_BE_DROPPED"] = "1"

            result = subprocess.run(
                [str(branch_root / "lca_smoke.sh")],
                cwd=launch_from,
                capture_output=True,
                text=True,
                check=False,
                env=env,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertFalse(
                launcher_tmpdir.exists(),
                msg="launcher success must clear stale launcher tmpdirs before and after dispatch",
            )
            capture_path = branch_root / "artifacts" / "lca_tree_stress_v5" / "launcher_env_capture.txt"
            captured = dict(
                line.split("=", 1)
                for line in capture_path.read_text(encoding="utf-8").splitlines()
                if "=" in line
            )
            self.assertEqual(captured["debug_manifest"], str(debug_manifest))
            self.assertEqual(captured["build_timeout"], "17")
            self.assertEqual(captured["launcher_timeout"], "19")
            self.assertEqual(captured["export_snapshot_root"], str(export_snapshot_root))
            self.assertIn("lca_smoke.sh", captured["original_command"])
            self.assertEqual(Path(captured["original_pwd"]).resolve(), launch_from.resolve())
            self.assertEqual(captured["launcher_clean_env"], "1")
            self.assertEqual(captured["inner_clean_env"], "1")
            self.assertEqual(captured["term"], "dumb")
            self.assertNotEqual(Path(captured["tmpdir"]).resolve(), host_tmpdir)
            self.assertNotEqual(Path(captured["home"]).resolve(), host_home)
            self.assertNotEqual(Path(captured["pycache"]).resolve(), host_pycache)
            self.assertEqual(
                Path(captured["tmpdir"]).resolve(),
                launcher_tmpdir.resolve(),
                msg="launcher tmp usage must reuse the stable branch-local launcher tmp root",
            )
            self.assertEqual(
                Path(captured["home"]).resolve().parent,
                launcher_tmpdir.resolve(),
                msg="launcher HOME isolation must live under the stable launcher tmpdir",
            )
            self.assertEqual(captured["unrelated"], "")
            self.assertEqual(Path(captured["pwd"]).resolve(), branch_root.resolve())

    def test_launcher_runtime_pins_a_deterministic_preflight_environment(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                CAPTURE_PATH="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/launcher_preflight_env.txt"
                mkdir -p "$(dirname "$CAPTURE_PATH")"
                {
                  printf 'path=%s\n' "$PATH"
                  printf 'term=%s\n' "${TERM:-}"
                  printf 'lc_all=%s\n' "${LC_ALL:-}"
                  printf 'lang=%s\n' "${LANG:-}"
                  printf 'tz=%s\n' "${TZ:-}"
                  printf 'pythonhashseed=%s\n' "${PYTHONHASHSEED:-}"
                  printf 'pythonnousersite=%s\n' "${PYTHONNOUSERSITE:-}"
                  printf 'tmpdir=%s\n' "${TMPDIR:-}"
                  printf 'tmp=%s\n' "${TMP:-}"
                  printf 'temp=%s\n' "${TEMP:-}"
                  printf 'branch_artifact_tmp_root=%s\n' "${BRANCH_ARTIFACT_TMP_ROOT:-}"
                  printf 'home=%s\n' "${HOME:-}"
                  printf 'xdg_config_home=%s\n' "${XDG_CONFIG_HOME:-}"
                  printf 'xdg_cache_home=%s\n' "${XDG_CACHE_HOME:-}"
                  printf 'xdg_state_home=%s\n' "${XDG_STATE_HOME:-}"
                  printf 'pythonpycacheprefix=%s\n' "${PYTHONPYCACHEPREFIX:-}"
                  printf 'pwd=%s\n' "$PWD"
                } > "$CAPTURE_PATH"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\tcase_tag\n1\tsmoke_comb_core_n64_seed1\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            capture_path = branch_root / "artifacts" / "lca_tree_stress_v5" / "launcher_preflight_env.txt"
            captured = dict(
                line.split("=", 1)
                for line in capture_path.read_text(encoding="utf-8").splitlines()
                if "=" in line
            )
            launcher_tmpdir = Path(captured["branch_artifact_tmp_root"]).resolve()
            stable_launcher_tmpdir = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp" / "lca_smoke.launcher.tmp"
            ).resolve()

            self.assertEqual(
                captured["path"],
                "/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin",
            )
            self.assertEqual(captured["term"], "dumb")
            self.assertEqual(captured["lc_all"], "C")
            self.assertEqual(captured["lang"], "C")
            self.assertEqual(captured["tz"], "UTC")
            self.assertEqual(captured["pythonhashseed"], "0")
            self.assertEqual(captured["pythonnousersite"], "1")
            self.assertEqual(Path(captured["tmpdir"]).resolve(), launcher_tmpdir)
            self.assertEqual(Path(captured["tmp"]).resolve(), launcher_tmpdir)
            self.assertEqual(Path(captured["temp"]).resolve(), launcher_tmpdir)
            self.assertEqual(launcher_tmpdir, stable_launcher_tmpdir)
            self.assertEqual(Path(captured["home"]).resolve().parent, stable_launcher_tmpdir)
            self.assertEqual(Path(captured["xdg_config_home"]).resolve().parent, stable_launcher_tmpdir)
            self.assertEqual(Path(captured["xdg_cache_home"]).resolve().parent, stable_launcher_tmpdir)
            self.assertEqual(Path(captured["xdg_state_home"]).resolve().parent, stable_launcher_tmpdir)
            self.assertEqual(Path(captured["pythonpycacheprefix"]).resolve().parent, stable_launcher_tmpdir)
            self.assertEqual(Path(captured["pwd"]).resolve(), branch_root.resolve())

    def test_launcher_repairs_symlinked_smoke_artifact_namespace_before_preflight(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root = self.make_fake_branch(
                temp_root,
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                CAPTURE_PATH="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/launcher_env_capture.txt"
                mkdir -p "$(dirname "$CAPTURE_PATH")"
                {
                  printf 'tmpdir=%s\n' "${TMPDIR:-}"
                  printf 'branch_artifact_tmp_root=%s\n' "${BRANCH_ARTIFACT_TMP_ROOT:-}"
                  printf 'pwd=%s\n' "$PWD"
                } > "$CAPTURE_PATH"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\tcase_tag\n1\tsmoke_comb_core_n64_seed1\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )
            artifact_namespace = branch_root / "artifacts" / "lca_tree_stress_v5"
            poisoned_namespace = (temp_root / "poisoned_smoke_artifacts").resolve()
            poisoned_namespace.mkdir(parents=True, exist_ok=True)
            self.write_text(poisoned_namespace / "stale.txt", "stale\n")
            artifact_namespace.parent.mkdir(parents=True, exist_ok=True)
            artifact_namespace.symlink_to(poisoned_namespace, target_is_directory=True)

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(
                artifact_namespace.is_dir(),
                msg="launcher must recreate the public smoke artifact namespace as a real directory before preflight writes",
            )
            self.assertFalse(
                artifact_namespace.is_symlink(),
                msg="launcher must not keep a symlinked smoke artifact namespace between reruns",
            )
            capture_path = artifact_namespace / "launcher_env_capture.txt"
            self.assertTrue(
                capture_path.is_file(),
                msg="launcher preflight writes must land in the repaired branch-local smoke artifact namespace",
            )
            captured = dict(
                line.split("=", 1)
                for line in capture_path.read_text(encoding="utf-8").splitlines()
                if "=" in line
            )
            expected_launcher_tmpdir = (artifact_namespace / ".tmp" / "lca_smoke.launcher.tmp").resolve()
            self.assertEqual(
                Path(captured["tmpdir"]).resolve(),
                expected_launcher_tmpdir,
                msg="launcher TMPDIR must stay pinned under the repaired branch-local smoke artifact namespace",
            )
            self.assertEqual(
                Path(captured["branch_artifact_tmp_root"]).resolve(),
                expected_launcher_tmpdir,
                msg="launcher BRANCH_ARTIFACT_TMP_ROOT must stay pinned under the repaired branch-local smoke artifact namespace",
            )
            self.assertEqual(Path(captured["pwd"]).resolve(), branch_root.resolve())
            self.assertFalse(
                (poisoned_namespace / "launcher_env_capture.txt").exists(),
                msg="launcher must not write preflight artifacts through the stale symlink target",
            )
            self.assertFalse(
                (poisoned_namespace / "smoke" / "suite_config.txt").exists(),
                msg="launcher must not publish smoke output through the stale symlink target",
            )
            self.assertEqual(
                (poisoned_namespace / "stale.txt").read_text(encoding="utf-8"),
                "stale\n",
                msg="repairing the smoke artifact namespace must remove only the stale symlink, not mutate the old external target",
            )

    def test_launcher_debug_manifest_override_drives_preflight_selection(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                smoke_cases_contents=(
                    "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                    "smoke\tcomb_core\t64\toops\t1\t1\t2\n"
                ),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\tcase_tag\n1\tdebug_case\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )
            debug_manifest = branch_root / "debug_smoke_cases.tsv"
            self.write_text(
                debug_manifest,
                (
                    "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                    "smoke\tcomb_sparse\t32\t7\t0\t1\t4\n"
                ),
            )
            env = os.environ.copy()
            env["LCA_SMOKE_DEBUG_MANIFEST"] = "debug_smoke_cases.tsv"

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
                env=env,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            run_archives = sorted(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_runs").glob("run-*")
            )
            self.assertEqual(len(run_archives), 1)
            selection_summary = (
                run_archives[0] / "launcher_preflight" / "smoke_manifest_selection.txt"
            ).read_text(encoding="utf-8")
            self.assertIn(f"manifest_path={debug_manifest.resolve()}", selection_summary)
            self.assertIn("case_count=1", selection_summary)
            self.assertIn("input_policy=debug_manifest_override", selection_summary)
            self.assertIn("case01=smoke\tcomb_sparse\t32\t7\t0\t1\t4", selection_summary)

    def test_launcher_ignores_stale_inherited_internal_clean_env_markers_on_fresh_run(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            branch_root = self.make_fake_branch(
                temp_root,
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                CAPTURE_PATH="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/launcher_env_capture.txt"
                mkdir -p "$(dirname "$CAPTURE_PATH")"
                {
                  printf 'original_command=%s\n' "${LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND:-}"
                  printf 'original_pwd=%s\n' "${LCA_SMOKE_LAUNCHER_ORIGINAL_PWD:-}"
                  printf 'launcher_clean_env=%s\n' "${LCA_SMOKE_LAUNCHER_CLEAN_ENV_READY:-}"
                  printf 'inner_clean_env=%s\n' "${LCA_SMOKE_CLEAN_ENV_READY:-}"
                  printf 'pwd=%s\n' "$PWD"
                } > "$CAPTURE_PATH"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\tcase_tag\n1\tsmoke_comb_core_n64_seed1\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )
            launch_from = temp_root / "fresh_launcher_cwd"
            launch_from.mkdir()
            env = os.environ.copy()
            env["LCA_SMOKE_LAUNCHER_CLEAN_ENV_READY"] = "1"
            env["LCA_SMOKE_CLEAN_ENV_READY"] = "1"
            env["LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND"] = "stale command"
            env["LCA_SMOKE_LAUNCHER_ORIGINAL_PWD"] = "/tmp/stale-launch-pwd"

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
                env=env,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            capture_path = branch_root / "artifacts" / "lca_tree_stress_v5" / "launcher_env_capture.txt"
            captured = dict(
                line.split("=", 1)
                for line in capture_path.read_text(encoding="utf-8").splitlines()
                if "=" in line
            )
            self.assertNotEqual(captured["original_command"], "stale command")
            self.assertEqual(Path(captured["original_pwd"]).resolve(), branch_root.resolve())
            self.assertEqual(captured["launcher_clean_env"], "1")
            self.assertEqual(captured["inner_clean_env"], "1")
            self.assertEqual(Path(captured["pwd"]).resolve(), branch_root.resolve())

    def test_launcher_rejects_repo_root_resume_workspace_directory_misconfiguration_before_dispatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            shutil.rmtree(branch_root / "boj28350_resume")
            self.write_text(branch_root / "boj28350_resume", "not a directory\n")

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_launcher_latest_failure"
            failure_summary = (failure_root / "failure_summary.txt").read_text(encoding="utf-8")
            preflight_manifest = (failure_root / "preflight_manifest.tsv").read_text(encoding="utf-8")
            self.assertIn("resume workspace directory is not a directory", failure_summary)
            self.assertIn(
                (
                    "directory\tresume workspace directory\tnot_directory\t"
                    f"{(branch_root / 'boj28350_resume').resolve()}\t-"
                ),
                preflight_manifest,
            )

    def test_launcher_rejects_invalid_smoke_manifest_seed_before_dispatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                smoke_cases_contents=(
                    "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                    "smoke\tcomb_core\t64\toops\t1\t1\t2\n"
                ),
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            status_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            self.assertTrue(status_summary.is_file(), msg="launcher preflight failures must still publish the stable status summary")
            status_summary_text = status_summary.read_text(encoding="utf-8")
            status_report_text = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "latest_status_report.md"
            ).read_text(encoding="utf-8")
            self.assertIn("failure_partition=harness_setup", status_summary_text)
            self.assertIn("failure_partition_label=harness/setup", status_summary_text)
            self.assertIn("normalized_outcome=harness_infrastructure_failure", status_summary_text)
            run_record_payload = json.loads(
                (
                    branch_root
                    / "artifacts"
                    / "lca_tree_stress_v5"
                    / "smoke_latest_status"
                    / "run_record.json"
                ).read_text(encoding="utf-8")
            )
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_launcher_latest_failure"
            failure_summary = failure_root / "failure_summary.txt"
            manifest_stderr = failure_root / "smoke_manifest_check.stderr.txt"
            self.assertTrue(failure_summary.is_file(), msg="launcher manifest failures must preserve the failure summary")
            self.assertTrue(manifest_stderr.is_file(), msg="launcher manifest failures must preserve the manifest-check stderr")
            self.assertIn(
                "invalid smoke case manifest",
                failure_summary.read_text(encoding="utf-8"),
            )
            self.assertIn(
                "invalid seed: oops",
                manifest_stderr.read_text(encoding="utf-8"),
            )
            self.assertIn("- Failure partition: `harness/setup`", status_report_text)
            self.assertEqual(run_record_payload["summary"]["result_family"], "harness")
            self.assertEqual(run_record_payload["summary"]["failure_partition"], "harness_setup")
            self.assertEqual(run_record_payload["summary"]["failure_partition_label"], "harness/setup")
            self.assertEqual(run_record_payload["summary"]["normalized_exit_code"], 70)
            self.assertEqual(
                run_record_payload["summary"]["normalized_outcome"],
                "harness_infrastructure_failure",
            )
            self.assertIn(
                "[lca_smoke] failure partition: harness/setup public_exit=70 raw_exit=70",
                result.stderr,
            )

    def test_launcher_preflight_failure_preserves_prior_public_smoke_output_until_validation_passes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                smoke_cases_contents=(
                    "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                    "smoke\tcomb_core\t64\toops\t1\t1\t2\n"
                ),
            )
            smoke_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke"
            self.write_text(smoke_root / "stale.txt", "prior smoke output\n")

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            self.assertTrue(
                smoke_root.is_dir(),
                msg="launcher-side preflight failures must not clear the prior public smoke root before validation passes",
            )
            self.assertEqual(
                (smoke_root / "stale.txt").read_text(encoding="utf-8"),
                "prior smoke output\n",
                msg="launcher-side preflight failures must preserve prior public smoke artifacts until dispatch ownership begins",
            )
            self.assertTrue(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt").is_file(),
                msg="launcher-side preflight failures must still publish a fresh status summary",
            )

    def test_launcher_preflight_uses_a_run_local_tmpdir_under_the_stable_branch_tmp_parent(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                smoke_cases_contents=(
                    "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                    "smoke\tcomb_core\t64\toops\t1\t1\t2\n"
                ),
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_launcher_latest_failure"
            env_snapshot = failure_root / "launcher_env.txt"
            self.assertTrue(env_snapshot.is_file(), msg="launcher preflight failures must preserve the environment snapshot")
            captured = dict(
                line.split("=", 1)
                for line in env_snapshot.read_text(encoding="utf-8").splitlines()
                if "=" in line
            )
            stable_launcher_tmpdir = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp" / "lca_smoke.launcher.tmp"
            ).resolve()
            launcher_tmpdir = Path(captured["launcher_tmpdir"]).resolve()
            launcher_preflight_root = Path(captured["launcher_preflight_root"]).resolve()
            launcher_home = Path(captured["launcher_home"]).resolve()

            self.assertEqual(
                launcher_tmpdir,
                stable_launcher_tmpdir,
                msg="launcher preflight must reuse the fixed branch-local launcher tmp root",
            )
            self.assertEqual(
                launcher_preflight_root.parent,
                launcher_tmpdir,
                msg="launcher preflight artifacts must live under the stable launcher tmpdir",
            )
            self.assertEqual(
                launcher_home.parent,
                launcher_tmpdir,
                msg="launcher HOME isolation must live under the stable launcher tmpdir",
            )
            self.assertFalse(
                stable_launcher_tmpdir.exists(),
                msg="launcher teardown must remove the stable launcher tmpdir after dispatch completes",
            )

    def test_launcher_rejects_broken_release_env_wrapper_before_dispatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            self.write_text(
                branch_root / "solver_release_env.sh",
                "#!/usr/bin/env bash\nif then\n",
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            status_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            self.assertTrue(status_summary.is_file(), msg="launcher shell-preflight failures must still publish the stable status summary")
            self.assertIn(
                "normalized_outcome=harness_infrastructure_failure",
                status_summary.read_text(encoding="utf-8"),
            )
            failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_launcher_latest_failure"
            failure_summary = (failure_root / "failure_summary.txt").read_text(encoding="utf-8")
            preflight_manifest = (failure_root / "preflight_manifest.tsv").read_text(encoding="utf-8")
            syntax_stderr = failure_root / "release_env_wrapper_syntax.stderr.txt"
            self.assertTrue(
                syntax_stderr.is_file(),
                msg="launcher shell-preflight failures must preserve the shell parser stderr for the broken prerequisite",
            )
            self.assertIn("broken release env wrapper syntax", failure_summary)
            self.assertIn(
                (
                    "shell_syntax\trelease env wrapper syntax\tbroken\t"
                    f"{(branch_root / 'solver_release_env.sh').resolve()}\t{syntax_stderr.resolve()}"
                ),
                preflight_manifest,
            )
            self.assertIn("syntax error", syntax_stderr.read_text(encoding="utf-8"))
            status_summary_text = status_summary.read_text(encoding="utf-8")
            status_report_text = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "latest_status_report.md"
            ).read_text(encoding="utf-8")
            self.assertIn("triage_stage_scope=launcher_pre_dispatch", status_summary_text)
            self.assertIn("triage_stage=shell_entrypoint_validation", status_summary_text)
            self.assertIn("triage_retry_command=./lca_smoke.sh", status_summary_text)
            self.assertIn("## Failed Stage", status_report_text)
            self.assertIn("- Failed stage: `shell_entrypoint_validation`", status_report_text)
            self.assertIn("- Retry command: `./lca_smoke.sh`", status_report_text)
            self.assertIn(
                "[lca_smoke] failed stage: shell_entrypoint_validation scope=launcher_pre_dispatch",
                result.stderr,
            )
            self.assertIn("[lca_smoke] retry next: ./lca_smoke.sh", result.stderr)

    def test_launcher_dispatch_result_survives_inner_wrapper_removing_launcher_tmpdir(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                rm -rf "$BRANCH_ROOT/artifacts/lca_tree_stress_v5/.tmp/lca_smoke.launcher.tmp"
                SMOKE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed1\\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(
                result.returncode,
                0,
                msg="launcher must keep its dispatch-result handoff outside the volatile launcher tmpdir",
            )
            status_summary = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            ).read_text(encoding="utf-8")
            self.assertIn("public_status=PASS", status_summary)
            self.assertIn("normalized_outcome=pass", status_summary)
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_launcher_latest_failure").exists(),
                msg="tmpdir loss during an otherwise successful inner run must not be reclassified as a launcher failure",
            )

    def test_interrupted_dispatch_kills_inner_wrapper_and_allows_same_worktree_rerun(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
                COUNT_PATH="$ARTIFACTS_ROOT/interrupt_count.txt"
                PID_PATH="$ARTIFACTS_ROOT/interrupt_inner_pid.txt"
                SIGNAL_PATH="$ARTIFACTS_ROOT/interrupt_inner_signal.txt"
                count=0

                mkdir -p "$ARTIFACTS_ROOT"
                if [[ -f "$COUNT_PATH" ]]; then
                  count="$(cat "$COUNT_PATH")"
                fi
                count=$(( count + 1 ))
                printf '%s\\n' "$count" > "$COUNT_PATH"

                if (( count == 1 )); then
                  trap 'printf "SIGTERM\\n" > "$SIGNAL_PATH"; exit 143' TERM
                  trap 'printf "SIGINT\\n" > "$SIGNAL_PATH"; exit 130' INT
                  trap 'printf "SIGHUP\\n" > "$SIGNAL_PATH"; exit 129' HUP
                  printf '%s\\n' "$$" > "$PID_PATH"
                  sleep 10
                  exit 0
                fi

                SMOKE_ROOT="$ARTIFACTS_ROOT/smoke"
                rm -rf "$SMOKE_ROOT"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed1\\n' > "$SMOKE_ROOT/suite_plan.tsv"
                exit 0
                """,
            )
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            pid_path = artifacts_root / "interrupt_inner_pid.txt"
            signal_path = artifacts_root / "interrupt_inner_signal.txt"

            proc = subprocess.Popen(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            deadline = time.time() + 10.0
            while time.time() < deadline:
                if pid_path.exists():
                    break
                if proc.poll() is not None:
                    break
                time.sleep(0.05)
            if not pid_path.exists():
                stdout, stderr = proc.communicate(timeout=5)
                self.fail(
                    "timed out waiting for path: "
                    f"{pid_path}\nstdout={stdout}\nstderr={stderr}"
                )
            inner_pid = int(pid_path.read_text(encoding="utf-8").strip())
            os.kill(proc.pid, signal.SIGTERM)
            stdout, stderr = proc.communicate(timeout=10)

            self.assertEqual(
                proc.returncode,
                70,
                msg="interrupted dispatches must normalize to a retry-safe harness failure instead of hanging\n"
                f"stdout={stdout}\nstderr={stderr}",
            )
            self.wait_for_path(signal_path)
            self.assertEqual(signal_path.read_text(encoding="utf-8").strip(), "SIGTERM")
            self.wait_for_pid_exit(inner_pid)
            self.assertIn(
                "launcher received SIGTERM while waiting for inner smoke wrapper",
                stderr,
                msg="the launcher should publish a stable interruption summary for retry analysis",
            )

            rerun = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(
                rerun.returncode,
                0,
                msg="a rerun after an interrupted dispatch must succeed without manual artifact cleanup",
            )
            status_summary = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            ).read_text(encoding="utf-8")
            self.assertIn("public_status=PASS", status_summary)
            self.assertIn("normalized_outcome=pass", status_summary)

    def test_launcher_zero_exit_without_fresh_smoke_bundle_normalizes_to_harness_failure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                exit 0
                """,
            )
            smoke_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke"
            smoke_root.mkdir(parents=True, exist_ok=True)
            self.write_text(smoke_root / "suite_config.txt", "stale smoke suite config\n")
            self.write_text(smoke_root / "suite_plan.tsv", "case_index\tcase_tag\n1\tstale_case\n")

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(
                result.returncode,
                70,
                msg="launcher must not trust a zero exit when the inner wrapper did not publish a fresh smoke bundle",
            )
            status_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            status_summary_text = status_summary.read_text(encoding="utf-8")
            self.assertIn("public_status=FAIL", status_summary_text)
            self.assertIn("result_family=harness", status_summary_text)
            self.assertIn("normalized_exit_code=70", status_summary_text)
            self.assertIn("raw_exit_code=0", status_summary_text)
            self.assertIn("normalized_outcome=harness_infrastructure_failure", status_summary_text)
            self.assertIn("returned success without publishing a fresh smoke bundle", status_summary_text)
            self.assertIn("[lca_smoke] public status: FAIL family=harness", result.stderr)

    def test_launcher_failure_run_clears_stale_public_smoke_root_before_publishing_status(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                mkdir -p "$FAILURE_ROOT"
                cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
                failure_summary=fresh failure bundle after smoke-root cleanup
                failure_kind=solver_timeout
                failure_origin=solver
                failure_retryable=1
                failure_stage=smoke
                EOF
                printf '# fresh failure bundle after smoke-root cleanup\\n' > "$FAILURE_ROOT/latest_failure_report.md"
                exit 124
                """,
            )
            smoke_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke"
            smoke_root.mkdir(parents=True, exist_ok=True)
            self.write_text(smoke_root / "suite_config.txt", "stale smoke suite config\n")
            self.write_text(smoke_root / "suite_plan.tsv", "case_index\tcase_tag\n1\tstale_case\n")
            self.write_text(smoke_root / "stale_only.txt", "stale smoke payload\n")

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(
                result.returncode,
                124,
                msg="launcher must preserve the normalized solver failure while still resetting the public smoke root",
            )
            self.assertTrue(
                (smoke_root / "summary.txt").is_file(),
                msg="launcher must republish the current run summary into a fresh smoke root",
            )
            self.assertTrue(
                (smoke_root / "status_report.md").is_file(),
                msg="launcher must republish the current run report into a fresh smoke root",
            )
            self.assertFalse(
                (smoke_root / "suite_config.txt").exists(),
                msg="failure runs must not leave stale suite config behind from a previous smoke bundle",
            )
            self.assertFalse(
                (smoke_root / "suite_plan.tsv").exists(),
                msg="failure runs must not leave stale suite plans behind from a previous smoke bundle",
            )
            self.assertFalse(
                (smoke_root / "stale_only.txt").exists(),
                msg="failure runs must start from a clean smoke root instead of carrying unrelated stale files forward",
            )
            self.assertIn(
                "public_status=FAIL",
                (smoke_root / "summary.txt").read_text(encoding="utf-8"),
            )

    def test_launcher_timeout_without_complete_fresh_failure_bundle_normalizes_to_harness_failure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                mkdir -p "$FAILURE_ROOT"
                printf 'failure_summary=partial timeout failure bundle\\n' > "$FAILURE_ROOT/failure_summary.txt"
                exit 124
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(
                result.returncode,
                70,
                msg="launcher must normalize timeout exits to harness failure when the inner wrapper left only a partial failure bundle",
            )
            status_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            status_summary_text = status_summary.read_text(encoding="utf-8")
            self.assertIn("public_status=FAIL", status_summary_text)
            self.assertIn("result_family=harness", status_summary_text)
            self.assertIn("normalized_exit_code=70", status_summary_text)
            self.assertIn("raw_exit_code=124", status_summary_text)
            self.assertIn("normalized_outcome=harness_infrastructure_failure", status_summary_text)
            self.assertIn("without publishing a complete fresh failure bundle", status_summary_text)
            self.assertIn("missing failure report", status_summary_text)
            self.assertIn("triage_stage_scope=inner_wrapper_bundle_validation", status_summary_text)
            self.assertIn("triage_stage=bundle_validation", status_summary_text)
            self.assertIn("triage_retry_command=./lca_smoke.sh", status_summary_text)
            self.assertIn("[lca_smoke] public status: FAIL family=harness", result.stderr)
            self.assertIn(
                "[lca_smoke] failed stage: bundle_validation scope=inner_wrapper_bundle_validation",
                result.stderr,
            )

    def test_launcher_partial_failure_bundle_backfills_failed_case_row_for_iteration_context(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                mkdir -p "$FAILURE_ROOT"
                printf 'stage\\tmode\\tn\\tseed\\tshuffle_labels\\tshuffle_queries\\ttimeout_s\\nsmoke\\tcomb_core\\t64\\t123\\t1\\t0\\t2\\n' > "$FAILURE_ROOT/failed_case_row.tsv"
                printf 'query #7 expected=4 got=3\\n' > "$FAILURE_ROOT/mismatch_summary.txt"
                cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
                failure_summary=validator mismatch before failure report completed
                failure_kind=solver_acceptance_failure
                failure_origin=validator
                failure_retryable=0
                EOF
                exit 1
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(
                result.returncode,
                70,
                msg="launcher must treat partial solver bundles as harness failures while still surfacing the preserved failing case",
            )
            failure_root = (branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_failure").resolve()
            failed_case_row = (failure_root / "failed_case_row.tsv").resolve()
            mismatch_summary = (failure_root / "mismatch_summary.txt").resolve()
            status_summary = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            ).read_text(encoding="utf-8")

            self.assertIn("public_status=FAIL", status_summary)
            self.assertIn("result_family=harness", status_summary)
            self.assertIn("normalized_exit_code=70", status_summary)
            self.assertIn("raw_exit_code=1", status_summary)
            self.assertIn("normalized_outcome=harness_infrastructure_failure", status_summary)
            self.assertIn("source_failure_summary=validator mismatch before failure report completed", status_summary)
            self.assertIn(
                f"source_failure_failed_case_row_path={failed_case_row}",
                status_summary,
            )
            self.assertIn(
                f"source_failure_mismatch_summary_path={mismatch_summary}",
                status_summary,
            )
            self.assertIn(
                "source_failure_case=stage=smoke mode=comb_core n=64 seed=123",
                status_summary,
            )
            self.assertIn(
                "[lca_smoke] outcome summary: inner smoke wrapper returned a solver-side result without publishing a complete fresh failure bundle:",
                result.stderr,
            )
            self.assertIn(
                "[lca_smoke] replay summary: validator mismatch before failure report completed",
                result.stderr,
            )
            self.assertIn(
                "[lca_smoke] replay case: stage=smoke mode=comb_core n=64 seed=123",
                result.stderr,
            )
            self.assertIn(
                f"[lca_smoke] source failed-case row: {failed_case_row}",
                result.stderr,
            )
            self.assertIn(f"[lca_smoke] mismatch summary: {mismatch_summary}", result.stderr)

    def test_launcher_kills_hung_inner_wrapper_after_timeout_and_preserves_stable_harness_status(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                mkdir -p "$FAILURE_ROOT"
                printf 'failure_summary=partial hung failure bundle\\n' > "$FAILURE_ROOT/failure_summary.txt"
                trap '' TERM
                while :; do
                  sleep 0.05
                done
                """,
            )
            env = os.environ.copy()
            env["LCA_SMOKE_LAUNCHER_TIMEOUT_S"] = "0.2"

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
                env=env,
            )

            self.assertEqual(
                result.returncode,
                70,
                msg="hung inner-wrapper dispatches must normalize to the public harness exit code after launcher timeout cleanup",
            )
            status_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status"
            status_summary_text = (status_root / "summary.txt").read_text(encoding="utf-8")
            status_report_text = (status_root / "latest_status_report.md").read_text(encoding="utf-8")
            self.assertIn("public_status=FAIL", status_summary_text)
            self.assertIn("result_family=harness", status_summary_text)
            self.assertIn("normalized_exit_code=70", status_summary_text)
            self.assertIn("raw_exit_code=124", status_summary_text)
            self.assertIn("normalized_outcome=harness_infrastructure_failure", status_summary_text)
            self.assertIn("dispatch_timeout_s=0.2", status_summary_text)
            self.assertIn("last_check_status=timeout", status_summary_text)
            self.assertIn("last_check_detail=0.2", status_summary_text)
            self.assertIn("without publishing a complete fresh failure bundle", status_summary_text)
            self.assertIn("without publishing a complete fresh failure bundle", status_report_text)
            self.assertIn("[lca_smoke] public status: FAIL family=harness", result.stderr)
            self.assertIn("[lca_smoke] normalized outcome: harness_infrastructure_failure", result.stderr)

    def test_launcher_sigterm_during_dispatch_cleans_up_and_allows_retry(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
                STATE_ROOT="$ARTIFACTS_ROOT/dispatch_signal_test"
                COUNT_PATH="$STATE_ROOT/dispatch_count.txt"
                LOCK_PATH="$STATE_ROOT/inner_active.lock"
                SMOKE_ROOT="$ARTIFACTS_ROOT/smoke"
                count=0

                mkdir -p "$STATE_ROOT"
                if [[ -f "$COUNT_PATH" ]]; then
                  count="$(cat "$COUNT_PATH")"
                fi
                count=$(( count + 1 ))
                printf '%s\\n' "$count" > "$COUNT_PATH"

                if [[ -e "$LOCK_PATH" ]]; then
                  FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
                  mkdir -p "$FAILURE_ROOT"
                  cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
                failure_summary=stale interrupted inner wrapper remained active
                failure_kind=harness_transient_failure
                failure_origin=launcher
                failure_retryable=1
                EOF
                  printf '# stale interrupted inner wrapper remained active\\n' > "$FAILURE_ROOT/latest_failure_report.md"
                  exit 125
                fi

                cleanup() {
                  rm -f "$LOCK_PATH"
                }
                trap cleanup EXIT INT TERM HUP
                : > "$LOCK_PATH"

                if (( count == 1 )); then
                  while :; do
                    sleep 0.05
                  done
                fi

                rm -rf "$SMOKE_ROOT"
                mkdir -p "$SMOKE_ROOT"
                printf 'case_count=1\\n' > "$SMOKE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed1\\n' > "$SMOKE_ROOT/suite_plan.tsv"
                """,
            )
            lock_path = branch_root / "artifacts" / "lca_tree_stress_v5" / "dispatch_signal_test" / "inner_active.lock"
            dispatch_count_path = (
                branch_root / "artifacts" / "lca_tree_stress_v5" / "dispatch_signal_test" / "dispatch_count.txt"
            )
            status_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"

            first = subprocess.Popen(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            deadline = time.monotonic() + 5.0
            while time.monotonic() < deadline:
                if lock_path.exists():
                    break
                if first.poll() is not None:
                    self.fail("the first smoke run exited before the inner wrapper entered its dispatch loop")
                time.sleep(0.05)
            self.assertTrue(lock_path.exists(), msg="the first smoke run must reach the inner dispatch loop before SIGTERM")

            first.terminate()
            _, first_stderr = first.communicate(timeout=10)

            self.assertEqual(first.returncode, 70, msg=first_stderr)
            deadline = time.monotonic() + 5.0
            while time.monotonic() < deadline and lock_path.exists():
                time.sleep(0.05)
            self.assertFalse(
                lock_path.exists(),
                msg="SIGTERM cleanup must stop the in-flight inner wrapper before the next retry",
            )
            status_summary_text = status_summary.read_text(encoding="utf-8")
            self.assertIn("public_status=FAIL", status_summary_text)
            self.assertIn("result_family=harness", status_summary_text)
            self.assertIn("normalized_outcome=harness_infrastructure_failure", status_summary_text)
            self.assertIn("launcher received SIGTERM while waiting for inner smoke wrapper", status_summary_text)

            second = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(second.returncode, 0, msg=second.stderr)
            self.assertEqual(dispatch_count_path.read_text(encoding="utf-8"), "2\n")
            self.assertFalse(
                lock_path.exists(),
                msg="successful retry must not inherit any live lock from the interrupted prior run",
            )
            self.assertIn("public_status=PASS", status_summary.read_text(encoding="utf-8"))

    def test_launcher_complete_timeout_failure_bundle_stays_on_solver_failure_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(
                Path(tmp),
                inner_wrapper_body="""
                #!/usr/bin/env bash
                set -euo pipefail
                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
                FAILURE_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
                CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed123"
                mkdir -p "$CASE_DIR"
                printf 'commands\\n' > "$FAILURE_ROOT/commands.txt"
                printf 'artifact\\tpath\\n' > "$FAILURE_ROOT/artifact_manifest.tsv"
                printf 'rerun\\n' > "$FAILURE_ROOT/rerun_command.txt"
                printf 'seed\\n' > "$FAILURE_ROOT/seed.txt"
                printf 'input\\n' > "$FAILURE_ROOT/input.txt"
                printf 'output\\n' > "$FAILURE_ROOT/solver_output.txt"
                printf 'expected\\n' > "$FAILURE_ROOT/expected_output.txt"
                printf 'invoked\\n' > "$FAILURE_ROOT/invoked_command.txt"
                printf '{"schema":"lca_smoke_failure_context_v1"}\\n' > "$FAILURE_ROOT/failure_context.json"
                printf 'VAR=1\\n' > "$FAILURE_ROOT/runtime_env.txt"
                printf 'manifest\\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
                printf 'case_count=1\\n' > "$FAILURE_ROOT/suite_config.txt"
                printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed123\\n' > "$FAILURE_ROOT/suite_plan.tsv"
                printf '#!/usr/bin/env bash\\nexit 124\\n' > "$FAILURE_ROOT/replay_active_manifest_case.sh"
                chmod +x "$FAILURE_ROOT/replay_active_manifest_case.sh"
                cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
                failure_summary=timeout while replaying preserved smoke case
                failure_kind=solver_timeout
                failure_origin=solver
                failure_retryable=1
                failure_reporting_status=complete
                failure_reporting_warning=
                failed_case_tag=smoke_comb_core_n64_seed123
                failed_stage=smoke
                failed_mode=comb_core
                failed_n=64
                failed_seed=123
                failure_root=$FAILURE_ROOT
                failure_case_dir=$CASE_DIR
                commands_path=$FAILURE_ROOT/commands.txt
                artifact_manifest_path=$FAILURE_ROOT/artifact_manifest.tsv
                structured_context_path=$FAILURE_ROOT/failure_context.json
                rerun_command_path=$FAILURE_ROOT/rerun_command.txt
                exact_seed_path=$FAILURE_ROOT/seed.txt
                exact_input_path=$FAILURE_ROOT/input.txt
                exact_output_path=$FAILURE_ROOT/solver_output.txt
                expected_output_path=$FAILURE_ROOT/expected_output.txt
                invoked_command_path=$FAILURE_ROOT/invoked_command.txt
                runtime_env_path=$FAILURE_ROOT/runtime_env.txt
                manifest_snapshot_path=$FAILURE_ROOT/smoke_cases_manifest.tsv
                suite_config_path=$FAILURE_ROOT/suite_config.txt
                suite_plan_path=$FAILURE_ROOT/suite_plan.tsv
                active_solver_replay_script=$FAILURE_ROOT/replay_active_manifest_case.sh
                EOF
                printf '# timeout failure report\\n' > "$FAILURE_ROOT/latest_failure_report.md"
                exit 124
                """,
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(
                result.returncode,
                124,
                msg="complete timeout failure bundles must preserve the public solver-timeout exit code",
            )
            status_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status"
            status_summary_text = (status_root / "summary.txt").read_text(encoding="utf-8")
            run_record_payload = json.loads((status_root / "run_record.json").read_text(encoding="utf-8"))
            launcher_failure_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_launcher_latest_failure"
            self.assertIn("public_status=FAIL", status_summary_text)
            self.assertIn("result_family=solver", status_summary_text)
            self.assertIn("failure_partition=solver_test", status_summary_text)
            self.assertIn("failure_partition_label=solver/test", status_summary_text)
            self.assertIn("raw_exit_code=124", status_summary_text)
            self.assertIn("normalized_exit_code=124", status_summary_text)
            self.assertIn("normalized_outcome=reproducible_solver_failure", status_summary_text)
            self.assertIn(
                "source_failure_summary=timeout while replaying preserved smoke case",
                status_summary_text,
            )
            self.assertFalse(
                launcher_failure_root.exists(),
                msg="handled inner-wrapper timeout failures must not be reclassified as launcher-preflight failures",
            )
            self.assertIn(
                "- Failure partition: `solver/test`",
                (status_root / "latest_status_report.md").read_text(encoding="utf-8"),
            )
            self.assertEqual(run_record_payload["summary"]["result_family"], "solver")
            self.assertEqual(run_record_payload["summary"]["failure_partition"], "solver_test")
            self.assertEqual(run_record_payload["summary"]["failure_partition_label"], "solver/test")
            self.assertEqual(run_record_payload["summary"]["normalized_exit_code"], 124)
            self.assertEqual(
                run_record_payload["summary"]["normalized_outcome"],
                "reproducible_solver_failure",
            )
            self.assertEqual(run_record_payload["summary"]["source_failure_kind"], "solver_timeout")
            self.assertIn("[lca_smoke] public status: FAIL family=solver", result.stderr)
            self.assertIn("[lca_smoke] normalized outcome: reproducible_solver_failure", result.stderr)
            self.assertIn("[lca_smoke] normalized exit code: 124 raw_exit_code=124 source=inner_wrapper", result.stderr)
            self.assertIn(
                "[lca_smoke] failure partition: solver/test public_exit=124 raw_exit=124",
                result.stderr,
            )

    def test_launcher_recovers_from_invalid_tmp_parent_before_dispatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            self.write_text(
                branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp",
                "not a directory\n",
            )

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            status_summary = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
            self.assertTrue(status_summary.is_file(), msg="launcher recovery from an invalid tmp parent must still publish the stable status summary")
            status_summary_text = status_summary.read_text(encoding="utf-8")
            self.assertIn("public_status=PASS", status_summary_text)
            self.assertIn("result_family=none", status_summary_text)
            self.assertIn("normalized_exit_code=0", status_summary_text)
            self.assertIn("normalized_outcome=pass", status_summary_text)
            self.assertFalse(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / ".tmp").exists(),
                msg="launcher recovery must clean the repaired tmp parent after dispatch completes",
            )

    def test_launcher_recovers_from_invalid_run_history_root_before_archiving_status(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            run_history_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_run_history"
            self.write_text(run_history_root, "not a directory\n")

            result = subprocess.run(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                capture_output=True,
                text=True,
                check=False,
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr)
            self.assertTrue(run_history_root.is_dir(), msg="launcher must recreate the stable run-history root when stale poison state is present")
            self.assertTrue(
                any(run_history_root.glob("run.*")),
                msg="launcher must still archive the completed run after repairing the run-history root",
            )


if __name__ == "__main__":
    unittest.main()
