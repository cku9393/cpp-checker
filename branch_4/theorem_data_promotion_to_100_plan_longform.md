# theorem-data 승격 완성도를 100점으로 만들기 위한 계획 문서

## 문서 목적

theorem-data promotion 관점에서 이번 기준점의 핵심 성과는 네 줄로 정리된다.

- support8 lock은 current runtime에서 유지된다.
- current bundle metadata와 imported provenance metadata는 코드 / TSV / markdown report에서 분리된다.
- top-level current verified theorem-data item은 모두 `fresh_current_runtime_generated`다.
- lower-frontier inventory-only row의 처리 방식은 `keep_inventory_only_nonblocking`으로 확정했다.
- family-chain lower layer `7`개는 모두 current constructor/cache-backed path로 승격했다.
- family-chain lower-layer imported caveat는 `7/7` fresh, imported `0`으로 닫혔다.

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

1. lower-frontier first-class inventory에는 shell11/shell12 pair `4`개가 direct shell15 dependency subset 밖 `keep_inventory_only_nonblocking` row로 남아 있다.
2. family-chain lower layer `7`개는 freshized됐고, remaining imported lower-layer row는 없다.
3. archival only `3`개 item은 preserved evidence로 남고 current theorem validation 결과로 승격되지 않는다.

즉 지금 남은 문제는 “top-level mixed item이 남아 있다”가 아니라 “broader theorem-data perimeter를 어디까지 fresh path로 넓힐 것인가”다.

## 4. 앞으로의 theorem-data promotion 계획

### 단계 1. lower-frontier perimeter 정리

lower-frontier inventory-only shell11/shell12 pair `4`개는 계속 분리해 두는 것으로 결정했다. 이들은 visible nonblocking perimeter row이며 top-level support8 blocker가 아니다.

### 단계 2. family-chain lower layers 정리

`pair_expansion_aggregate_52`는 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_pair_expansion_aggregate_52_from_bounded_region_scan_`
- pass2/pass3 cache loader: `load_current_pair_expansion_aggregate_52_runtime_artifact_`
- payload fingerprint: `6003:2005080337376028436`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`

`triple_family_expansion_theorem_data_53`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_triple_family_expansion_theorem_data_53_from_pair52_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_triple_family_expansion_theorem_data_53_runtime_artifact_`
- payload fingerprint: `11888:3562593626991170520`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream pair52 fallback hit: `0`

`quadruple_family_expansion_theorem_data_55`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_quadruple_family_expansion_theorem_data_55_from_triple53_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_quadruple_family_expansion_theorem_data_55_runtime_artifact_`
- payload fingerprint: `15637:3406948456738223960`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

`quintuple_family_expansion_theorem_data_57`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_quintuple_family_expansion_theorem_data_57_from_quad55_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_quintuple_family_expansion_theorem_data_57_runtime_artifact_`
- payload fingerprint: `11519:14985224666762482157`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream quad55 fallback hit: `0`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

`sextuple_family_expansion_theorem_data_57`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_sextuple_family_expansion_theorem_data_57_from_quintuple57_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_sextuple_family_expansion_theorem_data_57_runtime_artifact_`
- payload fingerprint: `4567:5441664472856347648`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream quintuple57 fallback hit: `0`
- upstream quad55 fallback hit: `0`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

`septuple_family_expansion_theorem_data_57`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_septuple_family_expansion_theorem_data_57_from_sextuple57_ready_family_scan_`
- pass2/pass3 cache loader: `load_current_septuple_family_expansion_theorem_data_57_runtime_artifact_`
- payload fingerprint: `804:10183455833117365445`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream sextuple57 fallback hit: `0`
- upstream quintuple57 fallback hit: `0`
- upstream quad55 fallback hit: `0`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

`high_family_expansion_theorem_data_57`도 다음 경로로 승격 완료됐다.

- pass1 constructor: `build_current_high_family_expansion_theorem_data_57_from_septuple57_ready_aggregate_scan_`
- pass2/pass3 cache loader: `load_current_high_family_expansion_theorem_data_57_runtime_artifact_`
- payload fingerprint: `317:16323892766005099572`
- imported equality result: `counts_and_consumer_visible_fingerprint_equal`
- upstream septuple57 fallback hit: `0`
- upstream sextuple57 fallback hit: `0`
- upstream quintuple57 fallback hit: `0`
- upstream quad55 fallback hit: `0`
- upstream triple53 fallback hit: `0`
- upstream pair52 fallback hit: `0`

다음 family-chain lower-layer target은 `none_family_chain_lower_layers_complete`이다.

### 단계 2-1. next theorem-data scope decision

Theorem-data promotion 자체는 current support8 top-level과 family-chain lower-layer 축에서 닫혔다. 다음 scope는 `general_gap_bridge_formalization`으로 둔다.

- shell16은 `preflight_contract_ready_no_scan`
- higher-support는 `higher_support_deferred_after_contract_equivalent_status_congruence_open`
- broader general gap theorem은 `ready_for_bridge_formalization`
- BOJ solver bridge는 `ready_for_problem_bridge_formalization`
- archive cleanup은 `worth_cleaning_later`

Bridge formalization 이후 proof-obligation 관점의 `prove_minimal_counterexample_reduction`은 proof-ready skeleton까지 진행됐고, limited bridge theorem은 current scope에서 증명됐다. support-bound/support-reduction/family-chain rounds now keep operation routes through `family_chain_absorption_reduction`. The current `status_preservation_congruence_bridge` formalizes the common status language and operation table, classifying preserved/reduced/refuted/absorbed/escaped outcomes while leaving operation-specific status proofs and residual absorption measure open. The higher-support recheck did not run support9+, the project-to-active status round made active projection status proof-ready with inactive-support status locality still open, and the coordinate-contraction status round made quotient status proof-ready with equivalent-coordinate status congruence open. Full theorem-data/general promotion은 아니다. 다음 target은 `canonical_compression_status_preservation`다.

### 단계 3. archival only 보존

archival only 항목은 삭제 대상이 아니다.  
이들은 preserved evidence로 남되, current verified와 섞이지 않도록 유지해야 한다.
