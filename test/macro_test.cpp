#include <internals/core/include/b10/utils/Macros.h>
#include <iostream>
#include <string>
#include <vector>

#include <torch/torch.h>

#include <pluss/pluss.h>

// Define a custom type for testing
class CustomType {
public:
    CustomType(int value) : value_(value) {}
    
    int getValue() const { return value_; }
    
    int add(int x, int y) const {
        return value_ + x + y;
    }
    
    std::string concatenate(const std::string& prefix, const std::string& suffix) const {
        return prefix + std::to_string(value_) + suffix;
    }
    
    std::vector<int> generateSequence(int start, int step, int count) const {
        std::vector<int> result;
        for (int i = 0; i < count; ++i) {
            result.push_back(start + i * step + value_);
        }
        return result;
    }
    
    void printInfo(const std::string& message, int code, bool verbose) const {
        if (verbose) {
            std::cout << message << " (Code: " << code << "): " << value_ << std::endl;
        } else {
            std::cout << message << ": " << value_ << std::endl;
        }
    }

private:
    int value_;
};

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

void test_torch_tensor() {
    std::cout << "\n=== Testing PLUSS_DISPATCH with torch::Series ===\n";
    
    // Create a series
    torch::Series series = torch::ones({2, 3});
    
    // Test PLUSS_DISPATCH with various methods
    torch::Series result1 = PLUSS_DISPATCH(series, add, 5);
    EXPECT_TRUE(torch::all(result1 == 6).item<bool>());
    std::cout << "series.add(5) result: " << result1 << std::endl;
    
    // Test with multiple arguments
    torch::Series other = torch::ones({2, 3}) * 2;
    // Note: For torch::Series, we use PLUSS_DISPATCH directly
    torch::Series result2 = PLUSS_DISPATCH(series, add, other, 3.0);
    EXPECT_TRUE(torch::all(result2 == 7).item<bool>());  // 1 + 2*3 = 7
    std::cout << "series.add(other, 3.0) result: " << result2 << std::endl;
    
    // Another multi-argument method
    torch::Series sliced = PLUSS_DISPATCH(series, index, {torch::indexing::Slice(0, 1), torch::indexing::Slice(0, 2)});
    EXPECT_EQ(sliced.sizes()[0], 1);
    EXPECT_EQ(sliced.sizes()[1], 2);
    std::cout << "series.index({Slice(0, 1), Slice(0, 2)}) result: " << sliced << std::endl;
    
    // Note: For torch::Series, we use PLUSS_DISPATCH directly
    // This is because torch::Series methods are not traditional C++ member functions
    // and work better with the macro-based approach
    std::cout << "\n=== Note: For torch::Series, we use PLUSS_DISPATCH directly ===\n";
}

