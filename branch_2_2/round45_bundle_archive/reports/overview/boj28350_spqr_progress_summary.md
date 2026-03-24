# BOJ 28350 진행 요약 문서

이 문서는 새 세션으로 넘어갈 때 바로 이어서 작업할 수 있도록, 현재까지의 상태를 한 번에 정리한 handoff 문서다.

## 1. 백준 문제 설명

문제는 **BOJ 28350 - 쿼리와 트리 2**다. 현재 코드와 스트레스 하네스가 다루는 해석은 아래와 같다.

- 정점이 `1..N`인 **루트가 1번인 트리**를 출력해야 한다.
- 입력은 `M`개의 쿼리 `(u, v, w)` 이고, 출력한 트리에서 모든 쿼리에 대해 `LCA(u, v) = w`가 성립해야 한다.
- 출력 형식은 길이 `N`의 부모 배열이며, `parent[1] = 0`, 나머지 정점은 `1..N` 범위의 부모를 가져야 한다.
- 채점은 **스페셜 저지**이고, 트리가 올바른 rooted tree인지와 모든 쿼리를 만족하는지를 검증한다.

이 문서의 설명은 **문제 전체를 그대로 복붙한 것이 아니라**, 현재 솔버/검증기/스트레스 하네스가 실제로 사용하고 있는 의미를 기준으로 재구성한 요약이다.

### 입력/출력 의미 요약

입력:

```text
N M
u1 v1 w1
u2 v2 w2
...
uM vM wM
```

출력:

```text
p1 p2 ... pN
```

여기서:
- `p1 = 0`
- `p_i` (`i >= 2`)는 정점 `i`의 부모
- 출력된 트리는 루트가 1인 유효한 트리여야 하며, 모든 쿼리에 대해 LCA 조건을 만족해야 함

### 스트레스 하네스가 검증하는 것

현재 사용 중인 `lca_tree_stress_v2` 패키지는 다음을 수행한다.

1. 숨은 트리를 먼저 생성
2. 그 트리에서 **항상 유효한** LCA 쿼리를 생성
3. 솔버 출력 부모 배열이 루트 1 트리인지 확인
4. 모든 쿼리를 다시 LCA로 검증
5. 다양한 모드(`chain_unary`, `comb_plus_unary`, `balanced_dense`, `random_recursive_mixed` 등)에서 성능을 본다.

관련 참고 파일:
- 스트레스 하네스 README: `/mnt/data/lca_tree_stress_v3.zip` 내부 `lca_tree_stress_v2/README.md`
- 현재 병합 기준선 코드: [flatten_spqr_current_merged.cpp](sandbox:/mnt/data/flatten_spqr_current_merged.cpp)

---

## 2. 만든 코드와 반례생성기 돌린 후 결과

### 현재 기준선 코드

현재 **새 기준선으로 잡아야 할 파일**은 아래다.

- 기준선 코드: [flatten_spqr_current_merged.cpp](sandbox:/mnt/data/flatten_spqr_current_merged.cpp)

이 파일은 다음을 포함한 병합본이다.

- cheap fan detector
- comb postfan 최적화
- strict S0 stateful shortcut 계열 중 현재 기준선에 남긴 안전한 부분
- branch5 / `splitBlockLocalRebuild()` 내부의 **non-DFS local-id / adjacency build 최적화**

병합 근거 파일:
- 병합 요약: [branch5_non_dfs_merge_diff_summary.txt](sandbox:/mnt/data/branch5_non_dfs_merge_diff_summary.txt)
- 병합 당시 검증: [branch5_non_dfs_merge_report.txt](sandbox:/mnt/data/branch5_non_dfs_merge_report.txt)
- 현재 환경 병합 확인: [branch5_non_dfs_merge_verification_current.txt](sandbox:/mnt/data/branch5_non_dfs_merge_verification_current.txt)

### 현재 환경에서 다시 돌린 반례생성기 결과

