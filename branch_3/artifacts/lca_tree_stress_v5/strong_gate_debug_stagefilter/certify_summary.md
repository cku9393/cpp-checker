# Certification summary

overall verdict: **FAIL**

## Reasons

- correctness_fuzz: 141 failing cases
- hard_scaling: 86 failing cases
- hard_scaling: comb_core: alpha=1.753 > 1.450
- hard_scaling: comb_core: ratio=3.654 > 2.900
- hard_scaling: comb_plus_unary: alpha=1.831 > 1.450
- hard_scaling: comb_plus_unary: ratio=3.558 > 2.900
- max_n_mix: 28 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: correctness_fuzz

status: **FAIL**  
cases: 900  
timeouts: 141  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | 0.211 | 1.406 | 0.476 | 0.549 | 64:0.258, 128:0.267, 256:0.296, 512:0.338, 1024:0.476 |
| broom_mixed | 0 | 0.598 | 2.702 | 1.546 | 2.010 | 64:0.282, 128:0.272, 256:0.326, 512:0.572, 1024:1.546 |
| caterpillar_rect_dense | 40 | 0.831 | 2.472 | 0.763 | 1.278 | 64:0.241, 128:0.309, 256:0.763 |
| chain_unary | 0 | 0.599 | 1.760 | 1.206 | 1.256 | 64:0.252, 128:0.322, 256:0.507, 512:0.892, 1024:1.206 |
| comb_rect_dense | 40 | 0.789 | 2.524 | 0.680 | 1.123 | 64:0.228, 128:0.269, 256:0.680 |
| multi_comb_cap | 20 | 0.515 | 2.026 | 0.651 | 0.823 | 64:0.220, 128:0.234, 256:0.321, 512:0.651 |
| multi_comb_rect | 20 | 0.776 | 3.439 | 1.161 | 1.598 | 64:0.219, 128:0.232, 256:0.338, 512:1.161 |
| random_recursive_mixed | 21 | 0.097 | 1.350 | 0.485 | 0.863 | 128:0.396, 256:0.335, 512:0.359, 1024:0.485 |
| star_pairs | 0 | 0.287 | 1.469 | 0.497 | 0.566 | 64:0.244, 128:0.277, 256:0.339, 512:0.497, 1024:0.491 |

## Stage: hard_scaling

status: **FAIL**  
cases: 108  
timeouts: 86  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| caterpillar_mixed | 12 | - | - | - | - | - |
| caterpillar_rect_dense | 12 | - | - | - | - | - |
| comb_core | 5 | 1.753 | 3.654 | 11.486 | 11.486 | 4096:1.011, 8192:3.143, 16384:11.486 |
| comb_dense | 12 | - | - | - | - | - |
| comb_plus_unary | 6 | 1.831 | 3.558 | 4.272 | 4.472 | 4096:1.201, 8192:4.272 |
| comb_rect_dense | 12 | - | - | - | - | - |
| multi_comb_cap | 12 | - | - | - | - | - |
| multi_comb_core | 3 | 0.763 | 2.303 | 8.821 | 10.263 | 4096:3.062, 8192:3.831, 16384:8.821 |
| multi_comb_rect | 12 | - | - | - | - | - |

Scale check hits:

- comb_core: alpha=1.753 > 1.450
- comb_core: ratio=3.654 > 2.900
- comb_plus_unary: alpha=1.831 > 1.450
- comb_plus_unary: ratio=3.558 > 2.900

## Stage: max_n_mix

status: **FAIL**  
cases: 28  
timeouts: 28  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 4 | - | - | - | - | - |
| caterpillar_rect_dense | 4 | - | - | - | - | - |
| comb_dense | 4 | - | - | - | - | - |
| comb_rect_dense | 4 | - | - | - | - | - |
| multi_comb_cap | 4 | - | - | - | - | - |
| multi_comb_rect | 4 | - | - | - | - | - |
| random_recursive_mixed | 4 | - | - | - | - | - |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| hard_scaling | comb_core | 16384 | 1 | 11.486 | 357168 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/comb_core/n16384/seed1_L1_Q1 |
| hard_scaling | multi_comb_core | 16384 | 1 | 10.263 | 206432 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/multi_comb_core/n16384/seed1_L1_Q1 |
| hard_scaling | multi_comb_core | 16384 | 3 | 8.821 | 206304 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/multi_comb_core/n16384/seed3_L1_Q1 |
| hard_scaling | multi_comb_core | 16384 | 2 | 8.546 | 206016 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/multi_comb_core/n16384/seed2_L1_Q1 |
| hard_scaling | multi_comb_core | 8192 | 1 | 8.168 | 64944 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/multi_comb_core/n8192/seed1_L1_Q1 |
| hard_scaling | comb_plus_unary | 8192 | 2 | 4.472 | 103456 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/comb_plus_unary/n8192/seed2_L1_Q1 |
| hard_scaling | comb_plus_unary | 8192 | 1 | 4.272 | 103376 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/comb_plus_unary/n8192/seed1_L1_Q1 |
| hard_scaling | comb_plus_unary | 8192 | 3 | 4.209 | 103360 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/comb_plus_unary/n8192/seed3_L1_Q1 |
| hard_scaling | multi_comb_core | 8192 | 2 | 3.831 | 66064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/multi_comb_core/n8192/seed2_L1_Q1 |
| hard_scaling | multi_comb_core | 8192 | 3 | 3.495 | 66080 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/multi_comb_core/n8192/seed3_L1_Q1 |
| hard_scaling | multi_comb_core | 4096 | 2 | 3.283 | 24528 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/multi_comb_core/n4096/seed2_L1_Q1 |
| hard_scaling | comb_core | 8192 | 2 | 3.162 | 102144 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/comb_core/n8192/seed2_L1_Q1 |
| hard_scaling | comb_core | 8192 | 1 | 3.143 | 102288 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/comb_core/n8192/seed1_L1_Q1 |
| hard_scaling | multi_comb_core | 4096 | 3 | 3.062 | 25232 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/multi_comb_core/n4096/seed3_L1_Q1 |
| hard_scaling | comb_core | 8192 | 3 | 2.946 | 102160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/comb_core/n8192/seed3_L1_Q1 |
| hard_scaling | multi_comb_core | 4096 | 1 | 2.048 | 25360 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/hard_scaling/multi_comb_core/n4096/seed1_L1_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 2.010 | 41952 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 1 | 1.929 | 42224 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/correctness_fuzz/broom_mixed/n1024/seed1_L0_Q1 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.838 | 44160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q0 |
| correctness_fuzz | broom_mixed | 1024 | 5 | 1.838 | 44160 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/strong_gate_debug_stagefilter/runs/correctness_fuzz/broom_mixed/n1024/seed5_L0_Q1 |
