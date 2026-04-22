# Lower Frontier Inventory-Only Freshization Report 90

## decision

No inventory-only shell11/shell12 row was freshized in this round.

All four rows were classified as `keep_inventory_only_nonblocking`:

- `antecedent_plus_eight_frontier`
- `support8_antecedent11_frontier`
- `antecedent_plus_nine_frontier`
- `support8_antecedent12_frontier`

## reason

The four rows have accessors, scan constructors, and cache paths, but they are not direct dependencies of the current support8 top-level proof slice.

The current top-level support8 state remains:

- fresh current runtime generated: `16`
- current runtime validated imported data: `0`
- mixed: `0`
- archival only: `3`

## result

- freshized inventory-only rows: `0`
- keep inventory-only nonblocking rows: `4`
- defer after family-chain rows: `0`

The family-chain lower imported layers are now closed at total `7`, fresh `7`, imported `0`. The shell11/shell12 inventory-only rows remain visible nonblocking perimeter rows and are not the selected next-scope blocker.
