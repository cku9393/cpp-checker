#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "exhaustive_generator.hpp"
#include "split_choice_oracle.hpp"

enum class MulticlassCatalogCategory : u8 {
    NONE = 0,
    HARMLESS = 1,
    TRACE_ONLY = 2,
    SEMANTIC_SHIFT = 3,
};

struct MulticlassCatalogObservation {
    std::string clusterKey;
    MulticlassCatalogCategory category = MulticlassCatalogCategory::NONE;
    std::size_t admissiblePairCount = 0;
    std::size_t semanticClassCount = 0;
    bool representativeShifted = false;
    std::pair<Vertex, Vertex> fastSelectedPair{NIL_U32, NIL_U32};
    std::pair<Vertex, Vertex> exactShadowSelectedPair{NIL_U32, NIL_U32};
    std::pair<Vertex, Vertex> exactFullSelectedPair{NIL_U32, NIL_U32};
    bool stopConditionDivergence = false;
    bool finalStateDivergence = false;
};

struct MulticlassCatalogStats {
    std::size_t clusterCount = 0;
    std::size_t harmlessClusterCount = 0;
    std::size_t traceOnlyClusterCount = 0;
    std::size_t semanticShiftClusterCount = 0;
    std::unordered_map<std::string, std::size_t> familyCategoryHistogram;
};

struct MulticlassCatalogEntry {
    std::string clusterKey;
    std::string familyName;
    std::size_t admissiblePairCount = 0;
    std::size_t semanticClassCount = 0;
    bool representativeShifted = false;
    std::pair<Vertex, Vertex> fastSelectedPair{NIL_U32, NIL_U32};
    std::pair<Vertex, Vertex> exactShadowSelectedPair{NIL_U32, NIL_U32};
    std::pair<Vertex, Vertex> exactFullSelectedPair{NIL_U32, NIL_U32};
    bool stopConditionDivergence = false;
    bool finalStateDivergence = false;
    MulticlassCatalogCategory category = MulticlassCatalogCategory::NONE;
};

const char* multiclass_catalog_category_name(MulticlassCatalogCategory category);
void reset_multiclass_catalog();
const MulticlassCatalogStats& multiclass_catalog_stats();
MulticlassCatalogObservation observe_multiclass_catalog(const SplitChoiceOracleRunResult& result);
void note_multiclass_catalog_observation(
    const TestOptions& options,
    const std::string& caseName,
    ScenarioFamily family,
    const ExhaustiveScenario& scenario,
    const SplitChoiceOracleRunResult& result
);
std::optional<MulticlassCatalogEntry> build_multiclass_catalog_entry(
    const std::string& familyName,
    const ExhaustiveScenario& scenario,
    const SplitChoiceOracleRunResult& result
);
void write_multiclass_catalog_entry(
    const TestOptions& options,
    const std::string& caseName,
    const ExhaustiveScenario& scenario,
    const MulticlassCatalogEntry& entry,
    const SplitChoiceOracleRunResult& result
);
