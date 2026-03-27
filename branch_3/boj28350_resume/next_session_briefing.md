# BOJ 28350 다음 세션 브리핑

이 문서는 다음 세션에서 바로 작업을 이어갈 수 있도록, 현재까지의 맥락을 한 번에 정리한 메모다. 목적은 네 가지다.

1. 백준 문제 자체를 짧고 정확하게 설명한다.
2. 반례 생성기 zip 파일의 구조와 용도를 설명한다.
3. 현재 코드가 어떤 시행착오를 거쳐 지금 상태에 왔는지 정리한다.
4. 다음 세션에서 무엇을 해야 하는지, 어떤 파일을 베이스로 잡아야 하는지 구체적으로 적는다.

## 1. 백준 문제 설명

문제 번호는 `BOJ 28350`이고, 현재 프로젝트 문서 기준 문제 이름은 `쿼리와 트리 2`다.

핵심은 이미 주어진 트리에서 LCA를 계산하는 문제가 아니라, 반대로 **LCA 제약만으로 rooted tree를 복원하는 문제**라는 점이다.

입력 형식은 다음과 같다.

1. 첫 줄에 정점 수 `N`, 쿼리 수 `M`이 온다.
2. 그다음 `M`개의 줄 또는 정수 묶음으로 `(u, v, w)`가 주어진다.
3. 이 의미는 최종적으로 복원해야 하는 rooted tree에서 `LCA(u, v) = w`여야 한다는 뜻이다.

출력은 길이 `N`의 부모 배열이다.

1. `parent[1] = 0`이어야 한다. 즉 루트는 정점 `1`로 고정된다.
2. `v >= 2`인 모든 정점에 대해 `parent[v]`를 출력한다.
3. 이 부모 배열이 실제 rooted tree를 이루어야 하고, 모든 `(u, v, w)`에 대해 계산된 LCA가 정확히 `w`가 되어야 한다.

현재 프로젝트에서 사용하는 validator도 이 의미를 그대로 검사한다. validator는 다음을 확인한다.

1. 출력 길이가 정확히 `N`인지
2. `parent[1] = 0`인지
3. 나머지 정점이 실제 rooted tree를 이루는지
4. 모든 질의 `(u, v, w)`에 대해 출력 트리의 LCA가 `w`와 일치하는지

즉 이 문제의 본질은 “모든 LCA 제약을 동시에 만족하는 rooted tree를 복원하는 것”이다.

## 2. 반례 생성기 zip 파일 설명

현재 프로젝트에서 핵심 스트레스 스위트로 쓰는 zip 파일은 다음이다.

`lca_tree_stress_v5.zip`

이 zip은 단순 입력 생성기가 아니라, 다음 네 층을 한 번에 포함하는 **검증용 패키지**다.

1. 반례 입력 생성기
2. 출력 validator
3. 성능 리포트 및 breakpoint 탐색 도구
4. preset 기반 인증형 gate 실행기

압축 해제 기준 주요 구성은 아래와 같다.

### 2.1 핵심 파일

1. `gen_case.py`
   각종 hard family를 생성한다.
2. `validator.py`
   출력 parent 배열이 실제 rooted tree인지, 모든 LCA 제약을 만족하는지 다시 검사한다.
3. `find_breakpoint.py`
   timeout 기준으로 버티는 최대 `N`을 찾는다.
4. `bench_report.py`
   CSV 및 Markdown 성능 리포트를 만든다.
5. `certify_suite.py`
   preset 기반 gate를 실행하고 stage별 통과 여부를 정리한다.
6. `hunt_hardest.py`
   현재 solver 기준으로 가장 느린 케이스 조합을 찾는다.

### 2.2 hard family의 의미

이번 프로젝트에서 특히 중요하게 본 hard family는 다음 계열이다.

1. `comb_rect_dense`
   comb 구조에 long-lived rectangular query를 강하게 얹는 family다.
