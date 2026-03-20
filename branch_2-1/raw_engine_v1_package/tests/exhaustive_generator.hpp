#pragma once

#include <unordered_map>

#include "test_harness.hpp"

struct SemanticOccurrenceSignature {
    Vertex orig = NIL_U32;
    std::vector<Vertex> allocNbrOrig;
    std::vector<std::string> ports;
    std::vector<std::string> corePatchEdges;

    bool operator==(const SemanticOccurrenceSignature& rhs) const;
    bool operator<(const SemanticOccurrenceSignature& rhs) const;
};

struct SemanticSkeletonSignature {
    std::vector<std::string> vertices;
    std::vector<std::string> edges;
    std::vector<std::string> hostedOccurrences;

    bool operator==(const SemanticSkeletonSignature& rhs) const;
    bool operator<(const SemanticSkeletonSignature& rhs) const;
};

struct SemanticStateCanonicalSignature {
    std::vector<SemanticSkeletonSignature> skeletons;
    std::vector<SemanticOccurrenceSignature> occurrences;

    bool operator==(const SemanticStateCanonicalSignature& rhs) const;
};

struct PrimitiveCanonicalSignature {
    PrimitiveKind primitive = PrimitiveKind::ISOLATE;
    SemanticStateCanonicalSignature finalState;

    bool operator==(const PrimitiveCanonicalSignature& rhs) const;
};

struct PlannerFinalStateCanonicalSignature {
    SemanticStateCanonicalSignature finalState;
    bool stopConditionSatisfied = false;

    bool operator==(const PlannerFinalStateCanonicalSignature& rhs) const;
};

struct ExplorerStateCanonicalSignature {
    ExhaustiveFamily family = ExhaustiveFamily::ALL;
    SemanticStateCanonicalSignature state;
    std::string targetOccurrence;
    std::vector<std::string> keepOccurrences;
    std::vector<std::string> initialQueue;

    bool operator==(const ExplorerStateCanonicalSignature& rhs) const;
};

struct PrimitiveExecutionPlan {
    PrimitiveKind primitive = PrimitiveKind::ISOLATE;
    RawSkelID sid = NIL_U32;
    RawSkelID leftSid = NIL_U32;
    RawSkelID rightSid = NIL_U32;
    Vertex aOrig = NIL_U32;
    Vertex bOrig = NIL_U32;
    RawSkelID parentSid = NIL_U32;
    RawSkelID childSid = NIL_U32;
    std::vector<BoundaryMapEntry> boundaryMap;
};

struct ExhaustiveScenario {
    RawEngine RE;
    RawPlannerCtx ctx;
    RawUpdateCtx U;
    std::vector<UpdJob> initialQueue;
    PrimitiveExecutionPlan primitivePlan;
    ExhaustiveFamily family = ExhaustiveFamily::ALL;
    std::string label;
};

struct ExhaustiveFamilyStats {
    std::size_t generated = 0;
    std::size_t unique = 0;
    std::size_t validatorChecks = 0;
    std::size_t primitiveChecks = 0;
    std::size_t plannerChecks = 0;
    std::size_t dedupeDrops = 0;
    std::size_t splitChoiceCompareStateCount = 0;
    std::size_t compareEligibleStateCount = 0;
    std::size_t compareIneligibleStateCount = 0;
    std::size_t compareCompletedStateCount = 0;
    std::size_t comparePartialStateCount = 0;
    std::size_t splitChoiceExactFullEvalCount = 0;
    std::size_t splitChoiceExactShadowEvalCount = 0;
    std::size_t splitChoiceFallbackCount = 0;
    std::size_t splitChoiceSameRepresentativeCount = 0;
    std::size_t splitChoiceSameSemanticClassCount = 0;
    std::size_t splitChoiceSameFinalStateCount = 0;
    std::size_t splitChoiceSemanticDisagreementCount = 0;
    std::size_t splitChoiceCapHitCount = 0;
    std::size_t splitChoiceQueueNormalizedCount = 0;
    std::size_t representativeShiftCount = 0;
    std::size_t harmlessShiftCount = 0;
    std::size_t traceOnlyShiftCount = 0;
    std::size_t semanticShiftCount = 0;
    std::size_t multiclassClusterCount = 0;
    std::size_t harmlessMulticlassCount = 0;
    std::size_t traceOnlyMulticlassCount = 0;
    std::size_t semanticShiftMulticlassCount = 0;
    std::unordered_map<std::string, std::size_t> compareIneligibleReasonHistogram;
};

struct OrganicDuplicateExample {
    ExhaustiveFamily family = ExhaustiveFamily::ALL;
    std::string hash;
    std::string firstLabel;
    std::string duplicateLabel;
    std::string cause;
};

