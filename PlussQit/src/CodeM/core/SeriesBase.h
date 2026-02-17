#pragma once
#include <cstdint>
#include <span>

#include "CodeM/core/SeriesImpl.h"
#include "MSharp/IntrusivePtr.h"
#include "pluss/export.h"

namespace pluss::codem {

enum class Platform : std::uint8_t {
  CPU = 0,
  CUDA = 1,
  MPS = 2
};

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
  std::int64_t itemsize() const { return static_cast<std::int64_t>(dtype().itemsize); }
  bool is_contiguous() const;
  Platform platform() const noexcept { return Platform::CPU; } // CPU-only for now.

  // Raw storage access (byte-addressable).
  std::byte* raw_data();
  const std::byte* raw_data() const;

protected:
  pluss::msharp::intrusive_ptr<SeriesImpl> impl_{nullptr};

  const pluss::msharp::intrusive_ptr<SeriesImpl>& impl() const noexcept { return impl_; }
  pluss::msharp::intrusive_ptr<SeriesImpl>& impl() noexcept { return impl_; }
};

} // namespace pluss::codem
