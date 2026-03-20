#pragma once
#include "harness/dump.hpp"
#include "harness/ogdf_wrapper.hpp"
#include "harness/project_hooks.hpp"

namespace harness {

struct RewriteSeqRegressionCase {
    std::string name;
    uint64_t seed = 1;
    int tcIndex = -1;
    int targetStep = -1;
    bool expectedTopLevelOk = true;
    bool expectedActualInvariantOk = true;
    bool expectedOracleEquivalentOk = true;
    std::string expectedPostcheckSubtype;
    std::string expectedSpecialPathTag;
};

struct RewriteSeqRegressionCaseResult {
    std::string name;
    bool ok = false;
    double elapsedMs = 0.0;
    std::string why;
    std::string dumpPath;
};

struct RewriteSeqRegressionSummary {
    int totalCases = 0;
    int passedCases = 0;
    int failedCases = 0;
    std::vector<std::string> failedCaseNames;
    std::vector<RewriteSeqRegressionCaseResult> caseResults;
    std::string manifestPath;
    std::string dumpDir;
};

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

HarnessResult runRewriteRSequenceCaseDumpAware(const ExplicitBlockGraph &G,
                                               IRawSpqrBackend &backend,
                                               IHarnessOps &ops,
                                               uint64_t seed,
                                               int tc,
                                               const std::string &dumpDir);

HarnessResult runRewriteRSequenceReplayDumpAware(const ExplicitBlockGraph &G,
                                                 IRawSpqrBackend &backend,
                                                 IHarnessOps &ops,
                                                 uint64_t seed,
                                                 int tc,
                                                 int targetStep,
                                                 const std::string &dumpDir,
                                                 HarnessBundle *capturedBundle = nullptr);

bool loadRewriteSeqRegressionCases(const std::string &manifestPath,
                                   std::vector<RewriteSeqRegressionCase> &cases,
                                   std::string &why);

bool runRewriteRSequenceRegressionManifestDumpAware(
    const std::vector<RewriteSeqRegressionCase> &cases,
    IRawSpqrBackend &backend,
    IHarnessOps &ops,
    const std::string &dumpDir,
    RewriteSeqRegressionSummary &summary,
    std::string &why);

std::vector<CompactGraph> buildManualCases();
std::vector<ExplicitBlockGraph> buildManualRewriteCases();
CompactGraph makeRandomCompactGraph(uint64_t seed, int caseIndex);
ExplicitBlockGraph makeRandomRewriteCase(uint64_t seed, int caseIndex);

} // namespace harness
