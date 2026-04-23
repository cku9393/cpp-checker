# Support Reduction Selected Operation Skeleton 90

## operation lemma

`delete_redundant_support_coordinate_smaller_witness`.

## exact statement

If a normal support witness `W` has a support coordinate `c` that is redundant for the obstruction payload, counterexample certificate, and recognized family-chain lift payload when present, then restricting `W` to `S \\ {c}` and renormalizing gives a smaller witness under the selected lexicographic support measure. The operation either preserves counterexample status under the redundancy precondition or refutes support-minimality by constructing a smaller counterexample candidate. If the redundancy precondition fails, this operation does not apply and the witness is routed to the next operation or named escape.

## final status

`proof_ready_skeleton_selected_delete_redundant_coordinate`.

Measure decrease and conditional counterexample transfer are proved under the redundancy precondition. Normal-form preservation, payload-refinement transfer, canonical reindexing, and family-chain lift compatibility remain proof-sketch level for arbitrary broader witnesses.

Runtime skeleton: `branch_4/90/runtime/support_reduction_selected_operation_skeleton_90.tsv`.
