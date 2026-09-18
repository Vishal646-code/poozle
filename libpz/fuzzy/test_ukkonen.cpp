#include "ukkonen.hpp"
#include "pz_cxx_std.hpp"

#include <chrono>

struct TestCase {
    std::string pattern;
    std::string text;
    int k;
    int expected_count;
    std::vector<std::pair<int, int>> expected_locations;
    std::string description;
};

bool run_test(const TestCase& test) {
    Ukkonen ukkonen;
    auto start = std::chrono::high_resolution_clock::now();
    int actual_count = ukkonen.count(test.pattern, test.text, test.k);
    auto actual_locations =
        ukkonen.locate(test.pattern, test.text, test.k);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - start);

    bool passed =
        actual_count == test.expected_count &&
        actual_locations == test.expected_locations;

    std::cout << "\n[" << (passed ? "PASS" : "FAIL") << "] "
              << test.description << '\n';
    std::cout << "Pattern: " << test.pattern << '\n';
    std::cout << "Text length: " << test.text.size() << '\n';
    std::cout << "k: " << test.k << '\n';
    std::cout << "Expected count: " << test.expected_count << '\n';
    std::cout << "Actual count:   " << actual_count << '\n';
    std::cout << "Expected locations: ";
    for (const auto& p : test.expected_locations) {
        std::cout << "{" << p.first << ", " << p.second << "} ";
    }
    std::cout << '\n';

    std::cout << "Actual locations:   ";
    for (const auto& p : actual_locations) {
        std::cout << "{" << p.first << ", " << p.second << "} ";
    }
    std::cout << '\n';

    std::cout << "Execution time: "
              << duration.count() << " us\n";

    return passed;
}

bool run_large_tests(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not open large test file: "
                  << filename << '\n';
        return false;
    }
    struct LargeTest {
        int test_number;
        int expected_count;
        std::string pattern;
        int k;
    };

    const std::vector<LargeTest> tests = {
        {21,  9991,  "aaaaaaaaaa",                    0},
        {22,  4996,  "ababababab",                   0},
        {23,  3332,  "abcabc",                       0},
        {24, 74973,  "aaaaaaaaaa",                    1},
        {25,     0,  "zzzzzzzzzzzzzzzzzzzz",          0},
        {26,     1,  "thequickbrownfox",              0},
        {27, 12499,  "abcdabcd",                      0},
        {28, 69248,  "aaaaaaaaaa",                    1},
        {29,     0,  "zzzzzzzzzzzzzzzzzzzz",          0},
        {30, 99981,  "aaaaaaaaaaaaaaaaaaaa",          0},
        {31, 33330,  "abcabcabcabc",                  0},
        {32,     0,  "zzzzzzzzzzzzzzzzzzzz",          0},
        {33, 99981,  "aaaaaaaaaa",                    0},
        {34,249973,  "abababababab",                  1},
        {35,  3846,  "abcdefghijkl",                 0},
        {36, 10000,  "abcdefghij",                   0},
        {37,145898,  "aaaaaaaaaaaaaaaa",              1},
        {38,     0,  "abcdefghijklmnopqr",            1},
        {39,     1,  "quantumsearch",                 0},
        {40,     0,  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                    "aaaaaaaaaaaaaaaaaa",             2}
    };

    int passed = 0;
    int failed = 0;

    for (const auto& test : tests) {
        std::string line;
        std::getline(file, line); // TEST N

        int text_length;
        file >> text_length;
        file.ignore();

        std::string text;
        std::getline(file, text);

        if (static_cast<int>(text.size()) != text_length) {
            std::cerr << "Test " << test.test_number
                      << ": text length mismatch\n";
            ++failed;
            continue;
        }

        Ukkonen ukkonen;

        auto start = std::chrono::high_resolution_clock::now();

        int actual_count =
            ukkonen.count(test.pattern, text, test.k);

        auto end = std::chrono::high_resolution_clock::now();

        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start);

        bool passed_test =
            actual_count == test.expected_count;

        std::cout << "\n["
                  << (passed_test ? "PASS" : "FAIL")
                  << "] Test " << test.test_number << '\n';

        std::cout << "Text length: "
                  << text.size() << '\n';

        std::cout << "Pattern length: "
                  << test.pattern.size() << '\n';

        std::cout << "k: "
                  << test.k << '\n';

        std::cout << "Expected count: "
                  << test.expected_count << '\n';

        std::cout << "Actual count:   "
                  << actual_count << '\n';

        std::cout << "Execution time: "
                  << duration.count() << " ms\n";

        if (passed_test) {
            ++passed;
        } else {
            ++failed;
        }
    }

    std::cout << "\n========================================\n";
    std::cout << "Large Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Passed: " << passed << '\n';
    std::cout << "Failed: " << failed << '\n';
    std::cout << "Total:  " << tests.size() << '\n';

    return failed == 0;
}

