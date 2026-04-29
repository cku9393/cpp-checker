#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


D90 = Path(__file__).resolve().parent
B4 = D90.parent
RUNTIME = D90 / "runtime"

SELECTED = "source_alignment_or_smaller_witness_or_escape"
ALIGNMENT_SEMANTICS = "family_chain_absorption_source_target_alignment_semantics_contract_ready"
PAYLOAD_ALIGNMENT = "family_chain_absorption_payload_alignment_proof_ready_source_target_payload_open"
DOMAIN_ALIGNMENT = "family_chain_absorption_status_domain_alignment_proof_ready_source_target_domain_open"
NORMAL_ALIGNMENT = "family_chain_absorption_normal_form_alignment_proof_ready_source_target_normal_form_open"
REFUTATION_TRANSFER = "lifted_refutation_to_source_refutation_payload_domain_normal_form_open"
SOURCE_ALIGNMENT_SKELETON = "proof_ready_skeleton_family_chain_source_alignment_payload_domain_normal_form_open"
FAMILY_STATUS = "partial_family_chain_absorption_source_alignment_proof_ready_residual_measure_open"
STATUS_CONGRUENCE = "partial_status_congruence_family_alignment_refined_payload_domain_normal_open_remaining_residual_project_contract_canonical_open"
SUPPORT_REDUCTION = "partition_ready_family_alignment_refined_payload_domain_normal_open_remaining_residual_project_contract_canonical_open"
SUPPORT_BOUND = "proof_ready_skeleton_family_alignment_refined_payload_domain_normal_open_remaining_residual_project_contract_canonical_open"
HIGHER_SUPPORT = "higher_support_deferred_after_family_chain_source_alignment_payload_domain_normal_open"
GENERAL_READY = "ready_for_residual_absorption_measure_decrease"
NEXT1 = "residual_absorption_measure_decrease"
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
    "source_scope",
    "lifted_target_scope",
    "alignment_scope",
    "uses_absorption_status_semantics",
    "uses_refutation_lemma",
    "uses_payload_correspondence",
    "uses_status_domain_transfer",
    "uses_normal_form_transfer",
    "uses_measure_decrease",
    "uses_higher_support_escape",
    "selected_for_attempt",
    "risk",
    "reason",
]

scope_rows = [
    {
        "statement_key": "lifted_target_refutation_aligns_with_source_payload",
        "formal_statement": "For recognized family-chain source form, lifted target refutation addresses the source payload obstruction through the source-target payload correspondence.",
        "assumptions": "recognized source form;fresh lower-layer target package;payload correspondence map defined",
        "conclusion": "target payload refutation is relevant to the source payload obstruction",
        "source_scope": "recognized family-chain source witnesses",
        "lifted_target_scope": "fresh lower-layer lifted targets",
        "alignment_scope": "payload alignment",
        "uses_absorption_status_semantics": "1",
        "uses_refutation_lemma": "1",
        "uses_payload_correspondence": "1",
        "uses_status_domain_transfer": "0",
        "uses_normal_form_transfer": "0",
        "uses_measure_decrease": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "Existing payload correspondence is contract-ready but does not by itself prove source counterexample refutation.",
    },
    {
        "statement_key": "lifted_target_refutation_refutes_source_counterexample",
        "formal_statement": "A lifted target refutation transfers to a source counterexample refutation when payload, status-domain, and normal-form alignment all hold.",
        "assumptions": "lifted target refutation;payload alignment;status-domain alignment;normal-form alignment",
        "conclusion": "source counterexample status is refuted",
        "source_scope": "recognized source counterexample candidates",
        "lifted_target_scope": "lifted absorption targets with available refutation",
        "alignment_scope": "refutation transfer",
        "uses_absorption_status_semantics": "1",
        "uses_refutation_lemma": "1",
        "uses_payload_correspondence": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_measure_decrease": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "high",
        "reason": "This is the central open transfer; current artifacts prove the lifted-target side, not the source-side transfer.",
    },
    {
        "statement_key": "source_target_status_alignment_under_family_chain_absorption",
        "formal_statement": "Source and lifted target status-domain, normal-form, and payload alignment are sufficient to compare counterexample-status predicates.",
        "assumptions": "recognized source form;aligned payload;aligned domains;aligned normal forms",
        "conclusion": "status predicate comparison is well-formed and can support source refutation",
        "source_scope": "source witness status language",
        "lifted_target_scope": "lifted target status language",
        "alignment_scope": "status-domain/normal-form/payload alignment",
        "uses_absorption_status_semantics": "1",
        "uses_refutation_lemma": "1",
        "uses_payload_correspondence": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_measure_decrease": "0",
        "uses_higher_support_escape": "0",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "Separates the status-comparison preconditions from residual measure decrease.",
    },
    {
        "statement_key": SELECTED,
        "formal_statement": "If source-target alignment holds, lifted target refutation refutes the source counterexample; if alignment fails, the branch is classified as a smaller witness requirement or a named escape.",
        "assumptions": "recognized source form;lifted target package;alignment classifier;status language",
        "conclusion": "source refutation, smaller-witness branch, or named escape",
        "source_scope": "recognized family-chain source witnesses only",
        "lifted_target_scope": "fresh lower-layer lifted targets",
        "alignment_scope": "payload/domain/normal/refutation alignment plus failure classification",
        "uses_absorption_status_semantics": "1",
        "uses_refutation_lemma": "1",
        "uses_payload_correspondence": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_measure_decrease": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "1",
        "risk": "medium",
        "reason": "This is the safest current statement: it proof-attempts source refutation and keeps non-refuting branches explicit.",
    },
    {
        "statement_key": "full_family_chain_absorption_source_alignment",
        "formal_statement": "All support-growth witnesses admit full source-target alignment under family-chain absorption.",
        "assumptions": "arbitrary support-growth witness",
        "conclusion": "full source-target alignment",
        "source_scope": "all support-growth witnesses",
        "lifted_target_scope": "all possible lifted absorption targets",
        "alignment_scope": "full alignment",
        "uses_absorption_status_semantics": "1",
        "uses_refutation_lemma": "1",
        "uses_payload_correspondence": "1",
        "uses_status_domain_transfer": "1",
        "uses_normal_form_transfer": "1",
        "uses_measure_decrease": "1",
        "uses_higher_support_escape": "1",
        "selected_for_attempt": "0",
        "risk": "high",
        "reason": "Out of scope; lower-layer freshness and recognized-source contracts do not prove arbitrary source-target alignment.",
    },
]


semantics_header = [
    "alignment_component",
    "definition",
    "source_condition",
    "target_condition",
    "alignment_rule",
    "proof_requirement",
    "failure_effect",
    "current_status",
    "caveat",
]

