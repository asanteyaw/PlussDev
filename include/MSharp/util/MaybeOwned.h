#pragma once

#include <utility>

namespace MSharp {

template <typename T>
struct MaybeOwnedTraits;

template <typename T>
class MaybeOwned {
 public:
  MaybeOwned() : ptr_(nullptr) {}
  MaybeOwned(T* p) : ptr_(p) {}
  T* get() const { return ptr_; }
  T* operator->() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }
 private:
  T* ptr_;
};

} // namespace MSharp
