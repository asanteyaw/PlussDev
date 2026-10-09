#pragma once

// `suppose(x)` -- a small fluent box around an arbitrary value: chain
// non-mutating transforms with `.then()` / `.transform()`, run side effects with
// `.tap()`, convert with `.into<U>()`, and pull the value back out with
// `.value()` / `.unwrap()`. Unlike `pluss::tils::let` (see <pluss/tils/let.h>),
// this wraps *any* type and is included by the PlussTils umbrella.

#include <functional>
#include <type_traits>
#include <utility>

namespace pluss::tils {

template <class T>
class SupposeBox {
 public:
  using value_type = T;

  constexpr explicit SupposeBox(T value) : value_(std::move(value)) {}

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
    return SupposeBox<std::decay_t<std::invoke_result_t<F&, T&>>>(
        std::invoke(std::forward<F>(fn), value_));
  }

  template <class F>
  constexpr auto then(F&& fn) const& {
    return SupposeBox<std::decay_t<std::invoke_result_t<F&, const T&>>>(
        std::invoke(std::forward<F>(fn), value_));
  }

  template <class F>
  constexpr auto then(F&& fn) && {
    return SupposeBox<std::decay_t<std::invoke_result_t<F&, T&&>>>(
        std::invoke(std::forward<F>(fn), std::move(value_)));
  }

  template <class F>
  constexpr auto transform(F&& fn) & { return then(std::forward<F>(fn)); }

  template <class F>
  constexpr auto transform(F&& fn) const& { return then(std::forward<F>(fn)); }

  template <class F>
  constexpr auto transform(F&& fn) && { return std::move(*this).then(std::forward<F>(fn)); }

  template <class F>
  constexpr SupposeBox& tap(F&& fn) & {
    std::invoke(std::forward<F>(fn), value_);
    return *this;
  }

  template <class F>
  constexpr const SupposeBox& tap(F&& fn) const& {
    std::invoke(std::forward<F>(fn), value_);
    return *this;
  }

  template <class F>
  constexpr SupposeBox&& tap(F&& fn) && {
    std::invoke(std::forward<F>(fn), value_);
    return std::move(*this);
  }

  template <class U>
  constexpr auto into() const& {
    static_assert(std::is_constructible_v<U, const T&>,
                  "suppose::into<U>: U must be constructible from const T&");
    return SupposeBox<U>(U(value_));
  }

  template <class U>
  constexpr auto into() && {
    static_assert(std::is_constructible_v<U, T&&>,
                  "suppose::into<U>: U must be constructible from T&&");
    return SupposeBox<U>(U(std::move(value_)));
  }

 private:
  T value_;
};

struct SupposeFn {
  template <class T>
  constexpr auto operator()(T&& value) const {
    return SupposeBox<std::decay_t<T>>(std::forward<T>(value));
  }

  template <class To, class From>
  constexpr auto as(From&& value) const {
    static_assert(std::is_constructible_v<To, From&&>,
                  "suppose.as<To>(value): To must be constructible from value");
    return SupposeBox<To>(To(std::forward<From>(value)));
  }
};

inline constexpr SupposeFn suppose{};

}  // namespace pluss::tils
