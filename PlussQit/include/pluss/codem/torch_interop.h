#pragma once

// LibTorch bridge: pluss::series::from_torch(tensor) and Series::tensor().
//
// There is no special build. The bridge is header-only and is compiled inside YOUR project,
// the same way PyTorch keeps its NumPy bridge out of its core: libpluss never mentions
// Torch, and this header turns itself on when Torch's headers are visible.
//
//   * Active when <ATen/ATen.h> is on the include path (or PLUSS_ENABLE_TORCH is defined).
//   * Define PLUSS_DISABLE_TORCH_INTEROP before including <pluss/pluss.h> to opt out.
//   * Your project links Torch itself (find_package(Torch) and target_link_libraries); the
//     PlussII archives do not contain or depend on it.
//
// Both directions are zero-copy and CPU-only: a Series and a tensor made from it share the
// same memory, and whichever side is still alive keeps the memory alive. Move to the GPU on
// the Torch side (tensor.to(device)), and back to the CPU before calling from_torch.

#if !defined(PLUSS_DISABLE_TORCH_INTEROP) && \
    (defined(PLUSS_ENABLE_TORCH) || (defined(__has_include) && __has_include(<ATen/ATen.h>)))
#define PLUSS_TORCH_INTEROP_AVAILABLE 1
#endif

#ifdef PLUSS_TORCH_INTEROP_AVAILABLE

#include <ATen/ATen.h>
#ifdef PLUSS_ENABLE_TORCH
#include <torch/torch.h> // builds configured with PLUSS_ENABLE_TORCH keep the full torch:: API in scope
#endif
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "pluss/codem/codem.h" // pluss::codem::Series, ScalarType
#include "CodeM/core/ViewInvariants.h"

