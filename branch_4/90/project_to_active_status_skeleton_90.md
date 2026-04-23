# Project To Active Status Skeleton 90

## lemma

`project_to_active_status_preserved_reduced_or_escape`

## exact statement

For a normal support witness `W` with support `S` and active support `A`, if `A` is a strict subset of `S` and contains the payload, certificate/status, canonical, frontier/tail, and family-chain fields required by the selected contract, then `W_active = normalize(restrict(W,A))` either preserves counterexample status, reduces to a smaller counterexample witness using the strict support-measure decrease, or routes the status failure to a named operation blocker/deferred higher-support escape.

## proof outline

1. Use active support notation to define `A` and inactive support `S \ A`.
2. Use project-to-active semantics to construct `W_active`.
3. Use the already-proved strict active-subset measure decrease.
4. Payload-side inactive irrelevance follows from active support containing payload dependency coordinates.
5. Counterexample-status preservation reduces to inactive-support status locality plus normal-form preservation.
6. If projected status changes but remains a valid counterexample/reduced obstruction, measure decrease supplies the smaller-witness branch.
7. If preservation/reduction cannot be established, route the case to a named blocker or deferred higher-support escape.

## relation to other operations

This is not delete-redundant coordinate deletion: project-to-active removes all inactive coordinates at once and depends on active dependency extraction. It is also separate from coordinate contraction, canonical motif compression, and family-chain absorption.

## final status

`proof_ready_skeleton_project_to_active_status_locality_open`

The status proof is not completed. The remaining proof obligation is inactive-support counterexample-status locality plus normal-form/source-form transfer for arbitrary projected witnesses.

Runtime skeleton: `branch_4/90/runtime/project_to_active_status_skeleton_90.tsv`.
