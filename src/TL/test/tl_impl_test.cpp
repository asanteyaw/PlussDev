#include <iostream>
#include <pluss/pluss.h>

using namespace pluss;

int main() {
    // Create a simple Series
    at::Tensor tensor = at::ones({2, 3});
    Series series(tensor);

    Series s1 = pluss::from_torch(tensor);
    Series s2 = pluss::value(2.0);
    std::cout << "S1:\n " << s1 << "\n";
    std::cout << "S2:\n " << s2 << "\n";
    Series a = pluss::value({1.2, 2.2, 3.2});
    Series b = pluss::value({0.0, 0.0});
    std::cout << a << "\n";
    std::cout << b << "\n";
    Series c = a.div(b); // or a.div_(b);
    std::cout << c << "\n";
    
    
    // Test some in-place operations
    series.add_(1.0);
    std::cout << "After add_(1.0): " << series.tensor() << std::endl;
    
    series.mul_(2.0);
    std::cout << "After mul_(2.0): " << series.tensor() << std::endl;
    
    // Test some out-of-place operations
    Series result = series.add(1.0);
    std::cout << "Original after add(1.0): " << series.tensor() << std::endl;
    std::cout << "Result of add(1.0): " << result.tensor() << std::endl;
    
    Series squared = series.pow(2.0);
    std::cout << "Original after pow(2.0): " << series.tensor() << std::endl;
    std::cout << "Result of pow(2.0): " << squared.tensor() << std::endl;
    
    // Test operations with other Series
    Series other = Series(at::ones({2, 3}));
    Series sum = series.add(other);
    std::cout << "Result of add(other): " << sum.tensor() << std::endl;
    
    series.add_(other);
    std::cout << "After add_(other): " << series.tensor() << std::endl;
    
    return 0;
}
