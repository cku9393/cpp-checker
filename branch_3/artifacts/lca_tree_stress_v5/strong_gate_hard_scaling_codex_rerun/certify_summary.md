# Certification summary

overall verdict: **PASS**

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: hard_scaling

status: **PASS**  
cases: 108  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 0 | 0.188 | 1.196 | 0.064 | 0.071 | 4096:0.043, 8192:0.051, 16384:0.056, 32768:0.064 |
| caterpillar_rect_dense | 0 | 0.295 | 1.299 | 0.081 | 0.093 | 4096:0.044, 8192:0.052, 16384:0.063, 32768:0.081 |
| comb_core | 0 | 0.023 | 1.202 | 0.013 | 0.017 | 4096:0.013, 8192:0.011, 16384:0.013, 32768:0.013 |
| comb_dense | 0 | 0.181 | 1.205 | 0.054 | 0.056 | 4096:0.037, 8192:0.039, 16384:0.045, 32768:0.054 |
| comb_plus_unary | 0 | 0.299 | 2.000 | 0.025 | 0.036 | 4096:0.013, 8192:0.013, 16384:0.013, 32768:0.025 |
| comb_rect_dense | 0 | 0.205 | 1.348 | 0.056 | 0.061 | 4096:0.039, 8192:0.038, 16384:0.052, 32768:0.056 |
| multi_comb_cap | 0 | 0.133 | 1.217 | 0.051 | 0.058 | 4096:0.037, 8192:0.042, 16384:0.042, 32768:0.051 |
| multi_comb_core | 0 | 0.330 | 1.689 | 0.025 | 0.030 | 4096:0.012, 8192:0.015, 16384:0.015, 32768:0.025 |
| multi_comb_rect | 0 | 0.129 | 1.320 | 0.057 | 0.067 | 4096:0.043, 8192:0.041, 16384:0.044, 32768:0.057 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | caterpillar_rect_dense | 32768 | 3 | 0.093 | 15760 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_rect_dense/n32768/seed3_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 32768 | 1 | 0.081 | 15776 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_rect_dense/n32768/seed1_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 16384 | 2 | 0.078 | 12592 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_rect_dense/n16384/seed2_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 32768 | 2 | 0.073 | 15776 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_rect_dense/n32768/seed2_L1_Q1 |
| hard_scaling | caterpillar_mixed | 32768 | 3 | 0.071 | 15680 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_mixed/n32768/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 3 | 0.067 | 12896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/multi_comb_rect/n32768/seed3_L1_Q1 |
| hard_scaling | caterpillar_mixed | 32768 | 1 | 0.064 | 15680 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_mixed/n32768/seed1_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 16384 | 3 | 0.063 | 12592 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_rect_dense/n16384/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 3 | 0.061 | 13008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/comb_rect_dense/n32768/seed3_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 16384 | 1 | 0.060 | 12576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_rect_dense/n16384/seed1_L1_Q1 |
| hard_scaling | caterpillar_mixed | 32768 | 2 | 0.060 | 15696 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_mixed/n32768/seed2_L1_Q1 |
| hard_scaling | caterpillar_mixed | 16384 | 3 | 0.059 | 12496 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_mixed/n16384/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 1 | 0.058 | 12720 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/multi_comb_cap/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 2 | 0.057 | 12880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/multi_comb_rect/n32768/seed2_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 1 | 0.056 | 13024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/comb_rect_dense/n32768/seed1_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 2 | 0.056 | 13008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/comb_rect_dense/n32768/seed2_L1_Q1 |
| hard_scaling | caterpillar_mixed | 16384 | 2 | 0.056 | 12496 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_mixed/n16384/seed2_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 2 | 0.056 | 12928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/comb_dense/n32768/seed2_L1_Q1 |
| hard_scaling | caterpillar_mixed | 8192 | 3 | 0.055 | 10912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/caterpillar_mixed/n8192/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 16384 | 3 | 0.054 | 10864 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_hard_scaling_codex_rerun/runs/hard_scaling/comb_rect_dense/n16384/seed3_L1_Q1 |
