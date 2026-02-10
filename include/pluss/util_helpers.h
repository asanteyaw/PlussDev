#pragma once

#include <PlussQit/core/SeriesBody.h>
#include <pluss/Note.h>
#include <pluss/series_factory.h>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss {
namespace helpers {

/**
 * @brief Type trait to check if a type is a pluss type
 */
template <typename T>
struct is_pluss_type : std::false_type {};

template <>
struct is_pluss_type<pluss::Series> : std::true_type {};

template <>
struct is_pluss_type<pluss::Note> : std::true_type {};

template <typename T>
inline constexpr bool is_pluss_type_v = is_pluss_type<T>::value;

/**
 * @brief Helper to get the shape of the first pluss type argument
 */
template <typename T, typename... Rest>
auto get_shape(T&& first, Rest&&... rest) {
    if constexpr (is_pluss_type_v<std::decay_t<T>>) {
        return first.sizes();
    } else {
        return get_shape(std::forward<Rest>(rest)...);
    }
}

template <typename T>
auto get_shape(T&& arg) {
    if constexpr (is_pluss_type_v<std::decay_t<T>>) {
        return arg.sizes();
    } else {
        // If no pluss type is found, return an empty shape
        return std::vector<int64_t>{};
    }
}

/**
 * @brief Helper to create a result container with the given shape
 */
template <typename ResultType>
auto create_result_container(const std::vector<int64_t>& shape) {
    // Create a new container based on the result type and shape
    if constexpr (std::is_same_v<ResultType, float> || 
                  std::is_same_v<ResultType, double> || 
                  std::is_same_v<ResultType, int> ||
                  std::is_same_v<ResultType, bool>) {
        // For numeric types, create a Series
        return pluss::from_torch(at::empty(shape, at::dtype<ResultType>()));
    } else {
        // For other types, create an appropriate container
        // This would need to be specialized for different pluss types
        return pluss::from_torch(at::empty(shape, at::dtype<float>()));
    }
}

/**
 * @brief Apply the function to each element using SIMD where possible
 */
template <typename Result, typename F, typename... Args>
void apply_vectorized(Result& result, F&& func, Args&&... args) {
    // Get the total number of elements
    int64_t total_elements = result.numel();
    
    // Process elements in chunks for better cache utilization
    constexpr int64_t CHUNK_SIZE = 1024;
    
    // Use OpenMP for parallelization if available
    #pragma omp parallel for if(total_elements > 10000)
    for (int64_t i = 0; i < total_elements; i += CHUNK_SIZE) {
        int64_t chunk_end = std::min(i + CHUNK_SIZE, total_elements);
        
        for (int64_t j = i; j < chunk_end; ++j) {
            // Apply the function to each element
            auto value = apply_to_element(std::forward<F>(func), j, std::forward<Args>(args)...);
            
            // Set the value in the result container
            // For Series, we can use the index method with a scalar
            if constexpr (is_pluss_type_v<std::decay_t<Result>> && 
                          std::is_same_v<std::decay_t<Result>, pluss::Series>) {
                // Create a series with a single value
                at::Series value_tensor = at::scalar_tensor(value);
                // Use index_put_ to set the value at index j
                at::Series result_tensor = result;
                result_tensor.index_put_({j}, value_tensor);
            }
            // For Note, we would need a different approach
            // This is a placeholder for now
        }
    }
}

/**
 * @brief Apply the function to a specific element
 */
template <typename F, typename... Args>
auto apply_to_element(F&& func, int64_t index, Args&&... args) {
    return std::forward<F>(func)(get_element(index, std::forward<Args>(args))...);
}

/**
 * @brief Get the element at the given index
 */
template <typename T>
auto get_element(int64_t index, T&& arg) {
    if constexpr (is_pluss_type_v<std::decay_t<T>>) {
        // For pluss types, get the element at the index
        return arg.view(-1).index({index}).item();
    } else {
        // For scalars, just return the scalar
        return arg;
    }
}

} // namespace helpers
} // namespace pluss
