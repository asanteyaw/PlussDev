#pragma once

#include "CodeM/core/Series.h"
#include "CodeM/core/Config.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/factories/ValueInitList.h"
#include "CodeM/core/Storage.h"
#include "CodeM/core/Errors.h"
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <type_traits>

namespace pluss::codem {

// Scalar constants / factories (CPU-only).
inline Series nan(ScalarType dtype=ScalarType::Float32) {
  const double v = std::numeric_limits<double>::quiet_NaN();
  return Series::full({}, v, dtype);
}
inline Series inf(ScalarType dtype=ScalarType::Float32) {
  const double v = std::numeric_limits<double>::infinity();
  return Series::full({}, v, dtype);
}
inline Series neg_inf(ScalarType dtype=ScalarType::Float32) {
  const double v = -std::numeric_limits<double>::infinity();
  return Series::full({}, v, dtype);
}

// Create a scalar Series (rvalue-style), similar to torch::tensor(scalar).
inline Series value(double v, ScalarType dtype = ScalarType::Float32) {
  return Series::full({}, v, dtype);
}


// Scalar overloads: integral defaults to Int64; bool defaults to Bool.
template <class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
inline Series value(T v, ScalarType dtype = ScalarType::Int64) {
  return Series::full({}, static_cast<double>(v), dtype);
}
inline Series value(bool v, ScalarType dtype = ScalarType::Bool) {
  return Series::full({}, v ? 1.0 : 0.0, dtype);
}

// Nested initializer_list: infer shape and flatten. Supports up to 7D (enforced by depth check).
template <class T>
inline Series value(std::initializer_list<T> list, ScalarType dtype = ScalarType::Float32) {
  static_assert(pluss::codem::factories::depth<std::initializer_list<T>>::value <= 7,
                "pluss::value: supports up to 7D nested initializer_list");
  auto inf = pluss::codem::factories::infer(list);
  Series out = Series::empty(inf.sizes, dtype);
  Series tmp = Series::empty(inf.sizes, ScalarType::Float64);
  std::memcpy(
      tmp.raw_mutable_data(),
      inf.data.data(),
      sizeof(double) * static_cast<std::size_t>(tmp.numel()));
  out.copy_(tmp.to(dtype), false);
  return out;
}

// Create a Series by copying from an lvalue buffer (e.g. std::vector / span).
// Overload that deduces T from std::vector<T>.
template <class T>
inline Series make_series(const std::vector<T>& data, std::span<const std::int64_t> sizes, Config cfg = {}) {
  return make_series<T>(std::span<const T>(data.data(), data.size()), sizes, cfg);
}

// Overload that deduces T from std::span<T> + initializer_list sizes.
template <class T>
inline Series make_series(std::span<const T> data, std::initializer_list<std::int64_t> sizes, Config cfg = {}) {
  return make_series<T>(data, std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg);
}

template <class T>
inline Series make_series(std::span<const T> data, std::span<const std::int64_t> sizes, Config cfg = {}) {
  // Infer dtype from T if user didn't override explicitly.
  ScalarType dt = cfg.dtype;
  if (dt == ScalarType::Float32) {
    using U = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
      dt = ScalarType::Bool;
    } else if constexpr (std::is_same_v<U, std::uint8_t>) {
      dt = ScalarType::UInt8;
    } else if constexpr (std::is_integral_v<U>) {
      // Torch-like default integral promotion policy for factories.
      dt = ScalarType::Int64;
    } else if constexpr (std::is_same_v<U, double>) {
      dt = ScalarType::Float64;
    } else if constexpr (std::is_same_v<U, float>) {
      dt = ScalarType::Float32;
    } else if constexpr (std::is_same_v<U, Float16>) {
      dt = ScalarType::Float16;
    } else if constexpr (std::is_same_v<U, BFloat16>) {
      dt = ScalarType::BFloat16;
    } else if constexpr (std::is_same_v<U, Complex64>) {
      dt = ScalarType::Complex64;
    } else if constexpr (std::is_same_v<U, Complex128>) {
      dt = ScalarType::Complex128;
    }
  }
  using U = std::remove_cv_t<T>;
  constexpr bool src_supported =
    std::is_same_v<U, bool> ||
    std::is_same_v<U, std::uint8_t> ||
    std::is_same_v<U, std::uint16_t> ||
    std::is_same_v<U, std::uint32_t> ||
    std::is_same_v<U, std::uint64_t> ||
    std::is_same_v<U, std::int8_t> ||
    std::is_same_v<U, std::int16_t> ||
    std::is_same_v<U, std::int32_t> ||
    std::is_same_v<U, std::int64_t> ||
    std::is_same_v<U, float> ||
    std::is_same_v<U, double> ||
    std::is_same_v<U, Float16> ||
    std::is_same_v<U, BFloat16> ||
    std::is_same_v<U, Complex64> ||
    std::is_same_v<U, Complex128>;
  static_assert(src_supported, "make_series: unsupported source element type");

