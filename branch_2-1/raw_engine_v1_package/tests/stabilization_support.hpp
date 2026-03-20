#pragma once

#include <string>

#include "test_harness.hpp"

enum class PrimitiveFaultKind : u8 {
    NONE = 0,
    DROP_ALLOC_NBR = 1,
    DUPLICATE_ALLOC_NBR = 2,
    WRONG_HOST_SKEL = 3,
    WRONG_CENTER_V = 4,
    DROP_CORE_PATCH_EDGE = 5,
    ADD_CENTER_INCIDENT_CORE_EDGE = 6,
    DROP_SKELETON_EDGE = 7,
    REWIRE_SKELETON_EDGE = 8,
    FLIP_BOUNDARY_ONLY = 9,
};

enum class PlannerFaultKind : u8 {
    NONE = 0,
    OMIT_AFTER_SPLIT_JOIN = 1,
    OMIT_AFTER_SPLIT_INTEGRATE = 2,
    OMIT_ENSURE_SOLE_AFTER_JOIN = 3,
    OMIT_ENSURE_SOLE_AFTER_INTEGRATE = 4,
    WRONG_BRANCH_ROUTING = 5,
};

struct FaultDetectionResult {
    bool validatorDetected = false;
    bool primitiveOracleDetected = false;
    bool plannerOracleDetected = false;
    std::string detail;
};

std::string primitive_fault_name_string(PrimitiveFaultKind fault);
std::string planner_fault_name_string(PlannerFaultKind fault);

bool validate_engine_state_soft(const RawEngine& RE, std::string* error = nullptr);
void inject_primitive_fault(RawEngine& RE, PrimitiveFaultKind fault, OccID targetOcc);

PlannerExecutionResult run_planner_checked_capture_fault_injected(
    RawEngine& RE,
    const RawPlannerCtx& ctx,
    RawUpdateCtx& U,
    const RawUpdaterRunOptions& runOptions,
    PlannerFaultKind fault,
    const std::vector<UpdJob>* initialQueue = nullptr,
    const std::string& label = {}
);
