#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace {

namespace fs = std::filesystem;

struct Options {
    fs::path cache_bin;
    fs::path output_dir = "lfu_benchmark_results";
    int requests = 100000;
    int keys = 10000;
    std::vector<int> capacities{100, 500, 1000};
    std::vector<double> alphas{0.8, 1.0, 1.2};
    std::uint64_t seed = 42;
    int runs = 1;
    bool self_test = false;
};

template <typename T>
T parseNumber(std::string_view value) {
    T result{};
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument("Invalid number: " + std::string(value));
    }
    return result;
}

template <typename T>
std::vector<T> parseList(const std::string& value) {
    std::vector<T> result;
    std::istringstream input(value);
    std::string item;
    while (std::getline(input, item, ',')) {
        result.push_back(parseNumber<T>(item));
    }
    if (result.empty() || value.back() == ',') {
        throw std::invalid_argument("Expected a nonempty comma-separated list");
    }
    return result;
}

Options parseOptions(int argc, char* argv[]) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];
        if (option == "--self-test") {
            options.self_test = true;
            continue;
        }
        if (index + 1 == argc) {
            throw std::invalid_argument("Missing value for " + option);
        }
        const std::string value = argv[++index];
        if (option == "--cache-bin")       options.cache_bin = value;
        else if (option == "--output")     options.output_dir = value;
        else if (option == "--requests")   options.requests = parseNumber<int>(value);
        else if (option == "--keys")       options.keys = parseNumber<int>(value);
        else if (option == "--capacities") options.capacities = parseList<int>(value);
        else if (option == "--alphas")     options.alphas = parseList<double>(value);
        else if (option == "--seed")       options.seed = parseNumber<std::uint64_t>(value);
        else if (option == "--runs")       options.runs = parseNumber<int>(value);

        else throw std::invalid_argument("Unknown option: " + option);
    }
    if (options.cache_bin.empty()) throw std::invalid_argument("--cache-bin is required");
    if (options.requests < 0 || options.keys <= 0 || options.runs <= 0) {
        throw std::invalid_argument("Require requests >= 0, keys > 0, runs > 0");
    }
    for (int capacity : options.capacities) {
        if (capacity < 0) throw std::invalid_argument("Capacity must be >= 0");
    }
    for (double alpha : options.alphas) {
        if (!std::isfinite(alpha) || alpha <= 0) {
            throw std::invalid_argument("Zipf alpha must be finite and > 0");
        }
    }
    if (static_cast<std::uint64_t>(options.runs - 1) >
        std::numeric_limits<std::uint64_t>::max() - options.seed) {
        throw std::invalid_argument("seed + runs - 1 overflows");
    }
    options.cache_bin = fs::absolute(options.cache_bin);
    if (!fs::is_regular_file(options.cache_bin) || access(options.cache_bin.c_str(), X_OK) != 0) {
        throw std::invalid_argument("Cache executable not found or not executable: " +
                                    options.cache_bin.string());
    }
    return options;
}

// An independent LFU implementation: ordered (frequency, last access, key).
// Unlike the cache under test, this model stores no list iterators.
std::size_t countReferenceHits(const std::vector<int>& requests, int capacity) {
    using Priority = std::tuple<std::size_t, std::size_t, int>;
    std::set<Priority> priorities;
    std::unordered_map<int, Priority> entries;
    std::size_t hits = 0;
    std::size_t tick = 0;
    for (int key : requests) {
        ++tick;
        if (capacity == 0) continue;
        auto entry_it = entries.find(key);
        std::size_t frequency = 1;
        if (entry_it != entries.end()) {
            ++hits;
            frequency = std::get<0>(entry_it->second) + 1;
            priorities.erase(entry_it->second);
        } else if (entries.size() == static_cast<std::size_t>(capacity)) {
            entries.erase(std::get<2>(*priorities.begin()));
            priorities.erase(priorities.begin());
        }
        Priority priority{frequency, tick, key};
        entries.insert_or_assign(key, priority);
        priorities.insert(priority);
    }
    return hits;
}

