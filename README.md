# BOJ 28350 stress suite v5

이 폴더는 `쿼리와 트리 2` 풀이를 **강한 반례군 + 검증 + 성능 리포트 + 인증형 게이트**로 평가하기 위한 패키지다.

핵심 목표는 두 가지다.

1. 정답성 검증
2. 느린 decomposition 계열 코드가 약한 데이터에서만 통과하는지 강하게 가려내기

중요한 점:

이 스위트가 hidden data를 그대로 복제하는 **수학적 증명**은 아니다.
하지만 `comb / multi_comb / caterpillar` 계열의 느린 구조와 `max-N` dense 케이스, 그리고 스케일링 분석까지 묶어서 평가하기 때문에,
이걸 통과하면 **백준 통과 가능성에 대한 강한 증거**가 되고,
동시에 “약한 데이터라서 통과했다”는 지적도 **매우 강하게 반박**할 수 있게 설계했다.

## 새로 강화된 핵심

이번 버전은 단순 케이스 추가가 아니라, **인증형 구조**로 바뀌었다.

### 1) 더 강한 반례 모드

기존 모드에 더해서 아래 hard family를 추가했다.

- `comb_rect_dense`
  - comb 계열에서 각 side leaf를 여러 깊이의 descendant와 엮어 long-lived query를 많이 만든다.
- `multi_comb_core`
  - heavy spine에 각 레벨마다 여러 side leaf가 달린 더 강한 comb 변형.
- `multi_comb_rect`
  - `multi_comb_core`에 cross-depth rectangular 쿼리를 얹은 강화형.
- `multi_comb_cap`
  - `multi_comb_rect`를 M cap 근처까지 공격적으로 채우는 버전.
- `caterpillar_rect_dense`
  - caterpillar 계열에서 여러 깊이로 long-lived 쿼리를 퍼뜨리는 강화형.

이 모드들은 “큰 컴포넌트가 거의 줄지 않는 구조”와 “같은 쿼리 묶음이 여러 단계에 남는 구조”를 더 세게 찌른다.

### 2) `certify_suite.py`

단순 벤치마크가 아니라, 아래를 한 번에 판단한다.

- 모든 케이스 validator 통과 여부
- timeout / RE / WA 여부
- hard family에서 크기 증가에 따른 **log-log slope(alpha)**
- 크기 두 배 근처에서의 **worst growth ratio**
- stage별 PASS / FAIL / WARN
- 전체 verdict

즉, “몇 초 걸렸다”만 보는 게 아니라,
**정말 scaling이 건강한지**까지 본다.

### 3) `rebuttal_gate.json`, `strong_gate.json`

바로 쓸 수 있는 인증 preset을 넣었다.

- `smoke.json`
  - 빠른 sanity check
- `rebuttal_gate.json`
  - 느린 decomposition 계열 지적을 반박하기 위한 hard-mode 중심 gate
- `strong_gate.json`
  - correctness fuzz + hard scaling + max-N dense run을 모두 보는 종합 gate
- `boj_3s_hard_gate.json`
  - adversarial family + max-N 케이스에 대해 **개별 케이스 sub-3s cap**까지 거는 더 빡센 BOJ 지향 gate

### 4) `hunt_hardest.py`

모드/크기/seed/셔플 조합을 훑어서 **현재 solver 기준 가장 느린 케이스**를 자동으로 골라준다.

즉, 단순히 정해진 세트만 돌리는 게 아니라,
**현재 코드가 실제로 어디서 약한지**를 상위 케이스로 바로 볼 수 있다.

## 파일 구성

- `gen_case.py`
  - 유효한 LCA 쿼리 입력 생성기
- `validator.py`
  - 출력 트리와 모든 쿼리의 LCA를 재검증
- `bench_report.py`
  - CSV / Markdown 리포트 생성
- `find_breakpoint.py`
  - timeout 기준으로 버티는 최대 N 탐색
- `certify_suite.py`
  - stage/preset 기반 인증형 실행기
