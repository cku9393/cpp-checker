# OGDF wrapper 채우기 체크리스트

## 목표
`buildRawSpqrDecompWithOgdf()`가 `RawSpqrDecomp`를 안정적으로 만들도록 wrapper 5개를 채운다.

## 채워야 하는 함수 5개

1. `forEachSkeletonEdge`
2. `originalOfSkeletonNode`
3. `polesOfSkeletonEdge`
4. `computeCycleOrderFromSkeleton`
5. `buildRShapeFromSkeleton`

## 권장 원칙

- 버전 번호 분기보다 feature 분기를 우선한다.
- `adjEdges` / `adjEntries` 차이는 wrapper macro에서만 처리한다.
- tree-edge 매핑은 `skeletonEdgeSrc/Tgt()`를 primary로 사용한다.
- `StaticSkeleton::treeEdge(edge)`는 debug cross-check 용도로만 쓴다.

## 함수별 체크

### 1) forEachSkeletonEdge
- `Skeleton::getGraph()`가 되는지
- `GS.edges` range iteration이 되는지
- 안 되면 `firstEdge()/succ()` fallback 사용

### 2) originalOfSkeletonNode
- `Skeleton::original(node)`가 original graph node를 주는지
- `BuildContext.origVertexOfGraphNode`에서 원래 vertex id를 제대로 읽는지

### 3) polesOfSkeletonEdge
- `eSk->source()/target()`를 skeleton node로 읽는지
- `originalOfSkeletonNode()`를 통해 pole pair를 얻는지

### 4) computeCycleOrderFromSkeleton
- S skeleton의 모든 정점 degree가 2인지 assert
- edge 수와 cycleSlots 길이가 같은지 assert
- slot 중복이 없는지 assert
- `adjEntries` 기준으로 다음 edge를 고르는지

### 5) buildRShapeFromSkeleton
- skeleton local vertex id를 stable하게 부여하는지
- slotOfSkel가 skeleton edge마다 정확히 1개씩 대응되는지
- `endsOfSlot`와 `incSlots`가 slot 수와 맞는지

## 첫 성공 조건

- `runStaticPipelineCaseDumpAware`에서 manual-only 케이스가 통과
- `RAW_VALIDATE_FAIL`가 더 이상 안 뜸