아래 수치는 **현재 세션에서 다시 컴파일해서** `lca_tree_stress_v2` 하네스로 돌린 결과다.

실행 조건:
- seed = 1
- shuffle labels = 1
- shuffle queries = 1

대표 결과:

| 모드 | N | 결과 | 시간 | 메모리 |
|---|---:|---|---:|---:|
| chain_unary | 1024 | PASS | 0.84s | 20368KB |
| chain_unary | 1536 | PASS | 1.56s | 21044KB |
| chain_unary | 2048 | PASS | 2.92s | 20112KB |
| comb_plus_unary | 4096 | PASS | 0.41s | 9120KB |
| balanced_dense | 512 | PASS | 0.22s | 26448KB |
| random_recursive_mixed | 512 | PASS | 0.22s | 27628KB |

실행 로그:
- [current_merged_rerun_chain1024.log](sandbox:/mnt/data/current_merged_rerun_chain1024.log)
- [current_merged_rerun_chain1536.log](sandbox:/mnt/data/current_merged_rerun_chain1536.log)
- [current_merged_rerun_chain2048.log](sandbox:/mnt/data/current_merged_rerun_chain2048.log)
- [current_merged_rerun_comb4096.log](sandbox:/mnt/data/current_merged_rerun_comb4096.log)
- [current_merged_rerun_bal512.log](sandbox:/mnt/data/current_merged_rerun_bal512.log)
- [current_merged_rerun_rand512.log](sandbox:/mnt/data/current_merged_rerun_rand512.log)

### 수치 해석

- 현재 merged 기준선은 `chain_unary` 쪽에서 예전 naive/초기 flatten 계열보다 훨씬 안정적으로 내려왔다.
- `comb_plus_unary`, `balanced_dense`, `random_recursive_mixed`는 현재 기준선에서 큰 붕괴 없이 유지되고 있다.
- 다만 이후 실험 브랜치들(`chainpolicy_tuned_release`, `chaindeg_release`)은 **아이디어는 유망했지만** 현재 환경 재실행 기준으론 baseline 승격까지는 못 갔다.

참고 비교 파일:
- chain policy line HOLD: [chainpolicy_tuning_rerun_summary.txt](sandbox:/mnt/data/chainpolicy_tuning_rerun_summary.txt)
- xDeg relax line HOLD: [flatten_spqr_chaindeg_compare.txt](sandbox:/mnt/data/flatten_spqr_chaindeg_compare.txt)

---

## 3. 지금까지 수정해온 이력

아래는 큰 줄기만 남긴 요약이다.

### 3-1. 초기 scaffold / semantic diff 정리

처음에는 멀티파일 scaffold를 만들고,
- `solver.hpp`
- `solver_patch_local.cpp`
- `solver_patch_spqr.cpp`
- `solver_debug.cpp`
- `solver_apply.cpp`

형태로 SPQR_DIRECT bring-up 뼈대를 만들었다.

이 단계의 목표는:
- `RSB_ONE_NODE`로 semantic diff를 맞추고
- hook 위치를 고정하고
- local oracle과 SPQR patch 결과가 의미적으로 같은지 확인하는 것이었다.

### 3-2. TRUE_SPQR raw builder 확장

그 다음에는 `rawSpqrBuildTrueSpqrSkeleton()`을 실제 raw builder 쪽으로 넓혀 갔다.

대략적인 확장 순서는:
- single two-cut star 계열
- chain-of-2-cuts / S-like recursive family
- parallel-of-simple-paths
- single 2-cut recursive family

즉 완전 일반 SPQR decomposition이 아니라, **fallback 영역을 줄이는 family-by-family 확장** 형태였다.

### 3-3. 단일 파일 제출형 flatten

그 뒤에는 멀티파일 코드를 BOJ 제출용 단일 파일로 평탄화하는 작업을 진행했다.

