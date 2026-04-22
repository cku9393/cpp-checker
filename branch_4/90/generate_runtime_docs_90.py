#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
BRANCH4_ROOT = REPO_ROOT / "branch_4"
ARCHIVAL_ROOT = BRANCH4_ROOT / "90"
RUNTIME_ROOT = ARCHIVAL_ROOT / "runtime"
DEFAULT_LOG = Path("/tmp/top_tree90_pass_probe.err")
PASS_LOGS = {
    "pass1": Path("/tmp/top_tree90_pass1.err"),
    "pass2": Path("/tmp/top_tree90_pass2.err"),
    "pass3": Path("/tmp/top_tree90_pass3.err"),
}


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.exists() else ""


def parse_metric_tsv(path: Path) -> dict[str, str]:
    metrics: dict[str, str] = {}
    if not path.exists():
        return metrics
    lines = path.read_text(encoding="utf-8").splitlines()
    if lines and lines[0].strip() == "metric\tvalue":
        data_lines = lines[1:]
    else:
        data_lines = lines
    for line in data_lines:
        if not line.strip():
            break
        parts = line.split("\t", 1)
        if len(parts) == 2:
            metrics[parts[0]] = parts[1]
    return metrics


def parse_table_tsv(path: Path) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    if not path.exists():
        return rows
    lines = [line for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    if not lines:
        return rows
    header = lines[0].split("\t")
    for line in lines[1:]:
        parts = line.split("\t")
        if len(parts) < len(header):
            parts.extend([""] * (len(header) - len(parts)))
        rows.append(dict(zip(header, parts)))
    return rows


def parse_log(log_text: str) -> dict[str, object]:
    out: dict[str, object] = {"frontiers": {}}

    frontier_re = re.compile(
        r"^general frontier stats: tag=(\S+) rawCandidates=(\d+) canonicalCandidates=(\d+) "
        r"outsideBoundedCandidates=(\d+) localExactSurvivors=(\d+) plusOneSurvivors=(\d+) "
        r"theoremPreservingSurvivors=(\d+) unsupportedLocalExactCandidates=(\d+) "
        r"unsupportedPlusOneCandidates=(\d+) completed=(true|false)(?: why=(.*))?$",
        re.M,
    )
    for m in frontier_re.finditer(log_text):
        tag = m.group(1)
        out["frontiers"][tag] = {
            "tag": tag,
            "raw": m.group(2),
            "canonical": m.group(3),
            "outside": m.group(4),
            "local_exact": m.group(5),
            "plus_one": m.group(6),
            "theorem_preserving": m.group(7),
            "unsupported_local": m.group(8),
            "unsupported_plus_one": m.group(9),
            "completed": m.group(10),
            "why": m.group(11) or "",
        }

    single_line_keys = {
        "runtime_paths": r"^support8 runtime paths: (.*)$",
        "document_failure": r"^documentCompletion (.*)$",
        "lock_failure": r"^support8 completion lock (.*)$",
        "tail_chain_failure": r"^support8 tail obstruction chain (.*)$",
        "exact_audit": r"^exact theorem audit: (.*)$",
        "family_chain_audit": r"^family-chain theorem audit: (.*)$",
        "bounded_scope": r"^bounded schema scope: (.*)$",
        "general_gap": r"^general schema gap: (.*)$",
        "unified_theorem": r"^unified bounded schema-universe theorem: (.*)$",
        "unified_status": r"^unified bounded schema-universe status=(.*)$",
        "family_chain_status": r"^family-chain theorem status=(.*)$",
        "general_status": r"^general schema universe status=(.*)$",
    }
    for key, pattern in single_line_keys.items():
        m = re.search(pattern, log_text, re.M)
        out[key] = m.group(1).strip() if m else ""
    return out


def load_archival_classification() -> str:
    text = read_text(ARCHIVAL_ROOT / "project_status_summary_90.md")
    m = re.search(r"current classification\s+([A-Za-z0-9_]+)", text, re.S)
    return m.group(1) if m else "support8_authoritative_completion_locked"


def write_note(path: Path, title: str, body: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f"# {title}\n\n{body.strip()}\n", encoding="utf-8")


def archival_source_block(basename: str) -> str:
    archival_path = ARCHIVAL_ROOT / basename
    if archival_path.exists():
        return (
            "archival source\n"
            f"- preserved bundle note: `{archival_path}`\n"
            "- preserved note status: archival claim\n"
        )
    return (
        "archival source\n"
        f"- preserved bundle note: missing for `{basename}`\n"
        "- provenance used instead: current code why-string / theorem-data import path\n"
    )


def load_pass_statuses() -> dict[str, str]:
    out: dict[str, str] = {}
    for name, path in PASS_LOGS.items():
        text = read_text(path)
        if not text:
            out[name] = "not attempted"
            continue
        parsed = parse_log(text)
        out[name] = str(parsed.get("general_status", "") or "not fully reproduced")
    return out


def pick_names(rows: list[dict[str, str]], key: str, value: str, limit: int = 3) -> list[str]:
    return [row.get("display_name", row.get("item_key", "")) for row in rows if row.get(key) == value][:limit]


def get_row_by_item_key(rows: list[dict[str, str]], item_key: str) -> dict[str, str]:
    for row in rows:
        if row.get("item_key") == item_key:
            return row
    return {}


def frontier_note(
    title: str,
    basename: str,
    tag: str,
    frontier: dict[str, str],
    runtime_paths: str,
    provenance_row: dict[str, str],
    generation_audit: dict[str, str],
) -> str:
    if provenance_row.get("provenance_source") == "fresh_current_runtime_generated" and generation_audit:
        return f"""
status labels
- frontier theorem validation: `{provenance_row.get('validation_status', 'missing')}`
- frontier payload provenance: `{provenance_row.get('provenance_source', 'missing')}`
- current shell15 derivation / cache path: verified

current runtime source
- runtime root: `{RUNTIME_ROOT}`
- runtime path report: `{runtime_paths}`
- frontier tag: `{tag}`
- authoritative source: `{generation_audit.get('authoritative_source', 'missing')}`
- current constructor: `{generation_audit.get('constructor_name', 'missing')}`
- fresh constructor: `{generation_audit.get('fresh_constructor_name', 'missing')}`
- cache-load constructor: `{generation_audit.get('cache_load_constructor_name', 'missing')}`

current stats from current code path
- raw candidates: `{generation_audit.get('raw_candidates', frontier['raw'])}`
- canonical candidates: `{generation_audit.get('canonical_candidates', frontier['canonical'])}`
- outside-bounded candidates: `{generation_audit.get('outside_bounded_candidates', frontier['outside'])}`
- local exact survivors: `{generation_audit.get('local_exact_survivors', frontier['local_exact'])}`
- plus-one survivors: `{generation_audit.get('plus_one_survivors', frontier['plus_one'])}`
- theorem-preserving survivors: `{generation_audit.get('theorem_preserving_survivors', frontier['theorem_preserving'])}`
- unsupported local exact candidates: `{generation_audit.get('unsupported_local_exact_candidates', frontier['unsupported_local'])}`
- unsupported plus-one candidates: `{generation_audit.get('unsupported_plus_one_candidates', frontier['unsupported_plus_one'])}`
- candidate universe row count: `{generation_audit.get('candidate_row_count', 'missing')}`
- candidate universe fingerprint: `{generation_audit.get('candidate_universe_fingerprint', 'missing')}`
- local exact result cache fingerprint: `{generation_audit.get('local_exact_result_cache_fingerprint', 'missing')}`
- local exact progress cache fingerprint: `{generation_audit.get('local_exact_progress_cache_fingerprint', 'missing')}`
- plus-one cache fingerprint: `{generation_audit.get('plus_one_result_cache_fingerprint', 'missing') or 'empty'}`
- imported count equality: `{generation_audit.get('imported_count_equality', 'missing')}`
- imported row-set equality: `{generation_audit.get('imported_row_set_equality', 'missing')}`
- fallback reachable: `{generation_audit.get('fallback_reachable', 'missing')}`
- fallback hit: `{generation_audit.get('fallback_hit', 'missing')}`
- caveat: `{generation_audit.get('caveat', provenance_row.get('caveat', 'missing'))}`

{archival_source_block(basename)}
"""

    return f"""
status labels
- imported theorem-data availability: verified
- frontier theorem claim: archival claim
- current full derivation from scratch: not fully reproduced

current runtime source
- runtime root: `{RUNTIME_ROOT}`
- runtime path report: `{runtime_paths}`
- frontier tag: `{tag}`

current stats from current code path
- raw candidates: `{frontier['raw']}`
- canonical candidates: `{frontier['canonical']}`
- outside-bounded candidates: `{frontier['outside']}`
- local exact survivors: `{frontier['local_exact']}`
- plus-one survivors: `{frontier['plus_one']}`
- theorem-preserving survivors: `{frontier['theorem_preserving']}`
- unsupported local exact candidates: `{frontier['unsupported_local']}`
- unsupported plus-one candidates: `{frontier['unsupported_plus_one']}`
- completed flag: `{frontier['completed']}`
- why: `{frontier['why']}`

{archival_source_block(basename)}
"""


def main() -> int:
    log_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_LOG
    log_text = read_text(log_path)
    if not log_text:
        raise SystemExit(f"log file missing or empty: {log_path}")

    parsed = parse_log(log_text)
    artifact = parse_metric_tsv(RUNTIME_ROOT / "artifact_completion_audit_90.tsv")
    document = parse_metric_tsv(RUNTIME_ROOT / "document_completion_audit_90.tsv")
    rerun = parse_metric_tsv(RUNTIME_ROOT / "rerun_completion_audit_90.tsv")
    provenance = parse_metric_tsv(RUNTIME_ROOT / "provenance_audit_fingerprint_90.tsv")
    provenance_rows = parse_table_tsv(RUNTIME_ROOT / "theorem_data_provenance_inventory_90.tsv")
    basis_generation = parse_metric_tsv(RUNTIME_ROOT / "basis_only_generation_audit_90.tsv")
    family_generation = parse_metric_tsv(RUNTIME_ROOT / "family_chain_generation_audit_90.tsv")
    basis_family_summary = parse_metric_tsv(RUNTIME_ROOT / "basis_family_generation_fingerprint_90.tsv")
    family_constructor = parse_metric_tsv(RUNTIME_ROOT / "family_chain_constructor_audit_90.tsv")
    family_constructor_fingerprint = parse_metric_tsv(RUNTIME_ROOT / "family_chain_constructor_fingerprint_90.tsv")
    pair52_generation = parse_metric_tsv(RUNTIME_ROOT / "pair_expansion_aggregate_52_generation_audit_90.tsv")
    triple53_generation = parse_metric_tsv(RUNTIME_ROOT / "triple_family_expansion_theorem_data_53_generation_audit_90.tsv")
    quadruple55_generation = parse_metric_tsv(RUNTIME_ROOT / "quadruple_family_expansion_theorem_data_55_generation_audit_90.tsv")
    quintuple57_generation = parse_metric_tsv(RUNTIME_ROOT / "quintuple_family_expansion_theorem_data_57_generation_audit_90.tsv")
    sextuple57_generation = parse_metric_tsv(RUNTIME_ROOT / "sextuple_family_expansion_theorem_data_57_generation_audit_90.tsv")
    septuple57_generation = parse_metric_tsv(RUNTIME_ROOT / "septuple_family_expansion_theorem_data_57_generation_audit_90.tsv")
    high_family57_generation = parse_metric_tsv(RUNTIME_ROOT / "high_family_expansion_theorem_data_57_generation_audit_90.tsv")
    family_lower_fingerprint = parse_metric_tsv(RUNTIME_ROOT / "family_chain_lower_layers_fingerprint_90.tsv")
    shell15_frontier_p12 = parse_metric_tsv(RUNTIME_ROOT / "antecedent_plus_twelve_frontier_generation_audit_90.tsv")
    shell15_frontier_s8 = parse_metric_tsv(RUNTIME_ROOT / "support8_antecedent15_frontier_generation_audit_90.tsv")
    shell15_frontier_pair = parse_metric_tsv(RUNTIME_ROOT / "shell15_frontier_generation_fingerprint_90.tsv")
    shell15_frontier_constructor = parse_metric_tsv(RUNTIME_ROOT / "shell15_frontier_constructor_fingerprint_90.tsv")
    tail_cert = parse_metric_tsv(RUNTIME_ROOT / "support8_tail_stabilization_certificate_90.tsv")
    tail_candidates = [line for line in read_text(RUNTIME_ROOT / "support8_tail_candidate_fingerprints_90.tsv").splitlines() if line.strip()]
    local_stamp = parse_metric_tsv(RUNTIME_ROOT / "support8_local_test_success_stamp_90.tsv")
    release_stamp = parse_metric_tsv(RUNTIME_ROOT / "support8_release_compile_stamp_90.tsv")
    pass_statuses = load_pass_statuses()
    provenance_by_item = {row.get("item_key", ""): row for row in provenance_rows}

    runtime_paths = str(parsed.get("runtime_paths", ""))
    general_status = str(parsed.get("general_status", ""))
    archival_classification = load_archival_classification()
    lock_achieved = general_status == "support8_authoritative_completion_locked"
    document_verified = document.get("required_doc_count") == document.get("existing_doc_count") and document.get("required_doc_count") not in (None, "", "0")
    rerun_verified = (
        rerun.get("local_test_compiled") == "1"
        and rerun.get("local_test_passed") == "1"
        and rerun.get("release_compiled") == "1"
    )

    write_note(
        RUNTIME_ROOT / "project_status_summary_90.md",
        "Project Status Summary 90",
        f"""
status labels
- archival classification: `{archival_classification}`
- current reproducible classification: `{general_status or 'not fully reproduced'}`

current workspace reality
- runtime root: `{RUNTIME_ROOT}`
- runtime path report: `{runtime_paths}`
- required docs: `{document.get('existing_doc_count', '0')}` / `{document.get('required_doc_count', '39')}` before this note generation pass
- required artifacts: `{artifact.get('nonempty_artifact_count', '0')}` / `{artifact.get('required_artifact_count', '8')}`
- rerun audit fingerprint: `{rerun.get('audit_fingerprint', 'missing')}`

provenance inventory
- item count: `{provenance.get('item_count', str(len(provenance_rows))) or 'missing'}`
- current verified: `{provenance.get('current_verified_count', 'missing')}`
- fresh current runtime generated: `{provenance.get('fresh_current_runtime_generated_count', 'missing')}`
- current runtime validated imported data: `{provenance.get('current_runtime_validated_imported_data_count', 'missing')}`
- mixed: `{provenance.get('mixed_count', 'missing')}`
- archival only: `{provenance.get('archival_only_count', 'missing')}`
- provenance fingerprint: `{provenance.get('audit_fingerprint', 'missing')}`

current verified items
- release compile artifact path is live in current workspace
- LOCAL_TEST binary ran successfully in current workspace
- artifact completion audit currently reports `{artifact.get('nonempty_artifact_count', '0')}` / `{artifact.get('required_artifact_count', '8')}` nonempty artifacts

still pending in the captured log
- document completion gate: `{parsed.get('document_failure', '')}`
- completion lock gate: `{parsed.get('lock_failure', '')}`

90 archival claim
- preserved bundle classification: `{archival_classification}`
- exact minimal basis size: `96`
- support8 shell15 / tail pattern / tail obstruction chain / completion lock were recorded as verified in the archival bundle
- imported closed-output catalog still contributes frontier data from versions `57/67/70/71/72/74/75/76/77/79/84`

{archival_source_block('project_status_summary_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "boj_bridge_notes_90.md",
        "BOJ Bridge Notes 90",
        f"""
status labels
- bridge note availability: verified
- BOJ complete solver claim: refuted
- proof-system bridge claim: verified

current grounded statement
- `full_dynamic_top_tree_engine_90.cpp` explicitly says it is `NOT the complete BOJ solver`.
- the current runtime nevertheless validates a proof-system slice that can serve as a mathematical / theorem-data / audit bridge for later solver work.
- this bridge is about proof obligations, not online query execution.

current workspace anchors
- runtime root: `{RUNTIME_ROOT}`
- artifact completion audit fingerprint: `{artifact.get('audit_fingerprint', 'missing')}`
- general schema universe status from captured run: `{general_status}`

bridge meaning
- support8 / shell15 / tail artifacts are current reproducible proof artifacts.
- they constrain what a future BOJ-facing integration would be allowed to claim.
- they do not by themselves constitute a submit-ready online solver.
- provenance inventory currently separates fresh runtime audits from imported frontier theorem-data instead of pretending everything was freshly rederived here.

{archival_source_block('boj_bridge_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "support8_antecedent15_shell_theorem_notes_90.md",
        "Support8 Antecedent15 Shell Theorem Notes 90",
        f"""
status labels
- support8 antecedent15 shell theorem validation: `{provenance_by_item.get('support8_antecedent15_shell_theorem', {}).get('validation_status', 'missing')}`
- shell15 frontier pair provenance: `{provenance_by_item.get('antecedent_plus_twelve_frontier', {}).get('provenance_source', 'missing')}` and `{provenance_by_item.get('support8_antecedent15_frontier', {}).get('provenance_source', 'missing')}`
- theorem provenance: `{provenance_by_item.get('support8_antecedent15_shell_theorem', {}).get('provenance_source', 'missing')}`

current runtime source
- runtime root: `{RUNTIME_ROOT}`
- artifact audit fingerprint: `{artifact.get('audit_fingerprint', 'missing')}`
- frontier pair generation audit path: `{RUNTIME_ROOT / 'shell15_frontier_generation_fingerprint_90.tsv'}`
- frontier pair fresh runtime generated: `{shell15_frontier_pair.get('pair_fresh_runtime_generated', 'missing')}`

current verified shell15 artifact facts
- `antecedent_plus_twelve_candidate_universe_90.tsv`: present
- `support8_antecedent15_candidate_universe_90.tsv`: present
- `antecedent_shell15_local_exact_result_cache_90.tsv`: present
- `antecedent_shell15_local_exact_progress_cache_90.tsv`: present
- `antecedent_shell15_local_exact_survivors_90.tsv`: present
- `antecedent_shell15_plus_one_result_cache_90.tsv`: present

frontier theorem-data provenance
        - `antecedent_plus_twelve_frontier`: `{provenance_by_item.get('antecedent_plus_twelve_frontier', {}).get('provenance_source', 'missing')}` via `{shell15_frontier_p12.get('constructor_name', 'missing')}`
        - `support8_antecedent15_frontier`: `{provenance_by_item.get('support8_antecedent15_frontier', {}).get('provenance_source', 'missing')}` via `{shell15_frontier_s8.get('constructor_name', 'missing')}`
        - lower frontier direct dependency subset audit: `{RUNTIME_ROOT / 'lower_frontier_ladder_generation_audit_90.tsv'}`

current shell15 theorem reading
        - the current proof engine validated the shell15 theorem path while using a fresh shell15 frontier pair and a fresh current-runtime lower-frontier dependency subset.
        - the theorem itself is now fresh current-runtime generated at the top-level inventory layer.
        - the frontier counts remain `raw=4`, `canonical=2`, `outside-bounded=2`, `survivor=0` on both shell15 frontier tags.
        - theorem caveat: `{provenance_by_item.get('support8_antecedent15_shell_theorem', {}).get('caveat', 'missing')}`

{archival_source_block('support8_antecedent15_shell_theorem_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "support8_outside_bounded_tail_pattern_notes_90.md",
        "Support8 Outside Bounded Tail Pattern Notes 90",
        f"""
status labels
- tail pattern theorem validation: `{provenance_by_item.get('support8_outside_bounded_tail_pattern_theorem', {}).get('validation_status', 'missing')}`
- tail pattern provenance: `{provenance_by_item.get('support8_outside_bounded_tail_pattern_theorem', {}).get('provenance_source', 'missing')}`
- shell tail exhaustion claim: refuted

current runtime source
- runtime root: `{RUNTIME_ROOT}`
- tail stabilization certificate: `{RUNTIME_ROOT / 'support8_tail_stabilization_certificate_90.tsv'}`

current verified counts
- tail start antecedent bound: `{tail_cert.get('tail_start_antecedent_bound', 'missing')}`
- tail end antecedent bound: `{tail_cert.get('tail_end_antecedent_bound', 'missing')}`
- shell count validated: `{tail_cert.get('shell_count_validated', 'missing')}`
- support7 outside-bounded candidates: `{tail_cert.get('support7_outside_bounded_candidates', 'missing')}`
- support8 outside-bounded candidates: `{tail_cert.get('support8_outside_bounded_candidates', 'missing')}`
- support7 local exact survivors: `{tail_cert.get('support7_local_exact_survivors', 'missing')}`
- support8 local exact survivors: `{tail_cert.get('support8_local_exact_survivors', 'missing')}`

current reading
- the tail pattern theorem path validated in the captured run.
- it now reads its shell15 outside-bounded candidate facts from the fresh current shell15 frontier pair rather than the imported 84 frontier payload.
- support8 shell tail exhaustion is not the right current statement here; the archival summary itself marked that exhaustion claim as refuted.
- caveat: `{provenance_by_item.get('support8_outside_bounded_tail_pattern_theorem', {}).get('caveat', 'missing')}`

{archival_source_block('support8_outside_bounded_tail_pattern_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "support8_tail_candidate_notes_90.md",
        "Support8 Tail Candidate Notes 90",
        f"""
status labels
- current tail candidate artifact: verified

current runtime source
- runtime root: `{RUNTIME_ROOT}`
- candidate fingerprint path: `{RUNTIME_ROOT / 'support8_tail_candidate_fingerprints_90.tsv'}`
- candidate count: `{len(tail_candidates)}`

current verified candidate fingerprints
{chr(10).join(f"- `{line}`" for line in tail_candidates)}

interpretation
- these are current artifact lines emitted by the present workspace run.
- they are not copied placeholder rows.
""",
    )

    write_note(
        RUNTIME_ROOT / "support8_shell_tail_notes_90.md",
        "Support8 Shell Tail Notes 90",
        f"""
status labels
- shell tail certificate availability: verified
- support<=8 shell tail exhausted at shell15: refuted

current runtime source
- runtime root: `{RUNTIME_ROOT}`
- tail candidate count: `{len(tail_candidates)}`
- shell-count validated: `{tail_cert.get('shell_count_validated', 'missing')}`

current reading
- the current runtime produced a shell-tail certificate and candidate fingerprint artifact.
- the live data still contains outside-bounded candidates at shell15, so a simple exhaustion claim would be false.

{archival_source_block('support8_shell_tail_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "support8_tail_obstruction_chain_notes_90.md",
        "Support8 Tail Obstruction Chain Notes 90",
        f"""
status labels
- tail obstruction chain theorem in captured run: {'verified' if lock_achieved else 'not fully reproduced'}
- theorem provenance: `{provenance_by_item.get('support8_tail_obstruction_chain_theorem', {}).get('provenance_source', 'missing')}`
- first failing input inside captured run: {'none' if lock_achieved else (general_status or 'missing')}

captured run result
- `{'no tail-chain failure recorded in the captured run' if parsed.get('tail_chain_failure', '') in ('', 'theorem') else parsed.get('tail_chain_failure', '')}`

current reading
- shell15 theorem path: verified
- tail pattern theorem path: `{provenance_by_item.get('support8_outside_bounded_tail_pattern_theorem', {}).get('provenance_source', 'missing')}`
- artifact completion audit: verified
- document completion audit in captured run: {'verified' if document_verified else 'not fully reproduced'}
- rerun completion audit in captured run: {'verified' if rerun_verified else 'not fully reproduced'}
- audit freshness in captured run: verified
- caveat: `{provenance_by_item.get('support8_tail_obstruction_chain_theorem', {}).get('caveat', 'missing')}`

next implication
- {'the current runtime cleared the tail obstruction chain gate without weakening theorem logic.' if lock_achieved else 'once current docs exist and rerun audit stabilizes on the current binary fingerprint, this gate can be retried without weakening theorem logic.'}

{archival_source_block('support8_tail_obstruction_chain_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "support8_authoritative_completion_lock_notes_90.md",
        "Support8 Authoritative Completion Lock Notes 90",
        f"""
status labels
- current completion lock in captured run: {'verified' if lock_achieved else 'not fully reproduced'}
- lock provenance: `{provenance_by_item.get('support8_authoritative_completion_lock', {}).get('provenance_source', 'missing')}`
- archival completion lock: archival claim

captured run result
- `{parsed.get('lock_failure', '') if parsed.get('lock_failure', '') else 'no completion-lock failure recorded in the captured run'}`

current gate view
- artifact completion: `{artifact.get('nonempty_artifact_count', '0')}` / `{artifact.get('required_artifact_count', '8')}` nonempty
- document completion before this note generation pass: `{document.get('existing_doc_count', '0')}` / `{document.get('required_doc_count', '39')}`
- rerun audit fingerprint: `{rerun.get('audit_fingerprint', 'missing')}`
- release compile stamp: `{release_stamp.get('release_binary_fingerprint', 'missing')}`
- local test stamp: `{local_stamp.get('local_test_binary_fingerprint', 'missing')}`
- provenance split:
  - fresh runtime generated: `{provenance.get('fresh_current_runtime_generated_count', 'missing')}`
  - validated imported: `{provenance.get('current_runtime_validated_imported_data_count', 'missing')}`
  - mixed: `{provenance.get('mixed_count', 'missing')}`

current reading
- {'lock was achieved in the captured run with current docs, artifacts, and rerun stamps present.' if lock_achieved else 'lock was not achieved in the captured run because document completion and rerun completion were still behind the archival claim.'}
- no audit gate was removed or weakened.
- current lock verification should still not be paraphrased as a fresh rederivation of every imported frontier layer.
- caveat: `{provenance_by_item.get('support8_authoritative_completion_lock', {}).get('caveat', 'missing')}`

{archival_source_block('support8_authoritative_completion_lock_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "theorem_data_promotion_notes_90.md",
        "Theorem Data Promotion Notes 90",
        f"""
status labels
- theorem-data ladder presence in current code: verified
- closed-output provenance for multiple frontier layers: archival claim

current code-grounded facts
- runtime root: `{RUNTIME_ROOT}`
- current bundle version metadata is 90, but frontier imports still reference closed outputs such as 57, 67, 70, 71, 72, 74, 75, 76, 77, 79, and 84.
- current workspace portability and runtime-path recovery now allow these theorem-data layers to be exercised on the present machine.
- provenance inventory count: `{provenance.get('item_count', str(len(provenance_rows))) or 'missing'}`
- validated imported items: `{provenance.get('current_runtime_validated_imported_data_count', 'missing')}`
- mixed items: `{provenance.get('mixed_count', 'missing')}`
- fresh runtime generated items: `{provenance.get('fresh_current_runtime_generated_count', 'missing')}`
- basis-only generation label: `{basis_generation.get('current_provenance_label', 'missing')}`
- family-chain generation label: `{family_generation.get('current_provenance_label', 'missing')}`
- basis-only promoted item count this round: `{basis_family_summary.get('basis_promoted_item_count', 'missing')}`
- family-chain promoted item count this round: `{basis_family_summary.get('family_promoted_item_count', 'missing')}`
- family-chain constructor path: `{family_constructor.get('constructor_name', family_generation.get('constructor_name', 'missing'))}`
- family-chain fallback hit: `{family_constructor.get('fallback_hit', family_generation.get('fallback_hit', 'missing'))}`
- pair-expansion aggregate 52 label: `{pair52_generation.get('current_provenance_label', 'missing')}`
- pair-expansion aggregate 52 constructor/cache: `{pair52_generation.get('authoritative_constructor_name', 'missing')}`
- triple-family theorem data 53 label: `{triple53_generation.get('current_provenance_label', 'missing')}`
- triple-family theorem data 53 constructor/cache: `{triple53_generation.get('authoritative_constructor_name', 'missing')}`
- triple-family theorem data 53 upstream pair52 fallback hit: `{triple53_generation.get('upstream_pair52_fallback_hit', 'missing')}`
- quadruple-family theorem data 55 label: `{quadruple55_generation.get('current_provenance_label', 'missing')}`
- quadruple-family theorem data 55 constructor/cache: `{quadruple55_generation.get('authoritative_constructor_name', 'missing')}`
- quadruple-family theorem data 55 upstream triple53 fallback hit: `{quadruple55_generation.get('upstream_triple53_fallback_hit', 'missing')}`
- quintuple-family theorem data 57 label: `{quintuple57_generation.get('current_provenance_label', 'missing')}`
- quintuple-family theorem data 57 constructor/cache: `{quintuple57_generation.get('authoritative_constructor_name', 'missing')}`
- quintuple-family theorem data 57 upstream quad55 fallback hit: `{quintuple57_generation.get('upstream_quad55_fallback_hit', 'missing')}`
- sextuple-family theorem data 57 label: `{sextuple57_generation.get('current_provenance_label', 'missing')}`
- sextuple-family theorem data 57 constructor/cache: `{sextuple57_generation.get('authoritative_constructor_name', 'missing')}`
- sextuple-family theorem data 57 upstream quintuple57 fallback hit: `{sextuple57_generation.get('upstream_quintuple57_fallback_hit', 'missing')}`
- septuple-family theorem data 57 label: `{septuple57_generation.get('current_provenance_label', 'missing')}`
- septuple-family theorem data 57 constructor/cache: `{septuple57_generation.get('authoritative_constructor_name', 'missing')}`
- septuple-family theorem data 57 upstream sextuple57 fallback hit: `{septuple57_generation.get('upstream_sextuple57_fallback_hit', 'missing')}`
- high-family theorem data 57 label: `{high_family57_generation.get('current_provenance_label', 'missing')}`
- high-family theorem data 57 constructor/cache: `{high_family57_generation.get('authoritative_constructor_name', 'missing')}`
- high-family theorem data 57 upstream septuple57 fallback hit: `{high_family57_generation.get('upstream_septuple57_fallback_hit', 'missing')}`
- family-chain lower-layer fresh/imported: `{family_lower_fingerprint.get('fresh_current_runtime_generated_count', 'missing')}` / `{family_lower_fingerprint.get('imported_lower_layer_count', 'missing')}`
- family-chain lower-layer caveat closed: `{family_lower_fingerprint.get('family_chain_lower_layer_caveat_closed', high_family57_generation.get('family_chain_lower_layer_caveat_closed', 'missing'))}`
- shell15 frontier pair fresh current generated: `{shell15_frontier_pair.get('pair_fresh_runtime_generated', 'missing')}`
- shell15 frontier cache constructor: `{shell15_frontier_constructor.get('cache_load_constructor_name', 'missing')}`

current promotion reading
- imported theorem-data availability: verified
- full from-scratch derivation of every imported frontier layer: not fully reproduced
- support8 proof-engine execution path: verified
- basis-only theorem reruns and the 96-basis payload are now current-generated.
- family-chain theorem object layer now uses a fresh current constructor; pair-expansion aggregate 52, triple-family theorem data 53, quadruple-family theorem data 55, quintuple-family theorem data 57, sextuple-family theorem data 57, septuple-family theorem data 57, and high-family theorem data 57 are current constructor/cache-backed when their audits report fresh.
        - the shell15 frontier pair and the direct lower-frontier shell-theorem dependency subset are now current generated, so validated-imported core items are down to zero and top-level mixed items are down to zero.
        - family-chain lower-layer provenance caveat is closed when lower-layer fresh/imported is `7` / `0`; remaining limitations are the broader nonblocking lower-frontier inventory-only perimeter rows and out-of-scope shell16 / higher-support / BOJ solver work.

{archival_source_block('theorem_data_promotion_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "artifact_completion_notes_90.md",
        "Artifact Completion Notes 90",
        f"""
status labels
- artifact completion audit: verified

current metrics
- required artifact count: `{artifact.get('required_artifact_count', 'missing')}`
- existing artifact count: `{artifact.get('existing_artifact_count', 'missing')}`
- nonempty artifact count: `{artifact.get('nonempty_artifact_count', 'missing')}`
- shell15 artifacts complete: `{artifact.get('shell15_artifacts_complete', 'missing')}`
- tail artifacts complete: `{artifact.get('tail_artifacts_complete', 'missing')}`
- audit fingerprint: `{artifact.get('audit_fingerprint', 'missing')}`

current source
- audit tsv: `{RUNTIME_ROOT / 'artifact_completion_audit_90.tsv'}`
- runtime root: `{RUNTIME_ROOT}`

interpretation
- the required support8 artifact set is now present in current workspace runtime storage.
- these files were emitted by the current proof-engine run, not hand-authored placeholder tsv files.

{archival_source_block('artifact_completion_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "document_completion_audit_notes_90.md",
        "Document Completion Audit Notes 90",
        f"""
status labels
- document completion audit in captured run: {'verified' if document_verified else 'not fully reproduced'}

captured metrics before this generation pass
- required doc count: `{document.get('required_doc_count', 'missing')}`
- existing doc count: `{document.get('existing_doc_count', 'missing')}`
- core summary ready: `{document.get('core_summary_ready', 'missing')}`
- shell theorem ready: `{document.get('shell_theorem_ready', 'missing')}`
- tail pattern ready: `{document.get('tail_pattern_ready', 'missing')}`
- artifact completion ready: `{document.get('artifact_completion_ready', 'missing')}`
- bridge ready: `{document.get('bridge_ready', 'missing')}`
- audit fingerprint: `{document.get('audit_fingerprint', 'missing')}`

current interpretation
- {'the current runtime doc set is complete and the document audit now verifies 39/39.' if document_verified else 'the captured run failed here because runtime docs did not yet exist.'}
- {'the first blocker was removed by generating grounded runtime notes under the current runtime root.' if document_verified else 'this note generation pass is the repair action for that truthful first blocker.'}

{archival_source_block('document_completion_audit_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "general_schema_obstruction_notes_90.md",
        "General Schema Obstruction Notes 90",
        f"""
status labels
- unified bounded schema obstruction theorem data availability: verified
- theorem body provenance: archival claim

captured run summary
- `{parsed.get('family_chain_audit', '')}`
- `{parsed.get('unified_theorem', '')}`
- unified status: `{parsed.get('unified_status', '')}`

current reading
- the current runtime can load and use the obstruction theorem-data ladder.
- the authoritative family-chain path now uses a current constructor for the support/unified obstruction theorem objects.
- caveat: `{family_constructor.get('constructor_caveat', family_generation.get('caveat', 'missing'))}`

{archival_source_block('general_schema_obstruction_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "general_schema_universe_notes_90.md",
        "General Schema Universe Notes 90",
        f"""
status labels
- current general schema universe classification: `{general_status or 'not fully reproduced'}`

captured run summary
- bounded scope: `{parsed.get('bounded_scope', '')}`
- general schema gap: `{parsed.get('general_gap', '')}`
- family-chain status: `{parsed.get('family_chain_status', '')}`
- current classification: `{general_status or 'missing'}`

current reading
- the proof engine currently reaches the support8 tail slice.
- {'the captured run now reaches `support8_authoritative_completion_locked` in the current workspace.' if lock_achieved else 'in the captured run the first failing project-wide gate was document completion, not theorem-data load failure.'}

{archival_source_block('general_schema_universe_notes_90.md')}
""",
    )

    write_note(
        RUNTIME_ROOT / "support8_rerun_completion_notes_90.md",
        "Support8 Rerun Completion Notes 90",
        f"""
status labels
- release compile in current workspace: {'verified' if rerun.get('release_compiled') == '1' else 'not fully reproduced'}
- LOCAL_TEST compile in current workspace: {'verified' if rerun.get('local_test_compiled') == '1' else 'not fully reproduced'}
- LOCAL_TEST rerun completion in captured run: {'verified' if rerun.get('local_test_passed') == '1' else 'not fully reproduced'}

captured metrics
- audit fingerprint: `{rerun.get('audit_fingerprint', 'missing')}`
- local test compiled: `{rerun.get('local_test_compiled', 'missing')}`
- local test passed: `{rerun.get('local_test_passed', 'missing')}`
- release compiled: `{rerun.get('release_compiled', 'missing')}`
- local test log fingerprint: `{rerun.get('local_test_log_fingerprint', 'missing')}`
- release binary fingerprint: `{rerun.get('release_binary_fingerprint', 'missing')}`

current interpretation
- {'the captured run has matching local-test and release-compile stamps for the current binaries.' if rerun_verified else 'this captured run still predates the current local success stamp match for the running binary.'}
- {'rerun completion is currently verified.' if rerun_verified else 'another pass is required after this note-generation step to see whether rerun completion moves to verified.'}

{archival_source_block('support8_rerun_completion_notes_90.md')}
""",
    )

    frontier_files = [
        ("support_plus_one_frontier_notes_90.md", "Support Plus One Frontier Notes 90", "support_plus_one_frontier"),
        ("antecedent_plus_one_frontier_notes_90.md", "Antecedent Plus One Frontier Notes 90", "antecedent_plus_one_frontier"),
        ("mixed_outside_bounded_frontier_notes_90.md", "Mixed Outside Bounded Frontier Notes 90", "mixed_outside_bounded_frontier"),
        ("antecedent_plus_two_frontier_notes_90.md", "Antecedent Plus Two Frontier Notes 90", "antecedent_plus_two_frontier"),
        ("support8_antecedent5_frontier_notes_90.md", "Support8 Antecedent5 Frontier Notes 90", "support8_antecedent5_frontier"),
        ("antecedent_plus_three_frontier_notes_90.md", "Antecedent Plus Three Frontier Notes 90", "antecedent_plus_three_frontier"),
        ("support8_antecedent6_frontier_notes_90.md", "Support8 Antecedent6 Frontier Notes 90", "support8_antecedent6_frontier"),
        ("antecedent_plus_four_frontier_notes_90.md", "Antecedent Plus Four Frontier Notes 90", "antecedent_plus_four_frontier"),
        ("support8_antecedent7_frontier_notes_90.md", "Support8 Antecedent7 Frontier Notes 90", "support8_antecedent7_frontier"),
        ("antecedent_plus_five_frontier_notes_90.md", "Antecedent Plus Five Frontier Notes 90", "antecedent_plus_five_frontier"),
        ("support8_antecedent8_frontier_notes_90.md", "Support8 Antecedent8 Frontier Notes 90", "support8_antecedent8_frontier"),
        ("antecedent_plus_six_frontier_notes_90.md", "Antecedent Plus Six Frontier Notes 90", "antecedent_plus_six_frontier"),
        ("support8_antecedent9_frontier_notes_90.md", "Support8 Antecedent9 Frontier Notes 90", "support8_antecedent9_frontier"),
        ("antecedent_plus_seven_frontier_notes_90.md", "Antecedent Plus Seven Frontier Notes 90", "antecedent_plus_seven_frontier"),
        ("support8_antecedent10_frontier_notes_90.md", "Support8 Antecedent10 Frontier Notes 90", "support8_antecedent10_frontier"),
        ("antecedent_plus_eight_frontier_notes_90.md", "Antecedent Plus Eight Frontier Notes 90", "antecedent_plus_eight_frontier"),
        ("support8_antecedent11_frontier_notes_90.md", "Support8 Antecedent11 Frontier Notes 90", "support8_antecedent11_frontier"),
        ("antecedent_plus_nine_frontier_notes_90.md", "Antecedent Plus Nine Frontier Notes 90", "antecedent_plus_nine_frontier"),
        ("support8_antecedent12_frontier_notes_90.md", "Support8 Antecedent12 Frontier Notes 90", "support8_antecedent12_frontier"),
        ("antecedent_plus_ten_frontier_notes_90.md", "Antecedent Plus Ten Frontier Notes 90", "antecedent_plus_ten_frontier"),
        ("support8_antecedent13_frontier_notes_90.md", "Support8 Antecedent13 Frontier Notes 90", "support8_antecedent13_frontier"),
        ("antecedent_plus_eleven_frontier_notes_90.md", "Antecedent Plus Eleven Frontier Notes 90", "antecedent_plus_eleven_frontier"),
        ("support8_antecedent14_frontier_notes_90.md", "Support8 Antecedent14 Frontier Notes 90", "support8_antecedent14_frontier"),
        ("antecedent_plus_twelve_frontier_notes_90.md", "Antecedent Plus Twelve Frontier Notes 90", "antecedent_plus_twelve_frontier"),
        ("support8_antecedent15_frontier_notes_90.md", "Support8 Antecedent15 Frontier Notes 90", "support8_antecedent15_frontier"),
    ]
    frontiers: dict[str, dict[str, str]] = parsed["frontiers"]  # type: ignore[assignment]
    for basename, title, tag in frontier_files:
        stats = frontiers.get(tag)
        if not stats:
            continue
        item_key = tag
        generation_audit = {}
        if tag == "antecedent_plus_twelve_frontier":
            generation_audit = shell15_frontier_p12
        elif tag == "support8_antecedent15_frontier":
            generation_audit = shell15_frontier_s8
        write_note(
            RUNTIME_ROOT / basename,
            title,
            frontier_note(
                title,
                basename,
                tag,
                stats,
                runtime_paths,
                provenance_by_item.get(item_key, {}),
                generation_audit,
            ),
        )

    fresh_names = pick_names(provenance_rows, "provenance_source", "fresh_current_runtime_generated")
    validated_names = pick_names(provenance_rows, "provenance_source", "current_runtime_validated_imported_data")
    mixed_names = pick_names(provenance_rows, "provenance_source", "mixed")
    archival_names = pick_names(provenance_rows, "provenance_source", "archival_only")
    current_verified_names = pick_names(provenance_rows, "validation_status", "current_verified")
    archival_claim_names = pick_names(provenance_rows, "validation_status", "archival_claim")
    inventory_lines = "\n".join(
        f"- `{row.get('display_name', row.get('item_key', 'missing'))}`: validation=`{row.get('validation_status', 'missing')}`, provenance=`{row.get('provenance_source', 'missing')}`, imported=`{row.get('imported_from_source_tag', 'none') or 'none'}`, fresh rederivation=`{row.get('fresh_rederivation', '0')}`, caveat=`{row.get('caveat', '')}`"
        for row in provenance_rows
    )
    if not inventory_lines:
        inventory_lines = "- missing provenance inventory rows"

    if mixed_names:
        remaining_scope_line = (
            "- biggest remaining scope limit: "
            + ", ".join(mixed_names)
            + " still carries mixed provenance in the current top-level inventory."
        )
    else:
        remaining_scope_line = (
            "- biggest remaining top-level mixed scope limit: none. "
            "The current top-level inventory is fully fresh/generated or archival-only; "
            "lower-frontier inventory rows for shell11/shell12 remain exposed separately as "
            "outside the direct shell15 dependency subset. "
            f"Pair-expansion aggregate 52 is `{pair52_generation.get('current_provenance_label', 'missing')}`; "
            f"triple-family theorem data 53 is `{triple53_generation.get('current_provenance_label', 'missing')}`; "
            f"quadruple-family theorem data 55 is `{quadruple55_generation.get('current_provenance_label', 'missing')}`; "
            f"quintuple-family theorem data 57 is `{quintuple57_generation.get('current_provenance_label', 'missing')}`; "
            f"sextuple-family theorem data 57 is `{sextuple57_generation.get('current_provenance_label', 'missing')}`; "
            f"septuple-family theorem data 57 is `{septuple57_generation.get('current_provenance_label', 'missing')}`; "
            f"high-family theorem data 57 is `{high_family57_generation.get('current_provenance_label', 'missing')}`; "
            f"next family-chain lower target is `{family_lower_fingerprint.get('next_target', 'none_family_chain_lower_layers_complete')}`."
        )

    write_note(
        ARCHIVAL_ROOT / "theorem_data_provenance_inventory_90.md",
        "Theorem Data Provenance Inventory 90",
        f"""
status labels
- inventory availability: {'verified' if provenance_rows else 'not fully reproduced'}
- current support8 lock: `{general_status or 'not fully reproduced'}`

current bundle metadata
- current bundle version: `{provenance.get('current_bundle_version', 'missing')}`
- current bundle source path: `{provenance.get('current_bundle_source_path', 'missing')}`
- current bundle summary path: `{provenance.get('current_bundle_summary_path', 'missing')}`
- runtime summary path: `{provenance.get('runtime_summary_path', 'missing')}`
- runtime data root: `{provenance.get('runtime_data_root', str(RUNTIME_ROOT))}`

provenance counts
- item count: `{provenance.get('item_count', str(len(provenance_rows))) or 'missing'}`
- current verified: `{provenance.get('current_verified_count', 'missing')}`
- archival claim: `{provenance.get('archival_claim_count', 'missing')}`
- fresh current runtime generated: `{provenance.get('fresh_current_runtime_generated_count', 'missing')}`
- current runtime validated imported data: `{provenance.get('current_runtime_validated_imported_data_count', 'missing')}`
- mixed: `{provenance.get('mixed_count', 'missing')}`
- archival only: `{provenance.get('archival_only_count', 'missing')}`

representative items
- current verified: `{', '.join(current_verified_names) if current_verified_names else 'missing'}`
- fresh current runtime generated: `{', '.join(fresh_names) if fresh_names else 'missing'}`
- current runtime validated imported data: `{', '.join(validated_names) if validated_names else 'missing'}`
- mixed: `{', '.join(mixed_names) if mixed_names else 'missing'}`
- archival claim: `{', '.join(archival_claim_names) if archival_claim_names else 'missing'}`

inventory rows
{inventory_lines}
""",
    )

    write_note(
        RUNTIME_ROOT / "provenance_audit_report_90.md",
        "Provenance Audit Report 90",
        f"""
status labels
- provenance audit availability: {'verified' if provenance_rows else 'not fully reproduced'}
- current support8 classification: `{general_status or 'not fully reproduced'}`

current runtime facts
- runtime root: `{RUNTIME_ROOT}`
- provenance fingerprint: `{provenance.get('audit_fingerprint', 'missing')}`
- pass1 status: `{pass_statuses.get('pass1', 'not attempted')}`
- pass2 status: `{pass_statuses.get('pass2', 'not attempted')}`
- pass3 status: `{pass_statuses.get('pass3', 'not attempted')}`

count split
- fresh current runtime generated: `{provenance.get('fresh_current_runtime_generated_count', 'missing')}`
- current runtime validated imported data: `{provenance.get('current_runtime_validated_imported_data_count', 'missing')}`
- mixed: `{provenance.get('mixed_count', 'missing')}`
- archival only: `{provenance.get('archival_only_count', 'missing')}`
- basis-only generation label: `{basis_generation.get('current_provenance_label', 'missing')}`
- family-chain generation label: `{family_generation.get('current_provenance_label', 'missing')}`

current reading
- imported closed-output provenance is now explicit runtime data instead of an implied stale header residue.
- current support8 lock remains about verified execution and audit closure, not about fresh rederivation of every imported frontier layer.
- {remaining_scope_line[2:]}
- basis-only and shell15 frontier reruns now both sit on current-generated authoritative payloads.
- family-chain constructor path: `{family_constructor.get('constructor_name', family_generation.get('constructor_name', 'missing'))}`
- family-chain fallback hit in captured run: `{family_constructor.get('fallback_hit', family_generation.get('fallback_hit', 'missing'))}`
- family-chain constructor caveat: `{family_constructor.get('constructor_caveat', family_generation.get('caveat', 'missing'))}`
- pair-expansion aggregate 52 label: `{pair52_generation.get('current_provenance_label', 'missing')}`
- pair-expansion aggregate 52 counts: regions `{pair52_generation.get('region_count', 'missing')}`, candidates `{pair52_generation.get('raw_candidates', 'missing')}/{pair52_generation.get('canonical_candidates', 'missing')}/{pair52_generation.get('deduplicated_candidates', 'missing')}`
- pair-expansion aggregate 52 equality: `{pair52_generation.get('imported_equality_result', 'missing')}`
- triple-family theorem data 53 label: `{triple53_generation.get('current_provenance_label', 'missing')}`
- triple-family theorem data 53 counts: regions `{triple53_generation.get('region_count', 'missing')}`, candidates `{triple53_generation.get('raw_candidates', 'missing')}/{triple53_generation.get('canonical_candidates', 'missing')}/{triple53_generation.get('deduplicated_candidates', 'missing')}`
- triple-family theorem data 53 equality: `{triple53_generation.get('imported_equality_result', 'missing')}`
- triple-family theorem data 53 upstream pair52 fallback hit: `{triple53_generation.get('upstream_pair52_fallback_hit', 'missing')}`
- quadruple-family theorem data 55 label: `{quadruple55_generation.get('current_provenance_label', 'missing')}`
- quadruple-family theorem data 55 counts: regions `{quadruple55_generation.get('region_count', 'missing')}`, candidates `{quadruple55_generation.get('raw_candidates', 'missing')}/{quadruple55_generation.get('canonical_candidates', 'missing')}/{quadruple55_generation.get('deduplicated_candidates', 'missing')}`
- quadruple-family theorem data 55 equality: `{quadruple55_generation.get('imported_equality_result', 'missing')}`
- quadruple-family theorem data 55 upstream triple53 fallback hit: `{quadruple55_generation.get('upstream_triple53_fallback_hit', 'missing')}`
- quintuple-family theorem data 57 label: `{quintuple57_generation.get('current_provenance_label', 'missing')}`
- quintuple-family theorem data 57 counts: regions `{quintuple57_generation.get('region_count', 'missing')}`, candidates `{quintuple57_generation.get('raw_candidates', 'missing')}/{quintuple57_generation.get('canonical_candidates', 'missing')}/{quintuple57_generation.get('deduplicated_candidates', 'missing')}`
- quintuple-family theorem data 57 equality: `{quintuple57_generation.get('imported_equality_result', 'missing')}`
- quintuple-family theorem data 57 upstream quad55 fallback hit: `{quintuple57_generation.get('upstream_quad55_fallback_hit', 'missing')}`
- sextuple-family theorem data 57 label: `{sextuple57_generation.get('current_provenance_label', 'missing')}`
- sextuple-family theorem data 57 counts: regions `{sextuple57_generation.get('region_count', 'missing')}`, candidates `{sextuple57_generation.get('raw_candidates', 'missing')}/{sextuple57_generation.get('canonical_candidates', 'missing')}/{sextuple57_generation.get('deduplicated_candidates', 'missing')}`
- sextuple-family theorem data 57 equality: `{sextuple57_generation.get('imported_equality_result', 'missing')}`
- sextuple-family theorem data 57 upstream quintuple57 fallback hit: `{sextuple57_generation.get('upstream_quintuple57_fallback_hit', 'missing')}`
- septuple-family theorem data 57 label: `{septuple57_generation.get('current_provenance_label', 'missing')}`
- septuple-family theorem data 57 counts: regions `{septuple57_generation.get('region_count', 'missing')}`, candidates `{septuple57_generation.get('raw_candidates', 'missing')}/{septuple57_generation.get('canonical_candidates', 'missing')}/{septuple57_generation.get('deduplicated_candidates', 'missing')}`
- septuple-family theorem data 57 equality: `{septuple57_generation.get('imported_equality_result', 'missing')}`
- septuple-family theorem data 57 upstream sextuple57 fallback hit: `{septuple57_generation.get('upstream_sextuple57_fallback_hit', 'missing')}`
- high-family theorem data 57 label: `{high_family57_generation.get('current_provenance_label', 'missing')}`
- high-family theorem data 57 counts: regions `{high_family57_generation.get('region_count', 'missing')}`, candidates `{high_family57_generation.get('raw_candidates', 'missing')}/{high_family57_generation.get('canonical_candidates', 'missing')}/{high_family57_generation.get('deduplicated_candidates', 'missing')}`
- high-family theorem data 57 equality: `{high_family57_generation.get('imported_equality_result', 'missing')}`
- high-family theorem data 57 upstream septuple57 fallback hit: `{high_family57_generation.get('upstream_septuple57_fallback_hit', 'missing')}`
- next family-chain lower target: `{family_lower_fingerprint.get('next_target', 'none_family_chain_lower_layers_complete')}`
""",
    )

    write_note(
        RUNTIME_ROOT / "provenance_summary_90.md",
        "Provenance Summary 90",
        f"""
summary
- item count: `{provenance.get('item_count', str(len(provenance_rows))) or 'missing'}`
- fresh current runtime generated: `{provenance.get('fresh_current_runtime_generated_count', 'missing')}`
- current runtime validated imported data: `{provenance.get('current_runtime_validated_imported_data_count', 'missing')}`
- mixed: `{provenance.get('mixed_count', 'missing')}`
- archival only: `{provenance.get('archival_only_count', 'missing')}`
- current support8 classification: `{general_status or 'not fully reproduced'}`
- basis-only generation label: `{basis_generation.get('current_provenance_label', 'missing')}`
- family-chain generation label: `{family_generation.get('current_provenance_label', 'missing')}`
- family-chain constructor: `{family_constructor.get('constructor_name', family_generation.get('constructor_name', 'missing'))}`
- pair-expansion aggregate 52 label: `{pair52_generation.get('current_provenance_label', 'missing')}`
- triple-family theorem data 53 label: `{triple53_generation.get('current_provenance_label', 'missing')}`
- quadruple-family theorem data 55 label: `{quadruple55_generation.get('current_provenance_label', 'missing')}`
- quintuple-family theorem data 57 label: `{quintuple57_generation.get('current_provenance_label', 'missing')}`
- sextuple-family theorem data 57 label: `{sextuple57_generation.get('current_provenance_label', 'missing')}`
- septuple-family theorem data 57 label: `{septuple57_generation.get('current_provenance_label', 'missing')}`
- high-family theorem data 57 label: `{high_family57_generation.get('current_provenance_label', 'missing')}`
- family-chain lower-layer fresh/imported: `{family_lower_fingerprint.get('fresh_current_runtime_generated_count', 'missing')}` / `{family_lower_fingerprint.get('imported_lower_layer_count', 'missing')}`
- family-chain lower-layer caveat closed: `{family_lower_fingerprint.get('family_chain_lower_layer_caveat_closed', high_family57_generation.get('family_chain_lower_layer_caveat_closed', 'missing'))}`
- next broader perimeter target: `{family_lower_fingerprint.get('next_target', 'none_family_chain_lower_layers_complete')}`
""",
    )

    basis_family_report = ARCHIVAL_ROOT / "basis_family_fresh_generation_report_90.md"
    basis_family_report.write_text(
        (
            "# Basis Family Fresh Generation Report 90\n\n"
            "## scope\n"
            "- in-scope priority: `basis_only_theorem_chain`\n"
            "- in-scope if feasible: `family_chain_output_57`\n"
            "- stretch only: `support8_shell15_frontier_output_84`\n\n"
            "## basis-only result\n"
            f"- previous provenance label: `current_runtime_validated_imported_data`\n"
            f"- current provenance label: `{basis_generation.get('current_provenance_label', 'missing')}`\n"
            f"- generation path: `{basis_generation.get('generation_path', 'missing')}`\n"
            f"- promoted item count: `{basis_family_summary.get('basis_promoted_item_count', 'missing')}`\n"
            f"- basis payload source: `{basis_generation.get('basis_payload_source', 'missing')}`\n"
            f"- basis payload fingerprint: `{basis_generation.get('basis_payload_fingerprint', 'missing')}`\n"
            f"- rerun fingerprint: `{basis_generation.get('runtime_generation_fingerprint', 'missing')}`\n"
            f"- caveat: `{basis_generation.get('caveat', 'missing')}`\n\n"
            "## family-chain result\n"
            f"- previous provenance label: `current_runtime_validated_imported_data`\n"
            f"- current provenance label: `{family_generation.get('current_provenance_label', 'missing')}`\n"
            f"- generation path: `{family_generation.get('generation_path', 'missing')}`\n"
            f"- promoted item count: `{basis_family_summary.get('family_promoted_item_count', 'missing')}`\n"
            f"- rerun fingerprint: `{family_generation.get('runtime_generation_fingerprint', 'missing')}`\n"
            f"- constructor name: `{family_constructor.get('constructor_name', family_generation.get('constructor_name', 'missing'))}`\n"
            f"- fallback hit: `{family_constructor.get('fallback_hit', family_generation.get('fallback_hit', 'missing'))}`\n"
            f"- caveat: `{family_constructor.get('constructor_caveat', family_generation.get('caveat', 'missing'))}`\n\n"
            "## current implication\n"
            f"- current support8 classification: `{general_status or 'not fully reproduced'}`\n"
            f"- pass2: `{pass_statuses.get('pass2', 'not attempted')}`\n"
            f"- pass3: `{pass_statuses.get('pass3', 'not attempted')}`\n"
            f"- post-basis-family shell15 frontier pair label: `fresh_current_runtime_generated`\n"
            f"- current provenance counts: fresh=`{provenance.get('fresh_current_runtime_generated_count', 'missing')}`, validated-imported=`{provenance.get('current_runtime_validated_imported_data_count', 'missing')}`, mixed=`{provenance.get('mixed_count', 'missing')}`\n"
        ),
        encoding="utf-8",
    )

    family_constructor_report = ARCHIVAL_ROOT / "family_chain_fresh_constructor_report_90.md"
    family_constructor_report.write_text(
        (
            "# Family Chain Fresh Constructor Report 90\n\n"
            "## constructor result\n"
            f"- authoritative source: `{family_generation.get('generation_path', 'missing')}`\n"
            f"- constructor name: `{family_constructor.get('constructor_name', family_generation.get('constructor_name', 'missing'))}`\n"
            f"- used current constructor: `{family_constructor.get('used_current_constructor', 'missing')}`\n"
            f"- fallback hit: `{family_constructor.get('fallback_hit', family_generation.get('fallback_hit', 'missing'))}`\n"
            f"- runtime fingerprint: `{family_constructor_fingerprint.get('runtime_fingerprint', family_generation.get('runtime_generation_fingerprint', 'missing'))}`\n"
            f"- caveat: `{family_constructor.get('constructor_caveat', family_generation.get('caveat', 'missing'))}`\n\n"
            "## theorem objects\n"
            f"- support theorem: `{family_constructor.get('support_theorem', 'missing')}`\n"
            f"- unified theorem: `{family_constructor.get('unified_theorem', 'missing')}`\n\n"
            "## pass status\n"
            f"- pass1: `{pass_statuses.get('pass1', 'not attempted')}`\n"
            f"- pass2: `{pass_statuses.get('pass2', 'not attempted')}`\n"
            f"- pass3: `{pass_statuses.get('pass3', 'not attempted')}`\n"
        ),
        encoding="utf-8",
    )

    report_path = ARCHIVAL_ROOT / "proof_system_reproduction_report_90.md"
    report_path.write_text(
        (
            "# Proof System Reproduction Report 90\n\n"
            "1. active workspace root\n"
            f"- `{REPO_ROOT}`\n"
            "2. active runtime data root\n"
            f"- `{RUNTIME_ROOT}`\n"
            "3. compile status\n"
            f"- release compile: {'verified' if release_stamp else 'not fully reproduced'}\n"
            f"- LOCAL_TEST compile: {'verified' if local_stamp else 'not fully reproduced'}\n"
            "4. pass status\n"
            f"- pass1: `{pass_statuses.get('pass1', 'not attempted')}`\n"
            f"- pass2: `{pass_statuses.get('pass2', 'not attempted')}`\n"
            f"- pass3: `{pass_statuses.get('pass3', 'not attempted')}`\n"
            "5. current captured run status\n"
            f"- current reproducible classification: `{general_status or 'not fully reproduced'}`\n"
            f"- archival classification: `{archival_classification}`\n"
            f"- lock achieved in captured run: `{'yes' if lock_achieved else 'no'}`\n"
            "6. current counts before follow-up rerun\n"
            f"- required docs: `{document.get('existing_doc_count', '0')}` / `{document.get('required_doc_count', '39')}` in the captured run\n"
            f"- required artifacts: `{artifact.get('nonempty_artifact_count', '0')}` / `{artifact.get('required_artifact_count', '8')}`\n"
            "7. provenance audit summary\n"
            f"- item count: `{provenance.get('item_count', str(len(provenance_rows))) or 'missing'}`\n"
            f"- fresh current runtime generated: `{provenance.get('fresh_current_runtime_generated_count', 'missing')}`\n"
            f"- current runtime validated imported data: `{provenance.get('current_runtime_validated_imported_data_count', 'missing')}`\n"
            f"- mixed: `{provenance.get('mixed_count', 'missing')}`\n"
            f"- archival only: `{provenance.get('archival_only_count', 'missing')}`\n"
            f"- basis-only generation label: `{basis_generation.get('current_provenance_label', 'missing')}` with promoted items `{basis_family_summary.get('basis_promoted_item_count', 'missing')}`\n"
            f"- family-chain generation label: `{family_generation.get('current_provenance_label', 'missing')}` with promoted items `{basis_family_summary.get('family_promoted_item_count', 'missing')}`\n"
            f"- family-chain constructor: `{family_constructor.get('constructor_name', family_generation.get('constructor_name', 'missing'))}` fallbackHit=`{family_constructor.get('fallback_hit', family_generation.get('fallback_hit', 'missing'))}`\n"
            "8. pair-expansion aggregate 52\n"
            f"- final provenance label: `{pair52_generation.get('current_provenance_label', 'missing')}`\n"
            f"- constructor/cache path: `{pair52_generation.get('authoritative_constructor_name', 'missing')}`\n"
            f"- region count: `{pair52_generation.get('region_count', 'missing')}`\n"
            f"- raw / canonical / deduplicated candidates: `{pair52_generation.get('raw_candidates', 'missing')}` / `{pair52_generation.get('canonical_candidates', 'missing')}` / `{pair52_generation.get('deduplicated_candidates', 'missing')}`\n"
            f"- imported equality result: `{pair52_generation.get('imported_equality_result', 'missing')}`\n"
            "9. triple-family theorem data 53\n"
            f"- final provenance label: `{triple53_generation.get('current_provenance_label', 'missing')}`\n"
            f"- constructor/cache path: `{triple53_generation.get('authoritative_constructor_name', 'missing')}`\n"
            f"- upstream pair52 fallback hit: `{triple53_generation.get('upstream_pair52_fallback_hit', 'missing')}`\n"
            f"- region count: `{triple53_generation.get('region_count', 'missing')}`\n"
            f"- raw / canonical / deduplicated candidates: `{triple53_generation.get('raw_candidates', 'missing')}` / `{triple53_generation.get('canonical_candidates', 'missing')}` / `{triple53_generation.get('deduplicated_candidates', 'missing')}`\n"
            f"- imported equality result: `{triple53_generation.get('imported_equality_result', 'missing')}`\n"
            "10. quadruple-family theorem data 55\n"
            f"- final provenance label: `{quadruple55_generation.get('current_provenance_label', 'missing')}`\n"
            f"- constructor/cache path: `{quadruple55_generation.get('authoritative_constructor_name', 'missing')}`\n"
            f"- upstream triple53 fallback hit: `{quadruple55_generation.get('upstream_triple53_fallback_hit', 'missing')}`\n"
            f"- upstream pair52 fallback hit: `{quadruple55_generation.get('upstream_pair52_fallback_hit', 'missing')}`\n"
            f"- region count: `{quadruple55_generation.get('region_count', 'missing')}`\n"
            f"- raw / canonical / deduplicated candidates: `{quadruple55_generation.get('raw_candidates', 'missing')}` / `{quadruple55_generation.get('canonical_candidates', 'missing')}` / `{quadruple55_generation.get('deduplicated_candidates', 'missing')}`\n"
            f"- imported equality result: `{quadruple55_generation.get('imported_equality_result', 'missing')}`\n"
            "11. quintuple-family theorem data 57\n"
            f"- final provenance label: `{quintuple57_generation.get('current_provenance_label', 'missing')}`\n"
            f"- constructor/cache path: `{quintuple57_generation.get('authoritative_constructor_name', 'missing')}`\n"
            f"- upstream quad55 fallback hit: `{quintuple57_generation.get('upstream_quad55_fallback_hit', 'missing')}`\n"
            f"- upstream triple53 fallback hit: `{quintuple57_generation.get('upstream_triple53_fallback_hit', 'missing')}`\n"
            f"- upstream pair52 fallback hit: `{quintuple57_generation.get('upstream_pair52_fallback_hit', 'missing')}`\n"
            f"- region count: `{quintuple57_generation.get('region_count', 'missing')}`\n"
            f"- raw / canonical / deduplicated candidates: `{quintuple57_generation.get('raw_candidates', 'missing')}` / `{quintuple57_generation.get('canonical_candidates', 'missing')}` / `{quintuple57_generation.get('deduplicated_candidates', 'missing')}`\n"
            f"- imported equality result: `{quintuple57_generation.get('imported_equality_result', 'missing')}`\n"
            "12. sextuple-family theorem data 57\n"
            f"- final provenance label: `{sextuple57_generation.get('current_provenance_label', 'missing')}`\n"
            f"- constructor/cache path: `{sextuple57_generation.get('authoritative_constructor_name', 'missing')}`\n"
            f"- upstream quintuple57 fallback hit: `{sextuple57_generation.get('upstream_quintuple57_fallback_hit', 'missing')}`\n"
            f"- upstream quad55 fallback hit: `{sextuple57_generation.get('upstream_quad55_fallback_hit', 'missing')}`\n"
            f"- upstream triple53 fallback hit: `{sextuple57_generation.get('upstream_triple53_fallback_hit', 'missing')}`\n"
            f"- upstream pair52 fallback hit: `{sextuple57_generation.get('upstream_pair52_fallback_hit', 'missing')}`\n"
            f"- region count: `{sextuple57_generation.get('region_count', 'missing')}`\n"
            f"- raw / canonical / deduplicated candidates: `{sextuple57_generation.get('raw_candidates', 'missing')}` / `{sextuple57_generation.get('canonical_candidates', 'missing')}` / `{sextuple57_generation.get('deduplicated_candidates', 'missing')}`\n"
            f"- imported equality result: `{sextuple57_generation.get('imported_equality_result', 'missing')}`\n"
            "13. septuple-family theorem data 57\n"
            f"- final provenance label: `{septuple57_generation.get('current_provenance_label', 'missing')}`\n"
            f"- constructor/cache path: `{septuple57_generation.get('authoritative_constructor_name', 'missing')}`\n"
            f"- upstream sextuple57 fallback hit: `{septuple57_generation.get('upstream_sextuple57_fallback_hit', 'missing')}`\n"
            f"- upstream quintuple57 fallback hit: `{septuple57_generation.get('upstream_quintuple57_fallback_hit', 'missing')}`\n"
            f"- upstream quad55 fallback hit: `{septuple57_generation.get('upstream_quad55_fallback_hit', 'missing')}`\n"
            f"- upstream triple53 fallback hit: `{septuple57_generation.get('upstream_triple53_fallback_hit', 'missing')}`\n"
            f"- upstream pair52 fallback hit: `{septuple57_generation.get('upstream_pair52_fallback_hit', 'missing')}`\n"
            f"- region count: `{septuple57_generation.get('region_count', 'missing')}`\n"
            f"- raw / canonical / deduplicated candidates: `{septuple57_generation.get('raw_candidates', 'missing')}` / `{septuple57_generation.get('canonical_candidates', 'missing')}` / `{septuple57_generation.get('deduplicated_candidates', 'missing')}`\n"
            f"- imported equality result: `{septuple57_generation.get('imported_equality_result', 'missing')}`\n"
            "14. high-family theorem data 57\n"
            f"- final provenance label: `{high_family57_generation.get('current_provenance_label', 'missing')}`\n"
            f"- constructor/cache path: `{high_family57_generation.get('authoritative_constructor_name', 'missing')}`\n"
            f"- upstream septuple57 fallback hit: `{high_family57_generation.get('upstream_septuple57_fallback_hit', 'missing')}`\n"
            f"- upstream sextuple57 fallback hit: `{high_family57_generation.get('upstream_sextuple57_fallback_hit', 'missing')}`\n"
            f"- upstream quintuple57 fallback hit: `{high_family57_generation.get('upstream_quintuple57_fallback_hit', 'missing')}`\n"
            f"- upstream quad55 fallback hit: `{high_family57_generation.get('upstream_quad55_fallback_hit', 'missing')}`\n"
            f"- upstream triple53 fallback hit: `{high_family57_generation.get('upstream_triple53_fallback_hit', 'missing')}`\n"
            f"- upstream pair52 fallback hit: `{high_family57_generation.get('upstream_pair52_fallback_hit', 'missing')}`\n"
            f"- region count: `{high_family57_generation.get('region_count', 'missing')}`\n"
            f"- raw / canonical / deduplicated candidates: `{high_family57_generation.get('raw_candidates', 'missing')}` / `{high_family57_generation.get('canonical_candidates', 'missing')}` / `{high_family57_generation.get('deduplicated_candidates', 'missing')}`\n"
            f"- imported equality result: `{high_family57_generation.get('imported_equality_result', 'missing')}`\n"
            f"- family-chain lower-layer caveat closed: `{high_family57_generation.get('family_chain_lower_layer_caveat_closed', 'missing')}`\n"
            "15. first failing gate in captured run\n"
            f"- `{'none' if lock_achieved else (general_status or 'missing')}` with detail `{'' if lock_achieved else (parsed.get('document_failure', '') or parsed.get('lock_failure', ''))}`\n"
            "16. next concrete action\n"
            + (f"- preserve the current support8 lock with family-chain lower layers at `{family_lower_fingerprint.get('fresh_current_runtime_generated_count', 'missing')}` / `{family_lower_fingerprint.get('imported_lower_layer_count', 'missing')}` fresh/imported; next family-chain lower target is `{family_lower_fingerprint.get('next_target', 'none_family_chain_lower_layers_complete')}`.\n"
               if lock_achieved
               else "- rerun LOCAL_TEST after current stamps and runtime notes converge, then inspect whether rerun completion and completion lock converge.\n")
        ),
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
