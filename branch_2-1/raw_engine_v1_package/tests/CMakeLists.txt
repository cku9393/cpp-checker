add_executable(raw_engine_tests
    raw_engine_cases.cpp
    raw_engine_main.cpp
)

target_include_directories(raw_engine_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(raw_engine_tests PRIVATE raw_engine_lib)

raw_engine_apply_common_flags(raw_engine_tests)

add_test(NAME raw_engine_micro COMMAND raw_engine_tests --case micro)
add_test(NAME raw_engine_roundtrip COMMAND raw_engine_tests --case roundtrip)
add_test(NAME raw_engine_split_integrate_ensure COMMAND raw_engine_tests --case split_integrate_ensure)
add_test(NAME raw_engine_regression_44001 COMMAND raw_engine_tests --case regression_44001)
add_test(NAME raw_engine_regression_isolate_split_no_sep COMMAND raw_engine_tests --case regression_isolate_split_no_sep)
add_test(NAME raw_engine_planner COMMAND raw_engine_tests --case planner)
add_test(NAME raw_engine_fuzz_matrix COMMAND raw_engine_tests --case fuzz_matrix)
add_test(NAME raw_engine_fuzz COMMAND raw_engine_tests --case fuzz)

if (RAW_ENGINE_ENABLE_ASAN OR RAW_ENGINE_ENABLE_UBSAN)
    foreach(test_name
            raw_engine_micro
            raw_engine_roundtrip
            raw_engine_split_integrate_ensure
            raw_engine_regression_44001
            raw_engine_regression_isolate_split_no_sep
            raw_engine_planner
            raw_engine_fuzz_matrix
            raw_engine_fuzz)
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1;UBSAN_OPTIONS=print_stacktrace=1"
        )
    endforeach()
endif()
