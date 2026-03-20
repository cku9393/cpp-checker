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
| balanced_dense | 0 | 0.002 | 1.015 | 0.013 | 0.013 | 64:0.013, 128:0.012, 256:0.013 |
| caterpillar_rect_dense | 0 | 0.519 | 1.990 | 0.025 | 0.025 | 64:0.012, 128:0.013, 256:0.025 |
| comb_core | 0 | 0.017 | 1.024 | 0.013 | 0.014 | 64:0.012, 128:0.013, 256:0.013 |
| multi_comb_rect | 0 | -0.054 | 1.000 | 0.013 | 0.013 | 64:0.013, 128:0.013, 256:0.012 |
| random_recursive_mixed | 0 | -0.038 | 0.999 | 0.013 | 0.013 | 64:0.013, 128:0.013, 256:0.012 |

## Top slow cases

| stage | mode | n | seed | sec | rss_kb | case_dir |
| --- | --- | --- | --- | --- | --- | --- |
| smoke_correctness | caterpillar_rect_dense | 256 | 1 | 0.025 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed1_L1_Q1 |
| smoke_correctness | caterpillar_rect_dense | 256 | 1 | 0.025 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed1_L0_Q1 |
| smoke_correctness | caterpillar_rect_dense | 256 | 2 | 0.025 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed2_L0_Q0 |
| smoke_correctness | caterpillar_rect_dense | 256 | 2 | 0.025 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed2_L1_Q1 |
| smoke_correctness | caterpillar_rect_dense | 256 | 2 | 0.025 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed2_L1_Q0 |
| smoke_correctness | caterpillar_rect_dense | 256 | 1 | 0.024 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed1_L1_Q0 |
| smoke_correctness | caterpillar_rect_dense | 256 | 2 | 0.024 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed2_L0_Q1 |
| smoke_correctness | caterpillar_rect_dense | 256 | 1 | 0.023 | 2576 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n256/seed1_L0_Q0 |
| smoke_correctness | comb_core | 64 | 1 | 0.014 | 1888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/comb_core/n64/seed1_L0_Q0 |
| smoke_correctness | comb_core | 128 | 1 | 0.013 | 1888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/comb_core/n128/seed1_L1_Q0 |
| smoke_correctness | balanced_dense | 256 | 2 | 0.013 | 2256 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/balanced_dense/n256/seed2_L1_Q1 |
| smoke_correctness | multi_comb_rect | 128 | 2 | 0.013 | 2128 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/multi_comb_rect/n128/seed2_L1_Q0 |
| smoke_correctness | caterpillar_rect_dense | 128 | 2 | 0.013 | 2144 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n128/seed2_L0_Q1 |
| smoke_correctness | balanced_dense | 256 | 1 | 0.013 | 2256 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/balanced_dense/n256/seed1_L1_Q1 |
| smoke_correctness | comb_core | 64 | 2 | 0.013 | 1888 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/comb_core/n64/seed2_L1_Q0 |
| smoke_correctness | balanced_dense | 256 | 1 | 0.013 | 2240 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/balanced_dense/n256/seed1_L0_Q1 |
| smoke_correctness | balanced_dense | 256 | 2 | 0.013 | 2240 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/balanced_dense/n256/seed2_L0_Q0 |
| smoke_correctness | caterpillar_rect_dense | 64 | 1 | 0.013 | 1984 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/caterpillar_rect_dense/n64/seed1_L1_Q0 |
| smoke_correctness | multi_comb_rect | 128 | 1 | 0.013 | 2128 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/multi_comb_rect/n128/seed1_L0_Q1 |
| smoke_correctness | random_recursive_mixed | 128 | 2 | 0.013 | 2064 | /Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/_verify_xplat/gate_smoke/runs/smoke_correctness/random_recursive_mixed/n128/seed2_L0_Q0 |
