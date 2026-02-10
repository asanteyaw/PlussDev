#include <cstdlib>
#include <iostream>
#include <cstddef>
#include <pluss/pluss.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RED     "\033[1;31m"

int total_tests = 0, passed_tests = 0;

#define EXPECT_TRUE(expr) test_assert((expr), #expr, __FILE__, __LINE__)
#define EXPECT_FALSE(expr) test_assert(!(expr), "!(" #expr ")", __FILE__, __LINE__)
#define EXPECT_EQ(actual, expected) test_equal((actual), (expected), #actual, #expected, __FILE__, __LINE__)

template <size_t N>
void test_equal(const pluss::Chord& actual, const char (&expected)[N], const char* act_str, const char* exp_str, const char* file, int line) {
    total_tests++;
    if (actual.str() == expected) {
        passed_tests++;
        std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << act_str << " == " << exp_str << "\n";
    } else {
        std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << act_str << " != " << exp_str << " at " << file << ":" << line << "\n";
        std::cout << "  Actual:   " << actual << "\n";
        std::cout << "  Expected: " << expected << "\n";
    }
}

template <typename T1, typename T2>
void test_equal(const T1& actual, const T2& expected, const char* act_str, const char* exp_str, const char* file, int line) {
    total_tests++;
    if (actual == expected) {
        passed_tests++;
        std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << act_str << " == " << exp_str << "\n";
    } else {
        std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << act_str << " != " << exp_str << " at " << file << ":" << line << "\n";
        std::cout << "  Actual:   " << actual << "\n";
        std::cout << "  Expected: " << expected << "\n";
    }
}

void test_assert(bool condition, const char* expr_str, const char* file, int line) {
    total_tests++;
    if (condition) {
        passed_tests++;
        std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << expr_str << "\n";
    } else {
        std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << expr_str << " at " << file << ":" << line << "\n";
    }
}

