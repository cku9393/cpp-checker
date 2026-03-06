# static hook search report

## 결론

현재 워크스페이스에는 `src/project_hook_shims.cpp`의 static 5 hook을
`thin forwarding`으로 바로 연결할 수 있는 실제 프로젝트 함수가 없다.

즉, 다음 함수들은 이름만 못 찾은 상태가 아니라,
하네스가 기대하는 타입/반환 형태를 만족하는 구현 자체가 없다.

- `validateRawSpqrDecomp`
- `materializeMiniCore`
- `normalizeWholeMiniCore`
- `checkOwnershipConsistency`
- `checkReducedInvariant`

## 근거

하네스는 아래 타입 기준의 free function을 기대한다.

- `CompactGraph`
- `RawSpqrDecomp`
- `StaticMiniCore`

참조:

- `include/harness/types.hpp`
- `include/harness/project_hooks.hpp`
- `src/project_hooks_real.cpp`
- `src/project_hook_shims.cpp`

반면 워크스페이스의 비하네스 코드에서 확인되는 실제 구현은
`branch_2-1/raw_engine_v1_package`의 `RawEngine` 계열뿐이다.
이쪽은 live engine state / assert 기반 validator / isolate prep 계층이고,
하네스의 `StaticMiniCore` 계층과 직접 호환되지 않는다.

## 후보와 불일치

### 1. `validateRawSpqrDecomp`

가장 가까운 후보:

- `assert_skeleton_wellformed`
- `assert_occ_patch_consistent`
- `debug_validate_skeleton_and_hosted`

위치:

- `branch_2-1/raw_engine_v1_package/src/raw_validators.cpp`

문제:

- 입력이 `RawEngine`, `RawSkelID`, `OccID`
- 반환이 `bool`이 아니라 `void` / `assert`
- `RawSpqrDecomp`를 입력으로 받지 않음

### 2. `materializeMiniCore`

가장 가까운 후보:

- `prepare_isolate_neighborhood`
- `build_isolated_occ_builder`

위치:

- `branch_2-1/raw_engine_v1_package/src/raw_primitives.cpp`

문제:

- 결과 타입이 `IsolatePrepared`, `RawSkeletonBuilder`
- 하네스가 요구하는 `StaticMiniCore`를 만들지 않음
- 입력도 `CompactGraph + RawSpqrDecomp`가 아니라 `RawEngine + RawSkelID + OccID`

### 3. `normalizeWholeMiniCore`

가장 가까운 후보:

- `normalize_prep`

위치:

- `branch_2-1/raw_engine_v1_package/tests/raw_engine_cases.cpp`

문제:

- 테스트 헬퍼
- `IsolatePrepared`용 normalization
- `StaticMiniCore`와 무관

### 4. `checkOwnershipConsistency`

가장 가까운 후보:

- `assert_occ_patch_consistent`
- `assert_skeleton_wellformed`

위치:

- `branch_2-1/raw_engine_v1_package/src/raw_validators.cpp`

문제:

- `StaticMiniCore` 입력이 아님
- `why` 문자열을 채우는 인터페이스가 아님
- assert 실패 기반이라 harness contract와 다름

### 5. `checkReducedInvariant`

가장 가까운 후보:

- `valid_split_pair_for_support`
- `discover_split_pair_from_support`

위치:

- `branch_2-1/raw_engine_v1_package/src/raw_planner.cpp`

문제:

- planner helper
- `StaticMiniCore` 전체 reduced invariant checker가 아님
- 반환 의미와 입력 타입이 다름

## 현재 판단

지금 필요한 것은 shim body 교체가 아니라,
아래 둘 중 하나다.

1. 실제 프로젝트의 static/mini 계층 코드를 이 워크스페이스에 가져온다.
2. `CompactGraph/RawSpqrDecomp -> RawEngine/IsolatePrepared/... -> StaticMiniCore` 변환을 포함한
   새 adapter layer를 작성한다.

둘 다 없이 `project_hook_shims.cpp`의 TODO를 채우는 것은
정상적인 thin forwarding이 아니라 임의 구현이 된다.

## 따라서 이번 작업에서 하지 않은 것

- `src/project_hook_shims.cpp`의 TODO를 허위 심볼로 교체하지 않음
- `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON` 재빌드를 반복하지 않음
- dummy hook은 건드리지 않음