struct ExhaustiveRunStats {
    std::size_t generatedStateCount = 0;
    std::size_t canonicalUniqueStateCount = 0;
    std::size_t dedupeDropCount = 0;
    std::size_t collisionSpotCheckCount = 0;
    std::size_t organicDuplicateExampleCount = 0;
    std::size_t exactAuditedStateCount = 0;
    std::size_t exactAuditSkippedCapCount = 0;
    std::size_t exactAuditSkippedBudgetCount = 0;
    std::size_t exactAuditSkippedSampleCount = 0;
    std::size_t fastUniqueCount = 0;
    std::size_t exactUniqueCount = 0;
    std::size_t fastVsExactDisagreementCount = 0;
    std::size_t falseMergeCount = 0;
    std::size_t falseSplitCount = 0;
    std::size_t splitChoiceCompareStateCount = 0;
    std::size_t compareEligibleStateCount = 0;
    std::size_t compareIneligibleStateCount = 0;
    std::size_t compareCompletedStateCount = 0;
    std::size_t comparePartialStateCount = 0;
    std::size_t splitChoiceExactFullEvalCount = 0;
    std::size_t splitChoiceExactShadowEvalCount = 0;
    std::size_t splitChoiceFallbackCount = 0;
    std::size_t splitChoiceSameRepresentativeCount = 0;
    std::size_t splitChoiceSameSemanticClassCount = 0;
    std::size_t splitChoiceSameFinalStateCount = 0;
    std::size_t splitChoiceSemanticDisagreementCount = 0;
    std::size_t splitChoiceCapHitCount = 0;
    std::size_t splitChoiceHarmlessCompareCount = 0;
    std::size_t splitChoiceTraceOnlyCompareCount = 0;
    std::size_t multiclassClusterCount = 0;
    std::size_t harmlessMulticlassCount = 0;
    std::size_t traceOnlyMulticlassCount = 0;
    std::size_t semanticShiftMulticlassCount = 0;
    std::size_t buildOrderDuplicateCount = 0;
    std::size_t commitOrderDuplicateCount = 0;
    std::size_t hostedOccOrderDuplicateCount = 0;
    std::size_t relabelDuplicateCount = 0;
    std::size_t occidDuplicateCount = 0;
    std::size_t symmetricStructureDuplicateCount = 0;
    std::size_t mixedDuplicateCount = 0;
    std::size_t unknownDuplicateCount = 0;
    std::unordered_map<ExhaustiveFamily, ExhaustiveFamilyStats> familyStats;
    std::unordered_map<std::string, std::size_t> compareIneligibleReasonHistogram;
    std::unordered_map<std::string, std::size_t> multiclassCatalogHistogram;
    std::vector<OrganicDuplicateExample> organicDuplicateExamples;
    std::vector<std::string> multiclassCatalogExamples;
};

struct RebuildTransform {
    std::unordered_map<Vertex, Vertex> relabelOrig;
    bool reverseSkeletonOrder = false;
    bool reverseEdgeOrder = false;
    bool reverseVertexOrder = false;
    bool reverseHostedOccOrder = false;
    bool reverseOccAllocationOrder = false;
};

const char* exhaustive_family_name(ExhaustiveFamily family);

SemanticStateCanonicalSignature capture_semantic_state_signature(const RawEngine& RE);
PrimitiveCanonicalSignature capture_primitive_canonical_signature(PrimitiveKind primitive, const RawEngine& RE);
PlannerFinalStateCanonicalSignature capture_planner_final_state_canonical_signature(const RawEngine& RE, OccID targetOcc);
ExplorerStateCanonicalSignature capture_explorer_state_signature(const ExhaustiveScenario& scenario);

std::string describe_semantic_state_signature(const SemanticStateCanonicalSignature& sig);
std::string describe_explorer_state_signature(const ExplorerStateCanonicalSignature& sig);
std::string hash_semantic_state_signature(const SemanticStateCanonicalSignature& sig);
std::string hash_explorer_state_signature(const ExplorerStateCanonicalSignature& sig);

std::vector<ExhaustiveScenario> generate_exhaustive_scenarios(const TestOptions& options, ExhaustiveFamily family);
ExhaustiveScenario make_targeted_planner_exhaustive_scenario(ScenarioFamily family, u32 seed);

ExhaustiveScenario rebuild_exhaustive_scenario(const ExhaustiveScenario& scenario, const RebuildTransform& transform);
std::unordered_map<Vertex, Vertex> invert_relabel_map(const std::unordered_map<Vertex, Vertex>& relabelOrig);
RebuildTransform make_relabel_transform(const ExhaustiveScenario& scenario);
RebuildTransform make_occid_renumber_transform();
RebuildTransform make_edge_order_transform();
RebuildTransform make_vertex_order_transform();
RebuildTransform make_hosted_occ_order_transform();
