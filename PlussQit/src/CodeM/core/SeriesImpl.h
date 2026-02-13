#pragma once
#include <cstdint>
#include <span>
#include <vector>

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Storage.h"
#include "MSharp/SnapPtr.h"

namespace pluss::codem {

// Metadata+storage backing object (Torch-like TensorImpl).
class SeriesImpl final : public pluss::msharp::RefCounted {
public:
  SeriesImpl(Storage storage,
             std::int64_t storage_offset_elems,
             std::vector<std::int64_t> sizes,
             std::vector<std::int64_t> strides,
             ScalarType st);

  ScalarType scalar_type() const noexcept { return st_; }
  DType dtype() const noexcept { return dtype_of(st_); }

  std::span<const std::int64_t> sizes() const noexcept { return sizes_; }
  std::span<const std::int64_t> strides() const noexcept { return strides_; }
  std::int64_t dim() const noexcept { return static_cast<std::int64_t>(sizes_.size()); }

  std::int64_t numel() const noexcept;
  bool is_contiguous() const noexcept;

  std::byte* raw_data() noexcept;
  const std::byte* raw_data() const noexcept;

  const Storage& storage() const noexcept { return storage_; }
  std::int64_t storage_offset() const noexcept { return storage_offset_elems_ * static_cast<std::int64_t>(dtype_of(st_).itemsize); }
  std::int64_t storage_offset_elems() const noexcept { return storage_offset_elems_; }

private:
  Storage storage_{};
  std::int64_t storage_offset_elems_{0};
  std::vector<std::int64_t> sizes_{};
  std::vector<std::int64_t> strides_{};
  ScalarType st_{ScalarType::Float32};
};

std::vector<std::int64_t> contiguous_strides(std::span<const std::int64_t> sizes);
std::int64_t compute_numel(std::span<const std::int64_t> sizes);

} // namespace pluss::codem
