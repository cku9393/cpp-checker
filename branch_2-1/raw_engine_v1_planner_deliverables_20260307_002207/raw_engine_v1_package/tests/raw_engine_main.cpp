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

bool next_is_value(int i, int argc, char** argv) {
    return i + 1 < argc && argv[i + 1] != nullptr && string(argv[i + 1]).rfind("--", 0) != 0;
}

void append_keep_occ(vector<OccID>& keepOcc, const string& csv) {
    size_t start = 0;
    while (start < csv.size()) {
        const size_t comma = csv.find(',', start);
        const string token = csv.substr(start, comma == string::npos ? string::npos : comma - start);
        if (!token.empty()) {
            keepOcc.push_back(static_cast<OccID>(stoul(token)));
        }
        if (comma == string::npos) {
            break;
        }
        start = comma + 1;
    }
}

optional<OracleMode> parse_oracle_mode(const string& text) {
    if (text == "primitive") {
        return OracleMode::PRIMITIVE;
    }
    if (text == "planner") {
        return OracleMode::PLANNER;
    }
    if (text == "all") {
        return OracleMode::ALL;
    }
    return nullopt;
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
    os << "  --target-occ <id>\n";
    os << "  --keep-occ <csv>\n";
    os << "  --artifact-dir <path>\n";
    os << "  --step-budget <size_t>\n";
    os << "  --repeat <int>\n";
    os << "  --dump-on-fail\n";
    os << "  --oracle [primitive|planner|all]\n";
    os << "  --dump-trace\n";
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
            } else if (arg == "--target-occ") {
                options.targetOcc = static_cast<OccID>(stoul(require_value(i, argc, argv, arg)));
            } else if (arg == "--keep-occ") {
                append_keep_occ(options.keepOcc, require_value(i, argc, argv, arg));
            } else if (arg == "--artifact-dir") {
                options.artifactDir = require_value(i, argc, argv, arg);
            } else if (arg == "--step-budget") {
                options.stepBudget = static_cast<size_t>(stoull(require_value(i, argc, argv, arg)));
            } else if (arg == "--repeat") {
                options.repeat = stoi(require_value(i, argc, argv, arg));
            } else if (arg == "--dump-on-fail") {
                options.dumpOnFail = true;
            } else if (arg == "--oracle") {
                if (next_is_value(i, argc, argv)) {
                    const string modeText = require_value(i, argc, argv, arg);
                    const optional<OracleMode> mode = parse_oracle_mode(modeText);
                    if (!mode.has_value()) {
                        throw runtime_error("unknown oracle mode: " + modeText);
                    }
                    options.oracleMode = *mode;
                } else {
                    options.oracleMode = OracleMode::PRIMITIVE;
                }
            } else if (arg == "--dump-trace") {
                options.dumpTrace = true;
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
