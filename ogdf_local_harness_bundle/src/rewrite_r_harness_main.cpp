#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#if __has_include("ogdf_feature_config.hpp")
#include "ogdf_feature_config.hpp"
#endif

struct Config {
    std::string backend = "ogdf";
    std::string mode = "static"; // static | dummy
    unsigned long long seed = 1;
    int rounds = 1;
    bool manualOnly = false;
    std::string dumpDir = "dumps";
    bool dryRun = true;
};

static void printUsage(const char *argv0) {
    std::cout
        << "Usage: " << argv0 << " [options]\n"
        << "  --backend <ogdf|ported>\n"
        << "  --mode <static|dummy>\n"
        << "  --seed <uint64>\n"
        << "  --rounds <int>\n"
        << "  --manual-only\n"
        << "  --dump-dir <path>\n"
        << "  --dry-run <0|1>\n"
        << "  --help\n";
}

static Config parseArgs(int argc, char **argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto needValue = [&](const char *name) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + name);
            return argv[++i];
        };

        if (a == "--backend") cfg.backend = needValue("--backend");
        else if (a == "--mode") cfg.mode = needValue("--mode");
        else if (a == "--seed") cfg.seed = std::stoull(needValue("--seed"));
        else if (a == "--rounds") cfg.rounds = std::stoi(needValue("--rounds"));
        else if (a == "--manual-only") cfg.manualOnly = true;
        else if (a == "--dump-dir") cfg.dumpDir = needValue("--dump-dir");
        else if (a == "--dry-run") cfg.dryRun = (needValue("--dry-run") != "0");
        else if (a == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        } else {
            throw std::runtime_error("unknown arg: " + a);
        }
    }

    if (cfg.mode != "static" && cfg.mode != "dummy") {
        throw std::runtime_error("--mode must be static or dummy");
    }
    if (cfg.backend != "ogdf" && cfg.backend != "ported") {
        throw std::runtime_error("--backend must be ogdf or ported");
    }
    if (cfg.rounds <= 0) {
        throw std::runtime_error("--rounds must be positive");
    }
    return cfg;
}

static void printFeatureReport() {
#if defined(USE_OGDF)
    std::cout << "USE_OGDF=1\n";
#else
    std::cout << "USE_OGDF=0\n";
#endif

#ifdef OGDF_HAS_RANGE_GRAPH_ITER
    std::cout << "OGDF_HAS_RANGE_GRAPH_ITER=" << OGDF_HAS_RANGE_GRAPH_ITER << "\n";
#endif
#ifdef OGDF_HAS_NODE_ADJENTRIES
    std::cout << "OGDF_HAS_NODE_ADJENTRIES=" << OGDF_HAS_NODE_ADJENTRIES << "\n";
#endif
#ifdef OGDF_HAS_STATICSKELETON_HEADER
    std::cout << "OGDF_HAS_STATICSKELETON_HEADER=" << OGDF_HAS_STATICSKELETON_HEADER << "\n";
#endif
#ifdef OGDF_CAN_BUILD_BASIC_STATICSPQR
    std::cout << "OGDF_CAN_BUILD_BASIC_STATICSPQR=" << OGDF_CAN_BUILD_BASIC_STATICSPQR << "\n";
#endif
}

int main(int argc, char **argv) {
    try {
        Config cfg = parseArgs(argc, argv);
        std::filesystem::create_directories(cfg.dumpDir);

        std::cout << "[rewrite_r_harness]\n";
        std::cout << "backend=" << cfg.backend << "\n";
        std::cout << "mode=" << cfg.mode << "\n";
        std::cout << "seed=" << cfg.seed << "\n";
        std::cout << "rounds=" << cfg.rounds << "\n";
        std::cout << "manualOnly=" << (cfg.manualOnly ? 1 : 0) << "\n";
        std::cout << "dumpDir=" << cfg.dumpDir << "\n";
        std::cout << "dryRun=" << (cfg.dryRun ? 1 : 0) << "\n";
        printFeatureReport();

        if (cfg.dryRun) {
            std::cout << "\n[dry-run]\n";
            std::cout << "Implement and connect these calls in your harness library:\n";
            if (cfg.mode == "static") {
                std::cout << "  runStaticPipelineCaseDumpAware(H, rawBackend, seed, tc, dumpDir)\n";
            } else {
                std::cout << "  runDummyGraftCaseDumpAware(H, rawBackend, seed, tc, dumpDir)\n";
            }
            std::cout << "Then rerun with --dry-run 0 after integration.\n";
            return 0;
        }

        std::cout << "\n[warning] This template main() does not call your project harness directly yet.\n";
        std::cout << "Connect the runner in src/rewrite_r_harness_main.cpp or replace this file with your integrated driver.\n";
        return 2;
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        printUsage(argv[0]);
        return 1;
    }
}
