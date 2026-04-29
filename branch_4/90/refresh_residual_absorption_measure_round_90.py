#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


D90 = Path(__file__).resolve().parent
B4 = D90.parent
RUNTIME = D90 / "runtime"

SELECTED = "residual_absorption_lexicographic_measure_decreases_or_escape"
BRANCH_CLASSIFICATION = "residual_absorption_branch_classification_contract_ready"
MEASURE_TUPLE = "residual_absorption_measure_tuple_well_founded_proof_ready"
SMALLER_WITNESS = "residual_absorption_smaller_witness_construction_proof_ready_alignment_defect_open"
MEASURE_SKELETON = "proof_ready_skeleton_residual_absorption_measure_decrease_alignment_defect_open"
SOURCE_ALIGNMENT = "proof_ready_skeleton_family_chain_source_alignment_payload_domain_normal_form_open_measure_refined"
FAMILY_STATUS = "partial_family_chain_absorption_residual_measure_proof_ready_shared_domain_normal_form_open"
STATUS_CONGRUENCE = "partial_status_congruence_residual_measure_refined_remaining_project_contract_canonical_domain_normal_open"
SUPPORT_REDUCTION = "partition_ready_residual_measure_refined_remaining_project_contract_canonical_domain_normal_open"
SUPPORT_BOUND = "proof_ready_skeleton_residual_measure_refined_remaining_project_contract_canonical_domain_normal_open"
HIGHER_SUPPORT = "higher_support_deferred_after_residual_absorption_measure_proof_ready_domain_normal_open"
GENERAL_READY = "ready_for_project_to_active_status_domain_refinement"
NEXT1 = "project_to_active_status_domain_refinement"
NEXT2 = "contract_equivalent_domain_normal_form_refinement"
NEXT3 = "canonical_compression_domain_normal_form_refinement"


def write_md(path: Path, title: str, body: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f"# {title}\n\n{body.strip()}\n", encoding="utf-8")


