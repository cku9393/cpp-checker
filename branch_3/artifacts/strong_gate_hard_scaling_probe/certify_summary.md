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
| caterpillar_mixed | 9 | - | - | 6.961 | 7.742 | 4096:6.961 |
| caterpillar_rect_dense | 9 | - | - | 7.698 | 8.700 | 4096:7.698 |
| comb_core | 0 | 0.305 | 2.105 | 0.025 | 0.025 | 4096:0.012, 8192:0.013, 16384:0.012, 32768:0.025 |
| comb_dense | 0 | 0.291 | 1.402 | 0.085 | 0.085 | 4096:0.046, 8192:0.051, 16384:0.061, 32768:0.085 |
| comb_plus_unary | 0 | 0.537 | 1.845 | 0.036 | 0.036 | 4096:0.013, 8192:0.012, 16384:0.022, 32768:0.036 |
| comb_rect_dense | 0 | 0.164 | 1.332 | 0.070 | 0.090 | 4096:0.050, 8192:0.048, 16384:0.053, 32768:0.070 |
| multi_comb_cap | 0 | 0.137 | 1.152 | 0.060 | 0.068 | 4096:0.046, 8192:0.050, 16384:0.057, 32768:0.060 |
| multi_comb_core | 0 | 0.301 | 1.997 | 0.030 | 0.037 | 4096:0.015, 8192:0.015, 16384:0.015, 32768:0.030 |
| multi_comb_rect | 0 | 0.186 | 1.274 | 0.095 | 0.154 | 4096:0.061, 8192:0.077, 16384:0.095, 32768:0.087 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | caterpillar_rect_dense | 4096 | 2 | 8.700 | 10736 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/caterpillar_rect_dense/n4096/seed2_L1_Q1 |
| hard_scaling | caterpillar_mixed | 4096 | 2 | 7.742 | 10672 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/caterpillar_mixed/n4096/seed2_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 4096 | 3 | 7.698 | 10752 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/caterpillar_rect_dense/n4096/seed3_L1_Q1 |
| hard_scaling | caterpillar_rect_dense | 4096 | 1 | 7.066 | 10800 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/caterpillar_rect_dense/n4096/seed1_L1_Q1 |
| hard_scaling | caterpillar_mixed | 4096 | 1 | 6.961 | 10704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/caterpillar_mixed/n4096/seed1_L1_Q1 |
| hard_scaling | caterpillar_mixed | 4096 | 3 | 6.033 | 10688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/caterpillar_mixed/n4096/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 1 | 0.154 | 12880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 8192 | 2 | 0.135 | 9632 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n8192/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 1 | 0.116 | 10688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n16384/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 3 | 0.095 | 10704 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n16384/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 16384 | 2 | 0.092 | 10688 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n16384/seed2_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 1 | 0.090 | 13040 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/comb_rect_dense/n32768/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 3 | 0.087 | 12880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n32768/seed3_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 1 | 0.085 | 12944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/comb_dense/n32768/seed1_L1_Q1 |
| hard_scaling | comb_dense | 32768 | 3 | 0.085 | 12928 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/comb_dense/n32768/seed3_L1_Q1 |
| hard_scaling | multi_comb_rect | 32768 | 2 | 0.079 | 12880 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n32768/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 4096 | 1 | 0.078 | 9088 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n4096/seed1_L1_Q1 |
| hard_scaling | multi_comb_rect | 8192 | 3 | 0.077 | 9632 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n8192/seed3_L1_Q1 |
| hard_scaling | comb_rect_dense | 32768 | 2 | 0.070 | 13008 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/comb_rect_dense/n32768/seed2_L1_Q1 |
| hard_scaling | multi_comb_rect | 8192 | 1 | 0.070 | 9648 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/strong_gate_hard_scaling_probe/runs/hard_scaling/multi_comb_rect/n8192/seed1_L1_Q1 |
