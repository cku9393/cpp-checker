# BOJ 28350 stress loop, upgraded

이 폴더는 다음 루프를 한 번에 돌리기 위한 패키지다.

1. 다양한 숨은 트리를 기반으로 유효한 반례 입력 생성
2. `solve.cpp` 실행
3. 출력 트리가 실제로 쿼리를 만족하는지 검증
4. 여러 모드와 크기에서 성능 경향 확인
5. 특정 모드에서 어느 `N`부터 위험해지는지 breakpoint 탐색

## 핵심 파일

- `gen_case.py`
  - 디테일한 반례 생성기
  - 모든 모드는 실제 숨은 트리를 먼저 만들고, 그 트리에서 유도한 **항상 유효한** LCA 쿼리만 출력한다
  - 옵션으로 메타 정보와 숨은 부모 배열도 저장 가능
- `validator.py`
  - 출력 부모 배열이 루트가 1인 올바른 트리인지 확인
  - 모든 쿼리를 다시 LCA로 검증
- `solve.cpp`
  - 현재 테스트용 솔버 자리
  - 원하는 코드로 교체해서 사용하면 된다
- `run_case.sh`
  - 단일 케이스 생성 -> 실행 -> 검증 -> 시간/메모리 출력
- `stress_modes.sh`
  - 여러 반례 모드를 일괄 실행
- `find_breakpoint.py`
  - 특정 모드에서 timeout 기준으로 어디까지 버티는지 자동 탐색

## 생성 모드

`python3 gen_case.py --list-modes`

현재 포함된 주요 모드:

- `comb_core`
  - 느리게 줄어드는 comb 계열 기본형
  - 분할이 절반 가까이 줄지 않는 구조를 가장 직접적으로 찌른다
- `comb_plus_unary`
  - `comb_core`에 unary ancestor 제약을 추가한 형태
  - 큰 쿼리 묶음이 여러 단계에 걸쳐 남는지 보기 좋다
- `comb_dense`
  - comb 계열을 더 조밀하게 채운 버전
  - 쿼리 스캔 상수까지 같이 압박한다
- `chain_unary`
  - path 트리 기반 unary-only 테스트
  - ancestor 처리 로직 검증용
- `star_pairs`
  - star 트리에서 leaf pair를 많이 넣은 케이스
  - root branching 처리, dense M 검증용
- `balanced_sibling`
  - balanced binary tree에서 각 내부 노드의 서로 다른 자식 서브트리 대표를 짝짓는 구조
- `balanced_dense`
  - balanced tree에서 branching query를 더 많이 넣은 버전
- `broom_mixed`
  - 긴 handle + 빽빽한 broom head
- `caterpillar_mixed`
  - 긴 spine + side leaf 혼합
- `random_recursive_mixed`
  - 랜덤 recursive tree에서 여러 유형의 유효 쿼리 혼합

## 빠른 사용법

```bash
cd lca_tree_stress_v2
./build.sh
./run_case.sh comb_core 99999 1 1 1
./run_case.sh comb_dense 99999 1 1 1
```

`run_case.sh` 인자 순서:

```text
MODE N SEED SHUFFLE_LABELS SHUFFLE_QUERIES [SOLVER] [OUTDIR]
```

예시:

```bash
./run_case.sh balanced_dense 50000 7 1 1 ./solve _runs/balanced_dense_50000
```

## 일괄 스트레스

```bash
./stress_modes.sh ./solve 1 1 1
```

인자 순서:

```text
SOLVER SEED SHUFFLE_LABELS SHUFFLE_QUERIES
```

## breakpoint 탐색

아래 명령은 특정 모드에서 timeout 2초 기준으로 어디까지 버티는지 찾는다.

```bash
python3 find_breakpoint.py --solver ./solve --mode comb_dense --timeout 2.0 --shuffle-labels --shuffle-queries
```

여러 seed를 함께 보고 싶으면:

```bash
python3 find_breakpoint.py --solver ./solve --mode comb_plus_unary --timeout 2.0 --seeds 1,2,3 --shuffle-labels --shuffle-queries
```

## CSV/표 벤치마크 리포트

여러 모드/크기/seed를 한 번에 돌리고, 결과를 CSV와 Markdown 표로 저장한다.

```bash
python3 bench_report.py --solver ./solve --out bench_out \
  --sizes 9999,19999,39999,79999,99999 --seeds 1,2,3 \
  --shuffle-labels --shuffle-queries --timeout 2.0
```

출력:

- `bench_out/bench.csv`: 전체 실행 로그(행 단위)
- `bench_out/bench_pivot.csv`: (mode,n)별 seed 최솟값 요약
- `bench_out/bench_summary.md`: 사람 보기 좋은 표
- 기본값은 모든 실행이 성공하면 `bench_out/runs/` 아티팩트를 정리한다.
- 아티팩트를 항상 보존하려면 `--keep`을 추가한다.

짧은 래퍼 스크립트도 있다:

```bash
./bench_report.sh ./solve bench_out 9999,19999,39999 1,2,3 2.0 1 1 1
```

`bench_report.sh` 인자 순서:

```text
SOLVER OUT SIZES SEEDS TIMEOUT SHUFFLE_LABELS SHUFFLE_QUERIES [MODES|KEEP] [KEEP]
```

- 8번째 인자가 `0` 또는 `1`이면 `KEEP`으로 해석된다.
- `MODES`와 `KEEP`을 같이 주고 싶으면 9번째 인자까지 전달하면 된다.

## 숨은 트리도 같이 저장하고 싶을 때

```bash
python3 gen_case.py --mode comb_dense --n 99999 --seed 1 \
  --shuffle-labels --shuffle-queries \
  --meta meta.json --parent-out hidden_parent.txt > in.txt
```

## 팁

- decomposition 계열 최악 성능을 보려면 `comb_core`, `comb_plus_unary`, `comb_dense`부터 보는 게 좋다.
- 쿼리 유형이 다양해도 맞는지 보려면 `balanced_dense`, `broom_mixed`, `random_recursive_mixed`를 같이 돌리면 좋다.
- label/query shuffle을 켜면 hard-coded 분기나 입력 순서 의존을 좀 더 잘 잡아낼 수 있다.
