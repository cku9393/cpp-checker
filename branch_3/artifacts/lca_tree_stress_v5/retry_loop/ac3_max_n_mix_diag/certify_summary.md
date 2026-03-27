# Certification summary

overall verdict: **FAIL**

## Reasons

- max_n_mix: 28 failing cases

이 스위트는 hidden data를 완전히 복제하는 증명은 아니지만, decomposition 계열 느린 풀이와 잘못된 풀이를 매우 강하게 걸러내도록 설계됐다.

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
