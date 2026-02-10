#pragma once

// #include "b10/utils/Macros.h"
#include <pluss/util_helpers.h>
#include <pluss/series_factory.h>
#include <pluss/note_factory.h>
#include <PlussQit/Parallel.h>
#include <type_traits>
#include <tuple>
#include <utility>

namespace pluss {
namespace detail {
template <typename T>
struct type_tag {
    using type = T;
};

template <typename T, typename F, typename PtrTuple, typename ArgTuple>
T apply_func_from_tuple(
    F const& func,
    const PtrTuple& data_ptrs,
    const ArgTuple& args_tuple,
    int64_t idx);
} // namespace detail
} // namespace pluss

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
    // Capture all arguments exactly once to avoid forwarding/move pitfalls inside loops.
    // Copying a Series is cheap (refcount bump) and keeps TensorImpl alive.
    auto args_tuple = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);

    // Find the first Series argument to get shape/options.
    Series first_series;
    bool found = false;
    std::apply(
        [&](auto const&... packed) {
            (([&] {
                 if (!found) {
                     using P = std::decay_t<decltype(packed)>;
                     if constexpr (std::is_same_v<P, Series>) {
                         first_series = packed;
                         found = true;
                     }
                 }
             }()),
             ...);
        },
        args_tuple);

    // Defensive: should be guaranteed by SFINAE, but keep it safe.
    if (!found || !first_series.defined()) {
        return Series();
    }

    const auto shape = first_series.sizes();
    Series result = pluss::empty(shape, first_series.options_config());
    const auto scalar_type = first_series.scalar_type();

    // Handle different data types
    auto process_with_type = [&](auto tag) {
        using T = typename decltype(tag)::type;
        auto get_data_ptr = [](auto const& arg) -> const T* {
            using A = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<A, Series>) {
                return arg.template data_ptr<T>();
            } else {
                return nullptr;
            }
        };

        const auto data_ptrs = std::apply(
            [&](auto const&... packed) {
                return std::make_tuple(get_data_ptr(packed)...);
            },
            args_tuple);

        T* result_ptr = result.data_ptr<T>();
        const int64_t num_elements = first_series.numel();

        auto apply_from_tuple = [&](int64_t idx) -> T {
            return detail::apply_func_from_tuple<T>(func, data_ptrs, args_tuple, idx);
        };

        // Use PyTorch's parallel backend (no OpenMP dependency; works on macOS).
        at::parallel_for(0, num_elements, 4096, [&](int64_t begin, int64_t end) {
            for (int64_t j = begin; j < end; ++j) {
                result_ptr[j] = apply_from_tuple(j);
            }
        });
    };

    // Dispatch based on the scalar type
    if (scalar_type == at::kDouble) {
        process_with_type(detail::type_tag<double>{});
    } else if (scalar_type == at::kFloat) {
        process_with_type(detail::type_tag<float>{});
    } else if (scalar_type == at::kInt) {
        process_with_type(detail::type_tag<int>{});
    } else if (scalar_type == at::kLong) {
        process_with_type(detail::type_tag<int64_t>{});
    } else {
        process_with_type(detail::type_tag<double>{});
    }

    return result;
}

namespace detail {

template <typename T, typename F, typename PtrTuple, typename ArgTuple, size_t... Is>
T apply_func_from_tuple_impl(
    F const& func,
    const PtrTuple& data_ptrs,
    const ArgTuple& args_tuple,
    int64_t idx,
    std::index_sequence<Is...>) {

    auto get_arg = [&](auto ic) -> decltype(auto) {
        constexpr size_t i = decltype(ic)::value;
        using A = std::decay_t<std::tuple_element_t<i, ArgTuple>>;
        if constexpr (std::is_same_v<A, Series>) {
            return std::get<i>(data_ptrs)[idx];
        } else {
            return std::get<i>(args_tuple);
        }
    };

    return static_cast<T>(func(get_arg(std::integral_constant<size_t, Is>{})...));
}

template <typename T, typename F, typename PtrTuple, typename ArgTuple>
T apply_func_from_tuple(
    F const& func,
    const PtrTuple& data_ptrs,
    const ArgTuple& args_tuple,
    int64_t idx) {
    return apply_func_from_tuple_impl<T>(
        func,
        data_ptrs,
        args_tuple,
        idx,
        std::make_index_sequence<std::tuple_size_v<ArgTuple>>{});
}

} // namespace detail
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
