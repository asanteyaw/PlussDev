#pragma once

#include <cstddef>
#include <utility>

namespace MSharp {

template <typename T>
class intrusive_ptr {
 public:
  intrusive_ptr() : ptr_(nullptr) {}
  intrusive_ptr(std::nullptr_t) : ptr_(nullptr) {}
  intrusive_ptr(T* p) : ptr_(p) {}
  T* get() const { return ptr_; }
  T* release() {
    T* p = ptr_;
    ptr_ = nullptr;
    return p;
  }
  T* operator->() const { return ptr_; }
  operator bool() const { return ptr_ != nullptr; }
  size_t use_count() const { return 1; }
  bool unique() const { return true; }
  static intrusive_ptr<T> reclaim(T* p) { return intrusive_ptr<T>(p); }
 private:
  T* ptr_;
};

template <typename T, typename... Args>
intrusive_ptr<T> make_intrusive(Args&&... args) {
  return intrusive_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
class weak_intrusive_ptr {
 public:
  weak_intrusive_ptr() : ptr_(nullptr) {}
  template <typename U>
  weak_intrusive_ptr(const intrusive_ptr<U>& p) : ptr_(p.get()) {}
  T* get() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }
 private:
  T* ptr_;
};

} // namespace MSharp