- `hunt_hardest.py`
  - 현재 solver 기준 최악 케이스 hunt
- `suite_presets/*.json`
  - smoke / rebuttal / strong gate preset
- `gate.sh`
  - 일반 preset용 `certify_suite.py` 래퍼
- `gate_boj3s.sh`
  - `boj_3s_hard_gate.json` 전용 래퍼
- `hunt.sh`
  - `hunt_hardest.py` 래퍼
- `stress_modes.sh`
  - 여러 모드 일괄 실행
- `solve.cpp`
  - 테스트할 솔버 자리

## 플랫폼별 진입점

- macOS / Linux
  - `build.sh`, `run_case.sh`, `stress_modes.sh`, `gate.sh`, `gate_boj3s.sh`, `hunt.sh`, `bench_report.sh`
- Windows PowerShell
  - `build.ps1`, `run_case.ps1`, `stress_modes.ps1`, `gate.ps1`, `gate_boj3s.ps1`, `hunt.ps1`, `bench_report.ps1`
- 공통 사항
  - Python 기반 실행 코어는 `.py` 파일들이다.
  - 기본 solver 이름은 macOS / Linux에서 `solve`, Windows에서 `solve.exe`다.
  - `build.py`는 사용 가능한 C++ 컴파일러를 자동 탐지한다.
  - Linux에서 정적 링크가 꼭 필요하면 `build.py --static always`를 쓰면 된다.

## 생성 모드 목록

```bash
python gen_case.py --describe-modes
```

현재 주요 모드:

- `comb_core`
- `comb_plus_unary`
- `comb_dense`
- `comb_rect_dense`
- `multi_comb_core`
- `multi_comb_rect`
- `multi_comb_cap`
- `chain_unary`
- `star_pairs`
- `balanced_sibling`
- `balanced_dense`
- `broom_mixed`
- `caterpillar_mixed`
- `caterpillar_rect_dense`
- `random_recursive_mixed`

## 빠른 시작

```bash
cd lca_tree_stress_v5
./build.sh
```

```powershell
Set-Location lca_tree_stress_v5
./build.ps1
```

단일 케이스:

```bash
./run_case.sh comb_rect_dense 99999 1 1 1
./run_case.sh multi_comb_cap 99999 1 1 1
```

```powershell
./run_case.ps1 comb_rect_dense 99999 1 1 1
./run_case.ps1 multi_comb_cap 99999 1 1 1
```

PowerShell에서는 같은 인자 구조로 `.ps1` 래퍼를 쓰면 된다. 직접 solver 경로를 넘길 때는 보통 `.\solve.exe`를 사용한다.

## 인증형 게이트

### 1) 종합 gate

```bash
./gate.sh ./solve suite_presets/strong_gate.json gate_out
```

### 2) 느린 데이터 반박용 gate

```bash
./gate.sh ./solve suite_presets/rebuttal_gate.json rebuttal_out
```

### 3) 더 느린 머신이면 time-based limit를 배수 조정

```bash
./gate.sh ./solve suite_presets/strong_gate.json gate_out 1.5
```

출력:

- `gate_out/certify_rows.csv`
- `gate_out/certify_summary.md`
- `gate_out/certify.json`

`certify_summary.md`에는 stage별 PASS/FAIL, scaling alpha, worst growth ratio, top slow cases가 정리된다.

### 4) BOJ 3초 지향 hard gate

```bash
./gate_boj3s.sh ./solve boj3s_out
```

또는 직접 preset 지정:

```bash
./gate.sh ./solve suite_presets/boj_3s_hard_gate.json boj3s_out
```

이 gate는 기존 `strong_gate`보다 더 엄격하다.

- large adversarial family를 직접 포함
- `hard_scaling_strict`, `boj_3s_large_adversarial`, `boj_3s_large_mix`는 `timeout=3.0`
- `correctness_smoke`는 빠른 정답성 확인용으로 `timeout=1.5`
- `case_sec_max`로 **개별 케이스 최대 시간**까지 제한
- `sec_max`로 size별 median 관점에서도 여유(headroom) 확인

