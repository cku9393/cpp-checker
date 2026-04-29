#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


D90 = Path(__file__).resolve().parent
B4 = D90.parent
RUNTIME = D90 / "runtime"

SELECTED = "canonical_motif_status_preserved_under_refined_congruence_or_reduced_or_escape"
CANONICAL_STATUS = "partial_canonical_compression_congruence_proof_ready_domain_normal_form_open"
CANONICAL_SKELETON = "proof_ready_skeleton_canonical_compression_congruence_domain_normal_form_open"
DOMAIN_STATUS = "canonical_compression_status_domain_transfer_proof_ready_motif_domain_open"
NORMAL_STATUS = "canonical_compression_normal_form_transfer_proof_ready_motif_normal_form_open"
CONGRUENCE_STATUS = "canonical_motif_congruence_payload_ready_domain_normal_form_open"
STATUS_CONGRUENCE = "partial_status_congruence_canonical_refined_domain_normal_form_open_remaining_project_contract_alignment_measure_open"
SUPPORT_REDUCTION = "partition_ready_canonical_refined_domain_normal_form_open_remaining_project_contract_alignment_measure_open"
SUPPORT_BOUND = "proof_ready_skeleton_canonical_refined_domain_normal_form_open_remaining_project_contract_alignment_measure_open"
HIGHER_SUPPORT = "higher_support_deferred_after_canonical_congruence_domain_normal_form_open"
GENERAL_READY = "ready_for_family_chain_absorption_source_alignment_refinement"
NEXT1 = "family_chain_absorption_source_alignment_refinement"
NEXT2 = "project_to_active_status_domain_refinement"
NEXT3 = "contract_equivalent_domain_normal_form_refinement"


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
        lines.append("\t".join(str(row.get(col, "")) for col in header))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


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
    "congruence_scope",
    "uses_canonical_motif_notation",
    "uses_compression_semantics",
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
        "statement_key": "canonical_motif_payload_congruence_refined",
        "formal_statement": "Accepted canonical motif compression has a relevant payload refinement from M to M_comp.",
        "assumptions": "W normal;canonical motif M accepted;compressed motif M_comp lower rank;payload roles compatible",
        "conclusion": "payload carriers rewrite or refine through W_comp=normalize(rewrite(W,M,M_comp))",
        "operation_scope": "canonical_motif_compression",
        "congruence_scope": "payload congruence",
        "uses_canonical_motif_notation": "1",
        "uses_compression_semantics": "1",
        "uses_status_language": "0",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "0",
        "uses_normal_form_transfer": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "Payload congruence is available as a motif contract, but layerwise transfer through normalization remains proof-sketch.",
    },
    {
        "statement_key": "canonical_motif_status_domain_transfer",
        "formal_statement": "The compressed witness status domain is compatible with the source motif status domain.",
        "assumptions": "status/certificate-role dependencies are tracked across motif rewrite",
        "conclusion": "source and compressed status predicates can be compared over compatible domains",
        "operation_scope": "canonical_motif_compression",
        "congruence_scope": "status-domain transfer",
        "uses_canonical_motif_notation": "1",
        "uses_compression_semantics": "1",
        "uses_status_language": "1",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "This is required before any status-preservation claim; payload congruence alone is insufficient.",
    },
    {
        "statement_key": "canonical_motif_normal_form_transfer",
        "formal_statement": "The normalized compressed witness stays in a status-applicable normal form.",
        "assumptions": "rewrite(W,M,M_comp) defined;normalization target selected",
        "conclusion": "W_comp has a normal form eligible for status comparison",
        "operation_scope": "canonical_motif_compression",
        "congruence_scope": "normal-form transfer",
        "uses_canonical_motif_notation": "1",
        "uses_compression_semantics": "1",
        "uses_status_language": "1",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "Normal form is named by normalize, but status-relevant invariants under motif rewrite remain a proof obligation.",
    },
    {
        "statement_key": "canonical_motif_counterexample_status_congruence",
        "formal_statement": "Canonical motif compression preserves counterexample-status predicate when payload, status-domain, and normal-form transfer hold.",
        "assumptions": "payload congruence;status-domain transfer;normal-form transfer;status predicate determined by these data",
        "conclusion": "counterexample status is congruent between W and W_comp",
        "operation_scope": "canonical_motif_compression",
        "congruence_scope": "counterexample-status congruence",
        "uses_canonical_motif_notation": "1",
        "uses_compression_semantics": "1",
        "uses_status_language": "1",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "high",
        "reason": "Central proof target; current artifacts do not prove status predicate determination from payload/domain/normal-form data.",
    },
    {
        "statement_key": "canonical_motif_status_preserved_or_reduced_or_escape",
        "formal_statement": "If refined congruence fails, accepted compression either yields a valid smaller witness or routes to a named operation blocker/deferred escape.",
        "assumptions": "accepted compression decreases support/canonical measure;compressed status is preserved, valid reduced status, or failure is classified",
        "conclusion": "preserved, reduced, or escaped branch",
        "operation_scope": "canonical_motif_compression",
        "congruence_scope": "preservation/reduction/escape",
        "uses_canonical_motif_notation": "1",
        "uses_compression_semantics": "1",
        "uses_status_language": "1",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "Measure decrease is available; reduced-status validity and hidden-case completeness remain proof obligations.",
    },
    {
        "statement_key": "full_canonical_motif_status_congruence",
        "formal_statement": "Every canonical motif compression case has complete status congruence.",
        "assumptions": "all canonical motif compression cases in arbitrary support-growth witnesses",
        "conclusion": "full motif status congruence",
        "operation_scope": "canonical_motif_compression",
        "congruence_scope": "full congruence",
        "uses_canonical_motif_notation": "1",
        "uses_compression_semantics": "1",
        "uses_status_language": "1",
        "uses_payload_refinement": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "0",
        "risk": "high",
        "reason": "Out of scope; this round does not prove arbitrary compressed-witness status-domain and normal-form transfer.",
    },
]


domain_header = [
    "domain_component",
    "definition",
    "source_condition",
    "compressed_condition",
    "transfer_rule",
    "proof_requirement",
    "failure_effect",
    "current_status",
    "caveat",
]

domain_rows = [
    {
        "domain_component": "source_motif_status_domain",
        "definition": "The status predicate domain before replacing canonical motif M.",
        "source_condition": "W normal;status predicate defined over source motif roles",
        "compressed_condition": "none yet",
        "transfer_rule": "source side fixed",
        "proof_requirement": "status dependency extraction on M",
        "failure_effect": "status comparison cannot be formed",
        "current_status": "proof_sketch_ready",
        "caveat": "Source domain alone does not prove compressed-domain compatibility.",
    },
    {
        "domain_component": "compressed_motif_status_domain",
        "definition": "The status predicate domain after W_comp=normalize(rewrite(W,M,M_comp)).",
        "source_condition": "rewrite accepted",
        "compressed_condition": "W_comp status inputs indexed by compressed motif roles",
        "transfer_rule": "compressed status domain after rewrite and normalization",
        "proof_requirement": "compressed-domain construction",
        "failure_effect": "ill-formed compressed status becomes named blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Compressed domain may be refined rather than identical.",
    },
    {
        "domain_component": "motif_domain_projection",
        "definition": "The map from source motif status inputs to compressed motif status inputs.",
        "source_condition": "status/certificate roles of M are visible",
        "compressed_condition": "roles of M_comp are visible",
        "transfer_rule": "push status dependencies through motif rewrite",
        "proof_requirement": "role-map compatibility",
        "failure_effect": "domain projection blocker",
        "current_status": "proof_ready_open",
        "caveat": "Not implied by payload congruence.",
    },
    {
        "domain_component": "motif_domain_quotient_refinement",
        "definition": "Compression may identify, remove, or refine motif roles without coordinate quotienting.",
        "source_condition": "accepted lower-rank motif relation",
        "compressed_condition": "compressed roles are lower-rank representatives",
        "transfer_rule": "domain refinement, not coordinate equivalence quotient",
        "proof_requirement": "motif-role status refinement lemma",
        "failure_effect": "status-domain refinement blocker",
        "current_status": "proof_ready_open",
        "caveat": "Separate from contract-equivalent coordinate quotient.",
    },
    {
        "domain_component": "domain_preserved_case",
        "definition": "Status domain is unchanged up to canonical motif relabeling.",
        "source_condition": "status dependencies are motif-rewrite invariant",
        "compressed_condition": "W_comp status inputs match source roles",
        "transfer_rule": "status preservation branch",
        "proof_requirement": "domain equivalence plus normal form",
        "failure_effect": "route to domain-refined or blocker branch",
        "current_status": "proof_sketch_ready",
        "caveat": "Conditional only.",
    },
    {
        "domain_component": "domain_refined_case",
        "definition": "Compression changes status representation but leaves a valid reduced status domain.",
        "source_condition": "accepted compression is nontrivial",
        "compressed_condition": "W_comp status is valid reduced obstruction",
        "transfer_rule": "reduction branch",
        "proof_requirement": "valid reduced compressed-status theorem",
        "failure_effect": "reduction branch blocked",
        "current_status": "proof_sketch_ready",
        "caveat": "Uses lexicographic measure decrease, not preservation.",
    },
    {
        "domain_component": "domain_lost_case",
        "definition": "The compressed status domain cannot be compared with the source domain.",
        "source_condition": "status roles are not preserved/refined",
        "compressed_condition": "W_comp exits comparable status domain",
        "transfer_rule": "no preservation claim",
        "proof_requirement": "named failure classification",
        "failure_effect": "named canonical-compression status-domain blocker",
        "current_status": "proved_under_current_scope_as_partition",
        "caveat": "Classification only.",
    },
    {
        "domain_component": "domain_escape_case",
        "definition": "Domain failure remains after operation-local proof attempts.",
        "source_condition": "operation blockers remain",
        "compressed_condition": "no true irreducible higher-support witness established",
        "transfer_rule": "defer higher support until operation proofs close",
        "proof_requirement": "higher-support bound later",
        "failure_effect": "deferred higher-support escape",
        "current_status": "higher_support_deferred",
        "caveat": "No support9+ scan.",
    },
    {
        "domain_component": "canonical_motif_rank_relation",
        "definition": "Accepted compression decreases motif rank when support size is fixed.",
        "source_condition": "rank(M)>rank(M_comp)",
        "compressed_condition": "support size fixed or smaller",
        "transfer_rule": "measure branch, not status-domain proof",
        "proof_requirement": "keep rank decrease separate from status congruence",
        "failure_effect": "measure proof cannot be used as status preservation",
        "current_status": "proved_under_current_scope_for_measure_only",
        "caveat": "Do not promote measure decrease to status congruence.",
    },
    {
        "domain_component": "normal_form_relation",
        "definition": "Status-domain transfer requires compressed normal-form eligibility.",
        "source_condition": "rewrite(W,M,M_comp) defined",
        "compressed_condition": "W_comp normalized",
        "transfer_rule": "normal form gates status predicate applicability",
        "proof_requirement": "normal-form transfer proof",
        "failure_effect": "status-domain transfer blocked",
        "current_status": "proof_ready_normal_form_open",
        "caveat": "Separate first-class obligation.",
    },
    {
        "domain_component": "payload_refinement_relation",
        "definition": "Payload refinement feeds domain transfer but does not imply it.",
        "source_condition": "payload roles compatible in accepted motif rewrite",
        "compressed_condition": "compressed motif has payload carrier",
        "transfer_rule": "payload side of motif rewrite",
        "proof_requirement": "motif payload transfer through normalization",
        "failure_effect": "status may still fail",
        "current_status": "proof_sketch_ready",
        "caveat": "Payload congruence and status-domain congruence are distinct.",
    },
    {
        "domain_component": "counterexample_status_predicate_relation",
        "definition": "Counterexample-status congruence follows only after payload, domain, and normal-form transfer plus predicate determination.",
        "source_condition": "all transfer premises hold",
        "compressed_condition": "W and W_comp comparable",
        "transfer_rule": "predicate equality/refinement",
        "proof_requirement": "canonical-motif counterexample-status congruence theorem",
        "failure_effect": "status congruence blocked",
        "current_status": "blocked_by_counterexample_status",
        "caveat": "No proof-completed promotion.",
    },
]


