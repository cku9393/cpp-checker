# Current Workspace Reality Check

## current baseline

- code baseline: `branch_4/90/full_dynamic_top_tree_engine_90.cpp`
- archival bundle root: `branch_4/90`
- current runtime root: `branch_4/90/runtime`

## code identity and metadata

- header states: `This is NOT the complete BOJ solver`
- non-LOCAL_TEST `main()`: dummy `return 0;`
- current bundle metadata: version `90`, source `branch_4/90/full_dynamic_top_tree_engine_90.cpp`
- imported provenance catalog retained in code: family-chain `57`, lower frontier ladder `67/69/70/71/72/74/75/76/77/79`, archival shell15 frontier source `84`
- retained provenance path example: `/mnt/data/full_dynamic_top_tree_engine_84.cpp`

## required docs / artifacts

- required docs count from code: `39`
- current nonempty required docs: `39`
- current document audit fingerprint: `39|39|1|1|1|1|1`
- required artifacts count from code: `8`
- current nonempty required artifacts: `8`
- current artifact audit fingerprint: `8|8|8|1|1`

## compile / rerun

- release compile: verified
- LOCAL_TEST compile: verified
- LOCAL_TEST pass1: `support8_authoritative_completion_locked`
- LOCAL_TEST pass2: `support8_authoritative_completion_locked`
- LOCAL_TEST pass3: `support8_authoritative_completion_locked`
- current rerun audit fingerprint: `1|1|1|1|1|84:9663593306329234703|16856:1011248525676628134`

## top-level provenance inventory

- inventory item count: `19`
- current verified count: `16`
- archival claim count: `3`
- fresh current runtime generated count: `16`
- current runtime validated imported data count: `0`
- mixed count: `0`
- archival only count: `3`
- provenance audit fingerprint: `1808:4932380748279645466`

## lower-frontier first-class inventory

- lower-frontier inventory row count: `23`
- direct shell-theorem dependency subset count: `19`
- direct shell-theorem dependency freshized count: `19`
- inventory-only mixed rows outside the direct shell15 dependency subset:
  - `antecedent_plus_eight_frontier`
  - `support8_antecedent11_frontier`
  - `antecedent_plus_nine_frontier`
  - `support8_antecedent12_frontier`

## current classifications

- current reproducible classification: `support8_authoritative_completion_locked`
- archival 90 classification: `support8_authoritative_completion_locked`
- lock achieved in current workspace: `yes`

## current verified representative items

- fresh current runtime generated:
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
- current runtime validated imported data:
  - none
- mixed:
  - none at top-level inventory

## remaining caveat

현재 가장 큰 caveat은 top-level support8 theorem item이 아니라, lower-frontier first-class inventory에 별도로 노출한 shell11/shell12 pair `4`개와 family-chain lower imported layers다.  
즉 현재 support8 slice는 locked and top-level freshized지만, broader frontier inventory와 deeper family-chain provenance 범위는 계속 분리해서 읽어야 한다.
