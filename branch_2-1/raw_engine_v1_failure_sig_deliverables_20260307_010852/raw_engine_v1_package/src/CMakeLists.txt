add_library(raw_engine_lib
    raw_core.cpp
    raw_validators.cpp
    raw_primitives.cpp
    raw_planner.cpp
)

target_include_directories(raw_engine_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

raw_engine_apply_common_flags(raw_engine_lib)
