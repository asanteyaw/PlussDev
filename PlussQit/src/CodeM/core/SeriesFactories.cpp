#include "MSharp/Parallel.h"
#include "CodeM/core/OpsInternal.h"

// OpsInternal.cpp defines Series member functions; it requires the full Series type.
#include "CodeM/core/Series.h"
// Series::rrelu() has no ops_internal free function (its fused single-node
// form lives in compat/Invoke.cpp), so it forwards through the same
// _compat::invoke registry the generated gelu()/leaky_relu() wrappers use.
#include "CodeM/compat/Invoke.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "CodeM/native/cpu/ReduceKernels.h"
#include "CodeM/native/cpu/WhereClampKernels.h"
#include "CodeM/native/cpu/ActivationKernels.h"
#include "CodeM/native/cpu/SoftmaxKernels.h"
#include "CodeM/native/cpu/ScatterKernels.h"
#include "CodeM/native/cpu/ScatterOps.h"
#include "CodeM/native/cpu/Reductions.h"
#include "CodeM/native/cpu/GatherScatterKernels.h"
#include "CodeM/native/cpu/ConcatStackKernels.h"
#include "CodeM/native/cpu/CumKernels.h"
#include "CodeM/native/cpu/AdvancedIndexingKernels.h"
#include "CodeM/native/cpu/IndexingExtras.h"
#include "CodeM/native/cpu/GroupedReductions.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/ViewInvariants.h"
#include "CodeM/core/SeriesTape.h"
#if __has_include(<execinfo.h>)
#include <execinfo.h>
#define PLUSS_HAVE_EXECINFO 1
#endif
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <set>
#include "CodeM/native/cpu/KernelTable.h"
#include "CodeM/native/cpu/ArithmeticKernels.h"
#include "CodeM/native/cpu/BitwiseKernels.h"
#include "CodeM/native/cpu/UnaryBasicOps.h"
#include "CodeM/native/cpu/UnaryMathKernels.h"
#include "CodeM/native/cpu/UnaryMoreKernels.h"
#include "CodeM/native/cpu/UnaryExtraMathKernels.h"
#include "CodeM/native/cpu/BinaryExtraMathKernels.h"
#include "CodeM/native/mps/Elementwise.h"
#include "CodeM/native/mps/BufferCache.h"
#include "CodeM/native/cuda/CUDARuntime.h"
#include "CodeM/native/cuda/ElementwiseOps.h"
#include "CodeM/native/cuda/FactoryOps.h"
#include "CodeM/native/cuda/ReductionOps.h"
#include "CodeM/native/cuda/ScanOps.h"
#include "CodeM/native/cuda/MaskingOps.h"
#include "CodeM/native/cuda/BitwiseOps.h"
#include "CodeM/native/cuda/ComposeOps.h"
#include "CodeM/native/cuda/CountingOps.h"
#include "CodeM/native/cuda/IndexingOps.h"
#include "CodeM/native/cuda/ScatterGatherOps.h"
#include "CodeM/native/mps/Gather.h"
#include "CodeM/native/mps/MPSRuntime.h"
#include "CodeM/native/mps/Cast.h"
#include "CodeM/native/mps/Cum.h"
#include "CodeM/native/mps/Reductions.h"
#include "CodeM/native/mps/Scatter.h"
#include "CodeM/native/mps/Nonzero.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/Half.h"
#include "pluss/codem/stack_array.h"
#include "pluss/codem/series_factory.h"
#include "pluss/tils/kit/rng_seed.h"

#ifdef PLUSS_ENABLE_TORCH
#include "pluss/codem/torch_interop.h"
#endif

