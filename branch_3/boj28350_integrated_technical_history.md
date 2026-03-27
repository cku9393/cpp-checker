# BOJ 28350 `쿼리와 트리 2` 개발 통합 문서

## 목차
- [Part I. 개요](#part-i-개요)
  - [1. 문서 목적](#1-문서-목적)
  - [2. 문제 요약](#2-문제-요약)
  - [3. 최종 strongest 버전 요약](#3-최종-strongest-버전-요약)
  - [4. 현재 strongest 구현의 의미와 한계](#4-현재-strongest-구현의-의미와-한계)
- [Part II. 코드 구조 / 형식 / 증명 구조](#part-ii-코드-구조--형식--증명-구조)
  - [5. 전체 아키텍처 개요](#5-전체-아키텍처-개요)
  - [6. `OuterSolver`의 역할과 데이터 흐름](#6-outersolver의-역할과-데이터-흐름)
  - [7. `LiteraturePotentialOracle`의 역할](#7-literaturepotentialoracle의-역할)
  - [8. `DecrementalNBTopology`의 역할과 local refinement update](#8-decrementalnbtopology의-역할과-local-refinement-update)
  - [9. `PotentialHandleKernel`의 역할과 synthetic potential](#9-potentialhandlekernel의-역할과-synthetic-potential)
  - [10. explicit decomposition child lattice 구조](#10-explicit-decomposition-child-lattice-구조)
  - [11. `buildClosedHandleFromWitness`의 의미](#11-buildclosedhandlefromwitness의-의미)
  - [12. strict-child relocation 구조](#12-strict-child-relocation-구조)
  - [13. topology local refinement correctness와 proof package 대응](#13-topology-local-refinement-correctness와-proof-package-대응)
  - [14. semantic completeness / strict-child completeness / complexity skeleton 대응](#14-semantic-completeness--strict-child-completeness--complexity-skeleton-대응)
  - [15. 코드 구성 요소와 proof package 간의 매핑](#15-코드-구성-요소와-proof-package-간의-매핑)
- [Part III. 구현 과정 히스토리](#part-iii-구현-과정-히스토리)
  - [16. 초기 접근과 느린 풀이](#16-초기-접근과-느린-풀이)
  - [17. BlockKernel / NeighborhoodTopologyKernel 계열의 시도](#17-blockkernel--neighborhoodtopologykernel-계열의-시도)
  - [18. watcher-local certificate로의 전환](#18-watcher-local-certificate로의-전환)
  - [19. strict-shrink 문제의 반례와 raw support witness의 한계](#19-strict-shrink-문제의-반례와-raw-support-witness의-한계)
  - [20. balanced handle 도입](#20-balanced-handle-도입)
  - [21. potential handle line 도입](#21-potential-handle-line-도입)
  - [22. BC-tree flavored decomposition line으로의 전환](#22-bc-tree-flavored-decomposition-line으로의-전환)
  - [23. `literature_progress` 계열로 strongest 라인에 수렴한 과정](#23-literature_progress-계열로-strongest-라인에-수렴한-과정)
  - [24. `progress7_bcdecomp_verified`에 이르기까지 무엇이 해결되었는가](#24-progress7_bcdecomp_verified에-이르기까지-무엇이-해결되었는가)
  - [25. 무엇을 버렸고 왜 버렸는가](#25-무엇을-버렸고-왜-버렸는가)
- [Part IV. 사용한 논문 / 참고자료 정리](#part-iv-사용한-논문--참고자료-정리)
  - [26. 핵심 논문 목록](#26-핵심-논문-목록)
  - [27. 각 논문/자료가 코드에 준 영향](#27-각-논문자료가-코드에-준-영향)
  - [28. 참고만 하고 구현에는 직접 넣지 못한 부분](#28-참고만-하고-구현에는-직접-넣지-못한-부분)
  - [29. 공개 구현 / 라이브러리 / 보조 자료 정리](#29-공개-구현--라이브러리--보조-자료-정리)
  - [30. 참고자료와 현재 코드 구조의 대응 관계](#30-참고자료와-현재-코드-구조의-대응-관계)
- [Part V. 결론](#part-v-결론)
  - [31. strongest 구현의 현재 위상](#31-strongest-구현의-현재-위상)
  - [32. proof package 문서와 strongest 구현의 관계](#32-proof-package-문서와-strongest-구현의-관계)
  - [33. 문헌급 최종 보장판이라고 부르기 위해 남은 증명/문서화 과제](#33-문헌급-최종-보장판이라고-부르기-위해-남은-증명문서화-과제)
  - [34. 부록: 용어 표 / 주요 파일 표 / 버전 계보](#34-부록-용어-표--주요-파일-표--버전-계보)

---

# Part I. 개요

## 1. 문서 목적

이 문서는 BOJ 28350 `쿼리와 트리 2`를 해결하기 위해 구축한 코드와 증명 패키지를 하나의 큰 기술 문서로 통합하기 위해 작성되었다. 기준 구현은 `boj28350_literature_progress7_bcdecomp_verified.cpp`이며, 본 문서는 이 파일이 왜 현재 strongest 기준 파일인지, 이 strongest 구조가 어떤 proof package 위에 서 있는지, 그리고 그 strongest 구조가 어떤 구현 히스토리를 거쳐 만들어졌는지를 한 흐름으로 설명한다.

문서의 목적은 세 가지다.

첫째, 최종 strongest 구현의 **코드 구조 / 형식 / 증명 구조**를 정리한다. 단순히 클래스 목록을 나열하는 것이 아니라, 각 클래스가 어떤 상태를 유지하고 어떤 불변식을 보장하며, 그것이 증명 패키지의 어느 정리와 대응하는지까지 보여준다.

둘째, 이 strongest 구현이 어떻게 형성되었는지 **구현 과정 히스토리**를 기록한다. 개발 과정은 단순한 코드 누적이 아니라, 여러 아이디어를 시도하고 반례와 복잡도 문제를 통해 버리고, locality와 decomposition을 강화해 가는 과정이었다. 따라서 이 문서는 그 시행착오를 구조적으로 정리한다.

셋째, 구현 과정에서 사용했던 **논문 및 참고자료**를 정리한다. 어떤 논문은 직접 구조로 반영되었고, 어떤 논문은 방향 설정이나 seam 설계 수준에서만 반영되었으며, 어떤 자료는 참고에 그쳤다. 이 구분을 명확히 남겨두는 것은 이후 증명 문서 작성이나 추가 구현을 위해 중요하다.

이 문서는 README보다 훨씬 크고, proof package보다 훨씬 넓다. README처럼 사용법만 설명하지도 않고, proof package처럼 정리와 보조정리만 담지도 않는다. strongest 구현을 중심으로, **설계 문서 + 구현 회고 + 참고문헌 해설**을 결합한 통합 문서라는 점이 이 문서의 핵심 성격이다.

## 2. 문제 요약

문제는 루트가 1인 트리의 구조를 모르는 상태에서, 여러 개의 질의 `(u, v, w)`가 주어졌을 때 `LCA(u, v) = w`를 만족하는 어떤 rooted tree를 복원하는 것이다. 입력은 항상 해가 존재한다고 보장되므로, 목표는 해를 판정하는 것이 아니라 **해를 하나 구성하는 것**이다.

이 문제는 겉으로 보면 LCA 제약을 만족하는 부모 배열 구성 문제처럼 보이지만, 실제 난점은 각 정점 `w`에 대해 자신이 owner인 쿼리 `(a, b, w)`가 현재 alive 부분문제에서 “서로 다른 child-subtree로 갈라져야 하는가”를 동적으로 판정해야 한다는 데 있다. 구현 도중 이 판정은 결국 다음과 동치라는 점이 고정되었다.

- query `(a, b, w)`가 현재 failing이라는 것은
- 보조 그래프 `H_alive - w` 안에서 `a`와 `b`가 연결된다는 것과 동치이고,
- 다시 말해 `w` 주변의 두 incident edge가 같은 block / biconnected region 안에 있다는 것과 동치이다.

즉 문제의 본질은 단순 트리 DP가 아니라, **정점 삭제 하의 동적 biconnectivity 의미론**과 연결된다. 구현 과정 내내 이 지점이 반복해서 드러났고, 최종 strongest 구조는 이 의미론을 직접 품는 방향으로 수렴했다.

## 3. 최종 strongest 버전 요약

현재 strongest 기준 파일은 **`boj28350_literature_progress7_bcdecomp_verified.cpp`**다. 이 파일이 strongest 기준인 이유는 다음과 같다.

1. `OuterSolver` - `LiteraturePotentialOracle` - `DecrementalNBTopology` - `PotentialHandleKernel`이 하나의 single-file solver 안에 통합되어 있다.
2. `LOCAL` 계측 기준으로 다음이 확인되었다.
   - `owner_rebuild_calls = 0`
   - `local_fallback = 0`
   - `partition_mismatch = 0`
   - `strict_child_exists_but_missed = 0`
   - `strict_child_structural_miss = 0`
   - `semantic_escape_count = 0`
   - `strict_child_global_fallback_used = 0`
3. topology 쪽은 exact rebuild 없는 local refinement-only update 경로로 닫혔고, witness 쪽은 explicit BC-tree flavored child lattice 위에서 strict-child relocation이 동작한다.
4. `buildClosedHandleFromWitness(...)`가 exact witness를 closed decomposition subtree handle로 올려서 semantic completeness differential까지 점검한다.

즉 strongest 기준 파일은 “대충 빠른 버전”이 아니라, 지금까지 쌓아 온 구조적 개선과 증명 패키지의 주요 invariant가 가장 많이 반영된 버전이다.

## 4. 현재 strongest 구현의 의미와 한계

현재 strongest 구현은 코드/실험 기준으로는 매우 강하다. topology local update는 `LOCAL` differential에서 mismatch 0을 기록하고, strict-child 검색도 structural miss 없이 동작한다. `owner_rebuild_calls = 0`이라는 계측은 release 경로에서 exact rebuild를 제거했다는 사실을 뒷받침한다. 또한 synthetic potential(`budgetExp`)은 모든 relocation에서 1 감소하도록 강제되어 strict-child descent 구조도 코드 수준에서는 닫혀 있다.

하지만 이 strongest 구현을 곧바로 “문헌급 최종 보장판”이라고 선언하는 것은 신중해야 한다. 이유는 세 가지다.

첫째, 현재 decomposition은 **BC-tree flavored explicit lattice**까지는 올라왔지만, block 내부 SPQR decomposition은 seam만 남아 있다. 즉 구조적으로는 BC-tree 수준에서 child lattice를 형성하는 strongest 구현이며, 이것이 SPQR-level decomposition과 1:1로 대응한다는 정리는 문서에서 별도로 닫아야 한다.

둘째, `buildClosedHandleFromWitness(...)`는 현재 코드/LOCAL differential 기준으로 semantic escape를 막지만, “전역 exact witness를 포함하는 최소 closed subtree를 생성한다”는 성질은 구현상 seam과 differential로 지지될 뿐, 논문화된 정리로는 아직 독립 정리 패키지에 추가해야 한다.

셋째, complexity theorem은 계측과 skeleton까지는 정리되어 있지만, literature-grade 선언을 하려면 정리 형태의 서술형 증명까지 분리해 적어야 한다.

즉 strongest 구현의 현재 위상은 다음처럼 요약할 수 있다.

> 코드와 LOCAL differential 기준으로는 strongest이며, proof package와의 대응 관계도 명확하다. 다만 문헌급 최종 보장판이라고 선언하려면 decomposition 정의, semantic completeness, complexity theorem을 문서로 독립 정리하는 마지막 단계가 남아 있다.

---

# Part II. 코드 구조 / 형식 / 증명 구조

BOJ 28350 `쿼리와 트리 2`는 정점 수 `N`, 쿼리 수 `M`이 모두 최대 100,000인 조건에서, 알려지지 않은 rooted tree를 직접 복원해야 하는 문제다. 입력으로는 `LCA(u, v) = w` 형태의 제약만 주어지고, 실제 트리의 간선은 전혀 주어지지 않는다. 따라서 이 문제의 본질은 일반적인 LCA 질의 처리와 반대로, **LCA 의미론을 만족하는 트리를 역으로 구성하는 것**에 있다.

이 문서의 Part II는 바로 이 역구성 문제를 코드 수준에서 어떻게 분해했는지를 설명한다. 구현은 쿼리를 직접 트리 구조로 변환하지 않고, 먼저 제약을 동적 그래프와 오라클 문제로 바꾼 뒤, `indeg`, `bad`, `compParent` 같은 상태를 유지하면서 유효한 루트를 한 단계씩 제거하는 방식으로 구성된다. 다시 말해, 이 파트는 “BOJ 28350의 원문제를 strongest 구현이 어떤 내부 문제들로 환원했고, 각 계층이 그 환원을 어떻게 담당하는가”를 기술하는 부분이다.

## 5. 전체 아키텍처 개요

strongest 파일의 전체 구조는 다음과 같이 읽는 것이 가장 자연스럽다.

1. **기반 동적 연결성 계층**
   - `EulerTourForest`
   - `DynamicGraph`
   - 이를 감싼 `DynamicForestCoreHDT`

2. **문제 의미론 계층**
   - `DecrementalNBTopology`
   - owner별 endpoint partition과 component split을 담당

3. **witness / handle 계층**
   - `PotentialHandleManager`
   - `PotentialHandleKernel`
   - explicit decomposition child lattice와 strict-child relocation 담당

4. **오라클 계층**
   - `LiteraturePotentialOracle`
   - topology와 handle을 합쳐 `bad[v]` 의미론을 제공

5. **외곽 솔버 계층**
   - `OuterSolver`
   - `indeg`, `bad`, `compParent`를 유지하며 최종 부모 배열을 구성

이 아키텍처는 구현 과정 후반부에 의도적으로 분리된 것이다. 초반에는 느린 exact rebuild와 witness 탐색이 뒤엉켜 있었고, 중간에는 `BlockKernel` 계열과 watcher-local certificate 계열이 오락가락했다. strongest 버전의 특징은 이 층들이 비교적 명확하게 분리되어 있고, proof package도 이 분리를 따라 서술된다는 점이다.

간단히 말해,
- `OuterSolver`는 “무엇을 삭제할지”를 정하고,
- `LiteraturePotentialOracle`은 “현재 제약이 만족되는지”를 알려주며,
- `DecrementalNBTopology`는 owner 기준 neighborhood partition을,
- `PotentialHandleKernel`은 strict-child decomposition handle을 담당한다.

## 6. `OuterSolver`의 역할과 데이터 흐름

`OuterSolver`는 문제의 외곽 reduction을 구현하는 계층이다. 이 계층이 유지하는 주 상태는 비교적 단순하다.

- `indeg[v]`: direct-ancestor digraph에서의 현재 indegree
- `bad[v]`: owner가 `v`인 branching query 중 현재 failing 개수
- `compParent[C]`: 현재 component `C`의 루트가 최종 트리에서 붙을 부모

핵심 아이디어는 이미 중반부에 고정된 다음 조건이다.

- 현재 alive 상태에서 `indeg[v] == 0`이고 `bad[v] == 0`이면
- 정점 `v`를 현재 component의 루트로 안전하게 선택할 수 있다.

`OuterSolver`는 실제로 다음 흐름을 반복한다.

1. direct edge 전처리와 branch query 전처리
2. `indeg` 초기화
3. 오라클 초기화 후 `bad` 초기화
4. `indeg=0, bad=0`인 정점을 큐에 넣음
5. 큐에서 정점을 뽑아
   - `parent[v] = compParent[comp(v)]`
   - owner direct edge 제거로 indeg 감소
   - 오라클 `eraseVertex(v)` 호출
   - component split과 witness change 반영
6. 모든 정점이 처리될 때까지 반복

중요한 점은 `OuterSolver`가 topology나 witness 구조를 거의 모른다는 것이다. `OuterSolver`는 오라클이 제공하는
- 현재 component,
- affected witness changes,
- component split
만을 이용한다. 그래서 구현 과정이 길어져도 `OuterSolver`는 비교적 일찍부터 안정되었고, 대부분의 개발 노력이 오라클 내부 커널에 집중되었다.

## 7. `LiteraturePotentialOracle`의 역할

`LiteraturePotentialOracle`는 strongest 파일에서 topology 커널과 potential handle 커널을 실제로 합치는 오라클이다. 이 클래스가 하는 일은 크게 세 가지다.

첫째, **현재 failing query의 집합**을 관리한다. 즉 owner별 branching query 중 어떤 것이 아직 failing인지, 어떤 witness handle을 들고 있는지, 어떤 정점 삭제에 의해 다시 검사되어야 하는지 관리한다.

둘째, **`watchersByVertex`** 스타일의 연결을 통해 local update를 유도한다. 어떤 query의 handle region이 특정 정점 `x`를 포함하면, `x` 삭제 시 그 query는 affected candidate가 된다. 이는 global exact rebuild를 피하는 locality의 핵심이다.

셋째, **`DecrementalNBTopology`와 `PotentialHandleKernel`을 연결**한다. query의 owner/endpoint 관계는 topology가 관리하는 `incidentClass`로 해석되고, witness relocation은 `PotentialHandleKernel::relocateToStrictChild(...)`가 처리한다. `LiteraturePotentialOracle`는 이 둘을 사용하여
- query가 resolved 되었는지
- strict child로 내려갔는지
- child 내부 rebuild가 사용되었는지
를 outer solver가 이해할 수 있는 형태로 바꿔 준다.

오라클 계층은 strongest 코드에서 가장 중요한 의미론 접합부다. outer solver와 local graph 구조 사이를 이어 주고, proof package의 여러 정리(Topology refinement, strict-child completeness, semantic completeness)가 실제 코드에서 만나게 되는 위치이기도 하다.

## 8. `DecrementalNBTopology`의 역할과 local refinement update

`DecrementalNBTopology`는 owner별 endpoint partition과 component split을 담당하는 커널이다. 이 클래스가 유지하는 상태는 proof package의 Topology local refinement correctness 정리와 직접 연결된다.

중요 필드는 다음과 같다.

- `ownerEndpoints_[v]`
- `endpointClass_[v]`
- `classEndpoints_[v]`
- `classRep_[v]`
- `classTouchedByRemoved_[v]`
- `endpointWitnessZone_[v]`
- `ownerDirty_[v]`
- `nextClassId_[v]`

이 구조는 “owner 전체 neighborhood”가 아니라, **owner endpoint induced partition**만 유지하는 방식이다. 즉 owner `v`에 대해 실제로 필요한 것은 `H_alive - v` 전체의 connected components가 아니라, `v`가 owner인 쿼리의 endpoint들이 어떤 class로 묶이는지다.

### local refinement-only update

strongest 버전의 `updateOwnerLocal(owner, removedX, oldNeighbors)`는 다음 구조로 움직인다.

1. 삭제 전 `removedX`의 old-neighbor를 수집한다.
2. `H_alive - owner - removedX`에서 이 old-neighbor들을 seed로 BFS한다.
3. BFS가 만드는 connected zone을 계산한다.
4. 기존 old class와 새 zone의 관계를 `old class -> new zones` refinement로 해석한다.
5. touched old class만 분해하고, untouched old class는 그대로 둔다.
6. touched class가 여러 조각으로 나뉘면 가장 큰 fragment가 old class id를 재사용하고, 나머지는 새 class id를 받는다.

이 규칙의 핵심은 다음 불변식이다.

> 삭제 후 owner partition은 old partition의 refinement일 뿐, 서로 다른 old class가 합쳐지는 일은 없다.

이 불변식 덕분에 global exact rebuild 없이도 local update-only 경로가 가능해졌다. `LOCAL` 모드에서는 `computeOwnerExactMap(...)`와의 canonical partition differential을 비교하며 `partition_mismatch`를 계측한다. strongest 기준 파일에서 이 값이 0이라는 것은, 적어도 현재 테스트 범위에서는 local refinement-only 경로가 exact partition과 일치한다는 뜻이다.

## 9. `PotentialHandleKernel`의 역할과 synthetic potential

`PotentialHandleKernel`은 strongest 구현의 witness 계층 핵심이다. 초반 구현에서는 raw support-subgraph나 지역 witness를 직접 들고 strict-shrink를 시도했지만, 그 family는 수학적으로 universal half-shrink를 줄 수 없다는 반례(theta family)와 brute-force로 붕괴되었다. 이후 strongest 라인은 witness를 **PotentialHandle**로 바꾸고, strict-shrink를 raw subgraph mass가 아니라 **synthetic potential**로 측정하는 방향으로 전환했다.

`PotentialHandle`이 들고 있는 핵심 정보는 다음과 같다.

- `nodeId`: explicit decomposition lattice 상의 현재 node
- `owner, a, b`: query 정보
- `regionVerts`: decomposition subtree가 가리키는 closed region
- `certVerts, certEdges`: region 안의 exact restricted witness certificate
- `budgetExp`: synthetic potential

이때 strict-child descent는 “subgraph가 절반으로 줄었다”가 아니라
- `new.budgetExp = old.budgetExp - 1`
로 표현된다.

즉 strongest 구현은 strict-shrink를 순수 기하적 크기 감소가 아니라 **decomposition child descent**로 본다. 이게 raw support witness에서 potential handle family로 넘어오며 생긴 결정적 변화다.

## 10. explicit decomposition child lattice 구조

`PotentialHandleKernel` 내부에는 explicit decomposition lattice가 있다. 코드상 주 구조는 다음이다.

- `DecompNode`
- `DecompTree`
- `BaseRegion`
- `LatticeNode`

### `DecompNode`
`DecompNode`는 explicit child lattice의 노드다. 필드로는 대략 다음이 있다.

- `id`
- `kind`
- `verts`
- `edges`
- `children`
- `parent`
- `boundaryVerts`
- `budgetExp`

여기서 strongest 구현은 **BC-tree flavored lattice**를 사용한다. 즉 `kind`는 현재 BC-tree 수준 decomposition을 표현하고, block 내부 SPQR decomposition은 seam만 남겨 두었다. 이 구조가 중요한 이유는 child가 더 이상 heuristic 후보 집합이 아니라, `children(nodeId)`라는 **명시적 lattice 자식 집합**을 가진다는 점이다.

### `BaseRegion`과 `LatticeNode`
`BaseRegion`은 closed region 그 자체를 canonicalize하여 저장하는 기초 단위다. `LatticeNode`는 `(baseId, budgetExp)`를 묶어 explicit lattice node로 만든다. 이 두 단계를 분리한 이유는 region 구조와 synthetic potential을 독립적으로 관리하기 위해서다.

이 explicit lattice 덕분에 `relocateToStrictChild(...)`는
- child 후보를 heuristic으로 생성해보는 절차
이 아니라
- explicit lattice 자식을 열거하고 그중 exact predicate가 true인 child를 선택하는 절차
로 바뀔 수 있었다.

## 11. `buildClosedHandleFromWitness`의 의미

`buildClosedHandleFromWitness(owner, a, b, pathVerts, pathEdges)`는 strongest 구현에서 semantic completeness를 담당하는 핵심 함수다. 이 함수의 목적은 **전역 exact witness path를 decomposition subtree handle로 lift**하는 것이다.

동작의 핵심은 다음과 같다.

1. exact witness path가 지나가는 block/atom을 찾는다.
2. 그 atom들을 BC-tree 위에서 연결하는 path를 취한다.
3. articulation/boundary vertex를 포함하도록 closure를 취한다.
4. 결과를 closed region으로 canonicalize한다.

즉 witness는 더 이상 단순한 exact path 자체가 아니다. path를 포함하는 **minimal closed decomposition region**이 handle이 된다.

이 설계가 필요한 이유는 semantic escape 때문이다. 만약 witness를 path 그대로만 저장하면, 삭제 후 query가 전역에서는 계속 failing인데 현재 handle region이 그 전역 witness를 담지 못하는 문제가 생길 수 있다. strongest 구현은 `buildClosedHandleFromWitness(...)`를 통해 witness를 closure된 region handle로 올려, semantic completeness differential(`semantic_escape_count`)를 검사할 수 있는 구조로 바꾸었다.

## 12. strict-child relocation 구조

strongest 구현에서 `relocateToStrictChild(...)`는 explicit child lattice를 사용하는 strict-child search로 재작성되어 있다. 구조는 다음 네 단계로 이해하면 된다.

1. `children(nodeId)`를 통해 explicit child를 열거한다.
2. 각 child에 대해 `childStillContainsWitness(handle, childNode, removedV)`를 exact restricted predicate로 평가한다.
3. strict child가 true인 child가 있으면, 그 child 내부에서만 exact rebuild를 수행한다.
4. rebuilt exact witness를 다시 closed handle로 lift하고, `budgetExp`를 1 줄인다.

이 구조에서 중요한 금지 조건이 있다.

- old region 바깥 global rebuild fallback 금지
- decomposition child가 아닌 region으로의 점프 금지

즉 strongest 구현은 strict-child completeness를 코드 수준에서 다음 형태로 강제한다.

> strict child가 존재하면 explicit lattice 자식 중 하나 안에서 rebuild가 이루어져야 하며, 그 결과 potential은 반드시 1 감소한다.

`LOCAL` 계측에서 `strict_child_exists_but_missed = 0`, `strict_child_structural_miss = 0`, `strict_child_global_fallback_used = 0`이 나오는 것은 바로 이 구조가 테스트 범위에서 잘 작동한다는 강한 지표다.

## 13. topology local refinement correctness와 proof package 대응

proof package에서 topology 쪽 핵심 정리는 Owner Partition Refinement Correctness였다. 코드에서 이 정리에 대응하는 위치는 `DecrementalNBTopology`다.

### 정리와 코드 대응
- 정리의 state는 `ownerEndpoints_`, `endpointClass_`, `classEndpoints_`, `classRep_`, `classTouchedByRemoved_`, `endpointWitnessZone_`로 구현된다.
- touched class만 refinement한다는 주장은 `updateOwnerLocal(...)` 로직과 `largest-fragment old id reuse` 규칙으로 구현된다.
- exact differential은 `LOCAL`에서 `computeOwnerExactMap(...)`와 canonical partition 비교로 구현된다.

즉 proof package의 문장을 코드로 옮기면 다음과 같다.

- 삭제는 old partition을 합치지 않는다.
- touched old class만 split 가능하다.
- untouched class는 그대로 유지된다.
- local zone BFS가 touched class의 실제 split을 찾아낸다.

계측이
- `owner_rebuild_calls = 0`
- `local_fallback = 0`
- `partition_mismatch = 0`
를 보인다는 것은, 이 정리가 적어도 differential test 범위에서는 코드와 충돌하지 않는다는 뜻이다.

## 14. semantic completeness / strict-child completeness / complexity skeleton 대응

proof package의 다른 세 축은 다음처럼 strongest 코드에 대응한다.

### semantic completeness
- 코드 위치: `buildClosedHandleFromWitness(...)`, semantic differential check
- 카운터: `semantic_escape_count`
- 의미: global exact witness가 있는데 current handle subtree 안 exact witness가 없으면 카운트 증가

### strict-child completeness
- 코드 위치: `children(nodeId)`, `childStillContainsWitness(...)`, `relocateToStrictChild(...)`
- 카운터: `strict_child_exists_but_missed`, `strict_child_structural_miss`, `strict_child_global_fallback_used`
- 의미: explicit child lattice가 strict child를 놓치지 않는지 확인

### complexity skeleton
- topology 계측: `topology_zone_bfs_vertices`, `topology_zone_bfs_edges`
- strict-child 계측: `strict_child_depth_sum`, `strict_child_rebuild_vertices`, `strict_child_rebuild_edges`
- 의미: local refinement update와 strict-child descent의 총 비용이 어디서 발생하는지 관찰

즉 proof package는 strongest 코드 위에 독립적으로 떠 있는 문서가 아니라, LOCAL 계측을 통해 strongest 코드에 연결된 정리 패키지라고 보는 편이 맞다.

## 15. 코드 구성 요소와 proof package 간의 매핑

| proof package 개념 | strongest 코드 대응 |
|---|---|
| owner endpoint partition | `DecrementalNBTopology::endpointClass_`, `classEndpoints_`, `classRep_` |
| touched class refinement | `updateOwnerLocal(...)` |
| exact differential oracle | `computeOwnerExactMap(...)` in `LOCAL` |
| explicit decomposition lattice | `PotentialHandleKernel::DecompTree`, `DecompNode`, `LatticeNode` |
| closed handle lift | `buildClosedHandleFromWitness(...)` |
| exact strict-child predicate | `childStillContainsWitness(...)` |
| strict-child relocation | `relocateToStrictChild(...)` |
| synthetic potential | `PotentialHandle::budgetExp` |
| semantic completeness differential | `semantic_escape_count` |
| complexity skeleton | `topology_zone_bfs_*`, `strict_child_depth_sum`, `strict_child_rebuild_*` |

이 표가 중요한 이유는, strongest 구현과 proof package가 어디까지 대응되는지 한눈에 보여주기 때문이다. strongest 코드가 강하다고 해서 자동으로 문헌급이 되는 것은 아니고, 이 대응 관계가 정리/증명과 함께 닫혀야 문헌급 선언이 가능해진다.

---

# Part III. 구현 과정 히스토리

## 16. 초기 접근과 느린 풀이

가장 초기 접근은 문제를 거의 정직하게 recursive subproblem으로 나누는 형태였다. 현재 alive 집합에서 루트 후보를 찾고, 그 후보를 삭제한 뒤 생기는 connected component를 child-subproblem으로 재귀하는 방식이었다. 이 방향 자체는 reduction 관점에서는 맞았지만, 구현은 정점 삭제마다
- DFS / low-link
- 같은 child 여부 판정
- root 가능성 검증
을 전역적으로 다시 계산하는 느린 재구축이었다.

이 방식은 테스트가 약하면 통과할 수도 있는 수준이었지만, 최악 시간복잡도와 특정 데이터에서의 폭발이 명확했다. 구현 과정 초반부터 “이건 데이터가 약해서 통과하는 코드일 뿐”이라는 피드백이 나온 것도 이 시기다.

즉 초반 실패는 알고리즘 아이디어가 틀렸다기보다, **정적 재귀를 동적 connectivity 문제로 잘못 다루고 있었다**는 데 있었다.

## 17. BlockKernel / NeighborhoodTopologyKernel 계열의 시도

다음 단계에서는 문제의 본질을 “같은 child인지 / 같은 block인지”로 보는 방향이 강화되면서 `BlockKernel`, `NeighborhoodTopologyKernel` 계열을 만들게 되었다. 이 계열은 정점 삭제 후 old block을 다시 분해하고, watcher를 block에 매달아 local하게 affected query만 다시 보는 구조였다.

이 라인은 초반 전역 재구축보다 훨씬 좋아 보였다. locality가 생겼고, witness도 path가 아니라 block에 묶일 수 있었다. 하지만 치명적인 한계가 있었다.

- 큰 block 일반형에서는 여전히 old block 전체 재분해 fallback이 남았다.
- strict-shrink witness를 raw support-subgraph로는 전면 보장할 수 없었다.

즉 `BlockKernel` 계열은 문제를 local하게 바꾸는 데는 성공했지만, **문헌급 final kernel**로 가기에는 중간 단계에 머물렀다.

## 18. watcher-local certificate로의 전환

그 다음 큰 전환이 watcher-local line이었다. 이 시점에는 affected query를 owner 전체나 block 전체가 아니라, **현재 witness가 실제로 포함하는 정점 집합** 기준으로 다시 보는 구조가 들어갔다. `watchersByVertex[x]`는 바로 이 시기의 산물이다.

이 라인이 가져온 이득은 명확했다.

- 삭제 정점 `x`가 witness에 없으면 query를 건드릴 필요가 없다.
- affected query 수가 실제 witness locality와 연결된다.
- local certificate 재배치(seam)를 넣을 자리가 생긴다.

하지만 raw support certificate는 곧 한계에 부딪혔다. 특정 theta-family에서는 exact witness가 존재해도 half-shrink가 항상 불가능했다. 이 반례가 이후 witness family 교체로 이어진다.

## 19. strict-shrink 문제의 반례와 raw support witness의 한계

이 프로젝트에서 가장 큰 개념적 전환 중 하나는, raw support-subgraph witness를 계속 강화해도 전면 strict-shrink는 얻을 수 없다는 사실을 **반례 family + 완전탐색**으로 닫은 것이다.

핵심 패턴은 theta graph였다. owner와 두 terminal 사이에 여러 평행 경로가 있는 구조에서는, 어떤 connected support-subgraph witness를 잡더라도 삭제 후에도 surviving witness mass가 old mass 절반 이하로 떨어지지 않는 경우가 존재한다.

이 결론은 굉장히 중요했다. 왜냐하면 그것이 뜻하는 바는 다음과 같았기 때문이다.

- witness를 path/support subgraph로 두는 line은 원천적으로 한계가 있다.
- strict-shrink를 위해서는 witness family 자체를 바꿔야 한다.

이 시점 이후 프로젝트는 raw support witness를 “잘 다듬는” 방향을 버리고, balanced handle / potential handle line으로 넘어간다.

## 20. balanced handle 도입

balanced handle은 raw support witness 대신, 더 큰 region과 exact local certificate를 분리해서 드는 구조였다. 즉 handle은
- region
- certificate
- budget/potential
을 함께 가지게 되었다.

이 도입의 의미는 다음과 같다.

- certificate는 exact witness를 위한 국소 증거 역할
- region은 semantic completeness를 위한 closure 역할
- potential은 strict-shrink descent의 장부 역할

balanced handle의 초기 버전은 separator child, block-cut child, BC-path child 같은 decomposition-oriented 후보를 추가하는 형태였다. 이 line은 raw support보다 훨씬 강했지만, raw region size 자체를 potential로 쓰면 여전히 완전한 strict-shrink를 얻을 수 없었다. 그래서 다음 단계로 potential handle이 나온다.

## 21. potential handle line 도입

potential handle line은 witness family를 더 명확히 바꾼 버전이다. 핵심은 다음과 같다.

- strict-shrink를 raw subgraph size가 아니라 `budgetExp` 감소로 본다.
- child로 내려갈 때는 반드시 `budgetExp = old - 1`을 강제한다.
- witness는 decomposition child 내부 exact rebuild를 통해 유지한다.

이 라인은 strict descent를 코드 invariant로 강제하는 데 성공했다. 하지만 초창기에는 child 자체가 heuristic candidate 집합에 가까웠다. 즉 budget은 synthetic하게 줄지만, 그 child가 진짜 decomposition child인지가 모호했다. 이 문제를 해결하기 위해 explicit child lattice를 도입하는 방향으로 이어졌다.

## 22. BC-tree flavored decomposition line으로의 전환

explicit child lattice를 strongest 코드에서 실제로 쓰기 시작한 것이 `BC-tree flavored decomposition` line이다. 이 시점부터 `PotentialHandle`은 `nodeId`, `children(nodeId)`, `boundaryVerts`, `budgetExp`를 가지게 되었고, child 후보는 더 이상 heuristic 후보 생성기가 아니라 explicit lattice 자식 집합을 사용하게 되었다.

BC-tree flavored라는 표현은 중요한 의미를 가진다. 현재 strongest 구현은 SPQR 전체를 구현하지는 않았지만,
- block-cut tree 수준에서는 explicit child lattice를 갖고,
- block 내부 SPQR는 seam으로 남겨둔 상태
이기 때문이다.

즉 decomposition line은 현재 strongest에서 “명시적 child lattice”까지는 확실히 올라왔고, 나중에 문헌급 문서화를 하려면 이 BC-tree flavored lattice가 논문 decomposition 정의와 어떻게 대응되는지 서술하면 된다.

## 23. `literature_progress` 계열로 strongest 라인에 수렴한 과정

`literature_progress` 계열은 strongest line을 “문헌급 proof package에 맞게” 재편하는 과정이었다. 여기서 핵심은 두 축이었다.

1. `DecrementalNBTopology`를 touched owner local update로 바꾸기
2. `PotentialHandleKernel`을 strict-child seam + child-budget rebuild line으로 바꾸기

이 과정을 거치며 exact rebuild guard가 점점 사라지고, LOCAL differential과 structural counters가 추가되었다. 즉 코드가 단순히 동작하는 수준을 넘어서, 어떤 정리/불변식을 테스트하고 있는지가 드러나게 된 시기다.

## 24. `progress7_bcdecomp_verified`에 이르기까지 무엇이 해결되었는가

`progress7_bcdecomp_verified`는 여러 줄기 중 strongest로 수렴한 지점이다. 이 버전까지 해결된 것은 다음과 같다.

- outer solver reduction 고정
- touched owner local refinement-only topology update
- owner exact rebuild 제거
- explicit BC-tree flavored child lattice
- `buildClosedHandleFromWitness(...)`에 의한 closed subtree seed handle 생성
- strict-child relocation의 explicit child-only 경로
- semantic completeness differential
- complexity 계측 추가

즉 strongest 기준 파일은 단지 가장 최신 버전이 아니라, **증명 패키지와의 대응이 가장 풍부한 버전**이기 때문에 strongest로 취급된다.

## 25. 무엇을 버렸고 왜 버렸는가

구현 과정에서 버린 아이디어는 오히려 최종 strongest 구조를 이해하는 데 중요하다.

### 버린 것 1: 전역 exact rebuild 중심 접근
이유:
- 최악 시간복잡도 폭발
- owner별/삭제별 locality를 전혀 반영하지 못함

### 버린 것 2: BlockKernel만으로 끝내려는 접근
이유:
- 큰 block 일반형에서 old block 전체 재분해 fallback이 남음
- 문헌급 dynamic biconnectivity kernel과의 간극이 큼

### 버린 것 3: raw support witness의 strict-shrink line
이유:
- theta family 반례
- connected support-subgraph family에서 universal half-shrink 불가능

### 버린 것 4: heuristic child 후보 집합만으로 strict-child를 닫으려는 접근
이유:
- structural completeness를 문서로 닫을 수 없음
- child가 진짜 decomposition 자식이라는 말이 안 됨

이 버려진 아이디어들의 흔적은 strongest 코드에도 남아 있다. 예를 들어 region closure, watcher-local counters, exact differential은 모두 “왜 이전 접근이 안 됐는가”의 부산물이다.

---

# Part IV. 사용한 논문 / 참고자료 정리

## 26. 핵심 논문 목록

아래는 구현 과정에서 핵심적으로 영향을 준 자료들이다.

1. **Jacob Holm, Kristian de Lichtenberg, Mikkel Thorup. _Poly-logarithmic deterministic fully-dynamic algorithms for connectivity, minimum spanning tree, 2-edge, and biconnectivity_. J. ACM, 2001.**
2. **Jacob Holm, Wojciech Nadara, Eva Rotenberg, Marek Sokołowski. _Fully dynamic biconnectivity in \~O(log^2 n) time_. arXiv:2503.21733, 2025.**
3. **Richard Peng, Bryce Sandlund, Daniel D. Sleator. _Optimal Offline Dynamic 2,3-Edge/Vertex Connectivity_. arXiv:1708.03812 / WADS, 2019.**
4. **Giuseppe Di Battista, Roberto Tamassia. _On-line maintenance of triconnected components with SPQR-trees_. Algorithmica 15(4), 1996.**
5. **Tom Tseng. `dynamic-connectivity-hdt` GitHub repository.**
6. **문제 관련 PS 커뮤니티 논의(동적 biconnectivity 관점 언급).**

## 27. 각 논문/자료가 코드에 준 영향

### Holm-de Lichtenberg-Thorup 2001
핵심 아이디어:
- dynamic connectivity / biconnectivity의 polylog deterministic framework

이 프로젝트에 준 영향:
- `DynamicForestCoreHDT`의 기반 개념
- dynamic connectivity substrate를 strongest 구현 하단에 두는 방향 정당화

반영 정도:
- **직접 반영 + 공개 구현을 통해 구현 substrate 사용**

### Holm-Nadara-Rotenberg-Sokołowski 2025
핵심 아이디어:
- fully dynamic biconnectivity를 spanning forest + per-vertex neighborhood data structure로 다룸

이 프로젝트에 준 영향:
- “남은 본질은 neighborhood-state local update와 decomposition child handle이다”라는 판단의 핵심 근거
- strongest 구조를 `DecrementalNBTopology + PotentialHandleKernel`로 분리한 배경

반영 정도:
- **구조/seam 강하게 반영**, 논문 내부 자료구조를 그대로 구현한 것은 아님

### Peng-Sandlund-Sleator
핵심 아이디어:
- offline dynamic higher connectivity는 divide-and-conquer와 equivalent graph reduction으로 매우 빠르게 처리할 수 있음

이 프로젝트에 준 영향:
- offline setting과 adaptive online deletion order의 차이를 분명히 인식하게 함
- “오프라인 정답 구조를 그대로 꽂을 수는 없다”는 결론의 근거

반영 정도:
- **참고만 함**

### Di Battista–Tamassia SPQR
핵심 아이디어:
- triconnected decomposition과 dynamic maintenance에 SPQR-tree 사용

이 프로젝트에 준 영향:
- BC-tree flavored decomposition 이후 SPQR seam을 남겨두는 설계
- child lattice가 block 내부에서도 더 세분화 가능하다는 방향 제시

반영 정도:
- **구조/seam 반영**

### Tom Tseng dynamic-connectivity-hdt
핵심 아이디어:
- dynamic connectivity HDT 구현체

이 프로젝트에 준 영향:
- single-file strongest solver 하단 substrate
- `DynamicForestCoreHDT` 구현 편입

반영 정도:
- **직접 반영**

### PS 커뮤니티 논의
핵심 아이디어:
- 이 문제를 결국 두 정점이 같은 이중연결요소에 남는지/언제 분리되는지의 문제로 봐야 한다는 관점

이 프로젝트에 준 영향:
- root recursion/closure-only line을 버리고 동적 biconnectivity 관점으로 고정하는 데 기여

반영 정도:
- **방향 설정 참고자료**

## 28. 참고만 하고 구현에는 직접 넣지 못한 부분

몇몇 자료는 강한 영향을 줬지만 strongest 구현에 직접 들어가진 못했다.

### fully dynamic biconnectivity의 neighborhood DS 내부 구현
이론적으로는 가장 직접적인 목표였지만, 공개 구현 부재와 구현 난이도 때문에 strongest 구현에서는 `DecrementalNBTopology`의 local refinement-only kernel로 근사했다.

### SPQR 내부 완전 구현
현재 strongest는 BC-tree flavored lattice까지는 explicit하지만, block 내부 SPQR decomposition은 seam으로 남겨두었다. 즉 SPQR는 문서와 구조에선 등장하지만, 구현의 주 상태로는 아직 완전히 쓰이지 않는다.

### complexity theorem의 완전한 polylog 상수/차수 증명
계측과 skeleton은 남겼지만, 논문용 complexity theorem으로는 아직 독립 문서화가 필요하다.

## 29. 공개 구현 / 라이브러리 / 보조 자료 정리

### 사용한 공개 구현
- `dynamic-connectivity-hdt` 계열 구현
  - Euler tour tree / dynamic graph substrate
  - strongest single-file 안에 내장된 형태로 사용

### 자체 구현 보조 구조
- watcher-local oracle line
- balanced handle / potential handle manager
- explicit BC-tree flavored lattice
- LOCAL differential / proof counters

### 보조 자료
- proof package 문서
- verification report
- intermediate versions (`progress`, `checklist`, `bcdecomp`, `verified` 등)

## 30. 참고자료와 현재 코드 구조의 대응 관계

| 참고자료 | strongest 코드에서 대응되는 곳 |
|---|---|
| HDT dynamic connectivity | `DynamicForestCoreHDT`, embedded `EulerTourForest`, `DynamicGraph` |
| dynamic biconnectivity neighborhood DS 관점 | `DecrementalNBTopology`의 owner local refinement kernel |
| BC-tree / SPQR decomposition | `PotentialHandleKernel::DecompTree`, `buildClosedHandleFromWitness`, `children(nodeId)` |
| offline higher connectivity 대비점 | direct 적용은 없음, 설계 판단 근거 |
| SPQR dynamic maintenance | block 내부 SPQR seam 필드/향후 확장 지점 |
| PS community dynamic biconnectivity 관점 | 전체 프로젝트 방향 전환의 해석 틀 |

---

# Part V. 결론

## 31. strongest 구현의 현재 위상

`boj28350_literature_progress7_bcdecomp_verified.cpp`는 이 프로젝트에서 만들어진 strongest 기준 파일이다. 이 파일은 단순히 가장 최신인 버전이 아니라,
- outer solver reduction,
- topology local refinement,
- explicit decomposition child lattice,
- closed handle seed generation,
- strict-child relocation,
- semantic completeness differential,
- complexity 계측
을 가장 많이 동시에 품고 있는 버전이기 때문에 strongest다.

## 32. proof package 문서와 strongest 구현의 관계

`literature_grade_proof_package.md`는 strongest 구현 위에 얹히는 정리 패키지다. strongest 구현이 어떤 구조를 실제로 갖고 있는지 보여 주는 것이 코드라면, proof package는 그 구조를 theorem/lemma로 다시 쓰는 문서다.

둘의 관계는 다음처럼 볼 수 있다.

- strongest 코드: 구현된 구조와 계측
- proof package: 그 구조가 어떤 correctness/completeness/complexity 주장으로 해석되는가

즉 proof package는 strongest 구현의 해설서가 아니라, strongest 구현을 **문헌급 기술 보고서**로 승격시키기 위한 정리 패키지다.

## 33. 문헌급 최종 보장판이라고 부르기 위해 남은 증명/문서화 과제

현재 strongest 구현은 코드/실험 기준으로는 매우 강하다. 하지만 완전히 신중한 표현을 쓰면, 아직 다음 문서화가 남아 있다.

1. BC-tree flavored explicit child lattice가 논문 decomposition 정의와 어떻게 정확히 대응되는지에 대한 독립 정리
2. `buildClosedHandleFromWitness(...)`가 minimal closed subtree를 만든다는 독립 정리
3. complexity theorem의 서술형 증명 패키지

즉 구현보다 남은 것은 **정리의 독립 문서화**에 가깝다.

## 34. 부록: 용어 표 / 주요 파일 표 / 버전 계보

### 용어 표
- **owner**: branching query `(a,b,owner)`의 LCA 후보 정점
- **endpoint partition**: `H_alive - owner`에서 owner의 endpoint들이 속한 connectivity class 분할
- **strict child**: explicit decomposition lattice 자식 중 witness를 유지하는 child
- **semantic escape**: global exact witness는 존재하지만 current handle region 안에는 witness가 없는 경우
- **synthetic potential**: `budgetExp`, strict-child descent 때 1씩 감소하는 budget

### 주요 파일 표
| 파일 | 의미 |
|---|---|
| `boj28350_literature_progress7_bcdecomp_verified.cpp` | strongest 기준 single-file solver |
| `literature_grade_proof_package.md` | strongest 구현 위의 정리 패키지 |
| `boj28350_literature_progress7_bcdecomp_report.md` | LOCAL 계측 검증 리포트 |

### 버전 계보(요약)
- 느린 전역 재구축 계열
- BlockKernel / NeighborhoodTopologyKernel 계열
- watcher-local certificate 계열
- balanced handle line
- potential handle line
- BC-tree flavored explicit child lattice
- `literature_progress7_bcdecomp_verified`

---

## 마무리

이 문서가 말하려는 바는 단순하다. strongest 구현은 우연히 통과하는 코드가 아니라, 여러 실패한 접근을 버리고 locality, decomposition, synthetic potential, semantic completeness를 점차 강화하면서 만들어진 결과물이다. 현재 strongest 기준 파일은 그 과정의 응축판이고, proof package는 그 응축된 구조를 문헌급 정리 패키지로 설명하기 위한 첫 완성형 문서다.
