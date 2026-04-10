#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import tempfile
import textwrap
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
printf '#!/usr/bin/env bash\\nexit 124\\n' > "$FAILURE_ROOT/replay_active_manifest_case.sh"
chmod +x "$FAILURE_ROOT/replay_active_manifest_case.sh"
cat > "$FAILURE_ROOT/failure_summary.txt" <<EOF
failure_summary=controlled regression failure
failure_kind=solver_timeout
failure_origin=solver
failure_retryable=1
failed_case_tag=smoke_comb_core_n64_seed123
failed_stage=smoke
failed_mode=comb_core
failed_n=64
failed_seed=123
failure_root=$FAILURE_ROOT
failure_case_dir=$CASE_DIR
commands_path=$FAILURE_ROOT/commands.txt
artifact_manifest_path=$FAILURE_ROOT/artifact_manifest.tsv
rerun_command_path=$FAILURE_ROOT/rerun_command.txt
exact_seed_path=$FAILURE_ROOT/seed.txt
exact_input_path=$FAILURE_ROOT/input.txt
exact_output_path=$FAILURE_ROOT/solver_output.txt
expected_output_path=$FAILURE_ROOT/expected_output.txt
invoked_command_path=$FAILURE_ROOT/invoked_command.txt
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

    def run_smoke(self, branch_root: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["./lca_smoke.sh"],
            cwd=branch_root,
            capture_output=True,
            text=True,
            check=False,
            env=os.environ.copy(),
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
            self.assertEqual(retry_loop_control["normalized_outcome"], "reproducible_solver_failure")
            self.assertEqual(retry_loop_control["retry_loop_action"], "resume_progress40_retry_loop")
            self.assertTrue(retry_loop_control["should_resume_retry_loop"])
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


if __name__ == "__main__":
    unittest.main()
