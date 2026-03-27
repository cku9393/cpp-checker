# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 121 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 121  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.339 | 1.674 | 0.369 | 0.586 | 64:0.139, 128:0.149, 256:0.170, 512:0.220, 1024:0.369 |
| broom_mixed | 1 | 0.835 | 2.833 | 1.454 | 2.005 | 64:0.142, 128:0.164, 256:0.246, 512:0.513, 1024:1.454 |
| caterpillar_rect_dense | 40 | 1.003 | 3.187 | 0.626 | 1.259 | 64:0.156, 128:0.197, 256:0.626 |
| chain_unary | 0 | 0.622 | 2.396 | 1.050 | 1.143 | 64:0.219, 128:0.233, 256:0.315, 512:0.755, 1024:1.050 |
| comb_rect_dense | 40 | 1.066 | 3.565 | 0.637 | 1.086 | 64:0.145, 128:0.179, 256:0.637 |
| multi_comb_cap | 20 | 0.617 | 2.497 | 0.537 | 0.738 | 64:0.143, 128:0.160, 256:0.215, 512:0.537 |
| multi_comb_rect | 20 | 0.821 | 3.732 | 1.188 | 1.801 | 64:0.201, 128:0.222, 256:0.318, 512:1.188 |
| random_recursive_mixed | 0 | 0.573 | 1.794 | 0.723 | 1.660 | 64:0.168, 128:0.206, 256:0.326, 512:0.585, 1024:0.723 |
| star_pairs | 0 | 0.443 | 1.781 | 0.416 | 0.637 | 64:0.138, 128:0.160, 256:0.234, 512:0.416, 1024:0.396 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | broom_mixed | 1024 | 4 | 2.005 | 39936 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed4_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.876 | 40240 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.842 | 41536 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q1 |
| correctness_fuzz | multi_comb_rect | 512 | 1 | 1.801 | 62208 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/multi_comb_rect/n512/seed1_L1_Q1 |
| correctness_fuzz | multi_comb_rect | 512 | 1 | 1.781 | 63152 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/multi_comb_rect/n512/seed1_L1_Q0 |
| correctness_fuzz | random_recursive_mixed | 1024 | 2 | 1.660 | 12848 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/random_recursive_mixed/n1024/seed2_L0_Q1 |
| correctness_fuzz | random_recursive_mixed | 1024 | 1 | 1.648 | 16064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/random_recursive_mixed/n1024/seed1_L1_Q1 |
| correctness_fuzz | multi_comb_rect | 512 | 2 | 1.625 | 61136 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/multi_comb_rect/n512/seed2_L1_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.604 | 42128 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.598 | 39472 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.594 | 41808 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q1 |
| correctness_fuzz | multi_comb_rect | 512 | 2 | 1.583 | 60656 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/multi_comb_rect/n512/seed2_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.577 | 43424 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.572 | 38992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed3_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 2 | 1.542 | 40400 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed2_L0_Q0 |
| correctness_fuzz | random_recursive_mixed | 1024 | 2 | 1.539 | 13072 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/random_recursive_mixed/n1024/seed2_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 3 | 1.454 | 37856 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/broom_mixed/n1024/seed3_L1_Q1 |
| correctness_fuzz | multi_comb_rect | 512 | 4 | 1.431 | 60640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/multi_comb_rect/n512/seed4_L1_Q1 |
| correctness_fuzz | multi_comb_rect | 512 | 4 | 1.423 | 60880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/multi_comb_rect/n512/seed4_L1_Q0 |
| correctness_fuzz | random_recursive_mixed | 1024 | 1 | 1.355 | 13472 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate/retry_loop/ac3_correctness_filtered_progress40restore/runs/correctness_fuzz/random_recursive_mixed/n1024/seed1_L0_Q0 |
