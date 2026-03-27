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
| caterpillar_mixed | 0 | 0.327 | 1.326 | 0.072 | 0.202 | 4096:0.036, 8192:0.048, 16384:0.060, 32768:0.072 |
| caterpillar_rect_dense | 0 | 0.238 | 1.299 | 0.073 | 0.676 | 4096:0.046, 8192:0.045, 16384:0.056, 32768:0.073 |
| comb_core | 0 | 0.324 | 1.971 | 0.025 | 0.025 | 4096:0.012, 8192:0.013, 16384:0.013, 32768:0.025 |
| comb_dense | 0 | 0.252 | 1.440 | 0.058 | 0.072 | 4096:0.036, 8192:0.035, 16384:0.050, 32768:0.058 |
| comb_plus_unary | 0 | 0.421 | 2.041 | 0.027 | 0.027 | 4096:0.013, 8192:0.011, 16384:0.023, 32768:0.027 |
| comb_rect_dense | 0 | 0.178 | 1.340 | 0.053 | 0.059 | 4096:0.035, 8192:0.047, 16384:0.048, 32768:0.053 |
| multi_comb_cap | 0 | 0.169 | 1.284 | 0.048 | 0.085 | 4096:0.036, 8192:0.038, 16384:0.048, 32768:0.048 |
| multi_comb_core | 0 | 0.284 | 1.926 | 0.024 | 0.025 | 4096:0.013, 8192:0.013, 16384:0.013, 32768:0.024 |
| multi_comb_rect | 0 | 0.200 | 1.262 | 0.060 | 0.137 | 4096:0.038, 8192:0.047, 16384:0.048, 32768:0.060 |

## Stage: max_dense_headroom

status: **PASS**  
cases: 20  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 0 | 0.530 | 1.444 | 0.100 | 0.112 | 50000:0.069, 99999:0.100 |
| comb_dense | 0 | 0.752 | 1.684 | 0.095 | 0.098 | 50000:0.056, 99999:0.095 |
| comb_rect_dense | 0 | 0.947 | 1.928 | 0.120 | 0.127 | 50000:0.062, 99999:0.120 |
| multi_comb_cap | 0 | 0.347 | 1.272 | 0.093 | 0.098 | 50000:0.073, 99999:0.093 |
| multi_comb_rect | 0 | 0.439 | 1.355 | 0.082 | 0.094 | 50000:0.060, 99999:0.082 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | caterpillar_rect_dense | 8192 | 1 | 0.676 | 10992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/hard_scaling/caterpillar_rect_dense/n8192/seed1_L1_Q1 |
| hard_scaling | caterpillar_mixed | 16384 | 1 | 0.202 | 12528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/hard_scaling/caterpillar_mixed/n16384/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 3 | 0.137 | 12896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/hard_scaling/multi_comb_rect/n32768/seed3_L1_Q1 |
| max_dense_headroom | comb_rect_dense | 99999 | 1 | 0.127 | 22624 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/comb_rect_dense/n99999/seed1_L1_Q1 |
| max_dense_headroom | caterpillar_rect_dense | 99999 | 2 | 0.112 | 28832 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/caterpillar_rect_dense/n99999/seed2_L1_Q1 |
| max_dense_headroom | comb_rect_dense | 99999 | 2 | 0.112 | 22624 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/comb_rect_dense/n99999/seed2_L1_Q1 |
| max_dense_headroom | comb_dense | 99999 | 1 | 0.098 | 21840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/comb_dense/n99999/seed1_L1_Q1 |
| max_dense_headroom | multi_comb_cap | 99999 | 1 | 0.098 | 21872 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/multi_comb_cap/n99999/seed1_L1_Q1 |
| max_dense_headroom | multi_comb_rect | 99999 | 1 | 0.094 | 21952 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/multi_comb_rect/n99999/seed1_L1_Q1 |
| max_dense_headroom | comb_dense | 99999 | 2 | 0.092 | 21840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/comb_dense/n99999/seed2_L1_Q1 |
| max_dense_headroom | multi_comb_cap | 99999 | 2 | 0.088 | 21904 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/multi_comb_cap/n99999/seed2_L1_Q1 |
| max_dense_headroom | caterpillar_rect_dense | 99999 | 1 | 0.088 | 28816 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/caterpillar_rect_dense/n99999/seed1_L1_Q1 |
| max_dense_headroom | multi_comb_cap | 50000 | 1 | 0.087 | 15008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/multi_comb_cap/n50000/seed1_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 1 | 0.085 | 12736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/hard_scaling/multi_comb_cap/n32768/seed1_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 32768 | 2 | 0.076 | 15776 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/hard_scaling/caterpillar_rect_dense/n32768/seed2_L1_Q1 |
| max_dense_headroom | comb_rect_dense | 50000 | 2 | 0.076 | 15264 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/comb_rect_dense/n50000/seed2_L1_Q1 |
| max_dense_headroom | multi_comb_rect | 50000 | 2 | 0.074 | 15216 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/multi_comb_rect/n50000/seed2_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 32768 | 1 | 0.073 | 15776 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/hard_scaling/caterpillar_rect_dense/n32768/seed1_L1_Q1 |
| hard_scaling | caterpillar_mixed | 32768 | 2 | 0.073 | 15712 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/hard_scaling/caterpillar_mixed/n32768/seed2_L1_Q1 |
| max_dense_headroom | caterpillar_rect_dense | 50000 | 1 | 0.072 | 19168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/rebuttal_gate/lca_tree_stress_v5/rebuttal_ac4_probe/runs/max_dense_headroom/caterpillar_rect_dense/n50000/seed1_L1_Q1 |
