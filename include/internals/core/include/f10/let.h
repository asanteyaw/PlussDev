#pragma once

// j2/let.h
//
// "let" as an "auto 2.0" *library* facility.
//
// Why this exists:
// - Standard C++ cannot introduce new keywords from a library.
// - A macro `#define let auto` is *only* syntactic sugar.
// - To make `let` meaningfully "more powerful" than `auto` while keeping the
//   surface syntax `let x = expr;`, we model `let` as a *deduction class*.
//
// What you get:
// 1) `let x = expr;` works via CTAD (class template argument deduction).
// 2) `let` binds to lvalues by reference by default (no accidental copies):
//      int a = 3; let x = a;   // x refers to a
// 3) `let` stores rvalues by value (safe):
//      let y = std::string("hi"); // y owns the string
// 4) Introspection:
//      x.value_type, x.is_ref, x.type_name(), x.type_id()
// 5) Transparent-ish usage:
//      - implicit conversion operators to the underlying value/reference
//      - operator* / operator->
//
// Trade-offs vs language `auto`:
// - `let` introduces a wrapper type (not identical to the deduced T).
// - You cannot use CTAD for function parameters, so `let` cannot mirror
//   `auto`'s C++20 abbreviated function templates in signatures.
//   (You *can* still use `auto` in signatures; `let` is for declarations.)
//
// If you later want `let` to be usable in signatures *with the same keyword*,
// you need a compiler extension (clang plugin / custom frontend). A pure
// header-only library cannot change the grammar.

#include <cstddef>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <functional>
// #include <new>

namespace j2 {

namespace detail {

// A small helper to provide a stable-ish type name hook.
// Note: typeid(T).name() is implementation-defined, but is often adequate.
// If you want demangling, add an optional platform layer.
template <class T>
constexpr std::string_view type_name_fallback() noexcept {
#if defined(__clang__)
  return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
  return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
  return __FUNCSIG__;
#else
  return "<type_name unavailable>";
#endif
}

} // namespace detail

// Primary template: owns a value (rvalue / prvalue initialization)
template <class T>
class let_t {
public:
  using value_type = T;
  static constexpr bool is_ref = false;

  // Perfect-forward into owned storage.
  template <class U>
  constexpr explicit let_t(U&& u) noexcept(std::is_nothrow_constructible_v<T, U&&>)
      : value_(std::forward<U>(u)) {}

  // Assignment writes through to the owned value (never rebinds).
  // This is intentionally more "value-like" than the compiler-generated operator=.
  template <class U>
  constexpr let_t& operator=(U&& u) noexcept(noexcept(std::declval<T&>() = std::forward<U>(u)))
    requires(std::is_assignable_v<T&, U&&>)
  {
    value_ = std::forward<U>(u);
    return *this;
  }

  // Assign from another let_t by assigning its underlying value.
  constexpr let_t& operator=(let_t const& other) noexcept(noexcept(std::declval<T&>() = other.value_))
    requires(std::is_copy_assignable_v<T>)
  {
    if (this != &other) value_ = other.value_;
    return *this;
  }

  constexpr let_t& operator=(let_t&& other) noexcept(noexcept(std::declval<T&>() = std::move(other.value_)))
    requires(std::is_move_assignable_v<T>)
  {
    if (this != &other) value_ = std::move(other.value_);
    return *this;
  }

  // Access
  constexpr T& get() & noexcept { return value_; }
  constexpr T const& get() const& noexcept { return value_; }
  constexpr T&& get() && noexcept { return std::move(value_); }

  // Transparent-ish usage
  constexpr operator T&() & noexcept { return value_; }
  constexpr operator T const&() const& noexcept { return value_; }

  constexpr T* operator->() noexcept { return &value_; }
  constexpr T const* operator->() const noexcept { return &value_; }
  constexpr T& operator*() noexcept { return value_; }
  constexpr T const& operator*() const noexcept { return value_; }

  // Introspection
  static constexpr std::string_view type_name() noexcept {
    return detail::type_name_fallback<T>();
  }
  static constexpr std::type_info const& type_id() noexcept {
    return typeid(T);
  }

private:
  T value_;
};

// Partial specialization: binds by reference (lvalue initialization)
template <class T>
class let_t<T&> {
public:
  using value_type = T;
  static constexpr bool is_ref = true;

  constexpr explicit let_t(T& t) noexcept : ptr_(std::addressof(t)) {}

