# 첫 failure bundle 읽는 순서 예시

## RAW_VALIDATE_FAIL
1. `CompactGraph`의 edge 수 / self-loop / biconnected 전제 확인
2. `RawSpqrDecomp.ownerOfInputEdge` 전부 채워졌는지 확인
3. `treeEdges[t].slotInA/slotInB`가 실제 TREE slot인지 확인
4. S-node면 `cycleSlots`, R-node면 `endsOfSlot` / `incSlots` 확인

## MINI_POSTCHECK_FAIL
1. `MiniBeforeNormalize`와 `MiniAfterNormalize` 비교
2. adjacent `S-S` / `P-P`가 남았는지 확인
3. `REAL_INPUT`, `PROXY_INPUT` 소유권이 유지되는지 확인

## DUMMY_PROXY_REWIRE_FAIL
1. `CompactGraph`의 `oldArc/outsideNode/oldSlotInU`
2. `ActualAfterGraft`에서 oldArc가 alive인지
3. `trace.actualOfMini[ownerMini]`와 proxy arc endpoint가 일치하는지
