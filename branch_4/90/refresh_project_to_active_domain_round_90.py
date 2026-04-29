#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


D90 = Path(__file__).resolve().parent
B4 = D90.parent
RUNTIME = D90 / "runtime"

SELECTED = "projected_status_domain_refined_under_active_projection_or_reduced_or_escape"
DOMAIN_SEMANTICS = "project_to_active_status_domain_semantics_contract_ready"
DOMAIN_TRANSFER = "project_to_active_domain_transfer_proof_ready_refinement_status_predicate_open"
NORMAL_INTERFACE = "project_to_active_normal_form_transfer_interface_contract_ready"
DOMAIN_SKELETON = "proof_ready_skeleton_project_to_active_domain_refinement_status_predicate_normal_form_open"
PROJECT_STATUS = "partial_project_to_active_domain_refinement_proof_ready_normal_form_status_predicate_open"
STATUS_CONGRUENCE = "partial_status_congruence_project_domain_refined_remaining_contract_canonical_normal_open"
SUPPORT_REDUCTION = "partition_ready_project_domain_refined_remaining_contract_canonical_normal_open"
SUPPORT_BOUND = "proof_ready_skeleton_project_domain_refined_remaining_contract_canonical_normal_open"
HIGHER_SUPPORT = "higher_support_deferred_after_project_to_active_domain_refinement_normal_form_open"
GENERAL_READY = "ready_for_project_to_active_normal_form_refinement"
NEXT1 = "project_to_active_normal_form_refinement"
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
    "operation_scope",
    "status_domain_scope",
    "uses_active_support_notation",
    "uses_project_to_active_semantics",
    "uses_payload_locality",
    "uses_counterexample_status_locality",
    "uses_normal_form_transfer",
    "uses_support_measure",
    "uses_higher_support_escape",
    "selected_for_attempt",
    "risk",
    "reason",
]

scope_rows = [
    {
        "statement_key": "projected_status_domain_preserved_under_active_projection",
        "formal_statement": "If every status dependency is active, active projection preserves the source status-domain.",
        "assumptions": "source witness normal;active support contains all status-domain dependencies",
        "conclusion": "projected domain is domain-preserved",
        "operation_scope": "project_to_active_support",
        "status_domain_scope": "domain-preserved case",
        "uses_active_support_notation": "1",
        "uses_project_to_active_semantics": "1",
        "uses_payload_locality": "1",
        "uses_counterexample_status_locality": "1",
        "uses_normal_form_transfer": "1",
        "uses_support_measure": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "This is the clean case, but it only applies when status dependencies are entirely active.",
    },
    {
        "statement_key": "projected_status_domain_refined_under_active_projection",
        "formal_statement": "If projection removes inactive status-irrelevant dependencies, the projected domain is a restriction/refinement of the source domain.",
        "assumptions": "inactive dependencies are payload-local and status-irrelevant under the domain map",
        "conclusion": "projected status-domain is a source-domain refinement",
        "operation_scope": "project_to_active_support",
        "status_domain_scope": "domain-refined case",
        "uses_active_support_notation": "1",
        "uses_project_to_active_semantics": "1",
        "uses_payload_locality": "1",
        "uses_counterexample_status_locality": "1",
        "uses_normal_form_transfer": "1",
        "uses_support_measure": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "high",
        "reason": "Central proof target; payload locality does not automatically prove status predicate meaning over a refined domain.",
    },
    {
        "statement_key": "projected_status_domain_reduction_implies_smaller_witness",
        "formal_statement": "If the domain shrinks in a status-relevant way but projected status remains valid, the branch is a smaller-witness reduction.",
        "assumptions": "strict active support subset;projected status valid under refined domain",
        "conclusion": "smaller witness or reduced obstruction branch",
        "operation_scope": "project_to_active_support",
        "status_domain_scope": "domain-reduced case",
        "uses_active_support_notation": "1",
        "uses_project_to_active_semantics": "1",
        "uses_payload_locality": "1",
        "uses_counterexample_status_locality": "1",
        "uses_normal_form_transfer": "1",
        "uses_support_measure": "1",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "Measure decrease is available for strict active subset, but valid reduced status is still a proof obligation.",
    },
    {
        "statement_key": "projected_status_domain_failure_is_named_escape",
        "formal_statement": "If projected status-domain transfer is not preserved, refined, or reduced, the failure is a named blocker or deferred higher-support escape.",
        "assumptions": "domain transfer attempt fails or projected status is ill-formed",
        "conclusion": "named project-to-active blocker or higher-support escape",
        "operation_scope": "project_to_active_support",
        "status_domain_scope": "domain escape case",
        "uses_active_support_notation": "1",
        "uses_project_to_active_semantics": "1",
        "uses_payload_locality": "1",
        "uses_counterexample_status_locality": "1",
        "uses_normal_form_transfer": "1",
        "uses_support_measure": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "1",
        "risk": "low",
        "reason": "Keeps failure visible instead of silently promoting locality to status preservation.",
    },
    {
        "statement_key": "full_project_to_active_status_domain_transfer",
        "formal_statement": "Every support-growth witness admits full project-to-active status-domain transfer.",
        "assumptions": "arbitrary support-growth witness",
        "conclusion": "full status-domain transfer",
        "operation_scope": "all project-to-active cases",
        "status_domain_scope": "full transfer",
        "uses_active_support_notation": "1",
        "uses_project_to_active_semantics": "1",
        "uses_payload_locality": "1",
        "uses_counterexample_status_locality": "1",
        "uses_normal_form_transfer": "1",
        "uses_support_measure": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "0",
        "risk": "high",
        "reason": "Out of scope; this round is one-operation domain refinement and keeps normal-form/status-predicate blockers open.",
    },
]


domain_header = [
    "domain_component",
    "definition",
    "source_condition",
    "projected_condition",
    "transfer_rule",
    "proof_requirement",
    "failure_effect",
    "current_status",
    "caveat",
]