2. `multi_comb_rect`
   multi comb core 위에 cross-depth rectangular query를 얹는 더 강한 family다.
3. `multi_comb_cap`
   `M` cap 근처까지 공격적으로 채우는 family다.
4. `caterpillar_rect_dense`
   caterpillar 구조에서 여러 깊이로 long-lived query를 퍼뜨리는 family다.

이 계열은 공통적으로 다음 성질을 세게 찌른다.

1. 큰 컴포넌트가 쉽게 줄지 않는다.
2. 같은 쿼리 묶음이 여러 deletion 단계에 오래 남는다.
3. decomposition 계열 구현의 “같은 witness, 같은 connector, 같은 dispatch” 재계산을 반복하게 만든다.

즉 이 스위트는 단순 correctness fuzz가 아니라, “약한 데이터에서는 빨라 보여도 hard family에서는 무너지는 구조”를 분명히 드러내기 위한 도구다.

### 2.3 preset과 gate

zip 안의 preset은 대략 아래 역할을 한다.

1. `smoke.json`
   빠른 sanity check
2. `rebuttal_gate.json`
   느린 decomposition 계열에 대한 반박용 gate
3. `strong_gate.json`
   correctness fuzz와 hard scaling을 함께 보는 종합 gate
4. `boj_3s_hard_gate.json`
   adversarial family와 개별 케이스 3초 cap까지 보는 더 빡센 gate

현재까지의 실전 해석은 이렇다.

1. 이 gate를 통과한다고 hidden data 통과가 수학적으로 증명되는 것은 아니다.
2. 하지만 comb, multi_comb, caterpillar 계열 hard family와 max-N dense 케이스를 함께 보기 때문에, 통과하면 백준 실전 통과 가능성에 대한 강한 증거가 된다.

## 3. 현재 코드가 만들어진 히스토리

여기서는 전체 시행착오를 “무엇을 바꾸었고, 왜 그 방향이 유지되거나 버려졌는가” 중심으로 정리한다.

### 3.1 출발점: strongest 기준 파일

가장 오래 기준점 역할을 한 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified.cpp`

이 파일은 문서 기준 strongest single-file solver였다. 여기서는

1. dynamic connectivity
2. topology partition
3. witness handle
4. oracle
5. outer solver

가 한 파일 안에 모두 들어가 있었고, correctness는 비교적 안정적이었다. 하지만 decomposition 쪽 exact rebuild와 witness search가 hard family에서 너무 비쌌다.

### 3.2 owner-class batch pivot

초기 hot path 병목은 per-query strict-child handle이었다. 이를 줄이기 위해 `owner-class batch` 라인으로 pivot했다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_ownerclass_batch.cpp`

핵심 아이디어는 query를 owner-class 단위로 묶고, split-by-smaller 관점에서 batch resolve하는 것이었다. 이 단계에서 per-query rebuild 병목은 상당 부분 사라졌다.

### 3.3 support rebuild BFS 제거와 artifact reuse

그다음 병목은 support tree rebuild BFS였다. 이를 줄이기 위해 topology split BFS artifact를 재사용하는 방향으로 갔다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_ownerclass_artifact_reuse.cpp`

이 단계에서 support rebuild가 별도 그래프 탐색을 다시 돌지 않도록 줄였다.

### 3.4 global delete artifact line

이후 병목이 owner별 zone BFS로 옮겨가자, 삭제 정점마다 old component를 한 번만 훑는 `global delete artifact` 라인이 들어왔다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_global_delete_artifact.cpp`

이 단계는 owner별 exact partition update를 공유 DFS artifact로 공동 처리하려는 시도였다.

### 3.5 watch churn과 live watch 관리

그다음에는 support collector의 중복 parent chain 순회와 stale watch churn이 병목으로 떠올랐다. 그래서

1. skip DSU 기반 collector
2. explicit live watch 관리

