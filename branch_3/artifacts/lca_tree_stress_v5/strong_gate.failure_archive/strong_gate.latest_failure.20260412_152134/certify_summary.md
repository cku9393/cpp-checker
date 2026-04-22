# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 4 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 4  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.831 | 2.232 | 0.134 | 0.161 | 64:0.015, 128:0.015, 256:0.030, 512:0.060, 1024:0.134 |
| broom_mixed | 0 | 1.266 | 3.150 | 0.518 | 0.910 | 64:0.015, 128:0.030, 256:0.060, 512:0.164, 1024:0.518 |
| caterpillar_rect_dense | 4 | 1.751 | 5.201 | 1.875 | 1.942 | 64:0.015, 128:0.030, 256:0.104, 512:0.361, 1024:1.875 |
| chain_unary | 0 | 1.510 | 4.389 | 0.706 | 0.765 | 64:0.015, 128:0.030, 256:0.132, 512:0.479, 1024:0.706 |
| comb_rect_dense | 0 | 1.605 | 4.122 | 1.238 | 1.265 | 64:0.015, 128:0.030, 256:0.090, 512:0.300, 1024:1.238 |
| multi_comb_cap | 0 | 1.187 | 2.837 | 0.330 | 0.359 | 64:0.015, 128:0.015, 256:0.042, 512:0.116, 1024:0.330 |
| multi_comb_rect | 0 | 1.246 | 3.367 | 0.506 | 0.534 | 64:0.015, 128:0.030, 256:0.060, 512:0.150, 1024:0.506 |
| random_recursive_mixed | 0 | 0.799 | 2.000 | 0.120 | 0.135 | 64:0.015, 128:0.015, 256:0.030, 512:0.060, 1024:0.120 |
| star_pairs | 0 | 1.155 | 3.208 | 0.278 | 0.312 | 64:0.015, 128:0.030, 256:0.096, 512:0.278, 1024:0.271 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.942 | 539952 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.940 | 540000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.926 | 545328 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.907 | 545328 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.898 | 545296 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.897 | 543088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 1 | 1.891 | 548320 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 4 | 1.879 | 542416 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.872 | 504848 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.842 | 518416 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.839 | 519824 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.831 | 505888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.825 | 500992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 3 | 1.822 | 502208 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed3_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.816 | 519456 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 1024 | 5 | 1.800 | 519792 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 1024 | 5 | 1.265 | 191920 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed5_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 1 | 1.252 | 191968 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed1_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 2 | 1.247 | 195088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed2_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 1024 | 3 | 1.245 | 191232 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/runs/correctness_fuzz/comb_rect_dense/n1024/seed3_L0_Q1 |