domain_rows = [
    {
        "domain_component": "source_status_domain",
        "definition": "The source witness dependency domain used by counterexample-status semantics.",
        "source_condition": "W has status predicate well-formed over source domain",
        "projected_condition": "none yet",
        "transfer_rule": "source domain is the comparison base",
        "proof_requirement": "status-domain extractor for source witness",
        "failure_effect": "source status ill-formed escape",
        "current_status": "proved_under_current_scope",
        "caveat": "Source well-formedness is not projection transfer.",
    },
    {
        "domain_component": "active_support_projected_domain",
        "definition": "The status-domain visible after W_active=normalize(restrict(W,A)).",
        "source_condition": "active support A selected",
        "projected_condition": "projected witness has domain over active coordinates and projected status data",
        "transfer_rule": "restrict source domain to active dependency image, then normalize",
        "proof_requirement": "projected-domain construction",
        "failure_effect": "projected status-domain blocker",
        "current_status": "proved_under_current_scope",
        "caveat": "Projected domain may be restriction/refinement, not equality.",
    },
    {
        "domain_component": "projected_domain_as_restriction",
        "definition": "Projection drops source-domain fields indexed only by inactive support.",
        "source_condition": "inactive fields are not status-relevant",
        "projected_condition": "projected status predicate omits those fields",
        "transfer_rule": "domain_restrict(source_domain,A)",
        "proof_requirement": "inactive-support domain irrelevance",
        "failure_effect": "domain reduction or escape",
        "current_status": "proof_sketch_ready",
        "caveat": "Payload locality alone does not prove domain irrelevance.",
    },
    {
        "domain_component": "projected_domain_as_quotient",
        "definition": "Projection may identify status dependencies that become indistinguishable after inactive coordinates are removed.",
        "source_condition": "dependencies agree on active support",
        "projected_condition": "projected dependencies are represented once",
        "transfer_rule": "quotient only over inactive-equal dependencies",
        "proof_requirement": "quotient compatibility with status predicate",
        "failure_effect": "domain quotient blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "This is not coordinate contraction.",
    },
    {
        "domain_component": "projected_domain_as_refinement",
        "definition": "Projection refines the source status-domain by keeping only active, status-relevant dependencies.",
        "source_condition": "source domain decomposes into active and inactive-local parts",
        "projected_condition": "projected domain contains active dependencies with preserved labels",
        "transfer_rule": "domain_refine(source_domain,A,status_dependency_map)",
        "proof_requirement": "refinement preserves predicate meaning",
        "failure_effect": "status-predicate blocker",
        "current_status": DOMAIN_TRANSFER,
        "caveat": "Proof-ready, not a completed status-locality theorem.",
    },
    {
        "domain_component": "domain_preserved_case",
        "definition": "Projected domain is equal to source domain modulo representation.",
        "source_condition": "all status dependencies are active",
        "projected_condition": "no status-domain field removed",
        "transfer_rule": "identity transfer",
        "proof_requirement": "active dependency containment",
        "failure_effect": "fall through to refinement/reduction",
        "current_status": "proof_sketch_ready",
        "caveat": "Does not cover shrinking active support.",
    },
    {
        "domain_component": "domain_refined_case",
        "definition": "Projected domain is a restriction/refinement preserving status predicate meaning.",
        "source_condition": "inactive dependencies are irrelevant or locally determined",
        "projected_condition": "projected predicate interprets same counterexample status over refined domain",
        "transfer_rule": "refined-domain comparison",
        "proof_requirement": "status predicate determination over refined domain",
        "failure_effect": "status-predicate blocker",
        "current_status": DOMAIN_TRANSFER,
        "caveat": "Normal-form interface remains required.",
    },
    {
        "domain_component": "domain_reduced_case",
        "definition": "Projected domain loses source-status information but still forms a valid reduced counterexample branch.",
        "source_condition": "strict active subset;source domain shrinks",
        "projected_condition": "projected witness status remains valid",
        "transfer_rule": "route to smaller-witness branch",
        "proof_requirement": "valid reduced-status theorem",
        "failure_effect": "smaller-witness blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Measure decrease is separate from valid status.",
    },
    {
        "domain_component": "domain_lost_case",
        "definition": "Projection removes status-relevant data with no current refined predicate interpretation.",
        "source_condition": "status dependency appears inactive but relevant",
        "projected_condition": "projected status predicate cannot be formed",
        "transfer_rule": "not accepted as preservation or reduction",
        "proof_requirement": "none under current proof; classify failure",
        "failure_effect": "named operation blocker",
        "current_status": "named_escape_ready",
        "caveat": "Not hidden as proof-ready preservation.",
    },
    {
        "domain_component": "domain_escape_case",
        "definition": "Domain transfer is outside current active-support contract or needs a true higher-support bound.",
        "source_condition": "unclassified dependency or support-growth escape",
        "projected_condition": "projected domain outside current refinement map",
        "transfer_rule": "named escape",
        "proof_requirement": "escape inventory",
        "failure_effect": "higher-support deferred escape",
        "current_status": "proved_under_current_scope",
        "caveat": "Escape is not a proof of transfer.",
    },
    {
        "domain_component": "relation_to_inactive_support_locality",
        "definition": "Inactive support can be removed only if no counterexample-status dependency needs it.",
        "source_condition": "inactive=S_minus_A",
        "projected_condition": "W_active omits inactive support",
        "transfer_rule": "inactive domain irrelevance or reduced-status branch",
        "proof_requirement": "counterexample-status locality",
        "failure_effect": "domain blocker remains",
        "current_status": DOMAIN_TRANSFER,
        "caveat": "Payload locality is necessary but not sufficient.",
    },
    {
        "domain_component": "relation_to_payload_locality",
        "definition": "Payload locality supplies active carrier containment for payload fields.",
        "source_condition": "payload carriers are active",
        "projected_condition": "payload preserved in projected witness",
        "transfer_rule": "payload data can feed domain transfer",
        "proof_requirement": "payload-to-status dependency sublemma",
        "failure_effect": "status predicate blocker",
        "current_status": "inactive_support_payload_locality_proved_under_current_scope",
        "caveat": "Does not automatically imply status-domain transfer.",
    },
    {
        "domain_component": "relation_to_counterexample_status_predicate",
        "definition": "Status predicate meaning must be invariant or validly reduced under the projected domain.",
        "source_condition": "source status predicate well-formed",
        "projected_condition": "projected status predicate well-formed over refined domain",
        "transfer_rule": "status predicate preservation, reduction, or escape",
        "proof_requirement": "status predicate determination over projected domain",
        "failure_effect": "status-predicate blocker",
        "current_status": "proof_ready_status_predicate_open",
        "caveat": "This is why the operation is not fully proved.",
    },
]


