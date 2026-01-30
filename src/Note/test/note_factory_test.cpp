#include <pluss/pluss.h>
#include <iostream>
#include <vector>
#include <string>

// ===== Test Helpers =====
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RED     "\033[1;31m"

int total_tests = 0, passed_tests = 0;

#define EXPECT_TRUE(expr) test_assert((expr), #expr, __FILE__, __LINE__)
#define EXPECT_FALSE(expr) test_assert(!(expr), "!(" #expr ")", __FILE__, __LINE__)
#define EXPECT_EQ(actual, expected) test_equal((actual), (expected), #actual, #expected, __FILE__, __LINE__)

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

// ====== Tests ======

void test_content_function() {
    std::cout << "\n=== Testing content() function ===\n";
    
    // Test 0D content (scalar)
    // Note: content() requires a TextStructure, so we use make_note for scalar
    // pluss::Note scalar = pluss::make_note(let::lex("hello"));
    // EXPECT_EQ(scalar.ndim(), 0);
    // EXPECT_EQ(scalar.scalar().str(), "hello");
    
    // Test 1D content (vector)
    pluss::Note vec = pluss::content(let::TextStructure({"a", "b", "c"}));
    EXPECT_EQ(vec.ndim(), 1);
    EXPECT_EQ(vec.shape()[0], 3);
    EXPECT_EQ(vec.vector()[0], "a");
    EXPECT_EQ(vec.vector()[1], "b");
    EXPECT_EQ(vec.vector()[2], "c");
    
    // Test 2D content (matrix)
    pluss::Note mat = pluss::content(let::TextStructure({{"1", "2"}, {"3", "4"}}));
    EXPECT_EQ(mat.ndim(), 2);
    EXPECT_EQ(mat.shape()[0], 2);
    EXPECT_EQ(mat.shape()[1], 2);
    EXPECT_EQ(mat.matrix()[0][0], "1");
    EXPECT_EQ(mat.matrix()[1][1], "4");
    
    // Test 3D content
    pluss::Note tensor3d = pluss::content(let::TextStructure({
        {{"a", "b"}, {"c", "d"}},
        {{"e", "f"}, {"g", "h"}}
    }));
    EXPECT_EQ(tensor3d.ndim(), 3);
    EXPECT_EQ(tensor3d.shape()[0], 2);
    EXPECT_EQ(tensor3d.shape()[1], 2);
    EXPECT_EQ(tensor3d.shape()[2], 2);
    
    // Test empty content
    pluss::Note empty = pluss::content({});
    EXPECT_EQ(empty.ndim(), 1);
    EXPECT_EQ(empty.shape().size(), 1);
    EXPECT_EQ(empty.shape()[0], 0);
}

void test_make_note_function() {
    std::cout << "\n=== Testing make_note() function ===\n";
    
    // Test scalar forms
    pluss::Note n1 = pluss::make_note("hello");
    EXPECT_EQ(n1.scalar().str(), "hello");
    
    pluss::Note n2 = pluss::make_note('x');
    EXPECT_EQ(n2.scalar().str(), "x");
    
    std::string str = "world";
    pluss::Note n3 = pluss::make_note(str);
    EXPECT_EQ(n3.scalar().str(), "world");
    
    // Test vector forms from initializer lists
    pluss::Note v1 = pluss::make_note({'a', 'b', 'c'});
    EXPECT_EQ(v1.vector().size(), 3);
    EXPECT_EQ(v1.vector()[0], "a");
    
    pluss::Note v2 = pluss::make_note({"one", "two", "three"});
    EXPECT_EQ(v2.vector().size(), 3);
    EXPECT_EQ(v2.vector()[1], "two");
    
    pluss::Note v3 = pluss::make_note({pluss::Chord("x"), pluss::Chord("y")});
    EXPECT_EQ(v3.vector().size(), 2);
    EXPECT_EQ(v3.vector()[1], "y");
    
    // Test matrix forms from initializer lists
    pluss::Note m1 = pluss::make_note({{'a', 'b'}, {'c', 'd'}});
    EXPECT_EQ(m1.matrix().size(), 2);
    EXPECT_EQ(m1.matrix()[0].size(), 2);
    EXPECT_EQ(m1.matrix()[1][0], "c");
    
    pluss::Note m2 = pluss::make_note({{"a1", "a2"}, {"b1", "b2"}});
    EXPECT_EQ(m2.matrix().size(), 2);
    EXPECT_EQ(m2.matrix()[0].size(), 2);
    EXPECT_EQ(m2.matrix()[1][1], "b2");
    
    // Test from C++ containers
    std::vector<pluss::Chord> vec = {pluss::Chord("x"), pluss::Chord("y"), pluss::Chord("z")};
    pluss::Note v4 = pluss::make_note(vec);
    EXPECT_EQ(v4.vector().size(), 3);
    EXPECT_EQ(v4.vector()[2], "z");
    
    std::vector<std::vector<pluss::Chord>> mat = {
        {pluss::Chord("a"), pluss::Chord("b")},
        {pluss::Chord("c"), pluss::Chord("d")}
    };
    pluss::Note m3 = pluss::make_note(mat);
    EXPECT_EQ(m3.matrix().size(), 2);
    EXPECT_EQ(m3.matrix()[0].size(), 2);
    EXPECT_EQ(m3.matrix()[1][0], "c");
    
    // Test empty forms
    pluss::Note empty1 = pluss::make_note();
    EXPECT_EQ(empty1.ndim(), 1);
    EXPECT_EQ(empty1.shape()[0], 0);
    
    std::vector<pluss::Chord> empty_vec;
    pluss::Note empty2 = pluss::make_note(empty_vec);
    EXPECT_EQ(empty2.ndim(), 1);
    EXPECT_EQ(empty2.shape()[0], 0);
    
    std::vector<std::vector<pluss::Chord>> empty_mat;
    pluss::Note empty3 = pluss::make_note(empty_mat);
    EXPECT_EQ(empty3.ndim(), 1);
    EXPECT_EQ(empty3.shape()[0], 0);
}

void test_note_operations() {
    std::cout << "\n=== Testing Note operations with factory-created Notes ===\n";
    
    // Test operations on content-created Notes
    pluss::Note c1 = pluss::content({"hello", "world"});
    pluss::Note c2 = pluss::content({"HELLO", "WORLD"});
    
    EXPECT_EQ(c1.to_upper().vector()[0], "HELLO");
    EXPECT_EQ(c2.to_lower().vector()[1], "world");
    
    // Test operations on make_note-created Notes
    pluss::Note m1 = pluss::make_note({"hello", "world"});
    pluss::Note m2 = pluss::make_note({"HELLO", "WORLD"});
    
    EXPECT_EQ(m1.to_upper().vector()[0], "HELLO");
    EXPECT_EQ(m2.to_lower().vector()[1], "world");
    
    // Test interoperability between content and make_note
    EXPECT_EQ(c1.add(m2).vector()[0], "helloHELLO");
    EXPECT_EQ(m1.add(c2).vector()[1], "worldWORLD");
    
    // Test where operation
    pluss::Note cond = pluss::make_note({"1", "0"});
    pluss::Note result = cond.where(c1, m2);
    EXPECT_EQ(result.vector()[0], "hello");
    EXPECT_EQ(result.vector()[1], "WORLD");
}

int main() {
    std::cout << "=== Note Factory Tests ===\n";
    
    test_content_function();
    test_make_note_function();
    test_note_operations();
    
    std::cout << "\nTest results: " << passed_tests << "/" << total_tests << " passed.\n";
    return passed_tests == total_tests ? 0 : 1;
}
