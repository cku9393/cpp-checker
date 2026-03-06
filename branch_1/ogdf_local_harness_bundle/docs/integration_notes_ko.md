# integration notes

## 권장 연결 구조

### raw backend
- `IRawSpqrBackend::buildRaw(H, raw, err)`
- OGDF 개발용 backend는 여기서 끝난다.

### static mini pipeline
- `materializeMiniCore(H, raw, mini, why)`
- `normalizeWholeMiniCore(mini)`
- `checkOwnershipConsistency(mini, H, why)`
- `checkReducedInvariant(mini, H, why)`

### dummy graft pipeline
- `buildDummyActualCoreEnvelope(H)`
- `chooseKeepMiniNode(mini)`
- `graftMiniCoreIntoPlace(core, oldR, H, mini, keep, q, &trace)`
- `checkReducedInvariantActual(core, why, &stubNodes)`
- `checkDummyRealEdgeSet(...)`
- `checkDummyProxyRewire(...)`

## main() 연결 위치

이 bundle의 `src/rewrite_r_harness_main.cpp`는 standalone CLI skeleton이다.
실제 프로젝트에서는 아래 둘 중 하나로 연결하면 된다.

1. 이 파일 안에서 실제 harness 함수를 include 해서 직접 호출
2. 이 파일을 참고해 프로젝트의 기존 test binary main으로 옮기기

## 가장 안전한 실행 순서

1. static runner manual-only
2. static runner random 1000
3. dummy runner manual-only
4. dummy runner random 1000
5. 그 다음에만 rewriteR_fallback + normalizeTouchedRegion
