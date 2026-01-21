#pragma once

// #include "b10/utils/Macros.h"
#include <pluss/util_helpers.h>
#include <pluss/series_factory.h>
#include <pluss/note_factory.h>
#include <type_traits>
#include <utility>

namespace pluss {

// Forward declarations for helper templates
namespace detail {
    // Helper for detecting if a method returns void
    template <typename T>
    struct is_void_return : std::is_same<T, void> {};

    // Helper for member function pointer detection
    template <typename T>
    struct is_member_function_pointer : std::is_member_function_pointer<T> {};
}

/**
 * @brief A higher-level utility function for method dispatch with non-void return
 * 
 * This function takes an object, a method pointer, and arguments, then calls the method
 * on the object with the arguments. It returns whatever the method returns.
 * 
 * @tparam T The type of the object
 * @tparam R The return type of the method (non-void)
 * @tparam Class The class that the method belongs to
 * @tparam MethodArgs The parameter types of the method
 * @tparam CallArgs The types of the arguments being passed
 * @param obj The object to call the method on
 * @param method_ptr A pointer to the method to call
 * @param args The arguments to pass to the method
 * @return The result of calling the method on the object with the arguments
 */
template <typename T, typename R, typename Class, typename... MethodArgs, typename... CallArgs,
          typename std::enable_if<!detail::is_void_return<R>::value, int>::type = 0>
auto dispatch(T&& obj, R (Class::*method_ptr)(MethodArgs...) const, CallArgs&&... args) {
    return (std::forward<T>(obj).*method_ptr)(std::forward<CallArgs>(args)...);
}

/**
 * @brief Overload for non-const methods with non-void return
 */
template <typename T, typename R, typename Class, typename... MethodArgs, typename... CallArgs,
          typename std::enable_if<!detail::is_void_return<R>::value, int>::type = 0>
auto dispatch(T&& obj, R (Class::*method_ptr)(MethodArgs...), CallArgs&&... args) {
    return (std::forward<T>(obj).*method_ptr)(std::forward<CallArgs>(args)...);
}

/**
 * @brief Overload for const methods with void return
 */
template <typename T, typename Class, typename... MethodArgs, typename... CallArgs>
void dispatch(T&& obj, void (Class::*method_ptr)(MethodArgs...) const, CallArgs&&... args) {
    (std::forward<T>(obj).*method_ptr)(std::forward<CallArgs>(args)...);
}

/**
 * @brief Overload for non-const methods with void return
 */
template <typename T, typename Class, typename... MethodArgs, typename... CallArgs>
void dispatch(T&& obj, void (Class::*method_ptr)(MethodArgs...), CallArgs&&... args) {
    (std::forward<T>(obj).*method_ptr)(std::forward<CallArgs>(args)...);
}

/**
 * @brief Universal fallback for any object and method
 * 
 * This overload uses the PLUSS_TRANSMIT macro which works with any object type
 * and method name, including third-party libraries and user-defined types.
 * 
 * @tparam T The type of the object
 * @tparam MethodToken The type of the method token (can be a method name, function pointer, etc.)
 * @tparam Args The types of the arguments to pass to the method
 * @param obj The object to call the method on
 * @param method The method to call
 * @param args The arguments to pass to the method
 * @return The result of calling the method on the object with the arguments
 */
// template <typename T, typename MethodToken, typename... Args>
// auto dispatch(T&& obj, MethodToken&& method, Args&&... args) {
//     return PLUSS_TRANSMIT(std::forward<T>(obj), method, std::forward<Args>(args)...);
// }

/**
 * @brief Helper function to dispatch based on scalar type
 * 
 * This function takes a scalar type and a callable that can be invoked with different
 * template arguments based on the scalar type. It supports a wide range of data types
 * and can be easily extended to support more types in the future.
 * 
 * @tparam Callable The type of the callable
 * @param scalar_type The scalar type to dispatch on
 * @param callable The callable to invoke with the appropriate template argument
 */
template <typename F, typename... Args,
 typename = std::enable_if_t<(std::is_same_v<std::decay_t<Args>, Series> || ...)>>
Series queue(F&& func, Args&&... args) {
    // Find the first Series argument to get its shape
    Series first_series;
    auto select_first_series = [&](auto&& arg) {
    if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, Series>) {
    first_series = std::forward<decltype(arg)>(arg);
    return true;
    }
    return false;
    };