normal_header = [
    "normal_form_component",
    "definition",
    "source_condition",
    "compressed_condition",
    "transfer_status",
    "proof_requirement",
    "failure_effect",
    "current_status",
    "caveat",
]

normal_rows = [
    {
        "normal_form_component": "source_motif_normal_form",
        "definition": "The canonical motif M is in the source witness normal form.",
        "source_condition": "W normal;M selected as canonical motif",
        "compressed_condition": "none yet",
        "transfer_status": "source side available",
        "proof_requirement": "source normal-form contract",
        "failure_effect": "not enough for compressed witness",
        "current_status": "proved_under_current_scope_input",
        "caveat": "Source normal form is not compressed transfer.",
    },
    {
        "normal_form_component": "compressed_motif_normal_form",
        "definition": "The compressed motif M_comp is the accepted lower-rank canonical representative.",
        "source_condition": "M accepted compressible",
        "compressed_condition": "M_comp canonical and lower rank",
        "transfer_status": "compressed motif target named",
        "proof_requirement": "motif normal-form compatibility",
        "failure_effect": "compressed motif invalid blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Accepted representative does not by itself prove witness normal form.",
    },
    {
        "normal_form_component": "source_witness_normal_form",
        "definition": "The full source witness W is normalized before compression.",
        "source_condition": "W normal over support S",
        "compressed_condition": "rewrite target not yet normalized",
        "transfer_status": "input invariant available",
        "proof_requirement": "source witness normal-form contract",
        "failure_effect": "operation precondition fails",
        "current_status": "proved_under_current_scope_input",
        "caveat": "Input only.",
    },
    {
        "normal_form_component": "compressed_witness_normal_form",
        "definition": "W_comp=normalize(rewrite(W,M,M_comp)) is the candidate compressed witness normal form.",
        "source_condition": "rewrite(W,M,M_comp) defined",
        "compressed_condition": "normalization succeeds and preserves status eligibility",
        "transfer_status": "target named, invariants open",
        "proof_requirement": "compressed witness normal-form theorem",
        "failure_effect": "status comparison blocked",
        "current_status": "proof_ready_open",
        "caveat": "Normalizing step is not proof completion.",
    },
    {
        "normal_form_component": "motif_compression_normal_form_rule",
        "definition": "Rewrite then normalize should preserve or refine every status-relevant invariant.",
        "source_condition": "accepted motif rewrite",
        "compressed_condition": "normal-form fields reconstructed",
        "transfer_status": "refinement rule",
        "proof_requirement": "motif-rewrite invariant preservation",
        "failure_effect": "normal-form blocker",
        "current_status": "proof_ready_open",
        "caveat": "Needs motif-specific lemma.",
    },
    {
        "normal_form_component": "normal_form_preservation",
        "definition": "Compression keeps the same status-applicable normal-form class.",
        "source_condition": "motif rewrite invariant on status fields",
        "compressed_condition": "W_comp in same status class",
        "transfer_status": "preservation branch",
        "proof_requirement": "normal-form preservation theorem",
        "failure_effect": "route to refinement or blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Conditional.",
    },
    {
        "normal_form_component": "normal_form_refinement",
        "definition": "Compression changes representation but leaves a valid refined normal form.",
        "source_condition": "accepted compression is nontrivial",
        "compressed_condition": "W_comp valid reduced witness form",
        "transfer_status": "reduction/status-domain branch",
        "proof_requirement": "valid refined compressed witness theorem",
        "failure_effect": "valid-reduction branch blocked",
        "current_status": "proof_sketch_ready",
        "caveat": "Can support reduction, not preservation.",
    },
    {
        "normal_form_component": "normal_form_failure",
        "definition": "Compressed normal-form invariants are not established.",
        "source_condition": "normal-form transfer missing",
        "compressed_condition": "W_comp not proven status-applicable",
        "transfer_status": "no status preservation claim",
        "proof_requirement": "named failure classification",
        "failure_effect": "canonical-compression normal-form blocker",
        "current_status": "proved_under_current_scope_as_partition",
        "caveat": "Does not imply higher-support by itself.",
    },
    {
        "normal_form_component": "status_domain_relation",
        "definition": "Compressed normal form feeds compressed status-domain validity.",
        "source_condition": "status predicate needs normal form",
        "compressed_condition": "W_comp must be status-domain valid",
        "transfer_status": "normal form gates domain transfer",
        "proof_requirement": "normal-form plus domain theorem",
        "failure_effect": "status-domain transfer blocked",
        "current_status": "blocked_by_normal_form",
        "caveat": "Separate from counterexample-status congruence.",
    },
    {
        "normal_form_component": "family_chain_source_form_relation",
        "definition": "Recognized family-chain source form must survive motif rewrite when relevant.",
        "source_condition": "source recognizer fields are motif-compatible",
        "compressed_condition": "W_comp recognized or failure named",
        "transfer_status": "source-form transfer",
        "proof_requirement": "family-chain source-form motif proof",
        "failure_effect": "source-form blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Not family-chain absorption.",
    },
    {
        "normal_form_component": "higher_support_relation",
        "definition": "Normal-form failure is operation-local until all operation proofs close.",
        "source_condition": "operation proof open",
        "compressed_condition": "no true irreducible escape established",
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
    "motif_definition_used",
    "compression_rule_used",
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
        "lemma_component": "motif_relation_available",
        "refined_statement": "Accepted canonical motif compression supplies finite source and compressed motif objects.",
        "assumptions": "W finite;canonical motif M accepted;M_comp selected",
        "conclusion": "M and M_comp are comparable motif objects",
        "motif_definition_used": "canonical_motif_notation_90.tsv",
        "compression_rule_used": "canonical_motif_compression_operation_semantics_90.tsv",
        "payload_congruence": "not_primary",
        "status_domain_transfer": "not_primary",
        "normal_form_transfer": "not_primary",
        "counterexample_status_congruence": "not_primary",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "global motif completeness outside accepted compression",
        "caveat": "Accepted relation only.",
    },
    {
        "lemma_component": "payload_congruence_refined",
        "refined_statement": "Motif payload roles rewrite/refine through the accepted compressed motif.",
        "assumptions": "payload-role compatibility in accepted motif compression",
        "conclusion": "compressed payload carrier is well-defined up to normalization",
        "motif_definition_used": "canonical_motif_notation_90.tsv",
        "compression_rule_used": "rewrite(W,M,M_comp)",
        "payload_congruence": "proof_sketch_ready",
        "status_domain_transfer": "possible",
        "normal_form_transfer": "possible",
        "counterexample_status_congruence": "possible",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "layerwise payload refinement through motif normalization",
        "caveat": "Not enough for status congruence.",
    },
    {
        "lemma_component": "status_domain_transfer_refined",
        "refined_statement": "Source and compressed status domains are compatible only when status dependencies rewrite/refine across M->M_comp.",
        "assumptions": "status/certificate roles compatible;motif-domain projection defined",
        "conclusion": "W and W_comp status domains comparable",
        "motif_definition_used": "canonical_compression_status_domain_transfer_90.tsv",
        "compression_rule_used": "normalize(rewrite(W,M,M_comp))",
        "payload_congruence": "possible",
        "status_domain_transfer": "proof_ready_open",
        "normal_form_transfer": "possible",
        "counterexample_status_congruence": "possible",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "motif status-domain invariance/refinement theorem",
        "caveat": "Open but first-class.",
    },
    {
        "lemma_component": "normal_form_transfer_refined",
        "refined_statement": "Motif compression followed by normalization preserves the normal form required by status semantics.",
        "assumptions": "rewrite(W,M,M_comp) defined",
        "conclusion": "W_comp is status-applicable",
        "motif_definition_used": "canonical_compression_normal_form_transfer_90.tsv",
        "compression_rule_used": "normalize(rewrite(W,M,M_comp))",
        "payload_congruence": "possible",
        "status_domain_transfer": "possible",
        "normal_form_transfer": "proof_ready_open",
        "counterexample_status_congruence": "possible",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "motif-rewrite normal-form preservation",
        "caveat": "Open but first-class.",
    },
    {
        "lemma_component": "payload_domain_normal_form_to_status",
        "refined_statement": "Payload congruence plus domain and normal-form transfer imply status congruence only if the status predicate is determined by those data.",
        "assumptions": "payload/domain/normal-form premises hold",
        "conclusion": "counterexample-status predicate is invariant",
        "motif_definition_used": "status_preservation_language_90.tsv",
        "compression_rule_used": "canonical motif rewrite",
        "payload_congruence": "yes",
        "status_domain_transfer": "yes",
        "normal_form_transfer": "yes",
        "counterexample_status_congruence": "blocked",
        "proof_status": "blocked_by_counterexample_status",
        "missing_hypothesis": "status predicate determination theorem for motif compression",
        "caveat": "Central unresolved theorem.",
    },
    {
        "lemma_component": "counterexample_status_congruence_refined",
        "refined_statement": "Canonical motif compression preserves counterexample status under refined congruence.",
        "assumptions": "all refined congruence premises hold",
        "conclusion": "status preserved",
        "motif_definition_used": "canonical_motif_status_congruence_lemma_90.tsv",
        "compression_rule_used": "rewrite and normalize",
        "payload_congruence": "yes",
        "status_domain_transfer": "yes",
        "normal_form_transfer": "yes",
        "counterexample_status_congruence": "proof_ready_open",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "canonical-motif counterexample-status congruence theorem",
        "caveat": "Conditional only.",
    },
    {
        "lemma_component": "domain_change_reduction",
        "refined_statement": "If compression changes status domain but W_comp is a valid reduced counterexample, lexicographic decrease gives a smaller witness.",
        "assumptions": "accepted compression decreases measure;W_comp valid reduced obstruction",
        "conclusion": "smaller witness branch",
        "motif_definition_used": "canonical_motif_compression_smaller_witness_90.tsv",
        "compression_rule_used": "normalize(rewrite(W,M,M_comp))",
        "payload_congruence": "possible",
        "status_domain_transfer": "possible",
        "normal_form_transfer": "yes",
        "counterexample_status_congruence": "reduced_status",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "valid reduced compressed-status theorem",
        "caveat": "Uses measure decrease, not preservation.",
    },
    {
        "lemma_component": "failure_classification",
        "refined_statement": "If refined congruence fails, compression status does not silently succeed.",
        "assumptions": "payload/domain/normal/status premise missing",
        "conclusion": "named blocker or deferred higher-support escape",
        "motif_definition_used": "higher_support_escape_interface_90.tsv",
        "compression_rule_used": "compression attempt optional",
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
        "refined_statement": "Canonical motif congruence is payload-ready but status-domain/normal-form/status-predicate open.",
        "assumptions": "rows above combined",
        "conclusion": "proof-ready refinement, not completed",
        "motif_definition_used": "accepted canonical motif relation",
        "compression_rule_used": "W_comp=normalize(rewrite(W,M,M_comp))",
        "payload_congruence": "payload proof-sketch",
        "status_domain_transfer": "status-domain open",
        "normal_form_transfer": "normal-form open",
        "counterexample_status_congruence": "status predicate open",
        "proof_status": CONGRUENCE_STATUS,
        "missing_hypothesis": "status-domain transfer;normal-form transfer;status predicate determination;valid reduced-status theorem",
        "caveat": "No full canonical compression proof.",
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
        "obligation_key": "canonical_compression_congruence_refinement_language_well_defined",
        "statement": "The refinement uses payload congruence, status-domain transfer, normal-form transfer, status congruence, reduction, and escape labels.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "canonical_motif_notation_90.tsv;status_preservation_language_90.tsv",
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
        "statement": "Accepted canonical motif compression includes payload-role refinement.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "canonical_motif_status_congruence_lemma_90.tsv;canonical_motif_compression_operation_semantics_90.tsv",
        "missing_sublemmas": "layerwise payload refinement under motif rewrite normalization",
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
        "statement": "Source and compressed status domains are comparable only under motif-rewrite status dependency compatibility.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "canonical_compression_status_domain_transfer_90.tsv",
        "missing_sublemmas": "motif status-domain invariance/refinement theorem",
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
        "statement": "W_comp must satisfy selected normal form after motif rewrite and normalization.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "canonical_compression_normal_form_transfer_90.tsv;support_notation_and_normal_form_90.tsv",
        "missing_sublemmas": "motif-rewrite normal-form preservation",
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
        "existing_verified_inputs": "canonical_motif_congruence_refinement_90.tsv",
        "missing_sublemmas": "canonical status predicate determination theorem",
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
        "statement": "Normal-form transfer makes the compressed witness eligible for status comparison.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "support_notation_and_normal_form_90.tsv;canonical_compression_normal_form_transfer_90.tsv",
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
        "obligation_key": "compression_preserves_status_under_refined_congruence",
        "statement": "If payload, domain, normal-form, and status predicate congruence hold, compression preserves status.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "canonical_compression_status_congruence_refinement_skeleton_90.tsv",
        "missing_sublemmas": "refined canonical motif congruence theorem",
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
        "obligation_key": "compression_reduces_to_smaller_witness_if_status_domain_changes",
        "statement": "If compression changes status domain but W_comp is valid reduced status, lexicographic measure decrease gives a smaller witness.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "canonical_motif_compression_smaller_witness_90.tsv",
        "missing_sublemmas": "valid reduced-status theorem for compressed witness",
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
        "obligation_key": "compression_failure_is_named_escape",
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
        "obligation_key": "no_hidden_canonical_congruence_failure_case",
        "statement": "The canonical branch has no hidden class outside preserved, reduced, no accepted compression/no decrease, named blocker, or deferred higher-support escape.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "canonical_compression_status_congruence_refinement_scope_inventory_90.tsv",
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
        "sublemma_key": "canonical_compression_congruence_refinement_language_well_defined",
        "proof_status": "proved_under_current_scope",
        "assumptions": "canonical motif notation and common status language are fixed",
        "conclusion": "refinement outcomes are comparable",
        "proof_summary": "The refinement distinguishes payload congruence, status-domain transfer, normal-form transfer, status congruence, reduction, named blocker, and deferred higher-support escape.",
        "evidence_path": "branch_4/90/runtime/canonical_motif_notation_90.tsv;branch_4/90/runtime/status_preservation_language_90.tsv",
        "missing_hypothesis": "none",
        "next_action": "use_refinement_language",
    },
    {
        "sublemma_key": "payload_congruence_available",
        "proof_status": "proof_sketch_only",
        "assumptions": "accepted motif compression includes payload-role compatibility",
        "conclusion": "payload roles transfer through motif rewrite",
        "proof_summary": "Payload roles are part of accepted motif compression, but layerwise payload refinement through rewrite normalization remains a dedicated proof.",
        "evidence_path": "branch_4/90/runtime/canonical_motif_status_congruence_lemma_90.tsv",
        "missing_hypothesis": "layerwise payload refinement under motif rewrite",
        "next_action": "payload_congruence_refinement_later",
    },
    {
        "sublemma_key": "status_domain_transfer_well_defined",
        "proof_status": "proof_sketch_only",
        "assumptions": "status/certificate dependencies are motif-rewrite compatible",
        "conclusion": "source and compressed status domains are comparable",
        "proof_summary": "The transfer relation is first-class, but current artifacts do not prove arbitrary compressed status-domain invariance or refinement.",
        "evidence_path": "branch_4/90/runtime/canonical_compression_status_domain_transfer_90.tsv",
        "missing_hypothesis": "motif status-domain invariance/refinement theorem",
        "next_action": "status_domain_transfer_refinement",
    },
    {
        "sublemma_key": "normal_form_transfer_well_defined",
        "proof_status": "proof_sketch_only",
        "assumptions": "rewrite(W,M,M_comp) is defined",
        "conclusion": "W_comp is status-domain eligible after normalization",
        "proof_summary": "The normal-form target is named, but motif-rewrite preservation of all status-relevant normal-form invariants remains open.",
        "evidence_path": "branch_4/90/runtime/canonical_compression_normal_form_transfer_90.tsv",
        "missing_hypothesis": "motif-rewrite normal-form preservation",
        "next_action": "normal_form_transfer_refinement",
    },
    {
        "sublemma_key": "payload_plus_domain_implies_status_congruence",
        "proof_status": "blocked_by_counterexample_status",
        "assumptions": "payload congruence and domain transfer hold",
        "conclusion": "status predicate is invariant",
        "proof_summary": "This requires a theorem that counterexample status is determined by motif-compatible payload, domain, and normal-form data.",
        "evidence_path": "branch_4/90/runtime/canonical_motif_congruence_refinement_90.tsv",
        "missing_hypothesis": "canonical status predicate determination theorem",
        "next_action": "status_predicate_congruence_refinement",
    },
    {
        "sublemma_key": "normal_form_transfer_supports_status_congruence",
        "proof_status": "proof_sketch_only",
        "assumptions": "normal-form transfer holds",
        "conclusion": "compressed witness is eligible for status comparison",
        "proof_summary": "Normal-form transfer supplies the status-domain precondition, but does not alone prove status congruence.",
        "evidence_path": "branch_4/90/runtime/canonical_compression_normal_form_transfer_90.tsv",
        "missing_hypothesis": "normal-form transfer theorem",
        "next_action": "normal_form_transfer_refinement",
    },
    {
        "sublemma_key": "compression_preserves_status_under_refined_congruence",
        "proof_status": "proof_sketch_only",
        "assumptions": "payload, domain, normal-form, and status predicate congruence hold",
        "conclusion": "status is preserved",
        "proof_summary": "Under all refined congruence premises, preservation is direct; the missing premises are explicitly isolated.",
        "evidence_path": "branch_4/90/runtime/canonical_compression_status_congruence_refinement_skeleton_90.tsv",
        "missing_hypothesis": "refined canonical motif congruence theorem",
        "next_action": "status_predicate_congruence_refinement",
    },
    {
        "sublemma_key": "compression_reduces_to_smaller_witness_if_status_domain_changes",
        "proof_status": "blocked_by_smaller_witness",
        "assumptions": "accepted compression decreases lexicographic measure and compressed status remains valid",
        "conclusion": "W_comp is a smaller witness",
        "proof_summary": "Measure decrease is proved, but validity of changed compressed status as a reduced counterexample is not yet proved.",
        "evidence_path": "branch_4/90/runtime/canonical_motif_compression_smaller_witness_90.tsv",
        "missing_hypothesis": "valid reduced compressed-status theorem",
        "next_action": "reduced_status_validity_refinement",
    },
    {
        "sublemma_key": "compression_failure_is_named_escape",
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
        D90 / "canonical_compression_status_congruence_refinement_scope_memo_90.md",
        "Canonical Compression Status Congruence Refinement Scope Memo 90",
        f"""
## selected target

`canonical_compression_status_congruence_refinement`

Selected statement:
`{SELECTED}`.

This round does not prove full canonical-motif status congruence. It selects
the payload/domain/normal-form/status predicate decomposition and keeps
`full_canonical_motif_status_congruence` out of scope.

Runtime inventory:
`branch_4/90/runtime/canonical_compression_status_congruence_refinement_scope_inventory_90.tsv`.
""",
    )
    write_table(RUNTIME / "canonical_compression_status_congruence_refinement_scope_inventory_90.tsv", scope_header, scope_rows)

    write_md(
        D90 / "canonical_compression_status_domain_transfer_90.md",
        "Canonical Compression Status Domain Transfer 90",
        f"""
## status

`{DOMAIN_STATUS}`

Status-domain transfer asks whether the source counterexample-status predicate
and the compressed witness status predicate are defined over compatible domains.
For canonical motif compression this is not automatic from payload congruence.
The motif rewrite can refine, remove, or reinterpret status/certificate roles
even when payload roles are compatible.

The transfer is proof-ready but open on motif status-domain invariance or
refinement, compressed normal-form eligibility, and valid reduced-status
fallback if the compressed domain changes.

Runtime table:
`branch_4/90/runtime/canonical_compression_status_domain_transfer_90.tsv`.
""",
    )
    write_table(RUNTIME / "canonical_compression_status_domain_transfer_90.tsv", domain_header, domain_rows)

    write_md(
        D90 / "canonical_compression_normal_form_transfer_90.md",
        "Canonical Compression Normal Form Transfer 90",
        f"""
## status

`{NORMAL_STATUS}`

Canonical motif compression constructs `W_comp =
normalize(rewrite(W,M,M_comp))`. The normalizing step names the target form, but
it does not prove that every status-relevant normal-form invariant transfers
through motif rewrite.

Runtime table:
`branch_4/90/runtime/canonical_compression_normal_form_transfer_90.tsv`.
""",
    )
    write_table(RUNTIME / "canonical_compression_normal_form_transfer_90.tsv", normal_header, normal_rows)

    write_md(
        D90 / "canonical_motif_congruence_refinement_90.md",
        "Canonical Motif Congruence Refinement 90",
        f"""
## status

`{CONGRUENCE_STATUS}`

The previous canonical-motif status congruence lemma is refined into accepted
motif relation availability, payload-role congruence, status-domain transfer,
normal-form transfer, counterexample-status predicate congruence, and
reduction/escape classification.

Payload congruence is proof-sketch ready. Counterexample-status congruence is
not completed because payload congruence alone does not prove status-domain or
normal-form transfer.

Runtime table:
`branch_4/90/runtime/canonical_motif_congruence_refinement_90.tsv`.
""",
    )
    write_table(RUNTIME / "canonical_motif_congruence_refinement_90.tsv", lemma_header, lemma_rows)

    write_md(
        D90 / "canonical_compression_status_congruence_refinement_obligations_90.md",
        "Canonical Compression Status Congruence Refinement Obligations 90",
        """
## status

`canonical_compression_congruence_refinement_obligations_domain_normal_form_open`

The refinement has `10` first-class obligations. Language and failure
classification are current-scope proved. Payload and conditional preservation
are proof-sketch. Status-domain transfer, normal-form transfer,
counterexample-status predicate determination, and valid reduced-status fallback
remain open.

Runtime table:
`branch_4/90/runtime/canonical_compression_status_congruence_refinement_obligations_90.tsv`.
""",
    )
    write_table(RUNTIME / "canonical_compression_status_congruence_refinement_obligations_90.tsv", obligation_header, obligation_rows)

    write_md(
        D90 / "canonical_compression_status_congruence_refinement_sublemma_proofs_90.md",
        "Canonical Compression Status Congruence Refinement Sublemma Proofs 90",
        """
## proof attempt status

- proved under current scope: `2`
- proof sketch only: `5`
- blocked: `2`

The proved rows are language well-definedness and named failure classification.
The blocked rows are status predicate determination and valid reduced-status
fallback. The other rows are proof-sketch/proof-ready and remain conditional.

Runtime table:
`branch_4/90/runtime/canonical_compression_status_congruence_refinement_sublemma_proofs_90.tsv`.
""",
    )
    write_table(RUNTIME / "canonical_compression_status_congruence_refinement_sublemma_proofs_90.tsv", sublemma_header, sublemma_rows)

    write_md(
        D90 / "canonical_compression_status_congruence_refinement_skeleton_90.md",
        "Canonical Compression Status Congruence Refinement Skeleton 90",
        f"""
## lemma

`{SELECTED}`

## statement

Let `W` be a normal support `>8` witness with canonical motif `M`, accepted
lower-rank compressed motif `M_comp`, and compressed witness
`W_comp = normalize(rewrite(W,M,M_comp))`.

Canonical motif compression either preserves counterexample status under
payload congruence, status-domain transfer, normal-form transfer, and status
predicate congruence; gives a smaller witness when changed compressed status is
valid; or routes failure to a named canonical-compression blocker or deferred
higher-support escape.

## status

`{CANONICAL_SKELETON}`

This is not a completed proof of `canonical_motif_compression`, not a full
support reduction proof, not support8 sufficiency, and not a full general
theorem.

Runtime skeleton:
`branch_4/90/runtime/canonical_compression_status_congruence_refinement_skeleton_90.tsv`.
""",
    )
    write_metric(
        RUNTIME / "canonical_compression_status_congruence_refinement_skeleton_90.tsv",
        [
            ("lemma_name", SELECTED),
            ("selected_statement", SELECTED),
            (
                "exact_statement",
                "For a normal support>8 witness W with accepted canonical motif compression M->M_comp, W_comp=normalize(rewrite(W,M,M_comp)) preserves counterexample status under payload congruence, status-domain transfer, normal-form transfer, and status-predicate congruence, otherwise yields a smaller valid reduced witness or a named canonical-compression blocker/deferred higher-support escape.",
            ),
            ("assumption_count", "9"),
            ("conclusion", "Canonical-compression congruence is first-class and proof-ready; payload congruence is proof-sketch ready, while status-domain, normal-form, status-predicate, and reduced-status obligations remain open."),
            ("canonical_motif_definition", "finite canonical motif with support, payload, certificate-role, status-domain, and motif-rank data"),
            ("compressed_witness_relation", "W_comp=normalize(rewrite(W,M,M_comp))"),
            ("status_domain_transfer", DOMAIN_STATUS),
            ("normal_form_transfer", NORMAL_STATUS),
            ("payload_congruence", "proof_sketch_ready_motif_payload_congruence"),
            ("counterexample_status_congruence", CONGRUENCE_STATUS),
            ("smaller_witness_fallback", "proof_sketch_ready_if_compressed_status_is_valid_reduced_counterexample"),
            ("failure_to_escape_case", "proved_under_current_scope_as_named_canonical_compression_blocker_or_deferred_higher_support_escape"),
            ("relation_to_previous_canonical_status_skeleton", "refines proof_ready_skeleton_canonical_compression_status_congruence_open into domain/normal-form/status-predicate obligations"),
            ("relation_to_coordinate_congruence_refinement", "separate operation; coordinate contraction quotients equivalent support coordinates, while compression rewrites canonical motifs"),
            ("relation_to_project_to_active_locality_result", "separate operation; project removes inactive support while compression rewrites an active canonical motif; both need status-domain/normal-form transfer"),
            ("relation_to_status_congruence_bridge", "refines canonical-compression row to payload/domain/normal-form open proof-ready skeleton"),
            ("missing_steps", "motif status-domain invariance/refinement;motif normal-form transfer;layerwise payload refinement;status predicate determination;valid reduced-status theorem;family-chain source-form motif transfer"),
            ("exact_caveat", "canonical_compression_operation_not_fully_proved_support_bound_not_completed_support8_sufficiency_not_proved_no_support9_scan_full_general_theorem_not_proved"),
            ("final_status", CANONICAL_SKELETON),
        ],
    )

    write_metric(RUNTIME / "canonical_congruence_refinement_fingerprint_90.tsv", [
        ("canonical_congruence_refinement", CANONICAL_SKELETON),
        ("selected_statement", SELECTED),
        ("obligation_count", "10"),
        ("proved_under_current_scope_sublemma_count", "2"),
        ("proof_sketch_only_sublemma_count", "5"),
        ("blocked_sublemma_count", "2"),
        ("fingerprint", f"{CANONICAL_SKELETON}|10|2|5|2|{NEXT1}"),
    ])
    write_metric(RUNTIME / "canonical_compression_status_domain_transfer_fingerprint_90.tsv", [
        ("canonical_compression_status_domain_transfer", DOMAIN_STATUS),
        ("domain_component_count", str(len(domain_rows))),
        ("fingerprint", f"{DOMAIN_STATUS}|{len(domain_rows)}|motif_domain_open"),
    ])
    write_metric(RUNTIME / "canonical_compression_normal_form_transfer_fingerprint_90.tsv", [
        ("canonical_compression_normal_form_transfer", NORMAL_STATUS),
        ("normal_form_component_count", str(len(normal_rows))),
        ("fingerprint", f"{NORMAL_STATUS}|{len(normal_rows)}|motif_normal_form_open"),
    ])


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
        "proof_status": "proof_ready_skeleton_status_domain_open",
        "missing_sublemma": "status-domain invariance;normal-form transfer;complete status dependency extraction;valid reduced-status fallback",
        "next_action": NEXT2,
    },
    {
        "operation_key": "contract_equivalent_support_coordinates",
        "current_operation_status": "partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open",
        "input_case": "accepted nontrivial equivalence class with payload/status congruence preconditions",
        "status_behavior": "status_preserved_or_reduced_or_named_operation_blocker_under_refined_congruence",
        "measure_decrease_status": "support_size_decreases_when_nontrivial_class_exists",
        "payload_refinement_status": "proof_sketch_ready_payload_role_congruence",
        "normal_form_status": "contract_equivalent_normal_form_transfer_proof_ready_quotient_normal_form_open",
        "counterexample_status_status": "proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open",
        "contradiction_status": "not_applicable",
        "escape_status": "status_escape_to_named_operation_blocker_or_deferred_higher_support_escape",
        "proof_status": "proof_ready_skeleton_domain_normal_form_open",
        "missing_sublemma": "quotient status-domain invariance;quotient normal-form transfer;status predicate determination;valid reduced-status fallback",
        "next_action": NEXT3,
    },
    {
        "operation_key": "canonical_motif_compression",
        "current_operation_status": CANONICAL_STATUS,
        "input_case": "accepted compressible motif lowers support size or motif rank",
        "status_behavior": "status_preserved_or_reduced_or_named_operation_blocker_under_refined_congruence",
        "measure_decrease_status": "lexicographic_measure_decreases_under_accepted_compression",
        "payload_refinement_status": "proof_sketch_ready_motif_payload_congruence",
        "normal_form_status": NORMAL_STATUS,
        "counterexample_status_status": CANONICAL_SKELETON,
        "contradiction_status": "not_applicable",
        "escape_status": "status_escape_to_named_operation_blocker_or_deferred_higher_support_escape",
        "proof_status": "proof_ready_skeleton_domain_normal_form_open",
        "missing_sublemma": "motif status-domain transfer;motif normal-form transfer;status predicate determination;valid reduced-status fallback",
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
        "proof_status": "proof_ready_skeleton_source_alignment_measure_open",
        "missing_sublemma": "source-target payload/status alignment;residual measure decrease;payload/status transfer",
        "next_action": NEXT1,
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


def write_existing_canonical_status_docs() -> None:
    write_md(
        D90 / "canonical_compression_status_skeleton_90.md",
        "Canonical Compression Status Skeleton 90",
        f"""
## status after congruence refinement

`{CANONICAL_SKELETON}`

The selected canonical-compression statement is refined to:

`{SELECTED}`.

This does not prove the full operation. It separates canonical-motif status
congruence into payload congruence, status-domain transfer, normal-form
transfer, counterexample-status predicate congruence, smaller-witness fallback,
and named escape classification.

Runtime skeleton:
`branch_4/90/runtime/canonical_compression_status_skeleton_90.tsv`.
""",
    )
    write_metric(
        RUNTIME / "canonical_compression_status_skeleton_90.tsv",
        [
            ("lemma_name", SELECTED),
            ("selected_statement", SELECTED),
            ("exact_statement", "Canonical motif compression preserves status under refined congruence, reduces if compressed status remains valid, or names blocker/deferred escape."),
            ("assumption_count", "10"),
            ("conclusion", "Canonical-compression status blocker is refined to payload/domain/normal-form/status-predicate open skeleton."),
            ("canonical_motif_definition", "accepted motif relation includes syntactic,payload,canonical,status/certificate,family-chain source roles"),
            ("compressed_witness_relation", "W_comp=normalize(rewrite(W,M,M_comp))"),
            ("congruence_lemma_status", CONGRUENCE_STATUS),
            ("status_domain_transfer", DOMAIN_STATUS),
            ("normal_form_transfer", NORMAL_STATUS),
            ("payload_preservation", "proof_sketch_ready_under_motif_payload_congruence"),
            ("counterexample_status_preservation", "proof_sketch_ready_if_payload_domain_normal_form_and_status_predicate_congruence_hold"),
            ("smaller_witness_fallback", "proof_sketch_ready_if_compressed_status_is_valid_reduced_counterexample"),
            ("measure_decrease_use", "proved_under_current_scope_for_accepted_compressible_motif"),
            ("failure_to_escape_case", "proved_under_current_scope_as_named_canonical_compression_blocker_or_deferred_higher_support_escape"),
            ("canonical_compression_status_obligation_count", "10"),
            ("proved_under_current_scope_sublemma_count", "2"),
            ("proof_sketch_only_sublemma_count", "5"),
            ("blocked_sublemma_count", "2"),
            ("missing_steps", "motif status-domain invariance/refinement;motif normal-form transfer;layerwise payload refinement;status predicate determination;valid reduced-status fallback;family-chain source-form motif transfer"),
            ("exact_caveat", "canonical_compression_status_not_proved_canonical_compression_operation_not_fully_proved_full_support_reduction_not_proved_no_support9_scan_full_general_theorem_not_proved"),
            ("final_status", CANONICAL_SKELETON),
            ("next_blocker", NEXT1),
        ],
    )

    write_md(
        D90 / "canonical_motif_status_congruence_lemma_90.md",
        "Canonical Motif Status Congruence Lemma 90",
        f"""
## status after congruence refinement

`{CONGRUENCE_STATUS}`

Payload-role congruence remains proof-sketch ready under the accepted canonical
motif compression contract. Counterexample-status congruence is not completed:
it requires compressed status-domain transfer, compressed normal-form transfer,
status predicate determination from motif-compatible data, and valid
reduced-status fallback if the compressed witness changes the status domain.

Runtime table:
`branch_4/90/runtime/canonical_motif_status_congruence_lemma_90.tsv`.
""",
    )
    legacy_header = [
        "lemma_component",
        "statement",
        "assumptions",
        "conclusion",
        "motif_definition_used",
        "compression_rule_used",
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
            "motif_definition_used": row["motif_definition_used"],
            "compression_rule_used": row["compression_rule_used"],
            "payload_dependency": row["payload_congruence"],
            "counterexample_status_dependency": row["counterexample_status_congruence"],
            "canonicalization_dependency": row["normal_form_transfer"],
            "proof_status": row["proof_status"],
            "missing_hypothesis": row["missing_hypothesis"],
            "caveat": row["caveat"],
        })
    write_table(RUNTIME / "canonical_motif_status_congruence_lemma_90.tsv", legacy_header, legacy_rows)

    write_md(
        D90 / "canonical_compression_status_obligations_90.md",
        "Canonical Compression Status Obligations 90",
        """
## status after congruence refinement

`canonical_compression_status_obligations_domain_normal_form_open`

The previous status obligations are refined by
`canonical_compression_status_congruence_refinement_obligations_90.md`.

Runtime table:
`branch_4/90/runtime/canonical_compression_status_obligations_90.tsv`.
""",
    )
    write_table(RUNTIME / "canonical_compression_status_obligations_90.tsv", obligation_header, obligation_rows)

    write_md(
        D90 / "canonical_compression_status_sublemma_proofs_90.md",
        "Canonical Compression Status Sublemma Proofs 90",
        """
## status after congruence refinement

`canonical_compression_status_sublemma_proofs_domain_normal_form_open`

Language and failure classification are current-scope proved. Payload
congruence, status-domain transfer, normal-form transfer, and conditional
preservation are proof-sketch/proof-ready. Status predicate determination and
valid reduced-status fallback remain blocked.

Runtime proofs:
`branch_4/90/runtime/canonical_compression_status_sublemma_proofs_90.tsv`.
""",
    )
    write_table(RUNTIME / "canonical_compression_status_sublemma_proofs_90.tsv", sublemma_header, sublemma_rows)


