sequence mode 에서만 `CBF_SELF_LOOP_PRECHECK / SL_PROXY_ONLY_REMAINDER_SPQR_READY`를 `self-loop strip + SPQR-ready remainder reuse + stripped proxy loop reattach` 경로로 처리했습니다. representative replay `seed=1, tc=328, step=1`는 green 이고 `actualInvariantOk=1`, `oracleEquivalentOk=1`, `SPECIAL_SELF_LOOP_SPQR_READY=1`입니다.

"
    f"dedup aggregate 기준(`s1_r100 + full_s1_10_r1000 + full_s1_20_r5000`, `s1_r1000` 제외)으로 `rewriteSeqCalls=110100`, `seqFallbackCaseCount=269`, `seqRewriteWholeCoreFallbackCount=269`입니다. baseline 대비 각각 `-414`, `-414` 변화이고, `CBF_SELF_LOOP_PRECHECK`는 `148`로 baseline `562` 대비 `-414` 줄었습니다. `SL_PROXY_ONLY_REMAINDER_SPQR_READY`는 `0`까지 내려갔고, 새 path stats 는 `attempt=414`, `handled=414`, `fallback=0`입니다.

"
    f"남은 dominant trigger 는 `RFT_COMPACT_BUILD_FAIL=148`이고, 대표 subtype 은 `SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED=148`입니다. 즉 다음 safe target 은 `SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED` 쪽입니다.
