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

const char* primitive_name(PrimitiveKind primitive);
std::optional<PrimitiveKind> parse_primitive_kind(const std::string& text);
std::string primitive_name_string(PrimitiveKind primitive);

void save_state_dump(const std::filesystem::path& path, const LiveStateDump& dump);
LiveStateDump load_state_dump(const std::filesystem::path& path);

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

bool replay_state_dump(
    const TestOptions& options,
    const LiveStateDump& dump,
    PrimitiveKind primitive,
    bool oracle,
    std::string* failure
);

std::filesystem::path reduce_state_dump_file(
    const TestOptions& options,
    const std::filesystem::path& stateFile,
    PrimitiveKind primitive,
    bool oracle
);

LiveStateDump make_reducer_smoke_state();

