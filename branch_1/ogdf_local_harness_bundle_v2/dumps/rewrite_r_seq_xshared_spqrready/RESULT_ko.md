sequence-only x-shared SPQR-ready direct path를 추가했고 representative replay(seed=1, tc=851, step=1)는 green입니다.

집계 기준은 s1_r100 + full_s1_10_r1000 + per-seed clean full_s1_20_r5000이며, s1_r1000는 seed1 중복이라 제외했습니다. 이 기준에서 rewriteSeqCalls=110100, seqFallbackCaseCount=0, seqRewriteWholeCoreFallbackCount=0 입니다.

핵심 감소폭은 RFT_X_INCIDENT_VIRTUAL_UNSUPPORTED 121 -> 0, XSR_HAFTER_SPQR_READY 121 -> 0, seqFallbackCaseCount 121 -> 0, seqRewriteWholeCoreFallbackCount 121 -> 0 입니다. 새 path stats는 attempt=121, handled=121, fallback=0 입니다.

현재 tracked sequence fallback trigger는 모두 0입니다. 다음 타깃은 새 dominant residual이 다시 생길 때 재선정하면 됩니다.
