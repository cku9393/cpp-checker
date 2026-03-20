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
    std::optional<std::string> stateFile;
    std::optional<std::string> primitiveName;
    std::string executablePath;
    std::size_t stepBudget = 200000;
    int repeat = 1;
    bool verbose = false;
    bool dumpOnFail = false;
    bool oracle = false;
    bool reduce = false;
};

void install_failure_handlers();
void print_usage(std::ostream& os);
int run_test_suite(const TestOptions& options);

void set_active_test_options(const TestOptions* options);
const TestOptions& active_test_options();
bool active_oracle_enabled();

IsolatePrepared prepare_isolate_checked(const RawEngine& RE, RawSkelID sid, OccID occ, const std::string& label = {});
SplitSeparationPairResult split_checked(
    RawEngine& RE,
    RawSkelID sid,
    Vertex saOrig,
    Vertex sbOrig,
    RawUpdateCtx& U,
    const std::string& label = {}
);
JoinSeparationPairResult join_checked(
    RawEngine& RE,
    RawSkelID leftSid,
    RawSkelID rightSid,
    Vertex saOrig,
    Vertex sbOrig,
    RawUpdateCtx& U,
    const std::string& label = {}
);
IntegrateResult integrate_checked(
    RawEngine& RE,
    RawSkelID parentSid,
    RawSkelID childSid,
    const std::vector<BoundaryMapEntry>& bm,
    RawUpdateCtx& U,
    const std::string& label = {}
);
