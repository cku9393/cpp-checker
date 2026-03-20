# rewrite-r-seq defer-sp 결과

## 실행 정보
- clean rebuild 완료
- replay 실행:
  - `./build/rewrite_r_harness --backend ogdf --mode rewrite-r-seq-replay --seed 1 --tc-index 40 --target-step 1 --dump-dir dumps/rewrite_r_seq_defer_sp/replay`

## 결과
- 이번 단계는 성공 기준을 충족하지 못함
- `GPS_SAME_TYPE_SP_ONLY` 를 pre-normalize soft defer 로 바꾸면
  - `seqRewriteWholeCoreFallbackCount = 0`
  - `seqDeferredSameTypeSPCount = 1`
  - fallback 은 사라짐
- 하지만 replay 는 `SEQ_ACTUAL_INVARIANT_FAIL` 로 실패
  - reason: `actual reduced: adjacent same-type S/P nodes are forbidden`

## 핵심 관찰
- defer sample:
  - `dumps/rewrite_r_seq_defer_sp/seq_defer_same_type_sp_seed1_tc40_step1.txt`
- failure bundle:
  - `dumps/rewrite_r_seq_defer_sp/replay/SEQ_ACTUAL_INVARIANT_FAIL_seed1_tc40_step1.txt`
- 이 둘 다 `GPS_SAME_TYPE_SP_ONLY` 를 기록함
- `AFTER_NORMALIZE` 시점에도
  - `node 0 = S`
  - `node 1 = S`
  - same-type S/P adjacency 가 그대로 남아 있음
- 즉 현재 harness 경로에서는 normalize 가 structural same-type S/P cleanup 을 하지 않음

## 해석
- 이번 변경으로 premature whole-core fallback 은 제거됨
- 하지만 correctness green 을 유지하지 못함
- replay 실패가 earliest-stage 로 바로 재현되므로 여기서 중단하는 것이 맞음

## 다음 타깃
- 다음 단계는 defer policy 확장이 아니라
  - `sequence-only local S/P cleanup`
- 더 정확히는
  - `AFTER_CLEAR_PRESERVE` 에서 생긴 same-type S/P adjacency 를
  - normalize 이전 또는 normalize 과정 앞단에서 실제로 해소하는 구조적 정리 로직이 필요함
