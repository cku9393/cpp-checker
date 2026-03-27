# Certification summary

overall verdict: **FAIL**

## Reasons

- hard_scaling: 14 failing cases
- hard_scaling: multi_comb_cap: alpha=1.502 > 1.450
- hard_scaling: multi_comb_cap: ratio=28.839 > 2.900

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: hard_scaling

status: **FAIL**  
cases: 108  
timeouts: 13  
re/wa: 1  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 6 | 1.077 | 2.109 | 8.502 | 11.406 | 4096:4.031, 8192:8.502 |
| caterpillar_rect_dense | 7 | 1.179 | 2.264 | 9.248 | 9.564 | 4096:4.084, 8192:9.248 |
| comb_core | 0 | 0.241 | 1.837 | 0.025 | 0.764 | 4096:0.014, 8192:0.025, 16384:0.023, 32768:0.025 |
| comb_dense | 0 | 0.190 | 1.489 | 0.073 | 0.073 | 4096:0.048, 8192:0.047, 16384:0.049, 32768:0.073 |
| comb_plus_unary | 0 | 0.298 | 1.922 | 0.025 | 0.037 | 4096:0.013, 8192:0.024, 16384:0.024, 32768:0.025 |
| comb_rect_dense | 0 | 0.241 | 1.316 | 0.061 | 0.085 | 4096:0.038, 8192:0.049, 16384:0.060, 32768:0.061 |
| multi_comb_cap | 1 | 1.502 | 28.839 | 1.330 | 2.487 | 4096:0.045, 8192:0.036, 16384:0.046, 32768:1.330 |
| multi_comb_core | 0 | 0.536 | 1.806 | 0.036 | 0.036 | 4096:0.013, 8192:0.012, 16384:0.023, 32768:0.036 |
| multi_comb_rect | 0 | 0.128 | 1.454 | 0.070 | 0.619 | 4096:0.048, 8192:0.048, 16384:0.070, 32768:0.057 |

Scale check hits:

- multi_comb_cap: alpha=1.502 > 1.450
- multi_comb_cap: ratio=28.839 > 2.900

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | caterpillar_mixed | 8192 | 3 | 11.406 | 11984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_mixed/n8192/seed3_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 8192 | 1 | 9.564 | 12112 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_rect_dense/n8192/seed1_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 8192 | 2 | 8.932 | 12096 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_rect_dense/n8192/seed2_L1_Q1 |
| hard_scaling | caterpillar_mixed | 8192 | 1 | 8.502 | 11968 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_mixed/n8192/seed1_L1_Q1 |
| hard_scaling | caterpillar_mixed | 8192 | 2 | 8.167 | 11952 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_mixed/n8192/seed2_L1_Q1 |
| hard_scaling | caterpillar_mixed | 4096 | 1 | 6.038 | 11056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_mixed/n4096/seed1_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 4096 | 2 | 4.595 | 11152 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_rect_dense/n4096/seed2_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 4096 | 1 | 4.084 | 11168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_rect_dense/n4096/seed1_L1_Q1 |
| hard_scaling | caterpillar_mixed | 4096 | 2 | 4.031 | 11072 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_mixed/n4096/seed2_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 4096 | 3 | 3.838 | 11136 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_rect_dense/n4096/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 1 | 3.344 | 12736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/multi_comb_cap/n32768/seed1_L1_Q1 |
| hard_scaling | caterpillar_mixed | 4096 | 3 | 3.217 | 11024 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/caterpillar_mixed/n4096/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 2 | 2.487 | 12752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/multi_comb_cap/n32768/seed2_L1_Q1 |
| hard_scaling | comb_core | 4096 | 1 | 0.764 | 2640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/comb_core/n4096/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 1 | 0.619 | 12896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/multi_comb_rect/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 3 | 0.295 | 10704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/multi_comb_rect/n16384/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 32768 | 3 | 0.173 | 12752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/multi_comb_cap/n32768/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 1 | 0.085 | 13056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/comb_rect_dense/n32768/seed1_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 1 | 0.073 | 12960 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/comb_dense/n32768/seed1_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 3 | 0.073 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/hard_scaling_probe_v2/runs/hard_scaling/comb_dense/n32768/seed3_L1_Q1 |
