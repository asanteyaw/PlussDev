#pragma once
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::codem::factories {

//------------------------------------------------------------------------------
// Nested initializer_list utilities (up to 7D) for pluss::make_series({{...}})
//------------------------------------------------------------------------------
//
// We support:
//   value(Scalar)          -> 0D Series
//   value({a,b,c})         -> 1D Series
//   value({{...},{...}})   -> 2D Series
// ...
// up to 7D.
//
// Semantics:
// - shape is inferred from the nesting
// - ragged lists throw at runtime
// - dtype deduction prefers: bool -> int64 -> double (unless user forces dtype)
//------------------------------------------------------------------------------

struct InferredList {
  std::vector<std::int64_t> sizes;
  std::vector<double> data; // stored as double; later cast to target dtype
  // Exact int64 form of `data`, populated iff the literal's leaf type is
  // integral (not bool, not float) -- see leaf_is_exact_integer below for
  // where it's set and value(std::initializer_list<T>, ...) in SeriesBody.h
  // for where it's used. `data` alone loses precision beyond 2^53 for
  // magnitudes a double's mantissa can't represent exactly.
  std::vector<std::int64_t> data_i64;
  bool leaf_is_exact_integer = false;
};

template <class T>
inline void append_sizes(std::vector<std::int64_t>& dst, const std::vector<std::int64_t>& src) {
  dst.insert(dst.end(), src.begin(), src.end());
}

template <class T>
struct is_init_list : std::false_type {};
template <class U>
struct is_init_list<std::initializer_list<U>> : std::true_type {};

template <class T>
struct depth {
  static constexpr int value = 0;
};
template <class U>
struct depth<std::initializer_list<U>> {
  static constexpr int value = 1 + depth<U>::value;
};

template <class T>
struct leaf_type {
  using type = T;
};
template <class U>
struct leaf_type<std::initializer_list<U>> {
  using type = typename leaf_type<U>::type;
};

template <class T>
inline constexpr bool leaf_is_exact_integer_v =
    std::is_integral_v<T> && !std::is_same_v<T, bool>;

template <class T>
inline void flatten_scalar(std::vector<double>& out, std::vector<std::int64_t>& out_i64, const T& v) {
  out.push_back(static_cast<double>(v));
  if constexpr (leaf_is_exact_integer_v<T>) {
    out_i64.push_back(static_cast<std::int64_t>(v));
  }
}

template <class List>
static void infer_shape_and_flatten(const List& list,
                                    std::vector<std::int64_t>& sizes,
                                    std::vector<double>& out,
                                    std::vector<std::int64_t>& out_i64,
                                    int level) {
  using Elem = typename List::value_type;
  const std::int64_t n = static_cast<std::int64_t>(list.size());
  if ((int)sizes.size() <= level) sizes.resize((std::size_t)level+1, -1);

  if (sizes[(std::size_t)level] == -1) sizes[(std::size_t)level] = n;
  else {
    // ragged check
    if (sizes[(std::size_t)level] != n) {
      ::pluss::throw_error("pluss::value: ragged nested initializer_list is not supported");
    }
  }

  if constexpr (is_init_list<Elem>::value) {
    for (const auto& sub : list) {
      infer_shape_and_flatten(sub, sizes, out, out_i64, level+1);
    }
  } else {
    // leaf
    for (const auto& v : list) flatten_scalar(out, out_i64, v);
  }
}

template <class List>
inline InferredList infer(const List& list) {
  InferredList r;
  r.leaf_is_exact_integer = leaf_is_exact_integer_v<typename leaf_type<List>::type>;
  infer_shape_and_flatten(list, r.sizes, r.data, r.data_i64, 0);
  // Trim sizes if leaf ended early (e.g. 1D)
  while (!r.sizes.empty() && r.sizes.back() == -1) r.sizes.pop_back();
  return r;
}

} // namespace pluss::codem::factories
