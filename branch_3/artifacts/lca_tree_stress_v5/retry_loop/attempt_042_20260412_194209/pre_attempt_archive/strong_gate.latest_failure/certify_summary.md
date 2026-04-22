# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 9 failing cases
- hard_scaling: 69 failing cases
- hard_scaling: comb_plus_unary: alpha=1.882 > 1.450
- hard_scaling: comb_plus_unary: ratio=3.886 > 2.900
- hard_scaling: multi_comb_core: alpha=1.858 > 1.450
- hard_scaling: multi_comb_core: ratio=3.718 > 2.900
- max_n_mix: 24 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 9  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.940 | 2.218 | 0.155 | 0.176 | 64:0.011, 128:0.021, 256:0.035, 512:0.070, 1024:0.155 |
| broom_mixed | 0 | 1.421 | 3.050 | 0.567 | 1.021 | 64:0.011, 128:0.025, 256:0.066, 512:0.186, 1024:0.567 |
| caterpillar_rect_dense | 9 | 1.784 | 4.690 | 1.837 | 1.927 | 64:0.012, 128:0.037, 256:0.106, 512:0.392, 1024:1.837 |
| chain_unary | 0 | 1.545 | 3.761 | 0.710 | 0.829 | 64:0.013, 128:0.035, 256:0.131, 512:0.491, 1024:0.710 |
| comb_rect_dense | 0 | 1.609 | 4.124 | 1.252 | 1.291 | 64:0.015, 128:0.032, 256:0.093, 512:0.304, 1024:1.252 |
| multi_comb_cap | 0 | 1.209 | 3.075 | 0.361 | 0.380 | 64:0.012, 128:0.025, 256:0.050, 512:0.117, 1024:0.361 |
| multi_comb_rect | 0 | 1.350 | 3.488 | 0.556 | 0.641 | 64:0.013, 128:0.026, 256:0.059, 512:0.159, 1024:0.556 |
| random_recursive_mixed | 0 | 0.868 | 2.137 | 0.115 | 0.127 | 64:0.011, 128:0.015, 256:0.028, 512:0.054, 1024:0.115 |
| star_pairs | 0 | 1.171 | 3.545 | 0.279 | 0.308 | 64:0.015, 128:0.028, 256:0.100, 512:0.279, 1024:0.277 |

## Stage: hard_scaling

status: **FAIL**  
cases: 108  
timeouts: 69  
re/wa: 0  

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
