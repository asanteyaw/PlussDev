#pragma once

// Read-only access to a Series' values as a contiguous array of T, without a
// copy when the data already is one (CPU, contiguous, the wanted dtype) and with
// one conversion/copy otherwise. Hot loops (reductions, join key encoding) use
// this instead of owning Series::host exports so large columns are not duplicated.
//
// The view keeps the Series alive; the pointer is valid for the view's lifetime.
// Do not edit the Series in place while a view of it is being read.

#include <cstddef>
#include <type_traits>
#include <vector>

#include "table/Slate.h"

namespace pluss::kindle {

template <class T>
class ColumnView {
  static_assert(!std::is_same_v<T, bool>, "ColumnView: bool columns are not exposed as arrays");

public:
  ColumnView() = default;

  ColumnView(const pluss::Series& series, pluss::codem::ScalarType want) {
    if (series.is_cpu() && series.scalar_type() == want && series.is_contiguous()) {
      keep_ = series;
      ptr_ = reinterpret_cast<const T*>(series.raw_data());
      n_ = static_cast<std::size_t>(series.numel());
      return;
    }
    owned_ = series.to(want).template host<T>();
    ptr_ = owned_.data();
    n_ = owned_.size();
  }

  ColumnView(const ColumnView&) = delete;
  ColumnView& operator=(const ColumnView&) = delete;
  ColumnView(ColumnView&& other) noexcept { *this = std::move(other); }
  ColumnView& operator=(ColumnView&& other) noexcept {
    keep_ = std::move(other.keep_);
    owned_ = std::move(other.owned_);
    n_ = other.n_;
    ptr_ = owned_.empty() ? other.ptr_ : owned_.data();  // owned storage moved: re-point
    other.ptr_ = nullptr;
    other.n_ = 0;
    return *this;
  }

  const T* data() const noexcept { return ptr_; }
  std::size_t size() const noexcept { return n_; }
  const T& operator[](std::size_t i) const noexcept { return ptr_[i]; }
  bool zero_copy() const noexcept { return owned_.empty() && ptr_ != nullptr; }

private:
  pluss::Series keep_{};
  std::vector<T> owned_{};
  const T* ptr_{nullptr};
  std::size_t n_{0};
};

}  // namespace pluss::kindle