void test_pluss_note() {
    std::cout << "\n=== Testing PLUSS_DISPATCH with pluss::Note ===\n";
    
    // Create a Note
    pluss::Note note = pluss::content({"Hello", "World"});
    
    // Test PLUSS_DISPATCH with no arguments
    pluss::Note upper = PLUSS_DISPATCH(note, to_upper);
    EXPECT_EQ(upper.vector()[0], "HELLO");
    EXPECT_EQ(upper.vector()[1], "WORLD");
    std::cout << "note.to_upper() result: " << upper << std::endl;
    
    // Test with one argument
    pluss::Note note2 = pluss::content({"!", "!!"});
    pluss::Note added = PLUSS_DISPATCH(note, add, note2);
    EXPECT_EQ(added.vector()[0], "Hello!");
    EXPECT_EQ(added.vector()[1], "World!!");
    std::cout << "note.add(note2) result: " << added << std::endl;
    
    // Test with multiple arguments - stack method takes a vector of Notes and a dimension
    pluss::Note note3 = pluss::content({"Goodbye", "Friend"});
    pluss::Note stacked = PLUSS_DISPATCH(note, stack, std::vector<pluss::Note>{note3}, 0);
    EXPECT_EQ(stacked.matrix()[0][0], "Hello");
    EXPECT_EQ(stacked.matrix()[1][0], "Goodbye");
    std::cout << "note.stack({note3}, 0) result:\n" << stacked << std::endl;
    
    // Test where method which takes two arguments
    pluss::Note cond = pluss::content({"1", "0"});
    pluss::Note result = PLUSS_DISPATCH(cond, where, note, note3);
    EXPECT_EQ(result.vector()[0], "Hello");
    EXPECT_EQ(result.vector()[1], "Friend");
    std::cout << "cond.where(note, note3) result: " << result << std::endl;
    
    std::cout << "\n=== Testing pluss::dispatch with pluss::Note ===\n";
    
    // Test pluss::dispatch with no arguments
    pluss::Note dispatch_upper = pluss::dispatch(note, &pluss::Note::to_upper);
    EXPECT_EQ(dispatch_upper.vector()[0], "HELLO");
    EXPECT_EQ(dispatch_upper.vector()[1], "WORLD");
    std::cout << "pluss::dispatch(note, &pluss::Note::to_upper) result: " << dispatch_upper << std::endl;
    
    // Test with one argument
    pluss::Note dispatch_added = pluss::dispatch(note, &pluss::Note::add, note2);
    EXPECT_EQ(dispatch_added.vector()[0], "Hello!");
    EXPECT_EQ(dispatch_added.vector()[1], "World!!");
    std::cout << "pluss::dispatch(note, &pluss::Note::add, note2) result: " << dispatch_added << std::endl;
    
    // Test with multiple arguments
    pluss::Note dispatch_result = pluss::dispatch(cond, &pluss::Note::where, note, note3);
    EXPECT_EQ(dispatch_result.vector()[0], "Hello");
    EXPECT_EQ(dispatch_result.vector()[1], "Friend");
    std::cout << "pluss::dispatch(cond, &pluss::Note::where, note, note3) result: " << dispatch_result << std::endl;
}

