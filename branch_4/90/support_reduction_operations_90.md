# Support Reduction Operations 90

## status after family-chain absorption status round

The support reduction attempt still does not prove an unrestricted support-reduction theorem. Current operation status:

- `delete_redundant_support_coordinate`: selected proof-ready/current-scope status case under redundancy precondition.
- `project_to_active_support`: measure decrease proved under strict active-subset; status branch is `proof_ready_skeleton_project_to_active_status_locality_open`.
- `contract_equivalent_support_coordinates`: measure decrease proved under nontrivial accepted equivalence; status branch is `proof_ready_skeleton_contract_equivalent_status_congruence_open`.
- `canonical_motif_compression`: lexicographic measure decrease proved under accepted compression; status branch is `proof_ready_skeleton_canonical_compression_status_congruence_open`.
- `family_chain_absorption_reduction`: status/refutation skeleton proof-ready, with source-target alignment and residual measure decrease open.
- frontier/tail capture: downstream after valid support `<=8` reduction.
- higher-support escape: deferred until operation proofs close.

Project-to-active locality, coordinate-contraction congruence, canonical-motif congruence, absorption source-target alignment, and residual absorption measure remain open. No support9+ scan was run.

Runtime table: `branch_4/90/runtime/support_reduction_operations_90.tsv`.
