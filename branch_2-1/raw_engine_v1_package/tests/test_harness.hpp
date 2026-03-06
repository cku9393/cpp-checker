#pragma once

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>

#include "raw_engine/raw_engine.hpp"

struct TestOptions {
    std::string caseName = "all";
    std::optional<u32> seed;
    std::optional<int> iters;
    std::optional<std::string> reproFile;
    std::string executablePath;
    std::size_t stepBudget = 200000;
    int repeat = 1;
    bool verbose = false;
};

void install_failure_handlers();
void print_usage(std::ostream& os);
int run_test_suite(const TestOptions& options);
