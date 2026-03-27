# BOJ 28350 Resume Workspace

이 디렉터리는 `branch_3` 안에서만 쓰는 BOJ 28350 재개용 작업 공간이다.

- `boj28350_branch_3_solver.cpp`
  - 현재 `branch_3`에서 직접 수정하는 active solver source.
  - 최신 archive 기준 `progress40` 스냅샷을 복사해 왔다.
- `current_state_summary.md`
  - 번들 기준 현재 상태 요약.
- `progress40_derived_reference.md`
  - bundled progress40 source/report/results에서 재사용할 기법, 기각된 접근, benchmark expectation만 추린 branch-local 참조 메모.
- `next_session_briefing.md`
  - 다음 세션 작업 맥락 메모.
  - section 6의 pre-rewrite decision checkpoint를 먼저 통과해야 major solver rewrite/pivot을 시작한다.
  - 조건은 `branch_3` notes review 완료 + bundled `progress40` materials review 완료다.
  - section 6.2.1이 두 review completion이 rewrite/pivot 시작 전에 실제로 기록됐다는 explicit evidence entry다.
- `smoke_cases.tsv`
  - branch-local stress smoke 기본 케이스 목록.

실행 순서는 아래를 기본으로 한다.

```bash
cd branch_3
python3 boj28350_resume.py build
./run.sh < in.txt > out.txt
python3 boj28350_resume.py smoke
```

기본 smoke 출력은 `artifacts/boj28350_resume/smoke` 아래에 쌓인다.
실제 케이스 생성과 validator는 바깥 repo root의 `lca_tree_stress_v5/`를 사용한다.
