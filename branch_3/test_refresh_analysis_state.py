#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent


class RefreshAnalysisStateFallbackTests(unittest.TestCase):
    def run_helper(self, *args: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(REPO_ROOT / ".ouroboros" / "refresh_analysis_state.py"), *args],
            check=False,
            capture_output=True,
            text=True,
        )

    def test_refresh_accepts_fallback_failure_breakdown_without_failed_acs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            attempt_dir = temp_root / "attempt_003"
            report_root = temp_root / "retry_loop"
            attempt_dir.mkdir(parents=True, exist_ok=True)
            report_root.mkdir(parents=True, exist_ok=True)

            (attempt_dir / "failure_report.json").write_text(
                json.dumps(
                    {
                        "attempt": 3,
                        "timestamp": "2026-03-28 08:07:57 KST",
                        "session_id": None,
                        "execution_id": None,
                        "failed_acs": [],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
            (attempt_dir / "failure_breakdown.json").write_text(
                json.dumps(
                    {
                        "attempt": 3,
                        "timestamp": "2026-03-28 08:07:57 KST",
                        "session_id": None,
                        "execution_id": None,
                        "failed_ac_breakdowns": [],
                        "fallback_failure_breakdown": {
                            "title": "Pre-AC Failure: retry-orchestration preflight",
                            "failure_type": "orchestration-preflight",
                            "failure_family": "analysis_preflight_soft_stop_argv_provenance",
                            "interpretation_lane": "retry-preflight",
                            "primary_axis": "zero_span_fastpath",
                            "secondary_axis": None,
                            "profile_mode": None,
                            "last_progress_checkpoint_phase": None,
                            "last_release_diag_phase": None,
                            "next_probe_command": "python3 - <<'PY'\nprint('probe')\nPY",
                            "current_summary_pivot": "zero-span eligibility and fastpath commit",
                            "structural_focus": [],
                        },
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )

            analysis_log = attempt_dir / "analysis.log"
            analysis_log.write_text("analysis ok\n", encoding="utf-8")
            state_path = temp_root / "failure_analysis_state.json"
            iteration_path = temp_root / "failure_analysis_iteration.md"

            result = self.run_helper(
                "--attempt",
                "3",
                "--attempt-dir",
                str(attempt_dir),
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

            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            state = json.loads(state_path.read_text(encoding="utf-8"))
            self.assertTrue(state["current_for_latest_failure"])
            self.assertEqual(state["current_failure_attempt"], "attempt_003")
            self.assertEqual(state["pinned_primary_axis"], "zero_span_fastpath")
            self.assertIn("probe", state["next_probe_command"])
            freshness_record = state["refresh_evidence"]["freshness_record"]
            self.assertEqual(freshness_record["failure_signature"], state["current_failure_signature"])
            self.assertEqual(freshness_record["refreshed_asset"], str(iteration_path.resolve()))
            self.assertTrue(iteration_path.exists())

    def test_refresh_accepts_current_state_nominal_pass_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            attempt_dir = temp_root / "attempt_006"
            report_root = temp_root / "retry_loop"
            attempt_dir.mkdir(parents=True, exist_ok=True)
            report_root.mkdir(parents=True, exist_ok=True)

            (attempt_dir / "failure_report.json").write_text(
                json.dumps(
                    {
                        "attempt": 6,
                        "timestamp": "2026-03-29 03:27:45 KST",
                        "session_id": "orch_8001613c3b7a",
                        "execution_id": "exec_ec9b44882b50",
                        "exit_code": 90,
                        "failed_acs": [],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
            (attempt_dir / "failure_breakdown.json").write_text(
                json.dumps(
                    {
                        "attempt": 6,
                        "timestamp": "2026-03-29 03:27:45 KST",
                        "session_id": "orch_8001613c3b7a",
                        "execution_id": "exec_ec9b44882b50",
                        "failed_ac_breakdowns": [],
                        "fallback_failure_breakdown": None,
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )

            analysis_log = attempt_dir / "analysis.log"
            analysis_log.write_text("analysis ok\n", encoding="utf-8")
            state_path = temp_root / "failure_analysis_state.json"
            state_path.write_text(
                json.dumps(
                    {
                        "analysis_revision": 94,
                        "current_for_latest_failure": True,
                        "current_failure_attempt": "attempt_006",
                        "current_failure_session_id": "orch_8001613c3b7a",
                        "current_failure_timestamp": "2026-03-29 03:27:45 KST",
                        "pinned_primary_axis": "zero_span_fastpath",
                        "pinned_secondary_axis": None,
                        "pinned_acs": ["5"],
                        "pinned_paths": ["/tmp/focus.cpp"],
                        "pinned_symbols": ["focus range [10-20]"],
                        "next_probe_command": "python3 branch_run_case.py comb_dense 50000 1 1 1 solver out --timeout 30",
                        "next_narrowing_target": "focus range [10-20]",
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
            iteration_path = temp_root / "failure_analysis_iteration.md"

            result = self.run_helper(
                "--attempt",
                "6",
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--analysis-log",
                str(analysis_log),
                "--analysis-round",
                "3",
                "--state-file",
                str(state_path),
                "--iteration-file",
                str(iteration_path),
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            state = json.loads(state_path.read_text(encoding="utf-8"))
            self.assertEqual(state["current_failure_attempt"], "attempt_006")
            self.assertEqual(state["pinned_primary_axis"], "zero_span_fastpath")
            self.assertEqual(state["pinned_acs"], ["5"])
            self.assertIn("branch_run_case.py", state["next_probe_command"])
            self.assertTrue(iteration_path.exists())

    def test_refresh_preserves_statement_level_retry_anchors(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            attempt_dir = temp_root / "attempt_017"
            report_root = temp_root / "retry_loop"
            attempt_dir.mkdir(parents=True, exist_ok=True)
            report_root.mkdir(parents=True, exist_ok=True)

            (attempt_dir / "failure_report.json").write_text(
                json.dumps(
                    {
                        "attempt": 17,
                        "timestamp": "2026-04-03 23:59:03 KST",
                        "session_id": "orch_5a1dcf0d5735",
                        "execution_id": "exec_aecf6c8c2a5a",
                        "failed_acs": ["3"],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
            anchor_path = "/tmp/branch_certify_suite.py"
            (attempt_dir / "failure_breakdown.json").write_text(
                json.dumps(
                    {
                        "attempt": 17,
                        "timestamp": "2026-04-03 23:59:03 KST",
                        "session_id": "orch_5a1dcf0d5735",
                        "execution_id": "exec_aecf6c8c2a5a",
                        "failed_ac_breakdowns": [
                            {
                                "ac_index": "3",
                                "failure_type": "stall/no-activity",
                                "failure_family": "strong_gate_timeout_cluster",
                                "interpretation_lane": "correctness-proof",
                                "primary_axis": "zero_span_fastpath",
                                "secondary_axis": None,
                                "profile_mode": None,
                                "next_probe_command": "LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh",
                                "current_summary_pivot": "zero-span eligibility and fastpath commit",
                                "structural_focus": [
                                    {
                                        "path": "/tmp/outer_suite_wrappers/lca_strong_gate.sh",
                                        "enclosing_symbols": ["function acquire_lock [266-308]"],
                                    }
                                ],
                                "retry_critical_anchors": [
                                    {
                                        "label": "branch_run_solver_with_time handoff",
                                        "path": anchor_path,
                                        "focus_range": "540-549",
                                        "symbol": "function run_one_case [491-607]",
                                        "statement_excerpt": "rc_sol, to_sol, sec, rss = branch_run_solver_with_time(...)",
                                        "note": "fresh attempt-local certify rows say all current failures are solver_rc=-9 timeouts, so this is the first live helper-side ingress",
                                        "evidence_lines": [
                                            "AC 3 → Bash: /bin/zsh -lc \"sed -n '491,620p' branch_certify_suite.py\""
                                        ],
                                    },
                                    {
                                        "label": "solver_timeout publication",
                                        "path": anchor_path,
                                        "focus_range": "557-568",
                                        "symbol": "function run_one_case [491-607]",
                                        "statement_excerpt": "_write_case_result(work_dir, status=\"solver_timeout\", ...)",
                                        "note": "this exact timeout branch is the first persisted timeout payload after the live solver call",
                                        "evidence_lines": [
                                            "AC 3 → Bash: /bin/zsh -lc \"sed -n '491,620p' branch_certify_suite.py\""
                                        ],
                                    },
                                ],
                            }
                        ],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )

            analysis_log = attempt_dir / "analysis.log"
            analysis_log.write_text("analysis ok\n", encoding="utf-8")
            state_path = temp_root / "failure_analysis_state.json"
            iteration_path = temp_root / "failure_analysis_iteration.md"

            result = self.run_helper(
                "--attempt",
                "17",
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--analysis-log",
                str(analysis_log),
                "--analysis-round",
                "2",
                "--state-file",
                str(state_path),
                "--iteration-file",
                str(iteration_path),
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            state = json.loads(state_path.read_text(encoding="utf-8"))
            self.assertEqual(state["pinned_primary_axis"], "zero_span_fastpath")
            self.assertEqual(state["pinned_acs"], ["3"])
            self.assertEqual(state["pinned_paths"][0], anchor_path)
            self.assertEqual(
                state["latest_retry_statement_anchors"][0]["focus_range"],
                "540-549",
            )
            self.assertEqual(
                state["latest_retry_statement_anchors"][0]["label"],
                "branch_run_solver_with_time handoff",
            )
            self.assertIn(
                f"{anchor_path}:540-549",
                state["latest_retry_anchor_ranges"],
            )
            self.assertIn(
                "::branch_run_solver_with_time handoff [540-549]",
                state["next_narrowing_target"],
            )

            iteration_text = iteration_path.read_text(encoding="utf-8")
            self.assertIn("## Latest Retry Failure Points", iteration_text)
            self.assertIn(
                f"`{anchor_path}::branch_run_solver_with_time handoff [540-549]`",
                iteration_text,
            )
            self.assertIn(
                "Statement: `rc_sol, to_sol, sec, rss = branch_run_solver_with_time(...)`",
                iteration_text,
            )
            self.assertIn("Role: `launcher-side ingress`", iteration_text)

    def test_refresh_prefers_same_failure_iteration_overlay_over_stale_breakdown_anchors(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            attempt_dir = temp_root / "attempt_017"
            report_root = temp_root / "retry_loop"
            attempt_dir.mkdir(parents=True, exist_ok=True)
            report_root.mkdir(parents=True, exist_ok=True)

            failure_signature = "attempt_017|orch_5a1dcf0d5735|2026-04-03 23:59:03 KST|3"
            (attempt_dir / "failure_report.json").write_text(
                json.dumps(
                    {
                        "attempt": 17,
                        "timestamp": "2026-04-03 23:59:03 KST",
                        "session_id": "orch_5a1dcf0d5735",
                        "execution_id": "exec_aecf6c8c2a5a",
                        "failed_acs": ["3"],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
            (attempt_dir / "failure_breakdown.json").write_text(
                json.dumps(
                    {
                        "attempt": 17,
                        "timestamp": "2026-04-03 23:59:03 KST",
                        "session_id": "orch_5a1dcf0d5735",
                        "execution_id": "exec_aecf6c8c2a5a",
                        "failed_ac_breakdowns": [
                            {
                                "ac_index": "3",
                                "failure_type": "stall/no-activity",
                                "failure_family": "strong_gate_timeout_cluster",
                                "interpretation_lane": "correctness-proof",
                                "primary_axis": "zero_span_fastpath",
                                "secondary_axis": None,
                                "profile_mode": None,
                                "next_probe_command": "LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh",
                                "current_summary_pivot": "zero-span eligibility and fastpath commit",
                                "retry_critical_anchors": [
                                    {
                                        "label": "Timed solver invocation",
                                        "path": "/tmp/branch_certify_suite.py",
                                        "focus_range": "466-475",
                                        "symbol": "function _write_case_result [461-488]",
                                        "statement_excerpt": "status: str,",
                                        "note": "stale carried-forward helper-header anchor",
                                    }
                                ],
                                "structural_focus": [],
                            }
                        ],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )

            analysis_log = attempt_dir / "analysis.log"
            analysis_log.write_text("analysis ok\n", encoding="utf-8")
            state_path = temp_root / "failure_analysis_state.json"
            state_path.write_text(
                json.dumps(
                    {
                        "analysis_revision": 159,
                        "current_for_latest_failure": True,
                        "current_failure_attempt": "attempt_017",
                        "current_failure_session_id": "orch_5a1dcf0d5735",
                        "current_failure_timestamp": "2026-04-03 23:59:03 KST",
                        "current_failure_signature": failure_signature,
                        "pinned_primary_axis": "zero_span_fastpath",
                        "pinned_secondary_axis": None,
                        "pinned_acs": ["3"],
                        "pinned_paths": ["/tmp/branch_certify_suite.py"],
                        "pinned_symbols": ["branch_run_solver_with_time handoff [540-549]"],
                        "next_probe_command": "bash -lc 'source ./solver_release_env.sh && python3 branch_run_case.py caterpillar_rect_dense 1024 1 0 0 ./boj28350_resume/solve out --timeout 2'",
                        "why_this_axis": "carry forward the same-failure single-case plateau probe",
                        "latest_retry_statement_anchors": [
                            {
                                "path": "/tmp/branch_certify_suite.py",
                                "label": "Timed solver invocation",
                                "focus_range": "466-475",
                                "symbol": "function _write_case_result [461-488]",
                                "excerpt": "status: str,",
                                "evidence": "stale carried-forward helper-header anchor",
                                "role": "launcher-side ingress",
                            }
                        ],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
            iteration_path = temp_root / "failure_analysis_iteration.md"
            iteration_path.write_text(
                "\n".join(
                    [
                        "# Failure Analysis Iteration Ledger",
                        "",
                        "- Current failure signature: `attempt_017|orch_5a1dcf0d5735|2026-04-03 23:59:03 KST|3`",
                        "",
                        "## Latest Retry Failure Points",
                        "",
                        "1. `/tmp/branch_certify_suite.py::branch_run_solver_with_time handoff [540-549]`",
                        "   Statement: `rc_sol, to_sol, sec, rss = branch_run_solver_with_time(...)`",
                        "   Evidence: `current-attempt certify rows say timeout=26 and solver_rc=-9 x26`",
                        "   Role: `launcher-side ingress`",
                        "2. `/tmp/branch_certify_suite.py::solver_timeout publication [557-568]`",
                        "   Statement: `_write_case_result(work_dir, status=\"solver_timeout\", ...)`",
                        "   Evidence: `this is the first persisted timeout payload after the live solver call`",
                        "   Role: `published timeout record`",
                        "",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            result = self.run_helper(
                "--attempt",
                "17",
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--analysis-log",
                str(analysis_log),
                "--analysis-round",
                "2",
                "--state-file",
                str(state_path),
                "--iteration-file",
                str(iteration_path),
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            state = json.loads(state_path.read_text(encoding="utf-8"))
            self.assertEqual(
                state["latest_retry_statement_anchors"][0]["label"],
                "branch_run_solver_with_time handoff",
            )
            self.assertEqual(
                state["latest_retry_statement_anchors"][0]["focus_range"],
                "540-549",
            )
            self.assertIn(
                "::branch_run_solver_with_time handoff [540-549]",
                state["next_narrowing_target"],
            )
            self.assertIn("branch_run_case.py", state["next_probe_command"])

            iteration_text = iteration_path.read_text(encoding="utf-8")
            self.assertIn(
                "`/tmp/branch_certify_suite.py::branch_run_solver_with_time handoff [540-549]`",
                iteration_text,
            )
            self.assertNotIn("`/tmp/branch_certify_suite.py::Timed solver invocation [466-475]`", iteration_text)

    def test_refresh_does_not_carry_forward_stale_axes_or_anchors_for_new_failure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            attempt_dir = temp_root / "attempt_028"
            report_root = temp_root / "retry_loop"
            attempt_dir.mkdir(parents=True, exist_ok=True)
            report_root.mkdir(parents=True, exist_ok=True)

            smoke_path = temp_root / "lca_smoke.sh"
            smoke_path.write_text(
                "\n".join(f"line {idx}" for idx in range(1, 21)) + "\n",
                encoding="utf-8",
            )

            (attempt_dir / "failure_report.json").write_text(
                json.dumps(
                    {
                        "attempt": 28,
                        "timestamp": "2026-04-11 02:16:52 KST",
                        "session_id": "orch_dd0f78e991dd",
                        "execution_id": "exec_a1f372e5ef17",
                        "failed_acs": ["2", "3"],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
            (attempt_dir / "failure_breakdown.json").write_text(
                json.dumps(
                    {
                        "attempt": 28,
                        "timestamp": "2026-04-11 02:16:52 KST",
                        "session_id": "orch_dd0f78e991dd",
                        "execution_id": "exec_a1f372e5ef17",
                        "failed_ac_breakdowns": [
                            {
                                "ac_index": "2",
                                "failure_type": "stall/no-activity",
                                "failure_family": "generic_retry_failure",
                                "interpretation_lane": "pre-gate-stability",
                                "primary_axis": "zero_span_fastpath",
                                "secondary_axis": None,
                                "profile_mode": None,
                                "next_probe_command": "./lca_smoke.sh",
                                "current_summary_pivot": "zero-span eligibility and fastpath commit",
                                "retry_critical_anchors": [],
                                "structural_focus": [
                                    {
                                        "path": str(smoke_path),
                                        "focus_ranges": ["12-12", "15-16"],
                                        "enclosing_symbols": [
                                            "function classify_inner_wrapper_exit [10-18]"
                                        ],
                                        "note": "current attempt structural focus from smoke wrapper trace",
                                    }
                                ],
                            }
                        ],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )

            analysis_log = attempt_dir / "analysis.log"
            analysis_log.write_text("analysis ok\n", encoding="utf-8")
            state_path = temp_root / "failure_analysis_state.json"
            state_path.write_text(
                json.dumps(
                    {
                        "analysis_revision": 189,
                        "current_for_latest_failure": True,
                        "current_failure_attempt": "attempt_027",
                        "current_failure_session_id": "orch_21832d83e390",
                        "current_failure_timestamp": "2026-04-11 00:24:14 KST",
                        "current_failure_signature": "attempt_027|orch_21832d83e390|2026-04-11 00:24:14 KST|2,3,8",
                        "pinned_primary_axis": "zero_span_fastpath",
                        "pinned_secondary_axis": "watch_diff",
                        "pinned_acs": ["2"],
                        "pinned_paths": ["/tmp/stale.cpp"],
                        "pinned_symbols": ["function stale_owner [90-99]"],
                        "next_probe_command": "./lca_smoke.sh",
                        "latest_retry_statement_anchors": [
                            {
                                "path": "/tmp/stale.cpp",
                                "label": "stale carry-forward anchor",
                                "focus_range": "90-99",
                                "symbol": "function stale_owner [90-99]",
                                "excerpt": "stale();",
                                "evidence": "old failure signature only",
                                "role": "solver-side fallback owner",
                            }
                        ],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
            iteration_path = temp_root / "failure_analysis_iteration.md"
            iteration_path.write_text(
                "\n".join(
                    [
                        "# Failure Analysis Iteration Ledger",
                        "",
                        "- Current failure signature: `attempt_027|orch_21832d83e390|2026-04-11 00:24:14 KST|2,3,8`",
                        "",
                        "## Latest Retry Failure Points",
                        "",
                        "1. `/tmp/stale.cpp::stale carry-forward anchor [90-99]`",
                        "   Statement: `stale();`",
                        "   Evidence: `old failure signature only`",
                        "   Role: `solver-side fallback owner`",
                        "",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            result = self.run_helper(
                "--attempt",
                "28",
                "--attempt-dir",
                str(attempt_dir),
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

            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            state = json.loads(state_path.read_text(encoding="utf-8"))
            self.assertEqual(state["current_failure_attempt"], "attempt_028")
            self.assertIsNone(state["pinned_secondary_axis"])
            self.assertEqual(state["pinned_paths"][0], str(smoke_path))
            self.assertEqual(
                state["latest_retry_statement_anchors"][0]["path"],
                str(smoke_path),
            )
            self.assertEqual(
                state["latest_retry_statement_anchors"][0]["focus_range"],
                "12-12",
            )
            self.assertNotEqual(
                state["latest_retry_statement_anchors"][0]["path"],
                "/tmp/stale.cpp",
            )
            self.assertIn(
                f"{smoke_path}::focus 12-12 [12-12]",
                state["next_narrowing_target"],
            )

    def test_refresh_uses_focused_ac_from_guard_rejected_nominal_pass_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            temp_root = Path(tmp)
            attempt_dir = temp_root / "attempt_006"
            report_root = temp_root / "retry_loop"
            attempt_dir.mkdir(parents=True, exist_ok=True)
            report_root.mkdir(parents=True, exist_ok=True)

            (attempt_dir / "failure_report.json").write_text(
                json.dumps(
                    {
                        "attempt": 6,
                        "timestamp": "2026-03-29 03:27:45 KST",
                        "session_id": "orch_8001613c3b7a",
                        "execution_id": "exec_ec9b44882b50",
                        "exit_code": 90,
                        "failed_acs": [],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
            (attempt_dir / "failure_breakdown.json").write_text(
                json.dumps(
                    {
                        "attempt": 6,
                        "timestamp": "2026-03-29 03:27:45 KST",
                        "session_id": "orch_8001613c3b7a",
                        "execution_id": "exec_ec9b44882b50",
                        "failed_ac_breakdowns": [],
                        "fallback_failure_breakdown": {
                            "title": "Guard-Rejected Nominal PASS",
                            "failure_type": "guard-rejected-nominal-pass",
                            "failure_family": "analysis_guard_rejected_nominal_pass",
                            "interpretation_lane": "performance-profile",
                            "primary_axis": "zero_span_fastpath",
                            "secondary_axis": None,
                            "profile_mode": None,
                            "last_progress_checkpoint_phase": None,
                            "last_release_diag_phase": None,
                            "next_probe_command": "python3 branch_run_case.py comb_dense 50000 1 1 1 solver out --timeout 30",
                            "current_summary_pivot": "focus range [10-20]",
                            "structural_focus": [],
                            "guard_implicated_acs": ["5", "6"],
                            "focused_ac_index": "5",
                        },
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )

            analysis_log = attempt_dir / "analysis.log"
            analysis_log.write_text("analysis ok\n", encoding="utf-8")
            state_path = temp_root / "failure_analysis_state.json"
            iteration_path = temp_root / "failure_analysis_iteration.md"

            result = self.run_helper(
                "--attempt",
                "6",
                "--attempt-dir",
                str(attempt_dir),
                "--report-root",
                str(report_root),
                "--analysis-log",
                str(analysis_log),
                "--analysis-round",
                "3",
                "--state-file",
                str(state_path),
                "--iteration-file",
                str(iteration_path),
            )

            self.assertEqual(result.returncode, 0, msg=result.stderr or result.stdout)
            state = json.loads(state_path.read_text(encoding="utf-8"))
            self.assertEqual(state["pinned_acs"], ["5"])
            self.assertEqual(state["pinned_primary_axis"], "zero_span_fastpath")
            self.assertEqual(state["current_failure_failed_acs"], [])
            self.assertIn("branch_run_case.py", state["next_probe_command"])
            self.assertTrue(iteration_path.exists())


if __name__ == "__main__":
    unittest.main()