를 도입했다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_live_watch_union.cpp`

이 단계에서 stale watch drop 비용은 크게 줄었다.

### 3.6 owner-wide relabel 제거와 class-local refine

그다음 병목은 touched owner마다 owner 전체 partition을 다시 보는 relabel volume이었다. 그래서 touched class만 exact refine하는 `class local refine` 라인으로 갔다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_class_local_refine.cpp`

구조상 owner-wide relabel은 없어졌지만, hard family에서는 touched class coverage가 거의 owner-wide처럼 퍼져 실질 volume은 많이 안 줄었다.

### 3.7 coverage collapse

그 뒤에는 support tree local cut과 representative bucket unanimity를 이용해, touched class 중 실제 split이 없는 class를 exact하게 skip하는 `coverage collapse` 라인이 들어갔다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_coverage_collapse.cpp`

이 단계에서 refine volume 자체는 크게 줄어드는 데 성공했다.

### 3.8 collector-native metadata

coverage collapse는 맞았지만 support metadata construction이 새 병목이 되었다. 그래서 metadata를 별도 graph recovery 없이 collector에서 바로 뽑는 `collector-native metadata` 라인으로 갔다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_collector_native_metadata.cpp`

이 단계에서 metadata build hot path는 크게 줄었다.

### 3.9 support reuse surgery

그다음에는 split이 필요 없는 class에서도 support full rebuild가 많이 남아 있었다. 그래서 skip certified class에 대해 old support를 재사용하거나 tiny connector만 붙이는 `support reuse surgery` 라인이 들어갔다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_support_reuse_surgery.cpp`

이 단계에서 full rebuild count는 진짜 split class 규모까지 줄었다.

### 3.10 shared backbone, owner-local oracle, BC local-surgery는 기각

이후 몇 가지 큰 pivot이 시험되었지만 hard family에서는 다 기각됐다.

1. `shared backbone`
   같은 owner, 같은 bucket을 공유하는 unanimous class끼리 connector를 공유하려 했지만 실제 cluster size가 거의 1이었다.
2. `owner-local exact oracle`
   old component 전체 DFS 대신 owner별 demanded representative exact BFS를 하려 했지만 edge work가 훨씬 더 컸다.
3. `BC local-surgery`
   삭제 정점이 포함된 block만 local surgery로 다시 계산하려 했지만 hard family에서는 locality 이득이 거의 없었다.

이 세 라인은 모두 “아이디어 자체는 그럴듯했지만 현재 hard family에서 cost가 줄지 않는다”는 쪽으로 정리됐다.

### 3.11 preserved piece forest와 touched-again handling

그다음에는 preserved component를 다시 materialize하지 않으려는 `piece forest` 라인이 들어갔다.

대표 파일들은 다음 순서다.

`boj28350_literature_progress7_bcdecomp_verified_preserved_piece_shadow_probe.cpp`  
`boj28350_literature_progress7_bcdecomp_verified_preserved_piece_actual.cpp`  
`boj28350_literature_progress7_bcdecomp_verified_piece_native_touched_again.cpp`

여기서 shadow는 매우 유망했지만, actual semantics에서는 touched-again class가 다시 fallback materialization으로 빠져 성능이 망가졌다.

### 3.12 connector delta debugging과 rollback recovery

그다음에는 preserved는 고정하고 connector-piece-hit를 actual delta로 처리하려 했지만 correctness가 깨졌다. 그래서

1. `connector delta actual`
2. `debug isolation`
3. `preserved rollback recovery`
4. `connector rollback recovery`

순으로 가면서 preserved correctness를 먼저 회복하고, connector는 rollback으로 correctness를 되찾았다.

이 단계의 핵심 파일은 아래다.

`boj28350_literature_progress7_bcdecomp_verified_connector_delta_actual.cpp`  
`boj28350_literature_progress7_bcdecomp_verified_connector_delta_debug_isolation.cpp`  
`boj28350_literature_progress7_bcdecomp_verified_preserved_rollback_recovery.cpp`  
`boj28350_literature_progress7_bcdecomp_verified_connector_rollback_recovery.cpp`

### 3.13 connector skeleton rebuild correctness 회복

connector delta를 더 복잡하게 고치기보다, connector state를 `현재 살아 있는 preserved pieces의 terminal에서 다시 만드는 single skeleton`으로 단순화하는 방향이 들어갔다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_connector_skeleton_rebuild.cpp`

