#include <pluss/pluss.h>
#include <iostream>
#include <cassert>
#include <cmath>
// #include <functional>

// as you are traversing through lines and building your code, make sure performance is your topmost priority as we want pluss::queue to apply the user defined function to the pluss types with lightening speed!

// Helper function to check if two Series are approximately equal
bool series_approx_equal(const pluss::Series& a, const pluss::Series& b, double tolerance = 1e-5) {
    if (a.sizes() != b.sizes()) return false;

    pluss::Series diff = a.sub(b).abs();
    pluss::Series flat = diff.view(-1);

    double max_diff = 0.0;
    for (int64_t i = 0; i < flat.numel(); ++i) {
        max_diff = std::max(max_diff, flat[i].template item<double>());
    }
    pluss::Series a1 = pluss::value(2.0);
    pluss::Series a2 = pluss::value(2.1);
    pluss::Series a3 = (a1 - a2).abs();

    return max_diff < tolerance;
}

// Helper function to check if two Notes are equal
bool notes_equal(const pluss::Note& a, const pluss::Note& b) {
    auto a_vec = a.vector();
    auto b_vec = b.vector();
    
    if (a_vec.size() != b_vec.size()) {
        return false;
    }
    
    for (size_t i = 0; i < a_vec.size(); ++i) {
        if (a_vec[i].str() != b_vec[i].str()) {
            return false;
        }
    }
    
    return true;
}

// Test queue with Series
void test_series_queue() {
    std::cout << "Testing Series queue..." << std::endl;
    
    // Test 1: Simple scalar function on a 1D Series
    {
        torch::Tensor k = torch::tensor({1.0, 2.0, 3.0, 4.0, 5.0});
        pluss::Series s = pluss::value({1.0, 2.0, 3.0, 4.0, 5.0}, pluss::config({"dtype", "float64"}));
        auto square = [](double x) { return x * x; };
        std::cout << "Input dtype: " << s.dtype() << std::endl;
        pluss::Series result = pluss::queue(square, s);
        pluss::Series expected = pluss::value({1.0, 4.0, 9.0, 16.0, 25.0});
        // std::cout << result << "\n";
        assert(series_approx_equal(result, expected));
        std::cout << "  Test 1 passed: Simple scalar function on 1D Series" << std::endl;
    }
    
    // Test 2: Function with multiple Series arguments
    {
        pluss::Series a = pluss::value({1.0, 2.0, 3.0, 4.0, 5.0});
        pluss::Series b = pluss::value({5.0, 4.0, 3.0, 2.0, 1.0});
        auto add = [](double x, double y) { return x + y; };
        
        pluss::Series result = pluss::queue(add, a, b);
        pluss::Series expected = pluss::value({6.0, 6.0, 6.0, 6.0, 6.0});
        
        assert(series_approx_equal(result, expected));
        std::cout << "  Test 2 passed: Function with multiple Series arguments" << std::endl;
    }
    
    // Test 3: Function with Series and scalar arguments
    {
        pluss::Series a = pluss::value({1.0, 2.0, 3.0, 4.0, 5.0});
        double scalar = 10.0;
        auto multiply = [](double x, double y) { return x * y; };
        
        pluss::Series result = pluss::queue(multiply, a, scalar);
        pluss::Series expected = pluss::value({10.0, 20.0, 30.0, 40.0, 50.0});
        
        assert(series_approx_equal(result, expected));
        std::cout << "  Test 3 passed: Function with Series and scalar arguments" << std::endl;
    }
    
    // Test 4: Function with 2D Series
    {
        pluss::Series a = pluss::value({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
        auto square = [](double x) { return x * x; };
        
        pluss::Series result = pluss::queue(square, a);
        pluss::Series expected = pluss::value({{1.0, 4.0, 9.0}, {16.0, 25.0, 36.0}});
        
        assert(series_approx_equal(result, expected));
        std::cout << "  Test 4 passed: Function with 2D Series" << std::endl;
    }
    
    // Test 5: Complex function
    {
        pluss::Series a = pluss::value({1.0, 2.0, 3.0, 4.0, 5.0});
        auto complex_func = [](double x) { return std::sin(x) + std::cos(x); };
        
        pluss::Series result = pluss::queue(complex_func, a);
        pluss::Series expected = pluss::empty({5});
        for (int i = 0; i < 5; ++i) {
            double val = i + 1.0;
            expected[i] = pluss::value(std::sin(val) + std::cos(val));
        }
        
        assert(series_approx_equal(result, expected));
        std::cout << "  Test 5 passed: Complex function" << std::endl;
    }
    
    std::cout << "All Series queue tests passed!" << std::endl;
}

// Test queue with Note
void test_note_queue() {
    std::cout << "Testing Note queue..." << std::endl;
    
    // Test 1: Simple string function on a 1D Note
    {
        pluss::Note n = pluss::make_note({"hello", "world", "test", "note"});
        auto to_upper = [](const std::string& s) -> std::string {
            std::string result = s;
            for (char& c : result) {
                c = std::toupper(c);
            }
            return result;
        };
        
        pluss::Note result = pluss::queue(to_upper, n);
        pluss::Note expected = pluss::make_note({"HELLO", "WORLD", "TEST", "NOTE"});
        
        assert(notes_equal(result, expected));
        std::cout << "  Test 1 passed: Simple string function on 1D Note" << std::endl;
    }
    
    // Test 2: Function with multiple Note arguments
    {
        pluss::Note a = pluss::make_note({"hello", "world"});
        pluss::Note b = pluss::make_note({"test", "note"});
        auto concat = [](const std::string& x, const std::string& y) -> std::string {
            return x + "-" + y;
        };
        
        pluss::Note result = pluss::queue(concat, a, b);
        pluss::Note expected = pluss::make_note({"hello-test", "world-note"});
        
        assert(notes_equal(result, expected));
        std::cout << "  Test 2 passed: Function with multiple Note arguments" << std::endl;
    }
    
    // Test 3: Function with Note and scalar arguments
    {
        pluss::Note a = pluss::make_note({"hello", "world"});
        std::string suffix = "!";
        auto append = [](const std::string& x, const std::string& y) -> std::string {
            return x + y;
        };
        
        pluss::Note result = pluss::queue(append, a, suffix);
        pluss::Note expected = pluss::make_note({"hello!", "world!"});
        
        assert(notes_equal(result, expected));
        std::cout << "  Test 3 passed: Function with Note and scalar arguments" << std::endl;
    }
    
    // Test 4: Complex string function
    {
        pluss::Note n = pluss::make_note({"hello", "world", "test", "note"});
        auto complex_func = [](const std::string& s) -> std::string {
            std::string result;
            for (char c : s) {
                if (std::isalpha(c)) {
                    result += std::toupper(c);
                    result += '.';
                }
            }
            return result;
        };
        
        pluss::Note result = pluss::queue(complex_func, n);
        pluss::Note expected = pluss::make_note({"H.E.L.L.O.", "W.O.R.L.D.", "T.E.S.T.", "N.O.T.E."});
        
        assert(notes_equal(result, expected));
        std::cout << "  Test 4 passed: Complex string function" << std::endl;
    }
    
    std::cout << "All Note queue tests passed!" << std::endl;
}

int main() {
    std::cout << "Running queue tests..." << std::endl;
    
    test_series_queue();
    test_note_queue();
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