def write_rollup_docs() -> None:
    write_md(
        D90 / "status_preservation_congruence_skeleton_90.md",
        "Status Preservation Congruence Skeleton 90",
        f"""
## status after canonical-compression congruence refinement

`{STATUS_CONGRUENCE}`

The common status language remains unchanged. This round refines the
canonical-compression row into payload, status-domain, normal-form,
status-predicate, reduced-status, and named escape obligations. Project-to-active
and coordinate-contraction domain/normal-form blockers remain open, and
family-chain absorption still needs source-target alignment and residual
measure. No support9+ scan was run.

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
            ("conclusion", "Canonical compression is refined to domain/normal-form/status-predicate open skeleton; project-to-active, contract-equivalent, family-chain alignment, and residual measure remain open."),
            ("project_to_active_proof_status", "proof_ready_skeleton_project_to_active_locality_status_domain_open"),
            ("coordinate_contraction_proof_status", "proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open"),
            ("canonical_compression_proof_status", CANONICAL_SKELETON),
            ("family_chain_absorption_proof_status", "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open"),
            ("higher_support_escape_proof_status", HIGHER_SUPPORT),
            ("status_congruence_obligation_count", "13"),
            ("proved_under_current_scope_sublemma_count", "4"),
            ("proof_sketch_only_sublemma_count", "2"),
            ("blocked_sublemma_count", "1"),
            ("proof_ready_open_sublemma_count", "3"),
            ("missing_steps", "project-to-active status-domain invariance and normal-form transfer;contract-equivalent quotient status-domain/normal-form/status-predicate transfer;canonical-motif domain/normal/status predicate transfer;family-chain source-target alignment and residual measure decrease;higher-support bound only after operation proofs close"),
            ("exact_caveat", "Does not prove full status preservation, support-bound completion, support8 sufficiency, higher-support necessity, or the full general theorem; no support9 scan was run."),
            ("final_status", STATUS_CONGRUENCE),
            ("next_blocker", NEXT1),
        ],
    )

    write_md(
        D90 / "support_reduction_operation_status_table_90.md",
        "Support Reduction Operation Status Table 90",
        f"""
## status after canonical-compression congruence refinement

`operation_status_table_canonical_refined_domain_normal_form_open_remaining_project_contract_alignment_measure_open`

Canonical motif compression is now refined from a generic motif status
congruence blocker into payload congruence, motif status-domain transfer,
motif normal-form transfer, status predicate determination, and valid
reduced-status fallback. It is still not a proved operation.

Runtime table:
`branch_4/90/runtime/support_reduction_operation_status_table_90.tsv`.
""",
    )
    write_table(RUNTIME / "support_reduction_operation_status_table_90.tsv", operation_header, operation_rows)

    write_md(
        D90 / "support_reduction_step_skeleton_90.md",
        "Support Reduction Step Skeleton 90",
        f"""
## status after canonical-compression congruence refinement

`{SUPPORT_REDUCTION}`

The support-growth partition is unchanged in shape, but the canonical
compression branch now carries explicit status-domain, normal-form,
status-predicate, and reduced-status obligations. Family-chain source alignment
is the next operation-specific target.

Runtime skeleton:
`branch_4/90/runtime/support_reduction_step_skeleton_90.tsv`.
""",
    )
    write_metric(
        RUNTIME / "support_reduction_step_skeleton_90.tsv",
        [
            ("lemma_name", "support_growth_partition"),
            ("exact_statement", "Support>8 normal minimal witnesses are partitioned into ready reduction, project-to-active payload-proved/status-domain-open branch, coordinate-contraction domain/normal-form-open branch, canonical-compression domain/normal-form-open branch, family-chain alignment/measure-open branch, downstream support8 capture, named operation blocker, or true higher-support escape after operation proofs close."),
            ("assumption_count", "10"),
            ("conclusion", "Canonical compression is refined but not proved; family-chain alignment/measure, project status-domain, and coordinate-contraction domain/normal-form remain open."),
            ("case_split", "delete_redundant_support_coordinate_selected;project_to_active_support_payload_proved_status_domain_open;contract_equivalent_support_coordinates_domain_normal_form_open;canonical_motif_compression_domain_normal_form_open;family_chain_absorption_status_proof_ready_refutation_measure_open;frontier_tail_captured_after_reduction;irreducible_higher_support_escape_after_operation_proofs"),
            ("measure_decrease_status", "delete/project/contraction/compression measure proved under preconditions;absorption residual measure open"),
            ("project_to_active_status_status", "proof_ready_skeleton_project_to_active_locality_status_domain_open"),
            ("contract_equivalent_status_status", "proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open"),
            ("canonical_compression_status_status", CANONICAL_SKELETON),
            ("missing_steps", "project-to-active status-domain invariance and normal-form transfer;contract-equivalent quotient domain/normal/status predicate;canonical-motif domain/normal/status predicate;family-chain absorption source-target alignment and residual measure;higher-support theoretical bound only after operation proofs close"),
            ("exact_caveat", "no_support9_scan_full_support8_sufficiency_not_proved_canonical_compression_not_fully_proved_and_full_general_theorem_not_proved"),
            ("final_status", SUPPORT_REDUCTION),
            ("next_blocker", NEXT1),
        ],
    )

    write_md(
        D90 / "support_bound_lemma_skeleton_90.md",
        "Support Bound Lemma Skeleton 90",
        f"""
## status after canonical-compression congruence refinement

`{SUPPORT_BOUND}`

Support-bound remains proof-ready only as a skeleton. The limited bridge theorem
is still proved under current scope. Canonical compression is refined but still
blocked by motif status-domain/normal-form/status predicate and valid
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
            ("conclusion", "Support-bound remains proof-ready; canonical-compression congruence is refined but domain/normal/status predicate and reduced-status obligations remain open."),
            ("support8_case", "limited_bridge_theorem_proved_under_current_scope"),
            ("delete_redundant_case", "proved_under_current_scope_under_redundancy_precondition"),
            ("project_to_active_case", "proof_ready_skeleton_project_to_active_locality_status_domain_open"),
            ("coordinate_contraction_case", "proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open"),
            ("canonical_compression_case", CANONICAL_SKELETON),
            ("family_chain_absorption_case", "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open"),
            ("higher_support_escape", HIGHER_SUPPORT),
            ("support_reduction_step_status", SUPPORT_REDUCTION),
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("measure_status", "delete/project/contraction/compression measure ready under preconditions;absorption residual measure open"),
            ("missing_steps", "status-domain invariance;contract-equivalent quotient congruence;canonical-motif congruence domain/normal/status predicate;family-chain source-target alignment;residual absorption measure;higher-support bound after operation proofs close"),
            ("exact_caveat", "no_support_bound_completion_no_support8_sufficiency_no_support9_scan_no_full_general_theorem"),
            ("final_status", SUPPORT_BOUND),
            ("next_blocker", NEXT1),
        ],
    )

    write_md(
        D90 / "higher_support_escape_interface_90.md",
        "Higher Support Escape Interface 90",
        f"""
## status after canonical-compression congruence refinement

`{HIGHER_SUPPORT}`

Named operation blockers remain separated from true higher-support theorem-data
needs. Canonical compression is now a domain/normal-form/status-predicate-open
proof-ready skeleton, so higher-support continues to defer until
project-to-active status-domain transfer, contract-equivalent quotient transfer,
family-chain alignment, and residual measure obligations are resolved.

Runtime interface:
`branch_4/90/runtime/higher_support_escape_interface_90.tsv`.
""",
    )
    write_metric(RUNTIME / "higher_support_escape_interface_90.tsv", [
        ("higher_support_escape_status", HIGHER_SUPPORT),
        ("operation_blockers_first", "canonical compression refined; project-to-active, contract-equivalent, family-chain alignment, and residual measure remain operation-local blockers"),
        ("support9_scan", "not_run"),
        ("current_decision", "higher_support_deferred_until_operation_proofs_close_or_named_true_escape_remains"),
        ("next_action", NEXT1),
    ])

    write_md(
        D90 / "support_reduction_operations_90.md",
        "Support Reduction Operations 90",
        f"""
## status after canonical-compression congruence refinement

Current operation status:

- `delete_redundant_support_coordinate`: selected redundancy case is current-scope proved under its precondition.
- `project_to_active_support`: payload locality proved; status branch is `proof_ready_skeleton_project_to_active_locality_status_domain_open`.
- `contract_equivalent_support_coordinates`: support-size decrease under nontrivial accepted equivalence remains proved, while status branch is `proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open`.
- `canonical_motif_compression`: lexicographic decrease remains available, while status branch is now `{CANONICAL_SKELETON}` with motif status-domain, motif normal-form, status predicate, and valid reduced-status obligations open.
- `family_chain_absorption_reduction`: refutation/reduction/escape skeleton is proof-ready with source-target alignment and residual measure open.

Runtime table:
`branch_4/90/runtime/support_reduction_operations_90.tsv`.
""",
    )
    write_table(RUNTIME / "support_reduction_operations_90.tsv", operation_header, operation_rows)


