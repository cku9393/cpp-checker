# branch_3 Archive Index

`boj28350_bundle_archive/` is the extracted archive root of
`boj28350_current_state_bundle.zip`.

Contents:

- `boj28350_literature_progress*.cpp`
  - archived solver snapshots by progress number
- `boj28350_progress*_report.md`
  - archived reports
- `boj28350_progress*_results*.json`
  - archived merged results
- `progress*_resume_remaining.sh`, `run_progress*_case_*.py`
  - original session scripts from the bundle
Important:

- many archive scripts still use the original `/mnt/data/...` paths from the
  captured environment
- the duplicated inner `lca_tree_stress_v5.zip` was removed after extraction
  cleanup because the workspace now reuses the outer repo-root
  `lca_tree_stress_v5/`
- current branch-local work should use `branch_3/boj28350_resume/` and
  `branch_3/boj28350_resume.py`
- the active baseline copied out of this archive is
  `boj28350_literature_progress40_layout_signature_reuse_gate.cpp`
