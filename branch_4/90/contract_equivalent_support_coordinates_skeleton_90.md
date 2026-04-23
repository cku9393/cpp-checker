# Contract Equivalent Support Coordinates Skeleton 90

## operation lemma

`contract_equivalent_support_coordinates_smaller_witness`.

## exact statement

Let `W` be a normal support `>8` witness with finite support set `S`. Let `~` be the accepted coordinate-equivalence relation, and suppose some equivalence class has size greater than one. If quotienting by `~` is congruent for payload fields, canonical motif fields, and counterexample-status fields, then `W_contract = normalize(pushforward(W,q))` is a smaller support witness candidate. The support measure strictly decreases because `|S/~| < |S|`. If status congruence is missing, contraction is only a named blocker or possible smaller-witness route, not a completed proof. The status follow-up now isolates this as `proof_ready_skeleton_contract_equivalent_status_congruence_open`.

## final status

`partial_contract_equivalent_status_proof_ready_congruence_open`.

The finite quotient, output well-definedness under preconditions, strict measure decrease, singleton no-op classification, and failure naming are proved under current scope. Normal form, payload refinement, canonicalization compatibility, and family-chain source-form preservation are proof-sketch. Counterexample-status preservation is not proved; it is now a first-class equivalent-coordinate congruence obligation.

Runtime skeleton: `branch_4/90/runtime/contract_equivalent_support_coordinates_skeleton_90.tsv`.
