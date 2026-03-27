# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 151 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 81  
re/wa: 70  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.150 | 1.249 | 0.284 | 0.370 | 64:0.182, 128:0.194, 256:0.208, 512:0.227, 1024:0.284 |
| broom_mixed | 8 | 0.646 | 2.557 | 0.894 | 1.142 | 64:0.143, 128:0.156, 256:0.190, 512:0.350, 1024:0.894 |
| caterpillar_rect_dense | 30 | 0.978 | 2.622 | 1.192 | 1.254 | 64:0.162, 128:0.205, 256:0.454, 512:1.192 |
| chain_unary | 10 | 0.707 | 2.392 | 0.908 | 0.935 | 64:0.154, 128:0.177, 256:0.286, 512:0.684, 1024:0.908 |
| comb_rect_dense | 31 | 1.084 | 3.021 | 1.573 | 1.956 | 64:0.174, 128:0.210, 256:0.521, 512:1.573 |
| multi_comb_cap | 0 | 0.825 | 3.689 | 1.553 | 1.979 | 64:0.144, 128:0.162, 256:0.199, 512:0.421, 1024:1.553 |
| multi_comb_rect | 20 | 0.693 | 2.626 | 0.610 | 0.750 | 64:0.141, 128:0.156, 256:0.232, 512:0.610 |
| random_recursive_mixed | 52 | -0.020 | 1.186 | 0.216 | 0.312 | 64:0.216, 128:0.155, 256:0.169, 512:0.200 |
| star_pairs | 0 | 0.442 | 1.602 | 0.461 | 0.490 | 64:0.154, 128:0.167, 256:0.249, 512:0.399, 1024:0.461 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | multi_comb_cap | 1024 | 5 | 1.979 | 176880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed5_L1_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 3 | 1.964 | 173504 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed3_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 1 | 1.956 | 161968 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/comb_rect_dense/n512/seed1_L0_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 5 | 1.922 | 180288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed5_L1_Q1 |
| correctness_fuzz | multi_comb_cap | 1024 | 1 | 1.911 | 173984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed1_L1_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 4 | 1.893 | 174928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed4_L1_Q1 |
| correctness_fuzz | multi_comb_cap | 1024 | 3 | 1.887 | 168768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed3_L1_Q1 |
| correctness_fuzz | multi_comb_cap | 1024 | 4 | 1.874 | 171280 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed4_L1_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 1 | 1.861 | 168256 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed1_L1_Q1 |
| correctness_fuzz | multi_comb_cap | 1024 | 2 | 1.857 | 182400 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed2_L1_Q0 |
| correctness_fuzz | multi_comb_cap | 1024 | 2 | 1.819 | 171712 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed2_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 2 | 1.769 | 164288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/comb_rect_dense/n512/seed2_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 5 | 1.598 | 170160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/comb_rect_dense/n512/seed5_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 5 | 1.582 | 171504 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/comb_rect_dense/n512/seed5_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 4 | 1.573 | 158256 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/comb_rect_dense/n512/seed4_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 2 | 1.569 | 171376 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/comb_rect_dense/n512/seed2_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 3 | 1.558 | 170144 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/comb_rect_dense/n512/seed3_L0_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 3 | 1.557 | 170112 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/comb_rect_dense/n512/seed3_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 4 | 1.557 | 167808 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/comb_rect_dense/n512/seed4_L0_Q1 |
| correctness_fuzz | multi_comb_cap | 1024 | 5 | 1.286 | 103600 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_after_importfix/runs/correctness_fuzz/multi_comb_cap/n1024/seed5_L0_Q1 |
