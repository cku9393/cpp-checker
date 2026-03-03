#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include "harness/ogdf_wrapper.hpp"
#include "harness/project_hooks.hpp"
#include "harness/runners.hpp"

using namespace harness;

namespace {
RunConfig parseArgs(int argc, char **argv) {
    RunConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char *name) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + name);
            return argv[++i];
        };
        if (a == "--seed") cfg.seed = std::stoull(next("--seed"));
        else if (a == "--rounds") cfg.rounds = std::stoi(next("--rounds"));
        else if (a == "--manual-only") cfg.manualOnly = true;
        else if (a == "--mode") cfg.mode = next("--mode");
        else if (a == "--dump-dir") cfg.dumpDir = next("--dump-dir");
        else if (a == "--backend") { (void)next("--backend"); }
        else if (a == "--help") {
            std::cout << "--backend ogdf --mode {static|dummy} --seed N --rounds N --manual-only --dump-dir DIR\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown argument: " + a);
        }
    }
    return cfg;
}
}

int main(int argc, char **argv) {
    try {
        RunConfig cfg = parseArgs(argc, argv);
        std::filesystem::create_directories(cfg.dumpDir);

        OgdfRawSpqrBackend backend;
        StubHarnessOps ops;

        int tc = 0;
        auto runOne = [&](const CompactGraph &H) -> bool {
            HarnessResult r;
            if (cfg.mode == "static") {
                r = runStaticPipelineCaseDumpAware(H, backend, ops, cfg.seed, tc, cfg.dumpDir);
            } else if (cfg.mode == "dummy") {
                r = runDummyGraftCaseDumpAware(H, backend, ops, cfg.seed, tc, cfg.dumpDir);
            } else {
                throw std::runtime_error("unknown mode: " + cfg.mode);
            }
            if (!r.ok) {
                std::cerr << "[FAIL] tc=" << tc << " where=" << r.where << " why=" << r.why << "\n";
                std::cerr << "bundle=" << r.dumpPath << "\n";
                return false;
            }
            ++tc;
            return true;
        };

        if (cfg.manualOnly) {
            for (const auto &H : buildManualCases()) {
                if (!runOne(H)) return 1;
            }
        } else {
            for (int i = 0; i < cfg.rounds; ++i) {
                if (!runOne(makeRandomCompactGraph(cfg.seed, i))) return 1;
            }
        }

        std::cout << "[OK] completed tc=" << tc << "\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 2;
    }
}
