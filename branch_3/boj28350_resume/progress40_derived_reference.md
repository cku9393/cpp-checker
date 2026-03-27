# Progress40-Derived Reference

이 문서는 branch-local solver 작업 전에 bundled `progress40` 패키지에서 바로 재사용해야 할 사실만 추려 놓은 참조 메모다. 목적은 세 가지다.

1. progress40이 실제로 추가한 최적화 축을 다시 고정한다.
2. hard family에서 이미 기각된 접근을 다시 반복하지 않도록 막는다.
3. `lca_tree_stress_v5` 기준으로 현재 package가 어디까지 authoritative한지 빠르게 확인하게 한다.

## 읽은 bundled source set

- `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`
- `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`
- `boj28350_bundle_archive/boj28350_progress40_results_merged.json`
- `boj28350_bundle_archive/boj28350_current_state_summary.md`
- `boj28350_bundle_archive/boj28350_next_session_briefing.md`

## 재사용해야 할 progress40 기법

1. progress40은 새 알고리즘 family가 아니라 `progress39` 위에 `layout signature reuse gate`를 얹은 라운드다.
   `boj28350_progress40_results_merged.json`도 `base_source`를 `boj28350_literature_progress39_same_layout_zero_span_elision.cpp`로 기록한다.
2. 다음 solver-side 최적화는 아래 세부 subaxis를 유지한 채 residual을 줄이는 방향이어야 한다.
   `signature source load`
   `signature materialize`
   `signature compare core`
   `same-layout gate`
   `zero-span eligibility gate`
   `fastpath commit core`
   `connector hotpath reuse`
3. source상 feature flag 계층은 `ENABLE_PACK_ENCODE_NORMALIZE_OPT`, `ENABLE_PACK_ENCODE_NORMALIZE_CORE_OPT`, `ENABLE_CANONICAL_NORMALIZE_OPT`, `ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT`, `ENABLE_LAYOUT_SIGNATURE_GATE_OPT` 순으로 이어진다.
   즉 progress40은 pack/normalize -> same-layout reuse -> zero-span elision -> fastpath commit의 누적 최적화 라인을 전제로 한다.
4. route-aware attribution은 계속 보존해야 한다.
   progress40 source는 `baseline`, `delta_preserved_then_skeleton`, `connector_skeleton`, `general_delta` route별 `lgate_*` 시간을 따로 집계한다.
5. reproducibility support artifact도 progress40 구성 일부다.
   `run_progress40_case_supervised.py`, `progress40_finalize_case.py`, `reconcile_progress40_results.py`, `merge_progress40_results.py`, `progress40_case_journal.jsonl`, `progress40_resume_remaining.sh`

## 다시 들고 오면 안 되는 접근

1. `shared backbone`
   unanimous class끼리 connector를 공유하려 했지만 hard family에서 실제 cluster size가 거의 1이라 이득이 없었다.
2. `owner-local exact oracle`
   old component 전체 DFS 대신 owner별 exact BFS를 쓰는 라인이었지만 edge work가 더 커져 기각됐다.
3. `BC local-surgery`
   삭제 정점이 든 block만 다시 계산하는 locality 가설이었지만 hard family에서 locality 이득이 거의 없었다.
4. progress40 report가 남긴 다음 pivot은 layout-gate 바깥의 다른 family 이동이 아니라 `zero-span eligibility and fastpath commit` 내부 residual 축소다.
   따라서 branch-local separator-decomposition 확장이나 heuristic-only rewrite를 progress40의 후속으로 해석하면 안 된다.
5. `connector hotpath normalize reuse`는 progress40 sampled aggregate share가 `0.0036%`라서 1차 타깃이 아니다.
   먼저 줄여야 할 것은 `zero-span eligibility and fastpath commit`, 그다음이 `signature source load and materialize`와 `layout signature compare and reuse gate core`다.
6. `progress38_authoritative_close_not_completed`는 carry-forward 메타 상태일 뿐 progress40 package 안에서 actual runnable close로 승격되지 않았다.
   따라서 partial close를 full close처럼 취급하면 안 된다.

## lca_tree_stress_v5 기준 benchmark expectation

### hard family 의미

다음 family는 계속 대표 hard family로 취급해야 한다.

- `comb_rect_dense`
- `multi_comb_rect`
- `multi_comb_cap`
- `caterpillar_rect_dense`

이들은 큰 component가 잘 줄지 않고, 같은 query 묶음이 여러 deletion 단계에 오래 남으며, 같은 witness/connector/dispatch 재계산을 강요한다. 이 성질 때문에 progress40 이후의 성능 평가는 약한 random보다 이 family와 max-N dense 쪽을 먼저 봐야 한다.

### package가 이미 확보한 evidence

- execution layer validation
  `smoke_detached_256` validator OK, elapsed `3.428s`
  `detached_multi_512_sampled_oneoff` validator OK, elapsed `5.006s`
  `synthetic_kill_512`는 supervisor 강제 종료 뒤 `progress40_finalize_case.py`로 terminal `result.json` 복구 확인
- smoke and gate
  `gate_connector_only_dense_256_after` validator OK, elapsed `3.275s`
  `gate_both_on_dense_256_after` validator OK, elapsed `3.376s`
  `gate_both_on_multi_512_after` validator OK, elapsed `4.932s`
- authoritative LOCAL 512 matrix
  dense sampled/base rows는 대체로 `27.138s`에서 `28.082s`
  `after_both_on_multi_512_sampled`는 `5.005s`
- release status
  `both_on_multi_1024_release` validator OK, elapsed `22.703s`

### package가 아직 확보하지 못한 범위

- `both_on_dense_1024_release`
- `both_on_dense_1024_release_repeat`
- `both_on_dense_4096_release`
- `both_on_multi_4096_release`
- long-run terminal row persistence authoritative close

즉 progress40 package는 `partial` 상태다. branch_3가 이보다 강한 주장을 하려면 fresh branch-local evidence가 추가로 필요하다.

### direct aggregate 기준 현재 기대 residual

authoritative sampled after 3개 row 기준 direct aggregate는 아래와 같다.

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| signature source load and materialize | 0.378774 | 24.9643 |
| layout signature compare and reuse gate core | 0.379830 | 25.0339 |
| zero-span eligibility and fastpath commit | 0.758605 | 49.9983 |
| connector hotpath normalize reuse | 0.000054 | 0.0036 |

strict dominant는 없지만 largest residual은 `zero-span eligibility and fastpath commit`이다. progress40 bundle이 남긴 안전한 다음 한 줄은 아래다.

`next pivot after layout-gate round: zero-span eligibility and fastpath commit`

## branch_3에서 이 메모를 어떻게 써야 하는가

1. solver-side 변경이 progress40-derived line인지 먼저 이 문서 기준으로 확인한다.
2. 새 프로파일링은 `zero-span fastpath`를 1차 축, `state materialization`과 `layout gate`를 2차 축으로 본다.
3. `lca_tree_stress_v5` evidence를 갱신할 때는 hard family와 dense representative를 우선 사용하고, partial authoritative 범위를 넘는 주장을 자동으로 하지 않는다.