semantics_rows = [
    {
        "alignment_component": "source_witness",
        "definition": "The recognized family-chain support-growth witness before absorption.",
        "source_condition": "recognized source form;source counterexample-status predicate is well-formed",
        "target_condition": "none",
        "alignment_rule": "source side anchors the transfer",
        "proof_requirement": "source recognizer and status semantics",
        "failure_effect": "recognized-source escape",
        "current_status": "proved_under_current_scope",
        "caveat": "Does not cover arbitrary source forms.",
    },
    {
        "alignment_component": "lifted_absorption_target",
        "definition": "The lower-layer target obtained by the family-chain lift package.",
        "source_condition": "source lift map selected",
        "target_condition": "fresh lower-layer target package available",
        "alignment_rule": "target refutation is read through the lift map",
        "proof_requirement": "lifted target availability and refutation lemma",
        "failure_effect": "target-package escape",
        "current_status": "proved_under_current_scope",
        "caveat": "Availability is not source payload alignment.",
    },
    {
        "alignment_component": "source_payload",
        "definition": "The obstruction payload carried by the source counterexample candidate.",
        "source_condition": "payload extracted from recognized source form",
        "target_condition": "none",
        "alignment_rule": "source payload is the object to be refuted",
        "proof_requirement": "payload semantics for source form",
        "failure_effect": "payload semantics blocker",
        "current_status": "proof_sketch_ready",
        "caveat": "Payload extraction is separate from status-domain transfer.",
    },
    {
        "alignment_component": "target_payload",
        "definition": "The payload appearing in the lifted target refutation.",
        "source_condition": "lift map applied",
        "target_condition": "target theorem-data row selected",
        "alignment_rule": "target payload is a refinement/projection of source payload",
        "proof_requirement": "source-target payload correspondence",
        "failure_effect": "payload mismatch branch",
        "current_status": PAYLOAD_ALIGNMENT,
        "caveat": "Not a coordinate quotient and not canonical motif compression.",
    },
    {
        "alignment_component": "source_status_domain",
        "definition": "Status-domain over which the source counterexample predicate is evaluated.",
        "source_condition": "source status language fixed",
        "target_condition": "none",
        "alignment_rule": "source domain must contain the lifted refutation's interpreted dependency",
        "proof_requirement": "domain dependency extraction",
        "failure_effect": "source-domain blocker",
        "current_status": DOMAIN_ALIGNMENT,
        "caveat": "Payload alignment alone does not prove domain compatibility.",
    },
    {
        "alignment_component": "target_status_domain",
        "definition": "Status-domain used by the lifted target refutation.",
        "source_condition": "lift map defines target",
        "target_condition": "target status language fixed",
        "alignment_rule": "target domain must map into or refine the source domain",
        "proof_requirement": "domain map well-defined",
        "failure_effect": "domain transfer escape",
        "current_status": DOMAIN_ALIGNMENT,
        "caveat": "The target domain may be a refinement rather than identical to the source domain.",
    },
    {
        "alignment_component": "source_normal_form",
        "definition": "Normal form required for source counterexample-status evaluation.",
        "source_condition": "source witness normalized in support notation",
        "target_condition": "none",
        "alignment_rule": "source form must remain comparable after interpreting the target",
        "proof_requirement": "normal-form predicate for recognized sources",
        "failure_effect": "normal-form blocker",
        "current_status": NORMAL_ALIGNMENT,
        "caveat": "Recognized source form is not a full arbitrary normal-form theorem.",
    },
    {
        "alignment_component": "target_normal_form",
        "definition": "Normal form of the lifted absorption target used by the target refutation.",
        "source_condition": "source form lifts to target",
        "target_condition": "target row normalized",
        "alignment_rule": "target normal form must be interpretable in the source normal form",
        "proof_requirement": "normal-form compatibility",
        "failure_effect": "normal-form escape or smaller-witness branch",
        "current_status": NORMAL_ALIGNMENT,
        "caveat": "Lower-layer freshness does not automatically transfer normal form.",
    },
    {
        "alignment_component": "payload_alignment",
        "definition": "The target payload refutation attacks the same obstruction represented by the source payload.",
        "source_condition": "source payload extracted",
        "target_condition": "target payload extracted",
        "alignment_rule": "correspondence or refinement map preserves obstruction meaning",
        "proof_requirement": "payload correspondence sublemma",
        "failure_effect": "smaller witness if payload mismatch yields strict reduction; otherwise named escape",
        "current_status": PAYLOAD_ALIGNMENT,
        "caveat": "This remains proof-ready, not fully proved.",
    },
    {
        "alignment_component": "status_domain_alignment",
        "definition": "The target refutation is meaningful inside the source status-domain.",
        "source_condition": "source status-domain available",
        "target_condition": "target status-domain available",
        "alignment_rule": "domain map preserves predicate dependencies",
        "proof_requirement": "domain alignment sublemma",
        "failure_effect": "domain escape or status-domain refinement target",
        "current_status": DOMAIN_ALIGNMENT,
        "caveat": "Open shared blocker with project/contract/canonical domain transfer.",
    },
    {
        "alignment_component": "normal_form_alignment",
        "definition": "The source and target normal forms support the same refutation interpretation.",
        "source_condition": "source normal form",
        "target_condition": "target normal form",
        "alignment_rule": "normal-form map preserves status-relevant canonical fields",
        "proof_requirement": "normal-form alignment sublemma",
        "failure_effect": "normal-form escape or smaller-witness branch",
        "current_status": NORMAL_ALIGNMENT,
        "caveat": "Open shared blocker with canonical and contract normal-form rounds.",
    },
    {
        "alignment_component": "refutation_alignment",
        "definition": "The lifted target refutation negates the source counterexample predicate after payload/domain/normal-form alignment.",
        "source_condition": "source counterexample-status predicate",
        "target_condition": "lifted target refutation",
        "alignment_rule": "target refutation transfers along the three alignment maps",
        "proof_requirement": "lifted-refutation-to-source-refutation lemma",
        "failure_effect": "refutation transfer blocker",
        "current_status": REFUTATION_TRANSFER,
        "caveat": "This is not yet a completed source refutation proof.",
    },
    {
        "alignment_component": "smaller_witness_fallback",
        "definition": "A failed alignment branch that constructs a strict smaller witness.",
        "source_condition": "alignment mismatch exposes strict residual or payload reduction",
        "target_condition": "candidate reduced witness valid",
        "alignment_rule": "route to residual measure decrease proof",
        "proof_requirement": "residual measure and valid-witness sublemma",
        "failure_effect": "remains open if residual measure is not proved",
        "current_status": "blocked_by_measure_decrease",
        "caveat": "This round separates alignment from residual measure.",
    },
    {
        "alignment_component": "alignment_escape",
        "definition": "A named non-refuting branch when source-target alignment cannot currently be proved.",
        "source_condition": "mismatch not reduced by current smaller-witness construction",
        "target_condition": "target package or domain/normal form outside current contract",
        "alignment_rule": "classify as named escape instead of hiding failure",
        "proof_requirement": "escape naming and no-hidden-case inventory",
        "failure_effect": "higher-support or operation-specific escape remains deferred",
        "current_status": "proved_under_current_scope",
        "caveat": "Escape classification is not a proof of the general theorem.",
    },
]


payload_header = [
    "payload_component",
    "source_payload_component",
    "target_payload_component",
    "correspondence_rule",
    "alignment_status",
    "refutation_relevance",
    "proof_status",
    "missing_hypothesis",
    "caveat",
]

payload_rows = [
    {
        "payload_component": "recognized_source_payload",
        "source_payload_component": "obstruction payload extracted by the source recognizer",
        "target_payload_component": "none",
        "correspondence_rule": "source payload anchors all alignment maps",
        "alignment_status": PAYLOAD_ALIGNMENT,
        "refutation_relevance": "defines what must be refuted",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "source payload extraction completeness",
        "caveat": "Recognized-source only.",
    },
    {
        "payload_component": "lifted_target_payload",
        "source_payload_component": "source obstruction payload",
        "target_payload_component": "fresh lower-layer target payload",
        "correspondence_rule": "target payload is a refinement/projection of source payload through lift map",
        "alignment_status": PAYLOAD_ALIGNMENT,
        "refutation_relevance": "target refutation must attack source obstruction",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "semantic preservation through lift map",
        "caveat": "Freshness is availability, not semantic equivalence.",
    },
    {
        "payload_component": "basis_pair_triple_quad_layers",
        "source_payload_component": "source family-chain lower-layer references",
        "target_payload_component": "fresh basis/pair/triple/quad rows",
        "correspondence_rule": "lower layers provide target packages for lifted refutation",
        "alignment_status": "available_package_alignment",
        "refutation_relevance": "supplies theorem-data target rows",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "7/7 freshness does not prove source-target status alignment.",
    },
    {
        "payload_component": "quintuple_sextuple_septuple_high_layers",
        "source_payload_component": "higher lower-layer source references",
        "target_payload_component": "fresh 57-family target rows",
        "correspondence_rule": "fresh target row availability closes imported-data caveat",
        "alignment_status": "available_package_alignment",
        "refutation_relevance": "prevents lower-layer provenance blocker",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Still not arbitrary support-growth alignment.",
    },
    {
        "payload_component": "obstruction_payload_correspondence",
        "source_payload_component": "source obstruction descriptor",
        "target_payload_component": "target obstruction descriptor",
        "correspondence_rule": "descriptor fields must preserve obstruction meaning",
        "alignment_status": PAYLOAD_ALIGNMENT,
        "refutation_relevance": "needed for target refutation to hit source obstruction",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "fieldwise obstruction preservation",
        "caveat": "Existing correspondence table is a contract, not a completed proof.",
    },
    {
        "payload_component": "counterexample_payload_status_link",
        "source_payload_component": "source payload inside source counterexample predicate",
        "target_payload_component": "target payload inside target refutation",
        "correspondence_rule": "payload correspondence plus domain/normal alignment determines status comparison",
        "alignment_status": PAYLOAD_ALIGNMENT,
        "refutation_relevance": "connects payload to counterexample status",
        "proof_status": "blocked_by_status_domain",
        "missing_hypothesis": "payload+domain+normal implies source status refutation",
        "caveat": "Payload congruence alone is insufficient.",
    },
    {
        "payload_component": "payload_mismatch_smaller_witness",
        "source_payload_component": "unmatched source payload remainder",
        "target_payload_component": "target omission or strict refinement",
        "correspondence_rule": "if mismatch exposes strict residual reduction, route to smaller witness",
        "alignment_status": "blocked_by_measure_decrease",
        "refutation_relevance": "non-refuting branch",
        "proof_status": "blocked_by_measure_decrease",
        "missing_hypothesis": "residual absorption measure decrease",
        "caveat": "This round does not prove residual measure decrease.",
    },
    {
        "payload_component": "payload_mismatch_escape",
        "source_payload_component": "payload outside current recognized correspondence",
        "target_payload_component": "target package outside current semantic map",
        "correspondence_rule": "name as alignment escape",
        "alignment_status": "named_escape_ready",
        "refutation_relevance": "prevents hidden failure",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Named escape is not theorem completion.",
    },
]


