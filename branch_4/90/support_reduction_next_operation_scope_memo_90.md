# Support Reduction Next Operation Scope Memo 90

## selected operation

`project_to_active_support`.

## scope

This round targets only the active-support projection operation. It does not replace the previous `delete_redundant_support_coordinate` proof-ready skeleton and does not prove all support-reduction operations.

`project_to_active_support` is selected because the runtime operation inventory already marks it as proof-sketch-ready and because it directly attacks the remaining operation-specific smaller-witness blocker: if a support `>8` normal witness contains coordinates outside the active payload/certificate/family-chain dependency set, projecting to active support gives a smaller support witness under the selected support measure.

The operation is distinct from:

- `delete_redundant_support_coordinate`, which deletes one presented redundant coordinate;
- `contract_equivalent_support_coordinates`, which quotients two equivalent coordinates;
- `canonical_motif_compression`, which lowers canonical rank without necessarily reducing support size.

Runtime inventory: `branch_4/90/runtime/support_reduction_next_operation_scope_inventory_90.tsv`.
