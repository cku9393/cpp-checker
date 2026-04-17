#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path
from typing import Any

SOURCE_SET_A_BASELINE = (
    ("research_branch_workspace_guide", Path("README.md")),
    ("research_resume_readme", Path("boj28350_resume/README.md")),
    ("research_current_state_summary", Path("boj28350_resume/current_state_summary.md")),
    ("research_next_session_briefing", Path("boj28350_resume/next_session_briefing.md")),
    ("research_pre_rewrite_checkpoint", Path("boj28350_resume/pre_rewrite_checkpoint.md")),
    ("research_pre_rewrite_synthesis_note", Path("boj28350_resume/pre_rewrite_synthesis_note.md")),
    ("research_progress40_derived_reference", Path("boj28350_resume/progress40_derived_reference.md")),
    ("research_active_solver", Path("boj28350_resume/boj28350_branch_3_solver.cpp")),
    ("research_master_document", Path("boj28350_complete_master_document_partA_raw.md")),
    ("research_integrated_technical_history", Path("boj28350_integrated_technical_history.md")),
    ("research_progress7_bcdecomp_report", Path("boj28350_literature_progress7_bcdecomp_report.md")),
    ("research_literature_grade_proof_package", Path("literature_grade_proof_package.md")),
)

SOURCE_SET_B_BASELINE = (
    (
        "research_progress40_source",
        Path("boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp"),
    ),
    (
        "research_progress40_report",
        Path("boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md"),
    ),
    (
        "research_progress40_results",
        Path("boj28350_bundle_archive/boj28350_progress40_results_merged.json"),
    ),
)

MANDATORY_RESEARCH_BASELINE = SOURCE_SET_A_BASELINE + SOURCE_SET_B_BASELINE
RESEARCH_REVIEW_CHECKPOINTS = (
    Path("boj28350_resume/pre_rewrite_checkpoint.md"),
    Path("boj28350_resume/pre_rewrite_synthesis_note.md"),
)
SOURCE_SET_A_RE = re.compile(r"reviewed source set A.*COMPLETE", re.IGNORECASE)
SOURCE_SET_B_RE = re.compile(r"reviewed source set B.*COMPLETE", re.IGNORECASE)


def resolve_branch_path(branch_root: Path, relative_path: Path) -> Path:
    resolved = (branch_root / relative_path).resolve()
    try:
        resolved.relative_to(branch_root)
    except ValueError as exc:
        raise ValueError(
            f"pre-rewrite research baseline path escaped branch root: {resolved}"
        ) from exc
    return resolved


def _resolve_source_set(branch_root: Path, source_set: tuple[tuple[str, Path], ...]) -> list[tuple[str, Path]]:
    return [
        (label, resolve_branch_path(branch_root, relative_path))
        for label, relative_path in source_set
    ]


def load_research_review_gate(branch_root: Path) -> dict[str, Any]:
    branch_root = branch_root.resolve()
    source_set_a = _resolve_source_set(branch_root, SOURCE_SET_A_BASELINE)
    source_set_b = _resolve_source_set(branch_root, SOURCE_SET_B_BASELINE)

    missing_baseline_paths = [
        str(path)
        for _, path in (*source_set_a, *source_set_b)
        if not path.is_file()
    ]
    if missing_baseline_paths:
        raise FileNotFoundError(
            "pre-rewrite research baseline is missing required branch-local notes or "
            f"bundled progress40 materials: {', '.join(missing_baseline_paths)}"
        )

    checkpoint_files: list[str] = []
    checkpoint_statuses: list[dict[str, Any]] = []
    incomplete_checkpoints: list[str] = []
    for relative_path in RESEARCH_REVIEW_CHECKPOINTS:
        checkpoint_path = resolve_branch_path(branch_root, relative_path)
        if not checkpoint_path.is_file():
            raise FileNotFoundError(
                f"pre-rewrite research review checkpoint is missing: {checkpoint_path}"
            )
        text = checkpoint_path.read_text(encoding="utf-8")
        reviewed_source_set_a_complete = bool(SOURCE_SET_A_RE.search(text))
        reviewed_source_set_b_complete = bool(SOURCE_SET_B_RE.search(text))
        checkpoint_files.append(str(checkpoint_path))
        checkpoint_statuses.append(
            {
                "path": str(checkpoint_path),
                "reviewed_source_set_a_complete": reviewed_source_set_a_complete,
                "reviewed_source_set_b_complete": reviewed_source_set_b_complete,
            }
        )
        if not reviewed_source_set_a_complete or not reviewed_source_set_b_complete:
            incomplete_checkpoints.append(str(checkpoint_path))

    if incomplete_checkpoints:
        raise ValueError(
            "pre-rewrite research review evidence must record both "
            "'reviewed source set A ... COMPLETE' and "
            "'reviewed source set B ... COMPLETE' in each checkpoint file before "
            f"solver rewrite/pivot retries: {', '.join(incomplete_checkpoints)}"
        )

    return {
        "status": "validated",
        "reviewed_source_set_a_complete": True,
        "reviewed_source_set_b_complete": True,
        "checkpoint_files": checkpoint_files,
        "checkpoint_statuses": checkpoint_statuses,
        "source_set_a_paths": [str(path) for _, path in source_set_a],
        "source_set_b_paths": [str(path) for _, path in source_set_b],
        "required_baseline_paths": [str(path) for _, path in (*source_set_a, *source_set_b)],
    }
