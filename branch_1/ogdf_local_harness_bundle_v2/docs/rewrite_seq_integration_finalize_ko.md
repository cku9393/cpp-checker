# rewrite-seq Integration Finalize

## 운영 상태
- default solver path는 `rewrite-seq`다.
- legacy solver path는 diagnostic-only다.
- hard gate는 `regressions/rewrite_seq_cases.json` 6개로 고정한다.
- compare baseline 표준은 `--baseline oracle --oracle-handoff normalize`다.
- summary writer는 atomic write + self-parse validation을 유지한다.

## 권장 검증 명령
- one-click release gate:
  - `./scripts/run_rewrite_seq_release_gate.sh`
- hard compare:
  - `./build/rewrite_r_harness --backend ogdf --mode solver-compare --manifest regressions/rewrite_seq_cases.json --baseline oracle --oracle-handoff normalize --dump-dir dumps/integration_finalize/hard_compare`
- random sanity:
  - `./build/rewrite_r_harness --backend ogdf --mode solver-compare --baseline oracle --oracle-handoff normalize --seed 1 --rounds 100 --dump-dir dumps/integration_finalize/random_sanity_s1_r100`
- direct solver smoke:
  - `./build/rewrite_r_harness --backend ogdf --mode rewrite-seq --seed 1 --rounds 10 --dump-dir dumps/integration_finalize/direct_solver_smoke`

## 빌드 정책
- `USE_REWRITE_SEQ_ENGINE=ON`을 기본값으로 유지한다.
- clean rebuild에서도 `${build}/generated/ogdf_feature_config.hpp`가 생성돼야 한다.
- clean build 검증 명령:
  - `cmake -S . -B build -DUSE_OGDF=ON -DOGDF_ROOT="${OGDF_ROOT}" -DHARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON -DUSE_REWRITE_SEQ_ENGINE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
  - `cmake --build build -j1 --verbose`

## 롤백 / 진단 경로
- legacy direct path가 필요하면 명시적으로 `--mode rewrite-r`를 사용한다.
- compare 진단은 기본적으로 `--baseline oracle --oracle-handoff normalize`를 유지하고, 다른 baseline은 diagnostic-only로 본다.
- hard gate mismatch가 나오면 earliest case 1개만 replay 체인으로 내린다.

## Release Candidate 체크리스트
- hard compare 6개 green
- direct solver smoke green
- random sanity green
- `oracleVsRewriteMismatchCount = 0`
- `summaryWriteMode = atomic`
- `summaryValidated = true`
- legacy path status = diagnostic-only
