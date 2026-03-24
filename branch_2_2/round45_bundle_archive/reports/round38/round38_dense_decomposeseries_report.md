# round38_dense_decomposeseries_report

결론은 HOLD다.

이번 round38 current pass에서는 round37 retained baseline에서 fresh workdir를 만들고, profiling context plumbing을 먼저 고치기 위해 전용 wrapper를 추가하고, decomposeSeriesFixed truth profiler를 다시 빌드해 truth row emission을 시도했다.

## 이번 current pass에서 실제로 확인된 것

1. dense 8192 tier에서 E_guard, dense_guard, Q_guard 중심 구조가 강하다는 retained 결론은 유지된다.
2. 현재까지의 retained evidence 기준으로 dominant raw-build hotspot은 spqr_raw_recursive_series_split_ms이고, 그 다음이 spqr_raw_choose_parallel_pair_ms다.
3. fresh current pass에서 env plumbing 자체는 일부 성립했다. emitted rows에는 nonempty case mode가 존재했고 census and rows 파일은 실제로 생성되었다.
4. 하지만 fresh emitted rows는 사실상 sanity-scale comb_dense n=256에서만 생성되었고, truth panel 8192 rows가 닫히지 않았다.
5. emitted rows의 mismatch reason은 debug_off로 고정되어 있었다. 즉 shadow attempted는 있었지만 shadow semantic equality를 실제 비교하는 build가 아니었다.
6. 따라서 truth gate는 통과하지 못했고, one-patch optimization 단계로 넘어가지 않았다.
7. micro panel은 fresh current pass로 다시 정리했다. comb_dense 4096, comb_rect_dense 4096, caterpillar_rect_dense 4096, comb_plus_unary 32768, comb_core 32768는 baseline-equivalent였다. dense 8192 계열은 계속 12초 timeout wall에 걸렸다.
8. reduced panel은 truth gate 실패 때문에 진행하지 않았다.
9. correctness current pass는 끝까지 닫히지 못했다. completed 92 cases 기준으로 observed mismatch가 1건 있었고, full close는 실패했다.

## 왜 HOLD인지

이번 라운드의 병목은 최적화 이전에 profiling truth였다. decomposeSeriesFixed 내부 subcolumn을 fresh truth panel에 대해 실제로 채우고, 그 위에서 dominant subphase 하나를 고르는 단계에 도달하지 못했다. 따라서 optimization patch를 넣는 것은 의미 보존상 부적절했고, release gate search도 의미 있는 coverage를 만들지 못했다.

최종 판단은 HOLD다. release gate는 없고, optimization patch는 적용하지 않았다.
