#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"
#include "pluss/queue.h"

namespace pluss::tils {

namespace detail {

template <class T>
struct is_let_payload : std::false_type {};

template <>
struct is_let_payload<::pluss::Series> : std::true_type {};

template <>
struct is_let_payload<::pluss::Note> : std::true_type {};

template <>
struct is_let_payload<::pluss::codeh::Timeline> : std::true_type {};

template <>
struct is_let_payload<::pluss::codeh::TimeIndex> : std::true_type {};

template <>
struct is_let_payload<::pluss::codeh::Duration> : std::true_type {};

template <>
struct is_let_payload<::pluss::codev::Queue> : std::true_type {};

template <class T>
inline constexpr bool is_let_payload_v = is_let_payload<std::remove_cvref_t<T>>::value;

template <class T>
class suppose_box {
public:
  using value_type = T;

  constexpr explicit suppose_box(T value) : value_(std::move(value)) {}

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
    return suppose_box<std::decay_t<std::invoke_result_t<F&, T&>>>(
        std::invoke(std::forward<F>(fn), value_));
  }

  template <class F>
  constexpr auto then(F&& fn) const& {
    return suppose_box<std::decay_t<std::invoke_result_t<F&, const T&>>>(
        std::invoke(std::forward<F>(fn), value_));
  }

  template <class F>
  constexpr auto then(F&& fn) && {
    return suppose_box<std::decay_t<std::invoke_result_t<F&, T&&>>>(
        std::invoke(std::forward<F>(fn), std::move(value_)));
  }

  template <class F>
  constexpr auto transform(F&& fn) & { return then(std::forward<F>(fn)); }

  template <class F>
  constexpr auto transform(F&& fn) const& { return then(std::forward<F>(fn)); }

  template <class F>
  constexpr auto transform(F&& fn) && { return std::move(*this).then(std::forward<F>(fn)); }

  template <class F>
  constexpr suppose_box& tap(F&& fn) & {
    std::invoke(std::forward<F>(fn), value_);
    return *this;
  }

  template <class F>
  constexpr const suppose_box& tap(F&& fn) const& {
    std::invoke(std::forward<F>(fn), value_);
    return *this;
  }

  template <class F>
  constexpr suppose_box&& tap(F&& fn) && {
    std::invoke(std::forward<F>(fn), value_);
    return std::move(*this);
  }

  template <class U>
  constexpr auto into() const& {
    static_assert(std::is_constructible_v<U, const T&>,
                  "suppose::into<U>: U must be constructible from const T&");
    return suppose_box<U>(U(value_));
  }

  template <class U>
  constexpr auto into() && {
    static_assert(std::is_constructible_v<U, T&&>,
                  "suppose::into<U>: U must be constructible from T&&");
    return suppose_box<U>(U(std::move(value_)));
  }

private:
  T value_;
};

} // namespace detail

template <class T>
concept let_payload = detail::is_let_payload_v<T>;

template <let_payload T>
class let : public std::remove_cvref_t<T> {
public:
  using value_type = std::remove_cvref_t<T>;
  using base_type = value_type;
  using base_type::base_type;

  constexpr let(const base_type& value) : base_type(value) {}
  constexpr let(base_type&& value) noexcept(std::is_nothrow_move_constructible_v<base_type>)
      : base_type(std::move(value)) {}

  constexpr let(const let&) = default;
  constexpr let(let&&) noexcept = default;
  constexpr let& operator=(const let&) = default;
  constexpr let& operator=(let&&) noexcept = default;

  constexpr let& operator=(const base_type& value)
  requires std::is_copy_assignable_v<base_type> {
    static_cast<base_type&>(*this) = value;
    return *this;
  }

  constexpr let& operator=(base_type&& value)
  requires std::is_move_assignable_v<base_type> {
    static_cast<base_type&>(*this) = std::move(value);
    return *this;
  }

  constexpr base_type& value() & noexcept { return static_cast<base_type&>(*this); }
  constexpr const base_type& value() const& noexcept { return static_cast<const base_type&>(*this); }
  constexpr base_type&& value() && noexcept { return static_cast<base_type&&>(*this); }
  constexpr const base_type&& value() const&& noexcept { return static_cast<const base_type&&>(*this); }

  constexpr base_type& unwrap() & noexcept { return value(); }
  constexpr const base_type& unwrap() const& noexcept { return value(); }
  constexpr base_type&& unwrap() && noexcept { return std::move(value()); }
  constexpr const base_type&& unwrap() const&& noexcept { return std::move(value()); }
};

template <class T>
requires let_payload<T>
let(T) -> let<std::remove_cvref_t<T>>;

struct suppose_t {
  template <class T>
  constexpr auto operator()(T&& value) const {
    return detail::suppose_box<std::decay_t<T>>(std::forward<T>(value));
  }

  template <class To, class From>
  constexpr auto as(From&& value) const {
    static_assert(std::is_constructible_v<To, From&&>,
                  "suppose.as<To>(value): To must be constructible from value");
    return detail::suppose_box<To>(To(std::forward<From>(value)));
  }
};

inline constexpr suppose_t suppose{};

} // namespace pluss::tils
