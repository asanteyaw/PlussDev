#include "CodeM/native/cpu/ReduceKernels.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include "MSharp/Parallel.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <vector>
#if defined(__APPLE__) && defined(PLUSS_HAS_ACCELERATE) && (PLUSS_HAS_ACCELERATE == 1)
#include <Accelerate/Accelerate.h>
#endif

namespace pluss::codem::native::cpu {

static std::int64_t norm_dim(std::int64_t dim, std::int64_t nd) {
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "reduce: dim out of range");
  return dim;
}

static void shape_for_reduce(const Series& a, std::int64_t dim, std::vector<std::int64_t>& out_sizes,
                             std::int64_t& outer, std::int64_t& reduce, std::int64_t& inner) {
  const auto sz = a.sizes();
  const std::int64_t nd = (std::int64_t)sz.size();
  dim = norm_dim(dim, nd);
  out_sizes.clear();
  for (std::int64_t i=0;i<nd;++i) if (i!=dim) out_sizes.push_back(sz[(size_t)i]);
  outer = 1; for (std::int64_t i=0;i<dim;++i) outer *= sz[(size_t)i];
  reduce = sz[(size_t)dim];
  inner = 1; for (std::int64_t i=dim+1;i<nd;++i) inner *= sz[(size_t)i];
}

template <class AccT, class InT, class OutT, class Load, class Store, class Init, class Step>
static void reduce_dim_impl(const Series& a, std::int64_t dim, Series& out, Load load, Store store, Init init, Step step) {
  std::vector<std::int64_t> out_sz;
  std::int64_t outer, reduce, inner;
  shape_for_reduce(a, dim, out_sz, outer, reduce, inner);

  const InT* ap = reinterpret_cast<const InT*>(a.raw_data());
  OutT* op = reinterpret_cast<OutT*>(out.raw_data());

  const std::int64_t out_elems = outer * inner;
  for (std::int64_t oi=0; oi<out_elems; ++oi) {
    AccT acc = init();
    const std::int64_t o = oi / inner;
    const std::int64_t i = oi - o*inner;
    const std::int64_t base = (o*reduce*inner) + i;
    for (std::int64_t r=0; r<reduce; ++r) {
      const std::int64_t idx = base + r*inner;
      acc = step(acc, load(ap[idx]));
    }
    op[oi] = store(acc, reduce);
  }
}

static float load_f16(Float16 x){ return float16_to_float(x); }
static float load_bf16(BFloat16 x){ return bfloat16_to_float(x); }
static float load_f8e4m3fn(Float8E4M3FN x){ return float8e4m3fn_to_float(x); }
static float load_f8e5m2(Float8E5M2 x){ return float8e5m2_to_float(x); }
static Float16 store_f16(float x){ return float16_from_float(x); }
static BFloat16 store_bf16(float x){ return bfloat16_from_float(x); }
static Float8E4M3FN store_f8e4m3fn(float x){ return float8e4m3fn_from_float(x); }
static Float8E5M2 store_f8e5m2(float x){ return float8e5m2_from_float(x); }

// amax/amin on floating-point data must PROPAGATE NaN (NumPy / PyTorch
// convention): once any NaN is seen the reduction is NaN. std::max/std::min
// silently drop a NaN second argument, so wrap them. Once `acc` is NaN,
// std::max(NaN, x) / std::min(NaN, x) already keep it NaN, so only the
// NaN-second-argument case needs the guard.
template <class T>
static inline T nan_propagating_max(T acc, T x) {
  return std::isnan(x) ? x : std::max(acc, x);
}
template <class T>
static inline T nan_propagating_min(T acc, T x) {
  return std::isnan(x) ? x : std::min(acc, x);
}

