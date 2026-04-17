#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import csv
import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable


os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from retry_artifact_io import (
    copy_output_file,
    prepare_output_dir,
    resolve_artifact_output_path,
    write_text_output,
)

REPO_ROOT = Path(__file__).resolve().parent.parent


ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
SESSION_RE = re.compile(r"\borch_[A-Za-z0-9]+\b")
EXEC_RE = re.compile(r"\bexec_[A-Za-z0-9]+\b")
ATTEMPT_DIR_RE = re.compile(r"attempt_\d+_(?P<date>\d{8})_(?P<time>\d{6})$")
FAILED_AC_RE = re.compile(r"### AC (\d+): \[FAIL\] (.+)")
BLOCKED_AC_RE = re.compile(r"### AC (\d+): \[BLOCKED\] (.+)")
PASS_AC_RE = re.compile(r"### AC (\d+): \[PASS\] (.+)")
FILE_MENTION_RE = re.compile(
    r"(?P<path>(?:\.\.?/)?[A-Za-z0-9_./-]+\.(?:py|sh|cpp|hpp|cc|cxx|md|json|yaml|yml|tsv))"
)
SED_RANGE_RE = re.compile(
    r"sed -n ['\"](?P<start>\d+),(?P<end>\d+)p['\"]\s+(?P<path>(?:\.\.?/)?[A-Za-z0-9_./-]+\.[A-Za-z0-9_+-]+)"
)
NL_RANGE_RE = re.compile(
    r"nl -ba\s+(?P<path>(?:\.\.?/)?[A-Za-z0-9_./-]+\.[A-Za-z0-9_+-]+)\s+\|\s+sed -n ['\"](?P<start>\d+),(?P<end>\d+)p['\"]"
)
ANCHOR_RANGE_RE = re.compile(r"^(?P<path>.+):(?P<start>\d+)-(?P<end>\d+)$")
FOCUS_RANGE_RE = re.compile(r"^(?P<start>\d+)-(?P<end>\d+)$")
ITERATION_FAILURE_POINT_HEADER_RE = re.compile(r"^(?:\d+\.\s+|-\s+)`(?P<anchor>[^`]+)`\s*$")
ITERATION_FAILURE_POINT_DETAIL_RE = re.compile(
    r"^\s+(?P<label>Statement|Evidence|Role):\s*(?P<body>.+?)\s*$"
)
ITERATION_FAILURE_POINT_ANCHOR_RE = re.compile(
    r"^(?P<path>(?:\.\.?/)?[A-Za-z0-9_./-]+\.[A-Za-z0-9_+-]+)"
    r"(?:::(?P<label>.+?))?\s*\[(?P<start>\d+)-(?P<end>\d+)\]\s*$"
)
TRACEBACK_FILE_LINE_RE = re.compile(
    r'File "(?P<path>[^"]+)", line (?P<line>\d+)(?:, in (?P<func>[A-Za-z0-9_<>]+))?'
)
ENOSPC_PATH_RE = re.compile(
    r"OSError: \[Errno 28\] No space left on device: ['\"](?P<path>[^'\"]+)['\"]"
)
GUARD_QA_AC_MENTION_RE = re.compile(r"\bAC\s*(?P<ac>\d+)\b")
CASE_BLOCKER_RE = re.compile(
    r"(?P<mode>[A-Za-z_]+)\s+(?P<n>\d+)\s+seed[= ](?P<seed>\d+)\s+L(?P<label>\d+)\s+Q(?P<query>\d+)"
)
AC_TRACE_LINE_RE = re.compile(r"\s*(?:AC \d+|Sub-AC \d+ of AC \d+)\s+→")
ENABLE_FLAG_RE = re.compile(r"\bENABLE_[A-Z0-9_]+\b")
PROFILE_MODE_RE = re.compile(r"\bPROFILE_(?:NONE|BASE|SAMPLED)\b")
PROGRESS_PHASE_RE = re.compile(r"\[progress\]\s+phase=([A-Za-z0-9_./-]+)")
RELEASE_DIAG_PHASE_RE = re.compile(r"\[release_diag\]\s+phase=([A-Za-z0-9_./-]+)")
PROBE_ACTIVE_GATE_RE = re.compile(
    r"another (?P<gate>lca_[a-z0-9_]+\.sh) run is active \(pid (?P<pid>\d+)\)"
)
PROBE_COMPLETED_CASES_RE = re.compile(r"completed_cases=(?P<count>\d+)")
AC_OWNER_RE = re.compile(r"^\s*AC (?P<ac>\d+)\b")
SUB_AC_OWNER_RE = re.compile(r"\bSub-AC \d+ of AC (?P<ac>\d+)\b")
MARKDOWN_AC_OWNER_RE = re.compile(r"### AC (?P<ac>\d+)\b")

