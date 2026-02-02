#include <torch/torch.h>
#include <iostream>
#include <cmath>

int main() {
    // Test 1: torch::full with std::nan("")
    auto t1 = torch::full({3}, std::nan(""));
    std::cout << "Test 1 - torch::full with std::nan(\"\"):\n";
    std::cout << t1 << "\n";
    std::cout << "dtype: " << t1.dtype() << "\n\n";
    
    // Test 2: torch::full with std::nan("") and explicit double dtype
    auto t2 = torch::full({3}, std::nan(""), torch::TensorOptions().dtype(torch::kFloat64));
    std::cout << "Test 2 - torch::full with std::nan(\"\") and kFloat64:\n";
    std::cout << t2 << "\n";
    std::cout << "dtype: " << t2.dtype() << "\n";
    std::cout << "Is first element NaN? " << (std::isnan(t2[0].item<double>()) ? "YES" : "NO") << "\n\n";
    
    // Test 3: Create tensor then check if NaN persists
    auto t3 = torch::tensor({1.0, 2.0, 3.0});
    auto t4 = torch::full({2}, std::nan(""), t3.options());
    std::cout << "Test 3 - torch::full with options from existing tensor:\n";
    std::cout << "Original tensor dtype: " << t3.dtype() << "\n";
    std::cout << "NaN tensor: " << t4 << "\n";
    std::cout << "NaN tensor dtype: " << t4.dtype() << "\n";
    std::cout << "Is first element NaN? " << (std::isnan(t4[0].item<double>()) ? "YES" : "NO") << "\n\n";
    
    // Test 4: Cat with NaN tensor
    auto combined = torch::cat({t4, t3}, 0);
    std::cout << "Test 4 - Concatenated tensor:\n";
    std::cout << combined << "\n";
    std::cout << "Is first element NaN? " << (std::isnan(combined[0].item<double>()) ? "YES" : "NO") << "\n";
    
    return 0;
}
