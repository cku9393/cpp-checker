# Canonical Motif Compression Skeleton 90

## lemma

`canonical_motif_compression_smaller_witness`

## exact statement

If a normal support witness has an accepted compressible canonical motif, then replacing the motif by its compressed canonical representative and normalizing gives a smaller witness candidate under the selected lexicographic support measure.  The status-preservation branch remains open unless the counterexample-status preservation or reduction sublemma is supplied.

## status after canonical-compression status round

`partial_canonical_compression_status_proof_ready_congruence_open`

The status branch is now first-class as `canonical_compression_status_or_escape`.
The proof-ready skeleton isolates canonical-motif counterexample-status
congruence, normal-form transfer, payload transfer, and family-chain source-form
transfer as the remaining operation-local obligations. The lexicographic
measure decrease from the previous round is unchanged and is not being used as
an unstated status-preservation proof.

Next exact target: `family_chain_absorption_status_preservation`.

This is not a full support-reduction theorem and not a full general theorem.

Runtime table: `branch_4/90/runtime/canonical_motif_compression_skeleton_90.tsv`.
