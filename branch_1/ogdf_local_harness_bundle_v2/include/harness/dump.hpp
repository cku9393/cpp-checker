#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include "harness/types.hpp"

namespace harness {

void dumpCompactGraph(const CompactGraph &H, std::ostream &os);
void dumpRawSpqrDecomp(const RawSpqrDecomp &raw, std::ostream &os);
void dumpStaticMiniCore(const StaticMiniCore &mini, std::ostream &os);
void dumpActualCore(const ReducedSPQRCore &C, std::ostream &os);
void dumpGraftTrace(const GraftTrace &T, std::ostream &os);
void dumpExplicitBlockGraph(const ExplicitBlockGraph &G, std::ostream &os);
void dumpHarnessBundle(const HarnessBundle &B, const std::string &path);

inline HarnessBundle makeBaseBundle(uint64_t seed,
                                    int tc,
                                    const char *backendName,
                                    const CompactGraph &H) {
    HarnessBundle B;
    B.seed = seed;
    B.tc = tc;
    B.backendName = backendName ? backendName : "UNKNOWN";
    B.compact = H;
    return B;
}

inline HarnessBundle makeBaseBundle(uint64_t seed,
                                    int tc,
                                    const char *backendName,
                                    const ExplicitBlockGraph &G) {
    HarnessBundle B;
    B.seed = seed;
    B.tc = tc;
    B.backendName = backendName ? backendName : "UNKNOWN";
    B.explicitInput = G;
    return B;
}

inline void setFailure(HarnessBundle &B,
                       HarnessStage stage,
                       std::string where,
                       std::string why) {
    B.stage = stage;
    B.where = std::move(where);
    B.why = std::move(why);
}

inline std::string makeBundlePath(const std::string &dir,
                                  const HarnessBundle &B) {
    std::filesystem::create_directories(dir);
    std::ostringstream oss;
    oss << dir << "/" << stageName(B.stage)
        << "_seed" << B.seed
        << "_tc" << B.tc;
    if (B.stepIndex.has_value()) oss << "_step" << *B.stepIndex;
    oss << ".txt";
    return oss.str();
}

inline HarnessResult failAndDump(HarnessBundle &B,
                                 const std::string &dumpDir) {
    HarnessResult R;
    R.ok = false;
    R.where = B.where;
    R.why = B.why;
    R.dumpPath = makeBundlePath(dumpDir, B);
    R.bundle = B;
    dumpHarnessBundle(B, R.dumpPath);
    return R;
}

} // namespace harness
