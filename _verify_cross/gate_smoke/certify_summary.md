# Certification summary

overall verdict: **PASS**

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

## Stage: smoke_correctness

status: **PASS**  
cases: 120  
timeouts: 0  
re/wa: 0  

| mode | bad_cases | alpha | worst_ratio | max_median_sec | worst_case_sec | median_sec_by_n |
| --- | --- | --- | --- | --- | --- | --- |
| balanced_dense | 0 | -0.008 | 1.007 | 0.013 | 0.015 | 64:0.013, 128:0.013, 256:0.012 |
| caterpillar_rect_dense | 0 | 0.514 | 1.946 | 0.024 | 0.025 | 64:0.012, 128:0.012, 256:0.024 |
| comb_core | 0 | 0.018 | 1.032 | 0.013 | 0.135 | 64:0.012, 128:0.012, 256:0.013 |
| multi_comb_rect | 0 | 0.001 | 1.035 | 0.013 | 0.013 | 64:0.013, 128:0.012, 256:0.013 |
| random_recursive_mixed | 0 | 0.015 | 1.102 | 0.013 | 0.013 | 64:0.012, 128:0.011, 256:0.013 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| smoke_correctness | comb_core | 64 | 1 | 0.135 | 1888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/comb_core/n64/seed1_L0_Q0 |
| smoke_correctness | caterpillar_rect_dense | 256 | 1 | 0.025 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed1_L1_Q1 |
| smoke_correctness | caterpillar_rect_dense | 256 | 2 | 0.024 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed2_L0_Q1 |
| smoke_correctness | caterpillar_rect_dense | 256 | 2 | 0.024 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed2_L1_Q0 |
| smoke_correctness | caterpillar_rect_dense | 256 | 2 | 0.024 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed2_L1_Q1 |
| smoke_correctness | caterpillar_rect_dense | 256 | 2 | 0.024 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed2_L0_Q0 |
| smoke_correctness | caterpillar_rect_dense | 256 | 1 | 0.024 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed1_L0_Q0 |
| smoke_correctness | caterpillar_rect_dense | 256 | 1 | 0.023 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed1_L1_Q0 |
| smoke_correctness | caterpillar_rect_dense | 256 | 1 | 0.021 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed1_L0_Q1 |
| smoke_correctness | balanced_dense | 256 | 2 | 0.015 | 2240 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/balanced_dense/n256/seed2_L0_Q0 |
| smoke_correctness | comb_core | 64 | 2 | 0.013 | 1888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/comb_core/n64/seed2_L0_Q1 |
| smoke_correctness | multi_comb_rect | 256 | 2 | 0.013 | 2464 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/multi_comb_rect/n256/seed2_L0_Q1 |
| smoke_correctness | multi_comb_rect | 256 | 2 | 0.013 | 2464 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/multi_comb_rect/n256/seed2_L1_Q0 |
| smoke_correctness | balanced_dense | 128 | 1 | 0.013 | 2064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/balanced_dense/n128/seed1_L0_Q0 |
| smoke_correctness | balanced_dense | 64 | 1 | 0.013 | 2000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/balanced_dense/n64/seed1_L1_Q1 |
| smoke_correctness | caterpillar_rect_dense | 128 | 2 | 0.013 | 2144 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n128/seed2_L0_Q1 |
| smoke_correctness | balanced_dense | 64 | 2 | 0.013 | 2000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/balanced_dense/n64/seed2_L1_Q0 |
| smoke_correctness | comb_core | 128 | 2 | 0.013 | 1888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/comb_core/n128/seed2_L0_Q0 |
| smoke_correctness | balanced_dense | 64 | 1 | 0.013 | 2000 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/balanced_dense/n64/seed1_L0_Q0 |
| smoke_correctness | random_recursive_mixed | 64 | 2 | 0.013 | 1984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_cross/gate_smoke/runs/smoke_correctness/random_recursive_mixed/n64/seed2_L1_Q0 |
