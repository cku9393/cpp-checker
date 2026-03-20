#include "test_harness.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

namespace {

string require_value(int& i, int argc, char** argv, const string& flag) {
    if (i + 1 >= argc) {
        throw runtime_error("missing value for " + flag);
    }
    ++i;
    return argv[i];
}

} // namespace

void print_usage(ostream& os) {
    os << "Usage: raw_engine_tests [options]\n";
    os << "  --case <name>\n";
    os << "  --seed <u32>\n";
    os << "  --iters <int>\n";
    os << "  --repro-file <path>\n";
    os << "  --state-file <path>\n";
    os << "  --primitive isolate|split|join|integrate\n";
    os << "  --step-budget <size_t>\n";
    os << "  --repeat <int>\n";
    os << "  --dump-on-fail\n";
    os << "  --oracle\n";
    os << "  --reduce\n";
    os << "  --verbose\n";
    os << "  --help\n";
}

int main(int argc, char** argv) {
    TestOptions options;
    if (argc > 0 && argv[0] != nullptr) {
        options.executablePath = argv[0];
    }

    try {
        install_failure_handlers();

        for (int i = 1; i < argc; ++i) {
            const string arg = argv[i];
            if (arg == "--case") {
                options.caseName = require_value(i, argc, argv, arg);
            } else if (arg == "--seed") {
                options.seed = static_cast<u32>(stoul(require_value(i, argc, argv, arg)));
            } else if (arg == "--iters") {
                options.iters = stoi(require_value(i, argc, argv, arg));
            } else if (arg == "--repro-file") {
                options.reproFile = require_value(i, argc, argv, arg);
            } else if (arg == "--state-file") {
                options.stateFile = require_value(i, argc, argv, arg);
            } else if (arg == "--primitive") {
                options.primitiveName = require_value(i, argc, argv, arg);
            } else if (arg == "--step-budget") {
                options.stepBudget = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--repeat") {
                options.repeat = stoi(require_value(i, argc, argv, arg));
            } else if (arg == "--dump-on-fail") {
                options.dumpOnFail = true;
            } else if (arg == "--oracle") {
                options.oracle = true;
            } else if (arg == "--reduce") {
                options.reduce = true;
            } else if (arg == "--verbose") {
                options.verbose = true;
            } else if (arg == "--help") {
                print_usage(cout);
                return 0;
            } else {
                throw runtime_error("unknown argument: " + arg);
            }
        }

        if (options.repeat <= 0) {
            throw runtime_error("--repeat must be positive");
        }
        if (options.iters.has_value() && *options.iters <= 0) {
            throw runtime_error("--iters must be positive");
        }

        return run_test_suite(options);
    } catch (const exception& ex) {
        cerr << "raw_engine_tests error: " << ex.what() << '\n';
        print_usage(cerr);
        return 1;
    }
}