template <class T>
static double sum_real_fast_(const T* ap, std::int64_t n) {
  if (n <= 0) return 0.0;

  constexpr std::int64_t kParallelThreshold = (1LL << 20); // 1M elems
#if defined(__APPLE__) && defined(PLUSS_HAS_ACCELERATE) && (PLUSS_HAS_ACCELERATE == 1)
  if constexpr (std::is_same_v<T, float>) {
    if (n < kParallelThreshold) {
      float acc = 0.0f;
      vDSP_sve(ap, 1, &acc, static_cast<vDSP_Length>(n));
      return static_cast<double>(acc);
    }
    const std::int64_t grain = std::max<std::int64_t>(1LL << 18, n / 32);
    const std::int64_t blocks = (n + grain - 1) / grain;
    std::vector<double> partial(static_cast<std::size_t>(blocks), 0.0);
    pluss::msharp::parallel_for(0, blocks, 1, [&](std::int64_t b0, std::int64_t b1) {
      for (std::int64_t b = b0; b < b1; ++b) {
        const std::int64_t start = b * grain;
        const std::int64_t end = std::min<std::int64_t>(n, start + grain);
        float acc = 0.0f;
        vDSP_sve(ap + start, 1, &acc, static_cast<vDSP_Length>(end - start));
        partial[static_cast<std::size_t>(b)] = static_cast<double>(acc);
      }
    });
    double total = 0.0;
    for (double v : partial) total += v;
    return total;
  }
  if constexpr (std::is_same_v<T, double>) {
    if (n < kParallelThreshold) {
      double acc = 0.0;
      vDSP_sveD(ap, 1, &acc, static_cast<vDSP_Length>(n));
      return acc;
    }
    const std::int64_t grain = std::max<std::int64_t>(1LL << 18, n / 32);
    const std::int64_t blocks = (n + grain - 1) / grain;
    std::vector<double> partial(static_cast<std::size_t>(blocks), 0.0);
    pluss::msharp::parallel_for(0, blocks, 1, [&](std::int64_t b0, std::int64_t b1) {
      for (std::int64_t b = b0; b < b1; ++b) {
        const std::int64_t start = b * grain;
        const std::int64_t end = std::min<std::int64_t>(n, start + grain);
        double acc = 0.0;
        vDSP_sveD(ap + start, 1, &acc, static_cast<vDSP_Length>(end - start));
        partial[static_cast<std::size_t>(b)] = acc;
      }
    });
    double total = 0.0;
    for (double v : partial) total += v;
    return total;
  }
#endif

  if (n < kParallelThreshold) {
    double acc0 = 0.0, acc1 = 0.0, acc2 = 0.0, acc3 = 0.0;
    std::int64_t i = 0;
    for (; i + 4 <= n; i += 4) {
      acc0 += static_cast<double>(ap[i + 0]);
      acc1 += static_cast<double>(ap[i + 1]);
      acc2 += static_cast<double>(ap[i + 2]);
      acc3 += static_cast<double>(ap[i + 3]);
    }
    double acc = (acc0 + acc1) + (acc2 + acc3);
    for (; i < n; ++i) acc += static_cast<double>(ap[i]);
    return acc;
  }

  const std::int64_t grain = std::max<std::int64_t>(1LL << 18, n / 32);
  const std::int64_t blocks = (n + grain - 1) / grain;
  std::vector<double> partial(static_cast<std::size_t>(blocks), 0.0);

  pluss::msharp::parallel_for(0, blocks, 1, [&](std::int64_t b0, std::int64_t b1) {
    for (std::int64_t b = b0; b < b1; ++b) {
      const std::int64_t start = b * grain;
      const std::int64_t end = std::min<std::int64_t>(n, start + grain);
      double acc0 = 0.0, acc1 = 0.0, acc2 = 0.0, acc3 = 0.0;
      std::int64_t i = start;
      for (; i + 4 <= end; i += 4) {
        acc0 += static_cast<double>(ap[i + 0]);
        acc1 += static_cast<double>(ap[i + 1]);
        acc2 += static_cast<double>(ap[i + 2]);
        acc3 += static_cast<double>(ap[i + 3]);
      }
      double acc = (acc0 + acc1) + (acc2 + acc3);
      for (; i < end; ++i) acc += static_cast<double>(ap[i]);
      partial[static_cast<std::size_t>(b)] = acc;
    }
  });

  double total = 0.0;
  for (double v : partial) total += v;
  return total;
}

