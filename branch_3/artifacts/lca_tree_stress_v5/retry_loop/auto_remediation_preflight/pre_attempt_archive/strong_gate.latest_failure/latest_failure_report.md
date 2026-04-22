# lca_strong_gate Failure Report

- Stage: `certify`
- Exit code: `1`
- Message: `certify suite failed`
- Output root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate`
- Failure root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure`
- Workdir: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/.tmp/lca_strong_gate.run.tuq6n2`
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
| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 12 | - | - | - | - | - |
| caterpillar_rect_dense | 12 | - | - | - | - | - |
| comb_core | 0 | 0.962 | 2.332 | 0.210 | 0.210 | 4096:0.030, 8192:0.045, 16384:0.105, 32768:0.210 |
| comb_dense | 12 | - | - | - | - | - |
| comb_plus_unary | 0 | 1.915 | 4.173 | 6.402 | 6.439 | 4096:0.119, 8192:0.405, 16384:1.534, 32768:6.402 |
| comb_rect_dense | 12 | - | - | - | - | - |
| multi_comb_cap | 9 | - | - | 7.404 | 7.597 | 4096:7.404 |
| multi_comb_core | 0 | 1.985 | 4.168 | 3.814 | 3.915 | 4096:0.060, 8192:0.251, 16384:0.927, 32768:3.814 |
| multi_comb_rect | 12 | - | - | - | - | - |

Scale check hits:

- comb_plus_unary: alpha=1.915 > 1.450
- comb_plus_unary: ratio=4.173 > 2.900
- multi_comb_core: alpha=1.985 > 1.450
- multi_comb_core: ratio=4.168 > 2.900

## Stage: max_n_mix

status: **FAIL**  
cases: 28  
timeouts: 24  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 2 | - | - | 5.167 | 5.191 | 50000:5.167 |
| caterpillar_rect_dense | 4 | - | - | - | - | - |
| comb_dense | 4 | - | - | - | - | - |
| comb_rect_dense | 4 | - | - | - | - | - |
| multi_comb_cap | 4 | - | - | - | - | - |
| multi_comb_rect | 4 | - | - | - | - | - |
| random_recursive_mixed | 2 | - | - | 2.865 | 3.101 | 50000:2.865 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | multi_comb_cap | 4096 | 3 | 7.597 | 719344 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_cap/n4096/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 4096 | 2 | 7.404 | 719872 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_cap/n4096/seed2_L1_Q1 |
| hard_scaling | multi_comb_cap | 4096 | 1 | 7.004 | 718400 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_cap/n4096/seed1_L1_Q1 |
| hard_scaling | comb_plus_unary | 32768 | 2 | 6.439 | 1512768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/comb_plus_unary/n32768/seed2_L1_Q1 |
| hard_scaling | comb_plus_unary | 32768 | 1 | 6.402 | 1512704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/comb_plus_unary/n32768/seed1_L1_Q1 |
| hard_scaling | comb_plus_unary | 32768 | 3 | 6.019 | 1512640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/comb_plus_unary/n32768/seed3_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 1 | 5.191 | 993040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/balanced_dense/n50000/seed1_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 2 | 5.144 | 967936 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/balanced_dense/n50000/seed2_L1_Q1 |
| hard_scaling | multi_comb_core | 32768 | 3 | 3.915 | 456928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_core/n32768/seed3_L1_Q1 |
| hard_scaling | multi_comb_core | 32768 | 2 | 3.814 | 457664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_core/n32768/seed2_L1_Q1 |
| hard_scaling | multi_comb_core | 32768 | 1 | 3.661 | 456832 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_core/n32768/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 50000 | 1 | 3.101 | 407728 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/random_recursive_mixed/n50000/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 50000 | 2 | 2.628 | 404624 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/random_recursive_mixed/n50000/seed2_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 2.006 | 558736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.979 | 550304 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.951 | 556944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.908 | 512800 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.906 | 548608 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.899 | 524400 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.884 | 524448 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
```
