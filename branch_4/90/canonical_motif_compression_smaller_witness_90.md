# Canonical Motif Compression Smaller Witness 90

## construction

Given a normal support witness `W` and an accepted compressible motif `M -> M_comp`, construct

`W_comp = normalize(rewrite(W, M, M_comp))`.

This is a smaller witness candidate when the selected lexicographic support measure decreases:

- support size decreases, or
- support size is unchanged and canonical motif rank decreases.

Counterexample-status preservation or reduction is not proved here.  It remains a named blocker.

Runtime table: `branch_4/90/runtime/canonical_motif_compression_smaller_witness_90.tsv`.