def write_higher_support_after_round() -> None:
    header = [
        "check_key",
        "question",
        "result",
        "evidence",
        "effect",
        "current_status",
        "next_action",
    ]
    rows = [
        {
            "check_key": "canonical_motif_status_congruence_closed",
            "question": "Did canonical-motif status congruence close?",
            "result": "no_proof_ready_domain_normal_form_open",
            "evidence": "canonical_compression_status_congruence_refinement_skeleton_90.tsv",
            "effect": "operation blocker narrowed but not closed",
            "current_status": CANONICAL_SKELETON,
            "next_action": NEXT1,
        },
        {
            "check_key": "compression_support_gt8_escape_reduced",
            "question": "Did compression-related support>8 escape shrink?",
            "result": "partially_classified_not_eliminated",
            "evidence": "canonical_congruence_refinement_fingerprint_90.tsv",
            "effect": "hidden status failure split into domain, normal-form, status predicate, smaller witness, and named escape cases",
            "current_status": "classified_operation_blocker",
            "next_action": NEXT1,
        },
        {
            "check_key": "project_to_active_priority",
            "question": "Is project-to-active locality still a priority?",
            "result": "yes_status_domain_open",
            "evidence": "project_to_active_status_locality_skeleton_90.tsv",
            "effect": "shared status-domain/normal-form blocker remains",
            "current_status": "proof_ready_skeleton_project_to_active_locality_status_domain_open",
            "next_action": NEXT2,
        },
        {
            "check_key": "coordinate_contraction_priority",
            "question": "Is coordinate-contraction domain/normal-form still a priority?",
            "result": "yes_quotient_domain_normal_form_open",
            "evidence": "contract_equivalent_status_congruence_refinement_skeleton_90.tsv",
            "effect": "coordinate branch remains proof-ready/open",
            "current_status": "proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open",
            "next_action": NEXT3,
        },
        {
            "check_key": "family_chain_alignment_priority",
            "question": "Is family-chain source alignment/residual measure now the highest value next operation?",
            "result": "yes_next_matrix_first",
            "evidence": "general_gap_bridge_next_action_matrix_90.tsv",
            "effect": "next exact target moves to source alignment before residual measure",
            "current_status": "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open",
            "next_action": NEXT1,
        },
        {
            "check_key": "higher_support_deferred",
            "question": "Is higher-support necessity still deferred?",
            "result": "yes",
            "evidence": "higher_support_escape_interface_90.tsv",
            "effect": "no support9+ scan and no higher-support theorem-data promotion",
            "current_status": HIGHER_SUPPORT,
            "next_action": "higher_support_necessity_recheck_after_operation_proofs",
        },
        {
            "check_key": "limited_to_broader_generalization_ready",
            "question": "Can limited-to-broader generalization be completed now?",
            "result": "not_yet_operation_proofs_open",
            "evidence": "support_bound_lemma_skeleton_90.tsv",
            "effect": "generalization plan remains downstream of operation-local proofs",
            "current_status": GENERAL_READY,
            "next_action": NEXT1,
        },
    ]
    write_md(
        D90 / "higher_support_necessity_after_canonical_congruence_refinement_90.md",
        "Higher Support Necessity After Canonical Congruence Refinement 90",
        f"""
## status

`{HIGHER_SUPPORT}`

Canonical-congruence refinement reduces the compression blocker to
status-domain transfer, motif normal-form transfer, status predicate
determination, and valid reduced-status fallback. This does not establish that
remaining witnesses require higher-support theorem-data. Operation-specific
proof obligations remain open, so support9+ work stays deferred.

Runtime table:
`branch_4/90/runtime/higher_support_necessity_after_canonical_congruence_refinement_90.tsv`.
""",
    )
    write_table(RUNTIME / "higher_support_necessity_after_canonical_congruence_refinement_90.tsv", header, rows)


