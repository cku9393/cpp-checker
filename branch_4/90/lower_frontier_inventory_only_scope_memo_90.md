# Lower Frontier Inventory-Only Scope Memo 90

## scope

This memo covers exactly four lower-frontier first-class inventory rows:

- `antecedent_plus_eight_frontier`
- `support8_antecedent11_frontier`
- `antecedent_plus_nine_frontier`
- `support8_antecedent12_frontier`

These rows are not hidden. They remain visible in `runtime/lower_frontier_ladder_inventory_90.tsv`.

## current code status

Each row has:

- an authoritative accessor name
- a scan constructor name
- runtime candidate/cache paths
- validation status `current_verified`
- provenance status `mixed`

The important boundary is consumer relevance. These rows are not part of `lower_frontier_direct_dependency_item_keys_()` and have no direct top-level support8 theorem consumer in `lower_frontier_direct_consumers_()`.

## decision

All four rows are retained as `keep_inventory_only_nonblocking`.

This means:

- they are not deleted
- they are not hidden
- they are not relabeled as fresh without a targeted freshization round
- they are not treated as support8 lock blockers

## family-chain comparison

The family-chain lower imported layers have now been closed at total `7`, fresh `7`, imported `0`. The shell11/shell12 rows remain a broader lower-frontier catalog perimeter and are still not direct support8 lock blockers.