std::vector<int> generateZipf(const Options& options, double alpha, std::uint64_t seed) {
    std::vector<double> weights(static_cast<std::size_t>(options.keys));
    for (std::size_t index = 0; index < weights.size(); ++index) {
        weights[index] = std::pow(static_cast<double>(index + 1), -alpha);
    }
    std::mt19937_64 engine(seed);
    std::discrete_distribution<int> distribution(weights.begin(), weights.end());
    std::vector<int> requests;
    requests.reserve(static_cast<std::size_t>(options.requests));
    for (int index = 0; index < options.requests; ++index) {
        requests.push_back(distribution(engine) + 1);
    }
    return requests;
}

// File redirection avoids shell quoting and pipe-buffer deadlocks on long traces.
void runCache(const fs::path& binary, const fs::path& input_path,
              const fs::path& output_path, const fs::path& error_path) {
    posix_spawn_file_actions_t actions;
    int error = posix_spawn_file_actions_init(&actions);
    if (error != 0) throw std::runtime_error(std::strerror(error));
    error = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO,
                                           input_path.c_str(), O_RDONLY, 0);
    if (error == 0) error = posix_spawn_file_actions_addopen(
        &actions, STDOUT_FILENO, output_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (error == 0) error = posix_spawn_file_actions_addopen(
        &actions, STDERR_FILENO, error_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    std::string executable = binary.string();
    char* arguments[] = {executable.data(), nullptr};
    pid_t child = -1;
    if (error == 0) error = posix_spawn(&child, executable.c_str(), &actions,
                                      nullptr, arguments, environ);
    posix_spawn_file_actions_destroy(&actions);
    if (error != 0) throw std::runtime_error("Cannot launch cache: " + std::string(std::strerror(error)));

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    int status = 0;
    for (;;) {
        const auto result = waitpid(child, &status, WNOHANG);
        if (result == child) break;
        if (result == -1) {
            if (errno == EINTR) continue;
            throw std::runtime_error("waitpid failed: " + std::string(std::strerror(errno)));
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            kill(child, SIGKILL);
            while (waitpid(child, &status, 0) == -1 && errno == EINTR) {}
            throw std::runtime_error("Cache exceeded the 30 second timeout");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw std::runtime_error("Cache failed; see " + error_path.string());
    }
}

fs::path createRunDirectory(const fs::path& root) {
    fs::create_directories(root);
    std::string pattern = (fs::absolute(root) / "run-XXXXXX").string();
    if (mkdtemp(pattern.data()) == nullptr) {
        throw std::runtime_error("Cannot create results directory: " + std::string(std::strerror(errno)));
    }
    return pattern;
}

bool runCase(const Options& options, const fs::path& run_dir, const std::string& case_name,
             const std::vector<int>& requests, int capacity, std::size_t expected_hits,
             std::ofstream& report, const std::string& metadata) {
    const fs::path input_path = run_dir / (case_name + ".in");
    const fs::path output_path = run_dir / (case_name + ".out");
    const fs::path error_path = run_dir / (case_name + ".err");
    {
        std::ofstream input(input_path);
        input.exceptions(std::ios::badbit | std::ios::failbit);
        input << requests.size() << ' ' << capacity << '\n';
        for (int key : requests) input << key << '\n';
        input.close();
    }
    try {
        runCache(options.cache_bin, input_path, output_path, error_path);
        std::ifstream output(output_path);
        long long actual_hits = -1;
        std::string extra;
        if (!(output >> actual_hits) || actual_hits < 0 ||
            static_cast<unsigned long long>(actual_hits) > requests.size() || (output >> extra)) {
            throw std::runtime_error("Expected one integer hit count in " + output_path.string());
        }
        const bool passed = static_cast<std::size_t>(actual_hits) == expected_hits;
        const double hit_rate = requests.empty() ? 0.0 :
            static_cast<double>(actual_hits) / static_cast<double>(requests.size());
        report << case_name << ',' << metadata << ',' << requests.size() << ',' << capacity
               << ',' << actual_hits << ',' << (requests.size() - static_cast<std::size_t>(actual_hits))
               << ',' << hit_rate << ',' << expected_hits << ',' << (passed ? "PASS" : "FAIL") << '\n';
        std::cout << case_name << ": hits=" << actual_hits << '/' << requests.size()
                  << " hit_rate=" << std::fixed << std::setprecision(2) << 100.0 * hit_rate
                  << "% expected=" << expected_hits << ' ' << (passed ? "PASS" : "FAIL") << '\n';
        return passed;
    } catch (const std::exception& error) {
        report << case_name << ',' << metadata << ',' << requests.size() << ',' << capacity
               << ",,,," << expected_hits << ",ERROR\n";
        std::cerr << case_name << ": " << error.what() << '\n';
        return false;
    }
}

bool runSelfTests(const Options& options, const fs::path& run_dir, std::ofstream& report) {
    struct TestCase {
        std::string name;
        std::vector<int> requests;
        int capacity;
        std::size_t hits;
    };
    const std::vector<TestCase> cases{
        {"empty", {}, 2, 0},
        {"zero_capacity", {1, 1, 2, 2}, 0, 0},
        {"one_key", {7, 7, 7, 7}, 1, 3},
        {"all_unique", {1, 2, 3, 4, 5}, 2, 0},
        {"all_fit", {1, 2, 3, 3, 2, 1}, 3, 3},
        {"lfu_priority", {1, 1, 2, 3, 1, 2}, 2, 2},
        {"lru_tie", {1, 2, 2, 1, 3, 1, 2}, 2, 3},
        {"integer_limits", {std::numeric_limits<int>::min(), std::numeric_limits<int>::max(),
                            std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}, 2, 2}
    };
    bool passed = true;
    for (const auto& test : cases) {
        if (countReferenceHits(test.requests, test.capacity) != test.hits) {
            throw std::runtime_error("Reference model failed known case: " + test.name);
        }
        passed = runCase(options, run_dir, test.name, test.requests, test.capacity,
                         test.hits, report, "fixed,,,") && passed;
    }
    return passed;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        std::cout << "lfu_benchmark --cache-bin PATH [options]\n"
                  << "  --requests N       Requests per stream (default 100000, may be 0)\n"
                  << "  --keys N           Integer keys 1..N (default 10000)\n"
                  << "  --capacities LIST  Cache sizes (default 100,500,1000)\n"
                  << "  --alphas LIST      Positive Zipf exponents (default 0.8,1.0,1.2)\n"
                  << "  --seed N           RNG seed (default 42)\n"
                  << "  --runs N           Streams per alpha; seeds seed..seed+N-1 (default 1)\n"
                  << "  --output DIR       Parent for fresh run directories (default lfu_benchmark_results)\n"
                  << "  --self-test        Run fixed edge cases against the executable\n";
        return 0;
    }
    try {
        const Options options = parseOptions(argc, argv);
        const fs::path run_dir = createRunDirectory(options.output_dir);
        std::cout << "Results: " << run_dir << '\n';
        std::ofstream report(run_dir / "results.csv");
        report.exceptions(std::ios::badbit | std::ios::failbit);
        report << std::setprecision(17)
               << "case,distribution,keys,alpha,seed,requests,capacity,hits,misses,hit_rate,reference_hits,status\n";
        bool passed = true;
        if (options.self_test) {
            passed = runSelfTests(options, run_dir, report);
        } else {
            for (std::size_t alpha_index = 0; alpha_index < options.alphas.size(); ++alpha_index) {
                const double alpha = options.alphas[alpha_index];
                for (int run = 0; run < options.runs; ++run) {
                    const auto seed = options.seed + static_cast<std::uint64_t>(run);
                    const auto requests = generateZipf(options, alpha, seed);
                    std::ostringstream metadata;
                    metadata << std::setprecision(17) << "zipf," << options.keys << ',' << alpha << ',' << seed;
                    for (std::size_t index = 0; index < options.capacities.size(); ++index) {
                        const int capacity = options.capacities[index];
                        const std::string name = "zipf_a" + std::to_string(alpha_index) +
                            "_run" + std::to_string(run) + "_c" + std::to_string(index);
                        const auto expected_hits = countReferenceHits(requests, capacity);
                        passed = runCase(options, run_dir, name, requests, capacity,
                                         expected_hits, report, metadata.str()) && passed;
                    }
                }
            }
        }
        report.close();
        return passed ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "lfu_benchmark: " << error.what() << '\n';
        return 1;
    }
}