lemma_header = [
    "lemma_component",
    "statement",
    "assumptions",
    "conclusion",
    "active_support_definition_used",
    "inactive_support_locality_used",
    "payload_locality_used",
    "projected_domain_used",
    "status_predicate_used",
    "proof_status",
    "missing_hypothesis",
    "caveat",
]

lemma_rows = [
    {
        "lemma_component": "source_domain_available",
        "statement": "The source status-domain is available from status semantics before projection.",
        "assumptions": "source witness satisfies status-domain preconditions",
        "conclusion": "source domain is well-defined",
        "active_support_definition_used": "0",
        "inactive_support_locality_used": "0",
        "payload_locality_used": "0",
        "projected_domain_used": "0",
        "status_predicate_used": "1",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "No projection transfer yet.",
    },
    {
        "lemma_component": "projected_domain_available",
        "statement": "The projected status-domain is well-defined as the active restriction/refinement candidate.",
        "assumptions": "active support A;W_active=normalize(restrict(W,A))",
        "conclusion": "projected domain candidate exists",
        "active_support_definition_used": "1",
        "inactive_support_locality_used": "1",
        "payload_locality_used": "1",
        "projected_domain_used": "1",
        "status_predicate_used": "1",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Candidate existence is not predicate preservation.",
    },
    {
        "lemma_component": "inactive_removal_domain_refinement",
        "statement": "Removing inactive support preserves or refines status-domain dependencies.",
        "assumptions": "inactive support outside active dependency map;payload locality holds",
        "conclusion": "projected domain is preservation/refinement candidate",
        "active_support_definition_used": "1",
        "inactive_support_locality_used": "1",
        "payload_locality_used": "1",
        "projected_domain_used": "1",
        "status_predicate_used": "1",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "status dependency containment or irrelevance",
        "caveat": "Central open domain sublemma.",
    },
    {
        "lemma_component": "domain_refinement_status_predicate_meaning",
        "statement": "A refined projected domain preserves counterexample-status predicate meaning.",
        "assumptions": "domain refinement map;normal-form interface;payload locality",
        "conclusion": "status predicate comparison is meaningful",
        "active_support_definition_used": "1",
        "inactive_support_locality_used": "1",
        "payload_locality_used": "1",
        "projected_domain_used": "1",
        "status_predicate_used": "1",
        "proof_status": DOMAIN_TRANSFER,
        "missing_hypothesis": "status predicate determination over refined domain",
        "caveat": "Proof-ready only; not completed.",
    },
    {
        "lemma_component": "domain_reduction_smaller_witness",
        "statement": "A strict domain shrink can be a smaller-witness branch if projected status is valid.",
        "assumptions": "strict active subset;projected status valid",
        "conclusion": "reduced counterexample or obstruction",
        "active_support_definition_used": "1",
        "inactive_support_locality_used": "1",
        "payload_locality_used": "1",
        "projected_domain_used": "1",
        "status_predicate_used": "1",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "valid reduced-status theorem",
        "caveat": "Measure decrease alone is insufficient.",
    },
    {
        "lemma_component": "domain_failure_escape",
        "statement": "If preservation/refinement/reduction fails, the case is a named project-to-active blocker or deferred higher-support escape.",
        "assumptions": "domain transfer cannot be formed or projected status invalid",
        "conclusion": "named escape",
        "active_support_definition_used": "1",
        "inactive_support_locality_used": "1",
        "payload_locality_used": "possible",
        "projected_domain_used": "possible",
        "status_predicate_used": "possible",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Escape is not preservation.",
    },
]


normal_header = [
    "interface_component",
    "statement",
    "source_normal_form",
    "projected_normal_form",
    "required_for_domain_transfer",
    "required_for_status_predicate",
    "proof_status",
    "missing_hypothesis",
    "caveat",
]

normal_rows = [
    {
        "interface_component": "source_normal_form_input",
        "statement": "Source normal form supplies the domain extractor's precondition.",
        "source_normal_form": "W normal",
        "projected_normal_form": "none yet",
        "required_for_domain_transfer": "yes",
        "required_for_status_predicate": "yes",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Does not prove projected normal form.",
    },
    {
        "interface_component": "projected_normal_form_candidate",
        "statement": "Projected witness is normalize(restrict(W,A)).",
        "source_normal_form": "W normal",
        "projected_normal_form": "W_active normalized candidate",
        "required_for_domain_transfer": "yes",
        "required_for_status_predicate": "yes",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "normal-form preservation under active projection",
        "caveat": "This round does not prove full normal-form transfer.",
    },
    {
        "interface_component": "domain_normal_form_compatibility",
        "statement": "Domain extraction after projection must read the same status-relevant normal-form fields.",
        "source_normal_form": "source domain fields visible",
        "projected_normal_form": "projected domain fields visible",
        "required_for_domain_transfer": "yes",
        "required_for_status_predicate": "yes",
        "proof_status": NORMAL_INTERFACE,
        "missing_hypothesis": "field preservation under normalization",
        "caveat": "Interface only.",
    },
    {
        "interface_component": "normal_form_blocker",
        "statement": "If projected normal form cannot be certified, domain transfer remains proof-ready but status proof stays open.",
        "source_normal_form": "source normal",
        "projected_normal_form": "uncertified",
        "required_for_domain_transfer": "possible",
        "required_for_status_predicate": "yes",
        "proof_status": "blocked_by_normal_form",
        "missing_hypothesis": "project_to_active_normal_form_refinement",
        "caveat": "Next target candidate.",
    },
]


obligation_header = [
    "obligation_key",
    "statement",
    "required_for_selected_statement",
    "existing_verified_inputs",
    "missing_sublemmas",
    "proof_status",
    "dependency_on_inactive_support",
    "dependency_on_payload_locality",
    "dependency_on_status_domain",
    "dependency_on_normal_form",
    "dependency_on_smaller_witness",
    "dependency_on_higher_support",
    "can_attempt_now",
    "recommended_next_action",
]


