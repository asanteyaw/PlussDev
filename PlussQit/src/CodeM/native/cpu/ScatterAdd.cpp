#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"

#include <vector>
#include <cstring>

namespace pluss::codem::native::cpu {

// scatter_add(input, dim, index, src): out = input clone; out[...] += src[...] at indexed positions.
// index: int64, rank matches input; src shape matches index; for d!=dim, match input sizes.
Series scatter_add(const Series& input, std::int64_t dim, const Series& index, const Series& src) {
  if (!input.defined() || !index.defined() || !src.defined()) throw_error("scatter_add: undefined");
  if (index.scalar_type() != ScalarType::Int64) throw_error("scatter_add: index must be int64");
  if (src.scalar_type() != input.scalar_type()) throw_error("scatter_add: src dtype mismatch");
  const auto nd = input.dim();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("scatter_add: dim out of range");
  if (index.dim() != nd || src.dim() != nd) throw_error("scatter_add: index/src rank must match input rank");

  for (std::int64_t d = 0; d < nd; ++d) {
    if (index.sizes()[(std::size_t)d] != src.sizes()[(std::size_t)d]) throw_error("scatter_add: index/src shape mismatch");
    if (d == dim) continue;
    if (index.sizes()[(std::size_t)d] != input.sizes()[(std::size_t)d]) throw_error("scatter_add: shape mismatch");
  }

  auto out = input.contiguous(); // copy
  auto idx = index.contiguous();
  auto ss  = src.contiguous();

  const auto* ip = reinterpret_cast<const std::int64_t*>(idx.raw_data());
  const auto total = ss.numel();

  // strides for contiguous input
  auto sizes = std::vector<std::int64_t>(out.sizes().begin(), out.sizes().end());
  auto strides = contiguous_strides(sizes); // elements
  const std::int64_t dim_size = sizes[(std::size_t)dim];

  std::vector<std::int64_t> multi((std::size_t)nd, 0);
  auto lin_to_multi_src = [&](std::int64_t lin){
    for (std::int64_t d = nd - 1; d >= 0; --d) {
      const auto s = ss.sizes()[(std::size_t)d];
      multi[(std::size_t)d] = (s == 0) ? 0 : (lin % s);
      lin = (s == 0) ? 0 : (lin / s);
    }
  };

  if (out.scalar_type() == ScalarType::Float64) {
    auto* op = reinterpret_cast<double*>(out.raw_data());
    const auto* sp = reinterpret_cast<const double*>(ss.raw_data());
    for (std::int64_t lin=0; lin<total; ++lin) {
      lin_to_multi_src(lin);
      std::int64_t k = ip[lin];
      if (k < 0) k += dim_size;
      if (k < 0 || k >= dim_size) throw_error("scatter_add: index out of range");
      multi[(std::size_t)dim] = k;
      std::int64_t out_off = 0;
      for (std::int64_t d=0; d<nd; ++d) out_off += multi[(std::size_t)d] * strides[(std::size_t)d];
      op[out_off] += sp[lin];
    }
    return out;
  }
  if (out.scalar_type() == ScalarType::Float32) {
    auto* op = reinterpret_cast<float*>(out.raw_data());
    const auto* sp = reinterpret_cast<const float*>(ss.raw_data());
    for (std::int64_t lin=0; lin<total; ++lin) {
      lin_to_multi_src(lin);
      std::int64_t k = ip[lin];
      if (k < 0) k += dim_size;
      if (k < 0 || k >= dim_size) throw_error("scatter_add: index out of range");
      multi[(std::size_t)dim] = k;
      std::int64_t out_off = 0;
      for (std::int64_t d=0; d<nd; ++d) out_off += multi[(std::size_t)d] * strides[(std::size_t)d];
      op[out_off] += sp[lin];
    }
    return out;
  }
  if (out.scalar_type() == ScalarType::Int64) {
    auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
    const auto* sp = reinterpret_cast<const std::int64_t*>(ss.raw_data());
    for (std::int64_t lin=0; lin<total; ++lin) {
      lin_to_multi_src(lin);
      std::int64_t k = ip[lin];
      if (k < 0) k += dim_size;
      if (k < 0 || k >= dim_size) throw_error("scatter_add: index out of range");
      multi[(std::size_t)dim] = k;
      std::int64_t out_off = 0;
      for (std::int64_t d=0; d<nd; ++d) out_off += multi[(std::size_t)d] * strides[(std::size_t)d];
      op[out_off] += sp[lin];
    }
    return out;
  }
  throw_error("scatter_add: dtype not implemented");
}

} // namespace pluss::codem::native::cpu


void scatter_add_inplace(Series& input, std::int64_t dim, const Series& index, const Series& src) {
  // Conservative implementation: compute scatter_add result then copy into input.
  // This preserves semantics and is safe for aliasing; later we can specialize for speed.
  Series out = scatter_add(input, dim, index, src);
  input.copy_(out);
}