이 단계에서 correctness는 다시 회복됐다. 다만 main path activation과 비용 분해가 추가 과제로 남았다.

### 3.14 route activation audit과 residual cost attribution

그다음 단계에서는 skeleton path가 실제로 선택되는지 확인했고, 실제로 선택된다는 점이 확인됐다. 이후 residual cost를 explicit component 기준으로 분해한 결과, 현재 지배축이

`connector skeleton build + watch churn`

이라는 점이 드러났다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_connector_route_activation_audit.cpp`  
`boj28350_literature_progress7_bcdecomp_verified_residual_cost_attribution.cpp`

### 3.15 watch diff update와 현재 상태

가장 최근 흐름은 old와 new skeleton의 공통 vertex를 이용해, 같은 vertex에 대한 watch handle을 재사용하는 `watch diff update` 라인이다.

대표 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_watch_diff_update.cpp`  
`boj28350_literature_progress7_bcdecomp_verified_watch_diff_profiled.cpp`  
`boj28350_literature_progress7_bcdecomp_verified_watch_diff_profiled_sampled.cpp`

완료된 케이스 기준으로는 다음이 분명하다.

1. old, new skeleton overlap이 매우 높다. 대체로 96퍼센트에서 98퍼센트 수준이다.
2. full watch churn 대비 diff churn은 약 3퍼센트에서 4퍼센트 수준까지 내려간다.
3. correctness는 완료된 케이스에서는 유지된다.

다만 이번 세션 기준으로는 아래 두 케이스를 끝까지 안정적으로 회수하지 못했다.

`connector_only, comb_rect_dense 512 LOCAL`  
`both_on, comb_rect_dense 512 LOCAL`

즉 현재 단계의 결론은

`watch diff update는 유망하다. 하지만 512 LOCAL과 1024 release를 끝까지 회수할 수 있는 profiling 체계를 먼저 갖춰야 한다.`

이다.

## 4. 다음 작업 설명

다음 세션에서 가장 추천하는 시작 베이스 파일은 다음이다.

`boj28350_literature_progress7_bcdecomp_verified_watch_diff_profiled_sampled.cpp`

이 파일을 베이스로 잡는 이유는 다음과 같다.

1. correctness가 맞는 현재 route를 유지한다.
2. watch diff update가 이미 들어가 있다.
3. sampled profiling과 top K deletion logging을 넣기에 적절한 위치다.
4. 현재 막힌 문제는 semantics가 아니라 “512 LOCAL과 1024 release를 끝까지 회수할 수 있는가”이기 때문이다.

### 4.1 다음 세션의 목표

다음 세션의 목표는 딱 두 가지다.

1. `connector_only, comb_rect_dense 512 LOCAL`과 `both_on, comb_rect_dense 512 LOCAL`을 끝까지 회수할 수 있는 profiling 체계를 만든다.
2. 남은 시간을 다음 하위 축 중 어디가 가장 크게 먹는지 확정한다.

`connector skeleton build core`  
`watch churn`  
`dispatch umbrella 내부`  
`global_delete_dfs`  
`preserved piece split`  
`query incident scans`

### 4.2 다음 세션에서 바로 할 일

1. correctness와 route를 다시 잠근다.
   `connector_only, comb_rect_dense 256 LOCAL`  
   `both_on, comb_rect_dense 256 LOCAL`  
   `both_on, multi_comb_rect 512 LOCAL`  
   에서 validator와 LOCAL invariant가 유지되는지 먼저 확인한다.