alignment_header = [
    "component_key",
    "source_component",
    "target_component",
    "alignment_rule",
    "relation_to_refutation",
    "relation_to_counterexample_status",
    "relation_to_smaller_witness",
    "failure_effect",
    "proof_status",
    "missing_hypothesis",
    "caveat",
]

domain_rows = [
    {
        "component_key": "source_domain_well_formed",
        "source_component": "source counterexample-status domain",
        "target_component": "none",
        "alignment_rule": "source domain is the comparison base",
        "relation_to_refutation": "target refutation must be interpreted in this base",
        "relation_to_counterexample_status": "defines status predicate inputs",
        "relation_to_smaller_witness": "none",
        "failure_effect": "status-domain escape",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "domain extractor completeness",
        "caveat": "Source domain well-formedness is not target alignment.",
    },
    {
        "component_key": "target_domain_well_formed",
        "source_component": "source lift context",
        "target_component": "target refutation domain",
        "alignment_rule": "target domain is available from lifted refutation package",
        "relation_to_refutation": "the lifted refutation lives here",
        "relation_to_counterexample_status": "must map back to source predicate",
        "relation_to_smaller_witness": "none unless domain loss exposes strict residual",
        "failure_effect": "target-domain escape",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Target well-formedness does not imply source compatibility.",
    },
    {
        "component_key": "domain_map",
        "source_component": "source domain dependencies",
        "target_component": "target domain dependencies",
        "alignment_rule": "target dependencies refine or embed into source dependencies",
        "relation_to_refutation": "allows lifted refutation to be read as source predicate denial",
        "relation_to_counterexample_status": "central comparison requirement",
        "relation_to_smaller_witness": "domain loss may create reduced witness obligation",
        "failure_effect": "blocked_by_status_domain",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "fieldwise source-target domain transfer",
        "caveat": "Shared blocker with project-to-active and contraction domain transfer.",
    },
    {
        "component_key": "domain_refinement_case",
        "source_component": "source domain with extra inactive/residual fields",
        "target_component": "target refined domain",
        "alignment_rule": "refined target domain is sufficient if dropped fields are status-local or reduced",
        "relation_to_refutation": "supports source refutation when residual fields are irrelevant",
        "relation_to_counterexample_status": "needs locality/determination proof",
        "relation_to_smaller_witness": "dropped status-relevant field triggers smaller-witness route",
        "failure_effect": "domain-refinement blocker",
        "proof_status": "blocked_by_status_domain",
        "missing_hypothesis": "status-domain irrelevance or reduction sublemma",
        "caveat": "Do not confuse with project-to-active projection.",
    },
    {
        "component_key": "domain_escape_case",
        "source_component": "source domain outside alignment contract",
        "target_component": "target domain outside recognized map",
        "alignment_rule": "name as source-target domain escape",
        "relation_to_refutation": "no direct source refutation claimed",
        "relation_to_counterexample_status": "comparison remains open",
        "relation_to_smaller_witness": "only if residual measure later proves reduction",
        "failure_effect": "named_escape",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Escape keeps higher-support deferred, not solved.",
    },
]

normal_rows = [
    {
        "component_key": "source_normal_form_well_formed",
        "source_component": "recognized source normal form",
        "target_component": "none",
        "alignment_rule": "source normal form anchors status predicate meaning",
        "relation_to_refutation": "target refutation must be interpretable without changing source form",
        "relation_to_counterexample_status": "normal form is a predicate precondition",
        "relation_to_smaller_witness": "normal-form failure may expose smaller witness",
        "failure_effect": "normal-form blocker",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "recognized source normal-form completeness",
        "caveat": "Recognized-source only.",
    },
    {
        "component_key": "target_normal_form_well_formed",
        "source_component": "lift context",
        "target_component": "lifted target normal form",
        "alignment_rule": "target refutation uses a normal-form target row",
        "relation_to_refutation": "target proof is meaningful on target side",
        "relation_to_counterexample_status": "needs transfer to source normal form",
        "relation_to_smaller_witness": "none by itself",
        "failure_effect": "target-normal escape",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Target normality is not source normality.",
    },
    {
        "component_key": "normal_form_map",
        "source_component": "source normal-form fields",
        "target_component": "target normal-form fields",
        "alignment_rule": "target normal-form fields preserve status-relevant source fields",
        "relation_to_refutation": "permits source reading of target contradiction",
        "relation_to_counterexample_status": "prevents predicate shift",
        "relation_to_smaller_witness": "field mismatch may route to residual measure",
        "failure_effect": "blocked_by_normal_form",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "normal-form field preservation",
        "caveat": "Open shared blocker with canonical/contract normal-form transfer.",
    },
    {
        "component_key": "normal_refinement_case",
        "source_component": "source form with residual family-chain fields",
        "target_component": "target refined form",
        "alignment_rule": "refinement is acceptable only when residual fields are refuted or reduced",
        "relation_to_refutation": "supports source contradiction conditionally",
        "relation_to_counterexample_status": "requires status predicate invariance under refinement",
        "relation_to_smaller_witness": "residual fields must decrease if not invariant",
        "failure_effect": "normal-refinement blocker",
        "proof_status": "blocked_by_normal_form",
        "missing_hypothesis": "normal-form invariance or residual measure decrease",
        "caveat": "Not a completed valid-witness transfer.",
    },
    {
        "component_key": "normal_escape_case",
        "source_component": "source normal form outside recognized map",
        "target_component": "target normal form outside transfer contract",
        "alignment_rule": "classify as named normal-form escape",
        "relation_to_refutation": "no source refutation claimed",
        "relation_to_counterexample_status": "status comparison remains open",
        "relation_to_smaller_witness": "future residual measure may discharge",
        "failure_effect": "named_escape",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Escape classification is not theorem completion.",
    },
]


lemma_header = [
    "lemma_component",
    "statement",
    "assumptions",
    "conclusion",
    "lifted_target_refutation_used",
    "payload_alignment_used",
    "status_domain_alignment_used",
    "normal_form_alignment_used",
    "source_counterexample_status_used",
    "proof_status",
    "missing_hypothesis",
    "caveat",
]

lemma_rows = [
    {
        "lemma_component": "lifted_target_refutation_available",
        "statement": "The lifted target refutation package is available for the recognized source form.",
        "assumptions": "fresh lower-layer target package;recognized source lift",
        "conclusion": "target-side refutation may be invoked",
        "lifted_target_refutation_used": "1",
        "payload_alignment_used": "0",
        "status_domain_alignment_used": "0",
        "normal_form_alignment_used": "0",
        "source_counterexample_status_used": "0",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Target refutation is not source refutation.",
    },
    {
        "lemma_component": "payload_refutation_transfer",
        "statement": "The target refutation attacks the source obstruction payload under payload alignment.",
        "assumptions": "source-target payload correspondence",
        "conclusion": "payload-level contradiction is relevant to source obstruction",
        "lifted_target_refutation_used": "1",
        "payload_alignment_used": "1",
        "status_domain_alignment_used": "0",
        "normal_form_alignment_used": "0",
        "source_counterexample_status_used": "0",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "semantic payload preservation",
        "caveat": "Payload relevance still needs domain/normal-form compatibility.",
    },
    {
        "lemma_component": "domain_refutation_transfer",
        "statement": "The target refutation is meaningful in the source status-domain.",
        "assumptions": "status-domain alignment map",
        "conclusion": "target contradiction can be evaluated by source status predicate dependencies",
        "lifted_target_refutation_used": "1",
        "payload_alignment_used": "1",
        "status_domain_alignment_used": "1",
        "normal_form_alignment_used": "0",
        "source_counterexample_status_used": "1",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "source-target domain transfer",
        "caveat": "Shared status-domain blocker remains open.",
    },
    {
        "lemma_component": "normal_refutation_transfer",
        "statement": "The target refutation is interpreted without changing the source normal-form predicate.",
        "assumptions": "normal-form alignment map",
        "conclusion": "normal-form preconditions are compatible",
        "lifted_target_refutation_used": "1",
        "payload_alignment_used": "1",
        "status_domain_alignment_used": "1",
        "normal_form_alignment_used": "1",
        "source_counterexample_status_used": "1",
        "proof_status": "proof_sketch_only",
        "missing_hypothesis": "normal-form transfer",
        "caveat": "Normal-form transfer is not currently proved.",
    },
    {
        "lemma_component": "source_counterexample_refutation",
        "statement": "Lifted target refutation refutes the source counterexample once payload, domain, and normal-form alignment hold.",
        "assumptions": "target refutation;payload alignment;status-domain alignment;normal-form alignment",
        "conclusion": "source counterexample-status predicate is denied",
        "lifted_target_refutation_used": "1",
        "payload_alignment_used": "1",
        "status_domain_alignment_used": "1",
        "normal_form_alignment_used": "1",
        "source_counterexample_status_used": "1",
        "proof_status": REFUTATION_TRANSFER,
        "missing_hypothesis": "predicate determination from payload/domain/normal-form alignment",
        "caveat": "Proof-ready skeleton, not completed source refutation.",
    },
    {
        "lemma_component": "alignment_failure_smaller_witness",
        "statement": "If alignment fails by exposing a strict residual, the branch should construct a smaller witness.",
        "assumptions": "alignment mismatch;candidate residual decrease",
        "conclusion": "smaller witness branch",
        "lifted_target_refutation_used": "0",
        "payload_alignment_used": "1",
        "status_domain_alignment_used": "1",
        "normal_form_alignment_used": "1",
        "source_counterexample_status_used": "1",
        "proof_status": "blocked_by_measure_decrease",
        "missing_hypothesis": "residual absorption measure decrease and valid reduced status",
        "caveat": "Kept separate from alignment proof.",
    },
    {
        "lemma_component": "alignment_failure_escape",
        "statement": "If alignment fails without an available smaller-witness construction, the failure is named as an operation-specific or higher-support escape.",
        "assumptions": "no current alignment proof;no current smaller witness proof",
        "conclusion": "named escape",
        "lifted_target_refutation_used": "0",
        "payload_alignment_used": "1",
        "status_domain_alignment_used": "1",
        "normal_form_alignment_used": "1",
        "source_counterexample_status_used": "1",
        "proof_status": "proved_under_current_scope",
        "missing_hypothesis": "",
        "caveat": "Escape does not prove source refutation.",
    },
]