void sum_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "sum_dim: requires contiguous");
  switch (a.scalar_type()) {
    case ScalarType::Bool:
      PLUSS_CHECK(out.scalar_type() == ScalarType::Int64, "sum_dim: bool input expects Int64 output");
      reduce_dim_impl<std::int64_t,std::uint8_t,std::int64_t>(
        a, dim, out,
        [](std::uint8_t x){ return static_cast<std::int64_t>(x != 0); },
        [](std::int64_t acc, std::int64_t){ return acc; },
        [](){ return static_cast<std::int64_t>(0); },
        [](std::int64_t acc, std::int64_t x){ return acc + x; }
      );
      return;
    case ScalarType::Float64:
      reduce_dim_impl<double,double,double>(a,dim,out,[](double x){return x;},[](double acc, std::int64_t){return acc;},[](){return 0.0;},[](double acc,double x){return acc+x;});
      return;
    case ScalarType::Float32:
      reduce_dim_impl<float,float,float>(a,dim,out,[](float x){return x;},[](float acc, std::int64_t){return acc;},[](){return 0.0f;},[](float acc,float x){return acc+x;});
      return;
    case ScalarType::Float16:
      reduce_dim_impl<float,Float16,Float16>(a,dim,out,load_f16,[](float acc, std::int64_t){return store_f16(acc);},[](){return 0.0f;},[](float acc,float x){return acc+x;});
      return;
    case ScalarType::BFloat16:
      reduce_dim_impl<float,BFloat16,BFloat16>(a,dim,out,load_bf16,[](float acc, std::int64_t){return store_bf16(acc);},[](){return 0.0f;},[](float acc,float x){return acc+x;});
      return;
    case ScalarType::Int64:
      reduce_dim_impl<std::int64_t,std::int64_t,std::int64_t>(a,dim,out,[](std::int64_t x){return x;},[](std::int64_t acc, std::int64_t){return acc;},[](){return (std::int64_t)0;},[](std::int64_t acc,std::int64_t x){return acc+x;});
      return;

    case ScalarType::Int32:
      reduce_dim_impl<std::int64_t,std::int32_t,std::int32_t>(a,dim,out,[](std::int32_t x){return (std::int64_t)x;},
        [](std::int64_t acc, std::int64_t){return (std::int32_t)acc;},[](){return (std::int64_t)0;},[](std::int64_t acc,std::int64_t x){return acc+x;});
      return;
    case ScalarType::Int16:
      reduce_dim_impl<std::int64_t,std::int16_t,std::int16_t>(a,dim,out,[](std::int16_t x){return (std::int64_t)x;},
        [](std::int64_t acc, std::int64_t){return (std::int16_t)acc;},[](){return (std::int64_t)0;},[](std::int64_t acc,std::int64_t x){return acc+x;});
      return;
    case ScalarType::Int8:
      reduce_dim_impl<std::int64_t,std::int8_t,std::int8_t>(a,dim,out,[](std::int8_t x){return (std::int64_t)x;},
        [](std::int64_t acc, std::int64_t){return (std::int8_t)acc;},[](){return (std::int64_t)0;},[](std::int64_t acc,std::int64_t x){return acc+x;});
      return;
    case ScalarType::UInt64:
      reduce_dim_impl<std::uint64_t,std::uint64_t,std::uint64_t>(a,dim,out,[](std::uint64_t x){return x;},
        [](std::uint64_t acc, std::int64_t){return acc;},[](){return (std::uint64_t)0;},[](std::uint64_t acc,std::uint64_t x){return acc+x;});
      return;
    case ScalarType::UInt32:
      reduce_dim_impl<std::uint64_t,std::uint32_t,std::uint32_t>(a,dim,out,[](std::uint32_t x){return (std::uint64_t)x;},
        [](std::uint64_t acc, std::int64_t){return (std::uint32_t)acc;},[](){return (std::uint64_t)0;},[](std::uint64_t acc,std::uint64_t x){return acc+x;});
      return;
    case ScalarType::UInt16:
      reduce_dim_impl<std::uint64_t,std::uint16_t,std::uint16_t>(a,dim,out,[](std::uint16_t x){return (std::uint64_t)x;},
        [](std::uint64_t acc, std::int64_t){return (std::uint16_t)acc;},[](){return (std::uint64_t)0;},[](std::uint64_t acc,std::uint64_t x){return acc+x;});
      return;
    case ScalarType::UInt8:
      reduce_dim_impl<std::uint64_t,std::uint8_t,std::uint8_t>(a,dim,out,[](std::uint8_t x){return (std::uint64_t)x;},
        [](std::uint64_t acc, std::int64_t){return (std::uint8_t)acc;},[](){return (std::uint64_t)0;},[](std::uint64_t acc,std::uint64_t x){return acc+x;});
      return;
    case ScalarType::Complex64:
      reduce_dim_impl<Complex64,Complex64,Complex64>(a,dim,out,
        [](Complex64 x){return x;},
        [](Complex64 acc, std::int64_t){return acc;},
        [](){return Complex64(0.0f,0.0f);},
        [](Complex64 acc, Complex64 x){ return acc + x; });
      return;
    case ScalarType::Complex128:
      reduce_dim_impl<Complex128,Complex128,Complex128>(a,dim,out,
        [](Complex128 x){return x;},
        [](Complex128 acc, std::int64_t){return acc;},
        [](){return Complex128(0.0,0.0);},
        [](Complex128 acc, Complex128 x){ return acc + x; });
      return;
default: break;
  }
  throw_error("sum_dim: dtype not implemented");
}

