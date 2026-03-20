#pragma once

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "raw_engine/raw_engine.hpp"
#include "reference_planner.hpp"

enum class OracleMode : u8 {
    NONE = 0,
    PRIMITIVE = 1,
    PLANNER = 2,
    ALL = 3,
};

enum class TraceLevel : u8 {
    NONE = 0,
    SUMMARY = 1,
    FULL = 2,
};

enum class WeightProfile : u8 {
    RANDOM = 0,
    WEIGHTED_SPLIT_HEAVY = 1,
    WEIGHTED_JOIN_HEAVY = 2,
    WEIGHTED_INTEGRATE_HEAVY = 3,
    ARTIFACT_HEAVY = 4,
    MULTIEDGE_HEAVY = 5,
};

struct FailureContextSnapshot {
    std::string caseName = "none";
    std::optional<u32> seed;
    int iter = -1;
};

struct TestOptions {
    std::string caseName = "all";
    std::optional<u32> seed;
    std::optional<int> iters;
    std::optional<std::string> reproFile;
    std::optional<std::string> stateFile;
    std::optional<std::string> primitiveName;
    std::optional<std::string> artifactDir;
    std::optional<OccID> targetOcc;
    std::vector<OccID> keepOcc;
    std::string executablePath;
    std::size_t stepBudget = 200000;
    std::size_t maxArtifacts = 0;
    int repeat = 1;
    bool verbose = false;
    bool dumpOnFail = false;
    bool reduce = false;
    bool dumpTrace = false;
    bool stats = false;
    bool keepOnlyFailures = true;
    bool compressArtifacts = false;
    OracleMode oracleMode = OracleMode::NONE;
    TraceLevel traceLevel = TraceLevel::SUMMARY;
    WeightProfile weightProfile = WeightProfile::RANDOM;
    std::optional<std::string> statsFile;
};

void install_failure_handlers();
void print_usage(std::ostream& os);
int run_test_suite(const TestOptions& options);

void set_active_test_options(const TestOptions* options);
const TestOptions& active_test_options();
bool primitive_oracle_enabled(const TestOptions& options);
bool planner_oracle_enabled(const TestOptions& options);
bool active_primitive_oracle_enabled();
bool active_planner_oracle_enabled();
FailureContextSnapshot current_failure_context();
std::string current_failure_stem();
bool trace_enabled(const TestOptions& options);
bool active_trace_enabled();
std::string trace_level_name_string(TraceLevel traceLevel);
std::string weight_profile_name_string(WeightProfile profile);
const std::vector<PlannerTraceEntry>& last_planner_trace();

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

void run_planner_checked(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    const std::string& label = {}
);

void execute_planner_capture(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    const std::vector<UpdJob>* initialQueue,
    std::vector<PlannerTraceEntry>& trace,
    PlannerPhase* activePhase = nullptr
);
