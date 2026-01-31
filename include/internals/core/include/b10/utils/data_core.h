#pragma once

// b10/utils/let.h
//
// A tiny “let” facility.
//
// Important notes:
// 1) C++ cannot introduce new keywords. So `let` here is a macro.
// 2) `let` does NOT magically make `std::vector` heterogeneous. `std::vector<T>`
//    is homogeneous by definition.
// 3) If you want a heterogeneous *sequence* with compile-time known alternatives,
//    use a tagged union like `std::variant<Ts...>`.
//
// Example:
//   #include <string>
//   using namespace std::string_literals;
//
//   let records = b10::hetero::make_vec(1, 2.5, "x"s, 7);
//   static_assert(std::is_same_v<decltype(records)::value_variant,
//                               std::variant<std::int64_t, double, std::string>>);
//
//   for (auto const& cell : records) {
//     std::visit([](auto const& x){ /* ... */ }, cell);
//   }

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifndef let
  #define let auto
#endif

namespace b10 {
namespace hetero {

// ---------------------------
// type_list utilities (C++17+)
// ---------------------------

template <class... Ts>
struct type_list { };

namespace detail {

template <class T>
using decay_t = std::remove_cv_t<std::remove_reference_t<T>>;

// contains<List, T>
template <class List, class T>
struct contains;

template <class T>
struct contains<type_list<>, T> : std::false_type { };

template <class Head, class... Tail, class T>
struct contains<type_list<Head, Tail...>, T>
    : std::conditional_t<std::is_same_v<Head, T>, std::true_type, contains<type_list<Tail...>, T>> { };

// push_unique<List, T>
template <class List, class T>
struct push_unique;

template <class... Ts, class T>
struct push_unique<type_list<Ts...>, T> {
  using type = std::conditional_t<contains<type_list<Ts...>, T>::value,
                                 type_list<Ts...>,
                                 type_list<Ts..., T>>;
};

// unique<List>
template <class List>
struct unique;

template <>
struct unique<type_list<>> {
  using type = type_list<>;
};

template <class Head, class... Tail>
struct unique<type_list<Head, Tail...>> {
private:
  using rest_unique = typename unique<type_list<Tail...>>::type;
public:
  using type = typename push_unique<rest_unique, Head>::type;
};

// list_to_variant<List>
template <class List>
struct list_to_variant;

template <class... Ts>
struct list_to_variant<type_list<Ts...>> {
  using type = std::variant<Ts...>;
};

// A small “default widen” policy so that common integer literals become int64_t.
// Feel free to adjust this for your project.
template <class T>
struct normalize_value_type {
  using D = decay_t<T>;
  using type = std::conditional_t<
      (std::is_integral_v<D> && !std::is_same_v<D, bool>),
      std::int64_t,
      D>;
};

template <class T>
using normalize_value_type_t = typename normalize_value_type<T>::type;

} // namespace detail

// ---------------------------
// hetero::vec  (vector of variant)
// ---------------------------

template <class... Ts>
class vec {
public:
  using value_variant = std::variant<Ts...>;
  using storage_type  = std::vector<value_variant>;

  vec() = default;

  explicit vec(std::size_t reserve_n) { data_.reserve(reserve_n); }

  std::size_t size() const noexcept { return data_.size(); }
  bool empty() const noexcept { return data_.empty(); }

  value_variant const& operator[](std::size_t i) const noexcept { return data_[i]; }
  value_variant& operator[](std::size_t i) noexcept { return data_[i]; }

  value_variant const& at(std::size_t i) const { return data_.at(i); }
  value_variant& at(std::size_t i) { return data_.at(i); }

  auto begin() noexcept { return data_.begin(); }
  auto end() noexcept { return data_.end(); }
  auto begin() const noexcept { return data_.begin(); }
  auto end() const noexcept { return data_.end(); }

  template <class T>
  void push(T&& v) {
    data_.emplace_back(std::forward<T>(v));
  }

  storage_type const& data() const noexcept { return data_; }
  storage_type& data() noexcept { return data_; }

private:
  storage_type data_;
};

// make_vec(args...)
//
// Deduces a *set* of alternative types from the arguments, normalizes them,
// uniquifies them, then stores each argument into a vector<variant<...>>.
//
// Example:
//   auto v = make_vec(1, 2, 3.0);
//   // v is vec<int64_t, double>
//
template <class... Args>
auto make_vec(Args&&... args) {
  static_assert(sizeof...(Args) > 0, "make_vec requires at least one argument");

  using raw_list = type_list<detail::normalize_value_type_t<Args>...>;
  using uniq_list = typename detail::unique<raw_list>::type;
  using V = typename detail::list_to_variant<uniq_list>::type;

  // Expand uniq_list into vec<Ts...> by using V as the public value_variant.
  // We build vec by extracting its alternatives through a helper.
  struct builder;

  template <class... Ts>
  struct builder_impl {
    using type = vec<Ts...>;
  };

  // Helper: map variant alternatives to vec alternatives
  template <class Variant>
  struct variant_to_vec;

  template <class... Ts>
  struct variant_to_vec<std::variant<Ts...>> {
    using type = vec<Ts...>;
  };

  using Vec = typename variant_to_vec<V>::type;

  Vec out;
  out.data().reserve(sizeof...(Args));
  (out.push(detail::normalize_value_type_t<Args>(std::forward<Args>(args))), ...);
  return out;
}

// ---------------------------
// Introspection helpers
// ---------------------------

template <class... Ts>
constexpr std::size_t type_count(vec<Ts...> const&) noexcept {
  return sizeof...(Ts);
}

} // namespace hetero
} // namespace b10