void prod_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "prod_dim: requires contiguous");
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      reduce_dim_impl<double,double,double>(a,dim,out,[](double x){return x;},[](double acc, std::int64_t){return acc;},[](){return 1.0;},[](double acc,double x){return acc*x;});
      return;
    case ScalarType::Float32:
      reduce_dim_impl<float,float,float>(a,dim,out,[](float x){return x;},[](float acc, std::int64_t){return acc;},[](){return 1.0f;},[](float acc,float x){return acc*x;});
      return;

    case ScalarType::Float16:
      reduce_dim_impl<float,Float16,Float16>(a,dim,out,load_f16,[](float acc, std::int64_t){return store_f16(acc);},[](){return 1.0f;},[](float acc,float x){return acc*x;});
      return;
    case ScalarType::BFloat16:
      reduce_dim_impl<float,BFloat16,BFloat16>(a,dim,out,load_bf16,[](float acc, std::int64_t){return store_bf16(acc);},[](){return 1.0f;},[](float acc,float x){return acc*x;});
      return;
    case ScalarType::Int64:
      reduce_dim_impl<std::int64_t,std::int64_t,std::int64_t>(a,dim,out,[](std::int64_t x){return x;},[](std::int64_t acc, std::int64_t){return acc;},[](){return (std::int64_t)1;},[](std::int64_t acc,std::int64_t x){return acc*x;});
      return;
    case ScalarType::Int32:
      reduce_dim_impl<std::int64_t,std::int32_t,std::int32_t>(a,dim,out,[](std::int32_t x){return (std::int64_t)x;},[](std::int64_t acc, std::int64_t){return (std::int32_t)acc;},[](){return (std::int64_t)1;},[](std::int64_t acc,std::int64_t x){return acc*x;});
      return;

    case ScalarType::Complex64:
      reduce_dim_impl<Complex64,Complex64,Complex64>(a,dim,out,
        [](Complex64 x){return x;},
        [](Complex64 acc, std::int64_t){return acc;},
        [](){return Complex64(1.0f,0.0f);},
        [](Complex64 acc, Complex64 x){ return acc * x; });
      return;
    case ScalarType::Complex128:
      reduce_dim_impl<Complex128,Complex128,Complex128>(a,dim,out,
        [](Complex128 x){return x;},
        [](Complex128 acc, std::int64_t){return acc;},
        [](){return Complex128(1.0,0.0);},
        [](Complex128 acc, Complex128 x){ return acc * x; });
      return;
default: break;
  }
  throw_error("prod_dim: dtype not implemented");
}

