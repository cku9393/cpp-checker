# authoritative completion을 100점으로 만들기 위한 계획 문서

## 문서 목적

현재 기준점의 핵심은 “support8 slice completion recovery”가 아니라 “support8 slice completion closure 이후 무엇이 아직 project-wide 100점이 아닌가”를 분리하는 것이다.

- current support8 classification: `support8_authoritative_completion_locked`
- pass1 / pass2 / pass3: all locked
- required docs `39 / 39`, required artifacts `8 / 8`
- top-level current verified theorem / audit item: all fresh current-runtime generated

즉 현재 병목은 빈 상태가 아니라 slice 밖 provenance expansion과 archive-wide consistency 범위다.

## 1. support8 slice에서 이미 닫힌 것

현재 current verified 사실은 다음과 같다.

- exact-basis payload `96`
- basis-only theorem trio
- family-chain top theorem object layer
- shell15 frontier pair
- support8 antecedent15 shell theorem
- support8 outside-bounded tail pattern theorem
- support8 tail obstruction chain theorem
- support8 authoritative completion lock
- artifact / document / rerun / freshness audit

따라서 support8 slice 안에서는 “completion pending”이 아니라 “completion achieved”가 맞다.

## 2. 왜 아직 project-wide 100점 completion은 아닌가

support8 slice가 닫혔다고 해서 archive 전체가 100점 completion인 것은 아니다.

현재 남은 범위 문제는 다음 두 축이다.

1. lower-frontier first-class inventory에 shell11/shell12 pair `4`개가 direct shell15 dependency subset 밖 mixed inventory row로 남아 있다.
2. family-chain lower triple/quadruple/quintuple/sextuple/septuple layers는 top theorem object가 fresh여도 여전히 imported provenance caveat를 가진다.

즉 현재 bottleneck은 문서 누락도, artifact 누락도, rerun 미재현도 아니다.  
현재 bottleneck은 “어디까지를 fresh current-runtime authoritative data로 더 끌어올릴 것인가”다.

## 3. 100점 completion 정의

이 문서에서 100점 completion은 다음 셋을 동시에 만족하는 상태다.

1. current verified support slice가 lock을 유지한다.
2. top-level theorem-data, lower-level theorem-data, audit data, docs, artifacts, rerun stamps가 provenance caveat 없이 더 넓은 범위까지 current authoritative path로 닫힌다.
3. preserved archival notes는 historical evidence로 남되 current verified와 혼동되지 않는다.

## 4. 다음 계획

현재 다음 계획은 support8 slice recovery가 아니라 completion 확장 계획이다.

### priority 1

lower-frontier inventory-only shell11/shell12 pair `4`개를 actual current constructor/cache path로 올릴지 결정한다.

### priority 2

family-chain lower imported layers를 어디까지 fresh current-runtime generated로 승격할지 결정한다.

### out of scope

- shell16
- higher-support expansion
- BOJ solver

이 셋은 현재 completion recovery의 필수 항목이 아니다.
