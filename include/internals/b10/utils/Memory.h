// // pluss/Memory.h
// #pragma once

// #include <atomic>
// #include <cstdint>
// #include <stdexcept>
// #include <utility>

// namespace pluss {

// #define PLUSS_INTERNAL_ASSERT(cond, msg)                          \
//     do {                                                         \
//         if (!(cond)) throw std::runtime_error(msg);              \
//     } while (0)

// // Base class for all intrusive_ptr-managed data
// class IntrusiveTarget {
//     mutable std::atomic<uint32_t> refcount_;

// public:
//     IntrusiveTarget() noexcept : refcount_(0) {}

//     void incref() const noexcept {
//         refcount_.fetch_add(1, std::memory_order_acq_rel);
//     }

//     void decref() const noexcept {
//         if (refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
//             delete this;
//     }

//     uint32_t use_count() const noexcept {
//         return refcount_.load(std::memory_order_acquire);
//     }

// protected:
//     virtual ~IntrusiveTarget() = default; // polymorphic base
// };

// // Smart pointer
// template <typename T>
// class intrusive_ptr {
//     T* ptr_;

// public:
//     intrusive_ptr() noexcept : ptr_(nullptr) {}
//     intrusive_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}

//     explicit intrusive_ptr(T* raw, bool add_ref = true) : ptr_(raw) {
//         if (ptr_ && add_ref) ptr_->incref();
//     }

//     intrusive_ptr(const intrusive_ptr& other) : ptr_(other.ptr_) {
//         if (ptr_) ptr_->incref();
//     }

//     intrusive_ptr(intrusive_ptr&& other) noexcept : ptr_(other.ptr_) {
//         other.ptr_ = nullptr;
//     }

//     ~intrusive_ptr() {
//         if (ptr_) ptr_->decref();
//     }

//     intrusive_ptr& operator=(const intrusive_ptr& other) {
//         if (this != &other) {
//             if (ptr_) ptr_->decref();
//             ptr_ = other.ptr_;
//             if (ptr_) ptr_->incref();
//         }
//         return *this;
//     }

//     intrusive_ptr& operator=(intrusive_ptr&& other) noexcept {
//         if (this != &other) {
//             if (ptr_) ptr_->decref();
//             ptr_ = other.ptr_;
//             other.ptr_ = nullptr;
//         }
//         return *this;
//     }

//     T* get() const noexcept { return ptr_; }
//     T& operator*() const noexcept { return *ptr_; }
//     T* operator->() const noexcept { return ptr_; }
//     explicit operator bool() const noexcept { return ptr_ != nullptr; }

//     void reset() {
//         if (ptr_) ptr_->decref();
//         ptr_ = nullptr;
//     }

//     uint32_t use_count() const noexcept {
//         return ptr_ ? ptr_->use_count() : 0;
//     }

//     bool unique() const noexcept { return use_count() == 1; }

//     static intrusive_ptr<T> reclaim(T* raw) noexcept {
//         intrusive_ptr<T> tmp;
//         tmp.ptr_ = raw;
//         return tmp;
//     }
// };

// } // namespace pluss

#pragma once

#include <atomic>
#include <utility>
#include <cstddef>
// #include <memory>
#include <cstdint>

namespace pluss {

struct intrusive_ptr_target {
  mutable std::atomic<uint32_t> refcount_{0};
  mutable std::atomic<uint32_t> weakcount_{1}; // starts at 1 to keep object alive during weak ptrs

  intrusive_ptr_target() noexcept = default;
  virtual ~intrusive_ptr_target() = default;
};

// === intrusive_ptr ===

template <typename T>
class intrusive_ptr {
public:
  using element_type = T;

  intrusive_ptr() noexcept : ptr_(nullptr) {}
  intrusive_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}

  explicit intrusive_ptr(T* raw, bool add_ref = true) noexcept : ptr_(raw) {
    if (ptr_) {
      if (add_ref) {
        incref(ptr_);
      } else {
        // Ensure refcount is at least 1 for objects that aren't explicitly ref-counted
        if (ptr_->refcount_.load(std::memory_order_acquire) == 0) {
          ptr_->refcount_.store(1, std::memory_order_release);
        }
      }
    }
  }

  intrusive_ptr(const intrusive_ptr& other) noexcept : ptr_(other.ptr_) {
    if (ptr_) incref(ptr_);
  }