void amax_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "amax_dim: requires contiguous");
  switch (a.scalar_type()) {
    case ScalarType::Bool:
      reduce_dim_impl<std::uint8_t,std::uint8_t,std::uint8_t>(a,dim,out,[](std::uint8_t x){return x;},[](std::uint8_t acc, std::int64_t){return acc;},[](){return (std::uint8_t)0;},[](std::uint8_t acc,std::uint8_t x){return (std::uint8_t)std::max(acc,x);});
      return;
    case ScalarType::Float64:
      reduce_dim_impl<double,double,double>(a,dim,out,[](double x){return x;},[](double acc, std::int64_t){return acc;},[](){return -std::numeric_limits<double>::infinity();},nan_propagating_max<double>);
      return;
    case ScalarType::Float32:
      reduce_dim_impl<float,float,float>(a,dim,out,[](float x){return x;},[](float acc, std::int64_t){return acc;},[](){return -std::numeric_limits<float>::infinity();},nan_propagating_max<float>);
      return;
    case ScalarType::Int64:
      reduce_dim_impl<std::int64_t,std::int64_t,std::int64_t>(a,dim,out,[](std::int64_t x){return x;},[](std::int64_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int64_t>::min();},[](std::int64_t acc,std::int64_t x){return std::max(acc,x);});
      return;
    case ScalarType::UInt64:
      reduce_dim_impl<std::uint64_t,std::uint64_t,std::uint64_t>(a,dim,out,[](std::uint64_t x){return x;},[](std::uint64_t acc, std::int64_t){return acc;},[](){return (std::uint64_t)0;},[](std::uint64_t acc,std::uint64_t x){return std::max(acc,x);});
      return;

    case ScalarType::Float16:
      reduce_dim_impl<float,Float16,Float16>(a,dim,out,load_f16,[](float acc, std::int64_t){return store_f16(acc);},[](){return -std::numeric_limits<float>::infinity();},nan_propagating_max<float>);
      return;
    case ScalarType::BFloat16:
      reduce_dim_impl<float,BFloat16,BFloat16>(a,dim,out,load_bf16,[](float acc, std::int64_t){return store_bf16(acc);},[](){return -std::numeric_limits<float>::infinity();},nan_propagating_max<float>);
      return;
    case ScalarType::Float8E4M3FN:
      reduce_dim_impl<float,Float8E4M3FN,Float8E4M3FN>(
          a, dim, out, load_f8e4m3fn,
          [](float acc, std::int64_t){ return store_f8e4m3fn(acc); },
          [](){ return -std::numeric_limits<float>::infinity(); },
          nan_propagating_max<float>);
      return;
    case ScalarType::Float8E5M2:
      reduce_dim_impl<float,Float8E5M2,Float8E5M2>(
          a, dim, out, load_f8e5m2,
          [](float acc, std::int64_t){ return store_f8e5m2(acc); },
          [](){ return -std::numeric_limits<float>::infinity(); },
          nan_propagating_max<float>);
      return;
    case ScalarType::Int32:
      reduce_dim_impl<std::int32_t,std::int32_t,std::int32_t>(a,dim,out,[](std::int32_t x){return x;},[](std::int32_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int32_t>::min();},[](std::int32_t acc,std::int32_t x){return std::max(acc,x);});
      return;
    case ScalarType::Int16:
      reduce_dim_impl<std::int16_t,std::int16_t,std::int16_t>(a,dim,out,[](std::int16_t x){return x;},[](std::int16_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int16_t>::min();},[](std::int16_t acc,std::int16_t x){return (std::int16_t)std::max(acc,x);});
      return;
    case ScalarType::Int8:
      reduce_dim_impl<std::int8_t,std::int8_t,std::int8_t>(a,dim,out,[](std::int8_t x){return x;},[](std::int8_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int8_t>::min();},[](std::int8_t acc,std::int8_t x){return (std::int8_t)std::max(acc,x);});
      return;
    case ScalarType::UInt32:
      reduce_dim_impl<std::uint32_t,std::uint32_t,std::uint32_t>(a,dim,out,[](std::uint32_t x){return x;},[](std::uint32_t acc, std::int64_t){return acc;},[](){return (std::uint32_t)0;},[](std::uint32_t acc,std::uint32_t x){return (std::uint32_t)std::max(acc,x);});
      return;
    case ScalarType::UInt16:
      reduce_dim_impl<std::uint16_t,std::uint16_t,std::uint16_t>(a,dim,out,[](std::uint16_t x){return x;},[](std::uint16_t acc, std::int64_t){return acc;},[](){return (std::uint16_t)0;},[](std::uint16_t acc,std::uint16_t x){return (std::uint16_t)std::max(acc,x);});
      return;
    case ScalarType::UInt8:
      reduce_dim_impl<std::uint8_t,std::uint8_t,std::uint8_t>(a,dim,out,[](std::uint8_t x){return x;},[](std::uint8_t acc, std::int64_t){return acc;},[](){return (std::uint8_t)0;},[](std::uint8_t acc,std::uint8_t x){return (std::uint8_t)std::max(acc,x);});
      return;
default: break;
  }
  throw_error("amax_dim: dtype not implemented");
}

