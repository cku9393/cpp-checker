# Round37 dense decomposeSeries truth-then-patch report

## Final status
HOLD. This bundle is a fresh current-pass status hold, not a retained candidate.

## What was completed fresh

1. Fresh case lists were generated for dense stage, truth panel, micro panel, and reduced panel.
2. Fresh baseline probes were at least partially recollected for the micro and truth panels.
3. Fresh after-run micro panel probes were collected with the current solve binary.
4. Fresh decomposeSeries profiling code was inserted and built into the profiling binary.

## What blocked progress

The critical blocker was instrumentation truth. Fresh current-pass truth profiling did not emit matched decomposeSeries rows with populated subcolumns, so the instrumentation truth gate failed before any one-patch optimization and before gate re-search.

## Retained structure conclusion

The retained dense structure conclusion is still the same. Dense 8192 tier is centered on E_guard, dense_guard, and Q_guard patterns.

## Retained hotspot conclusion

The retained SPQR raw-build hotspot remains spqr_raw_recursive_series_split_ms, followed by spqr_raw_choose_parallel_pair_ms. This bundle does not claim that those were freshly re-validated end-to-end in round37.

## Micro panel after-run

Fresh after-run micro panel results that were actually collected:

- comb_dense 4096 seed1 OK around 5.38s
- comb_rect_dense 4096 seed1 OK around 5.87s
- caterpillar_rect_dense 4096 seed1 OK around 5.75s
- comb_dense 8192 seed1,2,3 TIMEOUT
- comb_rect_dense 8192 seed1,2,3 TIMEOUT
- caterpillar_rect_dense 8192 seed1,2,3 TIMEOUT
- comb_plus_unary 32768 seed1 OK around 4.95s
- comb_core 32768 seed1 OK around 8.98s

## Patch selection

No optimization patch was selected in this round because the instrumentation truth gate failed. Therefore there is no trustworthy fresh dominant decomposeSeries subcolumn winner to target.

## Gate search

Gate search was not meaningfully executed. The generated gate-search artifacts are explicit not_completed artifacts because the instrumentation truth gate failed first.

## Correctness

A fresh full correctness sweep was not completed in this current pass. The correctness artifacts in this bundle explicitly mark that incomplete state instead of pretending success.

## Merge decision

HOLD. No release gate was found, no optimization patch was validated, and no reduced-panel retained candidate was established.
