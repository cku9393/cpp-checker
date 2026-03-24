# Progress32 partial update

산출물

소스: `boj28350_literature_progress32_slot_owner_update_owner_index_touch.cpp`

supervised runner: `run_progress32_case_supervised.py`

finalize script: `progress32_finalize_case.py`

merge script: `merge_progress32_results.py`

현재 상태는 partial authoritative다.

## 이번에 실제로 끝난 범위
1. progress31 베이스에서 progress32 브랜치를 만들고 `ENABLE_SLOT_OWNER_UPDATE_OPT`를 추가했다.
2. `time_sowner_*`, `sowner_*` 계열을 소스와 LOCAL summary까지 연결했다.
3. detached supervised runner와 finalize-only utility, merge script, journal, resume script를 만들었다.
4. LOCAL and RELEASE 빌드를 다시 성공시켰다.
5. tiny sampled smoke와 gate 3개를 다시 돌렸고 모두 `validator OK`였다.
6. direct slot-owner evidence로 아래 authoritative row를 확보했다.
   `direct_conn_dense_256_sampled_after`
   `direct_both_dense_256_sampled_after`
   `before_connector_only_dense_512_sampled`
   `before_both_on_dense_512_sampled`
   `before_both_on_multi_512_sampled`
   `after_connector_only_dense_512_sampled`
   `after_both_on_dense_512_sampled`
   `gate_both_on_multi_512_after`
   `after_both_on_multi_1024_release`

## smoke와 gate
- smoke_p32_multi64_after: validator_ok=True, elapsed_sec=3.278
- gate_connector_only_dense_256_after: validator_ok=True, elapsed_sec=4.384
- gate_both_on_dense_256_after: validator_ok=True, elapsed_sec=4.376
- gate_both_on_multi_512_after: validator_ok=True, elapsed_sec=5.004

모든 gate rerun은 `validator OK`, `local_active_mismatch=0`, `local_active_partition_mismatch=0`, `debug_touched_missing_classes=0`, `piece_materialize_fallback_calls=0`, `support_rebuild_fallback_calls=0`, `unanimous_baseline_path_calls=0`를 유지했다.

## sampled rows currently available
- before_connector_only_dense_512_sampled: validator_ok=True, elapsed_sec=25.073
- before_both_on_dense_512_sampled: validator_ok=True, elapsed_sec=27.188
- before_both_on_multi_512_sampled: validator_ok=True, elapsed_sec=5.005
- after_connector_only_dense_512_sampled: validator_ok=True, elapsed_sec=27.147
- after_both_on_dense_512_sampled: validator_ok=True, elapsed_sec=28.843
- gate_both_on_multi_512_after: validator_ok=True, elapsed_sec=5.004

## direct slot-owner sampled aggregate
| subaxis | aggregate_ms | share_pct |
| --- | ---: | ---: |
| slot-owner lookup and same-owner validation | 2.062522 | 24.6 |
| slot-owner patch apply and commit | 4.221184 | 50.4 |
| owner-side index touch and owner index search | 2.096212 | 25.0 |
| connector hotpath owner patch apply | 0.000062 | 0.0 |

이 aggregate 기준으로 현재까지의 safe partial direct conclusion은 `slot-owner patch apply and commit`이 largest residual이라는 것이다.

## release rows currently available
- after_both_on_multi_1024_release: validator_ok=True, elapsed_sec=19.288

## caveat
이번 progress32는 detached supervised runner, finalize-only utility, smoke, gate, 일부 sampled rows, multi 1024 release 한 건까지는 확보했지만, progress31 carry-forward close, 512 base matrix, dense 1024 release and repeat, 4096 representative authoritative close는 아직 incomplete다. 따라서 아래 결론은 current safe partial conclusion이다.

## 마지막 결론
`next pivot after slot-owner round: slot-owner patch apply and commit`