def write_metric(path: Path, rows: list[tuple[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("metric\tvalue\n" + "\n".join(f"{k}\t{v}" for k, v in rows) + "\n", encoding="utf-8")


def write_table(path: Path, header: list[str], rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = ["\t".join(header)]
    for row in rows:
        lines.append("\t".join(str(row.get(col, "")) for col in header))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def md_table(header: list[str], rows: list[dict[str, str]]) -> str:
    out = ["| " + " | ".join(header) + " |", "| " + " | ".join(["---"] * len(header)) + " |"]
    for row in rows:
        out.append("| " + " | ".join(str(row.get(col, "")).replace("|", "/") for col in header) + " |")
    return "\n".join(out)


def metric_table(rows: list[tuple[str, str]]) -> str:
    return md_table(["metric", "value"], [{"metric": k, "value": v} for k, v in rows])


def upsert_section(path: Path, heading: str, body: str) -> None:
    marker = f"\n## {heading}\n"
    existing = path.read_text(encoding="utf-8") if path.exists() else ""
    if marker in existing:
        existing = existing.split(marker, 1)[0].rstrip() + "\n"
    elif existing and not existing.endswith("\n"):
        existing += "\n"
    path.write_text(existing.rstrip() + marker + "\n" + body.strip() + "\n", encoding="utf-8")


scope_header = [
    "statement_key",
    "formal_statement",
    "assumptions",
    "conclusion",
    "residual_branch_scope",
    "measure_scope",
    "uses_source_alignment",
    "uses_payload_alignment",
    "uses_status_domain_alignment",
    "uses_normal_form_alignment",
    "uses_family_chain_depth",
    "uses_higher_support_escape",
    "selected_for_attempt",
    "risk",
    "reason",
]

scope_rows = [
    {
        "statement_key": "residual_absorption_support_measure_decreases",
        "formal_statement": "When residual absorption removes a support-carrying component, support size decreases.",
        "assumptions": "residual branch;removed component remains inactive after normalization",
        "conclusion": "support_size(output)<support_size(source)",
        "residual_branch_scope": "support-removal residual branch",
        "measure_scope": "support size",
        "uses_source_alignment": "1",
        "uses_payload_alignment": "1",
        "uses_status_domain_alignment": "0",
        "uses_normal_form_alignment": "1",
        "uses_family_chain_depth": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "Useful case split, but normalization can preserve support, so it is not a full residual proof.",
    },
    {
        "statement_key": "residual_absorption_family_depth_measure_decreases",
        "formal_statement": "When support size is unchanged, residual absorption decreases family-chain or absorption depth.",
        "assumptions": "residual output keeps support;lifted target is shallower or has lower absorption depth",
        "conclusion": "family_depth(output)<family_depth(source) or absorption_depth(output)<absorption_depth(source)",
        "residual_branch_scope": "same-support residual branch",
        "measure_scope": "family-chain depth / absorption depth",
        "uses_source_alignment": "1",
        "uses_payload_alignment": "1",
        "uses_status_domain_alignment": "1",
        "uses_normal_form_alignment": "1",
        "uses_family_chain_depth": "1",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "This is the intended fallback when support does not decrease, but depth monotonicity remains proof-sketch.",
    },
    {
        "statement_key": "residual_absorption_lexicographic_measure_decreases",
        "formal_statement": "The residual branch decreases a lexicographic tuple of support, active support, canonical rank, family depth, absorption depth, and alignment defects.",
        "assumptions": "residual branch classified;measure tuple well-founded;output witness valid",
        "conclusion": "lex_measure(output)<lex_measure(source)",
        "residual_branch_scope": "classified residual branch",
        "measure_scope": "lexicographic residual absorption tuple",
        "uses_source_alignment": "1",
        "uses_payload_alignment": "1",
        "uses_status_domain_alignment": "1",
        "uses_normal_form_alignment": "1",
        "uses_family_chain_depth": "1",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "high",
        "reason": "Central proof target; current round makes it proof-ready but still depends on alignment-defect and smaller-witness transfer sublemmas.",
    },
    {
        "statement_key": "residual_absorption_measure_or_escape",
        "formal_statement": "If the residual lexicographic measure is not proved to decrease, the branch is a named operation blocker or higher-support escape.",
        "assumptions": "residual branch;no strict measure proof available",
        "conclusion": "named residual-measure blocker or higher-support escape",
        "residual_branch_scope": "nondecreasing or unclassified residual branch",
        "measure_scope": "escape classifier",
        "uses_source_alignment": "1",
        "uses_payload_alignment": "1",
        "uses_status_domain_alignment": "1",
        "uses_normal_form_alignment": "1",
        "uses_family_chain_depth": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "1",
        "risk": "low",
        "reason": "Keeps nondecrease visible and prevents hidden promotion of a smaller-witness proof.",
    },
    {
        "statement_key": "full_absorption_measure_decrease",
        "formal_statement": "Every family-chain absorption residual branch strictly decreases measure.",
        "assumptions": "all residual branches for arbitrary support-growth witnesses",
        "conclusion": "full residual measure decrease",
        "residual_branch_scope": "all family-chain absorption residual branches",
        "measure_scope": "full residual measure",
        "uses_source_alignment": "1",
        "uses_payload_alignment": "1",
        "uses_status_domain_alignment": "1",
        "uses_normal_form_alignment": "1",
        "uses_family_chain_depth": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "0",
        "risk": "high",
        "reason": "Out of scope; proof would require all alignment, status-domain, normal-form, and smaller-witness validity sublemmas.",
    },
]


branch_header = [
    "branch_key",
    "definition",
    "trigger_condition",
    "relation_to_source_alignment",
    "relation_to_lifted_refutation",
    "relation_to_payload_alignment",
    "relation_to_status_domain_alignment",
    "relation_to_normal_form_alignment",
    "expected_measure_effect",
    "current_status",
    "caveat",
]

branch_rows = [
    {
        "branch_key": "direct_source_refutation_branch",
        "definition": "Aligned lifted target refutation directly refutes the source counterexample.",
        "trigger_condition": "payload/domain/normal-form alignment all hold and refutation transfer succeeds",
        "relation_to_source_alignment": "closed aligned case",
        "relation_to_lifted_refutation": "target refutation transfers",
        "relation_to_payload_alignment": "required",
        "relation_to_status_domain_alignment": "required",
        "relation_to_normal_form_alignment": "required",
        "expected_measure_effect": "no smaller witness needed",
        "current_status": "proof_sketch_only_refutation_transfer_open",
        "caveat": "Not residual smaller-witness branch.",
    },
    {
        "branch_key": "lifted_refutation_source_alignment_incomplete_branch",
        "definition": "Target-side refutation exists but source-side transfer is not proved.",
        "trigger_condition": "lifted target refutation available;at least one alignment transfer open",
        "relation_to_source_alignment": "residual alignment defect",
        "relation_to_lifted_refutation": "available target input",
        "relation_to_payload_alignment": "possibly open",
        "relation_to_status_domain_alignment": "possibly open",
        "relation_to_normal_form_alignment": "possibly open",
        "expected_measure_effect": "alignment_defect_count should decrease if residual construction resolves one defect",
        "current_status": BRANCH_CLASSIFICATION,
        "caveat": "Measure decrease not automatic from target refutation.",
    },
    {
        "branch_key": "payload_mismatch_branch",
        "definition": "The source payload has a residual component not matched by the lifted target payload.",
        "trigger_condition": "payload correspondence fails or is only partial",
        "relation_to_source_alignment": "payload alignment defect",
        "relation_to_lifted_refutation": "target refutation may miss source payload",
        "relation_to_payload_alignment": "primary blocker",
        "relation_to_status_domain_alignment": "secondary",
        "relation_to_normal_form_alignment": "secondary",
        "expected_measure_effect": "payload_defect_count decreases or escape",
        "current_status": "proof_sketch_ready",
        "caveat": "Requires payload-defect decrease sublemma.",
    },
    {
        "branch_key": "status_domain_mismatch_branch",
        "definition": "The target refutation domain does not yet embed in the source status-domain.",
        "trigger_condition": "domain map missing or drops status-relevant fields",
        "relation_to_source_alignment": "domain alignment defect",
        "relation_to_lifted_refutation": "target refutation remains target-local",
        "relation_to_payload_alignment": "payload may hold",
        "relation_to_status_domain_alignment": "primary blocker",
        "relation_to_normal_form_alignment": "secondary",
        "expected_measure_effect": "status_domain_defect_count decreases or escape",
        "current_status": "proof_sketch_ready",
        "caveat": "Shared with project-to-active status-domain refinement.",
    },
    {
        "branch_key": "normal_form_mismatch_branch",
        "definition": "Source and target normal forms are not yet compatible for source-status evaluation.",
        "trigger_condition": "target normal form cannot be interpreted in source normal-form predicate",
        "relation_to_source_alignment": "normal-form alignment defect",
        "relation_to_lifted_refutation": "target refutation may change predicate meaning",
        "relation_to_payload_alignment": "payload may hold",
        "relation_to_status_domain_alignment": "domain may hold",
        "relation_to_normal_form_alignment": "primary blocker",
        "expected_measure_effect": "normal_form_defect_count decreases or escape",
        "current_status": "proof_sketch_ready",
        "caveat": "Shared with contract/canonical normal-form transfer.",
    },
    {
        "branch_key": "measure_decrease_branch",
        "definition": "Residual construction produces an output witness with smaller lexicographic residual measure.",
        "trigger_condition": "one support/rank/depth/defect component strictly decreases and earlier components do not increase",
        "relation_to_source_alignment": "uses classified residual defect",
        "relation_to_lifted_refutation": "not direct refutation",
        "relation_to_payload_alignment": "used if payload defect branch",
        "relation_to_status_domain_alignment": "used if domain defect branch",
        "relation_to_normal_form_alignment": "used if normal defect branch",
        "expected_measure_effect": "lexicographic decrease",
        "current_status": MEASURE_SKELETON,
        "caveat": "Proof-ready, not proved.",
    },
    {
        "branch_key": "escape_branch",
        "definition": "Residual branch cannot currently be shown to refute or decrease.",
        "trigger_condition": "no strict measure proof and no direct source refutation proof",
        "relation_to_source_alignment": "alignment or measure escape",
        "relation_to_lifted_refutation": "target refutation insufficient",
        "relation_to_payload_alignment": "may fail",
        "relation_to_status_domain_alignment": "may fail",
        "relation_to_normal_form_alignment": "may fail",
        "expected_measure_effect": "none",
        "current_status": "named_escape_ready",
        "caveat": "Escape is not proof completion.",
    },
    {
        "branch_key": "not_applicable_branch",
        "definition": "Family-chain absorption is not the selected operation for the source witness.",
        "trigger_condition": "source form not recognized or another operation has priority",
        "relation_to_source_alignment": "outside selected source-alignment scope",
        "relation_to_lifted_refutation": "not invoked",
        "relation_to_payload_alignment": "not invoked",
        "relation_to_status_domain_alignment": "not invoked",
        "relation_to_normal_form_alignment": "not invoked",
        "expected_measure_effect": "operation table routes elsewhere",
        "current_status": "classified_not_applicable",
        "caveat": "Does not prove other operation status transfer.",
    },
]


measure_header = [
    "measure_component",
    "definition",
    "well_founded_reason",
    "decreases_in_which_branch",
    "relation_to_support_bound_measure",
    "relation_to_source_alignment",
    "relation_to_status_congruence",
    "current_status",
    "caveat",
]

measure_rows = [
    {
        "measure_component": "support_size",
        "definition": "Number of support coordinates in the residual witness.",
        "well_founded_reason": "natural number",
        "decreases_in_which_branch": "support-removal residual branch",
        "relation_to_support_bound_measure": "first component of support-bound tuple",
        "relation_to_source_alignment": "may be unchanged by alignment-only repairs",
        "relation_to_status_congruence": "smaller support helps reduction branch",
        "current_status": "proved_well_founded_component",
        "caveat": "Not guaranteed to decrease in all residual branches.",
    },
    {
        "measure_component": "active_support_size",
        "definition": "Number of active status-relevant support coordinates.",
        "well_founded_reason": "bounded by support_size",
        "decreases_in_which_branch": "inactive/residual component removal",
        "relation_to_support_bound_measure": "refines support size",
        "relation_to_source_alignment": "tracks active alignment defects",
        "relation_to_status_congruence": "needs status-domain locality",
        "current_status": "proof_sketch_ready",
        "caveat": "Depends on source status-domain transfer.",
    },
    {
        "measure_component": "canonical_motif_rank",
        "definition": "Rank of canonical motif data after residual rewrite.",
        "well_founded_reason": "finite rank over canonical motif inventory",
        "decreases_in_which_branch": "same-support canonical simplification branch",
        "relation_to_support_bound_measure": "compatible with canonical_rank component",
        "relation_to_source_alignment": "secondary when support is unchanged",
        "relation_to_status_congruence": "must not be confused with canonical compression proof",
        "current_status": "proof_sketch_ready",
        "caveat": "Canonical-rank monotonicity under absorption remains open.",
    },
    {
        "measure_component": "family_chain_layer_depth",
        "definition": "Depth of family-chain source layer needed by the residual target.",
        "well_founded_reason": "finite family-chain layer index",
        "decreases_in_which_branch": "shallower lifted target branch",
        "relation_to_support_bound_measure": "matches family_chain_depth component",
        "relation_to_source_alignment": "uses recognized source form",
        "relation_to_status_congruence": "supports residual reduction only after status transfer",
        "current_status": "proof_sketch_ready",
        "caveat": "Lower-layer freshness is not depth decrease proof.",
    },
    {
        "measure_component": "absorption_depth",
        "definition": "Number of remaining absorption steps required by the residual branch.",
        "well_founded_reason": "finite residual construction depth",
        "decreases_in_which_branch": "alignment-fixed same-support branch",
        "relation_to_support_bound_measure": "new residual refinement component",
        "relation_to_source_alignment": "records progress through residual mismatch",
        "relation_to_status_congruence": "helps classify repeated absorption",
        "current_status": "proof_sketch_ready",
        "caveat": "Needs a formal recurrence bound.",
    },
    {
        "measure_component": "source_target_payload_defect_count",
        "definition": "Number of unresolved payload correspondence defects.",
        "well_founded_reason": "finite field count in recognized source-target payload contract",
        "decreases_in_which_branch": "payload mismatch branch",
        "relation_to_support_bound_measure": "residual alignment refinement, below depth",
        "relation_to_source_alignment": "primary payload residual measure",
        "relation_to_status_congruence": "payload relevance for source refutation",
        "current_status": "proof_sketch_ready",
        "caveat": "Requires payload semantic preservation.",
    },
    {
        "measure_component": "status_domain_defect_count",
        "definition": "Number of unresolved source-target status-domain defects.",
        "well_founded_reason": "finite dependency field count",
        "decreases_in_which_branch": "status-domain mismatch branch",
        "relation_to_support_bound_measure": "residual alignment refinement",
        "relation_to_source_alignment": "primary domain residual measure",
        "relation_to_status_congruence": "shared domain blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Next round should address project-to-active status-domain transfer.",
    },
    {
        "measure_component": "normal_form_defect_count",
        "definition": "Number of unresolved source-target normal-form defects.",
        "well_founded_reason": "finite normal-form field count",
        "decreases_in_which_branch": "normal-form mismatch branch",
        "relation_to_support_bound_measure": "residual alignment refinement",
        "relation_to_source_alignment": "primary normal-form residual measure",
        "relation_to_status_congruence": "shared normal-form blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Open with contract/canonical normal-form transfer.",
    },
    {
        "measure_component": "unresolved_alignment_defect_count",
        "definition": "Sum of payload, status-domain, and normal-form unresolved defects.",
        "well_founded_reason": "finite sum of finite natural components",
        "decreases_in_which_branch": "any alignment-defect repair branch",
        "relation_to_support_bound_measure": "last residual-specific tie-breaker",
        "relation_to_source_alignment": "directly measures remaining source-alignment failure",
        "relation_to_status_congruence": "cannot by itself prove status preservation",
        "current_status": MEASURE_TUPLE,
        "caveat": "Decrease proof is conditional on one defect being repaired without earlier increase.",
    },
    {
        "measure_component": "higher_support_escape_rank",
        "definition": "Rank used only to name unresolved true higher-support escape cases.",
        "well_founded_reason": "finite named escape inventory under current scope",
        "decreases_in_which_branch": "not used for proof of smaller witness",
        "relation_to_support_bound_measure": "escape classifier, not reduction measure",
        "relation_to_source_alignment": "names residual branch outside current contract",
        "relation_to_status_congruence": "keeps escape visible",
        "current_status": "named_escape_ready",
        "caveat": "Not a substitute for measure decrease.",
    },
    {
        "measure_component": "lexicographic_tuple",
        "definition": "(support_size, active_support_size, canonical_motif_rank, family_chain_layer_depth, absorption_depth, payload_defects, domain_defects, normal_form_defects, unresolved_alignment_defects)",
        "well_founded_reason": "finite lexicographic product of natural-number components",
        "decreases_in_which_branch": "support, depth, or alignment-defect decrease branch",
        "relation_to_support_bound_measure": "refines lexicographic_support_tuple without weakening it",
        "relation_to_source_alignment": "absorbs residual alignment mismatch into finite defect counts",
        "relation_to_status_congruence": "requires status preservation/reduction separately",
        "current_status": MEASURE_TUPLE,
        "caveat": "Well-foundedness is ready; strict decrease proof remains conditional.",
    },
]


construction_header = [
    "construction_key",
    "residual_branch",
    "input_witness",
    "alignment_defect_used",
    "output_witness",
    "construction_rule",
    "support_before",
    "support_after",
    "canonical_rank_before",
    "canonical_rank_after",
    "family_depth_before",
    "family_depth_after",
    "alignment_defect_before",
    "alignment_defect_after",
    "lexicographic_delta",
    "preserves_counterexample_status",
    "reduces_counterexample_status",
    "refutes_counterexample_status",
    "failure_escape",
    "proof_status",
    "missing_hypothesis",
]

construction_rows = [
    {
        "construction_key": "payload_defect_residual_witness",
        "residual_branch": "payload_mismatch_branch",
        "input_witness": "recognized source witness with payload defect",
        "alignment_defect_used": "source_target_payload_defect_count",
        "output_witness": "payload-repaired residual witness candidate",
        "construction_rule": "replace unmatched payload component by lifted target payload or remove inactive residual component",
        "support_before": "k",
        "support_after": "k or k-minus-positive",
        "canonical_rank_before": "r",
        "canonical_rank_after": "r-or-less",
        "family_depth_before": "d",
        "family_depth_after": "d-or-less",
        "alignment_defect_before": "p>0",
        "alignment_defect_after": "p-1-or-less",
        "lexicographic_delta": "negative_if_no_earlier_component_increases",
        "preserves_counterexample_status": "open",
        "reduces_counterexample_status": "proof_sketch",
        "refutes_counterexample_status": "no",
        "failure_escape": "payload_alignment_escape",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "payload semantic preservation and normalization monotonicity",
    },
    {
        "construction_key": "domain_defect_residual_witness",
        "residual_branch": "status_domain_mismatch_branch",
        "input_witness": "source witness with domain defect",
        "alignment_defect_used": "status_domain_defect_count",
        "output_witness": "domain-refined residual witness candidate",
        "construction_rule": "restrict or refine status-domain dependency map while preserving source predicate meaning",
        "support_before": "k",
        "support_after": "k",
        "canonical_rank_before": "r",
        "canonical_rank_after": "r",
        "family_depth_before": "d",
        "family_depth_after": "d-or-less",
        "alignment_defect_before": "q>0",
        "alignment_defect_after": "q-1-or-less",
        "lexicographic_delta": "negative_if_domain_defect_repairs_and_earlier_components_fixed",
        "preserves_counterexample_status": "open",
        "reduces_counterexample_status": "open",
        "refutes_counterexample_status": "no",
        "failure_escape": "status_domain_escape",
        "proof_status": "blocked_by_status_domain",
        "missing_hypothesis": "source-target status-domain transfer",
    },
    {
        "construction_key": "normal_form_defect_residual_witness",
        "residual_branch": "normal_form_mismatch_branch",
        "input_witness": "source witness with normal-form defect",
        "alignment_defect_used": "normal_form_defect_count",
        "output_witness": "normal-form repaired residual witness candidate",
        "construction_rule": "normalize target interpretation while preserving source status predicate fields",
        "support_before": "k",
        "support_after": "k",
        "canonical_rank_before": "r",
        "canonical_rank_after": "r-or-less",
        "family_depth_before": "d",
        "family_depth_after": "d-or-less",
        "alignment_defect_before": "n>0",
        "alignment_defect_after": "n-1-or-less",
        "lexicographic_delta": "negative_if_normal_defect_repairs_and_no_earlier_increase",
        "preserves_counterexample_status": "open",
        "reduces_counterexample_status": "open",
        "refutes_counterexample_status": "no",
        "failure_escape": "normal_form_escape",
        "proof_status": "blocked_by_normal_form",
        "missing_hypothesis": "normal-form transfer",
    },
    {
        "construction_key": "family_depth_residual_witness",
        "residual_branch": "measure_decrease_branch",
        "input_witness": "same-support residual witness",
        "alignment_defect_used": "family_chain_layer_depth or absorption_depth",
        "output_witness": "shallower residual target witness",
        "construction_rule": "replace residual branch by shallower lifted target after alignment defect is fixed",
        "support_before": "k",
        "support_after": "k",
        "canonical_rank_before": "r",
        "canonical_rank_after": "r",
        "family_depth_before": "d>0",
        "family_depth_after": "d-1-or-less",
        "alignment_defect_before": "a",
        "alignment_defect_after": "a",
        "lexicographic_delta": "negative_if_depth_drops",
        "preserves_counterexample_status": "open",
        "reduces_counterexample_status": "proof_sketch",
        "refutes_counterexample_status": "no",
        "failure_escape": "family_depth_measure_escape",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "family-depth monotonicity under residual absorption",
    },
    {
        "construction_key": "direct_refutation_no_witness",
        "residual_branch": "direct_source_refutation_branch",
        "input_witness": "aligned source witness",
        "alignment_defect_used": "none",
        "output_witness": "none",
        "construction_rule": "target refutation transfers to source; no smaller witness constructed",
        "support_before": "k",
        "support_after": "not_applicable",
        "canonical_rank_before": "r",
        "canonical_rank_after": "not_applicable",
        "family_depth_before": "d",
        "family_depth_after": "not_applicable",
        "alignment_defect_before": "0",
        "alignment_defect_after": "closed",
        "lexicographic_delta": "not_applicable",
        "preserves_counterexample_status": "not_applicable",
        "reduces_counterexample_status": "not_applicable",
        "refutes_counterexample_status": "proof_sketch_open",
        "failure_escape": "refutation_transfer_escape",
        "proof_status": "out_of_scope_for_residual_measure",
        "missing_hypothesis": "lifted-refutation-to-source-refutation transfer",
    },
    {
        "construction_key": "measure_failure_escape",
        "residual_branch": "escape_branch",
        "input_witness": "residual branch without strict delta proof",
        "alignment_defect_used": "unclassified or nondecreasing residual mismatch",
        "output_witness": "none accepted",
        "construction_rule": "name residual measure blocker or higher-support escape",
        "support_before": "k",
        "support_after": "k-or-unknown",
        "canonical_rank_before": "r",
        "canonical_rank_after": "r-or-unknown",
        "family_depth_before": "d",
        "family_depth_after": "d-or-unknown",
        "alignment_defect_before": "a",
        "alignment_defect_after": "a-or-unknown",
        "lexicographic_delta": "not_proved",
        "preserves_counterexample_status": "no_claim",
        "reduces_counterexample_status": "no_claim",
        "refutes_counterexample_status": "no_claim",
        "failure_escape": "named_residual_measure_escape_or_higher_support_escape",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
    },
]


obligation_header = [
    "obligation_key",
    "statement",
    "required_for_selected_statement",
    "existing_verified_inputs",
    "missing_sublemmas",
    "proof_status",
    "dependency_on_measure_tuple",
    "dependency_on_payload_alignment",
    "dependency_on_status_domain",
    "dependency_on_normal_form",
    "dependency_on_family_depth",
    "dependency_on_counterexample_status",
    "dependency_on_higher_support",
    "can_attempt_now",
    "recommended_next_action",
]


def ob(key: str, statement: str, inputs: str, missing: str, status: str, deps: tuple[str, str, str, str, str, str, str], can: str, next_action: str) -> dict[str, str]:
    return {
        "obligation_key": key,
        "statement": statement,
        "required_for_selected_statement": "1",
        "existing_verified_inputs": inputs,
        "missing_sublemmas": missing,
        "proof_status": status,
        "dependency_on_measure_tuple": deps[0],
        "dependency_on_payload_alignment": deps[1],
        "dependency_on_status_domain": deps[2],
        "dependency_on_normal_form": deps[3],
        "dependency_on_family_depth": deps[4],
        "dependency_on_counterexample_status": deps[5],
        "dependency_on_higher_support": deps[6],
        "can_attempt_now": can,
        "recommended_next_action": next_action,
    }


obligation_rows = [
    ob("residual_branch_language_well_defined", "Residual branches are classified apart from direct refutation and not-applicable cases.", "source-alignment skeleton;family-chain status semantics", "", "proved_under_current_scope", ("0", "0", "0", "0", "0", "0", "0"), "1", "use_as_branch_language"),
    ob("residual_measure_tuple_well_defined", "The residual lexicographic measure tuple has named finite components.", "support_bound_measure;source-alignment defect inventories", "", "proved_under_current_scope", ("1", "0", "0", "0", "1", "0", "0"), "1", "use_measure_tuple"),
    ob("residual_measure_well_founded", "The residual tuple is well-founded as a finite lexicographic product of natural measures.", "support_bound_measure;finite alignment defect inventories", "", "proved_under_current_scope", ("1", "0", "0", "0", "1", "0", "0"), "1", "use_well_founded_tuple"),
    ob("payload_mismatch_reduces_alignment_defect", "A payload mismatch repair decreases payload defect count without increasing earlier components.", "payload alignment refinement", "payload semantic preservation", "proof_sketch_ready", ("1", "1", "0", "0", "0", "1", "0"), "1", "payload_alignment_refinement"),
    ob("status_domain_mismatch_reduces_domain_defect", "A status-domain repair decreases domain defect count without changing source predicate meaning.", "status-domain alignment inventory", "source-target domain transfer", "needs_status_domain_sublemma", ("1", "1", "1", "0", "0", "1", "0"), "1", NEXT1),
    ob("normal_form_mismatch_reduces_normal_form_defect", "A normal-form repair decreases normal-form defect count without changing source predicate meaning.", "normal-form alignment inventory", "normal-form transfer", "needs_normal_form_sublemma", ("1", "1", "1", "1", "0", "1", "0"), "1", NEXT2),
    ob("family_depth_decreases_when_alignment_fixed", "If support and rank are unchanged after alignment repair, family-chain or absorption depth decreases.", "family-chain lift map;lower-layer target package", "depth monotonicity", "proof_sketch_ready", ("1", "1", "1", "1", "1", "1", "0"), "1", "family_depth_monotonicity_sublemma"),
    ob("residual_smaller_witness_well_defined", "The residual output is a valid witness candidate when a strict delta branch is selected.", "reduction construction;normal form", "valid reduced witness/status transfer", "needs_smaller_witness_sublemma", ("1", "1", "1", "1", "1", "1", "0"), "1", "smaller_witness_validity_sublemma"),
    ob("residual_smaller_witness_preserves_or_reduces_status", "The residual witness preserves or reduces counterexample status rather than merely changing data.", "status language;source-alignment skeleton", "counterexample-status transfer", "needs_status_domain_sublemma", ("1", "1", "1", "1", "1", "1", "0"), "1", NEXT1),
    ob("residual_measure_decreases_lexicographically", "At least one residual tuple component strictly decreases and no earlier component increases.", "measure tuple;residual construction rows", "normalization monotonicity and defect decrease proof", "proof_sketch_ready", ("1", "1", "1", "1", "1", "1", "0"), "1", "lexicographic_delta_sublemma"),
    ob("measure_failure_is_named_escape", "If no strict delta is proved, the branch is named as residual-measure blocker or higher-support escape.", "higher-support escape interface;operation blocker language", "", "proved_under_current_scope", ("1", "1", "1", "1", "1", "0", "1"), "1", "keep_escape_visible"),
    ob("no_hidden_residual_measure_failure_case", "Every residual failure is payload/domain/normal/depth/smaller-witness open or a named escape.", "branch classification;obligation inventory", "full no-hidden-case proof after transfer sublemmas", "proof_sketch_ready", ("1", "1", "1", "1", "1", "1", "1"), "1", NEXT1),
]


sublemma_header = [
    "sublemma_key",
    "proof_status",
    "assumptions",
    "conclusion",
    "proof_summary",
    "evidence_path",
    "missing_hypothesis",
    "next_action",
]

sublemma_rows = [
    {
        "sublemma_key": "residual_branch_language_well_defined",
        "proof_status": "proved_under_current_scope",
        "assumptions": "source alignment classification;family-chain absorption status language",
        "conclusion": "direct refutation, residual mismatch, decrease, escape, and not-applicable cases are disjointly named",
        "proof_summary": "The branch table separates direct source refutation from residual smaller-witness construction and names every current mismatch axis.",
        "evidence_path": "residual_absorption_branch_classification_90.md",
        "missing_hypothesis": "",
        "next_action": "use_branch_classification",
    },
    {
        "sublemma_key": "residual_measure_tuple_well_defined",
        "proof_status": "proved_under_current_scope",
        "assumptions": "finite support witness;finite recognized source-target fields",
        "conclusion": "the residual measure tuple is well-defined",
        "proof_summary": "Each component is a natural-valued finite count or rank drawn from the support-bound tuple or the finite alignment-defect inventories.",
        "evidence_path": "residual_absorption_measure_tuple_90.md;support_bound_measure_90.md",
        "missing_hypothesis": "",
        "next_action": "use_measure_tuple",
    },
    {
        "sublemma_key": "residual_measure_well_founded",
        "proof_status": "proved_under_current_scope",
        "assumptions": "each tuple component is natural-valued and finite",
        "conclusion": "lexicographic residual measure is well-founded",
        "proof_summary": "A finite lexicographic product of well-founded natural-number measures is well-founded under the current finite normal-form witness scope.",
        "evidence_path": "residual_absorption_measure_tuple_90.md",
        "missing_hypothesis": "",
        "next_action": "prove_strict_delta_by_branch",
    },
    {
        "sublemma_key": "payload_mismatch_reduces_alignment_defect",
        "proof_status": "proof_sketch_only",
        "assumptions": "payload mismatch is selected;payload repair does not increase earlier tuple components",
        "conclusion": "payload defect count decreases",
        "proof_summary": "Repairing one unmatched source-target payload field lowers the finite payload-defect count, but semantic payload preservation remains open.",
        "evidence_path": "residual_absorption_smaller_witness_construction_90.md;family_chain_absorption_payload_alignment_refinement_90.md",
        "missing_hypothesis": "payload semantic preservation",
        "next_action": "payload_alignment_refinement",
    },
    {
        "sublemma_key": "status_domain_mismatch_reduces_domain_defect",
        "proof_status": "proof_sketch_only",
        "assumptions": "domain mismatch is selected;domain repair preserves source predicate meaning",
        "conclusion": "status-domain defect count decreases",
        "proof_summary": "A fieldwise domain repair should lower the finite domain-defect count, but source-target status-domain transfer is still open.",
        "evidence_path": "residual_absorption_smaller_witness_construction_90.md;family_chain_absorption_status_domain_alignment_90.md",
        "missing_hypothesis": "source-target status-domain transfer",
        "next_action": NEXT1,
    },
    {
        "sublemma_key": "normal_form_mismatch_reduces_normal_form_defect",
        "proof_status": "proof_sketch_only",
        "assumptions": "normal-form mismatch is selected;normal repair preserves source status fields",
        "conclusion": "normal-form defect count decreases",
        "proof_summary": "Normal-form repair reduces a finite normal-form defect count, but the normal-form transfer proof is not complete.",
        "evidence_path": "residual_absorption_smaller_witness_construction_90.md;family_chain_absorption_normal_form_alignment_90.md",
        "missing_hypothesis": "normal-form transfer",
        "next_action": NEXT2,
    },
    {
        "sublemma_key": "residual_smaller_witness_well_defined",
        "proof_status": "blocked_by_smaller_witness",
        "assumptions": "residual construction row selected;strict tuple delta candidate",
        "conclusion": "output is a valid smaller witness",
        "proof_summary": "The construction is specified, but witness validity still depends on status-domain, normal-form, and counterexample-status transfer.",
        "evidence_path": "residual_absorption_smaller_witness_construction_90.md",
        "missing_hypothesis": "valid reduced witness and counterexample-status transfer",
        "next_action": NEXT1,
    },
    {
        "sublemma_key": "residual_measure_decreases_lexicographically",
        "proof_status": "blocked_by_status_domain",
        "assumptions": "one tuple component decreases;earlier components do not increase after normalization",
        "conclusion": "lexicographic measure decreases",
        "proof_summary": "The tuple and branch deltas are proof-ready, but strict lexicographic decrease needs domain/normal-form transfer and normalization monotonicity.",
        "evidence_path": "residual_absorption_measure_decrease_skeleton_90.md",
        "missing_hypothesis": "normalization monotonicity plus transfer sublemmas",
        "next_action": NEXT1,
    },
    {
        "sublemma_key": "measure_failure_is_named_escape",
        "proof_status": "proved_under_current_scope",
        "assumptions": "no strict residual measure decrease proof is available",
        "conclusion": "branch is named as residual-measure blocker or higher-support escape",
        "proof_summary": "The escape branch records nondecrease explicitly and does not silently promote a smaller-witness proof.",
        "evidence_path": "higher_support_necessity_after_residual_absorption_measure_90.md;higher_support_escape_interface_90.md",
        "missing_hypothesis": "",
        "next_action": "keep_escape_visible",
    },
]


next_header = [
    "action_key",
    "resolves",
    "prerequisite_status",
    "readiness_score_0_100",
    "proof_value_0_100",
    "engineering_cost_0_100",
    "risk_0_100",
    "dependency_clarity_0_100",
    "expected_progress_value_0_100",
    "recommended_order",
    "final_recommendation",
    "reason",
]

next_rows = [
    {
        "action_key": NEXT1,
        "resolves": "shared_status_domain_transfer",
        "prerequisite_status": "project_to_active_payload_proved_status_domain_open",
        "readiness_score_0_100": "84",
        "proof_value_0_100": "86",
        "engineering_cost_0_100": "61",
        "risk_0_100": "62",
        "dependency_clarity_0_100": "86",
        "expected_progress_value_0_100": "84",
        "recommended_order": "1",
        "final_recommendation": NEXT1,
        "reason": "Residual measure is now proof-ready; the next blocker is source/status-domain transfer, with project-to-active the clearest entry point.",
    },
    {
        "action_key": NEXT2,
        "resolves": "contract_equivalent_quotient_domain_normal_form_transfer",
        "prerequisite_status": "proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open",
        "readiness_score_0_100": "80",
        "proof_value_0_100": "83",
        "engineering_cost_0_100": "63",
        "risk_0_100": "64",
        "dependency_clarity_0_100": "84",
        "expected_progress_value_0_100": "80",
        "recommended_order": "2",
        "final_recommendation": NEXT2,
        "reason": "Contract-equivalent quotient transfer remains a shared domain/normal-form blocker.",
    },
    {
        "action_key": NEXT3,
        "resolves": "canonical_motif_domain_normal_form_transfer",
        "prerequisite_status": "proof_ready_skeleton_canonical_compression_congruence_domain_normal_form_open",
        "readiness_score_0_100": "78",
        "proof_value_0_100": "82",
        "engineering_cost_0_100": "63",
        "risk_0_100": "64",
        "dependency_clarity_0_100": "82",
        "expected_progress_value_0_100": "78",
        "recommended_order": "3",
        "final_recommendation": NEXT3,
        "reason": "Canonical compression still has motif domain/normal-form transfer open.",
    },
    {
        "action_key": "support_bound_completion",
        "resolves": "support_bound_completion_after_operation_transfer",
        "prerequisite_status": SUPPORT_BOUND,
        "readiness_score_0_100": "70",
        "proof_value_0_100": "84",
        "engineering_cost_0_100": "66",
        "risk_0_100": "72",
        "dependency_clarity_0_100": "80",
        "expected_progress_value_0_100": "73",
        "recommended_order": "4",
        "final_recommendation": "support_bound_completion_after_transfer_refinements",
        "reason": "Residual measure is proof-ready, but support-bound completion still depends on operation-local transfer proofs.",
    },
    {
        "action_key": "higher_support_necessity_recheck",
        "resolves": "deferred_higher_support_escape_reassessment",
        "prerequisite_status": HIGHER_SUPPORT,
        "readiness_score_0_100": "69",
        "proof_value_0_100": "76",
        "engineering_cost_0_100": "55",
        "risk_0_100": "64",
        "dependency_clarity_0_100": "82",
        "expected_progress_value_0_100": "72",
        "recommended_order": "5",
        "final_recommendation": "higher_support_necessity_recheck_after_transfer_refinements",
        "reason": "Higher-support scan remains deferred while domain/normal-form operation blockers are still first-order.",
    },
    {
        "action_key": "limited_to_broader_generalization_plan",
        "resolves": "generalization_contract",
        "prerequisite_status": "operation_transfer_refinements_open",
        "readiness_score_0_100": "73",
        "proof_value_0_100": "80",
        "engineering_cost_0_100": "55",
        "risk_0_100": "61",
        "dependency_clarity_0_100": "84",
        "expected_progress_value_0_100": "74",
        "recommended_order": "6",
        "final_recommendation": "limited_to_broader_generalization_plan_after_transfer_refinements",
        "reason": "Planning can continue, but proof completion still depends on transfer sublemmas.",
    },
]


def write_round_docs() -> None:
    write_table(RUNTIME / "residual_absorption_measure_decrease_scope_inventory_90.tsv", scope_header, scope_rows)
    write_md(
        D90 / "residual_absorption_measure_decrease_scope_memo_90.md",
        "Residual Absorption Measure Decrease Scope Memo 90",
        "\n".join(
            [
                "This round proof-attempts only the family-chain absorption residual smaller-witness branch. It does not prove the full general theorem and does not prove full family-chain absorption reduction.",
                "",
                f"Selected statement: `{SELECTED}`.",
                "",
                md_table(scope_header, scope_rows),
            ]
        ),
    )

    write_table(RUNTIME / "residual_absorption_branch_classification_90.tsv", branch_header, branch_rows)
    write_md(
        D90 / "residual_absorption_branch_classification_90.md",
        "Residual Absorption Branch Classification 90",
        "\n".join([f"Final status: `{BRANCH_CLASSIFICATION}`.", "", md_table(branch_header, branch_rows)]),
    )

    write_table(RUNTIME / "residual_absorption_measure_tuple_90.tsv", measure_header, measure_rows)
    write_md(
        D90 / "residual_absorption_measure_tuple_90.md",
        "Residual Absorption Measure Tuple 90",
        "\n".join(
            [
                f"Final status: `{MEASURE_TUPLE}`.",
                "",
                "The tuple is well-founded as a finite lexicographic product of natural-valued support, depth, and alignment-defect components. Strict decrease remains a branch proof obligation.",
                "",
                md_table(measure_header, measure_rows),
            ]
        ),
    )

    write_table(RUNTIME / "residual_absorption_smaller_witness_construction_90.tsv", construction_header, construction_rows)
    write_md(
        D90 / "residual_absorption_smaller_witness_construction_90.md",
        "Residual Absorption Smaller Witness Construction 90",
        "\n".join(
            [
                f"Final status: `{SMALLER_WITNESS}`.",
                "",
                "The residual output witness is specified by branch. Direct refutation produces no smaller witness; nondecreasing branches are named escapes.",
                "",
                md_table(construction_header, construction_rows),
            ]
        ),
    )

    write_table(RUNTIME / "residual_absorption_measure_decrease_obligations_90.tsv", obligation_header, obligation_rows)
    write_md(
        D90 / "residual_absorption_measure_decrease_obligations_90.md",
        "Residual Absorption Measure Decrease Obligations 90",
        "Obligation count: `12`.\n\n" + md_table(obligation_header, obligation_rows),
    )

    write_table(RUNTIME / "residual_absorption_measure_decrease_sublemma_proofs_90.tsv", sublemma_header, sublemma_rows)
    write_md(
        D90 / "residual_absorption_measure_decrease_sublemma_proofs_90.md",
        "Residual Absorption Measure Decrease Sublemma Proofs 90",
        "Sublemma proof-attempt counts: `proved_under_current_scope=4`, `proof_sketch_only=3`, `blocked=2`.\n\n"
        + md_table(sublemma_header, sublemma_rows),
    )

    skeleton_rows = [
        ("lemma_name", "residual_absorption_lexicographic_measure_decreases_or_escape"),
        ("exact_statement", "For a classified residual family-chain absorption branch, either the residual output decreases the well-founded lexicographic tuple or the branch is a named blocker/escape."),
        ("assumptions", "recognized source witness;source-alignment residual classification;well-founded measure tuple;residual construction candidate"),
        ("conclusion", "lexicographic smaller-witness branch or named escape"),
        ("residual_branch_classification", BRANCH_CLASSIFICATION),
        ("measure_tuple", MEASURE_TUPLE),
        ("smaller_witness_construction", SMALLER_WITNESS),
        ("payload_mismatch_case", "proof_sketch_payload_defect_decrease_open"),
        ("status_domain_mismatch_case", "proof_sketch_domain_defect_decrease_open"),
        ("normal_form_mismatch_case", "proof_sketch_normal_defect_decrease_open"),
        ("family_depth_branch", "proof_sketch_family_depth_decrease_open"),
        ("direct_escape_branch", "named_residual_measure_escape_ready"),
        ("measure_decrease_outline", "finite lexicographic tuple; strict branch delta; no earlier component increase"),
        ("counterexample_status_relation", "preservation_or_reduction_open_status_domain_normal_form_transfer_required"),
        ("failure_to_escape_case", "measure_failure_is_named_escape"),
        ("relation_to_source_alignment_skeleton", SOURCE_ALIGNMENT),
        ("relation_to_family_chain_absorption_status_skeleton", FAMILY_STATUS),
        ("relation_to_support_bound_skeleton", SUPPORT_BOUND),
        ("missing_steps", "payload/domain/normal-form transfer; valid smaller-witness status; normalization monotonicity"),
        ("exact_caveat", "proof-ready residual measure skeleton only; not full absorption or support-bound proof"),
        ("final_status", MEASURE_SKELETON),
    ]
    write_metric(RUNTIME / "residual_absorption_measure_decrease_skeleton_90.tsv", skeleton_rows)
    write_md(D90 / "residual_absorption_measure_decrease_skeleton_90.md", "Residual Absorption Measure Decrease Skeleton 90", metric_table(skeleton_rows))

    write_metric(
        RUNTIME / "residual_absorption_measure_decrease_fingerprint_90.tsv",
        [
            ("selected_statement", SELECTED),
            ("branch_classification_status", BRANCH_CLASSIFICATION),
            ("measure_tuple_status", MEASURE_TUPLE),
            ("smaller_witness_construction_status", SMALLER_WITNESS),
            ("skeleton_status", MEASURE_SKELETON),
            ("obligation_count", "12"),
            ("proved_under_current_scope_sublemma_count", "4"),
            ("proof_sketch_only_sublemma_count", "3"),
            ("blocked_sublemma_count", "2"),
        ],
    )
    write_metric(
        RUNTIME / "residual_absorption_measure_tuple_fingerprint_90.tsv",
        [
            ("measure_tuple_status", MEASURE_TUPLE),
            ("component_count", "11"),
            ("well_founded_reason", "finite_lexicographic_product_of_natural_components"),
            ("strict_delta_status", "proof_ready_open"),
        ],
    )
    write_metric(
        RUNTIME / "residual_absorption_branch_classification_fingerprint_90.tsv",
        [
            ("branch_classification_status", BRANCH_CLASSIFICATION),
            ("branch_count", "8"),
            ("direct_refutation_separate", "1"),
            ("residual_smaller_witness_separate", "1"),
            ("escape_branch_named", "1"),
        ],
    )


def update_rollups() -> None:
    measure_rows_summary = [
        ("selected_residual_measure_statement", SELECTED),
        ("residual_branch_classification_status", BRANCH_CLASSIFICATION),
        ("residual_measure_tuple_status", MEASURE_TUPLE),
        ("residual_smaller_witness_construction_status", SMALLER_WITNESS),
        ("residual_measure_skeleton_status", MEASURE_SKELETON),
        ("direct_refutation_branch", "separate_not_smaller_witness"),
        ("measure_failure_escape", "named_residual_measure_or_higher_support_escape"),
        ("final_status", MEASURE_SKELETON),
        ("caveat", "not_full_absorption_measure_decrease_proof"),
    ]
    write_metric(RUNTIME / "family_chain_absorption_measure_decrease_90.tsv", measure_rows_summary)
    upsert_section(D90 / "family_chain_absorption_measure_decrease_90.md", "Residual Measure Decrease Refinement Round", metric_table(measure_rows_summary))

    family_status_rows = [
        ("source_alignment_skeleton_status", SOURCE_ALIGNMENT),
        ("residual_measure_skeleton_status", MEASURE_SKELETON),
        ("family_chain_absorption_status_after_round", FAMILY_STATUS),
        ("lifted_refutation_to_source_status", "lifted_refutation_to_source_refutation_payload_domain_normal_form_open"),
        ("counterexample_status_transfer", "status_domain_normal_form_transfer_open"),
        ("caveat", "residual_measure_proof_ready_not_full_absorption_proof"),
    ]
    write_metric(RUNTIME / "family_chain_absorption_status_skeleton_90.tsv", family_status_rows)
    upsert_section(D90 / "family_chain_absorption_status_skeleton_90.md", "Residual Measure Decrease Refinement Round", metric_table(family_status_rows))

    source_rows = [
        ("source_alignment_skeleton_status", SOURCE_ALIGNMENT),
        ("residual_measure_dependency", MEASURE_SKELETON),
        ("payload_alignment_status", "family_chain_absorption_payload_alignment_proof_ready_source_target_payload_open"),
        ("status_domain_alignment_status", "family_chain_absorption_status_domain_alignment_proof_ready_source_target_domain_open"),
        ("normal_form_alignment_status", "family_chain_absorption_normal_form_alignment_proof_ready_source_target_normal_form_open"),
        ("final_status", SOURCE_ALIGNMENT),
        ("caveat", "source_alignment_still_payload_domain_normal_form_open"),
    ]
    write_metric(RUNTIME / "family_chain_absorption_source_alignment_skeleton_90.tsv", source_rows)
    upsert_section(D90 / "family_chain_absorption_source_alignment_skeleton_90.md", "Residual Measure Decrease Refinement Round", metric_table(source_rows))

    refutation_rows = [
        ("lifted_target_refutation_status", "target_side_refutation_available_under_current_scope"),
        ("source_counterexample_refutation_status", "lifted_refutation_to_source_refutation_payload_domain_normal_form_open"),
        ("residual_measure_status", MEASURE_SKELETON),
        ("final_status", "family_chain_absorption_refutation_lifted_target_ready_source_alignment_open_residual_measure_refined"),
        ("caveat", "direct_refutation_not_promoted_without_payload_domain_normal_form_transfer"),
    ]
    write_metric(RUNTIME / "family_chain_absorption_refutation_lemma_90.tsv", refutation_rows)
    upsert_section(D90 / "family_chain_absorption_refutation_lemma_90.md", "Residual Measure Decrease Refinement Round", metric_table(refutation_rows))

    status_rows = [
        ("family_chain_absorption_status", FAMILY_STATUS),
        ("source_alignment_status", SOURCE_ALIGNMENT),
        ("residual_measure_status", MEASURE_SKELETON),
        ("project_to_active_status", "partial_project_to_active_locality_proof_ready_status_domain_open"),
        ("contract_equivalent_status", "partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open"),
        ("canonical_compression_status", "partial_canonical_compression_congruence_proof_ready_domain_normal_form_open"),
        ("status_congruence_bridge_status", STATUS_CONGRUENCE),
        ("remaining_blockers", "shared_status_domain_normal_form_transfer;project_contract_canonical_transfer"),
        ("caveat", "residual_measure_refined_not_all_operation_proof"),
    ]
    write_metric(RUNTIME / "status_preservation_congruence_skeleton_90.tsv", status_rows)
    upsert_section(D90 / "status_preservation_congruence_skeleton_90.md", "Residual Measure Decrease Refinement Round", metric_table(status_rows))

    op_header = ["operation_key", "operation_status", "status_congruence_status", "measure_status", "remaining_blocker", "next_action"]
    op_rows = [
        {
            "operation_key": "project_to_active",
            "operation_status": "partial_project_to_active_locality_proof_ready_status_domain_open",
            "status_congruence_status": "payload_locality_proved_status_domain_open",
            "measure_status": "not_primary_for_this_operation",
            "remaining_blocker": "status_domain_transfer",
            "next_action": NEXT1,
        },
        {
            "operation_key": "contract_equivalent_support_coordinates",
            "operation_status": "partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open",
            "status_congruence_status": "proof_ready_domain_normal_form_open",
            "measure_status": "measure_decrease_available_separate_from_status",
            "remaining_blocker": "quotient_domain_normal_form_transfer",
            "next_action": NEXT2,
        },
        {
            "operation_key": "canonical_motif_compression",
            "operation_status": "partial_canonical_compression_congruence_proof_ready_domain_normal_form_open",
            "status_congruence_status": "proof_ready_domain_normal_form_open",
            "measure_status": "lexicographic_measure_available_separate_from_status",
            "remaining_blocker": "motif_domain_normal_form_transfer",
            "next_action": NEXT3,
        },
        {
            "operation_key": "family_chain_absorption",
            "operation_status": FAMILY_STATUS,
            "status_congruence_status": SOURCE_ALIGNMENT,
            "measure_status": MEASURE_SKELETON,
            "remaining_blocker": "status_domain_normal_form_transfer_for_valid_smaller_witness",
            "next_action": NEXT1,
        },
    ]
    write_table(RUNTIME / "support_reduction_operation_status_table_90.tsv", op_header, op_rows)
    upsert_section(D90 / "support_reduction_operation_status_table_90.md", "Residual Measure Decrease Refinement Round", md_table(op_header, op_rows))

    reduction_rows = [
        ("support_reduction_step_status", SUPPORT_REDUCTION),
        ("family_chain_absorption_status", FAMILY_STATUS),
        ("residual_measure_status", MEASURE_SKELETON),
        ("source_alignment_status", SOURCE_ALIGNMENT),
        ("project_status_domain_status", "open"),
        ("contract_domain_normal_status", "open"),
        ("canonical_domain_normal_status", "open"),
        ("next_exact_target", NEXT1),
    ]
    write_metric(RUNTIME / "support_reduction_step_skeleton_90.tsv", reduction_rows)
    upsert_section(D90 / "support_reduction_step_skeleton_90.md", "Residual Measure Decrease Refinement Round", metric_table(reduction_rows))

    bound_rows = [
        ("support_bound_lemma_status", SUPPORT_BOUND),
        ("support_reduction_dependency", SUPPORT_REDUCTION),
        ("status_congruence_dependency", STATUS_CONGRUENCE),
        ("residual_measure_dependency", MEASURE_SKELETON),
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("general_theorem_readiness", GENERAL_READY),
        ("next_exact_target", NEXT1),
        ("caveat", "support_bound_not_completed_by_residual_measure_skeleton"),
    ]
    write_metric(RUNTIME / "support_bound_lemma_skeleton_90.tsv", bound_rows)
    upsert_section(D90 / "support_bound_lemma_skeleton_90.md", "Residual Measure Decrease Refinement Round", metric_table(bound_rows))

    higher_header = ["check_key", "finding", "result_status", "next_action"]
    higher_rows = [
        {"check_key": "residual_measure_decrease_closed", "finding": "not fully closed; proof-ready skeleton with transfer and smaller-witness validity open", "result_status": MEASURE_SKELETON, "next_action": NEXT1},
        {"check_key": "family_chain_support_gt8_escape_reduced", "finding": "partially reduced: nondecrease branches are named residual-measure or higher-support escapes", "result_status": HIGHER_SUPPORT, "next_action": "keep_higher_support_deferred"},
        {"check_key": "project_contract_canonical_priority", "finding": "shared domain/normal-form blockers now dominate next operation proof work", "result_status": GENERAL_READY, "next_action": NEXT1},
        {"check_key": "support_bound_completion_ready", "finding": "not yet; residual measure is proof-ready but transfer sublemmas remain open", "result_status": SUPPORT_BOUND, "next_action": "after_transfer_refinements"},
        {"check_key": "higher_support_deferred", "finding": "support9+ scan remains deferred", "result_status": HIGHER_SUPPORT, "next_action": "do_not_scan_support9"},
        {"check_key": "limited_to_broader_generalization_ready", "finding": "not ready for theorem completion; transfer refinements remain", "result_status": GENERAL_READY, "next_action": NEXT1},
    ]
    write_table(RUNTIME / "higher_support_necessity_after_residual_absorption_measure_90.tsv", higher_header, higher_rows)
    write_md(D90 / "higher_support_necessity_after_residual_absorption_measure_90.md", "Higher Support Necessity After Residual Absorption Measure 90", f"Final status: `{HIGHER_SUPPORT}`.\n\n" + md_table(higher_header, higher_rows))

    escape_rows = [
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("residual_measure_escape", "named_residual_measure_or_higher_support_escape"),
        ("support9_scan_status", "not_run"),
        ("next_exact_target", NEXT1),
        ("caveat", "higher_support_escape_visible_not_resolved"),
    ]
    write_metric(RUNTIME / "higher_support_escape_interface_90.tsv", escape_rows)
    upsert_section(D90 / "higher_support_escape_interface_90.md", "Residual Measure Decrease Refinement Round", metric_table(escape_rows))

    bridge_rows = [
        ("general_bridge_obligation_status", "residual_measure_refined_transfer_open"),
        ("residual_measure_status", MEASURE_SKELETON),
        ("source_alignment_status", SOURCE_ALIGNMENT),
        ("family_chain_absorption_status", FAMILY_STATUS),
        ("status_congruence_status", STATUS_CONGRUENCE),
        ("support_reduction_status", SUPPORT_REDUCTION),
        ("support_bound_status", SUPPORT_BOUND),
        ("general_theorem_readiness", GENERAL_READY),
        ("next_exact_target", NEXT1),
    ]
    for name in [
        "general_gap_bridge_obligation_inventory_90",
        "general_gap_bridge_dependency_graph_90",
        "general_gap_bridge_lemma_candidates_90",
        "limited_general_gap_bridge_skeleton_90",
    ]:
        write_metric(RUNTIME / f"{name}.tsv", bridge_rows)
        upsert_section(D90 / f"{name}.md", "Residual Measure Decrease Refinement Round", metric_table(bridge_rows))

    write_table(RUNTIME / "general_gap_bridge_next_action_matrix_90.tsv", next_header, next_rows)
    upsert_section(D90 / "general_gap_bridge_next_action_matrix_90.md", "Residual Measure Decrease Refinement Round", md_table(next_header, next_rows))
    readiness_rows = [
        ("support_bound_lemma_status", SUPPORT_BOUND),
        ("support_reduction_step_status", SUPPORT_REDUCTION),
        ("status_congruence_status", STATUS_CONGRUENCE),
        ("residual_measure_status", MEASURE_SKELETON),
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("readiness_label", GENERAL_READY),
        ("next_action_first", NEXT1),
        ("next_action_second", NEXT2),
        ("next_action_third", NEXT3),
        ("caveat", "not_full_general_theorem"),
    ]
    write_metric(RUNTIME / "general_gap_theorem_readiness_audit_90.tsv", readiness_rows)
    upsert_section(D90 / "general_gap_theorem_readiness_audit_90.md", "Residual Measure Decrease Refinement Round", metric_table(readiness_rows))

    write_metric(
        RUNTIME / "current_support8_closure_certificate_90.tsv",
        [
            ("support8_lock_status", "support8_authoritative_completion_locked"),
            ("pass1_result", "support8_authoritative_completion_locked"),
            ("pass2_result", "support8_authoritative_completion_locked"),
            ("pass3_result", "support8_authoritative_completion_locked"),
            ("required_docs", "39/39"),
            ("required_artifacts", "8/8"),
            ("top_level_fresh_current_runtime_generated", "16"),
            ("top_level_current_runtime_validated_imported_data", "0"),
            ("top_level_mixed", "0"),
            ("top_level_archival_only", "3"),
            ("family_chain_lower_layer_total", "7"),
            ("family_chain_lower_layer_fresh", "7"),
            ("family_chain_lower_layer_imported", "0"),
            ("family_chain_lower_layer_caveat_closed", "1"),
            ("lower_frontier_inventory_only_4_rows_decision", "keep_inventory_only_nonblocking"),
            ("limited_bridge_theorem_status", "limited_bridge_theorem_proved_under_current_scope"),
            ("bridge_next_exact_target", NEXT1),
            ("support_bound_lemma_status", SUPPORT_BOUND),
            ("support_reduction_step_status", SUPPORT_REDUCTION),
            ("family_chain_absorption_status", FAMILY_STATUS),
            ("status_congruence_bridge_status", STATUS_CONGRUENCE),
            ("higher_support_necessity_status", HIGHER_SUPPORT),
            ("general_theorem_readiness", GENERAL_READY),
            ("caveat", "readiness_boundary_not_general_theorem_or_boj_solver"),
        ],
    )
    upsert_section(D90 / "current_support8_closure_certificate_90.md", "Residual Measure Decrease Refinement Round", metric_table([(k, v) for k, v in (line.split("\t") for line in (RUNTIME / "current_support8_closure_certificate_90.tsv").read_text(encoding="utf-8").strip().splitlines()[1:])]))

    for fp_name, rows in {
        "support_bound_skeleton_fingerprint_90": [
            ("support_bound_lemma_skeleton", SUPPORT_BOUND),
            ("support_reduction_step_status", SUPPORT_REDUCTION),
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("next_action", NEXT1),
            ("fingerprint", f"{SUPPORT_BOUND}|{SUPPORT_REDUCTION}|{STATUS_CONGRUENCE}|{NEXT1}"),
        ],
        "support_reduction_step_skeleton_fingerprint_90": [
            ("support_reduction_step_skeleton", SUPPORT_REDUCTION),
            ("residual_measure_status", MEASURE_SKELETON),
            ("next_action", NEXT1),
            ("fingerprint", f"{SUPPORT_REDUCTION}|{MEASURE_SKELETON}|{NEXT1}"),
        ],
        "support_reduction_operation_status_table_fingerprint_90": [
            ("operation_status_table", "operation_status_table_residual_measure_refined"),
            ("family_chain_absorption_status", FAMILY_STATUS),
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("next_blocker", NEXT1),
            ("fingerprint", f"{FAMILY_STATUS}|{STATUS_CONGRUENCE}|{NEXT1}"),
        ],
        "status_preservation_congruence_fingerprint_90": [
            ("status_congruence_skeleton_status", STATUS_CONGRUENCE),
            ("residual_measure_status", MEASURE_SKELETON),
            ("family_chain_absorption_status", FAMILY_STATUS),
            ("next_action_first", NEXT1),
            ("fingerprint", f"{STATUS_CONGRUENCE}|{MEASURE_SKELETON}|{FAMILY_STATUS}|{NEXT1}"),
        ],
        "general_gap_bridge_readiness_fingerprint_90": [
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("support_bound_lemma_status", SUPPORT_BOUND),
            ("support_reduction_step_status", SUPPORT_REDUCTION),
            ("general_theorem_readiness_label", GENERAL_READY),
            ("next_action_first", NEXT1),
            ("next_action_second", NEXT2),
            ("next_action_third", NEXT3),
            ("fingerprint", f"{STATUS_CONGRUENCE}|{SUPPORT_BOUND}|{SUPPORT_REDUCTION}|{GENERAL_READY}|{NEXT1}"),
        ],
    }.items():
        write_metric(RUNTIME / f"{fp_name}.tsv", rows)

    contract_body = metric_table(
        [
            ("latest_round", "residual_absorption_measure_decrease"),
            ("release_compile", "verified_after_regression"),
            ("local_test_compile", "verified_after_regression"),
            ("pass1_pass2_pass3", "support8_authoritative_completion_locked"),
            ("residual_measure_status", MEASURE_SKELETON),
            ("general_theorem_readiness", GENERAL_READY),
        ]
    )
    upsert_section(D90 / "proof_system_contract_memo_90.md", "Residual Measure Decrease Refinement Round", contract_body)
    upsert_section(D90 / "proof_system_reproduction_report_90.md", "Residual Measure Decrease Refinement Round", contract_body)

    root_body = metric_table(
        [
            ("latest_round", "residual_absorption_measure_decrease"),
            ("selected_statement", SELECTED),
            ("residual_branch_classification_status", BRANCH_CLASSIFICATION),
            ("residual_measure_tuple_status", MEASURE_TUPLE),
            ("residual_smaller_witness_construction_status", SMALLER_WITNESS),
            ("residual_measure_skeleton_status", MEASURE_SKELETON),
            ("family_chain_absorption_status", FAMILY_STATUS),
            ("source_alignment_skeleton_status", SOURCE_ALIGNMENT),
            ("status_congruence_skeleton", STATUS_CONGRUENCE),
            ("support_reduction_skeleton", SUPPORT_REDUCTION),
            ("support_bound_skeleton", SUPPORT_BOUND),
            ("higher_support_necessity", HIGHER_SUPPORT),
            ("general_theorem_readiness", GENERAL_READY),
            ("next_action_1", NEXT1),
            ("next_action_2", NEXT2),
            ("next_action_3", NEXT3),
            ("caveat", "not_full_general_theorem_or_full_absorption_proof"),
        ]
    )
    for root_name in [
        "project_status_summary.md",
        "current_workspace_reality_check.md",
        "current_status_authoritative_longform.md",
        "authoritative_completion_to_100_plan_longform.md",
        "theorem_data_promotion_to_100_plan_longform.md",
        "mathematical_progress_to_100_plan_longform.md",
        "progress_history_1_to_85_longform.md",
    ]:
        upsert_section(B4 / root_name, "Residual Measure Decrease Refinement Round", root_body)
    upsert_section(RUNTIME / "project_status_summary_90.md", "Residual Measure Decrease Refinement Round", root_body)


def main() -> None:
    write_round_docs()
    update_rollups()


if __name__ == "__main__":
    main()
