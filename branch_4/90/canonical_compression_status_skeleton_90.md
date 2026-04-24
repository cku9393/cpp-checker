# Canonical Compression Status Skeleton 90

## lemma

`canonical_compression_status_preserved_reduced_or_escape`

## exact statement

For a normal support witness `W` with canonical motif `M` and accepted compressed motif `M_comp`, if `W_comp = normalize(rewrite(W,M,M_comp))` is defined and the accepted compression strictly decreases the selected support/canonical lexicographic measure, then `W_comp` either preserves counterexample status, reduces to a smaller counterexample witness using the strict measure decrease, or routes the status failure to a named operation blocker/deferred higher-support escape.

## proof outline

1. Use canonical motif notation to define `M`, `M_comp`, motif rank, and accepted compression.
2. Use compression semantics to construct `W_comp = normalize(rewrite(W,M,M_comp))`.
3. Use the already-proved lexicographic measure decrease: support size decreases, or support is fixed and canonical motif rank decreases.
4. Payload preservation reduces to motif payload-refinement congruence plus normalization.
5. Counterexample-status preservation reduces to canonical-motif status congruence plus normal-form preservation.
6. If compressed status changes but remains a valid counterexample or reduced obstruction, strict measure decrease supplies the smaller-witness branch.
7. If preservation/reduction cannot be established, route no accepted compression, no decrease, invalid rewrite, normal-form failure, payload failure, status failure, family-chain source-form failure, or later irreducible residual to a named blocker/deferred higher-support escape.

## relation to other operations

This is not coordinate contraction: compression rewrites a canonical motif and can decrease motif rank without quotienting equivalent support coordinates. It is not project-to-active support projection, because it does not remove inactive support. It is not family-chain absorption reduction, because any family-chain theorem/refutation use remains a separate operation.

## final status

`proof_ready_skeleton_canonical_compression_status_congruence_open`

The status proof is not completed. The remaining proof obligation is canonical-motif counterexample-status congruence plus normal-form, payload, and family-chain source-form transfer for arbitrary compressed witnesses.

Runtime skeleton: `branch_4/90/runtime/canonical_compression_status_skeleton_90.tsv`.
