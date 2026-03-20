rewrite-r manual-only local harness에서 occurrence metadata full rebuild까지 반영한 뒤 재실행했다.

- 실행 명령:
  `./build/rewrite_r_harness --backend ogdf --mode rewrite-r --manual-only --dump-dir dumps/rewrite_r_manual`
- 결과:
  stage bundle로 떨어지기 전에 fatal 종료
- fatal:
  `[HARNESS] S skeleton must have at least 3 edges`
- 실행 로그:
  `verify/rewrite_r_manual_run.log`

이번 bundle에는 최신 rewrite-r 관련 소스, 시작 checkpoint, 그리고 fatal 실행 로그를 포함했다.
