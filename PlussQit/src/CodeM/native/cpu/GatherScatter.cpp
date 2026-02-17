#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"

#include <cstring>
#include <vector>

namespace pluss::codem::native::cpu {

// Torch-like gather:
// out = gather(input, dim, index)
// index must be int64, and out has same shape as index.
// For each position, reads input at index along dim.
Series gather(const Series& input, std::int64_t dim, const Series& index) {
  if (!input.defined() || !index.defined()) throw_error("gather: undefined");
  if (index.scalar_type() != ScalarType::Int64) throw_error("gather: index must be int64");
  const auto nd = input.dim();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("gather: dim out of range");
  if (index.dim() != nd) throw_error("gather: index rank must match input rank");

  for (std::int64_t d = 0; d < nd; ++d) {
    if (d == dim) continue;
    if (index.sizes()[(std::size_t)d] != input.sizes()[(std::size_t)d]) {
      throw_error("gather: index shape must match input except at dim");
    }
  }

  auto in = input.contiguous();
  auto idx = index.contiguous();
  Series out = Series::empty(std::vector<std::int64_t>(idx.sizes().begin(), idx.sizes().end()), input.scalar_type());

  const auto dt = in.dtype();
  const auto* in_b = in.raw_data();
  const auto* ip = reinterpret_cast<const std::int64_t*>(idx.raw_data());
  auto* out_b = const_cast<std::byte*>(out.raw_data());

  // Compute strides for contiguous row-major
  auto sizes = std::vector<std::int64_t>(in.sizes().begin(), in.sizes().end());
  auto strides = contiguous_strides(sizes); // elements

  const std::int64_t dim_size = sizes[(std::size_t)dim];
  const std::int64_t total = out.numel();

  // linear -> multi-index, then replace idx at dim from index tensor and compute input offset
  std::vector<std::int64_t> multi((std::size_t)nd, 0);

  auto lin_to_multi = [&](std::int64_t lin){
    for (std::int64_t d = nd - 1; d >= 0; --d) {
      const auto s = sizes[(std::size_t)d];
      multi[(std::size_t)d] = (s == 0) ? 0 : (lin % s);
      lin = (s == 0) ? 0 : (lin / s);
    }
  };

  for (std::int64_t lin = 0; lin < total; ++lin) {
    lin_to_multi(lin);
    std::int64_t k = ip[lin];
    if (k < 0) k += dim_size;
    if (k < 0 || k >= dim_size) throw_error("gather: index out of range");
    multi[(std::size_t)dim] = k;

    std::int64_t in_off = 0;
    for (std::int64_t d = 0; d < nd; ++d) in_off += multi[(std::size_t)d] * strides[(std::size_t)d];

    std::memcpy(out_b + (std::ptrdiff_t)lin * (std::ptrdiff_t)dt.itemsize,
                in_b + (std::ptrdiff_t)in_off * (std::ptrdiff_t)dt.itemsize,
                dt.itemsize);
  }

  return out;
}

// Torch-like scatter (functional):
// out = scatter(input, dim, index, src)
// out starts as a copy of input; for each position in index/src, writes src into out along dim.
Series scatter(const Series& input, std::int64_t dim, const Series& index, const Series& src) {
  if (!input.defined() || !index.defined() || !src.defined()) throw_error("scatter: undefined");
  if (index.scalar_type() != ScalarType::Int64) throw_error("scatter: index must be int64");
  if (src.scalar_type() != input.scalar_type()) throw_error("scatter: src dtype mismatch");
  const auto nd = input.dim();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("scatter: dim out of range");
  if (index.dim() != nd || src.dim() != nd) throw_error("scatter: index/src rank must match input rank");

  // shape checks: index and src must match, and match input except possibly at dim (torch allows)
  for (std::int64_t d = 0; d < nd; ++d) {
    if (index.sizes()[(std::size_t)d] != src.sizes()[(std::size_t)d]) throw_error("scatter: index/src shape mismatch");
    if (d == dim) continue;
    if (index.sizes()[(std::size_t)d] != input.sizes()[(std::size_t)d]) throw_error("scatter: shape mismatch");
  }

  auto out = input.contiguous(); // start copy
  auto idx = index.contiguous();
  auto ss = src.contiguous();

  const auto dt = out.dtype();
  const auto* sp = ss.raw_data();
  const auto* ip = reinterpret_cast<const std::int64_t*>(idx.raw_data());
  auto* out_b = const_cast<std::byte*>(out.raw_data());

  auto sizes = std::vector<std::int64_t>(out.sizes().begin(), out.sizes().end());
  auto strides = contiguous_strides(sizes); // elements

  const std::int64_t dim_size = sizes[(std::size_t)dim];
  const std::int64_t total = ss.numel();

  // output multi-index comes from src position, but replace along dim with index value
  std::vector<std::int64_t> multi((std::size_t)nd, 0);

  auto lin_to_multi_src = [&](std::int64_t lin){
    for (std::int64_t d = nd - 1; d >= 0; --d) {
      const auto s = ss.sizes()[(std::size_t)d];
      multi[(std::size_t)d] = (s == 0) ? 0 : (lin % s);
      lin = (s == 0) ? 0 : (lin / s);
    }
  };

  for (std::int64_t lin = 0; lin < total; ++lin) {
    lin_to_multi_src(lin);
    std::int64_t k = ip[lin];
    if (k < 0) k += dim_size;
    if (k < 0 || k >= dim_size) throw_error("scatter: index out of range");
    multi[(std::size_t)dim] = k;

    std::int64_t out_off = 0;
    for (std::int64_t d = 0; d < nd; ++d) out_off += multi[(std::size_t)d] * strides[(std::size_t)d];

    std::memcpy(out_b + (std::ptrdiff_t)out_off * (std::ptrdiff_t)dt.itemsize,
                sp + (std::ptrdiff_t)lin * (std::ptrdiff_t)dt.itemsize,
                dt.itemsize);
  }

  return out;
}

void scatter_(Series& input, std::int64_t dim, const Series& index, const Series& src) {
  input = scatter(input, dim, index, src);
}

} // namespace pluss::codem::native::cpu
