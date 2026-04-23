# Support Reduction Selected Operation Semantics 90

## operation

`delete_redundant_support_coordinate`.

## semantics

The input is a normal support witness `W` with support set `S` and a coordinate `c in S`. The redundancy precondition says that `c` is absent from the normalized obstruction payload, absent from the counterexample certificate, and absent from the family-chain lifted payload if the witness is recognized family-chain source form. The output witness is the restriction of `W` to `S \\ {c}`, followed by the already-declared normal-form projection.

This semantics proves the operation contract only under the redundancy precondition. It does not prove that arbitrary support `>8` witnesses contain a redundant coordinate.

Runtime table: `branch_4/90/runtime/support_reduction_selected_operation_semantics_90.tsv`.
