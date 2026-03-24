# round27_dense_spqr_rawbuild_report

## Conclusion
HOLD. Fresh current-pass dense eq-guard shadow rows were collected and remained semantic-match in sampled checks, but no zero-mismatch plus positive-speedup release gate was found. The attempted raw-build cost reduction candidate was reverted, and the retained solver remains baseline-equivalent.

## Baseline dense blocker reproduction
comb_dense 8192: s1=TIMEOUT->TIMEOUT, s2=TIMEOUT->TIMEOUT, s3=TIMEOUT->TIMEOUT
comb_rect_dense 8192: s1=TIMEOUT->TIMEOUT, s2=TIMEOUT->TIMEOUT, s3=TIMEOUT->TIMEOUT
caterpillar_rect_dense 8192: s1=TIMEOUT->TIMEOUT, s2=TIMEOUT->TIMEOUT, s3=TIMEOUT->TIMEOUT
comb_dense 4096: s1=6.09->6.11, s2=5.95->6.21, s3=6.05->7.01
comb_rect_dense 4096: s1=6.58->6.9, s2=7.15->7.61, s3=6.53->6.64

## Dense eq-guard shadow structure
comb_dense 8192: shadow_attempted_ratio=0.036337209302325583, shadow_match_ratio=0.036337209302325583, dense_guard_ratio=0.501453488372093, E_guard_ratio=0.5, Q_guard_ratio=0.5, mean_spqr_total_ms=5907.02082, mean_fallback_total_ms=3.7820828488372094, dominant_rawbuild_hotspot=spqr_raw_recursive_total_ms (4717.65052 ms)
comb_rect_dense 8192: shadow_attempted_ratio=0.25, shadow_match_ratio=0.25, dense_guard_ratio=0.5625, E_guard_ratio=0.5, Q_guard_ratio=0.5, mean_spqr_total_ms=6689.014375, mean_fallback_total_ms=4.0868125, dominant_rawbuild_hotspot=spqr_raw_recursive_total_ms (5412.638375 ms)
caterpillar_rect_dense 8192: shadow_attempted_ratio=0.27586206896551724, shadow_match_ratio=0.27586206896551724, dense_guard_ratio=0.5172413793103449, E_guard_ratio=0.5172413793103449, Q_guard_ratio=0.5172413793103449, mean_spqr_total_ms=6640.57225, mean_fallback_total_ms=4.389655172413793, dominant_rawbuild_hotspot=spqr_raw_recursive_total_ms (5337.586 ms)

## Selected prototype
Chosen candidate: rawspqr_recursive_branch_partition_reuse_v2

Reason: fresh raw-build rows showed the dominant internal hotspot in matched shadow rows was spqr_raw_recursive_total_ms, with spqr_raw_recursive_series_split_ms and spqr_raw_choose_parallel_pair_ms dominating the builder interior. This pointed to recursive split and branch-partition scratch churn rather than rebuildBlockSpqrFull materialization or append-stitch remap as the first target.

Prototype action: attempted thread_local scratch reuse in decomposeParallelFixed and decomposeSeriesFixed queue and component buffers, then smoke-tested before broader rollout.

Observed smoke result on comb_dense 4096 s1 during candidate check: no meaningful win, and no evidence that the 8192 wall would move. The retained solver was therefore reverted to the baseline-equivalent path.

## Gate search result
{
  "gate_found": false,
  "reason": "no zero-mismatch positive-speedup gate under searched family",
  "searched_rows": 77
}

Interpretation: gate search found no release candidate satisfying both zero mismatch and positive speedup. selected_release_gate_hit_ratio therefore remained 0 in the retained bundle.

## Correctness
{
  "total_cases": 151,
  "mismatch": 0,
  "all_same": true
}

## Sparse and scale-check monitors
comb_plus_unary 32768: s1=6.88->6.17, s2=6.33->5.9, s3=5.97->6.5 | validators: s1:OK, s2:OK, s3:OK
comb_core 32768: s1=8.21->8.71, s2=8.32->10.22, s3=8.97->9.22 | validators: s1:OK, s2:OK, s3:OK
multi_comb_core 16384: s1=7.65->7.33, s2=7.67->7.13, s3=7.39->7.69 | validators: s1:OK, s2:OK, s3:OK
multi_comb_core 32768: s1=TIMEOUT->TIMEOUT, s2=TIMEOUT->TIMEOUT, s3=TIMEOUT->TIMEOUT | validators: s1:TIMEOUT, s2:TIMEOUT, s3:TIMEOUT
multi_comb_rect 2048: s1=1.37->1.44 | validators: s1:OK
multi_comb_cap 2048: s1=0.77->0.86 | validators: s1:OK
chain_unary 2048: s1=0.73->0.86 | validators: s1:OK
balanced_dense 512: s1=0.02->0.04 | validators: s1:OK
random_recursive_mixed 512: s1=0.02->0.03 | validators: s1:OK

## Dense relative preservation
comb_dense 4096: s1=6.09->6.11, s2=5.95->6.21, s3=6.05->7.01
comb_rect_dense 4096: s1=6.58->6.9, s2=7.15->7.61, s3=6.53->6.64
caterpillar_rect_dense 4096: s1=6.44->7.77, s2=6.67->6.55, s3=6.57->6.64

## Hard scaling and rebuttal gate
Not rerun in this retained hold bundle. Because no zero-mismatch positive-speedup release gate was found, the workflow stopped before hard_scaling full and full rebuttal_gate.

## Merge decision
HOLD. The fresh current pass confirmed dense eq-guard shadow match rows and identified the builder hotspot, but the attempted prototype did not establish a retained performance win, and no release gate with positive speedup was discovered.