# static adapter design

## 결정

경로 A는 보류한다.

이유:

- 현재 Git 브랜치는 `main`만 존재한다.
- 원격도 `origin/main`만 보인다.
- 현재 워크스페이스에서 `StaticMiniCore` 계층의 실제 구현은 하네스 바깥에 없다.
- `raw_engine_v1_package`는 raw engine 계층만 제공하고, `StaticMiniCore`에 직접 대응하는 구현이 없다.

따라서 다음 단계는 경로 B, 즉 adapter 설계다.

## 목표

`src/project_hook_shims.cpp`의 static 5 hook을 다음 흐름으로 채운다.

1. 하네스 입력을 프로젝트 raw 표현으로 변환
2. 프로젝트 raw validator / materializer 호출
3. 프로젝트 mini 표현을 하네스 `StaticMiniCore`로 역변환
4. 하네스가 요구하는 `bool(..., why)` 계약으로 정리

## static 5 대상

- `validateRawSpqrDecomp(H, raw, why)`
- `materializeMiniCore(H, raw, mini, why)`
- `normalizeWholeMiniCore(mini)`
- `checkOwnershipConsistency(mini, H, why)`
- `checkReducedInvariant(mini, H, why)`

## 타입 매핑표

### 1. `CompactGraph -> project raw type`

현재 직접 대응 타입은 없다.

가장 가까운 프로젝트 타입은 아래다.

- `RawEngine`
- `RawSkeleton`
- `RawOccRecord`
- `RawSkelID`
- `OccID`

문제:

- `CompactGraph`는 압축된 block view다.
- `RawEngine`은 slot pool 기반의 live graph/storage 전체 상태다.
- `CompactGraph`만으로는 occurrence pool, skeleton pool, hosted occurrence 관계를 복원할 수 없다.

따라서 필요한 새 중간 타입:

```cpp
struct ProjectRawSnapshot {
    RawEngine engine;
    RawSkelID rootSid = NIL_U32;
    std::vector<OccID> inputOccOfEdge;
};
```

필요 helper:

```cpp
bool buildProjectRawSnapshot(const harness::CompactGraph& H,
                             const harness::RawSpqrDecomp& raw,
                             ProjectRawSnapshot& out,
                             std::string& why);
```

이 helper가 해야 할 일:

- `RawSpqrDecomp.nodes` / `treeEdges`를 순회해 project `RawEngine` skeleton들로 재구성
- `ownerOfInputEdge`를 occurrence/slot ownership으로 매핑
- `CompactGraph.edges`의 `realEdge`, `outsideNode`, `oldArc`를 project edge/occ metadata와 연결

### 2. `project raw type -> RawSpqrDecomp validation`

직접 대상 validator는 없다.

현재 사용할 수 있는 project 측 후보:

- `assert_skeleton_wellformed`
- `assert_occ_patch_consistent`
- `debug_validate_skeleton_and_hosted`

문제:

- 모두 `RawEngine` 기반
- `void` / `assert` 계약
- harness가 요구하는 `bool(..., why)`와 다름

따라서 wrapper helper가 필요하다.

```cpp
bool validateProjectRawSnapshot(const ProjectRawSnapshot& snap,
                                std::string& why);
```

이 helper는 내부에서:

- root skeleton wellformed 검사
- occurrence ownership 검사
- assert 기반 함수를 `try/catch` 또는 사전 명시 검사로 감싸서 `why` 문자열로 바꿔야 한다

주의:

- project validator가 `assert` 기반이면 release build에서 무력화될 수 있다.
- 가능하면 assert 호출에만 의존하지 말고, 동일 조건을 명시적으로 다시 검사하는 함수가 필요하다.

### 3. `project mini type <-> StaticMiniCore`

현재 직접 대응하는 project mini 타입은 없다.

가장 가까운 후보:

- `IsolatePrepared`
- `RawSkeletonBuilder`

문제:

- `IsolatePrepared`는 occurrence 하나의 isolate view다.
- `StaticMiniCore`는 mini SPQR core 전체를 표현한다.
- `RawSkeletonBuilder`는 builder이며 normalized mini graph가 아니다.

따라서 두 가지 선택지 중 하나가 필요하다.

선택지 1:

- 새 project-side 중간 타입 `ProjectMiniCore`를 만든다.

```cpp
struct ProjectMiniNode { /* project-specific */ };
struct ProjectMiniArc  { /* project-specific */ };
struct ProjectMiniCore {
    bool valid = false;
    std::vector<ProjectMiniNode> nodes;
    std::vector<ProjectMiniArc> arcs;
    std::vector<std::pair<int,int>> ownerOfInputEdge;
};
```

선택지 2:

