#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace pluss::msharp {

//------------------------------------------------------------------------------
// RefCounted
//------------------------------------------------------------------------------
// Intrusive reference-counting base with thread-safe atomic refcount.
//
// Design goals:
// - low overhead: a single atomic counter stored in the object
// - deterministic lifetime: final release deletes the object
// - safe by default: acquire/release memory ordering appropriate for shared ownership
//
// Notes:
// - Derive your shared internal objects (e.g. SeriesImpl, StorageImpl) from RefCounted.
// - Objects start with refcount=1.
// - SnapPtr<T> calls retain/release automatically.
//------------------------------------------------------------------------------
class RefCounted {
public:
  RefCounted() noexcept = default;
  RefCounted(const RefCounted&) = delete;
  RefCounted& operator=(const RefCounted&) = delete;
  virtual ~RefCounted() = default;

  void retain() const noexcept {
    // Relaxed is sufficient for increment; ownership synchronization happens on release.
    refcount_.fetch_add(1, std::memory_order_relaxed);
  }

  void release() const noexcept {
    // Decrement with release; if this is the last reference, acquire to synchronize
    // prior writes from other threads before destruction.
    if (refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete this;
    }
  }

  std::size_t use_count() const noexcept {
    return refcount_.load(std::memory_order_relaxed);
  }

  bool unique() const noexcept { return use_count() == 1; }

protected:
  // Allow derived classes to customize destruction if desired (rare).
  RefCounted(std::uint32_t initial) noexcept : refcount_(initial) {}

private:
  mutable std::atomic<std::uint32_t> refcount_{1};
};

//------------------------------------------------------------------------------
// SnapPtr
//------------------------------------------------------------------------------
// Intrusive shared smart pointer (named SnapPtr to avoid "intrusive_ptr" naming).
//
// Production-grade behaviors:
// - thread-safe refcounting (atomic)
// - strong exception safety (no leaks; operations are noexcept where practical)
// - explicit adopt semantics (wrap a freshly-created object without extra retain)
// - casting helpers
//
// SnapPtr requires: T derives from RefCounted.
//------------------------------------------------------------------------------
struct adopt_snap_t {
  explicit adopt_snap_t() = default;
};
inline constexpr adopt_snap_t adopt_snap{};

template <class T>
class SnapPtr {
  static_assert(std::is_base_of_v<RefCounted, T>, "SnapPtr<T> requires T : RefCounted");

public:
  using element_type = T;

  // constructors
  SnapPtr() noexcept = default;
  SnapPtr(std::nullptr_t) noexcept : ptr_(nullptr) {}

  // Wrap raw pointer; retain by default (for adopting, use (adopt_snap, p)).
  explicit SnapPtr(T* p, bool retain = true) noexcept : ptr_(p) {
    if (ptr_ && retain) ptr_->retain();
  }

  // Adopt a pointer whose refcount already includes this reference (no retain).
  SnapPtr(adopt_snap_t, T* p) noexcept : ptr_(p) {}

  SnapPtr(const SnapPtr& o) noexcept : ptr_(o.ptr_) {
    if (ptr_) ptr_->retain();
  }

  template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  SnapPtr(const SnapPtr<U>& o) noexcept : ptr_(o.get()) {
    if (ptr_) ptr_->retain();
  }

  SnapPtr(SnapPtr&& o) noexcept : ptr_(o.ptr_) { o.ptr_ = nullptr; }

  template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  SnapPtr(SnapPtr<U>&& o) noexcept : ptr_(o.get()) {
    o.reset(nullptr, /*retain=*/false);
  }

  ~SnapPtr() { if (ptr_) ptr_->release(); }

  // assignment
  SnapPtr& operator=(const SnapPtr& o) noexcept {
    if (this == &o) return *this;
    reset(o.ptr_, /*retain=*/true);
    return *this;
  }

  SnapPtr& operator=(SnapPtr&& o) noexcept {
    if (this == &o) return *this;
    if (ptr_) ptr_->release();
    ptr_ = o.ptr_;
    o.ptr_ = nullptr;
    return *this;
  }

  void reset(T* p = nullptr, bool retain = true) noexcept {
    if (ptr_ == p) {
      // nothing to do; still honor retain=false by not touching refcount
      return;
    }
    if (p && retain) p->retain();
    if (ptr_) ptr_->release();
    ptr_ = p;
  }

  void swap(SnapPtr& o) noexcept {
    T* tmp = ptr_;
    ptr_ = o.ptr_;
    o.ptr_ = tmp;
  }

  // observers
  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  std::size_t use_count() const noexcept { return ptr_ ? ptr_->use_count() : 0; }
  bool unique() const noexcept { return ptr_ ? ptr_->unique() : false; }

  // release raw pointer without decrementing (caller assumes ownership of a reference)
  T* release_ptr() noexcept {
    T* p = ptr_;
    ptr_ = nullptr;
    return p;
  }

private:
  template <class U>
  friend class SnapPtr;
  T* ptr_{nullptr};
};

// comparisons
template <class T, class U>
bool operator==(const SnapPtr<T>& a, const SnapPtr<U>& b) noexcept { return a.get() == b.get(); }
template <class T, class U>
bool operator!=(const SnapPtr<T>& a, const SnapPtr<U>& b) noexcept { return a.get() != b.get(); }

// casts
template <class T, class U>
SnapPtr<T> static_snap_cast(const SnapPtr<U>& p) noexcept {
  return SnapPtr<T>(static_cast<T*>(p.get()), /*retain=*/true);
}
template <class T, class U>
SnapPtr<T> dynamic_snap_cast(const SnapPtr<U>& p) noexcept {
  if (auto* q = dynamic_cast<T*>(p.get())) return SnapPtr<T>(q, /*retain=*/true);
  return SnapPtr<T>{};
}

// factory: allocate and adopt without extra retain
template <class T, class... Args>
SnapPtr<T> make_snap(Args&&... args) {
  return SnapPtr<T>(adopt_snap, new T(std::forward<Args>(args)...));
}

} // namespace pluss::msharp
