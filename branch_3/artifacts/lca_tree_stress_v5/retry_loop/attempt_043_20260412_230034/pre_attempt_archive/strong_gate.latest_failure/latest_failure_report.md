# lca_strong_gate Failure Report

- Stage: `certify`
- Exit code: `1`
- Message: `certify suite failed`
- Output root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate`
- Failure root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure`
- Workdir: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/lca_strong_gate.run.VMGy8P`
- Selected preset source: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.preset_cache/lca_strong_gate.json`
- Selected preset snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/selected_preset.json`
- Solver binary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/boj28350_resume/build/solve`
- Solver build metadata: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/solver_build_meta.json`
- Solver snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/solver_snapshot`

## Recorded Artifacts

- Preflight manifest: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/preflight_manifest.tsv`
- Runtime env snapshot: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/runtime_env.txt`
- Build stdout: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/build.stdout.txt`
- Build stderr: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/build.stderr.txt`
- Certify stdout: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/certify.stdout.txt`
- Certify stderr: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/certify.stderr.txt`
- Certify JSON: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/certify.json`
- Certify summary: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/certify_summary.md`
- Non-artifact tree state: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/non_artifact_tree_current.json`
- Non-artifact tree report: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/non_artifact_tree_report.txt`
- Suite config: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/suite_config.txt`
- Suite plan: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure/suite_plan.tsv`

## Certify Summary Tail

```text
# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 20 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 20  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.865 | 2.248 | 0.141 | 0.148 | 64:0.015, 128:0.015, 256:0.030, 512:0.063, 1024:0.141 |
| broom_mixed | 0 | 1.328 | 3.179 | 0.505 | 0.890 | 64:0.012, 128:0.028, 256:0.059, 512:0.159, 1024:0.505 |
| caterpillar_rect_dense | 20 | 1.727 | 4.340 | 0.550 | 0.566 | 64:0.015, 128:0.041, 256:0.127, 512:0.550 |
| chain_unary | 0 | 1.499 | 3.720 | 0.711 | 0.733 | 64:0.014, 128:0.037, 256:0.131, 512:0.486, 1024:0.711 |
| comb_rect_dense | 0 | 1.632 | 4.206 | 1.346 | 1.449 | 64:0.014, 128:0.037, 256:0.098, 512:0.320, 1024:1.346 |
| multi_comb_cap | 0 | 1.191 | 2.981 | 0.335 | 0.351 | 64:0.012, 128:0.022, 256:0.046, 512:0.112, 1024:0.335 |
| multi_comb_rect | 0 | 1.368 | 3.310 | 0.509 | 0.529 | 64:0.011, 128:0.025, 256:0.055, 512:0.154, 1024:0.509 |
| random_recursive_mixed | 0 | 0.834 | 1.977 | 0.115 | 0.134 | 64:0.013, 128:0.015, 256:0.030, 512:0.058, 1024:0.115 |
| star_pairs | 0 | 1.138 | 3.095 | 0.250 | 0.263 | 64:0.015, 128:0.027, 256:0.084, 512:0.246, 1024:0.250 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | comb_rect_dense | 1024 | 1 | 1.449 | 194752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 3 | 1.440 | 193536 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed3_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 3 | 1.428 | 194176 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed3_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 2 | 1.406 | 192976 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed2_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 2 | 1.404 | 192912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed2_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 1 | 1.388 | 194592 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 2 | 1.385 | 194128 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed2_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 1 | 1.375 | 192848 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 1 | 1.354 | 193888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 3 | 1.352 | 193040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed3_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 2 | 1.341 | 192912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed2_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 4 | 1.306 | 164080 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed4_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 3 | 1.297 | 194800 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed3_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 1.285 | 194544 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 1.265 | 193376 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 1.261 | 194608 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 1.260 | 194624 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 4 | 1.248 | 165024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed4_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 4 | 1.247 | 165712 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed4_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 4 | 1.223 | 165056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed4_L1_Q1 |
```
