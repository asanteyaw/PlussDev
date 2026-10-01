#include "CodeM/native/cpu/SortingKernels.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/TypePromotion.h"
#include "MSharp/Parallel.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <type_traits>
#include <vector>

namespace pluss::codem::native::cpu {

static std::int64_t wrap_dim(std::int64_t dim, std::int64_t nd) {
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "sort/topk: dim out of range");
  return dim;
}

template <class T>
static void sort_rows(const T* in, T* out, std::int64_t* idx_out,
                      std::int64_t rows, std::int64_t cols,
                      bool descending, bool stable) {
  auto worker = [&](std::int64_t r0, std::int64_t r1) {
    std::vector<std::int64_t> idx((std::size_t)cols);
    for (std::int64_t r = r0; r < r1; ++r) {
      const T* row = in + r * cols;
      T* out_row = out + r * cols;
      std::iota(idx.begin(), idx.end(), 0);
      auto cmp = [&](std::int64_t i, std::int64_t j) {
        if constexpr (std::is_floating_point_v<T>) {
          const bool i_nan = std::isnan(row[i]);
          const bool j_nan = std::isnan(row[j]);
          if (i_nan != j_nan) return descending ? i_nan : !i_nan;
        }
        if (descending) return row[i] > row[j];
        return row[i] < row[j];
      };
      if (stable) std::stable_sort(idx.begin(), idx.end(), cmp);
      else std::sort(idx.begin(), idx.end(), cmp);
      for (std::int64_t c = 0; c < cols; ++c) {
        const auto ii = idx[(std::size_t)c];
        out_row[c] = row[ii];
        idx_out[r * cols + c] = ii;
      }
    }
  };
  constexpr std::int64_t kParallelThresholdRows = 64;
  if (rows >= kParallelThresholdRows) {
    const std::int64_t grain = std::max<std::int64_t>(1, rows / 8);
    pluss::msharp::parallel_for(0, rows, grain, worker);
  } else {
    worker(0, rows);
  }
}

template <class T>
static void topk_rows(const T* in, T* out, std::int64_t* idx_out,
                      std::int64_t rows, std::int64_t cols,
                      std::int64_t k, bool largest, bool sorted) {
  auto worker = [&](std::int64_t r0, std::int64_t r1) {
    std::vector<std::int64_t> idx((std::size_t)cols);
    for (std::int64_t r = r0; r < r1; ++r) {
      const T* row = in + r * cols;
      std::iota(idx.begin(), idx.end(), 0);
      auto better = [&](std::int64_t i, std::int64_t j) {
        if constexpr (std::is_floating_point_v<T>) {
          const bool i_nan = std::isnan(row[i]);
          const bool j_nan = std::isnan(row[j]);
          if (i_nan != j_nan) return largest ? i_nan : !i_nan;
        }
        if (largest) return row[i] > row[j];
        return row[i] < row[j];
      };
      if (k == cols) {
        if (sorted) {
          std::sort(idx.begin(), idx.end(), better);
        }
      } else if (sorted) {
        std::partial_sort(idx.begin(), idx.begin() + k, idx.end(), better);
      } else {
        std::nth_element(idx.begin(), idx.begin() + k, idx.end(), better);
      }
      for (std::int64_t c = 0; c < k; ++c) {
        const auto ii = idx[(std::size_t)c];
        out[r * k + c] = row[ii];
        idx_out[r * k + c] = ii;
      }
    }
  };
  constexpr std::int64_t kParallelThresholdRows = 64;
  if (rows >= kParallelThresholdRows) {
    const std::int64_t grain = std::max<std::int64_t>(1, rows / 8);
    pluss::msharp::parallel_for(0, rows, grain, worker);
  } else {
    worker(0, rows);
  }
}

template <class T>
static void argsort_rows(const T* in, std::int64_t* idx_out,
                         std::int64_t rows, std::int64_t cols,
                         bool descending, bool stable) {
  auto worker = [&](std::int64_t r0, std::int64_t r1) {
    std::vector<std::int64_t> idx((std::size_t)cols);
    for (std::int64_t r = r0; r < r1; ++r) {
      const T* row = in + r * cols;
      std::iota(idx.begin(), idx.end(), 0);
      auto cmp = [&](std::int64_t i, std::int64_t j) {
        if constexpr (std::is_floating_point_v<T>) {
          const bool i_nan = std::isnan(row[i]);
          const bool j_nan = std::isnan(row[j]);
          if (i_nan != j_nan) return descending ? i_nan : !i_nan;
        }
        if (descending) return row[i] > row[j];
        return row[i] < row[j];
      };
      if (stable) std::stable_sort(idx.begin(), idx.end(), cmp);
      else std::sort(idx.begin(), idx.end(), cmp);
      std::copy(idx.begin(), idx.end(), idx_out + r * cols);
    }
  };
  constexpr std::int64_t kParallelThresholdRows = 64;
  if (rows >= kParallelThresholdRows) {
    const std::int64_t grain = std::max<std::int64_t>(1, rows / 8);
    pluss::msharp::parallel_for(0, rows, grain, worker);
  } else {
    worker(0, rows);
  }
}

