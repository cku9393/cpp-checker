#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
import tempfile
import textwrap
import time
import unittest
from pathlib import Path


WRAPPER_PATH = Path(__file__).resolve().parent / "outer_suite_wrappers" / "lca_required_repeatability.sh"
WRAPPER_SOURCE = WRAPPER_PATH.read_text(encoding="utf-8")


class LcaRequiredRepeatabilityWrapperRegressionTests(unittest.TestCase):
    def write_text(self, path: Path, content: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def make_executable(self, path: Path) -> None:
        path.chmod(path.stat().st_mode | 0o111)

    def make_fake_branch(self, temp_root: Path) -> Path:
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
                    "lca_strong_gate": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "strong_gate",
                    "lca_boj3s_gate": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "boj3s_gate",
                    "lca_required_repeatability": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "required_repeatability",
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
        self.write_text(branch_root / "outer_suite_wrappers" / "lca_required_repeatability.sh", WRAPPER_SOURCE)
        self.write_text(
            branch_root / "lca_strong_gate.sh",
            textwrap.dedent(
                """
                #!/usr/bin/env bash
                set -euo pipefail

                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                OUTROOT="$(python3 "$SCRIPT_DIR/artifact_paths.py" lca_strong_gate)"
                rm -rf "$OUTROOT"
                mkdir -p "$OUTROOT"
                printf 'stub runtime env\\n' > "$OUTROOT/runtime_env.txt"
                printf 'kind\\tlabel\\tstatus\\tvalue\\npath\\tselected_preset\\tok\\tstub\\n' > "$OUTROOT/preflight_manifest.tsv"
                printf '{"name":"stub_strong_gate"}\\n' > "$OUTROOT/selected_preset.json"
                cat >"$OUTROOT/repeatability_gate_manifest.txt" <<EOF
                repeatability_run_token=${LCA_REPEATABILITY_RUN_TOKEN:-}
                repeatability_cycle=${LCA_REPEATABILITY_CYCLE:-}
                repeatability_gate_label=${LCA_REPEATABILITY_GATE_LABEL:-}
                EOF
                cat >"$OUTROOT/certify.json" <<'EOF'
                {
                  "verdict": "PASS",
                  "preset": "stub_strong_gate",
                  "reasons": [],
                  "stages": [
                    {
                      "name": "required",
                      "status": "PASS",
                      "cases": 2,
                      "timeouts": 0,
                      "re_wa": 0,
                      "limit_scale": 1.0,
                      "scale_fail": []
                    }
                  ]
                }
                EOF
                printf 'stub strong gate\\n' > "$OUTROOT/certify_summary.md"
                """
            ).strip()
            + "\n",
        )
        self.write_text(
            branch_root / "lca_boj3s_gate.sh",
            textwrap.dedent(
                """
                #!/usr/bin/env bash
                set -euo pipefail

                SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                OUTROOT="$(python3 "$SCRIPT_DIR/artifact_paths.py" lca_boj3s_gate)"
                rm -rf "$OUTROOT"
                mkdir -p "$OUTROOT"
                printf 'stub runtime env\\n' > "$OUTROOT/runtime_env.txt"
                printf 'kind\\tlabel\\tstatus\\tvalue\\npath\\tselected_preset\\tok\\tstub\\n' > "$OUTROOT/preflight_manifest.tsv"
                printf '{"name":"stub_boj3s_gate"}\\n' > "$OUTROOT/selected_preset.json"
                cat >"$OUTROOT/repeatability_gate_manifest.txt" <<EOF
                repeatability_run_token=${LCA_REPEATABILITY_RUN_TOKEN:-}
                repeatability_cycle=${LCA_REPEATABILITY_CYCLE:-}
                repeatability_gate_label=${LCA_REPEATABILITY_GATE_LABEL:-}
                EOF
                cat >"$OUTROOT/certify.json" <<'EOF'
                {
                  "verdict": "PASS",
                  "preset": "stub_boj3s_gate",
                  "reasons": [],
                  "stages": [
                    {
                      "name": "final",
                      "status": "PASS",
                      "cases": 3,
                      "timeouts": 0,
                      "re_wa": 0,
                      "limit_scale": 1.0,
                      "scale_fail": []
                    }
                  ]
                }
                EOF
                printf 'stub boj3s gate\\n' > "$OUTROOT/certify_summary.md"
                """
            ).strip()
            + "\n",
        )

        for rel_path in (
            Path("solver_release_env.sh"),
            Path("outer_suite_wrappers/lca_required_repeatability.sh"),
            Path("lca_strong_gate.sh"),
            Path("lca_boj3s_gate.sh"),
        ):
            self.make_executable(branch_root / rel_path)
        return branch_root

    def test_uses_fixed_branch_local_artifact_roots_and_staging(self) -> None:
        self.assertIn(
            'ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"',
            WRAPPER_SOURCE,
            msg="required repeatability must stay under the branch-local lca_tree_stress_v5 artifacts root",
        )
        self.assertIn(
            'STAGE_PARENT="$ARTIFACTS_ROOT/.repeatability_stage"',
            WRAPPER_SOURCE,
            msg="required repeatability must stage runs under the branch-local repeatability staging root",
        )
        self.assertIn(
            'OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_required_repeatability)"',
            WRAPPER_SOURCE,
            msg="required repeatability must publish to the fixed required_repeatability artifact root",
        )
        self.assertIn(
            'WORKDIR="$(mktemp -d "$STAGE_PARENT/$RUN_WORK_TEMPLATE")"',
            WRAPPER_SOURCE,
            msg="required repeatability must create a fresh staging workdir for each invocation",
        )

    def test_reruns_required_gate_sequence_in_each_cycle(self) -> None:
        self.assertIn(
            'REQUIRED_SEQUENCE="lca_strong_gate -> lca_boj3s_gate"',
            WRAPPER_SOURCE,
            msg="required repeatability summary must record the enforced gate order",
        )
        self.assertIn(
            'if ! run_gate_once "strong_gate" "lca_strong_gate" "$STRONG_WRAPPER" "$STRONG_OUTROOT" "$run_dir"; then',
            WRAPPER_SOURCE,
            msg="each repeatability cycle must run the strong gate first",
        )
        self.assertIn(
            'if ! run_gate_once "boj3s_gate" "lca_boj3s_gate" "$BOJ3S_WRAPPER" "$BOJ3S_OUTROOT" "$run_dir"; then',
            WRAPPER_SOURCE,
            msg="each repeatability cycle must run the BOJ 3s gate after the strong gate",
        )
        self.assertIn(
            "COMPLETED_RUNS=\"$run_index\"",
            WRAPPER_SOURCE,
            msg="repeatability must track how many full required-gate cycles completed",
        )

    def test_extracts_stable_pass_signatures_and_compares_against_baseline(self) -> None:
        self.assertIn(
            'extract_pass_signature \\',
            WRAPPER_SOURCE,
            msg="repeatability must extract a stable pass signature from each gate certify.json",
        )
        self.assertIn(
            '"$run_dir/$gate_label/pass_signature.json"',
            WRAPPER_SOURCE,
            msg="repeatability must persist per-run pass signatures for later comparison",
        )
        self.assertIn(
            "compare_gate_signatures",
            WRAPPER_SOURCE,
            msg="repeatability must compare later PASS signatures against the baseline cycle",
        )
        self.assertIn(
            'signature_status="matched_baseline"',
            WRAPPER_SOURCE,
            msg="repeatability must distinguish matched reruns from the baseline cycle",
        )
        self.assertIn(
            "BASELINE_RUN=\"$run_dir\"",
            WRAPPER_SOURCE,
            msg="repeatability must pin the first successful cycle as the baseline for later comparisons",
        )
        self.assertIn(
            "compared_signature_fields=$SIGNATURE_FIELDS",
            WRAPPER_SOURCE,
            msg="repeatability summaries must record which certify fields define a stable PASS",
        )
        self.assertIn(
            'verify_gate_output_freshness "$gate_human" "$freshness_marker" "$gate_outroot" "$freshness_report"',
            WRAPPER_SOURCE,
            msg="repeatability must reject a gate exit 0 when PASS artifacts were not regenerated in the current run",
        )
        self.assertIn(
            'FAILURE_REASON="${gate_human} returned exit code 0 but reused prior gate artifacts instead of regenerating current-run pass evidence"',
            WRAPPER_SOURCE,
            msg="repeatability must surface stale PASS artifact reuse as a dedicated failure mode",
        )
        self.assertIn(
            '("runtime_env", runtime_env_path)',
            WRAPPER_SOURCE,
            msg="repeatability freshness checks must require a current-run runtime_env.txt alongside the verdict files",
        )
        self.assertIn(
            '("preflight_manifest", preflight_manifest_path)',
            WRAPPER_SOURCE,
            msg="repeatability freshness checks must require a current-run preflight_manifest.tsv alongside the verdict files",
        )
        self.assertIn(
            '("selected_preset", selected_preset_path)',
            WRAPPER_SOURCE,
            msg="repeatability freshness checks must require a current-run selected_preset.json alongside the verdict files",
        )
        self.assertIn(
            '("repeatability_manifest", repeatability_manifest_path)',
            WRAPPER_SOURCE,
            msg="repeatability freshness checks must require a current-run repeatability_gate_manifest.txt alongside the verdict files",
        )
        self.assertIn(
            "selected_preset_sha256",
            WRAPPER_SOURCE,
            msg="repeatability signatures must include the selected preset snapshot digest so preset drift cannot hide behind a stable preset name",
        )
        self.assertIn(
            "runtime_env_config",
            WRAPPER_SOURCE,
            msg="repeatability signatures must capture the stable runtime-config subset alongside the certify verdict",
        )

    def test_rejects_dataless_gate_launchers_during_preflight(self) -> None:
        self.assertIn(
            "path_has_dataless_flag",
            WRAPPER_SOURCE,
            msg="repeatability must detect dataless gate launchers before starting a cycle",
        )
        self.assertIn(
            'require_materialized_executable "$STRONG_WRAPPER" "strong gate wrapper"',
            WRAPPER_SOURCE,
            msg="repeatability must require a materialized strong gate launcher",
        )
        self.assertIn(
            'require_materialized_executable "$BOJ3S_WRAPPER" "BOJ 3s gate wrapper"',
            WRAPPER_SOURCE,
            msg="repeatability must require a materialized BOJ 3s launcher",
        )
        self.assertIn(
            'fail "dataless executable ${label}: $path"',
            WRAPPER_SOURCE,
            msg="repeatability must fail fast instead of hanging on iCloud placeholders",
        )

    def test_republishes_output_without_manual_cleanup_between_invocations(self) -> None:
        self.assertIn(
            'clear_stale_state',
            WRAPPER_SOURCE,
            msg="repeatability must clear abandoned staging state before a new invocation",
        )
        self.assertIn(
            'for stale in "$STAGE_PARENT"/lca_required_repeatability.*; do',
            WRAPPER_SOURCE,
            msg="repeatability stale-state cleanup must remove abandoned required_repeatability staging dirs",
        )
        self.assertIn(
            'BACKUP_ROOT="${OUTROOT}.previous"',
            WRAPPER_SOURCE,
            msg="repeatability must rotate the previous published output instead of requiring manual deletion",
        )
        self.assertIn(
            'if [[ -e "$OUTROOT" ]]; then',
            WRAPPER_SOURCE,
            msg="repeatability publish must tolerate an existing output root from a prior invocation",
        )
        self.assertIn(
            'if ! move_path_retry "$OUTROOT" "$BACKUP_ROOT"; then',
            WRAPPER_SOURCE,
            msg="repeatability publish must rotate the prior output before publishing the new one",
        )
        self.assertIn(
            'if ! move_path_retry "$WORKDIR" "$OUTPARENT/$outleaf"; then',
            WRAPPER_SOURCE,
            msg="repeatability publish must atomically promote the new staged output into the fixed artifact root",
        )
        self.assertIn(
            'ROOT_GUARD_DIR="$ARTIFACTS_ROOT/required_repeatability.root_guard"',
            WRAPPER_SOURCE,
            msg="required repeatability must create a stable root guard under the shared lca_tree_stress_v5 artifact tree",
        )
        self.assertIn(
            'if ! assert_root_guard_intact "$gate_human" "$run_dir"; then',
            WRAPPER_SOURCE,
            msg="required repeatability must reject gate reruns that clear the shared artifact root mid-cycle",
        )

    def test_results_and_summary_capture_per_run_gate_verdicts(self) -> None:
        self.assertIn(
            "printf 'run\\tgate\\tverdict\\tsignature_status\\n' > \"$WORKDIR/results.tsv\"",
            WRAPPER_SOURCE,
            msg="repeatability must record a tabular run/gate verdict log",
        )
        self.assertIn(
            'record_result_row "${run_dir##*/}" "$gate_human" "PASS" "$signature_status"',
            WRAPPER_SOURCE,
            msg="repeatability must append each successful gate verdict to results.tsv",
        )
        pattern = re.compile(
            r"write_summary\(\)\s*\{.*?echo \"status=\$status\".*?echo \"results_tsv=results\.tsv\"",
            re.DOTALL,
        )
        self.assertRegex(
            WRAPPER_SOURCE,
            pattern,
            msg="repeatability summaries must point at the per-run result ledger",
        )
        self.assertIn(
            'echo "artifacts_root=$ARTIFACTS_ROOT"',
            WRAPPER_SOURCE,
            msg="repeatability summaries must record the shared lca_tree_stress_v5 artifact root they protected",
        )
        self.assertIn(
            'echo "artifact_root_guard=$ROOT_GUARD_MARKER"',
            WRAPPER_SOURCE,
            msg="repeatability summaries must publish the root-guard marker used to detect accidental artifact-root cleanup",
        )

    def test_executes_two_gate_cycles_and_republishes_without_manual_cleanup(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_required_repeatability.sh"
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "required_repeatability"
            stage_root = branch_root / "artifacts" / "lca_tree_stress_v5" / ".repeatability_stage"
            backup_root = repeatability_root.with_name("required_repeatability.previous")
            external_sentinel = artifacts_root / "external_keep.txt"

            self.write_text(external_sentinel, "preserve me\n")

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
            self.assertTrue(external_sentinel.is_file(), msg="repeatability must not delete preexisting shared artifact-root files on the first full-gate cycle")
            self.assertEqual(external_sentinel.read_text(encoding="utf-8"), "preserve me\n")

            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            results = (repeatability_root / "results.tsv").read_text(encoding="utf-8")
            self.assertIn("status=PASS", summary)
            self.assertIn("requested_runs=2", summary)
            self.assertIn("completed_runs=2", summary)
            self.assertIn("baseline_run=runs/run01", summary)
            self.assertIn(f"artifacts_root={artifacts_root.resolve()}", summary)
            self.assertIn("artifact_root_guard=", summary)
            self.assertIn("run01\tlca_strong_gate\tPASS\tbaseline", results)
            self.assertIn("run01\tlca_boj3s_gate\tPASS\tbaseline", results)
            self.assertIn("run02\tlca_strong_gate\tPASS\tmatched_baseline", results)
            self.assertIn("run02\tlca_boj3s_gate\tPASS\tmatched_baseline", results)

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
            self.assertTrue(external_sentinel.is_file(), msg="repeatability must keep the shared artifact root intact across consecutive invocations")
            self.assertEqual(external_sentinel.read_text(encoding="utf-8"), "preserve me\n")
            self.assertIn("status=PASS", (repeatability_root / "summary.txt").read_text(encoding="utf-8"))

    def test_fails_if_a_gate_clears_non_hidden_entries_under_the_shared_artifact_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_required_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "required_repeatability"

            self.write_text(
                branch_root / "lca_boj3s_gate.sh",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                    ROOT="$SCRIPT_DIR/artifacts/lca_tree_stress_v5"
                    OUTROOT="$(python3 "$SCRIPT_DIR/artifact_paths.py" lca_boj3s_gate)"
                    rm -rf "$ROOT"/*
                    mkdir -p "$OUTROOT"
                    cat >"$OUTROOT/certify.json" <<'EOF'
                    {
                      "verdict": "PASS",
                      "preset": "stub_boj3s_gate",
                      "reasons": [],
                      "stages": [
                        {
                          "name": "final",
                          "status": "PASS",
                          "cases": 3,
                          "timeouts": 0,
                          "re_wa": 0,
                          "limit_scale": 1.0,
                          "scale_fail": []
                        }
                      ]
                    }
                    EOF
                    printf 'stub boj3s gate\\n' > "$OUTROOT/certify_summary.md"
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "lca_boj3s_gate.sh")

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )

            self.assertNotEqual(result.returncode, 0, msg="repeatability must fail when a gate clears the shared artifact root")
            self.assertTrue(repeatability_root.is_dir(), msg="failed repeatability runs must still publish their summary bundle")
            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("status=FAIL", summary)
            self.assertIn("failed_gate=lca_boj3s_gate", summary)
            self.assertIn("failure_reason=shared artifact root was cleared during lca_boj3s_gate execution", summary)
            self.assertIn(
                "failure_hint=inspect lca_boj3s_gate cleanup behavior; artifacts/lca_tree_stress_v5 must survive consecutive required-gate reruns",
                summary,
            )

    def test_fails_when_gate_exit_zero_reuses_a_stale_pass_artifact(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_required_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "required_repeatability"
            strong_gate_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate"

            self.write_text(
                strong_gate_root / "certify.json",
                textwrap.dedent(
                    """
                    {
                      "verdict": "PASS",
                      "preset": "stale_strong_gate",
                      "reasons": [],
                      "stages": [
                        {
                          "name": "required",
                          "status": "PASS",
                          "cases": 2,
                          "timeouts": 0,
                          "re_wa": 0,
                          "limit_scale": 1.0,
                          "scale_fail": []
                        }
                      ]
                    }
                    """
                ).strip()
                + "\n",
            )
            self.write_text(strong_gate_root / "certify_summary.md", "stale strong gate\n")
            self.write_text(strong_gate_root / "runtime_env.txt", "stale runtime env\n")
            self.write_text(strong_gate_root / "preflight_manifest.tsv", "kind\tlabel\tstatus\tvalue\npath\tselected_preset\tok\tstale\n")
            self.write_text(strong_gate_root / "selected_preset.json", '{"name":"stale_strong_gate"}\n')
            self.write_text(
                strong_gate_root / "repeatability_gate_manifest.txt",
                "repeatability_run_token=stale\nrepeatability_cycle=run00\nrepeatability_gate_label=lca_strong_gate\n",
            )
            time.sleep(0.02)

            self.write_text(
                branch_root / "lca_strong_gate.sh",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    printf 'pretending to succeed without refreshing artifacts\\n' >&2
                    exit 0
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "lca_strong_gate.sh")

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )

            self.assertNotEqual(
                result.returncode,
                0,
                msg="repeatability must fail when a gate reuses an older PASS artifact instead of regenerating it",
            )
            self.assertTrue(repeatability_root.is_dir(), msg="failed repeatability runs must still publish their summary bundle")
            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            freshness_report = repeatability_root / "runs" / "run01" / "strong_gate.freshness_report.txt"
            self.assertIn("status=FAIL", summary)
            self.assertIn("failed_gate=lca_strong_gate", summary)
            self.assertIn(
                "failure_reason=lca_strong_gate returned exit code 0 but reused prior gate artifacts instead of regenerating current-run pass evidence",
                summary,
            )
            self.assertTrue(
                freshness_report.is_file(),
                msg="repeatability must preserve a per-gate freshness report when it rejects stale PASS reuse",
            )
            freshness_text = freshness_report.read_text(encoding="utf-8")
            self.assertIn("status=stale_or_missing_current_run_artifacts", freshness_text)
            self.assertIn("stale_required=certify_json", freshness_text)
            self.assertIn("stale_required=runtime_env", freshness_text)
            self.assertIn("stale_required=preflight_manifest", freshness_text)
            self.assertIn("stale_required=selected_preset", freshness_text)
            self.assertIn("stale_required=repeatability_manifest", freshness_text)

    def test_fails_when_gate_refreshes_verdict_files_but_reuses_stale_gate_bundle_support_files(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "outer_suite_wrappers" / "lca_required_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "required_repeatability"
            strong_gate_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "strong_gate"

            self.write_text(strong_gate_root / "certify.json", "{\"verdict\":\"PASS\",\"preset\":\"stale\",\"reasons\":[],\"stages\":[]}\n")
            self.write_text(strong_gate_root / "certify_summary.md", "stale summary\n")
            self.write_text(strong_gate_root / "runtime_env.txt", "stale runtime env\n")
            self.write_text(strong_gate_root / "preflight_manifest.tsv", "kind\tlabel\tstatus\tvalue\npath\tselected_preset\tok\tstale\n")
            self.write_text(strong_gate_root / "selected_preset.json", '{"name":"stale"}\n')
            self.write_text(
                strong_gate_root / "repeatability_gate_manifest.txt",
                "repeatability_run_token=stale\nrepeatability_cycle=run00\nrepeatability_gate_label=lca_strong_gate\n",
            )
            time.sleep(0.02)

            self.write_text(
                branch_root / "lca_strong_gate.sh",
                textwrap.dedent(
                    """
                    #!/usr/bin/env bash
                    set -euo pipefail

                    SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
                    OUTROOT="$(python3 "$SCRIPT_DIR/artifact_paths.py" lca_strong_gate)"
                    mkdir -p "$OUTROOT"
                    cat >"$OUTROOT/certify.json" <<'EOF'
                    {
                      "verdict": "PASS",
                      "preset": "fresh_json_only",
                      "reasons": [],
                      "stages": [
                        {
                          "name": "required",
                          "status": "PASS",
                          "cases": 2,
                          "timeouts": 0,
                          "re_wa": 0,
                          "limit_scale": 1.0,
                          "scale_fail": []
                        }
                      ]
                    }
                    EOF
                    printf 'fresh summary\\n' > "$OUTROOT/certify_summary.md"
                    exit 0
                    """
                ).strip()
                + "\n",
            )
            self.make_executable(branch_root / "lca_strong_gate.sh")

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )

            self.assertNotEqual(
                result.returncode,
                0,
                msg="repeatability must fail when a gate refreshes verdict files but leaves runtime_env/preflight stale",
            )
            self.assertTrue(repeatability_root.is_dir(), msg="failed repeatability runs must still publish their summary bundle")
            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            freshness_report = repeatability_root / "runs" / "run01" / "strong_gate.freshness_report.txt"
            self.assertIn("status=FAIL", summary)
            self.assertIn("failed_gate=lca_strong_gate", summary)
            self.assertIn(
                "failure_reason=lca_strong_gate returned exit code 0 but reused prior gate artifacts instead of regenerating current-run pass evidence",
                summary,
            )
            self.assertTrue(
                freshness_report.is_file(),
                msg="repeatability must preserve a freshness report when it rejects stale gate-bundle support files",
            )
            freshness_text = freshness_report.read_text(encoding="utf-8")
            self.assertIn("status=stale_or_missing_current_run_artifacts", freshness_text)
            self.assertIn("stale_required=runtime_env", freshness_text)
            self.assertIn("stale_required=preflight_manifest", freshness_text)
            self.assertIn("stale_required=selected_preset", freshness_text)
            self.assertIn("stale_required=repeatability_manifest", freshness_text)


if __name__ == "__main__":
    unittest.main()