void run_note_tests() {
    using namespace pluss;

    // --- Content construction ---
    Note n0 = content();  // Empty Note
    Note n1 = content("Hello People");
    Note n2 = content({"Hello", 'A', std::string("World")});  // 1D
    Note n3 = content({{"Row1", "Row2"}, {"Row3", "Row4"}});  // 2D
    Note n4 = content({
        {
            {"a", "b"},
            {"c", "d"}
        },
        {
            {"e", "f"},
            {"g", "h"}
        }
    }); // 3D

    // --- Scalar Note ---
    Note s = content("h");
    EXPECT_EQ(s.to_upper().scalar().str(), "H");
    EXPECT_EQ(s.to_lower().scalar().str(), "h");
    EXPECT_TRUE(s.is_alpha()[0]);

    Note a = content("H");
    Note b = content("i");
    EXPECT_EQ(a.add(b).scalar().str(), "Hi");
    EXPECT_TRUE(a.eq(a)[0]);
    EXPECT_FALSE(a.neq(a)[0]);

    // --- Vector Note ---
    Note v = content({"Hi", "There", "World"});
    EXPECT_EQ(v.to_upper().vector()[1], "THERE");
    EXPECT_TRUE(v.is_alpha()[2]);

    Note v2 = content({"X", "Y", "Z"});
    EXPECT_EQ(v.add(v2).vector()[1], "ThereY");

    // --- Ternary where ---
    Note cond = content({"1", "0", "1"});
    Note t = cond.where(v, v2);
    EXPECT_EQ(t.vector()[0], "Hi");
    EXPECT_EQ(t.vector()[1], "Y");
    EXPECT_EQ(t.vector()[2], "World");

    Note sc = content("1");
    Note result = sc.where(v, v2);
    EXPECT_EQ(result.vector()[2], "World");

    Note mat = content({{"A", "B"}, {"C", "D"}});
    Note mat2 = content({{"1", "2"}, {"3", "4"}});
    Note where_result = sc.where(mat, mat2);
    EXPECT_EQ(where_result.matrix()[1][0], "C");

    // --- Matrix ---
    Note m = content({{"Hi", "John"}, {"123", "DoE"}});
    EXPECT_EQ(m.to_upper().matrix()[0][1], "JOHN");
    // is_alpha() is elementwise over the flattened Note; "123" is not alphabetic.
    EXPECT_FALSE(m.is_alpha()[2]);

    // --- Transpose ---
    Note rect = content({{"A", "B", "C"}, {"D", "E", "F"}});
    Note trans = rect.T();
    EXPECT_EQ(trans.matrix()[0][1], "D");
    EXPECT_EQ(trans.matrix()[2][1], "F");

    // --- Stack ---
    Note s1 = content("x");
    Note s2 = content("y");
    Note s3 = s1.stack({s2}, 0);
    EXPECT_EQ(s3.vector()[1], "y");

    Note row1 = content({"a", "b"});
    Note row2 = content({"c", "d"});
    Note stacked = row1.stack({row2}, 0);
    EXPECT_EQ(stacked.matrix()[1][0], "c");

    // Stack 1D Notes into 2D along dim=0
    Note nn1 = content({"A", "B"});
    Note nn2 = content({"C", "D"});
    Note stacked0 = nn1.stack({nn2}, 0);  // shape [2, 2]

    EXPECT_EQ(stacked0.ndim(), 2);
    EXPECT_EQ(stacked0.shape()[0], 2);
    EXPECT_EQ(stacked0.shape()[1], 2);
    EXPECT_EQ(stacked0.matrix()[0][0], "A");
    EXPECT_EQ(stacked0.matrix()[1][1], "D");

    // Stack same 1D Notes into 2D along dim=1
    Note stacked1 = nn1.stack({nn2}, 1);  // shape [2, 2]
    EXPECT_EQ(stacked1.ndim(), 2);
    EXPECT_EQ(stacked1.shape()[0], 2);
    EXPECT_EQ(stacked1.shape()[1], 2);
    EXPECT_EQ(stacked1.matrix()[0][0], "A");
    EXPECT_EQ(stacked1.matrix()[0][1], "C");

    // Stack 2D Notes into 3D
    Note m1 = content({{"a", "b"}, {"c", "d"}});
    Note m2 = content({{"e", "f"}, {"g", "h"}});
    Note mstack = m1.stack({m2}, 0);  // shape [2, 2, 2]

    EXPECT_EQ(mstack.ndim(), 3);
    EXPECT_EQ(mstack.shape()[0], 2);
    EXPECT_EQ(mstack.shape()[1], 2);
    EXPECT_EQ(mstack.shape()[2], 2);
    EXPECT_EQ(mstack.flatten()[0], "a");
    EXPECT_EQ(mstack.flatten()[3], "d");

    // Stack with more than 2 Notes
    Note m3 = content({{"x", "y"}, {"z", "w"}});
    Note bigstack = m1.stack({m2, m3}, 0);  // shape [3, 2, 2]

    EXPECT_EQ(bigstack.ndim(), 3);
    EXPECT_EQ(bigstack.shape()[0], 3);
    EXPECT_EQ(bigstack.shape()[1], 2);
    EXPECT_EQ(bigstack.shape()[2], 2);
    EXPECT_EQ(bigstack.flatten()[0], "a");
    EXPECT_EQ(bigstack.flatten()[4], "e");
    EXPECT_EQ(bigstack.flatten()[8], "x");

    // Stack with dim=2
    Note mstack2 = m1.stack({m2}, 2);  // shape [2, 2, 2]
    EXPECT_EQ(mstack2.shape()[0], 2);
    EXPECT_EQ(mstack2.shape()[1], 2);
    EXPECT_EQ(mstack2.shape()[2], 2);
    EXPECT_EQ(mstack2.flatten()[0], "a");
    EXPECT_EQ(mstack2.flatten()[1], "e");
    EXPECT_EQ(mstack2.flatten()[3], "f");

    // Edge: stack identical scalars (scalar -> vector)
    Note ss = content("Z");
    Note sstack = ss.stack({ss, ss}, 0);  // shape [3]
    EXPECT_EQ(sstack.ndim(), 1);
    EXPECT_EQ(sstack.shape()[0], 3);
    EXPECT_EQ(sstack.vector()[2], "Z");

    // --- Slice ---
    Note big = content({{"X1", "X2"}, {"Y1", "Y2"}, {"Z1", "Z2"}});
    Note sliced = big.slice({1});
    EXPECT_EQ(sliced.vector()[0], "Y1");

    // --- Squeeze/Unsqueeze ---
    // Note singleton = content({{{"hi"}}});
    // Note squeezed = singleton.squeeze(0).squeeze(0);
    // EXPECT_EQ(squeezed.scalar().str(), "hi");

    // Note expanded = squeezed.unsqueeze(0).unsqueeze(0);
    // EXPECT_EQ(expanded.ndim(), 2);
    // EXPECT_EQ(expanded.shape()[0], 1);
    // EXPECT_EQ(expanded.shape()[1], 1);

    // --- Map ---
    Note mapped = v.map([](const Chord& c) {
        return Chord(c.str() + "!");
    });
    EXPECT_EQ(mapped.vector()[1], "There!");

    // --- New NLP helpers: bag-of-words and similarity ---
    auto bow = v.to_bow();
    // tokenize()/normalize_token() lowercases
    EXPECT_TRUE(bow["hi"] >= 1);
    Note t1 = content("hello world hello");
    Note t2 = content("hello there");
    double sim = t1.similarity(t2);
    EXPECT_TRUE(sim > 0.0 && sim <= 1.0);

    // --- Repeat ---
    Note base = content({"A", "B"});
    Note rep = base.repeat({2});
    EXPECT_EQ(rep.vector()[0], "A");
    EXPECT_EQ(rep.vector()[1], "A");
    EXPECT_EQ(rep.vector()[2], "B");
    EXPECT_EQ(rep.vector()[3], "B");

    Note rep2d = base.repeat({2, 1});
    EXPECT_EQ(rep2d.matrix()[1][0], "A");

    std::cout << "1D repeat:\n" << rep << "\n";
    std::cout << "2D repeat:\n" << rep2d << "\n";

    // --- Flatten ---
    Note deep = content({{"A", "B"}, {"C", "D"}});
    auto flat = deep.flatten();
    EXPECT_EQ(flat[2], "C");
    std::cout << "Flattened Note:\n";
    for (const auto& item : flat) {
        std::cout << "  " << item << "\n";
    }

    // --- Debug/Shape ---
    EXPECT_EQ(content("ok").debug(), "ok");
    EXPECT_EQ(content({"a", "b"}).shape().size(), 1);
    EXPECT_EQ(content({{"a", "b"}, {"c", "d"}}).shape()[0], 2);
}

int main() {
    run_note_tests();
    std::cout << "\nTest results: " << passed_tests << "/" << total_tests << " passed.\n";
    return passed_tests == total_tests ? 0 : 1;
}