// The primitive Series constructors (pluss::codem::factory::*). pluss::series::* in pluss/codem/series_factory.h is the public
// face and delegates here. Kept apart from OpsInternal.cpp, which holds the operations that belong to a Series object.
namespace pluss::codem {

using namespace ops_internal;

static std::uint64_t splitmix64_(std::uint64_t& x) {
  x += 0x9e3779b97f4a7c15ULL;
  std::uint64_t z = x;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}
static double u01_(std::uint64_t& state) {
  const std::uint64_t r = splitmix64_(state);
  // 53-bit mantissa
  return (double)((r >> 11) * (1.0 / 9007199254740992.0));
}


Series factory::empty(pluss::msharp::RangeView::Int sizes, ScalarType st) {
  return empty(sizes, st, scoped_platform_or_cpu_());
}

Series factory::empty(pluss::msharp::RangeView::Int sizes, ScalarType st, Platform platform) {
  if (platform == Platform::CUDA) {
    if (!native::cuda::runtime_available()) {
      PLUSS_FAIL(
          std::string("pluss::codem::factory::empty(..., CUDA): CUDA runtime unavailable: ") +
          native::cuda::runtime_error_cstr());
    }
  }
  if (platform == Platform::MPS) {
    if (!native::mps::runtime_available()) {
      PLUSS_FAIL(
          std::string("pluss::codem::factory::empty(..., MPS): MPS runtime unavailable: ") +
          native::mps::runtime_error_cstr());
    }
    PLUSS_CHECK(st != ScalarType::Float64,
                "pluss::codem::factory::empty(..., MPS): float64 is unsupported on MPS; use float32/float16/bfloat16");
  }
  const auto dt = dtype_of(st);
  const auto n = compute_numel(sizes);
  const auto bytes = std::max<std::size_t>(
      1, static_cast<std::size_t>(n) * dt.itemsize);
  const bool cuda_device_storage =
      platform == Platform::CUDA &&
      (native::cuda::device_storage_enabled() ||
       !native::cuda::cpu_bridge_enabled() ||
       native::cuda::native_elementwise_enabled() ||
       native::cuda::native_reduction_enabled() ||
       native::cuda::native_factory_enabled());
  Storage storage = cuda_device_storage
      ? Storage::cuda_device(bytes)
      : Storage(bytes);
  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      std::move(storage), 0,
      std::vector<std::int64_t>(sizes.begin(), sizes.end()),
      contiguous_strides(sizes),
      st,
      platform);
  return SeriesFactoryAccess::wrap(std::move(impl));
}

Series factory::zeros(pluss::msharp::RangeView::Int sizes, ScalarType st) {
  auto t = empty(sizes, st);
  const auto dt = t.dtype();
  const auto nbytes = static_cast<std::size_t>(t.numel()) * dt.itemsize;
  if (t.platform() == Platform::CUDA &&
      t.storage().location() == StorageLocation::CudaDevice) {
    if (!native::cuda::memset_device(t.raw_data(), 0, nbytes)) {
      PLUSS_FAIL("pluss::codem::factory::zeros(..., CUDA): cudaMemset failed");
    }
    return t;
  }
  std::memset(t.raw_data(), 0, nbytes);
  return t;
}

Series factory::zeros(pluss::msharp::RangeView::Int sizes, ScalarType st, Platform platform) {
  ops_internal::PlatformScope platform_scope{platform};
  return zeros(sizes, st);
}

Series factory::ones(pluss::msharp::RangeView::Int sizes, ScalarType st) {
  auto t = empty(sizes, st);
  if (t.platform() == Platform::CUDA &&
      t.storage().location() == StorageLocation::CudaDevice) {
    if (!native::cuda::run_fill_factory(t, 1.0)) {
      PLUSS_FAIL("pluss::codem::factory::ones(..., CUDA): native CUDA fill kernel is unsupported for dtype");
    }
    return t;
  }
  const auto n = t.numel();
  switch (st) {
    case ScalarType::Float64: {
      auto* p = reinterpret_cast<double*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=1.0;
      return t;
    }
    case ScalarType::Float32: {
      auto* p = reinterpret_cast<float*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=1.0f;
      return t;
    }
    case ScalarType::Float16: {
      auto* p = reinterpret_cast<std::uint16_t*>(t.raw_data());
      // 1.0 in IEEE half is 0x3C00
      for (std::int64_t i=0;i<n;++i) p[i]=0x3C00u;
      return t;
    }
    case ScalarType::BFloat16: {
      auto* p = reinterpret_cast<std::uint16_t*>(t.raw_data());
      // 1.0f in bfloat16 is upper16(0x3F800000) => 0x3F80
      for (std::int64_t i=0;i<n;++i) p[i]=0x3F80u;
      return t;
    }
    case ScalarType::Int64: {
      auto* p = reinterpret_cast<std::int64_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=1;
      return t;
    }
    case ScalarType::Int32: {
      auto* p = reinterpret_cast<std::int32_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=1;
      return t;
    }
    case ScalarType::Int16: {
      auto* p = reinterpret_cast<std::int16_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=1;
      return t;
    }
    case ScalarType::Int8: {
      auto* p = reinterpret_cast<std::int8_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=1;
      return t;
    }
    case ScalarType::UInt8: {
      auto* p = reinterpret_cast<std::uint8_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=1;
      return t;
    }
    case ScalarType::Bool: {
      auto* p = reinterpret_cast<std::uint8_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=1;
      return t;
    }
    case ScalarType::Complex64: {
      auto* p = reinterpret_cast<Complex64*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i] = Complex64(1.0f, 0.0f);
      return t;
    }
    case ScalarType::Complex128: {
      auto* p = reinterpret_cast<Complex128*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i] = Complex128(1.0, 0.0);
      return t;
    }
    default:
      break;
  }
  PLUSS_FAIL("ones: dtype not implemented");
}

