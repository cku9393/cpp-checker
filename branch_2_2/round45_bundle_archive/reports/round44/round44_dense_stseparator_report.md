# Round44 Dense s-t Separator Prefilter Report

## Conclusion
HOLD.

This round could not be executed as a fresh current-pass retained attempt because the supplied baseline zip source does not contain the same-side probe and separator-prefilter instrumentation implied by the round43 artifacts. The round43 artifact conclusions remain useful for diagnosis, but they are not enough to claim a fresh round44 close.

## Baseline source mismatch
Source inspection of `solve.cpp` from the supplied baseline zip found:

- `same_side_probe_present`: `False`
- `candidate_rows_sink_present`: `False`
- `prefilter_rows_sink_present`: `False`
- `round43_macro_present`: `False`
- `round44_macro_present`: `False`
- `dense_shadow_case_mode_env_present`: `True`

This means the supplied source cannot reproduce the round43 candidate-event artifacts without reconstructing missing instrumentation first.

## Maintained diagnosis from round43 artifacts
The maintained conclusions from the supplied round43 artifact set are:

- Dense 8192 remains dominated by the `E_guard`, `dense_guard`, `Q_guard` structure.
- Candidate rows existed at 8192 truth scale.
- Accepted candidate rows were zero.
- Same-side reject rows dominated candidate volume.
- The dominant hotspot was `bfs_neighbor_iter`.

Key rows from the round43 hotspot artifact:
- comb_dense 8192: candidate_rows=13210, same_side_rows=13201, accepted_rows=0, dominant_hotspot=bfs_neighbor_iter, mean_full_bfs_neighbor_iter_ms=7.272
- comb_rect_dense 8192: candidate_rows=7646, same_side_rows=7642, accepted_rows=0, dominant_hotspot=bfs_neighbor_iter, mean_full_bfs_neighbor_iter_ms=7.618
- caterpillar_rect_dense 8192: candidate_rows=7455, same_side_rows=7452, accepted_rows=0, dominant_hotspot=bfs_neighbor_iter, mean_full_bfs_neighbor_iter_ms=7.694

## Why round44 is HOLD
Round44 required a fresh current-pass separator-prefilter truth run. That could not be claimed because the supplied baseline source and the supplied round43 artifacts are structurally inconsistent. Running the supplied source as-is would not reproduce the round43 candidate instrumentation, so any fresh round44 numbers would mix source reconstruction work with the separator-prefilter hypothesis and would not satisfy the requested baseline discipline.

## Merge decision
HOLD. No fresh zero-mismatch and positive-speedup release gate was established.