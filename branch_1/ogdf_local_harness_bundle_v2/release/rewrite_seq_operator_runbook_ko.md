# rewrite-seq Operator Runbook

## 기본 실행
- direct solver smoke:
  - `./build/rewrite_r_harness --backend ogdf --mode rewrite-seq --seed 1 --rounds 10 --dump-dir dumps/direct_solver_smoke`
- hard compare:
  - `./build/rewrite_r_harness --backend ogdf --mode solver-compare --manifest regressions/rewrite_seq_cases.json --baseline oracle --oracle-handoff normalize --dump-dir dumps/hard_compare`
- random sanity:
  - `./build/rewrite_r_harness --backend ogdf --mode solver-compare --baseline oracle --oracle-handoff normalize --seed 1 --rounds 100 --dump-dir dumps/random_sanity_s1_r100`

## One-click Gate
- release gate:
  - `./scripts/run_rewrite_seq_release_gate.sh`
- CI wrapper:
  - `./scripts/ci_run_rewrite_seq_release_gate.sh`

## Summary Parse Fail 대처
- compare/log가 green인데 `summary.json` parse fail이면 먼저 atomic summary writer 경로를 의심한다.
- `.json` 대신 `run.log`만 남았으면 gate를 green으로 간주하지 않고 rerun한다.
- `summaryWriteMode=atomic`, `summaryValidated=true`가 아니면 release gate fail로 처리한다.

## Diagnostic-only Legacy Path
- legacy direct path:
  - `./build/rewrite_r_harness --backend ogdf --mode rewrite-r --seed 1 --rounds 10 --dump-dir dumps/legacy_diagnostic_smoke`
- legacy compare:
  - `./build/rewrite_r_harness --backend ogdf --mode solver-compare --manifest regressions/rewrite_seq_cases.json --baseline legacy --dump-dir dumps/legacy_compare_diagnostic`

## Failure Handling Rule
- fail이 나오면 즉시 멈춘다.
- 여러 case를 동시에 추적하지 않는다.
- earliest mismatch 1개만 고정한다.
- compare mismatch면 `solver-compare-replay`
- finalcore seam이면 `solver-finalcore-replay`
- semantic seam이면 `solver-semantic-replay`
- target seam이면 `solver-semantic-target-replay`
- builder seam이면 `explicit-core-builder-replay`
