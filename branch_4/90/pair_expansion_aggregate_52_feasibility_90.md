# Pair Expansion Aggregate 52 Feasibility 90

## semantic contract

`pair_expansion_aggregate_52` is the aggregate over all single-family and pair-merged bounded out-of-pool schema expansion regions below the support-bounded schema-universe obstruction theorem.

The current runtime must satisfy:

- region count: `28`
- single-family regions: `7`
- pair-merged regions: `21`
- raw candidates: `501`
- canonical candidates: `501`
- deduplicated candidates: `182`
- local-exact survivors: `0`
- plus-one survivors: `0`
- theorem-preserving survivors: `0`

## constructor feasibility

The current code already had the required enumeration and validation primitives:

- `build_bounded_expansion_region_specs_`
- `enumerate_out_of_pool_schema_candidates_for_failure_family_`
- `enumerate_out_of_pool_merged_family_candidates_`
- `scan_out_of_pool_bounded_expansion_region_detailed_`
- `build_out_of_pool_candidate_aggregate_stats_`

The missing piece was an authoritative current constructor/cache path for the output `52` aggregate itself.

## implemented path

The implemented current path is:

1. pass1 builder: `build_current_pair_expansion_aggregate_52_from_bounded_region_scan_`
2. runtime payload: `pair_expansion_aggregate_52_payload_90.tsv`
3. pass2/pass3 cache loader: `load_current_pair_expansion_aggregate_52_runtime_artifact_`
4. runtime audits:
   - `pair_expansion_aggregate_52_generation_audit_90.tsv`
   - `pair_expansion_aggregate_52_fingerprint_90.tsv`
   - `pair_expansion_aggregate_52_rowset_equality_90.tsv`
   - `pair_expansion_aggregate_52_constructor_fingerprint_90.tsv`

## equality result

- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- fallback reachable: `1`
- fallback hit in captured pass2/pass3: `0`
- final provenance label: `fresh_current_runtime_generated`

## remaining caveat

This round freshized output `52`, not every upstream object below it. The current builder still uses the existing exact-basis payload and family summary/snapshot substrate as inputs.

The follow-up lower-layer target was:

- `triple_family_expansion_theorem_data_53`

That target has now been promoted in the later triple53 round, `quadruple_family_expansion_theorem_data_55` has been promoted in the quad55 round, and `quintuple_family_expansion_theorem_data_57`, `sextuple_family_expansion_theorem_data_57`, and `septuple_family_expansion_theorem_data_57` have been promoted in later 57-layer rounds. The current next target is `high_family_expansion_theorem_data_57`.