실행 시간 `sec`과 메모리 `rss_kb`는 이제 공통 실행 코어가 직접 기록한다.
macOS / Linux에서는 프로세스 사용량을 직접 수집하고, Windows에서는 가능한 범위에서 peak working set을 기록한다.

주의할 점:

이 gate를 통과했다고 해서 BOJ hidden을 **수학적으로 증명**하는 것은 아니다.
하지만 “약한 데이터라서 통과한 것 아닌가?”라는 의심을 반박하는 용도로는 기존보다 훨씬 강하다.

로컬 머신이 느리면 마지막 인자 `LIMIT_SCALE`을 올릴 수 있다.
이 값은 `timeout`, `sec_max`, `case_sec_max`에 같이 적용된다.

```bash
./gate_boj3s.sh ./solve boj3s_out 1.15
```

## hardest-case hunt

```bash
./hunt.sh ./solve hunt_out 12000,24000,48000,99999 1,2,3 8.0
```

PowerShell 예시:

```powershell
./gate.ps1 .\solve.exe suite_presets/strong_gate.json gate_out
./gate_boj3s.ps1 .\solve.exe boj3s_out
./hunt.ps1 .\solve.exe hunt_out 12000,24000,48000,99999 1,2,3 8.0
```

출력:

- `hunt_out/hunt.csv`
- `hunt_out/hunt_summary.md`

이걸로 현재 코드가 실제로 어느 family/seed/셔플 조합에서 가장 느린지 바로 볼 수 있다.

## 일반 벤치 리포트

```bash
python bench_report.py --solver ./solve --out bench_out \
  --sizes 9999,19999,39999,79999,99999 --seeds 1,2,3 \
  --shuffle-labels --shuffle-queries --timeout 4.0
```

```powershell
./bench_report.ps1 .\solve.exe bench_out 9999,19999,39999,79999,99999 1,2,3 4.0 1 1
```

출력:

- `bench_out/bench.csv`
- `bench_out/bench_pivot.csv`
- `bench_out/bench_summary.md`

## breakpoint 탐색

```bash
python find_breakpoint.py --solver ./solve --mode multi_comb_cap \
  --timeout 2.0 --shuffle-labels --shuffle-queries
```

```powershell
python .\find_breakpoint.py --solver .\solve.exe --mode multi_comb_cap `
  --timeout 2.0 --shuffle-labels --shuffle-queries
```

## hidden tree도 저장

```bash
python gen_case.py --mode comb_rect_dense --n 99999 --seed 1 \
  --shuffle-labels --shuffle-queries \
  --meta meta.json --parent-out hidden_parent.txt > in.txt
```

```powershell
python .\gen_case.py --mode comb_rect_dense --n 99999 --seed 1 `
  --shuffle-labels --shuffle-queries `
  --meta meta.json --parent-out hidden_parent.txt > in.txt
```

## 추천 사용 순서

가장 실전적인 루프는 이렇다.

1. `./build.sh`
2. `./gate.sh ./solve suite_presets/strong_gate.json gate_out`
3. `./gate.sh ./solve suite_presets/rebuttal_gate.json rebuttal_out`
4. `./gate_boj3s.sh ./solve boj3s_out`
5. `./hunt.sh ./solve hunt_out`

이 순서로 보면,
정답성 문제인지,
hard family scaling 문제인지,
max-N headroom 문제인지,
가장 느린 실제 케이스가 뭔지까지 한 번에 잡힌다.

## 해석 팁

- `PASS`
  - 정답성 + hard scaling + dense max-N run까지 preset 기준 통과
- `WARN`
  - soft stage나 보조 기준에서만 이상
- `FAIL`
  - timeout / WA / RE / scaling blow-up 등 명확한 문제 존재

특히 `rebuttal_gate.json`에서 fail이 나면,
느린 decomposition 계열 지적을 반박하기 어렵다.
반대로 이 gate를 통과하면 그 지적은 상당 부분 무력화된다.
