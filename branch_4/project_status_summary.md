# 프로젝트 현황 정리

## 현재 current verified 상태

현재 workspace에서 직접 재현한 기준 코드는 `branch_4/90/full_dynamic_top_tree_engine_90.cpp`다.  
이 코드는 여전히 `This is NOT the complete BOJ solver`라고 명시하므로, 현재 verified 성공은 solver 완성이 아니라 support8 / shell15 / tail / completion-lock proof system의 current reproduction이다.

현재 verified 사실은 다음과 같다.

- release compile: verified
- LOCAL_TEST compile: verified
- active runtime root: `branch_4/90/runtime`
- required docs: `39 / 39`
- required artifacts: `8 / 8`
- LOCAL_TEST pass1: `support8_authoritative_completion_locked`
- LOCAL_TEST pass2: `support8_authoritative_completion_locked`
- LOCAL_TEST pass3: `support8_authoritative_completion_locked`
- current reproducible classification: `support8_authoritative_completion_locked`

## current bundle / imported provenance split

현재 top-level provenance inventory 기준 수치는 다음과 같다.

- provenance inventory item count: `19`
- fresh current runtime generated: `16`
- current runtime validated imported data: `0`
- mixed: `0`
- archival only: `3`

즉 현재 current verified top-level theorem / audit item은 모두 fresh current-runtime generated로 올라왔다.

- exact minimal basis size `96`
- exact n=5 basis-only theorem
- bounded n=6, c<=5 basis-only theorem
- bounded n=7, c<=3 basis-only theorem
- bounded family-chain theorem
- family-chain self verification
- antecedent plus twelve frontier
- support8 antecedent15 frontier
- support8 antecedent15 shell theorem
- support8 outside-bounded tail pattern theorem
- support8 tail obstruction chain theorem
- support8 authoritative completion lock
- artifact / document / rerun / freshness audit

다만 imported provenance 자체가 사라진 것은 아니다.  
현재 lower-frontier first-class inventory는 `23`개 row를 가지며, direct shell-theorem dependency subset `19`개는 freshized되었고 shell11/shell12 pair `4`개는 direct subset 밖의 inventory-only mixed row로 남아 있다.

## 90 archival claim

`branch_4/90/` preserved bundle은 여전히 archival claim을 보존한다.

- archival classification: `support8_authoritative_completion_locked`
- preserved `_90.md` 노트들은 역사적/보존된 요약 노트로 유지된다
- archival only top-level item count: `3`

현재 workspace는 archival classification을 현재 rerun으로 재현했고, top-level theorem-data도 fresh current-runtime generated로 끌어올렸다.  
남은 caveat은 support8 slice 바깥 확장이나 lower-frontier inventory-only shell11/shell12 pair, 그리고 family-chain lower imported layers를 어디까지 계속 freshize할지의 범위 문제다.

## 분류

- current reproducible classification: `support8_authoritative_completion_locked`
- archival classification: `support8_authoritative_completion_locked`