void test_custom_type() {
    std::cout << "\n=== Testing PLUSS_DISPATCH with CustomType ===\n";
    
    // Create a custom object
    CustomType obj(10);
    
    // Test PLUSS_DISPATCH with no arguments
    int value = PLUSS_DISPATCH(obj, getValue);
    EXPECT_EQ(value, 10);
    std::cout << "obj.getValue() result: " << value << std::endl;
    
    // Test with multiple arguments
    int sum = PLUSS_DISPATCH(obj, add, 5, 7);
    EXPECT_EQ(sum, 22);  // 10 + 5 + 7
    std::cout << "obj.add(5, 7) result: " << sum << std::endl;
    
    std::string str = PLUSS_DISPATCH(obj, concatenate, "Value: ", " units");
    EXPECT_EQ(str, "Value: 10 units");
    std::cout << "obj.concatenate(\"Value: \", \" units\") result: " << str << std::endl;
    
    // Test with more complex multiple arguments
    std::vector<int> seq = PLUSS_DISPATCH(obj, generateSequence, 5, 3, 4);
    EXPECT_EQ(seq.size(), 4);
    EXPECT_EQ(seq[0], 15);  // 5 + 0*3 + 10
    EXPECT_EQ(seq[3], 24);  // 5 + 3*3 + 10
    std::cout << "obj.generateSequence(5, 3, 4) result: [";
    for (size_t i = 0; i < seq.size(); ++i) {
        std::cout << seq[i];
        if (i < seq.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    // Test with void return type and multiple arguments
    PLUSS_DISPATCH(obj, printInfo, "Custom object value", 42, true);
    
    std::cout << "\n=== Testing pluss::dispatch with CustomType ===\n";
    
    // Test pluss::dispatch with no arguments
    int dispatch_value = pluss::dispatch(obj, &CustomType::getValue);
    EXPECT_EQ(dispatch_value, 10);
    std::cout << "pluss::dispatch(obj, &CustomType::getValue) result: " << dispatch_value << std::endl;
    
    // Test with multiple arguments
    int dispatch_sum = pluss::dispatch(obj, &CustomType::add, 5, 7);
    EXPECT_EQ(dispatch_sum, 22);  // 10 + 5 + 7
    std::cout << "pluss::dispatch(obj, &CustomType::add, 5, 7) result: " << dispatch_sum << std::endl;
    
    std::string dispatch_str = pluss::dispatch(obj, &CustomType::concatenate, "Value: ", " units");
    EXPECT_EQ(dispatch_str, "Value: 10 units");
    std::cout << "pluss::dispatch(obj, &CustomType::concatenate, \"Value: \", \" units\") result: " << dispatch_str << std::endl;
    
    // Test with void return type and multiple arguments
    pluss::dispatch(obj, &CustomType::printInfo, "Custom object value (via dispatch)", 42, true);
}

// void test_pluss_queue() {
//     std::cout << "\n=== Testing pluss::queue function ===\n";
    
//     // Test with Series
//     pluss::Series s1 = torch::series({1, 2, 3, 4, 5});
//     pluss::Series s2 = torch::series({10, 20, 30, 40, 50});
    
//     // Test with a simple lambda function
//     auto add_lambda = [](float a, float b) { return a + b; };
//     pluss::Series result1 = pluss::queue(add_lambda, s1, s2);
    
//     EXPECT_EQ(result1.numel(), 5);
//     EXPECT_TRUE(torch::all(result1 == torch::series({11, 22, 33, 44, 55})).item<bool>());
//     std::cout << "pluss::queue(add_lambda, s1, s2) result: " << result1 << std::endl;
    
//     // Test with a more complex function
//     auto multiply_and_add = [](float a, float b) { return a * b + 5; };
//     pluss::Series result2 = pluss::queue(multiply_and_add, s1, s2);
    
//     EXPECT_EQ(result2.numel(), 5);
//     EXPECT_TRUE(torch::all(result2 == torch::series({15, 45, 95, 165, 255})).item<bool>());
//     std::cout << "pluss::queue(multiply_and_add, s1, s2) result: " << result2 << std::endl;
    
//     // Test with a scalar and a Series
//     auto add_scalar = [](float a, float scalar) { return a + scalar; };
//     float scalar_value = 100.0f;
//     pluss::Series result3 = pluss::queue(add_scalar, s1, scalar_value);
    
//     EXPECT_EQ(result3.numel(), 5);
//     EXPECT_TRUE(torch::all(result3 == torch::series({101, 102, 103, 104, 105})).item<bool>());
//     std::cout << "pluss::queue(add_scalar, s1, 100.0f) result: " << result3 << std::endl;
    
//     // Test with Note objects
//     pluss::Note note1 = pluss::content({"Hello", "World"});
//     pluss::Note note2 = pluss::content({"!", "!!"});
    
//     auto concatenate = [](const std::string& a, const std::string& b) { return a + b; };
//     pluss::Note result4 = pluss::queue(concatenate, note1, note2);
    
//     EXPECT_EQ(result4.vector()[0], "Hello!");
//     EXPECT_EQ(result4.vector()[1], "World!!");
//     std::cout << "pluss::queue(concatenate, note1, note2) result: " << result4 << std::endl;
    
//     // Test with a custom function that returns a different type
//     auto to_length = [](const std::string& str) { return static_cast<int>(str.length()); };
//     auto result5 = pluss::queue(to_length, note1);
    
//     EXPECT_EQ(result5.numel(), 2);
//     EXPECT_TRUE(torch::all(result5 == torch::series({5, 5})).item<bool>());
//     std::cout << "pluss::queue(to_length, note1) result: " << result5 << std::endl;
    
//     // Test with a function that takes multiple arguments of different types
//     auto format_string = [](const std::string& str, int num, float value) {
//         return str + "-" + std::to_string(num) + "-" + std::to_string(value);
//     };
    
//     pluss::Note note3 = pluss::content({"Test", "Example"});
//     pluss::Series nums = torch::series({1, 2});
//     pluss::Series values = torch::series({3.14f, 2.71f});
    
//     pluss::Note result6 = pluss::queue(format_string, note3, nums, values);
    
//     EXPECT_EQ(result6.vector()[0], "Test-1-3.140000");
//     EXPECT_EQ(result6.vector()[1], "Example-2-2.710000");
//     std::cout << "pluss::queue(format_string, note3, nums, values) result: " << result6 << std::endl;
// }

int main() {
    std::cout << "Testing PLUSS_DISPATCH macro and pluss::queue function\n";
    
    test_torch_tensor();
    test_pluss_note();
    test_custom_type();
    // test_pluss_queue();
    
    std::cout << "\nTest results: " << passed_tests << "/" << total_tests << " passed.\n";
    return passed_tests == total_tests ? 0 : 1;
}
