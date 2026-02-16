#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"

#include <vector>
#include <cstring>

namespace pluss::codem::native::cpu {

static void check_same_shape_except(const std::vector<Series>& xs, std::int64_t dim) {
  if (xs.empty()) throw_error("cat: empty list");
  const auto nd = xs[0].dim();
  for (const auto& t: xs) {
    if (t.dim() != nd) throw_error("cat: rank mismatch");
  }
  for (std::int64_t d=0; d<nd; ++d) {
    if (d == dim) continue;
    const auto ref = xs[0].sizes()[(std::size_t)d];
    for (const auto& t: xs) if (t.sizes()[(std::size_t)d] != ref) throw_error("cat: shape mismatch");
  }
}

Series cat(const std::vector<Series>& tensors, std::int64_t dim) {
  if (tensors.empty()) throw_error("cat: empty");
  const auto nd = tensors[0].dim();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("cat: dim out of range");

  // dtype/device checks
  const auto st = tensors[0].scalar_type();
  for (const auto& t: tensors) {
    if (!t.defined()) throw_error("cat: undefined tensor");
    if (t.scalar_type() != st) throw_error("cat: dtype mismatch");
  }
  check_same_shape_except(tensors, dim);

  auto out_sizes = std::vector<std::int64_t>(tensors[0].sizes().begin(), tensors[0].sizes().end());
  std::int64_t sum = 0;
  for (const auto& t: tensors) sum += t.sizes()[(std::size_t)dim];
  out_sizes[(std::size_t)dim] = sum;

  Series out = Series::empty(out_sizes, st);

  // contiguous copies along dim using blocks
  auto dt = out.dtype();
  auto out_c = out.contiguous(); // will be contiguous anyway
  auto* out_b = const_cast<std::byte*>(out_c.raw_data());

  // compute inner block size (elements) after dim
  std::int64_t inner = 1;
  for (std::int64_t d = dim+1; d < nd; ++d) inner *= out_sizes[(std::size_t)d];

  std::int64_t outer = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= out_sizes[(std::size_t)d];

  std::int64_t offset_dim = 0;
  for (const auto& t0 : tensors) {
    auto t = t0.contiguous();
    const auto cur = t.sizes()[(std::size_t)dim];
    const std::int64_t block_elems = cur * inner;
    const std::size_t block_bytes = (std::size_t)block_elems * dt.itemsize;
    const auto* src = t.raw_data();

    for (std::int64_t o=0;o<outer;++o) {
      const std::int64_t out_base = (o*out_sizes[(std::size_t)dim] + offset_dim) * inner;
      const std::int64_t in_base  = (o*cur) * inner;
      std::memcpy(out_b + (std::ptrdiff_t)out_base * (std::ptrdiff_t)dt.itemsize,
                  src   + (std::ptrdiff_t)in_base  * (std::ptrdiff_t)dt.itemsize,
                  block_bytes);
    }
    offset_dim += cur;
  }
  return out_c;
}

Series stack(const std::vector<Series>& tensors, std::int64_t dim) {
  if (tensors.empty()) throw_error("stack: empty");
  const auto nd = tensors[0].dim();
  if (dim < 0) dim += (nd + 1);
  if (dim < 0 || dim > nd) throw_error("stack: dim out of range");

  // Ensure all same shape
  for (const auto& t: tensors) {
    if (t.dim() != nd) throw_error("stack: rank mismatch");
    for (std::int64_t d=0; d<nd; ++d)
      if (t.sizes()[(std::size_t)d] != tensors[0].sizes()[(std::size_t)d]) throw_error("stack: shape mismatch");
    if (t.scalar_type() != tensors[0].scalar_type()) throw_error("stack: dtype mismatch");
  }

  // Insert new dim of size len
  auto out_sizes = std::vector<std::int64_t>(tensors[0].sizes().begin(), tensors[0].sizes().end());
  out_sizes.insert(out_sizes.begin() + dim, (std::int64_t)tensors.size());

  // Implement as cat of unsqueezed views via reshape (no new API needed): we materialize by copying blocks.
  // We'll build out and fill slice by slice.
  Series out = Series::empty(out_sizes, tensors[0].scalar_type());
  auto out_c = out.contiguous();
  auto dt = out.dtype();
  auto* out_b = const_cast<std::byte*>(out_c.raw_data());

  // inner after dim
  std::int64_t inner = 1;
  for (std::int64_t d = dim+1; d < (std::int64_t)out_sizes.size(); ++d) inner *= out_sizes[(std::size_t)d];
  std::int64_t outer = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= out_sizes[(std::size_t)d];

  const std::int64_t block_elems = inner;
  const std::size_t block_bytes = (std::size_t)block_elems * dt.itemsize;

  for (std::size_t k=0;k<tensors.size();++k) {
    auto t = tensors[k].contiguous();
    const auto* src = t.raw_data();
    for (std::int64_t o=0;o<outer;++o) {
      const std::int64_t out_base = ((o*(std::int64_t)tensors.size() + (std::int64_t)k) * inner);
      const std::int64_t in_base = (o*inner);
      std::memcpy(out_b + (std::ptrdiff_t)out_base * (std::ptrdiff_t)dt.itemsize,
                  src   + (std::ptrdiff_t)in_base  * (std::ptrdiff_t)dt.itemsize,
                  block_bytes);
    }
  }

  return out_c;
}

} // namespace pluss::codem::native::cpu
