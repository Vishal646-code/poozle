#include "ukkonen.hpp"
#include "pz_cxx_std.hpp"

#include <chrono>
#include <fstream>


struct LargeCase {
    const char* file;
    const char* pattern;
    unsigned int k;
};


struct RapidFuzzReference {
    int test_number;
    std::string filename;
    std::string pattern;
    unsigned int k;
    int count;
};


std::string get_filename(const std::string& path) {
    std::size_t separator = path.find_last_of("/\\");

    if (separator == std::string::npos) {
        return path;
    }

    return path.substr(separator + 1);
}


bool parse_reference_line(
    const std::string& line,
    RapidFuzzReference& reference) {

    std::vector<std::string> fields;
    std::size_t start = 0;

    while (true) {
        std::size_t tab = line.find('\t', start);

        if (tab == std::string::npos) {
            fields.push_back(line.substr(start));
            break;
        }

        fields.push_back(
            line.substr(start, tab - start)
        );

        start = tab + 1;
    }

    if (fields.size() != 5) {
        return false;
    }

    try {
        reference.test_number = std::stoi(fields[0]);
        reference.filename = fields[1];
        reference.pattern = fields[2];
        reference.k =
            static_cast<unsigned int>(
                std::stoul(fields[3])
            );
        reference.count = std::stoi(fields[4]);
    } catch (...) {
        return false;
    }

    return true;
}


bool load_rapidfuzz_reference(
    const std::string& filename,
    std::vector<RapidFuzzReference>& references) {

    std::ifstream file(filename);

    if (!file) {
        return false;
    }

    std::string line;
    bool reference_section = false;

    while (std::getline(file, line)) {

        // Handle Windows CRLF line endings.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "RapidFuzz Reference Data") {
            reference_section = true;
            continue;
        }

        if (!reference_section) {
            continue;
        }

        if (line.empty()) {
            continue;
        }

        if (line == "========================================") {
            continue;
        }

        if (line == "Test\tFile\tPattern\tk\tCount") {
            continue;
        }

        RapidFuzzReference reference;

        if (parse_reference_line(line, reference)) {
            references.push_back(reference);
        }
    }

    return !references.empty();
}


bool find_rapidfuzz_reference(
    const std::vector<RapidFuzzReference>& references,
    const LargeCase& test,
    int test_number,
    int& expected_count) {

    const std::string filename =
        get_filename(test.file);

    for (const auto& reference : references) {

        if (reference.test_number == test_number &&
            reference.filename == filename &&
            reference.pattern == test.pattern &&
            reference.k == test.k) {

            expected_count = reference.count;
            return true;
        }
    }

    return false;
}


