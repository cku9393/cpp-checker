# round30_dense_spqr_seriessplit_report

Status: HOLD

This bundle closes the reduced dense-core panel and correctness guard fresh in the current pass, but it does **not** establish a new retained candidate.

## What was freshly re-run in this pass

The following were re-executed fresh from the retained round29 baseline:

1. Reduced dense-core panel before and after on identical generated inputs.
2. Fresh correctness sweep with the retained baseline used as both reference and candidate, confirming no semantic drift in the hold bundle.

Fresh correctness result:

- `total_cases = 151`
- `mismatch = 0`
- `all_same = true`

Fresh reduced dense-core observations:

- `comb_dense 4096` after: `5.47s`, `5.47s`, `5.42s`
- `comb_dense 8192` after: all three seeds timed out at the 12s wall
- `comb_rect_dense 4096` after: `6.90s`, `7.61s`, `6.64s`
- `comb_rect_dense 8192` after: all three seeds timed out at the 12s wall
- `caterpillar_rect_dense 4096` after: `7.77s`, `6.55s`, `6.64s`
- `caterpillar_rect_dense 8192` after: all three seeds timed out at the 12s wall

Sparse monitors remained healthy in the fresh reduced panel:

- `comb_plus_unary 32768`: `6.17s`, `5.90s`, `6.50s`
- `comb_core 32768`: `8.71s`, `10.22s`, `9.22s`

## What remained carry-forward reference material

The series-split profiler, shadow census, hotspot table, and gate-search artifacts in this bundle were carried forward from the retained round29 line.

Those reference artifacts still support the same working hypothesis:

1. Dense 8192-tier rows are concentrated in the `E_guard = 1`, `dense_guard = 1`, `Q_guard = 1` region.
2. The dominant raw-build hotspot remains inside `spqr_raw_recursive_total_ms`.
3. The largest subphase is still `spqr_raw_recursive_series_split_ms`, with `spqr_raw_choose_parallel_pair_ms` next.
4. Sampled shadow rows remained zero-mismatch in the retained reference line, but no zero-mismatch and positive-speedup release gate was established.

## Why this is HOLD

No fresh one-patch retained win was demonstrated in the current pass.

In particular:

- `comb_dense 8192` did not move below the 12s wall.
- `comb_rect_dense 8192` did not move below the 12s wall.
- `caterpillar_rect_dense 8192` did not move below the 12s wall.
- Because the reduced dense-core promotion gate was not satisfied, the pass did not expand to full dense stage, hard_scaling full, or full rebuttal gate.

## Final judgment

This round30 bundle is a reduced-panel fresh close plus a retained-reference profiler package. It is useful for bookkeeping and for preserving the current hypothesis around `spqr_raw_recursive_series_split_ms`, but it does **not** establish a new retained merge candidate.
