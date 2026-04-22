# Certification summary

overall verdict: **FAIL**

## Reasons

- ac3_repro: 3 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: ac3_repro

status: **FAIL**  
cases: 20  
timeouts: 3  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 3 | - | - | 1.825 | 2.009 | 1024:1.825 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| ac3_repro | caterpillar_rect_dense | 1024 | 2 | 2.009 | 617904 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 2 | 2.007 | 622688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed2_L1_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 4 | 1.909 | 551776 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 4 | 1.902 | 552112 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 4 | 1.896 | 551520 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 4 | 1.884 | 552752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 1 | 1.862 | 557744 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 1 | 1.844 | 557184 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 1 | 1.825 | 557744 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 1 | 1.823 | 557744 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 3 | 1.797 | 515088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 5 | 1.793 | 528592 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 3 | 1.790 | 513616 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed3_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 3 | 1.788 | 512944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 5 | 1.775 | 528608 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 5 | 1.775 | 528704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 3 | 1.767 | 513808 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/run/runs/ac3_repro/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