주요 목적:
- end-to-end `solve()` 파이프라인 연결
- 제출용으로 `buildFromQueries / eliminateOne / solve`가 한 파일에서 동작
- semantic diff와 smoke를 넘는 수준에서 스트레스 하네스에 바로 넣을 수 있는 형태 확보

### 3-4. TLE 원인 분해

그 다음 단계에서 가장 중요한 전환점은 **TLE 원인을 두 축으로 나눈 것**이었다.

1. sparse 쪽 (`chain_unary`, `comb_plus_unary`)는
   - component relabel / root refresh / local rebuild 쪽이 병목
2. dense / recursive 쪽 (`balanced_dense`, `random_recursive_mixed`)는
   - eager splitter / raw decomposition / exit expansion 쪽이 병목

이 구분 이후부터는 “전체를 다 고치기”보다, **패턴별 병목을 순서대로 죽이는 방식**으로 진행했다.

### 3-5. sparse 계열 최적화 라인

이 라인에서 실제로 많이 시도한 것들:

- branch5 boolean checker 단일 DFS화
- lambda/std::function 제거
- iterative DFS 실험 (효과 없음 / 폐기)
- no-bad fast path 확장
- cheap fan detector 도입
- `chain_unary` dominant bucket profiler
- split / nested / domination-order / quotient 등 여러 family probe 실험

이 중 의미 있게 남은 것은:
- **cheap fan detector**
- **single DFS boolean checker**
- 그리고 나중의 **stateful keep-only shortcut 실험 결과**였다.

### 3-6. stateful keep-only shortcut 실험 라인

여기서는 다음이 진행됐다.

- coarse signature / family signature profiler
- `statecert_direct`
- `fastkey` 버전
- gated version
- `xDef top2`, `xDeg top3` 등 relax frontier 실험
- `P6 (majorDef stable => disable defer)` 등 policy frontier 실험

이 라인은 중요한 insight를 많이 줬다.

남긴 결론:
- chain에는 도움이 될 수 있음
- random/comb에 새지 않게 막는 gate/policy가 필요함
- 하지만 **현재 환경 재실행 기준으론 baseline 교체까지는 못 갔다**

즉 이 라인은 “유망한 후보 실험군”으로 남아 있고, **현재 merged 기준선에는 완전히 들어가 있지 않다.**

### 3-7. comb residual 최적화 라인

`comb_plus_unary` 쪽에서는 실제로 다음이 큰 전환점이었다.

- fan detector 이후에도 residual path가 남는다는 걸 profiler로 확인
- fan-hit apply/update dominance가 아니라, 실제론 fan-hit splitter dominance를 확인
- 그래서 **fan pre-adj direct path**를 넣어 local graph build 자체를 건너뜀

이 작업이 comb 계열을 크게 끌어내렸다.

### 3-8. branch5 non-DFS 병합 라인

가장 최근에 실제로 병합한 큰 변경은 이거다.

- branch5 / `splitBlockLocalRebuild()` 내부 non-DFS phase를 쪼개서 계측
- top-1 non-DFS hotspot이 `deg_build / adj_edge_write`임을 확인
- `unordered_map` 반복 local-id lookup 대신 **thread_local stamp array** 기반 mapping으로 교체
- local-id / adjacency build 경로를 구조적으로 줄이는 최적화 적용
- 이걸 `flatten_spqr_current_merged.cpp`로 병합

이건 지금 기준선에 **실제로 들어가 있는 최적화**다.

관련 파일:
- profiler: [branch5_non_dfs_prof_report.txt](sandbox:/mnt/data/branch5_non_dfs_prof_report.txt)
- 병합 리포트: [branch5_non_dfs_merge_report.txt](sandbox:/mnt/data/branch5_non_dfs_merge_report.txt)

---

## 4. 현재 코드가 어디까지 왔는지

### 4-1. 현재 기준선

현재 기준선으로 써야 하는 파일은:

- [flatten_spqr_current_merged.cpp](sandbox:/mnt/data/flatten_spqr_current_merged.cpp)

