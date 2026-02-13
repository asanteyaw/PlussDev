#include "CodeM/native/cpu/SoftmaxKernels.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"

#include <cmath>
#include <algorithm>
#include <limits>

namespace pluss::codem::native::cpu {

static void check_softmax_dtype(const Series& a, const char* name) {
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              std::string(name) + ": complex dtype not supported");
  // accept floating only
  const auto dt = a.scalar_type();
  const bool ok = (dt == ScalarType::Float64 || dt == ScalarType::Float32 || dt == ScalarType::Float16 || dt == ScalarType::BFloat16);
  PLUSS_CHECK(ok, std::string(name) + ": expects floating dtype");
}


template <class T, class AccT>
static Series softmax_impl_contig(const Series& a, std::int64_t dim, bool is_log,
                                  AccT (*to_acc)(T), T (*from_acc)(AccT)) {
  auto aa = a.contiguous();
  const auto nd = (std::int64_t)aa.dim();
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "softmax: bad dim");

  Series out = Series::empty(aa.sizes(), aa.scalar_type());

  std::int64_t outer = 1;
  for (std::int64_t i=0;i<dim;++i) outer *= aa.sizes()[i];
  const std::int64_t dsize = aa.sizes()[dim];
  std::int64_t inner = 1;
  for (std::int64_t i=dim+1;i<nd;++i) inner *= aa.sizes()[i];

  const T* ap = reinterpret_cast<const T*>(aa.raw_data());
  T* op = reinterpret_cast<T*>(out.raw_data());

  for (std::int64_t o=0; o<outer; ++o) {
    for (std::int64_t in=0; in<inner; ++in) {
      AccT mx = -std::numeric_limits<AccT>::infinity();
      for (std::int64_t d=0; d<dsize; ++d) {
        const std::int64_t idx = (o * dsize * inner + d * inner + in);
        mx = std::max(mx, to_acc(ap[idx]));
      }
      AccT sum = (AccT)0;
      for (std::int64_t d=0; d<dsize; ++d) {
        const std::int64_t idx = (o * dsize * inner + d * inner + in);
        sum += std::exp(to_acc(ap[idx]) - mx);
      }
      const AccT lse = mx + std::log(sum);
      for (std::int64_t d=0; d<dsize; ++d) {
        const std::int64_t idx = (o * dsize * inner + d * inner + in);
        const AccT x = to_acc(ap[idx]);
        const AccT v = is_log ? (x - lse) : std::exp(x - lse);
        op[idx] = from_acc(v);
      }
    }
  }
  return out;
}

static float to_acc_f32(float x){ return x; }
static float from_acc_f32(float x){ return x; }
static double to_acc_f64(double x){ return x; }
static double from_acc_f64(double x){ return x; }

template <class LoadF, class StoreF>
static Series softmax_impl(const Series& a, std::int64_t dim, bool is_log, LoadF&& load, StoreF&& store) {
  auto aa = a.contiguous();
  const auto nd = (std::int64_t)aa.dim();
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "softmax: bad dim");

  std::vector<std::int64_t> out_sizes(aa.sizes().begin(), aa.sizes().end());
  Series out = Series::empty(out_sizes, aa.scalar_type());

  // We do a straightforward reduction over dim:
  // outer = product of sizes[0:dim), inner = product of sizes(dim+1:)
  std::int64_t outer = 1;
  for (std::int64_t i=0;i<dim;++i) outer *= aa.sizes()[i];
  const std::int64_t dsize = aa.sizes()[dim];
  std::int64_t inner = 1;
  for (std::int64_t i=dim+1;i<nd;++i) inner *= aa.sizes()[i];

  const auto* ap = reinterpret_cast<const std::byte*>(aa.raw_data());
  auto* op = reinterpret_cast<std::byte*>(out.raw_data());
  const auto item = aa.dtype().itemsize;

  // indexing: base = (o * dsize * inner + d * inner + in) * item
  for (std::int64_t o=0; o<outer; ++o) {
    for (std::int64_t in=0; in<inner; ++in) {
      // 1) max for numerical stability
      float mx = -std::numeric_limits<float>::infinity();
      for (std::int64_t d=0; d<dsize; ++d) {
        const std::int64_t idx = (o * dsize * inner + d * inner + in);
        mx = std::max(mx, load(ap + idx*item));
      }
      // 2) sum exp
      float sum = 0.0f;
      for (std::int64_t d=0; d<dsize; ++d) {
        const std::int64_t idx = (o * dsize * inner + d * inner + in);
        sum += std::exp(load(ap + idx*item) - mx);
      }
      const float lse = mx + std::log(sum);
      // 3) write outputs
      for (std::int64_t d=0; d<dsize; ++d) {
        const std::int64_t idx = (o * dsize * inner + d * inner + in);
        const float x = load(ap + idx*item);
        const float v = is_log ? (x - lse) : std::exp(x - lse);
        store(op + idx*item, v);
      }
    }
  }
  return out;
}

static float load_f32(const std::byte* p) { return *reinterpret_cast<const float*>(p); }
static void store_f32(std::byte* p, float v) { *reinterpret_cast<float*>(p) = v; }

