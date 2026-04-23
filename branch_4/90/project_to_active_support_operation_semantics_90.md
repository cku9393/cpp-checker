# Project To Active Support Operation Semantics 90

## operation

`project_to_active_support`.

## semantics

Input is a normal support `>8` witness `W` with finite support set `S`, selected support measure, and active support `A = active_support(W)`. The operation applies when `A` is a strict subset of `S` and all obstruction payload, counterexample certificate, canonical motif dependency, and recognized family-chain source/lift fields are contained in `A`.

The output is:

`W_active = normalize(restrict(W, A))`.

If `A` is a strict subset, the support-size component of the selected lexicographic support measure decreases. If `A = S`, the operation is a no-op and the witness is routed to the next operation. If required fields are not contained in `A`, the active-support recognizer is ill-formed and the case is routed to a named projection blocker or higher-support escape.

Runtime table: `branch_4/90/runtime/project_to_active_support_operation_semantics_90.tsv`.
