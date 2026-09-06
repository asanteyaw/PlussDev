#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/native/cpu/ReduceKernels.h"
#include "CodeM/native/cpu/Reductions.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <cstring>

namespace pluss::codem::native::cpu {

// argmax/argmin on floating-point data must PROPAGATE NaN: the first NaN in the
// scan wins and holds (matching the value channel and the NumPy / PyTorch
// convention). A plain `v > best` / `v < best` comparison is always false for a
// NaN `v`, so a NaN would otherwise be skipped entirely.
template <class T>
static inline bool arg_better_max(T v, T best) {
  return std::isnan(v) ? !std::isnan(best) : (v > best);
}
template <class T>
static inline bool arg_better_min(T v, T best) {
  return std::isnan(v) ? !std::isnan(best) : (v < best);
}

// Implemented in ReduceAndIndex.cpp.
Series sum(const Series& a, std::int64_t dim, bool keepdim);

static float load_f16_(Float16 x) { return float16_to_float(x); }
static float load_bf16_(BFloat16 x) { return bfloat16_to_float(x); }
static float load_f8e4m3fn_(Float8E4M3FN x) { return float8e4m3fn_to_float(x); }
static float load_f8e5m2_(Float8E5M2 x) { return float8e5m2_to_float(x); }

template <class CmpT, class InT, class Load, class Better, class Init>
static void arg_reduce_dim_impl_(
    const Series& a,
    std::int64_t dim,
    Series& out,
    Load load,
    Better better,
    Init init_best) {
  const auto nd = a.dim();
  if (dim < 0) dim += nd;

  const auto reduce_n = a.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  for (std::int64_t i = 0; i < dim; ++i) outer *= a.sizes()[(std::size_t)i];
  std::int64_t inner = 1;
  for (std::int64_t i = dim + 1; i < nd; ++i) inner *= a.sizes()[(std::size_t)i];

  const auto* p = reinterpret_cast<const InT*>(a.raw_data());
  auto* q = reinterpret_cast<std::int64_t*>(out.raw_data());
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      CmpT best = init_best();
      std::int64_t arg = 0;
      const std::int64_t base = o * reduce_n * inner + i;
      for (std::int64_t r = 0; r < reduce_n; ++r) {
        const CmpT v = static_cast<CmpT>(load(p[base + r * inner]));
        if (better(v, best)) {
          best = v;
          arg = r;
        }
      }
      q[o * inner + i] = arg;
    }
  }
}

Series mean(const Series& a, std::int64_t dim, bool keepdim) {
  auto s = sum(a, dim, keepdim);
  // divide by reduction size
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  const auto reduce_n = aa.sizes()[(std::size_t)dim];

  if (s.scalar_type() == ScalarType::Float64) {
    auto* p = reinterpret_cast<double*>(s.raw_data());
    for (std::int64_t i=0;i<s.numel();++i) p[i] /= (double)reduce_n;
    return s;
  }
  if (s.scalar_type() == ScalarType::Float32) {
    auto* p = reinterpret_cast<float*>(s.raw_data());
    for (std::int64_t i=0;i<s.numel();++i) p[i] /= (float)reduce_n;
    return s;
  }
  throw_error("mean: dtype not implemented");
}

Series amax(const Series& a, std::int64_t dim, bool keepdim) {
  if (!a.defined()) throw_error("amax: undefined");
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (nd == 0) return aa.clone();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("amax: dim out of range");

  // output sizes
  auto out_sizes = std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  Series out = Series::empty(out_sizes, aa.scalar_type());
  amax_dim(aa, dim, out);
  return out;
}

