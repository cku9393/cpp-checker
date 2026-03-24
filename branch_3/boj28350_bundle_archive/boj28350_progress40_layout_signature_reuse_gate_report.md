# boj28350 progress40 layout signature compare and reuse gate

## What changed

Progress39 base에 `ENABLE_LAYOUT_SIGNATURE_GATE_OPT` 브랜치를 추가했고 `time_lgate_*`, `lgate_*` 계열을 source와 LOCAL summary까지 연결했다.

새 direct subaxis는 아래다.

`time_lgate_sig_source_load_ns`  
`time_lgate_sig_materialize_ns`  
`time_lgate_sig_compare_core_ns`  
`time_lgate_same_layout_gate_ns`  
`time_lgate_zero_span_eligibility_gate_ns`  
`time_lgate_fastpath_commit_core_ns`  
`time_lgate_connector_hotpath_reuse_ns`

support artifact도 progress40 이름으로 분리했다.

`run_progress40_case_supervised.py`  
`progress40_finalize_case.py`  
`reconcile_progress40_results.py`  
`merge_progress40_results.py`  
`progress40_case_journal.jsonl`  
`progress40_resume_remaining.sh`

## Execution layer status

Detached supervised launch로 terminal `result.json`을 남기는 경로는 짧은 케이스와 512 sampled one-off에서 다시 확인했다.

검증된 케이스는 아래다.

`smoke_detached_256` validator OK, elapsed 3.428s  
`detached_multi_512_sampled_oneoff` validator OK, elapsed 5.006s

또 synthetic failure test도 확인했다.

`synthetic_kill_512`는 supervisor 강제 종료 뒤 `progress40_finalize_case.py`로 terminal `result.json`을 복구했다.

다만 long run terminal row persistence는 아직 authoritative하게 닫히지 않았다. dense 1024 release and repeat, 4096 representative는 여전히 missing이다.

## Smoke and gate

`gate_connector_only_dense_256_after` validator OK, elapsed 3.275s  
`gate_both_on_dense_256_after` validator OK, elapsed 3.376s  
`gate_both_on_multi_512_after` validator OK, elapsed 4.932s

## Authoritative LOCAL 512 matrix

`before_connector_only_dense_512_base` validator OK, elapsed 28.082s  
`before_both_on_dense_512_base` validator OK, elapsed 27.358s  
`before_connector_only_dense_512_sampled` validator OK, elapsed 27.138s  
`before_both_on_dense_512_sampled` validator OK, elapsed 27.721s  
`before_both_on_multi_512_sampled` validator OK, elapsed 4.980s

`after_connector_only_dense_512_base` validator OK, elapsed 27.138s  
`after_both_on_dense_512_base` validator OK, elapsed 27.708s  
`after_connector_only_dense_512_sampled` validator OK, elapsed 27.292s  
`after_both_on_dense_512_sampled` validator OK, elapsed 27.138s  
`after_both_on_multi_512_sampled` validator OK, elapsed 5.005s

## Direct layout signature compare and reuse gate aggregate

현재 direct aggregate는 authoritative sampled after 3개 row 기준이다.

`after_connector_only_dense_512_sampled`  
`after_both_on_dense_512_sampled`  
`after_both_on_multi_512_sampled`

| category | aggregate_ms | share_pct |
| --- | ---: | ---: |
| signature source load and materialize | 0.378774 | 24.9643 |
| layout signature compare and reuse gate core | 0.379830 | 25.0339 |
| zero-span eligibility and fastpath commit | 0.758605 | 49.9983 |
| connector hotpath normalize reuse | 0.000054 | 0.0036 |

## Release status

`both_on_multi_1024_release` validator OK, elapsed 22.703s

`both_on_dense_1024_release` missing  
`both_on_dense_1024_release_repeat` missing  
`both_on_dense_4096_release` missing  
`both_on_multi_4096_release` missing

## Carry-forward reconciliation

`progress38_authoritative_close_not_completed`는 메타 carry-forward 상태로 남아 있다. 현재 progress40 package 안에서는 actual runnable row close로 승격하지 못했다.

## Current conclusion

현재 authoritative clean LOCAL direct aggregate 기준 strict dominant는 없지만 largest residual은 `zero-span eligibility and fastpath commit`이다.

`next pivot after layout-gate round: zero-span eligibility and fastpath commit`

## Current package state

Progress40 source patch, support artifact, execution layer validation, smoke, gate, authoritative LOCAL 512 matrix, 그리고 `both_on_multi_1024_release` 1회까지는 확보했다.  
하지만 progress39 carry-forward close와 dense 1024 release and repeat, 4096 representative는 아직 partial authoritative 상태다.