2. profiling 모드를 분리한다.
   `PROFILE_NONE`  
   `PROFILE_BASE`  
   `PROFILE_SAMPLED`  
   세 모드로 나눠서, `512 LOCAL`은 먼저 `PROFILE_BASE`로 완주하고 그다음 `PROFILE_SAMPLED`로 세부 cost를 본다.

3. progress checkpoint와 flush를 넣는다.
   case 시작  
   initialization 완료  
   deletion step N개마다  
   종료 직전 summary  
   에서 stderr를 line buffered로 찍고 flush한다. 목적은 out과 time 파일이 비는 문제를 막는 것이다.

4. `time_route_dispatch_ns`를 더 잘게 분해한다.
   다음 다섯 축은 최소로 필요하다.

   1. `time_unanimous_mode_dispatch_ns`
   2. `time_terminal_collection_ns`
   3. `time_vertex_lookup_ns`
   4. `time_watch_diff_build_ns`
   5. `time_state_publish_ns`

5. `time_connector_skeleton_build_ns`도 더 잘게 분해한다.
   다음 다섯 축은 최소로 필요하다.

   1. `time_connector_skeleton_terminal_collection_ns`
   2. `time_connector_skeleton_terminal_dedupe_ns`
   3. `time_connector_skeleton_vertexset_build_ns`
   4. `time_connector_skeleton_vertex_lookup_build_ns`
   5. `time_connector_skeleton_core_build_ns`

6. top K slow deletion profiler를 유지한다.
   적어도 가장 느린 10개 deletion에 대해 다음을 남긴다.

   1. deletion index
   2. deleted vertex `x`
   3. touched class count
   4. connector skeleton terminals
   5. connector skeleton vertices
   6. connector watch removed
   7. connector watch added
   8. preserved piece split vertices
   9. global delete dfs edges
   10. query incident scans
   11. total deletion time

### 4.3 다음 세션의 판정 규칙

다음 세션에서 최종적으로 보고 싶은 판정은 아래 중 하나다.

1. `connector skeleton build core`가 절반 이상 크다.  
   그러면 다음 pivot은 connector skeleton state reduction이다.

2. `watch churn`이 절반 이상 크다.  
   그러면 다음 pivot은 더 강한 watch compression이다.

3. `dispatch umbrella 내부`가 절반 이상 크다.  
   그러면 다음 pivot은 dispatch 내부 최적화다.

4. `global_delete_dfs`가 다시 dominant로 올라온다.  
   그러면 다음 pivot은 더 근본적인 connectivity maintenance 축이다.

## 5. 다음 세션에서 바로 열어둘 파일

다음 세션을 열면 아래 파일들을 같이 열어두는 것이 좋다.

1. 현재 작업 베이스 파일  
   `boj28350_literature_progress7_bcdecomp_verified_watch_diff_profiled_sampled.cpp`

2. 문제 전체 맥락과 긴 히스토리  
   `boj28350_integrated_technical_history.md`

3. strongest 정리 패키지  
   `literature_grade_proof_package.md`

4. 스트레스 스위트 설명  
   `lca_tree_stress_v5/README.md` 또는 zip 내부 README

5. 최근 측정 결과  
   `watch_diff_update_report.md`  
   `watch_diff_update_results.csv`  
   `residual_cost_attribution_report.md`  
   `residual_cost_attribution_results.csv`  
   `residual_cost_topk.csv`

## 6. branch_3 pre-rewrite checkpoint (2026-03-25)

다음 solver rewrite 또는 pivot 전에, 이번 branch_3 재개 세션에서 아래 두 source set을 다시 읽고 기준을 잠갔다. 이 항목을 branch_3의 pre-rewrite checkpoint 기록이자, 실제 solver-side major rewrite/pivot 시작 여부를 결정하는 decision checkpoint로 사용한다.