Series factory::ones(pluss::msharp::RangeView::Int sizes, ScalarType st, Platform platform) {
  ops_internal::PlatformScope platform_scope{platform};
  return ones(sizes, st);
}

Series factory::full(pluss::msharp::RangeView::Int sizes, double value, ScalarType st) {
  auto t = empty(sizes, st);
  if (t.platform() == Platform::CUDA &&
      t.storage().location() == StorageLocation::CudaDevice) {
    if (value == 0.0) {
      const auto nbytes = static_cast<std::size_t>(t.numel()) * t.dtype().itemsize;
      if (!native::cuda::memset_device(t.raw_data(), 0, nbytes)) {
        PLUSS_FAIL("pluss::codem::factory::full(..., CUDA, 0): cudaMemset failed");
      }
      return t;
    }
    if (!native::cuda::run_fill_factory(t, value)) {
      PLUSS_FAIL("pluss::codem::factory::full(..., CUDA): native CUDA fill kernel is unsupported for dtype");
    }
    return t;
  }
  const auto n = t.numel();
  switch (st) {
    case ScalarType::Float64: {
      auto* p = reinterpret_cast<double*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<double>(value);
      return t;
    }
    case ScalarType::Float32: {
      auto* p = reinterpret_cast<float*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<float>(value);
      return t;
    }
    case ScalarType::Float16: {
      auto* p = reinterpret_cast<std::uint16_t*>(t.raw_data());
      // use to() conversion path via float32 helper
      Series tmp = pluss::codem::factory::full(sizes, value, ScalarType::Float32);
      auto half = tmp.to(ScalarType::Float16);
      std::memcpy(p, half.raw_data(), (std::size_t)n * 2);
      return t;
    }
    case ScalarType::BFloat16: {
      auto* p = reinterpret_cast<std::uint16_t*>(t.raw_data());
      Series tmp = pluss::codem::factory::full(sizes, value, ScalarType::Float32);
      auto bf = tmp.to(ScalarType::BFloat16);
      std::memcpy(p, bf.raw_data(), (std::size_t)n * 2);
      return t;
    }
    case ScalarType::Int64: {
      auto* p = reinterpret_cast<std::int64_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<std::int64_t>(value);
      return t;
    }
    case ScalarType::Int32: {
      auto* p = reinterpret_cast<std::int32_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<std::int32_t>(value);
      return t;
    }
    case ScalarType::Int16: {
      auto* p = reinterpret_cast<std::int16_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<std::int16_t>(value);
      return t;
    }
    case ScalarType::Int8: {
      auto* p = reinterpret_cast<std::int8_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<std::int8_t>(value);
      return t;
    }
    case ScalarType::UInt64: {
      auto* p = reinterpret_cast<std::uint64_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<std::uint64_t>(value);
      return t;
    }
    case ScalarType::UInt32: {
      auto* p = reinterpret_cast<std::uint32_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<std::uint32_t>(value);
      return t;
    }
    case ScalarType::UInt16: {
      auto* p = reinterpret_cast<std::uint16_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<std::uint16_t>(value);
      return t;
    }
    case ScalarType::UInt8: {
      auto* p = reinterpret_cast<std::uint8_t*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i]=static_cast<std::uint8_t>(value);
      return t;
    }
    case ScalarType::Bool: {
      auto* p = reinterpret_cast<std::uint8_t*>(t.raw_data());
      const std::uint8_t bit = (value == 0.0) ? 0U : 1U;
      for (std::int64_t i=0;i<n;++i) p[i]=bit;
      return t;
    }
    case ScalarType::Float8E4M3FN: {
      auto* p = reinterpret_cast<Float8E4M3FN*>(t.raw_data());
      const float v = static_cast<float>(value);
      const Float8E4M3FN q = float8e4m3fn_from_float(v);
      for (std::int64_t i=0;i<n;++i) p[i]=q;
      return t;
    }
    case ScalarType::Float8E5M2: {
      auto* p = reinterpret_cast<Float8E5M2*>(t.raw_data());
      const float v = static_cast<float>(value);
      const Float8E5M2 q = float8e5m2_from_float(v);
      for (std::int64_t i=0;i<n;++i) p[i]=q;
      return t;
    }
    case ScalarType::Complex64: {
      auto* p = reinterpret_cast<Complex64*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i] = Complex64((float)value, 0.0f);
      return t;
    }
    case ScalarType::Complex128: {
      auto* p = reinterpret_cast<Complex128*>(t.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i] = Complex128((double)value, 0.0);
      return t;
    }
    default:
      break;
  }
  PLUSS_FAIL("full: dtype not implemented");
}

