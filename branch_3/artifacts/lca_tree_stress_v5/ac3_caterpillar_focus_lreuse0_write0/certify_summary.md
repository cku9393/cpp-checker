# Certification summary

overall verdict: **FAIL**

## Reasons

- caterpillar_focus: 6 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: caterpillar_focus

status: **FAIL**  
cases: 20  
timeouts: 6  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 6 | - | - | 1.811 | 2.008 | 1024:1.811 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 2.008 | 556752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 2.000 | 557568 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 2 | 1.967 | 615472 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.945 | 514336 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.918 | 553984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.849 | 556656 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.827 | 550992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.796 | 551200 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.796 | 551712 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.788 | 548400 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.770 | 527840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.770 | 527056 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.753 | 527088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.739 | 526992 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_lreuse0_write0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
