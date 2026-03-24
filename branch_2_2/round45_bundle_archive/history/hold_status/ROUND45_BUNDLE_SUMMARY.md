# Round 45 번들 요약

## 한 줄 결론
Round 45의 현재 상태는 **HOLD**다. 이번 번들은 fresh current-pass 최적화 성공 번들이 아니라, **source and artifact alignment 실패를 명시한 상태 번들**이다.

## 이번 라운드에서 실제로 확인된 것
Round 45 보고서와 validation 기준으로, baseline source와 아티팩트가 기대하는 profiler 구조가 일치하지 않았다.

핵심 지표는 다음과 같다.

- `source_alignment_passed = false`
- `hook_same_side_probe_present = false`
- `hook_candidate_rows_present = false`
- `hook_prefilter_rows_present = false`
- `hook_shadowcheck_present = true`
- `header_sink_present = false`

즉 shadowcheck 매크로 흔적은 있었지만, same-side probe, candidate row sink, prefilter row sink, sink header 관리가 source 기준으로는 성립하지 않았다.

## smoke gate 결과
fresh smoke run 두 개를 돌렸지만 profiler row가 실제로 남지 않았다.

- `smoke_comb_dense_256` 에서 census, candidate, prefilter row 모두 0
- `smoke_comb_dense_1024` 에서 census, candidate, prefilter row 모두 0

이 때문에 round45는 truth panel 8192 profiling 단계로 진입하지 못했다.

## 유지되는 진단
fresh round45 source alignment는 실패했지만, 직전 fresh artifact들로부터 유지되는 진단은 다음과 같다.

- dense 8192 tier는 여전히 `E_guard`, `dense_guard`, `Q_guard` 중심 구조가 강하다.
- dense 8192 candidate volume은 same-side reject가 사실상 대부분이다.
- accepted candidate는 사실상 0에 가깝다.
- 지배적 raw-build hotspot은 `bfs_neighbor_iter` 계열이다.
- 따라서 다음 최적화의 방향은 same-side probe 미세 최적화보다 **candidate universe size 감소** 쪽이다.

## 이번 라운드에서 못 한 것
다음 항목은 round45 fresh current-pass에서 성립하지 않았다.

- separator prefilter row의 fresh 생성
- `st_separator_ratio`의 fresh 측정
- articulation prefilter patch 적용
- meaningful gate search
- fresh correctness sweep
- micro panel 재실행
- reduced panel 진입

## 현재 직접 blocker
가장 직접적인 blocker는 **baseline source와 profiling artifact 기대치의 불일치**다. source 쪽에 필요한 hook와 sink가 없어서, 이후 실험이 모두 smoke gate에서 멈춘다.

## 이 번들에 포함된 것
이 zip은 다음 내용을 포함한다.

- `lca_tree_stress_v5/` : round45 hold zip 기준 source tree
- `round45_artifacts/` : round45 보고서, tsv, json, correctness, manifest, 참고 소스 파일
- `ROUND45_BUNDLE_SUMMARY.md` : 이 요약 파일

## 다음 라운드에서 바로 해야 할 일
가장 우선순위가 높은 다음 작업은 아래 순서다.

1. source 안에 same-side probe, candidate sink, prefilter sink, header sink를 실제로 복구한다.
2. smoke run에서 row append가 되는 것을 확인한다.
3. truth panel 8192에서 `prefilter_rows.tsv`를 실제로 남긴다.
4. `st_separator_ratio`가 충분히 작으면 articulation prefilter one-patch를 적용한다.
5. 그 다음에만 micro panel과 reduced panel을 다시 닫는다.
