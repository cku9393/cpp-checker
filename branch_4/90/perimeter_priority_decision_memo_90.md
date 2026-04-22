# Perimeter Priority Decision Memo 90

## question

This memo decides how to treat the four lower-frontier inventory-only rows that remain outside the direct support8 antecedent15 shell-theorem dependency subset.

The current top-level support8 slice is already locked and freshized:

- support8 classification: `support8_authoritative_completion_locked`
- top-level fresh current runtime generated: `16`
- top-level current runtime validated imported data: `0`
- top-level mixed: `0`
- top-level archival only: `3`

## options

### A. Freshize the four inventory-only shell11/shell12 rows now

This is technically possible in principle because current accessor names, scan constructors, and runtime cache paths exist for all four rows.

However, the code does not list these rows in `lower_frontier_direct_dependency_item_keys_()`, and `lower_frontier_direct_consumers_()` returns no direct support8 theorem consumer for them. Freshizing them now would not improve the current top-level support8 lock.

### B. Keep them as non-blocking inventory-only perimeter rows

This is the current truthful interpretation. The rows are exposed so the broader imported lower-frontier catalog is visible, but they are not a direct blocker for:

- `support8 antecedent15 shell theorem`
- `support8 tail obstruction chain theorem`
- `support8 authoritative completion lock`

### C. Prioritize family-chain lower imported layers

The family-chain top theorem objects are fresh current-runtime constructors. This caveat has now been closed for all seven lower-layer rows:

- pair-expansion aggregate from output `52`
- triple-family theorem data from output `53`
- quadruple-family theorem data from output `55`
- quintuple/sextuple/septuple/high-family theorem data from output `57`

These are more directly relevant to reducing the remaining broader theorem-data caveat than the shell11/shell12 inventory-only rows.

## decision

The four shell11/shell12 rows remain `keep_inventory_only_nonblocking`.

No shell11/shell12 inventory-only row is promoted in this round. Family-chain lower-layer freshization is complete at total `7`, fresh `7`, imported `0`; the next selected scope is `general_gap_bridge_formalization`.

## evidence

- `lower_frontier_direct_dependency_item_keys_()` contains 19 rows and excludes the shell11/shell12 pair.
- `lower_frontier_inventory_item_keys_()` adds the shell11/shell12 pair only after the 19 direct dependencies.
- `lower_frontier_direct_consumers_()` returns an empty consumer list for the four shell11/shell12 rows.
- The top-level provenance audit remains fresh `16`, validated-imported `0`, mixed `0`, archival-only `3`.
