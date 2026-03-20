# rewrite-r NB_PATH_OF_BLOCKS optimization

## 결과
- manual-only: green
- rewrite-r random campaign: green
- rewriteCalls: 111100
- compactRejectedFallbackCount: 9543 -> 0
- fallback ratio: 8.59% -> 0.00%
- compactPathOfBlocksHandled: 9543
- rewriteFallbackWholeCoreCount: 0
- rewriteFallbackSpecialCaseCount: 47433
- observed NB_PATH_OF_BLOCKS subtype count: 9543

## 해석
- correctness green을 유지한 채 NB_PATH_OF_BLOCKS 일부 또는 전부를 local special path로 흡수했다.

## 산출물
- summary.json
- comparison.json
- manual_run.log
- logs/*.log
