# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 5 failing cases
- hard_scaling: 69 failing cases
- hard_scaling: comb_plus_unary: alpha=1.915 > 1.450
- hard_scaling: comb_plus_unary: ratio=4.173 > 2.900
- hard_scaling: multi_comb_core: alpha=1.985 > 1.450
- hard_scaling: multi_comb_core: ratio=4.168 > 2.900
- max_n_mix: 24 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 5  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.953 | 2.360 | 0.161 | 0.183 | 64:0.011, 128:0.021, 256:0.031, 512:0.072, 1024:0.161 |
| broom_mixed | 0 | 1.345 | 3.406 | 0.633 | 1.052 | 64:0.015, 128:0.029, 256:0.067, 512:0.186, 1024:0.633 |
| caterpillar_rect_dense | 5 | 1.751 | 5.191 | 1.869 | 2.006 | 64:0.014, 128:0.033, 256:0.095, 512:0.360, 1024:1.869 |
| chain_unary | 0 | 1.456 | 3.453 | 0.702 | 0.736 | 64:0.015, 128:0.045, 256:0.156, 512:0.502, 1024:0.702 |
| comb_rect_dense | 0 | 1.674 | 4.242 | 1.467 | 1.622 | 64:0.015, 128:0.030, 256:0.104, 512:0.346, 1024:1.467 |
| multi_comb_cap | 0 | 1.186 | 3.198 | 0.433 | 0.469 | 64:0.015, 128:0.030, 256:0.056, 512:0.135, 1024:0.433 |
| multi_comb_rect | 0 | 1.285 | 3.336 | 0.551 | 0.613 | 64:0.015, 128:0.030, 256:0.060, 512:0.165, 1024:0.551 |
| random_recursive_mixed | 0 | 0.829 | 2.215 | 0.133 | 0.147 | 64:0.015, 128:0.015, 256:0.030, 512:0.060, 1024:0.133 |
| star_pairs | 0 | 1.114 | 2.851 | 0.251 | 0.295 | 64:0.015, 128:0.030, 256:0.086, 512:0.242, 1024:0.251 |

## Stage: hard_scaling

status: **FAIL**  
cases: 108  
timeouts: 69  
re/wa: 0  

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
