# Rewrite Sequence Solver Integration Checklist

- feature flag 이름: `USE_REWRITE_SEQ_ENGINE`
- API entrypoint: `runRewriteSequenceToFixpoint(ReducedSPQRCore &, RewriteSeqStats &, std::string &)`
- freeze gate:
  - `rewrite-r` single-step green
  - `rewrite-r-seq` green
  - `rewrite-r-seq-regression` failedCases = 0
  - `rewrite-r-seq-bench`에서 `seqFallbackCaseCount == 0`
  - `rewrite-r-seq-bench`에서 `seqRewriteWholeCoreFallbackCount == 0`

## Solver Wiring

- solver 쪽 feature flag 분기 추가
  - `USE_REWRITE_SEQ_ENGINE=OFF`: 기존 solver path 유지
  - `USE_REWRITE_SEQ_ENGINE=ON`: current reduced core build 후 `runRewriteSequenceToFixpoint(...)` 호출
- solver가 넘겨야 하는 최소 입력:
  - initial `ReducedSPQRCore`
  - 호출당 fresh `RewriteSeqStats`
  - 실패 reason용 `std::string why`
- solver가 확인해야 하는 최소 출력:
  - `bool ok`
  - `RewriteSeqStats.reachedFixpoint`
  - `RewriteSeqStats.completedSteps`
  - 실패 시 `RewriteSeqStats.failureStage`, `failureWhere`, `failureWhy`

## Compare Gate

- solver compare mode 준비:
  - legacy solver output explicit graph
  - rewrite-seq engine output explicit graph
  - `checkEquivalentExplicitGraphs(...)`로 compare
- compare mismatch 시 남길 것:
  - solver input signature
  - engine `RewriteSeqStats`
  - explicit got/expected
  - failing seed/tc/step 또는 solver case id

## Rollout Rules

- 1단계:
  - compare-only shadow mode
  - solver 결과는 legacy path만 사용
- 2단계:
  - internal flag-on path
  - compare mismatch 시 legacy rollback
- 3단계:
  - default-on 전환
  - regression + bench를 release gate로 유지

## Freeze Reference

- freeze note:
  - `docs/rewrite_seq_engine_freeze_ko.md`
- regression manifest:
  - `regressions/rewrite_seq_cases.json`
