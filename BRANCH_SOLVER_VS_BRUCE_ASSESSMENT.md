# Branch Solver Assessment Against Bruce Baseline

## 기준선

기준 제출 후보는 `bruce/lca_tree2_practical_fast.cpp`다.

`bruce/lca_tree2_practical_fast.cpp`와 `bruce/bruce_structural_event.cpp`는 sha256 기준 완전 동일하다.

```text
sha256: c5daeebe7cb2eb6d05c3ecf5d3cabad037251cdc7f5ef2efe1e3e68774027c09
size: 8146 bytes
lines: 326
```

이 문서에서 브루스 기준선은 다음 성질을 가진 코드로 본다.

- 구조적 deterministic solver다.
- Tarjan BCC, DSU, layer deletion, component split을 직접 유지한다.
- 정답 선택에 randomized heuristic이나 timeout fallback을 쓰지 않는다.
- 성능 측면에서는 `largest surviving component` 재사용과 `pending layer queue`로 반복 작업을 줄인다.
- 단, 큰 BCC layer가 반복 재계산되는 최악 시간복잡도에 대한 완전한 수학적 증명은 없다.

## 비교 대상

| 분류 | 파일 | 역할 |
|---|---|---|
| Bruce baseline | `bruce/lca_tree2_practical_fast.cpp` | 최종 제출 후보 clean source |
| branch_1 | `branch_1/project_static_adapter.cpp`, `branch_1/rewrite_r_harness_main.cpp` | static adapter / rewrite harness |
| branch_2-1 | `branch_2-1/raw_engine_v1_package/` | raw-engine 검증 패키지 |
| branch_2_2 | `branch_2_2/round45_resume/round45_branch_2_2_solver.cpp` | Round45 SPQR/dense-shadow 실험 solver |
| branch_3 | `branch_3/boj28350_branch_3_solver.cpp` | DynamicGraph/EulerTourForest/progress40 계열 solver |
| branch_4 | `branch_4/90/full_dynamic_top_tree_engine_90.cpp` | proof/runtime engine |
| lca_tree_stress_v5 | `lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp` | outer LCA stress 실험 solver |

## 요약 결론

| 대상 | 브루스 대비 결론 | 제출 후보성 |
|---|---|---|
| branch_1 | LCA/BOJ28350 solver가 아니다. | 없음 |
| branch_2-1 | raw-engine 검증 패키지이며 LCA 제출 solver가 아니다. | 없음 |
| branch_2_2 | SPQR/dense-shadow 실험 계열. 계측/캐시/fallback이 많고 완결 제출본이 아니다. | 낮음 |
| branch_3 | DynamicGraph/EulerTourForest 계열. 구조 자체가 브루스와 다르고 partial authoritative 상태다. | 낮음 |
| branch_4 | 문서상 complete BOJ solver가 아니다. proof/runtime 재현용이다. | 없음 |
| lca_tree_stress_v5 | outer-suite 실험 라인. missing instrumentation 복구가 다음 과제다. | 낮음 |

## 브루스 기준선의 장점과 남은 리스크

### 장점

- 코드가 작다. 약 326줄이다.
- 제출용 clean source로 분리되어 있다.
- `bruce_structural_event`와 동일하며, full gate 기준 최고 성능 후보로 관리된다.
- 삭제된 vertex/layer마다 BCC를 재구성하고 component ownership을 갱신하는 구조가 명확하다.
- 재귀 DFS/고정 배열/OOB/assert RE 리스크를 줄인 계열이 별도로 있다.

### 남은 리스크

- 최악 시간복잡도에 대한 완전한 수학적 보장은 없다.
- 큰 BCC layer가 계속 살아남고, 삭제 이벤트마다 비싼 재계산이 반복되면 느려질 수 있다.
- 다만 hard gate에서는 `largest surviving component` 재사용과 `pending layer` scheduling이 효과적으로 작동했다.

## branch_1 문제점

대상 파일:

- `branch_1/project_static_adapter.cpp`
- `branch_1/rewrite_r_harness_main.cpp`

### 수학적 완결성

branch_1은 LCA/BOJ28350 solver 라인이 아니다. 정적 adapter와 rewrite harness 성격이므로, 브루스 기준의 LCA tree construction solver로 평가할 대상이 아니다.

### 휴리스틱성

문제 solver의 휴리스틱 여부를 평가할 수 없다. 이 브랜치는 solver 알고리즘이 아니라 harness/adapter 정리본이다.

### 속도 문제

BOJ28350 제출 runtime과 직접 관련이 없다.

### 결론

브루스 기준으로는 비교 대상 제외다.

## branch_2-1 문제점

대상:

- `branch_2-1/raw_engine_v1_package/`

### 수학적 완결성

branch_2-1은 raw-engine 검증 패키지다. policy gate, reference model, raw primitives, planner, validators 등이 중심이고, BOJ28350/LCA 제출 solver가 아니다.

### 휴리스틱성

solver heuristic 문제가 아니라 검증/정책/원시 연산 안정화 문제다.

