#include "CodeM/native/cpu/SortingKernels.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/TypePromotion.h"

#include <algorithm>
#include <numeric>
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
  std::vector<std::int64_t> idx(cols);
  for (std::int64_t r=0;r<rows;++r) {
    const T* row = in + r*cols;
    T* out_row = out + r*cols;
    std::iota(idx.begin(), idx.end(), 0);
    auto cmp = [&](std::int64_t i, std::int64_t j){
      if (descending) return row[i] > row[j];
      return row[i] < row[j];
    };
    if (stable) std::stable_sort(idx.begin(), idx.end(), cmp);
    else std::sort(idx.begin(), idx.end(), cmp);
    for (std::int64_t c=0;c<cols;++c) {
      const auto ii = idx[(std::size_t)c];
      out_row[c] = row[ii];
      idx_out[r*cols + c] = ii;
    }
  }
}

template <class T>
static void topk_rows(const T* in, T* out, std::int64_t* idx_out,
                      std::int64_t rows, std::int64_t cols,
                      std::int64_t k, bool largest, bool sorted) {
  std::vector<std::int64_t> idx(cols);
  for (std::int64_t r=0;r<rows;++r) {
    const T* row = in + r*cols;
    std::iota(idx.begin(), idx.end(), 0);
    auto better = [&](std::int64_t i, std::int64_t j){
      if (largest) return row[i] > row[j];
      return row[i] < row[j];
    };
    // nth_element to get top-k set
    auto mid = idx.begin() + k;
    std::nth_element(idx.begin(), mid, idx.end(), better);
    idx.resize((std::size_t)k);
    if (sorted) {
      std::sort(idx.begin(), idx.end(), better);
    }
    for (std::int64_t c=0;c<k;++c) {
      const auto ii = idx[(std::size_t)c];
      out[r*k + c] = row[ii];
      idx_out[r*k + c] = ii;
    }
    idx.resize((std::size_t)cols);
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

  Series vals = Series::empty(x.sizes(), x.scalar_type());
  Series idx = Series::empty(x.sizes(), ScalarType::Int64);

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
  auto [vals, idx] = sort(a, dim, descending, stable);
  (void)vals;
  return idx;
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
  Series vals = Series::empty(out_sizes, x.scalar_type());
  Series idx = Series::empty(out_sizes, ScalarType::Int64);

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