static float load_f64(const std::byte* p) { return (float)*reinterpret_cast<const double*>(p); }
static void store_f64(std::byte* p, float v) { *reinterpret_cast<double*>(p) = (double)v; }

static float load_f16p(const std::byte* p) { return float16_to_float(*reinterpret_cast<const Float16*>(p)); }
static void store_f16p(std::byte* p, float v) { *reinterpret_cast<Float16*>(p) = float16_from_float(v); }

static float load_bf16p(const std::byte* p) { return bfloat16_to_float(*reinterpret_cast<const BFloat16*>(p)); }
static void store_bf16p(std::byte* p, float v) { *reinterpret_cast<BFloat16*>(p) = bfloat16_from_float(v); }

Series softmax(const Series& a, std::int64_t dim) {
  check_softmax_dtype(a, "softmax");
  switch (a.scalar_type()) {
    case ScalarType::Float32: return softmax_impl_contig<float,float>(a, dim, false, to_acc_f32, from_acc_f32);
    case ScalarType::Float64: return softmax_impl_contig<double,double>(a, dim, false, to_acc_f64, from_acc_f64);
    case ScalarType::Float16: return softmax_impl(a, dim, false, load_f16p, store_f16p);
    case ScalarType::BFloat16: return softmax_impl(a, dim, false, load_bf16p, store_bf16p);
    default: break;
  }
  throw_error("softmax: dtype not implemented");
}

Series log_softmax(const Series& a, std::int64_t dim) {
  check_softmax_dtype(a, "log_softmax");
  switch (a.scalar_type()) {
    case ScalarType::Float32: return softmax_impl_contig<float,float>(a, dim, true, to_acc_f32, from_acc_f32);
    case ScalarType::Float64: return softmax_impl_contig<double,double>(a, dim, true, to_acc_f64, from_acc_f64);
    case ScalarType::Float16: return softmax_impl(a, dim, true, load_f16p, store_f16p);
    case ScalarType::BFloat16: return softmax_impl(a, dim, true, load_bf16p, store_bf16p);
    default: break;
  }
  throw_error("log_softmax: dtype not implemented");
}

Series logsumexp(const Series& a, std::int64_t dim, bool keepdim) {
  check_softmax_dtype(a, "logsumexp");
  auto aa = a.contiguous();
  const auto nd = (std::int64_t)aa.dim();
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "logsumexp: bad dim");

  std::vector<std::int64_t> out_sizes;
  out_sizes.reserve((std::size_t)nd - (keepdim?0:1));
  for (std::int64_t i=0;i<nd;++i) {
    if (i == dim) {
      if (keepdim) out_sizes.push_back(1);
    } else out_sizes.push_back(aa.sizes()[i]);
  }
  Series out = Series::empty(out_sizes, aa.scalar_type());

  std::int64_t outer = 1;
  for (std::int64_t i=0;i<dim;++i) outer *= aa.sizes()[i];
  const std::int64_t dsize = aa.sizes()[dim];
  std::int64_t inner = 1;
  for (std::int64_t i=dim+1;i<nd;++i) inner *= aa.sizes()[i];

  const auto* ap = reinterpret_cast<const std::byte*>(aa.raw_data());
  auto* op = reinterpret_cast<std::byte*>(out.raw_data());
  const auto item = aa.dtype().itemsize;

  auto load = [&](const std::byte* p)->float {
    switch (aa.scalar_type()) {
      case ScalarType::Float32: return load_f32(p);
      case ScalarType::Float64: return load_f64(p);
      case ScalarType::Float16: return load_f16p(p);
      case ScalarType::BFloat16: return load_bf16p(p);
      default: return 0.0f;
    }
  };
  auto store = [&](std::byte* p, float v) {
    switch (aa.scalar_type()) {
      case ScalarType::Float32: store_f32(p, v); break;
      case ScalarType::Float64: store_f64(p, v); break;
      case ScalarType::Float16: store_f16p(p, v); break;
      case ScalarType::BFloat16: store_bf16p(p, v); break;
      default: break;
    }
  };

  // out indexing: if keepdim, out has same outer/inner structure; else dim removed.
  for (std::int64_t o=0; o<outer; ++o) {
    for (std::int64_t in=0; in<inner; ++in) {
      float mx = -std::numeric_limits<float>::infinity();
      for (std::int64_t d=0; d<dsize; ++d) {
        const std::int64_t idx = (o * dsize * inner + d * inner + in);
        mx = std::max(mx, load(ap + idx*item));
      }
      float sum = 0.0f;
      for (std::int64_t d=0; d<dsize; ++d) {
        const std::int64_t idx = (o * dsize * inner + d * inner + in);
        sum += std::exp(load(ap + idx*item) - mx);
      }
      const float lse = mx + std::log(sum);

      // compute output offset
      std::int64_t out_idx;
      if (keepdim) {
        out_idx = (o * 1 * inner + 0 * inner + in);
      } else {
        out_idx = (o * inner + in);
      }
      store(op + out_idx * item, lse);
    }
  }
  return out;
}

} // namespace pluss::codem::native::cpu