def obligation(key: str, statement: str, inputs: str, missing: str, status: str, deps: tuple[str, str, str, str, str, str], can: str, next_action: str) -> dict[str, str]:
    return {
        "obligation_key": key,
        "statement": statement,
        "required_for_selected_statement": "1",
        "existing_verified_inputs": inputs,
        "missing_sublemmas": missing,
        "proof_status": status,
        "dependency_on_inactive_support": deps[0],
        "dependency_on_payload_locality": deps[1],
        "dependency_on_status_domain": deps[2],
        "dependency_on_normal_form": deps[3],
        "dependency_on_smaller_witness": deps[4],
        "dependency_on_higher_support": deps[5],
        "can_attempt_now": can,
        "recommended_next_action": next_action,
    }


obligation_rows = [
    obligation("project_to_active_domain_language_well_defined", "The domain refinement language separates preserved/refined/reduced/lost/escape cases.", "active support notation;status language", "", "proved_under_current_scope", ("1", "0", "1", "0", "0", "0"), "1", "use_domain_language"),
    obligation("source_status_domain_well_defined", "The source counterexample-status domain is well-defined.", "project_to_active_status_semantics", "", "proved_under_current_scope", ("0", "0", "1", "0", "0", "0"), "1", "use_source_domain"),
    obligation("projected_status_domain_well_defined", "The projected domain candidate is well-defined after active projection.", "project_to_active operation semantics", "", "proved_under_current_scope", ("1", "1", "1", "1", "0", "0"), "1", "use_projected_domain"),
    obligation("inactive_support_removal_preserves_or_refines_domain", "Inactive support removal preserves or refines the source status-domain.", "inactive support locality;payload locality", "status dependency irrelevance", "proof_sketch_ready", ("1", "1", "1", "1", "0", "0"), "1", "domain_dependency_sublemma"),
    obligation("domain_refinement_preserves_status_predicate_meaning", "A refined projected domain preserves counterexample-status predicate meaning.", "domain semantics;payload locality", "status predicate determination over refined domain", "needs_status_predicate_sublemma", ("1", "1", "1", "1", "0", "0"), "1", NEXT1),
    obligation("domain_reduction_implies_smaller_witness", "A strict domain reduction is a smaller-witness branch only if projected status is valid.", "measure decrease for strict active subset", "valid reduced-status theorem", "needs_smaller_witness_sublemma", ("1", "1", "1", "1", "1", "0"), "1", "valid_reduced_status_sublemma"),
    obligation("payload_locality_supports_domain_transfer", "Payload locality supplies active carrier containment for domain transfer but does not complete it.", "inactive_support_payload_locality_proved_under_current_scope", "payload-to-status dependency bridge", "proof_sketch_ready", ("1", "1", "1", "0", "0", "0"), "1", "payload_to_status_dependency_sublemma"),
    obligation("normal_form_interface_sufficient_for_domain_transfer", "The normal-form interface lists the assumptions needed to read projected domains.", "support notation;projected normal form candidate", "normal-form transfer proof", "needs_normal_form_sublemma", ("1", "0", "1", "1", "0", "0"), "1", NEXT1),
    obligation("domain_transfer_failure_is_named_escape", "Domain transfer failure is a named project-to-active blocker or higher-support escape.", "higher-support escape interface", "", "proved_under_current_scope", ("1", "1", "1", "1", "0", "1"), "1", "keep_escape_visible"),
    obligation("no_hidden_domain_transfer_failure_case", "Every transfer failure is domain, status-predicate, normal-form, smaller-witness, or named escape.", "domain semantics table;obligation inventory", "full no-hidden-case proof after normal-form/status sublemmas", "proof_sketch_ready", ("1", "1", "1", "1", "1", "1"), "1", NEXT1),
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
        "sublemma_key": "project_to_active_domain_language_well_defined",
        "proof_status": "proved_under_current_scope",
        "assumptions": "active support notation;projection semantics;status language",
        "conclusion": "preserved/refined/reduced/lost/escape domain cases are well-defined",
        "proof_summary": "The round names the projected domain as a restriction/refinement candidate and separates domain reduction from escape.",
        "evidence_path": "project_to_active_status_domain_semantics_90.md",
        "missing_hypothesis": "",
        "next_action": "use_domain_language",
    },
    {
        "sublemma_key": "source_status_domain_well_defined",
        "proof_status": "proved_under_current_scope",
        "assumptions": "source witness satisfies status semantics",
        "conclusion": "source status-domain exists before projection",
        "proof_summary": "The source side is the existing status-domain predicate input and is not changed by this round.",
        "evidence_path": "project_to_active_status_semantics_90.md;status_preservation_language_90.md",
        "missing_hypothesis": "",
        "next_action": "use_source_domain",
    },
    {
        "sublemma_key": "projected_status_domain_well_defined",
        "proof_status": "proved_under_current_scope",
        "assumptions": "active support A;projected witness normalize(restrict(W,A))",
        "conclusion": "projected status-domain candidate exists",
        "proof_summary": "Projection semantics provides the active restriction candidate; equality with source domain is not claimed.",
        "evidence_path": "project_to_active_support_operation_semantics_90.md;project_to_active_status_domain_semantics_90.md",
        "missing_hypothesis": "",
        "next_action": "domain_transfer_lemma",
    },
    {
        "sublemma_key": "inactive_support_removal_preserves_or_refines_domain",
        "proof_status": "proof_sketch_only",
        "assumptions": "inactive support is outside active dependency map;payload locality holds",
        "conclusion": "domain is preserved or refined by active projection",
        "proof_summary": "Payload locality rules out inactive payload carriers, but status-domain dependencies still need an irrelevance/refinement argument.",
        "evidence_path": "inactive_support_payload_locality_90.md;project_to_active_domain_transfer_lemma_90.md",
        "missing_hypothesis": "status dependency irrelevance",
        "next_action": "domain_dependency_sublemma",
    },
    {
        "sublemma_key": "domain_refinement_preserves_status_predicate_meaning",
        "proof_status": "blocked_by_status_predicate",
        "assumptions": "projected domain is a source-domain refinement;normal-form interface holds",
        "conclusion": "counterexample-status predicate remains meaningful",
        "proof_summary": "The refined domain comparison is proof-ready, but status-predicate determination over the refined projected domain is still open.",
        "evidence_path": "project_to_active_domain_transfer_lemma_90.md",
        "missing_hypothesis": "status predicate determination over refined domain",
        "next_action": NEXT1,
    },
    {
        "sublemma_key": "domain_reduction_implies_smaller_witness",
        "proof_status": "blocked_by_smaller_witness",
        "assumptions": "domain shrinks;active support strictly smaller;projected status valid",
        "conclusion": "smaller witness branch",
        "proof_summary": "Support measure decreases under strict active projection, but a valid reduced counterexample/status theorem is still required.",
        "evidence_path": "project_to_active_support_smaller_witness_90.md;project_to_active_domain_transfer_lemma_90.md",
        "missing_hypothesis": "valid reduced-status theorem",
        "next_action": "valid_reduced_status_sublemma",
    },
    {
        "sublemma_key": "payload_locality_supports_domain_transfer",
        "proof_status": "proof_sketch_only",
        "assumptions": "inactive-support payload locality is proved",
        "conclusion": "payload-local dependencies are active inputs for domain transfer",
        "proof_summary": "Payload locality can feed the domain map, but it does not eliminate non-payload status-domain dependencies.",
        "evidence_path": "inactive_support_payload_locality_90.md",
        "missing_hypothesis": "payload-to-status dependency bridge",
        "next_action": "payload_to_status_dependency_sublemma",
    },
    {
        "sublemma_key": "domain_transfer_failure_is_named_escape",
        "proof_status": "proved_under_current_scope",
        "assumptions": "domain transfer cannot be preserved/refined/reduced",
        "conclusion": "named project-to-active blocker or higher-support escape",
        "proof_summary": "The failure case remains explicit and does not become a hidden proof of status locality.",
        "evidence_path": "higher_support_necessity_after_project_to_active_domain_90.md;higher_support_escape_interface_90.md",
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
        "resolves": "project_to_active_normal_form_and_status_predicate_interface",
        "prerequisite_status": DOMAIN_SKELETON,
        "readiness_score_0_100": "83",
        "proof_value_0_100": "85",
        "engineering_cost_0_100": "60",
        "risk_0_100": "61",
        "dependency_clarity_0_100": "86",
        "expected_progress_value_0_100": "83",
        "recommended_order": "1",
        "final_recommendation": NEXT1,
        "reason": "Project-to-active domain is now proof-ready; the normal-form/status-predicate interface is the immediate remaining local blocker.",
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
        "reason": "Coordinate contraction still needs quotient domain/normal-form transfer.",
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
        "reason": "Canonical compression has motif domain/normal-form transfer open.",
    },
    {
        "action_key": "support_bound_completion",
        "resolves": "support_bound_completion_after_transfer_refinements",
        "prerequisite_status": SUPPORT_BOUND,
        "readiness_score_0_100": "71",
        "proof_value_0_100": "84",
        "engineering_cost_0_100": "66",
        "risk_0_100": "72",
        "dependency_clarity_0_100": "80",
        "expected_progress_value_0_100": "73",
        "recommended_order": "4",
        "final_recommendation": "support_bound_completion_after_domain_normal_form_refinements",
        "reason": "Premature until remaining operation-local normal-form and status-predicate blockers close.",
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
        "final_recommendation": "higher_support_necessity_recheck_after_normal_form_refinements",
        "reason": "Higher-support scan remains deferred while operation-local normal-form/domain blockers remain actionable.",
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
        "reason": "Planning can continue, but proof completion still depends on operation-local transfer sublemmas.",
    },
]


