#pragma once
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <vector>

namespace pluss::codem::factories {

//------------------------------------------------------------------------------
// Nested initializer_list utilities (up to 7D) for pluss::value({{...}})
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
inline void flatten_scalar(std::vector<double>& out, const T& v) {
  out.push_back(static_cast<double>(v));
}

template <class List>
static void infer_shape_and_flatten(const List& list,
                                    std::vector<std::int64_t>& sizes,
                                    std::vector<double>& out,
                                    int level) {
  using Elem = typename List::value_type;
  const std::int64_t n = static_cast<std::int64_t>(list.size());
  if ((int)sizes.size() <= level) sizes.resize((std::size_t)level+1, -1);

  if (sizes[(std::size_t)level] == -1) sizes[(std::size_t)level] = n;
  else {
    // ragged check
    if (sizes[(std::size_t)level] != n) {
      throw std::runtime_error("pluss::value: ragged nested initializer_list is not supported");
    }
  }

  if constexpr (is_init_list<Elem>::value) {
    for (const auto& sub : list) {
      infer_shape_and_flatten(sub, sizes, out, level+1);
    }
  } else {
    // leaf
    for (const auto& v : list) flatten_scalar(out, v);
  }
}

template <class List>
inline InferredList infer(const List& list) {
  InferredList r;
  infer_shape_and_flatten(list, r.sizes, r.data, 0);
  // Trim sizes if leaf ended early (e.g. 1D)
  while (!r.sizes.empty() && r.sizes.back() == -1) r.sizes.pop_back();
  return r;
}

} // namespace pluss::codem::factories
