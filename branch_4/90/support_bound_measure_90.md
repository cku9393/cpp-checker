# Support Bound Measure 90

Selected measure: `lexicographic_support_tuple`.

Definition:

`(support_size, support_outside_8_distance, family_chain_depth, frontier_rank, shell_index, tail_distance, canonical_rank)`.

This tuple is well-founded for finite normal-form witnesses. It becomes a proof tool only if a support-reduction or family-chain-lift step is proved to decrease it. Without that decrease proof, a support `>8` witness is routed to `higher_support_escape`.

Runtime table: `branch_4/90/runtime/support_bound_measure_90.tsv`.
