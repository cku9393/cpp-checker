# Project To Active Support Smaller Witness 90

## construction

Given a normal support witness `W` with support set `S` and active support `A`, construct `W_active = normalize(restrict(W,A))` when `A` is a strict subset of `S`.

The support measure decreases because `support_size(W_active)=|A|<|S|=support_size(W)`. If `A=S`, the operation is a no-op and does not create a smaller witness. If projection loses payload/certificate/family-chain fields, the operation is not successful and the case is routed to a named projection blocker or higher-support escape after remaining operations fail.

Runtime table: `branch_4/90/runtime/project_to_active_support_smaller_witness_90.tsv`.