obligation_header = [
    "obligation_key",
    "statement",
    "required_for_selected_statement",
    "existing_verified_inputs",
    "missing_sublemmas",
    "proof_status",
    "dependency_on_payload_alignment",
    "dependency_on_status_domain",
    "dependency_on_normal_form",
    "dependency_on_refutation",
    "dependency_on_measure_decrease",
    "dependency_on_higher_support",
    "can_attempt_now",
    "recommended_next_action",
]

obligation_rows = [
    {
        "obligation_key": "source_target_alignment_language_well_defined",
        "statement": "The source-target alignment language is well-defined for recognized family-chain source forms.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "source recognizer;lift map;status language",
        "missing_sublemmas": "",
        "proof_status": "proved_under_current_scope",
        "dependency_on_payload_alignment": "0",
        "dependency_on_status_domain": "0",
        "dependency_on_normal_form": "0",
        "dependency_on_refutation": "0",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "1",
        "recommended_next_action": "use_as_language_contract",
    },
    {
        "obligation_key": "payload_alignment_available",
        "statement": "Source and target payload correspondence is available for the lifted target refutation.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "family_chain_source_target_payload_correspondence",
        "missing_sublemmas": "semantic payload preservation",
        "proof_status": "proof_sketch_ready",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "0",
        "dependency_on_normal_form": "0",
        "dependency_on_refutation": "1",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "1",
        "recommended_next_action": "refine_payload_alignment_sublemma",
    },
    {
        "obligation_key": "status_domain_alignment_available",
        "statement": "The lifted target status-domain can be compared with the source status-domain.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "status language;family-chain status semantics",
        "missing_sublemmas": "source-target domain transfer",
        "proof_status": "needs_status_domain_sublemma",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "1",
        "dependency_on_normal_form": "0",
        "dependency_on_refutation": "1",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "1",
        "recommended_next_action": "project_to_active_status_domain_refinement",
    },
    {
        "obligation_key": "normal_form_alignment_available",
        "statement": "The lifted target normal form is compatible with the source normal form for status comparison.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "support notation and normal form;lift map",
        "missing_sublemmas": "source-target normal-form transfer",
        "proof_status": "needs_normal_form_sublemma",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "1",
        "dependency_on_normal_form": "1",
        "dependency_on_refutation": "1",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "1",
        "recommended_next_action": "contract_and_canonical_domain_normal_form_rounds",
    },
    {
        "obligation_key": "lifted_target_refutation_available",
        "statement": "The lifted target refutation from the lower-layer package is available.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "family_chain_absorption_refutation_lemma;7/7 fresh lower layers",
        "missing_sublemmas": "",
        "proof_status": "proved_under_current_scope",
        "dependency_on_payload_alignment": "0",
        "dependency_on_status_domain": "0",
        "dependency_on_normal_form": "0",
        "dependency_on_refutation": "1",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "1",
        "recommended_next_action": "use_as_target_side_input",
    },
    {
        "obligation_key": "payload_alignment_supports_source_refutation",
        "statement": "Payload alignment makes the target refutation relevant to the source payload obstruction.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "payload correspondence;target refutation",
        "missing_sublemmas": "payload semantic preservation",
        "proof_status": "proof_sketch_ready",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "0",
        "dependency_on_normal_form": "0",
        "dependency_on_refutation": "1",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "1",
        "recommended_next_action": "prove_payload_semantic_preservation",
    },
    {
        "obligation_key": "domain_alignment_supports_source_refutation",
        "statement": "Domain alignment lets the target contradiction be evaluated by the source status predicate.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "status-domain alignment inventory",
        "missing_sublemmas": "source-target domain transfer",
        "proof_status": "needs_status_domain_sublemma",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "1",
        "dependency_on_normal_form": "0",
        "dependency_on_refutation": "1",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "1",
        "recommended_next_action": "status_domain_alignment_refinement",
    },
    {
        "obligation_key": "normal_form_alignment_supports_source_refutation",
        "statement": "Normal-form alignment prevents target refutation from changing the source predicate meaning.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "normal-form alignment inventory",
        "missing_sublemmas": "normal-form transfer",
        "proof_status": "needs_normal_form_sublemma",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "1",
        "dependency_on_normal_form": "1",
        "dependency_on_refutation": "1",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "1",
        "recommended_next_action": "normal_form_alignment_refinement",
    },
    {
        "obligation_key": "lifted_refutation_refutes_source_counterexample",
        "statement": "The aligned lifted refutation denies the source counterexample-status predicate.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "target refutation;payload alignment inventory",
        "missing_sublemmas": "payload+domain+normal-form predicate determination",
        "proof_status": "needs_refutation_transfer_sublemma",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "1",
        "dependency_on_normal_form": "1",
        "dependency_on_refutation": "1",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "1",
        "recommended_next_action": "lifted_refutation_transfer_sublemma",
    },
    {
        "obligation_key": "alignment_failure_constructs_smaller_witness",
        "statement": "A failed alignment branch constructs a strict smaller witness when residual data decreases.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "absorption construction;measure framework",
        "missing_sublemmas": "residual absorption measure decrease",
        "proof_status": "needs_measure_sublemma",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "1",
        "dependency_on_normal_form": "1",
        "dependency_on_refutation": "0",
        "dependency_on_measure_decrease": "1",
        "dependency_on_higher_support": "0",
        "can_attempt_now": "0",
        "recommended_next_action": NEXT1,
    },
    {
        "obligation_key": "alignment_failure_is_named_escape",
        "statement": "A failed non-reduced alignment branch is named as an operation-specific or higher-support escape.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "higher-support escape interface;operation blocker language",
        "missing_sublemmas": "",
        "proof_status": "proved_under_current_scope",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "1",
        "dependency_on_normal_form": "1",
        "dependency_on_refutation": "0",
        "dependency_on_measure_decrease": "0",
        "dependency_on_higher_support": "1",
        "can_attempt_now": "1",
        "recommended_next_action": "keep_named_escape_visible",
    },
    {
        "obligation_key": "no_hidden_alignment_failure_case",
        "statement": "Every current source-target alignment failure is either payload/domain/normal/refutation open, smaller-witness open, or named escape.",
        "required_for_selected_statement": "1",
        "existing_verified_inputs": "source alignment inventories",
        "missing_sublemmas": "full no-hidden-case proof after residual measure",
        "proof_status": "proof_sketch_ready",
        "dependency_on_payload_alignment": "1",
        "dependency_on_status_domain": "1",
        "dependency_on_normal_form": "1",
        "dependency_on_refutation": "1",
        "dependency_on_measure_decrease": "1",
        "dependency_on_higher_support": "1",
        "can_attempt_now": "1",
        "recommended_next_action": NEXT1,
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
        "sublemma_key": "source_target_alignment_language_well_defined",
        "proof_status": "proved_under_current_scope",
        "assumptions": "recognized source form;lift map;status language",
        "conclusion": "source-target alignment components are named and well-typed",
        "proof_summary": "The source recognizer, lift map, payload correspondence, and status language define the four alignment axes without claiming they all hold.",
        "evidence_path": "family_chain_source_form_recognizer_90.md;family_chain_lift_map_90.md;status_preservation_language_90.md",
        "missing_hypothesis": "",
        "next_action": "use_as_contract",
    },
    {
        "sublemma_key": "payload_alignment_available",
        "proof_status": "proof_sketch_only",
        "assumptions": "source-target payload correspondence table;fresh lower-layer target rows",
        "conclusion": "payload alignment is available as a proof-ready contract",
        "proof_summary": "The payload correspondence maps source obstruction fields to lifted target payload fields, but semantic preservation through source refutation is still a proof obligation.",
        "evidence_path": "family_chain_source_target_payload_correspondence_90.md;family_chain_lower_layers_priority_map_90.md",
        "missing_hypothesis": "semantic payload preservation",
        "next_action": "refine_payload_semantics",
    },
    {
        "sublemma_key": "status_domain_alignment_available",
        "proof_status": "proof_sketch_only",
        "assumptions": "source status-domain;target refutation domain;alignment map",
        "conclusion": "status-domain alignment is proof-ready but open",
        "proof_summary": "The required source and target status domains are identified and related by an intended map; the domain transfer proof remains open.",
        "evidence_path": "family_chain_absorption_status_domain_alignment_90.md",
        "missing_hypothesis": "source-target status-domain transfer",
        "next_action": "status_domain_alignment_refinement",
    },
    {
        "sublemma_key": "normal_form_alignment_available",
        "proof_status": "proof_sketch_only",
        "assumptions": "source normal form;target normal form;support notation",
        "conclusion": "normal-form alignment is proof-ready but open",
        "proof_summary": "The normal-form components are separated from payload and domain transfer; the field-preservation proof remains open.",
        "evidence_path": "family_chain_absorption_normal_form_alignment_90.md;support_notation_and_normal_form_90.md",
        "missing_hypothesis": "source-target normal-form transfer",
        "next_action": "normal_form_alignment_refinement",
    },
    {
        "sublemma_key": "lifted_target_refutation_available",
        "proof_status": "proved_under_current_scope",
        "assumptions": "fresh lower-layer package;family-chain absorption refutation lemma",
        "conclusion": "target-side refutation is available",
        "proof_summary": "The lower-layer target package is fresh and the lifted target refutation side is ready; this does not assert source transfer.",
        "evidence_path": "family_chain_absorption_refutation_lemma_90.md;family_chain_lower_layers_priority_map_90.md",
        "missing_hypothesis": "",
        "next_action": "use_as_refutation_input",
    },
    {
        "sublemma_key": "payload_alignment_supports_source_refutation",
        "proof_status": "proof_sketch_only",
        "assumptions": "payload alignment;lifted target refutation",
        "conclusion": "target refutation is payload-relevant to source obstruction",
        "proof_summary": "Payload relevance follows at sketch level from correspondence, but counterexample-status refutation still needs domain and normal-form determination.",
        "evidence_path": "family_chain_absorption_payload_alignment_refinement_90.md",
        "missing_hypothesis": "payload semantic preservation and predicate determination",
        "next_action": "lifted_refutation_transfer_sublemma",
    },
    {
        "sublemma_key": "lifted_refutation_refutes_source_counterexample",
        "proof_status": "blocked_by_refutation_transfer",
        "assumptions": "target refutation;payload alignment;domain alignment;normal-form alignment",
        "conclusion": "source counterexample is refuted",
        "proof_summary": "This is not closed. The round isolates the exact missing bridge: payload/domain/normal-form alignment must determine the source counterexample-status predicate.",
        "evidence_path": "lifted_refutation_to_source_refutation_lemma_90.md",
        "missing_hypothesis": "payload+domain+normal-form implies source counterexample-status refutation",
        "next_action": "refutation_transfer_sublemma",
    },
    {
        "sublemma_key": "alignment_failure_constructs_smaller_witness",
        "proof_status": "blocked_by_measure_decrease",
        "assumptions": "alignment failure exposes residual reduction candidate",
        "conclusion": "strict smaller witness",
        "proof_summary": "The smaller-witness branch is identified, but it depends on the residual absorption measure decrease that remains open.",
        "evidence_path": "family_chain_absorption_measure_decrease_90.md",
        "missing_hypothesis": "residual absorption measure decrease",
        "next_action": NEXT1,
    },
    {
        "sublemma_key": "alignment_failure_is_named_escape",
        "proof_status": "proved_under_current_scope",
        "assumptions": "non-refuting alignment branch;no smaller-witness proof available",
        "conclusion": "named alignment escape",
        "proof_summary": "Payload, domain, normal-form, refutation-transfer, and higher-support escapes are all named rather than hidden.",
        "evidence_path": "higher_support_escape_interface_90.md;higher_support_necessity_after_family_chain_source_alignment_90.md",
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
        "resolves": "residual_absorption_measure_and_smaller_witness_branch",
        "prerequisite_status": SOURCE_ALIGNMENT_SKELETON,
        "readiness_score_0_100": "84",
        "proof_value_0_100": "87",
        "engineering_cost_0_100": "68",
        "risk_0_100": "70",
        "dependency_clarity_0_100": "86",
        "expected_progress_value_0_100": "85",
        "recommended_order": "1",
        "final_recommendation": NEXT1,
        "reason": "Source-target alignment is now first-class and proof-ready; the remaining family-chain branch is the residual measure/smaller-witness proof.",
    },
    {
        "action_key": NEXT2,
        "resolves": "shared_project_status_domain_transfer",
        "prerequisite_status": "project_to_active_payload_proved_status_domain_open",
        "readiness_score_0_100": "81",
        "proof_value_0_100": "85",
        "engineering_cost_0_100": "61",
        "risk_0_100": "62",
        "dependency_clarity_0_100": "84",
        "expected_progress_value_0_100": "81",
        "recommended_order": "2",
        "final_recommendation": NEXT2,
        "reason": "The domain blocker recurs across source alignment and project-to-active locality.",
    },
    {
        "action_key": NEXT3,
        "resolves": "contract_equivalent_quotient_domain_normal_form_transfer",
        "prerequisite_status": "proof_ready_skeleton_contract_equivalent_congruence_domain_normal_form_open",
        "readiness_score_0_100": "79",
        "proof_value_0_100": "83",
        "engineering_cost_0_100": "63",
        "risk_0_100": "64",
        "dependency_clarity_0_100": "83",
        "expected_progress_value_0_100": "79",
        "recommended_order": "3",
        "final_recommendation": NEXT3,
        "reason": "Coordinate contraction already has refined congruence skeleton and needs quotient domain/normal-form transfer.",
    },
    {
        "action_key": "canonical_compression_domain_normal_form_refinement",
        "resolves": "canonical_motif_domain_normal_form_transfer",
        "prerequisite_status": "proof_ready_skeleton_canonical_compression_congruence_domain_normal_form_open",
        "readiness_score_0_100": "77",
        "proof_value_0_100": "82",
        "engineering_cost_0_100": "63",
        "risk_0_100": "64",
        "dependency_clarity_0_100": "82",
        "expected_progress_value_0_100": "78",
        "recommended_order": "4",
        "final_recommendation": "canonical_compression_domain_normal_form_refinement",
        "reason": "Canonical side is proof-ready but still has motif domain/normal-form transfer open.",
    },
    {
        "action_key": "higher_support_necessity_recheck",
        "resolves": "deferred_higher_support_escape_reassessment",
        "prerequisite_status": HIGHER_SUPPORT,
        "readiness_score_0_100": "68",
        "proof_value_0_100": "76",
        "engineering_cost_0_100": "55",
        "risk_0_100": "64",
        "dependency_clarity_0_100": "81",
        "expected_progress_value_0_100": "71",
        "recommended_order": "5",
        "final_recommendation": "higher_support_necessity_recheck_after_operation_refinements",
        "reason": "Higher-support scan remains deferred while local operation blockers are still named and actionable.",
    },
    {
        "action_key": "limited_to_broader_generalization_plan",
        "resolves": "generalization_contract",
        "prerequisite_status": "operation_refinements_open",
        "readiness_score_0_100": "72",
        "proof_value_0_100": "80",
        "engineering_cost_0_100": "55",
        "risk_0_100": "61",
        "dependency_clarity_0_100": "84",
        "expected_progress_value_0_100": "74",
        "recommended_order": "6",
        "final_recommendation": "limited_to_broader_generalization_plan_after_operation_refinements",
        "reason": "Planning can proceed, but proof completion still depends on residual measure and domain/normal-form blockers.",
    },
    {
        "action_key": "support_bound_completion",
        "resolves": "support_bound_completion",
        "prerequisite_status": SUPPORT_BOUND,
        "readiness_score_0_100": "65",
        "proof_value_0_100": "83",
        "engineering_cost_0_100": "64",
        "risk_0_100": "72",
        "dependency_clarity_0_100": "80",
        "expected_progress_value_0_100": "70",
        "recommended_order": "7",
        "final_recommendation": "support_bound_completion_after_operation_refinements",
        "reason": "Premature until residual measure and operation-local transfer blockers close or become named true escapes.",
    },
]


