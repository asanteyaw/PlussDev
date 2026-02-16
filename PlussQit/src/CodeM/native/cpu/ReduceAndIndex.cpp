#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"

#include <cstring>
#include <vector>

namespace pluss::codem::native::cpu {

static std::vector<std::int64_t> out_sizes_for_reduce(std::span<const std::int64_t> sizes, std::int64_t dim, bool keepdim) {
  std::vector<std::int64_t> out(sizes.begin(), sizes.end());
  if (dim < 0) dim += (std::int64_t)out.size();
  if (dim < 0 || dim >= (std::int64_t)out.size()) throw_error("sum: dim out of range");
  if (keepdim) out[(std::size_t)dim] = 1;
  else out.erase(out.begin() + dim);
  return out;
}

Series sum(const Series& a, std::int64_t dim, bool keepdim) {
  if (!a.defined()) throw_error("sum: undefined");
  if (a.dim() == 0) {
    // scalar -> scalar
    auto out = Series::zeros({}, a.scalar_type());
    std::memcpy(out.raw_data(), a.raw_data(), (std::size_t)a.dtype().itemsize);
    return out;
  }
  auto out_sizes = out_sizes_for_reduce(a.sizes(), dim, keepdim);
  auto out = Series::zeros(out_sizes, a.scalar_type());

  // Simple reduction: materialize contiguous and reduce along dim using nested loops.
  auto src = a.contiguous();
  auto sizes = std::vector<std::int64_t>(src.sizes().begin(), src.sizes().end());
  auto strides = std::vector<std::int64_t>(src.strides().begin(), src.strides().end());
  if (dim < 0) dim += (std::int64_t)sizes.size();

  const auto reduce_n = sizes[(std::size_t)dim];
  // flatten outer and inner
  std::int64_t outer = 1;
  for (std::int64_t i=0;i<dim;++i) outer *= sizes[(std::size_t)i];
  std::int64_t inner = 1;
  for (std::int64_t i=dim+1;i<(std::int64_t)sizes.size();++i) inner *= sizes[(std::size_t)i];

  // For contiguous, stride at dim is inner, and element order is row-major; still compute with offsets.
  switch (src.scalar_type()) {
    case ScalarType::Float64: {
      const auto* p = reinterpret_cast<const double*>(src.raw_data());
      auto* q = reinterpret_cast<double*>(out.raw_data());
      for (std::int64_t o=0;o<outer;++o) {
        for (std::int64_t i=0;i<inner;++i) {
          double acc = 0.0;
          const std::int64_t base = o*reduce_n*inner + i;
          for (std::int64_t r=0;r<reduce_n;++r) acc += p[base + r*inner];
          q[o*inner + i] = acc;
        }
      }
      return out;
    }
    case ScalarType::Float32: {
      const auto* p = reinterpret_cast<const float*>(src.raw_data());
      auto* q = reinterpret_cast<float*>(out.raw_data());
      for (std::int64_t o=0;o<outer;++o) {
        for (std::int64_t i=0;i<inner;++i) {
          float acc = 0.0f;
          const std::int64_t base = o*reduce_n*inner + i;
          for (std::int64_t r=0;r<reduce_n;++r) acc += p[base + r*inner];
          q[o*inner + i] = acc;
        }
      }
      return out;
    }
    default:
      throw_error("sum: dtype not implemented");
  }
}

Series index_select(const Series& a, std::int64_t dim, const Series& index) {
  if (!a.defined() || !index.defined()) throw_error("index_select: undefined");
  if (index.dim() != 1) throw_error("index_select: index must be 1D");
  if (index.scalar_type() != ScalarType::Int64) throw_error("index_select: index dtype must be int64");
  auto nd = a.dim();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("index_select: dim out of range");

  auto out_sizes = std::vector<std::int64_t>(a.sizes().begin(), a.sizes().end());
  out_sizes[(std::size_t)dim] = index.numel();
  auto out = Series::empty(out_sizes, a.scalar_type());

  auto src = a.contiguous();
  auto idx = index.contiguous();
  const auto* ip = reinterpret_cast<const std::int64_t*>(idx.raw_data());

  // Copy slices along dim.
  // For simplicity we handle contiguous tensors: treat as row-major and copy blocks.
  const auto dt = src.dtype();
  const std::int64_t inner = [&]{
    std::int64_t inner = 1;
    for (std::int64_t i=dim+1;i<nd;++i) inner *= src.sizes()[(std::size_t)i];
    return inner;
  }();
  const std::int64_t outer = [&]{
    std::int64_t outer = 1;
    for (std::int64_t i=0;i<dim;++i) outer *= src.sizes()[(std::size_t)i];
    return outer;
  }();
  const std::int64_t in_dim = src.sizes()[(std::size_t)dim];
  const std::int64_t out_dim = out_sizes[(std::size_t)dim];

  auto* out_bytes = const_cast<std::byte*>(out.raw_data());
  const auto* in_bytes = src.raw_data();
  const std::size_t item = dt.itemsize;

  for (std::int64_t o=0;o<outer;++o) {
    for (std::int64_t j=0;j<out_dim;++j) {
      std::int64_t k = ip[j];
      if (k < 0) k += in_dim;
      if (k < 0 || k >= in_dim) throw_error("index_select: index out of range");
      const std::int64_t in_base = (o*in_dim + k)*inner;
      const std::int64_t out_base = (o*out_dim + j)*inner;
      std::memcpy(out_bytes + out_base*item, in_bytes + in_base*item, (std::size_t)inner*item);
    }
  }
  return out;
}

} // namespace pluss::codem::native::cpu
