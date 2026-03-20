#pragma once
#include "harness/dump.hpp"
#include "harness/ogdf_wrapper.hpp"
#include "harness/project_hooks.hpp"

namespace harness {

HarnessResult runStaticPipelineCaseDumpAware(const CompactGraph &H,
                                             IRawSpqrBackend &backend,
                                             IHarnessOps &ops,
                                             uint64_t seed,
                                             int tc,
                                             const std::string &dumpDir);

HarnessResult runDummyGraftCaseDumpAware(const CompactGraph &H,
                                         IRawSpqrBackend &backend,
                                         IHarnessOps &ops,
                                         uint64_t seed,
                                         int tc,
                                         const std::string &dumpDir);

HarnessResult runRewriteRFallbackCaseDumpAware(const ExplicitBlockGraph &G,
                                               IRawSpqrBackend &backend,
                                               IHarnessOps &ops,
                                               uint64_t seed,
                                               int tc,
                                               const std::string &dumpDir);

std::vector<CompactGraph> buildManualCases();
std::vector<ExplicitBlockGraph> buildManualRewriteCases();
CompactGraph makeRandomCompactGraph(uint64_t seed, int caseIndex);

} // namespace harness
