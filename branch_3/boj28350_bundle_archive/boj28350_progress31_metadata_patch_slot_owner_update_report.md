# Progress31 partial update

산출물

소스: `boj28350_literature_progress31_metadata_patch_slot_owner_update.cpp`

러너: `run_progress31_case_transactional.py`

merge script: `merge_progress31_results.py`

현재 상태는 partial authoritative다.

이번 시도에서 실제로 끝난 범위는 다음과 같다.

1. progress30 베이스에서 progress31 브랜치를 만들고 `ENABLE_METADATA_PATCH_SLOT_OWNER_OPT`를 추가했다.
2. `time_mpatch_*`, `mpatch_*` 계열을 소스와 LOCAL summary까지 연결했다.
3. LOCAL and RELEASE 빌드를 다시 성공시켰다.
4. tiny sampled smoke와 gate 3개를 다시 돌렸고 모두 `validator OK`였다.
5. direct metadata patch evidence로는 아래 authoritative row를 확보했다.
   `direct_conn_dense_256_sampled_after`
   `direct_both_dense_256_sampled_after`
   `gate_both_on_multi_512_after`

현재 확보된 direct sampled aggregate는 아래와 같다.

same-target fastpath and no-op commit: 40228 ns, 1.0%
metadata field patch apply core: 2050475 ns, 49.3%
slot-owner update and owner-side index touch: 2071043 ns, 49.8%
connector hotpath patch apply: 8 ns, 0.0%

즉 현재 partial direct 기준 largest residual은 `slot-owner update and owner-side index touch`다.

현재 가장 안전한 결론은 아래다.

`next pivot after metadata-patch round: slot-owner update and owner-side index touch`

다만 아직 authoritative하게 닫히지 않은 범위는 남아 있다.

LOCAL 512 before and after full matrix
1024 RELEASE rows
4096 representative rows

또 현재 runner는 shorter case에서는 terminal `result.json`을 남기지만, longer case에서는 session 종료나 environment interruption 때문에 finalize failure mode가 아직 있다. 그래서 progress31 패키지는 현재 partial authoritative 상태로 둔다.
