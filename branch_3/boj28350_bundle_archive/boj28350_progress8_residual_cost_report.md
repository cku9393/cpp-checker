# boj28350 progress 8 residual cost attribution report

이번 라운드는 semantics를 바꾸지 않고 profiling과 logging만 바꾼 결과다. current exact backend, global delete artifact line, split-required class line, preserved-piece-hit route는 유지했다.

## 변경 핵심

첫째, LOCAL profiling mode를 `PROFILE_NONE`, `PROFILE_BASE`, `PROFILE_SAMPLED`로 분리했다. `PROFILE_NONE`은 correctness gate 전용으로 time counter를 모두 끈다. `PROFILE_BASE`는 coarse cumulative time과 총량 counter만 남긴다. `PROFILE_SAMPLED`는 coarse cumulative time을 전 deletion에 대해 유지하면서, `PROFILE_SAMPLE_WARMUP=64`, `PROFILE_SAMPLE_STRIDE=8` 규칙으로 sampled deletion에만 세부 timer를 켠다. sampled 여부와 rule은 stderr summary에 그대로 남긴다.

둘째, long LOCAL run에서 `out.txt`, `time.txt`가 비는 문제를 막기 위해 line buffered progress checkpoint를 넣었다. case start, init done, deletion checkpoint, final summary 시점마다 stderr에 flush한다. 이번 512 LOCAL run들은 모두 `debug_progress_checkpoint_calls=19`, `debug_progress_last_deletion=512`로 끝까지 회수됐다.

셋째, `time_route_dispatch_ns`를 `time_unanimous_mode_dispatch_ns`, `time_terminal_collection_ns`, `time_vertex_lookup_ns`, `time_watch_diff_build_ns`, `time_state_publish_ns`로 분해했다. `time_connector_skeleton_build_ns`는 `time_connector_skeleton_terminal_collection_ns`, `time_connector_skeleton_terminal_dedupe_ns`, `time_connector_skeleton_vertexset_build_ns`, `time_connector_skeleton_vertex_lookup_build_ns`, `time_connector_skeleton_core_build_ns`로 분해했다. connector skeleton build detail은 `buildSupportProductFromLastDeleteArtifact` 내부 단계까지 내려가서 계측하도록 정리했다.

넷째, sampled mode에서도 top 10 slow deletion을 전체 deletion 기준으로 유지하도록 넣었다. 각 record는 deletion index, deleted vertex, touched class count, connector skeleton terminals, connector skeleton vertices, watch removed, watch added, preserved piece split vertices, global delete DFS edges, query incident scans, total deletion time을 남긴다.

## correctness gate

| case | validator | local_active_mismatch | local_active_partition_mismatch | debug_touched_missing_classes | piece_materialize_fallback_calls | support_rebuild_fallback_calls | unanimous_baseline_path_calls |
| --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only comb_rect_dense 256 PROFILE_NONE | OK | 0 | 0 | 0 | 0 | 0 | 0 |
| both_on comb_rect_dense 256 PROFILE_NONE | OK | 0 | 0 | 0 | 0 | 0 | 0 |
| both_on multi_comb_rect 512 PROFILE_SAMPLED | OK | 0 | 0 | 0 | 0 | 0 | 0 |

세 gate run 모두 조건을 유지했다. 따라서 cost attribution 단계로 진행했다.

## connector skeleton rebuild 선택 여부

| case | actual_calls | selected_classes | selected_connector_only | selected_both_on | watch_reused | watch_removed | watch_added |
| --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only comb_rect_dense 256 BASE | 7561 | 7561 | 7561 | 0 | 139950 | 7166 | 7353 |
| connector_only comb_rect_dense 512 BASE | 30887 | 30887 | 30887 | 0 | 1683816 | 29122 | 30186 |
| both_on comb_rect_dense 512 BASE | 30887 | 30887 | 0 | 30887 | 1686551 | 29122 | 27451 |
| both_on multi_comb_rect 512 SAMPLED | 7581 | 7581 | 0 | 7581 | 190351 | 6933 | 6455 |