void amin_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "amin_dim: requires contiguous");
  switch (a.scalar_type()) {
    case ScalarType::Bool:
      reduce_dim_impl<std::uint8_t,std::uint8_t,std::uint8_t>(a,dim,out,[](std::uint8_t x){return x;},[](std::uint8_t acc, std::int64_t){return acc;},[](){return (std::uint8_t)1;},[](std::uint8_t acc,std::uint8_t x){return (std::uint8_t)std::min(acc,x);});
      return;
    case ScalarType::Float64:
      reduce_dim_impl<double,double,double>(a,dim,out,[](double x){return x;},[](double acc, std::int64_t){return acc;},[](){return std::numeric_limits<double>::infinity();},nan_propagating_min<double>);
      return;
    case ScalarType::Float32:
      reduce_dim_impl<float,float,float>(a,dim,out,[](float x){return x;},[](float acc, std::int64_t){return acc;},[](){return std::numeric_limits<float>::infinity();},nan_propagating_min<float>);
      return;
    case ScalarType::Int64:
      reduce_dim_impl<std::int64_t,std::int64_t,std::int64_t>(a,dim,out,[](std::int64_t x){return x;},[](std::int64_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int64_t>::max();},[](std::int64_t acc,std::int64_t x){return std::min(acc,x);});
      return;
    case ScalarType::UInt64:
      reduce_dim_impl<std::uint64_t,std::uint64_t,std::uint64_t>(a,dim,out,[](std::uint64_t x){return x;},[](std::uint64_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::uint64_t>::max();},[](std::uint64_t acc,std::uint64_t x){return std::min(acc,x);});
      return;

    case ScalarType::Float16:
      reduce_dim_impl<float,Float16,Float16>(a,dim,out,load_f16,[](float acc, std::int64_t){return store_f16(acc);},[](){return std::numeric_limits<float>::infinity();},nan_propagating_min<float>);
      return;
    case ScalarType::BFloat16:
      reduce_dim_impl<float,BFloat16,BFloat16>(a,dim,out,load_bf16,[](float acc, std::int64_t){return store_bf16(acc);},[](){return std::numeric_limits<float>::infinity();},nan_propagating_min<float>);
      return;
    case ScalarType::Float8E4M3FN:
      reduce_dim_impl<float,Float8E4M3FN,Float8E4M3FN>(
          a, dim, out, load_f8e4m3fn,
          [](float acc, std::int64_t){ return store_f8e4m3fn(acc); },
          [](){ return std::numeric_limits<float>::infinity(); },
          nan_propagating_min<float>);
      return;
    case ScalarType::Float8E5M2:
      reduce_dim_impl<float,Float8E5M2,Float8E5M2>(
          a, dim, out, load_f8e5m2,
          [](float acc, std::int64_t){ return store_f8e5m2(acc); },
          [](){ return std::numeric_limits<float>::infinity(); },
          nan_propagating_min<float>);
      return;
    case ScalarType::Int32:
      reduce_dim_impl<std::int32_t,std::int32_t,std::int32_t>(a,dim,out,[](std::int32_t x){return x;},[](std::int32_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int32_t>::max();},[](std::int32_t acc,std::int32_t x){return std::min(acc,x);});
      return;
    case ScalarType::Int16:
      reduce_dim_impl<std::int16_t,std::int16_t,std::int16_t>(a,dim,out,[](std::int16_t x){return x;},[](std::int16_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int16_t>::max();},[](std::int16_t acc,std::int16_t x){return (std::int16_t)std::min(acc,x);});
      return;
    case ScalarType::Int8:
      reduce_dim_impl<std::int8_t,std::int8_t,std::int8_t>(a,dim,out,[](std::int8_t x){return x;},[](std::int8_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::int8_t>::max();},[](std::int8_t acc,std::int8_t x){return (std::int8_t)std::min(acc,x);});
      return;
    case ScalarType::UInt32:
      reduce_dim_impl<std::uint32_t,std::uint32_t,std::uint32_t>(a,dim,out,[](std::uint32_t x){return x;},[](std::uint32_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::uint32_t>::max();},[](std::uint32_t acc,std::uint32_t x){return (std::uint32_t)std::min(acc,x);});
      return;
    case ScalarType::UInt16:
      reduce_dim_impl<std::uint16_t,std::uint16_t,std::uint16_t>(a,dim,out,[](std::uint16_t x){return x;},[](std::uint16_t acc, std::int64_t){return acc;},[](){return std::numeric_limits<std::uint16_t>::max();},[](std::uint16_t acc,std::uint16_t x){return (std::uint16_t)std::min(acc,x);});
      return;
    case ScalarType::UInt8:
      reduce_dim_impl<std::uint8_t,std::uint8_t,std::uint8_t>(a,dim,out,[](std::uint8_t x){return x;},[](std::uint8_t acc, std::int64_t){return acc;},[](){return (std::uint8_t)255;},[](std::uint8_t acc,std::uint8_t x){return (std::uint8_t)std::min(acc,x);});
      return;
default: break;
  }
  throw_error("amin_dim: dtype not implemented");
}