Series factory::full(pluss::msharp::RangeView::Int sizes, double value, ScalarType st, Platform platform) {
  ops_internal::PlatformScope platform_scope{platform};
  return full(sizes, value, st);
}

Series factory::arange(double start, double end, double step, ScalarType dtype) {
  if (step == 0.0) PLUSS_FAIL("arange: step cannot be 0");
  const double nreal = (end - start) / step;
  std::int64_t n = (std::int64_t)std::ceil(nreal);
  if (n < 0) n = 0;
  Series out = pluss::codem::factory::empty({n}, dtype);
  if (out.platform() == Platform::CUDA &&
      out.storage().location() == StorageLocation::CudaDevice) {
    if (!native::cuda::run_arange_factory(out, start, step)) {
      PLUSS_FAIL("pluss::codem::factory::arange(..., CUDA): native CUDA arange kernel is unsupported for dtype");
    }
    return out;
  }
  for (std::int64_t i=0;i<n;++i) {
    const double v = start + (double)i * step;
    // reuse to() storage helper via scalar temporary
    // write directly:
    switch (dtype) {
      case ScalarType::Float64: reinterpret_cast<double*>(out.raw_data())[i] = v; break;
      case ScalarType::Float32: reinterpret_cast<float*>(out.raw_data())[i] = (float)v; break;
      case ScalarType::Float16: reinterpret_cast<Float16*>(out.raw_data())[i] = float16_from_float((float)v); break;
      case ScalarType::BFloat16: reinterpret_cast<BFloat16*>(out.raw_data())[i] = bfloat16_from_float((float)v); break;
      case ScalarType::Int64: reinterpret_cast<std::int64_t*>(out.raw_data())[i] = (std::int64_t)v; break;
      case ScalarType::Int32: reinterpret_cast<std::int32_t*>(out.raw_data())[i] = (std::int32_t)v; break;
      case ScalarType::Int16: reinterpret_cast<std::int16_t*>(out.raw_data())[i] = (std::int16_t)v; break;
      case ScalarType::Int8: reinterpret_cast<std::int8_t*>(out.raw_data())[i] = (std::int8_t)v; break;
      case ScalarType::UInt64: reinterpret_cast<std::uint64_t*>(out.raw_data())[i] = (std::uint64_t)v; break;
      case ScalarType::UInt32: reinterpret_cast<std::uint32_t*>(out.raw_data())[i] = (std::uint32_t)v; break;
      case ScalarType::UInt16: reinterpret_cast<std::uint16_t*>(out.raw_data())[i] = (std::uint16_t)v; break;
      case ScalarType::UInt8: reinterpret_cast<std::uint8_t*>(out.raw_data())[i] = (std::uint8_t)v; break;
      case ScalarType::Bool: reinterpret_cast<std::uint8_t*>(out.raw_data())[i] = (v != 0.0) ? 1 : 0; break;
      default: PLUSS_FAIL("arange: dtype not implemented");
    }
  }
  return out;
}

Series factory::arange(double start, double end, double step, ScalarType dtype, Platform platform) {
  ops_internal::PlatformScope platform_scope{platform};
  return arange(start, end, step, dtype);
}

