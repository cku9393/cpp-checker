# Canonical Compression Status Preservation Scope Memo 90

## selected target

`canonical_compression_status_or_escape`

## decision

This round attacks only the status branch of `canonical_motif_compression`. It does not prove the full compression operation, the full support-reduction theorem, support8 sufficiency, or the full general theorem.

The selected statement is:

If a normal support witness has an accepted compressible canonical motif and the compressed witness construction is defined, then the compressed witness either preserves counterexample status, reduces to a smaller counterexample witness using the already-proved lexicographic measure decrease, or routes the status failure to a named operation blocker/deferred higher-support escape.

The proof attempt reduces the blocker from an undifferentiated `blocked_by_status_preservation` label to a first-class canonical-motif status congruence obligation. Payload congruence is proof-sketch-ready under the accepted motif refinement contract. Counterexample-status congruence remains open and is not promoted to proved.

Runtime inventory: `branch_4/90/runtime/canonical_compression_status_preservation_scope_inventory_90.tsv`.
