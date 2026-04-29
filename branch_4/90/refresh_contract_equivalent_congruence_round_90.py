#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


D90 = Path(__file__).resolve().parent
B4 = D90.parent
RUNTIME = D90 / "runtime"

SELECTED = "equivalent_coordinate_status_preserved_under_refined_congruence_or_reduced_or_escape"
CONTRACT_STATUS = "partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open"
CONTRACT_SKELETON = "proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open"
DOMAIN_STATUS = "contract_equivalent_status_domain_transfer_proof_ready_quotient_domain_open"
NORMAL_STATUS = "contract_equivalent_normal_form_transfer_proof_ready_quotient_normal_form_open"
CONGRUENCE_STATUS = "equivalent_coordinate_congruence_payload_ready_domain_normal_form_open"
STATUS_CONGRUENCE = "partial_status_congruence_contract_equivalent_refined_domain_normal_form_open_remaining_canonical_alignment_measure_open"
SUPPORT_REDUCTION = "partition_ready_contract_equivalent_refined_domain_normal_form_open_remaining_canonical_alignment_measure_open"
SUPPORT_BOUND = "proof_ready_skeleton_contract_equivalent_refined_domain_normal_form_open_remaining_canonical_alignment_measure_open"
HIGHER_SUPPORT = "higher_support_deferred_after_contract_equivalent_congruence_domain_normal_form_open"
GENERAL_READY = "ready_for_canonical_compression_status_congruence_refinement"
NEXT1 = "canonical_compression_status_congruence_refinement"
NEXT2 = "project_to_active_status_domain_refinement"
NEXT3 = "family_chain_absorption_source_alignment_refinement"


def write_md(path: Path, title: str, body: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f"# {title}\n\n{body.strip()}\n", encoding="utf-8")


