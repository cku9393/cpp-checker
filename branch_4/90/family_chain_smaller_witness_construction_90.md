# Family-Chain Smaller-Witness Construction 90

## purpose

This document defines the fallback needed when payload refinement does not directly preserve counterexample status.

## key distinction

Counterexample status preservation and smaller-witness construction are separate.

- Preservation: the lifted target object keeps the same counterexample meaning.
- Reduction: the failed-preservation case yields a new witness with a strictly smaller support measure.

## selected measure

The measure is the existing lexicographic support tuple from the support-bound and support-reduction skeletons:

`support_size, support_outside_8_distance, family_chain_depth, frontier_rank, shell_index, tail_length, canonical_witness_rank`.

## status

The construction is formalized, but not fully proved. Measure decrease is proof-ready if an operation-specific reduced witness is provided; the operation-specific construction remains blocked by support-reduction operation sublemmas.

Runtime table: `branch_4/90/runtime/family_chain_smaller_witness_construction_90.tsv`.