void all_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "all_dim: requires contiguous");
  PLUSS_CHECK(out.scalar_type()==ScalarType::Bool, "all_dim: out must be Bool");
  const auto* ap = reinterpret_cast<const std::uint8_t*>(a.raw_data());
  auto* op = reinterpret_cast<std::uint8_t*>(out.raw_data());
  std::vector<std::int64_t> out_sz; std::int64_t outer, reduce, inner;
  shape_for_reduce(a, dim, out_sz, outer, reduce, inner);
  const std::int64_t out_elems = outer*inner;
  for (std::int64_t oi=0; oi<out_elems; ++oi) {
    const std::int64_t o = oi/inner;
    const std::int64_t i = oi - o*inner;
    const std::int64_t base = o*reduce*inner + i;
    std::uint8_t acc = 1;
    for (std::int64_t r=0;r<reduce;++r) {
      if (ap[base + r*inner]==0) { acc=0; break; }
    }
    op[oi]=acc;
  }
}

void any_dim(const Series& a, std::int64_t dim, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "any_dim: requires contiguous");
  PLUSS_CHECK(out.scalar_type()==ScalarType::Bool, "any_dim: out must be Bool");
  const auto* ap = reinterpret_cast<const std::uint8_t*>(a.raw_data());
  auto* op = reinterpret_cast<std::uint8_t*>(out.raw_data());
  std::vector<std::int64_t> out_sz; std::int64_t outer, reduce, inner;
  shape_for_reduce(a, dim, out_sz, outer, reduce, inner);
  const std::int64_t out_elems = outer*inner;
  for (std::int64_t oi=0; oi<out_elems; ++oi) {
    const std::int64_t o = oi/inner;
    const std::int64_t i = oi - o*inner;
    const std::int64_t base = o*reduce*inner + i;
    std::uint8_t acc = 0;
    for (std::int64_t r=0;r<reduce;++r) {
      if (ap[base + r*inner]!=0) { acc=1; break; }
    }
    op[oi]=acc;
  }
}

