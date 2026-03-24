# progress29 direct pointer validation and rebinding report

이번 라운드는 progress28의 proxy-based decomposition caveat를 줄이기 위해, pointer validation and rebinding 경로에 direct hot-loop 계측을 추가하는 작업으로 진행했다.

## 이번에 실제로 바꾼 것

`ENABLE_DIRECT_PREBIND_OPT`를 추가했다.
직접 계측용 `time_dprebind_*`와 `dprebind_*` 키를 넣고, carry-hit 이후 cached pointer consume와 rebind path가 도는 hot loop 내부에서 값을 기록하게 했다.
기존 `time_tresolve_*` 계열은 proxy umbrella로 유지했다.

## smoke와 gate

smoke_p29_multi64_after: validator_ok=True, elapsed_sec=1.24
gate_connector_only_dense_256_after: validator_ok=True, elapsed_sec=2.64
gate_both_on_dense_256_after: validator_ok=True, elapsed_sec=2.93
gate_both_on_multi_512_after: validator_ok=True, elapsed_sec=4.56

모든 gate rerun은 `validator OK`, `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, `unanimous_baseline_path_calls=0`를 유지했다.

## direct hot-loop sampled evidence

| run | cached pointer and slot-owner validation ms | stale detect and target remap lookup ms | rebind commit and metadata patch ms | same-pointer fastpath and connector pin ms | elapsed sec |
| --- | ---: | ---: | ---: | ---: | ---: |
| after_connector_only_dense_256_sampled | 0.000 | 0.000 | 0.004 | 0.000004 | 2.88 |
| after_both_on_dense_256_sampled | 0.826 | 0.033 | 2.709 | 0.000004 | 3.21 |
| gate_both_on_multi_512_after | 1.223 | 0.049 | 4.034 | 0.000000 | 4.56 |

## direct sampled aggregate

| subaxis | aggregate_ms | share_pct |
| --- | ---: | ---: |
| cached pointer and slot-owner validation | 2.049 | 23.1 |
| stale pointer detect and target remap lookup | 0.081 | 0.9 |
| rebind commit and metadata patch | 6.747 | 76.0 |
| same-pointer fastpath and connector hotpath pin apply | 0.000 | 0.0 |

이 aggregate 기준으로 strict dominant는 없지만, 현재까지의 direct hot-loop evidence에서는 `rebind commit and metadata patch`가 가장 크게 남는다.

## release and matrix status

before_both_on_multi_512_sampled: validator_ok=True, elapsed_sec=4.48
after_both_on_multi_1024_release: validator_ok=True, elapsed_sec=27.82

아직 authoritative row를 끝까지 만들지 못한 항목은 다음과 같다.
before_connector_only_dense_512_base
before_connector_only_dense_512_sampled
before_both_on_dense_512_base
before_both_on_dense_512_sampled
after_connector_only_dense_512_base
after_connector_only_dense_512_sampled
after_both_on_dense_512_base
after_both_on_dense_512_sampled
both_on_dense_1024_release
both_on_dense_1024_release_repeat
both_on_dense_4096_release
both_on_multi_4096_release

## caveat

이번 progress29는 direct timer를 실제 hot loop에 연결했지만, authoritative clean LOCAL 512 full matrix와 dense 1024 release, 4096 representative는 아직 incomplete다. 따라서 아래 결론은 current safe partial conclusion이다.

## 마지막 결론

`next pivot after direct-prebind round: rebind commit and metadata patch`