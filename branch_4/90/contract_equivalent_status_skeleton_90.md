# Contract Equivalent Status Skeleton 90

## lemma

`contract_equivalent_status_preserved_reduced_or_escape`

## exact statement

For a normal support witness `W` with finite support `S`, accepted coordinate-equivalence relation `~`, and quotient map `q:S->S/~`, if some equivalence class has size greater than one and `W_contract = normalize(pushforward(W,q))` is defined, then `W_contract` either preserves counterexample status, reduces to a smaller counterexample witness using the strict support-measure decrease, or routes the status failure to a named operation blocker/deferred higher-support escape.

## proof outline

1. Use equivalent support coordinate notation to define accepted classes and quotient map `q`.
2. Use contraction semantics to construct `W_contract = normalize(pushforward(W,q))`.
3. Use the already-proved finite partition arithmetic: a nontrivial class gives `|S/~| < |S|`.
4. Payload preservation reduces to payload-role congruence under accepted equivalence plus quotient normalization.
5. Counterexample-status preservation reduces to equivalent-coordinate status congruence plus normal-form preservation.
6. If contracted status changes but remains a valid counterexample or reduced obstruction, strict measure decrease supplies the smaller-witness branch.
7. If preservation/reduction cannot be established, route invalid equivalence, failed congruence, singleton/no-op, normal-form failure, canonicalization failure, family-chain source-form failure, or later irreducible residual to a named blocker/deferred higher-support escape.

## relation to other operations

This is not project-to-active support projection: contraction merges active equivalent coordinates rather than removing inactive coordinates. It is not delete-redundant coordinate deletion, because equivalent coordinates can both be active. It is not canonical motif compression, because the quotient operation acts on support coordinates and only uses canonicalization as a compatibility obligation.

## final status

`proof_ready_skeleton_contract_equivalent_status_congruence_open`

The status proof is not completed. The remaining proof obligation is equivalent-coordinate counterexample-status congruence plus normal-form, canonicalization, payload, and family-chain source-form transfer for arbitrary quotient witnesses.

Runtime skeleton: `branch_4/90/runtime/contract_equivalent_status_skeleton_90.tsv`.
