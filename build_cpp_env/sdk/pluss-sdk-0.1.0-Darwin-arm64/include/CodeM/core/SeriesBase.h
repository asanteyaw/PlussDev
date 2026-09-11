#pragma once
#include <cstdint>
#include <ostream>
#include <span>

#include "CodeM/core/Platform.h"
#include "CodeM/core/SeriesImpl.h"
#include "MSharp/IntrusivePtr.h"
#include "MSharp/RangeView.h"
#include "pluss/export.h"

namespace pluss::codem {

class Series;
struct ExtractSpec;

// Handle type (Torch-like TensorBase): cheap copy, shared underlying impl.
class CODEM_API SeriesBase {
public:
  SeriesBase() = default;
  // NOTE: Like at::TensorBase, this constructor is an implementation detail.
  // End users should NOT construct Series/SeriesBase directly; they should use
  // factories (pluss::value, pluss::make_series, etc.).
protected:
  explicit SeriesBase(pluss::msharp::intrusive_ptr<SeriesImpl> impl) : impl_(std::move(impl)) {}

public:
  friend struct ExtractSpec;

  bool defined() const noexcept { return (bool)impl_; }
  bool unique() const noexcept { return impl_.unique(); }
  const SeriesImpl* unsafe_impl() const noexcept { return impl_.get(); }
  SeriesImpl* unsafe_impl() noexcept { return impl_.get(); }

  const Storage& storage() const noexcept { return impl_->storage(); }
  std::int64_t storage_offset() const noexcept { return impl_->storage_offset(); }

  // Shape / dtype introspection
  ScalarType scalar_type() const;
  DType dtype() const;
  pluss::msharp::RangeView::Int sizes() const;
  std::int64_t size(std::int64_t dim) const;
  pluss::msharp::RangeView::Int strides() const;
  std::int64_t stride(std::int64_t dim) const;
  std::int64_t dim() const;
  std::int64_t numel() const;
  std::int64_t itemsize() const { return static_cast<std::int64_t>(dtype().itemsize); }
  bool is_contiguous() const;
  bool is_complex() const { return dtype().is_complex(); }
  bool is_floating_point() const { return dtype().is_floating(); }
  bool is_signed() const;
  Platform platform() const noexcept { return defined() ? impl_->platform() : Platform::CPU; }
  bool is_cpu() const noexcept { return platform() == Platform::CPU; }
  bool is_gpu() const noexcept { return !is_cpu(); }
  bool is_cuda() const noexcept { return platform() == Platform::CUDA; }
  bool is_mps() const noexcept { return platform() == Platform::MPS; }
  bool is_pinned() const noexcept { return false; }
  // Vector-style bounds-checked scalar selection for 1-D Series.
  // Returns a rank-0 Series view of the selected element.
  Series pos(std::int64_t index) const;
  // Matrix-style bounds-checked scalar selection for 2-D Series.
  // Returns a rank-0 Series view of the selected element.
  Series pos(std::int64_t row, std::int64_t col) const;
  // 3-D bounds-checked scalar selection.
  // Returns a rank-0 Series view of the selected element.
  Series pos(std::int64_t i, std::int64_t j, std::int64_t k) const;

  // Raw storage access (byte-addressable).
  std::byte* raw_data();
  const std::byte* raw_data() const;

protected:
  pluss::msharp::intrusive_ptr<SeriesImpl> impl_{nullptr};

  const pluss::msharp::intrusive_ptr<SeriesImpl>& impl() const noexcept { return impl_; }
  pluss::msharp::intrusive_ptr<SeriesImpl>& impl() noexcept { return impl_; }
};

} // namespace pluss::codem

inline std::ostream& operator<<(std::ostream& os, std::span<const std::int64_t> xs) {
  os << '[';
  for (std::size_t i = 0; i < xs.size(); ++i) {
    if (i != 0) {
      os << ", ";
    }
    os << xs[i];
  }
  os << ']';
  return os;
}
