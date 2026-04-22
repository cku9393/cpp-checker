# Certification summary

overall verdict: **FAIL**

## Reasons

- caterpillar_focus: 1 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: caterpillar_focus

status: **FAIL**  
cases: 20  
timeouts: 1  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 1 | - | - | 1.802 | 1.998 | 1024:1.802 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 2 | 1.998 | 622704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed2_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 2 | 1.992 | 615296 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 2 | 1.983 | 617968 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed2_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.854 | 556672 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.827 | 551184 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.821 | 554032 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.816 | 551248 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.812 | 556784 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.803 | 550976 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.802 | 550960 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.802 | 556704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.775 | 510928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.772 | 527808 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.770 | 527728 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.758 | 513264 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.752 | 527056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.749 | 524288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.743 | 510896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.738 | 510544 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
