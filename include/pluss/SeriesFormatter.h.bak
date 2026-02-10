#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <PlussQit/core/SeriesBody.h>
#include <MSharp/core/ScalarType.h>
#include "SeriesBody.h"

namespace pluss {

class SeriesFormatter {
public:
    // Convert at::ScalarType to a string representation
    static std::string dtype_to_string(at::ScalarType dtype) {
        switch (dtype) {
            case at::ScalarType::Byte: return "uint8";
            case at::ScalarType::Char: return "int8";
            case at::ScalarType::Short: return "int16";
            case at::ScalarType::Int: return "int32";
            case at::ScalarType::Long: return "int64";
            case at::ScalarType::Half: return "float16";
            case at::ScalarType::Float: return "float32";
            case at::ScalarType::Double: return "float64";
            case at::ScalarType::ComplexHalf: return "complex32";
            case at::ScalarType::ComplexFloat: return "complex64";
            case at::ScalarType::ComplexDouble: return "complex128";
            case at::ScalarType::Bool: return "bool";
            case at::ScalarType::QInt8: return "qint8";
            case at::ScalarType::QUInt8: return "quint8";
            case at::ScalarType::QInt32: return "qint32";
            case at::ScalarType::BFloat16: return "bfloat16";
            default: return "unknown";
        }
    }

    // Format a scalar value
    static std::string format_scalar(const at::Series& t) {
        if (t.numel() != 1) {
            return "Error: Not a scalar";
        }

        std::ostringstream oss;
        
        // Handle different scalar types
        switch (t.scalar_type()) {
            case at::ScalarType::Bool:
                oss << (t.item<bool>() ? "True" : "False");
                break;
            case at::ScalarType::Byte:
                oss << static_cast<int>(t.item<uint8_t>()); // Cast to int to avoid printing as char
                break;
            case at::ScalarType::Char:
                oss << static_cast<int>(t.item<int8_t>()); // Cast to int to avoid printing as char
                break;
            case at::ScalarType::Short:
                oss << t.item<int16_t>();
                break;
            case at::ScalarType::Int:
                oss << t.item<int32_t>();
                break;
            case at::ScalarType::Long:
                oss << t.item<int64_t>();
                break;
            case at::ScalarType::Half:
            case at::ScalarType::Float:
            case at::ScalarType::Double:
            case at::ScalarType::BFloat16:
                format_float_value(oss, t);
                break;
            case at::ScalarType::ComplexHalf:
            case at::ScalarType::ComplexFloat:
            case at::ScalarType::ComplexDouble:
                format_complex_value(oss, t);
                break;
            default:
                oss << "Unsupported dtype";
                break;
        }
        
        return oss.str();
    }

    // Format a Series for output
    static std::string format_series(const Series& s) {
        const at::Series& t = s.series();
        std::ostringstream oss;
        
        // Get shape and dtype
        std::vector<int64_t> shape;
        for (int64_t i = 0; i < t.dim(); i++) {
            shape.push_back(t.size(i));
        }
        
        // Print header with shape and dtype
        oss << "Series(shape=[";
        for (size_t i = 0; i < shape.size(); i++) {
            oss << shape[i];
            if (i < shape.size() - 1) {
                oss << ", ";
            }
        }
        oss << "], dtype=" << dtype_to_string(t.scalar_type()) << "):\n";
        
        // Format the series data
        if (t.dim() == 0) {
            // 0D series (scalar)
            oss << format_scalar(t);
        } else if (t.dim() == 1) {
            // 1D series (vector)
            format_1d(oss, t);
        } else if (t.dim() == 2) {
            // 2D series (matrix)
            format_2d(oss, t);
        } else {
            // Higher dimensional series
            format_nd(oss, t);
        }
        
        return oss.str();
    }

private:
    // Format a floating point value with appropriate precision
    static void format_float_value(std::ostringstream& oss, const at::Series& t) {
        double value;
        
        switch (t.scalar_type()) {
            case at::ScalarType::Half:
                value = static_cast<double>(t.item<at::Half>());
                break;
            case at::ScalarType::Float:
                value = static_cast<double>(t.item<float>());
                break;
            case at::ScalarType::Double:
                value = t.item<double>();
                break;
            case at::ScalarType::BFloat16:
                value = static_cast<double>(t.item<at::BFloat16>());
                break;
            default:
                oss << "Unsupported float type";
                return;
        }
        
        // Check if it's an integer value
        if (std::floor(value) == value) {
            oss << std::fixed << std::setprecision(1) << value; // Print as X.0
        } else {
            // Get string representation to count decimal places
            std::ostringstream temp;
            temp << std::fixed << std::setprecision(15) << value;
            std::string str = temp.str();
            
            // Find decimal point
            size_t decimal_pos = str.find('.');
            if (decimal_pos != std::string::npos) {
                // Count significant decimal places (ignoring trailing zeros)
                size_t last_non_zero = str.find_last_not_of('0');
                int decimal_places = (last_non_zero > decimal_pos) ? 
                                     last_non_zero - decimal_pos : 0;
                
                // If more than 5 decimal places, round to 5
                int precision = std::min(decimal_places, 5);
                oss << std::fixed << std::setprecision(precision) << value;
            } else {
                oss << value;
            }
        }
    }
    
