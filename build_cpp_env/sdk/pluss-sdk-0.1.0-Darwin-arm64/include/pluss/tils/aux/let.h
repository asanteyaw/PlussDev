#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace pluss::aux {

template <class T>
class let_box {
public:
  using value_type = T;

  constexpr explicit let_box(T value) : value_(std::move(value)) {}

  constexpr T& value() & noexcept { return value_; }
  constexpr const T& value() const& noexcept { return value_; }
  constexpr T&& value() && noexcept { return std::move(value_); }
  constexpr const T&& value() const&& noexcept { return std::move(value_); }

  constexpr T& unwrap() & noexcept { return value_; }
  constexpr const T& unwrap() const& noexcept { return value_; }
  constexpr T&& unwrap() && noexcept { return std::move(value_); }
  constexpr const T&& unwrap() const&& noexcept { return std::move(value_); }

  template <class F>
  constexpr auto then(F&& fn) & {
    return let_box<std::decay_t<std::invoke_result_t<F&, T&>>>(
        std::invoke(std::forward<F>(fn), value_));
  }

  template <class F>
  constexpr auto then(F&& fn) const& {
    return let_box<std::decay_t<std::invoke_result_t<F&, const T&>>>(
        std::invoke(std::forward<F>(fn), value_));
  }

  template <class F>
  constexpr auto then(F&& fn) && {
    return let_box<std::decay_t<std::invoke_result_t<F&, T&&>>>(
        std::invoke(std::forward<F>(fn), std::move(value_)));
  }

  template <class F>
  constexpr auto transform(F&& fn) & { return then(std::forward<F>(fn)); }

  template <class F>
  constexpr auto transform(F&& fn) const& { return then(std::forward<F>(fn)); }

  template <class F>
  constexpr auto transform(F&& fn) && { return std::move(*this).then(std::forward<F>(fn)); }

  template <class F>
  constexpr let_box& tap(F&& fn) & {
    std::invoke(std::forward<F>(fn), value_);
    return *this;
  }

  template <class F>
  constexpr const let_box& tap(F&& fn) const& {
    std::invoke(std::forward<F>(fn), value_);
    return *this;
  }

  template <class F>
  constexpr let_box&& tap(F&& fn) && {
    std::invoke(std::forward<F>(fn), value_);
    return std::move(*this);
  }

  template <class U>
  constexpr auto into() const& {
    static_assert(std::is_constructible_v<U, const T&>, "let::into<U>: U must be constructible from const T&");
    return let_box<U>(U(value_));
  }

  template <class U>
  constexpr auto into() && {
    static_assert(std::is_constructible_v<U, T&&>, "let::into<U>: U must be constructible from T&&");
    return let_box<U>(U(std::move(value_)));
  }

private:
  T value_;
};

struct suppose_t {
  template <class T>
  constexpr auto operator()(T&& value) const {
    return let_box<std::decay_t<T>>(std::forward<T>(value));
  }

  template <class To, class From>
  constexpr auto as(From&& value) const {
    static_assert(std::is_constructible_v<To, From&&>,
                  "suppose.as<To>(value): To must be constructible from value");
    return let_box<To>(To(std::forward<From>(value)));
  }
};

inline constexpr suppose_t suppose{};

template <class... Ts>
class let_one_of {
  static_assert(sizeof...(Ts) >= 2, "let_as(T1, T2, ...): provide at least two candidate types");

public:
  using variant_type = std::variant<Ts...>;

  let_one_of() = delete;
  let_one_of(const let_one_of&) = default;
  let_one_of(let_one_of&&) noexcept = default;
  let_one_of& operator=(const let_one_of&) = default;
  let_one_of& operator=(let_one_of&&) noexcept = default;
  ~let_one_of() = default;

  template <class U,
            class D = std::decay_t<U>,
            std::enable_if_t<(std::is_same_v<D, Ts> || ...), int> = 0>
  constexpr let_one_of(U&& value)
      : value_(std::in_place_type<D>, std::forward<U>(value)) {}

  template <class U,
            class D = std::decay_t<U>,
            std::enable_if_t<(std::is_same_v<D, Ts> || ...), int> = 0>
  constexpr let_one_of& operator=(U&& value) {
    value_.template emplace<D>(std::forward<U>(value));
    return *this;
  }

  constexpr variant_type& value() & noexcept { return value_; }
  constexpr const variant_type& value() const& noexcept { return value_; }
  constexpr variant_type&& value() && noexcept { return std::move(value_); }
  constexpr const variant_type&& value() const&& noexcept { return std::move(value_); }

  template <class T>
  constexpr bool is() const noexcept {
    return std::holds_alternative<T>(value_);
  }

  template <class T>
  constexpr T& get() & {
    return std::get<T>(value_);
  }

  template <class T>
  constexpr const T& get() const& {
    return std::get<T>(value_);
  }

  template <class T>
  constexpr T&& get() && {
    return std::get<T>(std::move(value_));
  }

  template <class F>
  constexpr decltype(auto) visit(F&& fn) & {
    return std::visit(std::forward<F>(fn), value_);
  }

  template <class F>
  constexpr decltype(auto) visit(F&& fn) const& {
    return std::visit(std::forward<F>(fn), value_);
  }

private:
  variant_type value_;
};

} // namespace pluss::aux
