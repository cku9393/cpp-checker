# Canonical Motif Compression Scope Memo 90

## selected target

This round selects `canonical_motif_compression` as the next support-reduction operation sublemma target.

The target is operation-specific only.  It does not prove the full support-reduction theorem, the full support-bound lemma, or the full general theorem.

## statement

If a support `>8` normal witness has an accepted compressible canonical motif, then replacing that motif by its compressed canonical representative and normalizing gives a smaller witness candidate under the selected lexicographic support measure.

The measure decrease is either:

- strict support-size decrease, if compression removes support atoms; or
- strict canonical motif rank decrease with support size fixed.

## boundaries

- delete-redundant, project-to-active, and coordinate-contraction routes remain previous operation routes.
- coordinate contraction merges equivalent coordinates; canonical motif compression rewrites a canonical motif to a lower-rank motif without requiring coordinate equivalence.
- family-chain absorption reduction remains a separate later operation.
- counterexample-status preservation is not promoted; it remains a named blocker unless the operation-specific status sublemma is proved.

Runtime inventory: `branch_4/90/runtime/canonical_motif_compression_scope_inventory_90.tsv`.