static Series move_dim_to_last_contig(const Series& a, std::int64_t dim) {
  const auto nd = a.dim();
  dim = wrap_dim(dim, nd);
  if (nd == 1 || dim == nd-1) return a.contiguous();
  std::vector<std::int64_t> perm;
  perm.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) if (d != dim) perm.push_back(d);
  perm.push_back(dim);
  return a.permute(perm).contiguous();
}

static std::pair<Series, Series> undo_move_last(const Series& vals_last,
                                                const Series& idx_last,
                                                const Series& orig,
                                                std::int64_t dim) {
  const auto nd = orig.dim();
  dim = wrap_dim(dim, nd);
  if (nd == 1 || dim == nd-1) return {vals_last, idx_last};
  // Inverse permutation from move_dim_to_last_contig
  std::vector<std::int64_t> perm;
  perm.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) if (d != dim) perm.push_back(d);
  perm.push_back(dim);
  std::vector<std::int64_t> inv((std::size_t)nd);
  for (std::int64_t i=0;i<nd;++i) inv[(std::size_t)perm[(std::size_t)i]] = i;
  return { vals_last.permute(inv), idx_last.permute(inv) };
}

std::pair<Series, Series> sort(const Series& a, std::int64_t dim, bool descending, bool stable) {
  PLUSS_CHECK(a.defined(), "sort: undefined");
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "sort: complex dtype not supported (Torch behavior differs); use abs/real/imag first");
  dim = wrap_dim(dim, a.dim());

  auto x = move_dim_to_last_contig(a, dim);
  const auto cols = x.sizes().back();
  std::int64_t rows = 1;
  for (std::size_t i=0;i+1<x.sizes().size();++i) rows *= x.sizes()[i];

  // Explicit Platform::CPU, not the ambient platform scope: this is a
  // CPU-only kernel (namespace pluss::codem::native::cpu) that assumes its
  // input is already on CPU, but the 2-arg Series::empty resolves platform
  // from the current PlatformScope, which can still be MPS when called from
  // an MPS-originated op that first moved its data to CPU for this sort.
  Series vals = Series::empty(x.sizes(), x.scalar_type(), Platform::CPU);
  Series idx = Series::empty(x.sizes(), ScalarType::Int64, Platform::CPU);

  switch (x.scalar_type()) {
    case ScalarType::Float64:
      sort_rows((const double*)x.raw_data(), (double*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::Float32:
      sort_rows((const float*)x.raw_data(), (float*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::Int64:
      sort_rows((const std::int64_t*)x.raw_data(), (std::int64_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::Int32:
      sort_rows((const std::int32_t*)x.raw_data(), (std::int32_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::Int16:
      sort_rows((const std::int16_t*)x.raw_data(), (std::int16_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::Int8:
      sort_rows((const std::int8_t*)x.raw_data(), (std::int8_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::UInt8:
      sort_rows((const std::uint8_t*)x.raw_data(), (std::uint8_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::UInt16:
      sort_rows((const std::uint16_t*)x.raw_data(), (std::uint16_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::UInt32:
      sort_rows((const std::uint32_t*)x.raw_data(), (std::uint32_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::UInt64:
      sort_rows((const std::uint64_t*)x.raw_data(), (std::uint64_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    case ScalarType::Bool:
      sort_rows((const bool*)x.raw_data(), (bool*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, descending, stable);
      break;
    default:
      throw_error("sort: dtype not implemented");
  }

  return undo_move_last(vals, idx, a, dim);
}

Series argsort(const Series& a, std::int64_t dim, bool descending, bool stable) {
  PLUSS_CHECK(a.defined(), "argsort: undefined");
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "argsort: complex dtype not supported");
  dim = wrap_dim(dim, a.dim());

  auto x = move_dim_to_last_contig(a, dim);
  const auto cols = x.sizes().back();
  std::int64_t rows = 1;
  for (std::size_t i = 0; i + 1 < x.sizes().size(); ++i) rows *= x.sizes()[i];

  Series idx = Series::empty(x.sizes(), ScalarType::Int64, Platform::CPU);
  switch (x.scalar_type()) {
    case ScalarType::Float64:
      argsort_rows((const double*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::Float32:
      argsort_rows((const float*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::Int64:
      argsort_rows((const std::int64_t*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::Int32:
      argsort_rows((const std::int32_t*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::Int16:
      argsort_rows((const std::int16_t*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::Int8:
      argsort_rows((const std::int8_t*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::UInt8:
      argsort_rows((const std::uint8_t*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::UInt16:
      argsort_rows((const std::uint16_t*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::UInt32:
      argsort_rows((const std::uint32_t*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::UInt64:
      argsort_rows((const std::uint64_t*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    case ScalarType::Bool:
      argsort_rows((const bool*)x.raw_data(), (std::int64_t*)idx.raw_data(), rows, cols, descending, stable);
      break;
    default:
      throw_error("argsort: dtype not implemented");
  }

  if (a.dim() == 1 || dim == a.dim() - 1) return idx;
  std::vector<std::int64_t> perm;
  perm.reserve((std::size_t)a.dim());
  for (std::int64_t d = 0; d < a.dim(); ++d) if (d != dim) perm.push_back(d);
  perm.push_back(dim);
  std::vector<std::int64_t> inv((std::size_t)a.dim());
  for (std::int64_t i = 0; i < a.dim(); ++i) inv[(std::size_t)perm[(std::size_t)i]] = i;
  return idx.permute(inv);
}

std::pair<Series, Series> topk(const Series& a, std::int64_t k, std::int64_t dim, bool largest, bool sorted_) {
  PLUSS_CHECK(a.defined(), "topk: undefined");
  PLUSS_CHECK(k >= 0, "topk: k must be >= 0");
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "topk: complex dtype not supported");
  dim = wrap_dim(dim, a.dim());

  auto x = move_dim_to_last_contig(a, dim);
  const auto cols = x.sizes().back();
  PLUSS_CHECK(k <= cols, "topk: k must be <= size(dim)");
  std::int64_t rows = 1;
  for (std::size_t i=0;i+1<x.sizes().size();++i) rows *= x.sizes()[i];

  auto out_sizes = std::vector<std::int64_t>(x.sizes().begin(), x.sizes().end());
  out_sizes.back() = k;
  Series vals = Series::empty(out_sizes, x.scalar_type(), Platform::CPU);
  Series idx = Series::empty(out_sizes, ScalarType::Int64, Platform::CPU);

  switch (x.scalar_type()) {
    case ScalarType::Float64:
      topk_rows((const double*)x.raw_data(), (double*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::Float32:
      topk_rows((const float*)x.raw_data(), (float*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::Int64:
      topk_rows((const std::int64_t*)x.raw_data(), (std::int64_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::Int32:
      topk_rows((const std::int32_t*)x.raw_data(), (std::int32_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::Int16:
      topk_rows((const std::int16_t*)x.raw_data(), (std::int16_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::Int8:
      topk_rows((const std::int8_t*)x.raw_data(), (std::int8_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::UInt8:
      topk_rows((const std::uint8_t*)x.raw_data(), (std::uint8_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::UInt16:
      topk_rows((const std::uint16_t*)x.raw_data(), (std::uint16_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::UInt32:
      topk_rows((const std::uint32_t*)x.raw_data(), (std::uint32_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::UInt64:
      topk_rows((const std::uint64_t*)x.raw_data(), (std::uint64_t*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    case ScalarType::Bool:
      topk_rows((const bool*)x.raw_data(), (bool*)vals.raw_data(), (std::int64_t*)idx.raw_data(),
                rows, cols, k, largest, sorted_);
      break;
    default:
      throw_error("topk: dtype not implemented");
  }

  // Need to undo permute with different last dim; reuse inv permute.
  const auto nd = a.dim();
  if (nd == 1 || dim == nd-1) return {vals, idx};
  std::vector<std::int64_t> perm;
  perm.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) if (d != dim) perm.push_back(d);
  perm.push_back(dim);
  std::vector<std::int64_t> inv((std::size_t)nd);
  for (std::int64_t i=0;i<nd;++i) inv[(std::size_t)perm[(std::size_t)i]] = i;
  return { vals.permute(inv), idx.permute(inv) };
}

} // namespace