### 6.0 decision gate before any major solver rewrite or pivot begins

다음 major solver rewrite 또는 pivot은 아래 두 review completion이 모두 명시적으로 확인되기 전에는 시작하지 않는다.

1. `reviewed source set A`로 정의한 branch_3 notes / working set review 완료
2. `reviewed source set B`로 정의한 bundled progress40 authoritative materials review 완료
3. solver rewrite planning note 또는 retry note에 위 두 review completion이 모두 확인됐다는 문장을 남긴 뒤에만 solver-side rewrite/pivot을 연다

둘 중 하나라도 미완료거나 기록이 빠져 있으면, 다음 solver rewrite/pivot은 보류하고 먼저 review state를 갱신한다.

### 6.1 reviewed source set A: branch_3 working set

검토한 파일은 아래다.

`boj28350_resume/README.md`  
`boj28350_resume/current_state_summary.md`  
`boj28350_resume/next_session_briefing.md`  
`boj28350_complete_master_document_partA_raw.md`  
`boj28350_integrated_technical_history.md`  
`boj28350_literature_progress7_bcdecomp_report.md`  
`literature_grade_proof_package.md`  
`boj28350_resume/boj28350_branch_3_solver.cpp`

여기서 다시 잠근 핵심은 세 가지다.

1. `progress7` 검증 리포트, 통합 히스토리, proof package는 branch_3가 원래 어떤 literature-grade invariant 위에서 발전했는지를 다시 고정한다. 핵심은 BC-tree flavored decomposition, explicit child lattice, minimal closed subtree handle, release-path exact rebuild 제거 라인이다.
2. branch-local resume note는 active solver가 progress40 snapshot에서 복사된 파일이라고 적고 있다. 하지만 현재 `boj28350_branch_3_solver.cpp` 머리 주석과 실제 구현은 `static separator decomposition` 기반 branch-local rewrite로 바뀌어 있고, progress40 계열의 layout-signature, zero-span, fastpath instrumentation line과 직접 이어지지 않는다.
3. 따라서 branch_3 active solver는 지금 시점에서 progress40-derived line에서 drift가 발생한 상태로 봐야 한다. 다음 rewrite는 현재 drift 상태를 더 밀어붙이는 것이 아니라, 위 문헌/증명 anchor를 보존한 채 progress40 bundle 기준으로 다시 맞춰서 진행해야 한다.

### 6.2 reviewed source set B: bundled progress40 authoritative set

검토한 파일은 아래다.

`boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`  
`boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`  
`boj28350_bundle_archive/boj28350_progress40_results_merged.json`

이 source set에서 다음을 다시 확인했다.

1. progress40의 핵심 방향은 layout-signature reuse gate를 넣은 뒤, authoritative sampled aggregate로 남은 residual을 분해하는 것이다.
2. 현재 가장 안전한 다음 pivot은 여전히 `zero-span eligibility and fastpath commit`이다.
3. `layout signature compare and reuse gate core` 자체는 이미 한 차례 줄인 뒤라, 다음 rewrite가 여기서 완전히 다른 알고리즘 family로 튀는 근거가 되지 않는다.
4. 남아 있는 미완료 범위는 dense 1024 release/repeat, 4096 representative, long-run terminal row persistence close다. 따라서 solver-side 최적화와 별개로 branch-local reproducibility hygiene도 계속 보존해야 한다.

### 6.2.1 explicit review completion evidence before the next rewrite opens

이 항목이 6.0의 3번 조건을 만족시키는 branch_3 research-trail evidence다.

`2026-03-26` 기준, 다음 major solver rewrite 또는 pivot이 열리기 전에 아래 두 review completion이 모두 끝났음을 명시적으로 기록한다.