Series argmax(const Series& a, std::int64_t dim, bool keepdim) {
  if (!a.defined()) throw_error("argmax: undefined");
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (nd == 0) {
    Series out = Series::zeros({}, ScalarType::Int64);
    *reinterpret_cast<std::int64_t*>(out.raw_data()) = 0;
    return out;
  }
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("argmax: dim out of range");
  if (aa.sizes()[(std::size_t)dim] == 0) throw_error("argmax: cannot reduce an empty dimension");

  auto out_sizes = std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  Series out = Series::empty(out_sizes, ScalarType::Int64);
  switch (aa.scalar_type()) {
    case ScalarType::Float64:
      arg_reduce_dim_impl_<double,double>(aa, dim, out, [](double x){ return x; }, arg_better_max<double>, [](){ return -std::numeric_limits<double>::infinity(); });
      return out;
    case ScalarType::Float32:
      arg_reduce_dim_impl_<float,float>(aa, dim, out, [](float x){ return x; }, arg_better_max<float>, [](){ return -std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::Float16:
      arg_reduce_dim_impl_<float,Float16>(aa, dim, out, load_f16_, arg_better_max<float>, [](){ return -std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::BFloat16:
      arg_reduce_dim_impl_<float,BFloat16>(aa, dim, out, load_bf16_, arg_better_max<float>, [](){ return -std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::Float8E4M3FN:
      arg_reduce_dim_impl_<float,Float8E4M3FN>(
          aa, dim, out, load_f8e4m3fn_,
          arg_better_max<float>,
          [](){ return -std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::Float8E5M2:
      arg_reduce_dim_impl_<float,Float8E5M2>(
          aa, dim, out, load_f8e5m2_,
          arg_better_max<float>,
          [](){ return -std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::Bool:
      arg_reduce_dim_impl_<std::uint8_t,std::uint8_t>(aa, dim, out, [](std::uint8_t x){ return x; }, [](std::uint8_t v, std::uint8_t best){ return v > best; }, [](){ return (std::uint8_t)0; });
      return out;
    case ScalarType::Int8:
      arg_reduce_dim_impl_<std::int8_t,std::int8_t>(aa, dim, out, [](std::int8_t x){ return x; }, [](std::int8_t v, std::int8_t best){ return v > best; }, [](){ return std::numeric_limits<std::int8_t>::min(); });
      return out;
    case ScalarType::Int16:
      arg_reduce_dim_impl_<std::int16_t,std::int16_t>(aa, dim, out, [](std::int16_t x){ return x; }, [](std::int16_t v, std::int16_t best){ return v > best; }, [](){ return std::numeric_limits<std::int16_t>::min(); });
      return out;
    case ScalarType::Int32:
      arg_reduce_dim_impl_<std::int32_t,std::int32_t>(aa, dim, out, [](std::int32_t x){ return x; }, [](std::int32_t v, std::int32_t best){ return v > best; }, [](){ return std::numeric_limits<std::int32_t>::min(); });
      return out;
    case ScalarType::Int64:
      arg_reduce_dim_impl_<std::int64_t,std::int64_t>(aa, dim, out, [](std::int64_t x){ return x; }, [](std::int64_t v, std::int64_t best){ return v > best; }, [](){ return std::numeric_limits<std::int64_t>::min(); });
      return out;
    case ScalarType::UInt8:
      arg_reduce_dim_impl_<std::uint8_t,std::uint8_t>(aa, dim, out, [](std::uint8_t x){ return x; }, [](std::uint8_t v, std::uint8_t best){ return v > best; }, [](){ return (std::uint8_t)0; });
      return out;
    case ScalarType::UInt16:
      arg_reduce_dim_impl_<std::uint16_t,std::uint16_t>(aa, dim, out, [](std::uint16_t x){ return x; }, [](std::uint16_t v, std::uint16_t best){ return v > best; }, [](){ return (std::uint16_t)0; });
      return out;
    case ScalarType::UInt32:
      arg_reduce_dim_impl_<std::uint32_t,std::uint32_t>(aa, dim, out, [](std::uint32_t x){ return x; }, [](std::uint32_t v, std::uint32_t best){ return v > best; }, [](){ return (std::uint32_t)0; });
      return out;
    case ScalarType::UInt64:
      arg_reduce_dim_impl_<std::uint64_t,std::uint64_t>(aa, dim, out, [](std::uint64_t x){ return x; }, [](std::uint64_t v, std::uint64_t best){ return v > best; }, [](){ return (std::uint64_t)0; });
      return out;
    default:
      break;
  }
  throw_error("argmax: dtype not implemented");
}

} // namespace pluss::codem::native::cpu

namespace pluss::codem::native::cpu {

Series amin(const Series& a, std::int64_t dim, bool keepdim) {
  if (!a.defined()) throw_error("amin: undefined");
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (nd == 0) return aa.clone();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("amin: dim out of range");

  auto out_sizes = std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  Series out = Series::empty(out_sizes, aa.scalar_type());
  amin_dim(aa, dim, out);
  return out;
}

Series argmin(const Series& a, std::int64_t dim, bool keepdim) {
  if (!a.defined()) throw_error("argmin: undefined");
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (nd == 0) {
    Series out = Series::zeros({}, ScalarType::Int64);
    *reinterpret_cast<std::int64_t*>(out.raw_data()) = 0;
    return out;
  }
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("argmin: dim out of range");
  if (aa.sizes()[(std::size_t)dim] == 0) throw_error("argmin: cannot reduce an empty dimension");

  auto out_sizes = std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  Series out = Series::empty(out_sizes, ScalarType::Int64);
  switch (aa.scalar_type()) {
    case ScalarType::Float64:
      arg_reduce_dim_impl_<double,double>(aa, dim, out, [](double x){ return x; }, arg_better_min<double>, [](){ return std::numeric_limits<double>::infinity(); });
      return out;
    case ScalarType::Float32:
      arg_reduce_dim_impl_<float,float>(aa, dim, out, [](float x){ return x; }, arg_better_min<float>, [](){ return std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::Float16:
      arg_reduce_dim_impl_<float,Float16>(aa, dim, out, load_f16_, arg_better_min<float>, [](){ return std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::BFloat16:
      arg_reduce_dim_impl_<float,BFloat16>(aa, dim, out, load_bf16_, arg_better_min<float>, [](){ return std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::Float8E4M3FN:
      arg_reduce_dim_impl_<float,Float8E4M3FN>(
          aa, dim, out, load_f8e4m3fn_,
          arg_better_min<float>,
          [](){ return std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::Float8E5M2:
      arg_reduce_dim_impl_<float,Float8E5M2>(
          aa, dim, out, load_f8e5m2_,
          arg_better_min<float>,
          [](){ return std::numeric_limits<float>::infinity(); });
      return out;
    case ScalarType::Bool:
      arg_reduce_dim_impl_<std::uint8_t,std::uint8_t>(aa, dim, out, [](std::uint8_t x){ return x; }, [](std::uint8_t v, std::uint8_t best){ return v < best; }, [](){ return (std::uint8_t)1; });
      return out;
    case ScalarType::Int8:
      arg_reduce_dim_impl_<std::int8_t,std::int8_t>(aa, dim, out, [](std::int8_t x){ return x; }, [](std::int8_t v, std::int8_t best){ return v < best; }, [](){ return std::numeric_limits<std::int8_t>::max(); });
      return out;
    case ScalarType::Int16:
      arg_reduce_dim_impl_<std::int16_t,std::int16_t>(aa, dim, out, [](std::int16_t x){ return x; }, [](std::int16_t v, std::int16_t best){ return v < best; }, [](){ return std::numeric_limits<std::int16_t>::max(); });
      return out;
    case ScalarType::Int32:
      arg_reduce_dim_impl_<std::int32_t,std::int32_t>(aa, dim, out, [](std::int32_t x){ return x; }, [](std::int32_t v, std::int32_t best){ return v < best; }, [](){ return std::numeric_limits<std::int32_t>::max(); });
      return out;
    case ScalarType::Int64:
      arg_reduce_dim_impl_<std::int64_t,std::int64_t>(aa, dim, out, [](std::int64_t x){ return x; }, [](std::int64_t v, std::int64_t best){ return v < best; }, [](){ return std::numeric_limits<std::int64_t>::max(); });
      return out;
    case ScalarType::UInt8:
      arg_reduce_dim_impl_<std::uint8_t,std::uint8_t>(aa, dim, out, [](std::uint8_t x){ return x; }, [](std::uint8_t v, std::uint8_t best){ return v < best; }, [](){ return std::numeric_limits<std::uint8_t>::max(); });
      return out;
    case ScalarType::UInt16:
      arg_reduce_dim_impl_<std::uint16_t,std::uint16_t>(aa, dim, out, [](std::uint16_t x){ return x; }, [](std::uint16_t v, std::uint16_t best){ return v < best; }, [](){ return std::numeric_limits<std::uint16_t>::max(); });
      return out;
    case ScalarType::UInt32:
      arg_reduce_dim_impl_<std::uint32_t,std::uint32_t>(aa, dim, out, [](std::uint32_t x){ return x; }, [](std::uint32_t v, std::uint32_t best){ return v < best; }, [](){ return std::numeric_limits<std::uint32_t>::max(); });
      return out;
    case ScalarType::UInt64:
      arg_reduce_dim_impl_<std::uint64_t,std::uint64_t>(aa, dim, out, [](std::uint64_t x){ return x; }, [](std::uint64_t v, std::uint64_t best){ return v < best; }, [](){ return std::numeric_limits<std::uint64_t>::max(); });
      return out;
    default:
      break;
  }
  throw_error("argmin: dtype not implemented");
}

} // namespace pluss::codem::native::cpu


namespace pluss::codem::native::cpu {


Series sum_all(const Series& a) {
  if (!a.defined()) throw_error("sum: undefined");
  auto aa = a.contiguous();
  const auto n = aa.numel();

  auto make_scalar = [&](ScalarType st) {
    return Series::empty({}, st);
  };

  switch (aa.scalar_type()) {
    case ScalarType::Float64: {
      const auto* p = reinterpret_cast<const double*>(aa.raw_data());
      double s = 0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Float64);
      *reinterpret_cast<double*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Float32: {
      const auto* p = reinterpret_cast<const float*>(aa.raw_data());
      double s = 0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Float32);
      *reinterpret_cast<float*>(out.raw_data()) = (float)s;
      return out;
    }
    case ScalarType::Float16: {
      const auto* p = reinterpret_cast<const Float16*>(aa.raw_data());
      double s = 0; for (std::int64_t i=0;i<n;++i) s += float16_to_float(p[i]);
      auto out = make_scalar(ScalarType::Float16);
      *reinterpret_cast<Float16*>(out.raw_data()) = float16_from_float((float)s);
      return out;
    }
    case ScalarType::BFloat16: {
      const auto* p = reinterpret_cast<const BFloat16*>(aa.raw_data());
      double s = 0; for (std::int64_t i=0;i<n;++i) s += bfloat16_to_float(p[i]);
      auto out = make_scalar(ScalarType::BFloat16);
      *reinterpret_cast<BFloat16*>(out.raw_data()) = bfloat16_from_float((float)s);
      return out;
    }
    case ScalarType::Bool: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      std::uint64_t s = 0; for (std::int64_t i=0;i<n;++i) s += (p[i] != 0);
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::UInt8: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      std::uint64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::UInt16: {
      const auto* p = reinterpret_cast<const std::uint16_t*>(aa.raw_data());
      std::uint64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::UInt32: {
      const auto* p = reinterpret_cast<const std::uint32_t*>(aa.raw_data());
      std::uint64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::UInt64: {
      const auto* p = reinterpret_cast<const std::uint64_t*>(aa.raw_data());
      std::uint64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::UInt64);
      *reinterpret_cast<std::uint64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Int8: {
      const auto* p = reinterpret_cast<const std::int8_t*>(aa.raw_data());
      std::int64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Int16: {
      const auto* p = reinterpret_cast<const std::int16_t*>(aa.raw_data());
      std::int64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Int32: {
      const auto* p = reinterpret_cast<const std::int32_t*>(aa.raw_data());
      std::int64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = s;
      return out;
    }
    case ScalarType::Int64: {
      const auto* p = reinterpret_cast<const std::int64_t*>(aa.raw_data());
      std::int64_t s=0; for (std::int64_t i=0;i<n;++i) s += p[i];
      auto out = make_scalar(ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = s;
      return out;
    }
    default:
      break;
  }
  throw_error("sum_all: dtype not implemented");
}


Series all_all(const Series& a) {
  if (!a.defined()) throw_error("all: undefined");
  auto aa = a.contiguous();
  const auto n = aa.numel();
  bool ok = true;

  auto step = [&](bool v) {
    ok = ok && v;
  };

  switch (aa.scalar_type()) {
    case ScalarType::Bool: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt8: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt16: {
      const auto* p = reinterpret_cast<const std::uint16_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt32: {
      const auto* p = reinterpret_cast<const std::uint32_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt64: {
      const auto* p = reinterpret_cast<const std::uint64_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int8: {
      const auto* p = reinterpret_cast<const std::int8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int16: {
      const auto* p = reinterpret_cast<const std::int16_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int32: {
      const auto* p = reinterpret_cast<const std::int32_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int64: {
      const auto* p = reinterpret_cast<const std::int64_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Float32: {
      const auto* p = reinterpret_cast<const float*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0.0f);
      break;
    }
    case ScalarType::Float64: {
      const auto* p = reinterpret_cast<const double*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0.0);
      break;
    }
    case ScalarType::Float16: {
      const auto* p = reinterpret_cast<const Float16*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(float16_to_float(p[i]) != 0.0f);
      break;
    }
    case ScalarType::BFloat16: {
      const auto* p = reinterpret_cast<const BFloat16*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(bfloat16_to_float(p[i]) != 0.0f);
      break;
    }
    default:
      throw_error("all_all: dtype not implemented");
  }

  Series out = Series::empty({}, ScalarType::Bool);
  *reinterpret_cast<std::uint8_t*>(out.raw_data()) = ok ? 1 : 0;
  return out;
}


Series any_all(const Series& a) {
  if (!a.defined()) throw_error("any: undefined");
  auto aa = a.contiguous();
  const auto n = aa.numel();
  bool ok = false;

  auto step = [&](bool v) {
    ok = ok || v;
  };

  switch (aa.scalar_type()) {
    case ScalarType::Bool: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt8: {
      const auto* p = reinterpret_cast<const std::uint8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt16: {
      const auto* p = reinterpret_cast<const std::uint16_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt32: {
      const auto* p = reinterpret_cast<const std::uint32_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::UInt64: {
      const auto* p = reinterpret_cast<const std::uint64_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int8: {
      const auto* p = reinterpret_cast<const std::int8_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int16: {
      const auto* p = reinterpret_cast<const std::int16_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int32: {
      const auto* p = reinterpret_cast<const std::int32_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Int64: {
      const auto* p = reinterpret_cast<const std::int64_t*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0);
      break;
    }
    case ScalarType::Float32: {
      const auto* p = reinterpret_cast<const float*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0.0f);
      break;
    }
    case ScalarType::Float64: {
      const auto* p = reinterpret_cast<const double*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(p[i] != 0.0);
      break;
    }
    case ScalarType::Float16: {
      const auto* p = reinterpret_cast<const Float16*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(float16_to_float(p[i]) != 0.0f);
      break;
    }
    case ScalarType::BFloat16: {
      const auto* p = reinterpret_cast<const BFloat16*>(aa.raw_data());
      for (std::int64_t i=0;i<n;++i) step(bfloat16_to_float(p[i]) != 0.0f);
      break;
    }
    default:
      throw_error("any_all: dtype not implemented");
  }

  Series out = Series::empty({}, ScalarType::Bool);
  *reinterpret_cast<std::uint8_t*>(out.raw_data()) = ok ? 1 : 0;
  return out;
}

} // namespace pluss::codem::native::cpu
