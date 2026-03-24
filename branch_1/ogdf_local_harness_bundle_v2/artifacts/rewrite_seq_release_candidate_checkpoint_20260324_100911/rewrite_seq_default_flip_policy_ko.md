# rewrite-seq Default Flip Policy

## 기본 정책
- default solver path는 `rewrite-seq`다.
- 빌드 기본값은 `USE_REWRITE_SEQ_ENGINE=ON`으로 고정한다.
- legacy baseline / legacy solver path는 삭제하지 않고 diagnostic-only 경로로 남긴다.

## Compare 기준
- compare baseline의 표준은 `--baseline oracle --oracle-handoff normalize`다.
- compare canonicalization 규칙은 유지한다.
- legacy compare는 diagnostic-only로만 사용한다.

## Regression Gate
- hard regression manifest `regressions/rewrite_seq_cases.json` 6개는 immutable이다.
- random compare는 release sanity gate로 유지한다.
- default flip 이후에도 hard compare와 최소 random sanity가 계속 green이어야 한다.

## 운영 원칙
- frozen rewrite-seq engine semantics는 수정하지 않는다.
- 이번 flip은 routing default만 바꾸는 변경이다.
- direct legacy 실행은 명시적 `--mode rewrite-r` 같은 opt-in diagnostic 경로로만 사용한다.

## Validation 체크리스트
- hard compare 6개 green
- random sanity green
- summary writer atomic/validated 유지
- legacy path status는 diagnostic-only로 유지
