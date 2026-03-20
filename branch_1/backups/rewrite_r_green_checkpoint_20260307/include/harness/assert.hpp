#pragma once
#include <stdexcept>
#include <string>

namespace harness {
struct HarnessError : std::runtime_error {
    explicit HarnessError(const std::string &msg) : std::runtime_error(msg) {}
};
}

#define HASSERT(cond, msg) \
    do { if (!(cond)) throw ::harness::HarnessError(std::string("[HARNESS] ") + (msg)); } while (0)
