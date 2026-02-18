#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "MSharp/Parallel.h"

#include <cstring>
#include <vector>
#include <algorithm>

namespace pluss::codem::native::cpu {

static bool can_use_2d_fast_path_(const Series& input, std::int64_t dim, const Series& index) {
  if (!input.is_contiguous() || !index.is_contiguous()) return false;
  if (input.dim() != 2 || index.dim() != 2) return false;
  if (!(dim == 0 || dim == 1)) return false;
  if (dim == 0) return index.sizes()[1] == input.sizes()[1];
  return index.sizes()[0] == input.sizes()[0];
}

static inline void for_each_row_(std::int64_t rows, const std::function<void(std::int64_t, std::int64_t)>& fn) {
  if (rows < 64) {
    fn(0, rows);
    return;
  }
  const std::int64_t grain = std::max<std::int64_t>(8, rows / 16);
  pluss::msharp::parallel_for(0, rows, grain, fn);
}

template <class T>
static void gather_2d_fast_typed_(const T* in_p, std::int64_t in_rows, std::int64_t in_cols,
                                  const std::int64_t* ip, std::int64_t idx_rows, std::int64_t idx_cols,
                                  std::int64_t dim, T* out_p) {
  if (dim == 1) {
    for_each_row_(idx_rows, [&](std::int64_t r0, std::int64_t r1) {
      for (std::int64_t r = r0; r < r1; ++r) {
        const std::int64_t row_base = r * idx_cols;
        const std::int64_t in_row_base = r * in_cols;
        for (std::int64_t c = 0; c < idx_cols; ++c) {
          std::int64_t k = ip[row_base + c];
          if (k < 0) k += in_cols;
          out_p[row_base + c] = in_p[in_row_base + k];
        }
      }
    });
    return;
  }

  for_each_row_(idx_rows, [&](std::int64_t r0, std::int64_t r1) {
    for (std::int64_t r = r0; r < r1; ++r) {
      const std::int64_t row_base = r * idx_cols;
      for (std::int64_t c = 0; c < idx_cols; ++c) {
        std::int64_t k = ip[row_base + c];
        if (k < 0) k += in_rows;
        out_p[row_base + c] = in_p[k * in_cols + c];
      }
    }
  });
}

template <class T>
static void scatter_2d_fast_typed_(T* out_p, std::int64_t out_rows, std::int64_t out_cols,
                                   const std::int64_t* ip, const T* src_p,
                                   std::int64_t idx_rows, std::int64_t idx_cols, std::int64_t dim) {
  if (dim == 1) {
    for_each_row_(idx_rows, [&](std::int64_t r0, std::int64_t r1) {
      for (std::int64_t r = r0; r < r1; ++r) {
        const std::int64_t row_base = r * idx_cols;
        const std::int64_t out_row_base = r * out_cols;
        for (std::int64_t c = 0; c < idx_cols; ++c) {
          std::int64_t k = ip[row_base + c];
          if (k < 0) k += out_cols;
          out_p[out_row_base + k] = src_p[row_base + c];
        }
      }
    });
    return;
  }

  // dim == 0: keep sequential order to preserve deterministic last-write behavior.
  for (std::int64_t r = 0; r < idx_rows; ++r) {
    const std::int64_t row_base = r * idx_cols;
    for (std::int64_t c = 0; c < idx_cols; ++c) {
      std::int64_t k = ip[row_base + c];
      if (k < 0) k += out_rows;
      out_p[k * out_cols + c] = src_p[row_base + c];
    }
  }
}

static void gather_2d_fast_(const Series& input, std::int64_t dim, const Series& index, Series& out) {
  const auto item = input.dtype().itemsize;
  const auto* in_b = input.raw_data();
  const auto* ip = reinterpret_cast<const std::int64_t*>(index.raw_data());
  auto* out_b = const_cast<std::byte*>(out.raw_data());

  const std::int64_t in_rows = input.sizes()[0];
  const std::int64_t in_cols = input.sizes()[1];
  const std::int64_t idx_rows = index.sizes()[0];
  const std::int64_t idx_cols = index.sizes()[1];

  const std::int64_t total = idx_rows * idx_cols;
  const std::int64_t dim_bound = (dim == 1) ? in_cols : in_rows;
  for (std::int64_t i = 0; i < total; ++i) {
    std::int64_t k = ip[i];
    if (k < 0) k += dim_bound;
    PLUSS_CHECK(k >= 0 && k < dim_bound, "gather: index out of range");
  }

  switch (input.scalar_type()) {
    case ScalarType::Float32:
      gather_2d_fast_typed_(reinterpret_cast<const float*>(in_b), in_rows, in_cols, ip, idx_rows, idx_cols, dim,
                            reinterpret_cast<float*>(out_b));
      return;
    case ScalarType::Float64:
      gather_2d_fast_typed_(reinterpret_cast<const double*>(in_b), in_rows, in_cols, ip, idx_rows, idx_cols, dim,
                            reinterpret_cast<double*>(out_b));
      return;
    default: break;
  }

  if (dim == 1) {
    for_each_row_(idx_rows, [&](std::int64_t r0, std::int64_t r1) {
      for (std::int64_t r = r0; r < r1; ++r) {
        const std::int64_t row_base = r * idx_cols;
        const std::int64_t in_row_base = r * in_cols;
        for (std::int64_t c = 0; c < idx_cols; ++c) {
          std::int64_t k = ip[row_base + c];
          if (k < 0) k += in_cols;
          std::memcpy(out_b + (std::ptrdiff_t)(row_base + c) * (std::ptrdiff_t)item,
                      in_b + (std::ptrdiff_t)(in_row_base + k) * (std::ptrdiff_t)item,
                      item);
        }
      }
    });
    return;
  }

  for_each_row_(idx_rows, [&](std::int64_t r0, std::int64_t r1) {
    for (std::int64_t r = r0; r < r1; ++r) {
      const std::int64_t row_base = r * idx_cols;
      for (std::int64_t c = 0; c < idx_cols; ++c) {
        std::int64_t k = ip[row_base + c];
        if (k < 0) k += in_rows;
        std::memcpy(out_b + (std::ptrdiff_t)(row_base + c) * (std::ptrdiff_t)item,
                    in_b + (std::ptrdiff_t)(k * in_cols + c) * (std::ptrdiff_t)item,
                    item);
      }
    }
  });
}

static void scatter_2d_fast_(Series& out, std::int64_t dim, const Series& index, const Series& src) {
  const auto item = out.dtype().itemsize;
  const auto* sp = src.raw_data();
  const auto* ip = reinterpret_cast<const std::int64_t*>(index.raw_data());
  auto* out_b = const_cast<std::byte*>(out.raw_data());

  const std::int64_t out_rows = out.sizes()[0];
  const std::int64_t out_cols = out.sizes()[1];
  const std::int64_t idx_rows = index.sizes()[0];
  const std::int64_t idx_cols = index.sizes()[1];

  const std::int64_t total = idx_rows * idx_cols;
  const std::int64_t dim_bound = (dim == 1) ? out_cols : out_rows;
  for (std::int64_t i = 0; i < total; ++i) {
    std::int64_t k = ip[i];
    if (k < 0) k += dim_bound;
    PLUSS_CHECK(k >= 0 && k < dim_bound, "scatter: index out of range");
  }

  switch (out.scalar_type()) {
    case ScalarType::Float32:
      scatter_2d_fast_typed_(reinterpret_cast<float*>(out_b), out_rows, out_cols, ip,
                             reinterpret_cast<const float*>(sp), idx_rows, idx_cols, dim);
      return;
    case ScalarType::Float64:
      scatter_2d_fast_typed_(reinterpret_cast<double*>(out_b), out_rows, out_cols, ip,
                             reinterpret_cast<const double*>(sp), idx_rows, idx_cols, dim);
      return;
    default: break;
  }

  if (dim == 1) {
    for_each_row_(idx_rows, [&](std::int64_t r0, std::int64_t r1) {
      for (std::int64_t r = r0; r < r1; ++r) {
        const std::int64_t row_base = r * idx_cols;
        const std::int64_t out_row_base = r * out_cols;
        for (std::int64_t c = 0; c < idx_cols; ++c) {
          std::int64_t k = ip[row_base + c];
          if (k < 0) k += out_cols;
          std::memcpy(out_b + (std::ptrdiff_t)(out_row_base + k) * (std::ptrdiff_t)item,
                      sp + (std::ptrdiff_t)(row_base + c) * (std::ptrdiff_t)item,
                      item);
        }
      }
    });
    return;
  }

  // dim == 0: keep sequential order to preserve deterministic last-write behavior.
  for (std::int64_t r = 0; r < idx_rows; ++r) {
    const std::int64_t row_base = r * idx_cols;
    for (std::int64_t c = 0; c < idx_cols; ++c) {
      std::int64_t k = ip[row_base + c];
      if (k < 0) k += out_rows;
      std::memcpy(out_b + (std::ptrdiff_t)(k * out_cols + c) * (std::ptrdiff_t)item,
                  sp + (std::ptrdiff_t)(row_base + c) * (std::ptrdiff_t)item,
                  item);
    }
  }
}

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