def write_new_round_docs() -> None:
    write_table(RUNTIME / "family_chain_absorption_source_alignment_refinement_scope_inventory_90.tsv", scope_header, scope_rows)
    write_md(
        D90 / "family_chain_absorption_source_alignment_refinement_scope_memo_90.md",
        "Family Chain Absorption Source Alignment Refinement Scope Memo 90",
        "\n".join(
            [
                "This round selects `family_chain_absorption_source_alignment_refinement` as an operation-specific proof-attempt. It does not prove the full general theorem and does not prove full family-chain absorption reduction.",
                "",
                f"Selected statement: `{SELECTED}`.",
                "",
                md_table(scope_header, scope_rows),
            ]
        ),
    )

    write_table(RUNTIME / "family_chain_absorption_source_target_alignment_semantics_90.tsv", semantics_header, semantics_rows)
    write_md(
        D90 / "family_chain_absorption_source_target_alignment_semantics_90.md",
        "Family Chain Absorption Source Target Alignment Semantics 90",
        "\n".join(
            [
                f"Final status: `{ALIGNMENT_SEMANTICS}`.",
                "",
                "The alignment language separates source witness, lifted target, payload, status-domain, normal-form, and refutation transfer. It names smaller-witness and escape branches instead of treating them as preservation.",
                "",
                md_table(semantics_header, semantics_rows),
            ]
        ),
    )

    write_table(RUNTIME / "family_chain_absorption_payload_alignment_refinement_90.tsv", payload_header, payload_rows)
    write_md(
        D90 / "family_chain_absorption_payload_alignment_refinement_90.md",
        "Family Chain Absorption Payload Alignment Refinement 90",
        "\n".join(
            [
                f"Final status: `{PAYLOAD_ALIGNMENT}`.",
                "",
                "Payload correspondence is available for recognized family-chain source forms, but payload relevance is not the same as counterexample-status refutation.",
                "",
                md_table(payload_header, payload_rows),
            ]
        ),
    )

    write_table(RUNTIME / "family_chain_absorption_status_domain_alignment_90.tsv", alignment_header, domain_rows)
    write_md(
        D90 / "family_chain_absorption_status_domain_alignment_90.md",
        "Family Chain Absorption Status Domain Alignment 90",
        "\n".join(
            [
                f"Final status: `{DOMAIN_ALIGNMENT}`.",
                "",
                "Status-domain alignment is proof-ready but open. The target refutation must be meaningful inside the source status-domain before source counterexample refutation can be claimed.",
                "",
                md_table(alignment_header, domain_rows),
            ]
        ),
    )

    write_table(RUNTIME / "family_chain_absorption_normal_form_alignment_90.tsv", alignment_header, normal_rows)
    write_md(
        D90 / "family_chain_absorption_normal_form_alignment_90.md",
        "Family Chain Absorption Normal Form Alignment 90",
        "\n".join(
            [
                f"Final status: `{NORMAL_ALIGNMENT}`.",
                "",
                "Normal-form alignment is kept separate from payload and status-domain alignment. A target normal form does not automatically certify a valid source witness.",
                "",
                md_table(alignment_header, normal_rows),
            ]
        ),
    )

    write_table(RUNTIME / "lifted_refutation_to_source_refutation_lemma_90.tsv", lemma_header, lemma_rows)
    write_md(
        D90 / "lifted_refutation_to_source_refutation_lemma_90.md",
        "Lifted Refutation To Source Refutation Lemma 90",
        "\n".join(
            [
                f"Final status: `{REFUTATION_TRANSFER}`.",
                "",
                "The target-side refutation is available, but source counterexample refutation remains conditional on payload, status-domain, and normal-form alignment determining the source counterexample-status predicate.",
                "",
                md_table(lemma_header, lemma_rows),
            ]
        ),
    )

    write_table(RUNTIME / "family_chain_absorption_source_alignment_obligations_90.tsv", obligation_header, obligation_rows)
    write_md(
        D90 / "family_chain_absorption_source_alignment_obligations_90.md",
        "Family Chain Absorption Source Alignment Obligations 90",
        "\n".join(
            [
                "Obligation count: `12`.",
                "",
                md_table(obligation_header, obligation_rows),
            ]
        ),
    )

    write_table(RUNTIME / "family_chain_absorption_source_alignment_sublemma_proofs_90.tsv", sublemma_header, sublemma_rows)
    write_md(
        D90 / "family_chain_absorption_source_alignment_sublemma_proofs_90.md",
        "Family Chain Absorption Source Alignment Sublemma Proofs 90",
        "\n".join(
            [
                "Sublemma proof-attempt counts: `proved_under_current_scope=3`, `proof_sketch_only=4`, `blocked=2`.",
                "",
                md_table(sublemma_header, sublemma_rows),
            ]
        ),
    )

    skeleton_rows = [
        ("lemma_name", "family_chain_absorption_source_alignment_or_smaller_witness_or_escape"),
        ("exact_statement", "For recognized family-chain source form, aligned lifted target refutation refutes the source counterexample; failed alignment routes to smaller witness or named escape."),
        ("assumptions", "recognized source witness;fresh lifted target;payload/status-domain/normal-form alignment contracts;status language"),
        ("conclusion", "source refutation or smaller-witness branch or named escape"),
        ("source_witness", "recognized family-chain source witness"),
        ("lifted_absorption_target", "fresh lower-layer target package"),
        ("payload_alignment", PAYLOAD_ALIGNMENT),
        ("status_domain_alignment", DOMAIN_ALIGNMENT),
        ("normal_form_alignment", NORMAL_ALIGNMENT),
        ("lifted_refutation", "target_side_refutation_available"),
        ("source_counterexample_refutation", REFUTATION_TRANSFER),
        ("smaller_witness_fallback", "blocked_by_residual_absorption_measure_decrease"),
        ("failure_to_escape_case", "named_alignment_escape_ready"),
        ("relation_to_family_chain_absorption_status_proof", FAMILY_STATUS),
        ("relation_to_canonical_congruence_refinement", "canonical_refined_domain_normal_form_open_kept_separate"),
        ("relation_to_status_congruence_bridge", STATUS_CONGRUENCE),
        ("missing_steps", "payload/domain/normal-form predicate determination; residual absorption measure decrease"),
        ("exact_caveat", "recognized-source alignment skeleton only; not full family-chain absorption proof"),
        ("final_status", SOURCE_ALIGNMENT_SKELETON),
    ]
    write_metric(RUNTIME / "family_chain_absorption_source_alignment_skeleton_90.tsv", skeleton_rows)
    write_md(
        D90 / "family_chain_absorption_source_alignment_skeleton_90.md",
        "Family Chain Absorption Source Alignment Skeleton 90",
        metric_table(skeleton_rows),
    )

    higher_rows = [
        {
            "check_key": "source_target_alignment_closed",
            "finding": "not fully closed; proof-ready skeleton with payload/domain/normal-form transfer open",
            "result_status": SOURCE_ALIGNMENT_SKELETON,
            "next_action": NEXT1,
        },
        {
            "check_key": "family_chain_source_refutation_blocker_reduced",
            "finding": "reduced from undifferentiated source-alignment open to named payload/domain/normal/refutation-transfer obligations",
            "result_status": REFUTATION_TRANSFER,
            "next_action": "refutation_transfer_sublemma_after_domain_normal",
        },
        {
            "check_key": "residual_absorption_measure_priority",
            "finding": "residual measure remains the next family-chain blocker after alignment formalization",
            "result_status": "residual_absorption_measure_decrease_open",
            "next_action": NEXT1,
        },
        {
            "check_key": "project_contract_canonical_blockers",
            "finding": "shared domain/normal-form transfer blockers remain open for project, contract, and canonical operations",
            "result_status": "shared_domain_normal_form_transfer_open",
            "next_action": NEXT2,
        },
        {
            "check_key": "higher_support_deferred",
            "finding": "higher-support scan remains deferred; current blockers are operation-local alignment and residual measure",
            "result_status": HIGHER_SUPPORT,
            "next_action": "do_not_scan_support9_yet",
        },
        {
            "check_key": "limited_to_broader_generalization",
            "finding": "not ready for general theorem completion; operation-local transfer and measure blockers remain",
            "result_status": GENERAL_READY,
            "next_action": NEXT1,
        },
    ]
    higher_header = ["check_key", "finding", "result_status", "next_action"]
    write_table(RUNTIME / "higher_support_necessity_after_family_chain_source_alignment_90.tsv", higher_header, higher_rows)
    write_md(
        D90 / "higher_support_necessity_after_family_chain_source_alignment_90.md",
        "Higher Support Necessity After Family Chain Source Alignment 90",
        "\n".join([f"Final status: `{HIGHER_SUPPORT}`.", "", md_table(higher_header, higher_rows)]),
    )

    write_metric(
        RUNTIME / "family_chain_absorption_source_alignment_fingerprint_90.tsv",
        [
            ("selected_statement", SELECTED),
            ("alignment_semantics_status", ALIGNMENT_SEMANTICS),
            ("payload_alignment_status", PAYLOAD_ALIGNMENT),
            ("status_domain_alignment_status", DOMAIN_ALIGNMENT),
            ("normal_form_alignment_status", NORMAL_ALIGNMENT),
            ("refutation_transfer_status", REFUTATION_TRANSFER),
            ("skeleton_status", SOURCE_ALIGNMENT_SKELETON),
            ("obligation_count", "12"),
            ("proved_under_current_scope_sublemma_count", "3"),
            ("proof_sketch_only_sublemma_count", "4"),
            ("blocked_sublemma_count", "2"),
        ],
    )
    write_metric(
        RUNTIME / "lifted_refutation_to_source_refutation_fingerprint_90.tsv",
        [
            ("target_refutation_available", "proved_under_current_scope"),
            ("source_refutation_transfer", REFUTATION_TRANSFER),
            ("missing_hypothesis", "payload_domain_normal_form_predicate_determination"),
        ],
    )
    write_metric(
        RUNTIME / "family_chain_absorption_payload_alignment_fingerprint_90.tsv",
        [
            ("payload_alignment_status", PAYLOAD_ALIGNMENT),
            ("lower_layer_fresh_count", "7"),
            ("lower_layer_imported_count", "0"),
            ("payload_alignment_caveat", "freshness_is_availability_not_semantic_equivalence"),
        ],
    )


