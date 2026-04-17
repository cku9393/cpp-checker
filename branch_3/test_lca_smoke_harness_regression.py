#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import os
import re
import signal
import subprocess
import tempfile
import textwrap
import time
import unittest
from pathlib import Path


ENTRYPOINT_PATH = Path(__file__).resolve().parent / "lca_smoke.sh"
HOST_TMP_ROOT = Path("/tmp/cpp-checker-branch3-smoke-tests/harness")
HOST_TMP_ROOT.mkdir(parents=True, exist_ok=True)
# The harness tests build disposable fake branches; keep that host scaffolding
# on a stable OS temp root instead of any inherited retry-loop TMPDIR.
tempfile.tempdir = str(HOST_TMP_ROOT)


class LcaSmokeHarnessRegressionTests(unittest.TestCase):
    maxDiff = None

    def write_text(self, path: Path, content: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def make_executable(self, path: Path) -> None:
        path.chmod(path.stat().st_mode | 0o111)

    def symlink_file(self, target: Path, link_path: Path) -> None:
        link_path.parent.mkdir(parents=True, exist_ok=True)
        link_path.symlink_to(target)

    def make_fake_branch(self, temp_root: Path) -> Path:
        branch_root = temp_root / "branch"
        self.symlink_file(ENTRYPOINT_PATH, branch_root / "lca_smoke.sh")
        self.write_text(
            branch_root / "artifact_paths.py",
            textwrap.dedent(
                """\
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
    print(ARTIFACTS_ROOT)
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
                """\
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed123"
DISPATCH_COUNT_PATH="$ARTIFACTS_ROOT/dispatch_count.txt"
count=0

mkdir -p "$ARTIFACTS_ROOT"
if [[ -f "$DISPATCH_COUNT_PATH" ]]; then
  count="$(cat "$DISPATCH_COUNT_PATH")"
fi
count=$(( count + 1 ))
printf '%s\\n' "$count" > "$DISPATCH_COUNT_PATH"

rm -rf "$FAILURE_ROOT"
mkdir -p "$CASE_DIR"
printf 'commands\\n' > "$FAILURE_ROOT/commands.txt"
printf 'artifact\\tpath\\n' > "$FAILURE_ROOT/artifact_manifest.tsv"
printf './lca_smoke.sh\\n' > "$FAILURE_ROOT/rerun_command.txt"
printf '123\\n' > "$FAILURE_ROOT/seed.txt"
printf 'input\\n' > "$FAILURE_ROOT/input.txt"
printf 'output\\n' > "$FAILURE_ROOT/solver_output.txt"
printf 'expected\\n' > "$FAILURE_ROOT/expected_output.txt"
printf './solve < input.txt\\n' > "$FAILURE_ROOT/invoked_command.txt"
printf '{"schema":"lca_smoke_failure_context_v1"}\\n' > "$FAILURE_ROOT/failure_context.json"
printf 'VAR=1\\n' > "$FAILURE_ROOT/runtime_env.txt"
printf 'manifest\\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
printf 'case_count=1\\n' > "$FAILURE_ROOT/suite_config.txt"
printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed123\\n' > "$FAILURE_ROOT/suite_plan.tsv"
printf '#!/usr/bin/env bash\\nexit 124\\n' > "$FAILURE_ROOT/replay_active_manifest_case.sh"
chmod +x "$FAILURE_ROOT/replay_active_manifest_case.sh"
cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
failure_summary=controlled regression failure
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
printf '# controlled regression failure\\n' > "$FAILURE_ROOT/latest_failure_report.md"
exit 124
"""
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "build.sh",
            textwrap.dedent(
                """\
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
                """\
#!/usr/bin/env bash
set -euo pipefail
exit 0
"""
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "solver_release_env.sh",
            textwrap.dedent(
                """\
#!/usr/bin/env bash
export LOCAL_SKIP_SELF_TEST="${LOCAL_SKIP_SELF_TEST:-1}"
"""
            ).strip()
            + "\n",
        )
        for rel_path in (
            Path("branch_run_case.py"),
            Path("branch_validator.py"),
            Path("build.py"),
            Path("boj28350_resume.py"),
        ):
            self.write_text(branch_root / rel_path, "from __future__ import annotations\n")
        self.write_text(
            branch_root / "boj28350_resume" / "boj28350_branch_3_solver.cpp",
            "int main() { return 0; }\n",
        )
        self.write_text(
            branch_root / "boj28350_resume" / "smoke_cases.tsv",
            (
                "stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n"
                "smoke\tcomb_core\t64\t123\t1\t1\t2\n"
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

    def make_fail_then_pass_branch(self, temp_root: Path) -> Path:
        branch_root = self.make_fake_branch(temp_root)
        self.write_text(
            branch_root / "outer_suite_wrappers" / "lca_smoke.sh",
            textwrap.dedent(
                """\
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
SMOKE_ROOT="$ARTIFACTS_ROOT/smoke"
FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed123"
DISPATCH_COUNT_PATH="$ARTIFACTS_ROOT/dispatch_count.txt"
count=0

mkdir -p "$ARTIFACTS_ROOT"
if [[ -f "$DISPATCH_COUNT_PATH" ]]; then
  count="$(cat "$DISPATCH_COUNT_PATH")"
fi
count=$(( count + 1 ))
printf '%s\\n' "$count" > "$DISPATCH_COUNT_PATH"

if (( count == 1 )); then
  rm -rf "$SMOKE_ROOT"
  rm -rf "$FAILURE_ROOT"
  mkdir -p "$CASE_DIR"
  printf 'commands\\n' > "$FAILURE_ROOT/commands.txt"
  printf 'artifact\\tpath\\n' > "$FAILURE_ROOT/artifact_manifest.tsv"
  printf './lca_smoke.sh\\n' > "$FAILURE_ROOT/rerun_command.txt"
  printf '123\\n' > "$FAILURE_ROOT/seed.txt"
  printf 'input\\n' > "$FAILURE_ROOT/input.txt"
  printf 'output\\n' > "$FAILURE_ROOT/solver_output.txt"
  printf 'expected\\n' > "$FAILURE_ROOT/expected_output.txt"
  printf './solve < input.txt\\n' > "$FAILURE_ROOT/invoked_command.txt"
  printf '{"schema":"lca_smoke_failure_context_v1"}\\n' > "$FAILURE_ROOT/failure_context.json"
  printf 'VAR=1\\n' > "$FAILURE_ROOT/runtime_env.txt"
  printf 'manifest\\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
  printf 'case_count=1\\n' > "$FAILURE_ROOT/suite_config.txt"
  printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed123\\n' > "$FAILURE_ROOT/suite_plan.tsv"
  printf '#!/usr/bin/env bash\\nexit 124\\n' > "$FAILURE_ROOT/replay_active_manifest_case.sh"
  chmod +x "$FAILURE_ROOT/replay_active_manifest_case.sh"
  cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
failure_summary=controlled regression failure
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
  printf '# controlled regression failure\\n' > "$FAILURE_ROOT/latest_failure_report.md"
  exit 124
fi

rm -rf "$SMOKE_ROOT"
mkdir -p "$SMOKE_ROOT"
cat > "$SMOKE_ROOT/suite_config.txt" <<EOF
suite=smoke
mode=branch_local_retry
EOF
cat > "$SMOKE_ROOT/suite_plan.tsv" <<EOF
stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s
smoke\tcomb_core\t64\t123\t1\t1\t2
EOF
exit 0
"""
            ).strip()
            + "\n",
        )
        self.make_executable(branch_root / "outer_suite_wrappers" / "lca_smoke.sh")
        return branch_root

    def make_interrupt_then_pass_branch(self, temp_root: Path) -> Path:
        branch_root = self.make_fake_branch(temp_root)
        self.write_text(
            branch_root / "outer_suite_wrappers" / "lca_smoke.sh",
            textwrap.dedent(
                """\
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
SMOKE_ROOT="$ARTIFACTS_ROOT/smoke"
FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
DISPATCH_COUNT_PATH="$ARTIFACTS_ROOT/dispatch_count.txt"
ACTIVE_MARKER="$ARTIFACTS_ROOT/active_inner_wrapper.pid"
FIRST_START_MARKER="$ARTIFACTS_ROOT/first_dispatch_started"
TERM_LOG_PATH="$ARTIFACTS_ROOT/termination.log"
count=0

cleanup() {
  rm -f "$ACTIVE_MARKER"
}

handle_interrupt() {
  mkdir -p "$ARTIFACTS_ROOT"
  printf 'term\\n' >> "$TERM_LOG_PATH"
  cleanup
  exit 143
}

trap handle_interrupt TERM INT HUP

mkdir -p "$ARTIFACTS_ROOT"
if [[ -f "$DISPATCH_COUNT_PATH" ]]; then
  count="$(cat "$DISPATCH_COUNT_PATH")"
fi
count=$(( count + 1 ))
printf '%s\\n' "$count" > "$DISPATCH_COUNT_PATH"

if [[ -e "$ACTIVE_MARKER" ]]; then
  echo "stale active marker remained from a previous run: $ACTIVE_MARKER" >&2
  exit 70
fi
printf '%s\\n' "$$" > "$ACTIVE_MARKER"

if (( count == 1 )); then
  : > "$FIRST_START_MARKER"
  while :; do
    sleep 1
  done
fi

cleanup
rm -rf "$FAILURE_ROOT"
rm -rf "$SMOKE_ROOT"
mkdir -p "$SMOKE_ROOT"
cat > "$SMOKE_ROOT/suite_config.txt" <<EOF
suite=smoke
mode=interrupt_recovery
EOF
cat > "$SMOKE_ROOT/suite_plan.tsv" <<EOF
stage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s
smoke\tcomb_core\t64\t123\t1\t1\t2
EOF
exit 0
"""
            ).strip()
            + "\n",
        )
        self.make_executable(branch_root / "outer_suite_wrappers" / "lca_smoke.sh")
        return branch_root

    def run_smoke(
        self,
        branch_root: Path,
        *,
        env: dict[str, str] | None = None,
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["./lca_smoke.sh"],
            cwd=branch_root,
            capture_output=True,
            text=True,
            check=False,
            env=env or os.environ.copy(),
        )

    def read_summary(self, branch_root: Path) -> dict[str, str]:
        summary_path = branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "summary.txt"
        return dict(
            line.split("=", 1)
            for line in summary_path.read_text(encoding="utf-8").splitlines()
            if "=" in line
        )

    def status_projection(self, branch_root: Path) -> dict[str, str]:
        summary = self.read_summary(branch_root)
        keys = (
            "public_status",
            "result_family",
            "normalized_exit_code",
            "raw_exit_code",
            "normalized_outcome",
            "outcome_source",
            "outcome_summary",
            "source_failure_summary",
            "source_failure_case",
            "source_failure_seed",
            "source_failure_stage",
            "source_failure_kind",
            "source_failure_origin",
            "source_failure_retryable",
            "standard_gap_status",
            "standard_gap_summary",
        )
        return {key: summary[key] for key in keys}

    def report_projection(self, branch_root: Path) -> list[str]:
        report_path = (
            branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "latest_status_report.md"
        )
        interesting_prefixes = (
            "- Public status:",
            "- Result family:",
            "- Normalized outcome:",
            "- Normalized exit code:",
            "- Raw exit code:",
            "- Summary:",
            "- Failed stage scope:",
            "- Failed stage:",
            "- Stage label:",
            "- Status:",
            "- Explanation:",
        )
        interesting_headings = {
            "# lca_smoke Status Report",
            "## Failed Stage",
            "## Standard Gap",
        }
        lines = report_path.read_text(encoding="utf-8").splitlines()
        projection: list[str] = []
        summary_count = 0
        for line in lines:
            if not (
                line in interesting_headings
                or any(line.startswith(prefix) for prefix in interesting_prefixes)
            ):
                continue
            if line.startswith("- Summary:"):
                summary_count += 1
                if summary_count > 1:
                    continue
            projection.append(line)
        return projection

    def stable_public_status_lines(self, result: subprocess.CompletedProcess[str]) -> list[str]:
        stable_prefixes = (
            "[lca_smoke] public status:",
            "[lca_smoke] normalized outcome:",
            "[lca_smoke] normalized exit code:",
            "[lca_smoke] outcome summary:",
            "[lca_smoke] failed stage:",
            "[lca_smoke] stage label:",
            "[lca_smoke] primary report:",
            "[lca_smoke] inspect first:",
            "[lca_smoke] retry next:",
            "[lca_smoke] retry guidance:",
            "[lca_smoke] source root:",
            "[lca_smoke] source summary:",
            "[lca_smoke] source report:",
            "[lca_smoke] replay summary:",
            "[lca_smoke] replay case:",
            "[lca_smoke] replay command:",
        )
        return [
            line
            for line in result.stderr.splitlines()
            if line.startswith(stable_prefixes)
        ]

    def iteration_reporting_lines(self, result: subprocess.CompletedProcess[str]) -> list[str]:
        iteration_prefixes = (
            "[lca_smoke] iteration summary:",
            "[lca_smoke] diagnostics manifest:",
            "[lca_smoke] run history index:",
            "[lca_smoke] run record:",
            "[lca_smoke] run comparison:",
            "[lca_smoke] run archive root:",
            "[lca_smoke] launcher console transcript:",
            "[lca_smoke] dispatch result:",
        )
        return [
            line
            for line in result.stderr.splitlines()
            if line.startswith(iteration_prefixes)
        ]

    def read_json(self, path: Path) -> dict[str, object]:
        return json.loads(path.read_text(encoding="utf-8"))

    def read_tsv_rows(self, path: Path) -> list[dict[str, str]]:
        with path.open(encoding="utf-8", newline="") as handle:
            return list(csv.DictReader(handle, delimiter="\t"))

    def read_retry_loop_control(self, branch_root: Path, *, mirror: bool = False) -> dict[str, object]:
        control_path = (
            branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke" / "retry_loop_control.json"
            if mirror
            else branch_root / "artifacts" / "lca_tree_stress_v5" / "smoke_latest_status" / "retry_loop_control.json"
        )
        return json.loads(control_path.read_text(encoding="utf-8"))

    def test_smoke_launcher_reruns_controlled_failure_with_stable_public_reporting(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            resolved_branch_root = branch_root.resolve()

            first = self.run_smoke(branch_root)

            self.assertEqual(first.returncode, 124, msg=first.stderr)
            self.assertEqual(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "dispatch_count.txt").read_text(encoding="utf-8"),
                "1\n",
                msg="the fake inner smoke wrapper must be dispatched on the first run",
            )
            first_projection = self.status_projection(branch_root)
            self.assertEqual(
                first_projection,
                {
                    "public_status": "FAIL",
                    "result_family": "solver",
                    "normalized_exit_code": "124",
                    "raw_exit_code": "124",
                    "normalized_outcome": "reproducible_solver_failure",
                    "outcome_source": "inner_wrapper",
                    "outcome_summary": "inner smoke wrapper failed at stage smoke: controlled regression failure",
                    "source_failure_summary": "controlled regression failure",
                    "source_failure_case": "tag=smoke_comb_core_n64_seed123 stage=smoke mode=comb_core n=64 seed=123",
                    "source_failure_seed": "123",
                    "source_failure_stage": "smoke",
                    "source_failure_kind": "solver_timeout",
                    "source_failure_origin": "solver",
                    "source_failure_retryable": "1",
                    "standard_gap_status": "smoke_blocker_detected",
                    "standard_gap_summary": "inner smoke wrapper failed at stage smoke: controlled regression failure",
                },
            )
            first_report_projection = self.report_projection(branch_root)
            self.assertEqual(
                first_report_projection,
                [
                    "# lca_smoke Status Report",
                    "- Public status: `FAIL`",
                    "- Result family: `solver`",
                    "- Normalized outcome: `reproducible_solver_failure`",
                    "- Normalized exit code: `124`",
                    "- Raw exit code: `124`",
                    "- Summary: `inner smoke wrapper failed at stage smoke: controlled regression failure`",
                    "## Failed Stage",
                    "- Failed stage scope: `inner_wrapper_case`",
                    "- Failed stage: `smoke`",
                    "- Stage label: `inner_wrapper_case:smoke`",
                    "## Standard Gap",
                    "- Status: `smoke_blocker_detected`",
                    "- Explanation: `inner smoke wrapper failed at stage smoke: controlled regression failure`",
                ],
            )
            first_public_lines = self.stable_public_status_lines(first)
            for expected_line in (
                "[lca_smoke] public status: FAIL family=solver",
                "[lca_smoke] normalized outcome: reproducible_solver_failure",
                "[lca_smoke] normalized exit code: 124 raw_exit_code=124 source=inner_wrapper",
                "[lca_smoke] outcome summary: inner smoke wrapper failed at stage smoke: controlled regression failure",
                "[lca_smoke] failed stage: smoke scope=inner_wrapper_case",
                "[lca_smoke] stage label: inner_wrapper_case:smoke",
                "[lca_smoke] replay summary: controlled regression failure",
                "[lca_smoke] replay case: tag=smoke_comb_core_n64_seed123 stage=smoke mode=comb_core n=64 seed=123",
            ):
                self.assertIn(expected_line, first_public_lines)
            smoke_summary_mirror = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "smoke"
                / "summary.txt"
            ).read_text(encoding="utf-8")
            smoke_status_report_mirror = (
                branch_root
                / "artifacts"
                / "lca_tree_stress_v5"
                / "smoke"
                / "status_report.md"
            ).read_text(encoding="utf-8")
            self.assertIn("public_status=FAIL", smoke_summary_mirror)
            self.assertIn("normalized_outcome=reproducible_solver_failure", smoke_summary_mirror)
            self.assertIn("# lca_smoke Status Report", smoke_status_report_mirror)
            self.assertIn(
                "- Summary: `inner smoke wrapper failed at stage smoke: controlled regression failure`",
                smoke_status_report_mirror,
            )
            retry_loop_control = self.read_retry_loop_control(branch_root)
            retry_loop_control_mirror = self.read_retry_loop_control(branch_root, mirror=True)
            escaped_branch_root = str(resolved_branch_root).replace(" ", "\\ ")
            self.assertEqual(retry_loop_control_mirror, retry_loop_control)
            self.assertEqual(retry_loop_control["public_status"], "FAIL")
            self.assertEqual(retry_loop_control["acceptance_signal_status"], "FAIL")
            self.assertEqual(retry_loop_control["iteration_support_status"], "ACTIONABLE")
            self.assertEqual(retry_loop_control["iteration_support_next_step"], "retry")
            self.assertEqual(retry_loop_control["normalized_outcome"], "reproducible_solver_failure")
            self.assertEqual(retry_loop_control["retry_loop_action"], "resume_progress40_retry_loop")
            self.assertTrue(retry_loop_control["should_resume_retry_loop"])
            self.assertFalse(retry_loop_control["should_retry_smoke_directly"])
            self.assertFalse(retry_loop_control["failure_is_terminal"])
            self.assertFalse(retry_loop_control["gate_escalation_allowed"])
            self.assertEqual(retry_loop_control["next_gate_status"], "blocked_by_ac2")
            self.assertEqual(retry_loop_control["next_gate_dependency"], "AC2")
            self.assertIn("strong gate is intentionally blocked", retry_loop_control["next_gate_summary"])
            self.assertEqual(retry_loop_control["command_control"]["mode"], "acceptance_failure_retry")
            self.assertEqual(retry_loop_control["command_control"]["preferred_command_kind"], "retry_loop")
            self.assertTrue(retry_loop_control["command_control"]["should_resume_retry_loop"])
            self.assertFalse(retry_loop_control["command_control"]["should_retry_smoke_directly"])
            self.assertFalse(retry_loop_control["command_control"]["failure_is_terminal"])
            self.assertFalse(retry_loop_control["command_control"]["gate_escalation_allowed"])
            self.assertEqual(retry_loop_control["command_control"]["next_gate"]["status"], "blocked_by_ac2")
            self.assertEqual(
                retry_loop_control["preferred_command"],
                f'cd {escaped_branch_root} && zsh .ouroboros/launch_retry_loop.sh smoke_latest_status_retry_loop.log .ouroboros/seed_branch3_progress40_research_loop.yaml .ouroboros/seed_branch3_failure_analysis.yaml',
            )
            self.assertEqual(
                retry_loop_control["launch_command"],
                f'cd {escaped_branch_root} && zsh .ouroboros/launch_retry_loop.sh smoke_latest_status_retry_loop.log .ouroboros/seed_branch3_progress40_research_loop.yaml .ouroboros/seed_branch3_failure_analysis.yaml',
            )
            self.assertEqual(
                retry_loop_control["direct_command"],
                f'cd {escaped_branch_root} && zsh .ouroboros/run_until_pass_progress40.sh .ouroboros/seed_branch3_progress40_research_loop.yaml .ouroboros/seed_branch3_failure_analysis.yaml',
            )
            self.assertEqual(retry_loop_control["next_gate_command"], "./lca_strong_gate.sh")
            self.assertEqual(
                retry_loop_control["log_path"],
                str(
                    resolved_branch_root
                    / "artifacts"
                    / "lca_tree_stress_v5"
                    / "retry_loop"
                    / "smoke_latest_status_retry_loop.log"
                ),
            )
            self.assertEqual(
                retry_loop_control["artifacts"]["published_control_path"],
                str(
                    resolved_branch_root
                    / "artifacts"
                    / "lca_tree_stress_v5"
                    / "smoke"
                    / "retry_loop_control.json"
                ),
            )

            second = self.run_smoke(branch_root)

            self.assertEqual(second.returncode, 124, msg=second.stderr)
            self.assertEqual(
                (branch_root / "artifacts" / "lca_tree_stress_v5" / "dispatch_count.txt").read_text(encoding="utf-8"),
                "2\n",
                msg="the fake inner smoke wrapper must still dispatch cleanly on a rerun without manual cleanup",
            )
            self.assertEqual(self.status_projection(branch_root), first_projection)
            self.assertEqual(self.report_projection(branch_root), first_report_projection)
            self.assertEqual(self.stable_public_status_lines(second), first_public_lines)
            self.assertIn(
                "public_status=FAIL",
                (
                    branch_root
                    / "artifacts"
                    / "lca_tree_stress_v5"
                    / "smoke"
                    / "summary.txt"
                ).read_text(encoding="utf-8"),
            )
            self.assertIn(
                "# lca_smoke Status Report",
                (
                    branch_root
                    / "artifacts"
                    / "lca_tree_stress_v5"
                    / "smoke"
                    / "status_report.md"
                ).read_text(encoding="utf-8"),
            )

    def test_smoke_launcher_can_be_invoked_again_after_a_failed_run_without_manual_cleanup(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fail_then_pass_branch(Path(tmp))
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            failure_root = artifacts_root / "smoke_latest_failure"

            first = self.run_smoke(branch_root)
            self.assertEqual(first.returncode, 124, msg=first.stderr)
            self.assertTrue(
                failure_root.is_dir(),
                msg="the first failing smoke run must publish its preserved failure bundle",
            )

            second = self.run_smoke(branch_root)

            self.assertEqual(second.returncode, 0, msg=second.stderr)
            self.assertEqual(
                (artifacts_root / "dispatch_count.txt").read_text(encoding="utf-8"),
                "2\n",
                msg="the public smoke launcher must dispatch a clean second invocation without manual reset",
            )
            self.assertFalse(
                failure_root.exists(),
                msg="a successful rerun must clear the stale smoke failure bundle without manual cleanup",
            )

            summary = self.read_summary(branch_root)
            self.assertEqual(summary["public_status"], "PASS")
            self.assertEqual(summary["result_family"], "none")
            self.assertEqual(summary["normalized_exit_code"], "0")
            self.assertEqual(summary["raw_exit_code"], "0")
            self.assertEqual(summary["normalized_outcome"], "pass")
            self.assertEqual(summary["outcome_source"], "inner_wrapper")
            self.assertEqual(summary["standard_gap_status"], "ready_for_gate_escalation")

            smoke_root = artifacts_root / "smoke"
            self.assertTrue(
                (smoke_root / "suite_config.txt").is_file(),
                msg="the second invocation must publish a fresh smoke bundle without any manual artifact cleanup",
            )
            self.assertIn(
                "public_status=PASS",
                (smoke_root / "summary.txt").read_text(encoding="utf-8"),
            )
            self.assertIn(
                "# lca_smoke Status Report",
                (smoke_root / "status_report.md").read_text(encoding="utf-8"),
            )

    def test_smoke_launcher_recovers_cleanly_after_an_interrupted_dispatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_interrupt_then_pass_branch(Path(tmp))
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            first_start_marker = artifacts_root / "first_dispatch_started"
            active_marker = artifacts_root / "active_inner_wrapper.pid"
            termination_log = artifacts_root / "termination.log"
            dispatch_state_path = artifacts_root / ".locks" / "lca_smoke.dispatch.state"

            first = subprocess.Popen(
                ["./lca_smoke.sh"],
                cwd=branch_root,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            for _ in range(50):
                if first_start_marker.exists():
                    break
                time.sleep(0.1)
            self.assertTrue(
                first_start_marker.exists(),
                msg="the fake inner wrapper never entered its first long-running dispatch",
            )

            first.send_signal(signal.SIGTERM)
            first_stdout, first_stderr = first.communicate(timeout=20)
            self.assertNotEqual(first.returncode, 0, msg=first_stdout + first_stderr)

            second = self.run_smoke(branch_root)

            self.assertEqual(second.returncode, 0, msg=second.stderr)
            self.assertEqual(
                (artifacts_root / "dispatch_count.txt").read_text(encoding="utf-8"),
                "2\n",
                msg="the second invocation must run the inner smoke wrapper again after the interrupted dispatch",
            )
            self.assertFalse(
                active_marker.exists(),
                msg="no stale active inner-wrapper marker should survive the interrupted-run cleanup path",
            )
            self.assertFalse(
                dispatch_state_path.exists(),
                msg="the tracked launcher dispatch state must be removed once the rerun finishes cleanly",
            )
            self.assertTrue(
                termination_log.is_file(),
                msg="the interrupted first dispatch must record that the owned inner wrapper was terminated",
            )
            self.assertEqual(self.read_summary(branch_root)["public_status"], "PASS")

    def test_smoke_launcher_archives_iteration_reporting_artifacts_for_retry_guidance(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            history_root = artifacts_root / "smoke_run_history"
            history_path = history_root / "history.tsv"

            first = self.run_smoke(branch_root)
            second = self.run_smoke(branch_root)

            self.assertEqual(first.returncode, 124, msg=first.stderr)
            self.assertEqual(second.returncode, 124, msg=second.stderr)

            first_archive = history_root / "run.000001"
            second_archive = history_root / "run.000002"
            first_record = self.read_json(first_archive / "run_record.json")
            second_record = self.read_json(second_archive / "run_record.json")
            first_comparison = self.read_json(first_archive / "run_comparison.json")
            second_comparison = self.read_json(second_archive / "run_comparison.json")
            history_rows = self.read_tsv_rows(history_path)
            diagnostics_rows = {
                row["artifact"]: row
                for row in self.read_tsv_rows(artifacts_root / "smoke_latest_status" / "diagnostics_manifest.tsv")
            }
            archive_manifest_rows = {
                row["artifact"]: row for row in self.read_tsv_rows(second_archive / "artifact_manifest.tsv")
            }

            self.assertEqual([row["run_id"] for row in history_rows], ["run.000001", "run.000002"])
            self.assertEqual(first_record["run"]["id"], "run.000001")
            self.assertEqual(second_record["run"]["id"], "run.000002")
            self.assertEqual(
                second_record["comparison"]["summary"],
                "same normalized outcome as previous run run.000001 (reproducible_solver_failure at inner_wrapper_case:smoke)",
            )
            self.assertFalse(first_comparison["has_previous_run"])
            self.assertTrue(second_comparison["has_previous_run"])
            self.assertEqual(second_comparison["previous_run"]["run_id"], "run.000001")

            resolved_artifacts_root = artifacts_root.resolve()
            resolved_history_root = history_root.resolve()
            expected_dispatch_result = (resolved_history_root / "run.000002" / "dispatch_result.txt").resolve()
            expected_console_log = (resolved_history_root / "run.000002" / "console.stderr.txt").resolve()
            self.assertEqual(
                self.read_summary(branch_root)["run_dispatch_result_path"],
                str(expected_dispatch_result),
            )
            self.assertEqual(
                second_record["artifacts"]["run_dispatch_result_path"],
                str(expected_dispatch_result),
            )
            self.assertTrue(expected_dispatch_result.is_file())
            self.assertTrue(expected_console_log.is_file())

            self.assertEqual(diagnostics_rows["run_dispatch_result"]["path"], str(expected_dispatch_result))
            self.assertEqual(diagnostics_rows["run_dispatch_result"]["exists"], "1")
            self.assertEqual(diagnostics_rows["run_console_stderr"]["path"], str(expected_console_log))
            self.assertEqual(diagnostics_rows["status_run_record"]["exists"], "1")
            self.assertEqual(diagnostics_rows["status_run_comparison"]["exists"], "1")
            self.assertEqual(archive_manifest_rows["dispatch_result"]["path"], str(expected_dispatch_result))
            self.assertEqual(
                archive_manifest_rows["dispatch_result"]["provenance"],
                "launcher_dispatch_result_snapshot",
            )

            status_report = (
                artifacts_root / "smoke_latest_status" / "latest_status_report.md"
            ).read_text(encoding="utf-8")
            self.assertIn(
                f"- Dispatch result snapshot: `{expected_dispatch_result}`",
                status_report,
            )

            second_iteration_lines = self.iteration_reporting_lines(second)
            self.assertTrue(
                any(
                    re.fullmatch(
                        r"\[lca_smoke\] iteration summary: run_id=run\.000002 elapsed_seconds=\d+ "
                        r"comparison=same normalized outcome as previous run run\.000001 "
                        r"\(reproducible_solver_failure at inner_wrapper_case:smoke\)",
                        line,
                    )
                    for line in second_iteration_lines
                ),
                msg=second.stderr,
            )
            for expected_line in (
                f"[lca_smoke] diagnostics manifest: {resolved_artifacts_root / 'smoke_latest_status' / 'diagnostics_manifest.tsv'}",
                f"[lca_smoke] run history index: {(resolved_history_root / 'history.tsv')}",
                f"[lca_smoke] run record: {resolved_artifacts_root / 'smoke_latest_status' / 'run_record.json'}",
                f"[lca_smoke] run comparison: {resolved_artifacts_root / 'smoke_latest_status' / 'run_comparison.json'}",
                f"[lca_smoke] run archive root: {(resolved_history_root / 'run.000002')}",
                f"[lca_smoke] launcher console transcript: {expected_console_log}",
                f"[lca_smoke] dispatch result: {expected_dispatch_result}",
            ):
                self.assertIn(expected_line, second_iteration_lines)

    def test_smoke_launcher_iteration_summary_reports_nonzero_elapsed_seconds_for_slow_failures(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            self.write_text(
                branch_root / "outer_suite_wrappers" / "lca_smoke.sh",
                textwrap.dedent(
                    """\
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed123"

mkdir -p "$CASE_DIR"
printf 'commands\\n' > "$FAILURE_ROOT/commands.txt"
printf 'artifact\\tpath\\n' > "$FAILURE_ROOT/artifact_manifest.tsv"
printf './lca_smoke.sh\\n' > "$FAILURE_ROOT/rerun_command.txt"
printf '123\\n' > "$FAILURE_ROOT/seed.txt"
printf 'input\\n' > "$FAILURE_ROOT/input.txt"
printf 'output\\n' > "$FAILURE_ROOT/solver_output.txt"
printf 'expected\\n' > "$FAILURE_ROOT/expected_output.txt"
printf './solve < input.txt\\n' > "$FAILURE_ROOT/invoked_command.txt"
printf '{"schema":"lca_smoke_failure_context_v1"}\\n' > "$FAILURE_ROOT/failure_context.json"
printf 'VAR=1\\n' > "$FAILURE_ROOT/runtime_env.txt"
printf 'manifest\\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
printf 'case_count=1\\n' > "$FAILURE_ROOT/suite_config.txt"
printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed123\\n' > "$FAILURE_ROOT/suite_plan.tsv"
printf '#!/usr/bin/env bash\\nexit 124\\n' > "$FAILURE_ROOT/replay_active_manifest_case.sh"
chmod +x "$FAILURE_ROOT/replay_active_manifest_case.sh"
cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
failure_summary=controlled slow regression failure
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
printf '# controlled slow regression failure\\n' > "$FAILURE_ROOT/latest_failure_report.md"
sleep 1
exit 124
"""
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "outer_suite_wrappers" / "lca_smoke.sh")

            result = self.run_smoke(branch_root)

            self.assertEqual(result.returncode, 124, msg=result.stderr)
            elapsed_seconds = int(self.read_summary(branch_root)["run_elapsed_seconds"])
            self.assertGreaterEqual(elapsed_seconds, 1)
            self.assertTrue(
                any(
                    re.fullmatch(
                        r"\[lca_smoke\] iteration summary: run_id=run\.000001 elapsed_seconds=[1-9]\d* "
                        r"comparison=first recorded smoke run under .+; no previous iteration is available for comparison",
                        line,
                    )
                    for line in self.iteration_reporting_lines(result)
                ),
                msg=result.stderr,
            )

    def test_launcher_dispatch_timeout_stays_harness_failure_even_with_solver_like_failure_bundle(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            self.write_text(
                branch_root / "outer_suite_wrappers" / "lca_smoke.sh",
                textwrap.dedent(
                    """\
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
CASE_DIR="$FAILURE_ROOT/smoke_comb_core_n64_seed123"

rm -rf "$FAILURE_ROOT"
mkdir -p "$CASE_DIR"
printf 'commands\\n' > "$FAILURE_ROOT/commands.txt"
printf 'artifact\\tpath\\n' > "$FAILURE_ROOT/artifact_manifest.tsv"
printf './lca_smoke.sh\\n' > "$FAILURE_ROOT/rerun_command.txt"
printf '123\\n' > "$FAILURE_ROOT/seed.txt"
printf 'input\\n' > "$FAILURE_ROOT/input.txt"
printf 'output\\n' > "$FAILURE_ROOT/solver_output.txt"
printf 'expected\\n' > "$FAILURE_ROOT/expected_output.txt"
printf './solve < input.txt\\n' > "$FAILURE_ROOT/invoked_command.txt"
printf '{"schema":"lca_smoke_failure_context_v1"}\\n' > "$FAILURE_ROOT/failure_context.json"
printf 'VAR=1\\n' > "$FAILURE_ROOT/runtime_env.txt"
printf 'manifest\\n' > "$FAILURE_ROOT/smoke_cases_manifest.tsv"
printf 'case_count=1\\n' > "$FAILURE_ROOT/suite_config.txt"
printf 'case_index\\tcase_tag\\n1\\tsmoke_comb_core_n64_seed123\\n' > "$FAILURE_ROOT/suite_plan.tsv"
printf '#!/usr/bin/env bash\\nexit 124\\n' > "$FAILURE_ROOT/replay_active_manifest_case.sh"
chmod +x "$FAILURE_ROOT/replay_active_manifest_case.sh"
cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
failure_summary=controlled regression failure
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
printf '# controlled regression failure\\n' > "$FAILURE_ROOT/latest_failure_report.md"
sleep 10
"""
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "outer_suite_wrappers" / "lca_smoke.sh")

            env = os.environ.copy()
            env["LCA_SMOKE_LAUNCHER_TIMEOUT_S"] = "0.2"
            result = self.run_smoke(branch_root, env=env)

            self.assertEqual(result.returncode, 70, msg=result.stderr)
            projection = self.status_projection(branch_root)
            self.assertEqual(
                projection,
                {
                    "public_status": "FAIL",
                    "result_family": "harness",
                    "normalized_exit_code": "70",
                    "raw_exit_code": "124",
                    "normalized_outcome": "harness_infrastructure_failure",
                    "outcome_source": "launcher",
                    "outcome_summary": (
                        "launcher-enforced dispatch timeout after 0.2s while waiting for inner smoke wrapper; "
                        "preserved inner-wrapper failure detail is advisory only: controlled regression failure"
                    ),
                    "source_failure_summary": "controlled regression failure",
                    "source_failure_case": "tag=smoke_comb_core_n64_seed123 stage=smoke mode=comb_core n=64 seed=123",
                    "source_failure_seed": "123",
                    "source_failure_stage": "smoke",
                    "source_failure_kind": "solver_timeout",
                    "source_failure_origin": "solver",
                    "source_failure_retryable": "1",
                    "standard_gap_status": "smoke_blocker_detected",
                    "standard_gap_summary": (
                        "launcher-enforced dispatch timeout after 0.2s while waiting for inner smoke wrapper; "
                        "preserved inner-wrapper failure detail is advisory only: controlled regression failure"
                    ),
                },
            )
            self.assertEqual(
                self.report_projection(branch_root),
                [
                    "# lca_smoke Status Report",
                    "- Public status: `FAIL`",
                    "- Result family: `harness`",
                    "- Normalized outcome: `harness_infrastructure_failure`",
                    "- Normalized exit code: `70`",
                    "- Raw exit code: `124`",
                    "- Summary: `launcher-enforced dispatch timeout after 0.2s while waiting for inner smoke wrapper; preserved inner-wrapper failure detail is advisory only: controlled regression failure`",
                    "## Failed Stage",
                    "- Failed stage scope: `launcher_dispatch`",
                    "- Failed stage: `dispatch`",
                    "- Stage label: `launcher_dispatch:dispatch`",
                    "## Standard Gap",
                    "- Status: `smoke_blocker_detected`",
                    "- Explanation: `launcher-enforced dispatch timeout after 0.2s while waiting for inner smoke wrapper; preserved inner-wrapper failure detail is advisory only: controlled regression failure`",
                ],
            )
            public_lines = self.stable_public_status_lines(result)
            for expected_line in (
                "[lca_smoke] public status: FAIL family=harness",
                "[lca_smoke] normalized outcome: harness_infrastructure_failure",
                "[lca_smoke] normalized exit code: 70 raw_exit_code=124 source=launcher",
                "[lca_smoke] outcome summary: launcher-enforced dispatch timeout after 0.2s while waiting for inner smoke wrapper; preserved inner-wrapper failure detail is advisory only: controlled regression failure",
                "[lca_smoke] failed stage: dispatch scope=launcher_dispatch",
                "[lca_smoke] stage label: launcher_dispatch:dispatch",
                "[lca_smoke] replay summary: controlled regression failure",
                "[lca_smoke] replay case: tag=smoke_comb_core_n64_seed123 stage=smoke mode=comb_core n=64 seed=123",
            ):
                self.assertIn(expected_line, public_lines)
            retry_loop_control = self.read_retry_loop_control(branch_root)
            self.assertEqual(retry_loop_control["retry_loop_action"], "repair_and_rerun_smoke")
            self.assertEqual(retry_loop_control["preferred_command"], "./lca_smoke.sh")
            self.assertFalse(retry_loop_control["should_resume_retry_loop"])
            self.assertTrue(retry_loop_control["should_retry_smoke_directly"])
            self.assertFalse(retry_loop_control["failure_is_terminal"])
            self.assertFalse(retry_loop_control["gate_escalation_allowed"])
            self.assertEqual(retry_loop_control["next_gate_status"], "blocked_by_ac2")
            self.assertEqual(retry_loop_control["command_control"]["mode"], "smoke_repair_retry")
            self.assertEqual(retry_loop_control["command_control"]["preferred_command_kind"], "smoke_rerun")
            self.assertFalse(retry_loop_control["command_control"]["should_resume_retry_loop"])
            self.assertTrue(retry_loop_control["command_control"]["should_retry_smoke_directly"])
            self.assertFalse(retry_loop_control["command_control"]["gate_escalation_allowed"])
            self.assertEqual(retry_loop_control["command_control"]["next_gate"]["status"], "blocked_by_ac2")


if __name__ == "__main__":
    unittest.main()