정리하면 connector skeleton rebuild는 실제로 강하게 선택된다. 특히 dense 512에서는 `connector_skeleton_actual_calls=30887`이고 선택된 class 수도 동일하다. watch diff도 계속 작동해서 reused 비중이 dense 512 기준 약 96.6퍼센트에서 96.8퍼센트다. 따라서 “connector skeleton rebuild가 선택되는가”의 답은 예다. 다만 이번 round의 핵심 질문인 residual cost 지배축은 다른 곳에 남아 있었다.

## 주요 시간 분해 표

아래 표의 coarse time은 case 전체 deletion에 대한 cumulative time이다. explicit total은 표에 나온 계측 counter의 합이며 wall time과 일치하지 않는 나머지는 계측하지 않은 outside cost다.

| case | profile | wall_s | route_dispatch_s | global_delete_dfs_s | connector_build_s | watch_unreg_s | watch_reg_s | piece_split_s | query_scan_s | explicit_total_s | explicit_vs_wall |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only comb_rect_dense 256 LOCAL | PROFILE_BASE | 1.949 | 1.111 | 0.009 | 0.063 | 0.025 | 0.034 | 0.008 | 0.001 | 1.250 | 64.2% |
| connector_only comb_rect_dense 512 LOCAL | PROFILE_BASE | 35.669 | 22.409 | 0.059 | 0.762 | 0.347 | 0.279 | 0.052 | 0.002 | 23.911 | 67.0% |
| both_on comb_rect_dense 512 LOCAL | PROFILE_BASE | 36.804 | 23.500 | 0.055 | 0.753 | 0.329 | 0.214 | 0.034 | 0.002 | 24.886 | 67.6% |
| both_on multi_comb_rect 512 LOCAL | PROFILE_SAMPLED | 5.069 | 3.116 | 0.019 | 0.119 | 0.061 | 0.049 | 0.007 | 0.001 | 3.372 | 66.5% |

관측은 일관된다. 네 case 모두 explicit time의 88.9퍼센트에서 94.4퍼센트가 `time_route_dispatch_ns`에 있다. 512 dense에서는 `connector skeleton build + watch register + watch unregister`를 모두 합쳐도 explicit total의 5.7퍼센트 안팎이고, route dispatch가 93퍼센트 이상을 먹는다. 즉 coarse view만 봐도 residual이 dispatch umbrella 안에 크게 남아 있다는 가설이 이미 강하다.

## sampled 세부 attribution

아래 표는 `PROFILE_SAMPLED`의 세부 timer 합이다. sample rule은 `warmup=64`, `stride=8`이고, 세부 timer는 sampled deletion `120/512`에 대해서만 누적된다. 따라서 아래 합계는 wall time과 직접 비교하지 않고 axis 간 상대 크기 비교에 사용해야 한다.

| case | sample_rule | sampled_deletions | skeleton_core_s | watch_churn_s | dispatch_internal_s | global_delete_dfs_s | piece_split_s | query_scan_s | dominant_axis | dominant_share |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only comb_rect_dense 512 LOCAL | stride=8, warmup=64 | 120/512 | 0.257 | 1.972 | 8.213 | 0.059 | 0.053 | 0.002 | dispatch umbrella 내부 | 77.8% |
| both_on comb_rect_dense 512 LOCAL | stride=8, warmup=64 | 120/512 | 0.279 | 1.980 | 10.061 | 0.058 | 0.040 | 0.002 | dispatch umbrella 내부 | 81.0% |
| both_on multi_comb_rect 512 LOCAL | stride=8, warmup=64 | 120/512 | 0.030 | 0.186 | 1.423 | 0.019 | 0.007 | 0.001 | dispatch umbrella 내부 | 85.4% |