  if (can_use_2d_fast_path_(in, dim, idx) && out.is_contiguous()) {
    gather_2d_fast_(in, dim, idx, out);
    return out;
  }

  const auto dt = in.dtype();
  const auto* in_b = in.raw_data();
  const auto* ip = reinterpret_cast<const std::int64_t*>(idx.raw_data());
  auto* out_b = const_cast<std::byte*>(out.raw_data());

  // Compute strides for contiguous row-major
  auto in_sizes = std::vector<std::int64_t>(in.sizes().begin(), in.sizes().end());
  auto out_sizes = std::vector<std::int64_t>(idx.sizes().begin(), idx.sizes().end());
  auto strides = contiguous_strides(in_sizes); // elements

  const std::int64_t dim_size = in_sizes[(std::size_t)dim];
  const std::int64_t total = out.numel();

  // linear -> multi-index, then replace idx at dim from index tensor and compute input offset
  std::vector<std::int64_t> multi((std::size_t)nd, 0);

  auto lin_to_multi = [&](std::int64_t lin){
    for (std::int64_t d = nd - 1; d >= 0; --d) {
      const auto s = out_sizes[(std::size_t)d];
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

  if (can_use_2d_fast_path_(out, dim, idx) && ss.is_contiguous()) {
    scatter_2d_fast_(out, dim, idx, ss);
    return out;
  }

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
