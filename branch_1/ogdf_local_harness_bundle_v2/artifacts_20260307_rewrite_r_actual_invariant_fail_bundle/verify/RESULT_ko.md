rewrite-r manual-only local harness에서 `normalizeTouchedRegion` unwired는 제거됐다.

- 실행 명령:
  `./build/rewrite_r_harness --backend ogdf --mode rewrite-r --manual-only --dump-dir dumps/rewrite_r_manual`
- earliest stage:
  `LOCAL_ACTUAL_INVARIANT_FAIL`
- reason:
  `actual occ: occurrence pole mismatch`
- first bundle:
  `verify/dumps/LOCAL_ACTUAL_INVARIANT_FAIL_seed1_tc0.txt`

이번 bundle에는 rewrite-r mode 최신 소스, 현재 failure dump, 그리고 rewrite-r 시작 checkpoint를 포함했다.