세 sampled case 모두 `dispatch umbrella 내부`가 50퍼센트를 크게 넘는다. connector_only dense 512는 77.8퍼센트, both_on dense 512는 81.0퍼센트, both_on multi 512는 85.4퍼센트다. user가 제시한 dominant 판정 규칙을 그대로 적용하면 다음 pivot은 `dispatch umbrella 내부 최적화`다.

## connector skeleton build core 세부 분해

| case | terminal_collection_s | terminal_dedupe_s | vertexset_build_s | vertex_lookup_build_s | core_build_s | sampled_build_detail_total_s | largest_inside_build |
| --- | --- | --- | --- | --- | --- | --- | --- |
| connector_only comb_rect_dense 512 LOCAL | 0.003 | 0.001 | 0.213 | 0.002 | 0.041 | 0.259 | vertexset_build 82.0% |
| both_on comb_rect_dense 512 LOCAL | 0.003 | 0.001 | 0.215 | 0.002 | 0.060 | 0.281 | vertexset_build 76.8% |
| both_on multi_comb_rect 512 LOCAL | 0.001 | 0.000 | 0.025 | 0.000 | 0.005 | 0.031 | vertexset_build 81.4% |

connector skeleton build 자체를 더 쪼개 보면 build 내부에서는 `vertexset_build`가 가장 크다. dense 512 sampled 기준 connector_only는 build detail의 82.0퍼센트, both_on은 76.8퍼센트, multi는 81.4퍼센트가 여기에 있다. 하지만 build core 축 전체가 sampled attribution total에서 차지하는 비중은 1.8퍼센트에서 2.4퍼센트에 불과하다. 따라서 이번 round 기준으로는 “connector skeleton state reduction”이 다음 pivot이 아니다.

watch churn 안에서는 `time_watch_diff_build_ns`가 제일 크다. dense 512 sampled에서 connector_only는 watch churn 내부의 68.8퍼센트, both_on은 71.8퍼센트다. 다만 watch churn 축 전체가 sampled attribution total에서 15.9퍼센트에서 18.7퍼센트 수준이므로 지배축은 아니다. watch compression을 더 세게 넣는 것은 두 번째 후보 정도로 보는 편이 맞다.

dispatch umbrella 내부에서는 `time_unanimous_mode_dispatch_ns`가 핵심이다. dense 512 sampled에서 connector_only는 dispatch internal의 88.7퍼센트, both_on은 83.4퍼센트, multi는 79.8퍼센트가 여기에 있다. 그 다음이 `time_state_publish_ns`다. `time_terminal_collection_ns`와 `time_vertex_lookup_ns`는 모두 미미하다. 따라서 dispatch umbrella 내부 최적화라고 해도 우선순위는 사실상 unanimous dispatch path와 state publish path다.

## top slow deletions 요약

### connector_only comb_rect_dense 512 PROFILE_SAMPLED 상위 5개

| rank | idx | x | touched | terms | skelV | unreg | reg | splitV | gdfsE | qscan | total_ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 55 | 54 | 223 | 8111 | 17674 | 190 | 200 | 2516 | 40690 | 84 | 281.095 |
| 1 | 91 | 90 | 204 | 7419 | 14521 | 196 | 212 | 388 | 35384 | 85 | 271.238 |
| 2 | 42 | 42 | 230 | 8173 | 19151 | 179 | 191 | 4008 | 42496 | 90 | 271.132 |
| 3 | 36 | 36 | 232 | 8025 | 19644 | 187 | 175 | 3854 | 43448 | 0 | 270.681 |
| 4 | 50 | 50 | 226 | 8167 | 18244 | 184 | 213 | 3060 | 41296 | 176 | 270.320 |

### both_on comb_rect_dense 512 PROFILE_SAMPLED 상위 5개