def write_round_docs() -> None:
    write_table(RUNTIME / "project_to_active_status_domain_refinement_scope_inventory_90.tsv", scope_header, scope_rows)
    write_md(
        D90 / "project_to_active_status_domain_refinement_scope_memo_90.md",
        "Project To Active Status Domain Refinement Scope Memo 90",
        "\n".join(
            [
                "This round proof-attempts only the `project_to_active_support` status-domain transfer. It does not prove the full general theorem or full project-to-active status preservation.",
                "",
                f"Selected statement: `{SELECTED}`.",
                "",
                md_table(scope_header, scope_rows),
            ]
        ),
    )
    write_table(RUNTIME / "project_to_active_status_domain_semantics_90.tsv", domain_header, domain_rows)
    write_md(D90 / "project_to_active_status_domain_semantics_90.md", "Project To Active Status Domain Semantics 90", f"Final status: `{DOMAIN_SEMANTICS}`.\n\n" + md_table(domain_header, domain_rows))
    write_table(RUNTIME / "project_to_active_domain_transfer_lemma_90.tsv", lemma_header, lemma_rows)
    write_md(D90 / "project_to_active_domain_transfer_lemma_90.md", "Project To Active Domain Transfer Lemma 90", f"Final status: `{DOMAIN_TRANSFER}`.\n\n" + md_table(lemma_header, lemma_rows))
    write_table(RUNTIME / "project_to_active_normal_form_transfer_interface_90.tsv", normal_header, normal_rows)
    write_md(D90 / "project_to_active_normal_form_transfer_interface_90.md", "Project To Active Normal Form Transfer Interface 90", f"Final status: `{NORMAL_INTERFACE}`.\n\n" + md_table(normal_header, normal_rows))
    write_table(RUNTIME / "project_to_active_status_domain_refinement_obligations_90.tsv", obligation_header, obligation_rows)
    write_md(D90 / "project_to_active_status_domain_refinement_obligations_90.md", "Project To Active Status Domain Refinement Obligations 90", "Obligation count: `10`.\n\n" + md_table(obligation_header, obligation_rows))
    write_table(RUNTIME / "project_to_active_status_domain_refinement_sublemma_proofs_90.tsv", sublemma_header, sublemma_rows)
    write_md(
        D90 / "project_to_active_status_domain_refinement_sublemma_proofs_90.md",
        "Project To Active Status Domain Refinement Sublemma Proofs 90",
        "Sublemma proof-attempt counts: `proved_under_current_scope=4`, `proof_sketch_only=2`, `blocked=2`.\n\n" + md_table(sublemma_header, sublemma_rows),
    )
    skeleton_rows = [
        ("lemma_name", "project_to_active_status_domain_refined_or_reduced_or_escape"),
        ("exact_statement", "Active projection preserves, refines, reduces, or names escape for the projected status-domain; status predicate preservation still requires normal-form and predicate-meaning sublemmas."),
        ("assumptions", "source witness normal;active support A;payload locality proved;projected witness W_active=normalize(restrict(W,A))"),
        ("conclusion", "domain-preserved, domain-refined, smaller-witness reduction, or named escape"),
        ("source_status_domain", "source counterexample-status dependency domain"),
        ("projected_status_domain", "active restriction/refinement candidate"),
        ("domain_transfer_rule", "preserve/refine/reduce/escape"),
        ("payload_locality_use", "inactive_support_payload_locality_proved_under_current_scope"),
        ("normal_form_interface", NORMAL_INTERFACE),
        ("status_predicate_meaning", "blocked_by_status_predicate_refined_domain_determination"),
        ("smaller_witness_fallback", "blocked_by_valid_reduced_status_theorem"),
        ("failure_to_escape_case", "named_project_to_active_domain_escape_or_higher_support_escape"),
        ("relation_to_inactive_support_locality_skeleton", "inactive_support_counterexample_status_locality_proof_ready_status_domain_open_refined"),
        ("relation_to_status_congruence_bridge", STATUS_CONGRUENCE),
        ("relation_to_support_bound_skeleton", SUPPORT_BOUND),
        ("missing_steps", "status predicate determination over refined domain; project-to-active normal-form transfer; valid reduced-status theorem"),
        ("exact_caveat", "one-operation domain proof-ready skeleton only; not full project-to-active proof"),
        ("final_status", DOMAIN_SKELETON),
    ]
    write_metric(RUNTIME / "project_to_active_status_domain_refinement_skeleton_90.tsv", skeleton_rows)
    write_md(D90 / "project_to_active_status_domain_refinement_skeleton_90.md", "Project To Active Status Domain Refinement Skeleton 90", metric_table(skeleton_rows))
    write_metric(
        RUNTIME / "project_to_active_status_domain_refinement_fingerprint_90.tsv",
        [
            ("selected_statement", SELECTED),
            ("domain_semantics_status", DOMAIN_SEMANTICS),
            ("domain_transfer_lemma_status", DOMAIN_TRANSFER),
            ("normal_form_interface_status", NORMAL_INTERFACE),
            ("skeleton_status", DOMAIN_SKELETON),
            ("obligation_count", "10"),
            ("proved_under_current_scope_sublemma_count", "4"),
            ("proof_sketch_only_sublemma_count", "2"),
            ("blocked_sublemma_count", "2"),
        ],
    )
    write_metric(RUNTIME / "project_to_active_domain_transfer_fingerprint_90.tsv", [("domain_transfer_status", DOMAIN_TRANSFER), ("payload_locality_input", "proved_under_current_scope"), ("status_predicate_meaning", "open"), ("normal_form_dependency", NORMAL_INTERFACE)])
    write_metric(RUNTIME / "project_to_active_normal_form_interface_fingerprint_90.tsv", [("normal_form_interface_status", NORMAL_INTERFACE), ("required_for_domain_transfer", "yes"), ("required_for_status_predicate", "yes"), ("next_action", NEXT1)])


