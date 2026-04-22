# Certification summary

overall verdict: **PASS**

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: ac3_repro

status: **PASS**  
cases: 20  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 0 | - | - | 1.797 | 1.995 | 1024:1.797 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| ac3_repro | caterpillar_rect_dense | 1024 | 2 | 1.995 | 623168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed2_L1_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 2 | 1.989 | 618704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed2_L0_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 2 | 1.979 | 624064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed2_L1_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 2 | 1.973 | 614880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed2_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 4 | 1.831 | 551552 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed4_L0_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 4 | 1.823 | 551664 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed4_L1_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 4 | 1.817 | 552032 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed4_L1_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 4 | 1.807 | 552064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed4_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 1 | 1.806 | 558224 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed1_L1_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 1 | 1.801 | 556624 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed1_L1_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 1 | 1.793 | 558224 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed1_L0_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 5 | 1.787 | 527312 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed5_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 1 | 1.787 | 557648 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed1_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 5 | 1.760 | 527856 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 5 | 1.759 | 528896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 5 | 1.754 | 527296 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed5_L1_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 3 | 1.742 | 514288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed3_L0_Q0 |
| ac3_repro | caterpillar_rect_dense | 1024 | 3 | 1.741 | 513824 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed3_L1_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 3 | 1.740 | 514656 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed3_L0_Q1 |
| ac3_repro | caterpillar_rect_dense | 1024 | 3 | 1.739 | 514752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_current/rerun/runs/ac3_repro/caterpillar_rect_dense/n1024/seed3_L1_Q0 |