1. `reviewed source set A`인 branch_3 notes / working set review는 6.1에 적은 파일 재검토와 핵심 잠금으로 완료됐다.
2. `reviewed source set B`인 bundled progress40 authoritative materials review는 6.2에 적은 파일 재검토와 pivot/residual 재확인으로 완료됐다.
3. 따라서 다음 solver-side rewrite/pivot은 이 completion evidence가 남겨진 이후에만 열 수 있고, 이후 retry/rewrite note는 6.1, 6.2, 6.2.1을 precondition record로 인용해야 한다.

### 6.2.2 dated checkpoint refresh for branch_3 research log

`2026-03-26` pre-rewrite checkpoint:
branch_3 notes / working-set review와 bundled progress40 authoritative materials review가 모두 완료됐음을 다시 기록한다.
따라서 다음 major solver rewrite 또는 pivot은 이 두 review completion이 research log에 남은 상태에서만 시작한다.

`2026-03-26` explicit source-set confirmation for Sub-AC 3:
1. source set A로 잠근 `branch_3` working set review는 `boj28350_resume/README.md`, `boj28350_resume/current_state_summary.md`, `boj28350_resume/next_session_briefing.md`, `boj28350_complete_master_document_partA_raw.md`, `boj28350_integrated_technical_history.md`, `boj28350_literature_progress7_bcdecomp_report.md`, `literature_grade_proof_package.md`, `boj28350_resume/boj28350_branch_3_solver.cpp` 재검토로 완료됐다.
2. source set B로 잠근 bundled `progress40` authoritative review는 `boj28350_bundle_archive/boj28350_literature_progress40_layout_signature_reuse_gate.cpp`, `boj28350_bundle_archive/boj28350_progress40_layout_signature_reuse_gate_report.md`, `boj28350_bundle_archive/boj28350_progress40_results_merged.json` 재검토로 완료됐다.
3. 이 두 source set review completion이 planning note에 다시 확인된 이후에만 다음 solver rewrite 또는 pivot을 연다.

### 6.3 rewrite rule that governs the next move

다음 rewrite 또는 pivot은 아래 규칙을 따른다.

1. active solver를 현재 separator decomposition line에서 더 밀어붙이지 않는다. 이 라인은 progress40 research direction과 맞지 않는다.
2. 다음 solver-side 작업은 progress40 source line으로 다시 anchor를 맞춘 뒤 진행한다. 필요하면 bundled progress40 source를 기준으로 branch_3 active solver를 되돌려 놓고, 그 위에서 최적화를 이어간다.
3. solver-side 첫 타깃은 progress40 report가 지목한 `zero-span eligibility and fastpath commit` 잔여 비용이어야 한다.
4. gate failure가 algorithm보다 execution-layer close 문제로 보이면 wrapper, artifact path, finalize 흐름을 branch-local 범위에서 보강하되 gate 의미 자체는 건드리지 않는다.
5. 다음 rewrite의 성공 기준은 “progress40-derived line 유지 + branch-local reproducibility 유지 + lca_tree_stress_v5 gate signal 개선”이다.

### 6.4 rewrite-planning note: solver constraints, prior hypotheses, known failure modes

다음 solver rewrite planning에서 바로 참조할 branch-local 요약은 아래다.

#### solver constraints

1. active solver의 기준 anchor는 여전히 bundled `progress40` line이다. `boj28350_resume/README.md`는 active solver가 progress40 snapshot에서 복사됐다고 적고 있고, 6.1/6.2 재검토 결과도 다음 rewrite를 progress40 source line으로 다시 맞추라고 요구한다.
2. solver-side 구조 제약은 literature-grade invariant를 보존하는 것이다. `progress7` report, integrated history, proof package가 다시 잠그는 핵심은 `BC-tree flavored explicit child lattice`, `buildClosedHandleFromWitness(...)`에 의한 minimal closed subtree handle, `owner exact rebuild 제거`, `strict-child relocation`의 explicit child-only path다.
3. branch_3에서 허용되는 다음 최적화는 progress40 residual 축 안에서의 축소여야 한다. 다른 알고리즘 family로 튀는 separator-decomposition branch-local rewrite나 heuristic-only line은 progress40-derived research direction을 깨므로 금지 축으로 본다.
4. solver 최적화와 별개로 branch-local reproducibility hygiene를 유지해야 한다. progress40 report와 current state summary 모두 dense 1024/repeat, 4096 representative, long-run terminal row persistence close가 아직 미완료라고 적고 있으므로 wrapper/finalize/artifact hygiene는 계속 solver planning에 포함된다.