int main() {

    std::vector<TestCase> tests = {

        {
            "abc", "abc", 0,
            1,
            {{0, 2}},
            "Exact match"
        },

        {
            "aa", "aaaa", 0,
            3,
            {{0, 1}, {1, 2}, {2, 3}},
            "Overlapping exact matches"
        },

        {
            "abc", "axc", 1,
            1,
            {{0, 2}},
            "Single substitution"
        },

        {
            "abc", "abxc", 1,
            3,
            {{0, 1}, {0, 2}, {0, 3}},
            "Insertion and different substring lengths"
        },

        {
            "abc", "ac", 1,
            1,
            {{0, 1}},
            "Single deletion"
        },

        {
            "ab", "ab", 1,
            3,
            {{0, 0}, {0, 1}, {1, 1}},
            "Multiple fuzzy substrings with same start"
        },

        {
            "abc", "xyz", 0,
            0,
            {},
            "No match"
        },

        {
            "abc", "xxabcxx", 0,
            1,
            {{2, 4}},
            "Exact match in middle"
        },

        {
            "a", "banana", 0,
            3,
            {{1, 1}, {3, 3}, {5, 5}},
            "Repeated exact matches"
        },

        {
            "a", "banana", 1,
            11,
            {{0, 0}, {0, 1}, {1, 1}, {1, 2},
             {2, 2}, {2, 3}, {3, 3}, {3, 4},
             {4, 4}, {4, 5}, {5, 5}},
            "Many fuzzy matches"
        },

        {
            "a", "b", 1,
            1,
            {{0, 0}},
            "Single-character substitution"
        },

        {
            "abcd", "ab", 2,
            1,
            {{0, 1}},
            "Maximum deletion boundary"
        },

        {
            "abcd", "ab", 1,
            0,
            {},
            "Beyond deletion boundary"
        },

        {
            "abc", "abc", 3,
            6,
            {{0, 0}, {0, 1}, {0, 2},
             {1, 1}, {1, 2}, {2, 2}},
            "k greater than pattern length"
        },

        {
            "aaaa", "aaaaaaaa", 1,
            15,
            {{0, 2}, {0, 3}, {0, 4},
             {1, 3}, {1, 4}, {1, 5},
             {2, 4}, {2, 5}, {2, 6},
             {3, 5}, {3, 6}, {3, 7},
             {4, 6}, {4, 7},
             {5, 7}},
            "Many overlapping fuzzy matches"
        },

        {
            "abcdef", "xxabcde", 2,
            4,
            {{1, 6}, {2, 5}, {2, 6}, {3, 6}},
            "Match near beginning with offset"
        },

        {
            "abcdef", "abcdefx", 2,
            8,
            {{0, 3}, {0, 4}, {0, 5}, {0, 6}, {1, 4}, {1, 5}, {1, 6}, {2, 5}},
            "Match near end with length variation"
        },

        {
            "aaaaab", "aaaaaa", 1,
            3,
            {{0, 4}, {0, 5}, {1, 5}},
            "Repeated-character boundary case"
        },

        {
            "abcde", "abXde", 1,
            1,
            {{0, 4}},
            "Internal substitution"
        },

        {
            "anb", "banana", 1,
            4,
            {{1, 2}, {1, 3}, {3, 4}, {3, 5}},
            "anb in banana"
        }
    };

    int passed = 0;
    int failed = 0;

    std::cout << "========================================\n";
    std::cout << "Ukkonen Fuzzy Search - Small Tests\n";
    std::cout << "========================================\n";

    for (const auto& test : tests) {
        if (run_test(test)) {
            ++passed;
        } else {
            ++failed;
        }
    }

    std::cout << "\n========================================\n";
    std::cout << "Small Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Passed: " << passed << '\n';
    std::cout << "Failed: " << failed << '\n';
    std::cout << "Total:  " << tests.size() << '\n';

    std::cout << "\n========================================\n";
    std::cout << "Running Large Tests\n";
    std::cout << "========================================\n";

    bool large_tests_passed = run_large_tests("/root/large_tests.txt");
    return (failed == 0 && large_tests_passed) ? 0 : 1;
}