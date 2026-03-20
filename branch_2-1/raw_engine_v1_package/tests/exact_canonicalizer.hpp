#pragma once

#include <cstddef>
#include <string>

#include "exhaustive_generator.hpp"

struct ExactCanonicalKey {
    bool skipped = false;
    std::size_t origCount = 0;
    std::size_t occurrenceCount = 0;
    std::size_t permutationCount = 0;
    std::string key;
};

ExactCanonicalKey compute_exact_state_canonical_key(const RawEngine& RE, std::size_t cap);
ExactCanonicalKey compute_exact_explorer_canonical_key(const ExhaustiveScenario& scenario, std::size_t cap);
ExactCanonicalKey compute_exact_isolate_canonical_key(
    const RawEngine& RE,
    const IsolatePrepared& prep,
    std::size_t cap
);
ExactCanonicalKey compute_exact_split_choice_canonical_key(
    const RawEngine& RE,
    const IsolatePrepared* prep,
    bool stopConditionSatisfied,
    const std::string& failureKey,
    std::size_t cap
);
bool should_sample_exact_canonical(const TestOptions& options, std::size_t ordinal);