  intrusive_ptr(intrusive_ptr&& other) noexcept : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }

  intrusive_ptr& operator=(const intrusive_ptr& other) noexcept {
    if (this != &other) {
      reset();
      ptr_ = other.ptr_;
      if (ptr_) incref(ptr_);
    }
    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr&& other) noexcept {
    if (this != &other) {
      reset();
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
    }
    return *this;
  }

  ~intrusive_ptr() { reset(); }

  void reset() noexcept {
    T* tmp = ptr_;
    ptr_ = nullptr;  // Set to nullptr first to avoid potential recursive destruction
    
    if (tmp && decref(tmp)) {
      // Only delete if refcount reached zero
      if (tmp->weakcount_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete tmp;
      }
    }
  }

  T* get() const noexcept { return ptr_; }
  T* operator->() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  uint32_t use_count() const noexcept {
    return ptr_ ? ptr_->refcount_.load(std::memory_order_acquire) : 0;
  }

  bool unique() const noexcept { return use_count() == 1; }

  void swap(intrusive_ptr& other) noexcept {
    std::swap(ptr_, other.ptr_);
  }

  static intrusive_ptr reclaim(T* raw) noexcept {
    return intrusive_ptr(raw, false);
  }

  T* release() noexcept {
    T* tmp = ptr_;
    ptr_ = nullptr;
    return tmp;
  }

  template <typename... Args>
  static intrusive_ptr make(Args&&... args) {
    T* obj = new T(std::forward<Args>(args)...);
    // Initialize refcount to 1 since we're creating a new object
    obj->refcount_.store(1, std::memory_order_release);
    return intrusive_ptr(obj, false);
  }

private:
  T* ptr_;

  static void incref(T* obj) {
    obj->refcount_.fetch_add(1, std::memory_order_acq_rel);
  }

  static bool decref(T* obj) {
    return obj->refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1;
  }

  template <typename U> friend class weak_intrusive_ptr;
};

// === weak_intrusive_ptr ===

template <typename T>
class weak_intrusive_ptr {
public:
  weak_intrusive_ptr() noexcept : ptr_(nullptr) {}
  weak_intrusive_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}

  explicit weak_intrusive_ptr(const intrusive_ptr<T>& strong) noexcept : ptr_(strong.ptr_) {
    if (ptr_) {
      // Ensure the object has a valid refcount before incrementing weakcount
      if (ptr_->refcount_.load(std::memory_order_acquire) > 0) {
        incref(ptr_);
      } else {
        ptr_ = nullptr; // Don't reference an object with no strong references
      }
    }
  }

  weak_intrusive_ptr(const weak_intrusive_ptr& other) noexcept : ptr_(other.ptr_) {
    if (ptr_) incref(ptr_);
  }

  weak_intrusive_ptr(weak_intrusive_ptr&& other) noexcept : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }

  weak_intrusive_ptr& operator=(const weak_intrusive_ptr& other) noexcept {
    if (this != &other) {
      reset();
      ptr_ = other.ptr_;
      if (ptr_) incref(ptr_);
    }
    return *this;
  }

  weak_intrusive_ptr& operator=(weak_intrusive_ptr&& other) noexcept {
    if (this != &other) {
      reset();
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
    }
    return *this;
  }

  ~weak_intrusive_ptr() { reset(); }

  void reset() noexcept {
    T* tmp = ptr_;
    ptr_ = nullptr;  // Set to nullptr first to avoid potential recursive destruction
    
    if (tmp && decref(tmp)) {
      delete tmp;
    }
  }

  intrusive_ptr<T> lock() const noexcept {
    if (!ptr_) return {};
    auto count = ptr_->refcount_.load(std::memory_order_acquire);
    while (count != 0) {
      if (ptr_->refcount_.compare_exchange_weak(count, count + 1))
        return intrusive_ptr<T>(ptr_, false);
    }
    return {};
  }

  uint32_t use_count() const noexcept {
    return ptr_ ? ptr_->refcount_.load(std::memory_order_acquire) : 0;
  }

  bool expired() const noexcept {
    return use_count() == 0;
  }

private:
  T* ptr_;

  static void incref(T* obj) {
    obj->weakcount_.fetch_add(1, std::memory_order_acq_rel);
  }

  static bool decref(T* obj) {
    return obj->weakcount_.fetch_sub(1, std::memory_order_acq_rel) == 1;
  }
};

// === Swap Support ===
template <typename T>
inline void swap(intrusive_ptr<T>& lhs, intrusive_ptr<T>& rhs) noexcept {
  lhs.swap(rhs);
}

template <typename T>
inline void swap(weak_intrusive_ptr<T>& lhs, weak_intrusive_ptr<T>& rhs) noexcept {
  weak_intrusive_ptr<T> tmp = std::move(lhs);
  lhs = std::move(rhs);
  rhs = std::move(tmp);
}

// === Hash Support ===
} // namespace pluss

namespace std {
template <typename T>
struct hash<pluss::intrusive_ptr<T>> {
  size_t operator()(const pluss::intrusive_ptr<T>& p) const noexcept {
    return std::hash<T*>()(p.get());
  }
};

// Removed hash specialization for weak_intrusive_ptr since it can't access private members
// and it's not needed for our current implementation
} // namespace std