  constexpr ScalarType src_dt = []() constexpr -> ScalarType {
    if constexpr (std::is_same_v<U, bool>) return ScalarType::Bool;
    else if constexpr (std::is_same_v<U, std::uint8_t>) return ScalarType::UInt8;
    else if constexpr (std::is_same_v<U, std::uint16_t>) return ScalarType::UInt16;
    else if constexpr (std::is_same_v<U, std::uint32_t>) return ScalarType::UInt32;
    else if constexpr (std::is_same_v<U, std::uint64_t>) return ScalarType::UInt64;
    else if constexpr (std::is_same_v<U, std::int8_t>) return ScalarType::Int8;
    else if constexpr (std::is_same_v<U, std::int16_t>) return ScalarType::Int16;
    else if constexpr (std::is_same_v<U, std::int32_t>) return ScalarType::Int32;
    else if constexpr (std::is_same_v<U, std::int64_t>) return ScalarType::Int64;
    else if constexpr (std::is_same_v<U, float>) return ScalarType::Float32;
    else if constexpr (std::is_same_v<U, double>) return ScalarType::Float64;
    else if constexpr (std::is_same_v<U, Float16>) return ScalarType::Float16;
    else if constexpr (std::is_same_v<U, BFloat16>) return ScalarType::BFloat16;
    else if constexpr (std::is_same_v<U, Complex64>) return ScalarType::Complex64;
    else return ScalarType::Complex128;
  }();

  Series src = Series::empty(sizes, src_dt, cfg.platform).contiguous();
  const std::size_t n = (std::size_t)src.numel();
  PLUSS_CHECK(data.size() == n, "make_series: data size mismatch with sizes");
  std::memcpy(src.raw_data(), data.data(), n * sizeof(U));
  if (src_dt != dt) {
    src = src.to(dt);
  }
  return (src.platform() == cfg.platform) ? src : src.to(cfg.platform);
}

// Expose external memory without taking ownership.
// `deleter` will be invoked when the last Series view is destroyed.
using BlobDeleter = std::function<void(void*)>;

inline Series from_buffer(
    void* data,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides,
    BlobDeleter deleter,
    ScalarType dtype = ScalarType::Float32) {

  PLUSS_CHECK(data != nullptr, "from_buffer: null data");
  PLUSS_CHECK(sizes.size() == strides.size(), "from_buffer: sizes/strides rank mismatch");
  const auto item = dtype_of(dtype).itemsize;

  // conservative byte-size: max offset + 1 element
  std::int64_t max_off = 0;
  for (std::size_t i=0;i<sizes.size();++i) {
    if (sizes[i] == 0) continue;
    max_off += (sizes[i]-1) * strides[i];
  }
  const std::size_t nbytes = (std::size_t)(max_off + 1) * (std::size_t)item;

  auto sp = std::shared_ptr<std::byte>((std::byte*)data, [deleter](std::byte* p){
    if (deleter) deleter((void*)p);
  });
  Storage stor(sp, nbytes, 1);
  return ops_internal::SeriesFactoryAccess::wrap(
      pluss::msharp::make_intrusive<SeriesImpl>(
          stor,
          /*storage_offset_elems=*/0,
          std::vector<std::int64_t>(sizes.begin(), sizes.end()),
          std::vector<std::int64_t>(strides.begin(), strides.end()),
          dtype,
          Platform::CPU));
}

} // namespace pluss::codem
