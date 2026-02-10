#include <cstddef>
#include <exception>
#include <iostream>

#include <torch/torch.h>

#include <pluss/pluss.h>

using namespace pluss;

static void print_tensor_shape(const at::Series& series) {
    auto sizes = series.sizes();
    std::cout << "shape=[";
    for (size_t i = 0; i < static_cast<size_t>(sizes.size()); ++i) {
        if (i) std::cout << ", ";
        std::cout << sizes[i];
    }
    std::cout << "]";
}

int main() {
    // Create a simple Series
    at::Series series = at::ones({2, 3});
    Series series = pluss::from_torch(series);

    Series s1 = pluss::from_torch(series);
    Series s2 = pluss::value(2.0);
    std::cout << "S1:\n " << s1 << "\n";
    std::cout << "S2:\n " << s2 << "\n";
    Series a = pluss::value({1.2, 2.2, 3.2});
    Series b = pluss::value({0.0, 0.0});
    std::cout << a << "\n";
    std::cout << b << "\n";
    // This is intentionally invalid (shape mismatch) and should raise a PlussError.
    bool caught = false;
    try {
        Series c = a.div(b); // or a.div_(b);
        std::cout << c << "\n";
    } catch (const std::exception& e) {
        caught = true;
        std::cout << "Caught expected exception: " << e.what() << "\n";
    }
    if (!caught) {
        std::cerr << "Expected a shape-mismatch exception, but none was thrown.\n";
        return 1;
    }
    
    
    // Test some in-place operations
    series.add_(1.0);
    std::cout << "After add_(1.0): ";
    print_tensor_shape(series.series());
    std::cout << std::endl;
    
    series.mul_(2.0);
    std::cout << "After mul_(2.0): ";
    print_tensor_shape(series.series());
    std::cout << std::endl;
    
    // Test some out-of-place operations
    Series result = series.add(1.0);
    std::cout << "Original after add(1.0): ";
    print_tensor_shape(series.series());
    std::cout << std::endl;
    std::cout << "Result of add(1.0): ";
    print_tensor_shape(result.series());
    std::cout << std::endl;
    
    Series squared = series.pow(2.0);
    std::cout << "Original after pow(2.0): ";
    print_tensor_shape(series.series());
    std::cout << std::endl;
    std::cout << "Result of pow(2.0): ";
    print_tensor_shape(squared.series());
    std::cout << std::endl;
    
    // Test operations with other Series
    Series other = pluss::from_torch(at::ones({2, 3}));
    Series sum = series.add(other);
    std::cout << "Result of add(other): ";
    print_tensor_shape(sum.series());
    std::cout << std::endl;
    
    series.add_(other);
    std::cout << "After add_(other): ";
    print_tensor_shape(series.series());
    std::cout << std::endl;
    
    return 0;
}
