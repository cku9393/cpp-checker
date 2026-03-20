#include <iostream>
#include <string>
#include "harness/project_hooks.hpp"
#include "harness/project_static_adapter.hpp"

namespace solver_compare_skeleton {

struct CompareResult {
    bool ok = false;
    std::string why;
    harness::RewriteSeqStats seqStats;
    harness::ExplicitBlockGraph legacyExplicit;
    harness::ExplicitBlockGraph engineExplicit;
};

CompareResult runCompare(harness::ReducedSPQRCore initialCore) {
    CompareResult out;
    harness::ProjectHarnessOps ops;

    harness::ReducedSPQRCore engineCore = initialCore;
    std::string why;
    if (!harness::runRewriteSequenceToFixpoint(engineCore, out.seqStats, why)) {
        out.why = why;
        return out;
    }

    out.legacyExplicit = ops.materializeWholeCoreExplicit(initialCore);
    out.engineExplicit = ops.materializeWholeCoreExplicit(engineCore);

    if (!ops.checkEquivalentExplicitGraphs(out.engineExplicit, out.legacyExplicit, why)) {
        out.why = why;
        return out;
    }

    out.ok = true;
    return out;
}

} // namespace solver_compare_skeleton