    (void)(bool{select_first_series(std::forward<Args>(args))} || ...);

    // Get the shape of the Series
    auto shape = first_series.sizes();
    // Create a result Series with the same shape
    Series result = pluss::empty(shape, first_series.options_config());

    // Get the scalar type of the first Series
    auto scalar_type = first_series.scalar_type();

    // Use a template function to handle different data types
    auto process_with_type = [&]<typename T>() {
        // Get raw pointers to the data for each Series argument
        auto get_data_ptr = [](auto&& arg) -> const T* {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, Series>) {
                return arg.template data_ptr<T>();
            } else {
                return nullptr;
            }
        };

        // Construct the tuple via pack expansion
        auto data_ptrs = std::make_tuple(get_data_ptr(std::forward<Args>(args))...);

        // Get a raw pointer to the result data for writing
        T* result_ptr = result.data_ptr<T>();
        
        // Get the total number of elements
        int64_t num_elements = first_series.numel();
        
        // Process elements in chunks for better cache utilization
        constexpr int64_t CHUNK_SIZE = 1024;
        
        // Use OpenMP for parallelization
        #pragma omp parallel for
        for (int64_t i = 0; i < num_elements; i += CHUNK_SIZE) {
            int64_t chunk_end = std::min(i + CHUNK_SIZE, num_elements);
        
            // Process each element in the chunk
            #pragma omp simd
            for (int64_t j = i; j < chunk_end; ++j) {
                auto value = apply_func(func, std::index_sequence_for<Args...>{}, data_ptrs, j, std::forward<Args>(args)...);
                result_ptr[j] = value;
            }
        }
    };

    // Dispatch based on the scalar type
    if (scalar_type == at::kDouble) {
        process_with_type.template operator()<double>();
    } else if (scalar_type == at::kFloat) {
        process_with_type.template operator()<float>();
    } else if (scalar_type == at::kInt) {
        process_with_type.template operator()<int>();
    } else if (scalar_type == at::kLong) {
        process_with_type.template operator()<int64_t>();
    } else {
        // Default to double for other types
        process_with_type.template operator()<double>();
    }

    return result;
}

// Helper function to apply the function to the elements
template <typename F, typename Tuple, typename... Args, size_t... Is>
auto apply_func(F&& func, std::index_sequence<Is...>,
       const Tuple& data_ptrs, int64_t idx, Args&&... args) {

    auto get_element = [&](auto&& arg, auto index_constant) -> decltype(auto) {
        constexpr size_t i = decltype(index_constant)::value;
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, Series>) {
            return std::get<i>(data_ptrs)[idx];
        } else {
            return arg;
        }
    };

    return func(get_element(std::forward<Args>(args), std::integral_constant<size_t, Is>{})...);
}
/*
 * @brief A high-performance vectorization function for Note
 * 
 * This function applies a callable to each element of the input Note
 * and returns a new Note with the results.
 * 
 * @tparam F The type of the callable
 * @tparam Args The types of the input arguments (at least one must be a Note)
 * @param func The callable to apply to each element
 * @param args The input arguments (Note or scalars)
 * @return A new Note with the results
 */
template <typename F, typename... Args,
          typename = std::enable_if_t<(std::is_same_v<std::decay_t<Args>, Note> || ...)>>
Note queue(F&& func, Args&&... args) {
    // Find the first Note argument
    Note first_note;
    
    auto select_first_note = [&](auto&& arg) {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, Note>) {
            first_note = std::forward<decltype(arg)>(arg);
            return true;
        }
        return false;
    };

    (void)(bool{select_first_note(std::forward<Args>(args))} || ...);
    
    // Get the vector representation of the Note
    auto vec = first_note.vector();
    
    // Create a result vector of Chords
    std::vector<Chord> result_vec;
    result_vec.reserve(vec.size());
    
    // Apply the function to each element
    for (size_t i = 0; i < vec.size(); ++i) {
        // Get the elements from each argument
        auto get_element = [i](auto&& arg) -> decltype(auto) {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, Note>) {
                return arg.vector()[i].str();
            } else {
                return arg;
            }
        };
        
        // Apply the function to the elements
        auto value = func(get_element(std::forward<Args>(args))...);
        
        // Convert the result to a Chord and add it to the result vector
        result_vec.push_back(Chord(value));
    }
    
    // Create a new Note from the result vector using the proper factory function
    return pluss::make_note(result_vec);
}

} // namespace pluss
