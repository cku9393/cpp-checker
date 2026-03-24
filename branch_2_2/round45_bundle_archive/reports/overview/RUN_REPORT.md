# Flatten SPQR integration run report

통합 내용

1. `flatten_spqr_current_merged.cpp`를 `solve.cpp`로 교체했다.
2. 원래 zip 안의 솔버는 `solve_original_from_zip.cpp`로 백업했다.
3. 참고용으로 원본 코드와 진행 요약 문서를 같은 폴더에 함께 넣었다.

컴파일

`./build.sh` 성공. wall time 11.99s, peak RSS 351116KB.

실행 요약

| mode | N | seed | shuffle labels | shuffle queries | result | sec | mem KB |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: |
| balanced_dense | 512 | 1 | 1 | 1 | PASS | 0.05 | 5288 |
| caterpillar_rect_dense | 1024 | 1 | 1 | 1 | PASS | 3.42 | 14104 |
| caterpillar_rect_dense | 2048 | 1 | 1 | 1 | PASS | 12.89 | 21856 |
| chain_unary | 1024 | 1 | 1 | 1 | PASS | 0.90 | 14912 |
| chain_unary | 1536 | 1 | 1 | 1 | PASS | 1.68 | 15992 |
| chain_unary | 2048 | 1 | 1 | 1 | PASS | 3.07 | 15968 |
| comb_plus_unary | 4096 | 1 | 1 | 1 | PASS | 0.42 | 4272 |
| comb_rect_dense | 1024 | 1 | 1 | 1 | PASS | 3.72 | 14064 |
| comb_rect_dense | 2048 | 1 | 1 | 1 | PASS | 12.99 | 21992 |
| multi_comb_cap | 1024 | 1 | 1 | 1 | PASS | 0.82 | 9868 |
| multi_comb_cap | 2048 | 1 | 1 | 1 | PASS | 3.42 | 19348 |
| multi_comb_rect | 1024 | 1 | 1 | 1 | PASS | 1.34 | 13692 |
| multi_comb_rect | 2048 | 1 | 1 | 1 | PASS | 5.83 | 22156 |
| random_recursive_mixed | 512 | 1 | 1 | 1 | PASS | 0.05 | 4368 |

관찰

현재 병합본은 대표 체인 계열과 중간 크기 hard family를 모두 통과했다.
특히 `chain_unary`, `comb_plus_unary`, `multi_comb_cap`, `multi_comb_rect`는 이번 재실행에서도 안정적으로 검증을 통과했다.
반면 `comb_rect_dense`와 `caterpillar_rect_dense`는 N이 1024에서 2048로 커질 때 시간이 뚜렷하게 증가했다. 따라서 다음 병목 제거 우선순위는 여전히 dense rectangular 계열로 보는 편이 맞다.
이 보고서는 full preset gate 전체를 끝까지 돈 결과가 아니라, 현재 세션에서 직접 재실행한 대표 케이스 묶음의 결과다.