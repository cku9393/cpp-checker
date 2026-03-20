이번 단계는 `SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED` 계열을 block-cut subtype으로만 다시 분해한 계측 단계입니다. code path 동작은 유지했고, clean rebuild 후 `seed=1, rounds=100`, `seed=1, rounds=1000`, `seed=1..10 x 1000`, `seed=1..20 x 5000`를 다시 실행했습니다. correctness는 계속 green입니다.

"
    f"dedup aggregate 기준(`s1_r100 + full_s1_10_r1000 + full_s1_20_r5000`, `s1_r1000` 제외)으로 `rewriteSeqCalls=245100`, `seqFallbackCaseCount=595`, `seqRewriteWholeCoreFallbackCount=595`, `RFT_COMPACT_BUILD_FAIL=337`, `CBF_SELF_LOOP_PRECHECK=337`, `SL_PROXY_ONLY_REMAINDER_OTHER_NOT_BICONNECTED=337`입니다. instrumentation-only 단계라 baseline 대비 이 값들은 그대로 유지됩니다.

"
    f"새 breakdown 결과는 `SLNB_DISCONNECTED=0`, `SLNB_STAR_AROUND_ONE_CUT=0`, `SLNB_COMPLEX_MULTI_CUT=0`, `SLNB_BLOCKS_ALL_TINY=0`, `SLNB_OTHER=337`입니다. 즉 현재 dominant subtype은 `SLNB_OTHER=337`이고, predefined bucket으로는 더 쪼개지지 않았습니다. 다음 단계는 handler 추가가 아니라 `SLNB_OTHER`를 다시 finer subtype으로 설계하는 쪽이 맞습니다.
