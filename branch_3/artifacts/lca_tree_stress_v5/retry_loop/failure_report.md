# Failure Report: Attempt 999

- Timestamp: `2026-03-25 12:29:34 KST`
- Seed: `.ouroboros/seed_branch3_progress40_research_loop.yaml`
- Exit code: `1`
- Session ID: `unknown`
- Execution ID: `unknown`
- Analysis state file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/.ouroboros/failure_analysis_state.json`
- Analysis state revision: `0`

## Result Summary

```text
[2026-03-25 12:22:38 KST] attempt 1 start: .ouroboros/seed_branch3_progress40_research_loop.yaml
```

## Parsed AC Verdicts

- Failed ACs: none found
- Blocked ACs: none found
- Passed ACs: none found

## Git Status At Failure

```text
git status skipped: timed out after 10s
```

## Relevant Artifact Snapshots

### smoke

- Latest file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/smoke/smoke_random_recursive_mixed_128_s1/run_case.stdout.txt`
- Latest mtime: `2026-03-25 11:33:51 KST`
- Summary file: `none`

### strong_gate

- Latest file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/certify.json`
- Latest mtime: `2026-03-25 00:32:23 KST`
- Summary file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/certify.json`

```text
{
  "verdict": "PASS",
  "reasons": [],
  "preset": "strong_gate",
  "stages": [
    {
      "name": "correctness_fuzz",
      "status": "PASS",
      "cases": 900,
      "timeouts": 0,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": null,
      "scale_fail": []
    },
    {
      "name": "hard_scaling",
      "status": "PASS",
      "cases": 108,
      "timeouts": 0,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": null,
      "scale_fail": []
    },
    {
      "name": "max_n_mix",
      "status": "PASS",
      "cases": 28,
      "timeouts": 0,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": null,
      "scale_fail": []
    }
  ]
}
```

### boj3s_gate

- Latest file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current.latest_failure/certify.json`
- Latest mtime: `2026-03-25 07:18:29 KST`
- Summary file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/boj3s_gate/retry_loop/ac5_formal_attempt_current.latest_failure/certify.json`

```text
{
  "verdict": "FAIL",
  "reasons": [
    "correctness_smoke: 64 failing cases",
    "hard_scaling_strict: 99 failing cases",
    "boj_3s_large_adversarial: 30 failing cases",
    "boj_3s_large_mix: 18 failing cases"
  ],
  "preset": "boj_3s_hard_gate",
  "stages": [
    {
      "name": "correctness_smoke",
      "status": "FAIL",
      "cases": 288,
      "timeouts": 24,
      "re_wa": 40,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": null,
      "scale_fail": []
    },
    {
      "name": "hard_scaling_strict",
      "status": "FAIL",
      "cases": 108,
      "timeouts": 99,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": 2.7,
      "scale_fail": []
    },
    {
      "name": "boj_3s_large_adversarial",
      "status": "FAIL",
      "cases": 30,
      "timeouts": 30,
      "re_wa": 0,
      "limit_scale": 1.0,
      "sec_max": 2.55,
      "case_sec_max": 2.7,
      "scale_fail": []
    },
    {
      "name": "boj_3s_large_mix",
      "status": "FAIL",
      "cases": 18,
      "timeouts": 15,
      "re_wa": 3,
      "limit_scale": 1.0,
      "sec_max": null,
      "case_sec_max": 2.7,
      "scale_fail": []
    }
  ]
}
```

### hunt

- Latest file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/hunt_summary.md`
- Latest mtime: `2026-03-25 03:18:57 KST`
- Summary file: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/hunt_summary.md`

```text
# Hardest-case hunt

상위 케이스는 현재 solver 기준으로 가장 느리게 측정된 조합이다. 느린 풀이를 반박하려면 이 목록에서 timeout/scale 문제가 없어야 한다.

| rank | mode | n | seed | L | Q | sec | rss_kb | val_ok | case_dir |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | caterpillar_rect_dense | 64 | 1 | 1 | 0 | 0.079 | 4496 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_rect_dense/n64/seed1_L1_Q0 |
| 2 | comb_rect_dense | 64 | 1 | 1 | 0 | 0.075 | 4400 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_rect_dense/n64/seed1_L1_Q0 |
| 3 | comb_dense | 64 | 1 | 1 | 1 | 0.074 | 4464 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_dense/n64/seed1_L1_Q1 |
| 4 | chain_unary | 64 | 1 | 1 | 1 | 0.073 | 2784 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/chain_unary/n64/seed1_L1_Q1 |
| 5 | comb_dense | 64 | 1 | 1 | 0 | 0.072 | 4432 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_dense/n64/seed1_L1_Q0 |
| 6 | caterpillar_rect_dense | 64 | 1 | 1 | 1 | 0.066 | 4432 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_rect_dense/n64/seed1_L1_Q1 |
| 7 | comb_rect_dense | 64 | 1 | 1 | 1 | 0.061 | 4416 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_rect_dense/n64/seed1_L1_Q1 |
| 8 | caterpillar_rect_dense | 64 | 1 | 0 | 1 | 0.059 | 3744 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_rect_dense/n64/seed1_L0_Q1 |
| 9 | comb_rect_dense | 64 | 1 | 0 | 1 | 0.054 | 3776 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_rect_dense/n64/seed1_L0_Q1 |
| 10 | comb_dense | 64 | 1 | 0 | 0 | 0.050 | 3744 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_dense/n64/seed1_L0_Q0 |
| 11 | caterpillar_mixed | 64 | 1 | 1 | 0 | 0.049 | 4464 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_mixed/n64/seed1_L1_Q0 |
| 12 | caterpillar_mixed | 64 | 1 | 1 | 1 | 0.047 | 4464 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_mixed/n64/seed1_L1_Q1 |
| 13 | balanced_dense | 64 | 1 | 1 | 0 | 0.046 | 2880 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/balanced_dense/n64/seed1_L1_Q0 |
| 14 | comb_rect_dense | 64 | 1 | 0 | 0 | 0.044 | 3728 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/comb_rect_dense/n64/seed1_L0_Q0 |
| 15 | balanced_sibling | 64 | 1 | 1 | 1 | 0.042 | 2928 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/balanced_sibling/n64/seed1_L1_Q1 |
| 16 | caterpillar_mixed | 64 | 1 | 0 | 1 | 0.040 | 3824 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/caterpillar_mixed/n64/seed1_L0_Q1 |
| 17 | multi_comb_rect | 64 | 1 | 0 | 1 | 0.039 | 3296 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/multi_comb_rect/n64/seed1_L0_Q1 |
| 18 | balanced_dense | 64 | 1 | 1 | 1 | 0.038 | 2912 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/balanced_dense/n64/seed1_L1_Q1 |
| 19 | broom_mixed | 64 | 1 | 0 | 0 | 0.038 | 3120 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/broom_mixed/n64/seed1_L0_Q0 |
| 20 | multi_comb_rect | 64 | 1 | 1 | 1 | 0.038 | 3520 | 1 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hunt/ac9_diag/runs/multi_comb_rect/n64/seed1_L1_Q1 |
```

## Session Log Excerpt

```text
(no session-specific log lines found)
```

## Workflow Log Tail

```text
[2026-03-25 12:22:38 KST] attempt 1 start: .ouroboros/seed_branch3_progress40_research_loop.yaml
```

See `failure_breakdown.md` for the per-AC phase split, structural hotspot analysis, and the refinement notes to carry into the next retry.