# Pair Expansion Aggregate 52 Scope Memo 90

## target

- item key: `pair_expansion_aggregate_52`
- current status after this round: `current_verified`
- current provenance after this round: `fresh_current_runtime_generated`
- imported source tag/version: `pair_expansion_aggregate_52` / output `52`

## code-level object

`pair_expansion_aggregate_52` is the bounded single/pair expansion layer under the support-bounded schema-universe obstruction theorem.

It covers:

- `7` single-family one-step regions
- `21` pair-merged regions
- `28` total bounded expansion regions
- aggregate candidates `raw=501`, `canonical=501`, `deduplicated=182`
- survivor counts all `0`

## accessors and constructors

- previous imported source: hardcoded aggregate inside `ensure_candidate_pool_completeness_and_expansion_ready_()`
- current authoritative constructor: `build_current_pair_expansion_aggregate_52_from_bounded_region_scan_`
- current cache loader: `load_current_pair_expansion_aggregate_52_runtime_artifact_`
- current consumer-facing accessor: `build_current_support_bounded_schema_universe_obstruction_theorem_`

## consumers

Direct consumers:

- `bounded family-chain theorem`
- `family-chain self verification`

Indirect consumers:

- `build_current_family_chain_output_57_theorem_objects_`
- `build_current_unified_bounded_schema_universe_obstruction_theorem_`

The layer is not part of the lower-frontier shell11/shell12 inventory-only perimeter decision.

## data shape

The runtime payload is materialized at:

- `branch_4/90/runtime/pair_expansion_aggregate_52_payload_90.tsv`

Each row represents one bounded expansion region and stores:

- region kind
- region fingerprint
- source family fingerprints
- symbol / antecedent bounds
- raw / canonical / out-of-pool candidate counts
- local-exact / plus-one / theorem-preserving survivor counts

## equality rule

Legacy output `52` does not carry a separate preserved candidate row-set artifact in this bundle.

Therefore equality against imported output `52` is:

- region-count equality: `28 = 7 + 21`
- aggregate-count equality: `501 / 501 / 182 / 0 / 0 / 0`
- consumer-visible support theorem equality
- runtime payload fingerprint stability across pass2/pass3

The imported comparison fingerprint is:

- `52|regions=28|single=7|pair=21|raw=501|canonical=501|deduplicated=182|local=0|plus=0|theorem=0`

## feasibility summary

Feasible and completed.

Pass1 rebuilt the layer from current bounded region enumeration / validation. Pass2 and pass3 loaded the validated runtime artifact. Fallback remains reachable, but the captured success run has `fallback_hit=0`.
