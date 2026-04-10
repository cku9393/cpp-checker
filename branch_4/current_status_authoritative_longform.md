# 현재 상태 정리: lower-frontier freshization까지 반영한 authoritative 현황

## 1. 문서 목적

이 문서는 브랜치4의 메인 current-status longform이다.  
이번 기준점은 네 문장으로 요약된다.

- support8 proof slice는 현재 workspace에서 `support8_authoritative_completion_locked`까지 3-pass 기준으로 재현된다.
- current bundle metadata와 imported closed-output provenance는 계속 분리해서 읽어야 한다.
- top-level current verified theorem / audit item은 이제 모두 `fresh_current_runtime_generated`다.
- 남은 caveat은 top-level mixed item이 아니라 lower-frontier first-class inventory의 shell11/shell12 pair와 family-chain lower imported layers다.

## 2. 현재 source of truth 구조

현재 authoritative 판단에서 먼저 보는 것은 문서가 아니라 코드와 runtime이다.

1. `branch_4/90/full_dynamic_top_tree_engine_90.cpp`
2. `branch_4/90/runtime/` 아래 current docs / artifacts / audit files
3. `branch_4/90/runtime/theorem_data_provenance_inventory_90.tsv`
4. `branch_4/90/runtime/provenance_audit_fingerprint_90.tsv`
5. `branch_4/90/runtime/lower_frontier_ladder_inventory_90.tsv`
6. `branch_4/90/runtime/support8_antecedent15_shell_theorem_generation_audit_90.tsv`
7. preserved `_90` notes

## 3. current workspace reproduction

### 3-1. code identity

- header says: `This is NOT the complete BOJ solver`
- non-`LOCAL_TEST` `main()`: dummy `return 0;`
- 현재 verified 성공의 의미: solver 완성 아님, support8 proof-system recovery 성공

### 3-2. compile / rerun

- release compile: verified
- LOCAL_TEST compile: verified
- pass1: `support8_authoritative_completion_locked`
- pass2: `support8_authoritative_completion_locked`
- pass3: `support8_authoritative_completion_locked`

### 3-3. docs / artifacts / audits

- required docs: `39 / 39`
- required artifacts: `8 / 8`
- artifact completion audit: verified
- document completion audit: verified
- rerun completion audit: verified
- audit freshness: verified
- current reproducible classification: `support8_authoritative_completion_locked`

## 4. current bundle metadata와 imported provenance의 분리

### current bundle metadata

- current bundle version: `90`
- current bundle source path: `branch_4/90/full_dynamic_top_tree_engine_90.cpp`
- current bundle summary path: `branch_4/90/project_status_summary_90.md`
- current runtime summary path: `branch_4/90/runtime/project_status_summary_90.md`

### imported closed-output provenance

- family-chain output `57`
- lower frontier ladder `67/69/70/71/72/74/75/76/77/79`
- archival shell15 frontier source `84`
- retained provenance path example: `/mnt/data/full_dynamic_top_tree_engine_84.cpp`

핵심은 이렇다.  
`90 current bundle`과 imported theorem-data provenance는 동시에 사실이지만 서로 다른 층이다.  
이번 라운드의 변화는 lower-frontier direct dependency subset까지 current constructor/cache path로 올라와 top-level mixed item이 사라졌다는 점이지, imported provenance catalog 자체가 삭제되었다는 뜻은 아니다.

## 5. provenance inventory 결과

### 5-1. top-level inventory

현재 machine-readable provenance inventory는 `19`개 item을 가진다.

- current verified: `16`
- archival claim: `3`
- fresh current runtime generated: `16`
- current runtime validated imported data: `0`
- mixed: `0`
- archival only: `3`

즉 top-level current verified theorem / audit item은 모두 fresh current-runtime generated다.

### 5-2. lower-frontier first-class inventory

별도 lower-frontier inventory는 `23`개 row를 가진다.

- direct shell-theorem dependency subset: `19`
- direct shell-theorem dependency freshized count: `19`
- inventory-only mixed rows outside the direct shell15 dependency subset: shell11/shell12 pair `4`

이 `4`개는 current top-level shell theorem을 막는 blocker가 아니라, first-class inventory transparency를 위해 분리해 둔 별도 row다.

## 6. 현재 신뢰 가능한 결론

현재 신뢰 가능한 결론은 다음 다섯 줄이다.

1. support8 lock은 현재 workspace에서 3-pass 기준으로 current verified다.
2. exact-basis / basis-only theorem trio / family-chain top theorem objects / shell15 frontier pair / shell theorem / tail pattern / tail chain / completion lock은 current-generated 쪽으로 올라왔다.
3. current_runtime_validated_imported_data와 top-level mixed item은 모두 `0`이다.
4. lower-frontier inventory-only shell11/shell12 pair와 family-chain lower imported layers는 여전히 provenance caveat로 남아 있다.
5. 따라서 현재 단계는 “support8 slice reproducible and top-level freshized”이지, “every preserved lower theorem object in the broader archive freshly rederived”는 아니다.

## 7. 다음 액션

이제 가장 현실적인 다음 단계는 둘 중 하나다.

1. lower-frontier inventory-only shell11/shell12 pair `4`개를 freshize할지, 아니면 계속 direct-subset 밖 inventory row로 유지할지 결정한다.
2. family-chain lower triple/quadruple/quintuple/sextuple/septuple imported layers를 추가로 current constructor path로 올릴지 결정한다.

둘 다 support8 lock recovery 이후의 provenance expansion 문제이지, 현재 support8 slice 복구 문제는 아니다.
