#include <pluss/datalog/table/thread/thread.h>
#include <pluss/series_factory.h>
#include <iostream>

using namespace pluss::table;

int main() {
    // Test Series add directly first
    std::cout << "Testing Series add directly:\n";
    auto s1 = pluss::from_torch(torch::tensor({10.0, 20.0, 30.0}));
    auto s2 = pluss::from_torch(torch::tensor({2.0, 3.0, 4.0}));
    
    std::cout << "s1: " << s1[0].item<double>() << ", " << s1[1].item<double>() << ", " << s1[2].item<double>() << "\n";
    std::cout << "s2: " << s2[0].item<double>() << ", " << s2[1].item<double>() << ", " << s2[2].item<double>() << "\n";
    
    // Test with operator+
    auto s_plus = s1 + s2;
    std::cout << "s1 + s2 (operator+): " << s_plus[0].item<double>() << ", " << s_plus[1].item<double>() << ", " << s_plus[2].item<double>() << "\n";
    
    // Test with add method
    auto s_add = s1.add(s2);
    std::cout << "s1.add(s2): " << s_add[0].item<double>() << ", " << s_add[1].item<double>() << ", " << s_add[2].item<double>() << "\n";
    
    // Test with explicit alpha = 1.0
    auto s_add_alpha = s1.add(s2, 1.0);
    std::cout << "s1.add(s2, 1.0): " << s_add_alpha[0].item<double>() << ", " << s_add_alpha[1].item<double>() << ", " << s_add_alpha[2].item<double>() << "\n";
    
    // Test calling tensor() method
    auto t_add = s1.tensor().add(s2.tensor());
    std::cout << "s1.tensor().add(s2.tensor()): " << t_add[0].item<double>() << ", " << t_add[1].item<double>() << ", " << t_add[2].item<double>() << "\n";
    
    // Test calling tensor().add with alpha
    auto t_add_alpha = s1.tensor().add(s2.tensor(), 1.0);
    std::cout << "s1.tensor().add(s2.tensor(), 1.0): " << t_add_alpha[0].item<double>() << ", " << t_add_alpha[1].item<double>() << ", " << t_add_alpha[2].item<double>() << "\n";
    
    std::cout << "Expected: 12, 23, 34\n\n";
    
    // Test Thread add
    std::cout << "Testing Thread add operation:\n";
    auto data1 = pluss::from_torch(torch::tensor({10.0, 20.0, 30.0}));
    auto data2 = pluss::from_torch(torch::tensor({2.0, 3.0, 4.0}));
    Thread t1(data1, "t1");
    Thread t2(data2, "t2");
    
    std::cout << "t1 is_series: " << t1.is_series() << "\n";
    std::cout << "t2 is_series: " << t2.is_series() << "\n";
    
    std::cout << "t1.as_series(): " << t1.as_series()[0].item<double>() << "\n";
    std::cout << "t2.as_series(): " << t2.as_series()[0].item<double>() << "\n";
    
    auto add_result = t1.add(t2);
    std::cout << "add_result is_series: " << add_result.is_series() << "\n";
    std::cout << "add_result size: " << add_result.size() << "\n";
    
    if (add_result.is_series()) {
        const auto& series = add_result.as_series();
        std::cout << "series numel: " << series.numel() << "\n";
        std::cout << "series dtype: " << series.dtype() << "\n";
        std::cout << "First element: " << series[0].item<double>() << "\n";
        std::cout << "Expected: 12.0\n";
    }
    
    // Test round
    std::cout << "\nTesting round operation:\n";
    auto decimal_data = pluss::from_torch(torch::tensor({1.234, 2.567, 3.891}));
    Thread t_dec(decimal_data, "decimal");
    
    std::cout << "t_dec is_series: " << t_dec.is_series() << "\n";
    
    auto rounded = t_dec.round(1);
    std::cout << "rounded is_series: " << rounded.is_series() << "\n";
    
    if (rounded.is_series()) {
        const auto& series = rounded.as_series();
        std::cout << "rounded series numel: " << series.numel() << "\n";
        std::cout << "First element: " << series[0].item<double>() << "\n";
        std::cout << "Expected: ~1.2\n";
    }
    
    return 0;
}
