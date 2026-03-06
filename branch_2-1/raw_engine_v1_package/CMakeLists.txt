cmake_minimum_required(VERSION 3.20)

project(raw_engine_v1 LANGUAGES CXX)

option(RAW_ENGINE_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(RAW_ENGINE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(RAW_ENGINE_ENABLE_CONVERSION_WARNINGS "Enable -Wconversion" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(raw_engine_apply_common_flags target_name)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
        )

        if (RAW_ENGINE_ENABLE_CONVERSION_WARNINGS)
            target_compile_options(${target_name} PRIVATE -Wconversion)
        endif()

        set(sanitize_flags "")
        if (RAW_ENGINE_ENABLE_ASAN)
            list(APPEND sanitize_flags -fsanitize=address -fno-omit-frame-pointer)
        endif()
        if (RAW_ENGINE_ENABLE_UBSAN)
            list(APPEND sanitize_flags -fsanitize=undefined)
        endif()

        if (sanitize_flags)
            target_compile_options(${target_name} PRIVATE ${sanitize_flags})
            target_link_options(${target_name} PRIVATE ${sanitize_flags})
        endif()
    endif()
endfunction()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
