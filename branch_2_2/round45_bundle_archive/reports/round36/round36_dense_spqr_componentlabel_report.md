# round36_dense_spqr_componentlabel_report

결론

이번 round36은 HOLD다. fresh current-pass로 micro panel과 correctness는 다시 실행했지만, source에 round36 component-label truth profiler hook 자체가 없어서 instrumentation truth gate를 통과하지 못했다. 따라서 patch 단계와 release gate search는 실제 실행되지 않았다.

핵심 관찰

- dense 8192 tier의 E_guard, dense_guard, Q_guard 중심 구조는 prior retained evidence를 유지한다.
- 지배적 raw-build hotspot은 prior retained evidence 기준 spqr_raw_recursive_series_split_ms, 그 다음은 spqr_raw_choose_parallel_pair_ms다.
- 이번 fresh current-pass에서는 component label subcolumn을 채우는 profiler hook가 solve.cpp에 없어서 instrumentation validation이 실패했다.
- micro panel은 fresh 재실행했고 candidate는 baseline-equivalent였다.
- correctness_round36_summary.json은 mismatch 0, all_same true, all_validator_pass true다.

instrumentation validation

- macro_present: False
- namespace_present: False
- field_seed_scan_present: False
- field_queue_reset_present: False
- field_queue_pushpop_present: False
- field_neighbor_iter_present: False
- field_label_write_present: False
- field_touched_reset_present: False
- field_other_present: False
- instrumentation_supported: False
- status: instrumentation_failure
- reason: source_does_not_contain_round36_componentlabel_truth_hooks

micro panel 상태

- comb_dense 4096 seed 1: rc=0 time=5.48s validator_ok=1
- comb_dense 8192 seed 1: rc=124 time=12.01s validator_ok=0
- comb_dense 8192 seed 2: rc=124 time=12.01s validator_ok=0
- comb_dense 8192 seed 3: rc=124 time=12.01s validator_ok=0
- comb_rect_dense 4096 seed 1: rc=0 time=5.85s validator_ok=1
- comb_rect_dense 8192 seed 1: rc=124 time=12.01s validator_ok=0
- comb_rect_dense 8192 seed 2: rc=124 time=12.01s validator_ok=0
- comb_rect_dense 8192 seed 3: rc=124 time=12.01s validator_ok=0
- caterpillar_rect_dense 4096 seed 1: rc=0 time=5.98s validator_ok=1
- caterpillar_rect_dense 8192 seed 1: rc=124 time=12.01s validator_ok=0
- caterpillar_rect_dense 8192 seed 2: rc=124 time=12.01s validator_ok=0
- caterpillar_rect_dense 8192 seed 3: rc=124 time=12.01s validator_ok=0
- comb_plus_unary 32768 seed 1: rc=0 time=4.91s validator_ok=1
- comb_core 32768 seed 1: rc=0 time=8.91s validator_ok=1

reduced panel

- instrumentation truth gate 실패로 진입하지 않았다.

merge or hold

- HOLD
- 이유: component-label profiler hook 부재로 fresh truth profiling 자체가 성립하지 않았고, zero-mismatch and positive-speedup release gate도 찾지 못했다.