int main() {

    const std::vector<LargeCase> tests = {
        {"libpz/fuzzy/tests/data/large_1.txt",
         "Tama of the Light Country", 0},

        {"libpz/fuzzy/tests/data/large_1.txt",
         "Tama of the Light Countr", 1},

        {"libpz/fuzzy/tests/data/large_1.txt",
         "Tama", 0},

        {"libpz/fuzzy/tests/data/large_1.txt",
         "Light Country", 0},

        {"libpz/fuzzy/tests/data/large_2.txt",
         "Alice", 0},

        {"libpz/fuzzy/tests/data/large_2.txt",
         "Alic", 1},

        {"libpz/fuzzy/tests/data/large_2.txt",
         "Wonderland", 0},

        {"libpz/fuzzy/tests/data/large_2.txt",
         "Wonderlan", 1},

        {"libpz/fuzzy/tests/data/large_3.txt",
         "Sherlock Holmes", 0},

        {"libpz/fuzzy/tests/data/large_3.txt",
         "Sherlock", 0},

        {"libpz/fuzzy/tests/data/large_3.txt",
         "Holmes", 0},

        {"libpz/fuzzy/tests/data/large_3.txt",
         "Sherloc", 1}
    };


    const int repetitions = 5;

    const std::string reference_file =
        "libpz/fuzzy/outputs/output_rapidfuzz.txt";


    // --------------------------------------------------
    // Load the independently generated RapidFuzz results.
    // --------------------------------------------------

    std::vector<RapidFuzzReference> references;

    if (!load_rapidfuzz_reference(
            reference_file,
            references)) {

        std::cerr
            << "Could not load RapidFuzz reference data from:\n"
            << reference_file << '\n';

        std::cerr
            << "Run rapidfuzz_reference.py first.\n";

        return 1;
    }


    if (references.size() != tests.size()) {

        std::cerr
            << "RapidFuzz reference contains "
            << references.size()
            << " test cases, but Poozle expects "
            << tests.size()
            << ".\n";

        return 1;
    }


    bool all_passed = true;


    std::cout
        << "========================================\n";

    std::cout
        << "Large Fuzzy Search Benchmark\n";

    std::cout
        << "========================================\n";

    std::cout
        << "Runs per test: "
        << repetitions
        << '\n';

    std::cout
        << "Reported timing: median\n";

    std::cout
        << "Correctness reference: RapidFuzz\n";


    // --------------------------------------------------
    // Run every large test case.
    // --------------------------------------------------

    for (int test_number = 0;
         test_number < static_cast<int>(tests.size());
         ++test_number) {

        const LargeCase& test = tests[test_number];


        // ----------------------------------------------
        // Load the input text.
        // ----------------------------------------------

        std::ifstream file(test.file);

        if (!file) {

            std::cerr
                << "Could not open: "
                << test.file
                << '\n';

            return 1;
        }


        std::string text(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );


        // ----------------------------------------------
        // Find the corresponding RapidFuzz result.
        // ----------------------------------------------

        int rapidfuzz_count = -1;

        bool reference_found =
            find_rapidfuzz_reference(
                references,
                test,
                test_number + 1,
                rapidfuzz_count
            );


        if (!reference_found) {

            std::cerr
                << "No matching RapidFuzz reference "
                   "found for test "
                << test_number + 1
                << ".\n";

            all_passed = false;
            continue;
        }


        // ----------------------------------------------
        // Run Poozle five times.
        // ----------------------------------------------

        Ukkonen ukkonen;

        std::vector<double> timings;
        timings.reserve(repetitions);

        int poozle_count = -1;
        bool consistent = true;


        for (int run = 0;
             run < repetitions;
             ++run) {

            auto start =
                std::chrono::steady_clock::now();


            int current_count =
                ukkonen.count(
                    test.pattern,
                    text,
                    test.k
                );


            auto end =
                std::chrono::steady_clock::now();


            std::chrono::duration<double> elapsed =
                end - start;


            timings.push_back(
                elapsed.count()
            );


            if (run == 0) {
                poozle_count = current_count;
            }
            else if (current_count != poozle_count) {
                consistent = false;
            }
        }


        // ----------------------------------------------
        // Compare Poozle against RapidFuzz.
        // ----------------------------------------------

        bool count_matches =
            poozle_count == rapidfuzz_count;

        bool test_passed =
            consistent && count_matches;


        if (!test_passed) {
            all_passed = false;
        }


        // ----------------------------------------------
        // Calculate median timing.
        // ----------------------------------------------

        std::vector<double> sorted_timings =
            timings;

        std::sort(
            sorted_timings.begin(),
            sorted_timings.end()
        );

        double median =
            sorted_timings[repetitions / 2];


        // ----------------------------------------------
        // Print test result.
        // ----------------------------------------------

        std::cout
            << "\n----------------------------------------\n";

        std::cout
            << "Test "
            << test_number + 1
            << ": "
            << test.pattern
            << '\n';

        std::cout
            << "----------------------------------------\n";

        std::cout
            << "Status: "
            << (test_passed ? "PASS" : "FAIL")
            << '\n';

        std::cout
            << "File: "
            << test.file
            << '\n';

        std::cout
            << "Pattern: "
            << test.pattern
            << '\n';

        std::cout
            << "k: "
            << test.k
            << '\n';

        std::cout
            << "Text size: "
            << text.size()
            << " bytes\n";

        std::cout
            << "Poozle count: "
            << poozle_count
            << '\n';

        std::cout
            << "RapidFuzz count: "
            << rapidfuzz_count
            << '\n';

        std::cout
            << "Count comparison: "
            << (count_matches ? "PASS" : "FAIL")
            << '\n';

        std::cout
            << "Result consistency: "
            << (consistent ? "PASS" : "FAIL")
            << '\n';


        std::cout
            << "\nTiming:\n";


        for (int run = 0;
             run < repetitions;
             ++run) {

            std::cout
                << "  Run "
                << run + 1
                << ": "
                << timings[run]
                << " s\n";
        }


        std::cout
            << "  Median: "
            << median
            << " s\n";
    }


    // --------------------------------------------------
    // Final summary.
    // --------------------------------------------------

    std::cout
        << "\n========================================\n";

    std::cout
        << "Large Test Summary\n";

    std::cout
        << "========================================\n";

    std::cout
        << "Tests completed: "
        << tests.size()
        << '\n';

    std::cout
        << "Runs per test: "
        << repetitions
        << '\n';

    std::cout
        << "Poozle vs RapidFuzz: "
        << (all_passed
                ? "ALL MATCH"
                : "MISMATCH FOUND")
        << '\n';

    std::cout
        << "Overall status: "
        << (all_passed
                ? "PASS"
                : "FAIL")
        << '\n';


    // --------------------------------------------------
    // Benchmark methodology.
    // --------------------------------------------------

    std::cout
        << "\n========================================\n";

    std::cout
        << "Benchmark Method\n";

    std::cout
        << "========================================\n";

    std::cout
        << "Poozle timer: "
           "std::chrono::steady_clock\n";

    std::cout
        << "Reported value: median of 5 runs\n";

    std::cout
        << "File loading: excluded from Poozle timing\n";

    std::cout
        << "Correctness reference: "
           "RapidFuzz Levenshtein distance\n";


    return all_passed ? 0 : 1;
}