namespace pluss::codem {

// Map pluss::codem::ScalarType to the ATen scalar type.
inline c10::ScalarType to_torch_dtype(ScalarType st) {
  switch (st) {
    case ScalarType::Bool: return c10::ScalarType::Bool;
    case ScalarType::UInt8: return c10::ScalarType::Byte;
    case ScalarType::UInt16: return c10::ScalarType::UInt16;
    case ScalarType::UInt32: return c10::ScalarType::UInt32;
    case ScalarType::UInt64: return c10::ScalarType::UInt64;
    case ScalarType::Int8: return c10::ScalarType::Char;
    case ScalarType::Int16: return c10::ScalarType::Short;
    case ScalarType::Int32: return c10::ScalarType::Int;
    case ScalarType::Int64: return c10::ScalarType::Long;
    case ScalarType::Float16: return c10::ScalarType::Half;
    case ScalarType::BFloat16: return c10::ScalarType::BFloat16;
    case ScalarType::Float32: return c10::ScalarType::Float;
    case ScalarType::Float64: return c10::ScalarType::Double;
    case ScalarType::Complex64: return c10::ScalarType::ComplexFloat;
    case ScalarType::Complex128: return c10::ScalarType::ComplexDouble;
    default: break;
  }
  ::pluss::throw_error("to_torch_dtype: unsupported dtype");
}

inline ScalarType from_torch_dtype(c10::ScalarType st) {
  switch (st) {
    case c10::ScalarType::Bool: return ScalarType::Bool;
    case c10::ScalarType::Byte: return ScalarType::UInt8;
    case c10::ScalarType::UInt16: return ScalarType::UInt16;
    case c10::ScalarType::UInt32: return ScalarType::UInt32;
    case c10::ScalarType::UInt64: return ScalarType::UInt64;
    case c10::ScalarType::Char: return ScalarType::Int8;
    case c10::ScalarType::Short: return ScalarType::Int16;
    case c10::ScalarType::Int: return ScalarType::Int32;
    case c10::ScalarType::Long: return ScalarType::Int64;
    case c10::ScalarType::Half: return ScalarType::Float16;
    case c10::ScalarType::BFloat16: return ScalarType::BFloat16;
    case c10::ScalarType::Float: return ScalarType::Float32;
    case c10::ScalarType::Double: return ScalarType::Float64;
    case c10::ScalarType::ComplexFloat: return ScalarType::Complex64;
    case c10::ScalarType::ComplexDouble: return ScalarType::Complex128;
    default: break;
  }
  ::pluss::throw_error("from_torch_dtype: unsupported dtype");
}

// Create a pluss Series view over a CPU torch::Tensor (zero-copy).
// Keeps the torch tensor alive via shared_ptr deleter capture.
//
// Refused, with a message that says what to do: undefined tensors, non-CPU tensors
// (call t.cpu() first), non-strided (sparse) layouts, tensors carrying an unresolved
// conjugate or negative bit (call t.resolve_conj() / t.resolve_neg() first), and
// overlapping or non-dense strides. A tensor that requires grad is shared as plain data;
// its autograd history stays on the Torch side.
inline Series from_torch(const at::Tensor& t) {
  if (!t.defined()) ::pluss::throw_error("from_torch: undefined tensor");
  if (!t.device().is_cpu()) ::pluss::throw_error("from_torch: CPU only; call t.cpu() first");
  if (t.layout() != c10::Layout::Strided) ::pluss::throw_error("from_torch: only strided tensors are supported; call t.to_dense() first");
  if (t.is_conj()) ::pluss::throw_error("from_torch: tensor has the conjugate bit set; call t.resolve_conj() first");
  if (t.is_neg()) ::pluss::throw_error("from_torch: tensor has the negative bit set; call t.resolve_neg() first");
  if (!t.is_contiguous() && !t.is_non_overlapping_and_dense()) {
    // We can still represent strided views, but we require well-defined strides without overlap.
    ::pluss::throw_error("from_torch: tensor must be non-overlapping and dense (or contiguous)");
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
  auto holder = std::make_shared<at::Tensor>(t);
  std::shared_ptr<std::byte> ptr(reinterpret_cast<std::byte*>(t.data_ptr()), [holder](std::byte*) mutable {
    holder.reset();
  });

  Storage storage(ptr, nbytes, /*alignment*/1);
  // data_ptr() already points at the view's first logical element. Starting the
  // adopted Storage there means applying storage_offset() again would skip it twice.
  return pluss::codem::factory::from_storage(storage, /*storage_offset=*/0, sizes, strides, st);
}

// Convert Series -> torch::Tensor view (zero-copy) when possible.
// If Series is overlapping/unsupported strides, you can call s.contiguous() first.
//
// This is an INTERNAL helper. The public Series -> torch direction is the
// method Series::tensor(), which forwards here; there is no public free
// to_torch function.
namespace detail { inline at::Tensor to_torch(const Series& s) {
  if (!s.defined()) ::pluss::throw_error("tensor: undefined Series");
  if (s.platform() != Platform::CPU) ::pluss::throw_error("tensor: CPU only; call s.to(pluss::Platform::CPU) first");
  const auto dtype = to_torch_dtype(s.scalar_type());

  std::vector<int64_t> sizes(s.sizes().begin(), s.sizes().end());
  std::vector<int64_t> strides(s.strides().begin(), s.strides().end());

  // Ensure a safe, dense, non-overlapping layout for interop.
  pluss::codem::check_view_invariants(std::vector<std::int64_t>(s.sizes().begin(), s.sizes().end()),
                                   std::vector<std::int64_t>(s.strides().begin(), s.strides().end()));

  // capture Storage handle to keep memory alive
  auto handle = s.storage().handle();
  auto base_ptr = const_cast<void*>(reinterpret_cast<const void*>(s.storage().data()));
  // Series::storage_offset() is already a BYTE offset (every other consumer in
  // the codebase divides it by itemsize to get elements) -- add it directly.
  base_ptr = static_cast<void*>(reinterpret_cast<std::byte*>(base_ptr) + (std::ptrdiff_t)s.storage_offset());

  auto options = at::TensorOptions().dtype(dtype).device(at::kCPU);
  auto deleter = [handle](void*) mutable { handle.reset(); };

  auto t = at::from_blob(base_ptr, sizes, strides, deleter, options);
  return t;
}

} // namespace detail

// Series::tensor(): declared (as a template, so that the library never needs Torch) in Series.h.
template <class Unused>
auto Series::tensor() const {
  return detail::to_torch(*this);
}

} // namespace pluss::codem

#endif // PLUSS_TORCH_INTEROP_AVAILABLE
