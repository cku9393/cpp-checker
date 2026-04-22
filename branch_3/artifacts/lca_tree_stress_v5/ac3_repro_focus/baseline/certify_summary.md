# Certification summary

overall verdict: **PASS**

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: ac3_focus

status: **PASS**  
cases: 4  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_rect_dense | 0 | - | - | 1.863 | 2.000 | 1024:1.863 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| ac3_focus | caterpillar_rect_dense | 1024 | 2 | 2.000 | 623232 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_focus/baseline/runs/ac3_focus/caterpillar_rect_dense/n1024/seed2_L1_Q1 |
| ac3_focus | caterpillar_rect_dense | 1024 | 2 | 1.969 | 619200 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_focus/baseline/runs/ac3_focus/caterpillar_rect_dense/n1024/seed2_L0_Q1 |
| ac3_focus | caterpillar_rect_dense | 1024 | 5 | 1.758 | 527520 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_focus/baseline/runs/ac3_focus/caterpillar_rect_dense/n1024/seed5_L1_Q1 |
| ac3_focus | caterpillar_rect_dense | 1024 | 5 | 1.757 | 528640 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/ac3_repro_focus/baseline/runs/ac3_focus/caterpillar_rect_dense/n1024/seed5_L0_Q1 |