def write_metric(path: Path, rows: list[tuple[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        "metric\tvalue\n" + "\n".join(f"{k}\t{v}" for k, v in rows) + "\n",
        encoding="utf-8",
    )


def write_table(path: Path, header: list[str], rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = ["\t".join(header)]
    for row in rows:
        lines.append("\t".join(row.get(col, "") for col in header))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


scope_header = [
    "statement_key",
    "formal_statement",
    "assumptions",
    "conclusion",
    "operation_scope",
    "congruence_scope",
    "uses_equivalent_coordinate_notation",
    "uses_contraction_semantics",
    "uses_status_language",
    "uses_payload_refinement",
    "uses_status_domain_transfer",
    "uses_normal_form_transfer",
    "uses_higher_support_escape",
    "selected_for_attempt",
    "risk",
    "reason",
]

scope_rows = [
    {
        "statement_key": "equivalent_coordinate_payload_congruence_refined",
        "formal_statement": "Accepted equivalent coordinates have mergeable payload roles through quotient.",
        "assumptions": "W normal;accepted equivalence relation;payload roles compatible",
        "conclusion": "payload carriers push forward to quotient coordinates",
        "operation_scope": "contract_equivalent_support_coordinates",
        "congruence_scope": "payload congruence",
        "uses_equivalent_coordinate_notation": "1",
        "uses_contraction_semantics": "1",
        "uses_status_language": "0",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "0",
        "uses_normal_form_transfer": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "Payload roles are explicit, but arbitrary layerwise payload refinement through quotient normalization remains proof-sketch.",
    },
    {
        "statement_key": "equivalent_coordinate_status_domain_transfer",
        "formal_statement": "The contracted witness has a status domain compatible with the source status domain.",
        "assumptions": "W and quotient map q defined;status-domain dependencies compatible",
        "conclusion": "status predicate can be compared after quotient",
        "operation_scope": "contract_equivalent_support_coordinates",
        "congruence_scope": "status-domain transfer",
        "uses_equivalent_coordinate_notation": "1",
        "uses_contraction_semantics": "1",
        "uses_status_language": "1",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "This is required before counterexample-status congruence can be claimed.",
    },
    {
        "statement_key": "equivalent_coordinate_counterexample_status_congruence",
        "formal_statement": "Equivalent-coordinate contraction preserves counterexample-status predicate when payload, domain, and normal form transfer hold.",
        "assumptions": "payload congruence;status-domain transfer;normal-form transfer",
        "conclusion": "status preserved by quotient",
        "operation_scope": "contract_equivalent_support_coordinates",
        "congruence_scope": "counterexample-status congruence",
        "uses_equivalent_coordinate_notation": "1",
        "uses_contraction_semantics": "1",
        "uses_status_language": "1",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "high",
        "reason": "Central proof target; current artifacts do not prove the status predicate is determined by payload plus domain plus normal form.",
    },
    {
        "statement_key": "equivalent_coordinate_status_preserved_or_reduced_or_escape",
        "formal_statement": "If refined congruence fails, contraction either yields a valid smaller witness or a named blocker/deferred escape.",
        "assumptions": "nontrivial class;contracted status valid or failure classified",
        "conclusion": "preserved, reduced, or escaped branch",
        "operation_scope": "contract_equivalent_support_coordinates",
        "congruence_scope": "reduction or escape",
        "uses_equivalent_coordinate_notation": "1",
        "uses_contraction_semantics": "1",
        "uses_status_language": "1",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "Measure decrease is available; valid reduced-status fallback and hidden-case completeness remain proof obligations.",
    },
    {
        "statement_key": "full_equivalent_coordinate_status_congruence",
        "formal_statement": "Every coordinate contraction case has complete status congruence.",
        "assumptions": "all support-growth coordinate contraction witnesses",
        "conclusion": "complete status congruence",
        "operation_scope": "contract_equivalent_support_coordinates",
        "congruence_scope": "full congruence",
        "uses_equivalent_coordinate_notation": "1",
        "uses_contraction_semantics": "1",
        "uses_status_language": "1",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "0",
        "risk": "high",
        "reason": "Out of scope; current artifacts do not prove arbitrary quotient status-domain and normal-form transfer.",
    },
]

domain_header = [
    "domain_component",
    "definition",
    "source_condition",
    "contracted_condition",
    "transfer_rule",
    "proof_requirement",
    "failure_effect",
    "current_status",
    "caveat",
]

domain_rows = [
    {
        "domain_component": "source_status_domain",
        "definition": "The domain on which the source counterexample-status predicate is evaluated.",
        "source_condition": "W normal;status predicate defined over S",
        "contracted_condition": "status inputs indexed by S",
        "transfer_rule": "identity before quotient",
        "proof_requirement": "status dependency extraction",
        "failure_effect": "incomplete extraction blocks comparison",
        "current_status": "proof_sketch_ready",
        "caveat": "Source domain is not enough for quotient preservation.",
    },
    {
        "domain_component": "quotient_status_domain",
        "definition": "The domain on which the contracted witness status predicate is evaluated.",
        "source_condition": "q:S->S/~;W_contract defined",
        "contracted_condition": "status inputs indexed by S/~",
        "transfer_rule": "quotient domain after pushforward",
        "proof_requirement": "quotient status-domain definition",
        "failure_effect": "ill-formed quotient status becomes named blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Quotient status domain may differ from source.",
    },
    {
        "domain_component": "domain_projection",
        "definition": "The map from source status inputs to quotient status inputs.",
        "source_condition": "accepted equivalence classes fixed",
        "contracted_condition": "each input maps through q",
        "transfer_rule": "pushforward of status dependencies",
        "proof_requirement": "status dependency compatibility per class",
        "failure_effect": "incompatible status inputs block preservation",
        "current_status": "proof_sketch_ready",
        "caveat": "Not payload congruence alone.",
    },
    {
        "domain_component": "domain_quotient",
        "definition": "The quotient relation induced on status predicate inputs.",
        "source_condition": "status roles compatible inside each class",
        "contracted_condition": "merged input has one quotient role",
        "transfer_rule": "identify equivalent status roles",
        "proof_requirement": "status/certificate-role congruence",
        "failure_effect": "missing role congruence is named blocker",
        "current_status": "proof_ready_open",
        "caveat": "Needs status-domain sublemma.",
    },
    {
        "domain_component": "domain_equivalence",
        "definition": "Source and quotient domains are equivalent for status evaluation.",
        "source_condition": "payload/status/domain inputs all compatible",
        "contracted_condition": "W and W_contract comparable",
        "transfer_rule": "status predicate domain invariant",
        "proof_requirement": "payload+status dependency theorem",
        "failure_effect": "status comparison not well-defined",
        "current_status": "blocked_by_status_domain",
        "caveat": "Status predicate congruence depends on this.",
    },
    {
        "domain_component": "domain_preserved_case",
        "definition": "No status-domain information is lost by quotient.",
        "source_condition": "status dependencies constant on equivalence classes",
        "contracted_condition": "W_contract same status domain up to quotient",
        "transfer_rule": "status preservation branch",
        "proof_requirement": "domain equivalence and normal form",
        "failure_effect": "failure routes to reduction or blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Conditional only.",
    },
    {
        "domain_component": "domain_refined_case",
        "definition": "Quotient changes representation but remains a valid reduced status domain.",
        "source_condition": "nontrivial quotient;contracted status valid",
        "contracted_condition": "W_contract valid reduced witness",
        "transfer_rule": "reduction branch",
        "proof_requirement": "valid reduced-status theorem",
        "failure_effect": "failure routes to named blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Uses measure decrease, not preservation.",
    },
    {
        "domain_component": "domain_lost_case",
        "definition": "Status domain is not compatible after quotient.",
        "source_condition": "status inputs not constant on class",
        "contracted_condition": "W_contract not comparable",
        "transfer_rule": "no preservation claim",
        "proof_requirement": "named failure classification",
        "failure_effect": "named coordinate-contraction blocker",
        "current_status": "proved_under_current_scope_as_partition",
        "caveat": "Classification only.",
    },
    {
        "domain_component": "domain_escape_case",
        "definition": "Domain failure cannot be resolved by current operation proof obligations.",
        "source_condition": "operation blockers remain after refinement",
        "contracted_condition": "possible higher-support only after operation proofs",
        "transfer_rule": "close operations first",
        "proof_requirement": "higher-support bound later",
        "failure_effect": "deferred higher-support escape",
        "current_status": "higher_support_deferred",
        "caveat": "No support9+ scan.",
    },
    {
        "domain_component": "normal_form_relation",
        "definition": "Domain transfer requires quotient normal form.",
        "source_condition": "pushforward(W,q) defined",
        "contracted_condition": "W_contract normalized",
        "transfer_rule": "status predicate applicable",
        "proof_requirement": "normal-form transfer proof",
        "failure_effect": "normal-form failure blocks status congruence",
        "current_status": "proof_ready_normal_form_open",
        "caveat": "Separate from status-domain transfer.",
    },
    {
        "domain_component": "payload_refinement_relation",
        "definition": "Payload congruence feeds domain transfer but does not imply it alone.",
        "source_condition": "payload roles compatible",
        "contracted_condition": "payload carriers merged",
        "transfer_rule": "payload side of quotient",
        "proof_requirement": "layerwise payload proof",
        "failure_effect": "status may still fail",
        "current_status": "proof_sketch_ready",
        "caveat": "Payload congruence is insufficient for status congruence.",
    },
    {
        "domain_component": "counterexample_status_predicate_relation",
        "definition": "Status predicate congruence follows only after payload, domain, and normal-form transfer.",
        "source_condition": "all transfer premises hold",
        "contracted_condition": "W and W_contract comparable",
        "transfer_rule": "predicate equality/refinement",
        "proof_requirement": "counterexample-status congruence theorem",
        "failure_effect": "failure named as blocker",
        "current_status": "blocked_by_counterexample_status",
        "caveat": "No proof-completed promotion.",
    },
]

normal_header = [
    "normal_form_component",
    "definition",
    "source_condition",
    "contracted_condition",
    "transfer_status",
    "proof_requirement",
    "failure_effect",
    "current_status",
    "caveat",
]

normal_rows = [
    {
        "normal_form_component": "source_normal_form",
        "definition": "The selected support normal form for W.",
        "source_condition": "W normal over support S",
        "contracted_condition": "source status predicate applicable",
        "transfer_status": "source side available",
        "proof_requirement": "source normal-form contract",
        "failure_effect": "not enough for quotient",
        "current_status": "proved_under_current_scope_input",
        "caveat": "Source normal form is not quotient transfer.",
    },
    {
        "normal_form_component": "contracted_normal_form",
        "definition": "The selected support normal form for W_contract.",
        "source_condition": "pushforward(W,q) defined",
        "contracted_condition": "normalize(pushforward(W,q)) target form",
        "transfer_status": "normalization target named",
        "proof_requirement": "quotient normal-form proof",
        "failure_effect": "ill-formed output named blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Normalization semantics is not proof completion.",
    },
    {
        "normal_form_component": "quotient_normal_form_rule",
        "definition": "Normal-form invariants must be preserved or reconstructed over S/~.",
        "source_condition": "accepted relation with compatible roles",
        "contracted_condition": "quotient fields satisfy normal form",
        "transfer_status": "refinement by quotient",
        "proof_requirement": "quotient invariant preservation",
        "failure_effect": "normal-form failure blocks status comparison",
        "current_status": "proof_ready_open",
        "caveat": "Needs quotient-specific lemma.",
    },
    {
        "normal_form_component": "normal_form_preservation",
        "definition": "Quotient keeps the same status-applicable normal-form class.",
        "source_condition": "source invariants constant on classes",
        "contracted_condition": "W_contract in same class",
        "transfer_status": "preservation branch",
        "proof_requirement": "normal-form transfer theorem",
        "failure_effect": "failure routes to refinement or blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Conditional.",
    },
    {
        "normal_form_component": "normal_form_refinement",
        "definition": "Quotient changes representation but remains a valid refined normal form.",
        "source_condition": "nontrivial quotient;normalization succeeds",
        "contracted_condition": "W_contract valid refined witness",
        "transfer_status": "reduction/status-domain branch",
        "proof_requirement": "valid refined witness theorem",
        "failure_effect": "failure routes to named blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "May support reduction, not preservation.",
    },
    {
        "normal_form_component": "normal_form_failure",
        "definition": "Quotient normal-form invariant is not established.",
        "source_condition": "normal-form transfer missing",
        "contracted_condition": "W_contract not proven valid",
        "transfer_status": "no status preservation claim",
        "proof_requirement": "named failure classification",
        "failure_effect": "coordinate-contraction normal-form blocker",
        "current_status": "proved_under_current_scope_as_partition",
        "caveat": "Does not imply higher-support by itself.",
    },
    {
        "normal_form_component": "status_domain_relation",
        "definition": "Status-domain transfer requires normal-form transfer.",
        "source_condition": "status predicate needs normal form",
        "contracted_condition": "W_contract must be status-domain valid",
        "transfer_status": "normal form feeds domain transfer",
        "proof_requirement": "normal-form + domain theorem",
        "failure_effect": "status-domain transfer blocked",
        "current_status": "blocked_by_normal_form",
        "caveat": "Separate from counterexample-status predicate congruence.",
    },
    {
        "normal_form_component": "family_chain_source_relation",
        "definition": "Recognized family-chain source form must survive quotient when relevant.",
        "source_condition": "source recognizer fields quotient-compatible",
        "contracted_condition": "W_contract recognized or named failure",
        "transfer_status": "source-form transfer",
        "proof_requirement": "source-form quotient proof",
        "failure_effect": "source-form blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Not family-chain absorption.",
    },
    {
        "normal_form_component": "higher_support_relation",
        "definition": "Normal-form failure is not a higher-support theorem-data need until operation proofs close.",
        "source_condition": "operation proof open",
        "contracted_condition": "no true irreducible escape yet",
        "transfer_status": "deferred escape interface",
        "proof_requirement": "higher-support bound later",
        "failure_effect": "operation blocker first",
        "current_status": "higher_support_deferred",
        "caveat": "No support9+ scan.",
    },
]

lemma_header = [
    "lemma_component",
    "refined_statement",
    "assumptions",
    "conclusion",
    "equivalence_definition_used",
    "quotient_coordinate_used",
    "payload_congruence",
    "status_domain_transfer",
    "normal_form_transfer",
    "counterexample_status_congruence",
    "proof_status",
    "missing_hypothesis",
    "caveat",
]

lemma_rows = [
    {
        "lemma_component": "equivalence_relation_available",
        "refined_statement": "Accepted coordinate equivalence supplies finite quotient classes.",
        "assumptions": "W finite;accepted relation fixed",
        "conclusion": "S/~ is finite and q is total",
        "equivalence_definition_used": "equivalent_support_coordinate_notation_90.tsv",
        "quotient_coordinate_used": "q:S->S/~",
        "payload_congruence": "not_primary",
        "status_domain_transfer": "not_primary",
        "normal_form_transfer": "not_primary",
        "counterexample_status_congruence": "not_primary",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "semantic completeness outside accepted relation",
        "caveat": "Accepted relation only.",
    },
    {
        "lemma_component": "payload_congruence_refined",
        "refined_statement": "Payload roles are mergeable through quotient coordinates.",
        "assumptions": "payload-role compatibility in accepted classes",
        "conclusion": "quotient payload carrier is well-defined",
        "equivalence_definition_used": "equivalent_support_coordinate_notation_90.tsv",
        "quotient_coordinate_used": "quotient coordinate [C]",
        "payload_congruence": "proof_sketch_ready",
        "status_domain_transfer": "possible",
        "normal_form_transfer": "possible",
        "counterexample_status_congruence": "possible",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "layerwise payload refinement through normalization",
        "caveat": "Not enough for status congruence.",
    },
    {
        "lemma_component": "status_domain_transfer_refined",
        "refined_statement": "Source and quotient status domains are compatible only when status dependencies are quotient-invariant.",
        "assumptions": "status/certificate roles compatible;domain projection defined",
        "conclusion": "W and W_contract status domains comparable",
        "equivalence_definition_used": "contract_equivalent_status_domain_transfer_90.tsv",
        "quotient_coordinate_used": "quotient coordinate [C]",
        "payload_congruence": "possible",
        "status_domain_transfer": "proof_ready_open",
        "normal_form_transfer": "possible",
        "counterexample_status_congruence": "possible",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "status-domain invariance theorem",
        "caveat": "Open but first-class.",
    },
    {
        "lemma_component": "normal_form_transfer_refined",
        "refined_statement": "Quotient normalization preserves the normal form required by status semantics.",
        "assumptions": "pushforward(W,q) defined",
        "conclusion": "W_contract status-applicable",
        "equivalence_definition_used": "contract_equivalent_normal_form_transfer_90.tsv",
        "quotient_coordinate_used": "S/~",
        "payload_congruence": "possible",
        "status_domain_transfer": "possible",
        "normal_form_transfer": "proof_ready_open",
        "counterexample_status_congruence": "possible",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "quotient normal-form preservation",
        "caveat": "Open but first-class.",
    },
    {
        "lemma_component": "payload_domain_normal_form_to_status",
        "refined_statement": "Payload congruence plus domain and normal-form transfer imply status congruence only if status predicate is determined by those data.",
        "assumptions": "payload/domain/normal premises hold",
        "conclusion": "counterexample-status predicate is invariant",
        "equivalence_definition_used": "status_preservation_language_90.tsv",
        "quotient_coordinate_used": "quotient coordinate [C]",
        "payload_congruence": "yes",
        "status_domain_transfer": "yes",
        "normal_form_transfer": "yes",
        "counterexample_status_congruence": "blocked",
        "proof_status": "blocked_by_counterexample_status",
        "missing_hypothesis": "status predicate determination theorem",
        "caveat": "Central unresolved theorem.",
    },
    {
        "lemma_component": "counterexample_status_congruence_refined",
        "refined_statement": "Equivalent-coordinate contraction preserves counterexample status under refined congruence.",
        "assumptions": "all refined congruence premises hold",
        "conclusion": "status preserved",
        "equivalence_definition_used": "equivalent_coordinate_congruence_lemma_90.tsv",
        "quotient_coordinate_used": "quotient coordinate [C]",
        "payload_congruence": "yes",
        "status_domain_transfer": "yes",
        "normal_form_transfer": "yes",
        "counterexample_status_congruence": "proof_ready_open",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "counterexample-status congruence theorem",
        "caveat": "Conditional only.",
    },
    {
        "lemma_component": "domain_change_reduction",
        "refined_statement": "If status domain changes but W_contract is a valid reduced counterexample, contraction is a smaller witness.",
        "assumptions": "nontrivial class;W_contract valid reduced obstruction",
        "conclusion": "smaller witness branch",
        "equivalence_definition_used": "contract_equivalent_support_coordinates_smaller_witness_90.tsv",
        "quotient_coordinate_used": "S/~",
        "payload_congruence": "possible",
        "status_domain_transfer": "possible",
        "normal_form_transfer": "yes",
        "counterexample_status_congruence": "reduced_status",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "valid reduced-status theorem",
        "caveat": "Uses measure decrease.",
    },
    {
        "lemma_component": "failure_classification",
        "refined_statement": "If refined congruence fails, status does not silently succeed.",
        "assumptions": "payload/domain/normal/status premise missing",
        "conclusion": "named blocker or deferred higher-support escape",
        "equivalence_definition_used": "higher_support_escape_interface_90.tsv",
        "quotient_coordinate_used": "quotient attempt optional",
        "payload_congruence": "possible",
        "status_domain_transfer": "possible",
        "normal_form_transfer": "possible",
        "counterexample_status_congruence": "possible",
        "proof_status": "proved_under_current_scope_as_partition",
        "missing_hypothesis": "higher-support bound after operation proofs close",
        "caveat": "Classification only.",
    },
    {
        "lemma_component": "overall_status",
        "refined_statement": "Equivalent-coordinate congruence is payload-ready but status-domain/normal-form/status-predicate open.",
        "assumptions": "rows above combined",
        "conclusion": "proof-ready refinement, not completed",
        "equivalence_definition_used": "accepted equivalence",
        "quotient_coordinate_used": "q:S->S/~",
        "payload_congruence": "payload proof-sketch",
        "status_domain_transfer": "status-domain open",
        "normal_form_transfer": "normal-form open",
        "counterexample_status_congruence": "status predicate open",
        "proof_status": CONGRUENCE_STATUS,
        "missing_hypothesis": "status-domain transfer;normal-form transfer;status predicate determination;valid reduced-status theorem",
        "caveat": "No full contraction proof.",
    },
]

obligation_header = [
    "obligation_key",
    "statement",
    "required_for_selected_statement",
    "existing_verified_inputs",
    "missing_sublemmas",
    "proof_status",
    "dependency_on_payload_congruence",
    "dependency_on_status_domain",
    "dependency_on_normal_form",
    "dependency_on_counterexample_status",
    "dependency_on_smaller_witness",
    "dependency_on_higher_support",
    "can_attempt_now",
    "recommended_next_action",
]

obligation_rows = [
    {
        "obligation_key": "contract_equivalent_congruence_refinement_language_well_defined",
        "statement": "The refinement uses payload congruence, status-domain transfer, normal-form transfer, status congruence, reduction, and escape labels.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "equivalent_support_coordinate_notation_90.tsv;status_preservation_language_90.tsv",
        "missing_sublemmas": "none",
        "proof_status": "proved_under_current_scope",
        "dependency_on_payload_congruence": "no",
        "dependency_on_status_domain": "no",
        "dependency_on_normal_form": "no",
        "dependency_on_counterexample_status": "no",
        "dependency_on_smaller_witness": "no",
        "dependency_on_higher_support": "no",
        "can_attempt_now": "1",
        "recommended_next_action": "use_refinement_language",
    },
    {
        "obligation_key": "payload_congruence_available",
        "statement": "Accepted equivalent-coordinate classes include payload-role compatibility.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "equivalent_coordinate_congruence_lemma_90.tsv",
        "missing_sublemmas": "layerwise payload refinement under quotient",
        "proof_status": "proof_sketch_ready",
        "dependency_on_payload_congruence": "yes",
        "dependency_on_status_domain": "possible",
        "dependency_on_normal_form": "possible",
        "dependency_on_counterexample_status": "possible",
        "dependency_on_smaller_witness": "no",
        "dependency_on_higher_support": "no",
        "can_attempt_now": "1",
        "recommended_next_action": "payload_congruence_refinement_later",
    },
    {
        "obligation_key": "status_domain_transfer_well_defined",
        "statement": "Source and contracted status domains are comparable only under quotient-invariant status dependencies.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "contract_equivalent_status_domain_transfer_90.tsv",
        "missing_sublemmas": "status-domain invariance under quotient",
        "proof_status": "needs_status_domain_sublemma",
        "dependency_on_payload_congruence": "possible",
        "dependency_on_status_domain": "yes",
        "dependency_on_normal_form": "possible",
        "dependency_on_counterexample_status": "yes",
        "dependency_on_smaller_witness": "possible",
        "dependency_on_higher_support": "no",
        "can_attempt_now": "1",
        "recommended_next_action": "status_domain_transfer_refinement",
    },
    {
        "obligation_key": "normal_form_transfer_well_defined",
        "statement": "W_contract must satisfy the selected normal form after quotient normalization.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "contract_equivalent_normal_form_transfer_90.tsv",
        "missing_sublemmas": "quotient normal-form preservation",
        "proof_status": "needs_normal_form_sublemma",
        "dependency_on_payload_congruence": "possible",
        "dependency_on_status_domain": "yes",
        "dependency_on_normal_form": "yes",
        "dependency_on_counterexample_status": "possible",
        "dependency_on_smaller_witness": "possible",
        "dependency_on_higher_support": "no",
        "can_attempt_now": "1",
        "recommended_next_action": "normal_form_transfer_refinement",
    },
    {
        "obligation_key": "payload_plus_domain_implies_status_congruence",
        "statement": "Payload congruence plus domain transfer implies status congruence only if the status predicate is determined by those data.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "equivalent_coordinate_congruence_refinement_90.tsv",
        "missing_sublemmas": "status predicate determination theorem",
        "proof_status": "needs_counterexample_status_sublemma",
        "dependency_on_payload_congruence": "yes",
        "dependency_on_status_domain": "yes",
        "dependency_on_normal_form": "possible",
        "dependency_on_counterexample_status": "yes",
        "dependency_on_smaller_witness": "possible",
        "dependency_on_higher_support": "no",
        "can_attempt_now": "1",
        "recommended_next_action": "status_predicate_congruence_refinement",
    },
    {
        "obligation_key": "normal_form_transfer_supports_status_congruence",
        "statement": "Normal-form transfer makes the quotient witness eligible for status comparison.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "support_notation_and_normal_form_90.md;contract_equivalent_normal_form_transfer_90.tsv",
        "missing_sublemmas": "normal-form transfer proof",
        "proof_status": "proof_sketch_ready",
        "dependency_on_payload_congruence": "possible",
        "dependency_on_status_domain": "yes",
        "dependency_on_normal_form": "yes",
        "dependency_on_counterexample_status": "yes",
        "dependency_on_smaller_witness": "possible",
        "dependency_on_higher_support": "no",
        "can_attempt_now": "1",
        "recommended_next_action": "normal_form_transfer_refinement",
    },
    {
        "obligation_key": "contraction_preserves_status_under_refined_congruence",
        "statement": "If payload, domain, normal-form, and status predicate congruence hold, contraction preserves status.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "contract_equivalent_status_congruence_refinement_skeleton_90.tsv",
        "missing_sublemmas": "refined congruence theorem",
        "proof_status": "proof_sketch_ready",
        "dependency_on_payload_congruence": "yes",
        "dependency_on_status_domain": "yes",
        "dependency_on_normal_form": "yes",
        "dependency_on_counterexample_status": "yes",
        "dependency_on_smaller_witness": "possible",
        "dependency_on_higher_support": "no",
        "can_attempt_now": "1",
        "recommended_next_action": "status_predicate_congruence_refinement",
    },
    {
        "obligation_key": "contraction_reduces_to_smaller_witness_if_status_domain_changes",
        "statement": "If quotient changes status domain but W_contract is valid reduced status, nontrivial quotient gives smaller witness.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "contract_equivalent_support_coordinates_smaller_witness_90.tsv",
        "missing_sublemmas": "valid reduced-status theorem for quotient witness",
        "proof_status": "needs_smaller_witness_sublemma",
        "dependency_on_payload_congruence": "possible",
        "dependency_on_status_domain": "yes",
        "dependency_on_normal_form": "yes",
        "dependency_on_counterexample_status": "yes",
        "dependency_on_smaller_witness": "yes",
        "dependency_on_higher_support": "possible",
        "can_attempt_now": "1",
        "recommended_next_action": "reduced_status_validity_refinement",
    },
    {
        "obligation_key": "contraction_failure_is_named_escape",
        "statement": "If refined congruence or valid reduction is not established, failure is named.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "higher_support_escape_interface_90.tsv;operation_open_vs_higher_support_required_90.tsv",
        "missing_sublemmas": "higher-support bound after operation proofs close",
        "proof_status": "proved_under_current_scope",
        "dependency_on_payload_congruence": "possible",
        "dependency_on_status_domain": "possible",
        "dependency_on_normal_form": "possible",
        "dependency_on_counterexample_status": "possible",
        "dependency_on_smaller_witness": "no",
        "dependency_on_higher_support": "yes",
        "can_attempt_now": "1",
        "recommended_next_action": "use_failure_classification",
    },
    {
        "obligation_key": "no_hidden_contraction_congruence_failure_case",
        "statement": "The contraction branch has no hidden class outside preserved, reduced, singleton/no-op, named blocker, or deferred higher-support escape.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "contract_equivalent_status_congruence_refinement_scope_inventory_90.tsv",
        "missing_sublemmas": "arbitrary witness completeness",
        "proof_status": "proof_sketch_ready",
        "dependency_on_payload_congruence": "possible",
        "dependency_on_status_domain": "possible",
        "dependency_on_normal_form": "possible",
        "dependency_on_counterexample_status": "possible",
        "dependency_on_smaller_witness": "possible",
        "dependency_on_higher_support": "yes",
        "can_attempt_now": "1",
        "recommended_next_action": "complete_taxonomy_after_operation_refinements",
    },
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
        "sublemma_key": "contract_equivalent_congruence_refinement_language_well_defined",
        "proof_status": "proved_under_current_scope",
        "assumptions": "equivalent coordinate notation and status language are fixed",
        "conclusion": "refinement outcomes are comparable",
        "proof_summary": "The refinement distinguishes payload congruence, domain transfer, normal-form transfer, status congruence, reduction, named blocker, and deferred higher-support escape.",
        "evidence_path": "branch_4/90/runtime/equivalent_support_coordinate_notation_90.tsv;branch_4/90/runtime/status_preservation_language_90.tsv",
        "missing_hypothesis": "none",
        "next_action": "use_refinement_language",
    },
    {
        "sublemma_key": "payload_congruence_available",
        "proof_status": "proof_sketch_only",
        "assumptions": "accepted equivalence includes payload-role compatibility",
        "conclusion": "payload roles merge through quotient",
        "proof_summary": "Payload roles are part of accepted equivalence, but layerwise payload refinement through quotient normalization remains a dedicated proof.",
        "evidence_path": "branch_4/90/runtime/equivalent_coordinate_congruence_refinement_90.tsv",
        "missing_hypothesis": "layerwise payload refinement under quotient",
        "next_action": "payload_congruence_refinement_later",
    },
    {
        "sublemma_key": "status_domain_transfer_well_defined",
        "proof_status": "proof_sketch_only",
        "assumptions": "status/certificate dependencies are quotient-compatible",
        "conclusion": "source and quotient status domains are comparable",
        "proof_summary": "The transfer relation is first-class, but current artifacts do not prove arbitrary quotient status-domain invariance.",
        "evidence_path": "branch_4/90/runtime/contract_equivalent_status_domain_transfer_90.tsv",
        "missing_hypothesis": "status-domain invariance theorem",
        "next_action": "status_domain_transfer_refinement",
    },
    {
        "sublemma_key": "normal_form_transfer_well_defined",
        "proof_status": "proof_sketch_only",
        "assumptions": "pushforward(W,q) is defined",
        "conclusion": "W_contract is status-domain eligible after normalization",
        "proof_summary": "The normal-form target is named, but quotient preservation of all status-relevant normal-form invariants remains open.",
        "evidence_path": "branch_4/90/runtime/contract_equivalent_normal_form_transfer_90.tsv",
        "missing_hypothesis": "quotient normal-form preservation",
        "next_action": "normal_form_transfer_refinement",
    },
    {
        "sublemma_key": "payload_plus_domain_implies_status_congruence",
        "proof_status": "blocked_by_counterexample_status",
        "assumptions": "payload congruence and domain transfer hold",
        "conclusion": "status predicate is invariant",
        "proof_summary": "This requires a theorem that counterexample status is determined by quotient-compatible payload, domain, and normal-form data.",
        "evidence_path": "branch_4/90/runtime/equivalent_coordinate_congruence_refinement_90.tsv",
        "missing_hypothesis": "status predicate determination theorem",
        "next_action": "status_predicate_congruence_refinement",
    },
    {
        "sublemma_key": "normal_form_transfer_supports_status_congruence",
        "proof_status": "proof_sketch_only",
        "assumptions": "normal-form transfer holds",
        "conclusion": "quotient witness is eligible for status comparison",
        "proof_summary": "Normal-form transfer supplies the status-domain precondition, but does not alone prove status congruence.",
        "evidence_path": "branch_4/90/runtime/contract_equivalent_normal_form_transfer_90.tsv",
        "missing_hypothesis": "normal-form transfer theorem",
        "next_action": "normal_form_transfer_refinement",
    },
    {
        "sublemma_key": "contraction_preserves_status_under_refined_congruence",
        "proof_status": "proof_sketch_only",
        "assumptions": "payload, domain, normal-form, and status predicate congruence hold",
        "conclusion": "status is preserved",
        "proof_summary": "Under all refined congruence premises, preservation is direct; the missing premises are explicitly isolated.",
        "evidence_path": "branch_4/90/runtime/contract_equivalent_status_congruence_refinement_skeleton_90.tsv",
        "missing_hypothesis": "refined congruence theorem",
        "next_action": "status_predicate_congruence_refinement",
    },
    {
        "sublemma_key": "contraction_reduces_to_smaller_witness_if_status_domain_changes",
        "proof_status": "blocked_by_smaller_witness",
        "assumptions": "nontrivial quotient and contracted status remains valid",
        "conclusion": "W_contract is a smaller witness",
        "proof_summary": "Measure decrease is proved, but validity of a changed quotient status as a reduced counterexample is not yet proved.",
        "evidence_path": "branch_4/90/runtime/contract_equivalent_support_coordinates_smaller_witness_90.tsv",
        "missing_hypothesis": "valid reduced-status theorem",
        "next_action": "reduced_status_validity_refinement",
    },
    {
        "sublemma_key": "contraction_failure_is_named_escape",
        "proof_status": "proved_under_current_scope",
        "assumptions": "preservation or valid reduction is not established",
        "conclusion": "failure is a named blocker or deferred higher-support escape",
        "proof_summary": "The higher-support interface keeps operation-proof failure separate from true higher-support need; no failure is hidden as success.",
        "evidence_path": "branch_4/90/runtime/higher_support_escape_interface_90.tsv",
        "missing_hypothesis": "none",
        "next_action": "use_failure_classification",
    },
]


def write_core_refinement_docs() -> None:
    write_md(
        D90 / "contract_equivalent_status_congruence_refinement_scope_memo_90.md",
        "Contract Equivalent Status Congruence Refinement Scope Memo 90",
        f"""
## selected target

`contract_equivalent_status_congruence_refinement`

Selected statement:
`{SELECTED}`.

This round does not prove full equivalent-coordinate status congruence. It
selects the payload/domain/normal-form/status predicate decomposition and keeps
`full_equivalent_coordinate_status_congruence` out of scope.

Runtime inventory:
`branch_4/90/runtime/contract_equivalent_status_congruence_refinement_scope_inventory_90.tsv`.
""",
    )
    write_table(RUNTIME / "contract_equivalent_status_congruence_refinement_scope_inventory_90.tsv", scope_header, scope_rows)

    write_md(
        D90 / "contract_equivalent_status_domain_transfer_90.md",
        "Contract Equivalent Status Domain Transfer 90",
        f"""
## status

`{DOMAIN_STATUS}`

Status-domain transfer asks whether the source counterexample-status predicate
and the quotient witness status predicate are defined over compatible domains.
For coordinate contraction this is not automatic from payload congruence. The
quotient may identify active coordinates, so the status predicate domain can
change even if payload roles are mergeable.

The transfer is proof-ready but open on quotient status-domain invariance,
normal-form eligibility, and valid reduced-status fallback if the quotient
changes the domain.

Runtime table:
`branch_4/90/runtime/contract_equivalent_status_domain_transfer_90.tsv`.
""",
    )
    write_table(RUNTIME / "contract_equivalent_status_domain_transfer_90.tsv", domain_header, domain_rows)

    write_md(
        D90 / "contract_equivalent_normal_form_transfer_90.md",
        "Contract Equivalent Normal Form Transfer 90",
        f"""
## status

`{NORMAL_STATUS}`

Coordinate contraction constructs `W_contract = normalize(pushforward(W,q))`.
The normalizing step names the target form, but it does not prove that every
status-relevant normal-form invariant transfers through quotient.

Runtime table:
`branch_4/90/runtime/contract_equivalent_normal_form_transfer_90.tsv`.
""",
    )
    write_table(RUNTIME / "contract_equivalent_normal_form_transfer_90.tsv", normal_header, normal_rows)

    write_md(
        D90 / "equivalent_coordinate_congruence_refinement_90.md",
        "Equivalent Coordinate Congruence Refinement 90",
        f"""
## status

`{CONGRUENCE_STATUS}`

The previous equivalent-coordinate congruence lemma is refined into accepted
equivalence availability, payload-role congruence, status-domain transfer,
normal-form transfer, counterexample-status predicate congruence, and
reduction/escape classification.

Payload congruence is proof-sketch ready. Counterexample-status congruence is
not completed because payload congruence alone does not prove status-domain or
normal-form transfer.

Runtime table:
`branch_4/90/runtime/equivalent_coordinate_congruence_refinement_90.tsv`.
""",
    )
    write_table(RUNTIME / "equivalent_coordinate_congruence_refinement_90.tsv", lemma_header, lemma_rows)

    write_md(
        D90 / "contract_equivalent_status_congruence_refinement_obligations_90.md",
        "Contract Equivalent Status Congruence Refinement Obligations 90",
        f"""
## status

`contract_equivalent_congruence_refinement_obligations_domain_normal_form_open`

The refinement has `10` first-class obligations. Language and failure
classification are current-scope proved. Payload and conditional preservation
are proof-sketch. Status-domain transfer, normal-form transfer,
counterexample-status predicate determination, and valid reduced-status fallback
remain open.

Runtime table:
`branch_4/90/runtime/contract_equivalent_status_congruence_refinement_obligations_90.tsv`.
""",
    )
    write_table(RUNTIME / "contract_equivalent_status_congruence_refinement_obligations_90.tsv", obligation_header, obligation_rows)

    write_md(
        D90 / "contract_equivalent_status_congruence_refinement_sublemma_proofs_90.md",
        "Contract Equivalent Status Congruence Refinement Sublemma Proofs 90",
        f"""
## proof attempt status

- proved under current scope: `2`
- proof sketch only: `5`
- blocked: `2`

The proved rows are language well-definedness and named failure classification.
The blocked rows are status predicate determination and valid reduced-status
fallback. The other rows are proof-sketch/proof-ready and remain conditional.

Runtime table:
`branch_4/90/runtime/contract_equivalent_status_congruence_refinement_sublemma_proofs_90.tsv`.
""",
    )
    write_table(RUNTIME / "contract_equivalent_status_congruence_refinement_sublemma_proofs_90.tsv", sublemma_header, sublemma_rows)

    write_md(
        D90 / "contract_equivalent_status_congruence_refinement_skeleton_90.md",
        "Contract Equivalent Status Congruence Refinement Skeleton 90",
        f"""
## lemma

`{SELECTED}`

## statement

Let `W` be a normal support `>8` witness with support `S`, accepted coordinate
equivalence `~`, quotient map `q:S->S/~`, and nontrivial quotient witness
`W_contract = normalize(pushforward(W,q))`.

Coordinate contraction either preserves counterexample status under payload
congruence, status-domain transfer, normal-form transfer, and status predicate
congruence; gives a smaller witness when changed quotient status is valid; or
routes failure to a named coordinate-contraction blocker or deferred
higher-support escape.

## status

`{CONTRACT_SKELETON}`

This is not a completed proof of `contract_equivalent_support_coordinates`, not
a full support reduction proof, not support8 sufficiency, and not a full general
theorem.

Runtime skeleton:
`branch_4/90/runtime/contract_equivalent_status_congruence_refinement_skeleton_90.tsv`.
""",
    )
    write_metric(
        RUNTIME / "contract_equivalent_status_congruence_refinement_skeleton_90.tsv",
        [
            ("lemma_name", SELECTED),
            ("selected_statement", SELECTED),
            (
                "exact_statement",
                "For a normal support>8 witness W with accepted equivalence ~ and nontrivial quotient q:S->S/~, W_contract=normalize(pushforward(W,q)) preserves counterexample status under payload congruence, status-domain transfer, normal-form transfer, and status-predicate congruence, otherwise yields a smaller valid reduced witness or a named coordinate-contraction blocker/deferred higher-support escape.",
            ),
            ("assumption_count", "9"),
            (
                "conclusion",
                "Coordinate-contraction congruence is first-class and proof-ready; payload congruence is proof-sketch ready, while status-domain, normal-form, status-predicate, and reduced-status obligations remain open.",
            ),
            (
                "equivalent_coordinate_definition",
                "accepted classes combine syntactic, payload, canonical, status/certificate, and family-chain source-role compatibility",
            ),
            ("quotient_witness_relation", "W_contract=normalize(pushforward(W,q))"),
            ("status_domain_transfer", DOMAIN_STATUS),
            ("normal_form_transfer", NORMAL_STATUS),
            ("payload_congruence", "proof_sketch_ready_payload_role_congruence"),
            ("counterexample_status_congruence", CONGRUENCE_STATUS),
            ("smaller_witness_fallback", "proof_sketch_ready_if_quotient_status_is_valid_reduced_counterexample"),
            ("failure_to_escape_case", "proved_under_current_scope_as_named_coordinate_contraction_blocker_or_deferred_higher_support_escape"),
            ("relation_to_previous_coordinate_status_skeleton", "refines proof_ready_skeleton_contract_equivalent_status_congruence_open into domain/normal-form/status-predicate obligations"),
            ("relation_to_project_to_active_locality_result", "separate operation; project removes inactive support while contraction merges active equivalent coordinates; both need status-domain/normal-form transfer"),
            ("relation_to_status_congruence_bridge", "refines coordinate-contraction row to payload/domain/normal-form open proof-ready skeleton"),
            ("missing_steps", "status-domain invariance;quotient normal-form transfer;layerwise payload refinement;status predicate determination;valid reduced-status theorem;family-chain source-form quotient transfer"),
            ("exact_caveat", "contract_equivalent_operation_not_fully_proved_support_bound_not_completed_support8_sufficiency_not_proved_no_support9_scan_full_general_theorem_not_proved"),
            ("final_status", CONTRACT_SKELETON),
        ],
    )

    write_metric(RUNTIME / "contract_equivalent_congruence_refinement_fingerprint_90.tsv", [
        ("contract_equivalent_congruence_refinement", CONTRACT_SKELETON),
        ("selected_statement", SELECTED),
        ("obligation_count", "10"),
        ("proved_under_current_scope_sublemma_count", "2"),
        ("proof_sketch_only_sublemma_count", "5"),
        ("blocked_sublemma_count", "2"),
        ("fingerprint", f"{CONTRACT_SKELETON}|10|2|5|2|{NEXT1}"),
    ])
    write_metric(RUNTIME / "contract_equivalent_status_domain_transfer_fingerprint_90.tsv", [
        ("contract_equivalent_status_domain_transfer", DOMAIN_STATUS),
        ("domain_component_count", str(len(domain_rows))),
        ("fingerprint", f"{DOMAIN_STATUS}|{len(domain_rows)}|quotient_domain_open"),
    ])
    write_metric(RUNTIME / "contract_equivalent_normal_form_transfer_fingerprint_90.tsv", [
        ("contract_equivalent_normal_form_transfer", NORMAL_STATUS),
        ("normal_form_component_count", str(len(normal_rows))),
        ("fingerprint", f"{NORMAL_STATUS}|{len(normal_rows)}|quotient_normal_form_open"),
    ])


def write_existing_coordinate_status_docs() -> None:
    write_md(
        D90 / "contract_equivalent_status_skeleton_90.md",
        "Contract Equivalent Status Skeleton 90",
        f"""
## status after congruence refinement

`{CONTRACT_SKELETON}`

The selected coordinate-contraction statement is refined to:

`{SELECTED}`.

This does not prove the full operation. It separates equivalent-coordinate
status congruence into payload congruence, status-domain transfer, normal-form
transfer, counterexample-status predicate congruence, smaller-witness fallback,
and named escape classification.

Runtime skeleton:
`branch_4/90/runtime/contract_equivalent_status_skeleton_90.tsv`.
""",
    )
    write_metric(
        RUNTIME / "contract_equivalent_status_skeleton_90.tsv",
        [
            ("lemma_name", SELECTED),
            ("selected_statement", SELECTED),
            ("exact_statement", "Coordinate contraction preserves status under refined congruence, reduces if quotient status remains valid, or names blocker/deferred escape."),
            ("assumption_count", "10"),
            ("conclusion", "Coordinate-contraction status blocker is refined to payload/domain/normal-form/status-predicate open skeleton."),
            ("equivalent_coordinate_definition", "accepted relation includes syntactic,payload,canonical,status/certificate,family-chain source roles"),
            ("quotient_contracted_relation", "W_contract=normalize(pushforward(W,q))"),
            ("congruence_lemma_status", CONGRUENCE_STATUS),
            ("status_domain_transfer", DOMAIN_STATUS),
            ("normal_form_transfer", NORMAL_STATUS),
            ("payload_preservation", "proof_sketch_ready_under_payload_role_congruence"),
            ("counterexample_status_preservation", "proof_sketch_ready_if_payload_domain_normal_form_and_status_predicate_congruence_hold"),
            ("smaller_witness_fallback", "proof_sketch_ready_if_quotient_status_is_valid_reduced_counterexample"),
            ("measure_decrease_use", "proved_under_current_scope_for_nontrivial_equivalence_class"),
            ("failure_to_escape_case", "proved_under_current_scope_as_named_coordinate_contraction_blocker_or_deferred_higher_support_escape"),
            ("contract_equivalent_status_obligation_count", "10"),
            ("proved_under_current_scope_sublemma_count", "2"),
            ("proof_sketch_only_sublemma_count", "5"),
            ("blocked_sublemma_count", "2"),
            ("missing_steps", "quotient status-domain invariance;quotient normal-form transfer;layerwise payload refinement;status predicate determination;valid reduced-status fallback;family-chain source-form quotient transfer"),
            ("exact_caveat", "contract_equivalent_status_not_proved_contract_equivalent_operation_not_fully_proved_full_support_reduction_not_proved_no_support9_scan_full_general_theorem_not_proved"),
            ("final_status", CONTRACT_SKELETON),
            ("next_blocker", NEXT1),
        ],
    )

    write_md(
        D90 / "equivalent_coordinate_congruence_lemma_90.md",
        "Equivalent Coordinate Congruence Lemma 90",
        f"""
## status after congruence refinement

`{CONGRUENCE_STATUS}`

Payload-role congruence remains proof-sketch ready under the accepted
equivalence contract. Counterexample-status congruence is not completed: it
requires quotient status-domain transfer, quotient normal-form transfer, status
predicate determination from quotient-compatible data, and valid reduced-status
fallback if the quotient changes the status domain.

Runtime table:
`branch_4/90/runtime/equivalent_coordinate_congruence_lemma_90.tsv`.
""",
    )
    # Keep the legacy column shape but refresh rows from the refined lemma.
    legacy_header = [
        "lemma_component",
        "statement",
        "assumptions",
        "conclusion",
        "equivalence_definition_used",
        "quotient_coordinate_used",
        "payload_dependency",
        "counterexample_status_dependency",
        "canonicalization_dependency",
        "proof_status",
        "missing_hypothesis",
        "caveat",
    ]
    legacy_rows = []
    for row in lemma_rows:
        legacy_rows.append({
            "lemma_component": row["lemma_component"],
            "statement": row["refined_statement"],
            "assumptions": row["assumptions"],
            "conclusion": row["conclusion"],
            "equivalence_definition_used": row["equivalence_definition_used"],
            "quotient_coordinate_used": row["quotient_coordinate_used"],
            "payload_dependency": row["payload_congruence"],
            "counterexample_status_dependency": row["counterexample_status_congruence"],
            "canonicalization_dependency": row["normal_form_transfer"],
            "proof_status": row["proof_status"],
            "missing_hypothesis": row["missing_hypothesis"],
            "caveat": row["caveat"],
        })
    write_table(RUNTIME / "equivalent_coordinate_congruence_lemma_90.tsv", legacy_header, legacy_rows)

    write_md(
        D90 / "contract_equivalent_status_obligations_90.md",
        "Contract Equivalent Status Obligations 90",
        f"""
## status after congruence refinement

`contract_equivalent_status_obligations_domain_normal_form_open`

The previous status obligations are refined by
`contract_equivalent_status_congruence_refinement_obligations_90.md`.

Runtime table:
`branch_4/90/runtime/contract_equivalent_status_obligations_90.tsv`.
""",
    )
    write_table(RUNTIME / "contract_equivalent_status_obligations_90.tsv", obligation_header, obligation_rows)

    write_md(
        D90 / "contract_equivalent_status_sublemma_proofs_90.md",
        "Contract Equivalent Status Sublemma Proofs 90",
        f"""
## status after congruence refinement

`contract_equivalent_status_sublemma_proofs_domain_normal_form_open`

Language and failure classification are current-scope proved. Payload
congruence, status-domain transfer, normal-form transfer, and conditional
preservation are proof-sketch/proof-ready. Status predicate determination and
valid reduced-status fallback remain blocked.

Runtime proofs:
`branch_4/90/runtime/contract_equivalent_status_sublemma_proofs_90.tsv`.
""",
    )
    write_table(RUNTIME / "contract_equivalent_status_sublemma_proofs_90.tsv", sublemma_header, sublemma_rows)


operation_header = [
    "operation_key",
    "current_operation_status",
    "input_case",
    "status_behavior",
    "measure_decrease_status",
    "payload_refinement_status",
    "normal_form_status",
    "counterexample_status_status",
    "contradiction_status",
    "escape_status",
    "proof_status",
    "missing_sublemma",
    "next_action",
]

operation_rows = [
    {
        "operation_key": "delete_redundant_support_coordinate",
        "current_operation_status": "proof_ready_skeleton_selected_operation",
        "input_case": "redundant coordinate unused by payload/certificate",
        "status_behavior": "status_reduced_to_smaller_witness",
        "measure_decrease_status": "support_size_decreases_by_one",
        "payload_refinement_status": "payload unaffected under redundancy precondition",
        "normal_form_status": "proof_sketch_ready_for_arbitrary_schema",
        "counterexample_status_status": "proved_under_current_scope_under_redundancy",
        "contradiction_status": "not_applicable",
        "escape_status": "named blocker if redundancy precondition fails",
        "proof_status": "proved_under_current_scope_for_selected_status_case",
        "missing_sublemma": "normal-form/payload transfer for arbitrary schemas",
        "next_action": "use_as_closed_selected_case",
    },
    {
        "operation_key": "project_to_active_support",
        "current_operation_status": "partial_project_to_active_locality_proof_ready_status_domain_open",
        "input_case": "active support strict subset contains payload/certificate/status fields",
        "status_behavior": "status_preserved_under_locality_or_reduced_or_named_operation_blocker",
        "measure_decrease_status": "support_size_decreases_when_active_subset_strict",
        "payload_refinement_status": "payload locality proved under active support contract",
        "normal_form_status": "proof_sketch_ready_status_domain_open",
        "counterexample_status_status": "proof_ready_skeleton_project_to_active_locality_status_domain_open",
        "contradiction_status": "not_applicable",
        "escape_status": "status_escape_to_named_project_to_active_blocker_or_deferred_higher_support_escape",
        "proof_status": "proof_ready_skeleton",
        "missing_sublemma": "status-domain invariance;normal-form transfer;complete status dependency extraction;valid reduced-status fallback",
        "next_action": NEXT2,
    },
    {
        "operation_key": "contract_equivalent_support_coordinates",
        "current_operation_status": CONTRACT_STATUS,
        "input_case": "accepted nontrivial equivalence class with payload/status congruence preconditions",
        "status_behavior": "status_preserved_or_reduced_or_named_operation_blocker_under_refined_congruence",
        "measure_decrease_status": "support_size_decreases_when_nontrivial_class_exists",
        "payload_refinement_status": "proof_sketch_ready_payload_role_congruence",
        "normal_form_status": NORMAL_STATUS,
        "counterexample_status_status": CONTRACT_SKELETON,
        "contradiction_status": "not_applicable",
        "escape_status": "status_escape_to_named_operation_blocker_or_deferred_higher_support_escape",
        "proof_status": "proof_ready_skeleton_domain_normal_form_open",
        "missing_sublemma": "quotient status-domain invariance;quotient normal-form transfer;status predicate determination;valid reduced-status fallback",
        "next_action": NEXT1,
    },
    {
        "operation_key": "canonical_motif_compression",
        "current_operation_status": "partial_canonical_compression_status_proof_ready_congruence_open",
        "input_case": "accepted compressible motif lowers support size or motif rank",
        "status_behavior": "status_preserved_or_reduced_or_named_operation_blocker",
        "measure_decrease_status": "lexicographic_measure_decreases_under_accepted_compression",
        "payload_refinement_status": "proof_sketch_ready",
        "normal_form_status": "proof_sketch_ready",
        "counterexample_status_status": "proof_ready_skeleton_canonical_compression_status_congruence_open",
        "contradiction_status": "not_applicable",
        "escape_status": "status_escape_to_named_operation_blocker_or_deferred_higher_support_escape",
        "proof_status": "proof_ready_skeleton",
        "missing_sublemma": "canonical-motif counterexample-status congruence;motif normal-form/payload/source-form transfer",
        "next_action": NEXT1,
    },
    {
        "operation_key": "family_chain_absorption_reduction",
        "current_operation_status": "partial_family_chain_absorption_status_proof_ready_refutation_measure_open",
        "input_case": "recognized family-chain absorption trigger with fresh target package",
        "status_behavior": "status_refuted_by_contradiction_or_status_reduced_to_smaller_witness_or_named_blocker",
        "measure_decrease_status": "contradiction closes;residual measure blocked",
        "payload_refinement_status": "proof_sketch_ready",
        "normal_form_status": "proof_sketch_ready",
        "counterexample_status_status": "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open",
        "contradiction_status": "proof_sketch_ready_lifted_target_refutation_source_alignment_open",
        "escape_status": "status_escape_to_named_operation_blocker_or_deferred_higher_support_escape",
        "proof_status": "proof_ready_skeleton",
        "missing_sublemma": "source-target payload/status alignment;residual measure decrease;payload/status transfer",
        "next_action": NEXT3,
    },
    {
        "operation_key": "frontier_capture_reduction",
        "current_operation_status": "proved_under_current_scope",
        "input_case": "reduced support<=8 witness in frontier/shell/tail input language",
        "status_behavior": "status_preserved_then_limited_theorem_applies",
        "measure_decrease_status": "already_decreased_before_capture",
        "payload_refinement_status": "not primary payload operation",
        "normal_form_status": "selected normal form required",
        "counterexample_status_status": "handled_by_limited_bridge_scope",
        "contradiction_status": "not_applicable",
        "escape_status": "not_escape_after_valid_reduction",
        "proof_status": "proved_under_current_scope_after_valid_reduction",
        "missing_sublemma": "none after valid reduction",
        "next_action": "use_after_reduction",
    },
    {
        "operation_key": "tail_obstruction_reduction",
        "current_operation_status": "proved_under_current_scope",
        "input_case": "reduced support<=8 checked tail witness",
        "status_behavior": "status_preserved_then_tail_obstruction_captured",
        "measure_decrease_status": "already_decreased_before_capture",
        "payload_refinement_status": "not primary payload operation",
        "normal_form_status": "checked tail normal form required",
        "counterexample_status_status": "handled_inside_checked_tail_scope",
        "contradiction_status": "not_applicable",
        "escape_status": "not_escape_inside_checked_scope",
        "proof_status": "proved_under_current_scope_after_valid_reduction",
        "missing_sublemma": "none inside checked tail scope",
        "next_action": "use_after_reduction",
    },
    {
        "operation_key": "higher_support_escape_when_irreducible",
        "current_operation_status": HIGHER_SUPPORT,
        "input_case": "support>8 after operation proofs close and no operation/status proof closes the branch",
        "status_behavior": "status_escape_to_higher_support",
        "measure_decrease_status": "none",
        "payload_refinement_status": "not_applicable",
        "normal_form_status": "not_applicable",
        "counterexample_status_status": "not_closed",
        "contradiction_status": "not_applicable",
        "escape_status": "higher_support_escape_candidate_deferred",
        "proof_status": "classified_not_closed",
        "missing_sublemma": "higher-support bound after operation proofs close",
        "next_action": "higher_support_bound_formalization_after_operation_proofs",
    },
]


def write_rollup_docs() -> None:
    write_table(RUNTIME / "support_reduction_operation_status_table_90.tsv", operation_header, operation_rows)
    write_md(
        D90 / "support_reduction_operation_status_table_90.md",
        "Support Reduction Operation Status Table 90",
        f"""
## status after contract-equivalent congruence refinement

`operation_status_table_contract_equivalent_refined_domain_normal_form_open_remaining_canonical_alignment_measure_open`

Coordinate contraction is now refined from a generic equivalent-coordinate
status-congruence blocker into payload congruence, quotient status-domain
transfer, quotient normal-form transfer, status predicate determination, and
valid reduced-status fallback. It is still not a proved operation.

Runtime table:
`branch_4/90/runtime/support_reduction_operation_status_table_90.tsv`.
""",
    )

    write_md(
        D90 / "status_preservation_congruence_skeleton_90.md",
        "Status Preservation Congruence Skeleton 90",
        f"""
## status after contract-equivalent congruence refinement

`{STATUS_CONGRUENCE}`

The common status language remains unchanged. This round refines the
coordinate-contraction row. Project-to-active remains payload-locality proved
and status-domain-open. Canonical compression and family-chain absorption remain
open. No support9+ scan was run.

Runtime skeleton:
`branch_4/90/runtime/status_preservation_congruence_skeleton_90.tsv`.
""",
    )
    write_metric(
        RUNTIME / "status_preservation_congruence_skeleton_90.tsv",
        [
            ("lemma_name", "ready_operation_status_congruence_or_named_escape"),
            ("selected_statement", "operation_status_preservation_congruence_for_ready_operations"),
            ("exact_statement", "For each currently ready or partial-ready support-reduction operation, status outcome is classified as preserved, reduced, refuted, absorbed, named operation blocker, higher-support escape, or not applicable."),
            ("assumption_count", "10"),
            ("conclusion", "Coordinate contraction is refined to domain/normal-form/status-predicate open skeleton; canonical compression, family-chain alignment, and residual measure remain open."),
            ("project_to_active_proof_status", "proof_ready_skeleton_project_to_active_locality_status_domain_open"),
            ("coordinate_contraction_proof_status", CONTRACT_SKELETON),
            ("canonical_compression_proof_status", "proof_ready_skeleton_canonical_compression_status_congruence_open"),
            ("family_chain_absorption_proof_status", "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open"),
            ("higher_support_escape_proof_status", HIGHER_SUPPORT),
            ("status_congruence_obligation_count", "13"),
            ("proved_under_current_scope_sublemma_count", "4"),
            ("proof_sketch_only_sublemma_count", "2"),
            ("blocked_sublemma_count", "1"),
            ("proof_ready_open_sublemma_count", "3"),
            ("missing_steps", "project-to-active status-domain invariance and normal-form transfer;contract-equivalent quotient status-domain/normal-form/status-predicate transfer;canonical-motif counterexample-status congruence;family-chain source-target alignment and residual measure decrease;higher-support bound only after operation proofs close"),
            ("exact_caveat", "Does not prove full status preservation, support-bound completion, support8 sufficiency, higher-support necessity, or the full general theorem; no support9 scan was run."),
            ("final_status", STATUS_CONGRUENCE),
            ("next_blocker", NEXT1),
        ],
    )

    write_md(
        D90 / "support_reduction_step_skeleton_90.md",
        "Support Reduction Step Skeleton 90",
        f"""
## status after contract-equivalent congruence refinement

`{SUPPORT_REDUCTION}`

The support-growth partition is unchanged in shape, but the coordinate
contraction branch now carries explicit status-domain, normal-form,
status-predicate, and reduced-status obligations. Canonical compression is the
next operation-specific congruence target.

Runtime skeleton:
`branch_4/90/runtime/support_reduction_step_skeleton_90.tsv`.
""",
    )
    write_metric(
        RUNTIME / "support_reduction_step_skeleton_90.tsv",
        [
            ("lemma_name", "support_growth_partition"),
            ("exact_statement", "Support>8 normal minimal witnesses are partitioned into ready reduction, project-to-active payload-proved/status-domain-open branch, coordinate-contraction domain/normal-form-open branch, canonical-compression congruence-open branch, family-chain alignment/measure-open branch, downstream support8 capture, named operation blocker, or true higher-support escape after operation proofs close."),
            ("assumption_count", "10"),
            ("conclusion", "Coordinate contraction is refined but not proved; canonical compression, project status-domain, and family-chain alignment/measure remain open."),
            ("case_split", "delete_redundant_support_coordinate_selected;project_to_active_support_payload_proved_status_domain_open;contract_equivalent_support_coordinates_domain_normal_form_open;canonical_motif_compression_status_proof_ready_congruence_open;family_chain_absorption_status_proof_ready_refutation_measure_open;frontier_tail_captured_after_reduction;irreducible_higher_support_escape_after_operation_proofs"),
            ("measure_decrease_status", "delete_redundant_project_to_active_contraction_and_compression_measure_proved_under_preconditions_absorption_residual_measure_open"),
            ("project_to_active_status_status", "proof_ready_skeleton_project_to_active_locality_status_domain_open"),
            ("contract_equivalent_status_status", CONTRACT_SKELETON),
            ("missing_steps", "project-to-active status-domain invariance and normal-form transfer;contract-equivalent quotient domain/normal/status predicate;canonical-motif counterexample-status congruence;family-chain absorption source-target alignment and residual measure;higher-support theoretical bound only after operation proofs close"),
            ("exact_caveat", "no_support9_scan_full_support8_sufficiency_not_proved_contract_equivalent_not_fully_proved_and_full_general_theorem_not_proved"),
            ("final_status", SUPPORT_REDUCTION),
            ("next_blocker", NEXT1),
        ],
    )

    write_md(
        D90 / "support_bound_lemma_skeleton_90.md",
        "Support Bound Lemma Skeleton 90",
        f"""
## status after contract-equivalent congruence refinement

`{SUPPORT_BOUND}`

Support-bound remains proof-ready only as a skeleton. The limited bridge theorem
is still proved under current scope. Coordinate contraction is refined but still
blocked by quotient status-domain/normal-form/status predicate and valid
reduced-status sublemmas.

Runtime skeleton:
`branch_4/90/runtime/support_bound_lemma_skeleton_90.tsv`.
""",
    )
    write_metric(
        RUNTIME / "support_bound_lemma_skeleton_90.tsv",
        [
            ("lemma_name", "support_minimal_counterexample_reduces_to_support8_or_escape"),
            ("exact_statement", "Every normal minimal counterexample either reduces to support<=8 through a valid operation chain or is routed to a named blocker/deferred true higher-support escape after operation proofs close."),
            ("assumption_count", "11"),
            ("conclusion", "Support-bound remains proof-ready; coordinate-contraction congruence is refined but domain/normal/status predicate and reduced-status obligations remain open."),
            ("support8_case", "limited_bridge_theorem_proved_under_current_scope"),
            ("delete_redundant_case", "proved_under_current_scope_under_redundancy_precondition"),
            ("project_to_active_case", "proof_ready_skeleton_project_to_active_locality_status_domain_open"),
            ("coordinate_contraction_case", CONTRACT_SKELETON),
            ("canonical_compression_case", "proof_ready_skeleton_canonical_compression_status_congruence_open"),
            ("family_chain_absorption_case", "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open"),
            ("higher_support_escape", HIGHER_SUPPORT),
            ("support_reduction_step_status", SUPPORT_REDUCTION),
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("measure_status", "delete/project/contraction/compression measure ready under preconditions;absorption residual measure open"),
            ("missing_steps", "status-domain invariance;contract-equivalent quotient congruence;canonical-motif congruence;family-chain source-target alignment;residual absorption measure;higher-support bound after operation proofs close"),
            ("exact_caveat", "no_support_bound_completion_no_support8_sufficiency_no_support9_scan_no_full_general_theorem"),
            ("final_status", SUPPORT_BOUND),
            ("next_blocker", NEXT1),
        ],
    )


def write_general_bridge_docs() -> None:
    action_header = [
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
    action_rows = [
        {
            "action_key": NEXT1,
            "resolves": "canonical_motif_status_congruence",
            "prerequisite_status": "canonical_compression_status_proof_ready_congruence_open",
            "readiness_score_0_100": "82",
            "proof_value_0_100": "84",
            "engineering_cost_0_100": "65",
            "risk_0_100": "66",
            "dependency_clarity_0_100": "87",
            "expected_progress_value_0_100": "82",
            "recommended_order": "1",
            "final_recommendation": NEXT1,
            "reason": "Coordinate contraction is now first-class refined; canonical motif status congruence is the clearest remaining operation-specific congruence blocker.",
        },
        {
            "action_key": NEXT2,
            "resolves": "status_domain_and_normal_form_transfer",
            "prerequisite_status": "project_to_active_payload_proved_status_domain_open",
            "readiness_score_0_100": "80",
            "proof_value_0_100": "86",
            "engineering_cost_0_100": "61",
            "risk_0_100": "62",
            "dependency_clarity_0_100": "82",
            "expected_progress_value_0_100": "81",
            "recommended_order": "2",
            "final_recommendation": NEXT2,
            "reason": "Status-domain and normal-form transfer remain the shared blocker exposed by both project-to-active and coordinate contraction.",
        },
        {
            "action_key": NEXT3,
            "resolves": "source_target_payload_status_alignment",
            "prerequisite_status": "family_chain_absorption_status_refutation_measure_open",
            "readiness_score_0_100": "75",
            "proof_value_0_100": "83",
            "engineering_cost_0_100": "68",
            "risk_0_100": "72",
            "dependency_clarity_0_100": "82",
            "expected_progress_value_0_100": "77",
            "recommended_order": "3",
            "final_recommendation": NEXT3,
            "reason": "Absorption source-target alignment is mandatory before residual absorption measure can be closed.",
        },
        {
            "action_key": "residual_absorption_measure_decrease",
            "resolves": "residual_absorption_measure",
            "prerequisite_status": "family_chain_absorption_source_alignment_open",
            "readiness_score_0_100": "70",
            "proof_value_0_100": "82",
            "engineering_cost_0_100": "68",
            "risk_0_100": "73",
            "dependency_clarity_0_100": "78",
            "expected_progress_value_0_100": "74",
            "recommended_order": "4",
            "final_recommendation": "residual_absorption_measure_decrease",
            "reason": "Measure decrease remains mandatory but source alignment should be clarified first.",
        },
        {
            "action_key": "higher_support_necessity_recheck",
            "resolves": "recheck_after_operation_refinements",
            "prerequisite_status": HIGHER_SUPPORT,
            "readiness_score_0_100": "66",
            "proof_value_0_100": "75",
            "engineering_cost_0_100": "55",
            "risk_0_100": "64",
            "dependency_clarity_0_100": "80",
            "expected_progress_value_0_100": "70",
            "recommended_order": "5",
            "final_recommendation": "higher_support_necessity_recheck",
            "reason": "Higher-support remains deferred until operation-local proof obligations close or fail as named true escapes.",
        },
        {
            "action_key": "limited_to_broader_generalization_plan",
            "resolves": "generalization_contract",
            "prerequisite_status": "operation_proofs_open_contract_equivalent_refined",
            "readiness_score_0_100": "72",
            "proof_value_0_100": "80",
            "engineering_cost_0_100": "55",
            "risk_0_100": "61",
            "dependency_clarity_0_100": "84",
            "expected_progress_value_0_100": "74",
            "recommended_order": "6",
            "final_recommendation": "limited_to_broader_generalization_plan",
            "reason": "Planning can proceed, but proof completion still depends on operation-local congruence, alignment, and residual measure blockers.",
        },
        {
            "action_key": "support_bound_completion",
            "resolves": "support_bound_completion",
            "prerequisite_status": SUPPORT_BOUND,
            "readiness_score_0_100": "63",
            "proof_value_0_100": "83",
            "engineering_cost_0_100": "64",
            "risk_0_100": "72",
            "dependency_clarity_0_100": "80",
            "expected_progress_value_0_100": "70",
            "recommended_order": "7",
            "final_recommendation": "support_bound_completion",
            "reason": "Premature until operation-local congruence, source-alignment, and residual absorption measure blockers close.",
        },
    ]
    write_table(RUNTIME / "general_gap_bridge_next_action_matrix_90.tsv", action_header, action_rows)
    write_md(
        D90 / "general_gap_bridge_next_action_matrix_90.md",
        "General Gap Bridge Next Action Matrix 90",
        f"""
## status after contract-equivalent congruence refinement

Top recommendation: `{NEXT1}`.

Coordinate contraction is refined to a proof-ready domain/normal-form/status
predicate skeleton. The full contraction proof remains open, but the previous
generic equivalent-coordinate blocker is now first-class. The next highest
operation-specific congruence blocker is canonical motif compression.

Recommended order:

1. `{NEXT1}`
2. `{NEXT2}`
3. `{NEXT3}`

No support9+ scan was run.

Runtime matrix:
`branch_4/90/runtime/general_gap_bridge_next_action_matrix_90.tsv`.
""",
    )

    write_metric(
        RUNTIME / "general_gap_bridge_readiness_fingerprint_90.tsv",
        [
            ("target_statement_count", "3"),
            ("bridge_obligation_count", "10"),
            ("satisfied_current_verified_obligation_count", "1"),
            ("partially_satisfied_obligation_count", "4"),
            ("project_to_active_status", "proof_ready_skeleton_project_to_active_locality_status_domain_open"),
            ("coordinate_contraction_status", CONTRACT_SKELETON),
            ("canonical_compression_status", "proof_ready_skeleton_canonical_compression_status_congruence_open"),
            ("family_chain_absorption_status", "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open"),
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("support_bound_lemma_status", SUPPORT_BOUND),
            ("support_reduction_step_status", SUPPORT_REDUCTION),
            ("higher_support_necessity_status", HIGHER_SUPPORT),
            ("general_theorem_readiness_label", GENERAL_READY),
            ("next_action_first", NEXT1),
            ("next_action_second", NEXT2),
            ("next_action_third", NEXT3),
            ("fingerprint", f"3|10|1|4|contract_equivalent_refined_domain_normal_form_open|{NEXT1}"),
        ],
    )

    write_md(
        D90 / "general_gap_bridge_obligation_inventory_90.md",
        "General Gap Bridge Obligation Inventory 90",
        f"""
## status after contract-equivalent congruence refinement

The limited bridge theorem remains proved under current scope. The broader
support-bound obligation is now `{SUPPORT_BOUND}`.

Coordinate contraction has been refined, not proved. Remaining broader blockers
are canonical-motif status congruence, project/status-domain transfer,
family-chain source-target alignment, residual absorption measure, and
higher-support only after operation proofs close.

Current next target: `{NEXT1}`.

Runtime inventory:
`branch_4/90/runtime/general_gap_bridge_obligation_inventory_90.tsv`.
""",
    )
    write_table(
        RUNTIME / "general_gap_bridge_obligation_inventory_90.tsv",
        [
            "obligation_key",
            "description",
            "required_for_limited_support8_statement",
            "required_for_bounded_shell_statement",
            "required_for_full_general_statement",
            "existing_evidence",
            "missing_evidence",
            "current_status",
            "dependency_on_shell16",
            "dependency_on_higher_support",
            "dependency_on_BOJ_bridge",
            "proof_type_needed",
            "estimated_difficulty",
            "recommended_next_action",
        ],
        [
            {
                "obligation_key": "minimal_counterexample_reduction",
                "description": "Show that a broader counterexample reduces to a bounded checked form or named extension.",
                "required_for_limited_support8_statement": "1",
                "required_for_bounded_shell_statement": "1",
                "required_for_full_general_statement": "1",
                "existing_evidence": "support8 closure;limited theorem proof;support-growth partition",
                "missing_evidence": "support-bound completion for broader scope",
                "current_status": "closed_for_limited_bridge_theorem",
                "dependency_on_shell16": "reviewed",
                "dependency_on_higher_support": "possible",
                "dependency_on_BOJ_bridge": "no",
                "proof_type_needed": "structural_reduction",
                "estimated_difficulty": "medium",
                "recommended_next_action": "after_support_bound",
            },
            {
                "obligation_key": "support_bound_justification",
                "description": "Justify support8 sufficiency or identify exact true higher-support escape after operation proofs close.",
                "required_for_limited_support8_statement": "0",
                "required_for_bounded_shell_statement": "0",
                "required_for_full_general_statement": "1",
                "existing_evidence": "support8 closure;limited bridge theorem;project-to-active locality skeleton;contract-equivalent refined skeleton;canonical-compression status skeleton;family-chain absorption status skeleton;support-bound skeleton;status-congruence bridge",
                "missing_evidence": "status-domain invariance;contract-equivalent valid reduced-status theorem;canonical-motif status congruence;family-chain source-target alignment;residual absorption measure",
                "current_status": SUPPORT_BOUND,
                "dependency_on_shell16": "no",
                "dependency_on_higher_support": "deferred",
                "dependency_on_BOJ_bridge": "no",
                "proof_type_needed": "bound_theorem",
                "estimated_difficulty": "high",
                "recommended_next_action": NEXT1,
            },
            {
                "obligation_key": "canonicalization_soundness",
                "description": "Show runtime canonical row equality matches mathematical equivalence.",
                "required_for_limited_support8_statement": "1",
                "required_for_bounded_shell_statement": "1",
                "required_for_full_general_statement": "1",
                "existing_evidence": "canonical runtime equality;payload semantics;canonical compression status skeleton;status bridge",
                "missing_evidence": "canonical-motif counterexample-status congruence and canonical payload mathematical equivalence",
                "current_status": "partially_satisfied",
                "dependency_on_shell16": "no",
                "dependency_on_higher_support": "no",
                "dependency_on_BOJ_bridge": "no",
                "proof_type_needed": "soundness_lemma",
                "estimated_difficulty": "medium",
                "recommended_next_action": NEXT1,
            },
            {
                "obligation_key": "finite_exhaustion_to_structural_lemma",
                "description": "Interpret finite scan results as a structural theorem under declared assumptions.",
                "required_for_limited_support8_statement": "1",
                "required_for_bounded_shell_statement": "1",
                "required_for_full_general_statement": "1",
                "existing_evidence": "finite scans;support8 lock;limited bridge theorem proof;support-growth partition",
                "missing_evidence": "full structural/general interpretation lemma",
                "current_status": "satisfied_for_limited_bridge_theorem",
                "dependency_on_shell16": "possible",
                "dependency_on_higher_support": "deferred",
                "dependency_on_BOJ_bridge": "no",
                "proof_type_needed": "structural_interpretation",
                "estimated_difficulty": "medium",
                "recommended_next_action": "after_support_bound",
            },
        ],
    )

    write_md(
        D90 / "general_gap_bridge_dependency_graph_90.md",
        "General Gap Bridge Dependency Graph 90",
        f"""
## status after contract-equivalent congruence refinement

The coordinate-contraction dependency now points to
`{CONTRACT_SKELETON}`. It is a sharper proof-ready node, not a completed status
proof. Canonical compression is the next operation-specific blocker.

Runtime graph:
`branch_4/90/runtime/general_gap_bridge_dependency_graph_90.tsv`.
""",
    )
    write_table(
        RUNTIME / "general_gap_bridge_dependency_graph_90.tsv",
        ["source_node", "target_node", "edge_type", "evidence_path", "current_status", "caveat"],
        [
            {
                "source_node": "support8_authoritative_completion_lock",
                "target_node": "minimal_counterexample_reduction",
                "edge_type": "verified_input_supports_obligation",
                "evidence_path": "branch_4/90/runtime/current_support8_closure_certificate_90.tsv",
                "current_status": "support8_authoritative_completion_locked",
                "caveat": "support8_specific",
            },
            {
                "source_node": "contract_equivalent_congruence_refinement",
                "target_node": "support_bound_justification",
                "edge_type": "verified_input_supports_obligation",
                "evidence_path": "branch_4/90/runtime/contract_equivalent_status_congruence_refinement_skeleton_90.tsv",
                "current_status": CONTRACT_SKELETON,
                "caveat": "domain_normal_form_status_predicate_open",
            },
            {
                "source_node": "canonical_compression_status_skeleton",
                "target_node": "support_bound_justification",
                "edge_type": "next_operation_blocker",
                "evidence_path": "branch_4/90/runtime/canonical_compression_status_skeleton_90.tsv",
                "current_status": "proof_ready_skeleton_canonical_compression_status_congruence_open",
                "caveat": "not_status_proved",
            },
            {
                "source_node": "operation_specific_status_proofs",
                "target_node": "support_bound_justification",
                "edge_type": "obligation_depends_on_obligation",
                "evidence_path": "branch_4/90/runtime/support_reduction_operation_status_table_90.tsv",
                "current_status": "remaining_operation_refinements_open",
                "caveat": f"{NEXT1}_next",
            },
            {
                "source_node": "higher_support_bound_formalization",
                "target_node": "support_bound_justification",
                "edge_type": "deferred_candidate_bound",
                "evidence_path": "branch_4/90/runtime/higher_support_bound_candidate_lemmas_90.tsv",
                "current_status": HIGHER_SUPPORT,
                "caveat": "not_scan",
            },
        ],
    )

    write_md(
        D90 / "general_gap_bridge_lemma_candidates_90.md",
        "General Gap Bridge Lemma Candidates 90",
        f"""
## status after contract-equivalent congruence refinement

The coordinate-contraction candidate is now refined to
`{CONTRACT_SKELETON}`. The next best lemma candidate is `{NEXT1}`.

Runtime candidates:
`branch_4/90/runtime/general_gap_bridge_lemma_candidates_90.tsv`.
""",
    )
    write_table(
        RUNTIME / "general_gap_bridge_lemma_candidates_90.tsv",
        [
            "lemma_key",
            "informal_name",
            "formal_candidate_statement",
            "assumptions",
            "conclusion",
            "existing_verified_inputs_used",
            "missing_sublemmas",
            "expected_proof_method",
            "dependency_on_shell16",
            "dependency_on_higher_support",
            "dependency_on_constructivity",
            "current_status",
            "risk",
            "recommended_next_action",
        ],
        [
            {
                "lemma_key": "coordinate_contraction_status_candidate",
                "informal_name": "Coordinate contraction status congruence",
                "formal_candidate_statement": "Equivalent-coordinate quotient preserves/reduces counterexample status or names blocker/escape under refined congruence.",
                "assumptions": "accepted equivalence relation and quotient preconditions",
                "conclusion": "status branch classified with domain/normal-form/status-predicate obligations",
                "existing_verified_inputs_used": "contract_equivalent_status_congruence_refinement_skeleton;contract_equivalent_status_domain_transfer;contract_equivalent_normal_form_transfer;equivalent_coordinate_congruence_refinement",
                "missing_sublemmas": "status-domain invariance;normal-form transfer;status predicate determination;valid reduced-status fallback",
                "expected_proof_method": "operation proof",
                "dependency_on_shell16": "no",
                "dependency_on_higher_support": "deferred",
                "dependency_on_constructivity": "no",
                "current_status": CONTRACT_SKELETON,
                "risk": "medium",
                "recommended_next_action": NEXT1,
            },
            {
                "lemma_key": "canonical_compression_status_candidate",
                "informal_name": "Canonical compression status preservation",
                "formal_candidate_statement": "Canonical motif compression preserves/reduces counterexample status or names blocker/escape.",
                "assumptions": "compressible motif and lexicographic decrease",
                "conclusion": "status branch classified with proof-ready skeleton",
                "existing_verified_inputs_used": "canonical_compression_status_skeleton;canonical_motif_status_congruence_lemma;support_bound_measure",
                "missing_sublemmas": "canonical-motif counterexample-status congruence;normal-form/payload/source-form transfer",
                "expected_proof_method": "operation proof",
                "dependency_on_shell16": "no",
                "dependency_on_higher_support": "deferred",
                "dependency_on_constructivity": "no",
                "current_status": "proof_ready_skeleton_canonical_compression_status_congruence_open",
                "risk": "medium",
                "recommended_next_action": NEXT1,
            },
            {
                "lemma_key": "project_to_active_status_domain_candidate",
                "informal_name": "Project-to-active status-domain transfer",
                "formal_candidate_statement": "Active projection preserves status domain/normal form, reduces to valid smaller status, or names blocker/escape.",
                "assumptions": "active support contains payload/status dependencies",
                "conclusion": "status-domain and normal-form transfer becomes first-class",
                "existing_verified_inputs_used": "project_to_active_status_locality_skeleton;inactive_support_payload_locality",
                "missing_sublemmas": "status-domain invariance;normal-form/source-form transfer;valid reduced-status fallback",
                "expected_proof_method": "operation proof",
                "dependency_on_shell16": "no",
                "dependency_on_higher_support": "deferred",
                "dependency_on_constructivity": "no",
                "current_status": "proof_ready_skeleton_project_to_active_locality_status_domain_open",
                "risk": "medium",
                "recommended_next_action": NEXT2,
            },
        ],
    )

    write_md(
        D90 / "limited_general_gap_bridge_skeleton_90.md",
        "Limited General Gap Bridge Skeleton 90",
        f"""
## status after contract-equivalent congruence refinement

The limited bridge theorem status is unchanged:
`limited_bridge_theorem_proved_under_current_scope`.

Broader support-bound readiness is `{SUPPORT_BOUND}`. Coordinate contraction is
refined, canonical compression remains open, and no full general theorem is
proved.

Runtime skeleton:
`branch_4/90/runtime/limited_general_gap_bridge_skeleton_90.tsv`.
""",
    )
    write_metric(
        RUNTIME / "limited_general_gap_bridge_skeleton_90.tsv",
        [
            ("limited_bridge_theorem_status", "limited_bridge_theorem_proved_under_current_scope"),
            ("support_bound_skeleton_status", SUPPORT_BOUND),
            ("support_reduction_skeleton_status", SUPPORT_REDUCTION),
            ("project_to_active_status", "proof_ready_skeleton_project_to_active_locality_status_domain_open"),
            ("coordinate_contraction_status", CONTRACT_SKELETON),
            ("canonical_compression_status", "proof_ready_skeleton_canonical_compression_status_congruence_open"),
            ("family_chain_absorption_status", "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open"),
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("higher_support_status", HIGHER_SUPPORT),
            ("readiness_label", GENERAL_READY),
            ("next_action_first", NEXT1),
            ("next_action_second", NEXT2),
            ("next_action_third", NEXT3),
        ],
    )

    write_md(
        D90 / "general_gap_theorem_readiness_audit_90.md",
        "General Gap Theorem Readiness Audit 90",
        f"""
## status after contract-equivalent congruence refinement

`{GENERAL_READY}`.

The full general theorem is not proved. The limited bridge theorem remains
proved under current scope. Broader readiness is blocked by canonical-motif
status congruence, status-domain/normal-form transfer, family-chain
source-target alignment, residual absorption measure, and true higher-support
only after operation proofs close.
""",
    )

    write_md(
        D90 / "higher_support_necessity_after_contract_equivalent_congruence_refinement_90.md",
        "Higher Support Necessity After Contract Equivalent Congruence Refinement 90",
        f"""
## status

`{HIGHER_SUPPORT}`

Coordinate-congruence refinement reduces the contraction blocker to
status-domain transfer, quotient normal-form transfer, status predicate
determination, and valid reduced-status fallback. This does not establish that
support9+ theorem-data is currently necessary. Higher-support remains deferred
because operation-specific congruence/alignment/measure obligations are still
open.

No support9+ scan was run.

Runtime table:
`branch_4/90/runtime/higher_support_necessity_after_contract_equivalent_congruence_refinement_90.tsv`.
""",
    )
    write_table(
        RUNTIME / "higher_support_necessity_after_contract_equivalent_congruence_refinement_90.tsv",
        ["item", "status", "evidence", "next_action"],
        [
            {
                "item": "equivalent_coordinate_status_congruence_closed",
                "status": "no",
                "evidence": "Refinement is proof-ready but status-domain, normal-form, and status predicate determination remain open.",
                "next_action": NEXT1,
            },
            {
                "item": "contraction_support_gt8_escape_reduced",
                "status": "partially",
                "evidence": "Failure is separated into payload congruence, domain transfer, normal-form transfer, reduced-status fallback, and named escape cases.",
                "next_action": NEXT1,
            },
            {
                "item": "higher_support_needed_now",
                "status": "no",
                "evidence": "No true irreducible higher-support escape is established while operation-specific proof obligations remain open.",
                "next_action": "higher_support_necessity_recheck_after_operation_refinements",
            },
            {
                "item": "final_label",
                "status": HIGHER_SUPPORT,
                "evidence": "No support9 scan was run; operation proof blockers still dominate.",
                "next_action": NEXT1,
            },
        ],
    )


def write_current_certificate_and_reports() -> None:
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
            ("limited_bridge_theorem_status", "limited_bridge_theorem_proved_under_current_scope"),
            ("bridge_next_exact_target", NEXT1),
            ("support_bound_lemma_status", SUPPORT_BOUND),
            ("support_reduction_step_status", SUPPORT_REDUCTION),
            ("contract_equivalent_operation_status", CONTRACT_STATUS),
            ("status_congruence_bridge_status", STATUS_CONGRUENCE),
            ("higher_support_necessity_status", HIGHER_SUPPORT),
            ("general_theorem_readiness", GENERAL_READY),
            ("support_bound_next_exact_target", NEXT1),
            ("caveat", "readiness_boundary_not_general_theorem_or_boj_solver"),
        ],
    )
    write_md(
        D90 / "current_support8_closure_certificate_90.md",
        "Current Support8 Closure Certificate 90",
        f"""
## verified lock baseline

- support8 lock: `support8_authoritative_completion_locked`
- pass1/pass2/pass3: `support8_authoritative_completion_locked`
- required docs/artifacts: `39/39`, `8/8`
- top-level provenance: fresh `16`, imported `0`, mixed `0`, archival `3`
- family-chain lower layers: total `7`, fresh `7`, imported `0`, caveat closed `1`
- limited bridge theorem: `limited_bridge_theorem_proved_under_current_scope`

## proof-boundary status after contract-equivalent congruence refinement

- contract-equivalent operation status: `{CONTRACT_STATUS}`
- status-domain transfer: `{DOMAIN_STATUS}`
- normal-form transfer: `{NORMAL_STATUS}`
- equivalent-coordinate congruence refinement: `{CONGRUENCE_STATUS}`
- status-congruence bridge: `{STATUS_CONGRUENCE}`
- support reduction skeleton: `{SUPPORT_REDUCTION}`
- support-bound skeleton: `{SUPPORT_BOUND}`
- higher-support necessity: `{HIGHER_SUPPORT}`
- general theorem readiness: `{GENERAL_READY}`
- next exact target: `{NEXT1}`

This certificate does not prove the full general theorem, full coordinate
contraction, or support8 sufficiency.
""",
    )
    write_md(
        D90 / "proof_system_contract_memo_90.md",
        "Proof System Contract Memo 90",
        f"""
## verified baseline

- release compile: verified
- LOCAL_TEST compile: verified
- pass1/pass2/pass3: `support8_authoritative_completion_locked`
- required docs/artifacts: `39/39`, `8/8`

## current proof contract

This round is `contract_equivalent_status_congruence_refinement`. It refines
coordinate contraction status congruence into payload, status-domain,
normal-form, counterexample-status predicate, smaller-witness, and named escape
branches.

It does not prove the full general theorem, does not prove
`contract_equivalent_support_coordinates` fully, and does not run support9+.

Next exact target: `{NEXT1}`.
""",
    )
    write_md(
        D90 / "proof_system_reproduction_report_90.md",
        "Proof System Reproduction Report 90",
        f"""
1. compile status
- release compile: verified
- LOCAL_TEST compile: verified

2. pass status
- pass1: `support8_authoritative_completion_locked`
- pass2: `support8_authoritative_completion_locked`
- pass3: `support8_authoritative_completion_locked`

3. proof status after this round
- selected coordinate-congruence statement: `{SELECTED}`
- status-domain transfer: `{DOMAIN_STATUS}`
- normal-form transfer: `{NORMAL_STATUS}`
- equivalent-coordinate congruence refinement: `{CONGRUENCE_STATUS}`
- coordinate-congruence refinement skeleton: `{CONTRACT_SKELETON}`
- contract-equivalent operation status: `{CONTRACT_STATUS}`
- status-congruence skeleton: `{STATUS_CONGRUENCE}`
- support reduction skeleton: `{SUPPORT_REDUCTION}`
- support-bound skeleton: `{SUPPORT_BOUND}`
- higher-support necessity: `{HIGHER_SUPPORT}`
- general theorem readiness: `{GENERAL_READY}`
- next target: `{NEXT1}`

4. non-claims
- no full general theorem proof
- no full coordinate-contraction proof
- no support9+ scan
- no BOJ solver implementation
""",
    )


def write_additional_sync_docs() -> None:
    write_md(
        D90 / "support_reduction_operations_90.md",
        "Support Reduction Operations 90",
        f"""
## status after contract-equivalent congruence refinement

This table remains an operation inventory, not a full support-reduction theorem.
Current operation status:

- `delete_redundant_support_coordinate`: selected redundancy case is
  current-scope proved under its precondition.
- `project_to_active_support`: payload locality proved; status branch is
  `proof_ready_skeleton_project_to_active_locality_status_domain_open`.
- `contract_equivalent_support_coordinates`: support-size decrease under
  nontrivial accepted equivalence remains proved, while status branch is now
  `{CONTRACT_SKELETON}` with quotient status-domain, quotient normal-form,
  status predicate, and valid reduced-status obligations open.
- `canonical_motif_compression`: lexicographic decrease proof-ready; status
  branch remains `proof_ready_skeleton_canonical_compression_status_congruence_open`.
- `family_chain_absorption_reduction`: refutation/reduction/escape skeleton is
  proof-ready with source-target alignment and residual measure open.

No support9+ scan was run.

Runtime table:
`branch_4/90/runtime/support_reduction_operations_90.tsv`.
""",
    )
    write_table(
        RUNTIME / "support_reduction_operations_90.tsv",
        [
            "operation_key",
            "operation_summary",
            "precondition",
            "output_candidate",
            "measure_delta",
            "status_preservation_status",
            "measure_decrease_available",
            "evidence_path",
            "missing_sublemma",
            "current_status",
            "risk",
        ],
        [
            {
                "operation_key": "delete_redundant_support_coordinate",
                "operation_summary": "Delete a coordinate not used by normalized obstruction payload or counterexample certificate.",
                "precondition": "redundant_support criterion holds under selected operation semantics",
                "output_candidate": "normal witness with one fewer coordinate",
                "measure_delta": "-1",
                "status_preservation_status": "proved_under_current_scope_under_redundancy",
                "measure_decrease_available": "yes for selected operation arithmetic",
                "evidence_path": "support_reduction_selected_operation_skeleton_90.tsv",
                "missing_sublemma": "normal-form and payload-transfer sublemmas for arbitrary schemas",
                "current_status": "proof_ready_skeleton_selected_operation",
                "risk": "medium",
            },
            {
                "operation_key": "project_to_active_support",
                "operation_summary": "Restrict witness to active support and normalize.",
                "precondition": "active support is strict subset and carries payload/status fields",
                "output_candidate": "active-support witness candidate",
                "measure_delta": "negative when active subset is strict",
                "status_preservation_status": "proof_ready_skeleton_project_to_active_locality_status_domain_open",
                "measure_decrease_available": "yes under strict active subset",
                "evidence_path": "project_to_active_status_locality_skeleton_90.tsv",
                "missing_sublemma": "status-domain invariance;normal-form transfer;valid reduced-status fallback",
                "current_status": "partial_project_to_active_locality_proof_ready_status_domain_open",
                "risk": "high",
            },
            {
                "operation_key": "contract_equivalent_support_coordinates",
                "operation_summary": "Identify equivalent support coordinates and quotient each nontrivial class to one coordinate.",
                "precondition": "accepted equivalence relation sound;nontrivial class exists;payload/domain/normal/status congruence preconditions hold",
                "output_candidate": "normal witness with contracted coordinate set",
                "measure_delta": "negative when a nontrivial class exists",
                "status_preservation_status": CONTRACT_SKELETON,
                "measure_decrease_available": "yes for support_size under nontrivial quotient; status preservation refined but not closed",
                "evidence_path": "contract_equivalent_status_congruence_refinement_skeleton_90.tsv;contract_equivalent_status_domain_transfer_90.tsv;contract_equivalent_normal_form_transfer_90.tsv",
                "missing_sublemma": "quotient status-domain invariance;quotient normal-form transfer;status predicate determination;valid reduced-status theorem",
                "current_status": CONTRACT_STATUS,
                "risk": "high",
            },
            {
                "operation_key": "canonical_motif_compression",
                "operation_summary": "Replace witness by lower-rank canonical motif with no larger support.",
                "precondition": "accepted compressible canonical motif exists and semantic preservation obligations are named",
                "output_candidate": "compressed canonical witness candidate",
                "measure_delta": "0 or negative",
                "status_preservation_status": "proof_ready_skeleton_canonical_compression_status_congruence_open",
                "measure_decrease_available": "yes for lexicographic measure under accepted compression; status preservation proof-ready but not closed",
                "evidence_path": "canonical_compression_status_skeleton_90.tsv;canonical_motif_compression_skeleton_90.tsv;support_bound_measure_90.tsv",
                "missing_sublemma": "canonical-motif counterexample-status congruence;normal-form/payload/canonical/source-form rewrite proof",
                "current_status": "partial_canonical_compression_status_proof_ready_congruence_open",
                "risk": "high",
            },
            {
                "operation_key": "family_chain_absorption_reduction",
                "operation_summary": "Use bounded family-chain theorem/refutation or residual absorption branch.",
                "precondition": "recognized family-chain source form and absorption trigger",
                "output_candidate": "refuted branch or residual reduced witness candidate",
                "measure_delta": "blocked for residual absorption",
                "status_preservation_status": "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open",
                "measure_decrease_available": "refutation branch yes; residual measure open",
                "evidence_path": "family_chain_absorption_status_skeleton_90.tsv",
                "missing_sublemma": "source-target payload/status alignment;residual measure decrease",
                "current_status": "partial_family_chain_absorption_status_proof_ready_refutation_measure_open",
                "risk": "high",
            },
        ],
    )

    write_md(
        D90 / "higher_support_escape_interface_90.md",
        "Higher Support Escape Interface 90",
        f"""
## status after contract-equivalent congruence refinement

`{HIGHER_SUPPORT}`

The escape interface names unresolved operation-proof failures without treating
them as true higher-support theorem-data needs. Coordinate contraction is now a
domain/normal-form/status-predicate-open proof-ready skeleton, so higher-support
continues to defer until canonical compression, project/status-domain transfer,
family-chain alignment, and residual measure obligations are resolved.

No support9+ scan was run.

Runtime table:
`branch_4/90/runtime/higher_support_escape_interface_90.tsv`.
""",
    )
    write_table(
        RUNTIME / "higher_support_escape_interface_90.tsv",
        ["escape_key", "description", "classification", "closure_condition", "evidence_path", "next_action"],
        [
            {
                "escape_key": "project_to_active_status_domain_open",
                "description": "payload locality proved but status-domain invariance or normal-form transfer missing",
                "classification": "operation_proof_open_complete_before_higher_support",
                "closure_condition": "complete status-domain locality or reduced-status fallback",
                "evidence_path": "branch_4/90/runtime/project_to_active_status_locality_skeleton_90.tsv",
                "next_action": NEXT2,
            },
            {
                "escape_key": "coordinate_contraction_domain_normal_form_open",
                "description": "equivalent-coordinate congruence refined but quotient domain, normal form, status predicate, and reduced-status validity remain open",
                "classification": "operation_proof_open_complete_before_higher_support",
                "closure_condition": "complete quotient transfer or name valid reduced-status blocker",
                "evidence_path": "branch_4/90/runtime/contract_equivalent_status_congruence_refinement_skeleton_90.tsv",
                "next_action": NEXT1,
            },
            {
                "escape_key": "canonical_compression_congruence_open",
                "description": "canonical motif status congruence remains open",
                "classification": "operation_proof_open_complete_before_higher_support",
                "closure_condition": "complete motif status congruence or named blocker",
                "evidence_path": "branch_4/90/runtime/canonical_compression_status_skeleton_90.tsv",
                "next_action": NEXT1,
            },
            {
                "escape_key": "family_chain_alignment_measure_open",
                "description": "family-chain source-target alignment and residual measure remain open",
                "classification": "operation_proof_open_complete_before_higher_support",
                "closure_condition": "complete source alignment and residual measure or named blocker",
                "evidence_path": "branch_4/90/runtime/family_chain_absorption_status_skeleton_90.tsv",
                "next_action": NEXT3,
            },
            {
                "escape_key": "final_label",
                "description": "no support9 scan;operation-specific blockers dominate",
                "classification": HIGHER_SUPPORT,
                "closure_condition": "operation proofs first",
                "evidence_path": "branch_4/90/runtime/higher_support_necessity_after_contract_equivalent_congruence_refinement_90.tsv",
                "next_action": NEXT1,
            },
        ],
    )

    write_md(
        D90 / "operation_open_vs_higher_support_required_90.md",
        "Operation Open Vs Higher Support Required 90",
        f"""
## status after contract-equivalent congruence refinement

`operation_open_dominates_higher_support_deferred_contract_equivalent_refined`

Coordinate contraction is no longer an opaque congruence blocker; it is a
domain/normal-form/status-predicate-open proof-ready skeleton. This remains an
operation-proof blocker, not a reason to run support9+.

Runtime table:
`branch_4/90/runtime/operation_open_vs_higher_support_required_90.tsv`.
""",
    )
    write_table(
        RUNTIME / "operation_open_vs_higher_support_required_90.tsv",
        [
            "open_item",
            "operation_status",
            "local_operation_proof_open",
            "higher_support_required_now",
            "measure_decrease_available",
            "status_transfer_open",
            "true_irreducible_escape_established",
            "support9_scan_required",
            "next_action",
            "reason",
        ],
        [
            {
                "open_item": "active_projection_status_preservation",
                "operation_status": "operation_proof_ready_status_domain_open",
                "local_operation_proof_open": "1",
                "higher_support_required_now": "0",
                "measure_decrease_available": "1",
                "status_transfer_open": "1",
                "true_irreducible_escape_established": "0",
                "support9_scan_required": "0",
                "next_action": NEXT2,
                "reason": "Payload locality is proved and measure decrease is available; remaining status-domain/normal-form transfer is local.",
            },
            {
                "open_item": "coordinate_contraction_status_congruence",
                "operation_status": CONTRACT_SKELETON,
                "local_operation_proof_open": "1",
                "higher_support_required_now": "0",
                "measure_decrease_available": "1",
                "status_transfer_open": "1",
                "true_irreducible_escape_established": "0",
                "support9_scan_required": "0",
                "next_action": NEXT1,
                "reason": "Nontrivial quotient decreases support; refined status transfer remains an operation proof obligation.",
            },
            {
                "open_item": "canonical_compression_status_congruence",
                "operation_status": "operation_proof_ready_congruence_open",
                "local_operation_proof_open": "1",
                "higher_support_required_now": "0",
                "measure_decrease_available": "1",
                "status_transfer_open": "1",
                "true_irreducible_escape_established": "0",
                "support9_scan_required": "0",
                "next_action": NEXT1,
                "reason": "Canonical motif congruence is the next direct operation proof target.",
            },
        ],
    )

    write_metric(RUNTIME / "general_gap_theorem_readiness_audit_90.tsv", [
        ("support_bound_lemma_status", SUPPORT_BOUND),
        ("support_reduction_step_status", SUPPORT_REDUCTION),
        ("status_congruence_status", STATUS_CONGRUENCE),
        ("coordinate_contraction_status", CONTRACT_SKELETON),
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("readiness_label", GENERAL_READY),
        ("next_action_first", NEXT1),
        ("next_action_second", NEXT2),
        ("next_action_third", NEXT3),
        ("exact_caveat", "full_general_theorem_not_proved_contract_equivalent_operation_not_fully_proved_no_support9_scan"),
    ])

    write_metric(RUNTIME / "contract_equivalent_status_skeleton_fingerprint_90.tsv", [
        ("skeleton_status", CONTRACT_SKELETON),
        ("domain_transfer", DOMAIN_STATUS),
        ("normal_form_transfer", NORMAL_STATUS),
        ("congruence_refinement", CONGRUENCE_STATUS),
        ("fingerprint", f"{CONTRACT_SKELETON}|{DOMAIN_STATUS}|{NORMAL_STATUS}|{CONGRUENCE_STATUS}"),
    ])
    write_metric(RUNTIME / "contract_equivalent_support_coordinates_skeleton_fingerprint_90.tsv", [
        ("skeleton_status", CONTRACT_STATUS),
        ("counterexample_status", CONTRACT_SKELETON),
        ("next_action", NEXT1),
        ("fingerprint", f"{CONTRACT_STATUS}|{CONTRACT_SKELETON}|{NEXT1}"),
    ])
    write_metric(RUNTIME / "contract_equivalent_support_coordinates_fingerprint_90.tsv", [
        ("selected_operation", "contract_equivalent_support_coordinates"),
        ("status_skeleton_status", CONTRACT_SKELETON),
        ("final_status", CONTRACT_STATUS),
        ("next_action", NEXT1),
        ("fingerprint", f"contract_equivalent_support_coordinates|domain_normal_form_open|{CONTRACT_STATUS}"),
    ])
    write_metric(RUNTIME / "support_reduction_operation_status_table_fingerprint_90.tsv", [
        ("operation_status_table", "operation_status_table_contract_equivalent_refined_domain_normal_form_open_remaining_canonical_alignment_measure_open"),
        ("contract_equivalent_status", CONTRACT_STATUS),
        ("next_blocker", NEXT1),
        ("fingerprint", f"{CONTRACT_STATUS}|{STATUS_CONGRUENCE}|{NEXT1}"),
    ])


def refresh_root_doc(path: Path) -> None:
    if not path.exists():
        return
    text = path.read_text(encoding="utf-8")
    replacements = {
        "ready_for_contract_equivalent_status_congruence_refinement": GENERAL_READY,
        "higher_support_deferred_after_project_to_active_locality_status_domain_open": HIGHER_SUPPORT,
        "partial_status_congruence_project_locality_proof_ready_remaining_congruence_alignment_measure_open": STATUS_CONGRUENCE,
        "partition_ready_project_locality_proof_ready_remaining_congruence_alignment_measure_open": SUPPORT_REDUCTION,
        "proof_ready_skeleton_project_locality_proof_ready_remaining_congruence_alignment_measure_open": SUPPORT_BOUND,
        "partial_contract_equivalent_status_proof_ready_congruence_open": CONTRACT_STATUS,
        "proof_ready_skeleton_contract_equivalent_status_congruence_open": CONTRACT_SKELETON,
        "coordinate-contraction status remains proof-ready with equivalent-coordinate status congruence open": "coordinate-contraction status is refined to payload/domain/normal-form/status-predicate open proof-ready skeleton",
        "Coordinate-contraction status remains proof-ready with equivalent-coordinate status congruence open": "Coordinate-contraction status is refined to payload/domain/normal-form/status-predicate open proof-ready skeleton",
        "keeps coordinate-contraction status proof-ready with equivalent-coordinate status congruence open": "keeps coordinate-contraction status refined to payload/domain/normal-form/status-predicate open proof-ready skeleton",
        "다음 target은 `contract_equivalent_status_congruence_refinement`다.": f"다음 target은 `{NEXT1}`다.",
        "다음 completion target은 `contract_equivalent_status_congruence_refinement`다.": f"다음 completion target은 `{NEXT1}`다.",
        "다음 수학 target은 `contract_equivalent_status_congruence_refinement`다.": f"다음 수학 target은 `{NEXT1}`다.",
        "다음 exact target은 `contract_equivalent_status_congruence_refinement`다.": f"다음 exact target은 `{NEXT1}`다.",
        "현재 다음 exact target은 `contract_equivalent_status_congruence_refinement`다.": f"현재 다음 exact target은 `{NEXT1}`다.",
        "The next exact proof-obligation target is `contract_equivalent_status_congruence_refinement`.": f"The next exact proof-obligation target is `{NEXT1}`.",
        "The next broader proof obligation is `contract_equivalent_status_congruence_refinement`.": f"The next broader proof obligation is `{NEXT1}`.",
        "next exact target: `contract_equivalent_status_congruence_refinement`": f"next exact target: `{NEXT1}`",
        "next action matrix first target: `contract_equivalent_status_congruence_refinement`": f"next action matrix first target: `{NEXT1}`",
        "top recommendation: `contract_equivalent_status_congruence_refinement`": f"top recommendation: `{NEXT1}`",
        "second recommendation: `canonical_compression_status_congruence_refinement`": f"second recommendation: `{NEXT2}`",
        "third recommendation: `family_chain_absorption_source_alignment_refinement`": f"third recommendation: `{NEXT3}`",
        "recommendation order: `contract_equivalent_status_congruence_refinement`, `canonical_compression_status_congruence_refinement`, `family_chain_absorption_source_alignment_refinement`": f"recommendation order: `{NEXT1}`, `{NEXT2}`, `{NEXT3}`",
    }
    for old, new in replacements.items():
        text = text.replace(old, new)
    marker = "\n## contract-equivalent congruence refinement update\n"
    update = f"""
## contract-equivalent congruence refinement update

- selected statement: `{SELECTED}`
- status-domain transfer: `{DOMAIN_STATUS}`
- normal-form transfer: `{NORMAL_STATUS}`
- equivalent-coordinate congruence refinement: `{CONGRUENCE_STATUS}`
- coordinate-congruence skeleton: `{CONTRACT_SKELETON}`
- contract-equivalent operation status: `{CONTRACT_STATUS}`
- higher-support necessity: `{HIGHER_SUPPORT}`
- general theorem readiness: `{GENERAL_READY}`
- next action order: `{NEXT1}`, `{NEXT2}`, `{NEXT3}`

This update does not prove the full general theorem and does not prove
`contract_equivalent_support_coordinates` fully.
"""
    if marker in text:
        text = text.split(marker)[0].rstrip() + "\n" + update
    else:
        text = text.rstrip() + "\n\n" + update
    path.write_text(text, encoding="utf-8")


def main() -> int:
    write_core_refinement_docs()
    write_existing_coordinate_status_docs()
    write_rollup_docs()
    write_general_bridge_docs()
    write_current_certificate_and_reports()
    write_additional_sync_docs()

    for rel in [
        "project_status_summary.md",
        "current_workspace_reality_check.md",
        "current_status_authoritative_longform.md",
        "authoritative_completion_to_100_plan_longform.md",
        "theorem_data_promotion_to_100_plan_longform.md",
        "mathematical_progress_to_100_plan_longform.md",
        "progress_history_1_to_85_longform.md",
    ]:
        refresh_root_doc(B4 / rel)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