def update_rollups() -> None:
    locality_rows = [
        ("selected_statement", SELECTED),
        ("payload_locality", "inactive_support_payload_locality_proved_under_current_scope"),
        ("status_domain_semantics", DOMAIN_SEMANTICS),
        ("domain_transfer_lemma", DOMAIN_TRANSFER),
        ("normal_form_interface", NORMAL_INTERFACE),
        ("counterexample_status_locality", "inactive_support_counterexample_status_locality_proof_ready_domain_refined_normal_form_open"),
        ("final_status", "proof_ready_skeleton_project_to_active_locality_domain_refined_normal_form_open"),
        ("caveat", "project_to_active_support_not_fully_proved"),
    ]
    write_metric(RUNTIME / "project_to_active_status_locality_skeleton_90.tsv", locality_rows)
    upsert_section(D90 / "project_to_active_status_locality_skeleton_90.md", "Project To Active Domain Refinement Round", metric_table(locality_rows))

    inactive_rows = [
        ("payload_locality_status", "inactive_support_payload_locality_proved_under_current_scope"),
        ("status_domain_transfer_status", DOMAIN_TRANSFER),
        ("normal_form_transfer_status", NORMAL_INTERFACE),
        ("status_predicate_meaning", "blocked_by_status_predicate"),
        ("valid_reduced_status", "blocked_by_smaller_witness"),
        ("final_status", "inactive_support_counterexample_status_locality_proof_ready_domain_refined_normal_form_open"),
        ("caveat", "payload_locality_not_status_locality"),
    ]
    write_metric(RUNTIME / "inactive_support_counterexample_status_locality_90.tsv", inactive_rows)
    upsert_section(D90 / "inactive_support_counterexample_status_locality_90.md", "Project To Active Domain Refinement Round", metric_table(inactive_rows))

    write_table(RUNTIME / "project_to_active_status_obligations_90.tsv", obligation_header, obligation_rows)
    upsert_section(D90 / "project_to_active_status_obligations_90.md", "Project To Active Domain Refinement Round", md_table(obligation_header, obligation_rows))
    write_table(RUNTIME / "project_to_active_status_sublemma_proofs_90.tsv", sublemma_header, sublemma_rows)
    upsert_section(D90 / "project_to_active_status_sublemma_proofs_90.md", "Project To Active Domain Refinement Round", md_table(sublemma_header, sublemma_rows))

    status_rows = [
        ("project_to_active_status", PROJECT_STATUS),
        ("project_to_active_domain_refinement", DOMAIN_SKELETON),
        ("contract_equivalent_status", "partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open"),
        ("canonical_compression_status", "partial_canonical_compression_congruence_proof_ready_domain_normal_form_open"),
        ("family_chain_absorption_status", "partial_family_chain_absorption_residual_measure_proof_ready_shared_domain_normal_form_open"),
        ("status_congruence_bridge_status", STATUS_CONGRUENCE),
        ("remaining_blockers", "project_to_active_normal_form_status_predicate;contract_canonical_domain_normal_form_transfer"),
        ("caveat", "one_operation_domain_refinement_not_all_operation_proof"),
    ]
    write_metric(RUNTIME / "status_preservation_congruence_skeleton_90.tsv", status_rows)
    upsert_section(D90 / "status_preservation_congruence_skeleton_90.md", "Project To Active Domain Refinement Round", metric_table(status_rows))

    op_header = ["operation_key", "operation_status", "status_congruence_status", "measure_status", "remaining_blocker", "next_action"]
    op_rows = [
        {"operation_key": "project_to_active", "operation_status": PROJECT_STATUS, "status_congruence_status": DOMAIN_SKELETON, "measure_status": "strict_active_subset_measure_available_separate_from_status", "remaining_blocker": "normal_form_status_predicate_valid_reduced_status", "next_action": NEXT1},
        {"operation_key": "contract_equivalent_support_coordinates", "operation_status": "partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open", "status_congruence_status": "proof_ready_domain_normal_form_open", "measure_status": "measure_decrease_available_separate_from_status", "remaining_blocker": "quotient_domain_normal_form_transfer", "next_action": NEXT2},
        {"operation_key": "canonical_motif_compression", "operation_status": "partial_canonical_compression_congruence_proof_ready_domain_normal_form_open", "status_congruence_status": "proof_ready_domain_normal_form_open", "measure_status": "lexicographic_measure_available_separate_from_status", "remaining_blocker": "motif_domain_normal_form_transfer", "next_action": NEXT3},
        {"operation_key": "family_chain_absorption", "operation_status": "partial_family_chain_absorption_residual_measure_proof_ready_shared_domain_normal_form_open", "status_congruence_status": "residual_measure_proof_ready", "measure_status": "residual_measure_proof_ready_alignment_defect_open", "remaining_blocker": "status_domain_normal_form_transfer_for_valid_smaller_witness", "next_action": NEXT1},
    ]
    write_table(RUNTIME / "support_reduction_operation_status_table_90.tsv", op_header, op_rows)
    upsert_section(D90 / "support_reduction_operation_status_table_90.md", "Project To Active Domain Refinement Round", md_table(op_header, op_rows))

    reduction_rows = [
        ("support_reduction_step_status", SUPPORT_REDUCTION),
        ("project_to_active_status", PROJECT_STATUS),
        ("project_to_active_domain_refinement", DOMAIN_SKELETON),
        ("contract_domain_normal_status", "open"),
        ("canonical_domain_normal_status", "open"),
        ("family_residual_status", "proof_ready"),
        ("next_exact_target", NEXT1),
    ]
    write_metric(RUNTIME / "support_reduction_step_skeleton_90.tsv", reduction_rows)
    upsert_section(D90 / "support_reduction_step_skeleton_90.md", "Project To Active Domain Refinement Round", metric_table(reduction_rows))

    bound_rows = [
        ("support_bound_lemma_status", SUPPORT_BOUND),
        ("support_reduction_dependency", SUPPORT_REDUCTION),
        ("status_congruence_dependency", STATUS_CONGRUENCE),
        ("project_to_active_domain_dependency", DOMAIN_SKELETON),
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("general_theorem_readiness", GENERAL_READY),
        ("next_exact_target", NEXT1),
        ("caveat", "support_bound_not_completed_by_one_operation_domain_round"),
    ]
    write_metric(RUNTIME / "support_bound_lemma_skeleton_90.tsv", bound_rows)
    upsert_section(D90 / "support_bound_lemma_skeleton_90.md", "Project To Active Domain Refinement Round", metric_table(bound_rows))

    higher_header = ["check_key", "finding", "result_status", "next_action"]
    higher_rows = [
        {"check_key": "project_to_active_status_domain_closed", "finding": "not fully closed; proof-ready domain refinement with status-predicate and normal-form open", "result_status": DOMAIN_SKELETON, "next_action": NEXT1},
        {"check_key": "project_to_active_support_gt8_escape_reduced", "finding": "partially reduced: domain failure cases are named preservation/refinement/reduction/escape branches", "result_status": HIGHER_SUPPORT, "next_action": NEXT1},
        {"check_key": "normal_form_priority", "finding": "project-to-active normal-form/status-predicate interface is the next local blocker", "result_status": NORMAL_INTERFACE, "next_action": NEXT1},
        {"check_key": "contract_canonical_priority", "finding": "contract and canonical domain/normal-form blockers remain after project domain refinement", "result_status": "contract_canonical_transfer_open", "next_action": NEXT2},
        {"check_key": "support_bound_completion_ready", "finding": "not yet; one-operation domain refinement does not close support-bound skeleton", "result_status": SUPPORT_BOUND, "next_action": "after_transfer_refinements"},
        {"check_key": "higher_support_deferred", "finding": "support9+ scan remains deferred", "result_status": HIGHER_SUPPORT, "next_action": "do_not_scan_support9"},
        {"check_key": "limited_to_broader_generalization_ready", "finding": "not ready for theorem completion; transfer refinements remain", "result_status": GENERAL_READY, "next_action": NEXT1},
    ]
    write_table(RUNTIME / "higher_support_necessity_after_project_to_active_domain_90.tsv", higher_header, higher_rows)
    write_md(D90 / "higher_support_necessity_after_project_to_active_domain_90.md", "Higher Support Necessity After Project To Active Domain 90", f"Final status: `{HIGHER_SUPPORT}`.\n\n" + md_table(higher_header, higher_rows))
    write_metric(RUNTIME / "higher_support_escape_interface_90.tsv", [("higher_support_necessity_status", HIGHER_SUPPORT), ("project_to_active_domain_escape", "named_project_to_active_domain_or_higher_support_escape"), ("support9_scan_status", "not_run"), ("next_exact_target", NEXT1), ("caveat", "higher_support_escape_visible_not_resolved")])
    upsert_section(D90 / "higher_support_escape_interface_90.md", "Project To Active Domain Refinement Round", metric_table([("higher_support_necessity_status", HIGHER_SUPPORT), ("project_to_active_domain_escape", "named_project_to_active_domain_or_higher_support_escape"), ("support9_scan_status", "not_run"), ("next_exact_target", NEXT1)]))

    bridge_rows = [
        ("general_bridge_obligation_status", "project_to_active_domain_refined_normal_form_open"),
        ("project_to_active_domain_status", DOMAIN_SKELETON),
        ("project_to_active_operation_status", PROJECT_STATUS),
        ("status_congruence_status", STATUS_CONGRUENCE),
        ("support_reduction_status", SUPPORT_REDUCTION),
        ("support_bound_status", SUPPORT_BOUND),
        ("general_theorem_readiness", GENERAL_READY),
        ("next_exact_target", NEXT1),
    ]
    for name in ["general_gap_bridge_obligation_inventory_90", "general_gap_bridge_dependency_graph_90", "general_gap_bridge_lemma_candidates_90", "limited_general_gap_bridge_skeleton_90"]:
        write_metric(RUNTIME / f"{name}.tsv", bridge_rows)
        upsert_section(D90 / f"{name}.md", "Project To Active Domain Refinement Round", metric_table(bridge_rows))
    write_table(RUNTIME / "general_gap_bridge_next_action_matrix_90.tsv", next_header, next_rows)
    upsert_section(D90 / "general_gap_bridge_next_action_matrix_90.md", "Project To Active Domain Refinement Round", md_table(next_header, next_rows))
    readiness_rows = [
        ("support_bound_lemma_status", SUPPORT_BOUND),
        ("support_reduction_step_status", SUPPORT_REDUCTION),
        ("status_congruence_status", STATUS_CONGRUENCE),
        ("project_to_active_domain_status", DOMAIN_SKELETON),
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("readiness_label", GENERAL_READY),
        ("next_action_first", NEXT1),
        ("next_action_second", NEXT2),
        ("next_action_third", NEXT3),
        ("caveat", "not_full_general_theorem"),
    ]
    write_metric(RUNTIME / "general_gap_theorem_readiness_audit_90.tsv", readiness_rows)
    upsert_section(D90 / "general_gap_theorem_readiness_audit_90.md", "Project To Active Domain Refinement Round", metric_table(readiness_rows))
    for fp_name, rows in {
        "support_reduction_step_skeleton_fingerprint_90": [("support_reduction_step_skeleton", SUPPORT_REDUCTION), ("project_to_active_domain_status", DOMAIN_SKELETON), ("next_action", NEXT1), ("fingerprint", f"{SUPPORT_REDUCTION}|{DOMAIN_SKELETON}|{NEXT1}")],
        "support_reduction_operation_status_table_fingerprint_90": [("operation_status_table", "operation_status_table_project_domain_refined"), ("project_to_active_status", PROJECT_STATUS), ("status_congruence_status", STATUS_CONGRUENCE), ("next_blocker", NEXT1), ("fingerprint", f"{PROJECT_STATUS}|{STATUS_CONGRUENCE}|{NEXT1}")],
        "status_preservation_congruence_fingerprint_90": [("status_congruence_skeleton_status", STATUS_CONGRUENCE), ("project_to_active_domain_status", DOMAIN_SKELETON), ("next_action_first", NEXT1), ("fingerprint", f"{STATUS_CONGRUENCE}|{DOMAIN_SKELETON}|{NEXT1}")],
        "general_gap_bridge_readiness_fingerprint_90": [("status_congruence_status", STATUS_CONGRUENCE), ("support_bound_lemma_status", SUPPORT_BOUND), ("support_reduction_step_status", SUPPORT_REDUCTION), ("general_theorem_readiness_label", GENERAL_READY), ("next_action_first", NEXT1), ("next_action_second", NEXT2), ("next_action_third", NEXT3), ("fingerprint", f"{STATUS_CONGRUENCE}|{SUPPORT_BOUND}|{SUPPORT_REDUCTION}|{GENERAL_READY}|{NEXT1}")],
    }.items():
        write_metric(RUNTIME / f"{fp_name}.tsv", rows)

    cert_rows = [
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
        ("project_to_active_operation_status", PROJECT_STATUS),
        ("status_congruence_bridge_status", STATUS_CONGRUENCE),
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("general_theorem_readiness", GENERAL_READY),
        ("caveat", "readiness_boundary_not_general_theorem_or_boj_solver"),
    ]
    write_metric(RUNTIME / "current_support8_closure_certificate_90.tsv", cert_rows)
    upsert_section(D90 / "current_support8_closure_certificate_90.md", "Project To Active Domain Refinement Round", metric_table(cert_rows))
    contract_body = metric_table([("latest_round", "project_to_active_status_domain_refinement"), ("release_compile", "verified_after_regression"), ("local_test_compile", "verified_after_regression"), ("pass1_pass2_pass3", "support8_authoritative_completion_locked"), ("project_to_active_domain_status", DOMAIN_SKELETON), ("general_theorem_readiness", GENERAL_READY)])
    upsert_section(D90 / "proof_system_contract_memo_90.md", "Project To Active Domain Refinement Round", contract_body)
    upsert_section(D90 / "proof_system_reproduction_report_90.md", "Project To Active Domain Refinement Round", contract_body)
    root_body = metric_table([
        ("latest_round", "project_to_active_status_domain_refinement"),
        ("selected_statement", SELECTED),
        ("project_to_active_status_domain_semantics_status", DOMAIN_SEMANTICS),
        ("project_to_active_domain_transfer_lemma_status", DOMAIN_TRANSFER),
        ("project_to_active_normal_form_interface_status", NORMAL_INTERFACE),
        ("project_to_active_domain_skeleton_status", DOMAIN_SKELETON),
        ("project_to_active_operation_status", PROJECT_STATUS),
        ("status_congruence_skeleton", STATUS_CONGRUENCE),
        ("support_reduction_skeleton", SUPPORT_REDUCTION),
        ("support_bound_skeleton", SUPPORT_BOUND),
        ("higher_support_necessity", HIGHER_SUPPORT),
        ("general_theorem_readiness", GENERAL_READY),
        ("next_action_1", NEXT1),
        ("next_action_2", NEXT2),
        ("next_action_3", NEXT3),
        ("caveat", "not_full_general_theorem_or_project_to_active_full_proof"),
    ])
    for root_name in ["project_status_summary.md", "current_workspace_reality_check.md", "current_status_authoritative_longform.md", "authoritative_completion_to_100_plan_longform.md", "theorem_data_promotion_to_100_plan_longform.md", "mathematical_progress_to_100_plan_longform.md", "progress_history_1_to_85_longform.md"]:
        upsert_section(B4 / root_name, "Project To Active Domain Refinement Round", root_body)
    upsert_section(RUNTIME / "project_status_summary_90.md", "Project To Active Domain Refinement Round", root_body)


def main() -> None:
    write_round_docs()
    update_rollups()


if __name__ == "__main__":
    main()
