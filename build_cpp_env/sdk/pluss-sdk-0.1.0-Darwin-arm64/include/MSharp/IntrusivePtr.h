#pragma once

// Torch-inspired intrusive_ptr (modernized for C++20).
//
// This provides a small, production-grade subset of c10::intrusive_ptr
// semantics while reusing the existing RefCounted base used by SnapPtr.
//
// Supported:
//   - default/null construction
//   - copy/move, reset(), release()
//   - get(), operator->, operator*, bool
//   - use_count(), unique()
//   - raw::DontIncreaseRefcount tag for unsafe borrowing
//   - make_intrusive<T>(...)

#include <cstddef>
#include <type_traits>
#include <utility>

#include "MSharp/SnapPtr.h" // RefCounted

namespace pluss::msharp {

namespace raw {
struct DontIncreaseRefcount {
  explicit DontIncreaseRefcount() = default;
};
} // namespace raw

template <class T>
class intrusive_ptr {
public:
  using element_type = T;

  intrusive_ptr() noexcept = default;
  intrusive_ptr(std::nullptr_t) noexcept : p_(nullptr) {}

  // Adopt a newly created object whose refcount is already 1.
  struct adopt_t {
    explicit adopt_t() = default;
  };
  static constexpr adopt_t adopt{};

  intrusive_ptr(adopt_t, T* p) noexcept : p_(p) {}

  // Construct from raw pointer; retains by default.
  explicit intrusive_ptr(T* p, bool retain = true) : p_(p) {
    if (p_ && retain) retain_();
  }

  // Unsafe borrow: do not increase refcount.
  intrusive_ptr(T* p, raw::DontIncreaseRefcount) noexcept : p_(p) {}

  intrusive_ptr(const intrusive_ptr& o) noexcept : p_(o.p_) {
    if (p_) retain_();
  }

  intrusive_ptr(intrusive_ptr&& o) noexcept : p_(o.p_) {
    o.p_ = nullptr;
  }

  intrusive_ptr& operator=(const intrusive_ptr& o) noexcept {
    if (this == &o) return *this;
    reset(o.p_);
    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr&& o) noexcept {
    if (this == &o) return *this;
    if (p_) release_();
    p_ = o.p_;
    o.p_ = nullptr;
    return *this;
  }

  ~intrusive_ptr() {
    if (p_) release_();
  }

  T* get() const noexcept { return p_; }
  T* operator->() const noexcept { return p_; }
  T& operator*() const noexcept { return *p_; }
  explicit operator bool() const noexcept { return p_ != nullptr; }

  void reset() noexcept {
    if (p_) release_();
    p_ = nullptr;
  }

  void reset(T* p, bool retain = true) {
    if (p == p_) return;
    if (p_) release_();
    p_ = p;
    if (p_ && retain) retain_();
  }

  // Release ownership without decrementing refcount.
  T* release() noexcept {
    T* out = p_;
    p_ = nullptr;
    return out;
  }

  std::size_t use_count() const noexcept {
    return p_ ? static_cast<const RefCounted*>(p_)->use_count() : 0;
  }
  bool unique() const noexcept { return use_count() == 1; }

private:
  void retain_() const { static_cast<RefCounted*>(p_)->retain(); }
  void release_() const { static_cast<RefCounted*>(p_)->release(); }

  T* p_ = nullptr;
};

template <class T, class... A>
intrusive_ptr<T> make_intrusive(A&&... a) {
  return intrusive_ptr<T>(intrusive_ptr<T>::adopt, new T(std::forward<A>(a)...));
}

} // namespace pluss::msharp
