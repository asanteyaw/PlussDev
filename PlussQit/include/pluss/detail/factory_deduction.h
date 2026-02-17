#pragma once
#include <cstdint>
#include <initializer_list>
#include <type_traits>

#include "pluss/codem/codem.h"

namespace pluss::detail {

// Leaf type of nested initializer_list
template <class T>
struct leaf_type { using type = T; };

template <class U>
struct leaf_type<std::initializer_list<U>> { using type = typename leaf_type<U>::type; };

template <class T>
using leaf_type_t = typename leaf_type<T>::type;

// Depth of nested initializer_list
template <class T>
struct depth { static constexpr int value = 0; };

template <class U>
struct depth<std::initializer_list<U>> { static constexpr int value = 1 + depth<U>::value; };

// Map C++ leaf type -> ScalarType (default policy)
template <class Leaf>
struct default_scalar_type {
  static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Float32;
};

// bool
template <>
struct default_scalar_type<bool> {
  static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Bool;
};

// floating
template <>
struct default_scalar_type<float> {
  static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Float32;
};
template <>
struct default_scalar_type<double> {
  static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Float64;
};

// signed integers -> Int64 by default (Torch-like)
template <> struct default_scalar_type<std::int8_t> { static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Int64; };
template <> struct default_scalar_type<std::int16_t> { static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Int64; };
template <> struct default_scalar_type<std::int32_t> { static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Int64; };
template <> struct default_scalar_type<std::int64_t> { static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Int64; };

// unsigned integers -> UInt8 only if the user explicitly uses uint8_t in the list;
// otherwise we still choose Int64 for safety. (User can override dtype.)
template <> struct default_scalar_type<std::uint8_t> { static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::UInt8; };
template <> struct default_scalar_type<std::uint16_t> { static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Int64; };
template <> struct default_scalar_type<std::uint32_t> { static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Int64; };
template <> struct default_scalar_type<std::uint64_t> { static constexpr pluss::codem::ScalarType value = pluss::codem::ScalarType::Int64; };

template <class T>
constexpr pluss::codem::ScalarType default_scalar_type_v = default_scalar_type<std::remove_cv_t<T>>::value;

// Helper: choose dtype for initializer_list when user didn't provide one.
template <class InitList>
constexpr pluss::codem::ScalarType dtype_from_init_list() {
  using Leaf = leaf_type_t<InitList>;
  static_assert(depth<InitList>::value <= 7, "pluss::value: supports up to 7D nested initializer_list");
  if constexpr (std::is_same_v<std::remove_cv_t<Leaf>, bool>) return pluss::codem::ScalarType::Bool;
  if constexpr (std::is_floating_point_v<Leaf>) {
    return std::is_same_v<std::remove_cv_t<Leaf>, double> ? pluss::codem::ScalarType::Float64 : pluss::codem::ScalarType::Float32;
  }
  if constexpr (std::is_same_v<std::remove_cv_t<Leaf>, std::uint8_t>) return pluss::codem::ScalarType::UInt8;
  if constexpr (std::is_integral_v<Leaf>) return pluss::codem::ScalarType::Int64;
  return pluss::codem::ScalarType::Float32;
}

} // namespace pluss::detail