Series factory::linspace(double start, double end, std::int64_t steps, ScalarType dtype) {
  if (steps < 0) PLUSS_FAIL("linspace: steps must be >= 0");
  Series out = pluss::codem::factory::empty({steps}, dtype);
  if (steps == 0) return out;
  if (steps == 1) {
    return pluss::codem::factory::full({1}, start, dtype, out.platform());
  }
  if (out.platform() == Platform::CUDA &&
      out.storage().location() == StorageLocation::CudaDevice) {
    if (!native::cuda::run_linspace_factory(out, start, end)) {
      PLUSS_FAIL("pluss::codem::factory::linspace(..., CUDA): native CUDA linspace kernel is unsupported for dtype");
    }
    return out;
  }
  const double denom = (double)(steps - 1);
  for (std::int64_t i=0;i<steps;++i) {
    const double t = (double)i / denom;
    const double v = start + t * (end - start);
    switch (dtype) {
      case ScalarType::Float64: reinterpret_cast<double*>(out.raw_data())[i] = v; break;
      case ScalarType::Float32: reinterpret_cast<float*>(out.raw_data())[i] = (float)v; break;
      case ScalarType::Float16: reinterpret_cast<Float16*>(out.raw_data())[i] = float16_from_float((float)v); break;
      case ScalarType::BFloat16: reinterpret_cast<BFloat16*>(out.raw_data())[i] = bfloat16_from_float((float)v); break;
      default: PLUSS_FAIL("linspace: dtype not implemented (float types only for now)");
    }
  }
  return out;
}

Series factory::linspace(double start, double end, std::int64_t steps, ScalarType dtype, Platform platform) {
  ops_internal::PlatformScope platform_scope{platform};
  return linspace(start, end, steps, dtype);
}

Series factory::eye(std::int64_t n, std::int64_t m, ScalarType dtype) {
  if (m < 0) {
    m = n;
  }
  if (n < 0 || m < 0) PLUSS_FAIL("eye: sizes must be >= 0");
  Series out = pluss::codem::factory::zeros({n,m}, dtype);
  if (out.platform() == Platform::CUDA &&
      out.storage().location() == StorageLocation::CudaDevice) {
    if (!native::cuda::run_eye_factory(out, n, m)) {
      PLUSS_FAIL("pluss::codem::factory::eye(..., CUDA): native CUDA eye kernel is unsupported for dtype");
    }
    return out;
  }
  const std::int64_t k = (n < m) ? n : m;
  for (std::int64_t i=0;i<k;++i) {
    const std::int64_t idx = i * out.strides()[0] + i * out.strides()[1];
    switch (dtype) {
      case ScalarType::Float64: reinterpret_cast<double*>(out.raw_data())[idx] = 1.0; break;
      case ScalarType::Float32: reinterpret_cast<float*>(out.raw_data())[idx] = 1.0f; break;
      case ScalarType::Float16: reinterpret_cast<Float16*>(out.raw_data())[idx] = float16_from_float(1.0f); break;
      case ScalarType::BFloat16: reinterpret_cast<BFloat16*>(out.raw_data())[idx] = bfloat16_from_float(1.0f); break;
      case ScalarType::Int64: reinterpret_cast<std::int64_t*>(out.raw_data())[idx] = 1; break;
      case ScalarType::Int32: reinterpret_cast<std::int32_t*>(out.raw_data())[idx] = 1; break;
      case ScalarType::Int16: reinterpret_cast<std::int16_t*>(out.raw_data())[idx] = 1; break;
      case ScalarType::Int8: reinterpret_cast<std::int8_t*>(out.raw_data())[idx] = 1; break;
      case ScalarType::UInt64: reinterpret_cast<std::uint64_t*>(out.raw_data())[idx] = 1; break;
      case ScalarType::UInt32: reinterpret_cast<std::uint32_t*>(out.raw_data())[idx] = 1; break;
      case ScalarType::UInt16: reinterpret_cast<std::uint16_t*>(out.raw_data())[idx] = 1; break;
      case ScalarType::UInt8: reinterpret_cast<std::uint8_t*>(out.raw_data())[idx] = 1; break;
      case ScalarType::Bool: reinterpret_cast<std::uint8_t*>(out.raw_data())[idx] = 1; break;
      default: PLUSS_FAIL("eye: dtype not implemented");
    }
  }
  return out;
}

Series factory::eye(std::int64_t n, std::int64_t m, ScalarType dtype, Platform platform) {
  ops_internal::PlatformScope platform_scope{platform};
  return eye(n, m, dtype);
}