  // Assignment writes through to the referred-to object (never rebinds).
  template <class U>
  constexpr let_t& operator=(U&& u) noexcept(noexcept(std::declval<T&>() = std::forward<U>(u)))
    requires(std::is_assignable_v<T&, U&&>)
  {
    *ptr_ = std::forward<U>(u);
    return *this;
  }

  constexpr let_t& operator=(let_t const& other) noexcept(noexcept(std::declval<T&>() = other.get()))
    requires(std::is_copy_assignable_v<T>)
  {
    if (this != &other) *ptr_ = other.get();
    return *this;
  }

  constexpr let_t& operator=(let_t&& other) noexcept(noexcept(std::declval<T&>() = other.get()))
    requires(std::is_move_assignable_v<T>)
  {
    if (this != &other) *ptr_ = other.get();
    return *this;
  }

  constexpr T& get() const noexcept { return *ptr_; }

  // Transparent-ish usage
  constexpr operator T&() const noexcept { return *ptr_; }

  constexpr T* operator->() const noexcept { return ptr_; }
  constexpr T& operator*() const noexcept { return *ptr_; }

  // Introspection
  static constexpr std::string_view type_name() noexcept {
    return detail::type_name_fallback<T>();
  }
  static constexpr std::type_info const& type_id() noexcept {
    return typeid(T);
  }

private:
  T* ptr_;
};

// ---------------------------
// CTAD (deduction guides)
// ---------------------------

// Lvalues bind by reference (avoid copies)
template <class U>
let_t(U&) -> let_t<U&>;

// Const lvalues bind by const reference
template <class U>
let_t(U const&) -> let_t<U const&>;

// Rvalues/prvalues are owned by value
// (we strip references and cv, because we own the value)
template <class U>
let_t(U&&) -> let_t<std::remove_cv_t<std::remove_reference_t<U>>>;

// Const-ref specialization (for completeness)
template <class T>
class let_t<T const&> {
public:
  using value_type = T;
  static constexpr bool is_ref = true;

  constexpr explicit let_t(T const& t) noexcept : ptr_(std::addressof(t)) {}

  // No assignment operator: a let_t<const T&> is a read-only view.

  constexpr T const& get() const noexcept { return *ptr_; }
  constexpr operator T const&() const noexcept { return *ptr_; }

  constexpr T const* operator->() const noexcept { return ptr_; }
  constexpr T const& operator*() const noexcept { return *ptr_; }

  static constexpr std::string_view type_name() noexcept {
    return detail::type_name_fallback<T>();
  }
  static constexpr std::type_info const& type_id() noexcept {
    return typeid(T);
  }

private:
  T const* ptr_;
};

// A convenience alias if you want the deduced underlying type.
// Example: using T = j2::let_underlying_t<decltype(x)>;
template <class Let>
using let_underlying_t = typename std::remove_reference_t<Let>::value_type;

// ---------------------------
// Utilities
// ---------------------------

// unwrap(x): get a reference to the underlying value.
// Useful because template argument deduction does NOT consider user-defined conversions.
// So passing a `let_t` object to a template would deduce T = let_t<...> instead of the wrapped type.
// Use unwrap(x) at call sites when you want the underlying type to participate in deduction.

template <class T>
constexpr T& unwrap(let_t<T>& x) noexcept { return x.get(); }

template <class T>
constexpr T const& unwrap(let_t<T> const& x) noexcept { return x.get(); }

template <class T>
constexpr T unwrap(let_t<T>&& x) noexcept(std::is_nothrow_move_constructible_v<T>) {
  return std::move(x).get();
}

template <class T>
constexpr T& unwrap(let_t<T&> x) noexcept { return x.get(); }

template <class T>
constexpr T const& unwrap(let_t<T const&> x) noexcept { return x.get(); }

// Convenience: detect whether a type is a j2::let_t wrapper.
template <class X>
struct is_let : std::false_type { };

template <class T>
struct is_let<let_t<T>> : std::true_type { };

template <class X>
inline constexpr bool is_let_v = is_let<std::remove_cv_t<std::remove_reference_t<X>>>::value;

// Comparison operators forward to the underlying value.
// These are constrained so they only participate when the underlying op exists.

template <class A, class B>
constexpr bool operator==(let_t<A> const& a, let_t<B> const& b)
  requires requires { unwrap(a) == unwrap(b); }
{ return unwrap(a) == unwrap(b); }

template <class A, class B>
constexpr bool operator!=(let_t<A> const& a, let_t<B> const& b)
  requires requires { unwrap(a) != unwrap(b); }
{ return unwrap(a) != unwrap(b); }

template <class A, class B>
constexpr bool operator<(let_t<A> const& a, let_t<B> const& b)
  requires requires { unwrap(a) < unwrap(b); }
{ return unwrap(a) < unwrap(b); }

template <class A, class B>
constexpr bool operator<=(let_t<A> const& a, let_t<B> const& b)
  requires requires { unwrap(a) <= unwrap(b); }
{ return unwrap(a) <= unwrap(b); }

template <class A, class B>
constexpr bool operator>(let_t<A> const& a, let_t<B> const& b)
  requires requires { unwrap(a) > unwrap(b); }
{ return unwrap(a) > unwrap(b); }

template <class A, class B>
constexpr bool operator>=(let_t<A> const& a, let_t<B> const& b)
  requires requires { unwrap(a) >= unwrap(b); }
{ return unwrap(a) >= unwrap(b); }

// swap for owned let_t<T>
template <class T>
constexpr void swap(let_t<T>& a, let_t<T>& b) noexcept(noexcept(std::swap(unwrap(a), unwrap(b))))
  requires(!let_t<T>::is_ref && std::is_swappable_v<T>)
{
  using std::swap;
  swap(unwrap(a), unwrap(b));
}

} // namespace j2