void sum_all(const Series& a, Series& out) {
  PLUSS_CHECK(a.is_contiguous(), "sum_all: requires contiguous");
  PLUSS_CHECK(out.numel()==1, "sum_all: out must be scalar");
  switch (a.scalar_type()) {
    case ScalarType::Bool: {
      const auto* ap = reinterpret_cast<const std::uint8_t*>(a.raw_data());
      std::int64_t acc = 0;
      for (std::int64_t i = 0; i < a.numel(); ++i) acc += (ap[i] != 0) ? 1 : 0;
      if (out.scalar_type() == ScalarType::Int64) {
        *reinterpret_cast<std::int64_t*>(out.raw_data()) = acc;
        return;
      }
      if (out.scalar_type() == ScalarType::UInt64) {
        *reinterpret_cast<std::uint64_t*>(out.raw_data()) = static_cast<std::uint64_t>(acc);
        return;
      }
      throw_error("sum_all: bool input expects Int64/UInt64 output");
      return;
    }
    case ScalarType::Float64: {
      const auto* ap = reinterpret_cast<const double*>(a.raw_data());
      const double acc = sum_real_fast_(ap, a.numel());
      *reinterpret_cast<double*>(out.raw_data()) = acc;
      return;
    }
    case ScalarType::Float32: {
      const auto* ap = reinterpret_cast<const float*>(a.raw_data());
      const float acc = static_cast<float>(sum_real_fast_(ap, a.numel()));
      *reinterpret_cast<float*>(out.raw_data()) = acc;
      return;
    }

    case ScalarType::Float16: {
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      float acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += float16_to_float(ap[i]);
      *reinterpret_cast<Float16*>(out.raw_data()) = float16_from_float(acc);
      return;
    }
    case ScalarType::BFloat16: {
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      float acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += bfloat16_to_float(ap[i]);
      *reinterpret_cast<BFloat16*>(out.raw_data()) = bfloat16_from_float(acc);
      return;
    }
    case ScalarType::Int64: {
      const auto* ap = reinterpret_cast<const std::int64_t*>(a.raw_data());
      std::int64_t acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += ap[i];
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = acc;
      return;
    }
    case ScalarType::Int32: {
      const auto* ap = reinterpret_cast<const std::int32_t*>(a.raw_data());
      std::int64_t acc=0; for (std::int64_t i=0;i<a.numel();++i) acc += ap[i];
      *reinterpret_cast<std::int32_t*>(out.raw_data()) = (std::int32_t)acc;
      return;
    }
    case ScalarType::Complex64: {
      const auto* ap = reinterpret_cast<const Complex64*>(a.raw_data());
      float re=0, im=0; for (std::int64_t i=0;i<a.numel();++i){ re += std::real(ap[i]); im += std::imag(ap[i]); }
      *reinterpret_cast<Complex64*>(out.raw_data()) = Complex64(re,im);
      return;
    }
    case ScalarType::Complex128: {
      const auto* ap = reinterpret_cast<const Complex128*>(a.raw_data());
      double re=0, im=0; for (std::int64_t i=0;i<a.numel();++i){ re += std::real(ap[i]); im += std::imag(ap[i]); }
      *reinterpret_cast<Complex128*>(out.raw_data()) = Complex128(re,im);
      return;
    }
default: break;
  }
  throw_error("sum_all: dtype not implemented");
}

void mean_all(const Series& a, Series& out) {
  sum_all(a, out);
  switch (out.scalar_type()) {
    case ScalarType::Float64: *reinterpret_cast<double*>(out.raw_data()) /= (double)a.numel(); return;
    case ScalarType::Float32: *reinterpret_cast<float*>(out.raw_data()) /= (float)a.numel(); return;
    case ScalarType::Float16: *reinterpret_cast<Float16*>(out.raw_data()) = float16_from_float(float16_to_float(*reinterpret_cast<Float16*>(out.raw_data())) / (float)a.numel()); return;
    case ScalarType::BFloat16: *reinterpret_cast<BFloat16*>(out.raw_data()) = bfloat16_from_float(bfloat16_to_float(*reinterpret_cast<BFloat16*>(out.raw_data())) / (float)a.numel()); return;
    case ScalarType::Complex64:
      *reinterpret_cast<Complex64*>(out.raw_data()) /= static_cast<float>(a.numel());
      return;
    case ScalarType::Complex128:
      *reinterpret_cast<Complex128*>(out.raw_data()) /= static_cast<double>(a.numel());
      return;
    default: break;
  }
  throw_error("mean_all: dtype not implemented");
}

} // namespace