Series factory::rand(pluss::msharp::RangeView::Int sizes, ScalarType dtype, std::uint64_t seed) {
  if (!(dtype == ScalarType::Float32 || dtype == ScalarType::Float64 || dtype == ScalarType::Float16 || dtype == ScalarType::BFloat16))
    PLUSS_FAIL("rand: dtype must be float");
  Series out = pluss::codem::factory::empty(sizes, dtype);
  auto state = pluss::tils::seed_or_next(seed);
  const auto n = out.numel();
  for (std::int64_t i=0;i<n;++i) {
    const float v = (float)u01_(state);
    if (dtype == ScalarType::Float64) reinterpret_cast<double*>(out.raw_data())[i] = (double)v;
    else if (dtype == ScalarType::Float32) reinterpret_cast<float*>(out.raw_data())[i] = v;
    else if (dtype == ScalarType::Float16) reinterpret_cast<Float16*>(out.raw_data())[i] = float16_from_float(v);
    else reinterpret_cast<BFloat16*>(out.raw_data())[i] = bfloat16_from_float(v);
  }
  return out;
}

Series factory::randn(pluss::msharp::RangeView::Int sizes, ScalarType dtype, std::uint64_t seed) {
  if (!(dtype == ScalarType::Float32 || dtype == ScalarType::Float64 || dtype == ScalarType::Float16 || dtype == ScalarType::BFloat16))
    PLUSS_FAIL("randn: dtype must be float");
  Series out = pluss::codem::factory::empty(sizes, dtype);
  auto state = pluss::tils::seed_or_next(seed);
  const auto n = out.numel();
  for (std::int64_t i=0;i<n; i+=2) {
    const double u1 = std::max(1e-12, u01_(state));
    const double u2 = u01_(state);
    const double r = std::sqrt(-2.0 * std::log(u1));
    const double th = 2.0 * 3.14159265358979323846 * u2;
    const float z0 = (float)(r * std::cos(th));
    const float z1 = (float)(r * std::sin(th));
    auto store = [&](std::int64_t j, float v) {
      if (dtype == ScalarType::Float64) reinterpret_cast<double*>(out.raw_data())[j] = (double)v;
      else if (dtype == ScalarType::Float32) reinterpret_cast<float*>(out.raw_data())[j] = v;
      else if (dtype == ScalarType::Float16) reinterpret_cast<Float16*>(out.raw_data())[j] = float16_from_float(v);
      else reinterpret_cast<BFloat16*>(out.raw_data())[j] = bfloat16_from_float(v);
    };
    store(i, z0);
    if (i+1 < n) store(i+1, z1);
  }
  return out;
}

Series factory::rand(pluss::msharp::RangeView::Int sizes, ScalarType dtype, std::uint64_t seed, Platform platform) {
  if (platform != Platform::CUDA) {
    ops_internal::PlatformScope platform_scope{platform};
    return rand(sizes, dtype, seed);
  }
  PLUSS_CHECK(dtype == ScalarType::Float32 || dtype == ScalarType::Float64,
              "rand(..., CUDA): native CUDA rand supports float32/float64 in this tranche");
  Series out = pluss::codem::factory::empty(sizes, dtype, Platform::CUDA);
  if (!native::cuda::run_uniform_factory(out, seed, 0.0, 1.0)) {
    PLUSS_FAIL("pluss::codem::factory::rand(..., CUDA): native CUDA uniform kernel failed");
  }
  return out;
}

Series factory::randn(pluss::msharp::RangeView::Int sizes, ScalarType dtype, std::uint64_t seed, Platform platform) {
  if (platform != Platform::CUDA) {
    ops_internal::PlatformScope platform_scope{platform};
    return randn(sizes, dtype, seed);
  }
  PLUSS_CHECK(dtype == ScalarType::Float32 || dtype == ScalarType::Float64,
              "randn(..., CUDA): native CUDA randn supports float32/float64 in this tranche");
  Series out = pluss::codem::factory::empty(sizes, dtype, Platform::CUDA);
  if (!native::cuda::run_normal_factory(out, seed, 0.0, 1.0)) {
    PLUSS_FAIL("pluss::codem::factory::randn(..., CUDA): native CUDA normal kernel failed");
  }
  return out;
}

Series factory::from_storage(const Storage& storage, std::int64_t storage_offset,
                            const std::vector<std::int64_t>& sizes,
                            const std::vector<std::int64_t>& strides,
                            ScalarType dtype,
                            Platform platform) {
  if (!storage.defined()) PLUSS_FAIL("from_storage: undefined storage");
  // Interop and view metadata must describe a valid non-overlapping layout.
  // Holes are legal: column and stepped slices are intentionally non-dense.
  check_view_invariants(sizes, strides);
  return ops_internal::SeriesFactoryAccess::wrap(
      pluss::msharp::make_intrusive<SeriesImpl>(storage, storage_offset, sizes, strides, dtype, platform));
}

} // namespace pluss::codem
