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
| caterpillar_rect_dense | 3 | - | - | 1.824 | 2.007 | 1024:1.824 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 2 | 2.007 | 617984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.847 | 556768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.838 | 557536 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.836 | 557520 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.834 | 551152 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.833 | 551184 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.829 | 551136 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 1 | 1.824 | 556800 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 4 | 1.824 | 550976 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.796 | 527072 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.777 | 510912 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.774 | 527088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.769 | 513616 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.766 | 527152 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.763 | 513264 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 3 | 1.750 | 513344 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed3_L0_Q0 |
| caterpillar_focus | caterpillar_rect_dense | 1024 | 5 | 1.749 | 527104 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_caterpillar_focus_baseline/runs/caterpillar_focus/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
