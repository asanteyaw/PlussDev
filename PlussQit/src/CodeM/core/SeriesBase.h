#pragma once
#include <cstdint>
#include <span>

#include "CodeM/core/SeriesImpl.h"
#include "MSharp/SnapPtr.h"

namespace pluss::codem {

// Handle type (Torch-like TensorBase): cheap copy, shared underlying impl.
class SeriesBase {
public:
  SeriesBase() = default;
  explicit SeriesBase(pluss::msharp::SnapPtr<SeriesImpl> impl) : impl_(std::move(impl)) {}

  bool defined() const noexcept { return (bool)impl_; }
  bool unique() const noexcept { return impl_.unique(); }
  const SeriesImpl* unsafe_impl() const noexcept { return impl_.get(); }
  SeriesImpl* unsafe_impl() noexcept { return impl_.get(); }

  const Storage& storage() const noexcept { return impl_->storage(); }
  std::int64_t storage_offset() const noexcept { return impl_->storage_offset(); }

  // Shape / dtype introspection
  ScalarType scalar_type() const;
  DType dtype() const;
  std::span<const std::int64_t> sizes() const;
  std::span<const std::int64_t> strides() const;
  std::int64_t dim() const;
  std::int64_t numel() const;
  bool is_contiguous() const;

  // Raw storage access (byte-addressable).
  std::byte* raw_data();
  const std::byte* raw_data() const;

protected:
  pluss::msharp::SnapPtr<SeriesImpl> impl_{nullptr};

  const pluss::msharp::SnapPtr<SeriesImpl>& impl() const noexcept { return impl_; }
  pluss::msharp::SnapPtr<SeriesImpl>& impl() noexcept { return impl_; }
};

} // namespace pluss::codem
