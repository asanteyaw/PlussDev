#include "pluss/Chord.h"
#include <pluss/pluss.h>
#include <iostream>
#include <chrono>
#include <vector>
// #include <functional>
#include <cmath>

// Performance test for pluss::queue
// This test measures the performance of pluss::queue for various operations

// Helper function to measure execution time
template<typename Func>
double measure_time(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    return duration.count();
}

// Test the performance of pluss::queue with Series
void test_series_queue_performance() {
    std::cout << "Testing Series queue performance..." << std::endl;
    
    // Create large Series for performance testing
    const int size = 10000000; // 10 million elements
    std::vector<double> data(size);
    for (int i = 0; i < size; ++i) {
        data[i] = static_cast<double>(i) / size;
    }
    
    pluss::Series large_series = pluss::value(data);
    
    // Test 1: Simple scalar function (square)
    {
        auto square = [](double x) { return x * x; };
        
        double time = measure_time([&]() {
            pluss::Series result = pluss::queue(square, large_series);
        });
        
        std::cout << "  Square function: " << time << " ms" << std::endl;
    }
    
    // Test 2: Trigonometric function (sin)
    {
        auto sin_func = [](double x) { return std::sin(x); };
        
        double time = measure_time([&]() {
            pluss::Series result = pluss::queue(sin_func, large_series);
        });
        
        std::cout << "  Sin function: " << time << " ms" << std::endl;
    }
    
    // Test 3: Complex function
    {
        auto complex_func = [](double x) { 
            return std::sin(x) * std::cos(x) + std::sqrt(std::abs(x)) + std::log(x + 1.0); 
        };
        
        double time = measure_time([&]() {
            pluss::Series result = pluss::queue(complex_func, large_series);
        });
        
        std::cout << "  Complex function: " << time << " ms" << std::endl;
    }
    
    // Test 4: Binary operation (addition)
    {
        pluss::Series other_series = pluss::value(data);
        auto add = [](double x, double y) { return x + y; };
        
        double time = measure_time([&]() {
            pluss::Series result = pluss::queue(add, large_series, other_series);
        });
        
        std::cout << "  Addition of two Series: " << time << " ms" << std::endl;
    }
    
    // Test 5: Binary operation with scalar
    {
        double scalar = 3.14159;
        auto multiply = [](double x, double y) { return x * y; };
        
        double time = measure_time([&]() {
            pluss::Series result = pluss::queue(multiply, large_series, scalar);
        });
        
        std::cout << "  Multiplication with scalar: " << time << " ms" << std::endl;
    }
    
    // Test 6: Ternary operation
    {
        pluss::Series series2 = pluss::value(data);
        pluss::Series series3 = pluss::value(data);
        auto ternary_op = [](double x, double y, double z) { return x * y + z; };
        
        double time = measure_time([&]() {
            pluss::Series result = pluss::queue(ternary_op, large_series, series2, series3);
        });
        
        std::cout << "  Ternary operation: " << time << " ms" << std::endl;
    }
    
    std::cout << "Series queue performance tests completed!" << std::endl;
}

// Test the performance of pluss::queue with Note
void test_note_queue_performance() {
    std::cout << "Testing Note queue performance..." << std::endl;
    
    // Create large Note for performance testing
    const int size = 100000; // 100,000 elements
    std::vector<pluss::Chord> data;
    data.reserve(size);
    for (int i = 0; i < size; ++i) {
        data.push_back("note_" + std::to_string(i));
    }
    
    pluss::Note large_note = pluss::make_note(data);
    
    // Test 1: Simple string function (to upper)
    {
        auto to_upper = [](const std::string& s) -> std::string {
            std::string result = s;
            for (char& c : result) {
                c = std::toupper(c);
            }
            return result;
        };
        
        double time = measure_time([&]() {
            pluss::Note result = pluss::queue(to_upper, large_note);
        });
        
        std::cout << "  To upper function: " << time << " ms" << std::endl;
    }
    
    // Test 2: String concatenation
    {
        std::string suffix = "_processed";
        auto append = [](const std::string& s, const std::string& suffix) -> std::string {
            return s + suffix;
        };
        
        double time = measure_time([&]() {
            pluss::Note result = pluss::queue(append, large_note, suffix);
        });
        
        std::cout << "  String concatenation: " << time << " ms" << std::endl;
    }
    
    // Test 3: Complex string operation
    {
        auto complex_op = [](const std::string& s) -> std::string {
            std::string result;
            result.reserve(s.size() * 2);
            for (char c : s) {
                if (std::isalpha(c)) {
                    result += std::toupper(c);
                    result += '.';
                } else {
                    result += c;
                }
            }
            return result;
        };
        
        double time = measure_time([&]() {
            pluss::Note result = pluss::queue(complex_op, large_note);
        });
        
        std::cout << "  Complex string operation: " << time << " ms" << std::endl;
    }
    
    std::cout << "Note queue performance tests completed!" << std::endl;
}

// Compare pluss::queue performance with native operations
void compare_with_native_operations() {
    std::cout << "Comparing pluss::queue with native operations..." << std::endl;
    
    // Create large Series for performance testing
    const int size = 10000000; // 10 million elements
    std::vector<double> data(size);
    for (int i = 0; i < size; ++i) {
        data[i] = static_cast<double>(i) / size;
    }
    
    pluss::Series large_series = pluss::value(data);
    pluss::Series other_series = pluss::value(data);
    
    // Test 1: Square operation
    {
        auto square = [](double x) { return x * x; };
        
        // Using pluss::queue
        double queue_time = measure_time([&]() {
            pluss::Series result = pluss::queue(square, large_series);
        });
        
        // Using native operation
        double native_time = measure_time([&]() {
            pluss::Series result = large_series.mul(large_series);
        });
        
        std::cout << "  Square operation:" << std::endl;
        std::cout << "    pluss::queue: " << queue_time << " ms" << std::endl;
        std::cout << "    Native operation: " << native_time << " ms" << std::endl;
        std::cout << "    Speedup: " << (native_time / queue_time) << "x" << std::endl;
    }
    
    // Test 2: Addition operation
    {
        auto add = [](double x, double y) { return x + y; };
        
        // Using pluss::queue
        double queue_time = measure_time([&]() {
            pluss::Series result = pluss::queue(add, large_series, other_series);
        });
        
        // Using native operation
        double native_time = measure_time([&]() {
            pluss::Series result = large_series.add(other_series);
        });
        
        std::cout << "  Addition operation:" << std::endl;
        std::cout << "    pluss::queue: " << queue_time << " ms" << std::endl;
        std::cout << "    Native operation: " << native_time << " ms" << std::endl;
        std::cout << "    Speedup: " << (native_time / queue_time) << "x" << std::endl;
    }
    
    std::cout << "Comparison completed!" << std::endl;
}

int main() {
    std::cout << "Running queue performance tests..." << std::endl;
    
    test_series_queue_performance();
    test_note_queue_performance();
    compare_with_native_operations();
    
    std::cout << "All performance tests completed!" << std::endl;
    return 0;
}
