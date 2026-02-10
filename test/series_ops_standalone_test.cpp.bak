#include <iostream>
#include <torch/torch.h>

// Include ONLY SeriesBody.h (high-level API) - no other PlussMa headers
#include <pluss/pluss.h>

using namespace pluss;

// Test counter
int tests_passed = 0;
int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            tests_passed++; \
            std::cout << "✓ " << message << std::endl; \
        } else { \
            tests_failed++; \
            std::cerr << "✗ FAILED: " << message << std::endl; \
        } \
    } while(0)

// Helper function to create Series from series
Series make_series(at::Series t) {
    return pluss::from_torch(std::move(t));
}

void test_basic_operations() {
    std::cout << "\n=== Testing Basic Operations ===\n";
    
    auto s = make_series(torch::series({1.0, 2.0, 3.0, 4.0}));
    
    double sum_val = s.sum().series().item<double>();
    TEST_ASSERT(std::abs(sum_val - 10.0) < 1e-6, "sum");
    
    double mean_val = s.mean().series().item<double>();
    TEST_ASSERT(std::abs(mean_val - 2.5) < 1e-6, "mean");
    
    double max_val = s.max().series().item<double>();
    TEST_ASSERT(std::abs(max_val - 4.0) < 1e-6, "max");
    
    double min_val = s.min().series().item<double>();
    TEST_ASSERT(std::abs(min_val - 1.0) < 1e-6, "min");
}

void test_scalar_operations() {
    std::cout << "\n=== Testing Scalar Operations ===\n";
    
    auto s = make_series(torch::series({2.0, 4.0, 6.0}));
    
    // pow_Scalar
    auto s_pow = s.pow(2.0);
    double first_val = s_pow.series()[0].item<double>();
    TEST_ASSERT(std::abs(first_val - 4.0) < 1e-6, "pow_Scalar");
    
    // float_power_Scalar  
    auto s_fp = s.float_power(0.5);
    double fp_val = s_fp.series()[0].item<double>();
    TEST_ASSERT(std::abs(fp_val - std::sqrt(2.0)) < 1e-6, "float_power_Scalar");
}

void test_tensor_operations() {
    std::cout << "\n=== Testing Series Operations ===\n";
    
    auto s1 = make_series(torch::series({1.0, 2.0, 3.0}));
    auto s2 = make_series(torch::series({3.0, 2.0, 1.0}));
    
    // maximum_other
    auto s_max = s1.maximum(s2);
    double max_val = s_max.series()[0].item<double>();
    TEST_ASSERT(std::abs(max_val - 3.0) < 1e-6, "maximum_other");
    
    // minimum_other
    auto s_min = s1.minimum(s2);
    double min_val = s_min.series()[0].item<double>();
    TEST_ASSERT(std::abs(min_val - 1.0) < 1e-6, "minimum_other");
}

void test_comparisons() {
    std::cout << "\n=== Testing Comparison Operations ===\n";
    
    auto s = make_series(torch::series({1.0, 2.0, 3.0}));
    
    // ge_Scalar
    s.ge_(2.0);
    bool ge_check = s.series()[1].item<bool>();
    TEST_ASSERT(ge_check, "ge_Scalar");
    
    // Reset for next test
    s = make_series(torch::series({1.0, 2.0, 3.0}));
    
    // gt_Scalar
    s.gt_(2.0);
    bool gt_check = s.series()[2].item<bool>();
    TEST_ASSERT(gt_check, "gt_Scalar");
}

void test_reshape_operations() {
    std::cout << "\n=== Testing Reshape Operations ===\n";
    
    auto s = make_series(torch::arange(12).to(torch::kDouble));
    
    // reshape
    auto s_reshaped = s.reshape({3, 4});
    auto sizes = s_reshaped.sizes();
    TEST_ASSERT(sizes[0] == 3 && sizes[1] == 4, "reshape");
    
    // transpose
    auto s_t = s_reshaped.transpose(0, 1);
    auto t_sizes = s_t.sizes();
    TEST_ASSERT(t_sizes[0] == 4 && t_sizes[1] == 3, "transpose");
}

void test_clone_operations() {
    std::cout << "\n=== Testing Clone Operations ===\n";
    
    auto s = make_series(torch::series({1.0, 2.0, 3.0}));
    
    // clone
    auto s_clone = s.clone();
    double clone_val = s_clone.series()[0].item<double>();
    TEST_ASSERT(std::abs(clone_val - 1.0) < 1e-6, "clone");
    
    // contiguous
    auto s_cont = s.contiguous();
    TEST_ASSERT(s_cont.is_contiguous(), "contiguous");
}

int main() {
    std::cout << "Starting SeriesOps.h Standalone Tests\n";
    std::cout << "=====================================\n";
    
    try {
        test_basic_operations();
        test_scalar_operations();
        test_tensor_operations();
        test_comparisons();
        test_reshape_operations();
        test_clone_operations();
        
        std::cout << "\n=====================================\n";
        std::cout << "Test Results:\n";
        std::cout << "  Passed: " << tests_passed << "\n";
        std::cout << "  Failed: " << tests_failed << "\n";
        std::cout << "=====================================\n";
        
        return (tests_failed == 0) ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
