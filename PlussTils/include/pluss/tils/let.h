#pragma once

// `let` -- an immutable binding for Pluss core value types, deliberately opt-in.
//
//   #include <pluss/tils/let.h>
//   let s1 = pluss::make_series({1.0, 2.0, 3.0});   // CTAD -> let<Series>, bare name
//   let n1 = pluss::note::make({"a", "b"});         // let<Note>
//
// This header is NOT pulled in by <pluss/tils.h>; including it is how you ask
// for `let`. It contributes exactly one name to the global namespace -- `let`,
// as a using-declaration for `pluss::tils::let` -- so the bare spelling works
// with no `using` on your side. Code that wants to avoid the global name simply
// does not include this header and writes `pluss::tils::let<T>` explicitly.
//
// Semantics (vs plain `auto`):
//   * The binding cannot be rebound: `s1 = other;` does not compile.
//   * Access to the payload is const-only (`get()` / `*` / `->` yield `const T`).
//     Note: Pluss core types follow Torch handle semantics -- their in-place
//     `_`-suffixed ops are `const` methods -- so `let` stops rebinding and
//     mutable-reference escape, but not `s1->mul_(x)` on the type's own const
//     mutators. Take an independent copy first (`s1.get().clone()`) if that
//     matters. `std::move(s1).unwrap()` moves the payload out and spends the
//     binding.
//   * Transparent where a `const T&` (or a by-value `T`) is expected, via an
//     implicit conversion, so `f(s1)` still works for `f(const Series&)` /
//     `f(Series)` but not for `f(Series&)`.
//   * `s1.map(fn)` stays in `let` when `fn`'s result is itself a payload type,
//     otherwise yields the bare value.
//
// Payloads are cheap handles (shared impl), so `let` always owns its value; a
// copy is a refcount bump, not a deep copy.

#include <functional>
#include <type_traits>
#include <utility>

#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"
#include "pluss/queue.h"

namespace pluss::tils {

// ---- extension point -------------------------------------------------------
// Opt a type in by specializing this to `true`. No need to open any `detail`
// namespace; downstream code can add its own specializations.
template <class T>
inline constexpr bool enable_let = false;

template <> inline constexpr bool enable_let<::pluss::Series> = true;
template <> inline constexpr bool enable_let<::pluss::Note> = true;
template <> inline constexpr bool enable_let<::pluss::codeh::Timeline> = true;
template <> inline constexpr bool enable_let<::pluss::codeh::Duration> = true;
template <> inline constexpr bool enable_let<::pluss::codev::Queue> = true;

template <class T>
concept let_payload =
    std::is_object_v<std::remove_cvref_t<T>> && enable_let<std::remove_cvref_t<T>>;

// ---- let ------------------------------------------------------------------
template <let_payload T>
class let {
  using V = std::remove_cvref_t<T>;
  V v_;

 public:
  using value_type = V;

  // Bind by copying or moving the payload in.
  let(const V& value) : v_(value) {}
  let(V&& value) noexcept(std::is_nothrow_move_constructible_v<V>)
      : v_(std::move(value)) {}

  // A `let` is copyable/movable as a whole (binding a fresh `let` is fine) ...
  let(const let&) = default;
  let(let&&) noexcept = default;
  ~let() = default;

  // ... but the binding itself can never be reassigned.
  let& operator=(const let&) = delete;
  let& operator=(let&&) = delete;

  // Read-only access to the payload.
  const V& get() const noexcept { return v_; }
  const V& operator*() const noexcept { return v_; }
  const V* operator->() const noexcept { return &v_; }
  operator const V&() const noexcept { return v_; }  // NOLINT(google-explicit-constructor)

  // Consume the binding: move the payload out. After this the `let` is spent.
  [[nodiscard]] V unwrap() && noexcept(std::is_nothrow_move_constructible_v<V>) {
    return std::move(v_);
  }
  // For a mutable / independent payload, go through `get()` explicitly and use
  // the payload's own facilities, e.g. `x.get().clone()` for a deep copy vs
  // `x.get()` for a shared-storage handle. `let` does not guess which you want.

  // Pipeline: stay in `let` when the result is itself a payload, else drop to
  // the bare value.
  template <class F>
  [[nodiscard]] auto map(F&& fn) const& {
    using U = std::remove_cvref_t<std::invoke_result_t<F&&, const V&>>;
    if constexpr (let_payload<U>) {
      return let<U>(std::invoke(std::forward<F>(fn), v_));
    } else {
      return std::invoke(std::forward<F>(fn), v_);
    }
  }

  template <class F>
  const let& peek(F&& fn) const& {
    std::invoke(std::forward<F>(fn), v_);
    return *this;
  }
};

// CTAD: `let x = payload_expr;` -> `let<PayloadType>`. Copying a `let` uses the
// implicit copy/move deduction candidate, so there is no `let<let<T>>`.
template <class T>
  requires let_payload<T>
let(T) -> let<std::remove_cvref_t<T>>;

}  // namespace pluss::tils

// The one global-scope name this header contributes. Including <pluss/tils/let.h>
// *is* the opt-in; nothing else pulls it in.
using ::pluss::tils::let;
