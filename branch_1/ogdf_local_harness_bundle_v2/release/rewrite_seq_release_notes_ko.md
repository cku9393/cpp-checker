# rewrite-seq Release Notes

## 변경 사항
- default solver path가 `rewrite-seq`로 승격됐다.
- legacy solver path는 삭제하지 않고 diagnostic-only 경로로 남겼다.
- compare baseline의 운영 표준은 `--baseline oracle --oracle-handoff normalize`로 고정했다.
- summary writer는 atomic write + self-parse validation을 사용한다.

## 운영 Gate
- hard gate: `regressions/rewrite_seq_cases.json` 6개
- random sanity gate: 최소 `seed=1, rounds=100`
- direct solver smoke: `--mode rewrite-seq --seed 1 --rounds 10`

## Known Limitations
- legacy direct path는 운영 기본 경로가 아니다.
- compare mismatch가 다시 나오면 earliest case 1개만 고정해서 replay 체인으로 좁힌다.
- large compare campaign은 summary artifact가 아니라 semantic mismatch 기준으로만 fail을 판단한다.

## Rollback
- direct legacy diagnostic:
  - `./build/rewrite_r_harness --backend ogdf --mode rewrite-r --seed 1 --rounds 10 --dump-dir dumps/legacy_diagnostic_smoke`
- compare diagnostic:
  - `./build/rewrite_r_harness --backend ogdf --mode solver-compare --manifest regressions/rewrite_seq_cases.json --baseline legacy --dump-dir dumps/legacy_compare_diagnostic`

## 운영 메모
- release candidate bundle과 release gate summary를 immutable reference로 보존한다.
- clean build에서는 `USE_REWRITE_SEQ_ENGINE=ON`과 `build/generated/ogdf_feature_config.hpp` 생성 경로가 유지돼야 한다.
