# rewrite-r-seq-replay 결과

## 실행 정보
- clean rebuild 후 replay 실행 성공
- 명령:
  - `./build/rewrite_r_harness --backend ogdf --mode rewrite-r-seq-replay --seed 1 --tc-index 40 --target-step 1 --dump-dir dumps/rewrite_r_seq_replay`
- replay bundle:
  - `dumps/rewrite_r_seq_replay/SEQ_REPLAY_CAPTURE_seed1_tc40_step1.txt`

## deterministic 재현 여부
- `seed=1 / tc=40 / step=1` replay 가 1회 deterministic 하게 재현됨
- replay 모드가 target testcase 와 target step 에 도달한 뒤 bundle 을 강제 저장함

## postcheck subtype 분해 결과
- `GOS_POSTCHECK_ADJ_MISMATCH` 내부 detailed subtype:
  - `GPS_SAME_TYPE_SP_ONLY = 1`
  - `GPS_ADJ_METADATA_ONLY = 0`
  - `GPS_ADJ_AND_SAME_TYPE_SP = 0`
  - `GPS_OTHER = 0`
- 상세 reason:
  - `graft: sequence rewrite produced adjacent same-type S/P nodes on arc 0 (0,1)`
  - `postcheck detailed: same-type S/P structural adjacency`

## adj mismatch 여부
- 이번 replay 케이스에서는 adj metadata mismatch 가 보이지 않음
- 증거:
  - `firstBadAdjNode = -1`
  - `expectedAdj = []`
  - `actualAdj = []`
  - `oldNodeAdjArcsBeforeRepair = [0]`
  - `oldNodeAdjArcsAfterRepair = [0]`
- 즉 adjacency repair 는 동작했고, dominant failure 는 structural same-type S/P adjacency 임

## phase별 관찰
- `BEFORE_CLEAR`
  - `node 0 = R`
  - `node 1 = S`
  - live arc `0` 이 `(0,1)` 을 연결
- `AFTER_CLEAR_PRESERVE`
  - `node 0 = S` 로 바뀜
  - `node 1 = S` 유지
  - preserved arc `0` 이 여전히 `(0,1)` 에 살아 있음
  - same-type S/P adjacency 가 이 시점에 이미 성립
- `AFTER_MATERIALIZE`
  - same-type S/P adjacency 지속
- `AFTER_INTERNAL_ARC_CONNECT`
  - same-type S/P adjacency 지속
- `AFTER_PROXY_REWIRE`
  - same-node rehome 후에도 same-type S/P adjacency 지속
- `AFTER_ADJ_REPAIR`
  - adjacency metadata 는 authoritative 상태
  - same-type S/P adjacency 는 그대로 지속
- `AFTER_NORMALIZE`
  - whole-core fallback / normalize 이후 최종 actual 은 정상화됨
  - replay bundle 의 `ActualAfterNormalize` 는 R 단일 노드 형태로 green

## 다음 타깃 결론
- 이번 단계의 fix target 은 `adjacency metadata repair refinement` 가 아님
- 다음 타깃은 `same-type S/P structural cleanup`
- 더 정확히는:
  - 문제는 `same-node rehome 이후`보다 더 이른 `AFTER_CLEAR_PRESERVE` 단계에서 이미 시작됨
  - 따라서 다음 단계는 sequence-only structural cleanup 을 보되, clear-preserve 직후의 local S/P adjacency 처리까지 포함해서 봐야 함
