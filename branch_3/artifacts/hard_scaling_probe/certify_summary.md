# Certification summary

overall verdict: **FAIL**

## Reasons

- hard_scaling: 18 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: hard_scaling

status: **FAIL**  
cases: 108  
timeouts: 18  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 9 | - | - | 6.377 | 7.543 | 4096:6.377 |
| caterpillar_rect_dense | 9 | - | - | 7.836 | 8.009 | 4096:7.836 |
| comb_core | 0 | 0.004 | 1.026 | 0.013 | 0.023 | 4096:0.012, 8192:0.013, 16384:0.012, 32768:0.013 |
| comb_dense | 0 | 0.179 | 1.207 | 0.071 | 0.096 | 4096:0.048, 8192:0.058, 16384:0.062, 32768:0.071 |
| comb_plus_unary | 0 | 0.469 | 2.828 | 0.036 | 0.037 | 4096:0.012, 8192:0.013, 16384:0.013, 32768:0.036 |
| comb_rect_dense | 0 | 0.062 | 1.153 | 0.057 | 0.063 | 4096:0.049, 8192:0.050, 16384:0.049, 32768:0.057 |
| multi_comb_cap | 0 | 0.121 | 1.309 | 0.063 | 0.735 | 4096:0.048, 8192:0.048, 16384:0.063, 32768:0.059 |
| multi_comb_core | 0 | 0.323 | 2.004 | 0.030 | 0.039 | 4096:0.014, 8192:0.015, 16384:0.015, 32768:0.030 |
| multi_comb_rect | 0 | 0.247 | 1.324 | 0.107 | 0.128 | 4096:0.067, 8192:0.067, 16384:0.089, 32768:0.107 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | caterpillar_rect_dense | 4096 | 1 | 8.009 | 10784 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/caterpillar_rect_dense/n4096/seed1_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 4096 | 3 | 7.836 | 10768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/caterpillar_rect_dense/n4096/seed3_L1_Q1 |
| hard_scaling | caterpillar_mixed | 4096 | 2 | 7.543 | 10672 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/caterpillar_mixed/n4096/seed2_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 4096 | 2 | 7.299 | 10768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/caterpillar_rect_dense/n4096/seed2_L1_Q1 |
| hard_scaling | caterpillar_mixed | 4096 | 1 | 6.377 | 10688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/caterpillar_mixed/n4096/seed1_L1_Q1 |
| hard_scaling | caterpillar_mixed | 4096 | 3 | 6.184 | 10688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/caterpillar_mixed/n4096/seed3_L1_Q1 |
| hard_scaling | multi_comb_cap | 16384 | 3 | 0.735 | 10592 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_cap/n16384/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 2 | 0.128 | 10704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n16384/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 8192 | 2 | 0.118 | 9632 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n8192/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 1 | 0.117 | 12896 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 2 | 0.107 | 12880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n32768/seed2_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 1 | 0.096 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/comb_dense/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 1 | 0.089 | 10688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n16384/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 4096 | 3 | 0.085 | 9088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n4096/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 3 | 0.082 | 12880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n32768/seed3_L1_Q1 |
| hard_scaling | comb_dense | 16384 | 2 | 0.072 | 10768 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/comb_dense/n16384/seed2_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 2 | 0.071 | 12928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/comb_dense/n32768/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 3 | 0.071 | 10688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n16384/seed3_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 3 | 0.070 | 12928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/comb_dense/n32768/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 8192 | 3 | 0.067 | 9632 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n8192/seed3_L1_Q1 |
