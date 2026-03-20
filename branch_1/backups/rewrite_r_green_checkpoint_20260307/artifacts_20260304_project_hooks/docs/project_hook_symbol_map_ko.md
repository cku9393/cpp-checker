# ProjectHarnessOps static-5 심볼 매핑표

기준 시점: 2026-03-04

## 검색 결과 요약

- 현재 워크스페이스에서 아래 5개 훅의 실제 구현 심볼은 발견되지 않았다.
- `branch_2-1/raw_engine_v1_package/raw_engine_v1.cpp`는 raw primitive 전용 엔진으로, 하네스의 `CompactGraph/RawSpqrDecomp/StaticMiniCore` 시그니처와 직접 호환되지 않는다.
- 따라서 지금 단계의 매핑은 "연결 대상 심볼 명세"까지 확정하고, 실제 프로젝트 코드베이스에서 해당 심볼을 제공하는 방식으로 진행한다.

## static 단계 우선 연결 (5개)

| Harness hook (`ProjectHarnessOps`) | 연결 대상 심볼(기대) | 현재 상태 |
|---|---|---|
| `validateRawSpqrDecomp(H, raw, why)` | `::validateRawSpqrDecomp(const harness::CompactGraph&, const harness::RawSpqrDecomp&, std::string&)` | 미발견 |
| `materializeMiniCore(H, raw, mini, why)` | `::materializeMiniCore(const harness::CompactGraph&, const harness::RawSpqrDecomp&, harness::StaticMiniCore&, std::string&)` | 미발견 |
| `normalizeWholeMiniCore(mini)` | `::normalizeWholeMiniCore(harness::StaticMiniCore&)` | 미발견 |
| `checkMiniOwnershipConsistency(mini, H, why)` | `::checkOwnershipConsistency(const harness::StaticMiniCore&, const harness::CompactGraph&, std::string&)` | 미발견 |
| `checkMiniReducedInvariant(mini, H, why)` | `::checkReducedInvariant(const harness::StaticMiniCore&, const harness::CompactGraph&, std::string&)` | 미발견 |

## 빌드 플래그

- `HARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON`이면 `ProjectHarnessOps`가 위 free-function 심볼을 직접 호출한다.
- 기본값은 `OFF`이며, 이 경우 `ProjectHarnessOps`는 "unwired hook" 메시지로 실패한다.

## 다음 작업

1. 실제 프로젝트 코드베이스에서 위 5개 심볼을 동일 시그니처로 노출한다.
2. 하네스 빌드 시 `-DHARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON`을 켠다.
3. static/manual-only를 다시 실행해 `RAW_VALIDATE_FAIL`을 넘기는지 확인한다.
