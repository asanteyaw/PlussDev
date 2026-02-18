#pragma once

// Optional interop with LibTorch.
// Enable by configuring CMake with -DPLUSS_ENABLE_TORCH=ON.
// This header is only safe to include when torch headers are available.

#ifdef PLUSS_ENABLE_TORCH

#include <torch/torch.h>
#include <memory>
#include <vector>

#include "pluss/codem/codem.h" // pluss::codem::Series, ScalarType
#include "CodeM/core/ViewInvariants.h"

namespace pluss::codem {

// Map pluss::codem::ScalarType to torch::ScalarType (CPU only).
inline torch::ScalarType to_torch_dtype(ScalarType st) {
  switch (st) {
    case ScalarType::Bool: return torch::kBool;
    case ScalarType::UInt8: return torch::kUInt8;
    case ScalarType::UInt16: return torch::kUInt16;
    case ScalarType::UInt32: return torch::kUInt32;
    case ScalarType::UInt64: return torch::kUInt64;
    case ScalarType::Int8: return torch::kInt8;
    case ScalarType::Int16: return torch::kInt16;
    case ScalarType::Int32: return torch::kInt32;
    case ScalarType::Int64: return torch::kInt64;
    case ScalarType::Float16: return torch::kFloat16;
    case ScalarType::BFloat16: return torch::kBFloat16;
    case ScalarType::Float32: return torch::kFloat32;
    case ScalarType::Float64: return torch::kFloat64;
    default: break;
  }
  throw std::runtime_error("to_torch_dtype: unsupported dtype");
}

inline ScalarType from_torch_dtype(torch::ScalarType st) {
  switch (st) {
    case torch::kBool: return ScalarType::Bool;
    case torch::kUInt8: return ScalarType::UInt8;
    case torch::kUInt16: return ScalarType::UInt16;
    case torch::kUInt32: return ScalarType::UInt32;
    case torch::kUInt64: return ScalarType::UInt64;
    case torch::kInt8: return ScalarType::Int8;
    case torch::kInt16: return ScalarType::Int16;
    case torch::kInt32: return ScalarType::Int32;
    case torch::kInt64: return ScalarType::Int64;
    case torch::kFloat16: return ScalarType::Float16;
    case torch::kBFloat16: return ScalarType::BFloat16;
    case torch::kFloat32: return ScalarType::Float32;
    case torch::kFloat64: return ScalarType::Float64;
    default: break;
  }
  throw std::runtime_error("from_torch_dtype: unsupported dtype");
}

// Create a pluss Series view over a CPU torch::Tensor (zero-copy).
// Keeps the torch tensor alive via shared_ptr deleter capture.
inline Series from_torch(const torch::Tensor& t) {
  if (!t.defined()) throw std::runtime_error("from_torch: undefined");
  if (!t.device().is_cpu()) throw std::runtime_error("from_torch: CPU only");
  if (!t.is_contiguous() && !t.is_non_overlapping_and_dense()) {
    // We can still represent strided views, but we require well-defined strides without overlap.
    throw std::runtime_error("from_torch: tensor must be non-overlapping and dense (or contiguous)");
  }

  const auto st = from_torch_dtype(t.scalar_type());
  std::vector<std::int64_t> sizes;
  std::vector<std::int64_t> strides;
  sizes.reserve(t.dim());
  strides.reserve(t.dim());
  for (int i=0;i<t.dim();++i) {
    sizes.push_back((std::int64_t)t.sizes()[i]);
    strides.push_back((std::int64_t)t.strides()[i]);
  }

  const std::size_t nbytes = (std::size_t)t.nbytes();
  // Keep torch tensor alive:
  auto holder = std::make_shared<torch::Tensor>(t);
  std::shared_ptr<std::byte> ptr(reinterpret_cast<std::byte*>(t.data_ptr()), [holder](std::byte*) mutable {
    holder.reset();
  });

  Storage storage(ptr, nbytes, /*alignment*/1);
  // SeriesImpl(storage, offset, sizes, strides, dtype)
  return Series::from_storage(storage, (std::int64_t)t.storage_offset(), sizes, strides, st);
}

// Convert Series -> torch::Tensor view (zero-copy) when possible.
// If Series is overlapping/unsupported strides, you can call s.contiguous() first.
namespace detail { inline torch::Tensor to_torch(const Series& s) {
  if (!s.defined()) throw std::runtime_error("to_torch: undefined");
  // We expose CPU tensors only.
  const auto dtype = to_torch_dtype(s.scalar_type());

  std::vector<int64_t> sizes(s.sizes().begin(), s.sizes().end());
  std::vector<int64_t> strides(s.strides().begin(), s.strides().end());

  // Ensure a safe, dense, non-overlapping layout for interop.
  pluss::codem::check_view_invariants(std::vector<std::int64_t>(s.sizes().begin(), s.sizes().end()),
                                   std::vector<std::int64_t>(s.strides().begin(), s.strides().end()));

  // capture Storage handle to keep memory alive
  auto handle = s.storage().handle();
  auto base_ptr = const_cast<void*>(reinterpret_cast<const void*>(s.storage().data()));
  // point to the correct offset
  base_ptr = static_cast<void*>(reinterpret_cast<std::byte*>(base_ptr) + (std::ptrdiff_t)s.storage_offset() * (std::ptrdiff_t)s.dtype().itemsize);

  auto options = torch::TensorOptions().dtype(dtype).device(torch::kCPU);
  auto deleter = [handle](void*) mutable { handle.reset(); };

  auto t = torch::from_blob(base_ptr, sizes, strides, deleter, options);
  return t;
}

} // namespace detail

inline torch::Tensor to_torch(const Series& s) {
  return detail::to_torch(s);
}

} // namespace pluss::codem

#endif // PLUSS_ENABLE_TORCH