def write_general_bridge_docs() -> None:
    obligation_rows2 = [
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
            "existing_evidence": "support8 closure;limited bridge theorem;project-to-active locality skeleton;contract-equivalent refined skeleton;canonical-compression refined skeleton;family-chain absorption status skeleton;support-bound skeleton;status-congruence bridge",
            "missing_evidence": "project status-domain invariance;contract-equivalent quotient transfer;canonical-motif transfer;family-chain source-target alignment;residual absorption measure",
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
            "description": "Show runtime canonical row equality matches mathematical equivalence and motif compression semantics.",
            "required_for_limited_support8_statement": "1",
            "required_for_bounded_shell_statement": "1",
            "required_for_full_general_statement": "1",
            "existing_evidence": "canonical runtime equality;payload semantics;canonical compression refined skeleton;status bridge",
            "missing_evidence": "canonical-motif counterexample-status congruence and canonical payload mathematical equivalence",
            "current_status": CONGRUENCE_STATUS,
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
    ]
    obligation_header2 = [
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
    ]
    write_md(
        D90 / "general_gap_bridge_obligation_inventory_90.md",
        "General Gap Bridge Obligation Inventory 90",
        f"""
## status after canonical-compression congruence refinement

`{SUPPORT_BOUND}`

The general bridge inventory now records canonical motif compression as
first-class refined but still open on motif status-domain, normal-form, status
predicate, and valid reduced-status obligations. Full general theorem proof is
not claimed.

Runtime inventory:
`branch_4/90/runtime/general_gap_bridge_obligation_inventory_90.tsv`.
""",
    )
    write_table(RUNTIME / "general_gap_bridge_obligation_inventory_90.tsv", obligation_header2, obligation_rows2)

    dep_header = [
        "edge_key",
        "source",
        "target",
        "dependency_type",
        "current_status",
        "blocks",
        "next_action",
        "reason",
    ]
    dep_rows = [
        {
            "edge_key": "canonical_payload_to_status",
            "source": "canonical_motif_payload_congruence_refined",
            "target": "canonical_motif_counterexample_status_congruence",
            "dependency_type": "insufficient_alone",
            "current_status": "payload_proof_sketch_domain_normal_form_open",
            "blocks": "canonical_compression_status_congruence_refinement",
            "next_action": NEXT1,
            "reason": "Payload congruence does not determine counterexample status without domain and normal-form transfer.",
        },
        {
            "edge_key": "canonical_domain_to_status",
            "source": "canonical_compression_status_domain_transfer",
            "target": "canonical_motif_counterexample_status_congruence",
            "dependency_type": "required_transfer",
            "current_status": DOMAIN_STATUS,
            "blocks": "canonical status preservation",
            "next_action": "shared_status_domain_refinement",
            "reason": "Compressed status predicate comparison needs compatible status domains.",
        },
        {
            "edge_key": "canonical_normal_to_status",
            "source": "canonical_compression_normal_form_transfer",
            "target": "canonical_motif_counterexample_status_congruence",
            "dependency_type": "required_transfer",
            "current_status": NORMAL_STATUS,
            "blocks": "canonical status preservation",
            "next_action": "shared_normal_form_refinement",
            "reason": "Compressed witness must be status-domain eligible after normalization.",
        },
        {
            "edge_key": "canonical_to_support_reduction",
            "source": "canonical_compression_status_congruence_refinement_skeleton",
            "target": "support_reduction_step_skeleton",
            "dependency_type": "operation_branch",
            "current_status": CANONICAL_SKELETON,
            "blocks": SUPPORT_REDUCTION,
            "next_action": NEXT1,
            "reason": "Canonical branch remains proof-ready/open, but hidden cases are now classified.",
        },
        {
            "edge_key": "family_alignment_next",
            "source": "family_chain_absorption_status_skeleton",
            "target": "support_bound_lemma_skeleton",
            "dependency_type": "next_operation_blocker",
            "current_status": "source_alignment_and_residual_measure_open",
            "blocks": SUPPORT_BOUND,
            "next_action": NEXT1,
            "reason": "After canonical refinement, family-chain source alignment is the clearest operation-local next target.",
        },
    ]
    write_md(
        D90 / "general_gap_bridge_dependency_graph_90.md",
        "General Gap Bridge Dependency Graph 90",
        f"""
## status after canonical-compression congruence refinement

`{STATUS_CONGRUENCE}`

The dependency graph records canonical compression as a refined proof-ready
operation branch, not a proved status-preservation theorem.

Runtime graph:
`branch_4/90/runtime/general_gap_bridge_dependency_graph_90.tsv`.
""",
    )
    write_table(RUNTIME / "general_gap_bridge_dependency_graph_90.tsv", dep_header, dep_rows)

    lemma_header2 = [
        "lemma_key",
        "statement",
        "current_status",
        "existing_evidence",
        "missing_hypothesis",
        "proof_value_0_100",
        "risk_0_100",
        "recommended_next_action",
    ]
    lemma_rows2 = [
        {
            "lemma_key": "canonical_motif_status_preserved_under_refined_congruence_or_reduced_or_escape",
            "statement": "Canonical motif compression preserves status under refined congruence or becomes smaller witness/named escape.",
            "current_status": CANONICAL_SKELETON,
            "existing_evidence": "canonical_congruence_refinement_fingerprint_90.tsv",
            "missing_hypothesis": "motif domain transfer;motif normal-form transfer;status predicate determination;valid reduced status",
            "proof_value_0_100": "84",
            "risk_0_100": "66",
            "recommended_next_action": NEXT1,
        },
        {
            "lemma_key": "family_chain_absorption_source_alignment",
            "statement": "Lifted target refutation aligns with source counterexample payload/status.",
            "current_status": "proof_ready_skeleton_family_chain_absorption_status_refutation_measure_open",
            "existing_evidence": "family_chain_absorption_status_skeleton_90.tsv",
            "missing_hypothesis": "source-target payload/status alignment",
            "proof_value_0_100": "86",
            "risk_0_100": "70",
            "recommended_next_action": NEXT1,
        },
        {
            "lemma_key": "project_to_active_status_domain_transfer",
            "statement": "Active projection preserves or refines status-domain and normal form.",
            "current_status": "proof_ready_skeleton_project_to_active_locality_status_domain_open",
            "existing_evidence": "project_to_active_status_locality_skeleton_90.tsv",
            "missing_hypothesis": "status-domain invariance;normal-form transfer;valid reduced-status fallback",
            "proof_value_0_100": "85",
            "risk_0_100": "62",
            "recommended_next_action": NEXT2,
        },
        {
            "lemma_key": "contract_equivalent_domain_normal_form_refinement",
            "statement": "Coordinate contraction quotient preserves/refines status-domain and normal form.",
            "current_status": "proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open",
            "existing_evidence": "contract_equivalent_status_congruence_refinement_skeleton_90.tsv",
            "missing_hypothesis": "quotient status-domain invariance;quotient normal-form transfer;valid reduced-status fallback",
            "proof_value_0_100": "83",
            "risk_0_100": "64",
            "recommended_next_action": NEXT3,
        },
    ]
    write_md(
        D90 / "general_gap_bridge_lemma_candidates_90.md",
        "General Gap Bridge Lemma Candidates 90",
        f"""
## status after canonical-compression congruence refinement

`{GENERAL_READY}`

Canonical-compression refinement is now a first-class lemma candidate with
domain/normal-form/status-predicate caveats. The next recommended candidate is
family-chain absorption source alignment.

Runtime candidates:
`branch_4/90/runtime/general_gap_bridge_lemma_candidates_90.tsv`.
""",
    )
    write_table(RUNTIME / "general_gap_bridge_lemma_candidates_90.tsv", lemma_header2, lemma_rows2)

    write_md(
        D90 / "limited_general_gap_bridge_skeleton_90.md",
        "Limited General Gap Bridge Skeleton 90",
        f"""
## status after canonical-compression congruence refinement

`{SUPPORT_BOUND}`

The limited bridge theorem remains proved under current scope. This round does
not expand it into a full general theorem. Canonical compression is refined to a
proof-ready domain/normal-form/status-predicate-open skeleton; family-chain
source alignment, project-to-active status-domain transfer, contract-equivalent
quotient transfer, residual measure, and higher-support bound remain outside
the completed limited proof.

Runtime skeleton:
`branch_4/90/runtime/limited_general_gap_bridge_skeleton_90.tsv`.
""",
    )
    write_metric(RUNTIME / "limited_general_gap_bridge_skeleton_90.tsv", [
        ("limited_bridge_status", "limited_bridge_theorem_proved_under_current_scope"),
        ("canonical_compression_refinement_status", CANONICAL_SKELETON),
        ("support_reduction_step_status", SUPPORT_REDUCTION),
        ("support_bound_status", SUPPORT_BOUND),
        ("general_readiness", GENERAL_READY),
        ("next_target", NEXT1),
        ("exact_caveat", "limited_bridge_not_general_theorem_no_support9_scan_no_full_canonical_compression_proof"),
    ])

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
            "resolves": "source_target_payload_status_alignment",
            "prerequisite_status": "family_chain_absorption_status_refutation_measure_open",
            "readiness_score_0_100": "82",
            "proof_value_0_100": "86",
            "engineering_cost_0_100": "66",
            "risk_0_100": "70",
            "dependency_clarity_0_100": "84",
            "expected_progress_value_0_100": "83",
            "recommended_order": "1",
            "final_recommendation": NEXT1,
            "reason": "Canonical compression is now first-class refined; source-target alignment is the clearest remaining operation-specific blocker before residual absorption measure.",
        },
        {
            "action_key": NEXT2,
            "resolves": "shared_status_domain_and_normal_form_transfer",
            "prerequisite_status": "project_to_active_payload_proved_status_domain_open",
            "readiness_score_0_100": "80",
            "proof_value_0_100": "85",
            "engineering_cost_0_100": "61",
            "risk_0_100": "62",
            "dependency_clarity_0_100": "83",
            "expected_progress_value_0_100": "81",
            "recommended_order": "2",
            "final_recommendation": NEXT2,
            "reason": "Status-domain and normal-form transfer remain shared blockers exposed by project-to-active and compression.",
        },
        {
            "action_key": NEXT3,
            "resolves": "quotient_domain_normal_form_transfer",
            "prerequisite_status": "contract_equivalent_congruence_domain_normal_form_open",
            "readiness_score_0_100": "78",
            "proof_value_0_100": "83",
            "engineering_cost_0_100": "63",
            "risk_0_100": "64",
            "dependency_clarity_0_100": "82",
            "expected_progress_value_0_100": "79",
            "recommended_order": "3",
            "final_recommendation": NEXT3,
            "reason": "Coordinate contraction is already refined and now needs a focused quotient domain/normal-form round.",
        },
        {
            "action_key": "residual_absorption_measure_decrease",
            "resolves": "residual_absorption_measure",
            "prerequisite_status": "family_chain_absorption_source_alignment_open",
            "readiness_score_0_100": "73",
            "proof_value_0_100": "84",
            "engineering_cost_0_100": "68",
            "risk_0_100": "73",
            "dependency_clarity_0_100": "78",
            "expected_progress_value_0_100": "76",
            "recommended_order": "4",
            "final_recommendation": "residual_absorption_measure_decrease",
            "reason": "Measure decrease is mandatory, but source alignment should be clarified first.",
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
            "final_recommendation": "higher_support_necessity_recheck_after_operation_proofs",
            "reason": "Higher-support remains deferred until operation-local proof obligations close or fail as named true escapes.",
        },
        {
            "action_key": "limited_to_broader_generalization_plan",
            "resolves": "generalization_contract",
            "prerequisite_status": "operation_proofs_open_canonical_refined",
            "readiness_score_0_100": "72",
            "proof_value_0_100": "80",
            "engineering_cost_0_100": "55",
            "risk_0_100": "61",
            "dependency_clarity_0_100": "84",
            "expected_progress_value_0_100": "74",
            "recommended_order": "6",
            "final_recommendation": "limited_to_broader_generalization_plan_after_operation_refinements",
            "reason": "Planning can proceed, but proof completion still depends on operation-local congruence, alignment, and residual measure blockers.",
        },
        {
            "action_key": "support_bound_completion",
            "resolves": "support_bound_completion",
            "prerequisite_status": SUPPORT_BOUND,
            "readiness_score_0_100": "64",
            "proof_value_0_100": "83",
            "engineering_cost_0_100": "64",
            "risk_0_100": "72",
            "dependency_clarity_0_100": "80",
            "expected_progress_value_0_100": "70",
            "recommended_order": "7",
            "final_recommendation": "support_bound_completion_after_operation_refinements",
            "reason": "Premature until operation-local congruence, source-alignment, and residual absorption measure blockers close.",
        },
    ]
    write_md(
        D90 / "general_gap_bridge_next_action_matrix_90.md",
        "General Gap Bridge Next Action Matrix 90",
        f"""
## status after canonical-compression congruence refinement

`{GENERAL_READY}`

The next exact target is `{NEXT1}`. This is still not a full general theorem
proof and not a support9+ scan.

Runtime matrix:
`branch_4/90/runtime/general_gap_bridge_next_action_matrix_90.tsv`.
""",
    )
    write_table(RUNTIME / "general_gap_bridge_next_action_matrix_90.tsv", action_header, action_rows)


