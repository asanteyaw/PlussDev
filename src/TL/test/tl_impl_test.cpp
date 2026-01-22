#include <iostream>
#include <pluss/pluss.h>

// ===== Test Helpers =====
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RED     "\033[1;31m"

int total_tests = 0, passed_tests = 0;

#define EXPECT_TRUE(expr) test_assert((expr), #expr, __FILE__, __LINE__)
#define EXPECT_FALSE(expr) test_assert(!(expr), "!(" #expr ")", __FILE__, __LINE__)
#define EXPECT_EQ(actual, expected) test_equal((actual), (expected), #actual, #expected, __FILE__, __LINE__)

template <typename T1, typename T2>
void test_equal(const T1& actual, const T2& expected,
                const char* act_str, const char* exp_str,
                const char* file, int line) {
    total_tests++;
    if (actual == expected) {
        passed_tests++;
        std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET
                  << act_str << " == " << exp_str << "\n";
    } else {
        std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET
                  << act_str << " != " << exp_str
                  << " at " << file << ":" << line << "\n";
        std::cout << "  Actual:   " << actual << "\n";
        std::cout << "  Expected: " << expected << "\n";
    }
}

void test_assert(bool condition, const char* expr_str,
                 const char* file, int line) {
    total_tests++;
    if (condition) {
        passed_tests++;
        std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET
                  << expr_str << "\n";
    } else {
        std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET
                  << expr_str << " at " << file << ":" << line << "\n";
    }
}

// ===== Timeline Tests =====
void run_tests() {
    using pluss::Timeline;

    // --- Construction ---
    Timeline tl;
    EXPECT_EQ(tl.size(), 0);

    // --- Insert points ---
    tl.insert(0.0, 10);
    tl.insert(1.0, 20);
    tl.insert(2.5, 30);

    EXPECT_EQ(tl.size(), 3);

    // --- Access by index ---
    EXPECT_EQ(tl.at(0).value, 10);
    EXPECT_EQ(tl.at(1).value, 20);
    EXPECT_EQ(tl.at(2).value, 30);

    // --- Time ordering ---
    EXPECT_TRUE(tl.at(0).time < tl.at(1).time);
    EXPECT_TRUE(tl.at(1).time < tl.at(2).time);

    // --- Query by time (if supported) ---
    EXPECT_EQ(tl.value_at(1.0), 20);

    // --- Boundary behavior ---
    EXPECT_TRUE(tl.contains(0.0));
    EXPECT_FALSE(tl.contains(9.9));

    // --- Clear ---
    tl.clear();
    EXPECT_EQ(tl.size(), 0);

    std::cout << "\nTimeline basic tests completed.\n";
}

// ===== Main =====
int main() {
    run_tests();
    std::cout << "\nTest results: "
              << passed_tests << "/" << total_tests
              << " passed.\n";
    return passed_tests == total_tests ? 0 : 1;
}