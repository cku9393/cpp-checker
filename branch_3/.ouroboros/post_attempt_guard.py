#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from retry_artifact_io import prepare_output_dir, write_text_output


ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]")
AC_HEADER_RE = re.compile(r"^[^#\n]*### AC (\d+): \[(PASS|FAIL|BLOCKED)\] (.+)$")

SUSPICIOUS_AC3_PHRASES = (
    "did not rerun the full strong gate",
    "not be a credible pass attempt",
    "isolated correctness-fuzz cases",
    "still timing out",
    "optimizer layer is not recoverable",
    "archived snapshot does not reproduce",
)
DIRECT_EVIDENCE_TOKENS = {
    3: (
        'status=pass',
        'verdict: pass',
        '"verdict": "pass"',
        'overall verdict: pass',
    ),
    4: (
        'both runs pass',
        'two consecutive',
        'two-pass closure',
        'run 1: pass',
        'run 2: pass',
        'run01=pass',
        'run02=pass',
    ),
    5: (
        'status=pass',
        'verdict: pass',
        '"verdict": "pass"',
        'overall verdict: pass',
    ),
    6: (
        'both runs pass',
        'two consecutive',
        'two-pass closure',
        'run 1: pass',
        'run 2: pass',
        'run01=pass',
        'run02=pass',
    ),
}
CONTRADICTORY_PASS_PHRASES = {
    3: SUSPICIOUS_AC3_PHRASES
    + (
        "remaining bad cases",
        "bad cases",
        "timeout cluster",
        "latest_failure",
    ),
    4: (
        "choose one path",
        "i’ll continue",
        "i'll continue",
        "rerun ac4",
        "recover the last strong-gate-passing line",
        "fix the strong-gate regression",
        "still holding the branch-local lock",
        "prevented me from signaling it directly",
    ),
    5: (
        "decomposed into 4 sub-acs",
        "latest_failure",
        "placeholder",
    ),
    6: (
        "not good enough for another formal boj retry",
        "would almost certainly reproduce",
        "timeout cluster",
        "direct release probe",
        "did not change the zero-reuse outcome",
    ),
}
QA_FAIL_TOKENS = (
    "verdict: fail",
    "loop action: escalate",
)


@dataclass
class GuardFinding:
    ac_index: int
    ac_text: str
    reason: str
    evidence_lines: list[str]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--branch-root", default="")
    parser.add_argument("--workflow-log", required=True)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    return parser.parse_args()


def _load_artifact_guard(branch_root: Path):
    sys.path.insert(0, str(branch_root))
    from artifact_paths import ensure_under_artifacts  # type: ignore

    return ensure_under_artifacts


def _resolve_artifact_path(branch_root: Path, ensure_under_artifacts, value: str) -> Path:
    path = Path(value).expanduser()
    resolved = path if path.is_absolute() else (branch_root / path).resolve()
    return ensure_under_artifacts(resolved)


def strip_ansi(raw_text: str) -> str:
    return ANSI_ESCAPE_RE.sub("", raw_text)


def iter_ac_sections(clean_text: str) -> list[tuple[int, str, list[str]]]:
    lines = clean_text.splitlines()
    headers: list[tuple[int, int, str]] = []
    for idx, line in enumerate(lines):
        match = AC_HEADER_RE.match(line.strip())
        if match:
            headers.append((idx, int(match.group(1)), match.group(3).strip()))

    sections: list[tuple[int, str, list[str]]] = []
    for pos, (start_idx, ac_index, ac_text) in enumerate(headers):
        end_idx = headers[pos + 1][0] if pos + 1 < len(headers) else len(lines)
        section_lines = [item.rstrip() for item in lines[start_idx:end_idx] if item.strip()]
        sections.append((ac_index, ac_text, section_lines))
    return sections