def write_baseline_docs() -> None:
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

This round is `canonical_compression_status_congruence_refinement`. It refines
canonical motif compression status congruence into payload, status-domain,
normal-form, counterexample-status predicate, smaller-witness, and named escape
branches.

It does not prove the full general theorem, does not prove
`canonical_motif_compression` fully, and does not run support9+.

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
- selected canonical-congruence statement: `{SELECTED}`
- status-domain transfer: `{DOMAIN_STATUS}`
- normal-form transfer: `{NORMAL_STATUS}`
- canonical motif congruence refinement: `{CONGRUENCE_STATUS}`
- canonical-congruence refinement skeleton: `{CANONICAL_SKELETON}`
- canonical-compression operation status: `{CANONICAL_STATUS}`
- status-congruence skeleton: `{STATUS_CONGRUENCE}`
- support reduction skeleton: `{SUPPORT_REDUCTION}`
- support-bound skeleton: `{SUPPORT_BOUND}`
- higher-support necessity: `{HIGHER_SUPPORT}`
- general theorem readiness: `{GENERAL_READY}`
- next target: `{NEXT1}`

4. non-claims
- no full general theorem proof
- no full canonical-compression proof
- no support9+ scan
- no BOJ solver implementation
""",
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

## proof-boundary status after canonical-compression congruence refinement

- canonical-compression operation status: `{CANONICAL_STATUS}`
- status-domain transfer: `{DOMAIN_STATUS}`
- normal-form transfer: `{NORMAL_STATUS}`
- canonical motif congruence refinement: `{CONGRUENCE_STATUS}`
- status-congruence bridge: `{STATUS_CONGRUENCE}`
- support reduction skeleton: `{SUPPORT_REDUCTION}`
- support-bound skeleton: `{SUPPORT_BOUND}`
- higher-support necessity: `{HIGHER_SUPPORT}`
- general theorem readiness: `{GENERAL_READY}`
- next exact target: `{NEXT1}`

This certificate does not prove the full general theorem, full canonical
compression, or support8 sufficiency.
""",
    )
    write_metric(RUNTIME / "current_support8_closure_certificate_90.tsv", [
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
        ("contract_equivalent_operation_status", "partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open"),
        ("canonical_compression_operation_status", CANONICAL_STATUS),
        ("status_congruence_bridge_status", STATUS_CONGRUENCE),
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("general_theorem_readiness", GENERAL_READY),
        ("support_bound_next_exact_target", NEXT1),
        ("caveat", "readiness_boundary_not_general_theorem_or_boj_solver"),
    ])

    root_summary = f"""
Latest round: `canonical_compression_status_congruence_refinement`.

- selected canonical-congruence statement: `{SELECTED}`
- status-domain transfer: `{DOMAIN_STATUS}`
- normal-form transfer: `{NORMAL_STATUS}`
- canonical motif congruence refinement: `{CONGRUENCE_STATUS}`
- canonical-congruence skeleton: `{CANONICAL_SKELETON}`
- canonical-compression operation status: `{CANONICAL_STATUS}`
- status-congruence skeleton: `{STATUS_CONGRUENCE}`
- support reduction skeleton: `{SUPPORT_REDUCTION}`
- support-bound lemma skeleton: `{SUPPORT_BOUND}`
- higher-support necessity: `{HIGHER_SUPPORT}`
- general theorem readiness: `{GENERAL_READY}`
- next action order: `{NEXT1}`, `{NEXT2}`, `{NEXT3}`

The support8 lock remains `support8_authoritative_completion_locked`; required
docs/artifacts remain `39/39` and `8/8`; top-level provenance remains fresh
`16`, imported `0`, mixed `0`, archival `3`; family-chain lower layers remain
total `7`, fresh `7`, imported `0`, caveat closed `1`; and the limited bridge
theorem remains `limited_bridge_theorem_proved_under_current_scope`.

This does not prove the full general theorem, does not prove
`canonical_motif_compression` fully, does not prove support8 sufficiency, and
does not run support9+.
"""
    for name in [
        "project_status_summary.md",
        "current_workspace_reality_check.md",
        "current_status_authoritative_longform.md",
        "authoritative_completion_to_100_plan_longform.md",
        "theorem_data_promotion_to_100_plan_longform.md",
        "mathematical_progress_to_100_plan_longform.md",
        "progress_history_1_to_85_longform.md",
    ]:
        upsert_section(B4 / name, "canonical-compression status congruence refinement round", root_summary)


def main() -> None:
    RUNTIME.mkdir(parents=True, exist_ok=True)
    write_core_refinement_docs()
    write_existing_canonical_status_docs()
    write_rollup_docs()
    write_higher_support_after_round()
    write_general_bridge_docs()
    write_baseline_docs()


if __name__ == "__main__":
    main()