| rank | idx | x | touched | terms | skelV | unreg | reg | splitV | gdfsE | qscan | total_ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 55 | 54 | 223 | 8111 | 17674 | 190 | 174 | 1258 | 40690 | 84 | 314.308 |
| 1 | 38 | 38 | 232 | 8004 | 19303 | 183 | 163 | 1999 | 43136 | 193 | 313.424 |
| 2 | 45 | 44 | 229 | 8132 | 18802 | 184 | 181 | 1756 | 42172 | 175 | 312.302 |
| 3 | 41 | 40 | 230 | 8117 | 19321 | 180 | 139 | 2026 | 42826 | 0 | 311.200 |
| 4 | 85 | 84 | 209 | 7762 | 15438 | 196 | 194 | 410 | 36272 | 89 | 307.860 |

### both_on multi_comb_rect 512 PROFILE_SAMPLED 상위 5개

| rank | idx | x | touched | terms | skelV | unreg | reg | splitV | gdfsE | qscan | total_ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 25 | 22 | 118 | 4208 | 4558 | 69 | 40 | 2910 | 30200 | 152 | 127.222 |
| 1 | 33 | 30 | 118 | 4275 | 4355 | 83 | 56 | 1907 | 29234 | 257 | 122.021 |
| 2 | 21 | 18 | 118 | 4059 | 4626 | 61 | 47 | 3563 | 30700 | 167 | 113.385 |
| 3 | 37 | 34 | 117 | 4395 | 4356 | 62 | 42 | 3033 | 28746 | 112 | 110.643 |
| 4 | 13 | 10 | 113 | 3349 | 4199 | 49 | 23 | 3895 | 31660 | 159 | 108.517 |

느린 deletion의 공통 패턴은 명확하다. dense 512의 최상위 slow deletion은 대부분 `touched`가 200 안팎 이상이고, `terms`가 7400에서 8100대, `skelV`가 1.4만에서 1.9만대다. 그런데 per deletion `t_skel_ns` 자체는 대략 6.6밀리초에서 8.0밀리초 수준이고, total deletion time은 270밀리초에서 314밀리초다. 즉 느린 deletion에서도 skeleton rebuild 자체보다 dispatch side work가 훨씬 크다. multi case도 같은 결론이고, 다만 절대량이 더 작다.

## release 결과

| case | elapsed_s | rc | validator | note |
| --- | --- | --- | --- | --- |
| both_on comb_rect_dense 1024 RELEASE | 120.023 | 124 | FAIL | timeout 120s |
| both_on multi_comb_rect 1024 RELEASE | 43.147 | 0 | OK | completed |
| both_on comb_rect_dense 4096 RELEASE | 83.183 | 1 | FAIL | rc=1 no output |
| both_on multi_comb_rect 4096 RELEASE | 90.056 | 124 | FAIL | timeout 90s |

best effort 기준으로 `multi_comb_rect 1024 RELEASE`는 43.147초에 validator OK로 끝났다. `comb_rect_dense 1024 RELEASE`는 120초 timeout 안에 회수하지 못했다. representative 4096에서는 `comb_rect_dense 4096 RELEASE`가 83.183초에 rc=1로 종료했고 stderr는 비어 있었으며, `multi_comb_rect 4096 RELEASE`는 90초 timeout 안에 회수하지 못했다. 따라서 4096은 현재 container 예산 안에서는 완주 결과 대신 failure mode만 확보했다.

## 최종 판단

coarse layer에서는 `time_route_dispatch_ns`가 explicit measured time의 대부분을 먹는다. sampled internal attribution에서는 user가 제시한 여섯 축 중 `dispatch umbrella 내부`가 세 대표 case 모두에서 50퍼센트를 크게 넘는다. `connector skeleton build core`는 2퍼센트 전후, `watch churn`은 11퍼센트에서 19퍼센트, `global_delete_dfs`, `preserved piece split`, `query incident scans`는 모두 미미하다.

따라서 다음 optimization pivot은 `dispatch umbrella 내부`, 더 구체적으로는 `time_unanimous_mode_dispatch_ns`와 `time_state_publish_ns`를 줄이는 방향으로 가는 것이 맞다. watch compression은 그 다음 순위다. connector skeleton state reduction은 이번 계측 데이터만으로는 우선순위를 올릴 근거가 부족하다.

residual cost dominated by dispatch umbrella 내부
