# progress28 pointer validation and rebinding

이번 패키지는 **partial authoritative** 상태다. progress27 베이스에서 `ENABLE_POINTER_REBIND_OPT` 경로를 추가하고, `prebind_*` 계열 summary key를 연결한 뒤 smoke와 gate, 일부 LOCAL rerun만 회수했다. 전체 512 LOCAL matrix와 RELEASE rerun, 4096 representative는 이번 시도에서 authoritative row를 끝까지 만들지 못했다.

## 무엇을 바꿨는지

`pointer validation and rebinding`을 별도 표면으로 보기 위해 summary에 `time_prebind_*`와 `prebind_*` 계열을 추가했다. 현재 구현은 기존 `pwrite_*` 및 `tresolve_*` 표면에서 파생한 **proxy-based decomposition**이다. 즉 hot-loop 내부에 완전히 독립된 validation and rebinding micro-timer를 심은 상태는 아니고, progress27의 proxy caveat를 줄이기 위한 중간 단계다.

## smoke

## correctness gate

| run | elapsed_sec | validator |
| --- | ---: | --- |
| gate_connector_only_dense_256_after | 3.38 | OK |
| gate_both_on_dense_256_after | 3.52 | OK |
| gate_both_on_multi_512_after | 5.30 | OK |

모든 gate rerun에서 `validator OK`, `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, `unanimous_baseline_path_calls=0`를 유지했다.

## 완료된 LOCAL rerun

| run | elapsed_sec | validator | note |
| --- | ---: | --- | --- |
| before_connector_only_dense_512_base | 27.12 | OK | completed |
| after_connector_only_dense_512_sampled | 28.03 | OK | completed |
| before_both_on_dense_512_base | - | missing | orchestration interrupted before authoritative result row |
| after_both_on_dense_512_sampled | - | missing | orchestration interrupted before authoritative result row |

## partial prebind observations

### gate_both_on_multi_512_after

- `time_prebind_cached_pointer_validate_ns=1310612`
- `time_prebind_slot_owner_match_check_ns=1310612`
- `time_prebind_same_pointer_fastpath_ns=0`
- `time_prebind_stale_pointer_detect_ns=12067`
- `time_prebind_rebind_target_lookup_ns=12067`
- `time_prebind_rebind_commit_ns=1212996`
- `time_prebind_metadata_patch_ns=1436713`
- `time_prebind_connector_hotpath_pin_apply_ns=0`
- `prebind_cached_pointer_validate_calls=629`
- `prebind_slot_owner_match_checks=629`
- `prebind_slot_owner_match_hits=0`
- `prebind_same_pointer_fastpath_hits=0`
- `prebind_stale_pointer_detect_calls=629`
- `prebind_rebind_target_lookup_calls=629`
- `prebind_rebind_commit_calls=629`
- `prebind_metadata_patch_calls=629`
- `prebind_connector_hotpath_pin_apply_calls=0`

### after_connector_only_dense_512_sampled

- `time_prebind_cached_pointer_validate_ns=2443`
- `time_prebind_slot_owner_match_check_ns=2443`
- `time_prebind_same_pointer_fastpath_ns=0`
- `time_prebind_stale_pointer_detect_ns=0`
- `time_prebind_rebind_target_lookup_ns=0`
- `time_prebind_rebind_commit_ns=5339`
- `time_prebind_metadata_patch_ns=4361`
- `time_prebind_connector_hotpath_pin_apply_ns=27`
- `prebind_cached_pointer_validate_calls=0`
- `prebind_slot_owner_match_checks=0`
- `prebind_slot_owner_match_hits=0`
- `prebind_same_pointer_fastpath_hits=0`
- `prebind_stale_pointer_detect_calls=0`
- `prebind_rebind_target_lookup_calls=0`
- `prebind_rebind_commit_calls=0`
- `prebind_metadata_patch_calls=0`
- `prebind_connector_hotpath_pin_apply_calls=27`

## current status

이 시점에서 progress28은 authoritative completion에 도달하지 못했다. 전체 LOCAL 512 before/after matrix와 dense 1024 RELEASE, 4096 representative rerun이 빠져 있어, `pointer validation and rebinding` 버킷 내부 next pivot을 새로 확정할 충분한 clean dataset이 없다.

따라서 현재 가장 안전한 결론은 progress27의 authoritative clean LOCAL 결론을 유지하는 것이다.

`next pivot after target-resolve round: pointer validation and rebinding`