def collect_findings(clean_text: str) -> list[GuardFinding]:
    findings: list[GuardFinding] = []
    lowered_text = clean_text.lower()
    if "qa verdict" in lowered_text and any(token in lowered_text for token in QA_FAIL_TOKENS):
        qa_lines = [item.strip() for item in clean_text.splitlines() if item.strip()]
        focus: list[str] = []
        for idx, line in enumerate(qa_lines):
            lower_line = line.lower()
            if "qa verdict" in lower_line or "verdict: fail" in lower_line or "loop action:" in lower_line:
                focus.extend(qa_lines[idx : min(len(qa_lines), idx + 18)])
        findings.append(
            GuardFinding(
                ac_index=0,
                ac_text="post_execution_qa",
                reason="qa_verdict_fail",
                evidence_lines=focus[:16] or ["QA verdict marked fail/escalate in workflow log."],
            )
        )

    for ac_index, ac_text, section_lines in iter_ac_sections(clean_text):
        if ac_index not in DIRECT_EVIDENCE_TOKENS:
            continue
        lowered = "\n".join(section_lines).lower()
        has_direct_evidence = any(token in lowered for token in DIRECT_EVIDENCE_TOKENS[ac_index])
        contradiction = next(
            (phrase for phrase in CONTRADICTORY_PASS_PHRASES.get(ac_index, ()) if phrase in lowered),
            None,
        )

        if not has_direct_evidence:
            findings.append(
                GuardFinding(
                    ac_index=ac_index,
                    ac_text=ac_text,
                    reason="missing_direct_gate_evidence",
                    evidence_lines=section_lines[:14],
                )
            )
            continue
        if contradiction:
            findings.append(
                GuardFinding(
                    ac_index=ac_index,
                    ac_text=ac_text,
                    reason=f"contradictory_pass_text:{contradiction}",
                    evidence_lines=section_lines[:14],
                )
            )
    return findings


def main() -> int:
    args = parse_args()
    branch_root = (
        Path(args.branch_root).expanduser().resolve()
        if args.branch_root
        else Path(__file__).resolve().parent.parent
    )
    ensure_under_artifacts = _load_artifact_guard(branch_root)
    workflow_log = _resolve_artifact_path(branch_root, ensure_under_artifacts, args.workflow_log)
    attempt_dir = _resolve_artifact_path(branch_root, ensure_under_artifacts, args.attempt_dir)
    report_root = _resolve_artifact_path(branch_root, ensure_under_artifacts, args.report_root)
    prepare_output_dir(attempt_dir)
    prepare_output_dir(report_root)

    clean_text = strip_ansi(workflow_log.read_text(errors="replace")) if workflow_log.exists() else ""
    findings = collect_findings(clean_text)
    payload = {
        "guard_passed": not findings,
        "finding_count": len(findings),
        "findings": [asdict(item) for item in findings],
    }

    report_json = attempt_dir / "attempt_guard.json"
    report_md = attempt_dir / "attempt_guard.md"
    latest_json = report_root / "latest_attempt_guard.json"
    latest_md = report_root / "latest_attempt_guard.md"
    write_text_output(report_json, json.dumps(payload, indent=2) + "\n")

    lines = [
        "# Attempt Guard Report",
        "",
        f"- Guard passed: `{'yes' if payload['guard_passed'] else 'no'}`",
        f"- Findings: `{len(findings)}`",
        "",
    ]
    if findings:
        for finding in findings:
            lines.append(f"## AC {finding.ac_index}: {finding.ac_text}")
            lines.append("")
            lines.append(f"- Reason: `{finding.reason}`")
            lines.append("- Evidence:")
            for evidence in finding.evidence_lines:
                lines.append(f"  - `{evidence}`")
            lines.append("")
    else:
        lines.append("No suspicious PASS findings were detected.")
        lines.append("")
    write_text_output(report_md, "\n".join(lines))
    write_text_output(latest_json, report_json.read_text())
    write_text_output(latest_md, report_md.read_text())

    print(report_md)
    if findings:
        print("suspicious pass evidence detected")
        return 2
    print("attempt guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
