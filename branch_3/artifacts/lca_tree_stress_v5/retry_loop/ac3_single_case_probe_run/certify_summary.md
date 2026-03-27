# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 1 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 1  
timeouts: 0  
re/wa: 1  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| comb_rect_dense | 1 | - | - | - | - | - |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| correctness_fuzz | comb_rect_dense | 64 | 1 | 0.965 | 4080 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/retry_loop/ac3_single_case_probe_report/runs/correctness_fuzz/comb_rect_dense/n64/seed1_L0_Q0 |
