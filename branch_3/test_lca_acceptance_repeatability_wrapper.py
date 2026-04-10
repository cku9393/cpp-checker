#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


ENTRYPOINT_PATH = Path(__file__).resolve().parent / "lca_acceptance_repeatability.sh"
WRAPPER_PATH = Path(__file__).resolve().parent / "outer_suite_wrappers" / "lca_acceptance_repeatability.sh"
WRAPPER_SOURCE = WRAPPER_PATH.read_text(encoding="utf-8")


class LcaAcceptanceRepeatabilityWrapperRegressionTests(unittest.TestCase):
    def write_text(self, path: Path, content: str) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def make_executable(self, path: Path) -> None:
        path.chmod(path.stat().st_mode | 0o111)

    def symlink_file(self, target: Path, link_path: Path) -> None:
        link_path.parent.mkdir(parents=True, exist_ok=True)
        link_path.symlink_to(target)

    def smoke_stub_source(self, mode: str = "pass") -> str:
        if mode == "pass":
            body = """
            SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
            OUTROOT="$(python3 "$SCRIPT_DIR/artifact_paths.py" lca_smoke)"
            STATUS_ROOT="$SCRIPT_DIR/artifacts/lca_tree_stress_v5/smoke_latest_status"
            SNAPSHOT_ROOT="${LCA_SMOKE_EXPORT_SNAPSHOT_ROOT:?}"

            rm -rf "$OUTROOT" "$STATUS_ROOT" "$SNAPSHOT_ROOT"
            mkdir -p "$OUTROOT/case_alpha" "$STATUS_ROOT" "$SNAPSHOT_ROOT/case_alpha"

            cat >"$OUTROOT/case_alpha/run_case.stdout.txt" <<EOF
            stable-prefix
            [run_case] mode=comb_dense time=0.10 mem=1234
            [run_case] artifacts: /tmp/transient/path
            stable-suffix
            EOF
            cat >"$OUTROOT/case_alpha/run_case_result.json" <<EOF
            {
              "verdict": "PASS",
              "sec": 0.10,
              "rss_kb": 1234,
              "case_tag": "alpha"
            }
            EOF
            printf '0.10\\n' > "$OUTROOT/case_alpha/time.txt"
            cat >"$OUTROOT/case_alpha/solver_env_snapshot.json" <<EOF
            {
              "schema": "branch_run_case_solver_env_snapshot_v1",
              "solver": {
                "exists": true,
                "mtime_ns": 101,
                "path": "$OUTROOT/volatile/solve",
                "sha256": "stable",
                "size_bytes": 777
              },
              "tracked_env": {
                "DENSE_PROFILE_OUTDIR": "$OUTROOT/volatile/case_alpha",
                "ENABLE_LAYOUT_SIGNATURE_GATE_OPT": "1"
              }
            }
            EOF
            printf 'solver ok\\n' > "$OUTROOT/case_alpha/out.txt"
            cp -R "$OUTROOT"/. "$SNAPSHOT_ROOT"/

            cat >"$STATUS_ROOT/summary.txt" <<EOF
            public_status=PASS
            result_family=none
            normalized_exit_code=0
            raw_exit_code=0
            normalized_outcome=pass
            outcome_source=inner_wrapper
            EOF
            printf '# PASS status\\n' > "$STATUS_ROOT/latest_status_report.md"
            """
        elif mode == "drift":
            body = """
            SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
            COUNTER_PATH="$SCRIPT_DIR/artifacts/lca_tree_stress_v5/smoke_counter.txt"
            OUTROOT="$(python3 "$SCRIPT_DIR/artifact_paths.py" lca_smoke)"
            STATUS_ROOT="$SCRIPT_DIR/artifacts/lca_tree_stress_v5/smoke_latest_status"
            SNAPSHOT_ROOT="${LCA_SMOKE_EXPORT_SNAPSHOT_ROOT:?}"

            count=0
            if [[ -f "$COUNTER_PATH" ]]; then
              read -r count < "$COUNTER_PATH"
            fi
            count="$(( count + 1 ))"
            printf '%s\\n' "$count" > "$COUNTER_PATH"

            rm -rf "$OUTROOT" "$STATUS_ROOT" "$SNAPSHOT_ROOT"
            mkdir -p "$OUTROOT/case_alpha" "$STATUS_ROOT" "$SNAPSHOT_ROOT/case_alpha"

            payload='stable-out'
            if (( count > 1 )); then
              payload='drifted-out'
            fi

            printf '%s\\n' "$payload" > "$OUTROOT/case_alpha/out.txt"
            printf '0.10\\n' > "$OUTROOT/case_alpha/time.txt"
            printf 'stable-prefix\\nstable-suffix\\n' > "$OUTROOT/case_alpha/run_case.stdout.txt"
            printf '{"verdict":"PASS","case_tag":"alpha"}\\n' > "$OUTROOT/case_alpha/run_case_result.json"
            printf '{"schema":"branch_run_case_solver_env_snapshot_v1","solver":{"path":"%s/volatile/solve","mtime_ns":1,"sha256":"x"},"tracked_env":{"DENSE_PROFILE_OUTDIR":"%s/volatile/case_alpha"}}\\n' "$OUTROOT" "$OUTROOT" > "$OUTROOT/case_alpha/solver_env_snapshot.json"
            cp -R "$OUTROOT"/. "$SNAPSHOT_ROOT"/

            cat >"$STATUS_ROOT/summary.txt" <<EOF
            public_status=PASS
            result_family=none
            normalized_exit_code=0
            raw_exit_code=0
            normalized_outcome=pass
            outcome_source=inner_wrapper
            EOF
            printf '# PASS status\\n' > "$STATUS_ROOT/latest_status_report.md"
            """
        else:
            raise AssertionError(f"unsupported smoke stub mode: {mode}")

        return (
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n\n"
            + textwrap.dedent(body).strip()
            + "\n"
        )

    def gate_stub_source(self, gate_key: str, preset: str) -> str:
        return (
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n\n"
            + textwrap.dedent(
                f"""
                SCRIPT_DIR="$(cd -- "$(dirname -- "${{BASH_SOURCE[0]}}")" && pwd -P)"
                OUTROOT="$(python3 "$SCRIPT_DIR/artifact_paths.py" {gate_key})"
                rm -rf "$OUTROOT"
                mkdir -p "$OUTROOT"
                printf 'stub runtime env\\n' > "$OUTROOT/runtime_env.txt"
                printf 'kind\\tlabel\\tstatus\\tvalue\\npath\\tselected_preset\\tok\\t{preset}\\n' > "$OUTROOT/preflight_manifest.tsv"
                printf '{{"name":"{preset}"}}\\n' > "$OUTROOT/selected_preset.json"
                cat >"$OUTROOT/repeatability_gate_manifest.txt" <<EOF
                repeatability_run_token=${{LCA_REPEATABILITY_RUN_TOKEN:-}}
                repeatability_cycle=${{LCA_REPEATABILITY_CYCLE:-}}
                repeatability_gate_label=${{LCA_REPEATABILITY_GATE_LABEL:-}}
                EOF
                cat >"$OUTROOT/certify.json" <<'EOF'
                {{
                  "verdict": "PASS",
                  "preset": "{preset}",
                  "reasons": [],
                  "stages": [
                    {{
                      "name": "{gate_key}",
                      "status": "PASS",
                      "cases": 2,
                      "timeouts": 0,
                      "re_wa": 0,
                      "limit_scale": 1.0,
                      "scale_fail": []
                    }}
                  ]
                }}
                EOF
                printf '{preset}\\n' > "$OUTROOT/certify_summary.md"
                """
            ).strip()
            + "\n"
        )

    def make_fake_branch(self, temp_root: Path, *, smoke_mode: str = "pass") -> Path:
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
                    "lca_acceptance_repeatability": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "acceptance_repeatability",
                    "lca_strong_gate": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "strong_gate",
                    "lca_boj3s_gate": ARTIFACTS_ROOT / "lca_tree_stress_v5" / "boj3s_gate",
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
        self.write_text(branch_root / "lca_smoke.sh", self.smoke_stub_source(smoke_mode))
        self.write_text(branch_root / "lca_strong_gate.sh", self.gate_stub_source("lca_strong_gate", "stub_strong_gate"))
        self.write_text(branch_root / "lca_boj3s_gate.sh", self.gate_stub_source("lca_boj3s_gate", "stub_boj3s_gate"))

        self.symlink_file(ENTRYPOINT_PATH, branch_root / "lca_acceptance_repeatability.sh")
        self.symlink_file(WRAPPER_PATH, branch_root / "outer_suite_wrappers" / "lca_acceptance_repeatability.sh")

        for rel_path in (
            Path("solver_release_env.sh"),
            Path("lca_smoke.sh"),
            Path("lca_strong_gate.sh"),
            Path("lca_boj3s_gate.sh"),
        ):
            self.make_executable(branch_root / rel_path)
        return branch_root

    def test_uses_fixed_branch_local_artifact_roots_and_stage_root(self) -> None:
        self.assertIn(
            'ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"',
            WRAPPER_SOURCE,
            msg="acceptance repeatability must stay under the branch-local lca_tree_stress_v5 artifacts root",
        )
        self.assertIn(
            'STAGE_PARENT="$ARTIFACTS_ROOT/.repeatability_stage"',
            WRAPPER_SOURCE,
            msg="acceptance repeatability must stage runs under the shared repeatability staging root",
        )
        self.assertIn(
            'OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_acceptance_repeatability)"',
            WRAPPER_SOURCE,
            msg="acceptance repeatability must publish to a fixed branch-local artifact root",
        )

    def test_reruns_the_full_smoke_strong_boj_sequence_in_each_cycle(self) -> None:
        self.assertIn(
            'REQUIRED_SEQUENCE="lca_smoke -> lca_strong_gate -> lca_boj3s_gate"',
            WRAPPER_SOURCE,
            msg="acceptance repeatability must record the enforced full-flow order",
        )
        self.assertIn(
            'if ! run_smoke_once "$run_dir"; then',
            WRAPPER_SOURCE,
            msg="each repeatability cycle must execute the smoke wrapper first",
        )
        self.assertIn(
            'if ! run_gate_once "strong_gate" "lca_strong_gate" "$STRONG_WRAPPER" "$STRONG_OUTROOT" "$run_dir"; then',
            WRAPPER_SOURCE,
            msg="each repeatability cycle must execute the strong gate after smoke",
        )
        self.assertIn(
            'if ! run_gate_once "boj3s_gate" "lca_boj3s_gate" "$BOJ3S_WRAPPER" "$BOJ3S_OUTROOT" "$run_dir"; then',
            WRAPPER_SOURCE,
            msg="each repeatability cycle must execute the BOJ 3s gate after the strong gate",
        )

    def test_gate_repeatability_tracks_selected_preset_and_stable_runtime_config(self) -> None:
        self.assertIn(
            'selected_preset.json',
            WRAPPER_SOURCE,
            msg="acceptance repeatability must require each gate rerun to republish the selected preset snapshot",
        )
        self.assertIn(
            'selected_preset_sha256',
            WRAPPER_SOURCE,
            msg="acceptance repeatability gate signatures must include the selected preset digest",
        )
        self.assertIn(
            'runtime_env_config',
            WRAPPER_SOURCE,
            msg="acceptance repeatability gate signatures must include the stable runtime-config subset",
        )
        self.assertIn(
            'selected_preset.json repeatability_gate_manifest.txt',
            WRAPPER_SOURCE,
            msg="acceptance repeatability freshness probes must treat selected_preset.json and repeatability_gate_manifest.txt as required current-run artifacts",
        )

    def test_executes_two_full_cycles_and_republishes_without_manual_cleanup(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp))
            wrapper_path = branch_root / "lca_acceptance_repeatability.sh"
            artifacts_root = branch_root / "artifacts" / "lca_tree_stress_v5"
            repeatability_root = artifacts_root / "acceptance_repeatability"
            stage_root = artifacts_root / ".repeatability_stage"
            backup_root = artifacts_root / "acceptance_repeatability.previous"
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
            self.assertFalse(stage_root.exists(), msg="staging dir should be cleaned after publish")
            self.assertFalse(backup_root.exists(), msg="backup output should not survive publish")
            self.assertTrue(external_sentinel.is_file(), msg="shared artifact-root content must survive the full-flow rerun helper")

            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            results = (repeatability_root / "results.tsv").read_text(encoding="utf-8")
            self.assertIn("status=PASS", summary)
            self.assertIn("requested_runs=2", summary)
            self.assertIn("completed_runs=2", summary)
            self.assertIn("baseline_run=runs/run01", summary)
            self.assertIn(f"artifacts_root={artifacts_root.resolve()}", summary)
            self.assertIn("run01\tlca_smoke\tPASS\tbaseline", results)
            self.assertIn("run01\tlca_strong_gate\tPASS\tbaseline", results)
            self.assertIn("run01\tlca_boj3s_gate\tPASS\tbaseline", results)
            self.assertIn("run02\tlca_smoke\tPASS\tmatched_baseline", results)
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
            self.assertFalse(backup_root.exists(), msg="backup output should not survive a clean rerun")
            self.assertEqual((repeatability_root / "summary.txt").read_text(encoding="utf-8").splitlines()[0], "status=PASS")

    def test_fails_when_repeated_smoke_snapshot_drift_breaks_the_full_flow(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            branch_root = self.make_fake_branch(Path(tmp), smoke_mode="drift")
            wrapper_path = branch_root / "lca_acceptance_repeatability.sh"
            repeatability_root = branch_root / "artifacts" / "lca_tree_stress_v5" / "acceptance_repeatability"

            result = subprocess.run(
                [str(wrapper_path), "2"],
                cwd=branch_root,
                check=False,
                capture_output=True,
                text=True,
            )

            self.assertNotEqual(result.returncode, 0, msg="manifest drift across repeated full-flow cycles must fail the helper")
            self.assertTrue(repeatability_root.is_dir(), msg="failed runs must still publish the acceptance repeatability bundle")
            summary = (repeatability_root / "summary.txt").read_text(encoding="utf-8")
            self.assertIn("status=FAIL", summary)
            self.assertIn("failed_stage=lca_smoke", summary)
            self.assertIn("failure_reason=smoke divergence between run01 and run02", summary)
            self.assertTrue(
                (repeatability_root / "runs" / "run02" / "smoke_manifest_diff.txt").is_file(),
                msg="smoke drift must preserve a manifest diff for the repeated full-flow failure",
            )


if __name__ == "__main__":
    unittest.main()
