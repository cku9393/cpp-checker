#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "raw_engine/raw_engine.hpp"

struct TestOptions;

enum class PrimitiveKind : u8 {
    ISOLATE = 0,
    SPLIT = 1,
    JOIN = 2,
    INTEGRATE = 3,
};

enum class FailureClass : u8 {
    NONE = 0,
    VALIDATOR_FAILURE = 1,
    PRIMITIVE_ORACLE_MISMATCH = 2,
    PLANNER_ORACLE_MISMATCH = 3,
    STEP_BUDGET_EXCEEDED = 4,
    SANITIZER_FAILURE = 5,
    CRASH = 6,
};

struct FailureSignature {
    FailureClass failureClass = FailureClass::NONE;
    std::string hash;
    std::string detail;

    bool empty() const;
    bool same_failure(const FailureSignature& rhs) const;
};

struct PrimitiveInvocation {
    PrimitiveKind primitive = PrimitiveKind::ISOLATE;
    RawSkelID sid = NIL_U32;
    OccID occ = NIL_U32;
    RawSkelID leftSid = NIL_U32;
    RawSkelID rightSid = NIL_U32;
    RawSkelID parentSid = NIL_U32;
    RawSkelID childSid = NIL_U32;
    Vertex aOrig = NIL_U32;
    Vertex bOrig = NIL_U32;
    std::vector<BoundaryMapEntry> boundaryMap;
    std::vector<OccID> keepOcc;
    std::vector<u32> throughBranches;
    std::vector<PrimitiveKind> sequence;
};

struct LiveStateDump {
    PrimitiveInvocation invocation;
    RawEngine engine;
};

struct PlannerStateDump {
    RawEngine engine;
    std::string caseName = "planner";
    std::optional<u32> seed;
    int iter = -1;
    OccID targetOcc = 0;
    std::vector<OccID> keepOcc;
    std::size_t stepBudget = 0;
    bool dumpTrace = false;
};

const char* primitive_name(PrimitiveKind primitive);
std::optional<PrimitiveKind> parse_primitive_kind(const std::string& text);
std::string primitive_name_string(PrimitiveKind primitive);
const char* failure_class_name(FailureClass failureClass);
std::string failure_class_name_string(FailureClass failureClass);
FailureSignature make_failure_signature(FailureClass failureClass, const std::string& detail);
std::filesystem::path resolve_artifact_dir(const TestOptions& options);
std::filesystem::path artifact_subdir(const TestOptions& options, const std::string& leaf);

void save_state_dump(const std::filesystem::path& path, const LiveStateDump& dump);
LiveStateDump load_state_dump(const std::filesystem::path& path);
void save_planner_state_dump(const std::filesystem::path& path, const PlannerStateDump& dump);
PlannerStateDump load_planner_state_dump(const std::filesystem::path& path);

void set_pending_dump_path(const std::optional<std::filesystem::path>& path);
std::optional<std::filesystem::path> pending_dump_path();

class PrimitiveCallDumpGuard {
public:
    PrimitiveCallDumpGuard(
        const TestOptions& options,
        const RawEngine& engine,
        const PrimitiveInvocation& invocation,
        const std::string& label
    );
    ~PrimitiveCallDumpGuard();

    PrimitiveCallDumpGuard(const PrimitiveCallDumpGuard&) = delete;
    PrimitiveCallDumpGuard& operator=(const PrimitiveCallDumpGuard&) = delete;

    const std::optional<std::filesystem::path>& path() const;
    void mark_success();

private:
    std::optional<std::filesystem::path> path_;
    bool success_ = false;
};

class PlannerCallDumpGuard {
public:
    PlannerCallDumpGuard(const TestOptions& options, const PlannerStateDump& dump, const std::string& label);
    ~PlannerCallDumpGuard();

    PlannerCallDumpGuard(const PlannerCallDumpGuard&) = delete;
    PlannerCallDumpGuard& operator=(const PlannerCallDumpGuard&) = delete;

    const std::optional<std::filesystem::path>& path() const;
    void mark_success();

private:
    std::optional<std::filesystem::path> path_;
    bool success_ = false;
};

bool replay_state_dump(
    const TestOptions& options,
    const LiveStateDump& dump,
    PrimitiveKind primitive,
    bool oracle,
    FailureSignature* failure
);

std::filesystem::path reduce_state_dump_file(
    const TestOptions& options,
    const std::filesystem::path& stateFile,
    PrimitiveKind primitive,
    bool oracle
);

bool replay_planner_state_dump(
    const TestOptions& options,
    const PlannerStateDump& dump,
    bool oracle,
    FailureSignature* failure
);

std::filesystem::path reduce_planner_state_dump_file(
    const TestOptions& options,
    const std::filesystem::path& stateFile,
    bool oracle
);

LiveStateDump make_reducer_smoke_state();
PlannerStateDump make_planner_reducer_smoke_state();