### 속도 문제

BOJ 제출 runtime과 직접 비교할 수 없다.

### 결론

브루스 기준 제출 후보와는 목적이 다르다. 보존 가치는 있지만, 제출 solver 후보는 아니다.

## branch_2_2 문제점

대상 파일:

- `branch_2_2/round45_resume/round45_branch_2_2_solver.cpp`
- `branch_2_2/round45_bundle_archive/solve.cpp`
- `branch_2_2/round45_artifacts/flatten_spqr_current_merged.cpp`

### 브루스와의 구조 차이

branch_2_2는 브루스 계열과 다르다.

관찰된 특징:

- `DENSE_SPQR` 계열 macro와 instrumentation이 많다.
- `PROFILE`, `SHADOW`, `fallback`, `cache`, `SPQR` 관련 코드가 많다.
- 코드 크기가 브루스보다 훨씬 크다.
- `round45_branch_2_2_solver.cpp`는 약 8600줄이다.
- `bruce/lca_tree2_practical_fast.cpp`는 약 326줄이다.

### 수학적 완결성 문제

`branch_2_2/round45_resume/README.md`는 이 workspace의 목표를 다음처럼 둔다.

- Round45 baseline을 독립적으로 빌드/실행한다.
- missing instrumentation 복구 전에도 smoke gate blocker를 같은 환경에서 재현한다.
- 이후 패치를 누적한다.

즉 현재 branch_2_2는 완료된 제출 solver라기보다 Round45 재개/계측 복구 workspace다.

### 휴리스틱성 문제

브루스는 작은 deterministic BCC-layer 구조인 반면, branch_2_2는 SPQR, dense shadow, cache, fallback, profile row gate가 섞여 있다.

이 구조는 정답 로직 자체를 검증하려면 많은 내부 invariant와 계측 row가 함께 맞아야 한다. 따라서 실전 제출 후보로는 다음 문제가 있다.

- 정답 로직과 계측/검증 로직이 강하게 섞여 있다.
- fallback/cache 경로가 많아 실제 제출 환경에서의 동작 설명이 어렵다.
- smoke/gate instrumentation 자체가 blocker였던 이력이 있다.

### 속도 문제

속도 리스크는 다음과 같다.

- SPQR/dense-shadow 구조 자체가 브루스보다 상수가 크다.
- 많은 profile/shadow/census/prefilter row 관련 코드가 있다.
- 캐시와 fallback은 특정 hard case에서는 도움될 수 있지만, 제출용 clean hot path를 복잡하게 만든다.
- branch-local smoke/gate 재현성이 핵심 과제로 남아 있었다.

### 결론

branch_2_2는 실험/재개/계측 복구 라인이다. 브루스 기준 제출 후보보다 수학적 설명과 runtime 신뢰성이 낮다.

## lca_tree_stress_v5 문제점

대상 파일:

- `lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp`

### 브루스와의 구조 차이

`lca_tree_stress_v5`는 독립 outer-suite stress 라인이다. README에 따르면 `branch_2_2`와 분리된 workspace이며, shared harness는 `tooling/` 아래에 있다.

관찰된 특징:

- 코드 크기는 약 5539줄이다.
- `SPQR`, `SHADOW`, `PROFILE`, `fallback` 관련 구조가 보인다.
- `branch_2_2`보다는 작지만 브루스보다 훨씬 크다.
- `branch_2_2`의 `flatten_spqr_current_merged.cpp`와 높은 계열 유사성을 가진다.

### 수학적 완결성 문제

README의 Immediate next task는 다음 취지다.

- Round45가 optimization quality에서 멈춘 것이 아니다.
- baseline source에서 profiler hook contract가 빠져 있었다.
- separator-prefilter optimization 전에 missing row-emission hook을 복구해야 한다.

즉 이 workspace도 제출용 완결본이라기보다 outer stress 실험 라인이다.

### 휴리스틱성 문제

브루스는 BCC layer deletion 구조가 핵심인데, `lca_tree_stress_v5`는 SPQR/dense-shadow 실험 라인이다.

문제점:

- 검증/계측 목적 코드가 제출 hot path와 섞일 가능성이 있다.
- shadow/prefilter/fallback류 구조는 hard gate 분석에는 유용하지만 clean 제출 설명에는 불리하다.
- 현재 README상 missing instrumentation 복구가 선행 과제로 남아 있다.

### 속도 문제

- SPQR 구조와 shadow 검증은 상수가 크다.
- 브루스의 326줄 구조보다 코드 경로가 훨씬 길다.
- 실험용 row emission과 profile 관련 코드가 성능과 신뢰성 양쪽의 부담이다.

### 결론

`lca_tree_stress_v5`는 현재 outer stress 연구 라인이다. 브루스 기준 제출 후보를 대체할 상태는 아니다.

## branch_3 문제점

대상 파일:

- `branch_3/boj28350_branch_3_solver.cpp`

### 브루스와의 구조 차이

branch_3은 브루스 계열과 완전히 다르다.

관찰된 특징:

