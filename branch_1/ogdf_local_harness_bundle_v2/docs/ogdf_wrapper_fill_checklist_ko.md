# OGDF wrapper 채우기 체크리스트

먼저 채울 5개:
- `forEachSkeletonEdge`
- `originalOfSkeletonNode`
- `polesOfSkeletonEdge`
- `computeCycleOrderFromSkeleton`
- `buildRShapeFromSkeleton`

원칙:
- 버전 번호 분기보다 **feature macro 분기** 사용
- tree-edge primary mapping은 `skeletonEdgeSrc()/skeletonEdgeTgt()` 사용
- `StaticSkeleton::treeEdge()`는 디버그 assert 용도로만 사용
- `RAW_*` stage가 깨지면 mini/actual은 보지 말 것
