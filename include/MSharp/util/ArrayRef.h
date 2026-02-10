#pragma once

#include <cstddef>
#include <vector>

template <typename T>
class ArrayRef {
 public:
  using value_type = T;
  ArrayRef() noexcept : data_(nullptr), size_(0) {}
  ArrayRef(const std::vector<T>& v) noexcept : data_(v.data()), size_(v.size()) {}
  ArrayRef(const T* data, size_t size) noexcept : data_(data), size_(size) {}

  size_t size() const noexcept { return size_; }
  const T& operator[](size_t i) const noexcept { return data_[i]; }
  const T* data() const noexcept { return data_; }
  const T* begin() const noexcept { return data_; }
  const T* end() const noexcept { return data_ + size_; }

 private:
  const T* data_;
  size_t size_;
};

using IntArrayRef = ArrayRef<int64_t>;
