#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
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


ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
SESSION_RE = re.compile(r"\borch_[A-Za-z0-9]+\b")
EXEC_RE = re.compile(r"\bexec_[A-Za-z0-9]+\b")
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
AC_TRACE_LINE_RE = re.compile(r"\s*(?:AC \d+|Sub-AC \d+ of AC \d+)\s+→")
ENABLE_FLAG_RE = re.compile(r"\bENABLE_[A-Z0-9_]+\b")
PROFILE_MODE_RE = re.compile(r"\bPROFILE_(?:NONE|BASE|SAMPLED)\b")
PROGRESS_PHASE_RE = re.compile(r"\[progress\]\s+phase=([A-Za-z0-9_./-]+)")
RELEASE_DIAG_PHASE_RE = re.compile(r"\[release_diag\]\s+phase=([A-Za-z0-9_./-]+)")
PROBE_ACTIVE_GATE_RE = re.compile(
    r"another (?P<gate>lca_[a-z0-9_]+\.sh) run is active \(pid (?P<pid>\d+)\)"
)

CODE_SUFFIXES = {".py", ".sh", ".cpp", ".hpp", ".cc", ".cxx"}
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
        "suite_utils.py",
        "solver_release_env.sh",
        "build.py",
        "boj28350_resume.py",
        "boj28350_resume/boj28350_branch_3_solver.cpp",
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
    ],
    6: [
        "lca_boj3s_gate.sh",
        "outer_suite_wrappers/lca_boj3s_gate.sh",
        "branch_certify_suite.py",
        "suite_utils.py",
        "boj28350_resume/boj28350_branch_3_solver.cpp",
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


@dataclass
class ArtifactSnapshot:
    label: str
    latest_file: str | None
    latest_mtime: str | None
    summary_file: str | None
    summary_excerpt: str | None


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
    if path.startswith("branch_3/"):
        path = path.split("/", 1)[1]
    return path


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


def build_artifact_snapshot(label: str, root: Path) -> ArtifactSnapshot:
    latest = latest_files(root, count=1)
    latest_file = str(latest[0]) if latest else None
    latest_mtime = safe_mtime_label(latest[0]) if latest else None
    summary = latest_summary_file(root)
    return ArtifactSnapshot(
        label=label,
        latest_file=str(summary or latest_file) if (summary or latest_file) else None,
        latest_mtime=latest_mtime,
        summary_file=str(summary) if summary else None,
        summary_excerpt=read_excerpt(summary),
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--attempt", required=True, type=int)
    parser.add_argument("--seed-file", required=True)
    parser.add_argument("--workflow-log", required=True)
    parser.add_argument("--report-root", required=True)
    parser.add_argument("--exit-code", required=True, type=int)
    return parser.parse_args()


def collect_ac_context_lines(clean_log: str, ac_numbers: Iterable[int], forward_lines: int = 2) -> list[str]:
    numbers = list(ac_numbers)
    if not numbers:
        return []
    lines = clean_log.splitlines()
    selected: set[int] = set()
    patterns = [re.compile(rf"(?:Sub-AC \d+ of AC {ac}\b|AC {ac}\b|### AC {ac}\b)") for ac in numbers]
    for idx, line in enumerate(lines):
        if any(pattern.search(line) for pattern in patterns):
            for cursor in range(idx, min(len(lines), idx + 1 + forward_lines)):
                if cursor > idx and AC_TRACE_LINE_RE.search(lines[cursor]):
                    break
                selected.add(cursor)
    return [lines[idx] for idx in sorted(selected)][-240:]


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


def extract_focus_ranges(clean_log: str, candidate_paths: Iterable[str]) -> dict[str, list[tuple[int, int]]]:
    path_set = set(candidate_paths)
    ranges: dict[str, list[tuple[int, int]]] = defaultdict(list)
    for regex in (SED_RANGE_RE, NL_RANGE_RE):
        for match in regex.finditer(clean_log):
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


def failure_family_for_ac(clean_log: str, ac_index: int) -> str:
    section_match = re.search(rf"### AC {ac_index}:.*?(?=\n### AC |\Z)", clean_log, re.DOTALL)
    section = section_match.group(0).lower() if section_match else clean_log.lower()
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


def progress40_axis_breakdown(
    branch_root: Path,
    ac_index: int,
    relevant_lines: Iterable[str],
    structural_focus: list[StructuralFocus],
    clean_log: str,
    probe_signal: ProbeSignal | None = None,
) -> Progress40AxisSummary:
    relevant_list = list(relevant_lines)
    summary_info = current_progress40_summary(branch_root)

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
    if score:
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

    if probe_signal_is_quick_fail_lock(probe_signal, ac_index):
        primary_axis = summary_info.get("pivot_axis") or primary_axis or DEFAULT_AXIS_BY_AC.get(ac_index)
        secondary_axis = None

    profile_mode = profile_mode_for_text(clean_log)
    enabled_flags = enabled_flags_for_text(clean_log)
    failure_family = failure_family_for_ac(clean_log, ac_index)

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
    counter: Counter[str] = Counter()
    samples: dict[str, str] = {}
    for line in list(relevant_lines) + probe_context_lines_for_ac(probe_signal, ac_index):
        for phase, markers in PHASE_RULES:
            if any(marker in line for marker in markers):
                counter[phase] += 1
                samples.setdefault(phase, line.strip())
                break
    return [
        PhaseSummary(phase=phase, count=count, sample=samples[phase])
        for phase, count in counter.most_common(6)
    ]


def structural_focus_for_ac(
    branch_root: Path,
    clean_log: str,
    ac_index: int,
    analysis_state: dict | None = None,
    probe_signal: ProbeSignal | None = None,
) -> list[StructuralFocus]:
    relevant_lines = collect_ac_context_lines(clean_log, [ac_index])
    probe_context = probe_context_lines_for_ac(probe_signal, ac_index)
    relevant_lines.extend(probe_context)
    file_mentions = extract_file_mentions(relevant_lines)
    for hinted_path in AC_FILE_HINTS.get(ac_index, []):
        file_mentions.setdefault(hinted_path, 0)
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
    focus_ranges = extract_focus_ranges(clean_log, candidate_paths)

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
    candidate_paths = list(file_mentions.keys())
    hinted_paths = {
        canonical_focus_path(branch_root, path) for path in AC_FILE_HINTS.get(ac_index, [])
    }
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

    ranked_paths = sorted(
        candidate_paths,
        key=lambda path: (
            file_mentions[path],
            path == inferred_wrapper_canonical,
            path in hinted_paths,
            path,
        ),
        reverse=True,
    )

    focuses: list[StructuralFocus] = []
    for path in ranked_paths[:8]:
        resolved = resolve_repo_path(branch_root, path)
        if resolved is None or resolved.suffix not in CODE_SUFFIXES:
            continue
        ranges = focus_ranges.get(path, [])
        symbol_entries = select_symbol_entries(symbol_ranges_for_path(resolved), ranges)
        symbols = [f"{entry[2]} {entry[3]} [{entry[0]}-{entry[1]}]" for entry in symbol_entries]
        evidence_lines = evidence_lines_for_path(relevant_lines, path)
        if inferred_wrapper_canonical is not None and path == inferred_wrapper_canonical:
            for line in inferred_wrapper_evidence:
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
        if path in AC_FILE_HINTS.get(ac_index, []):
            note_parts.append("mapped from failed AC semantics")
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


def artifact_snapshots_for_failed_acs(branch_root: Path, ac_numbers: Iterable[int]) -> list[ArtifactSnapshot]:
    seen: set[str] = set()
    snapshots: list[ArtifactSnapshot] = []
    for ac_index in ac_numbers:
        for label, relative_root in AC_ARTIFACT_HINTS.get(ac_index, []):
            key = f"{label}:{relative_root}"
            if key in seen:
                continue
            seen.add(key)
            snapshots.append(build_artifact_snapshot(label, branch_root / relative_root))
    return snapshots


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


def write_markdown_list(lines: list[str], items: Iterable[str], empty_text: str) -> None:
    values = list(items)
    if not values:
        lines.append(f"- {empty_text}")
        return
    for item in values:
        lines.append(f"- {item}")


def main() -> int:
    args = parse_args()
    branch_root = Path(
        "/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3"
    )
    workflow_log = Path(args.workflow_log)
    report_root = Path(args.report_root)
    report_root.mkdir(parents=True, exist_ok=True)
    attempt_dir = workflow_log.parent

    previous_report = load_json(report_root / "latest_failure_report.json")
    previous_breakdown = load_json(report_root / "latest_failure_breakdown.json")
    analysis_state_path = branch_root / ".ouroboros/failure_analysis_state.json"
    analysis_state = load_json(analysis_state_path)
    history_path = report_root / "failure_history.json"
    history_payload = load_json(history_path)
    history = history_payload if isinstance(history_payload, list) else []
    probe_signal = load_latest_next_probe_signal(branch_root)

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
        build_artifact_snapshot("smoke", branch_root / "artifacts/lca_tree_stress_v5/smoke"),
        build_artifact_snapshot("strong_gate", branch_root / "artifacts/lca_tree_stress_v5/strong_gate"),
        build_artifact_snapshot("boj3s_gate", branch_root / "artifacts/lca_tree_stress_v5/boj3s_gate"),
        build_artifact_snapshot("hunt", branch_root / "artifacts/lca_tree_stress_v5/hunt"),
    ]
    focused_snapshots = artifact_snapshots_for_failed_acs(branch_root, failed_ac_numbers)

    breakdowns: list[dict] = []
    for ac_index_str, ac_text in failed_acs:
        ac_index = int(ac_index_str)
        relevant_lines = collect_ac_context_lines(clean_log, [ac_index])
        structural_focus = structural_focus_for_ac(
            branch_root,
            clean_log,
            ac_index,
            analysis_state=analysis_state if isinstance(analysis_state, dict) else None,
            probe_signal=probe_signal,
        )
        axis_summary = progress40_axis_breakdown(
            branch_root,
            ac_index,
            relevant_lines,
            structural_focus,
            clean_log,
            probe_signal=probe_signal,
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
                "focus_range": (
                    range_label(*probe_signal.focus_range)
                    if probe_signal.focus_range is not None
                    else None
                ),
                "focus_symbol": probe_signal.focus_symbol,
                "quick_fail_lock": probe_signal_is_quick_fail_lock(probe_signal, ac_index),
            }
        breakdowns.append(
            {
                "ac_index": ac_index_str,
                "ac_text": ac_text,
                "failure_type": failure_type_for_ac(clean_log, ac_index),
                "failure_family": axis_summary.failure_family,
                "primary_axis": axis_summary.primary_axis,
                "secondary_axis": axis_summary.secondary_axis,
                "axis_evidence": axis_summary.axis_evidence,
                "profile_mode": axis_summary.profile_mode,
                "enabled_flags": axis_summary.enabled_flags,
                "last_release_diag_phase": axis_summary.last_release_diag_phase,
                "last_progress_checkpoint_phase": axis_summary.last_progress_checkpoint_phase,
                "current_summary_pivot": axis_summary.current_summary_pivot,
                "current_summary_residual_axes": axis_summary.current_summary_residual_axes,
                "next_probe_command": axis_summary.next_probe_command,
                "interpretation_lane": axis_summary.interpretation_lane,
                "trace_excerpt": relevant_lines[-80:],
                "phase_summaries": [
                    asdict(item)
                    for item in phase_breakdown(relevant_lines, ac_index, probe_signal=probe_signal)
                ],
                "structural_focus": [asdict(item) for item in structural_focus],
                "artifact_snapshots": [
                    asdict(snapshot)
                    for snapshot in artifact_snapshots_for_failed_acs(branch_root, [ac_index])
                ],
                "latest_probe_signal": latest_probe_signal,
            }
        )

    refinement_notes = build_refinement_notes(
        failed_acs,
        breakdowns,
        previous_report,
        previous_breakdown,
        probe_signal=probe_signal,
    )

    report_md = attempt_dir / "failure_report.md"
    report_json = attempt_dir / "failure_report.json"
    breakdown_md = attempt_dir / "failure_breakdown.md"
    breakdown_json = attempt_dir / "failure_breakdown.json"

    report_lines: list[str] = []
    report_lines.append(f"# Failure Report: Attempt {args.attempt}")
    report_lines.append("")
    report_lines.append(f"- Timestamp: `{stable_timestamp()}`")
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
    breakdown_lines.append(f"- Timestamp: `{stable_timestamp()}`")
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
        breakdown_lines.append("#### Code-Structure Hotspots")
        breakdown_lines.append("")
        if breakdown["structural_focus"]:
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
                if snapshot["summary_file"]:
                    breakdown_lines.append(f"  summary: `{snapshot['summary_file']}`")
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

    report_md.write_text("\n".join(report_lines), encoding="utf-8")
    breakdown_md.write_text("\n".join(breakdown_lines), encoding="utf-8")

    report_payload = {
        "attempt": args.attempt,
        "timestamp": stable_timestamp(),
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
        "timestamp": stable_timestamp(),
        "seed_file": args.seed_file,
        "exit_code": args.exit_code,
        "session_id": session_id,
        "execution_id": execution_id,
        "failed_ac_breakdowns": breakdowns,
        "refinement_notes": refinement_notes,
    }
    report_json.write_text(json.dumps(report_payload, indent=2), encoding="utf-8")
    breakdown_json.write_text(json.dumps(breakdown_payload, indent=2), encoding="utf-8")

    history.append(
        {
            "attempt": args.attempt,
            "timestamp": stable_timestamp(),
            "session_id": session_id,
            "execution_id": execution_id,
            "failed_acs": [item[0] for item in failed_acs],
            "failure_types": [item["failure_type"] for item in breakdowns],
            "failure_families": [item.get("failure_family") for item in breakdowns],
            "top_axes": [
                axis
                for breakdown in breakdowns
                for axis in [breakdown.get("primary_axis"), breakdown.get("secondary_axis")]
                if axis
            ],
            "profile_modes": [
                breakdown.get("profile_mode")
                for breakdown in breakdowns
                if breakdown.get("profile_mode")
            ],
            "release_diag_phases": [
                breakdown.get("last_release_diag_phase")
                for breakdown in breakdowns
                if breakdown.get("last_release_diag_phase")
            ],
            "progress_checkpoint_phases": [
                breakdown.get("last_progress_checkpoint_phase")
                for breakdown in breakdowns
                if breakdown.get("last_progress_checkpoint_phase")
            ],
            "next_probe_commands": [
                breakdown.get("next_probe_command")
                for breakdown in breakdowns
                if breakdown.get("next_probe_command")
            ],
            "top_phases": [
                phase["phase"]
                for breakdown in breakdowns
                for phase in breakdown["phase_summaries"][:2]
            ],
            "top_hotspots": [
                hotspot["path"]
                for breakdown in breakdowns
                for hotspot in breakdown["structural_focus"][:3]
            ],
            "top_symbols": [
                symbol
                for breakdown in breakdowns
                for hotspot in breakdown["structural_focus"][:3]
                for symbol in hotspot["enclosing_symbols"][:3]
            ],
            "top_focus_ranges": [
                f"{hotspot['path']}:{focus_range}"
                for breakdown in breakdowns
                for hotspot in breakdown["structural_focus"][:3]
                for focus_range in hotspot["focus_ranges"][:3]
            ],
        }
    )
    history = history[-20:]

    shutil.copy2(report_md, report_root / "latest_failure_report.md")
    shutil.copy2(report_json, report_root / "latest_failure_report.json")
    shutil.copy2(breakdown_md, report_root / "latest_failure_breakdown.md")
    shutil.copy2(breakdown_json, report_root / "latest_failure_breakdown.json")
    history_path.write_text(json.dumps(history, indent=2), encoding="utf-8")
    if workflow_log.exists():
        shutil.copy2(workflow_log, report_root / "latest_workflow.log")

    print(f"failure report written: {report_md}")
    print(f"failure breakdown written: {breakdown_md}")
    print(f"latest failure report updated: {report_root / 'latest_failure_report.md'}")
    print(f"latest failure breakdown updated: {report_root / 'latest_failure_breakdown.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
