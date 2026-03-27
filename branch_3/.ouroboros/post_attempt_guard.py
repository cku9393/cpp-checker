#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from dataclasses import asdict, dataclass
from pathlib import Path


PASS_AC_RE = re.compile(r"### AC (\d+): \[PASS\] (.+)")
SUSPICIOUS_AC3_PHRASES = (
    "did not rerun the full strong gate",
    "not be a credible pass attempt",
    "isolated correctness-fuzz cases",
    "still timing out",
    "optimizer layer is not recoverable",
    "archived snapshot does not reproduce",
)


@dataclass
class GuardFinding:
    ac_index: int
    ac_text: str
    reason: str
    evidence_lines: list[str]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--workflow-log", required=True)
    parser.add_argument("--attempt-dir", required=True)
    parser.add_argument("--report-root", required=True)
    return parser.parse_args()


def collect_findings(clean_text: str) -> list[GuardFinding]:
    findings: list[GuardFinding] = []
    lines = clean_text.splitlines()
    for idx, line in enumerate(lines):
        match = PASS_AC_RE.search(line)
        if not match:
            continue
        ac_index = int(match.group(1))
        ac_text = match.group(2).strip()
        if ac_index != 3:
            continue
        window = lines[idx : min(len(lines), idx + 14)]
        lowered = "\n".join(window).lower()
        if any(phrase in lowered for phrase in SUSPICIOUS_AC3_PHRASES):
            findings.append(
                GuardFinding(
                    ac_index=ac_index,
                    ac_text=ac_text,
                    reason="suspicious_strong_gate_pass",
                    evidence_lines=[item.strip() for item in window if item.strip()][:12],
                )
            )
    return findings


def main() -> int:
    args = parse_args()
    workflow_log = Path(args.workflow_log)
    attempt_dir = Path(args.attempt_dir)
    report_root = Path(args.report_root)
    attempt_dir.mkdir(parents=True, exist_ok=True)
    report_root.mkdir(parents=True, exist_ok=True)

    clean_text = workflow_log.read_text(errors="replace") if workflow_log.exists() else ""
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
    report_json.write_text(json.dumps(payload, indent=2) + "\n")

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
    report_md.write_text("\n".join(lines))
    latest_json.write_text(report_json.read_text())
    latest_md.write_text(report_md.read_text())

    print(report_md)
    if findings:
        print("suspicious pass evidence detected")
        return 2
    print("attempt guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
