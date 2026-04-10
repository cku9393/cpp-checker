# Proof System Contract Memo 90

## baseline

- code baseline: `branch_4/90/full_dynamic_top_tree_engine_90.cpp`
- bundle role: support8 / shell15 / tail / completion-lock proof-system engine
- explicit non-goal: BOJ complete solver

## current bundle metadata

| item | current code fact | assessment |
| --- | --- | --- |
| solver identity | header says `This is NOT the complete BOJ solver` | current truth |
| non-LOCAL_TEST main | `int main() { return 0; }` | current truth |
| current bundle version | `90` | current truth |
| current bundle role | `support8 / shell15 / tail / completion-lock proof-system bundle` | current truth |
| current bundle source basename | `full_dynamic_top_tree_engine_90.cpp` | current truth |
| current bundle summary basename | `project_status_summary_90.md` | current bundle reference |

## imported closed-output provenance

| item | source in code | assessment |
| --- | --- | --- |
| provenance catalog | `imported_closed_output_provenance_catalog_()` | current truth |
| family-chain lower data | `family_chain_output_57` | imported theorem-data output retained as provenance |
| lower frontier ladder catalog | `support_plus_one_frontier_output_67` through `support8_shell14_frontier_output_79` | retained as imported provenance catalog; direct shell15 dependency subset is now current-generated |
| archival shell15 source | `support8_shell15_frontier_output_84` | retained as compatibility fallback / equality oracle, no longer authoritative current path |
| explicit source path retained | `/mnt/data/full_dynamic_top_tree_engine_84.cpp` for the `84` frontier source | provenance retained, not current bundle identity |

## required lists

| contract item | source function | count |
| --- | --- | --- |
| required docs | `required_support8_tail_doc_paths_83_()` | `39` |
| required artifacts | `required_support8_tail_artifact_paths_83_()` | `8` |

## audit gates

| gate | source function | pass condition |
| --- | --- | --- |
| artifact audit | `validate_artifact_completion_audit_stats_()` | all required artifacts exist, are nonempty, shell15 artifact set complete, tail artifact set complete |
| document audit | `validate_document_completion_audit_stats_()` | all required docs are nonempty and core summary / shell theorem / tail pattern / artifact notes / bridge note are ready |
| rerun audit | `validate_rerun_completion_audit_stats_()` | local-test binary exists, current run or current stamp matches current binary, release binary exists and release stamp matches current binary |
| audit freshness | `validate_support8_audit_freshness_stats_()` | current artifact/doc/rerun audit fingerprints all match current filesystem / stamps |
| tail obstruction chain | `validate_support8_tail_obstruction_chain_theorem_data_()` | shell15 theorem + tail pattern + artifact audit + document audit + rerun audit + freshness all pass |
| completion lock | `validate_support8_authoritative_completion_lock_data_()` | shell15 frontier + shell15 scope + tail pattern + tail chain + artifact audit + document audit + rerun audit + freshness + stale audit eliminated + local test verified + release compile verified |

## provenance outputs

| output | path | role |
| --- | --- | --- |
| top-level inventory tsv | `branch_4/90/runtime/theorem_data_provenance_inventory_90.tsv` | machine-readable top-level theorem/audit validation split |
| lower-frontier inventory tsv | `branch_4/90/runtime/lower_frontier_ladder_inventory_90.tsv` | machine-readable first-class lower-frontier inventory |
| lower-frontier generation audit | `branch_4/90/runtime/lower_frontier_ladder_generation_audit_90.tsv` | direct shell-theorem dependency subset generation/cache audit |
| shell-theorem generation audit | `branch_4/90/runtime/support8_antecedent15_shell_theorem_generation_audit_90.tsv` | shell theorem freshization status over the lower ladder |
| provenance summary tsv | `branch_4/90/runtime/provenance_audit_fingerprint_90.tsv` | machine-readable counts and current bundle metadata |
| runtime audit report | `branch_4/90/runtime/provenance_audit_report_90.md` | short current-runtime provenance report |
| archival inventory note | `branch_4/90/theorem_data_provenance_inventory_90.md` | preserved markdown view of the current inventory |

## current verified recovery state

- release compile: verified
- LOCAL_TEST compile: verified
- pass1: `support8_authoritative_completion_locked`
- pass2: `support8_authoritative_completion_locked`
- pass3: `support8_authoritative_completion_locked`
- current runtime root: `branch_4/90/runtime`
- required docs: `39 / 39`
- required artifacts: `8 / 8`

## provenance snapshot

- top-level item count: `19`
- top-level fresh current runtime generated: `16`
- top-level current runtime validated imported data: `0`
- top-level mixed: `0`
- top-level archival only: `3`
- lower-frontier first-class inventory row count: `23`
- lower-frontier direct dependency subset count: `19`
- lower-frontier direct dependency freshized count: `19`
- lower-frontier inventory-only mixed rows: shell11/shell12 pair `4`

## immediate implication

- support8 lock recovery, lower-frontier direct dependency freshization, shell theorem freshization, tail-chain freshization, and completion-lock freshization are now current runtime facts for the support8 slice.
- the audit order and required counts were not weakened.
- the remaining limitation is no longer a top-level mixed theorem item; it is the broader perimeter choice of whether to freshize lower-frontier inventory-only shell11/shell12 rows and deeper family-chain imported layers.