- `EulerTourForest`, `DynamicGraph`, `Entry`, `TreeEdge`, `EdgeToken` 등 동적 그래프 구조가 중심이다.
- progress40, layout signature reuse gate, zero-span fastpath 계열이다.
- 코드 크기는 약 18847줄이다.
- 브루스와 token-level 유사도는 매우 낮다.

### 수학적 완결성 문제

`branch_3/current_state_summary.md`에 따르면 branch_3은 다음 상태다.

- `partial authoritative` 범위가 남아 있다.
- active solver는 `separator-decomposition drift` 상태로 읽힌다.
- 다음 rewrite는 bundled progress40 source line에 anchor를 맞춰야 한다.
- dense 1024 release/repeat, 4096 representative, long-run terminal row persistence close가 남아 있다.

즉 완전한 제출 후보가 아니다.

### 휴리스틱성 문제

branch_3은 문서상 literature-grade invariant를 목표로 하지만, 현재 구현은 다음 요소들이 섞여 있다.

- layout signature compare/reuse gate
- zero-span eligibility and fastpath commit
- route-aware time attribution
- cache/fallback
- progress40 derived reference

이들은 실험/최적화 라인으로는 의미가 있지만, 브루스처럼 단일 구조로 설명되는 clean deterministic 제출 코드와는 거리가 있다.

### 속도 문제

문서상 병목은 두 가지다.

1. 운영 병목: long run terminal row persistence failure
2. 알고리즘 병목: `zero-span eligibility and fastpath commit`

직접 수치도 문서에 남아 있다.

```text
signature source load and materialize: 24.9643%
layout signature compare and reuse gate core: 25.0339%
zero-span eligibility and fastpath commit: 49.9983%
```

즉 빠른 제출 후보라기보다 아직 profiling/rewriting 대상이다.

### 결론

branch_3은 별도 DynamicGraph/EulerTourForest 연구 라인이다. 현재 상태에서는 브루스보다 수학적/운영적 제출 신뢰성이 낮다.

## branch_4 문제점

대상 파일:

- `branch_4/90/full_dynamic_top_tree_engine_90.cpp`

### 브루스와의 구조 차이

branch_4는 BOJ 제출 solver가 아니다.

`branch_4/project_status_summary.md`는 다음을 명시한다.

- `full_dynamic_top_tree_engine_90.cpp`는 `This is NOT the complete BOJ solver`라고 명시한다.
- 현재 verified 성공은 solver 완성이 아니다.
- support8 / shell15 / tail / completion-lock proof system의 current reproduction이다.

### 수학적 완결성 문제

branch_4의 완결성은 BOJ solver 완결성이 아니다. support8 slice와 proof/runtime artifact 재현의 완결성이다.

남은 caveat도 문서상 존재한다.

- support8 slice 바깥 확장 범위 문제
- shell16 completion 아님
- higher-support completion 아님
- general theorem proof 아님
- BOJ solver 아님

### 휴리스틱성 문제

solver heuristic 문제로 평가하면 안 된다. branch_4는 proof/runtime engine이며, 제출 code hot path가 아니다.

### 속도 문제

파일은 매우 크고 runtime/cache/fallback/signature/support8 관련 코드가 많지만, BOJ 제출 runtime 비교 대상이 아니다.

### 결론

branch_4는 브루스 제출 후보와 비교할 solver가 아니다. 보존 목적은 proof/runtime 문서와 support8 completion-lock reproduction이다.

## 최종 제출 관점 순위

현재 파일 기준 제출 관점 순위는 다음과 같다.

| 순위 | 파일 | 이유 |
|---|---|---|
| 1 | `bruce/lca_tree2_practical_fast.cpp` | `bruce_structural_event.cpp`와 동일. 최종 제출용 clean source. |
| 1 | `bruce/bruce_structural_event.cpp` | clean full gate 444/444 PASS로 관리된 최고 성능 계열. |
| 2 | `bruce/bruce_structural.cpp` | structural event보다 느리지만 안정 계열. |
| 3 | `bruce/bruce_hardened_indexsafe.cpp` | index/OOB 안전성은 높지만 더 느리고 메모리 부담이 크다. |

브랜치 폴더 안 solver들은 제출 후보라기보다 실험, 재개, 검증, 증명, 프로파일링 라인으로 보는 것이 맞다.

## 작업 시 권장 기준

- 제출 코드는 `bruce/` 폴더에서 관리한다.
- `branch_2_2`와 `lca_tree_stress_v5`는 SPQR/dense-shadow 실험 라인으로 유지한다.
- `branch_3`은 progress40/DynamicGraph 계열 연구 라인으로 유지한다.
- `branch_4`는 proof/runtime reproduction 라인으로 유지한다.
- 브랜치 solver를 제출 후보로 승격하려면 먼저 브루스 기준으로 다음을 통과해야 한다.
  - 코드 경로 단순화
  - instrumentation/fallback/cache 분리
  - 독립 full gate 재실행
  - 제출용 clean source 생성
  - 최악 또는 준최악 runtime 리스크 설명
