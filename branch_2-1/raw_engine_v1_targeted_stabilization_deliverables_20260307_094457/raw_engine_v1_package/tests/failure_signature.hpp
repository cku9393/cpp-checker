#pragma once

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "raw_engine/raw_engine.hpp"

struct EngineStateSignature;
struct IsolatePreparedSignature;
struct SplitResultSignature;
struct MergeResultSignature;
struct PlannerTraceEntry;

enum class PrimitiveKind : u8 {
    NONE = 0,
    ISOLATE = 1,
    SPLIT = 2,
    JOIN = 3,
    INTEGRATE = 4,
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

enum class FailureStage : u8 {
    NONE = 0,
    PRIMITIVE = 1,
    PLANNER = 2,
    REDUCER = 3,
    REPLAY = 4,
};

enum class PlannerPhase : u8 {
    NONE = 0,
    ENSURE_SOLE = 1,
    ISOLATE = 2,
    SPLIT = 3,
    JOIN = 4,
    INTEGRATE = 5,
};

enum class FailureMismatchKind : u8 {
    NONE = 0,
    VALIDATION = 1,
    PREPARE = 2,
    SPLIT_RESULT = 3,
    SPLIT_STATE = 4,
    JOIN_RESULT = 5,
    JOIN_STATE = 6,
    INTEGRATE_RESULT = 7,
    INTEGRATE_STATE = 8,
    FINAL_STATE = 9,
    STOP_CONDITION = 10,
    TARGET_PREPARE = 11,
    TRACE_PREFIX = 12,
    STEP_BUDGET = 13,
    SANITIZER = 14,
    CRASH = 15,
};

struct FailureSignature {
    FailureClass failureClass = FailureClass::NONE;
    FailureStage stage = FailureStage::NONE;
    PrimitiveKind primitiveKind = PrimitiveKind::NONE;
    PlannerPhase plannerPhase = PlannerPhase::NONE;
    OccID targetOcc = NIL_U32;
    FailureMismatchKind mismatchKind = FailureMismatchKind::NONE;
    std::string canonicalStateHash;
    std::string oracleHash;
    std::string tracePrefixHash;
    std::string detail;

    bool empty() const;
    bool same_failure(const FailureSignature& rhs) const;
};

const char* primitive_name(PrimitiveKind primitive);
std::optional<PrimitiveKind> parse_primitive_kind(const std::string& text);
std::string primitive_name_string(PrimitiveKind primitive);

const char* failure_class_name(FailureClass failureClass);
std::optional<FailureClass> parse_failure_class(std::string_view text);
std::string failure_class_name_string(FailureClass failureClass);

const char* failure_stage_name(FailureStage stage);
std::optional<FailureStage> parse_failure_stage(std::string_view text);
std::string failure_stage_name_string(FailureStage stage);

const char* planner_phase_name(PlannerPhase phase);
std::optional<PlannerPhase> parse_planner_phase(std::string_view text);
std::string planner_phase_name_string(PlannerPhase phase);

const char* failure_mismatch_name(FailureMismatchKind mismatchKind);
std::optional<FailureMismatchKind> parse_failure_mismatch(std::string_view text);
std::string failure_mismatch_name_string(FailureMismatchKind mismatchKind);

std::string stable_hash_text(std::string_view text);
std::string hash_engine_state_signature(const EngineStateSignature& sig);
std::string hash_isolate_signature(const IsolatePreparedSignature& sig);
std::string hash_split_result_signature(const SplitResultSignature& sig);
std::string hash_merge_result_signature(const MergeResultSignature& sig);
std::string hash_engine_state(const RawEngine& RE);
std::string hash_planner_trace_prefix(const std::vector<PlannerTraceEntry>& trace, std::size_t prefixLen);
std::string failure_signature_hash(const FailureSignature& sig);

FailureSignature make_failure_signature(FailureClass failureClass, const std::string& detail = {});

void write_failure_signature_machine(std::ostream& os, const FailureSignature& sig);
FailureSignature parse_failure_signature_machine(const std::string& text);
