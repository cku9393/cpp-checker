# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 78 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 78  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.314 | 1.634 | 0.499 | 0.546 | 64:0.202, 128:0.212, 256:0.237, 512:0.306, 1024:0.499 |
| broom_mixed | 0 | 0.645 | 2.677 | 1.341 | 1.532 | 64:0.206, 128:0.243, 256:0.297, 512:0.501, 1024:1.341 |
| caterpillar_rect_dense | 25 | 0.998 | 3.661 | 1.866 | 1.999 | 64:0.230, 128:0.270, 256:0.510, 512:1.866 |
| chain_unary | 0 | 0.852 | 2.806 | 1.435 | 1.572 | 64:0.167, 128:0.191, 256:0.337, 512:0.946, 1024:1.435 |
| comb_rect_dense | 33 | 1.128 | 4.485 | 1.909 | 2.001 | 64:0.176, 128:0.217, 256:0.426, 512:1.909 |
| multi_comb_cap | 0 | 0.693 | 3.288 | 1.254 | 1.498 | 64:0.167, 128:0.177, 256:0.223, 512:0.381, 1024:1.254 |
| multi_comb_rect | 20 | 0.599 | 2.137 | 0.655 | 0.818 | 64:0.186, 128:0.212, 256:0.306, 512:0.655 |
| random_recursive_mixed | 0 | 0.253 | 1.474 | 0.391 | 0.461 | 64:0.190, 128:0.194, 256:0.218, 512:0.265, 1024:0.391 |
| star_pairs | 0 | 0.444 | 1.826 | 0.559 | 0.597 | 64:0.193, 128:0.213, 256:0.300, 512:0.548, 1024:0.559 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | comb_rect_dense | 512 | 3 | 2.001 | 240448 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/comb_rect_dense/n512/seed3_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 2 | 1.999 | 230480 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed2_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 2 | 1.980 | 140704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed2_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 5 | 1.964 | 211952 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed5_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 5 | 1.961 | 141312 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed5_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 4 | 1.950 | 141040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed4_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 2 | 1.942 | 140624 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed2_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 4 | 1.939 | 220640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/comb_rect_dense/n512/seed4_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 4 | 1.914 | 140608 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed4_L0_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 4 | 1.911 | 220304 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/comb_rect_dense/n512/seed4_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 2 | 1.909 | 233936 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/comb_rect_dense/n512/seed2_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 2 | 1.901 | 233040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/comb_rect_dense/n512/seed2_L1_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 1 | 1.866 | 134368 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed1_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 3 | 1.855 | 135296 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed3_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 4 | 1.832 | 193728 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed4_L1_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 3 | 1.816 | 135328 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed3_L0_Q1 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 1 | 1.814 | 134048 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed1_L0_Q0 |
| correctness_fuzz | caterpillar_rect_dense | 512 | 4 | 1.770 | 193712 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/caterpillar_rect_dense/n512/seed4_L1_Q1 |
| correctness_fuzz | comb_rect_dense | 512 | 1 | 1.706 | 212272 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/comb_rect_dense/n512/seed1_L1_Q0 |
| correctness_fuzz | comb_rect_dense | 512 | 1 | 1.669 | 212128 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag_ac3_correctness_capture_current/runs/correctness_fuzz/comb_rect_dense/n512/seed1_L1_Q1 |
