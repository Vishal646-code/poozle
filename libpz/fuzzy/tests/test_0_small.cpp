#include "ukkonen.hpp"
#include "pz_cxx_std.hpp"

#include <chrono>

struct TestCase {
    std::string pattern;
    std::string text;
    unsigned int k;
    int expected_count;
    std::vector<std::pair<int, int>> expected_locations;
    std::string description;
};

struct TestResult {
    bool passed;
    long long elapsed_microseconds;
};

int exact_edit_distance(const std::string& a, const std::string& b) {
    int m = static_cast<int>(a.size());
    int n = static_cast<int>(b.size());

    std::vector<int> prev(n + 1);
    std::vector<int> curr(n + 1);

    for (int j = 0; j <= n; j++) {
        prev[j] = j;
    }

    for (int i = 1; i <= m; i++) {
        curr[0] = i;

        for (int j = 1; j <= n; j++) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;

            curr[j] = std::min({
                prev[j] + 1,
                curr[j - 1] + 1,
                prev[j - 1] + cost
            });
        }

        std::swap(prev, curr);
    }

    return prev[n];
}

void print_matches(
    const std::vector<std::pair<int, int>>& locations,
    const std::string& pattern,
    const std::string& text) {

    if (locations.empty()) {
        std::cout << "None\n";
        return;
    }

    for (const auto& p : locations) {
        std::string matched_text = text.substr(
            p.first,
            p.second - p.first + 1
        );

        int distance = exact_edit_distance(pattern, matched_text);

        std::cout << "{"
                  << p.first << "," << p.second
                  << "} -> \"" << matched_text
                  << "\" -> edit distance = "
                  << distance << '\n';
    }
}

TestResult run_test(const TestCase& test, int test_number) {
    Ukkonen ukkonen;

    auto start = std::chrono::steady_clock::now();

    auto actual_locations =
        ukkonen.locate(test.pattern, test.text, test.k);

    auto end = std::chrono::steady_clock::now();

    auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        );

    int actual_count =
        static_cast<int>(actual_locations.size());

    bool all_distances_valid = true;

    for (const auto& p : actual_locations) {
        std::string matched_text = test.text.substr(
            p.first,
            p.second - p.first + 1
        );

        int distance =
            exact_edit_distance(test.pattern, matched_text);

        if (distance > static_cast<int>(test.k)) {
            all_distances_valid = false;
            break;
        }
    }

    bool passed =
        actual_count == test.expected_count &&
        actual_locations == test.expected_locations &&
        all_distances_valid;

    std::cout << "\n----------------------------------------\n";
    std::cout << "Test " << test_number << ": "
              << test.description << '\n';
    std::cout << "----------------------------------------\n";

    std::cout << "Status: "
              << (passed ? "PASS" : "FAIL") << '\n';

    std::cout << "Pattern: " << test.pattern << '\n';
    std::cout << "Text: " << test.text << '\n';
    std::cout << "k: " << test.k << '\n';

    std::cout << "Expected count: "
              << test.expected_count << '\n';

    std::cout << "Actual count:   "
              << actual_count << '\n';

    std::cout << "All actual distances <= k: "
              << (all_distances_valid ? "YES" : "NO") << '\n';

    std::cout << "Expected locations: ";

    if (test.expected_locations.empty()) {
        std::cout << "None\n";
    } else {
        for (const auto& p : test.expected_locations) {
            std::cout << "{"
                      << p.first << ","
                      << p.second << "} ";
        }
        std::cout << '\n';
    }

    std::cout << "Actual matches:\n";

    print_matches(
        actual_locations,
        test.pattern,
        test.text
    );

    std::cout << "Time: "
              << elapsed.count()
              << " us\n";

    return {passed, elapsed.count()};
}

int main() {
    const std::vector<TestCase> tests = {
        {"abc", "abc", 0, 1, {{0, 2}}, "Exact match"},
        {"aa", "aaaa", 0, 3, {{0, 1}, {1, 2}, {2, 3}}, "Multiple exact matches"},
        {"abc", "axc", 1, 1, {{0, 2}}, "One substitution"},
        {"abc", "abxc", 1, 3, {{0, 1}, {0, 2}, {0, 3}}, "Insertion creates multiple matching substrings"},
        {"abc", "ac", 1, 1, {{0, 1}}, "One deletion"},
        {"ab", "ab", 1, 3, {{0, 0}, {0, 1}, {1, 1}}, "All substrings within distance one"},
        {"abc", "xyz", 0, 0, {}, "No exact match"},
        {"abc", "xxabcxx", 0, 1, {{2, 4}}, "Exact match inside larger text"},
        {"a", "banana", 0, 3, {{1, 1}, {3, 3}, {5, 5}}, "Repeated exact character"},
        {"a", "banana", 1, 11,
            {{0,0},{0,1},{1,1},{1,2},{2,2},{2,3},{3,3},{3,4},{4,4},{4,5},{5,5}},
            "All substrings within distance one"},
        {"a", "b", 1, 1, {{0, 0}}, "Single substitution"},
        {"abcd", "ab", 2, 1, {{0, 1}}, "Two deletions"},
        {"abcd", "ab", 1, 0, {}, "Distance exceeds k"},
        {"abc", "abc", 3, 6, {{0,0},{0,1},{0,2},{1,1},{1,2},{2,2}}, "Large k"},
        {"aaaa", "aaaaaaaa", 1, 15,
            {{0,2},{0,3},{0,4},{1,3},{1,4},{1,5},{2,4},{2,5},{2,6},{3,5},{3,6},{3,7},{4,6},{4,7},{5,7}},
            "Repeated pattern with insertion tolerance"},
        {"abcdef", "xxabcde", 2, 4, {{1,6},{2,5},{2,6},{3,6}}, "Mixed length and edit differences"},
        {"abcdef", "abcdefx", 2, 8, {{0,3},{0,4},{0,5},{0,6},{1,4},{1,5},{1,6},{2,5}}, "Pattern with trailing insertion"},
        {"aaaaab", "aaaaaa", 1, 3, {{0,4},{0,5},{1,5}}, "Single substitution and length difference"},
        {"abcde", "abXde", 1, 1, {{0,4}}, "Middle substitution"},
        {"anb", "banana", 1, 4, {{1,2},{1,3},{3,4},{3,5}}, "Multiple approximate matches"}
    };

    int passed_count = 0;
    long long total_time = 0;
    long long min_time = -1;
    long long max_time = 0;

    for (int i = 0; i < static_cast<int>(tests.size()); i++) {
        TestResult result = run_test(tests[i], i + 1);

        if (result.passed) {
            passed_count++;
        }

        total_time += result.elapsed_microseconds;

        if (min_time == -1 ||
            result.elapsed_microseconds < min_time) {
            min_time = result.elapsed_microseconds;
        }

        if (result.elapsed_microseconds > max_time) {
            max_time = result.elapsed_microseconds;
        }
    }

    long long average_time =
        total_time /
        static_cast<long long>(tests.size());

    std::cout << "\n========================================\n";
    std::cout << "Small Test Summary\n";
    std::cout << "========================================\n";

    std::cout << "Passed: "
              << passed_count << "/"
              << tests.size() << '\n';

    std::cout << "Failed: "
              << tests.size() - passed_count << '\n';

    std::cout << "Total time: "
              << total_time << " us\n";

    std::cout << "Average time: "
              << average_time << " us\n";

    std::cout << "Minimum time: "
              << min_time << " us\n";

    std::cout << "Maximum time: "
              << max_time << " us\n";

    return passed_count ==
                   static_cast<int>(tests.size())
               ? 0
               : 1;
}