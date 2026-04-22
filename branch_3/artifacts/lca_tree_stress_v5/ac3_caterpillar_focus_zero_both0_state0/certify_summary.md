# Certification summary

overall verdict: **FAIL**

## Reasons

- caterpillar_focus: 3 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: caterpillar_focus

status: **FAIL**  
cases: 20  
timeouts: 3  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 3 | - | - | 1.908 | 1.997 | 1024:1.908 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 2 | 1.997 | 622816 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed2_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.997 | 551840 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 2 | 1.993 | 618160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed2_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.984 | 550976 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 2 | 1.976 | 618080 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.968 | 551120 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.966 | 551248 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.941 | 527088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.908 | 527104 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.898 | 514112 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.891 | 524096 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.871 | 527104 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.851 | 513664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.818 | 556752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.816 | 556672 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.815 | 556752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.801 | 554000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_zero_both0_state0/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
