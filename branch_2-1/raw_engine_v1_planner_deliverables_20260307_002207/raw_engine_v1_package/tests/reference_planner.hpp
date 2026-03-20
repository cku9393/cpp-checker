#pragma once

#include <string>
#include <vector>

#include "reference_model.hpp"

enum class PlannerPrimitiveKind : u8 {
    ISOLATE_VERTEX = 0,
    SPLIT = 1,
    JOIN = 2,
    INTEGRATE = 3,
};

struct PlannerTraceEntry {
    PlannerPrimitiveKind primitive = PlannerPrimitiveKind::ISOLATE_VERTEX;
    std::string summary;
    std::string hash;

    bool operator==(const PlannerTraceEntry& rhs) const;
};

struct PlannerResultSignature {
    EngineStateSignature finalState;
    bool stopConditionSatisfied = false;
    IsolatePreparedSignature targetPrepare;
    std::vector<PlannerTraceEntry> trace;

    bool operator==(const PlannerResultSignature& rhs) const;
};

struct PlannerRunResult {
    RawEngine engine;
    PlannerResultSignature signature;
};

const char* planner_primitive_name(PlannerPrimitiveKind primitive);
std::string describe_planner_trace(const std::vector<PlannerTraceEntry>& trace);
std::string describe_planner_signature(const PlannerResultSignature& signature);

PlannerRunResult run_reference_planner(
    const RawEngine& before,
    const RawPlannerCtx& ctx,
    const RawUpdaterRunOptions& runOptions,
    bool captureTrace
);

bool check_planner_oracle(
    const RawEngine& before,
    const RawEngine& after,
    const RawPlannerCtx& ctx,
    const RawUpdaterRunOptions& runOptions,
    const std::vector<PlannerTraceEntry>& actualTrace,
    std::string* failure
);
