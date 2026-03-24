# Round45 Dense s-t Separator Source Alignment then Patch Report

## Conclusion
HOLD.

This round completed the source-alignment and smoke-gate phase and stopped there. The supplied baseline source and the artifact expectations are still structurally inconsistent, so fresh truth-panel separator-prefilter verification did not proceed.

## Fresh current-pass source alignment result
Fresh inspection of the baseline source produced:

- source_alignment_passed: False
- hook_same_side_probe_present: False
- hook_candidate_rows_present: False
- hook_prefilter_rows_present: False
- hook_shadowcheck_present: True
- header_sink_present: False

The baseline source still does not contain the profiler sinks and separator-prefilter hooks that the prior artifacts assume.

## Fresh smoke gate result
Two fresh smoke runs were executed.

- smoke comb_dense 256 census_rows: 0
- smoke comb_dense 256 candidate_rows: 0
- smoke comb_dense 256 prefilter_rows: 0
- smoke comb_dense 1024 census_rows: 0
- smoke comb_dense 1024 candidate_rows: 0
- smoke comb_dense 1024 prefilter_rows: 0

No fresh profiling rows were emitted. Therefore the smoke gate failed before truth-panel profiling.

## Maintained diagnosis from prior fresh artifacts
The latest maintained diagnosis from prior fresh artifacts remains:

- dense 8192 tier is still dominated by E_guard, dense_guard, Q_guard structure.
- accepted candidate rows were effectively zero.
- same-side reject candidates dominated candidate volume.
- the dominant raw-build hotspot remained bfs_neighbor_iter.

This diagnosis is maintained for planning, but it was not re-verified as a fresh round45 truth-panel run because the source-alignment smoke gate failed first.

## Separator prefilter status
Separator prefilter rows were not produced in fresh current pass.
Therefore st_separator_ratio was not freshly measured in round45.
No articulation prefilter patch was applied.
No release gate search was meaningfully executed.

## Correctness and performance status
Fresh correctness sweep was not run because the source-alignment smoke gate failed first.
Micro panel, reduced panel, full dense stage, hard_scaling full, and rebuttal gate were not executed in round45.

## Sparse monitors
comb_plus_unary 32768 and comb_core 32768 were not freshly re-run in round45 because execution stopped at the smoke gate. Their prior retained status is unchanged, but not freshly re-verified this round.

## Merge decision
HOLD.
The blocking issue is still experimental structure: source and artifact alignment did not hold, so fresh separator-prefilter truth measurement could not be claimed.