- 실제 static/mini 구현 코드를 외부에서 가져온 뒤 그 타입을 그대로 사용한다.

현재 워크스페이스 상태에서는 선택지 1이 현실적이다.

필요 helper:

```cpp
bool materializeProjectMiniCore(const ProjectRawSnapshot& snap,
                                ProjectMiniCore& out,
                                std::string& why);

void normalizeProjectMiniCore(ProjectMiniCore& mini);

bool checkProjectMiniOwnershipConsistency(const ProjectMiniCore& mini,
                                          const ProjectRawSnapshot& snap,
                                          std::string& why);

bool checkProjectMiniReducedInvariant(const ProjectMiniCore& mini,
                                      const ProjectRawSnapshot& snap,
                                      std::string& why);

bool exportStaticMiniCore(const ProjectMiniCore& in,
                          harness::StaticMiniCore& out,
                          std::string& why);

bool importStaticMiniCore(const harness::StaticMiniCore& in,
                          ProjectMiniCore& out,
                          std::string& why);
```

## shim별 목표 호출 흐름

### `validateRawSpqrDecomp`

```cpp
ProjectRawSnapshot snap;
if (!buildProjectRawSnapshot(H, raw, snap, why)) return false;
return validateProjectRawSnapshot(snap, why);
```

### `materializeMiniCore`

```cpp
ProjectRawSnapshot snap;
ProjectMiniCore projectMini;
if (!buildProjectRawSnapshot(H, raw, snap, why)) return false;
if (!materializeProjectMiniCore(snap, projectMini, why)) return false;
return exportStaticMiniCore(projectMini, mini, why);
```

### `normalizeWholeMiniCore`

```cpp
ProjectMiniCore projectMini;
std::string why;
if (!importStaticMiniCore(mini, projectMini, why)) {
    throw std::runtime_error(why.empty() ? "importStaticMiniCore failed" : why);
}
normalizeProjectMiniCore(projectMini);
if (!exportStaticMiniCore(projectMini, mini, why)) {
    throw std::runtime_error(why.empty() ? "exportStaticMiniCore failed" : why);
}
```

### `checkOwnershipConsistency`

```cpp
ProjectMiniCore projectMini;
ProjectRawSnapshot snap;
if (!importStaticMiniCore(mini, projectMini, why)) return false;
if (!buildProjectRawSnapshot(H, /* raw unavailable here */ ???, snap, why)) return false;
return checkProjectMiniOwnershipConsistency(projectMini, snap, why);
```

중요한 문제:

- 이 hook 시그니처에는 `raw`가 없다.
- 즉 `checkOwnershipConsistency(mini, H, why)`만으로는 project raw snapshot을 완전 재구성할 수 없을 수 있다.

따라서 아래 둘 중 하나가 추가로 필요하다.

1. `StaticMiniCore` 자체에 ownership 검사용 충분한 정보가 들어 있어야 한다.
2. `CompactGraph`만으로 재구성 가능한 raw snapshot 축약형을 설계해야 한다.

현재는 1번이 더 현실적이다.

### `checkReducedInvariant`

```cpp
ProjectMiniCore projectMini;
if (!importStaticMiniCore(mini, projectMini, why)) return false;
return checkProjectMiniReducedInvariant(projectMini, why);
```

이 경우 `CompactGraph`는 보조 consistency 확인에만 사용하고,
reduced invariant는 mini 내부 구조만으로 판정하는 쪽이 더 자연스럽다.

## 구현 우선순위

1. `ProjectRawSnapshot` 설계
2. `buildProjectRawSnapshot(H, raw, ...)` 구현
3. `validateProjectRawSnapshot(...)` 구현
4. `ProjectMiniCore` 설계
5. `materializeProjectMiniCore(...)` / `exportStaticMiniCore(...)` 구현
6. `normalizeProjectMiniCore(...)`
7. `checkProjectMiniOwnershipConsistency(...)`
8. `checkProjectMiniReducedInvariant(...)`
9. 그 다음에만 `src/project_hook_shims.cpp` body 연결

## 현재 상태에서 바로 가능한 것

현재 워크스페이스만으로는 adapter 설계까지는 가능하지만,
`src/project_hook_shims.cpp`의 5개 body를 실제 성공 경로로 채울 수는 없다.

그 이유는 다음 두 축이 아직 없기 때문이다.

- `CompactGraph/RawSpqrDecomp`를 project raw state로 복원하는 변환기
- `StaticMiniCore`와 왕복 가능한 project mini 표현

즉, 다음 실제 구현 단위는 shim 교체가 아니라
`ProjectRawSnapshot` / `ProjectMiniCore` / import-export helper 작성이다.
