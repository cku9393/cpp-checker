# lca_strong_gate Failure Report

- Stage: `certify`
- Exit code: `1`
- Message: `certify suite failed`
- Output root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate`
- Failure root: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate.latest_failure`
- Workdir: `/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/lca_strong_gate.run.bfXm98`
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
| comb_core | 0 | 0.972 | 2.079 | 0.180 | 0.184 | 4096:0.023, 8192:0.047, 16384:0.087, 32768:0.180 |
| comb_dense | 12 | - | - | - | - | - |
| comb_plus_unary | 0 | 1.882 | 3.886 | 5.518 | 5.537 | 4096:0.111, 8192:0.371, 16384:1.420, 32768:5.518 |
| comb_rect_dense | 12 | - | - | - | - | - |
| multi_comb_cap | 9 | - | - | 6.713 | 7.304 | 4096:6.713 |
| multi_comb_core | 0 | 1.858 | 3.718 | 3.936 | 3.939 | 4096:0.083, 8192:0.305, 16384:1.134, 32768:3.936 |
| multi_comb_rect | 12 | - | - | - | - | - |

Scale check hits:

- comb_plus_unary: alpha=1.882 > 1.450
- comb_plus_unary: ratio=3.886 > 2.900
- multi_comb_core: alpha=1.858 > 1.450
- multi_comb_core: ratio=3.718 > 2.900

## Stage: max_n_mix

status: **FAIL**  
cases: 28  
timeouts: 24  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 2 | - | - | 5.249 | 5.271 | 50000:5.249 |
| caterpillar_rect_dense | 4 | - | - | - | - | - |
| comb_dense | 4 | - | - | - | - | - |
| comb_rect_dense | 4 | - | - | - | - | - |
| multi_comb_cap | 4 | - | - | - | - | - |
| multi_comb_rect | 4 | - | - | - | - | - |
| random_recursive_mixed | 2 | - | - | 3.063 | 3.303 | 50000:3.063 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | multi_comb_cap | 4096 | 1 | 7.304 | 702432 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_cap/n4096/seed1_L1_Q1 |
| hard_scaling | multi_comb_cap | 4096 | 3 | 6.713 | 703072 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_cap/n4096/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 4096 | 2 | 6.267 | 706464 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_cap/n4096/seed2_L1_Q1 |
| hard_scaling | comb_plus_unary | 32768 | 3 | 5.537 | 1514976 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/comb_plus_unary/n32768/seed3_L1_Q1 |
| hard_scaling | comb_plus_unary | 32768 | 1 | 5.518 | 1456240 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/comb_plus_unary/n32768/seed1_L1_Q1 |
| hard_scaling | comb_plus_unary | 32768 | 2 | 5.467 | 1514768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/comb_plus_unary/n32768/seed2_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 1 | 5.271 | 923456 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/balanced_dense/n50000/seed1_L1_Q1 |
| max_n_mix | balanced_dense | 50000 | 2 | 5.227 | 924816 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/balanced_dense/n50000/seed2_L1_Q1 |
| hard_scaling | multi_comb_core | 32768 | 2 | 3.939 | 459472 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_core/n32768/seed2_L1_Q1 |
| hard_scaling | multi_comb_core | 32768 | 1 | 3.936 | 460640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_core/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_core | 32768 | 3 | 3.906 | 459472 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/hard_scaling/multi_comb_core/n32768/seed3_L1_Q1 |
| max_n_mix | random_recursive_mixed | 50000 | 1 | 3.303 | 392800 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/random_recursive_mixed/n50000/seed1_L1_Q1 |
| max_n_mix | random_recursive_mixed | 50000 | 2 | 2.823 | 393024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/max_n_mix/random_recursive_mixed/n50000/seed2_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.927 | 505168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.892 | 540000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.880 | 542464 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.879 | 539936 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.851 | 543040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.837 | 506096 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.818 | 514688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
```
