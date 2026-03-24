# Round 45 Resume Workspace

이 디렉터리는 `branch_2_2` 안에서만 쓰는 Round 45 재개용 작업 공간이다.

- `round45_branch_2_2_solver.cpp`
  - 현재 `branch_2_2`에서 직접 수정하는 active solver source.
  - `python3 ../round45_resume.py build`는 이 파일을 컴파일한다.
- `reference_flatten_spqr_current_merged.cpp`
  - handoff 문서에서 기준선으로 언급한 merged source reference.
  - 실제 수정 기준과 비교할 때 옆에 두고 본다.
- `../round45_bundle_archive/solve.cpp`
  - 보관용 historical baseline.
  - 현재 실행 대상은 아니고, 비교가 필요할 때만 참조한다.
- `smoke_cases.tsv`
  - Round 45 smoke gate 재개용 기본 케이스 목록.

실행 순서는 아래를 기본으로 한다.

```bash
cd branch_2_2
python3 round45_resume.py build
./run.sh < in.txt > out.txt
python3 round45_resume.py smoke
```

기본 smoke 출력은 `artifacts/round45_resume/smoke` 아래에 쌓인다.
핵심 확인값은 `smoke_summary.tsv`의 `census_rows`, `candidate_rows`, `prefilter_rows`, `decompose_rows`, `row_gate_passed`다.

이 workspace의 목표는 두 가지다.

1. `branch_2_2` 내부 하네스를 유지한 채 Round 45 baseline을 독립적으로 빌드/실행하기
2. missing instrumentation 복구 전에도 smoke gate blocker를 같은 환경에서 다시 재현하고, 이후 패치를 같은 자리에서 누적하기