def update_existing_docs() -> None:
    family_status_rows = [
        ("selected_source_alignment_statement", SELECTED),
        ("source_target_alignment_semantics_status", ALIGNMENT_SEMANTICS),
        ("payload_alignment_refinement_status", PAYLOAD_ALIGNMENT),
        ("status_domain_alignment_status", DOMAIN_ALIGNMENT),
        ("normal_form_alignment_status", NORMAL_ALIGNMENT),
        ("lifted_refutation_to_source_refutation_status", REFUTATION_TRANSFER),
        ("source_alignment_skeleton_status", SOURCE_ALIGNMENT_SKELETON),
        ("family_chain_absorption_status_after_round", FAMILY_STATUS),
        ("residual_absorption_measure_status", "residual_absorption_measure_decrease_open"),
        ("caveat", "not_full_family_chain_absorption_proof"),
    ]
    write_metric(RUNTIME / "family_chain_absorption_status_skeleton_90.tsv", family_status_rows)
    upsert_section(
        D90 / "family_chain_absorption_status_skeleton_90.md",
        "Source Alignment Refinement Round",
        metric_table(family_status_rows),
    )

    refutation_rows = [
        ("lifted_target_refutation_status", "target_side_refutation_available_under_current_scope"),
        ("payload_alignment_status", PAYLOAD_ALIGNMENT),
        ("status_domain_alignment_status", DOMAIN_ALIGNMENT),
        ("normal_form_alignment_status", NORMAL_ALIGNMENT),
        ("source_counterexample_refutation_status", REFUTATION_TRANSFER),
        ("final_status", "family_chain_absorption_refutation_lifted_target_ready_source_alignment_proof_ready_domain_normal_open"),
        ("caveat", "source_refutation_not_promoted_without_alignment_proof"),
    ]
    write_metric(RUNTIME / "family_chain_absorption_refutation_lemma_90.tsv", refutation_rows)
    upsert_section(
        D90 / "family_chain_absorption_refutation_lemma_90.md",
        "Source Alignment Refinement Round",
        metric_table(refutation_rows),
    )

    write_table(RUNTIME / "family_chain_absorption_status_obligations_90.tsv", obligation_header, obligation_rows)
    upsert_section(
        D90 / "family_chain_absorption_status_obligations_90.md",
        "Source Alignment Refinement Round",
        "The family-chain status obligation inventory now delegates source-target transfer to `family_chain_absorption_source_alignment_obligations_90` and keeps residual measure separate.\n\n"
        + md_table(obligation_header, obligation_rows),
    )

    write_table(RUNTIME / "family_chain_absorption_status_sublemma_proofs_90.tsv", sublemma_header, sublemma_rows)
    upsert_section(
        D90 / "family_chain_absorption_status_sublemma_proofs_90.md",
        "Source Alignment Refinement Round",
        "The source-alignment proof attempt produced `proved_under_current_scope=3`, `proof_sketch_only=4`, `blocked=2` for the prioritized sublemmas.\n\n"
        + md_table(sublemma_header, sublemma_rows),
    )

    status_rows = [
        ("contract_equivalent_operation_status", "partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open"),
        ("canonical_compression_operation_status", "partial_canonical_compression_congruence_proof_ready_domain_normal_form_open"),
        ("family_chain_absorption_status", FAMILY_STATUS),
        ("project_to_active_status", "partial_project_to_active_locality_proof_ready_status_domain_open"),
        ("status_congruence_bridge_status", STATUS_CONGRUENCE),
        ("remaining_blockers", "residual_absorption_measure_decrease;shared_status_domain_normal_form_transfer;project_contract_canonical_transfer"),
        ("caveat", "one_operation_source_alignment_refinement_not_all_operation_proof"),
    ]
    write_metric(RUNTIME / "status_preservation_congruence_skeleton_90.tsv", status_rows)
    upsert_section(
        D90 / "status_preservation_congruence_skeleton_90.md",
        "Family Chain Source Alignment Refinement Round",
        metric_table(status_rows),
    )

    op_header = [
        "operation_key",
        "operation_status",
        "status_congruence_status",
        "measure_status",
        "remaining_blocker",
        "next_action",
    ]
    op_rows = [
        {
            "operation_key": "project_to_active",
            "operation_status": "partial_project_to_active_locality_proof_ready_status_domain_open",
            "status_congruence_status": "payload_locality_proved_status_domain_open",
            "measure_status": "not_primary_for_this_operation",
            "remaining_blocker": "status_domain_transfer",
            "next_action": NEXT2,
        },
        {
            "operation_key": "contract_equivalent_support_coordinates",
            "operation_status": "partial_contract_equivalent_congruence_proof_ready_domain_normal_form_open",
            "status_congruence_status": "proof_ready_domain_normal_form_open",
            "measure_status": "measure_decrease_available_separate_from_status",
            "remaining_blocker": "quotient_domain_normal_form_transfer",
            "next_action": NEXT3,
        },
        {
            "operation_key": "canonical_motif_compression",
            "operation_status": "partial_canonical_compression_congruence_proof_ready_domain_normal_form_open",
            "status_congruence_status": "proof_ready_domain_normal_form_open",
            "measure_status": "lexicographic_measure_available_separate_from_status",
            "remaining_blocker": "motif_domain_normal_form_transfer",
            "next_action": "canonical_compression_domain_normal_form_refinement",
        },
        {
            "operation_key": "family_chain_absorption",
            "operation_status": FAMILY_STATUS,
            "status_congruence_status": SOURCE_ALIGNMENT_SKELETON,
            "measure_status": "residual_absorption_measure_decrease_open",
            "remaining_blocker": "residual_measure_and_refutation_transfer",
            "next_action": NEXT1,
        },
    ]
    write_table(RUNTIME / "support_reduction_operation_status_table_90.tsv", op_header, op_rows)
    upsert_section(
        D90 / "support_reduction_operation_status_table_90.md",
        "Family Chain Source Alignment Refinement Round",
        md_table(op_header, op_rows),
    )

    reduction_rows = [
        ("support_reduction_step_status", SUPPORT_REDUCTION),
        ("family_chain_source_alignment_status", SOURCE_ALIGNMENT_SKELETON),
        ("family_chain_absorption_status", FAMILY_STATUS),
        ("residual_absorption_measure_status", "residual_absorption_measure_decrease_open"),
        ("project_status_domain_status", "open"),
        ("contract_domain_normal_status", "open"),
        ("canonical_domain_normal_status", "open"),
        ("next_exact_target", NEXT1),
    ]
    write_metric(RUNTIME / "support_reduction_step_skeleton_90.tsv", reduction_rows)
    upsert_section(
        D90 / "support_reduction_step_skeleton_90.md",
        "Family Chain Source Alignment Refinement Round",
        metric_table(reduction_rows),
    )

    bound_rows = [
        ("support_bound_lemma_status", SUPPORT_BOUND),
        ("support_reduction_dependency", SUPPORT_REDUCTION),
        ("status_congruence_dependency", STATUS_CONGRUENCE),
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("general_theorem_readiness", GENERAL_READY),
        ("next_exact_target", NEXT1),
        ("caveat", "support_bound_not_completed_by_one_alignment_round"),
    ]
    write_metric(RUNTIME / "support_bound_lemma_skeleton_90.tsv", bound_rows)
    upsert_section(
        D90 / "support_bound_lemma_skeleton_90.md",
        "Family Chain Source Alignment Refinement Round",
        metric_table(bound_rows),
    )
    write_metric(
        RUNTIME / "support_bound_skeleton_fingerprint_90.tsv",
        [
            ("support_bound_lemma_skeleton", SUPPORT_BOUND),
            ("support_reduction_step_status", SUPPORT_REDUCTION),
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("next_action", NEXT1),
            ("fingerprint", f"{SUPPORT_BOUND}|{SUPPORT_REDUCTION}|{STATUS_CONGRUENCE}|{NEXT1}"),
        ],
    )
    write_metric(
        RUNTIME / "support_bound_status_fingerprint_90.tsv",
        [
            ("support_bound_status", SUPPORT_BOUND),
            ("higher_support_necessity_status", HIGHER_SUPPORT),
            ("general_theorem_readiness", GENERAL_READY),
            ("fingerprint", f"{SUPPORT_BOUND}|{HIGHER_SUPPORT}|{GENERAL_READY}"),
        ],
    )
    write_metric(
        RUNTIME / "support_reduction_step_skeleton_fingerprint_90.tsv",
        [
            ("support_reduction_step_skeleton", SUPPORT_REDUCTION),
            ("family_chain_source_alignment_status", SOURCE_ALIGNMENT_SKELETON),
            ("next_action", NEXT1),
            ("fingerprint", f"{SUPPORT_REDUCTION}|{SOURCE_ALIGNMENT_SKELETON}|{NEXT1}"),
        ],
    )
    write_metric(
        RUNTIME / "support_reduction_step_status_fingerprint_90.tsv",
        [
            ("support_reduction_step_skeleton", SUPPORT_REDUCTION),
            ("support_bound_skeleton", SUPPORT_BOUND),
            ("next_action", NEXT1),
            ("fingerprint", f"{SUPPORT_REDUCTION}|{SUPPORT_BOUND}|{NEXT1}"),
        ],
    )
    write_metric(
        RUNTIME / "support_reduction_selected_operation_status_fingerprint_90.tsv",
        [
            ("selected_operation", "family_chain_absorption"),
            ("support_reduction_skeleton_status", SUPPORT_REDUCTION),
            ("support_bound_skeleton_status", SUPPORT_BOUND),
            ("family_chain_absorption_status", FAMILY_STATUS),
            ("next_action", NEXT1),
            ("fingerprint", f"family_chain_absorption|{SUPPORT_REDUCTION}|{SUPPORT_BOUND}|{FAMILY_STATUS}|{NEXT1}"),
        ],
    )
    write_metric(
        RUNTIME / "support_reduction_operation_status_table_fingerprint_90.tsv",
        [
            ("operation_status_table", "operation_status_table_family_alignment_refined_payload_domain_normal_open"),
            ("family_chain_absorption_status", FAMILY_STATUS),
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("next_blocker", NEXT1),
            ("fingerprint", f"{FAMILY_STATUS}|{STATUS_CONGRUENCE}|{NEXT1}"),
        ],
    )
    write_metric(
        RUNTIME / "status_preservation_congruence_fingerprint_90.tsv",
        [
            ("status_congruence_skeleton_status", STATUS_CONGRUENCE),
            ("family_chain_source_alignment_status", SOURCE_ALIGNMENT_SKELETON),
            ("family_chain_absorption_status", FAMILY_STATUS),
            ("next_action_first", NEXT1),
            ("fingerprint", f"{STATUS_CONGRUENCE}|{SOURCE_ALIGNMENT_SKELETON}|{FAMILY_STATUS}|{NEXT1}"),
        ],
    )

    bridge_rows = [
        ("general_bridge_obligation_status", "family_chain_source_alignment_refined_residual_measure_open"),
        ("source_alignment_status", SOURCE_ALIGNMENT_SKELETON),
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
        upsert_section(D90 / f"{name}.md", "Family Chain Source Alignment Refinement Round", metric_table(bridge_rows))

    write_table(RUNTIME / "general_gap_bridge_next_action_matrix_90.tsv", next_header, next_rows)
    upsert_section(
        D90 / "general_gap_bridge_next_action_matrix_90.md",
        "Family Chain Source Alignment Refinement Round",
        md_table(next_header, next_rows),
    )
    readiness_rows = [
        ("support_bound_lemma_status", SUPPORT_BOUND),
        ("support_reduction_step_status", SUPPORT_REDUCTION),
        ("status_congruence_status", STATUS_CONGRUENCE),
        ("family_chain_source_alignment_status", SOURCE_ALIGNMENT_SKELETON),
        ("family_chain_absorption_status", FAMILY_STATUS),
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("readiness_label", GENERAL_READY),
        ("next_action_first", NEXT1),
        ("next_action_second", NEXT2),
        ("next_action_third", NEXT3),
        ("caveat", "not_full_general_theorem"),
    ]
    write_metric(RUNTIME / "general_gap_theorem_readiness_audit_90.tsv", readiness_rows)
    upsert_section(
        D90 / "general_gap_theorem_readiness_audit_90.md",
        "Family Chain Source Alignment Refinement Round",
        metric_table(readiness_rows),
    )
    write_metric(
        RUNTIME / "general_gap_bridge_readiness_fingerprint_90.tsv",
        [
            ("status_congruence_status", STATUS_CONGRUENCE),
            ("support_bound_lemma_status", SUPPORT_BOUND),
            ("support_reduction_step_status", SUPPORT_REDUCTION),
            ("general_theorem_readiness_label", GENERAL_READY),
            ("next_action_first", NEXT1),
            ("next_action_second", NEXT2),
            ("next_action_third", NEXT3),
            ("fingerprint", f"{STATUS_CONGRUENCE}|{SUPPORT_BOUND}|{SUPPORT_REDUCTION}|{GENERAL_READY}|{NEXT1}"),
        ],
    )

    support_cert_rows = [
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
    ]
    write_metric(RUNTIME / "current_support8_closure_certificate_90.tsv", support_cert_rows)
    upsert_section(
        D90 / "current_support8_closure_certificate_90.md",
        "Family Chain Source Alignment Refinement Round",
        metric_table(support_cert_rows),
    )

    escape_rows = [
        ("higher_support_necessity_status", HIGHER_SUPPORT),
        ("source_alignment_escape", "payload_domain_normal_refutation_transfer_escape_named"),
        ("residual_measure_escape", "residual_absorption_measure_decrease_open"),
        ("support9_scan_status", "not_run"),
        ("caveat", "higher_support_escape_visible_not_resolved"),
    ]
    write_metric(RUNTIME / "higher_support_escape_interface_90.tsv", escape_rows)
    upsert_section(D90 / "higher_support_escape_interface_90.md", "Family Chain Source Alignment Refinement Round", metric_table(escape_rows))

    contract_body = metric_table(
        [
            ("latest_round", "family_chain_absorption_source_alignment_refinement"),
            ("release_compile", "verified_after_regression"),
            ("local_test_compile", "verified_after_regression"),
            ("pass1_pass2_pass3", "support8_authoritative_completion_locked"),
            ("source_alignment_status", SOURCE_ALIGNMENT_SKELETON),
            ("general_theorem_readiness", GENERAL_READY),
        ]
    )
    upsert_section(D90 / "proof_system_contract_memo_90.md", "Family Chain Source Alignment Refinement Round", contract_body)
    upsert_section(D90 / "proof_system_reproduction_report_90.md", "Family Chain Source Alignment Refinement Round", contract_body)

    root_body = metric_table(
        [
            ("latest_round", "family_chain_absorption_source_alignment_refinement"),
            ("selected_statement", SELECTED),
            ("source_alignment_semantics_status", ALIGNMENT_SEMANTICS),
            ("payload_alignment_status", PAYLOAD_ALIGNMENT),
            ("status_domain_alignment_status", DOMAIN_ALIGNMENT),
            ("normal_form_alignment_status", NORMAL_ALIGNMENT),
            ("lifted_refutation_to_source_status", REFUTATION_TRANSFER),
            ("source_alignment_skeleton_status", SOURCE_ALIGNMENT_SKELETON),
            ("family_chain_absorption_status", FAMILY_STATUS),
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
        upsert_section(B4 / root_name, "Family Chain Source Alignment Refinement Round", root_body)
    upsert_section(RUNTIME / "project_status_summary_90.md", "Family Chain Source Alignment Refinement Round", root_body)


def main() -> None:
    write_new_round_docs()
    update_existing_docs()


if __name__ == "__main__":
    main()