    // Format a complex value
    static void format_complex_value(std::ostringstream& oss, const at::Series& t) {
        MSharp::complex<double> value;
        
        switch (t.scalar_type()) {
            case at::ScalarType::ComplexHalf:
                value = static_cast<MSharp::complex<double>>(t.item<MSharp::complex<at::Half>>());
                break;
            case at::ScalarType::ComplexFloat:
                value = static_cast<MSharp::complex<double>>(t.item<MSharp::complex<float>>());
                break;
            case at::ScalarType::ComplexDouble:
                value = t.item<MSharp::complex<double>>();
                break;
            default:
                oss << "Unsupported complex type";
                return;
        }
        
        // Format real part
        double real_val = value.real();
        double imag_val = value.imag();
        
        // Check if real part is an integer
        if (std::floor(real_val) == real_val) {
            oss << std::fixed << std::setprecision(1) << real_val; // Print as X.0
        } else {
            // Format with appropriate precision
            std::ostringstream temp;
            temp << std::fixed << std::setprecision(15) << real_val;
            std::string str = temp.str();
            
            // Find decimal point
            size_t decimal_pos = str.find('.');
            if (decimal_pos != std::string::npos) {
                // Count significant decimal places (ignoring trailing zeros)
                size_t last_non_zero = str.find_last_not_of('0');
                int decimal_places = (last_non_zero > decimal_pos) ? 
                                     last_non_zero - decimal_pos : 0;
                
                // If more than 5 decimal places, round to 5
                int precision = std::min(decimal_places, 5);
                oss << std::fixed << std::setprecision(precision) << real_val;
            } else {
                oss << real_val;
            }
        }
        
        // Add sign for imaginary part
        if (imag_val >= 0) {
            oss << "+";
        }
        
        // Format imaginary part
        if (std::floor(std::abs(imag_val)) == std::abs(imag_val)) {
            oss << std::fixed << std::setprecision(1) << imag_val << "j"; // Print as X.0j
        } else {
            // Format with appropriate precision
            std::ostringstream temp;
            temp << std::fixed << std::setprecision(15) << std::abs(imag_val);
            std::string str = temp.str();
            
            // Find decimal point
            size_t decimal_pos = str.find('.');
            if (decimal_pos != std::string::npos) {
                // Count significant decimal places (ignoring trailing zeros)
                size_t last_non_zero = str.find_last_not_of('0');
                int decimal_places = (last_non_zero > decimal_pos) ? 
                                     last_non_zero - decimal_pos : 0;
                
                // If more than 5 decimal places, round to 5
                int precision = std::min(decimal_places, 5);
                oss << std::fixed << std::setprecision(precision) << imag_val << "j";
            } else {
                oss << imag_val << "j";
            }
        }
    }
    
    // Format a 1D series
    static void format_1d(std::ostringstream& oss, const at::Series& t) {
        int64_t size = t.size(0);
        
        // If 10 or fewer elements, print all
        if (size <= 10) {
            oss << "[";
            for (int64_t i = 0; i < size; i++) {
                oss << format_scalar(t[i]);
                if (i < size - 1) {
                    oss << ", ";
                }
            }
            oss << "]";
        } else {
            // Otherwise, print first 5, ellipsis, last 5
            oss << "[";
            for (int64_t i = 0; i < 5; i++) {
                oss << format_scalar(t[i]);
                oss << ", ";
            }
            oss << "..., ";
            for (int64_t i = size - 5; i < size; i++) {
                oss << format_scalar(t[i]);
                if (i < size - 1) {
                    oss << ", ";
                }
            }
            oss << "]";
        }
    }
    
    // Format a 2D series
    static void format_2d(std::ostringstream& oss, const at::Series& t) {
        int64_t rows = t.size(0);
        int64_t cols = t.size(1);
        
        oss << "[";
        
        // Determine how many rows to show
        int64_t show_rows = rows;
        bool show_ellipsis = false;
        if (rows > 10) {
            show_rows = 5;
            show_ellipsis = true;
        }
        
        // Print the first 'show_rows' rows
        for (int64_t i = 0; i < show_rows; i++) {
            oss << "[";
            
            // Determine how many columns to show
            int64_t show_cols = cols;
            bool show_col_ellipsis = false;
            if (cols > 10) {
                show_cols = 5;
                show_col_ellipsis = true;
            }
            
            // Print the first 'show_cols' columns
            for (int64_t j = 0; j < show_cols; j++) {
                oss << format_scalar(t[i][j]);
                if (j < cols - 1) {
                    if (j == show_cols - 1 && show_col_ellipsis) {
                        oss << ", ..., ";
                        j = cols - show_cols - 1; // Skip to the last 'show_cols' columns
                    } else {
                        oss << ", ";
                    }
                }
            }
            
            oss << "]";
            if (i < rows - 1) {
                if (i == show_rows - 1 && show_ellipsis) {
                    oss << ",\n ..., \n";
                    i = rows - show_rows - 1; // Skip to the last 'show_rows' rows
                } else {
                    oss << ",\n ";
                }
            }
        }
        
        oss << "]";
    }
    
    // Format a higher dimensional series
    static void format_nd(std::ostringstream& oss, const at::Series& t) {
        // For higher dimensions, we'll use a recursive approach
        oss << "[";
        
        int64_t dim0_size = t.size(0);
        bool show_ellipsis = false;
        int64_t show_items = dim0_size;
        
        if (dim0_size > 6) {
            show_items = 3;
            show_ellipsis = true;
        }
        
        for (int64_t i = 0; i < show_items; i++) {
            // Extract the (n-1)-dimensional slice
            at::Series slice = t[i];
            
            if (slice.dim() == 2) {
                format_2d(oss, slice);
            } else {
                format_nd(oss, slice);
            }
            
            if (i < dim0_size - 1) {
                if (i == show_items - 1 && show_ellipsis) {
                    oss << ",\n ..., \n";
                    i = dim0_size - show_items - 1; // Skip to the last 'show_items' items
                } else {
                    oss << ",\n ";
                }
            }
        }
        
        oss << "]";
    }
};

// Stream operator for Series
inline std::ostream& operator<<(std::ostream& os, const Series& s) {
    os << SeriesFormatter::format_series(s);
    return os;
}

} // namespace pluss
