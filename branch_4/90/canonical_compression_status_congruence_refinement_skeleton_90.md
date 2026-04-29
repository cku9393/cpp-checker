# Canonical Compression Status Congruence Refinement Skeleton 90

## lemma

`canonical_motif_status_preserved_under_refined_congruence_or_reduced_or_escape`

## statement

Let `W` be a normal support `>8` witness with canonical motif `M`, accepted
lower-rank compressed motif `M_comp`, and compressed witness
`W_comp = normalize(rewrite(W,M,M_comp))`.

Canonical motif compression either preserves counterexample status under
payload congruence, status-domain transfer, normal-form transfer, and status
predicate congruence; gives a smaller witness when changed compressed status is
valid; or routes failure to a named canonical-compression blocker or deferred
higher-support escape.

## status

`proof_ready_skeleton_canonical_compression_congruence_domain_normal_form_open`

This is not a completed proof of `canonical_motif_compression`, not a full
support reduction proof, not support8 sufficiency, and not a full general
theorem.

Runtime skeleton:
`branch_4/90/runtime/canonical_compression_status_congruence_refinement_skeleton_90.tsv`.
