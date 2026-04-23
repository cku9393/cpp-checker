# Family-Chain Absorption Reduction Operation Semantics 90

## operation

`family_chain_absorption_reduction`

## semantic contract

Input is a recognized normal support-growth witness containing a family-chain component that survives delete-redundant, project-to-active, coordinate-contraction, and canonical-motif-compression preconditions. The operation tries to construct a bounded family-chain target object and then splits into:

- `contradiction_by_family_chain_theorem`, where theorem applicability refutes the matched obstruction;
- `smaller_witness_by_absorption`, where the absorbable component is removed or replaced and a smaller witness candidate is produced;
- named escape or blocker if trigger, theorem applicability, payload/status transfer, or measure decrease is missing.

Runtime table: `branch_4/90/runtime/family_chain_absorption_reduction_operation_semantics_90.tsv`.