#### prior hypotheses carried into the rewrite

1. authoritative한 현재 1차 pivot은 `zero-span eligibility and fastpath commit`이다. `current_state_summary.md`와 bundled progress40 report가 둘 다 가장 안전한 다음 축으로 이를 지목한다.
2. `state materialization`과 `layout gate`는 보조 residual 축이다. progress40 direct aggregate에서 `signature source load and materialize`, `layout signature compare and reuse gate core`가 각각 약 25퍼센트 share로 남아 있어 zero-span 최적화 이후에도 재점검 대상이다.
3. strong gate failure는 먼저 correctness/proof-preservation lane으로 읽어야 하고, boj3s gate failure는 먼저 performance/profile lane으로 읽어야 한다. latest failure breakdown도 AC 4를 `correctness-proof`, AC 5를 `performance-profile`로 분류하고 fallback progress40 axis를 이 기준으로 붙였다.
4. 현재 retry-loop analysis state는 `pinned_primary_axis`와 `pinned_secondary_axis`가 비어 있다. 따라서 다음 rewrite planning에서는 stale한 broad rewrite 대신 current summary 기반 fallback axis(`zero_span_fastpath`, `state_materialization`, `layout_gate`)를 임시 기준으로 삼고, analysis asset refresh가 회복되면 그때 다시 pinning을 갱신해야 한다.

#### known failure modes to plan against

1. branch drift가 이미 확인됐다. 현재 `boj28350_branch_3_solver.cpp`는 progress40 snapshot이라고 적힌 resume note와 달리 separator decomposition 기반 branch-local rewrite로 drift해 있으므로, 현 solver를 그냥 미세 튜닝하는 접근은 연구 방향 자체를 더 흐릴 위험이 있다.
2. execution-layer close failure가 아직 남아 있다. progress40 report와 current state summary는 short case/512 sampled one-off는 닫혔지만 dense 1024 release/repeat, 4096 representative, long-run terminal persistence는 authoritative close가 아니라고 적는다.
3. latest formal gate evidence는 strong gate reproducibility와 boj3s final gate 모두 미달이다. latest failure report 기준 `./lca_boj3s_gate.sh`는 correctness smoke 64 fail, hard scaling strict 99 fail, large adversarial 30 fail, large mix 18 fail로 남아 있고, repeated strong gate PASS는 stall/no-activity 때문에 formal closure가 성립하지 않았다.
4. nominal strong-gate PASS도 그대로 신뢰하면 안 된다. latest attempt guard는 기존 AC 3 PASS를 `suspicious_strong_gate_pass`로 표시했고, 최신 analysis session은 mandatory analysis asset refresh 실패 때문에 다음 retry를 막았다.
5. repo-health signal도 약하다. latest git repo health에서 `git status`와 `git fsck`가 timeout이라, 다음 rewrite planning과 retry logging은 git cleanliness보다 branch-local artifact evidence를 우선 근거로 남겨야 한다.

## 마지막 한 줄

현재 가장 타당한 next move는 **branch_3 active solver를 progress40-derived line으로 다시 anchor한 뒤, `zero-span eligibility and fastpath commit` 잔여 비용과 long-run close reproducibility를 함께 줄이는 쪽으로 rewrite를 이어가는 것**이다.
