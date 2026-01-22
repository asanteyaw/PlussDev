#include <iostream>
#include <pluss/pluss.h>

using namespace pluss;

int main() {
    // Test 0D (scalar) Series
    {
        // Integer scalar
        Series s0d_int = pluss::value(42);
        std::cout << "0D Integer Series:\n" << s0d_int << "\n\n";
        
        // Float scalar
        Series s0d_float = pluss::value(3.14159f);
        std::cout << "0D Float Series:\n" << s0d_float << "\n\n";
        
        // Double scalar with many decimal places
        Series s0d_double = pluss::value(3.14159265358979);
        std::cout << "0D Double Series:\n" << s0d_double << "\n\n";
    }
    
    // Test 1D Series
    {
        // Small integer vector
        Series s1d_small = pluss::value({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
        std::cout << "1D Small Integer Series:\n" << s1d_small << "\n\n";
        
        // Large integer vector (more than 10 elements)
        Series s1d_large = pluss::arange(0, 20);
        std::cout << "1D Large Integer Series:\n" << s1d_large << "\n\n";
        
        // Float vector
        Series s1d_float = pluss::value({0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f});
        std::cout << "1D Float Series:\n" << s1d_float << "\n\n";
        
        // Float vector with many decimal places
        Series s1d_float_long = pluss::value({0.123456789f, 1.123456789f, 2.123456789f});
        std::cout << "1D Float Series with long decimals:\n" << s1d_float_long << "\n\n";
    }
    
    // Test 2D Series
    {
        // Small 2D matrix
        Series s2d_small = pluss::ones({3, 3});
        std::cout << "2D Small Matrix Series:\n" << s2d_small << "\n\n";
        
        // Create a 2D matrix with custom values
        at::Tensor custom_tensor = at::ones({3, 3});
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                custom_tensor[i][j] = i * 3 + j;
            }
        }
        Series s2d_custom = pluss::from_torch(custom_tensor);
        std::cout << "2D Custom Matrix Series:\n" << s2d_custom << "\n\n";
        std::cout << custom_tensor.sizes() << "\n";
        std::cout << s2d_custom.sizes()[0] << "\n";
        
        // Large 2D matrix
        Series s2d_large = pluss::ones({12, 12});
        std::cout << "2D Large Matrix Series:\n" << s2d_large << "\n\n";
    }
    
    // Test 3D Series
    {
        Series s3d = pluss::ones({3, 3, 3});
        std::cout << "3D Series:\n" << s3d << "\n\n";
        
    }
    
    // Test 4D Series
    {
        Series s4d = pluss::ones({2, 2, 2, 2});
        std::cout << "4D Series:\n" << s4d << "\n\n";
    }
    
    // Test different data types
    {
        Series s_int8 = pluss::ones({2, 2}, config({"dtype", "int8"}));
        std::cout << "Int8 Series:\n" << s_int8 << "\n\n";
        
        Series s_int16 = pluss::ones({2, 2}, config({"dtype", "int16"}));
        std::cout << "Int16 Series:\n" << s_int16 << "\n\n";
        
        Series s_int32 = pluss::ones({2, 2}, config({"dtype", "int32"}));
        std::cout << "Int32 Series:\n" << s_int32 << "\n\n";
        
        Series s_int64 = pluss::ones({2, 2}, config({"dtype", "int64"}));
        std::cout << "Int64 Series:\n" << s_int64 << "\n\n";
        
        Series s_float32 = pluss::ones({2, 2}, config({"dtype", "float32"}));
        std::cout << "Float32 Series:\n" << s_float32 << "\n\n";
        
        Series s_float64 = pluss::ones({2, 2}, config({"dtype", "float64"}));
        std::cout << "Float64 Series:\n" << s_float64 << "\n\n";
    }
    
    return 0;
}
