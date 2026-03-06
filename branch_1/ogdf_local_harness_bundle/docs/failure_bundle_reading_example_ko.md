# first failure bundle 읽는 순서 예시

## 원칙
항상 가장 이른 stage만 본다.

예:
- `RAW_VALIDATE_FAIL`이면 `mini/actual/trace`는 보지 않는다.
- `MINI_POSTCHECK_FAIL`이면 `actual/trace`는 보지 않는다.
- `DUMMY_PROXY_REWIRE_FAIL`이면 backend/raw/mini는 이미 통과했다고 본다.

---

## 예시 1: RAW_VALIDATE_FAIL

### 먼저 읽을 줄
1. `stage=RAW_VALIDATE_FAIL`
2. `where=`
3. `why=`

### 다음으로 볼 섹션
- `=== CompactGraph ===`
- `=== RawSpqrDecomp ===`

### 확인 순서
1. `ownerOfInputEdge`가 전부 채워졌는가
2. `treeEdge t`의 `slotInA/slotInB`가 실제 `TREE_EDGE` slot인가
3. S node면 `cycleSlots` 길이와 중복 여부
4. P node면 pole pair 동일성
5. R node면 `endsOfSlot` / `incSlots`

---

## 예시 2: MINI_POSTCHECK_FAIL

### 먼저 읽을 줄
1. `stage=MINI_POSTCHECK_FAIL`
2. `where=`
3. `why=`

### 다음으로 볼 섹션
- `=== MiniBeforeNormalize ===`
- `=== MiniAfterNormalize ===`

### 확인 순서
1. adjacent S-S / P-P가 남았는가
2. dead relay가 남았는가
3. ownerOfInputEdge stale entry가 있는가
4. REAL_INPUT / PROXY_INPUT이 사라졌는가

---

## 예시 3: DUMMY_PROXY_REWIRE_FAIL

### 먼저 읽을 줄
1. `stage=DUMMY_PROXY_REWIRE_FAIL`
2. `where=`
3. `why=`

### 다음으로 볼 섹션
- `=== CompactGraph ===`
- `=== ActualAfterGraft ===`
- `=== GraftTrace ===`

### 확인 순서
1. 각 PROXY edge의 `oldArc`가 alive인가
2. `oldArc`가 `stub <-> actualOfMini[ownerMini]`를 잇는가
3. actual endpoint slot의 `arcId`와 pole pair가 맞는가
4. stub endpoint slot은 그대로 유지됐는가
