# Progress40 Bundled Source Summary Note

이 메모는 bundled `progress40` line에서 branch_3가 바로 재사용해야 할 solver 아이디어, invariant, stress-test expectation만 따로 분리한 source summary다. 목적은 다음 세 가지다.

1. progress40이 실제로 추가한 최적화 축을 재확인한다.
2. branch_3 solver retry가 progress40-derived line 밖으로 drift하지 않게 막는다.
3. `lca_tree_stress_v5` 해석에서 어떤 family와 residual을 먼저 봐야 하는지 분명히 남긴다.

## Source provenance

- primary bundled source snapshot:
  `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`
- working-tree mirror cross-check:
  `boj28350_literature_progress40_layout_signature_reuse_gate.cpp`
- bundled report/results mirror used for extracted expectations:
  `boj28350_resume/progress40_derived_reference.md`
  `boj28350_resume/current_state_summary.md`
  `boj28350_resume/next_session_briefing.md`
- intended bundled source set:
  `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`
  `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`
  `boj28350_bundle_archive/boj28350_progress40_results_merged.json`

현재 checkout에서는 archive 안의 markdown/json 요약물 일부가 `dataless` placeholder 상태라 직접 본문을 안정적으로 읽을 수 없었다. 아래 요약은 local progress40 source와, 그 bundled report/results를 이미 인용해 둔 hydrated branch-local memo들을 교차 확인해서 정리했다.

## Reusable solver ideas

1. progress40은 새 solver family가 아니라 `progress39` 위에 `layout signature reuse gate`를 얹은 라운드다.
   `boj28350_resume/progress40_derived_reference.md`와 `boj28350_resume/current_state_summary.md`는 둘 다 이 라인을 authoritative baseline으로 둔다.
2. source상 최적화는 독립 heuristic 모음이 아니라 누적 gate stack이다.
   `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp:7584-7618`은
   `tscan` -> `state load` -> `prev_state_carry_reuse` -> `carry_reuse_fastpath` ->
   `slot owner patch/update chain` -> `pack_encode_normalize` ->
   `canonical_normalize` -> `layout_reuse_zero_elision` ->
   `layout_signature_gate`
   순으로 metric/enable chain을 걸어 둔다.
3. progress40이 직접 노출한 layout-gate subaxis는 일곱 개다.
   `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp:16727-16786`는
   `sig_source_load`, `sig_materialize`, `sig_compare_core`,
   `same_layout_gate`, `zero_span_eligibility_gate`,
   `fastpath_commit_core`, `connector_hotpath_reuse`
   시간을 따로 출력한다.
4. route-aware attribution을 보존해야 한다.
   source는 `baseline`, `delta_preserved_then_skeleton`,
   `connector_skeleton`, `general_delta` route별로 reuse/wscan/sload/pcarry 등의
   dominant route tag를 기록한다.
   관련 집계 hook은
   `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp:3385-3400`
   과
   `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp:16760-16786`
   에서 바로 확인된다.
5. watch churn과 query incident scan은 여전히 main pipeline의 관찰 대상이다.
   progress40 line도 `queryIncidentScans`, `timeQueryIncidentScanNs`,
   `timeWscanTotalNs`, `wscanDuplicateFullScanPasses`를 계속 남긴다.
   즉 layout signature gate가 들어와도 old hotspot attribution을 버린 것이 아니다.

## Invariants and guardrails

1. `layout signature gate`는 canonical normalize와 same-layout reuse 위에서만 해석해야 한다.
   `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp:7612-7618`에서
   `__cnorm_metric -> __lreuse_metric -> __lgate_metric` 순서가 고정돼 있다.
   branch_3 후속 수정은 이 chain을 끊거나, layout gate를 독립 heuristic처럼 떼어내면 안 된다.
2. fastpath commit은 gate 통과 뒤에만 의미가 있다.
   `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp:8818-8852`
   와
   `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp:16727-16786`는
   `same_layout_gate`, `zero_span_eligibility_gate`, `fastpath_commit_core`
   를 분리해서 찍는다. 따라서 후속 최적화는 gate semantics를 약화하는 대신,
   load/materialize/compare와 zero-span commit residual 자체를 줄여야 한다.
3. progress40 line은 proof-preserving rewrite 축 안에서만 이어가야 한다.
   `boj28350_resume/current_state_summary.md`와
   `boj28350_resume/next_session_briefing.md`는 둘 다
   separator-decomposition drift 확대를 금지하고,
   active solver를 bundled progress40 line으로 다시 anchor하라고 적는다.
4. low-share connector hotpath를 1차 축으로 오해하면 안 된다.
   `boj28350_resume/progress40_derived_reference.md`와
   `boj28350_resume/current_state_summary.md`는
   `connector hotpath normalize reuse = 0.0036%`라서 primary target이 아니라고 고정한다.
5. bundled evidence는 `partial` authoritative 상태다.
   따라서 bundle 안의 일부 gate row는 연구 방향을 고정하는 근거일 뿐,
   branch-local `./lca_strong_gate.sh`나 `./lca_boj3s_gate.sh`를 대체하지 않는다.

## Stress-test expectations for `lca_tree_stress_v5`

1. 대표 hard family는 계속 아래 네 개다.
   `comb_rect_dense`
   `multi_comb_rect`
   `multi_comb_cap`
   `caterpillar_rect_dense`
2. 이 family는 큰 component가 잘 줄지 않고, 같은 query 묶음이 여러 deletion 단계에 오래 남는다.
   그래서 같은 witness/connector/dispatch 재계산과 watch churn을 반복 강요한다.
   progress40 이후 gate failure도 먼저 이 구조적 pressure를 기준으로 읽어야 한다.
3. bundled progress40 aggregate가 남긴 residual ordering은 아래다.
   `zero-span eligibility and fastpath commit` `0.758605ms`, `49.9983%`
   `layout signature compare and reuse gate core` `0.379830ms`, `25.0339%`
   `signature source load and materialize` `0.378774ms`, `24.9643%`
   `connector hotpath normalize reuse` `0.000054ms`, `0.0036%`
4. 따라서 progress40 이후 branch_3의 가장 안전한 next pivot은 한 줄로 고정된다.
   `next pivot after layout-gate round: zero-span eligibility and fastpath commit`
5. branch-local benchmark expectation은 light random보다 dense representative와 hard family 우선 해석이어야 한다.
   strong gate failure는 correctness/proof preservation lane으로 먼저 읽고,
   boj3s gate failure는 위 residual ordering과 hard family pressure를 먼저 붙여 읽어야 한다.

## Reproducibility and acceptance expectations

1. progress40 package가 실제로 닫은 것은 execution-layer validation,
   small/medium gate representatives, LOCAL 512 matrix, 그리고
   `both_on_multi_1024_release` 1회까지다.
2. 아직 미완료로 남은 범위는 dense `1024` repeat, dense `4096`, multi `4096`,
   long-run terminal row persistence close다.
3. 따라서 branch_3에서 stronger claim을 하려면 fresh same-worktree rerun evidence가 필요하다.
   bundled success row를 carried-forward PASS처럼 쓰면 안 된다.

## Practical branch_3 use

1. solver-side 변경은 먼저 progress40 line의 누적 gate stack을 보존하는지 확인한다.
2. profiling과 retry 해석은 `zero-span eligibility and fastpath commit`을 1차 축,
   `signature source load and materialize`와
   `layout signature compare and reuse gate core`를 2차 축으로 둔다.
3. wrapper/retry-loop 관점에서는 hard family, dense representative, long-run persistence를
   우선 증거로 삼고, bundled partial close를 formal branch-local closure로 승격하지 않는다.