이 파일은 “지금까지 실제로 병합해도 된다고 판단된 것”만 들어간 상태다.

### 4-2. 현재 merged 기준선에 포함된 것

- query graph → BC/BCC 기반 구성
- elimination 기반 solve pipeline
- local rebuild splitter
- SPQR path와 local path의 혼합 구조
- cheap fan detector
- comb postfan 최적화
- strict S0 stateful shortcut 기반의 보수적 처리 일부
- branch5/local-rebuild의 **non-DFS local-id/adjacency build 최적화**

### 4-3. 아직 merged 안 된 것

아래는 아이디어는 좋았지만 **현재 기준선에는 합치지 않은** 실험들이다.

- `chainpolicy_tuned_release`
  - chain에선 약간 도움, 하지만 현재 환경 재실행 기준으론 baseline 교체급 이득 부족
- `chaindeg_release`
  - `xDeg top3` relax는 안전하지만 개선폭이 작아서 HOLD
- mixed `xDef + xDeg` relax frontier
  - 방향은 자연스럽지만 아직 merge 전 단계
- 추가 policy frontier (`P3`, `P6`, `P9` 류)
  - 일부 환경에선 좋아 보였지만, 현재 기준선 승격용으로는 불충분

### 4-4. 현재 코드 상태를 한 문장으로 정리하면

> **문제는 이미 end-to-end로 풀리고, 반례 생성기/검증기 기준 대표 케이스들을 통과하며, 최근 병합 최적화까지 반영된 “실행 가능한 고도화된 기준선”까지 와 있다.**
> 다만 아직 마지막 TLE 여유를 얻기 위한 **잔여 병목 1개씩 제거하는 단계**가 남아 있다.

### 4-5. 다음 세션에서 바로 해야 할 일

다음 세션에서는 아래 순서로 바로 들어가면 된다.

1. `flatten_spqr_current_merged.cpp` 기준선 재실행
2. 전체 family global reprior 다시 측정
3. 현재 진짜 worst family 재확인
4. 그 family의 residual hotspot **1개만** 선택
5. 그 hotspot만 겨냥한 prototype 1개

즉 다음 라운드 목표는:

> **현재 merged 기준선 위에서 진짜 1순위 잔여 병목을 하나 더 줄이는 것**

이다.

---

## 부록: 바로 열어볼 추천 파일

### 기준선 / 현재 코드
- [flatten_spqr_current_merged.cpp](sandbox:/mnt/data/flatten_spqr_current_merged.cpp)

### 현재 환경 재실행 결과
- [current_merged_rerun_chain1024.log](sandbox:/mnt/data/current_merged_rerun_chain1024.log)
- [current_merged_rerun_chain1536.log](sandbox:/mnt/data/current_merged_rerun_chain1536.log)
- [current_merged_rerun_chain2048.log](sandbox:/mnt/data/current_merged_rerun_chain2048.log)
- [current_merged_rerun_comb4096.log](sandbox:/mnt/data/current_merged_rerun_comb4096.log)
- [current_merged_rerun_bal512.log](sandbox:/mnt/data/current_merged_rerun_bal512.log)
- [current_merged_rerun_rand512.log](sandbox:/mnt/data/current_merged_rerun_rand512.log)

### 최신 병합 근거
- [branch5_non_dfs_merge_report.txt](sandbox:/mnt/data/branch5_non_dfs_merge_report.txt)
- [branch5_non_dfs_merge_diff_summary.txt](sandbox:/mnt/data/branch5_non_dfs_merge_diff_summary.txt)
- [branch5_non_dfs_merge_verification_current.txt](sandbox:/mnt/data/branch5_non_dfs_merge_verification_current.txt)

### 보류된 실험군 참고
- [chainpolicy_tuning_rerun_summary.txt](sandbox:/mnt/data/chainpolicy_tuning_rerun_summary.txt)
- [flatten_spqr_chaindeg_compare.txt](sandbox:/mnt/data/flatten_spqr_chaindeg_compare.txt)