// ---------------------------
// Global "let" token and mode switch
// ---------------------------
//
// J2_LET_MODE controls the meaning of the "let" macro:
//   - J2_LET_MODE == 1: let is just an alias for `auto`
//   - J2_LET_MODE == 2: let uses the ::j2::let_t wrapper (default, reference-friendly)
//   - Any other value: triggers a compile error.
//
// You can override J2_LET_MODE before including this header if you want "let" to mean "auto".
// Example:
//   #define J2_LET_MODE 1
//   #include <j2/let.h>
//
// This macro makes the *spelling* match desired syntax:
//     let x = expr;
//
// Note: this is still a macro (C++ cannot add keywords from a library), but the
// semantics are *not* mere sugar in mode 2: the deduced type is a richer wrapper that
// preserves references-by-default and provides introspection.

#ifndef J2_LET_MODE
#  define J2_LET_MODE 1
#endif

#ifndef let
#  if   J2_LET_MODE == 1
#    define let auto
#  elif J2_LET_MODE == 2
#    define let ::j2::let_t
#  else
#    error "J2_LET_MODE must be 1 (auto) or 2 (wrapper)"
#  endif
#endif

// LET(x): pass through non-let values, unwrap j2::let_t wrappers at call sites.
//   - Use LET(x) when calling templates/functions that should see the underlying type.
//   - LET(x) is a macro, so it works for both lvalues and rvalues.
#define LET(x) ::j2::detail::let_passthrough((x))

namespace std {

template <class T>
struct hash<j2::let_t<T>> {
  size_t operator()(j2::let_t<T> const& x) const noexcept(noexcept(std::hash<T>{}(j2::unwrap(x))))
    requires(!j2::let_t<T>::is_ref && requires { std::hash<T>{}(j2::unwrap(x)); })
  {
    return std::hash<T>{}(j2::unwrap(x));
  }
};

} // namespace std

// ---------------------------
// LET(x) implementation helpers
// ---------------------------
namespace j2::detail {
// Pass through non-let types unchanged.
template<class T> constexpr T&& let_passthrough(T&& x) noexcept { return std::forward<T>(x); }
// Unwrap let_t<T> wrappers.
template<class T> constexpr T& let_passthrough(::j2::let_t<T>& x) noexcept { return ::j2::unwrap(x); }
template<class T> constexpr T const& let_passthrough(::j2::let_t<T> const& x) noexcept { return ::j2::unwrap(x); }
template<class T> constexpr T let_passthrough(::j2::let_t<T>&& x) noexcept(std::is_nothrow_move_constructible_v<T>) { return ::j2::unwrap(std::move(x)); }
template<class T> constexpr T& let_passthrough(::j2::let_t<T&> x) noexcept { return ::j2::unwrap(x); }
template<class T> constexpr T const& let_passthrough(::j2::let_t<T const&> x) noexcept { return ::j2::unwrap(x); }
} // namespace j2::detail