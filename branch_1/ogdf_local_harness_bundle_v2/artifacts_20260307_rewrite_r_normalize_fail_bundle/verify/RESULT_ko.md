rewrite-r manual-only local harness에서 `rewriteR_fallback` unwired는 제거됐다.

- 실행 명령:
  `./build/rewrite_r_harness --backend ogdf --mode rewrite-r --manual-only --dump-dir dumps/rewrite_r_manual`
- earliest stage:
  `LOCAL_NORMALIZE_FAIL`
- reason:
  `project_hook_shims.cpp: TODO wire normalizeTouchedRegion -> project::normalizeTouchedRegion(core, why)`
- first bundle:
  `verify/dumps/LOCAL_NORMALIZE_FAIL_seed1_tc0.txt`

이번 bundle에는 rewrite-r mode 추가 상태와 `rewriteProjectRFallback` 연결 이후의 최신 관련 소스, failure dump, 시작 checkpoint를 포함했다.
