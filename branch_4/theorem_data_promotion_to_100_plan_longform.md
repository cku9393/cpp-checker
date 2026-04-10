# theorem-data 승격 완성도를 100점으로 만들기 위한 계획 문서

## 문서 목적

theorem-data promotion 관점에서 이번 기준점의 핵심 성과는 네 줄로 정리된다.

- support8 lock은 current runtime에서 유지된다.
- current bundle metadata와 imported provenance metadata는 코드 / TSV / markdown report에서 분리된다.
- top-level current verified theorem-data item은 모두 `fresh_current_runtime_generated`다.
- 남은 promotion 과제는 top-level mixed 해소가 아니라 lower-frontier inventory-only row와 family-chain lower imported layers의 범위 결정이다.

## 1. 현재 promotion substrate

현재 top-level machine-readable inventory는 theorem / claim / audit item을 다음 네 층으로 나눈다.

- fresh current runtime generated: `16`
- current runtime validated imported data: `0`
- mixed: `0`
- archival only: `3`

이건 현재 문장 설명이 아니라 runtime files에 남는다.

- `branch_4/90/runtime/theorem_data_provenance_inventory_90.tsv`
- `branch_4/90/runtime/provenance_audit_fingerprint_90.tsv`
- `branch_4/90/runtime/lower_frontier_ladder_inventory_90.tsv`
- `branch_4/90/runtime/support8_antecedent15_shell_theorem_generation_audit_90.tsv`

## 2. 지금 current verified인 theorem-data layer

현재 support8 slice에서 fresh current-runtime generated로 올라온 top-level item은 다음과 같다.

- exact minimal basis size `96`
- basis-only theorem trio
- bounded family-chain theorem
- family-chain self verification
- shell15 frontier pair
- support8 antecedent15 shell theorem
- support8 outside-bounded tail pattern theorem
- support8 tail obstruction chain theorem
- support8 authoritative completion lock
- artifact / document / rerun / freshness audit

즉 top-level theorem-data promotion 관점에서 support8 slice는 현재 닫혀 있다.

## 3. 왜 아직 theorem-data 100점이 아닌가

theorem-data 100점이 아직 아닌 이유는 다음 구조 때문이다.

1. lower-frontier first-class inventory에는 shell11/shell12 pair `4`개가 direct shell15 dependency subset 밖 mixed row로 남아 있다.
2. family-chain lower imported layers는 top theorem object가 fresh여도 caveat를 가진다.
3. archival only `3`개 item은 preserved evidence로 남고 current theorem validation 결과로 승격되지 않는다.

즉 지금 남은 문제는 “top-level mixed item이 남아 있다”가 아니라 “broader theorem-data perimeter를 어디까지 fresh path로 넓힐 것인가”다.

## 4. 앞으로의 theorem-data promotion 계획

### 단계 1. lower-frontier perimeter 정리

lower-frontier inventory-only shell11/shell12 pair `4`개를 계속 분리해 둘지, actual current constructor path로 승격할지 결정한다.

### 단계 2. family-chain lower layers 정리

family-chain lower imported layers를 더 아래에서부터 current constructor path로 재구성할 수 있는지 검토한다.

### 단계 3. archival only 보존

archival only 항목은 삭제 대상이 아니다.  
이들은 preserved evidence로 남되, current verified와 섞이지 않도록 유지해야 한다.