CODE_SUFFIXES = {".py", ".sh", ".cpp", ".hpp", ".cc", ".cxx"}
TEXTUAL_RETRY_ANCHOR_SUFFIXES = CODE_SUFFIXES | {".json", ".log", ".md", ".txt"}
READ_ONLY_TRACE_MARKERS = (
    "sed -n ",
    "nl -ba ",
    "rg -n ",
    "find ",
    "tail -n ",
    "cat ",
    "diff -u ",
    "bash -n ",
    "python3 - <<'py'",
    'python3 - <<"py"',
    "python3 -c ",
)
CERTIFY_RUNTIME_MARKERS = (
    "branch_certify_suite.py --solver",
    "certify_suite.py --solver",
    "branch_certify_suite.py --preset",
    "certify_suite.py --preset",
    "branch_certify_report_outdir=",
    "completed_cases=",
)
PHASE_RULES = (
    ("build", ("./build.sh", " build.py", "clang++", "clang -cc1")),
    (
        "concurrency-lock",
        ("run is active", "LOCKDIR", "LOCK_PID_FILE", "acquire_lock", ".locks/lca_"),
    ),
    (
        "wrapper",
        ("./lca_smoke.sh", "./lca_strong_gate.sh", "./lca_boj3s_gate.sh"),
    ),
    ("diagnostic-wrapper", ("./lca_rebuttal_gate.sh", "./lca_hunt.sh")),
    ("certify", ("branch_certify_suite.py", "certify_suite.py", "strong_gate.json", "boj3s_gate.json")),
    ("case-runner", ("branch_run_case.py", "branch_gen_case.py", "case_runs", "run_case/")),
    ("artifact-paths", ("artifact_paths.py", "gen_case_aux", "run_case")),
    ("solver-runtime", ("boj28350_resume/solve",)),
    (
        "solver-source",
        (
            "boj28350_branch_3_solver.cpp",
            "layout_signature",
            "progress40",
            "git diff --no-index --stat",
            "diff -u --speed-large-files",
        ),
    ),
    (
        "timing-timeout",
        ("suite_utils.py", "solver_release_env.sh", "timeout", "sec_max", "worst_ratio", "hard_scaling"),
    ),
)
AXIS_RULES = (
    ("watch_diff", ("watch", "watch_diff", "connector watch", "connector_watch", "watch scan")),
    ("retain_compaction", ("retain", "compaction", "kept vector", "stable compaction", "block copy", "copy plan")),
    ("state_materialization", ("state load", "materialization", "signature source load", "tscan", "state_load")),
    ("carry_writeback", ("carry reuse", "carry hit", "writeback", "prev state", "carry_reuse")),
    ("pointer_rebind", ("pointer rebind", "target resolve", "prebind", "rebind commit", "pointer_rebind")),
    ("slot_owner_patch", ("slot-owner", "slot owner", "metadata patch", "field patch", "owner update", "slot_owner")),
    ("layout_gate", ("layout signature", "layout-gate", "layout gate", "signature gate", "reuse gate")),
    ("zero_span_fastpath", ("zero-span", "zero span", "fastpath commit", "fastpath", "zero_elision")),
)
DEFAULT_AXIS_BY_AC = {
    3: "layout_gate",
    4: "layout_gate",
    5: "zero_span_fastpath",
    6: "zero_span_fastpath",
}
AC_FILE_HINTS = {
    1: [
        "boj28350_resume/README.md",
        "boj28350_resume/current_state_summary.md",
        "boj28350_resume/next_session_briefing.md",
        "boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp",
    ],
    2: [
        "lca_smoke.sh",
        "lca_smoke_repeatability.sh",
        "artifact_paths.py",
        "branch_run_case.py",
        "suite_utils.py",
        "boj28350_resume/boj28350_branch_3_solver.cpp",
    ],
    3: [
        "lca_strong_gate.sh",
        "outer_suite_wrappers/lca_strong_gate.sh",
        "branch_certify_suite.py",
        "boj28350_resume/boj28350_branch_3_solver.cpp",
        "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
    ],
    4: [
        "lca_strong_gate.sh",
        "outer_suite_wrappers/lca_strong_gate.sh",
        "branch_certify_suite.py",
        "suite_utils.py",
        "boj28350_resume/boj28350_branch_3_solver.cpp",
    ],
    5: [
        "lca_boj3s_gate.sh",
        "outer_suite_wrappers/lca_boj3s_gate.sh",
        "branch_certify_suite.py",
        "suite_utils.py",
        "build.py",
        "boj28350_resume/boj28350_branch_3_solver.cpp",
        "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
    ],
    6: [
        "lca_boj3s_gate.sh",
        "outer_suite_wrappers/lca_boj3s_gate.sh",
        "branch_certify_suite.py",
        "suite_utils.py",
        "boj28350_resume/boj28350_branch_3_solver.cpp",
        "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
    ],
    8: [
        "artifact_paths.py",
        "branch_run_case.py",
        "branch_gen_case.py",
        "branch_certify_suite.py",
        "lca_hunt.sh",
    ],
    9: [
        "lca_hunt.sh",
        "artifact_paths.py",
        "README.md",
    ],
}
AC_ARTIFACT_HINTS = {
    2: [
        ("smoke", "artifacts/lca_tree_stress_v5/smoke"),
        ("smoke_repeatability", "artifacts/lca_tree_stress_v5/smoke_repeatability"),
        ("tmp", "artifacts/lca_tree_stress_v5/.tmp"),
    ],
    3: [
        ("strong_gate", "artifacts/lca_tree_stress_v5/strong_gate"),
        ("tmp", "artifacts/lca_tree_stress_v5/.tmp"),
    ],
    4: [
        ("strong_gate", "artifacts/lca_tree_stress_v5/strong_gate"),
        ("tmp", "artifacts/lca_tree_stress_v5/.tmp"),
    ],
    5: [
        ("boj3s_gate", "artifacts/lca_tree_stress_v5/boj3s_gate"),
        ("tmp", "artifacts/lca_tree_stress_v5/.tmp"),
    ],
    6: [
        ("boj3s_gate", "artifacts/lca_tree_stress_v5/boj3s_gate"),
        ("tmp", "artifacts/lca_tree_stress_v5/.tmp"),
    ],
    8: [
        ("run_case", "artifacts/lca_tree_stress_v5/run_case"),
        ("gen_case_aux", "artifacts/lca_tree_stress_v5/gen_case_aux"),
    ],
    9: [
        ("hunt", "artifacts/lca_tree_stress_v5/hunt"),
    ],
}
FORMAL_ACCEPTANCE_ACS = {3, 4, 5, 6, 7}
DIAGNOSTIC_ONLY_ACS = {9}
FORMAL_ARTIFACT_LABELS = {"strong_gate", "boj3s_gate"}
DIAGNOSTIC_ARTIFACT_LABELS = {"hunt"}
LOCK_WRAPPER_RELATIVE_PATHS = {
    "lca_strong_gate.sh": "outer_suite_wrappers/lca_strong_gate.sh",
    "lca_boj3s_gate.sh": "outer_suite_wrappers/lca_boj3s_gate.sh",
}
CERTIFY_BUCKET_ORDER = {
    "timeout": 0,
    "re_wa": 1,
    "pass": 2,
}
AC_WRAPPER_HINTS = {
    3: "outer_suite_wrappers/lca_strong_gate.sh",
    4: "outer_suite_wrappers/lca_strong_gate.sh",
    5: "outer_suite_wrappers/lca_boj3s_gate.sh",
    6: "outer_suite_wrappers/lca_boj3s_gate.sh",
}
AC_WRAPPER_LOCK_MARKERS = {
    3: ("strong pid", ".locks/lca_strong_gate", "lca_strong_gate/pid", "artifacts/lca_tree_stress_v5/.locks", "failed to acquire strong gate lock"),
    4: ("strong pid", ".locks/lca_strong_gate", "lca_strong_gate/pid", "artifacts/lca_tree_stress_v5/.locks", "failed to acquire strong gate lock"),
    5: ("boj3s pid", ".locks/lca_boj3s_gate", "lca_boj3s_gate/pid", "artifacts/lca_tree_stress_v5/.locks", "failed to acquire boj3s gate lock"),
    6: ("boj3s pid", ".locks/lca_boj3s_gate", "lca_boj3s_gate/pid", "artifacts/lca_tree_stress_v5/.locks", "failed to acquire boj3s gate lock"),
}
AC_SOLVER_TRACE_HINTS = {
    3: (
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((9360, 9364),),
            "tokens": (
                "__dt_zero_elide",
                "__dt_layout_skip",
                "__dt_rule_dispatch",
                "__dt_tail_clear",
            ),
            "note": "fresh AC3 timeout rows now hit four-quadrant plateaus at `caterpillar_rect_dense n=1024`, so the first solver-side fallback is the exact included-body `__dt_zero_elide` budget split, not the stale thin-wrapper offsets or a broader zero-span corridor",
            "boost": 8,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((9367, 9370),),
            "tokens": (
                "time_cnorm_zero_span_elision_ns",
                "cnorm_zero_span_checks",
                "cnorm_zero_span_elision_hits",
            ),
            "note": "the next solver-side reread should stay on the first exact zero-span counter publication, not widen back to the enclosing helper corridor or the older layout-signature fallback split",
            "boost": 9,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((9394, 9396),),
            "tokens": (
                "time_lreuse_zero_span_scan_ns",
                "lreuse_zero_span_scan_calls",
                "lreuse_zero_span_segments_detected",
            ),
            "note": "the retained zero-span scan write is now the first live lreuse-side owner statement once the cnorm zero-span publication is confirmed",
            "boost": 10,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((9401, 9402),),
            "tokens": (
                "time_lreuse_noop_fastpath_commit_ns",
                "lreuse_noop_commit_hits",
            ),
            "note": "pin the fastpath-commit owner to the exact noop-commit metric write and hit counter because that statement most directly matches the current `zero_span_fastpath` axis name",
            "boost": 11,
        },
    ),
    4: (
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "ranges": ((9266, 9266),),
            "tokens": (
                "time_lgate_fastpath_commit_core_ns",
                "__acc_lgate(__dt_fast",
            ),
            "note": "narrowed to the exact fastpath metric-write statement inside `__lgate_opt`; the authoritative progress40 residual is named after this timer family, so start from the timer write itself before the companion increment lines",
            "boost": 10,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "ranges": ((9253, 9253),),
            "tokens": (
                "__dt_fast",
            ),
            "note": "keep the exact `__dt_fast` budget-definition line immediately behind the metric write so the regenerated breakdown names the defining budget source, not only the downstream counters",
            "boost": 9,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "ranges": ((9263, 9263),),
            "tokens": (
                "time_lgate_zero_span_eligibility_gate_ns",
                "__acc_lgate(__dt_zero_gate",
            ),
            "note": "keep the sibling zero-span eligibility metric-write statement as the first same-axis cross-check, but behind the fastpath metric write because the probe shape is a certify crawl rather than a quick-fail gate miss",
            "boost": 8,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "ranges": ((9252, 9252),),
            "tokens": (
                "__dt_zero_gate",
            ),
            "note": "keep the exact `__dt_zero_gate` budget-definition line immediately behind the eligibility metric write so the sibling branch is also pinned to one defining statement, not the wider child block",
            "boost": 7,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "ranges": ((9229, 9232),),
            "tokens": (
                "__dt_zero_scan",
                "__dt_zero_reuse",
                "__dt_skip_commit",
                "__dt_noop_commit",
            ),
            "note": "keep the zero-span reuse budget fan-out only as same-symbol fallback context after the four exact metric/budget lines above; it should no longer outrank them",
            "boost": 5,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "ranges": ((14039, 14061),),
            "tokens": (
                "StatePublishContext __publish_ctx",
                "connectorWatchEntryIds",
                "dispatchPublishRebuildCanonicalState",
            ),
            "note": "narrowed to the single connector publish handoff block that survives only as the first fallback if the zero-span gate/fastpath slice still looks innocent",
            "boost": 5,
        },
    ),
    5: (
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14544, 14549),),
            "tokens": (
                "canDeltaPreserved",
                "canConnectorSkeleton",
                "RepUnanimous",
                "deltaPreservedHitEnabled",
                "deltaConnectorHitEnabled",
                "forceSkeleton",
            ),
            "note": "narrowed to the included-body route predicate/toggle gate for the first still-unmet BOJ gate: start from the live `canDeltaPreserved` / `canConnectorSkeleton` decision pair and LOCAL toggles, not the stale wrapper slice 14034-14039",
            "boost": 12,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14553, 14557),),
            "tokens": (
                "connector-skeleton path already handles preserved-piece hits",
                "canDeltaPreserved = false",
                "watch compaction",
            ),
            "note": "keep the mutual-exclusion downgrade immediately behind the predicate pair because it is the first exact route discriminator inside `applyPieceNativeReuseForClass`",
            "boost": 11,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14573, 14588),),
            "tokens": (
                "applyPieceNativeReuseForClassBaseline",
                "REUSE_ROUTE_BASELINE",
                "ScopedWScanRouteContext",
                "reuse_route_baseline",
            ),
            "note": "narrowed to the exact included-body baseline return after the route predicates reject both preserved and connector-skeleton handling",
            "boost": 10,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14592, 14608),),
            "tokens": (
                "REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON",
                "applyPieceNativeReuseForClassBaseline",
                "applyConnectorSkeletonRebuildForClass",
                "reuse_route_delta_preserved_then_skeleton",
            ),
            "note": "narrowed to the included-body preserved-then-skeleton handoff because the live AC5 blocker still has to choose whether the preserved baseline update fires before the connector rebuild",
            "boost": 9,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14610, 14618),),
            "tokens": (
                "REUSE_ROUTE_CONNECTOR_SKELETON",
                "applyConnectorSkeletonRebuildForClass",
                "forceSkeleton",
                "reuse_route_connector_skeleton",
            ),
            "note": "narrowed to the direct included-body connector-skeleton handoff for the same AC5 route fork; keep it behind the predicate/toggle pair and mutual-exclusion downgrade",
            "boost": 8,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((9364, 9370),),
            "tokens": (
                "__dt_sig_load",
                "time_lreuse_layout_sig_load_ns",
                "__acc_lreuse",
            ),
            "note": "keep the exact signature-source load metric write as the primary-axis corroboration after the route fork above because the freshest same-worktree AC5 failure promoted `state_materialization`",
            "boost": 8,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((9365, 9372),),
            "tokens": (
                "__dt_sig_cmp",
                "time_lreuse_layout_sig_compare_ns",
                "__acc_lreuse",
            ),
            "note": "keep the exact layout signature compare metric write as the only allowed secondary-axis corroboration after the AC5 route-choice statements above",
            "boost": 7,
        },
    ),
    6: (
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14544, 14549),),
            "tokens": (
                "canDeltaPreserved",
                "canConnectorSkeleton",
                "RepUnanimous",
                "deltaPreservedHitEnabled",
                "deltaConnectorHitEnabled",
                "forceSkeleton",
            ),
            "note": "formal-closure AC6 still depends on the same included-body route predicate/toggle gate; do not let the stale wrapper slice 14034-14039 outrank the live `canDeltaPreserved` / `canConnectorSkeleton` pair",
            "boost": 11,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14553, 14557),),
            "tokens": (
                "connector-skeleton path already handles preserved-piece hits",
                "canDeltaPreserved = false",
                "watch compaction",
            ),
            "note": "keep the mutual-exclusion downgrade immediately behind the predicate pair because it is the first exact route discriminator for the same AC5/AC6 corridor",
            "boost": 10,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14573, 14588),),
            "tokens": (
                "REUSE_ROUTE_BASELINE",
                "applyPieceNativeReuseForClassBaseline",
                "ScopedWScanRouteContext",
                "reuse_route_baseline",
            ),
            "note": "narrowed to the exact included-body baseline return after the route predicates reject both preserved and connector-skeleton handling",
            "boost": 9,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14592, 14608),),
            "tokens": (
                "REUSE_ROUTE_DELTA_PRESERVED_THEN_SKELETON",
                "applyPieceNativeReuseForClassBaseline",
                "applyConnectorSkeletonRebuildForClass",
                "reuse_route_delta_preserved_then_skeleton",
            ),
            "note": "narrowed to the preserved-then-skeleton handoff that keeps the baseline update but still routes the unanimous class into connector-skeleton rebuild",
            "boost": 8,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((14610, 14618),),
            "tokens": (
                "REUSE_ROUTE_CONNECTOR_SKELETON",
                "applyConnectorSkeletonRebuildForClass",
                "forceSkeleton",
                "reuse_route_connector_skeleton",
            ),
            "note": "narrowed to the direct connector-skeleton handoff when the unanimous class skips the preserved path entirely",
            "boost": 8,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((9364, 9370),),
            "tokens": (
                "__dt_sig_load",
                "time_lreuse_layout_sig_load_ns",
                "__acc_lreuse",
            ),
            "note": "keep the exact signature-source load metric write as the primary-axis corroboration for the same AC5/AC6 corridor",
            "boost": 7,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "ranges": ((9365, 9372),),
            "tokens": (
                "__dt_sig_cmp",
                "time_lreuse_layout_sig_compare_ns",
                "__acc_lreuse",
            ),
            "note": "keep the exact layout signature compare metric write as the only allowed secondary-axis corroboration for the same AC5/AC6 corridor",
            "boost": 6,
        },
    ),
}
AC_RETRY_ANCHOR_HINTS = {
    2: (
        {
            "path": "artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/smoke_target_wrapper_syntax.stderr.txt",
            "range": (1, 1),
            "label": "Smoke-target shell_syntax stderr",
            "note": "the preserved shell-syntax stderr says `Operation canceled` for `lca_smoke_target.sh`, so the live blocker is wrapper readability/access at pre-dispatch time, not a parsed shell syntax defect inside the target wrapper body",
            "statement_excerpt": "/Users/.../lca_smoke_target.sh: ... Operation canceled",
            "priority": 410,
            "families": ("smoke_target_wrapper_readability_failure",),
            "allow_non_code": True,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv",
            "range": (33, 33),
            "label": "Smoke-target shell_syntax broken row",
            "note": "the launcher preflight localizes the failure to the `shell_syntax` row for `lca_smoke_target.sh`, so the next reread should stay on the shell-entrypoint validation chain rather than widening into smoke-manifest or solver paths",
            "statement_excerpt": "shell_syntax\tsmoke target wrapper syntax\tbroken\t.../lca_smoke_target.sh\t.../smoke_target_wrapper_syntax.stderr.txt",
            "priority": 405,
            "families": ("smoke_target_wrapper_readability_failure",),
            "allow_non_code": True,
        },
        {
            "path": "lca_smoke.sh",
            "range": (5296, 5300),
            "label": "shell_entrypoint_validation smoke-target call site",
            "note": "the newest same-worktree attempt dies in `shell_entrypoint_validation`, and the smoke-target wrapper is the last `check_shell_syntax(...)` call before manifest validation begins",
            "statement_excerpt": "set_launcher_failure_stage \"shell_entrypoint_validation\" ... check_shell_syntax \"$SMOKE_TARGET_WRAPPER\" \"smoke target wrapper syntax\"",
            "priority": 400,
            "families": ("smoke_target_wrapper_readability_failure",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (464, 469),
            "label": "check_shell_syntax broken handoff",
            "note": "this exact branch captures stderr, marks the last check as broken, and fails the launcher; combined with the preserved `Operation canceled` stderr, it narrows the blocker to smoke-target wrapper readability/access at the `bash -n` handoff",
            "statement_excerpt": "if \"$BASH_BIN\" -n \"$path\" >/dev/null 2>\"$stderr_path\"; then ... set_launcher_last_check \"shell_syntax\" \"$label\" \"broken\" \"$path\" \"$stderr_path\"; fail \"broken ${label}: $path\"",
            "priority": 395,
            "families": ("smoke_target_wrapper_readability_failure",),
        },
        {
            "path": "artifacts/lca_tree_stress_v5/smoke/failure_report.md",
            "range": (3, 14),
            "label": "166-second launcher failure timing",
            "note": "the newest same-worktree smoke run lasted 166 seconds before `dispatch_monitor` failed, which makes an immediate argument/launch parse miss unlikely and keeps the first surviving helper-side suspicion on the post-wait result-publication path",
            "statement_excerpt": "- Run id: `run.8FzpEa` ... - Run elapsed seconds: `166` ... - Summary: `inner wrapper dispatch monitor failed with exit code 1`",
            "priority": 400,
            "families": ("smoke_dispatch_monitor_helper_failure",),
        },
        {
            "path": "artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/preflight_manifest.tsv",
            "range": (33, 33),
            "label": "Dispatch monitor broken row",
            "note": "the launcher preflight reaches the dispatch monitor, records `broken 1`, and points at `dispatch_result.txt`, so the live reread boundary is the embedded helper exit-code handoff rather than a broader wrapper section",
            "statement_excerpt": "dispatch_monitor\tinner wrapper dispatch monitor\tbroken\t1\t.../dispatch_result.txt",
            "priority": 395,
            "families": ("smoke_dispatch_monitor_helper_failure",),
            "allow_non_code": True,
        },
        {
            "path": "lca_smoke.sh",
            "range": (4284, 4288),
            "label": "Dispatch-result write handoff",
            "note": "the outer shell only enters the `manager_rc=$?` else-branch when the embedded Python helper itself exits nonzero; combined with the 166-second smoke runtime and the now-missing `dispatch_result.txt`, the first surviving source-level hypothesis is the helper dying while writing its result payload after the child wait returns",
            "statement_excerpt": "result_path.write_text( f\"raw_exit_code={raw_exit_code}\\n\" f\"timed_out={1 if timed_out else 0}\\n\", encoding=\"utf-8\", )",
            "priority": 390,
            "families": ("smoke_dispatch_monitor_helper_failure",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (4292, 4301),
            "label": "Dispatch monitor helper-failure handoff",
            "note": "this exact non-negated else-branch preserves the embedded helper exit code as `manager_rc`; `manager_rc=1` here means the helper crashed, not merely that the inner wrapper returned exit code 1",
            "statement_excerpt": "else manager_rc=$?; set_launcher_failure_stage \"dispatch_monitor\"; ... fail \"inner wrapper dispatch monitor failed with exit code $manager_rc\"",
            "priority": 385,
            "families": ("smoke_dispatch_monitor_helper_failure",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (4304, 4313),
            "label": "Dispatch-result capture fallback",
            "note": "keep the missing-result guard only as the immediate fallback when a future same-worktree rerun leaves the helper exit code at zero but still fails to persist or parse `dispatch_result.txt`",
            "statement_excerpt": "if [[ ! -s \"$LAUNCHER_DISPATCH_RESULT_PATH\" ]]; then ... fail \"inner wrapper dispatch monitor did not record a dispatch result\"",
            "priority": 380,
            "families": ("smoke_dispatch_monitor_helper_failure",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (3610, 3610),
            "label": "Live smoke manifest path gate",
            "note": "current branch inspection shows `boj28350_resume/smoke_cases.tsv` is flagged `compressed,dataless` while the cached `artifacts/lca_tree_stress_v5/smoke/environment_validation/smoke_cases.snapshot.tsv` remains readable and structurally valid, so the first reread should start at the live manifest-path guard instead of a broad row-contract corridor",
            "statement_excerpt": "require_file \"$SMOKE_CASES_SOURCE\" \"smoke case manifest\"",
            "priority": 395,
            "families": ("smoke_manifest_contract_invalid",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (3632, 3633),
            "label": "Manifest-validation stage call site",
            "note": "the latest failure never reached inner-wrapper dispatch; `main` enters `smoke_manifest_validation` and immediately calls `check_smoke_manifest_selection` before any solver/runtime work begins",
            "statement_excerpt": "set_launcher_failure_stage \"smoke_manifest_validation\"; check_smoke_manifest_selection",
            "priority": 390,
            "families": ("smoke_manifest_contract_invalid",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (3319, 3347),
            "label": "Launcher smoke-manifest validator",
            "note": "the latest failure summary and preflight manifest both stop here: this embedded Python preflight enforces the 7-column smoke-manifest contract, but the current branch-local reread should treat a dataless live manifest as the first suspect before broadening into row-content edits",
            "statement_excerpt": "if len(row) != 7: raise SystemExit(...); ... if shuffle_labels not in {\"0\", \"1\"}: raise SystemExit(...); if timeout_value <= 0.0: raise SystemExit(...);",
            "priority": 385,
            "families": ("smoke_manifest_contract_invalid",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (3380, 3386),
            "label": "Manifest-invalid fail handoff",
            "note": "keep the exact last-check invalid handoff and launcher failure message ahead of older bundle-publication anchors when the newest AC2 failure says `invalid smoke case manifest`",
            "statement_excerpt": "set_launcher_last_check \"smoke_manifest\" \"smoke case manifest\" \"invalid\" \"$SMOKE_CASES_SOURCE\" \"$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR\"; fail \"invalid smoke case manifest: $SMOKE_CASES_SOURCE\"",
            "priority": 380,
            "families": ("smoke_manifest_contract_invalid",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (3277, 3284),
            "label": "Preflight stderr/selection copy",
            "note": "the latest failure summary only preserved a tmp-path `last_check_artifact`, and that launcher tmp stderr path was already gone during post-failure analysis, so keep the explicit preflight-artifact sync as the capture-boundary corroboration",
            "statement_excerpt": "copy_launcher_preflight_artifact \"$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION\" \"$LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION\"; copy_launcher_preflight_artifact \"$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR\" \"$LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR\"",
            "priority": 375,
            "families": ("smoke_manifest_contract_invalid",),
        },
        {
            "path": "outer_suite_wrappers/lca_smoke.sh",
            "range": (497, 538),
            "label": "Shared smoke-manifest row contract",
            "note": "the launcher preflight and the outer wrapper both enforce the same manifest row contract, so the next solver session should compare the manifest file directly against this narrower row validator instead of re-reading broader smoke setup code",
            "statement_excerpt": "if [[ -z \"$stage\" || -z \"$mode\" ]]; then fail ...; case \"$n\" in ''|*[!0-9]*) fail ...;; esac; case \"$shuffle_labels\" in 0|1) ;; *) fail ...;; esac",
            "priority": 370,
            "families": ("smoke_manifest_contract_invalid",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (2331, 2348),
            "label": "Failure-bundle freshness validator",
            "note": "the live smoke failure text quotes missing preserved failure root / failure summary / failure report, so the first reread is the exact validator that emits that bundle-publication message, not the success-bundle validator",
            "statement_excerpt": "if [[ ! -d \"$failure_root\" ]]; then issues=\"missing preserved failure root at $failure_root\" ... printf '%s\\n' \"inner smoke wrapper returned a ${failure_class} result without publishing a complete fresh failure bundle: $issues\"",
            "priority": 370,
            "families": ("smoke_bundle_publication_gap",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (2400, 2414),
            "label": "Solver-side classification handoff",
            "note": "raw exit code 124 enters the solver-side branch, so this exact `validate_inner_wrapper_failure_bundle(...)` -> `set_launcher_status(...)` -> `capture_launcher_source_failure_details(...)` handoff now outranks the older success-artifact path",
            "statement_excerpt": "if ! validation_message=\"$(validate_inner_wrapper_failure_bundle \"solver-side\" \"$SMOKE_FAILURE_ROOT\" \"$source_summary\" \"$source_report\")\"; then ... capture_launcher_source_failure_details \"$source_summary\"",
            "priority": 365,
            "families": ("smoke_bundle_publication_gap",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (706, 710),
            "label": "Run-archive snapshot copy guards",
            "note": "the run archive manifest copied only status-side artifacts and omitted both snapshot rows, so keep the exact source-root / failure-root copy conditions ahead of the earlier status copies and any later manifest or stage-label fallback helpers",
            "statement_excerpt": "if [[ \"${LAUNCHER_STATUS_OUTCOME:-}\" != \"pass\" && -n \"$LAUNCHER_STATUS_SOURCE_ROOT\" ]]; then copy_launcher_run_path ...; fi; if [[ -n \"$LAUNCHER_FAILURE_ROOT\" && \"$LAUNCHER_FAILURE_ROOT\" != \"${LAUNCHER_STATUS_SOURCE_ROOT:-}\" ]]; then copy_launcher_run_path ...; fi",
            "priority": 360,
            "families": ("smoke_bundle_publication_gap",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (799, 802),
            "label": "Source-summary ingress short-circuit",
            "note": "when the preserved source summary never materializes this helper clears prior replay metadata and immediately returns, which explains why the launcher-side source-failure fields stay empty without widening back to the zero-exit success validator",
            "statement_excerpt": "clear_launcher_source_failure_details; if [[ -z \"$source_summary\" || ! -f \"$source_summary\" ]]; then return 0; fi",
            "priority": 355,
            "families": ("smoke_bundle_publication_gap",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (1334, 1337),
            "label": "Published missing-bundle diagnostics rows",
            "note": "the diagnostics manifest proves the failure-root/source-root/source-summary/source-report tuple never existed, so keep those exact published rows as artifact-side corroboration after the source-provenance chain",
            "statement_excerpt": "append_launcher_status_diagnostic_entry \"smoke_failure_root\" \"$SMOKE_FAILURE_ROOT\" ...; append_launcher_status_diagnostic_entry \"source_root\" \"$LAUNCHER_STATUS_SOURCE_ROOT\" ...; append_launcher_status_diagnostic_entry \"source_summary\" \"$LAUNCHER_STATUS_SOURCE_SUMMARY\" ...; append_launcher_status_diagnostic_entry \"source_report\" \"$LAUNCHER_STATUS_SOURCE_REPORT\" ...",
            "priority": 350,
            "families": ("smoke_bundle_publication_gap",),
        },
        {
            "path": "lca_smoke.sh",
            "range": (1083, 1086),
            "label": "Iteration-evidence source-path handoff",
            "note": "the stage-label fallback remains downstream corroboration only; keep the exact source_root/source_summary/source_report publication lines so regenerated notes still name the missing bundle tuple without widening back to whole wrapper sections",
            "statement_excerpt": "echo \"source_root=$LAUNCHER_STATUS_SOURCE_ROOT\"; echo \"source_summary=$LAUNCHER_STATUS_SOURCE_SUMMARY\"; echo \"source_report=$LAUNCHER_STATUS_SOURCE_REPORT\"",
            "priority": 345,
            "families": ("smoke_bundle_publication_gap",),
        },
    ),
    3: (
        {
            "path": "outer_suite_wrappers/lca_strong_gate.sh",
            "range": (743, 747),
            "label": "Certify helper launch handoff",
            "note": "the newest same-worktree smoke report already says AC3 is ready to run, but the failed attempt still produced no fresh strong-gate artifact; stay on the wrapper-to-helper launch handoff before reviving any carried-forward timeout-cluster anchors",
            "priority": 330,
            "families": ("strong_gate_pre_artifact_stall",),
        },
        {
            "path": "outer_suite_wrappers/lca_strong_gate.sh",
            "range": (757, 758),
            "label": "Heartbeat completed-case sample",
            "note": "use the completed-case heartbeat only as fallback corroboration for this stale-artifact shape; it helps separate launched-but-unpublished runs from pure launch misses once the freshness boundary is pinned",
            "priority": 325,
            "families": ("strong_gate_pre_artifact_stall",),
        },
        {
            "path": "outer_suite_wrappers/lca_strong_gate.sh",
            "range": (681, 701),
            "label": "Published-plus-active time scan",
            "note": "the first wrapper-side progress witness for this stale-artifact AC3 shape is the published-plus-active completed-case scan, not the older solver-timeout publication anchors",
            "priority": 320,
            "families": ("strong_gate_pre_artifact_stall",),
        },
        {
            "path": "branch_certify_suite.py",
            "range": (540, 549),
            "label": "branch_run_solver_with_time handoff",
            "note": "fresh attempt-local strong-gate failure rows already collapse the live blocker to solver timeouts, so the first helper-side ingress is the actual timed solver call in `run_one_case`, not the downstream `_write_case_result` helper signature",
            "statement_excerpt": "rc_sol, to_sol, sec, rss = branch_run_solver_with_time( solver, in_path, out_path, time_path, solver_stderr, timeout, env=solver_env, cwd=work_dir, )",
            "priority": 320,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
        {
            "path": "branch_certify_suite.py",
            "range": (557, 568),
            "label": "solver_timeout publication",
            "note": "this exact timeout branch is the first persisted timeout payload after the live solver call, so it now outranks the broader `_write_case_result` helper header and older wrapper-only anchors",
            "statement_excerpt": "_write_case_result( work_dir, status=\"solver_timeout\", category=\"solver\", exit_code=124, message=\"solver timed out\", solver_exit_code=rc_sol, timed_out=True, validator_ok=False, sec=sec, rss_kb=rss, )",
            "priority": 319,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
        {
            "path": "branch_certify_suite.py",
            "range": (613, 614),
            "label": "outer_certify timeout row summary",
            "note": "this exact returned row is the certify-side boundary that becomes the stage/mode/n/LQ timeout cluster in the live strong-gate failure tree, so keep it ahead of stale wrapper bookkeeping",
            "statement_excerpt": "return outer_certify.Row(stage_name, mode, n, seed, shuffle_labels, shuffle_queries, 1, rc_sol, 1 if to_sol else 0, val_ok, sec, rss, str(reported_case_dir))",
            "priority": 318,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
        {
            "path": "outer_suite_wrappers/lca_strong_gate.sh",
            "range": (484, 489),
            "label": "Certify helper launch",
            "note": "the wrapper/helper handoff still matters as trust-boundary corroboration, but it now sits behind the helper-side launch and timeout publication statements because attempt_017 already produced fresh in-attempt strong-gate artifacts",
            "priority": 317,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
        {
            "path": "outer_suite_wrappers/lca_strong_gate.sh",
            "range": (498, 499),
            "label": "Heartbeat completed-case sample",
            "note": "keep the exact `completed_cases=...` heartbeat echo only as progress corroboration once the fresh timeout rows already prove this was not a zero-progress launch miss",
            "priority": 316,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
        {
            "path": "outer_suite_wrappers/lca_strong_gate.sh",
            "range": (430, 442),
            "label": "Published-plus-active time scan",
            "note": "keep the published/active `time.txt` counter only as fallback progress context once the helper-side timeout row and heartbeat evidence already prove partial publication",
            "priority": 260,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (9360, 9364),
            "label": "__dt_zero_elide budget split",
            "note": "current progress40 fallback owner once the helper-side timeout corridor is confirmed: the smallest full-`L/Q` timeout plateaus are no longer label/query-specific, so fall back to the exact included-body zero-span budget split before any other solver reread",
            "statement_excerpt": "long long __dt_zero_elide = std::max(1LL, __dt_layout_skip); long long __dt_rule_dispatch = std::max(1LL, __dt_norm / 5); long long __dt_field_apply = std::max(1LL, __dt_norm / 3); long long __dt_zero_fill = std::max(1LL, __dt_norm / 3); long long __dt_tail_clear = std::max(1LL, __dt_norm - __dt_rule_dispatch - __dt_field_apply - __dt_zero_fill);",
            "priority": 230,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (9367, 9370),
            "label": "time_cnorm_zero_span_elision_ns write",
            "note": "keep the first exact zero-span counter publication immediately behind the budget split so the next retry inherits the live cnorm write site instead of a larger solver corridor",
            "statement_excerpt": "if (__dt_zero_elide > 0) { __acc_cnorm(__dt_zero_elide, &g_batch_dbg.time_cnorm_zero_span_elision_ns, &g_batch_dbg.time_cnorm_zero_span_elision_calls); g_batch_dbg.cnorm_zero_span_checks++; g_batch_dbg.cnorm_zero_span_elision_hits++; }",
            "priority": 229,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (9394, 9396),
            "label": "time_lreuse_zero_span_scan_ns write",
            "note": "keep the exact zero-span scan metric write as the first retained lreuse-side owner statement once the cnorm zero-span publication is pinned",
            "statement_excerpt": "__acc_lreuse(__dt_zero_scan, &g_batch_dbg.time_lreuse_zero_span_scan_ns, &g_batch_dbg.time_lreuse_zero_span_scan_calls); g_batch_dbg.lreuse_zero_span_scan_calls++; g_batch_dbg.lreuse_zero_span_segments_detected++;",
            "priority": 228,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (9401, 9402),
            "label": "time_lreuse_noop_fastpath_commit_ns write",
            "note": "pin the fastpath-commit owner to the exact noop-commit metric write and hit counter so the regenerated breakdown names the direct progress40 axis owner instead of the wider lreuse block",
            "statement_excerpt": "__acc_lreuse(__dt_noop_commit, &g_batch_dbg.time_lreuse_noop_fastpath_commit_ns, &g_batch_dbg.time_lreuse_noop_fastpath_commit_calls); g_batch_dbg.lreuse_noop_commit_hits++;",
            "priority": 227,
            "families": (
                "strong_gate_timeout_cluster",
                "strong_gate_timeout_re_wa_cluster",
                "strong_gate_re_wa_cluster",
            ),
        },
    ),
    4: (
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "range": (9266, 9266),
            "label": "Fastpath commit metric write",
            "note": "repeat-closure AC4 still inherits the fresh strong-gate correctness_fuzz cluster, but the next regenerated breakdown should start from the exact `time_lgate_fastpath_commit_core_ns` write line rather than the older fused counter block or its companion increment lines",
            "priority": 330,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "range": (9253, 9253),
            "label": "__dt_fast budget definition",
            "note": "keep the exact `__dt_fast` definition immediately behind the fastpath metric write so the next reread names the budget source instead of widening back to the whole `__lgate_opt` body",
            "priority": 325,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "range": (9263, 9263),
            "label": "Zero-span eligibility metric write",
            "note": "keep the sibling `time_lgate_zero_span_eligibility_gate_ns` write as the first same-axis cross-check behind the fastpath metric line",
            "priority": 320,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "range": (9252, 9252),
            "label": "__dt_zero_gate budget definition",
            "note": "keep the exact `__dt_zero_gate` definition immediately behind the sibling eligibility metric write instead of a wider mixed budget span",
            "priority": 318,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "range": (9229, 9232),
            "label": "Zero-span reuse budget fan-out",
            "note": "keep only the immediate zero-span feeder block that turns `__dt_zero_elide` into `__dt_zero_scan`, `__dt_zero_reuse`, `__dt_skip_commit`, and `__dt_noop_commit` after the four exact metric/budget lines have been checked",
            "priority": 315,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "range": (14039, 14061),
            "label": "Connector publish handoff",
            "note": "keep the single connector publish handoff block as the first live fallback once the exact zero-span gate/fastpath slice and its feeder block have been reread",
            "priority": 310,
        },
    ),
    5: (
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14544, 14549),
            "label": "Route predicate/toggle gate",
            "note": "first AC5 narrowing point: latest_failure_breakdown revision 146 still points at stale wrapper lines 14034-14039, but the live `canDeltaPreserved` / `canConnectorSkeleton` predicate pair and LOCAL toggles now sit in the included solver body; start there",
            "priority": 360,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14553, 14557),
            "label": "Mutual-exclusion downgrade",
            "note": "second AC5 narrowing point: after the predicate/toggle pair, this exact included-body downgrade disables the preserved branch when both routes look eligible; keep it ahead of the broader route exits",
            "priority": 357,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14573, 14588),
            "label": "Baseline reuse return",
            "note": "exact baseline exit from the same included-body route triage when both preserved and connector-skeleton handling are unavailable; this replaces the stale wrapper slice 14062-14077",
            "priority": 354,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14592, 14608),
            "label": "Preserved-then-skeleton handoff",
            "note": "exact included-body handoff that keeps the preserved baseline update and then forces connector-skeleton rebuild; this is the first downstream discriminator after the predicate/toggle pair",
            "priority": 351,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14610, 14618),
            "label": "Direct connector-skeleton handoff",
            "note": "exact direct route into `applyConnectorSkeletonRebuildForClass` when the preserved branch is skipped entirely; keep it in the included solver body rather than the stale wrapper window",
            "priority": 345,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (9364, 9370),
            "label": "Signature-source load metric",
            "note": "primary-axis corroboration after the route-choice fork: the fresh AC5 breakdown elevated `state_materialization`, so keep the exact `__dt_sig_load` plus `time_lreuse_layout_sig_load_ns` write ahead of older zero-span carry-forward ranges",
            "priority": 340,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (9365, 9372),
            "label": "Layout signature compare metric",
            "note": "secondary-axis corroboration after the route fork: keep the exact `__dt_sig_cmp` plus `time_lreuse_layout_sig_compare_ns` write so the next retry stays on the live `layout_gate` evidence instead of the stale wrapper-side zero-span corridor",
            "priority": 335,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "range": (1, 6),
            "label": "Thin include wrapper",
            "note": "stale-range guard only: this file is now just the include bridge into the live solver body, so do not let wrapper offsets 14034-14107 or 9204-9214 outrank the included-body anchors above",
            "priority": 250,
        },
    ),
    6: (
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14544, 14549),
            "label": "Route predicate/toggle gate",
            "note": "formal-closure AC6 still depends on the same included-body route eligibility and LOCAL toggle gate; do not let the stale wrapper slice 14034-14039 outrank the live predicate pair",
            "priority": 360,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14553, 14557),
            "label": "Mutual-exclusion downgrade",
            "note": "formal-closure follow-up should keep the same exact included-body downgrade in view because it is the first route discriminator after the predicate pair",
            "priority": 357,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14573, 14588),
            "label": "Baseline reuse return",
            "note": "formal-closure fallback branch inside the same included-body route triage; keep this ahead of broader route-body rereads and stale wrapper windows",
            "priority": 354,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14592, 14608),
            "label": "Preserved-then-skeleton handoff",
            "note": "exact included-body handoff that keeps the baseline preserved update and then forces connector-skeleton rebuild for the unanimous class",
            "priority": 351,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (14610, 14618),
            "label": "Direct connector-skeleton handoff",
            "note": "exact direct route into `applyConnectorSkeletonRebuildForClass` when the preserved branch is skipped entirely; AC6 closure should keep this live path pinned even when the wrapper markdown is stale",
            "priority": 345,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (9364, 9370),
            "label": "Signature-source load metric",
            "note": "primary-axis corroboration for the same AC5/AC6 corridor: keep the exact `__dt_sig_load` plus `time_lreuse_layout_sig_load_ns` write ahead of older zero-span carry-forward ranges",
            "priority": 340,
        },
        {
            "path": "artifacts/lca_tree_stress_v5/retry_loop/ac3_active_solver_backup_before_restore_20260328.cpp",
            "range": (9365, 9372),
            "label": "Layout signature compare metric",
            "note": "secondary-axis corroboration for the same corridor: keep the exact `__dt_sig_cmp` plus `time_lreuse_layout_sig_compare_ns` write so AC6 follow-up stays aligned with the live layout-gate evidence",
            "priority": 335,
        },
        {
            "path": "boj28350_resume/boj28350_branch_3_solver.cpp",
            "range": (1, 6),
            "label": "Thin include wrapper",
            "note": "stale-range guard only: AC6 follow-up should confirm this file is the include bridge, then jump back into the included-body route anchors above",
            "priority": 250,
        },
        {
            "path": "outer_suite_wrappers/lca_boj3s_gate.sh",
            "range": (182, 193),
            "label": "Active-holder fail branch",
            "note": "trust-boundary corroboration only for this stale-direct-evidence shape; once the attempt body already names the live comb timeout, do not let this wrapper branch outrank the exact solver anchors above",
            "priority": 220,
        },
        {
            "path": "solver_release_env.sh",
            "range": (134, 136),
            "label": "Fastpath/materialization default pair",
            "note": "axis corroboration only; the live blocker already survives with `ENABLE_STATE_LOAD_MATERIALIZATION_OPT=1` and `0`, so keep this below the exact solver-side comb corridor",
            "priority": 210,
        },
    ),
}
WORKFLOW_PREFLIGHT_TRACE_MARKERS = (
    "monitor_codex_quota.py",
    "soft_stop_request.json",
    "artifact_paths.py",
    "output path must stay under",
    "Invalid seed format",
    "seed_branch3_progress40_research_loop.yaml",
    "attempt guard passed",
)
WORKFLOW_PREFLIGHT_HINTS = (
    {
        "path": ".ouroboros/run_until_pass_progress40.sh",
        "range": (7, 8),
        "label": "Retry-loop seed defaults",
        "note": "declares the solver and analysis seed inputs; if the YAML still parses as a mapping locally, treat the later Invalid seed format line as a workflow handoff problem instead of a seed-syntax failure",
        "tokens": (
            "attempt 3 start",
            "seed_branch3_progress40_research_loop.yaml",
        ),
        "priority": 120,
    },
    {
        "path": ".ouroboros/run_until_pass_progress40.sh",
        "range": (13, 13),
        "label": "Retry-loop soft-stop declaration",
        "note": "the shell contract still binds soft_stop_file to $report_root/soft_stop_request.json, so the stale .ouroboros path is not coming from the in-tree retry-loop default",
        "tokens": (
            "soft_stop_request.json",
            "seed_branch3_progress40_research_loop.yaml",
        ),
        "priority": 180,
    },
    {
        "path": ".ouroboros/run_until_pass_progress40.sh",
        "range": (84, 84),
        "label": "Workflow seed launch argv",
        "note": "the exact ouroboros workflow launch line; keep the Invalid seed format diagnosis pinned here instead of the broader function band when the YAML still parses as a mapping locally",
        "tokens": (
            "Invalid seed format",
            "seed_branch3_progress40_research_loop.yaml",
            "ouroboros run workflow",
        ),
        "priority": 220,
    },
    {
        "path": ".ouroboros/run_until_pass_progress40.sh",
        "range": (98, 98),
        "label": "Quota watchdog --soft-stop-file argv",
        "note": "the retry-loop passes the soft-stop path through this single argv line; if the traceback still shows .ouroboros/soft_stop_request.json, the stale value entered before or during this handoff",
        "tokens": (
            "soft_stop_request.json",
            "monitor_codex_quota.py",
        ),
        "priority": 250,
    },
    {
        "path": ".ouroboros/monitor_codex_quota.py",
        "range": (21, 22),
        "label": "Quota watchdog default soft-stop literal",
        "note": "the watchdog parser still defaults --soft-stop-file under artifacts/lca_tree_stress_v5/retry_loop/, so the .ouroboros path is narrower than a generic default-path drift",
        "tokens": (
            "soft_stop_request.json",
            "monitor_codex_quota.py",
        ),
        "priority": 225,
    },
    {
        "path": ".ouroboros/monitor_codex_quota.py",
        "range": (55, 55),
        "label": "Quota watchdog artifact guard call",
        "note": "this single return line is the Python handoff from runtime argv to ensure_under_artifacts, so it is the narrowest soft-stop provenance checkpoint before the guard trips",
        "tokens": (
            "monitor_codex_quota.py",
            "soft_stop_request.json",
            "resolve_artifact_path",
        ),
        "priority": 240,
    },
    {
        "path": ".ouroboros/monitor_codex_quota.py",
        "range": (428, 428),
        "label": "Quota watchdog failing soft-stop resolution",
        "note": "this exact assignment is the failing traceback frame; keep it ahead of the broader main preflight corridor",
        "tokens": (
            "monitor_codex_quota.py",
            "soft_stop_request.json",
            "output path must stay under",
        ),
        "priority": 300,
    },
    {
        "path": ".ouroboros/request_soft_stop.py",
        "range": (17, 18),
        "label": "Manual soft-stop default literal",
        "note": "the manual helper still defaults under retry artifacts, which corroborates that .ouroboros/soft_stop_request.json is stale runtime provenance rather than a shared helper change",
        "tokens": (
            "soft_stop_request.json",
            "output path must stay under",
        ),
        "priority": 230,
    },
    {
        "path": "test_retry_loop_artifact_locality.py",
        "range": (151, 165),
        "label": "Soft-stop locality regression contract",
        "note": "branch-local regression coverage asserts that the retry loop, manual soft-stop helper, and quota watchdog all keep soft-stop requests under retry artifacts",
        "tokens": (
            "soft_stop_request.json",
            "output path must stay under",
        ),
        "priority": 235,
    },
    {
        "path": "artifact_paths.py",
        "range": (235, 235),
        "label": "Artifact locality ValueError raise",
        "note": "the exact raise statement that converts the stale .ouroboros path into the retry-preflight failure signature; use this line, not the whole helper, as the guard anchor",
        "tokens": (
            "artifact_paths.py",
            "output path must stay under",
            "soft_stop_request.json",
        ),
        "priority": 280,
    },
)
WORKFLOW_PREFLIGHT_PROVENANCE_EXPECTATIONS = (
    (
        ".ouroboros/run_until_pass_progress40.sh",
        'soft_stop_file="$report_root/soft_stop_request.json"',
    ),
    (
        ".ouroboros/monitor_codex_quota.py",
        'default="artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json"',
    ),
    (
        ".ouroboros/request_soft_stop.py",
        'default="artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json"',
    ),
    (
        "test_retry_loop_artifact_locality.py",
        'soft_stop_file="$report_root/soft_stop_request.json"',
    ),
    (
        "test_retry_loop_artifact_locality.py",
        'default="artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json"',
    ),
)


@dataclass
class ArtifactSnapshot:
    label: str
    latest_file: str | None
    latest_mtime: str | None
    summary_file: str | None
    summary_excerpt: str | None
    attempt_start: str | None
    fresh_for_attempt: bool | None
    freshness_note: str | None


@dataclass
class PhaseSummary:
    phase: str
    count: int
    sample: str


@dataclass
class StructuralFocus:
    path: str
    observed_mentions: int
    focus_ranges: list[str]
    enclosing_symbols: list[str]
    evidence_lines: list[str]
    code_excerpt: str | None
    note: str
    mtime: str | None


@dataclass
class RetryCriticalAnchor:
    label: str
    path: str
    focus_range: str
    symbol: str | None
    evidence_lines: list[str]
    code_excerpt: str | None
    statement_excerpt: str | None
    note: str


@dataclass
class Progress40AxisSummary:
    primary_axis: str | None
    secondary_axis: str | None
    axis_evidence: dict[str, list[str]]
    profile_mode: str | None
    enabled_flags: list[str]
    last_release_diag_phase: str | None
    last_progress_checkpoint_phase: str | None
    current_summary_pivot: str | None
    current_summary_residual_axes: list[str]
    failure_family: str
    next_probe_command: str
    interpretation_lane: str


@dataclass
class FormalAcceptanceSummary:
    verdict: str
    required_ac_indices: list[int]
    passed: list[tuple[str, str]]
    failed: list[tuple[str, str]]
    blocked: list[tuple[str, str]]
    missing: list[int]
    excluded_diagnostic: list[tuple[str, str]]


@dataclass
class ProbeSignal:
    command: str | None
    primary_axis: str | None
    secondary_axis: str | None
    why_this_axis: str | None
    exit_code: int | None
    timed_out: bool | None
    elapsed_seconds: float | None
    stdout_log: str | None
    stderr_log: str | None
    stderr_excerpt: list[str]
    active_gate: str | None
    active_pid: str | None
    wrapper_path: str | None
    focus_range: tuple[int, int] | None
    focus_symbol: str | None


def strip_ansi(text: str) -> str:
    return ANSI_RE.sub("", text)


def normalize_path_token(token: str) -> str:
    path = token.strip("`'\"()[],:")
    if path.startswith("./"):
        path = path[2:]
    if path.endswith("..."):
        return ""
    return path


def normalize_repo_relative_path(path_text: str) -> str:
    path = normalize_path_token(path_text)
    if not path:
        return ""
    candidate = Path(path)
    if candidate.is_absolute():
        try:
            resolved = candidate.resolve()
        except OSError:
            resolved = candidate
        try:
            return resolved.relative_to(REPO_ROOT).as_posix()
        except ValueError:
            return resolved.as_posix()
    if path.startswith("branch_3/"):
        path = path.split("/", 1)[1]
    return path


def parse_anchor_range_text(value: object) -> tuple[str, tuple[int, int]] | None:
    if not isinstance(value, str):
        return None
    match = ANCHOR_RANGE_RE.match(value.strip())
    if not match:
        return None
    path = normalize_repo_relative_path(match.group("path"))
    if not path:
        return None
    start = int(match.group("start"))
    end = int(match.group("end"))
    return path, (start, end)


def parse_focus_range_text(value: object) -> tuple[int, int] | None:
    if not isinstance(value, str):
        return None
    match = FOCUS_RANGE_RE.match(value.strip())
    if not match:
        return None
    start = int(match.group("start"))
    end = int(match.group("end"))
    return start, end


def markdown_section_lines(text: str, heading: str) -> list[str]:
    lines = text.splitlines()
    start_idx = None
    for idx, line in enumerate(lines):
        if line.strip() == heading:
            start_idx = idx + 1
            break
    if start_idx is None:
        return []
    section: list[str] = []
    for line in lines[start_idx:]:
        if line.startswith("## "):
            break
        section.append(line.rstrip())
    return section


def strip_markdown_inline_code(text: str) -> str:
    stripped = text.strip()
    if stripped.startswith("`") and stripped.endswith("`") and len(stripped) >= 2:
        stripped = stripped[1:-1].strip()
    return stripped.replace("`", "").strip()


def parse_iteration_failure_point_anchor(
    anchor_text: str,
) -> tuple[str, tuple[int, int], str] | None:
    match = ITERATION_FAILURE_POINT_ANCHOR_RE.match(anchor_text.strip())
    if not match:
        return None
    path = normalize_repo_relative_path(match.group("path"))
    if not path:
        return None
    start = int(match.group("start"))
    end = int(match.group("end"))
    label = strip_markdown_inline_code(match.group("label") or "") or f"{path}:{start}-{end}"
    return path, (start, end), label


def iteration_failure_point_specs(iteration_path: Path) -> list[dict[str, object]]:
    if not iteration_path.exists():
        return []
    try:
        text = iteration_path.read_text(errors="replace")
    except OSError:
        return []

    section_lines = markdown_section_lines(text, "## Latest Retry Failure Points")
    if not section_lines:
        return []

    entries: list[dict[str, str]] = []
    current: dict[str, str] | None = None
    for raw_line in section_lines:
        header_match = ITERATION_FAILURE_POINT_HEADER_RE.match(raw_line.strip())
        if header_match:
            if current is not None:
                entries.append(current)
            current = {"anchor": header_match.group("anchor").strip()}
            continue
        if current is None:
            continue
        detail_match = ITERATION_FAILURE_POINT_DETAIL_RE.match(raw_line)
        if detail_match:
            current[detail_match.group("label").lower()] = strip_markdown_inline_code(
                detail_match.group("body")
            )
    if current is not None:
        entries.append(current)

    specs: list[dict[str, object]] = []
    for idx, entry in enumerate(entries):
        parsed = parse_iteration_failure_point_anchor(entry.get("anchor", ""))
        if parsed is None:
            continue
        path, focus_range, label = parsed
        note = "promoted from failure_analysis_iteration Latest Retry Failure Points"
        role = entry.get("role")
        if role:
            note += f"; role: {role}"
        evidence_lines = []
        evidence = entry.get("evidence")
        if evidence:
            evidence_lines.append(f"failure_analysis_iteration.md → {evidence}")
        specs.append(
            {
                "path": path,
                "range": focus_range,
                "label": label,
                "note": note,
                "priority": max(300, 390 - idx * 5),
                "statement_excerpt": entry.get("statement") or None,
                "evidence_lines": evidence_lines,
            }
        )
    return specs


def stable_timestamp() -> str:
    return datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")


def safe_mtime(path: Path) -> float | None:
    try:
        return path.stat().st_mtime
    except OSError:
        return None


def safe_mtime_label(path: Path) -> str | None:
    mtime = safe_mtime(path)
    if mtime is None:
        return None
    return datetime.fromtimestamp(mtime).astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")


def attempt_started_at_for_dir(path: Path) -> datetime | None:
    match = ATTEMPT_DIR_RE.match(path.name)
    if not match:
        return None
    try:
        started_at = datetime.strptime(
            f"{match.group('date')}_{match.group('time')}",
            "%Y%m%d_%H%M%S",
        )
    except ValueError:
        return None
    tzinfo = datetime.now().astimezone().tzinfo
    if tzinfo is None:
        return None
    return started_at.replace(tzinfo=tzinfo)


def attempt_freshness_for_path(
    path: Path | None,
    attempt_started_at: datetime | None,
) -> tuple[str | None, bool | None, str | None]:
    attempt_start_label = (
        attempt_started_at.strftime("%Y-%m-%d %H:%M:%S %Z")
        if attempt_started_at is not None
        else None
    )
    if path is None or attempt_started_at is None:
        return attempt_start_label, None, None
    mtime = safe_mtime(path)
    if mtime is None:
        return attempt_start_label, None, None
    if mtime >= attempt_started_at.timestamp():
        return (
            attempt_start_label,
            True,
            "latest summary/file was refreshed during this failed attempt",
        )
    return (
        attempt_start_label,
        False,
        "latest summary/file predates the failed attempt start; treat it as carried-forward evidence, not fresh gate output",
    )


def latest_files(root: Path, count: int = 3) -> list[Path]:
    if not root.exists():
        return []
    files_with_mtime: list[tuple[float, Path]] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        mtime = safe_mtime(path)
        if mtime is None:
            continue
        files_with_mtime.append((mtime, path))
    files_with_mtime.sort(key=lambda item: item[0])
    return [path for _, path in files_with_mtime[-count:]]


def latest_summary_file(root: Path) -> Path | None:
    if not root.exists():
        return None
    preferred = (
        "certify_summary.md",
        "certify.json",
        "summary.txt",
        "hunt_summary.md",
        "failure_report.md",
        "failure_breakdown.md",
    )
    candidates: list[Path] = []
    for name in preferred:
        candidates.extend(root.rglob(name))
    candidates = [p for p in candidates if p.is_file() and safe_mtime(p) is not None]
    if not candidates:
        return None
    candidates.sort(key=lambda p: safe_mtime(p) or 0.0)
    return candidates[-1]


def read_excerpt(path: Path | None, max_lines: int = 120) -> str | None:
    if path is None or not path.exists():
        return None
    try:
        lines = path.read_text(errors="replace").splitlines()
    except OSError:
        return None
    return "\n".join(lines[:max_lines])


def parse_optional_int(text: str | None) -> int | None:
    if text is None:
        return None
    try:
        return int(text)
    except ValueError:
        return None


def parse_optional_float(text: str | None) -> float | None:
    if text is None:
        return None
    try:
        return float(text)
    except ValueError:
        return None


def parse_optional_bool(text: str | None) -> bool | None:
    if text is None:
        return None
    lowered = text.lower()
    if lowered == "yes":
        return True
    if lowered == "no":
        return False
    return None


def normalize_focus_range_list(values: Iterable[tuple[int, int]]) -> list[tuple[int, int]]:
    deduped = {
        (max(1, int(start)), max(int(start), int(end)))
        for start, end in values
    }
    return sorted(deduped, key=lambda item: (item[1] - item[0], item[0], item[1]))[:6]


def run_git_status(branch_root: Path) -> str:
    try:
        result = subprocess.run(
            ["git", "-c", "status.showUntrackedFiles=no", "status", "--short", "--untracked-files=no"],
            cwd=branch_root,
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )
    except subprocess.TimeoutExpired:
        return "git status skipped: timed out after 10s"
    except OSError as exc:
        return f"git status unavailable: {exc}"
    return result.stdout.strip() or "(clean)"


def filter_log_for_session(log_path: Path, session_id: str | None, limit: int = 120) -> list[str]:
    if not session_id or not log_path.exists():
        return []
    try:
        lines = log_path.read_text(errors="replace").splitlines()
    except OSError:
        return []
    matched = [strip_ansi(line) for line in lines if session_id in line]
    return matched[-limit:]


def extract_section(text: str, marker: str) -> str | None:
    idx = text.rfind(marker)
    if idx == -1:
        return None
    return text[idx:].strip()


def parse_markdown_field(text: str, label: str) -> str | None:
    prefix = f"- {label}: "
    for line in text.splitlines():
        if line.startswith(prefix):
            return line[len(prefix) :].strip().strip("`")
    return None


def build_artifact_snapshot(
    label: str,
    root: Path,
    attempt_started_at: datetime | None = None,
) -> ArtifactSnapshot:
    latest = latest_files(root, count=1)
    latest_path = latest[0] if latest else None
    summary = latest_summary_file(root)
    display_path = summary or latest_path
    latest_mtime = safe_mtime_label(display_path) if display_path else None
    attempt_start, fresh_for_attempt, freshness_note = attempt_freshness_for_path(
        display_path,
        attempt_started_at,
    )
    return ArtifactSnapshot(
        label=label,
        latest_file=str(display_path) if display_path else None,
        latest_mtime=latest_mtime,
        summary_file=str(summary) if summary else None,
        summary_excerpt=read_excerpt(summary),
        attempt_start=attempt_start,
        fresh_for_attempt=fresh_for_attempt,
        freshness_note=freshness_note,
    )


def parse_key_value_summary(text: str | None) -> dict[str, str]:
    values: dict[str, str] = {}
    if not text:
        return values
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or "=" not in line:
            continue
        key, _, value = line.partition("=")
        key = key.strip()
        if not key or " " in key:
            continue
        values[key] = value.strip()
    return values


def has_fresh_smoke_gate_escalation(snapshot_list: Iterable[ArtifactSnapshot]) -> bool:
    for snapshot in snapshot_list:
        if snapshot.label != "smoke" or snapshot.fresh_for_attempt is not True:
            continue
        fields = parse_key_value_summary(snapshot.summary_excerpt)
        public_status = fields.get("public_status", "").strip().upper()
        acceptance_status = fields.get("acceptance_signal_status", "").strip().upper()
        next_gate_command = fields.get("next_gate_command", "").strip()
        gate_chain_ac2_status = fields.get("gate_chain_ac2_status", "").strip().lower()
        standard_gap_status = fields.get("standard_gap_status", "").strip().lower()
        if (
            public_status == "PASS"
            and acceptance_status == "PASS"
            and next_gate_command == "./lca_strong_gate.sh"
            and gate_chain_ac2_status == "satisfied"
            and standard_gap_status == "ready_for_gate_escalation"
        ):
            return True
    return False


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--branch-root",
        default=str(REPO_ROOT),
        help="Branch root used to resolve artifact-local workflow inputs.",
    )
    parser.add_argument("--attempt", required=True, type=int)
    parser.add_argument("--seed-file", required=True)
    parser.add_argument("--workflow-log", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument("--exit-code", required=True, type=int)
    return parser.parse_args()


def _load_artifact_guard(branch_root: Path):
    sys.path.insert(0, str(branch_root))
    import artifact_paths as artifact_guard  # type: ignore

    return artifact_guard.ensure_under_artifacts, getattr(
        artifact_guard, "resolve_branch_artifact_path", None
    )


def _resolve_artifact_path(
    branch_root: Path,
    ensure_under_artifacts,
    value: str,
    shared_resolver=None,
) -> Path:
    if shared_resolver is not None:
        return shared_resolver(value)
    return resolve_artifact_output_path(branch_root, value, ensure_under_artifacts)


def collect_ac_context_lines(clean_log: str, ac_numbers: Iterable[int], forward_lines: int = 2) -> list[str]:
    numbers = list(ac_numbers)
    if not numbers:
        return []
    lines = clean_log.splitlines()
    selected: set[int] = set()
    patterns = []
    for ac in numbers:
        patterns.extend(
            (
                re.compile(rf"\bSub-AC \d+ of AC {ac}\b"),
                re.compile(rf"^\s*AC {ac}\b"),
                re.compile(rf"### AC {ac}\b"),
            )
        )
    for idx, line in enumerate(lines):
        if any(pattern.search(line) for pattern in patterns):
            for cursor in range(idx, min(len(lines), idx + 1 + forward_lines)):
                if cursor > idx and AC_TRACE_LINE_RE.search(lines[cursor]):
                    break
                selected.add(cursor)
    target_acs = set(numbers)
    filtered_lines: list[str] = []
    for idx in sorted(selected):
        line = strip_ansi(lines[idx])
        owner = ac_owner_for_line(line)
        if owner is not None and owner not in target_acs:
            continue
        filtered_lines.append(line)
    return filtered_lines[-240:]


def ac_owner_for_line(line: str) -> int | None:
    for regex in (SUB_AC_OWNER_RE, AC_OWNER_RE, MARKDOWN_AC_OWNER_RE):
        match = regex.search(line)
        if match:
            return int(match.group("ac"))
    return None


def extract_file_mentions(lines: Iterable[str]) -> Counter[str]:
    counter: Counter[str] = Counter()
    for line in lines:
        for match in FILE_MENTION_RE.finditer(line):
            path = normalize_path_token(match.group("path"))
            if not path:
                continue
            suffix = Path(path).suffix
            if suffix in CODE_SUFFIXES:
                counter[path] += 1
    return counter


def extract_focus_ranges(lines: Iterable[str], candidate_paths: Iterable[str]) -> dict[str, list[tuple[int, int]]]:
    path_set = set(candidate_paths)
    ranges: dict[str, list[tuple[int, int]]] = defaultdict(list)
    for line in lines:
        for regex in (SED_RANGE_RE, NL_RANGE_RE):
            for match in regex.finditer(line):
                path = normalize_path_token(match.group("path"))
                if path not in path_set:
                    continue
                start = int(match.group("start"))
                end = int(match.group("end"))
                ranges[path].append((start, end))
    return {path: normalize_focus_range_list(values) for path, values in ranges.items()}


def resolve_repo_path(branch_root: Path, relative_path: str) -> Path | None:
    candidate = Path(relative_path)
    if candidate.is_absolute():
        return candidate if candidate.exists() else None
    resolved = (branch_root / relative_path).resolve()
    return resolved if resolved.exists() else None


def canonical_focus_path(branch_root: Path, path_text: str) -> str:
    normalized = normalize_path_token(path_text)
    resolved = resolve_repo_path(branch_root, normalized)
    return str(resolved) if resolved is not None else normalized


def language_for_path(path: Path) -> str:
    suffix = path.suffix
    if suffix == ".py":
        return "python"
    if suffix == ".sh":
        return "shell"
    if suffix in {".cpp", ".hpp", ".cc", ".cxx"}:
        return "cpp"
    return "text"


def build_symbol_ranges(symbols: list[tuple[int, str, str]], total_lines: int) -> list[tuple[int, int, str, str]]:
    ranged: list[tuple[int, int, str, str]] = []
    for idx, (start, kind, name) in enumerate(symbols):
        next_start = symbols[idx + 1][0] if idx + 1 < len(symbols) else total_lines + 1
        ranged.append((start, max(start, next_start - 1), kind, name))
    return ranged


def python_symbol_ranges(text: str) -> list[tuple[int, int, str, str]]:
    try:
        tree = ast.parse(text)
    except SyntaxError:
        return []
    symbols: list[tuple[int, int, str, str]] = []
    for node in ast.walk(tree):
        if isinstance(node, ast.ClassDef):
            symbols.append((node.lineno, getattr(node, "end_lineno", node.lineno), "class", node.name))
        elif isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            symbols.append((node.lineno, getattr(node, "end_lineno", node.lineno), "function", node.name))
    symbols.sort(key=lambda item: (item[0], item[1], item[3]))
    return symbols


def shell_symbol_ranges(text: str) -> list[tuple[int, int, str, str]]:
    lines = text.splitlines()
    starts: list[tuple[int, str, str]] = []
    pattern = re.compile(r"^\s*(?:function\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*(?:\(\))?\s*\{")
    for lineno, line in enumerate(lines, start=1):
        match = pattern.match(line)
        if match:
            starts.append((lineno, "function", match.group(1)))
    return build_symbol_ranges(starts, len(lines))


def cpp_symbol_ranges(text: str) -> list[tuple[int, int, str, str]]:
    lines = text.splitlines()
    starts: list[tuple[int, str, str]] = []
    class_pattern = re.compile(r"^\s*(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\b")
    function_pattern = re.compile(
        r"^\s*(?:template\s*<.*>\s*)?(?:inline\s+)?(?:static\s+)?(?:constexpr\s+)?"
        r"(?:[\w:&<>\[\],*]+\s+)+(?P<name>[A-Za-z_~][A-Za-z0-9_:~]*)\s*\([^;]*\)\s*(?:const\s*)?(?:noexcept\s*)?\{"
    )
    control_keywords = ("if", "for", "while", "switch", "catch")
    for lineno, line in enumerate(lines, start=1):
        class_match = class_pattern.match(line)
        if class_match:
            starts.append((lineno, class_match.group(1), class_match.group(2)))
            continue
        function_match = function_pattern.match(line)
        if function_match:
            name = function_match.group("name")
            if not any(name.startswith(keyword) for keyword in control_keywords):
                starts.append((lineno, "function", name))
    return build_symbol_ranges(starts, len(lines))


def symbol_ranges_for_path(path: Path) -> list[tuple[int, int, str, str]]:
    try:
        text = path.read_text(errors="replace")
    except OSError:
        return []
    language = language_for_path(path)
    if language == "python":
        return python_symbol_ranges(text)
    if language == "shell":
        return shell_symbol_ranges(text)
    if language == "cpp":
        return cpp_symbol_ranges(text)
    return []


def wrapper_relative_path_for_ac(ac_index: int) -> str | None:
    return AC_WRAPPER_HINTS.get(ac_index)


def infer_wrapper_focus_from_trace(
    branch_root: Path,
    relevant_lines: Iterable[str],
    ac_index: int,
) -> tuple[str | None, list[tuple[int, int]], list[str], list[str]]:
    wrapper_relative_path = wrapper_relative_path_for_ac(ac_index)
    if not wrapper_relative_path:
        return None, [], [], []

    trace_lines = [line.strip() for line in relevant_lines if line.strip()]
    note_parts: list[str] = []
    evidence_lines: list[str] = []
    inferred_ranges: list[tuple[int, int]] = []

    wrapper_sed_lines = [
        line
        for line in trace_lines
        if "sed -n" in line and "outer_suite_wrappers/" in line
    ]
    if wrapper_sed_lines:
        note_parts.append("inferred from truncated wrapper sed trace")
        evidence_lines.extend(wrapper_sed_lines[:3])
        for line in wrapper_sed_lines:
            match = re.search(r"sed -n ['\"](?P<start>\d+),(?P<end>\d+)p['\"]", line)
            if match:
                inferred_ranges.append((int(match.group("start")), int(match.group("end"))))

    lock_markers = AC_WRAPPER_LOCK_MARKERS.get(ac_index, ())
    lock_lines = [
        line for line in trace_lines if any(marker in line.lower() for marker in lock_markers)
    ]
    if lock_lines:
        note_parts.append("inferred from lock-artifact inspection")
        evidence_lines.extend(lock_lines[:3])
        wrapper_path = resolve_repo_path(branch_root, wrapper_relative_path)
        if wrapper_path is not None:
            acquire_lock_entry = next(
                (
                    entry
                    for entry in symbol_ranges_for_path(wrapper_path)
                    if entry[2] == "function" and entry[3] == "acquire_lock"
                ),
                None,
            )
            if acquire_lock_entry is not None:
                inferred_ranges.append((acquire_lock_entry[0], acquire_lock_entry[1]))

    if not inferred_ranges and not evidence_lines:
        return None, [], [], []

    return (
        wrapper_relative_path,
        normalize_focus_range_list(inferred_ranges),
        list(dict.fromkeys(note_parts)),
        list(dict.fromkeys(evidence_lines))[:6],
    )


def infer_solver_focus_from_trace(
    relevant_lines: Iterable[str],
    ac_index: int,
) -> dict[str, dict[str, object]]:
    specs = AC_SOLVER_TRACE_HINTS.get(ac_index, ())
    if not specs:
        return {}

    trace_lines = [line.strip() for line in relevant_lines if line.strip()]
    inferred: dict[str, dict[str, object]] = {}
    for spec in specs:
        path = str(spec["path"])
        basename = Path(path).name
        truncated_solver_prefixes: tuple[str, ...] = ()
        if basename == "boj28350_branch_3_solver.cpp":
            truncated_solver_prefixes = (
                "boj28350_resume/boj28",
                "boj28350_branch_3_so",
            )
        ranges = [tuple(item) for item in spec.get("ranges", ())]
        token_hits = tuple(str(token).lower() for token in spec.get("tokens", ()))

        matched_lines: list[str] = []
        for line in trace_lines:
            line_lower = line.lower()
            mentions_path = path in line or basename in line
            mentions_truncated_solver = bool(
                truncated_solver_prefixes
                and any(prefix in line for prefix in truncated_solver_prefixes)
            )
            token_match = any(token in line_lower for token in token_hits)
            if mentions_path and any(token in line_lower for token in token_hits):
                matched_lines.append(line)
                continue
            if (mentions_path or mentions_truncated_solver) and token_match:
                matched_lines.append(line)
                continue
            if (mentions_path or mentions_truncated_solver) and "sed -n" in line:
                match = re.search(r"sed -n ['\"](?P<start>\d+),(?P<end>\d+)p['\"]", line)
                if not match:
                    continue
                start = int(match.group("start"))
                end = int(match.group("end"))
                if any(not (end < lo or start > hi) for lo, hi in ranges):
                    matched_lines.append(line)

        if not matched_lines:
            continue

        entry = inferred.setdefault(
            path,
            {"ranges": [], "notes": [], "evidence": [], "boost": 0},
        )
        entry["ranges"].extend(ranges)
        entry["notes"].append(str(spec["note"]))
        entry["evidence"].extend(matched_lines[:3])
        entry["boost"] = int(entry["boost"]) + int(spec.get("boost", 0))

    for entry in inferred.values():
        entry["ranges"] = normalize_focus_range_list(entry["ranges"])
        entry["notes"] = list(dict.fromkeys(entry["notes"]))
        entry["evidence"] = list(dict.fromkeys(entry["evidence"]))[:6]
    return inferred


def range_label(start: int, end: int) -> str:
    return f"{start}-{end}"


def lock_focus_for_probe(
    branch_root: Path,
    gate_script: str | None,
) -> tuple[str | None, tuple[int, int] | None, str | None]:
    if not gate_script:
        return None, None, None
    wrapper_relative_path = LOCK_WRAPPER_RELATIVE_PATHS.get(gate_script)
    if not wrapper_relative_path:
        return None, None, None
    wrapper_path = resolve_repo_path(branch_root, wrapper_relative_path)
    if wrapper_path is None:
        return wrapper_relative_path, None, None

    try:
        lines = wrapper_path.read_text(errors="replace").splitlines()
    except OSError:
        return wrapper_relative_path, None, None

    marker = f"another {gate_script} run is active"
    fail_line = next((lineno for lineno, line in enumerate(lines, start=1) if marker in line), None)
    if fail_line is None:
        return wrapper_relative_path, None, None

    symbols = symbol_ranges_for_path(wrapper_path)
    symbol_entry = next((entry for entry in symbols if entry[0] <= fail_line <= entry[1]), None)
    focus_range = (max(1, fail_line - 11), min(len(lines), fail_line + 3))
    focus_symbol = None
    if symbol_entry is not None:
        focus_range = (
            max(symbol_entry[0], focus_range[0]),
            min(symbol_entry[1], focus_range[1]),
        )
        focus_symbol = f"{symbol_entry[2]} {symbol_entry[3]} [{symbol_entry[0]}-{symbol_entry[1]}]"
    return wrapper_relative_path, focus_range, focus_symbol


def load_latest_next_probe_signal(branch_root: Path) -> ProbeSignal | None:
    probe_report = branch_root / "artifacts/lca_tree_stress_v5/retry_loop/latest_next_probe_result.md"
    if not probe_report.exists():
        return None
    try:
        text = probe_report.read_text(errors="replace")
    except OSError:
        return None

    stderr_relative = normalize_repo_relative_path(parse_markdown_field(text, "Stderr log") or "")
    stdout_relative = normalize_repo_relative_path(parse_markdown_field(text, "Stdout log") or "")
    stderr_path = resolve_repo_path(branch_root, stderr_relative) if stderr_relative else None
    stderr_excerpt_text = read_excerpt(stderr_path, max_lines=20)
    stderr_excerpt = stderr_excerpt_text.splitlines() if stderr_excerpt_text else []

    active_gate = None
    active_pid = None
    for line in stderr_excerpt:
        active_match = PROBE_ACTIVE_GATE_RE.search(line)
        if active_match:
            active_gate = active_match.group("gate")
            active_pid = active_match.group("pid")
            break

    wrapper_path, focus_range, focus_symbol = lock_focus_for_probe(branch_root, active_gate)
    return ProbeSignal(
        command=parse_markdown_field(text, "Command"),
        primary_axis=parse_markdown_field(text, "Primary axis"),
        secondary_axis=parse_markdown_field(text, "Secondary axis"),
        why_this_axis=parse_markdown_field(text, "Why this axis"),
        exit_code=parse_optional_int(parse_markdown_field(text, "Exit code")),
        timed_out=parse_optional_bool(parse_markdown_field(text, "Timed out")),
        elapsed_seconds=parse_optional_float(parse_markdown_field(text, "Elapsed seconds")),
        stdout_log=stdout_relative or None,
        stderr_log=stderr_relative or None,
        stderr_excerpt=stderr_excerpt[:6],
        active_gate=active_gate,
        active_pid=active_pid,
        wrapper_path=wrapper_path,
        focus_range=focus_range,
        focus_symbol=focus_symbol,
    )


def load_attempt_guard(attempt_dir: Path) -> dict | None:
    payload = load_json(attempt_dir / "attempt_guard.json")
    return payload if isinstance(payload, dict) else None


def guard_implied_acs(attempt_guard: dict | None) -> list[int]:
    if not isinstance(attempt_guard, dict):
        return []
    findings = attempt_guard.get("findings")
    if not isinstance(findings, list):
        return []

    implicated: set[int] = set()
    for finding in findings:
        if not isinstance(finding, dict):
            continue
        ac_index = finding.get("ac_index")
        if isinstance(ac_index, int) and ac_index > 0:
            implicated.add(ac_index)
            continue
        if ac_index != 0:
            continue
        for line in finding.get("evidence_lines") or []:
            if not isinstance(line, str):
                continue
            for match in GUARD_QA_AC_MENTION_RE.finditer(line):
                try:
                    implied_ac = int(match.group("ac"))
                except ValueError:
                    continue
                if implied_ac > 0:
                    implicated.add(implied_ac)
    return sorted(implicated)


def branch_run_case_probe_for_clean_log(clean_log: str) -> str | None:
    normalized = " ".join(
        strip_ansi(clean_log).replace("│", " ").replace("`", " ").split()
    )
    best_match = None
    for match in CASE_BLOCKER_RE.finditer(normalized):
        mode = match.group("mode")
        if mode.startswith("comb_"):
            best_match = match

    if best_match is None:
        return None

    mode = best_match.group("mode")
    n_value = best_match.group("n")
    seed = best_match.group("seed")
    shuffle_labels = best_match.group("label")
    shuffle_queries = best_match.group("query")
    outdir = (
        "artifacts/lca_tree_stress_v5/retry_loop/"
        f"guard_probe_{mode}_{n_value}_seed{seed}_L{shuffle_labels}_Q{shuffle_queries}"
    )
    return (
        f"python3 branch_run_case.py {mode} {n_value} {seed} {shuffle_labels} {shuffle_queries} "
        f"boj28350_resume/solve {outdir} --timeout 30.0"
    )


def probe_signal_applies_to_ac(probe_signal: ProbeSignal | None, ac_index: int) -> bool:
    if probe_signal is None:
        return False
    if ac_index in {3, 4}:
        return probe_signal.active_gate == "lca_strong_gate.sh" or (
            probe_signal.command is not None and "lca_strong_gate.sh" in probe_signal.command
        )
    if ac_index in {5, 6}:
        return probe_signal.active_gate == "lca_boj3s_gate.sh" or (
            probe_signal.command is not None and "lca_boj3s_gate.sh" in probe_signal.command
        )
    return False


def probe_signal_is_quick_fail_lock(probe_signal: ProbeSignal | None, ac_index: int) -> bool:
    if not probe_signal_applies_to_ac(probe_signal, ac_index) or probe_signal is None:
        return False
    return (
        probe_signal.active_gate is not None
        and probe_signal.active_pid is not None
        and probe_signal.exit_code == 1
        and probe_signal.timed_out is False
        and probe_signal.elapsed_seconds is not None
        and probe_signal.elapsed_seconds < 1.0
    )


def probe_signal_completed_case_counts(probe_signal: ProbeSignal | None) -> list[int]:
    if probe_signal is None:
        return []
    counts: list[int] = []
    for line in probe_signal.stderr_excerpt:
        match = PROBE_COMPLETED_CASES_RE.search(line)
        if match is None:
            continue
        counts.append(int(match.group("count")))
    return counts


def probe_signal_is_zero_progress_timeout(probe_signal: ProbeSignal | None, ac_index: int) -> bool:
    if not probe_signal_applies_to_ac(probe_signal, ac_index) or probe_signal is None:
        return False
    completed_counts = probe_signal_completed_case_counts(probe_signal)
    return (
        probe_signal.exit_code == 124
        and probe_signal.timed_out is True
        and bool(completed_counts)
        and max(completed_counts) == 0
    )


def probe_context_lines_for_ac(probe_signal: ProbeSignal | None, ac_index: int) -> list[str]:
    if not probe_signal_applies_to_ac(probe_signal, ac_index) or probe_signal is None:
        return []

    lines: list[str] = []
    if probe_signal.command:
        lines.append(f"latest_next_probe_result.md → Command: {probe_signal.command}")
    if probe_signal.exit_code is not None or probe_signal.elapsed_seconds is not None:
        timed_out = "unknown"
        if probe_signal.timed_out is True:
            timed_out = "yes"
        elif probe_signal.timed_out is False:
            timed_out = "no"
        lines.append(
            "latest_next_probe_result.md → "
            f"Exit code {probe_signal.exit_code if probe_signal.exit_code is not None else 'unknown'} "
            f"elapsed {probe_signal.elapsed_seconds if probe_signal.elapsed_seconds is not None else 'unknown'}s "
            f"timed_out={timed_out}"
        )
    if probe_signal.active_gate and probe_signal.active_pid:
        wrapper_label = probe_signal.wrapper_path or "unknown"
        lines.append(
            "latest_next_probe_result.md → "
            f"quick-fail active-holder branch for {probe_signal.active_gate} pid {probe_signal.active_pid} "
            f"wrapper={wrapper_label}"
        )
    if probe_signal.stderr_log:
        lines.append(f"latest_next_probe_result.md → stderr log: {probe_signal.stderr_log}")
    for line in probe_signal.stderr_excerpt[:4]:
        lines.append(f"latest_next_probe_result.md → {line}")
    return lines[:6]


def select_symbols(
    symbols: list[tuple[int, int, str, str]],
    focus_ranges: list[tuple[int, int]],
) -> list[str]:
    selected_entries = select_symbol_entries(symbols, focus_ranges)
    return [f"{entry[2]} {entry[3]} [{entry[0]}-{entry[1]}]" for entry in selected_entries]


def select_symbol_entries(
    symbols: list[tuple[int, int, str, str]],
    focus_ranges: list[tuple[int, int]],
) -> list[tuple[int, int, str, str]]:
    if not symbols:
        return []

    selected: list[tuple[int, int, str, str]] = []

    def push(entry: tuple[int, int, str, str]) -> None:
        if entry not in selected:
            selected.append(entry)

    if focus_ranges:
        for start, end in focus_ranges:
            overlaps = [entry for entry in symbols if not (entry[1] < start or entry[0] > end)]
            if overlaps:
                for entry in overlaps[:4]:
                    push(entry)
                continue
            nearest = min(symbols, key=lambda entry: min(abs(entry[0] - start), abs(entry[1] - end)))
            push(nearest)
    else:
        for entry in symbols[:4]:
            push(entry)

    return selected[:6]


def evidence_lines_for_path(relevant_lines: Iterable[str], relative_path: str) -> list[str]:
    basename = Path(relative_path).name
    normalized = relative_path.replace("./", "")
    hits = [
        line.strip()
        for line in relevant_lines
        if basename in line or normalized in line or f"/{basename}" in line
    ]
    deduped = list(dict.fromkeys(hits))
    return deduped[:6]


def state_anchor_allowed_for_failure_family(path: str, failure_family: str | None) -> bool:
    normalized = normalize_repo_relative_path(path)
    if failure_family == "transport_disconnected_retry":
        if normalized in {
            "artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md",
            "artifacts/lca_tree_stress_v5/retry_loop/latest_attempt_guard.md",
        }:
            return True
        if re.fullmatch(
            r"artifacts/lca_tree_stress_v5/retry_loop/attempt_\d+_\d{8}_\d{6}/workflow\.log",
            normalized,
        ):
            return True
    return False


def format_attempt_label(value: object) -> str | None:
    if value is None:
        return None
    if isinstance(value, str) and value.startswith("attempt_"):
        return value
    try:
        return f"attempt_{int(value):03d}"
    except (TypeError, ValueError):
        text = str(value).strip()
        return text or None


def analysis_state_attempt_matches(
    analysis_state: dict | None,
    current_attempt_label: str | None,
) -> bool:
    if not isinstance(analysis_state, dict) or not current_attempt_label:
        return False

    current_failure = analysis_state.get("current_failure")
    candidates = [
        analysis_state.get("current_failure_attempt"),
        analysis_state.get("last_failed_attempt"),
    ]
    if isinstance(current_failure, dict):
        candidates.append(current_failure.get("attempt_label"))

    normalized_current = format_attempt_label(current_attempt_label)
    for candidate in candidates:
        if format_attempt_label(candidate) == normalized_current:
            return True
    return False


def ac_retry_anchor_specs(
    ac_index: int,
    analysis_state: dict | None = None,
    probe_signal: ProbeSignal | None = None,
    relevant_lines: Iterable[str] | None = None,
    stale_formal_artifacts: set[str] | None = None,
    failure_family: str | None = None,
    current_attempt_label: str | None = None,
) -> list[dict[str, object]]:
    specs: list[dict[str, object]] = []
    spec_index_by_key: dict[tuple[str, tuple[int, int]], int] = {}

    def register_spec(spec: dict[str, object]) -> None:
        path = normalize_repo_relative_path(str(spec.get("path") or ""))
        raw_range = spec.get("range")
        if not path or not isinstance(raw_range, tuple) or len(raw_range) != 2:
            return
        families = spec.get("families")
        if failure_family and isinstance(families, tuple) and families and failure_family not in families:
            return
        focus_range = (int(raw_range[0]), int(raw_range[1]))
        normalized_spec = dict(spec)
        normalized_spec["path"] = path
        normalized_spec["range"] = focus_range
        key = (path, focus_range)
        existing_idx = spec_index_by_key.get(key)
        if existing_idx is None:
            spec_index_by_key[key] = len(specs)
            specs.append(normalized_spec)
            return

        existing = specs[existing_idx]
        if int(normalized_spec.get("priority", 0)) > int(existing.get("priority", 0)):
            existing["label"] = normalized_spec.get("label", existing.get("label"))
            existing["note"] = normalized_spec.get("note", existing.get("note"))
            existing["priority"] = normalized_spec.get("priority", existing.get("priority"))
        if normalized_spec.get("statement_excerpt") and not existing.get("statement_excerpt"):
            existing["statement_excerpt"] = normalized_spec.get("statement_excerpt")
        if normalized_spec.get("symbol") and not existing.get("symbol"):
            existing["symbol"] = normalized_spec.get("symbol")
        if normalized_spec.get("allow_non_code"):
            existing["allow_non_code"] = True
        merged_evidence = list(existing.get("evidence_lines") or [])
        for line in normalized_spec.get("evidence_lines") or []:
            if isinstance(line, str) and line and line not in merged_evidence:
                merged_evidence.append(line)
        if merged_evidence:
            existing["evidence_lines"] = merged_evidence

    for spec in AC_RETRY_ANCHOR_HINTS.get(ac_index, ()):
        register_spec(dict(spec))

    relevant_paths = {
        normalize_repo_relative_path(str(spec["path"]))
        for spec in AC_RETRY_ANCHOR_HINTS.get(ac_index, ())
    }
    relevant_paths.update(
        normalize_repo_relative_path(str(spec["path"]))
        for spec in AC_SOLVER_TRACE_HINTS.get(ac_index, ())
    )
    relevant_paths.update(
        normalize_repo_relative_path(path) for path in AC_FILE_HINTS.get(ac_index, [])
    )
    wrapper_hint = wrapper_relative_path_for_ac(ac_index)
    if wrapper_hint:
        relevant_paths.add(normalize_repo_relative_path(wrapper_hint))

    for spec in iteration_failure_point_specs(Path(__file__).resolve().with_name("failure_analysis_iteration.md")):
        register_spec(spec)

    state_anchor_reuse_allowed = analysis_state_attempt_matches(
        analysis_state,
        current_attempt_label,
    )

    if isinstance(analysis_state, dict) and state_anchor_reuse_allowed:
        for item in analysis_state.get("latest_retry_statement_anchors", []):
            if not isinstance(item, dict):
                continue
            path = normalize_repo_relative_path(str(item.get("path") or ""))
            focus_range = parse_focus_range_text(item.get("focus_range"))
            if not path or focus_range is None:
                continue
            if (
                relevant_paths
                and path not in relevant_paths
                and not state_anchor_allowed_for_failure_family(path, failure_family)
            ):
                continue
            role = str(item.get("role") or "").strip()
            note = "promoted from failure_analysis_state.latest_retry_statement_anchors"
            if role:
                note += f"; role: {role}"
            evidence = str(item.get("evidence") or "").strip()
            register_spec(
                {
                    "path": path,
                    "range": focus_range,
                    "label": str(item.get("label") or f"{path}:{range_label(*focus_range)}").strip(),
                    "note": note,
                    "priority": 360,
                    "statement_excerpt": str(item.get("excerpt") or "").strip() or None,
                    "symbol": str(item.get("symbol") or "").strip() or None,
                    "allow_non_code": True,
                    "evidence_lines": [evidence] if evidence else [],
                }
            )
        for item in analysis_state.get("latest_retry_anchor_ranges", []):
            parsed = parse_anchor_range_text(item)
            if parsed is None:
                continue
            path, focus_range = parsed
            if relevant_paths and path not in relevant_paths:
                continue
            register_spec(
                {
                    "path": path,
                    "range": focus_range,
                    "label": f"State-pinned anchor {path}:{range_label(*focus_range)}",
                    "note": "promoted from failure_analysis_state.latest_retry_anchor_ranges",
                    "priority": 180,
                }
            )

    if probe_signal_applies_to_ac(probe_signal, ac_index) and probe_signal is not None:
        if probe_signal.wrapper_path and probe_signal.focus_range is not None:
            register_spec(
                {
                    "path": normalize_repo_relative_path(probe_signal.wrapper_path),
                    "range": probe_signal.focus_range,
                    "label": "Latest probe wrapper anchor",
                    "note": "promoted from latest_next_probe_result quick-fail wrapper focus",
                    "priority": 320,
                }
            )

    stale_formal_artifacts = set(stale_formal_artifacts or ())
    if stale_formal_artifacts and not ac_has_direct_certify_trace(relevant_lines or ()):
        specs = [
            spec
            for spec in specs
            if not is_pre_artifact_fallback_anchor(
                normalize_repo_relative_path(str(spec["path"])),
                tuple(spec["range"]),
                stale_formal_artifacts,
            )
        ]

    return specs


def retry_anchor_priority_paths(
    branch_root: Path,
    ac_index: int,
    analysis_state: dict | None = None,
    probe_signal: ProbeSignal | None = None,
    relevant_lines: Iterable[str] | None = None,
    stale_formal_artifacts: set[str] | None = None,
    failure_family: str | None = None,
    current_attempt_label: str | None = None,
) -> set[str]:
    return {
        canonical_focus_path(branch_root, str(spec["path"]))
        for spec in ac_retry_anchor_specs(
            ac_index,
            analysis_state=analysis_state,
            probe_signal=probe_signal,
            relevant_lines=relevant_lines,
            stale_formal_artifacts=stale_formal_artifacts,
            failure_family=failure_family,
            current_attempt_label=current_attempt_label,
        )
    }


def code_excerpt_for_focus(
    path: Path,
    focus_ranges: list[tuple[int, int]],
    symbol_entries: list[tuple[int, int, str, str]],
    max_lines: int = 28,
) -> str | None:
    try:
        lines = path.read_text(errors="replace").splitlines()
    except OSError:
        return None
    if not lines:
        return None

    if focus_ranges:
        start = max(1, focus_ranges[0][0] - 2)
        end = min(len(lines), max(focus_ranges[0][1], focus_ranges[0][0]) + 2)
    elif symbol_entries:
        start = max(1, symbol_entries[0][0] - 2)
        end = min(len(lines), symbol_entries[0][1] + 2)
    else:
        start = 1
        end = min(len(lines), max_lines)

    if end - start + 1 > max_lines:
        end = start + max_lines - 1

    excerpt_lines = []
    for lineno in range(start, end + 1):
        excerpt_lines.append(f"{lineno:5d}: {lines[lineno - 1]}")
    return "\n".join(excerpt_lines)


def exact_statement_excerpt_for_focus(path: Path, focus_range: tuple[int, int]) -> str | None:
    if focus_range[1] < focus_range[0] or focus_range[1] - focus_range[0] > 5:
        return None
    try:
        lines = path.read_text(errors="replace").splitlines()
    except OSError:
        return None

    excerpt_lines = []
    for line_no in range(focus_range[0], focus_range[1] + 1):
        if 1 <= line_no <= len(lines):
            snippet = lines[line_no - 1].strip()
            if snippet:
                excerpt_lines.append(snippet)
    if not excerpt_lines:
        return None

    excerpt = re.sub(r"\s+", " ", " ".join(excerpt_lines)).strip()
    if len(excerpt) > 240:
        excerpt = excerpt[:237].rstrip() + "..."
    return excerpt


def normalized_statement_excerpt(text: str | None) -> str | None:
    if text is None:
        return None
    excerpt = re.sub(r"\s+", " ", str(text)).strip()
    return excerpt or None


def retarget_state_anchor_label(
    label: str,
    original_range: tuple[int, int],
    resolved_range: tuple[int, int],
) -> str:
    stripped = label.strip()
    if not stripped:
        return f"focus {range_label(*resolved_range)}"
    original_label = range_label(*original_range)
    resolved_label = range_label(*resolved_range)
    if stripped == f"focus {original_label}":
        return f"focus {resolved_label}"
    if original_label in stripped:
        return stripped.replace(original_label, resolved_label)
    return stripped


def reconcile_state_anchor_to_live_source(
    path: Path,
    focus_range: tuple[int, int],
    stored_excerpt: str | None,
) -> tuple[tuple[int, int], str | None, str | None]:
    live_excerpt = normalized_statement_excerpt(exact_statement_excerpt_for_focus(path, focus_range))
    stored_normalized = normalized_statement_excerpt(stored_excerpt)
    if not stored_normalized:
        return focus_range, live_excerpt, None
    if live_excerpt == stored_normalized:
        return focus_range, live_excerpt, None

    # Punctuation-only snippets such as `;;` are too ambiguous to relocate safely.
    if len(re.sub(r"[^A-Za-z0-9_]+", "", stored_normalized)) < 8:
        note = None
        if live_excerpt:
            note = "stored state excerpt no longer matches current source; using live source statement"
        return focus_range, live_excerpt or stored_normalized, note

    try:
        lines = path.read_text(errors="replace").splitlines()
    except OSError:
        return focus_range, live_excerpt or stored_normalized, None

    if not lines:
        return focus_range, live_excerpt or stored_normalized, None

    search_start = max(1, focus_range[0] - 24)
    search_end = min(len(lines), focus_range[1] + 24)
    max_span = max(1, min(5, focus_range[1] - focus_range[0] + 1))
    matches: list[tuple[int, int]] = []
    for start in range(search_start, search_end + 1):
        for end in range(start, min(search_end, start + max_span - 1) + 1):
            candidate = normalized_statement_excerpt(
                exact_statement_excerpt_for_focus(path, (start, end))
            )
            if candidate == stored_normalized:
                matches.append((start, end))
                break

    if len(matches) == 1:
        return (
            matches[0],
            stored_normalized,
            "retargeted to current source range via stored statement excerpt match",
        )

    note = None
    if live_excerpt:
        note = "stored state excerpt no longer matches current source; using live source statement"
    return focus_range, live_excerpt or stored_normalized, note


def retry_critical_anchors_for_ac(
    branch_root: Path,
    clean_log: str,
    ac_index: int,
    analysis_state: dict | None = None,
    probe_signal: ProbeSignal | None = None,
    stale_formal_artifacts: set[str] | None = None,
    failure_family: str | None = None,
    current_attempt_label: str | None = None,
) -> list[RetryCriticalAnchor]:
    relevant_lines = collect_ac_context_lines(clean_log, [ac_index])
    probe_context = probe_context_lines_for_ac(probe_signal, ac_index)
    relevant_lines.extend(probe_context)
    state_trace_lines = [
        str(line).strip()
        for line in (analysis_state or {}).get("latest_retry_trace_lines", [])
        if isinstance(line, str) and line.strip()
    ]

    anchors: list[RetryCriticalAnchor] = []
    anchor_specs = ac_retry_anchor_specs(
        ac_index,
        analysis_state=analysis_state,
        probe_signal=probe_signal,
        relevant_lines=relevant_lines,
        stale_formal_artifacts=stale_formal_artifacts,
        failure_family=failure_family or failure_family_for_ac(clean_log, ac_index, branch_root),
        current_attempt_label=current_attempt_label,
    )
    anchor_specs.sort(
        key=lambda spec: (
            -int(spec.get("priority", 100)),
            int(spec["range"][1]) - int(spec["range"][0]),
            str(spec["path"]),
            int(spec["range"][0]),
        )
    )
    for spec in anchor_specs[:8]:
        relative_path = normalize_repo_relative_path(str(spec["path"]))
        resolved = resolve_repo_path(branch_root, relative_path)
        allow_non_code = bool(spec.get("allow_non_code"))
        if resolved is None or (resolved.suffix not in TEXTUAL_RETRY_ANCHOR_SUFFIXES and not allow_non_code):
            continue
        original_focus_range = tuple(spec["range"])
        focus_range = original_focus_range
        evidence_lines = evidence_lines_for_path(relevant_lines, relative_path)
        for line in spec.get("evidence_lines") or []:
            if isinstance(line, str) and line.strip():
                evidence_lines.append(line.strip())
        basename = Path(relative_path).name
        truncated_solver_prefixes: tuple[str, ...] = ()
        if basename == "boj28350_branch_3_solver.cpp":
            truncated_solver_prefixes = ("boj28350_resume/boj28", "boj28350_branch_3_so")
        for line in state_trace_lines:
            if basename in line or relative_path in line:
                evidence_lines.append(line)
                continue
            if truncated_solver_prefixes and any(prefix in line for prefix in truncated_solver_prefixes):
                evidence_lines.append(line)
        if (
            probe_signal_applies_to_ac(probe_signal, ac_index)
            and probe_signal is not None
            and probe_signal.wrapper_path
            and probe_signal.focus_range == focus_range
            and normalize_repo_relative_path(probe_signal.wrapper_path) == relative_path
        ):
            evidence_lines.extend(probe_context)
        evidence_lines = list(dict.fromkeys(line for line in evidence_lines if line))[:6]

        note_parts = [str(spec.get("note", "promoted from retry anchor hints"))]
        if isinstance(analysis_state, dict) and "failure_analysis_state" in str(spec.get("note", "")):
            note_parts.append("pinned in failure_analysis_state")
        stored_statement_excerpt = (
            str(spec.get("statement_excerpt")).strip() if spec.get("statement_excerpt") else None
        )
        if "failure_analysis_state" in str(spec.get("note", "")) and resolved.suffix in CODE_SUFFIXES:
            focus_range, reconciled_excerpt, reconcile_note = reconcile_state_anchor_to_live_source(
                resolved,
                focus_range,
                stored_statement_excerpt,
            )
            if reconcile_note:
                note_parts.append(reconcile_note)
            statement_excerpt = reconciled_excerpt or stored_statement_excerpt
        else:
            statement_excerpt = stored_statement_excerpt

        anchor_label = str(spec.get("label", f"{relative_path}:{range_label(*focus_range)}"))
        if focus_range != original_focus_range:
            anchor_label = retarget_state_anchor_label(anchor_label, original_focus_range, focus_range)

        symbol_entries: list[tuple[int, int, str, str]] = []
        symbol = None
        if resolved.suffix in CODE_SUFFIXES:
            symbol_entries = select_symbol_entries(symbol_ranges_for_path(resolved), [focus_range])
            if symbol_entries:
                entry = symbol_entries[0]
                symbol = f"{entry[2]} {entry[3]} [{entry[0]}-{entry[1]}]"
        if symbol is None and spec.get("symbol"):
            symbol = str(spec["symbol"]).strip() or None

        if (
            probe_signal_applies_to_ac(probe_signal, ac_index)
            and probe_signal is not None
            and probe_signal.wrapper_path
            and probe_signal.focus_range == focus_range
            and normalize_repo_relative_path(probe_signal.wrapper_path) == relative_path
        ):
            note_parts.append("matched latest_next_probe_result wrapper focus")
        if not statement_excerpt:
            statement_excerpt = exact_statement_excerpt_for_focus(resolved, focus_range)

        anchors.append(
            RetryCriticalAnchor(
                label=anchor_label,
                path=str(resolved),
                focus_range=range_label(*focus_range),
                symbol=symbol,
                evidence_lines=evidence_lines,
                code_excerpt=code_excerpt_for_focus(resolved, [focus_range], symbol_entries, max_lines=18),
                statement_excerpt=statement_excerpt,
                note=", ".join(dict.fromkeys(note_parts)),
            )
        )

    return anchors[:8]


def failure_type_for_ac(clean_log: str, ac_index: int) -> str:
    section_match = re.search(rf"### AC {ac_index}:.*?(?=\n### AC |\Z)", clean_log, re.DOTALL)
    section = section_match.group(0).lower() if section_match else clean_log.lower()
    if "stalled" in section or "stall_detected" in clean_log.lower():
        return "stall/no-activity"
    if "timeout" in section:
        return "timeout"
    if "compile" in section or "clang++" in section:
        return "compile/build"
    if "valueerror" in section or "traceback" in section:
        return "exception"
    return "failure"


def axis_from_text(text: str) -> str | None:
    lowered = text.lower()
    for axis, markers in AXIS_RULES:
        if any(marker in lowered for marker in markers):
            return axis
    return None


def current_progress40_summary(branch_root: Path) -> dict[str, object]:
    summary_path = branch_root / "boj28350_resume/current_state_summary.md"
    try:
        text = summary_path.read_text(errors="replace")
    except OSError:
        return {"pivot_text": None, "pivot_axis": None, "residual_axes": []}

    pivot_text = None
    for line in text.splitlines():
        if "next pivot after layout-gate round:" in line:
            pivot_text = line.strip().strip("`")
            pivot_text = pivot_text.split(":", 1)[1].strip() if ":" in pivot_text else pivot_text
            break

    residual_axes: list[str] = []
    for match in re.finditer(r"`([^`]+)`\s+\d+\.\d+ms", text):
        axis = axis_from_text(match.group(1))
        if axis and axis not in residual_axes:
            residual_axes.append(axis)

    pivot_axis = axis_from_text(pivot_text or "")
    return {
        "pivot_text": pivot_text,
        "pivot_axis": pivot_axis,
        "residual_axes": residual_axes,
    }


def latest_smoke_target_wrapper_syntax_stderr(branch_root: Path) -> str:
    path = branch_root / "artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure/smoke_target_wrapper_syntax.stderr.txt"
    try:
        return path.read_text(errors="replace")
    except OSError:
        return ""


def has_smoke_target_wrapper_operation_canceled(
    branch_root: Path | None,
    section_text: str,
) -> bool:
    lowered = section_text.lower()
    if "operation canceled" in lowered and "lca_smoke_target.sh" in lowered:
        return True
    if branch_root is None:
        return False
    artifact_text = latest_smoke_target_wrapper_syntax_stderr(branch_root).lower()
    return "operation canceled" in artifact_text and "lca_smoke_target.sh" in artifact_text


def profile_mode_for_text(text: str) -> str | None:
    matches = PROFILE_MODE_RE.findall(text)
    return matches[-1] if matches else None


def enabled_flags_for_text(text: str) -> list[str]:
    return sorted(set(ENABLE_FLAG_RE.findall(text)))


def last_progress_phase_for_text(text: str) -> str | None:
    matches = PROGRESS_PHASE_RE.findall(text)
    return matches[-1] if matches else None


def last_release_diag_phase_for_text(text: str) -> str | None:
    matches = RELEASE_DIAG_PHASE_RE.findall(text)
    return matches[-1] if matches else None


def failure_family_for_ac(clean_log: str, ac_index: int, branch_root: Path | None = None) -> str:
    section_match = re.search(rf"### AC {ac_index}:.*?(?=\n### AC |\Z)", clean_log, re.DOTALL)
    section = section_match.group(0).lower() if section_match else clean_log.lower()
    if (
        "stream disconnected before completion: error sending request for url" in section
        and "backend-api/codex/responses" in section
    ):
        return "transport_disconnected_retry"
    if (
        ac_index == 2
        and (
            "broken smoke target wrapper syntax" in section
            or (
                "shell_entrypoint_validation" in section
                and "smoke target wrapper syntax" in section
            )
        )
        and has_smoke_target_wrapper_operation_canceled(branch_root, section)
    ):
        return "smoke_target_wrapper_readability_failure"
    if (
        ac_index == 2
        and (
            "without publishing a complete fresh failure bundle" in section
            or "without publishing a fresh smoke bundle" in section
            or
            "missing failure summary" in section
            or "missing failure report" in section
            or "missing preserved failure root" in section
            or ("bundle_validation" in section and "smoke_latest_failure" in section)
        )
    ):
        return "smoke_bundle_publication_gap"
    if (
        ac_index == 2
        and (
            "invalid smoke case manifest" in section
            or "failure_stage=smoke_manifest_validation" in section
            or (
                "last_check_kind=smoke_manifest" in section
                and "last_check_status=invalid" in section
            )
        )
    ):
        return "smoke_manifest_contract_invalid"
    if (
        ac_index == 2
        and (
            "inner wrapper dispatch monitor failed with exit code" in section
            or "failed stage: dispatch_monitor" in section
            or "stage=dispatch_monitor" in section
            or (
                "last recorded check: kind=dispatch_monitor" in section
                and "status=broken" in section
            )
        )
    ):
        return "smoke_dispatch_monitor_helper_failure"
    if "another lca_strong_gate.sh run is active" in section or "failed to acquire strong gate lock" in section:
        return "strong_gate_lock_contention"
    if "another lca_boj3s_gate.sh run is active" in section or "failed to acquire boj3s gate lock" in section:
        return "boj3s_gate_lock_contention"
    family_markers = (
        ("correctness_fuzz", ("correctness_fuzz",)),
        ("hard_scaling", ("hard_scaling",)),
        ("max_n_mix", ("max_n_mix",)),
        ("boj_3s_large_mix", ("boj_3s_large_mix", "large_mix")),
        ("broom_mixed", ("broom_mixed",)),
        ("dense_1024_release", ("dense_1024",)),
        ("dense_4096_release", ("dense_4096",)),
        ("multi_4096_release", ("multi_4096",)),
    )
    for family, markers in family_markers:
        if any(marker in section for marker in markers):
            return family
    if ac_index in {3, 4}:
        return "strong_gate_unspecified"
    if ac_index in {5, 6}:
        return "boj3s_gate_unspecified"
    return "generic_retry_failure"


def recommended_probe_command(
    ac_index: int,
    failure_family: str,
    primary_axis: str | None,
    probe_signal: ProbeSignal | None = None,
) -> str:
    if ac_index == 2 and failure_family == "smoke_same_worktree_pass_gate_escalation_pending":
        return "./lca_strong_gate.sh"
    if ac_index == 2 and failure_family == "smoke_target_wrapper_readability_failure":
        return "/bin/bash -n ./lca_smoke_target.sh"
    if ac_index in {3, 4}:
        if failure_family == "strong_gate_lock_contention" or probe_signal_is_quick_fail_lock(probe_signal, ac_index):
            return (
                'if [[ -f artifacts/lca_tree_stress_v5/.locks/lca_strong_gate/pid ]]; then '
                'read -r pid < artifacts/lca_tree_stress_v5/.locks/lca_strong_gate/pid && '
                'printf "lock_pid=%s\\n" "$pid"; '
                'ps -p "$pid" -o pid=,ppid=,etime=,command= || true; '
                'pgrep -af "lca_strong_gate\\.sh|branch_certify_suite\\.py --solver .*boj28350_resume/solve"; '
                "else LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh; fi"
            )
        if failure_family == "hard_scaling":
            return "LCA_STAGE_FILTER=hard_scaling ./lca_strong_gate.sh"
        if failure_family == "max_n_mix":
            return "LCA_STAGE_FILTER=max_n_mix ./lca_strong_gate.sh"
        return "LCA_STAGE_FILTER=correctness_fuzz ./lca_strong_gate.sh"
    if ac_index in {5, 6}:
        if failure_family == "boj3s_gate_lock_contention" or probe_signal_is_quick_fail_lock(probe_signal, ac_index):
            return (
                'if [[ -f artifacts/lca_tree_stress_v5/.locks/lca_boj3s_gate/pid ]]; then '
                'read -r pid < artifacts/lca_tree_stress_v5/.locks/lca_boj3s_gate/pid && '
                'printf "lock_pid=%s\\n" "$pid"; '
                'ps -p "$pid" -o pid=,ppid=,etime=,command= || true; '
                'pgrep -af "lca_boj3s_gate\\.sh|branch_certify_suite\\.py --solver .*boj28350_resume/solve"; '
                "else PROFILE_MODE=PROFILE_SAMPLED ./lca_boj3s_gate.sh; fi"
            )
        if primary_axis in {"zero_span_fastpath", "layout_gate"}:
            return "PROFILE_MODE=PROFILE_SAMPLED ./lca_boj3s_gate.sh"
        return "PROFILE_MODE=PROFILE_BASE ./lca_boj3s_gate.sh"
    return "./lca_smoke.sh"


def interpretation_lane_for_ac(ac_index: int) -> str:
    if ac_index in {3, 4}:
        return "correctness-proof"
    if ac_index in {5, 6}:
        return "performance-profile"
    return "pre-gate-stability"


def phase_breakdown_for_lines(lines: Iterable[str]) -> list[PhaseSummary]:
    counter: Counter[str] = Counter()
    samples: dict[str, str] = {}
    for line in lines:
        for phase, markers in PHASE_RULES:
            if any(marker in line for marker in markers):
                counter[phase] += 1
                samples.setdefault(phase, line.strip())
                break
    return [
        PhaseSummary(phase=phase, count=count, sample=samples[phase])
        for phase, count in counter.most_common(6)
    ]


def progress40_axis_breakdown(
    branch_root: Path,
    ac_index: int,
    relevant_lines: Iterable[str],
    structural_focus: list[StructuralFocus],
    clean_log: str,
    certify_rows_summary: dict[str, object] | None = None,
    probe_signal: ProbeSignal | None = None,
    stale_formal_artifacts: set[str] | None = None,
    failure_family: str | None = None,
) -> Progress40AxisSummary:
    relevant_list = list(relevant_lines)
    summary_info = current_progress40_summary(branch_root)
    stale_formal_artifacts = set(stale_formal_artifacts or ())
    failure_family = failure_family or failure_family_for_ac(clean_log, ac_index, branch_root)

    corpus_lines: list[str] = list(relevant_list)
    for hotspot in structural_focus:
        corpus_lines.extend(hotspot.evidence_lines)
        corpus_lines.extend(hotspot.enclosing_symbols)
        if hotspot.code_excerpt:
            corpus_lines.extend(hotspot.code_excerpt.splitlines())

    score: Counter[str] = Counter()
    evidence: dict[str, list[str]] = defaultdict(list)
    for line in corpus_lines:
        lowered = line.lower()
        for axis, markers in AXIS_RULES:
            hits = [marker for marker in markers if marker in lowered]
            if not hits:
                continue
            score[axis] += len(hits)
            if len(evidence[axis]) < 6:
                evidence[axis].append(line.strip())

    primary_axis = None
    secondary_axis = None
    has_direct_axis_hits = bool(score)
    if has_direct_axis_hits:
        ranked = [axis for axis, _ in score.most_common()]
        primary_axis = ranked[0]
        if len(ranked) > 1:
            secondary_axis = ranked[1]
    else:
        primary_axis = summary_info.get("pivot_axis") or DEFAULT_AXIS_BY_AC.get(ac_index)

    if secondary_axis is None:
        for axis in summary_info.get("residual_axes", []):
            if axis and axis != primary_axis:
                secondary_axis = axis
                break

    if ac_index == 2 and not has_direct_axis_hits:
        primary_axis = summary_info.get("pivot_axis") or primary_axis or DEFAULT_AXIS_BY_AC.get(ac_index)
        secondary_axis = None
        if primary_axis:
            evidence[str(primary_axis)].insert(
                0,
                "AC2 stopped in smoke publication before any fresh solver/runtime/profile evidence, so keep the summary pivot only as a parked primary axis and suppress summary-derived secondary carry-forward.",
            )

    if (
        ac_index in {3, 4}
        and "strong_gate" in stale_formal_artifacts
        and not ac_has_direct_certify_trace(relevant_list)
    ):
        primary_axis = summary_info.get("pivot_axis") or primary_axis or DEFAULT_AXIS_BY_AC.get(ac_index)
        secondary_axis = None
        if primary_axis:
            evidence[str(primary_axis)].insert(
                0,
                "The newest strong_gate snapshot predates the failed attempt and no direct certify trace survived, so keep the summary pivot parked until AC3/AC4 emits fresh gate evidence.",
            )
    if (
        ac_index in {5, 6}
        and "boj3s_gate" in stale_formal_artifacts
        and not ac_has_direct_certify_trace(relevant_list)
    ):
        primary_axis = summary_info.get("pivot_axis") or primary_axis or DEFAULT_AXIS_BY_AC.get(ac_index)
        secondary_axis = None
        if primary_axis:
            evidence[str(primary_axis)].insert(
                0,
                "The newest boj3s_gate snapshot predates the failed attempt and no direct certify trace survived, so keep the summary pivot parked until AC5/AC6 emits fresh gate evidence.",
            )

    if probe_signal_is_quick_fail_lock(probe_signal, ac_index):
        primary_axis = summary_info.get("pivot_axis") or primary_axis or DEFAULT_AXIS_BY_AC.get(ac_index)
        secondary_axis = None
    elif probe_signal_is_zero_progress_timeout(probe_signal, ac_index):
        primary_axis = (
            probe_signal.primary_axis
            or summary_info.get("pivot_axis")
            or primary_axis
            or DEFAULT_AXIS_BY_AC.get(ac_index)
        )
        secondary_axis = None
        if probe_signal is not None and probe_signal.secondary_axis and probe_signal.secondary_axis != primary_axis:
            secondary_axis = probe_signal.secondary_axis

    if (
        ac_index in {3, 4}
        and isinstance(certify_rows_summary, dict)
        and certify_rows_summary.get("fresh_for_attempt") is True
        and certify_rows_summary.get("full_lq_timeout_plateaus")
    ):
        primary_axis = summary_info.get("pivot_axis") or primary_axis or DEFAULT_AXIS_BY_AC.get(ac_index)
        secondary_axis = None
        plateau_preview = ", ".join(
            f"{item['mode']} n={item['n']}"
            for item in list(certify_rows_summary.get("full_lq_timeout_plateaus") or [])[:4]
            if isinstance(item, dict)
        )
        if primary_axis:
            axis_note = "fresh current-attempt certify rows already hit full `L/Q` timeout plateaus"
            if plateau_preview:
                axis_note += f" at `{plateau_preview}`"
            axis_note += ", so shuffle-label/query-specific materialization is not the first live discriminator."
            evidence[str(primary_axis)].insert(0, axis_note)

    if ac_index == 2 and failure_family == "smoke_bundle_publication_gap":
        primary_axis = summary_info.get("pivot_axis") or primary_axis or "zero_span_fastpath"
        secondary_axis = None
        evidence = defaultdict(list)
        if primary_axis:
            evidence[primary_axis].append(
                "No fresh solver/runtime/profile evidence was produced; parked the authoritative progress40 pivot behind the AC2 smoke bundle-publication blocker."
            )
            evidence[primary_axis].append(
                "The newest attempt died in smoke bundle publication with missing source_root/source_summary/source_report artifacts, so summary-derived residual axes stay background only."
            )
            if summary_info.get("pivot_text"):
                evidence[primary_axis].append(str(summary_info["pivot_text"]))
    if ac_index == 2 and failure_family == "smoke_manifest_contract_invalid":
        primary_axis = summary_info.get("pivot_axis") or primary_axis or "zero_span_fastpath"
        secondary_axis = None
        evidence = defaultdict(list)
        if primary_axis:
            evidence[primary_axis].append(
                "AC2 stopped in launcher pre-dispatch smoke manifest validation, so no fresh solver/runtime/profile evidence exists for any competing progress40 axis."
            )
            evidence[primary_axis].append(
                "Keep the authoritative progress40 pivot parked behind the smoke manifest locality/validation blocker and do not revive older secondary axes from stale notes."
            )
            if summary_info.get("pivot_text"):
                evidence[primary_axis].append(str(summary_info["pivot_text"]))
    if ac_index == 2 and failure_family == "smoke_dispatch_monitor_helper_failure":
        primary_axis = summary_info.get("pivot_axis") or primary_axis or "zero_span_fastpath"
        secondary_axis = None
        evidence = defaultdict(list)
        if primary_axis:
            evidence[primary_axis].append(
                "AC2 stopped in launcher `dispatch_monitor` before any fresh solver/runtime/profile evidence survived, so keep the authoritative progress40 pivot parked and suppress secondary carry-forward."
            )
            evidence[primary_axis].append(
                "The embedded Python dispatch monitor exited nonzero before it could hand a usable `dispatch_result.txt` back to the launcher, so the next reread stays on the helper result-write/handoff lines rather than widening into solver axes."
            )
            if summary_info.get("pivot_text"):
                evidence[primary_axis].append(str(summary_info["pivot_text"]))
    if ac_index == 2 and failure_family == "smoke_target_wrapper_readability_failure":
        primary_axis = summary_info.get("pivot_axis") or primary_axis or "zero_span_fastpath"
        secondary_axis = None
        evidence = defaultdict(list)
        if primary_axis:
            evidence[primary_axis].append(
                "AC2 stopped in launcher `shell_entrypoint_validation` before any fresh solver/runtime/profile evidence, so keep the authoritative progress40 pivot parked and suppress secondary carry-forward."
            )
            evidence[primary_axis].append(
                "The preserved `smoke_target_wrapper_syntax.stderr.txt` says `Operation canceled` for `lca_smoke_target.sh`, so treat this shape as a smoke-target wrapper readability/access blocker rather than a parsed shell-syntax defect or a solver-owned axis."
            )
            if summary_info.get("pivot_text"):
                evidence[primary_axis].append(str(summary_info["pivot_text"]))
    if failure_family == "transport_disconnected_retry":
        primary_axis = summary_info.get("pivot_axis") or primary_axis or "zero_span_fastpath"
        secondary_axis = None
        evidence = defaultdict(list)
        if primary_axis:
            evidence[primary_axis].append(
                "AC1 through AC6 all failed with the same transport disconnect before any fresh solver/runtime/profile evidence survived, so keep the authoritative progress40 pivot parked and suppress stale secondary axes."
            )
            evidence[primary_axis].append(
                "`latest_attempt_guard.md` downgraded AC3 and AC5 to missing direct gate evidence, which confirms this shape is a transport/trust failure rather than a new solver-owned axis."
            )
            if summary_info.get("pivot_text"):
                evidence[primary_axis].append(str(summary_info["pivot_text"]))

    profile_mode = profile_mode_for_text(clean_log)
    enabled_flags = enabled_flags_for_text(clean_log)

    return Progress40AxisSummary(
        primary_axis=primary_axis,
        secondary_axis=secondary_axis,
        axis_evidence={axis: lines[:6] for axis, lines in evidence.items()},
        profile_mode=profile_mode,
        enabled_flags=enabled_flags[:24],
        last_release_diag_phase=last_release_diag_phase_for_text(clean_log),
        last_progress_checkpoint_phase=last_progress_phase_for_text(clean_log),
        current_summary_pivot=summary_info.get("pivot_text"),
        current_summary_residual_axes=list(summary_info.get("residual_axes", [])),
        failure_family=failure_family,
        next_probe_command=recommended_probe_command(ac_index, failure_family, primary_axis, probe_signal),
        interpretation_lane=interpretation_lane_for_ac(ac_index),
    )


def phase_breakdown(
    relevant_lines: Iterable[str],
    ac_index: int,
    probe_signal: ProbeSignal | None = None,
) -> list[PhaseSummary]:
    return phase_breakdown_for_lines(
        list(relevant_lines) + probe_context_lines_for_ac(probe_signal, ac_index)
    )


def workflow_preflight_trace_lines(clean_log: str) -> list[str]:
    trace_lines: list[str] = []
    for raw_line in clean_log.splitlines():
        line = strip_ansi(raw_line).strip()
        if not line:
            continue
        if any(marker in line for marker in WORKFLOW_PREFLIGHT_TRACE_MARKERS):
            trace_lines.append(line)
    return list(dict.fromkeys(trace_lines))[:80]


def structural_focus_for_ac(
    branch_root: Path,
    clean_log: str,
    ac_index: int,
    analysis_state: dict | None = None,
    probe_signal: ProbeSignal | None = None,
    stale_formal_artifacts: set[str] | None = None,
    failure_family: str | None = None,
    current_attempt_label: str | None = None,
) -> list[StructuralFocus]:
    relevant_lines = collect_ac_context_lines(clean_log, [ac_index])
    probe_context = probe_context_lines_for_ac(probe_signal, ac_index)
    relevant_lines.extend(probe_context)
    file_mentions = extract_file_mentions(relevant_lines)
    trace_mentioned_paths = set(file_mentions.keys())
    trace_boosted_paths: set[str] = set()
    for hinted_path in AC_FILE_HINTS.get(ac_index, []):
        file_mentions.setdefault(hinted_path, 0)
    if ac_index in {5, 6}:
        solver_focus_path = "boj28350_resume/boj28350_branch_3_solver.cpp"
        solver_trace_hits = sum(
            1
            for line in relevant_lines
            if solver_focus_path in line and ("sed -n" in line or "Edit:" in line or "rg -n" in line)
        )
        if solver_trace_hits:
            file_mentions[solver_focus_path] += max(4, solver_trace_hits)
            trace_boosted_paths.add(solver_focus_path)
    inferred_solver_focus = infer_solver_focus_from_trace(relevant_lines, ac_index)
    for solver_path, focus_data in inferred_solver_focus.items():
        file_mentions[solver_path] += max(4, int(focus_data.get("boost", 0)))
        trace_boosted_paths.add(solver_path)
    pinned_paths = []
    pinned_symbols = []
    if isinstance(analysis_state, dict):
        pinned_paths = [str(item) for item in analysis_state.get("pinned_paths", []) if isinstance(item, str)]
        pinned_symbols = [str(item) for item in analysis_state.get("pinned_symbols", []) if isinstance(item, str)]
    for pinned_path in pinned_paths:
        normalized = normalize_path_token(pinned_path)
        if normalized:
            file_mentions[normalized] += 3

    candidate_paths = list(file_mentions.keys())
    focus_ranges = extract_focus_ranges(relevant_lines, candidate_paths)
    for solver_path, focus_data in inferred_solver_focus.items():
        extra_ranges = focus_data.get("ranges", [])
        if extra_ranges:
            focus_ranges.setdefault(solver_path, []).extend(extra_ranges)
            focus_ranges[solver_path] = normalize_focus_range_list(focus_ranges[solver_path])

    inferred_wrapper_path = None
    inferred_wrapper_ranges: list[tuple[int, int]] = []
    inferred_wrapper_notes: list[str] = []
    inferred_wrapper_evidence: list[str] = []
    (
        inferred_wrapper_path,
        inferred_wrapper_ranges,
        inferred_wrapper_notes,
        inferred_wrapper_evidence,
    ) = infer_wrapper_focus_from_trace(branch_root, relevant_lines, ac_index)
    if inferred_wrapper_path:
        file_mentions[inferred_wrapper_path] += 4
        candidate_paths = list(file_mentions.keys())
        if inferred_wrapper_ranges:
            focus_ranges.setdefault(inferred_wrapper_path, []).extend(inferred_wrapper_ranges)
            focus_ranges[inferred_wrapper_path] = normalize_focus_range_list(
                focus_ranges[inferred_wrapper_path]
            )

    if probe_signal_applies_to_ac(probe_signal, ac_index) and probe_signal is not None:
        if probe_signal.wrapper_path:
            file_mentions[probe_signal.wrapper_path] += 4
            candidate_paths = list(file_mentions.keys())
        if probe_signal.wrapper_path and probe_signal.focus_range is not None:
            focus_ranges.setdefault(probe_signal.wrapper_path, []).append(probe_signal.focus_range)
            focus_ranges[probe_signal.wrapper_path] = normalize_focus_range_list(
                focus_ranges[probe_signal.wrapper_path]
            )

    canonical_mentions: Counter[str] = Counter()
    canonical_ranges: dict[str, list[tuple[int, int]]] = defaultdict(list)
    for path, count in file_mentions.items():
        canonical_mentions[canonical_focus_path(branch_root, path)] += count
    for path, ranges in focus_ranges.items():
        canonical_ranges[canonical_focus_path(branch_root, path)].extend(ranges)

    file_mentions = canonical_mentions
    focus_ranges = {
        path: normalize_focus_range_list(ranges)
        for path, ranges in canonical_ranges.items()
    }
    trace_boosted_paths = {
        canonical_focus_path(branch_root, path) for path in trace_boosted_paths
    }
    trace_mentioned_paths = {
        canonical_focus_path(branch_root, path) for path in trace_mentioned_paths
    }
    anchor_specs = ac_retry_anchor_specs(
        ac_index,
        analysis_state=analysis_state,
        probe_signal=probe_signal,
        relevant_lines=relevant_lines,
        stale_formal_artifacts=stale_formal_artifacts,
        failure_family=failure_family,
        current_attempt_label=current_attempt_label,
    )
    anchor_ranges_by_path: dict[str, list[tuple[int, int]]] = defaultdict(list)
    anchor_evidence_by_path: dict[str, list[str]] = defaultdict(list)
    for spec in anchor_specs:
        canonical_path = canonical_focus_path(branch_root, str(spec["path"]))
        anchor_ranges_by_path[canonical_path].append(tuple(spec["range"]))
        for line in spec.get("evidence_lines") or []:
            if isinstance(line, str) and line.strip():
                anchor_evidence_by_path[canonical_path].append(line.strip())
    for path, ranges in anchor_ranges_by_path.items():
        focus_ranges.setdefault(path, []).extend(ranges)
        focus_ranges[path] = normalize_focus_range_list(focus_ranges[path])
    candidate_paths = list(file_mentions.keys())
    hinted_paths = {
        canonical_focus_path(branch_root, path) for path in AC_FILE_HINTS.get(ac_index, [])
    }
    canonical_solver_focus: dict[str, dict[str, object]] = {}
    for path, focus_data in inferred_solver_focus.items():
        canonical_solver_focus[canonical_focus_path(branch_root, path)] = focus_data
    inferred_wrapper_canonical = (
        canonical_focus_path(branch_root, inferred_wrapper_path)
        if inferred_wrapper_path
        else None
    )
    probe_wrapper_canonical = (
        canonical_focus_path(branch_root, probe_signal.wrapper_path)
        if probe_signal_applies_to_ac(probe_signal, ac_index)
        and probe_signal is not None
        and probe_signal.wrapper_path
        else None
    )
    anchor_priority_paths = retry_anchor_priority_paths(
        branch_root,
        ac_index,
        analysis_state=analysis_state,
        probe_signal=probe_signal,
        relevant_lines=relevant_lines,
        stale_formal_artifacts=stale_formal_artifacts,
        failure_family=failure_family,
        current_attempt_label=current_attempt_label,
    )

    ranked_paths = sorted(
        candidate_paths,
        key=lambda path: (
            path in anchor_priority_paths,
            path in canonical_solver_focus,
            len(focus_ranges.get(path, [])),
            path == inferred_wrapper_canonical,
            file_mentions[path],
            path in hinted_paths,
            path,
        ),
        reverse=True,
    )
    stale_formal_artifacts = set(stale_formal_artifacts or ())
    if stale_formal_artifacts:
        direct_focus_paths = set(trace_mentioned_paths)
        direct_focus_paths.update(path for path, ranges in focus_ranges.items() if ranges)
        direct_focus_paths.update(canonical_solver_focus.keys())
        if inferred_wrapper_canonical is not None:
            direct_focus_paths.add(inferred_wrapper_canonical)
        if probe_wrapper_canonical is not None:
            direct_focus_paths.add(probe_wrapper_canonical)
        direct_focus_paths.update(anchor_priority_paths)
        narrowed_ranked_paths = [path for path in ranked_paths if path in direct_focus_paths]
        if inferred_wrapper_canonical is not None:
            wrapper_basename = Path(inferred_wrapper_canonical).name
            narrowed_ranked_paths = [
                path
                for path in narrowed_ranked_paths
                if not (
                    path != inferred_wrapper_canonical
                    and Path(path).name == wrapper_basename
                    and not focus_ranges.get(path)
                )
            ]
        if "strong_gate" in stale_formal_artifacts:
            narrowed_ranked_paths = [
                path
                for path in narrowed_ranked_paths
                if not (
                    Path(path).name == "lca_strong_gate.sh"
                    and "outer_suite_wrappers" not in path
                    and not focus_ranges.get(path)
                )
            ]
        if narrowed_ranked_paths:
            ranked_paths = narrowed_ranked_paths

    focuses: list[StructuralFocus] = []
    for path in ranked_paths[:8]:
        resolved = resolve_repo_path(branch_root, path)
        if resolved is None or resolved.suffix not in CODE_SUFFIXES:
            continue
        ranges = focus_ranges.get(path, [])
        symbol_entries = select_symbol_entries(symbol_ranges_for_path(resolved), ranges)
        symbols = [f"{entry[2]} {entry[3]} [{entry[0]}-{entry[1]}]" for entry in symbol_entries]
        evidence_lines = evidence_lines_for_path(relevant_lines, path)
        for line in anchor_evidence_by_path.get(path, []):
            if line not in evidence_lines:
                evidence_lines.append(line)
        if inferred_wrapper_canonical is not None and path == inferred_wrapper_canonical:
            for line in inferred_wrapper_evidence:
                if line not in evidence_lines:
                    evidence_lines.append(line)
        if path in canonical_solver_focus:
            for line in canonical_solver_focus[path].get("evidence", []):
                if line not in evidence_lines:
                    evidence_lines.append(line)
        if probe_wrapper_canonical is not None and path == probe_wrapper_canonical:
            for line in probe_context:
                if line not in evidence_lines:
                    evidence_lines.append(line)
            evidence_lines = evidence_lines[:6]
        code_excerpt = code_excerpt_for_focus(resolved, ranges, symbol_entries)
        mtime = safe_mtime_label(resolved)
        note_parts = []
        if file_mentions[path] > 0:
            note_parts.append("observed in failed-AC trace")
        if path in hinted_paths:
            note_parts.append("mapped from failed AC semantics")
        if path in anchor_priority_paths:
            note_parts.append("promoted by retry-critical anchor hints")
        if path in trace_boosted_paths:
            note_parts.append("boosted by solver-trace concentration")
        if path in canonical_solver_focus:
            note_parts.extend(canonical_solver_focus[path].get("notes", []))
        if path in pinned_paths or str(resolved) in pinned_paths:
            note_parts.append("boosted by failure_analysis_state")
        if inferred_wrapper_canonical is not None and path == inferred_wrapper_canonical:
            note_parts.extend(inferred_wrapper_notes)
        if probe_wrapper_canonical is not None and path == probe_wrapper_canonical:
            note_parts.append("narrowed by latest_next_probe_result quick-fail lock evidence")
            if probe_signal.focus_symbol and probe_signal.focus_symbol in symbols:
                note_parts.append(f"matched probe focus symbol `{probe_signal.focus_symbol}`")
        for pinned_symbol in pinned_symbols:
            if any(pinned_symbol in symbol for symbol in symbols):
                note_parts.append(f"matched pinned symbol `{pinned_symbol}`")
                break
        focuses.append(
            StructuralFocus(
                path=str(resolved),
                observed_mentions=file_mentions[path],
                focus_ranges=[range_label(start, end) for start, end in ranges],
                enclosing_symbols=symbols,
                evidence_lines=evidence_lines,
                code_excerpt=code_excerpt,
                note=", ".join(note_parts) or "mapped from failure context",
                mtime=mtime,
            )
        )
    return focuses


def artifact_snapshots_for_failed_acs(
    branch_root: Path,
    ac_numbers: Iterable[int],
    attempt_started_at: datetime | None = None,
) -> list[ArtifactSnapshot]:
    seen: set[str] = set()
    snapshots: list[ArtifactSnapshot] = []
    for ac_index in ac_numbers:
        for label, relative_root in AC_ARTIFACT_HINTS.get(ac_index, []):
            key = f"{label}:{relative_root}"
            if key in seen:
                continue
            seen.add(key)
            snapshots.append(
                build_artifact_snapshot(
                    label,
                    branch_root / relative_root,
                    attempt_started_at=attempt_started_at,
                )
            )
    return snapshots


def stale_formal_artifact_labels(snapshots: Iterable[ArtifactSnapshot]) -> set[str]:
    freshness_by_label: defaultdict[str, list[bool | None]] = defaultdict(list)
    for snapshot in snapshots:
        label = effective_formal_artifact_label(snapshot)
        if label in FORMAL_ARTIFACT_LABELS:
            freshness_by_label[label].append(snapshot.fresh_for_attempt)

    stale_labels: set[str] = set()
    for label, freshness_flags in freshness_by_label.items():
        if any(flag is False for flag in freshness_flags) and not any(
            flag is True for flag in freshness_flags
        ):
            stale_labels.add(label)
    return stale_labels


def effective_formal_artifact_label(snapshot: ArtifactSnapshot) -> str | None:
    if snapshot.label in FORMAL_ARTIFACT_LABELS:
        return snapshot.label

    if snapshot.summary_excerpt:
        stripped = snapshot.summary_excerpt.lstrip()
        if stripped.startswith("{"):
            try:
                payload = json.loads(snapshot.summary_excerpt)
            except json.JSONDecodeError:
                payload = None
            if isinstance(payload, dict):
                preset = str(payload.get("preset") or "").strip()
                if preset in FORMAL_ARTIFACT_LABELS:
                    return preset

    for raw_path in (snapshot.summary_file, snapshot.latest_file):
        if not raw_path or raw_path == "none":
            continue
        lowered = raw_path.lower()
        if "strong_gate" in lowered:
            return "strong_gate"
        if "boj3s_gate" in lowered:
            return "boj3s_gate"
        if "diag_bg_wrapper" in lowered and "certify" in lowered:
            return "strong_gate"

    return None


def parse_csv_bool(value: str | None) -> bool:
    return str(value or "").strip().lower() in {"1", "true", "yes"}


def certify_row_bucket(row: dict[str, str]) -> str:
    if parse_csv_bool(row.get("timed_out")):
        return "timeout"
    if parse_csv_bool(row.get("val_ok")):
        return "pass"
    return "re_wa"


def certify_bucket_sort_key(item: tuple[tuple[str, ...], int]) -> tuple[object, ...]:
    key, count = item
    bucket = key[-1]
    mode = key[0] if key else ""
    try:
        n_value = int(key[1]) if len(key) > 1 else 0
    except ValueError:
        n_value = 0
    tail = tuple(key[2:-1])
    return (-count, CERTIFY_BUCKET_ORDER.get(bucket, 99), mode, -n_value, tail)


def find_certify_rows_path(snapshot: ArtifactSnapshot) -> Path | None:
    candidates: list[Path] = []
    for raw_path in (snapshot.latest_file, snapshot.summary_file):
        if not raw_path or raw_path == "none":
            continue
        path = Path(raw_path)
        if path.name == "certify_rows.csv":
            candidates.append(path)
        else:
            candidates.append(path.with_name("certify_rows.csv"))
    seen: set[str] = set()
    for candidate in candidates:
        key = str(candidate)
        if key in seen:
            continue
        seen.add(key)
        if candidate.exists():
            return candidate
    return None


def summarize_certify_rows_snapshot(snapshot: ArtifactSnapshot) -> dict[str, object] | None:
    rows_path = find_certify_rows_path(snapshot)
    if rows_path is None:
        return None
    effective_label = effective_formal_artifact_label(snapshot) or snapshot.label
    try:
        with rows_path.open(newline="", errors="replace") as handle:
            rows = [
                {str(key or "").strip(): str(value or "").strip() for key, value in row.items()}
                for row in csv.DictReader(handle)
            ]
    except OSError:
        return None
    if not rows:
        return None

    bucket_counts: Counter[str] = Counter()
    mode_n_counts: Counter[tuple[str, str, str]] = Counter()
    mode_n_lq_counts: Counter[tuple[str, str, str, str, str]] = Counter()
    label_split_counts: defaultdict[tuple[str, str, str], Counter[str]] = defaultdict(Counter)
    quadrant_counts: defaultdict[tuple[str, str, str, str], Counter[str]] = defaultdict(Counter)
    timeout_solver_rc_counts: Counter[str] = Counter()
    pass_frontier: list[tuple[float, str, str, str, str, str]] = []

    for row in rows:
        bucket = certify_row_bucket(row)
        bucket_counts[bucket] += 1
        mode = row.get("mode", "?")
        n_value = row.get("n", "?")
        shuffle_labels = row.get("shuffle_labels", "?")
        shuffle_queries = row.get("shuffle_queries", "?")
        label_split_counts[(mode, n_value, shuffle_labels)][bucket] += 1
        quadrant_counts[(mode, n_value, shuffle_labels, shuffle_queries)][bucket] += 1
        if bucket != "pass":
            mode_n_counts[(mode, n_value, bucket)] += 1
            mode_n_lq_counts[(mode, n_value, shuffle_labels, shuffle_queries, bucket)] += 1
            if bucket == "timeout":
                timeout_solver_rc_counts[row.get("solver_rc", "?") or "?"] += 1
            continue
        try:
            sec = float(row.get("sec") or "")
        except ValueError:
            continue
        pass_frontier.append(
            (
                sec,
                mode,
                n_value,
                shuffle_labels,
                shuffle_queries,
                row.get("seed", "?"),
            )
        )

    label_sensitive_clusters: list[dict[str, object]] = []
    for (mode, n_value, shuffle_labels), counts in label_split_counts.items():
        if shuffle_labels != "1":
            continue
        fail_count = counts["timeout"] + counts["re_wa"]
        if fail_count <= 0:
            continue
        baseline = label_split_counts.get((mode, n_value, "0"))
        if not baseline or baseline["pass"] <= 0:
            continue
        label_sensitive_clusters.append(
            {
                "mode": mode,
                "n": n_value,
                "failures_at_l1": fail_count,
                "timeouts_at_l1": counts["timeout"],
                "re_wa_at_l1": counts["re_wa"],
                "passes_at_l0": baseline["pass"],
                "timeouts_at_l0": baseline["timeout"],
                "re_wa_at_l0": baseline["re_wa"],
            }
        )
    label_sensitive_clusters.sort(
        key=lambda item: (
            -int(item["failures_at_l1"]),
            -int(item["timeouts_at_l1"]),
            str(item["mode"]),
            -int(item["n"]),
        )
    )

    full_lq_timeout_plateaus: list[dict[str, object]] = []
    near_lq_timeout_plateaus: list[dict[str, object]] = []
    first_timeout_onsets: list[dict[str, object]] = []
    mode_to_n_values: defaultdict[str, set[str]] = defaultdict(set)
    for mode, n_value, _, _ in quadrant_counts:
        mode_to_n_values[mode].add(n_value)
    for mode in sorted(mode_to_n_values):
        recorded_full = False
        recorded_near = False
        recorded_any = False
        for n_value in sorted(mode_to_n_values[mode], key=lambda item: int(item)):
            timeout_total = 0
            pass_total = 0
            re_wa_total = 0
            full_timeout = True
            for shuffle_labels in ("0", "1"):
                for shuffle_queries in ("0", "1"):
                    counts = quadrant_counts[(mode, n_value, shuffle_labels, shuffle_queries)]
                    timeout_total += counts["timeout"]
                    pass_total += counts["pass"]
                    re_wa_total += counts["re_wa"]
                    if counts["timeout"] < 5:
                        full_timeout = False
            if not recorded_any and timeout_total > 0:
                first_timeout_onsets.append(
                    {
                        "mode": mode,
                        "n": n_value,
                        "timeout_total": timeout_total,
                        "pass_total": pass_total,
                        "re_wa_total": re_wa_total,
                    }
                )
                recorded_any = True
            if not recorded_near and timeout_total >= 18 and pass_total > 0:
                near_lq_timeout_plateaus.append(
                    {
                        "mode": mode,
                        "n": n_value,
                        "timeout_total": timeout_total,
                        "pass_total": pass_total,
                        "re_wa_total": re_wa_total,
                    }
                )
                recorded_near = True
            if not recorded_full and full_timeout:
                full_lq_timeout_plateaus.append(
                    {
                        "mode": mode,
                        "n": n_value,
                        "timeout_total": timeout_total,
                        "pass_total": pass_total,
                        "re_wa_total": re_wa_total,
                    }
                )
                recorded_full = True
            if recorded_full and recorded_near:
                break

    full_lq_timeout_onset_groups: list[dict[str, object]] = []
    grouped_full_lq_timeout_onsets: defaultdict[str, list[str]] = defaultdict(list)
    for item in full_lq_timeout_plateaus:
        grouped_full_lq_timeout_onsets[str(item["n"])].append(str(item["mode"]))
    for n_value in sorted(grouped_full_lq_timeout_onsets, key=lambda item: int(item)):
        full_lq_timeout_onset_groups.append(
            {
                "n": n_value,
                "modes": sorted(grouped_full_lq_timeout_onsets[n_value]),
                "mode_count": len(grouped_full_lq_timeout_onsets[n_value]),
            }
        )

    full_lq_timeout_by_mode = {
        str(item["mode"]): item for item in full_lq_timeout_plateaus if isinstance(item, dict)
    }
    pre_full_lq_timeout_onsets: list[dict[str, object]] = []
    for item in first_timeout_onsets:
        full_item = full_lq_timeout_by_mode.get(str(item["mode"]))
        if not full_item:
            continue
        try:
            first_timeout_n = int(str(item["n"]))
            first_full_n = int(str(full_item["n"]))
        except ValueError:
            continue
        if first_timeout_n >= first_full_n:
            continue
        pre_full_lq_timeout_onsets.append(
            {
                "mode": item["mode"],
                "first_timeout_n": item["n"],
                "first_timeout_total": item["timeout_total"],
                "first_timeout_pass_total": item["pass_total"],
                "first_timeout_re_wa_total": item["re_wa_total"],
                "first_full_lq_plateau_n": full_item["n"],
            }
        )
    pre_full_lq_timeout_onsets.sort(
        key=lambda item: (
            int(str(item["first_timeout_n"])),
            str(item["mode"]),
        )
    )

    return {
        "label": effective_label,
        "source_label": snapshot.label,
        "rows_path": str(rows_path),
        "fresh_for_attempt": snapshot.fresh_for_attempt,
        "row_count": len(rows),
        "bucket_counts": {bucket: bucket_counts.get(bucket, 0) for bucket in ("pass", "timeout", "re_wa")},
        "timeout_solver_rc_counts": [
            {
                "solver_rc": solver_rc,
                "count": count,
            }
            for solver_rc, count in sorted(
                timeout_solver_rc_counts.items(),
                key=lambda item: (-item[1], item[0]),
            )
        ],
        "full_lq_timeout_onset_groups": full_lq_timeout_onset_groups[:6],
        "pre_full_lq_timeout_onsets": pre_full_lq_timeout_onsets[:8],
        "full_lq_timeout_plateaus": full_lq_timeout_plateaus[:8],
        "near_lq_timeout_plateaus": near_lq_timeout_plateaus[:6],
        "mode_n_clusters": [
            {
                "mode": mode,
                "n": n_value,
                "bucket": bucket,
                "count": count,
            }
            for (mode, n_value, bucket), count in sorted(
                mode_n_counts.items(),
                key=certify_bucket_sort_key,
            )[:8]
        ],
        "mode_n_lq_clusters": [
            {
                "mode": mode,
                "n": n_value,
                "shuffle_labels": shuffle_labels,
                "shuffle_queries": shuffle_queries,
                "bucket": bucket,
                "count": count,
            }
            for (mode, n_value, shuffle_labels, shuffle_queries, bucket), count in sorted(
                mode_n_lq_counts.items(),
                key=certify_bucket_sort_key,
            )[:12]
        ],
        "label_sensitive_clusters": label_sensitive_clusters[:6],
        "pass_frontier": [
            {
                "sec": round(sec, 6),
                "mode": mode,
                "n": n_value,
                "shuffle_labels": shuffle_labels,
                "shuffle_queries": shuffle_queries,
                "seed": seed,
            }
            for sec, mode, n_value, shuffle_labels, shuffle_queries, seed in sorted(
                pass_frontier,
                reverse=True,
            )[:10]
        ],
    }


def certify_rows_summary_for_ac(snapshots: Iterable[ArtifactSnapshot]) -> dict[str, object] | None:
    candidates: list[dict[str, object]] = []
    for snapshot in snapshots:
        effective_label = effective_formal_artifact_label(snapshot)
        if effective_label not in FORMAL_ARTIFACT_LABELS:
            continue
        summary = summarize_certify_rows_snapshot(snapshot)
        if summary is not None:
            summary["label"] = effective_label
            candidates.append(summary)
    if not candidates:
        return None
    candidates.sort(
        key=lambda item: (
            item.get("fresh_for_attempt") is not True,
            str(item.get("label") or ""),
            str(item.get("source_label") or ""),
        )
    )
    return candidates[0]


def refine_failure_family_with_certify_rows(
    ac_index: int,
    failure_family: str,
    certify_rows_summary: dict[str, object] | None,
) -> str:
    if not certify_rows_summary or certify_rows_summary.get("fresh_for_attempt") is not True:
        return failure_family
    bucket_counts = certify_rows_summary.get("bucket_counts")
    if not isinstance(bucket_counts, dict):
        return failure_family
    timeout_count = int(bucket_counts.get("timeout") or 0)
    re_wa_count = int(bucket_counts.get("re_wa") or 0)
    if ac_index in {3, 4}:
        if timeout_count > 0 and re_wa_count > 0:
            return "strong_gate_timeout_re_wa_cluster"
        if timeout_count > 0:
            return "strong_gate_timeout_cluster"
        if re_wa_count > 0:
            return "strong_gate_re_wa_cluster"
    if ac_index in {5, 6}:
        if timeout_count > 0 and re_wa_count > 0:
            return "boj3s_gate_timeout_re_wa_cluster"
        if timeout_count > 0:
            return "boj3s_gate_timeout_cluster"
        if re_wa_count > 0:
            return "boj3s_gate_re_wa_cluster"
    return failure_family


def refine_failure_family_with_artifact_freshness(
    ac_index: int,
    failure_family: str,
    certify_rows_summary: dict[str, object] | None,
    relevant_lines: Iterable[str],
    stale_formal_artifacts: set[str] | None,
) -> str:
    stale_formal_artifacts = set(stale_formal_artifacts or ())
    has_fresh_rows = (
        isinstance(certify_rows_summary, dict)
        and certify_rows_summary.get("fresh_for_attempt") is True
    )
    has_direct_certify_trace = ac_has_direct_certify_trace(relevant_lines)
    if (
        ac_index in {3, 4}
        and failure_family == "strong_gate_unspecified"
        and "strong_gate" in stale_formal_artifacts
        and not has_fresh_rows
        and not has_direct_certify_trace
    ):
        return "strong_gate_pre_artifact_stall"
    if (
        ac_index in {5, 6}
        and failure_family == "boj3s_gate_unspecified"
        and "boj3s_gate" in stale_formal_artifacts
        and not has_fresh_rows
        and not has_direct_certify_trace
    ):
        return "boj3s_gate_pre_artifact_stall"
    return failure_family


def ac_has_direct_certify_trace(relevant_lines: Iterable[str]) -> bool:
    for raw_line in relevant_lines:
        line = raw_line.strip()
        lowered = line.lower()
        if any(marker in lowered for marker in CERTIFY_RUNTIME_MARKERS):
            return True
        if not any(marker in lowered for marker in ("branch_certify_suite.py", "certify_suite.py", "run_certify_suite")):
            continue
        if any(marker in lowered for marker in READ_ONLY_TRACE_MARKERS):
            continue
        if " --solver" in lowered or " --preset" in lowered:
            return True
        if "python3 " in lowered and ("branch_certify_suite.py" in lowered or "certify_suite.py" in lowered):
            return True
    return False


def is_pre_artifact_fallback_anchor(
    relative_path: str,
    focus_range: tuple[int, int],
    stale_formal_artifacts: set[str],
) -> bool:
    start, end = focus_range
    if "strong_gate" in stale_formal_artifacts and relative_path == "outer_suite_wrappers/lca_strong_gate.sh":
        return not (end < 337 or start > 366)
    return False


def ac_subset(items: Iterable[tuple[str, str]], indices: set[int]) -> list[tuple[str, str]]:
    subset: list[tuple[str, str]] = []
    for index, text in items:
        try:
            ac_index = int(index)
        except ValueError:
            continue
        if ac_index in indices:
            subset.append((index, text))
    return subset


def supporting_ac_subset(
    passed_acs: Iterable[tuple[str, str]],
    failed_acs: Iterable[tuple[str, str]],
    blocked_acs: Iterable[tuple[str, str]],
) -> list[tuple[str, str]]:
    excluded = FORMAL_ACCEPTANCE_ACS | DIAGNOSTIC_ONLY_ACS
    all_items = list(passed_acs) + list(failed_acs) + list(blocked_acs)
    return ac_subset(all_items, {int(index) for index, _ in all_items if int(index) not in excluded})


def summarize_formal_acceptance(
    passed_acs: Iterable[tuple[str, str]],
    failed_acs: Iterable[tuple[str, str]],
    blocked_acs: Iterable[tuple[str, str]],
) -> FormalAcceptanceSummary:
    passed = ac_subset(passed_acs, FORMAL_ACCEPTANCE_ACS)
    failed = ac_subset(failed_acs, FORMAL_ACCEPTANCE_ACS)
    blocked = ac_subset(blocked_acs, FORMAL_ACCEPTANCE_ACS)
    seen = {int(index) for index, _ in passed + failed + blocked}
    missing = sorted(FORMAL_ACCEPTANCE_ACS - seen)
    if failed:
        verdict = "FAIL"
    elif blocked:
        verdict = "BLOCKED"
    elif not missing:
        verdict = "PASS"
    else:
        verdict = "INCOMPLETE"
    return FormalAcceptanceSummary(
        verdict=verdict,
        required_ac_indices=sorted(FORMAL_ACCEPTANCE_ACS),
        passed=passed,
        failed=failed,
        blocked=blocked,
        missing=missing,
        excluded_diagnostic=ac_subset(passed_acs, DIAGNOSTIC_ONLY_ACS),
    )


def group_artifact_snapshots(
    snapshots: Iterable[ArtifactSnapshot],
) -> dict[str, list[ArtifactSnapshot]]:
    grouped = {"formal": [], "supporting": [], "diagnostic": []}
    for snapshot in snapshots:
        if snapshot.label in FORMAL_ARTIFACT_LABELS:
            grouped["formal"].append(snapshot)
        elif snapshot.label in DIAGNOSTIC_ARTIFACT_LABELS:
            grouped["diagnostic"].append(snapshot)
        else:
            grouped["supporting"].append(snapshot)
    return grouped


def format_ac_pairs(items: Iterable[tuple[str, str]]) -> list[str]:
    return [f"AC {index}: {text.strip()}" for index, text in items]


def append_snapshot_section(
    lines: list[str],
    title: str,
    snapshots: Iterable[ArtifactSnapshot],
    *,
    note: str | None = None,
    empty_text: str,
) -> None:
    lines.append(f"## {title}")
    lines.append("")
    if note:
        lines.append(f"- {note}")
        lines.append("")
    materialized = list(snapshots)
    if not materialized:
        lines.append(f"- {empty_text}")
        lines.append("")
        return
    for snapshot in materialized:
        lines.append(f"### {snapshot.label}")
        lines.append("")
        lines.append(f"- Latest file: `{snapshot.latest_file or 'none'}`")
        lines.append(f"- Latest mtime: `{snapshot.latest_mtime or 'unknown'}`")
        lines.append(f"- Summary file: `{snapshot.summary_file or 'none'}`")
        if snapshot.attempt_start:
            lines.append(f"- Attempt start: `{snapshot.attempt_start}`")
            freshness_value = "unknown"
            if snapshot.fresh_for_attempt is True:
                freshness_value = "yes"
            elif snapshot.fresh_for_attempt is False:
                freshness_value = "no"
            lines.append(f"- Fresh within attempt: `{freshness_value}`")
        if snapshot.freshness_note:
            lines.append(f"- Freshness note: {snapshot.freshness_note}")
        if snapshot.summary_excerpt:
            lines.append("")
            lines.append("```text")
            lines.append(snapshot.summary_excerpt.strip())
            lines.append("```")
        lines.append("")


def load_json(path: Path) -> dict | list | None:
    if not path.exists():
        return None
    try:
        return json.loads(path.read_text(errors="replace"))
    except (OSError, json.JSONDecodeError):
        return None


def build_refinement_notes(
    current_failed_acs: list[tuple[str, str]],
    current_breakdowns: list[dict],
    previous_report: dict | None,
    previous_breakdown: dict | None,
    probe_signal: ProbeSignal | None = None,
) -> list[str]:
    notes: list[str] = []
    if not previous_report and not previous_breakdown:
        notes.append("No prior failure analysis was available, so this breakdown becomes the first baseline for refinement.")
        return notes

    previous_failed = {item[0] for item in (previous_report or {}).get("failed_acs", []) if isinstance(item, list)}
    current_failed = {item[0] for item in current_failed_acs}
    repeated = sorted(previous_failed & current_failed)
    if repeated:
        notes.append(f"Repeated failed AC(s) versus the previous attempt: {', '.join(repeated)}.")
    else:
        notes.append("The failed AC set changed relative to the previous captured failure.")

    previous_breakdowns_list = (previous_breakdown or {}).get("failed_ac_breakdowns", [])
    previous_types = {
        item.get("ac_index"): item.get("failure_type")
        for item in previous_breakdowns_list
        if isinstance(item, dict)
    }
    for breakdown in current_breakdowns:
        previous_type = previous_types.get(breakdown["ac_index"])
        current_type = breakdown["failure_type"]
        if previous_type == current_type and previous_type is not None:
            notes.append(
                f"AC {breakdown['ac_index']} repeated the same failure classification `{current_type}`."
            )
        elif previous_type is not None:
            notes.append(
                f"AC {breakdown['ac_index']} changed failure classification from `{previous_type}` to `{current_type}`."
            )

    current_hotspots = {
        hotspot["path"]
        for breakdown in current_breakdowns
        for hotspot in breakdown["structural_focus"]
    }
    previous_hotspots = {
        hotspot.get("path")
        for breakdown in previous_breakdowns_list
        if isinstance(breakdown, dict)
        for hotspot in breakdown.get("structural_focus", [])
        if isinstance(hotspot, dict)
    }
    overlap = sorted(path for path in current_hotspots & previous_hotspots if path)
    if overlap:
        notes.append("Recurring code-structure hotspots: " + ", ".join(overlap[:6]) + ".")
    new_hotspots = sorted(path for path in current_hotspots - previous_hotspots if path)
    if new_hotspots:
        notes.append("New hotspots to fold into the next retry analysis: " + ", ".join(new_hotspots[:6]) + ".")

    current_symbols = {
        symbol
        for breakdown in current_breakdowns
        for hotspot in breakdown["structural_focus"]
        for symbol in hotspot["enclosing_symbols"]
    }
    previous_symbols = {
        symbol
        for breakdown in previous_breakdowns_list
        if isinstance(breakdown, dict)
        for hotspot in breakdown.get("structural_focus", [])
        if isinstance(hotspot, dict)
        for symbol in hotspot.get("enclosing_symbols", [])
        if isinstance(symbol, str)
    }
    recurring_symbols = sorted(current_symbols & previous_symbols)
    if recurring_symbols:
        notes.append("Recurring enclosing symbols: " + ", ".join(recurring_symbols[:6]) + ".")

    current_ranges = {
        f"{hotspot['path']}:{focus_range}"
        for breakdown in current_breakdowns
        for hotspot in breakdown["structural_focus"]
        for focus_range in hotspot["focus_ranges"]
    }
    previous_ranges = {
        f"{hotspot.get('path')}:{focus_range}"
        for breakdown in previous_breakdowns_list
        if isinstance(breakdown, dict)
        for hotspot in breakdown.get("structural_focus", [])
        if isinstance(hotspot, dict)
        for focus_range in hotspot.get("focus_ranges", [])
        if isinstance(focus_range, str)
    }
    recurring_ranges = sorted(current_ranges & previous_ranges)
    if recurring_ranges:
        notes.append("Recurring line-range hotspots: " + ", ".join(recurring_ranges[:6]) + ".")

    previous_primary_axes = {
        item.get("ac_index"): item.get("primary_axis")
        for item in previous_breakdowns_list
        if isinstance(item, dict)
    }
    for breakdown in current_breakdowns:
        current_axis = breakdown.get("primary_axis")
        previous_axis = previous_primary_axes.get(breakdown["ac_index"])
        if previous_axis and current_axis == previous_axis:
            notes.append(
                f"AC {breakdown['ac_index']} kept the same primary progress40 axis `{current_axis}`."
            )
        elif previous_axis and current_axis:
            notes.append(
                f"AC {breakdown['ac_index']} shifted primary progress40 axis from `{previous_axis}` to `{current_axis}`."
            )

    recurring_families = [
        breakdown["failure_family"]
        for breakdown in current_breakdowns
        if breakdown.get("failure_family")
        and any(
            isinstance(prev, dict)
            and prev.get("ac_index") == breakdown["ac_index"]
            and prev.get("failure_family") == breakdown["failure_family"]
            for prev in previous_breakdowns_list
        )
    ]
    if recurring_families:
        notes.append(
            "Recurring failure families: " + ", ".join(dict.fromkeys(recurring_families)) + "."
        )

    for breakdown in current_breakdowns:
        stale_formal_artifacts = [
            snapshot["label"]
            for snapshot in breakdown.get("artifact_snapshots", [])
            if snapshot.get("label") in FORMAL_ARTIFACT_LABELS
            and snapshot.get("fresh_for_attempt") is False
        ]
        if stale_formal_artifacts:
            notes.append(
                f"AC {breakdown['ac_index']} never refreshed its "
                + ", ".join(dict.fromkeys(stale_formal_artifacts))
                + " artifact root during the failed attempt; treat this as a pre-artifact stall in the wrapper/build corridor."
            )

    top_phases = []
    for breakdown in current_breakdowns:
        for phase in breakdown["phase_summaries"][:2]:
            top_phases.append(phase["phase"])
    if top_phases:
        notes.append(
            "Refine the next retry around these dominant phases: "
            + ", ".join(dict.fromkeys(top_phases))
            + "."
        )

    if probe_signal is not None and probe_signal.active_gate and probe_signal.wrapper_path:
        range_text = (
            range_label(*probe_signal.focus_range)
            if probe_signal.focus_range is not None
            else "unknown-range"
        )
        notes.append(
            "Latest probe narrowed the next retry to "
            f"`{probe_signal.wrapper_path}:{range_text}`"
            + (
                f" inside `{probe_signal.focus_symbol}`"
                if probe_signal.focus_symbol
                else ""
            )
            + "."
        )

    return notes[:8]


def non_ac_trace_excerpt(clean_log: str) -> list[str]:
    selected: list[str] = []
    for raw_line in clean_log.splitlines():
        line = strip_ansi(raw_line).strip()
        if not line:
            continue
        if (
            line == "Traceback (most recent call last):"
            or line.startswith('File "')
            or "ValueError:" in line
            or "Invalid seed format" in line
            or "attempt guard" in line.lower()
            or "git_repo_health" in line
        ):
            selected.append(line)
    if selected:
        return selected[-80:]
    tail = [strip_ansi(line).strip() for line in clean_log.splitlines()[-40:]]
    return [line for line in tail if line]


def fallback_probe_command_for_non_ac_failure(clean_log: str) -> str:
    lowered = clean_log.lower()
    if "soft_stop_request.json" in lowered and "output path must stay under" in lowered:
        return (
            "python3 - <<'PY'\n"
            "from pathlib import Path\n"
            "import yaml\n"
            "from artifact_paths import ensure_under_artifacts\n"
            "attempt = Path('artifacts/lca_tree_stress_v5/retry_loop/attempt_003_20260328_080642')\n"
            "files = sorted(p.name for p in attempt.iterdir() if p.is_file())\n"
            "print('attempt_files', files)\n"
            "print('has_runtime_snapshot', any('snapshot' in name for name in files))\n"
            "print('has_soft_stop_artifact', any('soft_stop' in name for name in files))\n"
            "payload = yaml.safe_load(Path('.ouroboros/seed_branch3_progress40_research_loop.yaml').read_text())\n"
            "print('seed_type', type(payload).__name__)\n"
            "print('seed_is_dict', isinstance(payload, dict))\n"
            "branch_root = Path('.').resolve()\n"
            "for candidate in ['artifacts/lca_tree_stress_v5/retry_loop/soft_stop_request.json', '.ouroboros/soft_stop_request.json']:\n"
            "    resolved = (branch_root / candidate).resolve()\n"
            "    try:\n"
            "        print(candidate, 'OK', ensure_under_artifacts(resolved))\n"
            "    except Exception as exc:\n"
            "        print(candidate, 'FAIL', exc)\n"
            "PY\n"
            "&& rg -n '\\.ouroboros/soft_stop_request\\.json|Invalid seed format' "
            "artifacts/lca_tree_stress_v5/retry_loop/attempt_003_20260328_080642/workflow.log "
            "artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md && "
            "nl -ba .ouroboros/run_until_pass_progress40.sh | sed -n '13,13p' && "
            "nl -ba .ouroboros/run_until_pass_progress40.sh | sed -n '84,84p' && "
            "nl -ba .ouroboros/run_until_pass_progress40.sh | sed -n '98,98p' && "
            "nl -ba .ouroboros/monitor_codex_quota.py | sed -n '21,22p' && "
            "nl -ba .ouroboros/monitor_codex_quota.py | sed -n '55,55p' && "
            "nl -ba .ouroboros/monitor_codex_quota.py | sed -n '428,428p' && "
            "nl -ba .ouroboros/request_soft_stop.py | sed -n '17,18p' && "
            "nl -ba test_retry_loop_artifact_locality.py | sed -n '151,165p' && "
            "nl -ba artifact_paths.py | sed -n '233,235p'"
        )
    return "tail -n 80 artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md"


def workflow_preflight_soft_stop_defaults_confirmed(branch_root: Path) -> bool:
    for relative_path, token in WORKFLOW_PREFLIGHT_PROVENANCE_EXPECTATIONS:
        path = resolve_repo_path(branch_root, relative_path)
        if path is None:
            return False
        try:
            text = path.read_text(errors="replace")
        except OSError:
            return False
        if token not in text:
            return False
    return True


def strongest_non_ac_failure_hypothesis(
    clean_log: str,
    *,
    soft_stop_path_guard: bool,
    soft_stop_defaults_confirmed: bool,
) -> str | None:
    lowered = clean_log.lower()
    invalid_seed = "invalid seed format" in lowered
    if soft_stop_path_guard and soft_stop_defaults_confirmed:
        hypothesis = (
            "Stale runtime `--soft-stop-file` argv provenance is the strongest surviving hypothesis: "
            "attempt_003 reached `.ouroboros/monitor_codex_quota.py` with `.ouroboros/soft_stop_request.json` "
            "even though the live shell assignment, watchdog default, manual helper default, and "
            "artifact-locality regression coverage still point under "
            "`artifacts/lca_tree_stress_v5/retry_loop/`."
        )
        if invalid_seed:
            hypothesis += (
                " Treat `Invalid seed format` as a secondary workflow-launch handoff symptom at "
                "`.ouroboros/run_until_pass_progress40.sh [84-84]` until the soft-stop argv source "
                "is proven clean."
            )
        return hypothesis
    if soft_stop_path_guard:
        hypothesis = (
            "Artifact-locality mismatch at the quota-watchdog soft-stop path is the strongest surviving "
            "hypothesis: the runtime resolved `.ouroboros/soft_stop_request.json` outside `artifacts/`, "
            "so the next probe should prove whether this came from a stale override, path rewrite, or helper drift."
        )
        if invalid_seed:
            hypothesis += (
                " Keep `Invalid seed format` secondary until the soft-stop path mismatch is resolved."
            )
        return hypothesis
    if invalid_seed:
        return (
            "Workflow seed CLI handoff is the strongest surviving hypothesis: the on-disk seed must be "
            "rechecked as a mapping, then the exact `ouroboros run workflow \"$workflow_seed\" --runtime codex` "
            "launch line should be treated as the primary boundary."
        )
    return None


def format_binary_size(value: int | None) -> str | None:
    if value is None:
        return None
    size = float(value)
    for unit in ("B", "KiB", "MiB", "GiB", "TiB"):
        if size < 1024.0 or unit == "TiB":
            if unit == "B":
                return f"{int(size)}{unit}"
            return f"{size:.1f}{unit}"
        size /= 1024.0
    return None


def attempt_local_capture_gap_summary(
    attempt_dir: Path,
    *,
    branch_root: Path | None = None,
    clean_log: str | None = None,
) -> dict[str, object]:
    try:
        files = sorted(path.name for path in attempt_dir.iterdir() if path.is_file())
    except OSError:
        files = []
    snapshot_hits = [name for name in files if "snapshot" in name]
    soft_stop_hits = [name for name in files if "soft_stop" in name]
    runtime_hits = [name for name in files if "runtime" in name]
    missing_snapshot = not snapshot_hits
    missing_soft_stop = not soft_stop_hits
    missing_runtime = not runtime_hits
    resource_pressure_hits: list[str] = []
    resource_pressure_path: str | None = None
    if clean_log:
        for raw_line in clean_log.splitlines():
            line = strip_ansi(raw_line).strip()
            if not line:
                continue
            lowered = line.lower()
            if (
                "errno 28" not in lowered
                and "no space left on device" not in lowered
                and "quota_watch_status" not in lowered
            ):
                continue
            resource_pressure_hits.append(line)
            if resource_pressure_path is None:
                match = ENOSPC_PATH_RE.search(line)
                if match:
                    resource_pressure_path = match.group("path")
    resource_pressure_hits = list(dict.fromkeys(resource_pressure_hits))[:6]

    disk_free_bytes: int | None = None
    disk_used_percent: float | None = None
    if branch_root is not None:
        try:
            usage = shutil.disk_usage(branch_root)
            disk_free_bytes = usage.free
            if usage.total:
                disk_used_percent = round((usage.used / usage.total) * 100.0, 1)
        except OSError:
            disk_free_bytes = None
            disk_used_percent = None

    resource_pressure_summary: str | None = None
    if resource_pressure_hits:
        disk_fragment = ""
        if disk_free_bytes is not None and disk_used_percent is not None:
            disk_fragment = (
                f" Branch-local disk availability was only `{format_binary_size(disk_free_bytes)}` "
                f"free at `{disk_used_percent:.1f}%` used when this analysis refresh ran."
            )
        resource_pressure_summary = (
            "Workflow log recorded `Errno 28` while writing the attempt-local quota-watch status temp file, "
            "so retry-loop disk pressure is a live same-worktree confounder before any new smoke/strong-gate "
            "or solver inference should be trusted."
            + disk_fragment
        )
        gap_summary = resource_pressure_summary
    elif missing_snapshot and missing_soft_stop and missing_runtime:
        gap_summary = (
            f"`{attempt_dir.name}` top-level inventory has {len(files)} files but no "
            "`snapshot`, `soft_stop`, or `runtime` artifact names, so the next probe must prove "
            "runtime argv provenance from the launcher/watchdog handoff lines before another retry."
        )
    elif missing_snapshot and missing_soft_stop:
        gap_summary = (
            f"`{attempt_dir.name}` top-level inventory is still missing both `snapshot` and "
            "`soft_stop` artifacts, so the pre-crash handoff remains the first boundary to inspect."
        )
    else:
        gap_summary = (
            f"`{attempt_dir.name}` top-level inventory recorded {len(files)} files; review the "
            "remaining runtime artifacts before broadening beyond the retry-preflight corridor."
        )
    return {
        "attempt_dir": str(attempt_dir),
        "file_count": len(files),
        "top_level_files": files[:16],
        "snapshot_hits": snapshot_hits[:8],
        "soft_stop_hits": soft_stop_hits[:8],
        "runtime_hits": runtime_hits[:8],
        "missing_snapshot": missing_snapshot,
        "missing_soft_stop": missing_soft_stop,
        "missing_runtime": missing_runtime,
        "gap_summary": gap_summary,
        "resource_pressure_summary": resource_pressure_summary,
        "resource_pressure_hits": resource_pressure_hits,
        "resource_pressure_path": resource_pressure_path,
        "disk_free_bytes": disk_free_bytes,
        "disk_free_human": format_binary_size(disk_free_bytes),
        "disk_used_percent": disk_used_percent,
    }


def append_attempt_local_capture_gap_section(
    lines: list[str],
    capture_gap: dict[str, object] | None,
    *,
    heading: str = "#### Attempt-Local Capture Gap",
) -> None:
    lines.append(heading)
    lines.append("")
    capture_gap = capture_gap if isinstance(capture_gap, dict) else {}
    if capture_gap:
        lines.append(f"- Gap summary: {capture_gap['gap_summary']}")
        lines.append(f"- Attempt dir: `{capture_gap['attempt_dir']}`")
        lines.append(f"- Top-level file count: `{capture_gap['file_count']}`")
        if capture_gap.get("resource_pressure_summary"):
            lines.append(f"- Resource pressure: {capture_gap['resource_pressure_summary']}")
        if capture_gap.get("resource_pressure_path"):
            lines.append(f"- ENOSPC path: `{capture_gap['resource_pressure_path']}`")
        if capture_gap.get("disk_free_human") or capture_gap.get("disk_used_percent") is not None:
            disk_free = capture_gap.get("disk_free_human") or "unknown"
            disk_used = capture_gap.get("disk_used_percent")
            disk_used_text = f"{disk_used:.1f}%" if isinstance(disk_used, (int, float)) else "unknown"
            lines.append(f"- Branch-local free space: `{disk_free}` (used=`{disk_used_text}`)")
        lines.append(f"- Snapshot hits: `{capture_gap['snapshot_hits'] or 'none'}`")
        lines.append(f"- Soft-stop hits: `{capture_gap['soft_stop_hits'] or 'none'}`")
        lines.append(f"- Runtime hits: `{capture_gap['runtime_hits'] or 'none'}`")
        if capture_gap.get("top_level_files"):
            lines.append(
                "- Top-level files present: "
                + ", ".join(f"`{name}`" for name in capture_gap["top_level_files"])
            )
        if capture_gap.get("resource_pressure_hits"):
            lines.append("- Resource pressure hits:")
            for hit in capture_gap["resource_pressure_hits"]:
                lines.append(f"  - `{hit}`")
        return
    lines.append("- Attempt-local inventory could not be inspected for this retry-preflight failure.")


def should_render_attempt_local_capture_gap_for_failed_ac(
    breakdown: dict[str, object],
) -> bool:
    capture_gap = breakdown.get("attempt_local_capture_gap")
    if not isinstance(capture_gap, dict) or not capture_gap:
        return False
    if capture_gap.get("resource_pressure_hits"):
        return True
    if breakdown.get("retry_critical_anchors"):
        return False
    if breakdown.get("certify_rows_summary"):
        return False
    failure_family = str(breakdown.get("failure_family") or "").strip()
    if failure_family in {"generic_retry_failure", "strong_gate_unspecified", "boj3s_gate_unspecified"}:
        return True
    return breakdown.get("latest_probe_signal") in {None, {}}


def non_ac_failure_breakdown(
    branch_root: Path,
    clean_log: str,
    attempt_dir: Path,
    analysis_state: dict | None = None,
    current_attempt_label: str | None = None,
) -> dict[str, object] | None:
    relevant_lines = workflow_preflight_trace_lines(clean_log) or non_ac_trace_excerpt(clean_log)
    lowered = clean_log.lower()
    if not relevant_lines or ("traceback" not in lowered and "valueerror" not in lowered):
        return None

    summary_info = current_progress40_summary(branch_root)
    attempt_capture_gap = attempt_local_capture_gap_summary(
        attempt_dir,
        branch_root=branch_root,
        clean_log=clean_log,
    )
    primary_axis = summary_info.get("pivot_axis") or "zero_span_fastpath"
    failure_family = "retry_loop_pre_ac_exception"
    soft_stop_path_guard = "soft_stop_request.json" in lowered and "output path must stay under" in lowered
    soft_stop_defaults_confirmed = False
    if soft_stop_path_guard:
        soft_stop_defaults_confirmed = workflow_preflight_soft_stop_defaults_confirmed(branch_root)
        failure_family = "analysis_preflight_artifact_path_guard"
        if soft_stop_defaults_confirmed:
            failure_family = "analysis_preflight_soft_stop_argv_provenance"
    if "soft_stop_request.json" in lowered and "invalid seed format" in lowered:
        failure_family = "analysis_preflight_artifact_path_and_seed"
        if soft_stop_defaults_confirmed:
            failure_family = "analysis_preflight_soft_stop_argv_provenance"
    strongest_hypothesis = strongest_non_ac_failure_hypothesis(
        clean_log,
        soft_stop_path_guard=soft_stop_path_guard,
        soft_stop_defaults_confirmed=soft_stop_defaults_confirmed,
    )

    anchor_specs: list[dict[str, object]] = []
    seen: set[tuple[str, tuple[int, int]]] = set()
    anchor_index_by_key: dict[tuple[str, tuple[int, int]], int] = {}

    def add_anchor(
        path_text: str,
        focus_range: tuple[int, int],
        note: str,
        label: str,
        *,
        priority: int = 100,
        statement_excerpt: str | None = None,
    ) -> None:
        normalized = normalize_repo_relative_path(path_text)
        key = (normalized, focus_range)
        if key in seen:
            existing = anchor_specs[anchor_index_by_key[key]]
            if priority > int(existing.get("priority", 0)):
                existing["note"] = note
                existing["label"] = label
                existing["priority"] = priority
            if statement_excerpt and not existing.get("statement_excerpt"):
                existing["statement_excerpt"] = statement_excerpt
            return
        seen.add(key)
        anchor_index_by_key[key] = len(anchor_specs)
        anchor_specs.append(
            {
                "path": normalized,
                "range": focus_range,
                "note": note,
                "label": label,
                "priority": priority,
                "statement_excerpt": statement_excerpt,
            }
        )

    def existing_exact_anchor_covers(path_text: str, line_no: int) -> bool:
        normalized = normalize_repo_relative_path(path_text)
        for spec in anchor_specs:
            if normalize_repo_relative_path(str(spec["path"])) != normalized:
                continue
            start, end = tuple(spec["range"])
            if start <= line_no <= end and (end - start) <= 1:
                return True
        return False

    state_anchor_reuse_allowed = analysis_state_attempt_matches(
        analysis_state,
        current_attempt_label,
    )

    if isinstance(analysis_state, dict) and state_anchor_reuse_allowed:
        for item in analysis_state.get("latest_retry_statement_anchors", []):
            if not isinstance(item, dict):
                continue
            path_text = item.get("path")
            focus_range = parse_focus_range_text(item.get("focus_range"))
            if not isinstance(path_text, str) or focus_range is None:
                continue
            role = str(item.get("role") or "").strip()
            note = "promoted from failure_analysis_state.latest_retry_statement_anchors"
            if role:
                note += f"; role: {role}"
            add_anchor(
                path_text,
                focus_range,
                note,
                f"State-pinned statement anchor {path_text}:{range_label(*focus_range)}",
                priority=360,
                statement_excerpt=str(item.get("excerpt") or "").strip() or None,
            )
        for item in analysis_state.get("latest_retry_anchor_ranges", []):
            parsed = parse_anchor_range_text(item)
            if parsed is None:
                continue
            path_text, focus_range = parsed
            add_anchor(
                path_text,
                focus_range,
                "promoted from failure_analysis_state.latest_retry_anchor_ranges",
                f"State-pinned anchor {path_text}:{range_label(*focus_range)}",
                priority=150,
            )

    if "soft_stop_request.json" in lowered and "output path must stay under" in lowered:
        for hint in WORKFLOW_PREFLIGHT_HINTS:
            add_anchor(
                str(hint["path"]),
                tuple(hint["range"]),
                str(hint["note"]),
                str(hint["label"]),
                priority=int(hint.get("priority", 100)),
            )

    for line in relevant_lines:
        match = TRACEBACK_FILE_LINE_RE.search(line)
        if not match:
            continue
        raw_path = match.group("path")
        candidate = Path(raw_path)
        resolved = candidate if candidate.is_absolute() else (branch_root / candidate).resolve()
        try:
            relative = resolved.relative_to(branch_root).as_posix()
        except ValueError:
            continue
        if resolved.suffix not in CODE_SUFFIXES:
            continue
        line_no = int(match.group("line"))
        if existing_exact_anchor_covers(relative, line_no):
            continue
        focus_range = (line_no, line_no)
        func_name = match.group("func") or ""
        priority = 320
        if func_name in {"<module>", "__main__"} or not func_name:
            priority = 160
        elif func_name in {"ensure_under_artifacts", "_ensure_under_artifacts"}:
            priority = 210
        add_anchor(
            relative,
            focus_range,
            "promoted from the latest non-AC traceback corridor",
            f"Traceback anchor {relative}:{range_label(*focus_range)}",
            priority=priority,
        )

    retry_anchors: list[dict[str, object]] = []
    structural_focus: list[dict[str, object]] = []
    anchor_specs.sort(
        key=lambda spec: (
            -int(spec.get("priority", 0)),
            int(spec["range"][1]) - int(spec["range"][0]),
            str(spec["path"]),
            int(spec["range"][0]),
        )
    )
    for spec in anchor_specs[:8]:
        relative_path = normalize_repo_relative_path(str(spec["path"]))
        resolved = resolve_repo_path(branch_root, relative_path)
        if resolved is None or resolved.suffix not in CODE_SUFFIXES:
            continue
        focus_range = tuple(spec["range"])
        symbol_entries = select_symbol_entries(symbol_ranges_for_path(resolved), [focus_range])
        symbol = None
        if symbol_entries:
            entry = symbol_entries[0]
            symbol = f"{entry[2]} {entry[3]} [{entry[0]}-{entry[1]}]"
        evidence_lines = evidence_lines_for_path(relevant_lines, relative_path)
        evidence_lines = list(dict.fromkeys(line for line in evidence_lines if line))[:6]
        note = str(spec["note"])
        if isinstance(analysis_state, dict):
            note += ", pinned in failure_analysis_state"
        excerpt = code_excerpt_for_focus(resolved, [focus_range], symbol_entries, max_lines=18)
        statement_excerpt = (
            str(spec.get("statement_excerpt")).strip()
            if spec.get("statement_excerpt")
            else exact_statement_excerpt_for_focus(resolved, focus_range)
        )
        retry_anchors.append(
            {
                "label": str(spec["label"]),
                "path": str(resolved),
                "focus_range": range_label(*focus_range),
                "symbol": symbol,
                "evidence_lines": evidence_lines,
                "code_excerpt": excerpt,
                "statement_excerpt": statement_excerpt,
                "note": note,
            }
        )
        structural_focus.append(
            {
                "path": str(resolved),
                "observed_mentions": max(1, len(evidence_lines)),
                "focus_ranges": [range_label(*focus_range)],
                "enclosing_symbols": select_symbols(symbol_ranges_for_path(resolved), [focus_range]),
                "evidence_lines": evidence_lines,
                "code_excerpt": excerpt,
                "note": note,
                "mtime": safe_mtime_label(resolved),
            }
        )

    axis_evidence: dict[str, list[str]] = {}
    if primary_axis:
        axis_evidence[str(primary_axis)] = [
            "No fresh solver/runtime/profile evidence was produced; kept the authoritative progress40 pivot."
        ]
        if summary_info.get("pivot_text"):
            axis_evidence[str(primary_axis)].append(str(summary_info["pivot_text"]))
        axis_evidence[str(primary_axis)].append(
            "boj28350_progress40_results_merged.json → zero-span eligibility and fastpath commit share 49.9983%"
        )

    return {
        "title": "Pre-AC Failure: retry-orchestration preflight",
        "failure_type": "orchestration-preflight",
        "failure_family": failure_family,
        "primary_axis": primary_axis,
        "secondary_axis": None,
        "interpretation_lane": "retry-preflight",
        "strongest_surviving_hypothesis": strongest_hypothesis,
        "next_probe_command": fallback_probe_command_for_non_ac_failure(clean_log),
        "trace_excerpt": relevant_lines[-80:],
        "phase_summaries": [asdict(item) for item in phase_breakdown_for_lines(relevant_lines)],
        "retry_critical_anchors": retry_anchors,
        "structural_focus": structural_focus,
        "attempt_local_capture_gap": attempt_capture_gap,
        "axis_evidence": axis_evidence,
        "profile_mode": profile_mode_for_text(clean_log),
        "enabled_flags": enabled_flags_for_text(clean_log)[:24],
        "last_release_diag_phase": last_release_diag_phase_for_text(clean_log),
        "last_progress_checkpoint_phase": last_progress_phase_for_text(clean_log),
        "current_summary_pivot": summary_info.get("pivot_text"),
        "current_summary_residual_axes": list(summary_info.get("residual_axes", [])),
        "probe_note": (
            "Treat latest_next_probe_result.md as historical metadata only; this retry failed before any gate or solver work started."
            + (
                " Current in-tree defaults and the locality regression test still point soft-stop requests under retry artifacts/, so the stale .ouroboros path should be investigated as runtime argv provenance."
                if soft_stop_defaults_confirmed
                else ""
            )
        ),
    }


def guard_rejected_nominal_pass_breakdown(
    branch_root: Path,
    clean_log: str,
    attempt_dir: Path,
    attempt_guard: dict | None,
    *,
    attempt_started_at: datetime | None = None,
    analysis_state: dict | None = None,
    probe_signal: ProbeSignal | None = None,
    current_attempt_label: str | None = None,
) -> dict[str, object] | None:
    if not isinstance(attempt_guard, dict) or attempt_guard.get("guard_passed") is not False:
        return None

    implicated_acs = guard_implied_acs(attempt_guard)
    if not implicated_acs:
        return None
    lowered_clean_log = clean_log.lower()
    if 5 in implicated_acs and (
        "### ac 6" in lowered_clean_log
        or "second back-to-back boj gate run" in lowered_clean_log
        or "comb_dense" in lowered_clean_log
    ):
        implicated_acs = sorted(set(implicated_acs) | {6})

    focus_ac = 5 if 5 in implicated_acs else 6 if 6 in implicated_acs else implicated_acs[-1]
    relevant_lines = collect_ac_context_lines(clean_log, implicated_acs, forward_lines=8)
    for finding in attempt_guard.get("findings") or []:
        if not isinstance(finding, dict):
            continue
        ac_index = finding.get("ac_index")
        if ac_index not in {0, *implicated_acs}:
            continue
        reason = str(finding.get("reason") or "guard_finding")
        ac_text = str(finding.get("ac_text") or "unknown")
        relevant_lines.append(
            f"attempt_guard.json → AC {ac_index}: reason={reason} text={ac_text}"
        )
        for line in finding.get("evidence_lines") or []:
            if isinstance(line, str) and line.strip():
                relevant_lines.append(f"attempt_guard.json → {line.strip()}")

    for raw_line in clean_log.splitlines():
        line = strip_ansi(raw_line).strip()
        if not line:
            continue
        lowered = line.lower()
        if (
            "suspicious pass evidence detected" in lowered
            or "guard rejected a nominal pass" in lowered
            or "formal closure not achieved" in lowered
            or "does not pass yet" in lowered
            or "comb_dense" in lowered
            or "enable_state_load_materialization_opt" in lowered
        ):
            relevant_lines.append(line)
    relevant_lines = list(dict.fromkeys(line for line in relevant_lines if line))[-120:]

    snapshots = artifact_snapshots_for_failed_acs(
        branch_root,
        implicated_acs,
        attempt_started_at=attempt_started_at,
    )
    stale_formal_artifacts = stale_formal_artifact_labels(snapshots)
    structural_focus = structural_focus_for_ac(
        branch_root,
        clean_log,
        focus_ac,
        analysis_state=analysis_state,
        probe_signal=probe_signal,
        stale_formal_artifacts=stale_formal_artifacts,
    )
    retry_anchors = retry_critical_anchors_for_ac(
        branch_root,
        clean_log,
        focus_ac,
        analysis_state=analysis_state,
        probe_signal=probe_signal,
        stale_formal_artifacts=stale_formal_artifacts,
        current_attempt_label=current_attempt_label,
    )

    summary_info = current_progress40_summary(branch_root)
    primary_axis = summary_info.get("pivot_axis") or DEFAULT_AXIS_BY_AC.get(focus_ac) or "zero_span_fastpath"
    blocker_probe = branch_run_case_probe_for_clean_log(clean_log)
    stale_labels = sorted(stale_formal_artifacts)
    stale_text = ", ".join(stale_labels) if stale_labels else "formal-gate"
    strongest_hypothesis = (
        "The latest retry was a guard-rejected nominal PASS, not a trustworthy closure run: "
        f"{stale_text} evidence was stale for the attempt, and the surviving live blocker stayed in the "
        "late BOJ comb corridor instead of producing a fresh passing gate artifact."
    )

    axis_evidence = {
        str(primary_axis): [
            "latest_attempt_guard rejected the nominal PASS, so the report headline is not authoritative.",
            (
                "latest_failure_report keeps the direct blocker in the large adversarial comb corridor; "
                "the body says `comb_dense 50000 seed=1 L1 Q1` still times out at `30s` with "
                "`ENABLE_STATE_LOAD_MATERIALIZATION_OPT=1` and `0`."
            ),
            (
                "current_state_summary.md and the bundled progress40 report still name "
                "`zero-span eligibility and fastpath commit` as the largest residual at `49.9983%`."
            ),
        ]
    }
    if summary_info.get("pivot_text"):
        axis_evidence[str(primary_axis)].append(str(summary_info["pivot_text"]))
    if stale_labels:
        axis_evidence[str(primary_axis)].append(
            "stale formal artifacts during the attempt: " + ", ".join(stale_labels)
        )

    return {
        "title": "Guard-Rejected Nominal PASS",
        "ac_index": str(focus_ac),
        "failure_type": "guard-rejected-nominal-pass",
        "failure_family": "analysis_guard_rejected_nominal_pass",
        "primary_axis": primary_axis,
        "secondary_axis": None,
        "interpretation_lane": interpretation_lane_for_ac(focus_ac),
        "strongest_surviving_hypothesis": strongest_hypothesis,
        "next_probe_command": blocker_probe or recommended_probe_command(
            focus_ac,
            "boj3s_gate_timeout_cluster" if focus_ac in {5, 6} else "failure",
            primary_axis,
            probe_signal,
        ),
        "trace_excerpt": relevant_lines[-80:],
        "phase_summaries": [
            asdict(item) for item in phase_breakdown(relevant_lines, focus_ac, probe_signal=probe_signal)
        ],
        "retry_critical_anchors": [asdict(item) for item in retry_anchors],
        "structural_focus": [asdict(item) for item in structural_focus],
        "attempt_local_capture_gap": attempt_local_capture_gap_summary(
            attempt_dir,
            branch_root=branch_root,
            clean_log=clean_log,
        ),
        "axis_evidence": axis_evidence,
        "profile_mode": profile_mode_for_text(clean_log),
        "enabled_flags": enabled_flags_for_text(clean_log)[:24],
        "last_release_diag_phase": last_release_diag_phase_for_text(clean_log),
        "last_progress_checkpoint_phase": last_progress_phase_for_text(clean_log),
        "current_summary_pivot": summary_info.get("pivot_text"),
        "current_summary_residual_axes": list(summary_info.get("residual_axes", [])),
        "probe_note": (
            "Use the guard-implied ACs plus stale formal-artifact freshness instead of the PASS headline. "
            "The live body text still points to the late BOJ comb blocker, so keep the next retry on a single "
            "solver-side axis rather than widening back out."
        ),
        "guard_implicated_acs": implicated_acs,
        "guard_findings": attempt_guard.get("findings") or [],
        "focused_ac_index": focus_ac,
        "artifact_snapshots": [asdict(snapshot) for snapshot in snapshots],
    }


def write_markdown_list(lines: list[str], items: Iterable[str], empty_text: str) -> None:
    values = list(items)
    if not values:
        lines.append(f"- {empty_text}")
        return
    for item in values:
        lines.append(f"- {item}")


ANCHOR_ROLE_RE = re.compile(r"(?:^|;\s*)role:\s*(?P<role>.+)$")


def display_repo_relative_path(branch_root: Path, path_text: str) -> str:
    if not path_text:
        return "unknown"
    candidate = Path(path_text)
    try:
        resolved = candidate if candidate.is_absolute() else (branch_root / candidate).resolve()
        return resolved.relative_to(branch_root).as_posix()
    except Exception:
        normalized = normalize_repo_relative_path(path_text)
        return normalized or path_text


def summarized_anchor_excerpt(anchor: dict[str, object]) -> str | None:
    statement_excerpt = str(anchor.get("statement_excerpt") or "").strip()
    if statement_excerpt:
        return statement_excerpt
    code_excerpt = str(anchor.get("code_excerpt") or "").strip()
    if not code_excerpt:
        return None
    for raw_line in code_excerpt.splitlines():
        line = raw_line.strip()
        if line:
            return line
    return None


def anchor_role_from_note(note: str) -> str | None:
    match = ANCHOR_ROLE_RE.search(note.strip())
    if not match:
        return None
    role = match.group("role").strip()
    for fragment in (
        ", pinned in failure_analysis_state",
        ", matched latest_next_probe_result wrapper focus",
    ):
        role = role.replace(fragment, "")
    role = role.strip(" ,.")
    return role or None


def condensed_anchor_note(note: object) -> str | None:
    if not isinstance(note, str):
        return None
    text = note.strip()
    for fragment in (
        ", pinned in failure_analysis_state",
        " pinned in failure_analysis_state",
        ", matched latest_next_probe_result wrapper focus",
    ):
        text = text.replace(fragment, "")
    return text.strip(" ,") or None


def anchor_path_is_code_local(anchor: dict[str, object]) -> bool:
    path_text = str(anchor.get("path") or "").strip()
    return Path(path_text).suffix in CODE_SUFFIXES


def append_filtered_failure_locus_section(
    lines: list[str],
    branch_root: Path,
    anchors: list[dict[str, object]],
    *,
    failure_family: str | None,
    heading: str = "#### Filtered Failure Locus",
    limit: int = 4,
) -> bool:
    if failure_family != "transport_disconnected_retry":
        return False

    filtered_anchors = [
        anchor
        for anchor in anchors
        if isinstance(anchor, dict) and not anchor_path_is_code_local(anchor)
    ]
    if not filtered_anchors:
        return False

    lines.append(heading)
    lines.append("")
    lines.append(
        "- The current attempt never reached fresh solver/runtime/profile evidence, so these transport/trust boundaries outrank any carried-forward wrapper or solver hotspot scan."
    )
    rendered = 0
    for anchor in filtered_anchors:
        display_path = display_repo_relative_path(branch_root, str(anchor.get("path") or ""))
        focus_range = str(anchor.get("focus_range") or "unknown")
        label = str(anchor.get("label") or "").strip()
        symbol = str(anchor.get("symbol") or "").strip() or "none inferred"
        lines.append(f"- `{display_path}:{focus_range}`")
        if label:
            lines.append(f"  Boundary: `{label}`")
        lines.append(f"  Symbol: `{symbol}`")
        role = anchor_role_from_note(str(anchor.get("note") or ""))
        if role:
            lines.append(f"  Role: `{role}`")
        excerpt = summarized_anchor_excerpt(anchor)
        if excerpt:
            lines.append(f"  Statement: `{excerpt}`")
        note = condensed_anchor_note(anchor.get("note"))
        if note:
            lines.append(f"  Why first: {note}")
        rendered += 1
        if rendered >= limit:
            break
    lines.append(
        "- Broad code hotspots stay suppressed for this failure family until a same-worktree rerun survives the disconnect and emits direct gate or solver evidence."
    )
    lines.append("")
    return True


def append_narrowed_localization_section(
    lines: list[str],
    branch_root: Path,
    anchors: list[dict[str, object]],
    *,
    heading: str = "#### Narrowed Localization Snapshot",
    empty_text: str = "No statement-level localization was resolved for this breakdown.",
    limit: int = 4,
) -> None:
    lines.append(heading)
    lines.append("")
    rendered = 0
    for anchor in anchors:
        if not isinstance(anchor, dict):
            continue
        display_path = display_repo_relative_path(branch_root, str(anchor.get("path") or ""))
        focus_range = str(anchor.get("focus_range") or "unknown")
        label = str(anchor.get("label") or "").strip()
        symbol = str(anchor.get("symbol") or "").strip() or "none inferred"
        lines.append(f"- `{display_path}:{focus_range}`")
        if label:
            lines.append(f"  Boundary: `{label}`")
        lines.append(f"  Symbol: `{symbol}`")
        role = anchor_role_from_note(str(anchor.get("note") or ""))
        if role:
            lines.append(f"  Role: `{role}`")
        excerpt = summarized_anchor_excerpt(anchor)
        if excerpt:
            lines.append(f"  Statement: `{excerpt}`")
        note = condensed_anchor_note(anchor.get("note"))
        if note:
            lines.append(f"  Why now: {note}")
        rendered += 1
        if rendered >= limit:
            break
    if rendered == 0:
        lines.append(f"- {empty_text}")
    lines.append("")


def main() -> int:
    args = parse_args()
    branch_root = Path(args.branch_root).expanduser().resolve()
    ensure_under_artifacts, shared_resolver = _load_artifact_guard(branch_root)
    workflow_log = _resolve_artifact_path(
        branch_root,
        ensure_under_artifacts,
        args.workflow_log,
        shared_resolver,
    )
    report_root = _resolve_artifact_path(
        branch_root,
        ensure_under_artifacts,
        args.report_root,
        shared_resolver,
    )
    attempt_dir = ensure_under_artifacts(workflow_log.parent.resolve())
    current_attempt_label = f"attempt_{args.attempt:03d}"
    prepare_output_dir(report_root)
    prepare_output_dir(attempt_dir)
    attempt_started_at = attempt_started_at_for_dir(attempt_dir)

    previous_report = load_json(report_root / "latest_failure_report.json")
    previous_breakdown = load_json(report_root / "latest_failure_breakdown.json")
    analysis_state_path = branch_root / ".ouroboros/failure_analysis_state.json"
    analysis_state = load_json(analysis_state_path)
    history_path = report_root / "failure_history.json"
    history_payload = load_json(history_path)
    history = history_payload if isinstance(history_payload, list) else []
    probe_signal = load_latest_next_probe_signal(branch_root)
    attempt_guard = load_attempt_guard(attempt_dir)

    raw_log = workflow_log.read_text(errors="replace") if workflow_log.exists() else ""
    clean_log = strip_ansi(raw_log)

    session_ids = SESSION_RE.findall(clean_log)
    execution_ids = EXEC_RE.findall(clean_log)
    session_id = session_ids[-1] if session_ids else None
    execution_id = execution_ids[-1] if execution_ids else None

    global_log_excerpt = filter_log_for_session(
        Path("/Users/free_1/.ouroboros/logs/ouroboros.log"),
        session_id,
    )

    failed_acs = FAILED_AC_RE.findall(clean_log)
    blocked_acs = BLOCKED_AC_RE.findall(clean_log)
    passed_acs = PASS_AC_RE.findall(clean_log)
    failed_ac_numbers = [int(index) for index, _ in failed_acs]

    summary_text = extract_section(clean_log, "Parallel Execution Complete")
    if summary_text is None:
        summary_text = "\n".join(clean_log.splitlines()[-120:])

    base_snapshots = [
        build_artifact_snapshot(
            "smoke",
            branch_root / "artifacts/lca_tree_stress_v5/smoke",
            attempt_started_at=attempt_started_at,
        ),
        build_artifact_snapshot(
            "strong_gate",
            branch_root / "artifacts/lca_tree_stress_v5/strong_gate",
            attempt_started_at=attempt_started_at,
        ),
        build_artifact_snapshot(
            "boj3s_gate",
            branch_root / "artifacts/lca_tree_stress_v5/boj3s_gate",
            attempt_started_at=attempt_started_at,
        ),
        build_artifact_snapshot(
            "hunt",
            branch_root / "artifacts/lca_tree_stress_v5/hunt",
            attempt_started_at=attempt_started_at,
        ),
    ]
    focused_snapshots = artifact_snapshots_for_failed_acs(
        branch_root,
        failed_ac_numbers,
        attempt_started_at=attempt_started_at,
    )
    attempt_capture_gap = attempt_local_capture_gap_summary(
        attempt_dir,
        branch_root=branch_root,
        clean_log=clean_log,
    )

    breakdowns: list[dict] = []
    for ac_index_str, ac_text in failed_acs:
        ac_index = int(ac_index_str)
        relevant_lines = collect_ac_context_lines(clean_log, [ac_index])
        ac_artifact_snapshots = artifact_snapshots_for_failed_acs(
            branch_root,
            [ac_index],
            attempt_started_at=attempt_started_at,
        )
        stale_formal_artifacts = stale_formal_artifact_labels(ac_artifact_snapshots)
        structural_focus = structural_focus_for_ac(
            branch_root,
            clean_log,
            ac_index,
            analysis_state=analysis_state if isinstance(analysis_state, dict) else None,
            probe_signal=probe_signal,
            stale_formal_artifacts=stale_formal_artifacts,
        )
        retry_anchors = retry_critical_anchors_for_ac(
            branch_root,
            clean_log,
            ac_index,
            analysis_state=analysis_state if isinstance(analysis_state, dict) else None,
            probe_signal=probe_signal,
            stale_formal_artifacts=stale_formal_artifacts,
            current_attempt_label=current_attempt_label,
        )
        certify_rows_summary = certify_rows_summary_for_ac(ac_artifact_snapshots)
        axis_summary = progress40_axis_breakdown(
            branch_root,
            ac_index,
            relevant_lines,
            structural_focus,
            clean_log,
            certify_rows_summary=certify_rows_summary,
            probe_signal=probe_signal,
            stale_formal_artifacts=stale_formal_artifacts,
        )
        failure_family = refine_failure_family_with_certify_rows(
            ac_index,
            axis_summary.failure_family,
            certify_rows_summary,
        )
        primary_axis = axis_summary.primary_axis
        secondary_axis = axis_summary.secondary_axis
        axis_evidence = {axis: list(lines) for axis, lines in axis_summary.axis_evidence.items()}
        next_probe_command = recommended_probe_command(
            ac_index,
            failure_family,
            primary_axis,
            probe_signal,
        )
        if ac_index == 2 and has_fresh_smoke_gate_escalation(ac_artifact_snapshots):
            summary_info = current_progress40_summary(branch_root)
            primary_axis = (
                summary_info.get("pivot_axis")
                or primary_axis
                or DEFAULT_AXIS_BY_AC.get(5)
                or "zero_span_fastpath"
            )
            secondary_axis = None
            failure_family = "smoke_same_worktree_pass_gate_escalation_pending"
            parked_axis_lines = axis_evidence.setdefault(str(primary_axis), [])
            parked_primary_note = (
                "Fresh same-worktree smoke status already published "
                "`public_status=PASS`, `gate_chain_ac2_status=satisfied`, and "
                "`next_gate_command=./lca_strong_gate.sh`, so AC2 is published partial progress "
                "rather than the live retry blocker."
            )
            parked_axis_only_note = (
                "Keep `zero_span_fastpath` parked as the only progress40 axis here because the "
                "same attempt produced no fresh strong-gate, solver-runtime, or profile evidence "
                "that would justify `state_materialization` or `layout_gate`."
            )
            if parked_primary_note not in parked_axis_lines:
                parked_axis_lines.insert(0, parked_primary_note)
            if parked_axis_only_note not in parked_axis_lines:
                parked_axis_lines.append(parked_axis_only_note)
            next_probe_command = recommended_probe_command(
                ac_index,
                failure_family,
                primary_axis,
                probe_signal,
            )
        latest_probe_signal = None
        if probe_signal_applies_to_ac(probe_signal, ac_index) and probe_signal is not None:
            latest_probe_signal = {
                "command": probe_signal.command,
                "primary_axis": probe_signal.primary_axis,
                "secondary_axis": probe_signal.secondary_axis,
                "why_this_axis": probe_signal.why_this_axis,
                "exit_code": probe_signal.exit_code,
                "timed_out": probe_signal.timed_out,
                "elapsed_seconds": probe_signal.elapsed_seconds,
                "stdout_log": probe_signal.stdout_log,
                "stderr_log": probe_signal.stderr_log,
                "stderr_excerpt": probe_signal.stderr_excerpt,
                "active_gate": probe_signal.active_gate,
                "active_pid": probe_signal.active_pid,
                "wrapper_path": probe_signal.wrapper_path,
                "completed_case_counts": probe_signal_completed_case_counts(probe_signal),
                "focus_range": (
                    range_label(*probe_signal.focus_range)
                    if probe_signal.focus_range is not None
                    else None
                ),
                "focus_symbol": probe_signal.focus_symbol,
                "quick_fail_lock": probe_signal_is_quick_fail_lock(probe_signal, ac_index),
                "zero_progress_timeout": probe_signal_is_zero_progress_timeout(probe_signal, ac_index),
            }
        breakdowns.append(
            {
                "ac_index": ac_index_str,
                "ac_text": ac_text,
                "failure_type": failure_type_for_ac(clean_log, ac_index),
                "failure_family": failure_family,
                "primary_axis": primary_axis,
                "secondary_axis": secondary_axis,
                "axis_evidence": axis_evidence,
                "profile_mode": axis_summary.profile_mode,
                "enabled_flags": axis_summary.enabled_flags,
                "last_release_diag_phase": axis_summary.last_release_diag_phase,
                "last_progress_checkpoint_phase": axis_summary.last_progress_checkpoint_phase,
                "current_summary_pivot": axis_summary.current_summary_pivot,
                "current_summary_residual_axes": axis_summary.current_summary_residual_axes,
                "next_probe_command": next_probe_command,
                "interpretation_lane": axis_summary.interpretation_lane,
                "trace_excerpt": relevant_lines[-80:],
                "phase_summaries": [
                    asdict(item)
                    for item in phase_breakdown(relevant_lines, ac_index, probe_signal=probe_signal)
                ],
                "retry_critical_anchors": [asdict(item) for item in retry_anchors],
                "structural_focus": [asdict(item) for item in structural_focus],
                "attempt_local_capture_gap": attempt_capture_gap,
                "artifact_snapshots": [asdict(snapshot) for snapshot in ac_artifact_snapshots],
                "certify_rows_summary": certify_rows_summary,
                "latest_probe_signal": latest_probe_signal,
            }
        )

    fallback_breakdown = None
    if not breakdowns:
        fallback_breakdown = guard_rejected_nominal_pass_breakdown(
            branch_root,
            clean_log,
            attempt_dir,
            attempt_guard,
            attempt_started_at=attempt_started_at,
            analysis_state=analysis_state if isinstance(analysis_state, dict) else None,
            probe_signal=probe_signal,
            current_attempt_label=current_attempt_label,
        )
        if fallback_breakdown is None:
            fallback_breakdown = non_ac_failure_breakdown(
                branch_root,
                clean_log,
                attempt_dir,
                analysis_state=analysis_state if isinstance(analysis_state, dict) else None,
                current_attempt_label=current_attempt_label,
            )
    refinement_notes = build_refinement_notes(
        failed_acs,
        breakdowns or ([fallback_breakdown] if fallback_breakdown else []),
        previous_report,
        previous_breakdown,
        probe_signal=probe_signal,
    )
    if fallback_breakdown is not None:
        if fallback_breakdown.get("failure_type") == "guard-rejected-nominal-pass":
            refinement_notes = [
                "Latest failure recorded zero failed ACs, but attempt_guard rejected the nominal PASS; the next retry must start from guard-implied ACs instead of the report headline.",
                "The current-attempt strong_gate and boj3s_gate artifacts were stale, so AC3/AC5/AC6 closure claims are not trustworthy until fresh gate outputs exist.",
                "Keep `zero_span_fastpath` as the only progress40 axis here because the authoritative progress40 baseline still leads there and the live BOJ blocker times out with materialization both on and off.",
            ] + refinement_notes
        else:
            refinement_notes = [
                "Latest failure exited before any AC verdicts were recorded, so the next retry must start from the retry-orchestration preflight corridor instead of stale AC-local hotspots.",
                "The workflow tail contains both the quota-watchdog artifact-path traceback and the solver-seed validation error; treat attempt-local solver analysis as blocked until both preflight failures are cleared.",
                "Keep `zero_span_fastpath` as the only progress40 axis because the current summary and bundled progress40 results still point to zero-span eligibility and fastpath commit, but no fresh solver/profile evidence survived long enough to justify a secondary axis.",
            ] + refinement_notes
        refinement_notes = refinement_notes[:8]
    report_localization_anchors = (
        (breakdowns[0].get("retry_critical_anchors") or [])
        if breakdowns
        else (fallback_breakdown.get("retry_critical_anchors") or []) if fallback_breakdown else []
    )
    report_failure_family = (
        str(breakdowns[0].get("failure_family") or "").strip()
        if breakdowns
        else str(fallback_breakdown.get("failure_family") or "").strip()
        if fallback_breakdown
        else None
    )

    report_md = attempt_dir / "failure_report.md"
    report_json = attempt_dir / "failure_report.json"
    breakdown_md = attempt_dir / "failure_breakdown.md"
    breakdown_json = attempt_dir / "failure_breakdown.json"
    capture_timestamp = stable_timestamp()

    report_lines: list[str] = []
    report_lines.append(f"# Failure Report: Attempt {args.attempt}")
    report_lines.append("")
    report_lines.append(f"- Timestamp: `{capture_timestamp}`")
    report_lines.append(f"- Seed: `{args.seed_file}`")
    report_lines.append(f"- Exit code: `{args.exit_code}`")
    report_lines.append(f"- Session ID: `{session_id or 'unknown'}`")
    report_lines.append(f"- Execution ID: `{execution_id or 'unknown'}`")
    report_lines.append(
        f"- Analysis state file: `{analysis_state_path}`"
    )
    if isinstance(analysis_state, dict):
        report_lines.append(
            f"- Analysis state revision: `{analysis_state.get('analysis_revision', 'unknown')}`"
        )
    report_lines.append("")
    report_lines.append("## Result Summary")
    report_lines.append("")
    report_lines.append("```text")
    report_lines.append(summary_text.strip())
    report_lines.append("```")
    report_lines.append("")
    report_lines.append("## Parsed AC Verdicts")
    report_lines.append("")
    report_lines.append(f"- Failed ACs: {failed_acs or 'none found'}")
    report_lines.append(f"- Blocked ACs: {blocked_acs or 'none found'}")
    report_lines.append(f"- Passed ACs: {passed_acs[:8] if passed_acs else 'none found'}")
    report_lines.append("")
    append_narrowed_localization_section(
        report_lines,
        branch_root,
        report_localization_anchors,
        heading="## Narrowed Localization Snapshot",
        empty_text="No statement-level localization was resolved from the latest failure trace.",
        limit=3,
    )
    append_filtered_failure_locus_section(
        report_lines,
        branch_root,
        report_localization_anchors,
        failure_family=report_failure_family,
        heading="## Filtered Failure Locus",
        limit=3,
    )
    report_lines.append("## Git Status At Failure")
    report_lines.append("")
    report_lines.append("```text")
    report_lines.append(run_git_status(branch_root))
    report_lines.append("```")
    report_lines.append("")
    report_lines.append("## Relevant Artifact Snapshots")
    report_lines.append("")
    for snapshot in base_snapshots:
        report_lines.append(f"### {snapshot.label}")
        report_lines.append("")
        report_lines.append(f"- Latest file: `{snapshot.latest_file or 'none'}`")
        report_lines.append(f"- Latest mtime: `{snapshot.latest_mtime or 'unknown'}`")
        report_lines.append(f"- Summary file: `{snapshot.summary_file or 'none'}`")
        if snapshot.attempt_start:
            report_lines.append(f"- Attempt start: `{snapshot.attempt_start}`")
            freshness_value = "unknown"
            if snapshot.fresh_for_attempt is True:
                freshness_value = "yes"
            elif snapshot.fresh_for_attempt is False:
                freshness_value = "no"
            report_lines.append(f"- Fresh within attempt: `{freshness_value}`")
        if snapshot.freshness_note:
            report_lines.append(f"- Freshness note: {snapshot.freshness_note}")
        if snapshot.summary_excerpt:
            report_lines.append("")
            report_lines.append("```text")
            report_lines.append(snapshot.summary_excerpt.strip())
            report_lines.append("```")
        report_lines.append("")
    report_lines.append("## Session Log Excerpt")
    report_lines.append("")
    report_lines.append("```text")
    report_lines.extend(global_log_excerpt or ["(no session-specific log lines found)"])
    report_lines.append("```")
    report_lines.append("")
    report_lines.append("## Workflow Log Tail")
    report_lines.append("")
    report_lines.append("```text")
    report_lines.extend(clean_log.splitlines()[-120:])
    report_lines.append("```")
    report_lines.append("")
    report_lines.append(
        "See `failure_breakdown.md` for the per-AC phase split, structural hotspot analysis, and the "
        "refinement notes to carry into the next retry."
    )

    breakdown_lines: list[str] = []
    breakdown_lines.append(f"# Failure Breakdown: Attempt {args.attempt}")
    breakdown_lines.append("")
    breakdown_lines.append(f"- Timestamp: `{capture_timestamp}`")
    breakdown_lines.append(f"- Session ID: `{session_id or 'unknown'}`")
    breakdown_lines.append(f"- Execution ID: `{execution_id or 'unknown'}`")
    breakdown_lines.append(f"- Analysis state file: `{analysis_state_path}`")
    if isinstance(analysis_state, dict):
        breakdown_lines.append(
            f"- Analysis state revision: `{analysis_state.get('analysis_revision', 'unknown')}`"
        )
    breakdown_lines.append("")
    breakdown_lines.append("## Failure Decomposition")
    breakdown_lines.append("")
    if not breakdowns:
        if fallback_breakdown:
            breakdown_lines.append(f"### {fallback_breakdown['title']}")
            breakdown_lines.append("")
            breakdown_lines.append(f"- Failure type: `{fallback_breakdown['failure_type']}`")
            breakdown_lines.append(f"- Failure family: `{fallback_breakdown['failure_family']}`")
            breakdown_lines.append(
                f"- Interpretation lane: `{fallback_breakdown['interpretation_lane']}`"
            )
            breakdown_lines.append(
                f"- Primary progress40 axis: `{fallback_breakdown['primary_axis'] or 'unknown'}`"
            )
            breakdown_lines.append("- Secondary progress40 axis: `none`")
            breakdown_lines.append(
                f"- Profile mode observed: `{fallback_breakdown['profile_mode'] or 'unknown'}`"
            )
            breakdown_lines.append(
                f"- Last progress checkpoint phase: `{fallback_breakdown['last_progress_checkpoint_phase'] or 'unknown'}`"
            )
            breakdown_lines.append(
                f"- Last release diag phase: `{fallback_breakdown['last_release_diag_phase'] or 'unknown'}`"
            )
            breakdown_lines.append(
                f"- Suggested next probe: `{fallback_breakdown['next_probe_command']}`"
            )
            if fallback_breakdown.get("probe_note"):
                breakdown_lines.append(f"- Probe interpretation: {fallback_breakdown['probe_note']}")
            breakdown_lines.append("")
            guard_implicated_acs = fallback_breakdown.get("guard_implicated_acs") or []
            guard_findings = fallback_breakdown.get("guard_findings") or []
            if guard_implicated_acs or guard_findings:
                breakdown_lines.append("#### Guard Rejection")
                breakdown_lines.append("")
                if guard_implicated_acs:
                    breakdown_lines.append(
                        "- Guard-implied ACs: "
                        + ", ".join(f"`{ac}`" for ac in guard_implicated_acs)
                    )
                if fallback_breakdown.get("focused_ac_index"):
                    breakdown_lines.append(
                        f"- Narrowing focus AC: `{fallback_breakdown['focused_ac_index']}`"
                    )
                if guard_findings:
                    for finding in guard_findings[:6]:
                        if not isinstance(finding, dict):
                            continue
                        breakdown_lines.append(
                            "- Finding: "
                            + f"`AC {finding.get('ac_index', 'unknown')}` "
                            + f"`{finding.get('reason', 'unknown')}` "
                            + f"`{finding.get('ac_text', 'unknown')}`"
                        )
            breakdown_lines.append("")
            breakdown_lines.append("#### Strongest Surviving Hypothesis")
            breakdown_lines.append("")
            if fallback_breakdown.get("strongest_surviving_hypothesis"):
                breakdown_lines.append(
                    f"- {fallback_breakdown['strongest_surviving_hypothesis']}"
                )
            else:
                breakdown_lines.append(
                    "- No single strongest surviving hypothesis could be isolated from the retry-preflight traceback."
                )
            breakdown_lines.append("")
            append_narrowed_localization_section(
                breakdown_lines,
                branch_root,
                fallback_breakdown.get("retry_critical_anchors") or [],
            )
            filtered_locus_rendered = append_filtered_failure_locus_section(
                breakdown_lines,
                branch_root,
                fallback_breakdown.get("retry_critical_anchors") or [],
                failure_family=str(fallback_breakdown.get("failure_family") or "").strip() or None,
            )
            breakdown_lines.append("#### Attempt-Local Capture Gap")
            breakdown_lines.append("")
            capture_gap = fallback_breakdown.get("attempt_local_capture_gap") or {}
            if capture_gap:
                breakdown_lines.append(f"- Gap summary: {capture_gap['gap_summary']}")
                breakdown_lines.append(f"- Attempt dir: `{capture_gap['attempt_dir']}`")
                breakdown_lines.append(f"- Top-level file count: `{capture_gap['file_count']}`")
                breakdown_lines.append(
                    f"- Snapshot hits: `{capture_gap['snapshot_hits'] or 'none'}`"
                )
                breakdown_lines.append(
                    f"- Soft-stop hits: `{capture_gap['soft_stop_hits'] or 'none'}`"
                )
                breakdown_lines.append(
                    f"- Runtime hits: `{capture_gap['runtime_hits'] or 'none'}`"
                )
                if capture_gap.get("top_level_files"):
                    breakdown_lines.append(
                        "- Top-level files present: "
                        + ", ".join(f"`{name}`" for name in capture_gap["top_level_files"])
                    )
            else:
                breakdown_lines.append(
                    "- Attempt-local inventory could not be inspected for this retry-preflight failure."
                )
            artifact_snapshots = fallback_breakdown.get("artifact_snapshots") or []
            if artifact_snapshots:
                breakdown_lines.append("")
                breakdown_lines.append("#### Focused Artifact Snapshots")
                breakdown_lines.append("")
                for snapshot in artifact_snapshots:
                    if not isinstance(snapshot, dict):
                        continue
                    breakdown_lines.append(
                        f"- `{snapshot.get('label', 'unknown')}` latest: `{snapshot.get('latest_file') or 'none'}`"
                    )
                    breakdown_lines.append(f"  mtime: `{snapshot.get('latest_mtime') or 'unknown'}`")
                    if snapshot.get("attempt_start"):
                        breakdown_lines.append(f"  attempt start: `{snapshot['attempt_start']}`")
                    freshness_value = "unknown"
                    if snapshot.get("fresh_for_attempt") is True:
                        freshness_value = "yes"
                    elif snapshot.get("fresh_for_attempt") is False:
                        freshness_value = "no"
                    breakdown_lines.append(f"  fresh within attempt: `{freshness_value}`")
                    if snapshot.get("freshness_note"):
                        breakdown_lines.append(f"  freshness note: {snapshot['freshness_note']}")
            breakdown_lines.append("")
            breakdown_lines.append("#### Progress40 Axis Evidence")
            breakdown_lines.append("")
            if fallback_breakdown["axis_evidence"]:
                for axis, evidence_lines in fallback_breakdown["axis_evidence"].items():
                    breakdown_lines.append(f"- `{axis}`")
                    for evidence_line in evidence_lines[:4]:
                        breakdown_lines.append(f"  - `{evidence_line}`")
            else:
                breakdown_lines.append(
                    "- No direct axis evidence was extracted from the traceback; fallback axis came from the current progress40 summary."
                )
            if fallback_breakdown["current_summary_pivot"]:
                breakdown_lines.append("")
                breakdown_lines.append(
                    f"- Current summary pivot baseline: `{fallback_breakdown['current_summary_pivot']}`"
                )
            if fallback_breakdown["current_summary_residual_axes"]:
                breakdown_lines.append(
                    "- Current summary residual axes: "
                    + ", ".join(
                        f"`{axis}`" for axis in fallback_breakdown["current_summary_residual_axes"]
                    )
                )
            breakdown_lines.append("")
            breakdown_lines.append("#### Phase Breakdown")
            breakdown_lines.append("")
            if fallback_breakdown["phase_summaries"]:
                for phase in fallback_breakdown["phase_summaries"]:
                    breakdown_lines.append(
                        f"- `{phase['phase']}` x{phase['count']} | sample: `{phase['sample']}`"
                    )
            else:
                breakdown_lines.append("- No command-phase decomposition could be inferred from the traceback.")
            breakdown_lines.append("")
            breakdown_lines.append("#### Retry-Critical Anchors")
            breakdown_lines.append("")
            if fallback_breakdown["retry_critical_anchors"]:
                for anchor in fallback_breakdown["retry_critical_anchors"]:
                    breakdown_lines.append(f"- Anchor: `{anchor['label']}`")
                    breakdown_lines.append(f"  Path: `{anchor['path']}`")
                    breakdown_lines.append(f"  Focus range: `{anchor['focus_range']}`")
                    breakdown_lines.append(
                        f"  Enclosing symbol: `{anchor['symbol'] or 'none inferred'}`"
                    )
                    if anchor.get("statement_excerpt"):
                        breakdown_lines.append(
                            f"  Statement excerpt: `{anchor['statement_excerpt']}`"
                        )
                    breakdown_lines.append(f"  Note: {anchor['note']}")
                    if anchor["evidence_lines"]:
                        breakdown_lines.append("  Evidence lines:")
                        for evidence_line in anchor["evidence_lines"]:
                            breakdown_lines.append(f"    - `{evidence_line}`")
                    if anchor["code_excerpt"]:
                        breakdown_lines.append("  Code excerpt:")
                        breakdown_lines.append("```text")
                        breakdown_lines.append(anchor["code_excerpt"])
                        breakdown_lines.append("```")
            else:
                breakdown_lines.append("- No retry-critical anchors were resolved for this non-AC failure.")
            breakdown_lines.append("")
            breakdown_lines.append("#### Code-Structure Hotspots")
            breakdown_lines.append("")
            if filtered_locus_rendered:
                breakdown_lines.append(
                    "- Suppressed broad code hotspots for `transport_disconnected_retry`; the filtered boundary section above is the authoritative reread target until direct gate or solver evidence exists."
                )
            elif fallback_breakdown["structural_focus"]:
                for hotspot in fallback_breakdown["structural_focus"]:
                    breakdown_lines.append(f"- File: `{hotspot['path']}`")
                    breakdown_lines.append(f"  Observed mentions: `{hotspot['observed_mentions']}`")
                    breakdown_lines.append(
                        "  Focus ranges: "
                        + (", ".join(hotspot["focus_ranges"]) if hotspot["focus_ranges"] else "none captured")
                    )
                    breakdown_lines.append(
                        "  Enclosing symbols: "
                        + (", ".join(hotspot["enclosing_symbols"]) if hotspot["enclosing_symbols"] else "no symbols inferred")
                    )
                    breakdown_lines.append(f"  Note: {hotspot['note']}")
                    breakdown_lines.append(f"  Mtime: `{hotspot['mtime'] or 'unknown'}`")
                    if hotspot["evidence_lines"]:
                        breakdown_lines.append("  Evidence lines:")
                        for evidence_line in hotspot["evidence_lines"]:
                            breakdown_lines.append(f"    - `{evidence_line}`")
                    if hotspot["code_excerpt"]:
                        breakdown_lines.append("  Code excerpt:")
                        breakdown_lines.append("```text")
                        breakdown_lines.append(hotspot["code_excerpt"])
                        breakdown_lines.append("```")
            else:
                breakdown_lines.append("- No structural hotspots could be resolved for this non-AC failure.")
            breakdown_lines.append("")
            breakdown_lines.append("#### Failure Excerpt")
            breakdown_lines.append("")
            breakdown_lines.append("```text")
            breakdown_lines.extend(fallback_breakdown["trace_excerpt"] or ["(no failure excerpt found)"])
            breakdown_lines.append("```")
        else:
            breakdown_lines.append("- No failed AC-specific breakdown could be extracted from the workflow log.")
    for breakdown in breakdowns:
        breakdown_lines.append(f"### AC {breakdown['ac_index']}: {breakdown['ac_text']}")
        breakdown_lines.append("")
        breakdown_lines.append(f"- Failure type: `{breakdown['failure_type']}`")
        breakdown_lines.append(f"- Failure family: `{breakdown['failure_family']}`")
        breakdown_lines.append(f"- Interpretation lane: `{breakdown['interpretation_lane']}`")
        breakdown_lines.append(f"- Primary progress40 axis: `{breakdown['primary_axis'] or 'unknown'}`")
        breakdown_lines.append(f"- Secondary progress40 axis: `{breakdown['secondary_axis'] or 'none'}`")
        breakdown_lines.append(f"- Profile mode observed: `{breakdown['profile_mode'] or 'unknown'}`")
        breakdown_lines.append(
            f"- Last progress checkpoint phase: `{breakdown['last_progress_checkpoint_phase'] or 'unknown'}`"
        )
        breakdown_lines.append(
            f"- Last release diag phase: `{breakdown['last_release_diag_phase'] or 'unknown'}`"
        )
        breakdown_lines.append(f"- Suggested next probe: `{breakdown['next_probe_command']}`")
        breakdown_lines.append(f"- Trace lines captured: `{len(breakdown['trace_excerpt'])}`")
        breakdown_lines.append("")
        append_narrowed_localization_section(
            breakdown_lines,
            branch_root,
            breakdown.get("retry_critical_anchors") or [],
        )
        filtered_locus_rendered = append_filtered_failure_locus_section(
            breakdown_lines,
            branch_root,
            breakdown.get("retry_critical_anchors") or [],
            failure_family=str(breakdown.get("failure_family") or "").strip() or None,
        )
        breakdown_lines.append("#### Progress40 Axis Evidence")
        breakdown_lines.append("")
        if breakdown["axis_evidence"]:
            for axis, evidence_lines in breakdown["axis_evidence"].items():
                breakdown_lines.append(f"- `{axis}`")
                for evidence_line in evidence_lines[:4]:
                    breakdown_lines.append(f"  - `{evidence_line}`")
        else:
            breakdown_lines.append(
                "- No direct axis evidence was extracted from the trace; fallback axis came from the current progress40 summary."
            )
        if breakdown["enabled_flags"]:
            breakdown_lines.append("")
            breakdown_lines.append(
                "- Enabled flags seen in trace: " + ", ".join(f"`{flag}`" for flag in breakdown["enabled_flags"])
            )
        if breakdown["current_summary_pivot"]:
            breakdown_lines.append(
                f"- Current summary pivot baseline: `{breakdown['current_summary_pivot']}`"
            )
        if breakdown["current_summary_residual_axes"]:
            breakdown_lines.append(
                "- Current summary residual axes: "
                + ", ".join(f"`{axis}`" for axis in breakdown["current_summary_residual_axes"])
            )
        breakdown_lines.append("")
        breakdown_lines.append("#### Phase Breakdown")
        breakdown_lines.append("")
        if breakdown["phase_summaries"]:
            for phase in breakdown["phase_summaries"]:
                breakdown_lines.append(
                    f"- `{phase['phase']}` x{phase['count']} | sample: `{phase['sample']}`"
                )
        else:
            breakdown_lines.append("- No command-phase decomposition could be inferred from the failed-AC trace.")
        breakdown_lines.append("")
        breakdown_lines.append("#### Latest Next-Probe Signal")
        breakdown_lines.append("")
        if breakdown["latest_probe_signal"]:
            probe = breakdown["latest_probe_signal"]
            breakdown_lines.append(f"- Command: `{probe['command'] or 'unknown'}`")
            breakdown_lines.append(f"- Exit code: `{probe['exit_code'] if probe['exit_code'] is not None else 'unknown'}`")
            breakdown_lines.append(
                f"- Elapsed seconds: `{probe['elapsed_seconds'] if probe['elapsed_seconds'] is not None else 'unknown'}`"
            )
            breakdown_lines.append(
                f"- Quick-fail lock signal: `{'yes' if probe['quick_fail_lock'] else 'no'}`"
            )
            if probe["wrapper_path"]:
                breakdown_lines.append(f"- Wrapper focus path: `{probe['wrapper_path']}`")
            if probe["focus_range"]:
                breakdown_lines.append(f"- Wrapper focus range: `{probe['focus_range']}`")
            if probe["focus_symbol"]:
                breakdown_lines.append(f"- Wrapper focus symbol: `{probe['focus_symbol']}`")
            if probe["stderr_log"]:
                breakdown_lines.append(f"- Stderr log: `{probe['stderr_log']}`")
            if probe["stderr_excerpt"]:
                breakdown_lines.append("- Probe stderr excerpt:")
                for stderr_line in probe["stderr_excerpt"][:4]:
                    breakdown_lines.append(f"  - `{stderr_line}`")
        else:
            breakdown_lines.append("- No latest probe signal was available for this AC.")
        breakdown_lines.append("")
        breakdown_lines.append("#### Current-Attempt Certify Rows")
        breakdown_lines.append("")
        certify_rows_summary = breakdown.get("certify_rows_summary")
        if certify_rows_summary:
            breakdown_lines.append(f"- Rows source: `{certify_rows_summary['rows_path']}`")
            fresh_value = "unknown"
            if certify_rows_summary.get("fresh_for_attempt") is True:
                fresh_value = "yes"
            elif certify_rows_summary.get("fresh_for_attempt") is False:
                fresh_value = "no"
            breakdown_lines.append(f"- Fresh within attempt: `{fresh_value}`")
            breakdown_lines.append(f"- Row count: `{certify_rows_summary['row_count']}`")
            bucket_counts = certify_rows_summary.get("bucket_counts") or {}
            if bucket_counts:
                breakdown_lines.append(
                    "- Bucket counts: "
                    + ", ".join(
                        f"`{bucket}`={bucket_counts.get(bucket, 0)}"
                        for bucket in ("pass", "timeout", "re_wa")
                    )
                )
            if certify_rows_summary.get("timeout_solver_rc_counts"):
                breakdown_lines.append(
                    "- Timeout `solver_rc` counts: "
                    + ", ".join(
                        f"`{item['solver_rc']}`={item['count']}"
                        for item in certify_rows_summary["timeout_solver_rc_counts"]
                    )
                )
            if certify_rows_summary.get("full_lq_timeout_onset_groups"):
                breakdown_lines.append("- First full-quadrant plateau onset groups:")
                for item in certify_rows_summary["full_lq_timeout_onset_groups"]:
                    mode_list = ", ".join(item["modes"])
                    breakdown_lines.append(
                        f"  - `n={item['n']}` -> `{mode_list}` (modes={item['mode_count']})"
                    )
            if certify_rows_summary.get("pre_full_lq_timeout_onsets"):
                breakdown_lines.append("- Earlier timeout onset before full plateau:")
                for item in certify_rows_summary["pre_full_lq_timeout_onsets"]:
                    breakdown_lines.append(
                        "  - "
                        + f"`{item['mode']} n={item['first_timeout_n']}` -> "
                        + f"`timeout={item['first_timeout_total']}` "
                        + f"`pass={item['first_timeout_pass_total']}` "
                        + f"`re/wa={item['first_timeout_re_wa_total']}` "
                        + f"before first full plateau at `n={item['first_full_lq_plateau_n']}`"
                    )
            if certify_rows_summary.get("full_lq_timeout_plateaus"):
                breakdown_lines.append("- Smallest full-quadrant timeout plateaus:")
                for item in certify_rows_summary["full_lq_timeout_plateaus"]:
                    breakdown_lines.append(
                        "  - "
                        + f"`{item['mode']} n={item['n']}` -> "
                        + f"`timeout={item['timeout_total']}` across all `L/Q` quadrants "
                        + f"(pass={item['pass_total']}, re/wa={item['re_wa_total']})"
                    )
            if certify_rows_summary.get("near_lq_timeout_plateaus"):
                breakdown_lines.append("- Near full-quadrant plateaus (one or few passes remain):")
                for item in certify_rows_summary["near_lq_timeout_plateaus"]:
                    breakdown_lines.append(
                        "  - "
                        + f"`{item['mode']} n={item['n']}` -> "
                        + f"`timeout={item['timeout_total']}` "
                        + f"`pass={item['pass_total']}` "
                        + f"`re/wa={item['re_wa_total']}`"
                    )
            if certify_rows_summary.get("mode_n_clusters"):
                breakdown_lines.append("- Dominant failing `(mode, n)` buckets:")
                for cluster in certify_rows_summary["mode_n_clusters"]:
                    breakdown_lines.append(
                        f"  - `{cluster['bucket']}` x{cluster['count']} -> `{cluster['mode']} n={cluster['n']}`"
                    )
            if certify_rows_summary.get("mode_n_lq_clusters"):
                breakdown_lines.append("- Failing `(mode, n, L, Q)` clusters:")
                for cluster in certify_rows_summary["mode_n_lq_clusters"]:
                    breakdown_lines.append(
                        "  - "
                        + f"`{cluster['bucket']}` x{cluster['count']} -> "
                        + f"`{cluster['mode']} n={cluster['n']} L{cluster['shuffle_labels']} Q{cluster['shuffle_queries']}`"
                    )
            if certify_rows_summary.get("label_sensitive_clusters"):
                breakdown_lines.append("- Label-sensitive subclusters (`L1` fails while `L0` still passes):")
                for cluster in certify_rows_summary["label_sensitive_clusters"]:
                    breakdown_lines.append(
                        "  - "
                        + f"`{cluster['mode']} n={cluster['n']}` -> "
                        + f"`L1 failures={cluster['failures_at_l1']}` "
                        + f"(timeouts={cluster['timeouts_at_l1']}, re/wa={cluster['re_wa_at_l1']}) "
                        + f"vs `L0 pass={cluster['passes_at_l0']}` "
                        + f"(timeouts={cluster['timeouts_at_l0']}, re/wa={cluster['re_wa_at_l0']})"
                    )
            if certify_rows_summary.get("pass_frontier"):
                breakdown_lines.append("- Near-limit passing frontier:")
                for row in certify_rows_summary["pass_frontier"]:
                    breakdown_lines.append(
                        "  - "
                        + f"`{row['mode']} n={row['n']} seed={row['seed']} "
                        + f"L{row['shuffle_labels']} Q{row['shuffle_queries']}` -> "
                        + f"`{row['sec']:.6f}s`"
                    )
        else:
            breakdown_lines.append("- No current-attempt certify row summary was available for this failed AC.")
        breakdown_lines.append("")
        breakdown_lines.append("#### Retry-Critical Anchors")
        breakdown_lines.append("")
        if breakdown["retry_critical_anchors"]:
            for anchor in breakdown["retry_critical_anchors"]:
                breakdown_lines.append(f"- Anchor: `{anchor['label']}`")
                breakdown_lines.append(f"  Path: `{anchor['path']}`")
                breakdown_lines.append(f"  Focus range: `{anchor['focus_range']}`")
                breakdown_lines.append(f"  Enclosing symbol: `{anchor['symbol'] or 'none inferred'}`")
                if anchor.get("statement_excerpt"):
                    breakdown_lines.append(f"  Statement excerpt: `{anchor['statement_excerpt']}`")
                breakdown_lines.append(f"  Note: {anchor['note']}")
                if anchor["evidence_lines"]:
                    breakdown_lines.append("  Evidence lines:")
                    for evidence_line in anchor["evidence_lines"]:
                        breakdown_lines.append(f"    - `{evidence_line}`")
                if anchor["code_excerpt"]:
                    breakdown_lines.append("  Code excerpt:")
                    breakdown_lines.append("```text")
                    breakdown_lines.append(anchor["code_excerpt"])
                    breakdown_lines.append("```")
        else:
            breakdown_lines.append("- No retry-critical anchors were resolved for this failed AC.")
        breakdown_lines.append("")
        breakdown_lines.append("#### Code-Structure Hotspots")
        breakdown_lines.append("")
        if filtered_locus_rendered:
            breakdown_lines.append(
                "- Suppressed broad code hotspots for `transport_disconnected_retry`; the filtered boundary section above is the authoritative reread target until direct gate or solver evidence exists."
            )
        elif breakdown["structural_focus"]:
            for hotspot in breakdown["structural_focus"]:
                breakdown_lines.append(f"- File: `{hotspot['path']}`")
                breakdown_lines.append(f"  Observed mentions: `{hotspot['observed_mentions']}`")
                breakdown_lines.append(
                    "  Focus ranges: "
                    + (", ".join(hotspot["focus_ranges"]) if hotspot["focus_ranges"] else "none captured")
                )
                breakdown_lines.append(
                    "  Enclosing symbols: "
                    + (", ".join(hotspot["enclosing_symbols"]) if hotspot["enclosing_symbols"] else "no symbols inferred")
                )
                breakdown_lines.append(f"  Note: {hotspot['note']}")
                breakdown_lines.append(f"  Mtime: `{hotspot['mtime'] or 'unknown'}`")
                if hotspot["evidence_lines"]:
                    breakdown_lines.append("  Evidence lines:")
                    for evidence_line in hotspot["evidence_lines"]:
                        breakdown_lines.append(f"    - `{evidence_line}`")
                if hotspot["code_excerpt"]:
                    breakdown_lines.append("  Code excerpt:")
                    breakdown_lines.append("```text")
                    breakdown_lines.append(hotspot["code_excerpt"])
                    breakdown_lines.append("```")
        else:
            breakdown_lines.append("- No structural hotspots could be resolved for this failed AC.")
        breakdown_lines.append("")
        breakdown_lines.append("#### Focused Artifact Snapshots")
        breakdown_lines.append("")
        if breakdown["artifact_snapshots"]:
            for snapshot in breakdown["artifact_snapshots"]:
                breakdown_lines.append(f"- `{snapshot['label']}` latest: `{snapshot['latest_file'] or 'none'}`")
                breakdown_lines.append(f"  mtime: `{snapshot['latest_mtime'] or 'unknown'}`")
                if snapshot.get("attempt_start"):
                    breakdown_lines.append(f"  attempt start: `{snapshot['attempt_start']}`")
                    freshness_value = "unknown"
                    if snapshot.get("fresh_for_attempt") is True:
                        freshness_value = "yes"
                    elif snapshot.get("fresh_for_attempt") is False:
                        freshness_value = "no"
                    breakdown_lines.append(f"  fresh within attempt: `{freshness_value}`")
                if snapshot["summary_file"]:
                    breakdown_lines.append(f"  summary: `{snapshot['summary_file']}`")
                if snapshot.get("freshness_note"):
                    breakdown_lines.append(f"  freshness note: {snapshot['freshness_note']}")
        else:
            breakdown_lines.append("- No failed-AC-specific artifact roots were mapped.")
        breakdown_lines.append("")
        breakdown_lines.append("#### Failed-AC Trace Excerpt")
        breakdown_lines.append("")
        breakdown_lines.append("```text")
        breakdown_lines.extend(breakdown["trace_excerpt"] or ["(no failed-AC trace excerpt found)"])
        breakdown_lines.append("```")
        breakdown_lines.append("")
    breakdown_lines.append("## Refinement Versus Previous Failure")
    breakdown_lines.append("")
    write_markdown_list(
        breakdown_lines,
        refinement_notes,
        "No prior failure existed to compare against.",
    )
    breakdown_lines.append("")
    breakdown_lines.append("## Next-Retry Analysis Rule")
    breakdown_lines.append("")
    breakdown_lines.append(
        "- Before the next session edits code, read this breakdown, start from the repeated failed AC if one exists, "
        "and inspect the listed phase and code-structure hotspots before running the heavy gate again."
    )
    breakdown_lines.append(
        "- If this breakdown still localizes the failure only at a broad file level, improve the retry analysis "
        "logic itself before the next heavy run so the next capture records narrower symbols, ranges, wrapper "
        "sections, and code excerpts."
    )
    breakdown_lines.append(
        "- When `failure_analysis_state.json` carries retry-specific line ranges or non-code transport/guard anchors, "
        "surface them first as dedicated `Retry-Critical Anchors` or `Filtered Failure Locus` sections before generic hotspots so the next solver session starts from exact slices."
    )

    write_text_output(report_md, "\n".join(report_lines), encoding="utf-8")
    write_text_output(breakdown_md, "\n".join(breakdown_lines), encoding="utf-8")

    report_payload = {
        "attempt": args.attempt,
        "timestamp": capture_timestamp,
        "seed_file": args.seed_file,
        "exit_code": args.exit_code,
        "session_id": session_id,
        "execution_id": execution_id,
        "failed_acs": failed_acs,
        "blocked_acs": blocked_acs,
        "passed_acs": passed_acs,
        "artifact_snapshots": [asdict(snapshot) for snapshot in base_snapshots],
        "global_log_excerpt": global_log_excerpt,
        "workflow_log_tail": clean_log.splitlines()[-120:],
        "breakdown_file": str(breakdown_md),
    }
    breakdown_payload = {
        "attempt": args.attempt,
        "timestamp": capture_timestamp,
        "seed_file": args.seed_file,
        "exit_code": args.exit_code,
        "session_id": session_id,
        "execution_id": execution_id,
        "failed_ac_breakdowns": breakdowns,
        "fallback_failure_breakdown": fallback_breakdown,
        "refinement_notes": refinement_notes,
    }
    write_text_output(report_json, json.dumps(report_payload, indent=2), encoding="utf-8")
    write_text_output(breakdown_json, json.dumps(breakdown_payload, indent=2), encoding="utf-8")

    history_breakdowns = breakdowns
    if not history_breakdowns and fallback_breakdown:
        history_breakdowns = [fallback_breakdown]

    history.append(
        {
            "attempt": args.attempt,
            "timestamp": capture_timestamp,
            "session_id": session_id,
            "execution_id": execution_id,
            "failed_acs": [item[0] for item in failed_acs],
            "failure_types": [item["failure_type"] for item in history_breakdowns],
            "failure_families": [item.get("failure_family") for item in history_breakdowns],
            "top_axes": [
                axis
                for breakdown in history_breakdowns
                for axis in [breakdown.get("primary_axis"), breakdown.get("secondary_axis")]
                if axis
            ],
            "profile_modes": [
                breakdown.get("profile_mode")
                for breakdown in history_breakdowns
                if breakdown.get("profile_mode")
            ],
            "release_diag_phases": [
                breakdown.get("last_release_diag_phase")
                for breakdown in history_breakdowns
                if breakdown.get("last_release_diag_phase")
            ],
            "progress_checkpoint_phases": [
                breakdown.get("last_progress_checkpoint_phase")
                for breakdown in history_breakdowns
                if breakdown.get("last_progress_checkpoint_phase")
            ],
            "next_probe_commands": [
                breakdown.get("next_probe_command")
                for breakdown in history_breakdowns
                if breakdown.get("next_probe_command")
            ],
            "top_phases": [
                phase["phase"]
                for breakdown in history_breakdowns
                for phase in breakdown["phase_summaries"][:2]
            ],
            "top_hotspots": [
                hotspot["path"]
                for breakdown in history_breakdowns
                for hotspot in breakdown["structural_focus"][:3]
            ],
            "top_symbols": [
                symbol
                for breakdown in history_breakdowns
                for hotspot in breakdown["structural_focus"][:3]
                for symbol in hotspot["enclosing_symbols"][:3]
            ],
            "retry_critical_anchors": [
                f"{anchor['path']}:{anchor['focus_range']}"
                for breakdown in history_breakdowns
                for anchor in breakdown.get("retry_critical_anchors", [])[:6]
            ],
            "top_focus_ranges": [
                f"{hotspot['path']}:{focus_range}"
                for breakdown in history_breakdowns
                for hotspot in breakdown["structural_focus"][:3]
                for focus_range in hotspot["focus_ranges"][:3]
            ],
        }
    )
    history = history[-20:]

    copy_output_file(report_md, report_root / "latest_failure_report.md")
    copy_output_file(report_json, report_root / "latest_failure_report.json")
    copy_output_file(breakdown_md, report_root / "latest_failure_breakdown.md")
    copy_output_file(breakdown_json, report_root / "latest_failure_breakdown.json")
    write_text_output(history_path, json.dumps(history, indent=2), encoding="utf-8")
    if workflow_log.exists():
        copy_output_file(workflow_log, report_root / "latest_workflow.log")

    print(f"failure report written: {report_md}")
    print(f"failure breakdown written: {breakdown_md}")
    print(f"latest failure report updated: {report_root / 'latest_failure_report.md'}")
    print(f"latest failure breakdown updated: {report_root / 'latest_failure_breakdown.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
