from pathlib import Path
from time import perf_counter

from rapidfuzz.distance import Levenshtein


TESTS = [
    ("large_1.txt", "Tama of the Light Country", 0),
    ("large_1.txt", "Tama of the Light Countr", 1),
    ("large_1.txt", "Tama", 0),
    ("large_1.txt", "Light Country", 0),
    ("large_2.txt", "Alice", 0),
    ("large_2.txt", "Alic", 1),
    ("large_2.txt", "Wonderland", 0),
    ("large_2.txt", "Wonderlan", 1),
    ("large_3.txt", "Sherlock Holmes", 0),
    ("large_3.txt", "Sherlock", 0),
    ("large_3.txt", "Holmes", 0),
    ("large_3.txt", "Sherloc", 1),
]


TEST_DIR = Path(__file__).resolve().parent
DATA_DIR = TEST_DIR / "data"
OUTPUT_DIR = TEST_DIR.parent / "outputs"

FINAL_OUTPUT = OUTPUT_DIR / "output_rapidfuzz.txt"


def count_matches(pattern, text, k):
    m = len(pattern)

    min_length = max(1, m - k)
    max_length = m + k

    count = 0

    for start in range(len(text)):
        remaining = len(text) - start

        if remaining < min_length:
            break

        last_length = min(max_length, remaining)

        for length in range(
            min_length,
            last_length + 1
        ):
            substring = text[
                start:start + length
            ]

            if Levenshtein.distance(
                pattern,
                substring
            ) <= k:
                count += 1

    return count


def main():
    repetitions = 5
    all_passed = True

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    reference_counts = []

    with FINAL_OUTPUT.open(
        "w",
        encoding="utf-8"
    ) as output:

        output.write(
            "========================================\n"
        )
        output.write(
            "RapidFuzz Reference Benchmark\n"
        )
        output.write(
            "========================================\n"
        )
        output.write(
            f"Runs per test: {repetitions}\n"
        )
        output.write(
            "Reported timing: median\n"
        )
        output.write(
            "Correctness reference: "
            "rapidfuzz.distance.Levenshtein\n"
        )
        output.write("\n")

        for test_number, (
            filename,
            pattern,
            k
        ) in enumerate(TESTS, start=1):

            data_file = DATA_DIR / filename

            if not data_file.exists():
                print(
                    f"Missing data file: {data_file}"
                )
                return 1

            text = data_file.read_text(
                encoding="utf-8",
                errors="ignore"
            )

            timings = []
            rapidfuzz_count = None
            consistent = True

            for _ in range(repetitions):
                start = perf_counter()

                current_count = count_matches(
                    pattern,
                    text,
                    k
                )

                elapsed = perf_counter() - start

                timings.append(elapsed)

                if rapidfuzz_count is None:
                    rapidfuzz_count = current_count
                elif current_count != rapidfuzz_count:
                    consistent = False

            sorted_timings = sorted(timings)

            median_time = sorted_timings[
                repetitions // 2
            ]

            if not consistent:
                all_passed = False

            reference_counts.append(
                (
                    test_number,
                    filename,
                    pattern,
                    k,
                    rapidfuzz_count
                )
            )

            output.write(
                "----------------------------------------\n"
            )

            output.write(
                f"Test {test_number}: {pattern}\n"
            )

            output.write(
                "----------------------------------------\n"
            )

            output.write(
                f"File: {filename}\n"
            )

            output.write(
                f"Pattern: {pattern}\n"
            )

            output.write(
                f"k: {k}\n"
            )

            output.write(
                "Text size: "
                f"{len(text.encode('utf-8'))} bytes\n"
            )

            output.write(
                f"RapidFuzz count: "
                f"{rapidfuzz_count}\n"
            )

            output.write(
                "Result consistency: "
                f"{'PASS' if consistent else 'FAIL'}\n"
            )

            output.write("\n")
            output.write("Timing:\n")

            for run, timing in enumerate(
                timings,
                start=1
            ):
                output.write(
                    f"  Run {run}: "
                    f"{timing:.6f} s\n"
                )

            output.write(
                f"  Median: "
                f"{median_time:.6f} s\n"
            )

            output.write("\n")

        output.write(
            "========================================\n"
        )
        output.write(
            "RapidFuzz Test Summary\n"
        )
        output.write(
            "========================================\n"
        )
        output.write(
            f"Tests completed: {len(TESTS)}\n"
        )
        output.write(
            f"Runs per test: {repetitions}\n"
        )
        output.write(
            "All results consistent: "
            f"{'YES' if all_passed else 'NO'}\n"
        )

        # Machine-readable reference data.
        # This is kept in the same output file so that
        # Poozle can compare against the independently
        # generated RapidFuzz counts without requiring
        # a second reference file.
        output.write("\n")
        output.write(
            "========================================\n"
        )
        output.write(
            "RapidFuzz Reference Data\n"
        )
        output.write(
            "========================================\n"
        )
        output.write(
            "Test\tFile\tPattern\tk\tCount\n"
        )

        for (
            test_number,
            filename,
            pattern,
            k,
            count
        ) in reference_counts:

            output.write(
                f"{test_number}\t"
                f"{filename}\t"
                f"{pattern}\t"
                f"{k}\t"
                f"{count}\n"
            )

    print(
        "RapidFuzz benchmark written to:\n"
        f"{FINAL_OUTPUT}"
    )

    return 0 if all_passed else 1


if __name__ == "__main__":
    raise SystemExit(main())