# Contract Equivalent Support Coordinates Smaller Witness 90

## construction

Given a normal support witness `W` with support set `S`, accepted equivalence relation `~`, and at least one nontrivial equivalence class, define `q:S -> S/~` and set `W_contract = normalize(pushforward(W,q))`.

The arithmetic smaller-witness part is current-scope proved under the nontrivial-class precondition: `|S/~| < |S|`. Semantic preservation remains conditional on normal-form, payload, canonical, and counterexample-status congruence.

## final status

`coordinate_contraction_smaller_witness_measure_proved_congruence_open`.

Runtime table: `branch_4/90/runtime/contract_equivalent_support_coordinates_smaller_witness_90.tsv`.
